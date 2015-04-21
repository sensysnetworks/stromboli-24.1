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
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
 
#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#include "test.h"

int main(void)
{
	RT_TASK *sfttsk;
	RTIME t0, t;
	int count, jit, maxj, maxjp, maxjs, period;

	struct sched_param mysched;

	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts("ERROR IN SETTING THE SCHEDULER");
		perror("errno");
		exit(0);
 	}       

	printf("SFTTSK: T0 %f (ms)\n", rt_get_cpu_time_ns()/1000000.);

 	if (!(sfttsk = rt_task_init(nam2num("SFTTSK"), 1, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(1);
	}

	rt_return(rt_receive(0, &maxj), 1); 
	period = nano2count(PERIOD); 
	mlockall(MCL_CURRENT | MCL_FUTURE);

	rt_task_make_periodic(sfttsk, rt_get_time() + period, period);

	count = maxj = 0;
	t0 = rt_get_cpu_time_ns();
	while(count++ < PERIODIC_LOOPS) {
		rt_task_wait_period();
		t = rt_get_cpu_time_ns();
		if ((jit = t - t0 - PERIOD) < 0) {
			jit = -jit;
		}
		if (count > 1 && jit > maxj) {
			maxj = jit;
		}
		t0 = t;
//		rtai_print_to_screen("SFTTSK: %d\n", count);
	}
	maxjp = (maxj + 499)/1000;

	rt_make_hard_real_time();
	rt_make_soft_real_time();
	rt_make_hard_real_time();
	rt_make_soft_real_time();

	count = maxj = 0;
	t0 = rt_get_cpu_time_ns();
	while(count++ < SLEEP_LOOPS) {
		t = rt_get_cpu_time_ns();
		if ((jit = t - t0 - PERIOD) < 0) {
			jit = -jit;
		}
		if (count > 1 && jit > maxj) {
			maxj = jit;
		}
		t0 = t;
//		rtai_print_to_screen("SFTTSK: %d\n", count);
		rt_sleep(period);
	}
	maxjs = (maxj + 499)/1000;

	rt_return(rt_receive(0, &maxj), 1); 
	rt_task_delete(sfttsk);
	printf("END %s, MAX JIT: PERIOD %d (us), SLEEP %d (us)\n", "SFTTSK", maxjp, maxjs);
	printf("SFTTSK: TF %f (ms)\n",  rt_get_cpu_time_ns()/1000000.);
	exit(0);
}
