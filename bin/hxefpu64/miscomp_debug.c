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

/*static char sccsid[] = "%Z%%M%    %I%  %W% %G% %U%";*/

#include "miscomp_debug.h"

extern uint32 shifted_pvr_os;
extern struct server_data global_sdata[3];
extern FILE *hlog;
extern struct htx_data hd;

static struct instruction_masks dep_instructions_array[] = {
/* addi    */ {0x38000000, 0, IMM_DATA, 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x57, "addi",  DUMMY, CPU_FIXED_ARTH, D_FORM_RT_RA_SI},
/* addis   */ {0x3C000000, 0, IMM_DATA, 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x57, "addis",  DUMMY, CPU_FIXED_ARTH, D_FORM_RT_RA_SI},
/* mfcr    */ {0x7C000026, 0, DUMMY, 0, DUMMY, 0, DUMMY, 0, GR, 21, 0, "mfcr", DUMMY, 0, XFX_FORM_RT_FXM_EOP_RC},
/* or      */ {0x7C000378, 0, GR, 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "or", DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
/* ori     */ {0x60000000, 0, GR, 21, IMM_DATA, 0, DUMMY, DUMMY, GR, 16, 0x43, "ori", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
/* and     */ {0x7C000038, 0, GR, 21, GR, 11, DUMMY, DUMMY, GR, 16, 0x44, "and", DUMMY, CPU_FIXED_LOGIC, X_FORM_RS_RA_RB_eop_rc},
/* stfd    */ {0xD8000000, 0, GR, 16, IMM_DATA, 0, DUMMY, DUMMY, BFP_DP, 21, 0x13, "stfd", (sim_fptr)&simulate_stfd, BFP_STORE_DP, D_FORM_RT_RA_D},
/* stxsdx  */ {0x7C000598, 0, GR, 16, GR, 11, DUMMY, DUMMY, SCALAR_DP, 21, 0x2,  "stxsdx",  (sim_fptr)&simulate_stxsdx, VSX_SCALAR_DP_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* stxvd2x */ {0x7C000798, 0, GR, 16, GR, 11, DUMMY, DUMMY, VECTOR_DP, 21, 0x2,  "stxvd2x",  (sim_fptr)&simulate_stxvd2x, VSX_VECTOR_DP_STORE_ONLY, FORM_XX1_XT_RA_RB},
/* mffs    */ {0xFC00048E, 0, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, BFP_DP, 21, 0x14, "mffs", (sim_fptr)&simulate_mffs, BFP_FPSCR_ONLY, X_FORM_RT_RA_RB_eop_rc},
/* stvx    */ {0x7C0001CE, 0, GR, 16, GR, 11, DUMMY, DUMMY, QGPR, 21, 0x31, "stvx", (sim_fptr)&simulate_stvx, VMX_STORE_ONLY, FORM_VT_VA_VB},
/* stswi   */ {0x7C0005AA, 0, IMM_DATA, 11  , GR, 16, DUMMY, DUMMY , GR , 21 , 0x59, "stswi",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_NB_eop},
/* oris    */ {0x64000000, 0, GR, 21, IMM_DATA,0 , DUMMY, DUMMY, GR, 16, 0x43, "oris", DUMMY, CPU_FIXED_LOGIC, D_FORM_RS_RA_UIM},
/* dcbt    */ {0x7C00022C, 0, GR, 16, GR, 11, DUMMY, DUMMY,  IMM_DATA, 21 , 0x5D, "dcbt",   DUMMY, CPU_CACHE, X_FORM_TH_RA_RB},
/* mfspr   */ {0x7C0002A6, 0, SPR_REG, 11, DUMMY ,DUMMY ,DUMMY, DUMMY, GR, 21, 0x53, "mfspr " , DUMMY, CPU_FIXED_SPR , X_FX_FORM_RT_SPR_eop},
/* mtspr   */ {0x7C0003A6, 0, GR, 21,DUMMY ,DUMMY ,DUMMY, DUMMY, SPR_REG, 16, 0x52, "mtspr " , DUMMY, CPU_FIXED_SPR , X_FX_FORM_RS_SPR_eop},
/* crxor   */ {0x4C000182, 0, CR_T, 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "crxor", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
/* creqv   */ {0x4C000242, 0, CR_T, 16, CR_T,11, DUMMY, DUMMY, CR_T, 21, 0x41, "creqv", DUMMY, CPU_COND_LOG, X_FORM_BT_BA_BB_eop_rc},
/* branch  */ {0x48000000, 0, DUMMY , DUMMY, DUMMY, DUMMY, DUMMY, DUMMY,  IMM_DATA, 2, 0x5E, "b",  DUMMY, CPU_BRANCH, I_FORM_LI_AA_LK},
/* brlink  */ {0x48000001, 0, DUMMY , DUMMY, DUMMY, DUMMY, DUMMY, DUMMY,  IMM_DATA, 2, 0x5E, "bl", DUMMY, CPU_BRANCH, I_FORM_LI_AA_LK},
/* stbcx.  */ {0x7C00056D, 0, GR, 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stbcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
/* sthcx.  */ {0x7C0005AD, 0, GR, 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "sthcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
/* stwcx.  */ {0x7C00012D, 0, GR, 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stwcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
/* stdcx.  */ {0x7C0001AD, 0, GR, 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stdcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
/* stswx   */ {0x7C00052A, 0, GR, 11, GR, 16, DUMMY , DUMMY , GR ,21, 0x55, "stswx",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
/* std     */ {0xF8000000, 0, IMM_DATA, 2 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "std",  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_DS},
/* stmw    */ {0xBC000000, 0, IMM_DATA, 0 , GR, 16, DUMMY, DUMMY , GR , 21 , 0x56, "stmw" ,  DUMMY, CPU_FIXED_STORE, D_FORM_RS_RA_D},
/* stqcx.  */ {0x7C00016D, 0, GR, 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stqcx." ,  DUMMY, CPU_ATOMIC_STORE, X_FORM_RS_RA_RB_eop},
/* lbzx    */ {0x7C0000AE, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lbzx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
/* lhzx    */ {0x7C00022E, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lhzx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
/* lwzx    */ {0x7C00002E, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "lwzx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
/* ldx     */ {0x7C00002A, 0, GR, 16, GR, 11, DUMMY, DUMMY, GR, 21, 0x40, "ldx", DUMMY, CPU_FIXED_LOAD, X_FORM_RT_RA_RB_eop_rc},
/* stbx    */ {0x7C0001AE, 0, GR, 11, GR, 16, DUMMY, DUMMY , GR, 21 , 0x55, "stbx",  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
/* sthx    */ {0x7C00032E, 0, GR, 11, GR, 16, DUMMY, DUMMY,  GR, 21 , 0x55, "sthx" ,  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
/* stwx    */ {0x7C00012E, 0, GR, 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stwx" ,  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
/* stdx    */ {0x7C00012A, 0, GR, 11, GR, 16, DUMMY, DUMMY,  GR, 21,   0x55, "stdx" ,  DUMMY, CPU_FIXED_STORE, X_FORM_RS_RA_RB_eop},
/* cmp     */ {0x7C000000, 0, GR, 11, IMM_DATA_1BIT, 21 , GR, 16, CR_T , 23 , 0x5A, "cmp",   DUMMY, CPU_FIXED_LOGIC, X_FORM_BF_L_RA_RB}
};

void decode_tc_instructions(int cno)
{
	int i, j, table_index = 0;
	uint32 *instr = NULL; 
	instruction_masks *ins_tuple;
	client_data *cptr;
	int prolog_ins, stream_ins; 
	unsigned short imm_data = 0, imm_data_flag;
	char mnemonic[50];

	cptr = global_sdata[INITIAL_BUF].cdata_ptr[cno];

	prolog_ins = cptr->prolog_size;
	stream_ins = cptr->num_ins_built;
	instr = (uint32 *)cptr->tc_ptr[INITIAL_BUF]->tc_ins + prolog_ins;

	#ifdef DBG_INSTR
	FILE * instr_fp = NULL;
	char instr_fname[128];
	sprintf(instr_fname, "%s/instr_list", hd.htx_exer_log_dir);
	instr_fp = fopen(instr_fname, "w");
	if (instr_fp == NULL) {
		printf("fopen failed for %s: %s\n", instr_fname, strerror(errno));
	}
	#endif
	bzero(cptr->dc_instr, sizeof(struct decoded_instruction) * MAX_INS_STREAM_DEPTH);

	for (i = prolog_ins, j = 0; i < (prolog_ins + stream_ins); i++, j++) {
		cptr->dc_instr[j].ea_offset = cptr->tc_ptr[INITIAL_BUF]->ea_off[i];
		if ( cptr->instr_index[i] & 0x10000000 ) { /* regular instruction stream */
        	table_index = cptr->instr_index[i] & ~(0x10000000);
			ins_tuple = &(cptr->enabled_ins_table[table_index].instr_table);
		} else if (cptr->instr_index[i] & 0x20000000) {      /* dependent instruction stream */
            table_index = cptr->instr_index[i] & ~(0x20000000);
            ins_tuple = &(dep_instructions_array[table_index]);
		}

		switch ( ins_tuple->insfrm ) {
			case D_FORM_RT_RA_D:
			{
					d_form_rt_ra_d *ptr;
					ptr = (d_form_rt_ra_d *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
                    cptr->dc_instr[j].op_val[0] = ptr->ra;
                    cptr->dc_instr[j].op_val[1] = ptr->d;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					if ((ins_tuple->tgt_dtype == BFP_SP) || (ins_tuple->tgt_dtype ==  BFP_DP) || (ins_tuple->tgt_dtype == DFP_LONG)) {
						sprintf(mnemonic, "%sf%d", mnemonic, ptr->rt);
					} else if ((ins_tuple->tgt_dtype == SCALAR_SP) || (ins_tuple->tgt_dtype == SCALAR_DP)) {
						sprintf(mnemonic, "%sx%d", mnemonic, ptr->rt);
					}
					else if ((ins_tuple->tgt_dtype == VECTOR_SP) || (ins_tuple->tgt_dtype == VECTOR_DP)) {
						sprintf(mnemonic, "%sv%d", mnemonic, ptr->rt);
                    }
 
					if ( ins_tuple->op1_dtype != DUMMY ) {
						sprintf(mnemonic,"%s,0x%x(r%d)", mnemonic, ptr->d, ptr->ra);
					}

					break;
			}

			case X_FORM_RT_RA_RB_eop_rc:
			{
					x_form_rt_ra_rb_eop_rc *ptr;
					ptr = (x_form_rt_ra_rb_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
                    cptr->dc_instr[j].op_val[0] = ptr->ra;
                    cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					if ( ins_tuple->tgt_dtype == GR || ins_tuple->tgt_dtype == GRU) {
						sprintf(mnemonic, "%sr%d", mnemonic, ptr->rt);
					}
					else if ( ins_tuple->tgt_dtype != DUMMY ) {
						sprintf(mnemonic, "%sf%d", mnemonic, ptr->rt);
					}
					else {
						sprintf(mnemonic, "%s%d", mnemonic, ptr->rt);
					}


					if ( ins_tuple->op1_dtype == GRU || ins_tuple->op1_dtype == GR ) {
							sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
					}
					else if ((ins_tuple->op1_dtype == IMM_DATA_2BIT) || (ins_tuple->op1_dtype == IMM_DATA_3BIT)) {
							sprintf(mnemonic, "%s,%d", mnemonic, ptr->ra);
					}
					else if ( ins_tuple->op1_dtype != DUMMY ) {
							sprintf(mnemonic, "%s,f%d", mnemonic, ptr->ra);
					}

					if ( ins_tuple->op2_dtype == GRU || ins_tuple->op2_dtype == GR ) {
						sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					}
					else if ( ins_tuple->op2_dtype != DUMMY ) {
							sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);
					}

					break;
			}

			case X_FORM_RT_RA_RB_eop_EH:
			{
					x_form_rt_ra_rb_eop_eh *ptr;
					ptr = (x_form_rt_ra_rb_eop_eh *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
                    cptr->dc_instr[j].op_val[0] = ptr->ra;
                    cptr->dc_instr[j].op_val[1] = ptr->rb;
                    cptr->dc_instr[j].op_val[2] = ptr->eh;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					if ( ins_tuple->tgt_dtype == GR || ins_tuple->tgt_dtype == GRU) {
						sprintf(mnemonic, "%sr%d", mnemonic, ptr->rt);
					}

					if ( ins_tuple->op1_dtype == GRU || ins_tuple->op1_dtype == GR ) {
						sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
					}

					if ( ins_tuple->op2_dtype == GRU || ins_tuple->op2_dtype == GR ) {
						sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					}

					if ((shifted_pvr_os >= SHIFTED_PVR_OS_P8) && (ptr->eop != 309 /*ldmx*/)) {
						sprintf(mnemonic, "%s,%d", mnemonic, ptr->eh);
					}

					break;
			}

			case X_FORM_BF_RA_RB_eop_rc:
			{
					x_form_bf_ra_rb_eop_rc *ptr;
					ptr = (x_form_bf_ra_rb_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->bf;
                    cptr->dc_instr[j].op_val[0] = ptr->ra;
                    cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->bf);
					if ( ins_tuple->op1_dtype != DUMMY ) {
						sprintf(mnemonic, "%s,f%d", mnemonic, ptr->ra);
					}
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);

					break;
			}

			case X_FORM_BF_BFA_eop_rc:
			{
					x_form_bf_bfa_eop_rc *ptr;
					ptr = (x_form_bf_bfa_eop_rc *)instr;
			
					cptr->dc_instr[j].tgt_val = ptr->bf;
                    cptr->dc_instr[j].op_val[0] = ptr->bfa;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->bf);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->bfa);

					break;
			}

			case X_FORM_BF_W_U_eop_rc:
			{
					x_form_bf_w_u_eop_rc *ptr;
					ptr = (x_form_bf_w_u_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->bf;
                    cptr->dc_instr[j].op_val[0] = ptr->u;
                    cptr->dc_instr[j].op_val[1] = ptr->w;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->bf);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->u);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->w);

					break;
			}

			case FORM_X_BF_UIM_FB:
			{
					Form_X_BF_UIM_FB *ptr;
					ptr = (Form_X_BF_UIM_FB *)instr;

					cptr->dc_instr[j].tgt_val = ptr->bf;
                    cptr->dc_instr[j].op_val[0] = ptr->uim;
                    cptr->dc_instr[j].op_val[1] = ptr->frb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->bf);
					if ( ins_tuple->op1_dtype == IMM_DATA ) {
						sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->uim);
					}
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->frb);

					break;
			}

			case X_FORM_RT_S_SP_RB_eop_rc:
			{
					x_form_rt_s_sp_rb_eop_rc *ptr;
					ptr = (x_form_rt_s_sp_rb_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
                    cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					if (ptr->xopcode == 322) {
						sprintf(mnemonic, "%s0x%x", mnemonic, ((ptr->s << 0x1) | (ptr->sp)));
                    	cptr->dc_instr[j].op_val[0] = (ptr->s << 0x1) | ptr->sp;
					}
					else {
						sprintf(mnemonic, "%s0x%x", mnemonic, ptr->s);
                    	cptr->dc_instr[j].op_val[0] = ptr->s;
					}
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rt);
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);

					break;
			}

			case XFL_FORM_L_FLM_W_RB_eop_rc:
			{
					xfl_form_l_flm_w_rb_eop_rc *ptr;
					ptr = (xfl_form_l_flm_w_rb_eop_rc *)instr;

                    cptr->dc_instr[j].op_val[0] = ptr->flm;
                    cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->flm);
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->l);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->w);

					break;
			}

			case A_FORM_RT_RA_RB_RC_eop_rc:
			{
					a_form_rt_ra_rb_rc_eop_rc *ptr;
					ptr = (a_form_rt_ra_rb_rc_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
                    cptr->dc_instr[j].op_val[0] = ptr->ra;
                    cptr->dc_instr[j].op_val[1] = ptr->rb;
                    cptr->dc_instr[j].op_val[2] = ptr->rc;
					/*printf("%s, rt:%d, ra:%d, rb:%d, rc:%d\n", ins_tuple->ins_name, ptr->rt, ptr->ra, ptr->rb, ptr->rc);*/
					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
				
					if (ins_tuple->tgt_dtype == GR) {
						sprintf(mnemonic, "%sr%d", mnemonic, ptr->rt);
						sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
						sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
						sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rc);
					} else  {
						sprintf(mnemonic, "%sf%d", mnemonic, ptr->rt);
						if ( ins_tuple->op1_dtype != DUMMY ) {
							sprintf(mnemonic, "%s,f%d", mnemonic, ptr->ra);
						}
						if ( ins_tuple->op3_dtype != DUMMY ) {
							sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rc);
						}
						if ( ins_tuple->op2_dtype != DUMMY ) {
							sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);
						}
					}

					break;
			}

			case VA_FORM_RS_RA_RB_RC_eop:
			{
				va_form_rs_ra_rb_rc_eop *ptr;
				ptr = (va_form_rs_ra_rb_rc_eop *)instr;
				cptr->dc_instr[j].tgt_val = ptr->ra;
                cptr->dc_instr[j].op_val[0] = ptr->rs;
                cptr->dc_instr[j].op_val[1] = ptr->rb;
                cptr->dc_instr[j].op_val[2] = ptr->rc;

				sprintf(mnemonic, "%s r%d, r%d,r%d,r%d", ins_tuple->ins_name, ptr->ra, ptr->rs, ptr->rb, ptr->rc);
		        break;
			}

			case FORM_XX1_RA_XS:
			{
					int vsr_t;
					Form_XX1_RA_XS *ptr;
					ptr = (Form_XX1_RA_XS*)instr;

					cptr->dc_instr[j].tgt_val = ptr->target;

					vsr_t = GET_REGISTER_VALUE(ptr->sx, ptr->source);
                    cptr->dc_instr[j].op_val[0] = vsr_t;

					sprintf(mnemonic, "%s r%d,x%d", ins_tuple->ins_name, ptr->target, vsr_t);
					break;
			}

			case FORM_XX1_XT_RA :
			{
					int vsr_t;
					Form_XX1_XT_RA *ptr;
					ptr = (Form_XX1_XT_RA*)instr;
					vsr_t = GET_REGISTER_VALUE(ptr->tx, ptr->target);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[0] = ptr->source;

					sprintf(mnemonic, "%s x%d,r%d", ins_tuple->ins_name, vsr_t, ptr->source);
					break;
			}

			case FORM_XX1_XT_RA_RB :
			{
					int vsr_t;
					Form_XX1_XT_RA_RB *ptr;
					ptr = (Form_XX1_XT_RA_RB*)instr;
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->target);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[0] = ptr->src1;
                    cptr->dc_instr[j].op_val[1] = ptr->src2;

					sprintf(mnemonic, "%s x%d,r%d,r%d", ins_tuple->ins_name, vsr_t,ptr->src1, ptr->src2);
					break;
			}

			case FORM_XX2_XT_XB :
			{
					int vsr_t, vsr1;
					Form_XX2_XT_XB *ptr;
					ptr = (Form_XX2_XT_XB *)instr;
					vsr1 = GET_REGISTER_VALUE(ptr->BX, ptr->src);
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->target);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[1] = vsr1;

					sprintf(mnemonic, "%s x%d,x%d", ins_tuple->ins_name, vsr_t, vsr1);
					break;
			}

			case FORM_XX2_BF_XB:
			{
					int vsr1;
					Form_XX2_BF_XB *ptr;
					ptr = (Form_XX2_BF_XB *)instr;
					vsr1 = GET_REGISTER_VALUE(ptr->BX, ptr->src);

					cptr->dc_instr[j].tgt_val = ptr->BF; 
                    cptr->dc_instr[j].op_val[1] = vsr1;

					sprintf(mnemonic, "%s %d, x%d", ins_tuple->ins_name, ptr->BF,vsr1);
					break;
			}

			case FORM_XX2_XT_UIM_XB:
			{
					int vsr_t, vsr1;
					Form_XX2_XT_UIM_XB *ptr;
					ptr = (Form_XX2_XT_UIM_XB *)instr;
					vsr1 =  GET_REGISTER_VALUE(ptr->BX, ptr->src);
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->target);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[1] = vsr1;

					sprintf(mnemonic, "%s x%d,x%d,%d", ins_tuple->ins_name,vsr_t,vsr1,ptr->uim);
					break;
			}

			case FORM_XX2_XT_UIM4_XB:
			{
					int vsr_t, vsr1;
					Form_XX2_XT_UIM4_XB *ptr;
					ptr = (Form_XX2_XT_UIM4_XB *)instr;
					vsr1 =  GET_REGISTER_VALUE(ptr->BX, ptr->src);
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->target);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[1] = vsr1;
                    cptr->dc_instr[j].op_val[0] = ptr->uim4;

					sprintf(mnemonic, "%s x%d,x%d,%d", ins_tuple->ins_name, vsr_t, vsr1, ptr->uim4);
					break;
			}

			case FORM_XX1_IMM8:
			{
					int vsr_t;
					Form_XX1_IMM8 *ptr;
					ptr = (Form_XX1_IMM8 *)instr;
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->T);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[0] = ptr->imm8;

					sprintf(mnemonic, "%s x%d,%d", ins_tuple->ins_name, vsr_t, ptr->imm8);
					break;
			}

			case FORM_XX2_DX_DC_DM:
			{
					int vsr_t, vsr1;
					uint32 dcmx = 0;
					Form_XX2_dx_dc_dm *ptr;
					ptr = (Form_XX2_dx_dc_dm *)instr;
					vsr1 =  GET_REGISTER_VALUE(ptr->BX, ptr->B);
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->T);
					dcmx = (ptr->dx << 2) | (ptr->dc << 1)| ptr->dm;

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[0] = vsr1; 
                    cptr->dc_instr[j].op_val[1] = dcmx; 

					sprintf(mnemonic, "%s x%d,x%d,%d", ins_tuple->ins_name, vsr_t, vsr1, dcmx);
					break;
			}

