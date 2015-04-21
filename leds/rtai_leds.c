/*
rtai_leds.c - mini-driver for generic control of digital signals
Copyright (C) 2000  Pierre Cloutier <pcloutier@PoseidonControls.com>
              2001  David A. Schleef <ds@schleef.org>

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


#include <linux/kernel.h>
#include <linux/module.h>

#include <rtai.h>
#include <rtai_leds.h>

MODULE_LICENSE("GPL");

static unsigned int leds;
static unsigned long port;

static void (*leds_func)(unsigned long port, unsigned int leds);

void rt_leds_set_mask(unsigned int mask, unsigned int value)
{
	leds &= ~mask;
	leds |= (mask & value);
	leds_func(port, leds);
}

void rt_toggle_leds(unsigned int l)
{
	leds ^= l;
	leds_func(port, leds);
}

void rt_reset_leds(unsigned int l)
{
	leds &= ~l;
	leds_func(port, leds);
}

void rt_set_leds(unsigned int l)
{
	leds |= l;
	leds_func(port, leds);
}

void rt_clear_leds(void)
{
	leds = 0;
	leds_func(port, leds);
}

unsigned int rt_get_leds(void)
{
	return leds;
}

void rt_set_leds_port(int p)
{
	port = p;
}

#ifdef CONFIG_X86
#include <asm/io.h>
static void parport_func(unsigned long port, unsigned int leds)
{
	outb(~leds, port);
}

#define DEFAULT_FUNC parport_func
#define DEFAULT_PORT 0x378
#endif

#if 0
#ifdef CONFIG_PPC
#include <linux/console.h>
#include <linux/tty.h>
#include <linux/console_struct.h>
static void fbdev_func(unsigned long port, unsigned int leds)
{
	int i,j;

	j = 48;
	for(i=0;i<32;i++){
		conswitchp->con_putc(vc_cons[fg_console].d,
			((vc_cons[fg_console].d->vc_attr)<<8)|
			(((leds<<i)&0x80000000)?'1':'0'),
			j,i);
	}
}

#define DEFAULT_FUNC fbdev_func
#define DEFAULT_PORT 0
#endif

#ifdef CONFIG_8260
/* You are responsible for configuring the pins yourself. */
#include <asm/cpm_8260.h>

static void cpm8260_func(unsigned long port, unsigned int leds)
{
	unsigned int bits;
	volatile iop8260_t *io = &immr->im_ioport;

	bits = io->iop_pdata;
	bits = (bits & ~port) | (leds & port);
	io->iop_pdata = bits;
}
#endif
#endif

#ifndef DEFAULT_FUNC
static void null_func(unsigned long port, unsigned int leds)
{
}
#define DEFAULT_FUNC null_func
#define DEFAULT_PORT 0
#endif

void rt_config_leds(unsigned int type,
	void (*func)(unsigned long p, unsigned int l),
	unsigned long p)
{

	switch(type){
	case 0:
		leds_func = func;
		port = p;
		break;
	case 1:
		leds_func = DEFAULT_FUNC;
		port = DEFAULT_PORT;
		break;
	}
}

int init_module(void)
{
	rt_config_leds(1,NULL,0);

	return(0);
}

void cleanup_module(void)
{
}

EXPORT_SYMBOL(rt_leds_set_mask);
EXPORT_SYMBOL(rt_toggle_leds);
EXPORT_SYMBOL(rt_reset_leds);
EXPORT_SYMBOL(rt_set_leds);
EXPORT_SYMBOL(rt_clear_leds);
EXPORT_SYMBOL(rt_get_leds);
EXPORT_SYMBOL(rt_set_leds_port);
EXPORT_SYMBOL(rt_config_leds);

