/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)
              2001  David Schleef <ds@schleef.org>
              2001  Lineo, Inc. <ds@lineo.com>

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
ACKNOWLEDGMENTS (LIKELY JUST A PRELIMINARY DRAFT): 
- Steve Papacharalambous (stevep@zentropix.com) has contributed an informative 
  proc filesystem procedure.
*/

/*
 * This code has been almost completely rewritten since about 24.1.4.
 * Apart from the functions that are not yet reimplemented, the
 * following features need more work or are non-existent:
 *
 *   - calling real-time handlers
 *
 */

#define DEBUG_MOUNT
//#define DEBUG_ME_HARDER
#define DEBUG_FAKE_IRQ
#define DEBUG_HARD_FLAGS

//#define unimplemented

#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <asm/system.h>
#include <asm/hw_irq.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <asm/bitops.h>
#include <asm/atomic.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include "rtai_proc_fs.h"
#endif

#include <rtai_trace.h>

#include <asm/rtai.h>
#include <asm/rtai_srq.h>

MODULE_LICENSE("GPL");

int rtai_proc_register(void);
void rtai_proc_unregister(void);

int rt_printk_init(void);
void rt_printk_cleanup(void);


/* Hook in the interrupt return path */
extern void (*rtai_soft_sti)(void);

void ppc_irq_dispatch_handler(struct pt_regs *regs, int irq);

#ifdef DEBUG_ME_HARDER
/*
 * This is pretty vicious with the console.  Not to be used lightly.
 */
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/console_struct.h>

void debug_ptr(unsigned long ptr,int irq)
{
	int i;
	static int j=0;
	char s[80];
	unsigned int flags;

__asm__ __volatile__("\tmfmsr %0\n\trlwinm %0,%0,0,17,15\n\tmtmsr %0\n":"=r" (flags));

	sprintf(s,"%08lx %d ",ptr,irq);
	for(i=0;i<strlen(s);i++){
		conswitchp->con_putc(vc_cons[fg_console].d,
			((vc_cons[fg_console].d->vc_attr)<<8)|s[i],
			(j&0x1f)+48,i+16*(j/32));
	}
	j++;
	if(j>=32*4)j=0;
__asm__ __volatile__("\tmtmsr %0\n"::"r" (flags));
}

#undef RT_BUG
#define RT_BUG() do{ debug_ptr(0,__LINE__); while(1); }while(0)
#else
#define debug_ptr(a,b)
#endif

static unsigned long old_flags;
void debug_check_flags(void *ptr)
{
	unsigned long current_flags;

	__asm__ __volatile__("\tmfmsr %0\n":"=r" (current_flags));
	current_flags&=MSR_EE;
	if(old_flags!=current_flags){
		if(!ptr)ptr = __builtin_return_address(0);
		debug_ptr(ptr,current_flags);
		/* bogus, but LTT isn't very extensible */
		TRACE_RTAI_TRAP_ENTRY((current_flags>>8)|1,(unsigned long)ptr);
	}
	old_flags=current_flags;
}
void debug_hard_flags(void *ptr,unsigned long flags)
{
	unsigned long current_flags;

	__asm__ __volatile__("\tmfmsr %0\n":"=r" (current_flags));
	current_flags&=MSR_EE;
	if(old_flags!=current_flags){
		if(!ptr)ptr = __builtin_return_address(0);
		debug_ptr(ptr,current_flags);
	}
	old_flags = flags&MSR_EE;
}
void debug_fix_flags(void)
{
	__asm__ __volatile__("\tmfmsr %0\n":"=r" (old_flags));
	old_flags&=MSR_EE;
}
int debug_trace;

/* some defines */

#define PIRQ_IDLE 0
#define PIRQ_PENDING 1
#define PIRQ_RUNNING 2

/* global variables */

static int global_priority;

static struct pt_regs rtai_regs = {  // Dummy registers.
	msr: MSR_USER,
};

static spinlock_t interrupt_controller_lock = SPIN_LOCK_UNLOCKED;

/* Our IRQ structures */

#define IRQ_TYPE_IRQ	0
#define IRQ_TYPE_TIMER	1
#define IRQ_TYPE_SRQ	2

typedef struct rt_irq_state_struct rt_irq_state;

struct rt_irq_state_struct {
	int			irq;
	int			type;
	int			priority;
	rt_irq_state		*pnext;
	rt_irq_state		*pprev;
	hw_irq_controller	*handler;
	unsigned int		status;
	int 			(*action)(int,void *,struct pt_regs *);
	void *dev_id;
	unsigned long		arg;
};

