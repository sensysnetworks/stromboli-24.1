/*
 * Project: rtai_cpp - RTAI C++ Framework
 *
 * File: $Id: com.cc,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $
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
 
#include "com.h"

namespace RTAI {

SerialPort::SerialPort()
:	m_ID(0)
{
}

SerialPort::SerialPort( unsigned int ttyS )
:	m_ID( ttyS )
{
}

SerialPort::~SerialPort()
{
}
	
int SerialPort::init( unsigned int ttyS )
{
	m_ID = ttyS;

	return 0;
}
	
int SerialPort::hwsetup( int base, int irq )
{
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_hwsetup( m_ID, base, irq );
#else
	return -1;
#endif
}

int SerialPort::setup( int baud, int mode,
	   unsigned int parity, unsigned int stopbits,
	   unsigned int wordlength, int fifotrig )
{
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_setup( m_ID, baud, mode, parity, stopbits, wordlength, fifotrig );
#else
	return -1;
#endif
}
		   
int SerialPort::read( char *buffer, int count)
{
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_read( m_ID, buffer, count );
#else
	return -1;
#endif
}

int SerialPort::write( char *buffer, int count )
{
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_write( m_ID, buffer, count );
#else
	return -1;
#endif
}

int SerialPort::clear_input( )
{
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_clear_input( m_ID );
#else
	return -1;
#endif
}

int SerialPort::clear_output( )
{
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_clear_output( m_ID );
#else
	return -1;
#endif
}

int SerialPort::read_modem( int signal )
{
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_read_modem( m_ID, signal );
#else
	return -1;
#endif
}

int SerialPort::write_modem( int signal, int value )
{
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_write_modem( m_ID, signal, value );
#else
	return -1;
#endif
}

int SerialPort::set_mode( int mode){
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_set_mode( m_ID, mode );
#else
	return -1;
#endif
}

int SerialPort::set_fifotrig( int fifotrig)
{
#if defined(CONFIG_RTAI_RTCOM) || defined(CONFIG_RTAI_RTCOM_MODULE)
	return rt_com_set_fifotrig( m_ID, fifotrig );
#else
	return -1;
#endif
}

}; // namespace RTAI
