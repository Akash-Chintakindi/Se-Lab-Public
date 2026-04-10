# CS 429 System Emulator - Lab Notes

## Project Structure
- **Build**: `make week2` (PIPE- without PIPE flag), `make week3`/`make week4` (with PIPE flag)
- **Test**: `bin/test-se -w 2` runs all week 2 tests; `bin/se-ref-wk2` is the reference binary
- **Run single test**: `bin/se -i testcase/path` vs `bin/se-ref-wk2 -i testcase/path`
- **Checkpoint comparison**: `bin/test-se` uses `-c` flag for checkpoint file comparison. Tests that pass with `diff` on stdout may FAIL on checkpoint comparison. Always test with: `diff <(bin/se-ref-wk2 -i TEST -c /dev/stdout 2>/dev/null) <(bin/se -i TEST -c /dev/stdout 2>/dev/null)`

## Pipeline Architecture (5-stage in-order: Fetch/Decode/Execute/Memory/Writeback)

### Pipeline Register Macros (include/pipe/instr_pipeline.h)
- `F_in`/`F_out` = input/output of F register (f_instr_impl_t: pred_PC, status)
- `D_in`/`D_out` = input/output of D register (d_instr_impl_t: insnbits, op, format, multipurpose_val, status)
- `X_in`/`X_out`, `M_in`/`M_out`, `W_in`/`W_out` similarly
- Stages are called: `fetch_instr(F_out, D_in)`, `decode_instr(D_out, X_in)`, etc.
- Stages READ from `_out` side, WRITE to `_in` side of next register

### Pipeline Flow (src/base/proc.c)
1. All 5 stages run combinationally (read _out, write _in) — fetch runs FIRST, then decode, etc.
2. `forward_reg()` runs (no-op in PIPE- mode)
3. `guest.proc->status = W_out->status` (determines loop exit)
4. `handle_hazards()` sets pipe control (P_LOAD/P_BUBBLE/P_STALL)
5. NZCV update, regfile_write (writes register file AFTER all stages run)
6. `guest.proc->PC = F_PC` (predicted PC from fetch stage)
7. `F_in->pred_PC = guest.proc->PC`
8. Latch: P_LOAD copies in→out, P_BUBBLE zeros out, P_STALL keeps out unchanged
9. Loop continues while `status == STAT_AOK || status == STAT_BUB`

### Execution Order Within a Cycle (CRITICAL)
```
fetch_instr(F_out, D_in)    // reads F_out, D_out (for select_PC), X_out, M_out
decode_instr(D_out, X_in)   // reads D_out, reads regfile
execute_instr(X_out, M_in)  // reads X_out
memory_instr(M_out, W_in)   // reads M_out
wback_instr(W_out)           // reads W_out, sets W_wval
forward_reg(...)             // updates X_in->val_a, X_in->val_b (no-op in PIPE-)
handle_hazards(...)
regfile_write(...)           // writes to register file
PC update, latch
```
- Decode reads regfile values written by PREVIOUS cycle's regfile_write
- In a given cycle: fetch sees X_out->val_a from the instruction decoded TWO cycles ago (decoded → latched to X_out)

### Handle Hazards - PIPE- mode (src/pipe/hazard_control.c)
```c
bool f_stall = F_out->status == STAT_HLT || F_out->status == STAT_INS;
pipe_control_stage(S_FETCH, false, f_stall);  // stall F on HLT/INS
pipe_control_stage(S_DECODE, false, false);    // all others P_LOAD
pipe_control_stage(S_EXECUTE, false, false);
pipe_control_stage(S_MEMORY, false, false);
pipe_control_stage(S_WBACK, false, false);
```

