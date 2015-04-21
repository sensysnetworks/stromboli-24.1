/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)

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
ACKNOWLEDGMENTS:
4/12/01, Jan Kiszka (Jan.Kiszka@web.de) for adding support to non 0/1 apics.
*/


#ifndef _RTAI_H_
#define _RTAI_H_

#include <config.h>

//#define OLD_FPU_SUP

#ifdef __KERNEL__
#include <linux/smp.h>
#include <linux/mm.h>
#include <asm/system.h>
#include <asm/desc.h>
#include <asm/io.h>
#endif
#include <asm/ptrace.h>

#include <asm/rtai_vectors.h>
#include <rtai_types.h>

#define FREQ_8254 1193180
#define LATENCY_8254 4700 
#define SETUP_TIME_8254	2011

// APIC frequency calibration
#define FREQ_APIC (tuned.apic_freq)
#define LATENCY_APIC 3944
#define SETUP_TIME_APIC 1000
#define CALIBRATED_APIC_FREQ    0 // Use this if you know better than the APIC!
/*
 * CPU frequency calibration - This is different for machine without TSC (486)
 */
#ifdef CONFIG_X86_TSC
#define CPU_FREQ (tuned.cpu_freq)
#define CALIBRATED_CPU_FREQ  0 // Use this if you know better than Linux!
#else
#define EMULATE_TSC
#define CPU_FREQ             FREQ_8254
#define CALIBRATED_CPU_FREQ  FREQ_8254
#endif

#define IFLAG  9

#define TIMER_8254_IRQ  0x00

#define RTAI_1_IPI  6
#define RTAI_2_IPI  7
#define RTAI_3_IPI  8
#define RTAI_4_IPI  9

#define RTAI_1_VECTOR  RTAI_APIC1_VECTOR
#define RTAI_2_VECTOR  RTAI_APIC2_VECTOR
#define RTAI_3_VECTOR  RTAI_APIC3_VECTOR
#define RTAI_4_VECTOR  RTAI_APIC4_VECTOR

#define RT_TIME_END 0x7FFFFFFFFFFFFFFFLL

#define RT_BUG() do { hard_cli(); BUG(); } while(0)

struct apic_timer_setup_data { int mode, count; };

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// If NR_RT_CPUS > 8 RTAI must be changed as it cannot use APIC flat delivery
// and the way processor[?].intr_flag is used must be changed (right now it
// exploits the fact that the IF flags is at bit 9 so that bits 0-7 are used
// to mark a cpu as Linux soft irq enabled/disabled. Bad but comfortable, it 
// will take a very very long time before I'll have available SMP with more
// than 8 cpus. Right now they are only NR_RT_CPUS defined below.
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#ifdef CONFIG_SMP
#define NR_RT_CPUS CONFIG_RTAI_CPUS
#else
#define NR_RT_CPUS  1
#endif

#define RTAI_NR_TRAPS 32

#ifdef __KERNEL__

struct global_rt_status {
        volatile unsigned int pending_irqs;
        volatile unsigned int activ_irqs;
	volatile unsigned int pending_srqs;
	volatile unsigned int activ_srqs;
        volatile unsigned int cpu_in_sti;
        volatile unsigned int used_by_linux;
        volatile unsigned int locked_cpus;
        volatile unsigned int hard_nesting;
        volatile unsigned int hard_lock_all_service;
#ifdef CONFIG_X86_REMOTE_DEBUG
	volatile unsigned int used_by_gdbstub;
#endif
        spinlock_t hard_lock;
        spinlock_t data_lock;
};

/*
///////////////////////////////////////////////////////////////////////////
// Brute force and total ignorance trace tool kit.
extern void rt_toggle_leds(int l);
extern void rt_reset_leds(int l);
extern void rt_set_leds(int l);
extern int  rt_get_leds(void);
///////////////////////////////////////////////////////////////////////////
*/

extern unsigned volatile int *locked_cpus;

