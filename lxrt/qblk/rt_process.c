/*
COPYRIGHT (C) 2000  Pierre Cloutier (pcloutier@PoseidonControls.com)

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

#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include "../qblk.h"

#define TICK_PERIOD 1000000000
#define STACK_SIZE 2000

RT_TASK agentask;
static QBLK Main;
static int count;

static void SomeOtherThingsTodo( void *var1, int var2 )
{
// This function acts like a synchronous thread.
// Function rt_qReceive(...) does whatever is necessary to receive proxies,
// messages and schedule the execution of both static and dynamic QBLK's.
rt_printk( "QBLK Says: Hello World\n" );

count++;

// Reschedule for execution in one second.
if(count < 12) rt_qBlkWait( &Main, nano2count( 1000000000 ));
else rt_task_suspend(rt_whoami());
}

void fun(int t)
{
	char buf[64];
	pid_t from;
	int msglen;

	rt_printk("Task %p starts.\n", rt_lxrt_whoami());

	// init a qBlk
	rt_InitTickQueue();
	rt_qBlkInit( &Main, SomeOtherThingsTodo, 0, 0);
	rt_qBlkWait( &Main, nano2count(1000000000));

        from = rt_qReceive( 0, buf, sizeof(buf), &msglen);

	rt_task_suspend(rt_whoami());
	rt_printk("Task %p Oops.\n", rt_lxrt_whoami());
}

int init_module(void)
{
	RTIME period;
	rt_task_init(&agentask, fun, 0, STACK_SIZE, 0, 0, 0);
	rt_set_oneshot_mode();
	period = start_rt_timer((int) nano2count(TICK_PERIOD));
	rt_task_make_periodic(&agentask, rt_get_time() + period, period);
	return 0;
}

void cleanup_module(void)
{
	stop_rt_timer();
	rt_busy_sleep(nano2count(TICK_PERIOD));
        rt_qCleanup(&agentask);
	rt_task_delete(&agentask);
}
