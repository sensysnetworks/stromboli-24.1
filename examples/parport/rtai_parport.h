/*
COPYRIGHT (C) 2000  Lorenzo Dozio (dozio@aero.polimi.it)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/


#ifndef _PARALLEL_H
#define _PARALLEL_H

#define TIMEOUT				2000000  // < period_trasm !!!
#define NUM_DATA			2
#define DATA_BITS			16
#define LOOPS 				2000

#define PARPORT_BASE_ADDRESS		0x378
#define PARPORT_IRQ_NUMBER		7

#define RT_PARPORT_CONTROL_STROBE    	0x1
#define RT_PARPORT_CONTROL_AUTOFD    	0x2
#define RT_PARPORT_CONTROL_INIT      	0x4
#define RT_PARPORT_CONTROL_SELECT    	0x8
#define RT_PARPORT_CONTROL_ENABLE_IRQ  	0x10
#define RT_PARPORT_CONTROL_DIRECTION 	0x20

#define RT_PARPORT_STATUS_ERROR      	0x8
#define RT_PARPORT_STATUS_SELECT     	0x10
#define RT_PARPORT_STATUS_PAPEROUT   	0x20
#define RT_PARPORT_STATUS_ACK        	0x40
#define RT_PARPORT_STATUS_BUSY       	0x80

#define RT_PARPORT_MODE_PCSPP	        0x0001
#define RT_PARPORT_MODE_PCPS2		0x0002
#define RT_PARPORT_MODE_PCEPP		0x0004
#define RT_PARPORT_MODE_SPPSPP		0x0008
#define RT_PARPORT_MODE_EPPEPP		0x0010

#define RT_PARPORT_LAPLINK_MODE_D4	0x0010
#define RT_PARPORT_LAPLINK_MODE_D5	0x0020
#define RT_PARPORT_LAPLINK_MODE_D6	0x0040
#define RT_PARPORT_LAPLINK_MODE_D7	0x0080
#define RT_PARPORT_LAPLINK		RT_PARPORT_LAPLINK_MODE_D6	


struct rt_parport_struct {
	unsigned int io_base;  		// parallel port base address
	unsigned int port_size; 	// number of parallel port addresses 
	const char *name;		// device name
	int irq;			// irq number
	void (*handler)(void);		// irq handler
	unsigned int port_mode; 	// parallel port mode (SPP, EPP, ...)
	unsigned int laplink_mode;	// laplink cable handshaking_bit 
	int data_size;			// number of data bits
	int n_data;			// number of data to send or receive
	unsigned int flags;		// parport flag (read or write)
};

extern struct rt_parport_struct rt_parport_dev;

extern void rt_parport_write (int *data_out);
extern void rt_parport_read (int *data_in, int remainder);

#endif
