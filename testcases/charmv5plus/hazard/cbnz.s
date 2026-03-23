	.global	start
	.type	start, %function
start:

	mvn	    x7, xzr

	movz	x1, 3
	cbnz	x1, .right

.wrong:
	movz	x0, 29
	b 		.end

.right:
	movz 	x0, 75

.end:
	stur	x0, [x7]
	ret

