/**************************************************************************
 * C S 429 system emulator
 *
 * machine.h - Headers for machine parameters used by the emulator.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, Z. Leeper.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#ifndef _MACHINE_H_
#define _MACHINE_H_

#include "cache/cache.h"
#include "mem.h"
#include "proc.h"

// Machine state.
typedef struct machine {
    char *name;     // Descriptive name of machine
    proc_t *proc;   // Pointer to machine's processor
    mem_t *mem;     // Pointer to machine's memory
    cache_t *cache; // Pointer to machine's cache
    // gpu_t *gpu;
} machine_t;

extern uint64_t seg_starts[]; // Starting locations of memory segments (e.g.,
                              // code, data, stack, etc.).
extern void init_machine();
extern void log_machine_state(void);
#endif