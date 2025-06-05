# ------------------------------------------------------------- #
#               factorial.asm – Recursive Factorial             #
# ------------------------------------------------------------- #
#   Purpose : Computes factorial(n) recursively                 #
#   Input   : n at memory address 0x100                         #
#   Output  : n! at memory address 0x101                        #
# ------------------------------------------------------------- #
#   Register usage:                                             #
#   $a0  – current n                                            #
#   $v0  – return value = factorial(n)                          #
#   $ra  – return address for recursive call                    #
#   $sp  – stack pointer                                        #
#   $t0  – temp register for calculations                       #
# ------------------------------------------------------------- #

MAIN:

        # --- Load n from memory into $a0 ----------------------

        lw     $a0,  $zero,  $imm,    0x100        # $a0 = Mem[0x100] (input n)

        # --- Base case: if n == 0 -----------------------------

        beq    $imm,  $a0,    $zero,   BASE_CASE   # if n == 0, jump to BASE_CASE

        # --- Store n on stack ---------------------------------

        sw     $a0,  $sp,    $zero,    0           # save n at top of stack
        sub    $sp,  $sp,    $imm,     1           # decrement stack pointer

        # --- Prepare recursive call with n - 1 ----------------

        sub    $a0,  $a0,    $imm,     1           # $a0 = n - 1

        # --- Recursive call to factorial ----------------------

        jal    $ra,  $imm,   $zero,    FACTORIAL   # jump to FACTORIAL (store return address in $ra)

        # --- Restore n from stack after return ----------------

        add    $sp,  $sp,    $imm,     1           # restore stack pointer
        lw     $a0,  $sp,    $zero,    0           # retrieve n into $a0

        # --- Multiply n * factorial(n - 1) --------------------

        mul    $v0,  $a0,    $v0,      0           # $v0 = n * result of recursive call

        # --- Return to caller ---------------------------------

        beq    $imm, $zero,  $zero,    RETURN      # unconditional jump to RETURN

BASE_CASE:

        # --- factorial(0) = 1 -------------------------------

        add    $v0,  $imm,   $imm,     1           # $v0 = 1

RETURN:

        # --- Write result to memory --------------------------

        sw     $v0,  $imm,   $zero,    0x101       # Mem[0x101] = $v0

        # --- Stop program ------------------------------------

        halt   $zero, $zero, $zero,    0

FACTORIAL:

        # --- Recursive call target ----------------------------

        beq    $imm, $zero, $zero,     MAIN        # jump to MAIN
