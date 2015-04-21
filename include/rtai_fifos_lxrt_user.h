/*
COPYRIGHT (C) 2003  Peter Soetens <peter.soetens@mech.kuleuven.ac.be>

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


#ifndef _RTAI_FIFOS_LXRT_USER_H_
#define _RTAI_FIFOS_LXRT_USER_H_

#include <config.h>
#include <rtai_fifos.h>
#include <rtai_declare.h>

/**
 * This file was created with the same aim as rtai_lxrt_user.h .
 * It is intended to exclude some kernel not C++ friendly code while
 * maintaining backward compatibility with C programs.
 *
 * Especially the replacement of the include rtai_lxrt.h with rtai_lxrt_user.h
 * helped avoiding double definitions when multiple object files need the
 * fifo functions.
 */

#include <rtai_lxrt_user.h>
				
extern int rtf_create(unsigned int fifo, int size);

extern int rtf_destroy(unsigned int fifo);

extern int rtf_put(unsigned int fifo, const void *buf, int count);

extern int rtf_get(unsigned int fifo, void *buf, int count);

extern int rtf_reset_lxrt(unsigned int fifo);

extern int rtf_resize_lxrt(unsigned int fifo, int size);

extern int rtf_sem_init_lxrt(unsigned int fifo, int value);

extern int rtf_sem_post_lxrt(unsigned int fifo);

extern int rtf_sem_trywait_lxrt(unsigned int fifo);

extern int rtf_sem_destroy_lxrt(unsigned int fifo);

#ifdef CONFIG_RTAI_RTF_NAMED
extern int rtf_create_named_lxrt(const char *name);

extern int rtf_getfifobyname_lxrt(const char *name);
#endif

#endif /* _RTAI_FIFOS_LXRT_USER_H_ */
