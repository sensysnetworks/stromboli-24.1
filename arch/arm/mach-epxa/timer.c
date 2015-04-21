/* 
* rtai/arch/arm/mach-epxa10db/timer.c
*
* COPYRIGHT (C) 2003 Thomas Gleixner (tglx@linutronix.de)
* COPYRIGHT (C) 2003 Bernard Haible, Marconi Communications GmbH 
*				(bernard.haible at marconi.com)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of version 2 of the GNU General Public License as
* published by the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
* 
* 
--------------------------------------------------------------------------
* Acknowledgements
* - Paolo Mantegazza	(mantegazza@aero.polimi.it) creator of RTAI 
*/

#include <linux/config.h>
#include <asm/system.h>
#include <asm/leds.h>
#include <asm/arch/hardware.h>
#define TIMER00_TYPE (volatile unsigned int*)
#include <asm/arch/timer00.h>

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/mach/irq.h>
#include <rtai.h>

#define CHECK_TIMER0_BOUNDS
#define CPU_FREQ (tuned.cpu_freq)
#define DEBUG_INLINE 0
#if DEBUG_INLINE > 0
#define DI1(x) x
#else
#define DI1(x)
#endif


unsigned long rtai_lasttsc;
unsigned long rtai_timer0_limit;

/*
 * linux_timer_interrupt is the handler, when RTAI is mounted,
 * but the timer was not requested by a realtime task
 * hacked from linux/include/asm-arm/arch-epxa10db/time.h
*/
void linux_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	/* update tsc */
	rdtsc(); 	

	/* ack int */
	*TIMER0_CR(IO_ADDRESS(EXC_TIMER00_BASE))|=TIMER0_CR_CI_MSK;

#ifdef CONFIG_LEDS_TIMER
	do_leds();
#endif
	do_timer(regs);
}

/*
 * soft_timer_interrupt is the handler, when RTAI is mounted,
 * the timer is requested by a realtime task and is invoked by
 * the linux interrupt pending service when there is time to do it
*/
void soft_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	/* update tsc */
	rdtsc(); 	
#ifdef CONFIG_LEDS_TIMER
	do_leds();
#endif
	do_timer(regs);
}

/*
 * rt_request_timer set's the new timer latch value, replaces 
 * linux_timer_handler with soft_timer_handler, set's the new
 * timer latch value and requests the global timer 
 * irq as realtime interrupt.
*/
void rt_request_timer(void (*handler)(void), unsigned int tick, int unused)
{
	RTIME t;
	unsigned long flags;

	flags = hard_lock_all();

	/* wait for timer0 match 0 and clear the interrupt bit */
	do {
		;
	} while (!(*TIMER0_CR(IO_ADDRESS(EXC_TIMER00_BASE)) & TIMER0_CR_CI_MSK));

	/* Ack int */	
	*TIMER0_CR(IO_ADDRESS(EXC_TIMER00_BASE)) |= TIMER0_CR_CI_MSK;

	/* set up rt_times structure */
	rt_times.linux_tick = LATCH;
	rt_times.periodic_tick = (tick > 0 && tick < rt_times.linux_tick) ? tick : rt_times.linux_tick;
	rt_times.tick_time  = t = rdtsc();
	rt_times.intr_time  = t + (RTIME) rt_times.periodic_tick;
	rt_times.linux_time = t + (RTIME) rt_times.linux_tick;

	/* update timer0 limit register */
	rt_set_timer0_limit(rt_times.periodic_tick);

	IRQ_DESC[TIMER_8254_IRQ].action->handler = soft_timer_interrupt;

	rt_free_global_irq(TIMER_8254_IRQ);
	rt_request_global_irq(TIMER_8254_IRQ, handler);

	/* pend linux timer irq to handle current jiffie */
 	rt_pend_linux_irq(TIMER_8254_IRQ);
 
	hard_unlock_all(flags);

	return;
}

/*
 * Switch back to linux_timer_handler, set the timer latch to
 * (linux) LATCH, release global interupt.
*/
void rt_free_timer(void)
{
	unsigned long flags;

	flags = hard_lock_all();

	rt_free_global_irq(TIMER_8254_IRQ);
	IRQ_DESC[TIMER_8254_IRQ].action->handler = linux_timer_interrupt;

	/* wait for a timer underflow and clear the int. bit */
	/* wait for timer0 match 0 and clear the interrupt bit */
	do {
		;
	} while (!(*TIMER0_CR(IO_ADDRESS(EXC_TIMER00_BASE)) & TIMER0_CR_CI_MSK));

	/* Ack int and stop timer */	
	*TIMER0_CR(IO_ADDRESS(EXC_TIMER00_BASE)) = TIMER0_CR_CI_MSK;
	/* Setup timer for 100HZ */
	*TIMER0_LIMIT(IO_ADDRESS(EXC_TIMER00_BASE))=(unsigned int)(EXC_AHB2_CLK_FREQUENCY/200);
	*TIMER0_PRESCALE(IO_ADDRESS(EXC_TIMER00_BASE))=1;
	*TIMER0_CR(IO_ADDRESS(EXC_TIMER00_BASE))=TIMER0_CR_IE_MSK | TIMER0_CR_S_MSK;

	/* pend linux timer irq to handle current jiffie */
 	rt_pend_linux_irq(TIMER_8254_IRQ);

	hard_unlock_all(flags);
}
