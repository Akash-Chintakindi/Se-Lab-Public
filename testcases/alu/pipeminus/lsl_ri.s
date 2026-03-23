.global	start
	.type	start, %function
start:

    // Populate register
    movk 	x0, #0xABCD
    nop
    nop
    nop
    lsl     x1, x0, #16

    nop
    nop
    nop
    // Should keep upper bits
    movk    x1, #0xDCBA

    nop
    nop
    nop
    lsl     x2, x1, #16
    lsl     x3, x0, #0
    ret