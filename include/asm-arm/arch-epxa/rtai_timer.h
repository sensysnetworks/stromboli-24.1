/* 
 * rtai/include/asm-arm/arch-epxa/rtai_timer.h
 *
 * DON'T include directly - it's included through asm-arm/rtai.h
 *
 * COPYRIGHT (C) 2003 Thomas Gleixner (tglx@linutronix.de)
 * COPYRIGHT (C) 2003 Bernard Haible, Marconi Communications 
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
 *--------------------------------------------------------------------------
 * Acknowledgements
 * - Paolo Mantegazza	(mantegazza@aero.polimi.it) creator of RTAI 
*/

#ifndef _ASM_ARCH_RTAI_TIMER_H_
#define _ASM_ARCH_RTAI_TIMER_H_

#include <asm/proc/hard_system.h>
#include <linux/sched.h>
#define TIMER00_TYPE (volatile unsigned int*)
#include <asm/arch/timer00.h>

#define CPU_FREQ (tuned.cpu_freq)
#define DEBUG_INLINE 0
#if DEBUG_INLINE > 0
#define DI1(x) x
#else
#define DI1(x)
#endif

extern unsigned long rtai_timer0_limit;
extern unsigned long rtai_lasttsc;
extern union rtai_tsc rtai_tsc;

union rtai_tsc {
	unsigned long long tsc;
	unsigned long hltsc[2];
};

/* This is used in the scheduler to do the timer specific 
 * ack, which is neccecary besides the msk_ack in dispatch_irq
*/
static inline int timer_irq_ack( void )
{
	/* ack int */
	*TIMER0_CR(IO_ADDRESS(EXC_TIMER00_BASE)) |= TIMER0_CR_CI_MSK;
	return 0;
}


/*
 * rdtsc reads RTAI's timestampcounter. We use timer1 on the
 * Excalibur CPU. This timer is not used by Linux. If used by RTAI,
 * it must not be used by any other drivers. Check this first.
 * The timer1 is operated in free-running heartbeat mode; in this
 * mode the timer increments until it reaches the limit value
 * configured in the LIMIT register, at which point it is reset
 * to 0 and begins incrementing again. This repeats while the start
 * bit S remains set (forever in our case).
 * The LIMIT value used corresponds to 1s at 125 MHz CPU clock.
*/
static inline unsigned long long rdtsc(void)
{
        unsigned long flags, ticks, act;

        hard_save_flags_and_cli(flags);

	act = (unsigned long) *TIMER1_READ(IO_ADDRESS(EXC_TIMER00_BASE));
        /* take care of overflow */
	if (rtai_lasttsc < act)
		ticks = act - rtai_lasttsc;
	else
		ticks = (unsigned long) *TIMER1_LIMIT(IO_ADDRESS(EXC_TIMER00_BASE)) - rtai_lasttsc + act;
	
	rtai_lasttsc = act;
        rtai_tsc.tsc += (unsigned long long) ticks;

        hard_restore_flags(flags);
        return rtai_tsc.tsc;
}


/*
 * Set the timer limit register for Excalibur timer0.
 * Check the bounds as long as we are in testphase;
 * switch this off by undef CHECK_TIMER0_BOUNDS
*/
static inline void rt_set_timer0_limit(unsigned long delay)
{
        unsigned long flags;
        RTIME act, diff, diff_real;

        // set 16 bit LATCH
        hard_save_flags_cli(flags);

	act = rdtsc();
        diff = rt_times.intr_time - act;
	diff_real = diff;
	/* we have missed the deadline already */
	if (diff < 0)
		diff = 1;

	DI1( if (delay > LATCH) {
		printk("rt_set_timer_latch delay > LATCH :%ld\n",delay);
		delay = LATCH;
	});

        rtai_timer0_limit = (!delay ? ((unsigned long*)(&diff))[0] : delay);
	if (rtai_timer0_limit > LATCH) {
		DI1(printk("rt_set_timer_latch > LATCH :%ld\n",rtai_timer0_limit));
		rtai_timer0_limit = LATCH;
	}
	/* This bound check is essential, remove it and you get in trouble */
	if (!rtai_timer0_limit) {
		DI1(printk("rt_set_timer_limit < 1 :%ld\n",rtai_timer0_limit));
		rtai_timer0_limit = 1;
	}

        /* Stop the timer, clear pending interrupt,
	   reconfigure the timer and then restart it */
        *TIMER0_CR(IO_ADDRESS(EXC_TIMER00_BASE)) = TIMER0_CR_CI_MSK;
	*TIMER0_LIMIT(IO_ADDRESS(EXC_TIMER00_BASE))= rtai_timer0_limit;
	*TIMER0_PRESCALE(IO_ADDRESS(EXC_TIMER00_BASE)) = 1;
	*TIMER0_CR(IO_ADDRESS(EXC_TIMER00_BASE))=TIMER0_CR_IE_MSK | TIMER0_CR_S_MSK;

	rt_unmask_irq(TIMER_8254_IRQ);

	hard_restore_flags(flags);
}


#define rt_set_timer_delay(x)  rt_set_timer0_limit(x)

#endif
