/*
    testing/rtai_test_timer_1.c
    A test for direct use of timers.

    RTAI - Real Time Application Interface
    Copyright (C) 2001 David A. Schleef <ds@schleef.org>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
    USA.
*/

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <rtai.h>


#define cpu_freq (tb_ticks_per_jiffy*HZ)

static int counter;

static void timer_handler(void)
{
	if((counter%10000)==0)printk("%d\n",counter);
	counter++;
}

RTIME nano2count(int ns)
{
	RTIME tick;

	if(ns<0){
		tick = -llimd(-ns, cpu_freq, 1000000000);
	}else{
		tick = llimd(ns, cpu_freq, 1000000000);
	}

	return tick;
}

int init_module(void)
{
	unsigned int tick;

	tick = nano2count(100000);
	printk("rt_request_timer(timer_handler, %d, 0)\n",(int)tick);
	rt_request_timer(timer_handler, tick, 0);

	return 0;
}

void cleanup_module(void)
{
	rt_free_timer();
}

