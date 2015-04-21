//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 2001 Steve Papacharalambous, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@lineo.com)
// Original date:       Thu 15 Jul 1999
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
//
// pthreads interface for Real Time Linux.
//
///////////////////////////////////////////////////////////////////////////////
static char id_rtai_pthread_c[] __attribute__ ((unused)) = "@(#)$Id: rtai_pthread.c,v 1.1.1.1 2004/06/06 14:03:03 rpm Exp $";


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/time.h>

#include <rtai_pthread.h>
#include <rtai_pthread_int.h>

#include <rtai_utils.h>

#include <rtai_trace.h>

#define RUNNABLE_ON_CPUS 3
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)

#define TIMER_TO_CPU 3             // < 0 || > 1 to maintain a symmetrically processed timer

#define RT_STACK 3000

#define DEFAULT_TICK_PERIOD 1000000

#ifdef CONFIG_RTAI_FPU_SUPPORT
#define TASK_USES_FPU TASK_FPU_ENABLE
#else
#define TASK_USES_FPU TASK_FPU_DISABLE
#endif

#define RT_CURRENT rt_smp_current[hard_cpu_id()]

#define LXRT_RESUME(x)  {extern void emuser_trxl(RT_TASK *); emuser_trxl((x));}
#define LXRT_SUSPEND {extern void (*dnepsus_trxl)(void); (*dnepsus_trxl)();}

typedef void (* RT_THREAD)(int);

// ----------------------------------------------------------------------------

static char pthread_version[] = "1.0";

static unsigned int num_pthreads = 0;
static struct _pthread_descr_struct rt_pthread_descr[POSIX_THREADS_MAX] = {{0}};
static RTIME tick_period;

// ----------------------------------------------------------------------------

// Kludge section.
// Replicates code sections that are not easily accessible from RT land.
// Hmm..  there must be a better way....

#define PTHREAD_PRIORITY_FACTOR 99

// Linux has no ENOTSUP error code.
#define ENOTSUP EOPNOTSUPP


int get_max_priority(int policy) {

  int ret = -EINVAL;

  switch (policy) {
    case SCHED_FIFO:
    case SCHED_RR:
      ret = 99;
      break;
    case SCHED_OTHER:
      ret = 0;
      break;
    default:
      break;
  }

  return(ret);

}  // End function - get_max_priority.


int get_min_priority(int policy) {

  int ret = -EINVAL;

  switch (policy) {
    case SCHED_FIFO:
    case SCHED_RR:
      ret = 1;
      break;
    case SCHED_OTHER:
      ret = 0;
      break;
    default:
      break;
  }

  return ret;

}  // End function - get_min_priority.


// ----------------------------------------------------------------------------


// Thread creation/control section.


