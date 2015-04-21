/*
Copyright (C) 2002 Lorenzo Dozio (dozio@aero.polimi.it)

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/kernel.h>
#include <linux/module.h>

#include <rtai.h>
#include <rtai_lxrt.h>
#include "ni_e.h"
#include "ni_e_lxrt.h"

#define MODULE_NAME "ni_e_lxrt"

static struct rt_fun_entry ni_e_fun[] = {
	[_NI_E_AI_CALIBRATE] = { 0, ni_e_AI_calibrate },
	[_NI_E_AO_CALIBRATE] = { 0, ni_e_AO_calibrate },
	[_NI_E_CLEAR_CONFIGURATION_FIFO] = { 0, ni_e_clear_Configuration_FIFO },
	[_NI_E_CLEAR_AI_FIFO] = { 0, ni_e_clear_AI_FIFO },
	[_NI_E_AITM_INIT] = { 0, ni_e_AITM_init },
	[_NI_E_AI_CONFIGURE] = { 0, ni_e_AI_configure },
	[_NI_E_AI_START] = { 0, ni_e_AI_start },
	[_NI_E_AI_READ] = { 0, ni_e_AI_read },
	[_NI_E_AOTM_INIT] = { 0, ni_e_AOTM_init },
	[_NI_E_AO_RESET] = { 0, ni_e_AO_reset },
	[_NI_E_AO_CONFIGURE] = { 0, ni_e_AO_configure },
	[_NI_E_AO_WRITE] = { 0, ni_e_AO_write },
	[_NI_E_AI_RESET] = { 0, ni_e_AI_reset },
	[_NI_E_AI_INIT] = { 0, ni_e_AI_init },
	[_NI_E_DAQSTC_DIO_CONFIGURE] = { 0, ni_e_daqstc_DIO_configure },
	[_NI_E_DAQSTC_DO_WRITE_BIT] = { 0, ni_e_daqstc_DO_write_bit },
	[_NI_E_DAQSTC_DO_RESET] = { 0, ni_e_daqstc_DO_reset },
	[_NI_E_DAQSTC_DI_READ_BIT] = { 0, ni_e_daqstc_DI_read_bit },
	[_NI_E_8255_DIO_CONFIGURE] = { 0, ni_e_8255_DIO_configure },
	[_NI_E_8255_DO_WRITE_BYTE] = { 0, ni_e_8255_DO_write_byte },
	[_NI_E_8255_DI_READ_BYTE] = { 0, ni_e_8255_DI_read_byte }
};

int init_module(void)
{
	if (set_rt_fun_ext_index(ni_e_fun, FUN_EXT_NI_E)) {
		rt_printk("%d is a wrong index module for lxrt.\n", FUN_EXT_NI_E);
		return -EACCES;
	}			
	printk("%s extension loaded.\n", MODULE_NAME);
	return 0;
}

void cleanup_module(void)
{
	reset_rt_fun_ext_index(ni_e_fun, FUN_EXT_NI_E);
	printk("%s extension unloaded.\n", MODULE_NAME);
}
