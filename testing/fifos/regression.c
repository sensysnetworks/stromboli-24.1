///////////////////////////////////////////////////////////////////////////////
//
// Copyright © 1999 Zentropic Computing LLC.
//
// Authors:            	Stuart Hughes
// 			Ian Soanes
// Contact:            	info@zentropix.com
// Original date:      	Monday November 29th 1999
// Ident:              	@(#)$Id: regression.c,v 1.1 2000/06/11 21:12:29 
// Description:        	This file implements some simple FIFO tests
//			that will give a pass/fail summary on stdout.
//			to run it insert the base RTL/RTAI
//			modules (e.g ../rtai and ./rtai_fifos.o for RTAI),
//			then type: ./regression
//
// License:
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// common user/kernel
//
///////////////////////////////////////////////////////////////////////////////

#include <rtai_fifos.h>
#define FIFO1		"user_FIFO_12345"
#define FIFO2		"kernel_FIFO_345"
#define WFIFO1		"writeff1";
#define MSG		"hello"
#define MSGLEN		6

///////////////////////////////////////////////////////////////////////////////
//
// kernel module
//
///////////////////////////////////////////////////////////////////////////////
#if __KERNEL__

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <rtai_fifos.h>

static int fifo1, fifo2;

int init_module(void)
{
    int err;

    // First test (named) FIFO already created in user space
#ifdef RTAI_RTF_NAMED
    if( (fifo1 = rtf_getfifobyname(FIFO1)) < 0) {
	printk("rtf_getfifobyname: %d\n", fifo1);
	return fifo1;
    }
#else
    fifo1 = 0;
#endif
    if( (err = rtf_put(fifo1, MSG, MSGLEN)) != MSGLEN) {
       printk("rtf_put, expected %d, got %d\n", MSGLEN, err);
    }
    if( (err = rtf_put(fifo1, MSG, MSGLEN)) != MSGLEN) {
       printk("rtf_put, expected %d, got %d\n", MSGLEN, err);
    }
    
    // Now we get to create and test one of our own
#ifdef RTAI_RTF_NAMED
    if( (fifo2 = rtf_create_named(FIFO2)) < 0) {
	printk("rtf_create_named: %d\n", fifo2);
	return fifo2;
    }
#else
    fifo2 = 1;
    if( (err = rtf_create(fifo2, 1000)) < 0) {
	printk("rtf_create: %d\n", err);
	return err;
    }
#endif
    if( (err = rtf_put(fifo2, MSG, MSGLEN)) != MSGLEN) {
       printk("rtf_put, expected %d, got %d\n", MSGLEN, err);
    }
    if( (err = rtf_put(fifo2, MSG, MSGLEN)) != MSGLEN) {
       printk("rtf_put, expected %d, got %d\n", MSGLEN, err);
    }
    return 0;
}

void cleanup_module(void)
{
    rtf_destroy(fifo1);
    rtf_destroy(fifo2);
}

///////////////////////////////////////////////////////////////////////////////
//
// user process
//
///////////////////////////////////////////////////////////////////////////////
#else

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define CHUNKSZ 10

int any_err	= 0;
int rc 		= 0;
void timeout(int sig) { any_err = 1; rc = 1; }
void result(char *msg)
{
    printf("%-60s : %s\n", msg, rc ? "failed" : "passed");
    rc		= 0;
}

