/*
COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it)

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
#include <signal.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

int main(int argc, char *argv[])
{
	RT_TASK *mytask, *task;
	int pid;

	if (argc != 2) {
		printf("NO TASK TO TERMINATE, USAGE: ./killhard pid.\n");
		return 0;
	}
 	if (!(mytask = rt_task_init(nam2num("KILTSK"), 1000, 0, 0))) {
		printf("CANNOT INIT HARDKILL TASK.\n");
		exit(1);
	}
	pid = strtol(argv[1], (char **)NULL, 10);
	printf("TERMINATING HARD/SOFT LXRT REAL TIME TASK, PID = %d.\n", pid);
	if ((task = rt_force_task_soft(pid))) {
		while (rt_is_hard_real_time(task));
		rt_task_suspend(task);
		kill(pid, SIGTERM);
		printf("HARD/SOFT LXRT REAL TIME TASK, PID = %d, TERMINATED.\n", pid);
	} else {	
		printf("TASK PID: %d, NOT HARD REAL TIME OR FAKY.\n", pid);
		printf("TRYING TO TERMINATE IT ANYHOW.\n");
		kill(pid, SIGTERM);
	}
	rt_task_delete(mytask);
	return 0;
}
