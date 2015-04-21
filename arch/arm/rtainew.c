/* 
 * rtai/arch/arm/rtai.c
 *
 * COPYRIGHT (C) 2003 Thomas Gleixner (tglx@linutronix.de)
 * COPYRIGHT (C) 2002 Guennadi Liakhovetski, DSA GmbH (gl@dsa-ac.de)
 * COPYRIGHT (C) 2002 Wolfgang Müller (wolfgang.mueller@dsa-ac.de)
 * Copyright (c) 2001 Alex Züpke, SYSGO RTS GmbH (azu@sysgo.de)
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: rtainew.c,v 1.1.1.1 2004/06/06 14:01:35 rpm Exp $
 *
*/

/*
 * Acknowledgements
 * - Paolo Mantegazza	(mantegazza@aero.polimi.it)	creator of RTAI 
*/

#define ARM_VERSION "2.7"
/* 
 * Changelog
 * 03-10-2002	TG	added support for trap handling
 *			added function rt_is_linux
 *			
 * 07-07-2003	TG	complete rework of locking and linux interrupt handling.
 *			Use of linked lists, as it makes serializing easier and we
 *			can support machines, where it is conveniant to demultiplex
 *			the multiplexed interrupt sources in fixup irq. This gives us
 *			up to 140 interrupt sources in case of the hynix 7202. This
 *			can not be handled with a 32 bit field. The previous version
 *			with a fifo buffer is not as conveniant as a linked list and
 *			has been replaced.
 *
 *			Fixed two dangerous places, where the old code went back to rmk's 
 *			entry stuff with int's enabled. This breaks not often, but it breaks
 *			especially if you put high load on it. This could be applied to the
 * 			old code too. It's at the end of dispatch_irq and dispatch_srq, as
 *			linux exspects that interrupts ate disabled when we go back to the
 *			assembler entry.
 *
 *			Port to linux-2.4.19-rmk7. It carefully tries to handle rmk's new 
 *			interrupt semantics. It works on 2.4.17 too.
 *			Maybe some points look like overkill in there, but at least the
 *			overall performance and stability of linux is much better than before.
 *			Maybe some hard locks could be removed, but at this point 10 assembler
 *			instructions overhead are better than an unstable system. They don't
 *			affect the realtime performance. The hard locking in linux_save_flags_cli
 *			is debug related and can be removed, if we have really fixed all places
 *			where linux comes up with a wrong interrupt state. 
 *
 *			In linux_cli, linux_save_flags, linux_save_flags_cli and linux_restore 
 *			are BUG()'s, which show places where linux comes up with a wrong state
 *			of interrupt flag. The stacktrace should show the origin of this
 *			violations. Those are active in DEBUG_LEVEL 2. We cannot have them active
 *			all the time, as realtime functions can call kernel functions
 *			e.g. kmalloc with hard irqs disabled, as you can see in rtai_fifos.
 *			We enable them with rtai.o loaded and then try to start all possible
 *			linux functions to find a new place, where linux comes up with ints
 *			hard disabled. 
 *
 *			The debug stuff, which can be enabled by DEBUG_LEVEL 1 is usefull
 *			to analyse system performance. 
 *			
 *			This rework was partially sponsored by Kurz Industrieelektronik GmbH
 *			(www.kurz-elektronik.de) and SYSGO realtime solutions AG (www.sysgo.de). 
 *			Thanks.
 *
 * 07-09-2003	TG	Fix the deferring of demultiplexed GPIO irq's for StrongARM
 * 07-12-2003   TG	Fix the hopefully last race
 * 07-24-2003   TG	Fix the typo in rt_pend_linux_srq. Replaced the missleading sysr by curr.
 * 07-26-2003   TG	Fix the calls to linux_cli/sti functions from realtime context
 * 07-26-2003   TG	Replace the ibit stuff by readable macros use I_BIT from kernel
*/

#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/list.h>

#include <asm/system.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <asm/arch/irq.h>
#include <linux/bitops.h>
#include <asm/atomic.h>
#include <linux/kernel_stat.h>

#include <asm/rtai.h>
#include <asm/rtai_srq.h>
#include <asm/mach/irq.h>

#include <rtai/version.h>

/* proc filesystem */
#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
static int rtai_proc_register(void);
static void rtai_proc_unregister(void);
#endif

/* IRQ defines */
#define NR_RTAI_IRQS  	 	NR_IRQS
#define NR_SYSRQS  	 	32
#define LAST_GLOBAL_RTAI_IRQ 	NR_RTAI_IRQS - 1
#define NR_TRAPS		32

