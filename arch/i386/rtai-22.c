/*
"COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USAr.
*/

/* 
ACKNOWLEDGMENTS: 
- Steve Papacharalambous (stevep@zentropix.com) has contributed an informative 
  proc filesystem procedure.
- rt_printk is just an RTAI adapted copy of David Schleef rt_printk idea of·
    hacking Linux printk so that it could be safely used within NMT_RTL modules.
- from rt_printk it was easily go ahead and add also "rt_print_to_screen", for
    those that do not want messages to be logged.
*/


/* Module to hook plain Linux up to do real time the way you like, hardware, */
/* hopefully, fully trapped; the machine is in your hand, do what you want!  */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <linux/types.h>		// __u8 needed by processor
#include <asm/processor.h>		// cpuinfo_x86 struct

#ifdef CONFIG_SMP
#include <asm/i82489.h>
#endif
#include <asm/fixmap.h>
#include <asm/smp.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/bitops.h>
#include <asm/atomic.h>
#include <asm/desc.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#endif

#include <rtai.h>
#include <asm/rtai_srq.h>

// proc filesystem additions.
static int rtai_proc_register(void);
static void rtai_proc_unregister(void);
// End of proc filesystem additions. 

static void rt_printk_sysreq_handler(void);
static int rt_printk_srq;

/* Macros to setup intercept-dispatching of any interrupt, Linux cannot hard */
/* interrupt any more and all those related to IPIs and IOs are catched and  */
/* can be our slaves.                                                        */

#define __STR(x) #x
#define STR(x) __STR(x)

// Simple SAVE/RSTR for compilation with the -fomit-frame-pointer option.
// Very simple assembler, support for using the fpu without problem, has been
// taken away for reason of efficiency. To use hard floating point in your 
// interrupt handlers, without any problem, see the fpu support macros 
// available in "rtai.h".

struct fill_t { unsigned long fill[6]; };

#define SAVE_REG(irq) __asm__ __volatile__ (" \
	pushl $"#irq"; pushl %es; pushl %ds; pushl %eax;\n\t \
	pushl %ebp; pushl %ecx; pushl %edx;\n\t \
	movl $" STR(__KERNEL_DS) ",%edx; mov %dx,%ds; mov %dx,%es")

#define RSTR_REG __asm__ __volatile__ (" \
	popl %edx; popl %ecx; testl %eax,%eax; jnz 1f;\n\t \
	popl %ebp; popl %eax; popl %ds; popl %es; addl $4,%esp; iret;\n\t \
	1: cld; pushl %edi; pushl %esi; pushl %edx;\n\t \
	pushl %ecx; pushl %ebx; jmp *"SYMBOL_NAME_STR(rthal + 60))

#define GLOBAL_IRQ_FNAME(y) y##_interrupt(void)
#define GLOBAL_IRQ_NAME(y) GLOBAL_IRQ_FNAME(GLOBAL##y)

#define BUILD_GLOBAL_IRQ(irq) static void GLOBAL_IRQ_NAME(irq) \
{ \
	SAVE_REG(irq); \
	__asm__ __volatile__ ("call "SYMBOL_NAME_STR(dispatch_global_irq)); \
	RSTR_REG; \
}

#define CPU_OWN_IRQ_FNAME(y) y##_interrupt(void)
#define CPU_OWN_IRQ_NAME(y) CPU_OWN_IRQ_FNAME(CPU_OWN##y)

#define BUILD_CPU_OWN_IRQ(irq) static void CPU_OWN_IRQ_NAME(irq) \
{ \
	SAVE_REG(irq); \
	__asm__ __volatile__ ("call "SYMBOL_NAME_STR(dispatch_cpu_own_irq)); \
	RSTR_REG; \
}

#define TRAP_SAVE_REG(vec) __asm__ __volatile__ (" \
        pushl %es; pushl %ds; pushl %eax; pushl %ebp;\n\t \
        pushl %ecx; pushl %edx; pushl $"#vec"\n\t \
        movl $" STR(__KERNEL_DS) ",%edx; mov %dx,%ds; mov %dx,%es")

#define TRAP_RSTR_REG(vec) __asm__ __volatile__ (" \
        addl $4,%esp; testl %eax,%eax; popl %edx; popl %ecx;
        popl %ebp; popl %eax; popl %ds; popl %es; jz 1f; iret;\n\t \
        1: jmp *"SYMBOL_NAME_STR(linux_isr + 4*vec))

#define TRAP_FNAME(y) TRAP_##y(void)
#define TRAP_NAME(y) TRAP_FNAME(##y)

#define BUILD_TRAP(vec) static void TRAP_NAME(vec) \
{ \
        TRAP_SAVE_REG(vec); \
        __asm__ __volatile__ ("call "SYMBOL_NAME_STR(dispatch_trap)); \
        TRAP_RSTR_REG(vec); \
}

static void srqisr(void)
{
	__asm__ __volatile__ (" \
	cld; pushl %es; pushl %ds; pushl %ebp;\n\t \
	pushl %edi; pushl %esi; pushl %ecx;\n\t \
	pushl %ebx; pushl %edx; pushl %eax;\n\t \
	movl $" STR(__KERNEL_DS) ",%ebx; mov %bx,%ds; mov %bx,%es");

	__asm__ __volatile__ ("call "SYMBOL_NAME_STR(dispatch_srq));

	__asm__ __volatile__ (" \
	addl $8,%esp; popl %ebx; popl %ecx; popl %esi;\n\t \
	popl %edi; popl %ebp; popl %ds; popl %es; iret");
}

/* Setup intercept-dispatching handlers */

BUILD_GLOBAL_IRQ(0)   BUILD_GLOBAL_IRQ(1)   BUILD_GLOBAL_IRQ(2)
BUILD_GLOBAL_IRQ(3)   BUILD_GLOBAL_IRQ(4)   BUILD_GLOBAL_IRQ(5)  
BUILD_GLOBAL_IRQ(6)   BUILD_GLOBAL_IRQ(7)   BUILD_GLOBAL_IRQ(8)
BUILD_GLOBAL_IRQ(9)   BUILD_GLOBAL_IRQ(10)  BUILD_GLOBAL_IRQ(11)
BUILD_GLOBAL_IRQ(12)  BUILD_GLOBAL_IRQ(13)  BUILD_GLOBAL_IRQ(14) 
BUILD_GLOBAL_IRQ(15)  BUILD_GLOBAL_IRQ(16)  BUILD_GLOBAL_IRQ(17) 
BUILD_GLOBAL_IRQ(18)  BUILD_GLOBAL_IRQ(19)  BUILD_GLOBAL_IRQ(20) 
BUILD_GLOBAL_IRQ(21)  BUILD_GLOBAL_IRQ(22)  BUILD_GLOBAL_IRQ(23)
BUILD_GLOBAL_IRQ(24)  BUILD_GLOBAL_IRQ(25)  BUILD_GLOBAL_IRQ(26) 
BUILD_GLOBAL_IRQ(27)  BUILD_GLOBAL_IRQ(28)  BUILD_GLOBAL_IRQ(29) 
BUILD_GLOBAL_IRQ(30)  BUILD_GLOBAL_IRQ(31)

BUILD_CPU_OWN_IRQ(0)  BUILD_CPU_OWN_IRQ(1)  BUILD_CPU_OWN_IRQ(2)
BUILD_CPU_OWN_IRQ(3)  BUILD_CPU_OWN_IRQ(4)  BUILD_CPU_OWN_IRQ(5)
BUILD_CPU_OWN_IRQ(6)  BUILD_CPU_OWN_IRQ(7)  BUILD_CPU_OWN_IRQ(8)
BUILD_CPU_OWN_IRQ(9)

BUILD_TRAP(0)   BUILD_TRAP(1)   BUILD_TRAP(2)   BUILD_TRAP(3)
BUILD_TRAP(4)   BUILD_TRAP(5)   BUILD_TRAP(6)   BUILD_TRAP(7)
BUILD_TRAP(8)   BUILD_TRAP(9)   BUILD_TRAP(10)  BUILD_TRAP(11)
BUILD_TRAP(12)  BUILD_TRAP(13)  BUILD_TRAP(14)  BUILD_TRAP(15)
BUILD_TRAP(16)  BUILD_TRAP(17)  BUILD_TRAP(18)  BUILD_TRAP(19)
BUILD_TRAP(20)  BUILD_TRAP(21)  BUILD_TRAP(22)  BUILD_TRAP(23)
BUILD_TRAP(24)  BUILD_TRAP(25)  BUILD_TRAP(26)  BUILD_TRAP(27)
BUILD_TRAP(28)  BUILD_TRAP(29)  BUILD_TRAP(30)  BUILD_TRAP(31)

/* Some define */

/* The two below cannot be > 32 (bits) */
#define NR_GLOBAL_IRQS   32
#define NR_CPU_OWN_IRQS  10
#define NR_TRAPS         32
#define NR_IC_DEV_TYPES   3 // 0=8259A, 1=level/edge-IO-APIC, 2=none

#define DUMMY_VECTOR  0x32

#define HARD_LOCK_IPI      	RTAI_1_IPI
#define RTAI_APIC_TIMER_IPI    	RTAI_3_IPI
#define RTAI_APIC_TIMER_VECTOR  RTAI_3_VECTOR
#define APIC_ICOUNT		((tuned.apic_freq + HZ/2)/HZ)

#define RESCHEDULE_IPI     0
#define INVALIDATE_IPI     1
#define STOP_CPU_IPI       2
#define LOCAL_TIMER_IPI    3
#define CALL_FUNCTION_IPI  4
#define SPURIOUS_IPI       5

