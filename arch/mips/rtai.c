/*
 * MIPS Port - Steve Papacharalambous (stevep@lineo.com) - 7 June 2001
 * COPYRIGHT (C) 2001 Steve Papacharalambous.
 * COPYRIGHT (C) 2001  Paolo Mantegazza (mantegazza@aero.polimi.it)
 *
 * This program is free software; you can redistribute it and/or modify * it under the terms of version 2 of the GNU General Public License as
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
 */

/*
 * ACKNOWLEDGMENTS:
 * - Steve Papacharalambous (stevep@zentropix.com) has contributed an
 * informative proc filesystem procedure.
 * Stuart Hughes (sehughes@zentropix.com) has helped a lot in debugging the
 * porting of this module to 2.4.xx.
 */

/*
 * Module to hook plain Linux up to do real time the way you like, hardware,
 * hopefully, fully trapped; the machine is in your hand, do what you want!
 */

/*
 * Modified for 2.4.18 and other various things by Steven Seeger
 * sseeger@stellartec.com 07/08/2002
 */

/*
 * Completely updated for the 2.4.22 MIPS kernel and modelled after Thomas Gleixner's new
 * implementation for ARM.
 * sseeger@stellartec.com 08/19/2003
 */

#define RTAI_VERSION "24.1.11-mips" //steve

#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/list.h>

#include <asm/system.h>
#include <asm/hw_irq.h>
#include <asm/io.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include "rtai_proc_fs.h"
#endif

#include <asm/rtai.h>
#include <asm/rtai_srq.h>

MODULE_LICENSE("GPL");

#define DEBUG_LEVEL 0
#if DEBUG_LEVEL > 0

#define getra(x) \
({ \
__asm__ __volatile__( \
    "\tmove\t%0, $31\t\n" \
    : "=r" (x) \
    : /*no inputs*/ \
    : "memory"); \
})
#endif
/* proc filesystem additions. */
static int rtai_proc_register(void);
static void rtai_proc_unregister(void);
/* End of proc filesystem additions. */

#undef RING_DEBUG
#ifdef RING_DEBUG
int blah=0;
int blah2=2;
int ring[1024][4];

static void add_ring(unsigned long val1, unsigned long val2, unsigned long val3, unsigned long val4)
{
    unsigned int flags;
    if (blah==6666) return;
    hard_save_flags_and_cli(flags);
    ring[blah][0]=val1;
    ring[blah][1]=val2;
    ring[blah][2]=val3;
    ring[blah][3]=val4;

    blah++;
    if (blah==1024) blah=0;
    hard_restore_flags(flags);
}

static void print_ring(void)
{
    int rtest=blah;
    int rcur = rtest+1;
    blah=6666;     

    printk("print time %d %d\n", rtest, rcur);

    while (rcur!=rtest) {
        if (rcur==1024) rcur=0;

        printk("%d %x %d %d\n", ring[rcur][0], ring[rcur][1], ring[rcur][2], ring[rcur][3]);
        rcur++;
    }   
}
#endif

/* Some defines. */

#undef CONFIG_RTAI_MOUNT_ON_LOAD
#define CONFIG_RTAI_MOUNT_ON_LOAD 1

#define IRQ_DESC ((irq_desc_t *)rthal.irq_desc)

#define HINT_DIAG_LSTI
#define HINT_DIAG_ECHO
#define HINT_DIAG_LRSTF

#ifdef HINT_DIAG_ECHO
#define HINT_DIAG_MSG(x) x
#else
#define HINT_DIAG_MSG(x)
#endif

/* External definitions. */
extern void *set_except_vector(int n, void *addr);

/* Function prototypes. */
unsigned long linux_save_flags_and_cli(void);
unsigned long linux_save_flags_and_sti(void);
unsigned long linux_save_flags_and_cli_cpuid(int);
void rtai_just_copy_back(unsigned long flags, int cpuid);

#define strip_flags(x) do { x&=~1; x|=(get_flag_ints()&1); x|=0x8000; } while(0)

static void rtai_linux_sti(struct pt_regs *regs);

/* Data declarations. */

static unsigned long linux_hz = 0;
static unsigned long rtai_hz = 0;
static unsigned int rt_timer_active;

static struct irq_handling {
    volatile unsigned long dest_status;
    void (*handler)(void);
} global_irq[NR_IRQS];

struct linux_irq {
    struct list_head list;
    int irq;
    int masked;
    int pending;
};

static struct list_head irqlist;
static struct linux_irq linux_irqs[NR_IRQS];

static struct cpu_own_status {
    volatile unsigned int intr_flag;
    volatile unsigned int linux_intr_flag;
    volatile unsigned int activ_irqs;
} processor[NR_RT_CPUS];

static struct sysrq_t {
    struct list_head list;
    int active;
    int pending;
    unsigned int label;
    void (*rtai_handler)(void);
    long long (*user_handler)(unsigned int whatever);
} sysrq[NR_SYSRQS];

static struct list_head srqlist;

extern unsigned long cycles_per_jiffy; 

static long long dispatch_mips_srq(unsigned int srq, unsigned int args);

struct rt_hal linux_rthal;

static void (*saved_linux_timer)(int , void *, struct pt_regs *);

static inline void enable_linux_ints(void)
{
    processor[hard_cpu_id()].intr_flag = 1;
}

static inline void disable_linux_ints(void)
{
    processor[hard_cpu_id()].intr_flag = 0;
}

static inline void set_flag_ints(unsigned int x)
{
    processor[hard_cpu_id()].intr_flag=x;
}

