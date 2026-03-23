/**************************************************************************
 * STUDENTS: DO NOT MODIFY.
 *
 * C S 429 system emulator
 *
 * test-hw.c - Module for emulating hardware elements.
 *
 * Copyright (c) 2024, 2025.
 * Authors: Kavya Rathod, Kiran Chandrasekhar, and Prithvi Jamadagni, Wyatt
 * Borden. All rights reserved. May not be used, modified, or copied without
 * permission.
 **************************************************************************/
#include "ansicolors.h"
#include "archsim.h"
#include "err_handler.h"
#include "hw_elts.h"
#include "interface.h"
#include <getopt.h>
#include <stddef.h>
#include <stdint.h>

#include "proc.h"
#include "stdio.h"

#define ALU_TESTFILE_VERSION 5
#define REGFILE_TESTFILE_VERSION 5

#define ALU_TESTCASES_FILENAME "testcases/hw_elts/alu_hw.tb"
#define REGFILE_TESTCASES_FILENAME "testcases/hw_elts/regfile_hw.tb"
#define EC_TESTCASES_FILENAME "testcases/hw_elts/ec_hw.tb"

// these are just to help code readability
#define reference_alu alu
#define reference_regfile regfile
#define reference_regfiler regfile_read
#define reference_regfilew regfile_write
#define reference_cond_holds cond_holds

// TODO: COMMENT THIS OUT BEFORE COMPILING STUDENT VERSION
// #define GENERATE_TESTS

int verbosity;
bool extra_credit;
char *op_to_test;

machine_t guest;
opcode_t itable[2 << 11];
format_t ftable[2 << 11];
FILE *infile, *outfile, *errfile, *checkpoint;
char *infile_name;
char *hw_prompt;
uint64_t num_instr;
uint64_t cycle_max;
int debug_level;
int A, B, C, d;
uint64_t inflight_cycles;
uint64_t inflight_addr;
bool inflight;
mem_status_t dmem_status;

struct alu_test {
    uint64_t vala;         // valA to pass in
    uint64_t valb;         // valB to pass in
    uint64_t vale_correct; // expected valE result
    cond_t cond;           // condition code to check
    alu_op_t op;           // ALU op to perform
    uint8_t valhw;         // hw val to pass in
    uint8_t nzcv_input; // the NZCV value we pass into the ALU, to simulate b.cc
                        // operation. Also where the ALU will return the nzcv
                        // output.
    uint8_t nzcv_correct; // the NZCV value we expect out of the ALU for things
                          // like ADDS, SUBS, etc
    bool set_flags;       // whether to set nzcv or not
    bool condval_correct; // the expected condval result
    bool check_condval;   // if true, this testcase is testing if condval is
                          // appropriately set.
                          //   if false, this testcase is testing if nzcv is
                          //   appropriately set.
};

struct regfile_testcase {
    uint64_t valw;         // the value to write to the GPR
    uint64_t vala_correct; // the expected value to be read
    uint64_t valb_correct; // the expected value to be read
    uint8_t src1;          // the first source register
    uint8_t src2;          // the second source register
    uint8_t dst;           // the write destination
    bool w_enable;         // the write control sig to be sent
    bool isRead;           // if this is true, this testcase tests regfile_read
                           // otherwise, this testcase tests regfile_write
    gpreg_val_t GPR[31];   // the state of the GPRs after the testcase is ran
    uint64_t SP;           // the value of SP after the testcase is ran
};

struct cond_holds_testcase {
    cond_t cond;
    uint8_t ccval;

    bool cond_val;
};

struct test_results {
    unsigned long failed;
    unsigned long total;
    // currently using long as a bitset, jank but safe <= 64 ops
    unsigned long failed_ops;
};

static char *alu_op_names[] = {
    "PLUS_OP", "MINUS_OP", "INV_OP",   "OR_OP",    "EOR_OP", "AND_OP",
    "MOV_OP",  "MOVK_OP",  "LSL_OP",   "LSR_OP",   "ASR_OP", "PASS_A_OP",
    "CSEL_OP", "CSINV_OP", "CSINC_OP", "CSNEG_OP", "CBZ_OP", "CBNZ_OP"};

alu_op_t str_to_op(char *op_str) {
    if (!op_str)
        return ERROR_OP;

// TODO ew....
#ifdef EC
    for (alu_op_t op = PLUS_OP; op <= CBNZ_OP; op++) {
#else
    for (alu_op_t op = PLUS_OP; op <= PASS_A_OP; op++) {
#endif
        if (!strcmp(op_str, alu_op_names[op])) {
            return op;
        }
    }

    return ERROR_OP;
}

static char *cond_names[] = {"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
                             "HI", "LS", "GE", "LT", "GT", "LE", "AL", "NV"};

// too cooked to do something smarter
static char *nzcv_bits[] = {"0000", "0001", "0010", "0011", "0100", "0101",
                            "0110", "0111", "1000", "1001", "1010", "1011",
                            "1100", "1101", "1110", "1110"};

static void print_alu_ops(unsigned long ops) {
    for (unsigned long i = 0; i < (sizeof(ops) * 64); i++) {
        bool activated = ops & 1;
        if (activated) {
            printf("\t\t%s\n", alu_op_names[i]);
        }

        ops >>= 1;
    }
}

static void print_alu_test(struct alu_test *test, uint64_t vale, bool condval,
                           uint8_t nzcv) {
    printf("ALU: %s [a, b, hw, cond, NZCV_old, set_flags] = [0x%lX, 0x%lX, "
           "0x%X, %s, 0b%s, %s]\n",
           alu_op_names[test->op], test->vala, test->valb, test->valhw,
           cond_names[test->cond], nzcv_bits[test->nzcv_input],
           test->set_flags ? "true" : "false");
    bool correct_vale = test->vale_correct == vale;
    bool correct_condval = test->condval_correct == condval;
    bool correct_nzcv = test->nzcv_correct == nzcv;
    printf("Expected: [vale, condval, NZCV] = [%s0x%lX\x1b[0m, %s%s\x1b[0m, "
           "%s0b%s\x1b[0m]\n",
           correct_vale ? ANSI_RESET : ANSI_COLOR_GREEN, test->vale_correct,
           correct_condval ? ANSI_RESET : ANSI_COLOR_GREEN,
           test->condval_correct ? "true" : "false",
           correct_nzcv ? ANSI_RESET : ANSI_COLOR_GREEN,
           nzcv_bits[test->nzcv_correct]);
    printf("Got: \t  [vale, condval, NZCV] = [%s0x%lX\x1b[0m, %s%s\x1b[0m, "
           "%s0b%s\x1b[0m]\n",
           correct_vale ? ANSI_RESET : ANSI_COLOR_RED, vale,
           correct_condval ? ANSI_RESET : ANSI_COLOR_RED,
           condval ? "true" : "false",
           correct_nzcv ? ANSI_RESET : ANSI_COLOR_RED, nzcv_bits[nzcv]);
}

static void print_regfile(gpreg_val_t *GPR, uint64_t SP, gpreg_val_t *otherGPR,
                          uint64_t otherSP, bool isRef) {
    char *colors[] = {ANSI_RESET, ANSI_COLOR_RED, ANSI_COLOR_GREEN};

    for (int i = 0; i < 31; i++) {
        printf("%sX%d: \t%6lX%s\t",
               GPR[i] == otherGPR[i] ? colors[0] : colors[1 + isRef], i, GPR[i],
               ANSI_RESET);

        if ((i + 1) % 3 == 0)
            putchar('\n');
    }

    printf("%sSP: \t%6lX%s\n", SP == otherSP ? colors[0] : colors[1 + isRef],
           SP, ANSI_RESET);
}

static void print_regfile_test(struct regfile_testcase *test, uint64_t vala,
                               uint64_t valb) {
    printf("Regfile: [src1, src2, dst, valw, w_enable] = [X%d, X%d, X%d, %lX, "
           "%d]\n",
           test->src1, test->src2, test->dst, test->valw, test->w_enable);
    printf("Expected: [vala, valb] = [%s0x%lX%s, %s0x%lX%s]\n",
           test->vala_correct == vala ? ANSI_RESET : ANSI_COLOR_GREEN,
           test->vala_correct, ANSI_RESET,
           test->valb_correct == valb ? ANSI_RESET : ANSI_COLOR_GREEN,
           test->valb_correct, ANSI_RESET);
    printf("Got: [vala, valb] = [%s0x%lX%s, %s0x%lX%s]\n",
           test->vala_correct == vala ? ANSI_RESET : ANSI_COLOR_RED, vala,
           ANSI_RESET, test->valb_correct == valb ? ANSI_RESET : ANSI_COLOR_RED,
           valb, ANSI_RESET);

    printf("\nRegister values (expected): \n");
    print_regfile(test->GPR, test->SP, guest.proc->GPR, guest.proc->SP, true);

    printf("\nRegister values (got): \n");
    print_regfile(guest.proc->GPR, guest.proc->SP, test->GPR, test->SP, false);
}

/*
 * usage - Prints usage info
 */
void usage(char *argv[]) {
    printf("Usage: %s [-hvo]\n", argv[0]);
    printf("Options:\n");
    printf("  -h        Print this help message.\n");
    printf(
        "  -v <num>  Verbosity level. Defaults to 0, which only shows final "
        "score.\n            Set to 1 to view which testbenches are failing.\n "
        "           Set to 2 to stop and print on any failed tests.\n");
#ifdef EC
    printf("  -e        Extra Credit. If enabled, runs testcases for EC "
           "instructions.\n");
#endif
    printf("  -a        Test only the ALU.\n");
    printf("  -r        Test only the regfile.\n");
    printf("  -o <op>   Operation. Specify the alu_op_t to test.\n");
}

struct regfile_testcase new_regfile_testcase() {
    return (struct regfile_testcase){.valw = 0,
                                     .vala_correct = 0,
                                     .valb_correct = 0,
                                     .src1 = 0,
                                     .src2 = 0,
                                     .dst = 0,
                                     .w_enable = false,
                                     .isRead = true,
                                     .GPR = {0},
                                     .SP = 0};
}

struct cond_holds_testcase new_cond_holds_testcase() {
    return (struct cond_holds_testcase){
        .cond = C_ERROR, .ccval = 0, .cond_val = false};
}

void copy_regfile_state(struct regfile_testcase *testcase) {
    memcpy(&testcase->GPR, &guest.proc->GPR, sizeof(gpreg_val_t) * 31);
    memcpy(&testcase->SP, &guest.proc->SP, sizeof(uint64_t));
}

struct alu_test new_alu_testcase() {
    return (struct alu_test){
        .vala = 0,
        .valb = 0,
        .vale_correct = 0,
        .cond = C_AL,
        .op = PASS_A_OP,
        .valhw = 0,
        .nzcv_input = PACK_CC(0, 0, 0, 0),
        .nzcv_correct = PACK_CC(0, 0, 0, 0),
        .set_flags = false, // if true and check_condval is false, will check
                            // generated nzcv.
        .condval_correct = false,
        .check_condval = false // if true, check the condval.
    };
}

struct test_results run_alu_tests(char *input_filename) {
    struct alu_test testcase;

    uint64_t vale_actual;    // the actual valE the ALU returns
    bool condval_actual;     // the actual condval the ALU generates
    uint8_t nzcv_input;      // the input nzcv to the ALU
    uint8_t nzcv_actual = 0; // the actual nzcv the ALU generates

    logging(LOG_INFO, "Opening ALU testcase file: alu_hw.tb");
    FILE *input_file = fopen(input_filename, "rb");
    if (input_file == NULL) {
        logging(LOG_FATAL, "Failed to open ALU testcase file");
    }

    // read header, a string "ALU!"
    char header[4];
    fread(header, sizeof(char), 4, input_file);
    if (strncmp(header, "ALU!", 4) != 0) {
        logging(LOG_FATAL, "ALU testcase file is wrong format");
    }

    int version;
    fread(&version, sizeof(int), 1, input_file);
    if (version != ALU_TESTFILE_VERSION) {
        logging(LOG_FATAL, "ALU testcase file is wrong version");
    }

    long num_testcases;
    fread(&num_testcases, sizeof(long), 1, input_file);

    struct test_results ret = (struct test_results){
        .failed = 0UL, .total = num_testcases, .failed_ops = 0UL};

    alu_op_t alu_op_to_test = str_to_op(op_to_test);
    if (alu_op_to_test == ERROR_OP && op_to_test != NULL) {
        // TODO throw an error
        char buffer[50];
        buffer[0] = '\0';
        strcpy(buffer, "Invalid ALU operation: ");
        strncat(buffer, op_to_test, 20);
        logging(LOG_ERROR, buffer);
        exit(0);
    } else if (op_to_test != NULL) {
        char buffer[50];
        buffer[0] = '\0';
        strcpy(buffer, "Testing operation: ");
        strncat(buffer, op_to_test, 20);
        logging(LOG_INFO, buffer);
    }

    long vale_fails = 0;
    long condval_fails = 0;
    long nzcv_fails = 0;
    while (num_testcases-- > 0) {
        // TODO: Possible deobfuscation of the input testcase? do we need that?

        // read the testcase
        fread(&testcase, sizeof(struct alu_test), 1, input_file);

        if (testcase.op != alu_op_to_test && alu_op_to_test != ERROR_OP) {
            continue;
        }

        nzcv_actual =
            0; // reset so it doesn't show up as different when it doesnt matter
        nzcv_input = testcase.nzcv_input;

        // run the testcase
        alu(testcase.vala, testcase.valb, testcase.valhw, nzcv_input,
            testcase.op, testcase.set_flags, testcase.cond, &vale_actual,
            &condval_actual, &nzcv_actual);

        // check the testcase
        if (vale_actual != testcase.vale_correct) {
            if (verbosity > 1) {
                print_alu_test(&testcase, vale_actual, condval_actual,
                               nzcv_actual);
                logging(LOG_ERROR, "Failed ALU test with verbosity >= 2 due to "
                                   "mismatched val_e.");
                exit(EXIT_FAILURE);
            }

            ret.failed_ops |= (1UL << testcase.op);
            vale_fails++; // if the valE didn't match
        } else if (testcase.check_condval &&
                   (condval_actual != testcase.condval_correct)) {
            if (verbosity > 1) {
                print_alu_test(&testcase, vale_actual, condval_actual,
                               nzcv_actual);
                logging(LOG_ERROR, "Failed ALU test with verbosity >= 2 due to "
                                   "mismatched condval.");
                exit(EXIT_FAILURE);
            }
            ret.failed_ops |= (1UL << testcase.op);
            condval_fails++; // if we are to check condval, and the condval
                             // didn't match
        } else if (testcase.set_flags &&
                   (nzcv_actual != testcase.nzcv_correct)) {
            if (verbosity > 1) {
                print_alu_test(&testcase, vale_actual, condval_actual,
                               nzcv_actual);
                logging(LOG_ERROR, "Failed ALU test with verbosity >= 2 due to "
                                   "mismatched NZCV.");
                exit(EXIT_FAILURE);
            }
            ret.failed_ops |= (1UL << testcase.op);
            nzcv_fails++; // if we are to check nzcv, and the nzcv didn't match
        }
    }

    // close testfile
    fclose(input_file);

    ret.failed = vale_fails + condval_fails + nzcv_fails;
    return ret;
}

void alu_simple_tests(FILE *output_file) {
    // a couple of simple tests for easy debugging.
    struct alu_test testcase;
    // easy tests for plus op
    testcase = new_alu_testcase(); // default value init
    testcase.vala = 0x5;
    testcase.valb = 0x7;
    testcase.op = PLUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0x115;
    testcase.valb = 0x314;
    testcase.op = PLUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // easy tests for minus op
    testcase = new_alu_testcase();
    testcase.vala = 0xA;
    testcase.valb = 0x8;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 100;
    testcase.valb = 27;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // easy tests for or op
    testcase = new_alu_testcase();
    testcase.vala = 0xDEAD0000;
    testcase.valb = 0xBEEF;
    testcase.op = OR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0xBEEF;
    testcase.valb = 0xDEAD0000;
    testcase.op = OR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // easy tests for eor op
    testcase = new_alu_testcase();
    testcase.vala = 0;
    testcase.valb = 0;
    testcase.op = EOR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0;
    testcase.valb = 1;
    testcase.op = EOR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 1;
    testcase.valb = 0;
    testcase.op = EOR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 1;
    testcase.valb = 1;
    testcase.op = EOR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // easy tests for and op
    testcase = new_alu_testcase();
    testcase.vala = 0xDEADBEEF;
    testcase.valb = 0xFFFF;
    testcase.op = AND_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0xDEADBEEF;
    testcase.valb = 0xFFFF0000;
    testcase.op = AND_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // easy tests for inv op
    testcase = new_alu_testcase();
    testcase.vala = 0;
    testcase.valb = 0xFFFFFF;
    testcase.op = INV_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0x0;
    testcase.valb = 0x0;
    testcase.op = INV_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0x429;
    testcase.valb = 0x429;
    testcase.op = INV_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // simple tests for lsl op
    testcase = new_alu_testcase();
    testcase.vala = 0x1;
    testcase.valb = 0x3;
    testcase.op = LSL_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0x5;
    testcase.valb = 0x20;
    testcase.op = LSL_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // simple tests for lsr op
    testcase = new_alu_testcase();
    testcase.vala = 0x10;
    testcase.valb = 0x4;
    testcase.op = LSR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0xDEAD00000000;
    testcase.valb = 0x20;
    testcase.op = LSR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // simple tests for asr op
    testcase = new_alu_testcase();
    testcase.vala = 0xF000;
    testcase.valb = 0xC;
    testcase.op = ASR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0x8000000000000000;
    testcase.valb = 0x3F;
    testcase.op = ASR_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // simple tests for mov op
    testcase = new_alu_testcase();
    testcase.vala = 0;
    testcase.valb = 0x429;
    testcase.op = MOV_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0xDEAD0000;
    testcase.valb = 0xBEEF;
    testcase.op = MOV_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    testcase = new_alu_testcase();
    testcase.vala = 0xBEEF;
    testcase.valb = 0xDEAD;
    testcase.valhw = 16;
    testcase.op = MOV_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);
}

