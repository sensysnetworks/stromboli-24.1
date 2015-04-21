
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

$Id: rtai_lxrt_user.h,v 1.1.1.1 2004/06/06 14:01:46 rpm Exp $ 
*/

/*
 * Defines required by users who need not be familiar with kernel internals. 
*/

#ifndef _RTAI_LXRT_USER_H_
#define _RTAI_LXRT_USER_H_

#include <rtai_types.h>
#include <rtai_declare.h>

/*
 This file allows to include rtai_lxrt.h without any other h files
 from the kernel. This should make it easier to compile C++ GUI applications
 that use lxrt.

 A user space program cannot access the data structure of a RT_TASK pointer
 for example. These pointers are more like handles and defining them
 as pointers to void in user space does not change anything.   
*/ 

#define __KERNEL_CS        0x10
#define __KERNEL_DS        0x18

//typedef void *    RT_TASK;
typedef void *    SEM;
typedef void *    MBX;
typedef void *    RWL;
typedef void *    SPL;
typedef void      QBLK;
typedef void      QHOOK;
typedef SEM       CND;

struct apic_timer_setup_data { int mode, count; };

extern int lock_all(int stacksize, int heapsize);
extern void init_linux_scheduler(int sched, int pri);

#define PRIO_Q    0
#define FIFO_Q    4

#define BIN_SEM   1
#define CNT_SEM   2
#define RES_SEM   3

#define RT_SCHED_FIFO  0
#define RT_SCHED_RR    1

#define SEM_TIMOUT (0xFffe)

#ifndef KEEP_STATIC_INLINE

extern unsigned long nam2num(const char *name);
extern void num2nam(unsigned long num, char *name);

extern void *rt_get_adr(unsigned long name);
extern unsigned long rt_get_name(void *adr);

#ifdef __CPP_KERNEL__ // C++ support
  extern int rt_task_init(RT_TASK *task, void (*rt_thread)(int), int data, int stack_size, int priority, int uses_fpu, void(*signal)(void));
  extern RT_TASK *rt_alloc_dynamic_task(void);
  extern int rt_printk( const char *fmt, ...);
#else 
  extern RT_TASK *rt_task_init(int name, int priority, int stack_size, int max_msg_size);
  extern RT_TASK *rt_task_init_schmod(int name, int priority, int stack_size, int max_msg_size, int policy, int cpus_allowed);
#endif

