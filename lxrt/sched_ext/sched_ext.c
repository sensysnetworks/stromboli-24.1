/*
COPYRIGHT (C) 2001  Pierre Cloutier  (pcloutier@poseidoncontrols.com)
		    and
		    Paolo Mantegazza (mantegazza@aero.polimi.it).

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


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/version.h>

#include <rtai_sched.h>
#include <rtai_nam2num.h>

#include "../registry.h"
#include "../msg.h"
#include "../proxies.h"

MODULE_LICENSE("GPL");

RT_TASK *rt_named_task_init(const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void))
{
	RT_TASK *task;
	unsigned long name;

	if ((task = rt_get_adr(name = nam2num(task_name)))) {
		return task;
	}
        if ((task = rt_malloc(sizeof(RT_TASK))) && !rt_task_init(task, thread, data, stack_size, prio, uses_fpu, signal)) {
		if (rt_register(name, task, IS_TASK, 0)) {
			return task;
		}
		rt_task_delete(task);
	}
	rt_free(task);
	return (RT_TASK *)0;
}

RT_TASK *rt_named_task_init_cpuid(const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void), unsigned int run_on_cpu)
{
	RT_TASK *task;
	unsigned long name;

	if ((task = rt_get_adr(name = nam2num(task_name)))) {
		return task;
	}
        if ((task = rt_malloc(sizeof(RT_TASK))) && !rt_task_init_cpuid(task, thread, data, stack_size, prio, uses_fpu, signal, run_on_cpu)) {
		if (rt_register(name, task, IS_TASK, 0)) {
			return task;
		}
		rt_task_delete(task);
	}
	rt_free(task);
	return (RT_TASK *)0;
}

SEM *rt_typed_named_sem_init(const char *sem_name, int value, int type)
{
	SEM *sem;
	unsigned long name;

	if ((sem = rt_get_adr(name = nam2num(sem_name)))) {
		return sem;
	}
	if ((sem = rt_malloc(sizeof(SEM)))) {
		rt_typed_sem_init(sem, value, type);
		if (rt_register(name, sem, IS_SEM, 0)) {
			return sem;
		}
		rt_sem_delete(sem);
	}
	rt_free(sem);
	return (SEM *)0;
}

MBX *rt_typed_named_mbx_init(const char *mbx_name, int size, int qtype)
{
	MBX *mbx;
	unsigned long name;

	if ((mbx = rt_get_adr(name = nam2num(mbx_name)))) {
		return mbx;
	}
	if ((mbx = rt_malloc(sizeof(MBX))) && !rt_typed_mbx_init(mbx, size, qtype)) {
		if (rt_register(name, mbx, IS_MBX, 0)) {
			return mbx;
		}
		rt_mbx_delete(mbx);
	}
	rt_free(mbx);
	return (MBX *)0;
}

int rt_named_task_delete(RT_TASK *task)
{
	if (!rt_task_delete(task)) {
		rt_free(task);
	}
	return rt_drg_on_adr(task);
}

int rt_named_sem_delete(SEM *sem)
{
	if (!rt_sem_delete(sem)) {
		rt_free(sem);
	}
	return rt_drg_on_adr(sem);
}

int rt_named_mbx_delete(MBX *mbx)
{
	if (!rt_mbx_delete(mbx)) {
		rt_free(mbx);
	}
	return rt_drg_on_adr(mbx);
}

#ifdef CONFIG_RTAI_BITS

#include <rtai_bits.h>

BITS *rt_named_bits_init(const char *bits_name, unsigned long mask)
{
	BITS *bits;
	unsigned long name;

	if ((bits = rt_get_adr(name = nam2num(bits_name)))) {
		return bits;
	}
	if ((bits = rt_malloc(sizeof(BITS)))) {
		rt_bits_init(bits, mask);
		if (rt_register(name, bits, IS_SEM, 0)) {
			return bits;
		}
		rt_bits_delete(bits);
	}
	rt_free(bits);
	return 0;
}

int rt_named_bits_delete(BITS *bits)
{
	if (!rt_bits_delete(bits)) {
		rt_free(bits);
	}
	return rt_drg_on_adr(bits);
}

#endif

/* ----------------------< proc filesystem section >----------------------*/

#ifdef CONFIG_PROC_FS
#include <linux/stat.h>
#include <linux/proc_fs.h>
#include <rtai_proc_fs.h>

extern struct proc_dir_entry *rtai_proc_root;

static int  rtai_proc_sched_ext_register(void);
static void rtai_proc_sched_ext_unregister(void);

static int rtai_read_sched_ext(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	PROC_PRINT_VARS;
	struct rt_registry_entry_struct entry;
	char* type_name[4] = { "TASK","SEM","MBX" };
	int i;
	char name[8];

	PROC_PRINT("\nRTAI SCHED_EXT Information.\n");
	PROC_PRINT("MAX_SLOTS = %d\n\n",MAX_SLOTS);

	PROC_PRINT("Slot Name    Type     RT Handle\n");
	PROC_PRINT("-------------------------------\n");
	for (i = 1; i <= MAX_SLOTS; i++) {
		if (rt_get_registry_slot(i, &entry)) {
			num2nam(entry.name, name);
			PROC_PRINT("%4d %-6.6s %-8.8s 0x%p\n",
			i,    			// the slot number
			name,       		// the name in 6 char asci
			entry.type > 3 ? "UNKNOWN" : 
			type_name[entry.type],	// the Type
			entry.adr);		// The RT Handle
		 }
	}
	PROC_PRINT("\n");
        PROC_PRINT_DONE;
}

static int rtai_proc_sched_ext_register(void)
{
	struct proc_dir_entry *proc_lxrt_ent;

	proc_lxrt_ent = create_proc_entry("SCHED_EXT", S_IFREG | S_IRUGO | S_IWUSR, rtai_proc_root);
	if (!proc_lxrt_ent) {
		printk("Unable to initialize /proc/rtai/lxrt\n");
		return -1;
	}
	proc_lxrt_ent->read_proc = rtai_read_sched_ext;
	return 0;
}


static void rtai_proc_sched_ext_unregister(void)
{
	remove_proc_entry("SCHED_EXT", rtai_proc_root);
	return;
}

#endif

/* ------------------< end of proc filesystem section >------------------*/

int init_module(void)
{
#ifdef CONFIG_PROC_FS
	rtai_proc_sched_ext_register();
#endif
	return 0;
}

static void krtai_objects_release(void)
{
	int slot;
        struct rt_registry_entry_struct entry;
	char name[8], *type;

	for (slot = 1; slot <= MAX_SLOTS; slot++) {
                if (rt_get_registry_slot(slot, &entry) && entry.adr) {
			switch (entry.type) {
	                       	case IS_TASK:
					type = "TASK";
					rt_named_task_delete(entry.adr);
					break;
				case IS_SEM:
					type = "SEM ";
					rt_named_sem_delete(entry.adr);
					break;
				case IS_MBX:
					type = "MBX ";
                               		rt_named_mbx_delete(entry.adr);
	                       		break;	
				case IS_PRX:
					type = "PRX ";
        	       	               	rt_Proxy_detach(rttask2pid(entry.adr));
					rt_drg_on_adr(entry.adr); 
					break;
	                       	default:
					type = "XXXX";
					break;
			}
			num2nam(entry.name, name);
			rt_printk("SCHED_EXT releases %s %s\n", type, name);
		}
	}
}

void cleanup_module(void)
{
	krtai_objects_release();
#ifdef CONFIG_PROC_FS
	rtai_proc_sched_ext_unregister();
#endif
	return;
}
