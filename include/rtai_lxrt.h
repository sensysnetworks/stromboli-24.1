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

/*
ACKNOWLEDGMENTS:
- Pierre Cloutier (pcloutier@poseidoncontrols.com) has suggested the 6 
  characters names and fixed many inconsistencies within this file.
*/

#ifndef _RTAI_LXRT_H_
#define _RTAI_LXRT_H_

#include <rtai_declare.h>
#include <rtai_types.h>

//union rtai_lxrt_t { RTIME rt; int i[2]; void *v[2]; };

// The function below has not been kept inlined because of the gcc optimization
// that caused missing function calls. We have not been able to force it to be
// called always by using various volatile construct. Any help to make it
// safely inlined will be appreciated.

#include <rtai_nam2num.h>

#define YIELD				 0
#define SUSPEND				 1
#define RESUME				 2
#define MAKE_PERIODIC 			 3
#define WAIT_PERIOD	 		 4
#define SLEEP		 		 5
#define SLEEP_UNTIL	 		 6
#define START_TIMER	 		 7
#define STOP_TIMER	 		 8
#define GET_TIME	 		 9
#define COUNT2NANO			10
#define NANO2COUNT			11
#define SEM_SIGNAL			12
#define SEM_WAIT			13
#define SEM_WAIT_IF			14
#define SEM_WAIT_UNTIL			15
#define SEM_WAIT_DELAY			16
#define BUSY_SLEEP			17
#define SENDMSG				18
#define SEND_IF				19
#define SEND_UNTIL			20
#define SEND_TIMED			21
#define RECEIVEMSG			22
#define RECEIVE_IF			23
#define RECEIVE_UNTIL			24
#define RECEIVE_TIMED			25
#define RPCMSG				26
#define RPC_IF				27
#define RPC_UNTIL			28
#define RPC_TIMED			29
#define ISRPC				30
#define RETURNMSG			31
#define SET_PERIODIC_MODE		32
#define SET_ONESHOT_MODE		33
#define SIGNAL_HANDLER	 		34
#define TASK_USE_FPU			35
#define LINUX_USE_FPU			36
#define PREEMPT_ALWAYS			37
#define GET_TIME_NS			38
#define GET_CPU_TIME_NS			39
#define MBX_SEND			40
#define MBX_SEND_WP 			41
#define MBX_SEND_IF 			42
#define MBX_SEND_UNTIL			43
#define MBX_SEND_TIMED			44
#define MBX_RECEIVE 			45
#define MBX_RECEIVE_WP 			46
#define MBX_RECEIVE_IF 			47
#define MBX_RECEIVE_UNTIL		48
#define MBX_RECEIVE_TIMED		49
#define SET_RUNNABLE_ON_CPUS		50 
#define SET_RUNNABLE_ON_CPUID		51	 
#define GET_TIMER_CPU			52	 
#define START_RT_APIC_TIMERS		53
#define PREEMPT_ALWAYS_CPUID		54
#define COUNT2NANO_CPUID		55
#define NANO2COUNT_CPUID		56
#define GET_TIME_CPUID			57
#define GET_TIME_NS_CPUID       	58
// synchronous IPC with proxies
#define RT_SEND                 	59
#define RT_RECEIVE              	60
#define RT_CRECEIVE          	   	61
#define RT_REPLY                	62
#define RT_PROXY_ATTACH         	63
#define RT_PROXY_DETACH         	64
#define RT_TRIGGER              	65
#define RT_NAME_ATTACH          	66
#define RT_NAME_DETACH          	67
#define RT_NAME_LOCATE          	68
// Qblk's 
#define RT_INITTICKQUEUE		69
#define RT_RELEASETICKQUEUE     	70
#define RT_QDYNALLOC            	71
#define RT_QDYNFREE             	72
#define RT_QDYNINIT             	73
#define RT_QBLKWAIT			74
#define RT_QBLKREPEAT			75
#define RT_QBLKSOON			76
#define RT_QBLKDEQUEUE			77
#define RT_QBLKCANCEL			78
#define RT_QSYNC			79
#define RT_QRECEIVE			80
#define RT_QLOOP			81
#define RT_QSTEP			82
#define RT_QBLKBEFORE			83
#define RT_QBLKAFTER			84
#define RT_QBLKUNHOOK			85
#define RT_QBLKRELEASE			86
#define RT_QBLKCOMPLETE			87
#define RT_QHOOKFLUSH			88
#define RT_QBLKATHEAD			89
#define RT_QBLKATTAIL			90
#define RT_QHOOKINIT			91
#define RT_QHOOKRELEASE			92
#define RT_QBLKSCHEDULE			93
#define RT_GETTICKQUEUEHOOK		94
// Testing
#define RT_BOOM				95
#define RTAI_MALLOC			96
#define RT_FREE				97
#define RT_MMGR_STATS			98
#define RT_STOMP                	99
// VC
#define RT_VC_ATTACH            	100
#define RT_VC_RELEASE           	101
#define RT_VC_RESERVE          		102
// Linux Signal Support
#define RT_GET_LINUX_SIGNAL		103
#define RT_GET_ERRNO			104
#define RT_SET_LINUX_SIGNAL_HANDLER	105
// A few ones forgotten
#define SEM_BROADCAST       		106
#define MAKE_PERIODIC_NS 		107
#define SET_SCHED_POLICY 		108
#define SET_RESUME_END 			109
#define SPV_RMS 			110
#define WAKEUP_SLEEPING			111
// Extended intertask messages
#define RPCX				112
#define RPCX_IF				113
#define RPCX_UNTIL			114
#define RPCX_TIMED			115
#define SENDX				116
#define SENDX_IF			117
#define SENDX_UNTIL			118
#define SENDX_TIMED			119
#define RETURNX				120
#define RECEIVEX			121
#define RECEIVEX_IF			122
#define RECEIVEX_UNTIL			123
#define RECEIVEX_TIMED			124
#define EVDRPX				125
// Again forgotten ones
#define EVDRP				126
#define MBX_EVDRP			127
#define CHANGE_TASK_PRIO 		128
#define SET_RESUME_TIME 		129
#define SET_PERIOD        		130
#define MBX_OVRWR_SEND                  131
#define HARD_TIMER_RUNNING              132
#define SEM_WAIT_BARRIER 		133
#define SEM_COUNT			134
#define COND_WAIT			135
#define COND_WAIT_UNTIL			136
#define COND_WAIT_TIMED			137
#define RWL_RDLOCK 			138
#define RWL_RDLOCK_IF 			139
#define RWL_RDLOCK_UNTIL 		140
#define RWL_RDLOCK_TIMED 		141
#define RWL_WRLOCK 			142	
#define RWL_WRLOCK_IF	 		143
#define RWL_WRLOCK_UNTIL		144
#define RWL_WRLOCK_TIMED		145
#define RWL_UNLOCK 			146
#define SPL_LOCK 			147	
#define SPL_LOCK_IF	 		148
#define SPL_LOCK_TIMED			149
#define SPL_UNLOCK 			150

#define MAX_LXRT_FUN 		500

#define GET_ADR	     		1000
#define GET_NAME       		1001
#define TASK_INIT      		1002
#define TASK_DELETE    		1003
#define SEM_INIT       		1004
#define SEM_DELETE		1005
#define MBX_INIT 		1006
#define MBX_DELETE		1007
#define MAKE_SOFT_RT		1008
#define MAKE_HARD_RT		1009
#define PRINT_TO_SCREEN		1010
#define NONROOT_HRT		1011
#define RT_BUDDY		1012
#define HRT_USE_FPU     	1013
#define USP_SIGHDL      	1014
#define GET_USP_FLAGS   	1015
#define SET_USP_FLAGS   	1016
#define GET_USP_FLG_MSK 	1017
#define SET_USP_FLG_MSK 	1018
#define IS_HARD         	1019
#define LXRT_FORK		1020
#define ALLOC_REGISTER 		1021
#define DELETE_DEREGISTER	1022
#define FORCE_TASK_SOFT  	1023
#define PRINTK			1024
#define GET_EXECTIME		1025
#define GET_TIMEORIG 		1026
#define RWL_INIT		1027
#define RWL_DELETE 		1028
#define SPL_INIT		1029
#define SPL_DELETE 		1030

