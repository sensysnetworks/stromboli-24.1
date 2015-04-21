/*
 * RTAI-Linux kernel module for communication across serial lines.
 * Copyright (C) 1999 Ebersold Andre.
 *
 * Adapted to the new spdrv version by Paolo Mantegazza.
 *
 */

#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif

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

#include <rtai.h>
#include <rtai_fifos.h>
#include <rtai_sched.h>

#include <rtai_spdrv.h>

#define  TICK_PERIOD  100000000 
#define  STACK_SIZE  1000
#define  PORT  0
 
RT_TASK thread;

static void serial_handler(int t) 
{
	while (1) {
		int n;
		char buf[210];

		rt_task_wait_period();
		n = 5 - rt_spread(PORT, buf, 5); 
		if (n > 0) {
			buf[n] = 0;
			rt_spwrite(PORT, "\rread->", 7);
			rt_spwrite(PORT, buf, n);
			rt_spwrite(PORT, "\r\n", 2);
		}
	}
}

int init_module(void)
{
        RTIME tick_period;

	rt_spopen(PORT, 38400, 8, 1, RT_SP_PARITY_NONE, 
		  RT_SP_NO_HAND_SHAKE, RT_SP_FIFO_SIZE_8);
	rt_spwrite(PORT, "Begin serial Test\n\r", 20);
	printk("<1>RT_Com Test Program.\n");
	rt_task_init(&thread, serial_handler, 0, STACK_SIZE, 0, 0, 0);
	tick_period = start_rt_timer(nano2count(TICK_PERIOD));
	rt_task_make_periodic(&thread, rt_get_time(), tick_period);
	return 0;
}

void cleanup_module(void)
{
	stop_rt_timer();
	rt_task_delete(&thread);
	rt_spclose(PORT);
	printk ("<1> Goodbye Test Serial.\n");
}
