/*
COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it)

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

#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>

#define ONE_SHOT

/* the address of the parallel port -- you probably should change this */
#define LPT 0x378

#define TIMER_TO_CPU 3 // < 0 || > 1 to maintain a symmetric processed timer.
#define RUNNABLE_ON_CPUS (i%2 + 1) // forced statically half and half
//#define RUNNABLE_ON_CPUS 3  // 1: on cpu 0 only, 2: on cpu 1 only, 3: on any.
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)

#define TICK_PERIOD 50000
#define STACK_SIZE 2000
#define LOOPS 200000

#define NTASKS 8

static unsigned int END = 0xFFFF;

RT_TASK thread[NTASKS];

static int cpu_used[NR_RT_CPUS];

RTIME tick_period;

void driver(int t)
{
	RT_TASK *thread[NTASKS];
	int i, l;
	unsigned int msg = 0;
	RTIME now;

	for (i = 1; i < NTASKS; i++) {
		thread[i] = 0;
	}
	for (i = 1; i < NTASKS; i++) {
		thread[0] = rt_receivex(0, &msg, sizeof(msg), &l);
			if (msg < 1 || msg >= NTASKS || thread[msg]) {

				rt_printk("SYNCHRONIZATION FAILED IN DRIVER (%d, %d).\n", t, i);
				return;
			}
		thread[msg] = thread[0];
	}
	for (i = 1; i < NTASKS; i++) {
		rt_returnx(thread[i], &i, sizeof(i));
	}
	now = rt_get_time();
	rt_task_make_periodic(rt_whoami(), now + NTASKS*tick_period, tick_period);

	msg = 0;
	l = LOOPS;
	while(l--) {
		for (i = 1; i < NTASKS; i++) {
			cpu_used[hard_cpu_id()]++;
			if (i%2) {
				rt_rpcx(thread[i], &msg, &msg, sizeof(msg), sizeof(msg));
			} else {
				rt_sendx(thread[i], &msg, sizeof(msg));
				msg = 1 - msg;
			}
			rt_task_wait_period();
		}
	}
	for (i = 1; i < NTASKS; i++) {
		rt_sendx(thread[i], &END, sizeof(END));
	}
	rt_printk("DRIVER ENDS\n");
}


void fun(int t)
{
	unsigned int msg, l;
	rt_rpcx(&thread[0], &t, &msg, sizeof(t), sizeof(msg));
	if (msg != t) {
		rt_printk("SYNCHRONIZATION FAILED IN TASK (%d, %d).\n", t, msg);
		return;
	}
	while(msg != END) {
		cpu_used[hard_cpu_id()]++;
		rt_receivex(&thread[0], &msg, sizeof(msg), &l);
		outb((msg & 1), LPT);
		if (rt_isrpcx(&thread[0])) {
			msg = 1 - msg;
			rt_returnx(&thread[0], &msg, sizeof(msg));
		}
	}
	outb(0, LPT);
	rt_printk("TASK %d ENDS\n", t);
}


int init_module(void)
{
	int i;

#ifdef ONE_SHOT
	rt_set_oneshot_mode();
#endif
	rt_task_init(&thread[0], driver, 0, STACK_SIZE, 0, 0, 0);
	for (i = 1; i < NTASKS; i++) {
		rt_task_init(&thread[i], fun, i, STACK_SIZE, 0, 0, 0);
	}
	tick_period = start_rt_timer(nano2count(TICK_PERIOD));
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, TIMER_TO_CPU);
	for (i = 0; i < NTASKS; i++) {
		rt_set_runnable_on_cpus(&thread[i], RUN_ON_CPUS); 
	}
	for (i = 0; i < NTASKS; i++) {
		rt_task_resume(&thread[i]);
	}
	return 0;
}


void cleanup_module(void)
{
	int i, cpuid;
	rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
	stop_rt_timer();
	rt_busy_sleep(10000000);
	for (i = 0; i < NTASKS; i++) {
		rt_task_delete(&thread[i]);
	}
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
}
