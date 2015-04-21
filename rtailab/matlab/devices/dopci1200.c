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

#define S_FUNCTION_NAME  dopci1200
#define S_FUNCTION_LEVEL 2

#ifdef MATLAB_MEX_FILE
#include "mex.h"      /* needed for declaration of mexErrMsgTxt */
#endif

#include "simstruc.h"

#ifndef MATLAB_MEX_FILE
#include "asm/io.h"
#include "krt_pci.h"
#include "linux/pci_ids.h"
#endif

#define NUMBER_OF_ARGS         (3)
#define CHANNELS_ARG           ssGetSFcnParam(S,0)
#define PORT_ARG               ssGetSFcnParam(S,1)
#define SAMP_TIME_ARG          ssGetSFcnParam(S,2)

#define CHANNELS_IND           (0)
#define PORT_IND               (0)
#define SAMP_TIME_IND          (0)

#define NO_I_WORKS             (2)
#define BASE_ADDR_I_IND        (0)
#define OUTPORT_I_IND          (1)

#define THRESHOLD              0.5

extern unsigned char dio_control;

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

  if (!ssSetNumOutputPorts(S, 0)) return;  		

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
unsigned int base_dio, base_status, base_pci;
int_T control, port;
unsigned int addr_mat[5];

  get_pci_addr(PCI_VENDOR_ID_CBOARDS,0x0F,addr_mat);

  base_pci    = addr_mat[0];
  base_status = addr_mat[1];
  base_dio    = addr_mat[3];

  port=(int_T)mxGetPr(PORT_ARG)[PORT_IND];

  switch(port){
  case 1:
    dio_control &=0xEF;
    control=0x80 | dio_control;
    outb(control, base_dio+0x07);
    outb(0x00, base_dio+0x04);
    ssSetIWorkValue(S,OUTPORT_I_IND,0x04);
    break;

  case 2:
    dio_control &=0xFD;
    control=0x80 | dio_control;
    outb(control, base_dio+0x07);
    outb(0x00, base_dio+0x05);
    ssSetIWorkValue(S,OUTPORT_I_IND,0x05);
    break;
  }
  ssSetIWorkValue(S,BASE_ADDR_I_IND,(int_T) base_dio);
#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
int_T             i;
InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0);
int_T             num_channels    = ssGetInputPortWidth(S,0);
int_T             base_dio=ssGetIWorkValue(S,BASE_ADDR_I_IND);
int_T             port=ssGetIWorkValue(S,OUTPORT_I_IND);
int_T             output;
real_T            u;

#ifndef MATLAB_MEX_FILE
  output=0;
  for(i=0; i<num_channels; i++){
    u=*uPtrs[i];
    if(u>THRESHOLD) output+= (1 << i);
  }
  outb(output,base_dio+port);
#endif
}

static void mdlTerminate(SimStruct *S)
{
int_T             base_dio=ssGetIWorkValue(S,BASE_ADDR_I_IND);
int_T             port=ssGetIWorkValue(S,OUTPORT_I_IND);

#ifndef MATLAB_MEX_FILE
 outb(0x00,base_dio+port);
#endif
}

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif

