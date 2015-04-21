/*
 * Project: rtai_cpp - RTAI C++ Framework 
 *
 * File: $Id: rtai_wrapper.h,v 1.1.1.1 2004/06/06 14:03:07 rpm Exp $
 *
 * Copyright: (C) 2001,2002 Erwin Rol <erwin@muffin.org>
 *
 * Licence:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <config.h>
#include "linux_wrapper.h"

#ifndef __RTAI_WRAPPER_H__
#define __RTAI_WRAPPER_H__

#ifdef __cplusplus
extern "C"  {
#endif

#include <rtai_types.h>
#include <linux/time.h>
#include <config.h>

#ifdef CONFIG_RTAI_ADEOS
#define rt_printk printk
#endif

// in LXRT a RT_TASK is a void
// while in RTAI it is the real structure
// we use a define that we undef in 
// rtai_wrapper.c so it won't colide with
// the typedef. Yes! this all is very ugly
// but untill a total cleanup of all the
// include files it is the best way to do it

#define RT_TASK struct rt_task_struct
#define SEM struct rt_semaphore
#define MBX struct rt_mailbox
#define CND SEM
#define TBX struct rt_typed_mailbox

struct rt_typed_mailbox;


#define ALL_SET               0
#define ANY_SET               1
#define ALL_CLR               2
#define ANY_CLR               3

#define ALL_SET_AND_ANY_SET   4
#define ALL_SET_AND_ALL_CLR   5
#define ALL_SET_AND_ANY_CLR   6
#define ANY_SET_AND_ALL_CLR   7
#define ANY_SET_AND_ANY_CLR   8
#define ALL_CLR_AND_ANY_CLR   9

#define ALL_SET_OR_ANY_SET   10
#define ALL_SET_OR_ALL_CLR   11
#define ALL_SET_OR_ANY_CLR   12
#define ANY_SET_OR_ALL_CLR   13
#define ANY_SET_OR_ANY_CLR   14
#define ALL_CLR_OR_ANY_CLR   15

#define SET_BITS              0
#define CLR_BITS              1
#define SET_CLR_BITS          2
#define NOP_BITS              3

#define BITS_ERR     0xFfff  // same as semaphores
#define BITS_TIMOUT  0xFffe  // same as semaphores

struct rt_bits_struct;
#define BITS struct rt_bits_struct

#ifdef CONFIG_RTAI_ADEOS
#define rt_printk printk
#endif

extern int rt_printk(const char *format, ... );

extern int cpp_key;       

#ifdef CONFIG_SMP
#define RTAI_NR_CPUS CONFIG_RTAI_CPUS
#else
#define RTAI_NR_CPUS 1
#endif

extern void __rt_get_global_lock(void);
extern void __rt_release_global_lock(void);

extern int __hard_cpu_id(void);
extern int rt_assign_irq_to_cpu(int irq, unsigned long cpus_mask);
extern int rt_reset_irq_to_sym_mode(int irq);


extern long long nano2count(long long nano);
extern long long count2nano(long long count);

extern long long rt_get_time(void); 
extern long long rt_get_time_ns(void); 
extern long long rt_get_cpu_time_ns(void);
extern void rt_set_oneshot_mode(void);
extern void rt_set_periodic_mode(void);
extern long long start_rt_timer(int period);
extern void stop_rt_timer(void);
extern int rt_is_hard_timer_running(void);
extern int rt_get_timer_cpu(void);

extern RT_TASK*  __rt_task_init(void (*rt_thread)(int), int data,
			int stack_size, int priority, int uses_fpu, void(*signal)(void));

extern RT_TASK*  __rt_task_init_cpuid(void (*rt_thread)(int), int data,
				int stack_size, int priority, int uses_fpu,
				void(*signal)(void), unsigned int run_on_cpu);

extern RT_TASK*  __rt_named_task_init(const char* name,void (*rt_thread)(int), int data,
			int stack_size, int priority, int uses_fpu, void(*signal)(void));

extern RT_TASK*  __rt_named_task_init_cpuid(const char* name, void (*rt_thread)(int), int data,
				int stack_size, int priority, int uses_fpu,
				void(*signal)(void), unsigned int run_on_cpu);

extern RT_TASK* __rt_get_named_task( const char* name );

extern void rt_set_runnable_on_cpus(RT_TASK *task, unsigned int cpu_mask);
extern void rt_set_runnable_on_cpuid(RT_TASK *task, unsigned int cpuid);
extern int __rt_task_delete(RT_TASK *task);
extern int __rt_named_task_delete(RT_TASK *task);
extern RT_TASK* rt_whoami(void);
extern void rt_task_yield(void);
extern int rt_task_suspend(RT_TASK* task);
extern int rt_task_resume(RT_TASK* task);

extern int __rt_tld_create_key(void);
extern int __rt_tld_free_key(int key);
extern int __rt_tld_set_data(RT_TASK *task,int key,void* data);
extern void* __rt_tld_get_data(RT_TASK *task,int key);

extern int rt_task_make_periodic(RT_TASK* task, long long start_time, long long period);
extern int rt_task_make_periodic_relative_ns(RT_TASK* task, long long start_delay, long long period);
extern void rt_task_wait_period(void);
extern void rt_busy_sleep(int nanosecs);
extern void rt_sleep(long long delay);
extern void rt_sleep_until(long long time);
extern int rt_task_signal_handler(RT_TASK *task, void (*handler)(void));
extern int rt_task_use_fpu(RT_TASK* task, int use_fpu_flag);
extern void rt_linux_use_fpu(int use_fpu_flag);
extern void rt_preempt_always(int yes_no);
extern void rt_preempt_always_cpuid(int yes_no, unsigned int cpu_id);

/* semaphore wrappers */

