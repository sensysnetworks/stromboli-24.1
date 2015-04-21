//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Trevor Woolven (trevw@zentropix.com)
// Original date:       Thu 15 Jul 1999
// Id:                  @(#)$Id: vsimple_q.c,v 1.1.1.1 2004/06/06 14:03:04 rpm Exp $
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
// Examples of the pqueues interface for Real Time Linux.
//
///////////////////////////////////////////////////////////////////////////////
#include <rtai_pqueue.h>

char *test_name = __FILE__;

//Declare and initialise task structure
RT_TASK parent_task = {0};

//Declare and initialise global queue Id
mqd_t my_q = INVALID_PQUEUE;

const int PARENT_PRIORITY = (RT_LOWEST_PRIORITY - 2);

#define	STR_SIZE 27
struct Msg {
  char str[STR_SIZE];
  uint prio;
};

//-----------------------------------------------------------------------------
volatile int zdbg = 0;

void parent_func(int arg) {
int i;
int my_oflags, n = 0;
mode_t my_mode;
struct mq_attr my_attrs;
char inBuf[50];
uint priority = 0;
uint nMsgs = 2;

#ifdef STEP_TRACE
  //Wait for the debugger to say GO!
  while(zdbg == 0) {
	rt_task_wait_period();
  }
#endif

  rt_printk("Starting parent task %d\n", arg);
  for(i = 0; i < 5; i++) {
    rt_printk("parent task %d, loop count %d\n", arg, i);
  }
  //Create my queue
  my_oflags = O_RDWR | O_CREAT | O_NONBLOCK;
  my_mode = S_IRUSR | S_IWUSR | S_IRGRP;
  my_attrs.mq_maxmsg = 10;
  my_attrs.mq_msgsize = 50;

  //Create a pQueue for comms with child
  my_q = mq_open("my_queue", my_oflags, my_mode, &my_attrs);

  if (my_q > 0) {
    for(i = 0; i <  nMsgs ; i++) {
      //Send child a message
      n = mq_send(my_q, "Queue message", 13, 1); 

      //Report how much was sent
      rt_printk("Parent put message %d of %d bytes on queue at priority %d\n",
					i, n, 1);
    }
  } else {
      rt_printk("ERROR: could not create my_queue\n");
  }

  //Now receive stuff back from the queue
  n = mq_receive(my_q, inBuf, sizeof(inBuf), &priority);  
  if (n > 0) {
    inBuf[n] = '\0';
    rt_printk("Parent got >%s< from queue\n", inBuf); 
  }
  //Finally, close the queue (while the owner task is still alive)
  mq_close(my_q);
  mq_unlink("my_queue");

  free_z_apps(rt_whoami());
  rt_task_delete(rt_whoami());

}

// Ugly hack to work around RTAI build problems
#include "mod_stuff.c"

//------------------------------------eof---------------------------------------
