/*
COPYRIGHT (C) 2001  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#ifndef _RTAI_DSCHED_H_
#define _RTAI_DSCHED_H_

#define MAX_STUBS     32  // _M_U_S_T___B_E___P_O_W_E_R___O_F___2_
#define MAX_SOCKS     32
#define MAX_MSG_SIZE  1500

#define NET_RPC_EXT  0
#define NET_RPC_IDX  2

#define NET_GET_ADR            	 	 0

#define NET_NAMED_TASK_INIT	 	 1
#define NET_NAMED_TASK_INIT_CPUID	 2
#define NET_NAMED_TASK_DELETE		 3
#define NET_GET_TIME_NS			 4
#define NET_GET_TIME_NS_CPUID		 5
#define NET_GET_CPU_TIME_NS		 6
#define NET_TASK_SUSPEND    		 7
#define NET_TASK_RESUME    		 8
#define NET_SLEEP              	 	 9
#define NET_SLEEP_UNTIL        	 	10

#define NET_NAMED_SEM_INIT		11
#define NET_NAMED_SEM_DELETE		12
#define NET_SEM_SIGNAL         	 	13
#define NET_SEM_BROADCAST      	 	14
#define NET_SEM_WAIT           	 	15
#define NET_SEM_WAIT_IF        	 	16
#define NET_SEM_WAIT_UNTIL		17
#define NET_SEM_WAIT_TIMED      	18

#define NET_SEND		    	19
#define NET_SEND_IF		    	20
#define NET_SEND_UNTIL		   	21
#define NET_SEND_TIMED		   	22

#define NET_RPC          	 	23
#define NET_RPC_IF       	 	24
#define NET_RPC_UNTIL    	 	25
#define NET_RPC_TIMED    	 	26

#define NET_ISRPC	   	 	27

#define NET_NAMED_MBX_INIT		28
#define NET_NAMED_MBX_DELETE		29
#define NET_MBX_SEND            	30
#define NET_MBX_SEND_WP         	31
#define NET_MBX_SEND_IF         	32
#define NET_MBX_SEND_UNTIL      	33
#define NET_MBX_SEND_TIMED      	34
#define NET_MBX_RECEIVE        	 	35
#define NET_MBX_RECEIVE_WP     	 	36
#define NET_MBX_RECEIVE_IF     	 	37
#define NET_MBX_RECEIVE_UNTIL  	 	38
#define NET_MBX_RECEIVE_TIMED  	 	39

#define NET_SENDX		    	40
#define NET_SENDX_IF		    	41
#define NET_SENDX_UNTIL		   	42
#define NET_SENDX_TIMED		   	43

#define NET_RPCX          	 	44
#define NET_RPCX_IF       	 	45
#define NET_RPCX_UNTIL    	 	46
#define NET_RPCX_TIMED    	 	47

#define NET_MBX_EVDRP    	 	48


#define LOW   0
#define HIGH  1

// for writes
#define UW1(bf, sz)  ((((unsigned long long)((((bf) & 0x7F) <<  2) | (((sz) & 0x7F) <<  9))) << 32) | 0x300000001LL)
#define UW2(bf, sz)  ((((unsigned long long)((((bf) & 0x7F) << 16) | (((sz) & 0x7F) << 23))) << 32) | 0x300000001LL)
#define UWSZ1LL      (0x4000000300000001LL)
#define UWSZ2LL      (0x8000000300000001LL)

// for reads
#define UR1(bf, sz)  ((((bf) & 0x7F) <<  2) | (((sz) & 0x7F) <<  9) | 0x300000001LL)
#define UR2(bf, sz)  ((((bf) & 0x7F) << 16) | (((sz) & 0x7F) << 23) | 0x300000001LL)
#define URSZ1LL      (0x340000001LL)
#define URSZ2LL      (0x380000001LL)

#define SIZARG sizeof(arg)

#define PACKPORT(port, ext, fun, timed) (((port) << 18) | ((timed) << 13) | ((ext) << 8) | (fun))

#define PORT(i) ((i) >> 18)
#define FUN(i) ((i) & 0xFF)
#define EXT(i) (((i) >> 8) & 0x1F)
#define TIMED(i) (((i) >> 13) & 0x1F)

#define PRT_REQ  1
#define PRT_SRV  2
#define PRT_RTR  3
#define PRT_RCV  4
#define RPC_REQ  5
#define RPC_SRV  6
#define RPC_RTR  7
#define RPC_RCV  8


#define OWNER(node, task) \
	((((unsigned long long)(node)) << 32) | (unsigned long)(task))

#ifdef __KERNEL__

#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include "../lxrt/registry.h"

extern void *RT_get_adr(unsigned long node, int port, const char *sname);

extern RT_TASK *RT_named_task_init(unsigned long node, int port, const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void));

extern RT_TASK *RT_named_task_init_cpuid(unsigned long node, int port, const char *task_name, void (*thread)(int), int data, int stack_size, int prio, int uses_fpu, void(*signal)(void), unsigned int run_on_cpu);

extern int RT_named_task_delete(unsigned long node, int port, RT_TASK *task);

extern RTIME RT_get_time_ns(unsigned long node, int port);

extern RTIME RT_get_time_ns_cpuid(unsigned long node, int port, int cpuid);

extern RTIME RT_get_cpu_time_ns(unsigned long node, int port);

extern int RT_task_suspend(unsigned long node, int port, RT_TASK *task);

extern int RT_task_resume(unsigned long node, int port, RT_TASK *task);

extern void RT_sleep(unsigned long node, int port, RTIME delay);

extern void RT_sleep_until(unsigned long node, int port, RTIME time);

extern SEM *RT_typed_named_sem_init(unsigned long node, int port, const char *sem_name, int value, int type);

extern int RT_named_sem_delete(unsigned long node, int port, SEM *sem);

extern int RT_sem_signal(unsigned long node, int port, SEM *sem);

extern int RT_sem_broadcast(unsigned long node, int port, SEM *sem);

extern int RT_sem_wait(unsigned long node, int port, SEM *sem);

extern int RT_sem_wait_if(unsigned long node, int port, SEM *sem);

extern int RT_sem_wait_until(unsigned long node, int port, SEM *sem, RTIME time);

extern int RT_sem_wait_timed(unsigned long node, int port, SEM *sem, RTIME delay);

extern RT_TASK *RT_send(unsigned long node, int port, RT_TASK *task, unsigned int msg);

extern RT_TASK *RT_send_if(unsigned long node, int port, RT_TASK *task, unsigned int msg);

extern RT_TASK *RT_send_until(unsigned long node, int port, RT_TASK *task, unsigned int msg, RTIME time);

extern RT_TASK *RT_send_timed(unsigned long node, int port, RT_TASK *task, unsigned int msg, RTIME delay);

extern RT_TASK *RT_evdrp(unsigned long node, int port, RT_TASK *task, unsigned int *msg);

extern RT_TASK *RT_receive(unsigned long node, int port, RT_TASK *task, unsigned int *msg);

extern RT_TASK *RT_receive_if(unsigned long node, int port, RT_TASK *task, unsigned int *msg);

extern RT_TASK *RT_receive_until(unsigned long node, int port, RT_TASK *task, unsigned int *msg, RTIME time);

extern RT_TASK *RT_receive_timed(unsigned long node, int port, RT_TASK *task, unsigned int *msg, RTIME delay);

extern RT_TASK *RT_rpc(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret);

extern RT_TASK *RT_rpc_if(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret);

extern RT_TASK *RT_rpc_until(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret, RTIME time);

extern RT_TASK *RT_rpc_timed(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret, RTIME delay);

extern int RT_isrpc(unsigned long node, int port, RT_TASK *task);

extern RT_TASK *RT_return(unsigned long node, int port, RT_TASK *task, unsigned int result);

extern RT_TASK *RT_rpcx(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize);

extern RT_TASK *RT_rpcx_if(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize);

extern RT_TASK *RT_rpcx_until(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME time);

extern RT_TASK *RT_rpcx_timed(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME delay);

extern RT_TASK *RT_sendx(unsigned long node, int port, RT_TASK *task, void *msg, int size);

extern RT_TASK *RT_sendx_if(unsigned long node, int port, RT_TASK *task, void *msg, int size);

extern RT_TASK *RT_sendx_until(unsigned long node, int port, RT_TASK *task, void *msg, int size, RTIME time);

extern RT_TASK *RT_sendx_timed(unsigned long node, int port, RT_TASK *task, void *msg, int size, RTIME delay);

extern RT_TASK *RT_returnx(unsigned long node, int port, RT_TASK *task, void *msg, int size);

extern RT_TASK *RT_evdrpx(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len);

extern RT_TASK *RT_receivex(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len);

extern RT_TASK *RT_receivex_if(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len);

extern RT_TASK *RT_receivex_until(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len, RTIME time);

extern RT_TASK *RT_receivex_timed(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len, RTIME delay);

extern MBX *RT_typed_named_mbx_init(unsigned long node, int port, const char *mbx_name, int size, int qtype);

extern int RT_named_mbx_delete(unsigned long node, int port, MBX *mbx);

extern int RT_mbx_send(unsigned long node, int port, MBX *mbx, void *msg, int msg_size);

extern int RT_mbx_send_wp(unsigned long node, int port, MBX *mbx, void *msg, int msg_size);

extern int RT_mbx_send_if(unsigned long node, int port, MBX *mbx, void *msg, int msg_size);

extern int RT_mbx_evdrp(unsigned long node, int port, MBX *mbx, void *msg, int msg_size);

extern int RT_mbx_receive(unsigned long node, int port, MBX *mbx, void *msg, int msg_size);

extern int RT_mbx_receive_if(unsigned long node, int port, MBX *mbx, void *msg, int msg_size);

extern int RT_mbx_send_until(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME time);

extern int RT_mbx_send_timed(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME delay);

extern int RT_mbx_receive(unsigned long node, int port, MBX *mbx, void *msg, int msg_size);

extern int RT_mbx_receive_wp(unsigned long node, int port, MBX *mbx, void *msg, int msg_size);

extern int RT_mbx_receive_if(unsigned long node, int port, MBX *mbx, void *msg, int msg_size);

extern int RT_mbx_receive_until(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME time);

extern int RT_mbx_receive_timed(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME delay);

extern int rt_send_req_rel_port(unsigned long node, int port, unsigned long id, MBX *mbx, int hard);

extern unsigned long ddn2nl(const char *ddn);

extern unsigned long rt_set_this_node(const char *ddn, unsigned long node, int hard);

extern RT_TASK *rt_find_asgn_stub(unsigned long long owner, int asgn);

extern int rt_rel_stub(unsigned long long owner);

extern int rt_waiting_return(unsigned long node, int port);

extern int rt_sync_net_rpc(unsigned long node, int port);

extern int rt_get_net_rpc_ret(MBX *mbx, unsigned long long *retval, void *msg1, int *msglen1, void *msg2, int *msglen2, RTIME timeout, int type);

#else

#include <stdlib.h>
#include <errno.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>

extern union rtai_lxrt_t rtai_lxrt(short int dynx, short int lsize, int srq, void *arg);

#define SIZARGS sizeof(args)

static inline void *RT_get_adr(unsigned long node, int port, const char *sname)
{
	if (node) {
		struct { int name; } arg = { nam2num(sname) };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_GET_ADR, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	} 
	return rt_get_adr(nam2num(sname));
} 

static inline RTIME RT_get_time_ns(unsigned long node, int port)
{
	if (node) {
		struct { int dummy; } arg = { 0 };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_GET_TIME_NS, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).rt;
	}
	return rt_get_time_ns();
} 

static inline RTIME RT_get_time_ns_cpuid(unsigned long node, int port, int cpuid)
{
	if (node) {
		struct { int cpuid; } arg = { cpuid };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_GET_TIME_NS_CPUID, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).rt;
	}
	return rt_get_time_ns_cpuid(cpuid);
} 

static inline RTIME RT_get_cpu_time_ns(unsigned long node, int port)
{
	if (node) {
		struct { int dummy; } arg = { 0 };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_GET_CPU_TIME_NS, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).rt;
	}
	return rt_get_cpu_time_ns();
} 

static inline void RT_task_suspend(unsigned long node, int port, RT_TASK *task)
{
	if (node) {
		struct { RT_TASK *task; } arg = { task };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_TASK_SUSPEND, 0), 0LL, &arg, SIZARG };
		rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args);
		return;
	}
	rt_task_suspend(task);
} 

static inline void RT_task_resume(unsigned long node, int port, RT_TASK *task)
{
	if (node) {
		struct { RT_TASK *task; } arg = { task };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_TASK_RESUME, 0), 0LL, &arg, SIZARG };
		rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args);
		return;
	}
	rt_task_resume(task);
} 

static inline void RT_sleep(unsigned long node, int port, RTIME delay)
{
	if (node) {
		struct { RTIME delay; } arg = { delay };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SLEEP, 1), 0LL, &arg, SIZARG };
		rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args);
		return;
	}
	rt_sleep(nano2count(delay));
} 

static inline void RT_sleep_until(unsigned long node, int port, RTIME time)
{
	if (node) {
		struct { RTIME time; } arg = { time };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SLEEP_UNTIL, 1), 0LL, &arg, SIZARG };
		rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args);
		return;
	}
	rt_sleep(nano2count(time));
} 

static inline int RT_sem_signal(unsigned long node, int port, SEM *sem)
{
	if (node) {
		struct { SEM *sem; } arg = { sem };
		struct { unsigned long node, fun; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEM_SIGNAL, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_sem_signal(sem);
} 

static inline int RT_sem_broadcast(unsigned long node, int port, SEM *sem)
{
	if (node) {
		struct { SEM *sem; } arg = { sem };
		struct { unsigned long node, fun; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEM_BROADCAST, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	}
	return rt_sem_broadcast(sem);
} 

static inline int RT_sem_wait(unsigned long node, int port, SEM *sem)
{
	if (node) {
		struct { SEM *sem; } arg = { sem };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEM_WAIT, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_sem_wait(sem);
} 

static inline int RT_sem_wait_if(unsigned long node, int port, SEM *sem)
{
	if (node) {
		struct { SEM *sem; } arg = { sem };
		struct { unsigned long node, fun; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEM_WAIT_IF, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	}
	return rt_sem_wait_if(sem);
} 

static inline int RT_sem_wait_until(unsigned long node, int port, SEM *sem, RTIME time)
{
	if (node) {
		struct { SEM *sem; RTIME time; } arg = { sem, time };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEM_WAIT_UNTIL, 2), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	}
	return rt_sem_wait_until(sem, nano2count(time));
} 

static inline int RT_sem_wait_timed(unsigned long node, int port, SEM *sem, RTIME delay)
{
	if (node) {
		struct { SEM *sem; RTIME delay; } arg = { sem, delay };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEM_WAIT_TIMED, 2), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_sem_wait_timed(sem, nano2count(delay));
} 

static inline RT_TASK *RT_send(unsigned long node, int port, RT_TASK *task, unsigned int msg)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; } arg = { task, msg };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEND, 0), 0LL, &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	} 
	return rt_send(task, msg);
} 

static inline RT_TASK *RT_send_if(unsigned long node, int port, RT_TASK *task, unsigned int msg)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; } arg = { task, msg };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEND_IF, 0), 0LL, &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	} 
	return rt_send_if(task, msg);
} 

static inline RT_TASK *RT_send_until(unsigned long node, int port, RT_TASK *task, unsigned int msg, RTIME time)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; RTIME time; } arg = { task, msg, time };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEND_UNTIL, 3), 0LL, &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	} 
	return rt_send_until(task, msg, nano2count(time));
} 

static inline RT_TASK *RT_send_timed(unsigned long node, int port, RT_TASK *task, unsigned int msg, RTIME delay)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; RTIME delay; } arg = { task, msg, delay };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SEND_TIMED, 3), 0LL, &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	} 
	return rt_send_timed(task, msg, nano2count(delay));
} 

static inline RT_TASK *rt_find_asgn_stub(unsigned long long owner, int asgn)
{
	struct { unsigned long long owner; int asgn; } args = { owner, asgn };
	return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 4, &args).v[LOW];
} 

static inline RT_TASK *RT_evdrp(unsigned long node, int port, RT_TASK *task, unsigned int *msg)
{
        if (!task || !node) {
		return rt_evdrp(task, msg);
	} 
	return rt_evdrp(rt_find_asgn_stub(OWNER(node, task), 1), msg) ? task : 0;
} 

static inline RT_TASK *RT_receive(unsigned long node, int port, RT_TASK *task, unsigned int *msg)
{
        if (!task || !node) {
		return rt_receive(task, msg);
	} 
	return rt_receive(rt_find_asgn_stub(OWNER(node, task), 1), msg) ? task : 0;
} 

static inline RT_TASK *RT_receive_if(unsigned long node, int port, RT_TASK *task, unsigned int *msg)
{
        if (!task || !node) {
		return rt_receive_if(task, msg);
	} 
	return rt_receive_if(rt_find_asgn_stub(OWNER(node, task), 1), msg) ? task : 0;
} 

static inline RT_TASK *RT_receive_until(unsigned long node, int port, RT_TASK *task, unsigned int *msg, RTIME time)
{
        if (!task || !node) {
		return rt_receive_until(task, msg, nano2count(time));
	} 
	return rt_receive_until(rt_find_asgn_stub(OWNER(node, task), 1), msg, nano2count(time)) ? task : 0;
} 

static inline RT_TASK *RT_receive_timed(unsigned long node, int port, RT_TASK *task, unsigned int *msg, RTIME delay)
{
        if (!task || !node) {
		return rt_receive_timed(task, msg, nano2count(delay));
	} 
	return rt_receive_timed(rt_find_asgn_stub(OWNER(node, task), 1), msg, nano2count(delay)) ? task : 0;
} 

static inline RT_TASK *RT_rpc(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; unsigned int *ret; } arg = { task, msg, ret };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_RPC, 0), UW1(3, 0), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_rpc(task, msg, ret);
} 

static inline RT_TASK *RT_rpc_if(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; unsigned int *ret; } arg = { task, msg, ret };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_RPC_IF, 0), UW1(3, 0), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_rpc_if(task, msg, ret);
} 

static inline RT_TASK *RT_rpc_until(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret, RTIME time)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; unsigned int *ret; RTIME time; } arg = { task, msg, ret, time };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_RPC_UNTIL, 4), UW1(3, 0), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_rpc_until(task, msg, ret, nano2count(time));
} 

static inline RT_TASK *RT_rpc_timed(unsigned long node, int port, RT_TASK *task, unsigned int msg, unsigned int *ret, RTIME delay)
{
	if (node) {
		struct { RT_TASK *task; unsigned int msg; unsigned int *ret; RTIME delay; } arg = { task, msg, ret, delay };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_RPC_TIMED, 4), UW1(3, 0), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_rpc_timed(task, msg, ret, nano2count(delay));
} 

static inline int RT_isrpc(unsigned long node, int port, RT_TASK *task)
{
	if (node) {
		struct { RT_TASK *task; } arg = { task };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_ISRPC, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_isrpc(task);
} 

static inline RT_TASK *RT_return(unsigned long node, int port, RT_TASK *task, unsigned int result)
{

        if (!task || !node) {
		return rt_return(task, result);
	} 
	return rt_return(rt_find_asgn_stub(OWNER(node, task), 1), result) ? task : 0;
} 

static inline RT_TASK *RT_rpcx(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	if (node) {
		struct { RT_TASK *task; void *smsg, *rmsg; int ssize, rsize; } arg = { task, smsg, rmsg, ssize, rsize };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_RPCX, 0), UR1(2, 4) | UW1(3, 5), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_rpcx(task, smsg, rmsg, ssize, rsize);
} 

static inline RT_TASK *RT_rpcx_if(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	if (node) {
		struct { RT_TASK *task; void *smsg, *rmsg; int ssize, rsize; } arg = { task, smsg, rmsg, ssize, rsize };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_RPCX_IF, 0), UR1(2, 4) | UW1(3, 5), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_rpcx_if(task, smsg, rmsg, ssize, rsize);
} 

static inline RT_TASK *RT_rpcx_until(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME time)
{
	if (node) {
		struct { RT_TASK *task; void *smsg, *rmsg; int ssize, rsize; RTIME time; } arg = { task, smsg, rmsg, ssize, rsize, time };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_RPCX_UNTIL, 6), UR1(2, 4) | UW1(3, 5), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_rpcx_until(task, smsg, rmsg, ssize, rsize, nano2count(time));
} 

static inline RT_TASK *RT_rpcx_timed(unsigned long node, int port, RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME delay)
{
	if (node) {
		struct { RT_TASK *task; void *smsg, *rmsg; int ssize, rsize; RTIME delay; } arg = { task, smsg, rmsg, ssize, rsize, delay };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_RPCX_TIMED, 6), UR1(2, 4) | UW1(3, 5), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_rpcx_timed(task, smsg, rmsg, ssize, rsize, nano2count(delay));
} 

static inline RT_TASK *RT_sendx(unsigned long node, int port, RT_TASK *task, void *msg, int size)
{
	if (node) {
		struct { RT_TASK *task; void *msg; int size; } arg = { task, msg, size };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SENDX, 0), UR1(2, 3), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_sendx(task, msg, size);
} 

static inline RT_TASK *RT_sendx_if(unsigned long node, int port, RT_TASK *task, void *msg, int size)
{
	if (node) {
		struct { RT_TASK *task; void *msg; int size; } arg = { task, msg, size };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SENDX_IF, 0), UR1(2, 3), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_sendx_if(task, msg, size);
} 

static inline RT_TASK *RT_sendx_until(unsigned long node, int port, RT_TASK *task, void *msg, int size, RTIME time)
{
	if (node) {
		struct { RT_TASK *task; void *msg; int size; RTIME time; } arg = { task, msg, size, time };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SENDX_UNTIL, 4), UR1(2, 3), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_sendx_until(task, msg, size, nano2count(time));
} 

static inline RT_TASK *RT_sendx_timed(unsigned long node, int port, RT_TASK *task, void *msg, int size, RTIME delay)
{
	if (node) {
		struct { RT_TASK *task; void *msg; int size; RTIME delay; } arg = { task, msg, size, delay };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_SENDX_TIMED, 4), UR1(2, 3), &arg, SIZARG };
		return (RT_TASK *)rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).v[LOW];
	}
	return rt_sendx_timed(task, msg, size, nano2count(delay));
} 

static inline RT_TASK *RT_returnx(unsigned long node, int port, RT_TASK *task, void *msg, int size)
{

        if (!task || !node) {
		return rt_returnx(task, msg, size);
	} 
	return rt_returnx(rt_find_asgn_stub(OWNER(node, task), 1), msg, size) ? task : 0;
} 

static inline RT_TASK *RT_evdrpx(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len)
{
        if (!task || !node) {
		return rt_evdrpx(task, msg, size, len);
	} 
	return rt_evdrpx(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len) ? task : 0;
} 

static inline RT_TASK *RT_receivex(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len)
{
        if (!task || !node) {
		return rt_receivex(task, msg, size, len);
	} 
	return rt_receivex(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len) ? task : 0;
} 

static inline RT_TASK *RT_receivex_if(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len)
{
        if (!task || !node) {
		return rt_receivex_if(task, msg, size, len);
	} 
	return rt_receivex_if(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len) ? task : 0;
} 

static inline RT_TASK *RT_receivex_until(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len, RTIME time)
{
        if (!task || !node) {
		return rt_receivex_until(task, msg, size, len, nano2count(time));
	} 
	return rt_receivex_until(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len, nano2count(time)) ? task : 0;
} 

static inline RT_TASK *RT_receivex_timed(unsigned long node, int port, RT_TASK *task, void *msg, int size, int *len, RTIME delay)
{
        if (!task || !node) {
		return rt_receivex_timed(task, msg, size, len, nano2count(delay));
	} 
	return rt_receivex_timed(rt_find_asgn_stub(OWNER(node, task), 1), msg, size, len, nano2count(delay)) ? task : 0;
} 

static inline int RT_mbx_send(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND, 0), UR1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_mbx_send(mbx, msg, msg_size);
} 

static inline int RT_mbx_send_wp(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND_WP, 0), UR1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_mbx_send_wp(mbx, msg, msg_size);
} 

static inline int RT_mbx_send_if(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND_IF, 0), UR1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_mbx_send_if(mbx, msg, msg_size);
} 

static inline int RT_mbx_send_until(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME time)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; RTIME time; } arg = { mbx, msg, msg_size, time };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND_UNTIL, 4), UR1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_mbx_send_until(mbx, msg, msg_size, nano2count(time));
} 

static inline int RT_mbx_send_timed(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME delay)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; RTIME delay; } arg = { mbx, msg, msg_size, delay };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_SEND_TIMED, 4), UR1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_mbx_send_until(mbx, msg, msg_size, nano2count(delay));
} 

static inline int RT_mbx_evdrp(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_EVDRP, 0), UW1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_mbx_evdrp(mbx, msg, msg_size);
} 

static inline int RT_mbx_receive(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE, 0), UW1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_mbx_receive(mbx, msg, msg_size);
} 

static inline int RT_mbx_receive_wp(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE_WP, 0), UW1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_mbx_receive_wp(mbx, msg, msg_size);
} 

static inline int RT_mbx_receive_if(unsigned long node, int port, MBX *mbx, void *msg, int msg_size)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; } arg = { mbx, msg, msg_size };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE_IF, 0), UW1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return rt_mbx_receive_if(mbx, msg, msg_size);
} 

static inline int RT_mbx_receive_until(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME time)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; RTIME time; } arg = { mbx, msg, msg_size, time };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE_UNTIL, 4), UW1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	}
	return rt_mbx_receive_until(mbx, msg, msg_size, nano2count(time));
} 

static inline int RT_mbx_receive_timed(unsigned long node, int port, MBX *mbx, void *msg, int msg_size, RTIME delay)
{
	if (node) {
		struct { MBX *mbx; void *msg; int msg_size; RTIME delay; } arg = { mbx, msg, msg_size, delay };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(port, NET_RPC_EXT, NET_MBX_RECEIVE_TIMED, 4), UW1(2, 3), &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	}
	return rt_mbx_receive_timed(mbx, msg, msg_size, nano2count(delay));
} 

static inline int rt_send_req_rel_port(unsigned long node, int port, unsigned long id, MBX *mbx, int hard)
{
	struct { unsigned long node, port; unsigned long id; MBX *mbx; int hard; } args = { node, port, id, mbx, hard };
	return rtai_lxrt(NET_RPC_IDX, SIZARGS, 1, &args).i[LOW];
} 

static inline unsigned long ddn2nl(const char *ddn)
{
	struct { const char *ddn; } args = { ddn };
	return rtai_lxrt(NET_RPC_IDX, SIZARGS, 2, &args).i[LOW];
} 

static inline unsigned long rt_set_this_node(const char *ddn, unsigned long node, int hard)
{
	struct { const char *ddn; unsigned long node; int hard; } args = { ddn, node, hard };
	return rtai_lxrt(NET_RPC_IDX, SIZARGS, 3, &args).i[LOW];
} 

static inline int rt_waiting_return(unsigned long node, int port)
{
	struct { unsigned long node; int port; } args = { node, port };
	return rtai_lxrt(NET_RPC_IDX, SIZARGS, 6, &args).i[LOW];
} 

static inline int rt_sync_net_rpc(unsigned long node, int port)
{
	if (node) {
                struct { int dummy; } arg = { 0 };
		struct { unsigned long node, port; long long type; void *args; int argsize; } args = { node, PACKPORT(abs(port), NET_RPC_EXT, 0xFF, 0), 0LL, &arg, SIZARG };
		return rtai_lxrt(NET_RPC_IDX, SIZARGS, 0, &args).i[LOW];
	} 
	return 1;
} 

static inline int rt_get_net_rpc_ret(MBX *mbx, unsigned long long *retval, void *msg1, int *msglen1, void *msg2, int *msglen2, RTIME timeout, int type)
{
	struct { int wsize, w2size; unsigned long long retval; } reply;
	int ret;

	switch (type) {
		case NET_MBX_RECEIVE:
			ret = rt_mbx_receive(mbx, &reply, sizeof(reply));
			break;
		case NET_MBX_RECEIVE_WP:
			ret = rt_mbx_receive_wp(mbx, &reply, sizeof(reply));
			break;
		case NET_MBX_RECEIVE_IF:
			ret = rt_mbx_receive_if(mbx, &reply, sizeof(reply));
			break;
		case NET_MBX_RECEIVE_UNTIL:
			ret = rt_mbx_receive_until(mbx, &reply, sizeof(reply), timeout);
			break;
		case NET_MBX_RECEIVE_TIMED:
			ret = rt_mbx_receive_timed(mbx, &reply, sizeof(reply), timeout);
		default:
			ret = -1;
	}
	if (!ret) {
		*retval = reply.retval;
		if (reply.wsize) {
			if (*msglen1 > reply.wsize) {
				*msglen1 = reply.wsize;
			}
			rt_mbx_receive(mbx, msg1, *msglen1);
		} else {
			*msglen1 = 0;
		}
		if (reply.w2size) {
			if (*msglen2 > reply.w2size) {
				*msglen2 = reply.w2size;
			}
			rt_mbx_receive(mbx, msg2, *msglen2);
		} else {
			*msglen2 = 0;
		}
		return 0;
	}
	return ret;
}

#endif

#define RT_isrpcx(task)  RT_isrpc(task)

#define rt_request_port(node) \
	rt_send_req_rel_port(node, 0, 0, 0, 0)
#define rt_request_port_id(node, id) \
	rt_send_req_rel_port(node, 0, id, 0, 0)
#define rt_request_port_mbx(node, mbx) \
	rt_send_req_rel_port(node, 0, 0, mbx, 0)
#define rt_request_port_id_mbx(node, id, mbx) \
	rt_send_req_rel_port(node, 0, id, mbx, 0)

#define rt_request_hard_port(node) \
	rt_send_req_rel_port(node, 0, 0, 0, 1)
#define rt_request_hard_port_id(node, id) \
	rt_send_req_rel_port(node, 0, id, 0, 1)
#define rt_request_hard_port_mbx(node, mbx) \
	rt_send_req_rel_port(node, 0, 0, mbx, 1)
#define rt_request_hard_port_id_mbx(node, id, mbx) \
	rt_send_req_rel_port(node, 0, id, mbx, 1)

#define rt_release_port(node, port) \
	rt_send_req_rel_port(node, port, 0, 0, 0) 

#endif
