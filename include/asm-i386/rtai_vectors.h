/*
 *   ARTI -- RTAI-compatible Adeos-based Real-Time Interface. Based on
 *   the original RTAI layer for x86.
 *
 *   Original RTAI/x86 layer implementation:
 *   Copyright (C) 2000 Paolo Mantegazza,
 *   Copyright (C) 2000 Steve Papacharalambous,
 *   Copyright (C) 2000 Stuart Hughes,
 *   and others.
 *
 *   RTAI/x86 rewrite over Adeos:
 *   Copyright (C) 2002 Philippe Gerum.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 675 Mass Ave, Cambridge MA 02139,
 *   USA; either version 2 of the License, or (at your option) any later
 *   version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _asm_rtai_vectors_h
#define _asm_rtai_vectors_h

#include <config.h>

#ifdef CONFIG_RTAI_ADEOS

/* On Linux x86, Adeos reserves vectors from 0xdf-0xee for domain
   usage. */

#define RTAI_APIC1_VECTOR  reserved
#define RTAI_APIC2_VECTOR  reserved
#define RTAI_APIC3_VECTOR  0xe1
#define RTAI_APIC4_VECTOR  0xe9

#define RTAI_APIC1_IPI     reserved
#define RTAI_APIC2_IPI     reserved
#define RTAI_APIC3_IPI     193
#define RTAI_APIC4_IPI     201

#define RTAI_SYS_VECTOR    0xe2
#define RTAI_LXRT_VECTOR   0xe3
#define RTAI_SHM_VECTOR    0xe4

#else /* CONFIG_RTHAL */

#ifdef CONFIG_RTAI_LINUX24

#define RTAI_APIC1_VECTOR  0xe1
#define RTAI_APIC2_VECTOR  0xe9
#define RTAI_APIC3_VECTOR  0xf1
#define RTAI_APIC4_VECTOR  0xf9

#else /* CONFIG_RTAI_LINUX22 */

#define RTAI_APIC1_VECTOR  0xd9
#define RTAI_APIC2_VECTOR  0xe1
#define RTAI_APIC3_VECTOR  0xe9
#define RTAI_APIC4_VECTOR  0xf1

#endif /* CONFIG_RTAI_LINUX24 */

#define RTAI_LXRT_VECTOR   0xfc
#define RTAI_SHM_VECTOR    0xfd
#define RTAI_SYS_VECTOR    0xfe

#endif /* CONFIG_RTAI_ADEOS */

#define __rtai_stringify(x)  #x
#define __rtai_do_trap(v)    __rtai_stringify(int $ ## v)

#define RTAI_DO_TRAP(v,r,a1,a2)  __asm__ __volatile__ ( __rtai_do_trap(v) \
						       : "=A" (r) : "a" (a1), "d" (a2))
#endif /* !_asm_rtai_vectors_h */
