	.arch armv8-a
    .text
	.global	main
	.type	main, %function
main:
        sub     sp, sp, 32
        stur    x29, [sp]
        stur    x30, [sp, 8]
        mov     x29, sp
        stur    x19, [sp, 16]

        adrp    x0, data
        add     x0, x0, :lo12:data
        movz    x1, 0
        movz    x2, 4096
        subs    x1, x1, x2
        ands    x0, x0, x1      //Aligns mem pointer to PAGESIZE
        adds    x0, x0, x2

        stur    x0, [sp, 24]
        mov     x1, 128        // Memory size used in bytes
        bl      init_data_fast
        ldur    x0, [sp, 24]
        bl      chase_indices
        movz    x0, 0

        ldur     x19, [sp, 16]
        ldur     x29, [sp]
        ldur     x30, [sp, 8]
        add      sp, sp, 32
        ret
init_data_fast:
        cmp     x1, xzr
        beq    .L2
        movz    x2, 0
        sub     x3, x0, 8
.L3:
        add     x2, x2, 1
        lsl     x4, x2, 3
        adds    x4, x3, x4
        stur    x2, [x4]
        cmp     x2, x1
        bne     .L3
.L2:
        sub     x1, x1, 1
        lsl     x4, x1, 3
        adds    x4, x0, x4
        stur    xzr, [x4]
        ret
chase_indices:
        movz    x2, 0
        movz    x1, 0
        movz    x5, 2047
.L6:
        add     x2, x2, 1

        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]

        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        lsl     x4, x1, 3
        adds    x4, x0, x4
        ldur    x1, [x4]
        
        cmp     x2, x5
        bls     .L6
        cmp     x1, xzr
        bne     .L6
        ret

	.size	main, .-main
  .data
  .align 8
  .type mem, %object
  .size mem, 8
mem:
        .zero   8

.type data, %object
.size data, 2129920
data:
        .zero   2129920         // Maximum memory size usable in bytes (2^emax + 4096) << 3
    .ident  "GCC: (Ubuntu/Linaro 7.5.0-3ubuntu1~18.04) 7.5.0"
    .section    .note.GNU-stack,"",@progbits