			case FORM_XX3_XT_XA_XB:
			{
					int vsr_t, vsr1, vsr2;
					Form_XX3_XT_XA_XB *ptr;
					ptr = (Form_XX3_XT_XA_XB *)instr;

					vsr1 = GET_REGISTER_VALUE(ptr->AX, ptr->src1);
					vsr2 = GET_REGISTER_VALUE(ptr->BX, ptr->src2);
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->target);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[0] = vsr1; 
                    cptr->dc_instr[j].op_val[1] = vsr2; 

					sprintf(mnemonic, "%s x%d,x%d,x%d", ins_tuple->ins_name, vsr_t, vsr1, vsr2);
					break;
			}

			case FORM_XX3_RC_XT_XA_XB:
			{
					int vsr_t, vsr1, vsr2;
					Form_XX3_RC_XT_XA_XB *ptr;
					ptr = (Form_XX3_RC_XT_XA_XB *)instr;

					vsr1 = GET_REGISTER_VALUE(ptr->AX, ptr->src1);
					vsr2 = GET_REGISTER_VALUE(ptr->BX, ptr->src2);
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->target);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[0] = vsr1; 
                    cptr->dc_instr[j].op_val[1] = vsr2; 

					sprintf(mnemonic, "%s x%d,x%d,x%d",ins_tuple->ins_name, vsr_t, vsr1, vsr2);

					break;
			}

			case FORM_XX3_BF_XA_XB:
			{
					int vsr1, vsr2;
					Form_XX3_BF_XA_XB *ptr;
					ptr = (Form_XX3_BF_XA_XB *)instr;

					vsr1 = GET_REGISTER_VALUE(ptr->AX, ptr->src1);
					vsr2 = GET_REGISTER_VALUE(ptr->BX, ptr->src2);

					cptr->dc_instr[j].tgt_val = ptr->BF; 
                    cptr->dc_instr[j].op_val[0] = vsr1; 
                    cptr->dc_instr[j].op_val[1] = vsr2; 

					sprintf(mnemonic, "%s %d,x%d,x%d",ins_tuple->ins_name, ptr->BF, vsr1, vsr2);
					break;
			}

			case FORM_XX3_XT_XA_XB_2Bit:
			{
					int vsr_t, vsr1, vsr2;
					Form_XX3_XT_XA_XB_2Bit *ptr;
					ptr = (Form_XX3_XT_XA_XB_2Bit *)instr;
					vsr1 = GET_REGISTER_VALUE(ptr->AX, ptr->src1);
					vsr2 = GET_REGISTER_VALUE(ptr->BX, ptr->src2);
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->target);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[0] = vsr1; 
                    cptr->dc_instr[j].op_val[1] = vsr2; 
                    cptr->dc_instr[j].op_val[2] = ptr->SHW_DM; 

					sprintf(mnemonic, "%s x%d,x%d,x%d,%d",ins_tuple->ins_name, vsr_t, vsr1, vsr2, ptr->SHW_DM);
					break;
			}

			case FORM_XX4_XT_XA_XB_XC:
			{
					int vsr_t, vsr1, vsr2, vsr3;
					Form_XX4_XT_XA_XB_XC *ptr;
					ptr = (Form_XX4_XT_XA_XB_XC *)instr;

					vsr1 = GET_REGISTER_VALUE(ptr->AX, ptr->src1);
					vsr2 = GET_REGISTER_VALUE(ptr->BX, ptr->src2);
					vsr3 = GET_REGISTER_VALUE(ptr->CX, ptr->src3);
					vsr_t = GET_REGISTER_VALUE(ptr->TX, ptr->target);

					cptr->dc_instr[j].tgt_val = vsr_t; 
                    cptr->dc_instr[j].op_val[0] = vsr1; 
                    cptr->dc_instr[j].op_val[1] = vsr2; 
                    cptr->dc_instr[j].op_val[2] = vsr3; 

					sprintf(mnemonic, "%s x%d,x%d,x%d,x%d",ins_tuple->ins_name, vsr_t, vsr1, vsr2, vsr3);
					break;
			}

			case Z_FORM_RT_RA_SH_eop_rc:
			{
					z_form_rt_ra_sh_eop_rc *ptr;
					ptr = (z_form_rt_ra_sh_eop_rc *) instr;

					cptr->dc_instr[j].tgt_val = ptr->rt; 
                    cptr->dc_instr[j].op_val[0] = ptr->ra; 
                    cptr->dc_instr[j].op_val[1] = ptr->sh; 

					sprintf(mnemonic, "%s f%d,f%d,0x%x", ins_tuple->ins_name, ptr->rt, ptr->ra, ptr->sh);
					break;
			}

			case Z_FORM_BF_RA_DM_eop_rc:
			{
					z_form_bf_ra_dm_eop_rc *ptr;
					ptr = (z_form_bf_ra_dm_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->bf; 
                    cptr->dc_instr[j].op_val[0] = ptr->ra; 
                    cptr->dc_instr[j].op_val[1] = ptr->dm; 

					sprintf(mnemonic, "%s 0x%x,f%d,0x%x", ins_tuple->ins_name, ptr->bf, ptr->ra,  ptr->dm);
					break;
			}

			case Z_FORM_RT_D_RB_RMC_eop_rc:
			{
					z_form_rt_d_rb_rmc_eop_rc *ptr;
					ptr = (z_form_rt_d_rb_rmc_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt; 
                    cptr->dc_instr[j].op_val[0] = ptr->ra_imm; 
                    cptr->dc_instr[j].op_val[1] = ptr->rb; 
                    cptr->dc_instr[j].op_val[1] = ptr->rm; 

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					if (ptr->xopcode == 67) {
						sprintf(mnemonic, "%s0x%x", mnemonic, ptr->ra_imm);
						sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rt);
					}
					else {
						sprintf(mnemonic, "%sf%d", mnemonic, ptr->rt);
						sprintf(mnemonic, "%s,f%d", mnemonic, ptr->ra_imm);
					}
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);
					sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->rm);
					break;
			}

			case Z_FORM_RT_R_RB_RMC_eop_rc:
			{
					z_form_rt_r_rb_rmc_eop_rc *ptr;
					ptr = (z_form_rt_r_rb_rmc_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt; 
                    cptr->dc_instr[j].op_val[1] = ptr->rb; 

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s0x%x", mnemonic, ptr->r);
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rt);
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);
					sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->rm);
					break;
			}

			case FORM_VT_VA_VB:
			{
					Form_VT_VA_VB *ptr;
					ptr = (Form_VT_VA_VB *) instr;
					imm_data_flag = 0;

					cptr->dc_instr[j].tgt_val = ptr->vt; 
                    cptr->dc_instr[j].op_val[0] = ptr->va; 
                    cptr->dc_instr[j].op_val[1] = ptr->vb; 

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					if (ins_tuple->tgt_dtype == GR) {
						sprintf(mnemonic, "%sr%d", mnemonic, ptr->vt);
					}
					else { 
						sprintf(mnemonic, "%sv%d", mnemonic, ptr->vt);
					}
					if (ins_tuple->op1_dtype != DUMMY) {
						if (ins_tuple->op1_dtype == GR) {
							sprintf(mnemonic, "%s,r%d", mnemonic, ptr->va);
						}
						else if ((ins_tuple->op1_dtype == QGPR) || (ins_tuple->op1_dtype == VECTOR_SP)) {
							sprintf(mnemonic, "%s,v%d", mnemonic, ptr->va);
						}
						else {
							imm_data_flag = 1;
							switch (ins_tuple->op1_dtype) {
								case IMM_DATA_2BIT:
									imm_data = ptr->va & 0x3;
									break;
								case IMM_DATA_3BIT:
									imm_data = ptr->va & 0x7;
									break;
								case IMM_DATA_4BIT:
									imm_data = ptr->va & 0xf;
									break;
								default:
									imm_data = ptr->va & 0x1f;
									break;
							}
						}
					}
					if (ins_tuple->op2_dtype != DUMMY) {
						if (ins_tuple->op2_dtype == GR) {
							sprintf(mnemonic, "%s,r%d", mnemonic, ptr->vb);
						}
						else {
							sprintf(mnemonic, "%s,v%d", mnemonic, ptr->vb);
						}
					}
					if (imm_data_flag) {
						sprintf(mnemonic, "%s,0x%x", mnemonic, imm_data);
					}
					if (ins_tuple->op3_dtype == IMM_DATA) {
						sprintf(mnemonic, "%s,0x%x", mnemonic, (ptr->xopcode & 0x3C0) >> 6);
					}
					break;
			}

			case FORM_VT_VA_VB_VC:
			{
					Form_VT_VA_VB_VC *ptr;
                    ptr = (Form_VT_VA_VB_VC *) instr;

					cptr->dc_instr[j].tgt_val = ptr->vt; 
                    cptr->dc_instr[j].op_val[0] = ptr->va; 
                    cptr->dc_instr[j].op_val[1] = ptr->vb; 
                    cptr->dc_instr[j].op_val[2] = ptr->vc; 

					sprintf(mnemonic, "%s v%d,v%d,v%d,v%d", ins_tuple->ins_name, ptr->vt, ptr->va, ptr->vb, ptr->vc);
					break;
			}

			case FORM_VT_VA_ST_SIX:
			{
					Form_VT_VA_ST_SIX *ptr;
                    ptr = (Form_VT_VA_ST_SIX *) instr;
					
					cptr->dc_instr[j].tgt_val = ptr->vt;
					cptr->dc_instr[j].op_val[0] = ptr->va;
                    cptr->dc_instr[j].op_val[1] = ptr->st; 
                    cptr->dc_instr[j].op_val[2] = ptr->six; 
				
					sprintf(mnemonic, "%s v%d,v%d,%d,%d", ins_tuple->ins_name, ptr->vt, ptr->va, ptr->st, ptr->six);
					break;
			}

			case X_FORM_BT_BA_BB_eop_rc:
			{
					x_form_bt_ba_bb *ptr;
					ptr = (x_form_bt_ba_bb *)instr;

					cptr->dc_instr[j].tgt_val = ptr->bt;
					cptr->dc_instr[j].op_val[0] = ptr->ba;
                    cptr->dc_instr[j].op_val[1] = ptr->bb; 

					sprintf(mnemonic, "%s crb%d,crb%d", ins_tuple->ins_name, ptr->bt,  ptr->ba);
					if(ins_tuple->op2_dtype != DUMMY){
						sprintf(mnemonic, "%s,crb%d", mnemonic, ptr->bb);
					}
					break;
			}

			case X_FORM_RT_RA_RB_OE_eop_rc:
			{
					x_form_rt_ra_rb_oe_eop_rc *ptr;
					ptr = (x_form_rt_ra_rb_oe_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
					cptr->dc_instr[j].op_val[0] = ptr->ra;
                    cptr->dc_instr[j].op_val[1] = ptr->rb; 

					sprintf(mnemonic, "%s r%d,r%d,r%d", ins_tuple->ins_name,  ptr->rt, ptr->ra, ptr->rb);
					break;
			}

			case X_FORM_RT_RA_RB_CY_eop:
			{
					x_form_rt_ra_rb_cy_eop *ptr;
					ptr = (x_form_rt_ra_rb_cy_eop *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
					cptr->dc_instr[j].op_val[0] = ptr->ra;
                    cptr->dc_instr[j].op_val[1] = ptr->rb; 
                    cptr->dc_instr[j].op_val[2] = ptr->cy; 

					sprintf(mnemonic, "%s r%d,r%d,r%d, %d", ins_tuple->ins_name,  ptr->rt, ptr->ra, ptr->rb, ptr->cy);
					break;
			}

			case X_FORM_RT_RA_OE_eop_rc:
			{
					x_form_rt_ra_oe_eop_rc *ptr;
					ptr = (x_form_rt_ra_oe_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
					cptr->dc_instr[j].op_val[0] = ptr->ra;

					sprintf(mnemonic, "%s r%d,r%d", ins_tuple->ins_name, ptr->rt, ptr->ra);
					break;
			}

			case D_FORM_RS_RA_UIM:
			{
					d_form_rs_ra_uim   *ptr;
					ptr = (d_form_rs_ra_uim *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;
					cptr->dc_instr[j].op_val[1] = ptr->uim;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->uim);
					break;
			}

			case X_FORM_RS_RA_RB_eop_rc:
			{
					x_form_rs_ra_rb_eop_rc *ptr;
					ptr = (x_form_rs_ra_rb_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;
					cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					break;
			}

			case X_FORM_RS_RA_eop_rc:
			{
					x_form_rs_ra_eop_rc *ptr;
					ptr = (x_form_rs_ra_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					break;
			}

			case X_FORM_RS_RA_eop:
			{
					x_form_rs_ra_eop *ptr;
					ptr = (x_form_rs_ra_eop *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					break;
			}

			case MDFORM_RS_RA_SH_MB_rc:
			{
					mdform_rs_ra_sh_mb_rc *ptr;
					ptr = (mdform_rs_ra_sh_mb_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;
					cptr->dc_instr[j].op_val[1] = ptr->sh1 + (ptr->sh2)*32;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,%d", mnemonic, ((ptr->sh1) + ((ptr->sh2)*32)) );
					if (ins_tuple->op3_dtype != IMM_DATA_1BIT) {
						sprintf(mnemonic, "%s,%d", mnemonic, ((ptr->mb1) + ((ptr->mb2)*32)) );
					}
					break;
			}

			case MFORM_RS_RA_SH_MB_ME_rc:
			{
					mform_rs_ra_sh_mb_me_rc *ptr;
					ptr = (mform_rs_ra_sh_mb_me_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;
					cptr->dc_instr[j].op_val[1] = ptr->sh;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->sh);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->mb);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->me);
					break;
			}

			case MDSFORM_RS_RA_RB_MB_eop_rc:
			{
					mdsform_rs_ra_rb_mb_eop_rc *ptr;
					ptr = (mdsform_rs_ra_rb_mb_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;
					cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					sprintf(mnemonic, "%s,%d", mnemonic, ((ptr->mb1) + ((ptr->mb2)*32)) );
					break;
			}

			case MDSFORM_RS_RA_RB_ME_eop_rc:
			{
					mdsform_rs_ra_rb_me_eop_rc *ptr;
					ptr = (mdsform_rs_ra_rb_me_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;
					cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					sprintf(mnemonic, "%s,%d", mnemonic, ((ptr->me1) + ((ptr->me2)*32)) );
					break;
			}

			case MFORM_RS_RA_RB_MB_ME_rc:
			{
					mform_rs_ra_rb_mb_me_rc *ptr;
					ptr = (mform_rs_ra_rb_mb_me_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;
					cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->mb);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->me);
					break;
			}

			case XS_FORM_RS_RA_SH_eop_rc:
			{
					xs_form_rs_ra_sh_eop_rc *ptr;
					ptr = (xs_form_rs_ra_sh_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,%d", mnemonic, ((ptr->sh1)+((ptr->sh2)*32)) );
					break;
			}

			case X_FORM_RS_RA_SH_eop_rc:
			{
					x_form_rs_ra_sh_eop_rc *ptr;
					ptr = (x_form_rs_ra_sh_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ra;
					cptr->dc_instr[j].op_val[0] = ptr->rs;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->sh);
					break;
			}

			case X_FX_FORM_RS_SPR_eop:
			{
					x_fx_form_rs_spr_eop *ptr;
					ptr = (x_fx_form_rs_spr_eop *)instr;

					cptr->dc_instr[j].tgt_val = (ptr->sprL)+((ptr->sprU)*32);
					cptr->dc_instr[j].op_val[0] = ptr->rs;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ((ptr->sprL)+((ptr->sprU)*32)));
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					break;
			}

			case X_FX_FORM_RT_SPR_eop:
			{
					x_fx_form_rt_spr_eop *ptr;
					ptr = (x_fx_form_rt_spr_eop *)instr;

					cptr->dc_instr[j].op_val[0] = (ptr->sprL)+((ptr->sprU)*32);
					cptr->dc_instr[j].tgt_val = ptr->rt;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->rt);
					sprintf(mnemonic, "%s,%d", mnemonic, ((ptr->sprL)+((ptr->sprU)*32)));
					break;
			}

			case X_FX_FORM_RS_FXM_eop:
			{
					x_fx_form_rs_fxm_eop *ptr;
					ptr = (x_fx_form_rs_fxm_eop *)instr;

					cptr->dc_instr[j].tgt_val = ptr->FXM;
					cptr->dc_instr[j].op_val[0] = ptr->rs;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, (ptr->FXM));
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rs);
					break;
			}

			case X_FORM_RS_RA_RB_eop:
			{
					x_form_rs_ra_rb_eop *ptr;
					ptr = (x_form_rs_ra_rb_eop *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rs;
					cptr->dc_instr[j].op_val[0] = ptr->rb;
					cptr->dc_instr[j].op_val[1] = ptr->ra;


					sprintf(mnemonic, "%s ", ins_tuple->ins_name);

					if ( ins_tuple->tgt_dtype == GR || ins_tuple->tgt_dtype == GRU) {
						sprintf(mnemonic, "%sr%d", mnemonic, ptr->rs);
					}

					if ( ins_tuple->op2_dtype == GRU || ins_tuple->op2_dtype == GR ) {
							sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					}

					if ( ins_tuple->op1_dtype == GRU || ins_tuple->op1_dtype == GR ) {
							sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
					}

					break;
			}

			case D_FORM_RS_RA_D:
			{
					d_form_rs_ra_d  *ptr;
					ptr = (d_form_rs_ra_d *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rs;
					cptr->dc_instr[j].op_val[0] = ptr->uim;
					cptr->dc_instr[j].op_val[1] = ptr->ra;


					sprintf(mnemonic, "%s ", ins_tuple->ins_name);

					if ( ins_tuple->tgt_dtype == GRU || ins_tuple->tgt_dtype == GR ) {
							sprintf(mnemonic, "%sr%d", mnemonic, ptr->rs);
					}

					sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->uim);
					if ( ins_tuple->tgt_dtype == GRU || ins_tuple->tgt_dtype == GR ) {
						sprintf(mnemonic, "%s(r%d)", mnemonic, ptr->ra);
					}
					break;
			}

			case D_FORM_RS_RA_DS:
			{
					d_form_rs_ra_ds  *ptr;
					ptr = (d_form_rs_ra_ds *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rs;
					cptr->dc_instr[j].op_val[0] = ptr->uim;
					cptr->dc_instr[j].op_val[1] = ptr->ra;


					sprintf(mnemonic, "%s ", ins_tuple->ins_name);

					if ( ins_tuple->tgt_dtype == GRU || ins_tuple->tgt_dtype == GR ) {
							sprintf(mnemonic, "%sr%d", mnemonic, ptr->rs);
					}

					sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->uim);
					if ( ins_tuple->tgt_dtype == GRU || ins_tuple->tgt_dtype == GR ) {
						sprintf(mnemonic, "%s(r%d)", mnemonic, ptr->ra);
					}
					break;
			}

			case D_FORM_RT_RA_SI:
			{
					d_form_rt_ra_si  *ptr;
					ptr = (d_form_rt_ra_si *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
					cptr->dc_instr[j].op_val[0] = ptr->uim;
					cptr->dc_instr[j].op_val[1] = ptr->ra;


					sprintf(mnemonic, "%s ", ins_tuple->ins_name);

					if ( ins_tuple->tgt_dtype == GRU || ins_tuple->tgt_dtype == GR ) {
							sprintf(mnemonic, "%sr%d", mnemonic, ptr->rt);
					}

					if ( ins_tuple->op2_dtype == GRU || ins_tuple->op2_dtype == GR ) {
						sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
					}
					sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->uim);
					break;
			}

			case X_FORM_L_E:
			{
					x_form_l_e  *ptr;
					ptr = (x_form_l_e *)instr;

					cptr->dc_instr[j].tgt_val = ptr->l;
					cptr->dc_instr[j].op_val[0] = ptr->e;

				    sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->l);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->e);
				    break;
			}

			case XL_FORM:
			{

				      sprintf(mnemonic, "%s ", ins_tuple->ins_name);
				      break;
			}

			case X_FORM_RS_RA_NB_eop:
			{
					x_form_rs_ra_nb_eop  *ptr;
					ptr = (x_form_rs_ra_nb_eop *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rs;
					cptr->dc_instr[j].op_val[0] = ptr->uim;
					cptr->dc_instr[j].op_val[1] = ptr->ra;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);

					if ( ins_tuple->tgt_dtype == GRU || ins_tuple->tgt_dtype == GR ) {
							sprintf(mnemonic, "%sr%d", mnemonic, ptr->rs);
					}

					if ( ins_tuple->tgt_dtype == GRU || ins_tuple->tgt_dtype == GR ) {
						sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
					}
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->uim);
					break;
			}

			case D_FORM_BF_L_RA_SI:
			{
					d_form_bf_l_ra_si  *ptr;
					ptr = (d_form_bf_l_ra_si *)instr;

					cptr->dc_instr[j].tgt_val = ptr->cr_t;
					cptr->dc_instr[j].op_val[0] = ptr->l;
					cptr->dc_instr[j].op_val[1] = ptr->uim;
					cptr->dc_instr[j].op_val[2] = ptr->ra;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%scr%d", mnemonic, ptr->cr_t);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->l);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->uim);
					break;
			}

			case X_FORM_BF_L_RA_RB:
			{
					x_form_bf_l_ra_rb  *ptr;
					ptr = (x_form_bf_l_ra_rb *)instr;

					cptr->dc_instr[j].tgt_val = ptr->cr_t;
					cptr->dc_instr[j].op_val[0] = ptr->ra;
					cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%scr%d", mnemonic, ptr->cr_t);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);

					if (ins_tuple->op2_dtype == IMM_DATA_1BIT) {
						cptr->dc_instr[j].op_val[1] = ptr->l;
						cptr->dc_instr[j].op_val[2] = ptr->rb;
						sprintf(mnemonic, "%s,%d", mnemonic, ptr->l);
					}
					
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					break;
			}

			case X_FORM_RA_RB:
			{
					x_form_ra_rb_eop  *ptr;
					ptr = (x_form_ra_rb_eop *)instr;

					cptr->dc_instr[j].op_val[0] = ptr->ra;
					cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					break;
			}

			case X_FORM_L_RA_RB:
			{
					x_form_l_ra_rb_eop  *ptr;
					ptr = (x_form_l_ra_rb_eop *)instr;

					cptr->dc_instr[j].tgt_val = ptr->l;
					cptr->dc_instr[j].op_val[0] = ptr->ra;
					cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->l);
					break;
			}

			case X_FORM_TH_RA_RB:
			{
					x_form_th_ra_rb_eop  *ptr;
					ptr = (x_form_th_ra_rb_eop *)instr;

					cptr->dc_instr[j].tgt_val = ptr->th;
					cptr->dc_instr[j].op_val[0] = ptr->ra;
					cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->th);
					break;
			}

			case B_FORM_BO_BI_BD_AA_LK:
			{
					b_form_bo_bi_bd_aa_lk  *ptr;
					ptr = (b_form_bo_bi_bd_aa_lk *)instr;

					cptr->dc_instr[j].tgt_val = ptr->bo;
					cptr->dc_instr[j].op_val[0] = ptr->bi;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d",mnemonic,ptr->bo);
					sprintf(mnemonic, "%s,%d",mnemonic,ptr->bi);
					if ((ptr->bd) & 0xff00) {
					  	sprintf(mnemonic, "%s,$-0x%x",mnemonic, (-((ptr->bd)*4) & 0x00000fff));
						cptr->dc_instr[j].op_val[1] = (-((ptr->bd)*4) & 0x00000fff);
					} else {
					  	sprintf(mnemonic, "%s,$+0x%x",mnemonic,((ptr->bd)*4));
						cptr->dc_instr[j].op_val[1] = (ptr->bd)*4;
					}
					break;
			}

			case XL_FORM_BO_BI_LK:
			{
					xl_form_bo_bi_lk  *ptr;
					ptr = (xl_form_bo_bi_lk *)instr;

					cptr->dc_instr[j].tgt_val = ptr->bo;
					cptr->dc_instr[j].op_val[0] = ptr->bi;
					cptr->dc_instr[j].op_val[1] = ptr->bh;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d",mnemonic,ptr->bo);
					sprintf(mnemonic, "%s,%d",mnemonic,ptr->bi);
					sprintf(mnemonic, "%s,%d",mnemonic,ptr->bh);
					break;
			}

			case D_FORM_RT_RA_DQ_PT:
			{
					d_form_rt_ra_uim_pt  *ptr;
					ptr = (d_form_rt_ra_uim_pt *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
					cptr->dc_instr[j].op_val[0] = ptr->uim;
					cptr->dc_instr[j].op_val[1] = ptr->ra;
					cptr->dc_instr[j].op_val[2] = ptr->pt;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d ",mnemonic,ptr->rt);
					sprintf(mnemonic, "%s,0x%d",mnemonic,ptr->uim);
					sprintf(mnemonic, "%s(r%d) ",mnemonic,ptr->ra);
					sprintf(mnemonic, "%s,%d",mnemonic,ptr->pt);
					break;
			}

			case X_FORM_CT_RA_RB_eop:
			{
					x_form_ct_ra_rb_eop  *ptr;
					ptr = (x_form_ct_ra_rb_eop *)instr;

					cptr->dc_instr[j].tgt_val = ptr->ct;
					cptr->dc_instr[j].op_val[0] = ptr->ra;
					cptr->dc_instr[j].op_val[1] = ptr->rb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d ",mnemonic,ptr->ct);
					sprintf(mnemonic, "%s,r%d ",mnemonic,ptr->ra);
					sprintf(mnemonic, "%s,r%d",mnemonic,ptr->rb);
					break;
			}

			case X_FORM_Rx_Rx_Rx_eop_rc:
			{
					x_form_rx_rx_rx_eop_rc *ptr;
					ptr = (x_form_rx_rx_rx_eop_rc *)instr;
					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->rx);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->ry);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->rz);
					break;
			}

			case X_FORM_RS_RA_FC_eop:
			{
					x_form_rs_ra_nb_eop *ptr;
					ptr = (x_form_rs_ra_nb_eop *)instr;

					cptr->dc_instr[j].op_val[0] = ptr->rs;
					cptr->dc_instr[j].op_val[1] = ptr->ra;
					cptr->dc_instr[j].op_val[2] = ptr->uim;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->rs);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->uim);
					break;
			}

			case DX_FORM_RT_D1_D0_D2:
			{
					dx_form_rt_d1_d0_d2 *ptr;
					ptr = (dx_form_rt_d1_d0_d2 *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
					cptr->dc_instr[j].op_val[0] = ptr->d1;
					cptr->dc_instr[j].op_val[1] = ptr->d0;
					cptr->dc_instr[j].op_val[2] = ptr->d2;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->rt);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->d1);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->d0);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->d2);
					break;
			}

			case X_FORM_RT_BFA:
			{
					x_form_rt_bfa *ptr;
					ptr = (x_form_rt_bfa *)instr;

					cptr->dc_instr[j].tgt_val = ptr->rt;
					if (ptr->eop == 257) { /* setbool */
						cptr->dc_instr[j].op_val[0] =(ptr->BFA << 2) | ptr->nu0; /* BA is 5 bits */
					} else { /* setb */
						cptr->dc_instr[j].op_val[0] = ptr->BFA;
					}

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->rt);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->BFA);
					break;
			}

			case X_FORM_BF_DCMX_vrb_eop_rc:
			{
					x_form_bf_dcmx_vrb_eop_rc *ptr;
					ptr = (x_form_bf_dcmx_vrb_eop_rc *)instr;

					cptr->dc_instr[j].tgt_val = ptr->bf;
					cptr->dc_instr[j].op_val[0] = ptr->dcmx;
					cptr->dc_instr[j].op_val[1] = ptr->vrb;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->bf);
					if ((ins_tuple->op2_dtype == SCALAR_DP) || (ins_tuple->op2_dtype == SCALAR_SP)) {
						sprintf(mnemonic, "%s,x%d", mnemonic, ptr->vrb);
					} else {
						sprintf(mnemonic, "%s,v%d", mnemonic, ptr->vrb);
					}
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->dcmx);
					break;
			}

			case XFX_FORM_RT_FXM_EOP_RC:
			{
					xfx_form_rt_fxm_eop_rc *ptr;
                    ptr = (xfx_form_rt_fxm_eop_rc *)instr;

                    cptr->dc_instr[j].tgt_val = ptr->rt;

                    sprintf(mnemonic, "%s r%d", ins_tuple->ins_name, ptr->rt);

                    break;
			}

			case I_FORM_LI_AA_LK:
			{
                    i_form_li_aa_lk *ptr;
                    ptr = (i_form_li_aa_lk *)instr;
                    if ((ptr->li) & 0xff0000) {
                   		if (strcmp("b", ins_tuple->ins_name) == 0) {
                      		sprintf(mnemonic, "%s$-0x%x ", ins_tuple->ins_name, (-((ptr->li)*4) & 0x00000fff));
							cptr->dc_instr[j].tgt_val = (-((ptr->li)*4) & 0x00000fff); 
						} else { /* bl */
                      		sprintf(mnemonic, "%s$-0x%x ", ins_tuple->ins_name, ((-((ptr->li)*4)) & 0x00ffffff));	
							cptr->dc_instr[j].tgt_val = (-((ptr->li)*4) & 0x00ffffff); 
						}
                    } else {
                      	sprintf(mnemonic, "%s$+0x%x ", ins_tuple->ins_name,((ptr->li)*4));
						cptr->dc_instr[j].tgt_val = (-((ptr->li)*4) & 0x00ffffff); 
                    }
                    break;
			}

			default:
					DPRINT(hlog, "Case default: Offset: 0x%x, Machine code: 0x%x, does not match to any form type!!!\n", cptr->instr_index[i], *instr);
					instr++;
		}
		#ifdef DBG_INSTR
		fprintf(instr_fp, "table index: %d, addr: %p, mcode: 0x%x, mnemonic: %s\n", j, instr, *instr, mnemonic);
		fflush(instr_fp);
		#endif

		/* target data type */
		if ((ins_tuple->tgt_dtype == DUMMY) || (ins_tuple->tgt_dtype == CR_T) || ((ins_tuple->tgt_dtype >= IMM_DATA) && (ins_tuple->tgt_dtype <= IMM_DATA_14BIT))) {
			cptr->dc_instr[j].tgt_dtype = OTHER_DTYPE; 
		}
		else if (((ins_tuple->tgt_dtype  >= BFP_SP) && (ins_tuple->tgt_dtype <= DFP_QUAD)) || (ins_tuple->tgt_dtype == BFP_QP)) {
			cptr->dc_instr[j].tgt_dtype = VSR_DTYPE; 
		}
		else {
			cptr->dc_instr[j].tgt_dtype = GPR_DTYPE;
		}

		/* src1 data type */
		if ((ins_tuple->op1_dtype == DUMMY) || (ins_tuple->op1_dtype == CR_T) || ((ins_tuple->op1_dtype >= IMM_DATA) && (ins_tuple->op1_dtype <= IMM_DATA_14BIT))) {
			cptr->dc_instr[j].op_dtype[0] = OTHER_DTYPE; 
		}
		else if (((ins_tuple->op1_dtype  >= BFP_SP) && (ins_tuple->op1_dtype <= DFP_QUAD)) || (ins_tuple->op1_dtype == BFP_QP)) {
			cptr->dc_instr[j].op_dtype[0] = VSR_DTYPE; 
		}
		else {
			cptr->dc_instr[j].op_dtype[0] = GPR_DTYPE;
		}
		/* src2 data type */
		if ((ins_tuple->op2_dtype == DUMMY) || (ins_tuple->op2_dtype == CR_T) || ((ins_tuple->op2_dtype >= IMM_DATA) && (ins_tuple->op2_dtype <= IMM_DATA_14BIT))) {
			cptr->dc_instr[j].op_dtype[1] = OTHER_DTYPE; 
		}
		else if (((ins_tuple->op2_dtype  >= BFP_SP) && (ins_tuple->op2_dtype <= DFP_QUAD)) || (ins_tuple->op2_dtype == BFP_QP)) {
			cptr->dc_instr[j].op_dtype[1] = VSR_DTYPE; 
		}
		else {
			cptr->dc_instr[j].op_dtype[1] = GPR_DTYPE;
		}
		/* src3 data type */
		if ((ins_tuple->op3_dtype == DUMMY) || (ins_tuple->op3_dtype == CR_T) || ((ins_tuple->op3_dtype >= IMM_DATA) && (ins_tuple->op3_dtype <= IMM_DATA_14BIT))) {
			cptr->dc_instr[j].op_dtype[2] = OTHER_DTYPE; 
		}
		else if (((ins_tuple->op3_dtype >= BFP_SP) && (ins_tuple->op3_dtype <= DFP_QUAD)) || (ins_tuple->op3_dtype == BFP_QP)) {
			cptr->dc_instr[j].op_dtype[2] = VSR_DTYPE; 
		}
		else {
			cptr->dc_instr[j].op_dtype[2] = GPR_DTYPE;
		}

		if (strncmp("st", mnemonic, 2) == 0) {
			cptr->dc_instr[j].instr_type = INSTR_TYPE_STORE;
		} else if (strncmp("l", mnemonic, 1) == 0) {
			cptr->dc_instr[j].instr_type = INSTR_TYPE_LOAD;
		} else {
			cptr->dc_instr[j].instr_type = INSTR_TYPE_OTHER;
		}
		strncpy(cptr->dc_instr[j].mnemonic, mnemonic, strlen(mnemonic));
		instr++;
	}
	#ifdef DBG_INSTR
	fclose(instr_fp);
	#endif
}


