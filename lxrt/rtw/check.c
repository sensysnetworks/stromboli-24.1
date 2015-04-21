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


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <sys/mman.h>
#include <pthread.h>
#include <unistd.h>

#include <asm/rtai_srq.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include "sysAuxClk.h"

#define OVR_CHK_STP  5

static pthread_t overuns_mon_thread;

static int ovr_chk_stp = OVR_CHK_STP;

static void *overuns_mon_fun(void *args)
{
	float run_time, ovrfreq, check_freq;
	int overuns, scnt;

	printf("OVERUNS MONITOR RUNNING.\n");
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	run_time = 0.0;
	check_freq = 1.0/ovr_chk_stp;
	while (1) {
		scnt = ovr_chk_stp;
		while (scnt--) {
			sleep(1);
		}
		if (sysAuxClkStatus() > 0) {
			run_time += ovr_chk_stp;
			overuns = sysAuxClkOveruns();
			if ((ovrfreq = overuns/run_time) >= check_freq) {
				printf("AFTER RUNNING %f (s): OVERUNS %d, AVERAGE OVERUNS FREQUENCY: %f.\n", run_time, overuns, ovrfreq);
			}
		} else {
			run_time = 0.0;
		}
	}
	return (void *)0;
}

int main(void)
{
	RT_TASK *buddy;
	SEM *sem;
	int count, test_time, overuns, module_overuns, busy_sleep;

        if (!(buddy = rt_task_init_schmod(nam2num("CLK"), 1, 0, 0, SCHED_FIFO, 0xFFFFFFFF))) {
                printf("CANNOT INIT BUDDY TASK\n");
                exit(1);
        }
	pthread_create(&overuns_mon_thread, NULL, overuns_mon_fun, NULL);
 	sem = rt_sem_init(nam2num("SEM"), 0);
	rt_grow_and_lock_stack(10000);

	while (1) {
		printf ("Previous frequency %d, new tick frequency (0 to exit): ", sysAuxClkRateGet());
		scanf("%d", &count);
		if (count <= 0) {
			break;
		}
		printf ("Test time (seconds): ");
		scanf("%d", &test_time);
		sysAuxClkRateSet(count);
		count = sysAuxClkRateGet()*test_time;
		sysAuxClkConnect(0, (int)sem);
		printf("Wait %d seconds.\n", test_time);
		overuns = 0;
		busy_sleep = 500000000/sysAuxClkRateGet();
		rt_make_hard_real_time();	
		sysAuxClkEnable();
		while(count--) {
			if (rt_sem_wait(sem) > 0) {
				overuns++;
			}
			rt_busy_sleep(busy_sleep);
		}
		sysAuxClkDisable();
		module_overuns = sysAuxClkOveruns();
		rt_make_soft_real_time();	
		printf("End of test (Overuns: local %d, module %d).\n\n", overuns, module_overuns);

	}
	sysAuxClkDisable();
	rt_sem_delete(sem);
	rt_task_delete(buddy);
	pthread_cancel(overuns_mon_thread);
	pthread_join(overuns_mon_thread, NULL);
	printf("OVERUNS MONITOR STOPPED.\n");
	return 0;
}
