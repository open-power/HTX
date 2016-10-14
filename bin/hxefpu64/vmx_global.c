/* IBM_PROLOG_BEGIN_TAG */
/* 
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

/*static char sccsid[] = "%Z%%M%	%I%  %W% %G% %U%";*/

#include "framework.h"

extern struct server_data global_sdata[];
extern struct ruleinfo rule;

/* vector instructions */

struct instruction_masks vmx_instructions_array[] = {
/*  VMX load instruction  */

/*  lvebx	*/  {0x7C00000E, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x30, "lvebx", (sim_fptr)&simulate_lvebx, VMX_LOAD_ONLY, FORM_VT_VA_VB},
/*  lvehx	*/  {0x7C00004E, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x30, "lvehx", (sim_fptr)&simulate_lvehx, VMX_LOAD_ONLY, FORM_VT_VA_VB},
/*  lvewx	*/  {0X7C00008E, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x30, "lvewx", (sim_fptr)&simulate_lvewx, VMX_LOAD_ONLY, FORM_VT_VA_VB},
/*  lvx		*/  {0x7C0000CE, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x30, "lvx", (sim_fptr)&simulate_lvx, VMX_LOAD_ONLY, FORM_VT_VA_VB},
/*  lvxl	*/  {0x7C0002CE, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x30, "lvxl", (sim_fptr)&simulate_lvxl, VMX_LOAD_ONLY, FORM_VT_VA_VB},

/*  VMX store instruction  */

/*  stvebx	*/  {0x7C00010E, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x31, "stvebx", (sim_fptr)&simulate_stvebx, VMX_STORE_ONLY, FORM_VT_VA_VB},
/*  stvehx	*/  {0x7C00014E, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x31, "stvehx", (sim_fptr)&simulate_stvehx, VMX_STORE_ONLY, FORM_VT_VA_VB},
/*  stvewx	*/  {0x7C00018E, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x31, "stvewx", (sim_fptr)&simulate_stvewx, VMX_STORE_ONLY, FORM_VT_VA_VB},
/*  stvx	*/  {0x7C0001CE, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x31, "stvx", (sim_fptr)&simulate_stvx, VMX_STORE_ONLY, FORM_VT_VA_VB},
/*  stvxl	*/  {0x7C0003CE, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x31, "stvxl", (sim_fptr)&simulate_stvxl, VMX_STORE_ONLY, FORM_VT_VA_VB},

/*  VMX alignment instruction  */

/*  lvsl	*/  {0x7C00000C, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x30, "lvsl", (sim_fptr)&simulate_lvsl, VMX_LOAD_ALIGNMENT_ONLY, FORM_VT_VA_VB},
/*  lvsr	*/  {0x7C00004C, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x30, "lvsr", (sim_fptr)&simulate_lvsr, VMX_LOAD_ALIGNMENT_ONLY, FORM_VT_VA_VB},

/* Vector pack/Unpack inst  */

/*  vpkpx	*/  {0x1000030E, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkpx", (sim_fptr)&simulate_vpkpx, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkshss	*/  {0x1000018E, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkshss", (sim_fptr)&simulate_vpkshss, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkswss	*/  {0x100001CE, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkswss", (sim_fptr)&simulate_vpkswss, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkshus	*/  {0x1000010E, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkshus", (sim_fptr)&simulate_vpkshus, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkswus	*/  {0x1000014E, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkswus", (sim_fptr)&simulate_vpkswus, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkuhum	*/  {0x1000000E, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkuhum", (sim_fptr)&simulate_vpkuhum, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkuwum	*/  {0x1000004E, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkuwum", (sim_fptr)&simulate_vpkuwum, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkuhus	*/  {0x1000008E, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkuhus", (sim_fptr)&simulate_vpkuhus, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkuwus	*/  {0x100000CE, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkuwus", (sim_fptr)&simulate_vpkuwus, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vupkhpx	*/  {0x1000034E, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vupkhpx", (sim_fptr)&simulate_vupkhpx, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vupkhsb	*/  {0x1000020E, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vupkhsb", (sim_fptr)&simulate_vupkhsb, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vupkhsh	*/  {0x1000024E, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vupkhsh", (sim_fptr)&simulate_vupkhsh, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vupklpx	*/  {0x100003CE, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vupklpx", (sim_fptr)&simulate_vupklpx, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vupklsb	*/  {0x1000028E, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vupklsb", (sim_fptr)&simulate_vupklsb, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vupklsh	*/  {0x100002CE, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vupklsh", (sim_fptr)&simulate_vupklsh, VMX_MISC_ONLY, FORM_VT_VA_VB},

/*  Vector Merge Inst 	*/
/*  vmrghb	*/  {0x1000000C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmrghb", (sim_fptr)&simulate_vmrghb, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vmrghh	*/  {0x1000004C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmrghh", (sim_fptr)&simulate_vmrghh, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vmrghw	*/ {0x1000008C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmrghw", (sim_fptr)&simulate_vmrghw, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vmrglb  */  {0x1000010C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmrglb", (sim_fptr)&simulate_vmrglb, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vmrglh  */  {0x1000014C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmrglh", (sim_fptr)&simulate_vmrglh, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vmrglw  */  {0x1000018C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmrglw", (sim_fptr)&simulate_vmrglw, VMX_MISC_ONLY, FORM_VT_VA_VB},

/*  Vector Splat Inst	*/

/*  vspltb	*/  {0x1000020C, 0, IMM_DATA_4BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vspltb", (sim_fptr)&simulate_vspltb, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vsplth  */  {0x1000024C, 0, IMM_DATA_3BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsplth", (sim_fptr)&simulate_vsplth, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vspltw  */  {0x1000028C, 0, IMM_DATA_2BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vspltw", (sim_fptr)&simulate_vspltw, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vspltisb*/  {0x1000030C, 0, IMM_DATA, 16, DUMMY, DUMMY, DUMMY, DUMMY, QGPR, 21, 0x32, "vspltisb", (sim_fptr)&simulate_vspltisb, VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vspltish	*/  {0x1000034C, 0, IMM_DATA, 16, DUMMY, DUMMY, DUMMY, DUMMY, QGPR, 21, 0x32, "vspltish", (sim_fptr)&simulate_vspltish, VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vspltisw	*/  {0x1000038C, 0, IMM_DATA, 16, DUMMY, DUMMY, DUMMY, DUMMY, QGPR, 21, 0x32, "vspltisw", (sim_fptr)&simulate_vspltisw, VMX_MISC_ONLY, FORM_VT_VA_VB},

/* vector permute Inst  */

/*  vperm 	*/  {0x1000002B, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vperm", (sim_fptr)&simulate_vperm, VMX_MISC_ONLY, FORM_VT_VA_VB_VC},
/*  Vsel	*/  {0x1000002A, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vsel", (sim_fptr)&simulate_vsel, VMX_MISC_ONLY, FORM_VT_VA_VB_VC},

/*  Vector Shift Inst  */

/*  vsl		  {0x100001C4, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsl", (sim_fptr)&simulate_vsl, VMX_MISC_ONLY, FORM_VT_VA_VB}, */
/*  vsldoi	*/  {0x1000002C, 0, QGPR, 16, QGPR, 11, IMM_DATA, 6, QGPR, 21, 0x33, "vsldoi", (sim_fptr)&simulate_vsldoi, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vslo	*/  {0x1000040C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vslo", (sim_fptr)&simulate_vslo, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vsr		  {0x100002C4, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsr", (sim_fptr)&simulate_vsr, VMX_MISC_ONLY, FORM_VT_VA_VB}, */
/* vsro		*/  {0x1000044C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsro", (sim_fptr)&simulate_vsro, VMX_MISC_ONLY, FORM_VT_VA_VB},

/*  Vector Integer Arithmetic  */

/*  vaddcuw	*/  {0x10000180, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vaddcuw", (sim_fptr)&simulate_vaddcuw, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vaddsbs	*/  {0x10000300, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vaddsbs", (sim_fptr)&simulate_vaddsbs, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vaddshs	*/  {0x10000340, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vaddshs", (sim_fptr)&simulate_vaddshs, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vaddsws	*/  {0x10000380, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vaddsws", (sim_fptr)&simulate_vaddsws, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vaddubm	*/  {0x10000000, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vaddubm", (sim_fptr)&simulate_vaddubm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vadduhm	*/  {0x10000040, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vadduhm", (sim_fptr)&simulate_vadduhm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vadduwm	*/  {0x10000080, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vadduwm", (sim_fptr)&simulate_vadduwm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vaddubs	*/  {0x10000200, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vaddubs", (sim_fptr)&simulate_vaddubs, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vadduhs	*/  {0x10000240, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vadduhs", (sim_fptr)&simulate_vadduhs, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vadduws	*/  {0x10000280, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vadduws", (sim_fptr)&simulate_vadduws, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},

/*  vsubcuw	*/  {0x10000580, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubcuw", (sim_fptr)&simulate_vsubcuw, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vsubsbs	*/  {0x10000700, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubsbs", (sim_fptr)&simulate_vsubsbs, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vsubshs	*/  {0x10000740, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubshs", (sim_fptr)&simulate_vsubshs, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vsubsws	*/  {0x10000780, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubsws", (sim_fptr)&simulate_vsubsws, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vsububm */  {0x10000400, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsububm", (sim_fptr)&simulate_vsububm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vsubuhm */  {0x10000440, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubuhm", (sim_fptr)&simulate_vsubuhm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vsubuwm */  {0x10000480, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubuwm", (sim_fptr)&simulate_vsubuwm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vsububs	*/  {0x10000600, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsububs", (sim_fptr)&simulate_vsububs, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vsubuhs	*/  {0x10000640, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubuhs", (sim_fptr)&simulate_vsubuhs, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vsubuws	*/  {0x10000680, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubuws", (sim_fptr)&simulate_vsubuws, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},

/*  vmulesb	*/  {0x10000308, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmulesb", (sim_fptr)&simulate_vmulesb, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmulesh	*/  {0x10000348, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmulesh", (sim_fptr)&simulate_vmulesh, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmuleub	*/  {0x10000208, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmuleub", (sim_fptr)&simulate_vmuleub, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vmuleuh	*/  {0x10000248, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmuleuh", (sim_fptr)&simulate_vmuleuh, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vmulosb	*/  {0x10000108, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmulosb", (sim_fptr)&simulate_vmulosb, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmulosh	*/  {0x10000148, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmulosh", (sim_fptr)&simulate_vmulosh, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmuloub	*/  {0x10000008, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmuloub", (sim_fptr)&simulate_vmuloub, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vmulouh	*/  {0x10000048, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmulouh", (sim_fptr)&simulate_vmulouh, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},

/*  vmhaddshs	*/  {0x10000020, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmhaddshs", (sim_fptr)&simulate_vmhaddshs, VMX_INT_MUL_ADD_SIGNED_ONLY, FORM_VT_VA_VB_VC},
/*  vmhraddshs	*/  {0x10000021, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmhraddshs", (sim_fptr)&simulate_vmhraddshs, VMX_INT_MUL_ADD_SIGNED_ONLY, FORM_VT_VA_VB_VC},
/*  vmladduhm  	*/  {0x10000022, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmladduhm", (sim_fptr)&simulate_vmladduhm, VMX_INT_MUL_ADD_UNSIGNED_ONLY, FORM_VT_VA_VB_VC},
/*  vmsumubm  	*/  {0x10000024, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmsumubm", (sim_fptr)&simulate_vmsumubm, VMX_INT_MUL_ADD_UNSIGNED_ONLY, FORM_VT_VA_VB_VC},
/*  vmsummbm  	*/  {0x10000025, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmsummbm", (sim_fptr)&simulate_vmsummbm, VMX_INT_MUL_ADD_SIGNED_ONLY, FORM_VT_VA_VB_VC},
/*  vmsumshm  	*/  {0x10000028, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmsumshm", (sim_fptr)&simulate_vmsumshm, VMX_INT_MUL_ADD_SIGNED_ONLY, FORM_VT_VA_VB_VC},
/*  vmsumuhm  	*/  {0x10000026, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmsumuhm", (sim_fptr)&simulate_vmsumuhm, VMX_INT_MUL_ADD_UNSIGNED_ONLY, FORM_VT_VA_VB_VC},
/*  vmsumshs  	*/  {0x10000029, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmsumshs", (sim_fptr)&simulate_vmsumshs, VMX_INT_MUL_ADD_SIGNED_ONLY, FORM_VT_VA_VB_VC},
/*  vmsumuhs  	*/  {0x10000027, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmsumuhs", (sim_fptr)&simulate_vmsumuhs, VMX_INT_MUL_ADD_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vsumsws  	*/  {0x10000788, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsumsws", (sim_fptr)&simulate_vsumsws, VMX_INT_SUM_ACROSS_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vsum2sws  	*/  {0x10000688, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsum2sws", (sim_fptr)&simulate_vsum2sws, VMX_INT_SUM_ACROSS_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vsum4sbs  	*/  {0x10000708, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsum4sbs", (sim_fptr)&simulate_vsum4sbs, VMX_INT_SUM_ACROSS_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vsum4ubs  	*/  {0x10000608, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsum4ubs", (sim_fptr)&simulate_vsum4ubs, VMX_INT_SUM_ACROSS_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vsum4shs  	*/  {0x10000648, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsum4shs", (sim_fptr)&simulate_vsum4shs, VMX_INT_SUM_ACROSS_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vavgsb	*/  {0x10000502, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vavgsb", (sim_fptr)&simulate_vavgsb, VMX_INT_AVERAGE_SIGNED_ONLY , FORM_VT_VA_VB},
/*  vavgsh	*/  {0x10000542, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vavgsh", (sim_fptr)&simulate_vavgsh, VMX_INT_AVERAGE_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vavgsw	*/  {0x10000582, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vavgsw", (sim_fptr)&simulate_vavgsw, VMX_INT_AVERAGE_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vavgub	*/  {0x10000402, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vavgub", (sim_fptr)&simulate_vavgub, VMX_INT_AVERAGE_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vavguh	*/  {0x10000442, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vavguh", (sim_fptr)&simulate_vavguh, VMX_INT_AVERAGE_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vavguw	*/  {0x10000482, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vavguw", (sim_fptr)&simulate_vavguw, VMX_INT_AVERAGE_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vmaxsb	*/  {0x10000102, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmaxsb", (sim_fptr)&simulate_vmaxsb, VMX_INT_MAX_MIN_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmaxsh	*/  {0x10000142, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmaxsh", (sim_fptr)&simulate_vmaxsh, VMX_INT_MAX_MIN_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmaxsw	*/  {0x10000182, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmaxsw", (sim_fptr)&simulate_vmaxsw, VMX_INT_MAX_MIN_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmaxub	*/  {0x10000002, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmaxub", (sim_fptr)&simulate_vmaxub, VMX_INT_MAX_MIN_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vmaxuh	*/  {0x10000042, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmaxuh", (sim_fptr)&simulate_vmaxuh, VMX_INT_MAX_MIN_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vmaxuw	*/  {0x10000082, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmaxuw", (sim_fptr)&simulate_vmaxuw, VMX_INT_MAX_MIN_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vminsb	*/  {0x10000302, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vminsb", (sim_fptr)&simulate_vminsb, VMX_INT_MAX_MIN_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vminsh	*/  {0x10000342, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vminsh", (sim_fptr)&simulate_vminsh, VMX_INT_MAX_MIN_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vminsw	*/  {0x10000382, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vminsw", (sim_fptr)&simulate_vminsw, VMX_INT_MAX_MIN_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vminub	*/  {0x10000202, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vminub", (sim_fptr)&simulate_vminub, VMX_INT_MAX_MIN_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vminuh	*/  {0x10000242, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vminuh", (sim_fptr)&simulate_vminuh, VMX_INT_MAX_MIN_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vminuw	*/  {0x10000282, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vminuw", (sim_fptr)&simulate_vminuw, VMX_INT_MAX_MIN_UNSIGNED_ONLY, FORM_VT_VA_VB},

/* Vector Integer Compare Instruction  */

/*  vcmpequb	*/  {0x10000006, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpequb", (sim_fptr)&simulate_vcmpequb, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpequb.	*/  {0x10000406, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpequb.", (sim_fptr)&simulate_vcmpequb_dot, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpequh	*/  {0x10000046, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpequh", (sim_fptr)&simulate_vcmpequh, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpequh.	*/  {0x10000446, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpequh.", (sim_fptr)&simulate_vcmpequh_dot, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpequw	*/  {0x10000086, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpequw", (sim_fptr)&simulate_vcmpequw, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpequw.	*/  {0x10000486, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpequw.", (sim_fptr)&simulate_vcmpequw_dot, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtsb	*/  {0x10000306, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtsb", (sim_fptr)&simulate_vcmpgtsb, VMX_INT_CMP_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtsb.	*/  {0x10000706, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtsb.", (sim_fptr)&simulate_vcmpgtsb_dot, VMX_INT_CMP_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtsh	*/  {0x10000346, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtsh", (sim_fptr)&simulate_vcmpgtsh, VMX_INT_CMP_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtsh.	*/  {0x10000746, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtsh.", (sim_fptr)&simulate_vcmpgtsh_dot, VMX_INT_CMP_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtsw	*/  {0x10000386, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtsw", (sim_fptr)&simulate_vcmpgtsw, VMX_INT_CMP_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtsw.	*/  {0x10000786, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtsw.", (sim_fptr)&simulate_vcmpgtsw_dot, VMX_INT_CMP_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtub	*/  {0x10000206, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtub", (sim_fptr)&simulate_vcmpgtub, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtub.	*/  {0x10000606, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtub.", (sim_fptr)&simulate_vcmpgtub_dot, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtuh	*/  {0x10000246, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtuh", (sim_fptr)&simulate_vcmpgtuh, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtuh.	*/  {0x10000646, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtuh.", (sim_fptr)&simulate_vcmpgtuh_dot, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtuw	*/  {0x10000286, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtuw", (sim_fptr)&simulate_vcmpgtuw, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtuw.	*/  {0x10000686, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtuw.", (sim_fptr)&simulate_vcmpgtuw_dot, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},

/*  Vector Logical Instruction  */

/*  vand 	*/  {0x10000404, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vand", (sim_fptr)&simulate_vand, VMX_INT_LOGICAL_ONLY, FORM_VT_VA_VB},
/*  vandc 	*/  {0x10000444, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vandc", (sim_fptr)&simulate_vandc, VMX_INT_LOGICAL_ONLY, FORM_VT_VA_VB},
/*  vnor 	*/  {0x10000504, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vnor", (sim_fptr)&simulate_vnor, VMX_INT_LOGICAL_ONLY, FORM_VT_VA_VB},
/*  vor 	*/  {0x10000484, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vor", (sim_fptr)&simulate_vor, VMX_INT_LOGICAL_ONLY, FORM_VT_VA_VB},
/*  vxor 	*/  {0x100004C4, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vxor", (sim_fptr)&simulate_vxor, VMX_INT_LOGICAL_ONLY, FORM_VT_VA_VB},

/*  Vector Integer Shift and Rotate instrutcion  */

/*  vrlb	*/  {0x10000004, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vrlb", (sim_fptr)&simulate_vrlb, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vrlh	*/  {0x10000044, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vrlh", (sim_fptr)&simulate_vrlh, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vrlw	*/  {0x10000084, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vrlw", (sim_fptr)&simulate_vrlw, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vslb	*/  {0x10000104, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vslb", (sim_fptr)&simulate_vslb, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vslh	*/  {0x10000144, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vslh", (sim_fptr)&simulate_vslh, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vslw	*/  {0x10000184, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vslw", (sim_fptr)&simulate_vslw, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vsrb	*/  {0x10000204, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsrb", (sim_fptr)&simulate_vsrb, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vsrh	*/  {0x10000244, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsrh", (sim_fptr)&simulate_vsrh, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vsrw	*/  {0x10000284, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsrw", (sim_fptr)&simulate_vsrw, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vsrab	*/  {0x10000304, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsrab", (sim_fptr)&simulate_vsrab, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vsrah	*/  {0x10000344, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsrah", (sim_fptr)&simulate_vsrah, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vsraw	*/  {0x10000384, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsraw", (sim_fptr)&simulate_vsraw, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},

/*  Vector Floating Pt. Arithmetic  */

/*  vaddfp	*/  {0x1000000A, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vaddfp", (sim_fptr)&simulate_vaddfp, VMX_FP_ARITHMETIC_ONLY, FORM_VT_VA_VB},
/*  vsubfp	*/  {0x1000004A, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vsubfp", (sim_fptr)&simulate_vsubfp, VMX_FP_ARITHMETIC_ONLY, FORM_VT_VA_VB},
/*  vmaddfp	*/  {0x1000002E, 0, VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 6, VECTOR_SP, 21, 0x33, "vmaddfp", (sim_fptr)&simulate_vmaddfp, VMX_FP_MUL_ADD_SUB_ONLY, FORM_VT_VA_VB_VC},
/*  vnmsubfp*/  {0x1000002F, 0, VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 6, VECTOR_SP, 21, 0x33, "vnmsubfp", (sim_fptr)&simulate_vnmsubfp, VMX_FP_MUL_ADD_SUB_ONLY, FORM_VT_VA_VB_VC},
/*  vmaxfp	*/  {0x1000040A, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vmaxfp", (sim_fptr)&simulate_vmaxfp, VMX_FP_MAX_MIN_ONLY, FORM_VT_VA_VB},
/*  vminfp	*/  {0x1000044A, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vminfp", (sim_fptr)&simulate_vminfp, VMX_FP_MAX_MIN_ONLY, FORM_VT_VA_VB},

/*  Vector Floating Pt. Rounding & Conversion  */

/*  vctsxs	*/  {0x100003CA, 0, VECTOR_SP, 16, IMM_DATA, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vctsxs", (sim_fptr)&simulate_vctsxs, VMX_FP_ROUND_CONV_ONLY, FORM_VT_VA_VB},
/*  vctuxs	*/  {0x1000038A, 0, VECTOR_SP, 16, IMM_DATA, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vctuxs", (sim_fptr)&simulate_vctuxs, VMX_FP_ROUND_CONV_ONLY, FORM_VT_VA_VB},
/*  vcfsx	*/  {0x1000034A, 0, VECTOR_SP, 16, IMM_DATA, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcfsx", (sim_fptr)&simulate_vcfsx, VMX_FP_ROUND_CONV_ONLY, FORM_VT_VA_VB},
/*  vcfux	*/  {0x1000030A, 0, VECTOR_SP, 16, IMM_DATA, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcfux", (sim_fptr)&simulate_vcfux, VMX_FP_ROUND_CONV_ONLY, FORM_VT_VA_VB},
/*  vrfim  	*/  {0x100002CA, 0, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vrfim", (sim_fptr)&simulate_vrfim, VMX_FP_ROUND_CONV_ONLY, FORM_VT_VA_VB},
/*  vrfin  	*/  {0x1000020A, 0, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vrfin", (sim_fptr)&simulate_vrfin, VMX_FP_ROUND_CONV_ONLY, FORM_VT_VA_VB},
/*  vrfip  	*/  {0x1000028A, 0, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vrfip", (sim_fptr)&simulate_vrfip, VMX_FP_ROUND_CONV_ONLY, FORM_VT_VA_VB},
/*  vrfiz  	*/  {0x1000024A, 0, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vrfiz", (sim_fptr)&simulate_vrfiz, VMX_FP_ROUND_CONV_ONLY, FORM_VT_VA_VB},

/*  Vector Floating Pt. Compare  */

/*  vcmpbfp  	*/  {0x100003C6, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcmpbfp", (sim_fptr)&simulate_vcmpbfp, VMX_FP_CMP_ONLY, FORM_VT_VA_VB},
/*  vcmpbfp.  	*/  {0x100007C6, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcmpbfp.", (sim_fptr)&simulate_vcmpbfp_dot, VMX_FP_CMP_ONLY, FORM_VT_VA_VB},
/*  vcmpeqfp  	*/  {0x100000C6, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcmpeqfp", (sim_fptr)&simulate_vcmpeqfp, VMX_FP_CMP_ONLY, FORM_VT_VA_VB},
/*  vcmpeqfp.  	*/  {0x100004C6, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcmpeqfp.", (sim_fptr)&simulate_vcmpeqfp_dot, VMX_FP_CMP_ONLY, FORM_VT_VA_VB},
/*  vcmpgefp  	*/  {0x100001C6, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcmpgefp", (sim_fptr)&simulate_vcmpgefp, VMX_FP_CMP_ONLY, FORM_VT_VA_VB},
/*  vcmpgefp.  	*/  {0x100005C6, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcmpgefp.", (sim_fptr)&simulate_vcmpgefp_dot, VMX_FP_CMP_ONLY, FORM_VT_VA_VB},
/*  vcmpgtfp  	*/  {0x100002C6, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcmpgtfp", (sim_fptr)&simulate_vcmpgtfp, VMX_FP_CMP_ONLY, FORM_VT_VA_VB},
/*  vcmpgtfp.  	*/  {0x100006C6, 0, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vcmpgtfp.", (sim_fptr)&simulate_vcmpgtfp_dot, VMX_FP_CMP_ONLY, FORM_VT_VA_VB},


#if 0 /* Estimate instructions not supported yet */
/*  Vector Floating Pt. Estimate  */

/*  vexptefp 	*/  {0x1000018A, 0, DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vexptefp", (sim_fptr)&simulate_vexptefp, VMX_FP_ESTIMATE_ONLY},
/*  vlogefp		*/  {0x100001CA, 0, DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vlogefp", (sim_fptr)&simulate_vlogefp, VMX_FP_ESTIMATE_ONLY, FORM_VT_VA_VB},
/*  vrefp		*/  {0x1000010A, 0, DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vrefp", (sim_fptr)&simulate_vrefp, VMX_FP_ESTIMATE_ONLY, FORM_VT_VA_VB},
/*  vrsqrtefp	*/  {0x1000014A, 0, DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x32, "vrsqrtefp", (sim_fptr)&simulate_vrsqrtefp, VMX_FP_ESTIMATE_ONLY, FORM_VT_VA_VB},
#endif


/*  Vector Status & Control Register  */

/*  mtvscr	*/  {0x10000644, 0, DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x33, "mtvscr", (sim_fptr)&simulate_mtvscr, VMX_VSCR_ONLY, FORM_VT_VA_VB},
/*  mfvscr	*/	{0x10000604, 0, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, VECTOR_SP, 21, 0x33, "mfvscr", (sim_fptr)&simulate_mfvscr, VMX_VSCR_ONLY, FORM_VT_VA_VB},
/* last ins	*/  {0xDEADBEEF, 0, DUMMY, 0, DUMMY, 0, DUMMY, 0, DUMMY, 0, 0x0, "last_instruction"}

};


struct instruction_masks vmx_p8_instructions_array[] = {
	/* All VMX p8 specific instructions */

	/* Vector Merge Instructions */
/*  vmrgew  */  {0x1000078C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmrgew", (sim_fptr)&simulate_vmrgew, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vmrgow  */  {0x1000068C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmrgow", (sim_fptr)&simulate_vmrgow, VMX_MISC_ONLY, FORM_VT_VA_VB},

/* Vector pack/Unpack inst  */
/*  vpksdss */  {0x100005CE, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpksdss", (sim_fptr)&simulate_vpksdss, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpksdus */  {0x1000054E, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpksdus", (sim_fptr)&simulate_vpksdus, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkudum	*/  {0x1000044E, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkudum", (sim_fptr)&simulate_vpkudum, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpkudus */  {0x100004CE, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpkudus", (sim_fptr)&simulate_vpkudus, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vupkhsw */  {0x1000064E, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vupkhsw", (sim_fptr)&simulate_vupkhsw, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vupklsw */  {0x100006CE, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vupklsw", (sim_fptr)&simulate_vupklsw, VMX_MISC_ONLY, FORM_VT_VA_VB},

/*  Vector Integer Arithmetic  */
/*  vaddudm	*/  {0x100000C0, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vaddudm", (sim_fptr)&simulate_vaddudm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vsubudm */  {0x100004C0, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubudm", (sim_fptr)&simulate_vsubudm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vmaxsd	*/  {0x100001C2, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmaxsd", (sim_fptr)&simulate_vmaxsd, VMX_INT_MAX_MIN_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmaxud	*/  {0x100000C2, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmaxud", (sim_fptr)&simulate_vmaxud, VMX_INT_MAX_MIN_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vminsd	*/  {0x100003C2, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vminsd", (sim_fptr)&simulate_vminsd, VMX_INT_MAX_MIN_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vminud	*/  {0x100002C2, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vminud", (sim_fptr)&simulate_vminud, VMX_INT_MAX_MIN_UNSIGNED_ONLY, FORM_VT_VA_VB},

/* Vector Integer Compare Instruction  */
/*  vcmpequd  */  {0x100000C7, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpequd", (sim_fptr)&simulate_vcmpequd, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpequd. */  {0x100004C7, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpequd.", (sim_fptr)&simulate_vcmpequd_dot, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtsd  */  {0x100003C7, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtsd", (sim_fptr)&simulate_vcmpgtsd, VMX_INT_CMP_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtsd. */  {0x100007C7, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtsd.", (sim_fptr)&simulate_vcmpgtsd_dot, VMX_INT_CMP_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtud  */  {0x100002C7, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtud", (sim_fptr)&simulate_vcmpgtud, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vcmpgtud. */  {0x100006C7, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpgtud.", (sim_fptr)&simulate_vcmpgtud_dot, VMX_INT_CMP_UNSIGNED_ONLY, FORM_VT_VA_VB},

/*  Vector Integer Shift and Rotate instrutcion  */
/*  vrld	*/  {0x100000C4, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vrld", (sim_fptr)&simulate_vrld, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vsld	*/  {0x100005C4, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsld", (sim_fptr)&simulate_vsld, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vsrd	*/  {0x100006C4, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsrd", (sim_fptr)&simulate_vsrd, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},
/*  vsrad	*/  {0x100003C4, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsrad", (sim_fptr)&simulate_vsrad, VMX_INT_ROTATE_SHIFT_ONLY, FORM_VT_VA_VB},

/*  Vector Count Leading Zeros instruction  */
/*  vclzb	*/  {0x10000702, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vclzb", (sim_fptr)&simulate_vclzb, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vclzh	*/  {0x10000742, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vclzh", (sim_fptr)&simulate_vclzh, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vclzw	*/  {0x10000782, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vclzw", (sim_fptr)&simulate_vclzw, VMX_MISC_ONLY, FORM_VT_VA_VB},

/* Vector Gather Instructions */
/*  vgbbd	*/  {0x1000050C, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vgbbd", (sim_fptr)&simulate_vgbbd, VMX_MISC_ONLY, FORM_VT_VA_VB},

/*  Vector Crypto Instructions  */
/*  vcipher	*/  	{0x10000508, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcipher", (sim_fptr)&simulate_vcipher, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vcipherlast	*/  {0x10000509, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcipherlast", (sim_fptr)&simulate_vcipherlast, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vncipher	*/ 	{0x10000548, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vncipher", (sim_fptr)&simulate_vncipher, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vncipherlast*/  {0x10000549, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vncipherlast", (sim_fptr)&simulate_vncipherlast, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vsbox		*/ 	{0x100005C8, 0, QGPR, 16, DUMMY, DUMMY, DUMMY, DUMMY, QGPR, 21, 0x32, "vsbox", (sim_fptr)&simulate_vsbox, VMX_MISC_ONLY, FORM_VT_VA_VB},

/*  vshasigmad	*/ 	{0x100006C2, 0, QGPR, 16, IMM_DATA_1BIT, 15, IMM_DATA_4BIT, 11, QGPR, 21, 0x33, "vshasigmad", (sim_fptr)&simulate_vshasigmad, VMX_MISC_ONLY, FORM_VT_VA_ST_SIX},
/*  vshasigmaw	*/ 	{0x10000682, 0, QGPR, 16, IMM_DATA_1BIT, 15, IMM_DATA_4BIT, 11, QGPR, 21, 0x33, "vshasigmaw", (sim_fptr)&simulate_vshasigmaw, VMX_MISC_ONLY, FORM_VT_VA_ST_SIX},

/*  vpmsumb	*/  	{0x10000408, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpmsumb", (sim_fptr)&simulate_vpmsumb, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpmsumd	*/  	{0x100004C8, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpmsumd", (sim_fptr)&simulate_vpmsumd, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpmsumh	*/  	{0x10000448, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpmsumh", (sim_fptr)&simulate_vpmsumh, VMX_MISC_ONLY, FORM_VT_VA_VB},
/*  vpmsumw	*/  	{0x10000488, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpmsumw", (sim_fptr)&simulate_vpmsumw, VMX_MISC_ONLY, FORM_VT_VA_VB},

/*  vpermxor*/  	{0x1000002D, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vpermxor", (sim_fptr)&simulate_vpermxor, VMX_MISC_ONLY, FORM_VT_VA_VB_VC},

/* RFC02241 */
/* veqv		*/		{0x10000684, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "veqv", (sim_fptr)&simulate_veqv, VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vorc		*/		{0x10000544, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vorc", (sim_fptr)&simulate_vorc, VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vnand	*/		{0x10000584, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vnand", (sim_fptr)&simulate_vnand, VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vclzd	*/		{0x100007C2, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vclzd", (sim_fptr)&simulate_vclzd, VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vpopcntb	*/		{0x10000703, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpopcntb", (sim_fptr)&simulate_vpopcntb, VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vpopcnth	*/		{0x10000743, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpopcnth", (sim_fptr)&simulate_vpopcnth, VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vpopcntd	*/		{0x100007C3, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpopcntd", (sim_fptr)&simulate_vpopcntd, VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vpopcntw	*/		{0x10000783, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vpopcntw", (sim_fptr)&simulate_vpopcntw, VMX_MISC_ONLY, FORM_VT_VA_VB},

/* RFC02243 - VMX 32-bit Multiply */
/*  vmulesw	*/  {0x10000388, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmulesw", (sim_fptr)&simulate_vmulesw, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmuleuw	*/  {0x10000288, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmuleuw", (sim_fptr)&simulate_vmuleuw, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vmulosw	*/  {0x10000188, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmulosw", (sim_fptr)&simulate_vmulosw, VMX_INT_ARITHMETIC_SIGNED_ONLY, FORM_VT_VA_VB},
/*  vmulouw	*/  {0x10000088, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmulouw", (sim_fptr)&simulate_vmulouw, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/*  vmuluwm	*/  {0x10000089, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vmuluwm", (sim_fptr)&simulate_vmuluwm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},

/* RFC02244 - VMX Decimal Integer operation */
/* bcdadd. PS=0	*/	{0x10000401, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdadd. PS=0", (sim_fptr)&simulate_bcdadd, VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdadd. PS=1 */	{0x10000601, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdadd. PS=1", (sim_fptr)&simulate_bcdadd, VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdsub. PS=0	*/	{0x10000441, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdsub. PS=0", (sim_fptr)&simulate_bcdsub, VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdsub. PS=1 */	{0x10000641, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdsub. PS=1", (sim_fptr)&simulate_bcdsub, VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},

/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};


struct instruction_masks vmx_p8_dd2_instructions_array[] = {
/* For All instructions supported in P8 DD2 and above */
/* RFC02247.r3 - VMX Vector Integer Add, Vector Integer Substract and Vector Bit Permute instructions */
/* vadduqm  */  {0x10000100, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vadduqm",  (sim_fptr)&simulate_vadduqm,  VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/* vaddeuqm */  {0x1000003C, 0, QGPR, 16, QGPR, 11, QGPR,      6, QGPR, 21, 0x32, "vaddeuqm", (sim_fptr)&simulate_vaddeuqm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB_VC},
/* vaddcuq  */  {0x10000140, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vaddcuq",  (sim_fptr)&simulate_vaddcuq,  VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/* vaddecuq */  {0x1000003D, 0, QGPR, 16, QGPR, 11, QGPR,      6, QGPR, 21, 0x32, "vaddecuq", (sim_fptr)&simulate_vaddecuq, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB_VC},
/* vsubuqm  */  {0x10000500, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubuqm",  (sim_fptr)&simulate_vsubuqm,  VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/* vsubeuqm */  {0x1000003E, 0, QGPR, 16, QGPR, 11, QGPR,      6, QGPR, 21, 0x32, "vsubeuqm", (sim_fptr)&simulate_vsubeuqm, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB_VC},
/* vsubcuq  */  {0x10000540, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vsubcuq",  (sim_fptr)&simulate_vsubcuq,  VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB},
/* vsubecuq */  {0x1000003F, 0, QGPR, 16, QGPR, 11, QGPR,      6, QGPR, 21, 0x32, "vsubecuq", (sim_fptr)&simulate_vsubecuq, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB_VC},
/* vbpermq  */  {0x1000054C, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vbpermq",  (sim_fptr)&simulate_vbpermq,  VMX_MISC_ONLY,                    FORM_VT_VA_VB},
/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};


struct instruction_masks vmx_p9_instructions_array[] = {
/* RFC02462 - Decimal integer support operation */
/* bcdcfn. PS=0 */	{0x10070581, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdcfn. PS=0", (sim_fptr)&simulate_bcdcfn, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdcfn. PS=1 */	{0x10070781, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdcfn. PS=1", (sim_fptr)&simulate_bcdcfn, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdcfz. PS=0 */	{0x10060581, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdcfz. PS=0", (sim_fptr)&simulate_bcdcfz, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdcfz. PS=1 */	{0x10060781, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdcfz. PS=1", (sim_fptr)&simulate_bcdcfz, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdctn. 		*/	{0x10050581, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdctn.",      (sim_fptr)&simulate_bcdctn, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdctz. PS=0 */	{0x10040581, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdctz. PS=0", (sim_fptr)&simulate_bcdctz, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdctz. PS=1 */	{0x10040781, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdctz. PS=1", (sim_fptr)&simulate_bcdctz, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdcfsq. PS=0*/	{0x10020581, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdcfsq. PS=0",(sim_fptr)&simulate_bcdcfsq,P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdcfsq. PS=1*/	{0x10020781, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdcfsq. PS=1",(sim_fptr)&simulate_bcdcfsq,P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdctsq.     */	{0x10000581, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdctsq.",     (sim_fptr)&simulate_bcdctsq,P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* vmul10uq		*/	{0x10000201, 0, QGPR, 16, DUMMY, DUMMY, DUMMY, DUMMY, QGPR, 21, 0x32, "vmul10uq",     (sim_fptr)&simulate_vmul10uq,  P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* vmul10cuq	*/	{0x10000001, 0, QGPR, 16, DUMMY, DUMMY, DUMMY, DUMMY, QGPR, 21, 0x32, "vmul10cuq",    (sim_fptr)&simulate_vmul10cuq, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* vmul10euq	*/	{0x10000241, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "vmul10euq",    (sim_fptr)&simulate_vmul10euq, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* vmul10ecuq	*/	{0x10000041, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "vmul10ecuq",   (sim_fptr)&simulate_vmul10ecuq,P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdcpsgn		*/	{0x10000341, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "bcdcpsgn",     (sim_fptr)&simulate_bcdcpsgn,  P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdsetsgn PS=0*/	{0x101F0581, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdsetsgn PS=0",(sim_fptr)&simulate_bcdsetsgn,P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdsetsgn PS=1*/	{0x101F0781, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "bcdsetsgn PS=1",(sim_fptr)&simulate_bcdsetsgn,P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcds. PS=0	*/	{0x100004C1, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "bcds. PS=0",   (sim_fptr)&simulate_bcds,      P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcds. PS=1	*/	{0x100006C1, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "bcds. PS=1",   (sim_fptr)&simulate_bcds,      P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdus.		*/	{0x10000481, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "bcdus.",       (sim_fptr)&simulate_bcdus,     P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdsr. PS=0	*/	{0x100005C1, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "bcdsr. PS=0",  (sim_fptr)&simulate_bcdsr,     P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdsr. PS=1	*/	{0x100007C1, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "bcdsr. PS=1",  (sim_fptr)&simulate_bcdsr,     P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdtrunc. PS=0*/	{0x10000501, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "bcdtrunc. PS=0",(sim_fptr)&simulate_bcdtrunc, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdtrunc. PS=1*/	{0x10000701, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "bcdtrunc. PS=1",(sim_fptr)&simulate_bcdtrunc, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* bcdutrunc.	*/	{0x10000541, 0, QGPR, 16, QGPR, 11,     DUMMY, DUMMY, QGPR, 21, 0x32, "bcdutrunc.",   (sim_fptr)&simulate_bcdutrunc, P9_VMX_DFP_ARITHMETIC, FORM_VT_VA_VB},
/* 
 * RFC02467.r10: String Operations (VSU Option)
 * ---------------------------------------------
 */
/* vcmpneb*/  {0x10000007, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpneb",	(sim_fptr)&simulate_vcmpneb, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpneb.*/ {0x10000407, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpneb.",	(sim_fptr)&simulate_vcmpneb, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpnezb*/ {0x10000107, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpnezb",	(sim_fptr)&simulate_vcmpnezb, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpnezb.*/{0x10000507, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpnezb.",(sim_fptr)&simulate_vcmpnezb, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpneh*/  {0x10000047, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpneh",	(sim_fptr)&simulate_vcmpneh, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpneh.*/ {0x10000447, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpneh.",	(sim_fptr)&simulate_vcmpneh, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpnezh*/ {0x10000147, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpnezh",	(sim_fptr)&simulate_vcmpnezh, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpnezh.*/{0x10000547, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpnezh.",(sim_fptr)&simulate_vcmpnezh, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpnew*/  {0x10000087, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpnew",	(sim_fptr)&simulate_vcmpnew, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpnew.*/ {0x10000487, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpnew.",	(sim_fptr)&simulate_vcmpnew, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpnezw*/ {0x10000187, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpnezw",	(sim_fptr)&simulate_vcmpnezw, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vcmpnezw.*/{0x10000587, 0, QGPR, 16,  QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vcmpnezw.",(sim_fptr)&simulate_vcmpnezw, P9_VMX_INT_LOGICAL_ONLY,FORM_VT_VA_VB},
/* vclzlsbb*/ {0x10000602, 0, DUMMY, 16, QGPR, 11, DUMMY, DUMMY, GR, 21, 0x32, "vclzlsbb",	(sim_fptr)&simulate_vclzlsbb, P9_VMX_INT_ARITHMETIC_UNSIGNED_ONLY,FORM_VT_VA_VB},
/* vctzlsbb*/ {0x10010602, 0, DUMMY, 16, QGPR, 11, DUMMY, DUMMY, GR, 21, 0x32, "vctzlsbb",	(sim_fptr)&simulate_vctzlsbb, P9_VMX_INT_ARITHMETIC_UNSIGNED_ONLY,FORM_VT_VA_VB},
/* vextublx*/ {0x1000060D, 0, GR,    16, QGPR, 11, DUMMY, DUMMY, GR, 21, 0x32, "vextublx",	(sim_fptr)&simulate_vextublx, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vextubrx*/ {0x1000070D, 0, GR,    16, QGPR, 11, DUMMY, DUMMY, GR, 21, 0x32, "vextubrx",	(sim_fptr)&simulate_vextubrx, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vextuhlx*/ {0x1000064D, 0, GR,    16, QGPR, 11, DUMMY, DUMMY, GR, 21, 0x32, "vextuhlx",	(sim_fptr)&simulate_vextuhlx, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vextuhrx*/ {0x1000074D, 0, GR,    16, QGPR, 11, DUMMY, DUMMY, GR, 21, 0x32, "vextuhrx",	(sim_fptr)&simulate_vextuhrx, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vextuwlx*/ {0x1000068D, 0, GR,    16, QGPR, 11, DUMMY, DUMMY, GR, 21, 0x32, "vextuwlx",	(sim_fptr)&simulate_vextuwlx, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vextuwrx*/ {0x1000078D, 0, GR,    16, QGPR, 11, DUMMY, DUMMY, GR, 21, 0x32, "vextuwrx",	(sim_fptr)&simulate_vextuwrx, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/*
 * RFC02469.r1: 128-bit SIMD Video Compression Operations
 */
/* vabsdub*/ {0x10000403, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vabsdub",	(sim_fptr)&simulate_vabsdub, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vabsduh*/ {0x10000443, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vabsduh",	(sim_fptr)&simulate_vabsduh, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vabsduw*/ {0x10000483, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vabsduw",	(sim_fptr)&simulate_vabsduw, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* 
 * RFC02470 128-bit SIMD FXU Operations
 * ---------------------------------------------
 */
/* vbpermd*/ {0x100005CC, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vbpermd",	(sim_fptr)&simulate_vbpermd, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vctzb*/ {0x101C0602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vctzb",	(sim_fptr)&simulate_vctzb, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vctzd*/ {0x101F0602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vctzd",	(sim_fptr)&simulate_vctzd, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vctzh*/ {0x101D0602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vctzh",	(sim_fptr)&simulate_vctzh, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vctzw*/ {0x101E0602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vctzw",	(sim_fptr)&simulate_vctzw, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vextsb2d*/ {0x10180602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vextsb2d",	(sim_fptr)&simulate_vextsb2d, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vextsb2w*/ {0x10100602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vextsb2w",	(sim_fptr)&simulate_vextsb2w, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vextsh2d*/ {0x10190602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vextsh2d",	(sim_fptr)&simulate_vextsh2d, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vextsh2w*/ {0x10110602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vextsh2w",	(sim_fptr)&simulate_vextsh2w, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vextsw2d*/ {0x101A0602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vextsw2d",	(sim_fptr)&simulate_vextsw2d, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vnegd*/ {0x10070602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vnegd",	(sim_fptr)&simulate_vnegd, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vnegw*/ {0x10060602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vnegw",	(sim_fptr)&simulate_vnegw, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vprtybd*/ {0x10090602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vprtybd",	(sim_fptr)&simulate_vprtybd, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vprtybq*/ {0x100A0602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vprtybq",	(sim_fptr)&simulate_vprtybq, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vprtybw*/ {0x10080602, 0, DUMMY, DUMMY, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vprtybw",	(sim_fptr)&simulate_vprtybw, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* vrldmi*/ {0x100000C5, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vrldmi",	(sim_fptr)&simulate_vrldmi, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vrldnm*/ {0x100001C5, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vrldnm",	(sim_fptr)&simulate_vrldnm, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vrlwmi*/ {0x10000085, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vrlwmi",	(sim_fptr)&simulate_vrlwmi, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/* vrlwnm*/ {0x10000185, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vrlwnm",	(sim_fptr)&simulate_vrlwnm, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/*
 * RFC02471.r13: 128-bit SIMD Miscellaneous Operations
 */
/*vextractd*/ {0x100002CD, 0, IMM_DATA_4BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vextractd", (sim_fptr)&simulate_vextractd, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/*vextractub*/ {0x1000020D, 0, IMM_DATA_4BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vextractub", (sim_fptr)&simulate_vextractub, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/*vextractuh*/ {0x1000024D, 0, IMM_DATA_4BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vextractuh",	(sim_fptr)&simulate_vextractuh, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/*vextractuw*/ {0x1000028D, 0, IMM_DATA_4BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vextractuw",	(sim_fptr)&simulate_vextractuw, P9_VMX_MISC_ONLY,FORM_VT_VA_VB},
/*vinsertb*/ {0x1000030D, 0, IMM_DATA_4BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vinsertb",	(sim_fptr)&simulate_vinsertb, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/*vinsertd*/ {0x100003CD, 0, IMM_DATA_4BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vinsertd",	(sim_fptr)&simulate_vinsertd, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/*vinserth*/ {0x1000034D, 0, IMM_DATA_4BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vinserth",	(sim_fptr)&simulate_vinserth, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/*vinsertw*/ {0x1000038D, 0, IMM_DATA_4BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x32, "vinsertw",	(sim_fptr)&simulate_vinsertw, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/*vpermr*/ {0x1000003B, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x32, "vpermr", (sim_fptr)&simulate_vpermr, P9_VMX_MISC_ONLY, FORM_VT_VA_VB},
/* last ins indicator */ {0xDEADBEEF, 0,DUMMY, 0, DUMMY, 0, DUMMY, DUMMY, DUMMY, 0, 0x0, "last_instruction", 0, 0, 0}
};

struct instruction_masks vmx_p9_dd2_instructions_array[]={
    /***************************RFC02511 - FXU & Vector Extensions for Blockchain Support**************************************/
    /* vmsumudm */ {0x10000023, 0, QGPR, 16, QGPR, 11, QGPR, 6, QGPR, 21, 0x33, "vmsumudm",  DUMMY, VMX_INT_ARITHMETIC_UNSIGNED_ONLY, FORM_VT_VA_VB_VC},
    /* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};



void class_vmx_load_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
        uint32 vsr4, vmx_vsr4;
        uint32 op1, op2, tgt;
        uint32 mcode, store_off, load_off, addi_mcode;
        uint32 prolog_size, *tc_memory, num_ins_built;
        struct vsr_list *vsrs;
        struct server_data *sdata = &global_sdata[INITIAL_BUF];
        struct client_data *cptr = sdata->cdata_ptr[client_no];

        prolog_size = cptr->prolog_size;
        num_ins_built = cptr->num_ins_built;
        tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

        /* Generate tgt VMX reg and figure out its data type */
        vsrs = &(cptr->vsrs[temp->tgt_dtype]);
        vsr4 = vsrs->head[VMX]->vsr_no;
        vmx_vsr4 =  (vsr4 > 32)?vsr4 - 32: vsr4;
		/* see if the target VSR is dirty. If yes, save it in store area */
        if( (0x1ULL << vsr4) & vsrs->dirty_mask) {
                /* reserve memory for store */
                store_off = init_mem_for_vsx_store(client_no, QGPR);
				addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
                *tc_memory = addi_mcode;
                cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
				cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
                tc_memory++;
                num_ins_built++;
				*tc_memory = STORE_VMX_128((vmx_vsr4));
                /* save offset */
                cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
                cptr->instr_index[prolog_size + num_ins_built] = vmx_stvx | 0x20000000;
				cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stvx;
                tc_memory++;
                num_ins_built++;
        }
        tgt = (vmx_vsr4 & 0x1f) << (temp->tgt_pos);

        /* find out load ptr and initialize the data pattern at that address */
        load_off = init_mem_for_vsx_load(client_no, temp->tgt_dtype);
        /* Create addi instruction as a pre instruction to this load instruction */
        addi_mcode = GEN_ADDI_MCODE(LOAD_RB, 0, load_off);
        *tc_memory = addi_mcode;
        cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;

        tc_memory++;
        num_ins_built++;

        /* Form machine code for load instruction */

        op1 = (LOAD_RA & 0x1f) << (temp->op1_pos);
        op2 = (LOAD_RB & 0x1f) << (temp->op2_pos);
        mcode = (temp->op_eop_mask | op1 | op2 | tgt);
        *tc_memory = mcode;

        /* save offset */
        cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
        cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
        tc_memory++;
        num_ins_built++;

        /* Update masks for tgt vsr */
        vsrs->dirty_mask |= (0x1ULL << vsr4);
        /* Restore the number of instruction built */
        cptr->num_ins_built = num_ins_built;
}

void class_vmx_store_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
        uint32 vsr4, vmx_vsr4;
        uint32 op1, op2, tgt;
        uint32 mcode, store_off, addi_mcode;
        uint32 num_ins_built, prolog_size, *tc_memory;
        struct vsr_list *vsrs;
        struct server_data *sdata = &global_sdata[INITIAL_BUF];
        struct client_data *cptr = sdata->cdata_ptr[client_no];

        prolog_size = cptr->prolog_size;
        num_ins_built = cptr->num_ins_built;
        tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

        /* Generate tgt VMX reg and figure out its data type */
        vsrs = &(cptr->vsrs[temp->tgt_dtype]);
        vsr4 = vsrs->head[VMX]->vsr_no;
        vmx_vsr4 =  (vsr4 > 32)?vsr4 - 32: vsr4;
	/* Reset dirty bit corresponding to target VSR */
        vsrs->dirty_mask &= (~(0x1ULL << vsr4));
        /* reserve memory for store */
        store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
        addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
        *tc_memory = addi_mcode;
        cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
        tc_memory++;
        num_ins_built++;
        op1 = (STORE_RA & 0x1f) << (temp->op1_pos);
        op2 = (STORE_RB & 0x1f) << (temp->op2_pos);
        tgt = (vmx_vsr4 & 0x1f) << (temp->tgt_pos);

        mcode = (temp->op_eop_mask | op1 | op2 | tgt);
        /* save offset */
        *tc_memory = mcode;
        cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
        cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
        tc_memory++;
        num_ins_built++;

        /* Restore the number of instruction built */
        cptr->num_ins_built = num_ins_built;
}

void class_vmx_normal_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
    uint32 vsr1, vsr2, vsr3, vsr4;
    uint32 op1 = 0, op2 = 0, op3 = 0, tgt = 0, tgt_reg_no = 0;
    uint32 vmx_vsr1, vmx_vsr2, vmx_vsr3, vmx_vsr4;
	uint32 mcode, store_off, addi_mcode;
    uint32 prolog_size, *tc_memory, num_ins_built;
    struct vsr_list *vsrs;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];

    prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
    tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	if((temp->op1_dtype != QGPR) && (temp->op1_dtype != VECTOR_SP) && (temp->op1_dtype != DUMMY))
	{
	    switch(temp->op1_dtype)
	    {
		case IMM_DATA_1BIT:
		     op1 = (random_no & 0x1) << (temp->op1_pos);
		case IMM_DATA_2BIT:
		     op1 = (random_no & 0x3) << (temp->op1_pos);
		     break;
		case IMM_DATA_3BIT:
		     op1 = (random_no & 0x7) << (temp->op1_pos);
		    break;
		case IMM_DATA_4BIT:
		    op1 = (random_no & 0xf) << (temp->op1_pos);
		    break;
		case GR:
        	vsrs = &(cptr->vsrs[temp->op1_dtype]);
	    	vsr1 = vsrs->head[BFP]->vsr_no;
	     	MOVE_VSR_TO_END(client_no, temp->op1_dtype, BFP);
		 	vsrs->dirty_mask &= (~(0x1ULL << vsr1));
		    op1 = (vsr1 & 0x1f) << (temp->op1_pos);
		    break;
		default:
		    op1 = (random_no & 0x1f) << (temp->op1_pos);
		    break;
	    }
	}
	else
	{
        vsrs = &(cptr->vsrs[temp->op1_dtype]);
	    vsr1 = vsrs->head[VMX]->vsr_no;
		/*
         * Move this VSR to the end of link list
         */
	     MOVE_VSR_TO_END(client_no, temp->op1_dtype, VMX);
		 vsrs->dirty_mask &= (~(0x1ULL << vsr1));
		 vmx_vsr1 = (vsr1 > 32)? vsr1 - 32:vsr1;
         op1 = (vmx_vsr1 & 0x1f) << (temp->op1_pos);
	}

	if(temp->op2_dtype != QGPR && temp->op2_dtype != VECTOR_SP && temp->op2_dtype != DUMMY)
	{
	    switch(temp->op2_dtype)
	    {
		case IMM_DATA_1BIT:
		     op2 = (random_no & 0x1) << (temp->op2_pos);
		     break;
		case IMM_DATA_2BIT:
		     op2 = (random_no & 0x3) << (temp->op2_pos);
		     break;
		case IMM_DATA_3BIT:
		     op2 = (random_no & 0x7) << (temp->op2_pos);
		    break;
		case IMM_DATA_4BIT:
		    op2 = (random_no & 0xf) << (temp->op2_pos);
		    break;
		default:
		    op2 = (random_no & 0x1f) << (temp->op2_pos);
		    break;
	    }
	}
	else
	{
		vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->op2_dtype]);
    	vsr2 = vsrs->head[VMX]->vsr_no;
    	/*
     	* Move this VSR to the end of link list
     	*/
    	MOVE_VSR_TO_END(client_no, temp->op2_dtype, VMX);
		vsrs->dirty_mask &= (~(0x1ULL << vsr2));
		vmx_vsr2 = (vsr2 > 32)?vsr2 - 32: vsr2;
    	op2 = (vmx_vsr2 & 0x1f) << (temp->op2_pos);
	}

	if(temp->ins_class != CLASS_VMX_NORMAL_3INPUTS)
	{
	   	vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->tgt_dtype]);
		if (temp->tgt_dtype == GR) {
			vsr4 = vsrs->head[BFP]->vsr_no;
			tgt = (vsr4 & 0x1f) << (temp->tgt_pos);
		} else {
			vsr4 = vsrs->head[VMX]->vsr_no;
        	vmx_vsr4 =  (vsr4 > 32)?vsr4 - 32: vsr4;
			tgt = (vmx_vsr4 & 0x1f) << (temp->tgt_pos);
		}
		tgt_reg_no = vsr4;

		mcode = temp->op_eop_mask | op1 | op2 | tgt;
	}
	else
	{
		if(temp->op3_dtype != QGPR && temp->op3_dtype != VECTOR_SP && temp->op3_dtype != DUMMY)
		{
	    	switch(temp->op3_dtype)
	    	{
			case IMM_DATA_1BIT:
				op3 = (random_no & 0x1) << (temp->op3_pos);
				break;
			case IMM_DATA_2BIT:
				op3 = (random_no & 0x3) << (temp->op3_pos);
				break;
			case IMM_DATA_3BIT:
				op3 = (random_no & 0x7) << (temp->op3_pos);
				break;
			case IMM_DATA_4BIT:
				op3 = (random_no & 0xf) << (temp->op3_pos);
				break;
			default:
				op3 = (random_no & 0x1f) << (temp->op3_pos);
				break;
			}
		}
		else
		{
			vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->op3_dtype]);
			vsr3 = vsrs->head[VMX]->vsr_no;
			/*
			* Move this VSR to the end of link list if its not a target.
			*/
			MOVE_VSR_TO_END(client_no, temp->op3_dtype, VMX);
			vsrs->dirty_mask &= (~(0x1ULL << vsr3));
			vmx_vsr3 = (vsr3 > 32)?vsr3 - 32:vsr3;
			op3 = (vmx_vsr3 & 0x1f) << (temp->op3_pos);
		}

   	   	vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->tgt_dtype]);
		vsr4 = vsrs->head[VMX]->vsr_no;
        vmx_vsr4 =  (vsr4 > 32)?vsr4 - 32: vsr4;
		tgt = (vmx_vsr4 & 0x1f) << (temp->tgt_pos);
		tgt_reg_no = vsr4;

		mcode = temp->op_eop_mask | op1 | op2 | op3 | tgt;
	}
    /*
     * Check if the target is dirty. If target is dirty build store instruction to save VSR
     * to testcase memory buffer. Check alignment requirement for store instruction.
     */

    if ((0x1ULL << tgt_reg_no) & vsrs->dirty_mask) {
    	/*
     	 * Generate store. To generate store, tgt_reg_no's data type should be known.
     	 */
		store_off = init_mem_for_vsx_store(client_no, QGPR);
        addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
        *tc_memory = addi_mcode;
        cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
        tc_memory++;
        num_ins_built++;
        *tc_memory = STORE_VMX_128(vmx_vsr4);
        /* save offset */
        cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
        cptr->instr_index[prolog_size + num_ins_built] = vmx_stvx | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stvx;
        tc_memory++;
        num_ins_built++;
	}

    *tc_memory = mcode;
    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
    tc_memory++;
    num_ins_built++;

    /* Update masks for tgt vsr */
    vsrs->dirty_mask |= (0x1ULL << vsr4);
    /* Restore the number of instruction built */
    cptr->num_ins_built = num_ins_built;
}

