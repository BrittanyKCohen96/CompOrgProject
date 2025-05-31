
# ------------------------------------------------------------- #
#           Assembly Code for Rectangle Creation                #
# ------------------------------------------------------------- #

#           Register values for debugging:
# -------------------------------------------------------------
#   $s0-2 + $gp - (Y, X) values for corners (A, B, C, D)

#   $a0 - A.x         $a1 - C.x       $a2 - A.y      $a3 - B.y
#   $t0 - width       $t1 - height
#   $t2 - y-counter   $sp - x-counter

#   $ra - (Y, X) values for beginning of row
#   $v0 - (Y, X) values for current position
# -------------------------------------------------------------

MAIN_CODE:
        
        # --- Set up color = 255 (white) ------------

        add   $s0,   $zero,  $imm,   255            # $s0 = 255 (white)
        out   $s0,   $zero,  $imm,   21             # write colour into monitor register (register 21)

        # --- Load corner‐coordinates from memory ---

        lw    $s0,   $zero,  $imm,   256            # $s0 = Mem[256] = A
        lw    $s1,   $zero,  $imm,   257            # $s1 = Mem[257] = B
        lw    $s2,   $zero,  $imm,   258            # $s2 = Mem[258] = C
        lw    $gp,   $zero,  $imm,   259            # $gp = Mem[259] = D

        # --- Extract X-Coordinates -----------------

        and   $a0,   $s0,    $imm,   0x000FF        # $a0 = A.x, A.x = lower 8b of $s0
        and   $a1,   $s2,    $imm,   0x000FF        # $a1 = C.x, C.x = lower 8b of $s2

        # --- Extract Y-Coordinates -----------------

        and   $a2,   $s0,    $imm,   0xFFF00        # $a2 = (A & 0xFFF00)
        srl   $a2,   $a2,    $imm,   8              # A.y = ($s0 & 0xFFF00) >> 8
        and   $a3,   $s1,    $imm,   0xFFF00        # $a3 = (B & 0xFFF00)
        srl   $a3,   $a3,    $imm,   8              # B.y = ($s1 & 0xFFF00) >> 8

        # --- Compute width and height --------------

        sub   $t0,   $a1,    $a0,    0              # $t0 = width,  width =  C.x - A.x (in pixels)
        sub   $t1,   $a3,    $a2,    0              # $t1 = height, height = B.y - A.y (in pixels)

        # --- Set up Y-loop counter -----------------

        add   $t2,   $zero,  $imm,   0              # $t2 = y-counter = 0

LOOP_Y:                                             # outer loop - increment y from top to bottom

        # --- Exit loop condition -------------------
        
        bgt   $imm,   $t2,    $t1,    END_DRAW      # if y-counter > height, done drawing rectangle

        # --- Get (Y, X) for beginning of row -------

        add   $ra,   $a2,    $t2,    0              # calculate current y value: $ra = A.y + y-counter
        sll   $ra,   $ra,    $imm,   8              # put y-value in MSBs by << 8 (shift left)
        add   $ra,   $ra,    $a0,    0              # add current X-coordinate to get x-value in LSBs

        # --- Set up X-loop counter -----------------

        add   $sp,   $zero,  $imm,   0              # $sp = xcounter = 0

LOOP_X:                                             # inner loop - move along x-value L->R (raster)

        # --- Exit loop condition -------------------

        bgt   $imm,  $sp,    $t0,    NEXT_Y         # if x-counter (which starts @ 0) > width, we've finished a loop

        # --- Calculate new (Y, X) for printing -----

        add   $v0,   $ra,    $sp,    0              # $v0 = (Y, X) of beginning of row + change in x-value

        # --- Send pixel coordinates to screen -----

        out   $v0,   $imm,   $zero,  20            # set monitor address = $v0
        add   $v0,   $imm,   $zero,  1             # set $v0 to 1 (tell monitor to output)
        out   $v0,   $imm,   $zero,  22            # Tell monitor to draw the pixel using register 22

        # --- Increment x-counter, loop again ------

        add   $sp,   $sp,    $imm,   1              # increment x-counter by 1
        beq   $imm,  $zero,  $zero,  LOOP_X         # unconditional loop

NEXT_Y:                                             # second part of y-loop: must increment y-counter after every row

        # --- Increment y-counter, go to LOOP_Y ----

        add   $t2,   $t2,    $imm,   1              # increment y-counter by 1
        beq   $imm,  $zero,  $zero,  LOOP_Y         # go back to beginning of LOOP_Y

END_DRAW:                                           # finish up

        halt  $zero, $zero,  $imm,   0              # stop the program

        # --- (A, B, C, D) Coordinates in Memory ----

        .word   256,   0x001400   # A = (200, 0)   -> 0x1400  {0x14,  0x00}
        .word   257,   0x00C800   # B = (20,  0)   -> 0xC800  {0xC8,  0x00}
        .word   258,   0x001468   # C = (20, 104)  -> 0x1468  {0x14,  0x68}
        .word   259,   0x00C868   # D = (200, 104) -> 0xC868  {0xC8,  0x68}