	.arch armv8-a
	.text
	.align	2
	.p2align 3,,7
start:
    // Simple tests for the cmn instruction.
    movz x0, #1
    movz x1, #0
    nop
    nop
    nop
    mvn x1, x1
    add x2, x2, #1
    nop
    nop
    nop
    cmn x1, x0
	ret
	.size	start, .-start
	.ident	"GCC: (Ubuntu/Linaro 7.5.0-3ubuntu1~18.04) 7.5.0"
	.section	.note.GNU-stack,"",@progbits
