/**************************************************************************
 * C S 429 system emulator
 *
 * Bubble and stall checking logic.
 * STUDENT TO-DO:
 * Implement logic for hazard handling.
 *
 * handle_hazards is called from proc.c with the appropriate
 * parameters already set, you must implement the logic for it.
 *
 * You may optionally use the other three helper functions to
 * make it easier to follow your logic.
 **************************************************************************/

#include "hazard_control.h"
#include "hw_elts.h"
#include "machine.h"
#include "mem.h"

extern machine_t guest;
extern mem_status_t dmem_status;
extern bool deassert_flags;

/* Use this method to actually bubble/stall a pipeline stage.
 * Call it in handle_hazards(). Do not modify this code. */
void pipe_control_stage(proc_stage_t stage, bool bubble, bool stall) {
    pipe_reg_t *pipe;
    switch (stage) {
        case S_FETCH:
            pipe = F_instr;
            break;
        case S_DECODE:
            pipe = D_instr;
            break;
        case S_EXECUTE:
            pipe = X_instr;
            break;
        case S_MEMORY:
            pipe = M_instr;
            break;
        case S_WBACK:
            pipe = W_instr;
            break;
        default:
            printf("Error: incorrect stage provided to pipe control.\n");
            return;
    }
    if (bubble && stall) {
        printf("Error: cannot bubble and stall at the same time.\n");
        pipe->ctl = P_ERROR;
    }
    // If we were previously in an error state, stay there.
    if (pipe->ctl == P_ERROR)
        return;

    if (bubble) {
        pipe->ctl = P_BUBBLE;
    } else if (stall) {
        pipe->ctl = P_STALL;
    } else {
        pipe->ctl = P_LOAD;
    }
}


bool check_ret_hazard(opcode_t D_opcode) {
    return D_opcode == OP_RET;
}

#ifdef EC
bool check_br_hazard(opcode_t D_opcode) {
    // Student TODO
    return false;
}

bool check_cb_hazard(opcode_t D_opcode, uint64_t D_val_a) {
    // Student TODO
    return false;
}
#endif

bool check_mispred_branch_hazard(opcode_t X_opcode, bool X_condval) {
    return X_opcode == OP_B_COND && !X_condval;
}

bool check_load_use_hazard(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2,
                           opcode_t X_opcode, uint8_t X_dst) {
    return X_opcode == OP_LDUR && X_dst != XZR_NUM &&
           (X_dst == D_src1 || X_dst == D_src2);
}

bool error(stat_t status) {
    return status == STAT_INS || status == STAT_ADR;
}

comb_logic_t handle_hazards(opcode_t D_opcode, uint8_t D_src1, uint8_t D_src2,
                            uint64_t D_val_a, opcode_t X_opcode, uint8_t X_dst,
                            bool X_condval) {
    /* Students: Change this code */
    // This will need to be updated in week 2, good enough for week 1
#ifdef PIPE
    bool f_stall = F_out->status == STAT_HLT || F_out->status == STAT_INS;
    bool ret_from_main = (D_opcode == OP_RET && D_val_a == RET_FROM_MAIN_ADDR);
    bool ret = check_ret_hazard(D_opcode) && !ret_from_main;
    bool mispred = check_mispred_branch_hazard(X_opcode, X_condval);
    bool load_use = check_load_use_hazard(D_opcode, D_src1, D_src2, X_opcode, X_dst);

    // F: stall on HLT/INS, load-use, or RET; bubble on ret-from-main (if not stalled)
    pipe_control_stage(S_FETCH, ret_from_main && !f_stall, f_stall || load_use || ret);
    // D: stall on load-use; bubble on RET (non-load-use), misprediction, or ret-from-main
    //    (ret-from-main must also drop the erroneous sequential fetch in D_in)
    pipe_control_stage(S_DECODE, (ret && !load_use) || mispred || ret_from_main, load_use);
    // X: bubble on load-use or misprediction
    pipe_control_stage(S_EXECUTE, load_use || mispred, false);
    pipe_control_stage(S_MEMORY, false, false);
    pipe_control_stage(S_WBACK, false, false);

    // Suppress NZCV updates from instructions behind an excepting instruction.
    // W_in was just written by memory_instr; if it has exception status,
    // any flag-setting instruction in X should not update NZCV.
    if (W_in->status == STAT_INS || W_in->status == STAT_ADR ||
        M_out->status == STAT_INS || M_out->status == STAT_ADR) {
        deassert_flags = true;
    }
#else
    bool f_stall = F_out->status == STAT_HLT || F_out->status == STAT_INS;
    bool ret_from_main = (D_opcode == OP_RET && D_val_a == RET_FROM_MAIN_ADDR);
    bool ret_normal = (D_opcode == OP_RET && D_val_a != RET_FROM_MAIN_ADDR);
    pipe_control_stage(S_FETCH, ret_from_main && !f_stall, f_stall);
    pipe_control_stage(S_DECODE, ret_normal, false);
    pipe_control_stage(S_EXECUTE, false, false);
    pipe_control_stage(S_MEMORY, false, false);
    pipe_control_stage(S_WBACK, false, false);
#endif
}
