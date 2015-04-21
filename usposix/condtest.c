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

#define USE_OPEN
#ifndef USE_OPEN
static pthread_cond_t    conds;
static pthread_mutex_t   mtxs;
static pthread_barrier_t barriers;
#endif
static pthread_cond_t    *cond;
static pthread_mutex_t   *mtx;
static pthread_barrier_t *barrier;

static RT_TASK *task1, *task2, *task3, *task4;

static int cond_data;

static void task_exit_handler(void *arg)
{
	pthread_barrier_wait_rt(barrier);
}

static void *task_func1(void *dummy)
{
 	task1 = rt_task_init_schmod(nam2num("TASK1"), 1, 0, 0, SCHED_FIFO, 0x1);
	mlockall(MCL_CURRENT | MCL_FUTURE);
	RT_MAKE_HARD_REAL_TIME();
	pthread_cleanup_push_rt(task_exit_handler, 0);
	pthread_barrier_wait_rt(barrier);
	DISPLAY("Starting task1, waiting on the conditional variable to be 1.\n");
	pthread_mutex_lock_rt(mtx);
	while(cond_data < 1) {
		pthread_cond_wait_rt(cond, mtx);
	}
	pthread_mutex_unlock_rt(mtx);
	if(cond_data == 1) {
		DISPLAY("task1, conditional variable signalled, value: %d.\n", cond_data);
	}
	DISPLAY("task1 signals after setting data to 2.\n");
	DISPLAY("task1 waits for a broadcast.\n");
	pthread_mutex_lock_rt(mtx);
	cond_data = 2;
	pthread_cond_signal_rt(cond);
	while(cond_data < 3) {
		pthread_cond_wait_rt(cond, mtx);
	}
	pthread_mutex_unlock_rt(mtx);
	if(cond_data == 3) {
		DISPLAY("task1, conditional variable broadcasted, value: %d.\n", cond_data);
	}
	pthread_cleanup_pop_rt(1);
	DISPLAY("Ending task1.\n");
	pthread_exit_rt(0);
}

static void *task_func2(void *dummy)
{
 	task2 = rt_task_init_schmod(nam2num("TASK2"), 2, 0, 0, SCHED_FIFO, 0x1);
	mlockall(MCL_CURRENT | MCL_FUTURE);
	RT_MAKE_HARD_REAL_TIME();
	pthread_cleanup_push_rt(task_exit_handler, 0);
	pthread_barrier_wait_rt(barrier);
	DISPLAY("Starting task2, waiting on the conditional variable to be 2.\n");
	pthread_mutex_lock_rt(mtx);
	while(cond_data < 2) {
		pthread_cond_wait_rt(cond, mtx);
	}
	pthread_mutex_unlock_rt(mtx);
	if(cond_data == 2) {
		DISPLAY("task2, conditional variable signalled, value: %d.\n", cond_data);
	}
	DISPLAY("task2 waits for a broadcast.\n");
	pthread_mutex_lock_rt(mtx);
	while(cond_data < 3) {
		pthread_cond_wait_rt(cond, mtx);
	}
	pthread_mutex_unlock_rt(mtx);
	if(cond_data == 3) {
		DISPLAY("task2, conditional variable broadcasted, value: %d.\n", cond_data);
	}
	pthread_cleanup_pop_rt(1);
	DISPLAY("Ending task2.\n");
	pthread_exit_rt(0);
}

static void *task_func3(void *dummy)
{
	struct timespec abstime;
 	task3 = rt_task_init_schmod(nam2num("TASK3"), 3, 0, 0, SCHED_FIFO, 0x1);
	mlockall(MCL_CURRENT | MCL_FUTURE);
	RT_MAKE_HARD_REAL_TIME();
	pthread_cleanup_push_rt(task_exit_handler, 0);
	pthread_barrier_wait_rt(barrier);
	DISPLAY("Starting task3, waiting on the conditional variable to be 3 with a 2 s timeout.\n");
	pthread_mutex_lock_rt(mtx);
	while(cond_data < 3) {
		nanos2timespec(rt_get_time_ns() + 2000000000LL, &abstime);
		if (pthread_cond_timedwait_rt(cond, mtx, &abstime)) {
			break;
		}
	}
	pthread_mutex_unlock_rt(mtx);
	if(cond_data < 3) {
		DISPLAY("task3, timed out, conditional variable value: %d.\n", cond_data);
	}
	pthread_mutex_lock_rt(mtx);
	cond_data = 3;
	pthread_mutex_unlock_rt(mtx);
	DISPLAY("task3 broadcasts after setting data to 3.\n");
	pthread_cond_broadcast_rt(cond);
	pthread_cleanup_pop_rt(1);
	DISPLAY("Ending task3.\n");
	pthread_exit_rt(0);
}

