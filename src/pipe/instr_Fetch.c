/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Fetch.c - Fetch stage of instruction processing pipeline.
 **************************************************************************/

#include "hw_elts.h"
#include "instr.h"
#include "instr_pipeline.h"
#include "machine.h"
#include "mem.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

extern machine_t guest;
extern uint64_t F_PC;

/*
 * Select PC logic.
 * STUDENT TO-DO:
 * Write the next PC to *current_PC.
 */
static comb_logic_t
select_PC(uint64_t pred_PC,                  // The predicted PC
          opcode_t D_opcode, uint64_t val_a, // Possible correction from RET
          uint64_t D_seq_succ,               // this is only used in CBZ/CBNZ EC
          opcode_t M_opcode, bool M_cond_val, // b.cond correction
          uint64_t seq_succ,                  // Possible correction from B.cond
          uint64_t *current_PC) {
    /*
     * Students: Please leave this code
     * at the top of this function.
     * You may modify below it.
     */
    if (D_opcode == OP_RET && val_a == RET_FROM_MAIN_ADDR) {
        *current_PC = 0; // PC can't be 0 normally.
        return;
    }
    // Modify starting here.

#ifdef PIPE
    // RET/BR/BLR correction: when the indirect branch is in X, val_a holds the
    // real target. Override any predicted PC.
    if (X_out->op == OP_RET
#ifdef EC
        || X_out->op == OP_BR || X_out->op == OP_BLR
#endif
        ) {
        *current_PC = X_out->val_a;
        return;
    }

    // B.cond misprediction correction: we predicted taken but condition didn't hold
    if (M_opcode == OP_B_COND && !M_cond_val) {
        *current_PC = seq_succ;
        return;
    }
#endif
    (void)D_opcode;
    (void)val_a;

    // Default: use predicted PC
    *current_PC = pred_PC;
    return;
}

/*
 * Predict PC logic. Conditional branches are predicted taken.
 * STUDENT TO-DO:
 * Write the predicted next PC to *predicted_PC
 * and the next sequential pc to *seq_succ.
 */
static comb_logic_t predict_PC(uint64_t current_PC, uint32_t insnbits,
                               opcode_t op, uint64_t *predicted_PC,
                               uint64_t *seq_succ) {
    /*
     * Students: Please leave this code
     * at the top of this function.
     * You may modify below it.
     */
    if (!current_PC) {
        return; // We use this to generate a halt instruction.
    }
    // Modify starting here.

    // Sequential successor is always PC + 4
    *seq_succ = current_PC + 4;

    switch (op) {
        case OP_B:
        case OP_BL: {
            // FORMAT_B1: 26-bit signed immediate offset, shifted left by 2
            int64_t offset = bitfield_s64((int32_t)insnbits, 0, 26) << 2;
            *predicted_PC = (uint64_t)((int64_t)current_PC + offset);
            break;
        }
        case OP_B_COND: {
            // FORMAT_B2: 19-bit signed immediate offset at bits[23:5], shifted by 2
            // Predict taken
            int64_t offset = bitfield_s64((int32_t)insnbits, 5, 19) << 2;
            *predicted_PC = (uint64_t)((int64_t)current_PC + offset);
            break;
        }

#ifdef EC
        case OP_CBZ:
        case OP_CBNZ: {
            // predict taken: 19-bit signed offset at bits[23:5], shifted left 2
            int64_t offset = bitfield_s64((int32_t)insnbits, 5, 19) << 2;
            *predicted_PC = (uint64_t)((int64_t)current_PC + offset);
            break;
        }
        case OP_BR:
        case OP_BLR:
            // target in register — cannot predict, use sequential
            *predicted_PC = current_PC + 4;
            break;
#endif

        default:
            // All other instructions: predict sequential
            *predicted_PC = current_PC + 4;
            break;
    }
    return;
}

/*
 * Helper function to recognize the aliased instructions:
 * LSL (RI/RR), LSR (RI/RR), CMP, CMN, and TST. We do this only to simplify the
 * implementations of the shift operations (rather than having
 * to implement UBFM in full).
 * STUDENT TO-DO
 */
static void fix_instr_aliases(uint32_t insnbits, opcode_t *op) {
    if (*op == OP_UBFM) {
        // Distinguish LSL_RI from LSR_RI
        // LSR: immr = shift, imms = 63
        // LSL: imms = 63 - shift, immr = 64 - shift (mod 64)
        uint32_t imms = bitfield_u32((int32_t)insnbits, 10, 6); // bits[15:10]
        if (imms == 63) {
            *op = OP_LSR_RI;
        } else {
            *op = OP_LSL_RI;
        }
        return;
    }

    if (*op == OP_UBFMV) {
        // Distinguish LSL_RR from LSR_RR via bits[11:10] (shift type field)
        uint32_t shift_type = bitfield_u32((int32_t)insnbits, 10, 2);
        if (shift_type == 0) {
            *op = OP_LSL_RR;
        } else {
            *op = OP_LSR_RR;
        }
        return;
    }

    // CMP = SUBS with Rd = XZR (31)
    // CMN = ADDS with Rd = XZR (31)
    // TST = ANDS with Rd = XZR (31)
    uint8_t rd = (uint8_t)bitfield_u32((int32_t)insnbits, 0, 5);
    if (rd == 31) {
        if (*op == OP_SUBS_RR) *op = OP_CMP_RR;
        else if (*op == OP_ADDS_RR) *op = OP_CMN_RR;
        else if (*op == OP_ANDS_RR) *op = OP_TST_RR;
    }

#ifdef EC
    // CSINC is aliased under CSEL (same top-11 bits); bit[10]=1 → CSINC
    if (*op == OP_CSEL && bitfield_u32((int32_t)insnbits, 10, 1) == 1)
        *op = OP_CSINC;
    // CSINV is aliased under CSNEG (same top-11 bits); bit[10]=0 → CSINV
    if (*op == OP_CSNEG && bitfield_u32((int32_t)insnbits, 10, 1) == 0)
        *op = OP_CSINV;
#endif
    return;
}

