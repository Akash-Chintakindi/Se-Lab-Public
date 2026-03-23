	.arch armv8-a
	.file	"c_template.c"
	.text
	.align	2
	.global	add_two_nums
	.type	add_two_nums, %function
add_two_nums:
	adds	x0, x0, x1
	nop
	nop
	nop
	ret
	nop
	nop
	nop
	.size	add_two_nums, .-add_two_nums
	.align	2
	.global	start
	.type	start, %function
start:
	add  sp, sp, #-16 
	stur x9, [sp, 0]  
	stur x30, [sp, 8]
	nop
	nop
	nop
	mov	x29, sp
	nop
	nop
	nop
	mov	x1, 1000
	nop
	nop
	nop
	mov	x0, 100
	nop
	nop
	nop
	bl	add_two_nums
	nop
	nop
	nop
	mov	x1, -1
	nop
	nop
	nop
	stur	x0, [x1]
	nop
	nop
	nop
	ldur x9, [sp, 0]  
	ldur x30, [sp, 8]
	add  sp, sp, #16 
	nop
	nop
	nop
	ret
	nop
	nop
	nop
.LFE1:
	.size	start, .-start
	.ident	"GCC: (Ubuntu 13.2.0-23ubuntu4) 13.2.0"
	.section	.note.GNU-stack,"",@progbits
