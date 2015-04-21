#ifndef _NI_E_REGS_H_
#define _NI_E_REGS_H_

#define _bit15		0x8000
#define _bit14		0x4000
#define _bit13		0x2000
#define _bit12		0x1000
#define _bit11		0x0800
#define _bit10		0x0400
#define _bit9		0x0200
#define _bit8		0x0100
#define _bit7		0x0080
#define _bit6		0x0040
#define _bit5		0x0020
#define _bit4		0x0010
#define _bit3		0x0008
#define _bit2		0x0004
#define _bit1		0x0002
#define _bit0		0x0001

#define Window_Address		0x00
#define Window_Data		0x02

#define Joint_Reset_Register	72
#define AO_Configuration_End	_bit9
#define AI_Configuration_End	_bit8
#define AO_Configuration_Start	_bit5
#define AI_Configuration_Start	_bit4
#define AO_Reset		_bit1
#define AI_Reset		_bit0

#define AO_Command_1_Register		9
#define AO_Analog_Trigger_Reset		_bit15
#define AO_START_Pulse			_bit14
#define AO_Disarm			_bit13
#define AO_UI2_Arm_Disarm		_bit12
#define AO_UI2_Load			_bit11
#define AO_UI_Arm			_bit10
#define AO_UI_Load			_bit9
#define AO_UC_Arm			_bit8
#define AO_UC_Load			_bit7
#define AO_BC_Arm			_bit6
#define AO_BC_Load			_bit5
#define AO_DAC1_Update_Mode		_bit4
#define AO_LDAC1_Source_Select		_bit3
#define AO_DAC0_Update_Mode		_bit2
#define AO_LDAC0_Source_Select		_bit1
#define AO_UPDATE_Pulse			_bit0

#define Interrupt_B_Enable_Register	75
#define AO_FIFO_Interrupt_Enable	_bit8
#define AO_UI2_TC_Interrupt_Enable	_bit7
#define AO_UC_TC_Interrupt_Enable	_bit6
#define AO_Error_Interrupt_Enable	_bit5
#define AO_STOP_Interrupt_Enable	_bit4
#define AO_START_Interrupt_Enable	_bit3
#define AO_UPDATE_Interrupt_Enable	_bit2
#define AO_START1_Interrupt_Enable	_bit1
#define AO_BC_TC_Interrupt_Enable	_bit0

#define Interrupt_B_Ack_Register	3

#define AO_Personal_Register		78
#define AO_FIFO_Flags_Polarity		_bit11
#define AO_FIFO_Enable			_bit10
#define AO_AOFREQ_Polarity		_bit9
#define AO_DMA_PIO_Control		_bit8
#define AO_UPDATE_Original_Pulse	_bit7
#define AO_UPDATE_Pulse_Timebase	_bit6
#define AO_UPDATE_Pulse_Width		_bit5
#define AO_BC_Source_Select		_bit4
#define AO_Interval_Buffer_Mode		_bit3
#define AO_UPDATE2_Original_Pulse	_bit2
#define AO_UPDATE2_Pulse_Timebase	_bit1
#define AO_UPDATE2_Pulse_Width		_bit0

#define AO_Output_Control_Register	86

#define AO_START_Select_Register	66

#define Clock_and_FOUT_Register		56
#define FOUT_Enable			_bit15
#define FOUT_Timebase_Select		_bit14
#define DIO_Serial_Out_Divide_by2	_bit13
#define Slow_Internal_Time_Divide_by2	_bit12
#define Slow_Internal_Timebase		_bit11
#define G_Source_Divide_by2		_bit10
#define Clock_To_Board_Divide_by2	_bit9
#define Clock_To_Board			_bit8
#define AI_Output_Divide_by2		_bit7
#define AI_Source_Divide_by2		_bit6
#define AO_Output_Divide_by2		_bit5
#define AO_Source_Divide_by2		_bit4
#define FOUT_Divider(a)			(a)

#define DIO_Control_Register		11
#define DIO_Output_Register		10
#define DIO_Parallel_Input_Register	7
#define DIO_Serial_Input_Register	28

#define Write_Strobe_0_Register		82
#define Write_Strobe_0			_bit0

#define Write_Strobe_1_Register		83
#define Write_Strobe_1			_bit0

#define Write_Strobe_2_Register		84
#define Write_Strobe_2			_bit0

#define Write_Strobe_3_Register		85
#define Write_Strobe_3			_bit0

