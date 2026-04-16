/**************************************************************************
 * C S 429 system emulator
 *
 * forward.c - The value forwarding logic in the pipelined processor.
 **************************************************************************/

#include "forward.h"
#include "hw_elts.h"
#include <stdbool.h>

/* STUDENT TO-DO:
 * Implement forwarding register values from
 * execute, memory, and writeback back to decode.
 */
comb_logic_t forward_reg(uint8_t D_src1, uint8_t D_src2, uint8_t X_dst,
                         uint8_t M_dst, uint8_t W_dst, uint64_t X_val_ex,
                         uint64_t M_val_ex, uint64_t M_val_mem,
                         uint64_t W_val_ex, uint64_t W_val_mem, bool M_wval_sel,
                         bool W_wval_sel, bool X_w_enable, bool M_w_enable,
                         bool W_w_enable, uint64_t *val_a, uint64_t *val_b) {

#ifdef PIPE
    // Forward val_a (D_src1): priority X > M > W
    if (X_w_enable && X_dst == D_src1 && D_src1 != XZR_NUM) {
        *val_a = X_val_ex;
    } else if (M_w_enable && M_dst == D_src1 && D_src1 != XZR_NUM) {
        *val_a = M_wval_sel ? M_val_mem : M_val_ex;
    } else if (W_w_enable && W_dst == D_src1 && D_src1 != XZR_NUM) {
        *val_a = W_wval_sel ? W_val_mem : W_val_ex;
    }

    // Forward val_b (D_src2): priority X > M > W
    if (X_w_enable && X_dst == D_src2 && D_src2 != XZR_NUM) {
        *val_b = X_val_ex;
    } else if (M_w_enable && M_dst == D_src2 && D_src2 != XZR_NUM) {
        *val_b = M_wval_sel ? M_val_mem : M_val_ex;
    } else if (W_w_enable && W_dst == D_src2 && D_src2 != XZR_NUM) {
        *val_b = W_wval_sel ? W_val_mem : W_val_ex;
    }
#endif
    return;
}
