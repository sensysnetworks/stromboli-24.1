/*****************************************************************
 * File : rtai_tracer.c
 * Description :
 *    Contains the code for the RTAI tracing driver.
 * Author :
 *    Karim Yaghmour (karym@opersys.com)
 * Date :
 *    16/05/00, Initial typing.
 * Note :
 *****************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <rtai_trace.h>

/* Local variables */
static int        rt_tracer_registered = 0;   /* Is there a tracer registered */
tracer_call       rt_tracer = NULL;           /* The registered tracer */

/****************************************************
 * Function: rt_register_tracer()
 * Description:
 *   Register the tracer to RTAI
 * Return values :
 *   0, all is OK
 *   -1, tracer already registered
 ****************************************************/
int rt_register_tracer(tracer_call pmTraceFunction)
{
  /* Is there a tracer already registered */
  if(rt_tracer_registered == 1)
    return -1;

  /* Set the tracer to the one being passed by the caller */
  rt_tracer = pmTraceFunction;

  /* Remember that a tracer is now registered */
  rt_tracer_registered = 1;

  /* Tell the caller that everything went fine */
  return 0;
}

/***************************************************
 * Function: rt_unregister_tracer()
 * Description:
 *   Unregister the currently registered tracer.
 * Return values :
 *   0, all is OK
 *   -ENOMEDIUM, there isn't a registered tracer
 *   -ENXIO, unregestering wrong tracer
 ***************************************************/
int rt_unregister_tracer(tracer_call pmTraceFunction)
{
  /* Is there a tracer already registered */
  if(rt_tracer_registered == 0)
    /* Nothing to unregister */
    return -ENOMEDIUM;

  /* Is it the tracer that was registered */
  if(rt_tracer == pmTraceFunction)
    /* There isn't any tracer in here */
    rt_tracer_registered = 0;
  else
    return -ENXIO;

  /* Reset tracer function */
  rt_tracer = NULL;

  /* Reset the registered flag */
  rt_tracer_registered = 0;

  /* Tell the caller that everything went OK */
  return 0;
}

/*******************************************************
 * Function: rt_trace_event()
 * Description:
 *   Trace an event
 * Parameters :
 *   pmEventID, the event's ID (check out rtai_trace.h)
 *   pmEventStruct, the structure describing the event
 * Return values :
 *   0, all is OK
 *   -ENOMEDIUM, there isn't a registered tracer
 *   -EBUSY, tracing hasn't started yet
 *******************************************************/
int rt_trace_event(uint8_t  pmEventID, 
		   void*    pmEventStruct)
{
  /* Is there a tracer registered */
  if(rt_tracer_registered != 1)
    return -ENOMEDIUM;

  /* Call the tracer */
  return (rt_tracer(pmEventID, pmEventStruct));
}

/*******************************************************
 * Function: init_module()
 * Description:
 *   Initializes the RTAI trace module
 * Parameters :
 *   NONE
 * Return values :
 *   0, all is OK
 *******************************************************/
int init_module(void)
{
  printk("RTAI-TRACE: Initilialization complete \n");
  return 0;
}

/*******************************************************
 * Function: cleanup_module()
 * Description:
 *   Cleans-up the RTAI trace module
 * Parameters :
 *   NONE
 * Return values :
 *   NONE
 *******************************************************/
void cleanup_module(void)
{
  printk("RTAI-TRACE: Trace facility removed \n");
}
