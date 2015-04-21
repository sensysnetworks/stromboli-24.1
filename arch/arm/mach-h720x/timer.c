/*
 *  /arch/arm/mach-h7202/timer.c
 *
 * (c) 2003 Thomas Gleixner, <tglx@linutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This is the timer handling for RTAI Real Time Application Interface
 * COPYRIGHT (C) 2001  Paolo Mantegazza (mantegazza@aero.polimi.it)
 *
 * $Id: timer.c,v 1.1.1.1 2004/06/06 14:01:35 rpm Exp $
 *
 */
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/mach/irq.h>
#include <asm/types.h>
#include <rtai.h>

unsigned long rtai_lasttsc;
unsigned long rtai_TC2latch;
u32 timer_mode = TM_REPEAT;

/*
*	linux_timer_interrupt is the handler, when RTAI is mounted,
*	but the timer was not requested by a realtime task
*/
void linux_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	/* Read timer status. Obviously this clears the timer int, but who knows :)
	 * Datasheet is very informative at this point. Without that system locks. 
	 */
	CPU_REG(TIMER_VIRT, TIMER_TOPSTAT);

	rdtsc();		/* update tsc */
	do_timer(regs);
}

/*
*	soft_timer_interrupt is the handler, when RTAI is mounted,
*	the timer is requested by a realtime task and is invoked by
*	the scheduler when there is time to do it
*/
void soft_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	rdtsc();		/* update tsc */
	do_timer(regs);
}

/*
*	rt_request_timer set's the new timer latch value, replaces 
*	linux_timer_handler with soft_timer_handler, set's the new
*	timer latch value and free's + requests the global timer 
*	irq as realtime interrupt.
*/
void rt_request_timer(void (*handler) (void), unsigned int tick, int unused)
{

	unsigned long flags;

	flags = hard_lock_all();

	/* set up rt_times structure */
	rt_times.linux_tick = LATCH;
	rt_times.periodic_tick = (tick > 0 && tick < rt_times.linux_tick) ? tick : rt_times.linux_tick;
	rt_times.tick_time = rdtsc();
	rt_times.intr_time = rt_times.tick_time + (RTIME) rt_times.periodic_tick;
	rt_times.linux_time = rt_times.tick_time + (RTIME) rt_times.linux_tick;

	/* update Match-register */

	D1(if (rt_times.periodic_tick > LATCH)
	   	printk("Periodic tick > LATCH\n"););

	timer_mode = 0;
	rt_set_timer_latch((unsigned long) rt_times.periodic_tick);

	IRQ_DESC[TIMER_8254_IRQ].action->handler = soft_timer_interrupt;

	rt_free_global_irq(TIMER_8254_IRQ);
	rt_request_global_irq(TIMER_8254_IRQ, handler);

	hard_unlock_all(flags);

	return;
}

/*
*	Switch back to linux_timer_handler, set the timer latch to
*	(linux) LATCH, release global interupt.
*/
void rt_free_timer(void)
{
	unsigned long flags;

	flags = hard_lock_all();

	rt_free_global_irq(TIMER_8254_IRQ);
	IRQ_DESC[TIMER_8254_IRQ].action->handler = linux_timer_interrupt;

	/* set up rt_times structure */
	rt_times.linux_tick = LATCH;
	rt_times.periodic_tick = LATCH;
	rt_times.tick_time = rdtsc();
	rt_times.intr_time = rt_times.tick_time + (RTIME) rt_times.periodic_tick;
	rt_times.linux_time = rt_times.tick_time + (RTIME) rt_times.linux_tick;

	/* update Match-register */
	timer_mode = TM_REPEAT;
	rt_set_timer_latch((unsigned long) rt_times.periodic_tick);

	hard_unlock_all(flags);
}
