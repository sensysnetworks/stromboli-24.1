/*
COPYRIGHT (C) 2001  Lineo Inc. (Author: bkuhn@lineo.com)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/

#ifndef RTAI_SHM_ASM_H
#define RTAI_SHM_ASM_H

#define RTAI_SHM_VECTOR 44

#ifndef __KERNEL__

static inline int rtai_shmrq(int srq, unsigned int whatever) {
  register long __res __asm__ ("%d0") = srq;
  register long __whatever __asm__ ("%d1") = whatever;
  
  __asm__ __volatile__ ("trap #12\n\t"
			: "=d" (__res)
			: "d" (__res), "d" (__whatever));
  return __res;
}

#else

#define RTAI_SHM_HANDLER rtai_shm_handler

#define DEFINE_SHM_HANDLER \
static void rtai_shm_handler(void) \
{ \
	__asm__ __volatile__ ("lea %sp@(-36),%sp\n\t" \
                              "moveml %d0-%d5/%a0-%a2,%sp@\n\t" \
                              "jsr shm_handler\n\t" \
                              "addql #4,%sp\n\t" \
                              "moveml %sp@,%d1-%d5/%a0-%a2\n\t" \
                              "lea %sp@(32),%sp\n\t" \
                              "rte\n\t"); \
}

#endif 

#endif 
