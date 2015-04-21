/*
COPYRIGHT (C) 2002  Giuseppe Renoldi (guseppe@renoldi.org)

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


/*
 * rtai_spdrv-LXRT test
 * ================
 *
 * Adaptation of rtai_spdrv ktest modules to provide the same examples in
 * RTAI environment using LXRT.
 * 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>

#include <rtai_spdrv_lxrt.h>

#define DIMBUF 133

int main(int argc, char **argv)
{
	unsigned long testrtsptsk_name = nam2num("TESTRTSP");
	RT_TASK *testrtsptsk;
	char hello[] = "Hello World!, is it a beautiful day ;-) ???";
	int count;
	char buffer[DIMBUF];
	int retval = 0;

 	if (!(testrtsptsk = rt_task_init(testrtsptsk_name, 1, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(1);
	}
	rt_set_oneshot_mode();
	start_rt_timer(0);
	mlockall(MCL_CURRENT | MCL_FUTURE);

//	rt_make_hard_real_time();
	
	if ( !rt_spopen( COM1, 9600, 8, 1, RT_SP_PARITY_NONE, RT_SP_NO_HAND_SHAKE, RT_SP_FIFO_SIZE_DEFAULT) &&
		 !rt_spopen( COM2, 9600, 8, 1, RT_SP_PARITY_NONE, RT_SP_NO_HAND_SHAKE, RT_SP_FIFO_SIZE_DEFAULT) ) {
		printf("rtai_spdrv_lxrt test: Sending >>%s<<\n", hello);
		count = strlen(hello) - rt_spwrite(0, hello, strlen(hello));
//		printf("rt_spwrite() = %d\n", count);
		rt_sleep(nano2count(500000000));
		count = DIMBUF - rt_spread(1, buffer, DIMBUF);
		buffer[count] = 0;
		printf("rtai_spdrv_lxrt test: %d characters read: \"%s\"\n", count, buffer);
	} else {
		retval = 1;	
	    printf("rtai_spdrv_lxrt test: problems with setup\n" );
	}		

	rt_spclose(COM1); // release port
	rt_spclose(COM2); // release port
	printf("rtai_spdrv_lxrt test: finished\n");
	rt_make_soft_real_time();
	stop_rt_timer();
	rt_task_delete(testrtsptsk);
	exit(retval);
}
