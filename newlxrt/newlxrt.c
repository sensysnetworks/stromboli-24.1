/*
COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it),
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/unistd.h>
#include <linux/mman.h>

#include <asm/uaccess.h>

#define INTERFACE_TO_LINUX
#include <rtai/version.h>
#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>
#include <rtai_trace.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#include <rtai_nam2num.h>

extern struct proc_dir_entry *rtai_proc_root;
static int rtai_proc_lxrt_register(void);
static void rtai_proc_lxrt_unregister(void);
#endif

#include "registry.h"
#include "proxies.h"
#include "msgnewlxrt.h"

/*
 * This is to make modutils happy. As GPL is a subset of LGPL, it is 
 * correct to tell modutils that this module complies with the GPL. 
 */  
MODULE_LICENSE("GPL");

#define MAX_FUN_EXT  16
static struct rt_fun_entry *rt_fun_ext[MAX_FUN_EXT];

/*
 * WATCH OUT for the max expected size of messages and arguments of rtai funs;
 */

#define MSG_SIZE    256  // Default max message size.
#define MAX_ARGS    10   // Max number of 4 bytes args in rtai functions.

DEFINE_LXRT_HANDLER

extern int use_buddy_version;
extern int get_min_tasks_cpuid(void);
extern int set_rtext(RT_TASK *, int, int, void(*)(void), unsigned int, struct task_struct *);
extern int clr_rtext(RT_TASK *);
extern void steal_from_linux(RT_TASK *);
extern void give_back_to_linux(RT_TASK *);
extern void rt_schedule_soft(RT_TASK *);
extern void start_buddy(RT_TASK *);
extern struct task_struct *end_buddy(RT_TASK *);

struct fun_args { int a0; int a1; int a2; int a3; int a4; int a5; int a6; int a7; int a8; int a9; long long (*fun)(int, ...); };

static inline long long lxrt_resume(void *fun, int narg, int *arg, unsigned long long type, RT_TASK *rt_task, int net_rpc)
{
	int wsize, w2size;
	int *wmsg_adr, *w2msg_adr;
	struct fun_args *funarg;

	memcpy(funarg = (void *)rt_task->fun_args, arg, narg*sizeof(int));
	funarg->fun = fun;
	if (net_rpc) {
		memcpy((void *)(rt_task->fun_args[4] = (int)(funarg + 1)), (void *)arg[4], arg[5]);
	}
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
	if ((int)rt_task->is_hard > 0) {
		rt_task->retval = ((long long (*)(int, ...))fun)(funarg->a0, funarg->a1, funarg->a2, funarg->a3, funarg->a4, funarg->a5, funarg->a6, funarg->a7, funarg->a8, funarg->a9);
	} else {
		rt_schedule_soft(rt_task);
	}
/*
 * A trashing of the comment about messaging mess at the beginning.
 */
	if (wsize > 0 && wmsg_adr) {
		copy_to_user(wmsg_adr, rt_task->msg_buf[0], wsize);
	}
	if (w2size > 0 && w2msg_adr) {
		copy_to_user(w2msg_adr, rt_task->msg_buf[1], w2size);
	}
	return rt_task->retval;
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
	if (!max_msg_size) {
		max_msg_size = MSG_SIZE;
	}
	if (!(msg_buf = rt_malloc(2*max_msg_size))) {
		return 0;
	}
	rt_task = rt_malloc(sizeof(RT_TASK) + 3*sizeof(struct fun_args)); 
	rt_task->magic = 0;
	if (smp_num_cpus > 1 && cpus_allowed) {
		cpus_allowed = hweight32(cpus_allowed) > 1 ? get_min_tasks_cpuid() : ffnz(cpus_allowed);
	} else {
		cpus_allowed = 0;
	}
	if (rt_task) {
	    if (!set_rtext(rt_task, prio, 0, 0, cpus_allowed, 0)) {
	        rt_task->fun_args = (int *)((struct fun_args *)(rt_task + 1));
		rt_task->msg_buf[0] = msg_buf;
		rt_task->msg_buf[1] = msg_buf + max_msg_size;
		rt_task->max_msg_size[0] =
		rt_task->max_msg_size[1] = max_msg_size;
		if (rt_register(name, rt_task, IS_TASK, 0)) {
			extern void *sys_call_table[];
/* mlockalls can be many without any harm, so we'll give a hint anyhow */
			((void (*)(int))sys_call_table[__NR_mlockall])(MCL_CURRENT | MCL_FUTURE);
			return rt_task;
		} else {
			clr_rtext(rt_task);
		}
	    }
	    rt_free(rt_task);
	}
	rt_free(msg_buf);
	return 0;
}