/* BE SURE TYPE DEFINITIONS BELOW ARE THE SAME AS THOSE OF Linux, ALWAYS!!!  */
/* AND ALSO MATCH THE ORDER OF THE SAME NUMBER OF ....._IPI MACROS DEFINED   */
/* IMMEDIATLY ABOVE, THE IRQ0_TRAP_VECTOR EXCLUDED.                          */
/* BE SURE TYPE DEFINITIONS BELOW ARE THE SAME AS THOSE OF Linux, ALWAYS!!!  */
/* BE SURE TYPE DEFINITIONS BELOW ARE THE SAME AS THOSE OF Linux, ALWAYS!!!  */

#define RESCHEDULE_VECTOR      0x30
#define INVALIDATE_TLB_VECTOR  0x31
#define STOP_CPU_VECTOR        0x40
#define LOCAL_TIMER_VECTOR     0x41
#define CALL_FUNCTION_VECTOR   0x50
#define SPURIOUS_APIC_VECTOR   0xFF

#define IRQ0_TRAP_VECTOR  0x51

struct hw_interrupt_type {
	const char        *typename;
	void  (*startup)  (unsigned int irq);
	void  (*shutdown) (unsigned int irq);
	void  (*handle)   (unsigned int irq, struct pt_regs *regs);
	void  (*enable)   (unsigned int irq);
	void  (*disable)  (unsigned int irq);
};
typedef struct {
	unsigned int              status;
	struct hw_interrupt_type  *handler;
	struct irqaction          *action;
	unsigned int              depth;
	unsigned int              unused[4];
} irq_desc_t;

#define IRQ_DESC ((irq_desc_t *)rthal.irq_desc)

/* BE SURE TYPE DEFINITIONS ABOVE ARE THE SAME AS THOSE OF Linux, ALWAYS!!! */
/* BE SURE TYPE DEFINITIONS ABOVE ARE THE SAME AS THOSE OF Linux, ALWAYS!!! */
/* BE SURE TYPE DEFINITIONS BELOW ARE THE SAME AS THOSE OF Linux, ALWAYS!!!  */


// RTHAL from Linux
extern struct rt_hal rthal;

/* Most of our data */

static int global_vector[NR_GLOBAL_IRQS] = { 0x20, 0x21, 0x22, 0x23, 0x24,
	 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
DUMMY_VECTOR, DUMMY_VECTOR, DUMMY_VECTOR, DUMMY_VECTOR, DUMMY_VECTOR,
DUMMY_VECTOR, DUMMY_VECTOR, DUMMY_VECTOR, DUMMY_VECTOR, DUMMY_VECTOR,
DUMMY_VECTOR, DUMMY_VECTOR, DUMMY_VECTOR, DUMMY_VECTOR, DUMMY_VECTOR,
DUMMY_VECTOR };

static void (*global_interrupt[NR_GLOBAL_IRQS])(void) = {
	GLOBAL0_interrupt,  GLOBAL1_interrupt,  GLOBAL2_interrupt,
	GLOBAL3_interrupt,  GLOBAL4_interrupt,  GLOBAL5_interrupt, 
	GLOBAL6_interrupt,  GLOBAL7_interrupt,  GLOBAL8_interrupt, 
	GLOBAL9_interrupt,  GLOBAL10_interrupt, GLOBAL11_interrupt,
	GLOBAL12_interrupt, GLOBAL13_interrupt, GLOBAL14_interrupt,
	GLOBAL15_interrupt, GLOBAL16_interrupt, GLOBAL17_interrupt,
	GLOBAL18_interrupt, GLOBAL19_interrupt,	GLOBAL20_interrupt,
	GLOBAL21_interrupt, GLOBAL22_interrupt, GLOBAL23_interrupt,
	GLOBAL24_interrupt, GLOBAL25_interrupt, GLOBAL26_interrupt,
	GLOBAL27_interrupt, GLOBAL28_interrupt, GLOBAL29_interrupt,
	GLOBAL30_interrupt, GLOBAL31_interrupt };

static void (*global_irq_handler[NR_GLOBAL_IRQS])(void);
static struct sysrq_t {
	unsigned int label;
	void (*rtai_handler)(void);
	long long (*user_handler)(unsigned int whatever);
} sysrq[NR_GLOBAL_IRQS];

static int cpu_own_vector[NR_CPU_OWN_IRQS] = {
	RESCHEDULE_VECTOR,  INVALIDATE_TLB_VECTOR, STOP_CPU_VECTOR, 
	LOCAL_TIMER_VECTOR, CALL_FUNCTION_VECTOR,  SPURIOUS_APIC_VECTOR,
	RTAI_1_VECTOR,      RTAI_2_VECTOR,         RTAI_3_VECTOR,
	RTAI_4_VECTOR };

static void (*cpu_own_interrupt[NR_CPU_OWN_IRQS])(void) = {
	CPU_OWN0_interrupt, CPU_OWN1_interrupt, CPU_OWN2_interrupt,
	CPU_OWN3_interrupt, CPU_OWN4_interrupt, CPU_OWN5_interrupt,
	CPU_OWN6_interrupt, CPU_OWN7_interrupt, CPU_OWN8_interrupt,
	CPU_OWN9_interrupt };

static struct cpu_own_irq_handling {
	volatile unsigned long dest_status;		
	void (*handler)(void);
} cpu_own_irq[NR_CPU_OWN_IRQS];

static void (*trap_interrupt[NR_TRAPS])(void) = {
        TRAP_0,  TRAP_1,  TRAP_2,  TRAP_3,  TRAP_4,
        TRAP_5,  TRAP_6,  TRAP_7,  TRAP_8,  TRAP_9,
        TRAP_10, TRAP_11, TRAP_12, TRAP_13, TRAP_14,
        TRAP_15, TRAP_16, TRAP_17, TRAP_18, TRAP_19,
        TRAP_20, TRAP_21, TRAP_22, TRAP_23, TRAP_24,
        TRAP_25, TRAP_26, TRAP_27, TRAP_28, TRAP_29,
        TRAP_30, TRAP_31 };

static int (*rtai_trap_handler[NR_TRAPS])(void);

volatile unsigned long lxrt_hrt_flags;

// The main items to be saved-restored to make Linux our humble slave
static struct rt_hal linux_rthal;
static struct desc_struct linux_idt_table[256];
static void (*linux_isr[256])(void);
static struct hw_interrupt_type *linux_irq_desc_handler[NR_GLOBAL_IRQS];

static struct global_rt_status {
	volatile unsigned int pending_irqs;
	volatile unsigned int activ_irqs;
	volatile unsigned int pending_srqs;
	volatile unsigned int activ_srqs;
	volatile unsigned int cpu_in_sti;
	volatile unsigned int used_by_linux;
  	volatile unsigned int locked_cpus;
  	volatile unsigned int hard_nesting;
	volatile unsigned int hard_lock_all_service;
	spinlock_t hard_lock;
	spinlock_t data_lock;
	spinlock_t ic_lock[NR_IC_DEV_TYPES];
} global;

volatile unsigned int *locked_cpus = &global.locked_cpus;

static struct cpu_own_status {
	volatile unsigned int intr_flag;
	volatile unsigned int linux_intr_flag;
	volatile unsigned int pending_irqs;
	volatile unsigned int activ_irqs;
} processor[NR_RT_CPUS]; 


/* Interrupt descriptor table manipulation. No assembler here. These are    */
/* the base for manipulating Linux interrupt handling without even touching */
/* the kernel.                                                              */

struct desc_struct rt_set_full_intr_vect(unsigned int vector, int type, int dpl, void (*handler)(void))
{

// "dpl" is the descriptor privilege level: 0-highest, 3-lowest.
// "type" is the interrupt type: 14 interrupt (cli), 15 trap (no cli).

struct desc_struct idt_element = rthal.idt_table[vector];

	rthal.idt_table[vector].a = (__KERNEL_CS << 16) | 
					((unsigned int)handler & 0x0000FFFF);
	rthal.idt_table[vector].b = ((unsigned int)handler & 0xFFFF0000) | 
					(0x8000 + (dpl << 13) + (type << 8));
	return idt_element;
}

void rt_reset_full_intr_vect(unsigned int vector, struct desc_struct idt_element)
{
	rthal.idt_table[vector] = idt_element;
	return;
}

// Get the interrupt handler proper.
static inline void *get_intr_handler(unsigned int vector)
{
	return (void *)((rthal.idt_table[vector].b & 0xFFFF0000) | 
			(rthal.idt_table[vector].a & 0x0000FFFF));
}

static inline void set_intr_vect(unsigned int vector, void (*handler)(void))
{

/* It should be done as above, in set_full_intr_vect, but we keep what has not
to be changed as it is already. So we have not to mind of DPL, TYPE and 
__KERNEL_CS. Then let's just change only the offset part of the idt table. */

	rthal.idt_table[vector].a = (rthal.idt_table[vector].a & 0xFFFF0000) | 
				           ((unsigned int)handler & 0x0000FFFF);
	rthal.idt_table[vector].b = ((unsigned int)handler & 0xFFFF0000) | 
			               (rthal.idt_table[vector].b & 0x0000FFFF);
}

void *rt_set_intr_handler(unsigned int vector, void (*handler)(void))
{
	void (*saved_handler)(void) = get_intr_handler(vector);
	set_intr_vect(vector, handler);
	return saved_handler;;
}

void rt_reset_intr_handler(unsigned int vector, void (*handler)(void))
{
	set_intr_vect(vector, handler);
	return;
}

#ifdef CONFIG_SMP

/* Our interprocessor messaging */ 

void send_ipi_shorthand(unsigned int shorthand, int irq)
{
	unsigned long flags;
	hard_save_flags(flags);
	hard_cli();
	apic_write(APIC_ICR, (apic_read(APIC_ICR) & ~0xCDFFF) |
	      	(APIC_DEST_DM_FIXED | shorthand | cpu_own_vector[irq]));
	hard_restore_flags(flags);
}

void send_ipi_logical(unsigned long dest, int irq)
{
	unsigned long flags;
	if ((dest &= cpu_present_map)) {
		hard_save_flags(flags);
		hard_cli();
		apic_write(APIC_ICR2, (apic_read(APIC_ICR2) & 0xFFFFFF) | 
								(dest << 24));
		apic_write(APIC_ICR, (apic_read(APIC_ICR) & ~0xCDFFF) |
				(APIC_DEST_LOGICAL | cpu_own_vector[irq]));
		hard_restore_flags(flags);
	}
}

static struct apic_timer_setup_data apic_timer_mode[NR_RT_CPUS];

static inline void setup_periodic_apic(unsigned int count, unsigned int vector)
{
	apic_read(APIC_LVTT);
	apic_write(APIC_LVTT, APIC_LVT_TIMER_PERIODIC | vector);
	apic_read(APIC_TMICT);
	apic_write(APIC_TMICT, count);
}

static inline void setup_oneshot_apic(unsigned int count, unsigned int vector)
{
	apic_read(APIC_LVTT);
	apic_write(APIC_LVTT, vector);
	apic_read(APIC_TMICT);
	apic_write(APIC_TMICT, count);
}

/* A few things dependending on being in a UP-SMP configuration */

static inline void ack_APIC_irq(void)
{
	apic_read(APIC_SPIV);
	apic_write(APIC_EOI, 0);
}

static void set_flat_dest_mode(void)
{
	int cpuid;
	set_bit(cpuid = hard_cpu_id(), &cpu_own_irq[HARD_LOCK_IPI].dest_status);
	rt_spin_lock(&(global.hard_lock));
	apic_write(APIC_LDR, apic_read(APIC_LDR)|(0x1000000<<hard_cpu_id()));
/*	apic_write(APIC_DFR, apic_read(APIC_DFR)|0xF0000000); Linux default */
	rt_spin_unlock(&(global.hard_lock));
	clear_bit(cpuid, &cpu_own_irq[HARD_LOCK_IPI].dest_status);
}

static long long apic_timers_sync_time;

static void hard_lock_all_handler(void)
{
	int cpuid;
	struct apic_timer_setup_data *p;

	set_bit(cpuid = hard_cpu_id(), &cpu_own_irq[HARD_LOCK_IPI].dest_status);
	rt_spin_lock(&(global.hard_lock));
	switch (global.hard_lock_all_service) {
		case 1:
			p = apic_timer_mode + cpuid;
			if (p->mode > 0) {
				while (rdtsc() < apic_timers_sync_time);
				setup_periodic_apic(p->count, RTAI_APIC_TIMER_VECTOR);
				break;
			} else if (!p->mode) {
				while (rdtsc() < apic_timers_sync_time);
				setup_oneshot_apic(p->count, RTAI_APIC_TIMER_VECTOR);
				break;
			}
		case 2:
			setup_oneshot_apic(0, RTAI_APIC_TIMER_VECTOR);
			break;
		case 3:
			setup_periodic_apic(APIC_ICOUNT, LOCAL_TIMER_VECTOR);
			break;
	}
	rt_spin_unlock(&(global.hard_lock));
	clear_bit(cpuid, &cpu_own_irq[HARD_LOCK_IPI].dest_status);
}

static inline void check_smp_invalidate(void)
{
	if (test_and_clear_bit(hard_cpu_id(), &smp_invalidate_needed)) {
			local_flush_tlb();
	}
}

static void invalidate_handler(void)
{
	check_smp_invalidate();
}

#else

void send_ipi_shorthand(unsigned int shorthand, int irq) { }

void send_ipi_logical(unsigned long dest, int irq) { }

#define setup_periodic_apic(count, vector)

#define setup_oneshot_apic(count, vector)

#define ack_APIC_irq()

static void set_flat_dest_mode(void) { }

static void hard_lock_all_handler(void) { }

#define check_smp_invalidate()

#endif


/* Emulation of Linux interrupt control and interrupt soft delivery.       */

static inline void do_linux_irq(int vector)
{
	__asm__ __volatile__ ("pushf; push %cs");
	linux_isr[vector]();
	return;
}

void rt_do_irq(int vector)
{
	__asm__ __volatile__ ("pushf; push %cs");
	((void (*)(void))get_intr_handler(vector))();
	return;
}

/* The old SFIF and SFREQ of RTLinux plus vanilla kernel services to be used */
/* within real time applications.                                            */

static void linux_cli(void)
{ 
	processor[hard_cpu_id()].intr_flag = 0;
}

static void linux_sti(void)
{
       	unsigned long irq, cpuid, intr_enabled;
       	struct cpu_own_status *cpu;
	cpu = processor + (cpuid = hard_cpu_id());
	cpu->intr_flag = intr_enabled = (1 << IFLAG) | (1 << cpuid);
	while (!test_and_set_bit(cpuid, &global.cpu_in_sti)) {
	       	while (global.pending_irqs || cpu->pending_irqs ||
						       global.pending_srqs) {
#ifdef CONFIG_SMP
			hard_cli();
			if ((irq = cpu->pending_irqs & ~(cpu->activ_irqs))) {
				irq = ffnz(irq);
				set_bit(irq, &cpu->activ_irqs);
				clear_bit(irq, &cpu->pending_irqs);
				hard_sti();
				cpu->intr_flag = 0;
		       		do_linux_irq(cpu_own_vector[irq]);
				clear_bit(irq, &cpu->activ_irqs);
				cpu->intr_flag = intr_enabled;
	       		} else {
				hard_sti();
			}
#endif
			rt_spin_lock_irq(&(global.data_lock));
			if ((irq = global.pending_srqs & ~global.activ_srqs)) {
				irq = ffnz(irq);
				set_bit(irq, &global.activ_srqs);
				clear_bit(irq, &global.pending_srqs);
				rt_spin_unlock_irq(&(global.data_lock));
				if (sysrq[irq].rtai_handler) {
					sysrq[irq].rtai_handler();
				}
				clear_bit(irq, &global.activ_srqs);
			} else {
				rt_spin_unlock_irq(&(global.data_lock));
			}
			rt_spin_lock_irq(&(global.data_lock));
			if ((irq = global.pending_irqs & ~global.activ_irqs)) {
				irq = ffnz(irq);
				set_bit(irq, &global.activ_irqs);
				clear_bit(irq, &global.pending_irqs);
				rt_spin_unlock_irq(&(global.data_lock));
				cpu->intr_flag = 0;
				do_linux_irq(global_vector[irq]);
				clear_bit(irq, &global.activ_irqs);
				cpu->intr_flag = intr_enabled;
			} else {
				rt_spin_unlock_irq(&(global.data_lock));
			}
		}
		clear_bit(cpuid, &global.cpu_in_sti);
	       	if (!(global.pending_irqs | cpu->pending_irqs |
	      				    global.pending_srqs)) break;
	}
}

static unsigned int linux_save_flags(void)
{
	return processor[hard_cpu_id()].intr_flag;
}

static void linux_restore_flags(unsigned int flags)
{
	if (flags) {
		linux_sti();
	} else {
		processor[hard_cpu_id()].intr_flag = 0;
	}
}

unsigned int linux_save_flags_and_cli(int cpuid)
{
	return xchg((unsigned int *)(&(processor[cpuid].intr_flag)), 0);
}

void rtai_just_copy_back(unsigned long flags, int cpuid)
{
        processor[cpuid].intr_flag = flags;
}


/* Hard lock-unlock all cpus except the one that sets the lock; to be used */
/* to atomically set up critical things affecting both Linux and RTAI.     */ 

static inline int hard_lock_all(void)
{
	unsigned long flags;
	flags = rt_global_save_flags_and_cli();

#ifdef CONFIG_SMP
	if (!global.hard_nesting++) {
		global.hard_lock_all_service = 0;
		rt_spin_lock(&(global.hard_lock));
		send_ipi_shorthand(APIC_DEST_ALLBUT, HARD_LOCK_IPI);
		while (cpu_own_irq[HARD_LOCK_IPI].dest_status != (cpu_present_map & ~global.locked_cpus));
	}
#endif

	return flags;
}

static inline void hard_unlock_all(unsigned long flags)
{
#ifdef CONFIG_SMP
	if (global.hard_nesting > 0) {
		if (!(--global.hard_nesting)) {
			rt_spin_unlock(&(global.hard_lock));
			while (cpu_own_irq[HARD_LOCK_IPI].dest_status);
		}
	}
#endif

	rt_global_restore_flags(flags);
}


/* Dispatching interrupts, the old RTLinux S_STI revisited with the addition */
/* of system services requests and interprocessors interrupts.               */

static spinlock_t *ic_lock[NR_GLOBAL_IRQS];
static void (*internal_ic_ack_irq[NR_GLOBAL_IRQS]) (unsigned int irq);
static void (*ic_ack_irq[NR_GLOBAL_IRQS]) (unsigned int irq);
static void (*ic_mask_and_ack_irq[NR_GLOBAL_IRQS])   (unsigned int irq);
static void (*ic_unmask_irq[NR_GLOBAL_IRQS]) (unsigned int irq);
static void (*linux_unmask_irq[NR_GLOBAL_IRQS]) (unsigned int irq);

void rt_ack_irq(unsigned int irq)
{
	unsigned int flags;
	spinlock_t *dev_lock;
	flags = rt_spin_lock_irqsave(dev_lock = ic_lock[irq]);
	ic_ack_irq[irq](irq);
	rt_spin_unlock_irqrestore(flags, dev_lock);
}

void rt_mask_and_ack_irq(unsigned int irq)
{
	unsigned int flags;
	spinlock_t *dev_lock;
	flags = rt_spin_lock_irqsave(dev_lock = ic_lock[irq]);
	ic_mask_and_ack_irq[irq](irq);
	rt_spin_unlock_irqrestore(flags, dev_lock);
}

void rt_unmask_irq(unsigned int irq)
{
	unsigned int flags;
	spinlock_t *dev_lock;
	flags = rt_spin_lock_irqsave(dev_lock = ic_lock[irq]);
	ic_unmask_irq[irq](irq);
	rt_spin_unlock_irqrestore(flags, dev_lock);
}

