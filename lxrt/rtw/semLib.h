/*
COPYRIGHT (C) 2001  Paolo Mantegazza  (mantegazza@aero.polimi.it)
                    Giuseppe Quaranta (quaranta@aero.polimi.it)

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


#ifndef _VXW_SEMLIB_H_
#define _VXW_SEMLIB_H_

#include "vxWorks.h"

#define SEM_ID SEM *

#define NO_WAIT          0
#define WAIT_FOREVER    -1
#define SEM_EMPTY        0
#define SEM_Q_PRIORITY   PRIO_Q

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

extern unsigned int get_an_rtw_name(void);

static inline SEM_ID semBCreate(int options, int initialState) 
{
	SEM_ID sem;
	sem = rt_typed_sem_init(get_an_rtw_name(), initialState, BIN_SEM | options);
#ifdef DBGPRT
	printf("SEM CREATED %p\n", sem);
#endif
	return sem;
}

static inline int semDelete(SEM_ID semid)
{
#ifdef DBGPRT
	printf("SEM DELETED %p\n", semid);
#endif
	return rt_sem_delete(semid) > 0 ? OK : ERROR;
}

static inline int semTake(SEM_ID semid, int timeout)
{
	if (timeout == NO_WAIT) {
		return rt_sem_wait_if(semid) > 0 ? OK : ERROR;
	}
	return rt_sem_wait(semid) < 0xFFFF ? OK : ERROR;
}

static inline int semGive(SEM_ID semid)
{
	return rt_sem_signal(semid) < 0xFFFF ? OK : ERROR;
}

#endif
