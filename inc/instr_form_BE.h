/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

#ifndef _htx_instruction_forms
#define _htx_instruction_forms

typedef struct
{
	uint32 opcode:6;
	uint32 rt:5;
	uint32 ra:5;
	uint32 d:16;
}d_form_rt_ra_d;

typedef struct
{
	uint32 opcode:6;
	uint32 rt:5;
	uint32 ra:5;
	uint32 rb:5;
	uint32 eop:10;
	uint32 cr:1;
}x_form_rt_ra_rb_eop_rc;

typedef struct
{
	uint32 opcode:6;
	uint32 rt:5;
	uint32 ra:5;
	uint32 rb:5;
	uint32 eop:10;
	uint32 eh:1;
}x_form_rt_ra_rb_eop_eh;

typedef struct
{
	uint32 opcode:6;
	uint32 bf:3;
	uint32 dc:2;	/*dont care*/
	uint32 ra:5;
	uint32 rb:5;
	uint32 eop:10;
	uint32 cr:1;
}x_form_bf_ra_rb_eop_rc;

typedef struct
{
	uint32 opcode:6;
	uint32 bf:3;
	uint32 dc1:2;
	uint32 bfa:3;
	uint32 dc2:2;
	uint32 rb:5;
	uint32 eop:10;
	uint32 cr:1;
}x_form_bf_bfa_eop_rc;

typedef struct
{
	uint32 opcode:6;
	uint32 bf:3;
	uint32 dc1:6;
	uint32 w:1;
	uint32 u:4;
	uint32 dc2:1;
	uint32 eop:10;
	uint32 cr:1;
}x_form_bf_w_u_eop_rc;

typedef struct
{
      unsigned opcode   	:6;
      unsigned rt     	 	:5;
      unsigned s        	:1;
      unsigned sp       	:1;
      unsigned dc	     	:3;
      unsigned rb       	:5;
      unsigned xopcode  	:10;
      unsigned Rc       	:1;
}x_form_rt_s_sp_rb_eop_rc;
typedef struct
{
	unsigned opcode			:6;
	unsigned				:3;
	unsigned l				:2;
    unsigned                :1;
	unsigned e				:4;
    unsigned                :5;
	unsigned eop            :10;
    unsigned                :1;
}x_form_l_e;

typedef struct
{
	unsigned opcode		:6;
	unsigned		:15;
	unsigned eop            :10;
        unsigned                :1;
}xl_form;

typedef struct
{
	uint32 opcode:6;
	uint32 l:1;
	uint32 flm:8;
	uint32 w:1;
	uint32 rb:5;
	uint32 eop:10;
	uint32 cr:1;
}xfl_form_l_flm_w_rb_eop_rc;

typedef struct
{
	uint32 opcode:6;
	uint32 rt:5;
	uint32 dc1:1;
	uint32 fxm:8;
	uint32 dc2:1;
	uint32 eop:10;
	uint32 cr:1;
}xfx_form_rt_fxm_eop_rc;

typedef struct
{
	uint32 opcode:6;
	uint32 rt:5;
	uint32 ra:5;
	uint32 rb:5;
	uint32 rc:5;
	uint32 eop:5;
	uint32 cr:1;
}a_form_rt_ra_rb_rc_eop_rc;

typedef struct
{
	uint32 opcode:6;
	uint32 rs:5;
	uint32 ra:5;
	uint32 rb:5;
	uint32 rc:5;
	uint32 eop:6;
}va_form_rs_ra_rb_rc_eop;

typedef struct
{
     unsigned opcode     	:6;
     unsigned rt        	:5;
     unsigned ra         	:5;
     unsigned sh         	:6;
     unsigned xopcode    	:9;
     unsigned Rc         	:1;
}z_form_rt_ra_sh_eop_rc;

typedef struct
{

       unsigned opcode   	:6;
       unsigned bf       	:3;
       unsigned dc	     	:2;
       unsigned ra       	:5;
       unsigned dm       	:6; /* data class/group mask  */
       unsigned xopcode  	:9;
       unsigned rsvd1    	:1;
}z_form_bf_ra_dm_eop_rc;

typedef struct
{
       unsigned opcode     	:6;
       unsigned rt         	:5;
       unsigned ra_imm     	:5;
       unsigned rb         	:5;
       unsigned rm         	:2;
       unsigned xopcode    	:8;
       unsigned Rc         	:1;
}z_form_rt_d_rb_rmc_eop_rc;

typedef struct
{
       unsigned opcode     	:6;
       unsigned rt         	:5;
       unsigned dc       	:4;
       unsigned r          	:1;
       unsigned rb         	:5;
       unsigned rm         	:2;
       unsigned xopcode    	:8;
       unsigned Rc         	:1;
}z_form_rt_r_rb_rmc_eop_rc;

