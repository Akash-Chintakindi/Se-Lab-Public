// ################################################
// ###          STUDENTS: DO NOT MODIFY         ###
// ################################################

.arch armv8-a
.text
.align 2
.global start
.type start, %function

.macro CALL_ABS_SUM array_name
    # Get pointer to array.
    adrp    x0, \array_name
    add     x0, x0, :lo12:\array_name

    # Load array size.
    adrp    x1, \array_name\()_size
    add     x1, x1, :lo12:\array_name\()_size
    ldur    x1, [x1]

    bl      abs_sum             // Call student function.

    movz    x1, #0              // Print result.
    mvn     x1, x1
    stur    x0, [x1]
.endm

start:
    CALL_ABS_SUM array1
    CALL_ABS_SUM array2
    CALL_ABS_SUM array3

    movz    x30, #0             // Ensure we HLT by returning to address 0.
    ret
.size start, .-start
