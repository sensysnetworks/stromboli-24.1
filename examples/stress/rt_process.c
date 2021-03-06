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


#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/io.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

#define JITTER_FIFO 0
#define PREEMPT_FIFO 1

#define ONE_SHOT

#define NAVRG 800

#define USE_FPU 0

#ifdef CONFIG_UCLINUX
#define TICK_TIME 1000000
#else
#define TICK_TIME 500000
#endif

static RT_TASK thread;
static RT_TASK Slow_Task;
static RT_TASK Fast_Task;

static int period;
static RTIME expected;

static int cpu_used[NR_RT_CPUS];

static void Slow_Thread(int t)
{
        static struct {char task, susres; unsigned long flags; RTIME time;} msg = {'S',};                      
        while (1) {  
		cpu_used[hard_cpu_id()]++;
                msg.time = rt_get_time_ns();
                msg.susres = 'r'; 
		rt_global_save_flags(&msg.flags);
                rtf_put(PREEMPT_FIFO, &msg, sizeof(msg)); 

                rt_busy_sleep(11*TICK_TIME);

                msg.time = rt_get_cpu_time_ns();
                msg.susres = 's'; 
		rt_global_save_flags(&msg.flags);
                rtf_put(PREEMPT_FIFO, &msg, sizeof(msg));

                rt_task_wait_period();                                        
        }
}                                        

static void Fast_Thread(int t) 
{                             
        static struct {char task, susres; unsigned long flags; RTIME time;} msg = {'F',};

        while (1) {
		cpu_used[hard_cpu_id()]++;
                msg.time = rt_get_time_ns();
                msg.susres = 'r'; 
		rt_global_save_flags(&msg.flags);
                rtf_put(PREEMPT_FIFO, &msg, sizeof(msg)); 

                rt_busy_sleep(2*TICK_TIME);

                msg.time = rt_get_time_ns();
                msg.susres = 's'; 
		rt_global_save_flags(&msg.flags);
                rtf_put(PREEMPT_FIFO, &msg, sizeof(msg));

                rt_task_wait_period();                                        
        }                      
}

static void fun(int thread) {

	struct sample { long long min; long long max; int avrg; unsigned long flags; } samp;
	int diff;
	int skip;
	int average;
	int min_diff;
	int max_diff;

	min_diff = 1000000000;
	max_diff = -1000000000;
	while (1) {
		unsigned long flags;
		samp.flags = 0x201;
		average = 0;

		for (skip = 0; skip < NAVRG; skip++) {
			cpu_used[hard_cpu_id()]++;
			expected += period;
			rt_task_wait_period();

			rt_global_save_flags(&flags);
			samp.flags &= flags;
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
		samp.avrg = average/NAVRG;
		rtf_put(JITTER_FIFO, &samp, sizeof(samp));
	}
}

int init_module(void)
{
	RTIME start;

	rtf_create(JITTER_FIFO, 1000);
	rtf_create_using_bh(PREEMPT_FIFO, 20000, 0);
	rt_linux_use_fpu(USE_FPU);
	rt_task_init(&thread, fun, 0, 3000, 0, USE_FPU, 0);
	rt_task_init(&Fast_Task, Fast_Thread, 0, 3000, 1, 0, 0);
	rt_task_init(&Slow_Task, Slow_Thread, 0, 3000, 2, 0, 0);
#ifdef ONE_SHOT
	rt_set_oneshot_mode();
#endif
	period = start_rt_timer(nano2count(TICK_TIME));
	expected = start = rt_get_time() + 100*period;
	rt_task_make_periodic(&thread, start, period);
	rt_task_make_periodic(&Fast_Task, start, 4*period);
	rt_task_make_periodic(&Slow_Task, start, 24*period);

	return 0;
}

void cleanup_module(void)
{
	int cpuid;

	stop_rt_timer();	
	rt_task_delete(&thread);
	rt_task_delete(&Slow_Task);
	rt_task_delete(&Fast_Task);
	rtf_destroy(JITTER_FIFO);
	rtf_destroy(PREEMPT_FIFO);
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
}