static rt_irq_state rt_irq_desc[NR_IRQS];
static rt_irq_state rt_timer_desc = { type: IRQ_TYPE_TIMER };
#define NR_SRQS 16
static rt_irq_state rt_srq_desc[NR_SRQS] =
	{ [0 ... NR_SRQS-1] = { type: IRQ_TYPE_SRQ } };

/* Pending IRQs are put on a linked list.  This is the head
 * of the list and the spinlock */
static rt_irq_state pending_irq_list;
static spinlock_t pending_irq_list_lock = SPIN_LOCK_UNLOCKED;

/* timers */

typedef struct rt_timer_struct rt_timer;

struct rt_timer_struct {
	rt_timer *pnext;
	rt_timer *pprev;
	unsigned int expires;
	void (*handler)(rt_timer *);
	unsigned long arg;
};

static rt_timer timer_list;
static spinlock_t timer_list_lock = SPIN_LOCK_UNLOCKED;

/* stuff that we replace in Linux */
static unsigned long saved_do_IRQ;
static unsigned long saved_timer_interrupt_intercept;
static int (*ppc_md_get_irq)(struct pt_regs *);

#define timer_interrupt ((int (*)(struct pt_regs *))saved_timer_interrupt_intercept)
#define do_IRQ ((int (*)(struct pt_regs *))saved_do_IRQ)

/* function prototypes */

/* pending irq list control */
static void __pirq_list_remove(rt_irq_state *pirq);
static void __pirq_list_insert_before(rt_irq_state *pirq,rt_irq_state *list);
#ifdef unused
static void pirq_list_remove(rt_irq_state *pirq);
static void pirq_list_insert_before(rt_irq_state *pirq,rt_irq_state *list);
#endif

/* pending irq stuff */
static void deliver_pending_irq(rt_irq_state *pirq);
static int run_one_pending_irq(void);
static int run_pending_irqs(void);
static void __pend_irq(rt_irq_state *pirq);
static void pend_irq(int irq);
static void pend_timer(void);
static void pend_srq(int srq);

/* IE emulation */
static void linux_soft_sti(void);
#ifdef unused
static void linux_cli(void);
static void linux_sti(void);
static void linux_save_flags(unsigned long *flags);
static void linux_restore_flags(unsigned long flags);
#endif
unsigned int linux_save_flags_and_cli(void);
void rtai_just_copy_back(unsigned long flags, int cpuid);

/* interrupt entry points */
static int rtai_do_IRQ(struct pt_regs *regs);
static int rtai_timer_interrupt(struct pt_regs *regs);

/* timer list control */
static void __timer_list_remove(rt_timer *timer);
static void __timer_list_insert_before(rt_timer *timer,rt_timer *list);
static void timer_list_remove(rt_timer *timer);
#ifdef unused
static void timer_list_insert_before(rt_timer *timer,rt_timer *list);
#endif
static void timer_list_add(rt_timer *timer,rt_timer *list);

/* timer stuff */
void timer_set_expiry(unsigned long expires);
static int run_one_timer(void);
static void run_timers(void);
static void linux_timer_init(void);
static void linux_timer_handler(rt_timer *timer);

/* mount, unmount */
static void __rt_mount_rtai(void);
static void __rt_umount_rtai(void);

#ifdef DEBUG_MOUNT
static void install_test_patch(void);
static void uninstall_test_patch(void);
#endif


/* pending irq list control */

static void __pirq_list_remove(rt_irq_state *pirq)
{
	pirq->pprev->pnext = pirq->pnext;
	pirq->pnext->pprev = pirq->pprev;
	pirq->pnext = NULL;
	pirq->pprev = NULL;
}

static void __pirq_list_insert_before(rt_irq_state *pirq,rt_irq_state *list)
{
	pirq->pprev = list->pprev;
	pirq->pnext = list;
	list->pprev = pirq;
	pirq->pprev->pnext = pirq;
}

#ifdef unused
static void pirq_list_remove(rt_irq_state *pirq)
{
	unsigned long flags;
	flags = rt_spin_lock_irqsave(&pending_irq_list_lock);
	__pirq_list_remove(pirq);
	rt_spin_unlock_irqrestore(flags,&pending_irq_list_lock);
}
#endif

#ifdef unused
static void pirq_list_insert_before(rt_irq_state *pirq,rt_irq_state *list)
{
	unsigned long flags;
	flags = rt_spin_lock_irqsave(&pending_irq_list_lock);
	__pirq_list_insert_before(pirq,list);
	rt_spin_unlock_irqrestore(flags,&pending_irq_list_lock);
}
#endif


/* do stuff with pending irqs */

