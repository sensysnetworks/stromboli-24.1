//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 2000 Pierre Cloutier (Poseidon Controls Inc.),
//                         Steve Papacharalambous (Lineo Inc.),
//                         All rights reserved
//
// Authors:             Pierre Cloutier (pcloutier@poseidoncontrols.com)
//                      Steve Papacharalambous (stevep@lineo.com)
//
// Original date:       Wed 23 Feb 2000
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
// Dynamic Memory Management simple test program for Real Time Linux.
//
///////////////////////////////////////////////////////////////////////////////
static char id_rt_mem_test_c[] __attribute__ ((unused)) = "@(#)$Id: rt_mem_test.c,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $";

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


// ----------------------------------------------------------------------------
//      Local Definitions.
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//      Package Global Data.
// ----------------------------------------------------------------------------
RT_TASK mem_thread;


// ----------------------------------------------------------------------------
void mem_alloc(int t)
{

  unsigned int mem_size = 0x4000;
  void *mem_ptr1 = NULL;
  void *mem_ptr2 = NULL;
  void *mem_ptr3 = NULL;

  if((mem_ptr1 = rt_malloc(mem_size)) == NULL) {
    rt_printk("mem_alloc - Error Allocating %d bytes.\n", mem_size);
  } else {
    DBG("mem_alloc - Allocated %d bytes, address: %p.\n", mem_size, mem_ptr1);
  }

  if((mem_ptr2 = rt_malloc(mem_size - 0x2000)) == NULL) {
    rt_printk("mem_alloc - Error Allocating %d bytes.\n", mem_size - 0x2000);
  } else {
    DBG("mem_alloc - Allocated %d bytes, address: %p.\n", mem_size - 0x2000,
                                                          mem_ptr2);
  }

  if((mem_ptr3 = rt_malloc(mem_size + 0x3000)) == NULL) {
    rt_printk("mem_alloc - Error Allocating %d bytes.\n", mem_size + 0x3000);
  } else {
    DBG("mem_alloc - Allocated %d bytes, address: %p.\n", mem_size + 0x3000,
                                                          mem_ptr3);
  }

  display_chunk(mem_ptr1);
  if(mem_ptr2 != NULL) {
    rt_free(mem_ptr2);
  }
  display_chunk(mem_ptr1);
  if(mem_ptr1 != NULL) {
    rt_free(mem_ptr1);
  }
  display_chunk(mem_ptr3);
  if(mem_ptr3 != NULL) {
    rt_free(mem_ptr3);
  }

//  display_chunk(mem_ptr1);
  rt_task_suspend(rt_whoami());

}  // End function - mem_alloc


// ----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
// Module Initialisation/Finalisation
//
///////////////////////////////////////////////////////////////////////////////
int init_module(void)
{

  int r_c = 0;
  RTIME tick_period;

  if((r_c = rt_task_init(&mem_thread, mem_alloc, 0,
                          STACK_SIZE, 2, 1, 0)) < 0) {

    printk("rt_mem_test - Error creating mem_thread: %d\n", r_c);
  }

#ifdef ZDEBUG
  printk("rt_mem_test - Created memory allocation thread.\n");
#endif

  tick_period = start_rt_timer(nano2count(TICK_PERIOD));
  if((r_c = rt_task_make_periodic(&mem_thread, rt_get_time()
                                   + (2 * tick_period), tick_period)) < 0) {

    printk("rt_mem_test - Error making mem_thread periodic: %d\n", r_c);
  }

  return(r_c);

} // End function - init_module


// -----------------------------------------------------------------------------
void cleanup_module(void)
{

  stop_rt_timer();
  rt_busy_sleep(10000000);
  rt_task_delete(&mem_thread);


} // End function - cleanup_module


// ---------------------------------< eof >------------------------------------
