	.global	start
	.type	start, %function
start:

	mvn	    x7, xzr

	movz	x1, 4
	movz	x2, 3
	cmp		x1, x2
	csel 	x0, x1, x2, gt
	stur	x0, [x7]
	cmp		x1, x2
	csel 	x0, x1, x2, le
	stur	x0, [x7]
	cmp		x1, x2
	csel 	x0, x1, x2, le
	stur	x0, [x7]
	cmp		x1, x2
	csel 	x0, x1, x2, gt
	stur	x0, [x7]
	


	ret

