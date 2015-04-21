/** rt_com test module
 *  ==================
 *
 * RT-Linux kernel module for communication across serial lines.
 *
 * Copyright (C) 2000 Jochen Küpper
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
 *
 * $Id: hello_world.c,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $ */


#define __KERNEL__
#define __RT__
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#include <rt_com.h>


/** Init module.
 *
 * Initialize and write to the serial port */
int init_module(void)
{
    char hello[] = "Hello World";
    
/*     if( 0 > rt_com_hwsetup( 0, 0, 0 ) ) */
/* 	return( -1 ); */
    if( 0 > rt_com_setup( 0, 9600, 0, RT_COM_PARITY_NONE, 1, 8 ) )
	return( -1 );
    rt_com_write( 0, hello, sizeof( hello ) );
    printk( KERN_INFO "rt_com test: >>%s<< sent.\n", hello );
    return( 0 );
}



/** free all allocated resources */
void cleanup_module(void)
{
    rt_com_setup( 0, -1, 0, 0, 0, 0 );
/*     rt_com_hwsetup( 0, -1, 0 ); */
    printk( KERN_INFO "rt_com test: finished\n");
}




/**
 * Local Variables:
 * mode: C
 * c-file-style: "Stroustrup"
 * End:
 */
