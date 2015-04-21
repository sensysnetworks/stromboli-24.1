/*
COPYRIGHT (C) 2000  Giuseppe Renoldi (giuseppe@renoldi.org)

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


#ifndef _RT_COM_LXRT_H_
#define _RT_COM_LXRT_H_

#include <rtai_declare.h>

#define  FUN_EXT_RT_COM 14

#define _SETUP         0
#define _HWSETUP       1
#define _COM_READ      2
#define _COM_WRITE     3
#define _CLEAR_INPUT   4
#define _CLEAR_OUTPUT  5
#define _READ_MODEM    6
#define _WRITE_MODEM   7
#define _ERROR         8
#define _SET_MODE      9
#define _SET_FIFOTRIG 10


#ifndef __KERNEL__

#include <stdarg.h>
#include <rtai_lxrt.h>
				
DECLARE int rt_com_setup(
	unsigned int ttyS, 
	int baud, 
	int mode, 
	unsigned int parity,
        unsigned int stopbits, 
	unsigned int wordlength, 
	int fifotrig)
{
	struct { unsigned int ttyS; int baud; int mode; unsigned int parity; \
		 unsigned int stopbits; unsigned int wordlength; int fifotrig;} arg = \
		{ ttyS, baud, mode, parity, stopbits, wordlength, fifotrig }; 
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _SETUP, &arg).i[LOW];
}


DECLARE int rt_com_hwsetup( unsigned int ttyS, int port, int irq)
{
	struct { unsigned int ttyS; int port; int irq; } arg = { ttyS, port, irq }; 
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _HWSETUP, &arg).i[LOW];
}


DECLARE int rt_com_read(unsigned int ttyS, char *buffer, int count)
{
	struct { unsigned int ttyS; char *buffer; int count; } arg = \
		{ ttyS, buffer, count };
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _COM_READ, &arg).i[LOW];
}


DECLARE int rt_com_write(unsigned int ttyS, char *buffer, int count)
{
	struct { unsigned int ttyS; char *buffer; int count; } arg = \
		{ ttyS, buffer, count };
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _COM_WRITE, &arg).i[LOW];
}


DECLARE int  rt_com_clear_input( unsigned int ttyS)
{
	struct { unsigned int ttyS; } arg = { ttyS };
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _CLEAR_INPUT, &arg).i[LOW];
}


DECLARE int  rt_com_clear_output( unsigned int ttyS)
{
	struct { unsigned int ttyS; } arg = { ttyS };
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _CLEAR_OUTPUT, &arg).i[LOW];
}


DECLARE int  rt_com_read_modem (unsigned int ttyS, int signal)
{
	struct { unsigned int ttyS; int signal; } arg = { ttyS, signal };
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _READ_MODEM, &arg).i[LOW];
}


DECLARE int  rt_com_write_modem( unsigned int ttyS, int signal, int value )
{
	struct { unsigned int ttyS; int signal; int value; } arg = { ttyS, signal, value };
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _WRITE_MODEM, &arg).i[LOW];
}


DECLARE int  rt_com_error( unsigned int ttyS )
{
	struct { unsigned int ttyS; } arg = { ttyS };
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _ERROR, &arg).i[LOW];
}


DECLARE int  rt_com_set_mode( unsigned int ttyS, int mode )
{
	struct { unsigned int ttyS; int mode; } arg = { ttyS, mode };
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _SET_MODE, &arg).i[LOW];
}


DECLARE int  rt_com_set_fifotrig( unsigned int ttyS, int fifotrig )
{
	struct { unsigned int ttyS; int fifotrig; } arg = { ttyS, fifotrig };
	return rtai_lxrt(FUN_EXT_RT_COM, SIZARG, _SET_FIFOTRIG, &arg).i[LOW];
}


#endif

#endif /* _RT_COM_LXRT_H_ */