#define FORCE_SOFT 0x80000000

struct rt_fun_entry { unsigned long long type; void *fun; };

#ifdef __KERNEL__

#define LXRT_RESUME(x) \
do { extern void emuser_trxl(RT_TASK *); emuser_trxl((x)); } while (0)

#define LXRT_SUSPEND(x) \
do { extern void (*dnepsus_trxl)(void); (*dnepsus_trxl)(); } while (0)

#define LXRT_RESTART(x, y) \
do { extern void tratser_trxl(void *, void *); tratser_trxl(x, y); } while (0)

/*       !!! ATTENTION: KEEP THE MACRO BELOW A POWER OF 2, ALWAYS !!!
  Must be >= the max number of expected soft-hard real time LINUX processes. */

#define MAX_SRQ    128
struct t_sigsysrq { int srq, in, out; struct { struct task_struct *tsk; int sig; RT_TASK *rt_task;} waitq[MAX_SRQ];};
 
extern int set_rtai_callback(void (*fun)(void));
extern void remove_rtai_callback(void (*fun)(void));
extern void linux_process_termination(void);
extern RT_TASK *rt_lxrt_whoami(void);
extern void exec_func(void (*func)(void *data, int evn), void *data, int evn);

int  set_rt_fun_ext_index(struct rt_fun_entry *fun, int idx);
void reset_rt_fun_ext_index( struct rt_fun_entry *fun, int idx);

//#ifdef LXRT_MODULE
#ifdef MODULE

#include <asm/rtai_lxrt.h>

/*
     Encoding of system call argument
            31                                    0  
soft SRQ    .... |||| |||| |||| .... .... .... ....  0 - 4095 max
int  NARG   .... .... .... .... |||| |||| |||| ||||  
arg  INDX   |||| .... .... .... .... .... .... ....
*/

/*
These USP (unsigned long long) type fields allow to read and write up to 2 arguments.  
                                               
RW marker .... .... .... .... .... .... .... ..|| .... .... .... .... .... .... .... ...|

HIGH unsigned long encodes writes
W ARG1 BF .... .... .... .... .... ...| |||| ||..
W ARG1 SZ .... .... .... .... |||| |||. .... ....
W ARG2 BF .... .... .||| |||| .... .... .... ....
W ARG2 SZ ..|| |||| |... .... .... .... .... ....
W 1st  LL .|.. .... .... .... .... .... .... ....
W 2nd  LL |... .... .... .... .... .... .... ....

LOW unsigned long encodes reads
R ARG1 BF .... .... .... .... .... ...| |||| ||..
R ARG1 SZ .... .... .... .... |||| |||. .... ....
R ARG2 BF .... .... .||| |||| .... .... .... ....
R ARG2 SZ ..|| |||| |... .... .... .... .... ....
R 1st  LL .|.. .... .... .... .... .... .... ....
R 2nd  LL |... .... .... .... .... .... .... ....

LOW unsigned long encodes also
RT Switch .... .... .... .... .... .... .... ...|

and 
Always 0  .... .... .... .... .... .... .... ..|.

If SZ is zero sizeof(int) is copied by default, if LL bit is set sizeof(long long) is copied.
*/

// These are for setting appropriate bits in any function entry structure, OR
// them in fun entry type to obtain the desired encoding

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

// and these are for deciding what to do in lxrt.c
#define	NEED_TO_RW(x)	(((unsigned long *)&(x))[HIGH])

#define NEED_TO_R(x)	(((unsigned long *)&(x))[LOW]  & 0x0000FFFC)
#define NEED_TO_W(x)	(((unsigned long *)&(x))[HIGH] & 0x0000FFFC)

#define NEED_TO_R2ND(x)	(((unsigned long *)&(x))[LOW]  & 0x3FFF0000)
#define NEED_TO_W2ND(x)	(((unsigned long *)&(x))[HIGH] & 0x3FFF0000)

#define USP_RBF1(x)  	((((unsigned long *)&(x))[LOW] >>  2) & 0x7F)
#define USP_RSZ1(x)    	((((unsigned long *)&(x))[LOW] >>  9) & 0x7F)
#define USP_RBF2(x)    	((((unsigned long *)&(x))[LOW] >> 16) & 0x7F)
#define USP_RSZ2(x)    	((((unsigned long *)&(x))[LOW] >> 23) & 0x7F)
#define USP_RSZ1LL(x)  	(((unsigned long *)&(x))[LOW] & 0x40000000)
#define USP_RSZ2LL(x)  	(((unsigned long *)&(x))[LOW] & 0x80000000)

#define USP_WBF1(x)   	((((unsigned long *)&(x))[HIGH] >>  2) & 0x7F)
#define USP_WSZ1(x)    	((((unsigned long *)&(x))[HIGH] >>  9) & 0x7F)
#define USP_WBF2(x)    	((((unsigned long *)&(x))[HIGH] >> 16) & 0x7F)
#define USP_WSZ2(x)    	((((unsigned long *)&(x))[HIGH] >> 23) & 0x7F)
#define USP_WSZ1LL(x)   (((unsigned long *)&(x))[HIGH] & 0x40000000)
#define USP_WSZ2LL(x)   (((unsigned long *)&(x))[HIGH] & 0x80000000)

#define SRQ(x)   (((x) >> 16) & 0xFFF)
#define NARG(x)  ((x) & 0xFFFF)
#define INDX(x)  (((x) >> 28) & 0xF)

extern struct rt_fun_entry rt_fun_lxrt[];

#endif // LXRT_MODULE
#else /* !__KERNEL__ */

#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <asm/rtai_lxrt.h>

#define rt_grow_and_lock_stack(incr) \
        do { \
                char buf[incr]; \
                memset(buf, 0, incr); \
                mlockall(MCL_CURRENT | MCL_FUTURE); \
        } while (0)

#include <asm/rtai_lxrt_sup.h>

#define BIDX   0 // rt_fun_ext[0]
#define SIZARG sizeof(arg)
//#define SRQ_NARG_EXT(srq,indx) ((srq) | ((indx) << 12))

DECLARE void *rt_get_adr(unsigned long name)
{
	struct { int name; } arg = { name };
	return rtai_lxrt(BIDX, SIZARG, GET_ADR, &arg).v[LOW];
} 

DECLARE unsigned long rt_get_name(void *adr)
{
	struct { void *adr; } arg = { adr };
	return rtai_lxrt(BIDX, SIZARG, GET_NAME, &arg).i[LOW];
}

DECLARE RT_TASK *rt_task_init_schmod(int name, int priority, int stack_size, int max_msg_size, int policy, int cpus_allowed)
{
        struct sched_param mysched;
        struct { int name, priority, stack_size, max_msg_size, cpus_allowed; } arg = { name, priority, stack_size, max_msg_size, cpus_allowed };

        mysched.sched_priority = sched_get_priority_max(policy) - priority;
        if (mysched.sched_priority < 1 ) {
        	mysched.sched_priority = 1;
	}
        if (sched_setscheduler(0, policy, &mysched) < 0) {
                return 0;
        }

//	if (!rtai_lxrt(BIDX, SIZARG, RT_BUDDY, &arg).v[LOW] && !rt_get_adr(name)) {
                return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, TASK_INIT, &arg).v[LOW];
//        }
// ???? return 0;
}

DECLARE RT_TASK *rt_task_init(int name, int priority, int stack_size, int
max_msg_size)
{
        struct { int name, priority, stack_size, max_msg_size, cpus_allowed; } arg = { name, priority, stack_size, max_msg_size, 0xFF };
//	if (!rtai_lxrt(BIDX, SIZARG, RT_BUDDY, &arg).v[LOW] && !rt_get_adr(name)) {
                return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, TASK_INIT, &arg).v[LOW];