/* 
 * traverse and delete instruction tree 
 */
void delete_reg_use_list(struct reguse *reg_node)
{
	int i;
	if (reg_node == NULL) {
		return;
	}
	for (i = 0; i < MAX_SRC_OPRS; i++) {
		if (reg_node->src[i] != NULL) {
			delete_reg_use_list(reg_node->src[i]);
		}
	}
	DPRINT(hlog, "freeing node: %p, reg_num: %d, index: %d\n", reg_node, reg_node->tgt_reg, reg_node->dc_index);
	free(reg_node);
}

/*
 * find instruction at or before miscomparing offset
 * and pass to create dependent instructions call
 * for further analysis
 */
struct reguse *prepare_ls_call_list(FILE *dump, int cno, int cmp_offset) 
{
	uint16 tgt_type;
	int i, k, tc_index = -1, index_found = -1;
	client_data *cptr = global_sdata[INITIAL_BUF].cdata_ptr[cno];

	/* find the nearest matching offset if miscomparing offset is not directly used by load/store instructions */ 

 	for (i = 0; (i < cptr->num_ins_built) && (tc_index == -1); i++) { 
		if ((cptr->dc_instr[i].instr_type == INSTR_TYPE_LOAD) || (cptr->dc_instr[i].instr_type == INSTR_TYPE_STORE)) {
  			if (cptr->dc_instr[i].ea_offset == cmp_offset) { 
				/* exact match */
				tc_index = i;
				break;
			}
			else if (cptr->dc_instr[i].ea_offset > cmp_offset) {
				 for (k = i - 1; k >= 0; k--) {
                 	if (cptr->dc_instr[k].ea_offset != 0) {
                    	tc_index = k;
                        break;
                    }
                 }
                 break;
			}
			else {
				continue;
			}
		}
	}

