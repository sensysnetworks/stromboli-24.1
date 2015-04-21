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
#include "ni_pci.h"
#include "ni_pci_lxrt.h"

#define MODULE_NAME "ni_pci_lxrt"

static struct rt_fun_entry ni_pci_fun[] = {
	[_NI_CALIBRATE] = { 0, ni_calibrate },
	[_NI_AI_INIT] = { 0, ni_AI_init },
	[_NI_AI_CONFIGURE] = { 0, ni_AI_configure },
	[_NI_AI_READ] = { 0, ni_AI_read },
	[_NI_AI_RESET] = { 0, ni_AI_reset },
	[_NI_AI_CLEAR_CONFIGURATION_FIFO] = { 0, ni_AI_clear_configuration_FIFO },
	[_NI_AO_INIT] = { 0, ni_AO_init },
	[_NI_AO_CONFIGURE] = { 0, ni_AO_configure },
	[_NI_AO_WRITE] = { 0, ni_AO_write },
	[_NI_AO_RESET] = { 0, ni_AO_reset },
	[_NI_671x_AO_SET_IMMEDIATE_MODE] = { 0, ni_671x_AO_set_immediate_mode },
	[_NI_82C55_CONFIGURE] = { 0, ni_82C55_configure },
	[_NI_82C55_WRITE_BYTE] = { 0, ni_82C55_write_byte },
	[_NI_82C55_READ_BYTE] = { 0, ni_82C55_read_byte },
	[_NI_DIO_MODULE_CONFIGURE] = { 0, ni_DIO_module_configure },
	[_NI_DIO_MODULE_RESET] = { 0, ni_DIO_module_reset },
	[_NI_DIO_MODULE_WRITE_BIT] = { 0, ni_DIO_module_write_bit },
	[_NI_DIO_MODULE_READ_BIT] = { 0, ni_DIO_module_read_bit },
	[_NI_GET_BOARD_LIST_INDEX] = { 0, ni_get_board_list_index },
	[_NI_GET_BOARD_N_AI_CHANNELS] = { 0, ni_get_board_n_ai_channels },
	[_NI_GET_BOARD_N_AO_CHANNELS] = { 0, ni_get_board_n_ao_channels },
	[_NI_GET_BOARD_DEVICE_ID] = { 0, ni_get_board_device_id },
	[_NI_GET_N_BOARDS] = { 0, ni_get_n_boards }
};

int init_module(void)
{
	if (set_rt_fun_ext_index(ni_pci_fun, FUN_EXT_NI_PCI)) {
		rt_printk("%d is a wrong index module for lxrt.\n", FUN_EXT_NI_PCI);
		return -EACCES;
	}			
	printk("%s extension loaded.\n", MODULE_NAME);
	return 0;
}

void cleanup_module(void)
{
	reset_rt_fun_ext_index(ni_pci_fun, FUN_EXT_NI_PCI);
	printk("%s extension unloaded.\n", MODULE_NAME);
}
