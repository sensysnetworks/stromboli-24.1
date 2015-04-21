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
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include <rtai_tasklets.h>

#define LOOPS 100

static volatile int end, data = LOOPS;

static struct rt_tasklet_struct *tasklet;

static int tskloops;

static void tasklet_handler(unsigned long data)
{
	if (tskloops++ < LOOPS) {
		rt_printk("\nTASKLET: %d, %d", tskloops, (*((int *)data))--);
	} else {
		end = 1;
	}
}

int main(void)
{
	struct sched_param mysched;

	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts(" ERROR IN SETTING THE SCHEDULER UP");
		perror("errno");
		exit(0);
 	}       
	mlockall(MCL_CURRENT | MCL_FUTURE);

	tasklet = rt_init_tasklet();
	rt_insert_tasklet(tasklet, 0, tasklet_handler, (unsigned long)&data, nam2num("TSKLET"), 1);
	while(!end) sleep(1);
	rt_remove_tasklet(tasklet);
	rt_delete_tasklet(tasklet);
	return 0;
}
