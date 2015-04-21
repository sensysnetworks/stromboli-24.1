/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: aio_suspend.c,v 1.1.1.1 2004/06/06 14:02:36 rpm Exp $
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
#include "rtai_sched.h"


int
aio_suspend (const struct aiocb *const list[], int nent, const struct timespec *timeout)
{
	struct waitlist waitlist[nent];
	struct requestlist *requestlist[nent];
	CND cond;
	int cnt;
	int result = 0;
	int dummy;
	int none = 1;

	rt_cond_init(&cond);

	/* Request the mutex.  */
	rt_sem_wait(&__aio_requests_mutex);

	/* There is not yet a finished request.  Signal the request that
	we are working for it.  */

	for (cnt = 0; cnt < nent; ++cnt)
	{
 		if (list[cnt] != NULL && list[cnt]->__error_code == EINPROGRESS)
 		{
			requestlist[cnt] = __aio_find_req((struct aiocb*)list[cnt]);
			
			if (requestlist[cnt] != NULL)
			{
				waitlist[cnt].cond = &cond;
				waitlist[cnt].next = requestlist[cnt]->waiting;
				waitlist[cnt].counterp = &dummy;
				waitlist[cnt].sigevp = NULL;
				waitlist[cnt].caller_pid = 0;	/* Not needed.  */
				requestlist[cnt]->waiting = &waitlist[cnt];
				none = 0;
			}
		}
	}

	/* If there is a not finished request wait for it.  */
	if (!none)
	{
		// int oldstate;

		if (timeout == NULL)
		{
			result = rt_cond_wait(&cond, &__aio_requests_mutex);
		}
		else
		{
			RTIME wait_time;
			wait_time = timeout->tv_sec * 1000000000 + timeout->tv_nsec;
			result = rt_cond_wait_timed( &cond, &__aio_requests_mutex, wait_time);
		}

		/* Now remove the entry in the waiting list for all requests
		which didn't terminate.  */
		for (cnt = 0; cnt < nent; ++cnt)
		{
			if (list[cnt] != NULL && list[cnt]->__error_code == EINPROGRESS
				&& requestlist[cnt] != NULL)
			{
				struct waitlist **listp = &requestlist[cnt]->waiting;
				/* There is the chance that we cannot find our entry anymore.
				This could happen if the request terminated and restarted
				again.  */
				while (*listp != NULL && *listp != &waitlist[cnt])
					listp = &(*listp)->next;

				if (*listp != NULL)
					*listp = (*listp)->next;
			}
		}

		/* Release the conditional variable.  */
		if( rt_cond_delete( &cond ) != 0) {
			/* This must never happen.  */
			 rt_task_suspend(rt_whoami());
		}

		if (result != 0)
		{
			/* An error occurred.  Possibly it's EINTR.  We have to translate
			the timeout error report of `pthread_cond_timedwait' to the
			form expected from `aio_suspend'.  */
			if (result == ETIMEDOUT){
				//__set_errno (EAGAIN);
			}
			result = -EAGAIN;
		}
	}

	/* Release the mutex.  */
	rt_sem_signal(&__aio_requests_mutex);

	return result;
}
