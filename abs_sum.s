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
    lsl     x5, x1, #3          // x5 = size * 8.
    adds    x5, x5, x0          // x5 = end_ptr.
    sub     x6, x5, #16         // x6 = last safe pos for 2-load (end_ptr - 16).
    cmp     x0, x6
    b.gt    .tail               // Skip main loop if fewer than 2 elements.
.loop2:
    ldur    x3, [x0, #0]        // Element A.
    ldur    x7, [x0, #8]        // Element B.
    add     x0, x0, #16         // Advance pointer by 2 elems (fills load-use slot).
    asr     x4, x3, #63         // Sign mask A.
    asr     x8, x7, #63         // Sign mask B.
    eor     x3, x3, x4          // One's complement A.
    eor     x7, x7, x8          // One's complement B.
    subs    x3, x3, x4          // Complete abs A.
    subs    x7, x7, x8          // Complete abs B.
    adds    x2, x2, x3          // Accumulate A.
    adds    x2, x2, x7          // Accumulate B.
    cmp     x0, x6              // Still >= 2 elements left?
    b.le    .loop2              // Predicted taken = correct while looping.
.tail:
    cmp     x0, x5              // Any single remainder element?
    b.ge    .done               // Even sizes: branch taken (correctly predicted).
    ldur    x3, [x0, #0]        // Odd size: process final element.
    asr     x4, x3, #63
    eor     x3, x3, x4
    subs    x3, x3, x4
    adds    x2, x2, x3
.done:
    adds    x0, xzr, x2         // Move sum into x0.
    ret
.size   abs_sum, .-abs_sum
