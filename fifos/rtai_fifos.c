/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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

/* 
ACKNOWLEDGEMENTS: 
- nice proc file contributed by Steve Papacharalambous (stevep@zentropix.com);
- added proc handler info contributed by Rich Walker (rw@shadow.org.uk)
- 11-19-2001, Truxton Fulton (trux@truxton.com) fixed a race in mbx_get.
- 11-29-2003 added atomic send contributed by Jan Kiszka
  (kiszka@rts.uni-hannover.de) and expanded it to rtf_get_if.
*/

/* 
ACKNOWLEDGEMENT NOTE: besides naming conventions and the idea of a fifo handler
function, the only remaining code from RTL original fifos, as written and 
copyrighted by Michael Barabanov, should be the function "check_blocked" 
(modified to suite my style). However I like to remark that I owe to that code 
my first understanding of Linux devices drivers (Paolo Mantegazza).
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/tty_driver.h>
#include <linux/console.h>
#include <linux/config.h>
#include <linux/slab.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/stat.h>
#include <linux/proc_fs.h>

#include <rtai_fifos.h>
#include <rtai_trace.h>
#include <rtai_proc_fs.h>
#include <rtai_sched.h>

MODULE_LICENSE("GPL");

#define  CONFIG_RTAI_RTF_NAMED

/* these are copied from <rt/rt_compat.h> */
#define rtf_save_flags_and_cli(x)	do{x=rt_spin_lock_irqsave(&rtf_lock);}while(0)
#define rtf_restore_flags(x)		rt_spin_unlock_irqrestore((x),&rtf_lock)
#define rtf_spin_lock_irqsave(x,y)	do{x=rt_spin_lock_irqsave(&(y));}while(0)
#define rtf_spin_unlock_irqrestore(x,y)	rt_spin_unlock_irqrestore((x),&(y))
#define rtf_request_srq(x)		rt_request_srq(0, (x), 0)
#define rtf_free_srq(x)			rt_free_srq((x))
#define rtf_pend_srq(x)			rt_pend_linux_srq((x))

#ifdef CONFIG_PROC_FS
static int rtai_proc_fifo_register(void);
static void rtai_proc_fifo_unregister(void);
#endif

typedef struct lx_queue {
	struct lx_queue *prev;
	struct lx_queue *next;
	struct lx_task_struct *task;
} F_QUEUE;

typedef struct lx_semaphore {
	int free;
	int qtype;
	F_QUEUE queue;
} F_SEM;

typedef struct lx_task_struct {
	int blocked;
	int priority;
	F_QUEUE queue;
	struct task_struct *task;
} LX_TASK;

typedef struct lx_mailbox {
	int size;   // size of the entire buffer
	int fbyte;  // head
	int lbyte;  // tail
	int avbs;   // bytes available in the buffer
	int frbs;   // free bytes in the buffer
	char *bufadr;
	F_SEM sndsem, rcvsem;
	struct task_struct *waiting_task;
	spinlock_t buflock;
} F_MBX;

typedef struct rt_fifo_struct {
	F_MBX mbx;		// MUST BE THE FIRST!
	int opncnt;
	int malloc_type;
	int pol_asyn_pended;
	wait_queue_head_t pollq;
	struct fasync_struct *asynq;
	int (*handler)(unsigned int arg);
	F_SEM sem;
#ifdef CONFIG_RTAI_RTF_NAMED
	char name[RTF_NAMELEN+1];
#endif
} FIFO;

#if LINUX_EXT_VERSION_CODE < KERNEL_EXT_VERSION(2,4,0,8)
static inline void wake_up_process(struct task_struct *p)
{
	struct wait_queue *waitq;
	struct wait_queue linux_task;

	waitq = 0;
	linux_task.task = p;
	 __add_wait_queue(&waitq, &linux_task);
	wake_up_interruptible(&waitq);
}
#endif

static int fifo_srq, async_sig;
static spinlock_t rtf_lock = SPIN_LOCK_UNLOCKED;
#ifdef CONFIG_RTAI_RTF_NAMED
static spinlock_t rtf_name_lock = SPIN_LOCK_UNLOCKED;
#endif

#define MAX_FIFOS 64
//static FIFO fifo[MAX_FIFOS] = {{{0}}};
static FIFO *fifo;

#define MAXREQS 64	// KEEP IT A POWER OF 2!!!
static struct { int in, out; struct task_struct *task[MAXREQS]; } taskq;
static struct { int in, out; FIFO *fifo[MAXREQS]; } pol_asyn_q;

static int do_nothing(unsigned int arg) { return 0; }

static inline int check_current_blocked(void)
{
	int i;
	sigset_t signal, blocked;

#if LINUX_EXT_VERSION_CODE < KERNEL_EXT_VERSION(2,4,0,8)
	signal = current->signal;
#else
	signal = current->pending.signal;
#endif
	blocked = current->blocked;

	for (i = 0; i < _NSIG_WORDS; i++) {
		if (signal.sig[i] & ~blocked.sig[i]) {
			return 1;
		}
	}
	return 0;
}

static inline void enqueue_blocked(LX_TASK *task, F_QUEUE *queue, int qtype, int priority)
{
	F_QUEUE *q;

	task->blocked = 1;
	q = queue;
	if (!qtype) {
		while ((q = q->next) != queue && (q->task)->priority >= priority);
	}
	q->prev = (task->queue.prev = q->prev)->next  = &(task->queue);
	task->queue.next = q;
}

static inline void dequeue_blocked(LX_TASK *task)
{
	task->blocked = 0;
	(task->queue.prev)->next = task->queue.next;
	(task->queue.next)->prev = task->queue.prev;
}

static inline void mbx_sem_signal(F_SEM *sem, FIFO *fifop)
{
	unsigned long flags;
	LX_TASK *task;

	rtf_save_flags_and_cli(flags);
	if ((task = (sem->queue.next)->task)) {
		dequeue_blocked(task);
		taskq.task[taskq.in] = task->task;
		taskq.in = (taskq.in + 1) & (MAXREQS - 1);
		rtf_pend_srq(fifo_srq);
	} else {
		sem->free = 1;
		if (fifop && !(fifop->pol_asyn_pended) &&
		    (((F_MBX *)fifop)->avbs || ((F_MBX *)fifop)->frbs) &&
		    (waitqueue_active(&fifop->pollq) || fifop->asynq)) {
			fifop->pol_asyn_pended = 1;
			pol_asyn_q.fifo[pol_asyn_q.in] = fifop;
			pol_asyn_q.in = (pol_asyn_q.in + 1) & (MAXREQS - 1);
			rtf_pend_srq(fifo_srq);
		}
	}
	rtf_restore_flags(flags);
	return;
}

