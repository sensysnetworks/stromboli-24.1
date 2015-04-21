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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/ioport.h>

#include <asm/io.h>

#include <rtai.h>
#include <rtai_sched.h>

#include "rtai_parport.h"

struct rt_parport_struct rt_parport_dev;
#define dev rt_parport_dev

void rt_parport_write (int *data_out)
{
	unsigned int irq_flag, handsh_flag, timeout, datum_out;
	RTIME init_trasm;
	int nd, nn, n_nibble, n_write, remainder;

	irq_flag = dev.laplink_mode;
	handsh_flag = dev.laplink_mode;
	n_nibble = (int)(dev.data_size/4); 
	n_write = 0;

	outb_p (irq_flag, dev.io_base); //IRQ Via Ack Line
	init_trasm = rt_get_time_ns ();
	for (nd = 0; nd < dev.n_data; nd++) {
		datum_out = data_out [nd];
		nn = 0;
		do {
			while ((inb(dev.io_base+1) & dev.laplink_mode) ^ handsh_flag) {
				timeout = (unsigned int)(rt_get_time_ns () - init_trasm);
				if (timeout >= TIMEOUT) goto end;
			}
			handsh_flag = dev.laplink_mode - handsh_flag;
			outb_p ((datum_out & 0x000F) + handsh_flag, dev.io_base);
			datum_out = (datum_out >> 4);
			n_write++;
		} while (++nn < n_nibble);

	}
	remainder = n_write%2;
	if (remainder) {
		while ((inb(dev.io_base+1) & dev.laplink_mode)) {
			timeout = rt_get_time_ns () - init_trasm;
			if (timeout >= TIMEOUT) goto end; 
		}
	} else {
		while (!(inb(dev.io_base+1) & dev.laplink_mode)) {
			timeout = rt_get_time_ns () - init_trasm;
			if (timeout >= TIMEOUT) goto end; 
		}
	}

 	end:
		outb_p (0, dev.io_base);

}


void rt_parport_read (int *data_in, int remainder)
{
	unsigned int handsh_flag, timeout, datum_in, shift;
	RTIME init_ric;
	int nd, n_nibble, n_read;

	handsh_flag = dev.laplink_mode;
	n_nibble = (int)(dev.data_size/4);
	n_read = 0; 
	init_ric = rt_get_time_ns ();
	for (nd = 0; nd < dev.n_data; nd++) {
		data_in [nd] = 0;
		shift = 0;
		do {
			outb (handsh_flag, dev.io_base);
			handsh_flag = dev.laplink_mode - handsh_flag;
			while ((inb(dev.io_base+1) & dev.laplink_mode) ^ handsh_flag) {
				timeout = (unsigned int)(rt_get_time_ns () - init_ric);
				if (timeout >= TIMEOUT) return;
			}
			datum_in = inb (dev.io_base+1);
			data_in [nd] += ( ( ((datum_in >> 3) & 0x7)+(((~datum_in) >> 4) & 0x8)) << (shift << 2));
			n_read++;
		} while (++shift < n_nibble);

	}
	remainder = n_read%2;
}


int init_module (void)
{
	dev.io_base   = PARPORT_BASE_ADDRESS;
	dev.port_size = 3;
	printk ("\n\n RTAI_PARPORT: 0x%04x \n\n", dev.io_base);
	if (check_region (dev.io_base, dev.port_size) < 0) {
		printk (" \n\n >>> I/O port conflict <<< \n\n");
		return -EIO;
	}
	request_region (dev.io_base, dev.port_size, "rtai_parport");
	dev.name = "rtai_parport";
	dev.irq = PARPORT_IRQ_NUMBER;
	dev.port_mode = RT_PARPORT_MODE_SPPSPP;
	dev.laplink_mode = RT_PARPORT_LAPLINK;
	dev.data_size = DATA_BITS;
	dev.n_data = NUM_DATA;
	outb (0, dev.io_base);
	return 0;
}


void cleanup_module (void)
{
	release_region (dev.io_base, dev.port_size);
	printk (" \n\n >>> RTAI_PARPORT unloaded <<< \n\n");
}

EXPORT_SYMBOL(rt_parport_dev);
EXPORT_SYMBOL(rt_parport_write);
EXPORT_SYMBOL(rt_parport_read);

