/*
COPYRIGHT (C) 1999  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


/* Produce a rectangular wave on output 0 of a parallel port */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <asm/system.h>
#include <asm/fixmap.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/bitops.h>
#include <asm/atomic.h>
#include <asm/desc.h>

#include <asm/rtai.h>
#include <rtai_fifos.h>
#include <rtai_sched.h>

/* the address of the parallel port -- you probably should change this */

#define LPT 0x378

#define TICK_PERIOD 50000

#define STACK_SIZE 2000

#define LOOPS 10000000

#define END 0xFFFF

#define NTASKS 8

RT_TASK thread[NTASKS];

static int cpu_used[NR_RT_CPUS];

void driver(int t)
{
	RT_TASK *thread[NTASKS];
	int i, l;
	unsigned int msg = 0;

	for (i = 1; i < NTASKS; i++) {
		thread[0] = rt_receive(0, &msg);
		thread[msg] = thread[0];
	}
	for (i = 1; i < NTASKS; i++) {
		rt_return(thread[i], i);
	}
	rt_task_make_periodic_relative_ns(rt_whoami(), 100000000, TICK_PERIOD);

	msg = 0;
	l = LOOPS;
	while(l--) {
		for (i = 1; i < NTASKS; i++) {
			cpu_used[hard_cpu_id()]++;
			if (i%2) {
				rt_rpc(thread[i], msg, &msg);
			} else {
				rt_send(thread[i], msg);
				msg = 1 - msg;
			}
			rt_task_wait_period();
		}
	}
	for (i = 1; i < NTASKS; i++) {
		rt_send(thread[i], END);
	}
	rt_task_delete(rt_whoami());
}


void fun(int t)
{
	unsigned int msg;
	rt_rpc(&thread[0], t, &msg);
	while(msg != END) {
		cpu_used[hard_cpu_id()]++;
		rt_receive(&thread[0], &msg);
		outb((msg & 1), LPT);
		if (rt_isrpc(&thread[0])) {
			rt_return(&thread[0], 1 - msg);
		}
	}
	outb(0, LPT);
	rt_task_delete(rt_whoami());
}


static struct apic_timer_setup_data apic_setup_data[NR_RT_CPUS] = { 
	{1, TICK_PERIOD}, 
#if NR_RT_CPUS > 1
	{1, TICK_PERIOD} 
#endif
};

int init_module(void)
{
	int i;

	rt_task_init_cpuid(&thread[0], driver, 0, STACK_SIZE, 0, 0, 0, 0);
	for (i = 1; i < NTASKS; i++) {
		rt_task_init_cpuid(&thread[i], fun, i, STACK_SIZE, 0, 0, 0, i%2);
	}
	start_rt_apic_timers(apic_setup_data, 0);
	rt_busy_sleep(100000000);
	for (i = 0; i < NTASKS; i++) {
		rt_task_resume(&thread[i]);
	}
	return 0;
}


void cleanup_module(void)
{
	int i, cpuid;
	stop_rt_timer();
	for (i = 0; i < NTASKS; i++) {
		rt_task_delete(&thread[i]);
	}
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
}
