/*
COPYRIGHT (C) 2000  Pierre Cloutier (pcloutier@PoseidonControls.com)
              2002  Paolo Mantegazza(mante@aero.polimi.it) (modified)

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

$Id: infnewlxrt.c,v 1.1.1.1 2004/06/06 14:02:47 rpm Exp $ 
*/

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>

#include "msgnewlxrt.h"
#include "registry.h"

extern int clr_rtext(RT_TASK *);
extern struct task_struct *end_buddy(RT_TASK *);

void linux_process_termination(void)
{
	unsigned long num;
	void *adr;
	int type;
	char name[8];
	RT_TASK *task2delete;
/*
 * Linux is just about to schedule current out of existence. With this feature, 
 * NEWLXRT frees the real time resources allocated to it.
*/
	while ((num = is_process_registered(current))) {
		rt_global_cli();
		adr = rt_get_adr(num);
		type = rt_get_type(num);
		rt_drg_on_adr(adr); 
		rt_global_sti();
		num2nam(num, name);
       		switch (type) {
			case IS_SEM:
				rt_printk("NEWLXRT releases SEM %s\n", name);
				rt_sem_delete(adr);
				rt_free(adr);
				break;
			case IS_MBX:
				rt_printk("NEWLXRT releases MBX %s\n", name);
				rt_mbx_delete(adr);
				rt_free(adr);
				break;
			case IS_PRX:
				num = rttask2pid(adr);
				rt_printk("NEWLXRT releases PROXY PID %lu\n", num);
				rt_Proxy_detach(num);
				break;
			case IS_TASK:
				rt_printk("NEWLXRT deregisters task %s\n", name);
				break;
		}
	}
	if ((task2delete = current->this_rt_task[0])) {
		end_buddy(task2delete);
		if (!clr_rtext(task2delete)) {
			rt_drg_on_adr(task2delete);
			rt_printk("NEWLXRT releases PID %d (ID: %s).\n", current->pid, current->comm);
			rt_free(task2delete->msg_buf[0]);
			rt_free(task2delete);
			current->this_rt_task[0] = current->this_rt_task[1] = 0;
		}
	}
}