void rt_startup_irq(unsigned int irq)
{
	unsigned int flags;
	spinlock_t *dev_lock;
	flags = rt_spin_lock_irqsave(dev_lock = ic_lock[irq]);
	linux_irq_desc_handler[irq]->startup(irq);
	rt_spin_unlock_irqrestore(flags, dev_lock);
}

void rt_shutdown_irq(unsigned int irq)
{
	unsigned int flags;
	spinlock_t *dev_lock;
	flags = rt_spin_lock_irqsave(dev_lock = ic_lock[irq]);
	linux_irq_desc_handler[irq]->shutdown(irq);
	rt_spin_unlock_irqrestore(flags, dev_lock);
}

void rt_enable_irq(unsigned int irq)
{
	unsigned int flags;
	spinlock_t *dev_lock;
	flags = rt_spin_lock_irqsave(dev_lock = ic_lock[irq]);
	linux_irq_desc_handler[irq]->enable(irq);
	rt_spin_unlock_irqrestore(flags, dev_lock);
}

void rt_disable_irq(unsigned int irq)
{
	unsigned int flags;
	spinlock_t *dev_lock;
	flags = rt_spin_lock_irqsave(dev_lock = ic_lock[irq]);
	linux_irq_desc_handler[irq]->disable(irq);
	rt_spin_unlock_irqrestore(flags, dev_lock);
}

static void do_nothing(unsigned int irq) { };

static void ack_8259_irq(unsigned int irq)
{
	if (irq & 8) {
		outb(0x62,0x20);
		outb(0x20,0xA0);
		return;
	}
	outb(0x20,0x20);
	return;
}

static void (*mask_apic_irq)(unsigned int irq);

static void mask_and_ack_apic_irq(unsigned int irq)
{
	mask_apic_irq(irq);
	ack_APIC_irq();
}

static void ack_apic_irq(unsigned int irq)
{
	ack_APIC_irq();
}

// The three dispatch function below should be static, they are not to avoid
// annoying warning messages at compile time.

int dispatch_global_irq(struct fill_t fill, int irq)
{
#ifdef CONFIG_SMP
	spinlock_t *dev_lock;
#endif
	rt_spin_lock(dev_lock = ic_lock[irq]);
	if (global_irq_handler[irq]) {
		internal_ic_ack_irq[irq](irq);
		rt_spin_unlock(dev_lock);
                ((void (*)(int))global_irq_handler[irq])(irq);
		rt_spin_lock_irq(&(global.data_lock));
	} else {
		ic_mask_and_ack_irq[irq](irq);
		rt_spin_unlock(dev_lock);
		rt_spin_lock(&(global.data_lock));
// This is a hack to avoid lock keys timeout.
		if (irq == 1 && test_bit(1, &global.activ_irqs)) {
			rt_spin_unlock_irq(&(global.data_lock));
			do_linux_irq(global_vector[1]);
			return 0;
		}
// End of hack.
		set_bit(irq, &(global.pending_irqs));
	}
	if ((global.used_by_linux & processor[hard_cpu_id()].intr_flag)) {
		rt_spin_unlock_irq(&(global.data_lock));
		linux_sti();
		return 1;
	} else {
		rt_spin_unlock(&(global.data_lock));
		return 0;
	}
}

int dispatch_cpu_own_irq(struct fill_t fill, unsigned int irq)
{
	ack_APIC_irq();
	if (cpu_own_irq[irq].handler) {
		cpu_own_irq[irq].handler();
		rt_spin_lock_irq(&(global.data_lock));
	} else {
		rt_spin_lock(&(global.data_lock));
		set_bit(irq, &processor[hard_cpu_id()].pending_irqs);
	}
	if ((global.used_by_linux & processor[hard_cpu_id()].intr_flag)) {
		rt_spin_unlock_irq(&(global.data_lock));
		linux_sti();
		return 1;
	} else {
		rt_spin_unlock(&(global.data_lock));
		return 0;
	}
}

void rt_grab_linux_traps(void)
{
	unsigned long flags, i;

	flags = hard_lock_all();
	for (i = 0; i < NR_TRAPS ; i++) {
		set_intr_vect(i, trap_interrupt[i]);
        }
	hard_unlock_all(flags);
}

void rt_release_linux_traps(void)
{
	unsigned long flags, i;

	flags = hard_lock_all();
	for (i = 0; i < NR_TRAPS ; i++) {
		rthal.idt_table[i] = linux_idt_table[i];
        }
	hard_unlock_all(flags);
}

int dispatch_trap(int err)
{
        if (err != 14) rtai_print_to_screen("RTAI traps %d (current %p pid %d)\n", err, current, current->pid );
        if (rtai_trap_handler[err]) {
                return rtai_trap_handler[err]();
        }
        return 0;
}

long long dispatch_srq(unsigned int srq, unsigned int whatever)
{
	if (srq > 0 && srq < NR_GLOBAL_IRQS && sysrq[srq].user_handler) {
		return sysrq[srq].user_handler(whatever);
	}
	for (srq = 1; srq < NR_GLOBAL_IRQS; srq++) {
		if (sysrq[srq].label == whatever) {
			return (long long)srq;
		}
	}
	return 0;
}

/* Here are the trapped irq actions for Linux interrupt handlers.          */

static void trpd_startup_irq(unsigned int irq)
{
	spinlock_t *dev_lock;
	rt_spin_lock_irq(dev_lock = ic_lock[irq]);
	linux_irq_desc_handler[irq]->startup(irq);
	rt_spin_unlock_irq(dev_lock);
}

static void trpd_shutdown_irq(unsigned int irq)
{
	spinlock_t *dev_lock;
	rt_spin_lock_irq(dev_lock = ic_lock[irq]);
	linux_irq_desc_handler[irq]->shutdown(irq);
	rt_spin_unlock_irq(dev_lock);
}

static void trpd_enable_irq(unsigned int irq)
{
	spinlock_t *dev_lock;
	rt_spin_lock_irq(dev_lock = ic_lock[irq]);
	linux_irq_desc_handler[irq]->enable(irq);
	rt_spin_unlock_irq(dev_lock);
}

static void trpd_disable_irq(unsigned int irq)
{
	spinlock_t *dev_lock;
	rt_spin_lock_irq(dev_lock = ic_lock[irq]);
	linux_irq_desc_handler[irq]->disable(irq);
	rt_spin_unlock_irq(dev_lock);
}

static void trpd_handle(unsigned int irq, struct pt_regs *regs)
{
	linux_irq_desc_handler[irq]->handle(irq, regs);
}

static void trpd_mask_and_ack_irq(unsigned int irq)
{
	return;
}

static void trpd_unmask_irq(unsigned int irq)
{
	spinlock_t *dev_lock;
	rt_spin_lock_irq(dev_lock = ic_lock[irq]);
	linux_unmask_irq[irq](irq);
	rt_spin_unlock_irq(dev_lock);
}

static struct hw_interrupt_type trapped_linux_irq_type = { 
				"RT SPVISD", 
				trpd_startup_irq,
				trpd_shutdown_irq,
				trpd_handle,
				trpd_enable_irq,
				trpd_disable_irq };

static struct hw_interrupt_type real_time_irq_type = { 
				"REAL TIME", 
				do_nothing,
				do_nothing,
				trpd_handle, // in case RT irqs are shared
				do_nothing,
				do_nothing };

/* Request and free interrupts, system requests and interprocessors messages */
/* Request for regular Linux irqs also included. They are nicely chained to  */
/* Linux, forcing sharing with any already installed handler, so that we can */
/* have an echo from Linux for global handlers. We found that usefull during */
/* debug, but can be nice for a lot of other things, e.g. see the jiffies    */
/* recovery in rtai_sched.c, and the global broadcast to local apic timers.  */

static unsigned long irq_action_flags[NR_GLOBAL_IRQS];
static int chained_to_linux[NR_GLOBAL_IRQS];

int rt_request_global_irq(unsigned int irq, void (*handler)(void))
{
	unsigned long flags;

	if (irq >= NR_GLOBAL_IRQS || !handler) {
		return -EINVAL;
	}
	if (global_irq_handler[irq]) {
		return -EBUSY;
	}
	
	flags = hard_lock_all();
	IRQ_DESC[irq].handler = &real_time_irq_type;
	global_irq_handler[irq] = handler;
	linux_unmask_irq[irq] = do_nothing;
	hard_unlock_all(flags);
	return 0;
}

int rt_free_global_irq(unsigned int irq)
{
	unsigned long flags;

	if (irq >= NR_GLOBAL_IRQS || !global_irq_handler[irq]) {
		return -EINVAL;
	}

	flags = hard_lock_all();
	IRQ_DESC[irq].handler = &trapped_linux_irq_type;
	global_irq_handler[irq] = 0; 
	linux_unmask_irq[irq] = ic_unmask_irq[irq];
	hard_unlock_all(flags);
	return 0;
}

int rt_request_linux_irq(unsigned int irq,
	void (*linux_handler)(int irq, void *dev_id, struct pt_regs *regs), 
	char *linux_handler_id, void *dev_id)
{
	unsigned long flags;

	if (irq >= NR_GLOBAL_IRQS || !linux_handler) {
		return -EINVAL;
	}
	spin_lock_irqsave(rthal.irq_controller_lock, flags);
	if (!chained_to_linux[irq]++) {
		if (IRQ_DESC[irq].action) {
			irq_action_flags[irq] = IRQ_DESC[irq].action->flags;
			IRQ_DESC[irq].action->flags |= SA_SHIRQ;
		}
	}
	spin_unlock_irqrestore(rthal.irq_controller_lock, flags);
	request_irq(irq, linux_handler, SA_SHIRQ, linux_handler_id, dev_id);
	return 0;
}

