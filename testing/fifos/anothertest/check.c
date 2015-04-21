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
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>

#include "rtai_fifos.h"

#define SELTOUT 200
#define TOUT 200
#define SELOOP 5
#define TIMLOOP 5
#define SEMLOOP 5

static int fd0;
static struct sample { long long min; long long max; int index; } samp;

int main(void)
{
 	fd_set rfds;
	struct timeval tv;
	struct sched_param mysched;

	mysched.sched_priority = 99;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts(" ERROR IN SETTING THE SCHEDULER");
		perror( "errno" );
		exit( 0 );
 	}       

	if ((fd0 = open("/dev/rtf0", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf0\n");
		exit(1);
	}
	rtf_sem_post(fd0);

printf("SELECT IO\n");

{
	int i, k;
	i = 0;
	while (i < SELOOP) {
		FD_ZERO(&rfds);
		FD_SET(fd0, &rfds);
		tv.tv_sec = 0;
		tv.tv_usec = SELTOUT*1000;
		if ((k = select(fd0 + 1, &rfds, NULL, NULL, &tv))) {
			i++;
			read(fd0, &samp, sizeof(samp));
			printf("*** SEL: min: %8d, max: %8d average: %d ***\n", (int) samp.min, (int) samp.max, samp.index);
		} else {
			printf ("SELECT TIMED OUT %d\n", k);
		}
		fflush(stdout);
	}
}

printf("\n\n\nEAVSDROPPING\n");

{
	int i, k;
	i = 0;
	while (i < TIMLOOP) {
		if ((k = rtf_evdrp(fd0, &samp, sizeof(samp))) > 0) {
			i++;
		printf("*** TIM: min: %8d, max: %8d average: %d (EVDRP) ***\n", (int) samp.min, (int) samp.max, samp.index);
			read(fd0, &samp, sizeof(samp));
			printf("*** TIM: min: %8d, max: %8d average: %d ***\n", (int) samp.min, (int) samp.max, samp.index);
		}
		fflush(stdout);
	}
}

printf("\n\n\nTIMED IO\n");

{
	int i, k;
	i = 0;
	while (i < TIMLOOP) {
		if ((k = rtf_read_timed(fd0, &samp, sizeof(samp), TOUT)) > 0) {
			i++;
			printf("*** TIM: min: %8d, max: %8d average: %d ***\n", (int) samp.min, (int) samp.max, samp.index);
		} else {
			printf ("TIMED OUT READ TIMED OUT %d\n", k);
		}
		fflush(stdout);
	}
}

printf("\n\n\nSEMPHORE\n");

{
	int i, k;
	rtf_sem_init(fd0, 1);
	k = 0;
	for (i = 1; i <= SEMLOOP; i++) {
		printf("SEM: %d\n", i);
		if (k) {
			rtf_sem_wait(fd0);
		} else {
			rtf_sem_trywait(fd0);
		}
		k = 1 - k;
		rtf_suspend_timed(fd0, 250);
		rtf_sem_trywait(fd0);
		rtf_sem_timed_wait(fd0, 750);
		rtf_sem_post(fd0);
	}
	rtf_sem_destroy(fd0);
}

	return 0;
}
