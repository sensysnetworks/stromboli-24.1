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

#ifndef NI_PCI_H
#define NI_PCI_H

#define NI_PCI_DEBUG

#define MAX_NI_BOARDS	5

#define VENDOR_ID_NI    		0x1093
#define DEVICE_ID_PCI_6071E     	0x1350
#define DEVICE_ID_PCI_MIO_16E_1		0x1180
#define DEVICE_ID_PCI_MIO_16E_4		0x1190
#define DEVICE_ID_PCI_6023E		0x2a60
#define DEVICE_ID_PCI_6024E		0x2a70
#define DEVICE_ID_PCI_6025E		0x2a80
#define DEVICE_ID_PCI_6711		0x1880
#define DEVICE_ID_PCI_6713		0x1870
#define DEVICE_ID_END			0xffff

struct ni_ai_struct {
	int n_channels;
	int n_bits;
	int max_sampling_rate;
	int *caldac_addr;
	int eeprom_base;
};

struct ni_ao_struct {
	int n_channels;
	int n_bits;
	int max_update_rate;
	int *caldac_addr;
	int eeprom_base;
	void (*write)(signed short int, int, unsigned int);
};

struct ni_dio_struct {
	int has_82C55;
	unsigned char CR_mask_82C55;
	unsigned char CR_mask_dio_module;
	unsigned char DO_mask_dio_module;
};

struct ni_specs_struct {
	unsigned short device_id;
	char *name;
	struct ni_ai_struct ai;
	struct ni_ao_struct ao;
	struct ni_dio_struct dio;
	void (*calibrate)(unsigned int);
	void (*write_caldac)(int, int, int, unsigned int);
};

struct ni_pci_struct {
	int slot;
	int index;
	unsigned long mite_start;
	unsigned long daq_start;
	unsigned long mite_end;
	unsigned long daq_end;
	unsigned int irq_line;
	void *mite_io_addr;
	void *daq_io_addr;
	int iobase;
	struct ni_specs_struct *subdev;
};

#define AO_unipolar     0
#define AO_bipolar      1
#define AO_int_ref      0
#define AO_ext_ref      1
#define AO_Vmax		10.0

#define AI_unipolar     1
#define AI_bipolar      0
#define AI_gain_05      0
#define AI_gain_1       1
#define AI_gain_2       2
#define AI_gain_5       3
#define AI_gain_10      4
#define AI_gain_20      5
#define AI_gain_50      6
#define AI_gain_100     7
#define AI_dither_on    1
#define AI_dither_off   0
#define AI_calibration  0
#define AI_differential 1
#define AI_NRSE         2
#define AI_RSE          3
#define AI_Aux          5
#define AI_Ghost        7

#define NI_DIO_MODULE_OUTPUT	1
#define NI_DIO_MODULE_INPUT	0
#define NI_82C55_OUTPUT		0
#define NI_82C55_INPUT		1
#define PORT_A_82C55		4
#define PORT_C_up_82C55		3
#define PORT_B_82C55		2
#define PORT_C_low_82C55	1

static inline signed short int ni_v2i(double fval, int polarity)
{
/*
   polarity = 0 -> AO unipolar mode
   polarity = 1 -> AO bipolar mode
*/
        signed short int ival;
        if (polarity == AO_unipolar) {
                if (fval > AO_Vmax) fval = AO_Vmax;
                if (fval < 0.) fval = 0.;
        } else {
                if (fval > AO_Vmax) fval = AO_Vmax;
                if (fval < -AO_Vmax) fval = -AO_Vmax;
        }
        ival = (signed short int)((fval/10.)*(4095./(polarity+1)));
        return ival;
}

static inline double ni_i2v(signed short int ival, double gain, int polarity)
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

extern void ni_calibrate(unsigned int);

extern int ni_AI_init(unsigned int);
extern int ni_AI_configure(int, int, int, int, int, int, unsigned int);
extern int ni_AI_read(signed short int *, int, unsigned int);
extern int ni_AI_reset(unsigned int);
extern int ni_AI_clear_configuration_FIFO(unsigned int);

extern int ni_AO_init(unsigned int);
extern int ni_AO_configure(unsigned int, int, int, int, unsigned int);
extern int ni_AO_write(signed short int, int, unsigned int);
extern int ni_AO_reset(unsigned int);
extern int ni_671x_AO_set_immediate_mode(unsigned int);

extern int ni_82C55_configure(int, int, unsigned int);
extern int ni_82C55_write_byte(int, unsigned char, unsigned int);
extern int ni_82C55_read_byte(unsigned char *, int, unsigned int);
extern int ni_DIO_module_configure(int, int, unsigned int);
extern int ni_DIO_module_reset(unsigned int);
extern int ni_DIO_module_write_bit(int, int, unsigned int);
extern int ni_DIO_module_read_bit(unsigned char *, unsigned char, unsigned int);

extern void ni_get_board_list_index(unsigned short, int *);
extern void ni_get_board_n_ai_channels(unsigned short, int *);
extern void ni_get_board_n_ao_channels(unsigned short, int *);
extern int ni_get_board_device_id(unsigned int, unsigned short *);
extern void ni_get_n_boards(int *);

