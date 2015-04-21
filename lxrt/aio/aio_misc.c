/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: aio_misc.c,v 1.1.1.1 2004/06/06 14:02:36 rpm Exp $
 *
 * Copyright: (C) 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
 *            This file was part of the GNU C Library.
 *            Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997
 *            Modified for RTAI kernelspace by Erwin Rol <erwin@muffin.org>, 2001
 *
 * Licence:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#ifndef LXRT_MODULE
#define LXRT_MODULE
#endif

static int errno;
#define __KERNEL_SYSCALLS__

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/smp_lock.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <asm/segment.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>
#include <rtai_nam2num.h>
extern struct proc_dir_entry *rtai_proc_root;
#endif


#include "names.h"
#include "proxies.h"
#include "msg.h"
#include "qblk.h"
#include "registry.h"
#include "traps.h"

#include "aio.h"
#include "aio_misc.h"

#include "rt_mem_mgr.h"
#include "rtai_lxk.h"


extern ssize_t sys_read(unsigned int fd, char * buf, size_t count);
extern ssize_t sys_write(unsigned int fd, const char * buf, size_t count);
extern ssize_t sys_pread(unsigned int fd, char * buf,
                             size_t count, loff_t pos);
extern ssize_t sys_pwrite(unsigned int fd, const char * buf,
                              size_t count, loff_t pos);
extern long sys_fsync(unsigned int fd);
extern long sys_fdatasync(unsigned int fd);
extern long sys_open(const char * filename, int flags, int mode);
extern long sys_close(unsigned int fd);


static void add_request_to_runlist (struct requestlist *newrequest);

/* Pool of request list entries.  */
static struct requestlist **pool = 0;

/* Number of total and allocated pool entries.  */
static volatile size_t pool_max_size = 0;
static volatile size_t pool_size = 0;

/* We implement a two dimensional array but allocate each row separately.
   The macro below determines how many entries should be used per row.
   It should better be a power of two.  */
#define ENTRIES_PER_ROW	32

/* How many rows we allocate at once.  */
#define ROWS_STEP	8

/* List of available entries.  */
static struct requestlist *freelist = 0;

/* List of request waiting to be processed.  */
static struct requestlist *runlist = 0;

/* Structure list of all currently processed requests.  */
static struct requestlist *requests = 0;

/* Number of threads currently running.  */
atomic_t nthreads;

/* Number of threads waiting for work to arrive. */
atomic_t idle_thread_count;


/* Since the list is global we need a mutex protecting it.  */
SEM __aio_requests_mutex;

/* When you add a request to the list and there are idle threads present,
   you signal this condition variable. When a thread finishes work, it waits
   on this condition variable for a time before it actually exits. */
CND __aio_new_request_notification;


/* Functions to handle request list pool.  */
static struct requestlist *
get_elem (void)
{
	struct requestlist *result;

	if (freelist == NULL)
	{
		struct requestlist *new_row;
		int cnt;

		if (pool_size + 1 >= pool_max_size)
		{
			size_t new_max_size = pool_max_size + ROWS_STEP;
			struct requestlist **new_tab;

			new_tab = (struct requestlist **)
										rt_malloc( new_max_size * sizeof (struct requestlist *));

			if (new_tab == NULL){
				printk("rt_malloc failed!\n");
				return NULL;
			}

			memset(new_tab, 0 ,new_max_size * sizeof (struct requestlist *));
			if(pool != NULL){
				memcpy(new_tab, pool, pool_max_size * sizeof (struct requestlist *));
				rt_free(pool);
			}
			
			pool_max_size = new_max_size;
			pool = new_tab;
		}

		/* Allocat the new row.  */
		cnt = pool_size == 0 ? aio_num_requests : ENTRIES_PER_ROW;
		new_row = (struct requestlist *) rt_malloc( cnt * sizeof (struct requestlist));

		if (new_row == NULL){
			printk("rt_malloc failed!\n");
			return NULL;
		}

		memset(new_row, 0 , cnt * sizeof (struct requestlist));

		pool[pool_size++] = new_row;

		/* Put all the new entries in the freelist.  */
		do
		{
			new_row->next_prio = freelist;
			freelist = new_row++;
		} while (--cnt > 0);
	}

	result = freelist;
	freelist = freelist->next_prio;

	return result;
}