//        }
// ???? return 0;
}

DECLARE void rt_set_sched_policy(RT_TASK *task, int policy, int rr_quantum_ns)
{
	struct { RT_TASK *task; int policy; int rr_quantum_ns; } arg = { task, policy, rr_quantum_ns };
	rtai_lxrt(BIDX, SIZARG, SET_SCHED_POLICY, &arg);
}

DECLARE int rt_change_prio(RT_TASK *task, int priority)
{
	struct { RT_TASK *task; int priority; } arg = { task, priority };
	return rtai_lxrt(BIDX, SIZARG, CHANGE_TASK_PRIO, &arg).i[LOW];
}

DECLARE void rt_make_soft_real_time(void)
{
	struct { unsigned long dummy; } arg;
	rtai_lxrt(BIDX, SIZARG, MAKE_SOFT_RT, &arg);
}

DECLARE int rt_task_delete(RT_TASK *task)
{
	struct { RT_TASK *task; } arg = { task };
	rt_make_soft_real_time();
	return rtai_lxrt(BIDX, SIZARG, TASK_DELETE, &arg).i[LOW];
}

DECLARE int rt_task_yield(void)
{
	struct { unsigned long dummy; } arg;
	return rtai_lxrt(BIDX, SIZARG, YIELD, &arg).i[LOW];
}

DECLARE int rt_task_suspend(RT_TASK *task)
{
	struct { RT_TASK *task; } arg = { task };
	return rtai_lxrt(BIDX, SIZARG, SUSPEND, &arg).i[LOW];
}

DECLARE int rt_task_resume(RT_TASK *task)
{
	struct { RT_TASK *task; } arg = { task };
	return rtai_lxrt(BIDX, SIZARG, RESUME, &arg).i[LOW];
}

DECLARE int rt_task_make_periodic(RT_TASK *task, RTIME start_time, RTIME period)
{
	struct { RT_TASK *task; RTIME start_time, period; } arg = { task, start_time, period };
	return rtai_lxrt(BIDX, SIZARG, MAKE_PERIODIC, &arg).i[LOW];
}

DECLARE int rt_task_make_periodic_relative_ns(RT_TASK *task, RTIME start_delay, RTIME period)
{
	struct { RT_TASK *task; RTIME start_time, period; } arg = { task, start_delay, period };
	return rtai_lxrt(BIDX, SIZARG, MAKE_PERIODIC_NS, &arg).i[LOW];
}

DECLARE void rt_task_wait_period(void)
{
	struct { unsigned long dummy; } arg;
	rtai_lxrt(BIDX, SIZARG, WAIT_PERIOD, &arg);
}

DECLARE void rt_sleep(RTIME delay)
{
	struct { RTIME delay; } arg = { delay };
	rtai_lxrt(BIDX, SIZARG, SLEEP, &arg);
}

DECLARE void rt_sleep_until(RTIME time)
{
	struct { RTIME time; } arg = { time };
	rtai_lxrt(BIDX, SIZARG, SLEEP_UNTIL, &arg);
}

DECLARE int rt_is_hard_timer_running(void)
{
	struct { unsigned long dummy; } arg;
	return rtai_lxrt(BIDX, SIZARG, HARD_TIMER_RUNNING, &arg).i[LOW];
}

DECLARE RTIME start_rt_timer(int period)
{
	struct { int period; } arg = { period };
	return rtai_lxrt(BIDX, SIZARG, START_TIMER, &arg).rt;
}

DECLARE void stop_rt_timer(void)
{
	struct { unsigned long dummy; } arg;
	rtai_lxrt(BIDX, SIZARG, STOP_TIMER, &arg);
}

DECLARE RTIME rt_get_time(void)
{
	struct { unsigned long dummy; } arg;
	return rtai_lxrt(BIDX, SIZARG, GET_TIME, &arg).rt;
}

DECLARE RTIME count2nano(RTIME count)
{
	struct { RTIME count; } arg = { count };
	return rtai_lxrt(BIDX, SIZARG, COUNT2NANO, &arg).rt;
}

DECLARE RTIME nano2count(RTIME nanos)
{
	struct { RTIME nanos; } arg = { nanos };
	return rtai_lxrt(BIDX, SIZARG, NANO2COUNT, &arg).rt;
}

DECLARE SEM *rt_typed_sem_init(int name, int value, int type)
{
	struct { int name, value, type; } arg = { name, value, type };
	return (SEM *)rtai_lxrt(BIDX, SIZARG, SEM_INIT, &arg).v[LOW];
}

#define rt_sem_init(name, value) rt_typed_sem_init(name, value, CNT_SEM)

DECLARE int rt_sem_delete(SEM *sem)
{
	struct { SEM *sem; } arg = { sem };
	return rtai_lxrt(BIDX, SIZARG, SEM_DELETE, &arg).i[LOW];
}

DECLARE int rt_sem_signal(SEM *sem)
{
	struct { SEM *sem; } arg = { sem };
	return rtai_lxrt(BIDX, SIZARG, SEM_SIGNAL, &arg).i[LOW];
}

DECLARE int rt_sem_broadcast(SEM *sem)
{
	struct { SEM *sem; } arg = { sem };
	return rtai_lxrt(BIDX, SIZARG, SEM_BROADCAST, &arg).i[LOW];
}

DECLARE int rt_sem_wait(SEM *sem)
{
	struct { SEM *sem; } arg = { sem };
	return rtai_lxrt(BIDX, SIZARG, SEM_WAIT, &arg).i[LOW];
}

DECLARE int rt_sem_wait_if(SEM *sem)
{
	struct { SEM *sem; } arg = { sem };
	return rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW];
}

DECLARE int rt_sem_wait_until(SEM *sem, RTIME time)
{
	struct { SEM *sem; RTIME time; } arg = { sem, time };
	return rtai_lxrt(BIDX, SIZARG, SEM_WAIT_UNTIL, &arg).i[LOW];
}

DECLARE int rt_sem_wait_timed(SEM *sem, RTIME delay)
{
	struct { SEM *sem; RTIME delay; } arg = { sem, delay };
	return rtai_lxrt(BIDX, SIZARG, SEM_WAIT_DELAY, &arg).i[LOW];
}

DECLARE int rt_sem_wait_barrier(SEM *sem)
{
	struct { SEM *sem; } arg = { sem };
	return rtai_lxrt(BIDX, SIZARG, SEM_WAIT_BARRIER, &arg).i[LOW];
}

DECLARE void rt_busy_sleep(int ns)
{
	struct { int ns; } arg = { ns };
	rtai_lxrt(BIDX, SIZARG, BUSY_SLEEP, &arg);
}

DECLARE RT_TASK *rt_send(RT_TASK *task, unsigned int msg)
{
	struct { RT_TASK *task; unsigned int msg; } arg = { task, msg };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, SENDMSG, &arg).v[LOW];
}

DECLARE RT_TASK *rt_send_if(RT_TASK *task, unsigned int msg)
{
	struct { RT_TASK *task; unsigned int msg; } arg = { task, msg };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, SEND_IF, &arg).v[LOW];
}

DECLARE RT_TASK *rt_send_until(RT_TASK *task, unsigned int msg, RTIME time)
{
	struct { RT_TASK *task; unsigned int msg; RTIME time; } arg = { task, msg, time };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, SEND_UNTIL, &arg).v[LOW];
}

DECLARE RT_TASK *rt_send_timed(RT_TASK *task, unsigned int msg, RTIME delay)
{
	struct { RT_TASK *task; unsigned int msg; RTIME delay; } arg = { task, msg, delay };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, SEND_TIMED, &arg).v[LOW];
}

DECLARE RT_TASK *rt_evdrp(RT_TASK *task, unsigned int *msg)
{
	struct { RT_TASK *task; unsigned int *msg; } arg = { task, msg };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, EVDRP, &arg).v[LOW];
}