int pthread_create(pthread_t *thread, pthread_attr_t *attr,
                    void *(*start_routine) (void *), void *arg) {

  int i, j, r_c;
  unsigned long flags;

  Z_APPS *zapps_ptr = NULL;	// ****TPW Additions: 10/1/00

  if( num_pthreads >= POSIX_THREADS_MAX ) {
    return(EAGAIN);

  }  // End if - Cannot create any more pthreads.
// Find a free pthread descriptor structure.
// Note: A value of 0 in the p_tid field indicates a free pthread descriptor.
  flags = rt_global_save_flags_and_cli();
  for(j = 0; j < POSIX_THREADS_MAX; j++) {
    if(rt_pthread_descr[j].p_tid == 0) {
// Initialise the thread descriptor.
      rt_pthread_descr[j].p_nextwaiting = NULL;
      rt_pthread_descr[j].p_tid = j + 1;
      rt_pthread_descr[j].p_priority = 0; // Default to lowest pthread priority.
      rt_pthread_descr[j].p_policy = SCHED_OTHER;
      rt_pthread_descr[j].p_spinlock = NULL;
      rt_pthread_descr[j].p_signal = 0;
      rt_pthread_descr[j].p_signal_jmp = NULL;
      rt_pthread_descr[j].p_cancel_jmp = NULL;
      rt_pthread_descr[j].p_terminated = 0;
      rt_pthread_descr[j].p_detached = attr == NULL ? 0 : attr->detachstate;
      rt_pthread_descr[j].p_exited = 0;
      rt_pthread_descr[j].p_retval = NULL;
      rt_pthread_descr[j].p_joining = NULL;
      rt_pthread_descr[j].p_cleanup = NULL;
      rt_pthread_descr[j].p_cancelstate = PTHREAD_CANCEL_ENABLE;
      rt_pthread_descr[j].p_canceltype = PTHREAD_CANCEL_DEFERRED;
      rt_pthread_descr[j].p_canceled = 0;
      rt_pthread_descr[j].p_errno = 0;
      rt_pthread_descr[j].p_h_errno = 0;
      for(i = 0; i < PTHREAD_KEY_1STLEVEL_SIZE; i++) {
        rt_pthread_descr[j].p_specific[i] = NULL;
      }
      rt_pthread_descr[j].p_start_args.schedpolicy = SCHED_OTHER;

      TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_CREATE,
		       start_routine,
		       rt_pthread_descr[j].p_tid,
		       0);

// Initialise the scheduling parameters for the new thread.
      if(attr != NULL && attr->schedpolicy != SCHED_OTHER) {
        switch(attr->inheritsched) {
          case PTHREAD_EXPLICIT_SCHED:
            rt_pthread_descr[j].p_start_args.schedpolicy = attr->schedpolicy;
            rt_pthread_descr[j].p_start_args.schedparam = attr->schedparam;
          break;
          case PTHREAD_INHERIT_SCHED:
// TODO
//These must be uncommented when the new RT calls have been implemented,
// also father_pid has to be determined.
// stevep 19 July 1999
//        rt_pthread_descr[j].schedpolicy = sched_getscheduler(father_pid);
//        sched_getparam(father_pid, &rt_pthread_descr[j].schedparam);
          break;
        }
        rt_pthread_descr[j].p_priority =
          rt_pthread_descr[j].p_start_args.schedparam.sched_priority;
        rt_pthread_descr[j].p_policy =
          rt_pthread_descr[j].p_start_args.schedpolicy;
      }
      rt_pthread_descr[j].p_start_args.start_routine = start_routine;
      rt_pthread_descr[j].p_start_args.arg = arg;
//TODO
// Not currently set up, but needed for full implementation?
//  rt_pthread_descr[j].p_start_args.mask = *mask;

// Initialise the priority in the RTAI task structure.
// The priorities in RTAI are the other way up to the POSIX ones, sigh...
      if(rt_pthread_descr[j].p_priority > PTHREAD_PRIORITY_FACTOR) {
        rt_pthread_descr[j].p_priority = PTHREAD_PRIORITY_FACTOR;
      }
      rt_pthread_descr[j].rtask_struct.priority =
                   PTHREAD_PRIORITY_FACTOR - rt_pthread_descr[j].p_priority;

// Save the pointer to this pthread descriptor in the RT scheduler structure.
// ****TPW Changes: 10/1/00 - Start
      if(rt_pthread_descr[j].rtask_struct.system_data_ptr == NULL) {
          rt_pthread_descr[j].rtask_struct.system_data_ptr = 
		init_z_apps(&rt_pthread_descr[j].rtask_struct);
      }
      zapps_ptr = (Z_APPS*)rt_pthread_descr[j].rtask_struct.system_data_ptr;
      zapps_ptr->pthreads = (void*)&rt_pthread_descr[j];

//      rt_pthread_descr[j].rtask_struct.system_data_ptr = (void *)&rt_pthread_descr[j];
// ****TPW Changes: 10/1/00 - End

// Create the new thread.
      r_c = rt_task_init(&rt_pthread_descr[j].rtask_struct,
                         (RT_THREAD)start_routine, (int)arg, RT_STACK,
                         rt_pthread_descr[j].rtask_struct.priority,
                         TASK_USES_FPU, NULL);

      if( r_c == 0 ) {

        rt_set_runnable_on_cpus(&rt_pthread_descr[j].rtask_struct,
                                RUN_ON_CPUS);

// Return the index (+1) into the attribute array as the new thread id.
        *thread = (pthread_t)j + 1;
        rt_pthread_descr[j].p_tid = j + 1;

// Save the pointer to this pthread descriptor in the RT scheduler structure.
// ****TPW Changes: 10/1/00 - Start
      if(rt_pthread_descr[j].rtask_struct.system_data_ptr == NULL) {
          rt_pthread_descr[j].rtask_struct.system_data_ptr = 
		init_z_apps(&rt_pthread_descr[j].rtask_struct);
      }
      zapps_ptr = (Z_APPS*)rt_pthread_descr[j].rtask_struct.system_data_ptr;
      zapps_ptr->pthreads = (void*)&rt_pthread_descr[j];
//        rt_pthread_descr[j].rtask_struct.system_data_ptr =
//                      (void *)&rt_pthread_descr[j];
// ****TPW Changes: 10/1/00 - End

        if( rt_task_resume(&rt_pthread_descr[j].rtask_struct) < 0 ) {
// Uurgh -- should be: -EINVAL, but only error return specified for
// pthread_create is EAGAIN.
// Also if activation of the thread fails should delete the task from
// the RTAI list.

          rt_pthread_descr[j].p_tid = 0;
          rt_task_delete(&rt_pthread_descr[j].rtask_struct);
          rt_global_restore_flags(flags);
          return(EAGAIN);
        }  // End if - Error from rt_task_resume
        rt_global_restore_flags(flags);
        return(0);
      } else {
        rt_global_restore_flags(flags);
        return(EAGAIN);
      }  // End if/else - process return code from rt_task_init

    }  // End if - Initialise pthread descriptor.

  }  // End for loop - find a free pthread descriptor.

  rt_global_restore_flags(flags);
  return(EAGAIN);

}  // End function - pthread_create


