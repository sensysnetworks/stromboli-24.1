/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: aio_cancel.c,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $
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


int
aio_cancel(int fildes,struct aiocb *aiocbp)
{
	struct requestlist *req = NULL;
	int result = AIO_ALLDONE;

	/* Request the mutex.  */
	rt_sem_wait(&__aio_requests_mutex);

	/* We are asked to cancel a specific AIO request.  */
	if (aiocbp != NULL)
	{
		/* If the AIO request is not for this descriptor it has no value
		to look for the request block.  */
		if (aiocbp->aio_fildes == fildes)
		{
			struct requestlist *last = NULL;

			req = __aio_find_req_fd (fildes);

			if (req == NULL)
			{
			not_found:
				rt_sem_signal(&__aio_requests_mutex);
				return -EINVAL;	// Should have been errno = EINVAL and return -1
			}

			while (req->aiocbp != aiocbp)
			{
				last = req;
				req = req->next_prio;
				if (req == NULL)
					goto not_found;
			}

			/* Don't remove the entry if a thread is already working on it.  */
			if (req->running == allocated)
			{
				result = AIO_NOTCANCELED;
				req = NULL;
			}
			else
			{
				/* We can remove the entry.  */
				__aio_remove_request (last, req, 0);

				result = AIO_CANCELED;

				req->next_prio = NULL;
			}
		}
	}
	else
	{
		/* Find the beginning of the list of all requests for this
		desriptor.  */
		req = __aio_find_req_fd (fildes);

		/* If any request is worked on by a thread it must be the first.
		So either we can delete all requests or all but the first.  */
		if (req != NULL)
		{
			if (req->running == allocated)
			{
				struct requestlist *old = req;
				req = req->next_prio;
				old->next_prio = NULL;

				result = AIO_NOTCANCELED;

				if (req != NULL)
					__aio_remove_request (old, req, 1);
			}
			else
			{
				result = AIO_CANCELED;

				/* We can remove the entry.  */
				__aio_remove_request (NULL, req, 1);
			}
		}
	}

	/* Mark requests as canceled and send signal.  */
	while (req != NULL)
	{
		struct requestlist *old = req;
		req->aiocbp->__error_code = ECANCELED;
		req->aiocbp->__return_value = -1;
		__aio_notify( req );
		req = req->next_prio;
		__aio_free_request (old);
	}

	/* Release the mutex.  */
	rt_sem_signal(&__aio_requests_mutex);

	return result;
}