static inline unsigned int get_flag_ints(void)
{
    return processor[hard_cpu_id()].intr_flag;
}

static struct hw_interrupt_type *linux_irq_desc_handler[NR_IRQS];

static struct global_rt_status global;

volatile unsigned int *locked_cpus = &global.locked_cpus;

static void (*internal_ic_ack_irq[NR_IRQS]) (unsigned int irq);
static void (*ic_ack_irq[NR_IRQS]) (unsigned int irq);
static void (*ic_end_irq[NR_IRQS]) (unsigned int irq);
static void (*linux_end_irq[NR_IRQS]) (unsigned int irq);

static inline unsigned long hard_lock_all(void)
{
    unsigned long flags;

    hard_save_flags_and_cli(flags);
    return(flags);
} /* End function - hard_lock_all */

#define hard_unlock_all(flags) hard_restore_flags((flags))

/* 
 * We do the mask_ack on entry to dispatch_irq. The mask bit is set,
 * when we call the linux handler. It just keeps us informed, that
 * linux has really masked this int.
 * This is very useful for preventing reentrant entries for global
 * used interrupts.
 */

void linux_irq_mask_ack(unsigned int irq)
{
    void linux_irq_mask(unsigned int irq);

    if (linux_irqs[irq].masked==2) linux_irq_mask(irq);
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
    if (ic_ack_irq[irq] && !global_irq[irq].handler) {
        ic_ack_irq[irq] (irq);
    }
}

/* remove the masked bit for an linux irq and unmask it
*  Check if this int is pending and reschedule it. If
*  it is marked as pending in linux irq handler, then
*  throw it away, as it will come back here
*/
void linux_irq_unmask(unsigned int irq)
{
    struct linux_irq *lirq = &linux_irqs[irq];

    hard_cli(); //This function is only called with hard ints enabled, so this is ok.

    /*Is it pending or already on the list ? */
    if (lirq->pending || lirq->list.next) {
        /* Set it masked again */
        lirq->masked = 1;
        /* If its not on the list, set it there, but only if none
        * is pending in linux, as this will call the handler again,
        * so we will end up here again */
        if (!lirq->list.next) {
            lirq->pending--;
            if (!(IRQ_DESC[irq].status&IRQ_PENDING)) {
                list_add(&lirq->list, &irqlist);
            }
        }
    }
    else {
        lirq->masked = 0;
        /* Do not unmask, if it is owned by a global handler */
        if (linux_end_irq[irq] && !global_irq[irq].handler) {
#ifdef RING_DEBUG	   
            add_ring(irq,read_c0_status(), list_empty(&irqlist), 4);
#endif	   
            linux_end_irq[irq] (irq);
        }
    }

    hard_sti();
}

/*
 * This is used in place of the standard Linux kernel function as the
 * standard Linux assembler version doesn't work properly with the IDT
 * CPU (causes adel exceptions out of the wazoo) and the c version uses
 * cli which becomes a soft cli when rtai is mounted.  - Stevep
 */
static inline unsigned long rtai_xchg_u32(volatile int *m, unsigned long val)
{
    unsigned long flags, retval;

    hard_save_flags_and_cli(flags);
    retval = *m;
    *m = val;
    hard_restore_flags(flags);
    return(retval);
} /* End function - rtai_xchg_u32 */


/*
 * Functions to control Advanced-Programmable Interrupt Controllers (A-PIC).
 */

static void do_nothing_picfun(unsigned int irq) {}

unsigned int rt_startup_irq(unsigned int irq)
{
    unsigned long flags;
    int retval;

    hard_save_flags_and_cli(flags);
    retval = linux_irq_desc_handler[irq]->startup(irq);
    hard_restore_flags(flags);

    return(retval);
} /* End function - rt_startup_irq */

void rt_shutdown_irq(unsigned int irq)
{
    unsigned long flags;

    hard_save_flags_and_cli(flags);
    linux_irq_desc_handler[irq]->shutdown(irq);
    hard_restore_flags(flags);
} /* End function - rt_shutdown_irq */


void rt_enable_irq(unsigned int irq)
{
    unsigned long flags;

    hard_save_flags_and_cli(flags);
    linux_irq_desc_handler[irq]->enable(irq);
    hard_restore_flags(flags);
} /* End function - rt_enable_irq */


void rt_disable_irq(unsigned int irq)
{
    unsigned long flags;

    hard_save_flags_and_cli(flags);
    linux_irq_desc_handler[irq]->disable(irq);
    hard_restore_flags(flags);
} /* End function - rt_disable_irq */


void rt_mask_and_ack_irq(unsigned int irq)
{
    unsigned long flags;

    hard_save_flags_and_cli(flags);
    ic_ack_irq[irq](irq);
    hard_restore_flags(flags);
} /* End function - rt_mask_and_ack_irq */


void rt_ack_irq(unsigned int irq)
{
    unsigned long flags;

    hard_save_flags_and_cli(flags);
    internal_ic_ack_irq[irq](irq);
    hard_restore_flags(flags);
} /* End function - rt_ack_irq */


void rt_unmask_irq(unsigned int irq)
{
    unsigned long flags;

    hard_save_flags_and_cli(flags);
    linux_irq_desc_handler[irq]->enable(irq);
    hard_restore_flags(flags);
} /* End function - rt_unmask_irq */


/*
 * The functions below are the same as those above, except that we do not need
 * to save the hard flags as they have the interrupt bit set for sure.
 */

