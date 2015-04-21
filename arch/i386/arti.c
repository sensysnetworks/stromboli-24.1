/*
 *   ARTI -- RTAI-compatible Adeos-based Real-Time Interface. Based on
 *   the original RTAI layer for x86.
 *
 *   Original RTAI/x86 layer implementation:
 *   Copyright (C) 2000 Paolo Mantegazza,
 *   Copyright (C) 2000 Steve Papacharalambous,
 *   Copyright (C) 2000 Stuart Hughes,
 *   and others.
 *
 *   RTAI/x86 rewrite over Adeos:
 *   Copyright (C) 2002 Philippe Gerum.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 675 Mass Ave, Cambridge MA 02139,
 *   USA; either version 2 of the License, or (at your option) any later
 *   version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <linux/version.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/wrapper.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/console.h>
#include <asm/system.h>
#include <asm/hw_irq.h>
#include <asm/irq.h>
#include <asm/desc.h>
#include <asm/io.h>
#include <asm/mmu_context.h>
#ifdef CONFIG_X86_LOCAL_APIC
#include <asm/fixmap.h>
#include <asm/bitops.h>
#include <asm/mpspec.h>
#ifdef CONFIG_X86_IO_APIC
#include <asm/io_apic.h>
#endif /* CONFIG_X86_IO_APIC */
#include <asm/apic.h>
#endif /* CONFIG_X86_LOCAL_APIC */
#define __ARTI__
#include <rtai.h>
#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#endif /* CONFIG_PROC_FS */
#include <stdarg.h>

MODULE_LICENSE("GPL");

static int arti_cpufreq_arg = ARTI_CALIBRATED_CPU_FREQ;
MODULE_PARM(arti_cpufreq_arg,"i");

#ifdef CONFIG_X86_LOCAL_APIC
static int arti_apicfreq_arg = ARTI_CALIBRATED_APIC_FREQ;
MODULE_PARM(arti_apicfreq_arg,"i");
#endif /* CONFIG_X86_LOCAL_APIC */

#ifdef CONFIG_SMP
static unsigned long arti_old_irq_affinity[NR_IRQS],
                     arti_set_irq_affinity[NR_IRQS];

static spinlock_t arti_iset_lock = SPIN_LOCK_UNLOCKED;
#endif /* CONFIG_SMP */

extern struct desc_struct idt_table[];

extern void show_stack(unsigned long *esp);

extern void show_registers(struct pt_regs *regs);

adomain_t arti_domain;

static struct {

    void (*handler)(unsigned irq, void *cookie);
    void *cookie;

} arti_realtime_irq[NR_IRQS];

static struct {

    unsigned long flags;
    int count;

} arti_linux_irq[NR_IRQS];

static struct {

    void (*k_handler)(void);
    long long (*u_handler)(unsigned);
    unsigned label;

} arti_sysreq_table[ARTI_NR_SRQS];

static unsigned arti_sysreq_virq;

static unsigned long arti_sysreq_map = 3; /* srqs #[0-1] are reserved */

static unsigned long arti_sysreq_pending;

static unsigned long arti_sysreq_running;

static spinlock_t arti_ssrq_lock = SPIN_LOCK_UNLOCKED;

static volatile int arti_sync_level;

static atomic_t arti_sync_count = ATOMIC_INIT(1);

static int arti_last_8254_counter2;

static RTIME arti_ts_8254;

static struct desc_struct arti_sysvec;

static RT_TRAP_HANDLER arti_trap_handler;

static int arti_mount_count;

struct rt_times rt_times = { 0 };

struct rt_times rt_smp_times[ARTI_NR_CPUS] = { { 0 } };

struct arti_switch_data arti_linux_context[ARTI_NR_CPUS] = { { 0 } };

struct calibration_data arti_tunables = { 0 };

volatile unsigned long arti_status = 0;

volatile unsigned long arti_cpu_realtime = 0;

volatile unsigned long arti_cpu_lock = 0;

volatile unsigned long arti_cpu_lxrt = 0;

int arti_adeos_ptdbase = -1;

int (*arti_signal_handler)(struct task_struct *task,
			   int sig);

static inline unsigned long arti_critical_enter (void (*synch)(void)) {

    unsigned long flags = adeos_critical_enter(synch);

    if (atomic_dec_and_test(&arti_sync_count))
	arti_sync_level = 0;
    else if (synch != NULL)
	printk("RTAI/Adeos: warning: nested sync will fail.\n");

    return flags;
}

static inline void arti_critical_exit (unsigned long flags) {

    atomic_inc(&arti_sync_count);
    adeos_critical_exit(flags);
}

/* Note: On Linux boxen running Adeos, adp_root == &linux_domain */

void arti_linux_cli (void) {
    adeos_stall_pipeline_from(adp_root);
}

void arti_linux_sti (void) {
    adeos_unstall_pipeline_from(adp_root);
}

unsigned arti_linux_save_flags (void) {
    return (unsigned)adeos_test_pipeline_from(adp_root);
}

void arti_linux_restore_flags (unsigned flags) {
    adeos_restore_pipeline_from(adp_root,flags);
}

unsigned arti_linux_save_flags_and_cli (void) {
    return (unsigned)adeos_test_and_stall_pipeline_from(adp_root);
}

unsigned long arti_linux_save_flags_and_cli_cpuid (int cpuid) {
    return adeos_test_and_stall_pipeline_remote(adp_root,cpuid);
}

void arti_linux_restore_flags_cpuid (unsigned long flags, int cpuid) {
    adeos_restore_pipeline_remote(adp_root,flags,cpuid);
}

