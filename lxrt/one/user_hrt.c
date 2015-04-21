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
#include <sched.h>
#include <stdlib.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#define OPT_MINUS(x)    (('-' << 8) + x)
#define OPT_EQ(x)       ((x << 8) + '=')
 
#include "test.h"

static int verbose, uspsh, oneshot, loops, query, softhard, sigcnt;
static int period, periodns;

static void handler(void)
{
	sigcnt++;
}

int main(int argc, char *argv[])
{
	unsigned long hrttsk_name = nam2num("HRTTSK");
	RT_TASK *hrttsk;
	RTIME t0, t;
	int i, count, jit, maxj, maxjp, maxjs;
	char *opt, status[32];
	struct sched_param mysched;

	loops    = PERIODIC_LOOPS;
	softhard++;
        periodns = PERIOD;

	for( i=1 ; i < argc ; i++ ) {
		opt = argv[i];
		switch((*opt<<8) + *(opt+1)) {
			case OPT_MINUS('s'):softhard=0;break;
                        case OPT_MINUS('q'):query++;   break;
			case OPT_MINUS('o'):oneshot++; break;
			case OPT_MINUS('u'):uspsh++;   break;
                        case OPT_MINUS('v'):verbose++; break;
			case OPT_EQ('p'):
				periodns = strtol(opt+2, NULL, 0);
				break;
                        case OPT_EQ('l'):
				loops = strtol(opt+2, NULL, 0);
                                break;
 		}
        }

	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts("ERROR IN SETTING THE SCHEDULER");
		perror("errno");
		exit(1);
 	}       

//	if(lock_all(0,0)) {
//		printf("lock_all(stack,heap) failed\n");
//		exit(2);
//	}
        mlockall(MCL_CURRENT | MCL_FUTURE);

 	if (!(hrttsk = rt_task_init(hrttsk_name, 1, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(3);
	}

	if (query) {
		printf("HARD (!=0) OR FIRM (==0) REAL TIME: ");
		scanf("%d", &softhard);
		}

	if (verbose) printf("T0 %f (ms): %s mode, per = %d, loops = %d, %s\n",  rt_get_cpu_time_ns()/1000000., softhard ? "HRT" : "PSX", periodns, loops, uspsh ? "+ usp sig":"");

	if (oneshot) rt_set_oneshot_mode();
	else rt_set_periodic_mode();
	period = (int) nano2count((RTIME)periodns);
	start_rt_timer(period);

        if (uspsh) rt_usp_signal_handler(handler);

        if (softhard) {
                rt_make_hard_real_time();
        }

	rt_task_make_periodic(hrttsk, rt_get_time() + period, period);

	sigcnt = count = maxj = 0;
	t0 = rt_get_cpu_time_ns();

	while(((softhard && uspsh) ? sigcnt : count) < loops) {
		rt_task_wait_period();
		t = rt_get_cpu_time_ns();
		if ((jit = t - t0 - periodns) < 0) {
			jit = -jit;
		}
		if (count++ > 1 && jit > maxj) {
			maxj = jit;
		}

		t0 = t;
	}
	maxjp = (maxj + 499)/1000;

	if (softhard) {
		rt_make_soft_real_time();
		rt_make_hard_real_time();
		rt_make_soft_real_time();
		rt_make_hard_real_time();
	}

	sigcnt = count = maxj = 0;
	t0 = rt_get_cpu_time_ns();

	while(((softhard && uspsh) ? sigcnt : count) < loops) {
		t = rt_get_cpu_time_ns();
		if ((jit = t - t0 - periodns) < 0) {
			jit = -jit;
		}
		if (count++ > 1 && jit > maxj) {
			maxj = jit;
		}
		t0 = t;

		rt_sleep(((RTIME)period));

		if(softhard && uspsh && (count!=sigcnt)) {
			rtai_print_to_screen( "rt_sleep(%d) fails, the period is too small\n", period);
			break;
		}
	}

	maxjs = (maxj + 499)/1000;
        if (uspsh) rt_usp_signal_handler(0);
	if (softhard) rt_make_soft_real_time();
	stop_rt_timer();
	rt_task_delete(hrttsk);
        sprintf(status, "%s %s %s", softhard ? "HRT" : "SFT", oneshot ? "ost":"per", uspsh ? "sig":"   ");
	printf("%s, MAX JIT: PERIOD %5d (us), SLEEP %5d (us)\n", status, maxjp, maxjs);
	if (verbose) printf("TF %f (ms)\n",  rt_get_cpu_time_ns()/1000000.);

	exit(0);
}
