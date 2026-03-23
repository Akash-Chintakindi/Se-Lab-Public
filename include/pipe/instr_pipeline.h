/**************************************************************************
 * C S 429 system emulator
 *
 * instr.h - Header file for everything related to the five-stage in-order
 * pipelined implementation of chArm-v2 instructions.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, Si. Nemana, Z. Leeper, B. Ma.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#ifndef _INSTR_PIPELINE_H_
#define _INSTR_PIPELINE_H_

#include <stdbool.h>
#include <stdint.h>

#include "instr.h"
#include "mem.h"

// Function pointer type for the extract register handlers.
typedef comb_logic_t (*extract_reg_func_t)(uint32_t insnbits, opcode_t op,
                                           uint8_t *src1, uint8_t *src2,
                                           uint8_t *dst);

// Map instruction format to function handler to extract src1, src2, and dst
// registers.
extern extract_reg_func_t extract_regs_table[];

// Pipeline stages.
typedef enum proc_stage {
    S_FETCH,
    S_DECODE,
    S_EXECUTE,
    S_MEMORY,
    S_WBACK,
    S_UPDATE_PC,
    S_ERROR = -1
} proc_stage_t;

// Operations supported by the processor ALU.
typedef enum alu_op {
    PLUS_OP,   // vala + valb
    MINUS_OP,  // vala - valb
    INV_OP,    // vala | (~valb)
    OR_OP,     // vala | valb
    EOR_OP,    // vala ^ valb
    AND_OP,    // vala & valb
    MOV_OP,    // vala | (valb << valhw)
    MOVK_OP,   // (vala & (~(0xFFFFUL << valhw))) | (valb << valhw)
    LSL_OP,    // vala << (valb & 0x3FUL)
    LSR_OP,    // vala >>L (valb & 0x3FUL)
    ASR_OP,    // vala >>A (valb & 0x3FUL)
    PASS_A_OP, // vala
#ifdef EC
    CSEL_OP,  // EC: used for csel
    CSINV_OP, // EC: used for csinv
    CSINC_OP, // EC: used for csinc
    CSNEG_OP, // EC: used for csneg
    CBZ_OP,   // EC: used for cbz
    CBNZ_OP,  // EC: used for cbnz
#endif
    ERROR_OP = -1
} alu_op_t;

// Pipeline control signals generated in Decode and used in Decode.
typedef struct d_ctl_sigs {
    bool src2_sel;    // src2 selector: 1 for memory store, 0 otherwise.
} d_ctl_sigs_t;

// Pipeline control signals generated in Decode and used in Execute.
typedef struct x_ctl_sigs {
    bool vala_sel;  // vala selector: 0 for val_a, 1 for multipurpose_val.
    bool valb_sel;  // valb selector: 0 for immediate, 1 for register.
    bool set_flags; // Whether to set condition flags: 0 for no, 1 for yes.
} x_ctl_sigs_t;

// Pipeline control signals generated in Decode and used in Memory.
typedef struct m_ctl_sigs {
    bool dmem_read;  // 1 if memory access is a read, 0 if not.
    bool dmem_write; // 1 if memory access is a write, 0 if not.
} m_ctl_sigs_t;

// Pipeline control signals generated in Decode and used in Writeback.
typedef struct w_ctl_sigs {
    bool dst_sel;  // dst selector: 1 for X30 (in BL), 0 otherwise.
    bool wval_sel; // wval selector: 1 for LDUR, 0 otherwise.
    bool w_enable; // Whether to perform a write: 0 for no, 1 for yes.
} w_ctl_sigs_t;

// Pipeline register feeding the Fetch stage.
typedef struct f_instr_impl {
    uint64_t pred_PC; // what do we think the next PC will be?
    stat_t status;    // status of this instruction
} f_instr_impl_t;

// Pipeline register feeding the Decode stage.
typedef struct d_instr_impl {
    uint32_t insnbits; // instruction bits
    opcode_t op;       // instruction opcode
    opcode_t print_op; // opcode to print: needed for aliased instructions
    format_t format;   // Instruction format
    union {
        uint64_t seq_succ_PC;     // next sequential PC
        uint64_t adrp_val;        // used for adrp
        uint64_t correction_PC;   // PC to correct to in case of an exception
    } multipurpose_val;
    stat_t status; // status of this instruction
} d_instr_impl_t;

// Pipeline register feeding the Execute stage.
typedef struct x_instr_impl {
    opcode_t op;       // instruction opcode
    opcode_t print_op; // opcode to print: needed for aliased instructions
    union {
        uint64_t seq_succ_PC;     // next sequential PC
        uint64_t correction_PC;   // PC to correct to in case of an exception
    } multipurpose_val;
    x_ctl_sigs_t X_sigs; // signals consumed by execute stage
    m_ctl_sigs_t M_sigs; // signals consumed by memory stage
    w_ctl_sigs_t W_sigs; // signals consumed by writeback stage
    alu_op_t ALU_op;     // operation for the ALU to perform
    cond_t cond;         // branch condition to test
    uint64_t val_a;      // regfile output for register src1
    uint64_t val_b;      // regfile output for register src2
    int64_t val_imm;     // imm field for M-, I-, and RI-format instructions
    uint8_t val_hw;      // hw field for I-format instructions
    uint8_t dst;         // destination register encoding
    stat_t status;       // status of this instruction
} x_instr_impl_t;

// Pipeline register feeding the Memory stage.
typedef struct m_instr_impl {
    opcode_t op;       // instruction opcode (only for debugging at this point)
    opcode_t print_op; // opcode to print: needed for aliased instructions
    union {
        uint64_t seq_succ_PC;     // next sequential PC
        uint64_t correction_PC;   // PC to correct to in case of an exception
    } multipurpose_val;
    bool cond_holds;     // result of testing NZCV codes
    m_ctl_sigs_t M_sigs; // signals consumed by memory stage
    w_ctl_sigs_t W_sigs; // signals consumed by writeback stage
    uint64_t val_b;      // regfile output for register src2
    uint8_t dst;         // destination register encoding
    uint64_t val_ex;     // value computed by ALU
    stat_t status;       // status of this instruction
} m_instr_impl_t;

// Pipeline register feeding the Writeback stage.
typedef struct w_instr_impl {
    opcode_t op;       // instruction opcode (only for debugging at this point)
    opcode_t print_op; // opcode to print: needed for aliased instructions
    w_ctl_sigs_t W_sigs; // signals consumed by writeback stage
    uint8_t dst;         // destination register encoding
    uint64_t val_ex;     // value computed by ALU
    uint64_t val_mem;    // value read from memory
    stat_t status;       // status of this instruction
} w_instr_impl_t;

// Pipeline register control signals.
typedef enum pipe_control_stat {
    P_LOAD,   // let instruction through
    P_ERROR,  // error in future stage, stop
    P_BUBBLE, // bubble instruction
    P_STALL   // stall instruction
} pipe_ctl_stat_t;

// Unified version of the various pipeline stage registers.
// Note that this is a union, not a struct.
typedef union pipeline_register_impl {
    f_instr_impl_t *f;
    d_instr_impl_t *d;
    x_instr_impl_t *x;
    m_instr_impl_t *m;
    w_instr_impl_t *w;
    void *generic;
} pipe_reg_implt_t;

// A full pipeline register, consisting of an input side, an output side, and
// control signal. At the "clock edge", the output side:
//  - receives a copy of the input side, if the control signal is P_LOAD;
//  - ??, if the control signal is P_ERROR;
//  - receives a pattern simulating the action of NOP, if the control signal is
//  P_BUBBLE; and
//  - retains its previous value, if the control signal is P_STALL.
typedef struct pipeline_register {
    pipe_reg_implt_t in;  // input side, previous stage writes to this
    pipe_reg_implt_t out; // output side, current stage reads from this
    uint64_t size;        // size of respective pipeline register struct
    pipe_ctl_stat_t ctl;  // what to do between cycles
} pipe_reg_t;

/*
 * You should really use these macros.
 * They will save a lot of time and space.
 */