int rt_free_linux_irq(unsigned int irq, void *dev_id)
{
	unsigned long flags;

	if (irq >= NR_GLOBAL_IRQS || !chained_to_linux[irq]) {
		return -EINVAL;
	}
	free_irq(irq, dev_id);
	spin_lock_irqsave(rthal.irq_controller_lock, flags);
	if (!(--chained_to_linux[irq]) && IRQ_DESC[irq].action) {
		IRQ_DESC[irq].action->flags = irq_action_flags[irq];
	}
	spin_unlock_irqrestore(rthal.irq_controller_lock, flags);
	return 0;
}

void rt_pend_linux_irq(unsigned int irq)
{
	set_bit(irq, &global.pending_irqs);
}

static struct desc_struct svdidt;

int rt_request_srq(unsigned int label, void (*rtai_handler)(void), long long (*user_handler)(unsigned int whatever))
{
	unsigned long flags;
	int srq;

	flags = rt_spin_lock_irqsave(&global.data_lock);
	if (!rtai_handler) {
		rt_spin_unlock_irqrestore(flags, &global.data_lock);
		return -EINVAL;
	}
	for (srq = 1; srq < NR_GLOBAL_IRQS; srq++) {
		if (!(sysrq[srq].rtai_handler)) {
			sysrq[srq].rtai_handler = rtai_handler;
			sysrq[srq].label = label;
			if (user_handler) {
				sysrq[srq].user_handler = user_handler;
				if (!svdidt.a && !svdidt.b) {
					svdidt = rt_set_full_intr_vect(RTAI_SYS_VECTOR, 15, 3, srqisr);
				}
			}
			rt_spin_unlock_irqrestore(flags, &global.data_lock);
			return srq;
		}
	}
	rt_spin_unlock_irqrestore(flags, &global.data_lock);
	return -EBUSY;
}

int rt_free_srq(unsigned int srq)
{
	unsigned long flags;

	flags = rt_spin_lock_irqsave(&global.data_lock);
	if (!srq || srq >= NR_GLOBAL_IRQS || !sysrq[srq].rtai_handler) {
		rt_spin_unlock_irqrestore(flags, &global.data_lock);
		return -EINVAL;
	}
	sysrq[srq].rtai_handler = 0; 
	sysrq[srq].user_handler = 0; 
	sysrq[srq].label = 0;
	for (srq = 1; srq < NR_GLOBAL_IRQS; srq++) {
		if (sysrq[srq].user_handler) {
			rt_spin_unlock_irqrestore(flags, &global.data_lock);
			return 0;
		}
	}
	if (svdidt.a || svdidt.b) {
		rt_reset_full_intr_vect(RTAI_SYS_VECTOR, svdidt);
		svdidt.a = svdidt.b = 0;
	}
	rt_spin_unlock_irqrestore(flags, &global.data_lock);
	return 0;
}

void rt_pend_linux_srq(unsigned int srq)
{
	set_bit(srq, &global.pending_srqs);
}

int rt_request_cpu_own_irq(unsigned int irq, void (*handler)(void))
{
	unsigned long flags;
	if (irq >= NR_CPU_OWN_IRQS || !handler) {
		return -EINVAL;
	}
	flags = rt_spin_lock_irqsave(&global.data_lock);
	if (cpu_own_irq[irq].handler) {
		rt_spin_unlock_irqrestore(flags, &global.data_lock);
		return -EBUSY;
	}
	cpu_own_irq[irq].dest_status = 0;
	cpu_own_irq[irq].handler = handler;
	rt_spin_unlock_irqrestore(flags, &global.data_lock);
	return 0;
}

int rt_free_cpu_own_irq(unsigned int irq)
{
	unsigned long flags;
	flags = rt_spin_lock_irqsave(&global.data_lock);
	if (irq >= NR_CPU_OWN_IRQS || !cpu_own_irq[irq].handler) {
		rt_spin_unlock_irqrestore(flags, &global.data_lock);
		return -EINVAL;
	}
	cpu_own_irq[irq].dest_status = 0;
	cpu_own_irq[irq].handler = 0;
	rt_spin_unlock_irqrestore(flags, &global.data_lock);
	return 0;
}

void rt_switch_to_linux(int cpuid)
{
	set_bit(cpuid, &global.used_by_linux);
	processor[cpuid].intr_flag = processor[cpuid].linux_intr_flag;
}

void rt_switch_to_real_time(int cpuid)
{
	processor[cpuid].linux_intr_flag = processor[cpuid].intr_flag;
	processor[cpuid].intr_flag = 0;
	clear_bit(cpuid, &global.used_by_linux);
}


/* IO_APIC entry modification code. Mostly taken from Linux io_apic.c       */

#define IO_APIC_BASE ((volatile int *)fix_to_virt(FIX_IO_APIC_BASE_0))
struct irq_2_pin_list { int apic, pin, next; };

struct IO_APIC_route_entry {
	__u32	vector		:  8,
		delivery_mode	:  3,	/* 000: FIXED
					 * 001: lowest prio
					 * 111: ExtINT */
		dest_mode	:  1,	/* 0: physical, 1: logical */
		delivery_status	:  1,
		polarity	:  1,
		irr		:  1,
		trigger		:  1,	/* 0: edge, 1: level */
		mask		:  1,	/* 0: enabled, 1: disabled */
		__reserved_2	: 15;

	union {		struct { __u32
					__reserved_1	: 24,
					physical_dest	:  4,
					__reserved_2	:  4;
			} physical;

			struct { __u32
					__reserved_1	: 24,
					logical_dest	:  8;
			} logical;
	} dest;

} __attribute__ ((packed));

#ifdef CONFIG_SMP

static inline unsigned int io_apic_read(unsigned int reg)
{
	*IO_APIC_BASE = reg;
	return *(IO_APIC_BASE+4);
}

static inline void io_apic_write(unsigned int reg, unsigned int value)
{
	*IO_APIC_BASE = reg;
	*(IO_APIC_BASE+4) = value;
}

static inline void io_apic_sync(void)
{
	(void) *(IO_APIC_BASE+4);
}

#else

static inline unsigned int io_apic_read(unsigned int reg) {return 0;}

#define io_apic_write(reg, value)

#define io_apic_sync()

#endif

static int original_saved[NR_GLOBAL_IRQS];

static struct IO_APIC_route_entry original_entry[NR_GLOBAL_IRQS];

int rt_assign_irq_to_cpu(int irq, int cpu)
{
	struct IO_APIC_route_entry entry;
	unsigned long flags;
	int pin;

	if (smp_num_cpus > 1 && cpu >= 0 && cpu < smp_num_cpus) {	
		if (irq < 0 || irq >= NR_GLOBAL_IRQS) {
			return -EINVAL;
		}
		pin = ((struct irq_2_pin_list *)rthal.irq_2_pin + irq)->pin;
		if (pin < 0) {
			return -EINVAL;
		}
		if (test_and_set_bit(0, original_saved + irq)) {
			entry =	original_entry[irq];
		} else {
			*(((int *)&entry) + 0) = io_apic_read(0x10 + 2*pin);
			*(((int *)&entry) + 1) = io_apic_read(0x11 + 2*pin);
			original_entry[irq] = entry;
		}
		entry.delivery_mode = 0;                 // fixed.
		entry.dest_mode = 0;                     // physical.
		entry.dest.physical.physical_dest = cpu; // user assigned.
		flags = hard_lock_all();
		io_apic_write(0x10 + 2*pin, *(((int *)&entry) + 0));
		io_apic_write(0x11 + 2*pin, *(((int *)&entry) + 1));
		io_apic_sync();
		hard_unlock_all(flags);
		return 0;
	}
	return smp_num_cpus;
}

int rt_reset_irq_to_sym_mode(int irq)
{
	unsigned long flags;
	int pin;

	if (smp_num_cpus > 1 && test_and_clear_bit(0, original_saved + irq)) {
		if (irq < 0 || irq >= NR_GLOBAL_IRQS) {
			return -EINVAL;
		}
		pin = ((struct irq_2_pin_list *)rthal.irq_2_pin + irq)->pin;
		if (pin < 0) {
			return -EINVAL;
		}
		flags = hard_lock_all();
		io_apic_write(0x10 + 2*pin,
			      *(((int *)(original_entry + irq)) + 0));
		io_apic_write(0x11 + 2*pin,
                              *(((int *)(original_entry + irq)) + 1));
		io_apic_sync();
		hard_unlock_all(flags);
	}
	return 0;
}


/* RTAI mount-unmount functions to be called from the application to       */
/* initialise the real time application interface, i.e. this module, only  */
/* when it is required; so that it can stay asleep when it is not needed   */

#ifdef CONFIG_SMP
static spinlock_t rtai_mount_lock = SPIN_LOCK_UNLOCKED;
#endif
static int rtai_mounted;

static void no_ack_apic_irq(void) { };

