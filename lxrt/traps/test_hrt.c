
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
#include <leds_lxrt.h>

#define OPT_MINUS(x)    (('-' << 8) + x)
#define OPT_EQ(x)       ((x << 8) + '=')

static char msg[512], rep[512];
static int query, hrt, crashkernel, crashfunc;

extern void rt_boom(void);

static void YetAnotherOne( void *var1, int var2 )
{
	//QBLK *q;
	int i, *zi;
	zi = (int *)0;
	rtai_print_to_screen( "Crash in qBlk\n");

	if( crashkernel) 
		rt_boom();
	else {
		i   = -1;
		*zi =  i;
	}
}

int main(int argc, char *argv[])
{
	unsigned long srv_name = nam2num("SRV");
	RT_TASK *srv;
	pid_t pid, my_pid ;
	int err, i, count, msglen, *zpt ;
	RTIME period, delay;
	char *opt;
	QBLK *q;
//	int dummy;

	rt_reset_leds(255);

	hrt = 1 ;
	for( i=1 ; i < argc ; i++ ) {
		opt = argv[i];
		switch((*opt<<8) + *(opt+1)) {

			case OPT_MINUS('k'):
				crashkernel++;
				break;

			case OPT_MINUS('f'):
				crashfunc++;
				break;

			case OPT_MINUS('q'):
				query++;
				break;

			case OPT_MINUS('s'):
				hrt--;
				break;

			}
	}

	if(query) {
		printf("CRASH IN KERNEL (!=0) OR USER SPACE (==0): ");
		scanf("%d", &crashkernel);
		printf("CRASH IN QBLK FUNCTION (!=0) OR NORMAL USER SPACE (==0):");
		scanf("%d", &crashfunc);
	}

    init_linux_scheduler(SCHED_FIFO, 98);

	err = lock_all(0,0);
	if(err) {
		printf("lock_all() failed\n");
		exit(2);
	}

	printf("SRV in %s mode to crash %s%s\n", hrt ? "HRT" : "POSIX", crashfunc ? "in qBlk ":"", crashkernel ? "in kernel." : "in user space.");

 	if (!(srv = rt_task_init(srv_name, 0, 0, 0))) {
		printf("CANNOT INIT SRV TASK\n");
		exit(3);
	}

	rt_set_oneshot_mode();

	period = nano2count(10000000);
	start_rt_timer(period);

//	printf("SRV starts\n");

	if (hrt) rt_make_hard_real_time();

	rt_task_make_periodic(srv, rt_get_time() + period, period);

	my_pid = rt_Alias_attach("");
	if(!my_pid) {
		rtai_print_to_screen("Cannot attach name\n");
		goto Exit;
	}

	rt_InitTickQueue();
	rt_qDynAlloc(32);

	rt_sleep(nano2count(1000000000));
	i = count = 0 ;
	zpt = (int *)0;
    while(++count) {
		rt_set_leds(count%256);
		memset( msg, 0, sizeof(msg));
		pid = rt_Creceive( 0, msg, sizeof(msg), &msglen, period);
		if(pid) {
			rtai_print_to_screen("SRV received msg    [%s] %d bytes from pid %04X\n", msg, msglen, pid);

			memcpy( rep, msg, sizeof(rep));

			if(rt_Reply(pid, rep, sizeof(rep)))
				rtai_print_to_screen("SRV rt_Reply() failed\n");
		}

		rtai_print_to_screen("SRV Loop %d\n", count);

		if(count == 10 && !crashfunc) {
			if(!crashkernel)
				*zpt = -1;  // Bad pointer
			else
				rt_boom();
		} else if(count == 10) {
			q = rt_qDynInit( 0, YetAnotherOne, 0, 0);
            delay = nano2count(100000000);
            rt_qBlkWait(q, delay);
			rt_qLoop();		
		}
        if(count == 20) break;
	}

	if(rt_Name_detach(my_pid)) 
		rtai_print_to_screen("SRV cannot detach name\n");

	rt_sleep(nano2count(1000000000));
Exit:
	if (hrt) rt_make_soft_real_time();

	if(rt_task_delete(srv))
		rtai_print_to_screen("SRV cannot delete task\n");

	stop_rt_timer();
	
	return 0;
}
