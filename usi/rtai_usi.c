/*
COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/slab.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include <rtai_tasklets.h>
#include <rtai_usi.h>

MODULE_LICENSE("GPL");

#define MODULE_NAME "RTAI_USI"

#define MAX_LOCKS  20
static int MaxLocks = MAX_LOCKS;
MODULE_PARM(MaxLocks, "i");

static spinlock_t *usi_lock_pool, **usi_lock_pool_p;
static spinlock_t usi_lock = SPIN_LOCK_UNLOCKED;
static volatile int usip;

static SEM *irqsem[NR_IRQS];
static struct rt_tasklet_struct *irqtasklet[NR_IRQS];

static void sem_handler(int irq)
{
	irqsem[irq]->owndby = (void *)irq;
	rt_sem_signal(irqsem[irq]);
} 

static void tasklet_handler(int irq)
{
	irqtasklet[irq]->data = ((-((irqtasklet[irq]->task)->suspdepth - 1)) << 16) | irq;
	rt_exec_tasklet(irqtasklet[irq]);
} 

static int usi_wait_intr(SEM *sem, unsigned long *irq)
{
	int retval;
	retval = rt_sem_wait(sem);	
	*irq = (unsigned long)sem->owndby; 
	return retval;
}

static int usi_wait_intr_if(SEM *sem, unsigned long *irq)
{
	int retval;
	retval = rt_sem_wait_if(sem);	
	*irq = (unsigned long)sem->owndby; 
	return retval;
}

static int usi_wait_intr_until(SEM *sem, RTIME until, unsigned long *irq)
{
	int retval;
	retval = rt_sem_wait_until(sem, until);	
	*irq = (unsigned long)sem->owndby; 
	return retval;
}

static int usi_wait_intr_timed(SEM *sem, RTIME delay, unsigned long *irq)
{
	int retval;
	retval = rt_sem_wait_timed(sem, delay);	
	*irq = (unsigned long)sem->owndby; 
	return retval;
}

static int usi_request_global_irq(unsigned int irq, void *hook, int hooktype)
{
	if (hook) {
		int retval;
		if (hooktype == USI_SEM) {
			if (!(retval = rt_request_global_irq(irq, (void *)sem_handler))) {
				irqsem[irq] = hook;
			}
			return retval;
		}
		if (hooktype == USI_TASKLET) {
			if (!(retval = rt_request_global_irq(irq, (void *)tasklet_handler))) {
				irqtasklet[irq] = hook;
			}
			return retval;
		}
	}
	return -EINVAL;
}

static void *rt_spin_lock_init(void)
{
	unsigned long flags;
	spinlock_t *p;
 
	flags = rt_spin_lock_irqsave(&usi_lock);
	if (usip < MaxLocks) {
		p = usi_lock_pool_p[usip++];
		rt_spin_unlock_irqrestore(flags, &usi_lock);
		spin_lock_init(p);
		return p;
	}
	rt_spin_unlock_irqrestore(flags, &usi_lock);
	return 0;
}

static inline int rt_spin_lock_delete(void *lock)
{
        unsigned long flags;
 
	flags = rt_spin_lock_irqsave(&usi_lock);
	if (usip > MaxLocks) {
		usi_lock_pool_p[--usip] = lock;
		rt_spin_unlock_irqrestore(flags, &usi_lock);
		return 0;
	}
	rt_spin_unlock_irqrestore(flags, &usi_lock);
	return -EINVAL;
}
                                                                               
static void usi_spin_lock(spinlock_t *lock)          
{
	spin_lock(lock);
}

static void usi_spin_unlock(spinlock_t *lock)          
{
	spin_unlock(lock);
}

static void usi_spin_lock_irq(spinlock_t *lock)          
{
	rt_spin_lock_irq(lock);
}

static void usi_spin_unlock_irq(spinlock_t *lock)
{
	rt_spin_unlock_irq(lock);
}

static unsigned long usi_spin_lock_irqsave(spinlock_t *lock)          
{
	return rt_spin_lock_irqsave(lock);
}

static void usi_spin_unlock_irqrestore(unsigned long flags, spinlock_t *lock)
{
	rt_spin_unlock_irqrestore(flags, lock);
}

static void usi_global_cli(void)
{
	rt_global_cli();
}

static void usi_global_sti(void)
{
	rt_global_sti();
}

static unsigned long usi_global_save_flags_and_cli(void)
{
	return rt_global_save_flags_and_cli();
}

static unsigned long usi_global_save_flags(void)
{
	unsigned long flags;
	rt_global_save_flags(&flags);
	return flags;
}

static void usi_global_restore_flags(unsigned long flags)
{
	rt_global_restore_flags(flags);
}


static void usi_cli(void)
{
	hard_cli();
}

static void usi_sti(void)
{
	hard_sti();
}

static unsigned long usi_save_flags_and_cli(void)
{
	unsigned long flags;
	hard_save_flags_and_cli(flags);
	return flags;
}

static unsigned long usi_save_flags(void)
{
	unsigned long flags;
	hard_save_flags(flags);
	return flags;
}

static void usi_restore_flags(unsigned long flags)
{
	hard_restore_flags(flags);
}

static struct rt_fun_entry rtai_usi_fun[] = {
	[_REQ_GLB_IRQ]		= { 0, usi_request_global_irq},
	[_FREE_GLB_IRQ]		= { 0, rt_free_global_irq},
	[_STARTUP_IRQ]		= { 0, rt_startup_irq },
	[_SHUTDOWN_IRQ]		= { 0, rt_shutdown_irq },
	[ _ENABLE_IRQ]		= { 0, rt_enable_irq },
	[_DISABLE_IRQ]		= { 0, rt_disable_irq },
	[_MASK_AND_ACK_IRQ]	= { 0, rt_mask_and_ack_irq },
	[_ACK_IRQ]		= { 0, rt_ack_irq },
	[_UNMASK_IRQ ]		= { 0, rt_unmask_irq },
	[_PEND_LINUX_IRQ]	= { 0, rt_pend_linux_irq },
	[_INIT_SPIN_LOCK]	= { 0, rt_spin_lock_init },
	[_SPIN_LOCK]		= { 0, usi_spin_lock },
	[_SPIN_UNLOCK]		= { 0, usi_spin_unlock },
	[_SPIN_LOCK_IRQ]	= { 0, usi_spin_lock_irq },
	[_SPIN_UNLOCK_IRQ]	= { 0, usi_spin_unlock_irq },
	[_SPIN_LOCK_IRQSV]	= { 0, usi_spin_lock_irqsave },
	[_SPIN_UNLOCK_IRQRST]	= { 0, usi_spin_unlock_irqrestore },
	[_GLB_CLI]		= { 0, usi_global_cli },
	[_GLB_STI]		= { 0, usi_global_sti},
	[_GLB_SVFLAGS_CLI]	= { 0, usi_global_save_flags_and_cli },
	[_GLB_SVFLAGS]		= { 0, usi_global_save_flags },
	[_GLB_RSTFLAGS]		= { 0, usi_global_restore_flags },
	[_CLI]			= { 0, usi_cli },
	[_STI]			= { 0, usi_sti},
	[_SVFLAGS_CLI]		= { 0, usi_save_flags_and_cli },
	[_SVFLAGS]		= { 0, usi_save_flags },
	[_RSTFLAGS]		= { 0, usi_restore_flags },
	[_WAIT_INTR]		= { UW1(2, 3), usi_wait_intr },
	[_WAIT_INTR_IF]		= { UW1(2, 3), usi_wait_intr_if },
	[_WAIT_INTR_UNTIL]	= { UW1(4, 5), usi_wait_intr_until },
	[_WAIT_INTR_TIMED]	= { UW1(4, 5), usi_wait_intr_timed }
};

int init_module(void)
{
	int i;
        if (!(usi_lock_pool = kmalloc(MaxLocks*sizeof(spinlock_t), GFP_KERNEL)) || !(usi_lock_pool_p = kmalloc(MaxLocks*sizeof(spinlock_t *), GFP_KERNEL))) {
		printk("KMALLOC FAILED ALLOCATING USI LOCKS\n");
		return -ENOMEM;
	}                                                                       
	if (set_rt_fun_ext_index(rtai_usi_fun, FUN_EXT_RTAI_USI)) {
		rt_printk("%d is a wrong index module for lxrt.\n", FUN_EXT_RTAI_USI);
		return -EACCES;
	}
	for (i = MaxLocks - 1; i >= 0; i--) {
		usi_lock_pool_p[i] = &usi_lock_pool[i];
	}
	printk("%s: loaded.\n",MODULE_NAME);
	return(0);
}

void cleanup_module(void)
{
	reset_rt_fun_ext_index(rtai_usi_fun, FUN_EXT_RTAI_USI);
	kfree(usi_lock_pool);
	kfree(usi_lock_pool_p);
	printk("%s: unloaded.\n", MODULE_NAME);
}