void pthread_exit(void *retval) {

  unsigned long flags;
  pthread_descr thread_ptr;
  RT_TASK *rt_task_ptr;

  Z_APPS *zapps_ptr = NULL;

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_EXIT, 0, 0, 0);

  rt_task_ptr = rt_whoami();
// ****TPW
  zapps_ptr = (Z_APPS*)rt_task_ptr->system_data_ptr;
  thread_ptr = (pthread_descr)zapps_ptr->pthreads;
  free_z_apps(rt_task_ptr);
  //thread_ptr = (pthread_descr)rt_task_ptr->system_data_ptr;
// ***TPW
  flags = rt_global_save_flags_and_cli();
  thread_ptr->p_tid = 0;
  rt_task_delete(rt_task_ptr);
  rt_global_restore_flags(flags);

} // End function - pthread_exit


int sched_yield(void) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_SCHED_YIELD, 0, 0, 0);
  rt_task_yield();
  return(0);

}  // End function - pthread_yield.


pthread_t pthread_self(void) {

  pthread_descr thread_ptr;
  RT_TASK *rt_task_ptr;

  Z_APPS *zapps_ptr = NULL;

  rt_task_ptr = rt_whoami();
// ****TPW
  zapps_ptr = (Z_APPS*)rt_task_ptr->system_data_ptr;
  thread_ptr = (pthread_descr)zapps_ptr->pthreads;
  //thread_ptr = (pthread_descr)rt_task_ptr->system_data_ptr;
// ***TPW

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_SELF, thread_ptr->p_tid, 0, 0);

  return(thread_ptr->p_tid);

} // End function - pthread_self


// ----------------------------------------------------------------------------

// Thread attribute section.

int pthread_attr_init(pthread_attr_t *attr) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_INIT, attr, 0, 0);

  attr->detachstate = PTHREAD_CREATE_JOINABLE;
  attr->schedpolicy = SCHED_OTHER;
  attr->schedparam.sched_priority = 0;
  attr->inheritsched = PTHREAD_EXPLICIT_SCHED;
  attr->scope = PTHREAD_SCOPE_SYSTEM;
  return(0);

}  // End function - pthread_attr_init


int pthread_attr_destroy(pthread_attr_t *attr) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_DESTROY, attr, 0, 0);

  return(0);

}  // End function - pthread_attr_destroy


int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_SETDETACHSTATE, attr, detachstate, 0);

  if (detachstate < PTHREAD_CREATE_JOINABLE ||
      detachstate > PTHREAD_CREATE_DETACHED)
    return(EINVAL);
  attr->detachstate = detachstate;
  return(0);

}  // End function - pthread_attr_setdetachstate


int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_GETDETACHSTATE, attr, attr->detachstate, 0);

  *detachstate = attr->detachstate;
  return(0);

}  // End function - pthread_attr_getdetachstate


int pthread_attr_setschedparam(pthread_attr_t *attr,
			       const struct sched_param *param) {

  int max_prio = get_max_priority(attr->schedpolicy);
  int min_prio = get_min_priority(attr->schedpolicy);

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_SETSCHEDPARAM, attr, 0, 0);

  if(param->sched_priority < min_prio || param->sched_priority > max_prio) {
    return(EINVAL);
  }
  attr->schedparam = *param;
  return(0);

}  // End function - pthread_attr_setschedparam


int pthread_attr_getschedparam(const pthread_attr_t *attr,
			       struct sched_param *param) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_GETSCHEDPARAM, attr, 0, 0);

  *param = attr->schedparam;
  return(0);

}  // End function - pthread_attr_getschedparam


int pthread_attr_setschedpolicy(pthread_attr_t *attr, int policy) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_SETSCHEDPOLICY, attr, policy, 0);

  if(policy != SCHED_OTHER && policy != SCHED_FIFO && policy != SCHED_RR) {
    return(EINVAL);
  }
  attr->schedpolicy = policy;
  return(0);

}  // End function - pthread_attr_setschedpolicy


int pthread_attr_getschedpolicy(const pthread_attr_t *attr, int *policy) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_GETSCHEDPOLICY, attr, attr->schedpolicy, 0);

  *policy = attr->schedpolicy;
  return(0);

}  // End function - pthread_attr_getschedpolicy


int pthread_attr_setinheritsched(pthread_attr_t *attr, int inherit) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_SETINHERITSCHED, attr, inherit, 0);

  if(inherit != PTHREAD_INHERIT_SCHED && inherit != PTHREAD_EXPLICIT_SCHED) {
    return(EINVAL);
  }
  attr->inheritsched = inherit;
  return(0);

}  // End function - pthread_attr_setinheritsched


int pthread_attr_getinheritsched(const pthread_attr_t *attr, int *inherit) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_SETINHERITSCHED, attr, attr->inheritsched, 0);

  *inherit = attr->inheritsched;
  return(0);

}  // End function - pthread_attr_getinheritsched