void
__aio_free_request (struct requestlist *elem)
{
	elem->running = no;
	elem->next_prio = freelist;
	freelist = elem;
}


struct requestlist *
__aio_find_req (struct aiocb *elem)
{
	struct requestlist *runp = requests;
	int fildes = elem->aio_fildes;

	while (runp != NULL && runp->aiocbp->aio_fildes < fildes)
		runp = runp->next_fd;

	if (runp != NULL)
	{
		if (runp->aiocbp->aio_fildes != fildes)
			runp = NULL;
		else
			while (runp != NULL && runp->aiocbp != elem)
				runp = runp->next_prio;
	}

	return runp;
}


struct requestlist *
__aio_find_req_fd (int fildes)
{
	struct requestlist *runp = requests;

	while (runp != NULL && runp->aiocbp->aio_fildes < fildes)
		runp = runp->next_fd;

	return (runp != NULL && runp->aiocbp->aio_fildes == fildes
						? runp : NULL);
}


void
__aio_remove_request (struct requestlist *last, struct requestlist *req, int all)
{
	if (last != NULL)
		last->next_prio = all ? NULL : req->next_prio;
  else
	{
		if (all || req->next_prio == NULL)
		{
			if (req->last_fd != NULL)
				req->last_fd->next_fd = req->next_fd;
			else
				requests = req->next_fd;

			if (req->next_fd != NULL)
				req->next_fd->last_fd = req->last_fd;
		}
		else
		{
			if (req->last_fd != NULL)
				req->last_fd->next_fd = req->next_prio;
			else
				requests = req->next_prio;

			if (req->next_fd != NULL)
				req->next_fd->last_fd = req->next_prio;

			req->next_prio->last_fd = req->last_fd;
			req->next_prio->next_fd = req->next_fd;

			/* Mark this entry as runnable.  */
			req->next_prio->running = yes;
		}

		if (req->running == yes)
		{
			struct requestlist *runp = runlist;

			last = NULL;
			while (runp != NULL)
			{
				if (runp == req)
				{
					if (last == NULL)
						runlist = runp->next_run;
					else
						last->next_run = runp->next_run;
					
					break;
				}

				last = runp;
				runp = runp->next_run;
			}
		}
	}
}


/* The main function of the async I/O handling.  It enqueues requests
   and if necessary starts and handles threads.  */
