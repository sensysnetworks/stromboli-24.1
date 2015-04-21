/*
COPYRIGHT (C) 2000  Emanuele Bianchi (bianchi@aero.polimi.it)
                    Paolo Mantegazza (mantegazza@aero.polimi.it)
                    Pierre Cloutier  (pcloutier@PoseidonControls.com)

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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#define LOOPS  1000
#define NR_RT_TASKS 30
#define taskname(x) (1000 + (x))

static pthread_t thread[NR_RT_TASKS];

static RT_TASK *mytask[NR_RT_TASKS];

static SEM *sem;

static int volatile hrt[NR_RT_TASKS], change, end;       

static int indx[NR_RT_TASKS];       

pid_t proxy;

static void *thread_fun(void *arg)
{
	int mytask_indx, *pti;
	struct sched_param mysched;

//	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if (sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		printf("ERROR IN SETTING THE POSIX SCHEDULER\n");
		exit(1);
 	}       

	pti = (int *) arg;
	mytask_indx = *pti;
 	if (!(mytask[mytask_indx] = rt_task_init(taskname(mytask_indx), 0, 0, 0))) {
		printf("CANNOT INIT TASK %d\n", taskname(mytask_indx));
		exit(1);
	}

	rt_grow_and_lock_stack(4000);

	rt_make_hard_real_time();
	hrt[mytask_indx] = 1;
	while (!end) {
		if (change) {
			rt_sem_wait(sem);
		} else {
		 	rt_task_suspend(mytask[mytask_indx]);
		}
	}

	rt_make_soft_real_time();

        hrt[mytask_indx] = 0;
	rt_task_suspend(mytask[mytask_indx]);
        rt_Trigger(proxy);

	return (void*)0;
}

int main(void)
{
	RTIME tsr, tsm;
	RT_TASK *mainbuddy;
	int  /* len, cnt,*/ i, k, s;       
	// char buf[32]; 	
	struct sched_param mysched;
	// pid_t pid;

	printf("\n\nWait for it ...\n");
 	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if (sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		printf("ERROR IN SETTING THE POSIX SCHEDULER\n");
		exit(1);
 	}       

 	if (!(mainbuddy = rt_task_init(nam2num("MASTER"), 1, 0, 0))) {
		printf("CANNOT INIT TASK %lu\n", nam2num("MASTER"));
		exit(1);
	}

        rt_Alias_attach("");
        proxy = rt_Proxy_attach(0, 0, 0, -1);

        rt_set_oneshot_mode();
        start_rt_timer(0);

	for (i = 0; i < NR_RT_TASKS; i++) {
		indx[i] = i;
		if (pthread_create(thread + i, NULL, thread_fun, &indx[i])) {
			printf("ERROR IN CREATING THREAD %d\n", indx[i]);
			exit(1);
 		}       
 	} 

	sem = rt_sem_init(nam2num("SEMAPH"), 1); 
	change =  0;
	
	do {
		rt_sleep(nano2count(10000000));
		s = 0;	
		for (i = 0; i < NR_RT_TASKS; i++) {
			s += hrt[i];
		}
	} while (s != NR_RT_TASKS);
	rt_grow_and_lock_stack(4000);

	rt_make_hard_real_time();
	tsr = rt_get_cpu_time_ns();
	for (i = 0; i < LOOPS; i++) {
		for (k = 0; k < NR_RT_TASKS; k++) {
			rt_task_resume(mytask[k]);
		} 
	} 
	tsr = rt_get_cpu_time_ns() - tsr;

	change = 1;

	for (k = 0; k < NR_RT_TASKS; k++) {
		rt_task_resume(mytask[k]);
	} 

	tsm = rt_get_cpu_time_ns();
	for (i = 0; i < LOOPS; i++) {
		for (k = 0; k < NR_RT_TASKS; k++) {
	        	rt_sem_signal(sem);
		}
	}
	tsm = rt_get_cpu_time_ns() - tsm;
	rt_make_soft_real_time();

	printf("\n\nFOR %d TASKS: ", NR_RT_TASKS);
	printf("TIME %d (ms), SUSP/RES SWITCHES %d, ", (int)(tsr/1000000), 2*NR_RT_TASKS*LOOPS);
	printf("SWITCH TIME %d (ns)\n", (int)(tsr/(2*NR_RT_TASKS*LOOPS)));

	printf("\nFOR %d TASKS: ", NR_RT_TASKS);
	printf("TIME %d (ms), SEM SIG/WAIT SWITCHES %d, ", (int)(tsm/1000000), 2*NR_RT_TASKS*LOOPS);
	printf("SWITCH TIME %d (ns)\n\n", (int)(tsm/(2*NR_RT_TASKS*LOOPS)));
	printf("NOW TYPE CTRL-C TO END IT ALL\n");
	fflush(stdout);

	// Nice end game without any rt_sleep()'s.
	end = 1;
	for (i = 0; i < NR_RT_TASKS; i++)
	        rt_sem_signal(sem);

        for (i = 0; i < NR_RT_TASKS; i++)
                rt_Receive(proxy, 0, 0, 0);

        for (i = 0; i < NR_RT_TASKS; i++)
	        rt_task_delete(mytask[i]);

	// We purposely forgot to detach the proxy. Check for memory leaks...
	rt_sem_delete(sem);
	rt_task_delete(mainbuddy);
        for (i = 0; i < NR_RT_TASKS; i++) {
                pthread_join(thread[i], NULL);
        }

	return 0;
}
