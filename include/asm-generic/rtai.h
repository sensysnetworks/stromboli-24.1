/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)
Copyright (C) 2001  David A. Schleef <ds@schleef.org>

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


#ifndef _RTAI_ASM_GENERIC_RTAI_H_
#define _RTAI_ASM_GENERIC_RTAI_H_

#include <linux/types.h>
//#include <asm/time.h>

#include <rtai_types.h>


#ifdef __KERNEL__

//#include <asm/ptrace.h>
#include <asm/hw_irq.h>
#include <asm/processor.h>
#include <asm/bitops.h>

struct pt_regs;
struct apic_timer_setup_data;

/* This needs to be fixed for the specific arch */
typedef struct generic_fpu_env { unsigned long fpu_regs[32]; } FPU_ENV;

/* This needs to be fixed for the specific arch */
#define RTAI_NR_TRAPS 16

/* random brokenness */
#define TIMER_8254_IRQ 0
#define NR_RT_CPUS 1
#define FREQ_8254 0	/* for examples/condtest */
#define CPU_FREQ 0	/* for examples/jepplin */
#define smp_num_cpus 1	/* for examples/jitter_free_sw */
#define LATENCY_8254 0
#define SETUP_TIME_8254 0

/* unknown stuff */
#define save_cr0_and_clts(x)
#define restore_cr0(x)
#define enable_fpu(x)
#define DECLR_8254_TSC_EMULATION
#define TICK_8254_TSC_EMULATION
#define SETUP_8254_TSC_EMULATION
#define CLEAR_8254_TSC_EMULATION

/* grrr - this should have a arch-independent name */
struct apic_timer_setup_data {
	int mode;
	int count;
};

/* grrr - should be in rtai_types.h */
struct calibration_data {
	unsigned int cpu_freq;
	unsigned int apic_freq;
	int latency;
	int setup_time_TIMER_CPUNIT;
	int setup_time_TIMER_UNIT;
	int timers_tol[NR_RT_CPUS];
};

/* grrr - doesn't belong here, whatever */
#define RT_TIME_END 0x7fffffffffffffffLL

extern struct rt_times rt_times;
extern struct rt_times rt_smp_times[NR_RT_CPUS];
extern struct calibration_data tuned;

void send_ipi_shorthand(unsigned int shorthand, int irq);
void send_ipi_logical(unsigned long dest, int irq);
int rt_assign_irq_to_cpu(int irq, unsigned long cpus_mask);
int rt_reset_irq_to_sym_mode(int irq);
int rt_request_global_irq(unsigned int irq, void (*handler)(void));
int rt_free_global_irq(unsigned int irq);
void rt_ack_irq(unsigned int irq);
void rt_mask_and_ack_irq(unsigned int irq);
void rt_unmask_irq(unsigned int irq);
unsigned int rt_startup_irq(unsigned int irq);
void rt_shutdown_irq(unsigned int irq);
void rt_enable_irq(unsigned int irq);
void rt_disable_irq(unsigned int irq);
int rt_request_linux_irq(unsigned int irq,
	void (*linux_handler)(int irq, void *dev_id, struct pt_regs *regs), 
	char *linux_handler_id, void *dev_id);
int rt_free_linux_irq(unsigned int irq, void *dev_id);
void rt_pend_linux_irq(unsigned int irq);
int rt_request_srq(unsigned int label, void (*rtai_handler)(void),
	long long (*user_handler)(unsigned int whatever));
int rt_free_srq(unsigned int srq);
#if 0
/* we don't want to pull in header for desc_struct */
struct desc_struct rt_set_full_intr_vect(unsigned int vector, int type,
	int dpl, void *handler);
void rt_reset_full_intr_vect(unsigned int vector,
	struct desc_struct idt_element);
#endif
void rt_pend_linux_srq(unsigned int srq);
int rt_request_cpu_own_irq(unsigned int irq, void (*handler)(void));
int rt_free_cpu_own_irq(unsigned int irq);
void rt_request_timer(void (*handler)(void), unsigned int tick, int apic);
void rt_free_timer(void);
void rt_request_apic_timers(void (*handler)(void),
	struct apic_timer_setup_data *apic_timer_data);
void rt_free_apic_timers(void);
void rt_mount_rtai(void);
void rt_umount_rtai(void);
int rt_printk(const char *format, ...);
int rtai_print_to_screen(const char *format, ...);

/* only in i386 */
void rt_do_irq(int vector);
void rt_request_timer_cpuid(void (*handler)(void), unsigned int tick,
	int cpuid);
/* only in ppc */
extern void rt_tick_linux_timer(void);
#define rt_set_intr_handler(vector, handler) ((void *)0)
#define rt_reset_intr_handler(vector, handler)

#ifdef INTERFACE_TO_LINUX
int rt_is_linux(void);
RT_TRAP_HANDLER rt_set_rtai_trap_handler(RT_TRAP_HANDLER handler);
struct task_struct *rt_whoislinux(int cpuid);
extern void rt_switch_to_linux(int cpuid);
extern void rt_switch_to_real_time(int cpuid);
#endif

#if 0
/* These should be in asm/system.h */
void hard_cli(void);
void hard_restore_flags(unsigned long flags);
void hard_sti(void);
void __hard_save_flags(unsigned long *flags);

#define hard_save_flags(flags)         do { __hard_save_flags(&(flags)); } while (0)
#define hard_save_flags_and_cli(flags) do { __hard_save_flags(&(flags)); hard_cli(); } while (0)
#endif

void rt_spin_lock(spinlock_t *lock);
void rt_spin_unlock(spinlock_t *lock);
int hard_cpu_id(void);

void rt_get_global_lock(void);
void rt_release_global_lock(void);

void __save_fpenv(FPU_ENV *fpu_env);
void __restore_fpenv(FPU_ENV *fpu_env);
#define save_fpenv(x) __save_fpenv(&(x))
#define restore_fpenv(x) __restore_fpenv(&(x))

void rt_spin_lock_irq(spinlock_t *lock);
void rt_spin_unlock_irq(spinlock_t *lock);
unsigned int rt_spin_lock_irqsave(spinlock_t *lock);
void rt_spin_unlock_irqrestore(unsigned long flags, spinlock_t *lock);
void rt_global_cli(void);
void rt_global_sti(void);
int rt_global_save_flags_and_cli(void);
void rt_global_save_flags(unsigned long *flags);
void rt_global_restore_flags(unsigned long flags);
RT_TRAP_HANDLER rt_set_rtai_trap_handler(RT_TRAP_HANDLER handler);

void rt_set_timer_delay(unsigned int delay);


unsigned long long rdtsc(void);
void rt_set_decrementer_count(int delay);

unsigned long long ullmul(unsigned long m0, unsigned long m1);
unsigned long long ulldiv(unsigned long long ull, unsigned long uld,
	unsigned long *r);
int imuldiv(int i, int mult, int div);
unsigned long long llimd(unsigned long long ull, unsigned long mult,
	unsigned long div);
int ffnz(long ul);


#endif /* __KERNEL__ */
#endif