static int __task_delete(RT_TASK *rt_task)
{
	struct task_struct *process;
	end_buddy(rt_task);
	if (clr_rtext(rt_task)) {
		return -EFAULT;
	}
	rt_free(rt_task->msg_buf[0]);
	rt_free(rt_task);
	if ((process = rt_task->lnxtsk)) {
		process->this_rt_task[0] = process->this_rt_task[1] = 0;
	}
	return (!rt_drg_on_adr(rt_task)) ? -ENODEV : 0;
}

//#define ECHO_SYSW
#ifdef ECHO_SYSW
#define SYSW_DIAG_MSG(x) x
#else
#define SYSW_DIAG_MSG(x)
#endif

static inline void __force_soft(RT_TASK *task)
{
	return;
	if (task && task->force_soft) {
		task->force_soft = 0;
		task->usp_flags &= ~FORCE_SOFT;
		give_back_to_linux(task);
	}
}

long long lxrt_handler(unsigned int lxsrq, void *arg, struct pt_regs regs)
{
#define larg ((struct arg *)arg)
#define ar   ((unsigned long *)arg)
#define MANYARGS ar[0],ar[1],ar[2],ar[3],ar[4],ar[5],ar[6],ar[7],ar[8],ar[9]

	union {unsigned long name; RT_TASK *rt_task; SEM *sem; MBX *mbx; RWL *rwl; SPL *spl; } arg0;
	int srq;
	RT_TASK *task;

	__force_soft(task = current->this_rt_task[0]);
	srq = SRQ(lxsrq);
	if (srq < MAX_LXRT_FUN) {
		int idx;
		unsigned long long type;
		struct rt_fun_entry *funcm;
/*
 * The next two lines of code do a lot. It makes possible to extend the use of
 * USP to any other real time module service in user space, both for soft and
 * hard real time. Concept contributed and copyrighted by: Giuseppe Renoldi 
 * (giuseppe@renoldi.org).
 */
		idx   = INDX(lxsrq);
		funcm = rt_fun_ext[idx];

		if (!funcm) {
			rt_printk("BAD: null rt_fun_ext[%d]\n", idx);
			return 0;
		}

		if ((type = funcm[srq].type)) {
			int net_rpc;
			if ((int)task->is_hard > 1) {
				task->is_hard = 1;
				SYSW_DIAG_MSG(rt_printk("GOING BACK TO HARD, PID = %d.\n", current->pid););
				steal_from_linux(task);
			} else if (use_buddy_version && !task->is_hard) {
				start_buddy(task);
			}
			net_rpc = idx == 2 && !srq;
			lxrt_resume(funcm[srq].fun, NARG(lxsrq), (int *)arg, net_rpc ? ((long long *)((int *)arg + 2))[0] : type, task, net_rpc);
			__force_soft(task);
			return task->retval;
		} else {
			return ((long long (*)(unsigned long, ...))funcm[srq].fun)(MANYARGS);
	        }
	}

	arg0.name = ar[0];
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


		case RWL_INIT: {
			if (rt_get_adr(arg0.name)) {
				return 0;
			}
			if ((arg0.rwl = rt_malloc(sizeof(RWL)))) {
				struct arg { int name; };
				rt_rwl_init(arg0.rwl);
				if (rt_register(larg->name, arg0.rwl, IS_SEM, current)) {
					return arg0.name;
				} else {
					rt_free(arg0.rwl);
				}
			}
			return 0;
		}

		case RWL_DELETE: {
			if (rt_rwl_delete(arg0.rwl)) {
				return -EFAULT;
			}
			rt_free(arg0.rwl);
			return rt_drg_on_adr(arg0.rwl);
		}

		case SPL_INIT: {
			if (rt_get_adr(arg0.name)) {
				return 0;
			}
			if ((arg0.spl = rt_malloc(sizeof(SPL)))) {
				struct arg { int name; };
				rt_spl_init(arg0.spl);
				if (rt_register(larg->name, arg0.spl, IS_SEM, current)) {
					return arg0.name;
				} else {
					rt_free(arg0.spl);
				}
			}
			return 0;
		}

		case SPL_DELETE: {
			if (rt_spl_delete(arg0.spl)) {
				return -EFAULT;
			}
			rt_free(arg0.spl);
			return rt_drg_on_adr(arg0.spl);
		}

		case MAKE_HARD_RT: {
			if (!(task = current->this_rt_task[0]) || (int)task->is_hard > 0) {
				 return 0;
			}
			end_buddy(task);
			steal_from_linux(task);
			return 0;
		}

		case MAKE_SOFT_RT: {
			if (!(task = current->this_rt_task[0]) || (int)task->is_hard <= 0) {
				return 0;
			}
			if ((int)task->is_hard > 1) {
				task->is_hard = 0;
			} else {
				give_back_to_linux(task);
			}
			return 0;
		}
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
				init_xfpu();
				((larg->task)->lnxtsk)->used_math = 1;
				((larg->task)->lnxtsk)->flags |= PF_USEDFPU;
			}
			return 0;
		}

                case GET_USP_FLAGS: {
                        return arg0.rt_task->usp_flags;
                }

                case SET_USP_FLAGS: {
                        struct arg { RT_TASK *task; unsigned long flags; };
                        arg0.rt_task->usp_flags = larg->flags;
                        arg0.rt_task->force_soft = ((int)arg0.rt_task->is_hard > 0) && (larg->flags & arg0.rt_task->usp_flags_mask & FORCE_SOFT);
                        return 0;
                }

                case GET_USP_FLG_MSK: {
                        return arg0.rt_task->usp_flags_mask;
                }

                case SET_USP_FLG_MSK: {
                        (task = current->this_rt_task[0])->usp_flags_mask = arg0.name;
                        task->force_soft = ((int)task->is_hard > 0) && (task->usp_flags & arg0.name & FORCE_SOFT);
                        return 0;
                }

                case FORCE_TASK_SOFT: {
                        struct task_struct *ltsk;
                        if ((ltsk = find_task_by_pid(arg0.name)) && (arg0.rt_task = ltsk->this_rt_task[0]) && (int)arg0.rt_task->is_hard > 0) {
				kill_proc(arg0.name, SIGCONT, 0);
				return (unsigned long)arg0.rt_task;
                        }
                        return 0;
                }

		case IS_HARD: {
			return (int)arg0.rt_task->is_hard > 0;
		}
		case GET_EXECTIME: {
			struct arg { RT_TASK *task; RTIME *exectime; };
			if ((larg->task)->exectime[0] && (larg->task)->exectime[1]) {
				larg->exectime[0] = (larg->task)->exectime[0]; 
				larg->exectime[1] = (larg->task)->exectime[1]; 
				larg->exectime[2] = rdtsc(); 
			}
                        return 0;
		}
		case GET_TIMEORIG: {
			struct arg { RTIME *time_orig; };
			rt_gettimeorig(larg->time_orig);
                        return 0;
		}
	}
	return 0;
}

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

