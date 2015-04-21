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

$Id: proxies.h,v 1.1.1.1 2004/06/06 14:02:48 rpm Exp $ 
*/

#ifndef _PROXIES_H_
#define _PROXIES_H_

#define RT_TASK_MAGIC 0x754d2774
#define PROXY_MIN_STACK_SIZE 2048
#define RT_SCHEDULE() {extern void (*dnepsus_trxl)(void); (*dnepsus_trxl)();}

// Create a generic proxy task.
RT_TASK *__rt_proxy_attach(void (*func)(int), RT_TASK *task, void *msg, int nbytes, int priority);

// Create a raw proxy task.
RT_TASK *rt_proxy_attach(RT_TASK *task, void *msg, int nbytes, int priority);

// Delete a proxy task (a simplified specific rt_task_delete).
int rt_proxy_detach(RT_TASK *proxy);

//Trigger a proxy.
RT_TASK *rt_trigger(RT_TASK *proxy);

#endif // _PROXIES_H_