/*
* Convenience macros to modify / read the intr_flag.
* Please use the macros, instead of modifying rthal.intr_flag
* directly.
*/
#define modify_linux_ints(flags) rthal.intr_flag = flags
#define get_linux_ints() rthal.intr_flag
#define disable_linux_ints() rthal.intr_flag = I_BIT
#define enable_linux_ints() rthal.intr_flag = 0
#define ints_disabled(flags) (flags & I_BIT)
#define linux_ints_disabled() ints_disabled(rthal.intr_flag)
#define strip_flags(flags) (flags & ~I_BIT)

/* prototypes for timer-handling */
void linux_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs);
void soft_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs);
void rt_request_timer(void (*handler)(void), unsigned int tick, int unused);
void rt_free_timer(void);

/* Realtime system status information */
static struct global_rt_status {
	volatile unsigned int cpu_in_sti;
	volatile unsigned int used_by_linux;
	volatile int lost_irqs;
	volatile rtai_irq_mask_t pending_srqs;
	volatile rtai_irq_mask_t active_srqs;
	volatile unsigned int locked_cpus;
	spinlock_t data_lock;
	spinlock_t ic_lock;
} global;

volatile unsigned int *locked_cpus = &global.locked_cpus;

/* Linux interrupt descriptions */
extern struct irqdesc irq_desc[];

/* Global irq handlers for realtime processing */
static struct irq_handling {
	volatile unsigned long dest_status;
	void (*handler)(int irq, void *dev_id, struct pt_regs *regs);
	void *dev_id;
} global_irq[NR_IRQS];

/* Storage for linux stuff
 * rthal stores the original linux rthal contents
 * saved_timer_action_handler stores the original linux timer handler
 */
static struct rt_hal linux_rthal;
static void *saved_timer_action_handler;

/* Storage for original linux interrupt handlers */
static struct irqdesc linux_irq_desc_handler[NR_IRQS];

/* Here we hold the mask/ack function */
static void (*ic_mask_and_ack_irq[NR_IRQS]) (unsigned int irq);
static void (*ic_mask_irq[NR_IRQS]) (unsigned int irq);
static void (*ic_unmask_irq[NR_IRQS]) (unsigned int irq);

/* Linux Interrupt handling */
struct linux_irq {
	struct list_head list;
	int irq;
	int masked;
	int pending;
#if DEBUG_LEVEL > 0
	/* Statistics */
	int lost;
	int pended;
	int pendedlinux;
	int processed;
	int triggered;
	int bogus;
#endif
};
static struct list_head irqlist;
static struct linux_irq linux_irqs[NR_IRQS];

/* flags for global int's */
static unsigned long irq_action_flags[NR_IRQS];
static int chained_to_linux[NR_IRQS];

/* sysreqest handling */
static struct list_head srqlist;
static struct sysrq_t {
	struct list_head list;
	int active;
	int pending;
	unsigned int label;
	void (*rtai_handler) (void);
	long long (*user_handler) (unsigned int whatever);
} sysrq[NR_SYSRQS];

/* Trap handler to capture linux traps like invalid opcode ... */
static RT_TRAP_HANDLER rtai_trap_handler[NR_TRAPS];

/* This will be just one, but who knows, if somebody is crazy enough to do
 * it on a MP machine. The code is not MP safe at least. 
*/
static struct cpu_own_status {
	volatile unsigned int linux_intr_flag;
} processor[NR_RT_CPUS];

/* The main time keeper and some timer stuff for other rtai modules*/
union rtai_tsc rtai_tsc;
struct calibration_data tuned;
struct rt_times rt_times;
struct rt_times rt_smp_times[NR_RT_CPUS];

/*
 * Just debug stuff, please let it there, it's helpful for performance 
 * analysis and bug tracking.
 */
#if DEBUG_LEVEL > 0
unsigned long long ininttime, linuxtime, realtime, lastswitchtime;
int lsti, lstireent, lstitot, lstiprocessed, srqsprocessed;
int totalints, clicnt, sticnt;
#endif

/* Some forward declarations */
static void linux_sti(void);
static void linux_cli(void);

/* 
* We do the mask_ack on entry to dispatch_irq. The mask bit is set, 
* when we call the linux handler. It just keeps us informed, that
* linux has really masked this int.
* This is very useful for preventing reentrant entries for global
* used interrupts.
*/
void linux_irq_mask_ack(unsigned int irq)
{
	linux_irqs[irq].masked = 2;
}

