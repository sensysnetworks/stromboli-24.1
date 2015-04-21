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
#include <linux/module.h>
*/


#include <linux/module.h>

#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include "../registry.h"

#include "period.h"

static MBX mbx;

static SEM sem;

static char wakeup;

static void rt_timer_tick(void)
{
	RT_TASK *task;
	rt_times.tick_time = rt_times.intr_time;
	rt_times.intr_time = rt_times.tick_time + rt_times.periodic_tick;
	rt_set_timer_delay(0);
	if (rt_times.tick_time >= rt_times.linux_time) {
		rt_times.linux_time += rt_times.linux_tick;
		rt_pend_linux_irq(TIMER_8254_IRQ);
	} 
	rt_mbx_receive_if(&mbx, &wakeup, 1);
	if (wakeup) {
		if ((task = rt_get_adr(nam2num("PRCTSK")))) {

			if (wakeup==1) rt_sem_signal(&sem);
			else rt_task_resume(task);
		}
	}
}

int init_module(void)
{
	rt_mbx_init(&mbx, 1);
	rt_register(nam2num("RESMBX"), &mbx, IS_MBX, 0);
	rt_sem_init(&sem, 0);
	rt_register(nam2num("RESEM"), &sem, IS_SEM, 0);
	rt_request_timer(rt_timer_tick, imuldiv(PERIOD, FREQ_8254, 1000000000), 0);
	return 0;
}

void cleanup_module(void)
{
	rt_free_timer();
	rt_mbx_delete(&mbx);
	rt_sem_delete(&sem);
}
