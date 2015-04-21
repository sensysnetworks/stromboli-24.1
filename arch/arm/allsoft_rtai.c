/* 020222 rtai/arch/arm/rtai.c
COPYRIGHT (C) 2002 Guennadi Liakhovetski, DSA GmbH (gl@dsa-ac.de)
COPYRIGHT (C) 2002 Wolfgang Müller (wolfgang.mueller@dsa-ac.de)
COPYRIGHT (C) 2002 Thomas Gleixner (gleixner@autronix.de)
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
/*
--------------------------------------------------------------------------
Changelog

03-10-2002	TG	added support for trap handling
			added function rt_is_linux
*/

#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>

#include <asm/system.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <asm/atomic.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#endif

#include <asm/rtai.h>
#include <asm/rtai_srq.h>

#undef CONFIG_RTAI_MOUNT_ON_LOAD

// proc filesystem additions.
#ifdef CONFIG_PROC_FS
static int rtai_proc_register(void);
static void rtai_proc_unregister(void);
#endif
// End of proc filesystem additions.

/* Some define */

#define NR_RTAI_IRQS  	 	NR_IRQS
#define NR_SYSRQS  	 	32
#define LAST_GLOBAL_RTAI_IRQ 	NR_RTAI_IRQS - 1
#define NR_TRAPS		32

/* these are prototypes for timer-handling abstraction */
void linux_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs);
void soft_timer_interrupt(rtai_irq_t irq, void *dev_id, struct pt_regs *regs);
void rt_request_timer(void (*handler)(void), unsigned int tick, int unused);
void rt_free_timer(void);

#define MAX_PENDING_IRQS 128 /* MUST be a power of 2!!! */

static struct global_rt_status {
//	volatile rtai_irq_mask_t pending_irqs; // bitmask
//	unsigned int * pending_irqs; // array
	volatile int irq_in, irq_out, lost_irqs;
	volatile rtai_irq_t pending_irqs[MAX_PENDING_IRQS];
	volatile rtai_irq_mask_t active_irqs;
	volatile rtai_irq_mask_t pending_srqs;
//	unsigned int * pending_srqs; // array
	volatile rtai_irq_mask_t active_srqs;
	volatile unsigned int cpu_in_sti;
	volatile unsigned int used_by_linux;
  	volatile unsigned int locked_cpus;
  	volatile unsigned int hard_nesting;
	volatile unsigned int hard_lock_all_service;
	spinlock_t hard_lock;
	spinlock_t data_lock;
	spinlock_t ic_lock;
} global;

volatile unsigned int *locked_cpus = &global.locked_cpus;

// ?????????????????????????????????????????????
/* VERY IMPORTANT!!!The struct below must be kept as the same in irq.c, caring*/
/* of the use of the struct ic_ops to ease porting to RTAI         */
/* (There should be no alignement problem within the structure, in any case   */
/* let's keep an eye on it                                                    */
/* ALSO VERY IMPORTANT, since I saw no way to just ack, we mask_ack always, so*/
/* it is likely we have to recall to set an arch dependent call to unmask in  */
/* in the scheduler timer handler. Other arch allow just to ack, maybe we'll  */
/* we can get along as it is now, let's recall this point.                    */

#include <asm/irqops.h>

struct ic_ops {
        void (*mask_ack)(rtai_irq_t irq);
        void (*mask)    (rtai_irq_t irq);
        void (*unmask)  (rtai_irq_t irq);
};

struct irqdesc {
        unsigned int     nomask       :  1;
        unsigned int     enabled      :  1;
        unsigned int     triggered    :  1;
        unsigned int     probing      :  1;
        unsigned int     probe_ok     :  1;
        unsigned int     valid        :  1;
        unsigned int     noautoenable :  1;
        unsigned int     unused       : 25;
	struct ic_ops    pic;
        struct irqaction *action;
        unsigned int     lck_cnt;
        unsigned int     lck_pc;
        unsigned int     lck_jif;
};

extern struct irqdesc irq_desc[];

/* Most of our data */

static struct irq_handling {
	volatile unsigned long dest_status;
	void (*handler)(void);
} global_irq[NR_IRQS];

static struct irqdesc linux_irq_desc_handler[NR_IRQS];

static struct sysrq_t {
	unsigned int label;
	void (*rtai_handler)(void);
	long long (*user_handler)(unsigned int whatever);
} sysrq[NR_SYSRQS];

static RT_TRAP_HANDLER rtai_trap_handler[NR_TRAPS];

volatile unsigned long lxrt_hrt_flags;

// The main items to be saved-restored to make Linux our humble slave

static struct rt_hal linux_rthal;

static struct pt_regs rtai_regs;  // Dummy registers.

static void *saved_timer_action_handler; // Saved timer-action handler

static struct cpu_own_status {
	volatile unsigned int intr_flag;
	volatile unsigned int linux_intr_flag;
	volatile rtai_irq_mask_t pending_irqs;
	volatile rtai_irq_mask_t active_irqs;
} processor[NR_RT_CPUS];

#define MAX_IRQS  32
#define MAX_LVEC  (MAX_IRQS + sizeof(unsigned long) - 1)/sizeof(unsigned long)

static struct cpu_own_status {
	volatile unsigned long intr_flag, linux_intr_flag, hvec;
	volatile unsigned long lvec[MAX_LVEC], irqcnt[MAX_IRQS], irq[MAX_IRQS];
}

