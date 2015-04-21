//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 2000 Pierre Cloutier (Poseidon Controls Inc.),
//                         Steve Papacharalambous (Lineo Inc.),
//                         All rights reserved
//
// Authors:             Pierre Cloutier (pcloutier@poseidoncontrols.com)
//                      Steve Papacharalambous (stevep@lineo.com)
//
// Original date:       Sat 04 Mar 2000
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
///////////////////////////////////////////////////////////////////////////////
static char id_rt_mem_test2_c[] __attribute__ ((unused)) = "@(#)$Id: rt_mem_test2.c,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $";

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

#define NUM_BLOCKS 8


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

  int i;
  unsigned int mem_size = 0x2000;
  void *mem_ptr[NUM_BLOCKS] = {NULL};
  void *my_mem_ptr = NULL;

// Allocate a bunch of memory blocks.
  for( i = 0; i < NUM_BLOCKS; i++ ) {
    if((mem_ptr[i] = rt_malloc(mem_size + i * 32)) == NULL) {
      rt_printk("mem_alloc - Error Allocating %d bytes.\n", mem_size + i * 32);
      goto alloc_err;
    } // End if - Error allocating memory block.
  }  // End for loop - allocate NUM_BLOCKS of memory.


// Allocation to force a chunk to fall beneath the low data mark.
  if((my_mem_ptr = rt_malloc(7200)) == NULL) {
    rt_printk("mem_alloc - Error Allocating %d bytes.\n", 7200);
  } // End if - Error allocating memory block.


// Now display the allocation details.
  display_chunk(mem_ptr[0]);
  display_chunk(my_mem_ptr);
alloc_err:
  rt_sleep(nano2count(5000000000LL));

// Free a bunch of memory blocks.
  for( i = 0; i < NUM_BLOCKS; i++ ) {
    if(mem_ptr[i] != NULL) {
      rt_free(mem_ptr[i]);
    }  // End if - Don't free NULL pointers.
  }  // End for loop - free NUM_BLOCKS of memory.

  if(my_mem_ptr != NULL) {
    rt_free(my_mem_ptr);
  }  // End if - Don't free a NULL pointer.

// Now display the allocation details.
  display_chunk(mem_ptr[0]);
  display_chunk(my_mem_ptr);

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

    printk("rt_mem_test2 - Error creating mem_thread: %d\n", r_c);
    return(r_c);
  }

#ifdef ZDEBUG
  printk("rt_mem_test2 - Created memory allocation thread.\n");
#endif

  tick_period = start_rt_timer(nano2count(TICK_PERIOD));
  if((r_c = rt_task_make_periodic(&mem_thread, rt_get_time()
                                   + (2 * tick_period), tick_period)) < 0) {

    printk("rt_mem_test2 - Error making mem_thread periodic: %d\n", r_c);
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
