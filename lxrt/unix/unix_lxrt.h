/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)

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
*/

#ifndef _UNIX_LXRT_H_
#define _UNIX_LXRT_H_

#include <string.h>

#include <rtai_shm.h>
#include <rtai_lxrt.h>

#define RT_END_UNIX_SRV 	 0
#define RT_SCANF  		 1
#define RT_PRINTF		 2
#define RT_OPEN			 3
#define RT_CLOSE		 4
#define RT_WRITE		 5
#define RT_READ			 6
#define RT_SELECT		 7
#define RT_LSEEK 		 8
#define RT_SYNC 		 9
#define RT_IOCTL 		10

#define UNIXTSK ((RT_TASK *)shm[0])
#define SHM (shm + 1)

int *shm;

static inline void rt_start_unix_server(void *task, int rt_prio, int shmsize)
{
	int pid;
	char args[44];
	sprintf(args, "%10d %10lu %10d %10d", pid = getpid(), (unsigned long)task, rt_prio, shmsize);
	args[10] = args[21] = args[32] = args[43] = 0;
	if (!fork()) {
		execl("./unix_lxrt", "unix_lxrt", args, args + 11, args + 22, args + 33, NULL);
	} 
	shm = (void *)rtai_malloc((nam2num("UM") << 16) | pid, shmsize + sizeof(RT_TASK *));
	rt_return(UNIXTSK = rt_receive(0, &pid), 0);
}

static inline int rt_end_unix_server(void)
{
	int ret;
	rt_rpc(UNIXTSK, RT_END_UNIX_SRV, &ret);
	rtai_free((nam2num("UM") << 16) | getpid(), shm);
	return ret;
}

static inline int rt_scanf(const char *fmt, ...)
{
	volatile int nargs, nread, scn, indx;
	for (nargs = nread = 0; (indx = fmt[nread]); nread++) {
		if (indx == '%') {
			nargs++; 
		}
	}
	nread = scn = indx = 0;
	{
		struct { void *args[nargs]; } args; 
		int scr;
		do {
			memcpy(&args, (int *)&fmt + nread + 1, (nargs - nread)*sizeof(int)); 
			do {
				rt_rpc(UNIXTSK, RT_SCANF, &scr);
			} while ((scr = sscanf ((char *)SHM, fmt + indx, args)) <= 0);

			nread += scr;
			while (scn < nread) {
				while (fmt[indx++] != '%' && fmt[indx]);
				if (fmt[indx]) {
					scn++;
				}
			}
			while (fmt[indx] && fmt[indx] != '%') {
				indx++;
			}
		} while (nread < nargs);
	}
	return nread;
}

static inline int rt_printf(const char *fmt, ...)
{
	va_list args;
	unsigned int nchar, ret;
	va_start(args, fmt);
	nchar = vsprintf((char *)SHM, fmt, args);
	va_end(args);
	rt_rpc(UNIXTSK, RT_PRINTF, &ret);
	return nchar;
}

static inline int rt_open(const char *pathname, int flags, mode_t mode)
{
	int ret;
	SHM[0] = flags;
	SHM[1] = mode;
	strcpy((char *)(SHM + 2), pathname);
	rt_rpc(UNIXTSK, RT_OPEN, &ret);
	return ret;
}

static inline int rt_close(int fd)
{
	int ret;
	SHM[0] = fd;
	rt_rpc(UNIXTSK, RT_CLOSE, &ret);
	return ret;
}

static inline int rt_write(int fd, void *buf, size_t count)
{
	int ret;
	SHM[0] = fd;
	SHM[1] = count;
	memcpy(SHM + 2, buf, count);
	rt_rpc(UNIXTSK, RT_WRITE, &ret);
	return ret;
}

static inline int rt_read(int fd, void *buf, size_t count)
{
	int ret;
	SHM[0] = fd;
	SHM[1] = count;
	rt_rpc(UNIXTSK, RT_READ, &ret);
	memcpy(buf, SHM + 2, count);
	return ret;
}

static inline int rt_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	int ret;
	struct args { int n; fd_set *readfds; fd_set *writefds; fd_set *exceptfds; struct timeval *timeout; fd_set rfds; fd_set wfds; fd_set efds; struct timeval tmo; } *arg;
	arg = (struct args *)SHM;
	arg->n = n;
	if ((arg->readfds = readfds)) {
		arg->rfds = *readfds;	
	}
	if ((arg->writefds = writefds)) {
		arg->wfds = *writefds;	
	}
	if ((arg->exceptfds = exceptfds)) {
		arg->efds = *exceptfds;	
	}
	if ((arg->timeout = timeout)) {
		arg->tmo = *timeout;	
	}
	rt_rpc(UNIXTSK, RT_SELECT, &ret);
	return ret;
}

static inline off_t rt_lseek(int fd, off_t offset, int whence)
{
	int ret;
	SHM[0] = fd;
	SHM[1] = (int)offset;
	SHM[2] = whence;
	rt_rpc(UNIXTSK, RT_LSEEK, &ret);
	return (off_t) ret;
}

static inline int rt_sync(void)
{
	int ret;
	rt_rpc(UNIXTSK, RT_SYNC, &ret);
	return ret;
}

// I found it difficult to have a single catch all ioctl. So I decided to
// resort to a 4th arg to understand what argp is about. If size is zero
// argp is a single argument and there is no need to copy. If size is > 0
// argp is a pointer to something of that size and it must be copied.
// Not totally the same as standard UNIX, but since ioctl is a var arg fun the
// call should be compatible and accepted also without the rt_ in front of it.
static inline int rt_ioctl(int d, int request, unsigned long argp, int size)
{
	int ret;
	SHM[0] = d;
	SHM[1] = request;
	if (size) {
		memcpy(SHM + 3, (void *)argp, size);
		SHM[2] = (int)(SHM + 3);
	} else {	
		SHM[2] = argp;
	}
	rt_rpc(UNIXTSK, RT_IOCTL, &ret);
	return ret;
}

#endif
