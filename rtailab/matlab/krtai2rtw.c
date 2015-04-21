/*
COPYRIGHT (C) 2003  Roberto Bucher (roberto.bucher@die.supsi.ch)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

#include "tmwtypes.h"
#include "simstruc.h"
#include "rt_sim.h"
#include "mdl_info.h"
#include "krtai2rtw.h"

#ifndef EXIT_FAILURE
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#endif

#define QUOTE1(name) #name
#define QUOTE(name) QUOTE1(name)    /* need to expand name    */

#ifndef RT
# error "must define RT"
#endif

#ifndef MODEL
# error "must define MODEL"
#endif

#ifndef NUMST
# error "must define number of sample times, NUMST"
#endif

#ifndef NCSTATES
# error "must define NCSTATES"
#endif

# if TID01EQ == 1
#  define FIRST_TID 1
# else
#  define FIRST_TID 0
# endif

#define MAX_RTAI_SCOPES         1000
#define MAX_RTAI_LOGS           1000

SimStruct * rtaiScope[MAX_RTAI_SCOPES];
SimStruct * rtaiLog[MAX_RTAI_LOGS];

extern SimStruct *MODEL(void);


SimStruct  *S;

extern void MdlInitializeSizes(void);
extern void MdlInitializeSampleTimes(void);
extern void MdlStart(void);
extern void MdlOutputs(int_T tid);
extern void MdlUpdate(int_T tid);
extern void MdlTerminate(void);

#if NCSTATES > 0
  extern void rt_CreateIntegrationData(SimStruct *S);
  extern void rt_UpdateContinuousStates(SimStruct *S);
#else
  #define rt_CreateIntegrationData(S)  ssSetSolverName(S,"FixedStepDiscrete");
  #define rt_UpdateContinuousStates(S) ssSetT(S,ssGetSolverStopTime(S));
#endif

real_T rtInf;
real_T rtMinusInf;
real_T rtNaN;

void rt_InitInfAndNaN(int_T realSize)
{
    short one = 1;
    enum {
        LittleEndian,
        BigEndian
    } machByteOrder = (*((char *) &one) == 1) ? LittleEndian : BigEndian;

    switch (realSize) {  
      case 4:
        switch (machByteOrder) {
          case LittleEndian: {
              typedef struct {
                  uint32_T fraction : 23;
                  uint32_T exponent  : 8;
                  uint32_T sign      : 1;
              } LittleEndianIEEEDouble;
	
              (*(LittleEndianIEEEDouble*)&rtInf).sign      = 0;
              (*(LittleEndianIEEEDouble*)&rtInf).exponent  = 0xFF;
              (*(LittleEndianIEEEDouble*)&rtInf).fraction  = 0;
              rtMinusInf = rtInf;
              rtNaN = rtInf;
              (*(LittleEndianIEEEDouble*)&rtMinusInf).sign = 1;
              (*(LittleEndianIEEEDouble*)&rtNaN).fraction  = 0x7FFFFF;
          }
          break;
          case BigEndian: {
              typedef struct {
                  uint32_T sign      : 1;
                  uint32_T exponent  : 8;
                  uint32_T fraction  : 23;
              } BigEndianIEEEDouble;
	
              (*(BigEndianIEEEDouble*)&rtInf).sign      = 0;
              (*(BigEndianIEEEDouble*)&rtInf).exponent  = 0xFF;
              (*(BigEndianIEEEDouble*)&rtInf).fraction  = 0;
              rtMinusInf = rtInf;
              rtNaN = rtInf;
              (*(BigEndianIEEEDouble*)&rtMinusInf).sign = 1;
              (*(BigEndianIEEEDouble*)&rtNaN).fraction  = 0x7FFFFF;
          }
          break;
        }
        break;    
    
      case 8:
        switch (machByteOrder) {
          case LittleEndian: {
              typedef struct {
                  struct {
                      uint32_T fraction2;
                  } wordH;
                  struct {
                      uint32_T fraction1 : 20;
                      uint32_T exponent  : 11;
                      uint32_T sign      : 1;
                  } wordL;
              } LittleEndianIEEEDouble;
              
              (*(LittleEndianIEEEDouble*)&rtInf).wordL.sign      = 0;
              (*(LittleEndianIEEEDouble*)&rtInf).wordL.exponent  = 0x7FF;
              (*(LittleEndianIEEEDouble*)&rtInf).wordL.fraction1 = 0;
              (*(LittleEndianIEEEDouble*)&rtInf).wordH.fraction2 = 0;
              
              rtMinusInf = rtInf;
              (*(LittleEndianIEEEDouble*)&rtMinusInf).wordL.sign = 1;
              (*(LittleEndianIEEEDouble*)&rtNaN).wordL.sign      = 0;
              (*(LittleEndianIEEEDouble*)&rtNaN).wordL.exponent  = 0x7FF;
              (*(LittleEndianIEEEDouble*)&rtNaN).wordL.fraction1 = 0xFFFFF;
              (*(LittleEndianIEEEDouble*)&rtNaN).wordH.fraction2 = 0xFFFFFFFF;
          }
          break;
          case BigEndian: {
              typedef struct {
                  struct {
                      uint32_T sign      : 1;
                      uint32_T exponent  : 11;
                      uint32_T fraction1 : 20;
                  } wordL;
                  struct {
                      uint32_T fraction2;
                  } wordH;
              } BigEndianIEEEDouble;
              
              (*(BigEndianIEEEDouble*)&rtInf).wordL.sign      = 0;
              (*(BigEndianIEEEDouble*)&rtInf).wordL.exponent  = 0x7FF;
              (*(BigEndianIEEEDouble*)&rtInf).wordL.fraction1 = 0;
              (*(BigEndianIEEEDouble*)&rtInf).wordH.fraction2 = 0;
	
              rtMinusInf = rtInf;
              (*(BigEndianIEEEDouble*)&rtMinusInf).wordL.sign = 1;
              (*(BigEndianIEEEDouble*)&rtNaN).wordL.sign      = 0;
              (*(BigEndianIEEEDouble*)&rtNaN).wordL.exponent  = 0x7FF;
              (*(BigEndianIEEEDouble*)&rtNaN).wordL.fraction1 = 0xFFFFF;
              (*(BigEndianIEEEDouble*)&rtNaN).wordH.fraction2 = 0xFFFFFFFF;
          }
          break;
        }
        break;
      default:
        break;
    }

} 

