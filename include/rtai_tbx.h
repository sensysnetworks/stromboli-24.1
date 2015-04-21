/*
COPYRIGHT (C) 2001  G.M. Bertani (gmbertani@yahoo.it)
                    P. Mantegazza (mantegazza@aero.polimi.it), just for its
		    extensions to LXRT.

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


#ifndef _RTAI_TBX_H_
#define _RTAI_TBX_H_

#ifdef INTERFACE_TO_LINUX
#define RT_LINUX_PRIORITY 0x7fffFfff
#endif

#define TBXIDX  12

/* TYPED MAILBOXES */

#define RT_TBX_MAGIC 0x6e93ad4b

#define TYPE_NONE      0x00
#define TYPE_NORMAL    0x01
#define TYPE_BROADCAST 0x02
#define TYPE_URGENT    0x04

#define TBX_INIT	 	 0
#define TBX_DELETE	 	 1
#define TBX_SEND 	 	 2
#define TBX_SEND_IF	 	 3
#define TBX_SEND_UNTIL 	 	 4
#define TBX_SEND_TIMED 	 	 5
#define TBX_RECEIVE	 	 6
#define TBX_RECEIVE_IF		 7
#define TBX_RECEIVE_UNTIL 	 8
#define TBX_RECEIVE_TIMED 	 9
#define TBX_BROADCAST		10
#define TBX_BROADCAST_IF	11
#define TBX_BROADCAST_UNTIL	12
#define TBX_BROADCAST_TIMED	13
#define TBX_URGENT		14
#define TBX_URGENT_IF		15
#define TBX_URGENT_UNTIL	16
#define TBX_URGENT_TIMED	17

#ifdef __KERNEL__

struct rt_typed_mailbox {
	int magic;
	int waiting_nr;   /* number of tasks waiting for a broadcast */
	SEM sndsmx, rcvsmx;
	SEM bcbsmx;       /* binary sem needed to wakeup the sleeping tasks 
                             when the broadcasting of a message is terminated */
	RT_TASK *waiting_task;
	char *bufadr;     /* mailbox buffer */
	char *bcbadr;     /* broadcasting buffer */
	int size;         /* mailbox size */
	int fbyte;        /* circular buffer read pointer */
	int avbs;         /* bytes occupied */
	int frbs;         /* bytes free */
	spinlock_t buflock;
};

typedef struct rt_typed_mailbox TBX;

#include <rtai_sched.h>
#include <rtai_lxrt.h>
#include <asm/rtai_lxrt.h>

/*
 * send_wp and receive_wp are not implemented because 
 * the packed message must be sent/received atomically
 */ 

extern int rt_tbx_init(TBX *tbx, int size, int flags);  
extern int rt_tbx_delete(TBX *tbx);
extern int rt_tbx_send(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_send_if(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_send_until(TBX *tbx, void *msg, int msg_size, RTIME time);
extern int rt_tbx_send_timed(TBX *tbx, void *msg, int msg_size, RTIME delay);
extern int rt_tbx_receive(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_receive_if(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_receive_until(TBX *tbx, void *msg, int msg_size, RTIME time);
extern int rt_tbx_receive_timed(TBX *tbx, void *msg, int msg_size, RTIME delay);

extern int rt_tbx_broadcast(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_broadcast_if(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_broadcast_until(TBX *tbx, void *msg, int msg_size, RTIME time);
extern int rt_tbx_broadcast_timed(TBX *tbx, void *msg, int msg_size, RTIME delay);

extern int rt_tbx_urgent(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_urgent_if(TBX *tbx, void *msg, int msg_size);
extern int rt_tbx_urgent_until(TBX *tbx, void *msg, int msg_size, RTIME time);
extern int rt_tbx_urgent_timed(TBX *tbx, void *msg, int msg_size, RTIME delay);

#else

#include <rtai_nam2num.h>
#include <rtai_declare.h>
#include <rtai_lxrt.h>

struct rt_typed_mailbox;
typedef struct rt_typed_mailbox TBX;

DECLARE TBX *rt_tbx_init(unsigned long name, int size, int flags)
{
	struct { unsigned long name; int size; int flags; } arg = { name, size, flags };
	return (TBX *)rtai_lxrt(TBXIDX, SIZARG, TBX_INIT, &arg).v[LOW];
}

DECLARE int rt_tbx_delete(TBX *tbx)
{
	struct { TBX *tbx; } arg = { tbx };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_DELETE, &arg).i[LOW];
}

DECLARE int rt_tbx_send(TBX *tbx, void *msg, int msg_size)
{
	struct { TBX *tbx; void *msg; int msg_size; } arg = { tbx, msg, msg_size };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_SEND, &arg).i[LOW];
}

DECLARE int rt_tbx_send_if(TBX *tbx, void *msg, int msg_size)
{
	struct { TBX *tbx; void *msg; int msg_size; } arg = { tbx, msg, msg_size };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_SEND_IF, &arg).i[LOW];
}

DECLARE int rt_tbx_send_until(TBX *tbx, void *msg, int msg_size, RTIME time)
{
	struct { TBX *tbx; void *msg; int msg_size; RTIME time; } arg = { tbx, msg, msg_size, time };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_SEND_UNTIL, &arg).i[LOW];
}

DECLARE int rt_tbx_send_timed(TBX *tbx, void *msg, int msg_size, RTIME delay)
{
	struct { TBX *tbx; void *msg; int msg_size; RTIME delay; } arg = { tbx, msg, msg_size, delay };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_SEND_TIMED, &arg).i[LOW];
}

DECLARE int rt_tbx_receive(TBX *tbx, void *msg, int msg_size)
{
	struct { TBX *tbx; void *msg; int msg_size; } arg = { tbx, msg, msg_size };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_RECEIVE, &arg).i[LOW];
}

DECLARE int rt_tbx_receive_if(TBX *tbx, void *msg, int msg_size)
{
	struct { TBX *tbx; void *msg; int msg_size; } arg = { tbx, msg, msg_size };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_RECEIVE_IF, &arg).i[LOW];
}

