
/*
COPYRIGHT (C) 2001  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)

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

File: $Id: signal.c,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $
*/

#include <linux/module.h>
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>

#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include "rtai_lxk.h"

#include "msg.h"
#include "registry.h"
#include "rtai_signal.h"

extern int (*rtai_signal_handler)(struct task_struct *lnxt, int sig);
extern void rt_task_force_ready(RT_TASK *task); 

#define ACCEPT (SEND|RPC|DELAYED|RECEIVE|RETURN|SEMAPHORE)

#define RTTASKSTACK  16
static RT_TASK *Stack[RTTASKSTACK]; /* the stack */
static int Sidx;                    /* next stack free spot */

static inline int push(RT_TASK *task)
{
    if(Sidx >= RTTASKSTACK) {
        return 0 ; // stack is full
    }
    Stack[Sidx++] = task;

    task->lnxtsk = 0;

    return 1 ;
}

static inline RT_TASK *pop(void)
{
    RT_TASK *task;

    if(!Sidx) return 0 ; // stack is empty
    else task = Stack[--Sidx];

    task->lnxtsk = current;

    return task;
}

void rt_task_force_ready(RT_TASK *task)
{
    unsigned long flags;

    flags = rt_global_save_flags_and_cli();
    task->errno  =  EINTR;
    task->state &= ~ACCEPT;
    rt_enq_ready_task(task);
    LXRT_SUSPEND(task);
    rt_global_restore_flags(flags);
}

static inline void do_force_ready(RT_TASK *task)
{
    RT_TASK *agent;

	struct { RT_TASK *task; } arg = { task };
	agent = (current->this_rt_task[0]) ? : pop();
	if( !agent ) return;
	lxk_resume(rt_task_force_ready, sizeof(arg)/sizeof(int), (int *)&arg, agent);
	if(!current->this_rt_task[0]) push(agent);
}

struct tq_struct_special {
	struct tq_struct tq_sig; // Must be first.
	RT_TASK	*task;
	};

static void try_force_ready(void *t)
{
	RT_TASK *task;

	task = ((struct tq_struct_special *)t)->task;

    if(task && task->magic==RT_TASK_MAGIC && task->linux_signal) {
        if( task->state & ACCEPT ) {
			do_force_ready(task);      
        } else {
			queue_task((struct tq_struct *) t, &tq_immediate);
			mark_bh(IMMEDIATE_BH);
			return;
		}
	}
	rt_free(t);
}

static int delay_force_ready(RT_TASK *task)
{
	struct tq_struct_special *t;

	t = rt_malloc(sizeof(struct tq_struct_special));
	if(!t) return -1;

	memset( t, 0, sizeof(struct tq_struct_special));
	t->tq_sig.routine = try_force_ready;
	t->tq_sig.data    = t;
	t->task           = task;

	queue_task((struct tq_struct *) t, &tq_immediate);
	mark_bh(IMMEDIATE_BH);
	return 0;
}

int rt_get_linux_signal(RT_TASK *task) // static inlined in rtai_sched.h ? 
{
	int sig;

    if(task && task->magic==RT_TASK_MAGIC) {
		sig = task->linux_signal;
		task->linux_signal = 0;
	}
	else sig = -1;

	return sig;
}

int rt_get_errno(RT_TASK *task) // static inlined in rtai_sched.h ?
{
	int err;

	if( task && task->magic==RT_TASK_MAGIC) {
		err = task->errno;
 	}
	else err = -EFAULT; // Negative 

	return err;
}

int rt_set_linux_signal_handler(RT_TASK *task, void (*handler)(int sig)) 
{
	if(task && task->magic==RT_TASK_MAGIC) {
		task->linux_signal_handler = handler;
		task->linux_signal = 0;
		return 0;
	} 
	else return -EFAULT;
} 	

static int linux_signal_handler(struct task_struct *lnx, int sig) // called from sys_kill() in kernel/signal.c
{
	RT_TASK *task;

	task = lnx->this_rt_task[0];
	if(task && task->magic==RT_TASK_MAGIC && task->linux_signal_handler) {
		task->linux_signal = sig;
		if( task->state & ACCEPT ) {
			do_force_ready(task);
		}
    	else {
			delay_force_ready(task);
		}
		return 0; // return 0 for Linux to skip further processing.
	}
	return 1;
}

void rt_init_linux_signal_handler(void)
{
	char name[8];
	int i;

	rtai_signal_handler = linux_signal_handler;
    for(i = 0; i < RTTASKSTACK; i++) {
		sprintf( name, "LA_%x", i);
		push(lxk_task_init(nam2num(name), 0, 0, 0));
    }
}

void rt_remove_linux_signal_handler(void)
{
	int i;
	RT_TASK *t;
	pid_t pid;

	rtai_signal_handler = 0;
	for(i = 0; i < RTTASKSTACK; i++) {
		t = pop();
		if(t) {
			pid = rttask2pid(t);
			rt_vc_release(pid);
			rt_task_delete(t);
		    rt_free(t->msg_buf[0]);
	    	rt_free(t);
			rt_drg_on_adr(t);
		}		
	}
}

int rt_lxrt_fork(struct pt_regs *regs, int is_a_clone)
{
	int pid;
	struct task_struct *lnx;

	if(!rt_is_linux() || rt_is_lxrt()) return -1; // Not allowed in HRT mode.

	if(!is_a_clone) {
	    pid = do_fork(SIGCHLD, regs->esp, regs, 0);
	} else {
	    unsigned long clone_flags;
	    unsigned long newsp;
/* NOTE: this case was not tested but should work if the call is setup exactly
   the way glibc does it in user space - taken verbatim out of sys_clone() */
	    clone_flags = regs->ebx;
    	newsp = regs->ecx; 
	    if(!newsp) newsp = regs->esp;
		pid = do_fork(clone_flags, newsp, regs, 0);
	}

	if(pid != -1) {
		lnx = find_task_by_pid(pid);
		if(lnx) memset(lnx->this_rt_task, 0, sizeof(current->this_rt_task));
		}
	return pid;
}
