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

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

static int end = 0;

static void endme(int dummy) { end = 1; }

static RT_TASK *displogtask;
MBX *logmbx;
#define DIMBUF 1024
char buf[DIMBUF];

int main(void)
{
	unsigned long displogtask_name = nam2num("DISPLOG");
	int k, rec;

	signal(SIGINT, endme);

	if (!(displogtask = rt_task_init(displogtask_name, 30, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(1);
	}

	if ((logmbx = rt_get_adr(nam2num("LOGMBX"))) != 0) { 
		printf("<> DISPLOG Program - Start\n");
		while (!end) {
			if ((rec = DIMBUF - rt_mbx_receive_wp(logmbx, buf, DIMBUF))>0) {
				printf("\nrec=%d\n",rec);
				for(k = 0; k < rec; ++k) {
					if (buf[k]) {
						putchar(buf[k]);
					} else {
						end = 1;
						break;
					}
				}	
			}	
		}
		printf("<> DISPLOG Program - End\n");
	} else {
		printf("<> No LOGMBX mailbox present!\n");
	}		

	rt_task_delete(displogtask);
	exit(0);
}
