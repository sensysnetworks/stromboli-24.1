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

#include <asm/system.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/bitops.h>
#include <asm/atomic.h>
#include <asm/desc.h>
#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>

/* the address of the parallel port -- you probably should change this */

#define LPT 0x378

#define TICK_PERIOD 20000

#define STACK_SIZE 2000

#define LOOPS 100000000

#define NTASKS 16

RT_TASK thread[NTASKS];

static int cpu_used[NR_RT_CPUS];

void fun(int t)
{
	unsigned int loops = LOOPS;
	while(loops--) {
		cpu_used[hard_cpu_id()]++;
		outb(t, LPT);
		rt_task_wait_period();
	}
}

static struct apic_timer_setup_data apic_setup_data[NR_RT_CPUS] = { 
	{1, TICK_PERIOD}, 
#if NR_RT_CPUS > 1
	{0, TICK_PERIOD} 
#endif
};

int init_module(void)
{
	int i, k;

	k = 1;
	for (i = 0; i < NTASKS; i++) {
		rt_task_init_cpuid(&thread[i], fun, k = ~k, STACK_SIZE, 0, 0, 0, i%2);
	}
	start_rt_apic_timers(apic_setup_data, 0);
	for (i = 0; i < NTASKS; i++) {
		rt_task_make_periodic_relative_ns(&thread[i], (i+1)*TICK_PERIOD, NTASKS*TICK_PERIOD);
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
