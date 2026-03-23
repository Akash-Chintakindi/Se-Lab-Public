	.global	start
	.type	start, %function
start:
    movz x0, #10
    adrp x1, .correct
    add x1, x1, :lo12:.correct
    sub sp, sp, #16
    stur x30, [sp]
    blr x1
.happens_once:
    ldur x30, [sp]
    add sp, sp, #16
    add x0, x0, #2 
    add x4, x4, #2
    ret
.correct:
    movz x4, #20
    ret
