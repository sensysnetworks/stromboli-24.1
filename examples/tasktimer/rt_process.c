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


#define TICK 1000000 //ns (!!!!! CAREFULL NEVER GREATER THAN 1E7 !!!!!)

#define LOOPS 80 // dot products for each cpu
#define DIM   300 // size of the dot product vectors
#define MUL   3.141592 // a number to do something
#define RESULT (LOOPS*MUL*MUL*DIM*(DIM + 1)/2.0)
#define SECS_STEP 3000000000LL //ns, macro to control the the period of the print task

/* simple module to exemplify the use of RTAI. However it can be used as it  */
/* is for a periodic time control, just change the dummy calculation and I/O */

#include <linux/module.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rtai_leds.h>

#define ONE_SHOT

#define DTF   0
#define CMDF  1
#define ECHOF 2

#define FLOAT float

static unsigned long out_secs, out_avrj, out_maxj, out_dot, out_timdot[2];
static int cpu_used[NR_RT_CPUS];
static volatile int go;

static void print_times(int arg)
{
	while(1) {
		rtf_put(ECHOF, &out_secs, sizeof(out_secs));
		rtf_put(ECHOF, &out_avrj, sizeof(out_avrj));
		rtf_put(ECHOF, &out_maxj, sizeof(out_maxj));
		rtf_put(ECHOF, &out_dot, sizeof(out_dot));
		rtf_put(ECHOF, &out_timdot[0], sizeof(out_timdot[0]));
		rtf_put(ECHOF, &out_timdot[1], sizeof(out_timdot[1]));
		rtf_put(ECHOF, &cpu_used[0], sizeof(cpu_used[0]));
		rtf_put(ECHOF, &cpu_used[1], sizeof(cpu_used[1]));
/*
		printk("<>RT_HAL time: %ld s, AvrJ: %ld, MaxJ: %ld us (%ld,%ld,%ld)<> %d %d \n", out_secs, out_avrj, out_maxj, out_dot, out_timdot[0], out_timdot[1], cpu_used[0], cpu_used[1]);
*/
		rt_task_wait_period();
	}
}

static FLOAT  a[NR_RT_CPUS*LOOPS][DIM], b[DIM], c[NR_RT_CPUS*LOOPS]; 

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
static volatile int sync;
static volatile RTIME tg, tick_period;
static int timer_freq;

static void task1(int arg)
{
	static volatile RTIME t0, t;
	static volatile int secs, first = 0;
	static volatile int avrjitter, maxjitter;
	volatile double s; 
	volatile int i, jitter, cpuid;
	t0 = rdtsc();
	while(1) {
		t = tg = rdtsc();
		rt_toggle_leds(1);
		cpuid = hard_cpu_id();
		cpu_used[cpuid]++;
		sync = smp_num_cpus;
		jitter = imuldiv(tick_period, CPU_FREQ, timer_freq) - 
							(int)(t - t0);
		t0 = t;
		if (jitter < 0) jitter = -jitter;
		avrjitter = (avrjitter + jitter)>>1;
		if (jitter > maxjitter && first > 100) maxjitter = jitter;
		first++;
		s = 0.0;
		for(i = cpuid*LOOPS; i < (cpuid + 1)*LOOPS; i++) {
			s += (c[i] = dot(a[i], b, DIM));
		}
		t = rdtsc() - tg;	
		if (atomic_dec_and_test((atomic_t *)&sync)) {
			if (go) {
				record.rec += RECSIZE;
				rtf_put(DTF, &record, sizeof(record));
			}
			t *= 10;
		}
		secs += TICK/1000;
		out_secs = secs/1000000;
		out_avrj = imuldiv(avrjitter, 1000000, CPU_FREQ);
		out_maxj = imuldiv(maxjitter, 1000000, CPU_FREQ);
		out_dot = (int)s;
		out_timdot[cpuid] = -imuldiv((int)t, 1000000, CPU_FREQ);
		rt_task_wait_period();
	}
}

static void task2(int arg)
{
	static volatile RTIME t;
	volatile int i, cpuid;

	while(1) {
		cpuid = hard_cpu_id();
		cpu_used[cpuid]++;
		rt_toggle_leds(1);
		for(i = cpuid*LOOPS; i < (cpuid + 1)*LOOPS; i++) {
			c[i] = dot(a[i], b, DIM);
		}
		t = rdtsc() - tg;	
		if (atomic_dec_and_test((atomic_t *)&sync)) {
			if (go) {
				record.rec += RECSIZE;
				rtf_put(DTF, &record, sizeof(record));
			}
			t *= 10;
		}
		out_timdot[cpuid] = imuldiv((int)t, 1000000, CPU_FREQ);
		rt_task_wait_period();
	}
}

static int start_stop(unsigned int fifo)
{
	rtf_get(CMDF, &fifo, 1);
	rtf_reset(DTF);
	record.rec = 0;
	go = !go;
	return 1;
}

static RT_TASK Task1;
static RT_TASK Task2;
static RT_TASK Task3;

static FPU_ENV linux_fpu_reg;

int init_module(void)
{
	RTIME now;
	int linux_cr0, i, k, timer_cpu;

	save_cr0_and_clts(linux_cr0);
	save_fpenv(linux_fpu_reg);
	init_xfpu();
	for (i = 0; i < DIM; i++) {
		b[i] = MUL;
	}
	for (i = 0; i < NR_RT_CPUS*LOOPS; i++) {
		for (k = 0; k < DIM; k++) {
			a[i][k] = (k + 1)*MUL;
		}
	}
	restore_fpenv(linux_fpu_reg);
	restore_cr0(linux_cr0);
	printk("<>>> FP RESULT CHECK %d <<<>\n", (int)RESULT);

	rtf_create_using_bh_and_usr_buf(DTF, buf, NREC*RECSIZE, 0);
	rtf_create(CMDF, 100);
	rtf_create_handler(CMDF, start_stop);
	rtf_create(ECHOF, 1000);
	rt_task_init(&Task1, task1, 0, 4000, 0, 1, 0); 
	rt_task_init(&Task2, task2, 0, 4000, 1, 1, 0); 
	rt_task_init(&Task3, print_times, 0, 2000, 1, 0, 0); 
#ifdef ONE_SHOT
	rt_set_oneshot_mode();
	timer_freq = CPU_FREQ;
#endif                             
	tick_period = start_rt_timer(nano2count(TICK));
	if ((timer_cpu = rt_get_timer_cpu()) > 0) {
		rt_set_runnable_on_cpus(&Task1, timer_cpu);
		rt_set_runnable_on_cpus(&Task2, timer_cpu == 1 ? 2 : 1);
#ifndef ONE_SHOT
		timer_freq = FREQ_APIC;
#endif                             
	} else {
		rt_assign_irq_to_cpu(TIMER_8254_IRQ, 0);
		rt_set_runnable_on_cpus(&Task1, 1);
		rt_set_runnable_on_cpus(&Task2, 2);
#ifndef ONE_SHOT
		timer_freq = FREQ_8254;
#endif                             
	}
	now = rt_get_time() + 5*tick_period;
	rt_task_make_periodic(&Task1, now, tick_period);
	rt_task_make_periodic(&Task2, now, tick_period);
	rt_task_make_periodic(&Task3, now + nano2count(SECS_STEP), nano2count(SECS_STEP));
	return 0;
}

void cleanup_module(void)
{
	int cpuid;
	rt_reset_irq_to_sym_mode(TIMER_8254_IRQ);
	stop_rt_timer();
	rt_busy_sleep(10000000);
	rt_task_delete(&Task3);
	rt_task_delete(&Task2);
	rt_task_delete(&Task1);
	rtf_destroy_using_usr_buf(DTF);
	rtf_destroy(CMDF);
	rtf_destroy(ECHOF);
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %d\n", cpuid, cpu_used[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
	return;
}