static inline void mbx_signal(F_MBX *mbx)
{
	unsigned long flags;
	struct task_struct *task;

	rtf_save_flags_and_cli(flags);
	if ((task = mbx->waiting_task)) {
		mbx->waiting_task = 0;
		taskq.task[taskq.in] = task;
		taskq.in = (taskq.in + 1) & (MAXREQS - 1);
		rtf_pend_srq(fifo_srq);
	}
	rtf_restore_flags(flags);
	return;
}

static inline int mbx_sem_wait_if(F_SEM *sem)
{
	unsigned long flags;

	rtf_save_flags_and_cli(flags);
	if (sem->free) {
		sem->free = 0;
		rtf_restore_flags(flags);
		return 1;
	}
	rtf_restore_flags(flags);
	return 0;
}

static inline int mbx_sem_wait(F_SEM *sem)
{
	unsigned long flags;
	LX_TASK task;
	int ret;

	ret = 0;
	rtf_save_flags_and_cli(flags);
	if (!sem->free) {
		task.queue.task = &task;
		task.priority = current->rt_priority;
		enqueue_blocked(&task, &sem->queue, sem->qtype, task.priority);
		task.task = current;
		rtf_restore_flags(flags);
		current->state = TASK_INTERRUPTIBLE;
		schedule();
		if (check_current_blocked()) {
			ret = -ERESTARTSYS;
		}
		rtf_save_flags_and_cli(flags);
		if (task.blocked) { 
			dequeue_blocked(&task);
			if (!(sem->queue.next)->task) {
				sem->free = 1;
			}
			rtf_restore_flags(flags);
			if (!ret) {
				ret = -1;
			}
		}
	} else {
		sem->free = 0;
	}
	rtf_restore_flags(flags);
	return ret;
}

static inline int mbx_wait(F_MBX *mbx, int *fravbs)
{
	unsigned long flags;

	rtf_save_flags_and_cli(flags);
	if (!(*fravbs)) {
		mbx->waiting_task = current;
		current->state = TASK_INTERRUPTIBLE;
		rtf_restore_flags(flags);
		schedule();
		if (check_current_blocked()) {
			return -ERESTARTSYS;
		}
		rtf_save_flags_and_cli(flags);
		if (mbx->waiting_task == current) {
			mbx->waiting_task = 0;
			rtf_restore_flags(flags);
			return -1;
		}
	}
	rtf_restore_flags(flags);
	return 0;
}

static inline int mbx_sem_wait_timed(F_SEM *sem, int delay)
{
	unsigned long flags;
	LX_TASK task;

	rtf_save_flags_and_cli(flags);
	if (!sem->free) {
		task.queue.task = &task;
		task.priority = current->rt_priority;
		enqueue_blocked(&task, &sem->queue, sem->qtype, task.priority);
		task.task = current;
		rtf_restore_flags(flags);
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(delay);
		if (check_current_blocked()) {
			return -ERESTARTSYS;
		}
		rtf_save_flags_and_cli(flags);
		if (task.blocked) { 
			dequeue_blocked(&task);
			if (!((sem->queue.next)->task)) {
				sem->free = 1;
			}
			rtf_restore_flags(flags);
			return -1;
		}
	} else {
		sem->free = 0;
	}
	rtf_restore_flags(flags);
	return 0;
}

static inline int mbx_wait_timed(F_MBX *mbx, int *fravbs, int delay)
{
	unsigned long flags;

	rtf_save_flags_and_cli(flags);
	if (!(*fravbs)) {
		mbx->waiting_task = current;
		rtf_restore_flags(flags);
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(delay);
		if (check_current_blocked()) {
			return -ERESTARTSYS;
		}
		rtf_save_flags_and_cli(flags);
		if (mbx->waiting_task == current) {;
			mbx->waiting_task = 0;
			rtf_restore_flags(flags);
			return -1;
		}
	}
	rtf_restore_flags(flags);
	return 0;
}

#define MOD_SIZE(indx) ((indx) < mbx->size ? (indx) : (indx) - mbx->size)

static inline int mbx_put(F_MBX *mbx, char **msg, int msg_size, int lnx)
{
	unsigned long flags;
	int tocpy;

	while (msg_size > 0 && mbx->frbs) {
		if ((tocpy = mbx->size - mbx->lbyte) > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > mbx->frbs) {
			tocpy = mbx->frbs;
		}
		if (lnx) {
			copy_from_user(mbx->bufadr + mbx->lbyte, *msg, tocpy);
		} else {
			memcpy(mbx->bufadr + mbx->lbyte, *msg, tocpy);
		}
		rtf_spin_lock_irqsave(flags, mbx->buflock);
		mbx->lbyte = MOD_SIZE(mbx->lbyte + tocpy);
		mbx->frbs -= tocpy;
		mbx->avbs += tocpy;
		rtf_spin_unlock_irqrestore(flags, mbx->buflock);
		msg_size -= tocpy;
		*msg     += tocpy;
	}
	return msg_size;
}

static inline int mbx_ovrwr_put(F_MBX *mbx, char **msg, int msg_size, int lnx)
{
	unsigned long flags;
	int tocpy,n;

	if ((n = msg_size - mbx->size) > 0) {
		*msg += n;
		msg_size -= n;
	}		
	while (msg_size > 0) {
		if (mbx->frbs) {	
			if ((tocpy = mbx->size - mbx->lbyte) > msg_size) {
				tocpy = msg_size;
			}
			if (tocpy > mbx->frbs) {
				tocpy = mbx->frbs;
			}
			if (lnx) {
				copy_from_user(mbx->bufadr + mbx->lbyte, *msg, tocpy);
			} else {
				memcpy(mbx->bufadr + mbx->lbyte, *msg, tocpy);
			}
			rtf_spin_lock_irqsave(flags, mbx->buflock);
			mbx->frbs -= tocpy;
			mbx->avbs += tocpy;
			rtf_spin_unlock_irqrestore(flags, mbx->buflock);
			msg_size -= tocpy;
			*msg     += tocpy;
			mbx->lbyte = MOD_SIZE(mbx->lbyte + tocpy);
		}	
		if (msg_size) {
			while ((n = msg_size - mbx->frbs) > 0) {
				if ((tocpy = mbx->size - mbx->fbyte) > n) {
					tocpy = n;
				}
				if (tocpy > mbx->avbs) {
					tocpy = mbx->avbs;
				}
				rtf_spin_lock_irqsave(flags, mbx->buflock);
				mbx->frbs  += tocpy;
				mbx->avbs  -= tocpy;
				rtf_spin_unlock_irqrestore(flags, mbx->buflock);
				mbx->fbyte = MOD_SIZE(mbx->fbyte + tocpy);
			}
		}		
	}
	return 0;
}