int pthread_attr_setscope(pthread_attr_t *attr, int scope) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_SETSCOPE, attr, scope, 0);

  switch (scope) {
  case PTHREAD_SCOPE_SYSTEM:
    attr->scope = scope;
    return(0);
  case PTHREAD_SCOPE_PROCESS:
    return(ENOTSUP);
  default:
    return(EINVAL);
  }

}  // End function - pthread_attr_setscope


int pthread_attr_getscope(const pthread_attr_t *attr, int *scope) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_ATTR_GETSCOPE, attr, attr->scope, 0);

  *scope = attr->scope;
  return(0);

}  // End function - pthread_attr_getscope


// ----------------------------------------------------------------------------

// Thread scheduling section.


int pthread_setschedparam(pthread_t thread, int policy,
                          const struct sched_param *param) {

  int max_prio;
  int min_prio;
  unsigned long flags;
  pthread_descr pthread_handle;


// Get the thread descriptor and verify that it is a valid thread.
  pthread_handle = &rt_pthread_descr[thread - 1];
  if(pthread_handle->p_tid == 0) {
    return(ESRCH);
  }

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_SETSCHEDPARAM, pthread_handle->p_tid, policy, 0);

  max_prio = get_max_priority(policy);
  min_prio = get_min_priority(policy);

  if(min_prio < 0 || max_prio < 0) {
    return(EINVAL);
  }

  if(param->sched_priority < min_prio || param->sched_priority > max_prio) {
    return(EINVAL);
  }

// Set the new scheduling parameters for the thread.
  flags = rt_global_save_flags_and_cli();
  pthread_handle->p_priority = param->sched_priority;
  pthread_handle->p_policy = policy;
  pthread_handle->rtask_struct.priority = PTHREAD_PRIORITY_FACTOR - pthread_handle->p_priority;
  pthread_handle->rtask_struct.base_priority = PTHREAD_PRIORITY_FACTOR - pthread_handle->p_priority;
  rt_global_restore_flags(flags);
  return(0);

} // End function - pthread_setschedparam


int pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param) {

  pthread_descr pthread_handle;


// Get the thread descriptor and verify that it is a valid thread.
  pthread_handle = &rt_pthread_descr[thread - 1];
  if(pthread_handle->p_tid == 0) {
    return(ESRCH);
  }

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_GETSCHEDPARAM,
		   pthread_handle->p_tid,
		   pthread_handle->p_policy,
		   0);

// Get the new scheduling parameters for the thread.
  *policy = pthread_handle->p_policy;
  param->sched_priority = pthread_handle->p_priority;
  return(0);

} // End function - pthread_getschedparam



// ----------------------------------------------------------------------------

// clock/timer section.

int clock_gettime(int clockid, struct timespec *current_time) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_CLOCK_GETTIME, clockid, 0, 0);

  if(clockid != CLOCK_REALTIME) {
    current_time->tv_sec = 0;
    current_time->tv_nsec = 0;
    return -EINVAL;
  }

  ts_from_ns(count2nano(rt_get_time()), current_time);
  return 0;

}  // End function - clock_gettime


int nanosleep(const struct timespec *rqtp, struct timespec *rmtp)
{
	RTIME expire;

	if (rqtp->tv_nsec >= 1000000000L || rqtp->tv_nsec < 0 || rqtp->tv_sec < 0) {
		return -EINVAL;
	}
	rt_sleep_until(expire = rt_get_time() + timespec2count(rqtp));
	if ((expire -= rt_get_time()) > 0) {
		if (rmtp) {
			count2timespec(expire, rmtp);
		}
		return -EINTR;
	}
	return 0;
}

// ----------------------------------------------------------------------------

//  RT Task Utility Functions.


// Insert RT task into mutex queue in priority order.
void priority_enqueue_task(pthread_mutex_t *mutex) {

  QUEUE *rt_q;
  RT_TASK *current_task;

  rt_q = &(mutex->m_semaphore.queue);

// Check that the queues have been properly initialised.  Hackety hack... due to not
// being able to initialise the queues if the static initialiser macro is used...
  if(rt_q->prev == NULL || rt_q->next == NULL) {
    rt_q->prev = &(mutex->m_semaphore.queue);
    rt_q->next = &(mutex->m_semaphore.queue);

  }

  current_task = rt_whoami();

  while((rt_q = rt_q->next) != &(mutex->m_semaphore.queue) &&
          (rt_q->task)->priority <= current_task->priority);

  rt_q->prev = (current_task->queue.prev = rt_q->prev)->next =
                                           &(current_task->queue);
  current_task->queue.next = rt_q;

}  // End function - priority_enqueue_task



// Insert RT task into conditional variable queue.
void cond_enqueue_task(pthread_cond_t *cond) {

  QUEUE *rt_q;
  RT_TASK *current_task;

  rt_q = &(cond->c_waiting.queue);

// Check that the queues have been properly initialised.  Hackety hack... due to not
// being able to initialise the queues if the static initialiser macro is used...
  if(rt_q->prev == NULL || rt_q->next == NULL) {
    rt_q->prev = &(cond->c_waiting.queue);
    rt_q->next = &(cond->c_waiting.queue);

  }

  current_task = rt_whoami();
  rt_q->prev = (current_task->queue.prev = rt_q->prev)->next =
                                           &(current_task->queue);
  current_task->queue.next = rt_q;

}  // End function - cond_enqueue_task


