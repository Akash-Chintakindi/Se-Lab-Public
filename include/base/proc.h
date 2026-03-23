/**************************************************************************
 * C S 429 system emulator
 *
 * proc.h - Headers for processor parameters used by the emulator.
 *
 * Copyright (c) 2022.
 * Authors: S. Chatterjee.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#ifndef _PROC_H_
#define _PROC_H_
#include "instr.h"
#include "instr_pipeline.h"
#include <stdint.h>

typedef uint64_t gpreg_val_t;

// Processor state.
typedef struct proc {
    // Architectural state
    uint64_t SP;         // Stack pointer
    uint64_t PC;         // Program counter
    uint8_t NZCV;        // Condition flags
    gpreg_val_t GPR[31]; // General-purpose registers

    // Micro-architectural state
    pipe_reg_t *f_insn; // Fetch stage pipeline register
    pipe_reg_t *d_insn; // Decode stage pipeline register
    pipe_reg_t *x_insn; // Execute stage pipeline register
    pipe_reg_t *m_insn; // Memory stage pipeline register
    pipe_reg_t *w_insn; // Writeback stage pipeline register

    pipe_reg_t *rf_stage; // Regfile staging area pipeline register

    stat_t status; // Pipeline status
} proc_t;

// Run the loaded ELF executable for no more than a specified number of cycles.
extern int runElf(const uint64_t);
#endif