/*
 * asm-arm/arch-h720x/rtai_arch.h
 *
 * Don't include directly - it's included through asm-arm/rtai.h
 *
 * Copyright (c) 2003, Thomas Gleixner, <tglx@linutronix.de)
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 *
 * $Id: rtai_arch.h,v 1.1.1.1 2004/06/06 14:01:47 rpm Exp $
 *
*/

/*
 * Acknowledgements
 * - Paolo Mantegazza	(mantegazza@aero.polimi.it) creator of RTAI 
*/

#include <asm/timex.h>

#ifndef _ASM_ARCH_RTAI_ARCH_H_
#define _ASM_ARCH_RTAI_ARCH_H_

#define FREQ_SYS_CLK        	CLOCK_TICK_RATE
#define LATENCY_MATCH_REG     	2500
#define SETUP_TIME_MATCH_REG   	500

#define TIMER_8254_IRQ 		15

#define arch_mount_rtai()			\
{						\
	extern union rtai_tsc rtai_tsc;		\
	/* setup our tsc compare register */	\
	rtai_tsc.tsc = 0LL;			\
}

/* nothing to do for umount */
#define arch_umount_rtai()	

/* We have no demultiplexing, so set this to 0 */
#define isdemuxirq(irq) 0

#endif
