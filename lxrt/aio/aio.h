/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: aio.h,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $
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

/*
 * ISO/IEC 9945-1:1996 6.7: Asynchronous Input and Output
 */


#ifndef __AIO_H__
#define __AIO_H__

#include <linux/types.h>
#include <asm/siginfo.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <asm/fcntl.h>
#include <linux/linkage.h>

/* GCC can always grok prototypes.  For C++ programs we add throw()
   to help it optimize the function calls.  But this works only with
   gcc 2.8.x and egcs.  */

#if defined __cplusplus
# define __THROW       throw ()
# define __BEGIN_DECLS  extern "C" {
# define __END_DECLS    }
#else
#	define __THROW
#	define __BEGIN_DECLS
#	define __END_DECLS
#endif


__BEGIN_DECLS


/* map to kernel type */
typedef loff_t off64_t;

#define ECANCELED       158     /* AIO operation canceled */

/* Maximum amount by which a process can descrease its asynchronous I/O
   priority level.  */
#define AIO_PRIO_DELTA_MAX      20

/* Asynchronous I/O control block.  */
struct aiocb
{
	int aio_fildes;		/* File desriptor.  */
	int aio_lio_opcode;		/* Operation to be performed.  */
	int aio_reqprio;		/* Request priority offset.  */
	volatile void *aio_buf;	/* Location of buffer.  */
	size_t aio_nbytes;		/* Length of transfer.  */
	struct sigevent aio_sigevent;	/* Signal number and value.  */

	/* Internal members.  */
	struct aiocb *__next_prio;
	int __abs_prio;
	int __policy;
	int __error_code;
	ssize_t __return_value;
	
	int aio_mode;
	int aio_flags;
	struct timespec *aio_timeout;

#ifndef __USE_FILE_OFFSET64
	off_t aio_offset;		/* File offset.  */
#else
	off64_t aio_offset;		/* File offset.  */
#endif
};

/* Return values of cancelation function.  */
enum
{
	AIO_CANCELED,
#define AIO_CANCELED AIO_CANCELED
	AIO_NOTCANCELED,
#define AIO_NOTCANCELED AIO_NOTCANCELED
	AIO_ALLDONE
#define AIO_ALLDONE AIO_ALLDONE
};


/* Operation codes for `aio_lio_opcode'.  */
enum
{
	LIO_READ,
#define LIO_READ LIO_READ
	LIO_WRITE,
#define LIO_WRITE LIO_WRITE
	LIO_NOP,
#define LIO_NOP LIO_NOP
	LIO_DSYNC,
#define LIO_DSYNC LIO_DSYNC
	LIO_SYNC,
#define LIO_SYNC LIO_SYNC
	LIO_OPEN,
#define LIO_OPEN LIO_OPEN
	LIO_CLOSE,
#define LIO_CLOSE LIO_CLOSE
};


/* Synchronization options for `lio_listio' function.  */
enum
{
	LIO_WAIT,
#define LIO_WAIT LIO_WAIT
	LIO_NOWAIT
#define LIO_NOWAIT LIO_NOWAIT
};


/* Enqueue read request for given number of bytes and the given priority.  */
extern int aio_read (struct aiocb *__aiocbp) __THROW;
/* Enqueue write request for given number of bytes and the given priority.  */
extern int aio_write (struct aiocb *__aiocbp) __THROW;

/* Enqueue read request for given number of bytes and the given priority.  */
extern int aio_open (struct aiocb *__aiocbp) __THROW;
/* Enqueue write request for given number of bytes and the given priority.  */
extern int aio_close (struct aiocb *__aiocbp) __THROW;

/* Initiate list of I/O requests.  */
extern int lio_listio (int __mode,
		       struct aiocb *const __list[],
		       int __nent, struct sigevent * __sig) __THROW;

/* Retrieve error status associated with AIOCBP.  */
extern int aio_error (const struct aiocb *__aiocbp) __THROW;

/* Return status associated with AIOCBP.  */
extern ssize_t aio_return (struct aiocb *__aiocbp) __THROW;

/* Try to cancel asynchronous I/O requests outstanding against file
   descriptor FILDES.  */
extern int aio_cancel (int __fildes, struct aiocb *__aiocbp) __THROW;

/* Suspend calling thread until at least one of the asynchronous I/O
   operations referenced by LIST has completed.  */
extern int aio_suspend (const struct aiocb *const __list[], int __nent,
			const struct timespec *__timeout) __THROW;

/* Force all operations associated with file desriptor described by
   `aio_fildes' member of AIOCBP.  */
extern int aio_fsync (int __operation, struct aiocb *__aiocbp) __THROW;

__END_DECLS

#endif /* !__AIO_H__ */
