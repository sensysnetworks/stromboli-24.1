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


#include <linux/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#ifndef CONFIG_UCLINUX
#include <sched.h>
#endif
#include <signal.h>
#include <unistd.h>

static int end;

static void endme(int dummy) { end = 1; }

int main(void)
{
	int cmd0;
#ifndef CONFIG_UCLINUX
	struct sched_param mysched;
#endif
        struct {char task, susres; int flags; long long time;} msg = {'S',};                      
	signal (SIGINT, endme);

#ifndef CONFIG_UCLINUX
	mysched.sched_priority = 99;

	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
	puts(" ERROR IN SETTING UP THE SCHEDULER");
	perror( "errno" );
	exit( 0 );
 	}       
#endif

	if ((cmd0 = open("/dev/rtf0", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf0\n");
		exit(1);
	}
	while(!end) {
		read(cmd0, &msg, sizeof(msg));
		printf("> %c %c %x %lld\n", msg.task, msg.susres, msg.flags & 0x201, msg.time/1000000);
	}
	return 0;
}

