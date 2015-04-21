/*
 * COPYRIGHT (C) 2001  Paolo Mantegazza <mantegazza@aero.polimi.it>
 *               2002  Robert Schwebel  <robert@schwebel.de>
 *               2002  Erwin Rol <erwin@muffin.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 * 
 * ACKNOWLEDGMENT: 
 * 
 * Part of this code is derived from that of the latency calibration example,
 * which in turn was copied from a similar program distributed with NMT-RTL.
 */

#ifndef __MODINFO_H__
#define __MODINFO_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int overall;
extern int period;
extern int avrgtime;
extern int use_fpu;
extern int start_timer;

#ifdef __cplusplus
}
#endif

#endif /* !__MODINFO_H__ */