static inline int mbx_get(F_MBX *mbx, char **msg, int msg_size, int lnx)
{
	unsigned long flags;
	int tocpy;

	while (msg_size > 0 && mbx->avbs) {
		if ((tocpy = mbx->size - mbx->fbyte) > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > mbx->avbs) {
			tocpy = mbx->avbs;
		}
		if (lnx) {
			copy_to_user(*msg, mbx->bufadr + mbx->fbyte, tocpy);
		} else {
			memcpy(*msg, mbx->bufadr + mbx->fbyte, tocpy);
		}
		rtf_spin_lock_irqsave(flags, mbx->buflock);
		mbx->fbyte = MOD_SIZE(mbx->fbyte + tocpy);
		mbx->frbs += tocpy;
		mbx->avbs -= tocpy;
		rtf_spin_unlock_irqrestore(flags, mbx->buflock);
		msg_size  -= tocpy;
		*msg      += tocpy;
	}
	return msg_size;
}

static inline int mbx_evdrp(F_MBX *mbx, char **msg, int msg_size, int lnx)
{
	int tocpy, fbyte, avbs;

	fbyte = mbx->fbyte;
	avbs  = mbx->avbs;
	while (msg_size > 0 && avbs) {
		if ((tocpy = mbx->size - fbyte) > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > avbs) {
			tocpy = avbs;
		}
		if (lnx) {
			copy_to_user(*msg, mbx->bufadr + fbyte, tocpy);
		} else {
			memcpy(*msg, mbx->bufadr + fbyte, tocpy);
		}
		avbs     -= tocpy;
		msg_size -= tocpy;
		*msg     += tocpy;
		fbyte = MOD_SIZE(fbyte + tocpy);
	}
	return msg_size;
}

static inline void mbx_sem_init(F_SEM *sem, int value)
{
	sem->free  = value;
	sem->qtype = 0;
	sem->queue.prev = &(sem->queue);
	sem->queue.next = &(sem->queue);
	sem->queue.task = 0;
}

static inline int mbx_sem_delete(F_SEM *sem)
{
	unsigned long flags;
	LX_TASK *task;

	rtf_save_flags_and_cli(flags);
	while ((task = (sem->queue.next)->task)) {
		sem->queue.next = task->queue.next;
		(task->queue.next)->prev = &(sem->queue);
		taskq.task[taskq.in] = task->task;
		taskq.in = (taskq.in + 1) & (MAXREQS - 1);
		rtf_pend_srq(fifo_srq);
	}
	rtf_restore_flags(flags);
	return 0;
}

static inline void mbx_init(F_MBX *mbx, int size, char *bufadr)
{
	mbx_sem_init(&(mbx->sndsem), 1);
	mbx_sem_init(&(mbx->rcvsem), 1);
	mbx->waiting_task = 0;
	mbx->bufadr = bufadr;
	mbx->size = mbx->frbs = size;
	mbx->fbyte = mbx->lbyte = mbx->avbs = 0;
#ifdef CONFIG_SMP
        mbx->buflock.lock = 0;
#endif
	spin_lock_init(&(mbx->buflock));
}

static inline int mbx_delete(F_MBX *mbx)
{
	mbx_signal(mbx);
	if (mbx_sem_delete(&(mbx->sndsem)) || mbx_sem_delete(&(mbx->rcvsem))) {
		return -EFAULT;
	}
	return 0;
}

static inline int mbx_send(F_MBX *mbx, void *msg, int msg_size, int lnx)
{
	if (mbx_sem_wait(&(mbx->sndsem))) {
		return msg_size;
	}
	while (msg_size) {
		if (mbx_wait(mbx, &mbx->frbs)) {
			mbx_sem_signal(&(mbx->sndsem), (FIFO *)mbx);
			return msg_size;
		}
		msg_size = mbx_put(mbx, (char **)(&msg), msg_size, lnx);
		mbx_signal(mbx);
	}
	mbx_sem_signal(&(mbx->sndsem), (FIFO *)mbx);
	return 0;
}

static inline int mbx_send_wp(F_MBX *mbx, void *msg, int msg_size, int lnx)
{
	unsigned long flags;

	rtf_save_flags_and_cli(flags);
	if (mbx->sndsem.free && mbx->frbs) {
		mbx->sndsem.free = 0;
		rtf_restore_flags(flags);
		msg_size = mbx_put(mbx, (char **)(&msg), msg_size, lnx);
		mbx_signal(mbx);
		mbx_sem_signal(&(mbx->sndsem), (FIFO *)mbx);
	} else {
		rtf_restore_flags(flags);
	}
	return msg_size;
}

static inline int mbx_send_if(F_MBX *mbx, void *msg, int msg_size, int lnx)
{
	unsigned long flags;
 
 	rtf_save_flags_and_cli(flags);
 	if (mbx->sndsem.free && (mbx->frbs >= msg_size)) {
 		mbx->sndsem.free = 0;
 		rtf_restore_flags(flags);
 		msg_size = mbx_put(mbx, (char **)(&msg), msg_size, lnx);
 		mbx_signal(mbx);
 		mbx_sem_signal(&(mbx->sndsem), (FIFO *)mbx);
 	} else {
 		rtf_restore_flags(flags);
 	}
 	return msg_size;
}

static int mbx_send_timed(F_MBX *mbx, void *msg, int msg_size, int delay, int lnx)
{
	if (mbx_sem_wait_timed(&(mbx->sndsem), delay)) {
		return msg_size;
	}
	while (msg_size) {
		if (mbx_wait_timed(mbx, &(mbx->frbs), delay)) {
			mbx_sem_signal(&(mbx->sndsem), (FIFO *)mbx);
			return msg_size;
		}
		msg_size = mbx_put(mbx, (char **)(&msg), msg_size, lnx);
		mbx_signal(mbx);
	}
	mbx_sem_signal(&(mbx->sndsem), (FIFO *)mbx);
	return 0;
}

