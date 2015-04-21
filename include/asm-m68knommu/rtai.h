/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)
m68knommu contributed by Lineo Inc. (Author: Bernhard Kuhn (bkuhn@lineo.com)

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


#ifndef _ASM_M68KNOMMU_RTAI_H_
#define _ASM_M68KNOMMU_RTAI_H_

#include <linux/config.h>
#include <linux/types.h>
#ifdef __KERNEL__
#include <linux/smp.h>
#endif

#include <rtai_types.h>

#define RTAI_TIMER_IRQLEVEL 0x2600

// CPU frequency calibration
#define CPU_FREQ (tuned.cpu_freq)
#define FREQ_DECR CPU_FREQ
#define CALIBRATED_CPU_FREQ     0 // Use this if you know better than Linux!

// These are truly PPC specific.
#define LATENCY_DECR 2500 
#define SETUP_TIME_DECR 500 

// Do not be messed up by macros names below, is a trick for keeping i386 code.
#define FREQ_8254 FREQ_DECR
#define FREQ_APIC FREQ_DECR
#define LATENCY_8254 27600
#define SETUP_TIME_8254 500
#ifdef CONFIG_NETtel
#define TIMER_8254_IRQ 25
#endif
#ifdef CONFIG_MOTOROLA
#define TIMER_8254_IRQ 72
#endif

#define IFLAG 15

#define RTAI_NR_TRAPS 32

#define RTAI_1_VECTOR  0xD9
#define RTAI_2_VECTOR  0xE1
#define RTAI_3_VECTOR  0xE9
#define RTAI_4_VECTOR  0xF1

#define RT_TIME_END 0x7FFFFFFFFFFFFFFFLL

struct apic_timer_setup_data { int mode, count; };

struct desc_struct {
	void* a;
};

extern unsigned volatile int *locked_cpus;

#ifdef __KERNEL__

#include <asm/ptrace.h>
#include <asm/processor.h>
#include <asm/bitops.h>

extern int  rt_request_global_irq(unsigned int irq, void (*handler)(void));
extern int  rt_free_global_irq(unsigned int irq);
extern void rt_ack_irq(unsigned int irq);
extern void rt_mask_and_ack_irq(unsigned int irq);
extern void rt_unmask_irq(unsigned int irq);
extern unsigned int rt_startup_irq(unsigned int irq);
extern void rt_shutdown_irq(unsigned int irq);
extern void rt_enable_irq(unsigned int irq);
extern void rt_disable_irq(unsigned int irq);
extern int rt_request_linux_irq(unsigned int irq,
	void (*linux_handler)(int irq, void *dev_id, struct pt_regs *regs), 
	char *linux_handler_id, void *dev_id);
extern int rt_free_linux_irq(unsigned int irq, void *dev_id);
extern void rt_pend_linux_irq(unsigned int irq);
extern void rt_tick_linux_timer(void);

extern int rt_request_srq(unsigned int label, void (*rtai_handler)(void), long long (*user_handler)(unsigned int whatever));
extern int rt_free_srq(unsigned int srq);
extern void rt_pend_linux_srq(unsigned int srq);

extern void rt_request_timer(void (*handler)(void), unsigned int tick, int apic);
extern void rt_free_timer(void);

extern void rt_mount_rtai(void);
extern void rt_umount_rtai(void);
extern int rt_printk(const char *format, ...);
extern int rtai_print_to_screen(const char *format, ...);

#ifdef INTERFACE_TO_LINUX
extern void rt_switch_to_linux(int cpuid);
extern void rt_switch_to_real_time(int cpuid);
#endif

extern struct desc_struct rt_set_full_intr_vect(unsigned int vector, int type, int dpl, void (*handler)(void));
extern void rt_reset_full_intr_vect(unsigned int vector, struct desc_struct idt_element);
#define rt_spin_lock(whatever)  
#define rt_spin_unlock(whatever)

#define rt_get_global_lock()  hard_cli()
#define rt_release_global_lock()

#define hard_cpu_id()  0

#define NR_RT_CPUS  1

typedef struct ppc_fpu_env { unsigned long fpu_reg[66]; } FPU_ENV;

#define save_cr0_and_clts(x)
#define restore_cr0(x)
#define enable_fpu()
#define save_fpenv(fpu_reg)
#define restore_fpenv(fpu_reg)

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
	int flags=0x2500;
	rt_release_global_lock();
	hard_restore_flags(flags);
}

#if 1

static inline int rt_global_save_flags_and_cli(void)
{
	unsigned long flags;
	hard_save_flags_and_cli(flags);
	return flags;
}

static inline void rt_global_save_flags(unsigned long *flags)
{
	unsigned long rflags;
	hard_save_flags(rflags);
	*flags = rflags;
}

static inline void rt_global_restore_flags(unsigned long flags)
{
	hard_restore_flags(flags);
}