static void deliver_pending_irq(rt_irq_state *pirq)
{
	int irq = pirq->irq;
	int cpu = smp_processor_id();
	int saved_gp;
	unsigned long flags;

	saved_gp = global_priority;
	global_priority = 1;
	hardirq_enter(cpu);
	switch(pirq->type){
	case IRQ_TYPE_IRQ:
		debug_check_flags(0);
		ppc_irq_dispatch_handler(&rtai_regs,irq);
		debug_check_flags(0);
		flags = rt_spin_lock_irqsave(&interrupt_controller_lock);
		if(pirq->handler->end){
			pirq->handler->end(irq);
		}else if(pirq->handler->enable){
			pirq->handler->enable(irq);
		}
		rt_spin_unlock_irqrestore(flags,&interrupt_controller_lock);
		break;
	case IRQ_TYPE_TIMER:
		timer_interrupt(&rtai_regs);
		break;
	case IRQ_TYPE_SRQ:
		pirq->action(irq,(void *)pirq->arg,&rtai_regs);
		break;
	}
	hardirq_exit(cpu);

	pirq->status = PIRQ_IDLE;
	global_priority = saved_gp;
}

static int run_one_pending_irq(void)
{
	unsigned long flags;
	rt_irq_state *pirq;

	flags = rt_spin_lock_irqsave(&pending_irq_list_lock);
	pirq = pending_irq_list.pnext;
	if(pirq != &pending_irq_list){
		__pirq_list_remove(pirq);
		pirq->status = PIRQ_RUNNING;
		rt_spin_unlock_irqrestore(flags, &pending_irq_list_lock);
		deliver_pending_irq(pirq);
		return 1;
	}else{
		rt_spin_unlock_irqrestore(flags, &pending_irq_list_lock);
		return 0;
	}
}

static int run_pending_irqs(void)
{
	int cpu = smp_processor_id();
	int n = 0;

	hard_sti();

	while(run_one_pending_irq())n=1;

	if(n){
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,10) /* ? */
		if(softirq_pending(cpu)){
			do_softirq();
		}
#else
		/* XXX do something here */
#endif
	}

	return n;
}

static void __pend_irq(rt_irq_state *pirq)
{
	unsigned long flags;

	flags = rt_spin_lock_irqsave(&pending_irq_list_lock);

	if(pirq->pnext){
		//RT_BUG();
		rt_printk("rtai: pend_irq(): irq %d already pending\n");
		return;
	}

	__pirq_list_insert_before(pirq,&pending_irq_list);
	pirq->status = PIRQ_PENDING;
	rt_spin_unlock_irqrestore(flags, &pending_irq_list_lock);
}

static void __unpend_irq(rt_irq_state *pirq)
{
	unsigned long flags;

	flags = rt_spin_lock_irqsave(&pending_irq_list_lock);

	if(!pirq->pnext){
		//RT_BUG();
		rt_printk("rtai: unpend_irq(): irq %d not pending\n");
		return;
	}

	__pirq_list_insert_before(pirq,&pending_irq_list);
	pirq->status = PIRQ_PENDING;
	rt_spin_unlock_irqrestore(flags, &pending_irq_list_lock);
}

static void pend_irq(int irq)
{
	__pend_irq(rt_irq_desc+irq);
}

static void pend_timer(void)
{
	__pend_irq(&rt_timer_desc);
}

static void pend_srq(int srq)
{
	__pend_irq(rt_srq_desc+srq);
}


/* emulation of the interrupt enable bit */

static void linux_soft_sti(void)
{
	debug_hard_flags(0,MSR_EE);

	global_priority = 0;
}

#ifdef unused
static void linux_cli(void)
{ 
	global_priority = 1;
}

static void linux_sti(void)
{
	global_priority = 0;
	run_pending_irqs();
	/* we should run checks that are done in the ret_to_user */
}

static void linux_save_flags(unsigned long *flags)
{
	*flags = global_priority;
}

static void linux_restore_flags(unsigned long flags)
{
	global_priority = flags;
	if(flags == 0)run_pending_irqs();
	/* we should run checks that are done in the ret_to_user */
}
#endif

unsigned int linux_save_flags_and_cli(void)
{
	unsigned int ret;

	ret = xchg_u32(&global_priority, 1);

	return ret;
}

void rtai_just_copy_back(unsigned long flags, int cpuid)
{
        global_priority = flags;
}


/* bogus PIC functions */

static int trpd_get_irq(struct pt_regs *regs)
{
	RT_BUG();
	return 0;
}

/* Here are the trapped irq actions for Linux interrupt handlers. */

