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

#include "ni_e.h"
#include "ni_e_regs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lorenzo Dozio (dozio@aero.polimi.it)");
MODULE_DESCRIPTION("National Instruments E-Series Boards driver");

struct ni_e_dev *ni_e;

#define CONVERT_MARGIN	105	/* 5 % increase between conversions */

#define ni_writew(a,b)          (writew((a), ni_e->iobase + (b)))
#define ni_writeb(a,b)          (writeb((a), ni_e->iobase + (b)))
#define ni_writeb_p(a,b)        (ni_writeb(a,b),ni_writeb(a,b))
#define ni_readw(a)             (readw(ni_e->iobase + (a)))
#define ni_readb(a)             (readb(ni_e->iobase + (a)))
#define ni_readb_p(a)           (ni_readb(a),ni_readb(a))
#define ni_win_writew(a,b)      (ni_writew((b), Window_Address), \
                                 ni_writew((a), Window_Data))
#define ni_win_readw(b)         (ni_writew((b), Window_Address), \
                                 ni_readw(Window_Data))
#define ni_win_writew2(a,b)     do { \
                                 ni_writew((b), Window_Address); \
                                 ni_writew(((a)>>16)&0xffff, Window_Data); \
                                 ni_writew((b)+1, Window_Address); \
                                 ni_writew((a)&0xffff, Window_Data); \
                                } while(0)
#define fifo_empty()            (ni_readw(AI_Status_1) & AI_FIFO_Empty_St)
#define fifo_not_empty()        (!(fifo_empty()))
#define read_AI_FIFO()		(ni_readw(ADC_FIFO_Data))

int ni_e_present(void);
struct ni_e_dev *ni_e_init(struct pci_dev *pdev);
void ni_e_cleanup(struct ni_e_dev *ni_e);
int ni_e_setup(struct ni_e_dev *ni_e);
void ni_e_unsetup(struct ni_e_dev *ni_e);

void ni_e_AITM_init(__u32, int, __u32);
void ni_e_AOTM_init(void);
void ni_e_AI_calibrate(void);
void ni_e_AO_calibrate(int);

int number_of_ni_e = 4;

struct {
	unsigned short dev_id;
	char *name;
	int n_ai_chan;
	int ai_resol;
	int samp_rate;
	int ao_resol;
} dev_list[] = {
	{DEVICE_ID_MIO_16E_1,
	"National Instruments PCI-MIO-16E-1",
	16, 12, 1250, 12},
	{DEVICE_ID_MIO_16E_4,
	"National Instruments PCI-MIO-16E-4",
	16, 12, 250, 12},
	{DEVICE_ID_6071E,
	"National Instruments PCI-6071E",
	64, 12, 1250, 12},
	{DEVICE_ID_6025E,
	"National Instruments PCI-6025E",
	16, 12, 200, 12},
};

