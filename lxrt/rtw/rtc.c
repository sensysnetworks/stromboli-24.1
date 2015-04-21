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


#include <linux/module.h>
#include <linux/mc146818rtc.h>

#include <asm/rtai.h>
#include "sysAuxClk.h"

static char rtc_freq_select, rtc_control;

static void (*timer_intr_service)(void);

void rtc_intr_handler(void)
{
	timer_intr_service();
 	CMOS_READ(RTC_INTR_FLAGS);
	rt_enable_irq(RTC_IRQ);
}

int rtc_periodic_timer_setup(int freq, void (*do_at_timer_intr)(void))
{
	int pwr2;
	rt_disable_irq(RTC_IRQ);
	rt_free_global_irq(RTC_IRQ);
	hard_cli();
	CMOS_WRITE(rtc_freq_select, RTC_FREQ_SELECT);
	CMOS_WRITE(rtc_control,     RTC_CONTROL);
	hard_sti();
	if (freq > 8192) {
		freq = 8192;
	} else if (freq < 2) {
		freq = 2;
	}
	pwr2 = 1;
	if (freq > 2) {
		while (freq > (1 << pwr2)) {
			pwr2++;
		}
		if (freq <= ((3*(1 << (pwr2 - 1)) + 1)>>1)) {
			pwr2--;
		}
	}	
	timer_intr_service = do_at_timer_intr;
	hard_cli();
	CMOS_WRITE(RTC_REF_CLCK_32KHZ | (16 - pwr2),          RTC_FREQ_SELECT);
	CMOS_WRITE((CMOS_READ(RTC_CONTROL) & 0x8F) | RTC_PIE, RTC_CONTROL);
	hard_sti();
	rt_request_global_irq(RTC_IRQ, rtc_intr_handler);
	rt_enable_irq(RTC_IRQ);
	CMOS_READ(RTC_INTR_FLAGS);
	return (1 << pwr2);
}

int init_module(void)
{
	rtc_freq_select = CMOS_READ(RTC_FREQ_SELECT);
	rtc_control     = CMOS_READ(RTC_CONTROL);
	sysAuxClkIsExt(rtc_periodic_timer_setup);
	printk("\n***** RTC CLOCK IN USE *****\n\n");
	return 0;
}

void cleanup_module(void)
{
	rt_disable_irq(RTC_IRQ);
	rt_free_global_irq(RTC_IRQ);
	hard_cli();
	CMOS_WRITE(rtc_freq_select, RTC_FREQ_SELECT);
	CMOS_WRITE(rtc_control,     RTC_CONTROL);
	hard_sti();
	return;
}