unsigned int trpd_startup_irq(unsigned int irq)
{
    unsigned int lflags;
    volatile unsigned int *lflagp;
    int retval;

    lflags = rtai_xchg_u32(lflagp = &processor[hard_cpu_id()].intr_flag, 0);
    retval = linux_irq_desc_handler[irq]->startup(irq);
    *lflagp = lflags;

    if (!linux_irqs[irq].list.next) linux_irqs[irq].masked = 0; //unmask since we just started up

    return(retval);
} /* End function - trpd_startup_irq */


void trpd_shutdown_irq(unsigned int irq)
{
    unsigned int lflags;
    volatile unsigned int *lflagp;

    lflags = rtai_xchg_u32(lflagp = &processor[hard_cpu_id()].intr_flag, 0);
    linux_irq_desc_handler[irq]->shutdown(irq);
    *lflagp = lflags;
} /* End function - trpd_shutdown_irq */


void trpd_enable_irq(unsigned int irq)
{
    unsigned int lflags;
    volatile unsigned int *lflagp;

    lflags = rtai_xchg_u32(lflagp = &processor[hard_cpu_id()].intr_flag, 0);
    linux_irq_desc_handler[irq]->enable(irq);
    *lflagp = lflags;
} /* End function - trpd_enable_irq */


void trpd_disable_irq(unsigned int irq)
{
    unsigned int lflags;
    volatile unsigned int *lflagp;

    lflags = rtai_xchg_u32(lflagp = &processor[hard_cpu_id()].intr_flag, 0);
    linux_irq_desc_handler[irq]->disable(irq);
    *lflagp = lflags;
} /* End function - trpd_disable_irq */

void trpd_end_irq(unsigned int irq)
{
    unsigned int lflags;
    volatile unsigned int *lflagp;

    lflags = rtai_xchg_u32(lflagp = &processor[hard_cpu_id()].intr_flag, 0);
    linux_end_irq[irq](irq);
    *lflagp = lflags;
} /* End function - trpd_end_irq */


void trpd_set_affinity(unsigned int irq, unsigned long mask)
{
    unsigned int lflags;
    volatile unsigned int *lflagp;

    lflags = rtai_xchg_u32(lflagp = (int *)&processor[hard_cpu_id()].intr_flag, 0);
    linux_irq_desc_handler[irq]->set_affinity(irq, mask);
    *lflagp = lflags;
} /* End function - trpd_set_affinity */


static struct hw_interrupt_type trapped_linux_irq_type = {
    "RT SPVISD",
    trpd_startup_irq,
    trpd_shutdown_irq,
    trpd_enable_irq,
    trpd_disable_irq,
    linux_irq_mask_ack,
    linux_irq_unmask,
    trpd_set_affinity};

static struct hw_interrupt_type real_time_irq_type = {
    "REAL TIME",
    (unsigned int (*)(unsigned int))do_nothing_picfun,
    do_nothing_picfun,
    do_nothing_picfun,
    do_nothing_picfun,
    linux_irq_mask_ack,
    linux_irq_unmask,
    (void (*)(unsigned int, unsigned long)) do_nothing_picfun};

void rt_switch_to_linux(int cpuid)
{      
    set_bit(cpuid, &global.used_by_linux);
    set_flag_ints(processor[cpuid].linux_intr_flag);
} /* End function - rt_switch_to_linux */

void rt_switch_to_real_time(int cpuid)
{
    processor[cpuid].linux_intr_flag = get_flag_ints();
    disable_linux_ints();
    clear_bit(cpuid, &global.used_by_linux);
} /* End function - rt_switch_to_linux */

/*
 * Request and free interrupts, system requests and interprocessors messages
 * Request for regular Linux irqs also included. They are nicely chained to
 * Linux, forcing sharing with any already installed handler, so that we can
 * have an echo from Linux for global handlers. We found that usefull during
 * debug, but can be nice for a lot of other things, e.g. see the jiffies
 * recovery in rtai_sched.c, and the global broadcast to local apic timers.
 */

static unsigned long irq_action_flags[NR_GLOBAL_IRQS];
static int chained_to_linux[NR_GLOBAL_IRQS];

int rt_request_global_irq(unsigned int irq, void (*handler)(void))
{

    unsigned long flags;

    if (irq >= NR_GLOBAL_IRQS  || !handler) {
        return(-EINVAL);
    }

    if (global_irq[irq].handler) {
        return(-EBUSY);
    }

    flags = hard_lock_all();
    IRQ_DESC[irq].handler = &real_time_irq_type;
    global_irq[irq].handler = handler;
    linux_end_irq[irq] = do_nothing_picfun;
    hard_unlock_all(flags);

    return(0);

}  /* End function - rt_request_global_irq */


int rt_free_global_irq(unsigned int irq)
{

    unsigned long flags;

    if (irq >= NR_GLOBAL_IRQS || !global_irq[irq].handler) {
        return(-EINVAL);
    }

    flags = hard_lock_all();
    IRQ_DESC[irq].handler = &trapped_linux_irq_type;
    global_irq[irq].handler = 0;
    linux_end_irq[irq] = ic_end_irq[irq];
    hard_unlock_all(flags);
    return(0);

}  /* End function - rt_free_global_irq */


