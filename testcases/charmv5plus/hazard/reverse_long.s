	.global	start
start:

// register allocation:
// x0 = input (and later on, the result, persistent)
// x1 = r (the result, persistent)
// x2 = lut (lookup table, persistent)
// x3 = i (loop counter, persistent)
// x4 = index (temporary)
// x5 = jump address to handle switch (temporary)
// x6 = AND mask (persistent)

// load the magic number, the LUT, into x2
	mov		x2, 50304
	movk	x2, 0xe6a2, lsl 16
	movk	x2, 0xd591, lsl 32
	movk	x2, 0xf7b3, lsl 48

// load AND mask into x6
	movz	x6, 15

// initialize r and i to 0
	eor x3, x3, x3 // i
	eor x1, x1, x1 // r
	b	.loop_guard


.loop_body:
// loop body:
// left shift r by 4
	lsl	x1, x1, 4
// AND input with 0xF to get an index
	ands	x4, x0, x6
// get base address of JT into x5
	adrp	x5, .jump_table
	add	x5, x5, :lo12:.jump_table
// increment this offset to get the address of the JT entry
// entry_addr = x5 + 4 * x4
	lsl x4, x4, 3
	adds x5, x5, x4
// now x5 stores the address of the JT entry that has our jump vector
// load this vector, then jump to it
	ldur x5, [x5]
	br	x5
.Lrtx5:
	.section	.rodata
	.align	0
	.align	2
.jump_table:
	.dword	(.case_0)
	.dword	(.case_1)
	.dword	(.case_2)
	.dword	(.case_3)
	.dword	(.case_4)
	.dword	(.case_5)
	.dword	(.case_6)
	.dword	(.case_7)
	.dword	(.case_8)
	.dword	(.case_9)
	.dword	(.case_10)
	.dword	(.case_11)
	.dword	(.case_12)
	.dword	(.case_13)
	.dword	(.case_14)
	.dword	(.case_15)
	.text
.case_0:
	b	.post_switch
.case_1:
	lsr	x4, x2, 4
	b	.post_switch
.case_2:
	lsr	x4, x2, 8
	b	.post_switch
.case_3:
	lsr	x4, x2, 12
	b	.post_switch
.case_4:
	lsr	x4, x2, 16
	b	.post_switch
.case_5:
	lsr	x4, x2, 20
	b	.post_switch
.case_6:
	lsr	x4, x2, 24
	b	.post_switch
.case_7:
	lsr	x4, x2, 28
	b	.post_switch
.case_8:
	lsr	x4, x2, 32
	b	.post_switch
.case_9:
	lsr	x4, x2, 36
	b	.post_switch
.case_10:
	lsr	x4, x2, 40
	b	.post_switch
.case_11:
	lsr	x4, x2, 44
	b	.post_switch
.case_12:
	lsr	x4, x2, 48
	b	.post_switch
.case_13:
	lsr	x4, x2, 52
	b	.post_switch
.case_14:
	lsr	x4, x2, 56
	b	.post_switch
.case_15:
	lsr	x4, x2, 60
	b 	.post_switch
.post_switch:
	ands x4, x4, x6
	orr x1, x1, x4
// shift input right by 4
	lsr	x0, x0, 4
// increment i
	add	x3, x3, 1
.loop_guard:
// load i, compare it to 15
	cmp	x3, x6
// if i is LOWER OR SAME than 15, execute the loop body
	bls	.loop_body
// otherwise load r in x0 (return value) and return
	adds x0, x1, xzr
	ret
.LFE0:
	.size	start, .-start
	.ident	"GCC: (Ubuntu 13.2.0-23ubuntu4) 13.2.0"
	.section	.note.GNU-stack,"",@progbits
