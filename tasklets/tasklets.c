/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#include <linux/module.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
//#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/interrupt.h>

#include <asm/uaccess.h>
#include <asm/system.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>
//#include <asm/rtai_lxrt.h>
#include <rtai_tasklets.h>

MODULE_LICENSE("GPL");
//EXPORT_NO_SYMBOLS;

#if defined(CONFIG_RTAI_DYN_MM) || defined(CONFIG_RTAI_DYN_MM_MODULE)
#define sched_malloc(size)      rt_malloc((size))
#define sched_free(adr)         rt_free((adr))
#else
#define sched_malloc(size)      kmalloc((size), GFP_KERNEL)
#define sched_free(adr)         kfree((adr))
#endif

static struct rt_tasklet_struct timers_list =
{ &timers_list, &timers_list, RT_LOWEST_PRIORITY, 0, RT_TIME_END, 0LL, 0, 0, 0 };

static struct rt_tasklet_struct tasklets_list =
{ &tasklets_list, &tasklets_list, };

static spinlock_t tasklets_lock = SPIN_LOCK_UNLOCKED;
static spinlock_t timers_lock   = SPIN_LOCK_UNLOCKED;

int rt_insert_tasklet(struct rt_tasklet_struct *tasklet, int priority, void (*handler)(unsigned long), unsigned long data, unsigned long id, int pid)
{
	unsigned long flags;

// tasklet initialization
	if (!handler || !id) {
		return -EINVAL;
	}
	tasklet->uses_fpu = 0;
	tasklet->priority = priority;
	tasklet->handler  = handler;
	tasklet->data     = data;
	tasklet->id       = id;
	if (!pid) {
		tasklet->task = 0;
	} else {
		(tasklet->task)->priority = priority;
		copy_to_user(tasklet->usptasklet, tasklet, sizeof(struct rt_tasklet_struct));
	}
// tasklet insertion tasklets_list
	flags = rt_spin_lock_irqsave(&tasklets_lock);
	tasklet->next             = &tasklets_list;
	tasklet->prev             = tasklets_list.prev;
	(tasklets_list.prev)->next = tasklet;
	tasklets_list.prev         = tasklet;
	rt_spin_unlock_irqrestore(flags, &tasklets_lock);
	return 0;
}

void rt_remove_tasklet(struct rt_tasklet_struct *tasklet)
{
	if (tasklet->next != tasklet && tasklet->prev != tasklet) {
		unsigned long flags;
		flags = rt_spin_lock_irqsave(&tasklets_lock);
		(tasklet->next)->prev = tasklet->prev;
		(tasklet->prev)->next = tasklet->next;
		tasklet->next = tasklet->prev = tasklet;
		rt_spin_unlock_irqrestore(flags, &tasklets_lock);
	}
}

struct rt_tasklet_struct *rt_find_tasklet_by_id(unsigned long id)
{
	struct rt_tasklet_struct *tasklet;

	tasklet = &tasklets_list;
	while ((tasklet = tasklet->next) != &tasklets_list) {
		if (id == tasklet->id) {
			return tasklet;
		}
	}
	return 0;
}

int rt_exec_tasklet(struct rt_tasklet_struct *tasklet)
{
	if (tasklet && tasklet->next != tasklet && tasklet->prev != tasklet) {
		if (!tasklet->task) {
			tasklet->handler(tasklet->data);
		} else {
			rt_task_resume(tasklet->task);
		}
		return 0;
	}
	return -EINVAL;
}

void rt_set_tasklet_priority(struct rt_tasklet_struct *tasklet, int priority)
{
	tasklet->priority = priority;
	if (tasklet->task) {
		(tasklet->task)->priority = priority;
	}
}

int rt_set_tasklet_handler(struct rt_tasklet_struct *tasklet, void (*handler)(unsigned long))
{
	if (!handler) {
		return -EINVAL;
	}
	tasklet->handler = handler;
	if (tasklet->task) {
		copy_to_user(tasklet->usptasklet, tasklet, sizeof(struct rt_tasklet_struct));
	}
	return 0;
}

void rt_set_tasklet_data(struct rt_tasklet_struct *tasklet, unsigned long data)
{
	tasklet->data = data;
	if (tasklet->task) {
		copy_to_user(tasklet->usptasklet, tasklet, sizeof(struct rt_tasklet_struct));
	}
}

RT_TASK *rt_tasklet_use_fpu(struct rt_tasklet_struct *tasklet, int use_fpu)
{
	tasklet->uses_fpu = use_fpu ? 1 : 0;
	return tasklet->task;
}

static RT_TASK timers_manager;

