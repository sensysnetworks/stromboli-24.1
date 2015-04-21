
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

$Id: names.c,v 1.1.1.1 2004/06/06 14:02:34 rpm Exp $ 
*/

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>

#include <asm/uaccess.h>
#include <asm/mmu_context.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <rtai_lxrt.h>

#include "registry.h"
#include "msg.h"
#include "tid.h"
#include "names.h"

void rt_stomp(void)
{
    *((int*) 0xc0000000) = 0;	// For testing address space protection!
}

void rt_boom(void)
{
    __asm__ ("int $1"); 	// Debug Exception for testing
}

// RTAI LXRT Name services.
pid_t rt_Name_attach(const char *argname)
{
	RT_TASK *me;
	pid_t my_pid;
	int len;
	char name[MAX_NAME_LENGTH], nullname[8];

	if(!name) return -EINVAL;

	if(rt_is_linux()) {
		// In linux context argname is in user space
    	strncpy_from_user(name, argname, MAX_NAME_LENGTH);
	}
	else strncpy(name, argname, MAX_NAME_LENGTH);

	len = strnlen(name, MAX_NAME_LENGTH);
	if( len > (MAX_NAME_LENGTH-1)) {
	    // Illegal alias name.
    	return -EINVAL;
    }
	name[MAX_NAME_LENGTH-1] = 0;

	me = rt_whoami();
	my_pid = rttask2pid(me);
	if( my_pid ) {
	//rt_printk("rt_Name_attach: %X already there\n", my_pid);
	return -EBUSY;
	}

	my_pid  = assign_pid(me);

	if(!my_pid) {
		return -EAGAIN; 
	}

	if(!len) {
		// User space task already registered a name with rt_task_init(name, .. 
		if(me->lnxtsk) {
			/* NOOP */
		}
		// or, a Kernel task is using a null name.
		else {
			pid2nam(my_pid, nullname);
			if(!rt_register(nam2num(nullname), me, IS_TASK, 0)) {
		        rt_vc_release(my_pid);
		        return -EBUSY;
        		}
			}
		return my_pid;
	}

	if(me->lnxtsk) {
		// User supplied the name again in rt_Name_attach
		// Why am I not simply calling assign_pid() in rt_task_init( name, ...)?
		num2nam(rt_get_name(me), nullname);
		if(strcmp(name, nullname)) {
			// It is an alias name.
			assign_alias_name(my_pid, name);
			}
		return my_pid;
		}

	// Kernel cases when len != 0. 
	if( len > 6 ) {
		assign_alias_name(my_pid, name);
		pid2nam(my_pid, nullname);
	}
	else strcpy( nullname, name);

	if(!rt_register(nam2num(nullname), me, IS_TASK, 0)) {
		// Kernel task.
		rt_vc_release(my_pid);
		return -EBUSY;
	}
	return my_pid;	
}

pid_t rt_Name_locate(const char *arghost, const char *argname)
{
	RT_TASK *task;
	pid_t pid;
	char host[MAX_NAME_LENGTH], name[MAX_NAME_LENGTH];

	if(!name || !host) return 0; 

	if(rt_is_linux()) {
    	// In linux context argname is in user space
	    strncpy_from_user(name, argname, MAX_NAME_LENGTH);
    	strncpy_from_user(host, arghost, MAX_NAME_LENGTH);
	} else {
		strncpy(name, argname, MAX_NAME_LENGTH);
	    strncpy(host, arghost, MAX_NAME_LENGTH);
	}

	if ( host[0] ) return 0; // Network stuff soon.

	if(strlen(name) <= 6) {
		task = rt_get_adr(nam2num(name)); // Native names.
		if(task) return rttask2pid(task);
	}

	pid = alias2pid(name); // Alias name
	return pid;
}

int rt_Name_detach(pid_t pid)
{
	RT_TASK *me, *owner;

	me    = rt_whoami();
	owner = pid2rttask(pid);

	//rt_printk( "rt_Name_detach qnx pid %04X me %X owner %X\n", pid, me, owner);

	if( me == owner ) {
		if (!me->lnxtsk) rt_drg_on_adr(me); // Kernel task.
		rt_vc_release(pid);
		return 0 ;
		}
	else return -EINVAL;
}

