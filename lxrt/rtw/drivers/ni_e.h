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

#ifndef _NI_E_H_
#define _NI_E_H_

#include <linux/types.h>

#define AO_unipolar	0
#define AO_bipolar	1
#define AO_int_ref	0
#define AO_ext_ref	1

#define AO_Vmax		10.0	/* Volts */

#define AI_unipolar	1
#define AI_bipolar	0

#define AI_gain_05      0.5
#define AI_gain_1       1.0
#define AI_gain_2       2.0
#define AI_gain_5       5.0
#define AI_gain_10      10.0
#define AI_gain_20      20.0
#define AI_gain_50      50.0
#define AI_gain_100     100.0

#define AI_dither_on    1
#define AI_dither_off   0

#define AI_calibration  0
#define AI_differential 1
#define AI_NRSE         2
#define AI_RSE          3
#define AI_Aux          5
#define AI_Ghost        7

#define AI_IRQ_on_nothing	0
#define AI_IRQ_on_SC_TC		1
#define AI_IRQ_on_START1	2
#define AI_IRQ_on_START2	4
#define AI_IRQ_on_START		8
#define AI_IRQ_on_STOP		16
#define AI_IRQ_on_Error		32
#define AI_IRQ_on_FIFO		128

#define NI_E_DAQSTC_DIO_OUTPUT	1
#define NI_E_DAQSTC_DIO_INPUT	0

#define NI_E_8255_OUTPUT	0
#define NI_E_8255_INPUT		1

#define PORT_A_8255		4
#define PORT_C_up_8255		3
#define PORT_B_8255		2
#define PORT_C_low_8255		1

static inline __s16 ni_e_v2i(double fval, int polarity)
{
/*
   polarity = 0 -> AO unipolar mode
   polarity = 1 -> AO bipolar mode
*/
	__s16 ival;
	if (polarity == AO_unipolar) {
		if (fval > AO_Vmax) fval = AO_Vmax;
		if (fval < 0.) fval = 0.;
	} else {
		if (fval > AO_Vmax) fval = AO_Vmax;
		if (fval < -AO_Vmax) fval = -AO_Vmax;
	}
	ival = (__s16)((fval/10.)*(4095./(polarity+1)));
	return ival;
}

static inline double ni_e_i2v(__s16 ival, double gain, int polarity)
{
/*
   polarity = 0 -> AI bipolar mode
   polarity = 1 -> AI unipolar mode
*/
	double fval, scale;

	if (polarity == AI_bipolar) {
		scale = 5./gain;
		fval = ((double)ival/2047.)*scale;
	} else {
		scale = 10./gain;
		fval = ((double)ival/4095.)*scale;
	}
	return fval;
}

#ifdef __KERNEL__

#define VENDOR_ID_NI		0x1093
#define DEVICE_ID_MIO_16XE_50	0x0162
#define DEVICE_ID_MIO_16XE_10	0x1170
#define DEVICE_ID_MIO_16E_1	0x1180
#define DEVICE_ID_MIO_16E_4	0x1190
#define DEVICE_ID_6031E		0x1330
#define DEVICE_ID_6071E		0x1350
#define DEVICE_ID_6025E		0x2a80

#define BOARD_TICK	50

struct ai_subdev {
	int nchan;
	int resol;
	int max_sampling;
};

struct ao_subdev {
	int resol;
};

struct dio_subdev {
	unsigned char CR_mask_8255;
	unsigned char CR_mask_daqstc;
	unsigned char DO_mask_daqstc;
};

struct ni_e_dev {
	char *name;
	unsigned long mite_start;
	unsigned long daq_start;
#if LINUX_VERSION_CODE > 0x020200
	unsigned long mite_end;
	unsigned long daq_end;
#endif
	unsigned int irq_line;
	void *mite_io_addr;
	void *daq_io_addr;
	int iobase;
	struct ai_subdev *ai;
	struct ao_subdev *ao;
	struct dio_subdev *dio;
};

extern void ni_e_AI_calibrate(void);
extern void ni_e_AO_calibrate(int);

extern void ni_e_clear_Configuration_FIFO(void);

extern void ni_e_clear_AI_FIFO(void);
extern void ni_e_AI_init(void);
extern void ni_e_AITM_init(__u32, int, __u32);
extern int ni_e_AI_configure(int, int, double, int, int, int);
extern void ni_e_AI_start(void);
extern void ni_e_AI_read(__s16 *, int, int);
extern void ni_e_AI_reset(void);

extern int ni_e_AI_IRQ_is_on_START(void);
extern int ni_e_AI_IRQ_is_on_STOP(void);
extern void ni_e_mask_and_ack_AI_irq(int);

extern void ni_e_AOTM_init(void);
extern void ni_e_AO_reset(void);
extern int ni_e_AO_configure(int, int, int, int, int);
extern void ni_e_AO_write(__s16, int);

extern int ni_e_daqstc_DIO_configure(int, int);
extern void ni_e_daqstc_DO_write_bit(int, int);
extern void ni_e_daqstc_DO_reset(void);
extern void ni_e_daqstc_DI_read_bit(unsigned char *, unsigned char);

extern int ni_e_8255_DIO_configure(int, int);
extern void ni_e_8255_DO_write_byte(int, unsigned char);
extern void ni_e_8255_DI_read_byte(unsigned char *, int);

#endif

#endif