static inline int mbx_receive(F_MBX *mbx, void *msg, int msg_size, int lnx)
{
	if (mbx_sem_wait(&(mbx->rcvsem))) {
		return msg_size;
	}
	while (msg_size) {
		if (mbx_wait(mbx, &mbx->avbs)) {
			mbx_sem_signal(&(mbx->rcvsem), (FIFO *)mbx);
			return msg_size;
		}
		msg_size = mbx_get(mbx, (char **)(&msg), msg_size, lnx);
		mbx_signal(mbx);
	}
	mbx_sem_signal(&(mbx->rcvsem), (FIFO *)mbx);
	return 0;
}

static inline int mbx_receive_wjo(F_MBX *mbx, void *msg, int msg_size, int lnx)
{
	if (mbx_sem_wait(&(mbx->rcvsem))) {
		return msg_size;
	}
	if (msg_size) {
		if (mbx_wait(mbx, &mbx->avbs)) {
			mbx_sem_signal(&(mbx->rcvsem), (FIFO *)mbx);
			return msg_size;
		}
		msg_size = mbx_get(mbx, (char **)(&msg), msg_size, lnx);
		mbx_signal(mbx);
	}
	mbx_sem_signal(&(mbx->rcvsem), (FIFO *)mbx);
	return msg_size;
}

static inline int mbx_receive_wp(F_MBX *mbx, void *msg, int msg_size, int lnx)
{
	unsigned long flags;

	rtf_save_flags_and_cli(flags);
	if (mbx->rcvsem.free && mbx->avbs) {
		mbx->rcvsem.free = 0;
		rtf_restore_flags(flags);
		msg_size = mbx_get(mbx, (char **)(&msg), msg_size, lnx);
		mbx_signal(mbx);
		mbx_sem_signal(&(mbx->rcvsem), (FIFO *)mbx);
	} else {
		rtf_restore_flags(flags);
	}
	return msg_size;
}

static inline int mbx_receive_if(F_MBX *mbx, void *msg, int msg_size, int lnx)
{
 	unsigned long flags;
 
 	rtf_save_flags_and_cli(flags);
 	if (mbx->rcvsem.free && (mbx->frbs >= msg_size)) {
 		mbx->rcvsem.free = 0;
 		rtf_restore_flags(flags);
 		msg_size = mbx_get(mbx, (char **)(&msg), msg_size, lnx);
 		mbx_signal(mbx);
 		mbx_sem_signal(&(mbx->rcvsem), (FIFO *)mbx);
 	} else {
 		rtf_restore_flags(flags);
 	}
 	return msg_size;
}
 
static int mbx_receive_timed(F_MBX *mbx, void *msg, int msg_size, int delay, int lnx)
{
	if (mbx_sem_wait_timed(&(mbx->rcvsem), delay)) {
		return msg_size;
	}
	while (msg_size) {
		if (mbx_wait_timed(mbx, &(mbx->avbs), delay)) {
			mbx_sem_signal(&(mbx->rcvsem), (FIFO *)mbx);
			return msg_size;
		}
		msg_size = mbx_get(mbx, (char **)(&msg), msg_size, lnx);
		mbx_signal(mbx);
	}
	mbx_sem_signal(&(mbx->rcvsem), (FIFO *)mbx);
	return 0;
}

static inline int mbx_ovrwr_send(F_MBX *mbx, void *msg, int msg_size, int lnx)
{
	unsigned long flags;

	rtf_save_flags_and_cli(flags);
	if (mbx->sndsem.free) {
		mbx->sndsem.free = 0;
		rtf_restore_flags(flags);
		msg_size = mbx_ovrwr_put(mbx, (char **)(&msg), msg_size, lnx);
		mbx_signal(mbx);
		mbx_sem_signal(&(mbx->sndsem), (FIFO *)mbx);
	} else {
		rtf_restore_flags(flags);
	}
	return msg_size;
}

#ifdef CONFIG_RTL
static void rtf_sysrq_handler(int irq, void *dev_id, struct pt_regs *p)
#else
static void rtf_sysrq_handler(void)
#endif
{
	FIFO *fifop;
	while (taskq.out != taskq.in) {
		if (taskq.task[taskq.out]->state == TASK_INTERRUPTIBLE) {
			wake_up_process(taskq.task[taskq.out]);
		}
		taskq.out = (taskq.out + 1) & (MAXREQS - 1);
	}

	while (pol_asyn_q.out != pol_asyn_q.in) {
		fifop = pol_asyn_q.fifo[pol_asyn_q.out];
		fifop->pol_asyn_pended = 0;
		if (waitqueue_active(&(fifop = pol_asyn_q.fifo[pol_asyn_q.out])->pollq)) {
			wake_up_interruptible(&(fifop->pollq));
		}
		if (fifop->asynq) { 
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
			kill_fasync(fifop->asynq, POLL_IN);
#elif LINUX_EXT_VERSION_CODE < KERNEL_EXT_VERSION(2,4,0,4) /* XXX */
			kill_fasync(fifop->asynq, async_sig, POLL_IN);
#else
			kill_fasync(&fifop->asynq, async_sig, POLL_IN);
#endif
		}
		pol_asyn_q.out = (pol_asyn_q.out + 1) & (MAXREQS - 1);
	}
	current->need_resched = 1;
}

#define VALID_FIFO	if (minor >= MAX_FIFOS) { return -ENODEV; } \
			if (!(fifo[minor].opncnt)) { return -EINVAL; }

int rtf_reset(unsigned int minor)
{
	F_MBX *mbx;
	VALID_FIFO;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_RESET, minor, 0);

	if (mbx_delete(mbx = &(fifo[minor].mbx))) {
		return -EFAULT;
	}
	mbx_init(mbx, mbx->size, mbx->bufadr);
	return 0;
}

int rtf_resize(unsigned int minor, int size)
{
	void *bufadr, *msg;
	int malloc_type;
	F_MBX *mbx;
	
	VALID_FIFO;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_RESIZE, minor, size);

	malloc_type = fifo[minor].malloc_type;
	if (size <= PAGE_SIZE*32) {
		if (!(bufadr = kmalloc(size, GFP_KERNEL))) {
			return -ENOMEM;
		}
		fifo[minor].malloc_type = 'k';
	} else {
		if (!(bufadr = vmalloc(size))) {
			return -ENOMEM;
		}
		fifo[minor].malloc_type = 'v';
	}
	msg = bufadr;
	mbx = &(fifo[minor].mbx);
	mbx->avbs = 1000000000 - mbx_get(mbx, (char **)&msg, 1000000000, 0);
	if (malloc_type == 'k') {
		kfree(fifo[minor].mbx.bufadr); 
	} else {
		vfree(fifo[minor].mbx.bufadr); 
	}
	mbx->bufadr = bufadr;
	mbx->size = size > mbx->size ? size : mbx->size;
	mbx->fbyte = 0;
	mbx->frbs = mbx->size - mbx->avbs;
	return size;
}

