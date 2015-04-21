/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: tst-rtai_syscall.c,v 1.1.1.1 2004/06/06 14:02:37 rpm Exp $
 *
 * Copyright: (c)2001 Erwin Rol <erwin@muffin.org>
 *
 * Licence:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */


#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>

#include <rtai.h>
#include <rtai_sched.h>

#include "aio.h"
#include "rtai_syscall.h"


MODULE_AUTHOR("Erwin Rol erwin@muffin.org");
MODULE_DESCRIPTION("RTAI system call support test module");

static void do_test (int unused);

RT_TASK test_task;

static void
do_test(int unused)
{
	char buf[1000];
	int fd, cnt;

	/* open */

	printk("aio test start\n");

	fd = rt_open( "/tmp/aio_test",O_RDWR | O_CREAT | O_TRUNC, 0777 );
	if(fd < 0){
		printk("rt_open failed!\n");
		return;
	}

	/* First a simple test.  */
	for (cnt = 0; cnt < 10; cnt++){
		memset(&buf[cnt * 100],'0'+cnt,100);
		if( rt_pwrite( fd, &buf[cnt * 100] , 100 , cnt * 100) != 100 ){
			printk("rt_pwrite failed!\n");
			return;
		}
	}

	/* Read now as we've written it.  */
	memset( buf, '\0', sizeof (buf) );
	/* Issue the commands.  */
	for (cnt = 0; cnt < 10; cnt++ )
	{
		if( rt_pread( fd, &buf[cnt * 100] , 100 , cnt * 100) != 100 ){
			printk("rt_pread failed!\n");
			return;
		}
	}

	/* Test this.  */
	for (cnt = 0; cnt < 1000; ++cnt)
	{
		if (buf[cnt] != '0' + (cnt / 100))
		{
			printk("comparison failed for aio_read test buf[%d] = 0x%02x\n",cnt,buf[cnt]);
			return;
		}
	}

	if (cnt == 1000)
		printk("aio_read test ok\n");

	if( rt_close( fd ) < 0 )
 	{
		printk("rt_close failed\n");
		return;
	}

	printk("finished\n");

	return;
}


// aio MODULE INIT and CLEANUP functions

int __init aio_test_init(void)
{
	RTIME tick_period;

	rt_task_init(&test_task, do_test, 0, 16*1024 , 0, 0, 0);

	rt_set_oneshot_mode();

	tick_period = start_rt_timer( nano2count(1000000) );

	rt_task_resume(&test_task);
	
	return 0 ;
}

void aio_test_cleanup(void)
{
	stop_rt_timer();

	rt_task_delete(&test_task);

	printk("AIO-Test: removed\n");

	return;
}

module_init(aio_test_init)
module_exit(aio_test_cleanup)

