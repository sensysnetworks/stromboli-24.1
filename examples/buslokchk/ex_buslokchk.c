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


#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/io.h>

#include <rtai.h>

MODULE_LICENSE("GPL");

//#define PARPORT 0x378
#ifdef PARPORT
static int bit;
#define toggle() outb(bit = 1 - bit, PARPORT); 
#else
#define toggle()
#endif

#define PERIOD      100000  // nanoseconds
#define THRESHOLD   30000  // nanoseconds
#define INILOOPS    100  // to avoid reporting startup latencies

static RTIME t0;
static int period, threshold, loops, maxj;

static int rt_timer_tick_ext(int irq, unsigned long data)
{
	static RTIME t;
	int jit;
	if (loops++ < INILOOPS) {
		t0 = rdtsc();
	} else {
		t = rdtsc();
		toggle();
		if ((jit = abs((int)(t - t0) - period)) > maxj) {
			maxj = jit;
			if (maxj > threshold) {
				rtai_print_to_screen("<%d, %d>\n", imuldiv(loops, PERIOD, 1000000), imuldiv(maxj, 1000000000, CPU_FREQ));
			}
		}
		t0 = t;
	}
	rt_times.tick_time = rt_times.intr_time;
	rt_times.intr_time = rt_times.tick_time + rt_times.periodic_tick;
	rt_set_timer_delay(0);
	if (rt_times.tick_time >= rt_times.linux_time) {
		rt_times.linux_time += rt_times.linux_tick;
		hard_sti();
		rt_pend_linux_irq(TIMER_8254_IRQ);
		return 0;
	} 
	hard_sti();
	return 1;
}

int init_module(void)
{
	rt_mount_rtai();
	printk("THRESHOLD %d (ns), PERIOD %d (ns)\n", THRESHOLD, PERIOD);
	period = imuldiv(PERIOD, CPU_FREQ, 1000000000);
	threshold = imuldiv(THRESHOLD, CPU_FREQ, 1000000000);
	rt_request_timer((void *)rt_timer_tick_ext, imuldiv(PERIOD, FREQ_8254, 1000000000), 0);
	rt_set_global_irq_ext(TIMER_8254_IRQ, 1, 0);
	return 0;
}

void cleanup_module(void)
{
	rt_free_timer();
	rt_umount_rtai();
}
