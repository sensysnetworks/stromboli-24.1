////////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 2000 Zentropic Computing LLC
//
//  Authors:            Stuart Hughes
//  Contact:            info@zentropix.com
//  Original date:      April 13th 2000
//  Ident:              @(#)$Id: rt_compat.h,v 1.1.1.1 2004/06/06 14:01:49 rpm Exp $
//  Description:	RTL/RTAI common api header
//  License: You are free to copy this header file without restriction
//
////////////////////////////////////////////////////////////////////////////////

#ifndef RTL_RTAI_H

#ifndef FREQ 
//#warning "FREQ not defined, defaulting FREQ to 1000 Hz"
#define FREQ 		1000
#endif
#define BASE_PER	(NSECS_PER_SEC/FREQ)
#define NSECS_PER_SEC   1000000000

typedef void *(* VP_FP)(void);
typedef void  (* V_FP_V  )(void);
typedef void  (* V_FP_I  )(int);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/config.h>
#include <asm/io.h>

#if CONFIG_RTL
#    include <rt/rtl.h>
#    ifndef _RTAI_NEWFIFOS_H_
#        include <rt/rtl_fifo.h>
#    endif
#    include <rt/rtl_sched.h>
#    include <rt/posix/pthread.h>
#    define rtf_save_flags_and_cli(x)  rtl_spin_lock_irqsave(&rtf_lock, (x))
#    define rtf_restore_flags(x)     rtl_spin_unlock_irqrestore(&rtf_lock, (x))
#    define rtf_spin_lock_irqsave(x, y)       rtl_spin_lock_irqsave(&(y), (x))
#    define rtf_spin_unlock_irqrestore(x, y)  \
					rtl_spin_unlock_irqrestore(&(y), (x))
#    define rtf_request_srq(x)  rtl_get_soft_irq((x), "fifo")
#    define rtf_free_srq(x)     rtl_free_soft_irq((x))
#    define rtf_pend_srq(x)     rtl_global_pend_irq((x))
#    define TASK_STR              pthread_t
#    define get_time_ns           gethrtime
#    define rt_task_wait_period   pthread_wait_np
#    define rt_task_suspend(x)    pthread_suspend_np(*x)
#    define rt_task_resume(x)     pthread_wakeup_np(*x)
#    define rt_get_time_ns        get_time_ns
#    define rtl_task_make_periodic(task_ptr, when_ns, period_ns)        \
        pthread_make_periodic_np(*task_ptr, when_ns, period_ns)
#    define rt_task_del(task_ptr)                                       \
        pthread_delete_np(*task_ptr)
#    define rt_timer_stop()
#    define rt_mount()
#    define rt_unmount()
#    define rt_set_oneshot_mode rtl_set_oneshot_mode
#    define rt_linux_use_fpu()
#    define rt_free_global_irq    rtl_free_irq
#    define rt_enable_irq         rtl_hard_enable_irq
#    define rt_request_global_irq rtl_request_irq
#else
#    include <rtai.h>
#    include <rtai_sched.h>
#    include <rtai_fifos.h>
#    define rtf_save_flags_and_cli(x)  x = rt_spin_lock_irqsave(&rtf_lock)
#    define rtf_restore_flags(x)      rt_spin_unlock_irqrestore((x), &rtf_lock)
#    define rtf_spin_lock_irqsave(x, y)       x = rt_spin_lock_irqsave(&(y))
#    define rtf_spin_unlock_irqrestore(x, y)  \
					rt_spin_unlock_irqrestore((x), &(y))
#    define rtf_request_srq(x)  rt_request_srq(0, (x), 0)
#    define rtf_free_srq(x)     rt_free_srq((x))
#    define rtf_pend_srq(x)     rt_pend_linux_srq((x))
#    define TASK_STR              RT_TASK
#    define rtf_create(num, size) rtf_create_using_bh(num, size, 0)
#    define get_time_ns           rt_get_cpu_time_ns
#    define rtl_schedule()
#    define rtl_task_make_periodic(task_ptr, when_ns, period_ns)        \
        rt_task_make_periodic(task_ptr,                                 \
                                nano2count(when_ns),                    \
                                nano2count(period_ns))
#    define     rt_task_del(task_ptr)                                   \
        do {    rt_task_suspend(task_ptr);                              \
                rt_task_delete(task_ptr);       } while(0)
#    define     rt_timer_stop()                                         \
        do {    stop_rt_timer();                                        \
                rt_busy_sleep(10000000);             } while(0)
#    define rt_mount   rt_mount_rtai
#    define rt_unmount rt_umount_rtai
#endif

struct task_data {
    TASK_STR task;
    VP_FP func;
    int arg;
    int stack_size;
    int priority;
    int period;
    int uses_fpu;
    int one_shot;
    V_FP_V sig_han;
    long long period_ns;
    long long when_ns;
};

////////////////////////////////////////////////////////////////
// task creation wrapper
///////////////////////////////////////////////////////////////
static inline int rt_task_create( struct task_data *t)
{
#if CONFIG_RTL

    struct sched_param p;
    pthread_create(     &t->task,
                        NULL,
                        t->func,
                        (void *)t->arg
                );
    if(t->uses_fpu) {
        pthread_setfp_np(t->task, 1);
    }
    p.sched_priority = 100 - t->priority;
    pthread_setschedparam(t->task, SCHED_FIFO, &p);

    if(t->period == 0) {
        return 0;
    }

#else                                   // RTAI

    static int timer_started = 0;


    if( timer_started == 0) {
        // start timer, this defaults to periodic mode
        if (t->one_shot)
	    rt_set_oneshot_mode();
        start_rt_timer(nano2count(BASE_PER));
        timer_started   = 1;
    }

    rt_task_init(       &t->task,
                        (V_FP_I)t->func,
                        t->arg,
                        t->stack_size,
                        t->priority,
                        t->uses_fpu,
                        t->sig_han
                );

    if(t->period == 0) {
        return 0;
    }
    if(t->uses_fpu) {
        rt_linux_use_fpu(1);
    }

#endif

    t->when_ns          = rt_get_time_ns() + 1 * NSECS_PER_SEC;
    t->period_ns        = (long long)t->period * BASE_PER;
    rtl_task_make_periodic( &t->task, t->when_ns, t->period_ns);

    return 0;
}



#endif
