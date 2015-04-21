/*
COPYRIGHT (C) 2000  POSEIDON CONTROLS INC (pcloutier@poseidoncontrols.com)

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

$Id: tid.h,v 1.1.1.1 2004/06/06 14:02:35 rpm Exp $ 
*/

#ifndef _TID_H_
#define _TID_H_

#define ARCNVCF	 0x8000  // ArcNet
#define ETH0VCF  0x10000
#define ETH1VCF  0x20000

void *assign_alias_name(pid_t pid, const char *name);
void *assign_host(pid_t pid, const char *host);
void pid2nam(pid_t pid, char *name);
void vc2nam(pid_t pid, char *name);
pid_t alias2pid( const char *name);

#endif // _TID_H_

