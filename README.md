# Welcome to the CS 429 System Emulator Lab!

This is a brief overview of the repository,
which contains information about the codebase and how to run it.

# Running the emulator

To get started, you can compile the project just by running `make` in the terminal.
This will remove any previously compiled binaries and compile a new `se` executable,
which will be located in the `bin` directory.

You can run the emulator on any binary file in one of the `testcases` subdirectories with the `-i` (input) flag.
By default, the emulator uses STUR with the effective address -1 as a trap to print a value to stderr,
which can be used to verify the output of a program.
An example would be `bin/se -i testcases/alu/print_simple/add`.

Additional flags can be added to display more information.
The `-v <level>` (verbose) flag can be added to view the status of each pipeline register after each processor cycle.
`-v 1` will print all the values of each stage,
and `-v 2` will additionally print the control signals of each stage.
It is not reccommended to use this when running the emulator on longer programs.

By default, the emulator will create a HLT instruction when the start function returns, and shut down.
It will also stop if a cycle limit is reached, which is 500 by default.
The low limit is in place for students who run into infinite loops and such while completing the project.
You can change this by adding the `-l <cycle limit>` flag to the command you use to run the emulator.

Also, the cache is unused by default, and memory accesses are treated as if everything is cache-resident.
To enable the cache, you need to provide the `-A <associativity>`, `-B <line size>`, `-C <capacity>`,
and `-d <miss penalty>` flags for creating the cache.
With the cache enabled, memory accesses that miss the cache will stall for the designated number of delay cycles,
and cache hits will not stall at all.
This lab only implements a cache for data memory, instruction memory will never incur a miss penalty.

Finally, the entire state of the machine can be logged as a "checkpoint" at the end of the program
with the `-c <checkpoint file>` flag.
This will print register and relevant memory contents to the provided checkpoint file.

Putting this all together, an example command would be
`bin/se -i testcases/applications/hard/gemm_block -l 40000000 -c checkpoint.out -A 4 -B 32 -C 512 -d 100`
to run the emulator on the blocked matrix-matrix multiplication example

# Repository structure

There are four main directories at the top level of the repository:
`bin`, which contains compiled binaries, `include`, which contains the project's header files,
`src`, which contains the source code, and `testcases`, which contains the binary files that the emulator can run.
The `checkpoints` directory is empty and is used by the testing suite for creating checkpoints of programs the emulator is run on.

The `bin` directory contains `csim-ref` and a few versions of `se-ref` by default,
which are provided references for students to compare against.
They use the same code as what is provided in this repository.
When compiling with `make`, the files `se`, `csim`, `test-se`, and `test-csim` are created.
`se` is the emulator described above, and `csim` is a standalone emulator for testing the cache separately.
`test-se` and `test-csim` are useful for students to test their code against the provided testing suite.

The `include` directory contains corresponding header files for each source code file.
Most of these files just have function signatures,
but some also contain struct declarations used throughout the program.
Files not mentioned here only contain function declarations that will be explained later.

In the `base` subdirectory:
- `archsim.h` contains declarations for most of the global variables used in the program that are not used by the student.
- `machine.h` contains the declaration for the machine_t struct,
  which acts as a container for all the different components of the emulator.
- `mem.h` contains the declaration for the mem_t struct and other enums,
  which contain information about the emulator's memory system.
- `proc.h` contains the declaration for the proc_t,
  which contains the pipeline registers, general purpose registers, control flags,
  stack pointer, program counter, and status.
- `hw_elts.h` contains the declarations for the hardware elements that implement the basic chunks of a processor.

In the `cache` subdirectory:
- `cache.h` contains the declaration for the cache_t struct, which emulates a cache.

In the `pipe` subdirectory:
- `instr_pipeline.h` contains the declaration for each pipeline register struct,
  which contains the values and control signals used at each stage of the pipeline.
- `instr.h` contains the declaration for opcode, conditional, and status enums.
- `forward.h` contains the declaration for `forward_reg`, used to implement value forwarding.
- `hazard_control.h` contains the declaration for `handle_hazards` as well as a few helper functions, used to implement hazard control.

The `src` directory contains the source code for the project.
There are several subdirectories for better organization.
The `cache` and `pipe` subdirectories are for students to complete,
though this repository contains the solution code for them.
The `base` subdirectory contains the remaining emulator code,
which is provided for students to use at the start.
Finally, the `testbench` subdirectory contains code for automatically testing
a student's solution and comparing against the provided reference.

In the `base` subdirectory:
- `archsim.c` contains the main function for running the emulator.
- `elf_loader.c` contains the ELF loader that opens the provided program and maps it to memory.
- `err_handler.c` contains a function for error and info logging.
- `handle_args.c` contains a function for handing the various input arguments the emulator accepts.
- `hw_elts.c` contains functions that emulate several hardware components,
  such as the register file and ALU. It also contains interfaces for accessing instruction and data memory.
- `interface.c` contains the code for printing messages to the terminal.
  At some point in the future, this will contain code for running the emulator in a mode that can step forward
  and backward through a program's execution.
- `machine.c` contains the code for initializing the machine state and logging the state to a checkpoint file.
- `mem.c` contains the code for interfacing with memory, and controlling whether to use the cache or not.
- `proc.c` contains the code that runs an emulated program to completion.
  It runs each stage of the pipeline every cycle,
  and handles transferring data from a pipeline register's input to its output.
  Students are meant to use the "output" side of a pipeline register to complete the stage's functionality,
  and write to the next stage's "input" side.
- `ptable.c` contains the code that manages the pagetable for the emulated program's memory.
- `stage_ordering.c` contains the code that generates all permutations of pipeline stages to run if using the RANDOM preprocessor flag.

In the `cache` subdirectory:
- `cache.c` contains the code for checking if a memory access is a cache hit or miss,
  as well as reading and writing to the cache itself.
- `csim.c` contains a separate main function for testing the cache on its own.
  This is for students to be able to test their cache implementation without having the rest of the lab working.

In the `pipe` subdirectory:
- `forward.c` contains the code for forwarding data from execute, memory, and writeback
  back to decode for handling data hazards.
- `hazard_control.c` contains the code for detecting control hazards and bubbling or stalling the pipeline.
- `instr_base.c` contains a couple utilities provided to students.
  There are functions for extracting bitfields from an instruction,
  as well as code that creates a table (called `itable` in the code)
  that maps bits of an instruction to the corresponding opcode.
  It also contains code for the verbose output that prints the values and control signals at each cycle.
- `instr_Decode_extractregs.c` contains code for setting up the table mapping instruction format to function handlers.
- The remaining `instr_<stage>.c` files contain code for completing their corresponding pipeline stage.

In the `testbench` subdirectory:
- `test-csim.c` contains a test harness for comparing a student's csim solution with the provided `csim-ref`.
- `test-se.c` contains a test harness for comparing a student's se solution with the provided `se-ref`.

Finally, the `testcases` directory contains ARM binaries for the emulator to run.
The tests are separated into several categories to be clearer to students on what they should focus on
when debugging each test.
Each test has three files associated with it: the handwritten assembly (.s file),
the disassembly generated by `objdump` (.od file), and the binary itself.
The assembly and disassembly are provided for student debugging purposes.
