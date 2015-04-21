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

#ifndef MODULE
#include <stdio.h>
#include <sys/io.h>
#include <unistd.h>
#endif

#define KEEP_STATIC_INLINE

/* Digital I/O registers */

#define PORT_0A_DATA_REG	0
#define PORT_0B_DATA_REG	1
#define PORT_0C_DATA_REG	2
#define CONTROL_0_REG		3
#define PORT_1A_DATA_REG	4
#define PORT_1B_DATA_REG	5
#define PORT_1C_DATA_REG	6
#define CONTROL_1_REG		7

#define MODE_PORT_CLOW(x)	((x) << 0)
#define MODE_PORT_B(x)		((x) << 1)
#define MODE_PORT_CHIGH(x)	((x) << 3)
#define MODE_PORT_A(x)		((x) << 4)

#define D_OUT	0	/* digital output mode */
#define D_IN	1	/* digital input mode */
#define PORT_NUMBER_0	0
#define PORT_NUMBER_1	1
#define PORT_A		4
#define PORT_C_up	3
#define PORT_B		2
#define PORT_C_low	1

/* DAC registers */

#define DAC_CONTROL_REG		0
/* write bitfields */
#define SIMULTANEOUS_UPDATE(x)	((x) << 0)
#define ENABLE_DAC(x)		((x) << 1)
#define SELECTED_CONFIG_DAC(x)	((x) << 2)
#define DAC_RANGE(x)		((x) << 6)

#define DAC_CALIBRATION_1_REG	4
/* write bitfields */
#define SERIAL_DATA_IN(x)	((x) << 0)
#define SELECTED_CALIB_DAC(x)	((x) << 1)
/* read bitfields */
#define COUNTER_OVERFLOW(x)	((x) << 5)
#define CALIB_COMP(x)		((x) << 6)
#define SERIAL_DATA_OUT(x)	((x) << 7)

#define DAC_CALIBRATION_2_REG	6
/* write bitfields */
#define SELECT_EEPROM(x)	((x) << 0)
#define DESELECT_542(x)		((x) << 1)
#define DESELECT_8800_xy(n)	(0x4 << (n))

#define DAC_0_DATA_REG		8

#define BIPOLAR		0
#define UNIPOLAR	1
#define FS_2V5		1
#define FS_5V		2
#define FS_10V		3

static inline unsigned char cb_pcidda_read_di(int number, int port)
{
	unsigned char byte = 0;

	switch (port) {
		case PORT_A:
			byte = inb(DIO_BASE+(int)(4*number));
			break;
		case PORT_B:
			byte = inb(DIO_BASE+1+(int)(4*number));
			break;
		case PORT_C_low:
			byte = inb(DIO_BASE+2+(int)(4*number)) & 0x0f;
			break;
		case PORT_C_up:
			byte = (inb(DIO_BASE+2+(int)(4*number)) & 0xf0) >> 4;
			break;
		default:
			break;
	}
	return byte;		
}

static inline void cb_pcidda_write_do(int number, int port, unsigned char data)
{
	switch (port) {
		case PORT_A:
			outb(data, DIO_BASE+(int)(4*number));
			break;
		case PORT_B:
			outb(data, DIO_BASE+1+(int)(4*number));
			break;
		case PORT_C_low:
			outb(data & 0x0f, DIO_BASE+2+(int)(4*number));
			break;
		case PORT_C_up:
			outb((data & 0x0f) << 4, DIO_BASE+2+(int)(4*number));
			break;
		default:
			break;
	}	
}

static inline void cb_pcidda_reset_dio(void)
{
	outb(0x00, DIO_BASE + CONTROL_0_REG);
	outb(0x00, DIO_BASE + CONTROL_1_REG);
	outb(0x00, DIO_BASE + PORT_0A_DATA_REG);
	outb(0x00, DIO_BASE + PORT_0B_DATA_REG);
	outb(0x00, DIO_BASE + PORT_0C_DATA_REG);
	outb(0x00, DIO_BASE + PORT_1A_DATA_REG);
	outb(0x00, DIO_BASE + PORT_1B_DATA_REG);
	outb(0x00, DIO_BASE + PORT_1C_DATA_REG);
}

static inline void cb_pcidda_configure_dio(int number, int port, int mode)
{
	unsigned char CR_word;
	int offset = (int)(4*number);

	if (port < 3) {
		port--;
	}
	CR_word = inb(DIO_BASE + CONTROL_0_REG + offset);	
	if (mode == D_OUT) {
		CR_word &= ~(1 << port);
	} else {
		CR_word |= (1 << port);
	}
	outb(CR_word, DIO_BASE + CONTROL_0_REG + offset);
}

static inline void cb_pcidda_write_dac(int channel, int value)
{
	outw(value, DAC_BASE + DAC_0_DATA_REG + 2*channel);
}