DECLARE RT_TASK *rt_receive(RT_TASK *task, unsigned int *msg)
{
	struct { RT_TASK *task; unsigned int *msg; } arg = { task, msg };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RECEIVEMSG, &arg).v[LOW];
}

DECLARE RT_TASK *rt_receive_if(RT_TASK *task, unsigned int *msg)
{
	struct { RT_TASK *task; unsigned int *msg; } arg = { task, msg };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RECEIVE_IF, &arg).v[LOW];
}

DECLARE RT_TASK *rt_receive_until(RT_TASK *task, unsigned int *msg, RTIME time)
{
	struct { RT_TASK *task; unsigned int *msg; RTIME time; } arg = { task, msg, time };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RECEIVE_UNTIL, &arg).v[LOW];
}

DECLARE RT_TASK *rt_receive_timed(RT_TASK *task, unsigned int *msg, RTIME delay)
{
	struct { RT_TASK *task; unsigned int *msg; RTIME delay; } arg = { task, msg, delay };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RECEIVE_TIMED, &arg).v[LOW];
}

DECLARE RT_TASK *rt_rpc(RT_TASK *task, unsigned int to_do, unsigned int *result)
{
	struct { RT_TASK *task; unsigned int to_do; unsigned int *result; } arg = { task, to_do, result };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RPCMSG, &arg).v[LOW];
}

DECLARE RT_TASK *rt_rpc_if(RT_TASK *task, unsigned int to_do, unsigned int *result)
{
	struct { RT_TASK *task; unsigned int to_do; unsigned int *result; } arg = { task, to_do, result };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RPC_IF, &arg).v[LOW];
}

DECLARE RT_TASK *rt_rpc_until(RT_TASK *task, unsigned int to_do, unsigned int *result, RTIME time)
{
	struct { RT_TASK *task; unsigned int to_do; unsigned int *result; RTIME time; } arg = { task, to_do, result, time };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RPC_UNTIL, &arg).v[LOW];
}

DECLARE RT_TASK *rt_rpc_timed(RT_TASK *task, unsigned int to_do, unsigned int *result, RTIME delay)
{
	struct { RT_TASK *task; unsigned int to_do; unsigned int *result; RTIME delay; } arg = { task, to_do, result, delay };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RPC_TIMED, &arg).v[LOW];
}

DECLARE int rt_isrpc(RT_TASK *task)
{
	struct { RT_TASK *task; } arg = { task };
	return (int)rtai_lxrt(BIDX, SIZARG, ISRPC, &arg).i[LOW];
}

DECLARE RT_TASK *rt_return(RT_TASK *task, unsigned int result)
{
	struct { RT_TASK *task; unsigned int result; } arg = { task, result };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RETURNMSG, &arg).v[LOW];
}

DECLARE RT_TASK *rt_rpcx(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	struct { RT_TASK *task; void *smsg; void *rmsg; int ssize; int rsize; } arg = { task, smsg, rmsg, ssize, rsize };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RPCX, &arg).v[LOW];
}

DECLARE RT_TASK *rt_rpcx_if(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize)
{
	struct { RT_TASK *task; void *smsg; void *rmsg; int ssize; int rsize; } arg = { task, smsg, rmsg, ssize, rsize };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RPCX_IF, &arg).v[LOW];
}

DECLARE RT_TASK *rt_rpcx_until(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME time)
{
	struct { RT_TASK *task; void *smsg; void *rmsg; int ssize; int rsize; RTIME time; } arg = { task, smsg, rmsg, ssize, rsize, time };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RPCX_UNTIL, &arg).v[LOW];
}

DECLARE RT_TASK *rt_rpcx_timed(RT_TASK *task, void *smsg, void *rmsg, int ssize, int rsize, RTIME delay)
{
	struct { RT_TASK *task; void *smsg; void *rmsg; int ssize; int rsize; RTIME delay; } arg = { task, smsg, rmsg, ssize, rsize, delay };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RPCX_TIMED, &arg).v[LOW];
}

DECLARE RT_TASK *rt_sendx(RT_TASK *task, void *msg, int size)
{
	struct { RT_TASK *task; void *msg; int size; } arg = { task, msg, size };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, SENDX, &arg).v[LOW];
}

DECLARE RT_TASK *rt_sendx_if(RT_TASK *task, void *msg, int size)
{
	struct { RT_TASK *task; void *msg; int size; } arg = { task, msg, size };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, SENDX_IF, &arg).v[LOW];
}

DECLARE RT_TASK *rt_sendx_until(RT_TASK *task, void *msg, int size, RTIME time)
{
	struct { RT_TASK *task; void *msg; int size; RTIME time; } arg = { task, msg, size, time };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, SENDX_UNTIL, &arg).v[LOW];
}

DECLARE RT_TASK *rt_sendx_timed(RT_TASK *task, void *msg, int size, RTIME delay)
{
	struct { RT_TASK *task; void *msg; int size; RTIME delay; } arg = { task, msg, size, delay };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, SENDX_TIMED, &arg).v[LOW];
}

DECLARE RT_TASK *rt_returnx(RT_TASK *task, void *msg, int size)
{
	struct { RT_TASK *task; void *msg; int size; } arg = { task, msg, size };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RETURNX, &arg).v[LOW];
}

#define rt_isrpcx(task)  rt_isrpc(task)

DECLARE RT_TASK *rt_evdrpx(RT_TASK *task, void *msg, int size, int *len)
{
	struct { RT_TASK *task; void *msg; int size, *len; } arg = { task, msg, size, len };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, EVDRPX, &arg).v[LOW];
}

DECLARE RT_TASK *rt_receivex(RT_TASK *task, void *msg, int size, int *len)
{
	struct { RT_TASK *task; void *msg; int size, *len; } arg = { task, msg, size, len };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RECEIVEX, &arg).v[LOW];
}

DECLARE RT_TASK *rt_receivex_if(RT_TASK *task, void *msg, int size, int *len)
{
	struct { RT_TASK *task; void *msg; int size, *len; } arg = { task, msg, size, len };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RECEIVEX_IF, &arg).v[LOW];
}

DECLARE RT_TASK *rt_receivex_until(RT_TASK *task, void *msg, int size, int *len, RTIME time)
{
	struct { RT_TASK *task; void *msg; int size, *len; RTIME time; } arg = { task, msg, size, len, time };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RECEIVEX_UNTIL, &arg).v[LOW];
}

DECLARE RT_TASK *rt_receivex_timed(RT_TASK *task, void *msg, int size, int *len, RTIME delay)
{
	struct { RT_TASK *task; void *msg; int size, *len; RTIME delay; } arg = { task, msg, size, len, delay };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RECEIVEX_TIMED, &arg).v[LOW];
}

DECLARE void rt_set_periodic_mode(void)
{
	struct { unsigned long dummy; } arg;
	rtai_lxrt(BIDX, SIZARG, SET_PERIODIC_MODE, &arg);
}

DECLARE void rt_set_oneshot_mode(void)
{
	struct { unsigned long dummy; } arg;
	rtai_lxrt(BIDX, SIZARG, SET_ONESHOT_MODE, &arg);
}

DECLARE int rt_task_signal_handler(RT_TASK *task, void (*handler)(void))
{
	struct { RT_TASK *task; void (*handler)(void); } arg = { task, handler };
	return rtai_lxrt(BIDX, SIZARG, SIGNAL_HANDLER, &arg).i[LOW];
}

DECLARE int rt_task_use_fpu(RT_TASK *task, int use_fpu_flag)
{
        struct { RT_TASK *task; int use_fpu_flag; } arg = { task, use_fpu_flag };
        if (rtai_lxrt(BIDX, SIZARG, RT_BUDDY, &arg).v[LOW] != task) {
                return rtai_lxrt(BIDX, SIZARG, TASK_USE_FPU, &arg).i[LOW];
        } else {
// note that it would be enough to do whatever FP op here to have it OK. But
// that is scary if it is done when already in hard real time, and we do not
// want to force users to call this before making it hard.
                rtai_lxrt(BIDX, SIZARG, HRT_USE_FPU, &arg);
                return 0;
        }
}

