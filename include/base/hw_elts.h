/**************************************************************************
 * C S 429 system emulator
 *
 * hw_elts.h - A header file defining the prototypes of the hardware elements
 * used in the pipeline simulator.
 *
 * All of the hardware elements are of return type void, so they write return
 * values in their out parameters (which must therefore be passed by reference).
 * The in and out parameters are indicated in the prototypes below.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, W. Borden.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#ifndef _HW_ELTS_H_
#define _HW_ELTS_H_
#include "instr.h"
#include "instr_pipeline.h"
#include <stdbool.h>
#include <stdint.h>

#define SP_NUM 31
#define XZR_NUM 32

typedef struct full_adder {
    bool bit_a;
    bool bit_b;
    bool c_in;
    bool c_out;
    bool s;
} fa_t;

/**
 * Register file read.
 *
 * Simulates a read from the regfile, reads 2 source registers.
 *
 * @param src1 the number of the first source register to read from.
 * @param src2 the number of the second source register to read from.
 * @param val_a a pointer to the space where the value read from src1 should go.
 * @param val_b a pointer to the spave where the value read from src2 should go.
 */
extern comb_logic_t regfile_read(uint8_t src1, uint8_t src2, uint64_t *val_a,
                                 uint64_t *val_b);

/**
 * Register file write.
 *
 * Simulates a write to the regfile. May preform a write to a single destination
 * register.
 *
 * @param dst the number of the destination register to write to.
 * @param val_w the value to write into the destination register.
 * @param w_enable a flag to enable writing val_w into dst.
 */
extern comb_logic_t regfile_write(uint8_t dst, uint64_t val_w, bool w_enable);

/**
 * ALU
 *
 * Simulates 1 ALU operation.
 *
 * @param alu_vala the "A" input of the ALU.
 * @param alu_valb the "B" input of the ALU.
 * @param alu_valhw the "val_hw" input to the ALU. Usually used for shift
 * amounts for non-shift ops.
 * @param nzcv the current value of the condition flags
 * @param ALUop the ALU operation to perform
 * @param set_flags a flag to enable writing NZCV flags to 'nzcv' or not
 * @param cond the condition code to check (things like NE, EQ, GT, etc)
 * @param val_e a pointer to the space where the ALU result should be placed
 * @param cond_val a pointer to the space where the cond_holds result should be
 * placed
 * @param nzcv_dst a pointer to the space where the updated NZCV flags should be
 * placed, if set_flags is true.
 */
extern comb_logic_t alu(uint64_t alu_vala, uint64_t alu_valb, uint8_t alu_valhw,
                        uint8_t nzcv, // in data
                        alu_op_t ALUop, bool set_flags, cond_t cond,
                        uint64_t *val_e, bool *cond_val,
                        uint8_t *nzcv_dst); // out

/* Instruction memory. */
extern comb_logic_t imem(uint64_t imem_addr,                   // in, data
                         uint32_t *imem_rval, bool *imem_err); // out

/* Data memory. */
extern comb_logic_t dmem(uint64_t dmem_addr, uint64_t dmem_wval, // in, data
                         bool dmem_read, bool dmem_write,        // in, control
                         uint64_t *dmem_rval, bool *dmem_err);   // out

#endif
