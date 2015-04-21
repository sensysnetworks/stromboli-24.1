/*
 * /include/asm/arch/mach-h7202/rtai_timer.h
 *
 * Don't include directly - it's included through asm-arm/rtai.h
 *
 * Copyright (c) 2003 Thomas Gleixner, <tglx@linutronix.de>
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
 * $Id: rtai_timer.h,v 1.1.1.1 2004/06/06 14:01:47 rpm Exp $
 *
 * mach_h720x (ARM7, hynix) timer inlines for 
 * RTAI Real Time Application Interface
 */
/*
--------------------------------------------------------------------------
Acknowledgements
- Paolo Mantegazza	(mantegazza@aero.polimi.it)
	creator of RTAI 
*/
#ifndef _ASM_ARCH_RTAI_TIMER_H_
#define _ASM_ARCH_RTAI_TIMER_H_

#include <asm/proc/hard_system.h>
#include <linux/sched.h>
#include <asm/arch/hardware.h>
#include <asm/types.h>

#define CPU_FREQ (tuned.cpu_freq)
#define CHECK_TC2_BOUNDS 1
#define DEBUG_INLINE 0
#if DEBUG_INLINE > 0
#define DI1(x) x
#else
#define DI1(x)
#endif

extern unsigned long rtai_TC2latch;
extern union rtai_tsc rtai_tsc;
extern u32 timer_mode;

union rtai_tsc {
	unsigned long long tsc;
	unsigned long hltsc[2];
};

/* This is used in the scheduler to do the timer specific 
 * ack, which is neccecary besides the msk_ack in dispatch_irq
*/
static inline int timer_irq_ack( void )
{
	/* Read timer status. Obviously this clears the timer int, but who knows :)
	 * Datasheet is very informative at this point. Without that system locks. 
	 */
	CPU_REG(TIMER_VIRT, TIMER_TOPSTAT);

	return 0;
}

/*
*	rdtsc reads RTAI's timestampcounter. We use Timer/Counter T3 
*	This is a 64 bit counter. Take care, that we don't read between
*	an overflow.
*
*/
static inline unsigned long long rdtsc(void)
{
        unsigned long flags;
 
        // we read the 16 bit timer and calc ts on this value
        hard_save_flags_and_cli(flags);
	while (1) {
		rtai_tsc.hltsc[1] = CPU_REG (TIMER_VIRT, T64_COUNTH);
		rtai_tsc.hltsc[0] = CPU_REG (TIMER_VIRT, T64_COUNTL);
		if (rtai_tsc.hltsc[1] == CPU_REG (TIMER_VIRT, T64_COUNTH))
			break;
	}
        hard_restore_flags(flags);
        return rtai_tsc.tsc;
}


/*
*	set the timer latch for timer/counter T0
*	we check the bounds, as long as we are in testphase
*	Switch this off by undef CHECK_TC2_BOUNDS
*/ 
static inline void rt_set_timer_latch(unsigned long delay)
{
        unsigned long flags;
        RTIME diff;
 
        // set 32 bit compare value
        hard_save_flags_cli(flags);

        diff = rt_times.intr_time - rdtsc();
	/* we have missed the deadline already */
	if (diff < 0)	
		diff = 1;	    

	DI1( if (delay > LATCH) {
		printk("rt_set_timer_latch delay > LATCH (%d) :%ld\n", LATCH, delay);
		delay = LATCH;
	});	

        rtai_TC2latch = (!delay ? ((unsigned long*)(&diff))[0] : delay);
#ifdef CHECK_TC2_BOUNDS
	if (rtai_TC2latch > LATCH) {
		DI1(printk("rt_set_timer_latch > LATCH :%ld\n",rtai_TC2latch));
		rtai_TC2latch = LATCH;
	}	
#endif
	/* This bound check is essential, remove it and you get in trouble */
	if (!rtai_TC2latch) {
		DI1(printk("rt_set_timer_latch < 400 :%ld\n",rtai_TC2latch));
		rtai_TC2latch = 1;
	}	
	CPU_REG (TIMER_VIRT, TM0_CTRL) = TM_RESET;
	CPU_REG (TIMER_VIRT, TM0_PERIOD) = rtai_TC2latch;
	CPU_REG (TIMER_VIRT, TM0_CTRL) = timer_mode | TM_START;  
        hard_restore_flags(flags);
}   

#define rt_set_timer_delay(x)  rt_set_timer_latch(x)

#endif
