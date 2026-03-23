/**************************************************************************
 * STUDENTS: DO NOT MODIFY.
 *
 * C S 429 system emulator
 *
 * instr_base.c - Common helper routines for instruction processing.
 *
 * Stage details are written in the instr_*.c files in this directory.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, P. Jamadagni, Z. Leeper, W. Borden.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "err_handler.h"
#include "hw_elts.h"
#include "instr.h"
#include "instr_pipeline.h"
#include "machine.h"

extern machine_t guest;
extern mem_status_t dmem_status;

// Declare "wires" that will be used to communicate from stages to clocked logic
uint64_t F_PC; // F stage should set the predicted PC to this, instead of
               // modifying the PC directly.
uint8_t D_src1, D_src2; // Used for Forwarding register values
uint8_t X_nzcvval;      // X stage will write the NZCV to this global
bool X_set_flags; // true if the X_nzcvval recieved from the X stage should be written
uint64_t M_PC;    // the PC of the instr in M stage if PC correction is required
int64_t W_wval;
bool deassert_flags; // handle hazards will set this signal if the NZCV from X
                     // should be NOT be written

/*
 * Extracts the bitfield src[frompos+width-1:frompos] and returns it
 * as an unsigned 32-bit integer.
 *
 * Use this function for register specifiers, condition specifiers,
 * and immediates in I1- and RI-format instructions. Also use it to
 * extract the 11 bits used to determine the opcode.
 */
uint32_t bitfield_u32(int32_t src, unsigned frompos, unsigned width) {
    return (uint32_t) ((src >> frompos) & ((1 << width) - 1));
}

/*
 * Extracts the bitfield src[frompos+width-1:frompos] and returns it
 * as an signed 64-bit integer.
 *
 * Use this function for 64-bit offsets needed in M-, I2-, B1-, and
 * B2-format instructions.
 */
int64_t bitfield_s64(int32_t src, unsigned frompos, unsigned width) {
    return ((int64_t) ((((int64_t) src >> frompos) & ((1 << width) - 1))
                       << (64 - width))) >>
           (64 - width);
}

static inline void init_itable_entry(opcode_t op, unsigned idx) {
    assert(OP_ERROR == itable[idx]);
    itable[idx] = op;
}

static inline void init_itable_range(opcode_t op, unsigned idx1,
                                     unsigned idx2) {
    for (unsigned i = idx1; i <= idx2; i++) {
        assert(OP_ERROR == itable[i]);
        itable[i] = op;
    }
}

/*
 * Initialize the itable. Called from interface.c.
 *
 * There are no entries for CMP, CMN, TST, LSL, and LSR because they are aliases
 * of other instructions. For extra credit there should be no entries for CSINC
 * and CSINV because they are aliases of other instruction
 */

// init_itable_entry(opcode_t opcode, unsigned long encoding)
// init_itable_entry(opcode_t opcode, unsigned long start_encoding_range,
// end_enconding_range)
void init_itable(void) {
    for (int i = 0; i < 2 << 11; i++)
        itable[i] = OP_ERROR;
    init_itable_entry(OP_LDUR, 0x7c2U);
    init_itable_entry(OP_STUR, 0x7c0U);
    init_itable_range(OP_MOVK, 0x794U, 0x797U);
    init_itable_range(OP_MOVZ, 0x694U, 0x697U);
    init_itable_range(OP_ADRP, 0x480U, 0x487U);
    init_itable_range(OP_ADRP, 0x580U, 0x587U);
    init_itable_range(OP_ADRP, 0x680U, 0x687U);
    init_itable_range(OP_ADRP, 0x780U, 0x787U);
    init_itable_range(OP_ADD_RI, 0x488U, 0x489U);
    init_itable_entry(OP_ADDS_RR, 0x558U);
    init_itable_range(OP_SUB_RI, 0x688U, 0x689U);
    init_itable_entry(OP_SUBS_RR, 0x758U);
    init_itable_entry(OP_MVN, 0x551U);
    init_itable_entry(OP_ORR_RR, 0x550U);
    init_itable_entry(OP_EOR_RR, 0x650U);
    init_itable_entry(OP_ANDS_RR, 0x750U);
    init_itable_range(OP_UBFM, 0x69aU,
                      0x69bU); // LSL, LSR share the same opcode
    init_itable_entry(OP_UBFMV,
                      0x4d6); // LSL_RR and LSR_RR share the same opcode
    init_itable_range(OP_ASR, 0x49aU, 0x49bU);
    init_itable_range(OP_B, 0x0a0U, 0x0bfU);
    init_itable_range(OP_B_COND, 0x2a0U, 0x2a7U);
    init_itable_range(OP_BL, 0x4a0U, 0x4bfU);
    init_itable_entry(OP_RET, 0x6b2U);
    init_itable_entry(OP_NOP, 0x6a8U);
    init_itable_entry(OP_HLT, 0x6a2U);

// extra credit
#ifdef EC
    init_itable_entry(OP_CSEL, 0x4d4U);
    init_itable_entry(OP_CSNEG, 0x6d4U);
    init_itable_range(OP_CBZ, 0x5a0U, 0x5a7U);
    init_itable_range(OP_CBNZ, 0x5a8U, 0x5afU);
    init_itable_entry(OP_BR, 0x6b0U);
    init_itable_entry(OP_BLR, 0x6b1U);
#endif
}

