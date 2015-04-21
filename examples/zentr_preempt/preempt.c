////////////////////////////////////////////////////////////////////////////////
//
//  Copyright © 1998 Zentropic Computing
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// ZENTROPIC COMPUTING LLC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of the Zentropic Computing LLC
// shall not be used in advertising or otherwise to promote the sale, use or
// other dealings in this Software without prior written authorization from the
// Zentropic Computing LLC
//
//  Authors:		Stuart Hughes
//  Contact:		info@zentropix.com
//  Original date:	Fri 22 Oct 1999
//  Ident:		@(#)$Id: preempt.c,v 1.1.1.1 2004/06/06 14:01:45 rpm Exp $
//  Description:	This code provides a preemption test that can
//			be used with RTAI and RTL2. 
// 
//  NOTE: Some of the code in this example was derived from the preempt 
//        test distributed with RTAI 0.7 Copyright Paolo Mantegazzao
//	  mantegazza@aero.polimi.it
//
//
////////////////////////////////////////////////////////////////////////////////
static char id_preempt_c[] __attribute__ ((unused)) = "@(#)$Id: preempt.c,v 1.1.1.1 2004/06/06 14:01:45 rpm Exp $";

///////////////////////////////////////////////////////////////////////////////
//
// common user/kernel 
//
///////////////////////////////////////////////////////////////////////////////
#define NSECS_PER_SEC	1000000000
#define FREQ		1000			// basic frequency in Hz
#define BASE_PER	(NSECS_PER_SEC/FREQ)	// basic periodic in seconds
#define LOAD 		830			// basic loop count per Mhz clk
#define NTASKS		2
#define FIFO_NO		0
typedef void *(* VP_FP_VP)(void *);
typedef void  (* V_FP_V  )(void);
typedef void  (* V_FP_I  )(int);
struct msg_ {
	int  task_id;
	char  susres;
	long long time;
};

#undef DBG
#ifdef ZDEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif


///////////////////////////////////////////////////////////////////////////////
//
// kernel module
//
///////////////////////////////////////////////////////////////////////////////
#if __KERNEL__

#    include <linux/module.h>
#    include <linux/kernel.h>
#    include <linux/config.h>
#    include <asm/io.h>
#if defined(CONFIG_RTAI) || defined(CONFIG_RTAI_MODULE)
#define RTAI
#endif

#ifdef RTAI
#    include <rtai.h>
#    include <rtai_sched.h>
#    include <rtai_fifos.h>
#    define TASK_STR		  RT_TASK
#    define rtf_create(num, size) rtf_create_using_bh(num, size, 0)
#    define get_time_ns		  rt_get_cpu_time_ns
#else
#    include <rtl.h>
#    include <rtl_fifo.h>
#    include <rtl_sched.h>
#    include <pthread.h>
#    define TASK_STR		  pthread_t
#    define get_time_ns		  gethrtime
#    define rt_task_wait_period   pthread_wait_np
#endif

MODULE_LICENSE("GPL");
EXPORT_NO_SYMBOLS;

// The processor clock speed in Mhz is normally passed down,
// using this an approximate calculation can be done to make 
// the test used about 70 % of the CPU.  The idea is to stress
// the system and clearly demonstrate whether preemption is
// working as expected
int clkspd = 10;
MODULE_PARM(clkspd, "i");


// task control data
void *func(void *arg);
struct t_dat_ {
    TASK_STR task;
    VP_FP_VP func;
    int arg;
    int stack_size;
    int priority;
    int period;
    int    uses_fpu;
    V_FP_V sig_han;
    long long period_ns;
    long long when_ns;

} td[NTASKS] = {
	    // func    arg   stack prior period uses_fpu sig_han
       {  {0}, func,     0,  3000,    5,      5,       0,     0  },
       {  {0}, func,     1,  3000,    6,      8,       0,     0  },
};

// function prototypes
int rt_task_create( struct t_dat_ *t);