// Propagate priority to task owning mutex.
void mutex_inherit_prio(pthread_mutex_t *mutex) {

  RT_TASK *current_task;

  current_task = rt_whoami();

  if(mutex->m_owner->priority > current_task->priority) {
    mutex->m_owner->priority = current_task->priority;
  }

}  // End function - mutex_inherit_prio



// Dequeue next task from the mutex queue.
void dequeue_task(pthread_mutex_t *mutex) {

  RT_TASK *next_task;

  next_task = (mutex->m_semaphore.queue.next)->task;
  (next_task->queue.prev)->next = next_task->queue.next;
  (next_task->queue.next)->prev = next_task->queue.prev;

}  // End function - dequeue_task


// Debug Routine - Display the tasks queued on a mutex queue.
void z_print_mutex_q(pthread_mutex_t *mutex) {

  QUEUE *rt_q;
  pthread_descr descr_ptr;
  
  rt_q = &(mutex->m_semaphore.queue);

  printk("--<z_print_mutex_q - RT threads on mutex queue>--\n");
  while((rt_q = rt_q->next) != &(mutex->m_semaphore.queue)) {
    descr_ptr = (pthread_descr)rt_q->task->system_data_ptr;
    printk("Task id: %ld, Priority: %d, State: %d \n", descr_ptr->p_tid, rt_q->task->priority, rt_q->task->state);

  }

  printk("--<End of thread list>--\n\n");

}  // End function - z_print_mutex_q



// ----------------------------------------------------------------------------

// Mutex section.

int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *mutex_attr) {

  QUEUE *rt_q;

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_MUTEX_INIT, mutex, 0, 0);

  mutex->m_owner = 0;
  mutex->m_kind =
    mutex_attr == NULL ? PTHREAD_MUTEX_FAST_NP : mutex_attr->mutexkind;
  mutex->m_semaphore.magic = RT_SEM_MAGIC;
  mutex->m_semaphore.count = 1;
  mutex->m_semaphore.type = BIN_SEM;
  rt_q = &(mutex->m_semaphore.queue);
  rt_q->prev = &(mutex->m_semaphore.queue);
  rt_q->next = &(mutex->m_semaphore.queue);
  rt_q->task = 0;
  mutex->m_semaphore.owndby = 0;
  mutex->m_semaphore.qtype = FIFO_Q;
  return(0);

}  // End function - pthread_mutex_init


int pthread_mutex_destroy(pthread_mutex_t *mutex) {

  int return_code;
  unsigned long flags;
  QUEUE *rt_q;

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_MUTEX_DESTROY, mutex, 0, 0);

  if(mutex->m_semaphore.magic != RT_SEM_MAGIC) {
    return(EINVAL);
  }
  flags = rt_global_save_flags_and_cli();
  if(mutex->m_semaphore.count < 1) {
    return_code = EBUSY;
  } else {
    mutex->m_semaphore.magic = 0;
    rt_q = &(mutex->m_semaphore.queue);
    while ((rt_q = rt_q->next) != &(mutex->m_semaphore.queue)) {
      (rt_q->task)->state &= ~(SEMAPHORE | DELAYED);
    }

    return_code = 0;

  }  // End else - mutex is free, clear its queue.

  rt_global_restore_flags(flags);
  return(return_code);

}  // End function - pthread_mutex_destroy



int pthread_mutexattr_init(pthread_mutexattr_t *attr) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_MUTEXATTR_INIT, attr, 0, 0);

  attr->mutexkind = PTHREAD_MUTEX_FAST_NP;
  return(0);

}  // End function - pthread_mutexattr_init



int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_MUTEXATTR_DESTROY, attr, 0, 0);

  return(0);

}  // End function - pthread_mutex_attr_destroy


int pthread_mutexattr_setkind_np(pthread_mutexattr_t *attr, int kind) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_MUTEXATTR_SETKIND_NP, attr, kind, 0);

  if (kind != PTHREAD_MUTEX_FAST_NP
      && kind != PTHREAD_MUTEX_RECURSIVE_NP
      && kind != PTHREAD_MUTEX_ERRORCHECK_NP)
    return(EINVAL);
  attr->mutexkind = kind;
  return(0);

}  // End function - pthread_mutexattr_setkind_np


int pthread_mutexattr_getkind_np(const pthread_mutexattr_t *attr, int *kind) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_MUTEXATTR_GETKIND_NP, attr, kind, 0);

  *kind = attr->mutexkind;
  return(0);

}  // End function - pthread_mutexattr_getkind_np


