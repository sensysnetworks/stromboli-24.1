/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: aio_notify.c,v 1.1.1.1 2004/06/06 14:02:36 rpm Exp $
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

#if 0
static void *
notify_func_wrapper (void *arg)
{
	struct sigevent *sigev = arg;
	sigev->sigev_notify_function (sigev->sigev_value);
	return NULL;
}
#endif

int
__aio_notify_only (struct sigevent *sigev, pid_t caller_pid)
{
	int result = 0;

	/* Send the signal to notify about finished processing of the request.  */
	if (sigev->sigev_notify == SIGEV_THREAD)
	{
#if 0
		/* We have to start a thread.  */
		pthread_t tid;
		pthread_attr_t attr, *pattr;

		pattr = (pthread_attr_t *) sigev->sigev_notify_attributes;
		if (pattr == NULL)
		{
			pthread_attr_init (&attr);
			pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
			pattr = &attr;
		}

		if (pthread_create (&tid, pattr, notify_func_wrapper, sigev) < 0)
			result = -1;
#else
		result = -1;
#endif
	}
	else if (sigev->sigev_notify == SIGEV_SIGNAL){
		/* We have to send a signal.  */
#if 0
		if ( __aio_sigqueue( sigev->sigev_signo, sigev->sigev_value, caller_pid ) < 0 ){
			result = -1;
		}
#else
		result = -1;
#endif
	}

	return result;
}


void
__aio_notify (struct requestlist *req)
{
	struct waitlist *waitlist;
	struct aiocb *aiocbp = req->aiocbp;
	int result;

	if( ( result = __aio_notify_only (&aiocbp->aio_sigevent, req->caller_pid) ) != 0 )
	{
		/* XXX What shall we do if already an error is set by read/write/fsync?  */
		aiocbp->__error_code = -result;
		aiocbp->__return_value = -1;
	}

	/* Now also notify possibly waiting threads.  */
	waitlist = req->waiting;
	while (waitlist != NULL)
	{
		struct waitlist *next = waitlist->next;

		/* Decrement the counter.  This is used in both cases.  */
		--*waitlist->counterp;

		if (waitlist->sigevp == NULL) {
			rt_cond_signal( waitlist->cond );
		} else {
			/* This is part of a asynchronous `lio_listio' operation.  If
			this request is the last one, send the signal.  */
			if (*waitlist->counterp == 0)
			{
				__aio_notify_only( waitlist->sigevp, waitlist->caller_pid );
				/* This is tricky.  See lio_listio.c for the reason why
				this works.  */
				rt_free( (void *) waitlist->counterp );
			}
		}
		waitlist = next;
	}
}
