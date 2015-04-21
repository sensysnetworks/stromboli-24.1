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

/*
ACKNOWLEDGMENT: 
- Part of this code is derived from that of the latency calibration example,
  which in turn was copied from a similar program distributed with NMT-RTL.
*/



#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>

#include <rtai.h>
#include <rtai_sched.h>
#include "rtai_fifos.h"

#define FIFO 0

#define SKIP 4000

#define PERIOD 100000

#define ONE_SHOT

static int period;
static RTIME expected;
static RT_TASK thread;
static struct sample { long long min; long long max; int index; } samp;

static int cpu_used[NR_RT_CPUS];

void fun(int thread) {

	int diff;
	int skip;
	int average;
	int min_diff;
	int max_diff;

	while (1) {
		min_diff = 1000000000;
		max_diff = -1000000000;
		average = 0;

		for (skip = 0; skip < SKIP; skip++) {
			cpu_used[hard_cpu_id()]++;
			expected += period;
			rt_task_wait_period();

			diff = (int)count2nano(rt_get_time() - expected);
			if (diff < min_diff) {
				min_diff = diff;
			}
			if (diff > max_diff) {
				max_diff = diff;
			}
		average += diff;
		}
		samp.min = min_diff;
		samp.max = max_diff;
		samp.index = average/SKIP;
		if (rtf_sem_trywait(FIFO)) {
			rtf_put(FIFO, &samp, sizeof(samp));
			rtf_sem_post(FIFO);
		}
	}
}

int init_module(void)
{
	RTIME start;
	rtf_create(FIFO, 1000);
	rt_task_init(&thread, fun, 0, 3000, 0, 0, 0);
#ifdef ONE_SHOT
	rt_set_oneshot_mode();
#endif
	period = start_rt_timer(nano2count(PERIOD));
	start = rt_get_time() + 10*period;
	expected = start;
	rt_task_make_periodic(&thread, start, period);
	return 0;
}

void cleanup_module(void)
{
	int cpuid;
	stop_rt_timer();	
	rt_busy_sleep(10000000);
	rt_task_delete(&thread);
	rtf_destroy(FIFO);
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
}
