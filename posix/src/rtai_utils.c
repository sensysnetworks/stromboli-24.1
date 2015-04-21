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
static char id_rtai_utils_c[] __attribute__ ((unused)) = "@(#)$Id: rtai_utils.c,v 1.1.1.1 2004/06/06 14:03:03 rpm Exp $";

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <linux/mm.h>
#include <linux/errno.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include "rtai_utils.h"
#include "rt_mem_mgr.h"
#include "zdefs.h"

// ----------------------------------------------------------------------------

#ifndef NULL
#define NULL ((void *) 0)
#endif


// ----------------------------------------------------------------------------
//      Local Definitions.
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
//      Package Global Data.
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------

// Misc functions.

extern void console_print(const char *b);


static inline void conout(const char *mesg) {

  console_print(mesg);
}

///////////////////////////////////////////////////////////////////////////////
// Zentropix Application Data Structure Controls
//
// The Z_APPS structure contains a set of pointers to Zentropix-Specific
// Application Data Control Structures such as pThreads, pQueues etc.
// These void pointers are initialised as required by the appropriate
// application(s) to point to their own relevant data structures.
//
// The application-specific data is tied to each 'Task' via the system_data_ptr
// in the RT_TASK structure, which is initialised to point to a Z_APPS
// structure when necessary.
//
///////////////////////////////////////////////////////////////////////////////
void *init_z_apps(void *this_task)
{
  Z_APPS *zapps;
  RT_TASK *task = (RT_TASK*)this_task;

    if( task->system_data_ptr == NULL) {
	// This task has not yet created a Z_APPS structure
    	task->system_data_ptr = rt_malloc(sizeof(Z_APPS));
    	zapps = (Z_APPS*)task->system_data_ptr;

	// Now initialise it
	zapps->in_use_count = 0;
	zapps->pthreads = NULL;
	zapps->pqueues  = NULL;
	zapps->vxtasks  = NULL;

    }
    return task->system_data_ptr;

} // End function - init_z_apps

//-----------------------------------------------------------------------------
void free_z_apps(void *this_task)
{
  RT_TASK *task = (RT_TASK*)this_task;

    if(task->system_data_ptr != NULL) {
	rt_free(task->system_data_ptr);
	task->system_data_ptr = NULL;
    }

} // End function - free_z_apps

///////////////////////////////////////////////////////////////////////////////
//
// Module Initialisation/Finalisation
//
///////////////////////////////////////////////////////////////////////////////
int init_module(void)
{

  printk("\nrtai_utils loaded.\n");
  return 0;

} // End function - init_module

//-----------------------------------------------------------------------------
void cleanup_module(void)
{
  printk("\nrtai_utils unloaded.\n");

} // End function - cleanup_module

// ---------------------------------< eof >------------------------------------
