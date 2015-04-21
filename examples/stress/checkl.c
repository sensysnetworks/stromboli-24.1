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
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>

static int end;

static void endme (int dummy) { end = 1; }

int main(int argc,char *argv[])
{
	int fd0;
	struct sample { long long min; long long max; int avrg; unsigned long flags; } samp;
	
	if ((fd0 = open("/dev/rtf0", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf0\n");
		exit(1);
	}

	signal (SIGINT, endme);

	while (!end) {
		read(fd0, &samp, sizeof(samp));
		printf("*** min: %8d, max: %8d average: %d flags: %lx***\n", (int) samp.min, (int) samp.max, samp.avrg, samp.flags);
		fflush(stdout);
	}

	return 0;
}
