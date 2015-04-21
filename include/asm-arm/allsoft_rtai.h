/* 020222 asm-arm/rtai.h
COPYRIGHT (C) 2002 Thomas Gleixner, autronix automation (gleixner@autronix.de) 
COPYRIGHT (C) 2002 Guennadi Liakhovetski, DSA GmbH (gl@dsa-ac.de)
COPYRIGHT (C) 2002 Wolfgang Müller (wolfgang.mueller@dsa-ac.de)
Copyright (c) 2001 Alex Züpke, SYSGO RTS GmbH (azu@sysgo.de)

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
--------------------------------------------------------------------------
Acknowledgements
- Paolo Mantegazza	(mantegazza@aero.polimi.it)
	creator of RTAI 
*/

#ifndef _RTAI_H_
#define _RTAI_H_

#include <asm/rtai_debug.h> /* Keep it first */
#include <rtai_types.h>
#include <asm/rtai_fpu.h>

#ifdef CONFIG_RTAI_ALLSOFT

extern void hard_cli(void);
extern void hard_sti(void);
extern void hard_restore_flags(unsigned long flags);
extern unsigned long __hard_save_flags(void);
extern unsigned long __hard_save_flags_cli(void);
#define hard_save_flags(flags) \
        do { flags = __hard_save_flags(); } while (0)
#define hard_save_flags_cli(flags) \
        do { flags = __hard_save_flags_cli(); } while (0)

#else

#define hard_cli                 HARD_CLI
#define hard_sti                 HARD_STI
#define hard_restore_flags       HARD_RESTORE_FLAGS
#define hard_save_flags          HARD_SAVE_FLAGS
#define hard_save_flags_cli      HARD_SAVE_FLAGS_CLI

#endif

#define rt_printk_srq		1

#define NR_GLOBAL_IRQS		NR_IRQS
#define NR_CPU_OWN_IRQS		NR_GLOBAL_IRQS

#define RTAI_1_IPI  -1 /* Just to make things compile */
#define RTAI_2_IPI  -1
#define RTAI_3_IPI  -1
#define RTAI_4_IPI  -1

// Do not be messed up by macros names below, is a trick for keeping i386 code.
#define FREQ_8254       FREQ_SYS_CLK
#define LATENCY_8254    LATENCY_MATCH_REG
#define SETUP_TIME_8254 SETUP_TIME_MATCH_REG
#define FREQ_APIC       FREQ_SYS_CLK

//#define IFLAG 15
#define IFLAG 7
#define IBIT (1 << IFLAG)

#define RT_TIME_END 0x7FFFFFFFFFFFFFFFLL

struct desc_struct { void *fun; };

extern unsigned volatile int *locked_cpus;

#ifndef CONFIG_SMP
extern unsigned long cpu_present_map;
#define smp_num_cpus 1
#endif

#ifndef CONFIG_RTAI_FPU_SUPPORT
#define enable_fpu()
#endif

#ifdef __KERNEL__

#include <asm/io.h>
#include <asm/ptrace.h>
#include <asm/irq.h>
//#include <asm/processor.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/proc/hard_system.h>

#define hard_unlock_all(flags) hard_restore_flags((flags))
#define hard_save_flags_and_cli(x) hard_save_flags_cli((x))
#define IRQ_DESC irq_desc
extern struct rt_times rt_times;

typedef int rtai_irq_t;
typedef unsigned long long rtai_irq_mask_t;
//typedef unsigned long rtai_irq_mask_t;

extern void send_ipi_shorthand(unsigned int shorthand, rtai_irq_t irq);
extern void send_ipi_logical(unsigned long dest, rtai_irq_t irq);
#define rt_assign_irq_to_cpu(irq, cpu)
#define rt_reset_irq_to_sym_mode(irq)
extern int  rt_request_global_irq(rtai_irq_t irq, void (*handler)(void));
extern int  rt_free_global_irq(rtai_irq_t irq);
extern void rt_ack_irq(rtai_irq_t irq);
extern void rt_mask_and_ack_irq(rtai_irq_t irq);
extern void rt_unmask_irq(rtai_irq_t irq);
extern unsigned int rt_startup_irq(rtai_irq_t irq);
extern void rt_shutdown_irq(rtai_irq_t irq);
extern void rt_enable_irq(rtai_irq_t irq);
extern void rt_disable_irq(rtai_irq_t irq);
extern int rt_request_linux_irq(rtai_irq_t irq,
	void (*linux_handler)(int irq, void *dev_id, struct pt_regs *regs),
	char *linux_handler_id, void *dev_id);
