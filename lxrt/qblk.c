/*
COPYRIGHT (C) 1992-2000  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)

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

$Id: qblk.c,v 1.1.1.1 2004/06/06 14:02:34 rpm Exp $ 
*/

/*
 * Originally developped circa 1992 by Alain Choquet who was in charge
 * of software development at POSEIDON CONTROLS INC for four years.
*/

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>

#include <asm/uaccess.h>
#include <asm/mmu_context.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>

#include "msg.h"
#include "qblk.h"

#define NULLNK		((QBLK **)-1)
#define TIC(cur)	(cur->tic)
#define RPT(cur)	(cur->rpt)

#define DECLARE_TQ struct TickQueue *TQ
#define ASSIGN_TQ  TQ=(struct TickQueue *)((rt_lxrt_whoami())->tick_queue)
#define LOCK_TQ    flags=rt_spin_lock_irqsave(&TQ->slock)
#define UNLOCK_TQ  rt_spin_unlock_irqrestore(flags, &TQ->slock)

inline void tqcancel(void *dum, QBLK *cur)
{
	unsigned long flags; DECLARE_TQ; ASSIGN_TQ; LOCK_TQ;

	if (cur->nxt) TIC(cur->nxt) += TIC(cur);
	if (TQ->wait == cur) TQ->wait = cur->nxt;

	UNLOCK_TQ;
}

inline int tqadjust(void)
{
	QBLK *cur;
	RTIME tic;
	int   ret;
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ; LOCK_TQ;

	tic = TQ->tic;
	TQ->tic = rt_get_time();	// time now
	if (TQ->wait) {
		tic = TQ->tic - tic;	// elapsed ticks since last call
		cur = TQ->wait;
		do {
			if (tic < TIC(cur)) {
				TIC(cur) -= tic;
			/**/	break;
				}
			tic -= TIC(cur);
			TIC(cur) = 0;
			} while ((cur = cur->nxt));
		TQ->wait = cur;
		}
        ret = (TQ->hook.head != TQ->wait);
	UNLOCK_TQ;
	return ret;
}

inline void tqinsert(QBLK *cur, RTIME tic, RTIME rpt)
{
	QBLK *nxt;
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	RPT(cur) = rpt ? tic : 0;

	tqadjust();

	LOCK_TQ;
	nxt = TQ->wait;
	if (nxt && (tic >= TIC(nxt))) {
		do {
			tic -= TIC(nxt);
			nxt = nxt->nxt;
		} while (nxt && (tic >= TIC(nxt)));
	} else {
		if (tic) TQ->wait = cur;
	}

	if (nxt) {
		TIC(nxt) -= tic;
	        UNLOCK_TQ;
		rt_qBlkBefore(cur, nxt);
	} else {
		UNLOCK_TQ;
		rt_qBlkAtTail(cur, &TQ->hook);
	}

	TIC(cur) = tic;
}

inline void tqcomplete(QBLK *cur)
{
        DECLARE_TQ; ASSIGN_TQ;

	if (!RPT(cur)) {
		rt_qBlkRelease(cur);
	} else {
		rt_qBlkAtHead(cur, &TQ->exec);
	}

	exec_func(cur->fun, cur->arg, cur->evn);

	if (TQ->exec.head == cur) {
		tqinsert(rt_qBlkUnhook(cur), RPT(cur), 1LL);
	}

}

/*****************************************************************************/

static void exit_handler(void *rtt,  int arg)
{
	(void) arg;
	rt_qCleanup((RT_TASK *) rtt);
}		

int rt_InitTickQueue(void)
{
	struct TickQueue *pt;
	RT_TASK *rtt;
	
	pt = rt_malloc(sizeof(struct TickQueue));
	if( !pt ) return -ENOMEM;

	memset( pt, 0, sizeof(struct TickQueue));
	pt->hook.cancel = tqcancel;
	pt->slock       = SPIN_LOCK_UNLOCKED;

	(rtt = rt_lxrt_whoami())->tick_queue = pt;

	if( !__set_exit_handler(rtt, exit_handler, rtt, 0)) {
		rt_free(pt);
		rtt->tick_queue = 0;
		return -ENOMEM;
	}
			
	return 0;
}

void rt_ReleaseTickQueue(void)
{
	DECLARE_TQ; ASSIGN_TQ;

	if (TQ) rt_free(TQ);
        rt_lxrt_whoami()->tick_queue = (void *)0;
}

QHOOK *rt_GetTickQueueHook(void)
{
	DECLARE_TQ; ASSIGN_TQ;

	return (TQ ? &TQ->hook : (QHOOK *) 0);
}

QHOOK *rt_qHookInit(QHOOK **_hook, void (*cancel)(void *, QBLK *), void *arg)
{
	QHOOK *hook;

	hook = rt_malloc(sizeof(QHOOK));
	if( !hook ) return 0;

	if(_hook) *_hook = hook;
	hook->head = hook->tail = 0;
	hook->cancel = cancel;
	hook->arg = arg;
	return hook;
}

