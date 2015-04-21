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
#include <rtai_sched.h>
#include <rtai_fifos.h>

#include "rtai_parport.h"

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

#define RECEIVER_STACK_SIZE	10000
#define RECEIVER_PRIORITY	2

static RT_TASK receiver_task;
static SEM sem_comm;

static void parport_irq_handler (void)
{
// Disable IRQ Via Ack Line
	outb (inb(rt_parport_dev.io_base+2) & (~0x10), rt_parport_dev.io_base+2);
	rt_sem_signal (&sem_comm);
}


static void receiver (int t)
{
	int inputs [NUM_DATA];
	int i, r = 0, remainder = 0;

	while (1) {
		if (remainder) {
			outb_p (0, rt_parport_dev.io_base);
		} else {
			outb_p (rt_parport_dev.laplink_mode, rt_parport_dev.io_base);
			outb_p (0, rt_parport_dev.io_base);
		}
//Enable IRQ Via Ack Line
		outb (inb(rt_parport_dev.io_base+2) | 0x10, rt_parport_dev.io_base+2);
		rt_sem_wait (&sem_comm);
		rt_parport_read (inputs, r);
		remainder = r;
		printk("RECEIVING  ");
		for (i = 0; i < NUM_DATA; i++) 
			printk(">>> %x <<<", inputs[i]);
		printk("\n");
	}
}


int init_module (void)
{
	outb (0, rt_parport_dev.io_base);
	rt_parport_dev.handler = parport_irq_handler;
	rt_request_global_irq (rt_parport_dev.irq, rt_parport_dev.handler);
	rt_startup_irq (rt_parport_dev.irq);	
	rt_set_oneshot_mode ();
	start_rt_timer (0);
 	rt_task_init (&receiver_task, receiver, 0, RECEIVER_STACK_SIZE, RECEIVER_PRIORITY, 1, 0);
	rt_sem_init (&sem_comm, 0);
 	rt_task_resume (&receiver_task);
	return 0;
}

void cleanup_module (void)
{
	stop_rt_timer ();
	rt_shutdown_irq (rt_parport_dev.irq);
	rt_free_global_irq (rt_parport_dev.irq);
	outb (0, rt_parport_dev.io_base);
	rt_task_delete (&receiver_task);
	rt_sem_delete (&sem_comm);
}
