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
/*
Dec. 2003, Peter Soetens (peter.soetens@mech.kuleuven.ac.be) revised and made 
it cleaner and uniform.
Dec. 2003, Panagiotis Issaris (panagiotis.issaris@mech.kuleuven.ac.be) added
support for aligning to gettimeofday.
*/

#ifndef _RTAI_USP_POSIX_H_
#define _RTAI_USP_POSIX_H_

#include <errno.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <asm/atomic.h>
#include <sys/time.h>
#include <time.h>
#undef LOCK

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

/*
 * SUPPORT STUFF
 */

#undef  SEM_VALUE_MAX 
#define SEM_VALUE_MAX  (SEM_TIMOUT - 1)
#define SEM_BINARY     (0x7FFFFFFF)

/**
 * To facilitate debugging.
 */
#ifdef CHECK_LXRT_CALLS
#define CHK_SYS_CALL() \
  do { if(rt_buddy() == 0) { \
     printf("LXRT NOT INITIALISED IN THIS THREAD pid=%d,\n\
     BUT TRIES TO INVOKE LXRT FUNCTION >>%s<< ANYWAY\n", getpid(), __FUNCTION__ );\
     printf("Exiting to prevent kernel crash !"); \
     exit(EXIT_FAILURE); }\
  } while(0)
#else
#define CHK_SYS_CALL()
#endif

static inline int MAKE_SOFT(void)
{
        RT_TASK* me = rt_buddy();
	if ( me != 0 && rt_is_hard_real_time( me ) ) {
		rt_make_soft_real_time();
		return 1;
	}
	return 0;
}

#define MAKE_HARD(hs)  do { if (hs) rt_make_hard_real_time(); } while (0)


static inline void count2timespec(RTIME rt, struct timespec *t)
{
	t->tv_sec = (rt = count2nano(rt))/1000000000;
	t->tv_nsec = rt - t->tv_sec*1000000000LL;
}

static inline void nanos2timespec(RTIME rt, struct timespec *t)
{
	t->tv_sec = rt/1000000000;
	t->tv_nsec = rt - t->tv_sec*1000000000LL;
}

static inline RTIME timespec2count(const struct timespec *t)
{
	return nano2count(t->tv_sec*1000000000LL + t->tv_nsec);
}

static inline RTIME timespec2nanos(const struct timespec *t)
{
	return t->tv_sec*1000000000LL + t->tv_nsec;
}

/*
 * DO NOTHING FUNCTIONS (IN RTAI HARD REAL TIME)
 */

#define pthread_attr_setdetachstate_rt(attr, detachstate)
#define pthread_detach_rt(thread)
#define pthread_getconcurrency_rt()
#define pthread_setconcurrency_rt(level)

#define pthread_mutexattr_getpshared_rt(attr, pshared)
#define pthread_mutexattr_setpshared_rt(attr, pshared)

#define pthread_condattr_init_rt(attr)
#define pthread_condattr_destroy_rt(attr)
#define pthread_condattr_getpshared_rt(attr, pshared)
#define pthread_condattr_setpshared_rt(attr, pshared)

#ifdef __USE_XOPEN2K
#define pthread_barrierattr_getpshared_rt(attr, pshared)
#define pthread_barrierattr_setpshared_rt(attr, pshared)
#define pthread_barrierattr_getpshared_rt(attr, pshared)
#define pthread_barrierattr_setpshared_rt(attr, pshared)
#endif /* __USE_XOPEN2K */

#ifdef __USE_UNIX98
#define pthread_rwlockattr_init_rt(attr)
#define pthread_rwlockattr_destroy_rt(attr)
#define pthread_rwlockattr_getpshared_rt( ttr, pshared)
#define pthread_rwlockattr_setpshared_rt(attr, pshared)
#define pthread_rwlockattr_getkind_np_rt(attr, pref)
#define pthread_rwlockattr_setkind_np_rt(attr, pref)
#endif /* __USE_UNIX98 */

