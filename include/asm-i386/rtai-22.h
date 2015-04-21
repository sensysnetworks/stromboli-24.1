/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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

#ifndef _RTAI_H_
#define _RTAI_H_

#include <config.h>

#include <asm/ptrace.h>
#include <asm/spinlock.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/types.h>
#include <asm/processor.h>

#include <asm/rtai_vectors.h>
#include <rtai_types.h>

// Just in case you want to do it from scratch. You must know the 
// BUS_FREQUENCY and APIC_DIVISOR values, those below are just a sample, then
// use the formula below.
//#define BUS_FREQ 100224400
//#define APIC_DIVISOR 16
//#define CALIBRATED_APIC_FREQ ((BUS_FREQ + APIC_DIVISOR/2)/APIC_DIVISOR)

//#define CALIBRATED_CPU_FREQ	FREQ_8254  // Use this to force a 486. 
#define CALIBRATED_CPU_FREQ	0 
#define CALIBRATED_APIC_FREQ	0 


#define NANO 1000000000LL

#define FREQ_8254 1193180
#define LATENCY_8254 6000 
#define SETUP_TIME_8254	2000 

#define LATENCY_APIC 3000
#define SETUP_TIME_APIC 500  // not important

#define CPU_FREQ (tuned.cpu_freq)
#define FREQ_APIC (tuned.apic_freq)

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

#define RTAI_NR_TRAPS 32

typedef union i387_union FPU_ENV;

#include <asm/desc.h>

extern unsigned volatile int *locked_cpus;

struct apic_timer_setup_data { int mode, count; };

#ifndef CONFIG_SMP
//extern unsigned long cpu_present_map;
#endif

//#ifdef MODULE
#ifdef __KERNEL__

extern void send_ipi_shorthand(unsigned int shorthand, int irq);
extern void send_ipi_logical(unsigned long dest, int irq);
extern int rt_assign_irq_to_cpu(int irq, int cpu);
extern int rt_reset_irq_to_sym_mode(int irq);
extern int  rt_request_global_irq(unsigned int irq, void (*handler)(void));
extern int  rt_free_global_irq(unsigned int irq);
extern void rt_ack_irq(unsigned int irq);
extern void rt_mask_and_ack_irq(unsigned int irq);
extern void rt_unmask_irq(unsigned int irq);
extern void rt_startup_irq(unsigned int irq);
extern void rt_shutdown_irq(unsigned int irq);
extern void rt_enable_irq(unsigned int irq);
extern void rt_disable_irq(unsigned int irq);
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
extern void rt_do_irq(int vector);
extern void rt_pend_linux_srq(unsigned int srq);
extern int rt_request_cpu_own_irq(unsigned int irq, void (*handler)(void));
extern int rt_free_cpu_own_irq(unsigned int irq);
extern void rt_request_timer(void (*handler)(void), unsigned int tick, int apic);
extern void rt_free_timer(void);
extern void rt_request_apic_timers(void (*handler)(void), struct apic_timer_setup_data *apic_timer_data);
extern void rt_free_apic_timers(void);
extern void rt_mount_rtai(void);
extern void rt_umount_rtai(void);
extern int rtai_print_to_screen(const char *format, ...);
extern int rt_printk(const char *fmt, ...);
#define rt_print_to_screen	rtai_print_to_screen
#define hard_save_flags_and_cli(x) \
__asm__ __volatile__("pushfl; popl %0; cli": "=g" (x): :"memory")

#endif

#ifdef INTERFACE_TO_LINUX
extern void rt_switch_to_linux(int cpuid);
extern void rt_switch_to_real_time(int cpuid);
#endif

// defines for compatibility with 2.0.xx RTLinux
#define request_RTirq(irq, handler)  rt_request_global_irq(irq, handler)
#define free_RTirq(irq)              rt_free_global_irq(irq)
#define disable_RTirq(irq)           rt_disable_irq(irq)
#define enable_RTirq(irq)            rt_enable_irq(irq)

#ifdef CONFIG_SMP

// defines for compatibility with 2.0.xx RTLinux
#define r_cli()                 rt_global_cli()
#define r_sti()                 rt_global_sti()
#define r_save_flags(flags)     rt_global_save_flags(&flags)
#define r_restore_flags(flags)  rt_global_restore_flags(flags)

static volatile inline void rt_spin_lock(spinlock_t *lock)
{
	while(test_and_set_bit(0, &(lock->lock)));
}

static volatile inline void rt_spin_unlock(spinlock_t *lock)
{
 	clear_bit(0, &(lock->lock));
}

#define hard_cpu_id()  hard_smp_processor_id() 

static inline void rt_get_global_lock(void)
{
	hard_cli();
	if (!test_and_set_bit(hard_cpu_id(), locked_cpus)) {
		while (test_and_set_bit(31, locked_cpus));
	}
}