////////////////////////////////////////////////////////////////
// RT module initialisation
///////////////////////////////////////////////////////////////
int init_module(void)
{                   
    int i;
    DBG(printk("clock speed Mhz = %d\n", clkspd);)
    rtf_create(FIFO_NO, 20000);

    for(i = 0; i < NTASKS; i++) {
	rt_task_create( &td[i] );
    }
    return 0;
}
////////////////////////////////////////////////////////////////
// task creation wrapper to keep both parties happy (or not)
///////////////////////////////////////////////////////////////
int rt_task_create( struct t_dat_ *t)
{
#ifdef RTAI
    static int timer_started = 0;

    if( timer_started == 0) {
	// start timer, this defaults to periodic mode
	rt_set_oneshot_mode();
        start_rt_timer(nano2count(BASE_PER));
	timer_started 	= 1;
	DBG(printk("timer started, wait .......\n");)
    }

    rt_task_init(	&t->task,
                   	(V_FP_I)t->func,
                        t->arg,
                        t->stack_size,
                        t->priority,
                        t->uses_fpu,
                        t->sig_han
		);
    t->when_ns 		= rt_get_time_ns() + 1 * NSECS_PER_SEC;
    t->period_ns	= (long long)t->period * BASE_PER;

    rt_task_make_periodic(
			&t->task,
			nano2count(t->when_ns),
			nano2count(t->period_ns)
		);


#else					// RTL 
    struct sched_param p;
    pthread_create(	&t->task,
			NULL,
			t->func,
			(void *)t->arg
		);
    t->when_ns 		= get_time_ns() + 1 * NSECS_PER_SEC;
    t->period_ns	= (long long)t->period * BASE_PER;
    pthread_make_periodic_np(
			t->task,
			t->when_ns,
			t->period_ns
		);
    p.sched_priority = 100 - t->priority;
    pthread_setschedparam(t->task, SCHED_FIFO, &p);

#endif

    return 0;
}

////////////////////////////////////////////////////////////////
// RT thread
///////////////////////////////////////////////////////////////
void *func(void *arg)
{
    unsigned long counter;
    struct msg_ msg;
			
    while (1) {  
	// you have to wait here otherwise RTL (pthreads) will run
	// one pass before the expected startup pause
        rt_task_wait_period();                                        

	msg.task_id	= (int) arg;
        msg.time   	= get_time_ns();
        msg.susres	= 'r'; 
        rtf_put(FIFO_NO, &msg, sizeof(msg)); 
		
	// waste some processor cycles to load the machine
	for(counter = 0; counter < LOAD * clkspd; counter++) {
		counter++;
		counter--;
	}
	msg.task_id	= (int) arg;
        msg.time   	= get_time_ns();
        msg.susres 	= 's'; 
        rtf_put(FIFO_NO, &msg, sizeof(msg)); 

    }
}                                        

////////////////////////////////////////////////////////////////
// RT module cleanup
///////////////////////////////////////////////////////////////
void cleanup_module(void)
{
    int i;
#ifdef RTAI
    stop_rt_timer();
#endif
    for(i = 0; i < NTASKS; i++) {
#ifdef RTAI
	rt_task_suspend(&td[i].task);
	rt_task_delete(&td[i].task);
#else
	pthread_delete_np(td[i].task);
#endif
    }
    rtf_destroy(FIFO_NO);
}

///////////////////////////////////////////////////////////////////////////////
//
// user code
//
///////////////////////////////////////////////////////////////////////////////
#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sched.h>


int main(void)
{
    int fd;
    struct sched_param mysched;
    struct msg_ msg;
    char fifo_dev[20] = "/dev/rtf";

    mysched.sched_priority = 99;

    if( sched_setscheduler( 0, SCHED_FIFO, &mysched ) == -1 ) {
	fprintf(stderr, "sched_setscheduler: %s\n", strerror(errno));
        exit(1);
    }

    sprintf(&fifo_dev[strlen(fifo_dev)], "%d", FIFO_NO);    
    if( (fd = open(fifo_dev, O_RDONLY)) < 0) {
	fprintf(stderr, "open(%s, O_RDONLY): %s\n", fifo_dev, strerror(errno));
        exit(1);
    }

    // print time in units of 0.1 millisecond
    while(1) {
	if( read(fd, &msg, sizeof(msg)) == sizeof(msg) ) {
	    printf("%d\t%c\t%lld\n", 	msg.task_id,
					msg.susres, 
					msg.time/100000);
	    fflush(stdout);
	}

    }
}
#endif