/*
 * FUNCTIONS (LIKELY) SAFELY USABLE IN HARD REAL TIME "AS THEY ARE", 
 * BECAUSE MAKE SENSE IN THE INITIALIZATION PHASE ONLY, I.E. BEFORE 
 * GOING HARD REAL TIME
 */

#define pthread_self_rt                  pthread_self
#define pthread_equal_rt                 pthread_equal
#define pthread_attr_init_rt             pthread_attr_init      
#define pthread_attr_destroy_rt          pthread_attr_destroy
#define pthread_attr_getdetachstate_rt   pthread_attr_getdetachstate
#define pthread_attr_setschedpolicy_rt   pthread_attr_setschedpolicy
#define pthread_attr_getschedpolicy_rt   pthread_attr_getschedpolicy 
#define pthread_attr_setschedparam_rt    pthread_attr_setschedparam
#define pthread_attr_getschedparam_rt    pthread_attr_getschedparam
#define pthread_attr_setinheritsched_rt  pthread_attr_setinheritsched
#define pthread_attr_getinheritsched_rt  pthread_attr_getinheritsched
#define pthread_attr_setscope_rt         pthread_attr_setscope
#define pthread_attr_getscope_rt         pthread_attr_getscope
#define pthread_attr_setguardsize_rt     pthread_attr_setguardsize
#define pthread_attr_getguardsize_rt     pthread_attr_getguardsize
#define pthread_attr_setstackaddr_rt     pthread_attr_setstackaddr
#define pthread_attr_getstackaddr_rt     pthread_attr_getstackaddr
#define pthread_attr_setstack_rt         pthread_attr_setstack
#define pthread_attr_getstack_rt         pthread_attr_getstack
#define pthread_attr_setstacksize_rt     pthread_attr_setstacksize
#define pthread_attr_getstacksize_rt     pthread_attr_getstacksize
#define pthread_mutexattr_init_rt        pthread_mutexattr_init
#define pthread_mutexattr_destroy_rt     pthread_mutexattr_destroy
#define pthread_mutexattr_settype_rt     pthread_mutexattr_settype
#define pthread_mutexattr_gettype_rt     pthread_mutexattr_gettype

/*
 * FUNCTIONS MADE SAFELY USABLE IN HARD REAL TIME, BUT BREAKING HARD REAL TIME
 */

DECLARE sem_t *sem_open_rt(const char *name, int oflags, int value, int type)
{
	int hs, fd;
	sem_t *sem;
    CHK_SYS_CALL();
	hs = MAKE_SOFT();
	if ((fd = open(name, O_RDONLY)) > 0) {
		read(fd, &sem, sizeof(int));
		close(fd); 
        	atomic_inc((atomic_t *)(&((int *)sem)[1]));
	} else {
		struct { int name, value, type; } arg = { nam2num(name), value, (type == SEM_BINARY ? BIN_SEM : CNT_SEM) | PRIO_Q };
		sem = (sem_t *)malloc(sizeof(sem_t));
	        if ((((int *)sem)[0] = rtai_lxrt(BIDX, SIZARG, SEM_INIT, &arg).i[LOW]) && (fd = open(name, O_WRONLY | O_CREAT))) {
			write(fd, &sem, sizeof(int));
			close(fd); 
        		((int *)sem)[1] = 1;
		} else {
			free(sem);
			sem = 0;
		}
	}
	MAKE_HARD(hs);
	return sem;
}

DECLARE int sem_init_rt(sem_t *sem, int pshared, unsigned int value)
{
	int hs;
    CHK_SYS_CALL();
	if (value <= SEM_VALUE_MAX) {
		struct { int name, value, type; } arg = { rt_get_name(0), value, (pshared == SEM_BINARY ? BIN_SEM : CNT_SEM) | PRIO_Q };
		hs = MAKE_SOFT();
		((int *)sem)[0] = rtai_lxrt(BIDX, SIZARG, SEM_INIT, &arg).i[LOW];
       		((int *)sem)[1] = 0;
		MAKE_HARD(hs);
		return 0;
	}
	errno = EINVAL;
	return -1;
}

