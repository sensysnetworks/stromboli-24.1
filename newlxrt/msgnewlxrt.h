/*
COPYRIGHT (C) 2002  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)
                    Paolo Mantegazza (mantegazza@aero.polimi.it)

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

$Id: msgnewlxrt.h,v 1.1.1.1 2004/06/06 14:02:47 rpm Exp $ 
*/

#ifndef _MSGNEWLXRT_H_
#define _MSGNEWLXRT_H_

// message control block structure
struct t_msgcb {
        int  cmd;
        void *sbuf;
        size_t sbytes;
        void *rbuf;
        size_t rbytes;
        };
typedef struct t_msgcb MSGCB;

// MSGCB Commands
#define SYNCMSG          0
#define PROXY           -1

extern int rt_Send(pid_t pid, void *smsg, void *rmsg, size_t ssize, size_t rsize);
extern pid_t rt_Receive(pid_t pid, void *msg, size_t maxsize, size_t *msglen);
extern pid_t rt_Creceive(pid_t pid, void *msg, size_t maxsize, size_t *msglen, RTIME delay);
extern int rt_Reply(pid_t pid, void *msg, size_t size);

extern pid_t rt_Proxy_attach( pid_t pid, void *msg, int nbytes, int priority);
extern int rt_Proxy_detach(pid_t pid);
extern pid_t rt_Trigger(pid_t pid);

static inline RT_TASK *pid2rttask(pid_t pid)
{
        return find_task_by_pid(pid)->this_rt_task[0];
}

// the following might look strange but it must be so to work with buddies also
static inline pid_t rttask2pid(RT_TASK * task)
{
        return ((struct task_struct *)(task->lnxtsk)->this_rt_task[1])->pid;
}

#define	MAX_NAME_LENGTH	16

pid_t rt_Name_attach(const char *name);
pid_t rt_Name_locate(const char *host, const char *name);
int rt_Name_detach(pid_t pid);

#endif // _MSGNEWLXRT_H_
