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

#ifndef _NI_E_LXRT_H_
#define _NI_E_LXRT_H_

#define KEEP_STATIC_INLINE

#include <rtai_declare.h>

#define  FUN_EXT_NI_E	13

#define _NI_E_AI_CALIBRATE		0
#define _NI_E_AO_CALIBRATE		1
#define _NI_E_CLEAR_CONFIGURATION_FIFO	2
#define _NI_E_CLEAR_AI_FIFO		3
#define _NI_E_AITM_INIT			4
#define _NI_E_AI_CONFIGURE		5
#define _NI_E_AI_START			6
#define _NI_E_AI_READ			7
#define _NI_E_AOTM_INIT			8
#define _NI_E_AO_RESET			9
#define _NI_E_AO_CONFIGURE		10
#define _NI_E_AO_WRITE			11
#define _NI_E_AI_RESET			12
#define _NI_E_AI_INIT			13
#define _NI_E_DAQSTC_DIO_CONFIGURE	14
#define _NI_E_DAQSTC_DO_WRITE_BIT	15
#define _NI_E_DAQSTC_DO_RESET		16
#define _NI_E_DAQSTC_DI_READ_BIT	17
#define _NI_E_8255_DIO_CONFIGURE	18
#define _NI_E_8255_DO_WRITE_BYTE	19
#define _NI_E_8255_DI_READ_BYTE		20

#ifndef __KERNEL__

#include <stdarg.h>
#include <rtai_lxrt_user.h>
#include <rtai_lxrt.h>
#include <linux/types.h>

DECLARE void ni_e_AI_calibrate(void)
{
	struct { int val; } arg = { 0 };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AI_CALIBRATE, &arg);
}

DECLARE void ni_e_AO_calibrate(int polarity)
{
	struct { int polarity; } arg = { polarity };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AO_CALIBRATE, &arg);
}

DECLARE void ni_e_clear_Configuration_FIFO(void)
{
	struct { int val; } arg = { 0 };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_CLEAR_CONFIGURATION_FIFO, &arg);
}
				
DECLARE void ni_e_clear_AI_FIFO(void)
{
	struct { int val; } arg = { 0 };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_CLEAR_AI_FIFO, &arg);
}
				
DECLARE void ni_e_AITM_init(__u32 sampling_time_ns, int irq_type, __u32 number_of_scans)
{
	struct {
		__u32 sampling_time_ns;
		int irq_type;
		__u32 number_of_scans;
	} arg = { sampling_time_ns, irq_type, number_of_scans };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AITM_INIT, &arg);
}

DECLARE int ni_e_AI_configure(int Chan, int ChanType, double Gain, int UnipBip, int DitherEn, int LastChan)
{
	struct {
		int Chan;
		int ChanType;
		double Gain;
		int UnipBip;
		int DitherEn;
		int LastChan;
	} arg = { Chan, ChanType, Gain, UnipBip, DitherEn, LastChan };
	return rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AI_CONFIGURE, &arg).i[LOW];
}

DECLARE void ni_e_AI_start(void)
{
	struct { int val; } arg = { 0 };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AI_START, &arg);
}
				
DECLARE void ni_e_AI_read(__s16 *values, int nchan, int irq_type)
{
	__s16 lvalues[nchan];
	struct {
		__s16 *values;
		int nchan;
		int irq_type;
	} arg = { lvalues, nchan, irq_type };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AI_READ, &arg);
	memcpy(values, lvalues, nchan*sizeof(__s16));
}

DECLARE void ni_e_AOTM_init(void)
{
	struct { int val; } arg = { 0 };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AOTM_INIT, &arg);
}
				
DECLARE void ni_e_AO_reset(void)
{
	struct { int val; } arg = { 0 };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AO_RESET, &arg);
}

DECLARE int ni_e_AO_configure(int DACSel, int GroundRef, int ExtRef, int ReGlitch, int BipDac)
{
	struct {
		int DACSel;
		int GroundRef;
		int ExtRef;
		int ReGlitch;
		int BipDac;
	} arg = { DACSel, GroundRef, ExtRef, ReGlitch, BipDac };
	return rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AO_CONFIGURE, &arg).i[LOW];
}

DECLARE void ni_e_AO_write(__s16 data, int channel)
{
	struct {
		__s16 data;
		int channel;
	} arg = { data, channel };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AO_WRITE, &arg);
}
				
DECLARE void ni_e_AI_reset(void)
{
	struct { int val; } arg = { 0 };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AI_RESET, &arg);
}

DECLARE void ni_e_AI_init(void)
{
	struct { int val; } arg = { 0 };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_AI_INIT, &arg);
}

DECLARE int ni_e_daqstc_DIO_configure(int pin, int mode)
{
	struct {
		int pin;
		int mode;
	} arg = { pin, mode };
	return rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_DAQSTC_DIO_CONFIGURE, &arg).i[LOW];
}

DECLARE void ni_e_daqstc_DO_write_bit(int bit, int onoff)
{
	struct {
		int bit;
		int onoff;
	} arg = { bit, onoff };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_DAQSTC_DO_WRITE_BIT, &arg);
}

DECLARE void ni_e_daqstc_DO_reset(void)
{
	struct { int val; } arg = { 0 };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_DAQSTC_DO_RESET, &arg);
}

DECLARE void ni_e_daqstc_DI_read_bit(unsigned char *data, unsigned char pin)
{
	unsigned char ldata[1];
	struct {
		unsigned char *data;
		unsigned char pin;
	} arg = { ldata, pin };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_DAQSTC_DI_READ_BIT, &arg);
	memcpy(data, ldata, sizeof(unsigned char));
}

DECLARE int ni_e_8255_DIO_configure(int port, int mode)
{
	struct {
		int port;
		int mode;
	} arg = { port, mode };
	return rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_8255_DIO_CONFIGURE, &arg).i[LOW];
}

DECLARE void ni_e_8255_DO_write_byte(int port, unsigned char data)
{
	struct {
		int port;
		unsigned char data;
	} arg = { port, data };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_8255_DO_WRITE_BYTE, &arg);
}

DECLARE void ni_e_8255_DI_read_byte(unsigned char *data, int port)
{
	unsigned char ldata[1];
	struct {
		unsigned char *data;
		int port;
	} arg = { ldata, port };
	rtai_lxrt(FUN_EXT_NI_E, SIZARG, _NI_E_8255_DI_READ_BYTE, &arg);
	memcpy(data, ldata, sizeof(unsigned char));
}

#endif

#endif