/*
* Let linux switch off an handler, but not if it is
* a global handler. Set the mask bit. That's cool, as when a 
* module is removed and the interrupt freed or disabled outside
* of interrupt context we know, that it's blocked. So a int, which
* is eventually on the list can be thrown away.
*/
void linux_irq_mask(unsigned int irq)
{
	linux_irqs[irq].masked = 2;
	/* Do not mask, if it is owned by a global handler */
	if (ic_mask_irq[irq] && !global_irq[irq].handler)
		ic_mask_irq[irq] (irq);
}

/* remove the masked bit for an linux irq and unmask it 
*  Check if this int is pending and reschedule it. If 
*  it is marked as pending in linux irq handler, then 
*  throw it away, as it will come back here
*/
void linux_irq_unmask(unsigned int irq)
{
	unsigned long flags;
	struct linux_irq *lirq = &linux_irqs[irq];

	hard_save_flags_cli(flags);
	/*Is it pending or already on the list ? */
	if (lirq->pending || lirq->list.next) {
		/* Set it masked again */
		lirq->masked = 1;
		/* If its not on the list, set it there, but only if none
		 * is pending in linux, as this will call the handler again,
		 * so we will end up here again */
		if (!lirq->list.next) {
			lirq->pending--;
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,18)
			if (!irq_desc[irq].pending) {
				list_add(&lirq->list, &irqlist);
				D1(lirq->pendedlinux++);
			} else
				D1(lirq->bogus++);
#else
			list_add(&lirq->list, &irqlist);
			D1(lirq->pendedlinux++);
#endif
		}
	} else {
		lirq->masked = 0;
		/* Do not unmask, if it is owned by a global handler */
		if (ic_unmask_irq[irq] && !global_irq[irq].handler)
			ic_unmask_irq[irq] (irq);
	}
	hard_restore_flags(flags);
}

/* Set this irq pending for linux. 
 */
void rt_pend_linux_irq(unsigned int irq)
{
	unsigned long flags;
	struct linux_irq *lirq = &linux_irqs[irq];

	hard_save_flags_cli(flags);

	/* we have masked it already, but it's here again. Should only happen 
	 * for global int's , which are delivered to linux (e.g. timer). 
	 * Else it's bogus cause we have usually no devices in linux, which 
	 * have int's enabled permanent. If you have one, make a decision
	 * especially for that device, instead of removing the code.
	 * The timer code is very sensitive, if it called too often,
	 * as it blocks a lot of stuff due to scheduling and triggering
	 * kernel daemons and it is per definition not reentrant.
	 */
	if (lirq->masked) {
		if (global_irq[irq].handler)
			lirq->pending++;
		else
			D1(lirq->bogus++);
		goto out;	
	}
	/* Mask it again */
	lirq->masked = 1;

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,18)
	/* If there is a pending one, throw it away and count it as bogus. 
	 * That happens especially on clps71xx uarts. Maybe I have a look into the
	 * driver code sometimes. But this hardware is really weird! 
	 */
	if (irq_desc[irq].pending) {
		D1(lirq->bogus++);
		goto out;	
	}
#endif

	/* If it's already on the list increment pending count for global used int's 
	 * like timer and set it pending for others.
	 */
	if (lirq->list.next) {
		if (global_irq[irq].handler)
			lirq->pending++;
		else
			lirq->pending = 1;
		goto out;	
	}

	/* Put it on the list */
	D1(lirq->pended++);
	list_add(&lirq->list, &irqlist);

out:	
	hard_restore_flags(flags);
}

/*
* Pend a sysrequest
*/
void rt_pend_linux_srq(unsigned int srq)
{
	unsigned long flags;
	struct sysrq_t *curr;

	if (srq < 0 || srq >= NR_SYSRQS) {
		printk("Invalid SRQ! (pend:) %d\n", srq);
		return;
	}
	hard_save_flags_cli(flags);
	curr = &sysrq[srq];
	if (curr->active || curr->list.next)
		curr->pending++;
	else
		list_add_tail(&curr->list, &srqlist);
	hard_restore_flags(flags);
}

