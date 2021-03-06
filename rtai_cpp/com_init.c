/*
 * Project: rtai_cpp - RTAI C++ Framework
 *
 * File: $Id: com_init.c,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $
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
#include <linux/sched.h>
#include <asm/unistd.h>

#include "rt_mem_mgr.h"
#include "rtai_wrapper.h"

// RTAI-- MODULE INIT and CLEANUP functions

MODULE_LICENSE("GPL");
MODULE_AUTHOR("the RTAI-Team (contact person Erwin Rol)");
MODULE_DESCRIPTION("RTAI C++ COM support");

int __init rtai_cpp_com_init(void){
	return 0;
}

void rtai_cpp_com_cleanup(void)
{
}

module_init(rtai_cpp_com_init)
module_exit(rtai_cpp_com_cleanup)
  