#if !defined(MULTITASKING)  /* SINGLETASKING */

static void rt_OneStep()
{
    real_T tnext;

    tnext = rt_GetNextSampleHit();
    ssSetSolverStopTime(S,tnext);

    MdlOutputs(0);
    MdlUpdate(0);
    rt_UpdateDiscreteTaskSampleHits(S);

    if (ssGetSampleTime(S,0) == CONTINUOUS_SAMPLE_TIME) {
        rt_UpdateContinuousStates(S);
    }
}

#else /* MULTITASKING */

static void rt_OneStep()
{
    int_T  eventFlags[NUMST];
    int_T  i;
    real_T tnext;
    int_T  *sampleHit = ssGetSampleHitPtr(S);

    tnext = rt_UpdateDiscreteEvents(S);
    ssSetSolverStopTime(S,tnext);
    for (i=FIRST_TID+1; i < NUMST; i++) {
        eventFlags[i] = sampleHit[i];
    }

    MdlOutputs(FIRST_TID);

   MdlUpdate(FIRST_TID);

    if (ssGetSampleTime(S,0) == CONTINUOUS_SAMPLE_TIME) {
        rt_UpdateContinuousStates(S);
    }
     else {
        rt_UpdateDiscreteTaskTime(S,0);
    }

#if FIRST_TID == 1
    rt_UpdateDiscreteTaskTime(S,1);
#endif

    for (i=FIRST_TID+1; i<NUMST; i++) {
        if (eventFlags[i]) {

            MdlOutputs(i);
            MdlUpdate(i);

            rt_UpdateDiscreteTaskTime(S,i);

        }
    }

} /* end rtOneStep */
#endif /* MULTITASKING */

void update_rtw()
{
  rt_OneStep();
}

void start_rtw()
{
  rt_InitInfAndNaN(sizeof(real_T));
  S = MODEL();
  MdlInitializeSizes();
  MdlInitializeSampleTimes();
  rt_InitTimingEngine(S);

  rt_CreateIntegrationData(S);

  MdlStart();
}

