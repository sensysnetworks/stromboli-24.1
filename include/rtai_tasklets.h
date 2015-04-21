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

#ifndef _RTAI_TMR_H_
#define _RTAI_TMR_H_

#define TSKIDX  1

#define INIT	 	 0
#define DELETE		 1
#define TASK_INSERT 	 2
#define TASK_REMOVE	 3
#define USE_FPU	 	 4
#define TIMER_INSERT 	 5
#define TIMER_REMOVE	 6
#define SET_TMR_PRI	 7
#define SET_FIR_TIM	 8	
#define SET_PER	 	 9
#define SET_HDL		10
#define SET_DAT	 	11
#define EXEC_TASKLET    12
#define WAIT_IS_HARD	13
#define SET_TSK_PRI	14
#define REG_TASK   	15

struct rt_tasklet_struct
{
	struct rt_tasklet_struct *next, *prev;
	int priority, uses_fpu;
	RTIME firing_time, period;
	void (*handler)(unsigned long);
	unsigned long data, id;
	int thread;
	RT_TASK *task;
	struct rt_tasklet_struct *usptasklet;
};

#ifdef __KERNEL__

#include <rtai_sched.h>

#define STACK_SIZE 8196

extern struct rt_tasklet_struct *rt_init_tasklet(void);

extern int rt_delete_tasklet(struct rt_tasklet_struct *tasklet);

extern int rt_insert_tasklet(struct rt_tasklet_struct *tasklet, int priority, void (*handler)(unsigned long), unsigned long data, unsigned long id, int pid);

extern void rt_remove_tasklet(struct rt_tasklet_struct *tasklet);

extern struct rt_tasklet_struct *rt_find_tasklet_by_id(unsigned long id);

extern int rt_exec_tasklet(struct rt_tasklet_struct *tasklet);

extern void rt_set_tasklet_priority(struct rt_tasklet_struct *tasklet, int priority);

// if you are sure of the handler address this function can be substituted with 
// a direct assignement, see macro rt_fast_set_tasklet_handler below
extern int rt_set_tasklet_handler(struct rt_tasklet_struct *tasklet, void (*handler)(unsigned long));

#define rt_fast_set_tasklet_handler(t, h) do { (t)->handler = (h); } while (0)

// this function can be substituted with a direct assignement, see macro 
// rt_fast_set_tasklet_data below
extern void rt_set_tasklet_data(struct rt_tasklet_struct *tasklet, unsigned long data);

#define rt_fast_set_tasklet_data(t, d) do { (t)->data = (d); } while (0)

extern RT_TASK *rt_tasklet_use_fpu(struct rt_tasklet_struct *tasklet, int use_fpu);

#define rt_init_timer rt_init_tasklet 

#define rt_delete_timer rt_delete_tasklet

extern int rt_insert_timer(struct rt_tasklet_struct *timer, int priority, RTIME firing_time, RTIME period, void (*handler)(unsigned long), unsigned long data, int pid);

extern void rt_remove_timer(struct rt_tasklet_struct *timer);

// this function cannot be substituted with a direct assignement as it sets the
// priority of the timers_manager task
extern void rt_set_timer_priority(struct rt_tasklet_struct *timer, int priority);

// this function cannot be substituted with a direct assignement as it sets the
// firing time of the timers_manager task
extern void rt_set_timer_firing_time(struct rt_tasklet_struct *timer, RTIME firing_time);

// this function can be substituted with a direct assignement if period can stay
// in 32 bits, use macro rt_fast_set_period below
extern void rt_set_timer_period(struct rt_tasklet_struct *timer, RTIME period);

#define rt_fast_set_timer_period(t, p) do { (t)->period = (p); } while (0)

// if you are sure of the handler address this macro can be substituted with 
// a direct assignement, see macro rt_fast_set_timer_handler below
#define rt_set_timer_handler rt_set_tasklet_handler

#define rt_fast_set_timer_handler(t, h) do { (t)->handler = (h); } while (0)

// this macro can be substituted with a direct assignement, see macro 
// rt_fast_set_timer_data below
#define rt_set_timer_data rt_set_tasklet_data

#define rt_fast_set_timer_data(t, d) do { (t)->data = (d); } while (0)

#define rt_timer_use_fpu rt_tasklet_use_fpu

extern void rt_wait_tasklet_is_hard(struct rt_tasklet_struct *tasklet, int thread);

