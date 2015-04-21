/*
COPYRIGHT (C) 2001  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#include <linux/module.h>
#include <linux/completion.h>
#include <linux/kernel_stat.h>

#define INTERFACE_TO_LINUX
#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>

#include "lock_task.h"

static RT_TASK idle_task;

volatile int rtai_cpu;

static volatile int was_used, nirqs, irqs[MAXIRQS];

extern struct kernel_stat kstat;

static inline int count_now(int cpu)
{
	int irq, count;
	count = 0;
	for (irq = 0; irq < nirqs; irq++) {
		count += kstat.irqs[cpu][irqs[irq]];
	}
	return count;
}

static void idle_fun(int t)
{
#ifdef CHECK
	int count, last_cnt;
	count = count_now(rtai_cpu);
	while(1) {
		if (count != (last_cnt = count_now(rtai_cpu))) {
			was_used += (last_cnt - count);
			count = last_cnt;
		}
	}
#else
	while (1);
#endif
}

int rt_lock_cpu(int cpu, int irqsl[], int nirqsl)
{
	int irq;
	if (cpu >= smp_num_cpus || cpu < 0) {
		return -EINVAL;
	}
	if (!idle_task.magic) {
		rt_task_init_cpuid(&idle_task, idle_fun, 0, STACK_SIZE, RT_LINUX_PRIORITY - 1, 0, 0, cpu);
	}
	rtai_cpu = cpu;
	if (irqsl && nirqsl < MAXIRQS && nirqsl > 0) {
		memcpy(&irqs, irqsl, (nirqs = nirqsl)*sizeof(int));
	} else if (!nirqs) {
		return -EINVAL;
	}
	for (irq = 0; irq < nirqs; irq++) {
		rt_assign_irq_to_cpu(irqs[irq], (1 << (1 - cpu)));
	}	
	return 0;
}

void rt_unlock_cpu(void)
{
	int irq;
	for (irq = 0; irq < nirqs; irq++) {
		rt_reset_irq_to_sym_mode(irqs[irq]);
	}
	rt_task_delete(&idle_task);
	idle_task.magic = 0;
}

static DECLARE_COMPLETION(lock_thread_exited);

static int lock_thread(void *null)
{
	int cpu;
	sprintf(current->comm, "RTAI_LOCK");
	current->nice = -20;
	cpu = -1;
	do {
		if (cpu != rtai_cpu) {
			current->cpus_allowed = 1 << (cpu = rtai_cpu);
			while (rtai_cpu != hard_cpu_id()) {
		        	current->state = TASK_INTERRUPTIBLE;
	        		schedule_timeout(2);
			}
		}
	} while (!signal_pending(current));
	return 0;
	complete_and_exit(&lock_thread_exited, 0);
}

static int lock_thread_pid;

int init_module(void)
{
	int irqs[] = IRQs;
	if (smp_num_cpus < 2) {
		printk("*** YOU CANNOT LOCK THE ONLY AVAILABLE CPU ***\n");
		return 1;
	}
	rt_lock_cpu(RT_CPU, irqs, sizeof(irqs)/sizeof(int));
	if ((lock_thread_pid = kernel_thread(lock_thread, NULL, 0/*CLONE_FS | CLONE_FILES | CLONE_SIGHAND*/)) < 0) {
		printk("*** CANNOT CREATE KERNEL LOCK THREAD ***\n");
		return 1;
	}
        return 0;
}

void cleanup_module(void)
{
        if (was_used) {
                printk("\nAHAH IT SEEMS IT WAS USED BY LINUX (%d), BAD.", was_used);
	}
	rt_unlock_cpu();
	kill_proc(lock_thread_pid, SIGTERM, 1);
//	wait_for_completion(&lock_thread_exited);
	return;
}