/* Deliver pending linux int's
 * We come here either from linux_sti or from the interrupt entry. This function 
 * is reentrant, as it tries to behave similar to the original arm-linux irq 
 * dispatcher. I think for other arch's this would be correct too. But you 
 * have to maintain the masked bit stuff correct. IMHO it increases linux
 * performance especially, if the realtime system uses a lot of system time as 
 * it is much closer to the linux behaviour, than processing the list by one
 * entry. That's what the STI instruction is for. At least it is neccecary
 * on small systems to handle even extreme situations correct.
 * We deliver real regs, as rmk's irq lock checker can hit us under heavy load
 * if we give him the dummy regs always.
*/
static void rtai_linux_sti(struct pt_regs *regs)
{

	/* Statistics for debugging only */
	D1(lstitot++);

	/* Grab the lock, as we are playing with a list */
	rt_spin_lock_irq(&(global.data_lock));
	global.cpu_in_sti++;

	/* loop here until all int's are served */
	while (!list_empty(&irqlist)) {

		struct linux_irq *curr = (struct linux_irq *) irqlist.next;

		D1(lstiprocessed++);

		/* Remove it from the list */
		list_del(&curr->list);
		curr->list.next = NULL;

		/* was it masked in meantime ? */
		if (curr->masked == 2) {
			curr->pending++;
			continue;
		}
		/* Disable linux int's */
		disable_linux_ints();
		/* call old Linux do_IRQ() to handle IRQs, which in fact calls sti/cli again 
		 * so we will be reentrant at that point
		 * Unlock so other ints can be handled by dispatch_irq
		 * Unmasking is done in do_IRQ, so we don't need to handle this here
		 */
		rt_spin_unlock_irq(&(global.data_lock));
		((void (*)(int, struct pt_regs *)) linux_rthal.do_IRQ) (curr->irq, regs);

		/* Grab the lock again */
		rt_spin_lock_irq(&(global.data_lock));
		D1(curr->processed++);
		/* Enable linux int's again like we do in the real linux handler */
		enable_linux_ints();
	}

	while (!list_empty(&srqlist)) {

		struct sysrq_t *curr = (struct sysrq_t *) srqlist.next;
		
		D1(srqsprocessed++);
		/* Remove it from the list */
		list_del(&curr->list);
		curr->list.next = NULL;
		curr->active = 1;
		/* Go there with int's enabled */
		rt_spin_unlock_irq(&(global.data_lock));
		if (curr->rtai_handler)
			curr->rtai_handler();
		/* Grab the lock again */
		rt_spin_lock_irq(&(global.data_lock));
		curr->active = 0;
		if (curr->pending) {
			curr->pending--;
			list_add_tail(&curr->list, &srqlist);
		}
	}

	/* reenable in any case */
	enable_linux_ints();
	global.cpu_in_sti--;
	/* release the lock, enable int's and get out of here */
	rt_spin_unlock_irq(&(global.data_lock));
}

/* Disable linux int's */
static void linux_cli(void)
{
	D1(clicnt++);
	disable_linux_ints();
}

/* 
*  Enable linux int's, check for pending int's to be delivered 
*/
static void linux_sti(void)
{
	unsigned long hflags;
	unsigned int lr;
	struct pt_regs regs;
	
	/* Get the caller and fake the instruction pointer for rmk's irq 
	 * locking check. If we use the faked rtai_regs without modification
	 * we can trigger this on very slow machines under big load
	 * This happens seldom, but it happens. 
	 */
	getlr(lr);
	regs.ARM_pc = lr;

	hard_save_flags(hflags);

	D1(sticnt++);
#if DEBUG_LEVEL > 1	
	if (ints_disabled(hflags)) {
		hard_sti();
		BUG();
	}
#else
	if (ints_disabled(hflags) || !rt_is_linux())
		return;
#endif
	/* Disable linux ints. The reenable happens in rtai_linux_sti */
	disable_linux_ints();
	rtai_linux_sti(&regs);
}

/* we need to return faked, but real flags We adjust the IBIT, 
 * Everything else is returned from the real flags, as we must 
 * preserve mode information et al.
 */
static unsigned int linux_save_flags(void)
{
	unsigned long hflags;

	hard_save_flags(hflags);
#if DEBUG_LEVEL > 1	
	if (ints_disabled(hflags)) {
		hard_sti();
		BUG();
	}
#endif	
	return strip_flags(hflags) | linux_ints_disabled();
}

/* Restore flags 
*/
static void linux_restore_flags(unsigned int flags)
{
	struct pt_regs regs;
	unsigned int lr;
	unsigned long hflags;
	hard_save_flags(hflags);

#if DEBUG_LEVEL > 1
	if (ints_disabled(hflags)) {
		hard_sti();
		BUG();
	}
#else	
	if (ints_disabled(hflags) || !rt_is_linux())
		return;
#endif

	/* check if interrupts are now disabled */
	if (ints_disabled(flags))
		disable_linux_ints();
	else {
		/* Get the caller and fake the instruction pointer for rmk's irq 
		 * locking check. If we use the faked rtai_regs without modification
		 * we can trigger this on very slow machines under big load
		 * This happens seldom, but it happens. 
		 */
		getlr(lr);
		regs.ARM_pc = lr;

		/* Disable linux ints. The reenable happens in rtai_linux_sti */
		disable_linux_ints();
		rtai_linux_sti(&regs);
	}
}

