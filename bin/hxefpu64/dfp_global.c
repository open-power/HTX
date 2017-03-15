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
extern struct htx_data hd;
extern char msg[1000];


/* Decimal Floating Point instructions */

struct instruction_masks dfp_instructions_array[] = {

/*  DFP Load/Store Instructions */
/* lfd		*/ {0xC8000000 , 0xffffffff , GR , 16 , IMM_DATA , 0 , DUMMY , DUMMY , DFP_LONG , 21 , 0x26 , "lfd", (sim_fptr)&simulate_lfd , DFP_LOAD_LONG},
/* lfdx		*/ {0x7C0004AE , 0xffffffff , GR , 16 , GR , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x27 , "lfdx", (sim_fptr)&simulate_lfdx , DFP_LOAD_LONG, X_FORM_RT_RA_RB_eop_rc},

/* stfd		*/ {0xD8000000 , 0xffffffff , GR , 16 , IMM_DATA , 0 , DUMMY , DUMMY , DFP_LONG , 21 , 0x28 , "stfd", (sim_fptr)&simulate_stfd , DFP_STORE_LONG, D_FORM_RT_RA_D},
/* stfdx	*/ {0x7C0005AE , 0xffffffff , GR , 16 , GR , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x29 , "stfdx", (sim_fptr)&simulate_stfdx , DFP_STORE_LONG, X_FORM_RT_RA_RB_eop_rc},
/*  DFP Arithmetic Instructions */

/* dadd 	*/ {0xEC000004 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dadd", (sim_fptr)&simulate_dadd ,DFP_AIRTH_LONG, X_FORM_RT_RA_RB_eop_rc},
/* dadd.	*/ {0xEC000005 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dadd.", (sim_fptr)&simulate_dadd_dot ,DFP_AIRTH_LONG, X_FORM_RT_RA_RB_eop_rc},
/* dsub		*/ {0xEC000404 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dsub", (sim_fptr)&simulate_dsub ,DFP_AIRTH_LONG, X_FORM_RT_RA_RB_eop_rc},
/* dsub.	*/ {0xEC000405 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dsub.", (sim_fptr)&simulate_dsub_dot ,DFP_AIRTH_LONG, X_FORM_RT_RA_RB_eop_rc},
/* dmul		*/ {0xEC000044 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dmul", (sim_fptr)&simulate_dmul ,DFP_AIRTH_LONG, X_FORM_RT_RA_RB_eop_rc},
/* dmul.	*/ {0xEC000045 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dmul.", (sim_fptr)&simulate_dmul_dot ,DFP_AIRTH_LONG, X_FORM_RT_RA_RB_eop_rc},
/* ddiv		*/ {0xEC000444 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "ddiv", (sim_fptr)&simulate_ddiv ,DFP_AIRTH_LONG, X_FORM_RT_RA_RB_eop_rc},
/* ddiv.	*/ {0xEC000445 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "ddiv.", (sim_fptr)&simulate_ddiv_dot ,DFP_AIRTH_LONG, X_FORM_RT_RA_RB_eop_rc},

/* daddq 	*/ {0xFC000004 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "daddq", (sim_fptr)&simulate_daddq ,DFP_AIRTH_QUAD, X_FORM_RT_RA_RB_eop_rc},
/* daddq.	*/ {0xFC000005 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "daddq.", (sim_fptr)&simulate_daddq_dot ,DFP_AIRTH_QUAD, X_FORM_RT_RA_RB_eop_rc},
/* dsubq	*/ {0xFC000404 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "dsubq", (sim_fptr)&simulate_dsubq ,DFP_AIRTH_QUAD, X_FORM_RT_RA_RB_eop_rc},
/* dsubq.	*/ {0xFC000405 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "dsubq.", (sim_fptr)&simulate_dsubq_dot ,DFP_AIRTH_QUAD, X_FORM_RT_RA_RB_eop_rc},
/* dmulq	*/ {0xFC000044 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "dmulq", (sim_fptr)&simulate_dmulq ,DFP_AIRTH_QUAD, X_FORM_RT_RA_RB_eop_rc},
/* dmulq.	*/ {0xFC000045 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "dmulq.", (sim_fptr)&simulate_dmulq_dot ,DFP_AIRTH_QUAD, X_FORM_RT_RA_RB_eop_rc},
/* ddivq	*/ {0xFC000444 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "ddivq", (sim_fptr)&simulate_ddivq ,DFP_AIRTH_QUAD, X_FORM_RT_RA_RB_eop_rc},
/* ddivq.	*/ {0xFC000445 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "ddivq.", (sim_fptr)&simulate_ddivq_dot ,DFP_AIRTH_QUAD, X_FORM_RT_RA_RB_eop_rc},

/* DFP Compare Instructions */
/* dcmpu	*/
		  {0xEC800504 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , CR_T , 23 , 0x21 , "dcmpu", (sim_fptr)&simulate_dcmpu ,DFP_CMP_LONG, X_FORM_BF_RA_RB_eop_rc},
/* dcmpuq	*/
		  {0xFC000504 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , CR_T , 23 , 0x21 , "dcmpuq", (sim_fptr)&simulate_dcmpuq ,DFP_CMP_QUAD, X_FORM_BF_RA_RB_eop_rc},
/* dcmpo 	*/
		  {0xEC000104 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , CR_T , 23 , 0x21 , "dcmpo", (sim_fptr)&simulate_dcmpo ,DFP_CMP_LONG, X_FORM_BF_RA_RB_eop_rc},
/* dcmpoq	*/
		  {0xFC000104 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , CR_T , 23 , 0x21 , "dcmpoq", (sim_fptr)&simulate_dcmpoq ,DFP_CMP_QUAD, X_FORM_BF_RA_RB_eop_rc},

/* DFP Test Instructions */
/* While building the instruction , make sure DCM bits are selceted then */

/* dtstdc	*/{0xEC000184 , 0xffffffff , DFP_LONG , 16 , DUMMY , DUMMY , DUMMY , DUMMY , CR_T , 23 , 0x22 , "dtstdc", (sim_fptr)&simulate_dtstdc , DFP_TEST_LONG, Z_FORM_BF_RA_DM_eop_rc},
/* dtstdcq	*/{0xFC000184 , 0xffffffff , DFP_QUAD , 16 , DUMMY , DUMMY , DUMMY , DUMMY , CR_T , 23 , 0x22 , "dtstdcq", (sim_fptr)&simulate_dtstdcq ,DFP_TEST_QUAD, Z_FORM_BF_RA_DM_eop_rc},

/* dtstdg	*/{0xEC0001C4 , 0xffffffff , DFP_LONG , 16 , DUMMY , DUMMY , DUMMY , DUMMY , CR_T , 23 , 0x22 , "dtstdg", (sim_fptr)&simulate_dtstdg , DFP_TEST_LONG, Z_FORM_BF_RA_DM_eop_rc},
/* dtstdgq	*/{0xFC0001C4 , 0xffffffff , DFP_QUAD , 16 , DUMMY , DUMMY , DUMMY , DUMMY , CR_T , 23 , 0x22 , "dtstdgq", (sim_fptr)&simulate_dtstdgq ,DFP_TEST_QUAD, Z_FORM_BF_RA_DM_eop_rc},

/* dtstex	*/{0xEC000144 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , CR_T , 23 , 0x21 , "dtstex", (sim_fptr)&simulate_dtstex , DFP_TEST_LONG, X_FORM_BF_RA_RB_eop_rc},
/* dtstexq	*/{0xFC000144 , 0xffffffff , DFP_QUAD , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , CR_T , 23 , 0x21 , "dtstexq", (sim_fptr)&simulate_dtstexq ,DFP_TEST_QUAD, X_FORM_BF_RA_RB_eop_rc},

/* dtstsf	*/{0xEC000544 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , CR_T , 23 , 0x21 , "dtstsf", (sim_fptr)&simulate_dtstsf ,DFP_TEST_LONG, X_FORM_BF_RA_RB_eop_rc},
/* dtstsfq  */{0xFC000544 , 0xffffffff , DFP_LONG , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , CR_T , 23 , 0x21 , "dtstsfq", (sim_fptr)&simulate_dtstsfq ,DFP_TEST_QUAD, X_FORM_BF_RA_RB_eop_rc},


/* DFP Quantum Adjustment Instructions */
#if 1
/* dquai 	*/{0xEC000086 , 0xffffffff , DFP_LONG ,16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG ,21 , 0x23 , "dquai ", (sim_fptr)&simulate_dquai, (DFP_QUAN_LONG | HW_ONLY), Z_FORM_RT_D_RB_RMC_eop_rc},
/* dquai.	*/{0xEC000087 , 0xffffffff , DFP_LONG ,16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG ,21 , 0x23 , "dquai. ", (sim_fptr)&simulate_dquai_dot, (DFP_QUAN_LONG | HW_ONLY), Z_FORM_RT_D_RB_RMC_eop_rc},
#endif
/* dquaiq	*/{0xFC000086 , 0xffffffff , DFP_QUAD ,16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD ,21 , 0x23 , "dquaiq ", (sim_fptr)&simulate_dquaiq  ,DFP_QUAN_QUAD, Z_FORM_RT_D_RB_RMC_eop_rc},
/* dquaiq.	*/{0xFC000087 , 0xffffffff , DFP_QUAD ,16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD ,21 , 0x23 , "dquaiq.", (sim_fptr)&simulate_dquaiq_dot ,DFP_QUAN_QUAD, Z_FORM_RT_D_RB_RMC_eop_rc},
/* dqua		*/{0xEC000006 , 0xffffffff , DFP_LONG ,16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG ,21 , 0x24 , "dqua", (sim_fptr)&simulate_dqua ,DFP_QUAN_LONG, Z_FORM_RT_D_RB_RMC_eop_rc},
/* dqua. 	*/{0xEC000007 , 0xffffffff , DFP_LONG ,16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG ,21 , 0x24 , "dqua.", (sim_fptr)&simulate_dqua_dot ,DFP_QUAN_LONG, Z_FORM_RT_D_RB_RMC_eop_rc},
/* dquaq	*/{0xFC000006 , 0xffffffff , DFP_QUAD ,16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD ,21 , 0x24 , "dquaq ", (sim_fptr)&simulate_dquaq  ,DFP_QUAN_QUAD, Z_FORM_RT_D_RB_RMC_eop_rc},
/* dquaq.	*/{0xFC000007 , 0xffffffff , DFP_QUAD ,16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD ,21 , 0x24 , "dquaq.", (sim_fptr)&simulate_dquaq_dot ,DFP_QUAN_QUAD, Z_FORM_RT_D_RB_RMC_eop_rc},


/* DFP Round Instructions */
/* drrnd	*/{0xEC000046 , 0xffffffff , DFP_LONG ,16 , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_LONG ,21 , 0x24 , "drrnd", (sim_fptr)&simulate_drrnd ,DFP_RERND_LONG, Z_FORM_RT_D_RB_RMC_eop_rc},
/* drrnd . 	*/{0xEC000047 , 0xffffffff , DFP_LONG ,16 , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_LONG ,21 , 0x24 , "drrnd.", (sim_fptr)&simulate_drrnd_dot ,DFP_RERND_LONG, Z_FORM_RT_D_RB_RMC_eop_rc},
/* drrndq	*/{0xFC000046 , 0xffffffff , DFP_LONG ,16 , DFP_QUAD , 11 ,DUMMY , DUMMY , DFP_QUAD ,21 , 0x24 , "drrndq", (sim_fptr)&simulate_drrndq ,DFP_RERND_QUAD, Z_FORM_RT_D_RB_RMC_eop_rc},
/* drrndq.	*/{0xFC000047 , 0xffffffff , DFP_LONG ,16 , DFP_QUAD , 11 ,DUMMY , DUMMY , DFP_QUAD ,21 , 0x24 , "drrndq.", (sim_fptr)&simulate_drrndq_dot ,DFP_RERND_QUAD, Z_FORM_RT_D_RB_RMC_eop_rc},

/* drintx R = 0 */
		{0xEC0000C6 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x24 , "drintx", (sim_fptr)&simulate_drintx ,DFP_RND_LONG_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},
/* 		R = 1 */
		{0xEC0100C6 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x24 , "drintx", (sim_fptr)&simulate_drintx ,DFP_RND_LONG_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},

/* drintx.R = 0 */
		{0xEC0000C7 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x24 , "drintx.", (sim_fptr)&simulate_drintx_dot ,DFP_RND_LONG_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},
/* 		  R = 1 */
		{0xEC0100C7 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x24 , "drintx.", (sim_fptr)&simulate_drintx_dot ,DFP_RND_LONG_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},

/* drintxq R = 0 */
		{0xFC0000C6 , 0xffffffff ,DUMMY , DUMMY , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x24 , "drintxq", (sim_fptr)&simulate_drintxq ,DFP_RND_QUAD_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},
/*		   R = 1 */
		{0xFC0100C6 , 0xffffffff ,DUMMY , DUMMY , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x24 , "drintxq.", (sim_fptr)&simulate_drintxq_dot ,DFP_RND_QUAD_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},

/* drintxq.R = 0 */
		{0xFC0000C7 , 0xffffffff ,DUMMY , DUMMY , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x24 , "drintxq.", (sim_fptr)&simulate_drintxq_dot ,DFP_RND_QUAD_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},
/*		   R = 1 */
		{0xFC0100C7 , 0xffffffff ,DUMMY , DUMMY , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x24 , "drintxq.", (sim_fptr)&simulate_drintxq_dot ,DFP_RND_QUAD_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},

/* drintn R = 0 */
		{0xEC0001C6 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x24 , "drintn", (sim_fptr)&simulate_drintn ,DFP_RND_LONG_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},
/*		  R = 1 */
		{0xEC0101C6 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x24 , "drintn", (sim_fptr)&simulate_drintn ,DFP_RND_LONG_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},

/* drintn. R = 0 */
		{0xEC0001C7 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x24 , "drintn.", (sim_fptr)&simulate_drintn_dot ,DFP_RND_LONG_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},
/*		   R = 1 */
		{0xEC0101C7 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x24 , "drintn.", (sim_fptr)&simulate_drintn_dot ,DFP_RND_LONG_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},

/* drintnq R = 0 */
		{0xFC0001C6 , 0xffffffff , DUMMY , DUMMY ,DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x24 , "drintnq", (sim_fptr)&simulate_drintnq ,DFP_RND_QUAD_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},
/* 		   R = 1 */
		{0xFC0101C6 , 0xffffffff , DUMMY , DUMMY ,DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x24 , "drintnq", (sim_fptr)&simulate_drintnq ,DFP_RND_QUAD_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},

/* drintnq.R = 0 */
		{0xFC0001C7 , 0xffffffff , DUMMY , DUMMY ,DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x24 , "drintnq.", (sim_fptr)&simulate_drintnq_dot ,DFP_RND_QUAD_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},
/* 		   R = 1 */
		{0xFC0101C7 , 0xffffffff , DUMMY , DUMMY ,DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x24 , "drintnq.", (sim_fptr)&simulate_drintnq_dot ,DFP_RND_QUAD_2FP, Z_FORM_RT_R_RB_RMC_eop_rc},


/* DFP Convert  Instructions */

#if 1
/* dctdp Rc bit varied */
		   {0xEC000204 , 0xffffffff , DUMMY , DUMMY , DFP_SHORT , 11 ,DUMMY , DUMMY , DFP_LONG ,21 ,0x20 , "dctdp", (sim_fptr)&simulate_dctdp ,(DFP_CONV_S2LONG | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},
		   {0xEC000205 , 0xffffffff , DUMMY , DUMMY , DFP_SHORT , 11 ,DUMMY , DUMMY , DFP_LONG ,21 ,0x20 , "dctdp.", (sim_fptr)&simulate_dctdp_dot ,(DFP_CONV_S2LONG | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},
#endif

/* dctqpq */
		   {0xFC000204 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_QUAD ,21 ,0x20 , "dctqpq", (sim_fptr)&simulate_dctqpq ,DFP_CONV_L2QUAD, X_FORM_RT_RA_RB_eop_rc},
		   {0xFC000205 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_QUAD ,21 ,0x20 , "dctqpq.", (sim_fptr)&simulate_dctqpq_dot ,DFP_CONV_L2QUAD, X_FORM_RT_RA_RB_eop_rc},


/*  DFP Format Instructions */

/* drsp	  */
		   {0xEC000604 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_SHORT ,21 ,0x20 , "drsp", (sim_fptr)&simulate_drsp ,DFP_CONV_L2SRT, X_FORM_RT_RA_RB_eop_rc},
		   {0xEC000605 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_SHORT ,21 ,0x20 , "drsp.", (sim_fptr)&simulate_drsp_dot ,DFP_CONV_L2SRT, X_FORM_RT_RA_RB_eop_rc},

#if 1
/* drdpq  */
		   {0xFC000604 , 0xffffffff , DUMMY , DUMMY , DFP_QUAD , 11 ,DUMMY , DUMMY , DFP_QUAD ,21 ,0x20 , "drdpq", (sim_fptr)&simulate_drdpq ,(DFP_CONV_Q2LNG | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},
		   {0xFC000605 , 0xffffffff , DUMMY , DUMMY , DFP_QUAD , 11 ,DUMMY , DUMMY , DFP_QUAD ,21 ,0x20 , "drdpq.", (sim_fptr)&simulate_drdpq_dot ,(DFP_CONV_Q2LNG  | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},


/* dcffixq */
		   {0xFC000644 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_QUAD ,21 ,0x20 , "dcffixq", (sim_fptr)&simulate_dcffixq , (DFP_CONV_FIXED_FROM_QUAD | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},
		   {0xFC000645 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_QUAD ,21 ,0x20 , "dcffixq.", (sim_fptr)&simulate_dcffixq_dot ,(DFP_CONV_FIXED_FROM_QUAD | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},
#endif


#if 1
/* dctfix  */
		   {0xEC000244 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_LONG ,21 ,0x20 , "dctfix", (sim_fptr)&simulate_dctfix ,(DFP_CONV_2FIXED_LONG | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},
		   {0xEC000245 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 ,DUMMY , DUMMY , DFP_LONG , 21 ,0x20 , "dctfix.", (sim_fptr)&simulate_dctfix_dot ,(DFP_CONV_2FIXED_LONG | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},

/* dctfixq */
		   {0xFC000244 , 0xffffffff , DUMMY , DUMMY ,DFP_QUAD , 11 ,DUMMY , DUMMY , DFP_LONG ,21 ,0x20 , "dctfixq", (sim_fptr)&simulate_dctfixq ,(DFP_CONV_2FIXED_QUAD | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},
		   {0xFC000245 , 0xffffffff , DUMMY , DUMMY ,DFP_QUAD , 11 ,DUMMY , DUMMY , DFP_LONG ,21 ,0x20 , "dctfixq.", (sim_fptr)&simulate_dctfixq_dot ,(DFP_CONV_2FIXED_QUAD | HW_ONLY), X_FORM_RT_RA_RB_eop_rc},
#endif

/* ddedpd	*/
			{0xEC000284 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_LONG , 11 , DUMMY , DUMMY  , DFP_LONG , 21 , 0x20 , "ddedpd", (sim_fptr)&simulate_ddedpd ,DFP_DPD_2BCD_LONG, X_FORM_RT_S_SP_RB_eop_rc},
			{0xEC100284 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_LONG , 11 , DUMMY , DUMMY  , DFP_LONG , 21 , 0x20 , "ddedpd", (sim_fptr)&simulate_ddedpd ,DFP_DPD_2BCD_LONG, X_FORM_RT_S_SP_RB_eop_rc},
			{0xEC180284 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_LONG , 11 , DUMMY , DUMMY  , DFP_LONG , 21 , 0x20 , "ddedpd", (sim_fptr)&simulate_ddedpd ,DFP_DPD_2BCD_LONG, X_FORM_RT_S_SP_RB_eop_rc},
			{0xEC000285 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_LONG , 11 , DUMMY , DUMMY  , DFP_LONG , 21 , 0x20 , "ddedpd.", (sim_fptr)&simulate_ddedpd_dot ,DFP_DPD_2BCD_LONG, X_FORM_RT_S_SP_RB_eop_rc},
			{0xEC100285 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_LONG , 11 , DUMMY , DUMMY  , DFP_LONG , 21 , 0x20 , "ddedpd.", (sim_fptr)&simulate_ddedpd_dot ,DFP_DPD_2BCD_LONG, X_FORM_RT_S_SP_RB_eop_rc},
			{0xEC180285 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_LONG , 11 , DUMMY , DUMMY  , DFP_LONG , 21 , 0x20 , "ddedpd.", (sim_fptr)&simulate_ddedpd_dot ,DFP_DPD_2BCD_LONG, X_FORM_RT_S_SP_RB_eop_rc},


/* ddedpdq 	*/
			{0xFC000284 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_QUAD , 11 , DUMMY , DUMMY  , DFP_QUAD , 21 , 0x20 , "ddedpdq", (sim_fptr)&simulate_ddedpdq , DFP_DPD_2BCD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},
			{0xFC100284 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_QUAD , 11 , DUMMY , DUMMY  , DFP_QUAD , 21 , 0x20 , "ddedpdq", (sim_fptr)&simulate_ddedpdq , DFP_DPD_2BCD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},
			{0xFC180284 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_QUAD , 11 , DUMMY , DUMMY  , DFP_QUAD , 21 , 0x20 , "ddedpdq", (sim_fptr)&simulate_ddedpdq , DFP_DPD_2BCD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},
			{0xFC000285 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_QUAD , 11 , DUMMY , DUMMY  , DFP_QUAD , 21 , 0x20 , "ddedpdq.", (sim_fptr)&simulate_ddedpdq_dot ,DFP_DPD_2BCD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},
			{0xFC100285 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_QUAD , 11 , DUMMY , DUMMY  , DFP_QUAD , 21 , 0x20 , "ddedpdq.", (sim_fptr)&simulate_ddedpdq_dot ,DFP_DPD_2BCD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},
			{0xFC180285 , 0xffffffff , IMM_DATA_2BIT, 19 , DFP_QUAD , 11 , DUMMY , DUMMY  , DFP_QUAD , 21 , 0x20 , "ddedpdq.", (sim_fptr)&simulate_ddedpdq_dot ,DFP_DPD_2BCD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},

/* denbcd	*/
			{0xEC000684 , 0xffffffff , IMM_DATA_1BIT, 20 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG, 21 ,0x20 , "denbcd", (sim_fptr)&simulate_denbcd ,DFP_BCD_2DPD_LONG, X_FORM_RT_S_SP_RB_eop_rc},
			{0xEC100684 , 0xffffffff , IMM_DATA_1BIT, 20 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG, 21 ,0x20 , "denbcd", (sim_fptr)&simulate_denbcd ,DFP_BCD_2DPD_LONG, X_FORM_RT_S_SP_RB_eop_rc},
			{0xEC000685 , 0xffffffff , IMM_DATA_1BIT, 20 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG, 21 ,0x20 , "denbcd.", (sim_fptr)&simulate_denbcd_dot ,DFP_BCD_2DPD_LONG, X_FORM_RT_S_SP_RB_eop_rc},
			{0xEC100685 , 0xffffffff , IMM_DATA_1BIT, 20 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG, 21 ,0x20 , "denbcd.", (sim_fptr)&simulate_denbcd_dot ,DFP_BCD_2DPD_LONG, X_FORM_RT_S_SP_RB_eop_rc},

/* denbcdq	*/
			{0xFC000684 , 0xffffffff , IMM_DATA_1BIT, 20 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD, 21 ,0x20 , "denbcdq", (sim_fptr)&simulate_denbcdq ,DFP_BCD_2DPD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},
			{0xFC100684 , 0xffffffff , IMM_DATA_1BIT, 20 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD, 21 ,0x20 , "denbcdq", (sim_fptr)&simulate_denbcdq ,DFP_BCD_2DPD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},
			{0xFC000685 , 0xffffffff , IMM_DATA_1BIT, 20 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD, 21 ,0x20 , "denbcdq.", (sim_fptr)&simulate_denbcdq_dot ,DFP_BCD_2DPD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},
			{0xFC100685 , 0xffffffff , IMM_DATA_1BIT, 20 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD, 21 ,0x20 , "denbcdq.", (sim_fptr)&simulate_denbcdq_dot ,DFP_BCD_2DPD_QUAD, X_FORM_RT_S_SP_RB_eop_rc},

/* dxex		*/
			{0xEC0002C4 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dxex", (sim_fptr)&simulate_dxex ,DFP_INRT_EXRT_BIAS_LONG, X_FORM_RT_RA_RB_eop_rc},
			{0xEC0002C5 , 0xffffffff , DUMMY , DUMMY , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dxex.", (sim_fptr)&simulate_dxex_dot ,DFP_INRT_EXRT_BIAS_LONG, X_FORM_RT_RA_RB_eop_rc},

/* dxexq	*/
			{0xFC0002C4 , 0xffffffff , DUMMY , DUMMY , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dxexq", (sim_fptr)&simulate_dxexq ,DFP_INRT_EXRT_BIAS_QUAD, X_FORM_RT_RA_RB_eop_rc},
			{0xFC0002C5 , 0xffffffff , DUMMY , DUMMY , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "dxexq.", (sim_fptr)&simulate_dxexq_dot ,DFP_INRT_EXRT_BIAS_QUAD, X_FORM_RT_RA_RB_eop_rc},


/* diex		*/
			{0xEC0006C4 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "diex", (sim_fptr)&simulate_diex ,DFP_INRT_EXRT_BIAS_LONG, X_FORM_RT_RA_RB_eop_rc},
			{0xEC0006C5 , 0xffffffff , DFP_LONG , 16 , DFP_LONG , 11 , DUMMY , DUMMY , DFP_LONG , 21 , 0x20 , "diex.", (sim_fptr)&simulate_diex_dot ,DFP_INRT_EXRT_BIAS_LONG, X_FORM_RT_RA_RB_eop_rc},

/* diexq	*/
			{0xFC0006C4 , 0xffffffff , DFP_LONG , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "diexq", (sim_fptr)&simulate_diexq ,DFP_INRT_EXRT_BIAS_QUAD, X_FORM_RT_RA_RB_eop_rc},
			{0xFC0006C5 , 0xffffffff , DFP_LONG , 16 , DFP_QUAD , 11 , DUMMY , DUMMY , DFP_QUAD , 21 , 0x20 , "diexq.", (sim_fptr)&simulate_diexq_dot ,DFP_INRT_EXRT_BIAS_QUAD, X_FORM_RT_RA_RB_eop_rc},

/* dscli	*/
			{0xEC000084 , 0xffffffff , DFP_LONG , 16 , DUMMY, DUMMY , DUMMY , DUMMY , DFP_LONG , 21 , 0x25 , "dscli", (sim_fptr)&simulate_dscli ,DFP_SHIFT_LONG, Z_FORM_RT_RA_SH_eop_rc},
			{0xEC000085 , 0xffffffff , DFP_LONG , 16 , DUMMY, DUMMY , DUMMY , DUMMY , DFP_LONG , 21 , 0x25 , "dscli.", (sim_fptr)&simulate_dscli_dot ,DFP_SHIFT_LONG, Z_FORM_RT_RA_SH_eop_rc},

/* dscliq	*/
			{0xFC000084 , 0xffffffff , DFP_QUAD , 16 , DUMMY, DUMMY , DUMMY , DUMMY , DFP_QUAD , 21 , 0x25 , "dscliq ", (sim_fptr)&simulate_dscliq  ,DFP_SHIFT_QUAD, Z_FORM_RT_RA_SH_eop_rc},
			{0xFC000085 , 0xffffffff , DFP_QUAD , 16 , DUMMY, DUMMY , DUMMY , DUMMY , DFP_QUAD , 21 , 0x25 , "dscliq. ", (sim_fptr)&simulate_dscliq_dot  ,DFP_SHIFT_QUAD, Z_FORM_RT_RA_SH_eop_rc},

/* dscri	*/
			{0xEC0000C4 , 0xffffffff , DFP_LONG , 16 , DUMMY, DUMMY , DUMMY , DUMMY , DFP_LONG , 21 , 0x25 , "dscri", (sim_fptr)&simulate_dscri ,DFP_SHIFT_LONG, Z_FORM_RT_RA_SH_eop_rc},
			{0xEC0000C5 , 0xffffffff , DFP_LONG , 16 , DUMMY, DUMMY , DUMMY , DUMMY , DFP_LONG , 21 , 0x25 , "dscri.", (sim_fptr)&simulate_dscri_dot ,DFP_SHIFT_LONG, Z_FORM_RT_RA_SH_eop_rc},

/* dscriq	*/
			{0xFC0000C4 , 0xffffffff , DFP_QUAD , 16 , DUMMY , DUMMY , DUMMY , DUMMY , DFP_QUAD , 21 , 0x25 , "dscriq", (sim_fptr)&simulate_dscriq ,DFP_SHIFT_QUAD, Z_FORM_RT_RA_SH_eop_rc},
			{0xFC0000C5 , 0xffffffff , DFP_QUAD , 16 , DUMMY , DUMMY , DUMMY , DUMMY , DFP_QUAD , 21 , 0x25 , "dscriq.", (sim_fptr)&simulate_dscriq_dot ,DFP_SHIFT_QUAD, Z_FORM_RT_RA_SH_eop_rc},
/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};

struct instruction_masks dfp_p9_instructions_array[] = {
/*2463 - Decimal Floating-Point Support Operations*/
/* dtstsfi */{0xEC000546, 0xffffffff, IMM_DATA, 16, DFP_LONG, 11, DUMMY, DUMMY, CR_T, 23, 0x21, "dtstsfi", (sim_fptr)&simulate_dtstsfi, P9_DFP_TEST_LONG, FORM_X_BF_UIM_FB},
/* dtstsfiq */{0xFC000546, 0xffffffff, IMM_DATA, 16, DFP_QUAD, 11, DUMMY, DUMMY, CR_T, 23, 0x21, "dtstsfiq", (sim_fptr)&simulate_dtstsfiq, P9_DFP_TEST_QUAD, FORM_X_BF_UIM_FB},

/* last ins indicator */ {0xDEADBEEF, 0,DUMMY    ,  0, DUMMY    ,  0, DUMMY, DUMMY, DUMMY    ,  0, 0x0, "last_instruction"}
};

void class_dfp_load_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
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
        vsr4 = vsrs->head[DFP]->vsr_no;

        if ( (0x1ULL << vsr4) & vsrs->dirty_mask) {
                store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
                store_off &= 0x0000ffff;
                store_mcode = STORE_BFP_DP(vsr4, store_off);
                cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
                cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
                *tc_memory = store_mcode;
                cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
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

        vsrs->dirty_mask |= (0x1ULL << vsr4);
        cptr->num_ins_built = num_ins_built;
        /* DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name); */
}

void class_dfp_load_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
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
        vsr4 = vsrs->head[DFP]->vsr_no;

        if ((0x1ULL << vsr4) & vsrs->dirty_mask) {
                store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
                store_off &= 0x0000ffff;
                store_mcode = STORE_BFP_DP(vsr4, store_off);
                *tc_memory = store_mcode;
                cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x10000000;
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

	vsrs->dirty_mask |= (0x1ULL << vsr4);
	cptr->num_ins_built = num_ins_built;
        /*
        DPRINT(log, "\nInstruction Built in %s is = %lx", __FUNCTION__, mcode);
        DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name);
        */
}

void class_dfp_store_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
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
        vsr4 = vsrs->head[DFP]->vsr_no;
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

        cptr->num_ins_built = num_ins_built;
        /* DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name); */
}

void class_dfp_store_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
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
        vsr4 = vsrs->head[DFP]->vsr_no;
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

        cptr->num_ins_built = num_ins_built;
        /* DPRINT(log, "\nInstruction Built in %s is = %lx, %s", __FUNCTION__, mcode, temp->ins_name); */
}
void class_dfp_normal_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
	uint32 vsr1, vsr2, vsr4;
	uint32 op1, op2, tgt, next_tgt;
	uint32 mcode, store_mcode, store_off, prolog_size, num_ins_built, *tc_memory;
	uint64 dirty_reg_mask;
	struct vsr_list *vsrs;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];


	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	if (temp->op1_dtype != DFP_LONG && temp->op1_dtype != DFP_QUAD && temp->op1_dtype != DUMMY) {
		switch(temp->op1_dtype) {
			case IMM_DATA_1BIT:
				op1 = (random_no & 0x1) << (temp->op1_pos);
				break;
			case IMM_DATA_2BIT:
				op1 = (random_no & 0x3) << (temp->op1_pos);
				break;
			default:
				op1 = (random_no & 0x1f) << (temp->op1_pos);
				break;
		}
	}
	else {
		vsrs = &(cptr->vsrs[temp->op1_dtype]);
		vsr1 = vsrs->head[DFP]->vsr_no;
		MOVE_VSR_TO_END(client_no, temp->op1_dtype, DFP); /* Move this to the end of list */
		op1 = (vsr1 & 0x1f) << (temp->op1_pos);
#ifdef REUSE
		vsrs->dirty_mask &= (~(0x1ULL << vsr1));
#endif
	}

	vsrs = &cptr->vsrs[temp->op2_dtype];
	vsr2 = vsrs->head[DFP]->vsr_no;
	MOVE_VSR_TO_END(client_no, temp->op2_dtype, DFP); /* Move this to the end of list */
	op2 = (vsr2 & 0x1f) << (temp->op2_pos);
#ifdef REUSE
	vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif

	vsrs = &cptr->vsrs[temp->tgt_dtype];
	vsr4 = vsrs->head[DFP]->vsr_no;
	tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

	mcode = (temp->op_eop_mask | op1 | op2 | tgt);
	dirty_reg_mask = vsrs->dirty_mask;

	if ( (0x1ULL << vsr4) & dirty_reg_mask) {
		store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
		store_off &= 0x0000ffff;
		store_mcode = STORE_BFP_DP(vsr4, store_off);
		cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
		*tc_memory = store_mcode;
		cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
		num_ins_built++;
		tc_memory++;
		if (temp->tgt_dtype == DFP_QUAD) {
			next_tgt = vsr4 + 1;
			store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
			store_off &= 0x0000ffff;
			store_mcode = STORE_BFP_DP(next_tgt, store_off);

			cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
			*tc_memory = store_mcode;
			cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
			cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
			num_ins_built++;
			tc_memory++;
		}
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

void class_dfp_cmp_test_ins_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
        uint32 vsr1, vsr2, vsr4;
        uint32 op1, op2, tgt, tgt_reg_no;
        uint32 mcode, store_off, addi_mcode;
        uint32 prolog_size, num_ins_built, *tc_memory;
        struct vsr_list *vsrs;
        struct server_data *sdata = &global_sdata[INITIAL_BUF];

        struct client_data *cptr = sdata->cdata_ptr[client_no];
        prolog_size = cptr->prolog_size;
        num_ins_built = cptr->num_ins_built;
        tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

		if (temp->op1_dtype == IMM_DATA) {
 	    	op1 = (random_no & 0x3f) << temp->op1_pos;
		}else {
        	vsrs = &(cptr->vsrs[temp->op1_dtype]);
        	vsr1 = vsrs->head[DFP]->vsr_no;
        	/*
         	* Move this VSR to the end of link list
         	*/
        	MOVE_VSR_TO_END(client_no, temp->op1_dtype, DFP);
	#ifdef REUSE
        	vsrs->dirty_mask &= (~(0x1ULL << vsr1));
	#endif
        	op1 = (vsr1 & 0x1f) << (temp->op1_pos);
		}

        vsrs = &(cptr->vsrs[temp->op2_dtype]);
        vsr2 = vsrs->head[DFP]->vsr_no;
        /*
         * Move this VSR to the end of link list
         */
        MOVE_VSR_TO_END(client_no, temp->op2_dtype, DFP);
#ifdef REUSE
        vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif
		op2 = (vsr2 & 0x1f) << (temp->op2_pos);

		/* target is CR */
        vsrs = &(cptr->vsrs[temp->tgt_dtype]);
        vsr4 = vsrs->head[VSX]->vsr_no;
        tgt = (vsr4 & 0x7) << (temp->tgt_pos);
        tgt_reg_no = vsr4;

		/* machine code */
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
            vsrs->dirty_mask &= (~(0x1ULL << tgt_reg_no));
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

void class_dfp_qua_rmc_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
       uint32 vsr1, vsr2, vsr4;
       uint32 op1, op2, tgt, next_tgt, rounding_mode;
       uint32 mcode, store_mcode, store_off, prolog_size, num_ins_built, *tc_memory;
       uint64 dirty_reg_mask;
       struct vsr_list *vsrs;
       struct server_data *sdata = &global_sdata[INITIAL_BUF];
       struct client_data *cptr = sdata->cdata_ptr[client_no];


       prolog_size = cptr->prolog_size;
       num_ins_built = cptr->num_ins_built;
       tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

       vsrs = &(cptr->vsrs[temp->op1_dtype]);
       vsr1 = vsrs->head[DFP]->vsr_no;
       if(temp->ins_class == CLASS_DFP_RMC) {
	 	   MOVE_VSR_TO_END(client_no, temp->op1_dtype, DFP); /* Move this to the end of list */
#ifdef REUSE
           vsrs->dirty_mask &= (~(0x1ULL << vsr1));
#endif
       }
       op1 = (vsr1 & 0x1f) << (temp->op1_pos);

       vsrs = &cptr->vsrs[temp->op2_dtype];
       vsr2 = vsrs->head[DFP]->vsr_no;
       MOVE_VSR_TO_END(client_no, temp->op2_dtype, DFP); /* Move this to the end of list */
       op2 = (vsr2 & 0x1f) << (temp->op2_pos);
#ifdef REUSE
       vsrs->dirty_mask &= (~(0x1ULL << vsr2));
#endif
       vsrs = &cptr->vsrs[temp->tgt_dtype];
       vsr4 = vsrs->head[DFP]->vsr_no;
       tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

       rounding_mode = (random_no & 0x3) << 9;

       mcode = (temp->op_eop_mask | op1 | op2 | tgt | rounding_mode);

       dirty_reg_mask = vsrs->dirty_mask;

       if ( (0x1ULL << vsr4) & dirty_reg_mask) {
            store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
            store_off &= 0x0000ffff;
            store_mcode = STORE_BFP_DP(vsr4, store_off);

            cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
            *tc_memory = store_mcode;
            cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
            cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
            tc_memory++;
            num_ins_built++;
            if (temp->tgt_dtype == DFP_QUAD) {
                 next_tgt = vsr4 + 1;
                 store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
                 store_off &= 0x0000ffff;
                 store_mcode = STORE_BFP_DP(next_tgt, store_off);

                 cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
                 *tc_memory = store_mcode;
                 cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
                 cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
                 num_ins_built++;
                 tc_memory++;
            }
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

void class_dfp_shift_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index)
{
       uint32 vsr1, vsr4;
       uint32 op1, tgt, next_tgt;
       uint32 mcode, store_mcode, store_off, prolog_size;
       uint32 shift, num_ins_built, *tc_memory;
       uint64 dirty_reg_mask;
       struct vsr_list *vsrs;
       struct server_data *sdata = &global_sdata[INITIAL_BUF];
       struct client_data *cptr = sdata->cdata_ptr[client_no];


       prolog_size = cptr->prolog_size;
       num_ins_built = cptr->num_ins_built;
       tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

       vsrs = &(cptr->vsrs[temp->op1_dtype]);
       vsr1 = vsrs->head[DFP]->vsr_no;
       MOVE_VSR_TO_END(client_no, temp->op1_dtype, DFP); /* Move this to the end of list */
       op1 = (vsr1 & 0x1f) << (temp->op1_pos);
#ifdef REUSE
       vsrs->dirty_mask &= (~(0x1ULL << vsr1));
#endif
       shift = (random_no & 0x3f) << 10;

       vsrs = &cptr->vsrs[temp->tgt_dtype];
       vsr4 = vsrs->head[DFP]->vsr_no;
       tgt = (vsr4 & 0x1f) << (temp->tgt_pos);

       mcode = (temp->op_eop_mask | op1 | shift | tgt);

       dirty_reg_mask = vsrs->dirty_mask;

       if ( (0x1ULL << vsr4) & dirty_reg_mask) {
              store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
              store_off &= 0x0000ffff;
              store_mcode = STORE_BFP_DP(vsr4, store_off);

              cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
              *tc_memory = store_mcode;
              cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
              cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
              tc_memory++;
              num_ins_built++;
              if (temp->tgt_dtype == DFP_QUAD) {
                   next_tgt = vsr4 + 1;
                   store_off = init_mem_for_vsx_store(client_no, temp->tgt_dtype);
                   store_off &= 0x0000ffff;
                   store_mcode = STORE_BFP_DP(next_tgt, store_off);

                   cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
                   *tc_memory = store_mcode;
                   cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
                   cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
                   num_ins_built++;
                   tc_memory++;
              }
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


/****************************************************/
/*		  	    DFP DATA GENERATION			        */
/****************************************************/

int DFPGENE(unsigned long long *Operand,int class,int Type , int cno)
{
	int retc=0;
	unsigned long long DecodedOp[3], EncodedOp[2];

	DFPType[cno]  =  Type;                          /* operand type */
	DFPClass[cno] =  class;                         /* class - zero, norma, etc */

	GenDFPOP(&DecodedOp[0],cno);   /* generate a dfp operand */


	/*
	DPRINT(cptr->clog, "\n%s[%d]:Decoded Input:\n", __FUNCTION__, __LINE__);
	if (DFPType[cno] == 0)
		DPRINT(cptr->clog, "RANDOM Generated input %llx\n",DecodedOp[0]);
	else
		for (k=0; k<DFPType[cno]; k++) {
		    DPRINT(cptr->clog,"\n RANDOM Generated input %llx\n",DecodedOp[k]);
		}
	*/

	/*Encode the operand */
	ActionType[cno] = ENCODE;
	retc = Decoder_Encoder(&DecodedOp[0],&EncodedOp[0],cno);


	/* 
	DPRINT(cptr->clog, "\n%s[%d]:Encoded Input:\n", __FUNCTION__, __LINE__);
	if ( DFPType[cno] == 0)
		DPRINT(cptr->clog, "ENCODED DFP FROM DFPGENE %llx \n",EncodedOp[0]);
	else
		for(k=0; k<DFPType[cno]; k++) {
		    DPRINT(cptr->clog, "ENCODED DFP FROM DFPGENE %llx \n",EncodedOp[k]);
		}

	*/


	*Operand = EncodedOp[0];
	if (DFPType[cno] == EXTENDED) {
	    Operand += 2;
	    *Operand = EncodedOp[1];
	}

	return retc;
}/* end DFPGEN */


int Decoder_Encoder(unsigned long long *Input, unsigned long long *Output, int cno)
{
	union TheUnDecoded MyUndecoded;
	union TheDecoded MyDecoded;
	union The_CV TheCF;

	unsigned long long *ToBDecoded, *Decoded,*Operand,TempIn[3], DecoderIn[3], EncoderIn[3], DecoderOut[3], *Source,*Target;
	int i, retc=0;

	bxcfLen_MASK[cno] = 0;

	Operand = Input;

	Spans[cno] = 0;
	DoInit(cno);           /* get basic initial values */

	retc = CheckFinite(Operand);  /* check if the operand is finite */
	if ((retc != 0) || (DFPClass[cno] == ZEERO)) {               /* operand was found to be infinity or NaN */
	    if (ActionType[cno] == ENCODE) {  /* place the operand into the encoder output */
			if (DFPType[cno] == SHORT)
				*Output = ((*Input) >> 32);
			else
				*Output = *Input;  /* keep whatever the input was */
			for(i=1; i<DFPType[cno];i++) { /* for the extended case */
				Output++;
				Input++;
				*Output = *Input;
			}
		}
		return 0;           /* do not know how to encode/decode non-finites*/

	}

	/* operand is niether infinity nor NaN/ZERO */
	if (ActionType[cno] == ENCODE) {
		for(i=0; i<=DFPType[cno];i++) {
			EncoderIn[i] = *Operand;
			Operand++;
		}
		Source = &EncoderIn[0];
		Target = &TempIn[0];

		Encoder(Source,Target,cno, &MyUndecoded, &MyDecoded, &TheCF);  /* Packer */

		if(DFPType[cno] == 0)
                   *Output = TempIn[0];
                else
		  for (i=0;i<DFPType[cno]; i++) { /* collect the output of the encoder */
			*Output = TempIn[i];
			if ((i+1)<=DFPType[cno])
				Output++;
		}
	}
	else if (ActionType[cno] == DECODE) {
		Decoded = &DecoderOut[0];
		ToBDecoded = Operand;
		for(i=0; i<=DFPType[cno];i++) {
			DecoderIn[i]  = *Operand;
			DecoderOut[i] = 0;
			Operand++;
		}

		Decoder(ToBDecoded, Decoded, cno, &MyUndecoded, &MyDecoded, &TheCF);  /* Unpacker the DFP number */
	}
	else {
		return 1;
	}
	return 0;
}/* end of decoder_encoder */

/******************************************************************************
 To decode a DFP number, the following steps are taken:
 1. First get the sign.
 2. Get the combined field and decompose it into 2 fields
	a- the most two significant bits of the exponent
        b- the most significant digit
 3. Put the most significant exponent bits into the decoded number
 4. Get the exponent continuation field and put it into the decoded field
 5. Place the most significant digits into the decoded number
 6. get 10 bits at a time from the undecoded number and create the corresponding
    decoded digits (12 bits) and put them into the decoded number
    ___________________ ____________________
   |    64 bit         | 64 bit             |   extended will require more
   |___________________| ___________________|   storage. Short needs only 64b.
                                           ^
         shift to the left<---------       |
                                   Decoded numbers
******************************************************************************/

void Decoder(unsigned long long *ToBDecoded, unsigned long long *Decoded, int cno, union TheUnDecoded *P2UnDec, union TheDecoded *P2Dec, union The_CV *TheCF)
{
	unsigned int i,j, ReadCounter,WrittenCounter,ReadDWs,WrittenDWs,temp;
	short SignAndCF;/* sign expo cont field */
	short LMD, Sign;/* left most digit and sign*/
	short nBits,my_temp;
	struct CF_Parts CFParts;

	i = j = 0;           /* zero them out */

	*Decoded = 0;

	SignAndCF = *ToBDecoded >> 58;    /* get the CF field and sign bit */
	Sign = *ToBDecoded>>63; /* sign bit only */
	*Decoded = *Decoded|Sign; /* sign bit only */
	LMD = Decode_CF(&CFParts, SignAndCF&0x001f, cno, TheCF);/* decode the CF field*/
	*Decoded = *Decoded <<1;/* positions to be occupied exp ms bit*/
	*Decoded = *Decoded | CFParts.EXPMSB1;  /* gets exp 1st most signif bit */
	*Decoded = *Decoded <<1;            /* position occupied 2nd exp ms bit*/
	*Decoded = *Decoded | CFParts.EXPMSB2;  /* gets exp 2nd most signif bit */
	*Decoded = *Decoded <<(bxcfLen[cno]);/* Make room for the biased exp con field */
	temp = *ToBDecoded>>(64-(6+bxcfLen[cno]));
	*ToBDecoded = *ToBDecoded<<(bxcfLen[cno]+6);/*get rid off sign,lmd,Exp Con field */
	temp = temp & bxcfLen_MASK[cno];
	*Decoded = *Decoded | temp;  /* store the exp con field */
	*Decoded = *Decoded <<4;	/* Make room for the LMD */
	*Decoded = *Decoded | LMD;  /* left most digit */

	WrittenCounter = 1+(2+bxcfLen[cno])+4;/* we wrote sign+ exp(2bit+exmsbs) + lmd */
	ReadCounter = 1+5+bxcfLen[cno];       /* we read sign+CF+bxcfLen */
	ReadDWs = 1;
	WrittenDWs = 1;
  /************************************************************************
    Read the operand to be decoded 10 bits at time. A critical point is
    when operand spans over two double words and we have to cross between
    the double words to get the 10 bits. ReadCounter will tell tell us
    how many bits we read from a given double word. nBits=(64-ReadCount)
    will give the number of bits that we have to read from the current double
    word before crossing to the next. (10-nBits) will tell us how many
    bits we have to read from the next double word for the first time
    The ReadDWs (read double words) cannot exceed 2 since the
    max operand can be 128 bits.
  ************************************************************************/
	for (i=0; i<NoThreeDigSets[cno]; i++) {   /* to decode the packed number  */
                                   		/* Loop for 2, 5, or 11 times   */
		nBits =0;
		if (((ReadCounter+10) > 64) & (ReadDWs<2)) {
			nBits = 64-ReadCounter;
			my_temp = *ToBDecoded >> (64-nBits);/* get 10-nBits bits */
			if(EnableTrace[cno] >=2 ) { /* tracing the numbers */
				printf("nBits  = %x my_temp %x\n", nBits,my_temp);
				printf("ReadCounter = %d \n", ReadCounter);
			}
			else{}
			ToBDecoded++;          /* should get the digits in the upper 64 bit */
			ReadDWs++;             /* started ready a new double word */
			my_temp = my_temp << (10-nBits);
			my_temp = my_temp | (*ToBDecoded >> (64-(10-nBits)));
			P2UnDec->ThreeDig = my_temp;
			ReadCounter = 0;
			Spans[cno] = 1;
		}
		else {
			P2UnDec->ThreeDig = *ToBDecoded >> (64-10);/* get the leftmost 10 bits */
		}
		*ToBDecoded = *ToBDecoded << (10-nBits); /* We decoded 10 bits. didn't we? */
		UnPackThree(P2UnDec, P2Dec, cno);/* Unpack or decode three digits at a time */
		ReadCounter = ReadCounter+10-nBits;            /* just read 10 bits */
		nBits = 0;
		if((WrittenCounter+Twelve)>=64) { /*can't fit in the array element (64bit long) */
			nBits = 64-WrittenCounter;  /* there is room for some bits */
			*Decoded = *Decoded << nBits;
			*Decoded = *Decoded | (P2Dec->ThreeDig>>(12-nBits)); /* squeze it*/
			P2Dec->ThreeDig = (P2Dec->ThreeDig<<(16-(Twelve-nBits)));
			P2Dec->ThreeDig = (P2Dec->ThreeDig>>(16-(Twelve-nBits)));
			Spans[cno] = 1;  /* data spans over more than 64bit field */
			Decoded++;  /* will store into the next 64 bit block (to the left)*/
			WrittenCounter = 0; /* reset this counter */
			WrittenDWs++;       /* no of written 64 bit fields */
		}
		else {
			*Decoded = *Decoded<<(Twelve-nBits);  /*room for 3 decoded digits */
		}
		*Decoded = *Decoded | P2Dec->ThreeDig;   /*store the decoded 3 digits */
		WrittenCounter = WrittenCounter+(Twelve-nBits);
	}/* end for loop */

	if (WrittenCounter <64) { /* for the long and extended*/
		*Decoded = *Decoded<<(64-(WrittenCounter));
	}
}/* decode_long */

/******************************************************************************
 To encode a DFP number, the following steps are taken:
 1. First get the sign.
 2. Create the combined field from
	a- the two most significant bits of the exponent
        b- the most significant digit
 3. Build the biased exponent continuation field (bxcfLen)
 4. get 12 bits at a time from the undecoded number and create the corresponding
    decoded digits and put them into the decoded number
    ___________________ ____________________
   |    64 bit         | 64 bit             |   extended will require more
   |___________________| ___________________|   storage. Short needs only 64b.
                                           ^
         shift to the left<---------       |
                                   encoded numbers
******************************************************************************/
void Encoder(unsigned long long *Source, unsigned long long *Target, int cno, union TheUnDecoded *P2UnDec, union TheDecoded *P2Dec, union The_CV *TheCF)
{
	unsigned int i,nBits,ReadCounter,WrittenCounter;
	unsigned short TempLMD,Sign;
	unsigned short LMD,Expo,cf;   /* LMD and exponent */
	unsigned short my_temp;               /* Gen purpose variable */

	*Target = 0;
	Sign = *Source>>63; /* get the sign bit from the decoded #*/
	*Target = *Target| Sign; /* sign bit into the encoded #*/
	*Source = *Source&0x7fffffffffffffffULL; /* wipe out the sign bit: already got it */
	Expo = *Source>>(64-(1+2+bxcfLen[cno])); /* exp lmbs+exp con field*/
	TempLMD = *Source>>(64-(1+2+bxcfLen[cno]+4));
	TempLMD = TempLMD &0x0000000f;
	LMD = TempLMD;
	cf = Encode_CF(Expo,LMD,cno, TheCF); /* encode the CF field: returns combined field */
	*Target = *Target<<5;/* position to be occupied by CF*/
	*Target = *Target |cf;/* Store the encoded cf value*/
	*Target = *Target<<bxcfLen[cno];/* position to be occupied by bxcfLen*/
	Expo = Expo<<(16-bxcfLen[cno]); /* get rid off the exp lm bits */
	Expo = Expo>>(16-bxcfLen[cno]); /* actual bxcfLen */
	*Target = *Target|Expo;/* Store the bxcfLen value*/
	*Source = *Source<<(1+2+bxcfLen[cno]+4); /*already dealt with the sign,exp lmb,bxcfLen*/
	ReadCounter = 1+2+bxcfLen[cno]+4;    /* we read sign,exp lmbs,biased exp con,lmd*/
	WrittenCounter = 1+5+bxcfLen[cno];    /* we wrote sign,CF, and  bxcfLen */
/**************************************************************************
   go through the unpacked fraction and take 12 bits at time. The number of
   times the for loop is executed depends on the operand type: 2 for short,
   5 for long, and 11 for extended
**************************************************************************/
	for (i=0; i<NoThreeDigSets[cno]; i++) { /* to endecode the unpacked number */
		nBits = 0;
/*********************************************************************
       for the long and extended, the decoded operand spans over more
       than 64 bit field. For example, 75 bits are needed for decoded
       long. Therefore the last 12 bit set will consist of one bit
       from the one 64 bit field and the other 11 will come from the
       next 64 bit field.
*********************************************************************/
		if ((ReadCounter+Twelve) > 64) {
			nBits = 64-ReadCounter;
			/* my_temp = *Source >> (64-(Twelve-nBits)); */ /* Get the remaining bits */
			my_temp = *Source >> (64-nBits); /* Get the remaining bits */
			if(EnableTrace[cno] >= 2) { /* tracing the numbers */
				printf("my_temp (unfinished) = %x nBits = %d\n", my_temp, nBits);
			}
			else{}
			Source++;          /* should get the digits in the upper 64 bit */
			my_temp = my_temp << (Twelve-nBits);
			my_temp = my_temp | (*Source >> (64-(Twelve-nBits)));
			ReadCounter = 0;
			P2Dec->ThreeDig = my_temp;
			Spans[cno] = 1;
			*Source = *Source << (Twelve-nBits);  /* We encoded 12-nBits. didn't we? */
		}
		else {
			P2Dec->ThreeDig = *Source >> (64-Twelve);/* get the leftmost 12 bits */
			*Source = *Source << Twelve;  /* We encoded 12 bits. didn't we? */
		}

		PackThree(P2UnDec, P2Dec, cno);
		ReadCounter = ReadCounter+(Twelve-nBits); /* we encoded 12 extra bits */

/**********************************************************************
      Now write the three digits into the undecoded double word. Must be
      careful since the three digits may span over more than one double
      word.
**********************************************************************/
		nBits = 0;
		if (WrittenCounter+10>64) { /* 3 digits won't fit in the current double word*/
			nBits = 64-WrittenCounter; /* how many bits should go into the current DW*/
			my_temp = P2UnDec->ThreeDig;
			*Target = *Target<<nBits; /* room for part of the 3 encoded digits */
			*Target = *Target |(my_temp>>(10-nBits));
			my_temp = my_temp << (16-(10-nBits));
			my_temp = my_temp >> (16-(10-nBits));
			Target++;          /* start writting into the next double word */
			*Target = *Target | my_temp;
			WrittenCounter = 0;  /* reset the counter - will be adjusted later */
		}
		else {
			*Target = *Target<<10; /* room for the 3 encoded digits */
			*Target = *Target |P2UnDec->ThreeDig;  /*store the encoded 3 digits */
		}
		WrittenCounter = WrittenCounter+10-nBits; /* how many bits did we write */
	}/* end for loop */

	if(DFPType[cno] != 0)  {
	  if ((WrittenCounter <64) & (Spans[cno] ==0)) {
		*Target = *Target<<(64-WrittenCounter);
	  }
       }

	if(DFPType[cno] == 2)
		Target--;
}/* end of encoder */

/* pack the three degits */
void PackThree(union TheUnDecoded *P2UnDec,union TheDecoded *P2Dec, int cno)
{
          union The_AEI TheAEI;

          TheAEI.val = 0;             /* ABCD EFGH IJKM */
          TheAEI.AEI.A = P2Dec->decoded.A;
          TheAEI.AEI.E = P2Dec->decoded.E;
          TheAEI.AEI.I = P2Dec->decoded.I;

          if (TheAEI.val == 0) {     /* go through the table */
                  P2UnDec->undecoded.P=P2Dec->decoded.B;
                  P2UnDec->undecoded.Q=P2Dec->decoded.C;
                  P2UnDec->undecoded.R=P2Dec->decoded.D;
                  P2UnDec->undecoded.S=P2Dec->decoded.F;
                  P2UnDec->undecoded.T=P2Dec->decoded.G;
                  P2UnDec->undecoded.U=P2Dec->decoded.H;
                  P2UnDec->undecoded.V=0;
                  P2UnDec->undecoded.W=P2Dec->decoded.J;
                  P2UnDec->undecoded.X=P2Dec->decoded.K;
                  P2UnDec->undecoded.Y=P2Dec->decoded.M;
          }
          else if (TheAEI.val == 1) {
                  P2UnDec->undecoded.P=P2Dec->decoded.B;
                  P2UnDec->undecoded.Q=P2Dec->decoded.C;
                  P2UnDec->undecoded.R=P2Dec->decoded.D;
                  P2UnDec->undecoded.S=P2Dec->decoded.F;
                  P2UnDec->undecoded.T=P2Dec->decoded.G;
                  P2UnDec->undecoded.U=P2Dec->decoded.H;
                  P2UnDec->undecoded.V=1;
                  P2UnDec->undecoded.W=0;
                  P2UnDec->undecoded.X=0;
                  P2UnDec->undecoded.Y=P2Dec->decoded.M;
          }
          else if (TheAEI.val == 2) {
                  P2UnDec->undecoded.P=P2Dec->decoded.B;
                  P2UnDec->undecoded.Q=P2Dec->decoded.C;
                  P2UnDec->undecoded.R=P2Dec->decoded.D;
                  P2UnDec->undecoded.S=P2Dec->decoded.J;
                  P2UnDec->undecoded.T=P2Dec->decoded.K;
                  P2UnDec->undecoded.U=P2Dec->decoded.H;
                  P2UnDec->undecoded.V=1;
                  P2UnDec->undecoded.W=0;
                  P2UnDec->undecoded.X=1;
                  P2UnDec->undecoded.Y=P2Dec->decoded.M;
          }
          else if (TheAEI.val == 3) {
                  P2UnDec->undecoded.P=P2Dec->decoded.B;
                  P2UnDec->undecoded.Q=P2Dec->decoded.C;
                  P2UnDec->undecoded.R=P2Dec->decoded.D;
                  P2UnDec->undecoded.S=1;
                  P2UnDec->undecoded.T=0;
                  P2UnDec->undecoded.U=P2Dec->decoded.H;
                  P2UnDec->undecoded.V=1;
                  P2UnDec->undecoded.W=1;
                  P2UnDec->undecoded.X=1;
                  P2UnDec->undecoded.Y=P2Dec->decoded.M;
          }
          else if (TheAEI.val == 4) {
                  P2UnDec->undecoded.P=P2Dec->decoded.J;
                  P2UnDec->undecoded.Q=P2Dec->decoded.K;
                  P2UnDec->undecoded.R=P2Dec->decoded.D;
                  P2UnDec->undecoded.S=P2Dec->decoded.F;
                  P2UnDec->undecoded.T=P2Dec->decoded.G;
                  P2UnDec->undecoded.U=P2Dec->decoded.H;
                  P2UnDec->undecoded.V=1;
                  P2UnDec->undecoded.W=1;
                  P2UnDec->undecoded.X=0;
                  P2UnDec->undecoded.Y=P2Dec->decoded.M;
          }
          else if (TheAEI.val == 5) {
                  P2UnDec->undecoded.P=P2Dec->decoded.F;
                  P2UnDec->undecoded.Q=P2Dec->decoded.G;
                  P2UnDec->undecoded.R=P2Dec->decoded.D;
                  P2UnDec->undecoded.S=0;
                  P2UnDec->undecoded.T=1;
                  P2UnDec->undecoded.U=P2Dec->decoded.H;
                  P2UnDec->undecoded.V=1;
                  P2UnDec->undecoded.W=1;
                  P2UnDec->undecoded.X=1;
                  P2UnDec->undecoded.Y=P2Dec->decoded.M;
          }
          else if (TheAEI.val == 6) {
                  P2UnDec->undecoded.P=P2Dec->decoded.J;
                  P2UnDec->undecoded.Q=P2Dec->decoded.K;
                  P2UnDec->undecoded.R=P2Dec->decoded.D;
                  P2UnDec->undecoded.S=0;
                  P2UnDec->undecoded.T=0;
                  P2UnDec->undecoded.U=P2Dec->decoded.H;
                  P2UnDec->undecoded.V=1;
                  P2UnDec->undecoded.W=1;
                  P2UnDec->undecoded.X=1;
                  P2UnDec->undecoded.Y=P2Dec->decoded.M;
          }
          else if (TheAEI.val == 7) {
                  P2UnDec->undecoded.P=0;
                  P2UnDec->undecoded.Q=0;
                  P2UnDec->undecoded.R=P2Dec->decoded.D;
                  P2UnDec->undecoded.S=1;
                  P2UnDec->undecoded.T=1;
                  P2UnDec->undecoded.U=P2Dec->decoded.H;
                  P2UnDec->undecoded.V=1;
                  P2UnDec->undecoded.W=1;
                  P2UnDec->undecoded.X=1;
                  P2UnDec->undecoded.Y=P2Dec->decoded.M;
          }
          else {
                  if (EnableTrace[cno] > 0) {
                          printf("Warning Warning: could not encode the digits\n");
                          printf("AEI val = %d\n", TheAEI.val);
                  }
                  else{}
          }
}

short Encode_CF(unsigned short Exponent, unsigned short lmd, int cno, union The_CV *TheCF) /* Generate CF  */
{
          unsigned short shifted_exp;
          union Exp Expo;

          shifted_exp= Exponent << (16-(2+bxcfLen[cno])); /*making msb at the msb positions */

          Expo.exp_val = shifted_exp; /* get the passed value - shifted to the right */
          TheCF->cf_val = 0; /* initialize it to zero; */

          switch (lmd) { /*Based on the leftmost digit, go through the table */
          case 0:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=00000*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=0;
                          TheCF->cf.C=0;
                          TheCF->cf.D=0;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=01000*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=1;
                          TheCF->cf.C=0;
                          TheCF->cf.D=0;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=10000*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=0;
                          TheCF->cf.C=0;
                          TheCF->cf.D=0;
                          TheCF->cf.E=0;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          case 1:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=00001*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=0;
                          TheCF->cf.C=0;
                          TheCF->cf.D=0;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=01001*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=1;
                          TheCF->cf.C=0;
                          TheCF->cf.D=0;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=10001*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=0;
                          TheCF->cf.C=0;
                          TheCF->cf.D=0;
                          TheCF->cf.E=1;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          case 2:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=00010*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=0;
                          TheCF->cf.C=0;
                          TheCF->cf.D=1;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=01001*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=1;
                          TheCF->cf.C=0;
                          TheCF->cf.D=1;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=10010*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=0;
                          TheCF->cf.C=0;
                          TheCF->cf.D=1;
                          TheCF->cf.E=0;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          case 3:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=00011*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=0;
                          TheCF->cf.C=0;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=01011*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=1;
                          TheCF->cf.C=0;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=10011*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=0;
                          TheCF->cf.C=0;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          case 4:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=00100*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=0;
                          TheCF->cf.C=1;
                          TheCF->cf.D=0;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=01100*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=0;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=01100*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=0;
                          TheCF->cf.C=1;
                          TheCF->cf.D=0;
                          TheCF->cf.E=0;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          case 5:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=00101*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=0;
                          TheCF->cf.C=1;
                          TheCF->cf.D=0;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=01101*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=0;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=10101*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=0;
                          TheCF->cf.C=1;
                          TheCF->cf.D=0;
                          TheCF->cf.E=1;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          case 6:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=00110*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=0;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=01110*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=10110*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=0;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=0;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          case 7:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=00111*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=0;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=01111*/
                          TheCF->cf.A=0;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=10111*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=0;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          case 8:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=11000*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=0;
                          TheCF->cf.D=0;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=11010*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=0;
                          TheCF->cf.D=1;
                          TheCF->cf.E=0;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=10111*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=0;
                          TheCF->cf.E=0;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          case 9:
                  if ((Expo.e.msb_a == 0) && (Expo.e.msb_b == 0)) { /*EXP BITS=00 CF=11001*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=0;
                          TheCF->cf.D=0;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 0)&&(Expo.e.msb_b == 1)) { /*EXP BITS=00 CF=11011*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=0;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  else if((Expo.e.msb_a == 1)&&(Expo.e.msb_b == 0)) { /*EXP BITS=10 CF=11101*/
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=0;
                          TheCF->cf.E=1;
                  }
                  else {
                          TheCF->cf.A=1;
                          TheCF->cf.B=1;
                          TheCF->cf.C=1;
                          TheCF->cf.D=1;
                          TheCF->cf.E=1;
                  }
                  break;
          default:
                  if (EnableTrace[cno] >=1) {
                          printf("wrong value of LMD = %x to Encode_CF \n",lmd);
                  }
                  else{}
          }
          return TheCF->cf_val;
} /* encodeTheCF->cf.*/

void UnPackThree(union TheUnDecoded *P2UnDec,union TheDecoded *P2Dec, int cno)
{
     IsRedundant[cno] = 0;     /* number is not in the 24 redundant cases */
      if(EnableTrace[cno] >= 2) /* tracing the numbers */
      {
     /*  printf("%u", P2UnDec->undecoded.P);
       printf("%u", P2UnDec->undecoded.Q);
       printf("%u", P2UnDec->undecoded.R);
       printf("%u", P2UnDec->undecoded.S);
       printf("%u", P2UnDec->undecoded.T);
       printf("%u", P2UnDec->undecoded.U);
       printf("%u", P2UnDec->undecoded.V);
       printf("%u", P2UnDec->undecoded.W);
       printf("%u", P2UnDec->undecoded.X);
       printf("%u = ", P2UnDec->undecoded.Y); */
       printf(" %x\n",P2UnDec->ThreeDig);
      }
      else{}

      /* first check if the three digits are in preferred/nonpreferred form.*/
      /* There are twenty four redundancy in the translation: for example,
         888 is the preferred form for 0x0e,0x16e,0x26e and 0x36e           */
      if ((P2UnDec->ThreeDig == 0x06e) ||
             (P2UnDec->ThreeDig == 0x16e) ||
             (P2UnDec->ThreeDig == 0x26e) ||
             (P2UnDec->ThreeDig == 0x36e) )
      {
           P2Dec->ThreeDig = 0x888;
           IsRedundant[cno] = 1;
       }
       else if ( (P2UnDec->ThreeDig == 0x06f) ||
            (P2UnDec->ThreeDig == 0x16f) ||
            (P2UnDec->ThreeDig == 0x26f) ||
            (P2UnDec->ThreeDig == 0x36f) )
       {
           P2Dec->ThreeDig = 0x889;
            IsRedundant[cno] = 1;
       }
       else if ( (P2UnDec->ThreeDig == 0x07e) ||
                 (P2UnDec->ThreeDig == 0x17e) ||
                 (P2UnDec->ThreeDig == 0x27e) ||
                 (P2UnDec->ThreeDig == 0x37e) )
       {
           P2Dec->ThreeDig = 0x898;
           IsRedundant[cno] = 1;
       }
       else if ( (P2UnDec->ThreeDig == 0x07f) ||
                 (P2UnDec->ThreeDig == 0x17f) ||
                 (P2UnDec->ThreeDig == 0x27f) ||
                 (P2UnDec->ThreeDig == 0x37f) )
       {
          P2Dec->ThreeDig = 0x899;
          IsRedundant[cno] = 1;
       }
       else if ( (P2UnDec->ThreeDig == 0x0ee) ||
            (P2UnDec->ThreeDig == 0x1ee) ||
            (P2UnDec->ThreeDig == 0x2ee) ||
            (P2UnDec->ThreeDig == 0x3ee) )
       {
           P2Dec->ThreeDig = 0x988;
           IsRedundant[cno] = 1;
       }
       else if ( (P2UnDec->ThreeDig == 0x0ef) ||
            (P2UnDec->ThreeDig == 0x1ef) ||
            (P2UnDec->ThreeDig == 0x2ef) ||
            (P2UnDec->ThreeDig == 0x3ef) )
       {
           P2Dec->ThreeDig = 0x989;
           IsRedundant[cno] = 1;
       }
       else if ( (P2UnDec->ThreeDig == 0x0fe) ||
            (P2UnDec->ThreeDig == 0x1fe) ||
            (P2UnDec->ThreeDig == 0x2fe) ||
            (P2UnDec->ThreeDig == 0x3fe) )
       {
            P2Dec->ThreeDig = 0x998;
            IsRedundant[cno] = 1;
       }
       else if ( (P2UnDec->ThreeDig == 0x0ff) ||
            (P2UnDec->ThreeDig == 0x1ff) ||
            (P2UnDec->ThreeDig == 0x2ff) ||
            (P2UnDec->ThreeDig == 0x3ff) )
       {
             P2Dec->ThreeDig = 0x999;
             IsRedundant[cno] = 1;
       }
       else /* no redundancy   */
       {
        if (P2UnDec->undecoded.V==0)
        {
          P2Dec->decoded.A=0;
          P2Dec->decoded.B=P2UnDec->undecoded.P;
          P2Dec->decoded.C=P2UnDec->undecoded.Q;
          P2Dec->decoded.D=P2UnDec->undecoded.R;
          P2Dec->decoded.E=0;
          P2Dec->decoded.F=P2UnDec->undecoded.S;
          P2Dec->decoded.G=P2UnDec->undecoded.T;
          P2Dec->decoded.H=P2UnDec->undecoded.U;
          P2Dec->decoded.I=0;
          P2Dec->decoded.J=P2UnDec->undecoded.W;
          P2Dec->decoded.K=P2UnDec->undecoded.X;
          P2Dec->decoded.M=P2UnDec->undecoded.Y;
        }
        else if ((P2UnDec->undecoded.V==1) && (P2UnDec->undecoded.W==0)
              && (P2UnDec->undecoded.X==0))
        {
          P2Dec->decoded.A=0;
          P2Dec->decoded.B=P2UnDec->undecoded.P;
          P2Dec->decoded.C=P2UnDec->undecoded.Q;
          P2Dec->decoded.D= P2UnDec->undecoded.R;
          P2Dec->decoded.E=0;
          P2Dec->decoded.F=P2UnDec->undecoded.S;
          P2Dec->decoded.G=P2UnDec->undecoded.T;
          P2Dec->decoded.H=P2UnDec->undecoded.U;
          P2Dec->decoded.I=1;
          P2Dec->decoded.J=0;
          P2Dec->decoded.K=0;
          P2Dec->decoded.M=P2UnDec->undecoded.Y;
        }
        else if ((P2UnDec->undecoded.V==1) && (P2UnDec->undecoded.W==0)
              && (P2UnDec->undecoded.X==1))
        {
          P2Dec->decoded.A=0;
          P2Dec->decoded.B=P2UnDec->undecoded.P;
          P2Dec->decoded.C=P2UnDec->undecoded.Q;
          P2Dec->decoded.D=P2UnDec->undecoded.R;
          P2Dec->decoded.E=1;
          P2Dec->decoded.F=0;
          P2Dec->decoded.G=0;
          P2Dec->decoded.H=P2UnDec->undecoded.U;
          P2Dec->decoded.I=0;
          P2Dec->decoded.J=P2UnDec->undecoded.S;
          P2Dec->decoded.K=P2UnDec->undecoded.T;
          P2Dec->decoded.M=P2UnDec->undecoded.Y;
        }
        else if ((P2UnDec->undecoded.V==1) && (P2UnDec->undecoded.W==1) \
              && (P2UnDec->undecoded.X==0))
        {
          P2Dec->decoded.A=1;
          P2Dec->decoded.B=0;
          P2Dec->decoded.C=0;
          P2Dec->decoded.D=P2UnDec->undecoded.R;
          P2Dec->decoded.E=0;
          P2Dec->decoded.F=P2UnDec->undecoded.S;
          P2Dec->decoded.G=P2UnDec->undecoded.T;
          P2Dec->decoded.H=P2UnDec->undecoded.U;
          P2Dec->decoded.I=0;
          P2Dec->decoded.J=P2UnDec->undecoded.P;
          P2Dec->decoded.K=P2UnDec->undecoded.Q;
          P2Dec->decoded.M=P2UnDec->undecoded.Y;
        }
        else if ((P2UnDec->undecoded.V==1) && (P2UnDec->undecoded.W==1) \
              && (P2UnDec->undecoded.X==1) && (P2UnDec->undecoded.S==0) \
              && (P2UnDec->undecoded.T==0))
        {
          P2Dec->decoded.A=1;
          P2Dec->decoded.B=0;
          P2Dec->decoded.C=0;
          P2Dec->decoded.D=P2UnDec->undecoded.R;
          P2Dec->decoded.E=1;
          P2Dec->decoded.F=0;
          P2Dec->decoded.G=0;
          P2Dec->decoded.H=P2UnDec->undecoded.U;
          P2Dec->decoded.I=0;
          P2Dec->decoded.J=P2UnDec->undecoded.P;
          P2Dec->decoded.K=P2UnDec->undecoded.Q;
          P2Dec->decoded.M=P2UnDec->undecoded.Y;
        }
        else if ((P2UnDec->undecoded.V==1) && (P2UnDec->undecoded.W==1) \
              && (P2UnDec->undecoded.X==1) && (P2UnDec->undecoded.S==0) \
              && (P2UnDec->undecoded.T==1))
        {
          P2Dec->decoded.A=1;
          P2Dec->decoded.B=0;
          P2Dec->decoded.C=0;
          P2Dec->decoded.D=P2UnDec->undecoded.R;
          P2Dec->decoded.E=0;
          P2Dec->decoded.F=P2UnDec->undecoded.P;
          P2Dec->decoded.G=P2UnDec->undecoded.Q;
          P2Dec->decoded.H=P2UnDec->undecoded.U;
          P2Dec->decoded.I=1;
          P2Dec->decoded.J=0;
          P2Dec->decoded.K=0;
          P2Dec->decoded.M=P2UnDec->undecoded.Y;
        }
        else if ((P2UnDec->undecoded.V==1) && (P2UnDec->undecoded.W==1) \
              && (P2UnDec->undecoded.X==1) && (P2UnDec->undecoded.S==1) \
              && (P2UnDec->undecoded.T==0))
        {
          P2Dec->decoded.A=0;
          P2Dec->decoded.B=P2UnDec->undecoded.P;
          P2Dec->decoded.C=P2UnDec->undecoded.Q;
          P2Dec->decoded.D=P2UnDec->undecoded.R;
          P2Dec->decoded.E=1;
          P2Dec->decoded.F=0;
          P2Dec->decoded.G=0;
          P2Dec->decoded.H=P2UnDec->undecoded.U;
          P2Dec->decoded.I=1;
          P2Dec->decoded.J=0;
          P2Dec->decoded.K=0;
          P2Dec->decoded.M=P2UnDec->undecoded.Y;
        }
        else if ((P2UnDec->undecoded.V==1) && (P2UnDec->undecoded.W==1) \
              && (P2UnDec->undecoded.X==1) && (P2UnDec->undecoded.S==1) \
              && (P2UnDec->undecoded.T==1))
        {
          P2Dec->decoded.A=1;
          P2Dec->decoded.B=0;
          P2Dec->decoded.C=0;
          P2Dec->decoded.D=P2UnDec->undecoded.R;
          P2Dec->decoded.E=1;
          P2Dec->decoded.F=0;
          P2Dec->decoded.G=0;
          P2Dec->decoded.H=P2UnDec->undecoded.U;
          P2Dec->decoded.I=1;
          P2Dec->decoded.J=0;
          P2Dec->decoded.K=0;
          P2Dec->decoded.M=P2UnDec->undecoded.Y;
        }
        else
        {
          if (EnableTrace[cno] >= 2)
          {
           printf("WARNING WARNING: I could not unpack the three digits \n");
          }
          else{}
        }
      }/* end of the regular stuff  */
      if(EnableTrace[cno] >= 2) /* tracing the numbers */
      {
       printf("Unpacked three digits = ");
      /*printf("%u", P2Dec->decoded.A);
       printf("%u", P2Dec->decoded.B);
       printf("%u", P2Dec->decoded.C);
       printf("%u",P2Dec->decoded.D);
       printf("%u",P2Dec->decoded.E);
       printf("%u",P2Dec->decoded.F);
       printf("%u",P2Dec->decoded.G);
       printf("%u",P2Dec->decoded.H);
       printf("%u",P2Dec->decoded.I);
       printf("%u",P2Dec->decoded.J);
       printf("%u",P2Dec->decoded.K);
       printf("%u = ",P2Dec->decoded.M);
       printf("%d = ",P2Dec->ThreeDig); */
       printf("%x\n",P2Dec->ThreeDig);
      }
      else{}

}

  /* decodes and encodes Combined Field (CF) */
short Decode_CF(struct CF_Parts *CF , unsigned short encoded_cf, int cno, union The_CV *TheCF)     /* decode the cf field */
{
          struct lmd { /* Left most digit of the DFP number */
                  unsigned :12;           /*  Upper 12 bits */
                  unsigned lmd1 :1;       /* leftmost digit bit1 - from left*/
                  unsigned lmd2 :1;       /* leftmost digit bit2 - from left */
                  unsigned lmd3 :1;       /* leftmost digit bit3 - from left */
                  unsigned lmd4 :1;       /* leftmost digit bit4 - from left */
          };

          union The_lmd {
                  struct lmd LMD;
                  short  lmd_val;
          } TheLMD;

          TheLMD.lmd_val = 0;
          TheCF->cf_val = encoded_cf; /* input */

          if ((TheCF->cf.A == 1) && (TheCF->cf.B == 1)) {
                  CF->EXPMSB1 = TheCF->cf.C;
                  CF->EXPMSB2 = TheCF->cf.D;
                  CF->LMD1 = 1;
                  CF->LMD2 = 0;
                  CF->LMD3 = 0;
                  CF->LMD4 = TheCF->cf.E;
          }
          else {
                  CF->EXPMSB1 = TheCF->cf.A;
                  CF->EXPMSB2 = TheCF->cf.B;
                  CF->LMD1 = 0;
                  CF->LMD2 = TheCF->cf.C;
                  CF->LMD3 = TheCF->cf.D;
                  CF->LMD4 = TheCF->cf.E;
          }

          if(EnableTrace[cno] >= 2) { /* tracing the numbers */
                  printf("The CF = %x\n", TheCF->cf_val);
          }
          else{}

          TheLMD.LMD.lmd1 = CF->LMD1;  /* 4 bits to be returned to the caller*/
          TheLMD.LMD.lmd2 = CF->LMD2;
          TheLMD.LMD.lmd3 = CF->LMD3;
          TheLMD.LMD.lmd4 = CF->LMD4;

          if(EnableTrace[cno] >= 2) { /* tracing the numbers */
                  printf("ret from Decode_CF = %x\n", TheLMD.lmd_val);
          }
          else{}

          return  TheLMD.lmd_val;
}/* end decoding CF field */


void DoInit(int cno) /* do initialization */
{
	char DfpClass[10],DfpType[10];

	switch (DFPType[cno]) { /* to set up bias exp cont length and # of 3 digit sets */
		case SHORT:
			bxcfLen[cno] = 6;
			bxcfLen_MASK[cno] = 0x0000003f;
			NoThreeDigSets[cno] = 2; /* for short */
			NoNibles[cno] = 10;      /* ~ number of nibles in a decoded short */
			strcpy(DfpType,"SHORT");
			break;
		case LONG:
			bxcfLen[cno] = 8;
			bxcfLen_MASK[cno] = 0x000000ff;
			NoThreeDigSets[cno] = 5; /* for long */
			NoNibles[cno] = 19;      /* ~ number of nibles in a decoded long*/
			strcpy(DfpType,"LONG");
			break;
		case EXTENDED:
			bxcfLen[cno] = 12;
			bxcfLen_MASK[cno] = 0x00000fff;
			NoThreeDigSets[cno] = 11; /* for extended  */
			NoNibles[cno] = 38;      /* ~ number of nibles in a decoded extended*/
			strcpy(DfpType,"EXTENDED");
			break;
		default:
			printf("Operand type is not known - you should expect errors\n");
	}/* end of switch */

	switch (DFPClass[cno]) {
		case ZEERO:
			strcpy(DfpClass,"ZERO");
			break;
		case INFINIT:
			strcpy(DfpClass,"INFINITY");
			break;
		case qNaN:
			strcpy(DfpClass,"qNaN");
			break;
		case sNaN:
			strcpy(DfpClass,"sNaN");
			break;
		case NORM:
			strcpy(DfpClass,"NORM");
			break;
		case LARGE:
			strcpy(DfpClass,"LARGE");
			break;
		case TINY:
			strcpy(DfpClass,"TINY");
			break;
		case SMALL:
			strcpy(DfpClass,"SMALL");
			break;
		case SUBNORMAL:
			strcpy(DfpClass,"SUBNORMAL");
			break;
		default:
			printf("Unknown DFPClass %d\n", DFPClass[cno]);
			return;
			break;
	}
}/* end DoInit */

/* print decoded operand */
/* Print the decoded operand in a formatted manner:  sign+fraction+power.
   Let us take the extended case. We want the fraction to seem DFP digits.
   Therefore, we need to perform a few shifts. One bit may be needed to
   come from the adjacent double (power and sign occupy 15 bits).
   So, bit C of the following diagram goes into the position of bit D (after
   the double word is shifted to the left. Bit A goes into bit B position.
    _______________________________________________
   | 15 bits (sign+exp) |                          |
   |____________________|__________________________|
    _______________________________________________
   |                                               |
   |_______________________________________________|
    _______________________________________________
   |                                               |
   |_______________________________________________|

*/
void PrintDecoded(unsigned long long *TheDecoded, int cno)
{
	unsigned int i,TempSign, Power;
	unsigned long long BitA,BitC;
	if (EnableTrace[cno] >=2) {
		printf("Unformated decoded number \n");
		for(i=0;i<=DFPType[cno];i++) {  /* up to 3 double words */
			/* PrintLong(&TheDecoded[i]); */
			printf("\n");
		}
	}
	else{}

	DoInit(cno);  /* we need to know basic info about the operand */

	/* format the decoded */
	TempSign = TheDecoded[0] >> (63);                /* leftmost bit is the sign */
	TheDecoded[0] = TheDecoded[0] & 0x7fffffffffffffffULL;/* get rid of the sign*/
	Power = TheDecoded[0] >> (64-(1+2+bxcfLen[cno]));       /* what is the power */
	TheDecoded[0] = TheDecoded[0] & 0x7fffffffffffffffULL;/* get rid of the sign*/

	printf("+++++++++++++++++++ User friendly format ++++++++++++++++++++++\n");
	printf("                   sign + fraction + 10^expo  \n");

	if (DFPType[cno] == SHORT) {
		*TheDecoded = *TheDecoded & SHORT_HIGHER_BYTES;/* mask out sign and expo*/
		*TheDecoded = *TheDecoded <<1;                 /*to align the digits   */
	}
	else if (DFPType[cno] == LONG) {
		*TheDecoded = *TheDecoded & LONG_HIGHER_BYTES; /* mask out sign and expo*/
		BitA = *TheDecoded <<63;                   /* need last bit */
		*TheDecoded = *TheDecoded >> 1;            /*align digits   */
		TheDecoded++;
		*TheDecoded = *TheDecoded >> 1;            /*align digits   */
		*TheDecoded = *TheDecoded | BitA;          /*append the bit   */
		TheDecoded--;
	}
	else {
		*TheDecoded = *TheDecoded & EXTE_HIGHER_BYTES; /* mask out sign and expo*/
		BitA = *TheDecoded <<63;                   /* need to get bit A */
		*TheDecoded = *TheDecoded>>1;               /* get rid off Bit A */
		TheDecoded++;
		BitC = *TheDecoded <<63;                   /* need to get bit C */
		*TheDecoded = *TheDecoded>>1;               /* get rid off Bit C and make room
                                                for Bit A */
		*TheDecoded = *TheDecoded | BitA;          /* Bit A takes Bit B position */
		TheDecoded++;
		*TheDecoded = *TheDecoded>>1;               /* Make room for  bit C */
		*TheDecoded = *TheDecoded | BitC;          /* Bit C takes Bit D position */
		TheDecoded = TheDecoded - 2;
	}

	if (TempSign == 1)
		printf("Decoded operand = - ");
	else
		printf("Decoded operand = + ");
	for(i=0;i<=DFPType[cno];i++) {  /* up to 3 double words */
		/* PrintLong(TheDecoded); */
		printf(" %llx", *TheDecoded);
		TheDecoded++;
	}
	printf(" X 10^(%d)\n", Power);  /* times ten to (power) */
	printf("------------------------- End Decoded ---------------------------\n");
	printf("\n");
}

/******************************************************************************
 Build the operand in a non-packed manner. The encoder will be called later
   to pack it.
******************************************************************************/
void GenDFPOP(unsigned long long *Operand, int cno)
{
	union the_short TheShort;
	union the_long TheLong;
	union the_ext TheExt;

    *Operand = 0;    /* clear 64 bit field  - min*/
    if ((DFPType[cno] == EXTENDED) | (DFPType[cno] == LONG)){
		Operand++;
		*Operand = 0;   /* decoded long occupies 128 bit */
		if (DFPType[cno] == EXTENDED) {
			Operand++;    /* decoded extended occupies ~150 bits */
			*Operand = 0;
			Operand--;
		}
   	else {}
   	Operand--;
    }
    else{}

    TheShort.TheOp_val=0;
    TheLong.TheOp_val=0;
    TheExt.TheOp_val=0;

    switch (DFPClass[cno]) {
	case (INFINIT)	: GenINFINITY(Operand, cno, &TheShort, &TheLong, &TheExt);
			  break;
	case (qNaN)	: GenNaN(Operand, cno, &TheShort, &TheLong, &TheExt);
			  break;
	case (sNaN)	: GenNaN(Operand, cno, &TheShort, &TheLong, &TheExt);
			  break;
	case (ZEERO)	: GenZERO(Operand, cno, &TheShort, &TheLong, &TheExt);
			  break;
	case (NORM)	: GenNORMAL(Operand, cno, &TheShort, &TheLong, &TheExt);
			  break;
	case (SUBNORMAL): GenTINY_SMALL_SUBNORMAL(Operand, cno, &TheShort, &TheLong, &TheExt);
			  break;
	case (LARGE)	: GenLARGE(Operand, cno, &TheShort, &TheLong, &TheExt);
			  break;
	case (SMALL)	: GenTINY_SMALL_SUBNORMAL(Operand, cno, &TheShort, &TheLong, &TheExt);
			  break;
	case (TINY)	: GenTINY_SMALL_SUBNORMAL(Operand, cno, &TheShort, &TheLong, &TheExt);
			  break;
    }
}/* end GenDFPOP */

void GenINFINITY(unsigned long long *Operand , int cno, union the_short *TheShort, union the_long *TheLong, union the_ext *TheExt)
{
    int rand_ret, Round;
    unsigned long long rand_op;
    unsigned long long *RandOp;

    RandOp = &rand_op;
    *RandOp = 0;
   rand_ret = GenRand(cno);
   if (rand_ret%2 != 0)  /* 50% of the time generate default infinity */
   {
     Round = 1;
     DFPOperand(RandOp, Round,cno);   /* generate random dfp number */
     if (DFPType[cno] == SHORT){
	TheShort->TheOp_val = TheShort->TheOp_val | *RandOp;
	TheShort->encoded.CF = 0x1e;
	*Operand = TheShort->TheOp_val;
     }
     else if (DFPType[cno] == LONG) {
	TheLong->TheOp_val = TheLong->TheOp_val | *RandOp;
	TheLong->encoded.CF = 0x1e;
	*Operand = TheLong->TheOp_val;
     }
     else if (DFPType[cno] == EXTENDED) {
	TheExt->TheOp_val = TheExt->TheOp_val | *RandOp;
	TheExt->encoded.CF = 0x1e;
	*Operand = TheExt->TheOp_val;
	Operand++;
	DFPOperand(RandOp, Round,cno);   /* generate random dfp number */
	*Operand = *RandOp;
	Operand--;
     }
     else{}
   }
   else {/* CF and all zeros are fsufficient for all types */
	TheShort->encoded.CF = 0x1e;
	*Operand = TheShort->TheOp_val;
	if (DFPType[cno] == SHORT){
	}
   }
}/* infinity operand */


void GenNaN(unsigned long long *Operand ,int cno, union the_short *TheShort, union the_long *TheLong, union the_ext *TheExt)
{
    int rand_ret,Round;
    unsigned long long rand_op;
    unsigned long long *RandOp;

    RandOp = &rand_op;
    rand_ret=GenRand(cno);
    *RandOp = 0;

    if (rand_ret%2 != 0) {  /* 50% of the time default infinity */
	Round = 1;
	DFPOperand(RandOp, Round, cno);   /* generate random dfp number */
	if (DFPType[cno] == SHORT) {
	    TheShort->TheOp_val = TheShort->TheOp_val | *RandOp;
	    TheShort->encoded.CF = 0x1f;
	    if(DFPClass[cno] == qNaN)
		TheShort->encoded.BXCF &= 0x1f;
	    else
		TheShort->encoded.BXCF |= 0x20;
	    *Operand = TheShort->TheOp_val;
	}
	else if (DFPType[cno] == LONG) {
	    TheLong->TheOp_val = TheLong->TheOp_val | *RandOp;
	    TheLong->encoded.CF = 0x1f;
	    if(DFPClass[cno] == qNaN)
                TheLong->encoded.BXCF &= 0x7f;
	    else
		TheLong->encoded.BXCF |= 0x80;
	    *Operand = TheLong->TheOp_val;
	}
	else if (DFPType[cno] == EXTENDED) {
	    TheExt->TheOp_val = TheExt->TheOp_val | *RandOp;
	    TheExt->encoded.CF = 0x1f;
	    if(DFPClass[cno] == qNaN)
		TheExt->encoded.BXCF &= 0x7ff;
	    else
		TheExt->encoded.BXCF &= 0x800;
	    *Operand = TheExt->TheOp_val;
     	    /* for the extended - do not bother with right reg of the pair - whatever
        	data that was there should be acceptable */
     	}
        else{}
    }
    else {
	if (DFPType[cno] == SHORT) {
	    TheShort->encoded.CF = 0x1f;
	    if(DFPClass[cno] == qNaN)
                TheShort->encoded.BXCF &= 0x1f;
            else
                TheShort->encoded.BXCF |= 0x20;
	    *Operand = TheShort->TheOp_val;
	}
	else if (DFPType[cno] == LONG) {
	    TheLong->encoded.CF = 0x1f;
	    if(DFPClass[cno] == qNaN)
                TheLong->encoded.BXCF &= 0x7f;
            else
                TheLong->encoded.BXCF |= 0x80;
	    *Operand = TheLong->TheOp_val;
	}
	else if (DFPType[cno] == EXTENDED) {
	    TheExt->encoded.CF = 0x1f;
	    if(DFPClass[cno] == qNaN)
                TheExt->encoded.BXCF &= 0x7ff;
            else
                TheExt->encoded.BXCF |= 0x800;
	    *Operand = TheExt->TheOp_val;
    	}
    }
}/* NaN operand */



void GenNORMAL(unsigned long long *Operand ,int cno, union the_short *TheShort, union the_long *TheLong, union the_ext *TheExt)
{
   int Round;
   unsigned long long rand_op;
   unsigned long long *RandOp;
   RandOp = &rand_op;
   Round = 0;
   switch (DFPType[cno])
   {
	case SHORT:
		Round++;    /* making the first call to the generator */
		DFPOperand(RandOp, Round, cno);   /* generate random dfp number */
		*RandOp = *RandOp & SHORT_DECODED_MASK;  /* delete unused bits */
		TheShort->decoded.expo =  (GenRand(cno)%MAX_EXP_SHORT);/* random exponent*/
		TheShort->TheOp_val = TheShort->TheOp_val | *RandOp; /* random fraction */
		*Operand = TheShort->TheOp_val;
		break;

	case LONG:
		Round++;    /* making the first call to the generator */
		DFPOperand(RandOp, Round,cno);   /* generate random dfp number */
		TheLong->decoded.expo =  (GenRand(cno)%MAX_EXP_LONG);/* random exponent*/
		TheLong->TheOp_val = TheLong->TheOp_val | *RandOp;
		*Operand = TheLong->TheOp_val;
		Round++;        /* making the second call to the generator */
		DFPOperand(RandOp, Round,cno);   /* generate random dfp number */
		*Operand = *Operand | *RandOp >> 63; /* need  most significant bit */
		*RandOp = *RandOp << 1;              /* since lost that bit */
		Operand++;
		*RandOp = *RandOp & LONG_DECODED_MASK;/* get rid off unused*/
		*Operand = *RandOp;
		Operand--;
		break;

	case EXTENDED:
		Round++;    /* calling the firt time */
		DFPOperand(RandOp, Round,cno );   /* generate random dfp number */
		TheExt->decoded.expo =  (GenRand(cno)%MAX_EXP_EXT);/* random exponent*/
		TheExt->TheOp_val = TheExt->TheOp_val | *RandOp;
		*Operand = TheExt->TheOp_val;
		Round++; /* calling the second time */
		DFPOperand(RandOp, Round,cno);   /* generate random dfp number */
		*Operand = *Operand | *RandOp >> 63; /* I need your most significant bit */
		*RandOp = *RandOp << 1;              /* since he lost that bit */
		Operand++;
		*Operand = *RandOp;
		Round++;  /* calling the generator the third time */
		DFPOperand(RandOp, Round,cno);   /* generate random dfp number */
		*Operand = *Operand | *RandOp >> 63; /* I need your most significant bit */
		*RandOp = *RandOp << 1;              /* since he lost that bit */
		Operand++;
		*RandOp = *RandOp & EXTENDED_DECODED_MASK;
		*Operand = *RandOp;
		Operand = Operand - 2;
		break;

    default: if (EnableTrace[cno] > 0) {
       		printf("unknow operand type %d\n", DFPType[cno]);
     	     }
	     else{}
   }
}/* Normal operand */


void GenZERO(unsigned long long *Operand , int cno, union the_short *TheShort, union the_long *TheLong, union the_ext *TheExt)
{
    int rand_ret;
    rand_ret = GenRand(cno)%3; /* 3 possibilities for the CF */
  switch (DFPType[cno])
  {
    case SHORT : if (rand_ret == 0)
		    TheShort->encoded.CF = 0x00;
		 else if (rand_ret == 1)
		    TheShort->encoded.CF = 0x8;
		 else
		    TheShort->encoded.CF = 0x10;

		 TheShort->encoded.BXCF = GenRand(cno)%64;
		 TheShort->encoded.TheRest = 0;
		 TheShort->encoded.lower = 0;
		 *Operand = TheShort->TheOp_val;
		 break;

    case LONG:  if (rand_ret == 0)
		    TheLong->encoded.CF = 0x00;
		else if (rand_ret == 1)
		    TheLong->encoded.CF = 0x8;
		else
		    TheLong->encoded.CF = 0x10;

		TheLong->encoded.BXCF = GenRand(cno)%256;
		TheLong->encoded.TheRest = 0;
		TheLong->encoded.lower = 0;
		*Operand = TheLong->TheOp_val;
		break;

    case EXTENDED: if (rand_ret == 0)
		       TheExt->encoded.CF = 0x00;
		   else if (rand_ret == 1)
		       TheExt->encoded.CF = 0x8;
		   else
		       TheExt->encoded.CF = 0x10;

		   TheExt->encoded.BXCF = GenRand(cno)%4096;
		   TheExt->encoded.TheRest = 0;
		   TheExt->encoded.lower = 0;
		   *Operand = TheExt->TheOp_val;
		   Operand++;   /* to point to right reg of the pair */
		   *Operand = 0; /* zero out R+2 value */
		   Operand--;
		   break;
    default:
       if (EnableTrace[cno] > 0)
       {
        printf("Unknow DFPType -----------------  Unknow DFPType\n");
       }
       else{}
  } /* end switch */

}/* zero operand */

void GenLARGE(unsigned long long *Operand,int cno, union the_short *TheShort, union the_long *TheLong, union the_ext *TheExt)
{
  int DoMax;
  unsigned int temp,i;
  unsigned long long long_temp;
  DoMax = GenRange(1,4,cno);
  switch (DFPType[cno])
  {
	case SHORT:
		if (DoMax == 2) /* make exponent as large as possible */
		    TheShort->decoded.expo = GenRange(180,MAX_EXP_SHORT ,cno);/*random exponent*/
		else
		    TheShort->decoded.expo = GenRange(1,MAX_EXP_SHORT ,cno);/* random exponent*/
		temp = 0x9999;               /* large fraction */
		for (i=0;i<2;i++) {
		    temp = temp << 4;
		    temp = temp | GenRand(cno)%10;
		}
		temp = temp >> 1;  /* we shifted one too many */
		TheShort->decoded.TheRest = temp;
		*Operand = TheShort->TheOp_val;
		break;

	case LONG:
		if (DoMax == 2) /* make exponent as large as possible */
		    TheLong->decoded.expo = GenRange(758,MAX_EXP_LONG,cno);/* random exponent*/
		else
		    TheLong->decoded.expo = GenRange(1,MAX_EXP_LONG,cno);/* random exponent*/

		temp = 0x99999;     /* large fraction */
		TheLong->decoded.TheRest = temp;  /* 5 out 16 digits done */
		for (i=0;i<8;i++) {
		    TheLong->decoded.lower = TheLong->decoded.lower | GenRange(5,9,cno);
		if (i<7)
		    TheLong->decoded.lower = TheLong->decoded.lower << 4;
		}
		TheLong->decoded.TheRest =    /* need one bit from lower */
                TheLong->decoded.TheRest | TheLong->decoded.lower>>31;
		TheLong->decoded.lower = TheLong->decoded.lower<<1;
		*Operand = TheLong->TheOp_val;
		temp = 0;

		for (i=0;i<3;i++) {   /* do the rest of digits: 16 - 5 - 8 */
		    temp = temp | GenRand(cno);
		    temp = temp << 4;
		}
		temp = temp << 20;
		*Operand = *Operand | (temp >> 31);  /* again one bit was needed there*/
		Operand++;
		temp = temp << 1;   /* get rid of that bit */
		*Operand = *Operand | temp;  /* append the three digits */
		*Operand = *Operand << 32;
		Operand--;
		break;

	case EXTENDED:
		if (DoMax == 2)
		    TheExt->decoded.expo = GenRange(12277,MAX_EXP_EXT,cno);/* random exponent*/
		else
		    TheExt->decoded.expo = GenRange(1,MAX_EXP_EXT,cno);/* random exponent*/
		temp = 0x9999;     /* large fraction */
		TheExt->decoded.TheRest = temp;  /* 5 out 16 digits done */
		for (i=0;i<8;i++) {
		    TheExt->decoded.lower = TheExt->decoded.lower | GenRange(5,9,cno);
		if (i<7)
		    TheExt->decoded.lower = TheExt->decoded.lower << 4;
		}
		TheExt->decoded.TheRest =    /* need one bit from lower */
		TheExt->decoded.TheRest | TheExt->decoded.lower>>31;
		TheExt->decoded.lower = TheExt->decoded.lower<<1;
		*Operand = TheExt->TheOp_val;
		long_temp = 0;
		for (i=0;i<16;i++)  {  /* do full 64 bit */
		    long_temp = long_temp | GenRange(5,9 ,cno);
		if(i<15)
		    long_temp = long_temp << 4;
		}
		*Operand = *Operand | (long_temp >> 63);/* again one bit was needed there*/
		Operand++;
		long_temp = long_temp << 1;   /* get rid of that bit */
		*Operand = *Operand | long_temp;  /* append the three digits */
		*Operand = *Operand << 1;

		for (i=0;i<6;i++) {  /* do the rest of digits: 34-4-8-16 */
		    temp = temp | GenRand(cno);
		    temp = temp << 4;
		}
		temp = temp << 20;
		*Operand = *Operand | (temp >> 31);  /* again one bit was needed there*/
		Operand++;
		temp = temp << 1;   /* get rid of that bit */
		*Operand = *Operand | temp;  /* append the three digits */
		*Operand = *Operand << 32;
		Operand = Operand - 2;
		break;

    default: break;
  }
} /* large operand */

void GenTINY_SMALL_SUBNORMAL(unsigned long long *Operand , int cno, union the_short *TheShort, union the_long *TheLong, union the_ext *TheExt)
{
  unsigned int temp,i;
  temp = 0;
  for (i=0;i<4;i++)  { /* generate a tiny fraction */
     if (GenRand(cno)%6==0) {
	temp = temp | (GenRand(cno)%10);
    }
    else{}
    if (i<3)
      temp = temp << 4;
  }/* end for loop */

  switch (DFPType[cno])
  {
	case SHORT:
		if (DFPClass[cno] == SMALL)
		    TheShort->decoded.expo = GenRange(1,128,cno);/* small and random exponent*/
		else if (DFPClass[cno] == TINY)
		    TheShort->decoded.expo = GenRange(1,10,cno);/* tiny and random exponent*/
		else if (DFPClass[cno] == SUBNORMAL)
		    TheShort->decoded.expo = GenRange(0,3,cno);/* sub normal */
		else{}

		TheShort->decoded.TheRest = temp;
		TheShort->decoded.TheRest = (temp>>1);
		*Operand = TheShort->TheOp_val;
		break;

	case LONG:
		if (DFPClass[cno] == SMALL)
		    TheLong->decoded.expo = GenRange(1,512,cno);/* random exponent*/
		else if (DFPClass[cno] == TINY)
		    TheLong->decoded.expo = GenRange(1,20,cno);/* random exponent*/
		else if (DFPClass[cno] == SUBNORMAL)
		    TheLong->decoded.expo = GenRange(0,3,cno);/* sub normal */
		else{}
		if (GenRand(cno)%2 == 0) {
		    TheLong->decoded.TheRest = (temp<<1);
		}
		else
		    TheLong->decoded.lower = (temp<<1);

		*Operand = TheLong->TheOp_val;
		break;

	case EXTENDED:
		if (DFPClass[cno] == SMALL)
		    TheExt->decoded.expo = GenRange(1,8192,cno);/* random exponent*/
		else if (DFPClass[cno] == TINY)
		    TheExt->decoded.expo = GenRange(1,30,cno);/* random exponent*/
		else if (DFPClass[cno] == SUBNORMAL)
		    TheLong->decoded.expo = GenRange(0,3,cno);/* sub normal */
		else{}

		if (GenRand(cno)%2 == 0) {
		   TheExt->decoded.TheRest = (temp<<1);
		}
		else
		    TheExt->decoded.lower = (temp<<1);
		*Operand = TheExt->TheOp_val;
		break;

    default:
    break;
  }
}/* tiny operand */



void DFPOperand(unsigned long long *RandOp, int Round, int cno)
{
   unsigned short randy,nDigits;
   int i, shift;
   shift = 0;
   /*************************************************************************
      While generating fraction we must keep in mind the exponent
      and the sign. One must be careful and leave a room for them
      since we did not generate them yet. Also if the fraction
      does not occupy the whole 64 bit we must shift it the right
      so that the bits will be contigues with those generated earlier.
      There are cases in which the last bit of a 64 bit field is unresided,
      therefore, one has make sure that this bit is filled be most significant
      bit of the next 64 bit field (if any). This must be done by the calling
      function.
   *************************************************************************/
   switch (DFPType[cno])
   {
     case SHORT:
     /**************************************************************************
      ________________________________________________
     | sign+exp=9bits|<----  | generated 28 bits      |
     |_______________|_______|________________________|
          64 bit
      must shift the generated to the left by (64 - (9+28))
     **************************************************************************/
         shift = 64-(9+28); /* six nibles or 3 bytes are unused */
         nDigits  = 7;          /* give me seven digits */
		 break;

     case LONG:
     /**************************************************************************
      There are two cases:
      case 1: building the first 64 bits of the decoded operand
      ________________________________________________
     | sign+exp=11bits|<----  | generated 13 digits   |
     |________________|_______|_______________________|
          64 bit
      therefore, we must shift the generated to the left by (64 - (11+52) = 1)
      case 2: building the remaining of the decoded long which is 4 digits
      _______________________________________________
     |               <----    | generated 3 digits   |
     |________________________|______________________|
          64 bit
      therefore, we must shift the generated to the left by (64 - 12) = 52)
      **************************************************************************/
       if (Round == 1)
       {
         shift = 1;  /* last bit of the 64 is not filled ???? */
         nDigits = 13;
       }
       else if (Round == 2) /* first time this function is called? */
       {
         shift = 52;
         nDigits = 3;
       }
       else
       {}
		break;

	case EXTENDED:
    /**************************************************************************
      There are three cases:
      case 1: building the first 64 bits of the decoded operand
      ________________________________________________
     | sign+exp=15bits|<----  | generated 12 digits   |
     |________________|_______|_______________________|
          64 bit
      therefore, we must shift the generated to the left by (64 - (15+48) = 1)

      case 2: building the second portion of the decoded ext which is 16 digits
      _______________________________________________
     |               generated 16 digits            |
     |______________________________________________|
          64 bit
      therefore, we must shift the generated to the left by (64 - 64) = 0)

      case 3: building the remaining portion of the decoded ext which is 6 digits
      _______________________________________________
     |               <----    | generated 6 digits   |
     |________________________|______________________|
          64 bit
      therefore, we must shift the generated to the left by (64 - 24) = 40)
      **************************************************************************/
       if (Round == 1)
       {
         shift = 1;  /* last bit of the 64 is not filled ???? */
         nDigits = 12;
       }
       else if (Round == 2) /* 2nd time this function is called? */
       {
         shift = 0;
         nDigits = 16;
       }
       else if (Round == 3) /* 3rd time this function is called? */
       {
         shift = 40;
         nDigits = 6;
       }
       else
       {}
     break;

     default:
     	sprintf(msg, "Wrong type: %d", DFPType[cno]);
     	hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
   }

   *RandOp = 0;
   for (i=0;i<nDigits;i++)       /* generate specified number of digits */
   {
		randy = DFPGenRand(cno);       /* let GenRand decide the degit */
		*RandOp = *RandOp | randy;       /* append the digit to *Op */
		if (i<(nDigits-1))          /* still need to generate more? */
		{
			*RandOp = *RandOp << 4;        /* shift by one digit */
		}
		else{}
   }/* end for */
   *RandOp = *RandOp << shift;   /* shift by one digit */
}/* Random DFP number */


/* random number generator */
int GenRand(int client_no)
{
  int randy;
  unsigned long long Gen_seed = get_random_no_64(client_no);
  randy = (Gen_seed*25173+13849)%65536;
   return (randy>0)? randy : (randy*(-1)); /* to be possitive*/
   if (Gen_seed%10 < 3)
   {
     Gen_seed--;
   }
   else{}
}
int GenRange(int min, int max,int client_no)/* generate a random number in a range */
{
   int randy;
   if (min>max)
   {
     printf("GenRange is called with min %d > max %d\n", min,max);
     return min;
   }
   randy = GenRand(client_no)%(max-min+1) + min;
   return randy;
}/* Random DFP number */

/* to generate DFP random digit - 0 to 9*/
short DFPGenRand(int cno)
{
   short DFP_no;
   unsigned int Gen_seed = get_random_no_32(cno);
   DFP_no = (Gen_seed*25173+13849)%65536;
   DFP_no = DFP_no%10; /* 0 to 9 */
   return (DFP_no>0)? DFP_no : (DFP_no*(-1));
}

/* Random DFP number */

/*  This function will tell us whether the operand is finite or not */
int CheckFinite(unsigned long long *Operand)
{
  unsigned long long InfinityMask;
  unsigned long long NaNMask;
  InfinityMask =  0x7800000000000000ULL;
  NaNMask = 0x7c00000000000000ULL;

 if ((*Operand & NaNMask) == NaNMask) {
	return 1;
 }
 else if ((*Operand & InfinityMask) == InfinityMask) {
	return 1;
 }
 else
    return 0;
}

/* DFP SHORT FORMATS */

/* Zero */
void  dfp_shrt_z(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = ZEERO;
	int DataType = SHORT;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* NAN */
void dfp_shrt_qnan(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = qNaN;
	int DataType = SHORT ;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

void dfp_shrt_snan(int cno , char *ptr , int tgt_dtype)
{
        int DataClass = sNaN;
        int DataType = SHORT ;
        DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* INFINITY */
void dfp_shrt_inf(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = INFINIT;
	int DataType = SHORT ;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* NORMAL */
void dfp_shrt_n(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = NORM;
	int DataType =  SHORT;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/*SUBNORMAL */
void dfp_shrt_subn(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = SUBNORMAL;
	int DataType =  SHORT;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* NORMAL TOWARDS INFINITY  - LARGE */
void dfp_shrt_l(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = LARGE;
	int DataType =  SHORT;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* SMALL */
void dfp_shrt_s(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = SMALL;
	int DataType =  SHORT;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* TINY  */
void dfp_shrt_t(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = TINY;
	int DataType =  SHORT;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* DFP LONG FORMATS */
/* Zero */
void  dfp_long_z(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = ZEERO;
	int DataType =  LONG;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* NAN */
void dfp_long_qnan(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = qNaN;
	int DataType =  LONG;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType ,cno);
}

/* sNAN */
void dfp_long_snan(int cno , char *ptr , int tgt_dtype)
{
        int DataClass = sNaN;
        int DataType = LONG;
        DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* INFINITY */
void dfp_long_inf(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = INFINIT;
	int DataType =  LONG;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* NORMAL */
void dfp_long_n(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = NORM;
	int DataType =  LONG;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType ,cno);
}

/*SUBNORMAL */
void dfp_long_subn(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = SUBNORMAL;
	int DataType =  LONG;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType ,cno);
}

/* NORMAL TOWARDS INFINITY  - LARGE */
void dfp_long_l(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = LARGE;
	int DataType =  LONG;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType ,cno);
}

/* SMALL */
void dfp_long_s(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = SMALL;
	int DataType =  LONG;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* TINY  */
void dfp_long_t(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = TINY;
	int DataType =  LONG;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}


/* DFP QUAD FORMATS */
/* Zero */
void  dfp_quad_z(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = ZEERO;
	int DataType =  EXTENDED;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* NAN */
void dfp_quad_qnan(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = qNaN;
	int DataType = EXTENDED;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* sNAN */
void dfp_quad_snan(int cno , char *ptr , int tgt_dtype)
{
        int DataClass = sNaN;
        int DataType = EXTENDED;
        DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* INFINITY */
void dfp_quad_inf(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = INFINIT;
	int DataType =  EXTENDED;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* NORMAL */
void dfp_quad_n(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = NORM;
	int DataType =  EXTENDED;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/*SUBNORMAL */
void dfp_quad_subn(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = SUBNORMAL;
	int DataType =  EXTENDED;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

/* NORMAL TOWARDS INFINITY  - LARGE */
void dfp_quad_l(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = LARGE ;
	int DataType =  EXTENDED ;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType ,cno);
}

/* SMALL */
void dfp_quad_s(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = SHORT;
	int DataType =  EXTENDED;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType ,cno);
}

/* TINY  */
void dfp_quad_t(int cno , char *ptr , int tgt_dtype)
{
	int DataClass = TINY;
	int DataType =  EXTENDED;
	DFPGENE((unsigned long long *)ptr , DataClass , DataType , cno);
}

uint32 func_store_64_p7(uint32 reg)
{
	/*DPRINT(hlog,"%s: %d\n",__FUNCTION__, reg);*/
	return(STXSDX(reg, STORE_RA, STORE_RB));
}

uint32 func_store_128_p7(uint32 reg)
{
	/*DPRINT(hlog,"%s: %d\n",__FUNCTION__, reg);*/
	return(STORE_128(reg));
}

uint32 func_store_64_p6(uint32 reg)
{
	/*DPRINT(hlog,"%s: %d\n",__FUNCTION__, reg);*/
	return(STFDX(reg, STORE_RA, STORE_RB));
}