extern void send_ipi_shorthand(unsigned int shorthand, int irq);
extern void send_ipi_logical(unsigned long dest, int irq);
extern int rt_assign_irq_to_cpu(int irq, unsigned long cpus_mask);
extern int rt_reset_irq_to_sym_mode(int irq);
extern int  rt_request_global_irq(unsigned int irq, void (*handler)(void));
extern int  rt_request_global_irq_ext(unsigned int irq, void (*handler)(void), unsigned long data);
extern void rt_set_global_irq_ext(unsigned int irq, int ext, unsigned long data);
extern int  rt_free_global_irq(unsigned int irq);
extern unsigned int rt_startup_irq(unsigned int irq);
extern void rt_shutdown_irq(unsigned int irq);
extern void rt_enable_irq(unsigned int irq);
extern void rt_disable_irq(unsigned int irq);
extern void rt_mask_and_ack_irq(unsigned int irq);
extern void rt_ack_irq(unsigned int irq);
extern void rt_unmask_irq(unsigned int irq);
extern int rt_request_linux_irq(unsigned int irq,
	void (*linux_handler)(int irq, void *dev_id, struct pt_regs *regs), 
	char *linux_handler_id, void *dev_id);
extern int rt_free_linux_irq(unsigned int irq, void *dev_id);
extern void rt_pend_linux_irq(unsigned int irq);
extern int rt_request_srq(unsigned int label, void (*rtai_handler)(void), long long (*user_handler)(unsigned int whatever));
extern int rt_free_srq(unsigned int srq);
extern struct desc_struct rt_set_full_intr_vect(unsigned int vector, int type, int dpl, void (*handler)(void));
extern void rt_reset_full_intr_vect(unsigned int vector, struct desc_struct idt_element);
extern void *rt_set_intr_handler(unsigned int vector, void (*handler)(void));
extern void rt_reset_intr_handler(unsigned int vector, void (*handler)(void));
extern void rt_do_irq(unsigned int vector);
extern void rt_pend_linux_srq(unsigned int srq);
extern int rt_request_cpu_own_irq(unsigned int irq, void (*handler)(void));
extern int rt_free_cpu_own_irq(unsigned int irq);
extern int rt_request_timer(void (*handler)(void), unsigned int tick, int apic);
extern void rt_request_timer_cpuid(void (*handler)(void), unsigned int tick, int cpuid);
extern void rt_free_timer(void);
extern void rt_request_apic_timers(void (*handler)(void), struct apic_timer_setup_data *apic_timer_data);
extern void rt_free_apic_timers(void);
extern void rt_mount_rtai(void);
extern void rt_umount_rtai(void);
extern int rt_printk(const char *format, ...);
extern void ll2a(long long ll, char *s);

extern int rtai_print_to_screen(const char *format, ...);
extern int rt_is_lxrt(void);

// The next 3 should probably go inside INTERFACE_TO_LINUX  
// PC: Can someone explain why we have INTERFACE_TO_LINUX ?
extern int rt_is_linux(void);
extern RT_TRAP_HANDLER rt_set_rtai_trap_handler(RT_TRAP_HANDLER handler);
extern struct task_struct *rt_whoislinux(int cpuid);

#ifdef INTERFACE_TO_LINUX
extern void rt_switch_to_linux(int cpuid);
extern void rt_switch_to_real_time(int cpuid);
#endif

#ifdef CONFIG_SMP

#define STAGGER(x) do { int i = 0; do { nop(); } while (++i < x); } while (0)

#define rt_spin_lock(lock)    spin_lock((lock))

#define rt_spin_unlock(lock)  spin_unlock((lock))

#ifdef CONFIG_RTAI_STRANGE_APIC
static inline unsigned int hard_cpu_id(void)
{
	extern struct rt_hal rthal;
	return rthal.apicmap[GET_APIC_ID(apic_read(APIC_ID))];
}
#else
#define hard_cpu_id()  hard_smp_processor_id() 
#endif

static inline void rt_get_global_lock(void)
{
	hard_cli();
	if (!test_and_set_bit(hard_cpu_id(), locked_cpus)) {
		while (test_and_set_bit(31, locked_cpus)) {
#ifdef STAGGER
			STAGGER(hard_cpu_id());
#endif
		}
	}
}

