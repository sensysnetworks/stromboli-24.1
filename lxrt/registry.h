/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it),
extensions for user space modules are jointly copyrighted (2000) with:
			Pierre Cloutier (pcloutier@poseidoncontrols.com),
			Steve Papacharalambous (stevep@zentropix.com).

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

$Id: registry.h,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $ 
*/

#ifndef _REGISTRY_H_
#define _REGISTRY_H_

struct rt_registry_entry_struct {
        unsigned long name;      // Numerical representation of resource name
        void *adr;               // Physical rt memory address of resource
        struct task_struct *tsk; // Linux task owner of the resource
        int pid;                 // Linux task pid
        int type;                // Type of resource
};

#define MAX_SLOTS  100          // Max number of registered objects
#define IS_TASK 0               // Used to identify registered resources
#define IS_SEM  1
#define IS_MBX  2
#define IS_PRX  3

extern unsigned long is_process_registered(struct task_struct *tsk);
extern int rt_register(unsigned long nam, void *adr, int typ, struct task_struct *tsk);
extern int rt_drg_on_name(unsigned long name);
extern int rt_drg_on_adr(void *adr);
extern unsigned long rt_get_name(void *adr);
extern void *rt_get_adr(unsigned long name);
extern int rt_get_type(unsigned long name);

#ifdef CONFIG_PROC_FS
extern int rt_get_registry_slot(int slot , struct rt_registry_entry_struct* entry);
#endif

#endif
