
/*
 *  * (C) Pierre Cloutier <pcloutier@PoseidonControls.com> LGPLv2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <ctype.h>

#include <rtai_lxrt_user.h>
#include <leds_lxrt.h>

static char msg[512], rep[512];

int main(int argc, char* argv[])
{
	unsigned long srv_name = nam2num("SRV");
	RT_TASK *srv;
	pid_t pid, my_pid, proxy ;
	int err,/* i,*/ count, msglen ;
	RTIME period;
	char *pt;
	
	init_linux_scheduler(SCHED_FIFO, 98);
		
	err = lock_all(0,0);
	if(err) {
		printf("lock_all() fails\n");
		exit(-1);
	}

 	if (!(srv = rt_task_init(srv_name, 0, 0, 0))) {
		printf("CANNOT INIT SRV TASK\n");
		exit(-1);
	}

	my_pid = rt_Alias_attach("");
	if(!my_pid) {
		rtai_print_to_screen("Cannot attach name SRV\n");
		exit(-1);
	}

	period = nano2count(1000000);
    rt_set_oneshot_mode();
	start_rt_timer(period);

	printf("SRV starts (%p)\n", srv);

    rt_make_hard_real_time();
	
	rt_task_make_periodic(srv, rt_get_time(), period);

  	proxy = rt_Proxy_attach( 0, "More beer please", 17, -1);
	if(proxy <= 0 ) {
		rtai_print_to_screen("Failed to attach proxy\n");
		exit(-1);
	}

    pid = rt_Receive( 0, 0, 0, &msglen);
	if(pid) {
		// handshake to give the proxy to CLT
		rtai_print_to_screen("rt_Reply the proxy %04X msglen = %d\n", proxy, msglen);
		rt_Reply( pid, &proxy, sizeof(proxy));
	}

	rt_sleep(nano2count(1000000000));
	count = 8 ;
	while(count--) {
		memset( msg, 0, sizeof(msg));
		pid = rt_Receive( 0, msg, sizeof(msg), &msglen);

		if(pid == proxy) {
			rtai_print_to_screen("SRV receives the PROXY event [%s]\n", msg);
			continue;
		} else if( pid <= 0) {
			rtai_print_to_screen("SRV rt_Receive() failed\n");
//			goto Exit;
			continue;
		}
		
		rtai_print_to_screen("SRV received msg    [%s] %d bytes from pid %04X\n", msg, msglen, pid);

		memcpy( rep, msg, sizeof(rep));
		pt = (char *) rep;
		while(*pt) *pt++ = toupper(*pt);
		if(rt_Reply(pid, rep, sizeof(rep)))
			rtai_print_to_screen("SRV rt_Reply() failed\n");
	}

	if(rt_Proxy_detach(proxy))
		rtai_print_to_screen("SRV cannot detach proxy\n");

	if(rt_Name_detach(my_pid)) 
		rtai_print_to_screen("SRV cannot detach name\n");

//Exit:
	rt_make_soft_real_time();

	rt_sleep(nano2count(1000000000));

	if(rt_task_delete(srv))
		rtai_print_to_screen("SRV cannot delete task\n");

	return 0;
}
