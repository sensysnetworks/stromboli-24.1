/*
 * test_pt.c
 *
 * Real-Time Workshop code generation for Simulink model "test.mdl".
 *
 * Model Version                        : 1.56
 * Real-Time Workshop file version      : 5.0 $Date: 2004/06/06 14:03:20 $
 * Real-Time Workshop file generated on : Fri Feb 28 16:22:51 2003
 * TLC version                          : 5.0 (Jun 18 2002)
 * C source code generated on           : Fri Feb 28 16:22:51 2003
 */

#ifndef _PT_INFO_test_
#define _PT_INFO_test_

#include "pt_info.h"

/* Tunable block parameters */

static const BlockTuning rtBlockTuning[] = {

  /* blockName, parameterName,
   * class, nRows, nCols, nDims, dimsOffset, source, dataType, numInstances,
   * mapOffset
   */

  /* SignalGenerator */
  {"test/sawtooth", "Amplitude",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 0}
  },
  /* SignalGenerator */
  {"test/sawtooth", "Frequency",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 1}
  },
  /* SignalGenerator */
  {"test/Pulse", "Amplitude",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 2}
  },
  /* SignalGenerator */
  {"test/Pulse", "Frequency",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 3}
  },
  /* DiscreteIntegrator */
  {"test/Discrete-Time Integrator", "InitialCondition",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 4}
  },
  /* S-Function */
  {"test/Scope", "P1",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 5}
  },
  /* S-Function */
  {"test/Scope", "P2",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 6}
  },
  /* DiscretePulseGenerator */
  {"test/Pulse Generator", "Amplitude",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 7}
  },
  /* DiscretePulseGenerator */
  {"test/Pulse Generator", "Period",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 8}
  },
  /* DiscretePulseGenerator */
  {"test/Pulse Generator", "PulseWidth",
    {rt_SCALAR, 1, 1, 2, -1, rt_SL_PARAM, SS_DOUBLE, 1, 9}
  },
  {NULL, NULL,
    {(ParamClass)0, 0, 0, 0, 0, (ParamSource)0, 0, 0, 0}
  }
};

/* Tunable variable parameters */

static const VariableTuning rtVariableTuning[] = {

  /* variableName,
   * class, nRows, nCols, nDims, dimsOffset, source, dataType, numInstances,
   * mapOffset
   */

  {NULL,
    {(ParamClass)0, 0, 0, 0, 0, (ParamSource)0, 0, 0, 0}
  }
};

static void * rtParametersMap[10];

void test_InitializeParametersMap(void) {
  rtParametersMap[0] = &rtP.sawtooth_Amplitude; /* 0 */
  rtParametersMap[1] = &rtP.sawtooth_Frequency; /* 1 */
  rtParametersMap[2] = &rtP.Pulse_Amplitude; /* 2 */
  rtParametersMap[3] = &rtP.Pulse_Frequency; /* 3 */
  rtParametersMap[4] = &rtP.Discrete_Time_Integrator_IC; /* 4 */
  rtParametersMap[5] = &rtP.Scope_P1;   /* 5 */
  rtParametersMap[6] = &rtP.Scope_P2;   /* 6 */
  rtParametersMap[7] = &rtP.Pulse_Generator_Amp; /* 7 */
  rtParametersMap[8] = &rtP.Pulse_Generator_Period; /* 8 */
  rtParametersMap[9] = &rtP.Pulse_Generator_Duty; /* 9 */
}

static uint_T const rtDimensionsMap[] = {
  0                                     /* Dummy */
};

#endif                                  /* _PT_INFO_test_ */