int pthread_mutex_trylock(pthread_mutex_t *mutex) {

  int return_code;
  unsigned long flags;
  RT_TASK *self;

  if(mutex->m_semaphore.magic != RT_SEM_MAGIC) {
    return(EINVAL);
  }

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_MUTEX_TRY_LOCK, mutex, 0, 0);

  flags = rt_global_save_flags_and_cli();
  self = rt_whoami();
  switch(mutex->m_kind) {
  case PTHREAD_MUTEX_FAST_NP:
    if(mutex->m_semaphore.count > 0) {
      mutex->m_semaphore.count = 0;
      mutex->m_semaphore.owndby = self;
      mutex->m_owner = self;
      return_code = 0;
    } else {
      return_code = EBUSY;
    }
    break;
  case PTHREAD_MUTEX_RECURSIVE_NP:
    if (mutex->m_semaphore.count == 0 || mutex->m_owner == self) {
      mutex->m_semaphore.count--;
      mutex->m_semaphore.owndby = self;
      mutex->m_owner = self;
      return_code = 0;
    } else {
      return_code = EBUSY;
    }
    break;
  case PTHREAD_MUTEX_ERRORCHECK_NP:
    if(mutex->m_semaphore.count > 0) {
      mutex->m_semaphore.count = 0;
      mutex->m_semaphore.owndby = self;
      mutex->m_owner = self;
      return_code = 0;
    } else {
      return_code = EBUSY;
    }
    break;
  default:
    return_code = EINVAL;
  }

  rt_global_restore_flags(flags);
  return(return_code);

}  // End function - pthread_mutex_trylock



int pthread_mutex_lock(pthread_mutex_t *mutex) {

  int return_code;
  unsigned long flags;
  RT_TASK *current_task;

  if(mutex->m_semaphore.magic != RT_SEM_MAGIC) {
    return(EINVAL);
  }

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_MUTEX_LOCK, mutex, 0, 0);

  flags = rt_global_save_flags_and_cli();
  current_task = rt_whoami();
  switch(mutex->m_kind) {
  case PTHREAD_MUTEX_FAST_NP:
    if(mutex->m_semaphore.count > 0) {
      mutex->m_semaphore.count = 0;        // Resource is free, grab it.
      mutex->m_owner = current_task;
      mutex->m_semaphore.owndby = current_task;
      return_code = 0;
    } else {
      current_task->state |= SEMAPHORE;
      rt_rem_ready_current(current_task);
      current_task->blocked_on = &(mutex->m_semaphore.queue);
      priority_enqueue_task(mutex);
      mutex_inherit_prio(mutex);
      LXRT_SUSPEND;
      if(current_task->blocked_on ||
         mutex->m_semaphore.magic != RT_SEM_MAGIC) {
          return_code = EINVAL;
      } else {
        mutex->m_semaphore.count = 0;        // Resource is free now, grab it.
        mutex->m_owner = current_task;
        mutex->m_semaphore.owndby = current_task;
        return_code = 0;
      }
    }
    break;
  case PTHREAD_MUTEX_RECURSIVE_NP:
    if(mutex->m_semaphore.magic != RT_SEM_MAGIC) {
      return(EINVAL);
    }
    if (mutex->m_semaphore.count == 0 || mutex->m_owner == current_task) {
      mutex->m_semaphore.count--;
      mutex->m_owner = current_task;
      mutex->m_semaphore.owndby = current_task;
      return_code = 0;
    } else {
      current_task->state |= SEMAPHORE;
      rt_rem_ready_current(current_task);
      current_task->blocked_on = &(mutex->m_semaphore.queue);
      priority_enqueue_task(mutex);
      mutex_inherit_prio(mutex);
      LXRT_SUSPEND;
      if(current_task->blocked_on ||
         mutex->m_semaphore.magic != RT_SEM_MAGIC) {
          return_code = EINVAL;
      } else {
        mutex->m_semaphore.count = 0;        // Resource is free now, grab it.
        mutex->m_owner = current_task;
        mutex->m_semaphore.owndby = current_task;
        return_code = 0;
      }
    }
    break;
  case PTHREAD_MUTEX_ERRORCHECK_NP:
    if(mutex->m_semaphore.count > 0) {
      mutex->m_semaphore.count = 0;
      mutex->m_owner = current_task;
      mutex->m_semaphore.owndby = current_task;
      return_code = 0;
    } else {
      current_task->state |= SEMAPHORE;
      current_task->blocked_on = &(mutex->m_semaphore.queue);
      rt_rem_ready_current(current_task);
      priority_enqueue_task(mutex);
      mutex_inherit_prio(mutex);
      LXRT_SUSPEND;
      if(current_task->blocked_on ||
         mutex->m_semaphore.magic != RT_SEM_MAGIC) {
          return_code = EINVAL;
      } else {
        mutex->m_semaphore.count = 0;        // Resource is free now, grab it.
        mutex->m_owner = current_task;
        mutex->m_semaphore.owndby = current_task;
        return_code = 0;
      }
    }
    break;
  default:
    return_code = EINVAL;
  }

  rt_global_restore_flags(flags);
  return(return_code);

}  // End function - pthread_mutex_lock


