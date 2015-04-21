/*
COPYRIGHT (C) 2001  G.M. Bertani (gmbertani@yahoo.it)

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


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>

#define INTERFACE_TO_LINUX
#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_tbx.h>

#if defined(CONFIG_RTAI_DYN_MM) || defined(CONFIG_RTAI_DYN_MM_MODULE)
#include <rt_mem_mgr.h>
#define sched_malloc(size)	rt_malloc((size))
#define sched_free(adr)		rt_free((adr))
#define sched_mem_init() \
	{ if(rt_mem_init() != 0) { \
                printk("Failed to allocate memory for task stack(s)\n"); \
                return(-ENOMEM); \
        } }
#define sched_mem_end()		rt_mem_end()
#else
#define sched_malloc(size)	kmalloc((size), GFP_KERNEL)
#define sched_free(adr)		kfree((adr))
#define sched_mem_init()
#define sched_mem_end()
#endif

#define NOTHING ((void *)0)

#define SOMETHING ((void *)1)

extern void (*dnepsus_trxl)(void);

#define rt_schedule() dnepsus_trxl();

static inline void enqueue_blocked(RT_TASK *task, QUEUE *queue, int q_type)
{
	QUEUE *q;
	task->blocked_on = (q = queue);
	if (!q_type) {
		while ((q = q->next) != queue && (q->task)->priority <= task->priority);
	}
	q->prev = (task->queue.prev = q->prev)->next  = &(task->queue);
	task->queue.next = q;
}


static inline void dequeue_blocked(RT_TASK *task)
{
	task->prio_passed_to     = NOTHING;
	(task->queue.prev)->next = task->queue.next;
	(task->queue.next)->prev = task->queue.prev;
	task->blocked_on    	 = NOTHING;
}


/*++++++++++++++++++++++++++++ TYPED MAILBOXES ++++++++++++++++++++++++++++++*/ 

#define CHK_TBX_MAGIC \
do { \
	if (tbx->magic != RT_TBX_MAGIC) {  \
		return -EINVAL; \
	} \
} while (0);

#define TBX_MOD_SIZE(indx) ((indx) < tbx->size ? (indx) : (indx) - tbx->size)

#define TBX_REVERSE_MOD_SIZE(indx) ((indx) >= 0 ? (indx) : tbx->size + (indx))

static inline void tbx_smx_signal(TBX* tbx, SEM *smx)
{
	unsigned long flags;
	RT_TASK *task;

	flags = rt_global_save_flags_and_cli();
	if ((task = (smx->queue.next)->task)) {
        	tbx->waiting_nr--;
		dequeue_blocked(task);
		rt_rem_timed_task(task);
		if ((task->state &= ~(SEMAPHORE | DELAYED)) == READY) {
			rt_enq_ready_task(task);
			rt_schedule();
		}
	} else {
		smx->count = 1;
	}
	rt_global_restore_flags(flags);
}

static inline int tbx_smx_wait(TBX* tbx, SEM *smx, RT_TASK *rt_current)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
    	if (!(smx->count)) {
		tbx->waiting_nr++;
	        rt_current->state |= SEMAPHORE;
                rt_rem_ready_current(rt_current);
                enqueue_blocked(rt_current, &smx->queue, smx->qtype);
                rt_schedule();
	} else {
		smx->count = 0;
	}
	rt_global_restore_flags(flags);
	return (int)(rt_current->blocked_on);
}

static inline int tbx_smx_wait_until(TBX *tbx, SEM *smx, RTIME time, RT_TASK *rt_current)
{
	int timed = 0;
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (!(smx->count)) {
		tbx->waiting_nr++;
		rt_current->blocked_on = &smx->queue;
		rt_current->resume_time = time;
		rt_current->state |= (SEMAPHORE | DELAYED);
		rt_rem_ready_current(rt_current);
		enqueue_blocked(rt_current, &smx->queue, smx->qtype);
		rt_enq_timed_task(rt_current);
		rt_schedule();
        	if (rt_current->blocked_on) {
                        dequeue_blocked(rt_current);
			timed = 1;
			tbx->waiting_nr--;
		}
	} else {
		smx->count = 0;
	}
	rt_global_restore_flags(flags);
	return timed;
}