int rt_request_irq (unsigned irq,
		    void (*handler)(unsigned irq, void *cookie),
		    void *cookie)
{
    unsigned long flags;

    if (handler == NULL || irq >= NR_IRQS)
	return -EINVAL;

    flags = arti_critical_enter(NULL);

    if (arti_realtime_irq[irq].handler != NULL)
	{
	arti_critical_exit(flags);
	return -EBUSY;
	}

    arti_realtime_irq[irq].handler = handler;
    arti_realtime_irq[irq].cookie = cookie;

    arti_critical_exit(flags);

    return 0;
}

int rt_release_irq (unsigned irq)

{
    if (irq >= NR_IRQS)
	return -EINVAL;

    xchg(&arti_realtime_irq[irq].handler,NULL);

    return 0;
}

void rt_set_irq_cookie (unsigned irq, void *cookie) {

    if (irq < NR_IRQS)
	arti_realtime_irq[irq].cookie = cookie;
}

/* Note: Adeos already does all the magic that allows calling the
   interrupt controller routines safely. */

unsigned rt_startup_irq (unsigned irq) {

    return irq_desc[irq].handler->startup(irq);
}

void rt_shutdown_irq (unsigned irq) {

    irq_desc[irq].handler->shutdown(irq);
}

void rt_enable_irq (unsigned irq) {

    irq_desc[irq].handler->enable(irq);
}

void rt_disable_irq (unsigned irq) {

    irq_desc[irq].handler->disable(irq);
}

void rt_mask_and_ack_irq (unsigned irq) {

    irq_desc[irq].handler->ack(irq);
}

void rt_unmask_irq (unsigned irq) {

    irq_desc[irq].handler->end(irq);
}

void rt_ack_irq (unsigned irq) {

    if (irq != ARTI_TIMER_8254_IRQ)
	irq_desc[irq].handler->end(irq);
}

void rt_do_irq (unsigned irq) {

    adeos_trigger_irq(irq);
}

int rt_request_linux_irq (unsigned irq,
			  void (*handler)(int irq,
					  void *dev_id,
					  struct pt_regs *regs), 
			  char *name,
			  void *dev_id)
{
    unsigned long flags;

    if (irq >= NR_IRQS || !handler)
	return -EINVAL;

    arti_local_irq_save(flags);

    spin_lock(&irq_desc[irq].lock);

    if (arti_linux_irq[irq].count++ == 0 && irq_desc[irq].action)
	{
	arti_linux_irq[irq].flags = irq_desc[irq].action->flags;
	irq_desc[irq].action->flags |= SA_SHIRQ;
	}

    spin_unlock(&irq_desc[irq].lock);

    arti_local_irq_restore(flags);

    request_irq(irq,handler,SA_SHIRQ,name,dev_id);

    return 0;
}

int rt_free_linux_irq (unsigned irq, void *dev_id)

{
    unsigned long flags;

    if (irq >= NR_IRQS || arti_linux_irq[irq].count == 0)
	return -EINVAL;

    arti_local_irq_save(flags);

    free_irq(irq,dev_id);

    spin_lock(&irq_desc[irq].lock);

    if (--arti_linux_irq[irq].count == 0 && irq_desc[irq].action)
	irq_desc[irq].action->flags = arti_linux_irq[irq].flags;

    spin_unlock(&irq_desc[irq].lock);

    arti_local_irq_restore(flags);

    return 0;
}

void rt_pend_linux_irq (unsigned irq) {

    adeos_propagate_irq(irq);
}

int rt_request_srq (unsigned label,
		    void (*k_handler)(void),
		    long long (*u_handler)(unsigned))
{
    unsigned long flags;
    int srq;

    if (k_handler == NULL)
	return -EINVAL;

    arti_local_irq_save(flags);

    if (arti_sysreq_map != ~0)
	{
	srq = ffz(arti_sysreq_map);
	set_bit(srq,&arti_sysreq_map);
	arti_sysreq_table[srq].k_handler = k_handler;
	arti_sysreq_table[srq].u_handler = u_handler;
	arti_sysreq_table[srq].label = label;
	}
    else
	srq = -EBUSY;

    arti_local_irq_restore(flags);

    return srq;
}

int rt_free_srq (unsigned srq)

{
    if (srq < 2 || srq >= ARTI_NR_SRQS ||
	!test_and_clear_bit(srq,&arti_sysreq_map))
	return -EINVAL;

    return 0;
}

void rt_pend_linux_srq (unsigned srq)

{
    if (srq > 1 && srq < ARTI_NR_SRQS)
	{
	set_bit(srq,&arti_sysreq_pending);
	adeos_schedule_irq(arti_sysreq_virq);
	}
}

#ifdef CONFIG_SMP

static long long arti_timers_sync_time;

static struct apic_timer_setup_data arti_timer_mode[ARTI_NR_CPUS];

void arti_broadcast_to_timers (int irq,
			       void *dev_id,
			       struct pt_regs *regs)
{
    unsigned long flags;

    arti_hw_lock(flags);
    apic_wait_icr_idle();
    apic_write_around(APIC_ICR,APIC_DM_FIXED|APIC_DEST_ALLINC|LOCAL_TIMER_VECTOR);
    arti_hw_unlock(flags);
} 

static inline void arti_setup_periodic_apic (unsigned count,
					     unsigned vector)
{
    apic_read(APIC_LVTT);
    apic_write(APIC_LVTT,APIC_LVT_TIMER_PERIODIC|vector);
    apic_read(APIC_TMICT);
    apic_write(APIC_TMICT,count);
}

