/*	$NetBSD: fplib_glue.S,v 1.1 1997/10/12 21:18:08 mark Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Neil A. Carson and Mark Brinicombe
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __netbsd__
#include <machine/asm.h>
#endif

#ifdef __ELF__
#define PLTJMP(_x)		_x##(PLT)
#define C_SYMBOL_NAME(_x)	_x
#else
#define PLTJMP(_x)		_x
#define C_SYMBOL_NAME(_x)	_##_x
#endif

#ifdef __linux__
#define ENTRY(_x)		.globl C_SYMBOL_NAME(_x); \
				.type C_SYMBOL_NAME(_x), %function; \
				.align 0; \
				C_SYMBOL_NAME(_x):
#define EXIT(_x)		.size C_SYMBOL_NAME(_x), . - C_SYMBOL_NAME(_x)
#endif

#ifdef __APCS_32__
#define HAT
#else
#define HAT ^
#endif

#define swap64(a,b)		\
	mov	ip, r##a; 	\
	mov	r##a, r##b;	\
	mov	r##b, ip

#define SIGN_BIT_TWIDDLE	0x80000000

/********************************* COMPARISONS ********************************/

/*
 * 'Equal' wrapper. This returns 0 if the numbers are equal, or (1 | -1)
 * otherwise. So we need to invert the output.
 */
ENTRY(__eqsf2)
	stmfd		sp!, {lr}
	bl		PLTJMP(C_SYMBOL_NAME(___float32_eq))
	eor		r0, r0, #1
	ldmfd		sp!, {pc} HAT
EXIT(__eqsf2)

ENTRY(__eqdf2)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_eq))
	eor		r0, r0, #1
	ldmfd		sp!, {pc} HAT
EXIT(__eqdf2)

/*
 * 'Not Equal' wrapper. This returns -1 or 1 (say, 1!) if the numbers are
 * not equal, 0 otherwise. However no not equal call is provided, so we have
 * to use an 'equal' call and invert the result. The result is already
 * inverted though! Confusing?!
 */
ENTRY(__nesf2)
	stmfd		sp!, {lr}
	bl		PLTJMP(C_SYMBOL_NAME(___float32_eq))
	eor		r0, r0, #1
	ldmfd		sp!, {pc} HAT
EXIT(__nesf2)

ENTRY(__nedf2)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_eq))
	eor		r0, r0, #1
	ldmfd		sp!, {pc} HAT
EXIT(__nesf2)

/*
 * 'Greater Than' wrapper. This returns 1 if the number is greater, 0
 * or -1 otherwise. Unfortunately, no such function exists. We have to
 * instead compare the numbers using the 'less than' calls in order to
 * make up our mind. This means that we can call 'less than or equal' and
 * invert the result.
 */
ENTRY(__gtsf2)
	stmfd		sp!, {lr}
	bl		PLTJMP(C_SYMBOL_NAME(___float32_le))
	eor		r0, r0, #1
	ldmfd		sp!, {pc} HAT
EXIT(__gtsf2)

ENTRY(__gtdf2)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_le))
	eor		r0, r0, #1
	ldmfd		sp!, {pc} HAT
EXIT(__gtdf2)

/*
 * 'Greater Than or Equal' wrapper. We emulate this by inverting the result
 * of a 'less than' call.
 */
ENTRY(__gesf2)
	stmfd		sp!, {lr}
	bl		PLTJMP(C_SYMBOL_NAME(___float32_lt))
	eor		r0, r0, #1
	cmp		r0, #0
	mvneq		r0, #0
	ldmfd		sp!, {pc} HAT
EXIT(__gesf2)

ENTRY(__gedf2)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_lt))
	eor		r0, r0, #1
	cmp		r0, #0
	mvneq		r0, #0
	ldmfd		sp!, {pc} HAT
EXIT(__gedf2)

/*
 * 'Less Than' wrapper. A 1 from the ARM code needs to be turned into -1.
 */