#else 

static inline int rt_global_save_flags_and_cli(void)
{
	unsigned long flags;

	hard_save_flags_and_cli(flags);
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

	hard_save_flags_and_cli(hflags);
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

#endif

#if 0 // XX

static inline RT_TRAP_HANDLER rt_set_rtai_trap_handler(RT_TRAP_HANDLER handler)
{
	return (RT_TRAP_HANDLER) 0;
}

#endif // XX

#define rt_assign_irq_to_cpu(irq,cpus_mask)
#define rt_reset_irq_to_sym_mode(irq)

struct calibration_data {
	unsigned int cpu_freq;
	int latency;
	int setup_time_TIMER_CPUNIT;
	int setup_time_TIMER_UNIT;
	int timers_tol[NR_RT_CPUS];
};

extern struct rt_times rt_times;
extern struct rt_times rt_smp_times[NR_RT_CPUS];
extern struct calibration_data tuned;

static inline unsigned long long ullmul(unsigned long m0, unsigned long m1)
{
	return ((long long)m0)*((long long)m1);
}

static inline unsigned long long ulldiv(unsigned long long ull, unsigned long uld, unsigned long *r)
{
	unsigned long long q, rf;
	unsigned long qh, rh, ql, qf;

	q = 0;
	rf = (unsigned long long)(0xFFFFFFFF - (qf = 0xFFFFFFFF / uld) * uld) + 1ULL;

	while (ull >= uld) {
		((unsigned long *)&q)[0] += (qh = ((unsigned long *)&ull)[0] / uld);
		rh = ((unsigned long *)&ull)[0] - qh * uld;
		q += rh * (unsigned long long)qf + (ql = ((unsigned long *)&ull)[1] / uld);
		ull = rh * rf + (((unsigned long *)&ull)[1] - ql * uld);
	}

	*r = ull;
	return q;
}

/*
static inline int imuldiv(int i, int mult, int div)
{
	unsigned long q, r;

	q = ulldiv(ullmul(i, mult), div, &r);

	return (r + r) > div ? q + 1 : q;
}
*/
static inline int imuldiv(int i, int mult, int div)
{
	long long q;
	q = ((long long)i) * ((long long)mult);
	q/= ((long long)div);
	return q;
}

/*
static inline unsigned long long llimd(unsigned long long ull, unsigned long mult, unsigned long div)
{
	unsigned long long low;
	unsigned long q, r;

	low  = ullmul(((unsigned long *)&ull)[1], mult);	
	q = ulldiv( ullmul(((unsigned long *)&ull)[0], mult) + ((unsigned long *)&low)[0], div, (unsigned long *)&low);
	low = ulldiv(low, div, &r);
	((unsigned long *)&low)[0] += q;

	return (r + r) > div ? low + 1 : low;
}
*/

static inline unsigned long long llimd(unsigned long long ull, unsigned long mult, unsigned long div)
{
  unsigned long long q;
  q = ull * ((long long)mult);
  q/= ((long long)div);
  return q;
};

#define FFNZ_CHECK_BYTE(b,offset) \
if(b & 0xf0) \
  if(b & 0xc0) \
    if(b & 0x80) \
      return offset+7; \
    else \
      return offset+6; \
  else \
    if(b & 0x20) \
      return offset+5; \
    else \
      return offset+4; \
else \
  if(b & 0x0c) \
    if(b & 0x08) \
      return offset+3; \
    else \
      return offset+2; \
  else \
    if(b & 0x02) \
      return offset+1; \
    else \
      return offset+0; \

// We like keeping it as for i386.
static inline int ffnz(long x) {
  unsigned char *b=(unsigned char*)&x;
  unsigned short *w=(unsigned short*)&x;
  if(w[0])
    if(b[0])
      FFNZ_CHECK_BYTE(b[0],24)
    else
      FFNZ_CHECK_BYTE(b[1],16)
  else
    if(b[2])
      FFNZ_CHECK_BYTE(b[2],8)
    else
      FFNZ_CHECK_BYTE(b[3],0);
  return -1;
};


extern void rt_set_timer_delay(unsigned short delay);
extern long long rdtsc(void);

#define soft_cli()  __asm__ __volatile__ ( \
                        "move %0,%%sr\n" \
                        : /* no output */ \
                        : "i" (LINUX_IRQS_DISABLE) \
                        : "cc", "memory")

#define soft_sti()  __asm__ __volatile__ ( \
                        "move #0x2000,%/sr\n" \
                        : /* no output */ \
                        : /* no input */ \
                        : "cc", "memory")

#endif

#define DECLR_8254_TSC_EMULATION
#define TICK_8254_TSC_EMULATION
#define SETUP_8254_TSC_EMULATION
#define CLEAR_8254_TSC_EMULATION

#endif
