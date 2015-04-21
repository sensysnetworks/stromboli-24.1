//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@lineo.com)
//			Trevor Woolven (trevw@lineo.com)
// Original date:       Sun 01 Aug 1999
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
// pthreads interface for Real Time Linux.
//
///////////////////////////////////////////////////////////////////////////////
//
// pthreads & pqueues test program.
//
// This tests cooperation between pthreads and pqueues. A control thread
// is created which creates two pqueues, puts a message on one and then blocks
// until a reply is received on the other. A child thread is created that 
// opens the pqueues and blocks until a message is received. When the message 
// arrives, it is printed and a reply posted back to the other queue. 
// The queues are then closed. 
// The control thread unlinks the pqueues once it receives it's reply from the
// child thread and everything dies gracefully.
//
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>

#include <rtai_pthread.h>
#include <rtai_pqueue.h>

#define RUNNABLE_ON_CPUS 3
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)

#define TIMER_TO_CPU 3             // < 0 || > 1 to maintain a symmetrically processed timer

#define RT_STACK 3000
#define DEFAULT_TICK_PERIOD 100000
#define T_BUFFER_SIZE 256

#define STR_SIZE 27
struct Msg {
    char str[STR_SIZE];
    int prio;
};

//-----------------------------------------------------------------------------
void *thread_func(void *arg) {

int r_c;
int my_policy;
pthread_t self;
struct sched_param my_param;

int my_oflags = O_RDWR;
mode_t my_mode = 0;
struct mq_attr my_attrs = {0,0,0,0};
char inBuf[50];
char reply[15] = "Reply message!!";
mqd_t rx_q = INVALID_PQUEUE;
mqd_t tx_q = INVALID_PQUEUE;
uint priority;
int n;

  self = pthread_self();

// Set thread scheduling parameters.
  my_param.sched_priority = 20 + (int)self;
  if(( r_c = pthread_setschedparam(self, SCHED_FIFO, &my_param)) != 0) {
    rt_printk("pthread: %s, pthread_setschedparam error: %d\n", (char *)arg, r_c);
  }

  if(( r_c = pthread_getschedparam(self, &my_policy, &my_param)) != 0) {
    rt_printk("pthread: %s, pthread_getschedparam error: %d\n", (char *)arg, r_c);
  }

  rt_printk("pthread: %s, running at %s/%d\n", (char *)arg,
          (my_policy == SCHED_FIFO ? "FIFO"
          : (my_policy == SCHED_RR ? "RR"
          : (my_policy == SCHED_OTHER ? "OTHER"
          : "unknown"))),
          my_param.sched_priority);

  rt_printk("Starting child pthread: %s, id: %ld\n", (char *)arg, self);

  // Open the rx pqueue 
  rx_q = mq_open("tx_queue", my_oflags, my_mode, &my_attrs);
  if(rx_q <= 0) {
    rt_printk("ERROR: child cannot open rx_queue\n");
  }
  else {
    // Get message(s)
    rt_printk("Child: waiting for message\n");
    n = mq_receive(rx_q, inBuf, sizeof(inBuf), &priority);
    inBuf[n] = '\0';
    rt_printk("Child: Message received = >%s< %d bytes at priority %d\n",
			inBuf, n, priority);
  }
  mq_close(rx_q);

  // Open the tx pqueue
  tx_q = mq_open("rx_queue", my_oflags, my_mode, &my_attrs);
  if(tx_q <= 0) {
    rt_printk("ERROR: child cannot open tx_queue\n");
  }
  else {
    n = mq_send(tx_q, reply, sizeof(reply), 1);
  }
  mq_close(tx_q);

  pthread_exit("b");

  return(NULL);

}

//-----------------------------------------------------------------------------
void *control_func(void *arg) {

int r_c;
int my_policy;
pthread_t self;
struct sched_param my_param;

int my_oflags = 0;
mode_t my_mode = 0;
struct mq_attr my_attrs = {0,0,0,0};
char inBuf[50];
char tx_msg[27] = "Simple test message to pass";
mqd_t tx_q = INVALID_PQUEUE;
mqd_t rx_q = INVALID_PQUEUE;
int n;
uint priority;

  self = pthread_self();

  rt_printk("Starting pthread: %s, id: %ld\n", (char *)arg, self);

  if(( r_c = pthread_getschedparam(self, &my_policy, &my_param)) != 0) {
    rt_printk("pthread: %s, pthread_getschedparam error: %d\n", (char *)arg, r_c);
  }

  rt_printk("pthread: %s, running at %s/%d\n", (char *)arg,
          (my_policy == SCHED_FIFO ? "FIFO"
          : (my_policy == SCHED_RR ? "RR"
          : (my_policy == SCHED_OTHER ? "OTHER"
          : "unknown"))),
          my_param.sched_priority);

  rt_printk("Starting control pthread: %s, id: %ld\n", (char *)arg, self);

  // Create the queues
  my_oflags = (O_WRONLY | O_CREAT);
  my_mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
  my_attrs.mq_maxmsg = 10;
  my_attrs.mq_msgsize = 50;
  tx_q = mq_open("tx_queue", my_oflags, my_mode, &my_attrs);
  my_oflags = (O_RDONLY | O_CREAT);
  rx_q = mq_open("rx_queue", my_oflags, my_mode, &my_attrs);

  if(tx_q > 0) {
    n = mq_send(tx_q, tx_msg, sizeof(tx_msg), 5);
    if(rx_q > 0) {
      n = mq_receive(rx_q, inBuf, sizeof(inBuf), &priority);
      inBuf[n] = '\0';
      rt_printk("Control: Message received = >%s< %d bytes at priority %d\n",
			inBuf, n, priority);
    }
    else {
      rt_printk("Control: cannot read from rx queue, invalid id %ld\n", rx_q);
    }
  }
  else {
    rt_printk("Control: cannot write to send queue, invalid id %ld\n", tx_q);
  } 
  mq_unlink("tx_queue");
  mq_unlink("rx_queue");

  pthread_exit("b");

  return(NULL);

}

//----------------------------------------------------------------------------
int init_module(void) 
{
int r_c;
pthread_t thread_id = 0;

  printk("\npThreads & pQueues test program.\n");

  // Create the control pthread.
  if(( r_c = pthread_create(&thread_id, NULL, control_func, "control pthread mutex tester")) != 0 ) {

    printk("mutextest - Control thread creation failed:  %d\n", r_c);
    return(-1);
  }

  // Create an application pthread.
  if(( r_c = pthread_create(&thread_id, NULL, thread_func, "application pthread mutex tester")) != 0 ) {

      printk("mutextest - Application thread creation failed:  %d\n", r_c);
      return(-1);
  }

  return(0);

}

//-----------------------------------------------------------------------------
void cleanup_module(void) 
{
  printk("pThreads & pQueues test module removed.\n");
}
