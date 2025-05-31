# simple_simp_decoder.py
#
# A quick Python script to translate a list of 32-bit binary words
# (one per line) into SIMP assembly instructions.  It expects “bigimm”
# instructions to span two lines (the first line with bigimm=1, the
# second line the full 32-bit immediate).
#
# Usage:
#   1. Put all your 32-bit binary words (as strings of ‘0’/‘1’, 32 chars)
#      in a text file, one per line (e.g. “machine_output.txt”).
#   2. Run: python simple_simp_decoder.py machine_output.txt
#   3. It will print each decoded assembly instruction to stdout.

import sys

# ----------------------------
#  Define opcode & register maps
# ----------------------------

opcode_table = {
    0:  "add",  1: "sub",   2: "mul",  3:  "and",
    4:  "or",   5: "xor",   6: "sll",  7:  "sra",
    8:  "srl",  9: "beq",  10: "bne", 11:  "blt",
   12:  "bgt", 13: "ble",  14: "bge", 15:  "jal",
   16:  "lw",  17: "sw",  18: "reti", 19: "in",
   20: "out",  21: "halt"
}

reg_table = {
    0: "$zero",  1: "$imm",  2: "$v0",  3: "$a0",
    4: "$a1",    5: "$a2",   6: "$a3",  7: "$t0",
    8: "$t1",    9: "$t2",  10: "$s0", 11: "$s1",
   12: "$s2",   13: "$gp",  14: "$sp", 15: "$ra"
}

# ----------------------------
#  Helper: sign-extend an 8-bit value to Python int
# ----------------------------
def sign_extend_8bit(x):
    """Given an integer 0..255, interpret it as signed 8-bit and return Python int."""
    if x & 0x80:
        return x - 0x100
    return x

# ----------------------------
#  Main decoding routine
# ----------------------------
def decode_simp_binary_lines(lines):
    """
    lines: a list of strings, each string is exactly 32 characters '0' or '1'.
    Returns: a list of decoded assembly instruction strings.
    """
    decoded = []
    i = 0
    n = len(lines)

    while i < n:
        word_str = lines[i].strip()
        if len(word_str) != 32 or any(c not in "01" for c in word_str):
            # skip empty or invalid lines
            i += 1
            continue

        # Convert the 32-bit binary string to an integer
        w = int(word_str, 2)

        # Extract fields
        opcode = (w >> 24) & 0xFF
        rd     = (w >> 20) & 0x0F
        rs     = (w >> 16) & 0x0F
        rt     = (w >> 12) & 0x0F
        bigimm = (w >> 8)  & 0x01  # 1 => two-word, 0 => single-word
        imm8   = w & 0xFF          # low 8 bits

        # Look up mnemonic and registers
        mnemonic = opcode_table.get(opcode, f"OP_{opcode}")
        rd_name  = reg_table.get(rd, f"r{rd}")
        rs_name  = reg_table.get(rs, f"r{rs}")
        rt_name  = reg_table.get(rt, f"r{rt}")

        if bigimm == 1:
            # Must have at least one more line available for the full immediate
            if i + 1 >= n:
                decoded.append(f"; ERROR: bigimm=1 at line {i}, but no low-word found")
                break

            # Parse the next line as a full 32-bit immediate
            low_word_str = lines[i+1].strip()
            if len(low_word_str) != 32 or any(c not in "01" for c in low_word_str):
                decoded.append(f"; ERROR: invalid low-word binary at line {i+1}")
                i += 2
                continue

            imm32 = int(low_word_str, 2)
            # Optionally, treat imm32 as signed 32-bit:
            if imm32 & 0x80000000:
                imm32 -= (1 << 32)

            # Format: “mnemonic rd, rs, rt, full_imm”
            decoded.append(f"{mnemonic} {rd_name}, {rs_name}, {rt_name}, {imm32}")
            i += 2
        else:
            # Single-word: immediate is sign-extended from 8 bits
            signed_imm8 = sign_extend_8bit(imm8)
            decoded.append(f"{mnemonic} {rd_name}, {rs_name}, {rt_name}, {signed_imm8}")
            i += 1

    return decoded

# ----------------------------
#  Example usage: main()
# ----------------------------
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python simple_simp_decoder.py <binary_input_file>")
        sys.exit(1)

    input_file = sys.argv[1]
    try:
        with open(input_file, 'r') as f:
            lines = f.readlines()
    except IOError as e:
        print(f"Error opening file {input_file}: {e}")
        sys.exit(1)

    asm_lines = decode_simp_binary_lines(lines)
    for asm in asm_lines:
        print(asm)