static inline void asgn_min_prio(void)
{
// find minimum priority in timers_struct 
	struct rt_tasklet_struct *timer;
	unsigned long flags;
	int priority;

	priority = (timer = timers_list.next)->priority;
	flags = rt_spin_lock_irqsave(&timers_lock);
	while ((timer = timer->next) != &timers_list) {
		if (timer->priority < priority) {
			priority = timer->priority;
		}
	}
	rt_spin_unlock_irqrestore(flags, &timers_lock);
	flags = rt_global_save_flags_and_cli();
	if (timers_manager.priority > priority) {
		timers_manager.priority = priority;
		if (timers_manager.state == READY || timers_manager.state == (READY | RUNNING)) {
			rt_rem_ready_task(&timers_manager);
			rt_enq_ready_task(&timers_manager);
		}
	}
	rt_global_restore_flags(flags);
}

static inline void set_timer_firing_time(struct rt_tasklet_struct *timer, RTIME firing_time)
{
	if (timer->next != timer && timer->prev != timer) {
		unsigned long flags;
		struct rt_tasklet_struct *tmr;

		tmr = &timers_list;
		timer->firing_time = firing_time;
		flags = rt_spin_lock_irqsave(&timers_lock);
		(timer->next)->prev = timer->prev;
		(timer->prev)->next = timer->next;
		while (firing_time >= (tmr = tmr->next)->firing_time);
		timer->next     = tmr;
		timer->prev     = tmr->prev;
		(tmr->prev)->next = timer;
		tmr->prev         = timer;
		rt_spin_unlock_irqrestore(flags, &timers_lock);
	}
}

#define RT_SCHEDULE() \
	do { extern void (*dnepsus_trxl)(void); (*dnepsus_trxl)(); } while (0);

int rt_insert_timer(struct rt_tasklet_struct *timer, int priority, RTIME firing_time, RTIME period, void (*handler)(unsigned long), unsigned long data, int pid)
{
	unsigned long flags;
	struct rt_tasklet_struct *tmr;

// timer initialization
	if (!handler) {
		return -EINVAL;
	}
	timer->uses_fpu    = 0;
	timer->priority    = priority;
	timer->firing_time = firing_time;
	timer->period      = period;
	timer->handler     = handler;
	timer->data        = data;
	if (!pid) {
		timer->task = 0;
	} else {
		(timer->task)->priority = priority;
		copy_to_user(timer->usptasklet, timer, sizeof(struct rt_tasklet_struct));
	}
// timer insertion in timers_list
	tmr = &timers_list;
	flags = rt_spin_lock_irqsave(&timers_lock);
	while (firing_time >= (tmr = tmr->next)->firing_time);
	timer->next     = tmr;
	timer->prev     = tmr->prev;
	(tmr->prev)->next = timer;
	tmr->prev         = timer;
	rt_spin_unlock_irqrestore(flags, &timers_lock);
// timers_manager priority inheritance
	if (timer->priority < timers_manager.priority) {
		timers_manager.priority = timer->priority;
	}
// timers_task deadline inheritance
	flags = rt_global_save_flags_and_cli();
	if (timers_list.next == timer && (timers_manager.state & DELAYED) && firing_time < timers_manager.resume_time) {
		timers_manager.resume_time = firing_time;
		rt_rem_timed_task(&timers_manager);
		rt_enq_timed_task(&timers_manager);
		RT_SCHEDULE();
	}
	rt_global_restore_flags(flags);
	return 0;
}

void rt_remove_timer(struct rt_tasklet_struct *timer)
{
	if (timer->next != timer && timer->prev != timer) {
		unsigned long flags;
		flags = rt_spin_lock_irqsave(&timers_lock);
		(timer->next)->prev = timer->prev;
		(timer->prev)->next = timer->next;
		timer->next = timer->prev = timer;
		rt_spin_unlock_irqrestore(flags, &timers_lock);
		asgn_min_prio();
	}
}

void rt_set_timer_priority(struct rt_tasklet_struct *timer, int priority)
{
	timer->priority = priority;
	if (timer->task) {
		(timer->task)->priority = priority;
	}
	asgn_min_prio();
}

void rt_set_timer_firing_time(struct rt_tasklet_struct *timer, RTIME firing_time)
{
	unsigned long flags;

	set_timer_firing_time(timer, firing_time);
	flags = rt_global_save_flags_and_cli();
	if (timers_list.next == timer && (timers_manager.state & DELAYED) && firing_time < timers_manager.resume_time) {
		timers_manager.resume_time = firing_time;
		rt_rem_timed_task(&timers_manager);
		rt_enq_timed_task(&timers_manager);
		RT_SCHEDULE();
	}
	rt_global_restore_flags(flags);
}

void rt_set_timer_period(struct rt_tasklet_struct *timer, RTIME period)
{
	unsigned long flags;
	flags = rt_spin_lock_irqsave(&timers_lock);
	timer->period = period;
	rt_spin_unlock_irqrestore(flags, &timers_lock);
}

