/*
Copyright (C) 2002 Lorenzo Dozio (dozio@aero.polimi.it)

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define S_FUNCTION_NAME		ni_e_do
#define S_FUNCTION_LEVEL	2

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
#include "simstruc.h"

#define NUMBER_OF_ARGS			(6)

#define HAS_8255_ARG			ssGetSFcnParam(S,0)
#define PORT_8255_ARG			ssGetSFcnParam(S,1)
#define NUM_CHANNELS_8255_ARG		ssGetSFcnParam(S,2)
#define HAS_DAQSTC_ARG			ssGetSFcnParam(S,3)
#define DAQSTC_LINE_ARG			ssGetSFcnParam(S,4)
#define SAMPLE_TIME_ARG			ssGetSFcnParam(S,5)

#define HAS_8255			((uint_T) mxGetPr(HAS_8255_ARG)[0])
#define PORT_8255			((uint_T) mxGetPr(PORT_8255_ARG)[0])
#define NUM_CHANNELS_8255		((uint_T) mxGetPr(NUM_CHANNELS_8255_ARG)[0])
#define HAS_DAQSTC			((uint_T) mxGetPr(HAS_DAQSTC_ARG)[0])
#define DAQSTC_LINE			((uint_T) mxGetPr(DAQSTC_LINE_ARG)[0])
#define SAMPLE_TIME             	((real_T) mxGetPr(SAMPLE_TIME_ARG)[0])

#ifndef MATLAB_MEX_FILE
#include <linux/types.h>
#include "ni_e.h"
#include "ni_e_lxrt.h"
#endif

static void mdlInitializeSizes(SimStruct *S)
{
	uint_T i;

	ssSetNumSFcnParams(S, NUMBER_OF_ARGS);
	if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
		return;
	}
	for (i = 0; i < NUMBER_OF_ARGS; i++) {
		ssSetSFcnParamNotTunable(S, i);
	}
	ssSetNumInputPorts(S, 1);
	ssSetNumOutputPorts(S, 0);
	ssSetInputPortWidth(S, 0, DYNAMICALLY_SIZED);
	ssSetInputPortDirectFeedThrough(S, 0, 1);
	ssSetNumContStates(S, 0);
	ssSetNumDiscStates(S, 0);
	ssSetNumSampleTimes(S, 1);
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
	ssSetSampleTime(S, 0, SAMPLE_TIME);
	ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_START 
static void mdlStart(SimStruct *S)
{
	int port_8255 = (int)(PORT_8255);
	FILE *logfile;

#ifndef MATLAB_MEX_FILE

        logfile = fopen("ni_e_log", "a");

	fprintf(logfile, "\nDigital Output section\n");
	fprintf(logfile, "======================\n");

	if (HAS_8255) {
		fprintf(logfile, "82C55 digital I/O\n");
		switch (port_8255) {
			case 1:
				fprintf(logfile, "Port : C low\n");
				break;
			case 2:
				fprintf(logfile, "Port : B\n");
				break;
			case 3:
				fprintf(logfile, "Port : C up\n");
				break;
			case 4:
				fprintf(logfile, "Port : A\n");
				break;
		}
		fprintf(logfile, "Number of DO lines : %d\n", NUM_CHANNELS_8255);
		ni_e_8255_DIO_configure(port_8255, NI_E_8255_OUTPUT);
	}
	if (HAS_DAQSTC) {
		fprintf(logfile, "DAQ-STC digital I/O\n");
                fprintf(logfile, "Digital Output Line : %d\n", DAQSTC_LINE-1);
		ni_e_daqstc_DIO_configure((int)(DAQSTC_LINE - 1), NI_E_DAQSTC_DIO_OUTPUT);
	}

	fclose(logfile);

#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
	InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0);
	real_T u;
	int n = 0;
	unsigned char DO_byte_8255 = 0;
	int port_8255 = (int)(PORT_8255);
	int daqstc_line = (int)(DAQSTC_LINE - 1);

#ifndef MATLAB_MEX_FILE

	if (HAS_8255) {
		for (n = 0; n < NUM_CHANNELS_8255; n++) {
			u = *uPtrs[n];
			if (u > 0.) {
				DO_byte_8255 |= (1 << n);
			}
		}
		ni_e_8255_DO_write_byte(port_8255, DO_byte_8255);
	}
	if (HAS_DAQSTC) {
		u = *uPtrs[n];
		if (u > 0.) {
			ni_e_daqstc_DO_write_bit(daqstc_line, 1);
		} else {
			ni_e_daqstc_DO_write_bit(daqstc_line, 0);
		}
	}

#endif
}

static void mdlTerminate(SimStruct *S)
{
#ifndef MATLAB_MEX_FILE
	ni_e_daqstc_DO_reset();
#endif
}

#ifdef  MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