#define VECTRANS(vec) (vec)

void send_ipi_shorthand(unsigned int shorthand, rtai_irq_t irq) { }

void send_ipi_logical(unsigned long dest, rtai_irq_t irq) { }

//static void hard_lock_all_handler(void) { }

union rtai_tsc rtai_tsc;
unsigned long rtai_timer_crash, rtai_big_delay;

#ifdef CONFIG_RTAI_ALLSOFT

void hard_cli(void)
{ 
	hprocessor[hard_cpu_id()].intr_flag = 0;
}

void hard_sti(void)
{
       	unsigned long hvec, lvec, vec, irq;
       	struct hcpu_own_status *cpu;

	cpu = hprocessor + hard_cpu_id();
	do {
		cpu->intr_flag = 0;
		if ((hvec = cpu->hvec)) {
			test_and_clear_bit(hvec = ffnz(hvec), &cpu->hvec);
			if ((lvec = cpu->lvec[hvec])) {
				test_and_clear_bit(lvec = ffnz(lvec), &cpu->lvec[hvec]);
				vec = (hvec << HVEC_SHIFT) | lvec;
				if (cpu->irqcnt[vec] > 0) {
					atomic_dec((atomic_t *)&cpu->irqcnt[vec]);
					if ((irq = cpu->irq[vec]) > 1000) {
						cpu_own_irq[irq - 1000].handler();
					} else {
						((int (*)(int, unsigned long))global_irq[irq].handler)(irq, global_irq[irq].data);
					}
				}
				if (cpu->irqcnt[vec] > 0) {
					test_and_set_bit(lvec, &cpu->lvec[hvec]);
				}
			}
			if (cpu->lvec[hvec]) {
				test_and_set_bit(hvec, &cpu->hvec);
			}
		}	
		hprocessor[hard_cpu_id()].intr_flag = INTR_ENABLED;
	} while ((cpu = hprocessor + hard_cpu_id())->hvec);
	return;
}

void hard_restore_flags(unsigned long flags)
{
	if (flags) {
		hard_sti();
	} else {
		hprocessor[hard_cpu_id()].intr_flag = 0;
	}
}

unsigned long __hard_save_flags(void)
{
	unsigned long flags;
	flags = hprocessor[hard_cpu_id()].intr_flag;
	return flags;
}

unsigned long __hard_save_flags_cli(void)
{
	unsigned long flags;
	flags = xchg(&(hprocessor[hard_cpu_id()].intr_flag), 0);
	return flags;
}

#endif

static void linux_cli(void)
{
	processor[hard_cpu_id()].intr_flag = 0;
}

static void (*ic_unmask_irq[NR_IRQS])(rtai_irq_t irq);

static void linux_sti(void)
{
	rtai_irq_t irq;
       	unsigned long cpuid;
	unsigned int intr_enabled;
       	struct cpu_own_status *cpu;
	static unsigned long timer_crash = 0;

	if ( rtai_timer_crash != timer_crash ) {
		timer_crash = rtai_timer_crash;
		printk( "Timer crashed again! (No. %lu, delay %lu)", timer_crash, rtai_big_delay );
	}

	cpu = processor + (cpuid = hard_cpu_id());
	cpu->intr_flag = intr_enabled = IBIT | (1 << cpuid);
	while (!test_and_set_bit(cpuid, &global.cpu_in_sti)) {
	       	while (have_pending_irq() || have_pending_srq()) {
			// Global IRQ Handling - same as on PPC
			rt_spin_lock_irq(&(global.data_lock));
			if ( (irq = pending_irq()) != -1 ) {
				activate_irq(irq);
				clear_pending_irq(irq);
				rt_spin_unlock_irq(&(global.data_lock));
				cpu->intr_flag = 0; /* cli */

	        	        // ** call old Linux do_IRQ() to handle IRQ
				linux_rthal.do_IRQ(irq, &rtai_regs);

				/* Unmasking is done in do_IRQ above - don't do twice */
				deactivate_irq(irq);
				cpu->intr_flag = intr_enabled;
			} else {
				rt_spin_unlock_irq(&(global.data_lock));
			}

			// Local IRQ Handling - missing here ... only on SMP

			// SRQ Handling - same as on PPC
			rt_spin_lock_irq(&(global.data_lock));
			if ( (irq = pending_srq()) != -1 ) {
				activate_srq(irq);
				clear_pending_srq(irq);
				rt_spin_unlock_irq(&(global.data_lock));
				if (sysrq[irq].rtai_handler) {
					sysrq[irq].rtai_handler();
				}
				deactivate_srq(irq);
			} else {
				rt_spin_unlock_irq(&(global.data_lock));
			}

		}
		clear_bit(cpuid, &global.cpu_in_sti);
		if ( ! ( have_pending_irq() || have_pending_srq() ) ) break;
	}
}

/* we need to return faked, but real flags
 *
 * imagine a function calling our linux_save_flags() while rtai is loaded
 * and restoring flags when rtai is unloaded. CPSR is broken ...
 */
static unsigned int linux_save_flags(void)
{
	unsigned long flags;

	hard_save_flags( flags );
	/* check if we are in CLI, then set I bit in flags */
	return (flags & ~IBIT) | ( processor[hard_cpu_id()].intr_flag ? 0 : IBIT );
}

static void linux_restore_flags(unsigned int flags)
{
	/* check if CLI-bit is set */
	if (flags & IBIT) {
		processor[hard_cpu_id()].intr_flag = 0;
	} else {
		linux_sti();
	}
}

unsigned int linux_save_flags_and_cli(void)
{
	unsigned long flags;

	flags = linux_save_flags();
        processor[hard_cpu_id()].intr_flag = 0;
	return flags;
}

unsigned long linux_save_flags_and_cli_cpuid(int cpuid)
{
	return linux_save_flags_and_cli();
}

void rtai_just_copy_back(unsigned long flags, int cpuid)
{
	/* also check if CLI-bit is set and set up intr_flag accordingly */
	if (flags & IBIT) {
	        processor[cpuid].intr_flag = 0;
	} else {
	        processor[cpuid].intr_flag = IBIT | (1 << cpuid);
	}
}

// For the moment we just do mask_ack_unmask, maybe it has to be adjusted

static void (*ic_mask_and_ack_irq[NR_IRQS]) (rtai_irq_t irq);
static void (*ic_mask_irq[NR_IRQS])         (rtai_irq_t irq);
static void (*linux_unmask_irq[NR_IRQS])    (rtai_irq_t irq);

static void do_nothing_picfun(rtai_irq_t irq) { };

unsigned int rt_startup_irq(rtai_irq_t irq)
{
	unsigned int flags;
	struct irqdesc *irq_desc;

	if ((irq_desc = &linux_irq_desc_handler[irq]) && irq_desc->pic.unmask) {
		flags = rt_spin_lock_irqsave(&global.ic_lock);
		irq_desc->probing = 0;
		irq_desc->triggered = 0;
		irq_desc->enabled = 1;
		irq_desc->pic.unmask(irq);
		rt_spin_unlock_irqrestore(flags, &global.ic_lock);
	}
	return 0;
}

void rt_shutdown_irq(rtai_irq_t irq)
{
	unsigned int flags;
	struct irqdesc *irq_desc;

	if ((irq_desc = &linux_irq_desc_handler[irq]) && irq_desc->pic.mask) {
		flags = rt_spin_lock_irqsave(&global.ic_lock);
		irq_desc->enabled = 0;
		irq_desc->pic.mask(irq);
		rt_spin_unlock_irqrestore(flags, &global.ic_lock);
	}
}

void rt_enable_irq(rtai_irq_t irq)
{
	unsigned int flags;
	struct irqdesc *irq_desc;

	if ((irq_desc = &linux_irq_desc_handler[irq]) && irq_desc->pic.unmask) {
		flags = rt_spin_lock_irqsave(&global.ic_lock);
		irq_desc->probing = 0;
		irq_desc->triggered = 0;
		irq_desc->enabled = 1;
		irq_desc->pic.unmask(irq);
		rt_spin_unlock_irqrestore(flags, &global.ic_lock);
	}
}

void rt_disable_irq(rtai_irq_t irq)
{
	unsigned int flags;
	struct irqdesc *irq_desc;

	if ((irq_desc = &linux_irq_desc_handler[irq]) && irq_desc->pic.mask) {
		flags = rt_spin_lock_irqsave(&global.ic_lock);
		irq_desc->enabled = 0;
		irq_desc->pic.mask(irq);
		rt_spin_unlock_irqrestore(flags, &global.ic_lock);
	}
}

void rt_mask_ack_irq(rtai_irq_t irq)
{
	unsigned int flags;

	flags = rt_spin_lock_irqsave(&global.ic_lock);
	if(ic_mask_and_ack_irq[irq])
		ic_mask_and_ack_irq[irq](irq);
#if 0//def CONFIG_ARCH_SA1100
	if ( irq == TIMER_8254_IRQ )
		OSSR = OSSR_M0;
#endif
	rt_spin_unlock_irqrestore(flags, &global.ic_lock);
}

void rt_mask_irq(rtai_irq_t irq)
{
	unsigned int flags;

	flags = rt_spin_lock_irqsave(&global.ic_lock);
	if(ic_mask_irq[irq])
		ic_mask_irq[irq](irq);
	rt_spin_unlock_irqrestore(flags, &global.ic_lock);
}

void rt_unmask_irq(rtai_irq_t irq)
{
	unsigned int flags;

	flags = rt_spin_lock_irqsave(&global.ic_lock);
	if(ic_unmask_irq[irq])
		ic_unmask_irq[irq](irq);
	rt_spin_unlock_irqrestore(flags, &global.ic_lock);
}

//?????????? a real time handler must unmask ASAP, likely to force changes in
// other RTAI modules

#define MAX_PEND_IRQ 10
#define RT_PRINTK(x, y) //rt_printk(x, y)

