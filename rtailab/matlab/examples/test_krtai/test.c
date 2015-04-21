/*
 * Real-Time Workshop code generation for Simulink model "test.mdl".
 *
 * Model Version                        : 1.56
 * Real-Time Workshop file version      : 5.0 $Date: 2004/06/06 14:03:20 $
 * Real-Time Workshop file generated on : Fri Feb 28 16:22:51 2003
 * TLC version                          : 5.0 (Jun 18 2002)
 * C source code generated on           : Fri Feb 28 16:22:51 2003
 */

#include <math.h>
#include <string.h>
#include "test.h"
#include "test_private.h"

#include "mdl_info.h"

#include "test_bio.c"

#include "test_pt.c"
#include "simstruc.h"

/* Block signals (auto storage) */
BlockIO rtB;

/* Continuous states */
ContinuousStates rtX;

/* Block states (auto storage) */
D_Work rtDWork;

/* Parent Simstruct */
static SimStruct model_S;
SimStruct *const rtS = &model_S;

/* Initial conditions for root system: '<Root>' */
void MdlInitialize(void)
{

  /* TransferFcn Block: <Root>/Passa_basso */
  rtX.Passa_basso_CSTATE = 0.0;

  /* DiscreteIntegrator Block: <Root>/Discrete-Time Integrator */
  rtDWork.Discrete_Time_Integrator_DSTATE = rtP.Discrete_Time_Integrator_IC;
}

/* Start for root system: '<Root>' */
void MdlStart(void)
{

  /* user code (Start function Header) */
  test_InitializeParametersMap();

  /* DiscreteIntegrator Block: <Root>/Discrete-Time Integrator */
  rtB.Discrete_Time_Integrator = rtP.Discrete_Time_Integrator_IC;

  /* Level2 S-Function Block: <Root>/Scope (sfun_rtai_scope) */
  {
    SimStruct *rts = ssGetSFunction(rtS, 0);
    sfcnStart(rts);
  }

  /* DiscretePulseGenerator Block: <Root>/Pulse Generator */
  {
    int_T Ns;
    real_T tFirst = ssGetTStart(rtS);
    Ns = (int_T)floor(tFirst / 0.001 + 0.5);
    if (Ns <= 0) {
      rtDWork.Pulse_Generator_IWORK.ClockTicksCounter = Ns;
    } else {
      rtDWork.Pulse_Generator_IWORK.ClockTicksCounter = Ns -
        (int_T)(rtP.Pulse_Generator_Period*floor((real_T)Ns /
        rtP.Pulse_Generator_Period));
    }
  }

  MdlInitialize();
}

/* Outputs for root system: '<Root>' */
void MdlOutputs(int_T tid)
{

  if (ssIsContinuousTask(rtS, tid)) {   /* Sample time: [0.0, 0.0] */

    /* TransferFcn Block: <Root>/Passa_basso */
    rtB.Passa_basso = rtP.Passa_basso_C*rtX.Passa_basso_CSTATE;

    /* SignalGenerator: '<Root>/sawtooth' */
    {
      real_T phase = rtP.sawtooth_Frequency*ssGetT(rtS);
      phase = 1.0-2.0*(phase-floor(phase));
      rtB.sawtooth = rtP.sawtooth_Amplitude*phase;
    }

    /* SignalGenerator: '<Root>/Pulse' */
    {
      real_T phase = rtP.Pulse_Frequency*ssGetT(rtS);
      phase = phase-floor(phase);
      rtB.Pulse = ( phase >= 0.5 ) ?
        rtP.Pulse_Amplitude : -rtP.Pulse_Amplitude;
    }
  }

  if (ssIsSampleHit(rtS, 1, tid)) {     /* Sample time: [0.001, 0.0] */

    /* DiscreteIntegrator: '<Root>/Discrete-Time Integrator'
     *
     * Regarding '<Root>/Discrete-Time Integrator':
     *    Unlimited, w/o Saturation Port
     */
    rtB.Discrete_Time_Integrator = rtDWork.Discrete_Time_Integrator_DSTATE;

    /* Level2 S-Function Block: <Root>/Scope (sfun_rtai_scope) */
    {
      SimStruct *rts = ssGetSFunction(rtS, 0);
      sfcnOutputs(rts, tid);
    }

    /* DiscretePulseGenerator: '<Root>/Pulse Generator' */
    rtB.Pulse_Generator =
      (rtDWork.Pulse_Generator_IWORK.ClockTicksCounter <
      rtP.Pulse_Generator_Duty &&
      rtDWork.Pulse_Generator_IWORK.ClockTicksCounter >= 0) ?
     rtP.Pulse_Generator_Amp :
      0.0;
    if (rtDWork.Pulse_Generator_IWORK.ClockTicksCounter >=
     rtP.Pulse_Generator_Period-1) {
      rtDWork.Pulse_Generator_IWORK.ClockTicksCounter = 0;
    } else {
      (rtDWork.Pulse_Generator_IWORK.ClockTicksCounter)++;
    }
  }
}

