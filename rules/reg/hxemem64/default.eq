* IBM_PROLOG_BEGIN_TAG 
* This is an automatically generated prolog. 
*  
* htxubuntu src/htx/usr/lpp/htx/rules/reg/hxemem64/default.eq 1.1 
*  
* Licensed Materials - Property of IBM 
*  
* COPYRIGHT International Business Machines Corp. 2016 
* All Rights Reserved 
*  
* US Government Users Restricted Rights - Use, duplication or 
* disclosure restricted by GSA ADP Schedule Contract with IBM Corp. 
*  
* IBM_PROLOG_END_TAG 
*******Global Rules**********
global_alloc_huge_page = no
global_disable_filters = no
*********************************
rule_id = mem_eq_test
oper = mem
pattern_id = HEXFF(8) HEXZEROS(8) 0x5555555555555555
switch_pat_per_seg = all
num_oper = 10
num_writes = 1
num_read_only = 1
num_read_comp = 1
width = 8
affinity = local
mem_filter = N*P*[4K_100%,64K_100%]