static int dispatch_global_irq(struct fill_t fill, int irq) __attribute__ ((__unused__));
static int dispatch_global_irq(struct fill_t fill, int irq)
{
	static unsigned long kbrdirq = 0;
       	struct scpu_own_status *cpu;
	volatile unsigned long lflags, *lflagp, cpuid;

	cpu = sprocessor + (cpuid = hard_cpu_id());
	lflags = xchg(lflagp = &cpu->intr_flag, 0);
	if (global_irq[irq].handler) {
		internal_ic_ack_irq[irq](irq);
        	*lflagp = lflags;
#ifdef CONFIG_RTAI_ALLSOFT
		do {	
			struct hcpu_own_status *cpu;
			int vector;
			cpu = hprocessor + cpuid;
			vector = VECTRANS(global_vector[irq]);
			cpu->irq[vector] = irq;
			if (cpu->irqcnt[vector]++ >= MAX_PEND_IRQ) {
				RT_PRINTK("REAL TIME EXTERNAL INTERRUPT FLOOD ON VECTOR 0x%x\n", global_vector[irq]);
			}
			test_and_set_bit(vector & LVEC_MASK, &cpu->lvec[vector = vector >> HVEC_SHIFT]);
			test_and_set_bit(vector, &cpu->hvec);
			if (cpu->intr_flag) {
				cpu->intr_flag = 0;
				HARD_STI();
				hard_sti();	
			}
		} while (0);
#else
		if (global_irq[irq].ext) {
			if (((int (*)(int, unsigned long))global_irq[irq].handler)(irq, global_irq[irq].data)) {
				HARD_CLI();
				return 0;
			}
		} else {
			((void (*)(int))global_irq[irq].handler)(irq);
		}
#endif
		HARD_CLI();
		lflags = xchg(lflagp = &sprocessor[cpuid = hard_cpu_id()].intr_flag, 0);
	} else {
		ic_ack_irq[irq](irq);
		if (irq == KBRD_IRQ) {
			kbrdirq = 1;
		} else {
			int vector;
			vector = VECTRANS(global_vector[irq]);
			if (cpu->irqcnt[vector]++ >= MAX_PEND_IRQ) {
				RT_PRINTK("LINUX EXTERNAL INTERRUPT FLOOD ON VECTOR 0x%x\n", global_vector[irq]);
			}
			test_and_set_bit(vector & LVEC_MASK, &cpu->lvec[vector = vector >> HVEC_SHIFT]);
			test_and_set_bit(vector, &cpu->hvec);
		}
	}
	if (hprocessor[cpuid].intr_flag && lflags) {
		HARD_STI();
		if (test_and_clear_bit(0, &kbrdirq)) {
			do_linux_irq(global_vector[KBRD_IRQ]);
		}
		linux_sti(0);
		return 1;
	}	
        *lflagp = lflags;
	return 0;
}

asmlinkage void dispatch_irq(rtai_irq_t irq, struct pt_regs *regs)
{
        struct scpu_own_status *cpu;
        volatile unsigned long lflags, *lflagp, cpuid;

	rt_spin_lock(&global.ic_lock);

	if (irq >= 0 && irq<NR_IRQS) {

		if(ic_mask_and_ack_irq[irq]) {
			ic_mask_and_ack_irq[irq](irq);
		}
		rt_spin_unlock(&global.ic_lock);

		// We just care about our own RT-Handlers installed

	        cpu = sprocessor + (cpuid = hard_cpu_id());
        	lflags = xchg(lflagp = &cpu->intr_flag, 0);
		if (global_irq[irq].handler) {
#ifdef CONFIG_RTAI_ALLSOFT
	                do {
        	                struct hcpu_own_status *cpu;
                	        int vector;
                        	cpu = hprocessor + cpuid;
                	        vector = VECTRANS(global_vector[irq]);
                        	cpu->irq[vector] = irq;
                   	     if (cpu->irqcnt[vector]++ >= MAX_PEND_IRQ) {
                        	        RT_PRINTK("REAL TIME EXTERNAL INTERRUPT FLOOD ON VECTOR 0x%x\n", global_vector[irq]);
              	          }
                	        test_and_set_bit(vector & LVEC_MASK, &cpu->lvec[vector = vector >> HVEC_SHIFT]);
                        	test_and_set_bit(vector, &cpu->hvec);
                      	  if (cpu->intr_flag) {
                        	        cpu->intr_flag = 0;
                                	HARD_STI();
                            	    hard_sti();
                        	}
                	} while (0);
#else
			((void (*)(int))global_irq[irq].handler)(irq);
#endif
                	HARD_CLI();
                	lflags = xchg(lflagp = &sprocessor[cpuid = hard_cpu_id()].intr_flag, 0);
			//?????????? we unmask in any case, just in case. However this is not enough
			// for the RTAI schedulers as it will block preemption. We must unmask their
			// ASAP
			if(ic_unmask_irq[irq]) {
				ic_unmask_irq[irq](irq);
			}
			rt_spin_lock_irq(&(global.data_lock)); // ?????????
		} else {
			rt_spin_lock(&(global.data_lock));
			rt_pend_linux_irq(irq);
		}
	        if (hprocessor[cpuid].intr_flag && lflags) {
                	HARD_STI();
			rt_spin_unlock_irq(&(global.data_lock));
			linux_sti();
			return;
		} else {
			rt_spin_unlock(&(global.data_lock));
        		*lflagp = lflags;
			return;
		}
	} else {
		printk(KERN_ERR "RTAI-IRQ: spurious interrupt 0x%02x\n", irq);
        	*lflagp = lflags;
		rt_spin_unlock(&global.ic_lock);
		return;

	}
}

#define MIN_IDT_VEC 0xF0
#define MAX_IDT_VEC 0xFF

static unsigned long long (*idt_table[MAX_IDT_VEC - MIN_IDT_VEC + 1])(rtai_irq_t srq, unsigned long name);

