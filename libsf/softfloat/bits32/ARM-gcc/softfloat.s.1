rfp	.req	r9
sl	.req	r10
fp	.req	r11
ip	.req	r12
sp	.req	r13
lr	.req	r14
pc	.req	r15
gcc2_compiled.:
___gnu_compiled_c:
	.global	_float_rounding_mode
.data
_float_rounding_mode:
	.byte	0
	.global	_float_exception_flags
_float_exception_flags:
	.byte	0
.text
	.align	0
_estimateDiv64To32:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #8
	mov	r7, r0
	mov	r9, r1
	mov	r5, r2
	cmp	r5, r7
	mvnls	r0, #0
	bls	L85
L71:
	mov	r6, r5, lsr #16
	cmp	r7, r6, asl #16
	bcs	L72
	mov	r0, r7
	mov	r1, r6
	bl	___udivsi3
	mov	r4, r0, asl #16
	b	L73
L72:
	mov	r4, #-2147483648
	mov	r4, r4, asr #15
L73:
	mov	r1, r5, asl #16
	mov	r1, r1, lsr #16
	mov	r3, r4, asl #16
	mov	r3, r3, lsr #16
	mul	lr, r3, r1
	mov	r0, r4, lsr #16
	mov	r2, r5, lsr #16
	mul	ip, r0, r2
	mul	r3, r2, r3
	mla	r2, r0, r1, r3
	add	r8, sp, #4
	cmp	r2, r3
	mov	r0, sp
	mov	r1, r2, lsr #16
	addcc	r3, ip, #65536
	addcc	ip, r3, r1
L74:
	addcs	ip, ip, r1
