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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
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

static struct task_struct *lock_process;

int rt_lock_cpu(int cpu, int irqsl[], int nirqsl)
{
	int irq;
	if (cpu >= smp_num_cpus || cpu < 0) {
		return -EINVAL;
	}
	if (!idle_task.magic) {
		rt_task_init_cpuid(&idle_task, idle_fun, 0, STACK_SIZE, RT_LINUX_PRIORITY - 1, 0, 0, cpu);
	}
	if (lock_process && cpu != rtai_cpu) {
        	lock_process->cpus_allowed = 1 << cpu;
        	lock_process->state = TASK_INTERRUPTIBLE;
		while (lock_process->run_list.next) {
			schedule();
		}
        	wake_up_process(lock_process);
		while (!lock_process->run_list.next) {
			schedule();
		}
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

static void rtai_srq_handler(void) { }

static long long user_srq_handler(unsigned int noarg)
{
	lock_process          = current;
        current->nice = -20;
        current->cpus_allowed = 1 << rtai_cpu;
	while (rtai_cpu != hard_cpu_id()) {
        	current->state = TASK_INTERRUPTIBLE;
	        schedule_timeout(2);
	}
	return 0;
}

static volatile int lock_srq;

int init_module(void)
{
	int irqs[] = IRQs;
	if (smp_num_cpus < 2) {
		printk("*** YOU CANNOT LOCK THE ONLY AVAILABLE CPU ***\n");
		return 1;
	}
	if ((lock_srq = rt_request_srq(LOCKNAME, rtai_srq_handler, user_srq_handler)) < 0) {
		printk("CPU LOCK: no sysrq available.\n");
		return lock_srq;
	}
	rt_lock_cpu(RT_CPU, irqs, sizeof(irqs)/sizeof(int));
        return 0;
}


void cleanup_module(void)
{
        if (was_used) {
                printk("\nAHAH IT SEEMS IT WAS USED BY LINUX (%d), BAD.", was_used);
	}
	if (rt_free_srq(lock_srq) < 0) {
		printk("CPU UNLOCK: sysrq %d illegal or already free.\n", lock_srq);
	}
	rt_unlock_cpu();
	force_sig(SIGKILL, lock_process);
	return;
}