	if (tc_index == -1) {
		DPRINT(hlog, "In %s, line %d: Error:- offset 0x%x, not found !!!\n", __FUNCTION__, __LINE__, cmp_offset);
		return (NULL);
	}
	else {
		/* index found for miscomparing offset for load/store instruction */
		index_found = tc_index; /* will recalculate in case of store instruction */

		DPRINT(hlog, "miscomparing offset: 0x%x, offset found[%d]: 0x%x, tgt type, %d, tgt value: %d, instr: %s\n", cmp_offset, tc_index, cptr->dc_instr[tc_index].ea_offset, cptr->dc_instr[tc_index].tgt_dtype,  cptr->dc_instr[tc_index].tgt_val, cptr->dc_instr[tc_index].mnemonic);

		if (cptr->dc_instr[tc_index].instr_type == INSTR_TYPE_STORE) {
			/* store does not directly modify register, find the instruction which modified this register 
 			 * just prior to store instruction otherwise return
 			 */
			index_found = -1;
			fprintf(dump, "%d: %s\n", tc_index + 1, cptr->dc_instr[tc_index].mnemonic);
			/* find index of target register which modified and that should not be a store type */
			for (i = tc_index - 1; i > 0; i--) {
				tgt_type = cptr->dc_instr[i].tgt_dtype;
	    		if (tgt_type == OTHER_DTYPE) {
	        		/* target type is not a register type */
	        		continue;
	    		}
	    		if ((cptr->dc_instr[i].tgt_val == cptr->dc_instr[tc_index].tgt_val) && (tgt_type == cptr->dc_instr[tc_index].tgt_dtype)) {
	        		index_found = i;
	        		break;
				}
			}
	
			if (index_found == -1) {
				DPRINT(hlog, "In %s, line %d: could not find index for target register: %d !!!\n", __FUNCTION__, __LINE__,  cptr->dc_instr[tc_index].tgt_val);
	    		return NULL;
			}
		}
		DPRINT(hlog, "index found: %d, instr: %s\n", index_found, cptr->dc_instr[index_found].mnemonic);
		/* create instruction lists */
		return (create_instr_tree(dump, cno, cptr->dc_instr[index_found].tgt_val, cptr->dc_instr[index_found].tgt_dtype, index_found));
	}
}

