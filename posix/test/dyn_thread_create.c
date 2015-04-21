//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@zentropix.com)
// Original date:       Sun 29 Aug 1999
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
// pthreads dynamic thread creation test program.
//
// This program tests dynamic thread creation.
// NUM_THREADS wait threads are created which all wait on a conditional variable.  A
// control thread sets the conditional variable and then broadcasts to all the threads
// waiting on the conditional variable.  This is repeated NUM_LOOP times to check that
// the dynamic thread creation mechanism is working.
//


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>

#include <rtai_fifos.h>
#include <rtai_pthread.h>



#define RUNNABLE_ON_CPUS 3
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)

#define TIMER_TO_CPU 3             // < 0 || > 1 to maintain a symmetrically processed timer

#define RT_STACK 3000

#define DEFAULT_TICK_PERIOD 100000

#define NUM_THREADS 40
#define NUM_LOOPS 100
#define MAX_YIELD_LOOPS 200

struct test_condvar {
  int t_value;
  pthread_cond_t t_cond;
  pthread_mutex_t t_lock;
};

static struct test_condvar t_cond_data = {
    0, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};

struct thread_stat {
  int created;
  int terminated;
};

static struct thread_stat test_stat = {0, 0};

static int num_wait_threads = 0;  // Only modify when own the mutex.
static pthread_t control_id = 0;
static pthread_t wait_id[NUM_THREADS] = {0};


void *wait_func(void *arg) {

  int r_c;
  pthread_t self;
  struct sched_param my_param;

  self = pthread_self();

  test_stat.created++;

// Increase thread scheduling priority.
  my_param.sched_priority = 50;
  if(( r_c = pthread_setschedparam(self, SCHED_FIFO, &my_param)) != 0) {
    rt_printk("%s, id: %ld pthread_setschedparam error: %d\n", (char *)arg, self, r_c);
  }

// Lock the mutex and wait on the condition variable.
  if(( r_c = pthread_mutex_lock(&t_cond_data.t_lock)) != 0) {
    rt_printk("%s, Cannot gain mutex: %d\n", (char *)arg, r_c);
    pthread_exit("b");
  }

  while( t_cond_data.t_value == 0 ) {
    if((r_c = pthread_cond_wait(&t_cond_data.t_cond, &t_cond_data.t_lock)) != 0 ) {
      rt_printk("%s id: %ld, pthread_cond_wait error: %d\n", (char *)arg, self, r_c);
      if(( r_c = pthread_mutex_unlock(&t_cond_data.t_lock)) != 0) {
        rt_printk("%s - Error unlocking mutex: %d\n", (char *)arg, r_c);
      }
      pthread_exit("b");
    }

  }

  num_wait_threads--;
  if(( r_c = pthread_mutex_unlock(&t_cond_data.t_lock)) != 0) {
    rt_printk("%s - Error unlocking mutex: %d\n", (char *)arg, r_c);
  }

//  rt_printk("%s - tid %ld, Woke up from condition broadcast.\n", (char *)arg, self);

  test_stat.terminated++;
  pthread_exit("b");

  return(NULL);

}  // End function - wait_func


void *control_func(void *arg) {

  int r_c, i, j;
  int abort_count;
  pthread_t self;

  self = pthread_self();

  rt_printk("%s, id: %ld\n", (char *)arg, self);

// Main test loop.
  for(j = 0; j < NUM_LOOPS; j++) {

// Lock the mutex, set the variable, and signal the condition variable.
    if(( r_c = pthread_mutex_lock(&t_cond_data.t_lock)) != 0) {
      rt_printk("%s, Cannot gain mutex: %d\n", (char *)arg, r_c);
    }
    t_cond_data.t_value = 1;
    num_wait_threads = 0;    // Just in case the abort count expired :-?

// Create the wait pthread(s).
    for(i = 0; i < NUM_THREADS; i++) {
      if(( r_c = pthread_create(&wait_id[i], NULL, wait_func, "wait thread")) != 0 ) {
        rt_printk("thread create test - Wait thread creation failed:  %d\n", r_c);
      }

      num_wait_threads++;
    }  // End for loop - create wait threads loop.

//    rt_printk("%s - Number of waiting threads: %d\n", (char *)arg, num_wait_threads);

    if((r_c = pthread_cond_broadcast(&t_cond_data.t_cond)) != 0 ) {
      rt_printk("%s, pthread_cond_signal error: %d\n", (char *)arg, r_c);
    }
    if(( r_c = pthread_mutex_unlock(&t_cond_data.t_lock)) != 0) {
      rt_printk("%s - Error unlocking mutex: %d\n", (char *)arg, r_c);
    }

// Wait until all wait tasks have completed.
    abort_count = MAX_YIELD_LOOPS;
    while(num_wait_threads != 0 && abort_count != 0) {
      rt_task_yield();  // Hmm... Not exactly a POSIX call, ugly.
      abort_count--;
    }

    if(abort_count <= 0) {
      rt_printk("%s - Loop abort count expired\n", (char *)arg);
      rt_printk("%s - Wait threads left: %d\n", (char *)arg, num_wait_threads);
    }

  }  // End for loop - main test loop

  rt_printk("%s - no of threads created: %d, no of threads terminated: %d\n", (char *)arg,
                                              test_stat.created, test_stat.terminated);

  pthread_exit("a");

  return(NULL);

}  // End function - control_func


int init_module(void) {

  int r_c;

  printk("\nDynamic thread creation test program.\n");

// Create the control pthread.
  if(( r_c = pthread_create(&control_id, NULL, control_func, "control thread")) != 0 ) {

    printk("thread create test - Control thread creation failed:  %d\n", r_c);
    return(-1);
  }

  return(0);

}  // End function - init_module


void cleanup_module(void) {

  printk("Dynamic thread creation test program removed.\n");

}  // End function - cleanup_module