DECLARE int rt_buddy_task_use_fpu(RT_TASK *task, int use_fpu_flag)
{
	struct { RT_TASK *task; int use_fpu_flag; } arg = { task, use_fpu_flag };
	return rtai_lxrt(BIDX, SIZARG, TASK_USE_FPU, &arg).i[LOW];
}

DECLARE int rt_linux_use_fpu(int use_fpu_flag)
{
	struct { int use_fpu_flag; } arg = { use_fpu_flag };
	return rtai_lxrt(BIDX, SIZARG, LINUX_USE_FPU, &arg).i[LOW];
}

DECLARE void rt_preempt_always(int yes_no)
{
	struct { int yes_no; } arg = { yes_no };
	rtai_lxrt(BIDX, SIZARG, PREEMPT_ALWAYS, &arg);
}

DECLARE RTIME rt_get_time_ns(void)
{
	struct { unsigned long dummy; } arg;
	return rtai_lxrt(BIDX, SIZARG, GET_TIME_NS, &arg).rt;
}

DECLARE RTIME rt_get_cpu_time_ns(void)
{
	struct { unsigned long dummy; } arg;
	return rtai_lxrt(BIDX, SIZARG, GET_CPU_TIME_NS, &arg).rt;
}

DECLARE void *rt_typed_mbx_init(int name, int size, int qtype)
{
	struct { int name; int size; int qtype; } arg = { name, size, qtype };
	return rtai_lxrt(BIDX, SIZARG, MBX_INIT, &arg).v[LOW];
}

#define rt_mbx_init(name, size) rt_typed_mbx_init(name, size, FIFO_Q)

DECLARE int rt_mbx_delete(MBX *mbx)
{
	void *arg = mbx;
	return rtai_lxrt(BIDX, SIZARG, MBX_DELETE, &arg).i[LOW];
}

DECLARE int rt_mbx_send(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { (MBX *)mbx, (char *)msg, msg_size };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_SEND, &arg).i[LOW];
}

DECLARE int rt_mbx_send_wp(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { (MBX *)mbx, (char *)msg, msg_size };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_SEND_WP, &arg).i[LOW];
}

DECLARE int rt_mbx_send_if(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { (MBX *)mbx, (char *)msg, msg_size };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_SEND_IF, &arg).i[LOW];
}

DECLARE int rt_mbx_send_until(MBX *mbx, void *msg, int msg_size, RTIME time)
{
	struct { MBX *mbx; char *msg; int msg_size; RTIME time; } arg = { (MBX *)mbx, (char *)msg, msg_size, time };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_SEND_UNTIL, &arg).i[LOW];
}

DECLARE int rt_mbx_send_timed(MBX *mbx, void *msg, int msg_size, RTIME delay)
{
	struct { MBX *mbx; char *msg; int msg_size; RTIME delay; } arg = { (MBX *)mbx, (char *)msg, msg_size, delay };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_SEND_TIMED, &arg).i[LOW];
}

DECLARE int rt_mbx_ovrwr_send(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { (MBX *)mbx, (char *)msg, msg_size };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_OVRWR_SEND, &arg).i[LOW];
}

DECLARE int rt_mbx_evdrp(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { (MBX *)mbx, (char *)msg, msg_size };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_EVDRP, &arg).i[LOW];
}

DECLARE int rt_mbx_receive(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { (MBX *)mbx, (char *)msg, msg_size };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_RECEIVE, &arg).i[LOW];
}

DECLARE int rt_mbx_receive_wp(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { (MBX *)mbx, (char *)msg, msg_size };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_RECEIVE_WP, &arg).i[LOW];
}

DECLARE int rt_mbx_receive_if(MBX *mbx, void *msg, int msg_size)
{
	struct { MBX *mbx; char *msg; int msg_size; } arg = { (MBX *)mbx, (char *)msg, msg_size };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_RECEIVE_IF, &arg).i[LOW];
}

DECLARE int rt_mbx_receive_until(MBX *mbx, void *msg, int msg_size, RTIME time)
{
	struct { MBX *mbx; void *msg; int msg_size; RTIME time; } arg = { (MBX *)mbx, (char *)msg, msg_size, time };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_RECEIVE_UNTIL, &arg).i[LOW];
}

DECLARE int rt_mbx_receive_timed(MBX *mbx, void *msg, int msg_size, RTIME delay)
{
	struct { MBX *mbx; char *msg; int msg_size; RTIME delay; } arg = { (MBX *)mbx, (char *)msg, msg_size, delay };
	return (int)rtai_lxrt(BIDX, SIZARG, MBX_RECEIVE_TIMED, &arg).i[LOW];
}

#define exist(name)  rt_get_adr(nam2num(name))

#define rt_named_task_init(task_name, thread, data, stack_size, prio, uses_fpu, signal) \
	rt_task_init(nam2num(task_name), thread, data, stack_size, prio, uses_fpu, signal)

#define rt_named_task_init_cpuid(task_name, thread, data, stack_size, prio, uses_fpu, signal, run_on_cpu) \
	rt_task_init_cpuid(nam2num(task_name), thread, data, stack_size, prio, uses_fpu, signal, run_on_cpu)

#define rt_typed_named_sem_init(sem_name, value, type) \
	rt_typed_sem_init(nam2num(sem_name), value, type)

#define rt_typed_named_mbx_init(mbx_name, size, qtype) \
	rt_typed_mbx_init(nam2num(mbx_name), size, qtype)

#define rt_named_sem_init(sem_name, value) \
	rt_typed_named_sem_init(sem_name, value, CNT_SEM)

#define rt_named_mbx_init(mbx_name, size) \
	rt_typed_named_mbx_init(mbx_name, size, FIFO_Q)

DECLARE void rt_set_runnable_on_cpus(RT_TASK *task, unsigned int cpu_mask)
{
	struct { RT_TASK *task; unsigned int cpu_mask; } arg = { task, cpu_mask };
	rtai_lxrt(BIDX, SIZARG, SET_RUNNABLE_ON_CPUS, &arg);
}

DECLARE void rt_set_runnable_on_cpuid(RT_TASK *task, unsigned int cpuid)
{
	struct { RT_TASK *task; unsigned int cpuid; } arg = { task, cpuid };
	rtai_lxrt(BIDX, SIZARG, SET_RUNNABLE_ON_CPUID, &arg);
}

DECLARE int rt_get_timer_cpu(void)
{
	struct { unsigned long dummy; } arg;
	return rtai_lxrt(BIDX, SIZARG, GET_TIMER_CPU, &arg).i[LOW];
}

DECLARE void start_rt_apic_timers(struct apic_timer_setup_data *setup_mode, unsigned int rcvr_jiffies_cpuid)
{
	struct { struct apic_timer_setup_data *setup_mode; unsigned int rcvr_jiffies_cpuid; } arg = { setup_mode, rcvr_jiffies_cpuid };
	rtai_lxrt(BIDX, SIZARG, START_RT_APIC_TIMERS, &arg);
}

DECLARE void rt_preempt_always_cpuid(int yes_no, unsigned int cpuid)
{
	struct { int yes_no; unsigned int cpuid; } arg = { yes_no, cpuid };
	rtai_lxrt(BIDX, SIZARG, PREEMPT_ALWAYS_CPUID, &arg);
}

DECLARE RTIME count2nano_cpuid(RTIME count, unsigned int cpuid)
{
	struct { RTIME count; unsigned int cpuid; } arg = { count, cpuid };
	return rtai_lxrt(BIDX, SIZARG, COUNT2NANO_CPUID, &arg).rt;
}

DECLARE RTIME nano2count_cpuid(RTIME nanos, unsigned int cpuid)
{
	struct { RTIME nanos; unsigned int cpuid; } arg = { nanos, cpuid };
	return rtai_lxrt(BIDX, SIZARG, NANO2COUNT_CPUID, &arg).rt;
}

