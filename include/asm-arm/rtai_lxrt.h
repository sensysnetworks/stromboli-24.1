/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it),
extensions for user space modules are jointly copyrighted (2000) with:
			Pierre Cloutier (pcloutier@poseidoncontrols.com),
			Steve Papacharalambous (stevep@zentropix.com).

Port to ARM Copyright (c) 2001 Alex Z�pke, SYSGO RTS GmbH (azu@sysgo.de)

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


#ifndef RTAI_LXRT_ASM_H
#define RTAI_LXRT_ASM_H

#define RTAI_LXRT_VECTOR  0xFC

#define LOW 1

#ifndef __KERNEL__

#define RTAI_SRQ_MAGIC	"0x404404"

union rtai_lxrt_t { RTIME rt; int i[2]; void *v[2]; };

static union rtai_lxrt_t rtai_lxrt(unsigned long srq, void *arg)
{
	union rtai_lxrt_t retval;
	register unsigned long __sc_0 __asm__ ("r0") = srq | (RTAI_LXRT_VECTOR << 24);
	register unsigned long __sc_1 __asm__ ("r1") = arg;

	__asm__ __volatile__ (
	"swi\t" RTAI_SRQ_MAGIC "\n\t"
		: "=&r" (__sc_0), "=&r" (__sc_1)
		: "0" (__sc_0), "1" (__sc_1)
		: "r0", "r1");
	retval.i[0] = __sc_0;
	retval.i[1] = __sc_1;
	return retval;
}

#else

#define my_switch_to(prev, next, last)

#define STACK_STAGGER  27

#define RTAI_LXRT_HANDLER lxrt_handler

#define DEFINE_LXRT_HANDLER

#define DEFINE_LXRT_SYSCALL_HANDLER

#define init_lxrt_arch_stack( ) do { \
	*(rt_task->stack_top - 8) = (int)lxrt_rtai_fun_call; \
	*(rt_task->stack = rt_task->stack_top - 9) = MSR_KERNEL | MSR_FP | MSR_EE; \
	rt_task->stack = rt_task->stack_top + STACK_STAGGER; \
} while (0)

#endif

#endif