struct requestlist *
__aio_enqueue_request (struct aiocb *aiocbp, int operation)
{
	int result = 0;
	int policy = 0, prio = 0;
	struct requestlist *last, *runp, *newp;
	int running = no;

	if (operation == LIO_SYNC || operation == LIO_DSYNC)
		aiocbp->aio_reqprio = 0;
	else if (aiocbp->aio_reqprio < 0
						|| aiocbp->aio_reqprio > AIO_PRIO_DELTA_MAX)
	{
		/* Invalid priority value.  */
 		//__set_errno (EINVAL);
		aiocbp->__error_code = EINVAL;
		aiocbp->__return_value = -1;
		return NULL;
	}

	/* Compute priority for this request.  */
	//  pthread_getschedparam (pthread_self (), &policy, &param);
	// FIXME prio = param.sched_priority - aiocbp->aio_reqprio;

	/* Get the mutex.  */
	rt_sem_wait( &__aio_requests_mutex );
	
	last = NULL;
	runp = requests;
	/* First look whether the current file descriptor is currently
     worked with.  */
	while (runp != NULL
		&& runp->aiocbp->aio_fildes < aiocbp->aio_fildes)
	{
		last = runp;
		runp = runp->next_fd;
	}


	/* Get a new element for the waiting list.  */
	newp = get_elem ();
	if (newp == NULL)
	{
		printk("failed to get new elem\n");
		rt_sem_signal(&__aio_requests_mutex);

		//__set_errno (EAGAIN);
		return NULL;
	}
	newp->aiocbp = aiocbp;
	// FIXME  newp->caller_pid = (aiocbp->aio_sigevent.sigev_notify == SIGEV_SIGNAL ? getpid () : 0);
	newp->caller_pid = 0;	
	newp->waiting = NULL;

	// XXX prio and policy are "unused" 
	aiocbp->__abs_prio = prio;
	aiocbp->__policy = policy;
	aiocbp->aio_lio_opcode = operation;
	aiocbp->__error_code = EINPROGRESS;
	aiocbp->__return_value = 0;

	if (runp != NULL
			&& runp->aiocbp->aio_fildes == aiocbp->aio_fildes)
	{
		/* The current file descriptor is worked on.  It makes no sense
		to start another thread since this new thread would fight
		with the running thread for the resources.  But we also cannot
		say that the thread processing this desriptor shall immediately
		after finishing the current job process this request if there
		are other threads in the running queue which have a higher
		priority.  */

		/* Simply enqueue it after the running one according to the
		priority.  */
		while (runp->next_prio != NULL
						&& runp->next_prio->aiocbp->__abs_prio >= prio)
			runp = runp->next_prio;

		newp->next_prio = runp->next_prio;
		runp->next_prio = newp;

		running = queued;
	}
	else
	{
		running = yes;
		/* Enqueue this request for a new descriptor.  */
		if (last == NULL)
		{
			newp->last_fd = NULL;
			newp->next_fd = requests;
			if (requests != NULL)
				requests->last_fd = newp;
			requests = newp;
		}
		else
		{
			newp->next_fd = last->next_fd;
			newp->last_fd = last;
			last->next_fd = newp;
			
			if (newp->next_fd != NULL)
				newp->next_fd->last_fd = newp;
		}

		newp->next_prio = NULL;
	}

	if (running == yes)
	{
		/* We try to create a new thread for this file descriptor.  The
		function which gets called will handle all available requests
		for this descriptor and when all are processed it will
		terminate.

		If no new thread can be created or if the specified limit of
		threads for AIO is reached we queue the request.  */

		/* See if we need to and are able to create a thread.  */
		if (atomic_read(&nthreads) < aio_threads && atomic_read(&idle_thread_count) == 0)
		{
			running = newp->running = allocated;

			/* Now try to start a thread.  */
			if (rt_create_thread(newp) == 0){
				/* We managed to enqueue the request.  All errors which can
				happen now can be recognized by calls to `aio_return' and
				`aio_error'.  */
				atomic_inc( &nthreads );
			}
			else
			{
				/* Reset the running flat.  The new request is not running.  */
				running = newp->running = yes;

				if (atomic_read(&nthreads) == 0)
					/* We cannot create a thread in the moment and there is
					also no thread running.  This is a problem.  `errno' is
					set to EAGAIN if this is only a temporary problem.  */
					result = -1;
			}
		}
	}

	/* Enqueue the request in the run queue if it is not yet running.  */
	if (running == yes && result == 0)
	{
		add_request_to_runlist (newp);

		/* If there is a thread waiting for work, then let it know that we
		have just given it something to do. */
		if (atomic_read(&idle_thread_count) > 0){
			rt_cond_signal( &__aio_new_request_notification );
		}
	}

	if (result == 0){
		newp->running = running;
	}
	else
	{
		/* Something went wrong.  */
		__aio_free_request (newp);
 		newp = NULL;
	}

	/* Release the mutex.  */
	rt_sem_signal(&__aio_requests_mutex);

	return newp;
}


