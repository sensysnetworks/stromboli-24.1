/*
COPYRIGHT (C) 2002  Paolo Mantegazza (mantegazza@aero.polimi.it),

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


#ifndef _RTAI_BITS_H_
#define _RTAI_BITS_H_

#define BITSIDX  11

#define BITS_INIT        0
#define BITS_DELETE      1
#define BITS_GET         2
#define BITS_RESET       3
#define BITS_SIGNAL      4
#define BITS_WAIT        5
#define BITS_WAIT_IF     6
#define BITS_WAIT_UNTIL  7
#define BITS_WAIT_TIMED  8

#define ALL_SET               0
#define ANY_SET               1
#define ALL_CLR               2
#define ANY_CLR               3

#define ALL_SET_AND_ANY_SET   4
#define ALL_SET_AND_ALL_CLR   5
#define ALL_SET_AND_ANY_CLR   6
#define ANY_SET_AND_ALL_CLR   7
#define ANY_SET_AND_ANY_CLR   8
#define ALL_CLR_AND_ANY_CLR   9

#define ALL_SET_OR_ANY_SET   10
#define ALL_SET_OR_ALL_CLR   11
#define ALL_SET_OR_ANY_CLR   12
#define ANY_SET_OR_ALL_CLR   13
#define ANY_SET_OR_ANY_CLR   14
#define ALL_CLR_OR_ANY_CLR   15

#define SET_BITS              0
#define CLR_BITS              1
#define SET_CLR_BITS          2
#define NOP_BITS              3

#define BITS_ERR     0xFfff  // same as semaphores
#define BITS_TIMOUT  0xFffe  // same as semaphores

#ifdef __KERNEL__

struct rt_bits_struct {
	struct rt_queue queue;  // must be first in struct
	int magic;
	int type;  // to align mask to semaphore count, for easier uspace init
	unsigned long mask;
};

typedef struct rt_bits_struct BITS;

#include <rtai_lxrt.h>

extern void rt_bits_init(BITS *bits, unsigned long mask);

extern int rt_bits_delete(BITS *bits);

extern unsigned long rt_get_bits(BITS *bits);

extern int rt_bits_reset(BITS *bits, unsigned long mask);

extern unsigned long rt_bits_signal(BITS *bits, int setfun, unsigned long masks);

extern int rt_bits_wait(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, unsigned long *resulting_mask);

extern int rt_bits_wait_if(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, unsigned long *resulting_mask);

extern int rt_bits_wait_until(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, RTIME time, unsigned long *resulting_mask);

extern int rt_bits_wait_timed(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, RTIME delay, unsigned long *resulting_mask);

static struct rt_fun_entry rt_bits_fun[] __attribute__ ((__unused__));
static struct rt_fun_entry rt_bits_fun[] = {
	{ 1, rt_bits_init },		//  0
	{ 1, rt_bits_delete },		//  1
	{ 1, rt_get_bits },		//  2
	{ 1, rt_bits_reset },		//  3
	{ 1, rt_bits_signal },		//  4
	{ UW1(6, 0), rt_bits_wait },		//  5
	{ UW1(6, 0), rt_bits_wait_if },		//  6
	{ UW1(8, 0), rt_bits_wait_until },	//  7
	{ UW1(8, 0), rt_bits_wait_timed }	//  8
};

#else

#define KEEP_STATIC_INLINE
#include <rtai_lxrt_user.h>
#include <asm/rtai_lxrt.h>
#include <rtai_lxrt.h>

typedef void *BITS;

extern union rtai_lxrt_t rtai_lxrt(short int dynx, short int lsize, int srq, void *arg);

#define SIZARG sizeof(arg)

#define rt_bits_init(name, mask)  rt_sem_init(name, mask)

#define rt_bits_delete(bits)      rt_sem_delete(bits)

static inline unsigned long rt_get_bits(BITS *bits)
{
	struct { BITS *bits; } arg = { bits };
	return rtai_lxrt(BITSIDX, SIZARG, BITS_GET, &arg).i[LOW];
}

static inline int rt_bits_reset(BITS *bits, unsigned long mask)
{
	struct { BITS *bits; unsigned long mask; } arg = { bits, mask };
	return rtai_lxrt(BITSIDX, SIZARG, BITS_RESET, &arg).i[LOW];
}

static inline unsigned long rt_bits_signal(BITS *bits, int setfun, unsigned long masks)
{
	struct { BITS *bits; int setfun; unsigned long masks; } arg = { bits, setfun, masks };
	return rtai_lxrt(BITSIDX, SIZARG, BITS_SIGNAL, &arg).i[LOW];
}

static inline int rt_bits_wait(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, unsigned long *resulting_mask)
{
	struct { BITS *bits; int testfun; unsigned long testmasks; int exitfun; unsigned long exitmasks; unsigned long *resulting_mask; } arg = { bits, testfun, testmasks, exitfun, exitmasks, resulting_mask };
	return rtai_lxrt(BITSIDX, SIZARG, BITS_WAIT, &arg).i[LOW];
}

static inline int rt_bits_wait_if(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, unsigned long *resulting_mask)
{
	struct { BITS *bits; int testfun; unsigned long testmasks; int exitfun; unsigned long exitmasks; unsigned long *resulting_mask; } arg = { bits, testfun, testmasks, exitfun, exitmasks, resulting_mask };
	return rtai_lxrt(BITSIDX, SIZARG, BITS_WAIT_IF, &arg).i[LOW];
}

static inline int rt_bits_wait_until(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, RTIME time, unsigned long *resulting_mask)
{
	struct { BITS *bits; int testfun; unsigned long testmasks; int exitfun; unsigned long exitmasks; RTIME time; unsigned long *resulting_mask; } arg = { bits, testfun, testmasks, exitfun, exitmasks, time, resulting_mask };
	return rtai_lxrt(BITSIDX, SIZARG, BITS_WAIT_UNTIL, &arg).i[LOW];
}

static inline int rt_bits_wait_timed(BITS *bits, int testfun, unsigned long testmasks, int exitfun, unsigned long exitmasks, RTIME delay, unsigned long *resulting_mask)
{
	struct { BITS *bits; int testfun; unsigned long testmasks; int exitfun; unsigned long exitmasks; RTIME delay; unsigned long *resulting_mask; } arg = { bits, testfun, testmasks, exitfun, exitmasks, delay, resulting_mask };
	return rtai_lxrt(BITSIDX, SIZARG, BITS_WAIT_TIMED, &arg).i[LOW];
}

#endif

#endif
