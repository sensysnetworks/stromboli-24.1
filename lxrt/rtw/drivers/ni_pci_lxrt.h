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

#ifndef _NI_PCI_LXRT_H_
#define _NI_PCI_LXRT_H_

#include <rtai_declare.h>

#define  FUN_EXT_NI_PCI		13

#define _NI_CALIBRATE				0
#define _NI_AI_INIT				1
#define _NI_AI_CONFIGURE			2
#define _NI_AI_READ				3
#define _NI_AI_RESET				4
#define _NI_AI_CLEAR_CONFIGURATION_FIFO		5
#define _NI_AO_INIT				6
#define _NI_AO_CONFIGURE			7
#define _NI_AO_WRITE				8
#define _NI_AO_RESET				9
#define _NI_671x_AO_SET_IMMEDIATE_MODE		10
#define _NI_82C55_CONFIGURE			11
#define _NI_82C55_WRITE_BYTE			12
#define _NI_82C55_READ_BYTE			13
#define _NI_DIO_MODULE_CONFIGURE		14
#define _NI_DIO_MODULE_RESET			15
#define _NI_DIO_MODULE_WRITE_BIT		16
#define _NI_DIO_MODULE_READ_BIT			17
#define _NI_GET_BOARD_LIST_INDEX		18
#define _NI_GET_BOARD_N_AI_CHANNELS		19
#define _NI_GET_BOARD_N_AO_CHANNELS		20
#define _NI_GET_BOARD_DEVICE_ID			21
#define _NI_GET_N_BOARDS			22

#ifndef __KERNEL__

#include <stdarg.h>
#include <rtai_lxrt.h>

DECLARE void ni_calibrate(unsigned int n)
{
	struct { unsigned int n; } arg = { n };
	rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_CALIBRATE, &arg);
}

DECLARE int ni_AI_init(unsigned int n)
{
	struct { unsigned int n; } arg = { n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_AI_INIT, &arg).i[LOW];
}

DECLARE int ni_AI_configure(int Chan, int ChanType, int Gain, int UnipBip, int DitherEn, int LastChan, unsigned int n)
{
	struct { int Chan;
		int ChanType;
		int Gain;
		int UnipBip;
		int DitherEn;
		int LastChan;
		unsigned int n;
	} arg = { Chan, ChanType, Gain, UnipBip, DitherEn, LastChan, n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_AI_CONFIGURE, &arg).i[LOW];
}

DECLARE int ni_AI_read(signed short int *data, int nchan, unsigned int n)
{
	signed short int ldata[nchan];
	int retval;

	struct {
		signed short int *data;
		int nchan;
		unsigned int n;
	} arg = { ldata, nchan, n };
	retval = rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_AI_READ, &arg).i[LOW];
	memcpy(data, ldata, nchan*sizeof(signed short int));
	return retval;
}

DECLARE int ni_AI_reset(unsigned int n)
{
	struct { unsigned int n; } arg = { n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_AI_RESET, &arg).i[LOW];
}

DECLARE int ni_AI_clear_configuration_FIFO(unsigned int n)
{
	struct { unsigned int n; } arg = { n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_AI_CLEAR_CONFIGURATION_FIFO, &arg).i[LOW];
}

DECLARE int ni_AO_init(unsigned int n)
{
	struct { unsigned int n; } arg = { n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_AO_INIT, &arg).i[LOW];
}

DECLARE int ni_AO_configure(unsigned int DACSel, int ExtRef, int ReGlitch, int BipDac, unsigned int n)
{
	struct {
		unsigned int DACSel;
		int ExtRef;
		int ReGlitch;
		int BipDac;
		unsigned int n;
	} arg = { DACSel, ExtRef, ReGlitch, BipDac, n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_AO_CONFIGURE, &arg).i[LOW];
}

DECLARE int ni_AO_write(signed short int data, int chan, unsigned int n)
{
	struct {
		signed short int data;
		int chan;
		unsigned int n;
	} arg = { data, chan, n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_AO_WRITE, &arg).i[LOW];
}

DECLARE int ni_AO_reset(unsigned int n)
{
	struct { unsigned int n; } arg = { n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_AO_RESET, &arg).i[LOW];
}

