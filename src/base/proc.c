/**************************************************************************
 * STUDENTS: DO NOT MODIFY.
 *
 * C S 429 system emulator
 *
 * proc.c - The top-level instruction processing loop of the processor.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, Z. Leeper, K. Chandrasekhar, W. Borden, B. Ma.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include "archsim.h"
#include "forward.h"
#include "hazard_control.h"
#include "hw_elts.h"
#include "instr.h"
#include "instr_pipeline.h"

bool running_sim = true; // should be no need to make this atomic
pthread_barrier_t cycle_start;
pthread_barrier_t cycle_end;

#ifdef RANDOM
#define NUM_ORDERINGS 120
extern const proc_stage_t orderings[NUM_ORDERINGS][5];
#endif

extern uint32_t bitfield_u32(int32_t src, unsigned frompos, unsigned width);
extern int64_t bitfield_s64(int32_t src, unsigned frompos, unsigned width);

extern machine_t guest;
extern mem_status_t dmem_status;
extern uint64_t F_PC;
extern uint8_t D_src1, D_src2;
extern uint8_t X_nzcvval;
extern bool X_set_flags;
extern uint64_t M_PC;
extern uint64_t W_wval;
extern bool deassert_flags;

void *start_fetch(void *unused) {
    pthread_barrier_wait(&cycle_start);
    do {
        fetch_instr(F_out, D_in);
        pthread_barrier_wait(&cycle_end);
        pthread_barrier_wait(&cycle_start);
    } while (running_sim);
    pthread_exit(NULL);
}

void *start_decode(void *unused) {
    pthread_barrier_wait(&cycle_start); // Guarded do :)
    do {
        decode_instr(D_out, X_in);
        pthread_barrier_wait(&cycle_end);
        pthread_barrier_wait(&cycle_start);
    } while (running_sim);
    pthread_exit(NULL);
}

void *start_execute(void *unused) {
    pthread_barrier_wait(&cycle_start);
    do {
        execute_instr(X_out, M_in);
        pthread_barrier_wait(&cycle_end);
        pthread_barrier_wait(&cycle_start);
    } while (running_sim);
    pthread_exit(NULL);
}

void *start_memory(void *unused) {
    pthread_barrier_wait(&cycle_start);
    do {
        memory_instr(M_out, W_in);
        pthread_barrier_wait(&cycle_end);
        pthread_barrier_wait(&cycle_start);
    } while (running_sim);
    pthread_exit(NULL);
}

void *start_writeback(void *unused) {
    pthread_barrier_wait(&cycle_start);
    do {
        wback_instr(W_out);
        pthread_barrier_wait(&cycle_end);
        pthread_barrier_wait(&cycle_start);
    } while (running_sim);
    pthread_exit(NULL);
}

int runElf(const uint64_t entry) {
    logging(LOG_INFO, "Running ELF executable");
    guest.proc->PC = entry;
    guest.proc->SP = guest.mem->seg_start_addr[STACK_SEG] - 8;
    guest.proc->NZCV = PACK_CC(0, 1, 0, 0);
    guest.proc->GPR[30] = RET_FROM_MAIN_ADDR;

    pipe_reg_t **pipes[] = {&F_instr, &D_instr, &X_instr, &M_instr, &W_instr};

    uint64_t sizes[5] = {sizeof(f_instr_impl_t), sizeof(d_instr_impl_t),
                         sizeof(x_instr_impl_t), sizeof(m_instr_impl_t),
                         sizeof(w_instr_impl_t)};
    for (int i = 0; i < 5; i++) {
        *pipes[i] = (pipe_reg_t *) calloc(1, sizeof(pipe_reg_t));
        (*pipes[i])->size = sizes[i];
        (*pipes[i])->in = (pipe_reg_implt_t) calloc(1, sizes[i]);
        (*pipes[i])->out = (pipe_reg_implt_t) calloc(1, sizes[i]);
        (*pipes[i])->ctl = P_BUBBLE;
    }

    /* Will be selected as the first PC */
    F_out->pred_PC = guest.proc->PC;
    F_out->status = STAT_AOK;
    dmem_status = READY;

    num_instr = 0;

#ifdef PARALLEL
    pthread_t stage_threads[5];
    void *(*stages[5])(void *args) = {&start_fetch, &start_decode,
                                      &start_execute, &start_memory,
                                      &start_writeback};

    running_sim = true;

    pthread_barrier_init(&cycle_start, NULL, 6);
    pthread_barrier_init(&cycle_end, NULL, 6);

    for (int stage = 0; stage < 5; stage++) {
        pthread_create(stage_threads + stage, NULL, stages[stage], NULL);
    }
