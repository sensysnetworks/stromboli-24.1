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
         out_rtai_scope_output(1,u,*t);
      } 
      break;
    case 4 : /* actuator initialisation */
      out_rtai_scope_init(1,2,"IO",0.000000,0.000000,0.000000,0.000000,0.000000);
      break;
    case 5 : /* actuator ending */
      out_rtai_scope_end(1);
      break;
    }
  break;
  case 2 :/* Port number 2 ----------*/
    /* skeleton to be customized */
    switch (*flag) {
    case 2 : 
      if(*nevprt>0) {/* get the input value */
         out_rtai_scope_output(2,u,*t);
      } 
      break;
    case 4 : /* actuator initialisation */
      out_rtai_scope_init(2,1,"U",0.000000,0.000000,0.000000,0.000000,0.000000);
      break;
    case 5 : /* actuator ending */
      out_rtai_scope_end(2);
      break;
    }
  break;
  case 3 :/* Port number 3 ----------*/
    /* skeleton to be customized */
    switch (*flag) {
    case 2 : 
      if(*nevprt>0) {/* get the input value */
         out_rtai_led_output(3,u,*t);
      } 
      break;
    case 4 : /* actuator initialisation */
      out_rtai_led_init(3,1,"LED",0.000000,0.000000,0.000000,0.000000,0.000000);
      break;
    case 5 : /* actuator ending */
      out_rtai_led_end(3);
      break;
    }
  break;
  case 4 :/* Port number 4 ----------*/
    /* skeleton to be customized */
    switch (*flag) {
    case 2 : 
      if(*nevprt>0) {/* get the input value */
         out_rtai_meter_output(4,u,*t);
      } 
      break;
    case 4 : /* actuator initialisation */
      out_rtai_meter_init(4,1,"METER",0.000000,0.000000,0.000000,0.000000,0.000000);
      break;
    case 5 : /* actuator ending */
      out_rtai_meter_end(4);
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
    inp_square_input(1,y,*t);
    break;
  case 2 : /* Update internal discrete state if any */
    inp_square_update();
    break;
  case 4 : /* sensor initialisation */
    inp_square_init(1,0,"0",1.000000,10.000000,5.000000,0.000000,0.000000);
    break;
  case 5 : /* sensor ending */
    inp_square_end(1);
    break;
  }
}
/*---------------------------------------- callback at user params updates */ 
void 
test_upar_update(int index)
{
}

