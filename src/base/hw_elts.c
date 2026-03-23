/**************************************************************************
 * C S 429 system emulator
 *
 * hw_elts.c - Module for emulating hardware elements.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, Z. Leeper., P. Jamadagni, W. Borden.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include "hw_elts.h"
#include "err_handler.h"
#include "instr.h"
#include "instr_pipeline.h"
#include "machine.h"
#include "mem.h"
#include <assert.h>
#include <stdint.h>

extern machine_t guest;

fa_t rca[64];

/*
 * Read from instruction memory. Students, do not modify.
 */
comb_logic_t imem(uint64_t imem_addr, uint32_t *imem_rval, bool *imem_err) {
    // imem_addr must be in "instruction memory" and a multiple of 4
    *imem_err = (!addr_in_imem(imem_addr) || (imem_addr & 0x3U));
    *imem_rval = (uint32_t) mem_read_I(imem_addr);
}

/*
 * Sets up the inputs to the ripple carry adder.
 * STUDENT TO-DO:
 */
comb_logic_t init_rca(uint64_t val_a, uint64_t val_b, bool c_in) {
    /* your implementation */
    return;
}

/*
 * Performs the ripple carry add.
 * STUDENT TO-DO:
 */
comb_logic_t ripple_carry_add(uint64_t *sum) {
    /* your implementation */
    return;
}

/*
 * Read from register file.
 * STUDENT TO-DO:
 * Read from src1 and src2 registers. Take extra care for SP/XZR.
 */
comb_logic_t regfile_read(uint8_t src1, uint8_t src2, uint64_t *val_a,
                          uint64_t *val_b) {
    /* your implementation */
    return;
}

/*
 * Write to register file.
 * STUDENT TO-DO:
 * Write to dst register if enabled. Take extra care for SP/XZR.
 */
comb_logic_t regfile_write(uint8_t dst, uint64_t val_w, bool w_enable) {
    /* your implementation */
    return;
}

/*
 * Check whether a condition is satisfied given the NCZV status flags.
 * STUDENT TO-DO:
 */
static bool cond_holds(cond_t cond, uint8_t flags) {
    /* your implementation */
    return false;
}

/*
 * Perform the appropriate ALU operation, setting NZCV flags if needed.
 * STUDENT TO-DO:
 */
comb_logic_t alu(uint64_t alu_vala, uint64_t alu_valb, uint8_t alu_valhw,
                 uint8_t nzcv, alu_op_t ALUop, bool set_flags, cond_t cond,
                 uint64_t *val_e, bool *cond_val, uint8_t *nzcv_dst) {
    /* your implementation */
    return;
}

/*
 * Read from data memory, Students do not modify.
 */
comb_logic_t dmem(uint64_t dmem_addr, uint64_t dmem_wval, bool dmem_read,
                  bool dmem_write, uint64_t *dmem_rval, bool *dmem_err) {
    if (!dmem_read && !dmem_write) {
        return;
    }
    // dmem_addr must be in "data memory" and a multiple of 8
    *dmem_err = (!addr_in_dmem(dmem_addr) || (dmem_addr & 0x7U));
    if (is_special_addr(dmem_addr))
        *dmem_err = false;
    if (dmem_read)
        *dmem_rval = (uint64_t) mem_read_L(dmem_addr);
    if (dmem_write)
        mem_write_L(dmem_addr, dmem_wval);
}