static inline void arti_setup_oneshot_apic (unsigned count,
					    unsigned vector)
{
    apic_read(APIC_LVTT);
    apic_write(APIC_LVTT,vector);
    apic_read(APIC_TMICT);
    apic_write(APIC_TMICT,count);
}

static void arti_critical_sync (void)

{
    struct apic_timer_setup_data *p;

    switch (arti_sync_level)
	{
	case 1:

	    p = &arti_timer_mode[adeos_processor_id()];
	    
	    while (arti_rdtsc() < arti_timers_sync_time)
		;

	    if (p->mode)
		arti_setup_periodic_apic(p->count,ARTI_APIC_TIMER_VECTOR);
	    else
		arti_setup_oneshot_apic(p->count,ARTI_APIC_TIMER_VECTOR);

	    break;

	case 2:

	    arti_setup_oneshot_apic(0,ARTI_APIC_TIMER_VECTOR);
	    break;

	case 3:

	    arti_setup_periodic_apic(ARTI_APIC_ICOUNT,LOCAL_TIMER_VECTOR);
	    break;
	}
}

int rt_assign_irq_to_cpu (int irq, unsigned long cpumask)

{
    unsigned long oldmask, flags;

    arti_local_irq_save(flags);

    spin_lock(&arti_iset_lock);

    oldmask = adeos_set_irq_affinity(irq,cpumask);

    if (oldmask == 0)
	{
	/* Oops... Something went wrong. */
	spin_unlock(&arti_iset_lock);
	arti_local_irq_restore(flags);
	return -EINVAL;
	}

    arti_old_irq_affinity[irq] = oldmask;
    arti_set_irq_affinity[irq] = cpumask;

    spin_unlock(&arti_iset_lock);

    arti_local_irq_restore(flags);

    return 0;
}

int rt_reset_irq_to_sym_mode (int irq)

{
    unsigned long oldmask, flags;

    arti_local_irq_save(flags);

    spin_lock(&arti_iset_lock);

    if (arti_old_irq_affinity[irq] == 0)
	{
	spin_unlock(&arti_iset_lock);
	arti_local_irq_restore(flags);
	return -EINVAL;
	}

    oldmask = adeos_set_irq_affinity(irq,0); /* Query -- no change. */

    if (oldmask == arti_set_irq_affinity[irq])
	{
	/* Ok, proceed since nobody changed it in the meantime. */
	adeos_set_irq_affinity(irq,arti_old_irq_affinity[irq]);
	arti_old_irq_affinity[irq] = 0;
	}

    spin_unlock(&arti_iset_lock);

    arti_local_irq_restore(flags);

    return 0;
}

void rt_request_timer_cpuid (void (*handler)(void),
			     unsigned tick,
			     int cpuid)
{
    unsigned long flags;
    int count;

    set_bit(ARTI_USE_APIC,&arti_status);
    arti_timers_sync_time = 0;

    for (count = 0; count < ARTI_NR_CPUS; count++)
	arti_timer_mode[count].mode = arti_timer_mode[count].count = 0;

    flags = arti_critical_enter(arti_critical_sync);

    arti_sync_level = 1;

    rt_times.tick_time = arti_rdtsc();

    if (tick > 0)
	{
	rt_times.linux_tick = ARTI_APIC_ICOUNT;
	rt_times.tick_time = ((RTIME)rt_times.linux_tick)*(jiffies + 1);
	rt_times.intr_time = rt_times.tick_time + tick;
	rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.periodic_tick = tick;

	if (cpuid == adeos_processor_id())
	    arti_setup_periodic_apic(tick,ARTI_APIC_TIMER_VECTOR);
	else
	    {
	    arti_timer_mode[cpuid].mode = 1;
	    arti_timer_mode[cpuid].count = tick;
	    arti_setup_oneshot_apic(0,ARTI_APIC_TIMER_VECTOR);
	    }
	}
    else
	{
	rt_times.linux_tick = arti_imuldiv(LATCH, arti_tunables.cpu_freq,ARTI_FREQ_8254);
	rt_times.intr_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.periodic_tick = rt_times.linux_tick;

	if (cpuid == adeos_processor_id())
	    arti_setup_oneshot_apic(ARTI_APIC_ICOUNT,ARTI_APIC_TIMER_VECTOR);
	else
	    {
	    arti_timer_mode[cpuid].mode = 0;
	    arti_timer_mode[cpuid].count = ARTI_APIC_ICOUNT;
	    arti_setup_oneshot_apic(0,ARTI_APIC_TIMER_VECTOR);
	    }
	}

    rt_release_irq(ARTI_APIC_TIMER_IPI);

    arti_critical_exit(flags);

    rt_request_irq(ARTI_APIC_TIMER_IPI,(rt_irq_handler_t)handler,NULL);

    rt_request_linux_irq(ARTI_TIMER_8254_IRQ,
			 &arti_broadcast_to_timers,
			 "broadcast",
			 &arti_broadcast_to_timers);
}

void rt_request_apic_timers (void (*handler)(void),
			     struct apic_timer_setup_data *tmdata)
{
    volatile struct rt_times *rtimes;
    struct apic_timer_setup_data *p;
    unsigned long flags;
    int cpuid;

    TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_REQUEST_APIC,handler,0);

    flags = arti_critical_enter(arti_critical_sync);

    arti_sync_level = 1;

    arti_timers_sync_time = arti_rdtsc() + arti_imuldiv(LATCH,
							arti_tunables.cpu_freq,
							ARTI_FREQ_8254);
    for (cpuid = 0; cpuid < ARTI_NR_CPUS; cpuid++)
	{
	p = &arti_timer_mode[cpuid];
	*p = tmdata[cpuid];
	rtimes = &rt_smp_times[cpuid];

	if (p->mode)
	    {
	    rtimes->linux_tick = ARTI_APIC_ICOUNT;
	    rtimes->tick_time = arti_llimd(arti_timers_sync_time,
					   ARTI_FREQ_APIC,
					   arti_tunables.cpu_freq);
	    rtimes->periodic_tick = arti_imuldiv(p->count,
						 ARTI_FREQ_APIC,
						 1000000000);
	    p->count = rtimes->periodic_tick;
	    }
	else
	    {
	    rtimes->linux_tick = arti_imuldiv(LATCH,
					      arti_tunables.cpu_freq,
					      ARTI_FREQ_8254);
	    rtimes->tick_time = arti_timers_sync_time;
	    rtimes->periodic_tick = rtimes->linux_tick;
	    p->count = ARTI_APIC_ICOUNT;
	    }

	rtimes->intr_time = rtimes->tick_time + rtimes->periodic_tick;
	rtimes->linux_time = rtimes->tick_time + rtimes->linux_tick;
	}

    p = &arti_timer_mode[adeos_processor_id()];

    while (arti_rdtsc() < arti_timers_sync_time)
	;

    if (p->mode)
	arti_setup_periodic_apic(p->count,ARTI_APIC_TIMER_VECTOR);
    else
	arti_setup_oneshot_apic(p->count,ARTI_APIC_TIMER_VECTOR);

    arti_critical_exit(flags);

    rt_release_irq(ARTI_APIC_TIMER_IPI);

    rt_request_irq(ARTI_APIC_TIMER_IPI,(rt_irq_handler_t)handler,NULL);

    rt_request_linux_irq(ARTI_TIMER_8254_IRQ,
			 &arti_broadcast_to_timers,
			 "broadcast",
			 &arti_broadcast_to_timers);

    for (cpuid = 0; cpuid < ARTI_NR_CPUS; cpuid++)
	{
	p = &tmdata[cpuid];

	if (p->mode)
	    p->count = arti_imuldiv(p->count,ARTI_FREQ_APIC,1000000000);
	else
	    p->count = arti_imuldiv(p->count,arti_tunables.cpu_freq,1000000000);
	}
}

void rt_free_apic_timers(void)

{
    unsigned long flags;

    TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_APIC_FREE,0,0);

    rt_free_linux_irq(ARTI_TIMER_8254_IRQ,&arti_broadcast_to_timers);

    flags = arti_critical_enter(arti_critical_sync);

    arti_sync_level = 3;
    arti_setup_periodic_apic(ARTI_APIC_ICOUNT,LOCAL_TIMER_VECTOR);
    rt_release_irq(ARTI_APIC_TIMER_IPI);

    arti_critical_exit(flags);
}

#else  /* !CONFIG_SMP */

#define arti_critical_sync NULL

#define arti_setup_periodic_apic(count,vector);

#define arti_setup_oneshot_apic(count,vector);

int rt_assign_irq_to_cpu (int irq, unsigned long cpus_mask) {

    return 0;
}

int rt_reset_irq_to_sym_mode (int irq) {

    return 0;
}

void arti_broadcast_to_timers (int irq,
			       void *dev_id,
			       struct pt_regs *regs) {
} 

void rt_request_timer_cpuid (void (*handler)(void),
			     unsigned tick,
			     int cpuid) {
}

void rt_request_apic_timers (void (*handler)(void),
			     struct apic_timer_setup_data *tmdata) {
}

#define rt_free_apic_timers() rt_free_timer()

#endif /* CONFIG_SMP */

int rt_request_timer (void (*handler)(void),
		      unsigned tick,
		      int use_apic)
{
    unsigned long flags;

    TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_REQUEST,handler,tick);

    if (use_apic)
	set_bit(ARTI_USE_APIC,&arti_status);
    else
	clear_bit(ARTI_USE_APIC,&arti_status);

    flags = arti_critical_enter(arti_critical_sync);

    rt_times.tick_time = arti_rdtsc();

    if (tick > 0)
	{
	rt_times.linux_tick = use_apic ? ARTI_APIC_ICOUNT : LATCH;
	rt_times.tick_time = ((RTIME)rt_times.linux_tick)*(jiffies + 1);
	rt_times.intr_time = rt_times.tick_time + tick;
	rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.periodic_tick = tick;

	if (use_apic)
	    {
	    arti_sync_level = 2;
	    rt_release_irq(ARTI_APIC_TIMER_IPI);
	    rt_request_irq(ARTI_APIC_TIMER_IPI,(rt_irq_handler_t)handler,NULL);
	    arti_setup_periodic_apic(tick,ARTI_APIC_TIMER_VECTOR);
	    }
	else
	    {
	    outb(0x34,0x43);
	    outb(tick & 0xff,0x40);
	    outb(tick >> 8,0x40);

	    rt_release_irq(ARTI_TIMER_8254_IRQ);

	    if (rt_request_irq(ARTI_TIMER_8254_IRQ,(rt_irq_handler_t)handler,NULL) < 0)
		{
		arti_critical_exit(flags);
		return -EINVAL;
		}
	    }
	}
    else
	{
	rt_times.linux_tick = arti_imuldiv(LATCH,arti_tunables.cpu_freq,ARTI_FREQ_8254);
	rt_times.intr_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.linux_time = rt_times.tick_time + rt_times.linux_tick;
	rt_times.periodic_tick = rt_times.linux_tick;

	if (use_apic)
	    {
	    arti_sync_level = 2;
	    rt_release_irq(ARTI_APIC_TIMER_IPI);
	    rt_request_irq(ARTI_APIC_TIMER_IPI,(rt_irq_handler_t)handler,NULL);
	    arti_setup_oneshot_apic(ARTI_APIC_ICOUNT,ARTI_APIC_TIMER_VECTOR);
	    }
	else
	    {
	    outb(0x30,0x43);
	    outb(LATCH & 0xff,0x40);
	    outb(LATCH >> 8,0x40);

	    rt_release_irq(ARTI_TIMER_8254_IRQ);

	    if (rt_request_irq(ARTI_TIMER_8254_IRQ,(rt_irq_handler_t)handler,NULL) < 0)
		{
		arti_critical_exit(flags);
		return -EINVAL;
		}
	    }
	}

    arti_critical_exit(flags);

    return use_apic ? rt_request_linux_irq(ARTI_TIMER_8254_IRQ,
					   arti_broadcast_to_timers,
					   "broadcast",
					   arti_broadcast_to_timers) : 0;
}

