/*
COPYRIGHT (C) 2000  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)

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

$Id: tid.c,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $ 
*/

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <asm/system.h>
#include <asm/bitops.h>
//#include <asm/spinlock.h>
#include <asm/smp.h>

#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rt_mem_mgr.h>
#include <rtai_lxrt.h>

#include "msg.h"
#include "tid.h"

extern spinlock_t qipc_lock;

//
// This code assigns a task pid or vc.   
// A vc name will be V_XXXX where XXXX is the hexadecimal
// ASCII representation of the value returned by assign_vc().
// 
// A pid name, if not given by the user, will be T_XXXX where XXXX
// is the hexadecimal ASCII representation of the value returned
// by assign_vc().
//
// I.E. for synchronous IPC, names starting with 'V_' and 'T_' are reserved.
//
// Native names cannot be longer that 6 characters. This is a design constraint.
// However, alias names can also be attached. Alias names can be 32 chr$ long. 
// rt_Name_locate() checks for equivalence with the native name first and then
// the alias name if any.
//
// Network names are of the form host_name:task_name for ethernet connections.
// I.E. a name_locate("MyBox:OMX") would create virtual circuits at both
// ends to establish a network connection with task OMX running on host MyBox. 
//
// For QNX 2 ArcNet the names will be of the form [node]task_name. 
// Example pid_t = name_locate("[5]OMX");
//

#pragma pack(1)
static struct t_pid_vc {
	char     *name;// For names or alias that may be more than 6 chr$ long
//      char     *host;// For vc's
	RT_TASK	 *task;
	unsigned char live;
	unsigned char version;
	pid_t	 pid;
	} tab[255] ;

#define MASKPID	0xFF

static pid_t __assign_pid(RT_TASK *task, unsigned int type)
{
int i ;
pid_t pid ;
unsigned long flags;

/*
 * The purpose of this algorithm is to prevent the creation of vc and pid's
 * that may get confused with those of tasks previously deleted.
*/

flags = rt_spin_lock_irqsave(&qipc_lock);
for( pid=0, i=1 ; i<255 ; i++) {
        if(!tab[i].live) {
                tab[i].version++;
                tab[i].version &= ~ARCNVCF;
                tab[i].task = task;
                pid = tab[i].pid = (tab[i].version << 8) | type | i ;
                tab[i].live++;
                tab[i].name = 0;
//              tab[i].host = 0;
//		rt_printk("RT %X qnxpid %04X\n", task, pid);
                break;
                }
        }
link_task( pid, task );
rt_spin_unlock_irqrestore(flags, &qipc_lock);

return pid;
}

pid_t assign_pid(RT_TASK *task)
{
return __assign_pid(task, 0 ); 
}

void *assign_alias_name(pid_t pid, const char *name)
{
unsigned long flags;
int len;

pid &= MASKPID;
if(!pid || (pid==MASKPID)) return 0;

flags = rt_spin_lock_irqsave(&qipc_lock);
if(pid && name ) {
	len = strlen(name) + 1;
        tab[pid].name = rt_malloc(len);
	if(tab[pid].name) memcpy(tab[pid].name, name, len); 
        }
rt_spin_unlock_irqrestore(flags, &qipc_lock);

return tab[pid].name;
}

pid_t alias2pid( const char *name)
{
int i ;
pid_t pid ;
unsigned long flags;

flags = rt_spin_lock_irqsave(&qipc_lock);
for( pid=0, i=1 ; i<255 ; i++) {
	if(tab[i].live && tab[i].name) {
		if(!strcmp(name, tab[i].name)) {
			pid = tab[i].pid;
			break;
			}
		}
	}
rt_spin_unlock_irqrestore(flags, &qipc_lock);

return pid;
}

//void *assign_host(pid_t pid, const char *host)
//{
//unsigned long flags;
//int len;
//
//pid &= MASKPID;
//if(!pid || (pid==MASKPID)) return 0;
//flags = rt_spin_lock_irqsave(&qipc_lock);
//if(pid && host ) {
//        len = strlen(host) + 1;
//        tab[pid].host = rt_malloc(len);
//        if(tab[pid].host) memcpy(tab[pid].host, host, len);
//        }
//rt_spin_unlock_irqrestore(flags, &qipc_lock);
//
//return tab[pid].host;
//}

