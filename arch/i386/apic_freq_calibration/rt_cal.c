#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <asm/io.h>

#include <asm/rtai.h>

#define LOCAL_TIMER_VECTOR  0x41

#define COUNT 4000000000U

static int SECS = 5;

MODULE_PARM(SECS, "i");

static int RESET_COUNT;

static void calibrate(void)
{
	static int count = 0, gcount = -1;
	static unsigned long tbase, time, temp;
	time = apic_read(APIC_TMCCT);
	if (gcount < 0) {
		tbase = time;
	}
	gcount++;
	if (++count == RESET_COUNT) {
		time = tbase - time;
		count =  imuldiv(time, CLOCK_TICK_RATE, gcount*LATCH);
		printk("\n->>> MEASURED APIC_FREQ: %d (hz) [%d (s)], IN USE %d (hz) <<<-\n", count, gcount/100 + 1, FREQ_APIC);
		count = 0;
	}
	rt_pend_linux_irq(TIMER_8254_IRQ);
	temp = (apic_read(APIC_ICR) & (~0xCDFFF)) |
	       (APIC_DM_FIXED | APIC_DEST_ALLINC | LOCAL_TIMER_VECTOR);
	apic_write(APIC_ICR, temp);
}

static void just_ret(void)
{
	return;
}

int init_module(void)
{
	RESET_COUNT = SECS*100;
	rt_mount_rtai();
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, 1 << hard_cpu_id());
	rt_request_timer(just_ret, COUNT, 1);
	rt_request_global_irq(TIMER_8254_IRQ, calibrate);
	printk("\n->>> HERE WE GO (PRINTING EVERY %d SECONDS, 'make stop' to end calibrating) <<<-\n\n", RESET_COUNT/100);
	return 0;
}

void cleanup_module(void)
{
	rt_free_timer();
	rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
	rt_free_global_irq(TIMER_8254_IRQ);
	rt_umount_rtai();
}
