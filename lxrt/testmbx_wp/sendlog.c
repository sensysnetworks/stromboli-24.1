/*
COPYRIGHT (C) 2001  Giuseppe Renoldi (giuseppe@renoldi.org)

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

#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <signal.h>
#include <string.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

static int end = 0;

static void endme(int dummy) { end = 1; }

static RT_TASK *sendlogtask;
MBX *logmbx;
#define DIMBUF 1024
char sendbuf[DIMBUF];

int main(int argc, char* argv[])
{
	unsigned long sendlogtask_name = nam2num("SENDLOG");
	//int k, rec;

	signal(SIGINT, endme);

	logmbx = rt_mbx_init(nam2num("LOGMBX"),DIMBUF);

	if (!(sendlogtask = rt_task_init(sendlogtask_name, 30, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(1);
	}

	printf("<> SENDLOG Program - Start\n");
	while (!end) {
		printf("message to send: ");
		scanf("%s",sendbuf);
		if (strlen(sendbuf) == 0) {
			break;
		}
		strcat(sendbuf,"\n");
		rt_mbx_send_wp(logmbx, sendbuf, strlen(sendbuf));
	}
	printf("<> SENDLOG Program - End\n");

	rt_mbx_delete(logmbx);
	rt_task_delete(sendlogtask);

	return 0;
}