void stop_rtw()
{
  MdlTerminate();
}

long long get_t_samp()
{
  double t=ssGetSampleTime(S,FIRST_TID);
  if(t==0.0) return(1000000L);
  else return((long long) (t*1.0e9));
}

void __bzero __P ((__ptr_t __s, size_t __n)) 
{
   char *s = (char*)__s;
   while(__n--)
     *s++ = 0;
}

int get_nBlockParams()
{
    ModelMappingInfo *MMI;
    MMI=(ModelMappingInfo *) ssGetModelMappingInfo(S);
    return(mmiGetNumBlockParams(MMI));
}

static inline void strncpyz(char *dest, const char *src, size_t n)
{
    int i;

    for(i=0;i<n;i++) dest[i]=src[i];
    dest[n-1] = '\0';
}

static inline void set_double(double *to, double *from)
{
    unsigned long l = ((unsigned long *)from)[0];
    unsigned long h = ((unsigned long *)from)[1];
    __asm__ __volatile__ (
	"1: movl (%0), %%eax; movl 4(%0), %%edx; lock; cmpxchg8b (%0); jnz 1b" : : "D"(to), "b"(l), "c"(h) : "ax", "dx", "memory");
}

int rt_GetParameterInfo(rtTargetParamInfo *rtpi, int i)
{
    ModelMappingInfo *mmi;
    ParameterTuning *ptRec;
    void * const     *pMap;
    uint_T nParams;
    uint_T mapOffset;
    uint_T paramIdx;

    mmi=(ModelMappingInfo *) ssGetModelMappingInfo(S);
    ptRec = (ParameterTuning*)mmiGetBlockTuningParamInfo(mmi,i);
    pMap  = mmiGetParametersMap(mmi);
    nParams   = ptinfoGetNumInstances(ptRec);
    mapOffset = ptinfoGetParametersOffset(ptRec);

    strncpyz(rtpi->modelName, ssGetModelName(S), MAX_NAME_SIZE);
    strncpyz(rtpi->blockName, mmiGetBlockTuningBlockName(mmi,i), MAX_NAME_SIZE);
    strncpyz(rtpi->paramName, mmiGetBlockTuningParamName(mmi,i), MAX_NAME_SIZE);

    switch (ptinfoGetDataTypeEnum(ptRec)) {

	case SS_DOUBLE:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		real_T *param = (real_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			rtpi->dataType = SS_DOUBLE;
			rtpi->dataClass = rt_SCALAR;
			rtpi->dataValue = (double)(*param);
			break;
              
		    default:
			return(1);
		}
	    }
	    break;

	case SS_SINGLE:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		real32_T *param = (real32_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			rtpi->dataType = SS_SINGLE;
			rtpi->dataClass = rt_SCALAR;
			rtpi->dataValue = (float)(*param);
			break;
              
		    default:
			return(1);
		}
	    }
	    break;

	case SS_INT8:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		int8_T *param = (int8_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			rtpi->dataType = SS_INT8;
			rtpi->dataClass = rt_SCALAR;
			rtpi->dataValue = (signed char)(*param);
			break;
              
		    default:
			return(1);
		}
	    }
	    break;

	case SS_UINT8:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		uint8_T *param = (uint8_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			rtpi->dataType = SS_UINT8;
			rtpi->dataClass = rt_SCALAR;
			rtpi->dataValue = (unsigned char)(*param);
			break;
              
		    default:
			return(1);
		}
	    }
	    break;
		
	case SS_INT16:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		int16_T *param = (int16_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			rtpi->dataType = SS_INT16;
			rtpi->dataClass = rt_SCALAR;
			rtpi->dataValue = (signed short int)(*param);
			break;
              
		    default:
			return(1);
		}
	    }
	    break;

	case SS_UINT16:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		uint16_T *param = (uint16_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			rtpi->dataType = SS_UINT16;
			rtpi->dataClass = rt_SCALAR;
			rtpi->dataValue = (unsigned short int)(*param);
			break;
              
		    default:
			return(1);
		}
	    }
	    break;
		
	case SS_INT32:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		int32_T *param = (int32_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			rtpi->dataType = SS_INT32;
			rtpi->dataClass = rt_SCALAR;
			rtpi->dataValue = (signed int)(*param);
			break;
              
		    default:
			return(1);
		}
	    }
	    break;

	case SS_UINT32:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		uint32_T *param = (uint32_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			rtpi->dataType = SS_UINT32;
			rtpi->dataClass = rt_SCALAR;
			rtpi->dataValue = (unsigned int)(*param);
			break;
              
		    default:
			return(1);
		}
	    }
	    break;
		
	case SS_BOOLEAN:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		boolean_T *param = (boolean_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			rtpi->dataType = SS_BOOLEAN;
			rtpi->dataClass = rt_SCALAR;
			rtpi->dataValue = (unsigned char)(*param);
			break;
              
		    default:
			return(1);
		}
	    }
	    break;
		
	default:

	    return(1);
    }

    return(0);
}

