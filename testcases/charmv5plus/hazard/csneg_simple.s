	.global	start
	.type	start, %function
start:

	movz	x1, 4
	movz	x2, 3
	cmp		x1, x2
	csneg 	x0, x1, x2, gt
	b		ret


ret:
	mvn	    x1, xzr
	stur	x0, [x1]
	ret

