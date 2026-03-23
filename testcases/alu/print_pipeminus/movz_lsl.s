	.global	start
	.type	start, %function
start:

    // Populate register
    movz 	x0, #0xABCD
    nop
    nop
    nop
    lsl     x0, x0, #16
    nop
    nop
    nop

    // Should zero out 48 upper bits of x0
    movz    x0, #0xDCBA
    nop
    nop
    nop
    mvn     x5, x5
    nop
    nop
    nop
    stur    x0, [x5]
    nop
    nop
    nop

    // Try moving to different register & zeroing out upper 48 bits
    eor     x1, x1, x1
    nop
    nop
    nop
    adds     x1, x0, x1
    nop
    nop
    nop
    movz    x1, #0x1234
    ret





