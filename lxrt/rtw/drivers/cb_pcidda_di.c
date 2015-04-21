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

#define S_FUNCTION_NAME		cb_pcidda_di
#define S_FUNCTION_LEVEL	2

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
#include "simstruc.h"

#define NUMBER_OF_PARAMS	(4)

#define PORT_NUMBER_PARAM	ssGetSFcnParam(S,0)
#define PORT_LETTER_PARAM	ssGetSFcnParam(S,1)
#define NCHAN_PARAM		ssGetSFcnParam(S,2)
#define SAMPLE_TIME_PARAM	ssGetSFcnParam(S,3)

#define PORT_NUMBER		((uint_T) mxGetPr(PORT_NUMBER_PARAM)[0])
#define PORT_LETTER		((uint_T) mxGetPr(PORT_LETTER_PARAM)[0])
#define NCHAN			((uint_T) mxGetPr(NCHAN_PARAM)[0])
#define SAMPLE_TIME             ((real_T) mxGetPr(SAMPLE_TIME_PARAM)[0])

#ifndef MATLAB_MEX_FILE
#include "cb_pcidda_addr.h"
#include "cb_pcidda.h"
#endif

#define MDL_CHECK_PARAMETERS
#if defined(MDL_CHECK_PARAMETERS) && defined(MATLAB_MEX_FILE)
static void mdlCheckParameters(SimStruct *S)
{
	static char_T errMsg[256];
	boolean_T allParamsOK = 1;

	if (mxGetNumberOfElements(NCHAN_PARAM) != 1) {
		sprintf(errMsg, "Number of channels parameters must be a scalar.\n");
		allParamsOK = 0;
		goto EXIT_POINT;
	}
	if (mxGetNumberOfElements(SAMPLE_TIME_PARAM) != 1) {
		sprintf(errMsg, "Sample Time must be a positive scalar.\n");
		allParamsOK = 0;
		goto EXIT_POINT;
	}
	if (PORT_LETTER == 1 || PORT_LETTER == 3) {
		if (!(NCHAN >= 1 && NCHAN <= 4)) {
			sprintf(errMsg, "Number of channels must be between 1 and 4.\n");
			allParamsOK = 0;
			goto EXIT_POINT;
		}
	}
	if (PORT_LETTER == 2 || PORT_LETTER == 4) {
		if (!(NCHAN >= 1 && NCHAN <= 8)) {
			sprintf(errMsg, "Number of channels must be between 1 and 8.\n");
			allParamsOK = 0;
			goto EXIT_POINT;
		}
	}
	if (SAMPLE_TIME <= 0) {
		sprintf(errMsg, "Sampling Time must be a positive value.\n");
		allParamsOK = 0;
		goto EXIT_POINT;
	}
		
EXIT_POINT:
	if (!allParamsOK) {
		ssSetErrorStatus(S, errMsg);
	}
}
#endif

static void mdlInitializeSizes(SimStruct *S)
{
	uint_T i;

	ssSetNumSFcnParams(S, NUMBER_OF_PARAMS);
	if (ssGetNumSFcnParams(S) == ssGetSFcnParamsCount(S)) {
		mdlCheckParameters(S);
		if (ssGetErrorStatus(S) != NULL) {
			return;
		}
	} else {
		return;
	}
	for (i = 0; i < NUMBER_OF_PARAMS; i++) {
		ssSetSFcnParamNotTunable(S, i);
	}
	ssSetNumInputPorts(S, 0);
	ssSetNumOutputPorts(S, 1);
	ssSetOutputPortWidth(S, 0, DYNAMICALLY_SIZED);
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
	int nchan = (int)NCHAN;
	int number = (int)PORT_NUMBER - 1;
	int port   = (int)PORT_LETTER;
	char letter[4] = {'C','B','C','A'};

#ifndef MATLAB_MEX_FILE
	printf("Port number : %d\n", number);
	printf("Port        : %c (INPUT)\n", letter[port-1]);
	printf("N channels  : %d\n", nchan);
	cb_pcidda_configure_dio(number, port, D_IN);
#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
	int nchan = (int)NCHAN;
	int number = (int)PORT_NUMBER - 1;
	int port   = (int)PORT_LETTER;
	int n;
	double *y = ssGetOutputPortRealSignal(S,0);

#ifndef MATLAB_MEX_FILE
	unsigned char DI_byte;
	
	DI_byte = cb_pcidda_read_di(number, port);
	for (n = 0; n < nchan; n++) {
		*y++ = (double)((DI_byte & (1 << n)) >> n);
	}
#endif
}

static void mdlTerminate(SimStruct *S)
{
#ifndef MATLAB_MEX_FILE
	cb_pcidda_reset_dio();
#endif
}

#ifdef  MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
