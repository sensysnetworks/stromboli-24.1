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

#define S_FUNCTION_NAME		ni_e_ai
#define S_FUNCTION_LEVEL	2

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
#include "simstruc.h"

#define NUMBER_OF_PARAMS	(7)

#define NUM_CHANNELS_PARAM	ssGetSFcnParam(S,0)
#define CHAN_TYPE_PARAM		ssGetSFcnParam(S,1)
#define GAIN_PARAM		ssGetSFcnParam(S,2)
#define POLARITY_PARAM		ssGetSFcnParam(S,3)
#define DITHER_EN_PARAM		ssGetSFcnParam(S,4)
#define TIMER_PARAM		ssGetSFcnParam(S,5)
#define SAMPLE_TIME_PARAM	ssGetSFcnParam(S,6)

#define NUM_CHANNELS		((uint_T) mxGetPr(NUM_CHANNELS_PARAM)[0])
#define CHAN_TYPE		((uint_T) mxGetPr(CHAN_TYPE_PARAM)[0])
#define GAIN			((real_T) mxGetPr(GAIN_PARAM)[0])
#define POLARITY		((uint_T) mxGetPr(POLARITY_PARAM)[0])
#define DITHER_EN		((uint_T) mxGetPr(DITHER_EN_PARAM)[0])
#define TIMER			((uint_T) mxGetPr(TIMER_PARAM)[0])
#define SAMPLE_TIME		((real_T) mxGetPr(SAMPLE_TIME_PARAM)[0])

#ifndef MATLAB_MEX_FILE
#include <linux/types.h>
#include "ni_e.h"
#include "ni_e_lxrt.h"
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
	int n;
	int LastChan = 0;
	int ChanType = (int)CHAN_TYPE;
	int UnipBip = (int)POLARITY - 1;
	int DitherEn = (int)DITHER_EN;
	char *bip = "BIPOLAR";
	char *unip = "UNIPOLAR";
	char *ditheron = "ON";
	char *ditheroff = "OFF";
	char *pctimer = "PC (internal)";
	char *boardtimer = "BOARD (external)";
	FILE *logfile;

#ifndef MATLAB_MEX_FILE

        logfile = fopen("ni_e_log", "a");

	fprintf(logfile, "\nAnalog Input section\n");
	fprintf(logfile, "====================\n");
	fprintf(logfile, "Number of channels : %d [form 0 to %d]\n", NUM_CHANNELS, NUM_CHANNELS-1);
	switch (ChanType) {
		case 1:
			fprintf(logfile, "Channels type      : DIFFERENTIAL\n");
			break;
		case 2:
			fprintf(logfile, "Channels type      : NRSE\n");
			break;
		case 3:
			fprintf(logfile, "Channels type      : RSE\n");
			break;
	}
	fprintf(logfile, "Gain               : %f\n", GAIN);
	fprintf(logfile, "Polarity           : %s\n", UnipBip ? unip : bip);
	fprintf(logfile, "Dither enable      : %s\n", DitherEn ? ditheron : ditheroff);
	fprintf(logfile, "Timer              : %s\n", TIMER ? pctimer : boardtimer);

	fclose(logfile);

	ni_e_clear_Configuration_FIFO();
	for (n = 1; n < NUM_CHANNELS; n++) {
		if (n == NUM_CHANNELS - 1) {
			LastChan = 1;
		}
		ni_e_AI_configure(n, ChanType, (double)GAIN, UnipBip, DitherEn, LastChan);
	}
	ni_e_AI_configure(0, ChanType, (double)GAIN, UnipBip, DitherEn, 0);
	if (TIMER) {
		ni_e_AI_init();
	}
		
#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
	double *y = ssGetOutputPortRealSignal(S, 0);
	int UnipBip = (int)POLARITY - 1;

#ifndef MATLAB_MEX_FILE

	__s16 adcval[64];
	int n;

	if (TIMER) {
		ni_e_AI_read(adcval, (int)NUM_CHANNELS, AI_IRQ_on_nothing);
	} else {
		ni_e_AI_read(adcval, (int)NUM_CHANNELS, AI_IRQ_on_STOP);
	}
        for (n = 0; n < NUM_CHANNELS; n++) {
		y[n] = ni_e_i2v(adcval[n], (double)GAIN, UnipBip);
//		*y++ = ni_e_i2v(adcval[n], (double)GAIN, UnipBip);
        }

#endif
}

static void mdlTerminate(SimStruct *S)
{
#ifndef MATLAB_MEX_FILE
	ni_e_AI_reset();
#endif
}

#ifdef  MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