#define _bit15          0x8000
#define _bit14          0x4000
#define _bit13          0x2000
#define _bit12          0x1000
#define _bit11          0x0800
#define _bit10          0x0400
#define _bit9           0x0200
#define _bit8           0x0100
#define _bit7           0x0080
#define _bit6           0x0040
#define _bit5           0x0020
#define _bit4           0x0010
#define _bit3           0x0008
#define _bit2           0x0004
#define _bit1           0x0002
#define _bit0           0x0001

#define Serial_Command_Register			0x0d
#define Misc_Command_Register			0x0f
#define Status_Register				0x01
#define ADC_FIFO_Data_Register			0x1c
#define Configuration_Memory_Low_Register	0x10
#define Configuration_Memory_High_Register	0x12
#define AO_Configuration_Register		0x16
#define DAC_FIFO_Data_Register			0x1e
#define DAC0_Direct_Data_Register		0x18
#define DAC1_Direct_Data_Register		0x1a
#define AI_AO_Select_Register			0x09
#define G0_G1_Select_Register			0x0b

#define Waddr_671x				0x18
#define Wdata_671x				0x1e
#define AO_Timed_Register			0x10
#define AO_Immediate_Register			0x11
#define AO_FIFO_Offset_Load_Register		0x13
#define AO_Waveform_Generation_Register		0x15
#define AO_Misc_Register			0x16
#define AO_Cal_Chan_Sel_Register		0x17
#define AO_Config_Register			0x18

#define Waddr_daqstc				0x00
#define Wdata_daqstc				0x02

#define Joint_Reset_Register    		72
#define AO_Configuration_End    	_bit9
#define AI_Configuration_End    	_bit8
#define AO_Configuration_Start  	_bit5
#define AI_Configuration_Start  	_bit4
#define AO_Reset                	_bit1
#define AI_Reset                	_bit0

#define AO_Command_1_Register           	9
#define AO_Analog_Trigger_Reset         _bit15
#define AO_START_Pulse                  _bit14
#define AO_Disarm                       _bit13
#define AO_UI2_Arm_Disarm               _bit12
#define AO_UI2_Load                     _bit11
#define AO_UI_Arm                       _bit10
#define AO_UI_Load                      _bit9
#define AO_UC_Arm                       _bit8
#define AO_UC_Load                      _bit7
#define AO_BC_Arm                       _bit6
#define AO_BC_Load                      _bit5
#define AO_DAC1_Update_Mode             _bit4
#define AO_LDAC1_Source_Select          _bit3
#define AO_DAC0_Update_Mode             _bit2
#define AO_LDAC0_Source_Select          _bit1
#define AO_UPDATE_Pulse                 _bit0

#define Interrupt_B_Enable_Register     	75
#define AO_FIFO_Interrupt_Enable        _bit8
#define AO_UI2_TC_Interrupt_Enable      _bit7
#define AO_UC_TC_Interrupt_Enable       _bit6
#define AO_Error_Interrupt_Enable       _bit5
#define AO_STOP_Interrupt_Enable        _bit4
#define AO_START_Interrupt_Enable       _bit3
#define AO_UPDATE_Interrupt_Enable      _bit2
#define AO_START1_Interrupt_Enable      _bit1
#define AO_BC_TC_Interrupt_Enable       _bit0

#define Interrupt_B_Ack_Register        	3

#define AO_Personal_Register            	78
#define AO_FIFO_Flags_Polarity          _bit11
#define AO_FIFO_Enable                  _bit10
#define AO_AOFREQ_Polarity              _bit9
#define AO_DMA_PIO_Control              _bit8
#define AO_UPDATE_Original_Pulse        _bit7
#define AO_UPDATE_Pulse_Timebase        _bit6
#define AO_UPDATE_Pulse_Width           _bit5
#define AO_BC_Source_Select             _bit4
#define AO_Interval_Buffer_Mode         _bit3
#define AO_UPDATE2_Original_Pulse       _bit2
#define AO_UPDATE2_Pulse_Timebase       _bit1
#define AO_UPDATE2_Pulse_Width          _bit0

#define AO_Output_Control_Register      	86

#define AO_START_Select_Register        	66

#define Clock_and_FOUT_Register         	56
#define FOUT_Enable                     _bit15
#define FOUT_Timebase_Select            _bit14
#define DIO_Serial_Out_Divide_by2       _bit13
#define Slow_Internal_Time_Divide_by2   _bit12
#define Slow_Internal_Timebase          _bit11
#define G_Source_Divide_by2             _bit10
#define Clock_To_Board_Divide_by2       _bit9
#define Clock_To_Board                  _bit8
#define AI_Output_Divide_by2            _bit7
#define AI_Source_Divide_by2            _bit6
#define AO_Output_Divide_by2            _bit5
#define AO_Source_Divide_by2            _bit4
#define FOUT_Divider(a)                 (a)

#define DIO_Control_Register			11
#define DIO_Output_Register			10
#define DIO_Parallel_Input_Register		7
#define DIO_Serial_Input_Register		28

