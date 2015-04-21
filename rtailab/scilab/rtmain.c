 /*
  COPYRIGHT (C) 2002  Lorenzo Dozio (dozio@aero.polimi.it)
  Paolo Mantegazza (mantegazza@aero.polimi.it)
  Roberto Bucher (roberto.bucher@supsi.ch)

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <float.h>
#include <math.h>
#include <pthread.h>

#include <sys/mman.h>
#include <sys/poll.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <net_rpc.h>

#define VERSION        "0.0.0"
#define MAX_ADR_SRCH   500
#define MAX_NAME_SIZE  256
#define MAX_SCOPES     100
#define MAX_LOGS       100
#define MAX_LEDS       100
#define MAX_METERS     100
#define HZ             100
#define POLL_PERIOD    100 // millisecs

#define rt_HostInterfaceTaskPriority  96
#define rt_MainTaskPriority	      97

#define MAX_DATA_SIZE           30

typedef struct rtTargetParamInfo {
    char modelName[MAX_NAME_SIZE];
    char blockName[MAX_NAME_SIZE];
    char paramName[MAX_NAME_SIZE];
    unsigned int nRows;
    unsigned int nCols;
    unsigned int dataType;
    unsigned int dataClass;
    double dataValue[MAX_DATA_SIZE];
} rtTargetParamInfo;

typedef struct devStr{
    int nch;
    char sName[20];
    double dParam[5];
    void * ptr;
}devStr;

static pthread_t  rt_HostInterfaceThread, rt_BaseRateThread;
static RT_TASK    *rt_MainTask, *rt_HostInterfaceTask, *rt_BaseRateTask;

static char *HostInterfaceTaskName = "IFTASK";
char *TargetMbxID                  = "RTS";
char *TargetLogMbxID               = "RTL";
char *TargetLedMbxID               = "RTE";
char *TargetMeterMbxID	           = "RTM";
char *TargetSynchronoscopeMbxID    = "RTY";

static volatile int CpuMap       = 0xF;
static volatile int UseHRT       = 1;
static volatile int WaitToStart  = 0;
static volatile int isRunning    = 0;
static volatile int verbose      = 0;
static volatile int endBaseRate  = 0;
static volatile int endInterface = 0;
static volatile int stackinc     = 100000;
static volatile int NUPAR1       = 0;
static volatile int ClockTick    = 0;
static volatile int InternTimer  = 1;
static RTIME rt_BasePeriod       = 100000000/HZ;
static float FinalTime           = 0.0;

static volatile int endex;

static double TIME, *UPAR1;
static struct { char name[MAX_NAME_SIZE]; int ntraces; } rtaiScope[MAX_SCOPES];
static struct { char name[MAX_NAME_SIZE]; int nrow, ncol; } rtaiLogData[MAX_LOGS];
static struct { char name[MAX_NAME_SIZE]; int nleds; } rtaiLed[MAX_LEDS];
static struct { char name[MAX_NAME_SIZE]; int nmeters; } rtaiMeter[MAX_METERS];

#define SS_DOUBLE  0
#define rt_SCALAR  0 

#define msleep(t)  do { poll(0, 0, t); } while (0)

#define MAX_COMEDI_DEVICES      4

void *ComediDev[MAX_COMEDI_DEVICES];
int ComediDev_InUse[MAX_COMEDI_DEVICES] = {0};
int ComediDev_AIInUse[MAX_COMEDI_DEVICES] = {0};
int ComediDev_AOInUse[MAX_COMEDI_DEVICES] = {0};
int ComediDev_DIOInUse[MAX_COMEDI_DEVICES] = {0};

devStr inpDevStr[40];
devStr outDevStr[40];

static void DummyWait(void) { }
static void DummySend(void) { }

// this function is hacked from system.h
static inline void set_double(double *to, double *from) 
{
    unsigned long l = ((unsigned long *)from)[0];
    unsigned long h = ((unsigned long *)from)[1];
    __asm__ __volatile__ (
	"1: movl (%0), %%eax; movl 4(%0), %%edx; lock; cmpxchg8b (%0); jnz 1b" : : "D"(to), "b"(l), "c"(h) : "ax", "dx", "memory");
}

static inline void strncpyz(char *dest, const char *src, int n)
{
    strncpy(dest, src, n);
    dest[n - 1] = '\0';
}

// the following 2 functions are unsafe, in theory, but can be used anyhow 
// since it is very unlikely that two controllers will be started in parallel,
// moreover it is also possible to avoid using them

char *get_a_name(const char *root, char *name)
{
    unsigned long i;
    for (i = 0; i < MAX_ADR_SRCH; i++) {
	sprintf(name, "%s%d", root, i);
	if (!rt_get_adr(nam2num(name))) {
	    return name;
	}
    }
    return 0;
}

static unsigned long get_an_id(const char *root)
{
    char name[7];
    if (get_a_name(root, name)) {
	return nam2num(name);
    }
    return 0;
}

int rtRegisterScope(const char *name, int n)
{
    int i;
    for (i = 0; i < MAX_SCOPES; i++) {
	if (!rtaiScope[i].ntraces) {
	    rtaiScope[i].ntraces = n;
	    strncpyz(rtaiScope[i].name, name, MAX_NAME_SIZE);
	    return 0;
	}
    }
    return -1;
}

int rtRegisterLed(const char *name, int n)
{
    int i;
    for (i = 0; i < MAX_LEDS; i++) {
	if (!rtaiLed[i].nleds) {
	    rtaiLed[i].nleds = n;
	    strncpyz(rtaiLed[i].name, name, MAX_NAME_SIZE);
	    return 0;
	}
    }
    return -1;
}

int rtRegisterMeter(const char *name, int n)
{
    int i;
    for (i = 0; i < MAX_METERS; i++) {
	if (!rtaiMeter[i].nmeters) {
	    rtaiMeter[i].nmeters = n;
	    strncpyz(rtaiMeter[i].name, name, MAX_NAME_SIZE);
	    return 0;
	}
    }
    return -1;
}

static int rtRegisterLogData(const char *name, int nrow, int ncol)
{
    int i;
    for (i = 0; i < MAX_SCOPES; i++) {
	if (!rtaiLogData[i].nrow) {
	    rtaiLogData[i].nrow = nrow;
	    rtaiLogData[i].ncol = ncol;
	    strncpyz(rtaiLogData[i].name, name, MAX_NAME_SIZE);
	    return 0;
	}
    }
    return -1;
}

#if 0  // this avoids annoying libc functions
static unsigned long udn2nl(double dudn)
{
    char ip[16];
    unsigned long r0, r1, r2, r3;

    if (!(r0 = dudn/1000000000.0)) {;
    return 0;
    }
    r1 = ((unsigned long)(dudn/1000000.0))%1000;
    r2 = (r3 = (unsigned long)(dudn/1000.0))%1000;
    r3 = dudn - (double)r3*1000.0;
    sprintf(ip, "%lu.%lu.%lu.%lu", r0, r1, r2, r3);
    return ddn2nl(ip);
}
#else  // this avoids using much FP
static unsigned long udn2nl(double dudn)
{
    char ip[16];
    unsigned long long udn;
    unsigned long r0, r1, r2, r3;

    udn = dudn;
    if (!(r0 = udn/1000000000)) {
	return 0;
    }
    r1 = ((unsigned long)(udn/1000000))%1000;
    r2 = (r3 = (unsigned long)(udn/1000))%1000;
    r3 = udn - r3*1000ULL;
    sprintf(ip, "%lu.%lu.%lu.%lu", r0, r1, r2, r3);
    return ddn2nl(ip);
}
#endif

static void grow_and_lock_stack(int inc)
{
    char c[inc];
    memset(c, 0, inc);
    mlockall(MCL_CURRENT | MCL_FUTURE);
}

static void (*WaitTimingEvent)(unsigned long);
static void (*SendTimingEvent)(unsigned long);
static unsigned long TimingEventArg;

#define XNAME(x,y)  x##y
#define NAME(x,y)   XNAME(x,y)

#define XSTR(x)    #x
#define STR(x)     XSTR(x)

#define MODELNAME  STR(NAME(MODEL, _standalone.c))
#include MODELNAME

static inline int rtModifyRParam(int i, double *param)
{
    if (i >= 0 && i < NRPAR1) {
	set_double(&RPAR1[i], param);
	if (verbose) {
	    printf("RPAR1[%d] : %le.\n", i, RPAR1[i]);
	}
	return 0;
    }
    return -1;
}

static inline int rtModifyIParam(int i, int param)
{
    if (i >= 0 && i < NIPAR1) {
	IPAR1[i] = param;
	if (verbose) {
	    printf("IPAR1[%d] : %d.\n", i, IPAR1[i]);
	}
	return 0;
    }
    return -1;
}

static inline int rtModifyUParam(int i, double *param)
{
    if (i >= 0 && i < NUPAR1) {
	set_double(&UPAR1[i], param);
	if (verbose) {
	    printf("UPAR1[%d] : %le.\n", i, UPAR1[i]);
	}
	return 0;
    }
    return -1;
}

static void *rt_BaseRate(void *args)
{
    RTIME now, t0;
    char name[7] = "BRT";

    strncat(name, TargetMbxID, 3);
    if (!(rt_BaseRateTask = rt_task_init_schmod(nam2num(name), *((int *)args), 0, 0, SCHED_FIFO, CpuMap))) {
	printf("Cannot init rt_BaseRateTask.\n");
	return (void *)1;
    }
    iopl(3);
    rt_task_use_fpu(rt_BaseRateTask, 1);
    NAME(MODEL,_init)(z, &TIME, RPAR1, &NRPAR1, IPAR1, &NIPAR1);
    grow_and_lock_stack(stackinc);
    if (UseHRT) {
	rt_make_hard_real_time();
    }
    rt_send(rt_MainTask, 0);	
    rt_task_suspend(rt_BaseRateTask);
    now = rt_get_time();
    t0 = count2nano(now) + rt_BasePeriod;
    rt_task_make_periodic(rt_BaseRateTask, now + nano2count(rt_BasePeriod), nano2count(rt_BasePeriod));
    while (!endBaseRate) {
	WaitTimingEvent(TimingEventArg);
	if (endBaseRate) break;
	TIME = (rt_get_cpu_time_ns() - t0)*1.0E-9;
	set_nevprt(1);
	NAME(MODEL,main1)(z, &TIME, RPAR1, &NRPAR1, IPAR1, &NIPAR1);
	NAME(MODEL,main2)(z, &TIME, RPAR1, &NRPAR1, IPAR1, &NIPAR1);
    }
    rt_make_soft_real_time();
    NAME(MODEL,_end)(z, &TIME, RPAR1, &NRPAR1, IPAR1, &NIPAR1);
    rt_task_delete(rt_BaseRateTask);
    return 0;
}

static inline void modify_any_param(int index, double param)
{
    if (index < NRPAR1) {
	rtModifyRParam(index, &param);
	NAME(MODEL,_const_update)(z, &TIME, RPAR1, &NRPAR1, IPAR1, &NIPAR1);
    } else if (index < (NRPAR1 + NIPAR1)) {
	rtModifyIParam(index -= NRPAR1, (int)param);
	NAME(MODEL,_upar_update)(index);
    } else {
	rtModifyUParam(index -= (NRPAR1 + NIPAR1), &param);
	NAME(MODEL,_upar_update)(index);
    }
}

static void *rt_HostInterface(void *args)
{
    RT_TASK *task;
    unsigned int Request;
    int Reply, len;

    if (!(rt_HostInterfaceTask = rt_task_init_schmod(nam2num(HostInterfaceTaskName), rt_HostInterfaceTaskPriority, 0, 0, SCHED_RR, 0xFF))) {
	printf("Cannot init rt_HostInterfaceTask.\n");
	return (void *)1;
    }
    while (!endInterface) {
	task = rt_receive(0, &Request);
	if (endInterface) break;
	switch (Request & 0xFF) {
	    case 'c': {
		int i, Idx;
		rtTargetParamInfo rtParam;
		float samplingTime;

		strncpyz(rtParam.modelName, MODELNAME, MAX_NAME_SIZE);
		rtParam.dataType  = SS_DOUBLE;
		rtParam.dataClass = rt_SCALAR;
		rtParam.nRows = 1;
		rtParam.nCols = 1;
		rt_return(task, (isRunning << 16) | ((NRPAR1 + NIPAR1 + NUPAR1) & 0xFFFF));
		strncpyz(rtParam.blockName, MODELNAME"/REAL PARAMS", MAX_NAME_SIZE);
		for (i = 0; i < NRPAR1; i++) {
		    rt_receivex(task, &Request, 1, &len);
		    sprintf(rtParam.paramName, "RPAR1[%d]", i);
		    rtParam.dataValue[0] = RPAR1[i];
		    rt_returnx(task, &rtParam, sizeof(rtParam));
		}
		strncpyz(rtParam.blockName, MODELNAME"/INTEGER PARAMS", MAX_NAME_SIZE);
		for (i = 0; i < NIPAR1; i++) {
		    rt_receivex(task, &Request, 1, &len);
		    sprintf(rtParam.paramName, "IPAR1[%d]", i);
		    rtParam.dataValue[0] = IPAR1[i];
		    rt_returnx(task, &rtParam, sizeof(rtParam));
		}
		strncpyz(rtParam.blockName, MODELNAME"/USER PARAMS", MAX_NAME_SIZE);
		for (i = 0; i < NUPAR1; i++) {
		    rt_receivex(task, &Request, 1, &len);
		    sprintf(rtParam.paramName, "UPAR1[%d]", i);
		    rtParam.dataValue[0] = UPAR1[i];
		    rt_returnx(task, &rtParam, sizeof(rtParam));
		}
		while (1) {
		    rt_receivex(task, &Idx, sizeof(int), &len);
		    if (Idx < 0) {
			rt_returnx(task, &Idx, sizeof(int));
			break;
		    } else {
			rt_returnx(task, &rtaiScope[Idx].ntraces, sizeof(int));
			rt_receivex(task, &Idx, sizeof(int), &len);
			rt_returnx(task, rtaiScope[Idx].name, MAX_NAME_SIZE);
			rt_receivex(task, &Idx, sizeof(int), &len);
			samplingTime = rt_BasePeriod*1.0E-9;
			rt_returnx(task, &samplingTime, sizeof(float));
		    }
		}
		while (1) {
		    rt_receivex(task, &Idx, sizeof(int), &len);
		    if (Idx < 0) {
			rt_returnx(task, &Idx, sizeof(int));
			break;
		    } else {
			rt_returnx(task, &rtaiLogData[Idx].nrow, sizeof(int));
			rt_receivex(task, &Idx, sizeof(int), &len);
			rt_returnx(task, &rtaiLogData[Idx].ncol, sizeof(int));
			rt_receivex(task, &Idx, sizeof(int), &len);
			rt_returnx(task, rtaiLogData[Idx].name, MAX_NAME_SIZE);
			rt_receivex(task, &Idx, sizeof(int), &len);
			samplingTime = rt_BasePeriod*1.0E-9;
			rt_returnx(task, &samplingTime, sizeof(float));
		    }
		}
		while (1) {
		    rt_receivex(task, &Idx, sizeof(int), &len);
		    if (Idx < 0) {
			rt_returnx(task, &Idx, sizeof(int));
			break;
		    } else {
			rt_returnx(task, &rtaiLed[Idx].nleds, sizeof(int));
			rt_receivex(task, &Idx, sizeof(int), &len);
			rt_returnx(task, rtaiLed[Idx].name, MAX_NAME_SIZE);
			rt_receivex(task, &Idx, sizeof(int), &len);
			samplingTime = rt_BasePeriod*1.0E-9;
			rt_returnx(task, &samplingTime, sizeof(float));
		    }
		}
		while (1) {
		    rt_receivex(task, &Idx, sizeof(int), &len);
		    if (Idx < 0) {
			rt_returnx(task, &Idx, sizeof(int));
			break;
		    } else {
			rt_returnx(task, rtaiMeter[Idx].name, MAX_NAME_SIZE);
			rt_receivex(task, &Idx, sizeof(int), &len);
			samplingTime = rt_BasePeriod*1.0E-9;
			rt_returnx(task, &samplingTime, sizeof(float));
		    }
		}
		break;
	    }
	    case 's': {
		rt_task_resume(rt_MainTask);
		rt_return(task, 1);
		break;
	    }
	    case 't': {
		endex = 1;
		rt_return(task, 0);
		break;
	    }
	    case 'p': {
		int index;
		double param;
		int mat_ind;

		rt_return(task, isRunning);
		rt_receivex(task, &index, sizeof(int), &len);
		Reply = 0;
		rt_returnx(task, &Reply, sizeof(int));
		rt_receivex(task, &param, sizeof(double), &len);
		Reply = 1;
		rt_returnx(task, &Reply, sizeof(int));
		rt_receivex(task, &mat_ind, sizeof(int), &len);
		modify_any_param(index, param);
		rt_returnx(task, &Reply, sizeof(int));
		break;			
	    }
	    case 'g': {
		int i;
		double value;
		rt_return(task, isRunning);
		for (i = 0; i < NRPAR1; i++) {
		    rt_receivex(task, &Request, 1, &len);
		    rt_returnx(task, &RPAR1[i], sizeof(double));
		}
		for (i = 0; i < NIPAR1; i++) {
		    rt_receivex(task, &Request, 1, &len);
		    value = IPAR1[i];
		    rt_returnx(task, &value, sizeof(double));
		}
		for (i = 0; i < NUPAR1; i++) {
		    rt_receivex(task, &Request, 1, &len);
		    rt_returnx(task, &UPAR1[i], sizeof(double));
		}
		break;
	    }
	    case 'd': {
		int ParamCnt;
		rt_return(task, isRunning);
		rt_receivex(task, &ParamCnt, sizeof(int), &len);
		Reply = 0;
		rt_returnx(task, &Reply, sizeof(int));
		{
		    struct {
			int index;
			int mat_ind;
			double value;
		    } Params[ParamCnt];
		    int i;
		    rt_receivex(task, &Params, sizeof(Params), &len);
		    for (i = 0; i < ParamCnt; i++) {
			modify_any_param(Params[i].index, Params[i].value);
		    }
		}
		Reply = 1;
		rt_returnx(task, &Reply, sizeof(int));
		break;			
	    }
	    case 'm': {
		float time = TIME;
		rt_return(task, isRunning);
		rt_receivex(task, &Reply, sizeof(int), &len);
		rt_returnx(task, &time, sizeof(float));
		break;
	    }
	    case 'b': {
		rt_return(task, (unsigned int)rt_BaseRateTask);
		break;
	    }
	    default : {
		rt_return(task, 0xFFFFFFFF);
		break;
	    }
	}
    }
    rt_task_delete(rt_HostInterfaceTask);
    return 0;
}

static int rt_Main(int priority)
{
    SEM *hard_timers_cnt;
    char name[7] = "MNT";

    strncat(name, TargetMbxID, 3);
    if (!(rt_MainTask = rt_task_init_schmod(nam2num(name), rt_MainTaskPriority, 0, 0, SCHED_RR, 0xFF))) {
	printf("Cannot init rt_MainTask.\n");
	return 1;
    }
    printf("TARGET STARTS.\n");
    pthread_create(&rt_HostInterfaceThread, NULL, rt_HostInterface, NULL);
    pthread_create(&rt_BaseRateThread, NULL, rt_BaseRate, &priority);
    if (InternTimer) {
	WaitTimingEvent = (void *)rt_task_wait_period;
	if (!(hard_timers_cnt = rt_get_adr(nam2num("HTMRCN")))) {
	    if (!ClockTick) {
		rt_set_oneshot_mode();
	    }
	    start_rt_timer(nano2count(ClockTick));
	    hard_timers_cnt = rt_sem_init(nam2num("HTMRCN"), 0);
	} 
	else {
	    rt_sem_signal(hard_timers_cnt);
	}
    }
    else {
	WaitTimingEvent = (void *)DummyWait;
	SendTimingEvent = (void *)DummySend;
    }

    if (verbose) {
	printf("Model : %s .\n", MODELNAME);
	printf("Executes on CPU map : %x.\n", CpuMap);
	printf("Sampling time : %e (s).\n", rt_BasePeriod*1.0E-9);
    }
    {
	int msg;
	rt_receive(0, &msg);
    }
    if (WaitToStart) {
	if (verbose) {
	    printf("Target is waiting to start ... ");
	    fflush(stdout);
	}
	rt_task_suspend(rt_MainTask);
    }
    if (verbose) {
	printf("Target is running.\n");
    }
    rt_task_resume(rt_BaseRateTask);
    isRunning = 1;
    while (!endex && (!FinalTime || TIME < FinalTime)) {
	msleep(POLL_PERIOD);
    }
    endBaseRate = 1;
    if (!InternTimer) {
	SendTimingEvent(TimingEventArg);
    }
    pthread_join(rt_BaseRateThread, NULL);
    isRunning = 0;
    endInterface = 1;
    rt_send(rt_HostInterfaceTask, 0);
    if (verbose) {
	printf("Target has been stopped.\n");
    }
    pthread_join(rt_HostInterfaceThread, NULL);
    if (InternTimer) {
	if (!rt_sem_wait_if(hard_timers_cnt)) {
//			stop_rt_timer();
	    rt_sem_delete(hard_timers_cnt);
	}
    }
    rt_task_delete(rt_MainTask);
    printf("TARGET ENDS.\n");
    return 0;
}

static struct option options[] = {
    { "usage",      0, 0, 'u' },
    { "verbose",    0, 0, 'v' },
    { "version",    0, 0, 'V' },
    { "soft",       0, 0, 's' },
    { "wait",       0, 0, 'w' },
    { "priority",   1, 0, 'p' },
    { "finaltime",  1, 0, 'f' },
    { "stackinc",   1, 0, 'm' },
    { "samptime",   1, 0, 'r' },
    { "name",       1, 0, 'n' },
    { "mbxid",      1, 0, 'i' },
    { "logid",      1, 0, 'l' },
    { "timertick",  1, 0, 't' },
    { "external",   0, 0, 'e' },
    { "cpumap",     1, 0, 'c' },
    { 0,            0, 0, 0 }
};

static void print_usage(void)
{
    fputs(
	("\nUsage:  rt_main [OPTIONS]\n"
	 "\n"
	 "OPTIONS:\n"
	 "  -u, --usage\n"
	 "      print usage\n"
	 "  -v, --verbose\n"
	 "      verbose output\n"
	 "  -V, --version\n"
	 "      print program version\n"
	 "  -s, --soft\n"
	 "      run program in soft real time\n"
	 "  -w, --wait\n"
	 "      wait to start\n"
	 "  -p <priority>, --priority <priority>\n"
	 "      for the model's highest priority task (default 0)\n"
	 "  -r <sampling time>, --samptime <sampling time>\n"
	 "      set the sampling time (default Linux tick 1.0/HZ (s))\n"
	 "  -f <finaltime>, --finaltime <finaltime>\n"
	 "      set the final time (default inf)\n"
	 "  -m <stack increment>, --stackinc <stack increment>\n"
	 "      set a guaranteed stack size extension (default 0)\n"
	 "  -n <ifname>, --name <ifname>\n"
	 "      set the name of the host interface task (default IFTASK)\n"
	 "  -i <mbxid>, --mbxid <mbxid>\n"
	 "      set the mailboxes identifier (default RTS)\n"
	 "  -l <logid>, --logid <logid>\n"
	 "      set the log mailboxes identifier (default RTL)\n"
	 "  -t <timertick>, --timertick <timertick>\n"
	 "      if not zero the hard timer will run in periodic mode (default oneshot)\n"
	 "  -e, --external\n"
	 "      controller timed by an external resume (default internal clock)\n"
	 "  -c <cpumap>, --cpumap <cpumap>\n"
	 "      (1 << cpunum) on which the controller runs (default: let RTAI choose)\n"
	 "\n")
	, stderr);
    exit(0);
}

static void endme(int dummy)
{
    endex = 1;
}

int main(int argc, char *argv[])
{
    extern char *optarg;
    int c, donotrun = 0, priority = 0;

    signal(SIGINT, endme);
    signal(SIGTERM, endme);

    do {
	c = getopt_long(argc, argv, "euvVswp:f:r:m:n:i:l:t:c:", options, NULL);
	switch (c) {
	    case 'c':
		if ((CpuMap = atoi(optarg)) <= 0) {
		    fprintf(stderr, "-> Invalid CPU map.\n");
		    donotrun = 1;
		}
		break;
	    case 'e':
		InternTimer = 0;
		break;
	    case 'f':
		if (strstr(optarg, "inf")) {
		    FinalTime = 0.0;
		} else if ((FinalTime = atof(optarg)) <= 0.0) {
		    fprintf(stderr, "-> Invalid final time.\n");
		    donotrun = 1;
		}
		break;
	    case 'i':
		TargetMbxID = strdup(optarg);
		break;
	    case 'l':
		TargetLogMbxID = strdup(optarg);
		break;
	    case 'm':
		if ((stackinc = atoi(optarg)) < 0 ) {
		    fprintf(stderr, "-> Invalid stack expansion.\n");
		    donotrun = 1;
		}
		break;
	    case 'n':
		HostInterfaceTaskName = strdup(optarg);
		break;
	    case 'p':
		if ((priority = atoi(optarg)) < 0) {
		    fprintf(stderr, "-> Invalid priority value.\n");
		    donotrun = 1;
		}
		break;
	    case 'r':
		if ((rt_BasePeriod = atoi(optarg)*1000) <= 0) {
		    fprintf(stderr, "-> Invalid control task period.\n");
		    donotrun = 1;
		}
		break;
	    case 's':
		UseHRT = 0;
		break;
	    case 't':
		if ((ClockTick = atoi(optarg)*1000) <= 0) {
		    fprintf(stderr, "-> Invalid periodic tick.\n");
		    donotrun = 1;
		}
		break;
	    case 'u':
		print_usage();
		break;
	    case 'V':
		fprintf(stderr, "Version %s.\n", VERSION);
		return 0;
		break;
	    case 'v':
		verbose = 1;
		break;
	    case 'w':
		WaitToStart = 1;
		break;
	    default:
		if (c >= 0) {
		    donotrun = 1;
		}
		break;
	}
    } while (c >= 0);
    if (verbose) {
	printf("\nTarget settings:\n");
	if (InternTimer) {
	    printf("  Real-time : %s;\n", UseHRT ? "HARD" : "SOFT");
	    if (ClockTick) {
		printf("  Internal Clock Tick : %e (s);\n", ClockTick*1.0E-9);
	    } else {
		printf("  Internal Clock is OneShot;\n");
	    }
	} else {
	    printf("  External timing\n");
	}
	printf("  Priority : %d;\n", priority);
	if (FinalTime > 0) {
	    printf("  Finaltime : %f (s).\n\n", FinalTime);
	} else {
	    printf("  RUN FOR EVER.\n\n");
	}
    }
    if (donotrun) {
	printf("ABORTED BECAUSE OF EXECUTION OPTIONS ERRORS.\n");
	return 1;
    }
    return rt_Main(priority);
}
