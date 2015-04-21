////////////////////////////////////////////////////////////////////////////////
//
// Copyright © 2000 Lineo Inc
//
// Authors:		Stuart Hughes 
// Original date:	Oct 2000
// Id:			@(#)$Id: regression2.c,v 1.1.1.1 2004/06/06 14:03:31 rpm Exp $
//
// Description:		Test select with fifo
//			the data flow is:
//	user process 1: writes to to_kern
//	kernel handler: wakes up and copies data to from_kern
//	user process 2: wakes up and reads data from from_kern 
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
#define FROM_KERN 1
#define TO_KERN   2
#define BUFSIZE 512

///////////////////////////////////////////////////////////////////////////////
//
// kernel module code
//
///////////////////////////////////////////////////////////////////////////////
#ifdef __KERNEL__

#include <linux/kernel.h>

#include <rtai_fifos.h>
#include <rt/rt_compat.h>

int handler(unsigned int fifo)
{
    int r, w;
    static char buffer[BUFSIZE];

    if( (r = rtf_get(TO_KERN, buffer, BUFSIZE - 1 )) < 0 )  {
        printk("Error: woken up by fifo %d,  rtf_get returns %d\n", fifo, r);
	return(r);
    }
    buffer[r] = 0;

    if( (w = rtf_put(FROM_KERN , buffer, r )) != r ) {
	printk("rtf_put(%d) != %d for: \"%s\"\n", w, r, buffer);
	return(w);
    }
    return 0;
}

int init_module(void)
{
    int err;

    if( (err = rtf_create(FROM_KERN, 1000)) < 0 ) {
    	printk("rtf_create: %d, errno = %d\n", FROM_KERN, err);
        return err;
    }
    if( (err = rtf_create(TO_KERN, 1000)) < 0 ) {
    	printk("rtf_create: %d, errno = %d\n", TO_KERN, err);
        return err;
    }
    if( (err = rtf_create_handler(TO_KERN, &handler)) ) {
        printk("rtf_create_handler: %d, errno = %d\n", TO_KERN,  err);
        return err;
    }
    rt_mount();
    return 0;
}

void cleanup_module(void)
{
    rtf_destroy(TO_KERN);
    rtf_destroy(FROM_KERN);
    rt_unmount();
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

static int run = 1;
static int count = 0;
static int bad_data = 0;
static int timeouts = 0;

//////////////////////////////////////////////////////////
// signal handler: request an orderly shutdown
//////////////////////////////////////////////////////////
void cleanup(int sig)
{
    signal(sig, cleanup);
    run = 0;
}


void *readproc(void *arg) 
{
    int from_kern, retval, n, i = 0, timeouts = 0, bad_data = 0;
    fd_set rfds;
    struct timeval tv;
    char tmp_buf[BUFSIZE];

    sprintf(tmp_buf, "/dev/rtf%d", FROM_KERN);
    if((from_kern = open(tmp_buf, O_RDONLY)) < 0) {
	fprintf(stderr, "open(%s) : %s\n", tmp_buf, strerror(errno));
	exit(1);
    }

    while(run) {
	FD_ZERO(&rfds);
	FD_SET(from_kern, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 200000;

	// only trigger the writer every other time
	if(i++ % 2) {
	    count++;	
	}	

	retval = select(from_kern+1, &rfds, NULL, NULL, &tv);
	switch(retval) {
	    case 0:
		if( (i%2) == 0 ) {
		    fprintf(stderr, "select unexpected timeout\n");
		    timeouts++;
		}
		break;
	    case -1:
		fprintf(stderr, "select error: %s\n", strerror(errno));
		break;
	    default:
		if(FD_ISSET(from_kern, &rfds) == 0) {
		    fprintf(stderr, "FD_ISSET returned unknown fd\n");
		    break;
		}
		if( (n = read(from_kern, tmp_buf,BUFSIZE-1)) < 0 ) {
		    fprintf(stderr, "read : %s\n", strerror(errno));
		    break;
		}
		tmp_buf[n] = 0;
		//fprintf(stderr, "%s:\n", tmp_buf);
		if(atoi(tmp_buf) != count) {
		    fprintf(stderr, "received incorrect data: %s:\n", tmp_buf);
		    bad_data++;
		}
		break;
	}
    }
    close(from_kern);

    return NULL;
}


int main(int argc, char *argv[])
{
    int to_kern, n, len, countl = 0, err;
    pthread_t thread_id;
    char tmp_buf[BUFSIZE];
    

    if( system("/sbin/insmod ./regression2_mod.o") ) {
        fprintf(stderr, "can't insert regression2_mod.o, bailing out\n");
        exit(1);
    }

    // install handlers to cleanly kill system
    signal(SIGINT,  cleanup);
    signal(SIGTERM, cleanup);


    if( (err = pthread_create(&thread_id, NULL, readproc, NULL)) < 0 ) {
	fprintf(stderr, "pthread_create: %s \n", strerror(err));
	exit(1);
    }

    sprintf(tmp_buf, "/dev/rtf%d", TO_KERN);
    if((to_kern = open(tmp_buf, O_WRONLY)) < 0) {
	fprintf(stderr, "open(%s) : %s\n", tmp_buf, strerror(errno));
        exit(1);
    }
    fprintf(stderr, "\n\nFIFO select read regression tests\n"
		         "---------------------------------\n");

    while(run) {
	if(count > 4) {
	    run = 0;
	    continue;
	} 
	if(count == countl) {
	    usleep(100000);
	    continue;
	}
        countl = count;
	
	sprintf(tmp_buf, "%d", count);
	len = strlen(tmp_buf)+1;
	if( (n = write(to_kern, tmp_buf, len)) != len ) {
	    fprintf(stderr, "short write: %d < %d\n", n, len);
	}
    }
    close(to_kern);
    system("/sbin/rmmod regression2_mod");

    printf("%-60s : %s\n", "FIFO select read test (data)",
			  bad_data ? "failed" : "passed");
    printf("%-60s : %s\n", "FIFO select read test (timeouts)",
			 timeouts ? "failed" : "passed");
    exit(0);
}

#endif