// Trivial, but we do things carefully, the blocking part is relatively short,
// should cause no troubles in the transition phase. 
// All the zeroings are strictly not required as mostly related to static data. 
// Done esplicitly for emphasis. Simple, just block other processors and grab 
// everything from Linux.  To this aim first block all the other cpus by using
// a dedicated HARD_LOCK_IPI and its vector without any protection.
void __rt_mount_rtai(void)
{
     	unsigned int i;

	rt_spin_lock(&rtai_mount_lock);
	rtai_mounted++;
	if (rtai_mounted == 1) {

		rt_spin_lock_irq(&(global.hard_lock));
#ifdef CONFIG_SMP
		if (smp_num_cpus > 1) {
			set_intr_vect(cpu_own_vector[HARD_LOCK_IPI], 
				      cpu_own_interrupt[HARD_LOCK_IPI]);
			cpu_own_irq[HARD_LOCK_IPI].handler = set_flat_dest_mode;
			cpu_own_irq[INVALIDATE_IPI].handler = invalidate_handler;
			send_ipi_shorthand(APIC_DEST_ALLBUT, HARD_LOCK_IPI);
// Be sure all are delivered.
			while (cpu_own_irq[HARD_LOCK_IPI].dest_status != (cpu_present_map & ~(1<<hard_cpu_id())));
		}
#endif

		rthal.disint   = linux_cli;
		rthal.enint    = linux_sti;
		rthal.getflags = linux_save_flags;
		rthal.setflags = linux_restore_flags;

		rthal.mask_and_ack_8259A = trpd_mask_and_ack_irq;
		rthal.unmask_8259A_irq   = trpd_unmask_irq;

// Re-ack or not re-ack APIC irqs? With the comment we let Linux re-ack since
// it seems to create no problem. Uncomment the line below if you want to test
// the other way.
		rthal.ack_APIC_irq   = no_ack_apic_irq;

		rthal.mask_IO_APIC_irq   = trpd_mask_and_ack_irq;
		rthal.unmask_IO_APIC_irq = trpd_unmask_irq;

		for (i = 0; i < NR_GLOBAL_IRQS; i++) {
			IRQ_DESC[i].handler = &trapped_linux_irq_type;
			set_intr_vect(global_vector[i], global_interrupt[i]); 
		}

		for (i = 0; i < NR_CPU_OWN_IRQS; i++) {
			set_intr_vect(cpu_own_vector[i], cpu_own_interrupt[i]);
		}

		rt_spin_unlock_irq(&(global.hard_lock));
// Be sure all processed.
		while (cpu_own_irq[HARD_LOCK_IPI].dest_status);
		if (smp_num_cpus > 1) {
			hard_cli();
			set_flat_dest_mode();
			cpu_own_irq[HARD_LOCK_IPI].dest_status = 0;
			cpu_own_irq[HARD_LOCK_IPI].handler = hard_lock_all_handler;
			hard_sti();
		}
		tuned.setup_8254 = calibrate_8254();
		printk("\n***** RTAI NEWLY MOUNTED (MOUNT COUNT %d) ******\n\n", rtai_mounted);
	} else {
		printk("\n***** RTAI ALREADY MOUNTED (MOUNT COUNT %d) ******\n\n", rtai_mounted);
	}
	rt_spin_unlock(&rtai_mount_lock);
}

// Simple, now we can simply block other processors and copy original data back
// to Linux. The HARD_LOCK_IPI is the last one to be reset.
void __rt_umount_rtai(void)
{
	int i;
	unsigned long flags;

	rt_spin_lock(&rtai_mount_lock);
	rtai_mounted--;
	if (!rtai_mounted) {

		flags = hard_lock_all();
		rthal = linux_rthal;
		for (i = 0; i < NR_GLOBAL_IRQS; i++) {
			IRQ_DESC[i].handler = linux_irq_desc_handler[i];
		}
		for (i = 0; i < 256; i++) {
			rthal.idt_table[i] = linux_idt_table[i];
		}
		cpu_own_irq[HARD_LOCK_IPI].dest_status = 0;
		cpu_own_irq[HARD_LOCK_IPI].handler = 0;
		cpu_own_irq[INVALIDATE_IPI].handler = 0;
		hard_unlock_all(flags);
		printk("\n***** RTAI UNMOUNTED (MOUNT COUNT %d) ******\n\n", rtai_mounted);
	} else {
		printk("\n***** RTAI CANNOT BE UMOUNTED (MOUNT COUNT %d) ******\n\n", rtai_mounted);
	}
	rt_spin_unlock(&rtai_mount_lock);
}

void rt_mount_rtai(void)
{
	rtai_mounted++;
	MOD_INC_USE_COUNT;
}
void rt_umount_rtai(void)
{
	rtai_mounted--;
	MOD_DEC_USE_COUNT;
}

// Module parameters to allow frequencies to be overriden via insmod
static int CpuFreq = CALIBRATED_CPU_FREQ;
MODULE_PARM(CpuFreq, "i");
#ifdef CONFIG_SMP
static int ApicFreq = CALIBRATED_APIC_FREQ;
MODULE_PARM(ApicFreq, "i");
#endif

/* module init-cleanup */

// Let's prepare our side without any problem, so that there remain just a few
// things to be done when mounting RTAI. All the zeroings are strictly not 
// required as mostly related to static data. Done esplicitly for emphasis.
int init_module(void)
{
     	unsigned int i;

	// Passed in CPU frequency overides auto detected Linux value
	if (CpuFreq == 0) {
		extern struct cpuinfo_x86 boot_cpu_data;
		extern unsigned long cpu_hz; 
		struct cpuinfo_x86 *c = &boot_cpu_data;

		if (c->x86_capability & X86_FEATURE_TSC) {
			CpuFreq = cpu_hz;
		} else {
			CpuFreq = FREQ_8254;
		}
	}
	tuned.cpu_freq = CpuFreq;

#ifdef CONFIG_SMP
	// Passed in APIC frequency overides auto detected value
	if (smp_num_cpus > 1) {
		if (ApicFreq == 0) {
			ApicFreq = apic_read(APIC_TMICT) * HZ;
		}
		tuned.apic_freq = ApicFreq;
	}
#endif
	linux_rthal = rthal;
	rtai_mounted = 0;
       	global.pending_irqs    = 0;
       	global.activ_irqs      = 0;
       	global.pending_srqs    = 0;
       	global.activ_srqs      = 0;
       	global.cpu_in_sti      = 0;
	global.used_by_linux   = ~(0xFFFFFFFF << smp_num_cpus);
	global.locked_cpus     = 0;
	global.hard_nesting    = 0;
      	spin_lock_init(&(global.data_lock));
      	spin_lock_init(&(global.hard_lock));
	for (i = 0; i < NR_IC_DEV_TYPES; i++) {
      		spin_lock_init(&(global.ic_lock[i]));
	}
	for (i = 0; i < NR_RT_CPUS; i++) {
		processor[i].intr_flag       = (1 << IFLAG) | (1 << i);
		processor[i].linux_intr_flag = (1 << IFLAG) | (1 << i);
		processor[i].pending_irqs    = 0;
		processor[i].activ_irqs      = 0;
	}
	for (i = 0; i < NR_CPU_OWN_IRQS; i++) {
		cpu_own_irq[i].dest_status = 0;
		cpu_own_irq[i].handler     = 0;
	}
        for (i = 0; i < NR_TRAPS; i++) {
                rtai_trap_handler[i] = 0;
        }
	for (i = 0; i < 256; i++) {
		linux_idt_table[i] = rthal.idt_table[i];
		linux_isr[i] = get_intr_handler(i);
	}
	mask_apic_irq = rthal.mask_IO_APIC_irq;
	for (i = 0; i < NR_GLOBAL_IRQS; i++) {
		global_irq_handler[i] = 0;
		sysrq[i].rtai_handler = 0;
		sysrq[i].user_handler = 0;
		sysrq[i].label        = 0;
		linux_irq_desc_handler[i] = IRQ_DESC[i].handler;

		ic_mask_and_ack_irq[i] = ic_ack_irq[i] =
		linux_unmask_irq[i] = ic_unmask_irq[i] = do_nothing;
		ic_lock[i] = global.ic_lock + 2;
		if ((*rthal.io_apic_irqs & (1 << i)) && rthal.irq_vector[i] > 31) {
			global_vector[i] = rthal.irq_vector[i];
			ic_mask_and_ack_irq[i] = ic_ack_irq[i] = 
			internal_ic_ack_irq[i] = ack_apic_irq;
			ic_lock[i] = global.ic_lock + 1;
			if (IRQ_DESC[i].handler->typename[8] == 'l') {
				internal_ic_ack_irq[i] =
				ic_mask_and_ack_irq[i] = mask_and_ack_apic_irq;
				linux_unmask_irq[i] =
				ic_unmask_irq[i] = rthal.unmask_IO_APIC_irq;
			}
		} else if (IRQ_DESC[i].handler->typename[0] == 'X') {
			internal_ic_ack_irq[i] =
			ic_ack_irq[i] = ack_8259_irq;
			ic_mask_and_ack_irq[i] = rthal.mask_and_ack_8259A;
			linux_unmask_irq[i] =
			ic_unmask_irq[i] = rthal.unmask_8259A_irq;
			ic_lock[i] = global.ic_lock + 0;
		}
	}

	if ((rt_printk_srq = rt_request_srq(0, rt_printk_sysreq_handler, 0)) < 0) {
		printk("RTAI-FIFO: no free srq slot available in rtai.\n");
		return -1;
	}

// Automount RTAI
	__rt_mount_rtai();
#ifdef CONFIG_PROC_FS
	rtai_proc_register();
#endif
	return 0;
}

static int trashed_local_timers_ipi, used_local_apic;

void cleanup_module(void)
{

// Remove the SRQ for rt_printk
	if (rt_free_srq(rt_printk_srq) < 0) {
		printk("RTAI-FIFO: rtai srq %d illegal or already free.\n", rt_printk_srq);
	}

// Autoumount RTAI
	__rt_umount_rtai();
#ifdef CONFIG_PROC_FS
	rtai_proc_unregister();
#endif
	if (used_local_apic) {
		printk("***** TRASHED LOCAL APIC TIMERS IPIs: %d *****\n", trashed_local_timers_ipi);
	}
	return;
}

/* ----------------------< proc filesystem section >----------------------*/