void alu_simple_tests_flags(FILE *output_file) {
    // simple testcases that sets flags for easier debugging
    // includes some edge cases to check for.
    struct alu_test testcase;
    // simple tests for plus op
    // no flags set
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 6;
    testcase.valb = 5;
    testcase.op = PLUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // negative flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = -20;
    testcase.valb = 5;
    testcase.op = PLUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // zero flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 0;
    testcase.valb = 0;
    testcase.op = PLUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // carry flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 0xFFFFFFFFFFFFFFFF;
    testcase.valb = 2;
    testcase.op = PLUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // positve overflow
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 0x7FFFFFFFFFFFFFFF;
    testcase.valb = 1;
    testcase.op = PLUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // negative overflow
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 0x8000000000000000;
    testcase.valb = 0x8000000000000000;
    testcase.op = PLUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // simple test cases for minus op
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 8;
    testcase.valb = 4;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // set the zero and carry flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 256;
    testcase.valb = 256;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // set the negative flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 8;
    testcase.valb = 256;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // set the negative and carry flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = -10;
    testcase.valb = -3;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // set the carry flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 8;
    testcase.valb = 4;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // set the carry flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 4;
    testcase.valb = -8;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // set overflow
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 0x7FFFFFFFFFFFFFFF;
    testcase.valb = -1;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // just adding this since it is in an se testcase
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = -1;
    testcase.valb = 0x8000000000000000;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // simple tests for and op
    // no flags
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 0xFFFF;
    testcase.valb = 0x1;
    testcase.op = AND_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // negative flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 0xFFFFFFFFFFFFFFFF;
    testcase.valb = 0x8000000000000000;
    testcase.op = AND_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // zero flag
    testcase = new_alu_testcase();
    testcase.set_flags = true;
    testcase.vala = 0xFFFFFFFFFFFFFFFF;
    testcase.valb = 0;
    testcase.op = AND_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);
}