extern int rt_free_linux_irq(rtai_irq_t irq, void *dev_id);
extern void rt_tick_linux_timer(void);
extern struct desc_struct rt_set_full_intr_vect(unsigned int vector, int type, int dpl, void *handler);
extern void rt_reset_full_intr_vect(unsigned int vector, struct desc_struct idt_element);
#define rt_set_intr_handler(vector, handler) ((void *)0)
#define rt_reset_intr_handler(vector, handler)
extern int rt_request_srq(unsigned int label, void (*rtai_handler)(void), long long (*user_handler)(unsigned int whatever));
extern int rt_free_srq(rtai_irq_t srq);
extern void rt_pend_linux_irq(rtai_irq_t irq);
extern void rt_pend_linux_srq(rtai_irq_t srq);
#define rt_request_cpu_own_irq(irq, handler) rt_request_global_irq((irq), (handler))
#define rt_free_cpu_own_irq(irq) rt_free_global_irq((irq))
extern void rt_request_timer(void (*handler)(void), unsigned int tick, int apic);
extern void rt_request_timer_cpuid(void (*handler)(void), unsigned int tick, int cpuid);
extern void rt_free_timer(void);
struct apic_timer_setup_data { int mode, count; };
//extern void rt_request_apic_timers(void (*handler)(void), struct apic_timer_setup_data *apic_timer_data);
//extern void rt_free_apic_timers(void);
extern void rt_mount_rtai(void);
extern void rt_umount_rtai(void);
extern int rt_printk(const char *format, ...);
extern int rtai_print_to_screen(const char *format, ...);

/* not all ARM cores support 32bit x 32bit = 64bit native ... */
static inline unsigned long long ullmul(unsigned long m0, unsigned long m1)
{
        unsigned long long res;

        res = (unsigned long long)m0 * (unsigned long long)m1;

        return res;
}

static inline unsigned long long ulldiv(unsigned long long ull, unsigned long uld, unsigned long *r)
{
	unsigned long long q = ull/(unsigned long long) uld;

	*r = (unsigned long) (ull - q * (unsigned long long) uld);

	return q;	
}

static inline int imuldiv(unsigned long i, unsigned long mult, unsigned long div)
{
	unsigned long q , r;
	unsigned long long m;

	if ( mult == div )
		return i;

	m = ((unsigned long long) i * (unsigned long long) mult);
	q = (unsigned long) (m / (unsigned long long) div);
	r = (unsigned long) (m - (unsigned long long) q * (unsigned long long) div );

	return (r + r) < div ? q : q + 1;
}

static inline unsigned long long llimd(unsigned long long ull, unsigned long mult, unsigned long div)
{
	unsigned long long low, high, q;
	unsigned long r;

	low  = ullmul(((unsigned long *)&ull)[0], mult);
	high = ullmul(((unsigned long *)&ull)[1], mult);
	q = ulldiv(high,div,&r) << 32;
	high = ((unsigned long long) r) << 32;
	q += ulldiv( high + low, div , &r);
	return (r + r) < div ? q : q + 1;
}

#define ffnz(ul) (ffs(ul)-1)

/* Timers are architecture dependent */
#include <asm/arch/rtai_arch.h>
#include <asm/arch/rtai_timer.h>

#ifdef INTERFACE_TO_LINUX
extern void rt_switch_to_linux(int cpuid);
extern void rt_switch_to_real_time(int cpuid);
#endif

extern struct rt_hal rthal;

#define rt_spin_lock(whatever)
#define rt_spin_unlock(whatever)

#define rt_get_global_lock()  hard_cli()
#define rt_release_global_lock()

#define hard_cpu_id()  0

#define NR_RT_CPUS  1

#define RTAI_NR_TRAPS 32

