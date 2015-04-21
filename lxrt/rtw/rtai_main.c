/*
COPYRIGHT (C) 2001  Paolo Mantegazza  (mantegazza@aero.polimi.it)
                    Giuseppe Quaranta (quaranta@aero.polimi.it)

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


#include <sys/types.h>
#include <sys/io.h>
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include <sys/mman.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include "sysAuxClk.h"

#ifndef DEBUG
#ifndef MODEL
# error "must define MODEL"
#endif
#include "simstruc.h"

extern SimStruct *MODEL(void);
extern SimStruct *rtS;

extern int rt_main(void *, ...);
#else
#define rt_main(a, b, c, d, e, f)
#endif

unsigned int get_an_rtw_name(void)
{
	static int count = 0;
	return nam2num("RTW_A") + count++;
}

#define PRIORITY         1
#define OVR_CHK_STP      5
#define MAX_MSG_SIZE   200
#define LOGMBX_SIZE   5000

static volatile int ovr_chk_stp = OVR_CHK_STP;

static void *overuns_mon_fun(void *args)
{
	float run_time, ovrfreq, check_freq;
	int overuns, scnt;
	struct sched_param mysched;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
        mysched.sched_priority = 99;
        if (sched_setscheduler( 0, SCHED_FIFO, &mysched) < 0) {
		printf("OVERUNS MONITOR CANNOT BECOME SCHED_FIFO.\n");
                return (void *)1; 
        }

#ifdef DBGPRT
	printf("OVERUNS MONITOR RUNNING.\n");
#endif
	run_time = 0.0;
	check_freq = 1.0/ovr_chk_stp;
	while (1) {
		scnt = ovr_chk_stp;
		while (scnt--) {
			sleep(1);
		}
		if (sysAuxClkStatus() > 0) {
			run_time += ovr_chk_stp;
			overuns = sysAuxClkOveruns();
			if ((ovrfreq = overuns/run_time) >= check_freq) {
				printf("AFTER RUNNING %f (s): OVERUNS %d, AVERAGE OVERUNS FREQUENCY: %f.\n", run_time, overuns, ovrfreq);
			}
		} else {
			run_time = 0.0;
		}
	}
	return (void *)0;
}

static MBX *logmbx;

static FILE *logfile;

static void *logMsg_fun(void *args)
{
	RT_TASK *buddy;
	struct { int nch; FILE *fs; } type;
	char msg[MAX_MSG_SIZE];

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
 	if (!(buddy = rt_task_init_schmod(nam2num("LOGSRV"), 1, 0, 0, SCHED_FIFO, 0xFFFFFFFF))) {
		printf("CANNOT INIT LOG MSG SERVER BUDDY TASK %p.\n", buddy);
		return (void *)1;
	}

	rt_send(*(RT_TASK **)args, (unsigned int)buddy);
	while (1) {
		if (!rt_mbx_receive(logmbx, &type, sizeof(type)) && !rt_mbx_receive(logmbx, msg, type.nch)) {
			if (type.fs) {
				fprintf(type.fs, msg);
			} else {
				fprintf(stderr, msg);
				fprintf(logfile, msg);
			}
		} else {
			while(!rt_mbx_receive_wp(logmbx, msg, MAX_MSG_SIZE));
		}
	}

	return (void *)0;
}

struct LOGMSG { int nch; FILE *fs; char msg[MAX_MSG_SIZE]; };

static inline void send_to_mbx(FILE *fs, struct LOGMSG msg)
{
	RT_TASK *buddy;

	msg.msg[MAX_MSG_SIZE - 1] = 0;
	if (++msg.nch >= (MAX_MSG_SIZE - 1)) {
		msg.nch = MAX_MSG_SIZE - 1;
	}
	if (!(buddy = rt_buddy()) || (buddy && !rt_is_hard_real_time(buddy))) {
		fprintf(stderr, msg.msg);
	} else {
		msg.fs = fs;
		rt_mbx_send_if(logmbx, &msg, sizeof(int) + sizeof(FILE *) + msg.nch);
	}
}

void logMsg(char *format, ...)
{
	struct LOGMSG msg;
	va_list args;

	va_start(args, format);
	msg.nch = vsprintf(msg.msg, format, args);
	va_end(args);
	send_to_mbx(0, msg);
	return;
}

int rtai_fprintf(FILE *stdfs, char *format, ...)
{
	struct LOGMSG msg;
	va_list args;

	va_start(args, format);
	msg.nch = vsprintf(msg.msg, format, args);
	va_end(args);
	send_to_mbx(stdfs, msg);
	return msg.nch;
}

/*
* Command line: ./prog_to_run nohrt p=23 o=5 "optStr", whatever order;
* nohrt to run soft realtime (for dubugging?),
* p=priority(1-99) no spaces, 
* o=overuns_check_interval(seconds) no spaces;
* if not given defaults are: hard real time, p=1, o=5.
*/

