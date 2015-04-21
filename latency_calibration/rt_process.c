/*
 * COPYRIGHT (C) 2001  Paolo Mantegazza <mantegazza@aero.polimi.it>
 *               2002  Robert Schwebel  <robert@schwebel.de>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 * 
 * ACKNOWLEDGMENT: 
 * 
 * Part of this code is derived from that of the latency calibration example,
 * which in turn was copied from a similar program distributed with NMT-RTL.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rtai_proc_fs.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Latency measurement tool for RTAI");
MODULE_AUTHOR("Paolo Mantegazza <mantegazza@aero.polimi.it>, Robert Schwebel <robert@schwebel.de>");


/*
 *	command line parameters
 */

int overall = 1;
MODULE_PARM(overall, "i");
MODULE_PARM_DESC(overall,
		 "Calculate overall (1) or per-loop (0) statistics (default: 1)");

int period = 100000;
MODULE_PARM(period, "i");
MODULE_PARM_DESC(period, "period in ns (default: 100000)");

static int loops;
int avrgtime = 1;
MODULE_PARM(avrgtime, "i");
MODULE_PARM_DESC(avrgtime, "Averages are calculated for <avrgtime (s)> runs (default: 1)");

int use_fpu = 0;
MODULE_PARM(use_fpu, "i");
MODULE_PARM_DESC(use_fpu, "do we want to use the FPU? (default: 0)");

int start_timer = 1;
MODULE_PARM(start_timer, "i");
MODULE_PARM_DESC(start_timer,
		 "declares if the timer should be started or not (default: 1)");

#define DEBUG_FIFO 3
#define TIMER_TO_CPU 3		// < 0  || > 1 to maintain a symmetric processed timer.
#define RUNNABLE_ON_CPUS 3	// 1: on cpu 0 only, 2: on cpu 1 only, 3: on any;
#define RUN_ON_CPUS (smp_num_cpus > 1 ? RUNNABLE_ON_CPUS : 1)

/*
 *	Global Variables
 */

int period_counts;
RTIME expected;
RT_TASK thread;
struct sample {
	long long min;
	long long max;
	int index;
} samp;

static int cpu_used[NR_RT_CPUS];


/* 
 *	/proc/rtai/latency_calibrate entry
 */

#ifdef CONFIG_PROC_FS
static int proc_read(char *page, char **start, off_t off, 
                     int count, int *eof, void *data)
{
	PROC_PRINT_VARS;
	PROC_PRINT("## RTAI latency calibration tool \n");
	PROC_PRINT("# overall=%i\n",overall);
	PROC_PRINT("# period=%i\n",period);
	PROC_PRINT("# avrgtime=%i\n",avrgtime);
	PROC_PRINT("# use_fpu=%i\n",use_fpu);
	PROC_PRINT("# start_timer=%i\n",start_timer);
	PROC_PRINT("\n");
	PROC_PRINT_DONE;
}
#endif


/*
 *	Periodic realtime thread 
 */
 
void
fun(int thread)
{

	int diff = 0;
	int i;
	int average;
	int min_diff = 0;
	int max_diff = 0;

	/* If we want to make overall statistics */
	/* we have to reset min/max here         */
	if (overall) {
		min_diff =  1000000000;
		max_diff = -1000000000;
	}

	while (1) {

		/* Not overall statistics: reset min/max */
		if (!overall) {
			min_diff =  1000000000;
			max_diff = -1000000000;
		}

		average = 0;

		for (i = 0; i < loops; i++) {
			cpu_used[hard_cpu_id()]++;
			expected += period_counts;
			rt_task_wait_period();

			diff = (int) count2nano(rt_get_time() - expected);

			if (diff < min_diff) { min_diff = diff; }
			if (diff > max_diff) { max_diff = diff; }
			average += diff;
		}
		samp.min = min_diff;
		samp.max = max_diff;
		samp.index = average / loops;
		rtf_put(DEBUG_FIFO, &samp, sizeof (samp));
	}
}


/*
 *	Initialisation. We have to select the scheduling mode and start 
 *      our periodical measurement task.  
 */

int
init_module(void)
{

	RTIME start;

	/* XXX check option ranges here */

	/* register a proc entry */
#ifdef CONFIG_PROC_FS
	create_proc_read_entry("rtai/latency_calibrate", /* name             */
	                       0,			 /* default mode     */
	                       NULL, 			 /* parent dir       */
			       proc_read, 		 /* function         */
			       NULL			 /* client data      */
	);
#endif

	rtf_create(DEBUG_FIFO, 16000);	/* create a fifo length: 16000 bytes */
	rt_linux_use_fpu(use_fpu);	/* declare if we use the FPU         */

	rt_task_init(			/* create our measuring task         */
			    &thread,	/* poiter to our RT_TASK             */
			    fun,	/* implementation of the task        */
			    0,		/* we could transfer data -> task    */
			    3000,	/* stack size                        */
			    0,		/* priority                          */
			    use_fpu,	/* do we use the FPU?                */
			    0		/* signal? XXX                       */
	);

	rt_set_runnable_on_cpus(	/* select on which CPUs the task is  */
		&thread,		/* allowed to run                    */
		RUN_ON_CPUS
	);

	/* Test if we have to start the timer                                */
	if (start_timer) {
		rt_set_oneshot_mode();
		rt_assign_irq_to_cpu(TIMER_8254_IRQ, TIMER_TO_CPU);
		period_counts = start_rt_timer(nano2count(period));
	} else {
		period_counts = nano2count(period);
	}

	loops = (1000000000*avrgtime)/period;

	/* Calculate the start time for the task. */
	/* We set this to "now plus 10 peroids"   */
	start = rt_get_time() + 10 * period_counts;
	expected = start;
	rt_task_make_periodic(&thread, start, period_counts);
	return 0;
}


/*
 *	Cleanup 
 */

void
cleanup_module(void)
{
	int cpuid;

	/* If we started the timer we have to revert this now. */
	if (start_timer) {
		rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
		stop_rt_timer();
	}

	/* Now delete our task and remove the FIFO. */
	rt_task_delete(&thread);
	rtf_destroy(DEBUG_FIFO);

	/* Remove proc dir entry */
#ifdef CONFIG_PROC_FS
	remove_proc_entry("rtai/latency_calibrate", NULL);
#endif

	/* Output some statistics about CPU usage */
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");

}
