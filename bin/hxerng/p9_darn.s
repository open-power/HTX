# %Z%%M%       %I%  %W% %G% %U%

.machine "any"
.file "p9_darn.s"

.globl read_rn_32
# L=0, 32-bit random number.
read_rn_32:
	.long 0x7C6005E6 #darn R3, 0
	blr

.globl read_rn_64_cond
# L=1, 64-bit conditioned random number.
read_rn_64_cond:
	.long 0x7C6105E6 #darn R3,1
	blr

.globl read_rn_64_raw
# L=2, 64-bit raw random number.
read_rn_64_raw:
	.long 0x7C6205E6 #darn R3,2
	blr
