/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it),
extensions for user space modules are jointly copyrighted (2000) with:
			Pierre Cloutier (pcloutier@poseidoncontrols.com),
			Steve Papacharalambous (stevep@zentropix.com).

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


/*
Nov. 2001, Jan Kiszka (Jan.Kiszka@web.de) fix a tiny bug in __task_init.
*/

#ifndef LXRT_MODULE
#define LXRT_MODULE
#endif
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>

#include <asm/uaccess.h>

#define INTERFACE_TO_LINUX
#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>
#include <rtai_trace.h>
#include "rtai_lxk.h"

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#include <rtai_nam2num.h>

extern struct proc_dir_entry *rtai_proc_root;
static int rtai_proc_lxrt_register(void);
static void rtai_proc_lxrt_unregister(void);
#endif

#include "names.h"
#include "proxies.h"
#include "msg.h"
#include "qblk.h"
#include "registry.h"
#include "traps.h"
#include "rtai_signal.h"

/*
 * This is to make modutils happy. As GPL is a subset of LGPL, it is 
 * correct to tell modutils that this module complies with the GPL. 
 */  
MODULE_LICENSE("GPL");

/*
 * SINGLE_CPU. If you want all hard real time processes to go onto a single 
 * CPU set the default here. A negative value means: put them on the one with 
 * the least number of hard real time processes, unless there is a single 
 * allowed CPU already assigned by the user. The default setting below can be 
 * changed, without recompiling, by using the following install command:
 * insmod ./lxrt "SingleCpu=<cpu number>".
 */

#define SINGLE_CPU  -1

#define ALLOW_SYSW
#define ECHO_SYSW
#ifdef ECHO_SYSW
#define SYSW_DIAG_MSG(x) x
#else
#define SYSW_DIAG_MSG(x)
#endif

#define IN_TRAP -1

extern int rt_signal_linux_task(struct task_struct *lxt, int sig, RT_TASK *rtt);

#define MAX_FUN_EXT  16
static struct rt_fun_entry *rt_fun_ext[MAX_FUN_EXT];

/*
 * BE CAREFULL TO KEEP THE 2 MACROS BELOW UP TO DATE WITH THE SCHEDULERS!
 * WATCH OUT for: 1) Largest stack size required by any rt_scheduled function;
 *                2) Max expected size of messages;
 * Note that the first two are just defaults. You are in full control of the 
 * stack size at task init and the size of any message buffer is automatically 
 * updated when needed, with an rt_malloc. 
 */

#define STACK_SIZE  4096 // Default stack size. qBlk's examples need > 2048.
                         // Running out of stack is a nigthmare to debug!
#define MSG_SIZE    256  // Default max message size.
#define MAX_ARGS    10   // Max number of 4 bytes args in rtai functions.

/*
 *    !!! ATTENTION: KEEP THE MACRO MAX_SRQ BELOW A POWER OF 2, ALWAYS !!! 
 *  Must be >= the max number of expected soft-hard real time LINUX processes.
 */
extern struct t_sigsysrq sigsysrq;

#define TASK_LXRT_OWNED  TASK_UNINTERRUPTIBLE

extern volatile unsigned long lxrt_hrt_flags; 

#ifdef CONFIG_RTAI_TRACE
/****************************************************************************/
/* Trace functions. These functions have to be used rather than insert
the macros as-is. Otherwise the system crashes ... You've been warned. K.Y. */
void trace_true_lxrt_rtai_syscall_entry(void)
{
	TRACE_RTAI_LXRT(TRACE_RTAI_EV_LXRT_RTAI_SYSCALL_ENTRY, 0, 0, 0);
}
void trace_true_lxrt_rtai_syscall_exit(void)
{
	TRACE_RTAI_LXRT(TRACE_RTAI_EV_LXRT_RTAI_SYSCALL_EXIT, 0, 0, 0);
}
/****************************************************************************/
#endif

DEFINE_LXRT_HANDLER

#ifdef HARD_LXRT

static RT_TRAP_HANDLER sched_trap_handler;

#define LINUX_SYS_VECTOR             0x80

static RT_TASK *rt_linux_tasks[NR_RT_CPUS], *lxrt_current[NR_RT_CPUS];
static volatile unsigned long linux_flags[NR_RT_CPUS];

/*
 * Should the preemptive fifo scheduler for hard real time LINUX processes be
 * SMP or MUP?
 * We tried an SMP based scheduler first, the MUP scheme should have made things
 * simpler. Succeeding this way meant we were in full control of what
 * we were doing and there are now no problem going MUP. Note that the SMP-MUP
 * choice here is completly independent from the RTAI MP scheduler in use.
 * This choice does not affect UP operation either. On the other hand using a 
 * MUP scheduler for hard real time in user space should be better in relation 
 * to MMU and cache usage.
 */

#ifdef CONFIG_SMP
#define SWITCH_MEM  0x80000000
#else
#define SWITCH_MEM  0x80000000
#endif

/*
 * This is to check if current is current also for hard real time processes,
 * switched on our own. It should be so for sure but now and then we can need i
 * it for debugging porpuses.
 */

#define CURRENT lxrt_current_process()
static inline struct task_struct *lxrt_current_process(void) {
	return test_bit(hard_cpu_id(), &lxrt_hrt_flags) ? lxrt_current[hard_cpu_id()]->lnxtsk : current;
}

RT_TASK *rt_lxrt_whoami(void)
{
	if (!rt_is_linux()) {
		return rt_whoami();
	}
#ifdef CONFIG_RTAI_ADEOS
	return (RT_TASK *)(rt_is_lxrt() ? lxrt_current[hard_cpu_id()] : arti_get_current(adeos_processor_id())->this_rt_task[0]);
#else /* !CONFIG_RTAI_ADEOS */
	return (RT_TASK *)(rt_is_lxrt() ? lxrt_current[hard_cpu_id()] : current->this_rt_task[0]);
#endif /* CONFIG_RTAI_ADEOS */
}

#ifdef CONFIG_RTAI_FPU_SUPPORT
static void init_fpu(struct task_struct *tsk)
{
	init_xfpu();
	tsk->used_math = 1;
	tsk->flags |= PF_USEDFPU;
}

static inline void restore_fpu(struct task_struct *tsk)
{
	restore_fpenv_lxrt(tsk);
	tsk->flags |= PF_USEDFPU;
}
#else
static void init_fpu(struct task_struct *tsk){}
static inline void restore_fpu(struct task_struct *tsk){}
#endif

/*
 * We are now using LXRT with applications that can involve a large number of
 * hard real time processes. So it's time to avoid running through a long 
 * list, often having just a few ready processes, to find the one to be run.
 * Note that there is no scheduling policy involved here, process buddies have
 * already done it all for us, within the RTAI schedulers. We must just grab 
 * the highest priority process among the ready ones, as quickly as possible.
 */

