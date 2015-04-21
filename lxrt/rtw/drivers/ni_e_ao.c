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

#define S_FUNCTION_NAME		ni_e_ao
#define S_FUNCTION_LEVEL	2

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
#include "simstruc.h"

#define NUMBER_OF_ARGS		(5)

#define DACSEL_ARG		ssGetSFcnParam(S,0)
#define EXTREF_ARG		ssGetSFcnParam(S,1)
#define BIPDAC_ARG		ssGetSFcnParam(S,2)
#define TIMER_ARG		ssGetSFcnParam(S,3)
#define SAMP_TIME_ARG		ssGetSFcnParam(S,4)

#define DACSEL_IND		(0)
#define EXTREF_IND		(0)
#define BIPDAC_IND		(0)
#define SAMP_TIME_IND		(0)
#define TIMER                   ((uint_T) mxGetPr(TIMER_ARG)[0])

#define NO_I_WORKS		(2)
#define DACSEL_I_IND		(0)
#define BIPDAC_I_IND		(1)

#ifndef MATLAB_MEX_FILE
#include "ni_e.h"
#include "ni_e_lxrt.h"
#endif

static void mdlInitializeSizes(SimStruct *S)
{
	int_T i;
	ssSetNumSFcnParams(S, NUMBER_OF_ARGS);
	if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
		return;
	}
	for (i = 0; i < NUMBER_OF_ARGS; i++) {
		ssSetSFcnParamNotTunable(S, i);
	}
	ssSetNumInputPorts(S, 1);
	ssSetNumOutputPorts(S, 0);
	ssSetInputPortWidth(S, 0, 1);
	ssSetInputPortDirectFeedThrough(S, 0, 1);
	ssSetNumContStates(S, 0);
	ssSetNumDiscStates(S, 0);
	ssSetNumSampleTimes(S, 1);
	ssSetNumIWork(S, NO_I_WORKS);
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
	ssSetSampleTime(S, 0, mxGetPr(SAMP_TIME_ARG)[SAMP_TIME_IND]);
	ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_START 
static void mdlStart(SimStruct *S)
{
	int_T DACSel, ExtRef, BipDac;
	char *bip = "BIPOLAR";
	char *unip = "UNIPOLAR";
	char *inref = "INTERNAL";
	char *exref = "EXTERNAL";
	FILE *logfile;

#ifndef MATLAB_MEX_FILE

	DACSel = (int_T)mxGetPr(DACSEL_ARG)[DACSEL_IND] - 1;
	ExtRef = (int_T)mxGetPr(EXTREF_ARG)[EXTREF_IND] - 1;
	BipDac = (int_T)mxGetPr(BIPDAC_ARG)[BIPDAC_IND] - 1;
	ssSetIWorkValue(S, DACSEL_I_IND, (int_T) DACSel);
	ssSetIWorkValue(S, BIPDAC_I_IND, (int_T) BipDac);

        logfile = fopen("ni_e_log", "a");

	fprintf(logfile, "\nAnalog Output section\n");
	fprintf(logfile, "=====================\n");
	fprintf(logfile, "DAC channel : %d\n", DACSel);
	fprintf(logfile, "Polarity    : %s\n", BipDac ? bip : unip);
	fprintf(logfile, "Reference   : %s\n", ExtRef ? exref : inref);

	fclose(logfile);

	ni_e_AO_configure((int)DACSel, 0, (int)ExtRef, 0, (int)BipDac);
	if (TIMER) {
		ni_e_AOTM_init();
	}

#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
	int_T DACSel = (int_T) ssGetIWorkValue(S, DACSEL_I_IND);
	int_T BipDac = (int_T) ssGetIWorkValue(S, BIPDAC_I_IND);
	InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0);
	real_T u;

#ifndef MATLAB_MEX_FILE

	u = *uPtrs[0];
	ni_e_AO_write(ni_e_v2i((double)u, (int)BipDac), (int)DACSel);

#endif
}

static void mdlTerminate(SimStruct *S)
{
#ifndef MATLAB_MEX_FILE
	ni_e_AO_reset();
#endif
}

#ifdef  MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