#define save_cr0_and_clts(x)

#define restore_cr0(x)


#ifdef CONFIG_RTAI_FPU_SUPPORT
/* someone should implement FPU support if necessary ... */
extern void save_fpenv(long *fpu_reg);
extern void restore_fpenv(long *fpu_reg);
typedef struct arm_fpu_env { unsigned long fpu_reg[1]; } FPU_ENV;
#else
typedef struct arm_fpu_env { unsigned long fpu_reg[1]; } FPU_ENV;
#endif

static inline unsigned long hard_lock_all(void)
{
	unsigned long flags;
	hard_save_flags_and_cli(flags);
	hard_clf(); /* we should block FIQs too */
	return flags;
}

static inline void rt_spin_lock_irq(spinlock_t *lock)          
{
	hard_cli(); 
	rt_spin_lock(lock);
}

static inline void rt_spin_unlock_irq(spinlock_t *lock)
{
	rt_spin_unlock(lock);
	hard_sti();
}

// Note that the spinlock calling convention below for irqsave/restore is 
// sligtly different from the one used in Linux. Done on purpose to get an 
// error if you use Linux spinlocks in real time applications as they do not
// guaranty any protection because of the soft irq disable. Be careful and
// sure to call the other spinlocks the right way, as they are compatible
// with Linux.

static inline unsigned int rt_spin_lock_irqsave(spinlock_t *lock)          
{
	unsigned long flags;
	hard_save_flags_and_cli(flags);
	rt_spin_lock(lock);
	return flags;
}

static inline void rt_spin_unlock_irqrestore(unsigned long flags, spinlock_t *lock)
{
	rt_spin_unlock(lock);
	hard_restore_flags(flags);
}

/* Global interrupts and flags control (simplified, and modified, version of */
/* similar global stuff in Linux irq.c).                                     */

static inline void rt_global_cli(void)
{
	rt_get_global_lock();
}

static inline void rt_global_sti(void)
{
	rt_release_global_lock();
	hard_sti();
}

static inline int rt_global_save_flags_and_cli(void)
{
	unsigned long flags;

	hard_save_flags_and_cli(flags);
	if (!test_and_set_bit(hard_cpu_id(), locked_cpus)) {
		while (test_and_set_bit(31, locked_cpus));
		return ((~flags & (1 << IFLAG)) | 1);
	} else {
		return (~flags & (1 << IFLAG));
	}
}

static inline void rt_global_save_flags(unsigned long *flags)
{
	unsigned long hflags, rflags;

	hard_save_flags_and_cli(hflags);
	hflags = ~hflags & (1 << IFLAG);
	rflags = hflags | !test_bit(hard_cpu_id(), &locked_cpus);
	if (hflags) {
		hard_sti();
	}
	*flags = rflags;
}

static inline void rt_global_restore_flags(unsigned long flags)
{
	switch (flags) {
		case (1 << IFLAG) | 1:	rt_release_global_lock();
		        	  	hard_sti();
					break;
		case (1 << IFLAG) | 0:	rt_get_global_lock();
				 	hard_sti();
					break;
		case (0 << IFLAG) | 1:	rt_release_global_lock();
					break;
		case (0 << IFLAG) | 0:	rt_get_global_lock();
					break;
	}
}

struct calibration_data {
	unsigned int cpu_freq;
	int latency;
	int setup_time_TIMER_CPUNIT;
	int setup_time_TIMER_UNIT;
	int timers_tol[NR_RT_CPUS];
};

extern struct rt_times rt_smp_times[NR_RT_CPUS];
extern struct calibration_data tuned;

static inline unsigned long xchg_u32(unsigned long *up, unsigned long val)
{
	unsigned long flags, ret;

	hard_save_flags_and_cli(flags);
	ret = *up;
	*up = val;
	hard_restore_flags(flags);
	return ret;
}

extern unsigned long linux_save_flags_and_cli_cpuid(int);

#define DECLR_8254_TSC_EMULATION
#define TICK_8254_TSC_EMULATION
#define SETUP_8254_TSC_EMULATION
#define CLEAR_8254_TSC_EMULATION


#endif

#endif