extern unsigned long linux_save_flags_and_cli_cpuid(int);
extern void rtai_just_copy_back(unsigned long, int);
extern void **rtai_env_linux_sigfun[NR_RT_CPUS];

#define SET_SIG_FUN(task, cpuid, fun)  do { (task)->signal = fun; } while (0)

#define EXECTIME
#ifdef EXECTIME
static RTIME switch_time[NR_RT_CPUS];
#define UEXECTIME() \
do { \
	RTIME now; \
	now = rdtsc(); \
	if (lxrt_current[cpuid]->is_hard) { \
		lxrt_current[cpuid]->exectime[0] += (now - switch_time[cpuid]); \
	} \
	switch_time[cpuid] = now; \
} while (0)
#else
#define UEXECTIME()
#endif

static inline void __lxrt_schedule(int cpuid)
{
	RT_TASK *new_task;

//	if (current != CURRENT) {
//		rt_printk("(SCHED) CURRENT (%d) != current (%d).\n", current->pid, CURRENT->pid);
//	}
	new_task = rt_linux_tasks[cpuid]->nextp;
	if (new_task != lxrt_current[cpuid]) {
		struct task_struct *prev, *next;
		if (!test_and_set_bit(cpuid, &lxrt_hrt_flags)) {
			linux_flags[cpuid] = linux_save_flags_and_cli_cpuid(cpuid);
#ifdef CONFIG_RTAI_ADEOS
			rt_linux_tasks[cpuid]->lnxtsk = prev = arti_get_current(cpuid);
#else /* !CONFIG_RTAI_ADEOS */
			rt_linux_tasks[cpuid]->lnxtsk = prev = current;
#endif /* CONFIG_RTAI_ADEOS */
		} else {
			prev = lxrt_current[cpuid]->lnxtsk;
		}
		UEXECTIME();
		lxrt_current[cpuid] = new_task;
		next = new_task->lnxtsk;
	        TRACE_RTAI_LXRT(TRACE_RTAI_EV_LXRT_SCHED_CHANGE,
				prev->pid, next->pid, prev->state);
		rthal.switch_mem(prev, next, cpuid | SWITCH_MEM);
		my_switch_to(prev, next, prev);
		new_task = lxrt_current[cpuid];
		if ((prev = new_task->lnxtsk) && prev->used_math) {
			restore_fpu(prev);
		}
		if (!new_task->is_hard) {
	            	clear_bit(cpuid, &lxrt_hrt_flags);
			SET_SIG_FUN(new_task, cpuid, 0);
			rtai_just_copy_back(linux_flags[cpuid], cpuid);
//			hard_sti();
		}
	}
	if (lxrt_current[cpuid]->usp_signal) {
//		hard_sti();
		lxrt_current[cpuid]->usp_signal();
	}
}

static void lxrt_sigfun(void)
{
	__lxrt_schedule(hard_cpu_id());
	hard_cli();  // We MUST return with interrupts hard disabled. 
}

static void lxrt_schedule(int cpuid)
{
	__lxrt_schedule(cpuid);
}

static void linux_lxrt_global_cli(void)
{
	hard_cli();
}

static inline void enq_ready_task(RT_TASK *ready_task, int cpuid)
{
	RT_TASK *ltask, *task;
	task = (ltask = rt_linux_tasks[cpuid])->nextp;
	while (ready_task->priority >= task->priority) {
		task = task->nextp;
	}
	task->prevp = (ready_task->prevp = task->prevp)->nextp = ready_task;
	ready_task->nextp = task;
	SET_SIG_FUN(ltask, cpuid, lxrt_sigfun);
}

static inline void rem_ready_task(RT_TASK *task)
{
	if (task->is_hard) {
		(task->prevp)->nextp = task->nextp;
		(task->nextp)->prevp = task->prevp;
	}
}

struct klist_t { volatile int srq, in, out; void *task[MAX_SRQ]; };
static struct klist_t sysrq;
static struct klist_t klistb[NR_RT_CPUS], *klistbp[NR_RT_CPUS];
static struct task_struct *kthreadb[NR_RT_CPUS];
static struct klist_t kliste[NR_RT_CPUS], *klistep[NR_RT_CPUS];
static struct task_struct *kthreade[NR_RT_CPUS];
static int endthread;

static int kthread_b(int *arg)
{
	int cpuid;
	RT_TASK *rt_task;
	struct klist_t *klistp;
	cpuid = *arg;
	sprintf(current->comm, "RTAI_KTHRD_B:%d", cpuid);
	current->cpus_allowed = 1 << cpuid;
	while (cpuid != hard_cpu_id()) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(1);
	}
	kthreadb[cpuid] = current;
	klistbp[cpuid] = klistp = klistb + cpuid;
	sigfillset(&current->blocked);
	while (!endthread) {
		current->state = TASK_UNINTERRUPTIBLE;
		schedule();
		while (klistp->out != klistp->in) {
			rt_task = klistp->task[klistp->out];
			rt_set_runnable_on_cpuid(rt_task, cpuid);
			hard_cli();
			enq_ready_task(rt_task, cpuid);
			lxrt_schedule(cpuid);
			hard_sti();
			klistp->out = (klistp->out + 1) & (MAX_SRQ - 1);
		}
	}
	kthreadb[cpuid] = 0;
	return 0;
}

static int kthread_e(int *arg)
{
	int cpuid;
	struct task_struct *lnxtsk;
	struct klist_t *klistp;
	cpuid = *arg;
	sprintf(current->comm, "RTAI_KTHRD_E:%d", cpuid);
	current->cpus_allowed = 1 << cpuid;
	while (cpuid != hard_cpu_id()) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(1);
	}
	kthreade[cpuid] = current;
	klistep[cpuid] = klistp = kliste + cpuid;
	current->nice = 19;
	sigfillset(&current->blocked);
	while (!endthread) {
		current->state = TASK_UNINTERRUPTIBLE;
		schedule();
		while (klistp->out != klistp->in) {
			lnxtsk = klistp->task[klistp->out];
			klistp->out = (klistp->out + 1) & (MAX_SRQ - 1);
			wake_up_process(lnxtsk);
			schedule();
		}
	}
	kthreade[cpuid] = 0;
	return 0;
}

