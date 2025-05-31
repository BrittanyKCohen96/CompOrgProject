INITIALIZE:
	add $s0, $zero, $imm, 255	   # Load the color value (255 for white) into $s0
	out $s0, $zero, $imm, 21	   # Set the color in the monitor register 21
		
	lw  $s0, $zero, $imm, 256      # Load coordinate A from memory address 256 into $s0
	lw  $s1, $zero, $imm, 257      # Load coordinate B from memory address 257 into $s1
	lw  $s2, $zero, $imm, 258      # Load coordinate C from memory address 258 into $s2

EXTRACT_X:
	and $a0, $s0, $imm, 0x000FF    # Extract the x-coordinate of A by taking the lower 8 bits
	and $a1, $s2, $imm, 0x000FF    # Extract the x-coordinate of C by taking the lower 8 bits

EXTRACT_Y:
	and $a2, $s0, $imm, 0xFFF00    # Mask the y-coordinate of A (shifted left by 8 bits)
	srl $a2, $a2, $imm, 8          # Shift right to obtain the actual y-coordinate
	and $a3, $s1, $imm, 0xFFF00    # Mask the y-coordinate of B (shifted left by 8 bits)
	srl $a3, $a3, $imm, 8          # Shift right to obtain the actual y-coordinate
	
	sub $s1, $a1, $a0, 0	         # Calculate the width (BC.x = C.x - A.x)
	sub $s0, $a3, $a2, 0	         # Calculate the height (AB.y = B.y - A.y)
	add $a1, $zero, $imm, -1       # Initialize the loop counter for drawing the triangle

DRAW_LOOP:
	bgt $imm, $a1, $s0, END_DRAW   # If the loop counter exceeds height, exit loop
	add $a1, $a1, $imm, 1	         # Increment loop counter (line += 1)
	mul $t1, $a1, $s1, 0	         # Calculate y * BC for the ratio calculation
	add $t2, $zero, $zero, 0       # Initialize the inner loop counter (i = 0)
	add $t0, $zero, $zero, 0       # Reset temporary quotient value to 0
	beq $imm, $zero, $zero, CALC_RATIO # Jump to calculate the ratio y * BC / AB

INNER_LOOP:
	bgt $imm, $t2, $t0, DRAW_LOOP  # If inner loop counter exceeds quotient, exit inner loop
	add $a3, $a1, $a2, 0           # Calculate the y-offset of the current line
	mul $a3, $a3, $imm, 256        # Scale the y-coordinate to match the framebuffer row size
	add $a3, $a3, $a0, 0           # Add x offset to position the pixel correctly
	add $a3, $a3, $t2, 0           # Adjust for the inner loop counter
	out $a3, $imm, $zero, 20       # Set the monitor address for this pixel
	add $a3, $imm, $zero, 1        # Reset the monitor command register
	out $a3, $imm, $zero, 22       # Send the command to draw the pixel
	add $t2, $t2, $imm, 1          # Increment inner loop counter
	beq $imm, $zero, $zero, INNER_LOOP # Continue drawing the line

END_DRAW:
	halt $zero, $zero, $imm, 0     # Halt the program when drawing is complete

CALC_RATIO:
	blt $imm, $t1, $s0, INNER_LOOP # If the remainder is less than the divisor, jump to inner loop
	sub $t1, $t1, $s0, 0           # Subtract divisor from remainder
	add $t0, $t0, $imm, 1          # Increment the quotient
	beq $imm, $zero, $zero, CALC_RATIO # Loop back to continue calculating the ratio

# Data Initialization
.word 256 5120   
.word 257 51400  
.word 258 102760 