static inline int tbx_wait_room(TBX *tbx, int *fravbs, int msgsize, RT_TASK *rt_current)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if ((*fravbs) < msgsize) {
		tbx->waiting_nr++;
		rt_current->suspdepth = 1;
		rt_current->state |= SUSPENDED;
		rt_rem_ready_current(rt_current);
		rt_current->blocked_on = SOMETHING;
		tbx->waiting_task = rt_current;
		rt_schedule();
	}
	rt_global_restore_flags(flags);
	return (int)(rt_current->blocked_on);
}

static inline int tbx_check_room(TBX *tbx, int *fravbs, int msgsize)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (((*fravbs) < msgsize) || !tbx->sndsmx.count) {
		rt_global_restore_flags(flags);
		return 0;    
	}
	tbx->sndsmx.count = 0;
	rt_global_restore_flags(flags);
	return msgsize;
}

static inline int tbx_wait_room_until(TBX *tbx, int *fravbs, int msgsize, RTIME time, RT_TASK *rt_current)
{
	int timed = 0;
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if ((*fravbs) < msgsize) {
	        tbx->waiting_nr++;
		rt_current->blocked_on = SOMETHING;
		rt_current->resume_time = time;
		rt_current->state |= DELAYED;
		rt_rem_ready_current(rt_current);
		tbx->waiting_task = rt_current;
		rt_enq_timed_task(rt_current);
		rt_schedule();
		if (rt_current->blocked_on) {
			tbx->waiting_nr--;
			rt_current->blocked_on = NOTHING;
			tbx->waiting_task = NOTHING;
			timed = 1;
		}
	}
	rt_global_restore_flags(flags);
	return timed;
}

static inline unsigned char tbx_check_msg(TBX *tbx, int *fravbs, int msgsize, unsigned char* type)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (tbx->rcvsmx.count == 0 || (*fravbs) < msgsize ) {
		rt_global_restore_flags(flags);
		return 0;
	}
	tbx->rcvsmx.count = 0;
	if (*tbx->bcbadr == TYPE_BROADCAST) {
		*type = TYPE_BROADCAST;
	} else {
	        *type = *(tbx->bufadr + tbx->fbyte);
		tbx->fbyte = TBX_MOD_SIZE(tbx->fbyte + sizeof(*type));
		tbx->frbs += sizeof(*type);
		tbx->avbs -= sizeof(*type);
	}        
	rt_global_restore_flags(flags);
	return *type;
}


static inline int tbx_wait_msg(TBX *tbx, int *fravbs, int msgsize, unsigned char*type, RT_TASK *rt_current)
{
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (*tbx->bcbadr == TYPE_BROADCAST) {
		*type = TYPE_BROADCAST;
	} else {
		if ((*fravbs) < msgsize ) {
			tbx->waiting_nr++;
			rt_current->state |= SUSPENDED;
	                rt_rem_ready_current(rt_current);
			rt_current->blocked_on = SOMETHING;
			tbx->waiting_task = rt_current;
			rt_schedule();
        	}
	        *type = *(tbx->bufadr + tbx->fbyte);
        	tbx->fbyte = TBX_MOD_SIZE(tbx->fbyte + sizeof(*type));
	        tbx->frbs += sizeof(*type);
        	tbx->avbs -= sizeof(*type);
	}        
	rt_global_restore_flags(flags);
	return (int)(rt_current->blocked_on);
}

static inline int tbx_wait_msg_until(TBX *tbx, int *fravbs, int msgsize, RTIME time, unsigned char *type, RT_TASK *rt_current)
{
	int timed = 0;
	unsigned long flags;

	flags = rt_global_save_flags_and_cli();
	if (*tbx->bcbadr == TYPE_BROADCAST) {
		*type = TYPE_BROADCAST;
	} else {
	        *type = *(tbx->bufadr + tbx->fbyte);
        	if ((*fravbs) < msgsize && *tbx->bcbadr == TYPE_NONE) {
			tbx->waiting_nr++;
        		rt_current->blocked_on = SOMETHING;
			rt_current->resume_time = time;
			rt_current->state |= DELAYED;
                        rt_rem_ready_current(rt_current);
			tbx->waiting_task = rt_current;
                        rt_enq_timed_task(rt_current);
			rt_schedule();
			if (rt_current->blocked_on) {
				tbx->waiting_nr--;
				rt_current->blocked_on = NOTHING;
				tbx->waiting_task = NOTHING;
				timed = 1;
				*type = TYPE_NONE;
				rt_global_restore_flags(flags);
				return timed;
			}
	        }
        	tbx->fbyte = TBX_MOD_SIZE(tbx->fbyte + sizeof(*type));
	        tbx->frbs += sizeof(*type);
        	tbx->avbs -= sizeof(*type);
	}
	rt_global_restore_flags(flags);
	return timed;
}


