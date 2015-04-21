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
	RT_TASK *hrttsk, *other;
	RTIME t0, t;
	int count, jit, maxj, maxjp, maxjs, period;

	struct sched_param mysched;

	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts("ERROR IN SETTING THE SCHEDULER");
		perror("errno");
		exit(0);
 	}       

	printf("HRTTSK: T0 %f (ms)\n", rt_get_cpu_time_ns()/1000000.);

 	if (!(hrttsk = rt_task_init(nam2num("HRTTSK"), 1, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(1);
	}

	rt_set_oneshot_mode();
	period = nano2count(PERIOD); 
	start_rt_timer(period);
	mlockall(MCL_CURRENT | MCL_FUTURE);

	rt_make_hard_real_time();
	while (!(other = rt_get_adr(nam2num("SFTTSK")))) {
		rt_sleep(nano2count(1000000));
	}
	rt_rpc(other, 1, &maxj);
	rt_task_make_periodic(hrttsk, rt_get_time() + period, period);

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
//		rtai_print_to_screen("HRTTSK: %d\n", count);
	}
	maxjp = (maxj + 499)/1000;

	rt_make_soft_real_time();
	rt_make_hard_real_time();
	rt_make_soft_real_time();
	rt_make_hard_real_time();

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
//		rtai_print_to_screen("HRTTSK: %d\n", count);
		rt_sleep(period);
	}
	maxjs = (maxj + 499)/1000;

	rt_rpc(other, 1, &maxj);
	while (rt_get_adr(nam2num("SFTTSK"))) {
		rt_sleep(nano2count(1000000));
	}
	stop_rt_timer();
	rt_make_soft_real_time();
	rt_task_delete(hrttsk);
	printf("END %s, MAX JIT: PERIOD %d (us), SLEEP %d (us)\n", "HRTTSK", maxjp, maxjs);
	printf("HRTTSK: TF %f (ms)\n", rt_get_cpu_time_ns()/1000000.);
	exit(0);
}