static inline signed short int cb_pcidda_v2i(double fval, int polarity, int vfs)
{
/*
   polarity = 0 -> AO unipolar mode
   polarity = 1 -> AO bipolar mode
*/
        signed short int ival;
	double fsval = 10.;

	switch (vfs) {
		case FS_2V5:
			fsval = 2.5;
			break;
		case FS_5V:
			fsval = 5.;
			break;
		case FS_10V:
			fsval = 10.;
			break;
	}
	if (polarity == UNIPOLAR) {
		if (fval > fsval) fval = fsval;
		if (fval < 0.) fval = 0.;
		ival = (signed short int)((fval/fsval)*4095.);
	} else {
		if (fval > fsval) fval = fsval;
		if (fval < -fsval) fval = -fsval;
		ival = (signed short int)(((fval/fsval)*2047.)+2048.);
	}
        return ival;
}

static inline void cb_pcidda_reset_dac(int chan, int polarity, int vfs)
{
	cb_pcidda_write_dac(chan, cb_pcidda_v2i(0., polarity, vfs));
}

static inline void cb_pcidda_configure_dac(int channel, int polarity, int vfs, int su_flag)
{
	unsigned short int dac_control_mask = 0;
	int range = (int)(4*polarity) + vfs;
	
	dac_control_mask = SIMULTANEOUS_UPDATE(su_flag) |
			   ENABLE_DAC(1) |			    
			   SELECTED_CONFIG_DAC(channel) |
			   DAC_RANGE(range);
	outw(dac_control_mask, DAC_BASE + DAC_CONTROL_REG);
}			   

static inline unsigned short int cb_pcidda_read_eeprom(unsigned int address)
{
	unsigned short int value = 0;
	int n, sdo_bit;

	outw_p(0x7F, DAC_BASE + DAC_CALIBRATION_2_REG);
	outw_p(0x01, DAC_BASE + DAC_CALIBRATION_1_REG);
	outw_p(0x01, DAC_BASE + DAC_CALIBRATION_1_REG);
	outw_p(0x00, DAC_BASE + DAC_CALIBRATION_1_REG);
	for (n = 7; n >= 0; n--) {
		outw_p((address >> n) & 0x01, DAC_BASE + DAC_CALIBRATION_1_REG);
	}
	for (n = 15; n >= 0; n--) {
		sdo_bit = (inw_p(DAC_BASE + DAC_CALIBRATION_1_REG) >> 7) & 0x01;
		value |= (sdo_bit << n);
	}
	outw_p(0x7E, DAC_BASE + DAC_CALIBRATION_2_REG);

	return value;
}

static inline void cb_pcidda_calibrate_dac(int chan, int polarity, int vfs, unsigned short int eeprom_data[])
{
	unsigned int trim_dac_code[4];
	unsigned int eeprom_addr[2];
	unsigned int value[4];
	unsigned int sel_mask;
	int range, bit, i;

/*
	chan  = 0 -> DAC0
		1 -> DAC1
		  ...
		7 -> DAC7

	range = 0 -> Bipolar 10V
		1 -> Bipolar 5V
		2 -> Bipolar 2.5 V
		3 -> Unipolar 10 V
		4 -> Unipolar 5V
		5 -> Unipolar 2.5 V

	value[0] -> fine gain byte
	value[1] -> coarse gain byte
	value[2] -> coarse offset byte
	value[3] -> fine offset byte
*/

	range = (int)(3 - vfs + (int)(3*polarity));
	eeprom_addr[0] = 8 + 2*range + 12*chan;
	eeprom_addr[1] = 7 + 2*range + 12*chan;
	value[0] = eeprom_data[eeprom_addr[0]] & 0xff;
	value[1] = (eeprom_data[eeprom_addr[0]] >> 8) & 0xff;
	value[2] = (eeprom_data[eeprom_addr[1]] >> 8) & 0xff;
	value[3] = eeprom_data[eeprom_addr[1]] & 0xff;
	for (i = 0; i < 4; i++) {
		trim_dac_code[i] = i + 4*(chan % 2);
		for (bit = 2; bit >= 0; bit--) {
			outw_p((trim_dac_code[i] >> bit) & 0x01, DAC_BASE + DAC_CALIBRATION_1_REG);
		}
		for (bit = 7; bit >= 0; bit--) {
			outw_p((value[i] >> bit) & 0x01, DAC_BASE + DAC_CALIBRATION_1_REG);
		}
		sel_mask = ~((0x4 << (chan/2)) | 0x1) & 0xff;
		outw_p(sel_mask, DAC_BASE + DAC_CALIBRATION_2_REG);
		outw_p(0x7E, DAC_BASE + DAC_CALIBRATION_2_REG);
	}
}
