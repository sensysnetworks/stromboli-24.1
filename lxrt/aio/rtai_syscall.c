/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: rtai_syscall.c,v 1.1.1.1 2004/06/06 14:02:37 rpm Exp $
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
#include "rtai_syscall.h"

int rt_open(const char *pathname, int flags, mode_t mode)
{
	struct aiocb cbs_open;
	struct aiocb* cbp_open;

	memset(&cbs_open,0,sizeof(cbs_open));
		
	cbs_open.aio_fildes = -1;
	cbs_open.aio_buf = (char*)pathname; /* we have to lose the const, to get rit of a warning */
	cbs_open.aio_mode =  mode;
	cbs_open.aio_flags = flags;
	cbs_open.aio_sigevent.sigev_notify = SIGEV_NONE;
	
	cbp_open = &cbs_open;

	aio_open( cbp_open );

	/* we loop until it is not in progress anymore */
	while(aio_error( &cbs_open ) == EINPROGRESS){
		aio_suspend ((const struct aiocb *const *) &cbp_open, 1, NULL);
	}

	return aio_return( &cbs_open );
}

ssize_t rt_pwrite(int fd, const void *buf, size_t count, off_t offset)
{
	struct aiocb cbs_write;
	struct aiocb* cbp_write;

	memset(&cbs_write,0,sizeof(cbs_write));
		
	cbs_write.aio_fildes = fd;
	cbs_write.aio_buf = (char*)buf; /* we have to lose the const, to get rit of a warning */
	cbs_write.aio_nbytes = count;
	cbs_write.aio_offset = offset;
	cbs_write.aio_sigevent.sigev_notify = SIGEV_NONE;
	
	cbp_write = &cbs_write;

	aio_write( cbp_write );

	/* we loop until it is not in progress anymore */
	while(aio_error( &cbs_write ) == EINPROGRESS){
		aio_suspend ((const struct aiocb *const *) &cbp_write, 1, NULL);
	}

	return aio_return( &cbs_write );
}

ssize_t rt_pread(int fd, void *buf, size_t count,off_t offset)
{
	struct aiocb cbs_read;
	struct aiocb* cbp_read;

	memset(&cbs_read,0,sizeof(cbs_read));
		
	cbs_read.aio_fildes = fd;
	cbs_read.aio_buf = (char*)buf; /* we have to lose the const, to get rit of a warning */
	cbs_read.aio_nbytes = count;
	cbs_read.aio_offset = offset;
	cbs_read.aio_sigevent.sigev_notify = SIGEV_NONE;
	
	cbp_read = &cbs_read;

	aio_read( cbp_read );

	/* we loop until it is not in progress anymore */
	while(aio_error( &cbs_read ) == EINPROGRESS){
		aio_suspend ((const struct aiocb *const *) &cbp_read, 1, NULL);
	}

	return aio_return( &cbs_read );
}

int rt_close(int fd)
{
	struct aiocb cbs_close;
	struct aiocb* cbp_close;

	memset(&cbs_close,0,sizeof(cbs_close));
		
	cbs_close.aio_fildes = fd;
	cbs_close.aio_sigevent.sigev_notify = SIGEV_NONE;
	
	cbp_close = &cbs_close;

	aio_close( cbp_close );

	/* we loop until it is not in progress anymore */
	while(aio_error( &cbs_close ) == EINPROGRESS){
		aio_suspend ((const struct aiocb *const *) &cbp_close, 1, NULL);
	}

	return aio_return( &cbs_close );
}

