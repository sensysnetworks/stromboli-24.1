#ifndef _RTAI_SCHED_MEM_H_
#define _RTAI_SCHED_MEM_H_
//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@zentropix.com)
// Original date:       Sat 28 Aug 1999
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
// Memory Allocation for the RTAI Schedulers
//
///////////////////////////////////////////////////////////////////////////////

#define MAX_RT_TASKS 128        // Maximum number of concurrent RT tasks.
#define MAX_STACK_SIZE 4000     // Maximum stack size.

// ----------------------------------------------------------------------------

extern int rtai_mem_init(unsigned int chunk_size, unsigned int num_chunks);
extern void *rtai_malloc(int size);
extern void rtai_free(void *mem_ptr);
extern void rtai_mem_end(void);

// ---------------------------------< eof >------------------------------------
#endif  // _RTAI_SCHED_MEM_H_