/* Save flags and disable linux int's
 * We return real, but faked flags, see linux_save_flags
*/
unsigned int linux_save_flags_and_cli(void)
{
	unsigned long hflags, retflags;

	hard_save_flags(hflags);
	D1(clicnt++);
#if DEBUG_LEVEL > 1
	if (ints_disabled(hflags)) {
		hard_sti();
		BUG();
	}
#endif	
	retflags = strip_flags(hflags) | linux_ints_disabled();
	disable_linux_ints();
	return retflags;
}

/* This is single cpu, so we use the stuff above */
unsigned long linux_save_flags_and_cli_cpuid(int cpuid)
{
	return linux_save_flags_and_cli();
}

/* Copy back some saved value. This modifies the IBIT only */
void rtai_just_copy_back(unsigned long flags, int cpuid)
{
	modify_linux_ints(ints_disabled(flags));
}

/*
* Startup an irq, set handler
*/
unsigned int rt_startup_irq(unsigned int irq)
{
	unsigned int flags;
	struct irqdesc *irq_descr;

	if ((irq_descr = &linux_irq_desc_handler[irq]) && irq_descr->unmask) {
		flags = rt_spin_lock_irqsave(&global.ic_lock);
		irq_descr->probing = 0;
		irq_descr->triggered = 0;
		irq_descr->unmask(irq);
		rt_spin_unlock_irqrestore(flags, &global.ic_lock);
	}
	return 0;
}

/*
* Shutdown an irq, replace it by a maybe available linux handler
*/
void rt_shutdown_irq(unsigned int irq)
{
	unsigned int flags;
	struct irqdesc *irq_descr;

	if ((irq_descr = &linux_irq_desc_handler[irq]) && irq_descr->mask) {
		flags = rt_spin_lock_irqsave(&global.ic_lock);
		irq_descr->mask(irq);
		rt_spin_unlock_irqrestore(flags, &global.ic_lock);
	}
}

/*
* Enable a realtime irq
*/
void rt_enable_irq(unsigned int irq)
{
	unsigned int flags;
	struct irqdesc *irq_descr;

	if ((irq_descr = &linux_irq_desc_handler[irq])  && irq_descr->unmask) {
		flags = rt_spin_lock_irqsave(&global.ic_lock);
		irq_descr->probing = 0;
		irq_descr->triggered = 0;
		irq_descr->unmask(irq);
		rt_spin_unlock_irqrestore(flags, &global.ic_lock);
	}
}

/*
* Disable a realtime irq
*/
void rt_disable_irq(unsigned int irq)
{
	unsigned int flags;
	struct irqdesc *irq_descr;

	if ((irq_descr = &linux_irq_desc_handler[irq])  && irq_descr->mask) {
		flags = rt_spin_lock_irqsave(&global.ic_lock);
		irq_descr->mask(irq);
		rt_spin_unlock_irqrestore(flags, &global.ic_lock);
	}
}

/*
* Mask and ack a realtime irq
*/
void rt_mask_ack_irq(unsigned int irq)
{
	unsigned int flags;

	flags = rt_spin_lock_irqsave(&global.ic_lock);
	if (ic_mask_and_ack_irq[irq])
		ic_mask_and_ack_irq[irq] (irq);
	rt_spin_unlock_irqrestore(flags, &global.ic_lock);
}

/*
* Mask a realtime irq
*/
void rt_mask_irq(unsigned int irq)
{
	unsigned int flags;

	flags = rt_spin_lock_irqsave(&global.ic_lock);
	if (ic_mask_irq[irq])
		ic_mask_irq[irq] (irq);
	rt_spin_unlock_irqrestore(flags, &global.ic_lock);
}

/*
* unmask a realtime irq
*/
void rt_unmask_irq(unsigned int irq)
{
	unsigned int flags;

	flags = rt_spin_lock_irqsave(&global.ic_lock);
	if (ic_unmask_irq[irq])
		ic_unmask_irq[irq] (irq);
	rt_spin_unlock_irqrestore(flags, &global.ic_lock);
}

/*
 * Dispatch irq's. realtime irq's are served immidiately. Linux irq's are put
 * on the pending list and eventually delivered to linux, if linux irq's are
 * enabled and linux is the active task. Else they will be served after the
 * next switch_to_linux and / or a upcoming linux_sti()
 */
