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

/*static char sccid[] = "%Z%%M% %I%  %W% %G% %U%";*/
#include "framework.h"

/*
 * Array of structures of type instruction_masks. Each element in array
 * describes one instruction.
 */
struct instruction_masks bfp_instructions_array[] = {
	/* All BFP instructions */

	/* BFP Load/Store Instructions */
	/* Load with indexed offset without update */
	/* lfsx		*/	{0x7C00042E, 0, GR, 16, GR, 11, DUMMY, DUMMY, BFP_SP, 21, 0x10, "lfsx", &simulate_lfsx, BFP_LOAD_SP, X_FORM_RT_RA_RB_eop_rc},
	/* lfdx		*/	{0x7C0004AE, 0, GR, 16, GR, 11, DUMMY, DUMMY, BFP_DP, 21, 0x10, "lfdx", &simulate_lfdx, BFP_LOAD_DP, X_FORM_RT_RA_RB_eop_rc},
	/* lfiwax	*/	{0x7C0006AE, 0, GR, 16, GR, 11, DUMMY, DUMMY, BFP_DP, 21, 0x10, "lfiwax", &simulate_lfiwax, BFP_LOAD_INT, X_FORM_RT_RA_RB_eop_rc},

	/* Load with indexed offset with update	*/
	/* lfsux	*/	{0x7C00046E, 0, GRU, 16, GR, 11, DUMMY, DUMMY, BFP_SP, 21, 0x10, "lfsux", &simulate_lfsux, BFP_LOAD_SP, X_FORM_RT_RA_RB_eop_rc},
	/* lfdux	*/	{0x7C0004EE, 0, GRU, 16, GR, 11, DUMMY, DUMMY, BFP_DP, 21, 0x10, "lfdux", &simulate_lfdux, BFP_LOAD_DP, X_FORM_RT_RA_RB_eop_rc},

	/* Load with immediate offset without update */
	/* lfs		*/	{0xC0000000, 0, GR, 16, IMM_DATA, 0, DUMMY, DUMMY, BFP_SP, 21, 0x11, "lfs", &simulate_lfs, BFP_LOAD_SP, D_FORM_RT_RA_D},
	/* lfd		*/	{0xC8000000, 0, GR, 16, IMM_DATA, 0, DUMMY, DUMMY, BFP_DP, 21, 0x11, "lfd", &simulate_lfd, BFP_LOAD_DP, D_FORM_RT_RA_D},

	/* Load with immediate offset with update */
	/* lfsu		*/	{0xC4000000, 0, GRU, 16, IMM_DATA, 0, DUMMY, DUMMY, BFP_SP, 21, 0x11, "lfsu", &simulate_lfsu, BFP_LOAD_SP, D_FORM_RT_RA_D},
	/* lfdu		*/	{0xCC000000, 0, GRU, 16, IMM_DATA, 0, DUMMY, DUMMY, BFP_DP, 21, 0x11, "lfdu", &simulate_lfdu, BFP_LOAD_DP, D_FORM_RT_RA_D},

	/* Store with indexed offset without update */
	/* stfsx	*/	{0x7C00052E, 0, GR, 16, GR, 11, DUMMY, DUMMY, BFP_SP, 21, 0x12, "stfsx", &simulate_stfsx, BFP_STORE_SP, X_FORM_RT_RA_RB_eop_rc},
	/* stfdx	*/	{0x7C0005AE, 0, GR, 16, GR, 11, DUMMY, DUMMY, BFP_DP, 21, 0x12, "stfdx", &simulate_stfdx, BFP_STORE_DP, X_FORM_RT_RA_RB_eop_rc},
	/* stfiwx	*/	{0x7C0007AE, 0, GR, 16, GR, 11, DUMMY, DUMMY, BFP_DP, 21, 0x12, "stfiwx", &simulate_stfiwx, BFP_STORE_INT, X_FORM_RT_RA_RB_eop_rc},

	/* Store with indexed offset with update */
	/* stfsux	*/	{0x7C00056E, 0, GRU, 16, GR, 11, DUMMY, DUMMY, BFP_SP, 21, 0x12, "stfsux", &simulate_stfsux, BFP_STORE_SP, X_FORM_RT_RA_RB_eop_rc},
	/* stfdux	*/	{0x7C0005EE, 0, GRU, 16, GR, 11, DUMMY, DUMMY, BFP_DP, 21, 0x12, "stfdux", &simulate_stfdux, BFP_STORE_DP, X_FORM_RT_RA_RB_eop_rc},

	/* Store with immediate offset without update */
	/* stfs		*/	{0xD0000000, 0, GR, 16, IMM_DATA, 0, DUMMY, DUMMY, BFP_SP, 21, 0x13, "stfs", &simulate_stfs, BFP_STORE_SP, D_FORM_RT_RA_D},
	/* stfd		*/	{0xD8000000, 0, GR, 16, IMM_DATA, 0, DUMMY, DUMMY, BFP_DP, 21, 0x13, "stfd", &simulate_stfd, BFP_STORE_DP, D_FORM_RT_RA_D},

	/* Store with immediate offset with update */
	/* stfsu	*/	{0xD4000000, 0, GRU, 16, IMM_DATA, 0, DUMMY, DUMMY, BFP_SP, 21, 0x13, "stfsu", &simulate_stfsu, BFP_STORE_SP, D_FORM_RT_RA_D},
	/* stfdu	*/	{0xDC000000, 0, GRU, 16, IMM_DATA, 0, DUMMY, DUMMY, BFP_DP, 21, 0x13, "stfdu", &simulate_stfdu, BFP_STORE_DP, D_FORM_RT_RA_D},


	/* Normal BFP arithmetic oprerations */

