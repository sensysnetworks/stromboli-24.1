/* include/asm-arm/arch-h720x/rtai_exports.h
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
 * $Id: rtai_exports.h,v 1.1.1.1 2004/06/06 14:01:47 rpm Exp $
*/

/*
 * Acknowledgements
 * - Paolo Mantegazza	(mantegazza@aero.polimi.it) creator of RTAI 
*/

/* 
* This file is included from rtai.c to support arch-specific exports
*/
#ifndef _ASM_ARCH_RTAI_EXPORT_H_
#define _ASM_ARCH_RTAI_EXPORT_H_

EXPORT_SYMBOL(rtai_TC2latch);
EXPORT_SYMBOL(timer_mode);

#endif