void alu_tb(alu_op_t op, FILE *output_file) {
    // 100 tests that just do an ALU operation
    struct alu_test testcase;
    for (int i = 0; i < 100; i++) {
        testcase = new_alu_testcase(); // default value initializer
        testcase.vala = (((uint64_t) rand()) << 32) | rand();
        testcase.valb = (((uint64_t) rand()) << 32) | rand();

        testcase.op = op;
        // run the reference alu, directing it's results into the testcase
        reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0,
                      testcase.op, testcase.set_flags, testcase.cond,
                      &testcase.vale_correct, &testcase.condval_correct,
                      &testcase.nzcv_correct);
        fwrite(&testcase, sizeof(testcase), 1, output_file);
    }
}

void alu_tb_with_hw(alu_op_t op, FILE *output_file) {
    // 100 tests that do ALU operations involving hw values.
    // realistically this is only the MOV op.
    struct alu_test testcase;
    for (int i = 0; i < 100; i++) {
        testcase = new_alu_testcase(); // default value initializer
        testcase.vala = (((uint64_t) rand()) << 32) | rand();
        testcase.valb = (((uint64_t) rand() % 65536));
        testcase.op = op;
        testcase.valhw = (uint8_t) ((uint64_t) rand() % 4) * 16;

        // run the reference alu, directing it's results into the testcase
        reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0,
                      testcase.op, testcase.set_flags, testcase.cond,
                      &testcase.vale_correct, &testcase.condval_correct,
                      &testcase.nzcv_correct);
        fwrite(&testcase, sizeof(testcase), 1, output_file);
    }
}

void alu_tb_with_set_flags(alu_op_t op, FILE *output_file) {
    // 100 tests that just do an ALU operation
    // Also sets cc.
    struct alu_test testcase;
    for (int i = 0; i < 100; i++) {
        testcase = new_alu_testcase(); // default value initializer
        testcase.set_flags = true;
        testcase.vala = (((uint64_t) rand()) << 32) | rand();
        testcase.valb = (((uint64_t) rand()) << 32) | rand();
        testcase.op = op;
        // run the reference alu, directing it's results into the testcase
        reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0,
                      testcase.op, testcase.set_flags, testcase.cond,
                      &testcase.vale_correct, &testcase.condval_correct,
                      &testcase.nzcv_correct);
        fwrite(&testcase, sizeof(testcase), 1, output_file);
    }
}

