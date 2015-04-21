/*
    rtnet/rtnet/rtifconfig/rtifconfig.c
    ifconfig replacement for rtner

    rtnet - real-time networking subsystem
    Copyright (C) 1999,2000 Zentropic Computing, LLC

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef unsigned int u32;

#include <rtnet.h>

void help(void);
void do_display(void);
void do_up(int argc,char *argv[]);
void do_down(int argc,char *argv[]);
void do_route(int argc,char *argv[]);
void do_route_display(void);
void do_route_solicit(int argc,char *argv[]);
void do_route_delete(int argc,char *argv[]);


int f;
struct rtnet_config cfg;

int main(int argc,char *argv[])
{
	if(argc<2 || !strcmp(argv[1],"--help"))help();

	f=open("/dev/rtnet",O_RDWR);

	if(f<0){
		perror("/dev/rtnet");
		exit(1);
	}

	memset(&cfg,0,sizeof(cfg));
	strncpy(cfg.if_name,argv[1],15);

	if(argc<3)do_display();

	if(!strcmp(argv[2],"up"))do_up(argc,argv);
	if(!strcmp(argv[2],"down"))do_down(argc,argv);
	if(!strcmp(argv[2],"route"))do_route(argc,argv);
	
	help();

	return 0;
}

void help(void)
{
	fprintf(stderr,"rtifconfig:\n");
	fprintf(stderr,"\trtifconfig <dev>\n");
	fprintf(stderr,"\trtifconfig <dev> up <addr> <mask>\n");
	fprintf(stderr,"\trtifconfig <dev> down\n");
	fprintf(stderr,"\trtifconfig <dev> route\n");
	fprintf(stderr,"\trtifconfig <dev> route solicit <addr>\n");
	fprintf(stderr,"\trtifconfig <dev> route delete <addr>\n");

	exit(1);
}

void do_display(void)
{
	fprintf(stderr,"fixme\n");
	exit(0);
}

void do_up(int argc,char *argv[])
{
	int r;
	struct in_addr addr;

	if(argc<5)help();

	inet_aton(argv[3], &addr);
	cfg.ip_addr=addr.s_addr;
	inet_aton(argv[4], &addr);
	cfg.ip_mask=addr.s_addr;

	cfg.ip_netaddr=cfg.ip_addr&cfg.ip_mask;
	cfg.ip_broadcast=cfg.ip_addr|(~cfg.ip_mask);

	r=ioctl(f,IOC_RT_IFUP,&cfg);
	if(r<0){
		perror("ioctl");
		exit(1);
	}
	exit(0);
}

void do_down(int argc,char *argv[])
{
	int r;

	r=ioctl(f,IOC_RT_IFDOWN,&cfg);
	if(r<0){
		perror("ioctl");
		exit(1);
	}
	exit(0);
}

void do_route(int argc,char *argv[])
{
	if(argc<4)do_route_display();

	if(!strcmp(argv[3],"solicit"))do_route_solicit(argc,argv);
	if(!strcmp(argv[3],"delete"))do_route_delete(argc,argv);

	help();
}

void do_route_display(void)
{
	fprintf(stderr,"fixme\n");
	exit(0);
}

void do_route_solicit(int argc,char *argv[])
{
	int r;
	struct in_addr addr;

	if(argc<5)help();

	inet_aton(argv[4], &addr);
	cfg.ip_addr=addr.s_addr;

	r=ioctl(f,IOC_RT_ROUTE_SOLICIT,&cfg);
	if(r<0){
		perror("ioctl");
		exit(1);
	}
	exit(0);
}

void do_route_delete(int argc,char *argv[])
{
	int r;
	struct in_addr addr;

	if(argc<5)help();

	inet_aton(argv[4], &addr);
	cfg.ip_addr=addr.s_addr;

	r=ioctl(f,IOC_RT_ROUTE_DELETE,&cfg);
	if(r<0){
		perror("ioctl");
		exit(1);
	}
	exit(0);
}

