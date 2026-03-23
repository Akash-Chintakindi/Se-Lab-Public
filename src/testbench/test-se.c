/******************x********************************************************
 * STUDENTS: DO NOT MODIFY.
 *
 * C S 429 system emulator
 *
 * test_se.c - Test harness for the emulator.
 *
 * Copyright (c) 2023, 2024, 2025.
 * Authors: Z. Leeper., K. Chandrasekhar, K. Rathod, W. Borden
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_STR 1024 /* Max string size */

int verbosity;

/*
 * usage - Prints usage info
 */
void usage(char *argv[]) {
    printf("Usage: %s [-hwv]\n", argv[0]);
    printf("Options:\n");
    printf("  -h        Print this help message.\n");
    printf("  -w <num>  Test a specific week. Defaults to week 2.\n");
    printf("  -v <num>  Verbosity level. Defaults to 0, which only shows final "
           "score.\n            Set to 1 to view which tests are failing.\n    "
           "        Set to 2 to view all tests as they run.\n");
#ifdef EC
    printf("  -e        Test extra credit test cases. Disabled by default.\n");
#endif
}

/*
 * SIGALRM handler
 */
void sigalrm_handler(int signum) {
    printf("Error: Program timed out.\n");
    exit(EXIT_FAILURE);
}

/*
 * runtrace - Runs the student and reference solution with provided parameters,
 * on a given test, and collects the results for the caller.
 * Return 0 if any problems, 1 if OK.
 */
static int run_test(int wk, int A, int B, int C, int d, char *testdir,
                    char *testname, int limit) {
    int status;
    char cmd[MAX_STR];
    char checkpoint_ref[MAX_STR >> 4];
    char checkpoint_student[MAX_STR >> 4];
    char testfile[MAX_STR >> 4];

    checkpoint_ref[0] = 0;
    checkpoint_student[0] = 0;
    strcpy(checkpoint_ref, "checkpoints/checkpoint_ref_");
    strcpy(checkpoint_student, "checkpoints/checkpoint_student_");
    strcat(checkpoint_ref, testname);
    strcat(checkpoint_student, testname);
    strcat(checkpoint_ref, ".out");
    strcat(checkpoint_student, ".out");

    testfile[0] = 0;
    strcpy(testfile, testdir);
    strcat(testfile, testname);

    /* Run the reference emulator */
    sprintf(cmd,
            "./bin/se-ref-wk%d -A %d -B %d -C %d -d %d -l %d -i %s -c %s > "
            "/dev/null 2> /dev/null",
            wk, A, B, C, d, limit, testfile, checkpoint_ref);
    status = system(cmd);
    if (status == -1) {
        fprintf(stderr, "Error invoking system() for reference sim: %s\n",
                strerror(errno));
        return 0;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Error running reference emulator: Status %d\n",
                WEXITSTATUS(status));
        return 0;
    }

    /* Run the student emulator */
    if (verbosity > 1) {
        sprintf(
            cmd,
            "./bin/se -A %d -B %d -C %d -d %d -l %d -i %s -c %s > /dev/null", A,
            B, C, d, limit, testfile, checkpoint_student);
    } else {
        sprintf(cmd,
                "./bin/se -A %d -B %d -C %d -d %d -l %d -i %s -c %s > "
                "/dev/null 2> /dev/null",
                A, B, C, d, limit, testfile, checkpoint_student);
    }

    status = system(cmd);
    if (status == -1) {
        if (verbosity > 0) {
            fprintf(stderr, "Error invoking system() for reference sim: %s\n",
                    strerror(errno));
        }
        return 0;
    }
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        if (verbosity > 0) {
            fprintf(stderr, "Error running student emulator: Status %d\n",
                    WEXITSTATUS(status));
        }
        return 0;
    }

    /* Diff the results */
    sprintf(cmd, "diff %s %s", checkpoint_ref, checkpoint_student);
    status = system(cmd);

    int pass = 1;
    // Conveniently, diff gives a nonzero exit status if they don't match.
    if (status)
        pass = 0;

    /* Cleanup */
    sprintf(cmd, "rm -f %s", checkpoint_ref);
    status = system(cmd);
    if (status == -1) {
        fprintf(stderr, "Error removing file %s: %s\n", checkpoint_ref,
                strerror(errno));
        return 0;
    }
    sprintf(cmd, "rm -f %s", checkpoint_student);
    status = system(cmd);
    if (status == -1) {
        fprintf(stderr, "Error removing file %s: %s\n", checkpoint_student,
                strerror(errno));
        return 0;
    }

    return pass;
}