void rt_free_timer (void)

{
    unsigned long flags;

    TRACE_RTAI_TIMER(TRACE_RTAI_EV_TIMER_FREE,0,0);

    if (test_bit(ARTI_USE_APIC,&arti_status))
	rt_free_linux_irq(ARTI_TIMER_8254_IRQ,
			  &arti_broadcast_to_timers);

    flags = arti_critical_enter(arti_critical_sync);

    if (test_bit(ARTI_USE_APIC,&arti_status))
	{
	arti_sync_level = 3;
	arti_setup_periodic_apic(ARTI_APIC_ICOUNT,LOCAL_TIMER_VECTOR);
	clear_bit(ARTI_USE_APIC,&arti_status);
	}
    else
	{
	outb(0x34,0x43);
	outb(LATCH & 0xff,0x40);
	outb(LATCH >> 8,0x40);
	rt_release_irq(ARTI_TIMER_8254_IRQ);
	}

    arti_critical_exit(flags);
}

RT_TRAP_HANDLER rt_set_trap_handler (RT_TRAP_HANDLER handler) {

    return (RT_TRAP_HANDLER)xchg(&arti_trap_handler,handler);
}

void rt_mount (void) {

    MOD_INC_USE_COUNT;
    arti_mount_count++;
    TRACE_RTAI_MOUNT();
}

void rt_umount (void) {

    TRACE_RTAI_UMOUNT();
    arti_mount_count--;
    MOD_DEC_USE_COUNT;
}

static void arti_irq_trampoline (unsigned irq)

{
    TRACE_RTAI_GLOBAL_IRQ_ENTRY(irq,0);

    if (arti_realtime_irq[irq].handler)
	arti_realtime_irq[irq].handler(irq,arti_realtime_irq[irq].cookie);
    else
	adeos_propagate_irq(irq);

    TRACE_RTAI_GLOBAL_IRQ_EXIT();
}

static void arti_trap_fault (adevinfo_t *evinfo)

{
    adeos_declare_cpuid;

    static const int trap2sig[] = {
    	SIGFPE,         //  0 - Divide error
	SIGTRAP,        //  1 - Debug
	SIGSEGV,        //  2 - NMI (but we ignore these)
	SIGTRAP,        //  3 - Software breakpoint
	SIGSEGV,        //  4 - Overflow
	SIGSEGV,        //  5 - Bounds
	SIGILL,         //  6 - Invalid opcode
	SIGSEGV,        //  7 - Device not available
	SIGSEGV,        //  8 - Double fault
	SIGFPE,         //  9 - Coprocessor segment overrun
	SIGSEGV,        // 10 - Invalid TSS
	SIGBUS,         // 11 - Segment not present
	SIGBUS,         // 12 - Stack segment
	SIGSEGV,        // 13 - General protection fault
	SIGSEGV,        // 14 - Page fault
	0,              // 15 - Spurious interrupt
	SIGFPE,         // 16 - Coprocessor error
	SIGBUS,         // 17 - Alignment check
	SIGSEGV,        // 18 - Reserved
	SIGFPE,         // 19 - XMM fault
	0,0,0,0,0,0,0,0,0,0,0,0
    };

    TRACE_RTAI_TRAP_ENTRY(evinfo->event,0);

    /* Note: NMI is not pipelined by Adeos. */

    if (evinfo->domid == ARTI_DOMAIN_ID)
	{
	if (evinfo->event == 7)	/* (FPU) Device not available. */
	    {
	    /* Ok, this one is a bit insane: some RTAI examples use
	       the FPU in real-time mode while the TS bit is on from a
	       previous Linux switch, so this trap is raised. We just
	       simulate a math_state_restore() using the proper
	       "current" value from the Linux domain here to please
	       everyone without impacting the existing code. */

	    struct task_struct *linux_task = arti_get_current(cpuid);

	    if (linux_task->used_math)
		restore_task_fpenv(linux_task);	/* Does clts(). */
	    else
		{
		init_xfpu();	/* Does clts(). */
		linux_task->used_math = 1;
		}

	    linux_task->flags |= PF_USEDFPU;

	    goto endtrap;
	    }

	if (arti_trap_handler != NULL &&
	    (test_bit(cpuid,&arti_cpu_realtime) || test_bit(cpuid,&arti_cpu_lxrt)) &&
	    arti_trap_handler(evinfo->event,
			      trap2sig[evinfo->event],
			      (struct pt_regs *)evinfo->evdata,
			      NULL) != 0)
	    goto endtrap;

	printk("RTAI domain error: trap #%d, eip %p, cpu #%d\n",
	       evinfo->event,
	       (void *)((struct pt_regs *)evinfo->evdata)->eip,
	       cpuid);

	show_stack(NULL);

	show_registers((struct pt_regs *)evinfo->evdata);

	for(;;) safe_halt();
	}
    else
	adeos_propagate_event(evinfo);

endtrap:

    TRACE_RTAI_TRAP_EXIT();
}

