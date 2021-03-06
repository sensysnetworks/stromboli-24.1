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
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <asm/rtai_srq.h>

#define PRINT_REPEAT 100

static int end;

static void endme (int dummy) { end = 1; }

int main(void)
{
	int srq, tick, count = 0, nextcount = 0;
	struct sched_param mysched;
	long long time0, time, dt;

	signal (SIGINT, endme);

	mysched.sched_priority = 99;

	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts(" ERROR IN SETTING THE SCHEDULER UP");
		perror( "errno" );
		exit( 0 );
 	}       

	srq = rtai_open_srq(0xcacca);
	rtai_srq(srq, (unsigned int)&time0);
	tick = rtai_srq(srq, 1);
	dt = time0;

	while (!end) {
		rtai_srq(srq, (unsigned int)&time);
		if (++count > nextcount) {
			nextcount += PRINT_REPEAT;
			printf("# %d TICK: %d TIME: %lld DTOT: %lld\n", nextcount, tick, time - time0, time - dt);
		dt = time;
		}
	}
	return 0;
}
