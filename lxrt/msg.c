/*
COPYRIGHT (C) 2000  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)
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

$Id: msg.c,v 1.1.1.1 2004/06/06 14:02:34 rpm Exp $
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
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>

#include "registry.h"
#include "proxies.h"
#include "msg.h"
#include "tid.h"

int rt_Send(pid_t pid, void *smsg, void *rmsg, size_t ssize, size_t rsize)
{
	RT_TASK *task, *replier;
	MSGCB cb;
	unsigned int replylen;

	if ((task = pid2rttask(pid))) {
		cb.cmd    = SYNCMSG;
		cb.sbuf   = smsg;
		cb.sbytes = ssize; 
		cb.rbuf   = rmsg;
		cb.rbytes = rsize;
		if (!(replier = rt_rpc(task, (unsigned int)&cb, &replylen))) {
			return -EINVAL;
		} else if (replier != task) {
			return -ESRCH;
		}
		return replylen ;
	}
	return -ESRCH;
}

pid_t rt_Receive(pid_t pid, void *msg, size_t maxsize, size_t *msglen)
{
	RT_TASK *task;
	MSGCB *cb;

	task = pid ? pid2rttask(pid) : 0;
	if ((task = rt_receive(task, (unsigned int *)&cb))) {
		if ((pid = rttask2pid(task))) {
			*msglen = maxsize <= cb->sbytes ? maxsize : cb->sbytes; 
			if (*msglen) {
				memcpy(msg, cb->sbuf, *msglen);
			}
			return pid;
		}
		return -ESRCH;
	}
	return -EINVAL;
}

pid_t rt_Creceive(pid_t pid, void *msg, size_t maxsize, size_t *msglen, RTIME delay)
{
	RT_TASK *task;
	MSGCB *cb;

	task = pid ? pid2rttask(pid) : 0;
	if (delay) {
		task = rt_receive_timed(task, (unsigned int *)&cb, delay);
	} else {
		task = rt_receive_if(task, (unsigned int *)&cb);
	}
	if (task) {
		if ((pid = rttask2pid(task))) {
			*msglen = maxsize <= cb->sbytes ? maxsize : cb->sbytes;
			if (*msglen) {
				memcpy(msg, cb->sbuf, *msglen);
			}
			return pid;
		}
		return 0;
	}
	return 0;
}

int rt_Reply(pid_t pid, void *msg, size_t size)
{
	RT_TASK *task, *retask;
	MSGCB *cb;
	unsigned int retlen;

	if ((task = pid2rttask(pid))) {
		if ((cb = (MSGCB *)task->msg)->cmd == SYNCMSG) {
			if ((retlen = size <= cb->rbytes ? size : cb->rbytes)) {
				memcpy(cb->rbuf, msg, retlen);
			}
			if (!(retask = rt_return(task, retlen))) {
				return -EINVAL;
			} else if (retask != task) {
				return -ESRCH;
			}
			return 0;
		}
		return -EPERM;
	}
	return -ESRCH;
}

// What a proxy agent is supposed to do for synchronous IPC.
static void Proxy_Task(RT_TASK *me)
{
        struct proxy_t *my;
	MSGCB cb;
        unsigned int replylen;

        my = (struct proxy_t *)me->stack_bottom;
	cb.cmd    = PROXY;
	cb.sbuf   = my->msg;
	cb.sbytes = my->nbytes;
	cb.rbuf   = &replylen;
	cb.rbytes = sizeof(replylen);

        while(1) {
		while (my->nmsgs) {
			atomic_dec((atomic_t *)&my->nmsgs);
                        rt_rpc(my->receiver, (unsigned int)(&cb), &replylen);
		}
		rt_task_suspend(me);
        }
}

// Create a synchronous IPC proxy task.
pid_t rt_Proxy_attach(pid_t pid, void *msg, int nbytes, int prio)
{
	RT_TASK *task;
	char proxy_name[8];

	task = pid ? pid2rttask(pid) : 0;
	task = __rt_proxy_attach((void *)Proxy_Task, task, msg, nbytes, prio);
	if (task) {
		if ((pid = assign_pid(task)) < 0) {
			rt_proxy_detach(task);
		} else if (task->lnxtsk) {
// A user space program may have created the proxy.
			pid2nam(pid, proxy_name);
			rt_register(nam2num(proxy_name), pid2rttask(task->retval), IS_PRX, task->lnxtsk);
        	}
		return pid;
        }
	return -ENOMEM;
}

// Delete a proxy task (a simplified specific rt_task_delete).
// Note: a self delete will not do the rt_free() call.
int rt_Proxy_detach(pid_t pid)
{
	RT_TASK *proxy;

	proxy = pid2rttask(pid);
	rt_vc_release(pid);
	if (!rt_task_delete(proxy)) {
// Not really a problem, called by owner of the proxy, either in RTAI or Linux 
// context, but, the proxy is a different task.
		rt_free(proxy);
		return 0;
	}
	return -EINVAL;
}

// Trigger a proxy.
pid_t rt_Trigger(pid_t pid)
{
	RT_TASK *proxy;
       	struct proxy_t *his;

	if ((proxy = pid2rttask(pid))) {
	        his = (struct proxy_t *)(proxy->stack_bottom);
        	if (his && proxy->magic == RT_TASK_MAGIC) {
	                atomic_inc((atomic_t *)&his->nmsgs);
        	        rt_task_resume(proxy);
                	return rttask2pid(his->receiver);
		}
		return -EINVAL;
	}
	return -ESRCH;
}
