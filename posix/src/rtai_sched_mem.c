//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@zentropix.com)
// Original date:       Fri 27 Aug 1999
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
// Utility routines for Real Time Linux.
//
///////////////////////////////////////////////////////////////////////////////
#include <linux/vmalloc.h>
#include <linux/errno.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include "rtai_sched_mem.h"
#include "zdefs.h"


// ----------------------------------------------------------------------------
#ifndef NULL
#define NULL ((void *) 0)
#endif


// ----------------------------------------------------------------------------
//      Local Definitions.
// ----------------------------------------------------------------------------
PACKAGE_NAME(sched_mem_alloc util);


// ----------------------------------------------------------------------------
//      Package Global Data.
// ----------------------------------------------------------------------------
static unsigned int alloc_size = 0;
static unsigned int chunk_count = 0;
static char *rt_mem_ptr = NULL;
static char *rt_mem_stat_ptr = NULL;
static spinlock_t rt_mem_lock;


// ----------------------------------------------------------------------------
// RT memory allocation functions.
//
// These routines provide a very simple form of memory allocation which
// can by used by RT tasks that cannot be blocked or subjected to
// significant delays during memory allocation.
//
// Currently only fixed memory chunks of preset size are allocated, requests
// for memory sizes greater than the preset size are rejected.   Requests
// for memory sizes less than or equal to the preset size are each allocated
// a chunk of this memory providing there is a free one available in the pool.
//
// These limits, the memory chunk size and maximum number of memory chunks
// in the pool are set by the application by calling rt_mem_init
// The application must call rt_mem_init once to initialise the memory
// pool.  It can then use the functions rt_malloc and rt_free to dynamically
// allocate and free memory during RT operation.
// It must call rt_mem_end on termination so that the memory can
// be cleaned up.
//

//-----------------------------------------------------------------------------

// Memory allocation system initialisation.
int rtai_mem_init(unsigned int chunk_size, unsigned int num_chunks) 
{

  F_HDR(rtai_mem_init);

  int i;

  if(num_chunks <= 0 || chunk_size <= 0) {
    DBG("Illegal argument(s) to rtai_mem_init.\n");
    return(-EINVAL);
  }

  rt_mem_ptr = (char *)vmalloc(chunk_size * num_chunks);
  if(!rt_mem_ptr) {
    DBG("Failed to allocate RT memory.\n");
    return(-ENOMEM);
  }

  memset((void *)rt_mem_ptr, 0, (chunk_size * num_chunks));

  rt_mem_stat_ptr = (char *)vmalloc(num_chunks);
  if(!rt_mem_ptr) {
    DBG("Failed to allocate RT management memory.\n");
    return(-ENOMEM);
  }

  memset((void *)rt_mem_stat_ptr, 0, num_chunks);

  alloc_size = chunk_size;
  chunk_count = num_chunks;

  for(i = 0; i < chunk_size * num_chunks; i++) {
    *(rt_mem_ptr + i) = 0;
  }

  for(i = 0; i < num_chunks; i++) {
    *(rt_mem_stat_ptr + i) = 0;
  }

  spin_lock_init(&rt_mem_lock);

  DBG("Allocated %d chunks, size: %d, Start address: %p\n", num_chunks, chunk_size, rt_mem_ptr);

  return(0);

}  // End function - rtai_mem_init

//-----------------------------------------------------------------------------

// Memory allocation.
void *rtai_malloc(int size) 
{

  F_HDR(rtai_malloc);

  int i;
  int found_free_chunk;
  unsigned long flags;

  if(size > alloc_size) {
    DBG("Asking for too large a chunk...rejected\n");
    return(NULL);
  }

  found_free_chunk = 0;
  flags = rt_spin_lock_irqsave(&rt_mem_lock);
  for(i = 0; i < chunk_count; i++) {
    if(*(rt_mem_stat_ptr + i) == 0) {
      found_free_chunk = 1;
      break;
    }
  }

  if(found_free_chunk) {
    *(rt_mem_stat_ptr + i) = 1;
    rt_spin_unlock_irqrestore(flags, &rt_mem_lock);
    DBG("Allocating chunk %d, Address: %p\n", i, (rt_mem_ptr + (i * alloc_size)));
    return((void *)(rt_mem_ptr + (i * alloc_size)));
  }

  rt_spin_unlock_irqrestore(flags, &rt_mem_lock);
  DBG("No free chunks left.\n");
  return(NULL);

}  // End function - rtai_malloc

//-----------------------------------------------------------------------------

// Memory release.
void rtai_free(void *mem_ptr) 
{

  F_HDR(rtai_free);

  unsigned int mem_index;
  unsigned long flags;

  if(mem_ptr == NULL) {
    DBG("Call to rtai_free with a NULL pointer.\n");
    return;
  }

  mem_index = ((unsigned int)mem_ptr - (unsigned int)rt_mem_ptr) / alloc_size;
  if(mem_index > chunk_count) {
    DBG("Attempt to free unknown memory.\n");
    return;
  }

  flags = rt_spin_lock_irqsave(&rt_mem_lock);
  *(rt_mem_stat_ptr + mem_index) = 0;
  rt_spin_unlock_irqrestore(flags, &rt_mem_lock);
  DBG("Deallocating chunk %d, Address: %p\n", mem_index, mem_ptr);
  return;

}  // End function - rtai_free

//-----------------------------------------------------------------------------

// Memory allocation system termination.
void rtai_mem_end(void) 
{

  F_HDR(rtai_mem_end);

  vfree((void *)rt_mem_ptr);
  vfree((void *)rt_mem_stat_ptr);

}  // End function - rtai_mem_end

// ---------------------------------< eof >------------------------------------
