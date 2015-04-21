//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 2001 Steve Papacharalambous, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@lineo.com)
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
// pthreads mutex test program.
//
// This program tests the mutex operations.
// At module initialisation a mutex is created and locked.  A low priority control
// thread is created (SCHED_OTHER/0) which tries to lock the mutex.  NUM_THREADS threads are then
// created with increrasing priorities (SCHED_FIFO) which also try to gain the mutex.
//  The mutex queue is displayed to verify that the tasks are queued correctly and
// that tasks are inserted into the mutex queue in order of increasing priority.
// The initialisation function then releases the mutex and the threads insert data
// into a data buffer using the mutex to gain atomic access.  The contents of the
// data buffer are displayed when the test module is removed to verify that the data
// has been  inserted into the buffer is the expected order.
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



  rt_printk("Starting pthread: %s, id: %ld\n", (char *)arg, self);

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

  int r_c, i;
  int my_policy;
  pthread_t self, thread_id;
  pthread_attr_t new_attr;
  struct sched_param my_param;
  struct rt_queue *mutex_q;


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

// Print the contents of the mutex queue.
  rt_printk("\nmutextest - Contents of the mutex queue.....\n");
  rt_printk("-------------------<Start>--------------------\n");
  mutex_q = &(test_op.lock.m_semaphore.queue);
  while((mutex_q = mutex_q->next) != &(test_op.lock.m_semaphore.queue)) {

    rt_printk("Priority: %d, Base priority: %d\n",(mutex_q->task)->base_priority , (mutex_q->task)->priority);

  }

  rt_printk("--------------------<End>---------------------\n\n");


// Create the application thread(s) attributes.
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

      rt_printk("mutex test - Application thread creation failed:  %d\n", r_c);
    }

  }

  if(( r_c = pthread_mutex_unlock(&test_op.lock)) != 0) {
    rt_printk("pthread %s - Error unlocking mutex: %d\n", (char *)arg, r_c);
  }

  pthread_exit("b");

  return(NULL);

}


int init_module(void) {

  int r_c;
  pthread_t thread_id;

  printk("\nmutex test program.\n");

// Create a control pthread.
  if(( r_c = pthread_create(&thread_id, NULL, control_func, "control pthread mutex tester")) != 0 ) {

    printk("mutextest - Control thread creation failed:  %d\n", r_c);
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