static inline void init_ftable_entry(opcode_t op, format_t format) {
    assert(ftable[op] == FORMAT_ERROR);
    assert(op >= 0 && op < NUM_OPCODES);
    ftable[op] = format;
}

void init_ftable(void) {
    for (int i = 0; i < NUM_OPCODES; i++) {
        ftable[i] = FORMAT_ERROR;
    }

    init_ftable_entry(OP_LDUR, FORMAT_M);
    init_ftable_entry(OP_STUR, FORMAT_M);
    init_ftable_entry(OP_MOVK, FORMAT_I1);
    init_ftable_entry(OP_MOVZ, FORMAT_I1);
    init_ftable_entry(OP_ADRP, FORMAT_I2);
    init_ftable_entry(OP_ADD_RI, FORMAT_RI);
    init_ftable_entry(OP_ADDS_RR, FORMAT_RR);
    init_ftable_entry(OP_CMN_RR, FORMAT_RR);
    init_ftable_entry(OP_SUB_RI, FORMAT_RI);
    init_ftable_entry(OP_SUBS_RR, FORMAT_RR);
    init_ftable_entry(OP_CMP_RR, FORMAT_RR);
    init_ftable_entry(OP_MVN, FORMAT_RR);
    init_ftable_entry(OP_ORR_RR, FORMAT_RR);
    init_ftable_entry(OP_EOR_RR, FORMAT_RR);
    init_ftable_entry(OP_ANDS_RR, FORMAT_RR);
    init_ftable_entry(OP_TST_RR, FORMAT_RR);
    init_ftable_entry(OP_LSL_RI, FORMAT_RI);
    init_ftable_entry(OP_LSR_RI, FORMAT_RI);
    init_ftable_entry(OP_UBFM, FORMAT_RI);
    init_ftable_entry(OP_LSL_RR, FORMAT_RR);
    init_ftable_entry(OP_LSR_RR, FORMAT_RR);
    init_ftable_entry(OP_UBFMV, FORMAT_RR);
    init_ftable_entry(OP_ASR, FORMAT_RI);
    init_ftable_entry(OP_B, FORMAT_B1);
    init_ftable_entry(OP_B_COND, FORMAT_B2);
    init_ftable_entry(OP_BL, FORMAT_B1);
    init_ftable_entry(OP_RET, FORMAT_B3);
    init_ftable_entry(OP_NOP, FORMAT_S);
    init_ftable_entry(OP_HLT, FORMAT_S);

// extra credit
#ifdef EC
    init_ftable_entry(OP_CSEL, FORMAT_EC);
    init_ftable_entry(OP_CSINV, FORMAT_EC);
    init_ftable_entry(OP_CSINC, FORMAT_EC);
    init_ftable_entry(OP_CSNEG, FORMAT_EC);
    init_ftable_entry(OP_CBZ, FORMAT_EC);
    init_ftable_entry(OP_CBNZ, FORMAT_EC);
    init_ftable_entry(OP_BR, FORMAT_EC);
    init_ftable_entry(OP_BLR, FORMAT_EC);
#endif
}

static char *opcode_names[] = {
    "NOP ", "LDUR ",   "STUR ", "MOVK ", "MOVZ ", "ADRP ", "ADD ",   "ADDS ",
    "CMN ", "SUB",     "SUBS ", "CMP ",  "MVN ",  "ORR ",  "EOR ",   "ANDS ",
    "TST ", "LSL ",    "LSR ",  "UBFM ", "LSL ",  "LSR ",  "UBFMV ", "ASR ",
    "B ",   "B.cond ", "BL ",   "RET ",  "HLT ",
#ifdef EC
    "CSEL", "CSINV",   "CSINC", "CSNEG", "CBZ",   "CBNZ",  "BR",     "BLR",
#endif
    "ERR "};

static char *format_names[] = {"ERR",       "M",  "I1", "I2", "RR",
                               "RI",        "B1", "B2", "B3", "S",
#ifdef EC
                               "FORMAT_EC",
#endif
                              };

