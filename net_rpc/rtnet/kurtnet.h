/*
COPYRIGHT (C) 2001  Paolo Mantegazza (mantegazza@aero.polimi.it)

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
