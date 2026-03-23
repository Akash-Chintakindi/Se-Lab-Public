/**************************************************************************
 * STUDENTS: DO NOT MODIFY.
 *
 * C S 429 system emulator
 *
 * handle_args.c - Module for handling command-line arguments.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, Z. Leeper.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include <getopt.h> // This does the job and keeps VSCode happy.

#include "archsim.h"

static char printbuf[BUF_LEN];

void usage(char *argv[]) {
    printf("Usage: %s -i <file> [OPTIONS]\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -i <file>  Input. Run se using <file> as the input binary. This "
           "argument is MANDATORY.\n");
    // TODO: what is -o supposed to do that stdout redirection can't handle
    // printf(
        // "  -o <file>  Output. Write the ouput of se to the specified file.\n");
    printf("  -c <file>  Checkpoint. Write a checkpoint of the machine state "
           "at the end of exection to the specified file.\n");
    printf("  -l <num>   Limit. Will limit the number of cycles se will run "
           "for to <num> cycles, default value is 500.\n");
    printf("  -v [0-3]   Verbosity. Controls how much diagnostic output you "
           "will see, valid values for <num> are 0 - 3.\n");
    printf("             Here is a desription of each level:\n");
    printf("       0: No outputs. (this is the default value if this flag is "
           "not specified)\n");
    printf(
        "       1: Outputs the contents of every stages' pipeline register + "
        "NZCV flags"
        " at the end of every cycle. (this is the default if <num> is not "
        "specified)\n");
    printf("       2: Same as level 1 but adds on the control signals for "
           "every stage.\n");
    printf("       3: Same as level 2 but omits some values that \"do not "
           "matter\". For example, when an add with a register and an "
           "immediate is in decode, val_b is always printed as 0.\n");
    printf(
        "  -A <num>   Associativity. The associativity of the cache to use.\n");
    printf("  -B <num>   Block size. The line size of the cache to use.\n");
    printf("  -C <num>   Capacity. The total capacity of the cache to use.\n");
    printf("  -d <num>   Delay. The number of cycles to stall for when a cache "
           "miss occurs.\n");
    printf(
        "NOTE: If any of the cache aguments are defined then all of them must "
        "be defined. The cache configuration must also be valid, if either of "
        "these conditions are not met then se will run without a cache.\n");
}

void handle_args(int argc, char **argv) {
    int option;
    infile = stdin;
    outfile = stdout;
    errfile = stderr;
    checkpoint = NULL;

    bool proper_usage = false;

    A = -1;
    B = -1;
    C = -1;
    d = -1;

    while ((option = getopt(argc, argv, "hi:o:c:l:v:A:B:C:d:")) != -1) {
        switch (option) {
            case 'h':
                usage(argv);
                exit(EXIT_SUCCESS);
            case 'i':
                infile_name = optarg;
                proper_usage = true;
                break;
            case 'o':
                if ((outfile = fopen(optarg, "w")) == NULL) {
                    outfile = stdout;
                    assert(strlen(optarg) < BUF_LEN);
                    sprintf(printbuf, "failed to open output file %s", optarg);
                    logging(LOG_FATAL, printbuf);
                    return;
                }
                break;
            case 'c':
                if ((checkpoint = fopen(optarg, "w")) == NULL) {
                    assert(strlen(optarg) < BUF_LEN);
                    sprintf(printbuf, "failed to open checkpoint file %s",
                            optarg);
                    logging(LOG_FATAL, printbuf);
                    return;
                }
                break;
            case 'l':
                cycle_max = atol(optarg);
                sprintf(printbuf, "Max cycles set to %ld.", cycle_max);
                logging(LOG_INFO, printbuf);
                break;
            case 'v':
                sprintf(printbuf, "Verbose debug logging enabled.");
                logging(LOG_INFO, printbuf);
                switch (*optarg) {
                    case '0':
                        debug_level = 0;
                        break;
                    case '1':
                        debug_level = 1;
                        break;
                    case '2':
                        debug_level = 2;
                        break;
                    case '3':
                        debug_level = 3;
                        break;
                    default:
                        sprintf(printbuf,
                                "Invalid logging level, options are 1, 2, "
                                "and 3. Defaulting to 1.");
                        logging(LOG_INFO, printbuf);
                        debug_level = 1;
                        break;
                }

                sprintf(printbuf, "Logging at level %d", debug_level);
                logging(LOG_INFO, printbuf);

                break;
            case 'A':
                A = atoi(optarg);
                break;
            case 'B':
                B = atoi(optarg);
                if (B != -1 && (B < 8 || __builtin_popcountll(B) != 1)) {
                    // invalid block size
                    printf(
                        "Block size invalid. Must be >= 8 and a power of 2:\n");
                    exit(1);
                }
                break;
            case 'C':
                C = atoi(optarg);
                break;
            case 'd':
                d = atoi(optarg);
                break;
            default:
                sprintf(printbuf, "Ignoring unknown option %c", optopt);
                logging(LOG_INFO, printbuf);
                break;
        }
    }

    if (!proper_usage) {
        usage(argv);
        exit(EXIT_FAILURE);
    }

    if (A == -1 || B == -1 || C == -1 || d == -1) {
        sprintf(printbuf,
                "Missing arguments for cache creation, running without cache.");
        logging(LOG_INFO, printbuf);
    } else if (__builtin_popcountll(C / (A * B)) != 1) {
        sprintf(printbuf, "Invalid cache configuration; the number of sets "
                          "must be a power of 2.");
        logging(LOG_INFO, printbuf);
    } else {
        sprintf(printbuf, "Running with cache.");
        logging(LOG_INFO, printbuf);
    }
    for (; optind < argc; optind++) { // when some extra arguments are passed
        assert(strlen(argv[optind]) < BUF_LEN);
        sprintf(printbuf, "Ignoring extra argument %s", argv[optind]);
        logging(LOG_INFO, printbuf);
    }
    return;
}
