/*
rtnet/rtnet_syms.c - export kernel symbols
Copyright (C) 1999      Lineo, Inc
              1999,2002 David A. Schleef <ds@schleef.org>

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

#include <rtnet.h>


EXPORT_SYMBOL(rt_socket_callback);

EXPORT_SYMBOL(rt_accept);
EXPORT_SYMBOL(rt_bind);
EXPORT_SYMBOL(rt_close);
EXPORT_SYMBOL(rt_connect);
EXPORT_SYMBOL(rt_listen);
EXPORT_SYMBOL(rt_recv);
EXPORT_SYMBOL(rt_recvfrom);
EXPORT_SYMBOL(rt_recvmsg);
//EXPORT_SYMBOL(rt_send);
EXPORT_SYMBOL(rt_sendmsg);
EXPORT_SYMBOL(rt_sendto);
EXPORT_SYMBOL(rt_socket);

EXPORT_SYMBOL(rtnetif_rx);
EXPORT_SYMBOL(rtnetif_wake_queue);
EXPORT_SYMBOL(rt_dev_alloc);
EXPORT_SYMBOL(rt_eth_header);
EXPORT_SYMBOL(alloc_rtskb);
EXPORT_SYMBOL(kfree_rtskb);

EXPORT_SYMBOL(rtnet_request_irq);
EXPORT_SYMBOL(rtnet_free_irq);