/* Update for root system: '<Root>' */
void MdlUpdate(int_T tid)
{

  if (ssIsSampleHit(rtS, 1, tid)) {     /* Sample time: [0.001, 0.0] */

    /* DiscreteIntegrator Block: <Root>/Discrete-Time Integrator */
    rtDWork.Discrete_Time_Integrator_DSTATE =
      rtDWork.Discrete_Time_Integrator_DSTATE + 0.001 * rtB.Pulse;
  }
}

/* Derivatives for root system: '<Root>' */
void MdlDerivatives(void)
{
  /* simstruct variables */
  StateDerivatives *rtXdot = (StateDerivatives*) ssGetdX(rtS);

  /* TransferFcn Block: <Root>/Passa_basso */
  {

    rtXdot->Passa_basso_CSTATE = rtB.Pulse_Generator;
    rtXdot->Passa_basso_CSTATE += (rtP.Passa_basso_A)*rtX.Passa_basso_CSTATE;
  }
}

/* Projection for root system: '<Root>' */
void MdlProjection(void)
{
}

/* Terminate for root system: '<Root>' */
void MdlTerminate(void)
{
  if(rtS != NULL) {

    /* Level2 S-Function Block: <Root>/Scope (sfun_rtai_scope) */
    {
      SimStruct *rts = ssGetSFunction(rtS, 0);
      sfcnTerminate(rts);
    }
  }
}

/* Helper function to make function calls from non-inlined S-functions */
int_T rt_CallSys(SimStruct *S, int_T element, int_T tid)
{
  (*(S)->callSys.fcns[element])((S)->callSys.args1[element],
   (S)->callSys.args2[element], tid);

  if (ssGetErrorStatus(S) != NULL) {
    return 0;
  } else {
    return 1;
  }
}

/* Function to initialize sizes */
void MdlInitializeSizes(void)
{
  ssSetNumContStates(rtS, 1);           /* Number of continuous states */
  ssSetNumY(rtS, 0);                    /* Number of model outputs */
  ssSetNumU(rtS, 0);                    /* Number of model inputs */
  ssSetDirectFeedThrough(rtS, 0);       /* The model is not direct feedthrough */
  ssSetNumSampleTimes(rtS, 2);          /* Number of sample times */
  ssSetNumBlocks(rtS, 6);               /* Number of blocks */
  ssSetNumBlockIO(rtS, 5);              /* Number of block outputs */
  ssSetNumBlockParams(rtS, 16);         /* Sum of parameter "widths" */
}

/* Function to initialize sample times */
void MdlInitializeSampleTimes(void)
{
  /* task periods */
  ssSetSampleTime(rtS, 0, 0.0);
  ssSetSampleTime(rtS, 1, 0.001);

  /* task offsets */
  ssSetOffsetTime(rtS, 0, 0.0);
  ssSetOffsetTime(rtS, 1, 0.0);
}

