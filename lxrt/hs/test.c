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
#include <unistd.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/time.h>

#define  KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#define NUMREC   500
#define RECSIZE  50

int main(void)
{
	char s[RECSIZE];
	int i, fd;
	long long ll;
	float f;
	double d;
	RT_TASK *mytask;
	RTIME t;
	struct timeval timout;

 	if (!(mytask = rt_task_init_schmod(nam2num("HRTSK"), 2, 0, 0, SCHED_FIFO, 0xFF))) {
		printf("CANNOT INIT TEST BUDDY TASK\n");
		exit(1);
	}
	start_rt_timer(nano2count(1000000));
	rt_grow_and_lock_stack(40000);

	rt_make_hard_real_time();
	rt_task_use_fpu(mytask, 1);

	printf("Input a: string, integer, long long, float, double: ");
	rt_task_yield();
	scanf("%s %i %lld %f %lf", s, &i, &ll, &f, &d);
	rt_task_yield();
	printf("Got string, integer, long long, now we wait 1 s using select.\n");
	rt_task_yield();
        timout.tv_sec = 1;
        timout.tv_usec = 0;
	select(1, NULL, NULL, NULL, &timout);
	rt_task_yield();
	printf("Select expired and we print what we read: %s %i %lld %f %f\n", s, i, ll, f, d);
	rt_task_yield();
	printf("Open a file.\n");
	rt_task_yield();
	fd = open("rtfile", O_RDWR | O_CREAT | O_TRUNC, 0666);
	rt_task_yield();
	printf("Write a 50 bytes header of 'B's.\n");
	rt_task_yield();
	memset(s, 'B', 50);
	write(fd, s, 50);
	rt_task_yield();
	memset(s, 'A', RECSIZE);
	printf("Write %d KB of 'A's to the opened file.\n", NUMREC*RECSIZE/1000);
	rt_task_yield();
	t = rt_get_cpu_time_ns();
	for (i = 0; i < NUMREC; i++) {
		write(fd, s, RECSIZE);
		rt_task_yield();
		system("sync");
		rt_task_yield();
	}
	printf("WRITE TIME: %lld (ms).\n", (rt_get_cpu_time_ns() - t + 500000)/1000000);
	rt_task_yield();
	printf("Write a 50 bytes trailer of 'E's.\n");
	rt_task_yield();
	memset(s, 'E', 50);
	write(fd, s, 50);
	rt_task_yield();
	printf("Sync file to disk.\n");
	rt_task_yield();
	printf("Position file at its beginning.\n");
	rt_task_yield();
	lseek(fd, 0, SEEK_SET);
	rt_task_yield();
	printf("Read the first 51 bytes (header plus + 'A').\n");
	rt_task_yield();
	read(fd, s, 51);
	rt_task_yield();
	s[51] = 0;
	printf("Here is the header  %s.\n", s);
	rt_task_yield();
	lseek(fd, -1, SEEK_CUR);
	rt_task_yield();
	printf("Read the written %d KB of 'A's back.\n", NUMREC*RECSIZE/1000);
	rt_task_yield();
	t = rt_get_cpu_time_ns();
	for (i = 0; i < NUMREC; i++) {
		read(fd, s, RECSIZE);
		rt_task_yield();
		system("sync");
		rt_task_yield();
	}
	printf("READ TIME %lld (ms).\n", (rt_get_cpu_time_ns() - t + 500000)/1000000);
	rt_task_yield();
	lseek(fd, -1, SEEK_CUR);
	rt_task_yield();
	printf("Read the last 51 bytes (last 'A' + trailer).\n");
	rt_task_yield();
	read(fd, s, 51);
	rt_task_yield();
	s[51] = 0;
	printf("Here is the trailer %s.\n", s);
	rt_task_yield();
	printf("Close the file and end the test.\n");
	rt_task_yield();
	close(fd);
	rt_task_yield();
	rt_make_soft_real_time();
	stop_rt_timer();
	rt_task_delete(mytask);

	return 0;
}
