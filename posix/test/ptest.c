//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@zentropix.com)
// Original date:       Thu 15 Jul 1999
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



void *thread_func(void *arg) {

  int i;
  int r_c;
  int my_policy;
  pthread_t self;
  struct sched_param my_param;


  rt_printk("Starting pthread: %s, id: %ld\n", (char *)arg, pthread_self());

  self = pthread_self();

  if(( r_c = pthread_getschedparam(self, &my_policy, &my_param)) != 0) {
    rt_printk("pthread: %s, pthread_getschedparam error: %d\n", (char *)arg, r_c);
  }

  rt_printk("pthread: %s, running at %s/%d\n", (char *)arg,
          (my_policy == SCHED_FIFO ? "FIFO"
          : (my_policy == SCHED_RR ? "RR"
          : (my_policy == SCHED_OTHER ? "OTHER"
          : "unknown"))),
          my_param.sched_priority);


  rt_printk("pthread: %s, Increasing priority...\n", (char *)arg);

  my_param.sched_priority = 20;
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


  for(i = 0; i < 5; i++) {

    rt_printk("pthread %s, loop count %d\n", (char *)arg, i);

  }

  pthread_exit("a");

  return(NULL);

}



int init_module(void) {

  int r_c;
  pthread_t thread_id = 0;

  printk("pthread test program.\n");

  if(( r_c = pthread_create(&thread_id, NULL, thread_func, "a")) != 0 ) {

    printk("ptest: Thread creation failed - %d\n", r_c);
    return(-1);

  }

  printk("ptest: Created pthread, id = %ld\n", thread_id);

  return(0);

}


void cleanup_module(void) {

  printk("ptest module removed.\n");

}
