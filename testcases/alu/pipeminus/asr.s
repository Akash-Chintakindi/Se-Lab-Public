.global	start
	.type	start, %function
start:

    // Positive
    movk 	x0, #0xFFFF
    nop
    nop
    nop
    movk 	x0, #0xFFFF, lsl 16
    nop
    nop
    nop
    movk 	x0, #0xFFFF, lsl 32
    nop
    nop
    nop
    movk 	x0, #0x7FFF, lsl 48

    nop
    nop
    nop
    asr     x1, x0, #0
    asr     x2, x0, #32
    asr     x3, x0, #63

    // Negative
    movk 	x4, #0xFFFF
    nop
    nop
    nop
    movk 	x4, #0xFFFF, lsl 16
    nop
    nop
    nop
    movk 	x4, #0xFFFF, lsl 32
    nop
    nop
    nop
    movk 	x4, #0xFFFF, lsl 48

    nop
    nop
    nop
    asr     x5, x4, #0
    asr     x6, x4, #32
    asr     x7, x4, #63
    ret