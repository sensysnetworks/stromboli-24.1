/*
 * Project: rtai_cpp - RTAI C++ Framework
 *
 * File: $Id: modinfo.c,v 1.1.1.1 2004/06/06 14:03:07 rpm Exp $
 *
 * COPYRIGHT: (C) 2001  Paolo Mantegazza <mantegazza@aero.polimi.it>
 *                2002  Robert Schwebel  <robert@schwebel.de>
 *                2002  Erwin Rol <erwin@muffin.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include "modinfo.h"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Latency measurement tool for RTAI in C++");
MODULE_AUTHOR("Paolo Mantegazza, Robert Schwebel, Erwin Rol");

int overall = 1;
MODULE_PARM(overall, "i");
MODULE_PARM_DESC(overall,
		 "Calculate overall (1) or per-loop (0) statistics (default: 1)");

int period = 100000;
MODULE_PARM(period, "i");
MODULE_PARM_DESC(period, "period in ns (default: 100000)");

int avrgtime = 1;
MODULE_PARM(avrgtime, "i");
MODULE_PARM_DESC(avrgtime, "Averages are calculated for <avrgtime (s)> runs (default: 1)");

int use_fpu = 0;
MODULE_PARM(use_fpu, "i");
MODULE_PARM_DESC(use_fpu, "do we want to use the FPU? (default: 0)");

int start_timer = 1;
MODULE_PARM(start_timer, "i");
MODULE_PARM_DESC(start_timer,
		 "declares if the timer should be started or not (default: 1)");