DECLARE RTIME rt_get_time_cpuid(unsigned int cpuid)
{
	struct { unsigned int cpuid; } arg = { cpuid };
	return rtai_lxrt(BIDX, SIZARG, GET_TIME_CPUID, &arg).rt;
}

DECLARE RTIME rt_get_time_ns_cpuid(unsigned int cpuid)
{
	struct { unsigned int cpuid; } arg = { cpuid };
	return rtai_lxrt(BIDX, SIZARG, GET_TIME_NS_CPUID, &arg).rt;
}

DECLARE int rt_Send(pid_t pid, void *smsg, void *rmsg, size_t ssize, size_t rsize )
{
	struct { pid_t pid; void *smsg; void *rmsg; size_t ssize, rsize;} arg = { pid, smsg, rmsg, ssize, rsize };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_SEND, &arg).i[LOW];
}

DECLARE pid_t rt_Receive(pid_t pid, void *msg, size_t maxsize, size_t *msglen)
{
	struct { pid_t pid; void *msg; size_t maxsize, *msglen;}
	arg = { pid, msg, maxsize, msglen };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_RECEIVE, &arg).i[LOW];
}

DECLARE pid_t rt_Creceive(pid_t pid, void *msg, size_t maxsize, size_t *msglen, RTIME delay)
{
	struct { pid_t pid; void *msg; size_t maxsize, *msglen; RTIME delay;} arg = { pid, msg, maxsize, msglen, delay };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_CRECEIVE, &arg).i[LOW];
}

DECLARE pid_t rt_Reply(pid_t pid, void *msg, size_t size)
{
	struct { pid_t pid; void *msg; size_t size;} arg = { pid, msg, size };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_REPLY, &arg).i[LOW];
}

DECLARE pid_t rt_Proxy_attach(pid_t pid, void *msg, int nbytes, int priority)
{
	struct { pid_t pid; void *msg; int nbytes, priority;} arg = { pid, msg, nbytes, priority };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_PROXY_ATTACH, &arg).i[LOW];
}

DECLARE pid_t rt_Proxy_detach(pid_t pid)
{
	struct { pid_t pid; } arg = { pid };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_PROXY_DETACH, &arg).i[LOW];
}

DECLARE pid_t rt_Trigger(pid_t pid)
{
	struct { pid_t pid; } arg = { pid };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_TRIGGER, &arg).i[LOW];
}

DECLARE pid_t rt_Alias_attach(const char *name)
{
	struct { const char *name; } arg = { name};
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_NAME_ATTACH, &arg).i[LOW];
}

DECLARE pid_t rt_Name_locate(const char *host, const char *name)
{
	struct { const char *host, *name; } arg = { host, name };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_NAME_LOCATE, &arg).i[LOW];
}

DECLARE int rt_Name_detach(pid_t pid)
{
	struct { pid_t pid; } arg = { pid };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_NAME_DETACH, &arg).i[LOW];
}

DECLARE int rt_InitTickQueue(void)
{
	struct { unsigned long dummy; } arg;
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_INITTICKQUEUE, &arg).i[LOW];
}

DECLARE void rt_ReleaseTickQueue(void)
{
	struct { unsigned long dummy; } arg;
	rtai_lxrt(BIDX, SIZARG, RT_RELEASETICKQUEUE, &arg);
}

DECLARE unsigned rt_qDynAlloc(unsigned n)
{
	struct { unsigned n; } arg = { n };
	return (unsigned) rtai_lxrt(BIDX, SIZARG, RT_QDYNALLOC, &arg).i[LOW];
} 

DECLARE unsigned rt_qDynFree(int n)
{
	struct { unsigned n; } arg = { n };
	return (unsigned) rtai_lxrt(BIDX, SIZARG, RT_QDYNFREE, &arg).i[LOW];
}

DECLARE QBLK *rt_qDynInit(QBLK **q, void (*fun)(void *, int), void *data, int evn )
{
	QBLK *r;

	struct { QBLK **q; void (*fun)(void *, int), *data; int evn; } arg = { 0, fun, data, evn };
	r  = (QBLK *) rtai_lxrt(BIDX, SIZARG, RT_QDYNINIT, &arg).v[LOW];
	if (q) *q = r;
	return r;
}

DECLARE void rt_qBlkWait(QBLK *q, RTIME t)
{
	struct { QBLK *q; RTIME t; } arg = { q, t } ;
	rtai_lxrt(BIDX, SIZARG, RT_QBLKWAIT, &arg);
}

DECLARE void rt_qBlkRepeat(QBLK *q, RTIME t)
{
	struct { QBLK *q; RTIME t; } arg = { q, t } ;
	rtai_lxrt(BIDX, SIZARG, RT_QBLKREPEAT, &arg);
}

DECLARE void rt_qBlkSoon(QBLK *q)
{
	struct { QBLK *q; } arg = { q };
	rtai_lxrt(BIDX, SIZARG, RT_QBLKSOON, &arg);
}

DECLARE void rt_qBlkDequeue(QBLK *q)
{
	struct { QBLK *q; } arg = { q };
	rtai_lxrt(BIDX, SIZARG, RT_QBLKDEQUEUE, &arg);
}

DECLARE void rt_qBlkCancel(QBLK *q)
{
	struct { QBLK *q; } arg = { q };
	rtai_lxrt(BIDX, SIZARG, RT_QBLKCANCEL, &arg);
}

DECLARE void rt_qBlkBefore(QBLK *cur, QBLK *nxt)
{
	struct { QBLK *cur, *nxt; } arg = { cur, nxt };
	rtai_lxrt(BIDX, SIZARG, RT_QBLKBEFORE, &arg);
}

DECLARE void rt_qBlkAfter(QBLK *cur, QBLK *prv)
{
	struct { QBLK *cur, *prv; } arg = { cur, prv };
	rtai_lxrt(BIDX, SIZARG, RT_QBLKAFTER, &arg);
}

DECLARE QBLK *rt_qBlkUnhook(QBLK *q)
{
	struct { QBLK *q; } arg = { q };
	return (QBLK *) rtai_lxrt(BIDX, SIZARG, RT_QBLKUNHOOK, &arg).v[LOW];
}

DECLARE void rt_qBlkRelease(QBLK *q)
{
	struct { QBLK *q; } arg = { q };
	rtai_lxrt(BIDX, SIZARG, RT_QBLKRELEASE, &arg);
}

DECLARE void rt_qBlkComplete(QBLK *q)
{
	struct { QBLK *q; } arg = { q };
	rtai_lxrt(BIDX, SIZARG, RT_QBLKCOMPLETE, &arg);
}

DECLARE int rt_qSync(void)
{
	struct { unsigned long dummy; } arg;
	return rtai_lxrt(BIDX, SIZARG, RT_QSYNC, &arg).i[LOW];
}

DECLARE pid_t rt_qReceive(pid_t target, void *buf, size_t maxlen, size_t *msglen)
{
	struct {pid_t target; void *buf; size_t maxlen, *msglen; } arg = { target, buf, maxlen, msglen };
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_QRECEIVE, &arg).i[LOW];
}

DECLARE void rt_qLoop(void)
{
	struct { unsigned long dummy; } arg;
	rtai_lxrt(BIDX, SIZARG, RT_QLOOP, &arg);
}

DECLARE RTIME rt_qStep(void)
{
	struct { unsigned long dummy; } arg;
	return rtai_lxrt(BIDX, SIZARG, RT_QSTEP, &arg).rt;
}

DECLARE void rt_qHookFlush(QHOOK *h)
{
	struct { QHOOK *h; } arg = { h };
	rtai_lxrt(BIDX, SIZARG, RT_QHOOKFLUSH, &arg);
}

DECLARE void rt_qBlkAtHead(QBLK *q, QHOOK *h)
{
	struct { QBLK *q; QHOOK *h; } arg = { q, h };
	rtai_lxrt(BIDX, SIZARG, RT_QBLKATHEAD, &arg);
}

