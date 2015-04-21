/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#define RTAI_SHM_MISC_MINOR  254 // The same minor used to mknod for major 10.
#define MAX_SLOTS            32  // Set it according to your needs.
#define MAX_OWNERS           4*MAX_SLOTS // avrg 4 simultaneous users per slot!

#ifdef DEBUG
static int closable;
static int echo;
#define DPRINTK rtai_print_to_screen
#else
#define DPRINTK printk
#endif

#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/wrapper.h>

#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/io.h>

#include <rtai.h>
#include <rtai_shm.h>
#include <rtai_trace.h>

#if LINUX_EXT_VERSION_CODE < KERNEL_EXT_VERSION(2,4,0,4)
#include "rtai_bttv.h"
#else
#include "kvmem.h"
#include "kvmem.c"
#endif

MODULE_LICENSE("GPL");

#ifdef __SHM_USE_VECTOR
DEFINE_SHM_HANDLER
#endif

/* The two lines below could go into a structure. It is preferred
   to keep them distinct to recall that a vmarea pertains only to 
   processes allocated/mmaped areas, and not to module ones. */

static unsigned long long name_pid_list[MAX_OWNERS + 1] = { 0LL, };
static struct vm_area_struct *vmarea[MAX_OWNERS + 1]   = { 0, };

static struct { void *adr; unsigned long name, size, count; } shm_list[MAX_SLOTS + 1] = { {0, 0, 0, 0}, };

static spinlock_t shm_lock = SPIN_LOCK_UNLOCKED;

#define NAME(x) ((unsigned long *)(&(x)))[0]
#define PID(x)  ((unsigned long *)(&(x)))[1]

static inline int registr(unsigned long long name_pid)
{
	int owner;
	for (owner = 1; owner <= MAX_OWNERS; owner++) {
		if (!name_pid_list[owner]) {
			name_pid_list[owner] = name_pid;
			vmarea[owner] = (struct vm_area_struct *)(PID(name_pid) | 0xF0000000);
			return owner;
		}
	}
	return 0;
} 

static inline int deregistr(unsigned long long name_pid)
{
	int owner;
	for (owner = 1; owner <= MAX_OWNERS; owner++) {
		if (name_pid_list[owner] == name_pid) {
			name_pid_list[owner] = 0LL;
			vmarea[owner] = 0;
			return owner;
		}
	}
	return 0;
} 

static inline int there_is_not_and_reg(unsigned long long name_pid)
{
	int owner;
	for (owner = 1; owner <= MAX_OWNERS; owner++) {
		if (name_pid_list[owner] == name_pid) {
			return 0;
		}
	}
	return registr(name_pid);
} 

static inline int find_name(unsigned long name)
{
	int slot;
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (shm_list[slot].name == name) {
			return slot;
		}
	}
	return 0;
} 

static inline int find_name_and_reg(unsigned long name, unsigned long pid)
{
	int slot;
	unsigned long long name_pid;
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (shm_list[slot].name == name) {
			NAME(name_pid) = name;
			PID(name_pid)  = pid;
			return there_is_not_and_reg(name_pid) ? slot : 0;
		}
	}
	return 0;
} 

static inline int find_free_slot_and_reg(unsigned long name, unsigned long pid)
{
	int slot;
	unsigned long long name_pid;
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (!shm_list[slot].name) {
			NAME(name_pid) = name;
			PID(name_pid)  = pid;
			return there_is_not_and_reg(name_pid) ? slot : 0;
		}
	}
	return 0;
} 

static inline int find_name_and_drg(unsigned long name, unsigned long pid)
{
	int slot;
	unsigned long long name_pid;
	for (slot = 1; slot <= MAX_SLOTS; slot++) {
		if (shm_list[slot].name == name) {
			NAME(name_pid) = name;
			PID(name_pid)  = pid;
			return deregistr(name_pid) ? slot : 0;
		}
	}
	return 0;
} 

void *rtai_kmalloc_f(int name, int size, unsigned long pid)
{
	unsigned long flags;
	void *adr;
	int slot;
	if (size <= 0) {
		return 0;
	}

	TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_KMALLOC, name, size, pid);

	spin_lock_irqsave(&shm_lock, flags);
	if ((slot = find_name_and_reg(name, pid))) {
		shm_list[slot].count++;
		MOD_INC_USE_COUNT;
		adr = shm_list[slot].adr;
		spin_unlock_irqrestore(&shm_lock, flags);
		return adr;
	}
