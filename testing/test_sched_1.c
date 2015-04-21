/*
COPYRIGHT (C) 2001  David Schleef <ds@schleef.org>

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

/* A very simple test to make sure that scheduling works */

static RT_TASK(task);

static void task_function(int i)
{
	static int counter;

	while(1){
		rt_printk("%d\n",counter++);
		rt_task_wait_period();
	}
}

int init_module(void)
{
	RTIME tick_period;
	RTIME now;

	tick_period = start_rt_timer(nano2count(500000));
#if 0
	now = rt_get_time();

	rt_task_init(&task,task_function,0,2000,0,0,0);

	rt_task_make_periodic(&task,now+tick_period,tick_period);
#endif

	return 0;
}

void cleanup_module(void)
{
	stop_rt_timer();
#if 0
	rt_task_delete(&task);
#endif
}