typedef struct
{
	uint32 opcode:6 ;
	uint32 source:5 ;
	uint32 target:5 ;
	uint32 unused:5 ;
	uint32 ext_opcode :10 ;
	uint32 sx:1;
}Form_XX1_RA_XS;

typedef struct
{
	uint32 opcode:6 ;
	uint32 target:5 ;
	uint32 source:5 ;
	uint32 unused:5 ;
	uint32 ext_opcode :10 ;
	uint32 tx:1;
}Form_XX1_XT_RA;

typedef struct
{
	uint32 opcode:6 ;
	uint32 target:5 ;
	uint32 src1  :5 ;
	uint32 src2  :5 ;
	uint32 ext_opcode :10 ;
	uint32 TX:1;
}Form_XX1_XT_RA_RB;

typedef struct
{
	uint32 opcode:6 ;
	uint32 target:5 ;
	uint32 unused :3 ;
	uint32 uim :2 ;
	uint32 src :5 ;
	uint32 ext_opcode :9 ;
	uint32 BX:1 ;
	uint32 TX:1 ;
}Form_XX2_XT_XB;


typedef struct
{
	uint32 opcode:6 ;
	uint32 BF:3 ;
	uint32 unused:7;
	uint32 src:5 ;
    uint32 ext_opcode:9 ;
	uint32 BX :1;
	uint32 NA :1;
}Form_XX2_BF_XB;

typedef struct
{
	uint32 opcode:6 ;
	uint32 target:5 ;
	uint32 unused :2 ;
	uint32 uim:3 ;
	uint32 src :5 ;
	uint32 ext_opcode :9 ;
	uint32 BX:1 ;
	uint32 TX:1 ;
}Form_XX2_XT_UIM_XB;
typedef struct
{
	uint32 opcode:6 ;
	uint32 target:5 ;
	uint32 src1  :5 ;
	uint32 src2  :5 ;
	uint32 ext_opcode :8 ;
	uint32 AX : 1 ;
	uint32 BX :1 ;
	uint32 TX :1 ;
}Form_XX3_XT_XA_XB;

typedef struct
{
	uint32 opcode:6 ;
	uint32 target:5 ;
	uint32 src1  :5 ;
	uint32 src2  :5 ;
	uint32 rc    :1 ;
	uint32 ext_opcdoe :7 ;
	uint32 AX :1 ;
	uint32 BX :1 ;
	uint32 TX :1 ;
}Form_XX3_RC_XT_XA_XB;

typedef struct
{
	uint32 opcode:6 ;
	uint32 BF:3 ;
	uint32 unused:2;
	uint32 src1:5 ;
	uint32 src2:5 ;
    uint32 ext_opcode:8 ;
	uint32 AX :1;
	uint32 BX :1;
	uint32 NA :1;
}Form_XX3_BF_XA_XB;

typedef struct
{
	uint32 opcode :6 ;
	uint32 target :5 ;
	uint32 src1   :5 ;
	uint32 src2   :5 ;
	uint32 zero   :1 ;
	uint32 SHW_DM    :2 ;
	uint32 ext_opcode :5 ;
	uint32 AX     :1 ;
	uint32 BX     :1 ;
	uint32 TX     :1 ;
}Form_XX3_XT_XA_XB_2Bit;

typedef struct
{
	uint32 opcode : 6;
	uint32 target : 5 ;
	uint32 src1   : 5 ;
	uint32 src2   : 5 ;
	uint32 src3   : 5 ;
	uint32 ext_opcode : 2 ;
	uint32 CX	:1 ;
	uint32 AX	:1 ;
	uint32 BX	:1 ;
	uint32 TX	:1 ;
}Form_XX4_XT_XA_XB_XC;

typedef struct
{
      uint32  opcode    :6;
      uint32  vt        :5;
      uint32  va        :5;
      uint32  vb        :5;
      uint32  xopcode   :11;

}Form_VT_VA_VB;

typedef struct
{
      uint32  opcode    :6;
      uint32  vt        :5;
      uint32  va        :5;
      uint32  vb        :5;
      uint32  vc        :5;
      uint32  xopcode   :6;
}Form_VT_VA_VB_VC;

typedef struct
{
      uint32  opcode    :6;
      uint32  vt        :5;
      uint32  va        :5;
      uint32  st        :1;
      uint32  six       :4;
      uint32  xopcode   :11;
}Form_VT_VA_ST_SIX;

typedef struct
{
	uint32 opcode   :6;
	uint32 vt       :5;
	uint32 va       :5;
	uint32 vb       :5;
	uint32 cnst     :1;
	uint32 ps       :1;
	uint32 xopcode  :9;
}Form_VT_VA_VB_PS_EOP;

