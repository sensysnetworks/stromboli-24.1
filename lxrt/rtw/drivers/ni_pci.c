/*
Copyright (C) 2002 Lorenzo Dozio (dozio@aero.polimi.it)

Partially derived from
COMEDI - Linux Control and Measurement Device Interface
Copyright (C) 1997-2001 David A. Schleef <ds@schleef.org>

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

#include <linux/module.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <linux/slab.h>

#include "ni_pci.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lorenzo Dozio (dozio@aero.polimi.it)");
MODULE_DESCRIPTION("National Instruments PCI boards driver");

#define ni_writeb(a,b,n)		(writeb((a), ni_board[n].iobase + (b)))
#define ni_writeb_p(a,b,n)		(ni_writeb(a,b,n),ni_writeb(a,b,n))
#define ni_writew(a,b,n)		(writew((a), ni_board[n].iobase + (b)))

#define ni_readb(b,n)			(readb(ni_board[n].iobase + (b)))
#define ni_readb_p(b,n)			(ni_readb(b,n),ni_readb(b,n))
#define ni_readw(b,n)			(readw(ni_board[n].iobase + (b)))

#define ni_win_writew(a,b,n)		(ni_writew((b), Waddr_daqstc, n), ni_writew((a), Wdata_daqstc, n))
#define ni_win_writew_671x(a,b,n)	(ni_writew((b), Waddr_671x, n), ni_writew((a), Wdata_671x, n))
#define ni_win_readw(b,n)		(ni_writew((b), Waddr_daqstc, n), ni_readw(Wdata_daqstc, n))
#define ni_win_readw_671x(b,n)		(ni_writew((b), Waddr_671x, n), ni_readw(Wdata_671x, n))

static void ni_671x_calibrate(unsigned int);
static void ni_e_calibrate(unsigned int);
static void ni_write_caldac_mb88341(int, int, int, unsigned int);
//static void ni_write_caldac_dac8800(int, int, int, unsigned int);
static void ni_e_AO_write(signed short int, int, unsigned int);
static void ni_671x_AO_write(signed short int, int, unsigned int);

static int ni_6071e_ai_caldac_addr[] = { 4, 1, 3, 14, 2 };
static int ni_602xe_ai_caldac_addr[] = { 4, 11, 1, 2 };
static int ni_6071e_ao_caldac_addr[] = { 5, 7, 6, 8, 10, 9 };
static int ni_671x_ao_caldac_addr[] = { 9, 5, 8, 3, 11, 7, 12, 2, 10, 6, 1, 4, 9, 5, 8, 15, 11, 7, 12, 14, 10, 6, 13, 4 };

static struct ni_pci_struct ni_board[MAX_NI_BOARDS];

static int ni_n_boards = 0;

static struct ni_specs_struct ni_specs[] = {
	{
	device_id : DEVICE_ID_PCI_6071E,
	name : "PCI 6071E",
	ai : { 64, 12, 1250, ni_6071e_ai_caldac_addr, 424 },
	ao : { 2, 12, 1000, ni_6071e_ao_caldac_addr, 420, ni_e_AO_write },
	dio : { 0, 0, 0, 0 },
	calibrate : ni_e_calibrate,
	write_caldac : ni_write_caldac_mb88341
	},
	{
	device_id : DEVICE_ID_PCI_MIO_16E_1,
	name : "PCI MIO 16E-1",
	ai : { 16, 12, 1250, ni_6071e_ai_caldac_addr, 424 },
	ao : { 2, 12, 1000, ni_6071e_ao_caldac_addr, 420, ni_e_AO_write },
	dio : { 0, 0, 0, 0 },
	calibrate : ni_e_calibrate,
	write_caldac : ni_write_caldac_mb88341
	},
	{
	device_id : DEVICE_ID_PCI_MIO_16E_4,
	name : "PCI MIO 16E-4",
	ai : { 16, 12, 250, ni_6071e_ai_caldac_addr, 424 },
	ao : { 2, 12, 100, ni_6071e_ao_caldac_addr, 420, ni_e_AO_write },
	dio : { 0, 0, 0, 0 },
	calibrate : ni_e_calibrate,
	write_caldac : ni_write_caldac_mb88341
	},
	{
	device_id : DEVICE_ID_PCI_6023E,
	name : "PCI 6023E",
	ai : { 16, 12, 200, ni_602xe_ai_caldac_addr, 442 },
	ao : { 0, 0, 0, NULL, 0, ni_e_AO_write },
	dio : { 0, 0, 0, 0 },
	calibrate : ni_e_calibrate,
	write_caldac : ni_write_caldac_mb88341
	},
	{
	device_id : DEVICE_ID_PCI_6024E,
	name : "PCI 6024E",
	ai : { 16, 12, 200, ni_602xe_ai_caldac_addr, 430 },
	ao : { 2, 12, 100, ni_6071e_ao_caldac_addr, 426, ni_e_AO_write },
	dio : { 0, 0, 0, 0 },
	calibrate : ni_e_calibrate,
	write_caldac : ni_write_caldac_mb88341
	},
	{
	device_id : DEVICE_ID_PCI_6025E,
	name : "PCI 6025E",
	ai : { 16, 12, 200, ni_602xe_ai_caldac_addr, 430 },
	ao : { 2, 12, 100, ni_6071e_ao_caldac_addr, 426, ni_e_AO_write },
	dio : { 1, 0, 0, 0 },
	calibrate : ni_e_calibrate,
	write_caldac : ni_write_caldac_mb88341
	},
	{
	device_id : DEVICE_ID_PCI_6711,
	name : "PCI 6711",
	ai : { 0, 0, 0, NULL, 0 },
	ao : { 4, 12, 1000, ni_671x_ao_caldac_addr, 467, ni_671x_AO_write },
	dio : { 0, 0, 0, 0 },
	calibrate : ni_671x_calibrate,
	write_caldac : ni_write_caldac_mb88341
	},
	{
	device_id : DEVICE_ID_PCI_6713,
	name : "PCI 6713",
	ai : { 0, 0, 0, NULL, 0 },
	ao : { 8, 12, 1000, ni_671x_ao_caldac_addr, 467, ni_671x_AO_write },
	dio : { 0, 0, 0, 0 },
	calibrate : ni_671x_calibrate,
	write_caldac : ni_write_caldac_mb88341
	},
	{ device_id : DEVICE_ID_END }
};

/*
	NI MITE PCI setup/unsetup stuff
	===============================
*/