static char *cond_names[] = {"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
                             "HI", "LS", "GE", "LT", "GT", "LE", "AL", "NV"};

static char *alu_op_names[] = {
    "PLUS_OP", "MINUS_OP", "INV_OP",   "OR_OP",    "EOR_OP", "AND_OP",
    "MOV_OP",  "MOVK_OP",  "LSL_OP",   "LSR_OP",   "ASR_OP", "PASS_A_OP",
#ifdef EC
    "CSEL_OP", "CSINV_OP", "CSINC_OP", "CSNEG_OP", "CBZ",    "CBNZ",
    "BR",      "BLR",
#endif
};

/*
 * A debugging aid to print out the fields of an instruction.
 * This routine is called from runElf().
 * Do not re-write.
 *
 * The amount of detail printed is controlled by debug_level.
 *  0: No output.
 *  1: Medium output. Data signals only in the pipeline register.
 *  2: Full output. Data and control signals in the pipeline register.
 */

static void get_stat_str(char *str, stat_t status) {
    switch (status) {
        case STAT_AOK:
            strcpy(str, "AOK");
            break;
        case STAT_BUB:
            strcpy(str, "BUB");
            break;
        case STAT_HLT:
            strcpy(str, "HLT");
            break;
        case STAT_ADR:
            strcpy(str, "ADR");
            break;
        case STAT_INS:
            strcpy(str, "INS");
            break;
    }
}

void show_nzcv(int debug_level) {
    if (debug_level < 1) {
        return;
    }

    printf("NZCV: [N, Z, C, V] = [%d, %d, %d, %d]\n", GET_NF(guest.proc->NZCV),
           GET_ZF(guest.proc->NZCV), GET_CF(guest.proc->NZCV),
           GET_VF(guest.proc->NZCV));
}

