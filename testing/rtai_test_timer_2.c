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


//#define PRINT rt_print_to_screen
#define PRINT rt_printk

#define cpu_freq (tb_ticks_per_jiffy*HZ)

typedef struct rt_timer_struct rt_timer;

struct rt_timer_struct{
	rt_timer *pnext;
	rt_timer *prev;
	unsigned int expires;
	void (*handler)(rt_timer *);
	unsigned long arg;
};

void rt_timer_add(rt_timer *);
void rt_timer_del(rt_timer *);

static int counter;
static rt_timer timer;
static unsigned int max_late = 0;
static unsigned int maxmax_late = 0;
static unsigned int total;

static void timer_handler2(rt_timer *t)
{
	unsigned int late;
	
	late = get_tbl() - t->expires;
	if(max_late<late)max_late=late;
	if(maxmax_late<late)maxmax_late=late;
	total+=late;

	if((counter%10000)==0){
		PRINT("%d %d %d %d\n",counter,total,
			maxmax_late,max_late);
		max_late = 0;
		total=0;
	}
	counter++;

	t->expires += t->arg;
	rt_timer_add(t);
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
	timer.arg = nano2count(100000);
	timer.handler = timer_handler2;
	timer.expires = get_tbl() + timer.arg;

	printk("tick (100 us) = %ld\n",timer.arg);

	rt_timer_add(&timer);

	return 0;
}

void cleanup_module(void)
{
	rt_timer_del(&timer);
}

