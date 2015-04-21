/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: aio_misc.h,v 1.1.1.1 2004/06/06 14:02:36 rpm Exp $
 *
 * Copyright: (C) 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
 *            This file was part of the GNU C Library.
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

#ifndef __AIO_MISC_H__
#define __AIO_MISC_H__

#include "rtai_lxk.h"
#include "aio.h"

/* Since the list is global we need a mutex protecting it.  */
extern SEM __aio_requests_mutex;

/* When you add a request to the list and there are idle threads present,
   you signal this condition variable. When a thread finishes work, it waits
   on this condition variable for a time before it actually exits. */
extern CND __aio_new_request_notification;

/* Used to synchronize.  */
struct waitlist
{
	struct waitlist *next;

	CND *cond;
	volatile int *counterp;
	/* The next field is used in asynchronous `lio_listio' operations.  */
	struct sigevent *sigevp;
	/* XXX See requestlist, it's used to work around the broken signal handling in Linux.  */
	pid_t caller_pid;
	RT_TASK* caller_task;
};

/* Status of a request.  */
enum
{
	no,
	queued,
	yes,
	allocated,
	done
};


/* Used to queue requests..  */
struct requestlist
{
	int running;

	struct requestlist *last_fd;
	struct requestlist *next_fd;
	struct requestlist *next_prio;
	struct requestlist *next_run;

	/* Pointer to the actual data.  */
	struct aiocb *aiocbp;

	/* PID of the initiator thread.
	XXX This is only necessary for the broken signal handling on Linux.  */
	pid_t caller_pid;

	/* List of waiting processes.  */
	struct waitlist *waiting;
};


/* Enqueue request.  */
extern struct requestlist *__aio_enqueue_request (struct aiocb *aiocbp,
						  int operation);

/* Find request entry for given AIO control block.  */
extern struct requestlist *__aio_find_req (struct aiocb *elem);

/* Find request entry for given file descriptor.  */
extern struct requestlist *__aio_find_req_fd (int fildes);

/* Remove request from the list.  */
extern void __aio_remove_request (struct requestlist *last,
				  struct requestlist *req, int all);

/* Release the entry for the request.  */
extern void __aio_free_request (struct requestlist *req);

/* Notify initiator of request and tell this everybody listening.  */
extern void __aio_notify (struct requestlist *req);

/* Notify initiator of request.  */
extern int __aio_notify_only (struct sigevent *sigev, pid_t caller_pid);

/* Send the signal.  */
extern int __aio_sigqueue (int sig, const union sigval val, pid_t caller_pid);

extern int rt_create_thread(struct requestlist * req);
extern int lxk_create_thread(struct requestlist * req);

extern int handle_fildes_io(void *arg);
extern void free_res( void );

#ifdef CONFIG_PROC_FS

extern int rtai_proc_aio_register(void);

extern void rtai_proc_aio_unregister(void);

#endif


extern int aio_threads;			/* Maximal number of threads.  */
extern int aio_num_requests;					/* Number of expected simultanious requests. */
extern int aio_idle_time;		/* Number of seconds before idle thread terminates.  */

extern int aio_debug;				/* Not used.  */


#endif /* !__AIO_MISC_H__ */