int rtf_create(unsigned int minor, int size)
{
extern unsigned long linux_save_flags_and_cli_cpuid(int);
extern void rtai_just_copy_back(unsigned long, int);

	void *buf;
	unsigned long flags, lflags;

	if (minor >= MAX_FIFOS) {
		return -ENODEV;
	}
	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_CREATE, minor, size);
	lflags = linux_save_flags_and_cli_cpuid(hard_cpu_id());
	rtf_save_flags_and_cli(flags);
	if (!(fifo[minor].opncnt)) {
		if (size <= PAGE_SIZE*32) {
			if (!(buf = kmalloc(size, GFP_KERNEL))) {
				rtf_restore_flags(flags);
				return -ENOMEM;
			}
			fifo[minor].malloc_type = 'k';
		} else {
			if (!(buf = vmalloc(size))) {
				rtf_restore_flags(flags);
				return -ENOMEM;
			}
			fifo[minor].malloc_type = 'v';
		}
		fifo[minor].handler = do_nothing;
		mbx_init(&(fifo[minor].mbx), size, buf);
		mbx_sem_init(&(fifo[minor].sem), 0);
		fifo[minor].pol_asyn_pended = 0;
		fifo[minor].asynq = 0;
#ifdef CONFIG_RTAI_RTF_NAMED
		fifo[minor].name[0] = 0;
#endif
	} else {
		if (size > fifo[minor].mbx.size) {
			rtf_resize(minor, size);
		}
	}
	MOD_INC_USE_COUNT;
	(fifo[minor].opncnt)++;
	rtf_restore_flags(flags);
	rtai_just_copy_back(lflags, hard_cpu_id());
	return 0;
}

int rtf_destroy(unsigned int minor)
{
	VALID_FIFO;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_DESTROY, minor, 0);

	MOD_DEC_USE_COUNT;
	(fifo[minor].opncnt)--;
	if(!(fifo[minor].opncnt)) {
		if (fifo[minor].malloc_type == 'k') {
			kfree(fifo[minor].mbx.bufadr); 
		} else {
			vfree(fifo[minor].mbx.bufadr); 
		}
		fifo[minor].handler = do_nothing;
		mbx_delete(&(fifo[minor].mbx));
		fifo[minor].pol_asyn_pended = 0;
		fifo[minor].asynq = 0;
#ifdef CONFIG_RTAI_RTF_NAMED
		fifo[minor].name[0] = 0;
#endif
	}
	return fifo[minor].opncnt;
}

int rtf_create_handler(unsigned int minor, int (*handler) (unsigned int fifo))
{
	if (minor >= MAX_FIFOS || !handler) {
		return -EINVAL;
	}

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_CREATE_HANDLER, minor, handler);

	fifo[minor].handler = handler;
	return 0;
}

int rtf_put(unsigned int minor, void *buf, int count)
{
	VALID_FIFO;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_PUT, minor, count);

	count -= mbx_send_wp(&(fifo[minor].mbx), buf, count, 0);
	return count;
}

int rtf_ovrwr_put(unsigned int minor, void *buf, int count)
{
	VALID_FIFO;
	return mbx_ovrwr_send(&(fifo[minor].mbx), buf, count, 0);
}

int rtf_put_if(unsigned int minor, void *buf, int count)
{
 	VALID_FIFO;
 
 	count -= mbx_send_if(&(fifo[minor].mbx), buf, count, 0);
 	return count;
}

int rtf_get(unsigned int minor, void *buf, int count)
{
	VALID_FIFO;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_GET, minor, count);

	count -= mbx_receive_wp(&(fifo[minor].mbx), buf, count, 0);
	return count;
}

int rtf_evdrp(unsigned int minor, void *msg, int msg_size)
{
	VALID_FIFO;

	return msg_size - mbx_evdrp(&(fifo[minor].mbx), (char **)(&msg), msg_size, 0);
}

int rtf_get_if(unsigned int minor, void *buf, int count)
{
 	VALID_FIFO;
 
 	return count - mbx_send_if(&(fifo[minor].mbx), buf, count, 0);
}

int rtf_sem_init(unsigned int minor, int value)
{
	VALID_FIFO;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_INIT, minor, value);

	mbx_sem_init(&(fifo[minor].sem), value);
	return 0;
}

int rtf_sem_post(unsigned int minor)
{
	VALID_FIFO;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_POST, minor, 0);

	mbx_sem_signal(&(fifo[minor].sem), 0);
	return 0;
}

int rtf_sem_trywait(unsigned int minor)
{
	VALID_FIFO;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_TRY_WAIT, minor, 0);

	return mbx_sem_wait_if(&(fifo[minor].sem));
}

int rtf_sem_delete(unsigned int minor)
{
	VALID_FIFO;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_DESTROY, minor, 0);

	return mbx_sem_delete(&(fifo[minor].sem));
}

static int rtf_open(struct inode *inode, struct file *filp)
{
#define DEFAULT_SIZE 1000
	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_OPEN, MINOR(inode->i_rdev), DEFAULT_SIZE);

	return rtf_create(MINOR(inode->i_rdev), DEFAULT_SIZE);
}

static int rtf_fasync(int fd, struct file *filp, int mode)
{	
	int minor;
	minor = MINOR((filp->f_dentry->d_inode)->i_rdev);

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_FASYNC, minor, fd);

	return fasync_helper(fd, filp, mode, &(fifo[minor].asynq));
	if (!mode) {
		fifo[minor].asynq = 0;
	}
}

static int rtf_release(struct inode *inode, struct file *filp)
{
	int minor;
	minor = MINOR(inode->i_rdev);

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_RELEASE, minor, 0);

	if (waitqueue_active(&(fifo[minor].pollq))) {
		wake_up_interruptible(&(fifo[minor].pollq));
	}
	rtf_fasync(-1, filp, 0);
	current->need_resched = 1;
	return rtf_destroy(minor);
}