void alu_set_flags_zero_carry_flag(FILE *output_file) {
    struct alu_test testcase;
    // zero flag with MINUS OP
    testcase = new_alu_testcase(); // default value initializer
    testcase.set_flags = true;
    testcase.vala = (((uint64_t) rand()) << 32) | rand();
    testcase.valb = testcase.vala;
    testcase.op = MINUS_OP;
    // run the reference alu, directing it's results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // zero flag with ands
    testcase = new_alu_testcase(); // default value initializer
    testcase.set_flags = true;
    testcase.vala = (((uint64_t) rand()) << 32) | rand();
    testcase.valb = 0;
    testcase.op = AND_OP;
    // run the reference alu, directing it's results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // carry flag
    testcase = new_alu_testcase(); // default value initializer
    testcase.set_flags = true;
    testcase.vala = 0xFFFFFFFFFFFFFFFF;
    testcase.valb = 0xFFFFFFFFFFFFFFFF;
    testcase.op = PLUS_OP; // temp
    // run the reference alu, directing it's results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);

    // unsigned carry minus flag set
    testcase = new_alu_testcase(); // default val init
    testcase.set_flags = true;
    testcase.vala = 0xE;
    testcase.valb = 0xE;
    testcase.op = MINUS_OP;
    // run the reference alu, directing its results into the testcase
    reference_alu(testcase.vala, testcase.valb, testcase.valhw, 0, testcase.op,
                  testcase.set_flags, testcase.cond, &testcase.vale_correct,
                  &testcase.condval_correct, &testcase.nzcv_correct);
    fwrite(&testcase, sizeof(testcase), 1, output_file);
}

void alu_tb_with_condval(FILE *output_file) {
    // a bunch of tests that simulate a B.cc of various conditions. Goes through
    // every combination.
    struct alu_test testcase;

    for (cond_t cond = C_EQ; cond <= C_NV; cond++) {
        for (uint8_t nzcv = 0; nzcv <= 0b1111; nzcv++) {
            testcase = new_alu_testcase(); // default init
            testcase.check_condval = true;
            testcase.op = PASS_A_OP;
            testcase.cond = cond;
            testcase.nzcv_input = nzcv;
            reference_alu(testcase.vala, testcase.valb, testcase.valhw,
                          testcase.nzcv_input, testcase.op, testcase.set_flags,
                          testcase.cond, &testcase.vale_correct,
                          &testcase.condval_correct, &testcase.nzcv_input);
            testcase.nzcv_correct = nzcv;
            fwrite(&testcase, sizeof(testcase), 1, output_file);
        }
    }
}

void alu_tb_csel(alu_op_t op, FILE *output_file) {
    // 100 tests that just do an ALU operation
    struct alu_test testcase;
    for (int i = 0; i < 100; i++) {
        testcase = new_alu_testcase(); // default value initializer
        testcase.vala = (((uint64_t) rand()) << 32) | rand();
        testcase.valb = (((uint64_t) rand()) << 32) | rand();

        testcase.op = op;
        testcase.cond = C_EQ;
        testcase.nzcv_input = rand() & 0b0100;
        testcase.nzcv_correct = testcase.nzcv_input;
        testcase.set_flags = false;
        // run the reference alu, directing it's results into the testcase
        reference_alu(testcase.vala, testcase.valb, testcase.valhw,
                      testcase.nzcv_input, testcase.op, testcase.set_flags,
                      testcase.cond, &testcase.vale_correct,
                      &testcase.condval_correct, &testcase.nzcv_correct);
        fwrite(&testcase, sizeof(testcase), 1, output_file);
    }
}

void generate_alu_tests(char *output_filename) {
    FILE *output_file = fopen(output_filename, "wb");
    if (output_file == NULL) {
        logging(LOG_FATAL, "Failed to open ALU testcase file");
    }

    // write header
    fwrite("ALU!", sizeof(char), 4, output_file);

    // write version number
    int version = ALU_TESTFILE_VERSION;
    fwrite(&version, sizeof(int), 1, output_file);

    // write temporary number of tests. This is then updated after writing all
    // tests.
    long num_testcases = 0;
    fwrite(&num_testcases, sizeof(long), 1, output_file);
    long first_testcase_offset = ftell(output_file);

    // seed RNG machine
    srand(69420); // haha funni

    // Add some simple tests for easier debugging.
    alu_simple_tests(output_file);
    alu_simple_tests_flags(output_file);
    alu_set_flags_zero_carry_flag(output_file);

    alu_tb(PLUS_OP, output_file);
    alu_tb(MINUS_OP, output_file);
    alu_tb(OR_OP, output_file);
    alu_tb(EOR_OP, output_file);
    alu_tb(AND_OP, output_file);
    alu_tb(INV_OP, output_file);
    alu_tb(LSL_OP, output_file);
    alu_tb(LSR_OP, output_file);
    alu_tb(ASR_OP, output_file);
    alu_tb(PASS_A_OP, output_file);
    alu_tb_with_hw(MOV_OP, output_file);
    alu_tb_with_hw(MOVK_OP, output_file);
    alu_tb_with_set_flags(PLUS_OP, output_file);
    alu_tb_with_set_flags(MINUS_OP, output_file);
    alu_tb_with_set_flags(AND_OP, output_file);

    alu_tb_with_condval(output_file);

    num_testcases =
        (ftell(output_file) - first_testcase_offset) / sizeof(struct alu_test);
    fseek(output_file, 16 - sizeof(long), SEEK_SET);
    fwrite(&num_testcases, sizeof(long), 1, output_file);

    char buf[50];
    sprintf(buf, "Generated %ld ALU testcases.", num_testcases);
    logging(LOG_INFO, buf);

    fclose(output_file);
}