DECLARE int sem_close_rt(sem_t *sem)
{
	int hs, cnt;
	char name[7];
	struct { void *sem; } arg = { ((void **)sem)[0] };
    CHK_SYS_CALL();
	if (rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW] < 0) {
		errno = EBUSY;
		return -1;
	}
	cnt = ((int *)sem)[1];
	if (!cnt || (cnt && atomic_dec_and_test((atomic_t *)&((int *)sem)[1]))) {
		hs = MAKE_SOFT();
		num2nam(rt_get_name(((void **)sem)[0]), name);
		rtai_lxrt(BIDX, SIZARG, SEM_DELETE, &arg);
	        if (cnt) {
			unlink(name);
			free((void *)sem);
		}
		MAKE_HARD(hs);
	}
	return 0;
}

DECLARE int sem_destroy_rt(sem_t *sem)
{
	return sem_close_rt(sem);
}

DECLARE int pthread_create_rt(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
	int hs, ret;
    CHK_SYS_CALL();
	hs = MAKE_SOFT();
	ret = pthread_create(thread, attr, start_routine, arg);
	MAKE_HARD(hs);
	return ret;
}

DECLARE int pthread_cancel_rt(pthread_t thread)
{
	int hs, ret;
    CHK_SYS_CALL();
	hs = MAKE_SOFT();
	ret = pthread_cancel(thread);
	MAKE_HARD(hs);
	return ret;
}

#define pthread_cleanup_push_rt(routine, arg) \
do { \
	{\
		int __hs_hs_hs__; \
		__hs_hs_hs__ = MAKE_SOFT(); \
		pthread_cleanup_push(routine, arg); \
		MAKE_HARD(__hs_hs_hs__);
	
#define pthread_cleanup_pop_rt(execute) \
		__hs_hs_hs__ = MAKE_SOFT(); \
		pthread_cleanup_pop(execute); \
		MAKE_HARD(__hs_hs_hs__); \
	} \
} while (0)

#define pthread_cleanup_push_defer_rt(routine, arg) \
do { \
	{\
		int __hs_hs_hs__; \
		__hs_hs_hs__ = MAKE_SOFT(); \
		pthread_cleanup_push_defer_np(routine, arg); \
		MAKE_HARD(__hs_hs_hs__);

#define pthread_cleanup_pop_restore_rt(execute) \
		__hs_hs_hs__ = MAKE_SOFT(); \
		pthread_cleanup_pop_restore_np(execute); \
		MAKE_HARD(__hs_hs_hs__); \
	} \
} while (0)

DECLARE int pthread_sigmask_rt(int how, const sigset_t *newmask, sigset_t *oldmask)
{
	int hs, ret;
    CHK_SYS_CALL();
	hs = MAKE_SOFT();
	ret = pthread_sigmask(how, newmask, oldmask);
	MAKE_HARD(hs);
	return ret;
}

DECLARE int pthread_kill_rt(pthread_t thread, int signo)
{
	int hs, ret;
    CHK_SYS_CALL();
	hs = MAKE_SOFT();
	ret = pthread_kill(thread, signo);
	MAKE_HARD(hs);
	return ret;
}


DECLARE int sigwait_rt(const sigset_t *set, int *sig)
{
	int hs, ret;
    CHK_SYS_CALL();
	hs = MAKE_SOFT();
	ret = sigwait(set, sig);
	MAKE_HARD(hs);
	return ret;
}

DECLARE pthread_mutex_t *pthread_mutex_open_rt(const char *name)
{
	int hs, fd;
	pthread_mutex_t *mutex;
    CHK_SYS_CALL();
	hs = MAKE_SOFT();
	if ((fd = open(name, O_RDONLY)) > 0) {
		read(fd, &mutex, sizeof(int));
		close(fd); 
        	atomic_inc((atomic_t *)(&((int *)mutex)[1]));
	} else {
	        struct { int name, value, type; } arg = { nam2num(name), 1, RES_SEM };
		mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
        	if ((((int *)mutex)[0] = rtai_lxrt(BIDX, SIZARG, SEM_INIT, &arg).i[LOW]) && (fd = open(name, O_WRONLY | O_CREAT))) {
			write(fd, &mutex, sizeof(int));
			close(fd); 
		        ((int *)mutex)[1] = 1;
		} else {
			free(mutex);
			mutex = 0;
		}
	}
	MAKE_HARD(hs);
	return mutex;
}

