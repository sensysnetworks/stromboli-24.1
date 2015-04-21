
/*
COPYRIGHT (C) 2003  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


/* RTAI rwlocks test */

#include <stdlib.h>
#include <sys/mman.h>

#define KEEP_STATIC_INLINE
#include <rtai_usp_posix.h>

#define TICK 1000000

#define MAKE_IT_HARD
#ifdef MAKE_IT_HARD
#define RT_MAKE_HARD_REAL_TIME() do { rt_make_hard_real_time(); } while (0)
#define DISPLAY  rt_printk
#else
#define RT_MAKE_HARD_REAL_TIME()
#define DISPLAY  printf
#endif

#define LOOPS 1

#define NTASKS 4

static RT_TASK **task;

static pthread_t *thread;

#define USE_OPEN
#ifndef USE_OPEN
static pthread_rwlock_t rwls;
#endif
static pthread_rwlock_t *rwl;

static pthread_barrier_t barrier;

static void *thread_fun(int idx)
{
	unsigned int loops = LOOPS;
	struct timespec abstime;
	char name[7];
	
	sprintf(name, "TASK%d", idx);
	task[idx - 1] = rt_task_init_schmod(nam2num(name), NTASKS - idx + 1, 0, 0, SCHED_FIFO, 0x1);
	mlockall(MCL_CURRENT | MCL_FUTURE);
	RT_MAKE_HARD_REAL_TIME();
	while(loops--) {
		DISPLAY("TASK %d 1 COND/TIMED PREWLOCKED\n", idx);
		nanos2timespec(rt_get_time_ns() + 2000000000LL, &abstime);
		if (idx%2) {
			if (pthread_rwlock_trywrlock_rt(rwl)) {
				DISPLAY("TASK %d 1 COND PREWLOCKED FAILED GO UNCOND\n", idx);
				pthread_rwlock_wrlock_rt(rwl);
			}
		} else if (pthread_rwlock_timedwrlock_rt(rwl, &abstime) >= SEM_TIMOUT) {
			DISPLAY("TASK %d 1 TIMED PREWLOCKED FAILED GO UNCOND\n", idx);
			pthread_rwlock_wrlock(rwl);
		}
		DISPLAY("TASK %d 1 WLOCKED\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 2 COND PREWLOCK\n", idx);
		if (pthread_rwlock_trywrlock_rt(rwl)) {
			DISPLAY("TASK %d 2 COND PREWLOCK FAILED GO UNCOND\n", idx);
			pthread_rwlock_wrlock_rt(rwl);
		}
		DISPLAY("TASK %d 2 WLOCK\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 3 PREWLOCK\n", idx);
		pthread_rwlock_wrlock_rt(rwl);
		DISPLAY("TASK %d 3 WLOCK\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 3 PREWUNLOCK\n", idx);
		pthread_rwlock_unlock_rt(rwl);
		DISPLAY("TASK %d 3 WUNLOCK\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 2 PREWUNLOCK\n", idx);
		pthread_rwlock_unlock_rt(rwl);
		DISPLAY("TASK %d 2 WUNLOCK\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 1 PREWUNLOCKED\n", idx);
		pthread_rwlock_unlock_rt(rwl);
		DISPLAY("TASK %d 1 WUNLOCKED\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 1 COND/TIMED PRERDLOCKED\n", idx);
		nanos2timespec(rt_get_time_ns() + 2000000000LL, &abstime);
		if (idx%2) {
			if (pthread_rwlock_tryrdlock_rt(rwl)) {
				DISPLAY("TASK %d 1 COND PRERDLOCKED FAILED GO UNCOND\n", idx);
				pthread_rwlock_rdlock_rt(rwl);
			}
		} else if (pthread_rwlock_timedrdlock_rt(rwl, &abstime) >= SEM_TIMOUT) {
			DISPLAY("TASK %d 1 TIMED PRERDLOCKED FAILED GO UNCOND\n", idx);
			pthread_rwlock_rdlock_rt(rwl);
		}
		DISPLAY("TASK %d 1 RDLOCKED\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 2 COND PRERDLOCK\n", idx);
		if (pthread_rwlock_tryrdlock_rt(rwl)) {
			DISPLAY("TASK %d 2 COND PRERDLOCK FAILED GO UNCOND\n", idx);
			pthread_rwlock_rdlock_rt(rwl);
		}
		DISPLAY("TASK %d 2 RDLOCK\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 3 PRERDLOCK\n", idx);
		pthread_rwlock_rdlock_rt(rwl);
		DISPLAY("TASK %d 3 RDLOCK\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 3 PRERDUNLOCK\n", idx);
		pthread_rwlock_unlock_rt(rwl);
		DISPLAY("TASK %d 3 RDUNLOCK\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 2 PRERDUNLOCK\n", idx);
		pthread_rwlock_unlock_rt(rwl);
		DISPLAY("TASK %d 2 RDUNLOCK\n", idx);
		rt_busy_sleep(100000);
		DISPLAY("TASK %d 1 PRERDUNLOCK\n", idx);
		pthread_rwlock_unlock_rt(rwl);
		DISPLAY("TASK %d 1 RDUNLOCK\n", idx);
		rt_busy_sleep(100000);
	}
	rt_make_soft_real_time();
	pthread_barrier_wait_rt(&barrier);
	rt_task_delete(task[idx - 1]);
	DISPLAY("TASK %d EXITED\n", idx);
	return NULL;
}

int main(void)
{
	int i;
	RT_TASK *mytask;

	thread = (void *)malloc(NTASKS*sizeof(pthread_t));
	task = (void *)malloc(NTASKS*sizeof(RT_TASK *));
	mytask = rt_task_init_schmod(nam2num("MAIN"), 2*NTASKS, 0, 0, SCHED_FIFO, 0x1);
#ifdef USE_OPEN
	rwl = pthread_rwlock_open_rt("RWLOCK");
	rwl = pthread_rwlock_open_rt("RWLOCK");
#else
	pthread_rwlock_init_rt(rwl = &rwls, 0);
#endif
	pthread_barrier_init_rt(&barrier, NULL, NTASKS + 1);
	rt_set_oneshot_mode();
	start_rt_timer(0);
	for (i = 0; i < NTASKS; i++) {
		pthread_create_rt(&thread[i], NULL, (void *)thread_fun, (void *)(i + 1));
	}
	pthread_barrier_wait_rt(&barrier);
#ifdef USE_OPEN
	pthread_rwlock_close_rt(rwl);
	pthread_rwlock_close_rt(rwl);
#else
	pthread_rwlock_destroy_rt(rwl);
#endif
	pthread_barrier_destroy_rt(&barrier);
	stop_rt_timer();
	rt_task_delete(mytask);
	free(thread);
	free(task);
	return 0;
}
