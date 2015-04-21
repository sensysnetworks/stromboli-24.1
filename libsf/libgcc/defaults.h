/* Defines for RTAI softfloat support hacked from GCC sources 
	Copyright (c) 2003, Thomas Gleixner, <tglx@linutronix.de)
*/

/* Definitions of various defaults for tm.h macros.
   Copyright (C) 1992, 1996, 1997, 1998, 1999, 2000, 2001
   Free Software Foundation, Inc.
   Contributed by Ron Guilmette (rfg@monkeys.com)

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

#ifndef GCC_DEFAULTS_H
#define GCC_DEFAULTS_H

/* Default sizes for base C types.  If the sizes are different for
   your target, you should override these values by defining the
   appropriate symbols in your tm.h file.  */

#ifndef CHAR_TYPE_SIZE
#define CHAR_TYPE_SIZE BITS_PER_UNIT
#endif

#ifndef SHORT_TYPE_SIZE
#define SHORT_TYPE_SIZE (BITS_PER_UNIT * MIN ((UNITS_PER_WORD + 1) / 2, 2))
#endif

#ifndef INT_TYPE_SIZE
#define INT_TYPE_SIZE BITS_PER_WORD
#endif

#ifndef LONG_TYPE_SIZE
#define LONG_TYPE_SIZE BITS_PER_WORD
#endif

#ifndef LONG_LONG_TYPE_SIZE
#define LONG_LONG_TYPE_SIZE (BITS_PER_WORD * 2)
#endif

#ifndef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE INT_TYPE_SIZE
#endif

#ifndef WCHAR_UNSIGNED
#define WCHAR_UNSIGNED 0
#endif

#ifndef FLOAT_TYPE_SIZE
#define FLOAT_TYPE_SIZE BITS_PER_WORD
#endif

#ifndef DOUBLE_TYPE_SIZE
#define DOUBLE_TYPE_SIZE (BITS_PER_WORD * 2)
#endif

#ifndef LONG_DOUBLE_TYPE_SIZE
#define LONG_DOUBLE_TYPE_SIZE (BITS_PER_WORD * 2)
#endif

#ifndef BUILD_VA_LIST_TYPE
#define BUILD_VA_LIST_TYPE(X) ((X) = ptr_type_node)
#endif

#endif  /* GCC_DEFAULTS_H */

