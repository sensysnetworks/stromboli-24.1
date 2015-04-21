/*
COPYRIGHT (C) 2003  Roberto Bucher (roberto.bucher@die.supsi.ch)

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

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <asm/param.h>

#include "net_rpc.h"

#include "krtai2rtw.h"
#include "krtmain.h"

#define TASK_PRIORITY                 1
#define STACK_SIZE                    25000
#define rt_MainTaskPriority           100001
#define rt_HostInterfaceTaskPriority  100000
#define rt_BaseTaskPriority           0

#define MAX_ADR_SRCH                  500
#define POLL_PERIOD                   100000000
#define MAX_NAME_SIZE                 50
#define MAX_SCOPES                    100
#define MAX_LOGS                      100

static RT_TASK *rt_MainTask, *rt_HostInterfaceTask, *rt_BaseRateTask;

extern void * rt_get_adr(unsigned long name);

char *TargetLogMbxID = "RTL";

static char * HostInterfaceTaskName  = "IFTASK";
MODULE_PARM(HostInterfaceTaskName, "s");
char *TargetScopeMbxID    = "RTS";
MODULE_PARM(TargetScopeMbxID, "s");
static RTIME rt_BasePeriod = 100000000/HZ;
static int isRunning = 1;
MODULE_PARM(isRunning, "i");

static volatile int endex;

static RTIME t_samp, tick_period;

#define SS_DOUBLE  0
#define rt_SCALAR  0

static inline void strncpyz(char *dest, const char *src, size_t n)
{
    int i;

    for(i=0;i<n;i++) dest[i]=src[i];
    dest[n-1] = '\0';
}

void *get_a_name(const char *root, char *name)
{
        unsigned long i;
        for (i = 0; i < MAX_ADR_SRCH; i++) {
                sprintf(name, "%s%ld", root, i);
                if (!RT_get_adr(0,0,name)) {
                        return name;
                }
        }
        return 0;
}

int mbx_rt_get_adr(char * TargetScopeID, int nmax)
{
    int i;
    char name[7];

    for (i = 0; i < nmax; i++) {
	sprintf(name, "%s%d", TargetScopeID, i);
	if (!rt_get_adr(nam2num(name))) break;
    }
    return(i);
}

MBX * mbx_rt_mbx_init(char * name,int nsize)
{
    MBX * mbx;
    mbx=(MBX*) RT_typed_named_mbx_init(0,0,name,nsize,FIFO_Q);
    return(mbx);
}

void mbx_rt_mbx_send_if(MBX * mbx, void *msg, int msg_size)
{
    RT_mbx_send_if(0,0,mbx, msg, msg_size);
}

void mbx_rt_mbx_delete(MBX * mbx)
{
    RT_named_mbx_delete(0,0,mbx);
}

unsigned long udn2nl(double dudn)
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

static void (*WaitTimingEvent)(unsigned long);

static void rt_BaseRate(int arg)
{
    RTIME now, t0;

    rt_send(rt_MainTask,0);
    rt_task_suspend(rt_BaseRateTask);
    now=rt_get_time();
    t0=rt_get_cpu_time_ns();
    rt_task_make_periodic(rt_BaseRateTask,
                          now+nano2count(rt_BasePeriod),
                          nano2count(rt_BasePeriod));

    while(1){
        WaitTimingEvent((unsigned long) rt_BaseRateTask);
	update_rtw();
    }
}

static void rt_HostInterface(int arg)
{
    RT_TASK *task;
    int iRequest;
    char Request;
    int len;

    while (1) {
	task = rt_receive(0, &iRequest);
	Request=(char) iRequest;

	switch (Request) {
	    case 'c': {
		unsigned int nBlockParams;
		{
		    int Reply;
		    nBlockParams=get_nBlockParams();
		    Reply = (isRunning << 16) | (nBlockParams & 0xffff);
		    rt_return(task, Reply);
		}
		{
		    int i;
		    rtTargetParamInfo rtParameters;
		    for (i = 0; i < nBlockParams; i++) {
			rt_receivex(task, &rtParameters, sizeof(char), &len);
			rt_GetParameterInfo(&rtParameters, i);
			rt_returnx(task, &rtParameters, sizeof(rtParameters));
		    }
		}
		{
		    int scopeIdx, Reply;
		    float samplingTime;
		    int ntraces;
		    while (1) {
			rt_receivex(task, &scopeIdx, sizeof(int), &len);
			if (scopeIdx < 0) {
			    Reply = scopeIdx;
			    rt_returnx(task, &Reply, sizeof(int));
			    break;
			} else {
			    ntraces = rtGetNumInpP_scope(scopeIdx);
			    rt_returnx(task, &ntraces, sizeof(int));
			    rt_receivex(task, &scopeIdx, sizeof(int), &len);
			    rt_returnx(task, rtGetModelName_scope(scopeIdx),
				       MAX_NAME_SIZE*sizeof(char));
			    rt_receivex(task, &scopeIdx, sizeof(int), &len);
			    samplingTime = rtGetSampT_scope(scopeIdx);
			    rt_returnx(task, &samplingTime, sizeof(float));
			}
		    }
		}
		{
		    int logIdx, Reply;
		    float samplingTime;
		    int nrow, ncol, *dim;
		    while (1) {
			rt_receivex(task, &logIdx, sizeof(int), &len);
			if (logIdx < 0) {
			    Reply = logIdx;
			    rt_returnx(task, &Reply, sizeof(int));
			    break;
			} else {
			    dim = rtGetInpPDim_log(logIdx);
			    nrow = dim[0];
			    ncol = dim[1];
			    rt_returnx(task, &nrow, sizeof(int));
			    rt_receivex(task, &logIdx, sizeof(int), &len);
			    rt_returnx(task, &ncol, sizeof(int));
			    rt_receivex(task, &logIdx, sizeof(int), &len);
			    rt_returnx(task, rtGetModelName_log(logIdx),
				       MAX_NAME_SIZE*sizeof(char));
			    rt_receivex(task, &logIdx, sizeof(int), &len);
			    samplingTime = rtGetSampT_log(logIdx);
			    rt_returnx(task, &samplingTime, sizeof(float));
			}
		    }
		}
		break;
	    }

	    case 's': {
		int Reply = 1;
		rt_task_resume(rt_BaseRateTask);
		isRunning=1;
		rt_return(task, Reply);
		break;
	    }

	    case 't': {
		endex=1;
		int Reply = 0;
		rt_return(task, Reply);
		break;
	    }

	    case 'p': {
		int index;
		int Reply;
		double newv;

		Reply = isRunning;
		rt_return(task, Reply);
		Reply = 0;
		rt_receivex(task, &index, sizeof(int), &len);
		rt_returnx(task, &Reply, sizeof(int));
		Reply = 1;
		rt_receivex(task, &newv, sizeof(double), &len);
		rt_ModifyParameterValue(index, &newv);
		rt_returnx(task, &Reply, sizeof(int));
		break;
	    }

	    case 'g': {
		int i;
		unsigned int nBlockParams;
		rtTargetParamInfo rtParameters;
		rt_return(task, isRunning);
		nBlockParams=get_nBlockParams();
		for (i = 0; i < nBlockParams; i++) {
		    rt_receivex(task, &rtParameters, sizeof(char), &len);
		    rt_GetParameterInfo(&rtParameters, i);
		    rt_returnx(task, &rtParameters, sizeof(rtParameters));
		}
		break;
	    }

	    case 'd': {
		int ParamCnt;
		int Reply;

		Reply = isRunning;
		rt_return(task, Reply);
		Reply = 0;
		rt_receivex(task, &ParamCnt, sizeof(int), &len);
		rt_returnx(task, &Reply, sizeof(int));

		{
		    int i;
		    struct {
			int index;
			double value;
		    } BatchParams[ParamCnt];

		    Reply = 1;
		    rt_receivex(task, &BatchParams, sizeof(BatchParams), &len);
		    for (i = 0; i < ParamCnt; i++) {
			rt_ModifyParameterValue(BatchParams[i].index, &BatchParams[i].value);
		    }
		}
		rt_returnx(task, &ParamCnt, sizeof(int));
		break;
	    }

	    case 'm': {
		float ttime = rtGetT();
		int Reply = isRunning;
		rt_return(task, Reply);
		rt_receivex(task, &Reply, sizeof(int), &len);
		rt_returnx(task,&ttime,sizeof(float));
		break;
	    }

	    default: {
		break;
	    }
	}
    }
}

static void rt_Main(int arg)
{
    SEM * hard_timers_cnt;

    t_samp=rt_BasePeriod;
    if(!(hard_timers_cnt=rt_get_adr(nam2num("HTMRCN")))){
        tick_period=start_rt_timer(nano2count(t_samp));
        hard_timers_cnt=RT_typed_named_sem_init(0,0,"HTMRCN",0,CNT_SEM);
    }
    else{
        RT_sem_signal(0,0,hard_timers_cnt);
    }
    {
	int msg;
	rt_receive(0, &msg);
    }
    while(!endex){
	rt_sleep(nano2count(POLL_PERIOD));
    }
    if(!rt_sem_wait_if(hard_timers_cnt)){
        stop_rt_timer();
        RT_named_sem_delete(0,0,hard_timers_cnt);
    }
}

int start_tasks()
{
    char name[7]="MNT";

    start_rtw();

    rt_BasePeriod = get_t_samp();

    strncat(name,TargetScopeMbxID,3);
    if (!(rt_MainTask = RT_named_task_init(0, 0, name, rt_Main, 0, STACK_SIZE,
					   rt_MainTaskPriority, 1, 0))) {
	rt_printk("Cannot init rt_MainTask.\n");
	return 1;
    }
    endex=0;
    rt_task_resume(rt_MainTask);
    rt_printk("TARGET STARTS.\n");

    if (!(rt_HostInterfaceTask = RT_named_task_init(0,0,
						    HostInterfaceTaskName,
						    rt_HostInterface, 0,
                                                    STACK_SIZE,
						    rt_HostInterfaceTaskPriority,
						    1, 0))) {
	rt_printk("Cannot init rt_HostInterfaceTask.\n");
	return 1;
    }
    rt_task_resume(rt_HostInterfaceTask);

    strcpy(name,"BRT");
    strncat(name,TargetScopeMbxID,3);
    if (!(rt_BaseRateTask = RT_named_task_init(0,0, name,
					       rt_BaseRate, 0,
                                               STACK_SIZE,
					       rt_BaseTaskPriority, 0, 0))) {
	rt_printk("Cannot init rt_BaseRateTask.\n");
	return 1;
    }
    rt_task_use_fpu(rt_BaseRateTask,1);
    rt_task_resume(rt_BaseRateTask);

    WaitTimingEvent = (void *)rt_task_wait_period;

    rt_printk("rt_process loaded with period %ld\n",rt_BasePeriod);
    rt_printk("Target is running.\n");
    if(isRunning) rt_task_resume(rt_BaseRateTask);

    return(0);
}


int init_module(void)
{
    return (start_tasks());
}

void cleanup_module(void)
{
    stop_rtw();
    RT_named_task_delete(0, 0, rt_HostInterfaceTask);
    RT_named_task_delete(0, 0, rt_BaseRateTask);
    RT_named_task_delete(0,0,rt_MainTask);
    rt_printk("TARGET ENDS\n");
    rt_printk("rt_process unloaded\n");
}

MODULE_LICENSE("GPL");
