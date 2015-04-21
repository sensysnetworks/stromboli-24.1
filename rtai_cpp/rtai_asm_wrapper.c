/*
 * Project: rtai_cpp - RTAI C++ Framework 
 *
 * File: $Id: rtai_asm_wrapper.c,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $
 *
 * Copyright: (C) 2001,2002 Erwin Rol <erwin@muffin.org>
 *
 * Licence:
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include "rtai_asm_wrapper.h"

#ifdef RTASK
#undef RTASK
#endif

#ifdef SEM
#undef SEM
#endif

#ifdef CND
#undef CND
#endif

#ifdef MBX
#undef MBX
#endif

#ifdef TBX
#undef TBX
#endif

#ifdef BITS
#undef BITS
#endif

#include <rtai_sched.h>
#include <rtai.h>
#include <rtai_fifos.h>
#include <rtai_nam2num.h>
#include <rtai_tbx.h>
#include <rtai_bits.h>
#include <rtai_trace.h>
#include <../lxrt/registry.h>


/* add wrappers for inline functions */