int main(void)
{
    int fd, n, then;
    char buf[128], *msg;
    int fifo;
    char dev[16];
    int named_ok;
#ifdef RTAI_RTF_NAMED
    int n_fifos, dup;
    struct rt_fifo_get_info_struct get_info;
    int i,j;
    struct rt_fifo_info_struct *info;
#endif

    signal(SIGALRM, timeout);
    printf("\nRTL/RTAI portable fifo regression tests\n"
	   "---------------------------------------\n");
    system("/sbin/rmmod regression_mod 2>/dev/null");

    // First check if named fifos are available
#ifdef RTAI_RTF_NAMED
    printf("\nChecking for named fifo support...                           : ");
    if( ((fifo = rtf_create_named(FIFO1)) < 0) && (errno == EINVAL) ) {
	printf("NOT AVAILABLE\n");
	named_ok = 0;
	fifo     = 0;
    } else {
	printf("OK\n");
	named_ok = 1;
    }
#else
    named_ok = 0;
    fifo     = 0;
#endif

    // Test user space created fifo
    printf("\nTesting %sfifo created in user space...\n", 
	    named_ok ? "named " : "");
    printf("----------------------------------%s\n", 
	    named_ok ? "------" : "");

#ifdef RTAI_RTF_NAMED
    if (named_ok) {
	msg = "creating named fifo";
	rc = (fifo < 0) ? 1 : 0;
	result(msg);

	if(fifo<0){
		printf("probably already exists\n");
		fifo = rtf_getfifobyname(FIFO1);
	}

	msg = "creating duplicate named fifos is prevented";
	dup = rtf_create_named(FIFO1);
	rc = (dup < 0) ? 0 : 1;
	result(msg);

	msg = "creating ridiculously long named fifos is prevented";
	dup = rtf_create_named("12345678901234567890");
	rc = (dup < 0) ? 0 : 1;
	result(msg);
    }
#endif

    msg = "opening fifo with no data in it doesn't block";
    alarm(2);
    sprintf(dev, "/dev/rtf%d", fifo);
    if( (fd = open(dev, O_RDONLY)) < 0 ) {
	perror(dev);
    }
    rc = (fd < 0) ? 1 : 0;
    result(msg);

    if( system("/sbin/insmod ./regression_mod.o") ) {
	fprintf(stderr, "can't insert regression_mod.o, bailing out\n");
	exit(1);
    }

    msg = "read less than available data doesn't block";
    alarm(2);
    *buf = 0;
    if( (n = read(fd, buf, MSGLEN)) < 0 ) {
	perror("read");
    }
    result(msg);

    msg = "read less than available returns correct amount";
    rc = (n != MSGLEN);
    result(msg);

    msg = "check data read was correct";
    rc = strcmp(buf, MSG);
    result(msg);

    msg = "read more than available data doesn't block";
    alarm(2);
    if( (n = read(fd, buf, sizeof(buf))) < 0 ) {
	perror("read");
    }
    result(msg);

    msg = "read more than available returns correct amount";
    rc = (n != MSGLEN);
    result(msg);

    msg = "read when no data available blocks";
    alarm(3);
    then   = time(0);
    if( (n = read(fd, buf, sizeof(buf))) < 0 ) {
	perror("read");
    }
    rc = ((time(0) - then) < 2) ? 1 : 0;
    result(msg);

    if( close(fd) != 0 ) {
	perror("close");
    }

    msg = "non-blocking read when no data available does not block";
    if( (fd = open(dev, O_RDONLY | O_NONBLOCK)) < 0 ) {
	perror(dev);
    }
    alarm(3);
    then   = time(0);
    if( (n = read(fd, buf, sizeof(buf))) < 0 ) {
	if( errno != EAGAIN) {
		perror("non-blocking read");
	}
    }
    rc = ((time(0) - then) <= 1) ? 0 : 1;
    result(msg);

    if( close(fd) != 0 ) {
	perror("close");
    }

    // Repeat (almost) everything for kernel space created fifo
    printf("\nTesting %sfifo created in kernel space...\n",
	    named_ok ? "named " : "");
    printf("------------------------------------%s\n",
	    named_ok ? "------" : "");

#ifdef RTAI_RTF_NAMED
    if (named_ok) {
	msg = "looking up named fifo";
	if( (fifo = rtf_getfifobyname(FIFO2)) < 0) {
	    perror("rtf_getfifobyname");
	}
	rc = (fifo < 0) ? 1 : 0;
	result(msg);
    } else
#endif
    {
	fifo = 1;
    }

    msg = "opening fifo with data in it doesn't block";
    alarm(2);
    sprintf(dev, "/dev/rtf%d", fifo);
    if( (fd = open(dev, O_RDONLY)) < 0 ) {
	perror(dev);
    }
    rc = (fd < 0) ? 1 : 0;
    result(msg);

    msg = "read less than available data doesn't block";
    alarm(2);
    *buf = 0;
    if( (n = read(fd, buf, MSGLEN)) < 0 ) {
	perror("read");
    }
    result(msg);

    msg = "check data read was correct";
    rc = strcmp(buf, MSG);
    result(msg);

    msg = "read more than available data doesn't block";
    alarm(2);
    if( (n = read(fd, buf, sizeof(buf))) < 0 ) {
	perror("read");
    }
    result(msg);

    msg = "read when no data available blocks";
    alarm(3);
    then   = time(0);
    if( (n = read(fd, buf, sizeof(buf))) < 0 ) {
	perror("read");
    }
    rc = ((time(0) - then) < 2) ? 1 : 0;
    result(msg);

    if( close(fd) != 0 ) {
	perror("close");
    }

    msg = "non-blocking read when no data available does not block";
    if( (fd = open(dev, O_RDONLY | O_NONBLOCK)) < 0 ) {
	perror(dev);
    }
    alarm(3);
    then   = time(0);
    if( (n = read(fd, buf, sizeof(buf))) < 0 ) {
	if( errno != EAGAIN) {
		perror("non-blocking read");
	}
    }
    rc = ((time(0) - then) <= 1) ? 0 : 1;
    result(msg);

#ifdef RTAI_RTF_NAMED
    // Test the get info ioctls
    if (named_ok) {
	printf("\nTesting get info controls...\n");
	printf("-------------------------\n");
	msg = "check GET_N_FIFOS ioctl";
	if( (n_fifos = ioctl(fd, RTF_GET_N_FIFOS, NULL)) < 0) {
	    rc = 1;
	}
	result(msg);

	msg = "check RTF_GET_FIFO_INFO ioctl";
	printf("\nFifos in use (%d in total)...\n", n_fifos);
	printf("Number   Count    Size     Name\n");
	get_info.fifo = 0;
	get_info.n    = CHUNKSZ;
	get_info.ptr  = malloc(CHUNKSZ * sizeof(struct rt_fifo_info_struct));
	while ( (i = ioctl(fd, RTF_GET_FIFO_INFO, &get_info)) ) {
	    for (j = 0; j < i; j++) {
		info = get_info.ptr + j;
		if (info->opncnt) {
		    printf( "%-8d %-8d %-8d %-16s\n", 
			    info->fifo_number,
			    info->opncnt,
			    info->size,
			    info->name
			    );
		}
	    }
	    get_info.fifo += i;
	}
	printf("\n");
	rc = (i < 0) ? 1 : 0;
	result(msg);
	free(get_info.ptr);
    }
#endif
    
    // Finished
    if( close(fd) != 0 ) {
	perror("close");
    }
    system("/sbin/rmmod regression_mod");
    exit(rc);
}

#endif