static int ni_boards_present(void)
{
	struct pci_dev *pdev;
	int n = 0, i;

	pci_for_each_dev(pdev) {
		i = 0;
		if (pdev->vendor == VENDOR_ID_NI) {
			while (ni_specs[i].device_id != DEVICE_ID_END && ni_specs[i].device_id != pdev->device) {
				++i;
			} 		
			ni_board[n].subdev = &ni_specs[i];
			ni_board[n].slot = PCI_SLOT(pdev->devfn);
			ni_board[n].index = n+1;
			ni_board[n].mite_start = pdev->resource[0].start;
			ni_board[n].mite_end   = pdev->resource[0].end;
			ni_board[n].daq_start  = pdev->resource[1].start;
			ni_board[n].daq_end    = pdev->resource[1].end;
			ni_board[n].irq_line   = pdev->irq;
			n++;
			if (n == MAX_NI_BOARDS) {
				return n;
			}
                }
        }
	return n;
}

static int ni_board_setup(unsigned int n)
{
	ni_board[n].mite_io_addr = ioremap(PCI_BASE_ADDRESS_MEM_MASK & ni_board[n].mite_start, 4096);
	ni_board[n].daq_io_addr = ioremap(PCI_BASE_ADDRESS_MEM_MASK & ni_board[n].daq_start, 4096);
	writel(ni_board[n].daq_start | 0x80, ni_board[n].mite_io_addr + 0xc0);
	return (int) ni_board[n].daq_io_addr;
}

