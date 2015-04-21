/*
 * Project: rtai_cpp - RTAI C++ Framework 
 *
 * File: $Id: modinfo.c,v 1.1.1.1 2004/06/06 14:03:07 rpm Exp $
 *
 * Copyright: (C) 2001,2002 Erwin Rol <erwin@muffin.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include "modinfo.h"

#include "rtai_cpp.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("C++ simple test");
MODULE_AUTHOR("Erwin Rol <erwin@muffin.org>");


extern void simple_function(void);

int __init simple_init( void )
{
	init_cpp();

	simple_function( );

        return 0;
}

void simple_cleanup( void )
{
	cleanup_cpp();
}

module_init(simple_init)
module_exit(simple_cleanup)
