
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

$Id: qblk.h,v 1.1.1.1 2004/06/06 14:02:34 rpm Exp $ 
*/

#ifndef _QBLK_H_
#define _QBLK_H_

/*
 * Originally developped under QNX 2 by Alain Choquet who was in charge
 * of software developement at POSEIDON CONTROLS INC for four years.
*/

typedef struct QueueBlock QBLK;
typedef struct QueueHook QHOOK;

// A QBLK is an elementary structure that can be inserted in a queue
struct QueueBlock {
			/* This part is managed by the q functions */
	struct QueueHook *hook;	/* Pointer to current QHOOK or NULL */
	struct QueueBlock *prv;	/* Pointer to previous QBLK when hooked */
	struct QueueBlock *nxt;	/* Pointer to next QBLK in queue when hooked */
	struct QueueBlock **lnk;/* For static QBLKs: (QBLK **)-1.
				   For dynamic QBLKs, pointer to the user's
				   pointer to the QBLK. The user's pointer is
				   set to NULL when the QBLK is released */

			/* This part may be used by any queue manager */
	RTIME tic, rpt;
				/* This part describes the action to take on
				   completion.
				   The fun and arg fields are usually set by the
				   QBLK owner and should not be altered by the
				   queue manager.
				   The evn field may be altered according to the
				   each queue manager convention */
	void (*fun)(void *,int);  /* Function called on completion */
	void *arg;		  /* First argument passed to function */
	int  evn;		  /* Second argument passed to function */
};

// A QHOOK is an elementary structure on wich QBLKs are linked to form a queue.
// Accordingly, any QHOOK owner is a queue manager.
struct QueueHook {
	struct QueueBlock *head;/* Pointer to first QBLK in queue (or NULL) */
	struct QueueBlock *tail;/* Pointer to last QBLK in queue (or NULL) */
	void (*cancel)(void *, QBLK *);/* Function to call when a QBLK is dequeued (or cancelled) by its owner. May be NULL. */
	void *arg;	/* First argument passed to the function */
			/* The function will receive a pointer to the not
			   yet unhooked QBLK as second argument */
};

//
// Each task needs a TickQueue to manage his qblk's
//
struct TickQueue {
	QBLK *free;
	QBLK *wait;
        RTIME tic;
        QHOOK hook;
        QHOOK exec;
	spinlock_t slock;
        };

/******************************************************************************
 * QBLK Initialisation.
 *****************************************************************************/
 
/*
 * A rt_task that wants to use QBLK's must first create a TickQueue.
 * When the program terminates it is necessary to release and free the memory.
 * These functions are required by LXRT & deviate from the QNX 2 implementation.*/

int rt_InitTickQueue(void);
                        /* Allocate and initialise the Tick queue. */

void rt_ReleaseTickQueue(void);
                        /* Release the Tick queue and free the memory. */

QHOOK *rt_GetTickQueueHook(void);
                        /* Get a pointer to Tick queue hook. */
		
void rt_qCleanup(RT_TASK *rtt);
			/* This function releases the memory of all qBlk's
			   and the tick queue as well. To be used at task
			   termination only. */				
	
/******************************************************************************
 * Static QBLK functions
 *****************************************************************************/

extern QBLK *rt_qBlkInit(QBLK *, void (*fun)(void *, int), void *, int);
			/* Initialise a QBLK allocated by the user. This
			   function may be applied to an initialised
			   QBLK provided that it is NOT hooked. It should never
			   be used on a dynamic QBLK.
			   The owner or the QBLK may determine if it is
			   queued by testing the hook field of the QBLK.
			   Static qBlk's are reserved for the real time kernel.
			   In user space dynamic qBlk's are mandatory. */	

/******************************************************************************
 * Dynamic QBLK functions
 *****************************************************************************/

extern unsigned rt_qDynAlloc(unsigned n);
			/* Allocate n QBLKs and put them on free list.
			   Return the number of QBLK not allocated
			   (normally 0) */

extern unsigned rt_qDynFree(int n);
			/* Free n QBLKs from free list. Return the number
			   of QBLK not freed (normally 0) */

extern QBLK *rt_qDynInit(QBLK **, void (*fun)(void *, int), void *, int );
			/* Get a QBLK from the free list and init it. The
			   optional link pointer is set to point to the QBLK.
			   Upon release of the QBLK the link pointer, if any,
                           will be zeroed provided the qBlk was initiated in
                           the kernel. Link pointers do not get zeroed if the
                           qBlk is initiated in user space. This is a deviation
                           from the QNX 2 implementation. 
			   The function returns zero if there is no available
                           QBLK (system out of memory).
			   Dynamic qBlk's can be used in user space in both
                           lxrt soft and hard real time modes. */

