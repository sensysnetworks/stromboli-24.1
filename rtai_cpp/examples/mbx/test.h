/*
 * Project: rtai_cpp - RTAI C++ Framework
 *
 * File: $Id: test.h,v 1.1.1.1 2004/06/06 14:03:07 rpm Exp $
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
 
#include <task.h>
#include <mbx.h>
#include <time.h>

//#define VERIFY_FLAGS

#ifdef  VERIFY_FLAGS
#define CHECK_FLAGS \
	{ \
		unsigned long flags; \
		rt_global_save_flags(&flags); \
		if (flags != ((1 << IFLAG) | 1)) { \
			rt_printk("<<<<<<<<<< FLAGS: %lx >>>>>>>>>>\n", flags); \
		} \
	}
#else
#define CHECK_FLAGS
#endif

#define TIMEOUT        1000000

#define SLEEP_TIME      200000

#define STACK_SIZE 2000


class MTask : public RTAI::Task {
public:
	MTask(int t);
	int run();
protected:
	int m_T;
};

class BTask : public RTAI::Task {
public:
	BTask(int t);
	int run();
protected:
	int m_T;	
};

class Module {
public:
	Module();
	~Module();
protected:
	BTask*	m_BTask;
	MTask*	m_MTask[2];	
public:
	RTAI::MailboxT<unsigned long long> smbx, rmbx[2];
};
