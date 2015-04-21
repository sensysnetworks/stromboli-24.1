/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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

#include <rtai.h>
#include <rtai_sched.h>

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

#define ONE_SHOT

//#define FORCE_CPU 0    // meaningfull just for the MPschedulers

//#define FORCE_TIMER 0  // meaningfull just for the SMPscheduler

#define TICK_PERIOD 2000000

#define STACK_SIZE 2000

#define EXECTIME 400000000LL

#define RR_QUANTUM  0

#define NTASKS 5

static SEM sync;

static RT_TASK thread[NTASKS];

static int nsw[NTASKS];

static void fun(int indx)
{
	RTIME startime;
	rtai_print_to_screen("RESUMED TASK #%d (%p) ON CPU %d.\n", indx, &thread[indx], hard_cpu_id());
	rt_sem_wait(&sync);
	rtai_print_to_screen("TASK #%d (%p) STARTS WORKING RR ON CPU %d.\n", indx, &thread[indx], hard_cpu_id());
	startime = rt_get_cpu_time_ns();
	while(rt_get_cpu_time_ns() < (startime + EXECTIME));
	thread[indx].signal = 0;
	rtai_print_to_screen("TASK #%d (%p) SUICIDES.\n", indx, &thread[indx]);
}

static void signal(void)
{
	RT_TASK *task;
	int i;
	for (i = 0; i < NTASKS; i++) {
		if ((task = rt_whoami()) == &thread[i]) {
			nsw[i]++;
			rtai_print_to_screen("TASK #%d (%p) ON CPU %d.\n", i, task, hard_cpu_id());
			break;
		}
	}
}

int init_module(void)
{
	int i;

	printk("INSMOD ON CPU %d.\n", hard_cpu_id());
	rt_sem_init(&sync, 0);
#ifdef ONE_SHOT
	rt_set_oneshot_mode();
#endif
#ifdef FORCE_TIMER
	start_rt_timer_cpuid(nano2count(TICK_PERIOD), 0);
#else
	start_rt_timer(nano2count(TICK_PERIOD));
#endif
	for (i = 0; i < NTASKS; i++) {
#ifdef FORCE_CPU
		rt_task_init_cpuid(&thread[i], fun, i, STACK_SIZE, 0, 0, signal, FORCE_CPU);
#else
		rt_task_init(&thread[i], fun, i, STACK_SIZE, 0, 0, signal);
#endif
		rt_set_sched_policy(&thread[i], RT_SCHED_RR, RR_QUANTUM);
	}
	for (i = 0; i < NTASKS; i++) {
		rt_task_resume(&thread[i]);
// to be sure they are all wait synchronized
		while(!(rt_get_task_state(&thread[i]) & SEMAPHORE));
	}
	rt_sem_broadcast(&sync);
	return 0;
}


void cleanup_module(void)
{
	int i;

	stop_rt_timer();
	rt_sem_delete(&sync);
	for (i = 0; i < NTASKS; i++) {
		printk("# %d -> %d\n", i, nsw[i]);
		rt_task_delete(&thread[i]);
	}
}