asmlinkage long long dispatch_srq(rtai_irq_t srq, unsigned long whatever)
{
	unsigned long vec;
	long long retval = -1;

       	if (!(vec = srq >> 24)) {
       		if (srq > 1 && srq < NR_SYSRQS && sysrq[srq].user_handler) {
       			retval = sysrq[srq].user_handler(whatever);
       		} else {
       			for (srq = 2; srq < NR_SYSRQS; srq++) {
       				if (sysrq[srq].label == whatever) {
       					retval = srq;
       				}
       			}
       		}
       	} else {
       		if ((vec >= MIN_IDT_VEC) && (vec <= MAX_IDT_VEC) && (idt_table[vec - MIN_IDT_VEC])) {
	       		retval = idt_table[vec - MIN_IDT_VEC](srq & 0xFFFFFF, whatever);
       		} else {
       			printk("RTAI SRQ DISPATCHER: bad srq (0x%0x)\n", (int) vec);
	       	}
       	}
       	return retval;
}

struct desc_struct rt_set_full_intr_vect(unsigned int vector, int type, int dpl, void *handler)
{
	struct desc_struct fun = { 0 };
	if (vector >= MIN_IDT_VEC && vector <= MAX_IDT_VEC) {
		fun.fun = idt_table[vector - MIN_IDT_VEC];
		idt_table[vector - MIN_IDT_VEC] = handler;
		if (!rthal.do_SRQ) {
			rthal.do_SRQ = dispatch_srq;
		}
	}
	return fun;
}

void rt_reset_full_intr_vect(unsigned int vector, struct desc_struct idt_element)
{
	if (vector >= MIN_IDT_VEC && vector <= MAX_IDT_VEC) {
		idt_table[vector - MIN_IDT_VEC] = idt_element.fun;
	}
}

/*
*	Dispatch Traps like Illegal instruction, ....
*	Keep call compatible to x386 
*/
asmlinkage int dispatch_trap(int vector, struct pt_regs *regs)
{
	if ( (vector < NR_TRAPS) && (rtai_trap_handler[vector]) ) 
		return rtai_trap_handler[vector](vector, vector, regs, NULL);

	return 1;	/* Let Linux do the job */
}

RT_TRAP_HANDLER rt_set_rtai_trap_handler(int trap, RT_TRAP_HANDLER handler)
{
        RT_TRAP_HANDLER old_handler = NULL;

	if (trap < NR_TRAPS) {
		old_handler = rtai_trap_handler[trap];
                rtai_trap_handler[trap] = handler;
        }
        return old_handler;
}

void rt_free_rtai_trap_handler(int trap)
{
	rtai_trap_handler[trap] = NULL;
}

/* Here are the trapped pic functions for Linux interrupt handlers. */

static void trpd_mask_ack_irq(rtai_irq_t irq)
{
        rt_spin_lock_irq(&global.ic_lock);
	if(linux_irq_desc_handler[irq].pic.mask_ack)
		linux_irq_desc_handler[irq].pic.mask_ack(irq);
	rt_spin_unlock_irq(&global.ic_lock);
}

static void trpd_mask_irq(rtai_irq_t irq)
{
        rt_spin_lock_irq(&global.ic_lock);
	if(linux_irq_desc_handler[irq].pic.mask)
		linux_irq_desc_handler[irq].pic.mask(irq);
	rt_spin_unlock_irq(&global.ic_lock);
}

static void trpd_unmask_irq(rtai_irq_t irq)
{
        rt_spin_lock_irq(&global.ic_lock);
	if(linux_irq_desc_handler[irq].pic.unmask)
		linux_irq_desc_handler[irq].pic.unmask(irq);
	rt_spin_unlock_irq(&global.ic_lock);
}

static struct ic_ops trapped_linux_irq_type = {
	do_nothing_picfun, // Only ack once - in RTAI irq_dispatch function
//	trpd_mask_ack_irq,
	trpd_mask_irq,
	trpd_unmask_irq,
};

static struct ic_ops real_time_irq_type = {
	do_nothing_picfun,
	do_nothing_picfun,
	do_nothing_picfun,
};

/* Request and free interrupts, system requests and interprocessors messages */
/* Request for regular Linux irqs also included. They are nicely chained to  */
/* Linux, forcing sharing with any already installed handler, so that we can */
/* have an echo from Linux for global handlers. We found that usefull during */
/* debug, but can be nice for a lot of other things, e.g. see the jiffies    */
/* recovery in rtai_sched.c, and the global broadcast to local apic timers.  */

static unsigned long irq_action_flags[NR_IRQS];
static int chained_to_linux[NR_IRQS];

int rt_request_global_irq(rtai_irq_t irq, void (*handler)(void))
{
	unsigned long flags;

	if (irq >= NR_IRQS || !handler) {
		return -EINVAL;
	}
	if (global_irq[irq].handler) {
		return -EBUSY;
	}

	flags = hard_lock_all();

	IRQ_DESC[irq].pic = real_time_irq_type;
	
	global_irq[irq].dest_status = 0;
	global_irq[irq].handler = handler;
	linux_unmask_irq[irq] = do_nothing_picfun;
	hard_unlock_all(flags);
	return 0;
}

int rt_free_global_irq(rtai_irq_t irq)
{
	unsigned long flags;

	if (irq >= NR_IRQS || !global_irq[irq].handler) {
		return -EINVAL;
	}

	flags = hard_lock_all();
	IRQ_DESC[irq].pic = trapped_linux_irq_type;
	global_irq[irq].dest_status = 0;
	global_irq[irq].handler = 0;
	linux_unmask_irq[irq] = ic_unmask_irq[irq];
	hard_unlock_all(flags);

	return 0;
}

