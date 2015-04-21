/*
 * test_bio.c
 *
 * Real-Time Workshop code generation for Simulink model "test.mdl".
 *
 * Model Version                        : 1.56
 * Real-Time Workshop file version      : 5.0 $Date: 2004/06/06 14:03:20 $
 * Real-Time Workshop file generated on : Fri Feb 28 16:22:51 2003
 * TLC version                          : 5.0 (Jun 18 2002)
 * C source code generated on           : Fri Feb 28 16:22:51 2003
 */

#ifndef BLOCK_IO_SIGNALS
#define BLOCK_IO_SIGNALS

#include "bio_sig.h"

/* Block output signal information */
static const BlockIOSignals rtBIOSignals[] = {

  /* blockName,
   * signalName, portNumber, signalWidth, signalAddr, dtName, dtSize
   */

  {
    "test/Passa_basso",
    "NULL", 0, 1, &rtB.Passa_basso, "double", sizeof(real_T)
  },
  {
    "test/sawtooth",
    "NULL", 0, 1, &rtB.sawtooth, "double", sizeof(real_T)
  },
  {
    "test/Pulse",
    "NULL", 0, 1, &rtB.Pulse, "double", sizeof(real_T)
  },
  {
    "test/Discrete-Time Integrator",
    "NULL", 0, 1, &rtB.Discrete_Time_Integrator, "double", sizeof(real_T)
  },
  {
    "test/Pulse Generator",
    "NULL", 0, 1, &rtB.Pulse_Generator, "double", sizeof(real_T)
  },
  {
    NULL, NULL, 0, 0, 0, NULL, 0
  }
};

#endif                                  /* BLOCK_IO_SIGNALS */
