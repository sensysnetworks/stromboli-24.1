/* 
 * rtai/include/asm-arm/arch-epxa10db/rtai_arch.h
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

#ifndef _ASM_ARCH_RTAI_ARCH_H_
#define _ASM_ARCH_RTAI_ARCH_H_

#define FREQ_SYS_CLK		CLOCK_TICK_RATE
#define LATENCY_MATCH_REG	2500
#define SETUP_TIME_MATCH_REG	500

#define TIMER_8254_IRQ		8


/*
 * We use timer1 on the Excalibur CPU as RTAI time stamp counter.
 * This timer is not used by Linux. If used by RTAI,
 * it must not be used by any other drivers. Check this first.
 * The timer1 is operated in free-running heartbeat mode; in this
 * mode the timer increments until it reaches the limit value
 * configured in the LIMIT register, at which point it is reset
 * to 0 and begins incrementing again. This repeats while the start
 * bit S remains set (forever in our case).
 * The LIMIT value used corresponds to 1s at 125 MHz CPU clock.
 * Note that timer1 is read only and the interrupt of this timer is
 * not enabled.
*/

#define arch_mount_rtai()			\
{						\
	extern unsigned long rtai_lasttsc;	\
	extern union rtai_tsc rtai_tsc;		\
	/* setup our tsc compare register */	\
	rtai_tsc.tsc = 0LL;			\
                                                \
        /* Stop the timer1, reconfigure it and then restart it */	\
        *TIMER1_CR(IO_ADDRESS(EXC_TIMER00_BASE))=0;			\
	*TIMER1_LIMIT(IO_ADDRESS(EXC_TIMER00_BASE))=(unsigned  int)(EXC_AHB2_CLK_FREQUENCY/2);	\
	*TIMER1_PRESCALE(IO_ADDRESS(EXC_TIMER00_BASE))=1;	\
	*TIMER1_CR(IO_ADDRESS(EXC_TIMER00_BASE))= TIMER0_CR_S_MSK;	\
	rtai_lasttsc = 0; \
}

/* nothing to do for umount */
#define arch_umount_rtai()			\
{						\
	/* Stop timestamp counter */		\
        *TIMER1_CR(IO_ADDRESS(EXC_TIMER00_BASE))=0;	\
}

/* We have no demultiplexing, so set this to 0 */
#define isdemuxirq(irq) 0

#endif