int ni_e_present(void)
{
	struct pci_dev *pdev = NULL;

#if LINUX_VERSION_CODE < 0x020300
	for (pdev = pci_devices; pdev; pdev = pdev->next) {
#else
	pci_for_each_dev(pdev) {
#endif
		if (pdev->vendor == VENDOR_ID_NI) {
			switch (pdev->device) {
				case DEVICE_ID_MIO_16E_1:
				case DEVICE_ID_MIO_16E_4:
				case DEVICE_ID_6071E:
				case DEVICE_ID_6025E:
					ni_e = ni_e_init(pdev);
					break;
				default:
					return -ENOSYS;
			}
			return 0;
		}
	}
	return -ENODEV;
}

struct ni_e_dev *ni_e_init(struct pci_dev *pdev)
{
	int i;
	ni_e = kmalloc(sizeof(*ni_e), GFP_KERNEL);
	memset(ni_e, 0, sizeof(*ni_e));
	ni_e->ai = kmalloc(sizeof(*ni_e->ai), GFP_KERNEL);
	memset(ni_e->ai, 0, sizeof(*ni_e->ai));
	ni_e->ao = kmalloc(sizeof(*ni_e->ao), GFP_KERNEL);
	memset(ni_e->ao, 0, sizeof(*ni_e->ao));
	ni_e->dio = kmalloc(sizeof(*ni_e->dio), GFP_KERNEL);
	memset(ni_e->dio, 0, sizeof(*ni_e->dio));
	for (i = 0; i < number_of_ni_e; i++) {
		if (pdev->device == dev_list[i].dev_id) {
			ni_e->name = dev_list[i].name;
			ni_e->ai->nchan = dev_list[i].n_ai_chan;
			ni_e->ai->resol = dev_list[i].ai_resol;
			ni_e->ai->max_sampling = dev_list[i].samp_rate;
			ni_e->ao->resol = dev_list[i].ao_resol;
			printk("Found %s\n", ni_e->name);
			printk("ADC channels      : %d\n", ni_e->ai->nchan);
			printk("Bit resolution    : %d\n", ni_e->ai->resol);
			printk("Max sampling rate : %d [KHz]\n", ni_e->ai->max_sampling);
		}
	} 
#if LINUX_VERSION_CODE < 0x020300
	ni_e->mite_start = pdev->base_address[0];
	ni_e->daq_start  = pdev->base_address[1];
#else
	ni_e->mite_start = pdev->resource[0].start;
	ni_e->mite_end	 = pdev->resource[0].end;
	ni_e->daq_start  = pdev->resource[1].start;
	ni_e->daq_end	 = pdev->resource[1].end;
#endif
	ni_e->irq_line   = pdev->irq;

#if LINUX_VERSION_CODE < 0x020300
	printk("MITE ADDRESS at 0x%x\n",
		(unsigned int)ni_e->mite_start);
	printk("DAQ  ADDRESS at 0x%x\n",
		(unsigned int)ni_e->daq_start);
#else
	printk("MITE ADDRESS at 0x%x [0x%x]\n",
		(unsigned int)ni_e->mite_start,
		(unsigned int)ni_e->mite_end);
	printk("DAQ  ADDRESS at 0x%x [0x%x]\n",
		(unsigned int)ni_e->daq_start,
		(unsigned int)ni_e->daq_end);
#endif
	printk("IRQ  NUMBER  :  %d\n", ni_e->irq_line);

	return ni_e;
}

void ni_e_cleanup(struct ni_e_dev *ni_e)
{
	kfree(ni_e->dio);
	kfree(ni_e->ao);
	kfree(ni_e->ai);
	kfree(ni_e);
}

int ni_e_setup(struct ni_e_dev *ni_e)
{
	ni_e->mite_io_addr = ioremap(PCI_BASE_ADDRESS_MEM_MASK & ni_e->mite_start, 4096);
	ni_e->daq_io_addr = ioremap(PCI_BASE_ADDRESS_MEM_MASK & ni_e->daq_start, 4096);
	writel(ni_e->daq_start | 0x80, ni_e->mite_io_addr + 0xc0);
	return (int) ni_e->daq_io_addr;
}

void ni_e_unsetup(struct ni_e_dev *ni_e)
{
	if (ni_e->mite_io_addr) {
		iounmap(ni_e->mite_io_addr);
		ni_e->mite_io_addr = NULL;
	}
	if (ni_e->daq_io_addr) {
		iounmap(ni_e->daq_io_addr);
		ni_e->daq_io_addr = NULL;
	}
}

void ni_e_clear_Configuration_FIFO(void)
{
	ni_win_writew(1, Write_Strobe_0_Register);
}

void daqstc_MSC_Clock_Configure(void)
{
	ni_win_writew(0x1b00, Clock_and_FOUT_Register);
}

void ni_e_clear_AI_FIFO(void)
{
	ni_win_writew(1, Write_Strobe_1_Register);
}

void daqstc_AI_Reset_All(void)
{
	ni_win_writew(AI_Reset, Joint_Reset_Register);
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(0x0000, Interrupt_A_Enable_Register);
	ni_win_writew(0x3f80, Interrupt_A_Ack_Register);
	ni_win_writew(0x000c, AI_Mode_1_Register);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register);
}

void daqstc_AI_Board_Personalize(void)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(0x1b80, Clock_and_FOUT_Register);
	ni_win_writew(0xa4a0, AI_Personal_Register);
	ni_win_writew(0x032e, AI_Output_Control_Register);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register);
}

void daqstc_AI_Initialize_Configuration_Memory_Output(void)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(1, AI_Command_1_Register);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register);
}

void daqstc_AI_Board_Environmentalize(void)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(0, AI_Mode_2_Register);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register);
}