pid_t assign_vc(RT_TASK *task)
{
return __assign_pid(task, ARCNVCF );
}

pid_t rt_vc_reserve(void)
{
	return __assign_pid( (RT_TASK *) 0xFFFFFFFF, ARCNVCF );
}

static inline int release_vc(pid_t pid)
{
	unsigned long flags;
	int idx, err ;

	idx = pid & MASKPID;
	if(!idx || (idx==MASKPID)) return -ESRCH;

	flags = rt_spin_lock_irqsave(&qipc_lock);

	unlink_task( pid, tab[idx].task );
	tab[idx].live = 0 ;
	tab[idx].task = 0 ;
	if(tab[idx].name) rt_free(tab[idx].name);
	//if(tab[idx].host) rt_free(tab[idx].host);
	tab[idx].name = 0 ;
	//tab[idx].host = 0 ;
	err = 0 ;
 
	rt_spin_unlock_irqrestore(flags, &qipc_lock);
	return err;
}

int rt_vc_release(pid_t pid)
{
	return release_vc(pid);
}

int rt_vc_attach(pid_t pid)
{
	unsigned long flags;
	RT_TASK *task;
	struct task_struct *lnxt;
	int idx, err;
	char name[8];

	// pid was already reserved
	flags = rt_spin_lock_irqsave(&qipc_lock);

	task = (RT_TASK *) 0;
	err = 0 ;
	idx = pid & MASKPID;
	if(!idx || (idx==MASKPID)) err = -ESRCH;

	if( !err ) err = unlink_task( pid, (RT_TASK *) 0xFFFFFFFF );
	if( !err ) err = link_task( pid, task = rt_whoami());
    if( !err && tab[idx].task == (RT_TASK *) 0xFFFFFFFF) tab[idx].task = task ; else err = -ESRCH;

	rt_spin_unlock_irqrestore(flags, &qipc_lock);

	if( !err ) {
		lnxt = task->lnxtsk;
		vc2nam( pid, name);
		strcpy(lnxt->comm, name);
	}

	return err;
}

int is_vc(pid_t pid)
{
pid &= MASKPID;
if(!pid || (pid==MASKPID)) return 0;

return tab[pid].live ? -1 : 0 ;
}

RT_TASK *pid2rttask(pid_t pid)
{
unsigned long flags;
RT_TASK *task=0;

pid &= MASKPID;
if(!pid || (pid==MASKPID)) return task;

flags = rt_spin_lock_irqsave(&qipc_lock);
task  = tab[pid].live ? tab[pid].task : 0;
rt_spin_unlock_irqrestore(flags, &qipc_lock);

return task;
}

void vc2nam(pid_t pid, char *name)
{
int i, c ;

name[0] = 'V';
name[1] = '_';

for( i=5 ; i>1 ; i-- ) { 
	c = pid & 0xf;
	name[i] =  (c <= 9) ? '0' + c : 'A' + c - 10 ;  
	pid >>= 4;
	}
name[6] = 0;
}

void pid2nam(pid_t pid, char *name)
{
int i, c ;

name[0] = 'T';
name[1] = '_';

for( i=5 ; i>1 ; i-- ) {
        c = pid & 0xf;
        name[i] =  (c <= 9) ? '0' + c : 'A' + c - 10 ;
        pid >>= 4;
        }
name[6] = 0;
}

pid_t nam2vc(char *name)
{
pid_t pid ;
int i, c ;

if (!name || name[0] != 'V' || name[1] != '_') {
	return 0 ; // Is not a valid VC name.
	}

for( i=2, pid=0 ; i<6  ; i++ ) {
        c = name[i];
	if(!c) return 0;
    
	if (c >= 'a' && c <= 'z')      c += (10 - 'a');
	else if (c >= 'A' && c <= 'Z') c += (10 - 'A');
	else if (c >= '0' && c <= '9') c -= '0';
        pid = pid * 16 + c;
        }

pid |= ARCNVCF;

return pid ;
}