static unsigned int trpd_startup_irq(unsigned int irq)
{
	unsigned long flags;
	unsigned int ret;
	flags = rt_spin_lock_irqsave(&interrupt_controller_lock);
	if(rt_irq_desc[irq].handler->startup)
		ret = rt_irq_desc[irq].handler->startup(irq);
	rt_spin_unlock_irqrestore(flags,&interrupt_controller_lock);
	debug_check_flags(0);
	return ret;
}

static void trpd_shutdown_irq(unsigned int irq)
{
	unsigned long flags;
	flags = rt_spin_lock_irqsave(&interrupt_controller_lock);
	if(rt_irq_desc[irq].handler->shutdown)
		rt_irq_desc[irq].handler->shutdown(irq);
	rt_spin_unlock_irqrestore(flags,&interrupt_controller_lock);
	debug_check_flags(0);
}

static void trpd_enable_irq(unsigned int irq)
{
	unsigned long flags;
	flags = rt_spin_lock_irqsave(&interrupt_controller_lock);
	if(rt_irq_desc[irq].handler->enable)
		rt_irq_desc[irq].handler->enable(irq);
	rt_spin_unlock_irqrestore(flags,&interrupt_controller_lock);
	debug_check_flags(0);
}

static void trpd_disable_irq(unsigned int irq)
{
	unsigned long flags;
	flags = rt_spin_lock_irqsave(&interrupt_controller_lock);
	if(rt_irq_desc[irq].handler->disable)
		rt_irq_desc[irq].handler->disable(irq);
	rt_spin_unlock_irqrestore(flags,&interrupt_controller_lock);
	debug_check_flags(0);
}

static void trpd_ack_irq(unsigned int irq)
{
	/* we handle this ourselves */
#if 0
	if(rt_irq_desc[irq].handler->ack)
		rt_irq_desc[irq].handler->ack(irq);
#endif
	debug_check_flags(0);
}

static void trpd_end_irq(unsigned int irq)
{
	/* we handle this ourselves */
#if 0
	if(rt_irq_desc[irq].handler->end)
		rt_irq_desc[irq].handler->end(irq);
#endif
	debug_check_flags(0);
}

static struct hw_interrupt_type trapped_irq_type = { 
	typename:	"trapped",
	startup:	trpd_startup_irq,
	shutdown:	trpd_shutdown_irq,
	enable:		trpd_enable_irq,
	disable:	trpd_disable_irq,
	ack:		trpd_ack_irq,
	end:		trpd_end_irq,
	set_affinity:	NULL,
};

#ifdef unused
static void do_nothing_picfun(unsigned int irq)
{
}
#endif

#ifdef unused
static struct hw_interrupt_type real_time_irq_type = { 
	typename:	"real-time",
	startup:	(unsigned int (*)(unsigned int))do_nothing_picfun,
	shutdown:	do_nothing_picfun,
	enable:		do_nothing_picfun,
	disable:	do_nothing_picfun,
	ack:		do_nothing_picfun,
	end:		do_nothing_picfun,
	set_affinity:	NULL,
};
#endif


/* Our interrupt handler */

static int rtai_do_IRQ(struct pt_regs *regs)
{
	int irq;
	int do_checks = 0;
	static int reenter = 0;

	debug_fix_flags();
	if(reenter){
		//RT_BUG();
		//debug_ptr(0x11111111,0);
	}
	reenter=1;

	irq = ppc_md_get_irq(regs);
	//debug_ptr(regs->nip,irq);
	debug_ptr(0,irq);

	TRACE_RTAI_GLOBAL_IRQ_ENTRY(irq,0);
	
	if(irq<0){
		/* ignore spurious interrupts */
		//printk("Bogus interrupt %d\n",irq);
		reenter=0;
		return 0;
	}

	rt_spin_lock(&interrupt_controller_lock);
	if(rt_irq_desc[irq].handler->ack)
		rt_irq_desc[irq].handler->ack(irq);
	rt_spin_unlock(&interrupt_controller_lock);

	pend_irq(irq);
	if(global_priority<1){
		do_checks = run_pending_irqs();
	}else{
		debug_ptr(0x22222222,0);
	}

	TRACE_RTAI_GLOBAL_IRQ_EXIT();

	reenter=0;
	return do_checks;
}

static int rtai_timer_interrupt(struct pt_regs *regs)
{
	int ret;
	unsigned long flags;
	int do_checks = 0;

	debug_fix_flags();
	if(atomic_read(&ppc_n_lost_interrupts)!=0){
		/* Grrr... this doesn't seem to be triggered anymore */
		//printk("replacing lost interrupt\n");
		ret = rtai_do_IRQ(regs);
		
		flags = rt_spin_lock_irqsave(&timer_list_lock);
		timer_set_expiry(timer_list.pnext->expires);
		rt_spin_unlock_irqrestore(flags,&timer_list_lock);

		return ret;
	}
	//debug_ptr(regs->nip,0);
	debug_ptr(0,0);
	run_timers();

	if(global_priority<1){
		do_checks = run_pending_irqs();
	}else{
		debug_ptr(0x22222222,0);
	}

	return do_checks;
}

