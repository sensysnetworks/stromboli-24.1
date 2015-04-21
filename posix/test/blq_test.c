//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Trevor Woolven (trevw@zentropix.com)
// Original date:       Thu 15 Jul 1999
// Id:                  @(#)$Id: blq_test.c,v 1.1.1.1 2004/06/06 14:03:03 rpm Exp $
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
#include "mod_stuff.h"

char *test_name = __FILE__;

//Declare and initialise task structure
RT_TASK parent_task = {0};
static RT_TASK child_task = {0};

const int PARENT_PRIORITY = (RT_LOWEST_PRIORITY - 2);
const int CHILD_PRIORITY  = (RT_LOWEST_PRIORITY - 1);

//Declare and initialise global queue Id
mqd_t my_q1 = INVALID_PQUEUE;
mqd_t my_q2 = INVALID_PQUEUE;


//-----------------------------------------------------------------------------
void child_func(int arg) {

int i;
size_t n = 0;
char inBuf[50];
uint priority = 0;
int nMsgs = 0;
struct mq_attr my_attrs ={0,0,0,0};
mode_t my_mode = 0;
int my_oflags = (O_RDWR | O_NONBLOCK);
mqd_t rx_q = INVALID_PQUEUE;
mqd_t tx_q = INVALID_PQUEUE;


  rt_printk("Starting child task %d\n", arg);
  for(i = 0; i < 3; i++) {

    rt_printk("child task %d, loop count %d\n", arg, i);

  }
  //Open a queue for reading and one for writing
  rx_q = mq_open("my_queue1", my_oflags, my_mode, &my_attrs);
  tx_q = mq_open("my_queue2", my_oflags, my_mode, &my_attrs);
  if (rx_q <= 0) {
    rt_printk("ERROR: child cannot open my_queue\n");
  } else {

    //Get the message(s) off the pQueue
    n = mq_getattr(rx_q, &my_attrs);
    nMsgs = my_attrs.mq_curmsgs;
    rt_printk("There are %d messages on the queue\n", nMsgs);

    while(nMsgs-- > 0) {
      n = mq_receive(rx_q, inBuf, sizeof(inBuf), &priority);
      inBuf[n] = '\0';

      //Report what happened
      rt_printk("Child got >%s<, %d bytes at priority %d\n", inBuf,n, priority);
    }
  }

  //Now write 'finished' back to parent
  if (tx_q > 0) {
    n = mq_send(tx_q, "queue open ok", 13, 1);
  }
  else {
    rt_printk("Child cannot reply, queue open failed\n");
  }
  mq_close(rx_q);
  mq_close(tx_q);
  free_z_apps(rt_whoami());
  rt_task_delete(rt_whoami());

}

//-----------------------------------------------------------------------------
volatile int zdbg = 0;

void parent_func(int arg) {
int i;
int my_oflags = 0;
int n = 0;
struct mq_attr my_attrs ={0,0,0,0};
mode_t my_mode = 0;
char inBuf[50];
uint priority = 0;

struct Msg {
    const char *str;
    int str_size;
    uint prio;
};

static const char str1[23] = "Test string number one\0";
static const char str2[28] = "This is test string num 2!!\0";
static const char str3[22] = "Test string number 3!\0";
static const char str4[34] = "This is test string number four!!\0";

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
  //Create my queue
  my_oflags = (O_RDWR | O_CREAT) ; //| O_NONBLOCK);
  my_mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  my_attrs.mq_maxmsg = 10;
  my_attrs.mq_msgsize = 50;
  
  //Create a pQueue for comms with child
  my_q1 = mq_open("my_queue1", my_oflags, my_mode, &my_attrs);
  my_q2 = mq_open("my_queue2", my_oflags, my_mode, &my_attrs);

  if (my_q1 > 0) {
    rt_printk("Putting %d messages on queue\n", nMsgs);

    for (i = 0; i < nMsgs; i++) {

      //Put messages on the queue
      rt_printk("Sending: %s\n", myMsgs[i].str);
      n = mq_send(my_q1, myMsgs[i].str, myMsgs[i].str_size, myMsgs[i].prio); 
      rt_printk("Parent put message 1 of %d bytes on queue at priority %d\n",
					n, myMsgs[i].prio);
    }
  } else {
      rt_printk("ERROR!, could not create my_queue\n");
  }

  mq_close(my_q1);

  rt_task_init(&child_task, child_func, 2, STACK_SIZE, CHILD_PRIORITY, 1, 0);
  rt_set_runnable_on_cpus(&child_task, RUN_ON_CPUS);
  rt_task_resume(&child_task);

  //Now wait for a message back when the child task has finished
  //the idea is that the task will block on this queue being empty, until
  //such time as the child task puts a message on the queue
  //Oh well, we live in hope.......... 
  n = mq_receive(my_q2, inBuf, sizeof(inBuf), &priority);
  if (n > 0) {
    rt_printk("Parent received %s from child, now winding up...\n", inBuf);
  }
  else {
    rt_printk("Error back from queue2 = %d\n", n);
  }

  mq_close(my_q2);
  mq_unlink("my_queue1");
  mq_unlink("my_queue2");

  rt_task_delete(rt_whoami());

}

// Ugly hack to work around RTAI build problems
#include "mod_stuff.c"

//------------------------------------eof---------------------------------------
