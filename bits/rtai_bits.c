/*
COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it),

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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/config.h>
#include <linux/version.h>

#include <rtai_sched.h>
#include <rtai_bits.h>

MODULE_LICENSE("GPL");

//#define RT_BITS_MAGIC 0x43431758  // nam2num("BITSMG")
#define RT_BITS_MAGIC 0xaabcdeff  // same as SEM_MAGIC to ease it in user space

#define NOTHING ((void *)0)

static void (*rt_schedule)(void);
static void (*rt_schedule_map)(unsigned long schedmap);

static inline void enqueue_blocked(RT_TASK *task, QUEUE *q)
{
        task->blocked_on = q;
        q->prev = (task->queue.prev = q->prev)->next = &(task->queue);
        task->queue.next = q;
}

static inline void dequeue_blocked(RT_TASK *task)
{
        task->prio_passed_to     = NOTHING;
        (task->queue.prev)->next = task->queue.next;
        (task->queue.next)->prev = task->queue.prev;
        task->blocked_on         = NOTHING;
}

#define MASK0(x) ((unsigned long *)&(x))[0]
#define MASK1(x) ((unsigned long *)&(x))[1]

static int all_set(BITS *bits, unsigned long mask)
{
	return (bits->mask & mask) == mask;
}

static int any_set(BITS *bits, unsigned long mask)
{
	return (bits->mask & mask);
}

static int all_clr(BITS *bits, unsigned long mask)
{
	return (~bits->mask & mask) == mask;
}

static int any_clr(BITS *bits, unsigned long mask)
{
	return (~bits->mask & mask);
}

static int all_set_and_any_set(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK1(masks)) && (bits->mask & MASK0(masks)) == MASK0(masks);
}

static int all_set_and_all_clr(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK0(masks)) == MASK0(masks) && (~bits->mask & MASK1(masks)) == MASK1(masks);
}

static int all_set_and_any_clr(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK0(masks)) == MASK0(masks) && (~bits->mask & MASK1(masks));
}

static int any_set_and_all_clr(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK0(masks)) && (~bits->mask & MASK1(masks)) == MASK1(masks);
}

static int any_set_and_any_clr(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK0(masks)) && (~bits->mask & MASK1(masks));
}

static int all_clr_and_any_clr(BITS *bits, unsigned long masks)
{
	return (~bits->mask & MASK1(masks)) && (~bits->mask & MASK0(masks)) == MASK0(masks);
}

static int all_set_or_any_set(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK1(masks)) || (bits->mask & MASK0(masks)) == MASK0(masks);
}

static int all_set_or_all_clr(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK0(masks)) == MASK0(masks) || (~bits->mask & MASK1(masks)) == MASK1(masks);
}

static int all_set_or_any_clr(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK0(masks)) == MASK0(masks) || (~bits->mask & MASK1(masks));
}

static int any_set_or_all_clr(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK0(masks)) || (~bits->mask & MASK1(masks)) == MASK1(masks);
}

static int any_set_or_any_clr(BITS *bits, unsigned long masks)
{
	return (bits->mask & MASK0(masks)) || (~bits->mask & MASK1(masks));
}

static int all_clr_or_any_clr(BITS *bits, unsigned long masks)
{
	return (~bits->mask & MASK1(masks)) || (~bits->mask & MASK0(masks)) == MASK0(masks);
}

static void set_bits(BITS *bits, unsigned long mask)
{
	bits->mask |= mask;
}

static void clr_bits(BITS *bits, unsigned long mask)
{
	bits->mask &= ~mask;
}

static void set_clr_bits(BITS *bits, unsigned long masks)
{
	bits->mask =  (bits->mask | MASK0(masks)) & ~MASK1(masks);
}

static void nop_fun(BITS *bits, unsigned long mask)
{
}

static int (*test_fun[])(BITS *, unsigned long) = {
	all_set, any_set,             all_clr,             any_clr, 
	         all_set_and_any_set, all_set_and_all_clr, all_set_and_any_clr,
	                              any_set_and_all_clr, any_set_and_any_clr,
	                                                   all_clr_and_any_clr,
	         all_set_or_any_set,  all_set_or_all_clr,  all_set_or_any_clr,
	                              any_set_or_all_clr,  any_set_or_any_clr,
	                                                   all_clr_or_any_clr
};

static void (*exec_fun[])(BITS *, unsigned long) = {
	set_bits, clr_bits,
	          set_clr_bits,
	nop_fun
};

void rt_bits_init(BITS *bits, unsigned long mask)
{
	bits->magic      = RT_BITS_MAGIC;
	bits->queue.prev = &(bits->queue);
	bits->queue.next = &(bits->queue);
	bits->queue.task = 0;
	bits->mask       = mask;
}

int rt_bits_delete(BITS *bits)
{
	unsigned long flags, schedmap;
	RT_TASK *task;
	QUEUE *q;

	if (bits->magic != RT_BITS_MAGIC) {
		return BITS_ERR;
	}

	schedmap = 0;
	q = &bits->queue;
	flags = rt_global_save_flags_and_cli();
	bits->magic = 0;
	while ((q = q->next) != &bits->queue && (task = q->task)) {
		rt_rem_timed_task(task);
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
			rt_enq_ready_task(task);	
			set_bit(task->runnable_on_cpus, &schedmap);
		}
	}
	if (schedmap) {
		rt_schedule_map(schedmap);
	}
	rt_global_restore_flags(flags);
	return 0;
}

#define TEST_FUN(x)   ((int *)&(x)->retval)[0]
#define TEST_MASK(x)  ((unsigned long *)&(x)->retval)[1]

unsigned long rt_get_bits(BITS *bits)
{
	return bits->mask;
}

int rt_bits_reset(BITS *bits, unsigned long mask)
{
	unsigned long flags, schedmap, oldmask;
	RT_TASK *task;
	QUEUE *q;

	if (bits->magic != RT_BITS_MAGIC) {
		return BITS_ERR;
	}

	schedmap = 0;
	q = &bits->queue;
	flags = rt_global_save_flags_and_cli();
	oldmask = bits->mask;
	bits->mask = mask;
	while ((q = q->next) != &bits->queue) {
		dequeue_blocked(task = q->task);
		rt_rem_timed_task(task);
		if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
			rt_enq_ready_task(task);
			set_bit(task->runnable_on_cpus, &schedmap);
		}
	}
	bits->queue.prev = bits->queue.next = &bits->queue;
	if (schedmap) {
		rt_schedule_map(schedmap);
	}
	rt_global_restore_flags(flags);
	return oldmask;
}

unsigned long rt_bits_signal(BITS *bits, int setfun, unsigned long masks)
{
	unsigned long flags, schedmap;
	RT_TASK *task;
	QUEUE *q;

	if (bits->magic != RT_BITS_MAGIC) {
		return BITS_ERR;
	}

	schedmap = 0;
	q = &bits->queue;
	flags = rt_global_save_flags_and_cli();
	exec_fun[setfun](bits, masks);
	masks = bits->mask;
	while ((q = q->next) != &bits->queue) {
		task = q->task;
		if (test_fun[TEST_FUN(task)](bits, TEST_MASK(task))) {
			dequeue_blocked(task);
			rt_rem_timed_task(task);
			if ((task->state & ~(READY | RUNNING)) && (task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
				rt_enq_ready_task(task);
				set_bit(task->runnable_on_cpus, &schedmap);
			}
		}
	}
	if (schedmap) {
		rt_schedule_map(schedmap);
	}
	rt_global_restore_flags(flags);
	return masks;
}

int rt_bits_wait(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, unsigned long *resulting_mask)
{
	RT_TASK *rt_current;
	unsigned long flags;

	if (bits->magic != RT_BITS_MAGIC) {
		return BITS_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if (!test_fun[testfun](bits, testmasks)) {
		rt_current = rt_whoami();
		TEST_FUN(rt_current)  = testfun;
		TEST_MASK(rt_current) = testmasks;
		rt_current->state |= SEMAPHORE;
		rt_rem_ready_current(rt_current);
		enqueue_blocked(rt_current, &bits->queue);
		rt_schedule();
		if (resulting_mask) {
			*resulting_mask = bits->mask;
		}
		if (rt_current->blocked_on || bits->magic != RT_BITS_MAGIC) {
			rt_current->prio_passed_to = NOTHING;
			rt_global_restore_flags(flags);
			return BITS_ERR;
		}
	} else if (resulting_mask) {
		*resulting_mask = bits->mask;
	}
	exec_fun[exitfun](bits, exitmasks);
	rt_global_restore_flags(flags);
	return 0;
}

int rt_bits_wait_if(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, unsigned long *resulting_mask)
{
	unsigned long flags;

	if (bits->magic != RT_BITS_MAGIC) {
		return BITS_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if (resulting_mask) {
		*resulting_mask = bits->mask;
	}
	if (test_fun[testfun](bits, testmasks)) {
		exec_fun[exitfun](bits, exitmasks);
		rt_global_restore_flags(flags);
		return 1;
	} 
	rt_global_restore_flags(flags);
	return 0;
}

int rt_bits_wait_until(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, RTIME time, unsigned long *resulting_mask)
{
	RT_TASK *rt_current;
	unsigned long flags;

	if (bits->magic != RT_BITS_MAGIC) {
		return BITS_ERR;
	}

	flags = rt_global_save_flags_and_cli();
	if (!test_fun[testfun](bits, testmasks)) {
		rt_current = rt_whoami();
		TEST_FUN(rt_current)  = testfun;
		TEST_MASK(rt_current) = testmasks;
		rt_current->blocked_on = &bits->queue;
		if ((rt_current->resume_time = time) > rt_get_time()) {
			rt_current->state |= (SEMAPHORE | DELAYED);
			rt_rem_ready_current(rt_current);
			enqueue_blocked(rt_current, &bits->queue);
			rt_enq_timed_task(rt_current);
			rt_schedule();
		} else {
			rt_current->queue.prev = rt_current->queue.next = &rt_current->queue;
		}
		if (resulting_mask) {
			*resulting_mask = bits->mask;
		}
		if (bits->magic != RT_BITS_MAGIC) {
			rt_current->prio_passed_to = NOTHING;
			rt_global_restore_flags(flags);
			return BITS_ERR;
		} else if (rt_current->blocked_on) {
			dequeue_blocked(rt_current);
			rt_global_restore_flags(flags);
			return BITS_TIMOUT;
		}
	} else if (resulting_mask) {
		*resulting_mask = bits->mask;
	}
	exec_fun[exitfun](bits, exitmasks);
	rt_global_restore_flags(flags);
	return 0;
}

int rt_bits_wait_timed(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, RTIME delay, unsigned long *resulting_mask)
{
	return rt_bits_wait_until(bits, testfun, testmasks, exitfun, exitmasks, rt_get_time() + delay, resulting_mask);
}

extern void (*dnepsus_trxl)(void);

#ifdef CONFIG_SMP

#define SCHED_VECTOR  RTAI_3_VECTOR

static void RT_SCHEDULE_MAP(unsigned long schedmap)
{
 	int local;	
	local = test_and_clear_bit(hard_cpu_id(), &schedmap);
	if (schedmap) {
#ifdef CONFIG_RTAI_ADEOS
	        unsigned long flags;
		arti_hw_lock(flags);
#endif /* CONFIG_RTAI_ADEOS */
		apic_wait_icr_idle();
		apic_write_around(APIC_ICR2, SET_APIC_DEST_FIELD(schedmap));
		apic_write_around(APIC_ICR, APIC_DEST_LOGICAL | SCHED_VECTOR);