#ifdef EC
struct test_results run_alu_tests_ec(char *input_filename) {
    struct alu_test testcase;

    uint64_t vale_actual; // the actual valE the ALU returns
    bool condval_actual;  // the actual condval the ALU generates
    uint8_t nzcv_input;   // the input nzcv to the ALU
    uint8_t nzcv_actual;  // the actual nzcv the ALU generates

    logging(LOG_INFO, "Opening chArmV5plus testcase file: ec_hw.tb");
    FILE *input_file = fopen(input_filename, "rb");
    if (input_file == NULL) {
        logging(LOG_FATAL, "Failed to open chArmV5plus testcase file");
    }

    // read header, a string "ALU!"
    char header[4];
    fread(header, sizeof(char), 4, input_file);
    if (strncmp(header, "CH5!", 4) != 0) {
        logging(LOG_FATAL, "chArmV5plus testcase file is wrong format");
    }

    int version;
    fread(&version, sizeof(int), 1, input_file);
    if (version != ALU_TESTFILE_VERSION) {
        logging(LOG_FATAL, "chArmV5plus testcase file is wrong version");
    }

    long num_testcases;
    fread(&num_testcases, sizeof(long), 1, input_file);

    struct test_results ret =
        (struct test_results){.failed = 0, .total = num_testcases};

    alu_op_t alu_op_to_test = str_to_op(op_to_test);
    if (alu_op_to_test == ERROR_OP && op_to_test != NULL) {
        // TODO throw an error
        char buffer[50];
        buffer[0] = '\0';
        strcpy(buffer, "Invalid ALU operation: ");
        strncat(buffer, op_to_test, 20);
        logging(LOG_ERROR, buffer);
        exit(0);
    } else if (op_to_test != NULL) {
        char buffer[50];
        buffer[0] = '\0';
        strcpy(buffer, "Testing operation: ");
        strncat(buffer, op_to_test, 20);
        logging(LOG_INFO, buffer);
    }

    long vale_fails = 0;
    long condval_fails = 0;
    long nzcv_fails = 0;
    while (num_testcases-- > 0) {
        // TODO: Possible deobfuscation of the input testcase? do we need that?

        // read the testcase
        fread(&testcase, sizeof(struct alu_test), 1, input_file);

        // skip the testcase if it uses an unspecified op
        if (testcase.op != alu_op_to_test && alu_op_to_test != ERROR_OP) {
            continue;
        }

        nzcv_input = testcase.nzcv_input;

        // run the testcase
        alu(testcase.vala, testcase.valb, testcase.valhw, nzcv_input,
            testcase.op, testcase.set_flags, testcase.cond, &vale_actual,
            &condval_actual, &nzcv_actual);

        // check the testcase
        if (vale_actual != testcase.vale_correct) {
            if (verbosity > 1) {
                print_alu_test(&testcase, vale_actual, condval_actual,
                               nzcv_actual);
                logging(LOG_ERROR,
                        "Failed chArmv5plus test with verbosity >= 2");
                exit(EXIT_FAILURE);
            }
            ret.failed_ops |= (1UL << testcase.op); // mark operation as failed
            vale_fails++;                           // if the valE didn't match
        } else if (testcase.check_condval &&
                   (condval_actual != testcase.condval_correct)) {
            if (verbosity > 1) {
                print_alu_test(&testcase, vale_actual, condval_actual,
                               nzcv_actual);
                logging(LOG_ERROR,
                        "Failed chArmv5plus test with verbosity >= 2");
                exit(EXIT_FAILURE);
            }
            ret.failed_ops |= (1UL << testcase.op); // mark operation as failed
            condval_fails++; // if we are to check condval, and the condval
                             // didn't match
        } else if (testcase.set_flags &&
                   (testcase.nzcv_input != testcase.nzcv_correct)) {
            if (verbosity > 1) {
                print_alu_test(&testcase, vale_actual, condval_actual,
                               nzcv_actual);
                logging(LOG_ERROR,
                        "Failed chArmv5plus test with verbosity >= 2");
                exit(EXIT_FAILURE);
            }
            ret.failed_ops |= (1UL << testcase.op); // mark operation as failed
            nzcv_fails++; // if we are to check nzcv, and the nzcv didn't match
        }
    }

    // close testfile
    fclose(input_file);

    ret.failed = vale_fails + condval_fails + nzcv_fails;
    return ret;
}
#endif

#ifdef EC
void generate_alu_tests_ec(char *output_filename) {
    FILE *output_file = fopen(output_filename, "wb");
    if (output_file == NULL) {
        logging(LOG_FATAL, "Failed to open chArmv5plus testcase file");
    }

    // write charmv5plus header
    fwrite("CH5!", sizeof(char), 4, output_file);

    // write version number
    int version = ALU_TESTFILE_VERSION;
    fwrite(&version, sizeof(int), 1, output_file);

    // write temporary number of tests. This is then updated after writing all
    // tests.
    long num_testcases = 0;
    fwrite(&num_testcases, sizeof(long), 1, output_file);
    long first_testcase_offset = ftell(output_file);

    // seed RNG machine
    srand(69420); // haha funni

    alu_tb_csel(CSEL_OP, output_file);
    alu_tb_csel(CSINV_OP, output_file);
    alu_tb_csel(CSINC_OP, output_file);
    alu_tb_csel(CSNEG_OP, output_file);

    num_testcases =
        (ftell(output_file) - first_testcase_offset) / sizeof(struct alu_test);
    fseek(output_file, 16 - sizeof(long), SEEK_SET);
    fwrite(&num_testcases, sizeof(long), 1, output_file);

    char buf[50];
    sprintf(buf, "Generated %ld plus ALU testcases.", num_testcases);
    logging(LOG_INFO, buf);

    fclose(output_file);
}
#endif

