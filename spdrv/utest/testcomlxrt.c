/*
COPYRIGHT (C) 2002  Giuseppe Renoldi (giuseppe@renoldi.org)

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


/*
 * rtai_spdrv-LXRT test
 * ================
 *
 * Adaptation of rtai_spdrv ktest modules to provide the same examples in
 * RTAI environment using LXRT.
 * 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#include <rtai_spdrv_lxrt.h>

#define TICK_PERIOD 100000000 // 100 msec
#define STACK_SIZE 1000
#define PORT 0

static int end = 0;
static void endme(int dummy) { end = 1; }

int main(int argc, char **argv)
{
	unsigned long testcomtsk_name = nam2num("TESTCOM");
	RT_TASK *testcomtsk;
	int tick_period;
	struct sched_param mysched;
	int retval=0;

	signal(SIGINT, endme);
	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts("ERROR IN SETTING THE SCHEDULER");
		perror("errno");
		exit(0);
 	}       
 	if (!(testcomtsk = rt_task_init(testcomtsk_name, 1, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(1);
	}

	rt_set_oneshot_mode();
	tick_period = start_rt_timer(nano2count(TICK_PERIOD));
	mlockall(MCL_CURRENT | MCL_FUTURE);

//	rt_make_hard_real_time();
	rt_task_make_periodic(testcomtsk, rt_get_time() + tick_period, tick_period);

	if (rt_spopen(PORT, 9600, RT_SP_NO_HAND_SHAKE, RT_SP_PARITY_NONE, 1, 8, -1) < 0) {
		printf("testcomlxrt: rt_spopen() error\n");
		retval = 1;
	} else {	
		rt_spwrite(PORT, "Begin serial Test\n\r", 20);
		printf("<1> RTAI_SPDRV_LXRT Test Program - Start\n");

		while (!end) {
		    	int n, sl;
		    	char buf[210];

			strcpy(buf,"\rread->");
			sl = strlen(buf);
		    	rt_task_wait_period();
			n = 5 - rt_spread(PORT, buf+sl,5); 
			if ( n > 0) {
				strcpy(buf + sl + n, "\r\n");
				rt_spwrite(PORT, buf, strlen(buf));
		    	}
		}
		rt_spclose(PORT);  // release port
	}

	rt_make_soft_real_time();
	stop_rt_timer();
	rt_task_delete(testcomtsk);
	printf ("<1> RTAI_SPDRV_LXRT Test Program - End\n");
	exit(retval);
}


