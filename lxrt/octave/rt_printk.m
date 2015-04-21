
## Copyright (C) 2001  Pierre Cloutier  <pcloutier@PoseidonControlds.com>
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
##
## $Id: rt_printk.m,v 1.1.1.1 2004/06/06 14:02:38 rpm Exp $
##

function retval = rt_printk (fmt, ...)

	retval = 0;

	if (nargin > 0) 
		s = sprintf(fmt, all_va_args);
		retval = rtai_print_to_screen(s);
	else 
		usage ("rt_printk (fmt, ...)");
	endif

endfunction