int pthread_mutex_unlock(pthread_mutex_t *mutex) {

  int return_code = EINVAL;
  unsigned long flags;
  RT_TASK *current_task, *next_task;

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_MUTEX_UNLOCK, mutex, 0, 0);

  flags = rt_global_save_flags_and_cli();
  current_task = rt_whoami();
  if(mutex->m_semaphore.magic != RT_SEM_MAGIC ||
                              mutex->m_owner != current_task) {
    rt_global_restore_flags(flags);
    return(EINVAL);
  }
  switch(mutex->m_kind) {
  case PTHREAD_MUTEX_FAST_NP:
    mutex->m_semaphore.count = 1;
    mutex->m_owner = 0;
    mutex->m_semaphore.owndby = 0;
    current_task->priority = current_task->base_priority;

// Dequeue the next task in the queue.
    if(mutex->m_semaphore.queue.next != 0 &&
       mutex->m_semaphore.queue.prev != 0 &&
       (next_task = (mutex->m_semaphore.queue.next)->task) != 0) {
      mutex->m_semaphore.queue.next->task->blocked_on = 0;
      mutex->m_semaphore.queue.next->task->state &= ~(SEMAPHORE | DELAYED);
      dequeue_task(mutex);
      rt_rem_timed_task(next_task);
      if(next_task->state == READY) {
        rt_enq_ready_task(next_task);
        LXRT_SUSPEND;
      }
    }
    return_code = 0;
    break;
  case PTHREAD_MUTEX_RECURSIVE_NP:
    mutex->m_semaphore.count++;
    if(mutex->m_semaphore.count <= 0) {
      return_code = 0;
    } else {
      mutex->m_semaphore.count = 1;
     mutex->m_owner = 0;
      current_task->priority = current_task->base_priority;

// Dequeue the next task in the queue.
      if(mutex->m_semaphore.queue.next != 0 &&
         mutex->m_semaphore.queue.prev != 0 &&
         (next_task = (mutex->m_semaphore.queue.next)->task) != 0) {
        mutex->m_semaphore.queue.next->task->blocked_on = 0;
        mutex->m_semaphore.queue.next->task->state &= ~(SEMAPHORE | DELAYED);
        dequeue_task(mutex);
        rt_rem_timed_task(next_task);
        if(next_task->state == READY) {
          rt_enq_ready_task(next_task);
          LXRT_SUSPEND;
        }
      }
      return_code = 0;
    }
    break;
  case PTHREAD_MUTEX_ERRORCHECK_NP:
    if(mutex->m_semaphore.count == 1 || mutex->m_owner != current_task) {
      return_code = EPERM;
    } else {
      mutex->m_semaphore.count = 1;
      mutex->m_owner = 0;
      current_task->priority = current_task->base_priority;

// Dequeue the next task in the queue.
      if(mutex->m_semaphore.queue.next != 0 &&
         mutex->m_semaphore.queue.prev != 0 &&
         (next_task = (mutex->m_semaphore.queue.next)->task) != 0) {
        mutex->m_semaphore.queue.next->task->blocked_on = 0;
        mutex->m_semaphore.queue.next->task->state &= ~(SEMAPHORE | DELAYED);
        dequeue_task(mutex);
        rt_rem_timed_task(next_task);
        if(next_task->state == READY) {
          rt_enq_ready_task(next_task);
          LXRT_SUSPEND;
        }
      }
      return_code = 0;
    }

    break;
  default:
    return_code = EINVAL;
    break;

  } // End switch statement on mutex kind

  rt_global_restore_flags(flags);
  return(return_code);

}  // End function - pthread_mutex_unlock


// ----------------------------------------------------------------------------

// Conditional Variables section.


int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t *cond_attr) {

  QUEUE *rt_q;

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_COND_INIT, cond, 0, 0);

  cond->c_waiting.magic = RT_SEM_MAGIC;
  cond->c_waiting.count = 1;
  cond->c_waiting.type = BIN_SEM;
  rt_q = &(cond->c_waiting.queue);
  rt_q->prev = &(cond->c_waiting.queue);
  rt_q->next = &(cond->c_waiting.queue);
  rt_q->task = 0;
  cond->c_waiting.owndby = 0;
  cond->c_waiting.qtype = FIFO_Q;
  return(0);

}  // End function - pthread_cond_init



int pthread_cond_destroy(pthread_cond_t *cond) {

  unsigned long flags;
  int return_code;
  QUEUE *rt_q;

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_COND_DESTROY, cond, 0, 0);

  rt_q = &(cond->c_waiting.queue);
  flags = rt_global_save_flags_and_cli();
  if( (rt_q->next != &(cond->c_waiting.queue)) &&
                     (rt_q->prev != &(cond->c_waiting.queue)) ) {
    return_code = EBUSY;
  } else {
    cond->c_waiting.magic = 0;
    return_code = 0;
  }

  rt_global_restore_flags(flags);
  return(return_code);

}  // End function - pthread_cond_destroy