static void ni_board_unsetup(unsigned int n)
{
	if (ni_board[n].mite_io_addr) {
		iounmap(ni_board[n].mite_io_addr);
	}
	if (ni_board[n].daq_io_addr) {
		iounmap(ni_board[n].daq_io_addr);
	}
}

/*
	DAQ-STC programming
	===================
*/

static void daqstc_MSC_Clock_Configure(unsigned int n)
{
	ni_win_writew(0x1b00, Clock_and_FOUT_Register, n);
}

static void daqstc_clear_AI_FIFO(unsigned int n)
{
	ni_win_writew(1, Write_Strobe_1_Register, n);
}

static void daqstc_AI_Reset_All(unsigned int n)
{
	ni_win_writew(AI_Reset, Joint_Reset_Register, n);
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register, n);
	ni_win_writew(0x0000, Interrupt_A_Enable_Register, n);
	ni_win_writew(0x3f80, Interrupt_A_Ack_Register, n);
	ni_win_writew(0x000c, AI_Mode_1_Register, n);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register, n);
}

static void daqstc_AI_Board_Personalize(unsigned int n)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register, n);
	ni_win_writew(0x1b80, Clock_and_FOUT_Register, n);
	ni_win_writew(0xa4a0, AI_Personal_Register, n);
	ni_win_writew(0x032e, AI_Output_Control_Register, n);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register, n);
}

static void daqstc_AI_Initialize_Configuration_Memory_Output(unsigned int n)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register, n);
	ni_win_writew(1, AI_Command_1_Register, n);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register, n);
}

static void daqstc_AI_Board_Environmentalize(unsigned int n)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register, n);
	ni_win_writew(0, AI_Mode_2_Register, n);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register, n);
}

static void daqstc_AI_Trigger_Signals(unsigned int n)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register, n);
	ni_win_writew(0x000d, AI_Mode_1_Register, n);
	ni_win_writew(0x0060, AI_Trigger_Select_Register, n);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register, n);
}

static void daqstc_AI_Scan_Start_End(unsigned int n)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register, n);
	ni_win_writew(AI_START_Edge | AI_START_Sync | AI_STOP_Select(0) | AI_STOP_Sync,
			AI_Start_Stop_Select_Register, n);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register, n);
}

static void daqstc_AI_sw_trigger(unsigned int n)
{
	ni_win_writew(1, AI_Command_1_Register, n);
}

static void daqstc_AO_Reset_All(unsigned int n)
{
	ni_win_writew(AO_Reset | AO_Configuration_Start, Joint_Reset_Register, n);
	ni_win_writew(AO_Disarm, AO_Command_1_Register, n);
	ni_win_writew(0, Interrupt_B_Enable_Register, n);
	ni_win_writew(AO_BC_Source_Select, AO_Personal_Register, n);
	ni_win_writew(0x3F98, Interrupt_B_Ack_Register, n);
	ni_win_writew(AO_Configuration_End, Joint_Reset_Register, n);
}

static void daqstc_AO_Board_Personalize(unsigned int n)
{
	ni_win_writew(AO_Configuration_Start, Joint_Reset_Register, n);
	if (ni_board[n].subdev->device_id == DEVICE_ID_PCI_6071E ||
		ni_board[n].subdev->device_id == DEVICE_ID_PCI_MIO_16E_1 ||
		ni_board[n].subdev->device_id == DEVICE_ID_PCI_MIO_16E_4) {
		ni_win_writew(0x1410, AO_Personal_Register, n);
	} else {
        	ni_win_writew(0x1430, AO_Personal_Register, n);
	}
	ni_win_writew(0x1B20, Clock_and_FOUT_Register, n);
	ni_win_writew(0, AO_Output_Control_Register, n);
	ni_win_writew(0, AO_START_Select_Register, n);
	ni_win_writew(AO_Configuration_End, Joint_Reset_Register, n);
}

