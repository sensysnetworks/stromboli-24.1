/*
COPYRIGHT (C) 2000  Pierre Cloutier (pcloutier@PoseidonControls.com)

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

$Id: informed.c,v 1.1.1.1 2004/06/06 14:02:32 rpm Exp $ 
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

#include "names.h"
#include "proxies.h"
#include "msg.h"
#include "tid.h"
#include "qblk.h"
#include "registry.h"

struct t_sigsysrq sigsysrq;

void linux_process_termination(void)
{
	RT_TASK *task2delete, *task2unblock, *base_linux_tasks[NR_RT_CPUS];
	int cpu, slot, nr_task_lists;
	pid_t my_pid;
	struct task_struct *ltsk;
	unsigned long num;
	void *adr;

/*
 * Linux is just about to schedule *ltsk out of existence.
 * With this feature, LXRT frees the real time resources allocated
 * by the task ltsk.
*/
	ltsk = current;
	rt_get_base_linux_task(base_linux_tasks);
	nr_task_lists = rt_sched_type() == MUP_SCHED ? NR_RT_CPUS : 1;
	rt_global_cli();
	for (cpu = 0; cpu < nr_task_lists; cpu++) {

		task2delete = base_linux_tasks[cpu];
		// Try to find if RTAI was aware of this dying Linux task.
        	while ((task2delete = task2delete->next) && task2delete->lnxtsk != ltsk);
		// First let's free the registered resources.
	        for (slot = 1; slot <= MAX_SLOTS; slot++) {
	                if ((num = is_process_registered(ltsk)) > 0) {
				adr = rt_get_adr(num);
       				switch (rt_get_type(num)) {
                       	        case IS_SEM:
					rt_printk("LXRT Informed releases SEM %p\n", adr);
					rt_sem_delete(adr);
					rt_free(adr);
					break;
                                case IS_MBX:
					rt_printk("LXRT Informed releases MBX %p\n", adr);
                                        rt_mbx_delete(adr);
                                        rt_free(adr);
                                        break;
                                case IS_PRX:
rt_printk("LXRT Informed releases PRX %p\n", adr);
                                        rt_Proxy_detach(rttask2pid(adr));
                                        break;
// to do:                       case IS_SHMEM:
				}
				rt_drg_on_adr(adr); 
                        }
                }

        // Synchronous IPC pid may need to be released
		if ((my_pid = rttask2pid(task2delete))) {
			rt_printk("Release vc %04X\n", my_pid);
			rt_vc_release(my_pid);
                }

	        if (!task2delete) {
			continue; // The user deleted the task but forgot to delete the resources.
		}

        // Other RTAI tasks may be SEND, RPC or RETURN blocked on task2delete.
Loop:
		task2unblock = base_linux_tasks[cpu];
		while ((task2unblock = task2unblock->next)) {
                	if (!(task2unblock->state & (SEND | RPC | RETURN))) {
				continue;
			} else if (task2unblock->msg_queue.task == task2delete) {
                        	task2unblock->state &= ~(SEND | RPC | RETURN | DELAYED);
	                        LXRT_RESUME(task2unblock);
				rt_global_cli();
				goto Loop;
                        }
                }

        // To do: other RTAI tasks may want to be informed as well.

	        // Ok, let's delete the task.
		if (!rt_task_delete(task2delete)) {
rt_printk("LXRT Informed releases RT %p, lnxpid %d (%p), name %s.\n", task2delete, ltsk->pid, ltsk, ltsk->comm);
			rt_free(task2delete->msg_buf[0]);
			rt_free(task2delete);
			rt_drg_on_adr(task2delete);
			break;
                }
        }
	rt_global_sti();
}

int rt_signal_linux_task(struct task_struct *lxt, int sig, RT_TASK *rtt)
{
	rt_global_cli();
	sigsysrq.waitq[sigsysrq.in].tsk     = lxt;
	sigsysrq.waitq[sigsysrq.in].sig     = sig ;
	sigsysrq.waitq[sigsysrq.in].rt_task = rtt;
	sigsysrq.in = (sigsysrq.in + 1) & (MAX_SRQ - 1);
	rt_global_sti();
	rt_pend_linux_srq(sigsysrq.srq);
	return 0;
}
