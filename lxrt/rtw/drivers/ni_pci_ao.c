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

#define S_FUNCTION_NAME		ni_pci_ao
#define S_FUNCTION_LEVEL	2

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
#include "simstruc.h"

#define NUMBER_OF_PARAMS	(5)

#define BOARD_INDEX_PARAM	ssGetSFcnParam(S,0)
#define DACSEL_PARAM		ssGetSFcnParam(S,1)
#define DACREF_PARAM		ssGetSFcnParam(S,2)
#define DACPOL_PARAM		ssGetSFcnParam(S,3)
#define SAMPLE_TIME_PARAM	ssGetSFcnParam(S,4)

#define BOARD_INDEX		((uint_T) mxGetPr(BOARD_INDEX_PARAM)[0])
#define DACSEL			((uint_T) mxGetPr(DACSEL_PARAM)[0])
#define DACREF			((uint_T) mxGetPr(DACREF_PARAM)[0])
#define DACPOL			((uint_T) mxGetPr(DACPOL_PARAM)[0])
#define SAMPLE_TIME		((real_T) mxGetPr(SAMPLE_TIME_PARAM)[0])

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
	ssSetInputPortWidth(S, 0, 1);
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
	int DACSel = (int)DACSEL - 1;
	int DACRef = (int)DACREF - 1;
	int DACPol = (int)DACPOL - 1;
	unsigned short dev_id;
	char *bip = "BIPOLAR";
	char *unip = "UNIPOLAR";
	char *intref = "INTERNAL";
	char *extref = "EXTERNAL";
	FILE *logfile;

#ifndef MATLAB_MEX_FILE

	ni_get_board_device_id(Board, &dev_id);

        logfile = fopen("ni_pci_log", "a");

	fprintf(logfile, "\nAnalog Output section\n");
	fprintf(logfile, "=====================\n");
	fprintf(logfile, "Board index : %d\n", Board);
	fprintf(logfile, "Board id    : 0x%x\n", dev_id);
	fprintf(logfile, "DAC channel : %d\n", DACSel);
	fprintf(logfile, "Polarity    : %s\n", DACPol ? bip : unip);
	fprintf(logfile, "Reference   : %s\n", DACRef ? extref : intref);

	fclose(logfile);

	ni_AO_configure((int)DACSel, (int)DACRef, 1, (int)DACPol, Board);
	ni_AO_init(Board);
	if (dev_id == DEVICE_ID_PCI_6713) {
		ni_671x_AO_set_immediate_mode(Board);
	}

#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
	int Board = (int)BOARD_INDEX - 1;
	int DACSel = (int)DACSEL - 1;
	int DACPol = (int)DACPOL - 1;
	InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0);
	real_T u;

#ifndef MATLAB_MEX_FILE

	u = *uPtrs[0];
	ni_AO_write(ni_v2i((double)u, (int)DACPol), (int)DACSel, Board);

#endif
}

static void mdlTerminate(SimStruct *S)
{
	int Board = (int)BOARD_INDEX - 1;

#ifndef MATLAB_MEX_FILE

	ni_AO_reset(Board);

#endif
}

#ifdef  MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
