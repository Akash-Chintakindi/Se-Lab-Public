.arch armv8-a
.text
.align 2
.global abs_sum
.type abs_sum, %function

// Compute the sum of the absolute value of values in the array.
// x0: address of long array[32]
// x1: size of array (32)
abs_sum:
    // Modify below here
    movz    x2, #0              // Running sum.
.loop:
    ldur    x3, [x0, #0]        // Load element from memory.
    add     x0, x0, #8          // Advance pointer (fills load-use slot; no dep on x3).
    sub     x1, x1, #1          // Decrement counter.
    asr     x4, x3, #63         // Sign mask: all 1s if x3<0, all 0s if x3>=0.
    eor     x3, x3, x4          // One's complement if negative.
    subs    x3, x3, x4          // Complete two's complement negate (flags overwritten by cmp).
    adds    x2, x2, x3          // Accumulate absolute value.
    cmp     x1, xzr             // Set flags for branch.
    b.ne    .loop               // Repeat if elements remain (predicted taken = correct).
.done:
    adds    x0, xzr, x2         // Move sum into x0.
    ret
.size   abs_sum, .-abs_sum