static void *task_func4(void *dummy)
{
 	task4 = rt_task_init_schmod(nam2num("TASK4"), 4, 0, 0, SCHED_FIFO, 0x1);
	mlockall(MCL_CURRENT | MCL_FUTURE);
	RT_MAKE_HARD_REAL_TIME();
	pthread_cleanup_push_rt(task_exit_handler, 0);
	pthread_barrier_wait_rt(barrier);
	DISPLAY("Starting task4, signalling after setting data to 1, then waits for a broadcast.\n");
	pthread_mutex_lock_rt(mtx);
	cond_data = 1;
  	pthread_mutex_unlock_rt(mtx);
	pthread_cond_signal_rt(cond);
	pthread_mutex_lock_rt(mtx);
	while(cond_data < 3) {
		pthread_cond_wait_rt(cond, mtx);
	}
	pthread_mutex_unlock_rt(mtx);
	if(cond_data == 3) {
		DISPLAY("task4, conditional variable broadcasted, value: %d.\n", cond_data);
	}
	pthread_cleanup_pop_rt(1);
	DISPLAY("Ending task4.\n");
	pthread_exit_rt(0);
}

static void main_exit_handler(void *arg)
{
#ifdef USE_OPEN
	pthread_barrier_close_rt(barrier);
#else
	pthread_barrier_destroy_rt(barrier);
#endif
	stop_rt_timer();
}

static pthread_t thread1, thread2, thread3, thread4;

int main(void)
{
	RT_TASK *task;
	task = rt_task_init_schmod(nam2num("MAIN"), 0, 0, 0, SCHED_FIFO, 0x1);
	mlockall(MCL_CURRENT | MCL_FUTURE);
	RT_MAKE_HARD_REAL_TIME();
	pthread_cleanup_push_rt(main_exit_handler, 0);
	start_rt_timer(nano2count(TICK));
	DISPLAY("User space POSIX test program.\n");
#ifdef USE_OPEN
	cond = pthread_cond_open_rt("CONDVR");
	mtx = pthread_mutex_open_rt("MUTEX");
	barrier = pthread_barrier_open_rt("BARIER", 5);
	cond = pthread_cond_open_rt("CONDVR");
	mtx = pthread_mutex_open_rt("MUTEX");
	barrier = pthread_barrier_open_rt("BARIER", 5);
#else
	pthread_cond_init_rt(cond = &conds, NULL);
	pthread_mutex_init_rt(mtx = &mtxs, NULL);
	pthread_barrier_init_rt(barrier = &barriers, NULL, 5);
#endif
	pthread_create_rt(&thread1, NULL, task_func1, NULL);
	pthread_create_rt(&thread2, NULL, task_func2, NULL);
	pthread_create_rt(&thread3, NULL, task_func3, NULL);
	pthread_create_rt(&thread4, NULL, task_func4, NULL);
	pthread_barrier_wait_rt(barrier);
	DISPLAY("\nDo not panic, wait 2 s, till task3 times out.\n\n");
	pthread_barrier_wait_rt(barrier);
	pthread_join_rt(thread1, NULL);
	pthread_join_rt(thread2, NULL);
	pthread_join_rt(thread3, NULL);
	pthread_join_rt(thread4, NULL);
#ifdef USE_OPEN
	pthread_cond_close_rt(cond);
	pthread_mutex_close_rt(mtx);
	pthread_barrier_close_rt(barrier);
	pthread_cond_close_rt(cond);
	pthread_mutex_close_rt(mtx);
#else
	pthread_cond_destroy_rt(cond);
	pthread_mutex_destroy_rt(mtx);
#endif
	pthread_cleanup_pop_rt(1);
	DISPLAY("User space POSIX test program removed.\n");
	pthread_exit_rt(0);
}
