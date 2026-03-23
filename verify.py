"""
 * C S 429 Assembly Coding Lab
 *
 * testbench.c - Testbench for all the assembly functions.
 *
 * Copyright (c) 2022, 2023, 2024, 2025
 * Authors: Prithvi Jamadagni, Kavya Rathod, Anoop Rachakonda, Zeeshan Ahmad
 * All rights reserved.
 * May not be used, modified, or copied without permission.
"""

import sys
import re


def is_xgpr_operand(operand: str):
    return is_xzr(operand) or (operand.startswith("X") and operand[1:].isnumeric())


def is_wreg_operand(operand: str):
    return operand.startswith("W") and (operand[1:] == "ZR" or operand[1:].isnumeric())


def is_xzr(operand: str):
    return operand == "XZR"


def is_sp(operand: str):
    return operand == "SP"


def is_imm(operand: str):
    return not (is_sp(operand) or is_xzr(operand) or is_wreg_operand(operand) or is_xgpr_operand(operand))
    # return operand.isnumeric() or (operand.startswith("#") and operand[1:].isnumeric())

def verify(line: str):
    # now we need to parse the actual instruction
    # chop off any comment if necessary
    line = line.split("//")[0].strip()

    if not line:
        return True, ""
    # ignore labels
    if line.endswith(":"):
        return True, ""
    # ignore comment lines
    if line.startswith("/") or line.startswith("*"):
        return True, ""
    # ignore assembler directives
    if line.startswith("."):
        return True, ""


    opcode = line.split(" ")[0]

    # the opcodes for which there are no variants we have to check
    if opcode in [
        "B",
        "BL",
        "RET",
        "NOP",
        "HLT",
        "ADRP",
    ]:
        return True, ""

    # opcodes for which we just have to do a W reg check
    if opcode in ["LDUR", "STUR", "MOVK", "MOVZ", "MVN"]:
        tokens = re.split(",\\s*|\\s+", line)
        if is_wreg_operand(tokens[1]):
            return False, "using a W register"
        return True, ""

    # b.cc, we don't have to chk variants
    # can probably do better with a regex capture group
    if opcode.startswith("B."):
        return True, ""

    # bcc, we don't have to chk variants
    if (
        opcode.startswith("B")
        and len(opcode) == 3
        and opcode[1:]
        in [
            "EQ",
            "NE",
            "CS",
            "HS",
            "CC",
            "LO",
            "MI",
            "PL",
            "VS",
            "VC",
            "HI",
            "LS",
            "GE",
            "LT",
            "GT",
            "LE",
            "AL",
        ]
    ):
        return True, ""

    # CMP, CMN, and TST
    if opcode in ["CMP", "CMN", "TST"]:
        # should be shifted register version.
        # of the form CMP <Xn>, <Xm>
        # Cannot use the shift!
        tokens = re.split(",\\s*|\\s+", line)
        if len(tokens) > 3:
            return (
                False,
                "using the shift on second operand or using the extended register version",
            )
        elif is_wreg_operand(tokens[1]) or is_wreg_operand(tokens[2]):
            return False, "using W registers"
        # uncomment the following code for SP2025
        # (can't add it in F2024 because it is too late)
        # This was not done in SP2025 but I'm doing it in F2025 ~ Zeeshan
        elif not is_xgpr_operand(tokens[2]):
            return False, f'using an RI variant of {opcode}'
        else:
            return True, ""

    # ALU_RR opcodes
    if opcode in ["ADDS", "SUBS", "ORR", "EOR", "ANDS"]:
        # Should be the shifted register version.
        # Of the form ADDS <Xd>, <Xn>, <Xm>
        # Cannot use the shift!
        tokens = re.split(",\\s*|\\s+", line)
        if len(tokens) > 4:
            return (
                False,
                "using the shift on second operand or using the extended register version",
            )
        elif is_wreg_operand(tokens[1]) or is_wreg_operand(tokens[2]) or is_wreg_operand(tokens[3]):
            return False, "using W registers"
        elif not is_xgpr_operand(tokens[3]):
            return False, f"using an RI variant of {opcode}"
        else:
            return True, ""

    # ALU_RI opcodes (except UBFM, since it has 2 immediates)
    if opcode in ["ADD", "SUB", "ASR"]:
        # should be the immediate version
        # Of the form ADD <Xd|SP>, <Xn|SP>, #<imm>
        tokens = re.split(",\\s*|\\s+", line)
        if len(tokens) > 4:
            return (
                False,
                "using the shift on second operand or using the extended register version",
            )
        elif is_wreg_operand(tokens[1]) or is_wreg_operand(tokens[2]):
            return False, "using W registers"
        elif is_xgpr_operand(tokens[3]) or is_sp(tokens[3]) or is_wreg_operand(tokens[3]):
            return False, f"using an RR variant of {opcode}"
        else:
            return True, ""

    # ALU_RR & ALU_RI opcodes
    if opcode in ["LSL", "LSR"]:
        tokens = re.split(",\\s*|\\s+", line)
        if len(tokens) > 4:
            return (
                False,
                "using the shift on second operand or using the extended register version",
            )
        elif is_wreg_operand(tokens[1]) or is_wreg_operand(tokens[2]):
            return False, "using W registers"
        else:
            return True, ""

    # UBFM
    if opcode in ["UBFM"]:
        tokens = re.split(r",\s*|\s+", line)
        if len(tokens) != 5:
            return (
                False,
                'UBFM has wrong number of arguments. Syntax should be "UBFM <Xd>, <Xn>, #<immr>, #<imms>"',
            )
        elif is_wreg_operand(tokens[1]) or is_wreg_operand(tokens[2]):
            return False, "using W registers"
        elif not is_imm(tokens[3]) or not is_imm(tokens[4]):
            return False, 'UBFM must have two immediates. Syntax is: "UBFM <Xd>, <Xn>, #<immr>, #<imms>"'
        else:
            return True, ""

    return False, "disallowed opcode"


