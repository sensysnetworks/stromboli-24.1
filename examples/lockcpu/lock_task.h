/*
COPYRIGHT (C) 2001  Paolo Mantegazza (mantegazza@aero.polimi.it)

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

#ifndef _RTAI_LOCK_H_
#define _RTAI_LOCK_H_

#define LOCKIDX  13

#define LOCK_CPU	0 
#define UNLOCK_CPU	1

#define RT_CPU 1
#define IRQs { 0, 1, 2, 10, 11, 12, 14, 15 }

#define LOCKNAME  0x11892C  // is nam2num("IDLE")

#ifdef __KERNEL__

#define STACK_SIZE 5000

#define CHECK

#define MAXIRQS 32

extern int rt_lock_cpu(int cpu, int irqs[], int nirqs);

extern void rt_unlock_cpu(void);

static struct rt_fun_entry rt_lock_fun[]  __attribute__ ((__unused__));
static struct rt_fun_entry rt_lock_fun[] = {
	{ 0, rt_lock_cpu },  	//   0
	{ 0, rt_unlock_cpu},   	//   1
};

#else

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>

extern union rtai_lxrt_t rtai_lxrt(short int dynx, short int lsize, int srq, void *arg);

#define SIZARG sizeof(arg)

static inline int lock_cpu(int cpu, int irqs[], int nirqs)
{
	struct { int cpu; int *irqs; int nirqs; } arg = { cpu, irqs, nirqs };
	return rtai_lxrt(LOCKIDX, SIZARG, LOCK_CPU, &arg).i[LOW];
}


static inline void unlock_cpu(void)
{
	struct { int dummy; } arg = { 0 };
	rtai_lxrt(LOCKIDX, SIZARG, UNLOCK_CPU, &arg);
}

#endif

#endif
