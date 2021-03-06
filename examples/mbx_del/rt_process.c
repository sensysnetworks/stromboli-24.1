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


#include <linux/module.h>

#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>

#define RT_PRINTK  rtai_print_to_screen

#define STACK_SIZE  2000

#define NUM_TASKS  16

static RT_TASK task[NUM_TASKS + 1];

static MBX mbx;

void fun1(int t)
{
	int msg;
	if (t & 1) {
		RT_PRINTK("TASK %d BLOCKS TIMED RECEIVING ON MBX\n", t); 
		RT_PRINTK("TASK %d UNBLOCKS RECEIVING ON MBX %d BYTES\n", t, rt_mbx_receive_timed(&mbx, &msg, sizeof(msg), 2000000000));
	} else {
		RT_PRINTK("TASK %d BLOCKS RECEIVING ON MBX\n", t); 
		RT_PRINTK("TASK %d UNBLOCKS RECEIVING ON MBX %d BYTES\n", t, rt_mbx_receive(&mbx, &msg, sizeof(msg)));
	}
//	rt_task_suspend(&task[t]);
}

void fun2(int t)
{
	RT_PRINTK("ANOTHER TASK DELETES THE MBX AND ...\n"); 
	rt_mbx_delete(&mbx);
//	rt_task_suspend(&task[t]);
}

int init_module(void)
{
	int i;
	rt_mbx_init(&mbx, 5);
	for (i = 0; i < NUM_TASKS; i++) {
		rt_task_init(&task[i], fun1, i, STACK_SIZE, 0, 0, 0);
		rt_task_resume(&task[i]);
	}
	rt_task_init(&task[NUM_TASKS], fun2, NUM_TASKS, STACK_SIZE, 1, 0, 0);
	rt_task_resume(&task[NUM_TASKS]);
	return 0;
}

void cleanup_module(void)
{
	int i;
	rt_mbx_delete(&mbx);
	for (i = 0; i <= NUM_TASKS; i++) {
		rt_task_delete(&task[i]);
	}
}
