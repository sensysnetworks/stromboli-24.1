/*
COPYRIGHT (C) 2000  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)
                    and
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

$Id: msg.h,v 1.1.1.1 2004/06/06 14:02:34 rpm Exp $ 
*/

#ifndef _MSG_H_
#define _MSG_H_

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
int rt_Proxy_detach(pid_t pid);
pid_t rt_Trigger(pid_t pid);

extern pid_t    rttask2pid(RT_TASK *);
extern RT_TASK *pid2rttask(pid_t);

extern pid_t assign_vc(RT_TASK *);
extern pid_t assign_pid(RT_TASK *);

extern pid_t rt_vc_reserve(void);
extern int   rt_vc_release(pid_t);
extern int   rt_vc_attach(pid_t);

extern int   is_vc(pid_t);
extern void  vc2nam(pid_t, char *);
extern pid_t nam2vc(char *);

extern int link_task( pid_t, RT_TASK *);
extern int unlink_task( pid_t, RT_TASK *);

extern int rt_create_vc( pid_t, pid_t, int, int);

#endif // _MSG_H_