DECLARE int ni_671x_AO_set_immediate_mode(unsigned int n)
{
	struct { unsigned int n; } arg = { n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_671x_AO_SET_IMMEDIATE_MODE, &arg).i[LOW];
}

DECLARE int ni_82C55_configure(int port, int mode, unsigned int n)
{
	struct {
		int port;
		int mode;
		unsigned int n;
	} arg = { port, mode, n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_82C55_CONFIGURE, &arg).i[LOW];
}

DECLARE int ni_82C55_write_byte(int port, unsigned char data, unsigned int n)
{
	struct {
		int port;
		unsigned char data;
		unsigned int n;
	} arg = { port, data, n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_82C55_WRITE_BYTE, &arg).i[LOW];
}

DECLARE int ni_82C55_read_byte(unsigned char *data, int port, unsigned int n)
{
	unsigned char ldata[1];
	int retval;

	struct {
		unsigned char *data;
		int port;
		unsigned int n;
	} arg = { ldata, port, n };
	retval = rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_82C55_READ_BYTE, &arg).i[LOW];
	memcpy(data, ldata, sizeof(unsigned char));
	return retval;
}

DECLARE int ni_DIO_module_configure(int pin, int mode, unsigned int n)
{
	struct {
		int pin;
		int mode;
		unsigned int n;
	} arg = { pin, mode, n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_DIO_MODULE_CONFIGURE, &arg).i[LOW];
}

DECLARE int ni_DIO_module_reset(unsigned int n)
{
	struct { unsigned int n; } arg = { n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_DIO_MODULE_RESET, &arg).i[LOW];
}

DECLARE int ni_DIO_module_write_bit(int bit, int onoff, unsigned int n)
{
	struct {
		int bit;
		int onoff;
		unsigned int n;
	} arg = { bit, onoff, n };
	return rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_DIO_MODULE_WRITE_BIT, &arg).i[LOW];
}

DECLARE int ni_DIO_module_read_bit(unsigned char *data, unsigned char pin, unsigned int n)
{
	unsigned char ldata[1];
	int retval;

	struct {
		unsigned char *data;
		unsigned char pin;
		unsigned int n;
	} arg = { ldata, pin, n };
	retval = rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_DIO_MODULE_READ_BIT, &arg).i[LOW];
	memcpy(data, ldata, sizeof(unsigned char));
	return retval;
}

DECLARE void ni_get_board_list_index(unsigned short id, int *index)
{
	int lindex[1];

	struct {
		unsigned short id;
		int *index;
	} arg = { id, lindex };
	rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_GET_BOARD_LIST_INDEX, &arg);
	memcpy(index, lindex, sizeof(int));
} 

DECLARE void ni_get_board_n_ai_channels(unsigned short id, int *nchan)
{
	int lnchan[1];

	struct {
		unsigned short id;
		int *nchan;
	} arg = { id, lnchan };
	rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_GET_BOARD_N_AI_CHANNELS, &arg);
	memcpy(nchan, lnchan, sizeof(int));
} 

DECLARE void ni_get_board_n_ao_channels(unsigned short id, int *nchan)
{
	int lnchan[1];

	struct {
		unsigned short id;
		int *nchan;
	} arg = { id, lnchan };
	rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_GET_BOARD_N_AO_CHANNELS, &arg);
	memcpy(nchan, lnchan, sizeof(int));
} 

DECLARE int ni_get_board_device_id(unsigned int n, unsigned short *id)
{
	unsigned short lid[1];
	int retval;

	struct {
		unsigned int n;
		unsigned short *id;
	} arg = { n, lid };
	retval = rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_GET_BOARD_DEVICE_ID, &arg).i[LOW];
	memcpy(id, lid, sizeof(unsigned short));
	return retval;
}

DECLARE void ni_get_n_boards(int *n_boards)
{
	int ln_boards[1];

	struct {
		int *n_boards;
	} arg = { ln_boards };
	rtai_lxrt(FUN_EXT_NI_PCI, SIZARG, _NI_GET_N_BOARDS, &arg);
	memcpy(n_boards, ln_boards, sizeof(int));
}

#endif

#endif
