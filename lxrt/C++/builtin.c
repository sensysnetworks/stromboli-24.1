/*
 * Project: rtai_cpp - RTAI C++ Framework 
 *
 * File: $Id: builtin.c,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $
 *
 * Copyright: (C) 2001 Erwin Rol <erwin@muffin.org>
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

#include <rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>

void
__builtin_delete(void* vp){
	rt_printk("__builtin_delete %p\n",vp);
	if(vp != 0)
		rt_free(vp);
}

void*
__builtin_new(int size){
	void* vp = rt_malloc(size);
	rt_printk("__builtin_new %p %d\n",vp,size);
	return vp;
}

void
__builtin_vec_delete(void* vp){
	rt_printk("__builtin_vec_delete %p\n",vp);
	if(vp != 0)
		rt_free(vp);
}

void*
__builtin_vec_new(int size){
	void* vp = rt_malloc(size);
	rt_printk("__builtin_vec_new %p %d\n",vp,size);
	return vp;
}

extern void __default_terminate (void) __attribute__ ((__noreturn__));

void
__default_terminate(void)
{
	while(1)
		rt_task_suspend(rt_whoami());
}
  
void (*__terminate_func)(void) = __default_terminate;

void
__terminate(void)
{
	(*__terminate_func)();
}
    
void
__pure_virtual(void)
{
	rt_printk("pure virtual method called\n");
	__terminate ();
}
