/**************************************************************************
 * C S 429 architecture emulator
 *
 * instr_Writeback.c - Writeback stage of instruction processing pipeline.
 **************************************************************************/

#include "instr.h"
#include "instr_pipeline.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

extern int64_t W_wval;

/*
 * Write-back stage logic.
 * STUDENT TO-DO:
 * Implement the writeback stage.
 *
 * Use in as the input pipeline register.
 *
 * You will need the global variable W_wval.
 */
comb_logic_t wback_instr(w_instr_impl_t *in) {
    // Select write value: wval_sel=1 means use val_mem (LDUR), else val_ex (ALU)
    W_wval = (int64_t)(in->W_sigs.wval_sel ? in->val_mem : in->val_ex);
    return;
}