L75:
	mov	r2, r2, asl #16
	adds	lr, lr, r2
	str	lr, [r0, #0]
	addcs	ip, ip, #1
	str	ip, [r8, #0]
	ldmia	sp, {r2, r3}	@ phole ldm
	rsb	r3, r3, r7
	cmp	r9, r2
	movcs	r0, r3
	subcc	r0, r3, #1
	cmp	r0, #0
	rsb	ip, r2, r9
	bge	L79
	mov	r1, r5, asl #16
L80:
	sub	r4, r4, #65536
	mov	r2, ip
	add	ip, ip, r1
	add	r3, r0, r6
	cmp	ip, r2
	movcs	r0, r3
	addcc	r0, r3, #1
	cmp	r0, #0
	blt	L80
L79:
	mov	r3, ip, lsr #16
	orr	r0, r3, r0, asl #16
	cmp	r0, r6, asl #16
	movcc	r1, r6
	blcc	___udivsi3
	orrcc	r4, r4, r0
L83:
	mvncs	r4, r4, lsr #16
	mvncs	r4, r4, asl #16
L84:
	mov	r0, r4
L85:
	add	sp, sp, #8
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, pc}^
	.even
_sqrtOddAdjustments.30:
	.short	4
	.short	34
	.short	93
	.short	177
	.short	285
	.short	415
	.short	566
	.short	736
	.short	924
	.short	1128
	.short	1349
	.short	1585
	.short	1835
	.short	2098
	.short	2374
	.short	2663
	.even
_sqrtEvenAdjustments.31:
	.short	2605
	.short	2223
	.short	1882
	.short	1577
	.short	1306
	.short	1065
	.short	854
	.short	670
	.short	512
	.short	377
	.short	265
	.short	175
	.short	104
	.short	52
	.short	18
	.short	2
	.align	0
_estimateSqrt32:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	tst	r0, #1
	mov	r5, r1
	mov	r3, r5, lsr #27
	and	r2, r3, #15
	beq	L87
	ldr	r3, L93
	ldr	r3, [r3, r2, asl #1]	@ movhi
	mov	r0, r5
	mov	r3, r3, asl #16
	mov	r3, r3, lsr #16
	sub	r3, r3, #16384
	rsb	r4, r3, r5, lsr #17
	mov	r1, r4
	bl	___udivsi3
	mov	r5, r5, lsr #1
	mov	r3, r4, asl #15
	add	r4, r3, r0, asl #14
	b	L88
L87:
	ldr	r3, L93+4
	ldr	r3, [r3, r2, asl #1]	@ movhi
	mov	r0, r5
	mov	r3, r3, asl #16
	mov	r3, r3, lsr #16
	sub	r3, r3, #32768
	rsb	r4, r3, r5, lsr #17
	mov	r1, r4
	bl	___udivsi3
	add	r4, r4, r0
	mvn	r3, #-16777216
	sub	r3, r3, #16646144
	cmp	r4, r3
	movls	r1, r4, asl #15
L89:
	movhi	r1, #-2147483648
	movhi	r1, r1, asr #16
L90:
	mov	r4, r1
	cmp	r4, r5
	movls	r0, r5, asr #1
	ldmlsfd	sp!, {r4, r5, pc}^
L88:
	mov	r0, r5
	mov	r1, #0
	mov	r2, r4
	bl	_estimateDiv64To32
	mov	r3, r4, lsr #1
	add	r0, r3, r0, lsr #1
	ldmfd	sp!, {r4, r5, pc}^
L94:
	.align	0
L93:
	.word	_sqrtOddAdjustments.30
	.word	_sqrtEvenAdjustments.31
_countLeadingZerosHigh.34:
	.byte	8
	.byte	7
	.byte	6
	.byte	6
	.byte	5
	.byte	5
	.byte	5
	.byte	5
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	4
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	3
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	2
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.align	0
_countLeadingZeros32:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	mov	r2, #0
	mov	r3, #65280
	add	r3, r3, #255
	cmp	r0, r3
	addls	r2, r2, #16
	movls	r0, r0, asl #16
L96:
	mvn	r3, #-16777216
	cmp	r0, r3
	addls	r3, r2, #8
	andls	r2, r3, #255
	movls	r0, r0, asl #8
L97:
	ldr	r3, L98
	ldrb	r0, [r3, r0, lsr #24]
	mov	r3, r2, asl #24
	add	r0, r0, r3, asr #24
	mov	r0, r0, asl #24
	mov	r0, r0, asr #24
	movs	pc, lr
L99:
	.align	0
L98:
	.word	_countLeadingZerosHigh.34
	.global	_float_detect_tininess
.data
_float_detect_tininess:
	.byte	0
.text
	.align	0
	.global	_float_raise
_float_raise:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	ldr	r2, L109
	ldrb	r3, [r2, #0]
	orr	r3, r3, r0
	strb	r3, [r2, #0]
	movs	pc, lr
L110:
	.align	0
L109:
	.word	_float_exception_flags
	.align	0
	.global	_float32_is_nan
_float32_is_nan:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	mov	r0, r0, asl #1
	cmp	r0, #-16777216
	movls	r0, #0
	movhi	r0, #1
	movs	pc, lr
	.align	0
	.global	_float32_is_signaling_nan
_float32_is_signaling_nan:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	mov	r1, #0
	mov	r2, #508
	add	r3, r2, #3
	and	r3, r3, r0, lsr #22
	add	r2, r2, #2
	cmp	r3, r2
	bne	L113
	bic	r3, r0, #-16777216
	bic	r3, r3, #12582912
	subs	r1, r3, r1
	movne	r1, #1
L113:
	mov	r0, r1
	movs	pc, lr
	.align	0
_float32ToCommonNaN:
	@ args = 0, pretend = 0, frame = 12
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	sub	sp, sp, #12
	mov	r5, r0
	mov	r4, r1
	mov	r0, r4
	bl	_float32_is_signaling_nan
	tst	r0, #255
	movne	r0, #1
	blne	_float_raise
L115:
	mov	r3, r4, lsr #31
	strb	r3, [sp, #0]
	mov	r3, #0
	str	r3, [sp, #8]
	mov	r3, r4, asl #9
	str	r3, [sp, #4]
	mov	r3, sp
	ldmia	r3, {r0-r2}	@ load multiple
	stmia	r5, {r0-r2}	@ str multiple
	mov	r0, r5
	add	sp, sp, #12
	ldmfd	sp!, {r4, r5, pc}^
	.align	0
_commonNaNToFloat32:
	@ args = 0, pretend = 0, frame = 12
	@ frame_needed = 0, current_function_anonymous_args = 0
	sub	sp, sp, #12
	mov	r3, sp
	stmia	r3, {r0-r2}	@ str multiple
	ldrb	r2, [sp, #0]	@ zero_extendqisi2
	ldr	r3, [sp, #4]
	mov	r0, #2130706432
	add	r0, r0, #12582912
	orr	r0, r0, r3, lsr #9
	orr	r0, r0, r2, asl #31
	add	sp, sp, #12
	movs	pc, lr
	.align	0
_propagateFloat32NaN:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #8
	mov	r7, r0
	mov	r6, r1
	bl	_float32_is_nan
	mov	r3, r0
	and	r3, r3, #255
	str	r3, [sp, #4]
	mov	r0, r7
	bl	_float32_is_signaling_nan
	mov	r4, r0
	mov	r0, r6
	and	r4, r4, #255
	bl	_float32_is_nan
	mov	r3, r0
	and	r3, r3, #255
	str	r3, [sp, #0]
	mov	r0, r6
	bl	_float32_is_signaling_nan
	ldr	r8, [sp, #0]
	orr	r7, r7, #4194304
	orr	r6, r6, #4194304
	and	r5, r0, #255
	orrs	ip, r4, r5
	mov	r9, r5
	movne	r0, #1
	blne	_float_raise
L118:
	cmp	r4, #0
	beq	L119
	cmp	r5, #0
	bne	L121
	ldr	ip, [sp, #0]
	cmp	ip, #0
	moveq	r0, r7
	movne	r0, r6
	b	L132
L119:
	ldr	ip, [sp, #4]
	cmp	ip, #0
	beq	L125
	cmp	r8, #0
	beq	L133
	cmp	r9, #0
	bne	L133
L121:
	mov	r2, r7, asl #1
	mov	r3, r6, asl #1
	cmp	r2, r3
	bcc	L125
	cmp	r3, r2
	bcs	L130
L133:
	mov	r0, r7
	b	L132
L130:
	cmp	r6, r7
	movcc	r0, r6
	movcs	r0, r7
	b	L132
L125:
	mov	r0, r6
L132:
	add	sp, sp, #8
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, pc}^
	.align	0
	.global	_float64_is_nan
_float64_is_nan:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	mov	r2, #0
	mvn	r3, #2097152
	cmp	r3, r0, asl #1
	bcs	L135
	cmp	r1, r2
	bne	L136
	bic	r3, r0, #-16777216
	bic	r3, r3, #15728640
	cmp	r3, r2
	beq	L135
L136:
	mov	r2, #1
L135:
	mov	r0, r2
	movs	pc, lr
	.align	0
	.global	_float64_is_signaling_nan
_float64_is_signaling_nan:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	mov	ip, #0
	mov	r2, #4080
	add	r3, r2, #15
	and	r3, r3, r0, lsr #19
	add	r2, r2, #14
	cmp	r3, r2
	bne	L138
	cmp	r1, ip
	bne	L139
	bic	r3, r0, #-16777216
	bic	r3, r3, #16252928
	cmp	r3, ip
	beq	L138
L139:
	mov	ip, #1
L138:
	mov	r0, ip
	movs	pc, lr
	.align	0
_float64ToCommonNaN:
	@ args = 0, pretend = 0, frame = 12
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	sub	sp, sp, #12
	mov	r6, r0
	mov	r5, r2
	mov	r4, r1
	mov	r1, r5
	mov	r0, r4
	bl	_float64_is_signaling_nan
	tst	r0, #255
	movne	r0, #1
	blne	_float_raise
L141:
	mov	r3, r4, lsr #31
	strb	r3, [sp, #0]
	mov	r3, sp
	ldmia	r3, {r0-r2}	@ load multiple
	stmia	r6, {r0-r2}	@ str multiple
	mov	r3, r5
	mov	r2, r3, asl #12
	str	r2, [sp, #8]
	mov	r3, r3, lsr #20
	orr	r3, r3, r4, asl #12
	str	r3, [sp, #4]
	mov	r0, r6
	add	sp, sp, #12
	ldmfd	sp!, {r4, r5, r6, pc}^
	.align	0
_commonNaNToFloat64:
	@ args = 0, pretend = 0, frame = 20
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	sub	sp, sp, #20
	add	ip, sp, #8
	stmia	ip, {r1-r3}	@ str multiple
	ldr	r1, [sp, #12]
	mov	r3, r1, lsr #12
	str	r3, [sp, #0]
	ldrb	r2, [sp, #8]	@ zero_extendqisi2
	orr	r3, r3, #2130706432
	orr	r3, r3, #16252928
	orr	r3, r3, r2, asl #31
	str	r3, [sp, #0]
	ldr	r3, [sp, #16]
	mov	r3, r3, lsr #12
	orr	r3, r3, r1, asl #20
	str	r3, [sp, #4]
	mov	r3, sp
	ldmia	r3, {r4-r5}
	stmia	r0, {r4-r5}
	add	sp, sp, #20
	ldmfd	sp!, {r4, r5, lr}
	movs	pc, lr
	.align	0
_propagateFloat64NaN:
	@ args = 8, pretend = 4, frame = 20
	@ frame_needed = 0, current_function_anonymous_args = 0
	sub	sp, sp, #4
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #20
	str	r0, [sp, #16]
	str	r3, [sp, #48]
	add	ip, sp, #48
	ldmia	ip, {r5-r6}
	mov	r8, r2
	mov	r7, r1
	mov	r1, r8
	mov	r0, r7
	bl	_float64_is_nan
	mov	r3, r0
	and	r3, r3, #255
	str	r3, [sp, #12]
	mov	r1, r8
	mov	r0, r7
	bl	_float64_is_signaling_nan
	mov	r4, r0
	mov	r1, r6
	mov	r0, r5
	and	r4, r4, #255
	bl	_float64_is_nan
	mov	r3, r0
	and	r3, r3, #255
	str	r3, [sp, #0]
	mov	r1, r6
	mov	r0, r5
	bl	_float64_is_signaling_nan
	and	r9, r0, #255
	str	r9, [sp, #4]
	orrs	ip, r4, r9
	ldr	ip, [sp, #0]
	str	ip, [sp, #8]
	orr	r7, r7, #524288
	orr	r5, r5, #524288
	movne	r0, #1
	blne	_float_raise
L154:
	cmp	r4, #0
	beq	L155
	cmp	r9, #0
	bne	L157
	ldr	ip, [sp, #0]
	cmp	ip, #0
	bne	L161
	b	L176
L155:
	ldr	ip, [sp, #12]
	cmp	ip, #0
	beq	L161
	ldr	ip, [sp, #8]
	cmp	ip, #0
	beq	L176
	ldr	ip, [sp, #4]
	cmp	ip, #0
	bne	L176
L157:
	mov	r0, #0
	mov	r3, r7, asl #1
	cmp	r3, r5, asl #1
	mov	r1, r8
	mov	r2, r6
	bcc	L166
	movne	r3, #0
	moveq	r3, #1
	cmp	r3, r0
	cmpne	r1, r2
	bcs	L167
L166:
	mov	r0, #1
L167:
	and	r3, r0, #255
	cmp	r3, #0
	bne	L161
	mov	r0, r3
	mov	r3, r5, asl #1
	cmp	r3, r7, asl #1
	mov	r1, r6
	mov	r2, r8
	bcc	L170
	movne	r3, #0
	moveq	r3, #1
	cmp	r3, #0
	cmpne	r1, r2
	bcs	L171
L170:
	mov	r0, #1
L171:
	cmp	r0, #0
	bne	L176
	cmp	r7, r5
	bcs	L173
L176:
	ldr	ip, [sp, #16]
	stmia	ip, {r7-r8}
	b	L153
L173:
L161:
	ldr	ip, [sp, #16]
	stmia	ip, {r5-r6}
L153:
	ldr	r0, [sp, #16]
	add	sp, sp, #20
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	add	sp, sp, #4
	movs	pc, lr
	.align	0
_normalizeFloat32Subnormal:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	mov	r4, r0
	mov	r6, r1
	mov	r5, r2
	bl	_countLeadingZeros32
	sub	r0, r0, #8
	mov	r0, r0, asl #24
	mov	r0, r0, asr #24
	mov	r4, r4, asl r0
	str	r4, [r5, #0]
	rsb	r0, r0, #1
	str	r0, [r6, #0]
	ldmfd	sp!, {r4, r5, r6, pc}^
	.align	0
_roundAndPackFloat32:
	@ args = 0, pretend = 0, frame = 4
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	sub	sp, sp, #4
	str	r2, [sp, #0]
	ldr	r3, L208
	ldrb	r3, [r3, #0]	@ zero_extendqisi2
	mov	r5, r1
	mov	r6, #64
	and	r8, r0, #255
	mov	r3, r3, asl #24
	cmp	r3, #0
	movne	r7, #0
	moveq	r7, #1
	cmp	r7, #0
	mov	r3, r3, asr #24
	bne	L183
	cmp	r3, #3
	mov	r6, r7
	beq	L183
	mov	r6, #127
	cmp	r8, #0
	beq	L186
	cmp	r3, #2
	movne	r6, #127
	moveq	r6, #0
	b	L183
L186:
	cmp	r3, #1
	moveq	r6, #0
L183:
	ldrb	r2, [sp, #0]
	mov	r3, r5, asl #16
	cmp	r3, #16515072
	and	r4, r2, #127
	bls	L190
	cmp	r5, #253
	bgt	L192
	bne	L191
	ldr	r3, [sp, #0]
	mov	r2, r6, asl #24
	adds	r2, r3, r2, asr #24
	bpl	L191
L192:
	mov	r0, #40
	bl	_float_raise
	mov	r0, r8, asl #31
	add	r0, r0, #2130706432
	add	r0, r0, #8388608
	movs	r6, r6, asl #24
	subeq	r0, r0, #1
	b	L207
L191:
	cmp	r5, #0
	bge	L190
	ldr	r3, L208+4
	ldrb	r2, [r3, #0]
	mov	r0, #0
	cmn	r5, #1
	movge	r3, #0
	movlt	r3, #1
	cmp	r2, #1
	moveq	r3, #1
	cmp	r3, r0
	bne	L196
	ldr	r3, [sp, #0]
	mov	r2, r6, asl #24
	adds	r2, r3, r2, asr #24
	bmi	L195
L196:
	mov	r0, #1
L195:
	ldr	r1, [sp, #0]
	rsb	r2, r5, #0
	cmp	r2, #0
	mov	ip, r0
	mov	r0, sp
	moveq	r3, r1
	beq	L198
L197:
	cmp	r2, #31
	bgt	L199
	and	r3, r5, #31
	movs	r3, r1, asl r3
	moveq	r3, #0
	movne	r3, #1
	orr	r3, r3, r1, lsr r2
	b	L198
L209:
	.align	0
L208:
	.word	_float_rounding_mode
	.word	_float_detect_tininess
L199:
	subs	r3, r1, #0
	movne	r3, #1
L198:
	str	r3, [r0, #0]
	ldrb	r3, [sp, #0]
	mov	r5, #0
	ands	r4, r3, #127
	moveq	r3, #0
	movne	r3, #1
	tst	ip, r3
	movne	r0, #16
	blne	_float_raise
L190:
	subs	r1, r4, #0
	ldrne	r2, L211
	ldrneb	r3, [r2, #0]
	orrne	r3, r3, #32
	strneb	r3, [r2, #0]
L203:
	ldr	r3, [sp, #0]
	mov	r2, r6, asl #24
	add	r3, r3, r2, asr #24
	mov	r2, r3, lsr #7
	str	r2, [sp, #0]
	eor	r3, r1, #64
	movs	r3, r3, asl #24
	mvneq	r3, r7
	andeq	r3, r2, r3
	streq	r3, [sp, #0]
L204:
	ldr	r3, [sp, #0]
	cmp	r3, #0
	moveq	r0, #0
	movne	r0, r5, asl #23
	add	r0, r0, r8, asl #31
	add	r0, r0, r3
L207:
	b	L210
L212:
	.align	0
L211:
	.word	_float_exception_flags
L210:
	add	sp, sp, #4
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}^
	.align	0
_normalizeRoundAndPackFloat32:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	mov	r4, r0
	mov	r6, r1
	mov	r5, r2
	mov	r0, r5
	and	r4, r4, #255
	bl	_countLeadingZeros32
	sub	r2, r0, #1
	mov	r0, r4
	mov	r2, r2, asl #24
	mov	r2, r2, asr #24
	rsb	r1, r2, r6
	mov	r2, r5, asl r2
	ldmfd	sp!, {r4, r5, r6, lr}
	b	_roundAndPackFloat32
	.align	0
_normalizeFloat64Subnormal:
	@ args = 4, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	ldr	r7, [sp, #24]
	mov	r4, r1
	mov	r8, r2
	mov	r6, r3
	subs	r5, r0, #0
	bne	L219
	mov	r0, r4
	bl	_countLeadingZeros32
	sub	r0, r0, #11
	and	r0, r0, #255
	mov	r3, r0, asl #24
	movs	r2, r3, asr #24
	bpl	L220
	rsb	r3, r2, #0
	mov	r3, r4, lsr r3
	str	r3, [r6, #0]
	and	r3, r2, #31
	mov	r3, r4, asl r3
	str	r3, [r7, #0]
	b	L221
L220:
	mov	r3, r4, asl r2
	str	r3, [r6, #0]
	str	r5, [r7, #0]
L221:
	mov	r3, r0, asl #24
	mov	r3, r3, asr #24
	rsb	r3, r3, #-67108863
	add	r3, r3, #66846720
	add	r3, r3, #261120
	add	r3, r3, #992
	str	r3, [r8, #0]
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}^
L219:
	mov	r0, r5
	bl	_countLeadingZeros32
	sub	r0, r0, #11
	and	r0, r0, #255
	mov	r3, r0, asl #24
	mov	r2, r3, asr #24
	mov	r3, r4, asl r2
	str	r3, [r7, #0]
	cmp	r2, #0
	beq	L223
	rsb	r3, r2, #0
	and	r3, r3, #31
	mov	r3, r4, lsr r3
	orr	r3, r3, r5, asl r2
	str	r3, [r6, #0]
	b	L225
L223:
	str	r5, [r6, #0]
L225:
	mov	r3, r0, asl #24
	mov	r3, r3, asr #24
	rsb	r3, r3, #1
	str	r3, [r8, #0]
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}^
	.align	0
_roundAndPackFloat64:
	@ args = 8, pretend = 0, frame = 44
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #44
	str	r0, [sp, #36]
	str	r3, [sp, #40]
	ldr	r3, [sp, #72]
	ldr	r3, L275
	ldrb	r3, [r3, #0]	@ zero_extendqisi2
	str	r3, [sp, #32]
	mov	r3, r3, asl #24
	cmp	r3, #0
	movne	r5, #0
	moveq	r5, #1
	str	r5, [sp, #28]
	ldr	ip, [sp, #76]
	mov	r8, r2
	and	r9, r1, #255
	mov	r0, ip, lsr #31
	cmp	r5, #0
	mov	r2, r3, asr #24
	bne	L228
	cmp	r2, #3
	moveq	r0, r5
	beq	L228
L229:
	cmp	r9, #0
	beq	L231
	subs	r3, ip, #0
	movne	r3, #1
	cmp	r2, #1
	moveq	r0, r3
	movne	r0, #0
	b	L228
L231:
	subs	r3, ip, #0
	movne	r3, #1
	cmp	r2, #2
	moveq	r0, r3
	movne	r0, #0
L228:
	mov	r1, #2032
	add	r2, r1, #12
	mov	r3, r8, asl #16
	cmp	r2, r3, lsr #16
	bcs	L233
	add	r3, r1, #13
	cmp	r8, r3
	bgt	L235
	bne	L234
	ldr	r1, [sp, #40]
	ldr	r2, [sp, #72]
	mvn	r3, #-16777216
	sub	r3, r3, #14680064
	cmp	r3, r1
	cmneq	r2, #1
	bne	L234
	cmp	r0, #0
	beq	L234
L235:
	mov	r0, #40
	bl	_float_raise
	ldr	r6, [sp, #32]
	mov	r3, r6, asl #24
	mov	r2, r3, asr #24
	cmp	r2, #3
	beq	L238
	cmp	r2, #2
	movne	r3, #0
	moveq	r3, #1
	cmp	r9, #0
	moveq	r3, #0
	cmp	r3, #0
	bne	L238
	cmp	r2, #1
	movne	r3, #0
	moveq	r3, #1
	cmp	r9, #0
	movne	r3, #0
	cmp	r3, #0
	beq	L237
L238:
	mvn	r7, #0
	str	r7, [sp, #8]
	mov	r3, r9, asl #31
	orr	r3, r3, #2130706432
	str	r3, [sp, #4]
	mov	r5, r3
	orr	r5, r5, #15663104
	str	r5, [sp, #4]
	mov	r6, r5
	orr	r6, r6, #65280
	str	r6, [sp, #4]
	mov	r7, r6
	orr	r7, r7, #255
	str	r7, [sp, #4]
	ldmib	sp, {r5-r6}
	b	L272
L237:
	str	r3, [sp, #16]
	mov	r3, r9, asl #31
	orr	r3, r3, #2130706432
	str	r3, [sp, #12]
	mov	r5, r3
	orr	r5, r5, #15728640
	str	r5, [sp, #12]
	ldr	r7, [sp, #36]
	add	r5, sp, #12
	ldmia	r5, {r5-r6}
	b	L273
L234:
	cmp	r8, #0
	bge	L233
	ldr	r3, L275+4
	ldrb	r2, [r3, #0]
	mov	lr, #0
	cmn	r8, #1
	movge	r3, #0
	movlt	r3, #1
	cmp	r2, #1
	moveq	r3, #1
	cmp	r3, lr
	bne	L243
	cmp	r0, lr
	beq	L243
	ldr	r2, [sp, #40]
	mvn	r3, #-16777216
	sub	r3, r3, #14680064
	mov	r1, lr
	cmp	r2, r3
	bcc	L244
	ldr	r2, [sp, #72]
	movne	r3, #0
	moveq	r3, #1
	cmp	r3, lr
	cmnne	r2, #1
	bcs	L245
L244:
	mov	r1, #1
L245:
	cmp	r1, #0
	beq	L242
L243:
	mov	lr, #1
L242:
	str	lr, [sp, #0]
	add	r6, sp, #40
	str	r6, [sp, #4]
	add	r7, sp, #72
	str	r7, [sp, #12]
	ldr	r4, [sp, #76]
	ldr	ip, [sp, #40]
	ldr	r3, [sp, #72]
	rsb	r1, r8, #0
	cmp	r1, #0
	and	r2, r8, #31
	add	lr, sp, #76
	moveq	r0, r4
	moveq	r2, r3
	moveq	r3, ip
	beq	L248
L247:
	cmp	r1, #31
	bgt	L249
	mov	r0, r3, asl r2
	mov	r3, r3, lsr r1
	orr	r2, r3, ip, asl r2
	mov	r3, ip, lsr r1
	b	L250
L249:
	cmp	r1, #32
	moveq	r0, r3
	moveq	r2, ip
	beq	L252
L251:
	orr	r4, r4, r3
	cmp	r1, #63
	movle	r0, ip, asl r2
	andle	r3, r1, #31
	movle	r2, ip, lsr r3
	ble	L252
L253:
	cmp	r1, #64
	beq	L255
	subs	r0, ip, #0
	movne	r0, #1
	b	L256
L255:
	mov	r0, ip
L256:
	mov	r2, #0
L252:
	mov	r3, #0
L250:
	cmp	r4, #0
	orrne	r0, r0, #1
L248:
	str	r0, [lr, #0]
	ldr	r5, [sp, #12]
	str	r2, [r5, #0]
	ldr	r6, [sp, #4]
	str	r3, [r6, #0]
	ldr	r7, [sp, #0]
	mov	r8, #0
	cmp	r7, r8
	beq	L258
	ldr	r3, [sp, #76]
	cmp	r3, r8
	movne	r0, #16
	blne	_float_raise
L258:
	ldr	r5, [sp, #28]
	cmp	r5, #0
	ldrne	r3, [sp, #76]
	movne	r0, r3, lsr #31
	bne	L233
L259:
	cmp	r9, #0
	beq	L261
	add	r3, sp, #28
	ldmia	r3, {r3, r6}	@ phole ldm
	cmp	r6, #1
	b	L274
L261:
	ldr	r7, [sp, #32]
	mov	r3, r9
	cmp	r7, #2
L274:
	bne	L264
	ldr	r3, [sp, #76]
	subs	r3, r3, #0
	movne	r3, #1
L264:
	mov	r0, r3
L233:
	ldr	r3, [sp, #76]
	cmp	r3, #0
	ldrne	r2, L275+8
	ldrneb	r3, [r2, #0]
	orrne	r3, r3, #32
	strneb	r3, [r2, #0]
L265:
	cmp	r0, #0
	beq	L266
	ldr	r1, [sp, #72]
	ldr	r3, [sp, #40]
	add	r2, r1, #1
	str	r2, [sp, #72]
	cmp	r2, r1
	addcc	r3, r3, #1
	str	r3, [sp, #40]
	ldr	r3, [sp, #76]
	movs	r3, r3, asl #1
	ldreq	r5, [sp, #28]
	mvneq	r3, r5
	andeq	r2, r2, r3
L268:
	str	r2, [sp, #72]
	b	L269
L276:
	.align	0
L275:
	.word	_float_rounding_mode
	.word	_float_detect_tininess
	.word	_float_exception_flags
L266:
	ldr	r3, [sp, #40]
	ldr	r2, [sp, #72]
	orrs	r3, r3, r2
	moveq	r8, #0
L269:
	ldr	r6, [sp, #72]
	ldr	r2, [sp, #40]
	mov	r3, r8, asl #20
	add	r3, r3, r9, asl #31
	str	r6, [sp, #24]
	add	r3, r3, r2
	str	r3, [sp, #20]
	add	r5, sp, #20
	ldmia	r5, {r5-r6}
L272:
	ldr	r7, [sp, #36]
L273:
	stmia	r7, {r5-r6}
	ldr	r0, [sp, #36]
	add	sp, sp, #44
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, pc}^
	.align	0
_normalizeRoundAndPackFloat64:
	@ args = 4, pretend = 0, frame = 28
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #36
	str	r3, [sp, #32]
	and	r1, r1, #255
	str	r1, [sp, #16]
	mov	r5, r2
	ldr	r2, [sp, #64]
	mov	r9, r0
	cmp	r3, #0
	streq	r2, [sp, #32]
	subeq	r5, r5, #32
	streq	r3, [sp, #64]
L278:
	ldr	r0, [sp, #32]
	bl	_countLeadingZeros32
	sub	r0, r0, #11
	and	r8, r0, #255
	mov	r3, r8, asl #24
	mov	r2, r3, asr #24
	cmp	r2, #0
	blt	L279
	mov	r3, #0
	str	r3, [sp, #28]
	ldr	r1, [sp, #64]
	ldr	r0, [sp, #32]
	mov	r3, r1, asl r2
	str	r3, [sp, #64]
	beq	L280
	rsb	r3, r2, #0
	and	r3, r3, #31
	mov	r3, r1, lsr r3
	orr	r3, r3, r0, asl r2
	str	r3, [sp, #32]
	b	L283
L280:
	str	r0, [sp, #32]
	b	L283
L279:
	add	r6, sp, #32
	str	r6, [sp, #8]
	add	r7, sp, #64
	str	r7, [sp, #12]
	ldr	ip, [sp, #32]
	ldr	r3, [sp, #64]
	mov	lr, #0
	rsb	r1, r2, #0
	cmp	r1, lr
	add	r4, sp, #28
	and	r2, r2, #31
	moveq	r0, lr
	moveq	r2, r3
	moveq	r3, ip
	beq	L285
L284:
	cmp	r1, #31
	bgt	L286
	mov	r0, r3, asl r2
	mov	r3, r3, lsr r1
	orr	r2, r3, ip, asl r2
	mov	r3, ip, lsr r1
	b	L287
L286:
	cmp	r1, #32
	moveq	r0, r3
	moveq	r2, ip
	beq	L289
L288:
	mov	lr, r3
	cmp	r1, #63
	movle	r0, ip, asl r2
	andle	r3, r1, #31
	movle	r2, ip, lsr r3
	ble	L289
L290:
	cmp	r1, #64
	beq	L292
	subs	r0, ip, #0
	movne	r0, #1
	b	L293
L292:
	mov	r0, ip
L293:
	mov	r2, #0
L289:
	mov	r3, #0
L287:
	cmp	lr, #0
	orrne	r0, r0, #1
L285:
	str	r0, [r4, #0]
	ldr	r6, [sp, #12]
	str	r2, [r6, #0]
	ldr	r7, [sp, #8]
	str	r3, [r7, #0]
L283:
	ldr	r2, [sp, #64]
	ldr	r3, [sp, #32]
	str	r2, [sp, #0]
	ldr	r2, [sp, #28]
	str	r2, [sp, #4]
	ldr	r1, [sp, #16]
	add	r4, sp, #20
	mov	r0, r4
	mov	r2, r8, asl #24
	sub	r2, r5, r2, asr #24
	bl	_roundAndPackFloat64
	ldmia	r4, {r6-r7}
	mov	r0, r9
	stmia	r9, {r6-r7}
	add	sp, sp, #36
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, pc}^
	.align	0
	.global	___floatsisf
___floatsisf:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	@ I don't think this function clobbers lr
	subs	r2, r0, #0
	moveq	r0, r2
	moveqs	pc, lr
L296:
	cmp	r2, #-2147483648
	beq	L297
	mov	r0, r2, lsr #31
	mov	r1, #156
	cmp	r2, #0
	rsblt	r2, r2, #0
	b	_normalizeRoundAndPackFloat32
L297:
	mov	r0, #-822083584
	movs	pc, lr
	.align	0
	.global	___floatsidf
___floatsidf:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	cmp	r1, #0
	sub	sp, sp, #8
	mov	r7, r0
	moveq	r3, r1
	moveq	r2, r1
	stmeqia	r7, {r2-r3}
	beq	L301
L302:
	eor	r4, r1, r1, asr #31
	sub	r4, r4, r1, asr #31
	mov	r0, r4
	mov	r8, r1, lsr #31
	bl	_countLeadingZeros32
	sub	r0, r0, #11
	and	r1, r0, #255
	mov	r3, r1, asl #24
	movs	r3, r3, asr #24
	bmi	L305
	mov	r3, r4, asl r3
	str	r3, [sp, #4]
	mov	r3, #0
	str	r3, [sp, #0]
	b	L306
L305:
	rsb	r0, r3, #0
	cmp	r0, #0
	add	ip, sp, #4
	mov	r2, sp
	and	r3, r3, #31
	moveq	r3, r0
	moveq	r0, r4
	beq	L308
L307:
	cmp	r0, #31
	movle	r3, r4, asl r3
	movle	r0, r4, lsr r0
	ble	L308
L309:
	cmp	r0, #63
	andle	r3, r0, #31
	movle	r3, r4, lsr r3
L311:
	movgt	r3, #0
L312:
	mov	r0, #0
L308:
	str	r3, [r2, #0]
	str	r0, [ip, #0]
L306:
	ldr	r6, [sp, #0]
	ldr	r3, [sp, #4]
	mov	r2, #1040
	add	r2, r2, #2
	mov	r1, r1, asl #24
	sub	r2, r2, r1, asr #24
	mov	r2, r2, asl #20
	add	r2, r2, r8, asl #31
	add	r5, r2, r3
	stmia	r7, {r5-r6}
L301:
	mov	r0, r7
	add	sp, sp, #8
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}^
	.align	0
	.global	_float32_to_int32
_float32_to_int32:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	mov	r3, r0, lsr #23
	bic	r4, r0, #-16777216
	bic	r4, r4, #8388608
	and	r5, r3, #255
	mov	r3, r5
	mov	r6, r0, lsr #31
	subs	r2, r5, #150
	bmi	L319
	cmp	r5, #157
	ble	L320
	cmp	r0, #-822083584
	mov	r0, #-2147483648
	ldmeqfd	sp!, {r4, r5, r6, pc}^
	mov	r0, #1
	bl	_float_raise
	cmp	r6, #0
	beq	L323
	subs	r3, r4, #0
	movne	r3, #1
	cmp	r5, #255
	movne	r3, #0
	cmp	r3, #0
	beq	L322
L323:
	mvn	r0, #-2147483648
	ldmfd	sp!, {r4, r5, r6, pc}^
L322:
	mov	r0, #-2147483648
	ldmfd	sp!, {r4, r5, r6, pc}^
L320:
	orr	r3, r4, #8388608
	mov	r0, r3, asl r2
	cmp	r6, #0
	rsbne	r0, r0, #0
	ldmfd	sp!, {r4, r5, r6, pc}^
L319:
	cmp	r3, #125
	orrle	r1, r3, r4
	movle	r0, #0
	ble	L327
L326:
	orr	r4, r4, #8388608
	and	r3, r2, #31
	mov	r1, r4, asl r3
	rsb	r3, r2, #0
	mov	r0, r4, lsr r3
L327:
	cmp	r1, #0
	ldrne	r2, L341
	ldrneb	r3, [r2, #0]
	orrne	r3, r3, #32
	strneb	r3, [r2, #0]
L328:
	ldr	r3, L341+4
	ldrb	r3, [r3, #0]	@ zero_extendqisi2
	mov	r3, r3, asl #24
	movs	r2, r3, asr #24
	bne	L329
	cmp	r1, #0
	bge	L330
	add	r0, r0, #1
	movs	r1, r1, asl #1
	bic	r3, r0, #1
	moveq	r0, r3
L330:
	cmp	r6, #0
	rsbne	r0, r0, #0
	ldmfd	sp!, {r4, r5, r6, pc}^
L329:
	subs	r1, r1, #0
	movne	r1, #1
	cmp	r6, #0
	beq	L334
	add	r3, r0, r1
	cmp	r2, #1
	moveq	r0, r3
	rsb	r0, r0, #0
	ldmfd	sp!, {r4, r5, r6, pc}^
L334:
	and	r3, r1, #1
	add	r3, r0, r3
	cmp	r2, #2
	moveq	r0, r3
	ldmfd	sp!, {r4, r5, r6, pc}^
L342:
	.align	0
L341:
	.word	_float_exception_flags
	.word	_float_rounding_mode
	.align	0
	.global	___fixsfsi
___fixsfsi:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	mov	r3, r0, lsr #23
	bic	r5, r0, #-16777216
	bic	r5, r5, #8388608
	and	r4, r3, #255
	mov	r3, r4
	mov	r6, r0, lsr #31
	subs	r2, r4, #158
	bmi	L347
	cmp	r0, #-822083584
	mov	r0, #-2147483648
	ldmeqfd	sp!, {r4, r5, r6, pc}^
	mov	r0, #1
	bl	_float_raise
	cmp	r6, #0
	beq	L350
	subs	r3, r5, #0
	movne	r3, #1
	cmp	r4, #255
	movne	r3, #0
	cmp	r3, #0
	beq	L349
L350:
	mvn	r0, #-2147483648
	ldmfd	sp!, {r4, r5, r6, pc}^
L349:
	mov	r0, #-2147483648
	ldmfd	sp!, {r4, r5, r6, pc}^
L347:
	cmp	r3, #126
	bgt	L351
	orrs	r3, r3, r5
	ldrne	r2, L357
	ldrneb	r3, [r2, #0]
	orrne	r3, r3, #32
	strneb	r3, [r2, #0]
L353:
	mov	r0, #0
	ldmfd	sp!, {r4, r5, r6, pc}^
L351:
	orr	r3, r5, #8388608
	mov	r5, r3, asl #8
	and	r3, r2, #31
	movs	r3, r5, asl r3
	rsb	r3, r2, #0
	mov	r1, r5, lsr r3
	ldrne	r2, L357
	ldrneb	r3, [r2, #0]
	orrne	r3, r3, #32
	strneb	r3, [r2, #0]
L354:
	cmp	r6, #0
	moveq	r0, r1
	rsbne	r0, r1, #0
	ldmfd	sp!, {r4, r5, r6, pc}^
L358:
	.align	0
L357:
	.word	_float_exception_flags
	.align	0
	.global	___extendsfdf2
___extendsfdf2:
	@ args = 0, pretend = 0, frame = 60
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #60
	str	r0, [sp, #20]
	mov	r3, r1, lsr #23
	and	r3, r3, #255
	str	r3, [sp, #36]
	mov	r5, r1, lsr #31
	str	r5, [sp, #16]
	bic	r0, r1, #-16777216
	bic	r0, r0, #8388608
	str	r0, [sp, #32]
	cmp	r3, #255
	bne	L363
	cmp	r0, #0
	beq	L364
	add	r4, sp, #48
	mov	r0, r4
	bl	_float32ToCommonNaN
	ldmia	r4, {r1-r3}	@ load multiple
	add	r4, sp, #40
	mov	r0, r4
	bl	_commonNaNToFloat64
	ldmia	r4, {r5-r6}
	ldr	ip, [sp, #20]
	stmia	ip, {r5-r6}
	b	L359
L364:
	str	r0, [sp, #12]
	ldr	r5, [sp, #16]
	mov	r3, r5, asl #31
	orr	r3, r3, #2130706432
	str	r3, [sp, #8]
	mov	r6, r3
	orr	r6, r6, #15728640
	str	r6, [sp, #8]
	ldr	ip, [sp, #20]
	add	r5, sp, #8
	ldmia	r5, {r5-r6}
	stmia	ip, {r5-r6}
	b	L359
L363:
	cmp	r3, #0
	bne	L366
	subs	r3, r0, #0
	bne	L367
	str	r3, [sp, #4]
	ldr	r6, [sp, #16]
	mov	r6, r6, asl #31
	str	r6, [sp, #0]
	ldr	ip, [sp, #20]
	ldmia	sp, {r5-r6}
	stmia	ip, {r5-r6}
	b	L359
L367:
	add	r1, sp, #36
	add	r2, sp, #32
	bl	_normalizeFloat32Subnormal
	ldr	r3, [sp, #36]
	sub	r3, r3, #1
	str	r3, [sp, #36]
L366:
	ldr	r3, [sp, #32]
	mov	r1, r3, asl #29
	str	r1, [sp, #24]
	mov	r3, r3, lsr #3
	str	r3, [sp, #28]
	ldr	r2, [sp, #36]
	add	r5, sp, #16
	ldmia	r5, {r5, r6}	@ phole ldm
	mov	r9, r1
	add	r2, r2, #896
	mov	r2, r2, asl #20
	add	r2, r2, r5, asl #31
	add	r8, r2, r3
	stmia	r6, {r8-r9}
L359:
	ldr	r0, [sp, #20]
	add	sp, sp, #60
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, pc}^
	.align	0
	.global	_float32_round_to_int
_float32_round_to_int:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, lr}
	mov	ip, r0
	mov	r3, ip, lsr #23
	and	r1, r3, #255
	cmp	r1, #149
	mov	r0, r1
	ble	L379
	cmp	r1, #255
	bne	L410
	bic	r3, ip, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	beq	L410
	mov	r0, ip
	mov	r1, r0
	ldmfd	sp!, {r4, lr}
	b	_propagateFloat32NaN
L379:
	cmp	r1, #126
	bgt	L382
	movs	r4, ip, asl #1
	bne	L383
L410:
	mov	r0, ip
	ldmfd	sp!, {r4, pc}^
L383:
	ldr	r2, L411
	ldrb	r3, [r2, #0]
	orr	r3, r3, #32
	strb	r3, [r2, #0]
	ldr	r3, L411+4
	ldrb	r3, [r3, #0]
	mov	r0, ip, lsr #31
	mov	r3, r3, asl #24
	mov	r3, r3, asr #24
	cmp	r3, #1
	beq	L390
	bgt	L398
	cmp	r3, #0
	beq	L386
	b	L385
L398:
	cmp	r3, #2
	beq	L393
	b	L385
L386:
	cmp	r1, #126
	bne	L385
	bic	r3, ip, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	beq	L385
	mov	r0, r0, asl #31
	orr	r0, r0, #1065353216
	ldmfd	sp!, {r4, pc}^
L390:
	cmp	r0, #0
	movne	r0, #-1090519040
	addne	r0, r0, #8388608
	ldmfd	sp!, {r4, pc}^
L393:
	cmp	r0, #0
	moveq	r0, #1065353216
	movne	r0, #-2147483648
	ldmfd	sp!, {r4, pc}^
L385:
	mov	r0, r0, asl #31
	ldmfd	sp!, {r4, pc}^
L382:
	ldr	r3, L411+4
	ldrb	r2, [r3, #0]	@ zero_extendqisi2
	mov	r1, #1
	rsb	r3, r0, #150
	mov	r1, r1, asl r3
	mov	r0, ip
	sub	lr, r1, #1
	mov	r2, r2, asl #24
	movs	r2, r2, asr #24
	bne	L400
	add	r0, r0, r1, lsr #1
	tst	r0, lr
	bic	r3, r0, r1
	moveq	r0, r3
	b	L402
L400:
	cmp	r2, #3
	beq	L402
	cmp	r2, #2
	mov	r3, r0, lsr #31
	bne	L406
	cmp	r3, #0
	beq	L407
	b	L402
L406:
	cmp	r3, #0
	beq	L402
L407:
	add	r0, r0, lr
L402:
	bic	r0, r0, lr
	cmp	r0, ip
	ldmeqfd	sp!, {r4, pc}^
	ldr	r2, L411
	ldrb	r3, [r2, #0]
	orr	r3, r3, #32
	strb	r3, [r2, #0]
	ldmfd	sp!, {r4, pc}^
L412:
	.align	0
L411:
	.word	_float_exception_flags
	.word	_float_rounding_mode
	.align	0
_addFloat32Sigs:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	sub	sp, sp, #8
	mov	r5, r1
	bic	r1, r0, #-16777216
	bic	r1, r1, #8388608
	str	r1, [sp, #0]
	bic	r3, r5, #-16777216
	bic	r3, r3, #8388608
	str	r3, [sp, #4]
	mov	r4, r1, asl #6
	str	r4, [sp, #0]
	mov	r1, r3, asl #6
	str	r1, [sp, #4]
	and	r7, r2, #255
	mov	r3, r0, lsr #23
	and	lr, r3, #255
	mov	r3, r5, lsr #23
	and	r6, r3, #255
	rsb	ip, r6, lr
	cmp	ip, #0
	mov	r8, lr
	ble	L418
	cmp	lr, #255
	bne	L419
	cmp	r4, #0
	beq	L447
	b	L448
L419:
	cmp	r6, #0
	subeq	ip, ip, #1
L421:
	orrne	r3, r1, #536870912
	strne	r3, [sp, #4]
L422:
	ldr	r2, [sp, #4]
	cmp	ip, #0
	add	r0, sp, #4
	moveq	r3, r2
	beq	L424
L423:
	cmp	ip, #31
	bgt	L425
	rsb	r3, ip, #0
	and	r3, r3, #31
	movs	r3, r2, asl r3
	moveq	r3, #0
	movne	r3, #1
	orr	r3, r3, r2, lsr ip
	b	L424
L425:
	subs	r3, r2, #0
	movne	r3, #1
L424:
	str	r3, [r0, #0]
	mov	r1, r8
	b	L428
L418:
	bge	L429
	cmp	r6, #255
	bne	L430
	cmp	r1, #0
	bne	L448
	mov	r0, r7, asl #31
	orr	r0, r0, #2130706432
	orr	r0, r0, #8388608
	b	L447
L430:
	cmp	lr, #0
	addeq	ip, ip, #1
L433:
	orrne	r3, r4, #536870912
	strne	r3, [sp, #0]
L434:
	ldr	r0, [sp, #0]
	rsb	r2, ip, #0
	cmp	r2, #0
	mov	r1, sp
	moveq	r3, r0
	beq	L436
L435:
	cmp	r2, #31
	bgt	L437
	and	r3, ip, #31
	movs	r3, r0, asl r3
	moveq	r3, #0
	movne	r3, #1
	orr	r3, r3, r0, lsr r2
	b	L436
L437:
	subs	r3, r0, #0
	movne	r3, #1
L436:
	str	r3, [r1, #0]
	mov	r1, r6
	b	L428
L429:
	cmp	lr, #255
	bne	L441
	orrs	r4, r4, r1
	beq	L447
L448:
	mov	r1, r5
	bl	_propagateFloat32NaN
	b	L447
L441:
	cmp	lr, #0
	addeq	r0, r4, r1
	moveq	r0, r0, lsr #6
	orreq	r0, r0, r7, asl #31
	beq	L447
L443:
	add	r3, r1, #1073741824
	add	r2, r4, r3
	mov	r1, r8
	b	L445
L428:
	ldr	r3, [sp, #0]
	orr	r3, r3, #536870912
	str	r3, [sp, #0]
	ldr	r2, [sp, #4]
	sub	r1, r1, #1
	add	r3, r3, r2
	movs	r2, r3, asl #1
	movmi	r2, r3
	addmi	r1, r1, #1
L445:
	mov	r0, r7
	bl	_roundAndPackFloat32
L447:
	add	sp, sp, #8
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}^
	.align	0
_subFloat32Sigs:
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	sub	sp, sp, #8
	mov	r4, r0
	mov	r5, r1
	bic	r1, r4, #-16777216
	bic	r1, r1, #8388608
	str	r1, [sp, #4]
	bic	r3, r5, #-16777216
	bic	r3, r3, #8388608
	str	r3, [sp, #0]
	mov	r0, r1, asl #7
	str	r0, [sp, #4]
	mov	lr, r3, asl #7
	str	lr, [sp, #0]
	and	r7, r2, #255
	mov	r3, r4, lsr #23
	and	r1, r3, #255
	mov	r3, r5, lsr #23
	and	r3, r3, #255
	mov	r6, r3
	rsb	ip, r3, r1
	cmp	ip, #0
	bgt	L455
	blt	L457
	cmp	r1, #255
	bne	L458
	orrs	r0, r0, lr
	bne	L487
	mov	r0, #1
	bl	_float_raise
	mov	r0, #-2147483648
	mov	r0, r0, asr #9
	b	L486
L458:
	cmp	r1, #0
	moveq	r1, #1
	moveq	r6, r1
L460:
	cmp	lr, r0
	bcc	L462
	cmp	r0, lr
	bcc	L464
	ldr	r3, L488
	ldrb	r0, [r3, #0]
	cmp	r0, #1
	movne	r0, #0
	moveq	r0, #1
	mov	r0, r0, asl #31
	b	L486
L457:
	cmp	r3, #255
	bne	L466
	cmp	lr, #0
	bne	L487
	eor	r0, r7, #1
	mov	r0, r0, asl #31
	orr	r0, r0, #2130706432
	orr	r0, r0, #8388608
	b	L486
L466:
	cmp	r1, #0
	addeq	ip, ip, #1
L469:
	orrne	r3, r0, #1073741824
	strne	r3, [sp, #4]
L470:
	ldr	r2, [sp, #4]
	rsb	lr, ip, #0
	cmp	lr, #0
	add	r1, sp, #4
	moveq	r3, r2
	beq	L472
L471:
	cmp	lr, #31
	bgt	L473
	and	r3, ip, #31
	movs	r3, r2, asl r3
	moveq	r3, #0
	movne	r3, #1
	orr	r3, r3, r2, lsr lr
	b	L472
L473:
	subs	r3, r2, #0
	movne	r3, #1
L472:
	str	r3, [r1, #0]
	ldr	r3, [sp, #0]
	orr	r3, r3, #1073741824
	str	r3, [sp, #0]
L464:
	ldmia	sp, {r2, r8}
	sub	r2, r2, r8
	mov	r1, r6
	eor	r7, r7, #1
	b	L476
L455:
	cmp	r1, #255
	bne	L477
	cmp	r0, #0
	beq	L478
L487:
	mov	r0, r4
	mov	r1, r5
	bl	_propagateFloat32NaN
	b	L486
L478:
	mov	r0, r4
	b	L486
L477:
	cmp	r6, #0
	subeq	ip, ip, #1
L479:
	orrne	r3, lr, #1073741824
	strne	r3, [sp, #0]
L480:
	ldr	lr, [sp, #0]
	cmp	ip, #0
	mov	r4, sp
	moveq	r3, lr
	beq	L482
L481:
	cmp	ip, #31
	bgt	L483
	rsb	r3, ip, #0
	and	r3, r3, #31
	movs	r3, lr, asl r3
	moveq	r3, #0
	movne	r3, #1
	orr	r3, r3, lr, lsr ip
	b	L482
L489:
	.align	0
L488:
	.word	_float_rounding_mode
L483:
	subs	r3, lr, #0
	movne	r3, #1
L482:
	str	r3, [r4, #0]
	ldr	r3, [sp, #4]
	orr	r3, r3, #1073741824
	str	r3, [sp, #4]
L462:
	ldmia	sp, {r2, r8}
	sub	r2, r8, r2
L476:
	mov	r0, r7
	sub	r1, r1, #1
	bl	_normalizeRoundAndPackFloat32
L486:
	add	sp, sp, #8
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}^
	.align	0
	.global	___addsf3
___addsf3:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	@ I don't think this function clobbers lr
	mov	r2, r0, lsr #31
	cmp	r2, r1, lsr #31
	bne	_subFloat32Sigs
L493:
	b	_addFloat32Sigs
	.align	0
	.global	___subsf3
___subsf3:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	@ I don't think this function clobbers lr
	mov	r2, r0, lsr #31
	cmp	r2, r1, lsr #31
	bne	_addFloat32Sigs
L499:
	b	_subFloat32Sigs
	.align	0
	.global	___mulsf3
___mulsf3:
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, lr}
	sub	sp, sp, #24
	mov	r5, r0
	bic	r0, r5, #-16777216
	bic	r0, r0, #8388608
	str	r0, [sp, #16]
	mov	r3, r5, lsr #23
	and	lr, r3, #255
	str	lr, [sp, #20]
	bic	r4, r1, #-16777216
	bic	r4, r4, #8388608
	str	r4, [sp, #8]
	mov	r3, r1, lsr #23
	and	ip, r3, #255
	str	ip, [sp, #12]
	mov	r6, lr
	cmp	r6, #255
	mov	r2, r5, lsr #31
	mov	r3, r1, lsr #31
	eor	r7, r2, r3
	bne	L509
	cmp	r0, #0
	bne	L529
	subs	r3, r4, #0
	movne	r3, #1
	cmp	ip, #255
	movne	r3, #0
	cmp	r3, #0
	bne	L529
	orrs	ip, ip, r4
	beq	L530
	b	L516
L509:
	cmp	ip, #255
	bne	L514
	cmp	r4, #0
	beq	L515
L529:
	mov	r0, r5
	bl	_propagateFloat32NaN
	b	L528
L515:
	orrs	r6, r6, r0
	bne	L516
L530:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #-2147483648
	mov	r0, r0, asr #9
	b	L528
L516:
	mov	r0, r7, asl #31
	orr	r0, r0, #2130706432
	orr	r0, r0, #8388608
	b	L528
L514:
	cmp	lr, #0
	bne	L518
	cmp	r0, #0
	beq	L531
	add	r1, sp, #20
	add	r2, sp, #16
	bl	_normalizeFloat32Subnormal
L518:
	ldr	r3, [sp, #12]
	cmp	r3, #0
	bne	L521
	ldr	r0, [sp, #8]
	cmp	r0, #0
	bne	L522
L531:
	mov	r0, r7, asl #31
	b	L528
L522:
	add	r1, sp, #12
	add	r2, sp, #8
	bl	_normalizeFloat32Subnormal
L521:
	ldr	r3, [sp, #16]
	ldr	r2, [sp, #8]
	orr	r3, r3, #8388608
	orr	r2, r2, #8388608
	mov	ip, r3, asl #23
	mov	r0, r2, asl #24
	mov	r3, r3, asl #7
	mov	r2, r2, asl #8
	mov	lr, r2, lsr #16
	mov	r1, r3, lsr #16
	mul	r4, lr, r1
	mov	ip, ip, lsr #16
	mov	r0, r0, lsr #16
	mul	r5, r0, ip
	mul	r1, r0, r1
	str	r3, [sp, #16]
	str	r2, [sp, #8]
	mla	r0, lr, ip, r1
	ldr	r3, [sp, #20]
	ldr	r2, [sp, #12]
	add	r3, r3, r2
	sub	ip, r3, #127
	cmp	r0, r1
	add	lr, sp, #4
	mov	r1, sp
	mov	r2, r0, lsr #16
	addcc	r3, r4, #65536
	addcc	r4, r3, r2
L524:
	addcs	r4, r4, r2
L525:
	mov	r0, r0, asl #16
	adds	r5, r5, r0
	str	r5, [r1, #0]
	addcs	r4, r4, #1
	str	r4, [lr, #0]
	ldmia	sp, {r2, r3}	@ phole ldm
	cmp	r2, #0
	orrne	r3, r3, #1
	str	r3, [sp, #4]
	movs	r3, r3, asl #1
	strpl	r3, [sp, #4]
	subpl	ip, ip, #1
L527:
	ldr	r2, [sp, #4]
	mov	r0, r7
	mov	r1, ip
	bl	_roundAndPackFloat32
L528:
	add	sp, sp, #24
	ldmfd	sp!, {r4, r5, r6, r7, pc}^
	.align	0
	.global	___divsf3
___divsf3:
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	sub	sp, sp, #24
	mov	lr, r0
	bic	r5, lr, #-16777216
	bic	r5, r5, #8388608
	str	r5, [sp, #8]
	mov	r3, lr, lsr #23
	and	r4, r3, #255
	str	r4, [sp, #12]
	bic	r0, r1, #-16777216
	bic	r0, r0, #8388608
	str	r0, [sp, #16]
	mov	r3, r1, lsr #23
	and	ip, r3, #255
	str	ip, [sp, #20]
	cmp	r4, #255
	mov	r2, lr, lsr #31
	mov	r3, r1, lsr #31
	eor	r6, r2, r3
	bne	L539
	cmp	r5, #0
	bne	L566
	cmp	ip, #255
	bne	L568
	cmp	r0, #0
	bne	L566
	b	L567
L539:
	cmp	ip, #255
	bne	L544
	cmp	r0, #0
	beq	L569
L566:
	mov	r0, lr
	bl	_propagateFloat32NaN
	b	L565
L544:
	cmp	ip, #0
	bne	L547
	cmp	r0, #0
	bne	L548
	orrs	r4, r4, r5
	bne	L549
L567:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #-2147483648
	mov	r0, r0, asr #9
	b	L565
L549:
	mov	r0, #4
	bl	_float_raise
L568:
	mov	r0, r6, asl #31
	orr	r0, r0, #2130706432
	orr	r0, r0, #8388608
	b	L565
L548:
	add	r1, sp, #20
	add	r2, sp, #16
	bl	_normalizeFloat32Subnormal
L547:
	ldr	r3, [sp, #12]
	cmp	r3, #0
	bne	L551
	ldr	r0, [sp, #8]
	cmp	r0, #0
	bne	L552
L569:
	mov	r0, r6, asl #31
	b	L565
L552:
	add	r1, sp, #12
	add	r2, sp, #8
	bl	_normalizeFloat32Subnormal
L551:
	ldr	r1, [sp, #8]
	ldr	r3, [sp, #16]
	orr	r1, r1, #8388608
	mov	r0, r1, asl #7
	str	r0, [sp, #8]
	orr	r3, r3, #8388608
	mov	ip, r3, asl #8
	str	ip, [sp, #16]
	ldr	r2, [sp, #12]
	ldr	r3, [sp, #20]
	cmp	ip, r1, asl #8
	rsb	r2, r3, r2
	add	r7, r2, #125
	movls	r3, r0, lsr #1
	strls	r3, [sp, #8]
	addls	r7, r2, #126
L554:
	ldr	r0, [sp, #8]
	mov	r1, #0
	mov	r2, ip
	bl	_estimateDiv64To32
	mov	lr, r0
	and	r3, lr, #63
	cmp	r3, #2
	bhi	L555
	ldr	r2, [sp, #16]
	mov	r3, lr, asl #16
	mov	r3, r3, lsr #16
	mov	r1, r2, asl #16
	mov	r1, r1, lsr #16
	mul	r4, r3, r1
	mov	r0, lr, lsr #16
	mov	r2, r2, lsr #16
	mul	ip, r0, r2
	mul	r3, r2, r3
	mla	r2, r0, r1, r3
	add	r5, sp, #4
	cmp	r2, r3
	mov	r0, sp
	mov	r1, r2, lsr #16
	addcc	r3, ip, #65536
	addcc	ip, r3, r1
L556:
	addcs	ip, ip, r1
L557:
	mov	r2, r2, asl #16
	adds	r4, r4, r2
	str	r4, [r0, #0]
	addcs	ip, ip, #1
	str	ip, [r5, #0]
	ldr	r3, [sp, #0]
	add	r8, sp, #8
	ldmda	r8, {r2, r8}
	sub	r2, r8, r2
	rsb	r1, r3, #0
	cmp	r3, #0
	subne	r2, r2, #1
	cmp	r2, #0
	bge	L561
	ldr	r0, [sp, #16]
L562:
	sub	lr, lr, #1
	mov	r3, r1
	add	r1, r1, r0
	cmp	r1, r3
	movcs	r3, r2
	addcc	r3, r2, #1
	mov	r2, r3
	cmp	r2, #0
	blt	L562
L561:
	cmp	r1, #0
	orrne	lr, lr, #1
L555:
	mov	r0, r6
	mov	r1, r7
	mov	r2, lr
	bl	_roundAndPackFloat32
L565:
	add	sp, sp, #24
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}^
	.align	0
	.global	_float32_rem
_float32_rem:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	sub	sp, sp, #16
	mov	r5, r0
	bic	lr, r5, #-16777216
	bic	lr, lr, #8388608
	str	lr, [sp, #0]
	mov	r2, r5, lsr #23
	and	r2, r2, #255
	str	r2, [sp, #4]
	bic	r0, r1, #-16777216
	bic	r0, r0, #8388608
	str	r0, [sp, #8]
	mov	r3, r1, lsr #23
	and	ip, r3, #255
	str	ip, [sp, #12]
	cmp	r2, #255
	mov	r6, r5, lsr #31
	bne	L577
	cmp	lr, #0
	bne	L607
	subs	r3, r0, #0
	movne	r3, #1
	cmp	ip, #255
	movne	r3, #0
	cmp	r3, #0
	bne	L607
	b	L608
L577:
	cmp	ip, #255
	bne	L580
	cmp	r0, #0
	beq	L609
L607:
	mov	r0, r5
	bl	_propagateFloat32NaN
	b	L606
L580:
	cmp	ip, #0
	bne	L582
	cmp	r0, #0
	bne	L583
L608:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #-2147483648
	mov	r0, r0, asr #9
	b	L606
L583:
	add	r1, sp, #12
	add	r2, sp, #8
	bl	_normalizeFloat32Subnormal
L582:
	ldr	r3, [sp, #4]
	cmp	r3, #0
	bne	L584
	ldr	r0, [sp, #0]
	cmp	r0, #0
	beq	L609
	add	r1, sp, #4
	mov	r2, sp
	bl	_normalizeFloat32Subnormal
L584:
	ldr	r3, [sp, #0]
	orr	r3, r3, #8388608
	mov	r1, r3, asl #8
	ldr	r3, [sp, #8]
	str	r1, [sp, #0]
	orr	r3, r3, #8388608
	mov	r3, r3, asl #8
	str	r3, [sp, #8]
	ldr	r2, [sp, #4]
	ldr	r3, [sp, #12]
	subs	r4, r2, r3
	bpl	L586
	cmn	r4, #1
	bge	L587
L609:
	mov	r0, r5
	b	L606
L587:
	mov	r3, r1, lsr #1
	str	r3, [sp, #0]
L586:
	ldr	r2, [sp, #8]
	ldr	r3, [sp, #0]
	cmp	r2, r3
	movhi	r0, #0
	movls	r0, #1
	cmp	r0, #0
	rsbne	r3, r2, r3
	strne	r3, [sp, #0]
L588:
	sub	r4, r4, #32
	cmp	r4, #0
	ble	L590
L591:
	ldr	r0, [sp, #0]
	ldr	r2, [sp, #8]
	mov	r1, #0
	bl	_estimateDiv64To32
	ldr	r3, [sp, #8]
	cmp	r0, #2
	subhi	r0, r0, #2
	movls	r0, #0
	mov	r3, r3, lsr #2
	mul	r3, r0, r3
	rsb	r3, r3, #0
	str	r3, [sp, #0]
	sub	r4, r4, #30
	cmp	r4, #0
	bgt	L591
L590:
	add	r4, r4, #32
	cmp	r4, #0
	ble	L595
	ldr	r0, [sp, #0]
	ldr	r2, [sp, #8]
	mov	r1, #0
	bl	_estimateDiv64To32
	ldr	r2, [sp, #8]
	mov	r2, r2, lsr #2
	str	r2, [sp, #8]
	cmp	r0, #2
	subhi	r0, r0, #2
	movls	r0, #0
	rsb	r3, r4, #32
	mov	r0, r0, lsr r3
	mul	r1, r0, r2
	ldr	r3, [sp, #0]
	mov	r3, r3, lsr #1
	sub	r2, r4, #1
	rsb	r1, r1, r3, asl r2
	str	r1, [sp, #0]
	b	L598
L595:
	ldr	r3, [sp, #0]
	mov	r3, r3, lsr #2
	str	r3, [sp, #0]
	ldr	r3, [sp, #8]
	mov	r3, r3, lsr #2
	str	r3, [sp, #8]
L598:
	ldr	r1, [sp, #8]
L599:
	ldr	r2, [sp, #0]
	rsb	r3, r1, r2
	str	r3, [sp, #0]
	add	r0, r0, #1
	cmp	r3, #0
	bge	L599
	add	r3, r3, r2
	cmp	r3, #0
	blt	L604
	bne	L603
	tst	r0, #1
	beq	L603
L604:
	str	r2, [sp, #0]
L603:
	ldr	r2, [sp, #0]
	mov	r3, r2, lsr #31
	cmp	r3, #0
	mov	r0, r3
	rsbne	r3, r2, #0
	strne	r3, [sp, #0]
L605:
	ldr	r1, [sp, #12]
	ldr	r2, [sp, #0]
	eor	r0, r6, r0
	bl	_normalizeRoundAndPackFloat32
L606:
	add	sp, sp, #16
	ldmfd	sp!, {r4, r5, r6, pc}^
	.align	0
	.global	_float32_sqrt
_float32_sqrt:
	@ args = 0, pretend = 0, frame = 20
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	bic	r2, r0, #-16777216
	bic	r2, r2, #8388608
	sub	sp, sp, #20
	str	r2, [sp, #12]
	mov	r3, r0, lsr #23
	and	r3, r3, #255
	str	r3, [sp, #16]
	cmp	r3, #255
	mov	r1, r0, lsr #31
	bne	L614
	cmp	r2, #0
	movne	r1, #0
	blne	_propagateFloat32NaN
	bne	L641
L615:
	cmp	r1, #0
	beq	L641
	b	L642
L614:
	cmp	r1, #0
	beq	L617
	orrs	r3, r3, r2
	beq	L641
L642:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #-2147483648
	mov	r0, r0, asr #9
	b	L641
L617:
	cmp	r3, #0
	bne	L619
	subs	r0, r2, #0
	beq	L641
	mov	r0, r2
	add	r1, sp, #16
	add	r2, sp, #12
	bl	_normalizeFloat32Subnormal
L619:
	ldr	r1, [sp, #12]
	orr	r1, r1, #8388608
	mov	r1, r1, asl #8
	str	r1, [sp, #12]
	ldr	r0, [sp, #16]
	sub	r3, r0, #127
	mov	r3, r3, asr #1
	add	r5, r3, #126
	bl	_estimateSqrt32
	add	r0, r0, #2
	str	r0, [sp, #0]
	and	r3, r0, #127
	cmp	r3, #5
	bhi	L621
	cmp	r0, #1
	mvnls	r3, #0
	bls	L643
L622:
	mov	r3, r0, asl #16
	mov	r3, r3, lsr #16
	mul	ip, r3, r3
	mov	r1, r0, lsr #16
	mul	r0, r1, r3
	add	r2, sp, #12
	ldmia	r2, {r2, r3}	@ phole ldm
	and	r3, r3, #1
	mov	r2, r2, lsr r3
	str	r2, [sp, #12]
	mul	r6, r1, r1
	add	r4, sp, #8
	add	lr, sp, #4
	mov	r1, r6
	mov	r3, r0
	mov	r0, r0, asl #1
	cmp	r0, r3
	mov	r2, r0, lsr #16
	addcc	r3, r1, #65536
	addcc	r1, r3, r2
L624:
	addcs	r1, r1, r2
L625:
	mov	r0, r0, asl #16
	adds	ip, ip, r0
	str	ip, [lr, #0]
	addcs	r1, r1, #1
	str	r1, [r4, #0]
	ldr	r3, [sp, #4]
	add	r6, sp, #12
	ldmda	r6, {r2, r6}
	sub	r2, r6, r2
	rsb	r0, r3, #0
	cmp	r3, #0
	moveq	ip, r2
	subne	ip, r2, #1
	cmp	ip, #0
	bge	L629
L630:
	ldr	r3, [sp, #0]
	sub	r3, r3, #1
	str	r3, [sp, #0]
	mov	r2, r3, asl #1
	str	r2, [sp, #4]
	mov	r3, r3, lsr #31
	str	r3, [sp, #8]
	orr	r2, r2, #1
	str	r2, [sp, #4]
	mov	r1, r0
	add	r0, r0, r2
	add	r3, ip, r3
	cmp	r0, r1
	movcs	ip, r3
	addcc	ip, r3, #1
	cmp	ip, #0
	blt	L630
L629:
	ldr	r3, [sp, #0]
	orrs	ip, ip, r0
	orrne	r3, r3, #1
L643:
	str	r3, [sp, #0]
L621:
	ldr	r3, [sp, #0]
	and	r2, r3, #1
	orr	r2, r2, r3, lsr #1
	str	r2, [sp, #0]
	mov	r0, #0
	mov	r1, r5
	bl	_roundAndPackFloat32
L641:
	add	sp, sp, #20
	ldmfd	sp!, {r4, r5, r6, pc}^
	.align	0
	.global	_float32_eq
_float32_eq:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, lr}
	mov	r3, r0, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	mov	r4, r1
	bne	L647
	bic	r3, r0, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	bne	L646
L647:
	mov	r3, r4, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	bne	L645
	bic	r3, r4, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	beq	L645
L646:
	bl	_float32_is_signaling_nan
	tst	r0, #255
	bne	L653
	mov	r0, r4
	bl	_float32_is_signaling_nan
	tst	r0, #255
	beq	L652
L653:
	mov	r0, #1
	bl	_float_raise
L652:
	mov	r0, #0
	ldmfd	sp!, {r4, pc}^
L645:
	mov	r2, #0
	cmp	r0, r4
	beq	L655
	orr	r3, r0, r4
	movs	r3, r3, asl #1
	bne	L654
L655:
	mov	r2, #1
L654:
	mov	r0, r2
	ldmfd	sp!, {r4, pc}^
	.align	0
	.global	_float32_le
_float32_le:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {lr}
	mov	r3, r0, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	bne	L660
	bic	r3, r0, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	bne	L659
L660:
	mov	r3, r1, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	bne	L658
	bic	r3, r1, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	beq	L658
L659:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #0
	ldmfd	sp!, {pc}^
L658:
	mov	r3, r0, lsr #31
	cmp	r3, r1, lsr #31
	beq	L667
	mov	r2, #0
	cmp	r3, r2
	bne	L669
	orr	r3, r0, r1
	movs	r3, r3, asl #1
	bne	L668
L669:
	mov	r2, #1
L668:
	mov	r0, r2
	ldmfd	sp!, {pc}^
L667:
	mov	r2, #0
	cmp	r0, r1
	beq	L671
	bcs	L672
	cmp	r3, r2
	beq	L671
	b	L670
L672:
	cmp	r3, #0
	beq	L670
L671:
	mov	r2, #1
L670:
	mov	r0, r2
	ldmfd	sp!, {pc}^
	.align	0
	.global	_float32_lt
_float32_lt:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {lr}
	mov	r2, r0
	mov	r3, r2, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	bne	L678
	bic	r3, r2, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	bne	L677
L678:
	mov	r3, r1, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	bne	L676
	bic	r3, r1, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	beq	L676
L677:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #0
	ldmfd	sp!, {pc}^
L676:
	mov	r3, r2, lsr #31
	cmp	r3, r1, lsr #31
	beq	L685
	mov	r0, #0
	cmp	r3, r0
	ldmeqfd	sp!, {pc}^
	orr	r3, r2, r1
	movs	r3, r3, asl #1
	moveq	r0, #0
	movne	r0, #1
	ldmfd	sp!, {pc}^
L685:
	mov	r0, #0
	cmp	r2, r1
	ldmeqfd	sp!, {pc}^
	bcs	L688
	cmp	r3, r0
	beq	L689
	ldmfd	sp!, {pc}^
L688:
	cmp	r3, #0
	ldmeqfd	sp!, {pc}^
L689:
	mov	r0, #1
	ldmfd	sp!, {pc}^
	.align	0
	.global	_float32_eq_signaling
_float32_eq_signaling:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {lr}
	mov	r3, r0, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	bne	L694
	bic	r3, r0, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	bne	L693
L694:
	mov	r3, r1, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	bne	L692
	bic	r3, r1, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	beq	L692
L693:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #0
	ldmfd	sp!, {pc}^
L692:
	mov	r2, #0
	cmp	r0, r1
	beq	L700
	orr	r3, r0, r1
	movs	r3, r3, asl #1
	bne	L699
L700:
	mov	r2, #1
L699:
	mov	r0, r2
	ldmfd	sp!, {pc}^
	.align	0
	.global	_float32_le_quiet
_float32_le_quiet:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, lr}
	mov	r3, r0, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	mov	r4, r1
	bne	L705
	bic	r3, r0, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	bne	L704
L705:
	mov	r3, r4, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	bne	L703
	bic	r3, r4, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	beq	L703
L704:
	bl	_float32_is_signaling_nan
	tst	r0, #255
	bne	L711
	mov	r0, r4
	bl	_float32_is_signaling_nan
	tst	r0, #255
	beq	L710
L711:
	mov	r0, #1
	bl	_float_raise
L710:
	mov	r0, #0
	ldmfd	sp!, {r4, pc}^
L703:
	mov	r3, r0, lsr #31
	cmp	r3, r4, lsr #31
	beq	L714
	mov	r2, #0
	cmp	r3, r2
	bne	L716
	orr	r3, r0, r4
	movs	r3, r3, asl #1
	bne	L715
L716:
	mov	r2, #1
L715:
	mov	r0, r2
	ldmfd	sp!, {r4, pc}^
L714:
	mov	r2, #0
	cmp	r0, r4
	beq	L718
	bcs	L719
	cmp	r3, r2
	beq	L718
	b	L717
L719:
	cmp	r3, #0
	beq	L717
L718:
	mov	r2, #1
L717:
	mov	r0, r2
	ldmfd	sp!, {r4, pc}^
	.align	0
	.global	_float32_lt_quiet
_float32_lt_quiet:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, lr}
	mov	r2, r0
	mov	r3, r2, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	mov	r4, r1
	bne	L725
	bic	r3, r2, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	bne	L724
L725:
	mov	r3, r4, lsr #23
	and	r3, r3, #255
	cmp	r3, #255
	bne	L723
	bic	r3, r4, #-16777216
	bic	r3, r3, #8388608
	cmp	r3, #0
	beq	L723
L724:
	mov	r0, r2
	bl	_float32_is_signaling_nan
	tst	r0, #255
	bne	L731
	mov	r0, r4
	bl	_float32_is_signaling_nan
	tst	r0, #255
	beq	L730
L731:
	mov	r0, #1
	bl	_float_raise
L730:
	mov	r0, #0
	ldmfd	sp!, {r4, pc}^
L723:
	mov	r3, r2, lsr #31
	cmp	r3, r4, lsr #31
	beq	L734
	mov	r0, #0
	cmp	r3, r0
	ldmeqfd	sp!, {r4, pc}^
	orr	r3, r2, r4
	movs	r3, r3, asl #1
	moveq	r0, #0
	movne	r0, #1
	ldmfd	sp!, {r4, pc}^
L734:
	mov	r0, #0
	cmp	r2, r4
	ldmeqfd	sp!, {r4, pc}^
	bcs	L737
	cmp	r3, r0
	beq	L738
	ldmfd	sp!, {r4, pc}^
L737:
	cmp	r3, #0
	ldmeqfd	sp!, {r4, pc}^
L738:
	mov	r0, #1
	ldmfd	sp!, {r4, pc}^
	.align	0
	.global	_float64_to_int32
_float64_to_int32:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	mov	ip, r1
	bic	r2, r0, #-16777216
	bic	r2, r2, #15728640
	mov	r3, r0, lsr #20
	mov	lr, r3, asl #21
	mov	lr, lr, lsr #21
	mov	r5, r0, lsr #31
	sub	r0, lr, #1040
	sub	r0, r0, #3
	cmp	r0, #0
	mov	r1, lr
	blt	L745
	cmp	r0, #11
	ble	L746
	mov	r3, #2032
	add	r3, r3, #15
	cmp	lr, r3
	bne	L773
	orrs	r2, r2, ip
	movne	r5, #0
	b	L773
L746:
	mov	r4, ip, asl r0
	cmp	r0, #0
	orr	r2, r2, #1048576
	beq	L749
	rsb	r3, r0, #0
	and	r3, r3, #31
	mov	r3, ip, lsr r3
	orr	r0, r3, r2, asl r0
	b	L751
L749:
	mov	r0, r2
L751:
	cmp	r0, #-1073741824
	bls	L753
L773:
	mov	r0, #-1073741824
	b	L753
L745:
	subs	ip, ip, #0
	movne	ip, #1
	mov	r3, #1020
	add	r3, r3, #1
	cmp	r1, r3
	orrle	r3, r1, r2
	orrle	r4, r3, ip
	movle	r0, #0
	ble	L753
L754:
	orr	r2, r2, #1048576
	and	r3, r0, #31
	orr	r4, ip, r2, asl r3
	rsb	r3, r0, #0
	mov	r0, r2, lsr r3
L753:
	ldr	r3, L774
	ldrb	r3, [r3, #0]	@ zero_extendqisi2
	mov	r3, r3, asl #24
	movs	r3, r3, asr #24
	bne	L756
	cmp	r4, #0
	bge	L757
	add	r0, r0, #1
	movs	r6, r4, asl #1
	bic	r3, r0, #1
	moveq	r0, r3
L757:
	cmp	r5, #0
	rsbne	r0, r0, #0
	b	L760
L756:
	subs	r4, r4, #0
	movne	r4, #1
	cmp	r5, #0
	beq	L761
	cmp	r3, #1
	addeq	r3, r0, r4
	rsbeq	r0, r3, #0
	beq	L760
L762:
	rsb	r0, r0, #0
	b	L760
L761:
	and	r2, r4, #1
	cmp	r3, #2
	movne	r3, #0
	moveq	r3, #1
	cmp	r2, #0
	addne	r0, r3, r0
L760:
	cmp	r0, #0
	bge	L767
	cmp	r5, #0
	beq	L768
	b	L766
L767:
	cmp	r5, #0
	beq	L766
L768:
	cmp	r0, #0
	beq	L766
	mov	r0, #1
	bl	_float_raise
	cmp	r5, #0
	mvneq	r0, #-2147483648
	movne	r0, #-2147483648
	ldmfd	sp!, {r4, r5, r6, pc}^
L766:
	cmp	r4, #0
	ldmeqfd	sp!, {r4, r5, r6, pc}^
	ldr	r2, L774+4
	ldrb	r3, [r2, #0]
	orr	r3, r3, #32
	strb	r3, [r2, #0]
	ldmfd	sp!, {r4, r5, r6, pc}^
L775:
	.align	0
L774:
	.word	_float_rounding_mode
	.word	_float_exception_flags
	.align	0
	.global	___fixdfsi
___fixdfsi:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	bic	ip, r0, #-16777216
	bic	ip, ip, #15728640
	mov	r3, r0, lsr #20
	mov	lr, r3, asl #21
	mov	lr, lr, lsr #21
	mov	r4, r0, lsr #31
	sub	r2, lr, #1040
	sub	r2, r2, #3
	cmp	r2, #0
	mov	r0, r1
	mov	r1, lr
	blt	L781
	cmp	r2, #11
	ble	L782
	mov	r3, #2032
	add	r3, r3, #15
	cmp	lr, r3
	bne	L783
	orrs	ip, ip, r0
	movne	r4, #0
L783:
	mov	r0, #-1073741824
	b	L788
L782:
	mov	r5, r0, asl r2
	cmp	r2, #0
	orr	r1, ip, #1048576
	beq	L785
	rsb	r3, r2, #0
	and	r3, r3, #31
	mov	r3, r0, lsr r3
	orr	r0, r3, r1, asl r2
	b	L788
L785:
	mov	r0, r1
	b	L788
L781:
	mov	r3, #1020
	add	r3, r3, #1
	cmp	r1, r3
	orrle	r3, r1, ip
	orrle	r5, r3, r0
	movle	r0, #0
	ble	L788
L789:
	orr	ip, ip, #1048576
	and	r3, r2, #31
	orr	r5, r0, ip, asl r3
	rsb	r3, r2, #0
	mov	r0, ip, lsr r3
L788:
	cmp	r4, #0
	rsbne	r0, r0, #0
	cmp	r0, #0
	bge	L793
	cmp	r4, #0
	beq	L794
	b	L792
L793:
	cmp	r4, #0
	beq	L792
L794:
	cmp	r0, #0
	beq	L792
	mov	r0, #1
	bl	_float_raise
	cmp	r4, #0
	mvneq	r0, #-2147483648
	movne	r0, #-2147483648
	ldmfd	sp!, {r4, r5, pc}^
L792:
	cmp	r5, #0
	ldmeqfd	sp!, {r4, r5, pc}^
	ldr	r2, L799
	ldrb	r3, [r2, #0]
	orr	r3, r3, #32
	strb	r3, [r2, #0]
	ldmfd	sp!, {r4, r5, pc}^
L800:
	.align	0
L799:
	.word	_float_exception_flags
	.align	0
	.global	___truncdfsf2
___truncdfsf2:
	@ args = 0, pretend = 0, frame = 20
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	mov	r2, r1
	mov	r1, r0
	bic	ip, r1, #-16777216
	bic	ip, ip, #15728640
	mov	r3, r1, lsr #20
	sub	sp, sp, #20
	mov	lr, r3, asl #21
	mov	lr, lr, lsr #21
	mov	r3, #2032
	add	r3, r3, #15
	cmp	lr, r3
	mov	r3, r2
	mov	r4, r1, lsr #31
	mov	r0, r4
	bne	L806
	orrs	ip, ip, r3
	beq	L807
	add	r4, sp, #8
	mov	r0, r4
	bl	_float64ToCommonNaN
	ldmia	r4, {r0-r2}	@ load multiple
	bl	_commonNaNToFloat32
	b	L819
L807:
	mov	r0, r0, asl #31
	orr	r0, r0, #2130706432
	orr	r0, r0, #8388608
	b	L819
L806:
	mov	r2, #10
	movs	r5, r3, asl r2
	mov	r3, r3, lsr #22
	orr	r3, r3, ip, asl r2
	moveq	r2, r3
	orrne	r2, r3, #1
	str	r2, [sp, #0]
	mov	r3, ip, lsr #22
	str	r3, [sp, #4]
	cmp	lr, #0
	orrne	r3, r2, #1073741824
	strne	r3, [sp, #0]
L818:
	ldr	r2, [sp, #0]
	mov	r0, r4
	sub	r1, lr, #896
	sub	r1, r1, #1
	bl	_roundAndPackFloat32
L819:
	add	sp, sp, #20
	ldmfd	sp!, {r4, r5, pc}^
	.align	0
	.global	_float64_round_to_int
_float64_round_to_int:
	@ args = 0, pretend = 0, frame = 64
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #68
	stmib	sp, {r1-r2}
	ldr	r4, [sp, #4]
	mov	r9, r0
	mov	lr, #1040
	mov	r3, r4, lsr #20
	mov	r0, r3, asl #21
	mov	r0, r0, lsr #21
	add	r3, lr, #2
	cmp	r0, r3
	mov	r2, r0
	ble	L822
	mov	r2, #1072
	add	r2, r2, #2
	cmp	r0, r2
	ble	L823
	mov	r3, #2032
	add	r3, r3, #15
	cmp	r0, r3
	bne	L824
	ldr	r2, [sp, #8]
	bic	r3, r4, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r3, r2
	beq	L824
	str	r2, [sp, #0]
	ldmib	sp, {r1-r2}
	add	r0, sp, #60
	mov	r3, r4
	bl	_propagateFloat64NaN
	add	r5, sp, #60
	ldmia	r5, {r4-r5}
	b	L873
L824:
	ldmib	sp, {r4-r5}
	b	L873
L823:
	ldmib	sp, {r4-r5}
	add	ip, sp, #52
	stmia	ip, {r4-r5}
	ldr	r3, L876
	ldrb	r1, [r3, #0]	@ zero_extendqisi2
	rsb	r2, r0, r2
	mov	r3, #2
	mov	r0, r3, asl r2
	sub	lr, r0, #1
	mov	r3, r1, asl #24
	movs	r3, r3, asr #24
	bne	L827
	cmp	r0, #0
	beq	L828
	add	r2, sp, #52
	ldmia	r2, {r2, r3}	@ phole ldm
	add	r1, r3, r0, lsr #1
	str	r1, [ip, #4]
	cmp	r1, r3
	addcc	r2, r2, #1
	str	r2, [sp, #52]
	ldr	r3, [sp, #56]
	tst	r3, lr
	biceq	r3, r3, r0
	streq	r3, [sp, #56]
	b	L834
L828:
	ldr	r2, [sp, #56]
	cmp	r2, #0
	bge	L834
	ldr	r3, [sp, #52]
	add	r3, r3, #1
	str	r3, [sp, #52]
	movs	r2, r2, asl #1
	bne	L834
	bic	r3, r3, #1
	b	L874
L827:
	cmp	r3, #3
	beq	L834
	ldr	r5, [sp, #4]
	cmp	r3, #2
	mov	r3, r5, lsr #31
	bne	L838
	cmp	r3, #0
	beq	L839
	b	L834
L838:
	cmp	r3, #0
	beq	L834
L839:
	ldr	r1, [sp, #56]
	ldr	r3, [sp, #52]
	add	r2, r1, lr
	str	r2, [sp, #56]
	cmp	r2, r1
	addcc	r3, r3, #1
L874:
	str	r3, [sp, #52]
L834:
	ldr	r3, [sp, #56]
	bic	r3, r3, lr
	str	r3, [sp, #56]
	b	L841
L822:
	mov	ip, #1020
	add	ip, ip, #2
	cmp	r0, ip
	bgt	L842
	ldr	r4, [sp, #8]
	ldr	r5, [sp, #4]
	orrs	r4, r4, r5, asl #1
	ldmeqib	sp, {r4-r5}
	beq	L873
L843:
	ldr	r2, L876+4
	ldrb	r3, [r2, #0]
	orr	r3, r3, #32
	strb	r3, [r2, #0]
	ldr	r5, [sp, #4]
	ldr	r3, L876
	ldrb	r3, [r3, #0]
	mov	r2, r5, lsr #31
	mov	r3, r3, asl #24
	mov	r1, r3, asr #24
	cmp	r1, #1
	beq	L851
	bgt	L861
	cmp	r1, #0
	beq	L846
	b	L845
L861:
	cmp	r1, #2
	beq	L855
	b	L845
L846:
	cmp	r0, ip
	bne	L845
	ldmib	sp, {r4, r5}	@ phole ldm
	bic	r3, r4, #-16777216
	bic	r3, r3, #15728640
	orrs	r5, r5, r3
	beq	L845
	str	r1, [sp, #48]
	mov	r3, r2, asl #31
	orr	r3, r3, #1069547520
	str	r3, [sp, #44]
	mov	r4, r3
	orr	r4, r4, #3145728
	str	r4, [sp, #44]
	add	r5, sp, #44
	ldmia	r5, {r4-r5}
	b	L873
L851:
	cmp	r2, #0
	beq	L852
	mov	r4, #0
	str	r4, [sp, #40]
	mov	r5, #-1090519040
	str	r5, [sp, #36]
	mov	r4, r5
	add	r4, r4, #15728640
	str	r4, [sp, #36]
	add	r5, sp, #36
	ldmia	r5, {r4-r5}
	b	L873
L852:
	mov	r8, r2
	mov	r7, r2
	stmia	r9, {r7-r8}
	b	L820
L855:
	cmp	r2, #0
	beq	L856
	mov	r4, #0
	str	r4, [sp, #32]
	mov	r5, #-2147483648
	str	r5, [sp, #28]
	add	r5, sp, #28
	ldmia	r5, {r4-r5}
	b	L873
L856:
	str	r2, [sp, #24]
	mov	r4, #1069547520
	str	r4, [sp, #20]
	mov	r5, r4
	add	r5, r5, #3145728
	str	r5, [sp, #20]
	add	r5, sp, #20
	ldmia	r5, {r4-r5}
	b	L873
L845:
	mov	r4, #0
	str	r4, [sp, #16]
	mov	r2, r2, asl #31
	str	r2, [sp, #12]
	add	r5, sp, #12
	ldmia	r5, {r4-r5}
	b	L873
L842:
	mov	r3, #0
	str	r3, [sp, #56]
	ldr	r4, [sp, #4]
	str	r4, [sp, #52]
	ldr	r3, L876
	ldrb	r1, [r3, #0]	@ zero_extendqisi2
	mov	r0, #1
	add	r7, sp, #52
	mov	ip, r4
	add	r3, lr, #3
	rsb	r3, r2, r3
	mov	r0, r0, asl r3
	sub	lr, r0, #1
	mov	r3, r1, asl #24
	movs	r2, r3, asr #24
	bne	L863
	add	r2, ip, r0, lsr #1
	str	r2, [sp, #52]
	ldr	r5, [sp, #8]
	and	r3, r2, lr
	orrs	r5, r5, r3
	bne	L865
	bic	r3, r2, r0
	b	L875
L863:
	cmp	r2, #3
	beq	L865
	ldmia	r7, {r3-r4}
	cmp	r2, #2
	mov	r3, r3, lsr #31
	bne	L869
	cmp	r3, #0
	beq	L870
	b	L865
L877:
	.align	0
L876:
	.word	_float_rounding_mode
	.word	_float_exception_flags
L869:
	cmp	r3, #0
	beq	L865
L870:
	ldr	r3, [sp, #52]
	ldr	r4, [sp, #8]
	cmp	r4, #0
	orrne	r3, r3, #1
	add	r3, r3, lr
L875:
	str	r3, [sp, #52]
L865:
	ldr	r3, [sp, #52]
	bic	r3, r3, lr
	str	r3, [sp, #52]
L841:
	ldr	r3, [sp, #56]
	ldr	r5, [sp, #8]
	cmp	r3, r5
	bne	L872
	ldr	r3, [sp, #52]
	ldr	r4, [sp, #4]
	cmp	r3, r4
	beq	L871
L872:
	ldr	r2, L879
	ldrb	r3, [r2, #0]
	orr	r3, r3, #32
	strb	r3, [r2, #0]
L871:
	add	r3, sp, #52
	ldmia	r3, {r4-r5}
L873:
	stmia	r9, {r4-r5}
L820:
	mov	r0, r9
	b	L878
L880:
	.align	0
L879:
	.word	_float_exception_flags
L878:
	add	sp, sp, #68
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, pc}^
	.align	0
_addFloat64Sigs:
	@ args = 12, pretend = 4, frame = 100
	@ frame_needed = 0, current_function_anonymous_args = 0
	sub	sp, sp, #4
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #108
	str	r0, [sp, #44]
	str	r3, [sp, #136]
	add	r7, sp, #136
	ldmia	r7, {r4-r5}
	mov	r3, r1, lsr #20
	mov	ip, r3, asl #21
	mov	r3, r4, lsr #20
	mov	r3, r3, asl #21
	str	r3, [sp, #32]
	ldrb	r8, [sp, #144]	@ zero_extendqisi2
	str	r8, [sp, #40]
	str	r2, [sp, #72]
	bic	r9, r1, #-16777216
	bic	r9, r9, #15728640
	str	r9, [sp, #28]
	str	r9, [sp, #76]
	str	r5, [sp, #92]
	bic	lr, r4, #-16777216
	bic	lr, lr, #15728640
	str	lr, [sp, #96]
	mov	ip, ip, lsr #21
	str	ip, [sp, #36]
	mov	r7, r3
	mov	r7, r7, lsr #21
	str	r7, [sp, #32]
	rsb	r0, r7, ip
	cmp	r0, #0
	ble	L888
	mov	r3, #2032
	add	r3, r3, #15
	cmp	ip, r3
	bne	L889
	orrs	r9, r2, r9
	beq	L890
	str	r5, [sp, #0]
	mov	r3, r4
	add	r0, sp, #100
	bl	_propagateFloat64NaN
	add	r8, sp, #100
	ldmia	r8, {r8-r9}
	b	L943
L890:
	ldr	r9, [sp, #44]
	stmia	r9, {r1-r2}
	b	L881
L889:
	ldr	r7, [sp, #32]
	cmp	r7, #0
	subeq	r0, r0, #1
L891:
	orrne	r3, lr, #1048576
	strne	r3, [sp, #96]
L892:
	add	r8, sp, #96
	str	r8, [sp, #16]
	ldr	ip, [sp, #96]
	ldr	lr, [sp, #92]
	mov	r4, #0
	cmp	r0, r4
	add	r6, sp, #92
	add	r5, sp, #88
	rsb	r3, r0, #0
	and	r2, r3, #31
	moveq	r1, r0
	moveq	r2, lr
	moveq	r3, ip
	beq	L894
L893:
	cmp	r0, #31
	bgt	L895
	mov	r1, lr, asl r2
	mov	r3, lr, lsr r0
	orr	r2, r3, ip, asl r2
	mov	r3, ip, lsr r0
	b	L896
L895:
	cmp	r0, #32
	moveq	r1, lr
	moveq	r2, ip
	beq	L898
L897:
	mov	r4, lr
	cmp	r0, #63
	movle	r1, ip, asl r2
	andle	r3, r0, #31
	movle	r2, ip, lsr r3
	ble	L898
L899:
	cmp	r0, #64
	beq	L901
	subs	r1, ip, #0
	movne	r1, #1
	b	L902
L901:
	mov	r1, ip
L902:
	mov	r2, #0
L898:
	mov	r3, #0
L896:
	cmp	r4, #0
	orrne	r1, r1, #1
L894:
	str	r1, [r5, #0]
	str	r2, [r6, #0]
	ldr	r9, [sp, #16]
	str	r3, [r9, #0]
	ldr	ip, [sp, #36]
	b	L904
L888:
	bge	L905
	ldr	r7, [sp, #32]
	mov	r3, #2032
	add	r3, r3, #15
	cmp	r7, r3
	bne	L906
	mov	r0, r5
	orrs	r3, lr, r0
	beq	L907
	str	r0, [sp, #0]
	mov	r3, r4
	add	r4, sp, #80
	mov	r0, r4
	bl	_propagateFloat64NaN
	b	L944
L907:
	str	r3, [sp, #12]
	ldr	r8, [sp, #40]
	mov	r3, r8, asl #31
	orr	r3, r3, #2130706432
	str	r3, [sp, #8]
	mov	r9, r3
	orr	r9, r9, #15728640
	str	r9, [sp, #8]
	ldr	r7, [sp, #44]
	add	r8, sp, #8
	ldmia	r8, {r8-r9}
	b	L945
L906:
	cmp	ip, #0
	addeq	r0, r0, #1
L909:
	ldrne	r9, [sp, #28]
	orrne	r3, r9, #1048576
	strne	r3, [sp, #76]
L910:
	add	r3, sp, #72
	ldmia	r3, {r3, ip}	@ phole ldm
	mov	lr, #0
	rsb	r1, r0, #0
	cmp	r1, lr
	add	r6, sp, #76
	add	r5, sp, #72
	add	r4, sp, #88
	and	r2, r0, #31
	moveq	r0, lr
	moveq	r2, r3
	moveq	r3, ip
	beq	L912
L911:
	cmp	r1, #31
	bgt	L913
	mov	r0, r3, asl r2
	mov	r3, r3, lsr r1
	orr	r2, r3, ip, asl r2
	mov	r3, ip, lsr r1
	b	L914
L913:
	cmp	r1, #32
	moveq	r0, r3
	moveq	r2, ip
	beq	L916
L915:
	mov	lr, r3
	cmp	r1, #63
	movle	r0, ip, asl r2
	andle	r3, r1, #31
	movle	r2, ip, lsr r3
	ble	L916
L917:
	cmp	r1, #64
	beq	L919
	subs	r0, ip, #0
	movne	r0, #1
	b	L920
L919:
	mov	r0, ip
L920:
	mov	r2, #0
L916:
	mov	r3, #0
L914:
	cmp	lr, #0
	orrne	r0, r0, #1
L912:
	str	r0, [r4, #0]
	str	r2, [r5, #0]
	str	r3, [r6, #0]
	ldr	ip, [sp, #32]
	b	L904
L905:
	mov	r3, #2032
	add	r3, r3, #15
	cmp	ip, r3
	bne	L923
	ldr	r7, [sp, #28]
	mov	r0, r5
	orr	r3, r2, r7
	orr	r3, r3, lr
	orrs	r3, r3, r0
	beq	L924
	str	r0, [sp, #0]
	mov	r3, r4
	add	r4, sp, #64
	mov	r0, r4
	bl	_propagateFloat64NaN
	b	L944
L924:
	ldr	r8, [sp, #44]
	stmia	r8, {r1-r2}
	b	L881
L923:
	ldr	r9, [sp, #28]
	add	r1, r5, r2
	str	r1, [sp, #56]
	add	r3, r9, lr
	cmp	r1, r2
	movcs	r2, r3
	addcc	r2, r3, #1
	str	r2, [sp, #60]
	cmp	ip, #0
	bne	L926
	str	r1, [sp, #24]
	ldr	r7, [sp, #40]
	orr	r2, r2, r7, asl #31
	str	r2, [sp, #20]
	ldr	r7, [sp, #44]
	add	r8, sp, #20
	ldmia	r8, {r8-r9}
	b	L945
L926:
	mov	r3, #0
	str	r3, [sp, #88]
	orr	r3, r2, #2097152
	str	r3, [sp, #60]
	ldr	ip, [sp, #36]
	b	L928
L904:
	ldr	r3, [sp, #76]
	orr	r3, r3, #1048576
	str	r3, [sp, #76]
	ldr	r0, [sp, #72]
	ldr	r2, [sp, #92]
	ldr	r1, [sp, #96]
	add	r2, r0, r2
	str	r2, [sp, #56]
	add	r3, r3, r1
	cmp	r2, r0
	addcc	r3, r3, #1
	str	r3, [sp, #60]
	sub	ip, ip, #1
	mvn	r2, #-16777216
	sub	r2, r2, #14680064
	cmp	r3, r2
	bls	L931
	add	ip, ip, #1
L928:
	ldr	r3, [sp, #88]
	ldr	r2, [sp, #56]
	mov	r1, #31
	subs	r3, r3, #0
	movne	r3, #1
	orr	r3, r3, r2, asl r1
	str	r3, [sp, #88]
	ldr	r3, [sp, #60]
	mov	r2, r2, lsr #1
	orr	r2, r2, r3, asl r1
	str	r2, [sp, #56]
	mov	r3, r3, lsr #1
	str	r3, [sp, #60]
L931:
	add	r2, sp, #56
	ldmia	r2, {r2, r3}	@ phole ldm
	str	r2, [sp, #0]
	ldr	r2, [sp, #88]
	str	r2, [sp, #4]
	ldr	r1, [sp, #40]
	add	r4, sp, #48
	mov	r0, r4
	mov	r2, ip
	bl	_roundAndPackFloat64
L944:
	ldmia	r4, {r8-r9}
L943:
	ldr	r7, [sp, #44]
L945:
	stmia	r7, {r8-r9}
L881:
	ldr	r0, [sp, #44]
	add	sp, sp, #108
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	add	sp, sp, #4
	movs	pc, lr
	.align	0
_subFloat64Sigs:
	@ args = 12, pretend = 4, frame = 88
	@ frame_needed = 0, current_function_anonymous_args = 0
	sub	sp, sp, #4
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #92
	str	r0, [sp, #40]
	str	r3, [sp, #120]
	ldrb	r7, [sp, #128]	@ zero_extendqisi2
	str	r7, [sp, #36]
	mov	r6, r2
	mov	r5, r1
	bic	r2, r5, #-16777216
	bic	r2, r2, #15728640
	str	r2, [sp, #72]
	ldr	r8, [sp, #124]
	str	r8, [sp, #52]
	str	r6, [sp, #68]
	mov	r9, r3
	bic	r1, r9, #-16777216
	bic	r1, r1, #15728640
	str	r1, [sp, #56]
	mov	r3, r6
	mov	r4, r3, asl #10
	mov	r3, r3, lsr #22
	orr	r0, r3, r2, asl #10
	mov	r7, r8
	mov	r3, r7, lsr #22
	orr	r2, r3, r1, asl #10
	mov	r3, r5, lsr #20
	mov	ip, r3, asl #21
	mov	r8, r9
	mov	r3, r8, lsr #20
	mov	r3, r3, asl #21
	str	r3, [sp, #32]
	str	r4, [sp, #68]
	str	r0, [sp, #72]
	str	r7, [sp, #4]
	str	r2, [sp, #56]
	mov	lr, r7, asl #10
	str	lr, [sp, #52]
	mov	r9, r3
	mov	r9, r9, lsr #21
	str	r9, [sp, #32]
	mov	ip, ip, lsr #21
	rsb	r1, r9, ip
	cmp	r1, #0
	bgt	L960
	blt	L962
	mov	r3, #2032
	add	r3, r3, #15
	cmp	ip, r3
	bne	L963
	orr	r3, r0, r4
	orr	r3, r3, r2
	orrs	r4, r3, lr
	beq	L964
	str	r7, [sp, #0]
	mov	r3, r8
	add	r0, sp, #84
	mov	r2, r6
	mov	r1, r5
	bl	_propagateFloat64NaN
	add	r7, sp, #84
	ldmia	r7, {r7-r8}
	b	L1003
L964:
	mov	r0, #1
	bl	_float_raise
	str	r4, [sp, #28]
	mov	r8, #-2147483648
	str	r8, [sp, #24]
	mov	r9, r8
	mov	r9, r9, asr #12
	str	r9, [sp, #24]
	ldr	r9, [sp, #40]
	add	r7, sp, #24
	ldmia	r7, {r7-r8}
	b	L1004
L963:
	cmp	ip, #0
	moveq	ip, #1
	streq	ip, [sp, #32]
L965:
	cmp	r2, r0
	bcc	L967
	cmp	r0, r2
	bcc	L969
	cmp	lr, r4
	bcc	L967
	cmp	r4, lr
	bcc	L969
	ldr	r3, L1007
	ldrb	r3, [r3, #0]
	mov	r8, #0
	str	r8, [sp, #20]
	cmp	r3, #1
	movne	r3, #0
	moveq	r3, #1
	mov	r3, r3, asl #31
	str	r3, [sp, #16]
	ldr	r9, [sp, #40]
	add	r7, sp, #16
	ldmia	r7, {r7-r8}
	b	L1004
L962:
	ldr	r8, [sp, #32]
	mov	r3, #2032
	add	r3, r3, #15
	cmp	r8, r3
	bne	L973
	orrs	r3, r2, lr
	beq	L974
	ldr	r9, [sp, #4]
	str	r9, [sp, #0]
	ldr	r3, [sp, #120]
	add	r4, sp, #76
	mov	r0, r4
	mov	r2, r6
	mov	r1, r5
	bl	_propagateFloat64NaN
	b	L1005
L974:
	str	r3, [sp, #12]
	ldr	r7, [sp, #36]
	eor	r3, r7, #1
	mov	r3, r3, asl #31
	orr	r3, r3, #2130706432
	str	r3, [sp, #8]
	mov	r8, r3
	orr	r8, r8, #15728640
	str	r8, [sp, #8]
	ldr	r9, [sp, #40]
	add	r7, sp, #8
	ldmia	r7, {r7-r8}
	b	L1004
L973:
	cmp	ip, #0
	addeq	r1, r1, #1
L976:
	orrne	r3, r0, #1073741824
	strne	r3, [sp, #72]
L977:
	add	r2, sp, #68
	ldmia	r2, {r2, ip}	@ phole ldm
	rsb	r0, r1, #0
	cmp	r0, #0
	add	r4, sp, #72
	add	lr, sp, #68
	and	r3, r1, #31
	moveq	r3, ip
	beq	L979
L978:
	cmp	r0, #31
	bgt	L980
	movs	r8, r2, asl r3
	mov	r2, r2, lsr r0
	orr	r2, r2, ip, asl r3
	orrne	r2, r2, #1
	mov	r3, ip, lsr r0
	b	L979
L980:
	cmp	r0, #32
	bne	L982
	cmp	r2, #0
	moveq	r2, ip
	orrne	r2, ip, #1
	b	L983
L982:
	cmp	r0, #63
	bgt	L984
	orrs	r3, r2, ip, asl r3
	and	r2, r0, #31
	moveq	r3, #0
	movne	r3, #1
	orr	r2, r3, ip, lsr r2
	b	L983
L984:
	orrs	ip, ip, r2
	moveq	r2, #0
	movne	r2, #1
L983:
	mov	r3, #0
L979:
	str	r2, [lr, #0]
	str	r3, [r4, #0]
	ldr	r3, [sp, #56]
	orr	r3, r3, #1073741824
	str	r3, [sp, #56]
L969:
	ldr	r9, [sp, #36]
	eor	r9, r9, #1
	str	r9, [sp, #36]
	add	r1, sp, #52
	ldmia	r1, {r1, r3}	@ phole ldm
	ldr	r0, [sp, #72]
	ldr	r2, [sp, #68]
	ldr	ip, [sp, #32]
	b	L1006
L960:
	mov	r3, #2032
	add	r3, r3, #15
	cmp	ip, r3
	bne	L989
	orrs	r0, r0, r4
	beq	L990
	ldr	r7, [sp, #4]
	str	r7, [sp, #0]
	ldr	r3, [sp, #120]
	add	r4, sp, #60
	mov	r0, r4
	mov	r2, r6
	mov	r1, r5
	bl	_propagateFloat64NaN
	b	L1005
L990:
	ldr	r7, [sp, #40]
	stmia	r7, {r5-r6}
	b	L946
L989:
	ldr	r8, [sp, #32]
	cmp	r8, #0
	subeq	r1, r1, #1
L991:
	orrne	r3, r2, #1073741824
	strne	r3, [sp, #56]
L992:
	ldr	r0, [sp, #56]
	ldr	r2, [sp, #52]
	cmp	r1, #0
	add	r4, sp, #56
	add	lr, sp, #52
	rsb	r3, r1, #0
	and	r3, r3, #31
	moveq	r3, r0
	beq	L994
L993:
	cmp	r1, #31
	bgt	L995
	movs	r9, r2, asl r3
	mov	r2, r2, lsr r1
	orr	r2, r2, r0, asl r3
	orrne	r2, r2, #1
	mov	r3, r0, lsr r1
	b	L994
L995:
	cmp	r1, #32
	bne	L997
	cmp	r2, #0
	moveq	r2, r0
	orrne	r2, r0, #1
	b	L998
L997:
	cmp	r1, #63
	bgt	L999
	orrs	r3, r2, r0, asl r3
	and	r2, r1, #31
	moveq	r3, #0
	movne	r3, #1
	orr	r2, r3, r0, lsr r2
	b	L998
L1008:
	.align	0
L1007:
	.word	_float_rounding_mode
L999:
	orrs	r0, r0, r2
	moveq	r2, #0
	movne	r2, #1
L998:
	mov	r3, #0
L994:
	str	r2, [lr, #0]
	str	r3, [r4, #0]
	ldr	r3, [sp, #72]
	orr	r3, r3, #1073741824
	str	r3, [sp, #72]
L967:
	add	r1, sp, #68
	ldmia	r1, {r1, r3}	@ phole ldm
	ldr	r2, [sp, #52]
	ldr	r0, [sp, #56]
L1006:
	rsb	lr, r2, r1
	rsb	r3, r0, r3
	cmp	r1, r2
	subcc	r3, r3, #1
	str	lr, [sp, #0]
	ldr	r1, [sp, #36]
	add	r4, sp, #44
	mov	r0, r4
	sub	r2, ip, #11
	bl	_normalizeRoundAndPackFloat64
L1005:
	ldmia	r4, {r7-r8}
L1003:
	ldr	r9, [sp, #40]
L1004:
	stmia	r9, {r7-r8}
L946:
	ldr	r0, [sp, #40]
	add	sp, sp, #92
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	add	sp, sp, #4
	movs	pc, lr
	.align	0
	.global	___adddf3
___adddf3:
	@ args = 8, pretend = 4, frame = 16
	@ frame_needed = 0, current_function_anonymous_args = 0
	sub	sp, sp, #4
	stmfd	sp!, {r4, r5, r6, r7, lr}
	sub	sp, sp, #24
	str	r3, [sp, #44]
	add	r6, sp, #44
	ldmia	r6, {r3-r4}
	mov	r5, r0
	mov	r0, r1, lsr #31
	cmp	r0, r3, lsr #31
	bne	L1012
	str	r0, [sp, #4]
	str	r4, [sp, #0]
	add	r0, sp, #16
	bl	_addFloat64Sigs
	add	r7, sp, #16
	ldmia	r7, {r6-r7}
	b	L1014
L1012:
	str	r4, [sp, #0]
	str	r0, [sp, #4]
	add	r4, sp, #8
	mov	r0, r4
	bl	_subFloat64Sigs
	ldmia	r4, {r6-r7}
L1014:
	stmia	r5, {r6-r7}
	mov	r0, r5
	add	sp, sp, #24
	ldmfd	sp!, {r4, r5, r6, r7, lr}
	add	sp, sp, #4
	movs	pc, lr
	.align	0
	.global	___subdf3
___subdf3:
	@ args = 8, pretend = 4, frame = 16
	@ frame_needed = 0, current_function_anonymous_args = 0
	sub	sp, sp, #4
	stmfd	sp!, {r4, r5, r6, r7, lr}
	sub	sp, sp, #24
	str	r3, [sp, #44]
	add	r6, sp, #44
	ldmia	r6, {r3-r4}
	mov	r5, r0
	mov	r0, r1, lsr #31
	cmp	r0, r3, lsr #31
	bne	L1018
	str	r0, [sp, #4]
	str	r4, [sp, #0]
	add	r0, sp, #16
	bl	_subFloat64Sigs
	add	r7, sp, #16
	ldmia	r7, {r6-r7}
	b	L1020
L1018:
	str	r4, [sp, #0]
	str	r0, [sp, #4]
	add	r4, sp, #8
	mov	r0, r4
	bl	_addFloat64Sigs
	ldmia	r4, {r6-r7}
L1020:
	stmia	r5, {r6-r7}
	mov	r0, r5
	add	sp, sp, #24
	ldmfd	sp!, {r4, r5, r6, r7, lr}
	add	sp, sp, #4
	movs	pc, lr
	.align	0
	.global	___muldf3
___muldf3:
	@ args = 8, pretend = 4, frame = 184
	@ frame_needed = 0, current_function_anonymous_args = 0
	sub	sp, sp, #4
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #192
	str	r0, [sp, #100]
	str	r3, [sp, #220]
	mov	r6, r2
	mov	r5, r1
	add	r7, sp, #220
	ldmia	r7, {r0-r1}
	str	r6, [sp, #164]
	bic	r8, r5, #-16777216
	bic	r8, r8, #15728640
	str	r8, [sp, #80]
	str	r8, [sp, #168]
	mov	r3, r5, lsr #20
	mov	r4, r3, asl #21
	mov	r4, r4, lsr #21
	str	r4, [sp, #172]
	str	r1, [sp, #152]
	bic	r9, r0, #-16777216
	bic	r9, r9, #15728640
	str	r9, [sp, #12]
	str	r9, [sp, #156]
	mov	r3, r0, lsr #20
	mov	ip, r3, asl #21
	mov	ip, ip, lsr #21
	str	ip, [sp, #160]
	mov	r7, #2032
	add	r7, r7, #15
	str	r7, [sp, #76]
	mov	r2, r5, lsr #31
	mov	r3, r0, lsr #31
	eor	r2, r2, r3
	str	r2, [sp, #96]
	mov	lr, r4
	cmp	lr, r7
	bne	L1030
	orrs	r8, r6, r8
	bne	L1032
	cmp	ip, lr
	bne	L1031
	orrs	r9, r1, r9
	beq	L1031
L1032:
	str	r1, [sp, #0]
	mov	r3, r0
	add	r0, sp, #184
	mov	r2, r6
	mov	r1, r5
	bl	_propagateFloat64NaN
	add	r8, sp, #184
	ldmia	r8, {r8-r9}
	b	L1079
L1031:
	add	r9, sp, #160
	ldmda	r9, {r2, r9}
	orr	r2, r9, r2
	ldr	r3, [sp, #152]
	orrs	r2, r2, r3
	beq	L1034
	mov	r7, #0
	str	r7, [sp, #72]
	ldr	r8, [sp, #96]
	mov	r3, r8, asl #31
	orr	r3, r3, #2130706432
	str	r3, [sp, #68]
	mov	r9, r3
	orr	r9, r9, #15728640
	str	r9, [sp, #68]
	ldr	r7, [sp, #100]
	add	r8, sp, #68
	ldmia	r8, {r8-r9}
	b	L1080
L1030:
	ldr	r9, [sp, #76]
	cmp	ip, r9
	bne	L1036
	ldr	r7, [sp, #12]
	mov	r3, r1
	orrs	r2, r7, r3
	beq	L1037
	str	r3, [sp, #0]
	add	r4, sp, #176
	mov	r3, r0
	mov	r0, r4
	mov	r2, r6
	mov	r1, r5
	bl	_propagateFloat64NaN
	b	L1081
L1037:
	ldr	r8, [sp, #80]
	orr	r3, lr, r8
	orrs	r5, r6, r3
	bne	L1038
L1034:
	mov	r0, #1
	bl	_float_raise
	mov	r9, #0
	str	r9, [sp, #88]
	mov	r7, #-2147483648
	str	r7, [sp, #84]
	mov	r8, r7
	mov	r8, r8, asr #12
	str	r8, [sp, #84]
	ldr	r7, [sp, #100]
	add	r8, sp, #84
	ldmia	r8, {r8-r9}
	b	L1080
L1038:
	str	r2, [sp, #64]
	ldr	r9, [sp, #96]
	mov	r3, r9, asl #31
	orr	r3, r3, #2130706432
	str	r3, [sp, #60]
	mov	r7, r3
	orr	r7, r7, #15728640
	str	r7, [sp, #60]
	ldr	r7, [sp, #100]
	add	r8, sp, #60
	ldmia	r8, {r8-r9}
	b	L1080
L1036:
	cmp	r4, #0
	bne	L1040
	ldr	r9, [sp, #80]
	mov	r1, r6
	orrs	r3, r9, r1
	bne	L1041
	str	r3, [sp, #20]
	ldr	r7, [sp, #96]
	mov	r7, r7, asl #31
	str	r7, [sp, #16]
	ldr	r7, [sp, #100]
	add	r8, sp, #16
	ldmia	r8, {r8-r9}
	b	L1080
L1041:
	add	r3, sp, #164
	str	r3, [sp, #0]
	ldr	r0, [sp, #80]
	add	r2, sp, #172
	add	r3, sp, #168
	bl	_normalizeFloat64Subnormal
L1040:
	ldr	r3, [sp, #160]
	cmp	r3, #0
	bne	L1043
	ldr	r0, [sp, #156]
	ldr	r1, [sp, #152]
	orrs	r3, r0, r1
	bne	L1044
	str	r3, [sp, #56]
	ldr	r9, [sp, #96]
	mov	r9, r9, asl #31
	str	r9, [sp, #52]
	ldr	r7, [sp, #100]
	add	r8, sp, #52
	ldmia	r8, {r8-r9}
	b	L1080
L1044:
	add	r3, sp, #152
	str	r3, [sp, #0]
	add	r2, sp, #160
	add	r3, sp, #156
	bl	_normalizeFloat64Subnormal
L1043:
	ldr	r1, [sp, #152]
	mov	r3, r1, asl #12
	str	r3, [sp, #48]
	ldr	r9, [sp, #164]
	mov	ip, r9, asl #16
	mov	ip, ip, lsr #16
	mov	r2, r1, asl #28
	mov	r2, r2, lsr #16
	mul	r7, r2, ip
	mov	lr, r3, lsr #16
	mov	r3, r9, lsr #16
	mul	r5, lr, r3
	str	r9, [sp, #16]
	ldr	r8, [sp, #48]
	str	r8, [sp, #152]
	str	r7, [sp, #8]
	mul	r4, r2, r3
	add	r9, sp, #148
	str	r9, [sp, #44]
	add	r8, sp, #140
	str	r8, [sp, #36]
	add	r7, sp, #144
	str	r7, [sp, #40]
	ldr	r0, [sp, #168]
	orr	r0, r0, #1048576
	str	r0, [sp, #168]
	str	r0, [sp, #12]
	ldr	r3, [sp, #156]
	ldr	r2, [sp, #160]
	mov	r1, r1, lsr #20
	orr	r1, r1, r3, asl #12
	ldr	r3, [sp, #172]
	str	r1, [sp, #156]
	add	r3, r3, r2
	sub	r3, r3, #1024
	str	r3, [sp, #92]
	add	r9, sp, #136
	str	r9, [sp, #32]
	add	r8, sp, #128
	str	r8, [sp, #24]
	add	r7, sp, #132
	str	r7, [sp, #28]
	mla	ip, lr, ip, r4
	cmp	ip, r4
	mov	lr, r1
	mov	r2, ip, lsr #16
	addcc	r3, r5, #65536
	addcc	r5, r3, r2
L1049:
	addcs	r5, r5, r2
L1050:
	ldr	r7, [sp, #8]
	mov	ip, ip, asl #16
	adds	r7, r7, ip
	str	r7, [sp, #8]
	ldr	r9, [sp, #16]
	mov	r3, lr, asl #16
	mov	r3, r3, lsr #16
	mov	r2, r9, asl #16
	mov	r2, r2, lsr #16
	mul	r6, r3, r2
	ldr	r8, [sp, #24]
	mov	r1, r9, lsr #16
	str	r7, [r8, #0]
	mov	r0, lr, lsr #16
	mul	r4, r0, r1
	ldr	r9, [sp, #28]
	addcs	r5, r5, #1
	str	r5, [r9, #0]
	mul	r3, r1, r3
	add	r7, sp, #124
	str	r7, [sp, #16]
	mla	r2, r0, r2, r3
	add	ip, sp, #120
	cmp	r2, r3
	mov	r1, r2, lsr #16
	addcc	r3, r4, #65536
	addcc	r4, r3, r1
L1052:
	addcs	r4, r4, r1
L1053:
	ldr	r8, [sp, #12]
	mov	r3, lr, asl #16
	mov	r3, r3, lsr #16
	mov	r0, r8, asl #16
	mov	r0, r0, lsr #16
	mul	r9, r3, r0
	str	r9, [sp, #8]
	mov	r2, r2, asl #16
	adds	r6, r6, r2
	str	r6, [ip, #0]
	mov	lr, lr, lsr #16
	mov	r2, r8, lsr #16
	mul	r5, lr, r2
	ldr	r7, [sp, #16]
	addcs	r4, r4, #1
	str	r4, [r7, #0]
	mul	ip, r3, r2
	ldr	r1, [sp, #120]
	ldr	r2, [sp, #132]
	ldr	r3, [sp, #124]
	add	r2, r1, r2
	str	r2, [sp, #132]
	cmp	r2, r1
	addcc	r3, r3, #1
	str	r3, [sp, #124]
	add	r8, sp, #116
	str	r8, [sp, #16]
	mla	r1, lr, r0, ip
	cmp	r1, ip
	add	ip, sp, #112
	mov	r2, r1, lsr #16
	addcc	r3, r5, #65536
	addcc	r5, r3, r2
L1056:
	addcs	r5, r5, r2
L1057:
	ldr	r8, [sp, #8]
	mov	r1, r1, asl #16
	adds	r8, r8, r1
	str	r8, [sp, #8]
	ldr	r9, [sp, #12]
	ldr	r7, [sp, #48]
	str	r8, [ip, #0]
	mov	r0, r9, asl #16
	mov	r0, r0, lsr #16
	and	r2, r7, #61440
	mul	r6, r2, r0
	mov	r3, r9, lsr #16
	ldr	r9, [sp, #16]
	addcs	r5, r5, #1
	mov	lr, r7, lsr #16
	str	r5, [r9, #0]
	mul	ip, r2, r3
	ldr	r1, [sp, #112]
	ldr	r2, [sp, #124]
	add	r2, r1, r2
	str	r2, [sp, #124]
	mul	r4, lr, r3
	ldr	r3, [sp, #116]
	cmp	r2, r1
	addcc	r3, r3, #1
	str	r3, [sp, #116]
	mla	r2, lr, r0, ip
	cmp	r2, ip
	add	ip, sp, #112
	add	r0, sp, #120
	mov	r1, r2, lsr #16
	addcc	r3, r4, #65536
	addcc	r4, r3, r1
L1060:
	addcs	r4, r4, r1
L1061:
	mov	r2, r2, asl #16
	adds	r6, r6, r2
	str	r6, [r0, #0]
	addcs	r4, r4, #1
	str	r4, [ip, #0]
	ldr	r1, [sp, #120]
	ldr	r3, [sp, #132]
	ldr	r2, [sp, #112]
	add	r3, r1, r3
	cmp	r3, r1
	addcc	r2, r2, #1
	str	r2, [sp, #112]
	str	r3, [sp, #132]
	ldr	r1, [sp, #124]
	ldr	r3, [sp, #116]
	add	r2, r1, r2
	str	r2, [sp, #124]
	cmp	r2, r1
	addcc	r3, r3, #1
	str	r3, [sp, #116]
	ldr	r7, [sp, #32]
	ldr	r3, [sp, #128]
	str	r3, [r7, #0]
	ldr	r3, [sp, #132]
	ldr	r8, [sp, #36]
	str	r3, [r8, #0]
	ldr	r3, [sp, #124]
	ldr	r9, [sp, #40]
	str	r3, [r9, #0]
	ldr	r3, [sp, #116]
	ldr	r7, [sp, #44]
	str	r3, [r7, #0]
	ldr	r1, [sp, #144]
	ldr	r3, [sp, #164]
	ldr	r2, [sp, #168]
	add	r0, r1, r3
	ldr	r3, [sp, #148]
	str	r0, [sp, #144]
	add	r3, r3, r2
	cmp	r0, r1
	movcs	r1, r3
	addcc	r1, r3, #1
	str	r1, [sp, #148]
	add	r2, sp, #136
	ldmia	r2, {r2, r3}	@ phole ldm
	cmp	r2, #0
	moveq	ip, r3
	orrne	ip, r3, #1
	str	ip, [sp, #140]
	mvn	r3, #-16777216
	sub	r3, r3, #14680064
	cmp	r1, r3
	bls	L1067
	mov	r2, #31
	subs	r3, ip, #0
	movne	r3, #1
	orr	r3, r3, r0, asl r2
	str	r3, [sp, #140]
	mov	r3, r0, lsr #1
	orr	r3, r3, r1, asl r2
	str	r3, [sp, #144]
	ldr	r8, [sp, #92]
	add	r8, r8, #1
	mov	r3, r1, lsr #1
	str	r3, [sp, #148]
	str	r8, [sp, #92]
L1067:
	add	r2, sp, #144
	ldmia	r2, {r2, r3}	@ phole ldm
	str	r2, [sp, #0]
	ldr	r2, [sp, #140]
	str	r2, [sp, #4]
	ldr	r1, [sp, #96]
	ldr	r2, [sp, #92]
	add	r4, sp, #104
	mov	r0, r4
	bl	_roundAndPackFloat64
L1081:
	ldmia	r4, {r8-r9}
L1079:
	ldr	r7, [sp, #100]
L1080:
	stmia	r7, {r8-r9}
	ldr	r0, [sp, #100]
	add	sp, sp, #192
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	add	sp, sp, #4
	movs	pc, lr
	.align	0
	.global	___divdf3
___divdf3:
	@ args = 8, pretend = 4, frame = 188
	@ frame_needed = 0, current_function_anonymous_args = 0
	sub	sp, sp, #4
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #196
	str	r0, [sp, #76]
	str	r3, [sp, #224]
	mov	r6, r2
	mov	r5, r1
	add	r7, sp, #224
	ldmia	r7, {r0-r1}
	mov	r3, r5, lsr #20
	mov	r3, r3, asl #21
	str	r3, [sp, #52]
	str	r6, [sp, #148]
	bic	r8, r5, #-16777216
	bic	r8, r8, #15728640
	str	r8, [sp, #56]
	str	r8, [sp, #152]
	mov	r9, r3
	mov	r9, r9, lsr #21
	str	r9, [sp, #156]
	str	r1, [sp, #160]
	bic	lr, r0, #-16777216
	bic	lr, lr, #15728640
	str	lr, [sp, #164]
	mov	r7, r9
	str	r7, [sp, #8]
	str	r9, [sp, #52]
	mov	r3, r0, lsr #20
	mov	ip, r3, asl #21
	mov	ip, ip, lsr #21
	str	ip, [sp, #168]
	mov	r2, r5, lsr #31
	mov	r3, r0, lsr #31
	eor	r2, r2, r3
	str	r2, [sp, #72]
	mov	r4, #2032
	add	r4, r4, #15
	cmp	r7, r4
	bne	L1091
	orrs	r3, r6, r8
	beq	L1092
	str	r1, [sp, #0]
	mov	r3, r0
	add	r0, sp, #188
	mov	r2, r6
	mov	r1, r5
	bl	_propagateFloat64NaN
	add	r7, sp, #188
	ldmia	r7, {r7-r8}
	b	L1164
L1092:
	ldr	r8, [sp, #8]
	cmp	ip, r8
	bne	L1093
	mov	r3, r1
	orrs	lr, lr, r3
	beq	L1095
	str	r3, [sp, #0]
	add	r4, sp, #180
	mov	r3, r0
	mov	r0, r4
	mov	r2, r6
	mov	r1, r5
	bl	_propagateFloat64NaN
	b	L1165
L1093:
	str	r3, [sp, #48]
	ldr	r7, [sp, #72]
	mov	r3, r7, asl #31
	orr	r3, r3, #2130706432
	str	r3, [sp, #44]
	mov	r8, r3
	orr	r8, r8, #15728640
	str	r8, [sp, #44]
	ldr	r9, [sp, #76]
	add	r7, sp, #44
	ldmia	r7, {r7-r8}
	b	L1166
L1091:
	cmp	ip, r4
	bne	L1097
	mov	r2, r1
	orrs	r3, lr, r2
	beq	L1098
	str	r2, [sp, #0]
	mov	r3, r0
	add	r4, sp, #172
	mov	r0, r4
	mov	r2, r6
	mov	r1, r5
	bl	_propagateFloat64NaN
	b	L1165
L1098:
	str	r3, [sp, #20]
	ldr	r7, [sp, #72]
	mov	r7, r7, asl #31
	str	r7, [sp, #16]
	ldr	r9, [sp, #76]
	add	r7, sp, #16
	ldmia	r7, {r7-r8}
	b	L1166
L1097:
	cmp	ip, #0
	bne	L1100
	orrs	r4, lr, r1
	bne	L1101
	add	r8, sp, #52
	ldmia	r8, {r8, r9}	@ phole ldm
	orr	r3, r8, r9
	orrs	r5, r6, r3
	bne	L1102
L1095:
	mov	r0, #1
	bl	_float_raise
	mov	r7, #0
	str	r7, [sp, #64]
	mov	r8, #-2147483648
	str	r8, [sp, #60]
	mov	r9, r8
	mov	r9, r9, asr #12
	str	r9, [sp, #60]
	ldr	r9, [sp, #76]
	add	r7, sp, #60
	ldmia	r7, {r7-r8}
	b	L1166
L1102:
	mov	r0, #4
	bl	_float_raise
	str	r4, [sp, #40]
	ldr	r8, [sp, #72]
	mov	r3, r8, asl #31
	orr	r3, r3, #2130706432
	str	r3, [sp, #36]
	mov	r9, r3
	orr	r9, r9, #15728640
	str	r9, [sp, #36]
	ldr	r9, [sp, #76]
	add	r7, sp, #36
	ldmia	r7, {r7-r8}
	b	L1166
L1101:
	add	r3, sp, #160
	str	r3, [sp, #0]
	mov	r0, lr
	add	r2, sp, #168
	add	r3, sp, #164
	bl	_normalizeFloat64Subnormal
L1100:
	ldr	r3, [sp, #156]
	cmp	r3, #0
	bne	L1104
	ldr	r0, [sp, #152]
	ldr	r1, [sp, #148]
	orrs	r3, r0, r1
	bne	L1105
	str	r3, [sp, #32]
	ldr	r8, [sp, #72]
	mov	r8, r8, asl #31
	str	r8, [sp, #28]
	ldr	r9, [sp, #76]
	add	r7, sp, #28
	ldmia	r7, {r7-r8}
	b	L1166
L1105:
	add	r3, sp, #148
	str	r3, [sp, #0]
	add	r2, sp, #156
	add	r3, sp, #152
	bl	_normalizeFloat64Subnormal
L1104:
	add	r1, sp, #148
	ldmia	r1, {r1, r3}	@ phole ldm
	ldr	r2, [sp, #160]
	ldr	r0, [sp, #168]
	mov	r4, r1, asl #11
	orr	r3, r3, #1048576
	mov	r1, r1, lsr #21
	orr	r1, r1, r3, asl #11
	ldr	r3, [sp, #164]
	mov	lr, r2, asl #11
	mov	r2, r2, lsr #21
	orr	r3, r3, #1048576
	orr	r2, r2, r3, asl #11
	ldr	r3, [sp, #156]
	rsb	r3, r0, r3
	add	r3, r3, #1020
	str	r3, [sp, #68]
	str	r4, [sp, #148]
	str	r1, [sp, #152]
	str	lr, [sp, #160]
	str	r2, [sp, #164]
	mov	r8, r3
	add	r8, r8, #1
	str	r8, [sp, #68]
	mov	ip, #0
	cmp	r2, r1
	bcc	L1114
	movne	r2, #0
	moveq	r2, #1
	cmp	lr, r4
	movhi	r3, #0
	movls	r3, #1
	tst	r2, r3
	beq	L1115
L1114:
	mov	ip, #1
L1115:
	cmp	ip, #0
	beq	L1113
	ldr	r3, [sp, #148]
	ldr	r2, [sp, #152]
	ldr	r9, [sp, #68]
	mov	r3, r3, lsr #1
	orr	r3, r3, r2, asl #31
	str	r3, [sp, #148]
	add	r9, r9, #1
	str	r9, [sp, #68]
	mov	r2, r2, lsr #1
	str	r2, [sp, #152]
L1113:
	ldr	r0, [sp, #152]
	ldr	r1, [sp, #148]
	ldr	r2, [sp, #164]
	bl	_estimateDiv64To32
	ldr	r2, [sp, #160]
	mov	r3, r0, asl #16
	mov	r3, r3, lsr #16
	mov	r1, r2, asl #16
	mov	r1, r1, lsr #16
	mul	r4, r3, r1
	mov	ip, r0, lsr #16
	mov	r2, r2, lsr #16
	mul	lr, ip, r2
	mul	r3, r2, r3
	str	r0, [sp, #96]
	add	r7, sp, #144
	str	r7, [sp, #12]
	add	r8, sp, #140
	str	r8, [sp, #8]
	mla	r1, ip, r1, r3
	ldr	r2, [sp, #164]
	add	r6, sp, #136
	add	r5, sp, #132
	cmp	r1, r3
	add	ip, sp, #128
	mov	r0, r1, lsr #16
	addcc	r3, lr, #65536
	addcc	lr, r3, r0
L1124:
	addcs	lr, lr, r0
L1125:
	mov	r1, r1, asl #16
	adds	r4, r4, r1
	str	r4, [ip, #0]
	addcs	lr, lr, #1
	str	lr, [r5, #0]
	ldr	r1, [sp, #96]
	mov	r0, r2, asl #16
	mov	r2, r2, lsr #16
	mov	r3, r1, asl #16
	mov	r1, r1, lsr #16
	mul	ip, r1, r2
	mov	r0, r0, lsr #16
	mov	r3, r3, lsr #16
	mul	lr, r3, r0
	mul	r3, r2, r3
	mla	r2, r1, r0, r3
	add	r4, sp, #124
	cmp	r2, r3
	add	r0, sp, #120
	mov	r1, r2, lsr #16
	addcc	r3, ip, #65536
	addcc	ip, r3, r1
L1127:
	addcs	ip, ip, r1
L1128:
	mov	r2, r2, asl #16
	adds	lr, lr, r2
	str	lr, [r0, #0]
	addcs	ip, ip, #1
	str	ip, [r4, #0]
	ldr	r1, [sp, #120]
	ldr	r2, [sp, #132]
	ldr	r3, [sp, #124]
	add	r2, r1, r2
	str	r2, [sp, #132]
	cmp	r2, r1
	addcc	r3, r3, #1
	str	r3, [sp, #124]
	ldr	r3, [sp, #128]
	str	r3, [r6, #0]
	ldr	r3, [sp, #132]
	ldr	r9, [sp, #8]
	str	r3, [r9, #0]
	ldr	r3, [sp, #124]
	ldr	r7, [sp, #12]
	str	r3, [r7, #0]
	ldr	r3, [sp, #152]
	add	r2, sp, #144
	ldmia	r2, {r2, r4}	@ phole ldm
	add	ip, sp, #136
	ldmia	ip, {ip, lr}	@ phole ldm
	rsb	r1, lr, r4
	rsb	r3, r2, r3
	subs	r0, ip, #0
	movne	r0, #1
	cmp	r1, r0
	movcs	r2, #0
	movcc	r2, #1
	rsb	r3, r2, r3
	rsb	r6, ip, #0
	rsb	r5, r0, r1
	cmp	r4, lr
	movcs	ip, r3
	subcc	ip, r3, #1
	cmp	ip, #0
	bge	L1134
	ldr	r4, [sp, #164]
	ldr	lr, [sp, #160]
L1135:
	ldr	r3, [sp, #96]
	sub	r3, r3, #1
	str	r3, [sp, #96]
	adds	r0, r6, lr
	movcc	r1, #0
	movcs	r1, #1
	mov	r6, r0
	adds	r3, r5, r4
	movcc	r2, #0
	movcs	r2, #1
	adds	r3, r3, r1
	mov	r5, r3
	adc	r2, ip, r2
	mov	ip, r2
	cmp	ip, #0
	blt	L1135
L1134:
	ldr	r2, [sp, #164]
	mov	r0, r5
	mov	r1, r6
	bl	_estimateDiv64To32
	str	r0, [sp, #92]
	mov	r3, r0, asl #22
	mov	r3, r3, lsr #22
	cmp	r3, #4
	bhi	L1138
	ldr	r2, [sp, #160]
	mov	r3, r0, asl #16
	mov	r3, r3, lsr #16
	mov	r1, r2, asl #16
	mov	r1, r1, lsr #16
	mul	lr, r3, r1
	mov	r0, r0, lsr #16
	mov	r2, r2, lsr #16
	mul	ip, r0, r2
	add	r8, sp, #140
	str	r8, [sp, #24]
	mul	r3, r2, r3
	add	r9, sp, #136
	str	r9, [sp, #8]
	add	r7, sp, #116
	str	r7, [sp, #12]
	add	r8, sp, #112
	str	r8, [sp, #16]
	mla	r1, r0, r1, r3
	ldr	r2, [sp, #164]
	add	r4, sp, #108
	cmp	r1, r3
	mov	r0, r1, lsr #16
	addcc	r3, ip, #65536
	addcc	ip, r3, r0
L1139:
	addcs	ip, ip, r0
L1140:
	mov	r1, r1, asl #16
	adds	lr, lr, r1
	str	lr, [r4, #0]
	ldr	r9, [sp, #16]
	addcs	ip, ip, #1
	str	ip, [r9, #0]
	ldr	r1, [sp, #92]
	mov	r0, r2, asl #16
	mov	r0, r0, lsr #16
	mov	r3, r1, asl #16
	mov	r3, r3, lsr #16
	mul	lr, r3, r0
	mov	r2, r2, lsr #16
	mul	r3, r2, r3
	mov	r1, r1, lsr #16
	mul	ip, r1, r2
	mla	r2, r1, r0, r3
	add	r4, sp, #104
	cmp	r2, r3
	add	r0, sp, #100
	mov	r1, r2, lsr #16
	addcc	r3, ip, #65536
	addcc	ip, r3, r1
L1142:
	addcs	ip, ip, r1
L1143:
	mov	r2, r2, asl #16
	adds	lr, lr, r2
	str	lr, [r0, #0]
	addcs	ip, ip, #1
	str	ip, [r4, #0]
	ldr	r1, [sp, #100]
	ldr	r2, [sp, #112]
	ldr	r3, [sp, #104]
	add	r2, r1, r2
	str	r2, [sp, #112]
	cmp	r2, r1
	addcc	r3, r3, #1
	str	r3, [sp, #104]
	ldr	r7, [sp, #12]
	ldr	r3, [sp, #108]
	str	r3, [r7, #0]
	ldr	r3, [sp, #112]
	ldr	r8, [sp, #8]
	str	r3, [r8, #0]
	ldr	r3, [sp, #104]
	ldr	r9, [sp, #24]
	str	r3, [r9, #0]
	ldr	r3, [sp, #140]
	ldr	ip, [sp, #136]
	ldr	lr, [sp, #116]
	rsb	r1, ip, r6
	rsb	r3, r3, r5
	subs	r0, lr, #0
	movne	r0, #1
	cmp	r1, r0
	movcs	r2, #0
	movcc	r2, #1
	rsb	r3, r2, r3
	cmp	r6, ip
	subcc	r3, r3, #1
	rsb	lr, lr, #0
	rsb	r6, r0, r1
	subs	r5, r3, #0
	bge	L1149
	ldr	r4, [sp, #164]
	ldr	ip, [sp, #160]
L1150:
	ldr	r3, [sp, #92]
	sub	r3, r3, #1
	str	r3, [sp, #92]
	adds	r1, lr, ip
	movcc	r2, #0
	movcs	r2, #1
	mov	lr, r1
	adds	r3, r6, r4
	movcc	r0, #0
	movcs	r0, #1
	adds	r3, r3, r2
	mov	r6, r3
	adc	r5, r5, r0
	cmp	r5, #0
	blt	L1150
L1149:
	ldr	r3, [sp, #92]
	orr	r2, r5, r6
	orrs	r2, r2, lr
	orrne	r3, r3, #1
	str	r3, [sp, #92]
L1138:
	ldr	r2, [sp, #92]
	mov	r1, #21
	mov	r3, r2, asl r1
	str	r3, [sp, #88]
	ldr	r3, [sp, #96]
	mov	r2, r2, lsr #11
	orr	r2, r2, r3, asl r1
	str	r2, [sp, #92]
	mov	r3, r3, lsr #11
	str	r3, [sp, #96]
	str	r2, [sp, #0]
	ldr	r2, [sp, #88]
	str	r2, [sp, #4]
	ldr	r1, [sp, #72]
	ldr	r2, [sp, #68]
	add	r4, sp, #80
	mov	r0, r4
	bl	_roundAndPackFloat64
L1165:
	ldmia	r4, {r7-r8}
L1164:
	ldr	r9, [sp, #76]
L1166:
	stmia	r9, {r7-r8}
	ldr	r0, [sp, #76]
	add	sp, sp, #196
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	add	sp, sp, #4
	movs	pc, lr
	.align	0
	.global	_float64_rem
_float64_rem:
	@ args = 8, pretend = 4, frame = 144
	@ frame_needed = 0, current_function_anonymous_args = 0
	sub	sp, sp, #4
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #148
	str	r0, [sp, #52]
	add	r5, sp, #12
	stmia	r5, {r1-r2}
	str	r3, [sp, #176]
	add	r6, sp, #176
	ldmia	r6, {r0-r1}
	ldr	r9, [sp, #16]
	str	r9, [sp, #108]
	ldr	r5, [sp, #12]
	bic	r4, r5, #-16777216
	bic	r4, r4, #15728640
	str	r4, [sp, #112]
	mov	r6, r5
	mov	r2, r6, lsr #20
	mov	r2, r2, asl #21
	mov	r2, r2, lsr #21
	str	r2, [sp, #116]
	str	r1, [sp, #120]
	bic	lr, r0, #-16777216
	bic	lr, lr, #15728640
	str	lr, [sp, #124]
	mov	r3, r0, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	str	r3, [sp, #128]
	mov	r9, r5
	mov	r9, r9, lsr #31
	str	r9, [sp, #48]
	mov	ip, #2032
	add	ip, ip, #15
	cmp	r2, ip
	bne	L1176
	ldr	r5, [sp, #16]
	orrs	r5, r5, r4
	bne	L1178
	cmp	r3, ip
	bne	L1179
	orrs	lr, r1, lr
	beq	L1179
L1178:
	str	r1, [sp, #0]
	mov	r3, r0
	add	r6, sp, #12
	ldmia	r6, {r1-r2}
	add	r0, sp, #140
	bl	_propagateFloat64NaN
	add	r5, sp, #140
	ldmia	r5, {r5-r6}
	b	L1276
L1176:
	cmp	r3, ip
	bne	L1180
	mov	r3, r1
	orrs	lr, lr, r3
	beq	L1181
	str	r3, [sp, #0]
	add	r6, sp, #12
	mov	r3, r0
	ldmia	r6, {r1-r2}
	add	r4, sp, #132
	mov	r0, r4
	bl	_propagateFloat64NaN
	b	L1277
L1181:
	add	r5, sp, #12
	ldmia	r5, {r5-r6}
	b	L1276
L1180:
	cmp	r3, #0
	bne	L1182
	orrs	r6, lr, r1
	bne	L1183
L1179:
	mov	r0, #1
	bl	_float_raise
	ldr	r9, [sp, #52]
	mov	r8, #0
	mov	r7, #-2147483648
	mov	r7, r7, asr #12
	stmia	r9, {r7-r8}
	b	L1167
L1183:
	add	r3, sp, #120
	str	r3, [sp, #0]
	mov	r0, lr
	add	r2, sp, #128
	add	r3, sp, #124
	bl	_normalizeFloat64Subnormal
L1182:
	ldr	r3, [sp, #116]
	cmp	r3, #0
	bne	L1184
	ldr	r0, [sp, #112]
	ldr	r1, [sp, #108]
	orrs	r5, r0, r1
	addeq	r5, sp, #12
	ldmeqia	r5, {r5-r6}
	beq	L1276
L1185:
	add	r3, sp, #108
	str	r3, [sp, #0]
	add	r2, sp, #116
	add	r3, sp, #112
	bl	_normalizeFloat64Subnormal
L1184:
	ldr	r2, [sp, #116]
	ldr	r3, [sp, #128]
	rsb	r8, r3, r2
	cmn	r8, #1
	addlt	r5, sp, #12
	ldmltia	r5, {r5-r6}
	blt	L1276
L1186:
	ldr	r0, [sp, #108]
	cmp	r8, #0
	movge	r2, #11
	movlt	r2, #10
	mov	r3, r0, asl r2
	str	r3, [sp, #108]
	ldr	r3, [sp, #112]
	cmp	r2, #0
	orr	r1, r3, #1048576
	beq	L1189
	rsb	r3, r2, #0
	and	r3, r3, #31
	mov	r3, r0, lsr r3
	orr	r3, r3, r1, asl r2
	str	r3, [sp, #112]
	b	L1191
L1189:
	str	r1, [sp, #112]
L1191:
	ldr	r3, [sp, #120]
	ldr	r2, [sp, #124]
	mov	ip, r3, asl #11
	str	ip, [sp, #120]
	orr	r2, r2, #1048576
	mov	r3, r3, lsr #21
	orr	r3, r3, r2, asl #11
	str	r3, [sp, #124]
	add	r1, sp, #108
	ldmia	r1, {r1, r2}	@ phole ldm
	mov	r0, #0
	cmp	r3, r2
	bcc	L1195
	movne	r2, #0
	moveq	r2, #1
	cmp	ip, r1
	movhi	r3, #0
	movls	r3, #1
	tst	r2, r3
	beq	L1196
L1195:
	mov	r0, #1
L1196:
	and	r7, r0, #255
	cmp	r7, #0
	beq	L1198
	ldr	r0, [sp, #108]
	ldr	r1, [sp, #120]
	rsb	r3, r1, r0
	str	r3, [sp, #108]
	ldr	r2, [sp, #124]
	ldr	r3, [sp, #112]
	rsb	r3, r2, r3
	cmp	r0, r1
	subcc	r3, r3, #1
	str	r3, [sp, #112]
L1198:
	sub	r8, r8, #32
	cmp	r8, #0
	ble	L1201
	add	r6, sp, #104
	str	r6, [sp, #44]
	add	r9, sp, #100
	str	r9, [sp, #40]
	add	r5, sp, #96
	str	r5, [sp, #36]
	add	r6, sp, #92
	str	r6, [sp, #32]
	add	r9, sp, #88
	str	r9, [sp, #4]
L1202:
	ldr	r0, [sp, #112]
	ldr	r1, [sp, #108]
	ldr	r2, [sp, #124]
	bl	_estimateDiv64To32
	ldr	r2, [sp, #120]
	cmp	r0, #4
	subhi	r7, r0, #4
	movls	r7, #0
	mov	r3, r7, asl #16
	mov	r3, r3, lsr #16
	mov	r1, r2, asl #16
	mov	r1, r1, lsr #16
	mul	r5, r3, r1
	mov	r0, r7, lsr #16
	mov	r2, r2, lsr #16
	mul	r4, r0, r2
	mul	r3, r2, r3
	str	r5, [sp, #8]
	mla	ip, r0, r1, r3
	ldr	r2, [sp, #124]
	cmp	ip, r3
	mov	r1, ip, lsr #16
	addcc	r3, r4, #65536
	addcc	r4, r3, r1
L1205:
	addcs	r4, r4, r1
L1206:
	mov	r1, r2, asl #16
	mov	r1, r1, lsr #16
	mov	r3, r7, asl #16
	mov	r3, r3, lsr #16
	mul	r6, r3, r1
	ldr	r9, [sp, #8]
	mov	ip, ip, asl #16
	adds	r9, r9, ip
	str	r9, [sp, #8]
	str	r6, [sp, #12]
	mov	r0, r7, lsr #16
	mov	r2, r2, lsr #16
	mul	lr, r0, r2
	ldr	r5, [sp, #4]
	str	r9, [r5, #0]
	mul	r3, r2, r3
	ldr	r6, [sp, #32]
	addcs	r4, r4, #1
	str	r4, [r6, #0]
	mla	r2, r0, r1, r3
	add	ip, sp, #84
	cmp	r2, r3
	add	r0, sp, #80
	mov	r1, r2, lsr #16
	addcc	r3, lr, #65536
	addcc	lr, r3, r1
L1208:
	addcs	lr, lr, r1
L1209:
	ldr	r9, [sp, #12]
	mov	r2, r2, asl #16
	adds	r9, r9, r2
	str	r9, [sp, #12]
	str	r9, [r0, #0]
	addcs	lr, lr, #1
	str	lr, [ip, #0]
	ldr	r1, [sp, #80]
	ldr	r2, [sp, #92]
	ldr	r3, [sp, #84]
	add	r2, r1, r2
	str	r2, [sp, #92]
	cmp	r2, r1
	addcc	r3, r3, #1
	str	r3, [sp, #84]
	ldr	r5, [sp, #36]
	ldr	r3, [sp, #88]
	str	r3, [r5, #0]
	ldr	r3, [sp, #92]
	ldr	r6, [sp, #40]
	str	r3, [r6, #0]
	ldr	r3, [sp, #84]
	ldr	r9, [sp, #44]
	str	r3, [r9, #0]
	add	r0, sp, #108
	ldmia	r0, {r0, r3}	@ phole ldm
	mov	r0, r0, lsr #3
	orr	r0, r0, r3, asl #29
	str	r0, [sp, #112]
	add	r1, sp, #96
	ldmia	r1, {r1, r2, r3}	@ phole ldm
	mov	ip, #3
	mov	r1, r1, lsr ip
	orr	r1, r1, r2, asl #29
	str	r1, [sp, #96]
	mov	r2, r2, lsr ip
	orr	r2, r2, r3, asl #29
	str	r2, [sp, #100]
	rsb	r3, r1, #0
	str	r3, [sp, #108]
	rsb	r0, r2, r0
	cmp	r1, #0
	subne	r0, r0, #1
	str	r0, [sp, #112]
	sub	r8, r8, #29
	cmp	r8, #0
	bgt	L1202
L1201:
	cmn	r8, #32
	ble	L1220
	ldr	r0, [sp, #112]
	ldr	r1, [sp, #108]
	ldr	r2, [sp, #124]
	bl	_estimateDiv64To32
	ldr	r3, [sp, #120]
	ldr	r2, [sp, #124]
	mov	r3, r3, lsr #8
	orr	r3, r3, r2, asl #24
	str	r3, [sp, #120]
	mov	r2, r2, lsr #8
	str	r2, [sp, #124]
	cmp	r0, #4
	subhi	r7, r0, #4
	movls	r7, #0
	rsb	r3, r8, #0
	mov	r7, r7, lsr r3
	add	r8, r8, #24
	cmp	r8, #0
	bge	L1230
	ldr	r0, [sp, #112]
	ldr	r3, [sp, #108]
	rsb	r1, r8, #0
	cmp	r1, #0
	add	lr, sp, #112
	add	ip, sp, #108
	and	r2, r8, #31
	moveq	r2, r3
	moveq	r3, r0
	beq	L1232
L1231:
	cmp	r1, #31
	movle	r3, r3, lsr r1
	orrle	r2, r3, r0, asl r2
	movle	r3, r0, lsr r1
	ble	L1232
L1233:
	cmp	r1, #63
	andle	r3, r1, #31
	movle	r2, r0, lsr r3
L1235:
	movgt	r2, #0
L1236:
	mov	r3, #0
L1232:
	str	r2, [ip, #0]
	str	r3, [lr, #0]
	b	L1238
L1230:
	ldr	r1, [sp, #108]
	mov	r3, r1, asl r8
	str	r3, [sp, #108]
	ldr	r2, [sp, #112]
	beq	L1239
	rsb	r3, r8, #0
	and	r3, r3, #31
	mov	r3, r1, lsr r3
	orr	r3, r3, r2, asl r8
	str	r3, [sp, #112]
	b	L1238
L1239:
	str	r2, [sp, #112]
L1238:
	ldr	r2, [sp, #120]
	mov	r3, r7, asl #16
	mov	r3, r3, lsr #16
	mov	r1, r2, asl #16
	mov	r1, r1, lsr #16
	mul	r5, r3, r1
	mov	r0, r7, lsr #16
	mov	r2, r2, lsr #16
	mul	r4, r0, r2
	add	r6, sp, #104
	str	r6, [sp, #28]
	str	r5, [sp, #8]
	mul	r3, r2, r3
	add	r9, sp, #100
	str	r9, [sp, #24]
	add	r6, sp, #76
	str	r6, [sp, #4]
	add	r5, sp, #96
	str	r5, [sp, #20]
	mla	ip, r0, r1, r3
	ldr	r1, [sp, #124]
	add	r8, sp, #72
	cmp	ip, r3
	mov	r2, ip, lsr #16
	addcc	r3, r4, #65536
	addcc	r4, r3, r2
L1242:
	addcs	r4, r4, r2
L1243:
	mov	r2, r1, asl #16
	mov	r2, r2, lsr #16
	mov	r3, r7, asl #16
	mov	r3, r3, lsr #16
	mul	r9, r3, r2
	ldr	r5, [sp, #8]
	mov	ip, ip, asl #16
	adds	r5, r5, ip
	str	r5, [sp, #8]
	str	r9, [sp, #12]
	mov	r0, r7, lsr #16
	mov	r1, r1, lsr #16
	mul	lr, r0, r1
	str	r5, [r8, #0]
	mul	r3, r1, r3
	ldr	r6, [sp, #4]
	addcs	r4, r4, #1
	str	r4, [r6, #0]
	mla	r2, r0, r2, r3
	add	ip, sp, #68
	cmp	r2, r3
	add	r0, sp, #64
	mov	r1, r2, lsr #16
	addcc	r3, lr, #65536
	addcc	lr, r3, r1
L1245:
	addcs	lr, lr, r1
L1246:
	ldr	r9, [sp, #12]
	mov	r2, r2, asl #16
	adds	r9, r9, r2
	str	r9, [sp, #12]
	str	r9, [r0, #0]
	addcs	lr, lr, #1
	str	lr, [ip, #0]
	ldr	r1, [sp, #64]
	ldr	r2, [sp, #76]
	ldr	r3, [sp, #68]
	add	r2, r1, r2
	str	r2, [sp, #76]
	cmp	r2, r1
	addcc	r3, r3, #1
	str	r3, [sp, #68]
	ldr	r5, [sp, #20]
	ldr	r3, [sp, #72]
	str	r3, [r5, #0]
	ldr	r3, [sp, #76]
	ldr	r6, [sp, #24]
	str	r3, [r6, #0]
	ldr	r3, [sp, #68]
	ldr	r9, [sp, #28]
	str	r3, [r9, #0]
	ldr	r0, [sp, #108]
	ldr	r1, [sp, #96]
	rsb	r3, r1, r0
	str	r3, [sp, #108]
	ldr	r2, [sp, #100]
	ldr	r3, [sp, #112]
	rsb	r3, r2, r3
	cmp	r0, r1
	subcc	r3, r3, #1
	str	r3, [sp, #112]
	b	L1251
L1220:
	ldr	r2, [sp, #112]
	ldr	r3, [sp, #108]
	mov	r1, #24
	mov	r3, r3, lsr #8
	orr	r3, r3, r2, asl r1
	str	r3, [sp, #108]
	mov	r2, r2, lsr #8
	str	r2, [sp, #112]
	ldr	r3, [sp, #120]
	ldr	r2, [sp, #124]
	mov	r3, r3, lsr #8
	orr	r3, r3, r2, asl r1
	str	r3, [sp, #120]
	mov	r2, r2, lsr #8
	str	r2, [sp, #124]
L1251:
	add	r0, sp, #120
	ldmia	r0, {r0, lr}	@ phole ldm
L1266:
	add	r2, sp, #108
	ldmia	r2, {r2, ip}	@ phole ldm
	rsb	r1, r0, r2
	str	r1, [sp, #108]
	rsb	r3, lr, ip
	cmp	r2, r0
	subcc	r3, r3, #1
	str	r3, [sp, #112]
	add	r7, r7, #1
	cmp	r3, #0
	bge	L1266
	adds	r1, r1, r2
	adc	r3, r3, ip
	cmp	r3, #0
	blt	L1273
	orrs	r3, r3, r1
	bne	L1272
	tst	r7, #1
	beq	L1272
L1273:
	str	ip, [sp, #112]
	str	r2, [sp, #108]
L1272:
	ldr	r1, [sp, #112]
	mov	r3, r1, lsr #31
	cmp	r3, #0
	mov	ip, r3
	beq	L1274
	ldr	r2, [sp, #108]
	rsb	r3, r2, #0
	str	r3, [sp, #108]
	rsb	r3, r1, #0
	cmp	r2, #0
	subne	r3, r3, #1
	str	r3, [sp, #112]
L1274:
	ldr	r2, [sp, #128]
	add	r1, sp, #108
	ldmia	r1, {r1, r3}	@ phole ldm
	str	r1, [sp, #0]
	ldr	r5, [sp, #48]
	add	r4, sp, #56
	mov	r0, r4
	sub	r2, r2, #4
	eor	r1, r5, ip
	bl	_normalizeRoundAndPackFloat64
L1277:
	ldmia	r4, {r5-r6}
L1276:
	ldr	r9, [sp, #52]
	stmia	r9, {r5-r6}
L1167:
	ldr	r0, [sp, #52]
	add	sp, sp, #148
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	add	sp, sp, #4
	movs	pc, lr
	.align	0
	.global	_float64_sqrt
_float64_sqrt:
	@ args = 0, pretend = 0, frame = 72
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, lr}
	sub	sp, sp, #80
	str	r0, [sp, #12]
	str	r2, [sp, #60]
	mov	r3, r1, lsr #20
	mov	ip, r3, asl #21
	mov	ip, ip, lsr #21
	str	ip, [sp, #68]
	bic	r0, r1, #-16777216
	bic	r0, r0, #15728640
	str	r0, [sp, #64]
	mov	r3, #2032
	add	r3, r3, #15
	cmp	ip, r3
	mov	lr, r1, lsr #31
	bne	L1283
	mov	r3, r2
	orrs	r0, r0, r3
	beq	L1284
	str	r3, [sp, #0]
	add	r0, sp, #72
	mov	r3, r1
	bl	_propagateFloat64NaN
	add	r7, sp, #72
	ldmia	r7, {r7-r8}
	b	L1350
L1284:
	cmp	lr, #0
	bne	L1286
	ldr	r8, [sp, #12]
	stmia	r8, {r1-r2}
	b	L1278
L1283:
	cmp	lr, #0
	beq	L1287
	orr	r3, ip, r0
	orrs	r3, r2, r3
	ldreq	r9, [sp, #12]
	stmeqia	r9, {r1-r2}
	beq	L1278
L1286:
	mov	r0, #1
	bl	_float_raise
	mov	r7, #0
	str	r7, [sp, #20]
	mov	r8, #-2147483648
	str	r8, [sp, #16]
	mov	r9, r8
	mov	r9, r9, asr #12
	str	r9, [sp, #16]
	ldr	r9, [sp, #12]
	add	r7, sp, #16
	ldmia	r7, {r7-r8}
	b	L1351
L1287:
	cmp	ip, #0
	bne	L1289
	mov	r1, r2
	orrs	r3, r0, r1
	bne	L1290
	ldr	r8, [sp, #12]
	mov	r5, r3
	mov	r4, r3
	stmia	r8, {r4-r5}
	b	L1278
L1290:
	add	r3, sp, #60
	str	r3, [sp, #0]
	add	r2, sp, #68
	add	r3, sp, #64
	bl	_normalizeFloat64Subnormal
L1289:
	ldr	r1, [sp, #60]
	mov	r3, r1, asl #11
	str	r3, [sp, #60]
	ldr	r0, [sp, #68]
	ldr	r3, [sp, #64]
	mov	r1, r1, lsr #21
	orr	r3, r3, #1048576
	orr	r1, r1, r3, asl #11
	str	r1, [sp, #64]
	mov	r2, #1020
	add	r2, r2, #2
	sub	r3, r0, #1020
	sub	r3, r3, #3
	add	r3, r2, r3, asr #1
	str	r3, [sp, #8]
	bl	_estimateSqrt32
	add	r0, r0, #2
	str	r0, [sp, #40]
	cmp	r0, #0
	mvnge	r3, #0
	strge	r3, [sp, #40]
L1295:
	ldrb	r9, [sp, #68]
	tst	r9, #1
	beq	L1296
	ldr	r3, [sp, #60]
	ldr	r2, [sp, #64]
	mov	r3, r3, lsr #1
	orr	r3, r3, r2, asl #31
	str	r3, [sp, #60]
	mov	r2, r2, lsr #1
	str	r2, [sp, #64]
L1296:
	ldr	r3, [sp, #40]
	mov	r2, r3, asl #16
	mov	r2, r2, lsr #16
	mul	ip, r2, r2
	mov	r3, r3, lsr #16
	mul	r1, r3, r3
	mul	r2, r3, r2
	add	r4, sp, #56
	add	lr, sp, #52
	mov	r3, r2
	mov	r2, r2, asl #1
	cmp	r2, r3
	mov	r0, r2, lsr #16
	addcc	r3, r1, #65536
	addcc	r1, r3, r0
L1304:
	addcs	r1, r1, r0
L1305:
	mov	r2, r2, asl #16
	adds	ip, ip, r2
	str	ip, [lr, #0]
	addcs	r1, r1, #1
	str	r1, [r4, #0]
	add	r0, sp, #60
	ldmia	r0, {r0, r3}	@ phole ldm
	add	r1, sp, #52
	ldmia	r1, {r1, r2}	@ phole ldm
	rsb	r3, r2, r3
	cmp	r0, r1
	movcs	ip, r3
	subcc	ip, r3, #1
	cmp	ip, #0
	rsb	r4, r1, r0
	bge	L1309
L1310:
	ldr	r3, [sp, #40]
	sub	r3, r3, #1
	str	r3, [sp, #40]
	mov	r2, r3, asl #1
	str	r2, [sp, #52]
	mov	r3, r3, lsr #31
	str	r3, [sp, #56]
	orr	r2, r2, #1
	str	r2, [sp, #52]
	mov	r1, r4
	add	r4, r4, r2
	add	r3, ip, r3
	cmp	r4, r1
	movcs	ip, r3
	addcc	ip, r3, #1
	cmp	ip, #0
	blt	L1310
L1309:
	ldr	r2, [sp, #40]
	mov	r0, r4, lsr #1
	orr	r0, r0, ip, asl #31
	mov	r1, r4, asl #31
	bl	_estimateDiv64To32
	str	r0, [sp, #36]
	mov	r3, r0, asl #22
	mov	r3, r3, lsr #22
	cmp	r3, #5
	bhi	L1319
	cmp	r0, #0
	moveq	r3, #1
	streq	r3, [sp, #36]
L1320:
	add	r1, sp, #36
	ldmia	r1, {r1, r2}	@ phole ldm
	mov	r0, r2, asl #16
	mov	r0, r0, lsr #16
	mov	r3, r1, asl #16
	mov	r3, r3, lsr #16
	mul	lr, r3, r0
	mov	r1, r1, lsr #16
	mov	r2, r2, lsr #16
	mul	ip, r1, r2
	mul	r3, r2, r3
	mla	r2, r1, r0, r3
	add	r5, sp, #52
	cmp	r2, r3
	add	r0, sp, #48
	mov	r1, r2, lsr #16
	addcc	r3, ip, #65536
	addcc	ip, r3, r1
L1321:
	addcs	ip, ip, r1
L1322:
	mov	r2, r2, asl #16
	adds	lr, lr, r2
	str	lr, [r0, #0]
	addcs	ip, ip, #1
	str	ip, [r5, #0]
	ldr	r2, [sp, #36]
	mov	r3, r2, asl #16
	mov	r3, r3, lsr #16
	mul	r5, r3, r3
	mov	r2, r2, lsr #16
	mul	ip, r2, r3
	ldr	r1, [sp, #52]
	ldr	r3, [sp, #48]
	mov	r0, r3, asl #1
	mov	r3, r3, lsr #31
	orr	r3, r3, r1, asl #1
	str	r3, [sp, #52]
	str	r0, [sp, #48]
	mul	r7, r2, r2
	add	r6, sp, #48
	mov	r2, r7
	rsb	lr, r0, #0
	rsb	r3, r3, r4
	cmp	r0, #0
	moveq	r4, r3
	subne	r4, r3, #1
	mov	r3, ip
	mov	ip, ip, asl #1
	cmp	ip, r3
	add	r0, sp, #44
	mov	r1, ip, lsr #16
	addcc	r3, r2, #65536
	addcc	r2, r3, r1
L1328:
	addcs	r2, r2, r1
L1329:
	mov	ip, ip, asl #16
	adds	r5, r5, ip
	str	r5, [r0, #0]
	addcs	r2, r2, #1
	str	r2, [r6, #0]
	ldr	ip, [sp, #44]
	ldr	r0, [sp, #48]
	subs	r1, ip, #0
	movne	r1, #1
	rsb	r2, r0, lr
	cmp	r2, r1
	movcs	r3, #0
	movcc	r3, #1
	rsb	r3, r3, r4
	cmp	lr, r0
	subcc	r3, r3, #1
	rsb	r0, ip, #0
	rsb	lr, r1, r2
	subs	r4, r3, #0
	bge	L1333
	ldr	r5, [sp, #40]
	mov	r6, #31
L1334:
	ldr	r3, [sp, #36]
	sub	r3, r3, #1
	mov	r2, r3, asl #1
	str	r2, [sp, #44]
	str	r3, [sp, #36]
	orr	r2, r2, #1
	str	r2, [sp, #44]
	mov	r3, r3, lsr r6
	orr	r3, r3, r5, asl #1
	str	r3, [sp, #48]
	mov	r1, r5, lsr r6
	str	r1, [sp, #52]
	adds	r2, r0, r2
	movcc	r0, #0
	movcs	r0, #1
	adds	r3, lr, r3
	movcc	ip, #0
	movcs	ip, #1
	adds	r3, r3, r0
	adc	r1, r4, r1
	mov	r0, r2
	mov	lr, r3
	adds	r4, r1, ip
	bmi	L1334
L1333:
	ldr	r3, [sp, #36]
	orr	r2, r4, lr
	orrs	r2, r2, r0
	orrne	r3, r3, #1
	str	r3, [sp, #36]
L1319:
	ldr	r2, [sp, #36]
	mov	r1, #21
	mov	r3, r2, asl r1
	str	r3, [sp, #32]
	ldr	r3, [sp, #40]
	mov	r2, r2, lsr #11
	orr	r2, r2, r3, asl r1
	str	r2, [sp, #36]
	mov	r3, r3, lsr #11
	str	r3, [sp, #40]
	str	r2, [sp, #0]
	ldr	r2, [sp, #32]
	str	r2, [sp, #4]
	ldr	r2, [sp, #8]
	add	r4, sp, #24
	mov	r0, r4
	mov	r1, #0
	bl	_roundAndPackFloat64
	ldmia	r4, {r7-r8}
L1350:
	ldr	r9, [sp, #12]
L1351:
	stmia	r9, {r7-r8}
L1278:
	ldr	r0, [sp, #12]
	add	sp, sp, #80
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, pc}^
	.align	0
	.global	_float64_eq
_float64_eq:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	mov	r5, r3
	mov	r4, r2
	mov	r3, r0, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	mov	r2, #2032
	add	r2, r2, #15
	cmp	r3, r2
	bne	L1355
	bic	r3, r0, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r1, r3
	bne	L1354
L1355:
	mov	r3, r4, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	cmp	r3, r2
	bne	L1353
	bic	r3, r4, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r5, r3
	beq	L1353
L1354:
	bl	_float64_is_signaling_nan
	tst	r0, #255
	bne	L1363
	mov	r1, r5
	mov	r0, r4
	bl	_float64_is_signaling_nan
	tst	r0, #255
	beq	L1362
L1363:
	mov	r0, #1
	bl	_float_raise
L1362:
	mov	r0, #0
	ldmfd	sp!, {r4, r5, pc}^
L1353:
	mov	r2, #0
	cmp	r1, r5
	bne	L1364
	cmp	r0, r4
	beq	L1365
	cmp	r1, r2
	bne	L1364
	orr	r3, r0, r4
	movs	r3, r3, asl #1
	bne	L1364
L1365:
	mov	r2, #1
L1364:
	mov	r0, r2
	ldmfd	sp!, {r4, r5, pc}^
	.align	0
	.global	_float64_le
_float64_le:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	mov	r5, r3
	mov	r4, r2
	mov	r3, r0, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	mov	r2, #2032
	add	r2, r2, #15
	cmp	r3, r2
	bne	L1370
	bic	r3, r0, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r1, r3
	bne	L1369
L1370:
	mov	r3, r4, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	cmp	r3, r2
	bne	L1368
	bic	r3, r4, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r5, r3
	beq	L1368
L1369:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #0
	ldmfd	sp!, {r4, r5, pc}^
L1368:
	mov	r3, r0, lsr #31
	cmp	r3, r4, lsr #31
	beq	L1379
	mov	r2, #0
	cmp	r3, r2
	bne	L1381
	orr	r3, r0, r4
	orr	r3, r1, r3, asl #1
	orrs	r4, r5, r3
	bne	L1380
L1381:
	mov	r2, #1
L1380:
	mov	r0, r2
	ldmfd	sp!, {r4, r5, pc}^
L1379:
	cmp	r3, #0
	beq	L1382
	mov	ip, #0
	cmp	r4, r0
	mov	r3, r5
	mov	r0, r1
	bcc	L1384
	movne	r2, #0
	moveq	r2, #1
	cmp	r3, r0
	movhi	r3, #0
	movls	r3, #1
	tst	r2, r3
	beq	L1385
L1384:
	mov	ip, #1
L1385:
	and	r0, ip, #255
	ldmfd	sp!, {r4, r5, pc}^
L1382:
	mov	ip, r3
	cmp	r0, r4
	mov	r0, r1
	mov	r3, r5
	bcc	L1387
	movne	r2, #0
	moveq	r2, #1
	cmp	r0, r3
	movhi	r3, #0
	movls	r3, #1
	tst	r2, r3
	beq	L1388
L1387:
	mov	ip, #1
L1388:
	and	r0, ip, #255
	ldmfd	sp!, {r4, r5, pc}^
	.align	0
	.global	_float64_lt
_float64_lt:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	mov	r5, r3
	mov	r4, r2
	mov	r3, r0, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	mov	r2, #2032
	add	r2, r2, #15
	cmp	r3, r2
	bne	L1394
	bic	r3, r0, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r1, r3
	bne	L1393
L1394:
	mov	r3, r4, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	cmp	r3, r2
	bne	L1392
	bic	r3, r4, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r5, r3
	beq	L1392
L1393:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #0
	ldmfd	sp!, {r4, r5, pc}^
L1392:
	mov	r3, r0, lsr #31
	cmp	r3, r4, lsr #31
	beq	L1403
	mov	r2, #0
	cmp	r3, r2
	beq	L1404
	orr	r3, r0, r4
	orr	r3, r1, r3, asl #1
	orrs	r4, r5, r3
	moveq	r2, #0
	movne	r2, #1
L1404:
	mov	r0, r2
	ldmfd	sp!, {r4, r5, pc}^
L1403:
	cmp	r3, #0
	beq	L1405
	mov	ip, #0
	cmp	r4, r0
	mov	r2, r5
	mov	r0, r1
	bcc	L1407
	movne	r3, #0
	moveq	r3, #1
	cmp	r3, ip
	cmpne	r2, r0
	bcs	L1408
L1407:
	mov	ip, #1
L1408:
	and	r0, ip, #255
	ldmfd	sp!, {r4, r5, pc}^
L1405:
	mov	ip, r3
	cmp	r0, r4
	mov	r0, r1
	mov	r2, r5
	bcc	L1410
	movne	r3, #0
	moveq	r3, #1
	cmp	r3, #0
	cmpne	r0, r2
	bcs	L1411
L1410:
	mov	ip, #1
L1411:
	and	r0, ip, #255
	ldmfd	sp!, {r4, r5, pc}^
	.align	0
	.global	_float64_eq_signaling
_float64_eq_signaling:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	mov	r5, r3
	mov	r4, r2
	mov	r3, r0, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	mov	r2, #2032
	add	r2, r2, #15
	cmp	r3, r2
	bne	L1417
	bic	r3, r0, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r1, r3
	bne	L1416
L1417:
	mov	r3, r4, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	cmp	r3, r2
	bne	L1415
	bic	r3, r4, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r5, r3
	beq	L1415
L1416:
	mov	r0, #1
	bl	_float_raise
	mov	r0, #0
	ldmfd	sp!, {r4, r5, pc}^
L1415:
	mov	r2, #0
	cmp	r1, r5
	bne	L1424
	cmp	r0, r4
	beq	L1425
	cmp	r1, r2
	bne	L1424
	orr	r3, r0, r4
	movs	r3, r3, asl #1
	bne	L1424
L1425:
	mov	r2, #1
L1424:
	mov	r0, r2
	ldmfd	sp!, {r4, r5, pc}^
	.align	0
	.global	_float64_le_quiet
_float64_le_quiet:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	mov	r5, r3
	mov	r4, r2
	mov	r3, r0, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	mov	r2, #2032
	add	r2, r2, #15
	cmp	r3, r2
	bne	L1430
	bic	r3, r0, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r1, r3
	bne	L1429
L1430:
	mov	r3, r4, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	cmp	r3, r2
	bne	L1428
	bic	r3, r4, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r5, r3
	beq	L1428
L1429:
	bl	_float64_is_signaling_nan
	tst	r0, #255
	bne	L1438
	mov	r1, r5
	mov	r0, r4
	bl	_float64_is_signaling_nan
	tst	r0, #255
	beq	L1437
L1438:
	mov	r0, #1
	bl	_float_raise
L1437:
	mov	r0, #0
	ldmfd	sp!, {r4, r5, pc}^
L1428:
	mov	r3, r0, lsr #31
	cmp	r3, r4, lsr #31
	beq	L1441
	mov	r2, #0
	cmp	r3, r2
	bne	L1443
	orr	r3, r0, r4
	orr	r3, r1, r3, asl #1
	orrs	r4, r5, r3
	bne	L1442
L1443:
	mov	r2, #1
L1442:
	mov	r0, r2
	ldmfd	sp!, {r4, r5, pc}^
L1441:
	cmp	r3, #0
	beq	L1444
	mov	ip, #0
	cmp	r4, r0
	mov	r3, r5
	mov	r0, r1
	bcc	L1446
	movne	r2, #0
	moveq	r2, #1
	cmp	r3, r0
	movhi	r3, #0
	movls	r3, #1
	tst	r2, r3
	beq	L1447
L1446:
	mov	ip, #1
L1447:
	and	r0, ip, #255
	ldmfd	sp!, {r4, r5, pc}^
L1444:
	mov	ip, r3
	cmp	r0, r4
	mov	r3, r1
	mov	r0, r5
	bcc	L1449
	movne	r2, #0
	moveq	r2, #1
	cmp	r3, r0
	movhi	r3, #0
	movls	r3, #1
	tst	r2, r3
	beq	L1450
L1449:
	mov	ip, #1
L1450:
	and	r0, ip, #255
	ldmfd	sp!, {r4, r5, pc}^
	.align	0
	.global	_float64_lt_quiet
_float64_lt_quiet:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, current_function_anonymous_args = 0
	stmfd	sp!, {r4, r5, lr}
	mov	r5, r3
	mov	r4, r2
	mov	r3, r0, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	mov	r2, #2032
	add	r2, r2, #15
	cmp	r3, r2
	bne	L1456
	bic	r3, r0, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r1, r3
	bne	L1455
L1456:
	mov	r3, r4, lsr #20
	mov	r3, r3, asl #21
	mov	r3, r3, lsr #21
	cmp	r3, r2
	bne	L1454
	bic	r3, r4, #-16777216
	bic	r3, r3, #15728640
	orrs	r3, r5, r3
	beq	L1454
L1455:
	bl	_float64_is_signaling_nan
	tst	r0, #255
	bne	L1464
	mov	r1, r5
	mov	r0, r4
	bl	_float64_is_signaling_nan
	tst	r0, #255
	beq	L1463
L1464:
	mov	r0, #1
	bl	_float_raise
L1463:
	mov	r0, #0
	ldmfd	sp!, {r4, r5, pc}^
L1454:
	mov	r3, r0, lsr #31
	cmp	r3, r4, lsr #31
	beq	L1467
	mov	r2, #0
	cmp	r3, r2
	beq	L1468
	orr	r3, r0, r4
	orr	r3, r1, r3, asl #1
	orrs	r4, r5, r3
	moveq	r2, #0
	movne	r2, #1
L1468:
	mov	r0, r2
	ldmfd	sp!, {r4, r5, pc}^
L1467:
	cmp	r3, #0
	beq	L1469
	mov	ip, #0
	cmp	r4, r0
	mov	r2, r5
	mov	r0, r1
	bcc	L1471
	movne	r3, #0
	moveq	r3, #1
	cmp	r3, ip
	cmpne	r2, r0
	bcs	L1472
L1471:
	mov	ip, #1
L1472:
	and	r0, ip, #255
	ldmfd	sp!, {r4, r5, pc}^
L1469:
	mov	ip, r3
	cmp	r0, r4
	mov	r2, r1
	mov	r0, r5
	bcc	L1474
	movne	r3, #0
	moveq	r3, #1
	cmp	r3, #0
	cmpne	r2, r0
	bcs	L1475
L1474:
	mov	ip, #1
L1475:
	and	r0, ip, #255
	ldmfd	sp!, {r4, r5, pc}^
