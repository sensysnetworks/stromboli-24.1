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


/* Dec, 2003: Abramo Bagnara<abramo.bagnara@tin.it> fixed a bug in the
   declaration of wait_intr_until */

#ifndef _RTAI_USI_H_
#define _RTAI_USI_H_

#define  FUN_EXT_RTAI_USI  3

#define USI_SEM      0
#define USI_TASKLET  1

#define _REQ_GLB_IRQ 	 	 0
#define _FREE_GLB_IRQ	 	 1
#define _STARTUP_IRQ	 	 2
#define _SHUTDOWN_IRQ	 	 3
#define _ENABLE_IRQ	 	 4
#define _DISABLE_IRQ	 	 5
#define _MASK_AND_ACK_IRQ 	 6
#define _ACK_IRQ 	  	 7
#define _UNMASK_IRQ 		 8
#define _PEND_LINUX_IRQ	 	 9
#define _INIT_SPIN_LOCK		10
#define _SPIN_LOCK		11
#define _SPIN_UNLOCK		12
#define _SPIN_LOCK_IRQ		13
#define _SPIN_UNLOCK_IRQ	14
#define _SPIN_LOCK_IRQSV	15
#define _SPIN_UNLOCK_IRQRST	16
#define _GLB_CLI		17
#define _GLB_STI		18
#define _GLB_SVFLAGS_CLI	19
#define _GLB_SVFLAGS		20
#define _GLB_RSTFLAGS		21
#define _CLI			22
#define _STI			23
#define _SVFLAGS_CLI     	24
#define _SVFLAGS 		25
#define _RSTFLAGS		26
#define _WAIT_INTR		27
#define _WAIT_INTR_IF		28
#define _WAIT_INTR_UNTIL	29
#define _WAIT_INTR_TIMED	30

#ifndef __KERNEL__

#include <rtai_declare.h>
#include <rtai_lxrt.h>

static inline int rt_expand_handler_data(unsigned long data, int *irq)
{
	*irq = data & 0xFFFF;
	return (data & 0xFFFF0000) >> 16;
}

DECLARE int rt_request_global_irq(unsigned int irq, void *hook, int hooktype)
{
        struct { unsigned int irq; void *hook; int hooktype; } arg = { irq, hook, hooktype };
        return rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _REQ_GLB_IRQ, &arg).i[LOW];
}
 
DECLARE int rt_free_global_irq(unsigned int irq)
{
        struct { unsigned int irq; } arg = { irq };
        return rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _FREE_GLB_IRQ, &arg).i[LOW];
}
 
DECLARE int rt_startup_irq(unsigned int irq)
{
        struct { unsigned int irq; } arg = { irq };
        return rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _STARTUP_IRQ, &arg).i[LOW];
}
 
DECLARE void rt_shutdown_irq(unsigned int irq)
{
        struct { unsigned int irq; } arg = { irq };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _SHUTDOWN_IRQ, &arg);
}
 
DECLARE void rt_enable_irq(unsigned int irq)
{
        struct { unsigned int irq; } arg = { irq };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _ENABLE_IRQ, &arg);
}
 
DECLARE void rt_disable_irq(unsigned int irq)
{
        struct { unsigned int irq; } arg = { irq };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _DISABLE_IRQ, &arg);
}
 
DECLARE void rt_mask_and_ack_irq(unsigned int irq)
{
        struct { unsigned int irq; } arg = { irq };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _MASK_AND_ACK_IRQ, &arg);
}
 
DECLARE void rt_ack_irq(unsigned int irq)
{
        struct { unsigned int irq; } arg = { irq };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _ACK_IRQ, &arg);
}
 
DECLARE void rt_unmask_irq(unsigned int irq)
{
        struct { unsigned int irq; } arg = { irq };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _UNMASK_IRQ, &arg);
}
 
DECLARE void rt_pend_linux_irq(unsigned int irq)
{
        struct { unsigned int irq; } arg = { irq };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _PEND_LINUX_IRQ, &arg);
}
 
DECLARE void *rt_spin_lock_init(void)
{
        struct { int dummy; } arg = { 0 };
	return rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _INIT_SPIN_LOCK, &arg).v[LOW];
}
 
DECLARE void rt_spin_lock(void *lock)
{
        struct { void *lock; } arg = { lock };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _SPIN_LOCK, &arg);
}
 
DECLARE void rt_spin_unlock(void *lock)
{
        struct { void *lock; } arg = { lock };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _SPIN_UNLOCK, &arg);
}
 