/* timer list control */

static void __timer_list_remove(rt_timer *timer)
{
	timer->pprev->pnext = timer->pnext;
	timer->pnext->pprev = timer->pprev;
	timer->pnext = NULL;
	timer->pprev = NULL;
}

static void __timer_list_insert_before(rt_timer *timer,rt_timer *list)
{
	timer->pprev = list->pprev;
	timer->pnext = list;
	list->pprev = timer;
	timer->pprev->pnext = timer;
}

static void timer_list_remove(rt_timer *timer)
{
	unsigned long flags;
	flags = rt_spin_lock_irqsave(&timer_list_lock);
	if(timer->pprev==&timer_list){
		timer_set_expiry(timer->expires);
	}
	__timer_list_remove(timer);
	rt_spin_unlock_irqrestore(flags,&timer_list_lock);
}

#ifdef unused
static void timer_list_insert_before(rt_timer *timer,rt_timer *list)
{
	unsigned long flags;
	flags = rt_spin_lock_irqsave(&timer_list_lock);
	__timer_list_insert_before(timer,list);
	rt_spin_unlock_irqrestore(flags,&timer_list_lock);
}
#endif

static void timer_list_add(rt_timer *timer,rt_timer *list)
{
	unsigned long flags;
	rt_timer *t;
	flags = rt_spin_lock_irqsave(&timer_list_lock);
	t = list->pnext;
	while(t!=list && (long)(t->expires-timer->expires)<=0){
		t=t->pnext;
	}
	__timer_list_insert_before(timer,t);
	if(timer->pprev==list){
		timer_set_expiry(timer->expires);
	}
	rt_spin_unlock_irqrestore(flags,&timer_list_lock);
}

/* decrementer set function */

void timer_set_expiry(unsigned long expires)
{
	long diff = expires - get_tbl();

	if(diff<1)diff=1;
	if(diff>tb_ticks_per_jiffy){
		printk("warning: setting decr to %ld\n",diff);
	}

	set_dec(diff);
}

/* timer dispatch routines */

static int run_one_timer(void)
{
	unsigned long flags;
	unsigned long time;
	rt_timer *timer;

	flags = rt_spin_lock_irqsave(&timer_list_lock);
	timer = timer_list.pnext;
	if(timer==&timer_list){
		RT_BUG();
	}
	time = get_tbl();
	if((long)(timer->expires-time)<=0){
		__timer_list_remove(timer);
		rt_spin_unlock_irqrestore(flags, &timer_list_lock);
		timer->handler(timer);
		return 1;
	}else{
		timer_set_expiry(timer->expires);
		rt_spin_unlock_irqrestore(flags, &timer_list_lock);
		return 0;
	}
}

static void run_timers(void)
{
	while(run_one_timer());
}

/* Linux 100 HZ tick emulation */

static rt_timer linux_timer;

static void linux_timer_init(void)
{
	linux_timer.handler = linux_timer_handler;
	linux_timer.expires = get_tbl() + tb_ticks_per_jiffy;

	timer_list_add(&linux_timer,&timer_list);
}

static void linux_timer_handler(rt_timer *timer)
{
	pend_timer();

	linux_timer.expires += tb_ticks_per_jiffy;
	timer_list_add(&linux_timer,&timer_list);
}


/*
 * This is the binary patch we're applying to the Linux kernel.
 *
00000000 <patch>:
   0:	3c 00 c0 01 	lis	r0,-16383
   4:	60 00 da fe 	ori	r0,r0,56062
   8:	7c 09 03 a6 	mtctr	r0
   c:	4e 80 04 20 	bctr
*/

static u32 patch_insns[]={
	0x3c000000, 0x60000000, 0x7c0903a6, 0x4e800420,
};
#define OFFSET_HI 0
#define OFFSET_LO 1
#define N_INSNS 4

static u32 saved_insns_cli[N_INSNS];
static u32 saved_insns_sti[N_INSNS];
static u32 saved_insns_save_flags[N_INSNS];
static u32 saved_insns_restore_flags[N_INSNS];


