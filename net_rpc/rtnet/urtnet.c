/*
COPYRIGHT (C) 2001  Paolo Mantegazza (mantegazza@aero.polimi.it),

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
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <asm/rtai_srq.h>
#include <rtai_shm.h>
#include <softrtnet.h>

#define PTOUT  500  // millisecs
#define ADRSZ  sizeof(struct sockaddr_in)

static struct sock_t *socks;
static int srq, *runsock;

static void *recv_fun(void *thread_arg)
{
	struct sched_param mysched;
	int i, revents, arg[2] = { 0, };
	struct pollfd polls[MAX_STUBS + MAX_SOCKS];

        mysched.sched_priority = sched_get_priority_max(SCHED_RR);
	if (sched_setscheduler(0, SCHED_RR, &mysched) == -1) {
		puts("ERROR IN SETTING URTNET RECV SERV TO SCHED_FIFO\n");
        	return (void *)1;
 	}       
	for (i = 0; i < (MAX_STUBS + MAX_SOCKS); i++) {
		polls[i].fd = socks[i].sock;
		polls[i].events = POLLIN;
	}
	mlockall(MCL_CURRENT | MCL_FUTURE);

	while (runsock[0]) {
		if ((revents = poll(polls, MAX_STUBS + MAX_SOCKS, PTOUT)) > 0) {
			i = -1;
			do {
				while (!polls[++i].revents);
				socks[i].addrlen = ADRSZ;
				socks[i].recv = recvfrom(socks[i].sock, socks[i].msg, MAX_MSG_SIZE, 0, &socks[i].addr, &socks[i].addrlen);
				arg[1] = i;
				rtai_srq(srq, (unsigned int)arg);
			} while (--revents);
		}
	}
        return (void *)0;
}

void endme (int dummy) { runsock[0] = 0; }

static pthread_t recv_thread;

int main(void)
{
	struct sched_param mysched;
	int i, arg[2] = { 1, 0 };

	signal(SIGTERM, endme);
	signal(SIGKILL, endme);
	signal(SIGINT,  endme);

        mysched.sched_priority = sched_get_priority_max(SCHED_RR);
	if (sched_setscheduler(0, SCHED_RR, &mysched) == -1) {
		puts("ERROR IN SETTING URTNET SEND SERVER (MAIN) TO SCHED_FIFO\n");
		return 1;
 	}       
        socks = (struct sock_t *)rtai_malloc(0xcacca0, (MAX_STUBS + MAX_SOCKS)*sizeof(struct sock_t) + 2*sizeof(int));
	(runsock = (int *)(socks + MAX_STUBS + MAX_SOCKS))[0] = 1;
	for (i = 0; i < (MAX_STUBS + MAX_SOCKS); i++) {
		socks[i].sock = socket(AF_INET, SOCK_DGRAM, 0);
                fcntl(socks[i].sock, O_NONBLOCK);
		bind(socks[i].sock, &socks[i].bindaddr, ADRSZ);
	}
	srq = rtai_open_srq(0xcacca1);
	if (pthread_create(&recv_thread, NULL, recv_fun, NULL)) {
		printf("ERROR IN CREATING RECV SERVER\n");
                return 1;
        }
	mlockall(MCL_CURRENT | MCL_FUTURE);

	while (runsock[0]) {
		if ((i = rtai_srq(srq, (unsigned int)arg)) >= 0) {
			sendto(socks[i].sock, socks[i].msg, socks[i].len, 0, &socks[i].addr, ADRSZ);
		}
	}
	pthread_join(recv_thread, NULL);
	for (i = 0; i < (MAX_STUBS + MAX_SOCKS); i++) {
		shutdown(socks[i].sock, 2);
	}
	runsock[0] = 1;
	rtai_free(0xcacca0, socks);
	return 0;
}