int rt_request_linux_irq(unsigned int irq,
                         void (*linux_handler)(int irq, void *dev_id, struct pt_regs *regs),
                         char *linux_handler_id, void *dev_id)
{

    unsigned long flags, lflags;

    if (irq >= NR_GLOBAL_IRQS || !linux_handler) {
        return(-EINVAL);
    }
    lflags = linux_save_flags_and_cli();
    spin_lock_irqsave(&(IRQ_DESC[irq].lock), flags);
    if (!chained_to_linux[irq]++) {
        if (IRQ_DESC[irq].action) {
            irq_action_flags[irq] = IRQ_DESC[irq].action->flags;
            IRQ_DESC[irq].action->flags |= SA_SHIRQ;
        }
    }
    spin_unlock_irqrestore(&(IRQ_DESC[irq].lock), flags);
    request_irq(irq, linux_handler, SA_SHIRQ, linux_handler_id, dev_id);
    rtai_just_copy_back(lflags, hard_cpu_id());
    return(0);

}  /* End function - rt_request_linux_irq */


int rt_free_linux_irq(unsigned int irq, void *dev_id)
{

    unsigned long flags, lflags;

    if (irq >= NR_GLOBAL_IRQS || !chained_to_linux[irq]) {
        return -EINVAL;
    }
    lflags = linux_save_flags_and_cli();
    free_irq(irq, dev_id);
    spin_lock_irqsave(&(IRQ_DESC[irq].lock), flags);
    if (!(--chained_to_linux[irq]) && IRQ_DESC[irq].action) {
        IRQ_DESC[irq].action->flags = irq_action_flags[irq];
    }
    spin_unlock_irqrestore(&(IRQ_DESC[irq].lock), flags);
    rtai_just_copy_back(lflags, hard_cpu_id());
    return(0);
}


/* Set this irq pending for linux.
*/
void rt_pend_linux_irq(int irq)
{
    unsigned long flags;
    struct linux_irq *lirq = &linux_irqs[irq];

    hard_save_flags_and_cli(flags);

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
        goto out;
    }
    /* Mask it again */
    lirq->masked = 1;

    /* If there is a pending one, throw it away and count it as bogus.
        * That happens especially on clps71xx uarts. Maybe I have a look into the
        * driver code sometimes. But this hardware is really weird!
        */
    if (IRQ_DESC[irq].status&IRQ_PENDING) {
        goto out;
    }

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
    list_add(&lirq->list, &irqlist);
    out:
    hard_restore_flags(flags);
}

int rt_request_srq(unsigned int label, void (*rtai_handler)(void),
                   long long (*user_handler)(unsigned int whatever))
{

    unsigned long flags;
    int srq;

    flags = rt_spin_lock_irqsave(&global.data_lock);
    if (!rtai_handler) {
        rt_spin_unlock_irqrestore(flags, &global.data_lock);
        return(-EINVAL);
    }

    for (srq = 2; srq < NR_SYSRQS; srq++) {
        if (!(sysrq[srq].rtai_handler)) {
            sysrq[srq].rtai_handler = rtai_handler;
            sysrq[srq].label = label;
            if (user_handler) {
                sysrq[srq].user_handler = user_handler;
            }
            rt_spin_unlock_irqrestore(flags, &global.data_lock);
            return(srq);
        }  /* End if - this srq slot is free. */
    }  /* End for loop - locate a free srq slot. */

    rt_spin_unlock_irqrestore(flags, &global.data_lock);
    return(-EBUSY);

}  /* End function - rt_request_srq */


int rt_free_srq(unsigned int srq)
{

    unsigned long flags;

    flags = rt_spin_lock_irqsave(&global.data_lock);
    if (srq < 2 || srq >= NR_SYSRQS || !sysrq[srq].rtai_handler) {
        rt_spin_unlock_irqrestore(flags, &global.data_lock);
        return(-EINVAL);
    }
    sysrq[srq].rtai_handler = 0;
    sysrq[srq].user_handler = 0;
    sysrq[srq].label = 0;
    rt_spin_unlock_irqrestore(flags, &global.data_lock);
    return(0);

}  /* End function - rt_free_srq */

/*
 * Pend a sysrequest
 */
void rt_pend_linux_srq(unsigned int srq)
{
    unsigned long flags;
    struct sysrq_t *sysr;

    if (srq < 0 || srq >= NR_SYSRQS) {
        printk("Invalid SRQ! (pend:) %d\n", srq);
        return;
    }
    hard_save_flags_and_cli(flags);
    sysr = &sysrq[srq];
    if (sysr->active || sysr->list.next) {
        sysr->pending++;
    }
    else {
        list_add(&sysr->list, &srqlist);
    }   
    hard_restore_flags(flags);   
}

static asmlinkage void linux_soft_sti(void)
{
   enable_linux_ints();   
}


/*
 * Linux cli/sti emulation routines.
 */
/* Disable linux int's */
static asmlinkage void linux_cli(void)
{
    disable_linux_ints();
}

/*
*  Enable linux int's, check for pending int's to be delivered
 */
static asmlinkage void linux_sti(void)
{
    struct pt_regs regs;

#if DEBUG_LEVEL > 1
     {
	unsigned int flags, ra;
	hard_save_flags(flags);
	if (!(flags&1)) {
        hard_sti();
	   getra(ra);
	   hard_sti();
	   printk("wtf! linux_sti 0x%08x\n", ra);
	   BUG();
	}
     }   
#endif

#ifdef RING_DEBUG    
    add_ring(666,0,0,0);
#endif   
    rtai_linux_sti(&regs);
}