int
handle_fildes_io (void *arg)
{
	struct requestlist *runp = (struct requestlist *) arg;
	struct aiocb *aiocbp = 0;
	int policy = 0;
	int fildes = -1;
	RT_TASK* rt_task = 0;
	sigset_t tmpsig;

	sprintf(current->comm,"kaiod");
	daemonize();

	set_fs( KERNEL_DS );

	current->policy = SCHED_RR;

	/* Block all signals except SIGKILL and SIGSTOP */
	spin_lock_irq(&current->sigmask_lock);
	tmpsig = current->blocked;
	siginitsetinv(&current->blocked, sigmask(SIGKILL) | sigmask(SIGSTOP) );
	recalc_sigpending(current);
	spin_unlock_irq(&current->sigmask_lock);

	rt_task = lxk_task_init(current->pid,1,0,0);

	if(rt_task == 0){
		printk("AIO: lxrt_task_init failed!\n");
		return 0;
	}

	do
	{
		/* If runp is NULL, then we were created to service the work queue
		in general, not to handle any particular request. In that case we
		skip the "do work" stuff on the first pass, and go directly to the
		"get work off the work queue" part of this loop, which is near the
		end. */
		if (runp == NULL)
		{
			lxk_sem_wait( &__aio_requests_mutex );
		}
		else
		{
			/* Hopefully this request is marked as running.  */

			/* Update our variables.  */
			aiocbp = runp->aiocbp;
			fildes = aiocbp->aio_fildes;

			/* Change the priority to the requested value (if necessary).  */
			if (/*aiocbp->__abs_prio != param.sched_priority ||*/
					aiocbp->__policy != policy)
			{
				// param.sched_priority = aiocbp->__abs_prio;
				policy = aiocbp->__policy;
				// pthread_setschedparam (self, policy, &param);
			}

			/* Process request pointed to by RUNP.  We must not be disturbed
			by signals.  */

			switch(aiocbp->aio_lio_opcode){
				case LIO_DSYNC: {
					printk("AIO: thread %d calling DSYNC\n",current->pid);
					aiocbp->__return_value = sys_fdatasync (fildes);
					printk("dsync returned %d\n",aiocbp->__return_value);	
				} break;

				case LIO_SYNC: {
					printk("AIO: thread %d calling SYNC\n", current->pid);
					aiocbp->__return_value = sys_fsync (fildes);
					printk("sync returned %d\n",aiocbp->__return_value);	
				} break;

				case LIO_OPEN: {
					printk("AIO: thread %d calling OPEN on %.64s \n",current->pid ,(char*)aiocbp->aio_buf);
					aiocbp->aio_fildes = aiocbp->__return_value =
						sys_open((const char*)aiocbp->aio_buf,aiocbp->aio_flags,aiocbp->aio_mode);
					//printk("open returned %d\n",aiocbp->aio_fildes);	
	
				} break;

				case LIO_CLOSE: {
					printk("AIO: thread %d calling CLOSE\n",current->pid);
					aiocbp->__return_value = sys_close (fildes);
					printk("close returned %d\n",aiocbp->__return_value);	

				} break;

				case LIO_WRITE: {
					printk("AIO: thread %d calling WRITE\n",current->pid);
					aiocbp->__return_value = sys_pwrite (fildes, (const void *)
																								aiocbp->aio_buf,
																								aiocbp->aio_nbytes,
																								aiocbp->aio_offset);

					if (aiocbp->__return_value == -1 && errno == ESPIPE)
					{
						/* The Linux kernel is different from others.  It returns
						ESPIPE if using pwrite on a socket.  Other platforms
						simply ignore the offset parameter and behave like
						write.  */
						aiocbp->__return_value = sys_write (fildes,
																								(void *) aiocbp->aio_buf,
																								aiocbp->aio_nbytes);
					}
				
					//	printk("write returned %d\n",aiocbp->__return_value);	
				} break;

				case LIO_READ: {
					printk("AIO: thread %d calling READ\n",current->pid);
					aiocbp->__return_value = sys_pread (fildes,
						(void *) aiocbp->aio_buf,
						aiocbp->aio_nbytes,
						aiocbp->aio_offset);

					if (aiocbp->__return_value == -1 && errno == ESPIPE)
					{
						/* The Linux kernel is different from others.  It returns
						ESPIPE if using pread on a socket.  Other platforms
						simply ignore the offset parameter and behave like
						read.  */
						aiocbp->__return_value =  sys_read (fildes,
																								(void *) aiocbp->aio_buf,
																								aiocbp->aio_nbytes);
					}
				//	printk("write returned %d\n",aiocbp->__return_value);	

				} break;

				default: {
					/* This is an invalid opcode.  */
					printk("AIO: thread %d calling INVALID\n",current->pid);
					aiocbp->__return_value = -1;
					//__set_errno (EINVAL);
				} break;
			}

			/* Get the mutex.  */
			lxk_sem_wait(&__aio_requests_mutex);

			/* In theory we would need here a write memory barrier since the
			callers test using aio_error() whether the request finished
			and once this value != EINPROGRESS the field __return_value
			must be committed to memory.

			But since the pthread_mutex_lock call involves write memory
			barriers as well it is not necessary.  */

			if (aiocbp->__return_value < 0)
				aiocbp->__error_code = errno;
			else
				aiocbp->__error_code = 0;

			/* Send the signal to notify about finished processing of the
			request.  */
			__aio_notify (runp);

			/* For debugging purposes we reset the running flag of the
			finished request.  */
			runp->running = done;

			/* Now dequeue the current request.  */
			__aio_remove_request (NULL, runp, 0);
			if (runp->next_prio != NULL)
				add_request_to_runlist (runp->next_prio);

			/* Free the old element.  */
	  	__aio_free_request (runp);
		}

		runp = runlist;

		/* If the runlist is empty, then we sleep for a while, waiting for
		something to arrive in it. */
		if (runp == NULL && aio_idle_time >= 0)
		{
			RTIME wakeup_time;
			wakeup_time = nano2count(aio_idle_time * 1000000000LL);
			atomic_inc(&idle_thread_count);

			lxk_cond_wait_timed( &__aio_new_request_notification, &__aio_requests_mutex, wakeup_time);

			atomic_dec(&idle_thread_count);
			runp = runlist;
		}

		if (runp == NULL)
		{
			atomic_dec(&nthreads);
		}
		else
		{
			runp->running = allocated;
			runlist = runp->next_run;

			/* If we have a request to process, and there's still another in
	     the run list, then we need to either wake up or create a new
	     thread to service the request that is still in the run list. */
			if (runlist != NULL)
			{
				/* There are at least two items in the work queue to work on.
				If there are other idle threads, then we should wake them
				up for these other work elements; otherwise, we should try
				to create a new thread. */
				if (atomic_read(&idle_thread_count) > 0)
				{
					lxk_cond_signal (&__aio_new_request_notification);
				}
				else if (atomic_read(&nthreads) < aio_threads)
				{
					/* Now try to start a thread. If we fail, no big deal,
					because we know that there is at least one thread (us)
					that is working on AIO operations. */
					if (lxk_create_thread(NULL) == 0)
						atomic_inc(&nthreads);
				}
			}
		}

		/* Release the mutex.  */
		lxk_sem_signal(&__aio_requests_mutex);
	}
	while (runp != NULL);

	lxk_task_delete(rt_task);	

	return 0;
}


