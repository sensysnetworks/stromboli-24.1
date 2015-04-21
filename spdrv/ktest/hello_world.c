/*
 * Copyright (C) 2000 Jochen Küpper.
 * Adapted to the new spdrv version by Paolo Mantegazza.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file License. if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307, USA.
 */


#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#include <rtai_spdrv.h>

static int port = COM1;
MODULE_PARM(port, "i");

int init_module(void)
{
	char hello[] = "Hello World";
    
	if (!rt_spopen(port, 9600, 8, 1, RT_SP_PARITY_NONE, RT_SP_NO_HAND_SHAKE, RT_SP_FIFO_SIZE_8)) {
		rt_spwrite(port, hello, sizeof(hello));
		printk(KERN_INFO "rt_spdrv test: >>%s<< sent.\n", hello);
		return 0;
	}
	return -1;
}

void cleanup_module(void)
{
	rt_spclose(port);
	printk(KERN_INFO "rtai_spdrv test: finished\n");
}
