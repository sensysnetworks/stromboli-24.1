/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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
#include <sys/mman.h>
#include <fcntl.h>
#include <sched.h>

#include <rtai_shm.h>
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#define COUNT 3000

int main(void)
{
	RT_TASK *sending_task ;
	SEM *shmsem, *agentsem;
	int i, *shm, shm_size, count;
	unsigned long chksum;

	struct sched_param mysched;

	mysched.sched_priority = 99;

	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
	puts(" ERROR IN SETTING THE SCHEDULER UP");
	perror( "errno" );
	exit( 0 );
 	}       

	mlockall(MCL_CURRENT | MCL_FUTURE);

	sending_task = rt_task_init(nam2num("STSK"), 0, 0, 0);
	shmsem   = rt_get_adr(nam2num("SHSM"));
	agentsem = rt_get_adr(nam2num("AGSM"));
	shm = rtai_malloc(nam2num("MEM"), 1);
	shm_size = shm[0];
	count = COUNT;
	while(count--) {
		printf("SENDING TASK WAIT ON SHMSEM\n");
		rt_sem_wait(shmsem);
		printf("SENDING TASK SIGNALLED ON SHMSEM\n");
			if (!(shm[0] = ((float)rand()/(float)RAND_MAX)*shm_size) || shm[0] > shm_size) {
				shm[0] = shm_size;
			}
			chksum = 0;
			for (i = 1; i <= shm[0]; i++) {
				shm[i] = rand();
				chksum += shm[i];
			}
			shm[shm[0] + 1] = chksum;
			printf("STSK: %d CHECKSUM = %lx\n", count, chksum);
		printf("SENDING TASK SIGNAL AGENTSEM\n");
		rt_sem_signal(agentsem);
	}
	printf("SENDING TASK LAST WAIT ON SHMSEM\n");
	rt_sem_wait(shmsem);
	printf("SENDING TASK SIGNALLED ON SHMSEM\n");
	shm[0] = 0;
	printf("SENDING TASK LAST SIGNAL TO AGENTSEM\n");
	rt_sem_signal(agentsem);
	printf("SENDING TASK DELETES ITSELF\n");
	rt_task_delete(sending_task);
	printf("END SENDING TASK\n");
	return 0;
}