/* Free allocated resources.  */
void
free_res( void )
{
	size_t row;

	for (row = 0; row < pool_max_size; ++row)
		if(pool[row] != 0)
			rt_free( pool[row] );

	if(pool != 0)
		rt_free( pool );
}


/* Add newrequest to the runlist. The __abs_prio flag of newrequest must
   be correctly set to do this. Also, you had better set newrequest's
   "running" flag to "yes" before you release your lock or you'll throw an
   assertion. */
static void
add_request_to_runlist (struct requestlist *newrequest)
{
	int prio = newrequest->aiocbp->__abs_prio;
	struct requestlist *runp;

	if (runlist == NULL || runlist->aiocbp->__abs_prio < prio)
	{
		newrequest->next_run = runlist;
		runlist = newrequest;
	}
	else
	{
		runp = runlist;

		while (runp->next_run != NULL
						&& runp->next_run->aiocbp->__abs_prio >= prio)
			runp = runp->next_run;

		newrequest->next_run = runp->next_run;
		runp->next_run = newrequest;
	}
}

// proc filesystem functions

static int rtai_read_aio(char *page, char **start, off_t off, int count,
													int *eof, void *data)
{
	PROC_PRINT_VARS;
	int threads_idle,threads_running;
	struct requestlist *req = 0;

	threads_idle = atomic_read(&idle_thread_count);
	threads_running = atomic_read(&nthreads);

	PROC_PRINT("\nRTAI AIO Information.\n\n");

	PROC_PRINT("Kernel threads:   Total = %d\n",threads_running+threads_idle);
	PROC_PRINT("                Running = %d\n",threads_running);
	PROC_PRINT("                   Idle = %d\n",threads_idle);
	PROC_PRINT("                   Max. = %d\n",aio_threads);

	PROC_PRINT("Requests:\n");
	PROC_PRINT("FD OP PRIO BUF_PTR BUF_SIZE OFFSET MODE FLAGS\n");

	req = runlist;
	while(req != NULL){
		PROC_PRINT("%d %d %d %p %d %ld %x %x\n",
								req->aiocbp->aio_fildes,
								req->aiocbp->aio_lio_opcode,
								req->aiocbp->aio_reqprio,
								req->aiocbp->aio_buf,
								req->aiocbp->aio_nbytes,
								req->aiocbp->aio_offset,
								req->aiocbp->aio_mode,
								req->aiocbp->aio_flags);

		req = req->next_run;
	}

	PROC_PRINT("FD OP PRIO BUF_PTR BUF_SIZE OFFSET MODE FLAGS\n");

	req = requests;
	while(req != NULL){
		PROC_PRINT("%d %d %d %p %d %ld %x %x\n",
								req->aiocbp->aio_fildes,
								req->aiocbp->aio_lio_opcode,
								req->aiocbp->aio_reqprio,
								req->aiocbp->aio_buf,
								req->aiocbp->aio_nbytes,
								req->aiocbp->aio_offset,
								req->aiocbp->aio_mode,
								req->aiocbp->aio_flags);

		req = req->next_run;
	}

	PROC_PRINT_DONE;
}  /* End function - rtai_read_aio */


