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
#include <signal.h>
#include <unistd.h>

static int end;

static void endme (int dummy) { end = 1; }

int main(void)
{
	int rtf;
	unsigned long out_secs, out_avrj, out_maxj, out_dot, out_timdot[2];
	int cpu_used[2];
	if ((rtf = open("/dev/rtf2", O_RDONLY)) < 0) {
		fprintf(stderr, "Error opening /dev/rtf2\n");
		exit(1);
	}

	signal (SIGINT, endme);

	while(!end) {
		read(rtf, &out_secs, sizeof(out_secs));
		read(rtf, &out_avrj, sizeof(out_avrj));
		read(rtf, &out_maxj, sizeof(out_maxj));
		read(rtf, &out_dot, sizeof(out_dot));
		read(rtf, &out_timdot[0], sizeof(out_timdot[0]));
		read(rtf, &out_timdot[1], sizeof(out_timdot[1]));
		read(rtf, &cpu_used[0], sizeof(cpu_used[0]));
		read(rtf, &cpu_used[1], sizeof(cpu_used[1]));
		printf("<>RT_HAL time: %ld s, AvrJ: %ld, MaxJ: %ld us (%ld,%ld,%ld)<> %d %d \n", out_secs, out_avrj, out_maxj, out_dot, out_timdot[0], out_timdot[1], cpu_used[0], cpu_used[1]);
	}
	return 0;
}