int use_hrt = 1;

static pthread_t logMsg_thread, overuns_mon_thread;

static 	RT_TASK *rtai_main_buddy;

static void endme(int dummy)
{
	SEM *sem;
#ifndef DEBUG
    ssSetStopRequested(rtS, 1);
    /*printf("stop requested in pid %d\n", getpid());*/
#endif
	if ((sem = sysAuxClkSem(rtai_main_buddy))) {
		rt_sem_signal(sem);
	}
}

int main(int argc, char *argv[])
{
	RT_TASK *buddy, *logmsg_buddy;
	int i, narg, priority;
	char *optStr;

	priority = PRIORITY;
	narg = argc;
	for (i = 0; i < argc; i++) {
		if (strstr(argv[i], "p=")) {
			priority = atoi(argv[i] + 2);
			argv[i] = 0;
			narg--;
			continue;
		}
		if (strstr(argv[i], "o=")) {
			ovr_chk_stp = atoi(argv[i] + 2);
			argv[i] = 0;
			narg--;
			continue;
		}
		if (strstr(argv[i], "nohrt")) {
			use_hrt = 0;
			argv[i] = 0;
			narg--;
			continue;
		}
	}
	if (narg > 2) {
		printf("TOO MANY ARGS IN COMMAND LINE.\n");
		return 1;
	}
	optStr = NULL;
	for (i = 1; i < argc; i++) {
		if (argv[i]) {
			optStr = argv[i];
			break;
		}
	}

        if (optStr == NULL) {
		optStr = "\0";
	}	

	rt_allow_nonroot_hrt();
 	if (!(buddy = rt_task_init_schmod(nam2num("RTWPRG"), 98, 0, 0, SCHED_FIFO, 0xFFFFFFFF))) {
		printf("CANNOT INIT MAIN TASK BUDDY %p.\n", buddy);
		return 1;
	}
#ifdef DBGPRT
	printf("MAIN TASK BUDDY CREATED %p, EXECUTION STARTED.\n", buddy);
#endif

	logfile = fopen("rtw_log", "w");
 	if (!(logmbx = rt_mbx_init(nam2num("LOGMBX"), LOGMBX_SIZE))) {
		printf("CANNOT INIT LOG MSG SERVER SUPPORT MAILBOX %p.\n", logmbx);
		return 1;
	}
#ifdef DBGPRT
	printf("LOG MSG SERVER SUPPORT MAILBOX CREATED %p.\n", logmbx);
#endif

	iopl(3);
	pthread_create(&overuns_mon_thread, NULL, overuns_mon_fun, NULL);
	pthread_create(&logMsg_thread, NULL, logMsg_fun, &buddy);
	rt_receive(0, (unsigned int *)&logmsg_buddy);
 	rtai_main_buddy = buddy;
	signal(SIGINT, endme);
	signal(SIGTERM, endme);

#ifdef DBGPRT
	printf("LOG MSG SERVER BUDDY TASK CREATED %p, LOG MSG SERVER RUNNING.\n", buddy);
	printf ("CALLING RT_MAIN WITH: HRT %d, PRIORITY %d, OVRCHK %d (s), OPTSTR %s.\n", use_hrt, priority, ovr_chk_stp, optStr);
#endif

	rt_main(MODEL, optStr, NULL, 0, priority, 0);

#ifdef DBGPRT
	printf ("RT_MAIN RETURNED.\n");
#endif

	pthread_cancel(overuns_mon_thread);
	pthread_join(overuns_mon_thread, NULL);
#ifdef DBGPRT
	printf("OVERUNS MONITOR STOPPED.\n");
#endif

	rt_task_delete(logmsg_buddy);
	pthread_cancel(logMsg_thread);
	pthread_join(logMsg_thread, NULL);
#ifdef DBGPRT
	printf("LOG MSG SERVER BUDDY TASK %p DELETED, LOG MSG SERVER STOPPED.\n", buddy);
#endif

	rt_mbx_delete(logmbx);
#ifdef DBGPRT
	printf("LOG MSG SERVER SUPPORT MAILBOX %p DELETED.\n", logmbx);
#endif
	rt_task_delete(buddy);
#ifdef DBGPRT
	printf("MAIN TASK BUDDY DELETED %p, EXECUTION TERMINATED.\n", buddy);
	printf("\nTOTAL OVERUNS: %ld.\n", sysAuxClkOveruns());
#endif
	fprintf(logfile, "\nTOTAL OVERUNS: %ld.\n", sysAuxClkOveruns());
	fclose(logfile);
	return 0;
}