void rt_qHookRelease(QHOOK *hook)
{
	if (hook) rt_free(hook);
}

void rt_qBlkBefore(QBLK *cur, QBLK *nxt)
{
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	if (cur->hook) rt_qBlkUnhook(cur);
	LOCK_TQ;
	cur->hook = nxt->hook;
	cur->nxt = nxt;
	cur->prv = nxt->prv;
	nxt->prv = cur;
	*(cur->prv ? &cur->prv->nxt : &cur->hook->head) = cur;
	UNLOCK_TQ;
}

void rt_qBlkAfter(QBLK *cur, QBLK *prv)
{
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	if (cur->hook) rt_qBlkUnhook(cur);
	LOCK_TQ;
	cur->hook = prv->hook;
	cur->prv = prv;
	cur->nxt = prv->nxt;
	prv->nxt = cur;
	*(cur->nxt ? &cur->nxt->prv : &cur->hook->tail) = cur;
	UNLOCK_TQ;
}

void rt_qBlkAtHead(QBLK *cur, QHOOK *hook)
{
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	if (cur->hook) rt_qBlkUnhook(cur);
	LOCK_TQ;
	cur->hook = hook;
	cur->prv = 0;
	cur->nxt = hook->head;
	*(cur->nxt ? &cur->nxt->prv : &hook->tail) = cur;
	hook->head = cur;
	UNLOCK_TQ;
}

void rt_qBlkAtTail(QBLK *cur, QHOOK *hook)
{
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	if (cur->hook) rt_qBlkUnhook(cur);
	LOCK_TQ;
	cur->hook = hook;
	cur->nxt = 0;
	cur->prv = hook->tail;
	*(cur->prv ? &cur->prv->nxt : &hook->head) = cur;
	hook->tail = cur;
	UNLOCK_TQ;	
}

QBLK *rt_qBlkUnhook(QBLK *cur)
{
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	LOCK_TQ;
	*(cur->nxt ? &cur->nxt->prv : &cur->hook->tail) = cur->prv;
	*(cur->prv ? &cur->prv->nxt : &cur->hook->head) = cur->nxt;
	cur->hook = 0;
	UNLOCK_TQ;

	return (cur);
}

void rt_qBlkSchedule(QBLK *cur, RTIME tic)
{
	if (cur->hook) rt_qBlkUnhook(cur);
	tqinsert(cur, tic, 0LL);
}

void rt_qHookFlush(QHOOK *hook)
{
	QBLK *cur;
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ; LOCK_TQ;

	cur = hook->head;
	if (cur) {
		do {
			RPT(cur) = TIC(cur) = 0;
			cur->hook = &TQ->hook;
		} while ((cur = cur->nxt));

		if (!(hook->tail->nxt = TQ->wait)) {
			hook->head->prv = TQ->hook.tail;
			TQ->hook.tail = hook->tail;
		} else {
			hook->head->prv = TQ->wait->prv;
			TQ->wait->prv = hook->tail;
		}
		*(hook->head->prv ? &hook->head->prv->nxt : &TQ->hook.head) = hook->head;
		hook->head = hook->tail = 0;
	}
	UNLOCK_TQ;
}

void rt_qBlkRelease(QBLK *cur)
{
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	if (cur->hook) rt_qBlkUnhook(cur);

	if (cur->lnk != NULLNK) {
		if (cur->lnk) *cur->lnk = 0;
		LOCK_TQ;
		cur->nxt = TQ->free;
		TQ->free = cur;
		UNLOCK_TQ;
	}
}

void rt_qBlkComplete(QBLK *cur)
{

	rt_qBlkRelease(cur);
	exec_func(cur->fun, cur->arg, cur->evn);
}

static void dump_qDynAlloc(void) __attribute__ ((unused));
static void dump_qDynAlloc(void) 
{
        QBLK *cur;
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	rt_printk("qBlk's: " );

	LOCK_TQ;
	cur = TQ->free;
        while (cur) {
		rt_printk("%p ", cur );
                cur = cur->nxt;
                }
        UNLOCK_TQ;

	rt_printk("\n" );
}

unsigned rt_qDynAlloc(unsigned int n)
{
	QBLK *cur;
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	while (n && (cur = (QBLK *)rt_malloc(sizeof(QBLK)))) {
		memset(cur, 0, sizeof(QBLK));
		LOCK_TQ;
		cur->nxt = TQ->free;
		TQ->free = cur;
		UNLOCK_TQ;
		n--;
	}

	return (n);
}