static void patch_function(void *dest,void *func,void *save)
{
	memcpy(save,dest,sizeof(patch_insns));
	memcpy(dest,patch_insns,sizeof(patch_insns));
	((u32 *)dest)[OFFSET_HI]|=(((unsigned int)(func))>>16)&0xffff;
	((u32 *)dest)[OFFSET_LO]|=((unsigned int)(func))&0xffff;
	flush_icache_range((int)dest,(int)dest+sizeof(patch_insns));
}

static void unpatch_function(void *dest,void *save)
{
	memcpy(dest,save,sizeof(patch_insns));
	flush_icache_range((int)dest,(int)dest+sizeof(patch_insns));
}

#ifdef unused
static void install_patch(void)
{
	patch_function(&__cli,&linux_cli,saved_insns_cli);
	patch_function(&__sti,&linux_sti,saved_insns_sti);
	patch_function(&__save_flags_ptr,&linux_save_flags,saved_insns_save_flags);
	patch_function(&__restore_flags,&linux_restore_flags,saved_insns_restore_flags);
}
#endif

#ifdef unused
static void uninstall_patch(void)
{
	unpatch_function(&__cli,saved_insns_cli);
	unpatch_function(&__sti,saved_insns_sti);
	unpatch_function(&__save_flags_ptr,saved_insns_save_flags);
	unpatch_function(&__restore_flags,saved_insns_restore_flags);
}
#endif


static void __rt_mount_rtai(void)
{
	int i;

	printk("rtai: mounting\n");

	pending_irq_list.pnext = &pending_irq_list;
	pending_irq_list.pprev = &pending_irq_list;
	timer_list.pnext = &timer_list;
	timer_list.pprev = &timer_list;

	hard_cli();

	TRACE_RTAI_MOUNT();

	install_test_patch();
	rtai_soft_sti = linux_soft_sti;

	saved_do_IRQ = do_IRQ_intercept;
	do_IRQ_intercept = (unsigned long)rtai_do_IRQ;

	saved_timer_interrupt_intercept = timer_interrupt_intercept;
	timer_interrupt_intercept = (unsigned long)rtai_timer_interrupt;

	ppc_md_get_irq = ppc_md.get_irq;
	ppc_md.get_irq = trpd_get_irq;

	linux_timer_init();

	for(i=0;i<NR_CPUS;i++){
		disarm_decr[i]=1;
	}

	for(i=0;i<NR_IRQS;i++){
		rt_irq_desc[i].irq = i;
		rt_irq_desc[i].priority = 1;
		rt_irq_desc[i].handler = irq_desc[i].handler;
		irq_desc[i].handler = &trapped_irq_type;
	}

	{
#define MSR(x) ((struct pt_regs *)((x)->thread.ksp + STACK_FRAME_OVERHEAD))->msr
	struct task_struct *task;
	task = &init_task;
	MSR(task) |= MSR_EE;
	if (task->thread.regs) {
		(task->thread.regs)->msr |= MSR_EE;
	}
	for_each_task(task) {
		MSR(task) |= MSR_EE;
		if (task->thread.regs) {
			(task->thread.regs)->msr |= MSR_EE;
		}
	}
	}

	hard_sti();

	printk("rtai: mount done\n");
}

static void __rt_umount_rtai(void)
{
	int i;

	printk("rtai: unmounting\n");
	hard_cli();

	TRACE_RTAI_UMOUNT();

	uninstall_test_patch();
	rtai_soft_sti = 0;

	do_IRQ_intercept = saved_do_IRQ;
	timer_interrupt_intercept = saved_timer_interrupt_intercept;
	ppc_md.get_irq = ppc_md_get_irq;

	for(i=0;i<NR_CPUS;i++){
		disarm_decr[i]=0;
	}
	set_dec(1);

	for(i=0;i<NR_IRQS;i++){
		irq_desc[i].handler = rt_irq_desc[i].handler;
	}

	hard_sti();
	printk("rtai: unmount done\n");
}


void rt_mount_rtai(void) { }
void rt_umount_rtai(void) { }


int init_module(void)
{
printk("&init_module=%p\n",init_module);
	rt_printk_init();

	__rt_mount_rtai();

#ifdef CONFIG_PROC_FS
	rtai_proc_register();
#endif

	return 0;
}

void cleanup_module(void)
{
	__rt_umount_rtai();

#ifdef CONFIG_PROC_FS
	rtai_proc_unregister();
#endif
	rt_printk_cleanup();
}


#ifdef DEBUG_MOUNT
void debug_priority(void)
{
	unsigned long flags;

	__asm__(
		"mfmsr 0\n\t"
		: "=r" (flags)
	);

	if(global_priority!=((flags&MSR_EE)?0:1)){
		debug_ptr(0x44444444,global_priority);
	}
	debug_check_flags(0);
}

