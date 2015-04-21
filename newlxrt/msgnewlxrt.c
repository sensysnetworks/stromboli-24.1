/*
COPYRIGHT (C) 2002  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)
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

$Id: msgnewlxrt.c,v 1.1.1.1 2004/06/06 14:02:47 rpm Exp $
*/

#include <linux/config.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/interrupt.h>

#include <asm/uaccess.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>

#include "msgnewlxrt.h"
#include "proxies.h"
#include "registry.h"

int rt_Send(pid_t pid, void *smsg, void *rmsg, size_t ssize, size_t rsize)
{
	RT_TASK *task;
	if ((task = pid2rttask(pid))) {
		MSGCB cb;
		RT_TASK *replier;
		unsigned int replylen;
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
	if ((task = rt_receive(pid ? pid2rttask(pid) : 0, (void *)&cb))) {
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
		task = rt_receive_timed(task, (void *)&cb, delay);
	} else {
		task = rt_receive_if(task, (void *)&cb);
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
	RT_TASK *task;
	if ((task = pid2rttask(pid))) {
		MSGCB *cb;
		if ((cb = (MSGCB *)task->msg)->cmd == SYNCMSG) {
			unsigned int retlen;
			RT_TASK *retask;
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

pid_t rt_Proxy_attach(pid_t pid, void *msg, int nbytes, int prio)
{
	RT_TASK *task;
	return (task = __rt_proxy_attach((void *)Proxy_Task, pid ? pid2rttask(pid) : 0, msg, nbytes, prio)) ? (task->lnxtsk)->pid : -ENOMEM;
}

int rt_Proxy_detach(pid_t pid)
{
	RT_TASK *proxy;
	if (!rt_task_delete(proxy = pid2rttask(pid))) {
		rt_free(proxy);
		return 0;
	}
	return -EINVAL;
}

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


pid_t rt_Name_attach(const char *argname)
{
	if (current->comm[0] != 'U' && current->comm[1] != ':') {
	    	strncpy_from_user(((RT_TASK *)current->this_rt_task[0])->task_name, argname, MAX_NAME_LENGTH);
	} else {
	    	strncpy(((RT_TASK *)current->this_rt_task[0])->task_name, argname, MAX_NAME_LENGTH);
	}
    	((RT_TASK *)current->this_rt_task[0])->task_name[MAX_NAME_LENGTH - 1] = 0;
	return strnlen(((RT_TASK *)current->this_rt_task[0])->task_name, MAX_NAME_LENGTH) > (MAX_NAME_LENGTH - 1) ? -EINVAL : ((struct task_struct *)current->this_rt_task[1])->pid;
}

pid_t rt_Name_locate(const char *arghost, const char *argname)
{
	extern RT_TASK rt_smp_linux_task[];
	int cpuid;
	RT_TASK *task;
        for (cpuid = 0; cpuid < smp_num_cpus; cpuid++) {
                task = &rt_smp_linux_task[cpuid];
                while ((task = task->next)) {
			if (!strncmp(argname, task->task_name, MAX_NAME_LENGTH - 1)) {
				return ((struct task_struct *)(task->lnxtsk)->this_rt_task[1])->pid;

			}
		}
	}
	return strlen(argname) <= 6 && (task = rt_get_adr(nam2num(argname))) ? rttask2pid(task) : 0;
}

int rt_Name_detach(pid_t pid)
{
	if (pid == ((struct task_struct *)current->this_rt_task[1])->pid) {
	    	((RT_TASK *)current->this_rt_task[0])->task_name[0] = 0;
		return 0;
	}
	return -EINVAL;
}