if len(sys.argv) != 2:
    print('Usage: "python3 verify.py FILENAME"')
    exit(0)
fails = 0
with open(sys.argv[1], "r") as infile:
    for i, line in enumerate(infile, 1):
        (valid, reason) = verify(line.strip().upper())
        if not valid:
            fails += 1
            valid_instructions = False
            print(
                f"Line {i} failed verification. Reason: {reason}. \n\tLine contents: '{line.strip()}'"
            )


def verify_straight_line(lines: list[str]):
    for line in lines:
        toks = line.split()
        if toks[0].strip().upper().startswith("B"):
            return False, "non straight-line code"
    return True, ""


def verify_tree_depth_is_recursive(lines: list[str]):
    for line in lines:
        toks = line.split()
        if (
            toks[0].strip().upper().startswith("BL")
            and toks[1].strip().upper() == "TREE_DEPTH"
        ):
            return True, ""
    return False, "\n\tfunction fails to call BL tree_depth"

def verify_insert_at_head_bl(lines: list[str]):
    calls_malloc = False
    for line in lines:
        toks = line.split()
        if toks[0].strip().upper().startswith("BL"):
            if toks[1].strip().upper() == "MALLOC" or toks[1].strip().upper() == "CALLOC":
                calls_malloc = True
    if not calls_malloc:
        return False, "\n\tfunction fails to call BL malloc"
    return True, ""

def verify_del_at_head_bl(lines: list[str]):
    calls_free = False
    for line in lines:
        toks = line.split()
        if toks[0].strip().upper().startswith("BL"):
            if toks[1].strip().upper() == "FREE":
                calls_free = True
    if not calls_free:
        return False, "\n\tfunction fails to call BL free"
    return True, ""



# now we need to check a few more things
# tree_depth has to be recursive
# random_num_gen has to call bit_rotate_l

with open(sys.argv[1], "r") as infile:
    while not infile.readline().strip().startswith("tree_depth"):
        pass
    lines = []
    for line in infile:
        if ".-tree_depth" in line:
            break
        if len(line.strip()) == 0:
            continue
        lines.append(line.strip())

    result, reason = verify_tree_depth_is_recursive(lines)
    if not result:
        print(f"tree_depth failed verification. Reason: {reason}")

    while not infile.readline().strip().startswith("insert_at_head"):
        pass
    lines = []
    for line in infile:
        if ".-insert_at_head" in line:
            break
        if len(line.strip()) == 0:
            continue
        lines.append(line.strip())
    result, reason = verify_insert_at_head_bl(lines)
    if not result:
        print(f"insert_at_head failed verification. Reason: {reason}")

    while not infile.readline().strip().startswith("del_at_head"):
        pass
    lines = []
    for line in infile:
        if ".-del_at_head" in line:
            break
        if len(line.strip()) == 0:
            continue
        lines.append(line.strip())
    result, reason = verify_del_at_head_bl(lines)
    if not result:
        print(f"del_at_head failed verification. Reason: {reason}")


exit(0)