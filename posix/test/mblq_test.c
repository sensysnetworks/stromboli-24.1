//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 2000 Zentropic Computing, All rights reserved
//
// Authors:             Ian Soanes
// Original date:       Fri 8 Jul 2000
// Id:                  @(#)$Id: mblq_test.c,v 1.1.1.1 2004/06/06 14:03:03 rpm Exp $
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
#define ZDEBUG
#include "rtai_pqueue.h"
#include "mod_stuff.h"

int rt_printk(const char *, ...);

char *test_name = __FILE__;

//Declare and initialise task structures
#define NUM_CHILDREN 10
RT_TASK parent_task = {0};
static RT_TASK child_task[NUM_CHILDREN] = {{0}};

const int PARENT_PRIORITY = (RT_LOWEST_PRIORITY - 1);
const int CHILD_PRIORITY  = (RT_LOWEST_PRIORITY - 2);

//Declare and initialise global queue Id
mqd_t my_q1 = INVALID_PQUEUE;
mqd_t my_q2 = INVALID_PQUEUE;


//-----------------------------------------------------------------------------
void child_func(int arg) {

int i;
size_t n = 0;
char inBuf[50];
uint priority = 0;
struct mq_attr my_attrs ={0,0,0,0};
mode_t my_mode = 0;
int my_oflags = O_RDWR; // | O_NONBLOCK;
mqd_t rx_q = INVALID_PQUEUE;
mqd_t tx_q = INVALID_PQUEUE;
char msg[50];

  rt_printk("Starting child task %d\n", arg);
  strcpy(msg, "Child X got ");
  for(i = 0; i < 3; i++) {
    rt_printk("child task %d, loop count %d\n", arg, i);
  }
  
  //Open a queue for reading and one for writing
  rx_q = mq_open("my_queue1", my_oflags, my_mode, &my_attrs);
  tx_q = mq_open("my_queue2", my_oflags, my_mode, &my_attrs);
  if (rx_q <= 0) {
    rt_printk("ERROR: child cannot open queues\n");
  } else {
    //Get a message off the pQueue
    n = mq_receive(rx_q, inBuf, sizeof(inBuf), &priority);
    rt_printk( "Child %d got >%s<, %d bytes at priority %d\n", 
     	    	arg, inBuf, n, priority
     	    	);
  }

  //Now write finished messageback to parent
  if (tx_q > 0) {
    msg[6] = arg + '0';
    memcpy(&msg[12], inBuf, n);
    n = mq_send(tx_q, msg, n + 12, priority);
  }
  else {
    rt_printk("Child cannot reply, queue open failed\n");
  }
  mq_close(rx_q);
  mq_close(tx_q);
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

static const char str1[26]  = "01:Test string number one";
static const char str2[31]  = "02:This is test string num 2!!";
static const char str3[25]  = "03:Test string number 3!";
static const char str4[37]  = "04:This is test string number four!!";
static const char str5[20]  = "05:Message number 5";
static const char str6[23]  = "06:This is message # 6";
static const char str7[33]  = "07:This is the seventh message!!";
static const char str8[12]  = "08:Message8";
static const char str9[17]  = "09:Message num 9";
static const char str10[15] = "10:Message 10!";

static const struct Msg myMsgs[NUM_CHILDREN] =
{
    {str1,  26, 5},
    {str2,  31, 4},
    {str3,  25, 3},
    {str4,  37, 2},
    {str5,  20, 1},
    {str6,  23, 5},
    {str7,  33, 6},
    {str8,  12, 7},
    {str9,  17, 8},
    {str10, 15, 9},
};

#ifdef STEP_TRACE 
  //Wait for the debugger to say GO!
  while(!zdbg) {
	rt_task_wait_period();
  }
#endif

  //Create my queue
  rt_printk("Starting parent task %d\n", arg);
  for(i = 0; i < 5; i++) {
    rt_printk("parent task %d, loop count %d\n", arg, i);
  }
  my_oflags = (O_RDWR | O_CREAT) ; //| O_NONBLOCK);
  my_mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  my_attrs.mq_maxmsg = 10;
  my_attrs.mq_msgsize = 50;
  
  //Create a pQueue for comms with child
  my_q1 = mq_open("my_queue1", my_oflags, my_mode, &my_attrs);
  my_q2 = mq_open("my_queue2", my_oflags, my_mode, &my_attrs);
  if (my_q1 <= 0 || my_q2 <= 0) {
    rt_printk("Error: could not create queues\n");
  }

  //Fire up the children and let them block for their messages
  for (i = 0; i < NUM_CHILDREN; i++) {
    rt_task_init(&child_task[i], child_func, i, STACK_SIZE, CHILD_PRIORITY,1,0);
    rt_set_runnable_on_cpus(&child_task[i], RUN_ON_CPUS);
    rt_task_resume(&child_task[i]);
  }

  //Put a message for each child on the queue (they each expect to receive 1)
  rt_printk("Putting %d messages on queue\n", NUM_CHILDREN);
  for (i = 0; i < NUM_CHILDREN; i++) {
    rt_printk("Sending: %s\n", myMsgs[i].str);
    n = mq_send(my_q1, myMsgs[i].str, myMsgs[i].str_size, myMsgs[i].prio); 
    rt_printk( "Parent put message %d of %d bytes on queue at priority %d\n",
	    i, n, myMsgs[i].prio
	    );
  }
  mq_close(my_q1);

  //Now wait for a message back from each child task to confirm it's finished
  for (i = 0; i < NUM_CHILDREN; i++) {
    n = mq_receive(my_q2, inBuf, sizeof(inBuf), &priority);
    if (n > 0) {
      rt_printk( "Parent received <%s> %d bytes from a child at priority %d\n", 
	         inBuf, n, priority
		 );
    }
    else {
      rt_printk("Error back from queue2 = %d\n", n);
    }
  }

  //Tidy up and finish
  mq_close(my_q2);
  mq_unlink("my_queue1");
  mq_unlink("my_queue2");
  rt_task_delete(rt_whoami());
}

// Ugly hack to work around RTAI build problems
#include "mod_stuff.c"
