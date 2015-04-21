
/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)

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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#define OPT_MINUS(x)    (('-' << 8) + x)
#define OPT_EQ(x)       ((x << 8) + '=')

#define PERIOD 	1000000
#define LOOPS  		150
#define MAXNTASKS	 48
#define NTASKS   	 32
#define KEYONE		 1

#define taskname(x) (1000 + (x))                                                 

pthread_t task[MAXNTASKS];
int ntasks = NTASKS;
int query, crashkernel;
RT_TASK *mytask;
SEM *sem;

// get the time stamp clock (TSC) of a cpu.
static inline unsigned long long rd_CPU_ts(void)
{
	unsigned long long time;
	__asm__ __volatile__( "rdtsc" : "=A" (time));
	return time;
}

void crash(int sig)
{
	int *pid;
	
	pid = (int *) pthread_getspecific(KEYONE);
	printf("Oops I just crashed! (pid = %d)\n", *pid);
	pthread_exit(0);
}

void rt_crash(int sig)
{
	int *pid;

	pid = (int *) pthread_getspecific(KEYONE);
	printf("Oops my real time agent just crashed!  (pid = %d)\n", *pid);
	pthread_exit(0);
}

void *thread_fun(void *arg)
{
	RTIME start_time, period;
	RTIME /*now,*/  t0, t;
	SEM *sem;
	RT_TASK *mytask;
	unsigned long mytask_name;
	int mytask_indx, jit, maxj, maxjp, count, key, my_pid;
//	int zint     =    0;
	int *explode = NULL;

	signal(SIGSEGV, crash);     // We dereference a NULL pointer
	signal(SIGTRAP, rt_crash);  // rt_boom() throws a debug trap
	
	init_linux_scheduler(SCHED_FIFO, sched_get_priority_max(SCHED_FIFO) - 1);
//	lock_all(4096,4096);

	mytask_indx = *((int *)arg);
	mytask_name = taskname(mytask_indx);
//	printf("thread_fun(%d)\n", mytask_indx);
 	if (!(mytask = rt_task_init(mytask_name, 1, 4096, 0))) {
		printf("CANNOT INIT TASK %lu\n", mytask_name);
		exit(1);
	}
	rtai_print_to_screen("THREAD INIT: pid %3d index = %d, name = %lu, address = %p.\n", my_pid = getpid(), mytask_indx, mytask_name, mytask);

	key = KEYONE;
	pthread_key_create(&key, 0);
	pthread_setspecific(key, &my_pid);
	
	rt_receive(0, (unsigned int*)&sem);

	period = nano2count(PERIOD);
	start_time = rt_get_time() + nano2count(10000000);
	rt_task_make_periodic(mytask, start_time + (mytask_indx + 1)*period, ntasks*period);

// start of task body
	{
		count = maxj = 0;
		t0 = rt_get_cpu_time_ns();
		while(count++ < LOOPS) {
			t = rt_get_cpu_time_ns();
			if ((jit = t - t0 - ntasks*(RTIME)PERIOD) < 0) {
				jit = -jit;
			}
			if (count > 1 && jit > maxj) {
				maxj = jit;
			}
			t0 = t;
//			rtai_print_to_screen("THREAD: index = %d, count %d\n", mytask_indx, count);
			rt_task_wait_period();
		}
		maxjp = (maxj + 499)/1000;
	}
// end of task body

// test trap
	rtai_print_to_screen("Boom: index = %d\n", mytask_indx);
	if (!crashkernel) *explode = 0; else rt_boom();

	rt_sem_signal(sem);

	rt_task_delete(mytask);
	printf("THREAD %lu ENDS, LOOPS: %d MAX JIT: %d (us)\n", mytask_name, count, maxjp);
	return 0;
}

void endme(int dummy)
{
	printf("endme()\n");
	rt_sem_delete(sem);
	rt_task_delete(mytask);
	stop_rt_timer();
	//	signal(SIGINT, SIG_DFL);
	exit(1);
}

int main(int argc, char *argv[])
{
	int i, indx[MAXNTASKS];       
	unsigned long mytask_name = nam2num("MASTER");
	pthread_attr_t attr;
    char *opt;
//	RTIME t1, t2;

    query = crashkernel = 0 ;
	
	for( i=1 ; i < argc ; i++ ) {
		opt = argv[i];
		switch((*opt<<8) + *(opt+1)) {
			case OPT_MINUS('k'): crashkernel++; break;
			case OPT_MINUS('q'): query++; break;
			case OPT_EQ('n'): ntasks = atoi(opt+2); break;
		}
	}

	if(ntasks > MAXNTASKS) ntasks = MAXNTASKS;

	if(query) {	
		printf("CRASH IN KERNEL (!=0) OR USER SPACE (==0): ");
        scanf("%d", &crashkernel);
	}

    init_linux_scheduler(SCHED_FIFO, sched_get_priority_max(SCHED_FIFO));
    lock_all(0,0);

 	if (!(mytask = rt_task_init(mytask_name, 1, 4096, 0))) {
		printf("CANNOT INIT TASK %lu\n", mytask_name);
		exit(1);
	}
	printf("MASTER INIT: pid %3d name = %lu, address = %p.\n", getpid(), mytask_name, mytask);

	sem = rt_sem_init(10000, 0); 

	signal(SIGINT, endme);

	rt_set_oneshot_mode();
//	rt_set_periodic_mode();
	start_rt_timer(nano2count(10000000));

//	rt_task_make_periodic(mytask, rt_get_time(), nano2count(10000000));

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched( &attr, PTHREAD_INHERIT_SCHED);

	for (i = 0; i < ntasks; i++) {
		indx[i] = i;
//		rtai_print_to_screen("pthread_create(%d)\n", i);
		if (pthread_create(&task[i], &attr, thread_fun, &indx[i])) {
			printf("ERROR IN CREATING THREAD %d\n", indx[i]);
			exit(1);
		}       
	}       

	for (i = 0; i < ntasks; i++) {
		while (!rt_get_adr(taskname(i))) {
			rt_sleep(nano2count(10000000));
		}
	}

	for (i = 0; i < ntasks; i++) {
		rt_send(rt_get_adr(taskname(i)), (unsigned int)sem);
	}

	for (i = 0; i < ntasks; i++) {
		while (rt_get_adr(taskname(i))) {
			rt_sleep(nano2count(10000000));
		}
	}

	rt_sem_delete(sem);
	rt_task_delete(mytask);
	printf("MASTER %lu %p ENDS\n", mytask_name, mytask);

	return 0;
}