static inline void rt_release_global_lock(void)
{
	hard_cli();
	if (test_and_clear_bit(hard_cpu_id(), locked_cpus)) {
		clear_bit(31, locked_cpus);
	}
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// If NR_RT_CPUS > 8 RTAI must be changed as it cannot use APIC flat delivery
// and the way processor[?].intr_flag is used must be changed (right now it
// exploits the fact that the IF flags is at bit 9 so that bits 0-7 are used
// to mark a cpu as Linux soft irq enabled/disabled. Bad but comfortable, it 
// will take a very very long time before I'll have available SMP with more
// than 8 cpus. Right now they are only:
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define NR_RT_CPUS  2

#else

// defines for compatibility with 2.0.xx RTLinux
#define r_cli()                 hard_cli()
#define r_sti()                 hard_sti()
#define r_save_flags(flags)     hard_save_flags(flags)
#define r_restore_flags(flags)  hard_restore_flags(flags)

#define rt_spin_lock(whatever)  
#define rt_spin_unlock(whatever)

#define rt_get_global_lock()  hard_cli()
#define rt_release_global_lock()

#define hard_cpu_id()  0

#define NR_RT_CPUS  1

#endif

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
		 
#ifndef CONFIG_RTAI_FPU_SUPPORT

#define save_cr0_and_clts(x)

#define restore_cr0(x)

#define save_fpenv(x)

#define restore_fpenv(x)

#else

#define save_cr0_and_clts(x) __asm__ __volatile__ ("movl %%cr0,%0; clts" :"=r" (x))

#define restore_cr0(x)   __asm__ __volatile__ ("movl %0,%%cr0": :"r" (x))

#define save_fpenv(x)    __asm__ __volatile__("fnsave %0" : "=m" (x))

#define restore_fpenv(x) __asm__ __volatile__("frstor %0" : "=m" (x))

#endif

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
	unsigned int flags;
	hard_save_flags(flags);
	hard_cli(); 
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

static inline int rt_global_save_flags_and_cli(void)
{
	unsigned long flags;

	hard_save_flags(flags);
	hard_cli();
	if (!test_and_set_bit(hard_cpu_id(), locked_cpus)) {
		while (test_and_set_bit(31, locked_cpus));
		return ((flags & (1 << IFLAG)) + 1);
	} else {
		return (flags & (1 << IFLAG));
	}
}

static inline void rt_global_save_flags(unsigned long *flags)
{
	unsigned long hflags, rflags;

	hard_save_flags(hflags);
	hard_cli();
	hflags = hflags & (1 << IFLAG);
	rflags = hflags | !test_bit(hard_cpu_id(), locked_cpus);
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


#define RT_TIME_END 0x7FFFFFFFFFFFFFFFLL

struct rt_smp_times {  int linux_tick,
                       periodic_tick;
		 RTIME tick_time, 
		       linux_time,
		       intr_time; };

struct calibration_data {
	unsigned int cpu_freq;
	unsigned int apic_freq;
	int latency;
	int setup_time_TIMER_CPUNIT;
	int setup_time_TIMER_UNIT;
	int setup_8254;
	int timers_tol[NR_RT_CPUS];
};

extern struct rt_times rt_times;
extern struct rt_times rt_smp_times[NR_RT_CPUS];
extern struct calibration_data tuned;

extern RTIME rd_8254_ts(void);
extern void rt_setup_8254_tsc(void);
extern int calibrate_8254(void);

#undef rdtsc
static inline unsigned long long rdtsc(void)
{

	if(tuned.cpu_freq == FREQ_8254) {
		return rd_8254_ts();
	} else {
		// get the time stamp clock (TSC) of a cpu.
		long long time;
		__asm__ __volatile__( "rdtsc" : "=A" (time));
		return time;
	}
}

// returns (int)i = (int)i*(int)(mult)/(int)div.
static inline int imuldiv(int i, int mult, int div)
{
	int dummy;
       __asm__ __volatile__ (\
	       "imull %%edx ;     idiv %%ecx; sal $1,%%edx  \n\t"
	       "cmpl %%edx,%%ecx; jge 1f;     incl %%eax;   1:"
               : "=a" (i), "=d" (dummy)
       	       : "a" (i), "d" (mult), "c" (div));
       return i;
}

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

#define set_8259_prio(mprio, sprio) \
do { \
	unsigned long flags; \
	hard_save_flags_and_cli(flags); \
	outb_p(0xC0 + ((mprio - 0) & 0xF), 0x20); \
	outb_p(0xC0 + ((sprio - 8) & 0xF), 0xA0); \
	hard_restore_flags(flags); \
} while (0)

#endif
