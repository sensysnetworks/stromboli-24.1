
/*
 * (C) Pierre Cloutier <pcloutier@PoseidonControls.com> LGPLv2
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

QBLK *Main;
int cnt;
RTIME period;

static inline int my_ss(void)
{
        int reg;
        __asm__("movl %%ss,%%eax " : "=a" (reg) : );
        return reg;
}

static inline int my_cs(void)
{
        int reg;
        __asm__("movl %%cs,%%eax " : "=a" (reg) : );
        return reg;
}

static inline int my_ds(void)
{
        int reg;
        __asm__("movl %%ds,%%eax " : "=a" (reg) : );
        return reg;
}

static void SomeOtherThingsTodo( void *var1, int var2 )
{
	// Function rt_qLoop() below waits and executes qBlk's until the queue is empty.
	rtai_print_to_screen( "QBLK Says: Hello World cs %X ss %X ds %X\n", my_cs(), my_ss(), my_ds());

	cnt++;

	if ( cnt == 4 ) {
       		rtai_print_to_screen( "rt_qBlkRepeat() reenters lxrt.\n");
        	rt_qBlkRepeat( Main, nano2count(500000000));
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
	/* pid_t pid, my_pid ; */
	int err, i /*, count, msglen */;

        init_linux_scheduler(SCHED_FIFO, 98);

        err = lock_all(0,0);
        if(err) {
                printf("lock_all() failed\n");
                exit(2);
        }
 
        printf("SRV starts 0 pid %d\n", getpid());

 	if (!(srv = rt_task_init(srv_name, 0, 0, 0))) {
		printf("CANNOT INIT SRV TASK\n");
		exit(3);
	}

	printf("SRV (%p) starts 1\n", srv);

	rt_set_oneshot_mode();

	period = nano2count(10000000);
        start_rt_timer(period);

	rt_InitTickQueue();
	
	rt_qDynAlloc(8);

	Main = rt_qDynInit( 0, SomeOtherThingsTodo, 0, 0);

	printf("SRV starts, qBlk %p cs %X ss %X\n", Main, my_cs(), my_ss());

        //rt_task_make_periodic(srv, rt_get_time() + period, period);

	for(i=0; i<1; i++) {	
		rt_qBlkRepeat( Main, nano2count(1000000000));
		rt_qLoop(); // Wait and Execute until queue is empty
		rtai_print_to_screen("rt_qLoop() done: cnt %d\n", cnt);
	}

	rt_sleep(nano2count(1000000000));
        rt_qDynFree(-1);
	rt_ReleaseTickQueue();
	if(rt_task_delete(srv)) {
		rtai_print_to_screen("SRV cannot delete task\n");
	}

       stop_rt_timer();

	return 0;
}