struct test_results run_regfile_tests(char *input_filename) {
    struct regfile_testcase testcase;

    uint64_t vala_actual,
        valb_actual; // the actual valA and valB the regfile returns

    logging(LOG_INFO, "Opening regfile testcase file: regfile_hw.tb");
    FILE *input_file = fopen(input_filename, "rb");
    if (input_file == NULL) {
        logging(LOG_FATAL, "Failed to open regfile testcase file");
    }

    // read header, a string "REG!"
    char header[4];
    fread(header, sizeof(char), 4, input_file);
    if (strncmp(header, "REG!", 4) != 0) {
        logging(LOG_FATAL, "regfile testcase file is wrong format");
    }

    int version;
    fread(&version, sizeof(int), 1, input_file);
    if (version != REGFILE_TESTFILE_VERSION) {
        logging(LOG_FATAL, "regfile testcase file is wrong version");
    }

    long num_testcases;
    fread(&num_testcases, sizeof(long), 1, input_file);

    struct test_results ret =
        (struct test_results){.failed = 0, .total = num_testcases};

    long vala_fails = 0, valb_fails = 0, gpr_fails = 0, sp_fails = 0;

    while (num_testcases-- > 0) {
        // read a testcase
        fread(&testcase, sizeof(struct regfile_testcase), 1, input_file);
        // do different things for each type of testcase
        if (testcase.isRead) {
            // TODO: This most likely needs to change
            if (testcase.dst < 32) {
                if (testcase.dst == 31)
                    guest.proc->SP = testcase.valw;
                else
                    guest.proc->GPR[testcase.dst] = testcase.valw;
            }

            regfile_read(testcase.src1, testcase.src2, &vala_actual,
                         &valb_actual);
        } else {
            vala_actual = 0xBABECAFEFACEFEED;
            valb_actual = 0xBADA55BABE5C0DE;
            regfile_write(testcase.dst, testcase.valw, testcase.w_enable);
        }

        bool fail = false;

        if (vala_actual != testcase.vala_correct) {
            vala_fails++;
            fail = true;
        } else if (valb_actual != testcase.valb_correct) {
            valb_fails++;
            fail = true;
        } else {
            bool gpr_failure = false;

            // check if registers match (no incorrect writes)
            for (int i = 0; i < 31; i++) {
                if (testcase.GPR[i] != guest.proc->GPR[i]) {
                    gpr_failure = true;
                }
            }

            if (gpr_failure) {
                gpr_fails++;
                fail = true;
            } else if (testcase.SP != guest.proc->SP) {
                sp_fails++;
                fail = true;
            }
        }

        if (fail && verbosity > 1) {
            print_regfile_test(&testcase, vala_actual, valb_actual);
            logging(LOG_ERROR, "Failed Regfile test with verbosity >= 2");
            exit(EXIT_FAILURE);
        }
    }

    fclose(input_file);
    ret.failed = vala_fails + valb_fails + gpr_fails + sp_fails;
    return ret;
}

static uint64_t random_val() {
    uint64_t r = 0;
    size_t bits = sizeof(uint64_t) * 8;
    for (size_t i = 0; i < bits; i += 15)
        r = (r << 15) ^ (rand() & 0x7FFF);
    return r;
}

void generate_regfile_tests(char *output_filename) {
    FILE *output_file = fopen(output_filename, "wb");
    if (output_file == NULL) {
        logging(LOG_FATAL, "Failed to open regfile testcase file");
    }

    // write header
    fwrite("REG!", sizeof(char), 4, output_file);

    // write version number
    int version = REGFILE_TESTFILE_VERSION;
    fwrite(&version, sizeof(int), 1, output_file);

    // initialize
    long num_testcases = 0;
    fwrite(&num_testcases, sizeof(long), 1, output_file);

    // seed RNG machine
    srand(69420); // haha funni

    logging(LOG_INFO, "Generating regfile tests");

    // generate initial testcase
    struct regfile_testcase testcase = new_regfile_testcase();
    // we will directly write to X0 a random val, and then read X0 in both
    // sources
    testcase.dst = 0;
    testcase.src1 = 0;
    testcase.src2 = 0;
    testcase.valw = random_val();
    guest.proc->GPR[0] = testcase.valw;
    reference_regfiler(testcase.src1, testcase.src2, &testcase.vala_correct,
                       &testcase.valb_correct);
    copy_regfile_state(&testcase);
    fwrite(&testcase, sizeof(testcase), 1, output_file);
    num_testcases++;

    // first set of testcases will be reads.
    // write a random val to X[i] and then read X[i - 1] and X[i]
    for (int i = 1; i < 33; i++) {
        // write correct vals to testcase
        testcase.dst = i;
        testcase.src1 = i - 1;
        testcase.src2 = i;
        testcase.valw = random_val();
        // dont want to try to write to XZR
        if (testcase.dst < 32) {
            // write to the correct location
            if (testcase.dst == 31)
                guest.proc->SP = testcase.valw;
            else
                guest.proc->GPR[testcase.dst] = testcase.valw;
        }

        // call reference read func and write result to file
        reference_regfiler(testcase.src1, testcase.src2, &testcase.vala_correct,
                           &testcase.valb_correct);
        copy_regfile_state(&testcase);
        fwrite(&testcase, sizeof(testcase), 1, output_file);
        num_testcases++;
    }

    // we have now tested reading from every reg, including SP and XZR
    // now we will begin testing for writes to every register
    testcase.w_enable = true;
    testcase.isRead = false;
    testcase.src1 = 0;
    testcase.src2 = 0;
    testcase.vala_correct = 0xBABECAFEFACEFEED;
    testcase.valb_correct = 0xBADA55BABE5C0DE;

    for (int i = 0; i < 33; i++) {
        testcase.valw = random_val();
        // run the reference regfile write, then save results to file
        reference_regfilew(testcase.dst, testcase.valw, testcase.w_enable);
        copy_regfile_state(&testcase);
        fwrite(&testcase, sizeof(testcase), 1, output_file);
        num_testcases++;
    }
    // NOTE: to students who may read this; we dont test for reading from XZR
    // after writing to it, so you might wanna be careful of that - Wyatt :3

    // now test for w_enable being false
    testcase.w_enable = false;
    testcase.valw = 0xBADA55BABE5C0DE;

    for (int i = 0; i < 32; i++) {
        // run reference regfile write, then save results to file
        reference_regfilew(testcase.dst, testcase.valw, testcase.w_enable);
        copy_regfile_state(&testcase);
        fwrite(&testcase, sizeof(testcase), 1, output_file);
        num_testcases++;
    }

    // write updated number of testcases
    fseek(output_file, 8, SEEK_SET);
    fwrite(&num_testcases, sizeof(long), 1, output_file);
    // close the file

    char buf[50];
    sprintf(buf, "Generated %ld regfile testcases.", num_testcases);
    logging(LOG_INFO, buf);

    fclose(output_file);
}

