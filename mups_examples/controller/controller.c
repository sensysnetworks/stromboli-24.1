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


#include <linux/module.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <asm/system.h>
#include <asm/smp.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/bitops.h>
#include <asm/atomic.h>
#include <asm/desc.h>
#include <asm/delay.h>

#include <rtai_fifos.h>
#include <rtai_shm.h>
#include <asm/rtai.h>
#include "controller.h"

static struct shm_control_block *shm = 0;
static atomic_t last_cpu = ATOMIC_INIT(-2*WARMUP - 1);

static unsigned long cpu_cnt[NR_RT_CPUS]  = { 0, };
static unsigned long rprt_cnt[NR_RT_CPUS] = { 0, };
static int avrj[NR_RT_CPUS] = { 0, };
static int maxj[NR_RT_CPUS] = { 0, };
static RTIME tprev[NR_RT_CPUS];
static int tick_period;
static int file_size = 1;  // just for testing, to check how things are going

static int *buf_base, *buf_frec, *buf_lrec;
static RTYPE *slope_to_pos_mat, *slopes, *pos;

static void MulMatVec(RTYPE *mat, RTYPE *vi, RTYPE *vo, int nr, int nc)
{
	int i, k;
	RTYPE s;

	for (i = 0; i < nr; i++) {
		s = 0.0;
		for (k = nc - 1; k >= 0; k--) {
			s += (mat[k]*vi[k]);
		}
		vo[i] = s;
		mat += nc;
	}
}

static RTYPE inline ChkSum(RTYPE *v, int n)
{
	int i;
	RTYPE s;

	for (s = 0.0, i = 0; i < n; i++) {
		s += v[i];
	}
	return s;
}

static void mirror_handler(void)
{
	unsigned long cr0, linux_fpu_reg[28];
	int cpuid, jitter, last, *record;
	struct exec_info info;
	RTIME tnow;

	tnow = rdtsc();
	cpuid = hard_cpu_id();

	if (!shm || !(shm->run)) {
		tprev[cpuid] = tnow;
		cpu_cnt[cpuid] = rprt_cnt[cpuid] = 0;
		atomic_set(&last_cpu, -2*WARMUP - 1);
		buf_frec = buf_lrec = buf_base;
		file_size = 0;
		return;
	}

	save_cr0_and_clts(cr0);
	save_fpenv(linux_fpu_reg);
	MulMatVec(slope_to_pos_mat + shm->nmeb*cpuid,
		  slopes,
		  pos + shm->nrb*cpuid,
		  shm->nrb,
		  shm->nc);
	restore_fpenv(linux_fpu_reg);
	restore_cr0(cr0);

	last = 0;
	if (atomic_inc_and_test_greater_zero(&last_cpu)) {
		atomic_set(&last_cpu, -NR_RT_CPUS + 1);
		last = cpuid + 1;
		if (shm->nrec < shm->maxrec) {

			record = buf_lrec;
// questa e', o dovrebbe, essere l'ultima cpu a finire; tutto quello che c'e 
// in giro da loggare va qui inpacchettato in record, le istruzioni qui sotto
// sono tanto per fare qualcosa e verificare tempo e se e come arriva al disco
			{
				int i;
				file_size += shm->recsize; 
				*record = file_size;
				for (i = 1; i < shm->recsize; i++) {
					record[i] = buf_lrec[i];
				}
			}
		
			shm->nrec++;
			if (++shm->lrec > shm->maxrec) {
				shm->lrec = 1;
				buf_lrec = buf_base;
			} else {
				buf_lrec += shm->recsize;
			}
			if (rtf_sem_trywait(SHMFIF)) {
				if (shm->nrecw <= 0) {
					shm->nrec += shm->nrecw;
					shm->nrecw = shm->nrec;
				}
				rtf_sem_post(SHMFIF);
				rtf_sem_post(SYNFIF);
			}
		}
	}

	if ((jitter = tick_period - (int)(tnow - tprev[cpuid])) < 0) {
		jitter = -jitter;
	}
	if (last_cpu.counter >= 0 && jitter > maxj[cpuid]) {
		maxj[cpuid] = jitter;
	}
	avrj[cpuid] = (avrj[cpuid] + jitter)>>1;
	tprev[cpuid] = tnow;

	if (cpu_cnt[cpuid]++ >= rprt_cnt[cpuid]) {
		rprt_cnt[cpuid] += REPORT_CNT;
		info.cpuid  = cpuid;
		info.cnt    = cpu_cnt[cpuid];
		info.avrj   = imuldiv(avrj[cpuid], 1000000, CPU_FREQ);
		info.maxj   = imuldiv(maxj[cpuid], 1000000, CPU_FREQ);
		info.res    = (int)ChkSum(pos + shm->nrb*cpuid, shm->nrb);
		info.last   = last;
		info.exctim = imuldiv(rdtsc() - tnow, 1000000, CPU_FREQ);
		rtf_put(SHMFIF, &info, sizeof(struct exec_info));
	}
}

int synfif_handler(unsigned int fifo, int rw)
{
	struct shm_control_block *p;

	if (!shm && rw == 'w' && rtf_get(SYNFIF, &p, sizeof(p)) == sizeof(p) && p) {
		p = rtai_kmalloc(nam2num(SHMNAM), 1);
		buf_frec = buf_lrec = buf_base = (int *)p + p->buf_base_ofst;
		slope_to_pos_mat = (RTYPE *)p + p->slope_to_pos_mat_ofst;
		slopes = slope_to_pos_mat + (p->nmeb)*NR_RT_CPUS;
		pos = slopes + p->nc;
		shm = p;
		return 0;
	}
	return -EINVAL;
}

static struct apic_timer_setup_data apic_setup_data[NR_RT_CPUS] = { 
	{1, TICK_PERIOD*1000}, 
#if NR_RT_CPUS > 1
	{1, TICK_PERIOD*1000}
#endif
};

int init_module(void)
{
	rt_mount_rtai();
	tick_period = imuldiv(TICK_PERIOD, CPU_FREQ, 1000000);
	rt_request_apic_timers(mirror_handler, apic_setup_data);
	rtf_create(SYNFIF, SYNFIF_SIZE);
	rtf_create_handler(SYNFIF, X_FIFO_HANDLER(synfif_handler));
	rtf_sem_init(SYNFIF, 0);
	rtf_create(SHMFIF, SHMFIF_SIZE);
	rtf_sem_init(SHMFIF, 1);
	return 0;
}

void cleanup_module(void)
{
	int cpuid;
	rt_free_apic_timers();
	rtf_destroy(SHMFIF);
	rtf_destroy(SYNFIF);
	rtai_kfree(nam2num(SHMNAM));
	rt_umount_rtai();
	printk("\n\nCPU USE SUMMARY\n");
	for (cpuid = 0; cpuid < NR_RT_CPUS; cpuid++) {
		printk("# %d -> %ld\n", cpuid, cpu_cnt[cpuid]);
	}
	printk("END OF CPU USE SUMMARY\n\n");
	return;
}