#define F_instr (guest.proc->f_insn)
#define F_in (guest.proc->f_insn->in.f)
#define F_out (guest.proc->f_insn->out.f)

#define D_instr (guest.proc->d_insn)
#define D_in (guest.proc->d_insn->in.d)
#define D_out (guest.proc->d_insn->out.d)

#define X_instr (guest.proc->x_insn)
#define X_in (guest.proc->x_insn->in.x)
#define X_out (guest.proc->x_insn->out.x)

#define M_instr (guest.proc->m_insn)
#define M_in (guest.proc->m_insn->in.m)
#define M_out (guest.proc->m_insn->out.m)

#define W_instr (guest.proc->w_insn)
#define W_in (guest.proc->w_insn->in.w)
#define W_out (guest.proc->w_insn->out.w)

/* Function prototypes. */
extern uint32_t bitfield_u32(int32_t src, unsigned frompos, unsigned width);
extern int64_t bitfield_s64(int32_t src, unsigned frompos, unsigned width);
extern void init_itable(void);
extern void init_ftable(void);
extern comb_logic_t fetch_instr(f_instr_impl_t *in, d_instr_impl_t *out);
extern comb_logic_t decode_instr(d_instr_impl_t *in, x_instr_impl_t *out);
extern void init_extract_regs_table(void);
extern comb_logic_t execute_instr(x_instr_impl_t *in, m_instr_impl_t *out);
extern comb_logic_t memory_instr(m_instr_impl_t *in, w_instr_impl_t *out);
extern comb_logic_t wback_instr(w_instr_impl_t *in);
extern void show_instr(const proc_stage_t, int);
extern void show_nzcv(int debug_level);
#endif
