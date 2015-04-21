//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 2000 Zentropic Computing, All rights reserved
//
// Authors:             Ian Soanes
// Original date:       Fri 20 Oct 2000
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
//
///////////////////////////////////////////////////////////////////////////////

#include <linux/module.h>

#include <asm/io.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>                                                        

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

#define TICK_PERIOD 100000
#define STACK_SIZE 2000                                                          

#define RUNNABLE_ON_CPUS 3
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)                    

#define NUM_CHILDREN 48
RT_TASK parent_task = {0};
static RT_TASK child_task[NUM_CHILDREN] = {{0}};
static int counters[NUM_CHILDREN];
int PRIORITY = RT_LOWEST_PRIORITY;

int my_trap_handler(int v, int signo, struct pt_regs *r, void *t)
{
    int i;

    rt_printk("RT task %p generated trap %d\n", t, v);
    for (i = 0; i < NUM_CHILDREN; i++) {
	if (&child_task[i] == t) {
	    if (counters[i] != 10) rt_printk("ERROR: ");
	    rt_printk("Child %d, loop counter = %d\n", i, counters[i]);
	    break;
	}
    }
    rt_task_suspend(t);
    rt_task_delete(t);
    return 1;
}

void child_func(int arg) 
{
    int *explode = 0;

    rt_printk("Starting child task %d\n", arg);
    for(counters[arg] = 0; counters[arg] < 10; counters[arg]++) {
	// rt_printk("Child task %d, loop count %d\n", arg, counters[arg]);
	rt_task_wait_period();
    }
    *explode = 0;
    rt_task_suspend(rt_whoami());
}

RTIME period;

void parent_func(int arg) 
{
    int i;

    rt_printk("Starting parent task %d\n", arg);
    for (i = 0; i < NUM_CHILDREN; i++) {
    	rt_task_init(&child_task[i], child_func, i, STACK_SIZE, PRIORITY, 0, 0);
	rt_set_task_trap_handler(&child_task[i], 14, my_trap_handler);
	rt_task_make_periodic(&child_task[i], rt_get_time() + period, period*i);
    }
    rt_task_suspend(rt_whoami());
}

int init_module(void)
{
    rt_task_init(&parent_task, parent_func, 0, STACK_SIZE, 0, 0, 0);
    rt_set_oneshot_mode();
    period = start_rt_timer((int) nano2count(TICK_PERIOD));
    rt_set_runnable_on_cpus(&parent_task, RUN_ON_CPUS);
    rt_task_resume(&parent_task);
    return 0;
}

void cleanup_module(void)
{
    int i;

    stop_rt_timer();
    for (i = 0; i < NUM_CHILDREN; i++) {
	rt_task_delete(&child_task[i]);
    }
    rt_busy_sleep(nano2count(TICK_PERIOD));
    rt_task_delete(&parent_task);
}