static int rtai_read_rtai(char *buf, char **start, off_t offset, int len,
	int unused)
{
	int i;
#define RTAI_UTS_RELEASE "0"
 	len = sprintf(buf, "\nRTAI Real Time Kernel, Version: %s\n\n",
					                  RTAI_UTS_RELEASE);
  	if (len > LIMIT) {
		return(len);
  	}
  	len += sprintf(buf + len, "    RTAI mount count: %d\n", rtai_mounted);
  	if (len > LIMIT) {
		return(len);
	}
#ifdef CONFIG_SMP
  	len += sprintf(buf + len, "    APIC Frequency: %d\n", tuned.apic_freq);
  	if (len > LIMIT) {
		return(len);
	}
	len += sprintf(buf + len, "    APIC Latency: %d ns\n", LATENCY_APIC);
	if (len > LIMIT) {
		return(len);
	}
	len += sprintf(buf + len, "    APIC Setup: %d ns\n", SETUP_TIME_APIC);
	if (len > LIMIT) {
		return(len);
	}
#endif
	len += sprintf(buf + len, "\nGlobal irqs used by RTAI: \n");
	if (len > LIMIT) {
		return(len);
	}
	for (i = 0; i < NR_GLOBAL_IRQS; i++) {
		if (global_irq_handler[i]) {
			if ((len += sprintf(buf + len, "%d ", i)) > LIMIT) {
				return(len);
			}
		}
	}
	len += sprintf(buf + len, "\nCpu_Own irqs used by RTAI: \n");
	if (len > LIMIT) {
		return(len);
	}
	for (i = 0; i < NR_CPU_OWN_IRQS; i++) {
		if (cpu_own_irq[i].handler) {
			if ((len += sprintf(buf + len, "%d ", i)) > LIMIT) {
				return(len);
			}
		}
	}
	len += sprintf(buf + len, "\nRTAI sysreqs in use: \n");
	if (len > LIMIT) {
		return(len);
	}
	for (i = 0; i < NR_GLOBAL_IRQS; i++) {
		if (sysrq[i].rtai_handler || sysrq[i].user_handler) {
			if ((len += sprintf(buf + len, "%d ", i)) > LIMIT) {
				return(len);
			}
		}
	}
	len += sprintf(buf + len, "\n\n");
	return(len);
}	/* End function - rtai_read_rtai */


struct proc_dir_entry *proc_rtai;
static struct proc_dir_entry *proc_rtai_rtai;

#ifdef MODULE
void rtai_modcount(struct inode *inode, int fill)
{
	if(fill)
		MOD_INC_USE_COUNT;
	else
		MOD_DEC_USE_COUNT;
}
#endif

static int rtai_proc_register(void)
{
	proc_rtai = create_proc_entry("rtai", S_IFDIR, 0);

#ifdef MODULE
	proc_rtai->fill_inode = rtai_modcount;
#endif

	proc_rtai_rtai = create_proc_entry("rtai", 0, proc_rtai);
	proc_rtai_rtai->get_info = rtai_read_rtai;

	return(0);
}	/* End function - rtai_proc_register */


static void rtai_proc_unregister(void) 
{
	remove_proc_entry("rtai",proc_rtai);
	remove_proc_entry("rtai",&proc_root);
}	/* End function - rtai_proc_unregister */

/* ------------------< end of proc filesystem section >------------------*/

/*************** SOME TIMER FUNCTIONS TO BE LIKELY PUT ELSWHERE **************/

/* Real time timers. No oneshot, and related timer programming, calibration. */
/* Use the utility module. It is also to be decided if this stuff has to     */
/* stay here.                                                                */

struct calibration_data tuned;
struct rt_times rt_times;
struct rt_times rt_smp_times[NR_RT_CPUS];
static int use_local_apic;

// The two functions below are for 486s.

#define COUNTER_2_LATCH 0xFFFE
static int last_8254_counter2; 
static RTIME ts_8254;

RTIME rd_8254_ts(void)
{
	RTIME t;
	unsigned long flags;
	int inc, c2;

	rt_global_save_flags(&flags);
	rt_global_cli();
//	outb(0x80, 0x43);
	outb(0xD8, 0x43);
      	c2 = inb(0x42);
	inc = last_8254_counter2 - (c2 |= (inb(0x42) << 8));
	last_8254_counter2 = c2;
	t = (ts_8254 += (inc > 0 ? inc : inc + COUNTER_2_LATCH));
	rt_global_restore_flags(flags);
	return t;
}

void rt_setup_8254_tsc(void)
{
	unsigned long flags;
	int c;

	flags = hard_lock_all();
	outb_p(0x00, 0x43);
	c = inb_p(0x40);
	c |= inb_p(0x40) << 8;
	outb_p(0xB4, 0x43);
	outb_p(COUNTER_2_LATCH & 0xFF, 0x42);
	outb_p(COUNTER_2_LATCH >> 8, 0x42);
	ts_8254 = c + ((RTIME)LATCH)*jiffies;
	last_8254_counter2 = 0; 
	outb_p((inb_p(0x61) & 0xFD) | 1, 0x61);
	hard_unlock_all(flags);
}

static void trap_trashed_local_timers_ipi(void)
{ 
	trashed_local_timers_ipi++;
	return;
}

#ifdef CONFIG_SMP
void broadcast_to_local_timers(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned long temp;
	temp = (apic_read(APIC_ICR) & ~0xCDFFF) |
	       (APIC_DEST_DM_FIXED | APIC_DEST_ALLINC | LOCAL_TIMER_VECTOR);
	apic_write(APIC_ICR, temp);
} 
#else
static void broadcast_to_local_timers(int irq, void *dev_id, struct pt_regs *regs) { }
#endif


void rt_request_timer(void (*handler)(void), unsigned int tick, int apic)
{
#define WAIT_LOCK 5

	int count;
	unsigned long flags;
	used_local_apic = use_local_apic = apic;
	flags = hard_lock_all();
	do {
		outb(0x00, 0x43);
		count = inb(0x40);
	} while	((count | (inb(0x40) << 8)) > WAIT_LOCK);
	rt_times.tick_time = rdtsc();
	if (tick > 0) {
#ifdef CONFIG_SMP
		rt_times.linux_tick = use_local_apic ? APIC_ICOUNT : LATCH;
#else
		rt_times.linux_tick = LATCH;
#endif
		rt_times.tick_time = ((RTIME)rt_times.linux_tick)*(jiffies + 1);
		rt_times.intr_time = rt_times.tick_time + tick;
		rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
		rt_times.periodic_tick = tick;
		if (use_local_apic) {
			global.hard_lock_all_service = 2;
			rt_free_cpu_own_irq(RTAI_APIC_TIMER_IPI);
			rt_request_cpu_own_irq(RTAI_APIC_TIMER_IPI, handler);
//			rt_request_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers, "broadcast", broadcast_to_local_timers);
			setup_periodic_apic(tick, RTAI_APIC_TIMER_VECTOR);
		} else {
			outb(0x34, 0x43);
			outb(tick & 0xFF, 0x40);
			outb(tick >> 8, 0x40);
			rt_free_global_irq(TIMER_8254_IRQ);
			rt_request_global_irq(TIMER_8254_IRQ, handler);
		}
	} else {
		rt_times.linux_tick = imuldiv(LATCH, tuned.cpu_freq, FREQ_8254);
//		rt_times.tick_time = rdtsc();
		rt_times.intr_time = rt_times.tick_time + rt_times.linux_tick;
		rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
		rt_times.periodic_tick = rt_times.linux_tick;
		if (use_local_apic) {
			global.hard_lock_all_service = 2;
			rt_free_cpu_own_irq(RTAI_APIC_TIMER_IPI);
			rt_request_cpu_own_irq(RTAI_APIC_TIMER_IPI, handler);
//			rt_request_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers, "broadcast", broadcast_to_local_timers);
			setup_oneshot_apic(APIC_ICOUNT, RTAI_APIC_TIMER_VECTOR);
		} else {
			outb(0x30, 0x43);
			outb(LATCH & 0xFF, 0x40);
			outb(LATCH >> 8, 0x40);
			rt_free_global_irq(TIMER_8254_IRQ);
			rt_request_global_irq(TIMER_8254_IRQ, handler);
		}
	}
	hard_unlock_all(flags);
	rt_request_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers, "broadcast", broadcast_to_local_timers);
	return;
}

void rt_free_timer(void)
{
	unsigned long flags;
	rt_free_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers);
	flags = hard_lock_all();
	if (use_local_apic) {
		global.hard_lock_all_service = 3;
//		rt_free_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers);
		setup_periodic_apic(APIC_ICOUNT, LOCAL_TIMER_VECTOR);
		rt_free_cpu_own_irq(RTAI_APIC_TIMER_IPI);
		rt_request_cpu_own_irq(RTAI_APIC_TIMER_IPI, trap_trashed_local_timers_ipi);
		use_local_apic = 0;
	} else {
		outb(0x34, 0x43);
		outb(LATCH & 0xFF, 0x40);
		outb(LATCH >> 8, 0x40);
		rt_free_global_irq(TIMER_8254_IRQ);
	}
	hard_unlock_all(flags);
}

#ifdef CONFIG_SMP

void rt_request_apic_timers(void (*handler)(void), struct apic_timer_setup_data *apic_timer_data)
{
#define WAIT_LOCK 5
	int count, cpuid;
	unsigned long flags;
	struct apic_timer_setup_data *p;
	volatile struct rt_times *rt_times;

	flags = hard_lock_all();
	global.hard_lock_all_service = 1;
	do {
		outb(0x00, 0x43);
		count = inb(0x40);
	} while	((count | (inb(0x40) << 8)) > WAIT_LOCK);
	apic_timers_sync_time = rdtsc() + imuldiv(LATCH, tuned.cpu_freq, FREQ_8254);
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		*(p = apic_timer_mode + cpuid) = apic_timer_data[cpuid];
		rt_times = rt_smp_times + cpuid;
		if (p->mode > 0) {
			p->mode = 1;
			rt_times->linux_tick = APIC_ICOUNT;
			rt_times->tick_time = llimd(apic_timers_sync_time, tuned.apic_freq, tuned.cpu_freq);
			rt_times->periodic_tick = 
			p->count = imuldiv(p->count, tuned.apic_freq, NANO);
		} else {
			p->mode =  0;
			rt_times->linux_tick = imuldiv(LATCH, tuned.cpu_freq, FREQ_8254);
			rt_times->tick_time = apic_timers_sync_time;
			rt_times->periodic_tick = rt_times->linux_tick;
			p->count = APIC_ICOUNT;
		}
		rt_times->intr_time = rt_times->tick_time + rt_times->periodic_tick;
		rt_times->linux_time = rt_times->tick_time + rt_times->linux_tick;
	}
	if ((p = apic_timer_mode + hard_cpu_id())->mode) {
		while (rdtsc() < apic_timers_sync_time);
		setup_periodic_apic(p->count, RTAI_APIC_TIMER_VECTOR);
	} else {
		while (rdtsc() < apic_timers_sync_time);
		setup_oneshot_apic(p->count, RTAI_APIC_TIMER_VECTOR);
	}
	hard_unlock_all(flags);
	rt_free_cpu_own_irq(RTAI_APIC_TIMER_IPI);
	rt_request_cpu_own_irq(RTAI_APIC_TIMER_IPI, handler);
	rt_request_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers, "broadcast", broadcast_to_local_timers);
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		if ((p = apic_timer_data + cpuid)->mode > 0) {
			p->mode = 1;
			p->count = imuldiv(p->count, tuned.apic_freq, NANO);
		} else {
			p->mode = 0;
			p->count = imuldiv(p->count, tuned.cpu_freq, NANO);
		}
	}
}

