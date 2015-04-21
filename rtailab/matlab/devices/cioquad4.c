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

#define S_FUNCTION_NAME  cioquad4
#define S_FUNCTION_LEVEL 2

#ifdef MATLAB_MEX_FILE
#include "mex.h"      /* needed for declaration of mexErrMsgTxt */
#endif

#include "simstruc.h"

#ifndef MATLAB_MEX_FILE
#include "asm/io.h"
#endif

#define NUM_PARAMS             (7)
#define BASE_ADDRESS_ARG       (ssGetSFcnParam(S,0))
#define MODULE_ARG             (ssGetSFcnParam(S,1))
#define RES_ARG                (ssGetSFcnParam(S,2))
#define MODE_ARG               (ssGetSFcnParam(S,3))
#define COUNT_ARG	       (ssGetSFcnParam(S,4))
#define ROT_ARG                (ssGetSFcnParam(S,5))
#define SAMP_TIME_ARG          (ssGetSFcnParam(S,6))

#define NO_I_WORKS             (4)
#define BASE_ADDR_I_IND        (0)
#define FIRSTINDEX_I_IND       (1)
#define TURNS_I_IND	       (2)
#define MODE_I_IND             (3)

#define NO_R_WORKS             (0)

#define RLD 0x00
#define CMR 0x20
#define IOR 0x40
#define IDR 0x60

#define PI	3.14159265358979

#define RESET_DETECT 0x800000
#define INDEX_DETECT 0x000000

static void mdlInitializeSizes(SimStruct *S)
{
int i;

  ssSetNumSFcnParams(S, NUM_PARAMS);
  if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
    return; /* Parameter mismatch will be reported by Simulink */
  }

  ssSetNumContStates(S, 0);
  ssSetNumDiscStates(S, 0);

  if (!ssSetNumInputPorts(S, 0)) return;

  ssSetNumOutputPorts(S, 3);
  for (i=0;i<3;i++) ssSetOutputPortWidth(S, i, 1);

  ssSetNumSampleTimes(S, 1);
  ssSetNumIWork(S, NO_I_WORKS);
  ssSetNumRWork(S, NO_R_WORKS);
  ssSetNumPWork(S, 0);
  ssSetNumModes(S, 0);
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
  ssSetSampleTime(S, 0, mxGetPr(SAMP_TIME_ARG)[0]);
  ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_START  
#if defined(MDL_START) 
static void mdlStart(SimStruct *S)
{
  int   mode;
  int_T baseAddrG = mxGetPr(BASE_ADDRESS_ARG)[0];
  int_T baseAddr;

#ifndef MATLAB_MEX_FILE

  ssSetIWorkValue(S,FIRSTINDEX_I_IND , 0);

  mode = mxGetPr(MODE_ARG)[0];

  switch(mode){
  case 1:
  case 2:
    ssSetIWorkValue(S,MODE_I_IND , mode);
    break;
  case 3:
    ssSetIWorkValue(S,MODE_I_IND , 4);
    break;
  }

  baseAddr = baseAddrG + ((mxGetPr(MODULE_ARG)[0]-1) * 2);
  ssSetIWorkValue(S,BASE_ADDR_I_IND , baseAddr);

  outb(CMR | (mode << 3),baseAddr + 0x01);   // Set mode SINGLE, DOUBLE,
                                             // QUADRUPLE */
  outb(RLD | 0x04,baseAddr + 0x01);          // RESET BT, CT, CPT, S
  outb(RLD | 0x06,baseAddr + 0x01);          // RESET E  

  outb(RLD | 0x01,baseAddr + 0x01);          // RESET BP  

  outb(INDEX_DETECT & 0x0000ff,baseAddr);         // PRESET INDEX_DETECT 
  outb((INDEX_DETECT >> 8) & 0x0000ff,baseAddr);  
  outb((INDEX_DETECT >> 16) & 0x0000ff,baseAddr);

  outb(IOR | 0x00,baseAddr + 0x01);   // DISABLE A/B 
  outb(IDR | 0x03,baseAddr + 0x01);   // ENABLE INDEX POSITIVE

  // Now : Index is enabled, positive and 
  //       Load CNTR Input

  outb(0x0f,baseAddrG + 0x08);         // Index TO LCNTR
  outb(0x00,baseAddrG + 0x09);         // 4x 24 Bit Counter
  outb(0x00,baseAddrG + 0x12);         // DISABLE INTERRUPT
  outb(RLD | 0x08,baseAddr + 0x01);   // PRESET TO COUNTER
  outb(RLD | 0x10,baseAddr + 0x01);   // COUNTER TO LATCH

  outb(RLD | 0x01,baseAddr + 0x01);   // RESET BP

  outb(RESET_DETECT & 0x0000ff,baseAddr);
  outb((RESET_DETECT >> 8) & 0x0000f,baseAddr);
  outb((RESET_DETECT >> 16) & 0x0000ff,baseAddr);

#endif /* MATLAB_MEX_FILE */
}
#endif /*  MDL_START */