/*
 * Analyse register miscompare and build tree for dependent instructions causing miscompare
 * input arguments:- 
 * cno: client number
 * reg_num:  miscomparing register number
 * reg_type: gpr, vsr, fpscr, vscr, cr
 * start_index: start looking for instruction backward in instruction execution queue
 * output argument:- 
 * reg_node: intermediate nodes being created during the dependency search
 */
struct reguse *create_instr_tree(FILE *dump, int cno, int reg_num, int tgt_dtype, int start_index) 
{
	int i, index_found = -1;
	uint16 tgt_type, op_type;
	struct reguse *reg_node = NULL;
	client_data *cptr = global_sdata[INITIAL_BUF].cdata_ptr[cno];
	
	DPRINT(hlog,"Entry: %s, client# %d, reg num: %d, tgt_dtype: %d, start index: %d, instruction: %s\n", __FUNCTION__, cno, reg_num, tgt_dtype, start_index, cptr->dc_instr[start_index].mnemonic);
	
	
	if (start_index == -1) {
		/* start from last instruction */
	    start_index = cptr->num_ins_built - 1;
	}
	else if (start_index == 0) {
	    /* search end */
		DPRINT(hlog, "Exit: %s, reach the top of instruction queue\n", __FUNCTION__);
	    return NULL;
	}