typedef struct
{
      uint32  opcode    :6;
      uint32  bt        :5;
      uint32  ba        :5;
      uint32  bb        :5;
      uint32  xopcode   :10;
      uint32  rc       	:1;
}x_form_bt_ba_bb;


typedef struct
{
      uint32  opcode    :6;
      uint32  rt        :5;
      uint32  ra        :5;
      uint32  rb        :5;
      uint32  OE	:1;
      uint32  xopcode   :9;
      uint32  rc       	:1;
}x_form_rt_ra_rb_oe_eop_rc;

typedef struct
{
      uint32  opcode    :6;
      uint32  rt        :5;
      uint32  ra        :5;
      uint32  uim       :5;
      uint32  OE	:1;
      uint32  xopcode   :9;
      uint32  rc       	:1;
}x_form_rt_ra_oe_eop_rc;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  uim       :16;
}d_form_rs_ra_uim;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  rb        :5;
      uint32  xopcode   :10;
      uint32  rc	:1;
}x_form_rs_ra_rb_eop_rc;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  nu        :5;
      uint32  xopcode   :10;
      uint32  rc	:1;
}x_form_rs_ra_eop_rc;
typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  nu        :5;
      uint32  xopcode   :10;
      uint32  nu1	:1;
}x_form_rs_ra_eop;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  sh1       :5;
      uint32  mb1	:5;
      uint32  mb2	:1;
      uint32  xopcode	:3;
      uint32  sh2	:1;
      uint32  rc	:1;
}mdform_rs_ra_sh_mb_rc;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  sh	:5;
      uint32  mb	:5;
      uint32  me 	:5;
      uint32  rc	:1;
}mform_rs_ra_sh_mb_me_rc;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  rb	:5;
      uint32  mb1	:5;
      uint32  mb2	:1;
      uint32  xopcode	:4;
      uint32  rc	:1;
}mdsform_rs_ra_rb_mb_eop_rc;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  rb	:5;
      uint32  me1	:5;
      uint32  me2	:1;
      uint32  xopcode	:4;
      uint32  rc	:1;
}mdsform_rs_ra_rb_me_eop_rc;
typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  rb	:5;
      uint32  me	:5;
      uint32  mb	:5;
      uint32  rc	:1;
}mform_rs_ra_rb_mb_me_rc;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  sh1       :5;
      uint32  xopcode	:9;
      uint32  sh2	:1;
      uint32  rc	:1;
}xs_form_rs_ra_sh_eop_rc;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  ra        :5;
      uint32  sh        :5;
      uint32  xopcode	:10;
      uint32  rc	:1;
}x_form_rs_ra_sh_eop_rc;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  sprL      :5;
      uint32  sprU	:5;
      uint32  xopcode	:10;
      uint32  		:1;
}x_fx_form_rs_spr_eop;

typedef struct
{
      uint32  opcode    :6;
      uint32  rt        :5;
      uint32  sprL      :5;
      uint32  sprU	:5;
      uint32  xopcode	:10;
      uint32  		:1;
}x_fx_form_rt_spr_eop;

typedef struct
{
      uint32  opcode    :6;
      uint32  rs        :5;
      uint32  	        :1;
      uint32  FXM	:8;
      uint32  xopcode	:10;
      uint32  		:1;
}x_fx_form_rs_fxm_eop;

typedef struct
{
	uint32 opcode	:6;
	uint32 rs	:5;
	uint32 rb	:5;
	uint32 ra	:5;
	uint32 eop	:10;
	uint32 		:1;
}x_form_rs_ra_rb_eop;

typedef struct
{
	uint32 opcode	:6;
	uint32 rs	:5;
	uint32 ra	:5;
	uint32 uim	:16;
}d_form_rs_ra_d;

typedef struct
{
	uint32 opcode	:6;
	uint32 rs	:5;
	uint32 ra	:5;
	uint32 uim	:16;
}d_form_rs_ra_ds;

typedef struct
{
	uint32 opcode	:6;
	uint32 rt	:5;
	uint32 ra	:5;
	uint32 uim	:16;
}d_form_rt_ra_si;

typedef struct
{
	uint32 opcode	:6;
	uint32 rs	:5;
	uint32 ra	:5;
	uint32 uim	:5;
        uint32 eop      :10;
	uint32          :1;
}x_form_rs_ra_nb_eop;

typedef struct
{
	uint32 opcode	:6;
	uint32 cr_t	:3;
	uint32 eop	:1;
	uint32 l	:1;
        uint32 ra       :5;
	uint32 uim      :16;
}d_form_bf_l_ra_si;

