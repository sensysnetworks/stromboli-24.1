/*
COPYRIGHT (C) 2000  Giuseppe Renoldi (giuseppe@renoldi.org)

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


#include <linux/kernel.h>
#include <linux/module.h>

#include <rtai.h>
#include <rtai_lxrt.h>
#include <rt_com.h>
#include <rt_com_lxrt.h>

#define MODULE_NAME "rt_com_lxrt"


static struct rt_fun_entry rt_com_fun[] = {
	[_SETUP]        = { 0, rt_com_setup},
	[_HWSETUP]      = { 0, rt_com_hwsetup},
	[_COM_READ]     = { UW1(2, 3), rt_com_read },
	[_COM_WRITE]    = { UR1(2, 3), rt_com_write },
	[_CLEAR_INPUT]  = { 0, rt_com_clear_input },
	[_CLEAR_OUTPUT] = { 0, rt_com_clear_output },
	[_READ_MODEM]   = { 0, rt_com_read_modem },
	[_WRITE_MODEM]  = { 0, rt_com_write_modem },
	[_ERROR]        = { 0, rt_com_error },
	[_SET_MODE]     = { 0, rt_com_set_mode },
	[_SET_FIFOTRIG] = { 0, rt_com_set_fifotrig }
};


/* init module */
int init_module(void)
{
	if (set_rt_fun_ext_index(rt_com_fun,FUN_EXT_RT_COM)) {
		rt_printk("%d is a wrong index module for lxrt.\n",FUN_EXT_RT_COM);
		return -EACCES;
	}			
	printk("%s: loaded.\n",MODULE_NAME);
	return(0);
}


/*  cleanup module */
void cleanup_module(void)
{
	reset_rt_fun_ext_index(rt_com_fun,FUN_EXT_RT_COM);
	printk("%s: unloaded.\n",MODULE_NAME);
}


