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


#include <linux/module.h>
#include <linux/kmod.h>

#include <asm/uaccess.h>
#include <asm/semaphore.h>

#include <rtai.h>
#include <rtai_shm.h>
#include "kurtnet.h"

MODULE_LICENSE("GPL");

static DECLARE_MUTEX_LOCKED(mtx);
static spinlock_t sysrq_lock = SPIN_LOCK_UNLOCKED;

static struct sock_t *socks;
static int *runsock;

int rt_socket(int domain, int type, int protocol)
{
	int i;
	for (i = 0; i < (MAX_SOCKS + MAX_STUBS); i++) {
		if (!cmpxchg(&socks[i].opnd, 0, 1)) {
			return i;
		}
	}
	return -1;
}

int rt_close(int sock)
{
	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS)) {
		return socks[sock].opnd = 0;
	}
	return -1;
}

int rt_bind(int sock, struct sockaddr *addr, int addrlen)
{
	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS)) {
		memcpy(&socks[sock].bindaddr, addr, addrlen);
		return 0;
	}
	return -1;
}

int rt_socket_callback(int sock, int (*func)(int sock, void *arg), void *arg)
{
	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS) && func > 0) {
		socks[sock].callback = func;
		socks[sock].arg      = arg;
		return 0;
	}
	return -1;
}

#define MAX_SOCK_SRQ 128
static struct { int srq, in, out, sockindx[MAX_SOCK_SRQ]; } sysrq;

int rt_sendto(int sock, const void *msg, int len, unsigned int sflags, struct sockaddr *to, int tolen)
{
	unsigned long flags;

	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS)) {
		if (len > MAX_MSG_SIZE) {
			len = MAX_MSG_SIZE;
		}
		memcpy(socks[sock].msg, msg, socks[sock].len = len);
		memcpy(&socks[sock].addr, to, tolen);
		flags = rt_spin_lock_irqsave(&sysrq_lock);
		sysrq.sockindx[sysrq.in] = sock;
	        sysrq.in = (sysrq.in + 1) & (MAX_SOCK_SRQ - 1);
		rt_spin_unlock_irqrestore(flags, &sysrq_lock);
		rt_pend_linux_srq(sysrq.srq);
		return 0;
	}
	return -1;
}

int rt_recvfrom(int sock, void *msg, int len, unsigned int flags, struct sockaddr *from, int *fromlen)
{
	if (sock >= 0 && sock < (MAX_SOCKS + MAX_STUBS)) {
		memcpy(msg, socks[sock].msg, socks[sock].recv);
		if (from && fromlen) { 
			memcpy(from, &socks[sock].addr, socks[sock].addrlen);
			*fromlen = socks[sock].addrlen;
		}
		return socks[sock].recv;
	}
	return -1;
}

long long user_rtnet(unsigned int whatever)
{
	int i, arg[2];
        sigset_t signal, blocked;

        copy_from_user(arg, (unsigned int *)whatever, 2*sizeof(unsigned int));
	if (arg[0]) {
		if (sysrq.out == sysrq.in) {
			down_interruptible(&mtx);
        		signal = current->pending.signal;
        		blocked = current->blocked;
		        for (i = 0; i < _NSIG_WORDS; i++) {
		                if (signal.sig[i] & ~blocked.sig[i]) {
                			return -ERESTARTSYS;
		                }
			}		
		}
		if (sysrq.out != sysrq.in) {
			i = sysrq.sockindx[sysrq.out];
               		sysrq.out = (sysrq.out + 1) & (MAX_SOCK_SRQ - 1);
			return i;
		}
		return -1;
	} 
	if (socks[arg[1]].callback) {
		socks[arg[1]].callback(arg[1], socks[arg[1]].arg);
	}
	return 0;
}

void rtai_rtnet(void)
{
	up(&mtx);
}

#define XSTR(x)    #x
#define MYSTR(x)   XSTR(x)
static char *argv[] = { MYSTR(URTNET), NULL };

int init_module(void)
{
	if ((sysrq.srq = rt_request_srq(0xcacca1, rtai_rtnet, user_rtnet)) < 0) {
                printk("KRTNET: no sysrq available.\n");
                return sysrq.srq;
	}
        socks = (struct sock_t *)rtai_kmalloc(0xcacca0, (MAX_SOCKS + MAX_STUBS)*sizeof(struct sock_t) + 2*sizeof(int));
        runsock = (int *)(socks + MAX_SOCKS + MAX_STUBS);
#if 0
	if (call_usermodehelper(argv[0], argv, NULL)) {
		rt_free_srq(sysrq.srq);
		rtai_kfree(0xcacca0);
		printk("FAILED IN SETTING SOFT LINUX SOCKETS SUPPORT\n");
		return 1;
	}
#endif
	return 0;
}

void cleanup_module(void)
{
	if (runsock[0]) {
		runsock[0] = 0;
		up(&mtx);
		while (!runsock[0]) {
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(HZ/10);
		}
	}
	rt_free_srq(sysrq.srq);
	rtai_kfree(0xcacca0);
	return;
}