// Commented spin locks below fix a race window under SMP.
//	spin_unlock_irqrestore(&shm_lock, flags);
	size = REAL_SIZE(size);
	if ((adr = rvmalloc(size))) {
//		spin_lock_irqsave(&shm_lock, flags);
		if ((slot = find_free_slot_and_reg(name, pid))) {
			memset(adr, 0, size);
			shm_list[slot].name  = name;
			shm_list[slot].adr   = adr;
			shm_list[slot].size  = size;
			shm_list[slot].count = 1;
			MOD_INC_USE_COUNT;
			spin_unlock_irqrestore(&shm_lock, flags);
			return adr;
		}
		spin_unlock_irqrestore(&shm_lock, flags);
		rvfree(adr, size);
	}
	return 0;
}

void rtai_kfree_f(int name, unsigned long pid)
{
	unsigned long flags;
	int slot, size;
	void *adr;

	TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_KFREE, name, 0, pid);

	spin_lock_irqsave(&shm_lock, flags);
	if ((slot = find_name_and_drg(name, pid))) {
		MOD_DEC_USE_COUNT;
		if (!(--shm_list[slot].count)) {
			adr  = shm_list[slot].adr;
			size = shm_list[slot].size;
			shm_list[slot] = shm_list[0];
			spin_unlock_irqrestore(&shm_lock, flags);
			rvfree(adr, size);
			return;
		}
	}
	spin_unlock_irqrestore(&shm_lock, flags);
	return;
}

long long shm_handler(unsigned int srq, unsigned long name);

static int rtai_shm_f_ioctl(struct inode *inode, struct file *file,
	unsigned int cmd, unsigned long arg)
{
	return shm_handler(cmd,arg);
}

long long shm_handler(unsigned int srq, unsigned long name)
{
	static void rtai_shm_vm_close(struct vm_area_struct *vma);
	int owner, slot;
	long long name_pid;
	switch (srq) {
		case 1:
		        TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_MALLOC, name, srq, current->pid);
			if (rtai_kmalloc_f(((unsigned long *)name)[0], ((unsigned long *)name)[1], current->pid)) {
				return shm_list[find_name(((unsigned long *)name)[0])].size;
			}
			return 0;
		case 2:
			NAME(name_pid) = name;
			PID(name_pid)  = current->pid;
		        TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_GET_SIZE, name, srq, current->pid);
			for (owner = 1; owner <= MAX_OWNERS; owner++) {
				if (name_pid_list[owner] == name_pid) {
					if (vmarea[owner] && (vmarea[owner]->vm_ops)->close == rtai_shm_vm_close) {
						vmarea[owner]->vm_ops = 0;
						if ((slot = find_name(name))) {
							return shm_list[slot].size;
						}
					}
					DPRINTK("VMAREA NOT FOUND FOR AN EXISTING NAME_PID AT UNMAP REQUEST.\n");
					return 0;
				}
			}
			return 0;
		case 3:
		        TRACE_RTAI_SHM(TRACE_RTAI_EV_SHM_FREE, name, srq, current->pid);
			rtai_kfree_f(name, current->pid);
			return 0;

#ifdef DEBUG
// all what's below, in this function, is just for testing

		case 4:
			if ((slot = find_name(name))) {
			return shm_list[slot].count > 1 ? shm_list[slot].size : - shm_list[slot].size;
			}
			return 0;
		case 5:
			if ((slot = find_name(name))) {
				int *adr;
				adr = shm_list[slot].adr;
				DPRINTK("ECHO FOR %lx FROM SHM MODULE %d\n", name, adr[0]);
				echo += 2;
				memset(shm_list[slot].adr, echo, shm_list[slot].size);
				DPRINTK("SHM MODULE WILL CHANGE %lx TO %d\n", name, adr[0]);

				slot = find_name(0xaaaa);
				adr = shm_list[slot].adr;
				DPRINTK("ECHO FOR %x FROM SHM MODULE %d\n", 0xaaaa, adr[0]);
				echo += 2;
				memset(shm_list[slot].adr, echo, shm_list[slot].size);
				DPRINTK("SHM MODULE WILL CHANGE %x TO %d\n", 0xaaaa, adr[0]);
			}
			return 0;
		case 6:
			return closable;
		case 7:
			return (closable = 1);
		case 8:
			return (closable = 0);
