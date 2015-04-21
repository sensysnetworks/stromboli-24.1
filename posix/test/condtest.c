//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 2001 Steve Papacharalambous, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@lineo.com)
// Original date:       Tue 03 Aug 1999
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
// pthreads conditional variables test program.
//
// This program tests conditional variables operations.
//
// A control thread and 2 waiting threads are created.  The waiting threads
// each wait on a conditional variable being set to a pre-determined value.
// The control thread set the condition variable to the first value and signals
// the 1st waiting thread which sets the conditional variable to the next value
// and signals the next waiting thread.
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



struct test_condvar {
  int t_value;
  pthread_cond_t t_cond;
  pthread_mutex_t t_lock;
};

static struct test_condvar t_cond_data = {
    0, PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};


void *wait_func1(void *arg) {

  int r_c;
  pthread_t self;

  self = pthread_self();

  rt_printk("Starting: %s, id: %ld\n", (char *)arg, self);

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

// DEBUG
rt_printk("%s - Returned from pthread_cond_wait okay.\n", (char *)arg);
// END DEBUG

    if(t_cond_data.t_value == 1) {
      rt_printk("%s id: %ld, Conditional variable value: %d\n",
                          (char *)arg, self, t_cond_data.t_value);
    }
  }

// Signal the next waiting task.
  t_cond_data.t_value = 2;

  if((r_c = pthread_cond_signal(&t_cond_data.t_cond)) != 0 ) {
    rt_printk("%s, pthread_cond_signal error: %d\n", (char *)arg, r_c);
  }

  if(( r_c = pthread_mutex_unlock(&t_cond_data.t_lock)) != 0) {
    rt_printk("%s - Error unlocking mutex: %d\n", (char *)arg, r_c);
  }

  pthread_exit("b");

  return(NULL);

}  // End function - wait_func1


void *wait_func2(void *arg) {

  int r_c;
  pthread_t self;

  self = pthread_self();

  rt_printk("Starting: %s, id: %ld\n", (char *)arg, self);

// Lock the mutex and wait on the condition variable.
  if(( r_c = pthread_mutex_lock(&t_cond_data.t_lock)) != 0) {
    rt_printk("%s, Cannot gain mutex: %d\n", (char *)arg, r_c);
    pthread_exit("b");
  }

  while( t_cond_data.t_value < 1 ) {
    if((r_c = pthread_cond_wait(&t_cond_data.t_cond, &t_cond_data.t_lock)) != 0 ) {
      rt_printk("%s id: %ld, pthread_cond_wait error: %d\n", (char *)arg, self, r_c);
      if(( r_c = pthread_mutex_unlock(&t_cond_data.t_lock)) != 0) {
        rt_printk("%s - Error unlocking mutex: %d\n", (char *)arg, r_c);
      }
      pthread_exit("b");
    }

    if(t_cond_data.t_value == 2) {
      rt_printk("%s id: %ld, Conditional variable value: %d\n",
                          (char *)arg, self, t_cond_data.t_value);
    }
  }
  if(( r_c = pthread_mutex_unlock(&t_cond_data.t_lock)) != 0) {
    rt_printk("%s - Error unlocking mutex: %d\n", (char *)arg, r_c);
  }

  pthread_exit("c");

  return(NULL);

}  // End function - wait_func2


void *control_func(void *arg) {

  int r_c;
  pthread_t self;

  self = pthread_self();

  rt_printk("%s, id: %ld\n", (char *)arg, self);

// Lock the mutex, set the variable, and signal the condition variable.
  if(( r_c = pthread_mutex_lock(&t_cond_data.t_lock)) != 0) {
    rt_printk("%s, Cannot gain mutex: %d\n", (char *)arg, r_c);
  }
  t_cond_data.t_value = 1;

  if((r_c = pthread_cond_signal(&t_cond_data.t_cond)) != 0 ) {
    rt_printk("%s, pthread_cond_signal error: %d\n", (char *)arg, r_c);
  }
  if(( r_c = pthread_mutex_unlock(&t_cond_data.t_lock)) != 0) {
    rt_printk("%s - Error unlocking mutex: %d\n", (char *)arg, r_c);
  }

  pthread_exit("a");

  return(NULL);

}  // End function - control_func


int init_module(void) {

  int r_c;
  pthread_t wait_id1 = 0;
  pthread_t wait_id2 = 0;
  pthread_t control_id = 0;

  printk("\nConditional variable test program.\n");

// Create the wait pthread(s).
  if(( r_c = pthread_create(&wait_id1, NULL, wait_func1, "wait thread 1")) != 0 ) {

    printk("condvar test - Wait thread #1 creation failed:  %d\n", r_c);
    return(-1);
  }

  if(( r_c = pthread_create(&wait_id2, NULL, wait_func2, "wait thread 2")) != 0 ) {

    printk("condvar test - Wait thread #2 creation failed:  %d\n", r_c);
    return(-1);
  }


// Create the control pthread.
  if(( r_c = pthread_create(&control_id, NULL, control_func, "control thread")) != 0 ) {

    printk("condvar test - Control thread creation failed:  %d\n", r_c);
    return(-1);
  }

  return(0);

}  // End function - init_module


void cleanup_module(void) {

  printk("Conditional variable test program removed.\n");

}  // End function - cleanup_module
