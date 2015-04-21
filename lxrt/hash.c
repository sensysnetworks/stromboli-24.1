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

$Id: hash.c,v 1.1.1.1 2004/06/06 14:02:32 rpm Exp $ 
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
#include "msg.h"

extern spinlock_t qipc_lock;
spinlock_t qipc_lock = SPIN_LOCK_UNLOCKED;

struct t_cell {
	pid_t	 pid;		/* eth0 vc, eth1 vc, arcnet vc, pid */
	RT_TASK *task;		/* RT_TASK pointer */
	struct t_cell *next;	/* pointer to next cell if collision */
	};

#define		           HASH_MAX	 251	/* Must be a prime number */
static struct t_cell *hash[HASH_MAX];

static inline int hash_idx( RT_TASK *pt )
{
	return (((unsigned int) pt) % HASH_MAX);
}

static struct t_cell *add_cell( pid_t pid, RT_TASK *task )
{
	struct t_cell *pt = 0;

	pt = rt_malloc(sizeof(struct t_cell));
	if(!pt) return 0 ;
	pt->pid  =  pid;
	pt->task = task;
	pt->next =    0;

	return( pt );
}

static int LinkCell( pid_t pid, RT_TASK *task, struct t_cell *from )
{

	/* find where the chain ends */
	while( from->next ) from=from->next;
	from->next = add_cell( pid, task );
	return from->next ? 0 : -1 ;
}

int link_task( pid_t pid, RT_TASK *task )
{
	int hai, err = -ENOMEM ;

	if( !task ) {
		return err ;
	}

	hai = hash_idx( task );

	if( !hash[ hai ] ) {
		hash[ hai ] = add_cell( pid, task );
		err = hash[ hai ] ? 0 : -ENOMEM ;
	}
	else {
		/* collision */
		err = LinkCell( pid, task, hash[ hai ] );
		}

	return err;
}

int unlink_task( pid_t pid, RT_TASK *task )
{
	struct t_cell *pt, *prev;
	int hai;

	hai  = hash_idx( task );
	pt   = hash[hai];
	prev = 0;
	while(pt) {
		if(pt->task == task && pt->pid == pid) {
			if(!prev) hash[hai] = pt->next; else prev->next = pt->next;
			rt_free(pt);
			return 0;
		} else {
			prev = pt;
			pt = pt->next;
		}
	}

	return -ESRCH ;
}

pid_t rttask2pid( RT_TASK *task )
{
	struct t_cell *next;
	int hai;
	unsigned long flags;

	if(!task) return 0;

	flags = rt_spin_lock_irqsave(&qipc_lock);

	hai  = hash_idx( task );
	next = hash[hai];
	while( next ) {
		if(next->task == task) {
			break ;
		}
		next = next->next;
	}

	rt_spin_unlock_irqrestore(flags, &qipc_lock);

	return next ? next->pid : 0 ;
}
