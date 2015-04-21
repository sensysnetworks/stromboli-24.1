/*
 * Project: rtai_cpp - RTAI C++ Framework
 *
 * File: $Id: com.h,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $
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
 

extern "C" {
#include <rt_com.h>
}

#ifndef __COM_H__
#define __COM_H__

namespace RTAI {

/**
 * Serial Port Control 
 */
class SerialPort {
public:
	SerialPort();
	SerialPort( unsigned int ttyS );
	virtual ~SerialPort();
	
	int init( unsigned int ttyS );
	
	int hwsetup( int base, int irq );
	int setup( int baud, int mode,
		   unsigned int parity, unsigned int stopbits,
		   unsigned int wordlength, int fifotrig );
		   
	int read( char *, int );
	int write( char *buffer, int count );
	int clear_input( );
	int clear_output( );
	int read_modem( int signal );
	int write_modem( int signal, int value );
	int set_mode( int mode);
	int set_fifotrig( int fifotrig);
protected:
	unsigned int m_ID;	
};

}; // namespace RTAI

#endif
