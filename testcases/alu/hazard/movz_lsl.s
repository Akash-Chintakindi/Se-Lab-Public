	.global	start
	.type	start, %function
start:

    // Populate register
    movz 	x0, #0xABCD
    lsl     x0, x0, #16

    // Should zero out 48 upper bits of x0
    movz    x0, #0xDCBA
    mvn     x5, x5
    stur    x0, [x5]

    // Try moving to different register & zeroing out upper 48 bits
    eor     x1, x1, x1
    adds     x1, x0, x1
    movz    x1, #0x1234
    ret