static void arti_ssrq_trampoline (unsigned virq)

{
    unsigned long pending;

    spin_lock(&arti_ssrq_lock);

    while ((pending = arti_sysreq_pending & ~arti_sysreq_running) != 0)
	{
	unsigned srq = ffnz(pending);
	set_bit(srq,&arti_sysreq_running);
	clear_bit(srq,&arti_sysreq_pending);
	spin_unlock(&arti_ssrq_lock);

	if (test_bit(srq,&arti_sysreq_map))
	    arti_sysreq_table[srq].k_handler();

	clear_bit(srq,&arti_sysreq_running);
	spin_lock(&arti_ssrq_lock);
	}

    spin_unlock(&arti_ssrq_lock);
}

static long long arti_usrq_trampoline(unsigned srq,
				      unsigned label) __attribute__ ((__unused__));

static long long arti_usrq_trampoline (unsigned srq, unsigned label)

{
    long long r = 0;

    TRACE_RTAI_SRQ_ENTRY(srq);

    if (srq > 1 && srq < ARTI_NR_SRQS &&
	test_bit(srq,&arti_sysreq_map) &&
	arti_sysreq_table[srq].u_handler != NULL)
	r = arti_sysreq_table[srq].u_handler(label);

    for (srq = 2; srq < ARTI_NR_SRQS; srq++)
	if (test_bit(srq,&arti_sysreq_map) &&
	    arti_sysreq_table[srq].label == label)
	    r = (long long)srq;

    TRACE_RTAI_SRQ_EXIT();

    return r;
}

static void arti_uvec_handler (void)

{
    __asm__ __volatile__ ( \
	"cld\n\t" \
        "pushl %es\n\t" \
        "pushl %ds\n\t" \
        "pushl %ebp\n\t" \
	"pushl %edi\n\t" \
        "pushl %esi\n\t" \
        "pushl %ecx\n\t" \
	"pushl %ebx\n\t" \
        "pushl %edx\n\t" \
        "pushl %eax\n\t" \
	"movl $" STR(__KERNEL_DS) ",%ebx\n\t" \
        "mov %bx,%ds\n\t" \
        "mov %bx,%es\n\t" \
        "call "SYMBOL_NAME_STR(arti_usrq_trampoline)"\n\t" \
	"addl $8,%esp\n\t" \
        "popl %ebx\n\t" \
        "popl %ecx\n\t" \
        "popl %esi\n\t" \
	"popl %edi\n\t" \
        "popl %ebp\n\t" \
        "popl %ds\n\t" \
        "popl %es\n\t" \
        "iret");
}