DECLARE int rt_tbx_receive_until(TBX *tbx, void *msg, int msg_size, RTIME time)
{
	struct { TBX *tbx; void *msg; int msg_size; RTIME time; } arg = { tbx, msg, msg_size, time };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_RECEIVE_UNTIL, &arg).i[LOW];
}

DECLARE int rt_tbx_receive_timed(TBX *tbx, void *msg, int msg_size, RTIME delay)
{
	struct { TBX *tbx; void *msg; int msg_size; RTIME delay; } arg = { tbx, msg, msg_size, delay };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_RECEIVE_TIMED, &arg).i[LOW];
}

DECLARE int rt_tbx_broadcast(TBX *tbx, void *msg, int msg_size)
{
	struct { TBX *tbx; void *msg; int msg_size; } arg = { tbx, msg, msg_size };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_BROADCAST, &arg).i[LOW];
}

DECLARE int rt_tbx_broadcast_if(TBX *tbx, void *msg, int msg_size)
{
	struct { TBX *tbx; void *msg; int msg_size; } arg = { tbx, msg, msg_size };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_BROADCAST_IF, &arg).i[LOW];
}

DECLARE int rt_tbx_broadcast_until(TBX *tbx, void *msg, int msg_size, RTIME time)
{
	struct { TBX *tbx; void *msg; int msg_size; RTIME time; } arg = { tbx, msg, msg_size, time };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_BROADCAST_UNTIL, &arg).i[LOW];
}

DECLARE int rt_tbx_broadcast_timed(TBX *tbx, void *msg, int msg_size, RTIME delay)
{
	struct { TBX *tbx; void *msg; int msg_size; RTIME delay; } arg = { tbx, msg, msg_size, delay };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_BROADCAST_TIMED, &arg).i[LOW];
}

DECLARE int rt_tbx_urgent(TBX *tbx, void *msg, int msg_size)
{
	struct { TBX *tbx; void *msg; int msg_size; } arg = { tbx, msg, msg_size };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_URGENT, &arg).i[LOW];
}

DECLARE int rt_tbx_urgent_if(TBX *tbx, void *msg, int msg_size)
{
	struct { TBX *tbx; void *msg; int msg_size; } arg = { tbx, msg, msg_size };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_URGENT_IF, &arg).i[LOW];
}

DECLARE int rt_tbx_urgent_until(TBX *tbx, void *msg, int msg_size, RTIME time)
{
	struct { TBX *tbx; void *msg; int msg_size; RTIME time; } arg = { tbx, msg, msg_size, time };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_URGENT_UNTIL, &arg).i[LOW];
}

DECLARE int rt_tbx_urgent_timed(TBX *tbx, void *msg, int msg_size, RTIME delay)
{
	struct { TBX *tbx; void *msg; int msg_size; RTIME delay; } arg = { tbx, msg, msg_size, delay };
	return rtai_lxrt(TBXIDX, SIZARG, TBX_URGENT_TIMED, &arg).i[LOW];
}

#endif

#endif