void rt_free_apic_timers(void)
{
	unsigned long flags;

	rt_free_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers);
	flags = hard_lock_all();
	global.hard_lock_all_service = 3;
//	rt_free_linux_irq(TIMER_8254_IRQ, broadcast_to_local_timers);
	setup_periodic_apic(APIC_ICOUNT, LOCAL_TIMER_VECTOR);
	rt_free_cpu_own_irq(RTAI_APIC_TIMER_IPI);
	rt_request_cpu_own_irq(RTAI_APIC_TIMER_IPI, trap_trashed_local_timers_ipi);
	hard_unlock_all(flags);
}

#else

void rt_request_apic_timers(void (*handler)(void), struct apic_timer_setup_data *apic_timer_data) { }

#define rt_free_apic_timers() rt_free_timer()

#endif

int calibrate_8254(void)
{
	unsigned long flags, i;
	RTIME t;
	flags = hard_lock_all();
	outb(0x34, 0x43);
	t = rdtsc();
	for (i = 0; i < 10000; i++) { 
		outb(LATCH & 0xFF, 0x40);
		outb(LATCH >> 8, 0x40);
	}
	i = rdtsc() - t;
	hard_unlock_all(flags);
	return imuldiv(i, 100000, tuned.cpu_freq);
}

#ifdef SELF_CALIBRATE	// No longer used
static struct {
    	struct wait_queue *wq;
	int duration;
	int freq;
	int tick_count;
} calibrate_data;

static void calibrate_cpu(void)
{
    	static int count;
	static RTIME tbase;
	long linux_cr0, fpu_reg[27];
	double freq;
	union {unsigned long long time; unsigned long time_lh[2];} tsc;

	tsc.time = rdtsc();
	if (calibrate_data.tick_count++ < 0) {
	    	tbase = tsc.time;
		count = 0;
	}
	if (++count == HZ) {
	    	tsc.time -= tbase;
		__asm__ __volatile__("movl %%cr0,%%eax": "=a" (linux_cr0):);
		__asm__ __volatile__("clts; fnsave %0" : "=m" (fpu_reg));
		freq  = (double) tsc.time_lh[1] * (double) 0x100000000LL 
		      + (double) tsc.time_lh[0];
		count = (freq * CLOCK_TICK_RATE) 
		      / (((double) calibrate_data.tick_count) * LATCH) 
		      + 0.4999999999999;
		__asm__ __volatile__("frstor %0"       : "=m" (fpu_reg));
		__asm__ __volatile__("movl %%eax,%%cr0": :"a" (linux_cr0));
		calibrate_data.freq = count;
		printk( "Calibrating CPU Frequency.... %d Hz (%ds)\n", 
			calibrate_data.freq, 
			calibrate_data.tick_count/HZ + 1
			);
		count = 0;
	}
	if ((calibrate_data.tick_count/HZ + 1) > calibrate_data.duration) {
		wake_up_interruptible(&calibrate_data.wq);
	}
}

int calibrate_cpu_freq(int duration_secs)
{
    	calibrate_data.tick_count = -1;
    	calibrate_data.duration = duration_secs;
    	rt_request_global_irq(TIMER_8254_IRQ, calibrate_cpu);
	interruptible_sleep_on(&calibrate_data.wq);
	rt_free_timer();
	rt_free_global_irq(TIMER_8254_IRQ);
	return calibrate_data.freq;
}

#ifdef CONFIG_SMP
static void calibrate_apic(void)
{
    	unsigned long temp;
	static int count;
	static RTIME tbase;
	long linux_cr0, fpu_reg[27];
	double freq;
	union {unsigned long long time; unsigned long time_lh[2];} apic;
	
	apic.time = apic_read(APIC_TMCCT);
	if (calibrate_data.tick_count++ < 0) {
	    	tbase = apic.time;
		count = 0;
	}
	if (++count == HZ) {
	    	apic.time = tbase - apic.time;
		__asm__ __volatile__("movl %%cr0,%%eax": "=a" (linux_cr0):);
		__asm__ __volatile__("clts; fnsave %0" : "=m" (fpu_reg));
		freq  = (double) apic.time_lh[1] * (double) 0x100000000LL 
		      + (double) apic.time_lh[0];
		count = (freq * CLOCK_TICK_RATE)
		      / (((double) calibrate_data.tick_count) * LATCH) 
		      + 0.4999999999999;
		__asm__ __volatile__("frstor %0"       : "=m" (fpu_reg));
		__asm__ __volatile__("movl %%eax,%%cr0": :"a" (linux_cr0));
		calibrate_data.freq = count;
		printk( "Calibrating APIC Frequency... %d Hz (%ds)\n", 
			calibrate_data.freq, 
			calibrate_data.tick_count/HZ + 1
			);
		count = 0;
	}
	temp = (apic_read(APIC_ICR) & (~0xCDFFF)) |
	       (APIC_DEST_DM_FIXED | APIC_DEST_ALLINC | LOCAL_TIMER_VECTOR);
	apic_write(APIC_ICR, temp);
	if ((calibrate_data.tick_count/HZ + 1) > calibrate_data.duration) {
	    	wake_up_interruptible(&calibrate_data.wq);
	}
}

static void just_ret(void)
{
    	return;
}

int calibrate_apic_freq(int duration_secs)
{
    	calibrate_data.tick_count = -1;
	calibrate_data.duration = duration_secs;
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, hard_cpu_id());
	rt_request_timer(just_ret, (unsigned int)(4*NANO), 1);
	rt_request_global_irq(TIMER_8254_IRQ, calibrate_apic);
	interruptible_sleep_on(&calibrate_data.wq);
	rt_free_timer();
	rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
	rt_free_global_irq(TIMER_8254_IRQ);
	return calibrate_data.freq;
}
#endif  // CONFIG_SMP
#endif  // SELF_CALIBRATE

/********** END OF SOME TIMER FUNCTIONS TO BE LIKELY PUT ELSEWHERE ***********/

#include <linux/console.h>

int rtai_print_to_screen(const char *format, ...)
{
        static spinlock_t display_lock = SPIN_LOCK_UNLOCKED;
        static char display[25*80];
        unsigned long flags;
        struct console *c;
        va_list args;
        int len;

        flags = rt_spin_lock_irqsave(&display_lock);
        va_start(args, format);
        len = vsprintf(display, format, args);
        va_end(args);
        c = console_drivers;
        while(c) {
                if ((c->flags & CON_ENABLED) && c->write)
                        c->write(c, display, len);
                c = c->next;
        }
        rt_spin_unlock_irqrestore(flags, &display_lock);

        return len;
}

/*
 *  rt_printk.c, hacked from linux/kernel/printk.c.
 *
 * Modified for RT support, David Schleef.
 *
 * Adapted to RTAI and restyled his way by Paolo Mantegazza.
 */

/* PRINTK_BUF_LEN must be a power of 2 */
#define PRINTK_BUF_LEN	(8192)
#define TEMP_BUF_LEN	(512)
#define PRINTK_BUF_LEN_MASK	(PRINTK_BUF_LEN-1)

static char rt_printk_buf[PRINTK_BUF_LEN];

/* buf_front may only be written by RT */
static unsigned int buf_front;

/* buf_back may only be read/written by non-RT */
static unsigned int buf_back;

static char buf[TEMP_BUF_LEN];

int rt_printk(const char *fmt, ...)
{
        static spinlock_t rt_printk_lock = SPIN_LOCK_UNLOCKED;
	va_list args;
	int len, i;
	unsigned long flags;
	unsigned int buf_offset;

        flags = rt_spin_lock_irqsave(&rt_printk_lock);
	va_start(args, fmt);
	len = vsprintf(buf, fmt, args);
	va_end(args);
	buf_offset = buf_front&PRINTK_BUF_LEN_MASK;
	if (buf_offset + len >= PRINTK_BUF_LEN) {
		i = PRINTK_BUF_LEN - buf_offset;
		memcpy(rt_printk_buf + buf_offset, buf, i);
		memcpy(rt_printk_buf, buf + i, len - i);
	} else {
		memcpy(rt_printk_buf + buf_offset, buf, len);
	}
	buf_front += len;
        rt_spin_unlock_irqrestore(flags, &rt_printk_lock);
	rt_pend_linux_srq(rt_printk_srq);

	return len;
}

static void rt_printk_sysreq_handler(void)
{
	unsigned int tmp_front;
	unsigned int offset;
	int n;
	
	while(buf_back < (tmp_front = buf_front)){
		n = tmp_front - buf_back;
		offset = buf_back&PRINTK_BUF_LEN_MASK;
		if(offset+n>=PRINTK_BUF_LEN){
			n=PRINTK_BUF_LEN-offset;
		}
		printk("%.*s", n, rt_printk_buf + offset);
		buf_back+=n;
	}
}
