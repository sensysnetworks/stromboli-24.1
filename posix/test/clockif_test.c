//////////////////////////////////////////////////////////////////////////////
//
//      Copyright (©) 1999 Zentropic Computing, All rights reserved
//
// Authors:             Steve Papacharalambous (stevep@zentropix.com)
// Original date:       Thu 15 Jul 1999
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
//
// pthreads interface for Real Time Linux.
//
///////////////////////////////////////////////////////////////////////////////
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/sched.h>

#include <asm/signal.h>


#include <asm/rtai.h>
#include <rtai_sched.h>

#define NSECS_PER_SEC 1000000000
#ifdef T_486
#define DEFAULT_TICK_PERIOD 1000000
#else
#define DEFAULT_TICK_PERIOD 100000
#endif

static inline struct timespec timespec_from_ns (long long t)
{
	struct timespec t_spec;
	t_spec.tv_sec = ulldiv(t, NSECS_PER_SEC, &t_spec.tv_nsec);
	if (t_spec.tv_nsec < 0) {
		t_spec.tv_nsec += NSECS_PER_SEC;
		t_spec.tv_sec --;
	}
	return t_spec;
}


static inline void ts_from_ns (long long t, struct timespec *t_spec)
{
	t_spec->tv_sec = ulldiv(t, NSECS_PER_SEC, &t_spec->tv_nsec);
	if (t_spec->tv_nsec < 0) {
		t_spec->tv_nsec += NSECS_PER_SEC;
		t_spec->tv_sec --;
	}
	return;
}




int init_module(void) {

  struct timespec ts_result, sp_ts;

  start_rt_timer((int)nano2count(DEFAULT_TICK_PERIOD));
  ts_result = timespec_from_ns(count2nano(rt_get_time()));
  ts_from_ns(count2nano(rt_get_time()), &sp_ts);

  printk("\nRT Timer sec: %ld, nsec: %ld\n", ts_result.tv_sec,
                                             ts_result.tv_nsec);

  printk("\nTimer 2 sec: %ld, nsec: %ld\n", sp_ts.tv_sec, sp_ts.tv_nsec);

  return(0);

}


void cleanup_module(void) {

  stop_rt_timer();

}