### Key Constants
- `STAT_BUB=0, STAT_AOK=1, STAT_HLT=2, STAT_ADR=3, STAT_INS=4`
- `OP_NOP=0` (first in enum, what calloc'd/zeroed registers have)
- `SP_NUM=31, XZR_NUM=32`
- `RET_FROM_MAIN_ADDR=0x0UL` - GPR[30] is initialized to this
- `guest.proc->GPR[30] = RET_FROM_MAIN_ADDR` set before pipeline runs

### Global Wires (src/pipe/instr_base.c)
- `F_PC` - fetch sets predicted PC here; proc.c copies to guest.proc->PC
- `D_src1, D_src2` - for forwarding
- `X_nzcvval, X_set_flags` - NZCV from execute
- `M_PC` - PC of instr in M stage (for exceptions)
- `W_wval` - writeback value

### select_PC Priority (src/pipe/instr_Fetch.c)
1. Framework: `D_opcode==OP_RET && val_a==RET_FROM_MAIN_ADDR` → PC=0 (halt)
2. RET correction: `D_opcode==OP_RET` → PC=val_a (PIPE only)
3. B.cond misprediction: `M_opcode==OP_B_COND && !M_cond_val` → PC=seq_succ (PIPE only)
4. Default: PC=pred_PC

### predict_PC (src/pipe/instr_Fetch.c)
- B/BL: PC + sign_extend(imm26)<<2
- B.cond: PC + sign_extend(imm19)<<2 (predict taken)
- All others: PC+4

### fetch_instr HLT generation condition
```c
if (!current_PC || F_in->status == STAT_HLT || F_in->status == STAT_INS) { /* generate HLT */ }
```

### Control Signals (set in decode)
- `d_ctl_sigs_t`: src2_sel (1 for STUR to read data reg)
- `x_ctl_sigs_t`: vala_sel (1=multipurpose_val), valb_sel (1=register, 0=imm), set_flags
- `m_ctl_sigs_t`: dmem_read, dmem_write
- `w_ctl_sigs_t`: dst_sel (1=X30 for BL), wval_sel (1=val_mem for LDUR), w_enable

### Text Segment Boundary (src/base/mem.c)
- `addr_in_imem(addr)`: returns true if `TEXT_SEG start <= addr < DATA_SEG start`
- imem error occurs when fetching past text segment → STAT_INS

## Current Status

### Checkpoint 1: COMPLETE (hw_elts)
### Checkpoint 2: PIPE- pipeline - IN PROGRESS

All 5 stage files implemented. Build succeeds with `make week2`.

### Test Results (50 pass, 14 fail) — checkpoint comparison with -c flag
**Passing (50 tests):** basics (5/5), alu/pipeminus (15/15), alu/print_pipeminus (10/11), mem/pipeminus (3/4), exceptions/pipeminus (17/21)

**Failing (14 tests):**
1. **Branch tests (6)**: bcond, bcond_cmn, bl, bl_ret, branch_not_taken, branch_taken — all hit 500 cycles (infinite loop)
2. **Exception tests (4)**: bad_insn_1-4 — wrong PC values
3. **Print test (1)**: alu/print_pipeminus/cmp — 500 cycles, infinite loop
4. **Mem test (1)**: mem/pipeminus/adrp3 — wrong cycle count (27 vs 21), wrong PC, INS vs HLT
5. **Application tests (2)**: 20thfib, 5factorial — 500 cycles, infinite loop

### Reference Expected Status for Failing Tests
- HLT: branch_taken, bl_ret, branch_not_taken, bcond, bcond_cmn, 20thfib, cmp
- INS: bl, 5factorial
- (bad_insn and adrp3 are separate issues)

### The val_a Problem (MAIN BLOCKER for 9 infinite-loop tests)

The `select_PC` framework check `if (D_opcode == OP_RET && val_a == RET_FROM_MAIN_ADDR)` needs val_a=0 to fire.
- Tests needing framework to fire (HLT): branch_taken, bl_ret, bcond, bcond_cmn, branch_not_taken, 20thfib, cmp — all have x30=0 (never modified by BL)
- Tests needing framework NOT to fire (INS): basic, add, all simple tests — also have x30=0!
- Tests where BL modifies x30: bl (INS), 5factorial (INS) — x30≠0, framework shouldn't fire

**The paradox**: basic and branch_taken BOTH have x30=0, but basic needs INS (framework should NOT fire) and branch_taken needs HLT (framework MUST fire).

**Approaches tried and results:**
1. `val_a = in->pred_PC` (always non-zero) → framework never fires → 50 pass, branch/app tests loop forever (CURRENT)
2. `val_a = X_out->val_a` → broke basic/add/nop etc. because X_out from decoded bubble has val_a=0 → only 42 pass
3. `val_a = regfile_read(x30)` → ALL simple tests break because x30=0 always triggers framework → HLT instead of INS
4. regfile_read + addr_in_imem override → addr_in_imem range is TOO WIDE (TEXT_SEG=0x400000 to DATA_SEG=0x800000 default), so pred_PC is always "in imem" even past actual code

**Memory layout facts:**
- Default seg_starts: {0x0, 0x400000, 0x800000, 0x10000000, ...}
- ELF loader updates TEXT_SEG/DATA_SEG from .text/.data section addresses
- If no .data section (e.g., basic test), DATA_SEG stays at default 0x800000
- So addr_in_imem covers 0x400110 to 0x800000 for basic — WAY past actual code

**Key observation about basic test reference behavior:**
- Reference: 6 cycles, PC=0x400118, Status=INS
- basic has only RET at 0x400110. After fetching RET, pred_PC=0x400114.
- At 0x400114: no real instruction, likely all zeros → itable[0] = OP_ERROR → STAT_INS
- So reference does NOT fire framework for basic. It fetches 0x400114, gets OP_ERROR, sets INS.
- This means reference uses val_a = something non-zero for basic (pred_PC approach matches!)

**Key observation about branch_taken reference behavior:**
- Reference: 39 cycles, PC=0x400170, Status=HLT
- branch_taken loops forever without framework (RET→.helper→B→.goback→RET→...)
- Reference MUST fire framework. This means val_a=0 when RET is in decode.
- x30=0 throughout (no BL). So regfile read WOULD give 0.
- But regfile read also gives 0 for basic, which would break basic.

**UNSOLVED**: What mechanism does the reference use to fire the framework for branch_taken but NOT for basic? Both have x30=0. Possible leads:
- Maybe decode_instr for a bubble produces X_in->val_a ≠ 0 for some reason (need to check what extract_regs gives for OP_NOP with insnbits=0)
- Maybe the answer involves the F_in->status latch behavior
- Maybe handle_hazards is supposed to do something for PIPE- mode regarding RET detection
- Consider: what if val_a should come from D_out fields that decode wrote in a previous cycle? D_out doesn't have val_a but maybe multipurpose_val is used?
- Consider: what if the student needs to read regfile in fetch ONLY when x30 has actually been written (tracking via pipeline)?

### bad_insn Tests (PC Issue)
bad_insn_1-4 show wrong PC values. Reference PCs are much smaller. M_PC = in->multipurpose_val.seq_succ_PC (which is PC+4 of the instruction). Need to investigate what PC the reference reports — could be related to how STAT_INS propagates.

### adrp3 Test
Wrong cycle count (27 vs 21), wrong PC, INS vs HLT. Likely related to the val_a problem (has branches/RET).

### Files Modified
- `src/pipe/instr_Fetch.c` — select_PC, predict_PC, fix_instr_aliases, fetch_instr
- `src/pipe/instr_Decode.c` — all decode logic
- `src/pipe/instr_Execute.c` — execute logic
- `src/pipe/instr_Memory.c` — memory logic
- `src/pipe/instr_Writeback.c` — writeback logic
- `src/pipe/hazard_control.c` — PIPE- hazard handling (unchanged from starter)
- `src/pipe/forward.c` — empty (PIPE- mode, no forwarding needed)

### Handout Note
The handout incorrectly states forward_reg() needs to be called in instr_Decode(). It is already called for you in proc.c.
