* @(#)64        1.7  src/htx/usr/lpp/htx/rules/reg/hxecpu/rules.default, exer_cpu, htxltsbml 4/23/14 04:52:25
* IBM_PROLOG_BEGIN_TAG 
* This is an automatically generated prolog. 
*  
* htxubuntu src/htx/usr/lpp/htx/rules/reg/hxecpu/default.p8 1.1 
*  
* Licensed Materials - Property of IBM 
*  
* COPYRIGHT International Business Machines Corp. 2014,2016 
* All Rights Reserved 
*  
* US Government Users Restricted Rights - Use, duplication or 
* disclosure restricted by GSA ADP Schedule Contract with IBM Corp. 
*  
* IBM_PROLOG_END_TAG 
* Rule file for hxefpu64 exerciser running on P6
* For details of rule keywords, please refer README.hxefpu64
* in /usr/lpp/htx/rules/reg/hxefpu64 dir on test machine.
*
* NOTE: Rule keyword & associated value should be all in one
*	line _only_. Currently '\n' (newline) is used as
*	delimiter to differentiate between keywords.
* Types of CPU instructions: load,storage,branch,cache,external,SPR,rotate,logic,arth,conditional,store
*
* All types of CPU Instructions except thread priority instructions
rule_id						test1
num_oper					5000 
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
INS_BIAS_MASK               [(0x05000000000037ff,100):(0x05000000000037ff,100):(0x05000000000037ff,100):(0x05000000000037ff,100):(0x05000000000037ff,100):(0x05000000000037ff,100):(0x05000000000037ff,100):(0x05000000000037ff,100)]

* Only LOADS and STORES and Thread Priority Instructions
rule_id						test2
num_oper					5000 
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
INS_BIAS_MASK               [(0x0500000000003841,100):(0x0500000000003841,100):(0x0500000000003841,100):(0x0500000000003841,100):(0x0500000000003841,100):(0x0500000000003841,100):(0x0500000000003841,100):(0x0500000000003841,100)]

* Only ARITHMETIC and ROTATE instructions
rule_id						test3
num_oper					5000 
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
INS_BIAS_MASK               [(0x0500000000000014,100):(0x0500000000000014,100):(0x0500000000000014,100):(0x0500000000000014,100):(0x0500000000000014,100):(0x0500000000000014,100):(0x0500000000000014,100):(0x0500000000000014,100)]

* Only Logical and Conditional Logic Instructions
rule_id						test4
num_oper					5000
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
INS_BIAS_MASK               [(0x050000000000000A,100):(0x050000000000000A,100):(0x050000000000000A,100):(0x050000000000000A,100):(0x050000000000000A,100):(0x050000000000000A,100):(0x050000000000000A,100):(0x050000000000000A,100)]


* Only  cache,storage and external instructions
rule_id						test5
num_oper					5000
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				1000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
INS_BIAS_MASK               [(0x0500000000000380,100):(0x0500000000000380,100):(0x0500000000000380,100):(0x0500000000000380,100):(0x0500000000000380,100):(0x0500000000000380,100):(0x0500000000000380,100):(0x0500000000000380,100)]

* Only  Branch Instructions
rule_id						test6
num_oper					5000	
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
INS_BIAS_MASK               [(0x0500000000000400,100):(0x0500000000000400,100):(0x0500000000000400,100):(0x0500000000000400,100):(0x0500000000000400,100):(0x0500000000000400,100):(0x0500000000000400,100):(0x0500000000000400,100)]

* VMX only. With de-norm data only.
rule_id						test7
num_oper					5000
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
fpscr						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
data_bias_mask				[0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0, 0xa0]
ins_bias_mask				[(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100)]

* VMX only. With norm and de-norm data.
rule_id						test8
num_oper					5000
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
fpscr						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
data_bias_mask				[0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55]
ins_bias_mask				[(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100):(0x0400000003ffffff,100)]

* VSX only. With norm and de-norm data.
rule_id						test9
num_oper					5000
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
fpscr						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
data_bias_mask				[0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55]
ins_bias_mask				[(0x0100001fffffffff,100):(0x0100001fffffffff,100):(0x0100001fffffffff,100):(0x0100001fffffffff,100):(0x0100001fffffffff,100):(0x0100001fffffffff,100):(0x0100001fffffffff,100):(0x0100001fffffffff,100)]

* 50% VSX and 50% VMX. With norm and de-norm data.
rule_id						test10
num_oper					5000
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
fpscr						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
data_bias_mask				[0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55]
ins_bias_mask				[(0x0100001fffffffff,50)(0x0400000003ffffff,50):(0x0100001fffffffff,50)(0x0400000003ffffff,50):(0x0100001fffffffff,50)(0x0400000003ffffff,50):(0x0100001fffffffff,50)(0x0400000003ffffff,50):(0x0100001fffffffff,50)(0x0400000003ffffff,50):(0x0100001fffffffff,50)(0x0400000003ffffff,50):(0x0100001fffffffff,50)(0x0400000003ffffff,50):(0x0100001fffffffff,50)(0x0400000003ffffff,50)]

* 50% VSX+VMX and 50% rest of all instructions. With norm and de-norm data.
rule_id						test11
num_oper					5000
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
fpscr						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
data_bias_mask				[0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55]
ins_bias_mask				[(0x0100001fffffffff,25)(0x0400000003ffffff,25)(0x0500000000003fff,50):(0x0100001fffffffff,25)(0x0400000003ffffff,25)(0x0500000000003fff,50):(0x0100001fffffffff,25)(0x0400000003ffffff,25)(0x0500000000003fff,50):(0x0100001fffffffff,25)(0x0400000003ffffff,25)(0x0500000000003fff,50):(0x0100001fffffffff,25)(0x0400000003ffffff,25)(0x0500000000003fff,50):(0x0100001fffffffff,25)(0x0400000003ffffff,25)(0x0500000000003fff,50):(0x0100001fffffffff,25)(0x0400000003ffffff,25)(0x0500000000003fff,50):(0x0100001fffffffff,25)(0x0400000003ffffff,25)(0x0500000000003fff,50)]

* 50% Atomic Load & Stores and 50% all other CPU instructions
rule_id						test12
num_oper					30000
num_threads					0
seed						[0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0]
stream_depth				2000
test_method					1
unaligned_loads_stores_%	[0,0,0,0,0,0,0,0]
INS_BIAS_MASK               [(0x0500000000003000,50)(0x0500000000000fff,50):(0x0500000000003000,50)(0x0500000000000fff,50):(0x0500000000003000,50)(0x0500000000000fff,50):(0x0500000000003000,50)(0x0500000000000fff,50):(0x0500000000003000,50)(0x0500000000000fff,50):(0x0500000000003000,50)(0x0500000000000fff,50):(0x0500000000003000,50)(0x0500000000000fff,50):(0x0500000000003000,50)(0x0500000000000fff,50)]
