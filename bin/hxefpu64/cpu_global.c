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

/*static char sccid[] = "%Z%%M%	%I%  %W% %G% %U%";*/

#include "framework.h" 

extern uint32 shifted_pvr_os;
extern struct htx_data hd;
extern char msg[1000];

/* ATOMIC OPERATION */
#define NUM_LD_FC	(13)

#define LD_ADD				(0x0)
#define LD_XOR				(0x1)
#define	LD_OR  				(0x2) 
#define	LD_AND 				(0x3) 
#define	LD_MAX_UNSIGNED 	(0x4) 
#define	LD_MAX_SIGNED		(0x5) 
#define	LD_MIN_UNSIGNED 	(0x6) 
#define	LD_MIN_SIGNED		(0x7) 
#define	LD_SWAP 			(0x8)
#define COMPARE_N_SWAP_NEQ 	(0x10)
#define	INCREAMENT_BOUNDED 	(0x18)
#define	INCREAMENT_EQUAL 	(0x19)
#define DECREAMENT_BOUNDED	(0x1C)

#define NUM_ST_FC 	(9)

#define ST_ADD				LD_ADD
#define ST_XOR 				LD_XOR
#define	ST_OR   			LD_OR
#define	ST_AND  			LD_AND
#define	ST_MAX_UNSIGNED  	LD_MAX_UNSIGNED
#define	ST_MAX_SIGNED 		LD_MAX_SIGNED
#define	ST_MIN_UNSIGNED  	LD_MIN_UNSIGNED
#define	ST_MIN_SIGNED 		LD_MIN_SIGNED
#define ST_TWIN				INCREAMENT_BOUNDED

struct instruction_masks cpu_macro_array[]={
	/* lhl*/	{0x0, 0, DUMMY , DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, CLASS_LHL, "lhl", DUMMY, MACRO_LHL, DUMMY},
	/* shl*/	{0x0, 0, DUMMY , DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, CLASS_SHL, "shl", DUMMY, MACRO_SHL, DUMMY},
	/* bc+8 */  {0x40000008, 0, IMM_DATA , 21 , IMM_DATA, 16 , DUMMY, DUMMY,  IMM_DATA, 2  , 0x66, "bc",   DUMMY, MACRO_BC8, B_FORM_BO_BI_BD_AA_LK},    
	/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};

