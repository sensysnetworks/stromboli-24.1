
/*
COPYRIGHT (C) 2000  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)

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

$Id: names.h,v 1.1.1.1 2004/06/06 14:02:34 rpm Exp $
*/

#ifndef _NAMES_H_
#define _NAMES_H_

// RTAI LXRT Name services.

#define	MAX_NAME_LENGTH		32

pid_t rt_Name_attach(const char *name);
pid_t rt_Name_locate(const char *host, const char *name);
int rt_Name_detach(pid_t pid);
void rt_boom(void);
void rt_stomp(void);

#endif // _NAMES_H_