static struct desc_struct sidt;

int init_module(void)
{
	RT_TASK *rt_linux_tasks[NR_RT_CPUS];

	sidt = rt_set_full_intr_vect(RTAI_LXRT_VECTOR, 15, 3, (void *)RTAI_LXRT_HANDLER);
	if(set_rtai_callback(linux_process_termination)) {
		printk("Could not setup rtai_callback\n");
		return -ENODEV;
	}
	rt_fun_ext[0] = rt_fun_lxrt;
	rt_get_base_linux_task(rt_linux_tasks);
	rt_linux_tasks[0]->task_trap_handler[0] = (void *)set_rt_fun_ext_index;
	rt_linux_tasks[0]->task_trap_handler[1] = (void *)reset_rt_fun_ext_index;
#ifdef CONFIG_PROC_FS
	rtai_proc_lxrt_register();
#endif
#ifdef CONFIG_RTAI_ADEOS
	arti_attach_lxrt();
#endif /* CONFIG_RTAI_ADEOS */
	return 0 ;
}

void cleanup_module(void)
{
#ifdef CONFIG_RTAI_ADEOS
	arti_detach_lxrt();
#endif /* CONFIG_RTAI_ADEOS */
#ifdef CONFIG_PROC_FS
	rtai_proc_lxrt_unregister();
#endif
	rt_reset_full_intr_vect(RTAI_LXRT_VECTOR, sidt);
	remove_rtai_callback(linux_process_termination);
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

	PROC_PRINT("\nRTAI NEWLXRT Information.\n\n");
	PROC_PRINT("    MAX_SLOTS = %d\n\n", MAX_SLOTS);

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
