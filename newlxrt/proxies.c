/*
COPYRIGHT (C) 2000  Pierre Cloutier  (pcloutier@poseidoncontrols.com)
                    and
                    Paolo Mantegazza (mantegazza@aero.polimi.it)

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

$Id: proxies.c,v 1.1.1.1 2004/06/06 14:02:48 rpm Exp $ 
*/


#include <linux/types.h>
#include <linux/sched.h>
#include <asm/system.h>
#include <asm/bitops.h>
#include <asm/smp.h>

#define INTERFACE_TO_LINUX
#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <rtai_lxrt.h>

#include "proxies.h"

extern int rt_kthread_init(RT_TASK *task,
			   void (*rt_thread)(int),
			   int data,
			   int stack_size,
			   int priority,
			   int uses_fpu,
			   void(*signal)(void));

// What any proxy is supposed to do, raw RTAI implementation.
static void proxy_task(RT_TASK *me)
{
	struct proxy_t *my;
	unsigned int ret;

	my = (struct proxy_t *)me->stack_bottom;        	
	while (1) {
		while (my->nmsgs) {
		 	atomic_dec((atomic_t *)&my->nmsgs);
			rt_rpc(my->receiver, *((unsigned int *)my->msg), &ret);
		}
		rt_task_suspend(me);
	}
}

// Create a raw proxy agent task.
RT_TASK *__rt_proxy_attach(void (*agent)(int), RT_TASK *task, void *msg, int nbytes, int priority)
{
	RT_TASK *proxy, *rt_current;
	struct proxy_t *my;

        rt_current = rt_whoami();
	if (!task) {
		task = rt_current;
	}

	if (task->magic != RT_TASK_MAGIC) {
		return 0;
	}

	if (!(proxy = rt_malloc(sizeof(RT_TASK)))) {
		return 0;
	}

	if (priority == -1 && (priority = rt_current->base_priority) == RT_LINUX_PRIORITY) {
		priority = RT_LOWEST_PRIORITY;
	}
	if (rt_kthread_init(proxy, agent, (int)proxy, PROXY_MIN_STACK_SIZE + nbytes + sizeof(struct proxy_t), priority, 0, 0)) {
		rt_free(proxy);
		return 0;
	}

	my = (struct proxy_t *)(proxy->stack_bottom);
	my->receiver = task ;
	my->msg      = ((char *)(proxy->stack_bottom)) + sizeof(struct proxy_t);
	my->nmsgs    = 0;
	my->nbytes   = nbytes;
	if (msg && nbytes) {
		memcpy(my->msg, msg, nbytes);
	}

        // agent is at *(proxy->stack + 2)
	return proxy;
}

// Create a raw proxy task.
RT_TASK *rt_proxy_attach(RT_TASK *task, void *msg, int nbytes, int prio)
{
	return __rt_proxy_attach((void *)proxy_task, task, msg, nbytes, prio);
}

// Delete a proxy task (a simplified specific rt_task_delete).
// Note: a self delete will not do the rt_free() call.
int rt_proxy_detach(RT_TASK *proxy)
{
	if (!rt_task_delete(proxy)) {
		rt_free(proxy);
		return 0;
	}
	return -EINVAL;
}

// Trigger a proxy.
RT_TASK *rt_trigger(RT_TASK *proxy)
{
	struct proxy_t *his;
	
	his = (struct proxy_t *)(proxy->stack_bottom);
	if (his && proxy->magic == RT_TASK_MAGIC) {
		atomic_inc((atomic_t *)&his->nmsgs);
		rt_task_resume(proxy);
		return his->receiver;
	}
	return (RT_TASK *)0;
}
