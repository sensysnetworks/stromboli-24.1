/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it),
extensions for user space modules are jointly copyrighted (2000) with:
			Pierre Cloutier (pcloutier@poseidoncontrols.com),
			Steve Papacharalambous (stevep@zentropix.com).

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

#include <asm/rtai_vectors.h>

#define LOW  0
#define HIGH 1

#define HARD_LXRT

union rtai_lxrt_t { RTIME rt; int i[2]; void *v[2]; };

#ifndef __KERNEL__

static union rtai_lxrt_t _rtai_lxrt(int srq, void *arg) __attribute__ ((__unused__));
static union rtai_lxrt_t _rtai_lxrt(int srq, void *arg)
{
	union rtai_lxrt_t retval;
	RTAI_DO_TRAP(RTAI_LXRT_VECTOR,retval,srq,arg);
	return retval;
}

#ifdef CONFIG_SMP
#define X86LOCK "lock ; "
#else
#define X86LOCK ""
#endif

static inline unsigned long _rtai_cmpxchgl (volatile void *ptr, unsigned long oval, unsigned long nval) {

    __asm__ __volatile__(X86LOCK "cmpxchgl %1,%2"
			 : "=a"(oval)
			 : "q"(nval), "m" (*((struct { unsigned long a[100]; } *)(ptr))), "0"(oval)
			 : "memory");
    return oval;
}

#else

#define __STR(x) #x
#define STR(x) __STR(x)

#define my_switch_to(prev,next,last) do {				\
	asm volatile("pushl %%esi\n\t"					\
		     "pushl %%edi\n\t"					\
		     "pushl %%ebp\n\t"					\
		     "pushl %%eax\n\t"					\
		     "pushl %%edx\n\t"					\
		     "movl %%esp,%0\n\t"	/* save ESP */		\
		     "movl %3,%%esp\n\t"	/* restore ESP */	\
		     "movl $1f,%1\n\t"		/* save EIP */		\
		     "pushl %4\n\t"		/* restore EIP */	\
		     "jmp *"SYMBOL_NAME_STR(rthal + 4)"\n"		\
		     "1:\t"						\
		     "popl %%edx\n\t"					\
		     "popl %%eax\n\t"					\
		     "popl %%ebp\n\t"					\
		     "popl %%edi\n\t"					\
		     "popl %%esi\n\t"					\
		     :"=m" (prev->thread.esp),"=m" (prev->thread.eip),	\
		      "=b" (last)					\
		     :"m" (next->thread.esp),"m" (next->thread.eip),	\
		      "a" (prev), "d" (next),				\
		      "b" (prev));					\
} while (0)

#define RTAI_LXRT_HANDLER rtai_lxrt_handler

extern struct global_rt_status global;

#ifdef CONFIG_RTAI_TRACE
/****************************************************************************/
/* Trace functions. These functions have to be used rather than insert
the macros as-is. Otherwise the system crashes ... You've been warned. K.Y. */
extern void trace_true_lxrt_rtai_syscall_entry(void);
extern void trace_true_lxrt_rtai_syscall_exit(void);
/****************************************************************************/
#endif

extern volatile unsigned long lxrt_hrt_flags;

static inline void rtai_lxrt_handler_asm(void)
{
	int ptr;
        __asm__ __volatile__ (
        	"pushl %%eax\n\t"
        	"cld\n\t"
		"pushl %%es\n\t"
		"pushl %%ds\n\t"
		"pushl %%eax\n\t"
		"pushl %%ebp\n\t"
		"pushl %%edi\n\t"
		"pushl %%esi\n\t"
		"pushl %%edx\n\t"
		"pushl %%ecx\n\t"
        	"pushl %%ebx\n\t"
		"pushl %%edx\n\t"
		"pushl %%eax\n\t"
		"movl $" STR(__KERNEL_DS) ",%%ebx\n\t"
		"mov %%bx,%%ds\n\t"
		"mov %%bx,%%es\n\t"
#ifdef CONFIG_RTAI_TRACE
		"call "SYMBOL_NAME_STR(trace_true_lxrt_rtai_syscall_entry)"\n\t"
#endif
		"call "SYMBOL_NAME_STR(lxrt_handler)"\n\t"
		"addl $8,%%esp;\n\t"
		"movl %%edx,8(%%esp);\n\t"
		"movl %%eax,24(%%esp);\n\t"
#ifdef CONFIG_RTAI_TRACE
		"call "SYMBOL_NAME_STR(trace_true_lxrt_rtai_syscall_exit)";\n\t"
#endif
		::);
#ifdef CONFIG_RTAI_ADEOS
	{
	static int lxrt_test_context(void);
	ptr = lxrt_test_context();
	}
#else /* CONFIG_RTHAL */
	ptr = test_bit(hard_cpu_id(), &lxrt_hrt_flags) || !test_bit(hard_cpu_id(), &global.used_by_linux);
#endif /* CONFIG_RTAI_ADEOS */
        __asm__ __volatile__ (
        	"testl %%eax,%%eax;\n\t"
		"jz 1f\n\t"
		"popl %%ebx\n\t"
		"popl %%ecx\n\t"
        	"popl %%edx\n\t"
		"popl %%esi\n\t"
		"popl %%edi\n\t"
		"popl %%ebp\n\t"
        	"popl %%eax\n\t"
		"popl %%ds\n\t"
		"popl %%es\n\t"
		"addl $4,%%esp\n\t"
		"iret\n\t"
        	"1:call *" SYMBOL_NAME_STR(rthal + 16) "\n\t"
        	"jmp *" SYMBOL_NAME_STR(rthal)
	        : : "a" (ptr));
}

#ifdef CONFIG_RTAI_ADEOS
#define DEFINE_LXRT_HANDLER \
static int lxrt_test_context(void) { \
    return test_bit(hard_cpu_id(), &lxrt_hrt_flags) || test_bit(hard_cpu_id(), &arti_cpu_realtime); \
} \
static void rtai_lxrt_handler(void) { \
    rtai_lxrt_handler_asm(); \
}
#else /* CONFIG_RTHAL */
#define DEFINE_LXRT_HANDLER \
static void rtai_lxrt_handler(void) { rtai_lxrt_handler_asm(); }
#endif /* CONFIG_RTAI_ADEOS */

#define LXRT_LINUX_SYSCALL_TRAP lxrt_linux_syscall_handler

#define DEFINE_LXRT_SYSCALL_HANDLER \
static void lxrt_linux_syscall_handler(void) \
{ \
	__asm__ __volatile__ ( \
		"push $0\n\t" \
        	"cld; pushl %es; pushl %ds; pushl %ebp\n\t" \
        	"pushl %edi; pushl %esi; pushl %ecx\n\t" \
        	"pushl %ebx; pushl %edx; pushl %eax\n\t" \
        	"movl $" STR(__KERNEL_DS) ",%ebx; mov %bx,%ds; mov %bx,%es\n\t" \
        	"movl " SYMBOL_NAME_STR(linux_syscall_handler) ",%ebx\n\t" \
        	"movl %ebx,36(%esp)" ); \
        __asm__ __volatile__ ("call "SYMBOL_NAME_STR(inhrtp)); \
        __asm__ __volatile__ ( \
        	"testl %eax,%eax; popl %eax; popl %edx;\n\t" \
        	"popl %ebx; popl %ecx; popl %esi; popl %edi\n\t" \
        	"popl %ebp; popl %ds; popl %es\n\t" \
        	"jz 1f; addl $4,%esp; movl $-38,%eax; iret; 1: ret"); \
}

#endif

#endif