void test_cli(void)
{
	debug_priority();
	global_priority=1;
#ifdef HARD
	__asm__(
		"mfmsr 0\n\t"
		"rlwinm 3,0,17,31,31\n\t"
		"rlwinm 0,0,0,17,15\n\t"
		"mtmsr 0\n\t"
		: : : "r0"
	);
#endif
	TRACE_RTAI_TRAP_ENTRY(global_priority,0x11111111);
}

void test_sti(void)
{
	debug_priority();
	global_priority=0;
#ifdef HARD
	__asm__(
		"mfmsr 3\n\t"
		"ori 3,3,(1<<15)\n\t"
		"mtmsr 3\n\t"
		: : : "r3"
	);
#else
	run_pending_irqs();
#endif
	TRACE_RTAI_TRAP_ENTRY(global_priority,0x22222222);
}

void test_restore_flags(unsigned long flags)
{
	debug_priority();
	if(flags&MSR_EE){
		global_priority=0;
#ifndef HARD
		run_pending_irqs();
#endif
	}else{
		global_priority=1;
	}
#ifdef HARD
	__asm__(
		"mfmsr 4\n\t"
		"rlwimi %0,4,0,17,15\n\t"
		"mtmsr %0\n\t"
		: : "r" (flags)
	);
#endif
	TRACE_RTAI_TRAP_ENTRY(global_priority,0x33333333);
}

void test_save_flags_ptr(unsigned long *flags)
{
	debug_priority();
#ifdef HARD
	__asm__(
		"mfmsr %0\n\t"
		: "=r" (*flags)
	);
#else
	*flags = global_priority?0:MSR_EE;
#endif
	TRACE_RTAI_TRAP_ENTRY(global_priority,0x44444444);
}

static void install_test_patch(void)
{
	patch_function(&__cli,&test_cli,saved_insns_cli);
	patch_function(&__sti,&test_sti,saved_insns_sti);
	patch_function(&__save_flags_ptr,&test_save_flags_ptr,saved_insns_save_flags);
	patch_function(&__restore_flags,&test_restore_flags,saved_insns_restore_flags);
}

static void uninstall_test_patch(void)
{
	unpatch_function(&__cli,saved_insns_cli);
	unpatch_function(&__sti,saved_insns_sti);
	unpatch_function(&__save_flags_ptr,saved_insns_save_flags);
	unpatch_function(&__restore_flags,saved_insns_restore_flags);
}
#endif



static rt_timer rtai_timer;

static struct rt_times rt_times;

static void periodic_timer_handler(rt_timer *timer)
{
	((void (*)(void))timer->arg)();

	rtai_timer.expires += rt_times.periodic_tick;
	if(rtai_timer.expires < get_tbl()){
		/* overrun.  What to do? */
	}
	timer_list_add(&rtai_timer,&timer_list);
}

void rt_request_timer(void (*handler)(void), unsigned int tick, int unused)
{
	RTIME t;

	TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_REQUEST, handler, tick);

	t = rdtsc();

	rt_times.linux_tick = tb_ticks_per_jiffy;
	rt_times.periodic_tick = tick;
	rt_times.tick_time = t;
	rt_times.intr_time = t+tick;
	rt_times.linux_time = t+tb_ticks_per_jiffy;

	rtai_timer.arg = (unsigned long)handler;
	rtai_timer.handler = periodic_timer_handler;
	rtai_timer.expires = get_tbl() + tick;

	timer_list_add(&rtai_timer,&timer_list);
}

void rt_free_timer(void)
{
	TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_FREE, 0, 0);

	timer_list_remove(&rtai_timer);
}

/* This function sucks */
void rt_timer_add(rt_timer *timer)
{
	timer_list_add(timer,&timer_list);
}

/* This function sucks */
void rt_timer_del(rt_timer *timer)
{
	timer_list_remove(timer);
}

#ifdef unimplemented
void rt_request_apic_timers(void (*handler)(void), struct apic_timer_setup_data
	*apic_timer_data)
{
	/* It's unclear what to do with this function on PowerPC. */
}
#endif

#ifdef unimplemented
void rt_free_apic_timers(void)
{
}
#endif

/* System request stuff */

void rt_pend_linux_srq(unsigned int srq)
{
	if(rt_srq_desc[srq].dev_id == NULL){
		rt_printk("rt_pend_linux_srq() called with bogus srq %d\n",srq);
		//RT_BUG();
	}else{
		/* small race here */
		if(!rt_srq_desc[srq].pnext){
			pend_srq(srq);
		}
	}
}

int rt_free_srq(unsigned int srq)
{
	if(!rt_srq_desc[srq].pnext){
		/* small race here */
		__unpend_irq(rt_srq_desc + srq);
	}
	rt_srq_desc[srq].dev_id = NULL;

	return 0;
}

