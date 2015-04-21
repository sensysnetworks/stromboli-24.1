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
 
#include <task.h>
#include <sem.h>
#include <iostream.h>
#include <new.h>

using namespace RTAI;
using namespace std;

class A : public Task {
public:
	A(){ cerr << "A()\n"; }
	virtual ~A(){ cerr << "~A()\n"; }

	virtual void hello(){ cerr << "A::hello()\n"; }

	virtual int run(){ cerr << "A::run()\n"; return 0; }
};

class B : public A {
public:
	B(){ cerr << "B()\n"; }
	virtual ~B(){ cerr << "~B()\n"; }

	virtual void hello(){ cerr << "B::hello()\n"; }

	virtual int run(){ cerr << "B::run()\n"; return 0; }
};


class C {
public:
	C(){ cerr << "C()\n"; }
	~C(){ cerr << "~C()\n"; }
};

class D {
public:
	D(){ cerr << "D()\n"; }
	~D(){ cerr << "~D()\n"; }
protected:
	static C m_c;
};


C D::m_c;

A g_a;
B g_b;

BinarySemaphore g_Lock;

int main(int argc, char* argv[])
{
	cerr << "main begin \n";
	static A s_a;

	A l_a;
	B l_b;

	A* p_a;

	p_a = new B;

	g_a.init(0,0,0,0,0);
	g_b.init(0,0,0,0,0);
	l_a.init(0,0,0,0,0);
	l_b.init(0,0,0,0,0);
	p_a->init(0,0,0,0,0);
	cerr << "init done\n";	

	g_a.resume();
	g_b.resume();
	l_a.resume();
	l_b.resume();
	p_a->resume();
	cerr << "resume done!\n";	

	g_a.hello();
	g_b.hello();
	l_a.hello();
	l_b.hello();
	p_a->hello();
	
	cerr << "hello done\n";

	delete p_a;

	char* buffer = new char[ sizeof( D ) ];
	D* pn_d = new( buffer )D;	
	delete pn_d;

	cerr << "main end \n";

	return 0;
}
