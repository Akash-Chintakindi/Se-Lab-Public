/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Fetch.c - Fetch stage of instruction processing pipeline.
 **************************************************************************/

#include "hw_elts.h"
#include "instr.h"
#include "instr_pipeline.h"
#include "machine.h"
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
    // Student TODO
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

    // Student TODO: Comment this line back in and fill in parameters
    // select_PC();
    
    /*
     * Students: This case is for generating HLT instructions
     * to stop the pipeline. Only write your code in the **else** case.
     */
    if (!current_PC || F_in->status == STAT_HLT) {
        out->insnbits = 0xD4400000U;
        out->op = OP_HLT;
        out->print_op = OP_HLT;
        out->format = ftable[out->op];
        imem_err = false;
    } else {
        // Student TODO

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

    return;
}
