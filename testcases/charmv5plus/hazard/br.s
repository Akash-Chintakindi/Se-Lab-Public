	.global	start
	.type	start, %function
start:
    movz x0, #10
    adrp x1, .correct
    add x1, x1, :lo12:.correct
    br x1
.wrong:
    movz x2, #0xBAD  
    movz x3, #5
    ret
.correct:
    movz x4, #20
    ret