static int rtai_read_aio_wrapper(char *page, char **start, off_t off, int count,
													int *eof, void *data)
{
	RT_TASK* rt_task;
	int res = -1;
	int old_pol;

	old_pol = current->policy;

	current->policy = SCHED_RR;

	rt_task = lxk_task_init(current->pid,0,0,0);

	if(rt_task != 0){
		// we have a RT_TASK so we can wait on the mutex
		lxk_sem_wait( &__aio_requests_mutex );

		res = rtai_read_aio(page,start,off,count,eof,data);

		lxk_sem_signal( &__aio_requests_mutex );

		// delete the task again
		lxk_task_delete(rt_task);
	}

	current->policy = old_pol;

	return res;
}

int rtai_proc_aio_register(void)
{
	struct proc_dir_entry *proc_aio_ent;

	proc_aio_ent = create_proc_entry("aio", S_IFREG|S_IRUGO|S_IWUSR, rtai_proc_root);
	if (!proc_aio_ent) {
		printk("Unable to initialize /proc/rtai/aio\n");
		return(-1);
	}

	proc_aio_ent->read_proc = rtai_read_aio_wrapper;

	return(0);
}  /* End function - rtai_proc_aio_register */

void rtai_proc_aio_unregister(void)
{
	remove_proc_entry("aio", rtai_proc_root);
}  /* End function - rtai_proc_aio_unregister */