static void daqstc_AO_LDAC_Source_and_Update_Mode(unsigned int n)
{
	ni_win_writew(AO_Configuration_Start, Joint_Reset_Register, n);
	ni_win_writew(0, AO_Command_1_Register, n);
	ni_win_writew(AO_Configuration_End, Joint_Reset_Register, n);
}

/*
	NI Analog Input subdevice
	=========================
*/

int ni_AI_init(unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->ai.n_channels == 0) {
		return -ENODEV;
	}

	daqstc_MSC_Clock_Configure(n);
	daqstc_clear_AI_FIFO(n);
	daqstc_AI_Reset_All(n);
	daqstc_AI_Board_Personalize(n);
	daqstc_AI_Initialize_Configuration_Memory_Output(n);
	daqstc_AI_Board_Environmentalize(n);
	daqstc_AI_Trigger_Signals(n);
	daqstc_AI_Scan_Start_End(n);
//	daqstc_clear_AI_FIFO(n);

	return 0;
}

int ni_AI_configure(int Chan, int ChanType, int Gain, int UnipBip, int DitherEn, int LastChan, unsigned int n)
{
	__u16 bank;
	__u16 AI_config_high_word, AI_config_low_word;

	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->ai.n_channels == 0) {
		return -ENODEV;
	}
	if (Chan < 0 || Chan > ni_board[n].subdev->ai.n_channels) {
		return -EINVAL;
	}

	bank = Chan / 16;

	daqstc_clear_AI_FIFO(n);

	AI_config_high_word = Chan | (bank << 4) | (ChanType << 12);
	ni_writew(AI_config_high_word, Configuration_Memory_High_Register, n);
	AI_config_low_word = Gain | (UnipBip << 8) | (DitherEn << 9) | (LastChan << 15);
	ni_writew(AI_config_low_word, Configuration_Memory_Low_Register, n);

	return 0;
}

int ni_AI_read(signed short int *data, int nchan, unsigned int n)
{
	int i;

	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->ai.n_channels == 0) {
		return -ENODEV;
	}
	if (nchan > ni_board[n].subdev->ai.n_channels) {
		return -EINVAL;
	}
	
	for (i = 0; i < nchan; i++) {
		daqstc_AI_sw_trigger(n);
		while (1) {
			if (!(ni_readw(AI_Status_1, n) & AI_FIFO_Empty_St)) {
				data[i] = ni_readw(ADC_FIFO_Data_Register, n);
				break;
			}
		}
	}
	ni_win_writew(1 << 3, AI_Command_2_Register, n);

	return 0;
}

int ni_AI_reset(unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->ai.n_channels == 0) {
		return -ENODEV;
	}

	daqstc_MSC_Clock_Configure(n);
	daqstc_clear_AI_FIFO(n);
	daqstc_AI_Reset_All(n);
	daqstc_AI_Board_Personalize(n);
	daqstc_AI_Initialize_Configuration_Memory_Output(n);

	return 0;
}

int ni_AI_clear_configuration_FIFO(unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->ai.n_channels == 0) {
		return -ENODEV;
	}

	ni_win_writew(1, Write_Strobe_0_Register, n);

	return 0;
}
	
/*
	NI Analog Output subdevice 
	==========================
*/

int ni_AO_init(unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->ao.n_channels == 0) {
		return -ENODEV;
	}

	daqstc_AO_Reset_All(n);
	daqstc_AO_Board_Personalize(n);
	daqstc_AO_LDAC_Source_and_Update_Mode(n);

	return 0;
}

int ni_AO_configure(unsigned int DACSel, int ExtRef, int ReGlitch, int BipDac, unsigned int n)
{
/*
   DACSel   = 0/7 -> DAC0...7 selected for actual configuration
   ExtRef   = 0 -> internal reference
              1 -> external reference
   ReGlitch = 1 -> more uniform reglithcing
   BipDac   = 0 -> unipolar mode
              1 -> bipolar mode
*/
	__u16 AO_config_word = 0;

	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->ao.n_channels == 0) {
		return -ENODEV;
	}
	if (DACSel >= ni_board[n].subdev->ao.n_channels) {
		return -EINVAL;
	}
	if (ExtRef < 0 || ExtRef > 1) {
		return -EINVAL;
	}
	if (ReGlitch < 0 || ReGlitch > 1) {
		return -EINVAL;
	}
	if (BipDac < 0 || BipDac > 1) {
		return -EINVAL;
	}

	AO_config_word = (DACSel << 8) | (ExtRef << 2) | (ReGlitch << 1) | (BipDac << 0);
	ni_writew(AO_config_word, AO_Configuration_Register, n);

	return 0;
}

