/*
 * Project: rtai_cpp - RTAI C++ Framework 
 *
 * File: $Id: mbx.cc,v 1.1.1.1 2004/06/06 14:03:06 rpm Exp $
 *
 * Copyright: (C) 2001,2002 Erwin Rol <erwin@muffin.org>
 *
 * Licence:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include "mbx.h"

namespace RTAI {

Mailbox::Mailbox(){
	rt_printk("Mailbox::Mailbox() %p\n",this);
	m_Mailbox = 0;
	m_Owner = true;
	m_Named = false;
}

Mailbox::Mailbox(int size){
	rt_printk("Mailbox::Mailbox(int size=%d) %p\n",size,this);
	m_Mailbox = 0;
	m_Owner = true;
	m_Named = false;
	init(size);
}

Mailbox::Mailbox(const char* name){
	rt_printk("Mailbox::Mailbox(const char* name=%s) %p\n",name,this);
	m_Mailbox = 0;
	m_Owner = false;
	m_Named = true;
	init(name);
}

Mailbox::Mailbox(const char* name, int size){
	rt_printk("Mailbox::Mailbox(const char* name, int size=%d)%p\n",name ,size,this);
	m_Mailbox = 0;
	m_Owner = true;
	m_Named = true;
	init(name,size);
}

Mailbox::~Mailbox(){
	rt_printk("Mailbox::~Mailbox() %p\n",this);

	if(m_Mailbox != 0){
                if( m_Owner ){
                        if( m_Named){ 
                                __rt_named_mbx_delete(m_Mailbox);
                        } else {
                                __rt_mbx_delete(m_Mailbox);
                        }
                }
	}
}

bool Mailbox::init(int size){
	rt_printk("Mailbox::init(int size=%d) %p\n",size,this);
	if(m_Mailbox == 0){
		m_Named = false;
		m_Owner = true;
		m_Mailbox = __rt_mbx_init(size);
		if( m_Mailbox == 0)
			return false;
		else
			return true;
	} else {
		return false;
	}
}

bool Mailbox::init(const char* name, int size){
	rt_printk("Mailbox::init(int size=%d) %p\n",size,this);
	if(m_Mailbox == 0){
		m_Owner = true;
		m_Named = true;
		m_Mailbox = __rt_named_mbx_init(name, size);
		if( m_Mailbox == 0)
			return false;
		else
			return true;
	} else {
		return false;
	}
}

bool Mailbox::init(const char* name)
{
	rt_printk("Mailbox::init() %p\n",this);
	if(m_Mailbox == 0){
		m_Owner = false;
		m_Named = true;
		
		m_Mailbox =  __rt_get_named_mbx(name);
		if( m_Mailbox == 0)
			return false;
		else
			return true;
	} else {
		return false;
	}
}

int Mailbox::send(const void* msg, int msg_size){
	return rt_mbx_send(m_Mailbox,const_cast<void*>(msg),msg_size);
}

int Mailbox::send_wp(const void* msg, int msg_size){
	return rt_mbx_send_wp(m_Mailbox,const_cast<void*>(msg),msg_size);
}

int Mailbox::send_if(const void* msg, int msg_size){
	return rt_mbx_send_if(m_Mailbox,const_cast<void*>(msg),msg_size);
}

int Mailbox::send_until(const void* msg, int msg_size, const Count& time){
	return rt_mbx_send_until(m_Mailbox,const_cast<void*>(msg),msg_size,time);
}

int Mailbox::send_timed(const void* msg, int msg_size, const Count& delay){
	return rt_mbx_send_timed(m_Mailbox,const_cast<void*>(msg),msg_size,delay);
}

int Mailbox::receive(void* msg, int msg_size){
	return rt_mbx_receive(m_Mailbox,msg,msg_size);
}

int Mailbox::receive_wp(void* msg, int msg_size){
	return rt_mbx_receive_wp(m_Mailbox,msg,msg_size);
}

int Mailbox::receive_if(void* msg, int msg_size){
	return rt_mbx_receive_if(m_Mailbox,msg,msg_size);
}

int Mailbox::receive_until(void* msg, int msg_size, const Count& time){
	return rt_mbx_receive_until(m_Mailbox,msg,msg_size,time);
}

int Mailbox::receive_timed(void* msg, int msg_size, const Count& delay){
	return rt_mbx_receive_timed(m_Mailbox,msg,msg_size,delay);
}

}; // namespace RTAI
