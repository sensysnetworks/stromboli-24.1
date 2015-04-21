#ifndef _RTAI_UTILS_H_
#define _RTAI_UTILS_H_
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
// Utility routines for Real Time Linux.
//
///////////////////////////////////////////////////////////////////////////////

static char id_rtai_utils_h[] __attribute__ ((unused)) = "@(#)$Id: rtai_utils.h,v 1.1.1.1 2004/06/06 14:03:03 rpm Exp $";

//--------------------------< Application Controls >---------------------------
// POSIX QUEUES //
// ============ //
#define MAX_PQUEUES     4       //Maximum number of message queues in module
#define MAX_MSGSIZE     50      //Maximum message size per queue (bytes)
#define MAX_MSGS        10      //Maximum number of messages per queue
#define MAX_BLOCKED_TASKS 10    //Maximum number of tasks blocked on a
                                //queue at any one time 
#define MSG_HDR_SIZE	16	//Note that this is hard-coded (urgh!) ensure 
				// it always matches pqueues sizeof(MSG_HDR)  
				// or do it a better way! (sic)

// a) A single Posix queue ( (MAX_MSGSIZE + sizeof(MSG_HDR) * MAX_MSGS) ) or 
// b) A blocked tasks queue (MAX_BLOCKED_TASKS * sizeof(MSG_HDR) ) or
// c) A Zentropix application data staging structure (sizeof(Z_APPS))
// 
// It is assumed that the first two are both bigger than a Z_APPS structure
// and so the choice is made between a) and b).
//
// Note that one control mechanism is used to allocate memory 'chunks' for a
// number of different application uses. This means that if the 'chunk' size
// becomes large in relation to the amount of memory required by one or other
// of these applications, memory usage becomes wasteful.
//

// ----------------------------------------------------------------------------
// Set of pointers to Application-Specific extensions to RTAI
// such as POSIX Threads, POSIX Queues, VxWorks Compatibility Library, etc
//
typedef struct z_apps {
    int in_use_count;	// Incremented whenever an application is initialised
    void *pthreads;
    void *pqueues;
    void *vxtasks;
			// anticipate... pclocks, psosTasks,
} Z_APPS;

// ----------------------------------------------------------------------------

extern void *init_z_apps(void *this_task);
extern void free_z_apps(void *this_task);

// ---------------------------------< eof >------------------------------------
#endif  // _RTAI_UTILS_H_
