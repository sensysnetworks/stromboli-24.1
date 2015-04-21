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

#define INTERFACE_TO_LINUX
#include <asm/rtai.h>
#include <rtai_lxrt.h>

//#define JUST_SWITCH

#include "lock_task.h"

extern int rtai_cpu;

int init_module(void)
{
#ifdef JUST_SWITCH
	rt_lock_cpu(1 - rtai_cpu, 0, 0);
#else
	do {
		int irqs[] = IRQs;
		rt_unlock_cpu();
		rt_lock_cpu(1 - rtai_cpu, irqs, sizeof(irqs)/sizeof(int));
	} while (0);
#endif
        return 0;
}

void cleanup_module(void)
{
	return;
}
