/*
COPYRIGHT (C) 2003  Paolo Mantegazza (mantegazza@aero.polimi.it)

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef _RTAI_KSP_POSIX_H_
#define _RTAI_KSP_POSIX_H_

#include <rtai_sched.h>

#undef  SEM_VALUE_MAX 
#define SEM_VALUE_MAX  (SEM_TIMOUT - 1)
#define SEM_BINARY     (0x7FFFFFFF)

typedef struct rt_semaphore sem_t;

static inline int sem_init_rt(sem_t *sem, int pshared, unsigned int value)
{
	if (value < SEM_TIMOUT) {
		rt_typed_sem_init(sem, value, pshared | PRIO_Q);
		return 0;
	}
	return -EINVAL;
}

static inline int sem_destroy_rt(sem_t *sem)
{
	if (rt_sem_wait_if(sem) >= 0) {
		rt_sem_signal(sem);
		return rt_sem_delete(sem);
	}
	return -EBUSY;
}

static inline int sem_wait_rt(sem_t *sem)
{
	return rt_sem_wait(sem) < SEM_TIMOUT ? 0 : -1;
}

static inline int sem_trywait_rt(sem_t *sem)
{
	return rt_sem_wait_if(sem) > 0 ? 0 : -EAGAIN;
}

static inline int sem_timedwait_rt(sem_t *sem, const struct timespec *abstime)
{
	return rt_sem_wait_until(sem, timespec2count(abstime)) < SEM_TIMOUT ? 0 : -1;
}

static inline int sem_post_rt(sem_t *sem)
{
	return rt_sem_wait(sem) < SEM_TIMOUT ? 0 : -ERANGE;
}

static inline int sem_getvalue_rt(sem_t *sem, int *sval)
{
	if ((*sval = rt_sem_wait_if(sem)) > 0) {
		rt_sem_signal(sem);
	}
	return 0;
}

typedef struct rt_semaphore pthread_mutex_t;
typedef unsigned long pthread_mutexattr_t;

static inline int pthread_mutex_init_rt(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{
	rt_typed_sem_init(mutex, 1, RES_SEM);
	return 0;
}

static inline int pthread_mutex_destroy_rt(pthread_mutex_t *mutex)
{
	if (rt_sem_wait_if(mutex) > 0) {
		rt_sem_signal(mutex);
		return rt_sem_delete(mutex);
	}
	return -EBUSY;
}

static inline int pthread_mutex_lock_rt(pthread_mutex_t *mutex)
{
	return rt_sem_wait(mutex) < SEM_TIMOUT ? 0 : -EINVAL;
}

static inline int pthread_mutex_timedlock_rt(pthread_mutex_t *mutex, const struct timespec *abstime)
{
	return rt_sem_wait_until(mutex, timespec2count(abstime)) < SEM_TIMOUT ? 0 : -1;
}

static inline int pthread_mutex_trylock_rt(pthread_mutex_t *mutex)
{
	return rt_sem_wait_if(mutex) > 0 ? 0 : -EBUSY;
}

static inline int pthread_mutex_unlock_rt(pthread_mutex_t *mutex)
{
	return rt_sem_wait(mutex) < SEM_TIMOUT ? 0 : -EINVAL;
}

typedef struct rt_semaphore pthread_cond_t;
typedef unsigned long pthread_condattr_t;

static inline int pthread_cond_init_rt(pthread_cond_t *cond, const pthread_condattr_t *cond_attr)
{
        return sem_init_rt((sem_t *)cond, SEM_BINARY, 0);
}

static inline int pthread_cond_destroy_rt(pthread_cond_t *cond)
{
        return sem_destroy_rt((sem_t *)cond);
}

static inline int pthread_cond_signal_rt(pthread_cond_t *cond)
{
	return sem_destroy_rt((sem_t *)cond);
}

static inline int pthread_cond_broadcast_rt(pthread_cond_t *cond)
{
	return rt_sem_broadcast(cond);
}

static inline int pthread_cond_wait_rt(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	return rt_cond_wait(cond, mutex);
}

static inline int pthread_cond_timedwait_rt(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
	return rt_cond_wait_until(cond, mutex, timespec2count(abstime));
}


typedef struct {  unsigned int data[2]; } pthread_barrier_t;
#define pthread_barrierattr_t  int

static inline int pthread_barrier_init_rt(pthread_barrier_t *barrier, const pthread_barrierattr_t *attr, unsigned int count)
{
	return sem_init_rt((sem_t *)barrier, 0, count);
}

static inline int pthread_barrier_destroy_rt(pthread_barrier_t *barrier)
{
	return sem_destroy_rt((sem_t *)barrier);
}

static inline int pthread_barrier_wait_rt(pthread_barrier_t *barrier)
{
	return rt_sem_wait_barrier((SEM *)barrier);
}

typedef struct rdwr_lock pthread_rwlock_t;
#define pthread_rwlockattr_t  int

static inline int pthread_rwlock_init_rt(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
	return rt_rwl_init((RWL *)rwlock);
}

static inline int pthread_rwlock_destroy_rt(pthread_rwlock_t *rwlock)
{
	return rt_rwl_delete((RWL *)rwlock);
}

static inline int pthread_rwlock_rdlock_rt(pthread_rwlock_t *rwlock)
{
	return rt_rwl_rdlock((RWL *)rwlock);
}

static inline int pthread_rwlock_tryrdlock_rt(pthread_rwlock_t *rwlock)
{
	return rt_rwl_rdlock_if((RWL *)rwlock);
}

static inline int pthread_rwlock_timedrdlock_rt(pthread_rwlock_t *rwlock, struct timespec *abstime)
{
	return rt_rwl_rdlock_until((RWL *)rwlock, timespec2count(abstime));
}

static inline int pthread_rwlock_wrlock_rt(pthread_rwlock_t *rwlock)
{
	return rt_rwl_wrlock((RWL *)rwlock);
}

static inline int pthread_rwlock_trywrlock_rt(pthread_rwlock_t *rwlock)
{
	return rt_rwl_wrlock_if((RWL *)rwlock);
}

static inline int pthread_rwlock_timedwrlock_rt(pthread_rwlock_t *rwlock, struct timespec *abstime)
{
	return rt_rwl_wrlock_until((RWL *)rwlock, timespec2count(abstime));
}

static inline int pthread_rwlock_unlock_rt(pthread_rwlock_t *rwlock)
{
	return rt_rwl_unlock((RWL *)rwlock);
}

typedef struct rt_spl_t pthread_spinlock_t;

static inline int pthread_spin_init_rt(pthread_spinlock_t *lock)
{
	return rt_spl_init((SPL *)lock);
}

static inline int pthread_spin_destroy_rt(pthread_spinlock_t *lock)
{
	return rt_spl_delete((SPL *)lock);
}

static inline int pthread_spin_lock_rt(pthread_spinlock_t *lock)
{
	return rt_spl_lock((SPL *)lock);
}

static inline int pthread_spin_trylock_rt(pthread_spinlock_t *lock)
{
	return rt_spl_lock_if((SPL *)lock);
}

static inline int pthread_spin_unlock_rt(pthread_spinlock_t *lock)
{
	return rt_spl_unlock((SPL *)lock);
}

typedef struct rt_task_struct * pthread_t;
typedef struct { int stacksize; int policy; int rr_quantum_ns; int priority; } pthread_attr_t;
#define MAX_PRIO  99
#define MIN_PRIO  1
#define STACK_SIZE  8192
#define RR_QUANTUM_NS  1000000

static inline int get_max_priority_rt(int policy)
{
	return MAX_PRIO;
}

static inline int get_min_priority_rt(int policy)
{
	return MIN_PRIO;
}

static inline int pthread_create_rt(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
	RT_TASK *rt_task;
	*thread = rt_task = (void *)rt_malloc(sizeof(RT_TASK) + sizeof(SEM));
	if (rt_task) {
		rt_task->magic = 0;
		if (!rt_task_init(rt_task, (void *)start_routine, (int)arg, attr->stacksize, attr->priority, 1, 0)) {
			rt_typed_sem_init((SEM *)(rt_task + 1), 0, BIN_SEM | FIFO_Q);
			rt_task->policy = attr->policy;
        		return 0;
		}
	}
	rt_free(rt_task);
       	return ENOMEM;
}

static inline void pthread_exit_rt(void *retval)
{
	RT_TASK *rt_task;
	rt_task = rt_whoami();
	rt_sem_broadcast((SEM *)(rt_task + 1));
        rt_task_delete(rt_task);
}

static inline int pthread_equal_rt(pthread_t thread1,pthread_t thread2)
{
	return thread1 == thread2;
}

static inline int pthread_yield_rt(void)
{
	rt_task_yield();
	return 0;
}

static inline pthread_t pthread_self_rt(void)
{
	return rt_whoami();
}

static inline int pthread_join_rt(pthread_t thread, void **thread_return)
{
	return rt_sem_wait((SEM *)(thread +sizeof(RT_TASK)));
}

static inline int pthread_attr_init_rt(pthread_attr_t *attr)
{
	attr->stacksize     = STACK_SIZE;
	attr->policy        = SCHED_FIFO;
	attr->rr_quantum_ns = RR_QUANTUM_NS;
	attr->priority      = 1;
	return 0;
}

static inline int pthread_attr_destroy_rt(pthread_attr_t *attr)
{
	return 0;
}

static inline int pthread_attr_setschedparam_rt(pthread_attr_t *attr, const struct sched_param *param)
{
	if(param->sched_priority < MIN_PRIO || param->sched_priority > MAX_PRIO) {
		return(EINVAL);
	}
	attr->priority = MAX_PRIO - param->sched_priority;
	return 0;
}

static inline int pthread_attr_getschedparam_rt(const pthread_attr_t *attr, struct sched_param *param)
{
	param->sched_priority = MAX_PRIO - attr->priority;
	return 0;
}

static inline int pthread_attr_setschedpolicy_rt(pthread_attr_t *attr, int policy)
{
	if(policy != SCHED_FIFO && policy != SCHED_RR) {
		return EINVAL;
	}
	if ((attr->policy = policy) == SCHED_RR) {
		rt_set_sched_policy(rt_whoami(), SCHED_RR, attr->rr_quantum_ns);
	}
	return 0;
}


static inline int pthread_attr_getschedpolicy_rt(const pthread_attr_t *attr, int *policy)
{
	*policy = attr->policy;
	return 0;
}

static inline int pthread_attr_setschedrr_rt(pthread_attr_t *attr, int rr_quantum_ns)
{
	attr->rr_quantum_ns = rr_quantum_ns;
	return 0;
}


static inline int pthread_attr_getschedrr_rt(const pthread_attr_t *attr, int *rr_quantum_ns)
{
	*rr_quantum_ns = attr->rr_quantum_ns;
	return 0;
}

static inline int pthread_attr_setstacksize_rt(pthread_attr_t *attr, int stacksize)
{
	attr->stacksize = stacksize;
	return 0;
}

static inline int pthread_attr_getstacksize_rt(const pthread_attr_t *attr, int *stacksize)
{
	*stacksize = attr->stacksize;
	return 0;
}

static inline int pthread_attr_setstack_rt(pthread_attr_t *attr, void *stackaddr, int stacksize)
{
	attr->stacksize = stacksize;
	return 0;
}

static inline int pthread_attr_getstack_rt(const pthread_attr_t *attr, void **stackaddr, int *stacksize)
{
	*stacksize = attr->stacksize;
	return 0;
}

static inline int pthread_cancel_rt(pthread_t thread)
{
	return rt_task_delete(thread);
}

static inline void pthread_testcancel_rt(void)
{
	rt_task_delete(rt_whoami());
	pthread_exit_rt(NULL);
}

static inline void clock_gettime_rt(int clockid, struct timespec *current_time)
{
	count2timespec(rt_get_time(), current_time);
}

static inline int nanosleep_rt(const struct timespec *rqtp, struct timespec *rmtp)
{
        RTIME expire;
        if (rqtp->tv_nsec >= 1000000000L || rqtp->tv_nsec < 0 || rqtp->tv_sec <
0) {
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

/*
 * DO NOTHING FUNCTIONS
 */

