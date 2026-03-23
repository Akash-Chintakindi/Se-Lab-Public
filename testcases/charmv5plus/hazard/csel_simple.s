	.global	start
	.type	start, %function
start:

	movz	x1, 4
	nop
	nop
	nop
	movz	x2, 3
	nop
	nop
	nop
	cmp		x1, x2
	nop
	nop
	nop
	csel 	x0, x1, x2, gt
	nop
	nop
	nop
	b		ret
	nop
	nop
	nop


ret:
	mvn	    x1, xzr
	nop
	nop
	nop
	stur	x0, [x1]
	nop
	nop
	nop
	ret

