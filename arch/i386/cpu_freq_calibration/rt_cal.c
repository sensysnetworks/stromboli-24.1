#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <asm/io.h>

#include <asm/rtai.h>

static int SECS = 5;

MODULE_PARM(SECS, "i");

static int RESET_COUNT;

static void calibrate(void)
{
	static int count = 0, gcount = -1;
	static RTIME tbase;
	static FPU_ENV fpu_reg;
	unsigned long linux_cr0;
	double freq;
	union {unsigned long long time; unsigned long time_lh[2]; } tsc;
	tsc.time =  rd_CPU_ts();
	if (gcount < 0) {
		tbase = tsc.time;
	}
	gcount++;
	if (++count == RESET_COUNT) {
		tsc.time -= tbase;
		save_cr0_and_clts(linux_cr0);
		save_fpenv(fpu_reg);
		freq =  (double)tsc.time_lh[1]*(double)0x100000000LL + (double)tsc.time_lh[0];
		count =  (freq*CLOCK_TICK_RATE)/(((double)gcount)*LATCH) + 0.4999999999999;
		restore_fpenv(fpu_reg);
		restore_cr0(linux_cr0);
		printk("\n->>> MEASURED CPU_FREQ: %d (hz) [%d (s)], IN USE %lu (hz) <<<-\n", count, gcount/100 + 1, CPU_FREQ);
		count = 0;
	}
	rt_pend_linux_irq(TIMER_8254_IRQ);
}

int init_module(void)
{
	RESET_COUNT = SECS*100;
	rt_mount_rtai();
	rt_request_global_irq(TIMER_8254_IRQ, calibrate);
	printk("\n->>> HERE WE GO (PRINTING EVERY %d SECONDS, 'make stop' to end calibrating) <<<-\n\n", RESET_COUNT/100);
	return 0;
}

void cleanup_module(void)
{
	rt_free_timer();
	rt_free_global_irq(TIMER_8254_IRQ);
	rt_umount_rtai();
}
