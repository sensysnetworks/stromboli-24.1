
/*
 *  * (C) Pierre Cloutier <pcloutier@PoseidonControls.com> LGPLv2
 *
 *  Adapted Linux floating point routines to RTAI.
 *
 */

/*
 *  linux/arch/i386/kernel/i387.c
 *
 *  Copyright (C) 1994 Linus Torvalds
 *
 *  Pentium III FXSR, SSE support
 *  General FPU state handling cleanups
 *	Gareth Hughes <gareth@valinux.com>, May 2000
 */

#ifndef RTAI_FPU_H
#define RTAI_FPU_H

#include <linux/config.h>

#include <asm/processor.h>
#include <asm/i387.h>

#ifdef CONFIG_RTAI_FPU_SUPPORT

#if defined(CONFIG_X86_FXSR)
#define HAVE_FXSR 1
#elif defined(CONFIG_X86_RUNTIME_FXSR)
#define HAVE_FXSR (cpu_has_fxsr)
#else
#define HAVE_FXSR 0
#endif

#define RT_USEDFPU  0x00000010  /* task used FPU during this slice */
#define RT_NEEDFPU  0x00000001  /* task needs FPU */

/*
 * The _current_ task is using the FPU for the first time
 * so initialize it and set the mxcsr to its default
 * value at reset if we support FXSR and then
 * remeber the current task has used the FPU.
 */

static inline void rt_init_fpu(RT_TASK *pt)
{
	__asm__("fninit");
	if ( HAVE_FXSR )
		load_mxcsr(0x1f80);
		
	pt->uses_fpu = RT_NEEDFPU;
}

/*
 * FPU lazy state save handling.
 */

static inline void rt_save_init_fpu(RT_TASK *pt )
{
	if ( HAVE_FXSR ) {
		asm volatile( "fxsave %0 ; fnclex"
			      : "=m" (pt->fpu_reg.fxsave) );
	} else {
		asm volatile( "fnsave %0 ; fwait"
			      : "=m" (pt->fpu_reg.fsave) );
	}
	pt->uses_fpu &= ~RT_USEDFPU;
	stts();
}

static inline void rt_restore_fpu( RT_TASK *pt )
{
	if ( HAVE_FXSR ) {
		asm volatile( "fxrstor %0"
			      : : "m" (pt->fpu_reg.fxsave) );
	} else {
		asm volatile( "frstor %0"
			      : : "m" (pt->fpu_reg.fsave) );
	}
}

#define save_fpenv(x)    rt_save_init_fpu(x)
#define restore_fpenv(x) rt_restore_fpu(x)

static inline void save_init_linux_fpu( struct task_struct *tsk )
{
	if ( HAVE_FXSR ) {
        asm volatile( "fxsave %0 ; fnclex"
                  : "=m" (tsk->thread.i387.fxsave) );
    } else {
        asm volatile( "fnsave %0 ; fwait"
                  : "=m" (tsk->thread.i387.fsave) );
    }
    tsk->flags &= ~PF_USEDFPU;
    stts();
}

static inline void init_linux_fpu(struct task_struct *pt)
{
	__asm__("fninit");
	if ( HAVE_FXSR )
		load_mxcsr(0x1f80);

	pt->used_math = 1;
}

static void restore_linux_fpu( struct task_struct *tsk )
{
	if ( HAVE_FXSR ) {
		asm volatile( "fxrstor %0" :: "m" (tsk->thread.i387.fxsave) );
	} else {
		asm volatile( "frstor %0" :: "m" (tsk->thread.i387.fsave) );
	}
}

#define unlazy_linux_fpu( tsk ) do { \
	if ( tsk->flags & PF_USEDFPU ) \
		save_init_linux_fpu( tsk ); \
	} while (0)

#endif // CONFIG_RTAI_FPU_SUPPORT

#endif // RTAI_FPU_H