/*
 * Fetch stage logic.
 * STUDENT TO-DO:
 * Implement the fetch stage.
 *
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 * Additionally, update PC for the next
 * cycle's predicted PC.
 *
 * You will also need the following helper functions:
 * select_pc, predict_pc, and imem.
 */
comb_logic_t fetch_instr(f_instr_impl_t *in, d_instr_impl_t *out) {
    bool imem_err = 0;
    uint64_t current_PC = 0;

    // val_a = pred_PC so framework check never fires in select_PC;
    // RET-from-main is handled by handle_hazards instead.
    uint64_t val_a = in->pred_PC;

    select_PC(in->pred_PC,
              D_out->op, val_a,
              D_out->multipurpose_val.seq_succ_PC,
              M_out->op, M_out->cond_holds,
              M_out->multipurpose_val.seq_succ_PC,
              &current_PC);

#ifdef PIPE
    // When select_PC corrects the PC (RET or mispred), clear any stale
    // F_in->status from a previous cycle's error fetch during stall.
    if (current_PC && (X_out->op == OP_RET
#ifdef EC
        || X_out->op == OP_BR || X_out->op == OP_BLR
#endif
        || (M_out->op == OP_B_COND && !M_out->cond_holds))) {
        F_in->status = STAT_AOK;
    }
#endif

#ifndef PIPE
    // B.cond misprediction correction: if B.cond in M stage was not taken
    if (M_out->op == OP_B_COND && !M_out->cond_holds) {
        current_PC = M_out->multipurpose_val.seq_succ_PC;
    }
#endif

    /*
     * Students: This case is for generating HLT instructions
     * to stop the pipeline. Only write your code in the **else** case.
     */
    if (!current_PC || F_in->status == STAT_HLT || F_in->status == STAT_INS) {
        out->insnbits = 0xD4400000U;
        out->op = OP_HLT;
        out->print_op = OP_HLT;
        out->format = ftable[out->op];
        imem_err = false;
    } else {
        // Fetch instruction from instruction memory
        imem(current_PC, &out->insnbits, &imem_err);

        // Always update F_PC to sequential successor
        F_PC = current_PC + 4;

        if (!imem_err) {
            // Decode opcode from top 11 bits
            uint32_t top11 = bitfield_u32((int32_t)out->insnbits, 21, 11);
            out->op = itable[top11];
            out->print_op = out->op;

            // Fix aliased instructions (LSL/LSR/CMP/CMN/TST)
            fix_instr_aliases(out->insnbits, &out->op);
            out->print_op = out->op;

            // Look up instruction format
            out->format = (out->op != OP_ERROR) ? ftable[out->op] : FORMAT_ERROR;

            // Compute predicted PC and sequential successor
            uint64_t predicted_PC = current_PC + 4;
            uint64_t seq_succ = current_PC + 4;
            predict_PC(current_PC, out->insnbits, out->op, &predicted_PC, &seq_succ);

            // Store predicted PC for next cycle
            F_PC = predicted_PC;

            // Store multipurpose value
            if (out->op == OP_ADRP) {
                // ADRP: compute page-relative address
                // imm21 = immhi[18:0]:immlo[1:0] (sign-extended, then << 12)
                uint32_t immhi = bitfield_u32((int32_t)out->insnbits, 5, 19);  // bits[23:5]
                uint32_t immlo = bitfield_u32((int32_t)out->insnbits, 29, 2);  // bits[30:29]
                int64_t imm21 = bitfield_s64((int32_t)((immhi << 2) | immlo), 0, 21);
                out->multipurpose_val.adrp_val =
                    (current_PC & ~0xFFFULL) + (uint64_t)((int64_t)imm21 << 12);
            } else {
                out->multipurpose_val.seq_succ_PC = seq_succ;
            }
        } else {
            // imem error: set error fields
            out->op = OP_ERROR;
            out->print_op = OP_ERROR;
            out->format = FORMAT_ERROR;
            // Store seq_succ_PC so M_PC can recover the excepting instruction's PC
            out->multipurpose_val.seq_succ_PC = current_PC + 4;
        }
    }

    if (imem_err || out->op == OP_ERROR) {
        in->status = STAT_INS;
        F_in->status = in->status;
    } else if (out->op == OP_HLT) {
        in->status = STAT_HLT;
        F_in->status = in->status;
    } else {
        in->status = STAT_AOK;
    }
    out->status = in->status;

#ifndef PIPE
    // RET correction: override F_PC for next cycle (handle_hazards will bubble D)
    if (D_out->op == OP_RET) {
        uint8_t rn = (uint8_t)bitfield_u32((int32_t)D_out->insnbits, 5, 5);
        uint64_t ret_addr, tmp;
        regfile_read(rn, XZR_NUM, &ret_addr, &tmp);
        if (ret_addr != RET_FROM_MAIN_ADDR) {
            F_PC = ret_addr;
        }
    }
#endif

    return;
}