int_T rt_ModifyParameterValue(int i, void *_newVal)
{
    ModelMappingInfo *mmi;
    ParameterTuning *ptRec;
    void * const     *pMap;
    uint_T nParams;
    uint_T mapOffset;
    uint_T paramIdx;

    mmi=(ModelMappingInfo *) ssGetModelMappingInfo(S);
    ptRec = (ParameterTuning*)mmiGetBlockTuningParamInfo(mmi,i);
    pMap  = mmiGetParametersMap(mmi);
    nParams   = ptinfoGetNumInstances(ptRec);
    mapOffset = ptinfoGetParametersOffset(ptRec);

    switch (ptinfoGetDataTypeEnum(ptRec)) {

	case SS_DOUBLE:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		real_T *param = (real_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			set_double(param, (double *)_newVal);
//						*param = ((double *) _newVal)[0];
			break;
		    default:
			return(1);
		}
	    }
	    break;

	case SS_SINGLE:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		real32_T *param = (real32_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			*param = ((double *) _newVal)[0];
			break;
		    default:
			return(1);
		}
	    }
	    break;

	case SS_INT8:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		int8_T *param = (int8_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			*param = ((double *) _newVal)[0];
			break;
		    default:
			return(1);
		}
	    }
	    break;

	case SS_UINT8:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		uint8_T *param = (uint8_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			*param = ((double *) _newVal)[0];
			break;
		    default:
			return(1);
		}
	    }
	    break;

	case SS_INT16:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		int16_T *param = (int16_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			*param = ((double *) _newVal)[0];
			break;
		    default:
			return(1);
		}
	    }
	    break;

	case SS_UINT16:
		
	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		uint16_T *param = (uint16_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			*param = ((double *) _newVal)[0];
			break;
		    default:
			return(1);
		}
	    }
	    break;

	case SS_INT32:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		int32_T *param = (int32_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			*param = ((double *) _newVal)[0];
			break;
		    default:
			return(1);
		}
	    }
	    break;

	case SS_UINT32:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		uint32_T *param = (uint32_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			*param = ((double *) _newVal)[0];
			break;
		    default:
			return(1);
		}
	    }
	    break;

	case SS_BOOLEAN:

	    for (paramIdx = 0; paramIdx < nParams; paramIdx++) {
		boolean_T *param = (boolean_T *)(pMap[mapOffset + paramIdx]);
		switch (ptinfoGetClass(ptRec)) {
		    case rt_SCALAR:
			*param = ((double *) _newVal)[0];
			break;
		    default:
			return(1);
		}
	    }
	    break;

	default:

	    return(1);
    }

    return(0);
}

float rtGetT(){
    return((float) ssGetT(S));
}
    
int rtGetNumInpP_scope(int idx)
{
    return(ssGetNumInputPorts(rtaiScope[idx]));
}

int * rtGetInpPDim_log(int idx)
{
    return(ssGetInputPortDimensions(rtaiLog[idx],0));
}

char * rtGetModelName_scope(int idx)
{
    return((char *)ssGetModelName(rtaiScope[idx]));
}

char * rtGetModelName_log(int idx)
{
    return((char *)ssGetModelName(rtaiLog[idx]));
}

float rtGetSampT_scope(int idx)
{
    return(ssGetSampleTime(rtaiScope[idx],0));
}

float rtGetSampT_log(int idx)
{
    return(ssGetSampleTime(rtaiLog[idx],0));
}







