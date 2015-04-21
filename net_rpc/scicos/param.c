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
#include <fcntl.h>
#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <net_rpc.h>

#define DEFAULT_PERIOD  10000  // us

#define MAX_PARAM 100
struct arg_value_t { double value; char name[10]; };

int main(int argc, char *argv[])
{
	char minid;
	char name[7];
	struct arg_value_t arg_value[MAX_PARAM];
	struct { int period, startimer, hardmode, stackinc; } startmode = { DEFAULT_PERIOD, 1, 0, 0 };
	unsigned int sysnode, sysport, i, npar;
	struct { int idx; double value; } param;
	RT_TASK *partsk, *systsk;
        struct sockaddr_in addr;

	printf("real time task minor id: ");
	scanf("%c", &minid);
	sprintf(name, "PRTSK%c", minid);
 	if (!(partsk = rt_task_init_schmod(nam2num(name), 2, 0, 0, SCHED_FIFO, 0xF))) {
		printf("CANNOT INIT PARAM TASK: %s.\n", name);
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
	sprintf(name, "TASK%c0", minid);
	if ((systsk = RT_get_adr(sysnode, sysport, name))) {
		RT_rpcx(sysnode, sysport, systsk, &startmode, &npar, sizeof(startmode), sizeof(npar));
		for (i = 0; i < npar; i++) {
			RT_rpcx(sysnode, sysport, systsk, &param, &arg_value[i], 1, sizeof(struct arg_value_t));
		}
		printf("PARAMETERS PREVIEW AND INITIALIZATION.\n");
		while (1) {	
			printf("PARAM TABLE:\n");
			for (i = 0; i < npar; i++) {
				printf("%s  %g\n", arg_value[i].name, arg_value[i].value);
			}  
			printf("index of param to change (0 to start execution): ");
			scanf("%d", &param.idx);
			if (param.idx > 0 && param.idx < (npar + 1)) {
				printf("new value: ");
				scanf("%lf", &param.value);
				arg_value[param.idx - 1].value = param.value;
			} else if (param.idx > 0) {
				continue;
			}
			RT_rpcx(sysnode, sysport, systsk, &param, &i, sizeof(param), 1);
			if (!param.idx) break;
		}
  printf("DEFAULT START MODE:\nPERIOD %d (us), %sSTART THE TIMER, %s REAL TIME, STACK EXPANSION %d BYTES.\n", startmode.period, startmode.startimer ? "" : "DO NOT ", startmode.hardmode ? "HARD" : "SOFT", startmode.stackinc);
		printf("period (int-us, <= 0 use default start mode): ");
		scanf("%d", &startmode.period);
		if (startmode.period > 0) {
			printf("start/nostart timer (1/0)?): ");
			scanf("%d", &startmode.startimer);
			printf("hard/soft execution (1/0)?): ");
			scanf("%d", &startmode.hardmode);
			printf("stack expansion (bytes): ");
			scanf("%d", &startmode.stackinc);
		} else  {
			startmode.period = DEFAULT_PERIOD;
		}
		RT_rpcx(sysnode, sysport, systsk, &startmode, &npar, sizeof(startmode), sizeof(npar));
		while (1) {	
			printf("PARAM TABLE:\n");
			for (i = 0; i < npar; i++) {
				printf("%s  %g\n", arg_value[i].name, arg_value[i].value);
			}  
			printf("index of param to change (0 to end execution): ");
			scanf("%d", &param.idx);
			if (param.idx > 0 && param.idx < (npar + 1)) {
				printf("new value: ");
				scanf("%lf", &param.value);
				arg_value[param.idx - 1].value = param.value;
			} else if (param.idx) {
				continue;
			}
			RT_rpcx(sysnode, sysport, systsk, &param, &i, sizeof(param), 1);
			if (!param.idx) break;
		}
	} else {
		printf("CONTROL TASK %s NOT FOUND.\n", name);
	}
	rt_release_port(sysnode, sysport);
	rt_task_delete(partsk);
	return 0;
}
