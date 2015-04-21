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


#define S_FUNCTION_NAME  adint
#define S_FUNCTION_LEVEL 2 

#ifdef MATLAB_MEX_FILE
#include "mex.h"      /* needed for declaration of mexErrMsgTxt */
#endif

#include "simstruc.h"


#define NUMBER_OF_ARGS         (3)
#define NUM_CHANNELS_ARG       ssGetSFcnParam(S,0)
#define FIRST_CHANNEL_ARG      ssGetSFcnParam(S,1)  
#define SAMP_TIME_ARG          ssGetSFcnParam(S,2)

#define NUM_CHANNELS_IND       (0)
#define SAMP_TIME_IND          (0)
#define FIRST_CHAN_IND         (0)

#define NO_I_WORKS             (1)
#define BASE_ADDR_I_IND        (0)

#ifndef MATLAB_MEX_FILE
#include "intelligent.h"
#endif


static void mdlInitializeSizes(SimStruct *S)
{
int_T num_channels;

  ssSetNumSFcnParams(S, NUMBER_OF_ARGS);
  if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
    return; /* Parameter mismatch will be reported by Simulink */
  }

  num_channels=(int_T) mxGetPr(NUM_CHANNELS_ARG)[NUM_CHANNELS_IND];

  if (!ssSetNumInputPorts(S, 0)) return;
  if (!ssSetNumOutputPorts(S,1)) return;
  ssSetOutputPortWidth(S, 0, num_channels);

  ssSetNumContStates(S, 0);
  ssSetNumDiscStates(S, 0);
  ssSetNumSampleTimes(S, 1);
  ssSetNumIWork(S,NO_I_WORKS);
  
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
  int_T first_chan;
   
  first_chan = (int_T) mxGetPr(FIRST_CHANNEL_ARG)[FIRST_CHAN_IND] - 1;
  ssSetIWorkValue(S,BASE_ADDR_I_IND,(int_T) first_chan);
  setup(first_chan);
#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
real_T            *y = ssGetOutputPortRealSignal(S,0);
int_T             num_channels = ssGetOutputPortWidth(S,0);
int_T             first_chan = ssGetIWorkValue(S,BASE_ADDR_I_IND);
int               i;
int               val[16];

#ifndef MATLAB_MEX_FILE

  read_adc(first_chan, num_channels, val);
  for(i=0; i<num_channels; i++){
        *y++=(real_T) i2v(val[i]);
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
