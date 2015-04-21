/*
COPYRIGHT (C) 2001  Paolo Mantegazza  (mantegazza@aero.polimi.it)
                    Giuseppe Quaranta (quaranta@aero.polimi.it)

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

#ifndef _SYSAUXCLK_H_
#define _SYSAUXCLK_H_

#include <rtai_nam2num.h>
#include <asm/rtai_srq.h>

#define AUX_CLK_RATE_SET      1
#define AUX_CLK_RATE_GET      2
#define AUX_CLK_CONNECT       3
#define AUX_CLK_ENABLE        4
#define AUX_CLK_DISABLE       5
#define AUX_CLK_OVERUNS       6
#define AUX_CLK_STATUS        7
#define AUX_CLK_CPUFREQ_MHZ   8
#define AUX_CLK_IS_EXT        9
#define AUX_CLK_SEM          10
#define AUX_CLK_REL_SEM      11

#ifdef __KERNEL__

extern long long aux_clk_handler(unsigned int arg);

#define sysAuxClkIsExt(rt_timer_setup) \
	do { \
		struct { int code; int (*rt_timer_setup)(int, void (*)(void)); } args = { AUX_CLK_IS_EXT, rt_timer_setup }; \
		aux_clk_handler((int)(&args)); \
	} while (0)

#else

static volatile int sys_aux_clk_srq;

static inline int get_sys_aux_clk_srq(void)
{
	if (!sys_aux_clk_srq) {
		sys_aux_clk_srq = rtai_open_srq(nam2num("AUXCLK"));
	}
	return sys_aux_clk_srq;
}

static inline int sysAuxClkRateSet(int ticks_per_sec) {
	struct { int code, ticks; } args = { AUX_CLK_RATE_SET, ticks_per_sec };	
	return rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline int sysAuxClkRateGet(void) {
	struct { int code; } args = { AUX_CLK_RATE_GET };	
	return rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline int sysAuxClkConnect(void *fun, int sem) {
	struct { int code, sem; } args = { AUX_CLK_CONNECT, sem };	
	return rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline int sysAuxClkEnable(void) {
	struct { int code; } args = { AUX_CLK_ENABLE };	
	return rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline void sysAuxClkDisable(void) {
	struct { int code; } args = { AUX_CLK_DISABLE };	
	rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline unsigned long sysAuxClkOveruns(void) {
	struct { int code; } args = { AUX_CLK_OVERUNS };	
	return rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline int sysAuxClkStatus(void) {
	struct { int code; } args = { AUX_CLK_STATUS };	
	return rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline int sysAuxClkCpuFreqMHz(void) {
	struct { int code; } args = { AUX_CLK_CPUFREQ_MHZ };	
	return rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline int sysAuxClkIsExt(int (*rt_timer_setup)(int ticks_per_sec, void (*do_at_timer_irq)(void))) {
	struct { int code; int (*rt_timer_setup)(int, void (*)(void)); } args = { AUX_CLK_IS_EXT, rt_timer_setup };	
	return rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline void *sysAuxClkSem(void *task) {
	struct { int code; void *task; } args = { AUX_CLK_SEM, task };
	return (void *)(int)rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

static inline int sysAuxClkRelSem(void *task) {
	struct { int code; void *task; } args = { AUX_CLK_REL_SEM, task };
	return rtai_srq(get_sys_aux_clk_srq(), (unsigned int)&args);
}

#endif

#endif