int pthread_condattr_init(pthread_condattr_t *attr) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_CONDATTR_INIT, attr, 0, 0);

  return(0);

}  // End function - pthread_condattr_init



int pthread_condattr_destroy(pthread_condattr_t *attr) {

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_CONDATTR_DESTROY, attr, 0, 0);

  return(0);

}  // End function - pthread_condattr_destroy


int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {

  unsigned long flags;
  RT_TASK *current_task;


  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_COND_WAIT, cond, mutex, 0);

  flags = rt_global_save_flags_and_cli();
  current_task = rt_whoami();
  current_task->state |= SEMAPHORE;
  rt_rem_ready_current(current_task);
  current_task->blocked_on = &(cond->c_waiting.queue);
  cond_enqueue_task(cond);
  pthread_mutex_unlock(mutex);
  LXRT_SUSPEND;
  pthread_mutex_lock(mutex);
  rt_global_restore_flags(flags);
  return(0);

}  // End function - pthread_cond_wait


int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime) {


  unsigned long flags, semret;
  RTIME time;
  RT_TASK *current_task;

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_COND_TIMEDWAIT, cond, mutex, 0);

  semret = 0;
  time = nano2count((1000000000LL)*abstime->tv_sec + abstime->tv_nsec);
  current_task = rt_whoami();
  flags = rt_global_save_flags_and_cli();
  current_task->resume_time = time;
  current_task->state |= (SEMAPHORE | DELAYED);
  rt_rem_ready_current(current_task);
  current_task->blocked_on = &(cond->c_waiting.queue);
  cond_enqueue_task(cond);
  rt_enq_timed_task(current_task);
  rt_global_restore_flags(flags);
  pthread_mutex_unlock(mutex);
  flags = rt_global_save_flags_and_cli();
  LXRT_SUSPEND;
  if (current_task->blocked_on) {
     current_task->blocked_on = 0;
     rt_dequeue_blocked(current_task);
     semret = 1;
  }
  rt_global_restore_flags(flags);
  pthread_mutex_lock(mutex);
  return (semret ? ETIMEDOUT : 0);

}  // End function - pthread_cond_timedwait


int pthread_cond_signal(pthread_cond_t *cond) {

  unsigned long flags;
  //QUEUE *rt_q;
  RT_TASK *next_task;

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_COND_SIGNAL, cond, 0, 0);

  flags = rt_global_save_flags_and_cli();
  if((next_task = (cond->c_waiting.queue.next)->task) != 0 &&
      cond->c_waiting.queue.next != 0 &&
      cond->c_waiting.queue.prev !=0) {

    cond->c_waiting.queue.next->task->blocked_on = 0;
    cond->c_waiting.queue.next->task->state &= ~(SEMAPHORE | DELAYED);
    (next_task->queue.prev)->next = next_task->queue.next;
    (next_task->queue.next)->prev = next_task->queue.prev;
    rt_rem_timed_task(next_task);
    if(next_task->state == READY) {
      rt_enq_ready_task(next_task);
      LXRT_SUSPEND;
    }
  }
  rt_global_restore_flags(flags);
  return(0);

}  // End function - pthread_cond_signal


int pthread_cond_broadcast(pthread_cond_t *cond) {

  unsigned long flags;
  //QUEUE *rt_q;
  RT_TASK *next_task;

  TRACE_RTAI_POSIX(TRACE_RTAI_EV_POSIX_PTHREAD_COND_BROADCAST, cond, 0, 0);

  flags = rt_global_save_flags_and_cli();
  if(cond->c_waiting.queue.next != NULL && cond->c_waiting.queue.prev != NULL) {

    while((cond->c_waiting.queue.next)->task != 0) {

      cond->c_waiting.queue.next->task->blocked_on = 0;
      cond->c_waiting.queue.next->task->state &= ~(SEMAPHORE | DELAYED);
      next_task = (cond->c_waiting.queue.next)->task;
      (next_task->queue.prev)->next = next_task->queue.next;
      (next_task->queue.next)->prev = next_task->queue.prev;
      rt_rem_timed_task(next_task);
      if(next_task->state == READY) {
        rt_enq_ready_task(next_task);
      }
    }
  }
  LXRT_SUSPEND;
  rt_global_restore_flags(flags);
  return(0);

}  // End function - pthread_cond_broadcast


// ----------------------------------------------------------------------------


int init_module(void) {

// Although pthreads are not periodic by default, needed by the scheduler?
  rt_set_periodic_mode();
  tick_period = start_rt_timer((int)nano2count(DEFAULT_TICK_PERIOD));
  rt_linux_use_fpu(TASK_USES_FPU);

  printk("\n\n==== RT POSIX Threads API v%s Loaded. ====\n\n", pthread_version);

  return(0);

}  // End function - init_module


void cleanup_module(void) {

  //int i;

  stop_rt_timer();
  rt_busy_sleep(10000000);
  printk("\n==== RT POSIX Threads API Unloaded. ====\n\n");

}  // End function - cleanup_module


// ---------------------------------< eof >------------------------------------
