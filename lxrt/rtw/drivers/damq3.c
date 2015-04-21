/*
COPYRIGHT (C) 2001  Giuseppe Quaranta (quaranta@aero.polimi.it)

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


#define S_FUNCTION_NAME  damq3
#define S_FUNCTION_LEVEL 2

#ifdef MATLAB_MEX_FILE
#include "mex.h"      /* needed for declaration of mexErrMsgTxt */
#endif

#include "simstruc.h"


#define NUMBER_OF_ARGS         (4)
#define CHANNELS_ARG           ssGetSFcnParam(S,0)
#define SAMP_TIME_ARG          ssGetSFcnParam(S,1)
#define MAX_SAT_ARG            ssGetSFcnParam(S,2)
#define MIN_SAT_ARG            ssGetSFcnParam(S,3)


#define CHANNELS_IND           (0)
#define SAMP_TIME_IND          (0)
#define MAX_SAT_IND            (0)
#define MIN_SAT_IND            (0)

#define NO_R_WORKS             (2)
#define BASE_ADDR_R_IND        (0)

#ifndef MATLAB_MEX_FILE
#include "mq3.h"
#endif

static void mdlInitializeSizes(SimStruct *S)
{
int_T num_channels;

  ssSetNumSFcnParams(S, NUMBER_OF_ARGS);
  if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
    return; /* Parameter mismatch will be reported by Simulink */
  }

  num_channels=(int_T) mxGetPr(CHANNELS_ARG)[CHANNELS_IND];

  if (!ssSetNumInputPorts(S, 1)) return;
  ssSetInputPortWidth(S, 0, num_channels);
  ssSetInputPortDirectFeedThrough(S, 0, 1);

  if (!ssSetNumOutputPorts(S, 0)) return;  		// !!

  ssSetNumContStates(S, 0);
  ssSetNumDiscStates(S, 0);
  ssSetNumSampleTimes(S, 1);
  ssSetNumRWork(S,NO_R_WORKS);
  
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
  ssSetSampleTime(S, 0, mxGetPr(SAMP_TIME_ARG)[SAMP_TIME_IND]);
  ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_START 
static void mdlStart(SimStruct *S)
{
#ifndef MATLAB_MEX_FILE
  real_T min_sat, max_sat;
  
  max_sat = (real_T) mxGetPr(MAX_SAT_ARG)[MAX_SAT_IND];
  min_sat = (real_T) mxGetPr(MIN_SAT_ARG)[MIN_SAT_IND];
  ssSetRWorkValue(S,BASE_ADDR_R_IND,(real_T) max_sat);  
  ssSetRWorkValue(S,BASE_ADDR_R_IND+1,(real_T) min_sat);
  setup();
#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
int               i,res;
InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0);
int_T             num_channels    = ssGetInputPortWidth(S,0);
int_T             max_sat  = ssGetRWorkValue(S,BASE_ADDR_R_IND);
int_T             min_sat  = ssGetRWorkValue(S,BASE_ADDR_R_IND+1);
real_T            u;

#ifndef MATLAB_MEX_FILE
  for(i=0; i<num_channels; i++){
    u=*uPtrs[i];
    if(u>max_sat) u=max_sat;
    if(u<-min_sat) u=-min_sat;
    res = v2i(u);
    write_dac(i, res);
  }
#endif
}

static void mdlTerminate(SimStruct *S)
{
#ifndef MATLAB_MEX_FILE
  reset();  
#endif
}

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
