/*
COPYRIGHT (C) 2001-2003  Paolo Mantegazza (mantegazza@aero.polimi.it),

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/


/* ethernet support(s) we want to use: 1 -> DO, 0 -> DO NOT */
#define SOFT_RTNET      1
#define HARD_RTNET      0
/* end of ethernet support(s) we want to use */

#define USE_KMOD        0  // if true you do not have run command "urtnet &"
#define SOFT_SOCK_TYPE  SOCK_DGRAM  // in soft mode we can use any type 

#define COMPILE_ANYHOW  // RTNet is not available but we want to compile anyhow

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/version.h>
#include <linux/timer.h>

#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include <net_rpc.h>

MODULE_LICENSE("GPL");

#if SOFT_RTNET && !HARD_RTNET
#include <softrtnet.h>
#define MSG_SOFT 0
#define MSG_HARD 0
#define hard_rt_socket(a, b, c)  portslot[i].socket[0]
#define hard_rt_bind(a, b, c)
#define hard_rt_close(a)
#define hard_rt_socket_callback  soft_rt_socket_callback
#define hard_rt_recvfrom         soft_rt_recvfrom
#define hard_rt_sendto           soft_rt_sendto
#endif

#if !SOFT_RTNET && HARD_RTNET
#ifdef COMPILE_ANYHOW 
#include <softrtnet.h>
#else
#include <rtnet.h>
#endif
#undef SOFT_SOCK_TYPE
#define SOFT_SOCK_TYPE  SOCK_DGRAM
#define MSG_SOFT 1
#define MSG_HARD 1
#define soft_rt_socket           rt_socket
#define soft_rt_bind(a, b, c)    rt_bind(a, b, c)
#define soft_rt_close(a)	 rt_close(a)
#define soft_rt_socket_callback  rt_socket_callback
#define soft_rt_recvfrom         rt_recvfrom
#define soft_rt_sendto           rt_sendto
#define hard_rt_socket(a, b, c)  portslot[i].socket[0]
#define hard_rt_bind(a, b, c)
#define hard_rt_close(a)
#define hard_rt_socket_callback  rt_socket_callback
#define hard_rt_recvfrom         rt_recvfrom
#define hard_rt_sendto           rt_sendto
#endif

#if SOFT_RTNET && HARD_RTNET
#include <softrtnet.h>
#ifndef COMPILE_ANYHOW 
#include <rtnet.h>
#endif
#undef SOFT_SOCK_TYPE
#define SOFT_SOCK_TYPE  SOCK_DGRAM
#define MSG_SOFT 0
#define MSG_HARD 1
#define hard_rt_socket           rt_socket
#define hard_rt_bind             rt_bind
#define hard_rt_close            rt_close
#define hard_rt_socket_callback  rt_socket_callback
#define hard_rt_recvfrom         rt_recvfrom
#define hard_rt_sendto           rt_sendto
#endif

#define LOCALHOST   "127.0.0.1"
#define BASEPORT    5000
#define STACK_SIZE  4000

static int MaxStubs = MAX_STUBS;
MODULE_PARM(MaxStubs, "i");

static int MaxSocks = MAX_SOCKS;
MODULE_PARM(MaxSocks, "i");

static int StackSize = STACK_SIZE;
MODULE_PARM(StackSize, "i");

static char *ThisNode = LOCALHOST;
MODULE_PARM(ThisNode, "s");

static char *ThisSoftNode = 0;
MODULE_PARM(ThisSoftNode, "s");

static char *ThisHardNode = 0;
MODULE_PARM(ThisHardNode, "s");

#define MAX_DFUN_EXT  16
static void *rt_net_rpc_fun[];
static unsigned long long (**rt_net_rpc_fun_ext[MAX_DFUN_EXT])(int, ...) = { (void *)rt_net_rpc_fun, };

static unsigned long this_node[2];

#define PRTSRVNAME  0xFFFFFFFF
struct portslot_t { struct portslot_t *p; int indx; unsigned long long owner; SEM sem; void *msg; int task; struct sockaddr_in addr; MBX *mbx; int socket[2]; int hard; unsigned long name; };
static spinlock_t portslot_lock = SPIN_LOCK_UNLOCKED;
static volatile int portslotsp;
static struct portslot_t *portslot;

static inline struct portslot_t *get_portslot(void)
{
	unsigned long flags;
	struct portslot_t *p;

	flags = rt_spin_lock_irqsave(&portslot_lock);
	if (portslotsp < MaxSocks) {
		p = portslot[portslotsp++].p;
		rt_spin_unlock_irqrestore(flags, &portslot_lock);
		return p;
	}
	rt_spin_unlock_irqrestore(flags, &portslot_lock);
	return 0;
}

static inline int gvb_portslot(struct portslot_t *portslotp)
{
	unsigned long flags;

	flags = rt_spin_lock_irqsave(&portslot_lock);
	if (portslotsp > MaxStubs) {
		portslot[--portslotsp].p = portslotp;
		rt_spin_unlock_irqrestore(flags, &portslot_lock);
		return 0;
	}
	rt_spin_unlock_irqrestore(flags, &portslot_lock);
	return -EINVAL;
}

#define ADRSZ sizeof(struct sockaddr_in)

#define TIMER_FREQ 50
static struct timer_list timer;
static SEM timer_sem;
static spinlock_t req_rel_lock = SPIN_LOCK_UNLOCKED;

static inline int hash_fun(unsigned long long owner)
{
	unsigned short *us;
	us = (unsigned short *)&owner;
	return ((us[0] >> 4) + us[3]) & (MAX_STUBS - 1);
}

static inline int hash_ins(unsigned long long owner)
{
	int i, k;
	unsigned long flags;

	i = hash_fun(owner);
	while (1) {
		k = i;
		while (portslot[k].owner) {
			if ((k = (k + 1) & (MAX_STUBS - 1)) == i) {
				return 0;
			}
		}
		flags = rt_spin_lock_irqsave(&req_rel_lock);
		if (!portslot[k].owner) {
			break;
		}
		rt_spin_unlock_irqrestore(flags, &req_rel_lock);
	}
	portslot[k].owner = owner;
	rt_spin_unlock_irqrestore(flags, &req_rel_lock);
	return k;
}

static inline int hash_find(unsigned long long owner)
{
	int i, k;

	k = i = hash_fun(owner);
	while (portslot[k].owner != owner) {
		if (!portslot[k].owner || (k = (k + 1) & (MAX_STUBS - 1)) == i) {
			return 0;
		}
	}
	return k;
}

static inline int hash_find_if_not_ins(unsigned long long owner)
{
	int i, k;
	unsigned long flags;

	i = hash_fun(owner);
	while (1) {
		k = i;
		while (portslot[k].owner && portslot[k].owner != owner) {
			if ((k = (k + 1) & (MAX_STUBS - 1)) == i) {
				return 0;
			}
		}
		flags = rt_spin_lock_irqsave(&req_rel_lock);
		if (portslot[k].owner == owner) {
			rt_spin_unlock_irqrestore(flags, &req_rel_lock);
			return k;
		} else if (!portslot[k].owner) {
			break;
		}
		rt_spin_unlock_irqrestore(flags, &req_rel_lock);
	}
	portslot[k].owner = owner;
	rt_spin_unlock_irqrestore(flags, &req_rel_lock);
	return k;
}

static inline int hash_rem(unsigned long long owner)
{
	int i, k;
	unsigned long flags;

	i = hash_fun(owner);
	while (1) {
		k = i;
		while (portslot[k].owner != owner) {
			if (!portslot[k].owner || (k = (k + 1) & (MAX_STUBS - 1)) == i) {
				return 0;
			}
		}
		flags = rt_spin_lock_irqsave(&req_rel_lock);
		if (portslot[k].owner == owner) {
			break;
		}
		rt_spin_unlock_irqrestore(flags, &req_rel_lock);
	}
	portslot[k].owner = 0;
	rt_spin_unlock_irqrestore(flags, &req_rel_lock);
	return k;
}

static void timer_fun(unsigned long none)
{
	if (timer_sem.count < 0) {
		rt_sem_broadcast(&timer_sem);
	}
	timer.expires = jiffies + (HZ + TIMER_FREQ/2 - 1)/TIMER_FREQ;
	add_timer(&timer);
}

static int (*encode)(struct portslot_t *portslotp, void *msg, int size, int where);
static int (*decode)(struct portslot_t *portslotp, void *msg, int size, int where);

