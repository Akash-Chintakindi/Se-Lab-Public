.global	start
	.type	start, %function
start:

    // Populate register
    movk 	x0, #0xABCD
    mov     x7, #16
    nop
    nop
    nop
    lsl     x1, x0, x7

    nop
    nop
    nop
    // Should keep upper bits
    movk    x1, #0xDCBA

    // Should shift by 16
    mov     x7, #80
    nop
    nop
    nop
    lsl     x2, x1, x7
    ret