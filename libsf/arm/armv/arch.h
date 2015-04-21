/* Defines for RTAI softfloat support hacked from GCC sources 
	Copyright (c) 2003, Thomas Gleixner, <tglx@linutronix.de)
*/

/* Definitions of target machine for GNU compiler, for ARM.
   Copyright (C) 1991, 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000,
   2001 Free Software Foundation, Inc.
   Contributed by Pieter `Tiggr' Schoenmakers (rcpieter@win.tue.nl)
   and Martin Simmons (@harleqn.co.uk).
   More major hacks by Richard Earnshaw (rearnsha@arm.com)
   Minor hacks by Nick Clifton (nickc@cygnus.com)

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __ARM_H__
#define __ARM_H__

/* Define endianess */
#define LIBGCC2_WORDS_BIG_ENDIAN 0

/* Define this if most significant word of doubles is the lowest numbered.
   This is always true, even when in little-endian mode.  */
#define FLOAT_WORDS_BIG_ENDIAN 1

/* Number of bits in an addressable storage unit */
#define BITS_PER_UNIT  8

#define BITS_PER_WORD  32

#define UNITS_PER_WORD	4

/* Define which functions should be enabled
   If they should be exported define the corresponding
   export symbol, else define only
 */

#define L_clz
#define L_divdi3	__divdi3
#define L_fixunssfsi	__fixunssfsi

#endif /* __ARM_H__ */