#ifdef CONFIG_RTAI_ADEOS
		arti_hw_unlock(flags);
#endif /* CONFIG_RTAI_ADEOS */
	}
	if (local) {
		dnepsus_trxl();
	}
}

#else

#define RT_SCHEDULE_MAP (void *)nop_fun

#endif

static RT_TASK *rt_base_linux_task;

int init_module(void)
{
	RT_TASK *rt_linux_tasks[NR_RT_CPUS];
	rt_base_linux_task = rt_get_base_linux_task(rt_linux_tasks);
	if(rt_base_linux_task->task_trap_handler[0]) {
		if(((int (*)(void *, int))rt_base_linux_task->task_trap_handler[0])(rt_bits_fun, BITSIDX)) {
			printk("Recompile your module with a different index\n");
			return -EACCES;
		}
	}
	rt_schedule = dnepsus_trxl;
	rt_schedule_map = rt_sched_type() == MUP_SCHED ? RT_SCHEDULE_MAP : (void *)nop_fun;
	return 0;
}

void cleanup_module(void)
{
	if(rt_base_linux_task->task_trap_handler[1]) {
		((int (*)(void *, int))rt_base_linux_task->task_trap_handler[1])(rt_bits_fun, BITSIDX);
	}
	return;
}

/*
EXPORT_SYMBOL(rt_bits_init);
EXPORT_SYMBOL(rt_bits_delete);
EXPORT_SYMBOL(rt_get_bits);
EXPORT_SYMBOL(rt_bits_reset);
EXPORT_SYMBOL(rt_bits_signal);
EXPORT_SYMBOL(rt_bits_wait);
EXPORT_SYMBOL(rt_bits_wait_if);
EXPORT_SYMBOL(rt_bits_wait_until);
EXPORT_SYMBOL(rt_bits_wait_timed);
*/
