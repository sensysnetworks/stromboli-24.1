/*
COPYRIGHT (C) 2002  Giuseppe Renoldi (giuseppe@renoldi.org)

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
#include <rtai_spdrv_lxrt.h>

#define MODULE_NAME "rtai_spdrv_lxrt"

static struct rt_fun_entry rtai_spdrv_fun[] = {
	[_SPOPEN]              = { 0, rt_spopen},
	[_SPCLOSE]             = { 0, rt_spclose},
	[_SPREAD]              = { 0, rt_spread },
	[_SPEVDRP]             = { 0, rt_spevdrp },
	[_SPWRITE]             = { 0, rt_spwrite },
	[_SPCLEAR_RX]          = { 0, rt_spclear_rx },
	[_SPCLEAR_TX]          = { 0, rt_spclear_tx },
	[_SPGET_MSR]           = { 0, rt_spget_msr },
	[_SPSET_MCR]           = { 0, rt_spset_mcr },
	[_SPGET_ERR]           = { 0, rt_spget_err },
	[_SPSET_MODE]          = { 0, rt_spset_mode },
	[_SPSET_FIFOTRIG]      = { 0, rt_spset_fifotrig },
	[_SPGET_RXAVBS]        = { 0, rt_spget_rxavbs },
	[_SPGET_TXFRBS]        = { 0, rt_spget_txfrbs },
	[_SPSET_THRS]          = { 0, rt_spset_thrs },
	[_SPSET_CALLBACK]      = { 0, rt_spset_callback_fun_usr },
	[_SPSET_ERR_CALLBACK]  = { 0, rt_spset_err_callback_fun_usr },
	[_SPWAIT_USR_CALLBACK] = { UW1(2, 3), rt_spwait_usr_callback },
	[_SPREAD_TIMED]        = { UW1(2, 3), rt_spread_timed },
	[_SPWRITE_TIMED]       = { UR1(2, 3), rt_spwrite_timed }
};

/* init module */
int init_module(void)
{
	if (set_rt_fun_ext_index(rtai_spdrv_fun, FUN_EXT_RTAI_SP)) {
		rt_printk("%d is a wrong index module for lxrt.\n", FUN_EXT_RTAI_SP);
		return -EACCES;
	}			
	printk("%s: loaded.\n",MODULE_NAME);
	return(0);
}

/*  cleanup module */
void cleanup_module(void)
{
	reset_rt_fun_ext_index(rtai_spdrv_fun, FUN_EXT_RTAI_SP);
	printk("%s: unloaded.\n", MODULE_NAME);
}
