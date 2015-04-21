
/* Copyright (C) 2001  Pierre Cloutier 	<pcloutier@PoseidonControlds.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 
**
** $Id: rtai_lxrt.cc,v 1.1.1.1 2004/06/06 14:02:38 rpm Exp $
**
*/

#include <sys/types.h>
#include <strstream>
#include <climits>
#include <algorithm>
#include <octave/oct.h>
#include <octave/lo-ieee.h>

extern "C" {
    #include <sys/types.h>
    #include <sched.h>
    #include <rtai_lxrt_user.h>
}

DEFUN_DLD (lock_all, args, ,
    "\n\
extern int lock_all(int stacksize, int heapsize);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin!=2 || !args(0).is_real_scalar() || !args(1).is_real_scalar()) {
    print_usage ("lock_all");
    }
else {
    int err, stk, hep;

    stk = (int) args(0).double_value();
    hep = (int) args(1).double_value();
    err = lock_all(stk, hep);
	retval(0)=octave_value(static_cast<double>(err));

    }

return retval;
}

DEFUN_DLD (init_linux_scheduler, args, ,
    "\n\
extern void init_linux_scheduler(int sched, int pri);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin!=2 || !args(0).is_real_scalar() || !args(1).is_real_scalar()) {
    print_usage ("init_linux_scheduler");
    }
else {
    int sch, pri;

    sch = (int) args(0).double_value();
    pri = (int) args(1).double_value();
	init_linux_scheduler(sch, pri);
	}
return retval;
}

DEFUN_DLD (rtai_print_to_screen, args, ,
    "\n\
extern int rtai_print_to_screen(string);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin != 1 || !args(0).is_string()) {
    print_usage ("rtai_print_to_screen");
    }
else {
	const char *str;
	string s;
	int len;

	s   = args(0).string_value();
	str = s.c_str();
    len = rtai_print_to_screen(str);
    retval(0)=octave_value(static_cast<double>(len));
	}
return retval;
}

DEFUN_DLD (rt_set_oneshot_mode, args, ,
    "\n\
extern void rt_set_oneshot_mode(void);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin!=0) {
    print_usage ("rt_set_oneshot_mode");
    }
else {
	rt_set_oneshot_mode();
	}
return retval;
}

DEFUN_DLD (start_rt_timer, args, ,
    "\n\
extern RTIME start_rt_timer(int period);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin!=1 || !args(0).is_real_scalar()) {
    print_usage ("start_rt_timer");
    }
else {
	int per;
	RTIME rtime;

    per = (int) args(0).double_value();
	rtime = start_rt_timer(per);
    retval(0)=octave_value(static_cast<double>(rtime));
	}
return retval;
}

DEFUN_DLD (nam2num, args, ,
    "\n\
extern unsigned long nam2num(const char *name);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin!=1 || !args(0).is_string()) {
    print_usage ("nam2num");
    }
else {
    const char *str;
    string s;
    unsigned long num;

    s   = args(0).string_value();
    str = s.c_str();
    num = nam2num(str);
    retval(0)=octave_value(static_cast<double>(num));
	}
return retval;
}

DEFUN_DLD (rt_task_init, args, ,
    "\n\
extern RT_TASK *rt_task_init(int name, int priority, int stack_size, int max_msg_size);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin!=4 || !args(0).is_real_scalar() || !args(1).is_real_scalar() || !args(2).is_real_scalar() || !args(3).is_real_scalar()) {
    print_usage ("rt_task_init");
    }
else {
	int nam, pri, stk, msgs;
	int tsk;

	nam = (int) args(0).double_value();
	pri = (int) args(1).double_value();
	stk = (int) args(2).double_value();
	msgs= (int) args(3).double_value();

	tsk = (int) rt_task_init(nam, pri, stk, msgs);
	retval(0)=octave_value(static_cast<double>(tsk));
    }
return retval;
}

DEFUN_DLD (rt_task_delete, args, ,
    "\n\
extern int rt_task_delete(RT_TASK *task);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin!=1 || !args(0).is_real_scalar()) {
    print_usage ("rt_task_delete");
    }
else {
	RT_TASK *tsk;
    int err, tmp;

    tmp = (int) args(0).double_value();
	tsk = (RT_TASK *) tmp;
	err = rt_task_delete(tsk);
    retval(0)=octave_value(static_cast<double>(err));
	}
return retval;
}

DEFUN_DLD (rt_sleep, args, ,
    "\n\
extern void rt_sleep(RTIME delay);\n")
{
RTIME t    = 0;
int nargin = args.length();
octave_value_list retval;

if (nargin!=1 || !args(0).is_real_scalar()) {
	print_usage ("rt_sleep");
	}
else {
	t = (long long) args(0).double_value();
	rt_sleep(t);
	}

return retval;
}

DEFUN_DLD (rt_make_soft_real_time, args, ,
    "\n\
void rt_make_soft_real_time(void)\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin) {
    print_usage ("rt_make_soft_real_time");
    }
else {
	rt_make_soft_real_time();
	}
return retval;
}

DEFUN_DLD (rt_make_hard_real_time, args, ,
    "\n\
void rt_make_hard_real_time(void)\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin) {
    print_usage ("rt_make_hard_real_time");
    }
else {
    rt_make_hard_real_time();
    }
return retval;
}

DEFUN_DLD (rt_task_suspend, args, ,
    "\n\
extern int rt_task_suspend(RT_TASK *task);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin!=1 || !args(0).is_real_scalar()) {
    print_usage ("rt_task_suspend");
    }
else {
    RT_TASK *tsk;
    int err, tmp;

    tmp = (int) args(0).double_value();
    tsk = (RT_TASK *) tmp;
    err = rt_task_suspend(tsk);
    retval(0)=octave_value(static_cast<double>(err));
    }
return retval;
}

DEFUN_DLD (rt_task_resume, args, ,
    "\n\
extern int rt_task_resume(RT_TASK *task);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin!=1 || !args(0).is_real_scalar()) {
    print_usage ("rt_task_resume");
    }
else {
    RT_TASK *tsk;
    int err, tmp;

    tmp = (int) args(0).double_value();
    tsk = (RT_TASK *) tmp;
    err = rt_task_resume(tsk);
    retval(0)=octave_value(static_cast<double>(err));
    }
return retval;
}

DEFUN_DLD (rt_get_time, args, ,
    "\n\
extern RTIME rt_get_time(void);\n")
{
int nargin = args.length();
octave_value_list retval;

if (nargin) {
    print_usage ("rt_get_time");
    }
else {
    RTIME rtime;
    rtime = rt_get_time();
    retval(0)=octave_value(static_cast<double>(rtime));
    }
return retval;
}