#define Write_Strobe_0_Register			82
#define Write_Strobe_0			_bit0
#define Write_Strobe_1_Register			83
#define Write_Strobe_1			_bit0
#define Write_Strobe_2_Register			84
#define Write_Strobe_2			_bit0
#define Write_Strobe_3_Register			85
#define Write_Strobe_3			_bit0

#define Interrupt_A_Ack_Register		2
#define AI_Error_Interrupt_Ack		_bit13
#define AI_STOP_Interrupt_Ack		_bit12
#define AI_START_Interrupt_Ack		_bit11
#define AI_START2_Interrupt_Ack		_bit10
#define AI_START1_Interrupt_Ack		_bit9
#define AI_SC_TC_Interrupt_Ack		_bit8

#define Interrupt_A_Enable_Register		73
#define AI_FIFO_Interrupt_Enable	_bit7
#define AI_Error_Interrupt_Enable	_bit5
#define AI_STOP_Interrupt_Enable	_bit4
#define AI_START_Interrupt_Enable	_bit3
#define AI_START2_Interrupt_Enable	_bit2
#define AI_START1_Interrupt_Enable	_bit1
#define AI_SC_TC_Interrupt_Enable	_bit0

#define Interrupt_Control_Register		59

#define AI_Mode_1_Register			12
#define AI_CONVERT_Source_Select(a)	((a)<<11)
#define AI_SI_Source_Select(a)		((a)<<6)
#define AI_CONVERT_Source_Polarity	_bit5
#define AI_SI_Source_Polarity		_bit4
#define AI_Start_Stop			_bit3
#define Reserved_One			_bit2
#define AI_Continuous			_bit1
#define AI_Trigger_Once			_bit0

#define AI_Mode_2_Register			13
#define AI_SC_Gate_Enable		_bit15
#define AI_Start_Stop_Gate_Enable	_bit14
#define AI_Pre_Trigger			_bit13
#define AI_External_MUX_Present		_bit12
#define AI_SI2_Initial_Load_Source	_bit9
#define AI_SI2_Reload_Mode		_bit8
#define AI_SI_Initial_Load_Source	_bit7
#define AI_SI_Reload_Mode(a)		((a)<<4)
#define AI_SI_Write_Switch		_bit3
#define AI_SC_Initial_Load_Source	_bit2
#define AI_SC_Reload_Mode		_bit1
#define AI_SC_Write_Switch		_bit0

#define AI_Personal_Register			77

#define AI_Output_Control_Register		60

#define AI_Command_2_Register			4
#define AI_START1_Pulse			_bit0

#define AI_Command_1_Register			8
#define AI_Analog_Trigger_Reset		_bit14
#define AI_Disarm			_bit13
#define AI_SI2_Arm			_bit12
#define AI_SI2_Load			_bit11
#define AI_SI_Arm			_bit10
#define AI_SI_Load			_bit9
#define AI_DIV_Arm			_bit8
#define AI_DIV_Load			_bit7
#define AI_SC_Arm			_bit6
#define AI_SC_Load			_bit5
#define AI_SCAN_IN_PROG_Pulse		_bit4
#define AI_EXTMUX_CLK_Pulse		_bit3
#define AI_LOCALMUX_CLK_Pulse		_bit2
#define AI_SC_TC_Pulse			_bit1
#define AI_CONVERT_Pulse		_bit0

#define AI_Trigger_Select_Register		63
#define AI_START1_Polarity		_bit15
#define AI_START2_Polarity		_bit14
#define AI_START2_Sync			_bit13
#define AI_START2_Edge			_bit12
#define AI_START2_Select(a)		((a)<<7)
#define AI_START1_Sync			_bit6
#define AI_START1_Edge			_bit5
#define AI_START1_Select(a)		(a)

#define AI_Start_Stop_Select_Register		62
#define AI_START_Polarity		_bit15
#define AI_STOP_Polarity		_bit14
#define AI_STOP_Sync			_bit13
#define AI_STOP_Edge			_bit12
#define AI_STOP_Select(a)		((a)<<7)
#define AI_START_Sync			_bit6
#define AI_START_Edge			_bit5
#define AI_START_Select(a)		(a)

#define AI_SI_Load_A_Registers			14
#define AI_SI_Load_B_Registers			16
#define AI_SC_Load_A_Registers			18
#define AI_SC_Load_B_Registers			20
#define AI_SI2_Load_A_Register			23
#define AI_SI2_Load_B_Register			25

#define AI_Mode_3_Register			87

#define AI_Status_1				0x04
#define Interrupt_A_St			_bit15
#define AI_FIFO_Full_St			_bit14
#define AI_FIFO_Half_Full_St		_bit13
#define AI_FIFO_Empty_St		_bit12
#define AI_Overrun_St			_bit11
#define AI_Overflow_St			_bit10
#define AI_SC_TC_Error_St		_bit9
#define AI_START2_St			_bit8
#define AI_START1_St			_bit7
#define AI_SC_TC_St			_bit6
#define AI_START_St			_bit5
#define AI_STOP_St			_bit4
#define AI_FIFO_Request_St		_bit1

#endif
