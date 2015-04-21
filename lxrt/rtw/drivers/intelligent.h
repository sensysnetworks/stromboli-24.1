/*
COPYRIGHT (C) 2001  Paolo Mantegazza (mantegazza@aero.polimi.it)
                    Lorenzo Dozio    (dozio@aero.polimi.it)

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


#ifndef _INTELLIGENT_H_
#define _INTELLIGENT_H_

#ifndef MODULE 
#include <sys/io.h>
#include <sys/time.h>
#include <unistd.h>
#endif
#define KEEP_STATIC_INLINE
#include "sysAuxClk.h"

#define BASE_ADR 0x320
#define DACZER    2047

#define CONV_TIMOUT_us		25	// microseconds
#define SWITCH_TIMOUT_us	2	// microseconds

static int cached_do, conv_time, switch_time, pa_gain, pa_gains[4] = { 0x00, 0x10, 0x20, 0x30 };

static inline unsigned long long rdtsc(void)
{
	long long time;
	__asm__ __volatile__ ("rdtsc": "=A" (time));
	return time;
}

#define mux_switch_nowait(ch) \
	do { outb(((ch) & 0x0F) | pa_gain, BASE_ADR + 0x09); } while (0)

static inline void mux_switch_wait(int chan)
{
        long long end_time;

        mux_switch_nowait(chan);
	end_time = rdtsc() + switch_time;
	while (rdtsc() < end_time);
}

static inline int v2i(float v)
{
	return v*(2048.0/5.0) + 2047.499;
}

static inline float i2v(int iv)
{
	return -5.0 + iv*(10.0/4095.0);
}

static inline void write_dac(int chan, int val)
{
	if (chan) {
		outb(val & 0xFF, BASE_ADR + 0x0E);
		outb(val >> 8,   BASE_ADR + 0x0F);
	} else {
		outb(val & 0xFF, BASE_ADR + 0x0C);
		outb(val >> 8,   BASE_ADR + 0x0D);
	}
	outb(0x00, BASE_ADR + 0x0B);
}

static inline void read_adc(int first_chan, int nchan, int val[])
{
	long long end_time;
	int chan, lowbyte;
        
	for (chan = first_chan; chan < (first_chan + nchan); chan++) {
		outb(0x00, BASE_ADR + 0x0A);
		mux_switch_nowait(chan + 1);
		end_time = rdtsc() + conv_time;
		while (!(inb(BASE_ADR + 0x01) & 1) && rdtsc() < end_time);
		lowbyte = (inb(BASE_ADR + 0x0A)) & 0xFF;		
		val[chan] = ((inb(BASE_ADR + 0x0B) & 0xF) << 8) | lowbyte;
	}
	mux_switch_nowait(first_chan);
}

static inline void set_gain(int gain)
{
	int i;

	for (i = 0; i < 4; i++) {
		if ((1 << i) == gain) {
			pa_gain = pa_gains[gain];
			return;
		}
	}
	pa_gain = pa_gains[0];
}

static inline int get_di(void)
{
	return inb(BASE_ADR + 0x02);
}

static inline int get_di_bit(int bit)
{
	return inb(BASE_ADR + 0x02) & (1 << bit);
}

#define set_do(byte) \
	do { outb(cached_do = byte, BASE_ADR + 0x02); } while(0)

#define set_do_bit(bit)	\
	do { outb(cached_do |= (1 << bit), BASE_ADR + 0x02); } while(0)

#define clear_do_bit(bit) \
	do { outb(cached_do &= ~(1 << bit), BASE_ADR + 0x02); } while(0)

static inline int setup(int first_chan)
{
	int CpuFreqMHz;
	CpuFreqMHz  = sysAuxClkCpuFreqMHz();
	conv_time   = CONV_TIMOUT_us*CpuFreqMHz;
	switch_time = SWITCH_TIMOUT_us*CpuFreqMHz;
	set_do(0x00);
	write_dac(0, DACZER);
	write_dac(1, DACZER);
	if (first_chan >= 0) {
		mux_switch_wait(first_chan);
	}
	return 0;
}

static inline void reset(void)
{
	set_do(0x00);
	write_dac(0, DACZER);
	write_dac(1, DACZER);
	mux_switch_nowait(0);
}

#endif