static inline void tbx_signal(TBX *tbx)
{
	unsigned long flags;
	RT_TASK *task;

	flags = rt_global_save_flags_and_cli();
	if ((task = tbx->waiting_task)) {
		tbx->waiting_nr--;
                rt_rem_timed_task(task);
        	task->blocked_on = NOTHING;
		tbx->waiting_task = NOTHING;
		if ((task->state &= ~(SUSPENDED | DELAYED)) == READY) {
                        rt_enq_ready_task(task);
			rt_schedule();
		}
	}
	rt_global_restore_flags(flags);
}


// in get/put we must keep it all atomic, because urgent can put things at the
// head, while get may be retrieving.

static inline int tbxput(TBX *tbx, char **msg, int msg_size, unsigned char type)
{
	int tocpy, last_byte;
	unsigned long flags;
	int msgpacksize;
            
	msgpacksize = msg_size + sizeof(type); 
	flags = rt_global_save_flags_and_cli();
	while (tbx->frbs && msgpacksize > 0) {
        	last_byte = TBX_MOD_SIZE(tbx->fbyte + tbx->avbs);
		tocpy = tbx->size - last_byte;
		if (tocpy > msgpacksize) {
			tocpy = msgpacksize;
		}
//		rt_global_restore_flags(flags);
		if (type != TYPE_NONE) {
			tocpy = sizeof(type);
			*(tbx->bufadr + last_byte) = type;
			msgpacksize -= tocpy;
			type = TYPE_NONE;
	        } else {
			memcpy(tbx->bufadr + last_byte, *msg, tocpy);
			msgpacksize -= tocpy;
			*msg += tocpy;
		}
//		flags = rt_global_save_flags_and_cli();
        	tbx->frbs -= tocpy;
		tbx->avbs += tocpy;
	}    
	rt_global_restore_flags(flags);
	return msgpacksize;
}

static inline int tbxget(TBX *tbx, char **msg, int msg_size)
{
	int tocpy;
	unsigned long flags;
    
	flags = rt_global_save_flags_and_cli();
	while (tbx->avbs && msg_size > 0) {
		tocpy = tbx->size - tbx->fbyte;
		if (tocpy > msg_size) {
			tocpy = msg_size;
		}
		if (tocpy > tbx->avbs) {
			tocpy = tbx->avbs;
		}
//		rt_global_restore_flags(flags);
	        memcpy(*msg, tbx->bufadr + tbx->fbyte, tocpy);
        	msg_size  -= tocpy;
	        *msg      += tocpy;
//		flags = rt_global_save_flags_and_cli();
	        tbx->fbyte = TBX_MOD_SIZE(tbx->fbyte + tocpy);
        	tbx->frbs += tocpy;
	        tbx->avbs -= tocpy;
	}
	rt_global_restore_flags(flags);
	return msg_size;
}


static inline int tbxbackput(TBX *tbx, char **msg, int msg_size, unsigned char type)
{
	int tocpy = 0, first_byte = -1;
	unsigned long flags;
	int msgpacksize;
        
	msgpacksize = msg_size + sizeof(type); 
	flags = rt_global_save_flags_and_cli();
	while (tbx->frbs && msgpacksize > 0) {
        	if (first_byte == -1) {
			tbx->fbyte = TBX_REVERSE_MOD_SIZE(tbx->fbyte - msgpacksize);
			first_byte = tbx->fbyte;
	        } else {
        		first_byte = TBX_MOD_SIZE(first_byte + tocpy);
		}
		tocpy = tbx->size - first_byte;
		if (tocpy > msgpacksize) {
			tocpy = msgpacksize;
		}
//		rt_global_restore_flags(flags);
		if (type != TYPE_NONE) {
			tocpy = sizeof(type);
			*(tbx->bufadr + first_byte) = type;
			msgpacksize -= tocpy;
			type = TYPE_NONE;
	        } else {
        		memcpy(tbx->bufadr + first_byte, *msg, tocpy);
			msgpacksize -= tocpy;
			*msg += tocpy;
		}
//		flags = rt_global_save_flags_and_cli();
		tbx->frbs -= tocpy;
		tbx->avbs += tocpy;
	}    
	rt_global_restore_flags(flags);
	return msgpacksize;
}