static void steal_from_linux(RT_TASK *rt_task)
{
	int cpuid;
	struct klist_t *klistp;
	TRACE_RTAI_LXRT(TRACE_RTAI_EV_LXRT_STEAL_TASK,
			(rt_task->lnxtsk)->pid, 0, 0);
       	rthal.lxrt_global_cli = linux_lxrt_global_cli;
	cpuid = ffnz((rt_task->lnxtsk)->cpus_allowed);
	klistp = klistbp[cpuid];
	hard_cli();
	klistp->task[klistp->in] = rt_task;
	klistp->in = (klistp->in + 1) & (MAX_SRQ - 1);
	hard_sti();
	current->state = TASK_LXRT_OWNED;
	wake_up_process(kthreadb[cpuid]);
	schedule();
	rt_task->is_hard = 1;
	rt_task->exectime[1] = rdtsc();
	hard_sti();
	if (current->used_math) {
		restore_fpu(current);
	}
}

static void give_back_to_linux(RT_TASK *rt_task, int in_trap)
{
	int cpuid;
	struct klist_t *klistp;
	TRACE_RTAI_LXRT(TRACE_RTAI_EV_LXRT_GIVE_BACK_TASK,
			(rt_task->lnxtsk)->pid, 0, 0);
	cpuid = ffnz((rt_task->lnxtsk)->cpus_allowed);
	hard_cli();
	if (in_trap) {
		rt_signal_linux_task((void *)0, 0, rt_task); 
	} else {
		klistp = klistep[cpuid];
		klistp->task[klistp->in] = rt_task->lnxtsk;
		klistp->in = (klistp->in + 1) & (MAX_SRQ - 1);
                rt_pend_linux_srq(sysrq.srq);
	}
	rem_ready_task(rt_task);
	lxrt_schedule(cpuid);
	rt_task->is_hard = 0;
	hard_sti();
}

//#define HINT_DIAG_LSTI
//#define HINT_DIAG_LRSTF
//#define HINT_DIAG_ECHO

#ifdef HINT_DIAG_ECHO
#define HINT_DIAG_MSG(x) x
#else
#define HINT_DIAG_MSG(x)
#endif

static void (*rthal_enint)(void) = 0;
static void (*rthal_setflags)(unsigned long flags) = 0;

static void lxrt_enint(unsigned long noarg, ...)
{
	if (!test_bit(hard_cpu_id(), &lxrt_hrt_flags)) {

#ifdef HINT_DIAG_LSTI
        do {
                unsigned long hflags, cpuid;
                hard_save_flags(hflags);
                if (!test_bit(IFLAG, &hflags)) {
                        cpuid = hard_cpu_id();
			rtai_just_copy_back((1 << IFLAG) | (1 << cpuid), cpuid);
                        HINT_DIAG_MSG(rt_printk("LXRT STI HAS INTERRUPT DISABLED, PID = %d, EIP = %p, ID = .\n", current->pid, (&noarg)[-1], current->comm););
                        return;
                }
        } while (0);
#endif

		rthal_enint();
		return;
	}
	rt_printk("STIing IN HRTP, PID = %d, EIP = %p, BAD!\n", current->pid, (void *)(&noarg)[-1]);
	return;
}

static void lxrt_setflags(unsigned long flags, ...)
{
	if (!test_bit(hard_cpu_id(), &lxrt_hrt_flags)) {

#ifdef HINT_DIAG_LRSTF
        do {
                unsigned long hflags;
                hard_save_flags(hflags);
                if (flags && !test_bit(IFLAG, &hflags)) {
			rtai_just_copy_back(flags, hard_cpu_id());
                        HINT_DIAG_MSG(rt_printk("LXRT RESTORE FLAGS HAS INTERRUPT DISABLED, FLAGS = %lx, PID = %d, EIP = %p, ID = %s.\n", flags, current->pid, (&flags)[-1], current->comm););
                        return;
                }
        } while (0);
#endif

		rthal_setflags(flags);
		return;
	}
	if (flags) {
		rtai_just_copy_back(flags, hard_cpu_id());
		rt_printk("RSTFing IN HRTP, PID = %d, EIP = %p, BAD!\n", current->pid, (void *)(&flags)[-1]);
		return;
	}
}

#endif

static inline void lxrt_suspend(RT_TASK *rt_task)
{
	TRACE_RTAI_LXRT(TRACE_RTAI_EV_LXRT_SUSPEND, 
			(rt_task->lnxtsk)->pid, 0, 0);
	rt_global_cli();
	if (rt_task->is_hard) {
		enq_ready_task(rt_task, hard_cpu_id());
	} else if ((rt_task->lnxtsk)->state == TASK_INTERRUPTIBLE) {
		sysrq.task[sysrq.in] = rt_task->lnxtsk;
		sysrq.in = (sysrq.in + 1) & (MAX_SRQ - 1);
		rt_pend_linux_srq(sysrq.srq);
	}
	(rt_task->rprev)->rnext = rt_task->rnext;
	(rt_task->rnext)->rprev = rt_task->rprev;
	rt_task->state = 0;
	LXRT_SUSPEND(rt_task);
	rt_global_sti();
}

struct fun_args { int a0; int a1; int a2; int a3; int a4; int a5; int a6; int a7; int a8; int a9; long long (*fun)(int, ...); };

static void buddy_fun(RT_TASK *mytask) 
{
	struct fun_args *arg = (void *)mytask->fun_args;
	while (1) {
		mytask->retval = arg->fun(arg->a0, arg->a1, arg->a2, arg->a3, arg->a4, arg->a5, arg->a6, arg->a7, arg->a8, arg->a9);
		lxrt_suspend(mytask);
	}
}

