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
 */
comb_logic_t init_rca(uint64_t val_a, uint64_t val_b, bool c_in) {
    for (int i = 0; i < 64; i++) {
        rca[i].bit_a = (val_a >> i) & 1;
        rca[i].bit_b = (val_b >> i) & 1;
        rca[i].c_in  = false;
        rca[i].c_out = false;
        rca[i].s     = false;
    }
    rca[0].c_in = c_in;
}

/*
 * Performs the ripple carry add.
 */
comb_logic_t ripple_carry_add(uint64_t *sum) {
    uint64_t result = 0;
    for (int i = 0; i < 64; i++) {
        bool a   = rca[i].bit_a;
        bool b   = rca[i].bit_b;
        bool cin = rca[i].c_in;
        rca[i].s     = a ^ b ^ cin;
        rca[i].c_out = (a & b) | (a & cin) | (b & cin);
        if (rca[i].s)
            result |= (1ULL << i);
        if (i + 1 < 64)
            rca[i + 1].c_in = rca[i].c_out;
    }
    *sum = result;
}

/*
 * Read from register file.
 * Read from src1 and src2 registers. Take extra care for SP/XZR.
 */
comb_logic_t regfile_read(uint8_t src1, uint8_t src2, uint64_t *val_a,
                          uint64_t *val_b) {
    /* src1 */
    if (src1 == XZR_NUM)
        *val_a = 0;
    else if (src1 == SP_NUM)
        *val_a = guest.proc->SP;
    else
        *val_a = guest.proc->GPR[src1];

    /* src2 */
    if (src2 == XZR_NUM)
        *val_b = 0;
    else if (src2 == SP_NUM)
        *val_b = guest.proc->SP;
    else
        *val_b = guest.proc->GPR[src2];
}

/*
 * Write to register file.
 * Write to dst register if enabled. Take extra care for SP/XZR.
 */
comb_logic_t regfile_write(uint8_t dst, uint64_t val_w, bool w_enable) {
    if (!w_enable)
        return;
    if (dst == XZR_NUM)
        return; /* writes to XZR are discarded */
    if (dst == SP_NUM)
        guest.proc->SP = val_w;
    else
        guest.proc->GPR[dst] = val_w;
}

/*
 * Check whether a condition is satisfied given the NZCV status flags.
 * Follows ARM Architecture Reference Manual, C1.2.4.
 */
static bool cond_holds(cond_t cond, uint8_t flags) {
    bool n = GET_NF(flags);
    bool z = GET_ZF(flags);
    bool c = GET_CF(flags);
    bool v = GET_VF(flags);

    switch (cond) {
        case C_EQ: return z;
        case C_NE: return !z;
        case C_CS: return c;
        case C_CC: return !c;
        case C_MI: return n;
        case C_PL: return !n;
        case C_VS: return v;
        case C_VC: return !v;
        case C_HI: return c && !z;
        case C_LS: return !c || z;
        case C_GE: return n == v;
        case C_LT: return n != v;
        case C_GT: return !z && (n == v);
        case C_LE: return z || (n != v);
        case C_AL: return true;
        case C_NV: return true;
        default:   return false;
    }
}

/*
 * Perform the appropriate ALU operation, setting NZCV flags if needed.
 */
comb_logic_t alu(uint64_t alu_vala, uint64_t alu_valb, uint8_t alu_valhw,
                 uint8_t nzcv, alu_op_t ALUop, bool set_flags, cond_t cond,
                 uint64_t *val_e, bool *cond_val, uint8_t *nzcv_dst) {
    uint64_t result  = 0;
    uint8_t new_nzcv = nzcv;

    switch (ALUop) {
        case PLUS_OP: {
            init_rca(alu_vala, alu_valb, false);
            ripple_carry_add(&result);
            if (set_flags) {
                bool n = (result >> 63) & 1;
                bool z = (result == 0);
                bool c = rca[63].c_out;
                /* signed overflow: operands same sign, result different sign */
                bool v = ((alu_vala >> 63) == (alu_valb >> 63)) &&
                         ((result >> 63) != (alu_vala >> 63));
                new_nzcv = PACK_CC(n, z, c, v);
            }
            break;
        }
        case MINUS_OP: {
            /* a - b  ==  a + ~b + 1 */
            init_rca(alu_vala, ~alu_valb, true);
            ripple_carry_add(&result);
            if (set_flags) {
                bool n = (result >> 63) & 1;
                bool z = (result == 0);
                bool c = rca[63].c_out;
                /* signed overflow: operands different sign, result sign differs from a */
                bool v = ((alu_vala >> 63) != (alu_valb >> 63)) &&
                         ((result >> 63) != (alu_vala >> 63));
                new_nzcv = PACK_CC(n, z, c, v);
            }
            break;
        }
        case INV_OP:
            result = alu_vala | (~alu_valb);
            if (set_flags) {
                bool n = (result >> 63) & 1;
                bool z = (result == 0);
                new_nzcv = PACK_CC(n, z, 0, 0);
            }
            break;
        case OR_OP:
            result = alu_vala | alu_valb;
            if (set_flags) {
                bool n = (result >> 63) & 1;
                bool z = (result == 0);
                new_nzcv = PACK_CC(n, z, 0, 0);
            }
            break;
        case EOR_OP:
            result = alu_vala ^ alu_valb;
            if (set_flags) {
                bool n = (result >> 63) & 1;
                bool z = (result == 0);
                new_nzcv = PACK_CC(n, z, 0, 0);
            }
            break;
        case AND_OP:
            result = alu_vala & alu_valb;
            if (set_flags) {
                bool n = (result >> 63) & 1;
                bool z = (result == 0);
                new_nzcv = PACK_CC(n, z, 0, 0);
            }
            break;
        case MOV_OP:
            /* MOVZ: result = 0 | (imm16 << hw*16).  vala is 0 (XZR). */
            result = alu_vala | (alu_valb << alu_valhw);
            break;
        case MOVK_OP:
            /* Keep all bits except the target 16-bit field, insert new imm16. */
            result = (alu_vala & ~(0xFFFFULL << alu_valhw)) |
                     (alu_valb << alu_valhw);
            break;
        case LSL_OP:
            result = alu_vala << (alu_valb & 0x3FULL);
            break;
        case LSR_OP:
            result = alu_vala >> (alu_valb & 0x3FULL);
            break;
        case ASR_OP:
            result = (uint64_t)((int64_t)alu_vala >> (alu_valb & 0x3FULL));
            break;
        case PASS_A_OP:
            result = alu_vala;
            break;
#ifdef EC
        case CSEL_OP:
            /* If cond holds, result = vala, else result = valb */
            result = cond_holds(cond, nzcv) ? alu_vala : alu_valb;
            break;
        case CSINV_OP:
            result = cond_holds(cond, nzcv) ? alu_vala : ~alu_valb;
            break;
        case CSINC_OP: {
            uint64_t inc;
            init_rca(alu_valb, 1ULL, false);
            ripple_carry_add(&inc);
            result = cond_holds(cond, nzcv) ? alu_vala : inc;
            break;
        }
        case CSNEG_OP: {
            uint64_t neg;
            init_rca(~alu_valb, 1ULL, false);
            ripple_carry_add(&neg);
            result = cond_holds(cond, nzcv) ? alu_vala : neg;
            break;
        }
        case CBZ_OP:
            result = alu_vala;
            break;
        case CBNZ_OP:
            result = alu_vala;
            break;
#endif
        default:
            result = 0;
            break;
    }

    *val_e    = result;
    *nzcv_dst = new_nzcv;
    *cond_val = cond_holds(cond, new_nzcv);
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
