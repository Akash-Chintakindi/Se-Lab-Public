/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Execute.c - Execute stage of instruction processing pipeline.
 **************************************************************************/

#include "hw_elts.h"
#include "instr.h"
#include "instr_pipeline.h"
#include "machine.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

extern machine_t guest;

extern comb_logic_t copy_m_ctl_sigs(m_ctl_sigs_t *, m_ctl_sigs_t *);
extern comb_logic_t copy_w_ctl_sigs(w_ctl_sigs_t *, w_ctl_sigs_t *);

extern uint8_t X_nzcvval;
extern bool X_set_flags;

/*
 * Execute stage logic.
 * STUDENT TO-DO:
 * Implement the execute stage.
 *
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 *
 * You will need the following helper functions:
 * copy_m_ctl_signals, copy_w_ctl_signals, and alu.
 */
comb_logic_t execute_instr(x_instr_impl_t *in, m_instr_impl_t *out) {
    // Pass through metadata
    out->op       = in->op;
    out->print_op = in->print_op;
    out->status   = in->status;
    out->multipurpose_val.seq_succ_PC = in->multipurpose_val.seq_succ_PC;
    out->dst      = in->dst;
    out->val_b    = in->val_b;

    // Copy control signals to M and W stages
    copy_m_ctl_sigs(&out->M_sigs, &in->M_sigs);
    copy_w_ctl_sigs(&out->W_sigs, &in->W_sigs);

    // Select val_a: vala_sel=1 uses multipurpose_val (for BL/ADRP), else val_a
    uint64_t alu_vala = in->X_sigs.vala_sel
                      ? in->multipurpose_val.seq_succ_PC
                      : in->val_a;

    // Select val_b: valb_sel=1 uses register val_b, else immediate val_imm
    uint64_t alu_valb = in->X_sigs.valb_sel
                      ? in->val_b
                      : (uint64_t)in->val_imm;

    // Run the ALU
    uint64_t val_ex  = 0;
    bool     cond_holds = false;
    uint8_t  nzcv_out   = 0;

    alu(alu_vala, alu_valb, in->val_hw,
        guest.proc->NZCV,
        in->ALU_op, in->X_sigs.set_flags, in->cond,
        &val_ex, &cond_holds, &nzcv_out);

    out->val_ex    = val_ex;
    out->cond_holds = cond_holds;

    // Update global wires for NZCV
    X_nzcvval  = nzcv_out;
    X_set_flags = in->X_sigs.set_flags;

    return;
}
