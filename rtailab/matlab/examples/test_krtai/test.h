/*
 * test.h
 *
 * Real-Time Workshop code generation for Simulink model "test.mdl".
 *
 * Model Version                        : 1.56
 * Real-Time Workshop file version      : 5.0 $Date: 2004/06/06 14:03:20 $
 * Real-Time Workshop file generated on : Fri Feb 28 16:22:51 2003
 * TLC version                          : 5.0 (Jun 18 2002)
 * C source code generated on           : Fri Feb 28 16:22:51 2003
 */

#ifndef _RTW_HEADER_test_h_
# define _RTW_HEADER_test_h_

#ifndef _test_COMMON_INCLUDES_
# define _test_COMMON_INCLUDES_
#include <math.h>
#include <string.h>

#include "simstruc.h"
#include "rt_logging.h"

#endif                                  /* _test_COMMON_INCLUDES_ */

#include "test_types.h"

#define MODEL_NAME                      test
#define NSAMPLE_TIMES                   (2)                      /* Number of sample times */
#define NINPUTS                         (0)                      /* Number of model inputs */
#define NOUTPUTS                        (0)                      /* Number of model outputs */
#define NBLOCKIO                        (5)                      /* Number of data output port signals */
#define NUM_ZC_EVENTS                   (0)                      /* Number of zero-crossing events */

#ifndef NCSTATES
# define NCSTATES (1)                   /* Number of continuous states */
#elif NCSTATES != 1
# error Invalid specification of NCSTATES defined in compiler command
#endif

/* Intrinsic types */
#ifndef POINTER_T
# define POINTER_T
typedef void * pointer_T;
#endif

/* Block signals (auto storage) */
typedef struct _BlockIO {
  real_T Passa_basso;                   /* '<Root>/Passa_basso' */
  real_T sawtooth;                      /* '<Root>/sawtooth' */
  real_T Pulse;                         /* '<Root>/Pulse' */
  real_T Discrete_Time_Integrator;      /* '<Root>/Discrete-Time Integrator' */
  real_T Pulse_Generator;               /* '<Root>/Pulse Generator' */
} BlockIO;

/* Block states (auto storage) for system: '<Root>' */
typedef struct D_Work_tag {
  real_T Discrete_Time_Integrator_DSTATE; /* <Root>/Discrete-Time Integrator */
  void *Scope_PWORK;                    /* '<Root>/Scope' */
  struct {
    int_T ClockTicksCounter;
  } Pulse_Generator_IWORK;              /* '<Root>/Pulse Generator' */
} D_Work;

/* Continuous states (auto storage) */
typedef struct _ContinuousStates {
  real_T Passa_basso_CSTATE;            /* '<Root>/Passa_basso' */
} ContinuousStates;

/* State derivatives (auto storage) */
typedef struct _StateDerivatives {
  real_T Passa_basso_CSTATE;            /* '<Root>/Passa_basso' */
} StateDerivatives;

/* State disabled  */
typedef struct _StateDisabled {
  boolean_T Passa_basso_CSTATE;         /* '<Root>/Passa_basso' */
} StateDisabled;

/* Parameters (auto storage) */
struct _Parameters {
  real_T Passa_basso_A;                 /* Computed Parameter: A
                                         * '<Root>/Passa_basso'
                                         */
  real_T Passa_basso_C;                 /* Computed Parameter: C
                                         * '<Root>/Passa_basso'
                                         */
  real_T sawtooth_Amplitude;            /* Expression: 1
                                         * '<Root>/sawtooth'
                                         */
  real_T sawtooth_Frequency;            /* Expression: 1
                                         * '<Root>/sawtooth'
                                         */
  real_T Pulse_Amplitude;               /* Expression: 1
                                         * '<Root>/Pulse'
                                         */
  real_T Pulse_Frequency;               /* Expression: 1.2
                                         * '<Root>/Pulse'
                                         */
  real_T Discrete_Time_Integrator_IC;   /* Expression: 0
                                         * '<Root>/Discrete-Time Integrator'
                                         */
  real_T Scope_P1_Size[2];              /* Computed Parameter: P1Size
                                         * '<Root>/Scope'
                                         */
  real_T Scope_P1;                      /* Expression: numch
                                         * '<Root>/Scope'
                                         */
  real_T Scope_P2_Size[2];              /* Computed Parameter: P2Size
                                         * '<Root>/Scope'
                                         */
  real_T Scope_P2;                      /* Expression: ts
                                         * '<Root>/Scope'
                                         */
  real_T Pulse_Generator_Amp;           /* Expression: 2
                                         * '<Root>/Pulse Generator'
                                         */
  real_T Pulse_Generator_Period;        /* Expression: 10000
                                         * '<Root>/Pulse Generator'
                                         */
  real_T Pulse_Generator_Duty;          /* Expression: 5000
                                         * '<Root>/Pulse Generator'
                                         */
};

/* Simulation Structure */
extern SimStruct *const rtS;

extern Parameters rtP;                  /* parameters */

/* External data declarations for dependent source files */

extern BlockIO rtB;                     /* block i/o */
extern ContinuousStates rtX;            /* continuous states */
extern D_Work rtDWork;                  /* data type work */

/* 
 * The generated code includes comments that allow you to trace directly 
 * back to the appropriate location in the model.  The basic format
 * is <system>/block_name, where system is the system number (uniquely
 * assigned by Simulink) and block_name is the name of the block.
 *
 * Use the MATLAB hilite_system command to trace the generated code back
 * to the model.  For example,
 *
 * hilite_system('<S3>')    - opens system 3
 * hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
 *
 * Here is the system hierarchy for this model
 *
 * '<Root>' : test
 */

#endif                                  /* _RTW_HEADER_test_h_ */
