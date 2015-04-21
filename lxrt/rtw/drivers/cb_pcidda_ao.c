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

#define S_FUNCTION_NAME		cb_pcidda_ao
#define S_FUNCTION_LEVEL	2

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
#include "simstruc.h"

#define EEPROM_ENTRIES	103

#define NUMBER_OF_PARAMS	(4)

#define DAC_CHAN_PARAM		ssGetSFcnParam(S,0)
#define POLARITY_PARAM		ssGetSFcnParam(S,1)
#define FULLSCALE_PARAM		ssGetSFcnParam(S,2)
#define SAMPLE_TIME_PARAM	ssGetSFcnParam(S,3)

#define DAC_CHAN		((uint_T) mxGetPr(DAC_CHAN_PARAM)[0])
#define POLARITY		((uint_T) mxGetPr(POLARITY_PARAM)[0])
#define FULLSCALE		((uint_T) mxGetPr(FULLSCALE_PARAM)[0])
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

	if (mxGetNumberOfElements(SAMPLE_TIME_PARAM) != 1) {
		sprintf(errMsg, "Sample Time must be a positive scalar.\n");
		allParamsOK = 0;
		goto EXIT_POINT;
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
	int DAC_channel = (int)DAC_CHAN - 1;
	int DAC_polarity = (int)POLARITY - 1;
	int DAC_vfs = 1;
	int index;
	unsigned short int eeprom_data[EEPROM_ENTRIES];

#ifndef MATLAB_MEX_FILE
	switch ((int)FULLSCALE) {
		case 1:
			DAC_vfs = FS_10V;
			break;
		case 2:
			DAC_vfs = FS_5V;
			break;
		case 3:
			DAC_vfs = FS_2V5;
			break;
		default:
			break;
	}
	printf("DAC channel : %d\n", DAC_channel);
	printf("Polarity    : %d %s\n", DAC_polarity, DAC_polarity ? "Unipolar" : "Bipolar");
	printf("Full scale  : %d %d\n", FULLSCALE, DAC_vfs);
	for (index = 0; index < EEPROM_ENTRIES; index++) {
		eeprom_data[index] = cb_pcidda_read_eeprom(index);
        }
	cb_pcidda_calibrate_dac(DAC_channel, DAC_polarity, DAC_vfs, eeprom_data);
	cb_pcidda_configure_dac(DAC_channel, DAC_polarity, DAC_vfs, 0);
	cb_pcidda_reset_dac(DAC_channel, DAC_polarity, DAC_vfs);
#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
	int DAC_channel = (int)DAC_CHAN - 1;
	int DAC_polarity = (int)POLARITY - 1;
	int DAC_vfs = 1;
	InputRealPtrsType uPtrs = ssGetInputPortRealSignalPtrs(S,0);
	real_T u;

#ifndef MATLAB_MEX_FILE
	u = *uPtrs[0];
	switch ((int)FULLSCALE) {
		case 1:
			DAC_vfs = FS_10V;
			break;
		case 2:
			DAC_vfs = FS_5V;
			break;
		case 3:
			DAC_vfs = FS_2V5;
			break;
		default:
			break;
	}
	cb_pcidda_write_dac(DAC_channel, cb_pcidda_v2i(u, DAC_polarity, DAC_vfs));
#endif
}

static void mdlTerminate(SimStruct *S)
{
	int DAC_channel = (int)DAC_CHAN - 1;
	int DAC_polarity = (int)POLARITY - 1;
	int DAC_vfs = 1;

#ifndef MATLAB_MEX_FILE
	switch ((int)FULLSCALE) {
		case 1:
			DAC_vfs = FS_10V;
			break;
		case 2:
			DAC_vfs = FS_5V;
			break;
		case 3:
			DAC_vfs = FS_2V5;
			break;
		default:
			break;
	}
	cb_pcidda_reset_dac(DAC_channel, DAC_polarity, DAC_vfs);
#endif
}

#ifdef  MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