static inline long long __lxrt_resume(void *fun, int narg, int *arg, unsigned long long type, RT_TASK *rt_task, int net_rpc)
{
	int wsize, w2size;
	int *wmsg_adr, *w2msg_adr;

	((struct fun_args *)rt_task->fun_args)->fun = fun;
	memcpy(rt_task->fun_args, arg, narg*sizeof(int));
	if (net_rpc) {
		memcpy(((struct fun_args *)rt_task->fun_args) + 1, (void *)arg[4], arg[5]);
		rt_task->fun_args[4] = (int)(((struct fun_args *)rt_task->fun_args) + 1);
	}

	TRACE_RTAI_LXRT(TRACE_RTAI_EV_LXRT_RESUME,
			rt_task->lnxtsk ? (rt_task->lnxtsk)->pid : -1, type, 0);
/*
 * Here type > 0 means any messaging with the need of copying from/to user
 * space. My knowledge of Linux memory menagment has led to this mess.
 * Whoever can do it better is warmly welcomed.
 */
	wsize = w2size = 0 ;
	wmsg_adr = w2msg_adr = 0;
	if (NEED_TO_RW(type)) {
		int msg_size, rsize, r2size;
		int *fun_args;
		
		fun_args = (net_rpc ? (int *)rt_task->fun_args[4] : rt_task->fun_args) - 1;
		rsize = r2size = 0;
		if( NEED_TO_R(type)) {			
			rsize = USP_RSZ1(type);
			rsize = rsize ? *(fun_args + rsize) : (USP_RSZ1LL(type) ? sizeof(long long) : sizeof(int));
		}
		if (NEED_TO_W(type)) {
			wsize = USP_WSZ1(type);
			wsize = wsize ? *(fun_args + wsize) : (USP_WSZ1LL(type) ? sizeof(long long) : sizeof(int));
		}
		if ((msg_size = rsize > wsize ? rsize : wsize) > 0) {
			if (msg_size > rt_task->max_msg_size[0]) {
				rt_free(rt_task->msg_buf[0]);
				rt_task->max_msg_size[0] = (msg_size*120 + 50)/100;
				rt_task->msg_buf[0] = rt_malloc(rt_task->max_msg_size[0]);
			}
			if (rsize > 0) {			
				int *buf_arg, *rmsg_adr;
				buf_arg = fun_args + USP_RBF1(type);
				rmsg_adr = (int *)(*buf_arg);
				*(buf_arg) = (int)rt_task->msg_buf[0];
				if (rmsg_adr) {
					copy_from_user(rt_task->msg_buf[0], rmsg_adr, rsize);
				}
			}
			if (wsize > 0) {
				int *buf_arg;
				buf_arg = fun_args + USP_WBF1(type);
				wmsg_adr = (int *)(*buf_arg);
				*(buf_arg) = (int)rt_task->msg_buf[0];
			}
		}
/*
 * 2nd buffer next.
 */
		if (NEED_TO_R2ND(type)) {
			r2size = USP_RSZ2(type);
			r2size = r2size ? *(fun_args + r2size) : (USP_RSZ2LL(type) ? sizeof(long long) : sizeof(int));
		}
		if (NEED_TO_W2ND(type)) {
			w2size = USP_WSZ2(type);
			w2size = w2size ? *(fun_args + w2size) : (USP_WSZ2LL(type) ? sizeof(long long) : sizeof(int));
		}
		if ((msg_size = r2size > w2size ? r2size : w2size) > 0) {
			if (msg_size > rt_task->max_msg_size[1]) {
				rt_free(rt_task->msg_buf[1]);
				rt_task->max_msg_size[1] = (msg_size*120 + 50)/100;
				rt_task->msg_buf[1] = rt_malloc(rt_task->max_msg_size[1]);
			}
			if (r2size > 0) {
				int *buf_arg, *r2msg_adr;
				buf_arg = fun_args + USP_RBF2(type);
				r2msg_adr = (int *)(*buf_arg);
				*(buf_arg) = (int)rt_task->msg_buf[1];
				if (r2msg_adr) {
					copy_from_user(rt_task->msg_buf[1], r2msg_adr, r2size);
       				}
       			}
			if (w2size > 0) {
				int *buf_arg;
				buf_arg = fun_args + USP_WBF2(type);
				w2msg_adr = (int *)(*buf_arg);
       		        	*(buf_arg) = (int)rt_task->msg_buf[1];
       			}
		}
	}
/*
 * End of messaging mess.
 */
	if (signal_pending(current)) {
		rt_task->force_soft = FORCE_SOFT;
	}
	rt_global_cli();
	rem_ready_task(rt_task);
	LXRT_RESUME(rt_task);

	rt_global_cli();
	if (!((struct fun_args *)rt_task->fun_args)->fun) {
		LXRT_RESTART(rt_task, buddy_fun);
		rt_global_sti();
		give_back_to_linux(rt_task, 0);
	        rt_task->linux_signal_handler = 0;
		force_sig(rt_task->trap_signo, rt_task->lnxtsk);
	} else if (rt_task->state) {
		if (rt_task->is_hard) {
			rt_release_global_lock();
			lxrt_schedule(hard_cpu_id());
			hard_sti();
		} else {
			current->state = TASK_INTERRUPTIBLE;
			rt_global_sti();
			schedule();
		}
	} else {
		rt_global_sti();
	}
/*
 * A trashing of the comment about messaging mess at the beginning.
 */
	if (rt_task->linux_signal_handler && rt_task->linux_signal) {
		rt_task->linux_signal_handler(rt_task->linux_signal);
		rt_task->linux_signal = 0;
	} else { // Do not try to write if system call was aborted.
		if (wsize > 0 && wmsg_adr) {
			copy_to_user(wmsg_adr, rt_task->msg_buf[0], wsize);
		}
		if (w2size > 0 && w2msg_adr) {
			copy_to_user(w2msg_adr, rt_task->msg_buf[1], w2size);
		}
	}
	return rt_task->retval; // On our way back to user space.
}

/*
 * The inline function __lxrt_resume, found above, is used only for the very
 * basic LXRT call, its use for special internal wait/signal calls on the 
 * steal_give semaphore or, in rare and/or likely non time critical calls, is 
 * done with the function and macros below to save memory space. The overhead 
 * will be negligeable anyhow.
 */

static long long lxrt_resume(void *fun, int narg, int *arg, unsigned long long type, RT_TASK *task)
{
	return __lxrt_resume(fun, narg, arg, type, task, 0);
}

static inline RT_TASK* __task_init(unsigned long name, int prio, int stack_size, int max_msg_size, int cpus_allowed)
{
	void *msg_buf;
	RT_TASK *rt_task;

	if (rt_get_adr(name)) {
		return 0;
	}
	if (prio > RT_LOWEST_PRIORITY) {
		prio = RT_LOWEST_PRIORITY;
	}
	if (!stack_size) {
		stack_size = STACK_SIZE;
	}
	if (!max_msg_size) {
		max_msg_size = MSG_SIZE;
	}
	if (!(msg_buf = rt_malloc(2*max_msg_size))) {
		return 0;
	}
	rt_task = rt_malloc(sizeof(RT_TASK) + 3*sizeof(struct fun_args)); 
	rt_task->magic = 0;
	if (rt_task && !rt_task_init(rt_task, (void *)buddy_fun, (int)rt_task, stack_size, prio, 0, 0)) {
		rt_task->fun_args = (int *)((struct fun_args *)(rt_task + 1));
		rt_task->msg_buf[0] = msg_buf;
		rt_task->msg_buf[1] = msg_buf + max_msg_size;
		rt_task->max_msg_size[0] =
		rt_task->max_msg_size[1] = max_msg_size;
		rt_task->prevp =
		rt_task->nextp = rt_task;
		rt_task->pstate         =
		rt_task->state          =
		rt_task->suspdepth      =
		rt_task->errno          =
		rt_task->linux_signal   =
		rt_task->usp_flags      =
		rt_task->usp_flags_mask =
		rt_task->force_soft     =
		rt_task->is_hard        = 0;
		rt_task->linux_signal_handler = 0;
		rt_task->usp_signal = 0;
		rt_task->lnxtsk = current;

		if (smp_num_cpus > 1 && cpus_allowed) {
			current->cpus_allowed = cpus_allowed;
			while (!test_bit(hard_cpu_id(), &cpus_allowed)) {
				current->state = TASK_INTERRUPTIBLE;
				schedule_timeout(2);
			}
		}
		if (rt_register(name, rt_task, IS_TASK, 0)) {
			current->this_rt_task[0] = rt_task;
			current->this_rt_task[1] = current;
			return rt_task;
		} else {
			rt_task_delete(rt_task);
		}
	}
	rt_free(rt_task);
	rt_free(msg_buf);
	return 0;
}