typedef struct
{
	uint32 opcode	:6;
	uint32 cr_t	:3;
	uint32  	:1;
	uint32 l	:1;
        uint32 ra       :5;
	uint32 rb       :5;
	uint32 eop      :10;
}x_form_bf_l_ra_rb;

typedef struct
{
	uint32 opcode	:6;
	uint32     	:5;
	uint32 ra	:5;
	uint32 rb	:5;
        uint32 eop      :9;
        uint32          :1;
}x_form_ra_rb_eop;

typedef struct
{
	uint32 opcode	:6;
	uint32     	:3;
	uint32 l	:2;
	uint32 ra	:5;
	uint32 rb	:5;
        uint32 eop      :9;
        uint32          :1;
}x_form_l_ra_rb_eop;
typedef struct
{
	uint32 opcode	:6;
	uint32 th    	:5;
	uint32 ra	:5;
	uint32 rb	:5;
        uint32 eop      :10;
        uint32          :1;
}x_form_th_ra_rb_eop;

typedef struct
{
	uint32 opcode	:6;
	uint32 li    	:24;
	uint32 aa	:1;
	uint32 lk	:1;
}i_form_li_aa_lk;

typedef struct
{
	uint32 opcode	:6;
	uint32 bo    	:5;
	uint32 bi    	:5;
	uint32 bd    	:14;
	uint32 aa	:1;
	uint32 lk	:1;
}b_form_bo_bi_bd_aa_lk;

typedef struct
{
	uint32 opcode	:6;
	uint32 bo    	:5;
	uint32 bi    	:5;
	uint32     	:3;
	uint32 bh	:2;
	uint32 aa	:10;
	uint32 lk	:1;
}xl_form_bo_bi_lk;
typedef struct
{
      uint32  opcode    :6;
      uint32  rt        :5;
      uint32  ra        :5;
      uint32  uim       :12;
      uint32  pt	:4;
}d_form_rt_ra_uim_pt;
typedef struct
{
      uint32  opcode    :6;
      uint32		:1;
      uint32  ct        :4;
      uint32  ra        :5;
      uint32  rb        :5;
      uint32  eop       :10;
      uint32  	 	:1;
}x_form_ct_ra_rb_eop;
typedef struct
{
      uint32  opcode    :6;
      uint32  rx        :5;
      uint32  ry        :5;
      uint32  rz        :5;
      uint32  xopcode   :10;
      uint32  rc	:1;
}x_form_rx_rx_rx_eop_rc;
typedef struct
{
      uint32  opcode    :6;
      uint32  rt        :5;
      uint32  d1        :5;
      uint32  d0        :10;
	  uint32  fx        :5;
      uint32  d2   		:1;
}dx_form_rt_d1_d0_d2;

typedef struct
{
	uint32  opcode  :6;
	uint32  rt      :5;
	uint32 BFA		:3;
	uint32 nu0		:2;
	uint32 nu1		:5;
	uint32 eop		:11;
}x_form_rt_bfa;

typedef struct
{
	uint32 opcode	:6;
	uint32 bf		:3;
	uint32 dcmx		:7;
	uint32 vrb		:5;
	uint32 xop		:10;
	uint32 rc		:1;
}x_form_bf_dcmx_vrb_eop_rc;

typedef struct
{
    uint32 opcode:6;
    uint32 target:5;
    uint32 dc:1;
    uint32 uim4:4;
    uint32 src:5;
    uint32 ext_op:9;
    uint32 BX:1;
    uint32 TX:1;
}Form_XX2_XT_UIM4_XB;

typedef struct
{
    uint32 opcode:6;
    uint32 T:5;
    uint32 dc:2;
    uint32 imm8:8;
    uint32 ext_op:10;
    uint32 TX:1;
}Form_XX1_IMM8;

typedef struct
{
    uint32 opcode:6;
    uint32 bf:3;
    uint32 dc:1;
    uint32 uim:6;
    uint32 frb:5;
    uint32 ext_op:10;
    uint32 nu:1;
}Form_X_BF_UIM_FB;

typedef struct
{
    uint32 opcode:6;
	uint32 T:5;
	uint32 dx:5;
	uint32 B:5;
	uint32 fxd1:4;
	uint32 dc:1;
	uint32 fxd2:3;
	uint32 dm:1;
	uint32 BX:1;
	uint32 TX:1;
}Form_XX2_dx_dc_dm;

typedef struct
{
    uint32 opcode:6;
    uint32 rt:5;
    uint32 ra:5;
    uint32 rb:5;
    uint32 cy:2;
    uint32 eop:8;
    uint32 nu:1;
}x_form_rt_ra_rb_cy_eop;

#endif