int rt_request_linux_irq(rtai_irq_t irq,
	void (*linux_handler)(int irq, void *dev_id, struct pt_regs *regs),
	char *linux_handler_id, void *dev_id)
{
	unsigned long flags;

//##	
//	if (irq == TIMER_8254_IRQ) {
//		processor[0].trailing_irq_handler = linux_handler;
//		return 0;
//	}

	if (irq >= NR_IRQS || !linux_handler) {
		return -EINVAL;
	}

	save_flags_cli(flags);
	if (!chained_to_linux[irq]++) {
		if (IRQ_DESC[irq].action) {
			irq_action_flags[irq] = IRQ_DESC[irq].action->flags;
			IRQ_DESC[irq].action->flags |= SA_SHIRQ;
		}
	}
	restore_flags(flags);
	request_irq(irq, linux_handler, SA_SHIRQ, linux_handler_id, dev_id);

	return 0;
}

int rt_free_linux_irq(rtai_irq_t irq, void *dev_id)
{
	unsigned long flags;

//##
//	if (irq == TIMER_8254_IRQ) {
//		processor[0].trailing_irq_handler = 0;
//		return 0;
//	}

	if (irq >= NR_IRQS || !chained_to_linux[irq]) {
		return -EINVAL;
	}

	free_irq(irq, dev_id);
	save_flags_cli(flags);
	if (!(--chained_to_linux[irq]) && IRQ_DESC[irq].action) {
		IRQ_DESC[irq].action->flags = irq_action_flags[irq];
	}
	restore_flags(flags);

	return 0;
}

int rt_request_srq(unsigned int label, void (*rtai_handler)(void), long long (*user_handler)(unsigned int whatever))
{
	unsigned long flags;
	rtai_irq_t srq;

	flags = rt_spin_lock_irqsave(&global.data_lock);
	if (!rtai_handler) {
		rt_spin_unlock_irqrestore(flags, &global.data_lock);
		return -EINVAL;
	}
	for (srq = 2; srq < NR_SYSRQS; srq++) {
		if (!(sysrq[srq].rtai_handler)) {
			sysrq[srq].rtai_handler = rtai_handler;
			sysrq[srq].label = label;
			if (user_handler) {
				sysrq[srq].user_handler = user_handler;
				if (!rthal.do_SRQ) {
					rthal.do_SRQ = dispatch_srq;
				}
			}
			rt_spin_unlock_irqrestore(flags, &global.data_lock);
			return srq;
		}
	}
	rt_spin_unlock_irqrestore(flags, &global.data_lock);

	return -EBUSY;
}

int rt_free_srq(rtai_irq_t srq)
{
	unsigned long flags;

	flags = rt_spin_lock_irqsave(&global.data_lock);
	if (srq < 2 || srq >= NR_SYSRQS || !sysrq[srq].rtai_handler) {
		rt_spin_unlock_irqrestore(flags, &global.data_lock);
		return -EINVAL;
	}
	sysrq[srq].rtai_handler = 0;
	sysrq[srq].user_handler = 0;
	sysrq[srq].label = 0;
	for (srq = 2; srq < NR_SYSRQS; srq++) {
		if (sysrq[srq].user_handler) {
			rt_spin_unlock_irqrestore(flags, &global.data_lock);
			return 0;
		}
	}
	if (rthal.do_SRQ) {
		rthal.do_SRQ = 0;
	}
	rt_spin_unlock_irqrestore(flags, &global.data_lock);

	return 0;
}

int rt_is_linux(void)
{
        return test_bit(hard_cpu_id(), &global.used_by_linux);
}

void rt_switch_to_linux(int cpuid)
{
	set_bit(cpuid, &global.used_by_linux);
	processor[cpuid].intr_flag = processor[cpuid].linux_intr_flag;
}

void rt_switch_to_real_time(int cpuid)
{
	if ( global.used_by_linux & (1<<cpuid) )
		processor[cpuid].linux_intr_flag = processor[cpuid].intr_flag;
	processor[cpuid].intr_flag = 0;
	clear_bit(cpuid, &global.used_by_linux);
}

/* RTAI mount-unmount functions to be called from the application to       */
/* initialise the real time application interface, i.e. this module, only  */
/* when it is required; so that it can stay asleep when it is not needed   */

#ifdef CONFIG_RTAI_MOUNT_ON_LOAD
#define rtai_mounted 1
#else
static int rtai_mounted;
#endif

// Trivial, but we do things carefully, the blocking part is relatively short,
// should cause no troubles in the transition phase.
// All the zeroings are strictly not required as mostly related to static data.
// Done esplicitly for emphasis. Simple, just lock and grab everything from
// Linux.

void __rt_mount_rtai(void)
{
     	unsigned long flags, i;

	flags = hard_lock_all();

	rthal.do_IRQ           	= dispatch_irq;
	rthal.do_SRQ           	= dispatch_srq;
	rthal.do_TRAP		= (void *)dispatch_trap;
	rthal.disint          	= linux_cli;
	rthal.enint            	= linux_sti;
	rthal.getflags         	= linux_save_flags;
	rthal.setflags         	= linux_restore_flags;
	rthal.getflags_and_cli 	= linux_save_flags_and_cli;
	rthal.fdisint          	= linux_cli;
	rthal.fenint           	= linux_sti;

	for (i = 0; i < NR_IRQS; i++) {
		if ((IRQ_DESC[i].pic.mask_ack || IRQ_DESC[i].pic.mask || IRQ_DESC[i].pic.unmask) ) {
			IRQ_DESC[i].pic = trapped_linux_irq_type;
		}
	}

	arch_mount_rtai();

	saved_timer_action_handler = IRQ_DESC[TIMER_8254_IRQ].action->handler;
	IRQ_DESC[TIMER_8254_IRQ].action->handler = linux_timer_interrupt;

	hard_unlock_all(flags);
	printk("\n***** RTAI NEWLY MOUNTED (MOUNT COUNT %d) ******\n\n", rtai_mounted);
}

