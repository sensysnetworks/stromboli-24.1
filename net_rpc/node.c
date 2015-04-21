/*
COPYRIGHT (C) 2001  Paolo Mantegazza (mantegazza@aero.polimi.it),

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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/param.h>

#include <net_rpc.h>

static int get_local_node(char hostname[])
{
	struct hostent *hostinfo;
	gethostname(hostname, MAXHOSTNAMELEN);
	hostinfo = gethostbyname(hostname);
	return ((struct in_addr *)hostinfo->h_addr)[0].s_addr;
}

unsigned long test_ddn2nl(const char *ip)
{
	int p, n, c;
	union { unsigned long l; char c[4]; } u;

	p = n = 0;
	while ((c = *ip++)) {
		if (c != '.') {
			n = n*10 + c - '0';
		} else {
			if (n > 0xFF) {
				return 0;
			}
			u.c[p++] = n;
			n = 0;
		}
	}
	u.c[3] = n;

	return u.l;
}

int main(void)
{
	char hostname[MAXHOSTNAMELEN];
	struct sockaddr_in addr;
	printf("CHECKING USE OF: gethostname, gethostbyname\n");
	addr.sin_addr.s_addr = get_local_node(hostname);
	printf("%x %x %s %s\n", addr.sin_addr.s_addr, get_local_node(hostname), hostname, inet_ntoa(addr.sin_addr));
	printf("CHECKING OUR DOTTED DECIMAL NOTATION (DDN) TRANSLATION\n");
while (1) {
	printf("GIVE A DDN: \n");
	scanf("%s", hostname);
	inet_aton(hostname, &addr.sin_addr);
	printf("SYS: %x, OURS: %lx\n", addr.sin_addr.s_addr, test_ddn2nl(hostname));
}
	return 0;
}