/* Function to register the model */
SimStruct *test(void)
{
  static struct _ssMdlInfo mdlInfo;
  (void)memset((char *)rtS, 0, sizeof(SimStruct));
  (void)memset((char *)&mdlInfo, 0, sizeof(struct _ssMdlInfo));
  ssSetMdlInfoPtr(rtS, &mdlInfo);

  /* timing info */
  {
    static time_T mdlPeriod[NSAMPLE_TIMES];
    static time_T mdlOffset[NSAMPLE_TIMES];
    static time_T mdlTaskTimes[NSAMPLE_TIMES];
    static int_T mdlTsMap[NSAMPLE_TIMES];
    static int_T mdlSampleHits[NSAMPLE_TIMES];

    {
      int_T i;

      for(i = 0; i < NSAMPLE_TIMES; i++) {
        mdlPeriod[i] = 0.0;
        mdlOffset[i] = 0.0;
        mdlTaskTimes[i] = 0.0;
      }
    }
    (void)memset((char_T *)&mdlTsMap[0], 0, 2 * sizeof(int_T));
    (void)memset((char_T *)&mdlSampleHits[0], 0, 2 * sizeof(int_T));

    ssSetSampleTimePtr(rtS, &mdlPeriod[0]);
    ssSetOffsetTimePtr(rtS, &mdlOffset[0]);
    ssSetSampleTimeTaskIDPtr(rtS, &mdlTsMap[0]);
    ssSetTPtr(rtS, &mdlTaskTimes[0]);
    ssSetSampleHitPtr(rtS, &mdlSampleHits[0]);
  }
  ssSetSolverMode(rtS, SOLVER_MODE_SINGLETASKING);

  /*
   * initialize model vectors and cache them in SimStruct
   */

  /* block I/O */
  {
    void *b = (void *) &rtB;
    ssSetBlockIO(rtS, b);

    {
      int_T i;

      b =&rtB.Passa_basso;
      for (i = 0; i < 5; i++) {
        ((real_T*)b)[i] = 0.0;
      }
    }
  }

  /* parameters */
  ssSetDefaultParam(rtS, (real_T *) &rtP);

  /* states */
  {
    int_T i;
    real_T *x = (real_T *) &rtX;
    ssSetContStates(rtS, x);
    for(i = 0; i < (int_T)(sizeof(ContinuousStates)/sizeof(real_T)); i++) {
      x[i] = 0.0;
    }
  }

  /* data type work */
  {
    void *dwork = (void *) &rtDWork;
    ssSetRootDWork(rtS, dwork);
    (void)memset((char_T *) dwork, 0, sizeof(D_Work));
    rtDWork.Discrete_Time_Integrator_DSTATE = 0.0;
  }

  /* C API for Parameter Tuning and/or Signal Monitoring */
  {
    static ModelMappingInfo mapInfo;

    memset((char_T *) &mapInfo, 0, sizeof(mapInfo));

    /* block signal monitoring map */
    mapInfo.Signals.blockIOSignals = &rtBIOSignals[0];
    mapInfo.Signals.numBlockIOSignals = 5;

    /* parameter tuning maps */
    mapInfo.Parameters.blockTuning = &rtBlockTuning[0];
    mapInfo.Parameters.variableTuning = &rtVariableTuning[0];
    mapInfo.Parameters.parametersMap = rtParametersMap;
    mapInfo.Parameters.dimensionsMap = rtDimensionsMap;
    mapInfo.Parameters.numBlockTuning = 10;
    mapInfo.Parameters.numVariableTuning = 0;

    ssSetModelMappingInfo(rtS, &mapInfo);
  }

  /* Model specific registration */
  ssSetRootSS(rtS, rtS);

  ssSetVersion(rtS, SIMSTRUCT_VERSION_LEVEL2);
  ssSetModelName(rtS, "test");
  ssSetPath(rtS, "test");

  ssSetTStart(rtS, 0.0);
  ssSetTFinal(rtS, -1);
  ssSetStepSize(rtS, 0.001);
  ssSetFixedStepSize(rtS, 0.001);
  /* Setup for data logging */
  {
    static RTWLogInfo rt_DataLoggingInfo;

    ssSetRTWLogInfo(rtS, &rt_DataLoggingInfo);

    rtliSetLogFormat(ssGetRTWLogInfo(rtS), 0);

    rtliSetLogMaxRows(ssGetRTWLogInfo(rtS), 0);

    rtliSetLogDecimation(ssGetRTWLogInfo(rtS), 1);

    rtliSetLogVarNameModifier(ssGetRTWLogInfo(rtS), "rt_");

    rtliSetLogT(ssGetRTWLogInfo(rtS), "tout");

    rtliSetLogX(ssGetRTWLogInfo(rtS), "");

    rtliSetLogXFinal(ssGetRTWLogInfo(rtS), "");

    rtliSetLogXSignalInfo(ssGetRTWLogInfo(rtS), NULL);

    rtliSetLogXSignalPtrs(ssGetRTWLogInfo(rtS), NULL);

    rtliSetLogY(ssGetRTWLogInfo(rtS), "");

    rtliSetLogYSignalInfo(ssGetRTWLogInfo(rtS), NULL);

    rtliSetLogYSignalPtrs(ssGetRTWLogInfo(rtS), NULL);
  }

  ssSetChecksumVal(rtS, 0, 2423075484U);
  ssSetChecksumVal(rtS, 1, 3056372984U);
  ssSetChecksumVal(rtS, 2, 1366991265U);
  ssSetChecksumVal(rtS, 3, 2544335103U);

  /* child S-Function registration */

  ssSetNumSFunctions(rtS, 1);

  /* register each child */
  {
    static SimStruct childSFunctions[1];
    static SimStruct *childSFunctionPtrs[1];

    (void)memset((char_T *)&childSFunctions[0], 0, sizeof(childSFunctions));
    ssSetSFunctions(rtS, &childSFunctionPtrs[0]);
    {
      int_T i;

      for(i = 0; i < 1; i++) {
        ssSetSFunction(rtS, i, &childSFunctions[i]);
      }
    }

    /* Level2 S-Function Block: test/<Root>/Scope (sfun_rtai_scope) */
    {
      SimStruct *rts = ssGetSFunction(rtS, 0);
      /* timing info */
      static time_T sfcnPeriod[1];
      static time_T sfcnOffset[1];
      static int_T sfcnTsMap[1];

      {
        int_T i;

        for(i = 0; i < 1; i++) {
          sfcnPeriod[i] = sfcnOffset[i] = 0.0;
        }
      }
      ssSetSampleTimePtr(rts, &sfcnPeriod[0]);
      ssSetOffsetTimePtr(rts, &sfcnOffset[0]);
      ssSetSampleTimeTaskIDPtr(rts, sfcnTsMap);

      /* Set up the mdlInfo pointer */
      ssSetMdlInfoPtr(rts, ssGetMdlInfoPtr(rtS));
      /* Allocate memory of model methods 2 */
      {
        static struct _ssSFcnModelMethods2 methods2;
        ssSetModelMethods2(rts, &methods2);
      }

      /* inputs */
      {
        static struct _ssPortInputs inputPortInfo[4];

        _ssSetNumInputPorts(rts, 4);
        ssSetPortInfoForInputs(rts, &inputPortInfo[0]);

        /* port 0 */
        {

          static real_T const *sfcnUPtrs[1];
          sfcnUPtrs[0] = &rtB.Passa_basso;
          ssSetInputPortSignalPtrs(rts, 0, (InputPtrsType)&sfcnUPtrs[0]);
          _ssSetInputPortNumDimensions(rts, 0, 1);
          ssSetInputPortWidth(rts, 0, 1);
        }

        /* port 1 */
        {

          static real_T const *sfcnUPtrs[1];
          sfcnUPtrs[0] = &rtB.sawtooth;
          ssSetInputPortSignalPtrs(rts, 1, (InputPtrsType)&sfcnUPtrs[0]);
          _ssSetInputPortNumDimensions(rts, 1, 1);
          ssSetInputPortWidth(rts, 1, 1);
        }

        /* port 2 */
        {

          static real_T const *sfcnUPtrs[1];
          sfcnUPtrs[0] = &rtB.Pulse;
          ssSetInputPortSignalPtrs(rts, 2, (InputPtrsType)&sfcnUPtrs[0]);
          _ssSetInputPortNumDimensions(rts, 2, 1);
          ssSetInputPortWidth(rts, 2, 1);
        }

        /* port 3 */
        {

          static real_T const *sfcnUPtrs[1];
          sfcnUPtrs[0] = &rtB.Discrete_Time_Integrator;
          ssSetInputPortSignalPtrs(rts, 3, (InputPtrsType)&sfcnUPtrs[0]);
          _ssSetInputPortNumDimensions(rts, 3, 1);
          ssSetInputPortWidth(rts, 3, 1);
        }
      }

      /* path info */
      ssSetModelName(rts, "Scope");
      ssSetPath(rts, "test/Scope");
      if (ssGetRTModel(rtS) == NULL) {
        ssSetParentSS(rts, rtS);
        ssSetRootSS(rts, ssGetRootSS(rtS));
      } else {
        ssSetRTModel(rts,ssGetRTModel(rtS));
        ssSetParentSS(rts, NULL);
        ssSetRootSS(rts, rts);
      }
      ssSetVersion(rts, SIMSTRUCT_VERSION_LEVEL2);

      /* parameters */
      {
        static mxArray *sfcnParams[2];

        ssSetSFcnParamsCount(rts, 2);
        ssSetSFcnParamsPtr(rts, &sfcnParams[0]);

        ssSetSFcnParam(rts, 0, &rtP.Scope_P1_Size[0]);
        ssSetSFcnParam(rts, 1, &rtP.Scope_P2_Size[0]);
      }

      /* work vectors */
      ssSetPWork(rts, (void **) &rtDWork.Scope_PWORK);
      {
        static struct _ssDWorkRecord dWorkRecord[1];

        ssSetSFcnDWork(rts, dWorkRecord);
        _ssSetNumDWork(rts, 1);

        /* PWORK */
        ssSetDWorkWidth(rts, 0, 1);
        ssSetDWorkDataType(rts, 0,SS_POINTER);
        ssSetDWorkComplexSignal(rts, 0, 0);
        ssSetDWork(rts, 0, &rtDWork.Scope_PWORK);
      }

      /* registration */
      sfun_rtai_scope(rts);

      sfcnInitializeSizes(rts);
      sfcnInitializeSampleTimes(rts);

      /* adjust sample time */
      ssSetSampleTime(rts, 0, 0.001);
      ssSetOffsetTime(rts, 0, 0.0);
      sfcnTsMap[0] = 1;

      /* set compiled values of dynamic vector attributes */

      ssSetNumNonsampledZCs(rts, 0);
      /* Update connectivity flags for each port */
      _ssSetInputPortConnected(rts, 0, 1);
      _ssSetInputPortConnected(rts, 1, 1);
      _ssSetInputPortConnected(rts, 2, 1);
      _ssSetInputPortConnected(rts, 3, 1);
      /* Update the BufferDstPort flags for each input port */
      ssSetInputPortBufferDstPort(rts, 0, -1);
      ssSetInputPortBufferDstPort(rts, 1, -1);
      ssSetInputPortBufferDstPort(rts, 2, -1);
      ssSetInputPortBufferDstPort(rts, 3, -1);
    }
  }

  return rtS;
}