DECLARE int pthread_mutex_init_rt(pthread_mutex_t *mutex, const pthread_mutexattr_t * mutexattr)
{
	int hs;
	int sem_type = BIN_SEM; // default to normal(fast) mutex
	struct { int name, value, type; } arg = { rt_get_name(0), 1, sem_type };
#ifdef __USE_UNIX98
	if ( mutexattr != 0 ) {
		int mut_type;
		pthread_mutexattr_gettype_rt( mutexattr, &mut_type);
		// support for POSIX and LinuxThreads mutex initialisation.
		arg.type = (
					mut_type == PTHREAD_MUTEX_DEFAULT 
#ifdef __USE_GNU
					|| mut_type == PTHREAD_MUTEX_FAST_NP 
#endif
					? BIN_SEM : RES_SEM);
	}
#endif // __USE_UNIX98
	CHK_SYS_CALL();
	hs = MAKE_SOFT();
	((int *)mutex)[0] = rtai_lxrt(BIDX, SIZARG, SEM_INIT, &arg).i[LOW];
	((int *)mutex)[1] = 0;
	MAKE_HARD(hs);
	return 0;
}

DECLARE int pthread_mutex_close_rt(pthread_mutex_t *mutex)
{
	int hs, cnt;
	char name[7];
	struct { void *sem; } arg = { ((void **)mutex)[0] };
    CHK_SYS_CALL();
	if (rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW] < 0) {
		return EBUSY;
	}
	cnt = ((int *)mutex)[1];
	if (!cnt || (cnt && atomic_dec_and_test((atomic_t *)&((int *)mutex)[1]))) {
		hs = MAKE_SOFT();
		num2nam(rt_get_name(((void **)mutex)[0]), name);
		rtai_lxrt(BIDX, SIZARG, SEM_DELETE, &arg);
	        if (cnt) {
			unlink(name);
			free((void *)mutex);
		}
		MAKE_HARD(hs);
	}
	return 0;
}

DECLARE int pthread_mutex_destroy_rt(pthread_mutex_t *mutex)
{
	return pthread_mutex_close_rt(mutex);
}

DECLARE pthread_cond_t *pthread_cond_open_rt(const char *name)
{
	return (pthread_cond_t *)sem_open_rt(name, 0, 0, SEM_BINARY);
}

DECLARE int pthread_cond_init_rt(pthread_cond_t *cond, pthread_condattr_t * cond_attr )
{
	cond_attr = cond_attr;
	return sem_init_rt((sem_t *)cond, SEM_BINARY, 0);
}

DECLARE int pthread_cond_destroy_rt(pthread_cond_t *cond)
{
	return sem_close_rt((sem_t *)cond);
}

DECLARE int pthread_cond_close_rt(pthread_cond_t *cond)
{
	return sem_close_rt((sem_t *)cond);
}

#ifdef __USE_XOPEN2K
DECLARE pthread_barrier_t *pthread_barrier_open_rt(const char *name, unsigned int count)
{
	return (pthread_barrier_t *)sem_open_rt(name, 0, count, 0);
}

DECLARE int pthread_barrier_init_rt(pthread_barrier_t *barrier, const pthread_barrierattr_t * attr, unsigned int count)
{
	return sem_init_rt((sem_t *)barrier, 0, count);
}

DECLARE int pthread_barrier_destroy_rt(pthread_barrier_t *barrier)
{
	return sem_close_rt((sem_t *)barrier);
}

DECLARE int pthread_barrier_close_rt(pthread_barrier_t *barrier)
{
	return sem_close_rt((sem_t *)barrier);
}

