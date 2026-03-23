.global	start
	.type	start, %function
start:

    // Populate register
    movk 	x0, #0x0123
    movk 	x0, #0x4567, lsl 16
    movk 	x0, #0x89AB, lsl 32
    movk 	x0, #0xCDEF, lsl 48
    mov     x7, #16
    lsr     x1, x0, x7

    // Should shift by 16
    mov     x7, #80
    lsr     x2, x1, x7
    ret