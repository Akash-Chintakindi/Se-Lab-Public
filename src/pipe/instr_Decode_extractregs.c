/**************************************************************************
 * STUDENTS: DO NOT MODIFY.
 *
 * C S 429 system emulator
 *
 * instr_Decode_extractregs.c - Lookup table for functions handlers to
 * extract registers.
 *
 * Copyright (c) 2025.
 * Authors: B. Ma.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include "instr_pipeline.h"

#include <assert.h>

extern comb_logic_t format_other(uint32_t insnbits, opcode_t op, uint8_t *src1,
                                 uint8_t *src2, uint8_t *dst);
extern comb_logic_t format_m(uint32_t insnbits, opcode_t op, uint8_t *src1,
                             uint8_t *src2, uint8_t *dst);
extern comb_logic_t format_i1(uint32_t insnbits, opcode_t op, uint8_t *src1,
                              uint8_t *src2, uint8_t *dst);
extern comb_logic_t format_i2(uint32_t insnbits, opcode_t op, uint8_t *src1,
                              uint8_t *src2, uint8_t *dst);
extern comb_logic_t format_rr(uint32_t insnbits, opcode_t op, uint8_t *src1,
                              uint8_t *src2, uint8_t *dst);
extern comb_logic_t format_ri(uint32_t insnbits, opcode_t op, uint8_t *src1,
                              uint8_t *src2, uint8_t *dst);
extern comb_logic_t format_b1(uint32_t insnbits, opcode_t op, uint8_t *src1,
                              uint8_t *src2, uint8_t *dst);
extern comb_logic_t format_b2(uint32_t insnbits, opcode_t op, uint8_t *src1,
                              uint8_t *src2, uint8_t *dst);
extern comb_logic_t format_b3(uint32_t insnbits, opcode_t op, uint8_t *src1,
                              uint8_t *src2, uint8_t *dst);
extern comb_logic_t format_s(uint32_t insnbits, opcode_t op, uint8_t *src1,
                             uint8_t *src2, uint8_t *dst);
#ifdef EC
extern comb_logic_t format_ec(uint32_t insnbits, opcode_t op, uint8_t *src1,
                              uint8_t *src2, uint8_t *dst);
#endif

// Initialize an entry in the extract regs look up table.
static void init_extract_regs_entry(format_t format, extract_reg_func_t func) {
    assert(format >= 0 && format < NUM_FORMATS);
    assert(extract_regs_table[format] == format_other);
    extract_regs_table[format] = func;
}

void init_extract_regs_table() {
    for (int i = 0; i < NUM_FORMATS; i++) {
        extract_regs_table[i] = format_other;
    }

    init_extract_regs_entry(FORMAT_M, format_m);
    init_extract_regs_entry(FORMAT_I1, format_i1);
    init_extract_regs_entry(FORMAT_I2, format_i2);
    init_extract_regs_entry(FORMAT_RR, format_rr);
    init_extract_regs_entry(FORMAT_RI, format_ri);
    init_extract_regs_entry(FORMAT_B1, format_b1);
    init_extract_regs_entry(FORMAT_B2, format_b2);
    init_extract_regs_entry(FORMAT_B3, format_b3);
    init_extract_regs_entry(FORMAT_S, format_s);
#ifdef EC
    init_extract_regs_entry(FORMAT_EC, format_ec);
#endif
    return;
}
