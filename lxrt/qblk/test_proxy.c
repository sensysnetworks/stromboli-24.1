
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
#include <malloc.h>

#include <rtai_lxrt_user.h>

#define OPT_MINUS(x)    (('-' << 8) + x)

static char msg[512];
QBLK *Main, *End;
QHOOK *Shutdown;
int cnt;
RTIME period;
pid_t proxy;

static inline int my_ss(void)
{
	int reg;
	__asm__("movl %%ss,%%eax " : "=a" (reg) : );
	return reg;
}

static inline int my_ds(void)
{
	int reg;
	__asm__("movl %%ds,%%eax " : "=a" (reg) : );
	return reg;
}

static inline int my_cs(void)
{
	int reg;
	__asm__("movl %%cs,%%eax " : "=a" (reg) : );
	return reg;
}

static void ToDoBeforeExit( void *var1, int evn)
{
	rtai_print_to_screen( "QBLK says: Job %d\n", evn);
}

static void SomeOtherThingsTodo( void *var1, int var2 )
{
	QBLK *q;

	rtai_print_to_screen( "\nQBLK says: Hello World cs %X ss %X ds %X\n", my_cs(), my_ss(), my_ds());
	cnt++;

    rtai_print_to_screen( "\nQBLK says: Hello World cs %X ss %X ds %X\n", my_cs(), my_ss(), my_ds());
 
	q = rt_qDynInit(0, ToDoBeforeExit, (void *)0, cnt);

	rt_qBlkAtHead( q, Shutdown);
    rt_qBlkAtTail( q, Shutdown);
    rt_qBlkCancel( q );
    q = rt_qDynInit(0, ToDoBeforeExit, (void *)0, cnt);
    rt_qBlkAtHead( q, Shutdown);
}

static void NoNotAgain( void *var1, int var2 )
{
	rtai_print_to_screen( "!");
}

static void YetAnotherOne( void *var1, int var2 )
{
	QBLK *q;
 
	rtai_print_to_screen( "+");
	// Scheduled by rt_qBlkWait(), goes back to the free list automatically.
	q = rt_qDynInit(0, NoNotAgain, (void *)0, 0);
	rt_qBlkSchedule( q, nano2count(10000));
}

static void TheEnd( void *var1, int var2 )
{
// Function rt_qReceive() below waits for messages and executes qBlk's.
// When cnt reaches 5, rt_qReceive() will receive the proxy message. 

	rtai_print_to_screen( "." );
	rt_qBlkRepeat( End, nano2count(50000000));
	if(cnt > 2) {
		rtai_print_to_screen( "\nQBLK says: Stop the show\n" );
		rt_Trigger(proxy);
	}
	else rt_qBlkWait(rt_qDynInit( 0, YetAnotherOne, 0, 0), nano2count(40000000));
}

int main(int argc, char *argv[])
{
	unsigned long srv_name = nam2num("SRV");
	RT_TASK *srv;
	pid_t pid, my_pid ;
	int query, softhard, err, i,/* count,*/ msglen ;
	char *opt;
	
	query = softhard = 0 ;

	for( i=1 ; i < argc ; i++ ) { 
		opt = argv[i];
		switch((*opt<<8) + *(opt+1)) {

			case OPT_MINUS('h'):
				softhard++;
				break;

			case OPT_MINUS('q'):
				query++;
				break;
		}
	}

	init_linux_scheduler(SCHED_FIFO, 98);

	err = lock_all(0,0);
	if(err) {
		printf("lock_all(stack,heap) failed\n");
		exit(2);
	}

	if(query) {
		printf("LXRT HRT (!=0) POSIX SOFT REAL TIME (==0):");
		scanf("%d", &softhard);
	}


	printf("SRV starts %s\n", softhard ? "HRT" : "SOFT" );

 	if (!(srv = rt_task_init(srv_name, 0, 0, 0))) {

		printf("CANNOT INIT SRV TASK\n");
		exit(3);
	}

	period = nano2count(10000000);
	rt_set_oneshot_mode();
	start_rt_timer(period);

	rt_InitTickQueue();

	if(!rt_qHookInit(&Shutdown, 0, 0)) {
		printf("rt_qHookInit() fails\n");
		exit(4);
	}	
	
	rt_qDynAlloc(32);

	rt_qDynInit( &Main, SomeOtherThingsTodo, 0, 0);
    End  = rt_qDynInit( 0, TheEnd, 0, 0);

	my_pid = rt_Alias_attach("");
	if(!my_pid) {
		rtai_print_to_screen("Cannot attach name\n");
		exit(5);
	}

	printf("SRV starts, qBlk %p cs %X ss %X lnk %p\n", Main, my_cs(), my_ss(), &Main);

	if (softhard) rt_make_hard_real_time();

	proxy = rt_Proxy_attach(0, 0, 0, -1);

	rt_qBlkRepeat( Main, nano2count(1000000000));
	rt_qBlkRepeat(  End, nano2count(1000000000));

	for(;;) {
 		pid = rt_qReceive( 0, msg, sizeof(msg), &msglen);

		if(pid == proxy) {
			rtai_print_to_screen("\nReceived the proxy :-)\n");
			rt_qBlkCancel(Main);
			rt_qBlkCancel(End);
			break;
		}
		else rt_Reply( pid, 0, 0);
	}

//	rt_sleep(nano2count(1000000));
//Exit:
	if (softhard) rt_make_soft_real_time();

	rt_qLoop();
	rtai_print_to_screen("Flush the Shutdown queue\n");
	rt_qHookFlush(Shutdown);
	rt_qLoop();

	rt_qHookRelease(Shutdown);
//	rt_qDynFree(-1);	
//	rt_ReleaseTickQueue();
	rt_Proxy_detach(proxy);
	rt_Name_detach(my_pid);
	if(rt_task_delete(srv))
		rtai_print_to_screen("SRV cannot delete task\n");

	stop_rt_timer();

	return 0;
}