DECLARE int pthread_barrier_wait_rt(pthread_barrier_t *barrier)
{
	struct { void *sem; } arg = { ((void **)barrier)[0] };
    CHK_SYS_CALL();
	rtai_lxrt(BIDX, SIZARG, SEM_WAIT_BARRIER, &arg);
	return 0;
}

#endif //__USE_XOPEN2K

/*
 * WORKING FUNCTIONS USABLE IN HARD REAL TIME, THIS IS THE REAL STUFF
 */

#define pthread_setcancelstate_rt  pthread_setcancelstate
#define pthread_setcanceltype_rt   pthread_setcanceltype

DECLARE void pthread_testcancel_rt(void)
{
	int oldtype, oldstate;
    CHK_SYS_CALL();
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldtype);
	if (oldstate != PTHREAD_CANCEL_DISABLE && oldtype != PTHREAD_CANCEL_DEFERRED) {
		MAKE_SOFT();
		rt_task_delete(rt_buddy());
		pthread_exit(NULL);
	}
	pthread_setcanceltype(oldtype, &oldtype);
	pthread_setcancelstate(oldstate, &oldstate);
}

#ifdef __USE_GNU
DECLARE int pthread_yield_rt(void)
{
    CHK_SYS_CALL();
	if (rt_is_hard_real_time(rt_buddy())) {
		struct { unsigned long dummy; } arg;
		rtai_lxrt(BIDX, SIZARG, YIELD, &arg);
		return 0;
	}
	return pthread_yield();
}
#endif /* __USE_GNU */

DECLARE void pthread_exit_rt(void *retval)
{
    CHK_SYS_CALL();
	MAKE_SOFT();
	rt_task_delete(rt_buddy());
	pthread_exit(retval);
}

DECLARE int pthread_join_rt(pthread_t thread, void **thread_return)
{
	int hs, ret;
    CHK_SYS_CALL();
	hs = MAKE_SOFT();
	ret = pthread_join(thread, thread_return);
	MAKE_HARD(hs);
	return ret;
}

DECLARE int sem_wait_rt(sem_t *sem)
{
	struct { void *sem; } arg = { ((void **)sem)[0] };
    CHK_SYS_CALL();
	rtai_lxrt(BIDX, SIZARG, SEM_WAIT, &arg);
	return 0;
}

DECLARE int sem_trywait_rt(sem_t *sem)
{
	struct { void *sem; } arg = { ((void **)sem)[0] };
    CHK_SYS_CALL();
	if (rtai_lxrt(BIDX, SIZARG, SEM_WAIT_IF, &arg).i[LOW] > 0) {
		return 0;
	}
	errno = EAGAIN;
	return -1;
}

DECLARE int sem_timedwait_rt(sem_t *sem, const struct timespec *abstime)
{
	struct { void *sem; RTIME until; } arg = { ((void **)sem)[0], timespec2count(abstime) };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, SEM_WAIT_UNTIL, &arg).i[LOW] < SEM_VALUE_MAX ? 0 : ETIMEDOUT;
}

DECLARE int sem_post_rt(sem_t *sem)
{
	struct { void *sem; } arg = { ((void **)sem)[0] };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, SEM_SIGNAL, &arg).i[LOW];
}

DECLARE int sem_getvalue_rt(sem_t *sem, int *sval)
{
	struct { void *sem; } arg = { ((void **)sem)[0] };
    CHK_SYS_CALL();
	*sval = rtai_lxrt(BIDX, SIZARG, SEM_COUNT, &arg).i[LOW];
	return 0;
}

DECLARE int pthread_mutex_lock_rt(pthread_mutex_t *mutex)
{
	return sem_wait_rt((sem_t *)mutex);
}

DECLARE int pthread_mutex_timedlock_rt(pthread_mutex_t *mutex, const struct timespec *abstime)
{
	return sem_timedwait_rt((sem_t *)mutex, abstime);
}

DECLARE int pthread_mutex_trylock_rt(pthread_mutex_t *mutex)
{
	return sem_trywait_rt((sem_t *)mutex);
}

