/**************************************************************************
 * STUDENTS: DO NOT MODIFY.
 *
 * C S 429 pointer chasing
 *
 * pc.c - The entry point for the pointer chasing emulator.
 *
 * Copyright (c) 2024, 2025.
 * Authors: S. Chatterjee, S. O'Brien, K. Chandrasekhar
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include <stdint.h>
#include <stdlib.h>

#include <stdbool.h>

#define MIN_EXPONENT_SIZE (6)
#define MAX_EXPONENT_SIZE (18)
#define MAX_ENTRIES (1 << MAX_EXPONENT_SIZE)

#define PAGE_SIZE (4096)
#define EXTRA (PAGE_SIZE)

unsigned long int data[MAX_ENTRIES + EXTRA]; /* The array we'll be traversing */
unsigned long int *mem; /* Points to the first "pointer" in the data array */

void chase_indices(unsigned long int *mem);
void init_data_fast(unsigned long int mem[], unsigned long int size);
void init_data_slow(unsigned long int mem[], unsigned long int size);

// External function references
int main() {
    int size = 1 << 15;

    // Set pointer mem to the beginning of a (4K-address-aligned) page.
    mem = (unsigned long int *) ((uintptr_t) data + PAGE_SIZE -
                                 ((uintptr_t) data % PAGE_SIZE));

    init_data_fast(mem, size);

    chase_indices(mem);
}

/*
 * Initializes size "pointers" (really indices) in the memory array.
 * This initialization should play nicely with the cache, and thus be relatively
 * fast.
 */
void init_data_fast(unsigned long int mem[], unsigned long int size) {
    unsigned long int i;

    for (i = 0; i < size; i++)
        mem[i] = (i + 1); // Setting "pointers" here

    // And now, we overwrite the final entry to go back to the start
    mem[size - 1] = 0;
}

/*
 * Initializes size "pointers" (really indices) in the memory array.
 * This initialization should be adversarial to the cache, and thus be
 * relatively slow.
 */
void init_data_slow(unsigned long int mem[], unsigned long int size) {
    unsigned long int i;

    unsigned long int spot = 0;
    unsigned long int nextSpot = 0;
    unsigned long int start = 0;

    for (i = 0; i < size; i++) {
        nextSpot = spot + 128;
        if (nextSpot >= size) {
            nextSpot = ++start;
        }
        mem[spot] = nextSpot; // Setting "pointers" here
        spot = nextSpot;
    }

    // And now, we overwrite the final entry to go back to the start
    mem[size - 1] = 0;
}

void chase_indices(unsigned long int *mem) {
    // Chase your pointers.
    unsigned long int val = 0;
    unsigned long int iters = 0;
    do {
        do {
            iters++;
            // 0
            val = mem[val];
            val = mem[val];
            val = mem[val];
            val = mem[val];
            // 4
            val = mem[val];
            val = mem[val];
            val = mem[val];
            val = mem[val];
            // 8
            val = mem[val];
            val = mem[val];
            val = mem[val];
            val = mem[val];
            // 12
            val = mem[val];
            val = mem[val];
            val = mem[val];
            // W idx = val;
            val = mem[val];
            // 16
            // 16
            val = mem[val];
            val = mem[val];
            val = mem[val];
            val = mem[val];
            // 20
            val = mem[val];
            val = mem[val];
            val = mem[val];
            val = mem[val];
            // 24
            val = mem[val];
            val = mem[val];
            val = mem[val];
            val = mem[val];
            // 28
            val = mem[val];
            val = mem[val];
            val = mem[val];
            val = mem[val];
            // 32
        } while (val); // Chase pointers until back at the start
    } while (iters < 2048); // Chase pointers again and again, 2048 times
}
