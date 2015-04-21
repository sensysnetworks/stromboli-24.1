#include "/usr/lib/scilab-2.7/routines/machine.h"
/*---------------------------------------- Actuators */ 
void 
test_actuator(flag,nport,nevprt,t,u,nu)
     /*
      * To be customized for standalone execution
      * flag  : specifies the action to be done
      * nport : specifies the  index of the Super Bloc 
      *         regular input (The input ports are numbered 
      *         from the top to the bottom ) 
      * nevprt: indicates if an activation had been received
      *         0 = no activation
      *         1 = activation
      * t     : the current time value
      * u     : the vector inputs value
      * nu    : the input  vector size
      */
     integer *flag,*nevprt,*nport;
     integer *nu;

     double  *t, u[];
{
  int k;
  switch (*nport) {
  case 1 :/* Port number 1 ----------*/
    /* skeleton to be customized */
    switch (*flag) {
    case 2 : 
      if(*nevprt>0) {/* get the input value */
         /* att_1_output */
      } 
      break;
    case 4 : /* actuator initialisation */
      /* att_1_init */
      break;
    case 5 : /* actuator ending */
      /* att_1_end */
      break;
    }
  break;
  case 2 :/* Port number 2 ----------*/
    /* skeleton to be customized */
    switch (*flag) {
    case 2 : 
      if(*nevprt>0) {/* get the input value */
         /* att_2_output */
      } 
      break;
    case 4 : /* actuator initialisation */
      /* att_2_init */
      break;
    case 5 : /* actuator ending */
      /* att_2_end */
      break;
    }
  break;
  case 3 :/* Port number 3 ----------*/
    /* skeleton to be customized */
    switch (*flag) {
    case 2 : 
      if(*nevprt>0) {/* get the input value */
         /* att_3_output */
      } 
      break;
    case 4 : /* actuator initialisation */
      /* att_3_init */
      break;
    case 5 : /* actuator ending */
      /* att_3_end */
      break;
    }
  break;
  case 4 :/* Port number 4 ----------*/
    /* skeleton to be customized */
    switch (*flag) {
    case 2 : 
      if(*nevprt>0) {/* get the input value */
         /* att_4_output */
      } 
      break;
    case 4 : /* actuator initialisation */
      /* att_4_init */
      break;
    case 5 : /* actuator ending */
      /* att_4_end */
      break;
    }
  break;
  }
}
/*---------------------------------------- Sensor */ 
void 
test_sensor(flag,nport,nevprt,t,y,ny)
     /*
      * To be customized for standalone execution
      * flag  : specifies the action to be done
      * nport : specifies the  index of the Super Bloc 
      *         regular input (The input ports are numbered 
      *         from the top to the bottom ) 
      * nevprt: indicates if an activation had been received
      *         0 = no activation
      *         1 = activation 
      * t     : the current time value
      * y     : the vector outputs value
      * ny    : the output  vector size
      */
     integer *flag,*nevprt,*nport;
     integer *ny;

     double  *t, y[];
{
  int k;
  switch (*flag) {
  case 1 : /* set the ouput value */
    /* sens_1_input */
    break;
  case 2 : /* Update internal discrete state if any */
    /* sens_1_update */
    break;
  case 4 : /* sensor initialisation */
    /* sens_1_init */
    break;
  case 5 : /* sensor ending */
    /* sens_1_end */
    break;
  }
}
/*---------------------------------------- callback at user params updates */ 
void 
test_upar_update(int index)
{
}