asmlinkage void dispatch_irq(unsigned int irq, struct pt_regs *regs)
{
	int realirq = irq;
#if DEBUG_LEVEL > 0
	unsigned long long now = rdtsc();
	totalints++;
#endif
	/* Call the machine specific fixup function */
	irq = fixup_irq(irq);

	/* Check, if this is a legal int */
	if (irq < NR_IRQS) {
		D1(linux_irqs[irq].triggered++);

		if (ic_mask_and_ack_irq[irq])
			ic_mask_and_ack_irq[irq] (irq);
		/* realtime handler is called immidiately */
		if (global_irq[irq].handler) {
			global_irq[irq].handler(irq, global_irq[irq].dev_id, regs);
			if (ic_unmask_irq[irq])
				ic_unmask_irq[irq] (irq);
		} else {
			/* Set it on the big list */
			rt_pend_linux_irq(irq);
		}
		
		/* Try to deliver linux int's. Don't do that for the demultiplexed I/O's
		 * e.g on StrongARM and PXA. We come back to this point, when we are done
		 * with demultiplexing.
		 */
		hard_cli();
		if (!isdemuxirq(irq) && global.used_by_linux && !linux_ints_disabled()) {
			disable_linux_ints();
			rtai_linux_sti(regs);
		}
	} else {
		if (realirq < NR_IRQS)
			D1(linux_irqs[realirq].triggered++);
		D2(printk (KERN_ERR "RTAI-IRQ: spurious interrupt 0x%02x\n", realirq));
	}
	D1(ininttime += rdtsc() - now);
	/* See arch/arm/kernel/entry-armv.S. */
	hard_cli();
}

#define MIN_IDT_VEC 0xF0
#define MAX_IDT_VEC 0xFF

static unsigned long long (*idt_table[MAX_IDT_VEC - MIN_IDT_VEC + 1])(unsigned int srq, unsigned long name);

