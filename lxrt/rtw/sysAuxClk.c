/*
COPYRIGHT (C) 2001  Paolo Mantegazza  (mantegazza@aero.polimi.it)
                    Giuseppe Quaranta (quaranta@aero.polimi.it)

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


//#define TIMER_IRQ_CPU_MAP 0x1

#include <linux/module.h>

#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include "sysAuxClk.h"

static int srq, enabled, ticks_per_sec = HZ;

static int (*rt_timer_setup)(int ticks_per_sec, void (*do_at_timer_irq)(void));

static volatile unsigned long overuns;

static SEM *sem;

static inline void fire_task(void)
{
	if (enabled > 0) {
		rt_sem_signal(sem);
		if (sem->count > 0) {
			overuns++;
		}
	} else if (enabled < 0 && sem->count < 0) {
		enabled = 1;
	}
}

static void do_at_timer_irq(void)
{
	fire_task();
}

static void loc8254_timer_handler(void)
{
	fire_task();
	rt_times.tick_time = rt_times.intr_time;
	rt_times.intr_time += rt_times.periodic_tick;
	rt_set_timer_delay(0);
	if (rt_times.tick_time >= rt_times.linux_time) {
		rt_times.linux_time += rt_times.linux_tick;
		rt_pend_linux_irq(TIMER_8254_IRQ);
	} 
}

#define STK_SIZ 8000
static RT_TASK timer_task;

static void loctask_timer_handler(int i)
{
	while (1) {
		fire_task();
		rt_task_wait_period();
	}
}

static int loc8254_timer_setup(int tps, void (*rt_timer_tick)(void))
{
	rt_free_timer();
	if (tps > (CLOCK_TICK_RATE >> 4)) {
		tps = CLOCK_TICK_RATE >> 4;
	}
	if ((tps = (CLOCK_TICK_RATE + (tps >> 1))/tps) > LATCH) {
		tps = LATCH; 
	}
	rt_request_timer(loc8254_timer_handler, tps, 0); 
	return ticks_per_sec = (CLOCK_TICK_RATE + (tps >> 1))/tps; 
}

static int loctask_timer_setup(int tps, void (*rt_timer_tick)(void))
{
	RTIME period;
	if (!timer_task.magic) {
		rt_task_init(&timer_task, loctask_timer_handler, 0, STK_SIZ, 0, 0, 0);
	}
	rt_set_periodic_mode();
	period = start_rt_timer(nano2count(1000000000/tps));
	rt_task_make_periodic(&timer_task, rt_get_time() + period, period);
	return ticks_per_sec = tps; 
}

static int UseTask = 0;
MODULE_PARM(UseTask, "i");

static int local_timer_setup(int tps, void (*rt_timer_tick)(void))
{
	if (UseTask) {
		printk("***** USING A TASK BASED LOCAL TIMER *****\n");
		return loctask_timer_setup(tps, rt_timer_tick);
	}
	printk("***** USING THE 8254 BASED LOCAL TIMER *****\n");
	return loc8254_timer_setup(tps, rt_timer_tick);
}

long long aux_clk_handler(unsigned int arg)
{
	switch (((int *)arg)[0]) {
		case AUX_CLK_RATE_SET: {
			enabled = 0;
			ticks_per_sec = rt_timer_setup(((int *)arg)[1], do_at_timer_irq);
//			printk("AUX_CLK_RATE_SET %d TICKS PER SEC.\n", ticks_per_sec);
			return ticks_per_sec;
		}
		case AUX_CLK_RATE_GET: {
//			printk("AUX_CLK_RATE_GET %d TICKS PER SEC.\n", ticks_per_sec);
			return ticks_per_sec;
		}
		case AUX_CLK_CONNECT: {
			if ((sem = ((SEM **)arg)[1])) { 
//				printk("AUX_CLK_CONNECT SEMAPHORE %p.\n", sem);
				return 0;
			} else {
//				printk("AUX_CLK_CONNECT INVALID SEMAPHORE\n");
				return -EINVAL;
			}
		}
		case AUX_CLK_ENABLE: {
			if (sem) {
//				printk("AUX_CLK_ENABLE DONE.\n");
				enabled = -1;
				return sem->count = overuns = 0;
			} else {
//				printk("AUX_CLK_ENABLE FAILED, NO SEMAPHORE CONNECTED.\n");
				return -EINVAL;
			}
		}
		case AUX_CLK_DISABLE: {
//			printk("AUX_CLK_DISABLE DONE.\n");
			return enabled = 0;
		}
		case AUX_CLK_OVERUNS: {
			return overuns;
		}
		case AUX_CLK_STATUS: {
			return enabled;
		}
		case AUX_CLK_CPUFREQ_MHZ: {
			return (tuned.cpu_freq + 499999)/1000000;
		}
		case AUX_CLK_IS_EXT: {
			rt_timer_setup = ((void **)arg)[1] ? ((void **)arg)[1] : local_timer_setup;
//			printk("EXT_AUX_CLK_IS_EXT %p (LOCAL 8254: %p).\n", rt_timer_setup, local_timer_setup);
			return rt_timer_setup != local_timer_setup ? 0 : -EINVAL;
		}
		case AUX_CLK_SEM: {
			RT_TASK *task;
			return (task = ((RT_TASK **)arg)[1]) && (task->state & SEMAPHORE) ? (int)task->blocked_on : 0;
		}
		case AUX_CLK_REL_SEM: {
			RT_TASK *task;
			if ((task = ((RT_TASK **)arg)[1]) && (task->state & SEMAPHORE)) {
				task->state = 0;
				(task->queue.prev)->next = task->queue.next;
				(task->queue.next)->prev = task->queue.prev;
				if (!((SEM *)(task->blocked_on))->type) {
					((SEM *)(task->blocked_on))->count++;
				} else {
					((SEM *)(task->blocked_on))->count = 1;
				}
				return 0;
			}
			return -ENODEV;
		}
	}
	return 0;
}

void donothing(void) { }

int init_module(void)
{
	if (!(srq = rt_request_srq(nam2num("AUXCLK"), donothing, aux_clk_handler))) {
		printk("CANNOT INSTALL AUX CLK\n");
		return 1;
	}
	rt_mount_rtai();
	rt_timer_setup = local_timer_setup;
#ifdef TIMER_IRQ_CPU_MAP
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, TIMER_IRQ_CPU_MAP);
#endif
	return 0;
}

void cleanup_module(void)
{
	enabled = 0;
#ifdef TIMER_IRQ_CPU_MAP
	rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
#endif
	stop_rt_timer();
	rt_free_timer();
	rt_free_srq(srq);
	rt_task_delete(&timer_task);
	rt_umount_rtai();
	return;
}