static unsigned long linux_save_flags(void)
{
    unsigned long flags;

    hard_save_flags(flags);
#if DEBUG_LEVEL > 1
    if (!(flags&1)) {
        unsigned int ra;
        getra(ra);
        hard_sti();
        printk("wtf! linux_save_flags 0x%08x\n", ra);
        BUG();
    }
#endif   
    strip_flags(flags);
#ifdef RING_DEBUG
    add_ring(330, flags, 0, 0);
#endif      
    return flags;
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

    /* Grab the lock, as we are playing with a list */

#ifdef RING_DEBUG   
    add_ring(999, read_c0_status(), 0, 0);
#endif   
    rt_spin_lock_irq(&(global.data_lock));

    global.cpu_in_sti++;
    /* loop here until all int's are served */
    while (!list_empty(&irqlist)) {
        struct linux_irq *curr = (struct linux_irq *) irqlist.next;

        /* Remove it from the list */
        list_del(&curr->list);
        curr->list.next = NULL;

        /* was it masked in meantime ?  WTF disabled the driver ??
            * Throw it away and mark it as lost.
            * This can happen, if a driver is unloaded or disabled from
            * non interrupt context. Deal with it, else it will
            * remain pending and eventually disturb the driver on reload
            * or reenable
     */
        if (curr->masked == 2) {
            curr->pending++;
//	  printk("lost %d?\n", curr->irq);
            continue;
        }
        /* Disable linux int's */
        disable_linux_ints();
        /* call old Linux do_IRQ() to handle IRQs, which in fact calls sti/cli again
            * so we will be reentrant at that point
            * Unlock so other ints can be handled by dispatch_irq
            * Unmasking is done in do_IRQ, so we don't need to handle this here
            */
#ifdef RING_DEBUG
        add_ring(curr->irq, read_c0_status(), list_empty(&irqlist), 3);
#endif       
        rt_spin_unlock_irq(&(global.data_lock));
       
       linux_rthal.mips_interrupt(curr->irq, regs);
#ifdef RING_DEBUG       
        add_ring(curr->irq, read_c0_status(), 0, 5);
#endif       
        /* Grab the lock again */
        rt_spin_lock_irq(&(global.data_lock));
        /* Enable linux int's again like we do in the real linux handler */
        enable_linux_ints();
    }

    while (!list_empty(&srqlist)) {
        struct sysrq_t *curr = (struct sysrq_t *) srqlist.next;
        /* Remove it from the list */
        list_del(&curr->list);
        curr->list.next = NULL;
        curr->active = 1;
        /* Go there with int's enabled */
        enable_linux_ints();
        rt_spin_unlock_irq(&(global.data_lock));
        if (curr->rtai_handler)
            curr->rtai_handler();
        /* Grab the lock again */
        rt_spin_lock_irq(&(global.data_lock));
        curr->active = 0;
        if (curr->pending) {
            curr->pending--;
            list_add(&curr->list, &srqlist);
        }
    }

    /* reenable in any case */
    enable_linux_ints();
    global.cpu_in_sti--;
    /* release the lock, enable int's and get out of here */
    rt_spin_unlock_irq(&(global.data_lock));
}
/* Restore flags
*/
static void linux_restore_flags(unsigned long flags)
{
    struct pt_regs regs;
   
#if DEBUG_LEVEL > 1
     {
	unsigned int hflags;
	
	hard_save_flags(hflags);
	if (!hflags&1) {
	   hard_save_flags(hflags);
	   getra(regs.regs[31]);
	   hard_sti();	   
	   printk("wtf? linux_restore_flags 0x%08x\n", regs.regs[31]);
	   BUG();
	}
     }
   
#endif
#ifdef RING_DEBUG
    add_ring(333, flags,0,0);
#endif   
    if (!(flags & 1)) {
        disable_linux_ints();
    }
    else {
        /* Enable linux ints */
        rtai_linux_sti(&regs);
    }
}

//this is added so rtai_fifos doesn't complain
unsigned long linux_save_flags_and_cli_cpuid(int bs)
{
    return linux_save_flags_and_cli();
}

unsigned long linux_save_flags_and_sti(void)
{
    unsigned long flags;
    struct pt_regs regs;

#if DEBUG_LEVEL > 1
    hard_save_flags(flags);
    if (!flags&1) {
        hard_sti();
        getra(regs.regs[31]);
        printk("wtf? linux_save_flags_and_sti 0x%08x\n", regs.regs[31]);
        BUG();
    }
#endif

    rtai_linux_sti(&regs);

    hard_save_flags(flags);
    strip_flags(flags);
#ifdef RING_DEBUG
    add_ring(331, flags, 0, 0);
#endif   

    return flags;
}

/* Save flags and disable linux int's
* We return real, but faked flags, see linux_save_flags
*/
unsigned long linux_save_flags_and_cli(void)
{
    unsigned long flags;


    hard_save_flags(flags);

#if DEBUG_LEVEL > 1
    if (!(flags&1)) {
        unsigned long ra;

        hard_sti();
        getra(ra);
        printk("wtf? linux_save_and_cli 0x%08x\n", ra);
        BUG();
    }
#endif

    strip_flags(flags);
    disable_linux_ints();
#ifdef RING_DEBUG
    add_ring(332, flags, 0, 0);
#endif      
    return flags;
}


void rtai_just_copy_back(unsigned long flags, int cpuid)
{
    if (flags&1) enable_linux_ints();
    else disable_linux_ints();
} /* End function - rtai_just_copy_back */