DECLARE void rt_qBlkAtTail(QBLK *q, QHOOK *h)
{
	struct { QBLK *q; QHOOK *h; } arg = { q, h };
	rtai_lxrt(BIDX, SIZARG, RT_QBLKATTAIL, &arg);
}

DECLARE QHOOK *rt_qHookInit(QHOOK **h, void (*c)(void *, QBLK *), void *a)
{
	QHOOK *r;
	struct { QHOOK **h; void (*c)(void *, QBLK *), *a;} arg = { 0, c, a };
	r = (QHOOK *) rtai_lxrt(BIDX, SIZARG, RT_QHOOKINIT, &arg).v[LOW];
	if (h) *h = r;
	return r;
}

DECLARE void rt_qHookRelease(QHOOK *h)
{
	struct { QHOOK *h; } arg = { h };
	rtai_lxrt(BIDX, SIZARG, RT_QHOOKRELEASE, &arg);
}

DECLARE void rt_qBlkSchedule(QBLK *q, RTIME t)
{
	struct { QBLK *q; RTIME t; } arg = { q, t } ;
	rtai_lxrt(BIDX, SIZARG, RT_QBLKSCHEDULE, &arg);
}

DECLARE QHOOK *rt_GetTickQueueHook(void)
{
	struct { unsigned long dummy; } arg;
	return (QHOOK *) rtai_lxrt(BIDX, SIZARG, RT_GETTICKQUEUEHOOK, &arg).v[LOW];
}

DECLARE pid_t rt_vc_reserve( void )
{
	struct { unsigned long dummy; } arg;
	return (pid_t) rtai_lxrt(BIDX, SIZARG, RT_VC_RESERVE, &arg).i[LOW];
}

DECLARE int rt_vc_attach(pid_t pid)
{
	struct { pid_t pid; } arg = { pid };
	return rtai_lxrt(BIDX, SIZARG, RT_VC_ATTACH, &arg).i[LOW];
}

DECLARE int rt_vc_release(pid_t pid)
{
	struct { pid_t pid; } arg = { pid };
	return rtai_lxrt(BIDX, SIZARG, RT_VC_RELEASE, &arg).i[LOW];
}

DECLARE void rt_boom(void)
{
	struct { int dummy; } arg = { 0 };
	rtai_lxrt(BIDX, SIZARG, RT_BOOM, &arg);
}

#ifdef _NO_US_MEMORY
DECLARE void *rt_malloc(unsigned int n)
{
	struct { unsigned int val; } arg = { n };
	return rtai_lxrt(BIDX, SIZARG, RTAI_MALLOC, &arg).v[LOW];
}

DECLARE void rt_free(void *v)
{
	struct { void *val; } arg = { v };
	rtai_lxrt(BIDX, SIZARG, RT_FREE, &arg);
}
#endif

DECLARE void rt_mmgr_stats(void)
{
	struct { int dummy; } arg = { 0 };
	rtai_lxrt(BIDX, SIZARG, RT_MMGR_STATS, &arg);
}

DECLARE void rt_stomp(void) 
{
	struct { int dummy; } arg = { 0 };
	rtai_lxrt(BIDX, SIZARG, RT_STOMP, &arg);
}

DECLARE int rt_get_linux_signal(RT_TASK *task)
{
    struct { RT_TASK *task; } arg = { task };
    return rtai_lxrt(BIDX, SIZARG, RT_GET_LINUX_SIGNAL, &arg).i[LOW];
}

DECLARE int rt_get_errno(RT_TASK *task)
{
    struct { RT_TASK *task; } arg = { task };
    return rtai_lxrt(BIDX, SIZARG, RT_GET_ERRNO, &arg).i[LOW];
}

DECLARE int rt_set_linux_signal_handler(RT_TASK *task, void (*handler)(int sig))
{
    struct { RT_TASK *task; void (*handler)(int sig); } arg = { task, handler };
    return rtai_lxrt(BIDX, SIZARG, RT_SET_LINUX_SIGNAL_HANDLER, &arg).i[LOW];
}

DECLARE int rt_lxrt_fork(int is_a_clone)
{
    struct { int is_a_clone; } arg = { is_a_clone };
    return rtai_lxrt(BIDX, SIZARG, LXRT_FORK, &arg).i[LOW];
}

// rtai_print_to_screen() was only static, not static inline ?
DECLARE int rtai_print_to_screen(const char *format, ...)
{
	char display[256];
	struct { const char *display; int nch; } arg = { display, 0 };
	va_list args;

	va_start(args, format);
	arg.nch = vsprintf(display, format, args);
	va_end(args);
	rtai_lxrt(BIDX, SIZARG, PRINT_TO_SCREEN, &arg);
	return arg.nch;
/*
	static char display[256];
	va_list args;
	static struct { char *display; int len; } arg; // _must_ be static!
	int len;

	va_start(args, format);
	arg.len = vsprintf(display, format, args);
	va_end(args);
	arg.display = (char *) display;
	len = rtai_lxrt(BIDX, SIZARG, PRINT_TO_SCREEN, &arg).i[LOW];
	return len;
*/
}

DECLARE int rt_printk(const char *format, ...)
{
	char display[256];
	struct { const char *display; int nch; } arg = { display, 0 };
	va_list args;

	va_start(args, format);
	arg.nch = vsprintf(display, format, args);
	va_end(args);
	rtai_lxrt(BIDX, SIZARG, PRINTK, &arg);
	return arg.nch;
}


DECLARE int rt_usp_signal_handler(void (*handler)(void))
{
	struct { void (*handler)(void); } arg = { handler };
	return rtai_lxrt(BIDX, SIZARG, USP_SIGHDL, &arg).i[0];
}

DECLARE unsigned long rt_get_usp_flags(RT_TASK *rt_task)
{
	struct { RT_TASK *task; } arg = { rt_task };
	return rtai_lxrt(BIDX, SIZARG, GET_USP_FLAGS, &arg).i[LOW];
}

DECLARE unsigned long rt_get_usp_flags_mask(RT_TASK *rt_task)
{
	struct { RT_TASK *task; } arg = { rt_task };
	return rtai_lxrt(BIDX, SIZARG, GET_USP_FLG_MSK, &arg).i[LOW];
}

DECLARE void rt_set_usp_flags(RT_TASK *rt_task, unsigned long flags)
{
	struct { RT_TASK *task; unsigned long flags; } arg = { rt_task, flags };
	rtai_lxrt(BIDX, SIZARG, SET_USP_FLAGS, &arg);
}

DECLARE void rt_set_usp_flags_mask(unsigned long flags_mask)
{
	struct { unsigned long flags_mask; } arg = { flags_mask };
	rtai_lxrt(BIDX, SIZARG, SET_USP_FLG_MSK, &arg);
}

DECLARE RT_TASK *rt_force_task_soft(int pid)
{
	struct { int pid; } arg = { pid };
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, FORCE_TASK_SOFT, &arg).v[LOW];
}

DECLARE RT_TASK *rt_agent(void)
{
	struct { unsigned long dummy; } arg;
	return (RT_TASK *)rtai_lxrt(BIDX, SIZARG, RT_BUDDY, &arg).v[LOW];
}

#ifndef rt_buddy
#define rt_buddy rt_agent
#endif

#ifdef HARD_LXRT
DECLARE void rt_make_hard_real_time(void)
{
	struct { unsigned long dummy; } arg;
	rtai_lxrt(BIDX, SIZARG, MAKE_HARD_RT, &arg);
}

DECLARE void rt_allow_nonroot_hrt(void)
{
	struct { unsigned long dummy; } arg;
	rtai_lxrt(BIDX, SIZARG, NONROOT_HRT, &arg);
}

DECLARE int rt_is_hard_real_time(RT_TASK *rt_task)
{
	struct { RT_TASK *task; } arg = { rt_task };
	return rtai_lxrt(BIDX, SIZARG, IS_HARD, &arg).i[LOW];
}

