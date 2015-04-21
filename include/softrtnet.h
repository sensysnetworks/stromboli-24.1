/*
COPYRIGHT (C) 2003  Paolo Mantegazza (mantegazza@aero.polimi.it)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/


#ifndef __SOFTRTNET_H_
#define __SOFTRTNET_H_

#ifdef __KERNEL__

#include <linux/kernel.h>
#include <linux/socket.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/errno.h>
#include <net/ip.h>

#include <rtai.h>

#endif

int soft_rt_bind(int s, struct sockaddr *my_addr, int addrlen);
int soft_rt_close(int s);
int soft_rt_recvfrom(int s, void *buf, int len, unsigned int flags, struct sockaddr *from, int *fromlen);
int soft_rt_sendto(int s, const void *buf, int len, unsigned int flags, struct sockaddr *to, int tolen);
int soft_rt_socket(int domain, int type, int protocol);
int soft_rt_socket_callback(int s, int (*func)(int s,void *arg), void *arg);

#include <net_rpc.h>

#define SENDTO    1
#define RECVFROM  2

struct sock_t { 
	int opnd, sock; 
	struct sockaddr bindaddr; 
	char msg[MAX_MSG_SIZE]; 
	int len, recv; 
	struct sockaddr addr; 
	int addrlen; 
	int (*callback)(int sock, void *arg); 
	void *arg; 
};


#ifdef COMPILE_ANYHOW 
#include <linux/socket.h>
#include <net/ip.h>

/* the hard RTNet external interface */

extern int rt_socket(int domain, int type, int protocol);
extern int rt_bind(int s, struct sockaddr *my_addr, int addrlen);
extern int rt_close(int s);
extern int rt_recvfrom(int s, void *buf, int len, unsigned int flags, struct sockaddr *from, int *fromlen);
extern int rt_sendto(int s, const void *buf, int len, unsigned int flags, struct sockaddr *to, int tolen);
extern int rt_socket_callback(int s, int (*func)(int s, void *arg), void *arg);
#endif

#endif
