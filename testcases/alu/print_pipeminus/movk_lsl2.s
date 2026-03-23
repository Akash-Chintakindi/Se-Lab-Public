	.global	start
	.type	start, %function
start:

    // Populate register
    movk 	x0, #0xABCD
    nop
    nop
    nop
    lsl     x0, x0, #16
    nop
    nop
    nop

    // Should keep upper bits
    movk    x0, #0xDCBA
    nop
    nop
    nop
    eor     x5, x5, x5
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

    // Try moving to different register & keeping upper bits
    eor     x1, x1, x1
    nop
    nop
    nop
    adds     x1, x0, x1
    nop
    nop
    nop
    movk    x1, #0x1234
    nop
    nop
    nop
    stur    x1, [x5]
    ret




