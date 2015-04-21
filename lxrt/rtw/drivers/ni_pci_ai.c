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

#define S_FUNCTION_NAME		ni_pci_ai
#define S_FUNCTION_LEVEL	2

#ifdef MATLAB_MEX_FILE
#include "mex.h"
#endif
#include "simstruc.h"

#define NUMBER_OF_PARAMS	(7)

#define BOARD_INDEX_PARAM	ssGetSFcnParam(S,0)
#define NUM_CHANNELS_PARAM	ssGetSFcnParam(S,1)
#define CHAN_TYPE_PARAM		ssGetSFcnParam(S,2)
#define GAIN_PARAM		ssGetSFcnParam(S,3)
#define POLARITY_PARAM		ssGetSFcnParam(S,4)
#define DITHER_PARAM		ssGetSFcnParam(S,5)
#define SAMPLE_TIME_PARAM	ssGetSFcnParam(S,6)

#define BOARD_INDEX		((uint_T) mxGetPr(BOARD_INDEX_PARAM)[0])
#define NUM_CHANNELS		((uint_T) mxGetPr(NUM_CHANNELS_PARAM)[0])
#define CHAN_TYPE		((uint_T) mxGetPr(CHAN_TYPE_PARAM)[0])
#define GAIN			((uint_T) mxGetPr(GAIN_PARAM)[0])
#define POLARITY		((uint_T) mxGetPr(POLARITY_PARAM)[0])
#define DITHER			((uint_T) mxGetPr(DITHER_PARAM)[0])
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
	int Board = (int)BOARD_INDEX - 1;
	int ChanType = (int)CHAN_TYPE;
	int Gain = (int)GAIN - 1;
	int UnipBip = (int)POLARITY - 1;
	int Dither = (int)DITHER;
	int LastChan = 0;
	unsigned short dev_id;
	char *bip = "BIPOLAR";
	char *unip = "UNIPOLAR";
	char *ditheron = "ON";
	char *ditheroff = "OFF";
	int n;
	FILE *logfile;

#ifndef MATLAB_MEX_FILE

	ni_get_board_device_id(Board, &dev_id);

        logfile = fopen("ni_pci_log", "a");

	fprintf(logfile, "\nAnalog Input section\n");
	fprintf(logfile, "====================\n");
	fprintf(logfile, "Board index : %d\n", Board);
	fprintf(logfile, "Board id    : 0x%x\n", dev_id);
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
	fprintf(logfile, "Gain               : %d\n", Gain);
	fprintf(logfile, "Polarity           : %s\n", UnipBip ? unip : bip);
	fprintf(logfile, "Dither enable      : %s\n", Dither ? ditheron : ditheroff);

	fclose(logfile);

	ni_AI_clear_configuration_FIFO(Board);
	for (n = 1; n < NUM_CHANNELS; n++) {
		if (n == NUM_CHANNELS - 1) {
			LastChan = 1;
		}
		ni_AI_configure(n, ChanType, Gain, UnipBip, Dither, LastChan, Board);
	}
	ni_AI_configure(0, ChanType, Gain, UnipBip, Dither, 0, Board);
	ni_AI_init(Board);
		
#endif
}

static void mdlOutputs(SimStruct *S, int_T tid)
{
	double Gain_List[8] = {0.5, 1., 2., 5., 10., 20., 50., 100.};
	int Board = (int)BOARD_INDEX - 1;
	int Gain = (int)GAIN - 1;
	int UnipBip = (int)POLARITY - 1;
	signed short int data[64];
	int n;
	double *y = ssGetOutputPortRealSignal(S, 0);

#ifndef MATLAB_MEX_FILE

	ni_AI_read(data, (int)NUM_CHANNELS, Board);
        for (n = 0; n < NUM_CHANNELS; n++) {
		y[n] = ni_i2v(data[n], Gain_List[Gain], UnipBip);
	}

#endif
}

static void mdlTerminate(SimStruct *S)
{
	int Board = (int)BOARD_INDEX - 1;

#ifndef MATLAB_MEX_FILE

	ni_AI_reset(Board);

#endif
}

#ifdef  MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