// the timers_manager task function

static void rt_timers_manager(int dummy)
{
	static unsigned long cr0;

	RTIME now;
	struct rt_tasklet_struct *tmr, *timer;
	unsigned long flags;
	int priority, used_fpu;

	while (1) {
		rt_sleep_until((timers_list.next)->firing_time);
		now = timers_manager.resume_time + tuned.timers_tol[0];
// find all the timers to be fired, in priority order
		while (1) {
			used_fpu = 0;
			tmr = timer = &timers_list;
			priority = RT_LOWEST_PRIORITY;
			flags = rt_spin_lock_irqsave(&timers_lock);
			while ((tmr = tmr->next)->firing_time <= now) {
				if (tmr->priority < priority) {
					priority = (timer = tmr)->priority;
				}
			}
			timers_manager.priority = priority;
			rt_spin_unlock_irqrestore(flags, &timers_lock);
			if (timer == &timers_list) {
				break;
			}
			if (!timer->period) {
				flags = rt_spin_lock_irqsave(&timers_lock);
				(timer->next)->prev = timer->prev;
				(timer->prev)->next = timer->next;
				timer->next = timer->prev = timer;
				rt_spin_unlock_irqrestore(flags, &timers_lock);
			} else {
				set_timer_firing_time(timer, timer->firing_time + timer->period);
			}
			if (!timer->task) {
				if (!used_fpu && timer->uses_fpu) {
					used_fpu = 1;
					save_cr0_and_clts(cr0);
					save_fpenv(timers_manager.fpu_reg);
				}
				timer->handler(timer->data);
			} else {
				rt_task_resume(timer->task);
			}
		}
		if (used_fpu) {
			restore_fpenv(timers_manager.fpu_reg);
			restore_cr0(cr0);
		}
// set next timers_manager priority according to the highest priority timer
		asgn_min_prio();
// if no more timers in timers_struct remove timers_manager from tasks list
	}
}

struct rt_tasklet_struct *rt_init_tasklet(void)
{
	struct rt_tasklet_struct *tasklet;
	tasklet = sched_malloc(sizeof(struct rt_tasklet_struct));
	memset(tasklet, 0, sizeof(struct rt_tasklet_struct));
	return tasklet;
}

void rt_register_task(struct rt_tasklet_struct *tasklet, struct rt_tasklet_struct *usptasklet, RT_TASK *task)
{
	tasklet->task = task;
	tasklet->usptasklet = usptasklet;
	copy_to_user(usptasklet, tasklet, sizeof(struct rt_tasklet_struct));
}

void rt_wait_tasklet_is_hard(struct rt_tasklet_struct *tasklet, int thread)
{
	tasklet->thread = thread;
	while (!tasklet->task || !((tasklet->task)->state & SUSPENDED)) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(2);
	}
}

int rt_delete_tasklet(struct rt_tasklet_struct *tasklet)
{
	int pid, thread;

	rt_remove_tasklet(tasklet);
	tasklet->handler = 0;
	pid = ((tasklet->task)->lnxtsk)->pid;
	copy_to_user(tasklet->usptasklet, tasklet, sizeof(struct rt_tasklet_struct));
	rt_task_resume(tasklet->task);
	while (find_task_by_pid(pid)) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(2);
	}
	thread = tasklet->thread;	
	sched_free(tasklet);
	return thread;	
}

static int StackSize = STACK_SIZE;
MODULE_PARM(StackSize, "i");

static RT_TASK *rt_base_linux_task;

int init_module(void)
{
	RT_TASK *rt_linux_tasks[NR_RT_CPUS];
	rt_base_linux_task = rt_get_base_linux_task(rt_linux_tasks);
	if (rt_sched_type() == MUP_SCHED) {
		tuned.timers_tol[0] = tuned.timers_tol[timers_manager.runnable_on_cpus];
	}
        if(rt_base_linux_task->task_trap_handler[0]) {
                if(((int (*)(void *, int))rt_base_linux_task->task_trap_handler[0])(rt_tasklet_fun, TSKIDX)) {
                        printk("Recompile your module with a different index\n");
                        return -EACCES;
                }
        }
	rt_task_init(&timers_manager, rt_timers_manager, 0, StackSize, RT_LOWEST_PRIORITY, 0, 0);
	rt_task_resume(&timers_manager);
	return 0;
}

void cleanup_module(void)
{
	rt_task_delete(&timers_manager);
        if(rt_base_linux_task->task_trap_handler[1]) {
                ((int (*)(void *, int))rt_base_linux_task->task_trap_handler[1])(rt_tasklet_fun, TSKIDX);
        }
}