	/* find first index for reg_num number in the decoded instruction list where it has been used as target */
	
	for (i = start_index; i > 0; i--) {
		tgt_type = cptr->dc_instr[i].tgt_dtype;
		if ((tgt_type == OTHER_DTYPE) || (cptr->dc_instr[i].instr_type == INSTR_TYPE_STORE) || cptr->dc_instr[i].marked) {
	        continue;
		}
	    if ((cptr->dc_instr[i].tgt_val == reg_num) && (tgt_type == tgt_dtype)) {
			reg_node = (struct reguse *)malloc(sizeof(struct reguse));
	        if (reg_node == NULL) {
				char err_msg[256];
				sprintf(err_msg, "Error allocating memory for node, %s, %d, %s\n", __FILE__, __LINE__, strerror(errno));
				hxfmsg(&hd, errno, HTX_HE_SOFT_ERROR, err_msg);
	            return NULL;
 	        }
	        reg_node->src[0] = reg_node->src[1] = reg_node->src[2] = NULL;
	        reg_node->tgt_reg = reg_num;
			reg_node->dc_index = i;
	        index_found = i;
            DPRINT(hlog, "%s: index found: %d, client# %d, reg num: %d, instr: %s\n", __FUNCTION__, index_found, cno, reg_num, cptr->dc_instr[index_found].mnemonic);
	        DPRINT(hlog, "%s: Node created: Addr: %p, tgt_reg: %d, dc index: %d\n", __FUNCTION__, reg_node, reg_node->tgt_reg, reg_node->dc_index);
			fprintf(dump, "%d: %s\n", index_found + 1, cptr->dc_instr[index_found].mnemonic);
			cptr->dc_instr[index_found].marked = 1;
	        break;
		}
	}
	
	if (index_found == -1) {
		DPRINT(hlog, "Exit: %s, index_found: %d\n", __FUNCTION__, index_found);
	    return NULL;
	}
	else {
        /* source operand */
        /* check backward for all src operands and reiterate node creation */
        for (i = 0; i < MAX_SRC_OPRS; i++) {
            op_type = cptr->dc_instr[index_found].op_dtype[i];
            if (op_type == OTHER_DTYPE) { 
                /* src operand type is not a register type */
                continue;
            }
			if ((cptr->dc_instr[index_found].instr_type == INSTR_TYPE_LOAD) && (op_type == GPR_DTYPE)) {
				continue;
			}

            reg_node->src[i] = create_instr_tree(dump, cno, cptr->dc_instr[index_found].op_val[i], op_type, index_found - 1);
        }
    }
    DPRINT(hlog, "Exit: %s\n", __FUNCTION__);
    return reg_node;
}
