
/*
COPYRIGHT (C) 1992-2000  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)

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

$Id: usrf.c,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $ 
*/

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/smp.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <rtai_lxrt.h>

#define SWITCH_MEM 0x80000000

#include "traps.h"

/*
   This file provides a function to execute a user handler of type:

	void (*handler)(void *data, int event);

   We have to be able to deal with three cases:

	1) The handler execute in the kernel. 

	2) The handler will execute in user space and was installed
           by a soft real time task. The handler executes real time.
 	
        3) The handler will execute in user space and was installed
           by a hard real time task. The handler also executes real time.

   qBlk's use exec_func() and thus become a very flexible tool
   that can run in all contexts.   

   Also, we setup a special trap handler to switch the memory back to Linux
   in case a problem occurs while the user function executes.  
*/

void unexec_func(void)
{
	RT_TASK *me;
	struct handler_data_t *hd;

	me = rt_lxrt_whoami();
	hd = me->trap_handler_data;

	hard_cli();
	rthal.switch_mem(hd->next, hd->prev, hd->cpuid | SWITCH_MEM);
	hard_sti();

	rt_free(hd);
	me->trap_handler_data = (void *)0;
}

void exec_func(void (*handler)(void *data, int evn), void *data, int evn)
{
	RT_TASK *me;
	struct handler_data_t *hd;
//	extern struct task_struct *rt_whoislinux(int cpuid);

	me = rt_lxrt_whoami();

	if (!me->lnxtsk) {
		// Plain real time task.
		handler(data, evn);
	} else {
/***
LXRT When handler executes, DS CS SS = kernel selectors.
***/
		hd = rt_malloc(sizeof(struct handler_data_t));
		me->trap_handler_data = hd;

		hard_cli();
		hd->cpuid       = hard_cpu_id();
		hd->next        = me->lnxtsk;
		hd->prev        = rt_whoislinux(hd->cpuid);
		hd->unexec_func = unexec_func;

		rthal.switch_mem(hd->prev, hd->next, hd->cpuid | SWITCH_MEM);
		hard_sti();

		handler(data, evn);

// SMP: Can we switch to a different CPU while in handler?		
//      If it can happen I think I'm in trouble below. 

		if( hd->cpuid != hard_cpu_id()) rt_printk( "BAD: CPU while changed in handler()\n");

		hard_cli();
 		rthal.switch_mem(hd->next, hd->prev, hd->cpuid | SWITCH_MEM);
		hard_sti();

		rt_free(hd);
		me->trap_handler_data = (void *)0;
	}
}
