/**
 * rt_com test
 * ===========
 *
 * RT-Linux kernel module for communication across serial lines.
 *
 * Adaptation of testcom.c module to provide the same example in RTAI environment.
 * (1999/11/04)
 *
 * Copyright (C) 1999 Ebersold Andre.
 */

#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef __RT__
#  define __RT__
#endif
#ifndef MODULE
#  define MODULE
#endif


#ifdef RTLINUX_V1
#include <rtl.h>
#include <rtl_fifo.h>
#endif


#ifdef RTAI
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/mc146818rtc.h>

#include <asm/system.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/bitops.h>
#include <asm/atomic.h>
#include <asm/desc.h>


#include <rtai.h>
#include <rtai_fifos.h>
#include <rtai_sched.h>

#endif

#include "rt_com.h"


#define   TICK_PERIOD 100000000 

#define STACK_SIZE 1000
#define PORT 0
 
RT_TASK thread;

/**
   Basic task that read and write on the serial port
*/
static void serial_handler(int t) {
		      
    do {
	int n;
	char buf[210];

       	rt_task_wait_period();
       	n = rt_com_read(PORT, buf,5); 
       	if (n > 0) {
	    buf[n] = 0;
	    rt_com_write(PORT, "\rread->", 7);
	    rt_com_write(PORT, buf, n);
	    rt_com_write(PORT, "\r\n", 2);
	}

    } while (1);

}
/**
   Init module. initialize the serial port,
   create a periodic task serial_handler that read and write to the serial port
*/
int init_module(void)
{
	int thread_status;
        RTIME tick_period;

	rt_com_setup(PORT, 38400, 0, RT_COM_PARITY_NONE, 1, 7);
	rt_com_write(PORT, "Begin serial Test\n\r", 20);

	printk("<1>RT_Com Test Program\n");

	rt_task_init(&thread, serial_handler, 0, STACK_SIZE, 0, 0, 0);
	tick_period = start_rt_timer(nano2count(TICK_PERIOD));
	rt_task_make_periodic(&thread, rt_get_time(), tick_period);
	return 0;
}

/**
   free all allocated resources
 */
void cleanup_module(void)
{
	stop_rt_timer();
	rt_busy_sleep(1E7);
	rt_task_delete(&thread);
	rt_com_setup(PORT, -1, 0, 0, 0, 0);
	printk ("<1> Goodbye Test Serial\n");
}