static inline void tbx_receive_core(TBX *tbx, void *msg, int *msg_size, unsigned char type, unsigned char *lock)
{
	char *temp;

	temp = msg;    
	if (type == TYPE_BROADCAST) {
		if (*tbx->bcbadr == TYPE_NONE) {
			tbxget(tbx, (char **)(&temp), *msg_size);
			if (tbx->waiting_nr > 0) {
				memcpy((char*)(tbx->bcbadr+sizeof(type)), msg, *msg_size);
				*tbx->bcbadr = TYPE_BROADCAST;
				*lock = 1;
			} else {
				rt_sem_broadcast(&(tbx->bcbsmx));
			}
			*msg_size = 0;
		} else {
			memcpy(msg, (char*)(tbx->bcbadr+sizeof(type)), *msg_size);
			*msg_size = 0;                
			if (tbx->waiting_nr == 0) {
				*tbx->bcbadr = TYPE_NONE;
				rt_sem_broadcast(&(tbx->bcbsmx));
            		} else {
				*lock = 1;
			}
		}
	} else {
		*msg_size = tbxget(tbx, (char **)(&temp), *msg_size);
	}
}


int rt_tbx_init(TBX *tbx, int size, int flags)
{
	if (!(tbx->bufadr = sched_malloc(size))) { 
		return -ENOMEM;
	}
	if (!(tbx->bcbadr = sched_malloc(size))) { 
		sched_free(tbx->bufadr);
		return -ENOMEM;
	}
	*tbx->bcbadr = TYPE_NONE;
	memset(tbx->bufadr, 0, size);
	memset(tbx->bcbadr, 0, size);
	rt_typed_sem_init(&(tbx->sndsmx), 1, CNT_SEM | flags);
	rt_typed_sem_init(&(tbx->rcvsmx), 1, CNT_SEM | flags);
	rt_typed_sem_init(&(tbx->bcbsmx), 1, BIN_SEM | flags);
	tbx->magic = RT_TBX_MAGIC;
	tbx->size = tbx->frbs = size;
	tbx->waiting_task = 0;
	tbx->waiting_nr = 0;
	spin_lock_init(&(tbx->buflock));
	tbx->fbyte = tbx->avbs = 0;
	return 0;
}


int rt_tbx_delete(TBX *tbx)
{
	CHK_TBX_MAGIC;
	tbx->magic = 0;
	if (rt_sem_delete(&(tbx->sndsmx)) || rt_sem_delete(&(tbx->rcvsmx))|| rt_sem_delete(&(tbx->bcbsmx))) {
		return -EFAULT;
	}
	while (tbx->waiting_task) {
		tbx_signal(tbx);
	}
	sched_free(tbx->bufadr); 
	sched_free(tbx->bcbadr); 
	return 0;
}


int rt_tbx_send(TBX *tbx, void *msg, int msg_size)
{
	unsigned char type = TYPE_NORMAL;
	RT_TASK *rt_current;
    
	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
		return -EMSGSIZE;
	}
	if (tbx_smx_wait(tbx, &(tbx->sndsmx), rt_current = rt_whoami())) {
		return msg_size;
	}
   	if (tbx_wait_room(tbx, &tbx->frbs, msg_size + sizeof(type), rt_current)) {
		tbx_smx_signal(tbx, &(tbx->sndsmx));
		return msg_size;
	}
	msg_size = tbxput(tbx, (char **)(&msg), msg_size, type);
	tbx_signal(tbx);
	tbx_smx_signal(tbx, &(tbx->sndsmx));
	return msg_size;
}


int rt_tbx_send_if(TBX *tbx, void *msg, int msg_size)
{
	unsigned char type = TYPE_NORMAL;
	CHK_TBX_MAGIC;

	if ((msg_size + sizeof(type)) > tbx->size) {
		return -EMSGSIZE;
	}
	if (tbx_check_room(tbx, &tbx->frbs, (msg_size + sizeof(type)))) {
		tbxput(tbx, (char **)(&msg), msg_size, type);
		tbx_signal(tbx);
		tbx_smx_signal(tbx, &(tbx->sndsmx));
		return 0;
	}
	return msg_size;
}