void daqstc_AI_Trigger_Signals(void)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(0x000d, AI_Mode_1_Register);
	ni_win_writew(0x0060, AI_Trigger_Select_Register);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register);
}

void daqstc_AI_Number_Of_Scans(__u32 scans)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register);
	if (scans == 0) {
		ni_win_writew(0x000e, AI_Mode_1_Register);
		ni_win_writew2(0, AI_SC_Load_A_Registers);
	} else {
		ni_win_writew(0x000c, AI_Mode_1_Register);
		ni_win_writew2(scans-1, AI_SC_Load_A_Registers);
	}
	ni_win_writew(0x0020, AI_Command_1_Register);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register);
}

void daqstc_AI_Scan_Start_End(__u32 ticks)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(AI_START_Edge | AI_START_Sync |
			AI_STOP_Select(19) | AI_STOP_Sync,
			AI_Start_Stop_Select_Register);
	ni_win_writew(0x0000, AI_SI_Load_A_Registers);
	ni_win_writew(0x0001, AI_SI_Load_A_Registers + 1);
	ni_win_writew(0x0200, AI_Command_1_Register);
	ni_win_writew2(ticks-1, AI_SI_Load_A_Registers);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register);
}

void daqstc_AI_CONVERT_Signal(__u16 ticks)
{
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(1, AI_SI2_Load_A_Register);
	ni_win_writew(ticks-1, AI_SI2_Load_B_Register);
	ni_win_writew(0x0100, AI_Mode_2_Register);
	ni_win_writew(0x0800, AI_Command_1_Register);
	ni_win_writew(0x0300, AI_Mode_2_Register);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register);
}

void daqstc_AI_Interrupt_Enable(int irq)
{
	switch (irq) {
		case AI_IRQ_on_nothing :
			ni_win_writew(0x0000, Interrupt_A_Enable_Register);
			break;
		case AI_IRQ_on_START :
			ni_win_writew(AI_START_Interrupt_Enable, Interrupt_A_Enable_Register);
			ni_win_writew(0, Interrupt_Control_Register);
			ni_win_writew((1<<0) | (1<<11), Interrupt_Control_Register);
			break;
		case AI_IRQ_on_STOP :
			ni_win_writew(AI_START_Interrupt_Enable | AI_STOP_Interrupt_Enable, Interrupt_A_Enable_Register);
			ni_win_writew(0, Interrupt_Control_Register);
			ni_win_writew((1<<0) | (1<<11), Interrupt_Control_Register);
			break;
		default:
			break;
	}
}

void daqstc_AI_Arming(void)
{
	ni_win_writew(0x1540, AI_Command_1_Register);
}

void daqstc_AI_software_trigger(void)
{
	ni_win_writew(1, AI_Command_1_Register);
}

