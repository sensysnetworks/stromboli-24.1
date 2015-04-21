//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@zentropix.com)
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
// pthreads mutex priority inheritance test program.
//
// This program tests that the mutex priority inheritance is working correctly.  It
// creates a "control" thread with scheduling parameters of SCHED_OTHER/0 which
// initialises a mutex and then locks it.  The control thread then creates NUM_THREADS
// "application" threads with scheduling parameters of SCHED_FIFO/12+<thread no>.
// The "application" threads attempt to gain the mutex to fill a data buffer.  As the
// mutex is already locked they sleep on the mutex.  The priority of the "control"
// thread is raised to that of the highest "application" thread, and it unlocks the mutex
// allowing the "application" threads to proceed.  This program also shows that threads
// are inserted into the mutex queue in priority order.
// The thread priorities are displayed at relevant points to verify that the priorities
// are manipulated correctly.
//

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>

#include <rtai_pthread.h>



#define RUNNABLE_ON_CPUS 3
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)

#define TIMER_TO_CPU 3             // < 0 || > 1 to maintain a symmetrically processed timer

#define RT_STACK 3000

#define DEFAULT_TICK_PERIOD 100000

#define T_BUFFER_SIZE 256

#define NUM_THREADS 4

struct op_buffer {
  int t_buffer_data[T_BUFFER_SIZE];
  long t_buffer_tid[T_BUFFER_SIZE];
  int t_buffer_pos;
  pthread_mutex_t lock;
};

static struct op_buffer test_op;


void *thread_func(void *arg) {

  int i;
  int r_c;
  int my_policy;
  pthread_t self;
  struct sched_param my_param;
  RT_TASK *rtai_task_ptr;


  self = pthread_self();

  rt_printk("Starting pthread: %s, id: %ld\n", (char *)arg, self);

  if(( r_c = pthread_getschedparam(self, &my_policy, &my_param)) != 0) {
    rt_printk("pthread: %s, pthread_getschedparam error: %d\n", (char *)arg, r_c);
  }

  rt_printk("pthread: %s, Created with following scheduling parameters.\n", (char *)arg);
  rt_printk("pthread: %s, running at %s/%d\n", (char *)arg,
          (my_policy == SCHED_FIFO ? "FIFO"
          : (my_policy == SCHED_RR ? "RR"
          : (my_policy == SCHED_OTHER ? "OTHER"
          : "unknown"))),
          my_param.sched_priority);

  rtai_task_ptr = rt_whoami();
  rt_printk("pthread: %s, RTAI priority: %d\n", (char *)arg, rtai_task_ptr->priority);

  if(( r_c = pthread_mutex_lock(&test_op.lock)) != 0) {
    rt_printk("pthread %s, Cannot gain mutex: %d\n", (char *)arg, r_c);
  }


  for(i = 0; i < 5; i++) {

    rt_printk("pthread: %s, loop count %d\n", (char *)arg, i);
    test_op.t_buffer_data[test_op.t_buffer_pos] = self + i;
    test_op.t_buffer_tid[test_op.t_buffer_pos] = self;
    test_op.t_buffer_pos++;
  }

  if(( r_c = pthread_mutex_unlock(&test_op.lock)) != 0) {
    rt_printk("pthread %s - Error unlocking mutex: %d\n", (char *)arg, r_c);
  }

  pthread_exit("b");

  return(NULL);

}


