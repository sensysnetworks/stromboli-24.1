#define MBX_BUF_SIZE  1000
static MBX *mbx;

#include "/home/mante/scilab-2.6/routines/machine.h"
/*---------------------------------------- Actuators */ 
void 
pid_actuator(flag,nport,nevprt,t,u,nu)
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
  static struct { int nh; double t, h[2]; } th = { 2, };

  switch (*nport) {
  case 1 :/* Port number 1 ----------*/
    /* skeleton to be customized */
    switch (*flag) {
    case 2 : 
      if(*nevprt>0) {/* get the input value */
  	/*for (k=0;k<*nu;k++) {????=u[k];} */
        th.t = *t;
        th.h[0] = u[0];
      } 
      break;
    case 4 : /* actuator initialisation */
      /* do whatever you want to initialize the actuator */
      break;
    case 5 : /* actuator ending */
      /* do whatever you want to end the actuator */
      break;
    }
  break;
  case 2 :/* Port number 2 ----------*/
    /* skeleton to be customized */
    switch (*flag) {
    case 2 : 
      if(*nevprt>0) {/* get the input value */
  	/*for (k=0;k<*nu;k++) {????=u[k];} */
        th.h[1] = u[0];
        rt_mbx_send_if(mbx, &th, sizeof(th));
      } 
      break;
    case 4 : /* actuator initialisation */
      /* do whatever you want to initialize the actuator */
      {
        char name[7];
        sprintf(name, "SCOP%c0", minid);
        mbx = rt_mbx_init(nam2num(name), MBX_BUF_SIZE);
      }
      break;
    case 5 : /* actuator ending */
      /* do whatever you want to end the actuator */
      rt_mbx_delete(mbx);
      break;
    }
  break;
  }
}
/*---------------------------------------- Sensor */ 
void 
pid_sensor(flag,nport,nevprt,t,y,ny)
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
    /* for (k=0;k<*ny;k++) {y[k]=????;}*/
	y[0] = sin(*t);
    break;
  case 2 : /* Update internal discrete state if any */
    break;
  case 4 : /* sensor initialisation */
    /* do whatever you want to initialize the sensor */
    break;
  case 5 : /* sensor ending */
    /* do whatever you want to end the sensor */
    break;
  }
}