ENTRY(__ltsf2)
	stmfd		sp!, {lr}
	bl		PLTJMP(C_SYMBOL_NAME(___float32_lt))
	cmp		r0, #1
	mvneq		r0, #0
	ldmfd		sp!, {pc} HAT
EXIT(__ltsf2)

ENTRY(__ltdf2)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_lt))
	cmp		r0, #1
	mvneq		r0, #0
	ldmfd		sp!, {pc} HAT
EXIT(__ltdf2)

/*
 * 'Less Than or Equal' wrapper. A 0 must turn into a 1, and a 1 into a 0.
 */
ENTRY(__lesf2)
	stmfd		sp!, {lr}
	bl		PLTJMP(C_SYMBOL_NAME(___float32_le))
	eor		r0, r0, #1
	ldmfd		sp!, {pc} HAT
EXIT(__lesf2)

ENTRY(__ledf2)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_le))
	eor		r0, r0, #1
	ldmfd		sp!, {pc} HAT
EXIT(__ledf2)

/*
 * Float negate... This isn't provided by the library, but it's hardly the
 * hardest function in the world to write... :) In fact, because of the
 * position in the registers of arguments, the double precision version can
 * go here too ;-)
 */
ENTRY(__negsf2)
	eor		r0, r0, #SIGN_BIT_TWIDDLE
#ifdef __APCS_32__
	mov		pc, lr
#else
	movs		pc, lr
#endif
EXIT(__negsf2)

ENTRY(__negdf2)
	eor		r0, r0, #SIGN_BIT_TWIDDLE
#ifdef __APCS_32__
	mov		pc, lr
#else
	movs		pc, lr
#endif
EXIT(__negdf2)

/*
 * Now the 64-bit operations.  The ARM uses a bizarre mixed-endian format for
 * doubles which means we must swap the two 32-bit words on the way into
 * and out of the emulation code.  One day we should fix this properly in
 * softfloat itself.
 */

ENTRY(__adddf3)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_add))
	swap64(0,1)
	ldmfd		sp!, {pc} HAT
EXIT(__adddf3)

ENTRY(__subdf3)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_sub))
	swap64(0,1)
	ldmfd		sp!, {pc} HAT
EXIT(__subdf3)

ENTRY(__muldf3)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_mul))
	swap64(0,1)
	ldmfd		sp!, {pc} HAT
EXIT(__muldf3)

ENTRY(__divdf3)
	stmfd		sp!, {lr}
	swap64(0,1)
	swap64(2,3)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_div))
	swap64(0,1)
	ldmfd		sp!, {pc} HAT
EXIT(__divdf3)

ENTRY(__floatsidf)
	stmfd		sp!, {lr}
	bl		PLTJMP(C_SYMBOL_NAME(___int32_to_float64))
	swap64(0,1)
	ldmfd		sp!, {pc} HAT
EXIT(__floatsidf)

ENTRY(__truncdfsf2)
	stmfd		sp!, {lr}
	swap64(0,1)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_to_float32))
	ldmfd		sp!, {pc} HAT
EXIT(__truncdfsf2)

ENTRY(__extendsfdf2)
	stmfd		sp!, {lr}
	bl		PLTJMP(C_SYMBOL_NAME(___float32_to_float64))
	swap64(0,1)
	ldmfd		sp!, {pc} HAT
EXIT(__extendsfdf2)

ENTRY(__fixdfsi)
	stmfd		sp!, {lr}
	swap64(0,1)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_to_int32_round_to_zero))
	ldmfd		sp!, {pc} HAT
EXIT(__fixdfsi)

ENTRY(__fixunsdfsi)
	stmfd		sp!, {lr}
	swap64(0,1)
	bl		PLTJMP(C_SYMBOL_NAME(___float64_to_uint32_round_to_zero))
	ldmfd		sp!, {pc} HAT
EXIT(__fixunsdfsi)