void ni_e_AO_write(signed short int data, int chan, unsigned int n)
{
	ni_writew(data, (chan & 1) ? DAC1_Direct_Data_Register : DAC0_Direct_Data_Register, n);
	return;
}

void ni_671x_AO_write(signed short int data, int chan, unsigned int n)
{
	ni_win_writew_671x(data, chan, n);
	return;
}

int ni_AO_write(signed short int data, int chan, unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->ao.n_channels == 0) {
		return -ENODEV;
	}

	ni_board[n].subdev->ao.write(data, chan, n);

	return 0;
}

int ni_AO_reset(unsigned int n)
{
	int i;

	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->ao.n_channels == 0) {
		return -ENODEV;
	}

	for (i = 0; i < ni_board[n].subdev->ao.n_channels; i++) {
		ni_board[n].subdev->ao.write(0, i, n);
	}
	daqstc_AO_Reset_All(n);

	return 0;
}

int ni_671x_AO_set_immediate_mode(unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}

	ni_win_writew_671x(0xff, AO_Immediate_Register, n);

	return 0;
}

/*
	NI Digital I/O subdevice 
	========================
*/

int ni_82C55_configure(int port, int mode, unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->dio.has_82C55 == 0) {
		return -ENODEV;
	}

        if (port < 3) {
                port--;
        }
        if (mode == NI_82C55_OUTPUT) {
		ni_board[n].subdev->dio.CR_mask_82C55 &= ~(1 << port);
        } else {
		ni_board[n].subdev->dio.CR_mask_82C55 |= (1 << port);
        }
        ni_writeb(ni_board[n].subdev->dio.CR_mask_82C55 | 0x80, 0x1f, n);

        return 0;
}

int ni_82C55_write_byte(int port, unsigned char data, unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->dio.has_82C55 == 0) {
		return -ENODEV;
	}

        switch (port) {
                case PORT_A_82C55:
                        ni_writeb(data, 0x19, n);
                        break;
                case PORT_B_82C55:
                        ni_writeb(data, 0x1b, n);
                        break;
                case PORT_C_low_82C55:
                        ni_writeb(data & 0x0f, 0x1d, n);
                        break;
                case PORT_C_up_82C55:
                        ni_writeb((data & 0x0f) << 4, 0x1d, n);
                        break;
                default:
                        break;
        }

	return 0;
}

int ni_82C55_read_byte(unsigned char *data, int port, unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}
	if (ni_board[n].subdev->dio.has_82C55 == 0) {
		return -ENODEV;
	}

        switch (port) {
                case PORT_A_82C55:
                        *data = ni_readb(0x19, n);
                        break;
                case PORT_B_82C55:
                        *data = ni_readb(0x1b, n);
                        break;
                case PORT_C_low_82C55:
                        *data = ni_readb(0x1d, n) & 0x0f;
                        break;
                case PORT_C_up_82C55:
                        *data = (ni_readb(0x1d, n) & 0xf0) >> 4;
                        break;
                default:
                        break;
        }

	return 0;
}

int ni_DIO_module_configure(int pin, int mode, unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}

        if (mode == NI_DIO_MODULE_OUTPUT) {
                ni_board[n].subdev->dio.CR_mask_dio_module |= (1 << pin);
        } else {
                ni_board[n].subdev->dio.CR_mask_dio_module &= ~(1 << pin);
        }
        ni_win_writew((unsigned short int)ni_board[n].subdev->dio.CR_mask_dio_module, DIO_Control_Register, n);

	return 0;
}

int ni_DIO_module_write_bit(int bit, int onoff, unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}

        if (onoff) {
                ni_board[n].subdev->dio.DO_mask_dio_module |= (1 << bit);
        } else {
                ni_board[n].subdev->dio.DO_mask_dio_module &= ~(1 << bit);
        }
        ni_win_writew((unsigned short int)ni_board[n].subdev->dio.DO_mask_dio_module, DIO_Output_Register, n);

	return 0;
}

int ni_DIO_module_reset(unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}

        ni_board[n].subdev->dio.DO_mask_dio_module = 0;
        ni_win_writew((unsigned short int)ni_board[n].subdev->dio.DO_mask_dio_module, DIO_Output_Register, n);

	return 0;
}

int ni_DIO_module_read_bit(unsigned char *bit, unsigned char pin, unsigned int n)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}

        *bit = (unsigned char)ni_win_readw(DIO_Parallel_Input_Register, n) & (1 << pin);

	return 0;
}

/*
	NI board calibration stuff
	==========================
*/

static int ni_read_eeprom(int addr, unsigned int n)
{
	int bit, bitstring;

	bitstring = 0x0300 | ((addr & 0x100) << 3) | (addr & 0xff);
	ni_writeb_p(0x04, Serial_Command_Register, n);
	for (bit = 0x8000; bit; bit>>=1) {
		ni_writeb_p(0x04 | ((bit&bitstring) ? 0x02:0), Serial_Command_Register, n);
		ni_writeb_p(0x05 | ((bit&bitstring) ? 0x02:0), Serial_Command_Register, n);
	}
	bitstring = 0;
	for (bit = 0x80; bit; bit>>=1) {
		ni_writeb_p(0x04, Serial_Command_Register, n);
		ni_writeb_p(0x05, Serial_Command_Register, n);
		bitstring |= ((ni_readb_p(Status_Register, n) & 0x01) ? bit:0);
	}
	ni_writeb_p(0x00, Serial_Command_Register, n);

	return bitstring;
}

void ni_write_caldac_mb88341(int addr, int val, int SerDacLd, unsigned int n)
{
        int bit, bitstring = 0;

        bitstring = ((addr & 0x1) << 11) |
                    ((addr & 0x2) << 9)  |
                    ((addr & 0x4) << 7)  |
                    ((addr & 0x8) << 5)  |
                    (val & 0xff);

        for (bit = 1<<11; bit; bit>>=1) {
                ni_writeb_p(((bit&bitstring) ? 0x02:0), Serial_Command_Register, n);
                ni_writeb_p(1 | ((bit&bitstring) ? 0x02:0), Serial_Command_Register, n);
        }
        ni_writeb_p(0x08 << SerDacLd, Serial_Command_Register, n);
        ni_writeb_p(0, Serial_Command_Register, n);

	return;
}

/*
void ni_write_caldac_dac8800(int addr, int val, int SerDacLd, unsigned int n)
{
	return;
}
*/

void ni_671x_calibrate(unsigned int n)
{
	int i, j = 0, k, chan, offset, eeprom_data[3];

/*
	Calibrating AO subdev
*/
	for (chan = 0; chan < ni_board[n].subdev->ao.n_channels; chan++) {
		offset = ni_board[n].subdev->ao.eeprom_base - (int)(3*chan);
		if (chan > 3) { k = 1; } else { k = 0; }
		for (i = 0; i < 3; i++) {
			eeprom_data[i] = ni_read_eeprom(offset-i, n);
			ni_board[n].subdev->write_caldac(ni_board[n].subdev->ao.caldac_addr[j], eeprom_data[i], k, n);
			j++;
		}
	}
	return;
}