/******************************************************************************
 * General QBLK functions
 *****************************************************************************/

extern void rt_qBlkWait(QBLK *qblk, RTIME tics);
			/* Insert a QBLK in the Tick queue (after dequeuing
			   it if need be) to be executed after the given
			   number of ticks. Specifying a tick count of 0
			   is the normal way of inserting a QBLK after
			   all currently expired QBLKs */

extern void rt_qBlkRepeat(QBLK *, RTIME);
			/* Insert a QBLK in the Tick queue (after dequeuing
			   it if need be) to be executed after the given
			   number of ticks. After completion, the QBLK will
			   be reinserted in the queue with the same delay
			   if it is not cancelled or dequeued. Note that a
			   tick count of 0 does not repeat */

extern void rt_qBlkSoon(QBLK *);
			/* Insert a QBLK at head of the Tick Queue (after
			   dequeuing it if need be). The QBLK will be
			   executed before any already expired QBLK. */

extern void rt_qBlkDequeue(QBLK *);
			/* Unhook a QBLK from a queue and notify the queue
			   manager. The QBLK is not released. If the QBLK
			   was not queued this function does nothing */

extern void rt_qBlkCancel(QBLK *);
			/* Dequeue (if it is hooked) and release a QBLK */

/******************************************************************************
 * Queue management functions
 *****************************************************************************/

extern QHOOK *rt_qHookInit(QHOOK **, void (*cancel)(void *, QBLK *), void *);
			/* Allocate and initialise a QHOOK.
			   Deviates from the QNX 2 implementation and is
			   required by LXRT. Returns zero on error or
			   returns the pointer and sets the link if any. */
	
extern void rt_qHookRelease(QHOOK *);
                        /* Release and free the memory of a QHOOK.
                           Required by the LXRT implementation. */

extern void rt_qBlkBefore(QBLK *, QBLK *);
			/* Insert the QBLK before another QBLK in a queue.
			   If it was hooked it will first be unhooked */

extern void rt_qBlkAfter(QBLK *, QBLK *);
			/* Insert the QBLK after another QBLK in a queue.
			   If it  was hooked it will first be unhooked */

extern void rt_qBlkAtHead(QBLK *, QHOOK *);
			/* Insert the QBLK at head of a queue.
			   If it was hooked it will first be unhooked */

extern void rt_qBlkAtTail(QBLK *, QHOOK *);
			/* Insert the QBLK at tail of a queue.
			   If it was hooked it will first be unhooked */

extern QBLK *rt_qBlkUnhook(QBLK *);
			/* Remove the QBLK from a queue. The QBLK is not
			   released and the cancel function not called */

extern void rt_qBlkRelease(QBLK *);
			/* Unhook the QBLK (if it is hooked); if it is a
			   dynamic QBLK, the user's link is cleared and
			   it is moved to the free list */

extern void rt_qBlkComplete(QBLK *);
			/* Release and execute the QBLK. The QBLK is
			   released BEFORE being executed */

extern void rt_qBlkSchedule(QBLK *, RTIME);
			/* Move the QBLK from a queue to the Tick queue.
			   This function is similar to qBlkWait() but it
			   does not execute the QHOOK's cancel function.
			   It is intended to be used by the QHOOK owner,
			   instead of the QBLK owner */

extern void rt_qHookFlush(QHOOK *);
			/* Move all QBLKs in a queue to the Tick queue.
			   There is no delay specified */

/******************************************************************************
 * Scheduling functions
 *****************************************************************************/

extern int rt_qSync(void);
			/* Complete (execute and release) all expired QBLKs
			   in the Tick Queue. QBLKs that expire during this
			   process will also be completed */

extern pid_t rt_qReceive(pid_t target, void *buf, size_t maxlen, size_t *msglen);
			/* Run scheduler until reception of a message or a
			   proxy from specific target, or any if target is zero */

extern void rt_qLoop(void);
			/* Run scheduler until the Tick queue is empty */

extern RTIME rt_qStep(void);
			/* Execute once if the queue is not empty and return 0,
			   or return -1 if the queue was empty,
			   or return the time to wait before the next qBlk. */	

#endif // _QBLK_H_
