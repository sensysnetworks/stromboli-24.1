/*
COPYRIGHT (C) 2001  Paolo Mantegazza  (mantegazza@aero.polimi.it)
                    Giuseppe Quaranta (quaranta@aero.polimi.it)

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


#ifndef _VXW_TASKLIB_H_
#define _VXW_TASKLIB_H_

#define VX_FP_TASK      1

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/io.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#include "semLib.h"

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include "sysAuxClk.h"

extern unsigned int get_an_rtw_name(void);
extern void hsleep(int hs);

extern int use_hrt;

#define NAME_SIZE 30
struct thread_args { char name[NAME_SIZE]; int priority, use_fpu, stack_size; void *(*fun)(int, ...); int a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, hrt; pthread_t thread; RT_TASK *buddy; };

static void *support_fun(void *arg)
{
	RT_TASK *buddy;
	struct thread_args *args;
	int hrt;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	args = (struct thread_args *)arg;
 	if (!(buddy = rt_task_init_schmod(get_an_rtw_name(), args->priority, 0, 0, SCHED_FIFO, 0xFFFFFFFF))) {
		printf("CANNOT INIT BUDDY TASK FOR %s.\n", args->name);
		free(args);
		return (void *)1;
	}
#ifdef DBGPRT
	printf("BUDDY CREATED FOR %s (%p).\n", args->name, buddy);
#endif
 	args->buddy = buddy;
	iopl(3);
       
	if (strstr(args->name, "Rate") && use_hrt) {
 		if (args->use_fpu == VX_FP_TASK) {
	        	rt_task_use_fpu(buddy, 1);
		}
		hrt = args->hrt = 1;
		rt_set_usp_flags_mask(FORCE_SOFT);
#ifdef DBGPRT
		printf("GOING HARD FOR %s (BUDDY %p).\n", args->name, buddy);
#endif
        	rt_grow_and_lock_stack(args->stack_size);
		rt_make_hard_real_time();
	} else {
		hrt = 0;
 	}

	args->fun(args->a1, args->a2, args->a3, args->a4, args->a5, args->a6, args->a7, args->a8, args->a9, args->a10);

	if (hrt) {
		rt_make_soft_real_time();
		args->hrt = 0;
#ifdef DBGPRT
		printf("BUDDY %p BACK TO SOFT.\n", buddy);
#endif
	}

#ifdef DBGPRT
	printf("BUDDY %p SUSPENDS HIMSELF.\n", buddy);
#endif
	rt_task_suspend(buddy);
	return (void *)0;
}

static inline int taskSpawn(char *name, int priority, int use_fpu, int stack_size, void *(*fun)(int, ...), int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10)
{
	struct thread_args *args;

	args = malloc(sizeof(struct thread_args));
	strncpy(args->name, name, NAME_SIZE - 1);
	args->name[NAME_SIZE - 1] = 0;
	args->priority   = priority;
	args->use_fpu    = use_fpu;
	args->stack_size = stack_size;
	args->fun        = fun;
	args->a1         = a1;
	args->a2         = a2;
	args->a3         = a3;
	args->a4         = a4;
	args->a5         = a5;
	args->a6         = a6;
	args->a7         = a7;
	args->a8         = a8;
	args->a9         = a9;
	args->a10        = a10;
	args->hrt        = 0;
	args->buddy      = (void *)0;
	pthread_create(&args->thread, NULL, support_fun, args);
	return (int)args;
}

static inline void taskDelete(int arg)
{
	struct thread_args *args;

	args = (struct thread_args *)arg;
#ifdef DBGPRT
	printf("TASK DELETE CALLED FOR BUDDY %p, PARENT PTHREAD %s.\n", args->buddy, args->name);
#endif
	if (args->hrt) {
		struct timeval timout;
		SEM *sem;

		rt_set_usp_flags(args->buddy, FORCE_SOFT);
		if ((sem = (SEM *)sysAuxClkSem(args->buddy))) {
			rt_sem_signal(sem);
		}
		timout.tv_sec = 0;
	        while(rt_is_hard_real_time(args->buddy)) {
			timout.tv_usec = 5000;
			select(1, NULL, NULL, NULL, &timout);
	        }
#ifdef DBGPRT
		printf("BUDDY %p (%s) BACK TO SOFT.\n", args->buddy, args->name);
#endif
	}
	sysAuxClkRelSem(args->buddy);
	rt_task_suspend(args->buddy);
	pthread_cancel(args->thread);
	pthread_join(args->thread, NULL);
#ifdef DBGPRT
	printf("BUDDY %p DELETED, PARENT THREAD %s CANCELLED.\n", args->buddy, args->name);
#endif
	free(args);
}

#endif
