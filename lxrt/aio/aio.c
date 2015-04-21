/*
 * Project: RTAI/LXRT - POSIX AIO
 *
 * File: $Id: aio.c,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $
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

#ifndef LXRT_MODULE
#define LXRT_MODULE
#endif

static int errno;
#define __KERNEL_SYSCALLS__

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/smp_lock.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>

#include <rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <rtai_lxrt.h>

#include "names.h"
#include "proxies.h"
#include "msg.h"
#include "qblk.h"
#include "registry.h"
#include "traps.h"

#include "rtai_lxk.h"

#include "aio.h"
#include "aio_misc.h"
#include "rtai_syscall.h"

MODULE_AUTHOR("Erwin Rol <erwin@muffin.org>");
MODULE_DESCRIPTION("RTAI POSIX like aio support.");
MODULE_LICENSE("GPL");

static volatile int stop_manager;

int aio_threads = 16;		/* Maximal number of threads.  */
MODULE_PARM(aio_threads, "i");
MODULE_PARM_DESC(aio_threads, "Maximal number of threads");

int aio_num_requests = 64;				/* Number of expected simultanious requests. */
MODULE_PARM(aio_num_requests, "i");
MODULE_PARM_DESC(aio_num_requests, "Number of expected simultanious requests");

int aio_idle_time = 10;		/* Number of seconds before idle thread terminates.  */
MODULE_PARM(aio_idle_time, "i");
MODULE_PARM_DESC(aio_idle_time, "Number of seconds before idle threads terminate");

int aio_debug = 0;		/* Flag to enable debug output */
MODULE_PARM(aio_debug, "i");
MODULE_PARM_DESC(aio_debug, "Flag to enable debug output");

MBX new_thread_mbx;

int rt_create_thread(struct requestlist * req){
	return rt_mbx_send(&new_thread_mbx, &req , sizeof(req));
}

int lxk_create_thread(struct requestlist * req){
	return lxk_mbx_send(&new_thread_mbx, &req , sizeof(req));
}

static int kaiod_manager(void *unused){
	RT_TASK* rt_task;
	sigset_t tmpsig;
	int waitpid_result;
	struct requestlist* arg;
	int res;

	sprintf(current->comm,"kaiod manager");
	daemonize();

	current->policy = SCHED_RR;

	/* Block all signals except SIGKILL and SIGSTOP */
	spin_lock_irq(&current->sigmask_lock);
	tmpsig = current->blocked;
	siginitsetinv(&current->blocked, sigmask(SIGKILL) | sigmask(SIGSTOP) );
	recalc_sigpending(current);
	spin_unlock_irq(&current->sigmask_lock);

	rt_task = lxk_task_init(current->pid,1,0,0);

	if(rt_task == 0){
		printk("AIO: lxrt_task_init failed!\n");
		goto end;
	}

	/* main loop */
	while( 1 )
	{
		arg = 0;
		if( ( res = lxk_mbx_receive(&new_thread_mbx, &arg, sizeof(arg) ) ) != 0 )
		{
			// this should not happen !??!?!?!
			printk("krt_mbx_receive returned res=%d arg=%p\n",res,arg);
			break;
		}
		
		if(stop_manager == 1)
			break;

		if ( signal_pending(current) )
			break;
		
		if( kernel_thread(handle_fildes_io, arg , CLONE_FS | CLONE_FILES | CLONE_SIGHAND ) <= 0 )
		{
			// could not create thread and we can not return an error, so mark this request with the error
			arg->aiocbp->__error_code = 0;
			arg->aiocbp->__return_value = -1;
		}
	}


	lxk_task_delete(rt_task);
end:

	/* reap the zombie-daemons */
	do {
		waitpid_result = waitpid(-1,NULL,__WCLONE|WNOHANG);
	} while (waitpid_result > 0);


	MOD_DEC_USE_COUNT;

	return 0;
}

// aio MODULE INIT and CLEANUP functions

int __init aio_init(void)
{
	MOD_INC_USE_COUNT;
	
	stop_manager = 0;
	
	rt_sem_init(&__aio_requests_mutex, 1);

	rt_cond_init(&__aio_new_request_notification);

	rt_mbx_init( &new_thread_mbx, sizeof( struct requestlist* ) );
	
	kernel_thread(kaiod_manager, NULL , CLONE_FS | CLONE_FILES | CLONE_SIGHAND);

#ifdef CONFIG_PROC_FS
	rtai_proc_aio_register();
#endif

	return 0 ;
}

void aio_cleanup(void)
{
	stop_manager = 1;
	
	rt_mbx_delete(&new_thread_mbx);
	
#ifdef CONFIG_PROC_FS
	rtai_proc_aio_unregister();
#endif

	free_res();

	printk("AIO: removed\n");
	return;
}

module_init(aio_init)
module_exit(aio_cleanup)


EXPORT_SYMBOL(aio_read);
EXPORT_SYMBOL(aio_write);
EXPORT_SYMBOL(aio_open);
EXPORT_SYMBOL(aio_close);
EXPORT_SYMBOL(lio_listio);
EXPORT_SYMBOL(aio_error);
EXPORT_SYMBOL(aio_return);
EXPORT_SYMBOL(aio_cancel);
EXPORT_SYMBOL(aio_suspend);
EXPORT_SYMBOL(aio_fsync);

EXPORT_SYMBOL(rt_open);
EXPORT_SYMBOL(rt_close);
EXPORT_SYMBOL(rt_pread);
EXPORT_SYMBOL(rt_pwrite);