static inline int __task_delete(RT_TASK *rt_task)
{
	struct task_struct *process;
	process = rt_task->lnxtsk;
	rt_qCleanup(rt_task);
	if (rt_task_delete(rt_task)) {
		return -EFAULT;
	}
	rt_free(rt_task->msg_buf[0]);
	rt_free(rt_task);
	if (process) {
		process->this_rt_task[0] = process->this_rt_task[1] = 0;
	}
	return (!rt_drg_on_adr(rt_task)) ? -ENODEV : 0;
}

static int tasks_per_cpu[NR_RT_CPUS];

static inline int get_min_tasks_cpuid(void)
{
	int i, cpuid, min;
	min =  tasks_per_cpu[cpuid = 0];
	for (i = 1; i < NR_RT_CPUS; i++) {
		if (tasks_per_cpu[i] < min) {
			min = tasks_per_cpu[cpuid = i];
		}
	}
	return cpuid;
}

static int SingleCpu = SINGLE_CPU;
MODULE_PARM(SingleCpu, "i");

static inline void assign_cpu(void)
{
	if (smp_num_cpus > 1) {
		if (SingleCpu >= 0) {
			current->cpus_allowed = 1 << SingleCpu;
		} else if (hweight32(current->cpus_allowed) > 1) {
			current->cpus_allowed = 1 << get_min_tasks_cpuid();
		}
		while (current->cpus_allowed != (1 << hard_cpu_id())) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(2);
		}
	}
	tasks_per_cpu[hard_cpu_id()]++;
}

static inline void release_cpu(void) {
	int cpuid;
	if (tasks_per_cpu[cpuid = hard_cpu_id()]) { 
		tasks_per_cpu[cpuid]--; 
	}
}

