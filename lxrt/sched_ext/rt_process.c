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

Module to test rtai_sched_ext.o and QNX like IPC.
*/

#include <linux/module.h>
#include <linux/ctype.h>

#include <asm/io.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include "../names.h"
#include "../msg.h"

#include <asm/smp.h>
#define TICK_PERIOD 1000000000
#define STACK_SIZE 2000

RT_TASK srv, clt;

void srvfun(int c)
{
	pid_t pid, my_pid, proxy;
	int count, msglen;
	char *pt, rep[128], msg[128];
   	
	my_pid = rt_Name_attach("THESERVER");
	
	proxy = rt_Proxy_attach( 0, "More beer please", 17, -1);
	if(proxy <= 0 ) {
		rtai_print_to_screen("Failed to attach proxy\n");
		rt_task_suspend(rt_whoami());
		}

	rtai_print_to_screen("SRV my_pid %04X proxy %04X\n", my_pid, proxy);
		
	pid = rt_Receive( 0, 0, 0, &msglen);
	if(pid) {
		// handshake to give the proxy to CLT
		rtai_print_to_screen("rt_Reply the proxy %04X msglen = %d\n", proxy, msglen);
		rt_Reply( pid, &proxy, sizeof(proxy));
		}

	count = 8 ;
	while(count--) {
		memset( msg, 0, sizeof(msg));
		pid = rt_Receive( 0, msg, sizeof(msg), &msglen);
		if(pid == proxy) {
			rtai_print_to_screen("SRV receives the PROXY event [%s]\n", msg);
			continue;
		} else if( pid <= 0) {
			rtai_print_to_screen("SRV rt_Receive() failed\n");
			continue;
		} else {

		rtai_print_to_screen("SRV received msg    [%s] %d bytes from pid %04X\n", msg, msglen, pid);
		memcpy( rep, msg, sizeof(rep));
		pt = (char *) rep;
		while(*pt) {
			*pt = toupper(*pt);
			pt++;
		}
		if(rt_Reply(pid, rep, sizeof(rep)))
			rtai_print_to_screen("SRV rt_Reply() failed\n");
		}
	}
	rt_Name_detach(my_pid);
	rt_Proxy_detach(proxy);
	rt_task_suspend(rt_whoami());
}

void cltfun(int c)
{
	pid_t srvpid, my_pid, proxy;
	int err, count, len;
	char rep[128], msg[128];
	
	my_pid = rt_Name_attach("");
	
	if (!(srvpid = rt_Name_locate("", "THESERVER"))) {
		rtai_print_to_screen("Cannot locate SRV (%04X)\n", srvpid);
		rt_task_suspend(rt_whoami());
	}
	
	len = rt_Send( srvpid, 0, &proxy, 0, sizeof(proxy));
	if(len == sizeof(proxy)) {
		rtai_print_to_screen("CLT got the proxy %04X\n", proxy );
		count = 4;
		while( count-- ) {
			err = rt_Trigger(proxy);
			if(err!=srvpid) rtai_print_to_screen("Failed to send the proxy\n");
		}
	}
	else rtai_print_to_screen("Failed to receive the proxy pid\n" );

	count = 4;
	while( count-- ) {
		rtai_print_to_screen("CLT sends to SRV\n" );
		strcpy( msg, "Hello Beautifull Linux World" );
		memset(rep, 0, sizeof(rep));
		len = rt_Send( srvpid, msg, rep, sizeof(msg), sizeof(rep));
		if( len < 0 ) {
			rtai_print_to_screen("CLT: rt_Send() failed\n" );
			break;
		}
		rtai_print_to_screen("CLT: reply from SRV [%s] %d\n", rep, len );
		rt_task_wait_period();
	}			
    rt_Name_detach(my_pid);
	rt_task_suspend(rt_whoami());
}			
	
int init_module(void)
{
    RTIME period;

	rt_task_init(&clt, cltfun, 0, STACK_SIZE, 0, 0, 0);
    rt_task_init(&srv, srvfun, 0, STACK_SIZE, 0, 0, 0);
	
	rt_set_oneshot_mode();
	
	start_rt_timer((int)(period=nano2count(TICK_PERIOD)));

	rt_task_make_periodic(&srv, rt_get_time() + period, period);
 	rt_task_make_periodic(&clt, rt_get_time() + period, period);
    
	return 0;
}

void cleanup_module(void)
{
	stop_rt_timer();
	rt_busy_sleep(TICK_PERIOD);
	rt_task_delete(&clt);
	rt_task_delete(&srv);
}
