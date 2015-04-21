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

#define S_FUNCTION_NAME  dapci1200
#define S_FUNCTION_LEVEL 2

#ifdef MATLAB_MEX_FILE
#include "mex.h"      /* needed for declaration of mexErrMsgTxt */
#endif

#include "simstruc.h"

#ifndef MATLAB_MEX_FILE
#include "asm/io.h"
#include "linux/delay.h"
#include "krt_pci.h"
#include "linux/pci_ids.h"
#endif

#define NUMBER_OF_ARGS         (2)
#define CHANNELS_ARG           ssGetSFcnParam(S,0)
#define SAMP_TIME_ARG          ssGetSFcnParam(S,1)

#define CHANNELS_IND           (0)
#define SAMP_TIME_IND          (0)

#define NO_I_WORKS             (1)
#define BASE_ADDR_I_IND        (0)

#ifndef MATLAB_MEX_FILE
#define SEL8800    0x0100
#define SEL7376    0x0200
#define SDI        0x8000

#define CALEN      0x4000

#define DAC0_BIP10V_COEFF   0xE0
#define DAC1_BIP10V_COEFF   0xF0

#define DAC0_FINE_GAIN     0
#define DAC0_COARSE_GAIN   1
#define DAC0_FINE_OFFSET   6
#define DAC0_COARSE_OFFSET 2
#define DAC1_FINE_GAIN     4
#define DAC1_COARSE_GAIN   5
#define DAC1_FINE_OFFSET   7
#define DAC1_COARSE_OFFSET 3

#define NVRAM_CTRL_REG    base_pci+0x3F
#define NVRAM_DATA_REG    base_pci+0x3E
#define CALIBRATE_REG     base_status+0x06

static unsigned char get_pci_dac1602_nVRam(unsigned int base_pci, unsigned int addr)
{

  unsigned char value;

  while (inb(NVRAM_CTRL_REG) & 0x80);   // D7=0 (not busy)
  
  outb(0x80, NVRAM_CTRL_REG);           // CMD to load low byte of address 
  outb(addr, NVRAM_DATA_REG);           // load low byte 

  outb(0xa0, NVRAM_CTRL_REG);           // CMD to load high byte of address 
  outb(0x00, NVRAM_DATA_REG);           // load high byte 

  outb(0xe0, NVRAM_CTRL_REG);           // CMD to read NVRAM data 
  
  while (inb(NVRAM_CTRL_REG) & 0x80);   // D7=0 (not busy) 
  value = inb(NVRAM_DATA_REG);
 
  return (value);
}

void write_pci_dac1602_8800( unsigned int base_status, unsigned char addr, unsigned char value )
{
  int i;

  /* write 3 bits MSB first of address, keep SEL8800 low */
  for ( i = 0; i < 3; i++) {
    if ( addr & 0x4 ) {
        outw_p(CALEN | SDI, CALIBRATE_REG );
    } else {
        outw_p(CALEN | 0x0, CALIBRATE_REG );
    }
    udelay(10);
    addr <<= 1;
  }

  /* write 8 bits MSB first of data, keep SEL8800 low */
  for ( i = 0; i < 8; i++) {
    if ( value & 0x80 ) {
        outw_p(CALEN | SDI, CALIBRATE_REG );
    } else {
        outw_p(CALEN | 0x0, CALIBRATE_REG );
    }
    udelay(10);
    value <<= 1;
  }

  /* SEL8800 to set load clock */
  outw_p( SEL8800, CALIBRATE_REG );
  udelay(10);

  /* SEL8800 to reset load clock and update output */
  outw_p( 0x0, CALIBRATE_REG );
}

static void cal_da(unsigned int base_pci, unsigned int base_status)
{
  unsigned char addr,value;

  addr=DAC0_BIP10V_COEFF;

  value = get_pci_dac1602_nVRam( base_pci, addr++ );  
  write_pci_dac1602_8800 ( base_status, DAC0_COARSE_GAIN, value );
  value = get_pci_dac1602_nVRam( base_pci, addr++ );
  write_pci_dac1602_8800 ( base_status, DAC0_COARSE_OFFSET, value );
  value = get_pci_dac1602_nVRam( base_pci, addr++ );
  write_pci_dac1602_8800 ( base_status, DAC0_FINE_GAIN, value );

  addr=DAC1_BIP10V_COEFF;

  value = get_pci_dac1602_nVRam( base_pci, addr++ );
  write_pci_dac1602_8800 ( base_status, DAC1_COARSE_GAIN, value );
  value = get_pci_dac1602_nVRam( base_pci, addr++ );
  write_pci_dac1602_8800 ( base_status, DAC1_COARSE_OFFSET, value );
  value = get_pci_dac1602_nVRam( base_pci, addr++ );
  write_pci_dac1602_8800 ( base_status, DAC1_FINE_GAIN, value );
  
}
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
unsigned int base_dac, base_status, base_pci;
unsigned int addr_mat[5];

  get_pci_addr(PCI_VENDOR_ID_CBOARDS,0x0F,addr_mat);

  base_pci    = addr_mat[0];
  base_status = addr_mat[1];
  base_dac    = addr_mat[4];

  outw(0x0502,base_status+0x08);

  cal_da(base_pci,base_status);

  ssSetIWorkValue(S,BASE_ADDR_I_IND,(int_T) base_dac);
#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
int_T             i;
InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0);
int_T             num_channels    = ssGetInputPortWidth(S,0);
uint_T            base_dac=ssGetIWorkValue(S,BASE_ADDR_I_IND);
int_T             res;
real_T            u;

#ifndef MATLAB_MEX_FILE
  for(i=0; i<num_channels; i++){
    u=*uPtrs[i];
    if(u>10.0) u=10.0;
    if(u<-10.0) u=-10.0;
    res=(int_T) ((u+10.0)/20.0*4096.0);
    outw(res,base_dac+2*i);
  }
#endif
}

static void mdlTerminate(SimStruct *S)
{
int_T        i=0;
int_T        num_channels=ssGetInputPortWidth(S,0);
uint_T       base_dac=ssGetIWorkValue(S,BASE_ADDR_I_IND);

#ifndef MATLAB_MEX_FILE
  for(i=0; i<num_channels; i++){
    outw(0x7ff,base_dac+2*i);
  }
#endif
}

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif
