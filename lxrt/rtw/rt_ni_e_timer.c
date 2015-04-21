/*
Copyright (C) 2002 Lorenzo Dozio (dozio@aero.polimi.it)

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/module.h>
#include <rtai.h>
#include "sysAuxClk.h"

#include "drivers/ni_e.h"

extern struct ni_e_dev *ni_e;

static void (*timer_intr_service)(void);

volatile int nirq;

void ni_e_irq_handler(void)
{
        if (ni_e_AI_IRQ_is_on_STOP()) {
                ni_e_mask_and_ack_AI_irq(AI_IRQ_on_STOP);
		timer_intr_service();
		nirq++;
        }
}

int ni_e_timer_setup(int freq, void (*do_at_timer_intr)(void))
{
        __u32 PERIOD = (__u32)(1000000000 + (freq >> 1) - 1)/freq;

	rt_shutdown_irq(ni_e->irq_line);
	rt_free_global_irq(ni_e->irq_line);
	timer_intr_service = do_at_timer_intr;
        ni_e_AITM_init(PERIOD, AI_IRQ_on_STOP, 0);
        ni_e_AOTM_init();
	rt_request_global_irq(ni_e->irq_line, ni_e_irq_handler);
	rt_startup_irq(ni_e->irq_line);
        ni_e_AI_start();
	printk("Period [ns] : %d\n", PERIOD);
	return freq;
}

int init_module(void)
{
	sysAuxClkIsExt(ni_e_timer_setup);
	return 0;
}

void cleanup_module(void)
{
	rt_shutdown_irq(ni_e->irq_line);
	rt_free_global_irq(ni_e->irq_line);
	printk("Number of irqs : %d\n", nirq);
	return;
}
