/*
COPYRIGHT (C) 2000  Andrew Hooper (andrew@best.net.nz)

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

#define PERIOD 50000

static MBX mbx;

static SEM sem;

static void encoder_tick(void)
{
	static char start = 0;
	rt_times.tick_time = rt_times.intr_time;
	rt_times.intr_time = rt_times.tick_time + rt_times.periodic_tick;
	if (rt_times.tick_time >= rt_times.linux_time) {
		rt_times.linux_time += rt_times.linux_tick;
		rt_pend_linux_irq(0);
	} 
	rt_mbx_receive_if(&mbx, &start, 1);
	if (start) {
		rt_sem_signal(&sem);
	}
}

int init_module(void)
{
	rt_mbx_init(&mbx, 1);
	rt_register(nam2num("RESMBX"), &mbx, IS_MBX, current);
	rt_sem_init(&sem, 0);
	rt_register(nam2num("RESEM"), &sem, IS_SEM, current);
	rt_request_timer(encoder_tick, imuldiv(PERIOD, FREQ_8254, 1000000000), 0);
	return 0;
}

void cleanup_module(void)
{
	rt_free_timer();
	rt_mbx_delete(&mbx);
}
