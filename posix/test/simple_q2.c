//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Trevor Woolven (trevw@zentropix.com)
// Original date:       Thu 15 Jul 1999
// Id:                  @(#)$Id: simple_q2.c,v 1.1.1.1 2004/06/06 14:03:04 rpm Exp $
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
const int PARENT_PRIORITY = (RT_LOWEST_PRIORITY - 2);

//Declare and initialise global queue Id
mqd_t my_q = INVALID_PQUEUE;

//-----------------------------------------------------------------------------
volatile int zdbg = 0;

void parent_func(int arg) {
int i;
int my_oflags, n = 0;
mode_t my_mode;
struct mq_attr my_attrs;
char inBuf[50];
uint priority = 0;

static const char str1[23] = "Test string number one\0";
static const char str2[28] = "This is test string num 2!!\0";
static const char str3[22] = "Test string number 3!\0";
static const char str4[34] = "This is test string number four!!\0";
 
struct Msg {
    const char *str;
    int str_size;
    uint prio;
};

uint nMsgs = 4;
static const struct Msg myMsgs[4] =
{
    {str1, 23, 5},
    {str2, 28, 1},
    {str3, 22, 6},
    {str4, 34, 7},
};

#ifdef STEP_TRACE
  //Wait for the debugger to say GO!
  while(!zdbg) {
	rt_task_wait_period();
  }
#endif

  rt_printk("Starting parent task %d\n", arg);
  for(i = 0; i < 5; i++) {
    rt_printk("parent task %d, loop count %d\n", arg, i);
  }
  //Create and open a queue
  my_oflags = O_RDWR | O_CREAT | O_NONBLOCK;
  my_mode = S_IRUSR | S_IWUSR | S_IRGRP;
  my_attrs.mq_maxmsg = 10;
  my_attrs.mq_msgsize = 50;
  my_q = mq_open("my_queue", my_oflags, my_mode, &my_attrs);

  if (my_q > 0) {
    rt_printk("Putting %d messages on queue\n", nMsgs);

    for (i = 0; i < nMsgs; i++) {

      //Put messages on the queue
      rt_printk("Sending: %s\n", myMsgs[i].str);
      n = mq_send(my_q, myMsgs[i].str, myMsgs[i].str_size, myMsgs[i].prio); 
      rt_printk("Parent put message 1 of %d bytes on queue at priority %d\n",
					n, myMsgs[i].prio);
    }
  } else {
      rt_printk("ERROR: could not create my_queue\n");
  }

  //Now receive stuff back from the queue
  for(i = 0; i < nMsgs; i++) {
    n = mq_receive(my_q, inBuf, sizeof(inBuf), &priority);  
    if (n > 0) {
      inBuf[n] = '\0';
      rt_printk("Parent got >%s< from queue\n", inBuf); 
    }
  }
  mq_close(my_q);
  mq_unlink("my_queue");
  free_z_apps(rt_whoami());
  rt_task_delete(rt_whoami());

}

// Ugly hack to work around RTAI build problems
#include "mod_stuff.c"

//------------------------------------eof---------------------------------------
