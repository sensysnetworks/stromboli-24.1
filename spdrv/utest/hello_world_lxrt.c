/*
COPYRIGHT (C) 2002  Giuseppe Renoldi (giuseppe@renoldi.org)

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

int main(int argc, char **argv)
{
	unsigned long testcomtsk_name = nam2num("TESTCOM");
	RT_TASK *testcomtsk;
	char hello[] = "Hello World\n\r";
	int retval = 0;
	
 	if (!(testcomtsk = rt_task_init(testcomtsk_name, 1, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(1);
	}
	rt_set_oneshot_mode();
	start_rt_timer(0);
	mlockall(MCL_CURRENT | MCL_FUTURE);

//	rt_make_hard_real_time();
	
	if (rt_spopen(COM1, 9600, 8, 1, RT_SP_PARITY_NONE, RT_SP_NO_HAND_SHAKE, RT_SP_FIFO_SIZE_DEFAULT)) {
		printf("hello_world_lxrt: error in rtai_spdrv_setup()\n");
		retval = 1;
	} else {	
		rt_spwrite(COM1, hello, sizeof(hello)-1);
		rt_sleep(nano2count(500000000));
		printf("rtai_spdrv_lxrt test: >>%s<< sent.\n", hello);
		rt_spclose(COM1);  // release port
		printf("rtai_spdrv_lxrt test: finished\n");
	}    

	rt_make_soft_real_time();
	stop_rt_timer();
	rt_task_delete(testcomtsk);
	exit(retval);
}
