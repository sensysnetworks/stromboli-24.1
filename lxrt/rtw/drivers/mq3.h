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


#ifndef _MQ3_H_
#define _MQ3_H_

#ifndef MODULE
#include <sys/io.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#define base_port	0x320

#define di_port    	base_port + 0x00
#define do_port    	base_port + 0x00
#define dac_cs    	base_port + 0x02
#define ad_cs    	base_port + 0x04
#define status_reg	base_port + 0x06
#define control_reg	base_port + 0x06
#define NUM_PORTS	8

#define AD_MUX_EN	 0x40
#define AD_AUTOZ	 0x80
#define AD_AUTOCAL 	0x100
#define AD_SH		0x200
#define AD_CLOCK_4M	0x400
#define CONTROL_MUST 	(AD_SH | AD_CLOCK_4M)

#define CONV_TIMOUT_us  25 // microseconds

static int conv_time, cached_do;

static inline unsigned long long rdtsc(void)
{
	long long time;
	__asm__ __volatile__ ("rdtsc": "=A" (time));
	return time;
}

static inline int v2i(float v)
{
	return v*(2048.0/5.0) + 2047.499;
}

static inline void write_dac(int chan, int val)
{
	outw(CONTROL_MUST | 0x1800 | chan, control_reg);
	outw(val, dac_cs);
	outw(CONTROL_MUST, control_reg);
	return;
}

static inline float i2v(int iv)
{
	return iv*(5.0/4095.0);
}

static inline void read_adc(int first_chan, int nchan, int val[])
{
	long long end_time;
	int chan, lowbyte;

	for (chan = first_chan; chan < (first_chan + nchan); chan++) {
		outw(CONTROL_MUST | AD_MUX_EN | (chan << 3), control_reg);
		end_time = rdtsc() + conv_time;
		while(!(inw(status_reg) & 0x08) && rdtsc() < end_time);
		outb(0, ad_cs);
		while(!(inw(status_reg) & 0x10) && rdtsc() < end_time);
		lowbyte = inb(ad_cs);
		val[chan] = (inb(ad_cs) << 8) + lowbyte;
	}
	outw(CONTROL_MUST, control_reg);
	return;
}

static inline int ver_adc(void)
{
	long long end_time;
	int chan, retval;

	for (retval = chan = 0; chan < 8; chan++) {
		outw(CONTROL_MUST | AD_MUX_EN | (chan << 3), control_reg);
		end_time = rdtsc() + conv_time;
		while(!(inw(status_reg) & 0x08) && rdtsc() < end_time);
		outb(0, ad_cs);
		while(!(inw(status_reg) & 0x10) && rdtsc() < end_time);
		if (rdtsc() > end_time) {
			retval++;
		}
	}
	outw(CONTROL_MUST, control_reg);
	return retval;
}

static inline int get_di(void)
{ 
	return inw(do_port);
}	

static inline int get_di_bit(int b)
{ 
	return inw(do_port) & (1 << b);
}	

#define set_do(w) \
	do { outw(cached_do = w, do_port); } while(0)

#define set_do_bit(b) \
	do { outw(cached_do |= (1 << b), do_port); } while(0)

static inline int setup(void)
{
	int chan, CpuFreqMHz;

        CpuFreqMHz  = sysAuxClkCpuFreqMHz();
        conv_time   = CONV_TIMOUT_us*CpuFreqMHz;
	set_do(0x00);
	for (chan = 0; chan < 8; chan++) {
		write_dac(chan, 0);
	}
	outw(CONTROL_MUST | AD_AUTOCAL, control_reg);
	end_time = rdtsc() + conv_time;
	while(!(inw(status_reg) & 0x08) && rdtsc() < end_time);
	return ver_adc();
}

static inline void reset(void)
{
	int chan;

	set_do(0x00);
	for (chan = 0; chan < 8; chan++) {
		write_dac(chan, 0);
	}
	outw(CONTROL_MUST, control_reg);
	return;
}

#endif