static ssize_t rtf_read(struct file *filp, char *buf, size_t count, loff_t* ppos)
{
	struct inode *inode = filp->f_dentry->d_inode;
	unsigned int minor = MINOR(inode->i_rdev);
	int handler_ret;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_READ, minor, count);

	if (filp->f_flags & O_NONBLOCK) {
		count -= mbx_receive_wp(&(fifo[minor].mbx), buf, count, 1);
		if (!count) {
			return -EAGAIN;
		}
	} else {
		count -= mbx_receive_wjo(&(fifo[minor].mbx), buf, count, 1);
	}

	if (count) {
		inode->i_atime = CURRENT_TIME;
		if ((handler_ret = ((int (*)(int, ...))(fifo[minor].handler))(minor, 'r')) < 0) {
			return handler_ret;
		}
		return count;
	}
	return 0;

	return count;
}

static ssize_t rtf_write(struct file *filp, const char *buf, size_t count, loff_t* ppos)
{
	struct inode *inode = filp->f_dentry->d_inode;
	unsigned int minor = MINOR(inode->i_rdev);
	int handler_ret;

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_WRITE, minor, count);

	if (filp->f_flags & O_NONBLOCK) {
		count -= mbx_send_wp(&(fifo[minor].mbx), (char *)buf, count, 1);
		if (!count) {
			return -EAGAIN;
		}
	} else {
		count -= mbx_send(&(fifo[minor].mbx), (char *)buf, count, 1);
	}

	inode->i_ctime = inode->i_mtime = CURRENT_TIME;
	if ((handler_ret = ((int (*)(int, ...))(fifo[minor].handler))(minor, 'w')) < 0) {
		return handler_ret;
	}

	return count;
}

#define DELAY(x) (((x)*HZ + 500)/1000)

static int rtf_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{	
	unsigned int minor;
	FIFO *fifop;

	fifop = fifo + (minor = MINOR(inode->i_rdev));

	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_IOCTL, minor, cmd);

	switch(cmd) {
		case RESET: {
			return rtf_reset(minor);
		}
		case RESIZE: {
			return rtf_resize(minor, arg);
		}
		case SUSPEND_TIMED: {
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SUSPEND_TIMED, DELAY(arg), 0);
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(DELAY(arg));
			if (check_current_blocked()) {
				return -ERESTARTSYS;
			}
			return 0;
		}
		case OPEN_SIZED: {
			return rtf_create(minor, arg);
		}
		case READ_ALL_AT_ONCE: {
			struct { char *buf; int count; } args;
			int handler_ret;
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_READ_ALLATONCE, 0, 0);
			copy_from_user(&args, (void *)arg, sizeof(args));
			args.count -= mbx_receive(&(fifop->mbx), args.buf, args.count, 1);
			if (args.count) {
				inode->i_atime = CURRENT_TIME;
				if ((handler_ret = ((int (*)(int, ...))(fifo[minor].handler))(minor, 'r')) < 0) {
					return handler_ret;
				}
				return args.count;
			}
			return 0;
		}
		case EAVESDROP: {
			struct { char *buf; int count; } args;
			copy_from_user(&args, (void *)arg, sizeof(args));
			return args.count - mbx_evdrp(&(fifop->mbx), (char **)&args.buf, args.count, 1);
		}
		case READ_TIMED: {
			struct { char *buf; int count, delay; } args;
			int handler_ret;
			copy_from_user(&args, (void *)arg, sizeof(args));
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_READ_TIMED, args.count, DELAY(args.delay));
			if (!args.delay) {
				args.count -= mbx_receive_wp(&(fifop->mbx), args.buf, args.count, 1);
				if (!args.count) {
					return -EAGAIN;
				}
			} else {
				args.count -= mbx_receive_timed(&(fifop->mbx), args.buf, args.count, DELAY(args.delay), 1);
			}
			if (args.count) {
				inode->i_atime = CURRENT_TIME;
//				if ((handler_ret = (fifop->handler)(minor)) < 0) {
				if ((handler_ret = ((int (*)(int, ...))(fifo[minor].handler))(minor, 'r')) < 0) {
					return handler_ret;
				}
				return args.count;
			}
			return 0;
		}
 		case READ_IF: {
 			struct { char *buf; int count; } args;
			int handler_ret;
 			copy_from_user(&args, (void *)arg, sizeof(args));
 			args.count -= mbx_receive_if(&(fifop->mbx), args.buf, args.count, 1);
 			if (args.count) {
 				inode->i_atime = CURRENT_TIME;
 				if ((handler_ret = ((int (*)(int, ...))(fifo[minor].handler))(minor, 'r')) < 0) {
 					return handler_ret;
 				}
 				return args.count;
 			}
 			return 0;
 		}
		case WRITE_TIMED: {
			struct { char *buf; int count, delay; } args;
			int handler_ret;
			copy_from_user(&args, (void *)arg, sizeof(args));
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_WRITE_TIMED, args.count, DELAY(args.delay));
			if (!args.delay) {
				args.count -= mbx_send_wp(&(fifop->mbx), args.buf, args.count, 1);
				if (!args.count) {
					return -EAGAIN;
				}
			} else {
				args.count -= mbx_send_timed(&(fifop->mbx), args.buf, args.count, DELAY(args.delay), 1);
			}
			inode->i_ctime = inode->i_mtime = CURRENT_TIME;
//			if ((handler_ret = (fifop->handler)(minor)) < 0) {
			if ((handler_ret = ((int (*)(int, ...))(fifo[minor].handler))(minor, 'w')) < 0) {
				return handler_ret;
			}
			return args.count;
		}
 		case WRITE_IF: {
 			struct { char *buf; int count, delay; } args;
 			int handler_ret;
 			copy_from_user(&args, (void *)arg, sizeof(args));
 			if (args.count) {
 				args.count -= mbx_send_wp(&(fifop->mbx), args.buf, args.count, 1);
 				if (!args.count) {
 					return -EAGAIN;
 				}
 			}
 			inode->i_ctime = inode->i_mtime = CURRENT_TIME;
 			if ((handler_ret = ((int (*)(int, ...))(fifo[minor].handler))(minor, 'w')) < 0) {
 				return handler_ret;
 			}
 			return args.count;
 		}
		case OVRWRITE: {
			struct { char *buf; int count; } args;
			copy_from_user(&args, (void *)arg, sizeof(args));
			return mbx_ovrwr_send(&(fifop->mbx), (char **)&args.buf, args.count, 1);
		}
		case RTF_SEM_INIT: {
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_INIT, minor, arg);
			mbx_sem_init(&(fifop->sem), arg);
			return 0;
		}
		case RTF_SEM_WAIT: {
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_WAIT, minor, 0);
			return mbx_sem_wait(&(fifop->sem));
		}
		case RTF_SEM_TRYWAIT: {
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_TRY_WAIT, minor, 0);
			return mbx_sem_wait_if(&(fifop->sem));
		}
		case RTF_SEM_TIMED_WAIT: {
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_TIMED_WAIT, minor, DELAY(arg));
			return mbx_sem_wait_timed(&(fifop->sem), DELAY(arg));
		}
		case RTF_SEM_POST: {
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_POST, minor, 0);
			mbx_sem_signal(&(fifop->sem), 0);
			return 0;
		}
		case RTF_SEM_DESTROY: {
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SEM_DESTROY, minor, 0);
			mbx_sem_delete(&(fifop->sem));
			return 0;
		}
		case SET_ASYNC_SIG: {
			TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_SET_ASYNC_SIG, arg, 0);
			async_sig = arg;
			return 0;
		}
		case FIONREAD: {
			return put_user(fifo[minor].mbx.avbs, (int *)arg);
	        }
