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
#include <rtai_leds.h>

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

#define MAX_JITTER  30000
#define TICK_PERIOD (MAX_JITTER + 20000)

#define TIMER_TO_CPU 0 // < 0 || > 1 to maintain a symmetric processed timer.
#define RUNNABLE_ON_CPUS 2     // 1: on cpu 0 only, 2: on cpu 1 only, 3: on any.
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)

#define STACK_SIZE 2000
#define LOOPS 1000000
#define NTASKS 8

static RT_TASK thread[NTASKS];

static int cpu_used[NR_RT_CPUS];

static int bit;

static void fun(int t)
{
	unsigned int loops = LOOPS;
	while(loops--) {
		cpu_used[hard_cpu_id()]++;
		bit = t;
		rt_task_wait_period();
	}
}

static int max_jitter;

static void signal(void)
{
	while(rdtsc() < (rt_whoami()->resume_time + max_jitter));
	rt_leds_set_mask(1,bit);
}

int init_module(void)
{
	RTIME tick_period;
	RTIME now;
	int i, k;

	rt_set_oneshot_mode();
	k = 1;
	for (i = 0; i < NTASKS; i++) {
		rt_task_init(&thread[i], fun, k = ~k, STACK_SIZE, 0, 0, signal);
		rt_set_runnable_on_cpus(&thread[i], RUN_ON_CPUS); 
	}
	tick_period = start_rt_timer(nano2count(TICK_PERIOD));
	max_jitter = nano2count(MAX_JITTER);
	now = rt_get_time() + NTASKS*tick_period;
	for (i = 0; i < NTASKS; i++) {
		rt_task_make_periodic(&thread[i], now + (i+1)*tick_period - max_jitter, NTASKS*tick_period);
	}
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, TIMER_TO_CPU);
	return 0;
}


void cleanup_module(void)
{
	int i, cpuid;

	rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
	stop_rt_timer();
	rt_busy_sleep(10000000);
	for (i = 0; i < NTASKS; i++) {
		rt_task_delete(&thread[i]);
	}
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
}
