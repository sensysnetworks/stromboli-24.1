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

#define S_FUNCTION_NAME		ni_pci_do
#define S_FUNCTION_LEVEL	2

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
#include "simstruc.h"

#define NUMBER_OF_PARAMS		(7)

#define BOARD_INDEX_PARAM		ssGetSFcnParam(S,0)
#define DIO_82C55_PARAM			ssGetSFcnParam(S,1)
#define PORT_82C55_PARAM		ssGetSFcnParam(S,2)
#define NUM_CHANS_82C55_PARAM		ssGetSFcnParam(S,3)
#define DIO_MODULE_PARAM		ssGetSFcnParam(S,4)
#define DIO_MODULE_LINE_PARAM		ssGetSFcnParam(S,5)
#define SAMPLE_TIME_PARAM		ssGetSFcnParam(S,6)

#define BOARD_INDEX			((uint_T) mxGetPr(BOARD_INDEX_PARAM)[0])
#define DIO_82C55			((uint_T) mxGetPr(DIO_82C55_PARAM)[0])
#define PORT_82C55			((uint_T) mxGetPr(PORT_82C55_PARAM)[0])
#define NUM_CHANS_82C55			((uint_T) mxGetPr(NUM_CHANS_82C55_PARAM)[0])
#define DIO_MODULE			((uint_T) mxGetPr(DIO_MODULE_PARAM)[0])
#define DIO_MODULE_LINE			((uint_T) mxGetPr(DIO_MODULE_LINE_PARAM)[0])
#define SAMPLE_TIME			((real_T) mxGetPr(SAMPLE_TIME_PARAM)[0])

#ifndef MATLAB_MEX_FILE
#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include "ni_pci_lxrt.h"
#include "ni_pci.h"
#endif

static void mdlInitializeSizes(SimStruct *S)
{
	uint_T i;

	ssSetNumSFcnParams(S, NUMBER_OF_PARAMS);
	if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
		return;
	}
	for (i = 0; i < NUMBER_OF_PARAMS; i++) {
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
	int Board = (int)BOARD_INDEX - 1;
	int port_82C55 = (int)(PORT_82C55);
	unsigned short dev_id;
	FILE *logfile;

#ifndef MATLAB_MEX_FILE

	ni_get_board_device_id(Board, &dev_id);

	logfile = fopen("ni_pci_log", "a");

	fprintf(logfile, "\nDigital Output section\n");
	fprintf(logfile, "======================\n");
	fprintf(logfile, "Board index : %d\n", Board);
	fprintf(logfile, "Board id    : 0x%x\n", dev_id);

	if (DIO_82C55) {
		fprintf(logfile, "82C55 digital I/O\n");
		switch (port_82C55) {
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
		fprintf(logfile, "Number of DO lines activated : %d\n", NUM_CHANS_82C55);
		ni_82C55_configure(port_82C55, NI_82C55_OUTPUT, Board);
	}
	if (DIO_MODULE) {
		fprintf(logfile, "Digital I/O module\n");
                fprintf(logfile, "DO line activated : %d\n", DIO_MODULE_LINE - 1);
		ni_DIO_module_configure((int)(DIO_MODULE_LINE - 1), NI_DIO_MODULE_OUTPUT, Board);
	}

	fclose(logfile);

#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
	int Board = (int)BOARD_INDEX - 1;
	int port_82C55 = (int)(PORT_82C55);
	int DIO_module_line = (int)(DIO_MODULE_LINE - 1);
	InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0);
	real_T u;
	int n = 0;
	unsigned char DO_byte_82C55 = 0;

#ifndef MATLAB_MEX_FILE

	if (DIO_82C55) {
		for (n = 0; n < NUM_CHANS_82C55; n++) {
			u = *uPtrs[n];
			if (u > 0.) {
				DO_byte_82C55 |= (1 << n);
			}
		}
		ni_82C55_write_byte(port_82C55, DO_byte_82C55, Board);
	}
	if (DIO_MODULE) {
		u = *uPtrs[n];
		if (u > 0.) {
			ni_DIO_module_write_bit(DIO_module_line, 1, Board);
		} else {
			ni_DIO_module_write_bit(DIO_module_line, 0, Board);
		}
	}

#endif
}

static void mdlTerminate(SimStruct *S)
{
	int Board = (int)BOARD_INDEX - 1;

#ifndef MATLAB_MEX_FILE

	if (DIO_MODULE) {
		ni_DIO_module_reset(Board);
	}
	
#endif
}

#ifdef  MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