void set_netrpc_encoding(void *encode_fun, void *decode_fun, void *ext)
{
	encode = encode_fun;
	decode = decode_fun;
        rt_net_rpc_fun_ext[1] = ext;
}

struct req_rel_msg { int op, port, hard; unsigned long long owner; unsigned long name, chkspare;};

static void soft_net_rcv_req_rel_port(int sock, struct portslot_t *portslotp)
{
	int i, rsize;

	if (portslot[0].sem.count < 0) {
		if ((rsize = soft_rt_recvfrom(portslot[0].socket[0], portslot[0].msg, MAX_MSG_SIZE, 0, (struct sockaddr *)portslot[0].task, &i)) > 0) {
			if (decode) {
				decode(&portslot[0], portslot[0].msg, rsize, PRT_SRV);
			}
			rt_sem_signal(&portslot[0].sem);
			return;
		}
	} else {
		struct req_rel_msg trashmsg;
		struct sockaddr_in addr;
		soft_rt_recvfrom(portslot[0].socket[0], &trashmsg, sizeof(trashmsg), 0, (struct sockaddr *)&addr, &i);
	}
}

static void hard_net_rcv_req_rel_port(int sock, struct portslot_t *portslotp)
{
	int i, rsize;

	if (portslot[0].sem.count < 0) {
		if ((rsize = hard_rt_recvfrom(portslot[0].socket[1], portslot[0].msg, MAX_MSG_SIZE, 0, (struct sockaddr *)portslot[0].task, &i)) > 0) {
			if (decode) {
				decode(&portslot[0], portslot[0].msg, rsize, PRT_SRV);
			}
			rt_sem_signal(&portslot[0].sem);
			return;
		}
	} else {
		struct req_rel_msg trashmsg;
		struct sockaddr_in addr;
		hard_rt_recvfrom(portslot[0].socket[1], &trashmsg, sizeof(trashmsg), 0, (struct sockaddr *)&addr, &i);
	}
}

#define MAX_PRIO  99
#define MIN_PRIO   1

static void soft_net_rcv_rpc(int sock, struct portslot_t *portslotp)
{
	int i, rsize;

	if ((rsize = soft_rt_recvfrom(sock, portslotp->msg, MAX_MSG_SIZE, 0, (struct sockaddr *)&portslotp->addr, &i)) > sizeof(int)) {
		RT_TASK *task;
		if (decode) {
			decode(portslotp, portslotp->msg, rsize, RPC_SRV);
		}
		task = (RT_TASK *)portslotp->task;
		if ((i = *((int *)portslotp->msg)) < task->priority) {
			task->priority = i;
			(task->lnxtsk)->rt_priority = i >= MAX_PRIO ? MIN_PRIO : MAX_PRIO - i;
		}
		rt_sem_signal(&portslotp->sem);
	}
}

static void hard_net_rcv_rpc(int sock, struct portslot_t *portslotp)
{
	int i, rsize;

	if ((rsize = hard_rt_recvfrom(sock, portslotp->msg, MAX_MSG_SIZE, 0, (struct sockaddr *)&portslotp->addr, &i)) > sizeof(int)) {
		RT_TASK *task;
		if (decode) {
			decode(portslotp, portslotp->msg, rsize, RPC_SRV);
		}
		task = (RT_TASK *)portslotp->task;
		if ((i = *((int *)portslotp->msg)) < task->priority) {
			task->priority = i;
		}
		rt_sem_signal(&portslotp->sem);
	}
}

extern int get_min_tasks_cpuid(void);
extern int set_rtext(RT_TASK *, int, int, void(*)(void), unsigned int, void *);
extern int clr_rtext(RT_TASK *);
extern void rt_schedule_soft(RT_TASK *);

struct fun_args { int a[10]; long long (*fun)(int, ...); };

static inline int soft_rt_fun_call(RT_TASK *task, void *fun, void *arg)
{
	task->fun_args[0] = (int)arg;
	((struct fun_args *)task->fun_args)->fun = fun;
	rt_schedule_soft(task);
	return (int)task->retval;
}

static inline long long soft_rt_genfun_call(RT_TASK *task, void *fun, void *args, int argsize)
{
	memcpy(task->fun_args, args, argsize);
	((struct fun_args *)task->fun_args)->fun = fun;
	rt_schedule_soft(task);
	return task->retval;
}

static int thread_fun(RT_TASK *task)
{
        current->rt_priority = MIN_PRIO;
        current->policy = SCHED_FIFO;
	if (!set_rtext(task, task->fun_args[2], 0, 0, get_min_tasks_cpuid(), 0)) {
		void (*task_fun)(int) = (void *)task->fun_args[0];
		int arg = task->fun_args[1];
        	soft_rt_fun_call(task, rt_task_suspend, task);
	        task_fun(arg);
		return 0;
	}
	return -1;
}

static int soft_kthread_init(RT_TASK *task, int fun, int arg, int priority)
{
	int retval;
	task->magic = 0;
	(task->fun_args = (int *)(task + 1))[0] = fun;
	task->fun_args[1] = arg;
	task->fun_args[2] = priority;
	if ((retval = kernel_thread((void *)thread_fun, task, 0)) > 0) {
		while (task->state != (READY | SUSPENDED)) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(1);
		}
	}
	return retval;
}

static int soft_kthread_delete(RT_TASK *task)
{
	struct task_struct *lnxtsk = task->lnxtsk;
	if (clr_rtext(task)) {
		return -EFAULT;
	} else {
		lnxtsk->this_rt_task[0] = lnxtsk->this_rt_task[1] = 0;
		lnxtsk->state = TASK_INTERRUPTIBLE;
		kill_proc(lnxtsk->pid, SIGTERM, 0);
	}
	return 0;
}

static void soft_stub_fun(struct portslot_t *portslotp) 
{
	char msg[MAX_MSG_SIZE];
	struct sockaddr *addr;
	RT_TASK *task;
	SEM *sem;
        struct par_t { int priority, base_priority, argsize, rsize, fun_ext_timed; long long type; int a[1]; } *par;
	int *a, wsize, w2size, sock;
	unsigned long name;
	long long type;

	addr = (struct sockaddr *)&portslotp->addr;
	sock = portslotp->socket[0];
	name = portslotp->name;
	sem  = &portslotp->sem;
	a = (par = portslotp->msg = msg)->a;
	task = (RT_TASK *)portslotp->task;
        sprintf(current->comm, "SFTSTB-%d", sock);

	while (!soft_rt_fun_call(task, rt_sem_wait, sem)) {
		task->base_priority = par->base_priority;
		type = par->type;
		if (par->rsize) {
			a[USP_RBF1(type) - 1] = (int)((char *)a + par->argsize);
		}
		if (NEED_TO_W(type)) {
			wsize = USP_WSZ1(type);
			wsize = wsize ? a[wsize - 1] : (USP_WSZ1LL(type) ? sizeof(long long) : sizeof(int));
		} else {
			wsize = 0;
		}
		if (NEED_TO_W2ND(type)) {
			w2size = USP_WSZ2(type);
			w2size = w2size ? a[w2size - 1] : (USP_WSZ2LL(type) ? sizeof(long long) : sizeof(int));
		} else {
			w2size = 0;
		}
		do {
			struct msg_t { int wsize, w2size; unsigned long long retval; char msg_buf[wsize], msg_buf2[w2size]; } arg;
			if (wsize > 0) {
				arg.wsize = wsize;
				a[USP_WBF1(type) - 1] = (int)arg.msg_buf;
			} else {
				arg.wsize = 0;
			}
			if (w2size > 0) {
				arg.w2size = w2size;
				a[USP_WBF2(type) - 1] = (int)arg.msg_buf2;
			} else {
				arg.w2size = 0;
			}
                        if ((wsize = TIMED(par->fun_ext_timed) - 1) >= 0) {
                                *((long long *)(a + wsize)) = nano2count(*((long long *)(a + wsize)));
                        }
			arg.retval = soft_rt_genfun_call(task, rt_net_rpc_fun_ext[EXT(par->fun_ext_timed)][FUN(par->fun_ext_timed)], a, par->argsize);
			soft_rt_sendto(sock, &arg, encode ? encode(portslotp, &arg, sizeof(struct msg_t), RPC_RTR) : sizeof(struct msg_t), 0, addr, ADRSZ);
		} while (0);
	}
	rt_task_suspend(task);
}

static void hard_stub_fun(struct portslot_t *portslotp) 
{
	char msg[MAX_MSG_SIZE];
	struct sockaddr *addr;
	RT_TASK *task;
	SEM *sem;
        struct par_t { int priority, base_priority, argsize, rsize, fun_ext_timed; long long type; int a[1]; } *par;
	int *a, wsize, w2size, sock;
	unsigned long name;
	long long type;

	addr = (struct sockaddr *)&portslotp->addr;
	sock = portslotp->socket[1];
	name = portslotp->name;
	sem  = &portslotp->sem;
	a = (par = portslotp->msg = msg)->a;
	task = (RT_TASK *)portslotp->task;
        sprintf(current->comm, "HRDSTB-%d", sock);

	while (!rt_sem_wait(sem)) {
		task->base_priority = par->base_priority;
		type = par->type;
		if (par->rsize) {
			a[USP_RBF1(type) - 1] = (int)((char *)a + par->argsize);
		}
		if (NEED_TO_W(type)) {
			wsize = USP_WSZ1(type);
			wsize = wsize ? a[wsize - 1] : (USP_WSZ1LL(type) ? sizeof(long long) : sizeof(int));
		} else {
			wsize = 0;
		}
		if (NEED_TO_W2ND(type)) {
			w2size = USP_WSZ2(type);
			w2size = w2size ? a[w2size - 1] : (USP_WSZ2LL(type) ? sizeof(long long) : sizeof(int));
		} else {
			w2size = 0;
		}
		do {
			struct msg_t { int wsize, w2size; unsigned long long retval; char msg_buf[wsize], msg_buf2[w2size]; } arg;
			if (wsize > 0) {
				arg.wsize = wsize;
				a[USP_WBF1(type) - 1] = (int)arg.msg_buf;
			} else {
				arg.wsize = 0;
			}
			if (w2size > 0) {
				arg.w2size = w2size;
				a[USP_WBF2(type) - 1] = (int)arg.msg_buf2;
			} else {
				arg.w2size = 0;
			}
                        if ((wsize = TIMED(par->fun_ext_timed) - 1) >= 0) {
                                *((long long *)(a + wsize)) = nano2count(*((long long *)(a + wsize)));
                        }
			arg.retval = rt_net_rpc_fun_ext[EXT(par->fun_ext_timed)][FUN(par->fun_ext_timed)](a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);
			hard_rt_sendto(sock, &arg, encode ? encode(portslotp, &arg, sizeof(struct msg_t), RPC_RTR) : sizeof(struct msg_t), 0, addr, ADRSZ);
		} while (0);
	}
	rt_task_suspend(task);
}

static void port_server_fun(RT_TASK *port_server)
{
	int i;
	RT_TASK *task;
	unsigned long flags;
	struct sockaddr_in addr;
	struct req_rel_msg msg;

        sprintf(current->comm, "PRTSRV");
	portslot[0].task = (unsigned long)&addr;
	portslot[0].msg = &msg;
while (!soft_rt_fun_call(port_server, rt_sem_wait, &portslot[0].sem)) {
	if (msg.op) {
		i = msg.op - BASEPORT;
		if (i > 0 && i < MaxStubs) {
        		flags = rt_spin_lock_irqsave(&req_rel_lock);
			if (portslot[i].owner == msg.owner) {
				task = (RT_TASK *)portslot[i].task;
				portslot[i].task = 0;
				portslot[i].owner = 0;
				msg.port = msg.op;
       				rt_spin_unlock_irqrestore(flags, &req_rel_lock);
				if (task->is_hard) {
					rt_task_delete(task);
				} else {
					soft_kthread_delete(task);
				}
				rt_free(task);
			} else {
				msg.port = !portslot[i].owner ? msg.op : -ENXIO;
       				rt_spin_unlock_irqrestore(flags, &req_rel_lock);
			}
		} else {
			msg.port = -EINVAL;
		}
		goto ret;
	}
	if ((msg.port = hash_find_if_not_ins(msg.owner)) <= 0) {
		msg.port = -ENODEV;
		goto ret;
	}
	if (!portslot[msg.port].task) {
		if ((task = rt_malloc(sizeof(RT_TASK) + 3*sizeof(struct fun_args)))) {
			if ((msg.hard ? rt_task_init(task, (void *)hard_stub_fun, (int)(portslot + msg.port), StackSize + 2*MAX_MSG_SIZE, RT_LOWEST_PRIORITY, 0, 0) : (soft_kthread_init(task, (int)soft_stub_fun, (int)(portslot + msg.port), RT_LOWEST_PRIORITY) < 0))) {
				rt_free(task);
				task = 0;
			}
		}
		if (!task) {
			portslot[msg.port].owner = 0;
			msg.port = -ENOMEM;
			goto ret;
		}
		portslot[msg.port].name = msg.name;
		portslot[msg.port].task = (unsigned long)(task);
		portslot[msg.port].sem.count = 0;
		portslot[msg.port].sem.queue.prev = 
		portslot[msg.port].sem.queue.next = &portslot[msg.port].sem.queue;
		if (msg.hard) {
			hard_rt_socket_callback(portslot[msg.port].socket[1], (void *)hard_net_rcv_rpc, portslot + msg.port);
		} else {
			soft_rt_socket_callback(portslot[msg.port].socket[0], (void *)soft_net_rcv_rpc, portslot + msg.port);
		}
		rt_task_resume(task);
	}
	msg.port += BASEPORT;
ret:
	if (msg.hard) {
		hard_rt_sendto(portslot[0].socket[1], &msg, encode ? encode(&portslot[0], &msg, sizeof(msg), PRT_RTR) : sizeof(msg), 0, (struct sockaddr *)&addr, ADRSZ);
	} else {
		soft_rt_sendto(portslot[0].socket[0], &msg, encode ? encode(&portslot[0], &msg, sizeof(msg), PRT_RTR) : sizeof(msg), 0, (struct sockaddr *)&addr, ADRSZ);
	}
}
rt_task_suspend(port_server);
}

static void soft_net_rcv_rtr(int sock, struct portslot_t *portslotp)
{
	int i, rsize;
	struct sockaddr addr;

	if (portslotp->task > 0) {
		rsize = soft_rt_recvfrom(sock, portslotp->msg, MAX_MSG_SIZE, 0, &addr, &i);
		if (decode) {
			decode(portslotp, portslotp->msg, rsize, RPC_RCV);
		}
		rt_sem_signal(&portslotp->sem);
		return;
	} 
	if (portslotp->task < 0) {
		rt_sem_signal(&portslotp->sem);
		return;
	} 
	rsize = soft_rt_recvfrom(sock, portslotp->msg, MAX_MSG_SIZE, 0, &addr, &i);
	if (decode) {
		decode(&portslot[0], portslotp->msg, rsize, PRT_RCV);
	}
}

static void hard_net_rcv_rtr(int sock, struct portslot_t *portslotp)
{
	int i, rsize;
	struct sockaddr addr;

	if (portslotp->task > 0) {
		rsize = hard_rt_recvfrom(sock, portslotp->msg, MAX_MSG_SIZE, 0, &addr, &i);
		if (decode) {
			decode(portslotp, portslotp->msg, rsize, RPC_RCV);
		}
		rt_sem_signal(&portslotp->sem);
		return;
	} 
	if (portslotp->task < 0) {
		rt_sem_signal(&portslotp->sem);
		return;
	} 
	rsize = hard_rt_recvfrom(sock, portslotp->msg, MAX_MSG_SIZE, 0, &addr, &i);
	if (decode) {
		decode(&portslot[0], portslotp->msg, rsize, PRT_RCV);
	}
}

int rt_send_req_rel_port(unsigned long node, int op, unsigned long id, MBX *mbx, int hard)
{
	int i, msgsiz;
	struct portslot_t *portslotp;
	struct req_rel_msg msg, tosend;

	if (!node || (op && (op < MaxStubs || op >= MaxSocks))) {
		return -EINVAL;
	}
	if (!(portslotp = get_portslot())) {
		return -ENODEV;
	}
	portslotp->name = PRTSRVNAME;
        portslot[0].addr.sin_addr.s_addr = node;
	msg.op = op ? ntohs(portslot[op].addr.sin_port) : 0;
	msg.port = 0;
	msg.hard = hard ? MSG_HARD : MSG_SOFT;
	msg.owner = OWNER(this_node[msg.hard], id ? id : (unsigned long)rt_whoami());
	msg.name = id;
	portslotp->msg  = &msg;
	tosend = msg;
	msgsiz = encode ? encode(&portslot[0], &tosend, sizeof(msg), PRT_REQ) : sizeof(msg);
	if (msg.hard) {
		hard_rt_socket_callback(portslotp->socket[1], (void *)hard_net_rcv_rtr, portslot + portslotp->indx);
	} else {
		soft_rt_socket_callback(portslotp->socket[0], (void *)soft_net_rcv_rtr, portslot + portslotp->indx);
	}
	for (i = 0; i < TIMER_FREQ && !msg.port; i++) {
		if (msg.hard) {
			hard_rt_sendto(portslotp->socket[1], &tosend, msgsiz, 0, (struct sockaddr *)&portslot[0].addr, ADRSZ);
		} else {
			soft_rt_sendto(portslotp->socket[0], &tosend, msgsiz, 0, (struct sockaddr *)&portslot[0].addr, ADRSZ);
		}
		rt_sem_wait(&timer_sem);
	}
	if (msg.port > 0) {
		if (op) {
			portslot[op].task = 0;
			gvb_portslot(portslot + op);
			gvb_portslot(portslotp);
			return op;
		} else {
			portslotp->hard = msg.hard;
			portslotp->name = msg.name;
			portslotp->addr = portslot[0].addr;
			portslotp->addr.sin_port = htons(msg.port);
		       	portslotp->addr.sin_addr.s_addr = node;
			portslotp->mbx  = mbx;
			portslotp->task = 1;
			return portslotp->indx;
		}
	}
	gvb_portslot(portslotp);
	return msg.port ? msg.port : -ETIMEDOUT;
}

RT_TASK *rt_find_asgn_stub(unsigned long long owner, int asgn)
{
	int i;
	i = asgn ? hash_find_if_not_ins(owner) : hash_find(owner);
	return i > 0 ? (RT_TASK *)portslot[i].task : 0;
}

int rt_rel_stub(unsigned long long owner)
{
	return hash_rem(owner) > 0 ? 0 : -ESRCH;
}

int rt_waiting_return(unsigned long node, int port)
{
	struct portslot_t *portslotp;
	portslotp = portslot + abs(port);
	return portslotp->task < 0 && !portslotp->sem.count;
}

static inline void mbx_send_if(MBX *mbx, void *sendmsg, int msg_size)
{
#define MOD_SIZE(indx) ((indx) < mbx->size ? (indx) : (indx) - mbx->size)
extern void (*dnepsus_trxl)(void);

	unsigned long flags;
	int tocpy, avbs;
	char *msg;

	if (!mbx) {
		return;
	}
	msg = sendmsg;
	if (msg_size <= mbx->frbs) {
		RT_TASK *task;
		avbs = mbx->avbs;
		while (msg_size > 0 && mbx->frbs) {
			if ((tocpy = mbx->size - mbx->lbyte) > msg_size) {
				tocpy = msg_size;
			}
			if (tocpy > mbx->frbs) {
				tocpy = mbx->frbs;
			}
			memcpy(mbx->bufadr + mbx->lbyte, msg, tocpy);
			flags = rt_spin_lock_irqsave(&mbx->lock);
			mbx->frbs -= tocpy;
			rt_spin_unlock_irqrestore(flags, &mbx->lock);
			avbs += tocpy;
			msg_size -= tocpy;
			*msg += tocpy;
			mbx->lbyte = MOD_SIZE(mbx->lbyte + tocpy);
		}
		mbx->avbs = avbs;
		flags = rt_global_save_flags_and_cli();
		if ((task = mbx->waiting_task)) {
			rt_rem_timed_task(task);
			mbx->waiting_task = (void *)0;
        	        if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(MBXSUSP | DELAYED)) == READY) {
                	        rt_enq_ready_task(task);
				dnepsus_trxl();
	        	}
		}
		rt_global_restore_flags(flags);
	}
}

static unsigned long long rt_net_rpc(unsigned long node, int fun_ext_timed, long long type, void *args, int argsize)
{
	char msg[MAX_MSG_SIZE];
	struct reply_t { int wsize, w2size; unsigned long long retval; char msg[1]; } *reply;
	int rsize, port;
	struct portslot_t *portslotp;

	if ((port = PORT(fun_ext_timed)) > 0) {
		if ((portslotp = portslot + port)->task < 0) {
			int i = 0;
        		struct sockaddr addr;
			rt_sem_wait(&portslotp->sem);
			if ((rsize = portslotp->hard ? hard_rt_recvfrom(portslotp->socket[1], msg, MAX_MSG_SIZE, 0, &addr, &i) : soft_rt_recvfrom(portslotp->socket[0], msg, MAX_MSG_SIZE, 0, &addr, &i))) {
				if (decode) {
					rsize = decode(portslotp, msg, rsize, RPC_RCV);
				}
				mbx_send_if(portslotp->mbx, msg, rsize); 
			}
			portslotp->task = 1;
		}
		portslotp->msg = msg;
	} else {
		if ((portslotp = portslot - port)->task < 0) {
			if (!rt_sem_wait_if(&portslotp->sem)) {
				return 0;
			} else {
				int i = 0;
	        		struct sockaddr addr;
			if ((rsize = portslotp->hard ? hard_rt_recvfrom(portslotp->socket[1], msg, MAX_MSG_SIZE, 0, &addr, &i) : soft_rt_recvfrom(portslotp->socket[0], msg, MAX_MSG_SIZE, 0, &addr, &i))) {
					if (decode) {
						rsize = decode(portslotp, msg, rsize, RPC_RCV);
					}
					mbx_send_if(portslotp->mbx, msg, rsize);
				}
			}
		} else {
			portslotp->task = -1;
		}
	}
	if (FUN(fun_ext_timed) == 0xFF) {
		return 1;
	}
	if (NEED_TO_R(type)) {			
		rsize = USP_RSZ1(type);
		rsize = rsize ? ((int *)args)[rsize - 1] : (USP_RSZ1LL(type) ? sizeof(long long) : sizeof(int));
	} else {
		rsize = 0;
	}
	{
		struct msg_t { int priority, base_priority, argsize, rsize, fun_ext_timed; long long type; int args[1]; } *arg;
		RT_TASK *task;

		arg = (void *)msg;
		arg->priority = (task = rt_whoami())->priority;
		arg->base_priority = task->base_priority;
		arg->argsize = argsize;
		arg->rsize = rsize;
		arg->fun_ext_timed = fun_ext_timed;
		arg->type = type;
		memcpy(arg->args, args, argsize);
		if (rsize > 0) {			
			memcpy((char *)arg->args + argsize, (int *)((int *)args + USP_RBF1(type) - 1)[0], rsize);

		}
		rsize = sizeof(struct msg_t) - sizeof(int) + argsize + rsize;
		if (encode) {
			rsize = encode(portslotp, msg, rsize, RPC_REQ);
		}
		if (portslotp->hard) {
			hard_rt_sendto(portslotp->socket[1], msg, rsize, 0, (struct sockaddr *)&portslotp->addr, ADRSZ);
		} else  {
			soft_rt_sendto(portslotp->socket[0], msg, rsize, 0, (struct sockaddr *)&portslotp->addr, ADRSZ);
		}
	}
	if (port > 0) {
		rt_sem_wait(&portslotp->sem);
		if ((reply = (void *)msg)->wsize) {
			memcpy((char *)(*((int *)args + USP_WBF1(type) - 1)), reply->msg, reply->wsize);
		}
		if (reply->w2size) {
			memcpy((char *)(*((int *)args + USP_WBF2(type) - 1)), reply->msg + reply->wsize, reply->w2size);
		}
		return reply->retval;
	}
	return 0;
}

int rt_get_net_rpc_ret(MBX *mbx, unsigned long long *retval, void *msg1, int *msglen1, void *msg2, int *msglen2, RTIME timeout, int type)
{
	struct { int wsize, w2size; unsigned long long retval; } reply;
	int ret;

	if ((ret = ((int (*)(MBX *, ...))rt_net_rpc_fun[type])(mbx, &reply, sizeof(reply), timeout))) {
		return ret;
	}
	*retval = reply.retval;
	if (reply.wsize) {
		if (*msglen1 > reply.wsize) {
			*msglen1 = reply.wsize;
		}
		rt_mbx_receive(mbx, &msg1, *msglen1);
	} else {
		*msglen1 = 0;
	}
	if (reply.w2size) {
		if (*msglen2 > reply.w2size) {
			*msglen2 = reply.w2size;
		}
		rt_mbx_receive(mbx, &msg2, *msglen2);
	} else {
		*msglen2 = 0;
	}
	return 0;
}

unsigned long ddn2nl(const char *ddn)
{
	int p, n, c;
	union { unsigned long l; char c[4]; } u;

	p = n = 0;
	while ((c = *ddn++)) {
		if (c != '.') {
			n = n*10 + c - '0';
		} else {
			if (n > 0xFF) {
				return 0;
			}
			u.c[p++] = n;
			n = 0;
		}
	}
	u.c[3] = n;

	return u.l;
}

unsigned long rt_set_this_node(const char *ddn, unsigned long node, int hard)
{
	return this_node[hard ? MSG_HARD : MSG_SOFT] = ddn ? ddn2nl(ddn) : node;
}

//void rt_netrpc_
static struct rt_fun_entry rt_usr_net_rpc_fun[] = {
        { 1LL, rt_net_rpc           },
	{ 1LL, rt_send_req_rel_port },
	{ 0LL, ddn2nl               },
	{ 0LL, rt_set_this_node     },
	{ 0LL, rt_find_asgn_stub    },
	{ 0LL, rt_rel_stub          },
	{ 0LL, rt_waiting_return    },
};

static RT_TASK *rt_base_linux_task, *port_server;

static int init_softrtnet(void);
static void cleanup_softrtnet(void);

int init_module(void)
{
	int i;
        RT_TASK *rt_linux_tasks[NR_RT_CPUS];

	if (init_softrtnet()) {
		return 1;
	}
        rt_base_linux_task = rt_get_base_linux_task(rt_linux_tasks);
        if(rt_base_linux_task->task_trap_handler[0]) {
                if(((int (*)(void *, int))rt_base_linux_task->task_trap_handler[0])(rt_usr_net_rpc_fun, NET_RPC_IDX)) {
			printk("LXRT EXTENSION SLOT FOR DSCHED (%d) ALREADY USED\n", NET_RPC_IDX);
                        return -EACCES;
                }
        }
	MaxSocks += MaxStubs;
	if (!(portslot = kmalloc(MaxSocks*sizeof(struct portslot_t), GFP_KERNEL))) {
		printk("KMALLOC FAILED ALLOCATING PORT SLOTS\n");
	}	
	if (!ThisSoftNode) {
		ThisSoftNode = ThisNode;
	}
	if (!ThisHardNode) {
		ThisHardNode = ThisNode;
	}
	this_node[0] = ddn2nl(ThisSoftNode);
	this_node[1] = ddn2nl(ThisHardNode);

	memset(&portslot[0].addr, 0, sizeof(struct sockaddr_in));
	portslot[0].addr.sin_family = AF_INET;
	portslot[0].addr.sin_addr.s_addr = htonl(INADDR_ANY);
	for (i = MaxSocks - 1; i >= 0; i--) {
		portslot[i].p = portslot + i;
		portslot[i].indx = i;
		portslot[0].addr.sin_port = htons(BASEPORT + i);
		portslot[i].addr = portslot[0].addr;
		portslot[i].socket[0] = soft_rt_socket(AF_INET, SOFT_SOCK_TYPE, 0);
		soft_rt_bind(portslot[i].socket[0], (struct sockaddr *)&portslot[i].addr, ADRSZ);
		portslot[i].socket[1] = hard_rt_socket(AF_INET, SOCK_DGRAM, 0);
		hard_rt_bind(portslot[i].socket[1], (struct sockaddr *)&portslot[i].addr, ADRSZ);
		portslot[i].owner = 0;
		rt_typed_sem_init(&portslot[i].sem, 0, BIN_SEM | FIFO_Q);
		portslot[i].task = 0;
	}
	portslotsp = MaxStubs;
	portslot[0].name = PRTSRVNAME;
	portslot[0].owner = OWNER(this_node, (unsigned long)&port_server);
	soft_rt_socket_callback(portslot[0].socket[0], (void *)soft_net_rcv_req_rel_port, portslot);
	hard_rt_socket_callback(portslot[0].socket[1], (void *)hard_net_rcv_req_rel_port, portslot);
	port_server = kmalloc(sizeof(RT_TASK) + 3*sizeof(struct fun_args), GFP_KERNEL);
	soft_kthread_init(port_server, (int)port_server_fun, (int)port_server, RT_LOWEST_PRIORITY);
	rt_task_resume(port_server);
	rt_typed_sem_init(&timer_sem, 0, BIN_SEM | FIFO_Q);
	init_timer(&timer);
	timer.function = timer_fun;
	timer.expires = jiffies + (HZ + TIMER_FREQ/2 - 1)/TIMER_FREQ;
	add_timer(&timer);
	return 0 ;
}

void cleanup_module(void)
{
	int i;

        if(rt_base_linux_task->task_trap_handler[1]) {
                ((int (*)(void *, int))rt_base_linux_task->task_trap_handler[1])(rt_usr_net_rpc_fun, NET_RPC_IDX);
        }
	del_timer(&timer);
	soft_kthread_delete(port_server);
	rt_sem_delete(&timer_sem);
	for (i = 0; i < MaxStubs; i++) {
		if (portslot[i].task) {
			rt_task_delete((RT_TASK *)portslot[i].task);
		}
	}
	for (i = 0; i < MaxSocks; i++) {
		rt_sem_delete(&portslot[i].sem);
		soft_rt_close(portslot[i].socket[0]);
		hard_rt_close(portslot[i].socket[1]);
	}
	cleanup_softrtnet();
	kfree(port_server);
	kfree(portslot);
	return;
}

#define SIZARG sizeof(arg)

static RT_TASK *named_task_init(const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void))
{
	RT_TASK *task;
	unsigned long name;

	if ((task = rt_get_adr(name = nam2num(task_name)))) {
		return task;
	}
        if ((task = rt_malloc(sizeof(RT_TASK))) && !rt_task_init(task, thread, data, stack_size, prio, uses_fpu, signal)) {
		if (rt_register(name, task, IS_TASK, 0)) {
			return task;
		}
		rt_task_delete(task);
	}
	rt_free(task);
	return (RT_TASK *)0;
}

static RT_TASK *named_task_init_cpuid(const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void), unsigned int run_on_cpu)
{
	RT_TASK *task;
	unsigned long name;

	if ((task = rt_get_adr(name = nam2num(task_name)))) {
		return task;
	}
        if ((task = rt_malloc(sizeof(RT_TASK))) && !rt_task_init_cpuid(task, thread, data, stack_size, prio, uses_fpu, signal, run_on_cpu)) {
		if (rt_register(name, task, IS_TASK, 0)) {
			return task;
		}
		rt_task_delete(task);
	}
	rt_free(task);
	return (RT_TASK *)0;
}

static int named_task_delete(RT_TASK *task)
{
	if (!rt_task_delete(task)) {
		rt_free(task);
	}
	return rt_drg_on_adr(task);
}

static SEM *typed_named_sem_init(const char *sem_name, int value, int type)
{
	SEM *sem;
	unsigned long name;

	if ((sem = rt_get_adr(name = nam2num(sem_name)))) {
		return sem;
	}
	if ((sem = rt_malloc(sizeof(SEM)))) {
		rt_typed_sem_init(sem, value, type);
		if (rt_register(name, sem, IS_SEM, 0)) {
			return sem;
		}
		rt_sem_delete(sem);
	}
	rt_free(sem);
	return (SEM *)0;
}

static int named_sem_delete(SEM *sem)
{
	if (!rt_sem_delete(sem)) {
		rt_free(sem);
	}
	return rt_drg_on_adr(sem);
}

static MBX *typed_named_mbx_init(const char *mbx_name, int size, int qtype)
{
	MBX *mbx;
	unsigned long name;

	if ((mbx = rt_get_adr(name = nam2num(mbx_name)))) {
		return mbx;
	}
	if ((mbx = rt_malloc(sizeof(MBX))) && !rt_typed_mbx_init(mbx, size, qtype)) {
		if (rt_register(name, mbx, IS_MBX, 0)) {
			return mbx;
		}
		rt_mbx_delete(mbx);
	}
	rt_free(mbx);
	return (MBX *)0;
}

static int named_mbx_delete(MBX *mbx)
{
	if (!rt_mbx_delete(mbx)) {
		rt_free(mbx);
	}
	return rt_drg_on_adr(mbx);
}

static void *rt_net_rpc_fun[] = {
		rt_get_adr,

		named_task_init,		//   1
		named_task_init_cpuid,
		named_task_delete,
		rt_get_time_ns,
		rt_get_time_ns_cpuid,
		rt_get_cpu_time_ns,
		rt_task_suspend,
		rt_task_resume,
		rt_sleep,
		rt_sleep_until,

		typed_named_sem_init,		//  11
		named_sem_delete,
		rt_sem_signal,
		rt_sem_broadcast,
		rt_sem_wait,
		rt_sem_wait_if,
		rt_sem_wait_until,
		rt_sem_wait_timed,

		rt_send,			//  19
		rt_send_if,
		rt_send_until,
		rt_send_timed,

		rt_rpc,				//  23
		rt_rpc_if,
		rt_rpc_until,
		rt_rpc_timed,

		rt_isrpc,			//  27

		typed_named_mbx_init,		//  28
		named_mbx_delete,
		rt_mbx_send,
		rt_mbx_send_wp,		
		rt_mbx_send_if,
		rt_mbx_send_until,
		rt_mbx_send_timed,
		rt_mbx_receive,
		rt_mbx_receive_wp,
		rt_mbx_receive_if,
		rt_mbx_receive_until,
		rt_mbx_receive_timed,

		rt_sendx,			//  40
		rt_sendx_if,
		rt_sendx_until,
		rt_sendx_timed,

		rt_rpcx,			//  44
		rt_rpcx_if,
		rt_rpcx_until,
		rt_rpcx_timed,

		rt_mbx_evdrp,			//  48
};

void *RT_get_adr(unsigned long node, int port, const char *sname)
{
	if (node) {
		struct { int name; } arg = { nam2num(sname) };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_GET_ADR, 0), 0LL, &arg, SIZARG);
	}
	return rt_get_adr(nam2num(sname));
} 

RT_TASK *RT_named_task_init(unsigned long node, int port, const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void))
{
	if (node) {
		struct { const char *task_name; void (*thread)(int); int data; int stack_size; int prio; int uses_fpu; void(*signal)(void); int namelen; } arg = { task_name, thread, data, stack_size, prio, uses_fpu, signal, strlen(task_name) };
		return (RT_TASK *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_NAMED_TASK_INIT, 0), UR1(1, 8), &arg, SIZARG);
	}
	return named_task_init(task_name, thread, data, stack_size, prio, uses_fpu, signal);
}

RT_TASK *RT_named_task_init_cpuid(unsigned long node, int port, const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void), unsigned int run_on_cpu)
{
	if (node) {
		struct { const char *task_name; void (*thread)(int); int data; int stack_size; int prio; int uses_fpu; void(*signal)(void); unsigned int run_on_cpu; int namelen; } arg = { task_name, thread, data, stack_size, prio, uses_fpu, signal, run_on_cpu, strlen(task_name) };
		return (RT_TASK *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_NAMED_TASK_INIT, 0), UR1(1, 9), &arg, SIZARG);
	}
	return named_task_init_cpuid(task_name, thread, data, stack_size, prio, uses_fpu, signal, run_on_cpu);
}

int RT_named_task_delete(unsigned long node, int port, RT_TASK *task)
{
	if (node) {
		struct { RT_TASK *task; } arg = { task };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_NAMED_TASK_DELETE, 0), 0LL, &arg, SIZARG);
	}
	return named_task_delete(task);
}

RTIME RT_get_time_ns(unsigned long node, int port)
{
	if (node) {
		struct { int dummy; } arg = { 0 };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_GET_TIME_NS, 0), 0LL, &arg, SIZARG);
	}
	return rt_get_time_ns();
}

RTIME RT_get_time_ns_cpuid(unsigned long node, int port, int cpuid)
{
	if (node) {
		struct { int cpuid; } arg = { cpuid };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_GET_TIME_NS_CPUID, 0), 0LL, &arg, SIZARG);
	}
	return rt_get_time_ns_cpuid(cpuid);
}

RTIME RT_get_cpu_time_ns(unsigned long node, int port)
{
	if (node) {
		struct { int dummy; } arg = { 0 };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_GET_CPU_TIME_NS, 0), 0LL, &arg, SIZARG);
	}
	return rt_get_cpu_time_ns();
}

int RT_task_suspend(unsigned long node, int port, RT_TASK *task)
{
	if (node) {
		struct { RT_TASK *task; } arg = { task };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_TASK_SUSPEND, 0), 0LL, &arg, SIZARG);
	}
	return rt_task_suspend(task);
}

int RT_task_resume(unsigned long node, int port, RT_TASK *task)
{
	if (node) {
		struct { RT_TASK *task; } arg = { task };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_TASK_RESUME, 0), 0LL, &arg, SIZARG);
	}
	return rt_task_resume(task);
}

void RT_sleep(unsigned long node, int port, RTIME delay)
{
	if (node) {
		struct { RTIME delay; } arg = { delay };
		rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SLEEP, 1), 0LL, &arg, SIZARG);
		return;
	}
	rt_sleep(nano2count(delay));
} 

void RT_sleep_until(unsigned long node, int port, RTIME time)
{
	if (node) {
		struct { RTIME time; } arg = { time };
		rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SLEEP_UNTIL, 1), 0LL, &arg, SIZARG);
		return;
	}
	rt_sleep_until(nano2count(time));
} 

SEM *RT_typed_named_sem_init(unsigned long node, int port, const char *sem_name, int value, int type)
{
	if (node) {
		struct { const char *sem_name; int value; int type; int namelen; } arg = { sem_name, value, type, strlen(sem_name) };
		return (SEM *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_NAMED_SEM_INIT, 0), UR1(1, 4), &arg, SIZARG);
	}
	return typed_named_sem_init(sem_name, value, type);
}

int RT_named_sem_delete(unsigned long node, int port, SEM *sem)
{
	if (node) {
		struct { SEM *sem; } arg = { sem };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_NAMED_SEM_DELETE, 0), 0LL, &arg, SIZARG);
	}
	return named_sem_delete(sem);
}

int RT_sem_signal(unsigned long node, int port, SEM *sem)
{
	if (node) {
		struct { SEM *sem; } arg = { sem };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEM_SIGNAL, 0), 0LL, &arg, SIZARG);
	}
	return rt_sem_signal(sem);
} 

int RT_sem_broadcast(unsigned long node, int port, SEM *sem)
{
	if (node) {
		struct { SEM *sem; } arg = { sem };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEM_BROADCAST, 0), 0LL, &arg, SIZARG);
	}
	return rt_sem_broadcast(sem);
} 

int RT_sem_wait(unsigned long node, int port, SEM *sem)
{
	if (node) {
		struct { SEM *sem; } arg = { sem };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEM_WAIT, 0), 0LL, &arg, SIZARG);
	}
	return rt_sem_wait(sem);
} 

int RT_sem_wait_if(unsigned long node, int port, SEM *sem)
{
	if (node) {
		struct { SEM *sem; } arg = { sem };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEM_WAIT_IF, 0), 0LL, &arg, SIZARG);
	}
	return rt_sem_wait_if(sem);
} 

int RT_sem_wait_until(unsigned long node, int port, SEM *sem, RTIME time)
{
	if (node) {
		struct { SEM *sem; RTIME time; } arg = { sem, time };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEM_WAIT_UNTIL, 2), 0LL, &arg, SIZARG);
	}
	return rt_sem_wait_until(sem, nano2count(time));
} 

int RT_sem_wait_timed(unsigned long node, int port, SEM *sem, RTIME delay)
{
	if (node) {
		struct { SEM *sem; RTIME delay; } arg = { sem, delay };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEM_WAIT_TIMED, 2), 0LL, &arg, SIZARG);
	}
	return rt_sem_wait_timed(sem, nano2count(delay));
} 

RT_TASK *RT_send(unsigned long node, int port, RT_TASK *task, unsigned int msg)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; } arg = { task, msg };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEND, 0), 0LL, &arg, SIZARG);
	}
	return rt_send(task, msg);
}

RT_TASK *RT_send_if(unsigned long node, int port, RT_TASK *task, unsigned int msg)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; } arg = { task, msg };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEND_IF, 0), 0LL, &arg, SIZARG);
	}
	return rt_send_if(task, msg);
}

RT_TASK *RT_send_until(unsigned long node, int port, RT_TASK *task, unsigned int msg, RTIME time)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; RTIME time; } arg = { task, msg, time };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEND_UNTIL, 3), 0LL, &arg, SIZARG);
	}
	return rt_send_until(task, msg, nano2count(time));
}

RT_TASK *RT_send_timed(unsigned long node, int port, RT_TASK *task, unsigned int msg, RTIME delay)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; RTIME delay; } arg = { task, msg, delay };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SEND_TIMED, 3), 0LL, &arg, SIZARG);
	}
	return rt_send_timed(task, msg, nano2count(delay));
}

RT_TASK *RT_receive(unsigned long node, int port, RT_TASK *task, unsigned int *msg)
{
	if (!task || !node) {
		return rt_receive(task, msg);
	}
	return rt_receive(rt_find_asgn_stub(OWNER(node, task), 1), msg) ? task : 0;
}

RT_TASK *RT_receive_if(unsigned long node, int port, RT_TASK *task, unsigned int *msg)
{
	if (!task || !node) {
		return rt_receive_if(task, msg);
	}
	return rt_receive_if(rt_find_asgn_stub(OWNER(node, task), 1), msg) ? task : 0;
}

RT_TASK *RT_receive_until(unsigned long node, int port, RT_TASK *task, unsigned int *msg, RTIME time)
{
	if (!task || !node) {
		return rt_receive_until(task, msg, nano2count(time));
	}
	return rt_receive_until(rt_find_asgn_stub(OWNER(node, task), 1), msg, nano2count(time)) ? task : 0;
}

RT_TASK *RT_receive_timed(unsigned long node, int port, RT_TASK *task, unsigned int *msg, RTIME delay)
{
	if (!task || !node) {
		return rt_receive_timed(task, msg, nano2count(delay));
	}
	return rt_receive_timed(rt_find_asgn_stub(OWNER(node, task), 1), msg, nano2count(delay)) ? task : 0;
}

RT_TASK *RT_rpc(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; unsigned int *ret; } arg = { task, msg, ret };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_RPC, 0), UW1(3, 0), &arg, SIZARG);
	}
	return rt_rpc(task, msg, ret);
}

RT_TASK *RT_rpc_if(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; unsigned int *ret; } arg = { task, msg };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_RPC_IF, 0), UW1(3, 0), &arg, SIZARG);
	}
	return rt_rpc_if(task, msg, ret);
}

RT_TASK *RT_rpc_until(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret, RTIME time)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; unsigned int *ret; RTIME time; } arg = { task, msg, ret, time };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_RPC_UNTIL, 4), UW1(3, 0), &arg, SIZARG);
	}
	return rt_rpc_until(task, msg, ret, nano2count(time));
}

RT_TASK *RT_rpc_timed(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret, RTIME delay)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; unsigned int *ret; RTIME delay; } arg = { task, msg, ret, delay };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_RPC_TIMED, 4), UW1(3, 0), &arg, SIZARG);
	}
	return rt_rpc_timed(task, msg, ret, nano2count(delay));
}

int RT_isrpc(unsigned long node, int port, RT_TASK *task)
{
        if (node) {
                struct { RT_TASK *task; } arg = { task };
                return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_ISRPC, 0), 0LL, &arg, SIZARG);
        }
        return rt_isrpc(task);
}

RT_TASK *RT_return(unsigned long node, int port, RT_TASK *task, unsigned int result)
{
	if (!task || !node) {
		return rt_return(task, result);
        }
	return rt_return(rt_find_asgn_stub(OWNER(node, task), 1), result) ? task : 0;
}

RT_TASK *RT_evdrp(unsigned long node, int port, RT_TASK *task, unsigned int *msg)
{
	if (!task || !node) {
		return rt_evdrp(task, msg);
	}
	return rt_evdrp(rt_find_asgn_stub(OWNER(node, task), 1), msg) ? task : 0;
}

RT_TASK *RT_rpcx(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	if (node) {
		struct { RT_TASK *task; void *smsg; void *rmsg; int ssize; int rsize; } arg = { task, smsg, rmsg, ssize, rsize };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_RPCX, 0), UR1(2, 4) | UW1(3, 5), &arg, SIZARG);
	}
	return rt_rpcx(task, smsg, rmsg, ssize, rsize);
}

RT_TASK *RT_rpcx_if(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	if (node) {
		struct { RT_TASK *task; void *smsg; void *rmsg; int ssize; int rsize; } arg = { task, smsg, rmsg, ssize, rsize };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_RPCX_IF, 0), UR1(2, 4) | UW1(3, 5), &arg, SIZARG);
	}
	return rt_rpcx_if(task, smsg, rmsg, ssize, rsize);
}

RT_TASK *RT_rpcx_until(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME time)
{
	if (node) {
		struct { RT_TASK *task; void *smsg; void *rmsg; int ssize; int rsize; RTIME time; } arg = { task, smsg, rmsg, ssize, rsize, time };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_RPCX_UNTIL, 6), UR1(2, 4) | UW1(3, 5), &arg, SIZARG);
	}
	return rt_rpcx_until(task, smsg, rmsg, ssize, rsize, nano2count(time));
}

RT_TASK *RT_rpcx_timed(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME delay)
{
	if (node) {
		struct { RT_TASK *task; void *smsg; void *rmsg; int ssize; int rsize; RTIME delay; } arg = { task, smsg, rmsg, ssize, rsize, delay };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_RPCX_TIMED, 6), UR1(2, 4) | UW1(3, 5), &arg, SIZARG);
	}
	return rt_rpcx_timed(task, smsg, rmsg, ssize, rsize, nano2count(delay));
}

RT_TASK *RT_sendx(unsigned long node, int port, RT_TASK *task, void *msg, int size)
{
	if (node) {
		struct { RT_TASK *task; void *msg; int size; } arg = { task, msg, size };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SENDX, 0), UR1(2, 3), &arg, SIZARG);
	}
	return rt_sendx(task, msg, size);
}

RT_TASK *RT_sendx_if(unsigned long node, int port, RT_TASK *task, void *msg, int size)
{
	if (node) {
		struct { RT_TASK *task; void *msg; int size; } arg = { task, msg, size };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SENDX_IF, 0), UR1(2, 3), &arg, SIZARG);
	}
	return rt_sendx_if(task, msg, size);
}

RT_TASK *RT_sendx_until(unsigned long node, int port, RT_TASK *task, void *msg, int size, RTIME time)
{
	if (node) {
		struct { RT_TASK *task; void *msg; int size; RTIME time; } arg = { task, msg, size, time };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SENDX_UNTIL, 4), UR1(2, 3), &arg, SIZARG);
	}
	return rt_sendx_until(task, msg, size, nano2count(time));
}

RT_TASK *RT_sendx_timed(unsigned long node, int port, RT_TASK *task, void *msg, int size, RTIME delay)
{
	if (node) {
		struct { RT_TASK *task; void *msg; int size; RTIME delay; } arg = { task, msg, size, delay };
		return (void *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_SENDX_TIMED, 4), UR1(2, 3), &arg, SIZARG);
	}
	return rt_sendx_timed(task, msg, size, nano2count(delay));
}

RT_TASK *RT_returnx(unsigned long node, int port, RT_TASK *task, void *msg, int size)
{
	if (!task || !node) {
		return rt_returnx(task, msg, size);
	}
	return rt_returnx(rt_find_asgn_stub(OWNER(node, task), 1), msg, size) ? task : 0;
}

RT_TASK *RT_evdrpx(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len)
{
	if (!task || !node) {
		return rt_evdrpx(task, msg, size, len);
	}
	return rt_evdrpx(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len) ? task : 0;
}

RT_TASK *RT_receivex(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len)
{
	if (!task || !node) {
		return rt_receivex(task, msg, size, len);
	}
	return rt_receivex(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len) ? task : 0;
}

RT_TASK *RT_receivex_if(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len)
{
	if (!task || !node) {
		return rt_receivex_if(task, msg, size, len);
	}
	return rt_receivex_if(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len) ? task : 0;
}

RT_TASK *RT_receivex_until(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len, RTIME time)
{
	if (!task || !node) {
		return rt_receivex_until(task, msg, size, len, nano2count(time));
	}
	return rt_receivex_until(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len, nano2count(time)) ? task : 0;
}

RT_TASK *RT_receivex_timed(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len, RTIME delay)
{
	if (!task || !node) {
		return rt_receivex_timed(task, msg, size, len, nano2count(delay));
	}
	return rt_receivex_timed(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len, nano2count(delay)) ? task : 0;
}

MBX *RT_typed_named_mbx_init(unsigned long node, int port, const char *mbx_name, int size, int qtype)
{
	if (node) {
		struct { const char *mbx_name; int size; int qype; int namelen; } arg = { mbx_name, size, qtype, strlen(mbx_name) };
		return (MBX *)(unsigned long)rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_NAMED_MBX_INIT, 0), UR1(1, 4), &arg, SIZARG);
	}
	return typed_named_mbx_init(mbx_name, size, qtype);
}

int RT_named_mbx_delete(unsigned long node, int port, MBX *mbx)
{
	if (node) {
		struct { MBX *mbx; } arg = { mbx };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_NAMED_MBX_DELETE, 0), 0LL, &arg, SIZARG);
	}
	return named_mbx_delete(mbx);
}

int RT_mbx_send(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND, 0), UR1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_send(mbx, msg, msg_size);
} 

int RT_mbx_send_wp(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND_WP, 0), UR1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_send_wp(mbx, msg, msg_size);
} 

int RT_mbx_send_if(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND_IF, 0), UR1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_send_if(mbx, msg, msg_size);
} 

int RT_mbx_send_until(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME time)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; RTIME time; } arg = { mbx, msg, msg_size, time };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND_UNTIL, 4), UR1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_send_until(mbx, msg, msg_size, nano2count(time));
} 

int RT_mbx_send_timed(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME delay)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; RTIME delay; } arg = { mbx, msg, msg_size, delay };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND_TIMED, 4), UR1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_send_timed(mbx, msg, msg_size, nano2count(delay));
} 

int RT_mbx_evdrp(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_EVDRP, 0), UW1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_evdrp(mbx, msg, msg_size);
} 

int RT_mbx_receive(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE, 0), UW1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_receive(mbx, msg, msg_size);
} 

int RT_mbx_receive_wp(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE_WP, 0), UW1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_receive_wp(mbx, msg, msg_size);
} 

int RT_mbx_receive_if(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE_IF, 0), UW1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_receive_if(mbx, msg, msg_size);
} 

int RT_mbx_receive_until(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME time)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; RTIME time; } arg = { mbx, msg, msg_size, time };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE_UNTIL, 4), UW1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_receive_until(mbx, msg, msg_size, nano2count(time));
} 

int RT_mbx_receive_timed(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME delay)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; RTIME delay; } arg = { mbx, msg, msg_size, delay };
		return rt_net_rpc(node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE_TIMED, 4), UW1(2, 3), &arg, SIZARG);
	}
	return rt_mbx_receive_timed(mbx, msg, msg_size, nano2count(delay));
} 

int rt_sync_net_rpc(unsigned long node, int port)
{
	if (node) {
		struct { int dummy; } arg = { 0 };
		return rt_net_rpc(abs(node), PACKPORT(port, NET_RPC_EXT, 0xFF, 0), 0LL, &arg, SIZARG);
	}
	return 1;
} 

#include <linux/kmod.h>

#if SOFT_RTNET

#include <asm/uaccess.h>

#include <rtai_shm.h>

static DECLARE_MUTEX_LOCKED(mtx);
static spinlock_t sysrq_lock = SPIN_LOCK_UNLOCKED;

static struct sock_t *socks;
static int *runsock;

int soft_rt_socket(int domain, int type, int protocol)
{
	int i;
	for (i = 0; i < (MAX_SOCKS + MAX_STUBS); i++) {
		if (!cmpxchg(&socks[i].opnd, 0, 1)) {
			return i;
		}
	}
	return -1;
}

int soft_rt_close(int sock)
{
	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS)) {
		return socks[sock].opnd = 0;
	}
	return -1;
}

int soft_rt_bind(int sock, struct sockaddr *addr, int addrlen)
{
	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS)) {
		memcpy(&socks[sock].bindaddr, addr, addrlen);
		return 0;
	}
	return -1;
}

int soft_rt_socket_callback(int sock, int (*func)(int sock, void *arg), void *arg)
{
	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS) && func > 0) {
		socks[sock].callback = func;
		socks[sock].arg      = arg;
		return 0;
	}
	return -1;
}

#define MAX_SOCK_SRQ 128
static struct { int srq, in, out, sockindx[MAX_SOCK_SRQ]; } sysrq;

int soft_rt_sendto(int sock, const void *msg, int len, unsigned int sflags, struct sockaddr *to, int tolen)
{
	unsigned long flags;

	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS)) {
		if (len > MAX_MSG_SIZE) {
			len = MAX_MSG_SIZE;
		}
		memcpy(socks[sock].msg, msg, socks[sock].len = len);
		memcpy(&socks[sock].addr, to, tolen);
		flags = rt_spin_lock_irqsave(&sysrq_lock);
		sysrq.sockindx[sysrq.in] = sock;
	        sysrq.in = (sysrq.in + 1) & (MAX_SOCK_SRQ - 1);
		rt_spin_unlock_irqrestore(flags, &sysrq_lock);
		rt_pend_linux_srq(sysrq.srq);
		return len;
	}
	return -1;
}

int soft_rt_recvfrom(int sock, void *msg, int len, unsigned int flags, struct sockaddr *from, int *fromlen)
{
	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS)) {
		if (len > MAX_MSG_SIZE) {
			len = MAX_MSG_SIZE;
		}
		if (len > socks[sock].recv) {
			len = socks[sock].recv;
		}
		memcpy(msg, socks[sock].msg, len);
		if (from && fromlen) { 
			memcpy(from, &socks[sock].addr, socks[sock].addrlen);
			*fromlen = socks[sock].addrlen;
		}
		return len;
	}
	return -1;
}

long long user_rtnet(unsigned int whatever)
{
	int i, arg[2];
        sigset_t signal, blocked;

        copy_from_user(arg, (unsigned int *)whatever, 2*sizeof(unsigned int));
	if (arg[0]) {
		if (sysrq.out == sysrq.in) {
			down_interruptible(&mtx);
        		signal = current->pending.signal;
        		blocked = current->blocked;
		        for (i = 0; i < _NSIG_WORDS; i++) {
		                if (signal.sig[i] & ~blocked.sig[i]) {
                			return -ERESTARTSYS;
		                }
			}		
		}
		if (sysrq.out != sysrq.in) {
			i = sysrq.sockindx[sysrq.out];
               		sysrq.out = (sysrq.out + 1) & (MAX_SOCK_SRQ - 1);
			return i;
		}
		return -1;
	} 
	if (socks[arg[1]].callback) {
		socks[arg[1]].callback(arg[1], socks[arg[1]].arg);
	}
	return 0;
}

void rtai_rtnet(void)
{
	up(&mtx);
}

int init_softrtnet(void)
{
	if ((sysrq.srq = rt_request_srq(0xcacca1, rtai_rtnet, user_rtnet)) < 0) {
                printk("KRTNET: no sysrq available.\n");
                return sysrq.srq;
	}
        socks = (struct sock_t *)rtai_kmalloc(0xcacca0, (MAX_SOCKS + MAX_STUBS)*sizeof(struct sock_t) + 2*sizeof(int));
        runsock = (int *)(socks + MAX_SOCKS + MAX_STUBS);
#if USE_KMOD
	{
		#define XSTR(x)    #x
		#define MYSTR(x)   XSTR(x)
		static char *urtnet = MYSTR(URTNET);
		if (call_usermodehelper(urtnet, NULL, NULL)) {
			rt_free_srq(sysrq.srq);
			rtai_kfree(0xcacca0);
			printk("UNABLE TO EXECUTE LINUX SOCKET SUPPORT\n");
			return 1;
		}
	}
#endif
	return 0;
}

void cleanup_softrtnet(void)
{
	if (runsock[0]) {
		runsock[0] = 0;
		up(&mtx);
		while (!runsock[0]) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(HZ/10);
		}
	}
	rt_free_srq(sysrq.srq);
	rtai_kfree(0xcacca0);
	return;
}

#else

int init_softrtnet(void)
{
#if USE_KMOD
	{
		#define XSTR(x)    #x
		#define MYSTR(x)   XSTR(x)
		static char *urtnet = MYSTR(URTNET);
		if (call_usermodehelper(urtnet, NULL, NULL)) {
			printk("UNABLE TO EXECUTE LINUX SOCKET SUPPORT\n");
			return 1;
		}
	}
#endif
	return 0;
}

void cleanup_softrtnet(void) { }

#endif

void *rt_net_rpc_fun_hook = rt_net_rpc_fun;