int main(int argc, char *argv[]) {
    A = -1;
    B = -1;
    C = -1;
    d = -1;

    bool test_both = true;
    bool test_alu = false;
    bool test_regfile = false;

    /* Parse command line args. */
    char c;
    while ((c = getopt(argc, argv, "hv:eo:ar")) != -1) {
        switch (c) {
            case 'h': // Help
                usage(argv);
                exit(EXIT_SUCCESS);
            case 'v': // Verbosity
                verbosity = atoi(optarg);
                break;
#ifdef EC
            case 'e': // Extra Credit
                extra_credit = true;
                break;
#endif
            case 'a':
                test_alu = true;
                test_both = false;
                break;
            case 'r':
                test_regfile = true;
                test_both = false;
                break;
            case 'o': // Operation
                op_to_test = (char *) malloc(strlen(optarg) + 1);
                strcpy(op_to_test, optarg);
                break;
            default:
                usage(argv);
                exit(EXIT_FAILURE);
        }
    }

    infile = stdin;
    outfile = stdout;
    errfile = stderr;
    init();

#ifdef GENERATE_TESTS
    generate_alu_tests(ALU_TESTCASES_FILENAME);
    generate_regfile_tests(REGFILE_TESTCASES_FILENAME);
#ifdef EC
    if (extra_credit) {
        generate_alu_tests_ec(EC_TESTCASES_FILENAME);
    }
#endif
    memset(&guest.proc->GPR, 0, 31 * sizeof(uint64_t));
    guest.proc->SP = 0;
#endif

    struct test_results results = {0, 0, 0};
    double score = 0;

    // run ALU tests
    if (test_both || test_alu) {
        results = run_alu_tests(ALU_TESTCASES_FILENAME);

#ifdef PARTIAL
        score += (1 - (double) results.failed /
                          results.total); // Partial credit calculation;
                                          // disallowed for now
#else
        if (!results.failed)
            score += 1;
#endif

        if (results.failed == 0) {
            logging(LOG_OUTPUT, "Passed all ALU hardware tests.");
        } else {
            char buffer[50];
            sprintf(buffer, "Failed %ld ALU hardware tests.", results.failed);
            logging(LOG_ERROR, buffer);

            if (verbosity > 0) {
                char ops_buffer[50];
                // TODO is this portable enough? Works on CS machine at least
                sprintf(ops_buffer, "Failed ops: %d",
                        __builtin_popcountll(results.failed_ops));
                logging(LOG_INFO, ops_buffer);
                print_alu_ops(results.failed_ops);
            }
        }
    }

    if (test_both || test_regfile) {
        struct test_results temp =
            run_regfile_tests(REGFILE_TESTCASES_FILENAME);

#ifdef PARTIAL
        score +=
            (1 -
             (double) temp.failed /
                 temp.total); // Partial credit calculation; disallowed for now
#else
        if (!temp.failed)
            score += 1;
#endif

        results.total += temp.total;
        results.failed += temp.failed;

#ifndef PARTIAL
        if (results.failed)
            score = 0;
#endif

        if (temp.failed == 0) {
            logging(LOG_OUTPUT, "Passed all regfile hardware tests.");
        } else {
            char buffer[50];
            sprintf(buffer, "Failed %ld regfile hardware tests.", temp.failed);
            logging(LOG_ERROR, buffer);
        }
    }

    if (op_to_test == NULL && (test_both || (test_alu && test_regfile)))
        printf("Total score for HW: %.2lf\n", score);
    else
        logging(LOG_INFO,
                "Run without the -o, -a, and -r flags to display score.\n");

#ifdef EC
    if (extra_credit) {
        struct test_results results_ec =
            run_alu_tests_ec(EC_TESTCASES_FILENAME);

        if (results_ec.failed == 0) {
            logging(LOG_OUTPUT, "Passed all chArmv5plus hardware tests.");
        } else {
            char buffer[50];
            sprintf(buffer, "Failed %ld chArmv5plus hardware tests.",
                    results_ec.failed);
            logging(LOG_ERROR, buffer);

            if (verbosity > 0) {
                char ops_buffer[50];
                // TODO is this portable enough? Works on CS machine at least
                sprintf(ops_buffer, "Failed ops: %d",
                        __builtin_popcountll(results_ec.failed_ops));
                logging(LOG_INFO, ops_buffer);
                print_alu_ops(results_ec.failed_ops);
            }
        }
    }
#endif

    return 0;
}
