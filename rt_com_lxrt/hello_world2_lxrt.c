/*
COPYRIGHT (C) 2000  Giuseppe Renoldi (guseppe@renoldi.org)

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
 * rt_com-LXRT test
 * ================
 *
 * RT-Linux kernel module for communication across serial lines.
 *
 * Adaptation of rt_com test modules to provide the same examples in
 * RTAI environment using LXRT.
 * 
 *
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

#include "rt_com_lxrt.h"
#include "rt_com.h"



int main(int argc, char **argv)
{
	unsigned long testcomtsk_name = nam2num("TESTCOM");
	RT_TASK *testcomtsk;
	char hello[] = "Hello World";
	int count;
	char buffer[64];
	int retval = 0;
	
	mlockall(MCL_CURRENT | MCL_FUTURE);

 	if (!(testcomtsk = rt_task_init(testcomtsk_name, 1, 0, 0))) {
		printf("CANNOT INIT MASTER TASK\n");
		exit(1);
	}

	rt_set_oneshot_mode();
	start_rt_timer(0);

	//rt_make_hard_real_time();
	
	// This example use rt_com_hwsetup() on port 0 and 1 so it needs
	// that you have compiled rt_com.c with rt_com_table[0].used=0 and
	// rt_com_table[1].used=0 otherwise rt_com_hwsetup() fails.
	// rt_com_hwsetup() set rt_com_table[].used=1 
	// 
	// On the contrary rt_com_setup needs that rt_com_table[].used=1
	// before calling it.
	
    if( (0 > rt_com_hwsetup( 0, 0, 0 )) ) {
    	printf("rt_com_lxrt test: problems with ttyS0 hardware setup\n" );
	    retval = 1;
    } else if( (0 > rt_com_hwsetup( 1, 0, 0 )) ) {
	    printf("rt_com_lxrt test: problems with ttyS1 hardware setup\n" );
	    rt_com_hwsetup( 0, -1, 0 );
	    retval = 1;
	} else if( (0 > rt_com_setup( 0, 9600, RT_COM_NO_HAND_SHAKE, RT_COM_PARITY_NONE, 1, 8, -1 ) ) 
	           || (0 > rt_com_setup( 1, 9600, RT_COM_NO_HAND_SHAKE, RT_COM_PARITY_NONE, 1, 8, -1 )) ){
	    printf("rt_com_lxrt test: problems with setup\n" );
	    rt_com_hwsetup( 0, -1, 0 );
	    rt_com_hwsetup( 1, -1, 0 );
	    retval = 1;
	}

        if (!retval) {
	    printf("rt_com_lxrt test: Sending >>%s<<\n", hello );
	    count = rt_com_write( 0, hello, sizeof( hello ) );
	    //printf("rt_com_write()=%d\n", count);
    	    rt_sleep(nano2count(500000000));

	    count = rt_com_read( 1, buffer, 63 );
	    buffer[count] = 0;
	    printf("rt_com_lxrt test: %d characters read: \"%s\"\n", count, buffer );
	}    

	rt_com_setup( 0, -1, 0, 0, 0, 0, 0 ); // release port
	rt_com_setup( 1, -1, 0, 0, 0, 0, 0 ); // release port
	rt_com_hwsetup( 0, -1, 0 );
	rt_com_hwsetup( 1, -1, 0 );

	printf("rt_com_lxrt test: finished\n");

	//rt_make_soft_real_time();
	stop_rt_timer();
	rt_task_delete(testcomtsk);
	exit(retval);
}


