/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: rtai_syscall.h,v 1.1.1.1 2004/06/06 14:02:37 rpm Exp $
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


#ifndef __RTAI_SYSCALL_H__
#define __RTAI_SYSCALL_H__

extern int rt_open(const char *pathname, int flags, mode_t mode);

extern ssize_t rt_pwrite(int fd, const void *buf, size_t count,off_t offset);

extern ssize_t rt_pread(int fd, void *buf, size_t count,off_t offset);

extern int rt_close(int fd);

#endif /* !__RTAI_SYSCALL_H__ */

