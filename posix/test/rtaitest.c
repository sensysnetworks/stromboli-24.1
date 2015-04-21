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

#define STACK_SIZE 3000
#ifdef T_486
#define TICK_PERIOD             (1000000000/1000)       // 1 millisecond
#else
#define TICK_PERIOD 100000
#endif

RT_TASK parent_task, child_task;


void child_func(int arg) {

  int i;

  rt_printk("Starting child pthread %d\n", arg);
  for(i = 0; i < 3; i++) {

    rt_printk("child pthread %d, loop count %d\n", arg, i);

  }

  rt_task_delete(rt_whoami());

}



void parent_func(int arg) {

  int i;

  rt_printk("Starting parent pthread %d\n", arg);
  for(i = 0; i < 5; i++) {

    rt_printk("parent pthread %d, loop count %d\n", arg, i);

  }

  rt_task_init(&child_task, child_func, 2, STACK_SIZE, RT_LOWEST_PRIORITY - 1, 1, 0);
  rt_set_runnable_on_cpus(&child_task, RUN_ON_CPUS);
  rt_task_resume(&child_task);
  rt_task_delete(rt_whoami());

}



int init_module(void) {

  RTIME now, tick_period;

  printk("rtaitest test program, tick_period = %d.\n",TICK_PERIOD);

  rt_task_init(&parent_task, parent_func, 1, STACK_SIZE, RT_LOWEST_PRIORITY - 1, 1, 0);
  rt_set_runnable_on_cpus(&parent_task, RUN_ON_CPUS);
  tick_period = start_rt_timer((int)nano2count(TICK_PERIOD));
  rt_assign_irq_to_cpu(TIMER_8254_IRQ, TIMER_TO_CPU);
  rt_linux_use_fpu(1);
  now = rt_get_time();
  rt_task_resume(&parent_task);
  return 0;

}


void cleanup_module(void) {

  rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
  stop_rt_timer();
  printk("rtaitest module removed.\n");

}
