	.arch armv8-a
	.text
	.align	2
	.p2align 3,,7
start:
    // set flags to 0100
    movz x2, #0
    adds x3, x2, x2
    movz x4, #6
    // Load from unaligned address
    adrp x0, .LANCHOR0
    add x0, x0, #1
    // Should have STAT_ADR in memory stage
    ldur x1, [x0]
    // This next instruction should NOT set the flags
    subs x3, x2, x4
	ret
	.size	start, .-start
    .data
	.align	4
	.set	.LANCHOR0,. + 0
	.type	x, %object
	.size	x, 64
x:
	.xword	100
    .xword  200
    .xword  300
    .xword  400
	.ident	"GCC: (Ubuntu/Linaro 7.5.0-3ubuntu1~18.04) 7.5.0"
	.section	.note.GNU-stack,"",@progbits
