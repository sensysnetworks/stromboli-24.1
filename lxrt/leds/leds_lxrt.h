
/*
COPYRIGHT (C) 2000  Pierre Cloutier (pcloutier@PoseidonControls.com)

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

#ifndef _LEDS_LXRT_H_
#define _LEDS_LXRT_H_

#include <rtai_declare.h>

extern void rt_set_leds(int v);
extern void rt_reset_leds(int v);
extern void rt_toggle_leds(int v);
extern void rt_set_leds_port(int port);
extern int  rt_get_leds(void);

// Every module that extends LXRT needs a unique MYIDX (1-15).
#define MYIDX					15

#define SET_LEDS                0
#define RESET_LEDS              1
#define TOGGLE_LEDS             2
#define RT_SET_LEDS_PORT		3
#define RT_GET_LEDS				4

#ifndef __KERNEL__

#include <stdarg.h>
#include <asm/rtai_lxrt.h>

extern union rtai_lxrt_t rtai_lxrt(short int dynx, short int lsize, int srq, void *arg);

#define SIZARG sizeof(arg)

DECLARE void rt_set_leds(int v)
{
	struct { int val; } arg = { v };
	rtai_lxrt(MYIDX, SIZARG, SET_LEDS, &arg);
}

DECLARE void rt_reset_leds(int v)
{
	struct { int val; } arg = { v };
	rtai_lxrt(MYIDX, SIZARG, RESET_LEDS, &arg);
}

DECLARE void rt_toggle_leds(int v)
{
	struct { int val; } arg = { v };
	rtai_lxrt(MYIDX, SIZARG, TOGGLE_LEDS, &arg);
}

DECLARE void rt_set_leds_port(int p)
{
	struct { int val; } arg = { p };
	rtai_lxrt(MYIDX, SIZARG, RT_SET_LEDS_PORT, &arg);
}

DECLARE int rt_get_leds(void)
{
	struct { int val; } arg = { 0 };
	return rtai_lxrt(MYIDX, SIZARG, RT_SET_LEDS_PORT, &arg).i[LOW];
}

#endif /* __KERNEL__ */
#endif // _LEDS_LXRT_H_ 