static void mdlOutputs(SimStruct *S, int_T tid)
{
  real_T        *y, *y2, *y3;
  int_T        enc_flags, cntrout;
  int_T        tmpout;
  int_T        tmpres = mxGetPr(RES_ARG)[0];
  int_T        baseAddr  = ssGetIWorkValue(S,BASE_ADDR_I_IND);
  int_T        quad_mode = ssGetIWorkValue(S,MODE_I_IND);
  int_T        firstindex = ssGetIWorkValue(S,FIRSTINDEX_I_IND);
  int_T        rotation = mxGetPr(ROT_ARG)[0];
  int_T        counter  = mxGetPr(COUNT_ARG)[0];
  real_T       angle;
  int_T        turns;
 
#ifndef MATLAB_MEX_FILE
  y         = ssGetOutputPortRealSignal(S,0);
  y2        = ssGetOutputPortRealSignal(S,1);
  y3        = ssGetOutputPortRealSignal(S,2);

  enc_flags = inb(baseAddr + 0x01);  // READ FLAGS

  outb(RLD | 0x10,baseAddr + 0x01);  // RESET FLAGS
  outb(RLD | 0x01,baseAddr + 0x01);  // RESET BP

  cntrout = inb(baseAddr) & 0x00ff;     // READ COUNTER
  cntrout = cntrout | ((inb(baseAddr) & 0x00ff) * 0x100);
  cntrout = cntrout | ((inb(baseAddr) & 0x00ff) * 0x10000);

  tmpout  = cntrout;

  if ((enc_flags & 0x03) != 0x00){
      firstindex=0;
      ssSetIWorkValue(S,FIRSTINDEX_I_IND , firstindex);
      outb(IOR | 0x00,baseAddr + 0x01); // DISABLE COUNTER
  }
  
  if (!firstindex) {
//      if (enc_flags & 0x04){
      if(abs(tmpout-INDEX_DETECT)>(2*tmpres*quad_mode)){
	  firstindex=1;
          ssSetIWorkValue(S,FIRSTINDEX_I_IND , firstindex);
         
	  if(counter == 2) outb(IOR | 0x01,baseAddr + 0x01); // ENABLE AB
	                                                     // LOAD CNTR input
	  else             outb(IOR | 0x03,baseAddr + 0x01); // ENABLE AB
	                                                     // LOAD OL input
      }
      else { 
          y[0]  = 0.0;
          y2[0] = 0.0;
          y3[0] = 0.0;
      }
  }
  else{
      tmpout-=RESET_DETECT;
  
      angle = (double) (tmpout)/((double)tmpres*quad_mode) * 2.0 * PI;
      turns = tmpout / (tmpres * quad_mode);

      if (rotation == 2) {
        turns = -turns;
        angle = -angle;
      }  
      y[0]  = angle;
      y2[0] = turns;
      y3[0] = firstindex;
  }

#endif /* MATLAB_MEX_FILE  */
}

static void mdlTerminate(SimStruct *S)
{
}

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif


