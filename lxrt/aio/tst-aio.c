/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: tst-aio.c,v 1.1.1.1 2004/06/06 14:02:37 rpm Exp $
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
MODULE_DESCRIPTION("RTAI POSIX like aio support test module");

static void do_test (int unused);

RT_TASK test_task;

/* We might need a bit longer timeout.  */
#define TIMEOUT 20 /* sec */

/* These are for the temporary file we generate.  */
char *name;
int fd;

static int
do_wait( struct aiocb **cbp, size_t nent, int allowed_err )
{
	int go_on;
	size_t cnt;
	int result = 0;

	do
	{
		aio_suspend ((const struct aiocb *const *) cbp, nent, NULL);
		go_on = 0;
		for (cnt = 0; cnt < nent; ++cnt)
			if (cbp[cnt] != NULL)
			{
				if (aio_error (cbp[cnt]) == EINPROGRESS)
					go_on = 1;
				else
				{
					if (aio_return (cbp[cnt]) == -1
							&& (allowed_err == 0
							|| aio_error (cbp[cnt]) != allowed_err))
					{
						printk("Operation failed :%d\n", aio_error( cbp[cnt] ) );
						result = 1;
					}
					cbp[cnt] = NULL;
				}
			}
	} while (go_on);

	return result;
}


static void
do_test(int unused)
{
	struct aiocb cbs[10];
	struct aiocb cbs_fsync;
	struct aiocb *cbp_fsync;
	struct aiocb cbs_open;
	struct aiocb *cbp_open;
	struct aiocb cbs_close;
	struct aiocb *cbp_close;
	struct aiocb *cbp[10];
	char buf[1000];
	size_t cnt;
	int result = 0;

	/* open */

	printk("aio test start\n");

	cbs_open.aio_fildes = -1;
	cbs_open.aio_buf = "/tmp/aio_test";
	cbs_open.aio_mode =  0777;
	cbs_open.aio_flags = O_RDWR | O_CREAT | O_TRUNC;
	cbs_open.aio_reqprio = 0;
	cbs_open.aio_nbytes = 0;
	cbs_open.aio_offset = 0;
	cbs_open.aio_sigevent.sigev_notify = SIGEV_NONE;
	cbp_open = &cbs_open;

	aio_open( cbp_open );

	do_wait(&cbp_open,1,0);
	
	fd = cbs_open.aio_fildes;
	if(fd < 0){
		printk("open failed!\n");
	}

	/* Preparation.  */
	for (cnt = 0; cnt < 10; ++cnt)
	{
		cbs[cnt].aio_fildes = fd;
		cbs[cnt].aio_reqprio = 0;
		cbs[cnt].aio_buf = memset (&buf[cnt * 100], '0' + cnt, 100);
		cbs[cnt].aio_nbytes = 100;
		cbs[cnt].aio_offset = cnt * 100;
		cbs[cnt].aio_sigevent.sigev_notify = SIGEV_NONE;

		cbp[cnt] = &cbs[cnt];
	}

	/* First a simple test.  */
	for (cnt = 10; cnt > 0; ){
		aio_write( cbp[--cnt] );
	}

	/* Wait 'til the results are there.  */
	result |= do_wait( cbp, 10, 0 );

	/* Read now as we've written it.  */
	memset( buf, '\0', sizeof (buf) );
	/* Issue the commands.  */
	for (cnt = 10; cnt > 0; )
	{
		--cnt;
 		cbp[cnt] = &cbs[cnt];
		aio_read( cbp[cnt] );
	}

	/* Wait 'til the results are there.  */
	result |= do_wait( cbp, 10, 0 );


	/* Test this.  */
	for (cnt = 0; cnt < 1000; ++cnt)
	{
		if (buf[cnt] != '0' + (cnt / 100))
		{
			result = 1;
			printk("comparison failed for aio_read test\n");
			break;
		}
	}

	if (cnt == 1000)
		printk("aio_read test ok\n");


	/* Write again.  */
  	for (cnt = 10; cnt > 0; )
	{
		--cnt;
 		cbp[cnt] = &cbs[cnt];
		aio_write( cbp[cnt] );
	}

	cbs_fsync.aio_fildes = fd;

	if( aio_fsync( O_SYNC, &cbs_fsync ) < 0 )
 	{
		printk("aio_fsync failed\n");
		result = 1;
	}
	cbp_fsync = &cbs_fsync;
		
	result |= do_wait( &cbp_fsync, 1, 0);

  /* Test aio_cancel.  */

  /* Write again.  */
	/* Write again.  */
  	for (cnt = 10; cnt > 0; )
	{
		--cnt;
 		cbp[cnt] = &cbs[cnt];
		aio_write( cbp[cnt] );
	}

	/* Cancel all requests.  */
	if (aio_cancel (fd, NULL) == -1)
		printk("aio_cancel (fd, NULL) cannot cancel anything\n");

	result |= do_wait (cbp, 10, ECANCELED);


	/* Another test for aio_cancel.  */

  /* Write again.  */
	for (cnt = 10; cnt > 0; )
	{
		--cnt;
		cbp[cnt] = &cbs[cnt];
		aio_write (cbp[cnt]);
	}

  printk("finished3\n");

	/* Cancel all requests.  */
	for (cnt = 10; cnt > 0; )
	{
		if (aio_cancel (fd, cbp[--cnt]) == -1)
		{
			/* This is not an error.  The request can simply be finished.  */
			printk("aio_cancel (fd, cbp[%Zd]) cannot be canceled\n", cnt);
		}
	}

	printk("finished2\n");

	result |= do_wait (cbp, 10, ECANCELED);


	cbs_close.aio_fildes = fd;

	if( aio_close( &cbs_close ) < 0 )
 	{
		printk("aio_close failed\n");
		result = 1;
	}
	cbp_close = &cbs_close;
		
	result |= do_wait( &cbp_close, 1, 0);

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

//	now = rt_get_time();

	rt_task_resume(&test_task);

//	rt_task_make_periodic(&test_task, now + tick_period,  tick_period);
	
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