#define rt_is_soft_real_time(rt_task) (!rt_is_hard_real_time((rt_task)))

DECLARE void rt_task_set_resume_end_times(RTIME resume, RTIME end)
{
	struct { RTIME resume, end; } arg = { resume, end };
	rtai_lxrt(BIDX, SIZARG, SET_RESUME_END, &arg);
}

DECLARE int rt_set_resume_time(RT_TASK *rt_task, RTIME new_resume_time)
{
	struct { RT_TASK *rt_task; RTIME new_resume_time; } arg = { rt_task, new_resume_time };
	return rtai_lxrt(BIDX, SIZARG, SET_RESUME_TIME, &arg).i[LOW];
}

DECLARE int rt_set_period(RT_TASK *rt_task, RTIME new_period)
{
	struct { RT_TASK *rt_task; RTIME new_period; } arg = { rt_task, new_period };
	return rtai_lxrt(BIDX, SIZARG, SET_PERIOD, &arg).i[LOW];
}

DECLARE void rt_spv_RMS(int cpuid)
{
	struct { int cpuid; } arg = { cpuid };
	rtai_lxrt(BIDX, SIZARG, SPV_RMS, &arg);
}

DECLARE int rt_task_wakeup_sleeping(RT_TASK *task)
{
	struct { RT_TASK *task; } arg = { task };
	return rtai_lxrt(BIDX, SIZARG, WAKEUP_SLEEPING, &arg).i[LOW];
}

DECLARE void rt_get_exectime(RT_TASK *task, RTIME *exectime)
{
	RTIME lexectime[] = { 0LL, 0LL, 0LL };
	struct { RT_TASK *task; RTIME *lexectime; } arg = { task, lexectime };
	rtai_lxrt(BIDX, SIZARG, GET_EXECTIME, &arg);
	memcpy(exectime, lexectime, sizeof(lexectime));
}

DECLARE void rt_gettimeorig(RTIME time_orig[])
{
	struct { RTIME *time_orig; } arg = { time_orig };
	rtai_lxrt(BIDX, SIZARG, GET_TIMEORIG, &arg);
}

#else
#define rt_make_soft_real_time()
#define rt_make_hard_real_time()
#define rt_allow_nonroot_hrt()
#define rt_is_hard_real_time( x ) 0
#define rt_is_soft_real_time( x ) 1
#endif // HARD_LXRT 

DECLARE int rt_sem_count(SEM *sem)
{
	struct { SEM *sem; } arg = { sem };
	return rtai_lxrt(BIDX, SIZARG, SEM_COUNT, &arg).i[LOW];
}

#define rt_cond_init(cnd)                  rt_typed_sem_init(cnd, 0, BIN_SEM)
#define rt_cond_delete(cnd)                rt_sem_delete(cnd)
#define rt_cond_destroy(cnd)               rt_sem_delete(cnd)
#define rt_cond_broadcast(cnd)             rt_sem_broadcast(cnd)
#define rt_cond_signal(cnd)                rt_sem_signal(cnd)
#define rt_cond_timedwait(cnd, mtx, time)  rt_cond_wait_until(cnd, mtx, time)

DECLARE int rt_cond_wait(CND *cond, SEM *mutex)
{
	struct { CND *cond; SEM *mutex; } arg = { cond, mutex };
	return rtai_lxrt(BIDX, SIZARG, COND_WAIT, &arg).i[LOW];
}

DECLARE int rt_cond_wait_until(CND *cond, SEM *mutex, RTIME time)
{
	struct { CND *cond; SEM *mutex; RTIME time; } arg = { cond, mutex, time };
	return rtai_lxrt(BIDX, SIZARG, COND_WAIT_UNTIL, &arg).i[LOW];
}

DECLARE int rt_cond_wait_timed(CND *cond, SEM *mutex, RTIME delay)
{
	struct { CND *cond; SEM *mutex; RTIME delay; } arg = { cond, mutex, delay };
	return rtai_lxrt(BIDX, SIZARG, COND_WAIT_TIMED, &arg).i[LOW];
}

DECLARE RWL *rt_rwl_init(unsigned long name)
{
        struct { unsigned long name; } arg = { name };
       	return (RWL *)rtai_lxrt(BIDX, SIZARG, RWL_INIT, &arg).v[LOW];
}

DECLARE int rt_rwl_delete(RWL *rwl)
{
	struct { RWL *rwl; } arg = { rwl };
	return rtai_lxrt(BIDX, SIZARG, RWL_DELETE, &arg).i[LOW];
}

DECLARE int rt_rwl_rdlock(RWL *rwl)
{
	struct { RWL *rwl; } arg = { rwl };
	return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK, &arg).i[LOW];
}

DECLARE int rt_rwl_rdlock_if(RWL *rwl)
{
	struct { void *rwl; } arg = { rwl };
	return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK_IF, &arg).i[LOW];
}

DECLARE int rt_rwl_rdlock_until(RWL *rwl, RTIME time)
{
	struct { RWL *rwl; RTIME time; } arg = { rwl, time };
	return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK_UNTIL, &arg).i[LOW];
}

DECLARE int rt_rwl_rdlock_timed(RWL *rwl, RTIME delay)
{
	struct { RWL *rwl; RTIME delay; } arg = { rwl, delay };
	return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK_TIMED, &arg).i[LOW];
}

DECLARE int rt_rwl_wrlock(RWL *rwl)
{
	struct { RWL *rwl; } arg = { rwl };
	return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK, &arg).i[LOW];
}

DECLARE int rt_rwl_wrlock_if(RWL *rwl)
{
	struct { RWL *rwl; } arg = { rwl };
	return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK_IF, &arg).i[LOW];
}

DECLARE int rt_rwl_wrlock_until(RWL *rwl, RTIME time)
{
	struct { RWL *rwl; RTIME time; } arg = { rwl, time };
	return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK_UNTIL, &arg).i[LOW];
}

DECLARE int rt_rwl_wrlock_timed(RWL *rwl, RTIME delay)
{
	struct { RWL *rwl; RTIME delay; } arg = { rwl, delay };
	return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK_TIMED, &arg).i[LOW];
}

DECLARE int rt_rwl_unlock(RWL *rwl)
{
	struct { RWL *rwl; } arg = { rwl };
	return rtai_lxrt(BIDX, SIZARG, RWL_UNLOCK, &arg).i[LOW];
}

DECLARE SPL *rt_spl_init(unsigned long name)
{
	struct { unsigned long name; } arg = { name };
	return (SPL *)rtai_lxrt(BIDX, SIZARG, SPL_INIT, &arg).v[LOW];
}

DECLARE int rt_spl_delete(SPL *spl)
{
	struct { SPL *spl; } arg = { spl };
	return rtai_lxrt(BIDX, SIZARG, SPL_DELETE, &arg).i[LOW];
}

DECLARE int rt_spl_lock(SPL *spl)
{
	struct { SPL *spl; } arg = { spl };
	return rtai_lxrt(BIDX, SIZARG, SPL_LOCK, &arg).i[LOW];
}

DECLARE int rt_spl_lock_if(SPL *spl)
{
	struct { SPL *spl; } arg = { spl };
	return rtai_lxrt(BIDX, SIZARG, SPL_LOCK_IF, &arg).i[LOW];
}

DECLARE int rt_spl_lock_timed(SPL *spl, RTIME delay)
{
	struct { SPL *spl; RTIME delay; } arg = { spl, delay };
	return rtai_lxrt(BIDX, SIZARG, SPL_LOCK_TIMED, &arg).i[LOW];
}

DECLARE int rt_spl_unlock(SPL *spl)
{
	struct { SPL *spl; } arg = { spl };
	return rtai_lxrt(BIDX, SIZARG, SPL_UNLOCK, &arg).i[LOW];
}

#endif /* __KERNEL__ */
#endif // _RTAI_LXRT_H_