int rt_tbx_send_until(TBX *tbx, void *msg, int msg_size, RTIME time)
{
	unsigned char type = TYPE_NORMAL;
	RT_TASK *rt_current;

	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
		return -EMSGSIZE;
	}
	if (tbx_smx_wait_until(tbx, &(tbx->sndsmx), time, rt_current = rt_whoami())) {
		return msg_size;
	}
	if (tbx_wait_room_until(tbx, &tbx->frbs, msg_size + sizeof(type), time, rt_current)) {
		tbx_smx_signal(tbx, &(tbx->sndsmx));
		return msg_size;
	}
	msg_size = tbxput(tbx, (char **)(&msg), msg_size, type);
	tbx_signal(tbx);
	tbx_smx_signal(tbx, &(tbx->sndsmx));
	return msg_size;
}


int rt_tbx_send_timed(TBX *tbx, void *msg, int msg_size, RTIME delay)
{
	return rt_tbx_send_until(tbx, msg, msg_size, rt_get_time() + delay);
}


int rt_tbx_receive(TBX *tbx, void *msg, int msg_size)
{
	unsigned char type = TYPE_NONE, lock = 0;  
	char* temp;
	RT_TASK *rt_current;
    
	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
		return -EMSGSIZE;
	}
	if (tbx_smx_wait(tbx, &(tbx->rcvsmx), rt_current = rt_whoami())) {
		return msg_size;
	}
	temp = msg;
	if (tbx_wait_msg(tbx, &tbx->avbs, msg_size + sizeof(type), &type, rt_current)) {
		tbx_smx_signal(tbx, &(tbx->rcvsmx));
		return msg_size;
	}
	tbx_receive_core(tbx, msg, &msg_size, type, &lock);
	tbx_signal(tbx);
	tbx_smx_signal(tbx, &(tbx->rcvsmx));
	if (lock == 1) {
        	rt_sem_wait(&(tbx->bcbsmx));
	}
	return msg_size;
}


int rt_tbx_receive_if(TBX *tbx, void *msg, int msg_size)
{
	unsigned char type = TYPE_NONE, lock = 0; 
	char* temp;
    
	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
		return -EMSGSIZE;
	}
	temp = msg;
	if (tbx_check_msg(tbx, &tbx->avbs, (msg_size + sizeof(type)), &type)) {
        	tbx_receive_core(tbx, msg, &msg_size, type, &lock);
	        tbx_signal(tbx);
        	tbx_smx_signal(tbx, &(tbx->rcvsmx));
	        if(lock == 1) {
			rt_sem_wait(&(tbx->bcbsmx));
	        }
	}
	return msg_size;
}


int rt_tbx_receive_until(TBX *tbx, void *msg, int msg_size, RTIME time)
{
	unsigned char type = TYPE_NONE, lock = 0;
	char* temp;
	RT_TASK *rt_current;

	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
		return -EMSGSIZE;
	}
	if (tbx_smx_wait_until(tbx, &(tbx->rcvsmx), time, rt_current = rt_whoami())) {
		return msg_size;
	}
	temp = msg;
	if (tbx_wait_msg_until(tbx, &tbx->avbs, msg_size + sizeof(type), time, &type, rt_current)) {
		tbx_smx_signal(tbx, &(tbx->rcvsmx));
		return msg_size;
	}
	tbx_receive_core(tbx, msg, &msg_size, type, &lock);
	tbx_signal(tbx);
	tbx_smx_signal(tbx, &(tbx->rcvsmx));
	if(lock == 1) {
		rt_sem_wait(&(tbx->bcbsmx));
	}
	return msg_size;
}


int rt_tbx_receive_timed(TBX *tbx, void *msg, int msg_size, RTIME delay)
{
	return rt_tbx_receive_until(tbx, msg, msg_size, rt_get_time() + delay);
}


int rt_tbx_broadcast(TBX *tbx, void *msg, int msg_size)
{
	unsigned char type = TYPE_BROADCAST; 
	int wakedup;
	RT_TASK *rt_current;
    
	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
		return -EMSGSIZE;
	}
	wakedup = tbx->waiting_nr;
	if (wakedup == 0) {
		return wakedup;
	}
	if (tbx_smx_wait(tbx, &(tbx->sndsmx), rt_current = rt_whoami())) {
		return 0;
	}
	if (tbx_wait_room(tbx, &tbx->frbs, msg_size + sizeof(type), rt_current)) {
		tbx_smx_signal(tbx, &(tbx->sndsmx));
		return 0;
	}
	msg_size = tbxput(tbx, (char **)(&msg), msg_size, type);
	tbx_signal(tbx);
	tbx_smx_signal(tbx, &(tbx->sndsmx));
	return wakedup;
}