// Simple, now we can simply block other processors and copy original data back
// to Linux.

void __rt_umount_rtai(void)
{
	int i;
	unsigned long flags;

	flags = hard_lock_all();

#ifdef CONFIG_ARCH_SA1100
	rt_free_global_irq( IRQ_GPIO11_27 );
//	request_irq( IRQ_GPIO11_27, sa1100_GPIO11_27_demux, SA_INTERRUPT, "GPIO 11-27", NULL );
#endif

        rthal = linux_rthal;

	for (i = 0; i < NR_IRQS; i++) {
		IRQ_DESC[i] = linux_irq_desc_handler[i];
	}

	IRQ_DESC[TIMER_8254_IRQ].action->handler = saved_timer_action_handler;

	hard_unlock_all(flags);
	printk("\n***** RTAI UNMOUNTED (MOUNT COUNT %d) ******\n\n", rtai_mounted);
}

#ifdef CONFIG_RTAI_MOUNT_ON_LOAD
void rt_mount_rtai(void) { }
void rt_umount_rtai(void) { }
#else
void rt_mount_rtai(void)
{
	rt_spin_lock(&rtai_mount_lock);
	MOD_INC_USE_COUNT;
	if(++rtai_mounted==1)
		__rt_mount_rtai();
	rt_spin_unlock(&rtai_mount_lock);
}

void rt_umount_rtai(void)
{
	rt_spin_lock(&rtai_mount_lock);
	MOD_DEC_USE_COUNT;
	if(!--rtai_mounted)
		__rt_umount_rtai();
	rt_spin_unlock(&rtai_mount_lock);

}
#endif

/* module init-cleanup */

extern void rt_printk_sysreq_handler(void);

// Let's prepare our side without any problem, so that there remain just a few
// things to be done when mounting RTAI. All the zeroings are strictly not
// required as mostly related to static data. Done esplicitly for emphasis.
static __init int init_rtai_arm(void)
{
     	unsigned int i;

	tuned.cpu_freq = FREQ_SYS_CLK;

	linux_rthal = rthal;

	init_pending_irqs();
	global.active_irqs     = 0;
	init_pending_srqs();
	global.active_srqs     = 0;
	global.cpu_in_sti      = 0;
	global.used_by_linux   = ~(0xFFFFFFFF << smp_num_cpus);
	global.locked_cpus     = 0;
	global.hard_nesting    = 0;
	spin_lock_init(&(global.data_lock));
	spin_lock_init(&(global.hard_lock));
	spin_lock_init(&(global.ic_lock));

	for (i = 0; i < NR_RT_CPUS; i++) {
		processor[i].intr_flag         = IBIT | (1 << i);
		processor[i].linux_intr_flag   = IBIT | (1 << i);
		processor[i].pending_irqs      = 0;
		processor[i].active_irqs       = 0;
		hprocessor[i].intr_flag       =
		hprocessor[i].linux_intr_flag = INTR_ENABLED;
	}
        for (i = 0; i < NR_SYSRQS; i++) {
		sysrq[i].rtai_handler = 0;
		sysrq[i].user_handler = 0;
		sysrq[i].label        = 0;
        }
	for (i = 0; i < NR_IRQS; i++) {
		global_irq[i].dest_status = 0;
		global_irq[i].handler = 0;

		linux_irq_desc_handler[i] = IRQ_DESC[i];
		ic_mask_and_ack_irq[i] = IRQ_DESC[i].pic.mask_ack;
		ic_mask_irq[i] = IRQ_DESC[i].pic.mask;
		ic_unmask_irq[i] = linux_unmask_irq[i] = IRQ_DESC[i].pic.unmask;
	}

	sysrq[rt_printk_srq].rtai_handler = rt_printk_sysreq_handler;
	sysrq[rt_printk_srq].label = 0x1F0000F1;

#ifdef CONFIG_RTAI_MOUNT_ON_LOAD
	__rt_mount_rtai();
#endif

#ifdef CONFIG_PROC_FS
	rtai_proc_register();
#endif

	return 0;
}

static __exit void cleanup_rtai_arm(void)
{

#ifdef CONFIG_RTAI_MOUNT_ON_LOAD
	__rt_umount_rtai();
#endif

#ifdef CONFIG_PROC_FS
	rtai_proc_unregister();
#endif

	return;
}

module_init(init_rtai_arm);
module_exit(cleanup_rtai_arm);


/* ----------------------< proc filesystem section >----------------------*/
#ifdef CONFIG_PROC_FS

struct proc_dir_entry *rtai_proc_root = NULL;