extern void rt_register_task(struct rt_tasklet_struct *tasklet, struct rt_tasklet_struct *usptasklet, RT_TASK *task);
 
static struct rt_fun_entry rt_tasklet_fun[]  __attribute__ ((__unused__));

static struct rt_fun_entry rt_tasklet_fun[] = {
	{ 0, rt_init_tasklet },    		//   0
	{ 0, rt_delete_tasklet },    		//   1
	{ 0, rt_insert_tasklet },    		//   2
	{ 0, rt_remove_tasklet },    		//   3
	{ 0, rt_tasklet_use_fpu },   		//   4
	{ 0, rt_insert_timer },    		//   5
	{ 0, rt_remove_timer },    		//   6
	{ 0, rt_set_timer_priority },  		//   7
	{ 0, rt_set_timer_firing_time },   	//   8
	{ 0, rt_set_timer_period },   		//   9
	{ 0, rt_set_tasklet_handler },  	//  10
	{ 0, rt_set_tasklet_data },   		//  11
	{ 0, rt_exec_tasklet },   		//  12
	{ 0, rt_wait_tasklet_is_hard },	   	//  13
	{ 0, rt_set_tasklet_priority },  	//  14
	{ 0, rt_register_task },	  	//  15
};

#else

#define KEEP_STATIC_INLINE
#include <stdarg.h>
#include <pthread.h>
#include <asm/rtai_lxrt.h>
#include <sys/mman.h>
#include <rtai_lxrt.h>

extern union rtai_lxrt_t rtai_lxrt(short int dynx, short int lsize, int srq, void *arg);

#define SIZARG sizeof(arg)
#ifndef __SUPPORT_TASKLET__
#define __SUPPORT_TASKLET__
static void *support_tasklet(void *arg)
{
	RT_TASK *task;
	struct rt_tasklet_struct *tasklet, usptasklet;
	struct { void *tasklet; void *handler; } upd;

	upd.tasklet = tasklet = ((struct rt_tasklet_struct **)arg)[0];
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if (!(task = rt_task_init_schmod((unsigned long)tasklet, 98, 0, 0, SCHED_FIFO, 0xF))) {
		printf("CANNOT INIT SUPPORT TASKLET\n");
		return (void *)1;
	}

	{
		struct { struct rt_tasklet_struct *tasklet, *usptasklet; RT_TASK *task; } arg = { tasklet, &usptasklet, task };
		rtai_lxrt(TSKIDX, SIZARG, REG_TASK, &arg);
	}

	mlockall(MCL_CURRENT | MCL_FUTURE);

	rt_make_hard_real_time();
	while (1) {
		rt_task_suspend(task);
		if ((upd.handler = (void*)usptasklet.handler)) {
			rtai_lxrt(TSKIDX, SIZARG, SET_HDL, &upd);
			usptasklet.handler(usptasklet.data);
		} else {
			break;
		}
	}
	rt_make_soft_real_time();

	rt_task_delete(task);
	return (void *)0;
}
#endif

static inline struct rt_tasklet_struct *rt_init_tasklet(void)
{
	pthread_t thread;
	struct rt_tasklet_struct *tasklet;

	{
		struct { int dummy; } arg = { 0 };
		tasklet = (struct rt_tasklet_struct*)rtai_lxrt(TSKIDX, SIZARG, INIT, &arg).v[LOW];
	}

	pthread_create(&thread, NULL, support_tasklet, &tasklet);

	{
		struct { struct rt_tasklet_struct *tasklet; pthread_t thread; } arg = { tasklet, thread };
		rtai_lxrt(TSKIDX, SIZARG, WAIT_IS_HARD, &arg);
	}

	return tasklet;
}

#define rt_init_timer rt_init_tasklet

static inline void rt_delete_tasklet(struct rt_tasklet_struct *tasklet)
{
	pthread_t thread;
	struct { struct rt_tasklet_struct *tasklet; } arg = { tasklet };
	if ((thread = (pthread_t)rtai_lxrt(TSKIDX, SIZARG, DELETE, &arg).i[LOW])) {
		pthread_join(thread, NULL);
	}
}

#define rt_delete_timer rt_delete_tasklet