int rt_tbx_broadcast_if(TBX *tbx, void *msg, int msg_size)
{
	unsigned char type = TYPE_BROADCAST; 
	int wakedup;
    
	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
		return -EMSGSIZE;
	}
	wakedup = tbx->waiting_nr;
	if (wakedup == 0) {
		return wakedup;
	}
	if (tbx_check_room(tbx, &tbx->frbs, (msg_size + sizeof(type)))) {
		msg_size = tbxput(tbx, (char **)(&msg), msg_size, type);
		tbx_signal(tbx);
		tbx_smx_signal(tbx, &(tbx->sndsmx));
		return wakedup;
	}
	return 0;
}


int rt_tbx_broadcast_until(TBX *tbx, void *msg, int msg_size, RTIME time)
{
	unsigned char type = TYPE_BROADCAST; 
	int wakedup;
	RT_TASK *rt_current;
    
	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
        	return -EMSGSIZE;
	}
	wakedup = tbx->waiting_nr;
	if (wakedup == 0) {
        	return wakedup;
	}
	if (tbx_smx_wait_until(tbx, &(tbx->sndsmx), time, rt_current = rt_whoami())) {
		return 0;
	}
	if (tbx_wait_room_until(tbx, &tbx->frbs, (msg_size + sizeof(type)), time, rt_current)) {
		tbx_smx_signal(tbx, &(tbx->sndsmx));
		return 0;
	}
	msg_size = tbxput(tbx, (char **)(&msg), msg_size, type);
	tbx_signal(tbx);
	tbx_smx_signal(tbx, &(tbx->sndsmx));
	return wakedup;
}


int rt_tbx_broadcast_timed(TBX *tbx, void *msg, int msg_size, RTIME delay)
{
	return rt_tbx_broadcast_until(tbx, msg, msg_size, rt_get_time() + delay);
}


int rt_tbx_urgent(TBX *tbx, void *msg, int msg_size)
{
	unsigned char type = TYPE_URGENT;
	RT_TASK *rt_current;
    
	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
        	return -EMSGSIZE;
	}
	if (tbx_smx_wait(tbx, &(tbx->sndsmx), rt_current = rt_whoami())) {
		return msg_size;
	}
	if (tbx_wait_room(tbx, &tbx->frbs, msg_size + sizeof(type), rt_current)) {
        	tbx_smx_signal(tbx, &(tbx->sndsmx));
		return msg_size;
	}
	msg_size = tbxbackput(tbx, (char **)(&msg), msg_size, type);
	tbx_signal(tbx);
	tbx_smx_signal(tbx, &(tbx->sndsmx));
	return msg_size;
}


int rt_tbx_urgent_if(TBX *tbx, void *msg, int msg_size)
{
	unsigned char type = TYPE_URGENT;
    
	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
        	return -EMSGSIZE;
	}
	if (tbx_check_room(tbx, &tbx->frbs, (msg_size + sizeof(type)))) {
        	msg_size = tbxbackput(tbx, (char **)(&msg), msg_size, type);
		tbx_signal(tbx);
		tbx_smx_signal(tbx, &(tbx->sndsmx));
		return 0;
	}
	return msg_size;
}


int rt_tbx_urgent_until(TBX *tbx, void *msg, int msg_size, RTIME time)
{
	unsigned char type = TYPE_URGENT;
	RT_TASK *rt_current;
    
	CHK_TBX_MAGIC;
	if ((msg_size + sizeof(type)) > tbx->size) {
        	return -EMSGSIZE;
	}
	if (tbx_smx_wait_until(tbx, &(tbx->sndsmx), time, rt_current = rt_whoami())) {
		return msg_size;
	}
	if (tbx_wait_room_until(tbx, &tbx->frbs, (msg_size + sizeof(type)), time, rt_current)) {
	        tbx_smx_signal(tbx, &(tbx->sndsmx));
        	return msg_size;
	}
	msg_size = tbxbackput(tbx, (char **)(&msg), msg_size, type);
    	tbx_signal(tbx);
	tbx_smx_signal(tbx, &(tbx->sndsmx));
	return msg_size;
}

int rt_tbx_urgent_timed(TBX *tbx, void *msg, int msg_size, RTIME delay)
{
	return rt_tbx_urgent_until(tbx, msg, msg_size, rt_get_time() + delay);
}

/*+++++++++++++++++++++++ END OF TYPED MAILBOXES +++++++++++++++++++++++++++*/ 
