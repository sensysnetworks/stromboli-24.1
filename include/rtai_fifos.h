/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#ifndef _RTAI_NEWFIFOS_H_
#define _RTAI_NEWFIFOS_H_

#define RESET		 1
#define RESIZE		 2
#define SUSPEND_TIMED	 3
#define OPEN_SIZED	 4
#define READ_ALL_AT_ONCE 5
#define READ_TIMED	 6
#define WRITE_TIMED	 7
#define RTF_SEM_INIT	 8
#define RTF_SEM_WAIT	 9
#define RTF_SEM_TRYWAIT	10
#define RTF_SEM_TIMED_WAIT	11
#define RTF_SEM_POST	12
#define RTF_SEM_DESTROY	13
#define SET_ASYNC_SIG	14
#define EAVESDROP	19
#define OVRWRITE	20
#define READ_IF         21
#define WRITE_IF        22

#define RTF_GET_N_FIFOS		15
#define RTF_GET_FIFO_INFO	16
#define RTF_CREATE_NAMED	17
#define RTF_NAME_LOOKUP		18

#define RTF_NAMELEN 15

struct rt_fifo_info_struct{
    	unsigned int fifo_number;
	unsigned int size;
	unsigned int opncnt;
	char name[RTF_NAMELEN+1];
};

struct rt_fifo_get_info_struct{
    	unsigned int fifo;
	unsigned int n;
	struct rt_fifo_info_struct *ptr;
};

#ifdef __KERNEL__

#define RTAI_MAJOR 150

extern int rtf_init(void);

/* Attach a handler to an RT-FIFO.
 *
 * Allow function handler to be called when a user process reads or writes to 
 * the FIFO. When the function is called, it is passed the fifo number as the 
 * argument.
 */

extern int rtf_create_handler(unsigned int fifo,	/* RT-FIFO */
		int (*handler)(unsigned int fifo)	/* function to be called */);


/* Define to make compatible the call of an extended arguments handler, i.e. one
 * prototypede as "int handler(unsigned int fifo, int rw);", so that it can be
 * used with rtf_create. To install such a handler:
 * rtf_create_handler(fifo, X_FIFO_HANDLER(handler)
 */

#define X_FIFO_HANDLER(handler) ((int (*)(unsigned int))(handler))


/* Create an RT-FIFO.
 * 
 * An RT-FIFO fifo is created with initial size of size.
 * Return value: On success, 0 is returned. On error, -1 is returned.
 */
#undef rtf_create
extern int rtf_create(unsigned int fifo, int size);

/* Create an RT-FIFO with a name.
 *
 * An RT-FIFO is created with a name of name, it will be allocated
 * the first unused minor number and will have a default size.
 * Return value: On success, the allocated minor number is returned. 
 *               On error, -errno is returned.
 */

extern int rtf_create_named(const char *name);

/* Look up a named RT-FIFO.
 *
 * Find the RT-FIFO with the name name.
 * Return value: On success, the minor number is returned. 
 *               On error, -errno is returned.
 */

extern int rtf_getfifobyname(const char *name);

/* Reset an RT-FIFO.
 * 
 * An RT-FIFO fifo is reset by setting its buffer pointers to zero, so
 * that any existing data is discarded and the fifo started anew like at its
 * creation.
 */

extern int rtf_reset(unsigned int fifo);



/* destroy an RT-FIFO.
 * 
 * Return value: On success, 0 is returned.
 */

extern int rtf_destroy(unsigned int fifo);


/* Resize an RT-FIFO.
 * 
 * Return value: size is returned on success. On error, a negative value
 * is returned.
 */

extern int rtf_resize(unsigned int minor, int size);


/* Write to an RT-FIFO.
 *
 * Try to write count bytes to an FIFO. Returns the number of bytes written.
 */

extern int rtf_put(unsigned int fifo,	/* RT-FIFO */
		void * buf,		/* buffer address */
	       	int count		/* number of bytes to write */);



/* Write to an RT-FIFO, over writing if there is not enough space.
 *
 * Try to write count bytes to an FIFO. Returns 0.
 */

extern int rtf_ovrwr_put(unsigned int fifo,	/* RT-FIFO */
		void * buf,		/* buffer address */
	       	int count		/* number of bytes to write */);


/* Write atomically to an RT-FIFO.
 *
 * Try to write count bytes in block to an FIFO. Returns the number of bytes
 * written.
 */

extern int rtf_put_if (unsigned int fifo,       /* RT-FIFO */
                void * buf,                     /* buffer address */
                int count                       /* number of bytes to write */);


/* Read from an RT-FIFO.
 *
 * Try to read count bytes from a FIFO. Returns the number of bytes read.
 */

extern int rtf_get(unsigned int fifo,	/* RT-FIFO */
		void * buf, 		/* buffer address */
		int count		/* number of bytes to read */);


/* Atomically read from an RT-FIFO.
 *
 * Try to read count bytes in a block from an FIFO. Returns the number of bytes read.
 */

int rtf_get_if(unsigned int fifo,       /* RT-FIFO */
	    void * buf,                 /* buffer address */
            int count           /* number of bytes to read */);

