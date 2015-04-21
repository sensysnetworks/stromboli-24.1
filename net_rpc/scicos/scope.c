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
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <net_rpc.h>

static volatile int end;

static void *endme(void *args)
{
	getchar();
	end = 1;
	return 0;
}

static pthread_t thread;

int main(int argc, char *argv[])
{
	char minid;
	char name[7];
	unsigned int sysnode, sysport, nh, i;
        struct { double t, h[10]; } th;
	RT_TASK *scptsk;
	MBX *mbx;
        struct sockaddr_in addr;

//	printf("#real time task minor id: ");
	minid = getchar();
	getchar();
        sprintf(name, "SCTSK%c", minid);
        pthread_create(&thread, NULL, endme, NULL);
 	if (!(scptsk = rt_task_init(nam2num(name), 3, 0, 0))) {
		printf("CANNOT INIT SCOPE TASK: %s.\n", name);
		exit(1);
	}
        sysnode = 0;
        if (argc == 2 && strstr(argv[1], "SysNode=")) {
                inet_aton(argv[1] + 8, &addr.sin_addr);
                sysnode = addr.sin_addr.s_addr;
        }
        if (!sysnode) {
                inet_aton("127.0.0.1", &addr.sin_addr);
                sysnode = addr.sin_addr.s_addr;
        }
	while ((sysport = rt_request_port(sysnode)) <= 0 && sysport != -EINVAL);
	sprintf(name, "SCOP%c0", minid);
	if ((mbx = RT_get_adr(sysnode, sysport, name))) {
		while (!end) {	
		RT_mbx_receive(sysnode, sysport, mbx, &nh, sizeof(int));
		RT_mbx_receive(sysnode, sysport, mbx, &th, (nh + 1)*sizeof(double));
			printf("%g ", th.t); 
			for (i = 0; i < nh; i++) {
				printf("%g ", th.h[i]); 
			}
			printf("\n"); 
		}
	} else {
		printf("SCOPE MAIL BOX %s NOT FOUND.\n", name);
	}
	rt_release_port(sysnode, sysport);
	rt_task_delete(scptsk);
	return 0;
}