void ni_e_calibrate(unsigned int n)
{
	int i, ao_eeprom_data[6], ai_eeprom_data[4];

/*
	Calibrating AO subdev
*/
	for (i = 0; i < 6; i++) {
		ao_eeprom_data[i] = ni_read_eeprom(ni_board[n].subdev->ao.eeprom_base-i, n);
		ni_board[n].subdev->write_caldac(ni_board[n].subdev->ao.caldac_addr[i], ao_eeprom_data[i], 0, n);
	}
/*
	Calibrating AI subdev
*/
	for (i = 0; i < 4; i++) {
		ai_eeprom_data[i] = ni_read_eeprom(ni_board[n].subdev->ai.eeprom_base-i, n);
		ni_board[n].subdev->write_caldac(ni_board[n].subdev->ai.caldac_addr[i], ai_eeprom_data[i], 0, n);
	}
	return;
}

void ni_calibrate(unsigned int n)
{
	ni_board[n].subdev->calibrate(n);
}

/*
	Utility functions
	=================
*/

void ni_get_n_boards(int *n_boards)
{
	int n = 0;
	
	while (ni_board[n].index > 0) {
		n++;
	}
	*n_boards = n;
}

void ni_get_board_list_index(unsigned short id, int *index)
{
	int n = 0;
	
	while (ni_board[n].index > 0) {
		if (ni_board[n].subdev->device_id == id) {
			*index = n;
			return;
		}
		n++;
	}
	*index = -1;
}

void ni_get_board_n_ai_channels(unsigned short id, int *nchan)
{
	int n = 0;

	while (ni_board[n].index > 0) {
		if (ni_board[n].subdev->device_id == id) {
			*nchan = ni_board[n].subdev->ai.n_channels;
			return;
		}
		n++;
	}
	*nchan = -1;
}
	
void ni_get_board_n_ao_channels(unsigned short id, int *nchan)
{
	int n = 0;

	while (ni_board[n].index > 0) {
		if (ni_board[n].subdev->device_id == id) {
			*nchan = ni_board[n].subdev->ao.n_channels;
			return;
		}
		n++;
	}
	*nchan = -1;
}
	
int ni_get_board_device_id(unsigned int n, unsigned short *id)
{
	if (n >= ni_n_boards) {
		return -EFAULT;
	}

	*id = ni_board[n].subdev->device_id;

	return 0;
}

/*
	Init module function
	====================
*/

int init_module(void)
{
	int n = 0;

	if (!pci_present())
		return -ENODEV;

	if ((ni_n_boards = ni_boards_present()) == 0) {
		printk(KERN_NOTICE "No National Instruments PCI board found\n");
		return -ENODEV;
	}

#ifdef NI_PCI_DEBUG
	printk("Number of NI boards found : %d\n", ni_n_boards);
#endif
	while (ni_board[n].index > 0) {
#ifdef NI_PCI_DEBUG
		printk("Board          : %s\n", ni_board[n].subdev->name);
		printk("   Number      : %d (n = %d)\n", ni_board[n].index, n);
		printk("   Slot        : %d\n", ni_board[n].slot);
		printk("   Mite addr   : 0x%x\n", (unsigned int)ni_board[n].mite_start);
		printk("   DAQ addr    : 0x%x\n", (unsigned int)ni_board[n].daq_start);
		printk("   IRQ line    : %d\n", ni_board[n].irq_line);
		printk("   AI channels : %d\n", ni_board[n].subdev->ai.n_channels);
		printk("   AI bits     : %d\n", ni_board[n].subdev->ai.n_bits);
		printk("   AI sampling : %d [kHz]\n", ni_board[n].subdev->ai.max_sampling_rate);
		printk("   AO channels : %d\n", ni_board[n].subdev->ao.n_channels);
		printk("   AO bits     : %d\n", ni_board[n].subdev->ao.n_bits);
		printk("   AO sampling : %d [kHz]\n", ni_board[n].subdev->ao.max_update_rate);
#endif
		ni_board[n].iobase = ni_board_setup(n);
		ni_calibrate(n);
		n++;
	}

	return 0;
}

/*
	Cleaunp module function
	=======================
*/

void cleanup_module(void)
{
	int n = 0;

	while (ni_board[n].index > 0) {
		ni_board_unsetup(n);
		n++;
	}
	return;
}