static int rtai_read_rtai(char *page, char **start, off_t off, int count,
                          int *eof, void *data)
{
        PROC_PRINT_VARS;
        int i;

        PROC_PRINT("\nRTAI Real Time Kernel, Version: %s\n\n", RTAI_VERSION);
        PROC_PRINT("    RTAI mount count: %d\n", rtai_mounted);
        PROC_PRINT("    APIC Frequency: %d\n", FREQ_SYS_CLK);
        PROC_PRINT("    APIC Latency: %d ns\n", LATENCY_MATCH_REG);
        PROC_PRINT("    APIC Setup: %d ns\n", SETUP_TIME_MATCH_REG);
        PROC_PRINT("\nGlobal irqs used by RTAI: \n");
        for (i = 0; i <= LAST_GLOBAL_RTAI_IRQ; i++) {
          if (global_irq[i].handler) {
            PROC_PRINT("%d ", i);
          }
        }
        PROC_PRINT("\nCpu_Own irqs used by RTAI: \n");
        for (i = LAST_GLOBAL_RTAI_IRQ + 1; i < NR_RTAI_IRQS; i++) {
          if (global_irq[i].handler) {
            PROC_PRINT("%d ", i);
          }
        }
        PROC_PRINT("\nRTAI sysreqs in use: \n");
        for (i = 0; i < NR_SYSRQS; i++) {
          if (sysrq[i].rtai_handler || sysrq[i].user_handler) {
            PROC_PRINT("%d ", i);
          }
        }
	if ( global.lost_irqs )
		PROC_PRINT( "### Lost IRQs: %d ###\n", global.lost_irqs );
	PROC_PRINT("\n\n");

	PROC_PRINT_DONE;
}       /* End function - rtai_read_rtai */

static int rtai_proc_register(void)
{

	struct proc_dir_entry *ent;

        rtai_proc_root = create_proc_entry("rtai", S_IFDIR, 0);
        if (!rtai_proc_root) {
		printk("Unable to initialize /proc/rtai\n");
                return(-1);
        }
	rtai_proc_root->owner = THIS_MODULE;
        ent = create_proc_entry("rtai", S_IFREG|S_IRUGO|S_IWUSR, rtai_proc_root);
        if (!ent) {
		printk("Unable to initialize /proc/rtai/rtai\n");
                return(-1);
        }
	ent->read_proc = rtai_read_rtai;
        return(0);
}       /* End function - rtai_proc_register */


static void rtai_proc_unregister(void)
{
        remove_proc_entry("rtai", rtai_proc_root);
        remove_proc_entry("rtai", 0);
}       /* End function - rtai_proc_unregister */

#endif /* CONFIG_PROC_FS */
/* ------------------< end of proc filesystem section >------------------*/


/********** SOME TIMER FUNCTIONS TO BE LIKELY NEVER PUT ELSWHERE *************/

/* Real time timers. No oneshot, and related timer programming, calibration. */
/* Use the utility module. It is also to be decided if this stuff has to     */
/* stay here.                                                                */

struct calibration_data tuned;
struct rt_times rt_times;
struct rt_times rt_smp_times[NR_RT_CPUS];

/******** END OF SOME TIMER FUNCTIONS TO BE LIKELY NEVER PUT ELSWHERE *********/

// Our printk function, its use should be safe everywhere.
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

EXPORT_SYMBOL(rt_mask_ack_irq);
EXPORT_SYMBOL(rt_mask_irq);
EXPORT_SYMBOL(rt_disable_irq);
EXPORT_SYMBOL(rt_enable_irq);
EXPORT_SYMBOL(rt_free_global_irq);
EXPORT_SYMBOL(rt_free_linux_irq);
EXPORT_SYMBOL(rt_free_srq);
EXPORT_SYMBOL(rt_free_timer);
EXPORT_SYMBOL(rt_mount_rtai);
EXPORT_SYMBOL(rt_pend_linux_irq);
EXPORT_SYMBOL(rt_pend_linux_srq);
EXPORT_SYMBOL(rt_printk);
EXPORT_SYMBOL(rt_request_global_irq);
EXPORT_SYMBOL(rt_request_linux_irq);
EXPORT_SYMBOL(rt_request_srq);
EXPORT_SYMBOL(rt_request_timer);
EXPORT_SYMBOL(rt_reset_full_intr_vect);
EXPORT_SYMBOL(rt_set_full_intr_vect);
EXPORT_SYMBOL(rt_shutdown_irq);
EXPORT_SYMBOL(rt_startup_irq);
EXPORT_SYMBOL(rt_switch_to_linux);
EXPORT_SYMBOL(rt_switch_to_real_time);
EXPORT_SYMBOL(rt_umount_rtai);
EXPORT_SYMBOL(rt_unmask_irq);
EXPORT_SYMBOL(rtai_proc_root);
EXPORT_SYMBOL(rt_smp_times);
EXPORT_SYMBOL(rt_times);
EXPORT_SYMBOL(tuned);
EXPORT_SYMBOL(locked_cpus);
EXPORT_SYMBOL(linux_save_flags_and_cli);
EXPORT_SYMBOL(linux_save_flags_and_cli_cpuid);
EXPORT_SYMBOL(rtai_just_copy_back);
EXPORT_SYMBOL(global);
EXPORT_SYMBOL(rtai_print_to_screen);
EXPORT_SYMBOL(rtai_tsc);
EXPORT_SYMBOL(rt_set_rtai_trap_handler);
EXPORT_SYMBOL(rt_free_rtai_trap_handler);
EXPORT_SYMBOL(rt_is_linux);

#include <asm/arch/rtai_exports.h>