#define pthread_mutexattr_init_rt(attr)
#define pthread_mutexattr_destroy_rt(attr)
#define pthread_mutexattr_getpshared_rt(attr, pshared)
#define pthread_mutexattr_setpshared_rt(attr, pshared)
#define pthread_mutexattr_settype_rt(attr, kind)
#define pthread_mutexattr_gettype_rt(attr, kind)

#define pthread_condattr_init_rt(attr)
#define pthread_condattr_destroy_rt(attr)
#define pthread_condattr_getpshared_rt(attr, pshared)
#define pthread_condattr_setpshared_rt(attr, pshared)

#define pthread_barrierattr_getpshared_rt(attr, pshared)
#define pthread_barrierattr_setpshared_rt(attr, pshared)
#define pthread_barrierattr_getpshared_rt(attr, pshared)
#define pthread_barrierattr_setpshared_rt(attr, pshared)

#define pthread_rwlockattr_init(attr)
#define pthread_rwlockattr_destroy(attr)
#define pthread_rwlockattr_getpshared( ttr, pshared)
#define pthread_rwlockattr_setpshared(attr, pshared)
#define pthread_rwlockattr_getkind_np(attr, pref)
#define pthread_rwlockattr_setkind_np(attr, pref)

#define pthread_detach_rt(thread)
#define pthread_attr_setdetachstate_rt(attr, detachstate)
#define pthread_attr_getdetachstate_rt(attr, detachstate)
#define pthread_setconcurrency_rt(level)
#define pthread_getconcurrency_rt()
#define pthread_attr_setinheritsched_rt(attr, inherit)
#define pthread_attr_getinheritsched_rt(attr, inherit)
#define pthread_attr_setscope_rt(attr, scope)
#define pthread_attr_getscope_rt(attr, scope)
#define pthread_attr_setguardsize_rt(attr, guardsize) 
#define pthread_attr_getguardsize_rt(attr, guardsize)
#define pthread_attr_setstackaddr_rt(attr, stackaddr)
#define pthread_attr_getstackaddr_rt(attr, stackaddr) 
#define pthread_setcancelstate_rt(state, oldstate)
#define pthread_setcanceltype_rt(type, oldtype)

#endif