static inline void rt_release_global_lock(void)
{
	hard_cli();
	if (test_and_clear_bit(hard_cpu_id(), locked_cpus)) {
		test_and_clear_bit(31, locked_cpus);
#ifdef STAGGER
			STAGGER(hard_cpu_id());
#endif
	}
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

static inline unsigned long rt_spin_lock_irqsave(spinlock_t *lock)          
{
	unsigned int flags;
	hard_save_flags_and_cli(flags);
	rt_spin_lock(lock);
	return flags;
}

static inline void rt_spin_unlock_irqrestore(unsigned int flags,
						spinlock_t *lock)
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

static inline unsigned long rt_global_save_flags_and_cli(void)
{
	unsigned long flags;

	hard_save_flags_and_cli(flags);
	if (!test_and_set_bit(hard_cpu_id(), locked_cpus)) {
		while (test_and_set_bit(31, locked_cpus)) {
#ifdef STAGGER
			STAGGER(hard_cpu_id());
#endif
		}
		return ((flags & (1 << IFLAG)) | 1);
	} else {
		return (flags & (1 << IFLAG));
	}
}

static inline void rt_global_save_flags(unsigned long *flags)
{
	unsigned long hflags, rflags;

	hard_save_flags_and_cli(hflags);
	hflags &= (1 << IFLAG);
	if (test_bit(hard_cpu_id(), locked_cpus)) {
		rflags = hflags;
	} else {
		rflags = hflags | 1;
	}
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

#else

#define rt_spin_lock(lock)  
#define rt_spin_unlock(lock)

#define rt_get_global_lock()      do { hard_cli(); } while (0)
#define rt_release_global_lock()  

#define rt_spin_lock_irq(lock)    do { hard_cli(); } while (0)
#define rt_spin_unlock_irq(lock)  do { hard_sti(); } while (0) 

static inline unsigned long rt_spin_lock_irqsave(spinlock_t *lock)
{
	unsigned long flags;
	hard_save_flags_and_cli(flags);
	return flags;
}          

#define rt_spin_unlock_irqrestore(flags, lock)  do { hard_restore_flags(flags); } while (0)

#define rt_global_cli()  do { hard_cli(); } while (0)

#define rt_global_sti()  do { hard_sti(); } while (0) 

static inline unsigned long rt_global_save_flags_and_cli(void)
{
	unsigned long flags;
	hard_save_flags_and_cli(flags);
	return flags;
}          

#define rt_global_save_flags(flags)  do { hard_save_flags(*flags); } while (0)

#define rt_global_restore_flags(flags)  do { hard_restore_flags(flags); } while (0)

#define hard_cpu_id()  0

//extern unsigned long cpu_online_map;

#endif

static inline unsigned long get_cr2(void)
{
	unsigned long address;
	__asm__("movl %%cr2,%0":"=r" (address));
	return address;
}

// macros to support hard floating point in interrupt handlers:
// typical sequence to be used:

// unsigned long cr0;
// unsigned long lfpe[27], tfpe[27];

// save_cr0_and_clts(cr0);  # To save Linux cr0 state. Always to be done.
// save_fpenv(lfpe);        # To save Linux fpu environment. Needed only if 
//                          # any Linux process uses the fpu.
// restore_fpenv(tfpe);     # To restore your fpu environment. Needed only if
//			    # it can be interrupted or if you left some
//			    # intermediate results in it.
//
// PUT HERE ALL YOUR INTERRUPT SERVICE ROUTINE FLOATING POINT CALCULATIONS.
//
// save_fpenv(tfpe);     # To save your fpu environment. Needed only if there 
//			 # is a suspect that any intermediate result, to be 
//			 # used at the next interrupts service, can be left
//			 # in it. It should never happen, but maybe the
//			 # compiler can do strange things while optimizing.
// restore_fpenv(lfpe);	 # To restore a previously saved Linux fpu environment.
// restore_cr0(cr0);	 # To restore Linux cr0. Always to be done.
		 
typedef union i387_union FPU_ENV;
   
#ifdef CONFIG_RTAI_FPU_SUPPORT

#define load_mxcsr(val) \
	do { \
        	unsigned long __mxcsr = ((unsigned long)(val) & 0xffbf); \
	        __asm__ __volatile__ ("ldmxcsr %0": : "m" (__mxcsr)); \
	} while (0)

#define save_cr0_and_clts(x) \
	do { \
		__asm__ __volatile__ ("movl %%cr0,%0; clts": "=r" (x)); \
	} while (0)

#define restore_cr0(x) \
	do { \
		if (x & 8) { \
			__asm__ __volatile__ ("movl %%cr0, %0": "=r" (x)); \
			__asm__ __volatile__ ("movl %0, %%cr0": :"r" (8 | x)); \
		} \
	} while (0)

#define enable_fpu() \
	do { \
		__asm__ __volatile__ ("clts"); \
	} while (0)

#ifdef OLD_FPU_SUP

#define init_xfpu()

#define save_fpenv(x) \
	do { \
		__asm__ __volatile__ ("fnsave %0": "=m" (x)); \
	} while (0)

#define restore_fpenv(x) \
	do { \
		__asm__ __volatile__ ("frstor %0": : "m" (x)); \
	} while (0)

static inline void restore_fpenv_lxrt(struct task_struct *tsk)
{
	__asm__ __volatile__ ("clts; frstor %0": : "m" (tsk->thread.i387.fsave));
}

#else

#define init_xfpu() \
	do { \
		__asm__ __volatile__ ("clts; fninit"); \
		if (cpu_has_xmm) { \
			load_mxcsr(0x1f80); \
		} \
	} while (0)

#define save_fpenv(x) \
	do { \
		if (cpu_has_fxsr) { \
			__asm__ __volatile__ ("fxsave %0; fnclex": "=m" (x)); \
		} else { \
			__asm__ __volatile__ ("fnsave %0; fwait": "=m" (x)); \
		} \
	} while (0)

#define restore_fpenv(x) \
	do { \
		if (cpu_has_fxsr) { \
			__asm__ __volatile__ ("fxrstor %0": : "m" (x)); \
		} else { \
			__asm__ __volatile__ ("frstor %0": : "m" (x)); \
		} \
	} while (0)

static inline void restore_fpenv_lxrt(struct task_struct *tsk)
{
	if (cpu_has_fxsr) {
		__asm__ __volatile__ ("clts; fxrstor %0": : "m" (tsk->thread.i387.fxsave));
	} else {
		__asm__ __volatile__ ("clts; frstor %0": : "m" (tsk->thread.i387.fsave));
	}
}

#endif

#else // !CONFIG_RTAI_FPU_SUPPORT
/* For a while, the FPU_ENV was defined to be empty, but that makes the
 * ABI (specifically, the size of RT_TASK) dependent on whether you include
 * config.h.  Since people probably don't do that correctly, and this is
 * the only place where it matters, we won't surprise them.  */
#define save_cr0_and_clts(x)
#define restore_cr0(x)
#define enable_fpu()
#define load_mxcsr(val)
#define init_xfpu()
#define save_fpenv(x)
#define restore_fpenv(x)
#endif // CONFIG_RTAI_FPU_SUPPORT


struct rt_smp_times {  int linux_tick,
                       periodic_tick;
		 RTIME tick_time, 
		       linux_time,
		       intr_time; };

struct calibration_data {
	unsigned long cpu_freq;
	unsigned int apic_freq;
	int latency;
	int setup_time_TIMER_CPUNIT;
	int setup_time_TIMER_UNIT;
	int timers_tol[NR_RT_CPUS];
};

extern struct rt_times rt_times;
extern struct rt_times rt_smp_times[NR_RT_CPUS];
extern struct calibration_data tuned;

extern RTIME rd_8254_ts(void);
extern void rt_setup_8254_tsc(void);

#endif

#undef rdtsc

#ifdef EMULATE_TSC
#  define rdtsc() rd_8254_ts()
#  define TICK_8254_TSC_EMULATION  do { rdtsc(); } while (0)
#  ifdef CONFIG_VT /* prevent kd_mksound from using the 8254 */
#    define DECLR_8254_TSC_EMULATION \
			extern void *kd_mksound; \
			static void *linux_mksound; \
			static void rtai_mksound(void) { }
#    define SETUP_8254_TSC_EMULATION \
			do { \
			    linux_mksound = kd_mksound; \
			    kd_mksound = rtai_mksound; \
			    rt_setup_8254_tsc(); \
			} while (0)
#    define CLEAR_8254_TSC_EMULATION \
			do { \
			    if (linux_mksound) kd_mksound = linux_mksound; \
			} while (0)
#  else /* no CONFIG_VT, no kd_mksound */
#    define DECLR_8254_TSC_EMULATION /* nothing */
#    define SETUP_8254_TSC_EMULATION rt_setup_8254_tsc()
#    define CLEAR_8254_TSC_EMULATION /* nothing */
#  endif /* CONFIG_VT */
#else /* use real TSC */
#  define rdtsc() rd_CPU_ts()
#  define DECLR_8254_TSC_EMULATION
#  define TICK_8254_TSC_EMULATION
#  define SETUP_8254_TSC_EMULATION
#  define CLEAR_8254_TSC_EMULATION
#endif /* EMULATE_TSC */

// get the time stamp clock (TSC) of a cpu.
static inline unsigned long long rd_CPU_ts(void)
{
	unsigned long long time;
	__asm__ __volatile__( "rdtsc" : "=A" (time));
	return time;
}

// returns (int)i = (int)i*(int)(mult)/(int)div.
static inline int imuldiv(int i, int mult, int div)
{
	int dummy;
       __asm__ __volatile__ (\
	       "mull %%edx; div %%ecx"
#if 0
	       "mull %%edx; div %%ecx; sal $1,%%edx\n\t"
	       "cmpl %%edx,%%ecx; jge 1f; incl %%eax; 1:"
#endif
               : "=a" (i), "=d" (dummy)
       	       : "a" (i), "d" (mult), "c" (div));
       return i;
}


/*
 * ULLDIV as modified by Marco Morandini (morandini@aero.polimi.it) to work 
 * with gcc-3.x.x even if the option -fnostrict-aliasing is forgotten, 
 * otherwise it does not work properly with -O2. RTAI making should enforce 
 * -fnostrict-aliasing always so it should be just in case one uses it on
 * his/her own.
 */
#define __BIGENDIAN__
#ifdef  __BIGENDIAN__
	#define LOW  0
	#define HIGH 1 
#else
	#define LOW  1
	#define HIGH 0  
#endif

static inline unsigned long long ulldiv(unsigned long long ull, unsigned long uld, unsigned long *r)
{
	unsigned long long qf, rf;
	unsigned long tq, rh;
	union { unsigned long long ull; unsigned long ul[2]; } p, q;

	p.ull = ull;
	q.ull = 0;
	rf = 0x100000000ULL - (qf = 0xFFFFFFFFUL / uld) * uld;
	while (p.ull >= uld) {
		q.ul[HIGH] += (tq = p.ul[HIGH] / uld);
		rh = p.ul[HIGH] - tq * uld;
		q.ull  += rh * qf + (tq = p.ul[LOW] / uld);
		p.ull   = rh * rf + (p.ul[LOW] - tq * uld);
	}
	*r = p.ull;
	return q.ull;
}

/*
 *  u64div32c.c is a helper function provided, 2003-03-03, by:
 *  Copyright (C) 2003 Nils Hagge <hagge@rts.uni-hannover.de>
 */

static inline unsigned long long u64div32c(unsigned long long a, unsigned long b, int *r)
{
  __asm__ __volatile(
    "\n        movl    %%eax,%%ebx"
    "\n        movl    %%edx,%%eax"
    "\n        xorl    %%edx,%%edx"
    "\n        divl    %%ecx"
    "\n        xchgl   %%eax,%%ebx"
    "\n        divl    %%ecx"
    "\n        movl    %%edx,%%ecx"
    "\n        movl    %%ebx,%%edx"
    : "=a" (((unsigned long *)&a)[0]), "=d" (((unsigned long *)&a)[1])
    : "a" (((unsigned long *)&a)[0]), "d" (((unsigned long *)&a)[1]), "c" (b)
    : "%ebx"
  );
  return a;
}

#if 0
static inline unsigned long long ulldiv(unsigned long long ull, unsigned long uld, unsigned long *r)
{
	unsigned long long q, rf;
	unsigned long qh, rh, ql, qf;

	q = 0;
	rf = (unsigned long long)(0xFFFFFFFF - (qf = 0xFFFFFFFF / uld) * uld) + 1ULL;

	while (ull >= uld) {
		((unsigned long *)&q)[1] += (qh = ((unsigned long *)&ull)[1] / uld);
		rh = ((unsigned long *)&ull)[1] - qh * uld;
		q += rh * (unsigned long long)qf + (ql = ((unsigned long *)&ull)[0] / uld);
		ull = rh * rf + (((unsigned long *)&ull)[0] - ql * uld);
	}

	*r = ull;
	return q;
}
#endif

// returns (long long)ll = (int)ll*(int)(mult)/(int)div.
static inline long long llimd(long long ll, int mult, int div)
{
	__asm__ __volatile (\
		 "movl %%edx,%%ecx; mull %%esi;       movl %%eax,%%ebx;  \n\t"
	         "movl %%ecx,%%eax; movl %%edx,%%ecx; mull %%esi;        \n\t"
		 "addl %%ecx,%%eax; adcl $0,%%edx;    divl %%edi;        \n\t"
	         "movl %%eax,%%ecx; movl %%ebx,%%eax; divl %%edi;        \n\t"
		 "sal $1,%%edx;     cmpl %%edx,%%edi; movl %%ecx,%%edx;  \n\t"
		 "jge 1f;           addl $1,%%eax;    adcl $0,%%edx;     1:"
		 : "=A" (ll)
		 : "A" (ll), "S" (mult), "D" (div)
		 : "%ebx", "%ecx");
	return ll;
}

// copied from Linux ffz (simply by taking ~ away).
static __inline__ unsigned long ffnz(unsigned long word)
{
	__asm__("bsfl %1, %0"
		: "=r" (word)
		: "r"  (word));
	return word;
}

#define SAVE_IRQ_REG __asm__ __volatile__ (" \
	cld; pushl %es; pushl %ds; pushl %eax;\n\t \
	pushl %ebp; pushl %ecx; pushl %edx;\n\t \
	movl $" STR(__KERNEL_DS) ",%edx; mov %dx,%ds; mov %dx,%es")

#define RSTR_IRQ_REG __asm__ __volatile__ (" \
	popl %edx; popl %ecx; popl %ebp; popl %eax;\n\t \
	popl %ds; popl %es; iret")

#define SAVE_FULL_IRQ_REG __asm__ __volatile__ (" \
	cld; pushl %es; pushl %ds; pushl %eax;\n\t \
	pushl %ebp; pushl %edi; pushl %esi;\n\t \
	pushl %edx; pushl %ecx; pushl %ebx;\n\t \
	movl $" STR(__KERNEL_DS) ",%edx; mov %dx,%ds; mov %dx,%es")

#define RSTR_FULL_IRQ_REG __asm__ __volatile__ (" \
	popl %ebx; popl %ecx; popl %edx; popl %esi; popl %edi;\n\t \
	popl %ebp; popl %eax; popl %ds; popl %es; iret")

#define SAVE_SRQ_REG __asm__ __volatile__ (" \
	cld; pushl %es; pushl %ds; pushl %ebp;\n\t \
	pushl %edi; pushl %esi; pushl %ecx;\n\t \
	pushl %ebx; pushl %edx; pushl %eax;\n\t \
	movl $" STR(__KERNEL_DS) ",%ebx; mov %bx,%ds; mov %bx,%es")

#define RSTR_SRQ_REG __asm__ __volatile__ (" \
	popl %eax; popl %edx; popl %ebx; popl %ecx; popl %esi;\n\t \
	popl %edi; popl %ebp; popl %ds; popl %es; iret")

#define rt_set_timer_delay(x) do  {\
	if (x) { \
		outb(x & 0xFF, 0x40);  \
		outb(x >> 8, 0x40);    \
	} } while(0)

#endif
