/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: lio_listio.c,v 1.1.1.1 2004/06/06 14:02:36 rpm Exp $
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

#include "aio.h"
#include "aio_misc.h"
#include "rt_mem_mgr.h"

/* We need this special structure to handle asynchronous I/O.  */
struct async_waitlist
{
	int counter;
	struct sigevent sigev;
	struct waitlist list[0];
};


int
lio_listio (int mode,struct aiocb *const list[],int nent,struct sigevent *sig)
{
	struct sigevent defsigev;
	struct requestlist *requests[nent];
	int cnt;
	volatile int total = 0;
	int result = 0;

	/* Check arguments.  */
	if (mode != LIO_WAIT && mode != LIO_NOWAIT)
	{
		//__set_errno (EINVAL);
		return -EINVAL;
	}

	if (sig == NULL)
	{
		defsigev.sigev_notify = SIGEV_NONE;
		sig = &defsigev;
	}

	/* Request the mutex.  */
	rt_sem_wait(&__aio_requests_mutex);

	/* Now we can enqueue all requests.  Since we already acquired the
	mutex the enqueue function need not do this.  */
	for (cnt = 0; cnt < nent; ++cnt)
		if (list[cnt] != NULL && list[cnt]->aio_lio_opcode != LIO_NOP)
		{
			list[cnt]->aio_sigevent.sigev_notify = SIGEV_NONE;
			requests[cnt] =  __aio_enqueue_request (list[cnt], list[cnt]->aio_lio_opcode);

			if (requests[cnt] != NULL)
				/* Successfully enqueued.  */
				++total;
			else
				/* Signal that we've seen an error.  `errno' and the error code
				of the aiocb will tell more.  */
				result = -1;
		}


	if (total == 0)
	{
		/* We don't have anything to do except signalling if we work
		asynchronously.  */

		/* Release the mutex.  We do this before raising a signal since the
		signal handler might do a `siglongjmp' and then the mutex is
		locked forever.  */
		rt_sem_signal(&__aio_requests_mutex);

		if (mode == LIO_NOWAIT)
			__aio_notify_only( sig,	sig->sigev_notify == SIGEV_SIGNAL ? /* FIXME getpid () */ 0 : 0);

		return result;
	}
	else if (mode == LIO_WAIT)
	{
		CND cond;
		struct waitlist waitlist[nent];

		rt_cond_init( &cond );

		total = 0;
		for (cnt = 0; cnt < nent; ++cnt) {
			if (list[cnt] != NULL && list[cnt]->aio_lio_opcode != LIO_NOP && requests[cnt] != NULL)
			{
				waitlist[cnt].cond = &cond;
				waitlist[cnt].next = requests[cnt]->waiting;
				waitlist[cnt].counterp = &total;
				waitlist[cnt].sigevp = NULL;
				waitlist[cnt].caller_pid = 0;	/* Not needed.  */
				requests[cnt]->waiting = &waitlist[cnt];
				++total;
			}
		}

		while (total > 0){
			rt_cond_wait(&cond, &__aio_requests_mutex);
		}

		/* Release the conditional variable.  */
		if( rt_cond_delete( &cond ) != 0) {
			/* this should _NEVER_ happen */
			rt_task_suspend(rt_whoami() );
		}
	}
	else
	{
		struct async_waitlist *waitlist;

		waitlist = (struct async_waitlist *) rt_malloc( sizeof (struct async_waitlist) + (nent * sizeof (struct waitlist)));

		if (waitlist == NULL)
		{
			//__set_errno (EAGAIN);
			result = -EAGAIN;
		}
		else
		{
			pid_t caller_pid = sig->sigev_notify == SIGEV_SIGNAL ? /* FIXME getpid ()*/ 0 : 0;
			total = 0;

			for (cnt = 0; cnt < nent; ++cnt)
			{
				if (list[cnt] != NULL && list[cnt]->aio_lio_opcode != LIO_NOP && requests[cnt] != NULL)
				{
					waitlist->list[cnt].cond = NULL;
					waitlist->list[cnt].next = requests[cnt]->waiting;
					waitlist->list[cnt].counterp = &waitlist->counter;
					waitlist->list[cnt].sigevp = &waitlist->sigev;
					waitlist->list[cnt].caller_pid = caller_pid;
					requests[cnt]->waiting = &waitlist->list[cnt];
					++total;
				}
			}

			waitlist->counter = total;
			waitlist->sigev = *sig;
		}
	}

	/* Release the mutex.  */
	rt_sem_signal(&__aio_requests_mutex);

	return result;
}