extern SEM* __rt_typed_sem_init(int value, int type);
extern SEM* __rt_typed_named_sem_init( const char* name, int value, int type );
extern SEM* __rt_get_named_sem( const char* name );

extern int __rt_sem_delete(SEM* sem);
extern int __rt_named_sem_delete(SEM* sem);
extern int rt_sem_signal(SEM* sem);
extern int rt_sem_broadcast(SEM* sem);
extern int rt_sem_wait(SEM* sem);
extern int rt_sem_wait_if(SEM* sem);
extern int rt_sem_wait_until(SEM* sem, long long time);
extern int rt_sem_wait_timed(SEM* sem, long long delay);

/* By peter soetens  */
extern void rt_sem_init(SEM* sem, int value);
extern int rt_sem_delete(SEM* sem);

extern int __rt_cond_wait(CND *cnd, SEM *mtx);
extern int __rt_cond_wait_until(CND *cnd, SEM *mtx, RTIME time);
extern int __rt_cond_wait_timed(CND *cnd, SEM *mtx, RTIME delay);

/* mailbox wrappers */

extern MBX* __rt_mbx_init(int size);
extern MBX* __rt_named_mbx_init( const char* name, int size);
extern MBX* __rt_get_named_mbx( const char* name );

extern int __rt_mbx_delete(MBX* mbx);
extern int __rt_named_mbx_delete(MBX* mbx);
extern int rt_mbx_send(MBX* mbx, void* msg, int msg_size);
extern int rt_mbx_send_wp(MBX* mbx, void* msg, int msg_size);
extern int rt_mbx_send_if(MBX* mbx, void* msg, int msg_size);
extern int rt_mbx_send_until(MBX* mbx, void* msg, int msg_size, long long time);
extern int rt_mbx_send_timed(MBX* mbx, void* msg, int msg_size, long long delay);
extern int rt_mbx_receive(MBX* mbx, void* msg, int msg_size);
extern int rt_mbx_receive_wp(MBX* mbx, void* msg, int msg_size);
extern int rt_mbx_receive_if(MBX* mbx, void* msg, int msg_size);
extern int rt_mbx_receive_until(MBX* mbx, void* msg, int msg_size, long long time);
extern int rt_mbx_receive_timed(MBX* mbx, void* msg, int msg_size, long long delay);


// rtf functions

#ifndef MAX_FIFOS
#define MAX_FIFOS 64
#endif

extern int rtf_create(unsigned int fifo, int size);
extern int rtf_destroy(unsigned int fifo);
extern int rtf_reset(unsigned int fifo);
extern int rtf_resize(unsigned int fifo, int size);
extern int rtf_put(unsigned int fifo, void *buf, int count);
extern int rtf_get(unsigned int fifo, void *buf, int count);
extern int rtf_create_handler(unsigned int fifo, int (*handler)(unsigned int fifo));


extern TBX* __rt_tbx_init(int size, int flags);  
extern int __rt_tbx_delete(TBX *tbx);
extern int rt_tbx_send(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_send_if(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_send_until(TBX *tbx, void *msg, int msg_size, RTIME time);
extern int rt_tbx_send_timed(TBX *tbx, void *msg, int msg_size, RTIME delay);
extern int rt_tbx_receive(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_receive_if(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_receive_until(TBX *tbx, void *msg, int msg_size, RTIME time);
extern int rt_tbx_receive_timed(TBX *tbx, void *msg, int msg_size, RTIME delay);

extern int rt_tbx_broadcast(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_broadcast_if(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_broadcast_until(TBX *tbx, void *msg, int msg_size, RTIME time);
extern int rt_tbx_broadcast_timed(TBX *tbx, void *msg, int msg_size, RTIME delay);

extern int rt_tbx_urgent(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_urgent_if(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_urgent_until(TBX *tbx, void *msg, int msg_size, RTIME time);
extern int rt_tbx_urgent_timed(TBX *tbx, void *msg, int msg_size, RTIME delay);

extern BITS* __rt_bits_init(unsigned long mask);
extern int __rt_bits_delete(BITS *bits);
extern unsigned long rt_get_bits(BITS *bits);
extern int rt_bits_reset(BITS *bits, unsigned long mask);
extern unsigned long rt_bits_signal(BITS *bits, int setfun, unsigned long masks);
extern int rt_bits_wait(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks,unsigned long *resulting_mask);
extern int rt_bits_wait_if(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, unsigned long *resulting_mask);
extern int rt_bits_wait_until(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, RTIME time, unsigned long *resulting_mask);
extern int rt_bits_wait_timed(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, RTIME delay, unsigned long *resulting_mask);

#ifdef CONFIG_RTAI_TRACE
extern int __trace_destroy_event( int id );
extern int __trace_create_event( const char* name, void* p1, void* p2);
extern int __trace_raw_event( int id, int size, void* p);
#endif

/* By Peter Soetens */

extern RTIME __timeval2count(struct timeval *t);

extern void  __count2timeval(RTIME rt, struct timeval *t);

extern RTIME __timespec2count(const struct timespec *t);

extern void __count2timespec(RTIME rt, struct timespec *t);

#ifdef __cplusplus

}
#endif

#endif
