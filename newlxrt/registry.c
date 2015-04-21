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

$Id: registry.c,v 1.1.1.1 2004/06/06 14:02:48 rpm Exp $ 
*/


#include <linux/module.h>
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>

#include <asm/rtai.h>
#include "registry.h"

static struct rt_registry_entry_struct lxrt_list[MAX_SLOTS + 1] = {{0, 0, 0, 0}, };
static spinlock_t list_lock = SPIN_LOCK_UNLOCKED;

static inline int registr(unsigned long name, void *adr, int type, struct task_struct *tsk)
{
        unsigned long flags;
        int slot;
/*
 * Register a resource. This allows other programs (RTAI and/or user space)
 * to use the same resource because they can find the address from the name.
*/
        flags = rt_spin_lock_irqsave(&list_lock);
        // index 0 is reserved for the null slot.
        for (slot = 1; slot <= MAX_SLOTS; slot++) {
                if (!lxrt_list[slot].name) {
                        lxrt_list[slot].name = name;
                        lxrt_list[slot].adr  = adr;
                        lxrt_list[slot].tsk  = tsk;
                        lxrt_list[slot].pid  = tsk ? tsk->pid : 0 ;
                        lxrt_list[slot].type = type;
                        rt_spin_unlock_irqrestore(flags, &list_lock);
                        return slot;
                }
        }
        rt_spin_unlock_irqrestore(flags, &list_lock);
        return 0;
}

static inline int drg_on_name(unsigned long name)
{
	unsigned long flags;
	int slot;
	flags = rt_spin_lock_irqsave(&list_lock);
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (lxrt_list[slot].name == name) {
			lxrt_list[slot] = lxrt_list[0];
			rt_spin_unlock_irqrestore(flags, &list_lock);
			return slot;
		}
	}
	rt_spin_unlock_irqrestore(flags, &list_lock);
	return 0;
} 

static inline int drg_on_adr(void *adr)
{
	unsigned long flags;
	int slot;
	flags = rt_spin_lock_irqsave(&list_lock);
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (lxrt_list[slot].adr == adr) {
			lxrt_list[slot] = lxrt_list[0];
			rt_spin_unlock_irqrestore(flags, &list_lock);
			return slot;
		}
	}
	rt_spin_unlock_irqrestore(flags, &list_lock);
	return 0;
} 

static inline unsigned long get_name(void *adr)
{
	static unsigned long nameseed = 0xfacade;
	unsigned long flags;
	int slot;
        if (!adr) {
		unsigned long name;
		flags = rt_spin_lock_irqsave(&list_lock);
		name = nameseed++;
		rt_spin_unlock_irqrestore(flags, &list_lock);
		return name;
        }
	flags = rt_spin_lock_irqsave(&list_lock);
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (lxrt_list[slot].adr == adr) {
		rt_spin_unlock_irqrestore(flags, &list_lock);
		return lxrt_list[slot].name;
		}
	}
	rt_spin_unlock_irqrestore(flags, &list_lock);
	return 0;
} 

static inline void *get_adr(unsigned long name)
{
	unsigned long flags;
	int slot;
	flags = rt_spin_lock_irqsave(&list_lock);
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (lxrt_list[slot].name == name) {
			rt_spin_unlock_irqrestore(flags, &list_lock);
			return lxrt_list[slot].adr;
		}
	}
	rt_spin_unlock_irqrestore(flags, &list_lock);
	return 0;
} 

static inline int get_type(unsigned long name)
{
        unsigned long flags;
        int slot;
        flags = rt_spin_lock_irqsave(&list_lock);
        for (slot = 1; slot <= MAX_SLOTS; slot++) {
                if (lxrt_list[slot].name == name) {
                        rt_spin_unlock_irqrestore(flags, &list_lock);
                        return lxrt_list[slot].type;
                }
        }
        rt_spin_unlock_irqrestore(flags, &list_lock);
        return -EINVAL;
}

unsigned long is_process_registered(struct task_struct *tsk)
{
	unsigned long flags;
	int slot;
	flags = rt_spin_lock_irqsave(&list_lock);
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (lxrt_list[slot].tsk == tsk) {
			if (lxrt_list[slot].pid == (tsk ? tsk->pid : 0)) {
				rt_spin_unlock_irqrestore(flags, &list_lock);
				return lxrt_list[slot].name;
			}
                }
        }
        rt_spin_unlock_irqrestore(flags, &list_lock);
        return 0;
}

int rt_register(unsigned long name, void *adr, int type, struct task_struct *t)
{
/*
 * Register a resource. This function provides the service to all RTAI tasks.
*/
	return get_adr(name) ? 0 : registr(name, adr, type, t );
}


int rt_drg_on_name(unsigned long name)
{
	return drg_on_name(name);
} 

int rt_drg_on_adr(void *adr)
{
	return drg_on_adr(adr);
} 

unsigned long rt_get_name(void *adr)
{
	return get_name(adr);
} 

void *rt_get_adr(unsigned long name)
{
	return get_adr(name);
}

int rt_get_type(unsigned long name)
{
	return get_type(name);
}

#ifdef CONFIG_PROC_FS
int rt_get_registry_slot(int slot, struct rt_registry_entry_struct* entry)
{
	unsigned long flags;

	// check if we got a valid pointer
	if(entry == 0)
		return 0;


	flags = rt_spin_lock_irqsave(&list_lock);
	// index 0 is reserved for the null slot.
	if (slot > 0 && slot <= MAX_SLOTS ) {
		if (lxrt_list[slot].name != 0) {
            // clear the result
			memset((char*)entry,0,sizeof(*entry));

			// copy the structure
			entry->name = lxrt_list[slot].name;
			entry->adr  = lxrt_list[slot].adr;
			entry->tsk  = lxrt_list[slot].tsk;
			entry->pid  = lxrt_list[slot].pid;
			entry->type = lxrt_list[slot].type;

			rt_spin_unlock_irqrestore(flags, &list_lock);
			return slot;
		}
	}
	rt_spin_unlock_irqrestore(flags, &list_lock);

	return 0;
}

#endif