void *control_func(void *arg) {

  int i;
  int r_c;
  int my_policy;
  pthread_t self;
  pthread_t thread_id = 0;
  pthread_attr_t new_attr;
  struct sched_param my_param;
  RT_TASK *rtai_task_ptr;


  self = pthread_self();

  rt_printk("Starting control pthread: %s, id: %ld\n", (char *)arg, self);

  if(( r_c = pthread_getschedparam(self, &my_policy, &my_param)) != 0) {
    rt_printk("pthread: %s, pthread_getschedparam error: %d\n", (char *)arg, r_c);
  }

  rt_printk("pthread: %s, Created with following scheduling parameters.\n", (char *)arg);
  rt_printk("pthread: %s, running at %s/%d\n", (char *)arg,
          (my_policy == SCHED_FIFO ? "FIFO"
          : (my_policy == SCHED_RR ? "RR"
          : (my_policy == SCHED_OTHER ? "OTHER"
          : "unknown"))),
          my_param.sched_priority);

  rtai_task_ptr = rt_whoami();
  rt_printk("pthread: %s, RTAI priority: %d\n", (char *)arg, rtai_task_ptr->priority);

// Initialise & lock the mutex: test_op
  pthread_mutex_init(&test_op.lock, NULL);
  test_op.t_buffer_pos = 0;
  for(i = 0; i < T_BUFFER_SIZE; i++) {
    test_op.t_buffer_data[i] = 0;
    test_op.t_buffer_tid[i] = 0;
  }

  if(( r_c = pthread_mutex_lock(&test_op.lock)) != 0) {
    rt_printk("pthread %s, Cannot gain mutex: %d\n", (char *)arg, r_c);
  }


// Create an application thread.
  if(( r_c = pthread_attr_init(&new_attr) ) != 0 ) {
    rt_printk("pthread %s, Error initialising thread attributes: %d\n", (char *)arg, r_c);
  }

  if(( r_c = pthread_attr_setschedpolicy(&new_attr, SCHED_FIFO) ) != 0 ) {
    rt_printk("pthread %s, Error setting thread attribute scheduling parameters: %d\n", (char *)arg, r_c);
  }


// Create NUM_THREADS application pthreads, each thread with increasing priority.
  for(i = 0; i < NUM_THREADS; i++) {

    my_param.sched_priority = 12 + i;
    if(( r_c = pthread_attr_setschedparam(&new_attr, &my_param) ) != 0 ) {
      rt_printk("pthread %s, Error setting thread attribute scheduling parameters: %d\n", (char *)arg, r_c);
    }

    if(( r_c = pthread_create(&thread_id, &new_attr, thread_func, "application pthread mutex tester")) != 0 ) {

      rt_printk("mutex priority inheritance test - Application thread creation failed:  %d\n", r_c);
    }

  }


// Display control thread priority.
  if(( r_c = pthread_getschedparam(self, &my_policy, &my_param)) != 0) {
    rt_printk("pthread: %s, pthread_getschedparam error: %d\n", (char *)arg, r_c);
  }


  rt_printk("pthread: %s, Priority should now be raised due to inheritance.\n", (char *)arg);
  rt_printk("pthread: %s, running at %s/%d\n", (char *)arg,
          (my_policy == SCHED_FIFO ? "FIFO"
          : (my_policy == SCHED_RR ? "RR"
          : (my_policy == SCHED_OTHER ? "OTHER"
          : "unknown"))),
          my_param.sched_priority);

  rt_printk("pthread: %s, RTAI priority: %d\n", (char *)arg, rtai_task_ptr->priority);


  rt_printk("pthread: %s, id: %ld, about to unlock mutex.\n", (char *)arg, self);

  if(( r_c = pthread_mutex_unlock(&test_op.lock)) != 0) {
    rt_printk("pthread %s - Error unlocking mutex: %d\n", (char *)arg, r_c);
  }

  rt_printk("pthread: %s, Priority should now be back to original.\n", (char *)arg);
  rt_printk("pthread: %s, running at %s/%d\n", (char *)arg,
          (my_policy == SCHED_FIFO ? "FIFO"
          : (my_policy == SCHED_RR ? "RR"
          : (my_policy == SCHED_OTHER ? "OTHER"
          : "unknown"))),
          my_param.sched_priority);

  rt_printk("pthread: %s, RTAI priority: %d\n", (char *)arg, rtai_task_ptr->priority);


  pthread_exit("a");

  return(NULL);

}


int init_module(void) {

  int r_c;
  pthread_t thread_id = 0;

  printk("\nmutex priority inheritance test program.\n");

// Create a control pthread.
  if(( r_c = pthread_create(&thread_id, NULL, control_func, "control pthread mutex tester")) != 0 ) {

    printk("mutex priority inheritance test - Control thread creation failed:  %d\n", r_c);
    return(-1);
  }

  return(0);

}


void cleanup_module(void) {

  int r_c;
  int i;

  printk("mutextest - Contents of mutex test_op buffer:\n");
  for(i = 0; i < test_op.t_buffer_pos; i++){
    printk("Position: %d, Value: %d, Thread id: %ld\n", i, test_op.t_buffer_data[i], test_op.t_buffer_tid[i]);
  }

  if(( r_c = pthread_mutex_destroy(&test_op.lock)) != 0) {
    printk("mutextest - Error removing mutex: %d\n", r_c);
  }

  printk("mutex test module removed.\n");

}