DECLARE int pthread_mutex_unlock_rt(pthread_mutex_t *mutex)
{
	return sem_post_rt((sem_t *)mutex);
}

DECLARE int pthread_cond_signal_rt(pthread_cond_t *cond)
{
	return sem_post_rt((sem_t *)cond);
}

DECLARE int pthread_cond_broadcast_rt(pthread_cond_t *cond)
{
	struct { void *cond; } arg = { ((void **)cond)[0] };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, SEM_BROADCAST, &arg).i[LOW];
}

DECLARE int pthread_cond_wait_rt(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	struct { void *cond; void *mutex; } arg = { ((void **)cond)[0], ((void **)mutex)[0] };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, COND_WAIT, &arg).i[LOW];
}

DECLARE int pthread_cond_timedwait_rt(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
	struct { void *cond; void *mutex; RTIME time; } arg = { ((void **)cond)[0], ((void **)mutex)[0], timespec2count(abstime) };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, COND_WAIT_UNTIL, &arg).i[LOW];
}

#ifdef __USE_UNIX98
DECLARE pthread_rwlock_t *pthread_rwlock_open_rt(const char *name)
{
	int hs, fd;
	pthread_rwlock_t *rwlock;
    CHK_SYS_CALL();
	hs = MAKE_SOFT();
	if ((fd = open(name, O_RDONLY)) > 0) {
		read(fd, &rwlock, sizeof(int));
		close(fd); 
        	atomic_inc((atomic_t *)(&((int *)rwlock)[1]));
	} else {
	        struct { int name, value, type; } arg = { nam2num(name), 1, RES_SEM };
		rwlock = (pthread_rwlock_t *)malloc(sizeof(pthread_rwlock_t));
        	if ((((int *)rwlock)[0] = rtai_lxrt(BIDX, SIZARG, RWL_INIT, &arg).i[LOW]) && (fd = open(name, O_WRONLY | O_CREAT))) {
			write(fd, &rwlock, sizeof(int));
			close(fd); 
		        ((int *)rwlock)[1] = 1;
		} else {
			free(rwlock);
			rwlock = 0;
		}
	}
	MAKE_HARD(hs);
	return rwlock;
}

DECLARE int pthread_rwlock_init_rt(pthread_rwlock_t *rwlock, pthread_rwlockattr_t * attr )
{
	int hs;
	struct { int name; } arg = { rt_get_name(0) };
	attr = attr;
	CHK_SYS_CALL();
	hs = MAKE_SOFT();
	((int *)rwlock)[0] = rtai_lxrt(BIDX, SIZARG, RWL_INIT, &arg).i[LOW];
	((int *)rwlock)[1] = 0;
	MAKE_HARD(hs);
	return 0;
}

DECLARE int pthread_rwlock_close_rt(pthread_rwlock_t *rwlock)
{
	int hs, cnt;
	char name[7];
	struct { void *rwlock; } arg = { ((void **)rwlock)[0] };
    CHK_SYS_CALL();
	if (rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK_IF, &arg).i[LOW] < 0) {
		return EBUSY;
	} else {
		rtai_lxrt(BIDX, SIZARG, RWL_UNLOCK, &arg);
		if (rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK_IF, &arg).i[LOW] < 0) {
			return EBUSY;
		}
		rtai_lxrt(BIDX, SIZARG, RWL_UNLOCK, &arg);
	}
	cnt = ((int *)rwlock)[1];
	if (!cnt || (cnt && atomic_dec_and_test((atomic_t *)&((int *)rwlock)[1]))) {
		hs = MAKE_SOFT();
		num2nam(rt_get_name(((void **)rwlock)[0]), name);
		rtai_lxrt(BIDX, SIZARG, RWL_DELETE, &arg);
	        if (cnt) {
			unlink(name);
			free((void *)rwlock);
		}
		MAKE_HARD(hs);
	}
	return 0;
}

DECLARE int pthread_rwlock_destroy_rt(pthread_rwlock_t *rwlock)
{
	return pthread_rwlock_close_rt(rwlock);
}