static asmlinkage unsigned int dispatch_mips_timer_interrupt(int irq,
                                                             struct pt_regs *regs)
{
    static asmlinkage unsigned int dispatch_mips_interrupt(int irq, struct pt_regs *regs);

    //TODO: update the kernel
    return dispatch_mips_interrupt(irq, regs);
} /* End function - dispatch_mips_timer_interrupt */
static asmlinkage unsigned int dispatch_mips_interrupt(int irq,
                                                       struct pt_regs *regs)
{            
#ifdef RING_DEBUG
    add_ring(irq, read_c0_status(), get_flag_ints(), 1);
#endif   

   if (ic_ack_irq[irq]) ic_ack_irq[irq](irq);
   
#ifdef RING_DEBUG   
    if (irq==IRQ_TIMER) blah2=0;
    else if (irq==26) { //26 is my network card irq, and this test is run with ping -f on. lots of irq 26s and few timers.
        blah2++;
        if (blah2==15) {
            if (blah==6666) goto out;
            print_ring();
            printk("die\n");
            while (1);
        }
    }
    out:
#endif

    //call realtime handler immediately
    if (global_irq[irq].handler) {
        global_irq[irq].handler();
        linux_irq_desc_handler[irq]->enable(irq); //call enable intead of end to avoid the kernel's
                                                  //check for IRQ_PENDING and IRQ_INPROGRESS
    }
    else {
#ifdef RING_DEBUG       
        add_ring(irq, read_c0_status(), linux_irqs[IRQ_TIMER].list.next, 2);
#endif   
        rt_pend_linux_irq(irq);
    }


    if (global.used_by_linux && (get_flag_ints())) {
        if (linux_irqs[irq].list.next) {
            disable_linux_ints();
            rtai_linux_sti(regs);
	   hard_cli();
        }
    }
   
#ifdef RING_DEBUG   
    add_ring(irq, read_c0_status(), 0, 7);
#endif
  
    return 1;
} /* End function - dispatch_mips_interrupt */


/*
 * RTAI mount-unmount functions to be called from the application to
 * initialise the real time application interface, i.e. this module, only
 * when it is required; so that it can stay asleep when it is not needed
 */

#ifdef CONFIG_RTAI_MOUNT_ON_LOAD
#define rtai_mounted 1
#else
static int rtai_mounted;
#ifdef CONFIG_SMP
static spinlock_t rtai_mount_lock = SPIN_LOCK_UNLOCKED;
#endif
#endif


/*
 * Trivial, but we do things carefully, the blocking part is relatively short,
 * should cause no troubles in the transition phase.
 * All the zeroings are strictly not required as mostly related to static data.
 * Done esplicitly for emphasis. Simple, just lock and grab everything from
 * Linux.
 */

void __rt_mount_rtai(void)
{
    static void rt_printk_sysreq_handler(void);
    int i;
    unsigned long flags;

    flags = hard_lock_all();
    rthal.disint = linux_cli;
    rthal.enint = linux_sti;
    rthal.rtai_active = 0xffffffff;
    rthal.getflags = linux_save_flags;
    rthal.setflags = linux_restore_flags;
    rthal.getflags_and_cli = linux_save_flags_and_cli;
    rthal.getflags_and_sti = linux_save_flags_and_sti;
    rthal.mips_timer_interrupt = dispatch_mips_timer_interrupt;
    rthal.mips_interrupt = dispatch_mips_interrupt;
    rthal.rtai_srq_interrupt = dispatch_mips_srq;
    rthal.soft_enint = linux_soft_sti;
    rthal.tsc.tsc = 0;

    INIT_LIST_HEAD(&irqlist);
    INIT_LIST_HEAD(&srqlist);

    memset(linux_irqs, 0, sizeof(linux_irqs));

    for (i = 0; i < NR_IRQS; i++) {
        IRQ_DESC[i].handler = &trapped_linux_irq_type;

        linux_irqs[i].irq = i;
        linux_irqs[i].masked = IRQ_DESC[i].depth ? 2 : 0;
    }
    saved_linux_timer = IRQ_DESC[IRQ_TIMER].action->handler;
    IRQ_DESC[IRQ_TIMER].action->handler = rthal.linux_mips_timer_intr;

    sysrq[1].rtai_handler = rt_printk_sysreq_handler;

    hard_unlock_all(flags);

    printk("\n***** RTAI NEWLY MOUNTED (MOUNT COUNT %d) ******\n\n", rtai_mounted);

} /* End function - __rt_mount_rtai */



/*
 * Simple, now we can simply block other processors and copy original data back
 * to Linux.
 */

void __rt_umount_rtai(void)
{

    int i;

    printk("empty? %x %x %x\n", read_c0_status(), get_flag_ints(), list_empty(&irqlist));

    hard_cli();
    rthal = linux_rthal;
    for (i = 0; i < NR_IRQS; i++) {
        IRQ_DESC[i].handler = linux_irq_desc_handler[i];
    }

    hard_sti();

    printk("\n***** RTAI UNMOUNTED (MOUNT COUNT %d) ******\n\n", rtai_mounted);
} /* End function - __rt_*/



#ifdef CONFIG_RTAI_MOUNT_ON_LOAD
void rt_mount_rtai(void) {MOD_INC_USE_COUNT;}
void rt_umount_rtai(void) {MOD_DEC_USE_COUNT;}
#else
void rt_mount_rtai(void)
{

    rt_spin_lock(&rtai_mount_lock);
    rtai_mounted++;
    MOD_INC_USE_COUNT;
    if (rtai_mounted == 1) {
        __rtai_mount_rtai();
    }
    rt_spin_unlock(&rtai_mount_lock);
} /* End function - rt_mount_rtai */



