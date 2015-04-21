/*
COPYRIGHT (C) 2000  Lorenzo Dozio (dozio@aero.polimi.it)

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
#include <linux/version.h>

#include <asm/io.h>

#include <rtai.h>
#include <rtai_fifos.h>
#include <rtai_sched.h>

#include "rtai_parport.h"

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

//#define ONE_SHOT

#define SENDER_TASK_STACK_SIZE 	10000
#define SENDER_PRIORITY     	2
#define PERIOD			5000000

static RT_TASK sender_task;
static int period_trasm  = PERIOD;

static void sender (int t)
{
	unsigned int count = 1,i;
	int outputs [NUM_DATA];

	printk("\n");
	while (count <= LOOPS) {
		outputs [0] = 0xabcd;
		outputs [1] = 0xefab;
		printk("%d  SENDING  ", count);
		for (i=0;i<NUM_DATA;i++)
			printk(">>> %x <<<", outputs[i]);
		printk("\n");
		rt_parport_write (outputs);
		count++;
		rt_task_wait_period ();

	}
	printk("\n\n *** END OF TRASMISSION ***\n");
}


int init_module (void)
{
	RTIME tick_period;
	RTIME now;

#ifdef ONE_SHOT
	rt_set_oneshot_mode ();
#else
	rt_set_periodic_mode ();
#endif

	outb (0, rt_parport_dev.io_base);
// Disable IRQ Via Ack Line
	outb (inb(rt_parport_dev.io_base+2) & (~RT_PARPORT_CONTROL_ENABLE_IRQ),
		rt_parport_dev.io_base+2);
	tick_period = start_rt_timer ((int)nano2count(period_trasm));
	rt_task_init (&sender_task, sender, 1, SENDER_TASK_STACK_SIZE, SENDER_PRIORITY, 1, 0);
	now = rt_get_time ();
	rt_task_make_periodic (&sender_task, now + tick_period, tick_period);
	
	return 0;
}


void cleanup_module (void)
{
	stop_rt_timer ();
	rt_task_delete (&sender_task);
	outb (0, rt_parport_dev.io_base);
}