long long lxrt_handler(unsigned int lxsrq, void *arg, struct pt_regs regs)
{
#define larg ((struct arg *)arg)
#define ar   ((unsigned long *)arg)
#define MANYARGS ar[0],ar[1],ar[2],ar[3],ar[4],ar[5],ar[6],ar[7],ar[8],ar[9]

	union {unsigned long name; RT_TASK *rt_task; SEM *sem; MBX *mbx; } arg0;
	int srq;
	RT_TASK *task;

//	if (current != CURRENT) {
//		rt_printk("(HDLR) CURRENT (%d) != current (%d).\n", current->pid, CURRENT->pid);
//	}
	TRACE_RTAI_LXRT(TRACE_RTAI_EV_LXRT_HANDLE, SRQ(lxsrq), 0, 0);

	arg0.name = ar[0];
   
	if (rt_is_lxrt() && (task = current->this_rt_task[0]) && task->force_soft && task->is_hard) { // Only in lxrt mode so we can reenter. 
		task->force_soft = 0;
		task->usp_flags &= ~FORCE_SOFT;
		give_back_to_linux(task, 0);
	}

	srq = SRQ(lxsrq);
	if (srq < MAX_LXRT_FUN) {
		int idx;
		unsigned long long type;
		struct rt_fun_entry *funcm;
/*
 * The next two lines of code do a lot. It makes possible to extend the use of
 * LXRT to any other real time module service in user space, both for soft and
 * hard real time. Concept contributed and copyrighted by: Giuseppe Renoldi 
 * (giuseppe@renoldi.org).
 */
		idx   = INDX(lxsrq);
		funcm = rt_fun_ext[idx];

		if (!funcm) {
			rt_printk("BAD: null rt_fun_ext[%d]\n", idx);
			return 0;
		}

#ifdef ALLOW_SYSW
		if ((task = current->this_rt_task[0]) && task->pstate) {
			task->pstate = 0;
			SYSW_DIAG_MSG(rt_printk("GOING BACK TO HARD, PID = %d.\n", current->pid););
			steal_from_linux(task);
		}
#endif
   
		if ((type = funcm[srq].type)) {
			if (!rt_is_linux()) { 
				// Any type 1 can now reenter.
				// Buuut, you cannot rt_sleep() or do anything
				// that would cause a switch back to Linux
				// because exec_func() has done damage.
				return ((long long (*)(unsigned long, ...))funcm[srq].fun)(MANYARGS);
			} else {
			        int net_rpc;
			        net_rpc = idx == 2 && !srq;
				return __lxrt_resume(funcm[srq].fun, NARG(lxsrq), (int *)arg, net_rpc ? ((long long *)((int *)arg + 2))[0] : type, current->this_rt_task[0], net_rpc);
			}
		} else {
			return ((long long (*)(unsigned long, ...))funcm[srq].fun)(MANYARGS);
	        }
	}

	switch (srq) {
		case GET_ADR: {
			return (unsigned long)rt_get_adr(arg0.name);
		}

		case GET_NAME: {
			return rt_get_name((void *)arg0.name);
		}

		case TASK_INIT: {
			struct arg { int name, prio, stack_size, max_msg_size, cpus_allowed; };
			return (unsigned long) __task_init(arg0.name, larg->prio, larg->stack_size, larg->max_msg_size, larg->cpus_allowed);
		}

		case TASK_DELETE: {
			return __task_delete(arg0.rt_task);
		}

		case SEM_INIT: {
			if (rt_get_adr(arg0.name)) {
				return 0;
			}
			if ((arg0.sem = rt_malloc(sizeof(SEM)))) {
				struct arg { int name; int cnt; int typ; };
				rt_typed_sem_init(arg0.sem, larg->cnt, larg->typ);
				if (rt_register(larg->name, arg0.sem, IS_SEM, current)) {
					return arg0.name;
				} else {
					rt_free(arg0.sem);
				}
			}
			return 0;
		}

		case SEM_DELETE: {
			if (rt_sem_delete(arg0.sem)) {
				return -EFAULT;
			}
			rt_free(arg0.sem);
			return rt_drg_on_adr(arg0.sem);
		}

		case MBX_INIT: {
			if (rt_get_adr(arg0.name)) {
				return 0;
			}
			if ((arg0.mbx = rt_malloc(sizeof(MBX)))) {
				struct arg { int name; int size; int qtype; };
				if (rt_typed_mbx_init(arg0.mbx, larg->size, larg->qtype) < 0) {
					rt_free(arg0.mbx);
					return 0;
				}
				if (rt_register(larg->name, arg0.mbx, IS_MBX, current)) {
					return arg0.name;
				} else {
					rt_free(arg0.mbx);
				}
			}
			return 0;
		}

		case MBX_DELETE: {
			if (rt_mbx_delete(arg0.mbx)) {
				return -EFAULT;
			}
			rt_free(arg0.mbx);
			return rt_drg_on_adr(arg0.mbx);
		}
#ifdef HARD_LXRT	
		case MAKE_HARD_RT: {
			if (!(task = current->this_rt_task[0]) || task->is_hard){
				 return 0;
			}
			assign_cpu();
			steal_from_linux(task);
			return 0;
		}

		case MAKE_SOFT_RT: {
			if (!(task = current->this_rt_task[0]) || !task->is_hard) {
				if (task->pstate) {
					task->pstate = 0;
					release_cpu();
				}
				return 0;
			}
			give_back_to_linux(task, 0);
			release_cpu();
			return 0;
		}
#endif
		case PRINT_TO_SCREEN: {
			struct arg { char *display; int nch; };
			return rtai_print_to_screen("%s", larg->display);
		}

		case PRINTK: {
			struct arg { char *display; int nch; };
			return rt_printk("%s", larg->display);
		}

		case NONROOT_HRT: {
			current->cap_effective |= ((1 << CAP_IPC_LOCK)  |
						   (1 << CAP_SYS_RAWIO) | 
						   (1 << CAP_SYS_NICE));
			return 0;
		}

		case RT_BUDDY: {
			return current->this_rt_task[0] && 
			       current->this_rt_task[1] == current ?
			       (unsigned long)(current->this_rt_task[0]) : 0;
		}

		case HRT_USE_FPU: {
			struct arg { RT_TASK *task; int use_fpu; };
			if(!larg->use_fpu) {
				((larg->task)->lnxtsk)->used_math = 0;
				((larg->task)->lnxtsk)->flags |= PF_USEDFPU;
			} else {
				init_fpu((larg->task)->lnxtsk);
			}
			return 0;
		}

		case USP_SIGHDL: {
			((RT_TASK *)(current->this_rt_task[0]))->usp_signal = (void *)arg0.name;
			return 0;
		}

		case GET_USP_FLAGS: {
			return arg0.rt_task->usp_flags;
		}

		case SET_USP_FLAGS: {
			struct arg { RT_TASK *task; unsigned long flags; };
			arg0.rt_task->usp_flags = larg->flags;
			arg0.rt_task->force_soft = larg->flags & arg0.rt_task->usp_flags_mask & FORCE_SOFT;
			return 0;
		}

		case GET_USP_FLG_MSK: {
			return arg0.rt_task->usp_flags_mask;
		}

		case SET_USP_FLG_MSK: {
			(task = current->this_rt_task[0])->usp_flags_mask = arg0.name;
			task->force_soft = task->usp_flags & arg0.name & FORCE_SOFT;
			return 0;
		}

		case FORCE_TASK_SOFT: {
			struct task_struct *ltsk;
			if ((ltsk = find_task_by_pid(arg0.name)))  {
				if ((arg0.rt_task = ltsk->this_rt_task[0])) {
					arg0.rt_task->force_soft = FORCE_SOFT;
					return (unsigned long)arg0.rt_task;
				}
			}
			return 0;
		}

		case IS_HARD: {
			return arg0.rt_task->is_hard;
		}

		case LXRT_FORK: {
			struct arg { int is_a_clone; };
			return rt_lxrt_fork(&regs, larg->is_a_clone);
		}
		case ALLOC_REGISTER: {
			struct arg { int name; int size; int type; };
			void *adr;
			if ((adr = rt_get_adr(larg->name))) {
				return (unsigned long)adr;
			}
			if ((adr = rt_malloc(larg->size))) {
				if (rt_register(larg->name, adr, larg->type, current)) {
					return (unsigned long)adr;
				} else {
					rt_free(adr);
				}
			}
			return 0;
		}

		case DELETE_DEREGISTER: {
			rt_free((void *)arg0.name);
			return rt_drg_on_adr((void *)arg0.name);
		}
		case GET_EXECTIME: {
			struct arg { RT_TASK *task; RTIME *exectime; };
			if ((larg->task)->exectime[0] && (larg->task)->exectime[1]) {
				larg->exectime[0] = (larg->task)->exectime[0];
				larg->exectime[1] = (larg->task)->exectime[1];
				larg->exectime[2] = rdtsc();
			}
		}
	}
	return 0;
}

static void lxrt_srq_handler(void)
{
	int cpuid;
	struct klist_t *klistp;
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		klistp = klistep[cpuid];
		if (klistp->out != klistp->in && kthreade[cpuid]) {
			wake_up_process(kthreade[cpuid]);
		}
	}
	while (sysrq.out != sysrq.in) {
		if (((struct task_struct *)sysrq.task[sysrq.out])->state == TASK_INTERRUPTIBLE) {
			wake_up_process(sysrq.task[sysrq.out]);
		}
		sysrq.out = (sysrq.out + 1) & (MAX_SRQ - 1);
	}
	current->need_resched = 1;
}

static struct desc_struct sidt;

#ifdef HARD_LXRT
static void (*linux_syscall_handler)(void);

static int inhrtp(unsigned long) __attribute__ ((__unused__));

static int inhrtp(unsigned long eax)
{
	if (!test_bit(hard_cpu_id(), &lxrt_hrt_flags)) {
		return 0;
	} else {

#ifdef ALLOW_SYSW
		RT_TASK *task;
		(task = current->this_rt_task[0])->pstate = 1;
		give_back_to_linux(task, 0);
		SYSW_DIAG_MSG(rt_printk("FORCING IT SOFT, PID = %d, EIP = %lx.\n", current->pid, *(&eax - 1)););

		return 0;
#endif

		rt_printk("LINUX SYSCALL (%ld) FROM HRTP, PID = %d, EIP = %p, BAD!\n", eax, current->pid, (void *)*(&eax - 1));
		return 1;
	}
}

DEFINE_LXRT_SYSCALL_HANDLER

static void lxrt_sigsrq_handler(void)
{
/*
 * This is a Linux context handler responding to system request from RTAI.
*/
	struct task_struct *tsk;
	int sig;
	RT_TASK *rt_task;

        while (sigsysrq.out != sigsysrq.in) {
                tsk     = sigsysrq.waitq[sigsysrq.out].tsk;
                sig     = sigsysrq.waitq[sigsysrq.out].sig;
                rt_task = sigsysrq.waitq[sigsysrq.out].rt_task;
		LXRT_RESTART(rt_task, buddy_fun);
		if (tsk && sig) {
	                kill_proc(tsk->pid, sig, 0);
		} else {
			int cpuid;
			struct klist_t *klistp;
			cpuid = ffnz((rt_task->lnxtsk)->cpus_allowed);
			klistp = klistep[cpuid];
			hard_cli();
			klistp->task[klistp->in] = rt_task->lnxtsk;
			klistp->in = (klistp->in + 1) & (MAX_SRQ - 1);
			hard_sti();
                	rt_pend_linux_srq(sysrq.srq);
		} 
                sigsysrq.out = (sigsysrq.out + 1) & (MAX_SRQ - 1);
        }
}

static int htrp7;

int lxrt_trap_handler(int vec, int signo, struct pt_regs *r, void *dummy_data)
{
	RT_TASK	*rt_task;
	struct task_struct *ltsk;
	int cpuid;

//rt_printk("LXRT Trap Handler: vec %d\n", vec);

	if (vec == 7) {
/*
 * We prefer to anticipate this by explicitely declaring a hard real time
 * user space task is using the FPU, it should also reduce jitter. However
 * we cannot be completely sure of what GCC and related libs are doing. So
 * we add these laces to our trousers belt. At least we should not risk
 * finding our ass exposed to the wind :-) in the middle of a critical task.
 */
		do {
			unsigned long flags;
			hard_save_flags_and_cli(flags);
			if (!test_and_set_bit(IFLAG, &flags)) {
				__cli();
			}
			hard_restore_flags(flags);
		} while (0);

		if ((ltsk = lxrt_current[hard_cpu_id()]->lnxtsk) && ltsk->used_math) {
			restore_fpu(ltsk);
		} else {
			init_fpu(ltsk);
		}
		htrp7++;
		return 1;
	}
	
	if (vec == 14) {
		unsigned long address;
		address = get_cr2();
		if (address >= TASK_SIZE) {
			if (!(r->orig_eax & 5)) {
				return 0;
			} else {
				rt_printk("vmalloc_fault at %p.\n", (void *)address);
			}
		}
	}

	rt_global_cli(); // do not allow anybody else.
	if (!rt_is_linux()) {
		rt_task = rt_whoami();
		ltsk = rt_task->lnxtsk; 
		if (ltsk && (ltsk->this_rt_task[0] == rt_task)) {
			rt_printk("LXRT Trap Handler: vector %d: Suspend User Space Soft RT task %p, pid %d, name %s.\n", vec, rt_task, ltsk->pid, ltsk->comm);
			if (rt_task->trap_handler_data) {
          			(((struct handler_data_t *)rt_task->trap_handler_data)->unexec_func)();
			}
			if (r->xcs == __KERNEL_CS && rt_task->is_hard) {
				// Tough, LXRT task in RTAI context.
				// We have to change to another stack.
				((struct fun_args *)rt_task->fun_args)->fun = 0;
				rt_task->trap_signo = signo;
			} else {
				rt_signal_linux_task(ltsk, signo, rt_task);
			}
			lxrt_suspend(rt_task);
		} else {
			if (rt_task->task_trap_handler[vec]) {
				int retval;
				retval = rt_task->task_trap_handler[vec](vec, signo, r, rt_task);
				rt_global_sti();
				return retval;
			}
			rt_printk("LXRT Trap Handler: vector %d: Suspend Plain RTAI task %p.\n", vec, rt_task);
			rt_global_sti();
			rt_task_suspend(rt_task);
			return 1;
		}
	} else {
		rt_task = (RT_TASK *)lxrt_current[cpuid = hard_cpu_id()];
		ltsk    = rt_task->lnxtsk;
		rt_printk("LXRT Trap Handler: vector %d: Suspend User Space HRT task %p, pid %d, name %s.\n", vec, rt_task, ltsk->pid, ltsk->comm);
		if (!rt_task->state && rt_task->nextp != rt_task) {
			// Sort of easier, the error occured in user space. 
			LXRT_RESTART(rt_task, buddy_fun);
			((struct fun_args *)rt_task->fun_args)->fun = (void *)rt_get_timer_cpu;
			rem_ready_task(rt_task);
			LXRT_RESUME(rt_task);
			rt_global_sti();
			give_back_to_linux(rt_task, IN_TRAP);
			rt_global_cli();
			LXRT_RESUME(rt_task);
			rt_global_sti();
			//do_exit(SIGKILL);
			rt_task->linux_signal_handler = 0;
			force_sig(signo, ltsk);
			return 1;
		} else {
			rt_global_sti();
			rt_printk("LXRT Trap Handler: vector %d: Ooops...\n", vec);
			return 1;
		}
	}
	rt_global_sti();
	return 1; // To make compiler happy.
}

#endif

int set_rt_fun_ext_index(struct rt_fun_entry *fun, int idx)
{
	if (idx > 0 && idx < MAX_FUN_EXT && !rt_fun_ext[idx]) {
		rt_fun_ext[idx] = fun;
		return 0;
	}
	return -EACCES;
}

void reset_rt_fun_ext_index( struct rt_fun_entry *fun, int idx)
{
	if (idx > 0 && idx < MAX_FUN_EXT && rt_fun_ext[idx] == fun) {
		rt_fun_ext[idx] = 0;
	}
}

int init_module(void)
{
#ifdef HARD_LXRT
	int i, cpuid;

	sidt = rt_set_full_intr_vect(RTAI_LXRT_VECTOR, 15, 3, (void *)RTAI_LXRT_HANDLER);
	if ((sysrq.srq = rt_request_srq(nam2num("LXRT"), lxrt_srq_handler, 0)) < 0) {
		printk("LXRT: no sysrq available.\n");
		return sysrq.srq;
	}
	sysrq.in = sysrq.out = 0;

	if ((sigsysrq.srq = rt_request_srq(nam2num("LSIG"), lxrt_sigsrq_handler, 0)) < 0) {
		printk("LXRT: no sigsysrq available.\n");
		return sigsysrq.srq;
	}
	sigsysrq.in = sigsysrq.out = 0;

	if(set_rtai_callback(linux_process_termination)) {
		printk("Could not setup rtai_callback\n");
		return -ENODEV;
	}

	rt_fun_ext[0] = rt_fun_lxrt;
	for (i = 1 ; i < MAX_FUN_EXT ; i++) {
		rt_fun_ext[i] = 0;
	}

	linux_syscall_handler = rt_set_intr_handler(LINUX_SYS_VECTOR, (void *)LXRT_LINUX_SYSCALL_TRAP);

	rt_get_base_linux_task(rt_linux_tasks);
	for (cpuid = 0; cpuid < smp_num_cpus; cpuid++) {
		lxrt_current[cpuid]   = 
		rt_linux_tasks[cpuid] = rt_linux_tasks[0] + cpuid;
                rt_linux_tasks[cpuid]->prevp = 
		rt_linux_tasks[cpuid]->nextp = rt_linux_tasks[cpuid];
		rt_linux_tasks[cpuid]->is_hard = 0;
		rt_linux_tasks[cpuid]->usp_signal = 0;
		linux_flags[cpuid] = (1 << IFLAG) | (1 << cpuid);
		kernel_thread((void *)kthread_b, &cpuid, 0);
		kernel_thread((void *)kthread_e, &cpuid, 0);
		while (!kthreadb[cpuid] || !kthreade[cpuid]) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(1);
		}
	}
	rt_linux_tasks[0]->task_trap_handler[0] = (void *)set_rt_fun_ext_index;
	rt_linux_tasks[0]->task_trap_handler[1] = (void *)reset_rt_fun_ext_index;
	rthal_enint    = rthal.enint;
	rthal_setflags = (void *)rthal.setflags;
	rthal.enint    = (void *)lxrt_enint;
	rthal.setflags = (void *)lxrt_setflags;

	sched_trap_handler = rt_set_rtai_trap_handler(lxrt_trap_handler);
#endif

#ifdef LINUX_SIGNAL_HANDLER
	rt_init_linux_signal_handler();
#endif

#ifdef CONFIG_RTAI_ADEOS
	arti_attach_lxrt();
#endif /* CONFIG_RTAI_ADEOS */

#ifdef CONFIG_PROC_FS
	rtai_proc_lxrt_register();
#endif
		
	return 0 ;
}

void cleanup_module(void)
{
	int cpuid;

#ifdef CONFIG_PROC_FS
	rtai_proc_lxrt_unregister();
#endif
		
#ifdef CONFIG_RTAI_ADEOS
	arti_detach_lxrt();
#endif /* CONFIG_RTAI_ADEOS */

#ifdef LINUX_SIGNAL_HANDLER
    rt_remove_linux_signal_handler();
#endif

	remove_rtai_callback(linux_process_termination);

#ifdef HARD_LXRT

	rt_set_rtai_trap_handler(sched_trap_handler);   // Replace original

	rthal.enint = rthal_enint;
	rthal.setflags = (void *)rthal_setflags;
       	rthal.lxrt_global_cli = 0;
	for (cpuid = 0; cpuid < smp_num_cpus; cpuid++) {
		SET_SIG_FUN(rt_linux_tasks[cpuid], cpuid, 0);
	}

        if (rt_free_srq(sigsysrq.srq) < 0) {
                printk("LXRT: sigsysrq %d illegal or already free.\n", sigsysrq.srq);
        }

	if (rt_free_srq(sysrq.srq) < 0) {
		printk("LXRT: sysrq %d illegal or already free.\n", sysrq.srq);
	}

	rt_reset_intr_handler(LINUX_SYS_VECTOR, linux_syscall_handler);
#endif
	rt_reset_full_intr_vect(RTAI_LXRT_VECTOR, sidt);
	if (htrp7) {
		printk("\nLXRT HAD TO MANAGE %d UNEXPECTED TRAP7.\n\n", htrp7);
	}

	endthread = 1;
	for (cpuid = 0; cpuid < smp_num_cpus; cpuid++) {
		wake_up_process(kthreadb[cpuid]);
		wake_up_process(kthreade[cpuid]);
		while (kthreadb[cpuid] || kthreade[cpuid]) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(1);
		}
	}
	return;
}

#ifdef CONFIG_PROC_FS
/* ----------------------< proc filesystem section >----------------------*/

static int rtai_read_lxrt(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	PROC_PRINT_VARS;
	struct rt_registry_entry_struct entry;
	char* type_name[4] = { "TASK","SEM","MBX","PRX" };
	unsigned int i = 1;
	char name[8];

	PROC_PRINT("\nRTAI LXRT Information.\n\n");
	PROC_PRINT("    MAX_SLOTS = %d\n\n",MAX_SLOTS);

    //                       1234 123456 0x12345678 UNKNOWN  0x12345678 0x12345678 12345678

	PROC_PRINT("                                           Linux  Owner Task  Linux Parent\n");
	PROC_PRINT("Slot Name   ID         Type     RT Handle  Pointer       PID           PID\n");
	PROC_PRINT("--------------------------------------------------------------------------\n");
	for (i = 1; i <= MAX_SLOTS; i++) {
		if (rt_get_registry_slot(i, &entry)) {
			num2nam(entry.name, name);
			PROC_PRINT("% 4d %-6.6s 0x%08lx %-8.8s 0x%p 0x%p % 6d        %6d\n",
			i,    			// the slot number
			name,       		// the name in 6 char asci
			entry.name, 		// the name as unsigned long hex
			entry.type > 3 ? 
			"UNKNOWN" : 
			type_name[entry.type],	// the Type
			entry.adr,		// The RT Handle
			entry.tsk,   		// The Owner task pointer
			entry.pid,   		// The Owner PID
			entry.type == IS_TASK && ((RT_TASK *)entry.adr)->lnxtsk ? (((RT_TASK *)entry.adr)->lnxtsk)->pid : 0);
		 }
	}
        PROC_PRINT_DONE;
}  /* End function - rtai_read_lxrt */

static int rtai_proc_lxrt_register(void)
{
	struct proc_dir_entry *proc_lxrt_ent;


	proc_lxrt_ent = create_proc_entry("lxrt", S_IFREG|S_IRUGO|S_IWUSR, rtai_proc_root);
	if (!proc_lxrt_ent) {
		printk("Unable to initialize /proc/rtai/lxrt\n");
		return(-1);
	}
	proc_lxrt_ent->read_proc = rtai_read_lxrt;
	return(0);
}  /* End function - rtai_proc_lxrt_register */


static void rtai_proc_lxrt_unregister(void)
{
	remove_proc_entry("lxrt", rtai_proc_root);
}  /* End function - rtai_proc_lxrt_unregister */

/* ------------------< end of proc filesystem section >------------------*/
#endif /* CONFIG_PROC_FS */

// Include lxk_ prefixed functions for async IO (aio) support.
#include "lxk.c"