void show_instr(const proc_stage_t stage, int debug_level) {
    if (debug_level < 1) {
        return;
    }
    char status[4];

    switch (stage) {
        case S_FETCH:
            get_stat_str(status, D_in->status);
            uint64_t seq_succ_PC, adrp_val;
            if (debug_level > 2) {
                seq_succ_PC = D_in->op == OP_ADRP
                                  ? 0
                                  : D_in->multipurpose_val.seq_succ_PC;
                adrp_val =
                    D_in->op == OP_ADRP ? D_in->multipurpose_val.adrp_val : 0;
            } else {
                seq_succ_PC = D_in->multipurpose_val.seq_succ_PC;
                adrp_val = D_in->multipurpose_val.adrp_val;
            }
            printf(
                "F: %-6s[PC, insn_bits] = [%08lX,  %08X], seq_succ_PC: 0x%lX, "
                "pred_PC: 0x%lX, adrp_val: 0x%lX, format: %s, status: %s\n",
                D_in->print_op != OP_ERROR ? opcode_names[D_in->print_op]
                                           : "ERR",
                guest.proc->PC, D_in->insnbits, seq_succ_PC, F_in->pred_PC,
                adrp_val, format_names[D_in->format], status);

            break;
        case S_DECODE:
            get_stat_str(status, X_in->status);
            uint64_t val_b, val_imm;
            const char *cond;
            uint8_t dst;
            if (debug_level > 2) {
                val_b = X_in->X_sigs.valb_sel || D_out->op == OP_STUR
                            ? X_in->val_b
                            : 0;
                val_imm = X_in->X_sigs.valb_sel ? 0 : X_in->val_imm;
                cond = D_out->op == OP_B_COND ? cond_names[X_in->cond] : "N/A";
                dst = X_in->W_sigs.w_enable ? X_in->dst : 0;
            } else {
                val_b = X_in->val_b;
                val_imm = X_in->val_imm;
                cond = cond_names[X_in->cond];
                dst = X_in->dst;
            }
            printf(
                "D: %-6s[val_a, val_b, imm] = [0x%lX, 0x%lX, 0x%lX], alu_op: "
                "%s, cond: %s, dst: X%d, status: %s\n",
                D_out->print_op != OP_ERROR ? opcode_names[D_out->print_op]
                                            : "ERR",
                X_in->val_a, val_b, val_imm, alu_op_names[X_in->ALU_op], cond,
                dst, status);

            if (debug_level == 1)
                break;

            printf(
                "\t X_sigs: [vala_sel, valb_sel, set_flags] = [%s, %s, %s]\n",
                X_in->X_sigs.vala_sel ? "true" : "false",
                X_in->X_sigs.valb_sel ? "true" : "false",
                X_in->X_sigs.set_flags ? "true" : "false");

            printf("\t M_sigs: [dmem_read, dmem_write] = [%s, %s]\n",
                   X_in->M_sigs.dmem_read ? "true " : "false",
                   X_in->M_sigs.dmem_write ? "true" : "false");

            printf("\t W_sigs: [dst_sel, wval_sel, w_enable] = [%s, %s, %s]\n",
                   X_in->W_sigs.dst_sel ? "true " : "false",
                   X_in->W_sigs.wval_sel ? "true " : "false",
                   X_in->W_sigs.w_enable ? "true" : "false");

            break;

        case S_EXECUTE:
            get_stat_str(status, M_in->status);
            uint64_t eval_b, eval_imm;
            uint8_t val_hw;
            if (debug_level > 2) {
                eval_b = X_out->X_sigs.valb_sel || X_out->op == OP_STUR
                             ? X_out->val_b
                             : 0;
                eval_imm = X_out->X_sigs.valb_sel ? 0 : X_out->val_imm;
                val_hw = X_out->ALU_op == MOV_OP ? X_out->val_hw : 0;
            } else {
                eval_b = X_out->val_b;
                eval_imm = X_out->val_imm;
                val_hw = X_out->val_hw;
            }
            printf(
                "X: %-6s[val_ex, a, b, imm, hw] = [0x%lX, 0x%lX, 0x%lX, 0x%lX, "
                "0x%X], alu_op: %s, status: %s\n",
                X_out->print_op != OP_ERROR ? opcode_names[X_out->print_op]
                                            : "ERR",
                M_in->val_ex, X_out->val_a, eval_b, eval_imm, val_hw,
                alu_op_names[X_out->ALU_op], status);
            printf("\t X_condval: %s\n", M_in->cond_holds ? "true" : "false");

            if (debug_level == 1)
                break;

            // debug level 2: also print control signals

            printf(
                "\t X_sigs: [vala_sel, valb_sel, set_flags] = [%s, %s, %s]\n",
                X_out->X_sigs.vala_sel ? "true" : "false",
                X_out->X_sigs.valb_sel ? "true" : "false",
                X_out->X_sigs.set_flags ? "true" : "false");

            break;
        case S_MEMORY:
            get_stat_str(status, W_in->status);
            uint64_t val_mem, mval_b;
            if (debug_level > 2) {
                val_mem = M_out->M_sigs.dmem_read || M_out->M_sigs.dmem_write
                              ? W_in->val_mem
                              : 0;
                mval_b = M_out->M_sigs.dmem_write ? M_out->val_b : 0;
            } else {
                val_mem = W_in->val_mem;
                mval_b = M_out->val_b;
            }
            printf("M: %-6s[val_ex, val_b, val_mem] = [0x%lX, 0x%lX, 0x%lX], "
                   "status: %s\n",
                   M_out->print_op != OP_ERROR ? opcode_names[M_out->print_op]
                                               : "ERR",
                   M_out->val_ex, mval_b, val_mem, status);

            if (debug_level == 1)
                break;

            // debug level 2: also print control signals

            printf("\t M_sigs: [dmem_read, dmem_write] = [%s, %s]\n",
                   M_out->M_sigs.dmem_read ? "true " : "false",
                   M_out->M_sigs.dmem_write ? "true" : "false");

            break;
        case S_WBACK:
            get_stat_str(status, W_out->status);
            uint8_t wdst;
            uint64_t val_ex, wval_mem;
            if (debug_level > 2) {
                if (!W_out->W_sigs.w_enable) {
                    wdst = 0;
                    val_ex = 0;
                    wval_mem = 0;
                } else {
                    wdst = W_out->dst;
                    val_ex = W_out->W_sigs.wval_sel ? 0 : W_out->val_ex;
                    wval_mem = W_out->W_sigs.wval_sel ? W_out->val_mem : 0;
                }
            } else {
                wdst = W_out->dst;
                val_ex = W_out->val_ex;
                wval_mem = W_out->val_mem;
            }
            printf("W: %-6s[dst, val_ex, val_mem] = [X%d, 0x%lX, 0x%lX], "
                   "status: %s\n",
                   W_out->print_op != OP_ERROR ? opcode_names[W_out->print_op]
                                               : "ERR",
                   wdst, val_ex, wval_mem, status);

            if (debug_level == 1)
                break;

            // debug level 2: also print control signals

            printf("\t W_sigs: [dst_sel, wval_sel, w_enable] = [%s, %s, %s], "
                   "W_wval: 0x%lx\n",
                   W_out->W_sigs.dst_sel ? "true " : "false",
                   W_out->W_sigs.wval_sel ? "true " : "false",
                   W_out->W_sigs.w_enable ? "true" : "false", W_wval);
            break;
        default:
            IMPOSSIBLE();
            break;
    }
    return;
}
