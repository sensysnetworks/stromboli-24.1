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
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <asm/io.h>

#define  KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include "./unix_lxrt.h"

int main(void)
{
	char s[1000];
	int i, fd;
	long long ll;
	float f;
	double d;
	RT_TASK *mytask;
	RTIME t;
        struct timeval timout;

	struct sched_param mysched;

	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts("ERROR IN SETTING THE SCHEDULER");
		perror("errno");
		exit(0);
 	}       
 	if (!(mytask = rt_task_init(nam2num("HRTSK"), 2, 0, 0))) {
		printf("CANNOT INIT TEST BUDDY TASK\n");
		exit(1);
	}
	rt_start_unix_server(mytask, 99, 10000);
	rt_grow_and_lock_stack(4000);

	rt_make_hard_real_time();
	rt_task_use_fpu(mytask, 1);

	rt_printf("(SCANF) Input a: string, integer, long long, float, double: ");
	rt_scanf("%s %i %lld %f %lf", s, &i, &ll, &f, &d);
	rt_printf("(SELECT) Got string, integer, long long, now we wait 1 s using select.\n");
        timout.tv_sec = 1;
        timout.tv_usec = 0;
	rt_select(1, NULL, NULL, NULL, &timout);
	rt_printf("(PRINTF) Select expired and we print what we read: %s %i %lld %f %lf\n", s, i, ll, f, d);
	rt_printf("(OPEN) Open a file.\n");
	fd = rt_open("rtfile", O_RDWR | O_CREAT | O_TRUNC, 0666);
	rt_printf("(WRITE) Write a 50 bytes header of 'B's.\n");
	memset(s, 'B', 50);
	rt_write(fd, s, 50);
	memset(s, 'A', 1000);
	rt_printf("(WRITE) Write 10 MB of 'A's to the opened file.\n");
	t = rt_get_cpu_time_ns();
	for (i = 0; i < 10000; i++) {
		rt_write(fd, s, 1000);
	}
	rt_printf("(PRINTF) WRITE TIME: %lld (ms).\n", (rt_get_cpu_time_ns() - t + 500000)/1000000);
	rt_printf("(WRITE) Write a 50 bytes trailer of 'E's.\n");
	memset(s, 'E', 50);
	rt_write(fd, s, 50);
	rt_printf("(SYNC) Sync file to disk.\n");
	rt_sync();
	rt_printf("(LSEEK) Position file at its beginning.\n");
	rt_lseek(fd, 0, SEEK_SET);
	rt_printf("(READ) Read the first 51 bytes (header plus + 'A').\n");
	rt_read(fd, s, 51);
	s[51] = 0;
	rt_printf("(READ) Here is the header  %s.\n", s);
	rt_lseek(fd, -1, SEEK_CUR);
	rt_printf("(READ) Read the written 10 MB of 'A's back.\n");
	t = rt_get_cpu_time_ns();
	for (i = 0; i < 10000; i++) {
		rt_read(fd, s, 1000);
	}
	rt_printf("(PRINTF) READ TIME %lld (ms).\n", (rt_get_cpu_time_ns() - t + 500000)/1000000);
	rt_lseek(fd, -1, SEEK_CUR);
	rt_printf("(READ) Read the last 51 bytes (last 'A' + trailer).\n");
	rt_read(fd, s, 51);
	s[51] = 0;
	rt_printf("(READ) Here is the trailer %s.\n", s);
	rt_printf("(CLOSE) Close the file and end the test.\n");
	rt_close(fd);
	rt_make_soft_real_time();
	rt_end_unix_server();
	rt_task_delete(mytask);

	return 0;
}
