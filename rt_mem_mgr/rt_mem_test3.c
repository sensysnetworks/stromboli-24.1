//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 2000 Pierre Cloutier (Poseidon Controls Inc.),
//                         Steve Papacharalambous (Lineo Inc.),
//                         All rights reserved
//
// Authors:             Pierre Cloutier (pcloutier@poseidoncontrols.com)
//                      Steve Papacharalambous (stevep@lineo.com)
//
// Original date:       Wed 19 Jul 2000
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
//
// Dynamic Memory Management test program for Real Time Linux.
//
// This program creates a large number of RT tasks and which suspend.
// This is to allow the memory allocation algorithms to be debugged.
//
///////////////////////////////////////////////////////////////////////////////
static char id_rt_mem_test3_c[] __attribute__ ((unused)) = "@(#)$Id: rt_mem_test3.c,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $";

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/errno.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rt_mem_mgr.h>

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

// ------------------------------< definitions >-------------------------------
#ifndef NULL
#define NULL ((void *) 0)
#endif

#define TICK_PERIOD 50000000

#define STACK_SIZE 2000

#define NUM_THREADS 64 


// ----------------------------------------------------------------------------
//      Local Definitions.
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//      Package Global Data.
// ----------------------------------------------------------------------------
RT_TASK task_struct[NUM_THREADS];
int threads_created = 0;


// ----------------------------------------------------------------------------
void task_func(int t)
{

// Just suspend...
  rt_task_suspend(rt_whoami());

}  // End function - task_func


// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
// Module Initialisation/Finalisation
//
///////////////////////////////////////////////////////////////////////////////
int init_module(void)
{

  int r_c = 0, i;
  RTIME tick_period;

  tick_period = start_rt_timer(nano2count(TICK_PERIOD));
  for(i = 0; i < NUM_THREADS; i++) {
    if((r_c = rt_task_init(&task_struct[i], task_func, 0,
                              STACK_SIZE, 2, 1, 0)) < 0) {

      printk("rt_mem_test3 - Error creating task_func #%d: %d\n", i, r_c);
    }

#ifdef ZDEBUG
    printk("rt_mem_test3 - Created dummy thread #%d.\n", i);
#endif


    if((r_c = rt_task_make_periodic(&task_struct[i], rt_get_time()
                                     + (2 * i * tick_period), tick_period)) < 0) {

      printk("rt_mem_test3 - Error making task_func #%d periodic: %d\n",
                                                                     i, r_c);
    }
    threads_created++;

  }  // End for loop - create NUM_THREADS tasks.
  printk("rt_mem_test3 - Created %d threads.\n", threads_created);
  return(r_c);

} // End function - init_module


// -----------------------------------------------------------------------------
void cleanup_module(void)
{

  int i;

  stop_rt_timer();
  rt_busy_sleep(10000000);
  printk("Killed: ");
  for(i = 0; i < threads_created; i++) {
    rt_task_delete(&task_struct[i]);
    printk("%d, ", i);
  }
  printk("\n");

} // End function - cleanup_module


// ---------------------------------< eof >------------------------------------