#ifdef CONFIG_RTAI_RTF_NAMED
		/* 
		 * Support for named FIFOS : Ian Soanes (ians@zentropix.com)
		 * Based on ideas from Stuart Hughes and David Schleef
		 */
		case RTF_GET_N_FIFOS: {
			return MAX_FIFOS;
		}
		case RTF_GET_FIFO_INFO: {
			struct rt_fifo_get_info_struct req;
			int i, n;

			copy_from_user(&req, (void *)arg, sizeof(req));
			for ( i = req.fifo, n = 0; 
			      i < MAX_FIFOS && n < req.n; 
			      i++, n++
			      ) {
				struct rt_fifo_info_struct info;

				info.fifo_number = i;
				info.size        = fifo[i].mbx.size;
				info.opncnt      = fifo[i].opncnt;
				strncpy(info.name, fifo[i].name, RTF_NAMELEN+1);
				copy_to_user(req.ptr + n, &info, sizeof(info));
			}
			return n;
		}
		case RTF_CREATE_NAMED: {
			char name[RTF_NAMELEN+1];

			copy_from_user(name, (void *)arg, RTF_NAMELEN+1);
			return rtf_create_named(name);
	        }
		case RTF_NAME_LOOKUP: {
			char name[RTF_NAMELEN+1];

			copy_from_user(name, (void *)arg, RTF_NAMELEN+1);
			return rtf_getfifobyname(name);
		}
#endif /* CONFIG_RTAI_RTF_NAMED */

	        case TCGETS:
		        /* Keep isatty() probing silent */
		        return -ENOTTY;

		default : {
			printk("RTAI-FIFO: cmd %d is not implemented\n", cmd);
			return -EINVAL;
	}
	}
	return 0;
}

static unsigned int rtf_poll(struct file *filp, poll_table *wait)
{
	unsigned int retval, minor;

	retval = 0;
	minor = MINOR((filp->f_dentry->d_inode)->i_rdev);
	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_POLL, minor, 0);
	poll_wait(filp, &(fifo[minor].pollq), wait);
	if (fifo[minor].mbx.avbs) {
		retval |= POLLIN | POLLRDNORM;
	}
	if (fifo[minor].mbx.frbs) {
		retval |= POLLOUT | POLLWRNORM;
	}
	return retval;
}

static loff_t rtf_llseek(struct file *filp, loff_t offset, int origin)
{
	TRACE_RTAI_FIFO(TRACE_RTAI_EV_FIFO_LLSEEK, MINOR((filp->f_dentry->d_inode)->i_rdev), offset);

	return rtf_reset(MINOR((filp->f_dentry->d_inode)->i_rdev));
}

static struct file_operations rtf_fops =
{
#if LINUX_EXT_VERSION_CODE >= KERNEL_EXT_VERSION(2,4,0,4)
	owner:		THIS_MODULE,
#endif
	llseek:		rtf_llseek,
	read:		rtf_read,
	write:		rtf_write,
	poll:		rtf_poll,
	ioctl:		rtf_ioctl,
	open:		rtf_open,
	release:	rtf_release,
	fasync:		rtf_fasync,
};

#ifdef CONFIG_DEVFS_FS
static devfs_handle_t devfs_handle;
#endif

static int MaxFifos = MAX_FIFOS;
MODULE_PARM(MaxFifos, "i");

int init_module(void)
{
	int minor;
#ifdef CONFIG_DEVFS_FS
	char name[16];
#endif

#ifdef CONFIG_DEVFS_FS
	devfs_handle = devfs_mk_dir(NULL, "rtf", NULL);
	if(!devfs_handle)
	{
		printk("RTAI-FIFO: cannot create devfs dir entry.\n");
		return -EIO;
	}
#endif
	if (devfs_register_chrdev(RTAI_MAJOR, "rtai_fifo", &rtf_fops)) {
		printk("RTAI-FIFO: cannot register major %d.\n", RTAI_MAJOR);
		return -EIO;
	}
	if ((fifo_srq = rtf_request_srq(rtf_sysrq_handler)) < 0) {
		printk("RTAI-FIFO: no srq available in rtai.\n");
		return fifo_srq;
	}
	taskq.in = taskq.out = pol_asyn_q.in = pol_asyn_q.out = 0;
	async_sig = SIGIO;

	if (!(fifo = (FIFO *)kmalloc(MaxFifos*sizeof(FIFO), GFP_KERNEL))) {
		printk("RTAI-FIFO: cannot allocate memory for FIFOS structure.\n");
		return -ENOSPC;
	}
	memset(fifo, 0, MaxFifos*sizeof(FIFO));

	for (minor = 0; minor < MAX_FIFOS; minor++) {
#ifdef CONFIG_DEVFS_FS
		sprintf(name, "%d", minor);
		devfs_register(devfs_handle, name, DEVFS_FL_DEFAULT,
			RTAI_MAJOR, minor, 0660 | S_IFCHR, &rtf_fops, NULL);
#endif
		fifo[minor].opncnt = fifo[minor].pol_asyn_pended = 0;
		init_waitqueue_head(&fifo[minor].pollq);
		fifo[minor].asynq = 0;;
		mbx_sem_init(&(fifo[minor].sem), 0);
	}
#ifdef CONFIG_PROC_FS
	rtai_proc_fifo_register();
#endif
	return 0;
}

