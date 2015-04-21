/*
COPYRIGHT (C) 2000  Pierre Cloutier (pcloutier@PoseidonControls.com)
		    Ian Soanes (ians@lineo.com)

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

Module for testing trap handling and watchdog.
*/

#include <linux/module.h>

#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>

#include <asm/smp.h>
#define TICK_PERIOD 1000000000
#define STACK_SIZE 2000
#define NUM_CHILDREN 1

RT_TASK agentask[NUM_CHILDREN];
static int count[NUM_CHILDREN];
static int watchdog = 0;			// Running with watchdog?

void crashme(void)
{
    int *zpt;
    int count;
    zpt   = NULL;
    count = 0;

//  Various ways to crash - take your pick!!
//  __asm__ __volatile__ ("int $3"); 		// Breakpoint
//  *zpt = -1; 					// Bad pointer
    count /= 0;					// Divide by zero

//  Various ways to hang (or sometimes crash) - for watchdog testing
//  rt_busy_sleep(TICK_PERIOD + 25000000);	// Overrun
//  for (count = 1; 1; count++); 		// Loop forever
}

void fun(int c)
{
    rt_printk("Task %p starts\n", rt_whoami());
    while (count[c] != 10) {
	count[c]++;
	if (count[c] >= c) {
	    crashme();
	}
	rt_printk("Loop %d child %d (CPU %d)\n", count[c], c, hard_cpu_id());
	rt_task_wait_period();
    }
    rt_task_suspend(rt_whoami());
}

int init_module(void)
{
    RTIME period;
    int i;

    // __asm__ __volatile__ ("int $3"); // Initial breakpoint
    for (i = 0; i < NUM_CHILDREN; i++) {
	rt_task_init(&agentask[i], fun, i, STACK_SIZE, NUM_CHILDREN - i, 0, 0);
	// rt_set_runnable_on_cpuid(&agentask[i], i % NR_RT_CPUS);
    }
    if (!watchdog) {
	rt_set_oneshot_mode();
	// rt_set_periodic_mode();
	start_rt_timer((int)nano2count(TICK_PERIOD));
    }
    period = nano2count(TICK_PERIOD);
    for (i = 0; i < NUM_CHILDREN; i++) {
	rt_task_make_periodic(&agentask[i], rt_get_time() + period, period);
    }
    return 0;
}

void cleanup_module(void)
{
    int i;

    if (!watchdog) stop_rt_timer();
    rt_busy_sleep(TICK_PERIOD);
    for (i = 0; i < NUM_CHILDREN; i++) {
	rt_task_delete(&agentask[i]);
    }
}
