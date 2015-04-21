/***************************************************************************
                          rtai_lxk.h  -  description
                             -------------------
    begin                : Sat Mar 3 2001
    copyright            : (C) 2001 by Erwin Rol
    email                : erwin@muffin.org
 
    $Id: rtai_lxk.h,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $
 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 * This library is free software; you can redistribute it and/or           *
 * modify it under the terms of the GNU Lesser General Public              *
 * License as published by the Free Software Foundation; either            *
 * version 2 of the License, or (at your option) any later version.        *
 *                                                                         *
 * This library is distributed in the hope that it will be useful,         *  
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * Lesser General Public License for more details.                         *
 *                                                                         *
 * You should have received a copy of the GNU Lesser General Public        *
 * License along with this library; if not, write to the Free Software     *
 * Foundation, Inc.,                                                       *
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.                  *
 *                                                                         *
 ***************************************************************************/

#ifndef __RTAI_LXK_H__
#define __RTAI_LXK_H__

#include <rtai.h>
#include <rtai_sched.h>
#include <asm/rtai_lxrt.h>


extern union rtai_lxrt_t lxk_resume(void *fun, int narg, int *arg, RT_TASK *rt_task);
extern RT_TASK* lxk_task_init(int name, int priority, int stack_size, int max_msg_size);
extern int lxk_task_delete(RT_TASK* rt_task);

static inline int lxk_mbx_receive(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { mbx, msg, msg_size };
	return lxk_resume(rt_mbx_receive, sizeof(arg)/sizeof(int), (int *)&arg, current->this_rt_task[0]).i[LOW];
}

static inline int lxk_mbx_send(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { mbx, msg, msg_size };
	return lxk_resume(rt_mbx_send, sizeof(arg)/sizeof(int),(int *)&arg, current->this_rt_task[0]).i[LOW];
}

static inline int lxk_sem_wait(SEM *sem)
{
	struct { SEM *sem; } arg = { sem };
	return lxk_resume(rt_sem_wait, sizeof(arg)/sizeof(int),(int *)&arg, current->this_rt_task[0]).i[LOW];
}

static inline int lxk_sem_wait_timed(SEM *sem, RTIME delay)
{
	struct { SEM *sem; RTIME delay;} arg = { sem , delay};
	return lxk_resume(rt_sem_wait_timed, sizeof(arg)/sizeof(int),(int *)&arg, current->this_rt_task[0]).i[LOW];
}

static inline int lxk_sem_signal(SEM *sem)
{
	struct { SEM *sem; } arg = { sem };
	return lxk_resume(rt_sem_signal, sizeof(arg)/sizeof(int),(int *)&arg, current->this_rt_task[0]).i[LOW];
}

#define lxk_cond_signal(cnd)     lxk_sem_signal((cnd))

#define lxk_cond_broadcast(cnd)  lxk_sem_broadcast((cnd))

static inline int lxk_cond_wait(CND *cnd, SEM *mtx)
{
	struct { CND *cnd; SEM *mtx; } arg = { cnd, mtx };
	return lxk_resume(rt_cond_wait, sizeof(arg)/sizeof(int),(int *)&arg, current->this_rt_task[0]).i[LOW];
}

static inline int lxk_cond_wait_until(CND *cnd, SEM *mtx, RTIME time)
{
	struct { CND *cnd; SEM *mtx; RTIME time; } arg = { cnd, mtx, time };
	return lxk_resume(rt_cond_wait_until, sizeof(arg)/sizeof(int),(int *)&arg, current->this_rt_task[0]).i[LOW];
}

static inline int lxk_cond_wait_timed(CND *cnd, SEM *mtx, RTIME delay)
{
	struct { CND *cnd; SEM *mtx; RTIME delay; } arg = { cnd, mtx, delay };
	return lxk_resume(rt_cond_wait_timed, sizeof(arg)/sizeof(int),(int *)&arg, current->this_rt_task[0]).i[LOW];
}

static inline int lxk_sleep(RTIME delay)
{
	struct { RTIME delay; } arg = { delay };
	return lxk_resume(rt_sleep, sizeof(arg)/sizeof(int),(int *)&arg, current->this_rt_task[0]).i[LOW];
}

#endif /* !__RTAI_LXK_H__ */
