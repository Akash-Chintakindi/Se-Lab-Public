	.arch armv8-a
	.text
	.align	2
start:

    movz x0, #0x7000, lsl #48
    movz x1, #0x4000, lsl #48
    nop
    nop
    nop
    cmn x0, x1
    nop
    nop
    nop
    nop
    b.vs .willoverflow

.goback:

    nop
    nop
    nop

    // Print x0
    eor 	x5, x5, x5
    nop
    nop
    nop
	mvn 	x5, x5
    nop
    nop
    nop
    //correct value is 7000000000000019
    //incorrect value is 7000000000000000
    //NZCV should be N, V (overflow when adding +s, resulting in negative)
	stur	x0, [x5]
	ret
	
.willoverflow:
    nop
    nop
    nop
    add x0, x0, #13
    nop
    nop
    nop
    add x0, x0, #12
    nop
    nop
    nop
    b .goback

    .size	start, .-start
	.ident	"GCC: (Ubuntu/Linaro 7.5.0-3ubuntu1~18.04) 7.5.0"
	.section	.note.GNU-stack,"",@progbits