double run_tests_dir(int week_num, char *testdirs[], size_t num_testdirs,
                     double weights[], size_t *total_num_tests, int *total,
                     int cache_configs[][4], int num_cache_configs) {
    int cycle_limit;
    if (week_num == 2) {
        cycle_limit = 500;
    } else if (week_num == 3) {
        cycle_limit = 1 << 13;
    } else if (week_num == 4) {
        cycle_limit = 1 << 26;
    }
    if (verbosity > 1) {
        fprintf(stderr, "Running tests for week %d...\n", week_num);
    }
    *total_num_tests = 0;
    *total = 0;
    double this_score = 0.0;

    for (size_t dirnum = 0; dirnum < num_testdirs; dirnum++) {
        struct dirent *dp;
        DIR *dfd;
        if ((dfd = opendir(testdirs[dirnum])) == NULL) {
            fprintf(stderr, "Can not open %s\n", testdirs[dirnum]);
            exit(EXIT_FAILURE);
        }

        int num_tests = 0;
        double passed_this_dir = 0;

        while ((dp = readdir(dfd)) != NULL) {
            char *name_cpy = malloc(strlen(dp->d_name) + 1);
            strcpy(name_cpy, dp->d_name);
            char *tok = strtok(name_cpy, ".");

            tok = strtok(NULL, ".");
            if (!tok && strcmp(dp->d_name, ".") && strcmp(dp->d_name, "..")) {
                // printf("%s\n", dp->d_name);
                for (int i = 0; i < num_cache_configs; i++) {
                    if (verbosity > 1) {
                        if (week_num < 4) {
                            fprintf(stderr, "Running %s%s\n", testdirs[dirnum],
                                    dp->d_name);
                        } else {
                            fprintf(stderr,
                                    "Running %s%s with cache config [A,B,C,d] "
                                    "= [%d,%d,%d,%d]\n",
                                    testdirs[dirnum], dp->d_name,
                                    cache_configs[i][0], cache_configs[i][1],
                                    cache_configs[i][2], cache_configs[i][3]);
                        }
                    }
                    int pass = run_test(
                        week_num, cache_configs[i][0], cache_configs[i][1],
                        cache_configs[i][2], cache_configs[i][3],
                        testdirs[dirnum], dp->d_name, cycle_limit);
                    passed_this_dir += pass;

                    if (!pass && verbosity > 0) {
                        fprintf(stderr, "Failed test %s%s\n", testdirs[dirnum],
                                dp->d_name);
                    }
                    num_tests++;
                }
            }
            free(name_cpy);
        }

        *total_num_tests += num_tests;
        *total += passed_this_dir;
        this_score += (weights[dirnum] * (passed_this_dir / num_tests));
    }
    return this_score;
}