static inline int rt_insert_timer( struct rt_tasklet_struct *timer, int priority, RTIME firing_time, RTIME period, void (*handler)(unsigned long), unsigned long data, int pid)
{
	struct { struct rt_tasklet_struct *timer; int priority; RTIME firing_time; RTIME period; void (*handler)(unsigned long); unsigned long data; int pid; } arg = { timer, priority, firing_time, period, handler, data, pid };
	return rtai_lxrt(TSKIDX, SIZARG, TIMER_INSERT, &arg).i[LOW];
}

static inline void rt_remove_timer(struct rt_tasklet_struct *timer)
{
	struct { struct rt_tasklet_struct *timer; } arg = { timer };
	rtai_lxrt(TSKIDX, SIZARG, TIMER_REMOVE, &arg);
}

static inline void rt_set_timer_priority(struct rt_tasklet_struct *timer, int priority)
{
	struct { struct rt_tasklet_struct *timer; int priority; } arg = { timer, priority };
	rtai_lxrt(TSKIDX, SIZARG, SET_TMR_PRI, &arg);
}

static inline void rt_set_timer_firing_time(struct rt_tasklet_struct *timer, RTIME firing_time)
{
	struct { struct rt_tasklet_struct *timer; RTIME firing_time; } arg = { timer, firing_time };
	rtai_lxrt(TSKIDX, SIZARG, SET_FIR_TIM, &arg);
}

static inline void rt_set_timer_period(struct rt_tasklet_struct *timer, RTIME period)
{
	struct { struct rt_tasklet_struct *timer; RTIME period; } arg = { timer, period };
	rtai_lxrt(TSKIDX, SIZARG, SET_PER, &arg);
}

static inline int rt_set_tasklet_handler(struct rt_tasklet_struct *tasklet, void (*handler)(unsigned long))
{
	struct { struct rt_tasklet_struct *tasklet; void (*handler)(unsigned long); } arg = { tasklet, handler };
	return rtai_lxrt(TSKIDX, SIZARG, SET_HDL, &arg).i[LOW];
}

#define rt_set_timer_handler rt_set_tasklet_handler

static inline void rt_set_tasklet_data(struct rt_tasklet_struct *tasklet, unsigned long data)
{
	struct { struct rt_tasklet_struct *tasklet; unsigned long data; } arg = { tasklet, data };
	rtai_lxrt(TSKIDX, SIZARG, SET_DAT, &arg);
}

#define rt_set_timer_data rt_set_tasklet_data

static inline RT_TASK *rt_tasklet_use_fpu(struct rt_tasklet_struct *tasklet, int use_fpu)
{
	RT_TASK *task;
	struct { struct rt_tasklet_struct *tasklet; int use_fpu; } arg = { tasklet, use_fpu };
	if ((task = (RT_TASK*)rtai_lxrt(TSKIDX, SIZARG, USE_FPU, &arg).v[LOW])) {
		rt_task_use_fpu(task, use_fpu);
	}
	return task;
}

#define rt_timer_use_fpu rt_tasklet_use_fpu

static inline int rt_insert_tasklet(struct rt_tasklet_struct *tasklet, int priority, void (*handler)(unsigned long), unsigned long data, unsigned long id, int pid)
{
	struct { struct rt_tasklet_struct *tasklet; int priority; void (*handler)(unsigned long); unsigned long data; unsigned long id; int pid; } arg = { tasklet, priority, handler, data, id, pid };
	return rtai_lxrt(TSKIDX, SIZARG, TASK_INSERT, &arg).i[LOW];
}

static inline void rt_set_tasklet_priority(struct rt_tasklet_struct *tasklet, int priority)
{
	struct { struct rt_tasklet_struct *tasklet; int priority; } arg = { tasklet, priority };
	rtai_lxrt(TSKIDX, SIZARG, SET_TSK_PRI, &arg);
}

static inline void rt_remove_tasklet(struct rt_tasklet_struct *tasklet)
{
	struct { struct rt_tasklet_struct *tasklet; } arg = { tasklet };
	rtai_lxrt(TSKIDX, SIZARG, TASK_REMOVE, &arg);
}

static inline int rt_exec_tasklet(struct rt_tasklet_struct *tasklet)
{
	struct { struct rt_tasklet_struct *tasklet; } arg = { tasklet };
	return rtai_lxrt(TSKIDX, SIZARG, EXEC_TASKLET, &arg).i[LOW];
}

#endif

#endif
