/*
COPYRIGHT (C) 2001  G.M. Bertani (gmbertani@yahoo.it)
              2003  P. Mantegazza (mantegazza@aero.polimi.it)

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
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>

#define INTERFACE_TO_LINUX
#include <asm/rtai.h>
#include <rtai_sched.h>
#include <rtai_tbx.h>
#include "../lxrt/registry.h"

#define MODULE_NAME "rtai_tbx_lxrt"

int rt_tbx_init_u(unsigned long name, int size, int flags)
{
	TBX *tbx;
	if (rt_get_adr(name)) {
		return 0;
	}
	if ((tbx = rt_malloc(sizeof(TBX)))) {
		rt_tbx_init(tbx, size, flags);
		if (rt_register(name, tbx, IS_MBX, current)) {
			return (int)tbx;
		} else {
			rt_free(tbx);
		}
	}
	return 0;
}

int rt_tbx_delete_u(TBX *tbx)
{
	if (rt_tbx_delete(tbx)) {
		return -EFAULT;
	}
	rt_free(tbx);
	return rt_drg_on_adr(tbx);
}

static struct rt_fun_entry rt_tbx_fun[] __attribute__ ((__unused__));

static struct rt_fun_entry rt_tbx_fun[] = {
	{ 0, rt_tbx_init_u },	    		//   0
	{ 0, rt_tbx_delete_u },    		//   1
	{ UR1(2, 3), rt_tbx_send }, 		//   2
	{ UR1(2, 3), rt_tbx_send_if },		//   3
	{ UR1(2, 3), rt_tbx_send_until },	//   4
	{ UR1(2, 3), rt_tbx_send_timed },	//   5
	{ UW1(2, 3), rt_tbx_receive },		//   6
	{ UW1(2, 3), rt_tbx_receive_if }, 	//   7
	{ UW1(2, 3), rt_tbx_receive_until }, 	//   8
	{ UW1(2, 3), rt_tbx_receive_timed },	//   9
	{ UR1(2, 3), rt_tbx_broadcast },	//  10
	{ UR1(2, 3), rt_tbx_broadcast_if },	//  11
	{ UR1(2, 3), rt_tbx_broadcast_until },	//  12
	{ UR1(2, 3), rt_tbx_broadcast_timed },	//  13
	{ UR1(2, 3), rt_tbx_urgent },		//  14
	{ UR1(2, 3), rt_tbx_urgent_if },	//  15
	{ UR1(2, 3), rt_tbx_urgent_until },	//  16
	{ UR1(2, 3), rt_tbx_urgent_timed },	//  17
};

int init_module(void)
{
        if (set_rt_fun_ext_index(rt_tbx_fun, TBXIDX)) {
                rt_printk("%d is a wrong index module for lxrt.\n", TBXIDX);
                return -EACCES;
        }
        printk("%s: loaded.\n",MODULE_NAME);
        return(0);
}

void cleanup_module(void)
{
        reset_rt_fun_ext_index(rt_tbx_fun, TBXIDX);
        printk("%s: unloaded.\n", MODULE_NAME);
}