int main(int argc, char *argv[]) {
    char c;
    int week = 2;
    verbosity = 0;
    int ec = 0;
    /* Parse command line args. */
    while ((c = getopt(argc, argv, ":h:w:v:e")) != -1) {
        switch (c) {
            case 'h':
                usage(argv);
                exit(EXIT_SUCCESS);
            case 'w':
                week = atoi(optarg);
                break;
            case 'v':
                verbosity = atoi(optarg);
                break;
#ifdef EC
            case 'e':
                ec = 1;
                break;
#endif
            default:
                usage(argv);
                exit(EXIT_FAILURE);
        }
    }

    if (week == 1)
        week = 2;

    /* Check for valid command line args. */
    if (!ec && (week < 2 || week > 4)) {
        fprintf(stderr,
                "Error: Week must be between 2 and 4 or left unspecified.\n");
        exit(EXIT_FAILURE);
    }

#ifdef EC
    if (ec && week == 2) {
        fprintf(stderr,
                "Error: Week must be at least 3 to test extra credit.\n");
        exit(EXIT_FAILURE);
    }
#endif

    /* Install timeout handler */
    if (signal(SIGALRM, sigalrm_handler) == SIG_ERR) {
        fprintf(stderr, "Unable to install SIGALRM handler\n");
        exit(EXIT_FAILURE);
    }

    /* Time out and give up after a while in case of infinite loops */
    alarm(200);

    double score = 0.0;

    /* If you add a new directory for testcases then you gotta:
        1. add the directory to the correct week's array
        2. increase the num in dir_nums
        3. adjust the corresponfing week's weights
       I will not explain what needs to be done for removing directory cause you
       are a lead TA on SE so you are probably smart. We can now easily add new
       testcases, YIPPEEE!!!
    */
    char *testdirs_w2[] = {
        "testcases/basics/",           "testcases/alu/pipeminus/",
        "testcases/mem/pipeminus/",    "testcases/alu/print_pipeminus/",
        "testcases/branch/pipeminus/", "testcases/exceptions/pipeminus/"};
    char *testdirs_w3[] = {
        "testcases/alu/hazard/",          "testcases/mem/hazard/",
        "testcases/branch/hazard/",       "testcases/exceptions/hazard/",
        "testcases/applications/hazard/", "testcases/applications/hard/"};
    char *testdirs_w4[] = {"testcases/mem/pipeminus/",
                           "testcases/applications/hard/"};
    char **testdirs[] = {testdirs_w2, testdirs_w3, testdirs_w4};

    int dir_nums[] = {6, 6, 2}; // number of dirs for weeks 2, 3, 4

    // this is a (kinda) hacky fix :(
    double weights[][6] = {
        {0.1, 0.25, 0.25, 0.15, 0.15, 0.1}, // weights for week 2
        {0.2, 0.2, 0.2, 0.1, 0.2, 0.1},     // weights for week 3
        {0.2, 0.8, 0.0, 0.0, 0.0,
         0.0}}; // weights for week 4 (had to add extra zeros to make not an
                // incomplete type)

    int no_cache_config[][4] = {
        {-1, -1, -1, -1}}; // cache config for weeks 2 and 3
    int cache_configs_w4[][4] = {{1, 8, 8, 2},
                                 {4, 8, 32, 4},
                                 {1, 32, 64, 8},
                                 {2, 8, 64, 8},
                                 {4, 32, 512, 100}}; // cache configs for week 4

    int num_cache_configs[] = {1, 1,
                               5}; // number of cache configs for weeks 2, 3, 4

    size_t total_num_tests = 0;
    int total = 0;
    int index = week - 2; // i didnt want to type week - 2 a ton

    score += run_tests_dir(week, testdirs[index], dir_nums[index],
                           weights[index], &total_num_tests, &total,
                           week < 4 ? no_cache_config : cache_configs_w4,
                           num_cache_configs[index]);

    if (verbosity > 0) {
        fprintf(stderr, "Total week %d tests passed: %d of %ld.\n", week, total,
                total_num_tests);
    }

    // Needs to be stdout for gradescope
    if (week == 2) {
        score *= 6;
    } else if (week == 3) {
        score *= 4;
    }

    if (1 < week && week < 5) {
        printf("Total score for week %d SE: %0.2f\n", week, score);
    }

#ifdef EC
    double ec_score = 0;
    // EXTRA CREDIT
    if (ec) {
        if (verbosity > 1) {
            fprintf(stderr,
                    "Running tests for charmv5plus (extra credit)...\n");
        }

        int cache_config_ec[] = {1, 8, 8, 2};
        if (week < 4) {
            cache_config_ec[0] = -1;
            cache_config_ec[1] = -1;
            cache_config_ec[2] = -1;
            cache_config_ec[3] = -1;
        }

        char *testdir = week == 2 ? "testcases/charmv5plus/pipeminus/"
                                  : "testcases/charmv5plus/hazard/";

        char *tests_ec[] = {"csel_simple",  "csel_less_simple",
                            "csinc_simple", "csinc_less_simple",
                            "csinv_simple", "csinv_less_simple",
                            "csneg_simple", "csneg_less_simple",
                            "cbz",          "cbnz",
                            "knapsack",     "br",
                            "blr",          "csel_four"};

        size_t num_tests_ec = 14;
        double passed_ec = 0;
        for (size_t testnum = 0; testnum < num_tests_ec; testnum++) {
            if (verbosity > 1) {
                fprintf(stderr,
                        "Running %s%s with cache config [A,B,C,d] = "
                        "[%d,%d,%d,%d]\n",
                        testdir, tests_ec[testnum], cache_config_ec[0],
                        cache_config_ec[1], cache_config_ec[2],
                        cache_config_ec[3]);
            }
            int pass = run_test(3, cache_config_ec[0], cache_config_ec[1],
                                cache_config_ec[2], cache_config_ec[3], testdir,
                                tests_ec[testnum], 1 << 26);
            passed_ec += pass;
            if (!pass && verbosity > 0) {
                fprintf(stderr, "Failed test %s%s\n", testdir,
                        tests_ec[testnum]);
            }
        }

        int total_ec = passed_ec;

        ec_score += 100.0 * (passed_ec / num_tests_ec);

        if (verbosity > 0) {
            fprintf(stderr, "Total charmv5plus tests passed: %d of %ld.\n",
                    total_ec, num_tests_ec);
        }

        printf("Total percent score for charmv5plus: %0.2f%%\n", ec_score);
    }
#endif
    exit(EXIT_SUCCESS);
}
