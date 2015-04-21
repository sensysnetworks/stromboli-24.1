/*
 * Project: rtai_cpp - RTAI C++ Framework
 *
 * Copyright: (C) 2001,2002 Peter Soetens <peter.soetens@mech.kuleuven.ac.be>
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

/**
 * I use this file to support the C++ 3.X compilers. It is quite similar
 * to builtins.c, but this file is compiled with a C++ compiler instead of a 
 * C compiler
 */
 
#include "rtai_wrapper.h"
extern "C"
{
#include "rt_mem_mgr.h"
}
#if __GNUC__ < 3
 /* use __builtin_delete() */
#else
void operator delete(void* vp)
{
	rt_printk("__builtin_delete %p\n",vp);
	if(vp != 0)
		rt_free(vp);
}

#endif
