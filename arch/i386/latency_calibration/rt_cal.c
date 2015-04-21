/*
ACKNOWLEDGMENT: 
- Part of this code is derived from that of the latency calibration example,
  which in turn was copied from a similar program distributed with NMT-RTL.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>

#define SKIP 50000

#define PERIOD 100000

int period;

RTIME expected;

RT_TASK thread;

void fun(int thread) {

	int skip, average;

	average = 0;
	for (skip = 0; skip < SKIP; skip++) {
		expected += period;
		rt_task_wait_period();

		average += (int)count2nano(rt_get_time() - expected);
	}
        if (abs(imuldiv(tuned.setup_time_TIMER_CPUNIT, 1000000000, tuned.cpu_freq) - SETUP_TIME_APIC) < 3) {
		rt_printk("\n\n*** '#define LATENCY_APIC %d' (IN USE %d)", LATENCY_APIC + average/SKIP, LATENCY_APIC);
	} else {
		rt_printk("\n\n*** '#define LATENCY_8254 %d' (IN USE %d)", LATENCY_8254 + average/SKIP, LATENCY_8254);
	}
	rt_printk(", you can do 'make stop' now ***\n");
	while(1) {
		rt_task_wait_period();
	}
}

int init_module(void)
{
	rt_task_init(&thread, fun, 0, 3000, 0, 0, 0);
	rt_set_oneshot_mode();
	period = start_rt_timer(nano2count(PERIOD));
	expected = rt_get_time() + 10*period;
	rt_task_make_periodic(&thread, expected, period);
//	rt_printk("\n\n*** 'LATENCY_8254 IN USE %d", LATENCY_8254);
	printk("\n*** Wait %d seconds for it ... ***\n\n", (int)(((long long)SKIP*(long long)PERIOD)/1000000000));
	return 0;
}

void cleanup_module(void)
{
	stop_rt_timer();	
	rt_task_delete(&thread);
}
