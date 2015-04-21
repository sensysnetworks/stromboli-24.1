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
 * $Id: hello_world2.c,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $ */



#define __KERNEL__
#define __RT__
#define MODULE


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>

#include <rt_com.h>



/** Init module.
 *
 * Initialize and write to the serial port
 *
 * @author Jochen Küpper
 * @version 2000/04/19 */
int init_module(void)
{
    char hello[] = "Hello World";
    
    if( (0 > rt_com_hwsetup( 0, 0, 0 )) ) {
	printk( KERN_INFO "rt_com test: problems with ttyS0 hardware setup\n" );
	return( -1 );
    } else if( (0 > rt_com_hwsetup( 1, 0, 0 )) ) {
	printk( KERN_INFO "rt_com test: problems with ttyS1 hardware setup\n" );
	rt_com_hwsetup( 0, -1, 0 );
	return( -1 );
    } else if( (0 > rt_com_setup( 0, 19200, 0, RT_COM_PARITY_NONE, 1, 7 ) ) 
	       || (0 > rt_com_setup( 1, 19200, 0, RT_COM_PARITY_NONE, 1, 7 )) ){
	printk( KERN_INFO "rt_com test: problems with setup\n" );
	rt_com_hwsetup( 0, -1, 0 );
	rt_com_hwsetup( 1, -1, 0 );
	return( -1 );
    }
    printk( KERN_INFO "rt_com test: Sending >>%s<<\n", hello );
    rt_com_write( 0, hello, sizeof( hello ) );
    return( 0 );
}



/** Release module.
 *
 * free all allocated resources
 *
 * @author Jochen Küpper
 * @version 2000/04/19 */
void cleanup_module(void)
{
    int count;
    char buffer[ 64 ];
    
    count = rt_com_read( 1, buffer, 63 );
    buffer[count ] = 0;

    printk( KERN_INFO "rt_com test: %d characters read: \"%s\"\n", count, buffer );
	
    rt_com_setup( 0, -1, 0, 0, 0, 0 );
    rt_com_setup( 1, -1, 0, 0, 0, 0 );
    rt_com_hwsetup( 0, -1, 0 );
    rt_com_hwsetup( 1, -1, 0 );

    printk( KERN_INFO "rt_com test: finished\n");
}




/**
 * Local Variables:
 * mode: C
 * c-file-style: "Stroustrup"
 * End:
 */
