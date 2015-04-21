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
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sched.h>
#include <stdlib.h>

//#include <asm/rtai.h>
//#include <rtai_sched.h>
//#include <rtai_shm.h>
//#define RT_HIGHEST_PRIORITY 
#define RT_LOWEST_PRIORITY 0x3fffFfff // 3 include above for this rtai_sched.h

#define  KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include "./unix_lxrt.h"

#define USHM (ushm + 1)

static int *ushm;

static int end;

static void endme(int dummy)
{
 	end = 1;
}

int main(int argc, char *argv[])
{
	RT_TASK *unixtsk, *lxrtsk;
	unsigned long shmnam;
	unsigned int opcode;
	int pid, ret = -1, shmsize;
	struct sched_param mysched;

	mysched.sched_priority = atol(argv[3]);
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts("ERROR IN SETTING THE SCHEDULER FOR THE UNIX SERVER");
		exit(1);
 	}       

 	if (!(unixtsk = rt_task_init((nam2num("UT") << 16) | (pid = atol(argv[1])), RT_LOWEST_PRIORITY - mysched.sched_priority, 0, 0))) {
		printf("CANNOT INIT UNIX BUDDY TASK\n");
		exit(1);
	}

	ushm = (void *)rtai_malloc(shmnam = (nam2num("UM") << 16) | + pid, shmsize = atol(argv[4]) + sizeof(RT_TASK *));
	rt_rpc(lxrtsk = (void *)strtoul(argv[2], (char **)NULL, 10), (unsigned int)unixtsk, &opcode);

        signal(SIGINT, endme);
	mlockall(MCL_CURRENT | MCL_FUTURE);

	do {
		rt_receive(lxrtsk, &opcode);
		switch (opcode) {
			case RT_END_UNIX_SRV: {
				rt_return(lxrtsk, 0);
				rtai_free(shmnam, ushm);
 				rt_task_delete(unixtsk);
				return 0;
			}
			case RT_SCANF: {
				ret = (int)fgets((char *)USHM, shmsize, stdin);
				break;
			}
			case RT_PRINTF: {
				ret = printf("%s",(char*)USHM);
				break;
			}
			case RT_OPEN: {
				ret = open((char *)(USHM + 2), USHM[0], USHM[1]);
				break;
			}
			case RT_CLOSE: {
				ret = close(USHM[0]);
				break;
			}
			case RT_WRITE: {
				ret = write(USHM[0], USHM + 2, USHM[1]);
				break;
			}
			case RT_READ: {
				ret = read(USHM[0], USHM + 2, USHM[1]);
				break;
			}
			case RT_SELECT: {
				struct args { int n; fd_set *readfds; fd_set *writefds; fd_set *exceptfds; struct timeval *timeout; fd_set rfds; fd_set wfds; fd_set efds; struct timeval tmo; } *arg;
				arg = (struct args *)USHM;
				ret = select(arg->n, arg->readfds ? &arg->rfds: 0, arg->writefds ? &arg->wfds : 0, arg->exceptfds ? &arg->efds: 0, arg->timeout ? &arg->tmo : 0);
				break;
			}
			case RT_LSEEK: {
				ret = lseek(USHM[0], USHM[1], USHM[2]);
				break;
			}
			case RT_SYNC: {
				sync();
				ret = 0;
				break;
			}
			case RT_IOCTL: {
				ret = ioctl(USHM[0], USHM[1], USHM[2]);
				break;
			}
		}
		rt_return(lxrtsk, ret);
	} while(!end);

	return 0;
}