struct instruction_masks cpu_p9_instructions_array[]={
	/**************************RFC02461.r3: Hashing Support Operations************************************/
    /* modsw */	{0x7C000616, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "modsw"  , DUMMY, P9_CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    /* moduw */	{0x7C000216, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "moduw"  , DUMMY, P9_CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    /* modsd */	{0x7C000612, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "modsd"  , DUMMY, P9_CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    /* modud */	{0x7C000212, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "modud"  , DUMMY, P9_CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    /* cnttzw */{0x7C000434, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x44, "cnttzw"  , DUMMY, P9_CPU_FIXED_LOGIC, X_FORM_RT_RA_RB_OE_eop_rc},
    /* cnttzw. */{0x7C000434, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x44, "cnttzw."  , DUMMY, P9_CPU_FIXED_LOGIC, X_FORM_RT_RA_RB_OE_eop_rc},
    /* cnttzd */{0x7C000474, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x44, "cnttzd"  , DUMMY, P9_CPU_FIXED_LOGIC, X_FORM_RT_RA_RB_OE_eop_rc},
    /* cnttzd. */{0x7C000474, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x44, "cnttzd."  , DUMMY, P9_CPU_FIXED_LOGIC, X_FORM_RT_RA_RB_OE_eop_rc},
	/**************************RFC02476.r4: PC-Relative Load/Store Addressing************************************/
	/* addpcis */{0x4C000004, 0, IMM_DATA, 16, IMM_DATA, 6, IMM_DATA, DUMMY, GR, 21, CLASS_CPU_LOAD_RELATIVE, "addpcis", DUMMY, P9_CPU_FIXED_ARTH, DX_FORM_RT_D1_D0_D2},
	/**************************RFC02466.r6: String Operations (FXU option)************************************/
    /* setb */{0x7C000100, 0, IMM_DATA, 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, CLASS_CPU_STRING_OPS_FX, "setb", DUMMY, P9_CPU_FIXED_LOGIC, X_FORM_RT_BFA},
    /* cmprb */{0x7C000180, 0, IMM_DATA_1BIT, 22, GR, 16, GR, 11, CR_T, 23, CLASS_CPU_STRING_OPS_FX, "cmprb", DUMMY, P9_CPU_FIXED_LOGIC, X_FORM_BF_L_RA_RB},
    /* cmpeqb */{0x7C0001C0, 0, GR, 16, GR, 11, DUMMY, DUMMY, CR_T, 23, CLASS_CPU_STRING_OPS_FX, "cmpeqb", DUMMY, P9_CPU_FIXED_LOGIC, X_FORM_BF_L_RA_RB},
	/**************************RFC02494.r1: FXU Multiply-Add Instructions************************************/
	/* maddhd */ {0x7C0001D2, 0, GR , 16, GR, 11, GR, 6, GR, 21, CLASS_CPU_MUL_ADD_DW, "maddhd" , DUMMY, P9_CPU_FIXED_ARTH, A_FORM_RT_RA_RB_RC_eop_rc},
	/* maddhdu */{0x7C0001D2, 0, GR , 16, GR, 11, GR, 6, GR, 21, CLASS_CPU_MUL_ADD_DW, "maddhdu", DUMMY, P9_CPU_FIXED_ARTH, A_FORM_RT_RA_RB_RC_eop_rc},
	/* maddld */ {0x7C0001D2, 0, GR , 16, GR, 11, GR, 6, GR, 21, CLASS_CPU_MUL_ADD_DW, "maddld" , DUMMY, P9_CPU_FIXED_ARTH, A_FORM_RT_RA_RB_RC_eop_rc},
	/**************************RFC02497.r1: Array Index Support************************************/
	/* extswsli  */ {0x7C0006F4, 0, GR, 21, IMM_DATA, 11, IMM_DATA_1BIT, 1, GR, 16, 0x57, "extswsli",  DUMMY, P9_CPU_FIXED_ARTH, MDFORM_RS_RA_SH_MB_rc},
	/* extswsli. */ {0x7C0006F5, 0, GR, 21, IMM_DATA, 11, IMM_DATA_1BIT, 1, GR, 16, 0x57, "extswsli.", DUMMY, P9_CPU_FIXED_ARTH, MDFORM_RS_RA_SH_MB_rc},

  /* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};

struct instruction_masks cpu_p9_dd2_instructions_array[]={
	/*********************RFC02466B Set Boolean Extension**********************************/
	/* setbool */{0x7C000101, 0, IMM_DATA, 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, CLASS_CPU_STRING_OPS_FX, "setbool", DUMMY, P9_CPU_FIXED_LOGIC, X_FORM_RT_BFA},
	/***************************RFC02511 - FXU & Vector Extensions for Blockchain Support**************************************/
	/* addex */ {0x7C000154, 0, GR, 16, GR, 11, IMM_DATA_2BIT, 10, GR, 21, 0x42, "addex",  DUMMY, P9_CPU_FIXED_ARTH, X_FORM_RT_RA_RB_CY_eop},
	/* brh */	{0x7C0001B6, 0, GR, 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "brh" , DUMMY, P9_CPU_FIXED_ARTH, X_FORM_RS_RA_eop},
	/* brd */	{0x7C000176, 0, GR, 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "brd" , DUMMY, P9_CPU_FIXED_ARTH, X_FORM_RS_RA_eop},
	/* brw */	{0x7C000136, 0, GR, 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "brw" , DUMMY, P9_CPU_FIXED_ARTH, X_FORM_RS_RA_eop},
	/**************************RFC02517 SHA-3 Assist Instructions***********************************************************************/
	/* xor3 */   {0x10000036, 0, GR , 21, GR, 11, GR, 6, GR, 16, 0x44, "xor3", DUMMY, P9_CPU_FIXED_LOGIC, VA_FORM_RS_RA_RB_RC_eop},
	/* andcxor */{0x10000037, 0, GR , 21, GR, 11, GR, 6, GR, 16, 0x44, "andcxor", DUMMY, P9_CPU_FIXED_LOGIC, VA_FORM_RS_RA_RB_RC_eop},
	/* rxor */   {0xC000017C, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "rxor", DUMMY, P9_CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/**************************RFC02491 Load Doubleword Monitored Instruction************************************/
	/* ldmx */{0x7C00026A, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "ldmx", DUMMY, P9_CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_EH},
	/***************************RFC 2485.r3: Load Atomic Near Memory**************************************/
  	/* lwat */ {0x7C00048C, 0, GR, 16, IMM_DATA_5BIT, 11, DUMMY, DUMMY, GR, 21, CLASS_CPU_LOAD_ATOMIC, "lwat",  DUMMY, P9_CPU_ATOMIC_LOAD, X_FORM_RS_RA_NB_eop},
  	/* stwat*/ {0x7C00058C, 0, GR, 16, IMM_DATA_5BIT, 11, DUMMY, DUMMY, GR, 21, CLASS_CPU_STORE_ATOMIC, "stwat", DUMMY, P9_CPU_ATOMIC_STORE,  X_FORM_RS_RA_FC_eop},
  	/* ldat */ {0x7C0004CC, 0, GR, 16, IMM_DATA_5BIT, 11, DUMMY, DUMMY, GR, 21, CLASS_CPU_LOAD_ATOMIC, "ldat",  DUMMY, P9_CPU_ATOMIC_LOAD, X_FORM_RS_RA_NB_eop},
  	/* stdat*/ {0x7C0005CC, 0, GR, 16, IMM_DATA_5BIT, 11, DUMMY, DUMMY, GR, 21, CLASS_CPU_STORE_ATOMIC, "stdat", DUMMY, P9_CPU_ATOMIC_STORE, X_FORM_RS_RA_FC_eop},

  	/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};

struct instruction_masks cpu_p8_instructions_array[]={
	/* lqarx*/	{0x7C000228, 0, GR , 16, GR, 11, IMM_DATA_1BIT, 0, GR, 21, 0x64, "lqarx", DUMMY, CPU_ATOMIC_LOAD, X_FORM_RT_RA_RB_eop_EH},
	/* stqcx.*/	{0x7C00016D, 0, GR , 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stqcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
	/* lharx*/	{0x7C0000E8, 0, GR , 16, GR, 11, IMM_DATA_1BIT, 0, GR, 21, 0x64, "lharx", DUMMY, CPU_ATOMIC_LOAD, X_FORM_RT_RA_RB_eop_EH},
	/* sthcx.*/	{0x7C0005AD, 0, GR , 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "sthcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
	/* lbarx*/	{0x7C000068, 0, GR , 16, GR, 11, IMM_DATA_1BIT, 0, GR, 21, 0x64, "lbarx", DUMMY, CPU_ATOMIC_LOAD, X_FORM_RT_RA_RB_eop_EH},
	/* stbcx.*/	{0x7C00056D, 0, GR , 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stbcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
    /* load instructions D_FORM_RT_RA_DQ_PT */
	#ifndef SCTU
   	/* lq */ 	{0xE0000000, 0, IMM_DATA , 4 , GR, 16, IMM_DATA_4BIT , 0 , GR , 21 , 0x61, "lq",  DUMMY, CPU_FIXED_LOAD, D_FORM_RT_RA_DQ_PT},
	/* stq */ 	{0xF8000002, 0, IMM_DATA , 2 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "stq" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_DS},
    /* icbt*/   {0x7C00002C, 0, IMM_DATA_1BIT, 25, IMM_DATA_3BIT , 21 , GR , 16, GR , 11 , 0x62, "icbt", DUMMY , CPU_CACHE, X_FORM_CT_RA_RB_eop},
	#endif
	/* bctar   	{0x4C000460, 0, IMM_DATA , 21 , IMM_DATA, 16 , DUMMY, DUMMY,  IMM_DATA, 11  , 0x60, "bctar",   DUMMY, CPU_BRANCH, XL_FORM_BO_BI_LK},  */ 
	/* bctarl  	{0x4C000461, 0, IMM_DATA , 21 , IMM_DATA, 16 , DUMMY, DUMMY,  IMM_DATA, 11  , 0x60, "bctarl",   DUMMY, CPU_BRANCH, XL_FORM_BO_BI_LK},  */ 
    /* makeitso   {0x7C000064, 0, DUMMY, DUMMY ,DUMMY ,DUMMY , DUMMY, DUMMY, DUMMY , DUMMY  , 0x58, "makeitso",  DUMMY, CPU_CACHE, XL_FORM},   */ 
   	/* or 26,26,26(makeitso) */ {0x7F5AD378, 0, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, 0x58, "or", DUMMY, CPU_FIXED_STORE|CPU_STORAGE, X_FORM_Rx_Rx_Rx_eop_rc},
	/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};
struct instruction_masks cpu_p7_instructions_array[]={
	/* popcntw   */	{0x7C0002F4, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "popcntw" , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop},
	/* popcntd   */	{0x7C0003F4, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "popcntd" , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop},
    	/*bperm */ 	{0x7C0001f8, 0, GR , 11, GR, 16, DUMMY , DUMMY , GR , 21, 0x55, "bperm" ,  DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop},
    	/* ldbrx*/	{0x7C000428, 0, GR , 16, GR, 11, DUMMY,  DUMMY , GR, 21, 0x40, "ldbrx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* stdbrx*/	{0x7C000528, 0, GR , 11, GR, 16, DUMMY , DUMMY , GR , 21, 0x55, "stdbrx",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
    	/* divwe     */	{0x7C000356, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divwe "   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divwe.    */	{0x7C000357, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divwe."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divweo    */	{0x7C000756, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divweo"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divweo.   */	{0x7C000757, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divweo."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divweu    */	{0x7C000316, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divweu"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divweu.   */	{0x7C000317, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divweu."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divweuo   */	{0x7C000716, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divweuo"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divweuo.  */	{0x7C000717, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divweuo." , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divde     */	{0x7C000352, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divde "   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divde.    */	{0x7C000353, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divde."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdeo    */	{0x7C000752, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdeo"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdeo.   */	{0x7C000753, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdeo."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdeu    */	{0x7C000312, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdeu"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdeu.   */	{0x7C000313, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdeu."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdeuo   */	{0x7C000712, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdeuo"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdeuo.  */	{0x7C000713, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdeuo." , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* or        */	{0x7C000378, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x63, "or"   , DUMMY, CPU_THREAD_PRI, X_FORM_RS_RA_RB_eop_rc},

	/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};

struct instruction_masks cpu_instructions_array[]={
   	 /* lbzx*/	{0x7C0000AE, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lbzx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
	 /* lbzux*/	{0x7C0000EE, 0, GRU, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lbzux", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lhzx	*/	{0x7C00022E, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lhzx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lhzux*/	{0x7C00026E, 0, GRU, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lhzux", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lhax	*/	{0x7C0002AE, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lhax", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lhaux*/	{0x7C0002EE, 0, GRU, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lhaux", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lwzx	*/	{0x7C00002E, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lwzx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lwzux*/	{0x7C00006E, 0, GRU, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lwzux", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lwax	*/	{0x7C0002AA, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lwax", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lwaux*/	{0x7C0002EA, 0, GRU, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lwaux", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* ldx 	*/	{0x7C00002A, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "ldx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* ldux */	{0x7C00006A, 0, GRU, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "ldux", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lhbrx*/	{0x7C00062C, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lhbrx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
    	/* lwbrx*/	{0x7C00042C, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lwbrx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
	/* lwarx*/	{0x7C000028, 0, GR , 16, GR, 11, IMM_DATA_1BIT, 0, GR, 21, 0x64, "lwarx", DUMMY, CPU_ATOMIC_LOAD, X_FORM_RT_RA_RB_eop_EH},
	/* ldarx*/	{0x7C0000A8, 0, GR , 16, GR, 11, IMM_DATA_1BIT, 0, GR, 21, 0x64, "ldarx", DUMMY, CPU_ATOMIC_LOAD, X_FORM_RT_RA_RB_eop_EH},

	
	/* conditional logical register instructions */
    	/* crand*/	{0x4C000202, 0, CR_T , 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "crand", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
    	/* cror */      {0x4C000382, 0, CR_T , 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "cror", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
    	/* crxor */     {0x4C000182, 0, CR_T , 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "crxor", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
    	/* crnand */    {0x4C0001C2, 0, CR_T , 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "crnand", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
    	/* crnor  */    {0x4C000042, 0, CR_T , 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "crnor", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
    	/* creqv  */    {0x4C000242, 0, CR_T , 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "creqv", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
    	/* crandc */    {0x4C000102, 0, CR_T , 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "crandc", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
    	/* crorc  */    {0x4C000342, 0, CR_T , 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "crorc", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
    	/* mcrf */      {0x4C000000, 0, CR_T , 18, DUMMY, DUMMY , DUMMY, DUMMY, CR_T, 23, 0x41, "mcrf", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
	/* Fixed point arithmetic instructions */
	/* add     */	{0x7C000214, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "add"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
	/* add.    */	{0x7C000215, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "add."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* addo    */	{0x7C000614, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "addo"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* addo.   */	{0x7C000615, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "addo."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subf    */	{0x7C000050, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subf"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subf.   */	{0x7C000051, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subf."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfo   */	{0x7C000450, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfo"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfo.  */	{0x7C000451, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfo."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* addc    */	{0x7C000014, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "addc"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* addc.   */	{0x7C000015, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "addc."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* addco   */	{0x7C000414, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "addco"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* addco.   */	{0x7C000415, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "addco."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfc    */	{0x7C000010, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfc"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfc.    */	{0x7C000011, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfc."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfco    */	{0x7C000410, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfco"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfco.   */	{0x7C000411, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfco."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* adde    */	{0x7C000114, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "adde"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* adde.   */	{0x7C000115, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "adde."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* addeo   */	{0x7C000514, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "addeo"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* addeo.  */	{0x7C000515, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "addeo."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfe   */	{0x7C000110, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfe"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfe.  */	{0x7C000111, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfe."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfeo  */	{0x7C000510, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfeo"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* subfeo.  */	{0x7C000511, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "subfeo."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulld   */	{0x7C0001D2, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulld"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulld.  */	{0x7C0001D3, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulld."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulldo  */	{0x7C0005D2, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulldo"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulldo.  */	{0x7C0005D3, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulldo."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mullw   */	{0x7C0001D6, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mullw"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mullw.  */	{0x7C0001D7, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mullw."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mullwo  */	{0x7C0005D6, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mullwo"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mullwo.  */	{0x7C0005D7, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mullwo."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulhd   */	{0x7C000092, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulhd"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulhd.   */	{0x7C000093, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulhd."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulhw   */	{0x7C000096, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulhw"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulhw.   */	{0x7C000097, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulhw."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulhdu   */	{0x7C000012, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulhdu"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulhdu.   */	{0x7C000013, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulhdu."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulhwu   */	{0x7C000016, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulhwu"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* mulhwu.   */	{0x7C000017, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "mulhwu."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divw     */	{0x7C0003D6, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divw"    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divw.     */	{0x7C0003D7, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divw."    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divwo    */	{0x7C0007D6, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divwo"    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divwo.    */	{0x7C0007D7, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divwo."    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divd     */	{0x7C0003D2, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divd"    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divd.     */	{0x7C0003D3, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divd."    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdo     */	{0x7C0007D2, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdo"    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdo.    */	{0x7C0007D3, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdo."    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divwu    */	{0x7C000396, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divwu"    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divwu.   */	{0x7C000397, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divwu."    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divwuo   */	{0x7C000796, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divwuo"    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divwuo.  */	{0x7C000797, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divwuo."    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdu    */	{0x7C000392, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdu"    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divdu.    */	{0x7C000393, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divdu."    , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divduo    */	{0x7C000792, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divduo"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    	/* divduo.   */	{0x7C000793, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x42, "divduo."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_RB_OE_eop_rc},
    /* another form */	
	/* addme   */	{0x7C0001D4, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "addme"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* addme.   */	{0x7C0001D5, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "addme."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* addmeo   */	{0x7C0005D4, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "addmeo"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* addmeo.  */	{0x7C0005D5, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "addmeo."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* subfme   */	{0x7C0001D0, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "subfme"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* subfme.  */	{0x7C0001D1, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "subfme."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* subfmeo  */	{0x7C0005D0, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "subfmeo"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* subfmeo. */	{0x7C0005D1, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "subfmeo."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* addze   */	{0x7C000194, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "addze"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* addze.   */	{0x7C000195, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "addze."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* addzeo   */	{0x7C000594, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "addzeo"   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* addzeo.  */	{0x7C000595, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "addzeo."   , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* subfze  */	{0x7C000190, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "subfze"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* subfze. */	{0x7C000191, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "subfze."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* subfzeo */	{0x7C000590, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "subfzeo"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* subfzeo. */	{0x7C000591, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "subfzeo."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* neg      */	{0x7C0000D0, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "neg" , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* neg.      */	{0x7C0000D1, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "neg." , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* nego      */	{0x7C0004D0, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "nego" , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},
	/* nego.     */	{0x7C0004D1, 0, GR , 16, DUMMY, DUMMY, DUMMY, DUMMY, GR, 21, 0x42, "nego." , DUMMY, CPU_FIXED_ARTH, X_FORM_RT_RA_OE_eop_rc},


	/* dForm_RS_RA_UI format instructions */
	/* andi */    	{0x70000000, 0, GR , 21, IMM_DATA, 0 , DUMMY, DUMMY, GR, 16, 0x43, "andi", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
	/* andis*/    	{0x74000000, 0, GR , 21, IMM_DATA , 0, DUMMY, DUMMY, GR, 16, 0x43, "andis", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
	/* ori  */    	{0x60000000, 0, GR , 21, IMM_DATA,0 , DUMMY, DUMMY, GR, 16, 0x43, "ori", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
	/* nop  */    	{0x60000000, 0, DUMMY  , DUMMY,   DUMMY, DUMMY, DUMMY, DUMMY,DUMMY,DUMMY, 0x43, "nop", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
	/* oris */    	{0x64000000, 0, GR , 21, IMM_DATA,0 , DUMMY, DUMMY, GR, 16, 0x43, "oris", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
	/*xori */    	{0x68000000, 0, GR , 21, IMM_DATA,0 , DUMMY, DUMMY, GR, 16, 0x43, "xori", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
	/*xnop  */    	{0x60000000, 0, DUMMY  , DUMMY, DUMMY, DUMMY, DUMMY, DUMMY,DUMMY,DUMMY, 0x43, "xnop", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
	/*xoris */    	{0x6C000000, 0, GR , 21, IMM_DATA,0 , DUMMY, DUMMY, GR, 16, 0x43, "xoris", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
	/* xForm_RS_RA_RB_Rc form of instructions */	
	/* and       */	{0x7C000038, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "and"  , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* and.      */	{0x7C000039, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "and."  , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* or        */	{0x7C000378, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "or"   , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* or.       */	{0x7C000379, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "or."   , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* xor       */	{0x7C000278, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "xor"   , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* xor.      */	{0x7C000279, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "xor."   , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* nand      */	{0x7C0003B8, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "nand"   , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* nand.     */	{0x7C0003B9, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "nand."   , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* nor       */	{0x7C0000F8, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "nor"  , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* nor.      */	{0x7C0000F9, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "nor."  , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* eqv       */	{0x7C000238, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "eqv"  , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* eqv.      */	{0x7C000239, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "eqv."  , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* andc      */	{0x7C000078, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "andc"  , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* andc.     */	{0x7C000079, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "andc."  , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/*  orc      */	{0x7C000338, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "orc"   , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/*  orc.     */	{0x7C000339, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "orc."  , DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},

	/*  sld      */	{0x7C000036, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "sld"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  sld.     */	{0x7C000037, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "sld."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  slw      */	{0x7C000030, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "slw"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  slw.     */	{0x7C000031, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "slw."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  srd      */	{0x7C000436, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "srd"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  srd.     */	{0x7C000437, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "srd."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  srw      */	{0x7C000430, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "srw"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  srw.     */	{0x7C000431, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "srw."  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  srad     */	{0x7C000634, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "srad"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  srad.    */	{0x7C000635, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "srad." , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  sraw     */	{0x7C000630, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "sraw"  , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},
	/*  sraw.    */	{0x7C000631, 0, GR , 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "sraw." , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_RB_eop_rc},

	/* X_FORM_RS_RA_RC type of instructions */
	/* extsb     */	{0x7C000774, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "extsb" , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop_rc},
	/* extsb.    */	{0x7C000775, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "extsb." , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop_rc},
	/* extsh     */	{0x7C000734, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "extsh" , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop_rc},
	/* extsh.    */	{0x7C000735, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "extsh." , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop_rc},
	/* extsw     */	{0x7C0007B4, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "extsw" , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop_rc},
	/* extsw.    */	{0x7C0007B5, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "extsw." , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop_rc},
	/* cntlzd    */	{0x7C000074, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "cntlzd" , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop_rc},
	/* cntlzd.   */	{0x7C000075, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "cntlzd." , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop_rc},
	
	/* X_FORM_RS_RA type of instructions prtyw and prtyd are p6 instructions */
	/* prtyw     */	{0x7C000134, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "prtyw" , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop},
	/* prtyd     */	{0x7C000174, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "prtyd" , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop},
	/* popcntb   */	{0x7C0000F4, 0, GR , 21, DUMMY, DUMMY, DUMMY, DUMMY, GR, 16, 0x45, "popcntb" , DUMMY, CPU_FIXED_ARTH, X_FORM_RS_RA_eop},
	
	/* mdForm_RS_RA_sh_mb_Rc form of instructions */
	/* rldicl    */	{0x78000000, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x46, "rldicl" , DUMMY, CPU_FIXED_ROTATE, MDFORM_RS_RA_SH_MB_rc},
	/* rldicl.   */	{0x78000001, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x46, "rldicl." , DUMMY, CPU_FIXED_ROTATE, MDFORM_RS_RA_SH_MB_rc},
	/* rldicr   */	{0x78000004, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x46, "rldicr" , DUMMY, CPU_FIXED_ROTATE, MDFORM_RS_RA_SH_MB_rc},
	/* rldicr   */	{0x78000005, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x46, "rldicr." , DUMMY, CPU_FIXED_ROTATE, MDFORM_RS_RA_SH_MB_rc},
	/* rldic   */	{0x78000008, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x46, "rldic" , DUMMY, CPU_FIXED_ROTATE, MDFORM_RS_RA_SH_MB_rc},
	/* rldic.  */	{0x78000009, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x46, "rldic." , DUMMY, CPU_FIXED_ROTATE, MDFORM_RS_RA_SH_MB_rc},
	/* rldimi */	{0x7800000C, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x46, "rldimi" , DUMMY, CPU_FIXED_ROTATE, MDFORM_RS_RA_SH_MB_rc},
	/* rldimi. */	{0x7800000D, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x46, "rldimi." , DUMMY, CPU_FIXED_ROTATE, MDFORM_RS_RA_SH_MB_rc},

	/* mForm_RS_RA_sh_mb_me_Rc form of instructions */
	/* rlwinm    */	{0x54000000, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x47, "rlwinm" , DUMMY, CPU_FIXED_ROTATE, MFORM_RS_RA_SH_MB_ME_rc},
	/* rlwinm.   */	{0x54000001, 0, GR , 21, IMM_DATA,11, IMM_DATA, 6 , GR, 16, 0x47, "rlwinm." , DUMMY, CPU_FIXED_ROTATE, MFORM_RS_RA_SH_MB_ME_rc},
	
	/* mForm_RS_RA_RB_mb_me_Rc form of instructions */
	/* rlwnm    */	{0x5C000000, 0, GR , 21, GR ,11, IMM_DATA, 6 , GR, 16, 0x49, "rlwnm" , DUMMY, CPU_FIXED_ROTATE, MFORM_RS_RA_RB_MB_ME_rc},
	/* rlwnm.   */	{0x5C000001, 0, GR , 21, GR ,11, IMM_DATA, 6 , GR, 16, 0x49, "rlwnm." , DUMMY, CPU_FIXED_ROTATE, MFORM_RS_RA_RB_MB_ME_rc},
	
	/* mdsForm_RS_RA_RB_mb_Rc form of instructions */
	/* rldcl  */	{0x78000010, 0, GR , 21, GR ,11, IMM_DATA, 6 , GR, 16, 0x48, "rldcl" , DUMMY, CPU_FIXED_ROTATE, MDSFORM_RS_RA_RB_MB_eop_rc},
	/* rldcl. */	{0x78000011, 0, GR , 21, GR ,11, IMM_DATA, 6 , GR, 16, 0x48, "rldcl." , DUMMY, CPU_FIXED_ROTATE, MDSFORM_RS_RA_RB_MB_eop_rc},
	
	/* mdsForm_RS_RA_RB_me_Rc form of instructions */
	/* rldcr  */	{0x78000012, 0, GR , 21, GR ,11, IMM_DATA, 6 , GR, 16, 0x48, "rldcr" , DUMMY, CPU_FIXED_ROTATE, MDSFORM_RS_RA_RB_ME_eop_rc},
	/* rldcr. */	{0x78000013, 0, GR , 21, GR ,11, IMM_DATA, 6 , GR, 16, 0x48, "rldcr." , DUMMY, CPU_FIXED_ROTATE, MDSFORM_RS_RA_RB_ME_eop_rc},
	
	/* xsForm_RS_RA_SH_eop_Rc form of instructions */
	/* sradi  */	{0x7C000674, 0, GR , 21, IMM_DATA,11, DUMMY , DUMMY,GR, 16, 0x50, "sradi " , DUMMY, CPU_FIXED_ARTH ,  XS_FORM_RS_RA_SH_eop_rc},
	/* sradi.  */	{0x7C000675, 0, GR , 21, IMM_DATA,11, DUMMY , DUMMY,GR, 16, 0x50, "sradi. " , DUMMY, CPU_FIXED_ARTH ,  XS_FORM_RS_RA_SH_eop_rc},
	
	/* xsForm_RS_RA_SH_eop_Rc form of instructions */
	/* srawi  */	{0x7C000670, 0, GR , 21, IMM_DATA,11, DUMMY , DUMMY,GR, 16, 0x51, "srawi " , DUMMY, CPU_FIXED_ARTH ,  X_FORM_RS_RA_SH_eop_rc},
	/* srawi.  */	{0x7C000671, 0, GR , 21, IMM_DATA,11, DUMMY , DUMMY,GR, 16, 0x51, "srawi. " , DUMMY, CPU_FIXED_ARTH ,  X_FORM_RS_RA_SH_eop_rc},


	/* xfxForm_RS_spr form of instructions */
	/* mtspr  */	{0x7C0003A6, 0, GR , 21,DUMMY ,DUMMY ,DUMMY, DUMMY, SPR_REG, 16, 0x52, "mtspr " , DUMMY, CPU_FIXED_SPR , X_FX_FORM_RS_SPR_eop},
	/* xfxForm_RT_spr form of instructions */
	/* mfspr  */	{0x7C0002A6, 0, SPR_REG, 11, DUMMY ,DUMMY ,DUMMY, DUMMY, GR, 21, 0x53, "mfspr " , DUMMY, CPU_FIXED_SPR , X_FX_FORM_RT_SPR_eop},
	/* xfxForm_RS_fxm form of instructions */
	/* mtcrf  */	{0x7C000120, 0, GR , 21,DUMMY ,DUMMY ,DUMMY, DUMMY, IMM_DATA, 12, 0x54, "mtcrf " , DUMMY, CPU_FIXED_SPR , X_FX_FORM_RS_FXM_eop},
	/* store instructions X_FORM_RS_RA_RB */                       
	/* stbx */	{0x7C0001AE, 0, GR , 11, GR, 16, DUMMY, DUMMY , GR, 21 , 0x55, "stbx",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* stbux*/	{0x7C0001EE, 0, GR , 11, GRU, 16, DUMMY , DUMMY ,GR, 21,  0x55, "stbux",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* sthx */	{0x7C00032E, 0, GR , 11, GR, 16, DUMMY, DUMMY,  GR, 21 , 0x55, "sthx" ,  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* sthux*/	{0x7C00036E, 0, GR , 11, GRU, 16, DUMMY, DUMMY,  GR, 21,   0x55, "sthux",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* stwx */	{0x7C00012E, 0, GR , 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stwx" ,  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* stwux*/	{0x7C00016E, 0, GR , 11, GRU, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stwux",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* stdx */	{0x7C00012A, 0, GR , 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stdx" ,  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* stdux*/	{0x7C00016A, 0, GR , 11, GRU, 16, DUMMY , DUMMY , GR, 21,  0x55, "stdux",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* sthbrx*/	{0x7C00072C, 0, GR , 11, GR, 16, DUMMY , DUMMY , GR, 21, 0x55, "sthbrx",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* stwbrx*/	{0x7C00052C, 0, GR , 11, GR, 16, DUMMY , DUMMY , GR , 21, 0x55, "stwbrx",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
	/* cmpb */ 	{0x7C0003f8, 0, GR , 11, GR, 16, DUMMY , DUMMY , GR ,21, 0x55, "cmpb" ,  DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
	/* store instructions D_FORM_RS_RA_D */                       
	/* stb */	{0x98000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "stb",  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
	/* stbu */	{0x9C000000, 0, IMM_DATA , 0 , GRU, 16, DUMMY, DUMMY , GR , 21 , 0x56, "stbu",  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
	/* sth */	{0xB0000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "sth" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
	/* sthu */	{0xB4000000, 0, IMM_DATA , 0 , GRU, 16, DUMMY, DUMMY , GR , 21 , 0x56, "sthu" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
	/* stw */	{0x90000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "stw" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
	/* stwu */	{0x94000000, 0, IMM_DATA , 0 , GRU, 16, DUMMY, DUMMY , GR , 21 , 0x56, "stwu" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
	/* load instructions D_FORM_RT_RA_D */
	/* lbz */	{0x88000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lbz",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_D},          
	/* lbzu*/	{0x8C000000, 0, IMM_DATA , 0 , GRU, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lbzu",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_D},          
	/* lhz */	{0xA0000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lhz",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_D},          
	/* lhzu */	{0xA4000000, 0, IMM_DATA , 0 , GRU, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lhzu",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_D},          
	/* lwz */	{0x80000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lwz",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_D},          
	/* lwzu */	{0x84000000, 0, IMM_DATA , 0 , GRU, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lwzu",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_D},          
	/* lha */	{0xA8000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lha",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_D},          
	/* lhau*/	{0xAC000000, 0, IMM_DATA , 0 , GRU, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lhau",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_D},          
		#ifndef SCTU
	/* load instructions D_FORM_RT_RA_DS */
	/* lwa */	{0xE8000002, 0, IMM_DATA , 2 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lwa",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_DS},          
	/* ld */	{0xE8000000, 0, IMM_DATA , 2 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "ld",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_DS},          
	/* ldu */	{0xE8000001, 0, IMM_DATA , 2 , GRU, 16, DUMMY, DUMMY , GR , 21 , 0x56, "ldu",  DUMMY, CPU_FIXED_LOAD, D_FORM_RS_RA_DS},          
	/* store instructions D_FORM_RS_RA_DS */
	/* std */	{0xF8000000, 0, IMM_DATA , 2 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "std",  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_DS},          
	/* stdu*/	{0xF8000001, 0, IMM_DATA , 2 , GRU, 16, DUMMY, DUMMY , GR , 21 , 0x56, "stdu",  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_DS},          
		#endif
	/* arithmetic  instructions D_FORM_RT_RA_SI */
	/* addi */	{0x38000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x57, "addi",  DUMMY, CPU_FIXED_ARTH, D_FORM_RT_RA_SI},          
	/* addis*/	{0x3C000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x57, "addis",  DUMMY, CPU_FIXED_ARTH, D_FORM_RT_RA_SI},          
	/* addic*/	{0x30000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x57, "addic",  DUMMY, CPU_FIXED_ARTH, D_FORM_RT_RA_SI},          
	/* addic.*/	{0x34000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x57, "addic.",  DUMMY, CPU_FIXED_ARTH, D_FORM_RT_RA_SI},          
	/* subfic*/	{0x20000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x57, "subfic",  DUMMY, CPU_FIXED_ARTH, D_FORM_RT_RA_SI},          
	/*  mulli*/	{0x1C000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x57, "mulli",  DUMMY, CPU_FIXED_ARTH, D_FORM_RT_RA_SI},          
	/* xForm_RS_RA_RB_1 form */ 
	/* stwcx.*/	{0x7C00012D, 0, GR , 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stwcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
	/* stdcx.*/	{0x7C0001AD, 0, GR , 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stdcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
	/* dForm_BF_L_RA_SI form */
	/* cmpi */  	{0x2C000000, 0, IMM_DATA, 0, IMM_DATA_1BIT, 21 , GR, 16, CR_T , 23 , 0x5A, "cmpi",   DUMMY, CPU_FIXED_LOGIC, D_FORM_BF_L_RA_SI},            
	/* cmpli */ 	{0x28000000, 0, IMM_DATA, 0, IMM_DATA_1BIT, 21 , GR, 16, CR_T , 23 , 0x5A, "cmpli",   DUMMY, CPU_FIXED_LOGIC, D_FORM_BF_L_RA_SI},            
	/* xForm_BF_L_RA_RB form instructions.But will use the same build as cmpi */	
	/* cmp  */	{0x7C000000, 0, GR , 11, IMM_DATA_1BIT, 21 , GR, 16, CR_T , 23 , 0x5A, "cmp",   DUMMY, CPU_FIXED_LOGIC, X_FORM_BF_L_RA_RB},            
	/* cmpl */ 	{0x7C000040, 0, GR , 11, IMM_DATA_1BIT, 21 , GR, 16, CR_T , 23 , 0x5A, "cmpl",   DUMMY, CPU_FIXED_LOGIC, X_FORM_BF_L_RA_RB},            
	/* x_form_ra_rb type of instructions */
	#ifndef SCTU
	/* dcbz */ 	{0x7C0007EC, 0, GR , 16, GR, 11, DUMMY, DUMMY,  DUMMY, DUMMY , 0x5B, "dcbz",   DUMMY, CPU_CACHE, X_FORM_RA_RB},
	#endif
	/* dcbst*/ 	{0x7C00006C, 0, GR , 16, GR, 11, DUMMY, DUMMY,  DUMMY, DUMMY , 0x5B, "dcbst",   DUMMY, CPU_CACHE, X_FORM_RA_RB},          
	/* icbi */ 	{0x7C0007AC, 0, GR , 16, GR, 11, DUMMY, DUMMY,  DUMMY, DUMMY , 0x5B, "icbi",   DUMMY, CPU_CACHE, X_FORM_RA_RB},          
	/* x_form_l_ra_rb instruction */
	/* dcbf */	{0x7C0000AC, 0, GR , 16, GR, 11, DUMMY, DUMMY,  IMM_DATA_2BIT, 21 , 0x5C, "dcbf",   DUMMY, CPU_CACHE, X_FORM_L_RA_RB},            
	/*x_form_th_ra_rb_eop*/ 
	/* dcbt */	{0x7C00022C, 0, GR , 16, GR, 11, DUMMY, DUMMY,  IMM_DATA, 21 , 0x5D, "dcbt",   DUMMY, CPU_CACHE, X_FORM_TH_RA_RB},            
	/* dcbtst */	{0x7C0001EC, 0, GR , 16, GR, 11, DUMMY, DUMMY,  IMM_DATA, 21 , 0x5D, "dcbtst",   DUMMY, CPU_CACHE, X_FORM_TH_RA_RB},            

/*  iForm_LI_AA_LK form of instructions mainly branch instructions;only relative instructions is supported-AA bit is off */

	/* branch */	{0x48000000, 0, DUMMY , DUMMY, DUMMY, DUMMY, DUMMY, DUMMY,  IMM_DATA, 2  , 0x5E, "b",   DUMMY, CPU_BRANCH, I_FORM_LI_AA_LK},            
	/* branch */	{0x48000001, 0, DUMMY , DUMMY, DUMMY, DUMMY, DUMMY, DUMMY,  IMM_DATA, 2  , 0x5E, "bl",   DUMMY, CPU_BRANCH, I_FORM_LI_AA_LK},            
	/*  b_form_bo_bi_bd_AA_LK form of branch instructions ;only relative instructions are supported*/
	/* branch on condition */ {0x40000000, 0, IMM_DATA , 21 , IMM_DATA, 16 , DUMMY, DUMMY,  IMM_DATA, 2  , 0x5F, "bc",   DUMMY, CPU_BRANCH, B_FORM_BO_BI_BD_AA_LK},        
	/* branch on condition */ {0x40000001, 0, IMM_DATA , 21 , IMM_DATA, 16 , DUMMY, DUMMY,  IMM_DATA, 2  , 0x5F, "bcl",   DUMMY, CPU_BRANCH, B_FORM_BO_BI_BD_AA_LK},   
	/* xl_Form_BO_BI_LK_E*/
	/* bcctr */{0x4C000420, 0, IMM_DATA , 21 , IMM_DATA, 16 , DUMMY, DUMMY,  IMM_DATA, 11  , 0x60, "bcctr",   DUMMY, CPU_BRANCH, XL_FORM_BO_BI_LK},            
	/* bcctrl */{0x4C000421, 0, IMM_DATA , 21 , IMM_DATA, 16 , DUMMY, DUMMY,  IMM_DATA, 11  , 0x60, "bcctrl",   DUMMY, CPU_BRANCH, XL_FORM_BO_BI_LK},            
	/* bclr   */{0x4C000020, 0, IMM_DATA , 21 , IMM_DATA, 16 , DUMMY, DUMMY,  IMM_DATA, 11  , 0x60, "bclr",   DUMMY, CPU_BRANCH, XL_FORM_BO_BI_LK},            
	/* bclrl  */{0x4C000021, 0, IMM_DATA , 21 , IMM_DATA, 16 , DUMMY, DUMMY,  IMM_DATA, 11  , 0x60, "bclrl",   DUMMY, CPU_BRANCH, XL_FORM_BO_BI_LK},            
/* x_form */
	/* sync */	{0x7C0004AC, 0,IMM_DATA_2BIT, 21 ,IMM_DATA_4BIT ,16 , DUMMY, DUMMY, DUMMY , DUMMY  , 0x65, "sync",  DUMMY, CPU_CACHE, X_FORM_L_E},
	/* isync */	{0x4C00012C, 0,DUMMY, DUMMY ,DUMMY ,DUMMY , DUMMY, DUMMY, DUMMY , DUMMY , 0x58, "isync",  DUMMY, CPU_CACHE, XL_FORM},          
#if 0
/* priviliged instructions..not supported */
	/* eieio */	{0x7C0006AC, DUMMY, DUMMY ,DUMMY ,DUMMY , DUMMY, DUMMY, DUMMY , DUMMY , DUMMY , 0x58, "eieio",  DUMMY, CPU_EXTERNAL, X_FORM},          
	/* slbia */	{0x7C0003E4, DUMMY, DUMMY ,DUMMY ,DUMMY , DUMMY, DUMMY, DUMMY , DUMMY , DUMMY , 0x58, "slbia",  DUMMY, CPU_STORAGE, X_FORM},          
	/* tlbia */	{0x7C0002E4, DUMMY, DUMMY ,DUMMY ,DUMMY , DUMMY, DUMMY, DUMMY , DUMMY , DUMMY , 0x58, "tlbia",  DUMMY, CPU_STORAGE, X_FORM},          
	/* tlbsync */	{0x7C00046C, DUMMY, DUMMY ,DUMMY ,DUMMY , DUMMY, DUMMY, DUMMY , DUMMY , DUMMY , 0x58, "tlbsync",  DUMMY, CPU_STORAGE, X_FORM},          
/* warning - lmd and stmd should be enabled only when tag mode is ON */
	/* stmd */	{0xF8000003, 0, IMM_DATA , 2 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "stmd" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
	/* lmd */	{0xE8000003, 0, IMM_DATA , 2, GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lmd" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
#endif

#if !defined( __HTX_LE__) && !defined(SCTU)
	/* Following instructions are not supported in LE mode. */

	/* X_FORM_RS_RA_NB form.Both lswi and stswi have same build module */
	/* lswi  */	{0x7C0004AA, 0, IMM_DATA , 11  , GR, 16, DUMMY, DUMMY , GR , 21 , 0x59, "lswi",   DUMMY, CPU_FIXED_LOAD, X_FORM_RS_RA_NB_eop},          
	/* stswi */	{0x7C0005AA, 0, IMM_DATA , 11  , GR, 16, DUMMY, DUMMY , GR , 21 , 0x59, "stswi",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_NB_eop},          
	/* lswx */	{0x7C00042A, 0, GR , 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lswx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
	/* stswx*/	{0x7C00052A, 0, GR , 11, GR, 16, DUMMY , DUMMY , GR ,21, 0x55, "stswx",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},

	/* lmw */	{0xB8000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "lmw" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
	/* stmw */	{0xBC000000, 0, IMM_DATA , 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "stmw" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},          
#endif
	/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};

macro_load_store_class macro_lsc[4] = 
{ 
	{215, 87, lbzx, stbx},
	{407, 279, lhzx, sthx},
	{151, 23, lwzx, stwx},
	{149, 21, ldx, stdx}
};



void class_cpu_lhl_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	/* Build load followed by a load */
	uint32 gpr_t, gpr_s;
	uint32 offset;
	uint32 num_ins_built, prolog_size, *tc_memory;
	int local_rand;

	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	gpr_t = get_random_gpr(client_no,GR,1);
	gpr_s = get_random_gpr(client_no,GR,0);
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	local_rand = random_no % 4;

	/* Load EA into R4 */
	offset = init_mem_for_gpr(client_no, (0x1<<local_rand));
	*tc_memory = GEN_ADDI_MCODE(LOAD_RB, 0, offset);
	cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
	tc_memory++;
	num_ins_built++;

	/* Build 1st load */
	*tc_memory = LXZX(gpr_t, LOAD_RA, LOAD_RB, macro_lsc[local_rand].ld_eopcode);
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
	cptr->instr_index[prolog_size + num_ins_built] = macro_lsc[local_rand].ld_index | 0x20000000;
	tc_memory++;
	num_ins_built++;

	/* Build next Store instruction */
	*tc_memory = STXX(gpr_s, LOAD_RA, LOAD_RB, macro_lsc[local_rand].st_eopcode);
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
	cptr->instr_index[prolog_size + num_ins_built] = macro_lsc[local_rand].st_index | 0x20000000;
	tc_memory++;
	num_ins_built++;

	/* Build 2nd dependent load */
	*tc_memory = LXZX(gpr_t, LOAD_RA, LOAD_RB, macro_lsc[local_rand].ld_eopcode);
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
	cptr->instr_index[prolog_size + num_ins_built] = macro_lsc[local_rand].ld_index | 0x20000000;
	tc_memory++;
	num_ins_built++;
	
	cptr->num_ins_built = num_ins_built;
}

void class_cpu_shl_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{

	/* Build load followed by a load */
	uint32 gpr_t, gpr_s;
	uint32 offset;
	uint32 num_ins_built, prolog_size, *tc_memory;
	int local_rand;

	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	gpr_t = get_random_gpr(client_no,GR,1);
	gpr_s = get_random_gpr(client_no,GR,0);
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	local_rand = random_no % 4;

	/* Load EA into R4 */
	offset = init_mem_for_gpr(client_no, (0x1<<local_rand));
   	*tc_memory = GEN_ADDI_MCODE(LOAD_RB, 0, offset);
   	cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
   	tc_memory++;
   	num_ins_built++;

	/* write store into testcase */
	*tc_memory = STXX(gpr_s, LOAD_RA, LOAD_RB, macro_lsc[local_rand].st_eopcode);
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
	cptr->instr_index[prolog_size + num_ins_built] = macro_lsc[local_rand].st_index | 0x20000000; 
 	tc_memory++;
	num_ins_built++;

	/* write load into testcase */
	*tc_memory = LXZX(gpr_t, LOAD_RA, LOAD_RB, macro_lsc[local_rand].ld_eopcode);
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
	cptr->instr_index[prolog_size + num_ins_built] = macro_lsc[local_rand].ld_index | 0x20000000;
 	tc_memory++;
	num_ins_built++;

	cptr->num_ins_built = num_ins_built;
}

void class_cpu_load_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4,i;
	uint32 eop,gpr_e,gpr_s, gpr_t, spr_reg;
	uint32 store_mcode, store_off, load_off;
	uint32 num_ins_built, prolog_size, *tc_memory;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	struct vsr_list *vsrs;
	vsrs = &cptr->vsrs[GR];
	char   insert_inst= 0;
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
    eop = (temp->op_eop_mask) & 0x0000ffff; 

	switch(eop) {	
		case 618:  /* P9:ldmx, EOP 309 */
		  	gpr_t = get_random_gpr(client_no, temp->tgt_dtype, 1);	/* target register */	
		  	gpr_s = get_random_gpr(client_no, temp->tgt_dtype, 0);	/* hold EA offset */	

			load_off = init_mem_for_gpr(client_no, 32);
			load_off &= 0x0000ffff;
			store_mcode = GEN_ADDI_MCODE(gpr_s, 0, load_off);  /* offset to  cptr->ls_base[pass] */
			*tc_memory = store_mcode;
			cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
			tc_memory++;
			num_ins_built++;


            uint32 op1 = (gpr_s << temp->op1_pos);
            uint32 op2 = (LOAD_RA << temp->op2_pos);
            uint32 tgt = (gpr_t << temp->tgt_pos);
            store_mcode = (temp->op_eop_mask | op1 | op2 | tgt);
            *tc_memory = store_mcode;
            cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
			cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
            tc_memory++;
            num_ins_built++;
			cptr->num_ins_built = num_ins_built;
			break;
		case 1066: /* lswx: EOP: 533 */
			/* Sequence of events:
	         * 1.  Select a set of target registers (eop, numRegs)
			 * 2.  Calculate the number of bytes to load (numBytes)
			 * 2.1 Only registers 10 to 31 are free.So,source register should be GPR30 or below
		 	 * 	   so that we can load atleast 2 registers
             * 3.  Allocate and initialize memory for loading (offset)
             * 4.  Update XER with the number of bytes to load (XER, numBytes)
             * 5.  Load the bytes (eop, RB, offset)
             * 
		  	 * though gpr_s is a target,tgt is off here.This is because STSWX is used to store all
		   	 * registers and all registers are marked dirty here..
		   	 * */
		  	gpr_s = get_random_gpr(client_no, temp->tgt_dtype, 0);	/* register to start with*/	
		  	gpr_e = MAX_REG;					            		/* Can load till GPR31 */
		  	vsr4 = ((gpr_e - gpr_s + 1) * 4) ;  				    /* number of bytes to be stored */ 
		  	/* Load the number of bytes to be written to XER in LOAD_RB register */          
		  	load_off = cpu_build_module(client_no, FC_ADDI_IMM, vsr4, LOAD_RB, index); /* get offset needed for load */ 
		  	num_ins_built = cptr->num_ins_built;					/* read the data again */
		  	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	        /* Update XER now using MTSPR */
		  	spr_reg=1*32;
		  	store_mcode = MTSPR(spr_reg,LOAD_RB);
		  	*tc_memory = store_mcode;	
		  	cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
            tc_memory++;
            num_ins_built++; 		  
        	cptr->num_ins_built=num_ins_built;					/* update the instructions*/
		  	/* do a stswx instruction for store here*/
   	        store_off = cpu_build_module(client_no,FC_ADDI,vsr4,STORE_RB,index); 
		  	/* stswx is used to store GPR's since lswx will store into all these registers*/
		  	store_mcode = STSWX(STORE_RA,STORE_RB,gpr_s);
		  	num_ins_built = cptr->num_ins_built;					/* read the data again */
		  	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
            *tc_memory = store_mcode;
		  	cptr->instr_index[prolog_size + num_ins_built] = stswx |0x20000000;
		  	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
            tc_memory++;
            num_ins_built++; 		  
        	cptr->num_ins_built=num_ins_built;					/* write the data again*/
            /* This function will load LOAD_RB with offset of memory from where we can store vsr4 number of bytes*/
		  	load_off = cpu_build_module(client_no,FC_ADDI,vsr4,LOAD_RB,index); /* get offset for load */
		  	for (i = gpr_s; i <= gpr_e; i++) {
			  vsrs->dirty_mask |= (0x1ULL << i );				/* mark registers dirty*/	
		  	}
		  	/* now build the instruction */
		  	vsr4 = gpr_s;							 		/* needed to build lswx */
		  	insert_inst = 1;
		  break;
	   default:		 
		  	vsr4 = get_random_gpr(client_no,temp->tgt_dtype,1);			/* get GPR's */
			num_ins_built = cptr->num_ins_built;					/* read the data again*/
        	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
     		/* This function will return the offset of memory and populate it with data */
			load_off = cpu_build_module(client_no, FC_ADDI, ALIGN_DW, LOAD_RB, index); /* get offset for load */
			insert_inst = 1;
			break;
    } /* end of switch */  	
      	     
	if (insert_inst == 1) {
	    load_off = cpu_build_module(client_no, FC_INST_LOAD, load_off, vsr4, index);
	} 	
	num_ins_built = cptr->num_ins_built;					/* read the data again */
    tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* Check if its a load update */
	if (temp->op1_dtype == GRU) {
		*tc_memory = OR(BASE_GPR, LOAD_RA);
		cptr->instr_index[prolog_size + num_ins_built] = or | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_or;
		tc_memory++;
		num_ins_built++;
	}
	cptr->num_ins_built = num_ins_built;
	cptr->bdy_req = 0;
 	/*	
	DPRINT(log, "\nInstruction Built in %s is = %lx", __FUNCTION__, mcode);
	DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name);
	*/
}

void class_cpu_load_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *instr_mask, int index)
{
	uint32 vsr4, i, size = 0;
	uint32 RT;
	uint32 store_mcode, store_off, load_off;
	uint32 num_ins_built, prolog_size, *tc_memory;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	struct vsr_list *vsrs;

	vsrs = &cptr->vsrs[GR];
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

    RT = (instr_mask->op_eop_mask) & 0x0000ffff; 
    if (RT == 552) {
	   	cptr->bdy_req = ALIGN_QW;					/* 16 byte boundary */
		size = ALIGN_QW;							/* 16 bytes needed in memory*/
	} else if (RT == 168) {
	   	cptr->bdy_req = ALIGN_DW;					/* 8 byte boundary */
	   	size = ALIGN_DW;							/* 8 byte needed */
	} else if (RT == 232) {
	   	cptr->bdy_req = ALIGN_HW;					/* 2 byte boundary */
	   	size = ALIGN_HW;							/* 2 byte needed */
	} else if ( RT == 104) {
	   	cptr->bdy_req = ALIGN_B;					/*  byte boundary */
	   	size = ALIGN_B;							   	/*  byte needed */
	} else {
		cptr->bdy_req = ALIGN_W;					/* 4 byte bdy needed */
	   	size = ALIGN_W;								/* 4 byte needed */
	}
	switch(RT) {	
		case 552: 		  							/* lqarx */
		case 168: 		  							/* ldarx */
	   	case 40:									/* lwarx */
	   	case 232:									/* lharx */
	   	case 104:									/* lbarx */
	  		/* lqarx needs vsr4 to be even register.So do not use the routine directly */
		   	if (RT == 552) {
		   		do{
		     		vsr4 = vsrs->head[BFP]->vsr_no;
		   			vsr4 = vsr4 & 0x1f;
					MOVE_VSR_TO_END(client_no, GR, BFP);		/* Move the pointer*/
		        }while((vsr4 % 2) == 1);						/* Get even register*/
			 	/* Store and Mark vsr4 and vsr4 + 1 as dirty */
				i = vsr4;										/* Initialize */
				do{
					if ((0x1ULL << i ) & vsrs->dirty_mask) {		/* store target if dirty */
						store_off = init_mem_for_gpr(client_no,8);
						store_off &= 0x0000ffff;
						store_mcode = STD(i,store_off,LOAD_RA);
						*tc_memory = store_mcode;
						cptr->instr_index[prolog_size + num_ins_built] = std | 0x20000000;
						cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
						tc_memory++;
						num_ins_built++;
					}
					vsrs->dirty_mask |= ((0x1ULL << i ));			/* mark it as dirty      */
				   	i++;
				}while(i < (vsr4 + 2));
		  	} else {									/* other instructions*/
		   		vsr4 = get_random_gpr(client_no, instr_mask->tgt_dtype, 1);	  	        /* get register */
		   	}   	
		   	load_off = cpu_build_module(client_no,FC_ADDI,size,LOAD_RB,index);   /* get offset for load   */
		   	num_ins_built = cptr->num_ins_built;					/* read the data again*/
        	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
		   	/* Build Instruction */
		   	vsr4 = vsr4 & 0x1f;
		 	cpu_build_module(client_no, FC_INST_LOAD_EH, load_off,vsr4, index);
		   	/* Now build corresponding store to clear reservation */
		   	num_ins_built = cptr->num_ins_built;
		   	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
		   	switch(RT) {
			   case 104:
			   		store_mcode = STBCX(vsr4,LOAD_RA,LOAD_RB); /* stbcx.*/
		   			cptr->instr_index[prolog_size + num_ins_built] = stbcx | 0x20000000;
		   			break;
			   case 232:
			   		store_mcode = STHCX(vsr4,LOAD_RA,LOAD_RB); /* sthcx.*/
		   			cptr->instr_index[prolog_size + num_ins_built] = sthcx | 0x20000000;
		   			break;
			   case 40:
			   		store_mcode = STWCX(vsr4,LOAD_RA,LOAD_RB); /* stwcx.*/
		   			cptr->instr_index[prolog_size + num_ins_built] = stwcx | 0x20000000;
		   			break;
			   case 168:
		    			store_mcode = STDCX(vsr4,LOAD_RA,LOAD_RB); /* stdcx.*/
		   			cptr->instr_index[prolog_size + num_ins_built] = stdcx | 0x20000000;
					break;
			   case 552:
					store_mcode = STQCX(vsr4,LOAD_RA,LOAD_RB); /* stqcx.*/
		   			cptr->instr_index[prolog_size + num_ins_built] = stqcx | 0x20000000;
					break;
		   	   default:	
					sprintf(msg, "Unknown instruction EOP: %d, in cpu_load_1_gen routine", RT);
	   				hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
	   				exit(1);
					break;						/* none */
		   	}
		   	*tc_memory = store_mcode;
		   	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
           	tc_memory++;
		   	num_ins_built++;
		   	cptr->num_ins_built=num_ins_built;					/* read the data again*/
	   		break;
		default:  sprintf(msg, "Unknown instruction in EOP: %d, in cpu_load_1_gen routine", RT);
	   		hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
	   		exit(1);
	} /* end of switch */  	

	cptr->num_ins_built = num_ins_built;
	cptr->bdy_req = 0;
 	/*	
	DPRINT(log, "\nInstruction Built in %s is = %lx", __FUNCTION__, mcode);
	DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, instr_mask->ins_name);
	*/
}

void class_cpu_cond_log_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,mcode,*tc_memory,num_ins_built,prolog_size,addi_mcode,store_off,vsr4;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	uint64 dirty_reg_mask;
	struct vsr_list *vsrs;
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
    vsrs = &cptr->vsrs[ins->tgt_dtype];
	vsr4 = vsrs->head[VSX]->vsr_no;
    MOVE_VSR_TO_END(client_no, ins->op1_dtype, VSX);
	op1 = ((vsr4 & 0x7) << (ins->op1_pos));
	/* op2 CR.MCRF instruction has only op1 and tgt data type */
	if (ins->op2_dtype != DUMMY){
		vsrs = &cptr->vsrs[ins->tgt_dtype];
		vsr4 = vsrs->head[VSX]->vsr_no;
        MOVE_VSR_TO_END(client_no, ins->op2_dtype, VSX);
		op2 = ((vsr4 & 0x7) << (ins->op2_pos));
	}
	/* target CR */
	vsrs = &cptr->vsrs[ins->tgt_dtype];
	vsr4 = vsrs->head[VSX]->vsr_no;
    MOVE_VSR_TO_END(client_no, ins->tgt_dtype, VSX);
    tgt = ((vsr4 & 0x7) << (ins->tgt_pos));
	dirty_reg_mask = vsrs->dirty_mask;
    mcode = (ins->op_eop_mask | op1 | op2 | tgt);

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
        store_off = init_mem_for_vsx_store(client_no, ins->tgt_dtype);
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
    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = ins->sim_func;
    tc_memory++;
    num_ins_built++;
    vsrs->dirty_mask |= (0x1ULL << vsr4);
    cptr->num_ins_built = num_ins_built;
    /* DPRINT(log, "\nIns Built in %s is = %lx, %s, sim_ptr=%lx\n", __FUNCTION__,mcode,ins->ins_name,ins->sim_func); */
}


void class_cpu_fixed_arth_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, op3=0, tgt=0, mcode=0, *tc_memory, num_ins_built, prolog_size, RT1=0, RT2=0, RT=0;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;

	/* pick registeres */
	RT1= get_random_gpr(client_no, ins->op1_dtype, 0);          /* get source register */
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     

	if (ins->op2_pos > 0) {
	  	RT2= get_random_gpr(client_no, ins->op2_dtype, 0);      /* get source register */
		op2 = ((RT2 & 0x1f) << (ins->op2_pos));                     
	}

	/* check for addex */
	if ((ins->op3_pos > 0) && (ins->op3_dtype == IMM_DATA_2BIT)) {
		op3 = ((get_random_no_32(client_no)) % 3);        			/* 2 bit CY - imm value(0, 1, 2) */
    	op3  = op3 << (ins->op3_pos);
	}

	RT= get_random_gpr(client_no, ins->tgt_dtype, 1);           /* get target register */
	tgt = ((RT & 0x1f) << (ins->tgt_pos));                     

	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	mcode = (ins->op_eop_mask | op1 | op2 | op3 | tgt);
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_fixed_logic_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size, RT1=0,Imm_data=0,RT=0;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	if (ins->tgt_dtype == DUMMY) {
		op1 = 0;
		tgt=0;
		Imm_data =0;
	}
	else {
	  	RT1= get_random_gpr(client_no,ins->op1_dtype,0);   	       /* get a register */
		op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
		Imm_data = ((get_random_no_32(client_no)) % 65536);		/* get immediate data */
		RT= get_random_gpr(client_no,ins->tgt_dtype,1);          /* get a register */
		num_ins_built = cptr->num_ins_built;
		tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
		tgt = ((RT & 0x1f) << (ins->tgt_pos));                     
	}

	mcode = (ins->op_eop_mask | op1 | Imm_data | tgt);
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */

}


void class_cpu_fixed_logic_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, op3=0, tgt=0, mcode=0, *tc_memory, num_ins_built, prolog_size, reg_val = 0;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	reg_val = get_random_gpr(client_no, ins->op1_dtype, 0);         /* get a register */
	op1 = ((reg_val & 0x1f) << (ins->op1_pos));                     

	reg_val= get_random_gpr(client_no, ins->op2_dtype, 0);          /* get a register */
	op2 = ((reg_val & 0x1f) << (ins->op2_pos));                     

	if (ins->op3_dtype != DUMMY) {
		reg_val= get_random_gpr(client_no, ins->op3_dtype, 0);          /* get a register */
		op3 = ((reg_val & 0x1f) << (ins->op3_pos));                     
	}

	reg_val = get_random_gpr(client_no, ins->tgt_dtype, 1);         /* get a register */
	tgt = ((reg_val & 0x1f) << (ins->tgt_pos));                     

	mcode = (ins->op_eop_mask | op1 | op2 | op3 | tgt);
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_thread_pri_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 *tc_memory,num_ins_built,prolog_size;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	uint32 priority_states[]={1,6,2,31,5},prior_num;
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* Randomly force Thread Priority change.The change can be done using or RX,RX,RX.
	 * RX  Priority
	 * 31   very low ->Only Salerno and above
	 * 1    low
	 * 6    normal
	 * 2    medium
	 * 5    medium high ---> This needs OS intervention.But,can be tested and not harmful.Only salerno and above.
	 *    Thread priority should have low frequency so that FPU/CPU exerciser performace should not be affected.
	 */
	/* Pick a priority */
	if (shifted_pvr_os <= 0x4a){				/* P7+ and below procs */
		prior_num = priority_states[(get_random_no_32(client_no))%3]; /* Choose only 1,6 or 2 */ 
	} else if (shifted_pvr_os >= SHIFTED_PVR_OS_P8) {				  /* P8 and above */
		prior_num = priority_states[(get_random_no_32(client_no))%5]; /* Choose only 1,6 or 2 */ 
	}
	/* build first test method */
	*tc_memory = OR(prior_num,prior_num); 
	cptr->instr_index[prolog_size + num_ins_built] = or | 0x20000000;
	tc_memory++;
	num_ins_built++;
		
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_fixed_arth_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT=0;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT1= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
	RT= get_random_gpr(client_no,ins->tgt_dtype,1);          /* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	tgt = ((RT & 0x1f) << (ins->tgt_pos));                     
	mcode = (ins->op_eop_mask | op1 | tgt);
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = ins->sim_func;           
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_rotate_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0,op3=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT2=0,RT=0;
	uint32 SH1,SH2,MB1,MB2;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT1= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
	RT2 = ((get_random_no_32(client_no)) %64);
	SH1 = RT2%32;
	op2 = (SH1) << (ins->op2_pos); 
	SH2 = RT2/32;
	SH2 = SH2 << 1;
	RT2 = ((get_random_no_32(client_no)) %64);
	MB1 = RT2%32;
	op3 = MB1 << (ins->op3_pos);
	MB2 = RT2/32;
	MB2 = MB2 << 5;
	RT= get_random_gpr(client_no,ins->tgt_dtype,1);          /* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	tgt = ((RT & 0x1f) << (ins->tgt_pos));                     
	mcode = (ins->op_eop_mask | op1 | tgt | op2 | op3 | MB2 | SH2);
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = ins->sim_func;           
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_rotate_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT2=0,RT=0;
	uint32 var_mb,ME;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT1= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
	RT2 = ((get_random_no_32(client_no)) %32);		  /* SH value */
	op2 = (RT2) << (ins->op2_pos); 
	RT2 = ((get_random_no_32(client_no)) %32);		  /* var_mb value */
	var_mb  = (RT2) << (ins->op3_pos); 
	RT2 = ((get_random_no_32(client_no)) %32);		  
	ME  = (RT2) << 1 ;                        		  /* ME value */
	RT= get_random_gpr(client_no,ins->tgt_dtype,1);          /* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	tgt = ((RT & 0x1f) << (ins->tgt_pos));                     
	mcode = (ins->op_eop_mask | op1 | tgt | op2 | var_mb  | ME  );        
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = ins->sim_func;           
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_rotate_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT2=0,RT=0;
	uint32 var_mb,MB1;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT1= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
	RT2= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op2 = ((RT2 & 0x1f) << (ins->op2_pos));                     
	RT2 = ((get_random_no_32(client_no)) %64);		  /* var_mb value */
	var_mb  = RT2 % 32;
	var_mb  = (var_mb) << (ins->op3_pos); 
	MB1 = RT2 /32;
	MB1 = (MB1) << 1;
	RT= get_random_gpr(client_no,ins->tgt_dtype,1);          /* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	tgt = ((RT & 0x1f) << (ins->tgt_pos));                     
	mcode = (ins->op_eop_mask | op1 | tgt | op2 | var_mb | MB1  );        
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = ins->sim_func;           
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_rotate_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT2=0,RT=0;
	uint32 var_mb,ME;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT1= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
	RT2= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op2 = ((RT2 & 0x1f) << (ins->op2_pos));                     
	RT2 = ((get_random_no_32(client_no)) %32);		  /* var_mb value */
	var_mb  = RT2  << (ins->op3_pos); 
	RT2 = ((get_random_no_32(client_no)) %32);		  /* var_mb value */
	ME  = RT2  << 1; 
	RT= get_random_gpr(client_no,ins->tgt_dtype,1);          /* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	tgt = ((RT & 0x1f) << (ins->tgt_pos));                     
	mcode = (ins->op_eop_mask | op1 | tgt | op2 | var_mb | ME );        
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = ins->sim_func;           
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}
void class_cpu_fixed_arth_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT2=0,RT=0;
	uint32 SH,SH2;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT1= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
	RT2 = ((get_random_no_32(client_no)) %64);		  /* SH value */
	SH  = RT2%32;
	SH  = SH << (ins->op2_pos);
	SH2 = RT2/32;
	SH2 = SH2 << 1;
	RT= get_random_gpr(client_no,ins->tgt_dtype,1);          /* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	tgt = ((RT & 0x1f) << (ins->tgt_pos));                     
	mcode = (ins->op_eop_mask | op1 | tgt | SH  | SH2 );           
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = ins->sim_func;           
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_fixed_arth_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT2=0,RT=0;
	uint32 SH;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT1= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
	RT2 = ((get_random_no_32(client_no)) %32);		  /* SH value */
	SH  = RT2<< (ins->op2_pos);
	RT= get_random_gpr(client_no,ins->tgt_dtype,1);          /* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	tgt = ((RT & 0x1f) << (ins->tgt_pos));                     
	mcode = (ins->op_eop_mask | op1 | tgt | SH );           
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = ins->sim_func;           
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_fixed_spr_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT2=0,RT=0;
	/* allow write only to link,ctr and xer.VRSAVE can not be written from user space */
	const uint32 sprs[3] = {1, 8, 9};              
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT1= get_random_gpr(client_no,ins->op1_dtype,0);          /* get a register */
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
	RT2 = ((get_random_no_32(client_no)) % 3);		  /* SPR register number */
	RT = sprs[RT2];
	tgt = (RT) << (ins->tgt_pos);				   /* SPLIT FIELD lower bits */
	mcode = (ins->op_eop_mask | op1 | tgt);           
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = ins->sim_func;           
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_fixed_spr_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT2=0,RT=0;
	const uint32 sprs[4] = {8, 256, 288, 392};          
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT2 = ((get_random_no_32(client_no)) % 4);		  /* SPR register number */
	RT1 = sprs[RT2];
	op1 = RT1 << (ins->op1_pos);
	/* from now use R6, as R10 is used for cpu_id_mask with THREADS_SYNC enabled */
	RT = 6;						/* only reg10 for loading values from sprs*/
	tgt = ((RT & 0x1f) << (ins->tgt_pos));                     
	mcode = (ins->op_eop_mask | op1 | tgt );           
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_fixed_spr_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, mcode=0,*tc_memory,num_ins_built,prolog_size,RT1=0,RT2=0;
	/* allow write only to link,ctr and xer.VRSAVE can not be written from user space */
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* pick registeres */
	RT1 = get_random_gpr(client_no,ins->op1_dtype, 1);          /* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	op1 = ((RT1 & 0x1f) << (ins->op1_pos));                     
	RT2 = ((get_random_no_32(client_no)) % 256);		  /* mask value      */
	op2 = (RT2 & 0xff) << (ins->tgt_pos);
	mcode = (ins->op_eop_mask | op1 | op2 );           
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_fixed_store_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, mcode=0,*tc_memory,num_ins_built,prolog_size,RT;                
	uint32 opcode_ext=0,gpr_s=0, tgt,i;
	int offset,store_off;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	struct vsr_list *vsrs;
	vsrs = &cptr->vsrs[GR];
	prolog_size = cptr->prolog_size;
	opcode_ext = ins->op_eop_mask & 0x0000ffff;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	do { 
		RT =  get_random_gpr(client_no,ins->op2_dtype,0);  		/* get the register */  
	}while ( (opcode_ext == 0X016D) && ( (RT%2) == 1) );

	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	switch (opcode_ext){
		case 0x056D:							/* stbcx. */
			cptr->bdy_req = 1;						   /* Word bdy */
	 	case 0x01EE:							/* stbux */
	 	case 0x01AE:							/* stbx */
			offset = ALIGN_B;		    		
			break;
	 	case 0x012D:
			cptr->bdy_req = 4;						   /* Word bdy */
	 	case 0x016E:
	 	case 0x012E:							/* stwux */
	 	case 0x052C:							/* stwux */
			offset = ALIGN_W;				/* stwx */
			break;
	 	case 0x05AD:							/* sthcx. */
			cptr->bdy_req = 2;						   /* Word bdy */
	 	case 0x036E:
	 	case 0x032E:
		case 0x072C:
			offset = ALIGN_HW;						  /* sthux and sthx */
			break;
	 	case 0x01AD:
			cptr->bdy_req = 8;						   /* Doubleword bdy */
	 	case 0x0528:
	 	case 0x012A:
	 	case 0x016A:
			offset = ALIGN_DW;						    /* stdx */
			break;								    /* stdbrx */	
	 	case 0x016D:
			offset = ALIGN_QW;						    /* stqcx */
			cptr->bdy_req = 16;						   /* Quadword bdy */
			break;			
	 	case 0x052A:									    /* stswx */
			do { 
				gpr_s = get_random_gpr(client_no,ins->tgt_dtype,1);  		    /* start register*/
			}while(RT < gpr_s);
			/* Load and Store has same build..do not load beyond GPR31 or else it will
			 * start writing to GPR0 and so on which will corrupt important registers
			 */
			offset = ((MAX_REG - RT + 1) * 4) ;				/* offset */
			/* to be done..if register is dirty,then store the contents of RT before addi */
			store_off = cpu_build_module(client_no,FC_ADDI_IMM,offset,gpr_s,index);/* load RT with offset */
			num_ins_built = cptr->num_ins_built;	/* read num_ins_built and tc_memory */
			tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
			/* Move the value to fixed point exception register */
			*tc_memory = MTSPR(32,gpr_s);				   
			cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
            tc_memory++;
		   	num_ins_built++;
			if (opcode_ext == 0x052A) {						/* stswx */	
				for(i=RT;i<=MAX_REG;++i){
					vsrs->dirty_mask &= (~(0x1ULL << i ));			/* mark it as clean */
				}
			}
			break;				
	 	case 0x03F8: /* cmpb and bperm */
	 	case 0x01F8:
			offset = 0;
			gpr_s =  get_random_gpr(client_no,ins->op1_dtype,0);  		/* get the register */  
			tgt = (RT & 0x1f) << (ins->tgt_pos);
			op1 = (gpr_s & 0x1f) << (ins->op1_pos);
		 	mcode = ((ins->op_eop_mask) | op1 | op2 | tgt);		
			*tc_memory = mcode;
			tc_memory++;
			num_ins_built++;
			break;
          
	 	default:   
			sprintf(msg, "Unknown instruction in fixed_store_1_gen routine ");
		    hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
		    exit(1);
	}
	if(offset > 0 ) {
		store_off = init_mem_for_gpr(client_no, offset );			/* get offset */
		cptr->num_ins_built = num_ins_built;					/* Update number of instructions */
		offset = cpu_build_module(client_no,FC_ADDI_IMM,store_off,STORE_RB,index);
		store_off = cpu_build_module(client_no,FC_INST_STORE,store_off,RT,index);
	}
	num_ins_built = cptr->num_ins_built;	/* Read and write back before leaving. if not num_ins_built will have old value */
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
	cptr->bdy_req = 0;							/* Reset boundary required element */
	vsrs->dirty_mask &= (~(0x1ULL << RT ));					/* mark it as clean */
	if (opcode_ext == 0X016D){						/* for stqcx. */
		vsrs->dirty_mask &= (~(0x1ULL << (RT+1) ));			/* mark RT+1 as clean for stqcx */
	}
}

void class_cpu_fixed_store_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT;                
	uint32 opcode_val=0,i;
	int offset,store_off;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	struct vsr_list *vsrs;
	vsrs = &cptr->vsrs[GR];
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	opcode_val = ((ins->op_eop_mask & 0xffff0000) >> 16);
	opcode_val = opcode_val & 0x0000ffff;

	if (ins->ins_class == CPU_FIXED_STORE) {
		RT =  get_random_gpr(client_no,ins->op2_dtype,0);  		/* get the register */  
	} else {
		RT =  get_random_gpr(client_no,ins->op2_dtype,1);  		/* register is target here */
		num_ins_built = cptr->num_ins_built;
		tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	}
	/* For store quadword - stq,the RT must be even register */
	if ( (ins->op_eop_mask == 0XF8000002) && ((RT %2) == 1)) {
		do {
			RT =  get_random_gpr(client_no,ins->op2_dtype,0);  		/* get the register */  
		}while((RT%2) == 1);
	}

	switch (opcode_val){
		case 0x8800:
	 	case 0x8C00:
	 	case 0x9800:							/* stbu */
	 	case 0x9C00:
			offset = ALIGN_B;		    		/* stbx */
			break;
	 	case 0xA000:
	 	case 0xA400:
	 	case 0xA800:
	 	case 0xAC00:
	 	case 0xB000:
	 	case 0xB400:								/* sthu  */
			offset = ALIGN_HW;					/* sth  */
			break;
	 	case 0x8000:
	 	case 0x8400:
	 	case 0x9000:
	 	case 0x9400:
			offset = ALIGN_W;					/* stwu and stw */
			break;
	 	case 0xB800:								/* lmw and stmw */
	 	case 0xBC00:
                  	offset = ((MAX_REG - RT + 1) * 4) ;
			/* Store the registers which will be loaded by lmw using stmw */
			if (ins->ins_class == CPU_FIXED_LOAD ) {
				store_off = init_mem_for_gpr(client_no,offset);				/* get offset */
				mcode = STMW(RT,STORE_RA,store_off); 
	        	*tc_memory = mcode;
				cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = (store_off & 0x0000fffc);
				cptr->instr_index[prolog_size + num_ins_built] = stmw | 0x20000000;
       			tc_memory++;
				num_ins_built++;	
			}
			for(i = RT; i <= MAX_REG; i++){
				if (ins->ins_class == CPU_FIXED_LOAD ) {
					vsrs->dirty_mask |= ((0x1ULL << i)); /* lmw,mark it as dirty  */	
				}else {
					vsrs->dirty_mask &= (~(0x1ULL << i));/* stmw,mark it as clean */
				}
			}/* end of for loop */
			break;
	 	case 0xE800:
	 	case 0xF800:
			opcode_val = (ins->op_eop_mask & 0X0000ffff);	/* stmd and lmd not supported*/
			if (opcode_val == 3) {							/* stmd and lmd*/
				offset = ((MAX_REG - RT + 1) * 8);
			}
			else if (ins->op_eop_mask == 0XE8000002) {		/* lwa */
				offset = ALIGN_W;
			} 
			else if (ins->op_eop_mask == 0XF8000002) {		/* stq */
				cptr->bdy_req = 16;
				offset = ALIGN_QW;
			}
			else {											/* std and stdu */
				offset = ALIGN_DW;
			}
			break;
	 
	 	default:
			sprintf(msg, "Unknown instruction in fixed_store_2_gen routine ");
		    hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
		    exit(1);
	}

	store_off = init_mem_for_gpr(client_no,offset);				/* get offset */
	tgt = (RT & 0x1f) << (ins->tgt_pos);
    op2 = (STORE_RA & 0x1f) << (ins->op2_pos);
	if (opcode_val == 0xE800 || opcode_val == 0xF800) {
		op1 = store_off >> 2; /* rshift as hdware will lshift by 2 bits */
	}
	op1 = store_off << (ins->op1_pos);	/*lshift for bit position*/
    mcode = (ins->op_eop_mask | op1 | op2 | tgt);
    *tc_memory = mcode;
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = (store_off & 0x0000fffc);
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    tc_memory++;
	num_ins_built++;
 	if (ins->op2_dtype == GRU) {
		*tc_memory = OR(BASE_GPR, LOAD_RA);				/* restore the GPR value */
		cptr->instr_index[prolog_size + num_ins_built] = or | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_or;
		tc_memory++;
		num_ins_built++;
	}
	cptr->num_ins_built = num_ins_built;						/* Update number of instructions */
	if (ins->ins_class == CPU_FIXED_LOAD ) {
		;														/* for load -random_gpr will mark */
	}else {														/* the register as dirty */
		vsrs->dirty_mask &= (~(0x1ULL << RT ));					/* for store - mark it as clean */
		if (ins->op_eop_mask == 0XF8000002){
		  vsrs->dirty_mask &= (~(0x1ULL << (RT+1) ));		    /*for store - mark RT+1 alsoas clean */	
		}
	}
	cptr->bdy_req = 0;							/* reset the boundary requirement */
	
}

void class_cpu_fixed_arth_4_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT;                
	uint32 gpr_s=0;
	uint32 offset;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	RT =  get_random_gpr(client_no,ins->tgt_dtype, 1);  		/* get target register */  
	tgt = (RT & 0x1f) << (ins->tgt_pos);
	gpr_s =  get_random_gpr(client_no, ins->op2_dtype, 0);  	/* get source register */  

	if (ins->op1_dtype == IMM_DATA) {
    	offset = ( (get_random_no_32(client_no)) % (65535));    /* need only 16 bits */	
		op1 = (offset & 0x0000ffff); 
	}
	else if (ins->op1_dtype == GR) {
		op1 = gpr_s & 0x1f;
	}
	op1 = op1  << ins->op1_pos;

	if ((ins->op2_dtype == IMM_DATA) && (ins->op3_dtype == IMM_DATA_1BIT)) {
		op2 = (random_no & 0x0000001f); /* 5 bit sh0:4 */
		uint32 op3 = (random_no & 0x00000001); /* 1 bit sh5 */
		op2 = (op3 << 6) | op2;
	}
	else if (ins->op2_dtype == GR) {
    	op2 = (gpr_s & 0x1f); 
	}
	op2 = op2 << (ins->op2_pos);

    mcode = (ins->op_eop_mask | op1 | op2 | tgt);
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
        /* mark the register as dirty */
}

void class_cpu_fixed_all_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 mcode=0,*tc_memory,num_ins_built,prolog_size;                
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
    mcode = (ins->op_eop_mask);
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_fixed_all_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, mcode=0,*tc_memory,num_ins_built,prolog_size;                
	uint32 opcode_ext=0,gpr_t;
	uint32 e=0, l=0, size=8, load_off;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	opcode_ext = ins->op_eop_mask & 0x000007fe;
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	switch( opcode_ext )
	{
		case 0x4AC:												/*sync instruction*/
					l = ( (get_random_no_32(client_no)) % (3));
					e = ( (get_random_no_32(client_no)) % (16));
					load_off = cpu_build_module(client_no,FC_ADDI,size,LOAD_RB,index);
					num_ins_built = cptr->num_ins_built;
					tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
					if ( e & 0xC || e == 0 )	/*mblx */
					{
						gpr_t = get_random_gpr(client_no,GR,1);
						num_ins_built = cptr->num_ins_built;
						tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
						mcode = LDX(gpr_t, LOAD_RA, LOAD_RB);
						*tc_memory = mcode;
						cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
						cptr->instr_index[prolog_size + num_ins_built] = ldx | 0x20000000;
					 	tc_memory++;
						num_ins_built++;
						cptr->num_ins_built = num_ins_built;
					}
					if ( e & 0x3 || e == 0 )	/*mbsx */
					{
						gpr_t = get_random_gpr(client_no,GR,0);
						num_ins_built = cptr->num_ins_built;
						tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
						mcode = STDX(gpr_t, STORE_RA, STORE_RB);
						*tc_memory = mcode;
						cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
						cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
						tc_memory++;
						num_ins_built++;
						cptr->num_ins_built = num_ins_built;
					}
					if ( e & 0x2 )
					{
						l=0;
					}
					op1 = (l & 0x00000003) << (ins->op1_pos);
					op2 = (e & 0x0000000f) << (ins->op2_pos);
		   	    	mcode = (ins->op_eop_mask | op1 | op2);
					num_ins_built = cptr->num_ins_built;
					tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
					*tc_memory = mcode;
					cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
					tc_memory++;
					num_ins_built++;
					cptr->num_ins_built = num_ins_built;
					if( e & 0xA || e == 0 )	/*mbxl */
					{
						gpr_t = get_random_gpr(client_no,GR,1);
						num_ins_built = cptr->num_ins_built;
						tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
						mcode = LDX(gpr_t, LOAD_RA, LOAD_RB);
						*tc_memory = mcode;
					 	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
			            cptr->instr_index[prolog_size + num_ins_built] = ldx | 0x20000000;
				       	tc_memory++;
						num_ins_built++;
						cptr->num_ins_built = num_ins_built;
					}
					if ( e & 0x5 || e == 0 )	/*mbxs */
					{
						gpr_t = get_random_gpr(client_no,GR,0);
						num_ins_built = cptr->num_ins_built;
						tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
						mcode = STDX(gpr_t, STORE_RA, STORE_RB);
						*tc_memory = mcode;
						cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
			            cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
				       	tc_memory++;
						num_ins_built++;
						cptr->num_ins_built = num_ins_built;
					}
					break;
		default:
					sprintf(msg, "Unknown instruction in cpu_fixed_all_gen routine ");
					hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
					exit(1);		
	}
}

void class_cpu_fixed_store_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size;                
	uint32 opcode_val=0,gpr_s=0,gpr_e=0,max_reg,num_bytes=0,addi_mcode=0,i;
	int offset;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	struct vsr_list *vsrs;
	vsrs = &cptr->vsrs[ins->tgt_dtype];
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
    opcode_val = (ins->op_eop_mask & 0x0000ffff);
    max_reg = MAX_REG;							/* represents 31st register */	
    /* steps to be followed */
	/* 1. select set of registers to be loaded or stored */
	/* 2.for lswi,number of bytes loaded should not crossover to zeroth register i.e. load only from say reg24 till
	    31 */
	/* 3. For LSWI,first store all the target registers and then do a load */
	/* 4.Get an offset where stswi will store data or for LSWI,registers will be first stored */
	/* 5.Build the offset in to a register */
	/* 6.Get another offset for lswi from data will be fetched */
	/* 7.Build the actual instruction */
	switch(opcode_val)
	{
		case 0x04AA:
		case 0x05AA:							/* stswi */
		      	do{
				  	gpr_e = get_random_gpr(client_no,ins->tgt_dtype,1);	/* end register */	
					num_ins_built = cptr->num_ins_built;
					tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
				}while(gpr_e == 0x1f);						/* should not be 31st*/  
				do{
				  	gpr_s = get_random_gpr(client_no,ins->tgt_dtype,1);	     /* start register */
					num_ins_built = cptr->num_ins_built;
					tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
				}while(gpr_s == 0x1f);						/* should not be 31st*/
				gpr_e = gpr_e & 0x1f;
				gpr_s = gpr_s & 0x1f;
				if (gpr_s > gpr_e) {
				   	tgt = gpr_e;
		   		   	gpr_e = gpr_s;
				   	gpr_s = tgt;
				}
				/* Idea is to randomly store one to eight register.Registers,starting gpr_e  */
				/* will be stored.gpr_s will be used to store the offset of load/store area */   
			    /* calculate the number of bytes which will be stored */ 	
				/* FOR LSWI,load only from gpr_e till reg31 */
			    num_bytes = ( (max_reg - gpr_e) + 1 ) * 4;
				if (num_bytes >= 32){
					num_bytes = 28;
				}
				/* now get offset where we need to store the register */
				offset = init_mem_for_gpr(client_no, num_bytes);
			    offset = offset & 0x0000ffff;
				/* store gpr_s and mark it as clean */
				addi_mcode = GEN_ADDI_MCODE(gpr_s, LOAD_RA, offset);
                *tc_memory = addi_mcode;
	            cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
        	    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
                tc_memory++;
			    num_ins_built++; 
				op1 = (num_bytes & 0x1f) << (ins->op1_pos);
			    op2 = (gpr_s & 0x1f) << (ins->op2_pos); 
			    tgt = (gpr_e & 0x1f) << (ins->tgt_pos);
			    if (opcode_val == 0x05AA) {  
				   	mcode = (ins->op_eop_mask | op1 | op2 | tgt);
			        cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
				} else if (opcode_val == 0x04AA) {				
					mcode = STSWI(num_bytes,gpr_s,gpr_e);
					cptr->instr_index[prolog_size + num_ins_built] = stswi | 0x20000000;
				}
				*tc_memory = mcode;
				cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;      
       			tc_memory++;
	            num_ins_built++;	
			    /* build the LSWI now */	
			    if (opcode_val == 0x04AA) {
					/* now get another offset from where registers will be loaded */
			        offset = init_mem_for_gpr(client_no, num_bytes);
				    offset = offset & 0x0000ffff;
					/* store gpr_s and mark it as clean */
					addi_mcode = GEN_ADDI_MCODE(gpr_s, LOAD_RA, offset);
                	*tc_memory = addi_mcode;
	                cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
                	tc_memory++;
			        num_ins_built++; 
					/* now build lswi */
					op1 = (num_bytes & 0x1f) << (ins->op1_pos);
			        op2 = (gpr_s & 0x1f) << (ins->op2_pos); 
			        tgt = (gpr_e & 0x1f) << (ins->tgt_pos);
			        mcode = (ins->op_eop_mask | op1 | op2 | tgt);
        		    *tc_memory = mcode;
					cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;      
			        cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       			    tc_memory++;
	                num_ins_built++;	
				  	/* mark registers starting gpr_e as dirty */
					for(i = gpr_e; i < (gpr_e + (num_bytes/4)); i++) {
						vsrs->dirty_mask |= ((0x1ULL << i ));	/* mark it as dirty   */
					}
				}	
				else{
					/* mark all registers used by store as clean */
					for(i=gpr_e;i<(gpr_e+(num_bytes/4));i++) {
						vsrs->dirty_mask &= (~(0x1ULL << i));	/* mark it as clean   */
					}
				}
				/* Cover up the index registers */
				mcode = OR(gpr_e,gpr_s);
				*tc_memory = mcode;
				cptr->instr_index[prolog_size + num_ins_built] = or | 0x20000000;
				tc_memory++;
	            num_ins_built++;
				break;

		    default:    sprintf(msg, "Unknown instruction in fixed_store_3_gen routine ");
				hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
				exit(1);
	}
	/* update number of instructions built */
	cptr->num_ins_built = num_ins_built;	
}

void class_cpu_fixed_logic_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RA,RB;                
	uint32 vsr4,Imm_data,op3=0,addi_mcode,store_off;
	uint64 dirty_reg_mask;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* target CR */
	vsrs = &cptr->vsrs[ins->tgt_dtype];
	vsr4 = vsrs->head[VSX]->vsr_no;
    MOVE_VSR_TO_END(client_no, ins->tgt_dtype, VSX);
    tgt = ((vsr4 & 0x7) << (ins->tgt_pos));
	dirty_reg_mask = vsrs->dirty_mask;

	if ((0x1ULL << vsr4) & dirty_reg_mask) {
		/*
		 *
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
        store_off = init_mem_for_vsx_store(client_no, ins->tgt_dtype);
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
	/* now get a source register */
    cptr->num_ins_built=num_ins_built;					/* write the data again*/
	RA = get_random_gpr(client_no,ins->op3_dtype, 0);
	/* get immediate data for comparision */
	Imm_data = get_random_no_32(client_no) % 65536;
	Imm_data = Imm_data & 0x0000ffff;
	if (ins->op1_dtype == GR ) {
	    RB = get_random_gpr(client_no,ins->op1_dtype,1);
		num_ins_built = cptr->num_ins_built;
		tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
		op1 = ((RB & 0x1f) << (ins->op1_pos));       
	} else {
	   	op1 = ((Imm_data & 0x0000ffff) << (ins->op1_pos));   
	}
	/* now build instruction with L=0 or L=1 */
	if ((get_random_no_32(client_no) % 2) == 0) {
		op2 = 0;					        /* L bit is zero */
	} else {
		op2 = (1 << (ins->op2_pos));  		/* L value is one */
	}
	/* now build op3 */
	op3 = ((RA & 0x1f) << (ins->op3_pos));       
	mcode=(ins->op_eop_mask | op1 | op2 | op3 | tgt);           /* get the machine code */
	*tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_cache_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, mcode=0,*tc_memory,num_ins_built,prolog_size,RB;                
	uint32 store_mcode=0;
	int offset;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	char *ptr;
	ptr = cptr->ls_current[INITIAL_BUF];				/* point to load store area */
	prolog_size = cptr->prolog_size;
    /* Follow these steps
	 * 1. For all 4 instructions,get a register and offset (Double word boudary..)
	 * 2. Load that offset in to register
	 * 3. Do a store any register in to that offset,which will create a cacheline
	 * 4. Then build the instruction
	 */ 
    /* get offset with cache align size = 256 */

	/* Keep alignment in place for fpu/cpu run */
	/* Remove it for sctu as, mem allocatn routine is different */
	/* Another note, we ll not enable dcbz for sctu as it ll miscompare with current mem allocation design */
#ifndef SCTU
    ptr = (char *)(((uint64)ptr + 256 - 1) & ((uint64)~(256 - 1)));	
	cptr->ls_current[INITIAL_BUF] = ptr + 8;            /* 	Do a GPR store  */
	offset = (uint64)ptr - (uint64)cptr->ls_base[INITIAL_BUF];
	cptr->last_ls_off = offset;
#else
	char *updated_ptr;
	offset = get_mem_and_offset(client_no, 8, ptr, &updated_ptr);
#endif

	/* now load offset in to LOAD_RB register with a addi instruction */
	cpu_build_module(client_no,FC_ADDI_IMM,offset,LOAD_RB,index); /* get offset for load */
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* now generate a store word instruction */
    RB = get_random_gpr(client_no,ins->op2_dtype,0);              /* get a random source register */
	store_mcode = STORE_GPR(RB,STORE_RA,STORE_RB);
    *tc_memory = store_mcode;
	cptr->instr_index[prolog_size + num_ins_built] = stwx | 0x20000000;
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stwx;
	tc_memory++;
	num_ins_built++; 	
    /* Now build the actual instruction */
    op1 = LOAD_RA << (ins->op1_pos);
    op2 = LOAD_RB << (ins->op2_pos);	
	mcode = (ins->op_eop_mask | op1 | op2);
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}
	
void class_cpu_cache_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,store_off=0;                
	uint32 imm_value=0,offset=0;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
        
	/* get L value..it should be 1 or 3 */
    do{
		 imm_value=((get_random_no_32(client_no) % 4)); 	
	}while((imm_value == 0) || (imm_value == 2));
	/* load an available register with the offset(one bytesize ) using addi instruction */
	offset = 1;                              
	store_off = cpu_build_module(client_no,FC_ADDI,offset,LOAD_RB,index); /* get offset for load */
	prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);	
	/* Now build the actual instruction */
    op1 = LOAD_RA << (ins->op1_pos);
    op2 = LOAD_RB << (ins->op2_pos);	
	tgt = imm_value << (ins->tgt_pos); 
	mcode = (ins->op_eop_mask | op1 | op2 | tgt);
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
    tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_cache_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0, mcode=0, *tc_memory, num_ins_built, prolog_size, RB;                
	uint32 TH, RC,RD,random,rand_offset,mask, streamid,stop,random_upper,random_lower;
	uint32 offset=0, mod_offset=0;
	uint32 tharray[9]={0,1,3,8,10,11,16,24,17};
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	prolog_size = cptr->prolog_size;
    random = rand_offset = mask = 0;
	rand_offset = offset = init_mem_for_gpr(client_no, 1);		/* reserve 1 byte */
    streamid = offset & 0x0000000f;
    /* based on the POWER version generate TH values */
	if ( shifted_pvr_os >= SHIFTED_PVR_OS_P8 ) {
		TH=tharray[(get_random_no_32(client_no)%9)];           /* get any value */
	}else if ( shifted_pvr_os == 0x3f) {
		TH=tharray[(get_random_no_32(client_no)%8)];           /* get any value */
	} else {
		TH=tharray[(get_random_no_32(client_no)%4)];           /* get any value */
	}
	switch(TH){
	    case 8:
		    /* the UG bit is set to one..which says unlimited data stream */ 
			rand_offset = offset & 0x00000020;
		case 0:
		case 1:
		case 3:
		case 16:	
		case 24:	
		case 17:	
			cptr->num_ins_built = num_ins_built;
			/* load an available register with the offset(one bytesize ) using addi instruction */
			cpu_build_module(client_no,FC_ADDI_IMM,rand_offset,LOAD_RB,index);/* get offset*/
        	num_ins_built = cptr->num_ins_built;
			tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);	
			/* Now build the actual instruction */
	        op1 = LOAD_RA << (ins->op1_pos);
        	op2 = LOAD_RB << (ins->op2_pos);	
			tgt = TH << (ins->tgt_pos); 
			mcode = (ins->op_eop_mask | op1 | op2 | tgt);
        	*tc_memory = mcode;
			cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
        	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = rand_offset;
       		tc_memory++;
			num_ins_built++;
			break;
		case 11:
			/* With TH = 11,some more attributes like stride -distance between succcessive blocks are defined
			 * Here 3 instructions are built to describe the stream attributes
			 * 1) dcbt/dcbtst with TH = 8,UG = 0
			 * 2) dcbt/dcbtst with TH = 10 with streamId, UG = 0 and stop = 0
			 * 3) dcbt/dcbtst with TH = 11 and specifying the random stride and offset value
			 * 4) Again build a dcbt/dcbst with TH = 10 and GO bit set
			 */
		case 10:
			/* two cases here : GO = 1 and stop = 10 | 11.For dcbtst GO and STOP is not applicable*/
		    stop = (get_random_no_32(client_no)%2);	 		/* stop will be zero or one */
			/* for dcbtst and TH = 11,go to else part...*/
			if (((ins->op_eop_mask) == 0x7C0001EC) || (TH == 11)) {	
				stop = 0;
			}
			if (stop == 1) {					        /* this case is with GO = 0 */
				random = get_random_no_32(client_no);
				/* Now set GO to zero, stop to 10 or 11 and insert stream id */
				random = (random & 0x7fffffff);		/* random no. with GO =0 */
				random = (random | 0x40000000);		/* make stop = 10 or 11  */
				rand_offset = ((random & 0xfffffff0) | streamid); /* insert stream id in the EA*/
				/* RB = get_random_gpr(client_no,ins->op1_dtype,1); */
				RC = get_random_gpr(client_no,ins->op1_dtype,1);
				num_ins_built = cptr->num_ins_built;
				tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
				/* RB contains address and hxence should not be compared..so use R10*/
				/* from now use R6, as R10 is used for cpu_id_mask with THREADS_SYNC enabled */
				RB = 6;
				random_upper = (rand_offset) >> 16;
				random_lower = (rand_offset) & 0x0000ffff; 
				mcode = ORIS(RB,0,random_upper);
				*tc_memory = mcode;
				cptr->instr_index[prolog_size + num_ins_built] = oris | 0x20000000;
       			tc_memory++;
				num_ins_built++;
				mcode = ORI(RB,0,random_lower);
				*tc_memory = mcode;
				cptr->instr_index[prolog_size + num_ins_built] = ori | 0x20000000;
       			tc_memory++;
				num_ins_built++;
				op1 = (RB & 0x1f) << (ins->op1_pos);
				op2 = (RC & 0x1f) << (ins->op2_pos);
				tgt = TH << (ins->tgt_pos); 
				mcode = (ins->op_eop_mask | op1 | op2 | tgt);
				*tc_memory = mcode;
				cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       			tc_memory++;
				num_ins_built++;
			} else {
			    /* Here GO =1 and it means we are hinting that the data stream would be accessed.
			        To completly describe the stream -
				 1) Build a dcbt/dcbst with TH=8 and UG=0 
				 2) Then Build a dcbt/dcbtst with TH = 10 ,UG=0,STOP=0
				 3) Then Build a dcbt/dcbtst with TH = 10 ,UG=1,STOP=0
				 */
			    mod_offset = offset & 0xffffffdf;           /* reset UG bit for dcbt */
				/* load an available register with the offset(one bytesize ) using addi instruction */
			 	cptr->num_ins_built = num_ins_built;
				cpu_build_module(client_no,FC_ADDI_IMM,mod_offset,LOAD_RB,index); 
	        	num_ins_built = cptr->num_ins_built;
				tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);	
				/* Now build the actual instruction */
	        	op1 = LOAD_RA << (ins->op1_pos);
        		op2 = LOAD_RB << (ins->op2_pos);	
				tgt = 8 << (ins->tgt_pos); 			/* build for TH = 8 */
				mcode = (ins->op_eop_mask | op1 | op2 | tgt);
        		*tc_memory = mcode;
				cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
        		cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
       			tc_memory++;
				num_ins_built++;
			    /* Now build dcbt/dcbtst with TH=10 and UG=STOP=0 */
			    random = get_random_no_32(client_no);
				random = (random & 0x1fffffff);			/* random no. with GO=STOP =0 */ 	
			    rand_offset = ((random & 0xfffffff0) | streamid);    /* put the stream id */
				cptr->num_ins_built = num_ins_built;		/* update no. of instructions */
				/* RB = get_random_gpr(client_no,ins->op1_dtype,1);
				 num_ins_built = cptr->num_ins_built;
				 tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);*/
				RC = get_random_gpr(client_no,ins->op1_dtype,1);
				num_ins_built = cptr->num_ins_built;
				tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
				/* RB will have address.So use R10..it will not be compared  */
				/* from now use R6, as R10 is used for cpu_id_mask with THREADS_SYNC enabled */
				RB = 6;
				random_upper = (rand_offset) >> 16;
				random_lower = (rand_offset) & 0x0000ffff; 
				mcode = ORIS(RB,0,random_upper);
				*tc_memory = mcode;
				cptr->instr_index[prolog_size + num_ins_built] = oris | 0x20000000;
       			tc_memory++;
				num_ins_built++;
				mcode = ORI(RB,0,random_lower);
				*tc_memory = mcode;
				cptr->instr_index[prolog_size + num_ins_built] = ori | 0x20000000;
       			tc_memory++;
				num_ins_built++;
			    op1 = (RB & 0x1f) << (ins->op1_pos);
			    op2 = (RC & 0x1f) << (ins->op2_pos);
				/* For TH =11,always build dcbt with th=10.dcbtst should not be built*/
				if(TH == 11) {
				  mcode = DCBT(10,RB,RC);
				  *tc_memory = mcode;
				  cptr->instr_index[prolog_size + num_ins_built] = dcbt | 0x20000000;
				}else {
				  tgt = TH << (ins->tgt_pos); 
				  mcode = (ins->op_eop_mask | op1 | op2 | tgt);
				  *tc_memory = mcode;
				  cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
				}
       			tc_memory++;
				num_ins_built++;
				/* For TH = 11,build a dcbt/dcbtst with stride details */ 
				if (TH == 11) {
			        /* Use the same random number as above and just build a dcbt/dcbst with 
				     * TH = 11
				     */
				   	tgt = TH << (ins->tgt_pos); 
				   	mcode = (ins->op_eop_mask | op1 | op2 | tgt);
				   	*tc_memory = mcode;
				   	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       				tc_memory++;
				   	num_ins_built++;  		
				}
				/* Now build dcbt/dcbtst with TH = 10,UG =1 and stop = 0 */
				cptr->num_ins_built = num_ins_built;		/* update no. of instructions */
				/*
				RD = get_random_gpr(client_no,ins->op1_dtype,1);
				num_ins_built = cptr->num_ins_built;
				tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
				*/
				/* RD will have address and hence use R10.R10 will not be compared b/w passes */
				/* from now use R6, as R10 is used for cpu_id_mask with THREADS_SYNC enabled */
				RD = 6;
			    rand_offset = (rand_offset | 0x80000000);                /* set UG bit */
				random_upper = (rand_offset) >> 16;
			    mcode = ORIS(RD,0,random_upper);
				*tc_memory = mcode;
				cptr->instr_index[prolog_size + num_ins_built] = oris | 0x20000000;
       			tc_memory++;
				num_ins_built++;
				op1 = (RD & 0x1f) << (ins->op1_pos);
				op2 = (RC & 0x1f) << (ins->op2_pos);
				tgt = 10 << (ins->tgt_pos); 
				if(TH == 11) {
				  mcode = DCBT(10,RB,RC);
				  *tc_memory = mcode;
				  cptr->instr_index[prolog_size + num_ins_built] = dcbt | 0x20000000;
				}else {
				  mcode = (ins->op_eop_mask | op1 | op2 | tgt);
				  *tc_memory = mcode;
				  cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
				}
       			tc_memory++;
				num_ins_built++;
			}
			break;
		
		 default:
			sprintf(msg, "Unknown instruction in cache_2_gen routine ");
			hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
			exit(1);
	}

			
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_branch_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	
	uint32 op1=0, mcode=0,*tc_memory,num_ins_built,prolog_size,RT,RS,spr_reg=0;                
	uint32 offset=0;
	uint32 distance,negdistance,insttype=0,i;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	num_ins_built = cptr->num_ins_built;
	prolog_size = cptr->prolog_size;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

    /* only relative instructions are supported */
	/* distance is the space within which all branch instructions and filler inst. are built but
	 * excluding mt/mf spr instructions 
	*/
	distance = getdistance(client_no);

	if(ins->op_eop_mask & 0x00000001) {				/* link bit is set */
		insttype += 1;
	}
        if(takebackward_branch(client_no)){				/* check if we need to take backward branch*/
		insttype += 2;
	}

       switch(insttype){
	       case 0:							/* forward branch with no link */
		    /* 1) build branch instruction
			*  2) Fill all the inbetween space with addi and store instructions/no op
			*  3) But,these branch instructions should not be executed by the processor.
			*/
				/* Build actual branch instruction */
 			   	op1 = (distance) << (ins->tgt_pos);
			   	mcode = (ins->op_eop_mask | op1);		/* machine code */
				*tc_memory = mcode;
		       	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       			tc_memory++;
				num_ins_built++;

				/* Fill even space with addi and stores */
			    for(i=0;i<(distance-1)/2;i++){

					offset = init_mem_for_gpr(client_no, 8);
                    mcode = GEN_ADDI_MCODE(STORE_RB, 0, offset);
                    *tc_memory = mcode;
                    cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
                    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
                    tc_memory++;
       			    num_ins_built++;

					RS = get_random_gpr(client_no, GR, 0);
                    mcode = STDX(RS, STORE_RA, STORE_RB);
                    *tc_memory = mcode;
                    cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
                    cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
                    tc_memory++;
                    num_ins_built++;

       			}

				if((distance-1)%2 != 0){  /* build no op for extra space */
					*tc_memory = ORI(0,0,0);
					cptr->instr_index[prolog_size + num_ins_built] = ori | 0x20000000;
					cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_ori;
			        tc_memory++;
					num_ins_built++;
				}

	       	 	break;

	       case 1:							/* forward branch with link */
			/* 1) First save link register value 
			 * 2) Build branch instruction and fill space in between
			 * 3) At the end,restore back link register value
			 */
			    /*RT = get_random_gpr(client_no,GR,1);*/		/* get a random register    */
				/* from now use R6, as R10 is used for cpu_id_mask with THREADS_SYNC enabled */
			    RT = 6;
			    spr_reg = 8*32;
			    mcode = MFSPR(RT,spr_reg);			/* save link register value in RT */
			    *tc_memory = mcode;
		        cptr->instr_index[prolog_size + num_ins_built] = mfspr | 0x20000000;
       		    tc_memory++;
			    num_ins_built++;

				/* Build actual branch instruction */
 			   	op1 = (distance) << (ins->tgt_pos);
			   	mcode = (ins->op_eop_mask | op1);		/* machine code */
				*tc_memory = mcode;
		       	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       			tc_memory++;
				num_ins_built++;

				/* Fill even space with addi and stores */
			    for(i=0;i<(distance-1)/2;i++){

					offset = init_mem_for_gpr(client_no, 8);
                    mcode = GEN_ADDI_MCODE(STORE_RB, 0, offset);
                    *tc_memory = mcode;
                    cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
                    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
                    tc_memory++;
       			    num_ins_built++;

					RS = get_random_gpr(client_no, GR, 0);
                    mcode = STDX(RS, STORE_RA, STORE_RB);
                    *tc_memory = mcode;
                    cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
                    cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
                    tc_memory++;
                    num_ins_built++;

       			}

				if((distance-1)%2 != 0){  /* build no op for extra space */
					*tc_memory = ORI(0,0,0);
					cptr->instr_index[prolog_size + num_ins_built] = ori | 0x20000000;
					cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_ori;
			        tc_memory++;
					num_ins_built++;
				}

			    mcode = MTSPR(spr_reg,RT);			/* restore link register value from RT */
			    *tc_memory = mcode;
		        cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
       			tc_memory++;
			    num_ins_built++;
			    break;

	       case 3:							/* backward branch with link */
			    /* 1) Build branch forward to selected instruction
			     * 2) Fill all intervening space with addi and stores
			     * 3) Then build branch with backward branch
			     * 4) For Insttype = 3, first save link and restore it back in the end */
			    /*RT = get_random_gpr(client_no,GR,1);*/		/* get a random register    */
				/* from now use R6, as R10 is used for cpu_id_mask with THREADS_SYNC enabled */
			    RT = 6;
			    spr_reg = 8*32;
			    mcode = MFSPR(RT,spr_reg);			/* save link register value in RT */
			    *tc_memory = mcode;
		            cptr->instr_index[prolog_size + num_ins_built] = mfspr | 0x20000000;
       			    tc_memory++;
			    num_ins_built++;
	       case 2:
	     		 /* backward branch with no link */
			   	negdistance = (uint32) - ((distance) -2);		/* backward branch distance*/
       			   negdistance &= 0x00ffffff;				/* get 24 bit value */

				/* Build actual branch instruction twice*/
				for ( i=0; i < 2; i++ ) {
 			   		op1 = (distance-1) << (ins->tgt_pos);
			   		mcode = (ins->op_eop_mask | op1);		/* machine code */
					*tc_memory = mcode;
		       		cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       				tc_memory++;
					num_ins_built++;
				}

				/* Fill even space with addi and stores */
			    for(i=0;i<(distance-3)/2;i++){

					offset = init_mem_for_gpr(client_no, 8);
                    mcode = GEN_ADDI_MCODE(STORE_RB, 0, offset);
                    *tc_memory = mcode;
                    cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
                    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
                    tc_memory++;
       			    num_ins_built++;

					RS = get_random_gpr(client_no, GR, 0);
                    mcode = STDX(RS, STORE_RA, STORE_RB);
                    *tc_memory = mcode;
                    cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
                    cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
                    tc_memory++;
                    num_ins_built++;

       			}

				if((distance-3)%2 != 0){  /* build no op for extra space */
					*tc_memory = ORI(0,0,0);
					cptr->instr_index[prolog_size + num_ins_built] = ori | 0x20000000;
					cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_ori;
			        tc_memory++;
					num_ins_built++;
				}

				op1 = (negdistance) << (ins->tgt_pos);
			   	mcode = (ins->op_eop_mask | op1);		        /* machine code */			   
	       	    *tc_memory = mcode;
		        cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       		    tc_memory++;
			    num_ins_built++;
				if( insttype == 3) {
					mcode = MTSPR(spr_reg,RT);			/* restore link register value from RT */
			        *tc_memory = mcode;
		            cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
       			    tc_memory++;
			        num_ins_built++;
				} 
				break;		   
	       
	       default:  sprintf(msg, "Unknown instruction in branch_1_gen routine ");
			 hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
			 exit(1);
       } 
			
       cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_branch_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	
	uint32 op1=0, op2=0, tgt=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT,RS, spr_reg=0;                
	uint32 offset=0,BO=0,counter_val=0,savelink=0,NOT_TAKEN=0;
	uint32 distance,negdistance,insttype=0,i,BI=0;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	num_ins_built = cptr->num_ins_built;
	prolog_size = cptr->prolog_size;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

    /* only relative instructions are supported */
	/* distance is the space within which all branch instructions and filler inst. are built but
	 * excluding mt/mf spr instructions 
	*/
	distance = getdistance(client_no);				/* get distance */
	negdistance = -(distance - 2);					/* backward branch distance is obtained */
	negdistance &= 0x00003fff;					/* limit it to 14 bits */
	if(ins->op_eop_mask & 0x00000001) {				/* link bit is set */
		insttype += 1;
	}

    RT = get_random_gpr(client_no,GR,1);				/* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
        savelink = 6;							/* get a register to save link register value */
	BO = get_random_no_32(client_no)%32;				/* BO varies from 0 to 31 */
	BI = get_random_no_32(client_no)%20;				/* BI varies from 0 to 19;CR bit value*/
	if((get_random_no_32(client_no)%3) == 0){			/* randomly choose to branch or not to branch*/
		NOT_TAKEN  = 1;						/* yes we will not take the branch */
		if ( (BO & 0x10) && (BO & 0x04) ) {	/* this condition is branch always so build as taken */
			NOT_TAKEN = 0;
		}
	}

	switch(insttype){
		case 1:							/* branch with link    */
			/* Save the link register in the beginning and restore it back in the end */
			spr_reg = 8*32;						/* do a left shift for link       */
			mcode = MFSPR(savelink,spr_reg);			/* save link register value in RT */
			*tc_memory = mcode;
		        cptr->instr_index[prolog_size + num_ins_built] = mfspr | 0x20000000;
       			tc_memory++;
			num_ins_built++;
			/* rest it same as case 0,except that restore back link register in the end */
		case 0:							/* branch with no link */
			  if(!(BO & 0x04)){				/* Cases where Counter Matters*/
				  if(BO & 0x02){			/* Branch when counter is decremented to zero*/
					counter_val=1;			/* when decremented it will be zero */
					 if(NOT_TAKEN){			/* do not want to take the branch */
					   counter_val=10;		/* some non-zero number so that when
									 decremented it will be non zero */
					}
				  } else {			     /* branch when decremented counter is non zero*/	
		                        counter_val = 10;	        /* some non zero value */	
			     	 	if(NOT_TAKEN){		     
					  counter_val=1;		/* this will make counter value zero,so no branch*/
					}					
				  }					/* end of if else*/
			        /* now build the addi and mtspr instruction */
			 	cptr->num_ins_built = num_ins_built;
				cpu_build_module(client_no,FC_ADDI_IMM,counter_val,RT,index);/*addi*/
        		num_ins_built = cptr->num_ins_built;
			    tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);	
				/* now build mtspr and pass on the counter value to Counter Register */
				spr_reg = 9*32;					/* COunter register is shifted */
				mcode = MTSPR(spr_reg,RT);			/* build MTSPR */
				*tc_memory = mcode;
		                cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
       			        tc_memory++;
			        num_ins_built++;
			  }						/* end of BO checking if */
			/* Now set the branch condition*/
			  if(!(BO & 0x10)){				/* only where condition matters*/
				  if(BO & 0x08){			/* CR bit must be set to branch */
					  if(NOT_TAKEN){
						  	mcode=CRXOR(BI,BI,BI);			/* clear CR bit - not taken*/
		                			cptr->instr_index[prolog_size + num_ins_built] = crxor | 0x20000000;
					  }else{
						  	mcode=CREQV(BI,BI,BI);			/* set CR bit -taken case */
		                			cptr->instr_index[prolog_size + num_ins_built] = creqv | 0x20000000;
					  }
				  } else{					/* CR bit must be clear for taking branch*/
					 if(NOT_TAKEN){
						  	mcode=CREQV(BI,BI,BI);			/* set CR bit - not taken*/
		                			cptr->instr_index[prolog_size + num_ins_built] = creqv | 0x20000000;
					  }else{
						  	mcode=CRXOR(BI,BI,BI);		/* clear CR bit -taken case */
		                			cptr->instr_index[prolog_size + num_ins_built] = crxor | 0x20000000;
					  }
				  }
       			        	  *tc_memory =mcode;	
				          tc_memory++;
			        	  num_ins_built++;
				  }					 		/* end of BO checking if */
			  /* Now build instruction*/
			  if(takebackward_branch(client_no)){				/* build backward branch  */

				for ( i=0 ; i<2 ; i++ ) { /* build same branch twice */
				mcode = BRANCH(distance-1);						/* build branch */
       			       	*tc_memory =mcode;	
		               	cptr->instr_index[prolog_size + num_ins_built] = branch | 0x20000000;
       			        tc_memory++;
			        num_ins_built++;
				}

				/* Fill even space with addi and stores */
			    for(i=0;i<(distance-3)/2;i++){

					offset = init_mem_for_gpr(client_no, 8);
                    mcode = GEN_ADDI_MCODE(STORE_RB, 0, offset);
                    *tc_memory = mcode;
                    cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
                    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
                    tc_memory++;
       			    num_ins_built++;

					RS = get_random_gpr(client_no, GR, 0);
                    mcode = STDX(RS, STORE_RA, STORE_RB);
                    *tc_memory = mcode;
                    cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
                    cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
                    tc_memory++;
                    num_ins_built++;
       			}

				if((distance-3)%2 == 1){  /* build no op for extra space */
					*tc_memory = ORI(0,0,0);
					cptr->instr_index[prolog_size + num_ins_built] = ori | 0x20000000;
					cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_ori;
			        tc_memory++;
					num_ins_built++;
				}
			        /* Now build actual backward branch instruction */
		                tgt = (negdistance) << (ins->tgt_pos);
				op1 = BO << (ins->op1_pos);				/* BO is placed */
				op2 = BI << (ins->op2_pos);				/* BI is placed */
				mcode = (ins->op_eop_mask | op1	| op2 | tgt );	
       			       	*tc_memory =mcode;	
		               	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       			       	tc_memory++;
			       	num_ins_built++;
			  } else {							/*  forward branch */
				tgt = (distance) << (ins->tgt_pos);
				op1 = BO << (ins->op1_pos);				/* BO is placed */
				op2 = BI << (ins->op2_pos);				/* BI is placed */
				mcode = (ins->op_eop_mask | op1	| op2 | tgt );	
       			       	*tc_memory =mcode;	
		               	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       			       	tc_memory++;
			       	num_ins_built++;
			
				if( !NOT_TAKEN ) {  /* Fill unused space with instructions only if branch is taken */

					/* Fill even space with addi and stores */
			    	for(i=0;i<(distance-1)/2;i++){

						offset = init_mem_for_gpr(client_no, 8);
                    	mcode = GEN_ADDI_MCODE(STORE_RB, 0, offset);
                    	*tc_memory = mcode;
                    	cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
                    	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
                    	tc_memory++;
       			    	num_ins_built++;

						RS = get_random_gpr(client_no, GR, 0);
                    	mcode = STDX(RS, STORE_RA, STORE_RB);
                    	*tc_memory = mcode;
                    	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
                    	cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
                    	tc_memory++;
                    	num_ins_built++;

       			    }

					if((distance-1)%2 == 1){  /* build no op for extra space */
						*tc_memory = ORI(0,0,0);
						cptr->instr_index[prolog_size + num_ins_built] = ori | 0x20000000;
						cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_ori;
			        	tc_memory++;
						num_ins_built++;
					}

				}

			 }	
	  		  if(insttype == 1) {
			     	spr_reg = 8*32;					/* restore link register */
				mcode = MTSPR(spr_reg,savelink);		/* restore link register value from reg.*/
			        *tc_memory = mcode;
		                cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
       			        tc_memory++;
			        num_ins_built++;
			   } 							/* restore link */
			  break;
		
		default: sprintf(msg, "Unknown instruction in class_cpu_branch_2_gen routine ");
			 hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
			 exit(1);
	}								/* end of switch */

    cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}	

void class_cpu_branch_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	
	uint32 op1=0, op2=0, mcode=0,*tc_memory,num_ins_built,prolog_size,RT,spr_reg=0;                
	uint32 BO=0,counter_val=0,savelink=0,NOT_TAKEN=0;
	uint32 distance,negdistance,insttype=0,i,temp_distance=0,BI=0,negbranch=0,ctr_bit,addl_inst=0;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	/* only relative instructions are supported */
	distance = getdistance(client_no);			/* get distance */
	negdistance = -distance;					/* backward branch distance is obtained */
	negdistance &= 0x00ffffff;					/* limit it to 24 bits */
	if((ins->op_eop_mask == 0x4C000020) || (ins->op_eop_mask == 0x4C000021)) {			/* if bclr/bclrl */
		insttype += 1;
	}
	if( (ins->op_eop_mask == 0x4C000460)||(ins->op_eop_mask == 0x4C000461) ){  /* bctar and bctarl */
		insttype += 1;
	}
        RT = get_random_gpr(client_no,GR,1);	/* get a register */
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
    savelink = 6;								/* get a register to save link register value */
	BO = get_random_no_32(client_no)%32;		/* BO varies from 0 to 31 */
	BO = BO & 0x1f;
	BI = get_random_no_32(client_no)%20;		/* BI varies from 0 to 19;CR bit value*/
	BI = BI & 0x1f;
	if((get_random_no_32(client_no)%3) == 0){	/* randomly choose to branch or not to branch*/
		NOT_TAKEN  = 1;							/* yes we will not take the branch */
	}
	switch(insttype){
		case 0:									/* bcctr and bcctrl */
			/* Here counter register acts as target address.
			 * 1) Remove BO conditions which refers to counter register
			 * 2) Use branch with link instruction which stores the next inst. address in link register.
			 * 3) Now,move that link register to counter register using MTSPR and MFSPR
			 * 4) Build actual instruction and also fill the gaps.
			 */
			/* Step 1. Remove references to counter register in BO */
			 BO |= 0x04;					/* check BO encodings in document */
			 BO &= 0x1D;					/* These two statements will remove any reference to counter register */
	      	 ctr_bit = 1;					/* check if we need control register bit */
			 if (BO & 0x10) {
				 ctr_bit = 0;				/* control register bit is not needed */
			 }
			 if(takebackward_branch(client_no)){
				negbranch = 1;				/* yes we are taking backward branch */
			 }				
			 /* Do setup instructions for backward branch */
		 	 if(negbranch){	
				/* build a branch with link instruction which jumps forward.this will set the link register 
				 * with next instruction address */
				temp_distance = ((distance) - (1 + ctr_bit));     
				temp_distance = temp_distance;
				mcode = BRLINK(temp_distance);			/* This will build bl inst.*/
			    *tc_memory = mcode;
		        cptr->instr_index[prolog_size + num_ins_built] = brlink | 0x20000000;
       			tc_memory++;
			    num_ins_built++;
				/* fill space in between with some branch instructions */
				for(i=0;i<(distance - (2+ctr_bit));i++){
					temp_distance = distance - i + 1;
					mcode=BRANCH(temp_distance);
					*tc_memory = mcode;
		            cptr->instr_index[prolog_size + num_ins_built] = branch | 0x20000000;
       			    tc_memory++;
			        num_ins_built++;
				}
			 }else{
				 /* build a branch forward instruction which will jump to mtspr instructions*/
				 temp_distance = ((distance) + (2 + ctr_bit));
				 mcode=BRANCH(temp_distance);
				 *tc_memory = mcode;
		         cptr->instr_index[prolog_size + num_ins_built] = branch | 0x20000000;
       			 tc_memory++;
			     num_ins_built++;
			 }
			 /* Now build a MFSPR,which will copy link register value to a register */
			spr_reg = 8*32;						/* do a left shift for link       */
			mcode = MFSPR(savelink,spr_reg);			/* save link register value in RT */
			*tc_memory = mcode;
		    cptr->instr_index[prolog_size + num_ins_built] = mfspr | 0x20000000;
       		tc_memory++;
			num_ins_built++; 	 
			/* Now move the contents of savelink register to counter register */
			spr_reg = 9*32;					 
			mcode = MTSPR(spr_reg,savelink);		 
			*tc_memory = mcode;
		    cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
       		tc_memory++;
			num_ins_built++;
			/* Now set the branch condition*/
			if (!(BO & 0x10)){				/* only where condition matters*/
				if (BO & 0x08){			/* CR bit must be set to branch */
					if (NOT_TAKEN){
						mcode=CRXOR(BI,BI,BI);			/* clear CR bit - not taken*/
		                cptr->instr_index[prolog_size + num_ins_built] = crxor | 0x20000000;
					}else{
						mcode=CREQV(BI,BI,BI);			/* set CR bit -taken case */
		                cptr->instr_index[prolog_size + num_ins_built] = creqv | 0x20000000;
					}
				}else{					/* CR bit must be clear for taking branch*/
					if (NOT_TAKEN){
						mcode=CREQV(BI,BI,BI);			/* set CR bit - not taken*/
		                cptr->instr_index[prolog_size + num_ins_built] = creqv | 0x20000000;
					}else{
						mcode=CRXOR(BI,BI,BI);		/* clear CR bit -taken case */
		                cptr->instr_index[prolog_size + num_ins_built] = crxor | 0x20000000;
					}
				}
       			*tc_memory =mcode;	
				tc_memory++;
			    num_ins_built++;
			}
			/* build instruction */
			op1 = BO << (ins->op1_pos);
			op2 = BI << (ins->op2_pos);
			mcode = (ins->op_eop_mask | op1 | op2 );		/* BH does not matter */
			*tc_memory =mcode;	
			cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
			tc_memory++;
			num_ins_built++;
			if(negbranch){
				;
			} else {							/* fill in spaces  */
				for(i=0;i<(distance - 2);i++){
					temp_distance = (distance - i - 1);
					mcode=BRANCH(temp_distance);
					*tc_memory = mcode;
		                	cptr->instr_index[prolog_size + num_ins_built] = branch | 0x20000000;
       			        	tc_memory++;
			        	num_ins_built++;
				}
				/* Now build a branch back with link which will jump to actual instruction */
				temp_distance = negdistance - (1 + ctr_bit);
				temp_distance = temp_distance;
				mcode=BRLINK(temp_distance);
				*tc_memory = mcode;
		        cptr->instr_index[prolog_size + num_ins_built] = brlink | 0x20000000;
       			tc_memory++;
			    num_ins_built++;
			}
		    break; 		  					 /* end of BO checking if */
		case  1:
			addl_inst = 0;
			if( (ins->op_eop_mask == 0x4C000460)||(ins->op_eop_mask == 0x4C000461) ){
		 		addl_inst = 2;				/* additional two instructions - mfspr and
									   mtspr for bctar and bctarl*/
			 }
			/* Set the counter value */
			 if(!(BO & 0x04)){				/* Cases where Counter Matters*/
				  if(BO & 0x02){			/* Branch when counter is decremented to zero*/
					counter_val=1;			/* when decremented it will be zero */
					 if(NOT_TAKEN){			/* do not want to take the branch */
					   counter_val=10;		/* some non-zero number so that when
									 decremented it will be non zero */
					}
				  } else {			     /* branch when decremented counter is non zero*/	
		                        counter_val = 10;	        /* some non zero value */	
			     	 	if(NOT_TAKEN){		     
					  counter_val=1;		/* this will make counter value zero,so no branch*/
					}					
				  }					/* end of if else*/
			 	/* now build the addi instruction */
			 	cptr->num_ins_built = num_ins_built;
				cpu_build_module(client_no,FC_ADDI_IMM,counter_val,RT,index);/*addi*/
        		num_ins_built = cptr->num_ins_built;
			 	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
			 	/* now build mtspr and pass on the counter value to Counter Register */
				spr_reg = 9*32;					/* COunter register is shifted */
				mcode = MTSPR(spr_reg,RT);			/* build MTSPR */
				*tc_memory = mcode;
		        cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
       			tc_memory++;
			    num_ins_built++;
			  }						/* end of BO checking if */ 			
			/* Set the branch condition*/
			if (!(BO & 0x10)) {				/* only where condition matters*/
				if (BO & 0x08) {			/* CR bit must be set to branch */
					if (NOT_TAKEN) {
						mcode=CRXOR(BI,BI,BI);			/* clear CR bit - not taken*/
		                cptr->instr_index[prolog_size + num_ins_built] = crxor | 0x20000000;
					}else{
						mcode=CREQV(BI,BI,BI);			/* set CR bit -taken case */
		                cptr->instr_index[prolog_size + num_ins_built] = creqv | 0x20000000;
					}
				}else{					/* CR bit must be clear for taking branch*/
					if (NOT_TAKEN){
						mcode=CREQV(BI,BI,BI);			/* set CR bit - not taken*/
		                cptr->instr_index[prolog_size + num_ins_built] = creqv | 0x20000000;
					}else{
						mcode=CRXOR(BI,BI,BI);		/* clear CR bit -taken case */
		                cptr->instr_index[prolog_size + num_ins_built] = crxor | 0x20000000;
					}
				}
       			*tc_memory =mcode;	
				tc_memory++;
			    num_ins_built++;
			}
			 
			 /* Now build instruction*/
			  if(takebackward_branch(client_no)){					/* build backward branch  */
				temp_distance = distance + 1;			/* jump to actual inst    */
				mcode = BRLINK(temp_distance);				/* build branch           */
       			       	*tc_memory =mcode;	
		               	cptr->instr_index[prolog_size + num_ins_built] = brlink | 0x20000000;
       			        tc_memory++;
			        num_ins_built++;
				/* now fill the space in between with branch instructions */
				for(i=0;i<distance;i++){
					temp_distance = distance-i+1+addl_inst;        	/* branch to actual instruction*/
					mcode = BRANCH(temp_distance);			/* build branch           */
       			        	*tc_memory =mcode;	
					cptr->instr_index[prolog_size + num_ins_built] = branch | 0x20000000;
       			        	tc_memory++;
			        	num_ins_built++;
				}
				/* Build mfspr and mtspr for bctar and bctarl */
				if( (ins->op_eop_mask == 0x4C000460)||(ins->op_eop_mask == 0x4C000461) ){
					/* Now Build mfspr and transfer link register contents to RT */
					/* If instruction is BCTAR,then transfer the contents of link register to TAR */
					/* BCTAR is 815 decimal. For,MFSPR and MTSPR the bits are reversed.
				 	 * Hence,815 = '1100101111'b is reversed to get 0X1F9 = '0111111001'b
				 	 */
				        spr_reg = 8 * 32;			/* Read from link register  */
					mcode = MFSPR(savelink,spr_reg);		/* save link register value in RT */
					*tc_memory = mcode;
		        		cptr->instr_index[prolog_size + num_ins_built] = mfspr | 0x20000000;
       					tc_memory++;
					num_ins_built++;
					/* now build mtspr and pass on the counter value to Counter Register */
					spr_reg = 0x1F9;				/* BCTAR is shifted */
					mcode = MTSPR(spr_reg,savelink);			/* build MTSPR */
					*tc_memory = mcode;
		                	cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
       			        	tc_memory++;
			        	num_ins_built++;
				}
			        /* Now build actual backward branch instruction */
				op1 = BO << (ins->op1_pos);				/* BO is placed */
				op2 = BI << (ins->op2_pos);				/* BI is placed */
				mcode = (ins->op_eop_mask | op1	| op2 );	
       			       	*tc_memory =mcode;	
		               	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       			       	tc_memory++;
			       	num_ins_built++;
			  }else {							/*  forward branch */
				temp_distance = (distance);				/* jump to actual inst    */
				mcode = BRANCH(temp_distance);				/* build branch           */
       			       	*tc_memory =mcode;	
		               	cptr->instr_index[prolog_size + num_ins_built] = branch | 0x20000000;
       			        tc_memory++;
			        num_ins_built++;
				/* If instruction is BCTAR,then transfer the contents of link register to TAR */
				/* BCTAR is 815 decimal. For,MFSPR and MTSPR the bits are reversed.
				 * Hence,815 = '1100101111'b is reversed to get 0X1F9 = '0111111001'b
				 */
				if( (ins->op_eop_mask == 0x4C000460)||(ins->op_eop_mask == 0x4C000461) ){
			 		addl_inst = 2;				/* additional two instructions
										   mfspr and mtspr */
					/* Now Build mfspr and transfer link register contents to RT */
				        spr_reg = 8 * 32;				/* Save from link  */
					mcode = MFSPR(savelink,spr_reg);		/* save link register value in RT */
					*tc_memory = mcode;
		        		cptr->instr_index[prolog_size + num_ins_built] = mfspr | 0x20000000;
       					tc_memory++;
					num_ins_built++;
					/* now build mtspr and pass on the counter value to Counter Register */
					spr_reg = 0x1F9;			/* pass it on to TAR */			
					mcode = MTSPR(spr_reg,savelink);		/* build MTSPR */
					*tc_memory = mcode;
		                	cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
       			        	tc_memory++;
			        	num_ins_built++;
				}
				/* Now build instruction */
				op1 = BO << (ins->op1_pos);				/* BO is placed */
				op2 = BI << (ins->op2_pos);				/* BI is placed */
				mcode = (ins->op_eop_mask | op1	| op2 );	
       			       	*tc_memory =mcode;	
		               	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       			       	tc_memory++;
			       	num_ins_built++;
				for(i=0;i<(distance-2-addl_inst);++i){
       			       		temp_distance = (distance-i-1-addl_inst);	/* jump out of loop       */
					mcode = BRANCH(temp_distance);			/* build branch           */
					*tc_memory =mcode;	
		               		cptr->instr_index[prolog_size + num_ins_built] = branch | 0x20000000;
       			        	tc_memory++;
			        	num_ins_built++;
				}
				/* Now build a bl instruction branching back to bclr or bclrl */
				temp_distance = ((negdistance+1));			/* jump to actual inst    */
				mcode = BRLINK(temp_distance);				/* build branch           */
       			       	*tc_memory =mcode;	
		               	cptr->instr_index[prolog_size + num_ins_built] = brlink | 0x20000000;
       			        tc_memory++;
			        num_ins_built++;
			  }
			break;	
		
		
		default: sprintf(msg, "Unknown instruction in branch_3_gen routine ");
			 hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
			 exit(1);
	}								/* end of switch */

    cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}	

void class_cpu_branch_4_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{

	uint32 op1=0, op2=0,mcode=0,*tc_memory,num_ins_built,prolog_size,RT,RS,RA,RB,spr_reg=0;                
	uint32 offset=0,BO=0,counter_val=0;
	uint32 temp_distance=0,BI=0;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	num_ins_built = cptr->num_ins_built;
	prolog_size = cptr->prolog_size;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	BO = get_random_no_32(client_no)%32;				/* BO varies from 0 to 31 */
	counter_val = get_random_no_32(client_no) % 11;		/* get a value for CTR b/w 0 and 10 */

	/* only relative instructions are supported */
	/* distance = 2;	 use LI field encoded distance for bc $+8 */
	/* this routine is being used exclusively for bc $+8 testing with LI field encoded in the instruction table 
	 *  we are building test cases for conditions where only CTR matters OR only CR matters 
	 */

	/* Clear 3rd bit (from left) of BO field.
	 * This is because if that bit is set, it will mean branch always (unconditional branch).
	 * But in BC $+8, it should always be conditional jump.
	 */
	BO &= 0x1b;
	
	if ( BO & 0x10 ) { /* Cases where only counter matters */

		/* Populate a GPR with counter value and use mtspr to move the value to CTR
		*  build branch(bc) instruction 
		*  build b $-4 to loop till condition is met, take care of forever looping
		*/

		if ( BO & 0x02 && counter_val == 0 ) { /* branch when decremented CTR = 0 , hence make CTR non zero to avoid forever looping */
			counter_val += 1;
		}
		
        RT = get_random_gpr(client_no,GR,1);				/* get a register */
		num_ins_built = cptr->num_ins_built;
		tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

		mcode = GEN_ADDI_MCODE(RT, 0, counter_val);
		*tc_memory = mcode;
		cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
		tc_memory++;
		num_ins_built++;

	 	/* now build mtspr and pass on the counter value to Counter Register */
		spr_reg = 9*32;					/* COunter register is shifted */
		mcode = MTSPR(spr_reg,RT);			/* build MTSPR */
		*tc_memory = mcode;
	    cptr->instr_index[prolog_size + num_ins_built] = mtspr | 0x20000000;
        tc_memory++;
	    num_ins_built++;

		/* Now build instruction */
		op1 = BO << (ins->op1_pos);				/* BO is placed */
		op2 = BI << (ins->op2_pos);				/* BI is placed */
		mcode = (ins->op_eop_mask | op1	| op2 );	
    	*tc_memory =mcode;	
		cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       	tc_memory++;
		num_ins_built++;

		/* now build b $-4 for looping */
		temp_distance = -1;
		temp_distance = 0x00ffffff;				 /* limit to 24 bits for li field */
		mcode = BRANCH(temp_distance);			/* build branch */
		*tc_memory =mcode;	
	    cptr->instr_index[prolog_size + num_ins_built] = branch | 0x20000000;
       	tc_memory++;
		num_ins_built++;
	}


	if ( BO & 0x4 ) { /* Cases where only CR matters */

		/* get two GPRs and compare their values using cmp instruction to update CR
		*  build addi for store after branch inst.
		*  build branch instruction with BI value randomized but in CR0 field
		*  build store instruction, which should not go through if branch is taken
		*/

        RA = get_random_gpr(client_no,GR,0);				/* get a register */
        RB = get_random_gpr(client_no,GR,0);				/* get a register */

		/* build compare instruction */
		mcode = CMP(0,1,RA,RB);			/* build cmpd and update crf0 */
		*tc_memory = mcode;
	    cptr->instr_index[prolog_size + num_ins_built] = cmp | 0x20000000;
        tc_memory++;
	    num_ins_built++;

		offset = init_mem_for_gpr(client_no, 8);
        mcode = GEN_ADDI_MCODE(STORE_RB, 0, offset);
        *tc_memory = mcode;
        cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
        tc_memory++;
		num_ins_built++;

		BI = BO % 3; /* Use a bit for comparison check in CR field 0 */
		/* Now build instruction */
		op1 = BO << (ins->op1_pos);				/* BO is placed */
		op2 = BI << (ins->op2_pos);				/* BI is placed */
		mcode = (ins->op_eop_mask | op1	| op2 );	
    	*tc_memory =mcode;	
		cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
       	tc_memory++;
		num_ins_built++;

		RS = get_random_gpr(client_no, GR, 0);
        mcode = STDX(RS, STORE_RA, STORE_RB);
        *tc_memory = mcode;
        cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
        cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
       	tc_memory++;
        num_ins_built++;

	}

    cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}

void class_cpu_fixed_load_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr4,i;
	uint32 op1, op2, tgt,RT,op3,pt_value;
	uint32 mcode, store_mcode, store_off, load_off;
	uint32 num_ins_built, prolog_size, *tc_memory;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	struct vsr_list *vsrs;
	vsrs = &cptr->vsrs[GR];
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
    RT   = (temp->op_eop_mask & 0xfc000000) >> 26;
    if (RT == 56) {
		cptr->bdy_req = 16;							/* quadword boundary requirement */
	   	/*size = ALIGN_QW;*/						/* 16 bytes needed in memory*/
	} 
	switch(RT)   
	{
		case 56:								/* lq instruction */
			do{
		     	vsr4 = vsrs->head[BFP]->vsr_no;
		   		vsr4 = vsr4 & 0x1f;
				MOVE_VSR_TO_END(client_no, GR, BFP);				/* Move the pointer*/
		    }while((vsr4%2) ==1);					/* Get even register*/
		 	/* Store and Mark vsr4 and vsr4 + 1 as dirty */
			i = vsr4;							/* Initialize      */
			do{
				if ((0x1ULL << i ) & vsrs->dirty_mask) {		/* store target if dirty */
					store_off = init_mem_for_gpr(client_no,8);
					store_off &= 0x0000ffff;
					store_mcode = STD(i,store_off,LOAD_RA);
					*tc_memory = store_mcode;
					cptr->instr_index[prolog_size + num_ins_built] = std | 0x20000000;
					cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
					tc_memory++;
					num_ins_built++;
				}
				vsrs->dirty_mask |= ((0x1ULL << i ));			/* mark it as dirty      */
			   	i++;
		   	}while(i < (vsr4 + 2));
		   	pt_value = get_random_no_32(client_no)%16;
		   	load_off = init_mem_for_gpr(client_no,ALIGN_QW);
		   	load_off &= 0x00ffffff;						/* 12 bit offset */
		   	tgt = (vsr4 << temp->tgt_pos);					/* Target register */
		   	op1 = (load_off << (temp->op1_pos));
		   	op2 = (LOAD_RA  << (temp->op2_pos));
		   	op3 = (pt_value << (temp->op3_pos));
		   	mcode = ( (temp->op_eop_mask) | tgt | op1 | op2 | op3 );
		   	*tc_memory = mcode;
		    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
			cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = load_off;
		   	tc_memory++;
		   	num_ins_built++;
			break;			
		default: 
			sprintf(msg, "Unknown instruction in fixed_load_2_gen routine ");
			hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
			exit(1);
	}
	cptr->bdy_req = 0;							/* quadword boundary requirement */
    cptr->num_ins_built = num_ins_built;		/* Update number of instructions */
}

void class_cpu_cache_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0, tgt=0,*tc_memory,num_ins_built,prolog_size,rand_num=0;                
	uint32 CT=0,store_mcode=0;
	int offset;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	char *ptr;
	ptr = cptr->ls_current[INITIAL_BUF];				/* point to load store area */
	prolog_size = cptr->prolog_size;
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
    /* Follow these steps
	 * 1. For icbt,get an address.But,do not reserve
	 * 2. Load that offset in to register
	 * 3. Then build the instruction
	 */ 
    /* get offset with cache align size = 256 */
    ptr = (char *)(((uint64)ptr + 256 - 1) & ((uint64)~(256 - 1)));	
    offset = (uint64)ptr - (uint64)cptr->ls_base[INITIAL_BUF];
	rand_num = get_random_no_32(client_no)%2;
	if (rand_num %2 ==0) {
		CT = 0;							/* Primary cache */
	} else {
		CT = 2;							/* secondary cache */
	}
	/* now build addi instruction */
	store_mcode = GEN_ADDI_MCODE(LOAD_RB,0,offset);
    *tc_memory = store_mcode;
	cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
    cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
	tc_memory++;
    num_ins_built++;
	/* now build icbt instruction */
	op1 = (CT << (ins->op2_pos));
	op2 = (LOAD_RA << (ins->op3_pos));
	tgt = (LOAD_RB << (ins->tgt_pos));
    *tc_memory=(ins->op_eop_mask | op1 | op2  | tgt ); 
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
	tc_memory++;
    num_ins_built++;
    cptr->num_ins_built = num_ins_built;						/* Update number of instructions */
}

/* Routine to decide if we need to take backward branch */
uint32 takebackward_branch(uint32 client_no)
{
	uint32 random_num;
  	random_num = get_random_no_32(client_no);
  	if ((random_num % 5) == 0) {					/* randomly choose backward jumps */
		return 1;
  	} else {
		return 0;
 	}
}

/* routine to calculate the distance */
uint32 getdistance(uint32 client_no)
{
  
	uint32 distance;
  	uint32 max_distance=20;

  	/* Calculate minimum distance assuming 100% branch biasing and 1k stream depth 
    theoretically: numbranches * distance + 1024 - numbranches <= 4096 (testcase space)
    but we need to consider instructions from pre and post requisites and prolog and epilog ,so 
    consider 2k space to be available out of 4k for testcase(tc_ins)
    numbranches = 1024
    numbranches * mindistance + 1024 - numbranches = 2048
    numbranches * mindistance = 2048
    mindistance = 2
  	*/


  	/* Let maximum distance be 20.This is needed to make sure that we don't overrun the testcase space/disturb biasing with heavy branching biasing */
  	/* Let minimum distance be 10.This is needed to make sure that we have space to take care of pre and post requisites esp for bcctr and bclr*/ 
  	do { 
		distance = (get_random_no_32(client_no) % (max_distance + 1));
  	}while(distance < 10);
  	return distance;
}

/* common build routine */
uint32 cpu_build_module(int cno,int FC_CODE,int offset,int gpr_no,int index)
{
	uint32 op1, op2, op3=0, tgt, value_return;
	uint32 mcode, store_mcode, store_off, addi_mcode, load_off;
	uint32 num_ins_built, prolog_size, *tc_memory;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[cno];        
	struct instruction_masks *temp;
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	temp = &(cptr->enabled_ins_table[index].instr_table);

	switch(FC_CODE) {
		case FC_ADDI:
			/* This function will return the offset of memory and populate it with data */
		 	load_off = init_mem_for_gpr(cno, offset);
		 	load_off &= 0x0000ffff;
            addi_mcode = GEN_ADDI_MCODE(gpr_no, 0, load_off);
            *tc_memory = addi_mcode;
	        cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
        	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
            tc_memory++;
			num_ins_built++;
			value_return = load_off;		/* return the offset*/
			break;
	    case FC_STORE:
		 	store_off = init_mem_for_gpr(cno, offset);
			store_off &= 0x0000ffff;
			store_mcode = STORE_BFP_DP(gpr_no, store_off);
			*tc_memory = store_mcode;
			cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
			cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
	 		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
			tc_memory++;
			num_ins_built++;
		    value_return = store_off;	        /* return the value */	
			break;
		case FC_ADDI_IMM:
            addi_mcode = GEN_ADDI_MCODE(gpr_no, 0, offset);
            *tc_memory = addi_mcode;
	        cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
        	cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
            tc_memory++;
			num_ins_built++;
			value_return = 0;                     /* return zero*/
			break;
		case FC_INST_LOAD_EH:
			if ( temp->op3_dtype == IMM_DATA_1BIT && shifted_pvr_os >= SHIFTED_PVR_OS_P8 ){				/* generate value for EH bit for P8 and above */
				op3 = get_random_no_32(cno);
				op3 = (op3 % 2) << temp->op3_pos;
			} else {
				op3 = 0;
			}
			/*continue to fall to LOAD case*/ 
		case FC_INST_LOAD:
            tgt = (gpr_no & 0x1f) << (temp->tgt_pos);
			op1 = (LOAD_RA & 0x1f) << (temp->op1_pos);
            op2 = (LOAD_RB & 0x1f) << (temp->op2_pos);
            mcode = (temp->op_eop_mask | op1 | op2 | op3 | tgt);
        	*tc_memory = mcode;
	        cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
			cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
            cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = temp->sim_func;
            tc_memory++;
	       	num_ins_built++;
			value_return=0;
			break;
		case FC_INST_STORE:
            tgt = (gpr_no & 0x1f) << (temp->tgt_pos);
			if ( temp->op2_dtype == GRU ) {
				op2 = (STORE_RA & 0x1f) << (temp->op1_pos);
              	op1 = (STORE_RB & 0x1f) << (temp->op2_pos);
			} else {
				op2 = (STORE_RA & 0x1f) << (temp->op2_pos);
               	op1 = (STORE_RB & 0x1f) << (temp->op1_pos);
			}
			mcode = (temp->op_eop_mask | op1 | op2 | tgt);
        	*tc_memory = mcode;
	        cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
			cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
            tc_memory++;
	        num_ins_built++;
			value_return=0;
			break;
		default:
			value_return=-1;
			sprintf(msg, "%s: Unknown instruction category code !!!\n", __FUNCTION__);
	   		hxfmsg(&hd, -1 , HTX_HE_HARD_ERROR, msg);
			break;
	} /* end of switch */
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
	return (value_return);
} /* end of routine */

/*
 * P9 Word and Double word load atomic operations
 * lat RT, RA, FC
 */
void class_cpu_load_atomic_gen(uint32 client_no, uint32 random_no, struct instruction_masks *instr_ptr, int index)
{
	int gpr_t = 0, gpr_num = 0, extra_gpr_count = 0;
	uint32 tgt, op1, op2;
    uint32 offset = 0, mcode = 0;
    uint32 num_ins_built, prolog_size, *tc_memory;
	struct vsr_list *vsrs;
	uint16 backup_bdy;

    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];

	/*DPRINT(log,"Entry: %s\n", __FUNCTION__);*/

    vsrs = &cptr->vsrs[GR];
	prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	/* 1. Atomic operation function code as defined in RFC02485.AMO.r5 */
	short function_code[NUM_LD_FC] = {LD_ADD, LD_XOR, LD_OR, LD_AND, LD_MAX_UNSIGNED, LD_MAX_SIGNED, LD_MIN_UNSIGNED,
									  LD_MIN_SIGNED, LD_SWAP, COMPARE_N_SWAP_NEQ, INCREAMENT_BOUNDED, INCREAMENT_EQUAL, DECREAMENT_BOUNDED};
	/* select a function code randomely */
	int index_fc = random_no % NUM_LD_FC;


	if (function_code[index_fc] == COMPARE_N_SWAP_NEQ) {
		extra_gpr_count = 2;
	}
	else if (function_code[index_fc] < COMPARE_N_SWAP_NEQ) {
		extra_gpr_count = 1;
	}
	else {
		extra_gpr_count = 0;
	}
    /* 2. Would be using RT, RT + 1 and RT + 2 based on function code */
	gpr_t = get_random_gpr(client_no, GR, 1);
	if (extra_gpr_count == 2) {
		while ((gpr_t + 2) > 31) {
			gpr_t = get_random_gpr(client_no, GR, 1);
		}
	}
	else if (extra_gpr_count == 1) {
		while ((gpr_t + 1) > 31) {
            gpr_t = get_random_gpr(client_no, GR, 1);
        }
	}	
    for (gpr_num = gpr_t; gpr_num <= (gpr_t + extra_gpr_count); gpr_num++) { 
    	if ((0x1ULL << gpr_num) & vsrs->dirty_mask) {        /* store target register if dirty */
        	offset = init_mem_for_gpr(client_no, 8);
            offset &= 0x0000ffff;
	     	mcode = STD(gpr_num, offset, LOAD_RA);
	     	*tc_memory = mcode;
	     	cptr->instr_index[prolog_size + num_ins_built] = std | 0x20000000;
	     	cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
	     	tc_memory++;
	     	num_ins_built++;
         }
         vsrs->dirty_mask |= ((0x1ULL << gpr_num));        /* mark it as dirty  */
    }
        

    
	/* 3. get a register, to be loaded with EA */

	/* using LOAD_RA in place of gpr_a, to allocate 32 byte of memory */
	backup_bdy = cptr->bdy_req;
	cptr->bdy_req = ALIGN_2QW; 
	offset = init_mem_for_gpr(client_no, 32);
	if (index_fc == (NUM_LD_FC - 1)) {
		offset += 8;
	}
	cptr->bdy_req = backup_bdy;
    offset &= 0x0000ffff;
	mcode = GEN_ADDI_MCODE(LOAD_RB, LOAD_RA, offset);
    *tc_memory = mcode;
    cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
    tc_memory++;
    num_ins_built++;

    

    /* 5. Queue atomic instruction */
    tgt = (gpr_t) << (instr_ptr->tgt_pos);
    op1 = (LOAD_RB) << (instr_ptr->op1_pos);
    op2 = (function_code[index_fc] & 0x1f) << (instr_ptr->op2_pos);
    mcode = (instr_ptr->op_eop_mask | op1 | op2 | tgt);
    *tc_memory = mcode;
    cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    tc_memory++;
    num_ins_built++;
	    
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
	/*DPRINT(log, "Exit: %s, prolog_size: 0x%X, num_ins_built: %d\n", __FUNCTION__, prolog_size, num_ins_built);*/
}

/*
 * P9 Word and Double word store atomic operations
 * stat RS, RA, FC
 */
void class_cpu_store_atomic_gen(uint32 client_no, uint32 random_no, struct instruction_masks *instr_ptr, int index)
{
	uint32 op1 = 0, op2 = 0, tgt = 0;
 	uint32 gpr_s;
    uint32 offset = 0, mcode = 0;
    uint32 num_ins_built, prolog_size, *tc_memory;
	uint16 backup_bdy;	
	struct vsr_list *vsrs;

    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];

	/*DPRINT(log, "Entry: %s\n", __FUNCTION__);*/
    vsrs = &cptr->vsrs[GR];
	prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);


	/* get register RS to be source of random data as input to atomic operation*/
    gpr_s = get_random_gpr(client_no, GR, 1);

	/* Atomic operation function code as defined in RFC02485.AMO.r5 */
	short function_code[NUM_ST_FC] = {ST_ADD, ST_XOR, ST_OR, ST_AND, ST_MAX_UNSIGNED, ST_MAX_SIGNED, ST_MIN_UNSIGNED, ST_MIN_SIGNED, ST_TWIN};
	/* select a function code randomely */
	int index_fc = random_no % NUM_ST_FC;


	backup_bdy = cptr->bdy_req;
	cptr->bdy_req = ALIGN_2QW; 
	offset = init_mem_for_gpr(client_no, 32);
	/* restore bdy */
	cptr->bdy_req = backup_bdy;
    offset &= 0xffff;
	mcode = GEN_ADDI_MCODE(STORE_RB, STORE_RA, offset);
    *tc_memory = mcode;
    cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;

    tc_memory++;
    num_ins_built++;

    /* Queue atomic store instruction */
    op1 = (STORE_RB & 0x1f) << (instr_ptr->op1_pos);
    op2 = (function_code[index_fc] & 0x1f) << (instr_ptr->op2_pos);
    tgt = (gpr_s & 0x1f) << (instr_ptr->tgt_pos);
    mcode = (instr_ptr->op_eop_mask | op1 | op2 | tgt);
    *tc_memory = mcode;
    cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = offset;
    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    tc_memory++;
    num_ins_built++;

	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
	/*DPRINT(log, "Exit: %s, prolog_size: 0x%X, num_ins_built: %d\n", __FUNCTION__, prolog_size, num_ins_built);*/
}

void class_cpu_load_relative_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
    uint32 op1=0, op2=0, op3 = 0, tgt=0, mcode=0, *tc_memory, num_ins_built, prolog_size;

    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];
    prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
    tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

    op3 = (random_no >> ins->op3_pos) & 0x1;   /* d2 */ 
	op2 = (random_no >> ins->op2_pos) & 0x3FF; /* d0 */
    op1 = (random_no >> ins->op1_pos) & 0x1F;  /* d1 */
	

    tgt = (STORE_RB & 0x1f) << (ins->tgt_pos);
    op3 = op3 << (ins->op3_pos);
    op2 = op2 << (ins->op2_pos);
    op1 = op1 << (ins->op1_pos);
    mcode = (ins->op_eop_mask | op1 | op2 | op3 | tgt);
    *tc_memory = mcode;
    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    tc_memory++;
    num_ins_built++;
    cptr->num_ins_built = num_ins_built;
} 

void class_cpu_string_operations_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1=0, op2=0,  op3=0, tgt=0, mcode=0, *tc_memory, num_ins_built, prolog_size;                
	uint32 RT, RA, RB, BFA, imm_data, addi_mcode, store_off;

	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	uint32 ext_op = ins->op_eop_mask & 0x000007ff; /* 11 bits */
	BFA = random_no & 0x7; /* 3 bits */

	switch (ext_op) {
		case 256:       /* Setb */
			op1 = (BFA << ins->op1_pos);
			RT =  get_random_gpr(client_no,ins->tgt_dtype,1);       /* get the target register */
			tgt = (RT & 0x1f) << (ins->tgt_pos);
			mcode = (ins->op_eop_mask | op1 | tgt);
    		*tc_memory = mcode;
    		cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    		tc_memory++;
    		num_ins_built++;
			break;
		case 257:       /* Setbool */
			BFA = random_no & 0x1F; /* 5 bits */
			op1 = (BFA << ins->op1_pos);
			RT =  get_random_gpr(client_no,ins->tgt_dtype,1);       /* get the target register */
			tgt = (RT & 0x1f) << (ins->tgt_pos);
			mcode = (ins->op_eop_mask | op1 | tgt);
    		*tc_memory = mcode;
    		cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    		tc_memory++;
    		num_ins_built++;
			break;
		case 384:		/* cmprb */
		case 448:		/* cmpeqb */
			/* 1. get CR register and save it into R6 */
			*tc_memory = MFCR(GPR_TO_SAVE_CR);
    		cptr->instr_index[prolog_size + num_ins_built] = mfcr |0x20000000;
    		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_mfcr;
    		tc_memory++;
    		num_ins_built++;

    		/* Following 3 instructions are to mask the CR0, and CR1 field of this register */
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
    		store_off = init_mem_for_gpr(client_no, 8); /* 32 bit CR register */
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
			RA =  get_random_gpr(client_no,ins->tgt_dtype, 0);       /* get the source register */
			RB =  get_random_gpr(client_no,ins->tgt_dtype, 0);       /* get the source register */
			if (ext_op == 384) {
				imm_data = random_no & 0x1; /* L */
    			op1 = (imm_data << ins->op1_pos); 
				op2 = (RA << ins->op2_pos);
    			op3 = (RB << ins->op3_pos);
			}
			else {
				imm_data = 0;	
				op3 = 0;
				op1 = (RA << ins->op1_pos);
    			op2 = (RB << ins->op2_pos);
			}
			tgt = (BFA << ins->tgt_pos);
    		mcode = (ins->op_eop_mask | op1 | op2 | op3 | tgt);
    		*tc_memory = mcode;
    		cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    		tc_memory++;
    		num_ins_built++;
			break;
		default:
			/* Error Msg */
			break;
	}

    cptr->num_ins_built = num_ins_built;
}

void class_cpu_mul_add_gen(uint32 client_no, uint32 random_no, struct instruction_masks *instr, int index)
{
    uint32 mcode;
	uint32 op1, op2, op3, tgt;
    uint32 gpr_t, gpr_a, gpr_b, gpr_c;
    uint32 num_ins_built, prolog_size, *tc_memory;

    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[client_no];

    prolog_size = cptr->prolog_size;
    num_ins_built = cptr->num_ins_built;
    tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	gpr_t = get_random_gpr(client_no, GR, 1);
	gpr_a = get_random_gpr(client_no, GR, 0);
	gpr_b = get_random_gpr(client_no, GR, 0);
	gpr_c = get_random_gpr(client_no, GR, 0);

	tgt = (gpr_t << instr->tgt_pos);
    op1 = (gpr_a << instr->op3_pos);
	op2 = (gpr_b << instr->op2_pos);
    op3 = (gpr_c << instr->op1_pos); 

    mcode = (instr->op_eop_mask | op1 | op2 | op3 | tgt);
    *tc_memory = mcode;
    cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
    tc_memory++;
    num_ins_built++;

    cptr->num_ins_built = num_ins_built;
}

void class_cpu_array_index_support_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index)
{
	uint32 op1 = 0, op2 = 0, op3 = 0, tgt = 0, mcode = 0, *tc_memory, num_ins_built, prolog_size;
	uint32 gpr_s, gpr_a, SH1, SH2;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];        
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	/* pick registeres */
	gpr_s = get_random_gpr(client_no,ins->op1_dtype, 0);          /* source register */
	gpr_a = get_random_gpr(client_no,ins->op1_dtype, 1);          /* target register */

	op1 = ((gpr_s & 0x1f) << (ins->op1_pos));                     
	tgt = ((gpr_a & 0x1f) << (ins->tgt_pos));                     
	SH1 = random_no & 0x1F;
	op2 = ((SH1 & 0x1f) << (ins->op2_pos));                     
	SH2 = random_no & 0x1; 
	op3 = ((SH2 & 0x1) << (ins->op3_pos));                     

	mcode = (ins->op_eop_mask | op1 | tgt | op2 | op3);           
    *tc_memory = mcode;
	cptr->instr_index[prolog_size + num_ins_built] = index | 0x10000000;
	tc_memory++;
	num_ins_built++;
	cptr->num_ins_built = num_ins_built;			/* Update number of instructions */
}