void rt_umount_rtai(void)
{

    rt_spin_lock(&rtai_mount_lock);
    rtai_mounted--;
    MOD_DEC_USE_COUNT;
    if (!rtai_mounted) {
        __rtai_umount_rtai();
    }
    rt_spin_unlock(&rtai_mount_lock);

} /* End function - rt_umount_rtai */

#endif



/*
 * Module parameters to allow frequencies to be overriden via insmod.
 */
static int CpuFreq = 0;
MODULE_PARM(CpuFreq, "i");


/*
 * Module initialization and cleanup.
 */

/*
 * Let's prepare our side without any problem, so that there remain just a few
 * things to be done when mounting RTAI. All the zeroings are strictly not
 * required as mostly related to static data. Done esplicitly for emphasis.
 */
static int __init rtai_init(void)
{
    extern unsigned int mips_hpt_frequency;
    unsigned int i;

    /*
     * Passed in CPU frequency overides auto detected Linux value.
     */
    if (CpuFreq == 0) {
        CpuFreq = mips_hpt_frequency;
    }
    tuned.cpu_freq = CpuFreq;
    printk("rtai: Using cpu_freq %d\n", tuned.cpu_freq);

    linux_rthal = rthal;

    global.activ_irqs = 0;
    global.activ_srqs = 0;
    global.cpu_in_sti = 0;
    global.used_by_linux = ~(0xFFFFFFFF << smp_num_cpus);
    global.locked_cpus = 0;
    spin_lock_init(&(global.data_lock));
    spin_lock_init(&global.global.ic_lock);

    for (i = 0; i < NR_RT_CPUS; i++) {
        processor[i].intr_flag = 1;
        processor[i].linux_intr_flag = 1;
        processor[i].activ_irqs = 0;
    }

    for (i = 0; i < NR_IRQS; i++) {
        global_irq[i].dest_status=0;
        global_irq[i].handler = 0;
        linux_irq_desc_handler[i] = IRQ_DESC[i].handler;
        ic_ack_irq[i] = internal_ic_ack_irq[i] =
                        linux_irq_desc_handler[i]->ack;
        linux_end_irq[i] = ic_end_irq[i] =
                           linux_irq_desc_handler[i]->end;
    }

    /*
     * Initialize the Linux tick period.
     */
    linux_hz = (long)(mips_hpt_frequency) / cycles_per_jiffy;
    rt_timer_active = 0;

#ifdef CONFIG_RTAI_MOUNT_ON_LOAD
    __rt_mount_rtai();
#endif

#ifdef CONFIG_PROC_FS
    rtai_proc_register();
#endif

    return(0);
} /* End function - rtai_init */


static void __exit rtai_end(void)
{
#ifdef CONFIG_RTAI_MOUNT_ON_LOAD
    __rt_umount_rtai();
#endif

#ifdef CONFIG_PROC_FS
    rtai_proc_unregister();
#endif

    return;
} /* End function - rtai_end */


module_init(rtai_init);
module_exit(rtai_end);
MODULE_DESCRIPTION("RTAI core services module.");
MODULE_AUTHOR("Paolo Mantegazza <mantegazza@aero.polimi.it>");


/* ----------------------< proc filesystem section >---------------------- */
#ifdef CONFIG_PROC_FS

struct proc_dir_entry *rtai_proc_root = NULL;  

