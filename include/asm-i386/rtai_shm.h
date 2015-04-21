/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)

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

#include <asm/rtai_vectors.h>

#define __SHM_USE_VECTOR 1

#ifndef __KERNEL__

#ifdef __SHM_USE_VECTOR
static inline long long rtai_shmrq(int srq, unsigned int whatever)
{
	long long retval;
	RTAI_DO_TRAP(RTAI_SHM_VECTOR,retval,srq,whatever);
	return retval;
}
#endif

#else

#define RTAI_SHM_HANDLER rtai_shm_handler

#define __STR(x) #x
#define STR(x) __STR(x)

#define DEFINE_SHM_HANDLER \
static void rtai_shm_handler(void) \
{ \
	__asm__ __volatile__ (" \
	cld; pushl %es; pushl %ds; pushl %ebp;\n\t \
	pushl %edi; pushl %esi; pushl %ecx;\n\t \
	pushl %ebx; pushl %edx; pushl %eax;\n\t \
	movl $" STR(__KERNEL_DS) ",%ebx; mov %bx,%ds; mov %bx,%es"); \
	__asm__ __volatile__ ("call "SYMBOL_NAME_STR(shm_handler)); \
	__asm__ __volatile__ (" \
	addl $8,%esp; popl %ebx; popl %ecx; popl %esi;\n\t \
	popl %edi; popl %ebp; popl %ds; popl %es; iret"); \
}

#endif 

#endif 
