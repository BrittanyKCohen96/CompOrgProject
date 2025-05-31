INITIALIZE:
	add $s0, $zero, $imm, 255	   # Load the color value (255 for white) into $s0
	out $s0, $zero, $imm, 21	   # Set the color in the monitor register 21
		
	lw  $s0, $zero, $imm, 256      # Load coordinate A from memory address 256 into $s0
	lw  $s1, $zero, $imm, 257      # Load coordinate B from memory address 257 into $s1
	lw  $s2, $zero, $imm, 258      # Load coordinate C from memory address 258 into $s2

	beq $imm, $zero, $zero, orsh 


orsh:
	add $s0, $zero, $imm, 127
	# add $s0, $zero, $imm, 127 # Load coordinate A from memory address 256 into $s0

# Data Initialization
.word 16 5120   