DECLARE int pthread_rwlock_rdlock_rt(pthread_rwlock_t *rwlock)
{
	struct { void *rwlock; } arg = { ((void **)rwlock)[0] };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK, &arg).i[LOW];
}

DECLARE int pthread_rwlock_tryrdlock_rt(pthread_rwlock_t *rwlock)
{
	struct { void *rwlock; } arg = { ((void **)rwlock)[0] };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK_IF, &arg).i[LOW];
}

DECLARE int pthread_rwlock_timedrdlock_rt(pthread_rwlock_t *rwlock, struct timespec *abstime)
{
	struct { void *rwlock; RTIME time; } arg = { ((void **)rwlock)[0], timespec2count(abstime) };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, RWL_RDLOCK_UNTIL, &arg).i[LOW];
}

DECLARE int pthread_rwlock_wrlock_rt(pthread_rwlock_t *rwlock)
{
	struct { void *rwlock; } arg = { ((void **)rwlock)[0] };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK, &arg).i[LOW];
}

DECLARE int pthread_rwlock_trywrlock_rt(pthread_rwlock_t *rwlock)
{
	struct { void *rwlock; } arg = { ((void **)rwlock)[0] };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK_IF, &arg).i[LOW];
}

DECLARE int pthread_rwlock_timedwrlock_rt(pthread_rwlock_t *rwlock, struct timespec *abstime)
{
	struct { void *rwlock; RTIME time; } arg = { ((void **)rwlock)[0], timespec2count(abstime) };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, RWL_WRLOCK_UNTIL, &arg).i[LOW];
}

DECLARE int pthread_rwlock_unlock_rt(pthread_rwlock_t *rwlock)
{
	struct { void *rwlock; } arg = { ((void **)rwlock)[0] };
    CHK_SYS_CALL();
	return rtai_lxrt(BIDX, SIZARG, RWL_UNLOCK, &arg).i[LOW];
}
#endif /* __USE_UNIX98 */

#ifdef __USE_XOPEN2K
DECLARE int pthread_spin_init_rt(pthread_spinlock_t *lock)
{
	return (((int *)lock)[0] = 0);
}

DECLARE int pthread_spin_destroy_rt(pthread_spinlock_t *lock)
{
	return ((int *)lock)[0] = 0;
}

DECLARE int pthread_spin_lock_rt(pthread_spinlock_t *lock)
{
	while (_rtai_cmpxchgl(&lock, 0, 1));
	return 0;
}

DECLARE int pthread_spin_trylock_rt(pthread_spinlock_t *lock)
{
	if (_rtai_cmpxchgl(&lock, 0, 1)) {
		return EAGAIN;
	}
	return 0;
}

DECLARE int pthread_spin_unlock_rt(pthread_spinlock_t *lock)
{
	return ((int *)lock)[0] = 0;
}

#endif /* __USE_XOPEN2K */

DECLARE void clock_gettime_rt(int clockid, struct timespec *current_time)
{
	count2timespec(rt_get_time(), current_time);
}

DECLARE int nanosleep_rt(const struct timespec *rqtp, struct timespec *rmtp)
{
	RTIME expire;
    CHK_SYS_CALL();
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

extern unsigned long long orig_tsc_value;

static inline unsigned long long rdtsc(void)
{
       unsigned long long t;
       asm volatile( "rdtsc" : "=A" (t));
       return t;
}

// seems to be necessary to silent compiler warnings.
struct timezone;

DECLARE int gettimeofday_rt(struct timeval *tv, struct timezone *tz)
{
    unsigned long long tsc_value_total;
    unsigned long long nano_value;
    
    if (tv == NULL)
        return -1;
    
    tsc_value_total = orig_tsc_value + rdtsc();
    nano_value = count2nano(tsc_value_total);
    tv->tv_sec = nano_value / 1000000000;
    tv->tv_usec = (nano_value % 1000000000) / 1000;
    return 0;
}

DECLARE void init_gettimeofday_rt( void )
{
    RTIME v[2];
    rt_gettimeorig(v);
    orig_tsc_value = v[0];
}

#endif
