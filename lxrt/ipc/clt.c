#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sched.h>

#include <rtai_lxrt_user.h>

int main(int argc, char* argv[])
{
	unsigned long clt_name = nam2num("CLT");
	int count, len, err;
	pid_t pid, my_pid, proxy;
	RT_TASK *clt;
	char msg[512], rep[512];
	struct sched_param mysched;

	mysched.sched_priority = 98;
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts(" ERROR IN SETTING THE SCHEDULER UP");
		perror( "errno" );
		exit(1);
 	}       

	err = lock_all(0,0);
	if(err) {
		printf("lock_all() fails\n");
		exit(2);
	}

	// Give a lower priority than SRV and proxy.	
 	if (!(clt = rt_task_init(clt_name, 1, 0, 0))) {
		printf("CANNOT INIT CLIENT TASK\n");
		exit(3);
	}

	if (!(my_pid = rt_Alias_attach("CLIENT"))) {
		printf("Cannot attach name CLT\n");
		exit(4);
	}

	rt_sleep(nano2count(1000000000));

 	if (!(pid = rt_Name_locate("", "SRV"))) {
		printf("Cannot locate SRV\n");
		exit(5);
	}

	len = rt_Send( pid, 0, &proxy, 0, sizeof(proxy));
	if(len == sizeof(proxy)) {
		rtai_print_to_screen("CLT got the proxy %04X\n", proxy );
		count = 4;
		while( count-- ) {
			err = rt_Trigger(proxy);
			if(err!=pid) printf("Failed to send the proxy\n");
		}
	}
	else printf("Failed to receive the proxy pid\n" );

	count = 4;
	while( count-- ) {
  		printf("CLT sends to SRV\n" );
		strcpy( msg, "Hello Beautifull Linux World" );
		memset(rep, 0, sizeof(rep));
		len = rt_Send( pid, msg, rep, sizeof(msg), sizeof(rep));
		if( len < 0 ) {
			printf("CLT: rt_Send() failed\n" );
			break;
		}
        printf("CLT: reply from SRV [%s] %d\n", rep, len );
		if (count) rt_sleep(nano2count(1000000000));
	}

	if(rt_Name_detach(my_pid)) {
		printf("CLT cannot detach name\n");
	}

	while(rt_get_adr(nam2num("SRV"))) rt_sleep(nano2count(1000000));
	rt_sleep(nano2count(1000000));
	if(rt_task_delete(clt)) {
		printf("CLT cannot delete task\n");
	}

	stop_rt_timer();
	
	return 0;
}
