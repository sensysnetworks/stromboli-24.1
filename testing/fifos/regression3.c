////////////////////////////////////////////////////////////////////////////////
//
// Copyright © 2000 Lineo Inc
//
// Authors:		Stuart Hughes 
// Original date:	Oct 2000
// Id:			@(#)$Id: regression3.c,v 1.1.1.1 2004/06/06 14:03:31 rpm Exp $
//
// Description:		Test read select with indefinite timeout
//			
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////
#define FROM_KERN	1
#define BUFSIZE		512
#define FREQ            1
#define NTASKS          1

///////////////////////////////////////////////////////////////////////////////
//
// kernel module code
//
///////////////////////////////////////////////////////////////////////////////
#ifdef __KERNEL__

#include <rtai_fifos.h>
#include <rt/rt_compat.h>

void *send_data(void);
struct task_data td[NTASKS] = {
        {
                {0},                    // task structure
                send_data,              // function
                0,                      // function arg
                3000,                   // stack size
                4,                      // priority 
                1,                      // task period = n * BASE_PER
                0,                      // uses_fpu
                0                       // signal handler
        }
};

void *send_data(void) {
    int i = 0;
    char buf[20];

    while(1) {
        sprintf(buf, "%d", i);
        rtf_put(FROM_KERN, buf, 4);
        i++;
	//printk("put %s\n", buf);
        rt_task_wait_period();
    }
}

int init_module(void)
{
    int err, i;

    if( (err = rtf_create(FROM_KERN, 1000)) < 0 ) {
    	printk("rtf_create: %d, errno = %d\n", FROM_KERN, err);
        return err;
    }
    for(i = 0; i < NTASKS; i++) {
        if( (err = rt_task_create(&td[i])) ) {
            return err;
        }
    }
    return 0;
}

void cleanup_module(void)
{
    int i;

    rt_timer_stop();

    // kill tasks
    for(i = 0; i < NTASKS; i++) {
        rt_task_del(&td[i].task);
    }
    rtf_destroy(FROM_KERN);
}

///////////////////////////////////////////////////////////////////////////////
//
// user process
//
///////////////////////////////////////////////////////////////////////////////

#else

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>

int mypid	= 0;
int run		= 1;
int any_err     = 0;

void timeout(int sig) 
{ 
    any_err	= 1; 
    run		= 0;
}

void cleanup(int sig)
{
    signal(sig, cleanup);
    run = 0;
}

void *readproc(void *arg) 
{
    int from_kern, retval, n;
    //int i = 0, timeouts = 0, bad_data = 0;
    fd_set rfds;
    //struct timeval tv;
    char tmp_buf[BUFSIZE];

    sprintf(tmp_buf, "/dev/rtf%d", FROM_KERN);
    if((from_kern = open(tmp_buf, O_RDONLY)) < 0) {
	fprintf(stderr, "open(%s) : %s\n", tmp_buf, strerror(errno));
	exit(1);
    }

    while(run) {
	FD_ZERO(&rfds);
	FD_SET(from_kern, &rfds);
	//tv.tv_sec = 0;
	//tv.tv_usec = 500000;
	//retval = select(from_kern+1, &rfds, NULL, NULL, &tv);
	alarm(2);
	retval = select(from_kern+1, &rfds, NULL, NULL, NULL);
	alarm(0);
	switch(retval) {
	    case 0:
		fprintf(stderr, "select timeout\n");
		break;
	    case -1:
		//fprintf(stderr, "select error: %s\n", strerror(errno));
		break;
	    default:
		if(FD_ISSET(from_kern, &rfds) == 0) {
		    fprintf(stderr, "FD_ISSET returned unknown fd\n");
		    break;
		}
		if( (n = read(from_kern, tmp_buf, BUFSIZE-1)) < 0 ) {
		    fprintf(stderr, "read : %s\n", strerror(errno));
		    break;
		}
		tmp_buf[n] = 0;
		//fprintf(stderr, "%s:\n", tmp_buf);
		if( atoi(tmp_buf) > 2 ) {
		    run = 0;
		}
		break;
	}
    }
    close(from_kern);

    return NULL;
}

int main(int argc, char *argv[])
{
    //int err;

    if( system("/sbin/insmod ./regression3_mod.o") ) {
        fprintf(stderr, "can't insert regression3_mod.o, bailing out\n");
        exit(1);
    }

    // install handlers to cleanly kill system
    signal(SIGALRM, timeout);
    signal(SIGINT,  cleanup);
    signal(SIGTERM, cleanup);


    fprintf(stderr,
		"\n\nFIFO select read regression tests, indefinite timeout\n"
		    "-----------------------------------------------------\n");
    
    mypid = getpid();
    readproc(NULL);
    system("/sbin/rmmod regression3_mod");

    printf("%-60s : %s\n", "FIFO select read test, indefinite timeout ",
			  any_err ? "failed" : "passed");
    exit(0);
}

#endif