#endif

    do {
#ifdef PARALLEL
        // Start a cycle
        pthread_barrier_wait(&cycle_start);

        // Wait for the cycle to end
        pthread_barrier_wait(&cycle_end);
#elif defined(RANDOM)
        const int which_order = rand() % NUM_ORDERINGS;
        bool completed_stage[5] = {0};
        for (int step = 0; step < 5; step++) {
            switch (orderings[which_order][step]) {
                case S_FETCH:
                    completed_stage[0] = true;
                    fetch_instr(F_out, D_in);
                    break;
                case S_DECODE:
                    completed_stage[1] = true;
                    decode_instr(D_out, X_in);
                    break;
                case S_EXECUTE:
                    completed_stage[2] = true;
                    execute_instr(X_out, M_in);
                    break;
                case S_MEMORY:
                    completed_stage[3] = true;
                    memory_instr(M_out, W_in);
                    break;
                case S_WBACK:
                    completed_stage[4] = true;
                    wback_instr(W_out);
                    break;
                default:
                    assert(false);
            }
        }

        // Expect all stages to have been executed.
        assert(completed_stage[0] && completed_stage[1] && completed_stage[2] &&
               completed_stage[3] && completed_stage[4]);
#else
        /* Run each stage. */
        fetch_instr(F_out, D_in);
        decode_instr(D_out, X_in);
        execute_instr(X_out, M_in);
        memory_instr(M_out, W_in);
        wback_instr(W_out);
#endif

        forward_reg(D_src1, D_src2, X_out->dst, M_out->dst, W_out->dst,
                    M_in->val_ex, M_out->val_ex, W_in->val_mem, W_out->val_ex,
                    W_out->val_mem, M_out->W_sigs.wval_sel,
                    W_out->W_sigs.wval_sel, X_out->W_sigs.w_enable,
                    M_out->W_sigs.w_enable, W_out->W_sigs.w_enable,
                    &X_in->val_a, &X_in->val_b);

        // combonational logic finished, run the correction steps.

        /* Set machine state to either continue executing or shutdown */
        guest.proc->status = W_out->status;

        /* Hazard handling and pipeline control */
        handle_hazards(D_out->op, D_src1, D_src2, X_in->val_a, X_out->op,
                       X_out->dst, M_in->cond_holds);

        // correct "stage" finished, update clocked logic
        if (!deassert_flags && X_set_flags) {
            // If X wants to write NZCV, AND handle hazards doesnt want to
            // deassert, write the value
            guest.proc->NZCV = X_nzcvval;
        }

        regfile_write(W_out->dst, W_wval,
                      W_out->W_sigs.w_enable && W_out->status == STAT_AOK);

        // update to the PC to be correct
#ifdef PIPE
        if (W_in->status == STAT_ADR || W_in->status == STAT_INS) {
            // PC of the excepting instruction
            guest.proc->PC = M_PC;
        } else
#endif
        {
            // predicted PC
            guest.proc->PC = F_PC;
        }

        // Latch
        F_in->pred_PC = guest.proc->PC;

        /* Print debug output */
        if (debug_level > 0)
            printf("\nPipeline state at end of cycle %ld:\n", num_instr);

        show_instr(S_FETCH, debug_level);
        show_instr(S_DECODE, debug_level);
        show_instr(S_EXECUTE, debug_level);
        show_instr(S_MEMORY, debug_level);
        show_instr(S_WBACK, debug_level);

        if (debug_level > 0)
            printf("\n"); // here for spacing
        show_nzcv(debug_level);

        if (debug_level > 0)
            printf("\n\n");

        for (int i = 0; i < 5; i++) {
            pipe_reg_t *pipe = *pipes[i];
            switch (pipe->ctl) {
                case P_LOAD: // Normal, cycle stage
                    memcpy(pipe->out.generic, pipe->in.generic, pipe->size);
                    break;
                case P_ERROR: // Error, bubble this stage
                    guest.proc->status = STAT_HLT;
                    // fallthrough
                case P_BUBBLE: // Hazard, needs to bubble
                    memset(pipe->out.generic, 0, pipe->size);
                    break;
                case P_STALL: // Hazard, needs to stall
                    break;
            }
        }

        num_instr++;
    } while (
        (guest.proc->status == STAT_AOK || guest.proc->status == STAT_BUB) &&
        num_instr < cycle_max);

    running_sim = false;

#ifdef PARALLEL
    // Start threads to send end 'signal' (could also just use pthread_kill...)
    pthread_barrier_wait(&cycle_start);

    for (int stage = 0; stage < 5; stage++) {
        pthread_join(stage_threads[stage], NULL);
    }

    pthread_barrier_destroy(&cycle_start);
    pthread_barrier_destroy(&cycle_end);
#endif

    return EXIT_SUCCESS;
}
