
/*
COPYRIGHT (C) 2000  Pierre Cloutier (pcloutier@PoseidonControls.com)

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

//#define __KERNEL__
//#define MODULE

#include <linux/module.h>

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <rtai_lxrt.h>

#include "leds_lxrt.h"

MODULE_LICENSE("GPL");
//EXPORT_NO_SYMBOLS;

static int leds, port;

static struct rt_fun_entry rt_leds_fun[] = {
	[SET_LEDS]	= { 0, rt_set_leds },
	[RESET_LEDS]	= { 0, rt_reset_leds },
	[TOGGLE_LEDS]	= { 0, rt_toggle_leds },
	[RT_SET_LEDS_PORT ] = { 0, rt_set_leds_port },
	[RT_GET_LEDS]	= { 0, rt_get_leds },
};


void rt_toggle_leds(int l)
{
	// only works if l has one bit.
	if(leds & l) leds &= ~l ; else leds |= l;
	outb(leds, port);
}

void rt_reset_leds(int l)
{
	//rt_printk("rt_reset_leds(%d)\n", l);
	leds |= l;
	outb(leds, port);
}

void rt_set_leds(int l)
{
	leds &= ~l;
	outb(leds, port);
}

int rt_get_leds(void)
{
	return ~leds;
}

void rt_set_leds_port(int p)
{
	port = p;
}
		
/* init module */
int init_module(void)
{

	if( set_rt_fun_ext_index(rt_leds_fun, MYIDX)) {
		printk("Recompile your module with a different index\n");
		return -EACCES;
	}

	rt_set_leds_port(0x378);

	return(0);
}

/*  cleanup module */
void cleanup_module(void)
{
	reset_rt_fun_ext_index(rt_leds_fun, MYIDX);
}