static int rtai_read_rtai(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{

    PROC_PRINT_VARS;
    int i;

    PROC_PRINT("\nRTAI Real Time Kernel, Version: %s\n\n", RTAI_VERSION);
    PROC_PRINT("    RTAI mount count: %d\n", rtai_mounted);
#ifdef CONFIG_SMP
    PROC_PRINT("    APIC Frequency: %d\n", tuned.apic_freq);
    PROC_PRINT("    APIC Latency: %d ns\n", LATENCY_APIC);
    PROC_PRINT("    APIC Setup: %d ns\n", SETUP_TIME_APIC);
#endif
    PROC_PRINT("\nGlobal irqs used by RTAI: \n");
    for (i = 0; i < NR_GLOBAL_IRQS; i++) {
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

    PROC_PRINT("\nMIPS Timer controlled by: %s\n",
               rt_timer_active ? "RTAI" : "Linux");
    if (rt_timer_active) {
        PROC_PRINT("RTAI Tick Frequency: %ldHz\n", rtai_hz);
    }
    else {
        PROC_PRINT("Linux Tick Frequency: %ldHz\n", linux_hz);
    }

    PROC_PRINT("RTHAL TSC: 0x%.8lx%.8lx\n",
               rthal.tsc.hltsc[1], rthal.tsc.hltsc[0]);

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
/* ------------------< end of proc filesystem section >------------------ */

/* ---------------------------< Timer section >--------------------------- */

/********** SOME TIMER FUNCTIONS TO BE LIKELY NEVER PUT ELSWHERE *************/

/*
 * Real time timers. No oneshot, and related timer programming, calibration.
 * Use the utility module. It is also been decided that this stuff has to
 * stay here.
 */

struct calibration_data tuned;
struct rt_times rt_times = {0};



/*
 * Restore the MIPS timer to its Linux kernel value.
 */
void restore_mips_timer(unsigned long linux_tick)
{

    unsigned long flags;
    unsigned long cp0_compare = 0;  /* Counter value at next timer irq */

    flags = hard_lock_all();
    cp0_compare = ((unsigned long)read_c0_count());
    cp0_compare += linux_tick;
    write_c0_compare(cp0_compare);
    hard_unlock_all(flags);

}  /* End function - restore_mips_timer */


int rt_request_timer(void (*handler)(void), unsigned int tick, int apic)
{

    unsigned long flags;
    int r_c = 0;
    RTIME t;

    flags = hard_lock_all();

    t = rdtsc();

    rt_times.linux_tick = cycles_per_jiffy;
    rt_times.periodic_tick = tick > 0 && tick < cycles_per_jiffy ? tick :
                             rt_times.linux_tick;
    rt_times.tick_time = t;
    rt_times.intr_time = rt_times.tick_time + rt_times.periodic_tick;
    rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
    rt_free_global_irq(IRQ_TIMER);
    if (rt_request_global_irq(IRQ_TIMER, handler) < 0) {
        r_c = -EINVAL;
        goto set_timer_exit;
    }

    IRQ_DESC[IRQ_TIMER].action->handler = rthal.linux_soft_mips_timer_intr;
    rt_set_timer_delay(rt_times.periodic_tick);
    if (rt_times.periodic_tick != 0) {
        rtai_hz = tuned.cpu_freq / rt_times.periodic_tick;
    }
    rt_timer_active = 1;

    set_timer_exit:
    hard_unlock_all(flags);
    return(r_c);

} /* End function - rt_request_timer */


int rt_free_timer(void)
{

    unsigned long flags;
    int r_c = 0;

    flags = hard_lock_all();
    clear_c0_status(IE_IRQ5);
    rt_timer_active = 0;
    IRQ_DESC[IRQ_TIMER].action->handler = rthal.linux_mips_timer_intr;
    if (rt_free_global_irq(IRQ_TIMER) < 0) {
        r_c = -EINVAL;
    }

    restore_mips_timer(cycles_per_jiffy);
    set_c0_status(IE_IRQ5);
    hard_unlock_all(flags);
    return(r_c);

}  /* End function - rt_free_timer */


/* ------------------------< End of timer section >----------------------- */

/* ---------------------------< printk section >--------------------------- */

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
    while (c) {
        if ((c->flags & CON_ENABLED) && c->write) {
            c->write(c, display, len);
        }
        c = c->next;
    }
    rt_spin_unlock_irqrestore(flags, &display_lock);

    return(len);

}  /* End function - rtai_print_to_screen */


/*
 * rt_printk.c, hacked from linux/kernel/printk.c.
 *
 * Modified for RT support, David Schleef.
 *
 * Adapted to RTAI, and restyled his way by Paolo Mantegazza. Now it has been
 * taken away from the fifos module and has become an integral part of the
 * basic RTAI module.
 */

#define PRINTK_BUF_LEN  (4096*2) /* Some programs generate much output. PC */
#define TEMP_BUF_LEN    (256)

static char rt_printk_buf[PRINTK_BUF_LEN];
static int buf_front, buf_back;

static char buf[TEMP_BUF_LEN];


int rt_printk(const char *fmt, ...)
{

    static spinlock_t display_lock = SPIN_LOCK_UNLOCKED;
    va_list args;
    int len, i;
    unsigned long flags, lflags; //steve added lflags at othersteve's suggestion
   
//    lflags = linux_save_flags_and_cli();
    lflags = rtai_xchg_u32(&processor[hard_cpu_id()].intr_flag, 0);    
    flags = rt_spin_lock_irqsave(&display_lock);
    va_start(args, fmt);
    len = vsprintf(buf, fmt, args);
    va_end(args);
    if (buf_front + len >= PRINTK_BUF_LEN) {
        i = PRINTK_BUF_LEN - buf_front;
        memcpy(rt_printk_buf + buf_front, buf, i);
        memcpy(rt_printk_buf, buf + i, len - i);
        buf_front = len - i;
    }
    else {
        memcpy(rt_printk_buf + buf_front, buf, len);
        buf_front += len;
    }

    rt_spin_unlock_irqrestore(flags, &display_lock);
    rtai_just_copy_back(lflags, hard_cpu_id());
    rt_pend_linux_srq(1);

    return len;
}  /* End function - rt_printk */



static asmlinkage void rt_printk_sysreq_handler(void)
{

    int tmp;

    while (1) {
        tmp = buf_front;
        if (buf_back  > tmp) {
            printk("%.*s", PRINTK_BUF_LEN - buf_back,
                   rt_printk_buf + buf_back);
            buf_back = 0;
        }
        if (buf_back == tmp) {
            break;
        }
        printk("%.*s", tmp - buf_back, rt_printk_buf + buf_back);
        buf_back = tmp;
    }

    sysrq[1].pending=0;
}  /* End function - rt_printk_sysreq_handler */

/* ------------------------< End of printk section >----------------------- */

/* -------< srq section - handling userspace requests via syscall >-------- */

static long long dispatch_mips_srq(unsigned int srq, unsigned int args)
{
    int i;

    if (srq>0&&srq<NR_SYSRQS) {
        if (sysrq[srq].user_handler) return sysrq[srq].user_handler(args);
    }
    else if (srq==0) { //find srq based on label
        for (i=1;i<NR_SYSRQS;i++) {
            if (sysrq[i].label==args) {
                return(long long)i;
            }
        }
    }

    return 0; //invalid srq or srq not found(srq==0)
}

/* ---------------------------< End of srq section >----------------------- */
/* ----------------------------< End of File >--------------------------- */

