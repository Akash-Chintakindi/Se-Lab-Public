	.global	start
	.type	start, %function
start:

    // Populate register
    movk 	x0, #0xABCD
    lsl     x0, x0, #16

    // Should keep upper bits
    movk    x0, #0xDCBA
    eor     x5, x5, x5
    mvn     x5, x5
    stur    x0, [x5]

    // Try moving to different register & keeping upper bits
    eor     x1, x1, x1
    adds     x1, x0, x1
    movk    x1, #0x1234
    stur    x1, [x5]
    ret