void ni_e_AI_init(void)
{
	daqstc_MSC_Clock_Configure();
	ni_e_clear_AI_FIFO();
	daqstc_AI_Reset_All();
	daqstc_AI_Board_Personalize();
	daqstc_AI_Initialize_Configuration_Memory_Output();
	daqstc_AI_Board_Environmentalize();
	daqstc_AI_Trigger_Signals();
	ni_win_writew(AI_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(AI_START_Edge | AI_START_Sync |
			AI_STOP_Select(0) | AI_STOP_Sync,
//			AI_STOP_Select(19) | AI_STOP_Sync,
			AI_Start_Stop_Select_Register);
	ni_win_writew(AI_Configuration_End, Joint_Reset_Register);
//	ni_e_clear_AI_FIFO();
}

void ni_e_AITM_init(__u32 sampling_time_ns, int irq_type, __u32 number_of_scans)
{
	int fastest_convert = (((1000000+(ni_e->ai->max_sampling-1)/2) / ni_e->ai->max_sampling)*CONVERT_MARGIN)/100;
	__u16 convert_ticks = (fastest_convert+((BOARD_TICK-1)/2)) / BOARD_TICK;
	__u32 scan_ticks = (sampling_time_ns+((BOARD_TICK-1)/2)) / BOARD_TICK;
	
	daqstc_MSC_Clock_Configure();
	ni_e_clear_AI_FIFO();
	daqstc_AI_Reset_All();
	daqstc_AI_Board_Personalize();
	daqstc_AI_Initialize_Configuration_Memory_Output();
	daqstc_AI_Board_Environmentalize();
	daqstc_AI_Trigger_Signals();
	daqstc_AI_Number_Of_Scans(number_of_scans);
	daqstc_AI_Scan_Start_End(scan_ticks);
	daqstc_AI_CONVERT_Signal(convert_ticks);
	ni_e_clear_AI_FIFO();
	daqstc_AI_Interrupt_Enable(irq_type);
	daqstc_AI_Arming();
}

void ni_e_AI_start(void)
{
	ni_win_writew(AI_START1_Pulse, AI_Command_2_Register);
}

void ni_e_AI_read(__s16 *values, int nchan, int irq_type)
{
        int n;

	switch (irq_type) {
		case AI_IRQ_on_nothing :
			for (n = 0; n < nchan; n++) {
				daqstc_AI_software_trigger();
				while (1) {
					if (fifo_not_empty()) {
						values[n] = read_AI_FIFO() & 0xFFFF;
						break;
					}
				}
			}
			ni_win_writew(1 << 3, AI_Command_2_Register);
			break;
		case AI_IRQ_on_START :
			n = 0;
			do {
				if (fifo_not_empty()) {
					values[n] = read_AI_FIFO() & 0xFFFF;
					n++;
				}
			} while (n < nchan);
			break;
		case AI_IRQ_on_STOP :
        		for (n = 0; n < nchan; n++) {
				values[n] = read_AI_FIFO() & 0xFFFF;
        		}
			break;
		default :
			break;
	}
}

int ni_e_AI_IRQ_is_on_START(void)
{
	return (ni_readw(AI_Status_1) & AI_START_St);
}

int ni_e_AI_IRQ_is_on_STOP(void)
{
	return (ni_readw(AI_Status_1) & AI_STOP_St);
}

void ni_e_mask_and_ack_AI_irq(int irq)
{
	__u16 ack_irq = (irq << 8);
	__u16 en_irq = irq;

	if (irq == AI_IRQ_on_STOP) {
		ack_irq |= (1 << 11);
		en_irq |= (1 << 3);
	}
	ni_win_writew(ack_irq, Interrupt_A_Ack_Register);
	ni_win_writew(en_irq, Interrupt_A_Enable_Register);
}

void daqstc_AO_Reset_All(void)
{
	ni_win_writew(AO_Reset | AO_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(AO_Disarm, AO_Command_1_Register);
	ni_win_writew(0, Interrupt_B_Enable_Register);
	ni_win_writew(AO_BC_Source_Select, AO_Personal_Register);
	ni_win_writew(0x3F98, Interrupt_B_Ack_Register);
	ni_win_writew(AO_Configuration_End, Joint_Reset_Register);
}

void daqstc_AO_Board_Personalize(void)
{
	ni_win_writew(AO_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(0x1410, AO_Personal_Register);
	ni_win_writew(0x1B20, Clock_and_FOUT_Register);
	ni_win_writew(0, AO_Output_Control_Register);
	ni_win_writew(0, AO_START_Select_Register);
	ni_win_writew(AO_Configuration_End, Joint_Reset_Register);
}

void daqstc_AO_LDAC_Source_and_Update_Mode(void)
{
	ni_win_writew(AO_Configuration_Start, Joint_Reset_Register);
	ni_win_writew(0, AO_Command_1_Register);
	ni_win_writew(AO_Configuration_End, Joint_Reset_Register);
}

void ni_e_AOTM_init(void)
{
	daqstc_AO_Reset_All();
	daqstc_AO_Board_Personalize();
	daqstc_AO_LDAC_Source_and_Update_Mode();
}

void ni_e_AO_write(__s16 data, int channel)
{
	ni_writew(data, (channel & 1) ? DAC1_Direct_Data : DAC0_Direct_Data);
}

void ni_e_AI_reset(void)
{
	daqstc_MSC_Clock_Configure();
	ni_e_clear_AI_FIFO();
	daqstc_AI_Reset_All();
	daqstc_AI_Board_Personalize();
	daqstc_AI_Initialize_Configuration_Memory_Output();
}

void ni_e_AO_reset(void)
{
        ni_e_AO_write(0, 0);
        ni_e_AO_write(0, 1);
        daqstc_AO_Reset_All();
}

int ni_e_AI_configure(int Chan, int ChanType, double Gain, int UnipBip, int DitherEn, int LastChan)
{
	__u16 AI_Configuration_High_Mask = 0;
	__u16 AI_Configuration_Low_Mask = 0;
	__u16 bank;
	__u16 ai_gain = 0;
	int iGain = (int)Gain;

	switch (iGain) {
		case 0 :
			ai_gain = 0;
			break;
		case 1 :
			ai_gain = 1;
			break;
		case 2 :
			ai_gain = 2;
			break;
		case 5 :
			ai_gain = 3;
			break;
		case 10 :
			ai_gain = 4;
			break;
		case 20 :
			ai_gain = 5;
			break;
		case 50 :
			ai_gain = 6;
			break;
		case 100 :
			ai_gain = 7;
			break;
		default :
			break;
	}
	if (Chan < 0 || Chan > ni_e->ai->nchan) {
                printk("Error AI_configure: invalid channel number.\n");
                return -EINVAL;
        }
        if (ChanType < 0 || ChanType > 7) {
                printk("Error AI_configure: invalid channel type.\n");
                return -EINVAL;
        }
        if (UnipBip < 0 || UnipBip > 1) {
                printk("Error AI_configure: invalid polarity setting.\n");
                return -EINVAL;
        }
        if (ai_gain == 0 && UnipBip == AI_unipolar) {
                printk("Error AI_configure: invalid configuration.\n");
                return -EINVAL;
        }
        if (DitherEn < 0 || DitherEn > 1) {
                printk("Error AI_configure: invalid dither setting.\n");
                return -EINVAL;
        }
        if (LastChan < 0 || LastChan > 1) {
                printk("Error AI_configure: invalid lastchannel setting.\n");
                return -EINVAL;
        }

	bank = Chan / 16;

/* Clear AI FIFO */

	ni_e_clear_AI_FIFO();

/* Configuration memory high register */

        AI_Configuration_High_Mask = Chan;
        AI_Configuration_High_Mask |= (bank << 4);
        AI_Configuration_High_Mask |= (ChanType << 12);
        ni_writew(AI_Configuration_High_Mask, AI_Config_Mem_High);

/* Configuration memory low register */

        AI_Configuration_Low_Mask = ai_gain;
        AI_Configuration_Low_Mask |= (UnipBip << 8);
        AI_Configuration_Low_Mask |= (DitherEn << 9);
        AI_Configuration_Low_Mask |= (LastChan << 15);
        ni_writew(AI_Configuration_Low_Mask, AI_Config_Mem_Low);

//	ni_win_writew(1, AI_Command_1_Register);

	return 0;
}

int ni_e_AO_configure(int DACSel, int GroundRef, int ExtRef, int ReGlitch, int BipDac)
{
	__u16 AO_Configuration_Mask = 0;
/*
   DACSel = 0/1 -> DAC0/DAC1 selected for actual configuration
   GroundRef = 1 -> connects the reference for both DACs to ground
   ExtRef    = 0 -> internal reference (Vref = + 10 V)
             = 1 -> external reference
   ReGlitch  = 1 -> more uniform glitch
   BipDac    = 0 -> unipolar mode (0/+Vref)
             = 1 -> bipolar mode (-Vref/+Vref)
*/	
	if (DACSel < 0 || DACSel > 1) {
		printk("Error AO_configure: bad argument.\n");
		return -EINVAL;
	}
	if (GroundRef < 0 || GroundRef > 1) {
		printk("Error AO_configure: bad argument.\n");
		return -EINVAL;
	}
	if (ExtRef < 0 || ExtRef > 1) {
		printk("Error AO_configure: bad argument.\n");
		return -EINVAL;
	}
	if (ReGlitch < 0 || ReGlitch > 1) {
		printk("Error AO_configure: bad argument.\n");
		return -EINVAL;
	}
	if (BipDac < 0 || BipDac > 1) {
		printk("Error AO_configure: bad argument.\n");
		return -EINVAL;
	}
	AO_Configuration_Mask = (DACSel << 8) |
				(GroundRef << 3) |
				(ExtRef << 2) |
				(ReGlitch << 1) |
				(BipDac << 0);
	ni_writew(AO_Configuration_Mask, AO_Config);
	return 0;
}

/*
	82C55 digital I/O stuff
	=======================
*/

int ni_e_8255_DIO_configure(int port, int mode)
{
	if (port < 3) {
		port--;
	}
	if (mode == NI_E_8255_OUTPUT) {
		ni_e->dio->CR_mask_8255 &= ~(1 << port);
	} else {
		ni_e->dio->CR_mask_8255 |= (1 << port);
	}
	ni_writeb(ni_e->dio->CR_mask_8255 | 0x80, 0x1f);
	return (int)(ni_e->dio->CR_mask_8255);
}

void ni_e_8255_DO_write_byte(int port, unsigned char data)
{
        switch (port) {
                case PORT_A_8255:
                        ni_writeb(data, 0x19);
                        break;
                case PORT_B_8255:
                        ni_writeb(data, 0x1b);
                        break;
                case PORT_C_low_8255:
                        ni_writeb(data & 0x0f, 0x1d);
                        break;
                case PORT_C_up_8255:
                        ni_writeb((data & 0x0f) << 4, 0x1d);
                        break;
                default:
                        break;
        }      
}

void ni_e_8255_DI_read_byte(unsigned char *data, int port)
{
        switch (port) {
                case PORT_A_8255:
			*data = ni_readb(0x19);
                        break;
                case PORT_B_8255:
			*data = ni_readb(0x1b);
                        break;
                case PORT_C_low_8255:
			*data = ni_readb(0x1d) & 0x0f;
                        break;
                case PORT_C_up_8255:
			*data = (ni_readb(0x1d) & 0xf0) >> 4;
                        break;
                default:
                        break;
        }
}

/*
	DAQ-STC digital I/O stuff
	=========================
*/

int ni_e_daqstc_DIO_configure(int pin, int mode)
{
	if (mode == NI_E_DAQSTC_DIO_OUTPUT) {
		ni_e->dio->CR_mask_daqstc |= (1 << pin);
	} else {
		ni_e->dio->CR_mask_daqstc &= ~(1 << pin);
	}
	ni_win_writew((unsigned short int)ni_e->dio->CR_mask_daqstc, DIO_Control_Register);
	return (int)(ni_e->dio->CR_mask_daqstc);
}

void ni_e_daqstc_DO_write_bit(int bit, int onoff)
{
	if (onoff) {
                ni_e->dio->DO_mask_daqstc |= (1 << bit);
        } else {
                ni_e->dio->DO_mask_daqstc &= ~(1 << bit);
        }
	ni_win_writew((unsigned short int)ni_e->dio->DO_mask_daqstc, DIO_Output_Register);
}

void ni_e_daqstc_DO_reset(void)
{
	ni_e->dio->DO_mask_daqstc = 0;
	ni_win_writew((unsigned short int)ni_e->dio->DO_mask_daqstc, DIO_Output_Register);
}

void ni_e_daqstc_DI_read_bit(unsigned char *bit, unsigned char pin)
{
	*bit = (unsigned char)ni_win_readw(DIO_Parallel_Input_Register) & (1 << pin);
}

/*
	Calibration stuff
	=================
*/

int read_eeprom(int addr)
{
	int bit, bitstring;

	bitstring = 0x0300 | ((addr & 0x100) << 3) | (addr & 0xff);
	ni_writeb_p(0x04, Serial_Command);
	for (bit = 0x8000; bit; bit>>=1) {
		ni_writeb_p(0x04 | ((bit&bitstring) ? 0x02:0), Serial_Command);
		ni_writeb_p(0x05 | ((bit&bitstring) ? 0x02:0), Serial_Command);
	}
	bitstring = 0;
	for (bit = 0x80; bit; bit>>=1) {
		ni_writeb_p(0x04, Serial_Command);
		ni_writeb_p(0x05, Serial_Command);
		bitstring |= ((ni_readb_p(XXX_Status) & 0x01) ? bit:0);
	}
	ni_writeb_p(0x00, Serial_Command);

	return bitstring;
}

void write_caldac_mb88341(int addr, int val)
{
	int bit, bitstring = 0;

	bitstring = ((addr & 0x1) << 11) |
		    ((addr & 0x2) << 9)  |
		    ((addr & 0x4) << 7)  |
		    ((addr & 0x8) << 5)  |
		    (val & 0xff);
		
	for (bit = 1<<11; bit; bit>>=1) {
		ni_writeb_p(((bit&bitstring) ? 0x02:0), Serial_Command);
		ni_writeb_p(1 | ((bit&bitstring) ? 0x02:0), Serial_Command);
	}
	ni_writeb_p(0x08, Serial_Command);
	ni_writeb_p(0, Serial_Command);
}

void ni_e_AI_calibrate(void)
{
	int i, eeprom_data[4];

	if (ni_e->name == "National Instruments PCI-6025E") {
		for (i = 0; i < 4; i++)
			eeprom_data[i] = read_eeprom(430-i);
		write_caldac_mb88341(4, eeprom_data[0]);
		write_caldac_mb88341(11, eeprom_data[1]);
		write_caldac_mb88341(1, eeprom_data[2]);
		write_caldac_mb88341(2, eeprom_data[3]);
	} else {
		for (i = 0; i < 4; i++)
			eeprom_data[i] = read_eeprom(424-i);
		write_caldac_mb88341(4, eeprom_data[0]);
		write_caldac_mb88341(1, eeprom_data[1]);
		write_caldac_mb88341(3, eeprom_data[2]);
		write_caldac_mb88341(14, eeprom_data[2]);
		write_caldac_mb88341(2, eeprom_data[3]);
	}
}

void ni_e_AO_calibrate(int polarity)
{
	int i, eeprom_data[6];
/*
   polarity = 0 -> unipolar AO calibration
   polarity = 1 -> bipolar AO calibration
*/
	if (ni_e->name == "National Instruments PCI-6025E") {
		for (i = 0; i < 6; i++)
			eeprom_data[i] = read_eeprom(426-i);
		write_caldac_mb88341(5, eeprom_data[0]);
		write_caldac_mb88341(7, eeprom_data[1]);
		write_caldac_mb88341(6, eeprom_data[2]);
		write_caldac_mb88341(8, eeprom_data[3]);
		write_caldac_mb88341(10, eeprom_data[4]);
		write_caldac_mb88341(9, eeprom_data[5]);
	} else {
		if (polarity) {
			for (i = 0; i < 6; i++)
				eeprom_data[i] = read_eeprom(420-i);
			write_caldac_mb88341(5, eeprom_data[0]);
			write_caldac_mb88341(7, eeprom_data[1]);
			write_caldac_mb88341(13, eeprom_data[1]);
			write_caldac_mb88341(6, eeprom_data[2]);
			write_caldac_mb88341(8, eeprom_data[3]);
			write_caldac_mb88341(10, eeprom_data[4]);
			write_caldac_mb88341(9, eeprom_data[5]);
		} else {
			for (i = 0; i < 6; i++)
				eeprom_data[i] = read_eeprom(414-i);
			write_caldac_mb88341(5, eeprom_data[0]);
			write_caldac_mb88341(7, eeprom_data[1]);
			write_caldac_mb88341(6, eeprom_data[2]);
			write_caldac_mb88341(8, eeprom_data[3]);
			write_caldac_mb88341(10, eeprom_data[4]);
			write_caldac_mb88341(9, eeprom_data[5]);
		}
	}
}

int init_module(void)
{
	if (!pci_present())
		return -ENODEV;

	if (ni_e_present() < 0) {
		printk(KERN_NOTICE "No supported NI-PCI-ESeries board found\n");
		return -ENODEV;
	}
	ni_e->iobase = ni_e_setup(ni_e);

	if (ni_e->name == "National Instruments PCI-6025E") {
		printk("Board Code : %d\n", (read_eeprom(474) << 8) + (read_eeprom(511)));
	}

	ni_e_AI_calibrate();
	ni_e_AO_calibrate(AO_bipolar);
/*
	ni_e_AO_calibrate(AO_unipolar);
	daqstc_MSC_Clock_Configure();
	ni_e_clear_AI_FIFO();
	daqstc_AI_Reset_All();
	daqstc_AI_Board_Personalize();
	daqstc_AI_Initialize_Configuration_Memory_Output();
*/

	return 0;
}

void cleanup_module(void)
{
	ni_e_unsetup(ni_e);
	ni_e_cleanup(ni_e);
	return;
}
