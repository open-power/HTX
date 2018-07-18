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


#include "framework.h"

/*
 * Array of structures of type instruction_masks. Each element in array
 * describes one instruction.
 */
struct instruction_masks vsx_instructions_array[] = {
/* VSX Scalar Load instructions */
/* Following VSX update_form storage instructions are disabled as its support is withdrawn in PowerISA 2.06.
 * Load instructions : lxsdux, lxvw4ux, lxvd2ux
 * Store instructions: stxsdux, stxvw4ux, stxvd2ux
 */

/* lxsdux 		{0x7C0004D8, 0,GRU, 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x1,  "lxsdux",  (sim_fptr)&simulate_lxsdux, VSX_SCALAR_DP_LOAD_ONLY, FORM_XX1_XT_RA_RB}, */
/* lxsdx  */	{0x7C000498, 0,GR , 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x1,  "lxsdx",   (sim_fptr)&simulate_lxsdx, VSX_SCALAR_DP_LOAD_ONLY, FORM_XX1_XT_RA_RB},

/* VSX  Vector Load instructions */
/* lxvd2ux 		{0x7C0006D8, 0,GRU, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1,  "lxvd2ux", (sim_fptr)&simulate_lxvd2ux, VSX_VECTOR_DP_LOAD_ONLY, FORM_XX1_XT_RA_RB}, */
/* lxvd2x */	{0x7C000698, 0,GR , 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1,  "lxvd2x",  (sim_fptr)&simulate_lxvd2x, VSX_VECTOR_DP_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* lxvw4ux		{0x7C000658, 0,GRU, 16, GR, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x1,  "lxvw4ux", (sim_fptr)&simulate_lxvw4ux, VSX_VECTOR_SP_LOAD_ONLY, FORM_XX1_XT_RA_RB}, */
/* lxvw4x */	{0x7C000618, 0,GR , 16, GR, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x1,  "lxvw4x",  (sim_fptr)&simulate_lxvw4x, VSX_VECTOR_SP_LOAD_ONLY, FORM_XX1_XT_RA_RB},

/* VSX  Vector Load and Splat Instruction */
/* lxvdsx */	{0x7C000298, 0,GR , 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1,  "lxvdsx",  (sim_fptr)&simulate_lxvdsx, VSX_VECTOR_DP_LOAD_ONLY, FORM_XX1_XT_RA_RB},

/* VSX  Scalar Store instructions */
/* stxsdux 		{0x7C0005D8, 0,GRU, 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x2,  "stxsdux",  (sim_fptr)&simulate_stxsdux,VSX_SCALAR_DP_STORE_ONLY, FORM_XX1_XT_RA_RB}, */
/* stxsdx */	{0x7C000598, 0,GR , 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x2,  "stxsdx",  (sim_fptr)&simulate_stxsdx, VSX_SCALAR_DP_STORE_ONLY, FORM_XX1_XT_RA_RB},

/* VSX  Vector Store instructions */
/* stxvd2ux 	{0x7C0007D8, 0,GRU, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x2,  "stxvd2ux", (sim_fptr)&simulate_stxvd2ux, VSX_VECTOR_DP_STORE_ONLY, FORM_XX1_XT_RA_RB}, */
/* stxvd2x */	{0x7C000798, 0,GR , 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x2,  "stxvd2x",  (sim_fptr)&simulate_stxvd2x, VSX_VECTOR_DP_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* stxvw4ux 	{0x7C000758, 0,GRU, 16, GR, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x2,  "stxvw4ux", (sim_fptr)&simulate_stxvw4ux, VSX_VECTOR_SP_STORE_ONLY, FORM_XX1_XT_RA_RB}, */
/* stxvw4x */	{0x7C000718, 0,GR , 16, GR, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x2,  "stxvw4x",  (sim_fptr)&simulate_stxvw4x, VSX_VECTOR_SP_STORE_ONLY, FORM_XX1_XT_RA_RB},

/* VSX  Scalar Move instructions */
/* xsabsdp */	{0xF0000564, 0,DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xsabsdp",  (sim_fptr)&simulate_xsabsdp, VSX_SCALAR_DP_MOVE_ONLY, FORM_XX2_XT_XB},
/* xsnabsdp */	{0xF00005A4, 0,DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xsnabsdp", (sim_fptr)&simulate_xsnabsdp, VSX_SCALAR_DP_MOVE_ONLY, FORM_XX2_XT_XB},
/* xsnegdp */	{0xF00005E4, 0,DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xsnegdp",  (sim_fptr)&simulate_xsnegdp, VSX_SCALAR_DP_MOVE_ONLY, FORM_XX2_XT_XB},


/* VSX  Vector Move instructions */
/* xvabsdp */	{0xF0000764, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvabsdp",  (sim_fptr)&simulate_xvabsdp, VSX_VECTOR_DP_MOVE_ONLY, FORM_XX2_XT_XB},
/* xvabssp */	{0xF0000664, 0,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvabssp",  (sim_fptr)&simulate_xvabssp, VSX_VECTOR_SP_MOVE_ONLY, FORM_XX2_XT_XB},
/* xvnabsdp */	{0xF00007A4, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvnabsdp", (sim_fptr)&simulate_xvnabsdp, VSX_VECTOR_DP_MOVE_ONLY, FORM_XX2_XT_XB},
/* xvnabssp */	{0xF00006A4, 0,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvnabssp", (sim_fptr)&simulate_xvnabssp, VSX_VECTOR_SP_MOVE_ONLY, FORM_XX2_XT_XB},
/* xvnegdp */	{0xF00007E4, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvnegdp",  (sim_fptr)&simulate_xvnegdp, VSX_VECTOR_DP_MOVE_ONLY, FORM_XX2_XT_XB},
/* xvnegsp */	{0xF00006E4, 0,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvnegsp",  (sim_fptr)&simulate_xvnegsp, VSX_VECTOR_SP_MOVE_ONLY, FORM_XX2_XT_XB},

/* VSX  Scalar Floating-Point Elementary Arithmetic instructions */
/* xsadddp */   {0xF0000100, F_XSASDP, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0, "xsadddp",  (sim_fptr)&simulate_xsadddp, VSX_SCALAR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xsdivdp */	{0xF00001C0, F_XSDIVDP, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0, "xsdivdp", (sim_fptr)&simulate_xsdivdp, VSX_SCALAR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xsmuldp */	{0xF0000180, F_XSMULDP, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0, "xsmuldp", (sim_fptr)&simulate_xsmuldp, VSX_SCALAR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xsredp */ 	{0xF0000168, 0, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP,  21, 0x0,  "xsredp",   (sim_fptr)&simulate_xsredp, (VSX_SCALAR_DP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX2_XT_XB}, 
/* xsrsqrtedp */{0xF0000128, 0, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP,  21, 0x0,  "xsrsqrtedp",  (sim_fptr)&simulate_xsrsqrtedp, (VSX_SCALAR_DP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX2_XT_XB},
/* xssqrtdp */	{0xF000012C, F_XSSQRTDP, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0, "xssqrtdp", (sim_fptr)&simulate_xssqrtdp, VSX_SCALAR_DP_FP_ARITHMETIC_ONLY, FORM_XX2_XT_XB},
/* xssubdp */	{0xF0000140, F_XSASDP, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0, "xssubdp", (sim_fptr)&simulate_xssubdp, VSX_SCALAR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xstdivdp */	{0xF00001E8,0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, CR_T	 , 23, 0x4,  "xstdivdp", (sim_fptr)&simulate_xstdivdp, VSX_SCALAR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_BF_XA_XB},
/* xstsqrtdp */	{0xF00001A8,0,DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY,  CR_T	 , 23, 0x4,  "xstsqrtdp",(sim_fptr)&simulate_xstsqrtdp, VSX_SCALAR_DP_FP_ARITHMETIC_ONLY, FORM_XX2_BF_XB},

/* VSX  Vector Floating-Point Elementary ARITHMETIC instructions */
/* xvadddp */	{0xF0000300, F_XVADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0, "xvadddp", (sim_fptr)&simulate_xvadddp, VSX_VECTOR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xvaddsp */	{0xF0000200, F_XVADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0, "xvaddsp", (sim_fptr)&simulate_xvaddsp, VSX_VECTOR_SP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xvdivdp */	{0xF00003C0, F_XVDIV,VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvdivdp",  (sim_fptr)&simulate_xvdivdp, VSX_VECTOR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xvdivsp */	{0xF00002C0, F_XVDIV,VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvdivsp",  (sim_fptr)&simulate_xvdivsp, VSX_VECTOR_SP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xvmuldp */	{0xF0000380, F_XVMUL,VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvmuldp",  (sim_fptr)&simulate_xvmuldp, VSX_VECTOR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xvmulsp */	{0xF0000280, F_XVMUL,VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvmulsp",  (sim_fptr)&simulate_xvmulsp, VSX_VECTOR_SP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xvredp */	{0xF0000368, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP,  21, 0x0,  "xvredp",  (sim_fptr)&simulate_xvredp,  (VSX_VECTOR_DP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX2_XT_XB}, 
/* xvrsqrtedp */{0xF0000328, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP,  21, 0x0,  "xvrsqrtedp",(sim_fptr)&simulate_xvrsqrtedp,(VSX_VECTOR_DP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX2_XT_XB},
/* xvsqrtdp */	{0xF000032C, F_XVSQRT,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP,  21, 0x0,  "xvsqrtdp",  (sim_fptr)&simulate_xvsqrtdp,VSX_VECTOR_DP_FP_ARITHMETIC_ONLY, FORM_XX2_XT_XB},
/* xvsqrtsp */	{0xF000022C, F_XVSQRT,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP,  21, 0x0,  "xvsqrtsp",  (sim_fptr)&simulate_xvsqrtsp,VSX_VECTOR_SP_FP_ARITHMETIC_ONLY, FORM_XX2_XT_XB},
/* xvsubdp */	{0xF0000340, F_XVADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvsubdp",  (sim_fptr)&simulate_xvsubdp, VSX_VECTOR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xvsubsp */	{0xF0000240, F_XVADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvsubsp",  (sim_fptr)&simulate_xvsubsp, VSX_VECTOR_SP_FP_ARITHMETIC_ONLY, FORM_XX3_XT_XA_XB},
/* xvtdivdp */	{0xF00003E8, 0,VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, CR_T	   , 23, 0x4,  "xvtdivdp",(sim_fptr)&simulate_xvtdivdp,VSX_VECTOR_DP_FP_ARITHMETIC_ONLY, FORM_XX3_BF_XA_XB},
/* xvtdivsp */	{0xF00002E8, 0,VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, CR_T	,    23, 0x4,  "xvtdivsp",(sim_fptr)&simulate_xvtdivsp,VSX_VECTOR_SP_FP_ARITHMETIC_ONLY, FORM_XX3_BF_XA_XB},
/* xvtsqrtdp */	{0xF00003A8, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, CR_T	  ,  23, 0x4,  "xvtsqrtdp",  (sim_fptr)&simulate_xvtsqrtdp, VSX_VECTOR_DP_FP_ARITHMETIC_ONLY, FORM_XX2_BF_XB},
/* xvtsqrtsp */	{0xF00002A8, 0,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, CR_T	  ,  23, 0x4,  "xvtsqrtsp",  (sim_fptr)&simulate_xvtsqrtsp, VSX_VECTOR_SP_FP_ARITHMETIC_ONLY, FORM_XX2_BF_XB},

/* VSX  Scalar Floating-Point Multiply-Add Arithmetic Instructions */
/* xsmaddadp */	{0xF0000108, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_DP, 21, 0x3, "xsmaddadp", (sim_fptr)&simulate_xsmaddadp, VSX_SCALAR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xsmaddmdp  */{0xF0000148, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_DP, 21, 0x3,  "xsmaddmdp",  (sim_fptr)&simulate_xsmaddmdp, VSX_SCALAR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xsmsubadp  */{0xF0000188, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_DP, 21, 0x3,  "xsmsubadp",  (sim_fptr)&simulate_xsmsubadp, VSX_SCALAR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xsmsubmdp  */{0xF00001C8, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_DP, 21, 0x3,  "xsmsubmdp",  (sim_fptr)&simulate_xsmsubmdp, VSX_SCALAR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xsnmaddadp */{0xF0000508, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_DP, 21, 0x3,  "xsnmaddadp", (sim_fptr)&simulate_xsnmaddadp, VSX_SCALAR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xsnmaddmdp */{0xF0000548, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_DP, 21, 0x3,  "xsnmaddmdp", (sim_fptr)&simulate_xsnmaddmdp, VSX_SCALAR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xsnmsubadp */{0xF0000588, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_DP, 21, 0x3,  "xsnmsubadp", (sim_fptr)&simulate_xsnmsubadp, VSX_SCALAR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xsnmsubmdp */{0xF00005C8, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_DP, 21, 0x3,  "xsnmsubmdp", (sim_fptr)&simulate_xsnmsubmdp, VSX_SCALAR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},

/* VSX  Vector Floating-Point Multiply-Add Arithmetic Instructions */
/* xvmaddadp  */{0xF0000308, F_XVMADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, VECTOR_DP, 21, VECTOR_DP, 21, 0x3,  "xvmaddadp",  (sim_fptr)&simulate_xvmaddadp, VSX_VECTOR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvmaddasp  */{0xF0000208, F_XVMADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 21, VECTOR_SP, 21, 0x3,  "xvmaddasp",  (sim_fptr)&simulate_xvmaddasp, VSX_VECTOR_SP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvmaddmdp  */{0xF0000348, F_XVMADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, VECTOR_DP, 21, VECTOR_DP, 21, 0x3,  "xvmaddmdp",  (sim_fptr)&simulate_xvmaddmdp, VSX_VECTOR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvmaddmsp  */{0xF0000248, F_XVMADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 21, VECTOR_SP, 21, 0x3,  "xvmaddmsp",  (sim_fptr)&simulate_xvmaddmsp, VSX_VECTOR_SP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvmsubadp  */{0xF0000388, F_XVMADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, VECTOR_DP, 21, VECTOR_DP, 21, 0x3,  "xvmsubadp",  (sim_fptr)&simulate_xvmsubadp, VSX_VECTOR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvmsubasp  */{0xF0000288, F_XVMADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 21, VECTOR_SP, 21, 0x3,  "xvmsubasp",  (sim_fptr)&simulate_xvmsubasp, VSX_VECTOR_SP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvmsubmdp  */{0xF00003C8, F_XVMADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, VECTOR_DP, 21, VECTOR_DP, 21, 0x3,  "xvmsubmdp",  (sim_fptr)&simulate_xvmsubmdp, VSX_VECTOR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvmsubmsp  */{0xF00002C8, F_XVMADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 21, VECTOR_SP, 21, 0x3,  "xvmsubmsp",  (sim_fptr)&simulate_xvmsubmsp, VSX_VECTOR_SP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvnmaddadp */{0xF0000708, F_XVMADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, VECTOR_DP, 21, VECTOR_DP, 21, 0x3,  "xvnmaddadp", (sim_fptr)&simulate_xvnmaddadp, VSX_VECTOR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvnmaddasp */{0xF0000608, F_XVMADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 21, VECTOR_SP, 21, 0x3,  "xvnmaddasp", (sim_fptr)&simulate_xvnmaddasp, VSX_VECTOR_SP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvnmaddmdp */{0xF0000748, F_XVMADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, VECTOR_DP, 21, VECTOR_DP, 21, 0x3,  "xvnmaddmdp", (sim_fptr)&simulate_xvnmaddmdp, VSX_VECTOR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvnmaddmsp */{0xF0000648, F_XVMADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 21, VECTOR_SP, 21, 0x3,  "xvnmaddmsp", (sim_fptr)&simulate_xvnmaddmsp, VSX_VECTOR_SP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvnmsubadp */{0xF0000788, F_XVMADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, VECTOR_DP, 21, VECTOR_DP, 21, 0x3,  "xvnmsubadp", (sim_fptr)&simulate_xvnmsubadp, VSX_VECTOR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvnmsubasp */{0xF0000688, F_XVMADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 21, VECTOR_SP, 21, 0x3,  "xvnmsubasp", (sim_fptr)&simulate_xvnmsubasp, VSX_VECTOR_SP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvnmsubmdp */{0xF00007C8, F_XVMADDSUB,VECTOR_DP, 16, VECTOR_DP, 11, VECTOR_DP, 21, VECTOR_DP, 21, 0x3,  "xvnmsubmdp", (sim_fptr)&simulate_xvnmsubmdp, VSX_VECTOR_DP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},
/* xvnmsubmsp */{0xF00006C8, F_XVMADDSUB,VECTOR_SP, 16, VECTOR_SP, 11, VECTOR_SP, 21, VECTOR_SP, 21, 0x3,  "xvnmsubmsp", (sim_fptr)&simulate_xvnmsubmsp, VSX_VECTOR_SP_FP_MUL_ADD_ONLY, FORM_XX3_XT_XA_XB},

/* VSX  Scalar Floating-Point Comparison Instructions */
/* xscmpodp */	{0xF0000158, F_XSCMP,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, CR_T, 23, 0x4,  "xscmpodp", (sim_fptr)&simulate_xscmpodp, VSX_SCALAR_DP_FP_COMP_ONLY | HW_ONLY, FORM_XX3_BF_XA_XB},
/* Following is an old way of randomizing CR field in instruction coding. New way is to use CR_T.*/
#if 0

			{0xF0800158, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpodp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF1000158, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpodp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF1800158, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpodp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF2000158, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpodp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF2800158, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpodp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF3000158, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpodp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF3800158, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpodp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
#endif

/* xscmpudp */	{0xF0000118, F_XSCMP,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, CR_T, 23, 0x4,  "xscmpudp",  (sim_fptr)&simulate_xscmpudp, VSX_SCALAR_DP_FP_COMP_ONLY | HW_ONLY, FORM_XX3_BF_XA_XB},
/* Following is an old way of randomizing CR field in instruction coding. New way is to use CR_T.*/
#if 0
			{0xF0800118, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpudp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF1000118, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpudp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF1800118, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpudp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF2000118, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpudp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF2800118, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpudp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF3000118, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpudp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
			{0xF3800118, 0,SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, DUMMY, DUMMY, 0x0,  "xscmpudp",  NULL, VSX_SCALAR_DP_FP_COMP_ONLY, FORM_XX3_BF_XA_XB},
#endif

/* VSX  Vector Floating-Point Comparison Instructions */
/* xvcmpeqdp */	{0xF0000318, F_XCMPEQ, VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpeqdp",  (sim_fptr)&simulate_xvcmpeqdp, VSX_VECTOR_DP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpeqsp */	{0xF0000218, F_XCMPEQ, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpeqsp",  (sim_fptr)&simulate_xvcmpeqsp, VSX_VECTOR_SP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpgedp */	{0xF0000398, F_XCMPGE, VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpgedp",  (sim_fptr)&simulate_xvcmpgedp, VSX_VECTOR_DP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpgesp */	{0xF0000298, F_XCMPGE, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpgesp",  (sim_fptr)&simulate_xvcmpgesp, VSX_VECTOR_SP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpgtdp */	{0xF0000358, F_XCMPGE, VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpgtdp",  (sim_fptr)&simulate_xvcmpgtdp, VSX_VECTOR_DP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpgtsp */	{0xF0000258, F_XCMPGE, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpgtsp",  (sim_fptr)&simulate_xvcmpgtsp, VSX_VECTOR_SP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpeqdp _*/{0xF0000718, F_XCMPEQ, VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpeqdp.",(sim_fptr)&simulate_xvcmpeqdp,VSX_VECTOR_DP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpeqsp_ */{0xF0000618, F_XCMPEQ, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpeqsp.",(sim_fptr)&simulate_xvcmpeqsp,VSX_VECTOR_SP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpgedp_ */{0xF0000798, F_XCMPGE, VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpgedp.",(sim_fptr)&simulate_xvcmpgedp,VSX_VECTOR_DP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpgesp_ */{0xF0000698, F_XCMPGE, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpgesp.",(sim_fptr)&simulate_xvcmpgesp,VSX_VECTOR_SP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpgtdp_ */{0xF0000758, F_XCMPGE, VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpgtdp.",(sim_fptr)&simulate_xvcmpgtdp,VSX_VECTOR_DP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},
/* xvcmpgtsp_ */{0xF0000658, F_XCMPGE, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xvcmpgtsp.",(sim_fptr)&simulate_xvcmpgtsp,VSX_VECTOR_SP_FP_COMP_ONLY, FORM_XX3_RC_XT_XA_XB},

/* VSX  Scalar Floating-Point Maximum/Minimum Instructions */
/* xsmaxdp */	{0xF0000500, F_XCMPEQ, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xsmaxdp",  (sim_fptr)&simulate_xsmaxdp, VSX_SCALAR_DP_FP_MAX_MIN_ONLY, FORM_XX3_XT_XA_XB},
/* xsmindp */	{0xF0000540, F_XCMPEQ, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xsmindp",  (sim_fptr)&simulate_xsmindp, VSX_SCALAR_DP_FP_MAX_MIN_ONLY, FORM_XX3_XT_XA_XB},

/* VSX  Vector Floating-Point Maximum/Minimum Instructions */
/* xvmaxdp */	{0xF0000700, F_XCMPEQ, VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvmaxdp",  (sim_fptr)&simulate_xvmaxdp, VSX_VECTOR_DP_FP_MAX_MIN_ONLY, FORM_XX3_XT_XA_XB},
/* xvmaxsp */	{0xF0000600, F_XCMPEQ, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvmaxsp",  (sim_fptr)&simulate_xvmaxsp, VSX_VECTOR_SP_FP_MAX_MIN_ONLY, FORM_XX3_XT_XA_XB},
/* xvmindp */	{0xF0000740, F_XCMPEQ, VECTOR_DP, 16, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvmindp",  (sim_fptr)&simulate_xvmindp, VSX_VECTOR_DP_FP_MAX_MIN_ONLY, FORM_XX3_XT_XA_XB},
/* xvminsp */	{0xF0000640, F_XCMPEQ, VECTOR_SP, 16, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvminsp",  (sim_fptr)&simulate_xvminsp, VSX_VECTOR_SP_FP_MAX_MIN_ONLY, FORM_XX3_XT_XA_XB},

/* VSX  Scalar DP-SP Conversion Instructions */
/* xscvdpsp */	{0xF0000424, F_XSCVDPSP, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0,  "xscvdpsp",  (sim_fptr)&simulate_xscvdpsp, VSX_SCALAR_DP_CONV_SP_ONLY, FORM_XX2_XT_XB},
/* xscvspdp */	{0xF0000524, F_XSCVSPDP,DUMMY, DUMMY, SCALAR_SP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xscvspdp",  (sim_fptr)&simulate_xscvspdp, VSX_SCALAR_SP_CONV_DP_ONLY, FORM_XX2_XT_XB},

/* VSX  Vector DP-SP Conversion Instructions */
/* xvcvdpsp */	{0xF0000624, F_XVCVDPSP,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_SP_HH, 21, 0x0, "xvcvdpsp",  (sim_fptr)&simulate_xvcvdpsp, VSX_VECTOR_DP_CONV_SP_ONLY, FORM_XX2_XT_XB},
/* xvcvspdp */	{0xF0000724, F_XVCVSPDP,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,    "xvcvspdp",  (sim_fptr)&simulate_xvcvspdp, VSX_VECTOR_SP_CONV_DP_ONLY, FORM_XX2_XT_XB},

/* VSX  Scalar Floating-Point to Integer Conversion Instructions */
/* xscvdpsxds */{0xF0000560, F_XSCV, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, GPR_T,   21, 0x0,  "xscvdpsxds",  (sim_fptr)&simulate_xscvdpsxds, VSX_SCALAR_DP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xscvdpsxws */{0xF0000160, F_XSCV, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, GPR32, 21, 0x0,  "xscvdpsxws",    (sim_fptr)&simulate_xscvdpsxws, VSX_SCALAR_DP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xscvdpuxds */{0xF0000520, F_XSCV, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, GPR_T,   21, 0x0,  "xscvdpuxds",  (sim_fptr)&simulate_xscvdpuxds, VSX_SCALAR_DP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xscvdpuxws */{0xF0000120, F_XSCV, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, GPR32, 21, 0x0,  "xscvdpuxws",    (sim_fptr)&simulate_xscvdpuxws, VSX_SCALAR_DP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},

/* VSX  Vector Floating-Point to Integer Conversion Instructions */
/* xvcvdpsxds */{0xF0000760, F_XVCV,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, QGPR,    21, 0x0,  "xvcvdpsxds",  (sim_fptr)&simulate_xvcvdpsxds, VSX_VECTOR_DP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xvcvdpsxws */{0xF0000360, F_XVCV,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, QGPR_HH, 21, 0x0,  "xvcvdpsxws",  (sim_fptr)&simulate_xvcvdpsxws, VSX_VECTOR_DP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xvcvdpuxds */{0xF0000720, F_XVCV,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, QGPR,    21, 0x0,  "xvcvdpuxds",  (sim_fptr)&simulate_xvcvdpuxds, VSX_VECTOR_DP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xvcvdpuxws */{0xF0000320, F_XVCV,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, QGPR_HH, 21, 0x0,  "xvcvdpuxws",  (sim_fptr)&simulate_xvcvdpuxws, VSX_VECTOR_DP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xvcvspsxds */{0xF0000660, F_XVCV,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, QGPR,    21, 0x0,  "xvcvspsxds",  (sim_fptr)&simulate_xvcvspsxds, VSX_VECTOR_SP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xvcvspsxws */{0xF0000260, F_XVCV,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, QGPR,    21, 0x0,  "xvcvspsxws",  (sim_fptr)&simulate_xvcvspsxws, VSX_VECTOR_SP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xvcvspuxds */{0xF0000620, F_XVCV,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, QGPR,    21, 0x0,  "xvcvspuxds",  (sim_fptr)&simulate_xvcvspuxds, VSX_VECTOR_SP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},
/* xvcvspuxws */{0xF0000220, F_XVCV,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, QGPR,    21, 0x0,  "xvcvspuxws",  (sim_fptr)&simulate_xvcvspuxws, VSX_VECTOR_SP_CONV_FP2INT_ONLY, FORM_XX2_XT_XB},

/* VSX  Scalar Integer to Floating-Point Conversion Instructions */
/* xscvsxddp */	{0xF00005E0, F_XSCVI2F,DUMMY, DUMMY, GPR_T,   11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xscvsxddp",  (sim_fptr)&simulate_xscvsxddp, VSX_SCALAR_DP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},
/* xscvuxddp */	{0xF00005A0, F_XSCVI2F,DUMMY, DUMMY, GPR_T,   11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xscvuxddp",  (sim_fptr)&simulate_xscvuxddp, VSX_SCALAR_DP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},

/* VSX  Vector Integer to Floating-Point Conversion Instructions */
/* xvcvsxddp */	{0xF00007E0, F_XVCVI2F, DUMMY, DUMMY, QGPR,  11, DUMMY, DUMMY, VECTOR_DP,    21, 0x0,  "xvcvsxddp",  (sim_fptr)&simulate_xvcvsxddp,VSX_VECTOR_DP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},
/* xvcvsxdsp */	{0xF00006E0, F_XVCVI2F, DUMMY, DUMMY, QGPR,  11, DUMMY, DUMMY, VECTOR_SP_HH, 21, 0x0,  "xvcvsxdsp",  (sim_fptr)&simulate_xvcvsxdsp,VSX_VECTOR_SP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},
/* xvcvsxwdp */	{0xF00003E0, F_XVCVI2F, DUMMY, DUMMY, QGPR,  11, DUMMY, DUMMY, VECTOR_DP,    21, 0x0,  "xvcvsxwdp",  (sim_fptr)&simulate_xvcvsxwdp,VSX_VECTOR_DP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},
/* xvcvsxwsp */	{0xF00002E0, F_XVCVI2F, DUMMY, DUMMY, QGPR,  11, DUMMY, DUMMY, VECTOR_SP,    21, 0x0,  "xvcvsxwsp",  (sim_fptr)&simulate_xvcvsxwsp,VSX_VECTOR_SP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},
/* xvcvuxddp */	{0xF00007A0, F_XVCVI2F, DUMMY, DUMMY, QGPR,  11, DUMMY, DUMMY, VECTOR_DP,    21, 0x0,  "xvcvuxddp",  (sim_fptr)&simulate_xvcvuxddp,VSX_VECTOR_DP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},
/* xvcvuxdsp */	{0xF00006A0, F_XVCVI2F, DUMMY, DUMMY, QGPR,  11, DUMMY, DUMMY, VECTOR_SP_HH, 21, 0x0,  "xvcvuxdsp",  (sim_fptr)&simulate_xvcvuxdsp,VSX_VECTOR_SP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},
/* xvcvuxwdp */	{0xF00003A0, F_XVCVI2F, DUMMY, DUMMY, QGPR,  11, DUMMY, DUMMY, VECTOR_DP,    21, 0x0,  "xvcvuxwdp",  (sim_fptr)&simulate_xvcvuxwdp,VSX_VECTOR_DP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},
/* xvcvuxwsp */	{0xF00002A0, F_XVCVI2F, DUMMY, DUMMY, QGPR,  11, DUMMY, DUMMY, VECTOR_SP,    21, 0x0,  "xvcvuxwsp",  (sim_fptr)&simulate_xvcvuxwsp,VSX_VECTOR_SP_CONV_INT2FP_ONLY, FORM_XX2_XT_XB},

/* VSX  Scalar Round to Floating-Point Integer Instructions */
/* xsrdpi  */	{0xF0000124, F_XSRD, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xsrdpi",  (sim_fptr)&simulate_xsrdpi, VSX_SCALAR_DP_RND_2_FPINT_ONLY,  FORM_XX2_XT_XB},
/* xsrdpic */	{0xF00001AC, F_XSRDPIC, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0, "xsrdpic",  (sim_fptr)&simulate_xsrdpic, VSX_SCALAR_DP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xsrdpim */	{0xF00001E4, F_XSRD, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xsrdpim",  (sim_fptr)&simulate_xsrdpim, VSX_SCALAR_DP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xsrdpip */	{0xF00001A4, F_XSRD, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xsrdpip",  (sim_fptr)&simulate_xsrdpip, VSX_SCALAR_DP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xsrdpiz */	{0xF0000164, F_XSRD, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0,  "xsrdpiz",  (sim_fptr)&simulate_xsrdpiz, VSX_SCALAR_DP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},

/* VSX  Vector Round to Floating-Point Integer Instructions */
/* xvrdpi  */	{0xF0000324, F_XVRDPI,  DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvrdpi",  (sim_fptr)&simulate_xvrdpi, VSX_VECTOR_DP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xvrdpic */	{0xF00003AC, F_XVRDPIC, DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvrdpic", (sim_fptr)&simulate_xvrdpic, VSX_VECTOR_DP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xvrdpim */	{0xF00003E4, F_XVRDPI,  DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvrdpim", (sim_fptr)&simulate_xvrdpim, VSX_VECTOR_DP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xvrdpip */	{0xF00003A4, F_XVRDPI,  DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvrdpip", (sim_fptr)&simulate_xvrdpip, VSX_VECTOR_DP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xvrdpiz */	{0xF0000364, F_XVRDPI,  DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvrdpiz", (sim_fptr)&simulate_xvrdpiz, VSX_VECTOR_DP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xvrspi  */	{0xF0000224, F_XVRDPI,  DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvrspi",  (sim_fptr)&simulate_xvrspi, VSX_VECTOR_SP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xvrspic */	{0xF00002AC, F_XVRDPIC, DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvrspic", (sim_fptr)&simulate_xvrspic, VSX_VECTOR_SP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xvrspim */	{0xF00002E4, F_XVRDPI,  DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvrspim", (sim_fptr)&simulate_xvrspim, VSX_VECTOR_SP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xvrspip */	{0xF00002A4, F_XVRDPI,  DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvrspip", (sim_fptr)&simulate_xvrspip, VSX_VECTOR_SP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},
/* xvrspiz */	{0xF0000264, F_XVRDPI,  DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvrspiz", (sim_fptr)&simulate_xvrspiz, VSX_VECTOR_SP_RND_2_FPINT_ONLY, FORM_XX2_XT_XB},

/* VSX  Logical Instructions */
/* xxland	*/	{0xF0000410, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0, "xxland",  (sim_fptr)&simulate_xxland,	VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},
/* xxlandc	*/	{0xF0000450, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0, "xxlandc", (sim_fptr)&simulate_xxlandc,VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},
/* xxlnor	*/	{0xF0000510, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0, "xxlnor",  (sim_fptr)&simulate_xxlnor,	VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},
/* xxlor	*/ 	{0xF0000490, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0, "xxlor",   (sim_fptr)&simulate_xxlor,	VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},
/* xxlxor	*/	{0xF00004D0, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0, "xxlxor",  (sim_fptr)&simulate_xxlxor,	VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},

/* VSX  Merge Instructions */
/* xxmrghw */	{0xF0000090, 0,QGPR, 16, QGPR, 11,DUMMY, DUMMY, QGPR, 21, 0x0,  "xxmrghw",  (sim_fptr)&simulate_xxmrghw, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB },
/* xxmrglw */	{0xF0000190, 0,QGPR, 16, QGPR, 11,DUMMY, DUMMY, QGPR, 21, 0x0,  "xxmrglw",  (sim_fptr)&simulate_xxmrglw, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},

/* VSX  Splat Instruction */
/* xxspltw */	{0xF0000290, 0,IMM_DATA_2BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x8,  "xxspltw",  (sim_fptr)&simulate_xxspltw, VSX_MISC_ONLY, FORM_XX2_XT_UIM_XB},
				{0xF0010290, 0,IMM_DATA_2BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x8,  "xxspltw",  (sim_fptr)&simulate_xxspltw, VSX_MISC_ONLY, FORM_XX2_XT_UIM_XB},
				{0xF0020290, 0,IMM_DATA_2BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x8,  "xxspltw",  (sim_fptr)&simulate_xxspltw, VSX_MISC_ONLY, FORM_XX2_XT_UIM_XB},
				{0xF0030290, 0,IMM_DATA_2BIT, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x8,  "xxspltw",  (sim_fptr)&simulate_xxspltw, VSX_MISC_ONLY, FORM_XX2_XT_UIM_XB},

/* VSX  Permute Instruction */
/* xxpermdi,DM=00 */	 {0xF0000050, 0,QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxpermdi",  (sim_fptr)&simulate_xxpermdi, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB_2Bit},
/* xxpermdi,DM=01 */	 {0xF0000150, 0,QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxpermdi",  (sim_fptr)&simulate_xxpermdi, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB_2Bit},
/* xxpermdi,DM=10 */	 {0xF0000250, 0,QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxpermdi",  (sim_fptr)&simulate_xxpermdi, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB_2Bit},
/* xxpermdi,DM=11 */	 {0xF0000350, 0,QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxpermdi",  (sim_fptr)&simulate_xxpermdi, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB_2Bit},

/* VSX  Vector Select Instruction */
/* xxsel */		{0xF0000030, 0,QGPR, 16, QGPR, 11, QGPR,  6, QGPR, 21, 0x0,  "xxsel",  (sim_fptr)&simulate_xxsel, VSX_MISC_ONLY, FORM_XX4_XT_XA_XB_XC},

/* VSX  Shift Instruction */
/* xxsldwi,SHW=00 */	 {0xF0000010, 0,QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxsldwi SHW=00",  (sim_fptr)&simulate_xxsldwi, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB_2Bit},
/* xxsldwi,SHW=01 */	 {0xF0000110, 0,QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxsldwi SHW=01",  (sim_fptr)&simulate_xxsldwi, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB_2Bit},
/* xxsldwi,SHW=10 */	 {0xF0000210, 0,QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxsldwi SHW=10",  (sim_fptr)&simulate_xxsldwi, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB_2Bit},
/* xxsldwi,SHW=11 */	 {0xF0000310, 0,QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxsldwi SHW=11",  (sim_fptr)&simulate_xxsldwi, VSX_MISC_ONLY, FORM_XX3_XT_XA_XB_2Bit},
/* last ins indicator */  {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0,  DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction",  NULL,VSX_MISC_ONLY}
};

struct instruction_masks vsx_p8_instructions_array[] = {

/* instructions moved to P8 table, sp versions results as illegal instruction on P7 test machine: skyp03 */
/* xsaddsp */   {0xF0000000, F_XSASDP, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0, "xsaddsp",  (sim_fptr)&simulate_xsaddsp, (VSX_SCALAR_SP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsdivsp */	{0xF00000C0, F_XSDIVDP, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0, "xsdivsp", (sim_fptr)&simulate_xsdivsp, (VSX_SCALAR_SP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsmulsp */	{0xF0000080, F_XSMULDP, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0, "xsmulsp", (sim_fptr)&simulate_xsmulsp, (VSX_SCALAR_SP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsresp */ 	{0xF0000068, 0, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP,  21, 0x0,  "xsresp",   (sim_fptr)&simulate_xsresp, (VSX_SCALAR_SP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX2_XT_XB}, 
/* xsrsqrtesp */{0xF0000028, 0, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP,  21, 0x0,  "xsrsqrtesp",  (sim_fptr)&simulate_xsrsqrtesp, (VSX_SCALAR_SP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX2_XT_XB},
/* xssqrtsp */	{0xF000002C, F_XSSQRTDP, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0, "xssqrtsp", (sim_fptr)&simulate_xssqrtsp, (VSX_SCALAR_SP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX2_XT_XB},
/* xssubsp */	{0xF0000040, F_XSASDP, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0, "xssubsp", (sim_fptr)&simulate_xssubsp, (VSX_SCALAR_SP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xvresp */	{0xF0000268, 0,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP,  21, 0x0,  "xvresp",  (sim_fptr)&simulate_xvresp,  (VSX_VECTOR_SP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX2_XT_XB},
/* xvrsqrtesp */{0xF0000228, 0,DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP,  21, 0x0,  "xvrsqrtesp",(sim_fptr)&simulate_xvrsqrtesp,(VSX_VECTOR_SP_FP_ARITHMETIC_ONLY | HW_ONLY), FORM_XX2_XT_XB},
/* xsmaddasp */	{0xF0000008, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_SP, 21, 0x3, "xsmaddasp", (sim_fptr)&simulate_xsmaddasp, (VSX_SCALAR_DP_FP_MUL_ADD_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsmaddmsp  */{0xF0000048, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_SP, 21, 0x3,  "xsmaddmsp",  (sim_fptr)&simulate_xsmaddmsp, (VSX_SCALAR_DP_FP_MUL_ADD_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsmsubasp  */{0xF0000088, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_SP, 21, 0x3,  "xsmsubasp",  (sim_fptr)&simulate_xsmsubasp, (VSX_SCALAR_DP_FP_MUL_ADD_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsmsubmsp  */{0xF00000C8, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_SP, 21, 0x3,  "xsmsubmsp",  (sim_fptr)&simulate_xsmsubmsp, (VSX_SCALAR_DP_FP_MUL_ADD_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsnmaddasp */{0xF0000408, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_SP, 21, 0x3,  "xsnmaddasp", (sim_fptr)&simulate_xsnmaddasp, (VSX_SCALAR_DP_FP_MUL_ADD_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsnmaddmsp */{0xF0000448, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_SP, 21, 0x3,  "xsnmaddmsp", (sim_fptr)&simulate_xsnmaddmsp, (VSX_SCALAR_DP_FP_MUL_ADD_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsnmsubasp */{0xF0000488, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_SP, 21, 0x3,  "xsnmsubasp", (sim_fptr)&simulate_xsnmsubasp, (VSX_SCALAR_DP_FP_MUL_ADD_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xsnmsubmsp */{0xF00004C8, F_XSMADDSUB,SCALAR_DP, 16, SCALAR_DP, 11, SCALAR_DP, 21, SCALAR_SP, 21, 0x3,  "xsnmsubmsp", (sim_fptr)&simulate_xsnmsubmsp, (VSX_SCALAR_DP_FP_MUL_ADD_ONLY | HW_ONLY), FORM_XX3_XT_XA_XB},
/* xscvsxdsp */	{0xF00004E0, F_XSCVI2F,DUMMY, DUMMY, GPR_T,   11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0,  "xscvsxdsp",  (sim_fptr)&simulate_xscvsxdsp, (VSX_SCALAR_SP_CONV_INT2FP_ONLY| HW_ONLY), FORM_XX2_XT_XB},
/* xscvuxdsp */	{0xF00004A0, F_XSCVI2F,DUMMY, DUMMY, GPR_T,   11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0,  "xscvuxdsp",  (sim_fptr)&simulate_xscvuxdsp, (VSX_SCALAR_SP_CONV_INT2FP_ONLY | HW_ONLY), FORM_XX2_XT_XB},
/* xsrsp  */	{0xF0000464, F_XSRD, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0,  "xsrsp",  (sim_fptr)&simulate_xsrsp, (VSX_SCALAR_DP_RND_2_FPINT_ONLY | HW_ONLY), FORM_XX2_XT_XB},

/* All VSX p8 specific instructions */

/* Vector Move Instructions */
/* mfvsrd*/   {0x7C000066, 0, SCALAR_DP, 21 ,DUMMY ,DUMMY , DUMMY, DUMMY, GR, 16  , 0x7, "mfvsrd",  (sim_fptr)&simulate_mfvsrd, VSX_SCALAR_DP_MOVE_ONLY, FORM_XX1_RA_XS},
/* mfvsrwz*/  {0x7C0000E6, 0, SCALAR_DP, 21 ,DUMMY ,DUMMY , DUMMY, DUMMY, GR, 16  , 0x7, "mfvsrwz", (sim_fptr)&simulate_mfvsrwz, VSX_SCALAR_DP_MOVE_ONLY, FORM_XX1_RA_XS},
/* mtvsrd*/   {0x7C000166, 0, GR, 16, DUMMY ,DUMMY , DUMMY, DUMMY, SCALAR_DP, 21  , 0x7, "mtvsrd",  (sim_fptr)&simulate_mtvsrd, VSX_SCALAR_DP_MOVE_ONLY, FORM_XX1_XT_RA},
/* mtvsrwa*/  {0x7C0001A6, 0, GR, 16, DUMMY ,DUMMY , DUMMY, DUMMY, SCALAR_DP, 21  , 0x7, "mtvsrwa", (sim_fptr)&simulate_mtvsrwa, VSX_SCALAR_DP_MOVE_ONLY, FORM_XX1_XT_RA},
/* mtvsrwz*/  {0x7C0001E6, 0, GR, 16, DUMMY ,DUMMY , DUMMY, DUMMY, SCALAR_DP, 21  , 0x7, "mtvsrwz", (sim_fptr)&simulate_mtvsrwz, VSX_SCALAR_DP_MOVE_ONLY, FORM_XX1_XT_RA},

/* VSX Scalar Load Instructions */
/* lxsiwax*/	{0x7C000098, 0,GR , 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x1,  "lxsiwax",   (sim_fptr)&simulate_lxsiwax, VSX_SCALAR_SP_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* lxsiwzx*/	{0x7C000018, 0,GR , 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x1,  "lxsiwzx",   (sim_fptr)&simulate_lxsiwzx, VSX_SCALAR_SP_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* lxsspx*/		{0x7C000418, 0,GR , 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x1,  "lxsspx",   (sim_fptr)&simulate_lxsspx, VSX_SCALAR_SP_LOAD_ONLY, FORM_XX1_XT_RA_RB},

/* VSX Scalar Store Instructions */
/* stxsiwx */	{0x7C000118, 0,GR , 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x2,  "stxsiwx",  (sim_fptr)&simulate_stxsiwx, VSX_SCALAR_SP_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* stxsspx */	{0x7C000518, 0,GR , 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x2,  "stxsspx",  (sim_fptr)&simulate_stxsspx, VSX_SCALAR_SP_STORE_ONLY, FORM_XX1_XT_RA_RB},

/* VSX  Scalar DP-SP Conversion Instructions - RFC02242*/
/* xscvdpspn */	{0xF000042C, 0, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_SP, 21, 0x0, "xscvdpspn", (sim_fptr)&simulate_xscvdpspn, VSX_SCALAR_DP_CONV_SP_ONLY, FORM_XX2_XT_XB},
/* xscvspdpn */	{0xF000052C, 0, DUMMY, DUMMY, SCALAR_SP, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x0, "xscvspdpn", (sim_fptr)&simulate_xscvspdpn, VSX_SCALAR_SP_CONV_DP_ONLY, FORM_XX2_XT_XB},

/* VSX  Logical Instructions - RFC02242 */
/* xxleqv	*/	{0xF00005D0, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0, "xxleqv",	(sim_fptr)&simulate_xxleqv,	VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},
/* xxlnand	*/	{0xF0000590, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0, "xxlnand",	(sim_fptr)&simulate_xxlnand,VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},
/* xxlorc	*/	{0xF0000550, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0, "xxlorc",	(sim_fptr)&simulate_xxlorc,	VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},

/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}

};


struct instruction_masks vsx_p9_instructions_array[] = {
/* RFC02462 - Decimal integer support operation */
/* lxvll	*/	{0x7C00025A, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1, "lxvll",	(sim_fptr)&simulate_lxvll, VSX_VECTOR_LENGTH_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* stxvll	*/	{0x7C00035A, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0xb, "stxvll",	(sim_fptr)&simulate_stxvll, VSX_VECTOR_LENGTH_STORE_ONLY, FORM_XX1_XT_RA_RB},

/* VSX half precision - RFC02468 */
/* xscvhpdp	*/	{0xF010056C, 0, DUMMY, DUMMY, SCALAR_HP, 11, DUMMY, DUMMY, SCALAR_HP, 21, 0x0, "xscvhpdp", (sim_fptr)&simulate_xscvhpdp, VSX_SCALAR_CONV_HP_DP_ONLY | HW_ONLY, FORM_XX2_XT_XB},
/* xscvdphp */	{0xF011056C, 0, DUMMY, DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, SCALAR_HP, 21, 0x0, "xscvdphp", (sim_fptr)&simulate_xscvdphp, VSX_SCALAR_CONV_HP_DP_ONLY | HW_ONLY, FORM_XX2_XT_XB},
/* xvcvhpsp	*/	{0xF018076C, (FPS_FX | FPS_VXSNAN), DUMMY, DUMMY, VECTOR_HP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0, "xvcvhpsp", (sim_fptr)&simulate_xvcvhpsp, VSX_VECTOR_CONV_HP_SP_ONLY | HW_ONLY, FORM_XX2_XT_XB},
/* xvcvsphp */	{0xF019076C, 0, DUMMY, DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_HP, 21, 0x0, "xvcvsphp", (sim_fptr)&simulate_xvcvsphp, VSX_VECTOR_CONV_HP_SP_ONLY | HW_ONLY, FORM_XX2_XT_XB},

/* 2496   - VSX Permute instructions */
/* xxperm */  {0xF00000D0, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxperm",  (sim_fptr)&simulate_xxperm, P9_VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},
/* xxpermr */ {0xF00001D0, 0, QGPR, 16, QGPR, 11, DUMMY, DUMMY, QGPR, 21, 0x0,  "xxpermr", (sim_fptr)&simulate_xxpermr, P9_VSX_MISC_ONLY, FORM_XX3_XT_XA_XB},

/* RFC02467.r10: String Operations (VSU Option) */ 
/************************************************/
/* lxvl	*/	{0x7C00021A, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1, "lxvl",	(sim_fptr)&simulate_lxvl, VSX_VECTOR_LENGTH_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* stxvl*/	{0x7C00031A, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x2, "stxvl",	(sim_fptr)&simulate_stxvl, VSX_VECTOR_LENGTH_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* RFC02470 128-bit SIMD FXU Operations */
/************************************************/
/* lxsibzx */{0x7C00061A, 0, GR, 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x1, "lxsibzx",(sim_fptr)&simulate_lxsibzx, VSX_VECTOR_LENGTH_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* lxsihzx */{0x7C00065A, 0, GR, 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x1, "lxsihzx",(sim_fptr)&simulate_lxsihzx, VSX_VECTOR_LENGTH_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* stxsibx */{0x7C00071A, 0, GR, 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x2, "stxsibx",(sim_fptr)&simulate_stxsibx, VSX_VECTOR_LENGTH_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* stxsihx */{0x7C00075A, 0, GR, 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x2, "stxsihx",(sim_fptr)&simulate_stxsihx, VSX_VECTOR_LENGTH_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* RFC02471.r13: 128-bit SIMD Miscellaneous Operations */ 
/*******************************************************/
/* lxvb16x 	*/{0x7C0006D8, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1, "lxvb16x",(sim_fptr)&simulate_lxvb16x, VSX_VECTOR_LENGTH_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* lxvh8x  	*/{0x7C000658, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1, "lxvh8x",(sim_fptr)&simulate_lxvh8x, VSX_VECTOR_LENGTH_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* lxvx 	*/{0x7C000218, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1, "lxvx",(sim_fptr)&simulate_lxvx, VSX_VECTOR_LENGTH_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* lxvx {0x7C000258, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1, "lxvx",(sim_fptr)&simulate_lxvx, VSX_VECTOR_LENGTH_LOAD_ONLY, FORM_XX1_XT_RA_RB},*/
/* lxvwsx */{0x7C0002D8, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x1, "lxvwsx",(sim_fptr)&simulate_lxvwsx, VSX_VECTOR_LENGTH_LOAD_ONLY, FORM_XX1_XT_RA_RB},
/* mfvsrld*/{0x7C000266, 0, VECTOR_DP, 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x7, "mfvsrld",(sim_fptr)&simulate_mfvsrld, P9_VSX_SCALAR_DP_MOVE_ONLY, FORM_XX1_RA_XS},
/* mtvsrdd */{0x7C000366, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x7, "mtvsrdd",(sim_fptr)&simulate_mtvsrdd, P9_VSX_SCALAR_DP_MOVE_ONLY, FORM_XX1_XT_RA_RB},
/* mtvsrws*/{0x7C000326, 0, GR, 16, DUMMY, DUMMY, DUMMY, DUMMY, VECTOR_DP, 21, 0x7, "mtvsrws",(sim_fptr)&simulate_mtvsrws, P9_VSX_SCALAR_DP_MOVE_ONLY, FORM_XX1_XT_RA},
/* stxvb16x*/{0x7C0007D8, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x2, "stxvb16x",(sim_fptr)&simulate_stxvb16x, VSX_VECTOR_LENGTH_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* stxvh8x */{0x7C000758, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x2, "stxvh8x",(sim_fptr)&simulate_stxvh8x, VSX_VECTOR_LENGTH_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* stxvx */{0x7C000318, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x2, "stxvx",(sim_fptr)&simulate_stxvx, VSX_VECTOR_LENGTH_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* xxbrd */	{0xF017076C, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xxbrd", (sim_fptr)&simulate_xxbrd, P9_VSX_MISC_ONLY, FORM_XX2_XT_XB},
/* xxbrh */	{0xF007076C, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xxbrh", (sim_fptr)&simulate_xxbrh, P9_VSX_MISC_ONLY, FORM_XX2_XT_XB},
/* xxbrq */	{0xF01F076C, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xxbrq", (sim_fptr)&simulate_xxbrq, P9_VSX_MISC_ONLY, FORM_XX2_XT_XB},
/* xxbrw */	{0xF00F076C, 0,DUMMY, DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xxbrw", (sim_fptr)&simulate_xxbrw, P9_VSX_MISC_ONLY, FORM_XX2_XT_XB},
/* xxextractuw*/{0xF0000294, 0, IMM_DATA_4BIT, 16, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x8,  "xxextractuw", (sim_fptr)&simulate_xxextractuw, P9_VSX_MISC_ONLY, FORM_XX2_XT_UIM4_XB},
/* xxinsertw*/{0xF00002D4, 0,IMM_DATA_4BIT, 16, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x8,  "xxinsertw", (sim_fptr)&simulate_xxinsertw, P9_VSX_MISC_ONLY, FORM_XX2_XT_UIM4_XB},
/* xxspltib*/{0xF00002D0, 0, IMM_DATA_8BIT, 11, DUMMY, DUMMY, DUMMY, DUMMY, VECTOR_DP, 21, 0x8, "xxspltib",(sim_fptr)&simulate_xxspltib, P9_VSX_MISC_ONLY, FORM_XX1_IMM8},
/* RFC 2464 - Binary Floating-Point Support Operations */
/***********using VSX instructions for BFP **************************/
/* xscmpexpdp */ {0xF00001D8, 0, SCALAR_DP, 16, SCALAR_DP, 11, DUMMY, DUMMY, CR_T,      23, 0x4, "xscmpexpdp", (sim_fptr)&simulate_xscmpexpdp, VSX_SCALAR_DP_FP_COMP_ONLY | HW_ONLY, FORM_XX3_BF_XA_XB},
/* xsiexpdp */   {0xF000072C, 0, GR,            16,    GR,        11, DUMMY, DUMMY, SCALAR_DP, 21, 0x7,  "xsiexpdp",   (sim_fptr)&simulate_xsiexpdp,   P9_VSX_SCALAR_DP_MOVE_ONLY,           FORM_XX1_XT_RA_RB},
/* xststdcdp */  {0xF00005A8, FPS_FPCC, IMM_DATA_7BIT, 16, SCALAR_DP, 11, DUMMY, DUMMY, CR_T,      23, 0x4,  "xststdcdp",  (sim_fptr)&simulate_xststdcdp,  P9_VSX_MISC_ONLY,  X_FORM_BF_DCMX_vrb_eop_rc},
/* xststdcsp */  {0xF00004A8, FPS_FPCC, IMM_DATA_7BIT, 16, SCALAR_SP, 11, DUMMY, DUMMY, CR_T,  23, 0x4,  "xststdcsp",  (sim_fptr)&simulate_xststdcsp,  P9_VSX_MISC_ONLY,  X_FORM_BF_DCMX_vrb_eop_rc},
/* xsxexpdp */   {0xF000056C, 0, DUMMY,         DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, GR,        21, 0x7,  "xsxexpdp",   (sim_fptr)&simulate_xsxexpdp,   P9_VSX_SCALAR_DP_MOVE_ONLY,           FORM_XX2_XT_UIM4_XB},
/* xsxsigdp */   {0xF001056C, 0, DUMMY,         DUMMY, SCALAR_DP, 11, DUMMY, DUMMY, GR,        21, 0x7,  "xsxsigdp",   (sim_fptr)&simulate_xsxsigdp,   P9_VSX_SCALAR_DP_MOVE_ONLY,           FORM_XX2_XT_UIM4_XB},
/* xviexpdp */   {0xF00007C0, 0, VECTOR_DP,     16,    VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xviexpdp",   (sim_fptr)&simulate_xviexpdp,   P9_VSX_SCALAR_DP_MOVE_ONLY,           FORM_XX3_XT_XA_XB},
/* xviexpsp */   {0xF00006C0, 0, VECTOR_SP,     16,    VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xviexpsp",   (sim_fptr)&simulate_xviexpsp,   P9_VSX_MISC_ONLY,           FORM_XX3_XT_XA_XB},
/* xvtstdcdp */  {0xF00007A8, 0, IMM_DATA_5BIT, 16,    VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x8,  "xvtstdcdp",  (sim_fptr)&simulate_xvtstdcdp,  P9_VSX_MISC_ONLY,           FORM_XX2_DX_DC_DM},
/* xvtstdcsp */  {0xF00006A8, 0, IMM_DATA_5BIT, 16,    VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x8,  "xvtstdcsp",  (sim_fptr)&simulate_xvtstdcsp,  P9_VSX_MISC_ONLY,           FORM_XX2_DX_DC_DM},
/* xvxexpdp */   {0xF000076C, 0, DUMMY,         DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvxexpdp",   (sim_fptr)&simulate_xvxexpdp,   P9_VSX_MISC_ONLY,           FORM_XX2_XT_XB},
/* xvxexpsp */   {0xF008076C, 0, DUMMY,         DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvxexpsp",   (sim_fptr)&simulate_xvxexpsp,   P9_VSX_MISC_ONLY,           FORM_XX2_XT_XB},
/* xvxsigdp */   {0xF001076C, 0, DUMMY,         DUMMY, VECTOR_DP, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x0,  "xvxsigdp",   (sim_fptr)&simulate_xvxsigdp,   P9_VSX_MISC_ONLY,           FORM_XX2_XT_XB},
/* xvxsigsp */   {0xF009076C, 0, DUMMY,         DUMMY, VECTOR_SP, 11, DUMMY, DUMMY, VECTOR_SP, 21, 0x0,  "xvxsigsp",   (sim_fptr)&simulate_xvxsigsp,   P9_VSX_MISC_ONLY,           FORM_XX2_XT_XB},
/* RFC02493.r3: D-form VSX Floating-Point Storage Access Instructions */
/* lxsd  */  {0xE4000002, 0, GR, 16, IMM_DATA_14BIT, 2, DUMMY, DUMMY, SCALAR_DP, 21, 0x9, "lxsd",  (sim_fptr)&simulate_lxsd,    VSX_VECTOR_LENGTH_LOAD_ONLY | HIGH32, D_FORM_RT_RA_D},
/* stxsd */  {0xF4000002, 0, GR, 16, IMM_DATA_14BIT, 2, DUMMY, DUMMY, SCALAR_DP, 21, 0xa, "stxsd", (sim_fptr)&simulate_stxsd,   VSX_VECTOR_LENGTH_STORE_ONLY | HIGH32, D_FORM_RT_RA_D},
/* lxssp */  {0xE4000003, 0, GR, 16, IMM_DATA_14BIT, 2, DUMMY, DUMMY, SCALAR_SP, 21, 0x9, "lxssp",  (sim_fptr)&simulate_lxssp,  VSX_VECTOR_LENGTH_LOAD_ONLY | HIGH32, D_FORM_RT_RA_D},
/* stxssp*/  {0xF4000003, 0, GR, 16, IMM_DATA_14BIT, 2, DUMMY, DUMMY, SCALAR_SP, 21, 0xa, "stxssp", (sim_fptr)&simulate_stxssp, VSX_VECTOR_LENGTH_STORE_ONLY | HIGH32, D_FORM_RT_RA_D},
/* lxv   */  {0xF4000001, 0, GR, 16, IMM_DATA_12BIT, 4, DUMMY, DUMMY, VECTOR_DP, 21, 0x9, "lxv",    (sim_fptr)&simulate_lxv,    VSX_VECTOR_LENGTH_LOAD_ONLY, D_FORM_RT_RA_D},
/* stxv  */  {0xF4000005, 0, GR, 16, IMM_DATA_12BIT, 4, DUMMY, DUMMY, VECTOR_DP, 21, 0xa, "stxv",   (sim_fptr)&simulate_stxv,   VSX_VECTOR_LENGTH_STORE_ONLY, D_FORM_RT_RA_D},

/* last ins indicator */ {0xDEADBEEF, 0, DUMMY, 0, DUMMY, 0, DUMMY, DUMMY, DUMMY, 0, 0x0, "last_instruction", 0, 0, 0}

};


uint32 vsr_reg_wt[VSR_OP_TYPES] = {0, 11, 11, 11, 10, 10, 0};
extern store_macro_fptr st_fptrs[VSR_OP_TYPES];
sim_fptr st_sim_fptrs[VSR_OP_TYPES] = {NULL, (sim_fptr)&simulate_stxsdx, (sim_fptr)&simulate_stxsdx, (sim_fptr)&simulate_stxvd2x ,(sim_fptr) &simulate_stxvd2x, (sim_fptr)&simulate_stxvd2x, (sim_fptr)&simulate_stxvd2x, (sim_fptr)&simulate_stxvd2x, NULL};
int sim_fptrs_index[VSR_OP_TYPES]= {0, stxsdx, stxsdx, stxvd2x, stxvd2x, stxvd2x, stxvd2x, stxvd2x, 0};

void class_vsx_mul_add_sub_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr1, vsr2, vsr3;
	uint32 op1, op2, op3, tgt, tgt_reg_no;
	uint32 mcode, store_off, addi_mcode;
	uint32 prolog_size, *tc_memory, num_ins_built;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);


	vsrs = &(cptr->vsrs[temp->op1_dtype]);
	vsr1 = vsrs->head[VSX]->vsr_no;
	/*
	 * Move this VSR to the end of link list
	 */
	MOVE_VSR_TO_END(client_no, temp->op1_dtype, VSX);
#ifdef REUSE
	vsrs->dirty_mask &= (~(0x1ULL << vsr1));
#endif
	op1 = (vsr1 & 0x1f) << (temp->op1_pos) | ((vsr1 & 0x20)  >> 3);

	vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->op2_dtype]);
	vsr2 = vsrs->head[VSX]->vsr_no;
	/*
	 * Move this VSR to the end of link list
	 */
	MOVE_VSR_TO_END(client_no, temp->op2_dtype, VSX);
#ifdef REUSE
	vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif
	op2 = (vsr2 & 0x1f) << (temp->op2_pos) | ((vsr2 & 0x20) >> 4);


	vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->op3_dtype]);
	vsr3 = vsrs->head[VSX]->vsr_no;
	/*
	 * Move this VSR to the end of link list if its not a target.
	 */
	op3 = (vsr3 & 0x1f) << (temp->op3_pos) | ((vsr3 & 0x20) >> 2);

	tgt_reg_no = vsr3;
	tgt = (vsr3 & 0x1f) << (temp->op3_pos) | ((vsr3 & 0x20) >> 5);

	mcode = temp->op_eop_mask | op1 | op2 | op3 | tgt;
	/*
	 * Check if the target is dirty. If target is dirty build store instruction to save VSR
	 * to testcase memory buffer. Check alignment requirement for store instruction.
	 */

	if( (0x1ULL << tgt_reg_no) & vsrs->dirty_mask) {
    /*
	 * Generate store. To generate store, tgt_reg_no's data type should be known.
	 */
		store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
		addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
	 	*tc_memory = addi_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
	 	tc_memory++;
	 	num_ins_built++;
		*tc_memory = (*st_fptrs[temp->tgt_dtype])(tgt_reg_no);
		cptr->instr_index[prolog_size + num_ins_built] = sim_fptrs_index[temp->tgt_dtype] | 0x20000000;
	 	/* save offset */
		cptr->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = store_off;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = st_sim_fptrs[temp->tgt_dtype];
	 	tc_memory++;
		num_ins_built++;
	}

	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
	tc_memory++;
	num_ins_built++;

	/* Update masks for tgt vsr */
	vsrs->dirty_mask |= (0x1ULL << vsr3);
	/* Restore the number of instruction built */
	cptr->num_ins_built = num_ins_built;
}

void class_vsx_normal_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr1, vsr2, vsr3, vsr4;
	uint32 op1, op2, op3, tgt, tgt_reg_no;
	uint32 mcode, store_off, addi_mcode;
	uint32 prolog_size, *tc_memory, num_ins_built;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);


	vsrs = &(cptr->vsrs[temp->op1_dtype]);
	vsr1 = vsrs->head[VSX]->vsr_no;
	/*
	 * Move this VSR to the end of link list
	 */
	MOVE_VSR_TO_END(client_no, temp->op1_dtype, VSX);
#ifdef REUSE
	vsrs->dirty_mask &= (~(0x1ULL << vsr1));
#endif
	op1 = (vsr1 & 0x1f) << (temp->op1_pos) | ((vsr1 & 0x20) >> 3);

	vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->op2_dtype]);
	vsr2 = vsrs->head[VSX]->vsr_no;
	/*
	 * Move this VSR to the end of link list
	 */
	MOVE_VSR_TO_END(client_no, temp->op2_dtype, VSX);
#ifdef REUSE
	vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif
	op2 = (vsr2 & 0x1f) << (temp->op2_pos) | ((vsr2 & 0x20) >> 4);


	vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->op3_dtype]);
	vsr3 = vsrs->head[VSX]->vsr_no;
	/*
	 * Move this VSR to the end of link list if its not a target.
	 */
	op3 = (vsr3 & 0x1f) << (temp->op3_pos) | ((vsr3 & 0x20) >> 2);

	MOVE_VSR_TO_END(client_no, temp->op3_dtype, VSX);
#ifdef REUSE
		vsrs->dirty_mask &= (~(0x1ULL << vsr3));
#endif
	vsrs = &(cptr->vsrs[temp->tgt_dtype]);
	vsr4 = vsrs->head[VSX]->vsr_no;
	tgt = (vsr4 & 0x1f) << (temp->tgt_pos) | ((vsr4 & 0x20) >> 5);
	tgt_reg_no = vsr4;
	mcode = temp->op_eop_mask | op1 | op2 | op3 | tgt;
	/*
	 * Check if the target is dirty. If target is dirty build store instruction to save VSR
	 * to testcase memory buffer. Check alignment requirement for store instruction.
	 */

	if( (0x1ULL << tgt_reg_no) & vsrs->dirty_mask) {
    /*
	 * Generate store. To generate store, tgt_reg_no's data type should be known.
	 */
		store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
		addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
	 	*tc_memory = addi_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
	 	tc_memory++;
	 	num_ins_built++;
		*tc_memory = (*st_fptrs[temp->tgt_dtype])(tgt_reg_no);
		cptr->instr_index[prolog_size + num_ins_built] = sim_fptrs_index[temp->tgt_dtype] | 0x20000000;
	 	/* save offset */
		cptr->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = store_off;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = st_sim_fptrs[temp->tgt_dtype];
	 	tc_memory++;
		num_ins_built++;
	}

	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)temp->sim_func;
	tc_memory++;
	num_ins_built++;

	/* Update masks for tgt vsr */
	vsrs->dirty_mask |= (0x1ULL << vsr4);
	/* Restore the number of instruction built */
	cptr->num_ins_built = num_ins_built;
}

void class_vsx_move_gen (uint32 client_no, uint32 random_no, struct instruction_masks * temp, int index)
{
    uint32 vsr4, vsr1, vsr2;
    uint32 op1 = 0, op2 = 0, tgt = 0;
    uint32 mcode, store_off, addi_mcode;
    uint32 prolog_size, *tc_memory, num_ins_built;
    struct vsr_list *vsrs;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];

    prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
    tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

    /* Generate tgt VSX/GPR reg */
    vsrs = &(cptr->vsrs[temp->tgt_dtype]);
    if (temp->tgt_dtype == GR) {
        vsr4 = vsrs->head[BFP]->vsr_no;
		tgt = ((vsr4 & 0x1f) << (temp->tgt_pos));

	} else {
		vsr4 = vsrs->head[VSX]->vsr_no;
       	tgt = ((vsr4 & 0x1f) << (temp->tgt_pos)) | ((vsr4 & 0x20) >> 5);
	}

	/* see if the target VSR is dirty. If yes, save it in store area */
	if ((0x1ULL << vsr4) & vsrs->dirty_mask) {
    	/* reserve memory for store */
		if (temp->tgt_dtype == GR) {	
    		store_off = init_mem_for_gpr (client_no, 8);
		} else {
        	store_off = init_mem_for_vsx_store (client_no, temp->tgt_dtype);
		}
        addi_mcode = GEN_ADDI_MCODE (STORE_RB, 0, store_off);
        *tc_memory = addi_mcode;
        cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr) & simulate_addi;
        tc_memory++;
        num_ins_built++;
		if (temp->tgt_dtype == GR) {	
        	*tc_memory = STDX (vsr4, STORE_RA, STORE_RB);
        	cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
        	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = simulate_stdx;
		} else {
        	*tc_memory = (*st_fptrs[temp->tgt_dtype]) (vsr4);
        	cptr->instr_index[prolog_size + num_ins_built] = sim_fptrs_index[temp->tgt_dtype] | 0x20000000;
        	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = st_sim_fptrs[temp->tgt_dtype];
		}
        sdata->cdata_ptr[client_no]->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = store_off;
        tc_memory++;
        num_ins_built++;
	}


    /* Update masks for tgt vsr */
    vsrs->dirty_mask |= (0x1ULL << vsr4);

    /* Get source VSR/GPR 1 */
    vsrs = &(cptr->vsrs[temp->op1_dtype]);
    if (temp->op1_dtype == GR) {
        vsr1 = vsrs->head[BFP]->vsr_no;
        MOVE_VSR_TO_END (client_no, temp->op1_dtype, BFP);
        op1 = ((vsr1 & 0x1f) << (temp->op1_pos));
    } else {
        vsr1 = vsrs->head[VSX]->vsr_no;
        MOVE_VSR_TO_END (client_no, temp->op1_dtype, VSX);
        op1 = ((vsr1 & 0x1f) << (temp->op1_pos)) | ((vsr1 & 0x20) >> 5);
    }
#ifdef REUSE
    vsrs->dirty_mask &= (~(0x1ULL << vsr1));
#endif

    /* Get source VSR/GPR 2 */
    vsrs = &(cptr->vsrs[temp->op2_dtype]);
    if (temp->op2_dtype == GR) {
        vsr2 = vsrs->head[BFP]->vsr_no;
        MOVE_VSR_TO_END (client_no, temp->op2_dtype, BFP);
        op2 = ((vsr2 & 0x1f) << (temp->op2_pos));
    } else {
        vsr2 = vsrs->head[VSX]->vsr_no;
        MOVE_VSR_TO_END (client_no, temp->op2_dtype, VSX);
        op2 = ((vsr2 & 0x1f) << (temp->op2_pos)) | ((vsr2 & 0x20) >> 5);
    }
#ifdef REUSE
    vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif

    /* Form machine code */
    mcode = (temp->op_eop_mask | op1 | op2 | tgt);
    *tc_memory = mcode;
    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr) temp->sim_func;
    tc_memory++;
    num_ins_built++;

    /* Restore the number of instruction built */
    cptr->num_ins_built = num_ins_built;
}

void class_vsx_load_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4;
	uint32 op1, op2, tgt;
	uint32 mcode, store_off, load_off, addi_mcode;
	uint32 prolog_size, *tc_memory, num_ins_built;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	/* Generate tgt VSX reg and figure out its data type */
	vsrs = &(cptr->vsrs[temp->tgt_dtype]);
	vsr4 = vsrs->head[VSX]->vsr_no;
	/* see if the target VSR is dirty. If yes, save it in store area */
	if( (0x1ULL << vsr4) & vsrs->dirty_mask) {
		/* reserve memory for store */
		store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
		addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
		*tc_memory = addi_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
		tc_memory++;
		num_ins_built++;
		*tc_memory = (*st_fptrs[temp->tgt_dtype])(vsr4);
		cptr->instr_index[prolog_size + num_ins_built] = sim_fptrs_index[temp->tgt_dtype] | 0x20000000;
		sdata->cdata_ptr[client_no]->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = store_off;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = st_sim_fptrs[temp->tgt_dtype];
		tc_memory++;
		num_ins_built++;
	}
	tgt = (vsr4 & 0x1f) << (temp->tgt_pos) | ((vsr4 & 0x20)  >> 5);
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
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	/* save offset */
	cptr->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = load_off;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)temp->sim_func;
	tc_memory++;
	num_ins_built++;
	/* Check if its a load update */
	if(temp->op1_dtype == GRU) {
		*tc_memory = OR(BASE_GPR, LOAD_RA);
		cptr->instr_index[prolog_size + num_ins_built] = or | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_or;
		tc_memory++;
		num_ins_built++;
	}

	/* Update masks for tgt vsr */
	vsrs->dirty_mask |= (0x1ULL << vsr4);
	/* Restore the number of instruction built */
	cptr->num_ins_built = num_ins_built;
}

void class_vsx_store_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4;
	uint32 op1, op2, tgt;
	uint32 mcode, store_off, addi_mcode;
	uint32 num_ins_built, prolog_size, *tc_memory;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	/* Generate tgt VSX reg and figure out its data type */
	vsrs = &(cptr->vsrs[temp->tgt_dtype]);
	vsr4 = vsrs->head[VSX]->vsr_no;
	/* For store ins, vsr is not getting dirty hence moving to end */
	MOVE_VSR_TO_END(client_no, temp->tgt_dtype, VSX);
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
	tgt = ((vsr4 & 0x1f) << (temp->tgt_pos)) | ((vsr4 & 0x20) >> 5);
	mcode = (temp->op_eop_mask | op1 | op2 | tgt);
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)temp->sim_func;
	/* save offset */
	sdata->cdata_ptr[client_no]->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = store_off;
	tc_memory++;
	num_ins_built++;
	/* Check if its a store update */
	if(temp->op1_dtype == GRU) {
		*tc_memory = OR(BASE_GPR, LOAD_RA);
		cptr->instr_index[prolog_size + num_ins_built] = or | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_or;
		tc_memory++;
		num_ins_built++;
	}

	/* Restore the number of instruction built */
	cptr->num_ins_built = num_ins_built;
}

void class_vsx_test_ins_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr1, vsr2, vsr4;
	uint32 op1, op2, tgt, tgt_reg_no;
	uint32 mcode, store_off, addi_mcode;
	uint32 num_ins_built, prolog_size, *tc_memory;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	vsrs = &(cptr->vsrs[temp->op1_dtype]);
	if (temp->op1_dtype == IMM_DATA_7BIT) {
        vsr1 = random_no & 0x7F;    /* Collect 7 random bits. */
		op1 = (vsr1 & 0x1f) << (temp->op1_pos);
    }
    else {
		vsr1 = vsrs->head[VSX]->vsr_no;
		/*
	 	 * Move this VSR to the end of link list
	 	 */
		MOVE_VSR_TO_END(client_no, temp->op1_dtype, VSX);
	#ifdef REUSE
		vsrs->dirty_mask &= (~(0x1ULL << vsr1));
	#endif
		op1 = (vsr1 & 0x1f) << (temp->op1_pos) | ((vsr1 & 0x20) >> 3);
	}


	vsrs = &(cptr->vsrs[temp->op2_dtype]);
	vsr2 = vsrs->head[VSX]->vsr_no;
	/*
	 * Move this VSR to the end of link list
	 */
	MOVE_VSR_TO_END(client_no, temp->op2_dtype, VSX);
#ifdef REUSE
	vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif
	op2 = (vsr2 & 0x1f) << (temp->op2_pos) | ((vsr2 & 0x20) >> 4);

	/* Target is CR */
	vsrs = &(cptr->vsrs[temp->tgt_dtype]);
	vsr4 = vsrs->head[VSX]->vsr_no;
	/*
	 * Move this to the end of list. This is an exceptional case where the target is getting moved to
	 * the end. This is because there is no instruction which uses CR as source. Hence, this needs to
	 * be shifted explicitly to the end here.
	 */
	MOVE_VSR_TO_END(client_no, temp->tgt_dtype, VSX);
	tgt = (vsr4 & 0x7) << (temp->tgt_pos);
	tgt_reg_no = vsr4;

	mcode = temp->op_eop_mask | op1 | op2 | tgt;
	/*
	 * Check if the target is dirty. If target is dirty build store instruction to save VSR
	 * to testcase memory buffer. Check alignment requirement for store instruction.
	 */

	if( (0x1ULL << tgt_reg_no) & vsrs->dirty_mask) {
		/*
		 * Generate store. To generate store, tgt_reg_no's data type should be known.
		 */

		*tc_memory = MFCR(GPR_TO_SAVE_CR);
		cptr->instr_index[prolog_size + num_ins_built] = mfcr | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_mfcr;
		tc_memory++;
		num_ins_built++;
		/* Following 3 instructions are to mask the CR0 CR1 field of this register */
		addi_mcode = ADDIS(0,0,0x00ff);
		*tc_memory = addi_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = addis | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addis;
		tc_memory++;
		num_ins_built++;
		*tc_memory = ORI(0,0,0xffff);
		cptr->instr_index[prolog_size + num_ins_built] = ori | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_ori;
		tc_memory++;
		num_ins_built++;
		*tc_memory = AND(GPR_TO_SAVE_CR, GPR_TO_SAVE_CR, 0, 0);
		cptr->instr_index[prolog_size + num_ins_built] = and | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_and;
		tc_memory++;
		num_ins_built++;

		/* Reserve memory to save CR value */
		store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
		addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
		*tc_memory = addi_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
		tc_memory++;
		num_ins_built++;
		/* Save dirty CR */
		*tc_memory = STORE_GPR(GPR_TO_SAVE_CR, STORE_RA, STORE_RB);
		cptr->instr_index[prolog_size + num_ins_built] = stwx | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stwx;
		/* save offset */
		cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
		tc_memory++;
		num_ins_built++;
		/*
		 * Clear dirty_mask for target reg
		 */
		 vsrs->dirty_mask &= (~(0x1ULL << tgt_reg_no));
	}
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)temp->sim_func;
	tc_memory++;
	num_ins_built++;

	/* Update masks for tgt vsr */
	vsrs->dirty_mask |= (0x1ULL << vsr4);
	/* Restore the number of instruction built */
	cptr->num_ins_built = num_ins_built;
}

void class_vsx_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 dc = 0, dm = 0;
	uint32 vsr1, vsr2;
	uint32 op1 = 0, op2 = 0, tgt = 0, tgt_reg_no = 0;
	uint32 mcode, store_off, addi_mcode;
	uint32 prolog_size, *tc_memory, num_ins_built;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	/**op1 is immidiate dtype**/
	if (temp->op1_dtype == IMM_DATA_8BIT) {
		op1 = random_no & 0xFF;
	}else if (temp->op1_dtype == IMM_DATA_5BIT) {
		/* xvtstdcdp and xvtstdcsp */
		op1 = random_no & 0x1F;
		dc = random_no & 0x1;
		dc = dc << 6;
		dm = (random_no & 0x2) >> 1;
		dm = dm << 2;
	}else if (temp->op1_dtype == IMM_DATA_4BIT) {
		op1 = random_no & 0xF;
	}else if (temp->op1_dtype == IMM_DATA_2BIT) {
		op1 = random_no & 0x3;
	}
	op1 = op1  << (temp->op1_pos) | dc | dm;   /* DCMX .. dc || dm || dx */

	/***op2 is vsx dtype, DUMMY would be taken care of***/

    vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->op2_dtype]);
    vsr1 = vsrs->head[VSX]->vsr_no;
    /*
     * Move this VSR to the end of link list
     */
    MOVE_VSR_TO_END(client_no, temp->op2_dtype, VSX);
#ifdef REUSE
    vsrs->dirty_mask &= (~(0x1ULL << vsr1));
#endif
    op2 = (vsr1 & 0x1f) << (temp->op2_pos) | ((vsr1 & 0x20) >> 4);

	/***target VSR***/
    vsrs = &(cptr->vsrs[temp->tgt_dtype]);
    vsr2 = vsrs->head[VSX]->vsr_no;
    tgt = (vsr2 & 0x1f) << (temp->tgt_pos) | ((vsr2 & 0x20) >> 5);
    tgt_reg_no = vsr2;
    mcode = temp->op_eop_mask | op1 | op2 | tgt;
    /*
     * Check if the target is dirty. If target is dirty build store instruction to save VSR
     * to testcase memory buffer. Check alignment requirement for store instruction.
     */
    if( (0x1ULL << tgt_reg_no) & vsrs->dirty_mask) {
    /*
     * Generate store. To generate store, tgt_reg_no's data type should be known.
     */
        store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
        addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
        *tc_memory = addi_mcode;
        cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
        tc_memory++;
        num_ins_built++;
        *tc_memory = (*st_fptrs[temp->tgt_dtype])(tgt_reg_no);
        cptr->instr_index[prolog_size + num_ins_built] = sim_fptrs_index[temp->tgt_dtype] | 0x20000000;
        /* save offset */
        cptr->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = store_off;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = st_sim_fptrs[temp->tgt_dtype];
        tc_memory++;
        num_ins_built++;
    }

    *tc_memory = mcode;
    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)temp->sim_func;
    tc_memory++;
    num_ins_built++;

    /* Update masks for tgt vsr */
    vsrs->dirty_mask |= (0x1ULL << vsr2);
    /* Update the number of instruction built */
    cptr->num_ins_built = num_ins_built;
}

void class_vsx_load_gen2(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
    uint32 gpr_num, vsr4;
    uint32 op1 = 0, op2 = 0, tgt = 0;
    uint32 mcode = 0, store_off = 0, load_off = 0, addi_mcode = 0;
    uint32 prolog_size, *tc_memory, num_ins_built;
    struct vsr_list *vsrs;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];
	uint16 backup_bdy;

    prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
    tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

    /* Generate tgt VSX reg and figure out its data type */
    vsrs = &(cptr->vsrs[temp->tgt_dtype]);
    vsr4 = vsrs->head[VSX]->vsr_no;
    /* see if the target VSR is dirty. If yes, save it in store area */
    if( (0x1ULL << vsr4) & vsrs->dirty_mask) {
        /* reserve memory for store */
        store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
        addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
        *tc_memory = addi_mcode;
        cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
        tc_memory++;
        num_ins_built++;
        *tc_memory = (*st_fptrs[temp->tgt_dtype])(vsr4);
        cptr->instr_index[prolog_size + num_ins_built] = sim_fptrs_index[temp->tgt_dtype] | 0x20000000;
        sdata->cdata_ptr[client_no]->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = store_off;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = st_sim_fptrs[temp->tgt_dtype];
        tc_memory++;
        num_ins_built++;
    }
    
    op1 = (LOAD_RA) << (temp->op1_pos);

	backup_bdy = cptr->bdy_req;
	if (temp->op2_dtype == IMM_DATA_12BIT) {
		tgt = (vsr4 & 0x1f) << (temp->tgt_pos) | (((vsr4 & 0x20) >> 5) << 3);
		/* reserve memory for load */
        cptr->bdy_req = ALIGN_QW;
    	/* find out load ptr and initialize the data pattern at that address */
    	load_off = init_mem_for_vsx_load(client_no, temp->tgt_dtype);    
        op2 = ((load_off >> 4) & 0xFFF) << (temp->op2_pos); /* immidiate data 12 bits */
	}
	else if (temp->op2_dtype == IMM_DATA_14BIT) {
		vsr4 = (vsr4 >= 32)? vsr4 - 32: vsr4;
    	tgt = (vsr4 & 0x1f) << (temp->tgt_pos); /* instruction would use register vsr4 + 32 */
		backup_bdy = cptr->bdy_req;
    	cptr->bdy_req = ALIGN_W;
    	load_off = init_mem_for_vsx_load(client_no, temp->tgt_dtype);    
    	op2 = ((load_off >> 2) & 0x3fff) << (temp->op2_pos);
    }
	else if (temp->op2_dtype == GR) {
		gpr_num = get_random_gpr(client_no, GR, 0);
		op2 = (gpr_num  & 0x1f) << temp->op2_pos;
	}

	cptr->bdy_req = backup_bdy;

    /* Form machine code for load instruction */
    mcode = (temp->op_eop_mask | op1 | op2 | tgt);
    *tc_memory = mcode;
    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    
    /* save offset */
    cptr->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = load_off;
    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)temp->sim_func;
    tc_memory++;
    num_ins_built++;
    
    /* Update masks for tgt vsr */
    vsrs->dirty_mask |= (0x1ULL << vsr4);
    
    /* Restore the number of instruction built */
    cptr->num_ins_built = num_ins_built;
}


void class_vsx_store_gen2(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
    uint32 vsr4;
    uint32 op1 = 0, op2 = 0, tgt = 0;
    uint32 mcode, store_off = 0;
    uint32 num_ins_built, prolog_size, *tc_memory;
    struct vsr_list *vsrs;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];
	uint16 backup_bdy;

    prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
    tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

    /* Generate tgt VSX reg and figure out its data type */
    vsrs = &(cptr->vsrs[temp->tgt_dtype]);
    vsr4 = vsrs->head[VSX]->vsr_no;
    
    /* For store ins, vsr is not getting dirty hence moving to end */
    MOVE_VSR_TO_END(client_no, temp->tgt_dtype, VSX);
    /* Reset dirty bit corresponding to target VSR */
    vsrs->dirty_mask &= (~(0x1ULL << vsr4));
    
    op1 = (STORE_RA & 0x1f) << (temp->op1_pos);
    
	backup_bdy = cptr->bdy_req;
	if (temp->op2_dtype == IMM_DATA_12BIT) {
    	/* reserve memory for store */
    	cptr->bdy_req = ALIGN_QW;
    	store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
    	op2 = ((store_off >> 4) & 0xFFF) << (temp->op2_pos); /* immidiate data 12 bits */
	}
	else if (temp->op2_dtype == IMM_DATA_14BIT) {
    	/* reserve memory for store */
		backup_bdy = cptr->bdy_req;
    	cptr->bdy_req = ALIGN_W;
    	store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
    	op2 = ((store_off >> 2) & 0x3fff) << (temp->op2_pos); /* immidiate data 14 bits */
	}
	cptr->bdy_req = backup_bdy;
    
	if (temp->op2_dtype == IMM_DATA_12BIT) {
		tgt = (vsr4 & 0x1f) << (temp->tgt_pos) | (((vsr4 & 0x20) >> 5) << 3);
	} else if (temp->op2_dtype == IMM_DATA_14BIT) {
		vsr4 = (vsr4 >= 32)? vsr4 - 32: vsr4;
    	tgt = ((vsr4 & 0x1f) << (temp->tgt_pos)); 
	}
    
    mcode = (temp->op_eop_mask | op1 | op2 | tgt);
    *tc_memory = mcode;
    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)temp->sim_func;
    
    /* save offset */
    sdata->cdata_ptr[client_no]->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = store_off;
    tc_memory++;
    num_ins_built++;

    /* Restore the number of instruction built */
    cptr->num_ins_built = num_ins_built;
}


void class_vsx_stxvll_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4;
	uint32 op1, op2, src;
	uint32 mcode, store_off, dep_mcode;
	uint32 num_ins_built, prolog_size, *tc_memory;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	/* Generate tgt VSX reg and figure out its data type */
	vsrs = &(cptr->vsrs[temp->tgt_dtype]);
	vsr4 = vsrs->head[VSX]->vsr_no;
	/* For store ins, vsr is not getting dirty hence moving to end */
	MOVE_VSR_TO_END(client_no, temp->tgt_dtype, VSX);
	/* Reset dirty bit corresponding to target VSR */
	vsrs->dirty_mask &= (~(0x1ULL << vsr4));

	/* reserve memory for store */
	store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
	dep_mcode = GEN_ADDI_MCODE(STORE_RA, STORE_RA, store_off);
	*tc_memory = dep_mcode;
	cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
	tc_memory++;
	num_ins_built++;

	dep_mcode = GEN_ADDI_MCODE(STORE_RB, 0, (random_no & 0xF));
	*tc_memory = dep_mcode;
	cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
	tc_memory++;
	num_ins_built++;

	op1 = (STORE_RA) << (temp->op1_pos); /* RA = RA + store_off */
	op2 = (STORE_RB) << (temp->op2_pos);
	src = ((vsr4 & 0x1f) << (temp->tgt_pos)) | ((vsr4 & 0x20) >> 5);

	mcode = (temp->op_eop_mask | op1 | op2 | src);
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)temp->sim_func;
	tc_memory++;
	num_ins_built++;

	/* save offset */
	sdata->cdata_ptr[client_no]->tc_ptr[INITIAL_BUF]->ea_off[num_ins_built + prolog_size] = store_off;

	/* Restore R5 */
	*tc_memory = OR(BASE_GPR, LOAD_RA);
	cptr->instr_index[prolog_size + num_ins_built] = or | 0x20000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_or;
	tc_memory++;
	num_ins_built++;

	/* Restore the number of instruction built */
	cptr->num_ins_built = num_ins_built;
}