	/* Floating-point move instruction */
	/* fmr		*/ 	{0xFC000090, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fmr", &simulate_fmr, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* fmr.		*/ 	{0xFC000091, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fmr.", &simulate_fmr, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* fabs		*/ 	{0xFC000210, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fabs", &simulate_fabs, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* fabs.	*/ 	{0xFC000211, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fabs.", &simulate_fabs, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* fnabs	*/ 	{0xFC000110, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fnabs", &simulate_fnabs, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* fnabs.	*/ 	{0xFC000111, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fnabs.", &simulate_fnabs, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* fneg		*/	{0xFC000050, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fneg", &simulate_fneg, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* fneg.	*/	{0xFC000051, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fneg.", &simulate_fneg, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* fcpsgn	*/ 	{0xFC000010, 0, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fcpsgn", &simulate_fcpsgn, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* fcpsgn.	*/ 	{0xFC000011, 0, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fcpsgn.", &simulate_fcpsgn, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},

	/* Floating-point elementary arithmetic instructions */
	/* fadd		*/	{0xFC00002A, 0x9b87f000, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fadd", &simulate_fadd, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fadd.	*/	{0xFC00002B, 0x9b87f000, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fadd.", &simulate_fadd_dot, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fadds	*/	{0xEC00002A, 0x9b87f000, BFP_SP, 16, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fadds", &simulate_fadds, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fadds.	*/	{0xEC00002B, 0x9b87f000, BFP_SP, 16, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fadds.", &simulate_fadds_dot, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fsub		*/	{0xFC000028, 0x9b87f000, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fsub", &simulate_fsub, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fsub.	*/	{0xFC000029, 0x9b87f000, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fsub.", &simulate_fsub_dot, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fsubs	*/	{0xEC000028, 0x9b87f000, BFP_SP, 16, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fsubs", &simulate_fsubs, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fsubs.	*/	{0xEC000029, 0x9b87f000, BFP_SP, 16, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fsubs.", &simulate_fsubs_dot, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmul		*/	{0xFC000032, 0x9b17f000, BFP_DP, 16, DUMMY, DUMMY, BFP_DP, 6, BFP_DP, 21, 0x14, "fmul", &simulate_fmul, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmul.	*/	{0xFC000033, 0x9b17f000, BFP_DP, 16, DUMMY, DUMMY, BFP_DP, 6, BFP_DP, 21, 0x14, "fmul.", &simulate_fmul_dot, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmuls	*/	{0xEC000032, 0x9b17f000, BFP_SP, 16, DUMMY, DUMMY, BFP_SP, 6, BFP_SP, 21, 0x14, "fmuls", &simulate_fmuls, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmuls.	*/	{0xEC000033, 0x9b17f000, BFP_SP, 16, DUMMY, DUMMY, BFP_SP, 6, BFP_SP, 21, 0x14, "fmuls.", &simulate_fmuls_dot, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fdiv		*/	{0xFC000024, 0x9f67f000, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fdiv", &simulate_fdiv, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fdiv.	*/	{0xFC000025, 0x9f67f000, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fdiv.", &simulate_fdiv_dot, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fdivs	*/	{0xEC000024, 0x9f67f000, BFP_SP, 16, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fdivs", &simulate_fdivs, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fdivs.	*/	{0xEC000025, 0x9f67f000, BFP_SP, 16, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fdivs.", &simulate_fdivs_dot, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fsqrt	*/	{0xFC00002C, 0x8307f200, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fsqrt", &simulate_fsqrt, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fsqrt.	*/	{0xFC00002D, 0x8307f200, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fsqrt.", &simulate_fsqrt_dot, BFP_ELEM_ARITH_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fsqrts	*/	{0xEC00002C, 0x9b07f200, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fsqrts", &simulate_fsqrts, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fsqrts.	*/	{0xEC00002D, 0x9b07f200, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fsqrts.", &simulate_fsqrts_dot, BFP_ELEM_ARITH_SP, A_FORM_RT_RA_RB_RC_eop_rc},

	/* Floating-point Multiply-Add instructions */
	/* fmadd	*/	{0xFC00003A, 0x9b97f000, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fmadd", &simulate_fmadd, BFP_ADD_MUL_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmadd.	*/	{0xFC00003B, 0x9b97f000, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fmadd.", &simulate_fmadd, BFP_ADD_MUL_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmadds	*/	{0xEC00003A, 0x9b97f000, BFP_SP, 16, BFP_SP, 11, BFP_SP, 6, BFP_SP, 21, 0x14, "fmadds", &simulate_fmadds, BFP_ADD_MUL_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmadds.	*/	{0xEC00003B, 0x9b97f000, BFP_SP, 16, BFP_SP, 11, BFP_SP, 6, BFP_SP, 21, 0x14, "fmadds.", &simulate_fmadds, BFP_ADD_MUL_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmsub	*/	{0xFC000038, 0x9b97f000, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fmsub", &simulate_fmsub, BFP_ADD_MUL_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmsub.	*/	{0xFC000039, 0x9b97f000, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fmsub.", &simulate_fmsub, BFP_ADD_MUL_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmsubs	*/	{0xEC000038, 0x9b97f000, BFP_SP, 16, BFP_SP, 11, BFP_SP, 6, BFP_SP, 21, 0x14, "fmsubs", &simulate_fmsubs, BFP_ADD_MUL_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fmsubs.	*/	{0xEC000039, 0x9b97f000, BFP_SP, 16, BFP_SP, 11, BFP_SP, 6, BFP_SP, 21, 0x14, "fmsubs.", &simulate_fmsubs, BFP_ADD_MUL_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fnmadd	*/	{0xFC00003E, 0x9b97f000, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fnmadd", &simulate_fnmadd, BFP_ADD_MUL_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fnmadd.	*/	{0xFC00003F, 0x9b97f000, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fnmadd.", &simulate_fnmadd, BFP_ADD_MUL_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fnmadds	*/	{0xEC00003E, 0x9b97f000, BFP_SP, 16, BFP_SP, 11, BFP_SP, 6, BFP_SP, 21, 0x14, "fnmadds", &simulate_fnmadds, BFP_ADD_MUL_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fnmadds.	*/	{0xEC00003F, 0x9b97f000, BFP_SP, 16, BFP_SP, 11, BFP_SP, 6, BFP_SP, 21, 0x14, "fnmadds.", &simulate_fnmadds, BFP_ADD_MUL_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fnmsub	*/	{0xFC00003C, 0x9b97f000, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fnmsub", &simulate_fnmsub, BFP_ADD_MUL_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fnmsub.	*/	{0xFC00003D, 0x9b97f000, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fnmsub.", &simulate_fnmsub, BFP_ADD_MUL_DP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fnmsubs	*/	{0xEC00003C, 0x9b97f000, BFP_SP, 16, BFP_SP, 11, BFP_SP, 6, BFP_SP, 21, 0x14, "fnmsubs", &simulate_fnmsubs, BFP_ADD_MUL_SP, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fnmsubs.	*/	{0xEC00003D, 0x9b97f000, BFP_SP, 16, BFP_SP, 11, BFP_SP, 6, BFP_SP, 21, 0x14, "fnmsubs.", &simulate_fnmsubs, BFP_ADD_MUL_SP, A_FORM_RT_RA_RB_RC_eop_rc},

	/* Floating-point Rounding and Conversion instructions */
	/* frsp		*/	{0xFC000018, 0x9b07f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "frsp", &simulate_frsp, BFP_ROUND_2_SP, X_FORM_RT_RA_RB_eop_rc},
	/* frsp.	*/	{0xFC000019, 0x9b07f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "frsp.", &simulate_frsp, BFP_ROUND_2_SP, X_FORM_RT_RA_RB_eop_rc},
	/* fctid	*/	{0xFC00065C, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fctid", &simulate_fctid, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctid.	*/	{0xFC00065D, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fctid.", &simulate_fctid, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctidz	*/	{0xFC00065E, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fctidz", &simulate_fctidz, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctidz.	*/	{0xFC00065F, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fctidz.", &simulate_fctidz, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiw	*/	{0xFC00001C, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fctiw", &simulate_fctiw, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiw.	*/	{0xFC00001D, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fctiw.", &simulate_fctiw, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiwz	*/	{0xFC00001E, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fctiwz", &simulate_fctiwz, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiwz.	*/	{0xFC00001F, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fctiwz.", &simulate_fctiwz, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fcfid	*/	{0xFC00069C, 0x8207f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fcfid", &simulate_fcfid, BFP_CONV_FROM_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fcfid.	*/	{0xFC00069D, 0x8207f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fcfid.", &simulate_fcfid, BFP_CONV_FROM_INT, X_FORM_RT_RA_RB_eop_rc},
	/* frin		*/	{0xFC000310, 0x8107f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frin", &simulate_frin, BFP_ROUND_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* frin.	*/	{0xFC000311, 0x8107f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frin.", &simulate_frin, BFP_ROUND_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* friz		*/	{0xFC000350, 0x8107f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "friz", &simulate_friz, BFP_ROUND_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* friz.	*/	{0xFC000351, 0x8107f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "friz.", &simulate_friz, BFP_ROUND_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* frip		*/	{0xFC000390, 0x8107f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frip", &simulate_frip, BFP_ROUND_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* frip.	*/	{0xFC000391, 0x8107f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frip.", &simulate_frip, BFP_ROUND_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* frim		*/	{0xFC0003D0, 0x8107f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frim", &simulate_frim, BFP_ROUND_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* frim.	*/	{0xFC0003D1, 0x8107f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frim.", &simulate_frim, BFP_ROUND_2_INT, X_FORM_RT_RA_RB_eop_rc},

	/* Floating-point Select instruction */
	/* fsel		*/	{0xFC00002E, 0, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fsel", &simulate_fsel, BFP_SELECT_ONLY, A_FORM_RT_RA_RB_RC_eop_rc},
	/* fsel.	*/	{0xFC00002F, 0, BFP_DP, 16, BFP_DP, 11, BFP_DP, 6, BFP_DP, 21, 0x14, "fsel.", &simulate_fsel_dot, BFP_SELECT_ONLY, A_FORM_RT_RA_RB_RC_eop_rc},

	/* Floating Reciprocal Estimate instructions */
	/* fre(L0)		{0xFC000030, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fre", &simulate_fre, A_FORM_RT_RA_RB_RC_eop_rc},
	 fre.(L0)		{0xFC000031, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fre.", &simulate_fre, A_FORM_RT_RA_RB_RC_eop_rc},
	 fre(L1)		{0xFC010030, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fre", &simulate_fre, A_FORM_RT_RA_RB_RC_eop_rc},
	 fre.(L1)		{0xFC010031, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fre.", &simulate_fre, A_FORM_RT_RA_RB_RC_eop_rc},
	 fres(L0)		{0xEC000030, 0, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fres", &simulate_fres, A_FORM_RT_RA_RB_RC_eop_rc},
	 fres.(L0)	{0xEC000031, 0, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fres.", &simulate_fres, A_FORM_RT_RA_RB_RC_eop_rc},
	 fres(L1)		{0xEC010030, 0, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fres", &simulate_fres, A_FORM_RT_RA_RB_RC_eop_rc},
	 fres.(L1)	{0xEC010031, 0, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fres.", &simulate_fres, A_FORM_RT_RA_RB_RC_eop_rc},
	 frsqrte		{0xFC000034, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frsqrte", &simulate_frsqrte, A_FORM_RT_RA_RB_RC_eop_rc},
	 frsqrte.		{0xFC000035, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frsqrte.", &simulate_frsqrte, A_FORM_RT_RA_RB_RC_eop_rc},
	 frsqrte		{0xFC010034, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frsqrte", &simulate_frsqrte, A_FORM_RT_RA_RB_RC_eop_rc},
	 frsqrte.		{0xFC010035, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "frsqrte.", &simulate_frsqrte, A_FORM_RT_RA_RB_RC_eop_rc},
	 frsqrtes		{0xEC000034, 0, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "frsqrtes", &simulate_frsqrtes, A_FORM_RT_RA_RB_RC_eop_rc},
	 frsqrtes.	{0xEC000035, 0, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "frsqrtes.", &simulate_frsqrtes, A_FORM_RT_RA_RB_RC_eop_rc},
	 frsqrtes		{0xEC010034, 0, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "frsqrtes", &simulate_frsqrtes, A_FORM_RT_RA_RB_RC_eop_rc},
	 frsqrtes.	{0xEC010035, 0, DUMMY, DUMMY, BFP_SP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "frsqrtes.", &simulate_frsqrtes, A_FORM_RT_RA_RB_RC_eop_rc},

*/

	/* Floating-point Status and Control Register(FPSCR) instructions */
	/* mffs		*/	{0xFC00048E, 0, DUMMY, DUMMY, DUMMY, DUMMY,DUMMY, DUMMY, BFP_DP, 21, 0x14, "mffs", &simulate_mffs, BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* mffs.	*/	{0xFC00048F, 0, DUMMY, DUMMY, DUMMY, DUMMY,DUMMY, DUMMY, BFP_DP, 21, 0x14, "mffs.", &simulate_mffs, BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},

	/* mtfsfi(W0)*/	{0xFC00010C, 0xffffffff, DUMMY, DUMMY, IMM_DATA_4BIT, 12, DUMMY, DUMMY, CR_T, 23, 0x16, "mtfsfi", &simulate_mtfsfi, BFP_FPSCR_ONLY, X_FORM_BF_W_U_eop_rc},
	/* mtfsfi.	*/	{0xFC00010D, 0xffffffff, DUMMY, DUMMY, IMM_DATA_4BIT, 12, DUMMY, DUMMY, CR_T, 23, 0x16, "mtfsfi.", &simulate_mtfsfi, BFP_FPSCR_ONLY, X_FORM_BF_W_U_eop_rc},

	/* mtfsf(LW)*/
	/* mtfsf(00)*/	{0xFC00058E, 0xffffffff, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, DUMMY, 17, 0x15, "mtfsf", &simulate_mtfsf, BFP_FPSCR_ONLY, XFL_FORM_L_FLM_W_RB_eop_rc},
	/* mtfsf.	*/	{0xFC00058F, 0xffffffff, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, DUMMY, 17, 0x15, "mtfsf.", &simulate_mtfsf, BFP_FPSCR_ONLY, XFL_FORM_L_FLM_W_RB_eop_rc},

	/* mtfsb0	*/	{0xFC00008C, 0xffffffff, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, 21, 0x19, "mtfsb0", &simulate_mtfsb0, BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* mtfsb0.	*/	{0xFC00008D, 0xffffffff, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, 21, 0x19, "mtfsb0.", &simulate_mtfsb0, BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* mtfsb1	*/	{0xFC00004C, 0xffffffff, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, 21, 0x19, "mtfsb1", &simulate_mtfsb1, BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* mtfsb1.	*/	{0xFC00004D, 0xffffffff, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, 21, 0x19, "mtfsb1.", &simulate_mtfsb1, BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* Those instructions that update CR */

	/* Floating-point Compare instructions */
	/* fcmpu	*/	{0xFC000000, 0x8100f000, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, CR_T, 23, 0x17, "fcmpu", &simulate_fcmpu, BFP_COMPARE_ONLY, X_FORM_BF_RA_RB_eop_rc},
	/* fcmpo	*/	{0xFC000040, 0x8108f000, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, CR_T, 23, 0x17, "fcmpo", &simulate_fcmpo, BFP_COMPARE_ONLY, X_FORM_BF_RA_RB_eop_rc},

	/* mcrfs	*/	{0xFC000080, 0xffffffff, CR_T, 18, DUMMY, DUMMY, DUMMY, DUMMY, CR_T, 23, 0x18, "mcrfs", &simulate_mcrfs, BFP_FPSCR_ONLY, X_FORM_BF_BFA_eop_rc},
	/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};

struct instruction_masks bfp_p7_instructions_array[] = {
	/* All BFP p7 specific instructions */

	/* lfiwzx	*/	{0x7C0006EE, 0, GR, 16, GR, 11, DUMMY, DUMMY, BFP_DP, 21, 0x10, "lfiwzx", &simulate_lfiwzx, BFP_LOAD_INT, X_FORM_RT_RA_RB_eop_rc},

	/* Floating-point Test instructions */
	/* ftdiv	*/	{0xFC000100, 0, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, CR_T, 23, 0x17, "ftdiv", &simulate_ftdiv, BFP_TEST_ONLY, X_FORM_BF_RA_RB_eop_rc},
	/* ftsqrt	*/	{0xFC000140, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, CR_T, 23, 0x17, "ftsqrt", &simulate_ftsqrt, BFP_TEST_ONLY, X_FORM_BF_RA_RB_eop_rc},

	/* fctidu	*/	{0xFC00075C, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fctidu", &simulate_fctidu, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctidu.	*/	{0xFC00075D, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fctidu.", &simulate_fctidu, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiduz	*/	{0xFC00075E, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fctiduz", &simulate_fctiduz, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiduz.	*/	{0xFC00075F, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fctiduz.", &simulate_fctiduz, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiwu	*/	{0xFC00011C, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fctiwu", &simulate_fctiwu, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiwu.	*/	{0xFC00011D, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fctiwu.", &simulate_fctiwu, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiwuz	*/	{0xFC00011E, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fctiwuz", &simulate_fctiwuz, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fctiwuz.	*/	{0xFC00011F, 0x8307f100, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fctiwuz.", &simulate_fctiwuz, BFP_CONV_2_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fcfidu	*/	{0xFC00079C, 0x8207f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fcfidu", &simulate_fcfidu, BFP_CONV_FROM_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fcfidu.	*/	{0xFC00079D, 0x8207f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fcfidu.", &simulate_fcfidu, BFP_CONV_FROM_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fcfids	*/	{0xEC00069C, 0x8207f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fcfids", &simulate_fcfids, BFP_CONV_FROM_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fcfids.	*/	{0xEC00069D, 0x8207f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fcfids.", &simulate_fcfids, BFP_CONV_FROM_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fcfidus	*/	{0xEC00079C, 0x8207f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fcfidus", &simulate_fcfidus, BFP_CONV_FROM_INT, X_FORM_RT_RA_RB_eop_rc},
	/* fcfidus. */ 	{0xEC00079D, 0x8207f000, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_SP, 21, 0x14, "fcfidus.", &simulate_fcfidus, BFP_CONV_FROM_INT, X_FORM_RT_RA_RB_eop_rc},

	/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}


};

struct instruction_masks bfp_p8_instructions_array[] = {
	/* All BFP p8 specific instructions */

	/* Floating point merge instructions */
	/* fmrgew */	{0xFC00078C, 0, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fmrgew", simulate_fmrgew, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc }, 
	/* fmrgow */	{0xFC00068C, 0, BFP_DP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "fmrgow", simulate_fmrgow, BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc }, 
	/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}


};


struct instruction_masks bfp_p9_instructions_array[] = {
/* RFC02465 - Quad precision BFP operations */
/* xscpsgnqp */	{0xFC0000C8, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xscpsgnqp",(sim_fptr)&simulate_xscpsgnqp,P9_BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},

/* xsaddqp	*/	{0xFC000008, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsaddqp", (sim_fptr)&simulate_xsaddqp, P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsaddqpo	*/	{0xFC000009, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsaddqpo",(sim_fptr)&simulate_xsaddqpo,P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xssubqp	*/	{0xFC000408, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xssubqp", (sim_fptr)&simulate_xssubqp, P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xssubqpo	*/	{0xFC000409, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xssubqpo",(sim_fptr)&simulate_xssubqpo,P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsmulqp	*/	{0xFC000048, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsmulqp", (sim_fptr)&simulate_xsmulqp, P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsmulqpo	*/	{0xFC000049, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsmulqpo",(sim_fptr)&simulate_xsmulqpo,P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsdivqp	*/	{0xFC000448, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsdivqp", (sim_fptr)&simulate_xsdivqp, P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsdivqpo	*/	{0xFC000449, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsdivqpo",(sim_fptr)&simulate_xsdivqpo,P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsnegqp	*/	{0xFC100648, 0, DUMMY,DUMMY,BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsnegqp", (sim_fptr)&simulate_xsnegqp, P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsabsqp	*/	{0xFC000648, 0, DUMMY,DUMMY,BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsabsqp", (sim_fptr)&simulate_xsabsqp, P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsnabsqp	*/	{0xFC080648, 0, DUMMY,DUMMY,BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsnabsqp",(sim_fptr)&simulate_xsnabsqp,P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},

/* xscmpexpqp*/ {0xFC000148, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, CR_T, 23, 0x14,"xscmpexpqp",(sim_fptr)&simulate_xscmpexpqp,P9_BFP_COMPARE_ONLY,X_FORM_BF_RA_RB_eop_rc},
/* xscmpoqp	*/	{0xFC000108, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, CR_T, 23, 0x14, "xscmpoqp", (sim_fptr)&simulate_xscmpoqp,  P9_BFP_COMPARE_ONLY,X_FORM_BF_RA_RB_eop_rc},
/* xscmpuqp	*/	{0xFC000508, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, CR_T, 23, 0x14, "xscmpuqp", (sim_fptr)&simulate_xscmpuqp,  P9_BFP_COMPARE_ONLY,X_FORM_BF_RA_RB_eop_rc},

/* xscvdpqp	*/	{0xFC160688, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xscvdpqp", (sim_fptr)&simulate_xscvdpqp, P9_BFP_CONV_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xscvqpdp	*/	{0xFC140688, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "xscvqpdp", (sim_fptr)&simulate_xscvqpdp, P9_BFP_CONV_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xscvqpdpo*/	{0xFC140689, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "xscvqpdpo",(sim_fptr)&simulate_xscvqpdpo,P9_BFP_CONV_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xscvqpsdz*/	{0xFC190688, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "xscvqpsdz",(sim_fptr)&simulate_xscvqpsdz,P9_BFP_CONV_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xscvqpswz*/	{0xFC090688, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "xscvqpswz",(sim_fptr)&simulate_xscvqpswz,P9_BFP_CONV_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xscvqpudz*/	{0xFC110688, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "xscvqpudz",(sim_fptr)&simulate_xscvqpudz,P9_BFP_CONV_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xscvqpuwz*/	{0xFC010688, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "xscvqpuwz",(sim_fptr)&simulate_xscvqpuwz,P9_BFP_CONV_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xscvsdqp	*/	{0xFC0A0688, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xscvsdqp", (sim_fptr)&simulate_xscvsdqp, P9_BFP_CONV_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xscvudqp	*/	{0xFC020688, 0, DUMMY, DUMMY, BFP_DP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xscvudqp", (sim_fptr)&simulate_xscvudqp, P9_BFP_CONV_ONLY, X_FORM_RT_RA_RB_eop_rc},

/* xsxexpqp	*/	{0xFC020648, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "xsxexpqp",(sim_fptr)&simulate_xsxexpqp, P9_BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xsxsigqp	*/	{0xFC120648, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsxsigqp",(sim_fptr)&simulate_xsxsigqp, P9_BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* xsiexpqp	*/	{0xFC0006C8, 0, BFP_QP, 16, BFP_DP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14,   "xsiexpqp",(sim_fptr)&simulate_xsiexpqp, P9_BFP_MOVE_ONLY, X_FORM_RT_RA_RB_eop_rc},

/* xsmaddqp	*/	{0xFC000308, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsmaddqp", (sim_fptr)&simulate_xsmaddqp, P9_BFP_ADD_MUL_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsmaddqpo*/	{0xFC000309, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsmaddqpo",(sim_fptr)&simulate_xsmaddqpo,P9_BFP_ADD_MUL_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsmsubqp	*/	{0xFC000348, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsmsubqp", (sim_fptr)&simulate_xsmsubqp, P9_BFP_ADD_MUL_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsmsubqpo*/	{0xFC000349, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsmsubqpo",(sim_fptr)&simulate_xsmsubqpo,P9_BFP_ADD_MUL_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsnmaddqp*/	{0xFC000388, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsnmaddqp",(sim_fptr)&simulate_xsnmaddqp,P9_BFP_ADD_MUL_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsnmaddqpo*/	{0xFC000389, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14,"xsnmaddqpo",(sim_fptr)&simulate_xsnmaddqpo,P9_BFP_ADD_MUL_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsnmsubqp*/	{0xFC0003C8, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14, "xsnmsubqp",(sim_fptr)&simulate_xsnmsubqp,P9_BFP_ADD_MUL_QP, X_FORM_RT_RA_RB_eop_rc},
/* xsnmsubqpo*/	{0xFC0003C9, 0, BFP_QP, 16, BFP_QP, 11, DUMMY, DUMMY, BFP_QP, 21, 0x14,"xsnmsubqpo",(sim_fptr)&simulate_xsnmsubqpo,P9_BFP_ADD_MUL_QP, X_FORM_RT_RA_RB_eop_rc},

/* xsrqpi	*/	{0xFC00000A, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x1A, "xsrqpi",  (sim_fptr)&simulate_xsrqpi, P9_BFP_ROUND_2_INT, Z_FORM_RT_R_RB_RMC_eop_rc},
/* xsrqpix	*/	{0xFC00000B, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x1A, "xsrqpix", (sim_fptr)&simulate_xsrqpix,P9_BFP_ROUND_2_INT, Z_FORM_RT_R_RB_RMC_eop_rc},
/* xsrqpxp	*/	{0xFC00004A, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x1A, "xsrqpxp", (sim_fptr)&simulate_xsrqpxp,P9_BFP_ROUND_2_INT, Z_FORM_RT_R_RB_RMC_eop_rc},

/* xssqrtqp	*/	{0xFC1B0648, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "xssqrtqp", (sim_fptr)&simulate_xssqrtqp, P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},
/* xssqrtqpo*/	{0xFC1B0649, 0, DUMMY, DUMMY, BFP_QP, 11, DUMMY, DUMMY, BFP_DP, 21, 0x14, "xssqrtqpo",(sim_fptr)&simulate_xssqrtqpo,P9_BFP_ELEM_ARITH_QP, X_FORM_RT_RA_RB_eop_rc},

/* xststdcqp*/	{0xFC000588, 0,IMM_DATA_7BIT, 16, BFP_QP,11, DUMMY,DUMMY, CR_T, 23, 0x1B,"xststdcqp",(sim_fptr)&simulate_xststdcqp,P9_BFP_TEST_ONLY,X_FORM_BF_DCMX_vrb_eop_rc},

/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}

};

struct instruction_masks bfp_p9_dd2_instructions_array[] = {
	/* mffsce */ 	{0xFC01048E, 0, DUMMY, DUMMY, DUMMY, DUMMY,DUMMY, DUMMY, BFP_DP, 21, 0x14, "mffsce", &simulate_mffsce, P9_BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* mffscdrn */ 	{0xFC14048E, 0, BFP_DP, 11, DUMMY, DUMMY,DUMMY, DUMMY, BFP_DP, 21, 0x14, "mffscdrn", &simulate_mffscdrn, P9_BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* mffscdrni */ {0xFC15048E, 0, IMM_DATA_3BIT, 11, DUMMY, DUMMY,DUMMY, DUMMY, BFP_DP, 21, 0x14, "mffscdrni", &simulate_mffscdrni, P9_BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* mffscrn */ 	{0xFC16048E, 0, BFP_DP, 11, DUMMY, DUMMY,DUMMY, DUMMY, BFP_DP, 21, 0x14, "mffscrn", &simulate_mffscrn, P9_BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* mffscrni */ 	{0xFC17048E, 0, IMM_DATA_2BIT, 11, DUMMY, DUMMY,DUMMY, DUMMY, BFP_DP, 21, 0x14, "mffscrni", &simulate_mffscrni, P9_BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* mffsl */ 	{0xFC18048E, 0, DUMMY, DUMMY, DUMMY, DUMMY,DUMMY, DUMMY, BFP_DP, 21, 0x14, "mffsl", &simulate_mffsl, P9_BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
	/* last ins indicator */ {0xDEADBEEF, 0, DUMMY,  0, DUMMY, 0, DUMMY, DUMMY, DUMMY,  0, 0x0, "last_instruction", 0, 0, 0}
};


extern uint32 vsr_reg_wt[];
uint32 bfp_reg_wt[3] = {0, 5, 5};

void class_bfp_normal_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr1, vsr2, vsr3, vsr4;
	uint32 op1 = 0, op2 = 0, op3 = 0, tgt = 0;
	uint32 mcode, store_mcode, store_off, prolog_size, num_ins_built, *tc_memory;
	uint64 dirty_reg_mask;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];


	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	if (temp->op1_dtype == IMM_DATA_3BIT) {
		op1 = (0x00000007) & (random_no % 7);
	}
	else if (temp->op1_dtype == IMM_DATA_2BIT) {
		op1 = (0x00000003) & (random_no % 3);
	}
	else {
		vsrs = &(cptr->vsrs[temp->op1_dtype]);
		vsr1 = vsrs->head[BFP]->vsr_no;
		MOVE_VSR_TO_END(client_no, temp->op1_dtype, BFP); /* Move this to the end of list */
		op1 = (vsr1 & 0x1f); 
	#ifdef REUSE
		vsrs->dirty_mask &= (~(0x1ULL << vsr1));
	#endif
	}
	op1 = op1 << (temp->op1_pos);

	vsrs = &cptr->vsrs[temp->op2_dtype];
	vsr2 = vsrs->head[BFP]->vsr_no;
	MOVE_VSR_TO_END(client_no, temp->op2_dtype, BFP); /* Move this to the end of list */
	op2 = (vsr2 & 0x1f) << (temp->op2_pos);
#ifdef REUSE
	vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif

	vsrs = &cptr->vsrs[temp->op3_dtype];
	vsr3 = vsrs->head[BFP]->vsr_no;
	MOVE_VSR_TO_END(client_no, temp->op3_dtype, BFP); /* Move this to the end of list */
	op3 = (vsr3 & 0x1f) << (temp->op3_pos);
#ifdef REUSE
	vsrs->dirty_mask &= (~(0x1ULL << vsr3));
#endif

	vsrs = &cptr->vsrs[temp->tgt_dtype];
	vsr4 = vsrs->head[BFP]->vsr_no;
	tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

	mcode = (temp->op_eop_mask | op1 | op2 | op3 | tgt);

	dirty_reg_mask = vsrs->dirty_mask;

	if ( (0x1ULL << vsr4) & dirty_reg_mask) {
		store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
		store_off &= 0x0000ffff;
		store_mcode = STORE_BFP_DP(vsr4, store_off);

		cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
		*tc_memory = store_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)simulate_stfd;
		tc_memory++;
		num_ins_built++;
	}

	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
	tc_memory++;
	num_ins_built++;

	vsrs->dirty_mask |= (0x1ULL << vsr4);
	cptr->num_ins_built = num_ins_built;
	/* DPRINT(log, "\nIns Built in %s is = %lx, %s, sim_ptr=%lx\n", __FUNCTION__,mcode,temp->ins_name,temp->sim_func); */
}

void class_bfp_load_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4;
	uint32 op1, op2, tgt;
	uint32 mcode, store_mcode, store_off, addi_mcode, load_off;
	uint32 num_ins_built, prolog_size, *tc_memory;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	struct vsr_list *vsrs;

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	vsrs = &cptr->vsrs[temp->tgt_dtype];
	vsr4 = vsrs->head[BFP]->vsr_no;

	if ((0x1ULL << vsr4) & vsrs->dirty_mask) {
		store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
		store_off &= 0x0000ffff;
		store_mcode = STORE_BFP_DP(vsr4, store_off);
		*tc_memory = store_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
		tc_memory++;
		num_ins_built++;
	}

	tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

	/* This function will return the offset of memory and populate it with data */
	load_off = init_mem_for_vsx_load(client_no, temp->tgt_dtype);

	addi_mcode = GEN_ADDI_MCODE(LOAD_RB, 0, load_off);
	*tc_memory = addi_mcode;
	cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
	tc_memory++;
	num_ins_built++;

	op1 = (LOAD_RA & 0x1f) << (temp->op1_pos);
	op2 = (LOAD_RB & 0x1f) << (temp->op2_pos);
	mcode = (temp->op_eop_mask | op1 | op2 | tgt);

	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
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

	vsrs->dirty_mask |= (0x1ULL << vsr4);
	cptr->num_ins_built = num_ins_built;
	/*
	DPRINT(log, "\nInstruction Built in %s is = %lx", __FUNCTION__, mcode);
	DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name);
	*/
}

void class_bfp_load_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4;
	uint32 op1, tgt;
	uint32 mcode, store_mcode, store_off, load_off;
	uint32 prolog_size, num_ins_built, *tc_memory;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct vsr_list *vsrs;
	struct client_data *cptr = sdata->cdata_ptr[client_no];


	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	vsrs = &cptr->vsrs[temp->tgt_dtype];
	vsr4 = vsrs->head[BFP]->vsr_no;

	if ( (0x1ULL << vsr4) & vsrs->dirty_mask) {
		store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
		store_off &= 0x0000ffff;
		store_mcode = STORE_BFP_DP(vsr4, store_off);
		cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
		*tc_memory = store_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
		tc_memory++;
		num_ins_built++;
	}
	tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

	/* This function will return the offset of memory and populate it with data */
	load_off = init_mem_for_vsx_load(client_no, temp->tgt_dtype);
	load_off &= 0x0000ffff;

	op1 = (LOAD_RA & 0x1f) << (temp->op1_pos);

	mcode = (temp->op_eop_mask | op1 | tgt | load_off);

	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
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

	vsrs->dirty_mask |= (0x1ULL << vsr4);
	cptr->num_ins_built = num_ins_built;
	/* DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name); */
}

void class_bfp_store_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4;
	uint32 op1, op2, tgt;
	uint32 mcode, store_off, addi_mcode;
	uint32 prolog_size, num_ins_built, *tc_memory;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	vsrs = &cptr->vsrs[temp->tgt_dtype];
	vsr4 = vsrs->head[BFP]->vsr_no;
	MOVE_VSR_TO_END(client_no, temp->tgt_dtype, BFP);
	vsrs->dirty_mask &= (~(0x1ULL << vsr4));

	store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
	addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
	*tc_memory = addi_mcode;
	cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
	tc_memory++;
	num_ins_built++;

	op1 = (STORE_RA & 0x1f) << (temp->op1_pos);
	op2 = (STORE_RB & 0x1f) << (temp->op2_pos);
	tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

	mcode = (temp->op_eop_mask | op1 | op2 | tgt);

	/* save offset */
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
	tc_memory++;
	num_ins_built++;

	/* Check if its a store update */
	if(temp->op1_dtype == GRU) {
		*tc_memory = OR(BASE_GPR, LOAD_RA);
		cptr->instr_index[prolog_size + num_ins_built] =  or |0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_or;
		tc_memory++;
		num_ins_built++;
	}

	cptr->num_ins_built = num_ins_built;
	/* DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name); */
}

void class_bfp_store_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4;
	uint32 op1, tgt;
	uint32 mcode, store_off;
	uint32 prolog_size, num_ins_built, *tc_memory;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	vsrs = &cptr->vsrs[temp->tgt_dtype];
	vsr4 = vsrs->head[BFP]->vsr_no;
	MOVE_VSR_TO_END(client_no, temp->tgt_dtype, BFP);
	vsrs->dirty_mask &= (~(0x1ULL << vsr4));

	store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
	store_off &= 0x0000ffff;

	op1 = (STORE_RA & 0x1f) << (temp->op1_pos);
	tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

	mcode = (temp->op_eop_mask | op1 | tgt | store_off);

	/* save offset */
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
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

	cptr->num_ins_built = num_ins_built;
	/* DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name); */
}

void class_bfp_fpscr_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr2;
	uint32 op2, tgt;
	uint32 mcode, prolog_size, num_ins_built, *tc_memory;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	vsrs = &cptr->vsrs[temp->op2_dtype];
	vsr2 = vsrs->head[BFP]->vsr_no;

	/* Following line is commented out becoz contents of this registers may not have been used */
	/* completely. Hence this register's content is still dirty.*/

	/* MOVE_VSR_TO_END(temp->op2_dtype); Move this to the end of list */

	op2 = (vsr2 & 0x1f) << (temp->op2_pos);

	/* This is not that actual target. This is just a mask deciding with nibble to be modified */
	/* in the FPSCR. Hence, we move this to the end so that we don't end picking same mask time */
	/* and again for different instances of these class of instructions */

	/* MOVE_VSR_TO_END(temp->tgt_dtype); Move this to the end of list */
	/* Last two enable nibbles are not being modified */
	tgt = (random_no & 0xfc) << (temp->tgt_pos);

	mcode = (temp->op_eop_mask | op2 | tgt);
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
	tc_memory++;
	num_ins_built++;

	/* Decide whether this is to be done or not */
	/* vsrs[temp->op2_dtype].dirty_mask |= (~(0x1ULL << vsr2)); */

	cptr->num_ins_built = num_ins_built;
	/* DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name); */
}

void class_bfp_fpscr_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4;
	uint32 op2, tgt;
	uint32 mcode, prolog_size, num_ins_built, *tc_memory;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	/* Following line is commented out becoz contents of this registers may not have been used */
	/* completely. Hence this register's content is still dirty.*/

	/* MOVE_VSR_TO_END(temp->op2_dtype); Move this to the end of list */
	/* two immediate data field, where op1 = W (1b) should always be 0, op2 = U(4b) */
	op2 = (random_no & 0xf) << (temp->op2_pos);

	vsrs = &cptr->vsrs[temp->tgt_dtype];
	vsr4 = vsrs->head[VSX]->vsr_no;

	/* This is not that actual target. This is just a mask deciding with nibble to be modified */
	/* in the FPSCR.Hence, moving this to the end so that we don't endup picking same mask time */
	/* and again for different instances of this class of instructions */

	/* MOVE_VSR_TO_END(temp->tgt_dtype); Move this to the end of list */
	/* FPSCR field 0:5 only should be touched hence vsr4 % 6 */

	tgt = (vsr4 % 6) << (temp->tgt_pos);

	mcode = (temp->op_eop_mask | op2 | tgt);
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
	tc_memory++;
	num_ins_built++;

	/* Decide whether this is to be done or not */
	/* vsrs[temp->op2_dtype]->dirty_mask |= (~(0x1ULL << vsr2)); */

	cptr->num_ins_built = num_ins_built;
	/* DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name); */
}

void class_bfp_cr_update_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
    uint32 vsr1, vsr2, vsr4;
    uint32 op1, op2, tgt;
    uint32 mcode, addi_mcode, store_off, prolog_size, num_ins_built, *tc_memory;
    uint64 dirty_reg_mask;
    struct vsr_list *vsrs;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];

    prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
    tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

    vsrs = &(cptr->vsrs[temp->op1_dtype]);
    vsr1 = vsrs->head[BFP]->vsr_no;
    MOVE_VSR_TO_END(client_no, temp->op1_dtype, BFP); /* Move this to the end of list */
    op1 = (vsr1 & 0x1f) << (temp->op1_pos);
#ifdef REUSE
    vsrs->dirty_mask &= (~(0x1ULL << vsr1));
#endif

    vsrs = &cptr->vsrs[temp->op2_dtype];
    vsr2 = vsrs->head[BFP]->vsr_no;
    MOVE_VSR_TO_END(client_no, temp->op2_dtype, BFP); /* Move this to the end of list */
    op2 = (vsr2 & 0x1f) << (temp->op2_pos);
#ifdef REUSE
    vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif

    vsrs = &cptr->vsrs[temp->tgt_dtype];
    vsr4 = vsrs->head[VSX]->vsr_no;
    MOVE_VSR_TO_END(client_no, temp->tgt_dtype, VSX);
	/* Move this to the end of list. This is an exceptional case where the target is getting moved to
	 * the end. This is because there is no instruction which uses CR as source. Hence, this needs to
	 * be shifted explicitly to the end here.
	 */
    tgt = ((vsr4 & 0x7) << (temp->tgt_pos));

    mcode = (temp->op_eop_mask | op1 | op2 | tgt);

    dirty_reg_mask = vsrs->dirty_mask;

    if ( (0x1ULL << vsr4) & dirty_reg_mask) {
		/*
		 * Generate store. To generate store, tgt_reg_no's data type should be known.
		 */

        *tc_memory = MFCR(GPR_TO_SAVE_CR);
     	cptr->instr_index[prolog_size + num_ins_built] = mfcr |0x20000000;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_mfcr;
        tc_memory++;
        num_ins_built++;

		/* Following 3 instructions are to mask the CR0 & CR1 fields of the CR register */
     	addi_mcode = ADDIS(0,0,0x00ff);
     	*tc_memory = addi_mcode;
     	cptr->instr_index[prolog_size + num_ins_built] = addis |0x20000000;
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
        /* save offset */
        cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stwx;
        tc_memory++;
        num_ins_built++;
		/*
		 * Clear dirty_mask for target reg
		 */
#if 0
		vsrs->dirty_mask &= (~(0x1ULL << vsr4));
#else
		vsrs->dirty_mask = 0;
#endif
    }

    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
    tc_memory++;
    num_ins_built++;

    vsrs->dirty_mask |= (0x1ULL << vsr4);
    cptr->num_ins_built = num_ins_built;
    /* DPRINT(log, "\nIns Built in %s is = %lx, %s, sim_ptr=%lx\n", __FUNCTION__,mcode,temp->ins_name,temp->sim_func); */
}

void class_bfp_fpscr_2_cr_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
    uint32 vsr4;
    uint32 op1, tgt;
    uint32 mcode, addi_mcode, store_off, prolog_size, num_ins_built, *tc_memory;
    uint64 dirty_reg_mask;
    struct vsr_list *vsrs;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];


    prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
 	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	op1 = ((random_no & 0x7) << temp->op1_pos);

	vsrs = &cptr->vsrs[temp->tgt_dtype];
	vsr4 = vsrs->head[VSX]->vsr_no;
	MOVE_VSR_TO_END(client_no, temp->tgt_dtype, VSX); /* Move this to the end of list */
	tgt = ((vsr4 & 0x7) << (temp->tgt_pos));

    mcode = (temp->op_eop_mask | op1 | tgt);

    dirty_reg_mask = vsrs->dirty_mask;

    if ( (0x1ULL << vsr4) & dirty_reg_mask) {
		/*
		 * Generate store. To generate store, tgt_reg_no's data type should be known.
		 */

        *tc_memory = MFCR(GPR_TO_SAVE_CR);
		cptr->instr_index[prolog_size + num_ins_built] = mfcr | 0x20000000;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_mfcr;
        tc_memory++;
        num_ins_built++;

     	/* Following 3 instructions are to mask the CR0 field of this register */
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
        /* save offset */
        cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stwx;
        tc_memory++;
        num_ins_built++;
		/*
		 * Clear dirty_mask for target reg
		 */
#if 0
		vsrs->dirty_mask &= (~(0x1ULL << vsr4));
#else
		vsrs->dirty_mask = 0;
#endif
    }

    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
    tc_memory++;
    num_ins_built++;

    vsrs->dirty_mask |= (0x1ULL << vsr4);
    cptr->num_ins_built = num_ins_built;
    /* DPRINT(log, "\nIns Built in %s is = %lx, %s, sim_ptr=%lx\n", __FUNCTION__,mcode,temp->ins_name,temp->sim_func); */
}

void class_bfp_fpscr_bit_set_unset(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 tgt;
	uint32 mcode, prolog_size, num_ins_built, *tc_memory;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);


	/* This is not that actual target. This is just a mask deciding with nibble to be modified */
	/* in the FPSCR.Hence, moving this to the end so that we don't endup picking same mask time */
	/* and again for different instances of this class of instructions */

	/* MOVE_VSR_TO_END(temp->tgt_dtype); Move this to the end of list */
    /*  Choose any FPSCR bit from 0-23 only. Not altering 6th and 7th Enable nibbles */
	tgt = (random_no % 24) << (temp->tgt_pos);

	mcode = (temp->op_eop_mask | tgt);
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;

	tc_memory++;
	num_ins_built++;

	cptr->num_ins_built = num_ins_built;
	/* DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name); */
}

void class_bfp_qp_round_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	/* Used by xsrqpi, xsrqpix*/
	uint32 vsr2, vsr4;
	uint32 op2, tgt;
	uint32 prolog_size, num_ins_built, *tc_memory, store_off;
	uint32 rmode, rmask, mcode, addi_mcode;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	/* Get R and RMC bits(rmode) out of the random number received.
	 * Criteria : 001 and 010 are reserved. Rest everything is valid.
	 * In case if rmode comes out to be  reserved value, make R=1 so
	 * that it becomes valid.
	 */

	rmode = random_no & 0x7;	/* Get last 3 bits */
	if ( rmode == 0x1 || rmode == 0x2 ) {
		/* Handling reserved case */
		rmode |= 0x4; /* Set R=1 to make it valid */
	}

	rmask = ( ((rmode & 0x3) << 9) | ((rmode & 0x4) << 14) );

	vsrs = &cptr->vsrs[temp->op2_dtype];
	vsr2 = vsrs->head[BFP]->vsr_no;
	MOVE_VSR_TO_END(client_no, temp->op2_dtype, BFP); /* Move this to the end of list */
	op2 = (vsr2 & 0x1f) << (temp->op2_pos);
#ifdef REUSE
	vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif

	vsrs = &cptr->vsrs[temp->tgt_dtype];
	vsr4 = vsrs->head[BFP]->vsr_no;
	tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

	mcode = (temp->op_eop_mask | op2 | tgt | rmask );

	if ( (0x1ULL << tgt) & vsrs->dirty_mask) {
		store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
		addi_mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
		*tc_memory = addi_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
		tc_memory++;
		num_ins_built++;
		*tc_memory = STXVD2X(tgt,STORE_RA,STORE_RB);
		cptr->instr_index[prolog_size + num_ins_built] = stxvd2x | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stxvd2x;
		tc_memory++;
		num_ins_built++;
	}
	
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;	
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)temp->sim_func;
	tc_memory++;
	num_ins_built++;

	vsrs->dirty_mask |= (0x1ULL << vsr4);
	cptr->num_ins_built = num_ins_built;
}

void class_bfp_qp_test_data_class_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	/* Used for xststdcqp */
	uint32 vsr1, vsr2, vsr4;
	uint32 op1, op2, tgt;
	uint32 prolog_size, num_ins_built, *tc_memory;
	uint32 mcode, addi_mcode, store_off;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	if ( temp->op1_dtype == IMM_DATA_7BIT ) {
		vsr1 = random_no & 0x7F;	/* Collect 7 random bits. */
	}
	else {
		vsrs = &(cptr->vsrs[temp->op1_dtype]);
		vsr1 = vsrs->head[BFP]->vsr_no;
	}
	op1 = vsr1 << temp->op1_pos;

	vsrs = &(sdata->cdata_ptr[client_no]->vsrs[temp->op2_dtype]);
	vsr2 = vsrs->head[BFP]->vsr_no;
	MOVE_VSR_TO_END(client_no, temp->op2_dtype, VSX);
	op2 = (vsr2 & 0x1f) << (temp->op2_pos);

	vsrs = &(cptr->vsrs[temp->tgt_dtype]);
	vsr4 = vsrs->head[BFP]->vsr_no;
	tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

	mcode = temp->op_eop_mask | op1 | op2 | tgt;

	if ( ((0x1ULL << tgt) & vsrs->dirty_mask) && (temp->tgt_dtype == CR_T) ) {
		*tc_memory = MFCR(GPR_TO_SAVE_CR);
		cptr->instr_index[prolog_size + num_ins_built] = mfcr |0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_mfcr;
		tc_memory++;
		num_ins_built++;

		/* Following 3 instructions are to mask the CR0 & CR1 fields of the CR register */
		addi_mcode = ADDIS(0,0,0x00ff);
		*tc_memory = addi_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = addis |0x20000000;
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
		/* save offset */
		cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stwx;
		tc_memory++;
		num_ins_built++;

		vsrs->dirty_mask = 0;
	}


	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
	tc_memory++;
	num_ins_built++;

	vsrs->dirty_mask |= (0x1ULL << vsr4);
	cptr->num_ins_built = num_ins_built;
}