extern int rt_task_delete(RT_TASK *task);
extern int rt_task_yield(void);
extern int rt_task_suspend(RT_TASK *task);
extern int rt_task_resume(RT_TASK *task);
extern int rt_task_make_periodic(RT_TASK *task, RTIME start_time, RTIME period);
extern int rt_task_make_periodic_relative_ns(RT_TASK *task, RTIME start_delay, RTIME period);
extern void rt_task_wait_period(void);
extern void rt_sleep(RTIME delay);
extern void rt_sleep_until(RTIME time);
extern RTIME start_rt_timer(int period);
extern void stop_rt_timer(void);
extern RTIME rt_get_time(void);
extern RTIME count2nano(RTIME count);
extern RTIME nano2count(RTIME nanos);
extern SEM *rt_typed_sem_init(int name, int value, int type);
#define rt_sem_init(name, value) rt_typed_sem_init(name, value, CNT_SEM)
extern int rt_sem_delete(SEM *sem);
extern int rt_sem_signal(SEM *sem);
extern int rt_sem_wait(SEM *sem);
extern int rt_sem_wait_if(SEM *sem);
extern int rt_sem_wait_until(SEM *sem, RTIME time);
extern int rt_sem_wait_timed(SEM *sem, RTIME delay);
extern void rt_busy_sleep(int ns);
extern RT_TASK *rt_send(RT_TASK *task, unsigned int msg);
extern RT_TASK *rt_send_if(RT_TASK *task, unsigned int msg);
extern RT_TASK *rt_send_until(RT_TASK *task, unsigned int msg, RTIME time);
extern RT_TASK *rt_send_timed(RT_TASK *task, unsigned int msg, RTIME delay);
extern RT_TASK *rt_receive(RT_TASK *task, unsigned int *msg);
extern RT_TASK *rt_receive_if(RT_TASK *task, unsigned int *msg);
extern RT_TASK *rt_receive_until(RT_TASK *task, unsigned int *msg, RTIME time);
extern RT_TASK *rt_receive_timed(RT_TASK *task, unsigned int *msg, RTIME delay);
extern RT_TASK *rt_rpc(RT_TASK *task, unsigned int to_do, unsigned int *result);
extern RT_TASK *rt_rpc_if(RT_TASK *task, unsigned int to_do, unsigned int *result);
extern RT_TASK *rt_rpc_until(RT_TASK *task, unsigned int to_do, unsigned int *result, RTIME time);
extern RT_TASK *rt_rpc_timed(RT_TASK *task, unsigned int to_do, unsigned int *result, RTIME delay);
extern int rt_isrpc(RT_TASK *task);
extern RT_TASK *rt_return(RT_TASK *task, unsigned int result);
extern void rt_set_periodic_mode(void);
extern void rt_set_oneshot_mode(void);
extern int rt_task_signal_handler(RT_TASK *task, void (*handler)(void));
extern int rt_task_use_fpu(RT_TASK *task, int use_fpu_flag);
extern int rt_linux_use_fpu(int use_fpu_flag);
extern void rt_preempt_always(int yes_no);
extern RTIME rt_get_time_ns(void);
extern RTIME rt_get_cpu_time_ns(void);
extern void *rt_typed_mbx_init(int name, int size, int qtype);
#define rt_mbx_init(name, size) rt_typed_mbx_init(name, size, FIFO_Q)
extern int rt_mbx_delete(MBX *mbx);
extern int rt_mbx_send(MBX *mbx, void *msg, int msg_size);
extern int rt_mbx_send_wp(MBX *mbx, void *msg, int msg_size);
extern int rt_mbx_send_if(MBX *mbx, void *msg, int msg_size);
extern int rt_mbx_send_until(MBX *mbx, void *msg, int msg_size, RTIME time);
extern int rt_mbx_send_timed(MBX *mbx, void *msg, int msg_size, RTIME delay);
extern int rt_mbx_receive(MBX *mbx, void *msg, int msg_size);
extern int rt_mbx_receive_wp(MBX *mbx, void *msg, int msg_size);
extern int rt_mbx_receive_if(MBX *mbx, void *msg, int msg_size);
extern int rt_mbx_receive_until(MBX *mbx, void *msg, int msg_size, RTIME time);
extern int rt_mbx_receive_timed(MBX *mbx, void *msg, int msg_size, RTIME delay);
extern void rt_set_runnable_on_cpus(RT_TASK *task, unsigned int cpu_mask);
extern void rt_set_runnable_on_cpuid(RT_TASK *task, unsigned int cpuid);
extern int rt_get_timer_cpu(void);
extern void start_rt_apic_timers(struct apic_timer_setup_data *setup_mode, unsigned int rcvr_jiffies_cpuid);
extern void rt_preempt_always_cpuid(int yes_no, unsigned int cpuid);
extern RTIME count2nano_cpuid(RTIME count, unsigned int cpuid);
extern RTIME nano2count_cpuid(RTIME nanos, unsigned int cpuid);
extern RTIME rt_get_time_cpuid(unsigned int cpuid);
extern RTIME rt_get_time_ns_cpuid(unsigned int cpuid);
extern int rt_Send(pid_t pid, void *smsg, void *rmsg, size_t ssize, size_t rsize );
extern pid_t rt_Receive(pid_t pid, void *msg, size_t maxsize, size_t *msglen);
extern pid_t rt_Creceive(pid_t pid, void *msg, size_t maxsize, size_t *msglen, RTIME delay);
extern pid_t rt_Reply(pid_t pid, void *msg, size_t size);
extern pid_t rt_Proxy_attach(pid_t pid, void *msg, int nbytes, int priority);
extern pid_t rt_Proxy_detach(pid_t pid);
extern pid_t rt_Trigger(pid_t pid);
extern pid_t rt_Alias_attach(const char *name);
extern pid_t rt_Name_locate(const char *host, const char *name);
extern int rt_Name_detach(pid_t pid);
extern int rt_InitTickQueue(void);
extern void rt_ReleaseTickQueue(void);
extern unsigned rt_qDynAlloc(unsigned n);
extern unsigned rt_qDynFree(int n);
extern QBLK *rt_qDynInit(QBLK **q, void (*fun)(void *, int), void *data, int evn );
extern void rt_qBlkWait(QBLK *q, RTIME t);
extern void rt_qBlkRepeat(QBLK *q, RTIME t);
extern void rt_qBlkSoon(QBLK *q);
extern void rt_qBlkDequeue(QBLK *q);
extern void rt_qBlkCancel(QBLK *q);
extern void rt_qBlkBefore(QBLK *cur, QBLK *nxt);
extern void rt_qBlkAfter(QBLK *cur, QBLK *prv);
extern QBLK *rt_qBlkUnhook(QBLK *q);
extern void rt_qBlkRelease(QBLK *q);
extern void rt_qBlkComplete(QBLK *q);
extern int rt_qSync(void);
extern pid_t rt_qReceive(pid_t target, void *buf, size_t maxlen, size_t *msglen);
extern void rt_qLoop(void);
extern RTIME rt_qStep(void);
extern void rt_qHookFlush(QHOOK *h);
extern void rt_qBlkAtHead(QBLK *q, QHOOK *h);
extern void rt_qBlkAtTail(QBLK *q, QHOOK *h);
extern QHOOK *rt_qHookInit(QHOOK **h, void (*c)(void *, QBLK *), void *a);
extern void rt_qHookRelease(QHOOK *h);
extern void rt_qBlkSchedule(QBLK *q, RTIME t);
extern QHOOK *rt_GetTickQueueHook(void);
extern pid_t rt_vc_reserve( void );
extern int rt_vc_attach(pid_t pid);
extern int rt_vc_release(pid_t pid);

extern void *rt_malloc(unsigned int n);
extern void rt_free(void *v);
extern void rt_mmgr_stats(void);

extern RT_TASK *rt_agent(void);

#ifndef rt_buddy
#define rt_buddy rt_agent
#endif

extern int rtai_print_to_screen(const char *format, ...);
extern void rt_make_soft_real_time(void);
extern void rt_make_hard_real_time(void);
extern void rt_allow_nonroot_hrt(void);

extern int rt_is_hard_real_time(RT_TASK *rt_task);

extern void rt_task_set_resume_end_times(RTIME resume, RTIME end);

extern int rt_set_resume_time(RT_TASK *rt_task, RTIME new_resume_time);
extern int rt_set_period(RT_TASK *rt_task, RTIME new_period);
extern void rt_spv_RMS(int cpuid);
extern int rt_task_resume_sleep(RT_TASK *task);
extern void rt_get_exectime(RT_TASK *task, RTIME *exectime);                   

extern int rt_get_linux_signal(RT_TASK *task);
extern int rt_get_errno(RT_TASK *task);
extern int rt_lxrt_fork(int is_a_clone);

#endif // #ifndef KEEP_STATIC_INLINE
#endif // _RTAI_LXRT_USER_H_