asmlinkage long long dispatch_srq(unsigned int srq, unsigned long whatever)
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
       			printk(KERN_ERR "RTAI SRQ DISPATCHER: bad srq (0x%0x)\n", (int) vec);
	       	}
       	}
	/* See arch/arm/kernel/entry-armv.S. */
	hard_cli();
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
* Dispatch Traps like Illegal instruction, ....
* Keep call compatible to x386.	The handler must return 0, if it was able to solve
* the problem and 1 if linux should handle this problem.
*/
asmlinkage int dispatch_traps(int vector, struct pt_regs *regs)
{
	/* Preset to 1, This marks it as has tobe processed by linux */
	int retval = 1;

	if ((vector < NR_TRAPS) && (rtai_trap_handler[vector]))
		retval = rtai_trap_handler[vector] (vector, vector, regs, NULL);
		
	return retval;
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

/* Request and free interrupts, system requests and interprocessors messages 
 * Request for regular Linux irqs also included. They are nicely chained to 
 * Linux, forcing sharing with any already installed handler, so that we can
 * have an echo from Linux for global handlers. We found that usefull during
 * debug, but can be nice for a lot of other things, e.g. see the jiffies   
 * recovery in rtai_sched.c, and the global broadcast to local apic timers. 
*/
int rt_request_global_irq_ext(unsigned int irq, void (*handler)(int, void *, struct pt_regs *), void *dev_id)
{
	unsigned long flags;

	if (irq >= NR_IRQS || !handler) {
		return -EINVAL;
	}
	if (global_irq[irq].handler) {
		return -EBUSY;
	}

	flags = hard_lock_all();
	/* This can be done for non multiplexed irq's only */
	if (irq < 32)
		rthal.rt_ints |= (1 << irq);
	global_irq[irq].dest_status = 0;
	global_irq[irq].handler = handler;
	global_irq[irq].dev_id = dev_id;
	hard_unlock_all(flags);
	return 0;
}

int rt_free_global_irq(unsigned int irq)
{
	unsigned long flags;

	if (irq >= NR_IRQS || !global_irq[irq].handler) {
		return -EINVAL;
	}

	flags = hard_lock_all();
	/* This can be done for non multiplexed irq's only */
	if (irq < 32)
		rthal.rt_ints &= ~(1 << irq);
	global_irq[irq].dest_status = 0;
	global_irq[irq].handler = 0;
	hard_unlock_all(flags);

	return 0;
}

int rt_request_linux_irq(unsigned int irq,
	 void (*linux_handler)(int irq, void *dev_id, struct pt_regs *regs),
	 char *linux_handler_id, void *dev_id)
{
	unsigned long flags;

	if (irq >= NR_IRQS || !linux_handler) {
		return -EINVAL;
	}

	save_flags_cli(flags);
	if (!chained_to_linux[irq]++) {
		if (irq_desc[irq].action) {
			irq_action_flags[irq] =
			    irq_desc[irq].action->flags;
			irq_desc[irq].action->flags |= SA_SHIRQ;
		}
	}
	restore_flags(flags);
	request_irq(irq, linux_handler, SA_SHIRQ, linux_handler_id, dev_id);

	return 0;
}

int rt_free_linux_irq(unsigned int irq, void *dev_id)
{
	unsigned long flags;

	if (irq >= NR_IRQS || !chained_to_linux[irq]) {
		return -EINVAL;
	}

	free_irq(irq, dev_id);
	save_flags_cli(flags);
	if (!(--chained_to_linux[irq]) && irq_desc[irq].action) {
		irq_desc[irq].action->flags = irq_action_flags[irq];
	}
	restore_flags(flags);

	return 0;
}

int rt_request_srq(unsigned int label, void (*rtai_handler)(void), long long (*user_handler)(unsigned int whatever))
{
	unsigned long flags;
	int srq;

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

int rt_free_srq(unsigned int srq)
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

/*
* Check, if we are running in linux context
*/
int rt_is_linux(void)
{
	return test_bit(hard_cpu_id(), (void *) &global.used_by_linux);
}

/*
* Switch from realtime to linux. This is usually called by the scheduler
* Restore the last valid interrupt flag, as the linux int's were disabled
* on the last switch_to_real_time ()
*/
void rt_switch_to_linux(int cpuid)
{
	D1( {
	   unsigned long long act = rdtsc();
	   realtime += act - lastswitchtime; lastswitchtime = act;});
	modify_linux_ints(processor[cpuid].linux_intr_flag);
	set_bit(cpuid, &global.used_by_linux);
}

/*
* Switch from linux to realtime. This is usually called by the scheduler
* Save the current linux interrupt state and disable the dispatching
* of linux interrupts.
*/
void rt_switch_to_real_time(int cpuid)
{
	if (global.used_by_linux & (1 << cpuid)) {
		D1( {
		   unsigned long long act = rdtsc();
		   linuxtime += act - lastswitchtime;
		   lastswitchtime = act;});
		processor[cpuid].linux_intr_flag = get_linux_ints();
		disable_linux_ints();
		clear_bit(cpuid, &global.used_by_linux);
	}
}


/* RTAI mount-unmount functions to be called from the application to       */
/* initialise the real time application interface, i.e. this module, only  */
/* when it is required; so that it can stay asleep when it is not needed   */

#ifdef CONFIG_RTAI_MOUNT_ON_LOAD
#define rtai_mounted 1
#else
static int rtai_mounted;
#endif

/* 
 * Replace the linux functions and (re)initialize rtai
 */
void __rt_mount_rtai(void)
{
	unsigned long flags, i;

	flags = hard_lock_all();

	rthal.do_IRQ = (void *) dispatch_irq;
	rthal.do_SRQ = (void *) dispatch_srq;
	rthal.do_TRAP = (void *) dispatch_traps;
	rthal.disint = linux_cli;
	rthal.enint = linux_sti;
	rthal.getflags = linux_save_flags;
	rthal.setflags = linux_restore_flags;
	rthal.getflags_and_cli = linux_save_flags_and_cli;
	rthal.fdisint = linux_cli;
	rthal.fenint = linux_sti;
	rthal.mounted = 1;

	INIT_LIST_HEAD(&irqlist);
	INIT_LIST_HEAD(&srqlist);
	memset(linux_irqs, 0, sizeof(linux_irqs));
	for (i = 0; i < NR_IRQS; i++) {
		/* Set the faked mask/unmask functions */
		if ((irq_desc[i].mask_ack || irq_desc[i].mask
		     || irq_desc[i].unmask)) {
			irq_desc[i].mask_ack = &linux_irq_mask_ack;
			irq_desc[i].mask = &linux_irq_mask;
			irq_desc[i].unmask = &linux_irq_unmask;
		}
		/* Store int. number for faster processing */
		linux_irqs[i].irq = i;

		/* Check, if an int is disabled. */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,4,18)
		linux_irqs[i].masked = irq_desc[i].disable_depth ? 2 : 0;
#else
		linux_irqs[i].masked = irq_desc[i].enabled ? 0 : 2;
#endif
	}
	modify_linux_ints(ints_disabled(flags));

	/* Save and replace the timer handler */
	saved_timer_action_handler = irq_desc[TIMER_8254_IRQ].action->handler;
	irq_desc[TIMER_8254_IRQ].action->handler = linux_timer_interrupt;

	/* Architecture specific mount code */
	arch_mount_rtai();

	hard_unlock_all(flags);
	printk("RTAI mounted:\n");
}

/* 
 * Restore the original linux functions
*/
void __rt_umount_rtai(void)
{
	int i;
	unsigned long flags;

	flags = hard_lock_all();

	arch_umount_rtai();

	rthal = linux_rthal;

	/* Restore trapped stuff */
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc[i] = linux_irq_desc_handler[i];
		irq_desc[i].mask_ack = ic_mask_and_ack_irq[i];
		irq_desc[i].mask = ic_mask_irq[i];
		irq_desc[i].unmask = ic_unmask_irq[i];
	}

	irq_desc[TIMER_8254_IRQ].action->handler =  saved_timer_action_handler;

	hard_unlock_all(flags);
	printk("RTAI unmounted\n");
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

/* 
 * Initialize some stuff on module load
 */
static __init int init_rtai_arm(void)
{
	unsigned int i;

	tuned.cpu_freq = FREQ_SYS_CLK;
	/* Save linux rthal for umount */
	linux_rthal = rthal;

	global.used_by_linux = ~0x0;
	spin_lock_init(&(global.data_lock));
	spin_lock_init(&(global.ic_lock));

	/* Get the linux interrupt handlers and mask/unmask functions */
	for (i = 0; i < NR_IRQS; i++) {
		global_irq[i].dest_status = 0;
		global_irq[i].handler = 0;

		linux_irq_desc_handler[i] = irq_desc[i];
		ic_mask_and_ack_irq[i] = irq_desc[i].mask_ack;
		ic_mask_irq[i] = irq_desc[i].mask;
		ic_unmask_irq[i] = irq_desc[i].unmask;
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

/* Cleanup before removal */
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

	PROC_PRINT("\nRTAI Real Time Kernel, Version: %s-ARM%s\n\n",
		   RTAI_RELEASE, ARM_VERSION);
	PROC_PRINT("    RTAI mount count: 1\n");
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
	PROC_PRINT("\n");
	if (global.lost_irqs) {
		PROC_PRINT("### Lost IRQs: %d ###\n", global.lost_irqs);
	}
#if DEBUG_LEVEL > 0
	/* This is really useful for system performance analysis 
	 * You can even tell a lot about linux behaviour, if you just
	 * load rtai. 
	 */
	PROC_PRINT("total ints: %lu\n", (unsigned long) totalints);
	PROC_PRINT("linux_sti total: %lu\n", (unsigned long) lstitot);
	PROC_PRINT("linux_sti processed: %lu\n", (unsigned long) lstiprocessed);
	PROC_PRINT("linux_srqs processed: %lu\n", (unsigned long) srqsprocessed);
	PROC_PRINT("linuxtime: %lu\n", (unsigned long) (linuxtime / (CLOCK_TICK_RATE / 1000)));
	PROC_PRINT("realtime: %lu\n", (unsigned long) (realtime / (CLOCK_TICK_RATE / 1000)));
	PROC_PRINT("ininttime: %lu\n", (unsigned long) (ininttime / (CLOCK_TICK_RATE / 1000)));
	PROC_PRINT("systime: %llu\n", ulldiv(rdtsc(), (CLOCK_TICK_RATE / 1000), (unsigned long *) &i));
	PROC_PRINT("rt_ints: 0x%08x\n", (unsigned int) rthal.rt_ints);
	for (i = 0; i < NR_IRQS; i++) {
		struct linux_irq *lirq = &linux_irqs[i];
		if (lirq->pended) {
			PROC_PRINT
			    ("irq %2d: prc %8d, pnd %2d, lost %3d, bogus %3d , pndl %3d, trig %8d, mskd %d, onlist %x\n",
			     i, lirq->pended, lirq->pending, lirq->lost,
			     lirq->bogus, lirq->pendedlinux,
			     lirq->triggered, lirq->masked,
			     (lirq->list.next ? 1 : 0) | (list_empty(&irqlist) ? 0 : 0x100));
		}
	}
#endif
	PROC_PRINT("\n");
	PROC_PRINT_DONE;
	
}	/* End function - rtai_read_rtai */


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
EXPORT_SYMBOL(rt_request_global_irq_ext);
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

/* Include the arch specific exports */
#include <asm/arch/rtai_exports.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RTAI Team");
MODULE_DESCRIPTION("RTAI for ARM");
