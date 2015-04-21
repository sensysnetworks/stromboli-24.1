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


/* Produce a rectangular wave on output 0 of a parallel port */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_leds.h>

MODULE_DESCRIPTION("Runs various realtime tasks periodically.");
MODULE_AUTHOR("Paolo Mantegazza <mantegazza@aero.polimi.it>");
MODULE_LICENSE("GPL");

/*
 * Command line parameters
 */
int period = 100000;
MODULE_PARM(period, "i");
MODULE_PARM_DESC(period, "Task period in ns (default: 100000)");

int ntasks = 16;
MODULE_PARM(ntasks, "i");
MODULE_PARM_DESC(ntasks, "Number of tasks to run periodically (default: 16)");

int stack_size = 3000;
MODULE_PARM(stack_size, "i");
MODULE_PARM_DESC(stack_size, "Task stack size in bytes (default: 4000)");


EXPORT_NO_SYMBOLS;

#define ONE_SHOT

#define TIMER_TO_CPU 3 // < 0 || > 1 to maintain a symmetric processed timer.
#define RUNNABLE_ON_CPUS (i%2 + 1) // forced statically half and half.
//#define RUNNABLE_ON_CPUS 3     // 1: on cpu 0 only, 2: on cpu 1 only, 3: on any.
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)

#define LOOPS 1000000000

static RT_TASK *thread;

static int cpu_used[NR_RT_CPUS];

static void fun(int t)
{
	unsigned int loops = LOOPS;
	while(loops--) {
		cpu_used[hard_cpu_id()]++;
		rt_leds_set_mask(1,t);
		rt_task_wait_period();
	}
}


int init_module(void)
{
	RTIME tick_period;
	RTIME now;
	int i, k;

#ifdef ONE_SHOT
	rt_set_oneshot_mode();
#endif
	k = 1;
	thread = (RT_TASK *)kmalloc(ntasks*sizeof(RT_TASK), GFP_KERNEL);
	for (i = 0; i < ntasks; i++) {
		rt_task_init(&thread[i], fun, k = ~k, stack_size, 0, 0, 0);
	}
	tick_period = start_rt_timer(nano2count(period));
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, TIMER_TO_CPU);
	now = rt_get_time() + ntasks*tick_period;
	for (i = 0; i < ntasks; i++) {
		rt_task_make_periodic(&thread[i], now + (i+1)*tick_period, ntasks*tick_period);
	}
	for (i = 0; i < ntasks; i++) {
		rt_set_runnable_on_cpus(&thread[i], RUN_ON_CPUS); 
	}
	return 0;
}


void cleanup_module(void)
{
	int i, cpuid;

	rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
	stop_rt_timer();
	rt_busy_sleep(10000000);
	for (i = 0; i < ntasks; i++) {
		rt_task_delete(&thread[i]);
	}
	kfree(thread);
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
}