#define arti_set_gate(gate_addr,type,dpl,addr) \
do { \
  int __d0, __d1; \
  __asm__ __volatile__ ("movw %%dx,%%ax\n\t" \
	"movw %4,%%dx\n\t" \
	"movl %%eax,%0\n\t" \
	"movl %%edx,%1" \
	:"=m" (*((long *) (gate_addr))), \
	 "=m" (*(1+(long *) (gate_addr))), "=&a" (__d0), "=&d" (__d1) \
	:"i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	 "3" ((char *) (addr)),"2" (__KERNEL_CS << 16)); \
} while (0)

static struct mmreq {
    int in, out, count;
#define MAX_MM 32  /* Should be more than enough (must be a power of 2). */
#define bump_mmreq(x) do { x = (x + 1) & (MAX_MM - 1); } while(0)
    struct mm_struct *mm[MAX_MM];
} arti_mmrqtab[NR_CPUS];

static void arti_linux_schedule_head (adevinfo_t *evinfo)

{
    struct { struct task_struct *prev, *next; } *evdata = (__typeof(evdata))evinfo->evdata;
    struct task_struct *prev = evdata->prev;

    /* The SCHEDULE_HEAD event is sent by the (Adeosized) Linux kernel
       whenever it's about to switch a process out. This hook on such
       event is aimed at preventing the last active MM from being
       dropped during the LXRT real-time operations since it's a far
       too lengthy atomic operation. See kernel/sched.c (schedule())
       for more. The MM dropping is simply postponed until the
       SCHEDULE_TAIL event is received, right after the incoming task
       has been switched in. */

    if (!prev->mm)
	{
	struct mmreq *p = arti_mmrqtab + prev->processor;
	struct mm_struct *oldmm = prev->active_mm;
	BUG_ON(p->count >= MAX_MM);
	/* Prevent the MM from being dropped in schedule(), then pend
	   a request to drop it later in arti_linux_schedule_tail(). */
	atomic_inc(&oldmm->mm_count);
	p->mm[p->in] = oldmm;
	bump_mmreq(p->in);
	p->count++;
	}

    adeos_propagate_event(evinfo);
}

static void arti_linux_schedule_tail (adevinfo_t *evinfo)

{
    struct mmreq *p = arti_mmrqtab + smp_processor_id();

    while (p->out != p->in)
	{
	struct mm_struct *oldmm = p->mm[p->out];
	mmdrop(oldmm);
	bump_mmreq(p->out);
	p->count--;
	}

    adeos_propagate_event(evinfo);
}

void arti_switch_linux_mm (struct task_struct *prev,
			   struct task_struct *next,
			   int cpuid)
{
    struct mm_struct *oldmm = prev->active_mm;

    cpuid &= 0x7fffffff;

    switch_mm(oldmm,next->active_mm,next,cpuid);

    if (!next->mm)
	enter_lazy_tlb(oldmm,next,cpuid);
}

static void arti_linux_signal_process (adevinfo_t *evinfo)

{
    if (arti_signal_handler)
	{
	struct { struct task_struct *task; int sig; } *evdata = (__typeof(evdata))evinfo->evdata;
	struct task_struct *task = evdata->task;

	if (evdata->sig == SIGKILL &&
	    (task->policy == SCHED_FIFO || task->policy == SCHED_RR) &&
	    task->ptd[0])
	    {
	    if (!arti_signal_handler(task,evdata->sig))
		/* Don't propagate so that Linux won't further process
		   the signal. */
		return;
	    }
	}

    adeos_propagate_event(evinfo);
}

void arti_attach_lxrt (void)

{
    /* Must be called on behalf the Linux domain. */
    adeos_catch_event(ADEOS_SCHEDULE_TAIL,&arti_linux_schedule_tail);
    adeos_catch_event(ADEOS_SCHEDULE_HEAD,&arti_linux_schedule_head);
    adeos_catch_event(ADEOS_SIGNAL_PROCESS,&arti_linux_signal_process);
}

void arti_detach_lxrt (void)

{
    unsigned long flags;
    struct mmreq *p;
    
    /* Must be called on behalf the Linux domain. */
    adeos_catch_event(ADEOS_SIGNAL_PROCESS,NULL);
    adeos_catch_event(ADEOS_SCHEDULE_HEAD,NULL);
    adeos_catch_event(ADEOS_SCHEDULE_TAIL,NULL);

    flags = arti_critical_enter(NULL);

    /* Flush the MM log for all processors */
    for (p = arti_mmrqtab; p < arti_mmrqtab + NR_CPUS; p++)
	{
	while (p->out != p->in)
	    {
	    struct mm_struct *oldmm = p->mm[p->out];
	    mmdrop(oldmm);
	    bump_mmreq(p->out);
	    p->count--;
	    }
	}

    arti_critical_exit(flags);
}

static void arti_install_archdep (void)

{
    /* Backup and replace the sysreq vector. */
    arti_sysvec = idt_table[RTAI_SYS_VECTOR];
    arti_set_gate(idt_table+RTAI_SYS_VECTOR,15,3,&arti_uvec_handler);

    if (arti_cpufreq_arg == 0)
	{
	adsysinfo_t sysinfo;
	adeos_get_sysinfo(&sysinfo);
	arti_cpufreq_arg = (int)sysinfo.cpufreq;
	}

    arti_tunables.cpu_freq = arti_cpufreq_arg;

#ifdef CONFIG_X86_LOCAL_APIC
    if (arti_apicfreq_arg == 0)
	arti_apicfreq_arg = apic_read(APIC_TMICT) * HZ;

    arti_tunables.apic_freq = arti_apicfreq_arg;
#endif /* CONFIG_X86_LOCAL_APIC */
}

static void arti_uninstall_archdep (void) {

    idt_table[RTAI_SYS_VECTOR] = arti_sysvec;
}

RTIME rd_8254_ts (void)

{
    unsigned long flags;
    int inc, c2;
    RTIME t;

    adeos_hw_local_irq_save(flags);	/* local hw masking is
					   required here. */
    outb(0xD8,0x43);
    c2 = inb(0x42);
    inc = arti_last_8254_counter2 - (c2 |= (inb(0x42) << 8));
    arti_last_8254_counter2 = c2;
    t = (arti_ts_8254 += (inc > 0 ? inc : inc + ARTI_COUNTER_2_LATCH));

    adeos_hw_local_irq_restore(flags);

    return t;
}

void rt_setup_8254_tsc (void)

{
    unsigned long flags;
    int c;

    flags = arti_critical_enter(NULL);

    outb_p(0x00,0x43);
    c = inb_p(0x40);
    c |= inb_p(0x40) << 8;
    outb_p(0xB4, 0x43);
    outb_p(ARTI_COUNTER_2_LATCH & 0xff, 0x42);
    outb_p(ARTI_COUNTER_2_LATCH >> 8, 0x42);
    arti_ts_8254 = c + ((RTIME)LATCH)*jiffies;
    arti_last_8254_counter2 = 0; 
    outb_p((inb_p(0x61) & 0xFD) | 1, 0x61);

    arti_critical_exit(flags);
}

int rt_calibrate_8254 (void)

{
    unsigned long flags, dt;
    RTIME t;
    int n;

    flags = arti_critical_enter(NULL);

    outb(0x34,0x43);

    t = arti_rdtsc();

    for (n = 0; n < 10000; n++)
	{ 
	outb(LATCH & 0xff,0x40);
	outb(LATCH >> 8,0x40);
	}

    dt = arti_rdtsc() - t;

    arti_critical_exit(flags);

    return arti_imuldiv(dt,100000,ARTI_CPU_FREQ);
}

#ifdef CONFIG_PROC_FS

struct proc_dir_entry *rtai_proc_root = NULL;

static int arti_read_proc (char *page,
			   char **start,
			   off_t off,
			   int count,
			   int *eof,
			   void *data)
{
    PROC_PRINT_VARS;
    int i, none;

    PROC_PRINT("\n** RTAI/x86 over Adeos:\n\n");
    PROC_PRINT("    RTAI mount count: %d\n",arti_mount_count);
#ifdef CONFIG_X86_LOCAL_APIC
    PROC_PRINT("    APIC Frequency: %d\n",arti_tunables.apic_freq);
    PROC_PRINT("    APIC Latency: %d ns\n",ARTI_LATENCY_APIC);
    PROC_PRINT("    APIC Setup: %d ns\n",ARTI_SETUP_TIME_APIC);
#endif /* CONFIG_X86_LOCAL_APIC */
    
    none = 1;

    PROC_PRINT("\n** Real-time IRQs used by RTAI: ");

    for (i = 0; i < NR_IRQS; i++)
	{
	if (arti_realtime_irq[i].handler)
	    {
	    if (none)
		{
		PROC_PRINT("\n");
		none = 0;
		}

	    PROC_PRINT("\n    #%d at %p", i, arti_realtime_irq[i].handler);
	    }
        }

    if (none)
	PROC_PRINT("none");

    PROC_PRINT("\n\n");

    PROC_PRINT("** RTAI extension traps: \n\n");
    PROC_PRINT("    SYSREQ=0x%x\n",RTAI_SYS_VECTOR);
    PROC_PRINT("      LXRT=0x%x\n",RTAI_LXRT_VECTOR);
    PROC_PRINT("       SHM=0x%x\n\n",RTAI_SHM_VECTOR);

    none = 1;
    PROC_PRINT("** RTAI SYSREQs in use: ");

    for (i = 0; i < ARTI_NR_SRQS; i++)
	{
	if (arti_sysreq_table[i].k_handler ||
	    arti_sysreq_table[i].u_handler)
	    {
	    PROC_PRINT("#%d ", i);
	    none = 0;
	    }
        }

    if (none)
	PROC_PRINT("none");

    PROC_PRINT("\n\n");

    PROC_PRINT_DONE;
}

static int arti_proc_register (void)

{
    struct proc_dir_entry *ent;

    rtai_proc_root = create_proc_entry("rtai",S_IFDIR, 0);

    if (!rtai_proc_root)
	{
	printk("Unable to initialize /proc/rtai\n");
	return -1;
        }

    rtai_proc_root->owner = THIS_MODULE;

    ent = create_proc_entry("rtai",S_IFREG|S_IRUGO|S_IWUSR,rtai_proc_root);

    if (!ent)
	{
	printk("Unable to initialize /proc/rtai/rtai\n");
	return -1;
        }

    ent->read_proc = arti_read_proc;

    return 0;
}

static void arti_proc_unregister (void)

{
    remove_proc_entry("rtai",rtai_proc_root);
    remove_proc_entry("rtai",0);
}

#endif /* CONFIG_PROC_FS */

static void arti_domain_entry (int iflag)

{
    unsigned irq, trapnr;

    if (iflag)
	{
	for (irq = 0; irq < NR_IRQS; irq++)
	    adeos_virtualize_irq(irq,
				 &arti_irq_trampoline,
				 NULL,
				 IPIPE_DYNAMIC_MASK);

	/* Trap all faults. */
	for (trapnr = 0; trapnr < ADEOS_NR_FAULTS; trapnr++)
	    adeos_catch_event(trapnr,&arti_trap_fault);

	printk("RTAI/Adeos mounted.\n");
	}

    for (;;)
	adeos_suspend_domain();
}

int init_module (void)

{
    unsigned long flags;
    int key0, key1;
    adattr_t attr;

    /* Allocate a virtual interrupt to handle sysreqs within the Linux
       domain. */
    arti_sysreq_virq = adeos_alloc_irq();

    if (!arti_sysreq_virq)
	{
	printk("RTAI/Adeos: no virtual interrupt available\n");
	return 1;
	}

    /* Reserve the first two _consecutive_ per-thread data key in the
       Linux domain. This is rather crappy, since we depend on
       statically defined PTD key values, which is exactly what the
       PTD scheme is here to prevent. Unfortunately, reserving these
       specific keys is the only way to remain source compatible with
       the current LXRT implementation. */
    flags = arti_critical_enter(NULL);
    arti_adeos_ptdbase = key0 = adeos_alloc_ptdkey();
    key1 = adeos_alloc_ptdkey();
    arti_critical_exit(flags);

    if (key0 != 0 && key1 != 1)
	{
	printk("RTAI/Adeos: per-thread keys #0 and/or #1 are busy\n");
	return 1;
	}

    adeos_virtualize_irq(arti_sysreq_virq,
			 &arti_ssrq_trampoline,
			 NULL,
			 IPIPE_HANDLE_MASK);

    arti_install_archdep();

    arti_mount_count = 1;

#ifdef CONFIG_PROC_FS
    arti_proc_register();
#endif

    /* Let Adeos do its magic for our real-time domain. */
    adeos_init_attr(&attr);
    attr.name = "RTAI";
    attr.domid = ARTI_DOMAIN_ID;
    attr.entry = &arti_domain_entry;
    attr.priority = ADEOS_ROOT_PRI + 100; /* Precede Linux in the pipeline */

    return adeos_register_domain(&arti_domain,&attr);
}

void cleanup_module (void)

{
#ifdef CONFIG_PROC_FS
    arti_proc_unregister();
#endif

    adeos_virtualize_irq(arti_sysreq_virq,NULL,NULL,0);
    adeos_free_irq(arti_sysreq_virq);
    arti_uninstall_archdep();
    adeos_free_ptdkey(arti_adeos_ptdbase); /* #0 and #1 actually */
    adeos_free_ptdkey(arti_adeos_ptdbase + 1);
    adeos_unregister_domain(&arti_domain);
    printk("RTAI/Adeos unmounted.\n");
}
