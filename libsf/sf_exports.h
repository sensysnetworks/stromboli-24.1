/*
rtai/libm/softfloat/sf_exports.h - exports for soft-float support
RTAI - Real-Time Application Interface
Copyright (c) 2003, Thomas Gleixner, <tglx@linutronix.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
*	int helper functions from gcc hack
*
*	Please follow the give scheme to avoid broken exports
*	for other architectures
*	Thanks, Thomas	
*/
#include "arch.h"

#ifdef L_divdi3
extern void L_divdi3 (void);
EXPORT_SYMBOL(L_divdi3);
#endif

#ifdef L_fixunssfsi
extern void L_fixunssfsi (void);
EXPORT_SYMBOL(L_fixunssfsi);
#endif

/*
*	float helper functions from libfloat hack
*	this list should be complete
*/
extern void __adddf3(void);
extern void __addsf3(void);
extern void __divdf3(void);
extern void __divsf3(void);
extern void __eqdf2(void);
extern void __eqsf2(void);
extern void __extendsfdf2(void);
extern void __fixdfsi(void);
extern void __fixsfsi(void);
extern void __fixunsdfsi(void);
extern void __floatsidf(void);
extern void __floatsisf(void);
extern void __gedf2(void);
extern void __gesf2(void);
extern void __gtdf2(void);
extern void __gtsf2(void);
extern void __ledf2(void);
extern void __lesf2(void);
extern void __ltdf2(void);
extern void __ltsf2(void);
extern void __muldf3(void);
extern void __mulsf3(void);
extern void __nedf2(void);
extern void __negdf2(void);
extern void __negsf2(void);
extern void __nesf2(void);
extern void __subdf3(void);
extern void __subsf3(void);
extern void __truncdfsf2(void);

EXPORT_SYMBOL(__adddf3);
EXPORT_SYMBOL(__addsf3);
EXPORT_SYMBOL(__divdf3);
EXPORT_SYMBOL(__divsf3);
EXPORT_SYMBOL(__eqdf2);
EXPORT_SYMBOL(__eqsf2);
EXPORT_SYMBOL(__extendsfdf2);
EXPORT_SYMBOL(__fixdfsi);
EXPORT_SYMBOL(__fixsfsi);
EXPORT_SYMBOL(__fixunsdfsi);
EXPORT_SYMBOL(__floatsidf);
EXPORT_SYMBOL(__floatsisf);
EXPORT_SYMBOL(__gedf2);
EXPORT_SYMBOL(__gesf2);
EXPORT_SYMBOL(__gtdf2);
EXPORT_SYMBOL(__gtsf2);
EXPORT_SYMBOL(__ledf2);
EXPORT_SYMBOL(__lesf2);
EXPORT_SYMBOL(__ltdf2);
EXPORT_SYMBOL(__ltsf2);
EXPORT_SYMBOL(__muldf3);
EXPORT_SYMBOL(__mulsf3);
EXPORT_SYMBOL(__nedf2);
EXPORT_SYMBOL(__negdf2);
EXPORT_SYMBOL(__negsf2);
EXPORT_SYMBOL(__nesf2);
EXPORT_SYMBOL(__subdf3);
EXPORT_SYMBOL(__subsf3);
EXPORT_SYMBOL(__truncdfsf2);