void cleanup_module(void)
{
#ifdef CONFIG_DEVFS_FS
	int minor;
	char name[16];
#endif

#ifdef CONFIG_RTL
	rtf_free_srq(fifo_srq);
#else
	if (rtf_free_srq(fifo_srq) < 0) {
		printk("RTAI-FIFO: rtai srq %d illegal or already free.\n", fifo_srq);
	}
#endif
#ifdef CONFIG_DEVFS_FS
	for (minor = 0; minor < MAX_FIFOS; minor++) {
		sprintf(name, "%d", minor);
		devfs_unregister(devfs_find_handle(devfs_handle, name, RTAI_MAJOR, 
			minor, DEVFS_SPECIAL_CHR, 0));
	}
#endif
#ifdef CONFIG_PROC_FS
        rtai_proc_fifo_unregister();
#endif
	devfs_unregister_chrdev(RTAI_MAJOR, "rtai_fifo");
#ifdef CONFIG_DEVFS_FS
	devfs_unregister(devfs_handle);
#endif
	kfree(fifo);
}

#ifdef CONFIG_PROC_FS
/* ----------------------< proc filesystem section >----------------------*/

static int rtai_read_fifos(char* buf, char** start, off_t offset,
	int len, int *eof, void *data)
{
	int i;

	len = sprintf(buf, "RTAI Real Time fifos status.\n\n" );
	if (len > LIMIT) {
		return(len);
	}
	len += sprintf(buf + len, "Maximum number of FIFOS %d.\n\n", MaxFifos);
	if (len > LIMIT) {
		return(len);
	}
	len += sprintf(buf+len, "fifo No  Open Cnt  Buff Size  handler  malloc type");
	if (len > LIMIT) {
		return(len);
	}
#ifdef CONFIG_RTAI_RTF_NAMED
	len += sprintf(buf+len, " Name\n----------------");
#else
	len += sprintf(buf+len, "\n");
#endif
	if (len > LIMIT) {
		return(len);
	}
	len += sprintf(buf+len, "-----------------------------------------\n");
	if (len > LIMIT) {
		return(len);
	}
/*
 * Display the status of all open RT fifos.
 */
	for (i = 0; i < MAX_FIFOS; i++) {
		if (fifo[i].opncnt > 0) {
			len += sprintf( buf+len, "%-8d %-9d %-10d %-10p %-12s", i,
                        	        fifo[i].opncnt, fifo[i].mbx.size,
					fifo[i].handler,
					fifo[i].malloc_type == 'v'
					    ? "vmalloc" : "kmalloc" 
					);
			if (len > LIMIT) {
				return(len);
			}
#ifdef CONFIG_RTAI_RTF_NAMED
			len += sprintf(buf+len, "%s\n", fifo[i].name);
#else
			len += sprintf(buf+len, "\n");
#endif
			if (len > LIMIT) {
				return(len);
			}
		} /* End if - fifo is open. */
	} /* End for loop - loop for all fifos. */
	return len;

}  /* End function - rtai_read_fifos */

static int rtai_proc_fifo_register(void) 
{
        struct proc_dir_entry *proc_fifo_ent;
        proc_fifo_ent = create_proc_entry("fifos", S_IFREG|S_IRUGO|S_IWUSR, 
								rtai_proc_root);
        if (!proc_fifo_ent) {
                printk("Unable to initialize /proc/rtai/fifos\n");
                return(-1);
        }
        proc_fifo_ent->read_proc = rtai_read_fifos;
	return 0;
}

static void rtai_proc_fifo_unregister(void) 
{
	remove_proc_entry("fifos", rtai_proc_root);
}

/* ------------------< end of proc filesystem section >------------------*/
#endif /* CONFIG_PROC_FS */

#ifdef CONFIG_RTAI_RTF_NAMED
/* 
 * Support for named FIFOS : Ian Soanes (ians@zentropix.com)
 * Based on ideas from Stuart Hughes and David Schleef
 */
int rtf_create_named(const char *name)
{
	int minor, err;
	unsigned long flags;

	if (strlen(name) > RTF_NAMELEN) {
	    	return -EINVAL;
	}
	rtf_spin_lock_irqsave(flags, rtf_name_lock);
	for (minor = 0; minor < MAX_FIFOS; minor++) {
	    	if (!strncmp(name, fifo[minor].name, RTF_NAMELEN)) {
			rtf_spin_unlock_irqrestore(flags, rtf_name_lock);
		        return -EBUSY;
		}
		else if (!fifo[minor].opncnt && !fifo[minor].name[0]) {
		        if ((err = rtf_create(minor, DEFAULT_SIZE)) < 0) {
				rtf_spin_unlock_irqrestore(flags, rtf_name_lock);
			        return err;
			}
		        strncpy(fifo[minor].name, name, RTF_NAMELEN+1);
			rtf_spin_unlock_irqrestore(flags, rtf_name_lock);
			return minor;
		}
	}
	rtf_spin_unlock_irqrestore(flags, rtf_name_lock);
	return -EBUSY;
}

int rtf_getfifobyname(const char *name)
{
    	int minor;

	if (strlen(name) > RTF_NAMELEN) {
	    	return -EINVAL;
	}
	for (minor = 0; minor < MAX_FIFOS; minor++) {
	    	if ( fifo[minor].opncnt && 
		     !strncmp(name, fifo[minor].name, RTF_NAMELEN)
		     ) {
		    	return minor;
		}
	}
	return -ENODEV;
}

EXPORT_SYMBOL(rtf_create_named);
EXPORT_SYMBOL(rtf_getfifobyname);

#endif /* CONFIG_RTAI_RTF_NAMED */


EXPORT_SYMBOL(rtf_create);
EXPORT_SYMBOL(rtf_create_handler);
EXPORT_SYMBOL(rtf_destroy);
EXPORT_SYMBOL(rtf_get);
EXPORT_SYMBOL(rtf_evdrp);
EXPORT_SYMBOL(rtf_put);
EXPORT_SYMBOL(rtf_reset);
EXPORT_SYMBOL(rtf_resize);
EXPORT_SYMBOL(rtf_sem_init);
EXPORT_SYMBOL(rtf_sem_delete);
EXPORT_SYMBOL(rtf_sem_post);
EXPORT_SYMBOL(rtf_sem_trywait);
EXPORT_SYMBOL(rtf_ovrwr_put);
EXPORT_SYMBOL(rtf_put_if);
EXPORT_SYMBOL(rtf_get_if);
