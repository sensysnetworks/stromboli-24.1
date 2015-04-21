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


#define TICK 100000 //ns (!!!!! CAREFULL NEVER GREATER THAN 1E7 !!!!!)

#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

static DECLARE_WAIT_QUEUE_HEAD(waitq);
static struct task_struct *sleeping_process;

static int srq;

static inline int is_signalled(sigset_t signal, sigset_t blocked)
{
	int i =  _NSIG_WORDS - 1;
	do {
		if ( signal.sig[i] & ~blocked.sig[i] ) {
			return 1;
		}
	} while (i--);
	return 0;
}

static long long user_srq_handler(unsigned int whatever)
{
	long long time;

	if (whatever == 1) {
		return llimd(rt_times.periodic_tick, 1000000, FREQ_8254);
	}
	sleeping_process = current;
	interruptible_sleep_on(&waitq);
#if LINUX_EXT_VERSION_CODE < KERNEL_EXT_VERSION(2,4,0,8)
	if (is_signalled(current->signal, current->blocked)) {
		return -ERESTARTSYS;
	}
#else
	if (is_signalled(current->pending.signal, current->blocked)) {
		return -ERESTARTSYS;
	}
#endif
	time = llimd(rt_times.tick_time, 1000000, FREQ_8254);
// let's show how to communicate. Copy to and from user shall allow any kind of
// data interchange and service.
	copy_to_user((long long *)whatever, &time, 8);
	return time;
}

static void rtai_srq_handler(void)
{
        if (sleeping_process && !sleeping_process->run_list.next) {
		sleeping_process = 0;
		wake_up_interruptible(&waitq);
	}
}

static void rt_timer_tick(void)
{
	rt_pend_linux_srq(srq);
	rt_times.tick_time = rt_times.intr_time;
	rt_times.intr_time = rt_times.tick_time + rt_times.periodic_tick;
	rt_set_timer_delay(0);
	if (rt_times.tick_time >= rt_times.linux_time) {
		rt_times.linux_time += rt_times.linux_tick;
		rt_pend_linux_irq(TIMER_8254_IRQ);
	} 
}

int init_module(void)
{
	rt_mount_rtai();
	srq = rt_request_srq(0xcacca, rtai_srq_handler, user_srq_handler);
	rt_request_timer(rt_timer_tick, imuldiv(TICK, FREQ_8254, 1000000000), 0); 
	return 0;
}

void cleanup_module(void)
{
	rt_free_timer();
	rt_free_srq(srq);
	rt_umount_rtai();
}

