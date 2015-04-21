/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)
                    Stuart Hughes    (shughes@zentropix.com)
m68knommu contributed by Lineo Inc. (Author: Bernhard Kuhn (bkuhn@lineo.com)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/


#ifndef RTAI_SCHED_H
#define RTAI_SCHED_H

extern void up_task_sw(void *, void *);

#define rt_switch_to(new_task) up_task_sw(&rt_current, (new_task));

#define rt_exchange_tasks(oldtask, newtask) up_task_sw(&(oldtask), (new_task));

#define init_arch_stack() \
do { \
        *--(task->stack) = data;                \
        *--(task->stack) = (int) rt_thread;     \
        *--(task->stack) = 0;                   \
        *--(task->stack) = (int) rt_startup;    \
} while(0)

#define DEFINE_LINUX_CR0

#define DEFINE_LINUX_SMP_CR0

#define init_fp_env() \
do { \
	memset(&task->fpu_reg, 0, sizeof(task->fpu_reg)); \
}while(0)

static inline void *get_stack_pointer(void)
{
	void *sp;
	asm volatile ("movel %%sp,%0" : "=d" (sp));
	return sp;
}

#define RT_SET_RTAI_TRAP_HANDLER(x)

#define DO_TIMER_PROPER_OP()

#endif
