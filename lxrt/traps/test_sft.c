
/*
 * (C) Pierre Cloutier <pcloutier@PoseidonControls.com> LGPLv2
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

#include <rtai_lxrt_user.h>

static char msg[512], rep[512];

int main(int argc, char *argv[])
{
	unsigned long srv_name = nam2num("SRV");
	RT_TASK *srv;
	pid_t pid, my_pid ;
	int err, i, count, msglen, *zpt ;
	RTIME period;

        init_linux_scheduler(SCHED_FIFO, 98);

        err = lock_all(0,0);
        if(err) {
                printf("lock_all(stack,heap) failed\n");
                exit(2);
        }

        printf("SRV starts\n");

 	if (!(srv = rt_task_init(srv_name, 0, 0, 0))) {
		printf("CANNOT INIT SRV TASK\n");
		exit(3);
	}

	rt_set_oneshot_mode();

	period = nano2count(1000000);

        start_rt_timer(period);

	// Add int $1 in ../names.c to test trap while buddy task is running.	
	my_pid = rt_Alias_attach("");
	if(!my_pid) {
                printf("Cannot attach name\n");
                printf("and did not stop!\n");
                exit(4);
        }

	rt_sleep(nano2count(1000000000));
	i = count = 0 ;
	zpt = (int *)0;
        while(++count) {
		memset( msg, 0, sizeof(msg));
		pid = rt_Creceive( 0, msg, sizeof(msg), &msglen, period);
		if(pid) {
			printf("SRV received msg    [%s] %d bytes from pid %04X\n", msg, msglen, pid);
			memcpy( rep, msg, sizeof(rep));
			if(rt_Reply(pid, rep, sizeof(rep)))
				printf("SRV rt_Reply() failed\n");
		}

		printf("SRV Loop %d\n", count );
//		if(count == 8) count /= i; 	// Divide by zero
		if(count == 8) *zpt = -1; 	// Bad pointer
		if(count == 12) break;
	}

	if(rt_Name_detach(my_pid)) 
		printf("SRV cannot detach name\n");

	rt_sleep(nano2count(1000000000));

	if(rt_task_delete(srv))
		printf("SRV cannot delete task\n");

        stop_rt_timer();

	exit(0);
}