#define Interrupt_A_Ack_Register	2
#define AI_Error_Interrupt_Ack		_bit13
#define AI_STOP_Interrupt_Ack		_bit12
#define AI_START_Interrupt_Ack		_bit11
#define AI_START2_Interrupt_Ack		_bit10
#define AI_START1_Interrupt_Ack		_bit9
#define AI_SC_TC_Interrupt_Ack		_bit8

#define Interrupt_A_Enable_Register	73
#define AI_FIFO_Interrupt_Enable	_bit7
#define AI_Error_Interrupt_Enable	_bit5
#define AI_STOP_Interrupt_Enable	_bit4
#define AI_START_Interrupt_Enable	_bit3
#define AI_START2_Interrupt_Enable	_bit2
#define AI_START1_Interrupt_Enable	_bit1
#define AI_SC_TC_Interrupt_Enable	_bit0

#define Interrupt_Control_Register	59


#define AI_Mode_1_Register		12
#define AI_CONVERT_Source_Select(a)	((a)<<11)
#define AI_SI_Source_Select(a)		((a)<<6)
#define AI_CONVERT_Source_Polarity	_bit5
#define AI_SI_Source_Polarity		_bit4
#define AI_Start_Stop			_bit3
#define Reserved_One			_bit2
#define AI_Continuous			_bit1
#define AI_Trigger_Once			_bit0

#define AI_Mode_2_Register		13
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

#define AI_Personal_Register		77

#define AI_Output_Control_Register	60

#define AI_Command_2_Register		4
#define AI_START1_Pulse			_bit0

#define AI_Command_1_Register		8
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

#define AI_Trigger_Select_Register	63
#define AI_START1_Polarity		_bit15
#define AI_START2_Polarity		_bit14
#define AI_START2_Sync			_bit13
#define AI_START2_Edge			_bit12
#define AI_START2_Select(a)		((a)<<7)
#define AI_START1_Sync			_bit6
#define AI_START1_Edge			_bit5
#define AI_START1_Select(a)		(a)

#define AI_Start_Stop_Select_Register	62
#define AI_START_Polarity		_bit15
#define AI_STOP_Polarity		_bit14
#define AI_STOP_Sync			_bit13
#define AI_STOP_Edge			_bit12
#define AI_STOP_Select(a)		((a)<<7)
#define AI_START_Sync			_bit6
#define AI_START_Edge			_bit5
#define AI_START_Select(a)		(a)

#define AI_SI_Load_A_Registers		14
#define AI_SI_Load_B_Registers		16
#define AI_SC_Load_A_Registers		18
#define AI_SC_Load_B_Registers		20
#define AI_SI2_Load_A_Register		23
#define AI_SI2_Load_B_Register		25

#define AI_Mode_3_Register		87

#define INTA_Ack		0x04
#define INTB_Ack		0x06
#define AI_Command_2		0x08
#define AO_Command_2		0x0A
#define G0_Command		0x0C
#define G1_Command		0x0E

#define AI_Status_1		0x04
#define Interrupt_A_St		_bit15
#define AI_FIFO_Full_St		_bit14
#define AI_FIFO_Half_Full_St	_bit13
#define AI_FIFO_Empty_St	_bit12
#define AI_Overrun_St		_bit11
#define AI_Overflow_St		_bit10
#define AI_SC_TC_Error_St	_bit9
#define AI_START2_St		_bit8
#define AI_START1_St		_bit7
#define AI_SC_TC_St		_bit6
#define AI_START_St		_bit5
#define AI_STOP_St		_bit4
#define AI_FIFO_Request_St	_bit1

#define AO_Status_1		0x06
#define G_Status		0x08
#define AI_Status_2		0x0A
#define AO_Status_2		0x0C
#define DIO_Parport		0x0E
#define Serial_Command		0x0D
#define Misc_Command		0x0F
#define XXX_Status		0x01
#define ADC_FIFO_Data		0x1C
#define AI_Config_Mem_Low	0x10
#define AI_Config_Mem_High	0x12
#define AO_Config		0x16
#define DAC_FIFO_Data		0x1E
#define DAC0_Direct_Data	0x18
#define DAC1_Direct_Data	0x1A
#define AI_AO_Select		0x09
#define G0_G1_Select		0x0B
#define Config_Mem_Clear	0x52
#define ADC_FIFO_Clear		0x53
#define DAC_FIFO_Clear		0x54

//#define PORT_A_8255		0x19

#endif