#endif
	}
	return 0;
}

static void rtai_shm_vm_close(struct vm_area_struct *vma)
{
	int owner, slot;
	for (owner = 1; owner <= MAX_OWNERS; owner++) {
		if (vmarea[owner] == vma && PID(name_pid_list[owner]) == current->pid) {
			if((slot = find_name(NAME(name_pid_list[owner])))) {
				if (shm_list[slot].count == 1) {
					DPRINTK("DELETING AREA %lx AT KERNEL REQUEST.\n", shm_list[slot].name);
				} else {
					DPRINTK("UNMAPPING AREA %lx AT KERNEL REQUEST.\n", shm_list[slot].name);
				}
				rtai_kfree_f(shm_list[slot].name, current->pid);
				return;
			}
			DPRINTK("NO AREA NAME FOUND FOR AN EXISTING PID_VMA AT KERNEL UNMAP REQUEST.\n");
			return;
		}
	}
	DPRINTK("NO PID_VMA FOUND AT KERNEL UNMAP REQUEST.\n");
	return;
}

struct vm_operations_struct rtai_shm_vm_ops = { close: rtai_shm_vm_close } ;

static int rtai_shm_mmap(struct file *file, struct vm_area_struct *vma)
{
	int owner, slot;
	for (owner = 1; owner <= MAX_OWNERS; owner++) {
		if (PID(name_pid_list[owner]) == current->pid && vmarea[owner] == ((struct vm_area_struct *)(current->pid | 0xF0000000))) {
			if((slot = find_name(NAME(name_pid_list[owner])))) {
				vmarea[owner] = vma;
				if(!vma->vm_ops) {
					vma->vm_ops = &rtai_shm_vm_ops;
				}
				return rvmmap(shm_list[slot].adr, shm_list[slot].size, vma); 
			}
			DPRINTK("NO AREA NAME FOUND FOR A MAPPABLE PID AT MMAPP REQUEST.\n");
			return -EFAULT;
		}
	}
	DPRINTK("NO MAPPABLE PID FOUND AT MMAPP REQUEST.\n");
	return -EFAULT;
}

static int rtai_shm_f_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int rtai_shm_f_close(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations rtai_shm_fops = {
#if LINUX_EXT_VERSION_CODE >= KERNEL_EXT_VERSION(2,4,0,4)
	owner:		THIS_MODULE,
#endif
	mmap:		rtai_shm_mmap,
	open:		rtai_shm_f_open,
	release:	rtai_shm_f_close,
	ioctl:		rtai_shm_f_ioctl,
};

static struct miscdevice rtai_shm_dev = 
	{ RTAI_SHM_MISC_MINOR, "RTAI_SHM", &rtai_shm_fops, NULL, NULL };

#ifdef __SHM_USE_VECTOR
static struct desc_struct sidt;
#endif

int init_module (void)
{
	if (misc_register(&rtai_shm_dev) < 0) {
		DPRINTK("***** COULD NOT REGISTER SHARED MEMORY DEVICE *****\n");
		return -EBUSY;
	}
#ifdef __SHM_USE_VECTOR
	rt_mount_rtai();
	sidt = rt_set_full_intr_vect(RTAI_SHM_VECTOR, 15, 3, (void *)RTAI_SHM_HANDLER);
#endif
	return 0 ;
}

void cleanup_module (void)
{
	int i;
	DPRINTK("\n");
	for (i = 1; i <= MAX_SLOTS; i++) {
		if (shm_list[i].name) {
			DPRINTK("***** FREEING AREA %lx AT MODULE CLEANUP *****\n", shm_list[i].name);
			rvfree(shm_list[i].adr, shm_list[i].size);
		}
	}
	DPRINTK("\n");
#ifdef __SHM_USE_VECTOR
	rt_reset_full_intr_vect(RTAI_SHM_VECTOR, sidt);
	rt_umount_rtai();
#endif
	misc_deregister(&rtai_shm_dev);
	return;
}