DECLARE void rt_spin_lock_irq(void *lock)
{
        struct { void *lock; } arg = { lock };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _SPIN_LOCK_IRQ, &arg);
}
 
DECLARE void rt_spin_unlock_irq(void *lock)
{
        struct { void *lock; } arg = { lock };
        rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _SPIN_UNLOCK_IRQ, &arg);
}
 
DECLARE unsigned long rt_spin_lock_irqsave(void *lock)
{
	struct { void *lock; } arg = { lock };
	return rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _SPIN_LOCK_IRQSV, &arg).i[LOW];
}
 
DECLARE void rt_spin_unlock_irqrestore(unsigned long flags, void *lock)
{
	struct { unsigned long flags; void *lock; } arg = { flags, lock };
	rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _SPIN_UNLOCK_IRQRST, &arg);
}
 

DECLARE void rt_global_cli(void)
{
	struct { int dummy; } arg = { 0 };
	rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _GLB_CLI, &arg);
}

DECLARE void rt_global_sti(void)
{
	struct { int dummy; } arg = { 0 };
	rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _GLB_STI, &arg);
}

DECLARE unsigned long rt_global_save_flags_and_cli(void)
{
	struct { int dummy; } arg = { 0 };
	return rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _GLB_SVFLAGS_CLI, &arg).i[LOW];
}
 
DECLARE unsigned long rt_global_save_flags(void)
{
	struct { int dummy; } arg = { 0 };
	return rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _GLB_SVFLAGS, &arg).i[LOW];
}
 
DECLARE void rt_global_restore_flags(unsigned long flags)
{
	struct { unsigned long flags; } arg = { flags };
	rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _GLB_RSTFLAGS, &arg);
}

DECLARE void hard_cli(void)
{
	struct { int dummy; } arg = { 0 };
	rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _CLI, &arg);
}

DECLARE void hard_sti(void)
{
	struct { int dummy; } arg = { 0 };
	rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _STI, &arg);
}

DECLARE unsigned long hard_save_flags_and_cli(void)
{
	struct { int dummy; } arg = { 0 };
	return rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _SVFLAGS_CLI, &arg).i[LOW];
}
 
DECLARE unsigned long hard_save_flags(void)
{
	struct { int dummy; } arg = { 0 };
	return rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _SVFLAGS, &arg).i[LOW];
}
 
DECLARE void hard_restore_flags(unsigned long flags)
{
	struct { unsigned long flags; } arg = { flags };
	rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _RSTFLAGS, &arg);
}

DECLARE int rt_wait_intr(void *sem, unsigned long *irq)
{
	unsigned long lirq;
	int retval;
        struct { void *sem; unsigned long *lirq; int size; } arg = { sem, &lirq, sizeof(unsigned long *) };
	retval = rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _WAIT_INTR, &arg).i[LOW];
	if (irq) {
		*irq = lirq;
	}
	return retval;
}                                                                               

DECLARE int rt_wait_intr_if(void *sem, unsigned long *irq)
{
	unsigned long lirq;
	int retval;
        struct { void *sem; unsigned long *lirq; int size; } arg = { sem, &lirq, sizeof(unsigned long *) };
	retval = rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _WAIT_INTR_IF, &arg).i[LOW];
	if (irq) {
		*irq = lirq;
	}
	return retval;
}                                                                               

DECLARE int rt_wait_intr_until(void *sem, RTIME until, unsigned long *irq)
{
	unsigned long lirq;
	int retval;
        struct { void *sem; RTIME until; unsigned long *lirq; int size; } arg = { sem, until, &lirq, sizeof(unsigned long *) };
	retval = rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _WAIT_INTR_UNTIL, &arg).i[LOW];
	if (irq) {
		*irq = lirq;
	}
	return retval;
}                                                                               

DECLARE int rt_wait_intr_timed(void *sem, RTIME delay, unsigned long *irq)
{
	unsigned long lirq;
	int retval;
        struct { void *sem; RTIME delay; unsigned long *lirq; int size; } arg = { sem, delay, &lirq, sizeof(unsigned long *) };
	retval = rtai_lxrt(FUN_EXT_RTAI_USI, SIZARG, _WAIT_INTR_TIMED, &arg).i[LOW];
	if (irq) {
		*irq = lirq;
	}
	return retval;
}                                                                               

#endif

#endif /* _RTAI_USI_H_ */
