/*
COPYRIGHT (C) 2002  Thomas Leibner (leibner@t-online.de)
              2002  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#ifndef _RTAI_FIFOS_LXRT_H_
#define _RTAI_FIFOS_LXRT_H_

#include <config.h>
#include <rtai_fifos.h>
#include <rtai_declare.h>

#define  FUN_FIFOS_LXRT_INDX 10

#define _CREATE         0
#define _DESTROY        1
#define _PUT            2
#define _GET            3
#define _RESET          4
#define _RESIZE         5
#define _SEM_INIT       6
#define _SEM_DESTRY     7
#define _SEM_POST       8
#define _SEM_TRY        9
#ifdef CONFIG_RTAI_RTF_NAMED
#define _CREATE_NAMED  10
#define _GETBY_NAME    11
#endif

#ifdef __KERNEL__

extern int rtf_sem_delete(int);
#define rtf_sem_destroy rtf_sem_delete

#else

#include <string.h>

#include <rtai_lxrt.h>
				
DECLARE int rtf_create(unsigned int fifo, int size)
{
	struct { unsigned int fifo, size; } arg = { fifo, size }; 
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _CREATE, &arg).i[LOW];
}

DECLARE int rtf_destroy(unsigned int fifo)
{
	struct { unsigned int fifo; } arg = { fifo };
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _DESTROY, &arg).i[LOW];
}

DECLARE int rtf_put(unsigned int fifo, const void *buf, int count)
{
	char lbuf[count];
	struct { unsigned int fifo; void *buf; int count; } arg = { fifo, lbuf, count };
	memcpy(lbuf, buf, count);
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _PUT, &arg).i[LOW];
}

DECLARE int rtf_get(unsigned int fifo, void *buf, int count)
{
	int retval;
	char lbuf[count];
	struct { unsigned int fifo; void *buf; int count; } arg = { fifo, lbuf, count };
	retval = rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _GET, &arg).i[LOW];
	if (retval > 0) {
		memcpy(buf, lbuf, retval);
	}
	return retval;
}

DECLARE int rtf_reset_lxrt(unsigned int fifo)
{
	struct { unsigned int fifo; } arg = { fifo };
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _RESET, &arg).i[LOW];
}

DECLARE int rtf_resize_lxrt(unsigned int fifo, int size)
{
	struct { unsigned int fifo, size; } arg = { fifo, size }; 
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _RESIZE, &arg).i[LOW];
}

DECLARE int rtf_sem_init_lxrt(unsigned int fifo, int value)
{
	struct { unsigned int fifo, value; } arg = { fifo, value }; 
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _SEM_INIT, &arg).i[LOW];
}

DECLARE int rtf_sem_post_lxrt(unsigned int fifo)
{
	struct { unsigned int fifo; } arg = { fifo }; 
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _SEM_POST, &arg).i[LOW];
}

DECLARE int rtf_sem_trywait_lxrt(unsigned int fifo)
{
	struct { unsigned int fifo; } arg = { fifo }; 
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _SEM_TRY, &arg).i[LOW];
}

DECLARE int rtf_sem_destroy_lxrt(unsigned int fifo)
{
	struct { unsigned int fifo; } arg = { fifo }; 
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _SEM_DESTRY, &arg).i[LOW];
}

#ifdef CONFIG_RTAI_RTF_NAMED
DECLARE int rtf_create_named_lxrt(const char *name)
{
	int len;
	char lname[len = strlen(name)];
	struct { char * name; } arg = { lname };
	strncpy(lname, name, len);
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _CREATE_NAMED, &arg).i[LOW];
}

DECLARE int rtf_getfifobyname_lxrt(const char *name)
{
	int len;
	char lname[len = strlen(name)];
	struct { char * name; } arg = { lname };
	strncpy(lname, name, len);
	return rtai_lxrt(FUN_FIFOS_LXRT_INDX, SIZARG, _GETBY_NAME, &arg).i[LOW];
}
#endif

#endif

#endif /* _RTAI_FIFOS_LXRT_H_ */
