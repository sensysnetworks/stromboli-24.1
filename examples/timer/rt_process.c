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


#define ECHO_ALWAYS 1

#define TICK 1000000 //us (!!!!! CAREFULL NEVER GREATER THAN 1E7 !!!!!)

#define APIC_TIMER 0

#define TIMER_FREQ (APIC_TIMER ? FREQ_APIC : FREQ_8254)

#define LOOPS 80 // dot products for each cpu
#define DIM   300 // size of the dot product vectors
#define MUL   3.141592 // a number to do something
#define RESULT (LOOPS*MUL*MUL*DIM*(DIM + 1)/2.0)
#define SECS_STEP 3 // macro to control the printk interval in seconds

/* simple module to exemplify the use of RTAI. However it can be used as it  */
/* is for a periodic time control, just change the dummy calculation and I/O */

#include <linux/kernel.h>
#include <linux/module.h>

#include <rtai.h>
#include <rtai_fifos.h>
#include <rtai_leds.h>

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

#define IPI_MSG RTAI_4_IPI

#define DTF 0
#define CMDF 1

#define FLOAT float

static volatile unsigned long do_out, out_secs, out_avrj, out_maxj, out_dot, out_timdot[2];
static volatile unsigned long cpu_cnt[2];

// this is called with the echo service attached at the Linux timer
static void print_something(int irq, void *dev_id, struct pt_regs *regs)
{
	if (do_out) {
		do_out = 0;
		printk("<>RT_HAL time: %ld s, AvrJ: %ld, MaxJ: %ld us (%ld,%ld,%ld)<> %ld %ld \n",
			out_secs, out_avrj, out_maxj, out_dot, out_timdot[0], out_timdot[1], cpu_cnt[0], cpu_cnt[1]);
//			rtf_put(DTF, &out_secs, sizeof(out_secs));
	}
}

const int Tick = TICK;

static FPU_ENV fpu_reg[NR_RT_CPUS];
static FPU_ENV linux_fpu_reg[NR_RT_CPUS];
static FLOAT  a[LOOPS*NR_RT_CPUS][DIM], b[DIM], c[LOOPS*NR_RT_CPUS]; 

// gcc compiler its smart in getting a very tight optimised loop for this
static FLOAT dot(FLOAT *a, FLOAT *b, int n)
{
	int i = n - 1;
	FLOAT s = 0.0;
	for(; i >= 0; i--) {
		s = s + a[i]*b[i];
	}
return s;
}

// this is an example of a periodic controller that does a lot of fp 
// calculations and keeps the Linux timer handling alive at due time
// it also controls the jitter and toggle a bit on the parallel port and
// strain fifos with a lot of (dummy) data
// the computer load it entails is controlled by TICK, LOOPS and DIM macros

#define NREC 1000
#define RECSIZE 5000
static struct { int rec; char buf[RECSIZE - sizeof(int)];} record;
static volatile int go;
static volatile int sync;
static volatile RTIME tg;

static void rt_timer_test(void)
{
	unsigned long cr0;
	static volatile RTIME t0, t;
	static volatile int count, secs = 100*SECS_STEP;
	static volatile int avrjitter, maxjitter;
	volatile double s; 
	volatile int i, jitter, cpuid;

	t = tg = rdtsc();
	cpuid = hard_cpu_id();
	sync = smp_num_cpus;
#ifdef CONFIG_SMP
	send_ipi_logical(cpu_online_map & ~(1 << cpuid), IPI_MSG);
#endif
	rt_toggle_leds(1);
	if (!count) {
		t0 = t;
	}

	rt_times.tick_time = rt_times.intr_time;
	rt_times.intr_time = rt_times.tick_time + rt_times.periodic_tick;
	jitter = imuldiv(rt_times.periodic_tick, CPU_FREQ, TIMER_FREQ) -
							 (int)(t - t0);
	t0 = t;
	if (jitter < 0) jitter = -jitter;
	avrjitter = (avrjitter + jitter)>>1;
	if (jitter > maxjitter && count > 100) maxjitter = jitter;

	save_cr0_and_clts(cr0);
	save_fpenv(linux_fpu_reg[cpuid]);
	restore_fpenv(fpu_reg[cpuid]);
		s = 0.0;
		for(i = cpuid*LOOPS; i < (cpuid + 1)*LOOPS; i++) {
			s += (c[i] = dot(a[i], b, DIM));
		}
	save_fpenv(fpu_reg[cpuid]);
	restore_fpenv(linux_fpu_reg[cpuid]);
	restore_cr0(cr0);
	cpu_cnt[cpuid]++;
	t = rdtsc() - tg;	
	if (atomic_dec_and_test((atomic_t *)&sync)) {
		if (go) {
			record.rec += RECSIZE;
			rtf_put(DTF, &record, sizeof(record));
		}
		t *= 10;
	}

	if (rt_times.tick_time >= rt_times.linux_time) {
		rt_times.linux_time += rt_times.linux_tick;
		if (++count == secs) {
			secs += 100*SECS_STEP;
			out_secs = secs/100;
			out_avrj = imuldiv(avrjitter, 1000000, CPU_FREQ);
			out_maxj = imuldiv(maxjitter, 1000000, CPU_FREQ);
			out_dot = (int)s;
			out_timdot[cpuid] = -imuldiv((int)t, 1000000, CPU_FREQ);
			do_out = ECHO_ALWAYS ? 1 : go;
		}
		if (!APIC_TIMER) {
			rt_pend_linux_irq(TIMER_8254_IRQ);
		}
	} 
}

