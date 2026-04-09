/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Memory.c - Memory stage of instruction processing pipeline.
 **************************************************************************/

#include "hw_elts.h"
#include "instr.h"
#include "instr_pipeline.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

extern uint64_t M_PC;

extern comb_logic_t copy_w_ctl_sigs(w_ctl_sigs_t *, w_ctl_sigs_t *);

/*
 * Memory stage logic.
 * STUDENT TO-DO:
 * Implement the memory stage.
 *
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 *
 * You will need the following helper functions:
 * copy_w_ctl_signals and dmem.
 */
comb_logic_t memory_instr(m_instr_impl_t *in, w_instr_impl_t *out) {
    // Pass through metadata
    out->op       = in->op;
    out->print_op = in->print_op;
    out->status   = in->status;
    out->dst      = in->dst;
    out->val_ex   = in->val_ex;

    // Copy writeback control signals
    copy_w_ctl_sigs(&out->W_sigs, &in->W_sigs);

    // Store M_PC for exception handling (PC of excepting instruction)
    M_PC = in->multipurpose_val.seq_succ_PC;

    // Access data memory if needed
    bool     dmem_err = false;
    uint64_t dmem_rval = 0;

    dmem(in->val_ex,          // address (computed by ALU)
         in->val_b,           // write value (for STUR)
         in->M_sigs.dmem_read,
         in->M_sigs.dmem_write,
         &dmem_rval,
         &dmem_err);

    out->val_mem = dmem_rval;

    // Set bad address status if memory access failed
    if (dmem_err) {
        out->status = STAT_ADR;
    }

    return;
}
