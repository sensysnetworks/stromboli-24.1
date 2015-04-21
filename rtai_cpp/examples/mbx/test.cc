/*
 * Project: rtai_cpp - RTAI C++ Framework
 *
 * File: $Id: test.cc,v 1.1.1.1 2004/06/06 14:03:07 rpm Exp $
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
 
#include "test.h"

static unsigned long long name[2] = { 0xaaaaaaaaaaaaaaaaLL, 0xbbbbbbbbbbbbbbbbLL };

static Module theModule;

MTask::MTask(int t)
: RTAI::Task(STACK_SIZE,0,0,0,0) 
{
	m_T = t;
}
	
int MTask::run(){
	unsigned long long msg;
	int size;

	while (1) {
		CHECK_FLAGS;
		inc_cpu_use();

		if ( (size = theModule.smbx.send_timed(&name[m_T],RTAI::Count::from_time(TIMEOUT)) ) ) {
			rt_printk("SEND TIMEDOUT %d %d\n", m_T, size);
			goto prem;
		}

		msg = 0;
		if ((size = theModule.rmbx[m_T].receive_timed(&msg,RTAI::Count::from_time(TIMEOUT)))) {
			rt_printk("RECEIVE TIMEDOUT %d %d %x %x<\n", m_T, size, ((int *)&msg)[0], ((int *)&msg)[1]);
			goto prem;
		}

		if (msg != 0xccccccccccccccccLL) {
			rt_printk("WRONG REPLY %d %x %x<\n", m_T, ((int *)&msg)[0], ((int *)&msg)[1]);
			goto prem;
		}
			
		sleep(RTAI::Count::from_time(SLEEP_TIME));
	}
prem: 
	rt_printk("TASK # %d ENDS PREMATURELY\n", m_T);

	return -1;
}


BTask::BTask(int t)
: RTAI::Task(STACK_SIZE,0,0,0,0) 
{
	m_T = t;
}

int BTask::run()
{
	unsigned long long msg;
	unsigned long long name = 0xccccccccccccccccLL;

	while (1) {
		CHECK_FLAGS;
		inc_cpu_use();

		theModule.smbx.receive(&msg);
		if (msg == 0xaaaaaaaaaaaaaaaaLL) {
			m_T = 0;
		} else {
			if (msg == 0xbbbbbbbbbbbbbbbbLL) {
				m_T = 1;
			} else {
				rt_printk("SERVER RECEIVED AN UNKNOWN MSG %x %x<\n", ((int *)&msg)[0], ((int *)&msg)[1]);
				m_T = 0;
				goto prem;
			}
		}
		theModule.rmbx[m_T].send(&name);
	}
prem:
	rt_printk("SERVER TASK ENDS PREMATURELY\n");

	return -1;
}


Module::Module(){
	smbx.init(5);
	rmbx[0].init(1);
	rmbx[1].init(3);

	m_BTask = new BTask(0);
	m_MTask[0] = new MTask(0);
	m_MTask[1] = new MTask(1);

	RTAI::set_oneshot_mode();
	RTAI::start_timer( RTAI::Count::from_time(SLEEP_TIME) );
	m_BTask->resume();
	m_MTask[0]->resume();
	m_MTask[1]->resume();
}

Module::~Module()
{
	RTAI::stop_timer();

	m_BTask->dump_cpu_use();
	m_MTask[0]->dump_cpu_use();
	m_MTask[1]->dump_cpu_use();

	delete m_MTask[1];
	delete m_MTask[0];
	delete m_BTask;
}

