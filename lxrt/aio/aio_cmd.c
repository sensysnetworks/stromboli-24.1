/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: aio_cmd.c,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $
 *
 * Copyright: (c)2001 Erwin Rol <erwin@muffin.org>
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
aio_open(struct aiocb *aiocbp)
{
	return (__aio_enqueue_request ( aiocbp, LIO_OPEN) == NULL ? -1 : 0);
}

int
aio_close(struct aiocb *aiocbp)
{
	return (__aio_enqueue_request ( aiocbp, LIO_CLOSE) == NULL ? -1 : 0);
}


int
aio_write( struct aiocb *aiocbp )
{
	return (__aio_enqueue_request ( aiocbp, LIO_WRITE) == NULL ? -1 : 0);
}

ssize_t
aio_return (struct aiocb *aiocbp)
{
	return aiocbp->__return_value;
}

int
aio_read( struct aiocb *aiocbp )
{
	return (__aio_enqueue_request ( aiocbp, LIO_READ) == NULL ? -1 : 0);
}

// XXX fix me
#define O_DSYNC O_SYNC

int
aio_fsync( int op, struct aiocb *aiocbp )
{
	if (op != O_DSYNC && op != O_SYNC)
	{
		//__set_errno (EINVAL);
		return -EINVAL;
	}

	op = O_SYNC ? LIO_SYNC : LIO_DSYNC;

	return (__aio_enqueue_request ( aiocbp, op) == NULL ? -1 : 0);
}

int
aio_error( const struct aiocb *aiocbp )
{
	return aiocbp->__error_code;
}
