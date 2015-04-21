
/***************************************************************************
                          lxk.c  -  description
                          ---------------------

The lxk_ prefix family of functions have the same functionality as their rt_
counterpart but are meant to be called from the Linux context while in kernel
mode. Again, before using any of these functions, a kernel thread must first
create and initialise a RTAI real time agent with function lxk_task_init(). 

    begin                : Sat Mar 3 2001
    copyright            : (C) 2001 by Erwin Rol
    email                : erwin@muffin.org

 $Id: lxk.c,v 1.1.1.1 2004/06/06 14:02:32 rpm Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

union rtai_lxrt_t lxk_resume(void *fun, int narg, int *arg, RT_TASK *rt_task)
{
	return (union rtai_lxrt_t) lxrt_resume(fun, narg, arg, 0, rt_task);
}

int lxk_task_delete(RT_TASK* rt_task)
{
	return __task_delete(rt_task);
}

RT_TASK* lxk_task_init(int name, int priority, int stack_size, int max_msg_size)
{
     return __task_init(name, priority, stack_size, max_msg_size, 0xFFFFFFFF);
}

//EXPORT_SYMBOL(lxk_resume);
//EXPORT_SYMBOL(lxk_task_init);
//EXPORT_SYMBOL(lxk_task_delete);