/* 
 * Preview the an RT-FIFO content.
 */

extern int rtf_evdrp(unsigned int fifo,	/* RT-FIFO */
		void * buf, 		/* buffer address */
		int count		/* number of bytes to read */);


/* Open an RT-FIFO semaphore.
 *
 */

extern int rtf_sem_init(unsigned int fifo,	/* RT-FIFO */
		int value			/* initial semaphore value */);


/* Post to an RT-FIFO semaphore.
 *
 */

extern int rtf_sem_post(unsigned int fifo	/* RT-FIFO */);


/* Try to acquire an RT-FIFO semaphore.
 *
 */

extern int rtf_sem_trywait(unsigned int fifo	/* RT-FIFO */);


/* Destroy an RT-FIFO semaphore.
 *
 */

extern int rtf_sem_destroy(unsigned int fifo	/* RT-FIFO */);


/* Just for compatibility with earlier rtai_fifos releases. No more bh and user
buffers. Fifos are now awakened immediately and buffers > 128K are vmalloced */

#define rtf_create_using_bh(fifo, size, bh_list) rtf_create(fifo, size)
#define rtf_create_using_bh_and_usr_buf(fifo, buf, size, bh_list) rtf_create(fifo, size)
#define rtf_destroy_using_usr_buf(fifo) rtf_destroy(fifo)

#else
/*******************************************************************
* User Space API
********************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

static inline int rtf_reset(int fd)
{
	return ioctl(fd, RESET);
}

static inline int rtf_resize(int fd, int size)
{
	return ioctl(fd, RESIZE, size);
}

static inline void rtf_suspend_timed(int fd, int ms_delay)
{
	ioctl(fd, SUSPEND_TIMED, ms_delay);
}

static inline int rtf_open_sized(const char *dev, int perm, int size)
{
	int fd, err;

	if( (fd = open(dev, perm)) < 0) { 
		return fd;
	}
	if( (err = ioctl(fd, RESIZE, size)) < 0) {
	    	close(fd);
		return err;
	}
	return fd; 
}

static inline int rtf_evdrp(int fd, void *buf, int count)
{
	struct { void *buf; int count; } args = { buf, count };
	return ioctl(fd, EAVESDROP, &args);
}

static inline int rtf_read_all_at_once(int fd, void *buf, int count)
{
	struct { void *buf; int count; } args = { buf, count };
	return ioctl(fd, READ_ALL_AT_ONCE, &args);
}

static inline int rtf_read_timed(int fd, void *buf, int count, int ms_delay)
{
	struct { void *buf; int count, delay; } args = { buf, count, ms_delay };
	return ioctl(fd, READ_TIMED, &args);
}

static inline int rtf_write_timed(int fd, void *buf, int count, int ms_delay)
{
	struct { void *buf; int count, delay; } args = { buf, count, ms_delay };
	return ioctl(fd, WRITE_TIMED, &args);
}

static inline int rtf_overwrite(int fd, void *buf, int count)
{
	struct { void *buf; int count; } args = { buf, count };
	return ioctl(fd, OVRWRITE, &args);
}

static inline void rtf_sem_init(int fd, int value)
{
	ioctl(fd, RTF_SEM_INIT, value);
}

static inline int rtf_sem_wait(int fd)
{
	return ioctl(fd, RTF_SEM_WAIT);
}

static inline int rtf_sem_trywait(int fd)
{
	return ioctl(fd, RTF_SEM_TRYWAIT);
}

static inline int rtf_sem_timed_wait(int fd, int ms_delay)
{
	return ioctl(fd, RTF_SEM_TIMED_WAIT, ms_delay);
}

static inline void rtf_sem_post(int fd)
{
	ioctl(fd, RTF_SEM_POST);
}

static inline void rtf_sem_destroy(int fd)
{
	ioctl(fd, RTF_SEM_DESTROY);
}

static inline void rtf_set_async_sig(int fd, int signum)
{
	ioctl(fd, SET_ASYNC_SIG, signum);
}

/*
 * Support for named FIFOS : Ian Soanes (ians@zentropix.com)
 * Based on ideas from Stuart Hughes and David Schleef
 */
static inline int rtf_getfifobyname(const char *name)
{
    	int fd, minor;
	char nm[RTF_NAMELEN+1];

	if (strlen(name) > RTF_NAMELEN) {
	    	return -1;
	}
	if ((fd = open("/dev/rtf0", O_RDONLY)) < 0) {
	    	return fd;
	}
	strncpy(nm, name, RTF_NAMELEN+1);
	minor = ioctl(fd, RTF_NAME_LOOKUP, nm);
	close(fd);
	return minor;
}

static inline int rtf_create_named(const char *name)
{
	int fd, minor;
	char nm[RTF_NAMELEN+1];

	if (strlen(name) > RTF_NAMELEN) {
	    	return -1;
	}
	if ((fd = open("/dev/rtf0", O_RDONLY)) < 0) { 
		return fd;
	}
	strncpy(nm, name, RTF_NAMELEN+1);
	minor = ioctl(fd, RTF_CREATE_NAMED, nm);
	close(fd);
	return minor;
}

#endif

#endif
