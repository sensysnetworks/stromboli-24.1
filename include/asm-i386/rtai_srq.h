/*
COPYRIGHT (C) 2000  Paolo Mantegazza (mantegazza@aero.polimi.it)

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


#ifndef _RTAI_SRQ_H_
#define _RTAI_SRQ_H_

#include <asm/rtai_vectors.h>

#ifndef __KERNEL__
static inline long long rtai_srq(int srq, unsigned int whatever)
{
	long long retval;
	RTAI_DO_TRAP(RTAI_SYS_VECTOR,retval,srq,whatever);
	return retval;
}

static inline int rtai_open_srq(unsigned int label)
{
	return (int)rtai_srq(0, label);
}
#endif

#endif