void rt_qCleanup(RT_TASK *rtt)
{
        QBLK *cur;
        unsigned long flags;
	struct TickQueue *TQ;

	TQ=(struct TickQueue *)(rtt->tick_queue);
	if (!TQ) return;

	LOCK_TQ;
	while (TQ->free) {
		cur = TQ->free;
		TQ->free = cur->nxt;
		rt_free(cur);
	}

	while (TQ->wait) {
		cur = TQ->wait;
		TQ->wait = cur->nxt;
		rt_free(cur);
	}
	UNLOCK_TQ;
	rtt->tick_queue = (void *)0;
	rt_free(TQ);
}

unsigned rt_qDynFree(int n)
{
	QBLK *cur;
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	while (n && TQ->free) {
		LOCK_TQ;
		cur = TQ->free;
		TQ->free = cur->nxt;
		UNLOCK_TQ;
		rt_free(cur);
		n--;
	}

	return (n);
}

QBLK *rt_qDynInit(QBLK **lnk, void (*fun)(void *, int), void *arg, int evn)
{
	QBLK *cur;
        unsigned long flags; DECLARE_TQ; ASSIGN_TQ;

	if (!TQ) return (QBLK *)0;

	LOCK_TQ;
        cur = TQ->free;

	if (cur) {
		TQ->free = cur->nxt;
		UNLOCK_TQ;
	} else {
		UNLOCK_TQ;
		if (!(cur = (QBLK *) rt_malloc(sizeof(QBLK)))) return 0;
	}

	cur->hook = (QHOOK *) 0;
	cur->lnk  = lnk;
	cur->fun  = fun;
	cur->arg  = arg;
	cur->evn  = evn;

	if (lnk) *lnk = cur;

	return (cur);
}

QBLK *rt_qBlkInit(QBLK *cur, void (*fun)(void *, int ), void *arg, int evn)
{
	cur->hook = 0;
	cur->lnk = NULLNK;
	cur->fun = fun;
	cur->arg = arg;
	cur->evn = evn;

	return (cur);
}

void rt_qBlkDequeue(QBLK *cur) 
{
	void *fun;

	if (cur->hook) {
		if (cur->hook->cancel) { // usually tqcancel()
			fun = cur->hook->cancel;
                        (*cur->hook->cancel)(cur->hook->arg, cur);

		}
		rt_qBlkUnhook(cur);
	}
}

void rt_qBlkWait(QBLK *cur, RTIME tic)
{
	if (cur->hook) rt_qBlkDequeue(cur);
	tqinsert(cur, tic, 0LL);
}

void rt_qBlkRepeat(QBLK *cur, RTIME tic)
{
	if (cur->hook) rt_qBlkDequeue(cur);
	tqinsert(cur, tic, 1LL);
}

void rt_qBlkSoon(QBLK *cur)
{
        DECLARE_TQ; ASSIGN_TQ;

	if (cur->hook) rt_qBlkDequeue(cur);
	rt_qBlkAtHead(cur, &TQ->hook);
	RPT(cur) = TIC(cur) = 0;
}

void rt_qBlkCancel(QBLK *cur) 
{
	if (cur->hook) rt_qBlkDequeue(cur);
	rt_qBlkRelease(cur);
}

int rt_qSync(void)
{
DECLARE_TQ; ASSIGN_TQ; 
// Complete (execute and release) all expired QBLKs in the Tick Queue.
// QBLKs that expire during this process will also be completed.

do {
	while (TQ->hook.head != TQ->wait) tqcomplete(TQ->hook.head);
	} while (TQ->wait && tqadjust());

return (TQ->wait != 0); // TRUE if queue not empty on exit.
}

void rt_qLoop(void)
{
DECLARE_TQ; ASSIGN_TQ;
// Execute and wait until the queue is empty.

while (rt_qSync()) {
	rt_sleep(TIC(TQ->wait));
	}
}

RTIME rt_qStep(void)
{
DECLARE_TQ; ASSIGN_TQ;
// Single step through the queue.
if (!TQ->hook.head) {
	return (-1LL); // The queue is empty.
} else if ((TQ->hook.head != TQ->wait) || tqadjust()) {
	tqcomplete(TQ->hook.head);
	return (0LL); // There may be others to dequeue now.
} else {
	return (TIC(TQ->wait)); // Time to wait before the next one.
	}
}

pid_t rt_qReceive(pid_t target, void *buf, size_t maxlen, size_t *msglen)
{
pid_t tid;
DECLARE_TQ; ASSIGN_TQ;

	// Execute and wait until a message or a proxy is received from anywhere if target is zero.
	// Execute and wait until a message or a proxy is received from a specific target.
	while (!(tid = rt_Creceive( target, buf, maxlen, msglen, 0))) {
		if (TQ->hook.head != TQ->wait) {
			tqcomplete(TQ->hook.head);
		} else if (!TQ->wait) {
			// wait forever
			return(rt_Receive(target, buf, maxlen, msglen));
		} else if (tqadjust()) {
			/* NOOP */
		} else {
			tid = rt_Creceive( target, buf, maxlen, msglen, TIC(TQ->wait));
			if (tid) return tid ;
		}	
	}

	return tid ;
}
