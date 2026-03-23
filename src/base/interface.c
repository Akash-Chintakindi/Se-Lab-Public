/**************************************************************************
 * STUDENTS: DO NOT MODIFY, except for line 18.
 *
 * C S 429 system simulator
 *
 * interface.c - Initialization and finalization routines for the interpreter.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, Z. Leeper.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include "ansicolors.h"
#include "archsim.h"

static char default_hw_prompt[] = ANSI_BOLD ANSI_COLOR_BLUE
    "UTCS429-S2023, 2024, 2025-archsim>>> " ANSI_RESET;
static const char author[] =
    ANSI_BOLD ANSI_COLOR_RED "Reference Implementation" ANSI_RESET;

static void print_init_msg(void) {
    time_t t;

    fprintf(outfile, "Welcome to the C S 429 System Emulator\n\n");
    fprintf(outfile, "Author: %s\n", author);
    assert(time(&t) != -1);
    fprintf(outfile, "Run begun at %s\n\n", ctime(&t));
}

void init(void) {
    if (!hw_prompt)
        hw_prompt = default_hw_prompt;
    init_machine();
    init_itable();
    init_ftable();
    init_extract_regs_table();
    if (outfile != stdout) {
        hw_prompt = "";
        return;
    }
    print_init_msg();
    return;
}

void finalize() {
    if (outfile == stdout) {
        time_t t;
        assert(time(&t) != -1);
        fprintf(outfile, "Run ended at %s\n", ctime(&t));
        fprintf(outfile, ANSI_BOLD "Goodbye!\n\n" ANSI_RESET);
    }
    if (checkpoint) {
        log_machine_state();
    }
    return;
}
