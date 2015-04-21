

/*
 *  * (C) Pierre Cloutier <pcloutier@PoseidonControls.com> LGPLv2
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>

#include <rtai_lxrt_user.h>

/*static char msg[512], rep[512];*/

QBLK *Main;
int cnt;
RTIME period;

static int my_ss(void)
{
	int reg;
	__asm__("movl %%ss,%%eax " : "=a" (reg) : );
	return reg;
}

static int my_ds(void)
{
	int reg;
	__asm__("movl %%ds,%%eax " : "=a" (reg) : );
	return reg;
}

static int my_cs(void)
{
	int reg;
	__asm__("movl %%cs,%%eax " : "=a" (reg) : );
	return reg;
}

void SomeOtherThingsTodo( void *var1, int var2 )
{
	// Function rt_qLoop() below waits and executes qBlk's until the queue is empty.

	rtai_print_to_screen( "QBLK Says: Hello World cs %X ss %X ds %X\n", my_cs(), my_ss(), my_ds());

	cnt++;

	if ( cnt == 4 ) {
		rtai_print_to_screen( "rt_qBlkRepeat() reenters lxrt.\n");
		rt_qBlkRepeat( Main, nano2count(50000));
	}
	else if ( cnt == 8) {
		rtai_print_to_screen( "rt_qBlkCancel() reenters lxrt.\n");
		rt_qBlkCancel( Main );
	}
}

int main(int argc, char* argv[])
{
	unsigned long srv_name = nam2num("SRV");
	RT_TASK *srv;
	/*pid_t pid, my_pid ;*/
	int err, i/*, count, msglen */;
	struct sched_param mysched;

	mysched.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1; 
	if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
		puts(" ERROR IN SETTING THE SCHEDULER");
		perror("errno");
		exit(1);
	}       

	err = lock_all(8096,8096);
	if(err) {
		printf("lock_all(stack,heap) failed\n");
		exit(2);
        }

	printf("SRV starts 0 pid %d\n", getpid());

 	if (!(srv = rt_task_init(srv_name, 0, 0, 0))) {
		printf("CANNOT INIT SRV TASK\n");
		exit(3);
	}

	printf("SRV rt_task %p starts\n", srv);

	period = nano2count(10000000);
	rt_set_oneshot_mode();
	start_rt_timer(period);

	rt_InitTickQueue();
	
	rt_qDynAlloc(8);

	Main = rt_qDynInit( 0, SomeOtherThingsTodo, 0, 0);

	printf("SRV starts, qBlk %p cs %X ss %X\n", Main, my_cs(), my_ss());

	rt_make_hard_real_time();

	//rt_task_make_periodic(srv, rt_get_time() + period, period);

	for(i=0; i<1; i++) {	
		rt_qBlkRepeat( Main, nano2count(100000000));
		rt_qLoop(); // Wait and Execute until queue is empty
		rtai_print_to_screen("rt_qLoop() done. cnt %d\n", cnt);
	}

	rt_sleep(nano2count(1000000000));

//Exit:
        rt_make_soft_real_time();
        rt_qDynFree(64);
        rt_ReleaseTickQueue();
	if(rt_task_delete(srv)) {
		rtai_print_to_screen("SRV cannot delete task\n");
	}
	stop_rt_timer();

	return 0;
}