int rt_request_srq(unsigned int label, void (*handler)(void), long long
	(*user_handler)(unsigned int arg))
{
	int i;

	for(i=0;i<NR_SRQS;i++){
		/* XXX we shouldn't overload the dev_id as an "in-use"
		 * flag */
		if(rt_srq_desc[i].dev_id == NULL){
			rt_srq_desc[i].dev_id = (void *)1;
			//rt_srq_desc[i].label = label;
			rt_srq_desc[i].action = (void *)handler;
			/* user_handler is ignored */
			return i;
		}
	}

	return -ENOSYS;
}

/* Scheduler stuff */

void rt_switch_to_linux(int cpuid)
{
	rt_printk("rt_switch_to_linux unimplmented\n");
}

int rt_free_linux_irq(unsigned int irq, void *dev_id)
{
	rt_printk("rt_free_linux_irq unimplmented\n");
	return -ENOSYS;
}

void rt_switch_to_real_time(int cpuid)
{
	rt_printk("rt_switch_to_real_time unimplmented\n");
}

struct calibration_data tuned;

void rt_pend_linux_irq(unsigned int irq)
{
	pend_irq(irq);
}

int rt_request_linux_irq(unsigned int irq,
	void (*linux_handler)(int irq, void *dev_id, struct pt_regs *regs),
	char *linux_handler_id, void *dev_id)
{
	rt_printk("rt_request_linux_irq unimplmented\n");
	return -ENOSYS;
}

/* existing interface */
EXPORT_SYMBOL(rt_request_timer);
EXPORT_SYMBOL(rt_free_timer);

EXPORT_SYMBOL(rt_pend_linux_srq);
EXPORT_SYMBOL(rt_request_srq);
EXPORT_SYMBOL(rt_free_srq);

EXPORT_SYMBOL(rt_mount_rtai);
EXPORT_SYMBOL(rt_umount_rtai);

/* in lib.c */
EXPORT_SYMBOL(ullmul);
EXPORT_SYMBOL(ulldiv);
EXPORT_SYMBOL(imuldiv);
EXPORT_SYMBOL(llimd);

/* in rt_printk.c */
EXPORT_SYMBOL(rtai_print_to_screen);
EXPORT_SYMBOL(rt_printk);

/* in proc.c */
EXPORT_SYMBOL(rtai_proc_root);

/* new stuff */
EXPORT_SYMBOL(rt_timer_add);
EXPORT_SYMBOL(rt_timer_del);

/* unimplmented functions */
EXPORT_SYMBOL(rt_free_linux_irq);
EXPORT_SYMBOL(rt_pend_linux_irq);
EXPORT_SYMBOL(rt_request_linux_irq);
EXPORT_SYMBOL(rt_switch_to_linux);
EXPORT_SYMBOL(rt_switch_to_real_time);
EXPORT_SYMBOL(rt_times);
EXPORT_SYMBOL(tuned);

#ifdef unimplemented
EXPORT_SYMBOL(rt_request_apic_timers);
EXPORT_SYMBOL(rt_free_apic_timers);
EXPORT_SYMBOL(rt_enable_irq);
EXPORT_SYMBOL(rt_free_global_irq);
EXPORT_SYMBOL(rt_request_global_irq);
EXPORT_SYMBOL(rt_reset_full_intr_vect);
EXPORT_SYMBOL(rt_set_full_intr_vect);
EXPORT_SYMBOL(rtai_just_copy_back);
//EXPORT_SYMBOL(rt_typed_named_mbx_init);
//EXPORT_SYMBOL(rt_typed_named_sem_init);

/* smp stuff */
EXPORT_SYMBOL(linux_save_flags_and_cli_cpuid);
EXPORT_SYMBOL(locked_cpus);
EXPORT_SYMBOL(rt_smp_times);
EXPORT_SYMBOL(init_xfpu);
#endif

/* tracepoints we haven't added: */
#ifdef unused
TRACE_RTAI_SWITCHTO_LINUX(cpuid);
TRACE_RTAI_SWITCHTO_RT(cpuid);
TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_REQUEST_APIC, handler, 0);
TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_APIC_FREE, 0, 0);
TRACE_RTAI_OWN_IRQ_ENTRY(irq);
TRACE_RTAI_OWN_IRQ_EXIT();
TRACE_RTAI_TRAP_ENTRY(err);
TRACE_RTAI_TRAP_EXIT();
TRACE_RTAI_SRQ_ENTRY(srq);
TRACE_RTAI_SRQ_EXIT();
#endif