static void cpu_gang(void)
{
	unsigned long cr0;
	static volatile RTIME t;
	volatile int i, cpuid;

	rt_toggle_leds(1);
	cpuid = hard_cpu_id();
	save_cr0_and_clts(cr0);
	save_fpenv(linux_fpu_reg[cpuid]);
	restore_fpenv(fpu_reg[cpuid]);
	for(i = cpuid*LOOPS; i < (cpuid + 1)*LOOPS; i++) {
		c[i] = dot(a[i], b, DIM);
	}
	save_fpenv(fpu_reg[cpuid]);
	restore_fpenv(linux_fpu_reg[cpuid]);
	restore_cr0(cr0);
	cpu_cnt[cpuid]++;
	t = rdtsc() - tg;	
	if (atomic_dec_and_test((atomic_t *)&sync)) {
		if (go) {
			record.rec += RECSIZE;
			rtf_put(DTF, &record, sizeof(record));
		}
		t *= 10;
	}
	out_timdot[cpuid] = imuldiv((int)t, 1000000, CPU_FREQ);
}

static int start_stop(unsigned int fifo)
{
	rtf_get(CMDF, &fifo, 1);
	rtf_reset(DTF);
	record.rec = 0;
	go = !go;
	return 1;
}

int init_module(void)
{
	rtf_create_using_bh_and_usr_buf(DTF, buf, NREC*RECSIZE, 0);
	rtf_create(CMDF, 100);
	rtf_create_handler(CMDF, start_stop);
	rt_mount_rtai();
{
	int linux_cr0, i, k, cpuid;
	cpuid = hard_cpu_id();
	save_cr0_and_clts(linux_cr0);
	save_fpenv(linux_fpu_reg[cpuid]);
	init_xfpu();
	for (i = 0; i < smp_num_cpus; i++) {
		save_fpenv(fpu_reg[i]);
	}
	for (i = 0; i < DIM; i++) {
		b[i] = MUL;
	}
	for (i = 0; i < NR_RT_CPUS*LOOPS; i++) {
		for (k = 0; k < DIM; k++) {
			a[i][k] = (k + 1)*MUL;
		}
	}
	restore_fpenv(linux_fpu_reg[cpuid]);
	restore_cr0(linux_cr0);
	rt_request_cpu_own_irq(IPI_MSG, cpu_gang);
	printk("<>>> FP RESULT CHECK %d <<<>\n", (int)RESULT);
	rt_assign_irq_to_cpu(TIMER_8254_IRQ, 0);
	rt_request_timer(rt_timer_test, imuldiv(Tick, TIMER_FREQ, 1000000000), APIC_TIMER); 
	rt_request_linux_irq(TIMER_8254_IRQ, print_something, "rt_timer", print_something);
}
	return 0;
}

void cleanup_module(void)
{
	rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
	rt_free_timer();
	rt_free_linux_irq(TIMER_8254_IRQ, print_something);
	rt_free_cpu_own_irq(IPI_MSG);
	rtf_destroy_using_usr_buf(DTF);
	rtf_destroy(CMDF);
	rt_umount_rtai();
	return;
}
