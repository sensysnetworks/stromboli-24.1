/*
  COPYRIGHT (C) 2002  Lorenzo Dozio (dozio@aero.polimi.it)
  Paolo Mantegazza (mantegazza@aero.polimi.it)

  Modified 15.1.2003 Roberto Bucher bucher@die.supsi.ch

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


#define S_FUNCTION_NAME		sfun_rtai_scope
#define S_FUNCTION_LEVEL	2

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
#include "simstruc.h"

#define NUMBER_OF_PARAMS	(2)
#define NUM_CHANNELS_PARAM	ssGetSFcnParam(S,0)
#define SAMPLE_TIME_PARAM	ssGetSFcnParam(S,1)
#define NUM_CHANNELS		((uint_T) mxGetPr(NUM_CHANNELS_PARAM)[0])
#define SAMPLE_TIME             ((real_T) mxGetPr(SAMPLE_TIME_PARAM)[0])

#ifndef MATLAB_MEX_FILE

#ifdef KRTAI
#include "net_rpc.h"
#include "krtmain.h"
#else
#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#endif

#define MAX_RTAI_SCOPES               1000
#define MBX_RTAI_SCOPE_SIZE           5000
extern SimStruct *rtaiScope[];
extern char *TargetScopeMbxID;

#endif

static void mdlInitializeSizes(SimStruct *S)
{
	uint_T i;
	uint_T nchan = NUM_CHANNELS;

	ssSetNumSFcnParams(S, NUMBER_OF_PARAMS);
	if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
		return;
	}
	for (i = 0; i < NUMBER_OF_PARAMS; i++) {
		ssSetSFcnParamNotTunable(S, i);
	}
	if (!ssSetNumInputPorts(S, nchan)) return;
	ssSetNumOutputPorts(S, 0);
	for (i = 0; i < nchan; i++) {
		ssSetInputPortWidth(S, i, 1);
	        ssSetInputPortDirectFeedThrough(S, i, 1);
	}
	ssSetNumContStates(S, 0);
	ssSetNumDiscStates(S, 0);
	ssSetNumSampleTimes(S, 1);
	ssSetNumPWork(S, 1);
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
	ssSetSampleTime(S, 0, SAMPLE_TIME);
	ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_START 
static void mdlStart(SimStruct *S)
{
#ifndef MATLAB_MEX_FILE
	char name[7];
	MBX *mbx;
	int i;
	int n = NUM_CHANNELS;

#ifdef KRTAI
	i = mbx_rt_get_adr(TargetScopeMbxID,MAX_RTAI_SCOPES);
	sprintf(name,"%s%d",TargetScopeMbxID,i);
	rtaiScope[i] = S;
	mbx = mbx_rt_mbx_init(name,(MBX_RTAI_SCOPE_SIZE/(n*sizeof(float))*(n*sizeof(float))));
#else
	for (i = 0; i < MAX_RTAI_SCOPES; i++) {
		sprintf(name, "%s%d", TargetScopeMbxID, i);
		if (!rt_get_adr(nam2num(name))) break;
	}
	rtaiScope[i] = S;
	if (!(mbx = (MBX *)rt_mbx_init(nam2num(name), (MBX_RTAI_SCOPE_SIZE/(n*sizeof(float))*(n*sizeof(float)))))) {
		printf("Cannot init mailbox\n");
		exit(1);
	}
#endif
	ssGetPWork(S)[0]= (void *)mbx;
#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
	InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0);
	struct {
		float t;
		float u[NUM_CHANNELS];
	} data;
	int i;

#ifndef MATLAB_MEX_FILE
	MBX *mbx = (MBX *)ssGetPWorkValue(S,0);
	data.t = ssGetT(S);
	for (i = 0; i < NUM_CHANNELS; i++) {
		data.u[i] = (float)*uPtrs[i];
	}
#ifdef KRTAI
	mbx_rt_mbx_send_if(mbx, &data, sizeof(data));
#else
	rt_mbx_send_if(mbx, &data, sizeof(data));
#endif
#endif
}

static void mdlTerminate(SimStruct *S)
{
#ifndef MATLAB_MEX_FILE
	MBX *mbx = (MBX *)ssGetPWorkValue(S,0);
#ifdef KRTAI
	mbx_rt_mbx_delete(mbx);
#else
	rt_mbx_delete(mbx);
#endif
#endif
}

#ifdef  MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
