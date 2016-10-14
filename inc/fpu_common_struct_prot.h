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

#ifndef _FPU_COMMON_H
#define _FPU_COMMON_H

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


#ifdef DEBUG
#define DPRINT(x, format, ...) fprintf(x, format, ##__VA_ARGS__)
#define FFLUSH(x) fflush(x)
#define mprintf(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DPRINT(x, format, ...)
#define FFLUSH(x)
#define mprintf(format, ...)
#endif

#define INLINE static
#define ASSERT(arg)

extern FILE *hlog;

/* TypeDef declarations */
typedef unsigned int        boolean;
typedef int                 bool;
typedef signed char         int8;
typedef signed short        int16;
typedef signed int          int32;
typedef unsigned char       uint8;
typedef unsigned short      uint16; 
typedef unsigned int        uint;
typedef unsigned int        uint32;
typedef unsigned long long  uint64; 
#ifdef __HTX_LINUX__
typedef long long           int64;
#endif
typedef long long           sint64;


/* The below #undefs are done because htxsyscfg64_new.h has */
/* macros define with these names. There are variables in */
/* existing code with same names for eg int B , which is making */
/* compiler look at the variables like macros and resulting in */
/* unnecessary errors */
#ifdef B
#undef B
#endif

#ifdef K
#undef K
#endif

#ifdef M
#undef M
#endif

#define NUM_VSRS		64
#define NUM_GPRS		32
#define NUM_FPRS 		32
#define MAX_INS_STREAM_DEPTH		(8192)
#define MAX_NUM_CPUS                8
#define WORD                     4

#define FPSCR_Value_type uint64
#define CAST(t,e) ((t)(e))
#define FPSCR_STICKY		CAST(FPSCR_Value_type,0xFFF80700)
#define FPS_EXCEPTIONS		CAST(FPSCR_Value_type,0x1FF80700)


#define FPS_BIT_FX         0
#define FPS_FX			CAST(FPSCR_Value_type,0x80000000)
#define FPS_FEX			CAST(FPSCR_Value_type,0x40000000)
#define FPS_VX			CAST(FPSCR_Value_type,0x20000000)
#define FPS_OX			CAST(FPSCR_Value_type,0x10000000)
#define FPS_UX			CAST(FPSCR_Value_type,0x08000000)
#define FPS_ZX			CAST(FPSCR_Value_type,0x04000000)
#define FPS_XX			CAST(FPSCR_Value_type,0x02000000)
#define FPS_VXSNAN		CAST(FPSCR_Value_type,0x01000000)
#define FPS_VXISI		CAST(FPSCR_Value_type,0x00800000)
#define FPS_VXIDI		CAST(FPSCR_Value_type,0x00400000)
#define FPS_VXZDZ		CAST(FPSCR_Value_type,0x00200000)
#define FPS_VXIMZ		CAST(FPSCR_Value_type,0x00100000)
#define FPS_VXVC		CAST(FPSCR_Value_type,0x00080000)
#define FPS_FR			CAST(FPSCR_Value_type,0x00040000)
#define FPS_FI			CAST(FPSCR_Value_type,0x00020000)
#define FPS_FPRF		CAST(FPSCR_Value_type,0x0001F000)
#define FPS_C			CAST(FPSCR_Value_type,0x00010000)
#define FPS_FPCC		CAST(FPSCR_Value_type,0x0000F000)
#define FPS_FL			CAST(FPSCR_Value_type,0x00008000)
#define FPS_FG			CAST(FPSCR_Value_type,0x00004000)
#define FPS_FE			CAST(FPSCR_Value_type,0x00002000)
#define FPS_FU			CAST(FPSCR_Value_type,0x00001000)
#define FPS_VXSOFT		CAST(FPSCR_Value_type,0x00000400)
#define FPS_VXSQRT		CAST(FPSCR_Value_type,0x00000200)
#define FPS_VXCVI		CAST(FPSCR_Value_type,0x00000100)



/* FPSCR masks for all the VSX instructions */
#define F_XSASDP FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_OX | FPS_UX | FPS_XX | FPS_VXSNAN | FPS_VXISI
#define F_XSDIVDP FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_OX | FPS_UX | FPS_ZX | FPS_XX | FPS_VXSNAN | FPS_VXIDI | FPS_VXZDZ
#define F_XSMULDP FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_OX | FPS_UX | FPS_XX | FPS_VXSNAN | FPS_VXIMZ
#define F_XSSQRTDP FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_XX | FPS_VXSNAN | FPS_VXSQRT
#define F_XVADDSUB FPS_FX | FPS_OX | FPS_UX | FPS_XX | FPS_VXSNAN | FPS_VXISI
#define F_XVMUL FPS_FX | FPS_OX | FPS_UX | FPS_XX | FPS_VXSNAN | FPS_VXIMZ
#define F_XVDIV FPS_FX | FPS_OX | FPS_UX | FPS_ZX | FPS_XX | FPS_VXSNAN | FPS_VXIDI | FPS_VXZDZ
#define F_XVSQRT FPS_FX | FPS_XX | FPS_VXSNAN | FPS_VXSQRT
#define F_XSMADDSUB FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_OX | FPS_UX | FPS_XX | FPS_VXSNAN | FPS_VXISI | FPS_VXIMZ
#define F_XVMADDSUB	FPS_FX | FPS_OX | FPS_UX | FPS_XX | FPS_VXSNAN | FPS_VXISI | FPS_VXIMZ
#define F_XSCMP FPS_FPCC | FPS_FX | FPS_VXSNAN | FPS_VXVC
#define F_XCMPEQ FPS_FX | FPS_VXSNAN
#define F_XCMPGE FPS_FX | FPS_VXSNAN | FPS_VXVC
#define F_XSCVDPSP FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_OX | FPS_UX | FPS_XX | FPS_VXSNAN
#define F_XSCVSPDP FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_VXSNAN
#define F_XVCVDPSP FPS_FX | FPS_OX | FPS_UX | FPS_XX | FPS_VXSNAN
#define F_XVCVSPDP FPS_FX | FPS_VXSNAN
#define F_XSCV FPS_FR | FPS_FI | FPS_FX | FPS_XX | FPS_VXSNAN | FPS_VXCVI | FPS_FPRF
#define F_XVCV FPS_FX | FPS_XX | FPS_VXSNAN | FPS_VXCVI
#define F_XSCVI2F FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_XX
#define F_XVCVI2F FPS_FX | FPS_XX
#define F_XSRD FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_VXSNAN
#define F_XVRDPI FPS_FX | FPS_VXSNAN
#define F_XVRDPIC FPS_FX | FPS_XX | FPS_VXSNAN
#define F_XSRDPIC FPS_FPRF | FPS_FR | FPS_FI | FPS_FX | FPS_XX | FPS_VXSNAN


union DoubleWord
{
    float   d_sgl[2];
    double  d_dbl;
    int8    d_ival8[8];
    uint8   d_uval8[8];
    int16   d_ival16[4];
    uint16  d_uval16[4];
    int32   d_ival32[2];
    uint32  d_uval32[2];
    sint64  d_ival64;
    uint64  d_uval64;
};
typedef union DoubleWord DoubleWord;

union QuadWord
{
    float       q_sgl[4];
    double      q_dbl[2];
    int8        q_ival8[16];
    uint8       q_uval8[16];
    int16       q_ival16[8];
    uint16      q_uval16[8];
    int32       q_ival32[4];
    uint32      q_uval32[4];
    sint64      q_ival64[2];
    uint64      q_uval64[2];
    DoubleWord  q_dw[2];
};
typedef union QuadWord QuadWord;

enum EXEStatus
{
    EXE_FAILURE,
    EXE_SUCCESS
};
typedef enum EXEStatus EXEStatus;

enum dfp_type {SHORT, LONG,EXTENDED};
enum dfp_class {ZEERO, INFINIT, qNaN, sNaN, TINY, NORM, SUBNORMAL,LARGE,SMALL};
enum action_type {DECODE, ENCODE};


enum dfp_type DFPType[MAX_NUM_CPUS];
enum dfp_class DFPClass[MAX_NUM_CPUS];
enum action_type ActionType[MAX_NUM_CPUS];

typedef struct testcase testcase;
typedef struct tc_context tc_context;

/* Function pointer declaration */
typedef void (*sim_fptr)(unsigned int*,struct testcase*);
typedef void (*db_fptr)(int, char *, int);
typedef int (*copy_fptr)(int);
typedef uint32 (*store_macro_fptr)(uint32);
typedef QuadWord 			VRegValue;
typedef unsigned long       RoundingMode_t;
typedef uint32				FP_EXCEPTION_T;
typedef uint64              FPRegValue;
typedef uint32 		SingleWord;

struct testcase {
    struct tc_context {
        union {
            uint32 vsrs[NUM_VSRS*4];    /* [NUM_VSRS * 4] 32 bit values */
            uint64 fprs[NUM_VSRS][2];
        };
        uint64 gprs[NUM_GPRS];          /* [NUM_GPRS] 64 bit values */
        struct sprs_context {
            uint64 fpscr;               /* ptr to FPSCR */
            uint64 cr;
            uint64 ctr;
            uint64 lr;
            uint64 iar;
            uint64 xer;
            /* uint64 vscr_dummy; */
            uint64 vscr[2];
            uint64 msr;
            uint64 comp_fpscr;
            uint64 vmx_alignment_dummy;
        }sprs;
    }tcc;
    uint64 context_gprs[NUM_GPRS];
    uint32 compare_status;                  /* boolean var to indicate if compare was good or bad */
    uint32 tc_ins[MAX_INS_STREAM_DEPTH];    /* testcase instructions for this client */
    sim_fptr sim_ptr[MAX_INS_STREAM_DEPTH]; /* ptr holds instruction mcode n corresponding sim routine fptr tuple */
    uint32 ea_off[MAX_INS_STREAM_DEPTH];    /* Keeps EA off used by ins from load/store base */
    uint32 ea_initbase_off[MAX_INS_STREAM_DEPTH];    /* Keeps EA off used by ins from initial load/store base */
#ifndef SCTU
    char ls_ptr[MAX_INS_STREAM_DEPTH*16];   /* ptr to hold load memory area for this client */
#endif
};

typedef enum
{
	D_FORM_RT_RA_D,
	X_FORM_RT_RA_RB_eop_rc,
	X_FORM_RT_RA_RB_eop_EH,
	X_FORM_BF_RA_RB_eop_rc,
	X_FORM_BF_BFA_eop_rc,
	X_FORM_BF_W_U_eop_rc,
	X_FORM_RT_S_SP_RB_eop_rc,
	XFL_FORM_L_FLM_W_RB_eop_rc,
	A_FORM_RT_RA_RB_RC_eop_rc,
	Z_FORM_RT_RA_SH_eop_rc,
	Z_FORM_BF_RA_DM_eop_rc,
	Z_FORM_RT_D_RB_RMC_eop_rc,
	Z_FORM_RT_R_RB_RMC_eop_rc,
	FORM_XX1_XT_RA_RB,
	FORM_XX1_RA_XS,
	FORM_XX1_XT_RA,
	FORM_XX2_XT_XB,
	FORM_XX2_BF_XB,
	FORM_XX2_XT_UIM_XB,
	FORM_XX3_XT_XA_XB,
	FORM_XX3_RC_XT_XA_XB,
	FORM_XX3_BF_XA_XB,
	FORM_XX3_XT_XA_XB_2Bit,
	FORM_XX4_XT_XA_XB_XC,
	FORM_VT_VA_VB,
	FORM_VT_VA_VB_VC,
	FORM_VT_VA_VB_RC,
	FORM_VT_VA_ST_SIX,
	X_FORM_BT_BA_BB_eop_rc,
	X_FORM_RT_RA_RB_OE_eop_rc,
	X_FORM_RT_RA_OE_eop_rc,
	D_FORM_RS_RA_UIM,
	X_FORM_RS_RA_RB_eop_rc,
	X_FORM_RS_RA_eop_rc,
	X_FORM_RS_RA_eop,
	MDFORM_RS_RA_SH_MB_rc,
	MFORM_RS_RA_SH_MB_ME_rc,
	MDSFORM_RS_RA_RB_MB_eop_rc,
	MDSFORM_RS_RA_RB_ME_eop_rc,
	MFORM_RS_RA_RB_MB_ME_rc,
	XS_FORM_RS_RA_SH_eop_rc,
	X_FORM_RS_RA_SH_eop_rc,
	X_FX_FORM_RS_SPR_eop,
	X_FX_FORM_RT_SPR_eop,
	X_FX_FORM_RS_FXM_eop,
	X_FORM_RS_RA_RB_eop,
	D_FORM_RS_RA_D,
	D_FORM_RS_RA_DS,
	D_FORM_RT_RA_SI,
	XL_FORM,
	X_FORM_L_E,
	X_FORM_RS_RA_NB_eop,
	D_FORM_BF_L_RA_SI,
	X_FORM_BF_L_RA_RB,
	X_FORM_RA_RB,
	X_FORM_L_RA_RB,
	X_FORM_TH_RA_RB,
	I_FORM_LI_AA_LK,
	B_FORM_BO_BI_BD_AA_LK,
	XL_FORM_BO_BI_LK,
	D_FORM_RT_RA_DQ_PT,
	X_FORM_CT_RA_RB_eop,
	X_FORM_Rx_Rx_Rx_eop_rc,
	X_FORM_RS_RA_FC_eop,
	DX_FORM_RT_D1_D0_D2,
	X_FORM_RT_BFA,
	X_FORM_BF_DCMX_vrb_eop_rc,
	FORM_XX2_XT_UIM4_XB,
	FORM_XX1_IMM8,
	FORM_X_BF_UIM_FB,
	FORM_XX2_DX_DC_DM,
	XFX_FORM_RT_FXM_EOP_RC,
	X_FORM_RT_RA_RB_CY_eop,
	VA_FORM_RS_RA_RB_RC_eop
}instr_form;

/*
 * Data structure to represent instruction. It contains instruction related masks, operands details etc.
 */
struct instruction_masks {
	uint32 	op_eop_mask;	/* Mask for opcode and extended opcode */
	uint32 	fpscr_mask;		/* Mask for FPSCR bits */
	uint32 	op1_dtype;		/* Data type of operand 1 */
	uint32 	op1_pos;		/* op1 bit position in 32 bit instruction word */
	uint32 	op2_dtype;		/* Data type of operand 2 */
	uint32 	op2_pos;		/* op2 bit position in 32 bit instruction word */
	uint32 	op3_dtype;		/* Data type of operand 3 */
	uint32 	op3_pos;		/* op3 bit position in 32 bit instruction word */
	uint32 	tgt_dtype;		/* Data type of target */
	uint32 	tgt_pos;		/* target bit position in 32 bit instruction word */
	uint64 	ins_class;		/* Mask for identifying class of instructions */
	char	ins_name[16];	/* Instruction mnemonics */
	sim_fptr sim_func;		/* pointer to this instructions's simulation function. All set to NULL for now */
	uint64	ins_cat_mask;	/* All ins categories this ins belong to */
	instr_form insfrm;      /* for instruction dis-assembly */
};
typedef struct instruction_masks instruction_masks;

/* BFP prototypes */

void simulate_fabs(uint32 *machine_code , struct testcase *test_case);
void simulate_fmr(uint32 *machine_code, struct testcase *test_case);
void simulate_fmrgew(uint32 *machine_code, struct testcase *test_case);
void simulate_fmrgow(uint32 *machine_code, struct testcase *test_case);
void simulate_fnabs(uint32 *machine_code, struct testcase *test_case);
void simulate_fneg(uint32 *machine_code, struct testcase *test_case);
void simulate_fcpsgn(uint32 *machine_code, struct testcase *test_case);
void simulate_fadd(uint32 *machine_code, struct testcase *test_case);
void simulate_fadd_dot(uint32 *machine_code, struct testcase *test_case);
void simulate_fsub(uint32 *machine_code, struct testcase *test_case);
void simulate_fsub_dot(uint32 *machine_code, struct testcase *test_case);
void simulate_fmul(uint32 *machine_code, struct testcase *test_case);
void simulate_fmul_dot(uint32 *machine_code, struct testcase *test_case);
void simulate_fdiv(uint32 *machine_code, struct testcase *test_case);
void simulate_fdiv_dot(uint32 *machine_code, struct testcase *test_case);
void simulate_fsqrt(uint32 *machine_code, struct testcase *test_case);
void simulate_fsqrt_dot(uint32* machine_code, struct testcase *test_case);
void simulate_fadds(uint32 *machine_code, struct testcase *test_case);
void simulate_fadds_dot(uint32 *machine_code, struct testcase *test_case);
void simulate_fsubs(uint32 *machine_code, struct testcase *test_case);
void simulate_fsubs_dot(uint32 *machine_code, struct testcase *test_case);
void simulate_fmuls(uint32 *machine_code, struct testcase *test_case);
void simulate_fmuls_dot(uint32 *machine_code, struct testcase *test_case);
void simulate_fdivs(uint32 *machine_code, struct testcase *test_case);
void simulate_fdivs_dot(uint32 *machine_code, struct testcase *test_case);
void simulate_fsqrts(uint32 *machine_code, struct testcase *test_case);
void simulate_fsqrts_dot(uint32 *machine_code, struct testcase *test_case);
void simulate_fcfidus(uint32 *machine_code , struct testcase *test_case);
void simulate_fcfids(uint32 *machine_code , struct testcase *test_case);
void simulate_fcfidu(uint32 *machine_code , struct testcase *test_case);
void simulate_fctiwuz(uint32 *machine_code , struct testcase *test_case);
void simulate_fctiwu(uint32 *machine_code, struct testcase *test_case);
void simulate_fctiduz(uint32 *machine_code, struct testcase *test_case);
void simulate_fctidu(uint32 *machine_code, struct testcase *test_case);
void simulate_ftsqrt(uint32 *machine_code, struct testcase *test_case);
void simulate_ftdiv(uint32 *machine_code, struct testcase *test_case);
void simulate_mcrfs(uint32 *machine_code, struct testcase *test_case);
void simulate_fcmpu(uint32 *machine_code, struct testcase *test_case);
void simulate_fcmpo(uint32 *machine_code, struct testcase *test_case);
void simulate_mtfsb0(uint32 *machine_code, struct testcase *test_case);
void simulate_mtfsb1(uint32 *machine_code, struct testcase *test_case);
void simulate_mtfsf(uint32 *machine_code, struct testcase *test_case);
void simulate_mtfsfi(uint32 *machine_code, struct testcase *test_case);
void simulate_mffs(uint32 *machine_code, struct testcase *test_case);
void simulate_frsp(uint32 *machine_code, struct testcase *test_case);
void simulate_fctid(uint32 *machine_code, struct testcase *test_case);
void simulate_fctidz(uint32 *machine_code, struct testcase *test_case);
void simulate_fctiw(uint32 *machine_code, struct testcase *test_case);
void simulate_fctiwz(uint32 *machine_code, struct testcase *test_case);
void simulate_fcfid(uint32 *machine_code, struct testcase *test_case);
void simulate_frin(uint32 *machine_code, struct testcase *test_case);
void simulate_friz(uint32 *machine_code, struct testcase *test_case);
void simulate_frim(uint32 *machine_code, struct testcase *test_case);
void simulate_frip(uint32 *machine_code, struct testcase *test_case);
void simulate_fmadd(uint32 *machine_code, struct testcase *test_case);
void simulate_fmadds(uint32 *machine_code, struct testcase *test_case);
void simulate_fmsub(uint32 *machine_code, struct testcase *test_case);
void simulate_fmsubs(uint32 *machine_code, struct testcase *test_case);


void simulate_fnmadd(uint32 *machine_code, struct testcase *test_case);
void simulate_fnmadds(uint32 *machine_code, struct testcase *test_case);
void simulate_fnmsub(uint32 *machine_code, struct testcase *test_case);
void simulate_fnmsubs(uint32 *machine_code, struct testcase *test_case);
void simulate_fsel(uint32 *machine_code, struct testcase *test_case);
void simulate_fsel_dot(uint32*,struct testcase*);
void simulate_fre(uint32 *machine_code, struct testcase *test_case);
void simulate_fres(uint32 *machine_code, struct testcase *test_case);
void simulate_frsqrte(uint32 *machine_code, struct testcase *test_case);
void simulate_frsqrtes(uint32 *machine_code, struct testcase *test_case);
void simulate_lfsx(uint32 *machine_code, struct testcase *test_case);
void simulate_lfdx(uint32 *machine_code, struct testcase *test_case);
void simulate_lfiwax(uint32 *machine_code, struct testcase *test_case);
void simulate_lfiwzx(uint32 *machine_code, struct testcase *test_case);
void simulate_lfsux(uint32 *machine_code, struct testcase *test_case);
void simulate_lfdux(uint32 *machine_code, struct testcase *test_case);
void simulate_lfs(uint32 *machine_code, struct testcase *test_case);
void simulate_lfd(uint32 *machine_code, struct testcase *test_case);
void simulate_lfsu(uint32 *machine_code, struct testcase *test_case);
void simulate_lfdu(uint32 *machine_code, struct testcase *test_case);
void simulate_stfsx(uint32 *machine_code, struct testcase *test_case);
void simulate_stfdx(uint32 *machine_code, struct testcase *test_case);
void simulate_stfiwx(uint32 *machine_code, struct testcase *test_case);
void simulate_stfsux(uint32 *machine_code, struct testcase *test_case);
void simulate_stfdux(uint32 *machine_code, struct testcase *test_case);
void simulate_stfs(uint32 *machine_code, struct testcase *test_case);
void simulate_stfd(uint32 *machine_code, struct testcase *test_case);
void simulate_stfsu(uint32 *machine_code, struct testcase *test_case);
void simulate_stfdu(uint32 *machine_code, struct testcase *test_case);

void simulate_addi(uint32 *machine_code, struct testcase *test_case);
void simulate_addis(uint32 *machine_code, struct testcase *test_case);
void simulate_stwx(uint32 *machine_code, struct testcase *test_case);
void simulate_stdx(uint32 *machine_code, struct testcase *test_case);
void simulate_and(uint32 *machine_code, struct testcase *test_case);
void simulate_andc(uint32 *machine_code, struct testcase *test_case);
void simulate_or(uint32 *machine_code, struct testcase *test_case);
void simulate_ori(uint32 *machine_code, struct testcase *test_case);
void simulate_nop(uint32 *machine_code, struct testcase *test_case);
void simulate_mfcr(uint32 *machine_code, struct testcase *test_case);

/* mffs lightweight */
void simulate_mffsce(uint32 *machine_code, struct testcase *test_case);
void simulate_mffscdrn(uint32 *machine_code, struct testcase *test_case);
void simulate_mffscdrni(uint32 *machine_code, struct testcase *test_case);
void simulate_mffscrn(uint32 *machine_code, struct testcase *test_case);
void simulate_mffscrni(uint32 *machine_code, struct testcase *test_case);
void simulate_mffsl(uint32 *machine_code, struct testcase *test_case);

/* VSX prototypes */

int simulate_lxsdux(uint32 * machine_code , struct testcase *test_case);
int simulate_lxsdx(uint32 * machine_code , struct testcase *test_case);
int simulate_lxswux(uint32 * machine_code , struct testcase *test_case);
int simulate_lxswx(uint32 * machine_code , struct testcase *test_case);
int simulate_lxsiwax(uint32 * machine_code , struct testcase *test_case);
int simulate_lxsiwzx(uint32 * machine_code , struct testcase *test_case);
int simulate_lxsspx(uint32 * machine_code , struct testcase *test_case);
int simulate_lxvd2ux(uint32 * machine_code , struct testcase *test_case);
int simulate_lxvd2x(uint32 * machine_code , struct testcase *test_case);
int simulate_lxvw4ux(uint32 *machine_code , struct testcase *test_case);
int simulate_lxvw4x(uint32 *machine_code , struct testcase *test_case );
int simulate_lxvdsx(uint32 *machine_code , struct testcase *test_case );
int simulate_stxsdux(uint32 *machine_code , struct testcase *test_case );
int simulate_stxsdx(uint32 *machine_code , struct testcase *test_case);
int simulate_stxswux(uint32 *machine_code , struct testcase *test_case);
int simulate_stxswx(uint32 *machine_code , struct testcase *test_case);
int simulate_stxsiwx(uint32 *machine_code , struct testcase *test_case);
int simulate_stxsspx(uint32 *machine_code , struct testcase *test_case);
int simulate_stxvd2ux(uint32 *machine_code , struct testcase *test_case);
int simulate_stxvd2x(uint32 *machine_code , struct testcase *test_case);
int simulate_stxvw4ux(uint32 *machine_code , struct testcase *test_case);
int simulate_stxvw4x(uint32 *machine_code , struct testcase *test_case);
int simulate_xsredp(uint32 * machine_code , struct testcase *test_case );
int simulate_xvredp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsresp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvresp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsrsqrtedp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvrsqrtedp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsrsqrtesp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvrsqrtesp(uint32 * machine_code , struct testcase *test_case);
int simulate_xssqrtdp(uint32 * machine_code , struct testcase *test_case );
int simulate_xssqrtsp(uint32 * machine_code , struct testcase *test_case );
int simulate_xvsqrtdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvsqrtsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xscpsgndp( uint32 *machine_code , struct testcase *test_case);
int simulate_xscpsgnsp(uint32 *machine_code , struct testcase *test_case  );
int simulate_xvcpsgnsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcpsgndp(uint32 *machine_code , struct testcase *test_case);
int simulate_xscmpodp( uint32 *machine_code , struct testcase *test_case);
int simulate_xscmposp( uint32 *machine_code , struct testcase *test_case);
int simulate_xscmpudp( uint32 *machine_code , struct testcase *test_case);
int simulate_xscmpusp( uint32 *machine_code , struct testcase *test_case);
int simulate_xvcmpeqdp( uint32 *machine_code , struct testcase *test_case );
int simulate_xvcmpeqsp( uint32 *machine_code , struct testcase *test_case);
int simulate_xvcmpgedp( uint32 *machine_code , struct testcase *test_case);
int simulate_xvcmpgesp( uint32 *machine_code , struct testcase *test_case );
int simulate_xvcmpgtdp( uint32 *machine_code , struct testcase *test_case );
int simulate_xvcmpgtsp( uint32 *machine_code , struct testcase *test_case );
int simulate_xscvdpsp(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xscvspdp(uint32 *machine_code ,  struct testcase *test_case );
int simulate_xscvdpspn(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xscvspdpn(uint32 *machine_code ,  struct testcase *test_case );
int simulate_xvcvdpsp(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xvcvspdp(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xscvdpsxds(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xscvdpsxws(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xscvdpuxds(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xscvdpuxws(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xscvspsxds( uint32 *machine_code ,  struct testcase *test_case);
int simulate_xscvspsxws(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xscvspuxds(uint32 *machine_code , struct testcase *test_case);
int simulate_xscvspuxws(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvdpsxds(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvdpsxws(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvdpuxds(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvdpuxws(uint32 *machine_code , struct testcase *test_case );
int simulate_xvcvspsxds(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvspsxws(uint32 *machine_code , struct testcase *test_case );
int simulate_xvcvspuxds(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvspuxws(uint32 *machine_code , struct testcase *test_case);
int simulate_xscvsxddp(uint32 *machine_code , struct testcase *test_case);
int simulate_xscvsxdsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xscvsxwdp(uint32 *machine_code , struct testcase *test_case);
int simulate_xscvsxwsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xscvuxddp(uint32 *machine_code , struct testcase *test_case);
int simulate_xscvuxdsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xscvuxwdp(uint32 *machine_code , struct testcase *test_case);
int simulate_xscvuxwsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvsxddp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvsxdsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvsxwdp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvsxwsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvuxddp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvuxdsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvuxwdp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvcvuxwsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrdpi(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrdpic(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrdpim(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrdpip(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrdpiz(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrspi(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrspic(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrspim(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrspip(uint32 *machine_code , struct testcase *test_case);
int simulate_xsrspiz(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrdpi(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrdpic(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrdpim(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrdpip(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrdpiz(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrspi(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrspic(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrspim(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrspip(uint32 *machine_code , struct testcase *test_case);
int simulate_xvrspiz(uint32 *machine_code , struct testcase *test_case);
int simulate_xxland( uint32 *machine_code ,  struct testcase *test_case );
int simulate_xxlandc( uint32 *machine_code ,  struct testcase *test_case );
int simulate_xxlnor( uint32 *machine_code ,  struct testcase *test_case );
int simulate_xxlor(uint32 *machine_code ,  struct testcase *test_case);
int simulate_xxlxor(uint32 *machine_code ,  struct testcase *test_case );
int simulate_xxleqv(uint32 *machine_code ,  struct testcase *test_case );
int simulate_xxlnand(uint32 *machine_code ,  struct testcase *test_case );
int simulate_xxlorc(uint32 *machine_code ,  struct testcase *test_case );
int simulate_xxmrghw( uint32 * machine_code ,  struct testcase *test_case );
int simulate_xxmrglw( uint32 *machine_code ,  struct testcase *test_case );
int simulate_xxspltw( uint32 *machine_code ,  struct testcase *test_case );
int simulate_xxpermdi( uint32 * machine_code ,  struct testcase *test_case );
int simulate_xxperm( uint32 * machine_code ,  struct testcase *test_case );
int simulate_xxpermr( uint32 * machine_code ,  struct testcase *test_case );
int simulate_xxsel( uint32 * machine_code ,  struct testcase *test_case );
int simulate_xxsldwi( uint32 * machine_code ,  struct testcase *test_case );
int simulate_xsmaxdp( uint32 *machine_code , struct testcase *test_case );
int simulate_xsmaxsp( uint32 *machine_code , struct testcase *test_case );
int simulate_xsmindp( uint32 *machine_code , struct testcase *test_case);
int simulate_xsminsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvmaxdp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvmaxsp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvmindp(uint32 *machine_code , struct testcase *test_case);
int simulate_xvminsp(uint32 *machine_code , struct testcase *test_case );
int simulate_xsabsdp(uint32 *machine_code ,  struct testcase * test_case);
int simulate_xsabssp(uint32 *machine_code ,  struct testcase * test_case );
int simulate_xsnabsdp(uint32 *machine_code ,  struct testcase * test_case);
int simulate_xsnabssp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnegdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnegsp(uint32 *machine_code , struct testcase * test_case );
int simulate_xvabsdp(uint32 *machine_code , struct testcase * test_case );
int simulate_xvabssp(uint32 *machine_code , struct testcase * test_case );
int simulate_xvnabsdp( uint32 *machine_code , struct testcase * test_case);
int simulate_xvnabssp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvnegdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvnegsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsadddp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsaddsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xssubdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xssubsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmuldp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmulsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsdivdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsdivsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvadddp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvaddsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvsubdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvsubsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvmuldp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvmulsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvdivdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xvdivsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmaddadp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmaddasp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmaddmdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmaddmsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmsubadp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmsubasp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmsubmdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsmsubmsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnmaddadp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnmaddasp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnmaddmdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnmaddmsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnmsubadp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnmsubasp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnmsubmdp(uint32 *machine_code , struct testcase * test_case);
int simulate_xsnmsubmsp(uint32 *machine_code , struct testcase * test_case);
int simulate_xstdivdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xstdivsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xstsqrtdp(uint32 * machine_code , struct testcase *test_case );
int simulate_xstsqrtsp(uint32 * machine_code , struct testcase *test_case );
int simulate_xvtdivdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvtdivsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvtsqrtdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvtsqrtsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvmaddadp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvmaddasp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvmaddmdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvmaddmsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvmsubadp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvmsubasp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvmsubmdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvmsubmsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvnmaddadp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvnmaddasp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvnmaddmdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvnmaddmsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvnmsubadp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvnmsubasp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvnmsubmdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvnmsubmsp(uint32 * machine_code , struct testcase *test_case);
int simulate_mfvsrd(uint32 * machine_code , struct testcase *test_case);
int simulate_mfvsrwz(uint32 * machine_code , struct testcase *test_case);
int simulate_mtvsrd(uint32 * machine_code , struct testcase *test_case);
int simulate_mtvsrwa(uint32 * machine_code , struct testcase *test_case);
int simulate_mtvsrwz(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvhpdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvdphp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvcvhpsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvcvsphp(uint32 * machine_code , struct testcase *test_case);
/* ---- P9 VSX Instructions - RFC02465 ----- */
int simulate_xscpsgnqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsaddqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsaddqpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xssubqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xssubqpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xsmulqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsmulqpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xsdivqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsdivqpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xsnegqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsabsqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsnabsqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xscmpexpqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xscmpoqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xscmpuqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvdpqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvqpdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvqpdpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvqpsdz(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvqpswz(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvqpudz(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvqpuwz(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvsdqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xscvudqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsxexpqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsxsigqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsiexpqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsmaddqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsmaddqpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xsmsubqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsmsubqpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xsnmaddqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsnmaddqpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xsnmsubqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsnmsubqpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xsrqpi(uint32 * machine_code , struct testcase *test_case);
int simulate_xsrqpix(uint32 * machine_code , struct testcase *test_case);
int simulate_xsrqpxp(uint32 * machine_code , struct testcase *test_case);
int simulate_xssqrtqp(uint32 * machine_code , struct testcase *test_case);
int simulate_xssqrtqpo(uint32 * machine_code , struct testcase *test_case);
int simulate_xststdcqp(uint32 * machine_code , struct testcase *test_case);


int simulate_lvebx (uint32 * machine_code , struct testcase *test_case);
int simulate_lvehx (uint32 * machine_code , struct testcase *test_case);
int simulate_lvewx (uint32 * machine_code , struct testcase *test_case);
int simulate_lvx (uint32 * machine_code , struct testcase *test_case);
int simulate_lvxl (uint32 * machine_code , struct testcase *test_case);
int simulate_stvebx (uint32 * machine_code , struct testcase *test_case);
int simulate_stvehx (uint32 * machine_code , struct testcase *test_case);
int simulate_stvewx (uint32 * machine_code , struct testcase *test_case);
int simulate_stvx (uint32 * machine_code , struct testcase *test_case);
int simulate_stvxl (uint32 * machine_code , struct testcase *test_case);
int simulate_lvsl (uint32 * machine_code , struct testcase *test_case);
int simulate_lvsr (uint32 * machine_code , struct testcase *test_case);
int simulate_vpkpx (uint32 * machine_code , struct testcase *test_case);
int simulate_vpkshss (uint32 * machine_code , struct testcase *test_case);
int simulate_vpkswss(uint32 * machine_code , struct testcase *test_case);
int simulate_vpksdss(uint32 * machine_code , struct testcase *test_case);
int simulate_vpkshus (uint32 * machine_code , struct testcase *test_case);
int simulate_vpkswus(uint32 * machine_code , struct testcase *test_case);
int simulate_vpksdus(uint32 * machine_code , struct testcase *test_case);
int simulate_vpkuhum (uint32 * machine_code , struct testcase *test_case);
int simulate_vpkuwum (uint32 * machine_code , struct testcase *test_case);
int simulate_vpkudum (uint32 * machine_code , struct testcase *test_case);
int simulate_vpkuhus (uint32 * machine_code , struct testcase *test_case);
int simulate_vpkuwus (uint32 * machine_code , struct testcase *test_case);
int simulate_vpkudus (uint32 * machine_code , struct testcase *test_case);
int simulate_vupkhpx (uint32 * machine_code , struct testcase *test_case);
int simulate_vupkhsb (uint32 * machine_code , struct testcase *test_case);
int simulate_vupkhsh (uint32 * machine_code , struct testcase *test_case);
int simulate_vupkhsw (uint32 * machine_code , struct testcase *test_case);
int simulate_vupklpx (uint32 * machine_code , struct testcase *test_case);
int simulate_vupklsb (uint32 * machine_code , struct testcase *test_case);
int simulate_vupklsh (uint32 * machine_code , struct testcase *test_case);
int simulate_vupklsw (uint32 * machine_code , struct testcase *test_case);
int simulate_vmrghb (uint32 * machine_code , struct testcase *test_case);
int simulate_vmrghh (uint32 * machine_code , struct testcase *test_case);
int simulate_vmrghw (uint32 * machine_code , struct testcase *test_case);
int simulate_vmrglb (uint32 * machine_code , struct testcase *test_case);
int simulate_vmrglh (uint32 * machine_code , struct testcase *test_case);
int simulate_vmrglw (uint32 * machine_code , struct testcase *test_case);
int simulate_vmrgew (uint32 * machine_code , struct testcase *test_case);
int simulate_vmrgow (uint32 * machine_code , struct testcase *test_case);
int simulate_vspltb (uint32 * machine_code , struct testcase *test_case);
int simulate_vsplth (uint32 * machine_code , struct testcase *test_case);


/* DFP simulation routines */
int simulate_dadd(uint32 * machine_code , struct testcase *test_case);
int simulate_dadd_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_daddq(uint32 * machine_code , struct testcase *test_case);
int simulate_daddq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dsub(uint32 * machine_code , struct testcase *test_case);
int simulate_dsub_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dsubq(uint32 * machine_code , struct testcase *test_case);
int simulate_dsubq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dmul(uint32 * machine_code , struct testcase *test_case);
int simulate_dmul_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dmulq(uint32 * machine_code , struct testcase *test_case);
int simulate_dmulq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_ddiv(uint32 * machine_code , struct testcase *test_case);
int simulate_ddiv_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_ddivq(uint32 * machine_code , struct testcase *test_case);
int simulate_ddivq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dcmpo(uint32 * machine_code , struct testcase *test_case);
int simulate_dcmpoq (uint32 * machine_code , struct testcase *test_case);
int simulate_dcmpu(uint32 * machine_code , struct testcase *test_case);
int simulate_dcmpuq(uint32 * machine_code , struct testcase *test_case);
int simulate_dtstdc(uint32 * machine_code , struct testcase *test_case);
int simulate_dtstdcq(uint32 * machine_code , struct testcase *test_case);
int simulate_dtstdg(uint32 * machine_code , struct testcase *test_case);
int simulate_dtstdgq(uint32 * machine_code , struct testcase *test_case);
int simulate_dtstex(uint32 * machine_code , struct testcase *test_case);
int simulate_dtstexq(uint32 * machine_code , struct testcase *test_case);
int simulate_dtstsf(uint32 * machine_code , struct testcase *test_case);
int simulate_dtstsfq(uint32 * machine_code , struct testcase *test_case);
int simulate_dquai(uint32 * machine_code , struct testcase *test_case);
int simulate_dquai_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dquaiq(uint32 * machine_code , struct testcase *test_case);
int simulate_dquaiq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dqua(uint32 * machine_code , struct testcase *test_case);
int simulate_dqua_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dquaq(uint32 * machine_code , struct testcase *test_case);
int simulate_dquaq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_drrnd(uint32 * machine_code , struct testcase *test_case);
int simulate_drrnd_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_drrndq(uint32 * machine_code , struct testcase *test_case);
int simulate_drrndq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_drintx(uint32 * machine_code , struct testcase *test_case);
int simulate_drintx_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_drintxq(uint32 * machine_code , struct testcase *test_case);
int simulate_drintxq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_drintn(uint32 * machine_code , struct testcase *test_case);
int simulate_drintn_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_drintnq(uint32 * machine_code , struct testcase *test_case);
int simulate_drintnq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dctdp(uint32 * machine_code , struct testcase *test_case);
int simulate_dctdp_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dctqpq(uint32 * machine_code , struct testcase *test_case);
int simulate_dctqpq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_drsp(uint32 * machine_code , struct testcase *test_case);
int simulate_drsp_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_drdpq(uint32 * machine_code , struct testcase *test_case);
int simulate_drdpq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dcffix(uint32 * machine_code , struct testcase *test_case);
int simulate_dcffix_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dcffixq(uint32 * machine_code , struct testcase *test_case);
int simulate_dcffixq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dctfix(uint32 * machine_code , struct testcase *test_case);
int simulate_dctfix_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dctfixq(uint32 * machine_code , struct testcase *test_case);
int simulate_dctfixq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_ddedpd(uint32 * machine_code , struct testcase *test_case);
int simulate_ddedpd_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_ddedpdq (uint32 * machine_code , struct testcase *test_case);
int simulate_ddedpdq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_denbcd(uint32 * machine_code , struct testcase *test_case);
int simulate_denbcd_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_denbcdq(uint32 * machine_code , struct testcase *test_case);
int simulate_denbcdq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dxex(uint32 * machine_code , struct testcase *test_case);
int simulate_dxex_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dxexq(uint32 * machine_code , struct testcase *test_case);
int simulate_dxexq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_diex(uint32 * machine_code , struct testcase *test_case);
int simulate_diex_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_diexq(uint32 * machine_code , struct testcase *test_case);
int simulate_diexq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dscli(uint32 * machine_code , struct testcase *test_case);
int simulate_dscli_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dscliq(uint32 * machine_code , struct testcase *test_case);
int simulate_dscliq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dscri(uint32 * machine_code , struct testcase *test_case);
int simulate_dscri_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dscriq(uint32 * machine_code , struct testcase *test_case);
int simulate_dscriq_dot(uint32 * machine_code , struct testcase *test_case);
int simulate_dtstsfi(uint32 * machine_code, struct testcase *test_case);
int simulate_dtstsfiq(uint32 * machine_code, struct testcase *test_case);


int simulate_vspltw (uint32 * machine_code , struct testcase *test_case);
int simulate_vspltisb (uint32 * machine_code , struct testcase *test_case);
int simulate_vspltish (uint32 * machine_code , struct testcase *test_case);
int simulate_vspltisw (uint32 * machine_code , struct testcase *test_case);
int simulate_vperm(uint32 * machine_code , struct testcase *test_case);
int simulate_vsel(uint32 * machine_code , struct testcase *test_case);
int simulate_vsl(uint32 * machine_code , struct testcase *test_case);
int simulate_vsldoi(uint32 * machine_code , struct testcase *test_case);
int simulate_vslo(uint32 * machine_code , struct testcase *test_case);
int simulate_vsr(uint32 * machine_code , struct testcase *test_case);
int simulate_vsro(uint32 * machine_code , struct testcase *test_case);
int simulate_vaddcuw(uint32 * machine_code , struct testcase *test_case);
int simulate_vaddsbs(uint32 * machine_code , struct testcase *test_case);
int simulate_vaddshs(uint32 * machine_code , struct testcase *test_case);
int simulate_vaddsws(uint32 * machine_code , struct testcase *test_case);
int simulate_vaddubm(uint32 * machine_code , struct testcase *test_case);
int simulate_vadduhm(uint32 * machine_code , struct testcase *test_case);
int simulate_vadduwm(uint32 * machine_code , struct testcase *test_case);
int simulate_vaddubs(uint32 * machine_code , struct testcase *test_case);
int simulate_vadduhs(uint32 * machine_code , struct testcase *test_case);
int simulate_vadduws(uint32 * machine_code , struct testcase *test_case);
int simulate_vaddudm(uint32 * machine_code , struct testcase *test_case);
int simulate_vsubudm(uint32 * machine_code , struct testcase *test_case);
int simulate_vsubcuw(uint32 * machine_code , struct testcase *test_case);
int simulate_vsubsbs(uint32 * machine_code , struct testcase *test_case);
int simulate_vsubshs(uint32 * machine_code , struct testcase *test_case);
int simulate_vsubsws(uint32 * machine_code , struct testcase *test_case);
int simulate_vsububm(uint32 * machine_code , struct testcase *test_case);
int simulate_vsubuhm(uint32 * machine_code , struct testcase *test_case);
int simulate_vsubuwm(uint32 * machine_code , struct testcase *test_case);
int simulate_vsububs(uint32 * machine_code , struct testcase *test_case);
int simulate_vsubuhs(uint32 * machine_code , struct testcase *test_case);
int simulate_vsubuws(uint32 * machine_code , struct testcase *test_case);
int simulate_vmulesb(uint32 * machine_code , struct testcase *test_case);
int simulate_vmulesh(uint32 * machine_code , struct testcase *test_case);
int simulate_vmulesw(uint32 * machine_code , struct testcase *test_case);
int simulate_vmuleub(uint32 * machine_code , struct testcase *test_case);
int simulate_vmuleuh(uint32 * machine_code , struct testcase *test_case);
int simulate_vmuleuw(uint32 * machine_code , struct testcase *test_case);
int simulate_vmulosb(uint32 * machine_code , struct testcase *test_case);
int simulate_vmulosh(uint32 * machine_code , struct testcase *test_case);
int simulate_vmulosw(uint32 * machine_code , struct testcase *test_case);
int simulate_vmuloub(uint32 * machine_code , struct testcase *test_case);
int simulate_vmulouh(uint32 * machine_code , struct testcase *test_case);
int simulate_vmulouw(uint32 * machine_code , struct testcase *test_case);
int simulate_vmuluwm(uint32 * machine_code , struct testcase *test_case);
int simulate_vmhaddshs(uint32 * machine_code , struct testcase *test_case);
int simulate_vmhraddshs(uint32 * machine_code , struct testcase *test_case);
int simulate_vmladduhm(uint32 * machine_code , struct testcase *test_case);
int simulate_vmsumubm(uint32 * machine_code , struct testcase *test_case);
int simulate_vmsummbm(uint32 * machine_code , struct testcase *test_case);
int simulate_vmsumshm(uint32 * machine_code , struct testcase *test_case);
int simulate_vmsumshs(uint32 * machine_code , struct testcase *test_case);
int simulate_vmsumuhm(uint32 * machine_code , struct testcase *test_case);
int simulate_vmsumuhs(uint32 * machine_code , struct testcase *test_case);
int simulate_vsumsws(uint32 * machine_code , struct testcase *test_case);
int simulate_vsum2sws(uint32 * machine_code , struct testcase *test_case);
int simulate_vsum4sbs(uint32 * machine_code , struct testcase *test_case);
int simulate_vsum4ubs(uint32 * machine_code , struct testcase *test_case);
int simulate_vsum4shs(uint32 * machine_code , struct testcase *test_case);
int simulate_vavgsb(uint32 * machine_code , struct testcase *test_case);
int simulate_vavgsh(uint32 * machine_code , struct testcase *test_case);
int simulate_vavgsw(uint32 * machine_code , struct testcase *test_case);
int simulate_vavgub(uint32 * machine_code , struct testcase *test_case);
int simulate_vavguh(uint32 * machine_code , struct testcase *test_case);
int simulate_vavguw(uint32 * machine_code , struct testcase *test_case);
int simulate_vmaxsb(uint32 * machine_code , struct testcase *test_case);
int simulate_vmaxsh(uint32 * machine_code , struct testcase *test_case);
int simulate_vmaxsw(uint32 * machine_code , struct testcase *test_case);
int simulate_vmaxub(uint32 * machine_code , struct testcase *test_case);
int simulate_vmaxuh(uint32 * machine_code , struct testcase *test_case);
int simulate_vmaxuw(uint32 * machine_code , struct testcase *test_case);
int simulate_vmaxsd(uint32 * machine_code , struct testcase *test_case);
int simulate_vmaxud(uint32 * machine_code , struct testcase *test_case);
int simulate_vminsb(uint32 * machine_code , struct testcase *test_case);
int simulate_vminsh(uint32 * machine_code , struct testcase *test_case);
int simulate_vminsw (uint32 * machine_code , struct testcase *test_case);
int simulate_vminub (uint32 * machine_code , struct testcase *test_case);
int simulate_vminuh (uint32 * machine_code , struct testcase *test_case);
int simulate_vminuw (uint32 * machine_code , struct testcase *test_case);
int simulate_vminsd (uint32 * machine_code , struct testcase *test_case);
int simulate_vminud (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpequb (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpequb_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpequh (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpequh_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpequw (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpequw_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpequd (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpequd_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtsb (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtsb_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtsh (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtsh_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtsw (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtsw_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtsd (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtsd_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtub (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtub_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtuh (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtuh_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtuw (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtuw_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtud (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtud_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vand (uint32 * machine_code , struct testcase *test_case);
int simulate_vandc (uint32 * machine_code , struct testcase *test_case);
int simulate_vnor (uint32 * machine_code , struct testcase *test_case);
int simulate_vor (uint32 * machine_code , struct testcase *test_case);
int simulate_vxor (uint32 * machine_code , struct testcase *test_case);
int simulate_vrlb (uint32 * machine_code , struct testcase *test_case);
int simulate_vrlh (uint32 * machine_code , struct testcase *test_case);
int simulate_vrlw (uint32 * machine_code , struct testcase *test_case);
int simulate_vrld (uint32 * machine_code , struct testcase *test_case);
int simulate_vslb (uint32 * machine_code , struct testcase *test_case);
int simulate_vslh (uint32 * machine_code , struct testcase *test_case);
int simulate_vslw (uint32 * machine_code , struct testcase *test_case);
int simulate_vsld (uint32 * machine_code , struct testcase *test_case);
int simulate_vsrb (uint32 * machine_code , struct testcase *test_case);
int simulate_vsrh (uint32 * machine_code , struct testcase *test_case);
int simulate_vsrw (uint32 * machine_code , struct testcase *test_case);
int simulate_vsrab (uint32 * machine_code , struct testcase *test_case);
int simulate_vsrah (uint32 * machine_code , struct testcase *test_case);
int simulate_vsraw (uint32 * machine_code , struct testcase *test_case);
int simulate_vsrd (uint32 * machine_code , struct testcase *test_case);
int simulate_vsrad (uint32 * machine_code , struct testcase *test_case);
int simulate_vaddfp (uint32 * machine_code , struct testcase *test_case);
int simulate_vsubfp (uint32 * machine_code , struct testcase *test_case);
int simulate_vmaddfp (uint32 * machine_code , struct testcase *test_case);
int simulate_vnmsubfp (uint32 * machine_code , struct testcase *test_case);
int simulate_vmaxfp (uint32 * machine_code , struct testcase *test_case);
int simulate_vminfp (uint32 * machine_code , struct testcase *test_case);
int simulate_vctsxs (uint32 * machine_code , struct testcase *test_case);
int simulate_vctuxs (uint32 * machine_code , struct testcase *test_case);
int simulate_vcfsx (uint32 * machine_code , struct testcase *test_case);
int simulate_vcfux (uint32 * machine_code , struct testcase *test_case);
int simulate_vrfim (uint32 * machine_code , struct testcase *test_case);
int simulate_vrfin (uint32 * machine_code , struct testcase *test_case);
int simulate_vrfip (uint32 * machine_code , struct testcase *test_case);
int simulate_vrfiz (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpbfp (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpbfp_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpeqfp (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpeqfp_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgefp (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgefp_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtfp (uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpgtfp_dot (uint32 * machine_code , struct testcase *test_case);
int simulate_vclzb (uint32 * machine_code , struct testcase *test_case);
int simulate_vclzh (uint32 * machine_code , struct testcase *test_case);
int simulate_vclzw (uint32 * machine_code , struct testcase *test_case);
int simulate_vclzd (uint32 * machine_code , struct testcase *test_case);
int simulate_vgbbd (uint32 * machine_code , struct testcase *test_case);
int simulate_bcdadd(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdsub(uint32 * machine_code , struct testcase *test_case);

#if 0 /* Estimate instructions */
int simulate_vexptefp (uint32 * machine_code , struct testcase *test_case);
int simulate_vlogefp (uint32 * machine_code , struct testcase *test_case);
int simulate_vrefp (uint32 * machine_code , struct testcase *test_case);
int simulate_vrsqrtefp (uint32 * machine_code , struct testcase *test_case);
#endif /* Estimate instructions */

int simulate_mtvscr (uint32 * machine_code , struct testcase *test_case);
int simulate_mfvscr (uint32 * machine_code , struct testcase *test_case);

/* P9 : RFC02462 */
int simulate_lxvll(uint32 * machine_code , struct testcase *test_case);
int simulate_stxvll(uint32 * machine_code , struct testcase *test_case);

/* VMX crypto instructions */
int simulate_vcipher (uint32 * machine_code , struct testcase *test_case);
int simulate_vcipherlast (uint32 * machine_code , struct testcase *test_case);
int simulate_vncipher (uint32 * machine_code , struct testcase *test_case);
int simulate_vncipherlast (uint32 * machine_code , struct testcase *test_case);
int simulate_vsbox (uint32 * machine_code , struct testcase *test_case);
int simulate_vshasigmad (uint32 * machine_code , struct testcase *test_case);
int simulate_vshasigmaw (uint32 * machine_code , struct testcase *test_case);
int simulate_vpmsumb (uint32 * machine_code , struct testcase *test_case);
int simulate_vpmsumd (uint32 * machine_code , struct testcase *test_case);
int simulate_vpmsumh (uint32 * machine_code , struct testcase *test_case);
int simulate_vpmsumw (uint32 * machine_code , struct testcase *test_case);
int simulate_vpermxor(uint32 * machine_code , struct testcase *test_case);
int simulate_veqv(uint32 * machine_code , struct testcase *test_case);
int simulate_vorc(uint32 * machine_code , struct testcase *test_case);
int simulate_vnand(uint32 * machine_code , struct testcase *test_case);

/* VMX populate instructions */
int simulate_vpopcntb(uint32 * machine_code , struct testcase *test_case);
int simulate_vpopcnth(uint32 * machine_code , struct testcase *test_case);
int simulate_vpopcntw(uint32 * machine_code , struct testcase *test_case);
int simulate_vpopcntd(uint32 * machine_code , struct testcase *test_case);

/* VMX Integer instructions */
int simulate_vadduqm(uint32 *machine_code, struct testcase *test_case);
int simulate_vaddcuq(uint32 *machine_code, struct testcase *test_case);
int simulate_vaddeuqm(uint32 *machine_code, struct testcase *test_case);
int simulate_vaddecuq(uint32 *machine_code, struct testcase *test_case);
int simulate_vsubuqm(uint32 *machine_code, struct testcase *test_case);
int simulate_vsubcuq(uint32 *machine_code, struct testcase *test_case);
int simulate_vsubeuqm(uint32 *machine_code, struct testcase *test_case);
int simulate_vsubecuq(uint32 *machine_code, struct testcase *test_case);
int simulate_vbpermq(uint32 *machine_code, struct testcase *test_case);

/* P9 - RFC02462 */
int simulate_bcdcfn(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdcfz(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdctn(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdctz(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdcfsq(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdctsq(uint32 * machine_code , struct testcase *test_case);
int simulate_vmul10uq(uint32 * machine_code , struct testcase *test_case);
int simulate_vmul10cuq(uint32 * machine_code , struct testcase *test_case);
int simulate_vmul10euq(uint32 * machine_code , struct testcase *test_case);
int simulate_vmul10ecuq(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdcpsgn(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdsetsgn(uint32 * machine_code , struct testcase *test_case);
int simulate_bcds(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdus(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdsr(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdtrunc(uint32 * machine_code , struct testcase *test_case);
int simulate_bcdutrunc(uint32 * machine_code , struct testcase *test_case);

/*
 * RFC02467.r10: String Operations (VSU Option)
 * ---------------------------------------------
 */
int simulate_vcmpneb(uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpnezb(uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpneh(uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpnezh(uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpnew(uint32 * machine_code , struct testcase *test_case);
int simulate_vcmpnezw(uint32 * machine_code , struct testcase *test_case);
int simulate_vclzlsbb(uint32 * machine_code , struct testcase *test_case);
int simulate_vctzlsbb(uint32 * machine_code , struct testcase *test_case);
int simulate_vextublx(uint32 * machine_code , struct testcase *test_case);
int simulate_vextubrx(uint32 * machine_code , struct testcase *test_case);
int simulate_vextuhlx(uint32 * machine_code , struct testcase *test_case);
int simulate_vextuhrx(uint32 * machine_code , struct testcase *test_case);
int simulate_vextuwlx(uint32 * machine_code , struct testcase *test_case);
int simulate_vextuwrx(uint32 * machine_code , struct testcase *test_case);


int lvebx (VRegValue *VT,  uint64 *RA, uint64 *RB);
int lvehx (VRegValue *VT,  uint64 *RA, uint64 *RB);
int lvewx (VRegValue *VT,  uint64 *RA, uint64 *RB);
int lvx (VRegValue *VT,  uint64 *RA, uint64 *RB);
int lvxl (VRegValue *VT,  uint64 *RA, uint64 *RB);
int stvebx (VRegValue *VT,  uint64 *RA, uint64 *RB);
int stvehx (VRegValue *VT,  uint64 *RA, uint64 *RB);
int stvewx (VRegValue *VT,  uint64 *RA, uint64 *RB);
int stvx (VRegValue *VT,  uint64 *RA, uint64 *RB);
int stvxl (VRegValue *VT,  uint64 *RA, uint64 *RB);
int lvsl (VRegValue *VT,  uint64 *RA, uint64 *RB);
int lvsr (VRegValue *VT,  uint64 *RA, uint64 *RB);
int vpkpx (VRegValue *VT, VRegValue VA, VRegValue VB);
int vpkshss (VRegValue *VT, VRegValue VA, VRegValue VB, uint64 *vscr);
int vpkswss (VRegValue *VT, VRegValue VA, VRegValue VB, uint64 *vscr);
int vpksdss (VRegValue *VT, VRegValue VA, VRegValue VB, uint64 *vscr);
int vpkshus (VRegValue *VT, VRegValue VA, VRegValue VB, uint64 *vscr);
int vpkswus (VRegValue *VT, VRegValue VA, VRegValue VB, uint64 *vscr);
int vpksdus (VRegValue *VT, VRegValue VA, VRegValue VB, uint64 *vscr);
int vpkuhum (VRegValue *VT, VRegValue VA, VRegValue VB);
int vpkuwum (VRegValue *VT, VRegValue VA, VRegValue VB);
int vpkudum (VRegValue *VT, VRegValue VA, VRegValue VB);
int vpkuhus (VRegValue *VT, VRegValue VA, VRegValue VB, uint64 *vscr);
int vpkuwus (VRegValue *VT, VRegValue VA, VRegValue VB, uint64 *vscr);
int vpkudus (VRegValue *VT, VRegValue VA, VRegValue VB, uint64 *vscr);
int vupkhpx (VRegValue *VT, VRegValue VB);
int vupkhsb(VRegValue *VT, VRegValue VB);
int vupkhsh(VRegValue *VT, VRegValue VB);
int vupkhsw(VRegValue *VT, VRegValue VB);
int vupklpx(VRegValue *VT, VRegValue VB);
int vupklsb(VRegValue *VT, VRegValue VB);
int vupklsh(VRegValue *VT, VRegValue VB);
int vupklsw(VRegValue *VT, VRegValue VB);
int vmrghb ( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmrghh ( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmrghw ( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmrglb ( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmrglh ( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmrglw ( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmrgew ( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmrgow ( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vspltb (VRegValue *VT,  VRegValue VB, unsigned char val );
int vsplth (VRegValue *VT,  VRegValue VB, unsigned char val );
int vspltw (VRegValue *VT,  VRegValue VB, unsigned char val );
int vspltisb (VRegValue *VT,  unsigned char val );
int vspltish (VRegValue *VT,  unsigned char val );
int vspltisw (VRegValue *VT,  unsigned char val );
int vperm (VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC);
int vsel(VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC);
int vsl (VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsldoi (VRegValue *VT,  VRegValue VA, VRegValue VB, uint8 val);
int vslo (VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsr (VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsro (VRegValue *VT,  VRegValue VA, VRegValue VB);
int vaddcuw( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vaddsbs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vaddshs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vaddsws( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vaddubm( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vadduhm( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vadduwm( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vaddubs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vadduhs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vadduws( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vaddudm( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsubudm( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsubcuw( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsubsbs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vsubshs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vsubsws( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vsububm( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsubuhm( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsubuwm( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsububs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vsubuhs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vsubuws( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vmulesb( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmulesh( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmulesw( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmuleub( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmuleuh( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmuleuw( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmulosb( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmulosh( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmulosw( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmuloub( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmulouh( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmulouw( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmuluwm( VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmhaddshs( VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC, uint64 *vscr);
int vmhraddshs( VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC, uint64 *vscr);
int vmladduhm( VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC);
int vmsumubm( VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC);
int vmsummbm( VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC);
int vmsumshm( VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC);
int vmsumuhm( VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC);
int vmsumshs( VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC, uint64 *vscr);
int vmsumuhs( VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC, uint64 *vscr);
int vsumsws( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vsum2sws( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vsum4sbs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vsum4shs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vsum4ubs( VRegValue *VT,  VRegValue VA, VRegValue VB, uint64 *vscr);
int vavgsb(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vavgsh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vavgsw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vavgub(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vavguh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vavguw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmaxsb(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmaxsh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmaxsw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmaxub(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmaxuh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmaxuw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmaxsd(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vmaxud(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminsb(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminsh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminsw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminub(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminuh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminsw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminub(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminuh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminuw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminsd(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vminud(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpequb(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpequh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpequw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpequd(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpgtsb(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpgtsh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpgtsw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpgtsd(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpgtub(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpgtuh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpgtuw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcmpgtud(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vand(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vandc(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vnor(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vor(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vxor(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vrlb(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vrlh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vrlw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vrld(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vslb(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vslh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vslw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsld(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsrb(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsrh(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsrw(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsrab(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsrah (VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsraw (VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsrd(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsrad(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vaddfp (VRegValue *VT,  VRegValue VA, VRegValue VB, uint64* vscr);
int vsubfp (VRegValue *VT,  VRegValue VA, VRegValue VB, uint64* vscr);
int vmaddfp (VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC, uint64* vscr);
int vnmsubfp (VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC, uint64* vscr);
int vmaxfp (VRegValue *VT,  VRegValue VA, VRegValue VB, uint64* vscr);
int vminfp (VRegValue *VT,  VRegValue VA, VRegValue VB, uint64* vscr);
int vctsxs (VRegValue *VT,  VRegValue VB, uint8 uim, uint64* vscr);
int vctuxs (VRegValue *VT,  VRegValue VB, uint8 uim, uint64* vscr);
int vcfsx (VRegValue *VT,  VRegValue VB, uint8 uim, uint64* vscr);
int vcfux (VRegValue *VT,  VRegValue VB, uint8 uim, uint64* vscr);
int vrfim (VRegValue *VT,  VRegValue VB , uint64* vscr);
int vrfin (VRegValue *VT,  VRegValue VB , uint64* vscr);
int vrfip (VRegValue *VT,  VRegValue VB , uint64* vscr);
int vrfiz (VRegValue *VT,  VRegValue VB , uint64* vscr);
int vcmpbfp (VRegValue *VT, VRegValue VA, VRegValue VB, uint8* cr_fld, uint64* vscr);
int vcmpeqfp (VRegValue *VT, VRegValue VA, VRegValue VB, uint64* vscr);
int vcmpgefp (VRegValue *VT, VRegValue VA, VRegValue VB, uint64* vscr);
int vcmpgtfp (VRegValue *VT, VRegValue VA, VRegValue VB, uint64* vscr);
int vclzb(VRegValue *VT, VRegValue VB);
int vclzh(VRegValue *VT, VRegValue VB);
int vclzw(VRegValue *VT, VRegValue VB);
int vclzd(VRegValue *VT, VRegValue VB);
int vgbbd(VRegValue *VT, VRegValue VB);
int bcdadd(VRegValue *, VRegValue, VRegValue, short, uint8 *, boolean);
int vpopcntb(VRegValue *VT, VRegValue VB);
int vpopcnth(VRegValue *VT, VRegValue VB);
int vpopcntw(VRegValue *VT, VRegValue VB);
int vpopcntd(VRegValue *VT, VRegValue VB);
int veqv(VRegValue *VT, VRegValue VA, VRegValue VB);
int vorc(VRegValue *VT, VRegValue VA, VRegValue VB);
int vnand(VRegValue *VT, VRegValue VA, VRegValue VB);

#if 0 /* Estimate instructions */
int vexptefp(VRegValue *VT, VRegValue VB, uint64* vscr);
int vlogefp(VRegValue *VT, VRegValue VB, uint64* vscr);
int vrefp(VRegValue *VT, VRegValue VB, uint64* vscr);
int vrsqrtefp(VRegValue *VT, VRegValue VB, uint64* vscr);
#endif /* Estimate instructions */

int mtvscr (VRegValue VB, uint64* vscr);
int mfvscr (VRegValue* VT, uint64* vscr);
int vcipher(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vcipherlast(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vncipher(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vncipherlast(VRegValue *VT,  VRegValue VA, VRegValue VB);
int vsbox(VRegValue *VT,  VRegValue VA);
int vshasigmad(VRegValue *VT,  VRegValue VA, uint8 st_value, uint8 six_value);
int vshasigmaw(VRegValue *VT,  VRegValue VA, uint8 st_value, uint8 six_value);
int vpmsumb (VRegValue *VT, VRegValue VA, VRegValue VB);
int vpmsumd (VRegValue *VT, VRegValue VA, VRegValue VB);
int vpmsumh (VRegValue *VT, VRegValue VA, VRegValue VB);
int vpmsumw (VRegValue *VT, VRegValue VA, VRegValue VB);
int vpermxor (VRegValue *VT,  VRegValue VA, VRegValue VB, VRegValue VC);

FP_EXCEPTION_T vmicro_dp_recip(uint64 *fpscr , FPRegValue *t, DoubleWord *b);
FP_EXCEPTION_T vmicro_dp_rsqrt( uint64 *fpscr , FPRegValue *t, DoubleWord *b);
FP_EXCEPTION_T vmicro_sp_recip(uint64 * fpscr , SingleWord *t, SingleWord srca);
uint32 vmicro_sp_rsqrt( uint64 *fpscr, uint32 *t, uint32 srca);
uint32 vmicro_dp_sqrt(uint64 *fpscr , FPRegValue *t, DoubleWord *b, RoundingMode_t round_mode);
uint32 vmicro_sp_sqrt(uint64 *fpscr , SingleWord *t, SingleWord b, RoundingMode_t round_mode);
uint32 vmicro_dp_max( uint64 *fpscr, FPRegValue *t, DoubleWord *a, DoubleWord *b);
uint32 vmicro_sp_max(uint64 *fpscr, uint32 *t, uint32 a, uint32 b);
uint32 vmicro_dp_min(uint64 *fpscr , FPRegValue *t, DoubleWord *a, DoubleWord *b);
uint32 vmicro_sp_min(uint64 *fpscr , uint32 *t, uint32 a, uint32 b);
uint32 vmicro_dp_add(FPRegValue *t, DoubleWord *a, DoubleWord *b, uint64 *fpscr);
uint32 vmicro_dp_sub(FPRegValue *t, DoubleWord *a, DoubleWord *b, uint64 *fpscr);
uint32 vmicro_dp_mul(FPRegValue *t, DoubleWord *a, DoubleWord *b, uint64 *fpscr);
uint32 vmicro_dp_div(FPRegValue *t, DoubleWord *a, DoubleWord *b, uint64 *fpscr);
uint32 vmicro_dp_madd(FPRegValue *t, DoubleWord *a, DoubleWord *c, DoubleWord *b, uint64 *fpscr);
uint32 vmicro_dp_msub(FPRegValue *t, DoubleWord *a, DoubleWord *c, DoubleWord *b, uint64 *fpscr);
uint32 vmicro_dp_nmadd(FPRegValue *t, DoubleWord *a, DoubleWord *c, DoubleWord *b, uint64 *fpscr);
uint32 vmicro_dp_nmsub(FPRegValue *t, DoubleWord *a, DoubleWord *b, DoubleWord *c, uint64 *fpscr);
SingleWord vmicro_sp_add(SingleWord *t, SingleWord a, SingleWord b, uint64 *fpscr);
SingleWord vmicro_sp_sub(SingleWord *t, SingleWord a, SingleWord b, uint64 *fpscr);
SingleWord vmicro_sp_mul(SingleWord *t, SingleWord a, SingleWord b, uint64 *fpscr);
SingleWord vmicro_sp_div(SingleWord *t, SingleWord a, SingleWord b, uint64 *fpscr);
SingleWord vmicro_sp_madd(SingleWord *t, SingleWord a, SingleWord c, SingleWord b, uint64 *fpscr);
SingleWord vmicro_sp_msub(SingleWord *t, SingleWord a, SingleWord c, SingleWord b, uint64 *fpscr);
SingleWord vmicro_sp_nmadd(SingleWord *t, SingleWord a, SingleWord c, SingleWord b, uint64 *fpscr);
SingleWord vmicro_sp_nmsub(SingleWord *t, SingleWord a, SingleWord c, SingleWord b, uint64 *fpscr);
uint8 vmicro_dp_div_test( DoubleWord *a, DoubleWord *b);
uint8 vmicro_sp_div_test(SingleWord a, SingleWord b);
uint8 vmicro_dp_sqrt_test(DoubleWord *b);
uint8 vmicro_sp_sqrt_test(SingleWord b);

/* RFC02470 128-bit SIMD FXU Operations */
int simulate_lxsibzx(uint32 * machine_code, struct testcase *test_case);
int simulate_lxsihzx(uint32 * machine_code, struct testcase *test_case);
int simulate_stxsibx(uint32 * machine_code, struct testcase *test_case);
int simulate_stxsihx(uint32 * machine_code, struct testcase *test_case);

int simulate_vbpermd(uint32 * machine_code, struct testcase *test_case);
int simulate_vctzb(uint32 * machine_code, struct testcase *test_case);
int simulate_vctzd(uint32 * machine_code, struct testcase *test_case);
int simulate_vctzh(uint32 * machine_code, struct testcase *test_case);
int simulate_vctzw(uint32 * machine_code, struct testcase *test_case);
int simulate_vextsb2d(uint32 * machine_code, struct testcase *test_case);
int simulate_vextsb2w(uint32 * machine_code, struct testcase *test_case);
int simulate_vextsh2d(uint32 * machine_code, struct testcase *test_case);
int simulate_vextsh2w(uint32 * machine_code, struct testcase *test_case);
int simulate_vextsw2d(uint32 * machine_code, struct testcase *test_case);
int simulate_vnegd(uint32 * machine_code, struct testcase *test_case);
int simulate_vnegw(uint32 * machine_code, struct testcase *test_case);
int simulate_vprtybd(uint32 * machine_code, struct testcase *test_case);
int simulate_vprtybq(uint32 * machine_code, struct testcase *test_case);
int simulate_vprtybw(uint32 * machine_code, struct testcase *test_case);
int simulate_vrldmi(uint32 * machine_code, struct testcase *test_case);
int simulate_vrldnm(uint32 * machine_code, struct testcase *test_case);
int simulate_vrlwmi(uint32 * machine_code, struct testcase *test_case);
int simulate_vrlwnm(uint32 * machine_code, struct testcase *test_case);

/* RFC02469.r1: 128-bit SIMD Video Compression Operations */
int simulate_vabsdub(uint32 * machine_code, struct testcase *test_case);
int simulate_vabsduh(uint32 * machine_code, struct testcase *test_case);
int simulate_vabsduw(uint32 * machine_code, struct testcase *test_case);

/* RFC02471.r13: 128-bit SIMD Miscellaneous Operations */
int simulate_vextractd(uint32 * machine_code , struct testcase *test_case);
int simulate_vextractub(uint32 * machine_code , struct testcase *test_case);
int simulate_vextractuh(uint32 * machine_code , struct testcase *test_case);
int simulate_vextractuw(uint32 * machine_code , struct testcase *test_case);
int simulate_vinsertb(uint32 * machine_code , struct testcase *test_case);
int simulate_vinsertd(uint32 * machine_code , struct testcase *test_case);
int simulate_vinserth(uint32 * machine_code , struct testcase *test_case);
int simulate_vinsertw(uint32 * machine_code , struct testcase *test_case);
int simulate_vpermr(uint32 * machine_code , struct testcase *test_case);

/* RFC02471.r13: 128-bit SIMD Miscellaneous Operations */
int simulate_lxvb16x(uint32 * machine_code , struct testcase *test_case);
int simulate_lxvh8x(uint32 * machine_code , struct testcase *test_case);
int simulate_lxvx(uint32 * machine_code , struct testcase *test_case);
int simulate_lxvwsx(uint32 * machine_code , struct testcase *test_case);
int simulate_mfvsrld(uint32 * machine_code , struct testcase *test_case);
int simulate_mtvsrdd(uint32 * machine_code , struct testcase *test_case);
int simulate_mtvsrws(uint32 * machine_code , struct testcase *test_case);
int simulate_stxvb16x(uint32 * machine_code , struct testcase *test_case);
int simulate_stxvh8x(uint32 * machine_code , struct testcase *test_case);
int simulate_stxvx(uint32 * machine_code , struct testcase *test_case);
int simulate_xxbrd(uint32 * machine_code , struct testcase *test_case);
int simulate_xxbrh(uint32 * machine_code , struct testcase *test_case);
int simulate_xxbrq(uint32 * machine_code , struct testcase *test_case);
int simulate_xxbrw(uint32 * machine_code , struct testcase *test_case);
int simulate_xxextractuw(uint32 * machine_code , struct testcase *test_case);
int simulate_xxinsertw(uint32 * machine_code , struct testcase *test_case);
int simulate_xxspltib(uint32 * machine_code , struct testcase *test_case);

/** 2464   - Binary Floating-Point Support Operations **/
/*******************************************************/
int simulate_xscmpexpdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsiexpdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xststdcdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xststdcsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsxexpdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xsxsigdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xviexpdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xviexpsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvtstdcdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvtstdcsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvxexpdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvxexpsp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvxsigdp(uint32 * machine_code , struct testcase *test_case);
int simulate_xvxsigsp(uint32 * machine_code , struct testcase *test_case);

/* RFC02493.r3: D-form VSX Floating-Point Storage Access Instructions */
int simulate_lxsd(uint32 * machine_code , struct testcase *test_case);
int simulate_stxsd(uint32 * machine_code , struct testcase *test_case);
int simulate_lxssp(uint32 * machine_code , struct testcase *test_case);
int simulate_stxssp(uint32 * machine_code , struct testcase *test_case);
int simulate_lxv(uint32 * machine_code , struct testcase *test_case);
int simulate_stxv(uint32 * machine_code , struct testcase *test_case);




void adjust_fpscr(int cno, uint64 old_fpscr, uint64 fpscr_mask);
void Update_FPSCR(uint64 *fpscr);
boolean Calculate_FPSCR_VX (uint64 val);
boolean Calculate_FPSCR_FEX (uint64 val);

#endif /* _FPU_COMMON_H */


/*****************************************************************/
/*                      DFP definitions                          */
/****************************************************************/

typedef unsigned int DecimalFP32;
typedef unsigned long long DecimalFP64;

#define Twelve 				12
#define MAX_EXP_SHORT  			192 /* max biased exp of short + plus one */
#define MAX_EXP_LONG   			768 /* max biased exp of long + plus one */
#define MAX_EXP_EXT    			12288 /* max biased exp of extended + plus one */
#define SHORT_DECODED_MASK  		0xffffffff80000000ULL;
#define LONG_DECODED_MASK   		0xffe0000000000000ULL;
#define EXTENDED_DECODED_MASK    	0xfffffe0000000000ULL;
#define SHORT_HIGHER_BYTES  		0x007fffffffffffffULL;/* 1st 64 bit w/out sign and exp*/
#define LONG_HIGHER_BYTES  		0x001fffffffffffffULL;/* 1st 64 bit w/out sign and exp*/
#define EXTE_HIGHER_BYTES  		0x0001ffffffffffffULL;/* 1st 64 bit w/out sign and exp*/

struct Decoded {           /* Bits A to M of the 3 unpacked digits */
    unsigned    :20;   /* first 20 bits - unsigned */
    unsigned  A :1;
    unsigned  B :1;
    unsigned  C :1;
    unsigned  D :1;
    unsigned  E :1;
    unsigned  F :1;
    unsigned  G :1;
    unsigned  H :1;
    unsigned  I :1;
    unsigned  J :1;
    unsigned  K :1;
    unsigned  M :1;
} ;

struct UnDecoded {     /* Bits P to Y of the 3 packed digits */
    unsigned    :22;    
    unsigned  P :1;
    unsigned  Q :1;
    unsigned  R :1;
    unsigned  S :1;
    unsigned  T :1;
    unsigned  U :1;
    unsigned  V :1;
    unsigned  W :1;
    unsigned  X :1;
    unsigned  Y :1;
} ;


struct aei {        /* collection of the first bit of each of the 3 digits */
        unsigned    :29;
        unsigned A  :1;   /* first bit of the lefmost digit */
        unsigned E  :1;  /* first bit of the middle digit */
        unsigned I  :1;  /* first bit of the rightmost digit */
};

union The_AEI {
       struct aei AEI;    /* bits A E and I: first 3 bits of the 3 digits */
       unsigned int val;  /* valued of combined AEI bits */
};

struct CF_Parts { /* CF field contains LMD and upper 2 bits of exp*/
        unsigned EXPMSB1 :1; /* exponent most significant bit */
        unsigned EXPMSB2 :1; /* exponent 2nd most significant bit */
        unsigned LMD1 :4;       /* leftmost digit bit1 - from left*/
        unsigned LMD2 :4;       /* leftmost digit bit2 - from left */
        unsigned LMD3 :4;       /* leftmost digit bit3 - from left */
        unsigned LMD4 :4;       /* leftmost digit bit4 - from left */
} ;

struct short_op1 {     /* DFP short operand */
    unsigned sign    :1; /* sign bit */
    unsigned CF      :5; /* combined field */
    unsigned BXCF    :6; /* biased exp con field - short */
    unsigned TheRest :20;
    unsigned lower   :32;   /* bis 32-63 */
};

struct short_op2 {     /* DFP short operand */
    unsigned sign    :1; /* sign bit */
    unsigned expo    :8; /* combined field */
    unsigned TheRest :23;
    unsigned lower   :32;   /* bis 32-63 */
};

union the_short {
    struct short_op1  encoded;
    struct short_op2 decoded;
    unsigned long long TheOp_val;
};

struct long_op1 {     /* DFP long operand - when encoded */
   unsigned sign    :1; /* sign bit */
   unsigned CF      :5; /* combined field */
   unsigned BXCF    :8; /* biased exp con field - short */
   unsigned TheRest :18;
   unsigned lower   :32;   /* bis 32-63 */
};

struct long_op2 {     /* DFP long operand  - when decoded - only 64bits*/
    unsigned sign    :1; /* sign bit */
    unsigned expo    :10; /* combined field */
    unsigned TheRest :21;
    unsigned lower   :32;   /* bis 32-63 */
};

union the_long {
    struct long_op1 encoded;
    struct long_op2 decoded;
    unsigned long long TheOp_val;
};

struct ext_op1 {     /* DFP long operand */
    unsigned sign    :1; /* sign bit */
    unsigned CF      :5; /* combined field */
    unsigned BXCF    :12; /* biased exp con field - short */
    unsigned TheRest :14;
    unsigned lower   :32;   /* bis 32-63 */
};
struct ext_op2 {     /* DFP long operand */
    unsigned sign    :1; /* sign bit */
    unsigned expo    :14; /* combined field */
    unsigned TheRest :17;
    unsigned lower   :32;   /* bis 32-63 */
};
union the_ext {
    struct ext_op1 encoded;
    struct ext_op2 decoded;
    unsigned long long TheOp_val;
};

int IsRedundant[MAX_NUM_CPUS];
int NoNibles[MAX_NUM_CPUS];   			/* no of digits in one register */
int EnableTrace[MAX_NUM_CPUS];
int bxcfLen_MASK[MAX_NUM_CPUS];
short Spans[MAX_NUM_CPUS];
short bxcfLen[MAX_NUM_CPUS];  			/* how long the bias expo continuation field is */
short NoThreeDigSets[MAX_NUM_CPUS];     /* how many three digit sets are there: 2,5, or 11 */

union TheUnDecoded { /* undecoded three digits */
	struct UnDecoded undecoded;
	unsigned int ThreeDig;
};

union TheDecoded { /* Decoded 3 digits */
	struct Decoded decoded;
	unsigned int ThreeDig;
};

struct cf_field {   /* Combined field */
       unsigned   :11;       /* unused bits */
       unsigned A :1;
       unsigned B :1;
       unsigned C :1;
       unsigned D :1;
       unsigned E :1;
};

union The_CV {
  struct cf_field cf;
  short  cf_val;
};

struct expo {
      unsigned msb_a :1;     /* interested in only the 2 most sign bits */
      unsigned msb_b :1;
      unsigned       :14;    /* who cares what they are?? */
};

union Exp { /* Exponent foramatted in bits */
   struct expo e;
   short  exp_val;
};

int Decoder_Encoder(unsigned long long *Input, unsigned long long *Output, int cno);
void Decoder(unsigned long long *Source, unsigned long long *Target, int cno, union TheUnDecoded *P2UnDec, union TheDecoded *P2Dec, union The_CV *TheCF);
void Encoder(unsigned long long *Source, unsigned long long *Target, int cno, union TheUnDecoded *P2UnDec, union TheDecoded *P2Dec, union The_CV *TheCF);
void UnPackThree(union TheUnDecoded *P2UnDec,union TheDecoded *P2Dec, int cno);
void PackThree(union TheUnDecoded *P2UnDec,union TheDecoded *P2Dec, int cno);
short Decode_CF(struct CF_Parts *CF ,unsigned short lmd, int cno, union The_CV *TheCF);/* decode the cf field */
short Encode_CF(unsigned short Exponent, unsigned short lmd, int cno, union The_CV *TheCF); /* encode the CF field */
void GenDFPOP(unsigned long long *Operand, int cno);
int GenRand(int);
short DFPGenRand(int);
int GenRange(int, int, int);
void DFPOperand(unsigned long long *Operand, int, int cno);
void DoInit(int cno);
void GenINFINITY(unsigned long long *Operand, int cno, union the_short *, union the_long *, union the_ext *);
void GenNaN(unsigned long long *Operand, int cno, union the_short *, union the_long *, union the_ext *);
void GenZERO(unsigned long long *Operand, int cno, union the_short *, union the_long *, union the_ext *);
void GenSUBNORMAL(unsigned long long *Operand, int cno, union the_short *, union the_long *, union the_ext *);
void GenLARGE(unsigned long long *Operand, int cno, union the_short *, union the_long *, union the_ext *);
void GenNORMAL(unsigned long long *Operand, int cno, union the_short *, union the_long *, union the_ext *);
void GenTINY_SMALL_SUBNORMAL(unsigned long long *Operand, int cno, union the_short *, union the_long *, union the_ext *);
void PrintLong(unsigned long long *MyPointer);
void PrintDecoded(unsigned long long *Fraction, int cno);
int CheckFinite(unsigned long long *Operand);

#define    fpscrRSRV0to28     0xFFFFFFF800000000ULL
#define    fpscrDRN           0x0000000700000000ULL

/* BitMasksLower */
#define    fpscrFX            0x0000000080000000ULL
#define    fpscrFEX           0x0000000040000000ULL
#define    fpscrVX            0x0000000020000000ULL
#define    fpscrOX            0x0000000010000000ULL
#define    fpscrUX            0x0000000008000000ULL
#define    fpscrZX            0x0000000004000000ULL
#define    fpscrXX            0x0000000002000000ULL
#define    fpscrVXSNAN        0x0000000001000000ULL
#define    fpscrVXISI         0x0000000000800000ULL
#define    fpscrVXIDI         0x0000000000400000ULL
#define    fpscrVXZDZ         0x0000000000200000ULL
#define    fpscrVXIMZ         0x0000000000100000ULL
#define    fpscrVXVC          0x0000000000080000ULL
#define    fpscrFR            0x0000000000040000ULL
#define    fpscrFI            0x0000000000020000ULL
#define    fpscrC             0x0000000000010000ULL
#define    fpscrFL            0x0000000000008000ULL
#define    fpscrFG            0x0000000000004000ULL
#define    fpscrFE            0x0000000000002000ULL
#define    fpscrFU            0x0000000000001000ULL
#define    fpscrRSRV20        0x0000000000000800ULL
#define    fpscrVXSOFT        0x0000000000000400ULL
#define    fpscrVXSQRT        0x0000000000000200ULL
#define    fpscrVXCVI         0x0000000000000100ULL
#define    fpscrVE            0x0000000000000080ULL
#define    fpscrOE            0x0000000000000040ULL
#define    fpscrUE            0x0000000000000020ULL
#define    fpscrZE            0x0000000000000010ULL
#define    fpscrXE            0x0000000000000008ULL
#define    fpscrBRN           0x0000000000000007ULL
#define    fpscrFPCC_SHIFT    12
#define    fpscrFPRF_SHIFT    12
#define    fpscrCR_SHIFT      28

/* FPSCR_BitMasks */
#define fpscrFPCC 			(fpscrFL | fpscrFG | fpscrFE | fpscrFU)
#define fpscrFPRF 			(fpscrC | fpscrFL | fpscrFG | fpscrFE | fpscrFU )
#define fpscrVX_EXCP_MASK 	(fpscrVXSNAN | fpscrVXISI | fpscrVXIDI | fpscrVXZDZ | fpscrVXIMZ | fpscrVXVC | fpscrVXSOFT | fpscrVXSQRT | fpscrVXCVI)

#define fpscrALL_EXCP_MASK (fpscrVX_EXCP_MASK | fpscrOX | fpscrUX | fpscrZX | fpscrXX)

typedef struct {
    unsigned long long FPSCR_Val;  /* Floating Point Status & Control Reg contents */
    unsigned long long FPSCR_Mask;  /* Bits of FPSCR written */
    unsigned char CR_Val;               /* Condition Register field contents */
    unsigned char CR_Mask;          /* Bits of CR field written */
} Pseries_status;

enum Pseries_DataClassMask {
   DCM_Pseries_Zero                     = 0x20,        /* Zero */
   DCM_Pseries_Subnormal                = 0x10,        /* Subnormal number */
   DCM_Pseries_Normal                   = 0x08,        /* Normal number */
   DCM_Pseries_Infinity                 = 0x04,        /* Infinity */
   DCM_Pseries_QuietNAN                 = 0x02,        /* Quiet NAN */
   DCM_Pseries_SignalingNAN             = 0x01         /* Signaling NAN */
};

/*
 Data class bit definitions for the test data group instructions.
*/
enum Pseries_DataGroupMask {
   DGM_Pseries_Zero_NotExtreme           = 0x20, /* Zero with Not Extreme Exponent */
   DGM_Pseries_Zero_Extreme              = 0x10, /* Zero with Extreme Exponent */
   /*   DGM_NormalOrSub_Extreme      = 0x08, */  /* Normal or Subnormal w/ extreme */
   DGM_Pseries_SubNormalOr_ExtremeNormal = 0x08, /* Normal or Subnormal w/ extreme */
   DGM_Pseries_Norm_NotExt_LeftZero      = 0x04, /* Normal,not extreme, Left digit=0 */
   DGM_Pseries_Norm_NotExt_LeftNotZero   = 0x02, /* Normal,not extreme, Left digit!=0*/
   DGM_Pseries_Special                   = 0x01  /* Infinity, QNan or SNan */
};


enum RMCValues {
   RMC_Nearest         = 0,   /* Round to nearst with ties to even */
   RMC_Zero            = 1,   /* Round toward zero */
   RMC_HalfUp          = 2,   /* Round to nearst with ties away from zero */
   RMC_FromEnvironment = 3    /* Use rounding mode from the environment's RN field */
};

enum RMCsecondaryValues {
   RMC_TowardPositiveInfinity        = 0,   /* Round toward positive infinity */
   RMC_TowardNegativeInfinity        = 1,   /* Round toward negative infinity */
   RMC_Up                            = 2,   /* Round away from zero */
   RMC_HalfDown                      = 3    /* Round to nearst with ties toward zero*/
};

#define DFP_SIGN_BIT 0x8000000000000000ULL

enum DFP_OperandConstants {
    ExpBits32 =         8,              /* Number of exponent bits in format */
    ExpBits64 =         10,
    ExpBits128 =        14,

    UxMin32 =           -101,           /* Unbiased Exponent Minimum values */
    UxMin64 =           -398,
    UxMin128 =          -6176,

    UxMax32 =           90,             /* Unbiased Exponent Maximum values */
    UxMax64 =           369,
    UxMax128 =          6111,

    ExpBias32 =         101,            /* Exponent biasing values */
    ExpBias64 =         398,
    ExpBias128 =        6176,

    ExpAdjust32 =       144,           /* Exponent adjustment for wrapping of overflow
                                          and underflow. The calculation is
                                          ExpAdjust = roundup[3/4 * (UxMax - UxMin)].
                                          32 bit value is our guess                   */

    ExpAdjust64 =       576,
    ExpAdjust128 =      9216,

    CoefDigits32 =      7,              /* Total number of Coefficient digits  */
    CoefDigits64 =      16,
    CoefDigits128 =     34,

    CoefDPDgroups32 =   2,              /* Number of 10-bit DPD groups */
    CoefDPDgroups64 =   5,
    CoefDPDgroups128 =  11,

    /* Packed format is layed out as: */
    /* Sign | Combination | Exponent Continuation | Coefficient Continuation */
    SignBits =          1,

    CombinationBits =   5,

    ExpContBits32 =     6,              /* Number of exponent continuation bits */
    ExpContBits64 =     8,
    ExpContBits128 =    12,

    CoefContBits32 =     20,            /* Number of coefficient continuation bits */
    CoefContBits64 =     50,
    CoefContBits128   =  110,
    CoefContBits128Hi =  CoefContBits128-64
};

#define MAX_COEFFICIENT_DIGITS CoefDigits128

enum DFP_IntermediateConstants {
    ImUxMax32 =           194,             /* Biased Exponent Maximum values */
    ImUxMax64 =           770,
    ImUxMax128 =         12290,

    ImCoefDigits32 =      8,              /* Total number of Coefficient digits  */
    ImCoefDigits64 =      17,
    ImCoefDigits128 =     35
};
extern const unsigned long long ComboMask128;
extern const unsigned long long ExpContMask128;
extern const unsigned long long CoefContMaskHi128;

extern const unsigned SignShift128;
extern const unsigned ComboShift128;
extern const unsigned ExpContShift128;

extern const unsigned long long ComboMask64;
extern const unsigned long long ExpContMask64;
extern const unsigned long long CoefContMask64;

extern const unsigned SignShift64;
extern const unsigned ComboShift64;
extern const unsigned ExpContShift64;

extern const unsigned long long ComboMask32;
extern const unsigned long long ExpContMask32;
extern const unsigned long long CoefContMask32;

extern const unsigned SignShift32;
extern const unsigned ComboShift32;
extern const unsigned ExpContShift32;

enum DFP_Type {
    Undefined_DFP_Type,
    Normal,
    QuietNAN,
    SignalingNAN,
    Infinity
} ;

enum SpecificDFPnumber {
   DFP_ZERO,
   DFP_MIN_SUB_NORM,
   DFP_MIN_NORM,
   DFP_MAX_NORM
};

enum DFP_Result_Type {
    No_Update,
    DFP_Single,
    DFP_Double,
    DFP_Quad,
    SInt_64,
    BCD_64,
    BCD_128
} ;

enum {
   false = 0,
   true = 1
};

typedef enum bv {
   ZZERO = 0,
   ONE
}bitValue;

typedef struct {
   unsigned precision;        /* number of BCD digits in the coefficient  */
   int UxMax;                 /* Max unbiased exp in this format */
   int UxMinNorm;             /* Min unbiased exp for norm numbers in this format */
   int UxMin;                 /* Min unbiased exp for subnormal numbers in this format */
   int expAdjust;
} DFP_Format;

#define MAX_NUM_UNITS 30 /* At most 80 */
#define BCD_Unit uint32         /* We chose to use int rather than short
                           to avoid  a lot of casting */

typedef struct {
   BCD_Unit Units[MAX_NUM_UNITS];
   uint NumUnits; /* It is necessary that NumUnits >= 1 */
} BCDnumber;

/* This is Cowlishaw's way to do a fast divide by powers of 10. */
 static const unsigned short multipliers[]={26215,     /* approx 2**17/5   */
                                            5243,      /* approx 2**17/25  */
                                            1049,      /* approx 2**17/125 */
                                            210};      /* approx 2**17/625 */

/* QUOT10 -- macro to return the quotient of unit u divided by 10**n.
   Not safe for u>19983 */
#define QUOT10(u, n) (n==0?u:(((unsigned)(u)>>(n))*multipliers[n-1])>>17)

/* Cowlishaw's way to do fast multiply n by 10
   Should we define shortcuts for multiply by 100, 1000, 1000 as well? */
#define TIMES10(n) (((n)<<1)+((n)<<3))

#define MOST_SIG_UNIT(bcd_num, n) \
	for (n=bcd_num->NumUnits - 1; n>0; n--) { if (bcd_num->Units[n] != 0) break; }

#define RADIX 10000
#define RADIX_MINUS1 9999 /* radix-1 */

static const BCD_Unit power10[]={1, 10, 100, 1000, 10000};
static const char DecimalDigits[] = {'0','1','2','3','4','5','6','7','8','9'};

void BCD_SetLeadingUnitsToZero(BCDnumber *BCD_num);

/* BCD_PrintAsUnits. */
/* Prints the NumUnits units of BCD_number, starting with the most
   significant, leaving one space between every two consecutive units. */
void BCD_PrintAsUnits(const BCDnumber* BCD_number);

/* BCD_PadWithZeroes */
/* Pads with zeroes starting in location M & minimizes NumUnits,
   starting in location N */
void BCD_PadWithZeroes(BCDnumber* BCD_number, uint M, uint N);

/* BCD_Copy */
/* The new BCD number is set to be equal to the original BCD number. */
void BCD_Copy(const BCDnumber* originalBCD_number,       /* input.  */
              BCDnumber *newBCD_number);                 /* output. */

/* BCD_GetNumUnits */
/* Find the number of units of BCD_number. */
int  BCD_GetNumUnits(const BCDnumber* BCD_number);       /* input.  */

/* BCD_GetNumDigits */
/* Find the number of digits of a BCD_number when it is written as a
   non-negative decimal integer. */
int  BCD_GetNumDigits(const BCDnumber* BCD_number);      /* input.  */

/* BCD_PrintString. */
/* Prints BCD_number into a char buffer, as a non-negative decimal integer.
   Commas separate triplets of consecutive decimal digits.              */
void BCD_PrintString(const BCDnumber* BCD_number,             /* input. */
                     char* result);                           /* output */

/* BCD_Print. */
/* Prints BCD_number to the standard output. Printing format is
   as specified for BCD_PrintString. */
void BCD_Print(const BCDnumber* BCD_number);             /* input. */

/* Operations on digits - digits are numbered so that lsb is 0 */

/* BCD_GetDigit */
/* Returns the digit of BCD_number in the 'index' decimal place.   */
uint BCD_GetDigit(const BCDnumber* BCD_number,           /* input. */
                  uint index);                           /* input. */

/* BCD_GetDigitAsChar */
/* Returns the character value of the digit of BCD_number in the 'index' decimal place */
char BCD_GetDigitAsChar(const BCDnumber* BCD_number,     /* input. */
                        uint index);                     /* input. */

/* BCD_SetDigit */
/* Change the numerical value of BCD_number so that its decimal digit in
   the index'th place becomes equal to 'value' and the other digits
   remain unchanged. */
void BCD_SetDigit(BCDnumber* BCD_number,                /* input & output. */
                  uint index,                           /* input. */
                  uint value);                          /* input. */

/* BCD_SetDigitToChar */
/* Same as BCD_SetDigit, but the input is the character value of the digit to set. */
void BCD_SetDigitToChar(BCDnumber* BCD_number,          /* input & output. */
                        uint index,                     /* input. */
                        char value);                    /* input. */

/* Conversion routines. */

/* BCD_FromInteger */
/* Converts a 64-bit unsigned integer to a BCD number representing the
   same integer. */
void BCD_FromInteger(unsigned long long value, BCDnumber* BCD_number);

/* BCD_FromString */
/* Converts a string of decimal digits into a BCD number representing the
   same integer. */
void BCD_FromString(const char *StringNumber,           /* input.  */
                    BCDnumber *BCD_number);             /* output. */

/* BCD_ToInteger */
/* Converts a BCD number to a 64-bit unsigned integer.
   Returns true if successful, false if the source value is too large. */
boolean BCD_ToInteger(const BCDnumber* BCD_number, unsigned long long *value);

/* BCD_ToString */
/* Converts a BCD number into a string of decimal digits representing the
   same integer. */
void BCD_ToString(const BCDnumber* BCD_number,           /* input.  */
                  char *StringNumber);            /* output. */

/* Arithmetic operations. */
/* The following is true for all four arithmetic operations routines:
   a.NumUnits and b.NumUnits should be in the interval [1,MAX_NUM_UNITS).
   a.Units[i], b.Units[i] (i=0,...,NumUnits-1) should be, each, in the
     interval [0,radixm1].
   a.Units[i], b.Units[i] (i=NumUnits,...,MAX_NUM_UNITS) are ignored.
   res->Units[i] (i=0,...,res->NumUnits-1) are, each, set to be in the
      interval [0,radixm1].
   res->Units[i] (i=res->NumUnits,...,MAX_NUM_UNITS-1) are all set to be
      zeroes.
   The same is true for the output remainder of the divide routine.

   It is acceptable for Output and input pointers to point to the same address. */

/* Set value functions: */
void BCD_SetZero(BCDnumber *num);
void BCD_SetOne(BCDnumber *num);
void BCD_SetNines(uint numDigits, BCDnumber *num);

void BCD_AddOne(BCDnumber *num);
void BCD_SubOne(BCDnumber *num);

/* BCD_IsZero */
/* returns true if the number is zero, false otherwise */
boolean BCD_IsZero(const BCDnumber* num);

/* BCD_CountTrailingZeros */
/* returns the number of trailing zeros, or -1 if the entire BCD number is zero */
int BCD_CountTrailingZeros(const BCDnumber* num);

/* BCD_CountTrailingNines */
/* returns the number of trailing nines, or -1 if the BCD number is all nines */
int BCD_CountTrailingNines(const BCDnumber* num);

/* BCD_Add */
/* Computes a+b and stores it in res. The result is exact. */
void BCD_Add(const BCDnumber* a, const BCDnumber* b, BCDnumber *result);

/* BCD_Subtract */
/* Computes a-b and stores it in res. The result is exact. */
/* It is the responsibility of the user to set a >= b. */
void BCD_Subtract(const BCDnumber* a, const BCDnumber* b, BCDnumber *result);

/* BCD_Multiply */
/* Computes a*b and stores it in res. The result is exact. */
/* It is the responsibility of the user to insure that
   a.NumUnits+b.NumUnits <= MAX_NUM_UNITS. */
void BCD_Multiply(const BCDnumber* a, const BCDnumber* b, BCDnumber *result);

/* BCD_Divide */
/* Computes the integral part of a/b and stores it in res.
   The remainder is stored in remainder. The results are exact. */
void BCD_Divide(const BCDnumber* a, const BCDnumber* b,
                BCDnumber *result, BCDnumber *remainder);

/* BCD_Compare */
/* Compares the sizes of a&b. Returns -1 if a<b, 0 if a=b, 1 if a>b. */
int BCD_Compare(const BCDnumber* a,                      /* input.  */
                const BCDnumber* b);                      /* input.  */

/* Decimal shifts. */

/* BCD_ShiftLeft */
void BCD_ShiftLeft(const BCDnumber* input,               /* input.  */
                   uint shiftAmount,                    /* input.  */
                   BCDnumber *output);                  /* output. */

/* BCD_ShiftRight */
void BCD_ShiftRight(const BCDnumber* input,              /* input.  */
                    uint shiftAmount,                   /* input.  */
                    BCDnumber *output);                 /* output. */

/* BCD_ShiftRightOr - for sticky bit calculations. */
/* Effect is the same as BCD_ShiftRight, except for the least significant digit: */
/* if all the digits shifted out, and also the lsd itself, are zeros: lsd = 0    */
/* otherwise: lsd = 1                                                            */
void BCD_ShiftRightOR(const BCDnumber* input,             /* input.  */
                      uint shiftAmount,                   /* input.  */
                      BCDnumber *output);                 /* output. */

/* BCD_GetLowerDigits */
/* Limits the number of nonzero digits to "maxDigits", by setting all digits */
/* beyond them to zero.                                                      */
void BCD_GetLowerDigits(const BCDnumber* input,               /* input   */
                        uint maxDigits,                       /* input   */
                        BCDnumber* output);                   /* output  */

/******************************************************************************/

typedef struct {
   DFP_Format format;
   enum DFP_Type type;
   bitValue sign;
   int exponent;                /* unbiased exponent */
   union {
       BCDnumber bcdnumber;                     /* using HRL's BCD package */
       unsigned long long NANinfinity[2];       /* binary form for NANs & Infinity */
   } coefficient;
} DFP_Operand;

/*
DFP_Intermediate  - this structure is very similar to that of DFP_Operand, with a few additions. Since the DFP_Operand struct
ure can have any number of digits (not limited to particular formats), we do not need to differentiate between guard digits a
nd regular digits. In this implementation we also do not need to differentiate between regular intermediate results and micro
-architecture intermediate results.
*/


typedef struct {
   DFP_Operand decimalNumber;
   bitValue    stickyBit;
   bitValue    roundedBit;
} DFP_Intermediate;

/************************************************************************************/

/* DFP_Format basic primitives */
void DFP_SetFormat(unsigned precision, int UxMax, int expAdjust,
                   DFP_Format* format);
void DFP_CopyFormat(const DFP_Format* origFormat, DFP_Format* newFormat);

/* DFP_Operand basic primitives */
void DFP_InitOperand(const DFP_Format* format, DFP_Operand* operand);

/* DFP_SetValueOperand assumes the operand type is Normal */
void DFP_SetValueOperand(enum DFP_Type type, bitValue sign, int exponent,
                         BCDnumber* coefficient,
                         DFP_Operand* operand);
void DFP_CopyOperand(const DFP_Operand* origOperand,
                     DFP_Operand* newOperand);

void DFP_CopyOperandValue(const DFP_Operand* origOperand,
                     DFP_Operand* newOperand);
void DFP_PrintOperand(const DFP_Operand *operand);
void DFP_PrintOperandString(char *string, const DFP_Operand* operand);
void DFP_NegateOperand(DFP_Operand *operand);
long long int DFP_ExponentBias(const DFP_Operand *operand);

/* DFP_Operand specific values primitives */
/* Operand Set-value functions: all these functions set positive constants */
void DFP_OpSetInfinity(DFP_Operand *operand);
void DFP_OpSetDefaultQNaN(DFP_Operand *operand);
void DFP_OpSetSpecificNumber(DFP_Operand *op,                  /* output */
                             const DFP_Format *format,         /* input */
                             enum SpecificDFPnumber specific); /* input */

/*
Primitives for determining the type of a DFP_Operand.
*/
int DFP_OpIsNAN
(const DFP_Operand *Op1                     /* input */
);
/*
PURPOSE:  Determines if input Op1 is "not a number", either
          a signaling or quiet NAN.
REQUIRES: Op1 of data type DFP_Operand
RETURNS:  +1 if Op1 is "signaling not a number"
          0 if Op1 is not a NAN
          -1 if Op1 is "quiet not a number"
EXCEPTIONS: none
*/

unsigned DFP_OpIsQNAN
(const DFP_Operand *Op1                     /* input */
);
/*
PURPOSE:  Determines if input Op1 is "quiet not a number"
REQUIRES: Op1 of data type DFP_Operand
RETURNS:  1 if Op1 is a QNAN
          0 if Op1 is not a QNAN
EXCEPTIONS: none
*/

unsigned DFP_OpIsSNAN
(const DFP_Operand *Op1                     /* input */
);
/*
PURPOSE:  Determines if input Op1 is "signaling not a number"
REQUIRES: Op1 of data type DFP_Operand
RETURNS:  1 if Op1 is a SNAN
          0 if Op1 is not a SNAN
EXCEPTIONS: none
*/


int DFP_OpIsInfinity
(const DFP_Operand *Op1                     /* input */
);
/*
PURPOSE:  Determines if input Op1 is plus or minus infinity or not
          infinity..
REQUIRES: Op1 of data type DFP_Operand
RETURNS:  +1 if Op1 is "Plus infinity"
          0 if Op1 is not infinity
          -1 if Op1 is minus infinity
EXCEPTIONS: none
*/


boolean DFP_OpIsZero
(const DFP_Operand *Op1                     /* input */
);
/*
PURPOSE:  Determines if input Op1 is zero
REQUIRES: Op1 of data type DFP_Operand
RETURNS:  +1 if Op1 is zero
          0 if Op1 is not zero
EXCEPTIONS: none
*/


boolean DFP_OpIsSubnormal
(const DFP_Operand *Op                      /* input */
);
/*
PURPOSE:  Determines if input Op is a subnormal number.
REQUIRES: Op1 of data type DFP_Operand
RETURNS:  1 if Op is a subnormal
          0 if Op is not a subnormal
EXCEPTIONS: none
*/


boolean DFP_OpIsNormal
(const DFP_Operand *Op                          /* input */
);
/*
PURPOSE:  Determines if input Op is a normalized number
REQUIRES: Op of data type DFP_Operand
RETURNS:  1 if Op is a normal
          0 if Op is not normal
EXCEPTIONS: none
*/

/* Is-value functions: all these functions return true both for negative and for positive
   values */
int DFP_OpIsMinNorm(const DFP_Operand *operand);
int DFP_OpIsMaxNorm(const DFP_Operand *operand);
int DFP_OpIsMinSubNorm(const DFP_Operand *operand);
int DFP_OpIsExtremeExponent(const DFP_Operand *operand);

/* Additional DFP_Operand functions */
/* DFP_Normalize - moves the msd of the coefficient to the leftmost position possible for
   the given precision. The exponent is adjusted accordingly. This function is used
   by DFP_CompareOperands.
   Warning - the number created by this function is not necessarily a legal DFP number -
   it might be bigger than max-norm. */
void DFP_Normalize(const DFP_Operand *inputOp,
                   DFP_Operand *normalizedOp);

/* DFP_CompareOperands - compares Op1 and Op2 by magnitude.
   Returns  1 if Op1 > Op2
            0 if Op1 = Op2
           -1 if Op1 < Op2
   Note that +0 == -0, as defined in for the Compare instruction in the architecture. */
int DFP_CompareOperands(const DFP_Operand *Op1,
                        const DFP_Operand *Op2);

/* DFP_CompareOperands - compares Op1 and Op2 by magnitude, disregarding their
   signs.
   Returns  1 if Op1 > Op2
            0 if Op1 = Op2
           -1 if Op1 < Op2
*/
int DFP_CompareOperandsByAbsValue(const DFP_Operand *Op1,
                                  const DFP_Operand *Op2);

/* DFP_CompareCoefficient normalizes the numbers and compares the values of their
   coefficients.
   The exponents are ignored. E.g., 13E5 < 6E0 */
int DFP_CompareCoefficients(const DFP_Operand *operand1,
                            const DFP_Operand *operand2);

/* DFP_Intermediate basic primitives */

/* returns true if the im is NaN or infinity */
int DFP_ImIsSpecial(const DFP_Intermediate* im);
int DFP_ImIsPositive(const DFP_Intermediate* im);

/* DFP_SetValueOperand assumes the operand type is Normal */
void DFP_SetValueIntermediate(enum DFP_Type type, bitValue sign, int exponent,
                              BCDnumber* coefficient,
                              bitValue stickyBit,  bitValue roundedBit,
                              DFP_Intermediate* im);
void DFP_CopyIntermediate(const DFP_Intermediate* origIm,
                          DFP_Intermediate* newIm);
void DFP_PrintIntermediate(const DFP_Intermediate* im);

void DFP_OperandToIntermediate(const DFP_Operand *op,
                               DFP_Intermediate *im);
int DFP_CompareImByAbsValue(const DFP_Intermediate* im1,
                            const DFP_Intermediate* im2);
int DFP_CompareIm(const DFP_Intermediate* im1,
                  const DFP_Intermediate* im2);
void DFP_NegateIntermediate(DFP_Intermediate *im);

/* DFP_Operand specific values primitives */
/* Operand Set-value functions: all these functions set positive constants */

/* DFP_ImSetSpecificNumber: set a specific number in im.
   specific - indicated the number to set, e.g., MAX_NORM, etc.
   opFormat - is the format of the suitable operand */
void DFP_ImSetSpecificNumber(DFP_Intermediate *im,             /* output */
                             const DFP_Format *opFormat,       /* input */
                             enum SpecificDFPnumber specific); /* input */

/* Converts a BCDnumber with any number of digits to a DFP_IntermediateResult, whose
   format, sign, and exponent have been already defined. The coefficient of intermResult
   is set, and the exponent may have to be adjusted if there is any shift */

void DFP_convertWideResultToIm(const BCDnumber *wideResult,       /* input */
                               DFP_Intermediate *intermResult,    /* input/output */
                               int imExtraDigits) ;               /* input */

#ifndef dfpMIN
#define dfpMIN(a, b) ((int)(a) < (int)(b) ? (a) : (b))
#endif
#ifndef dfpMAX
#define dfpMAX(a, b) ((int)(a) > (int)(b) ? (a) : (b))
#endif

#define MIN_LONG_LONG_INT 0x8000000000000000ULL
#define MAX_LONG_LONG_INT 0x7fffffffffffffffULL

#define DFP_INT_MAX 0x7fffffff

#define SET_STATUS_BIT(statusPtr, bit)     *((int*)(statusPtr)) |= (bit);
#define CLEAR_STATUS_BIT(statusPtr, bit)   *((int*)(statusPtr)) &= ~(bit);

/*
 DFP_Status is updated as the DFP operation is executed and is used to determine
 how to set the FPSCR/FPC registers ultimately in the simulation environment.
 NOTES:
 - FR/FI bits are to be updated in the final result only if dfpFI_FR_Update =1
 - all FPCC bits will be updated if any of them has been set
 */
enum DFP_Status {
    dfpTargetUpdate= (int)0x80000000,       /* Target FPR was written */

    dfpGen_QNaN         = 0x00800000,       /* Generate a default QNaN */
    dfpFI_FR_Update     = 0x00400000,       /* Update FI and FR bits */
    dfpC_Update         = 0x00200000,       /* Update C bit */
    dfpFPCC_Update      = 0x00100000,       /* Update FPCC field */

    dfpFR               = 0x00040000,       /* rounded bit value */
    dfpFI               = 0x00020000,       /* inexact bit value */
    dfpC                = 0x00010000,       /* Class */

    dfpFL               = 0x00008000,       /* Less than or < */
    dfpFG               = 0x00004000,       /* Greater than or > */
    dfpFE               = 0x00002000,       /* Equal or = */
    dfpFU               = 0x00001000,       /* Unordered or ? */

    dfpOX               = 0x00000800,       /* Overflow */
    dfpUX               = 0x00000400,       /* Underflow */
    dfpXX               = 0x00000200,       /* Inexact */
    dfpZX               = 0x00000100,       /* zero divide */

    dfpVXSNAN           = 0x00000080,       /* Inv op Signalling NAN */
    dfpVXISI            = 0x00000040,       /* Inv Op inf minus inf */
    dfpVXIDI            = 0x00000020,       /* Inv Op inf div inf */
    dfpVXZDZ            = 0x00000010,       /* Inv Op zero div zero */

    dfpVXIMZ            = 0x00000008,       /* Inv Op inf mult zero */
    dfpVXVC             = 0x00000004,       /* Inv Op invalid cmpr */
    dfpVXSQRT           = 0x00000002,       /* Inv Op Sq root */
    dfpVXCVI            = 0x00000001,       /* Inv Op integer cnvrt */

    dfpVX_EXCEPT_MASK   = 0x000000FF,       /* Invalid Op exceptions mask */

    dfpFPCC_MASK        = dfpFL | dfpFG | dfpFE | dfpFU | dfpFPCC_Update,
                                            /* Condition code update mask */
    dfpFPCC_SHIFT       = 12,               /* Right shift count to isolate FPCC */
    /*
    Result Mask: Setting any of these bits will cause the FPRF field to be
    written in the status register.
    */
    dfpFPRF_MASK        = 0x0001F000
};

enum ExtractEncode {
    DFP_Extract_finite   = 0,
    DFP_Extract_infinite = -1,
    DFP_Extract_qnan     = -2,
    DFP_Extract_snan     = -3

};

typedef enum {
    RoundToNearest              = 0,
    RoundTowardZero             = 1,
    RoundTowardPositiveInfinity = 2,
    RoundTowardNegativeInfinity = 3,
    RoundHalfUp                 = 4,
    RoundHalfDown               = 5,
    RoundUp                     = 6,
    RoundForReround             = 7
} RoundingMode;

#define DFP_ROUNDING_MODE_COUNT 8

/* This vector maps RoundingModes to their string values for easy printing */

#if 0
const char RoundingModeStrings[][28] =
{
   "RoundToNearest",
   "RoundTowardZero",
   "RoundTowardPositiveInfinity",
   "RoundTowardNegativeInfinity",
   "RoundHalfUp",
   "RoundHalfDown",
   "RoundUp",
   "RoundForShorterPrecision"
};
#endif



typedef struct {
    unsigned InvalidOperation   : 1;    /* VE */
    unsigned Overflow           : 1;    /* OE */
    unsigned Underflow          : 1;    /* UE */
    unsigned ZeroDivide         : 1;    /* ZE */
    unsigned Inexact            : 1;    /* XE */
} ExceptionEnableBits;

enum FPRF_Settings {
    QNaN                        = ( dfpFU | dfpC  | dfpC_Update),
    SNaN                        = ( dfpFU         | dfpC_Update),
    NegativeInfinity            = ( dfpFL | dfpFU | dfpC_Update),
    NegativeNormalized          = ( dfpFL         | dfpC_Update),
    NegativeSubnormal           = ( dfpFL | dfpC  | dfpC_Update),
    NegativeZero                = ( dfpFE | dfpC  | dfpC_Update),
    PositiveZero                = ( dfpFE         | dfpC_Update),
    PositiveSubnormal           = ( dfpFG | dfpC  | dfpC_Update),
    PositiveNormalized          = ( dfpFG         | dfpC_Update),
    PositiveInfinity            = ( dfpFG | dfpFU | dfpC_Update)
};

enum DFP_Op_Type {
    Undefined_Operation,
    Arithmetic,
    Compare,
    Convert,
    CopySign,
    Decode,
    Encode,
    ExtractExponent,
    InsertExponent,
    Reround,
    Round,
    RoundFPinteger,
    Shift,
    TestDataClass,
    TestDataGroup,
    TestExponent,
    TestSignificance,
    Quantize,
    Load
} ;

enum DFP_Arith_Op_Type {
    Undefined_Arith_Op,
    Add_Op,
    Multiply_Op,
    Divide_Op
} ;

/*
 System for which the results are being calculated.  The status and control
 registers are different format for the systems.
*/
enum DFP_System {
    Pseries = 1,
    Zseries
} ;

/* User directives */
typedef struct
{
    boolean FPSCR_64_Bit;
} DFP_User_Directives;

/*
Environment specifies information whose source varies depending on
the system being simulated, but is still common.
*/
typedef struct {
    RoundingMode   roundingMode;           /* Decimal FP rounding mode */
    ExceptionEnableBits exceptionEnableBits;    /* FP exception enabled bits. */
    enum DFP_System     system;                 /* System targeted for results */
    enum DFP_Op_Type    op_type;                /* type of op being performed */
} Environment;

/* Structures related to coverage metrics */

typedef enum {
   Undefined_DFP_DataType,
   PosZero,
   NegZero,
   PosSubnormal,
   NegSubnormal,
   PosNormal,
   NegNormal,
   PosInfinity,
   NegInfinity,
   PosQNaN,
   NegQNaN,
   PosSNaN,
   NegSNaN
} DFP_DataType;

#define DFP_DATATYPE_COUNT 13

/* This vector maps DFP_DataTypes to their string values for easy printing */

#if 0
const char DFP_DataTypeStrings[][13] =
{
   "Undefined",
   "PosZero",
   "NegZero",
   "PosSubnormal",
   "NegSubnormal",
   "PosNormal",
   "NegNormal",
   "PosInfinity",
   "NegInfinity",
   "PosQNaN",
   "NegQNaN",
   "PosSNaN",
   "NegSNaN"
};

#endif


/* The following struct represents the lines of part 1 of the "Rounding and Range
   Actions" table */
typedef enum {
   vHuge_qHuge,
   vHuge_qNmax,
   vNormal,
   vSubnormal,
   vMicroUpperHalf,
   vMicroHalfDmin,
   vMicroLowerHalf,
   vZero,
   vExactZeroDiff,
   vUndefined   /* also used for non-numeric intermediate (exact infinity or NaN) */
} AbsResultRange;

#define ABS_RESULT_RANGE_COUNT 10

/* match codes for Test instructions */
typedef enum {
   TST_NoMatch,
} DFP_MatchCode;

#define DFP_MATCH_CODE_COUNT 2

/* CaseType distinguishes between different classes of rounding cases.
   The "Other" class stands for exact zero, exact infinity, NaN, or no result. */
typedef enum {CTYPE_Overflow, CTYPE_Normal, CTYPE_Tiny, CTYPE_Other} CaseType;

typedef struct {
   AbsResultRange range;
   bitValue sign;
} ResultRange;

/* The following definition serves to locate the case within part 2 of the "Rounding
   and Range Actions" table. */
typedef struct {
   boolean rInexact,
        rIncremented,
        qInexact,
        qIncremented;
} RoundingCase;

/* this enum must be able to distinguish between every two instruction of the
   pSeries OR zSeries architecture, as relevant.  There is no need to distinguish between
   the zSeries and pSeries versions of an instruction when they vary (e.g. Compare, etc.)
*/
typedef enum {
   AddDouble,
   AddQuad,
   SubtractDouble,
   SubtractQuad,
   MultiplyDouble,
   MultiplyQuad,
   DivideDouble,
   DivideQuad,
   CompareDouble,
   CompareQuad,
   CompareSignalDouble,
   CompareSignalQuad,
   ConvertFromFixedDouble,
   ConvertFromFixedQuad,
   /* for pSeries, "Convert From BCD" is split into two types - one for signed and
      one for unsigned conversion */
   ConvertFromSignedBCDDouble,
   ConvertFromSignedBCDQuad,
   ConvertFromUnsignedBCDDouble,
   ConvertFromUnsignedBCDQuad,
   ConvertToFixedDouble,
   ConvertToFixedQuad,
   ConvertToSignedBCDDouble,
   ConvertToSignedBCDQuad,
   ConvertToUnsignedBCDDouble,
   ConvertToUnsignedBCDQuad,
   ExtractExponentDouble,
   ExtractExponentQuad,
   InsertExponentDouble,
   InsertExponentQuad,
   LoadAndTestDouble,
   LoadAndTestQuad,
   RoundFPIntegerDouble,
   RoundFPIntegerQuad,
   RoundFPIntegerNoInexactDouble,
   RoundFPIntegerNoInexactQuad,
   ConvertSingleToDouble,
   ConvertDoubleToQuad,
   RoundQuadToDouble,
   RoundDoubleToSingle,
   QuantizeDouble,
   QuantizeQuad,
   QuantizeImmediateDouble,
   QuantizeImmediateQuad,
   ReroundDouble,
   ReroundQuad,
   ShiftCoeffLeftDouble,
   ShiftCoeffLeftQuad,
   ShiftCoeffRightDouble,
   ShiftCoeffRightQuad,
   TestDataClassSingle,
   TestDataClassDouble,
   TestDataClassQuad,
   TestDataGroupSingle,
   TestDataGroupDouble,
   TestDataGroupQuad,
   TestExponentDouble,
   TestExponentQuad,
   TestSignificanceDouble,
   TestSignificanceQuad
} InstType;

#define DFP_INST_TYPE_COUNT 62

typedef char Standard_CovMetrics;  /* dummy, used when no extra coverage info needed */

typedef struct {
   boolean isMaxOrMin;  /* set when the result is MAX_LONG_LONG_INT or MIN_LONG_LONG_INT */
} ConvertToFixed_CovMetrics;
typedef struct {
   long long exp;
   long long MBE;  /* max. legal biased exponent */
} InsertExp_CovMetrics;

typedef struct {
   boolean srcPrecision;  /* set to 1 on enabled overflow or underflow */
} RoundTo_CovMetrics;

typedef enum {
   QUA_ExpCmp_Decrease,
   QUA_ExpCmp_Equal,
   QUA_ExpCmp_Increase,
   QUA_ExpCmp_Other
} QuantizeExpCmpStatus;

typedef struct {
   QuantizeExpCmpStatus expCmp;
} Quantize_CovMetrics;

typedef enum {
   RRND_PrecCmp_Decrease,
   RRND_PrecCmp_Equal,
   RRND_PrecCmp_Increase,
   RRND_PrecCmp_Unlimited,
   RRND_PrecCmp_Other
} ReroundPrecCmpStatus;

typedef struct {
   ReroundPrecCmpStatus precCmp;
} Reround_CovMetrics;

typedef struct {
   int dgm;
} TestDataGroup_CovMetrics;

typedef struct {
   int refSignificance;
} TestSignificance_CovMetrics;

typedef struct {
   InstType instType;
   union {
      Standard_CovMetrics standard;
      ConvertToFixed_CovMetrics convertToFixed;
      InsertExp_CovMetrics insertExp;
      RoundTo_CovMetrics roundTo;
      Quantize_CovMetrics quantize;
      Reround_CovMetrics reround;
      TestDataGroup_CovMetrics testDataGroup;
      TestSignificance_CovMetrics testSignificance;
   } inst;
} InstructionCoverageMetrics;

typedef struct {
   DFP_DataType input1Type, input2Type, resultType;
   ResultRange resultRange;
   RoundingCase roundingCase;
   Environment environment;
   ExceptionEnableBits exceptionSignals;
   enum DFP_Status dfp_status;
   InstructionCoverageMetrics instCoverageMetrics;
} CoverageMetrics;

/*
DFP_System_Status: this structure will hold the status and control register
facilities that are unique to each system.
*/
typedef struct
 {
    union {
        Pseries_status    pstatus;        /* P-series status */
    } status;
    enum DFP_System       system;         /* System for which results are targeted */
    char                  record;         /* record condition code flag */
    enum DFP_Result_Type  FPR_updated;    /* Flag for target FPR updated by inst */
    /* The next structure is reserved for special user directives. This is not
       actually part of the architecture. The function DFP_UserDirectivesInit
       must be called whenever construction a new DFP_System_Status */
    DFP_User_Directives user_directives;
    /* coverage information for the current instruction */
    CoverageMetrics coverage_metrics;
} DFP_System_Status ;

struct dfp_context {
	unsigned long long FRA[2];       /* input operand A value (considering extended) */
	unsigned long long FRB[2];       /* input operand B value (considering extended) */
	unsigned long long FRT[2];       /* target operand value (considering extended) */
	DFP_System_Status *Ptr2DFP_System_Status;

	int UsesOpA;
	int UsesOpB;
	int UsesOpT;
	int isQuad;
};

typedef enum {
   INTERMED_EXACT,       /* all digits shifted out during Truncate are zeros */
   INTERMED_BELOW_MIDWAY,/* shifted digits' value is strictly between EXACT and MIDWAY */
   INTERMED_MIDWAY,      /* most significant shifted digit is 5, all others zeros */
   INTERMED_ABOVE_MIDWAY /* shifted digits' value strictly higher than the MIDWAY case */
} IntermedShiftedDigitsStatus;

/*************************************************************************/

/* Data Class macros */
#define DCM_SUBNORMAL(system) ( DCM_Pseries_Subnormal)

#define DCM_ZERO(system) ( DCM_Pseries_Zero)

#define DCM_NORMAL(system) ( DCM_Pseries_Normal)

#define DCM_INFINITY(system) (DCM_Pseries_Infinity)

#define DCM_QUIETNAN(system) (DCM_Pseries_QuietNAN)

#define DCM_SIGNALLINGNAN(system) ( DCM_Pseries_SignalingNAN)

/* Data Group macros */

#define DGM_ZERO_EXTREME(system) ( DGM_Pseries_Zero_Extreme)

#define DGM_ZERO_NOT_EXTREME(system) (DGM_Pseries_Zero_NotExtreme)

#define DGM_SUBNORMAL_OR_EXTREME_NORMAL(system) (DGM_Pseries_SubNormalOr_ExtremeNormal)

#define DGM_NORM_NOTEXT_LEFTNOTZERO(system) (DGM_Pseries_Norm_NotExt_LeftNotZero)

#define DGM_NORM_NOTEXT_LEFTZERO(system)  (DGM_Pseries_Norm_NotExt_LeftZero)

#define DGM_SPECIAL(system) ( DGM_Pseries_Special)


#define ADJUST_SINGLE_FOR_PACK(single, system) ( single)

#define ADJUST_SINGLE_FOR_UNPACK(single, system) ( single)

/* the following function serves the DFP_Test_Data_Class and DFP_Test_Data_Group
   functions to adjust the return values according to the sign of the input operand, and
   depending if this is P or Z series */
void DFP_DataClassSetStatusFromSign
(bitValue               sign,                /* input */
 enum DFP_System        system,              /* input */
 int                    input_code,          /* input of the data class/group operation*/
 int                    *output_code,        /* input/output - either dcm or dgm */
 enum DFP_Status        *dfp_status);        /* input/output */

void DFP_ConvertToFixedUpdateStatus
(enum DFP_System        system,              /* input */
 const DFP_Operand      *Op1,                /* input */
 enum DFP_Status        *dfp_status);        /* input/output */


void DFP_InvalidEncodeUpdateStatus
(Environment            *environment,       /* input */
 enum DFP_Status        *dfp_status,        /* input/output */
 DFP_Operand            *FRT,               /* output */
 DFP_System_Status      *dfp_sys_status);   /* system status   --input-output */

void DFP_TestSignificanceUpdateStatus
(enum DFP_System        system,              /* input */
 unsigned               Actual,              /* input */
 unsigned               Ref,                 /* input */
 enum DFP_Status        *dfp_status);        /* input/output */


/***************************************************************************/

void DFP_Round(const DFP_Intermediate* im,           /* input  */
               const Environment* environment,       /* input  */
               DFP_Operand* result,                  /* output */
               enum DFP_Status* status,              /* output */
               RoundingCase* roundingCase,           /* output */
               int* adjust);                         /* output */

boolean DFP_RoundZero(const DFP_Intermediate* im,
                   const Environment* envir,
                   DFP_Operand* operand,
                   enum DFP_Status* status,
                   RoundingCase* roundingCase);

void DFP_RoundZeroWithSticky(const Environment*      environment,   /* input */
                             const DFP_Intermediate* intermResult,  /* input */
                             DFP_Operand*            finalResult,   /* output */
                             enum DFP_Status*        dfp_status,    /* input / output */
                             RoundingCase* roundingCase);           /* output */

boolean RoundToNearest_Direction(const DFP_Operand* tempResult,
                              const Environment* envir,
                              IntermedShiftedDigitsStatus shiftedDigitsStat);

boolean RoundForReround_Direction(const DFP_Operand* tempResult,
                               IntermedShiftedDigitsStatus shiftedDigitsStat);

IntermedShiftedDigitsStatus Truncate(const DFP_Intermediate* im, boolean roundToException,
                                     DFP_Operand *operand);

/* Checks the intermediate result for underflow exceptions.
   Returns true if the intermediate result is smaller than MinNorm. */
void HandleUnderflow(DFP_Intermediate* internalIm,
                     const DFP_Format* opFormat,
                     const Environment* envir,
                     IntermedShiftedDigitsStatus shiftedDigitsStat,
                     boolean* roundToException,
                     enum DFP_Status* status,
                     RoundingCase* roundingCase,
                     int* adjust);

/* Handles rounded results that overflow.*/
void HandleOverflow(DFP_Operand* tempResult,
                    const Environment* envir,
                    DFP_Intermediate* internalIm,
                     boolean* roundToException,
                    enum DFP_Status* status,
                    RoundingCase* roundingCase,
                    int* adjust);

void Clamp(DFP_Operand* result);

/* changes the sign of internal Im to positive. If the sign was changed then the
environment will be change accordingly, and the function will return true, otherwise,
it will return false. */
boolean DFP2Positive(DFP_Intermediate* internalIm,
                  RoundingMode* roundingMode);

/*
This is a low-level Decimal FP add routine which will be called by a higher level
add function.
*/
void DFP_Add
(const Environment      *environment,                /* input */
 const DFP_Operand      *Op1,                        /* input */
 const DFP_Operand      *Op2,                        /* input */
 DFP_Operand            *finalResult,                /* output */
 enum DFP_Status        *dfp_status,                 /* output */
 RoundingCase           *roundingCase,               /* output */
 DFP_Intermediate       *intermResult                /* output */
);
/*
PURPOSE:  Adds two DFP_Operand operands and updates the
          finalResult, dfp_status and (optional) intermResult parameters
REQUIRES: -Op1 and Op2 of data type DFP_Operand
          -environment: contains rounding mode
RETURNS:  Updated values of:
          - finalResult = Op1 + Op2
          - dfp_status = status and exceptions
          - intermResult
EXCEPTIONS: Decimal FP exceptions are returned in dfp_status
*/

/*
This is a Decimal FP subtract routine with an interface identical to the low-level
 FP add.  It is not used by the higher level subtract functions of the DFP reference
  model, but is needed for a complete arbitrary-precision interface.
*/
void DFP_Subtract
(const Environment      *environment,           /* input */
 const DFP_Operand      *Op1,                   /* input */
 const DFP_Operand      *Op2,                   /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase,          /* output */
 DFP_Intermediate       *intermResult           /* output */
);
/*
PURPOSE:  Subtracts two DFP_Operand operands and updates the
          finalResult, dfp_status and (optional) intermResult parameters
REQUIRES: -Op1 and Op2 of data type DFP_Operand
          -environment: contains rounding mode
RETURNS:  Updated values of:
          - finalResult = Op1 - Op2
          - dfp_status = status and exceptions
          - intermResult
EXCEPTIONS: Decimal FP exceptions are returned in dfp_status
*/

/* this is a sub-routine of DFP_Add */
void DFP_ConstructImFromPreRoundingOp(const DFP_Operand *preRoundingOp,     /* input */
                                      DFP_Intermediate  *intermResult) ;     /* output */

/* Handles infinity operands.
   Returns true  if at least one operand was infinite,
           false otherwise */
boolean DFP_AddSpecialNumbers
(
 const Environment              *environment,           /* input */
 const DFP_Operand              *Op1,                   /* input */
 const DFP_Operand              *Op2,                   /* input */
 DFP_Operand                    *finalResult,           /* output */
 enum DFP_Status                *dfp_status             /* output */
 );

void DFP_Multiply
(const Environment      *environment,           /* input */
 const DFP_Operand      *Op1,                   /* input */
 const DFP_Operand      *Op2,                   /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase,          /* output */
 DFP_Intermediate       *intermResult           /* output */
);
/*
PURPOSE:  Multiplies two DFP_Operand operands and updates the
          finalResult, dfp_status and (optional) intermResult parameters
REQUIRES: Op1 and Op2 of data type DFP_Operand
RETURNS:  Updated values of:
          - finalResult = Op1 * Op2
          - dfp_status = status and exceptions
          - intermResult
EXCEPTIONS: Decimal FP exceptions (as enabled by environmant) are returned in
            dfp_status
*/

/* Handles infinity operands.
   Returns true  if at least one operand was infinite,
           false otherwise */
boolean DFP_MultiplySpecialNumbers
(
 const Environment              *environment,           /* input */
 const DFP_Operand              *Op1,                   /* input */
 const DFP_Operand              *Op2,                   /* input */
 DFP_Operand                    *finalResult,           /* output */
 enum DFP_Status                *dfp_status             /* output */
 );

void DFP_Divide(const Environment      *environment,           /* input */
                const DFP_Operand      *Op1,                   /* input */
                const DFP_Operand      *Op2,                   /* input */
                DFP_Operand            *finalResult,           /* output */
                enum DFP_Status        *dfp_status,            /* output */
                RoundingCase           *roundingCase,          /* output */
                DFP_Intermediate       *intermResult);         /* output */

/* Handles infinity and 0 operands.
   Returns true  if at least one operand was infinite or 0,
           false otherwise */
boolean DFP_DivideSpecialNumbers
(
 const Environment              *environment,           /* input */
 const DFP_Operand              *Op1,                   /* input */
 const DFP_Operand              *Op2,                   /* input */
 DFP_Operand                    *finalResult,           /* output */
 enum DFP_Status                *dfp_status,            /* output */
 RoundingCase                   *roundingCase           /* output */
 );

void DFP_Compare
(const Environment      *environment,                 /* input */
 const unsigned         Ordered,                      /* input */
 const DFP_Operand      *Op1,                         /* input */
 const DFP_Operand      *Op2,                         /* input */
 enum DFP_Status        *dfp_status,                  /* input / output */
 RoundingCase           *roundingCase,
 DFP_Intermediate       *intermResult)                /* input / output */
;
/*
PURPOSE:  Compares two DFP_Operand operands and updates the
          dfp_status with the results.
REQUIRES: - environment - specifying ExceptionEnableBits
          - Ordered flag - if TRUE then VXVC set for NAN/SNAN inputs
          - Op1 and Op2 - of data type DFP_Operand
RETURNS:  Updated value of dfp_status
EXCEPTIONS: Decimal FP exceptions are returned in dfp_status
*/


void DFP_Status_Adjust_Compare
(enum DFP_Status        *dfp_status                     /* input / output */
);
/*
PURPOSE:  Clears bits set in DFP_Status by the DFP_Add (which performs the
          compare) which are not updated in the final status registers.
REQUIRES: DFP_Status input
RETURNS:  - DFP_Status cleared of exception and update bits that are not correct
            for the compare operation
EXCEPTIONS: None
*/

void DFP_Test_Data_Class
(const Environment              *environment,   /* input */
 const unsigned                 DCM,            /* input */
 const DFP_Operand              *Op1,           /* input */
 enum DFP_Status                *dfp_status,    /* output */
 RoundingCase                   *roundingCase)  /* output */
;
/*
PURPOSE:    Sets FPCC in dfp_status with the state of Op1
REQUIRES:   Op1 of type DFP_Operand
RETURNS:    Updated value of dfp_status FPCC bits
EXCEPTIONS: None
*/


void DFP_Test_Data_Group
(const Environment              *environment,   /* input */
 const unsigned                 DGM,            /* input */
 const DFP_Operand              *Op1,           /* input */
 enum DFP_Status                *dfp_status,    /* output */
 unsigned                       *op1DataGroup,  /* output */
 RoundingCase                   *roundingCase)  /* output */
;
/*
PURPOSE:    Sets FPCC in dfp_status with the state of Op1
REQUIRES:   Op1 of type DFP_Operand
RETURNS:    Updated value of dfp_status FPCC bits
EXCEPTIONS: None
*/

void DFP_Test_Exponent
(const Environment              *environment,   /* input */
 const DFP_Operand              *Op1,           /* input */
 const DFP_Operand              *Op2,           /* input */
 enum DFP_Status                *dfp_status,    /* output */
 RoundingCase                   *roundingCase)  /* output */
   ;
/*
PURPOSE:    Sets FPCC in dfp_status with the compare value of the exponent of Op1 and
            Op2
REQUIRES:   Op1 and Op2 of type DFP_Operand
RETURNS:    Updated value of dfp_status FPCC bits
EXCEPTIONS: None
*/

void DFP_Test_Significance
(const Environment              *environment,   /* input */
 const DecimalFP64              *Op1,           /* input */
 const DFP_Operand              *Op2,           /* input */
 enum DFP_Status                *dfp_status,    /* output */
 RoundingCase                   *roundingCase)  /* output */
;
/*
PURPOSE:    Sets FPCC in dfp_status with the compare value of the exponent of Op1 and
            Op2
REQUIRES:   Op1 and Op2 of type DFP_Operand
RETURNS:    Updated value of dfp_status FPCC bits
EXCEPTIONS: None
*/

void DFP_Quantize
(const Environment      *environment,           /* input */
 const DFP_Operand      *ExpOp,                 /* input */
 const DFP_Operand      *Op1,                   /* input */
 const uint32           RMC,                    /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase,          /* output */
 DFP_Intermediate       *intermResult           /* output */
);
/*
PURPOSE:  Adjusts the input decimal FP operand Op1 so that its exponent is equal to
          that of ExpOp.  The coefficient of Op1 is shifted or rounded as necessary.
REQUIRES: - decimal FP input operand for exponent, in ExpOp
          - decimal FP input operand, in Op1
          - two-bit rounding mode selection field, in RMC
RETURNS:  Updated values of:
          - finalResult = result of quantize operation
          - dfp_status = status and exceptions
          - intermResult = intermediate result
EXCEPTIONS: Decimal FP exceptions (as enabled by environmant) are returned in
            dfp_status
*/

/* The function DFP_QuantizeDouble handles Double-precision Quantize calls,
   with the exponent given as a DFP operand or immediate value.
   For immediate-value functionality, the exponent should be given as a signed 5-bit
   integer in the lower bits of ExponentImmediate.  "ExponentOperand" is then ignored.
   For obtaining the exponent from a DFP number, set ExponentImmediate to -1,
   and pass the DFP number in ExponentOperand.
 */
void DFP_QuantizeDouble
(const int          ExponentImmediate,    /* input */
 const DecimalFP64  ExponentOperand,      /* input */
 const DecimalFP64  Op1,                  /* input */
 const unsigned     RMC,                  /* input */
 DecimalFP64        *Result,              /* output */
 DFP_System_Status  *dfp_sys_status);     /* input / output */

/* The function DFP_QuantizeQuad handles Quad-precision Quantize calls,
   with the exponent given as a DFP operand or immediate value.
   For immediate-value functionality, the exponent should be given as a signed 5-bit
   integer in the lower bits of ExponentImmediate.  "ExponentOperand" is then ignored.
   For obtaining the exponent from a DFP number, set ExponentImmediate to -1,
   and pass the DFP number in ExponentOperand.
 */
void DFP_QuantizeQuad
(const int          ExponentImmediate,    /* input */
 const DecimalFP64  ExponentOperand[2],   /* input */
 const DecimalFP64  Op1[2],               /* input */
 const unsigned     RMC,                  /* input */
 DecimalFP64        Result[2],            /* output */
 DFP_System_Status  *dfp_sys_status);     /* input / output */

void DFP_Reround
(const Environment      *environment,           /* input */
 const unsigned         Prec,                  /* input */
 const DFP_Operand      *Op1,                   /* input */
 const uint32           RMC,                    /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase,          /* output */
 DFP_Intermediate       *intermResult           /* output */
);
/*
PURPOSE:  Adjusts the input decimal FP operand Op1 so that its precision is equal to
          Prec.  The coefficient of Op1 is shifted or rounded as necessary.
REQUIRES: - unsigned int specifying the precision in the range [0, 63] (6 bits), in Prec
          - decimal FP input operand, in Op1
          - two-bit rounding mode selection field, in RMC
RETURNS:  Updated values of:
          - finalResult = result of Reround operation
          - dfp_status = status and exceptions
          - intermResult = intermediate result
EXCEPTIONS: Decimal FP exceptions (as enabled by environmant) are returned in
            dfp_status
*/

/* Returns the correct value of the coverage metrics PrecCmp field. */
ReroundPrecCmpStatus DFP_GetPrecCmpField
(const DFP_Operand*     op1,                    /* input */
 int                    Precision);             /* input */

void DFP_RoundTo
(const Environment      *environment,           /* input */
 const DFP_Operand      *Op,                    /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase,          /* output */
 DFP_Intermediate       *intermResult           /* output */
 );
/*
PURPOSE:  Converts a DFP64 to DFP32 or DFP128 to DFP64 and updates
          finalResult, dfp_status
REQUIRES: Op of data type DFP_Operand
RETURNS:  Updated values of:
          - finalResult = Op
          - dfp_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions (as enabled by environmant) are returned in
            dfp_status
*/

void DFP_CreateIntermediateForRounding
(const DFP_Operand  *Op,
 DFP_Intermediate   *im,
 int                resultPrec);

void DFP_ConvertFromFixed
(const Environment      *environment,           /* input */
 const long long        Op1,                    /* input */
 boolean                   updateFR_FI,            /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase,          /* output */
 DFP_Intermediate       *intermResult           /* output */
);
/*
PURPOSE:  Converts the input decimal FP operand to a 64-bit signed integer.
REQUIRES: Op1 of data type long long (64-bit integer)
RETURNS:  Updated values of:
          - finalResult = result of conversion of Op1 to decimal FP
          - dfp_status = status and exceptions
          - intermResult
EXCEPTIONS: Decimal FP exceptions (as enabled by environment) are returned in
            dfp_status
*/

void DFP_ConvertToFixed
(const Environment      *environment,           /* input */
 const DFP_Operand      *Op1,                   /* input */
 long long              *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase);         /* output */
/*
PURPOSE:  Converts the input decimal FP operand to a 64-bit signed integer.
REQUIRES: Op1 of data type DFP_Operand
RETURNS:  Updated values of:
          - finalResult = result of conversion of Op1 to 64-bit signed integer
          - dfp_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions (as enabled by environmant) are returned in
            dfp_status
*/

void DFP_ConvertTo
(const Environment      *environment,           /* input */
 const DFP_Operand      *Op,                    /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase           /* output */
 );

/* prepares the final result according to the magnitude of "bcd" and the sign of "Op1".
   In case of overflow, the output and status are set according to the given
   environment. */
void DFP_ConvertBCDnumberToFixed
(const Environment* environment,           /* input */
 const DFP_Operand* Op1,                   /* input */
 const BCDnumber* bcd,                     /* input */
 long long *finalResult,                   /* output */
 enum DFP_Status* dfp_status,              /* input / output */
 RoundingCase* roundingCase);              /* output */

/* constructs BCD equivalent of a small input value (absolute value less than 0.5).
   the possible results are 0 or 1, depending on the rounding mode.
   The FI and FR status bits are updated as necessary. */
void DFP_PrepareBCD_Underflow
(const DFP_Operand* Op1,                  /* input */
const Environment* environment,           /* input */
 BCDnumber* result,                       /* input / output */
 enum DFP_Status* dfp_status,             /* input / output */
 RoundingCase* roundingCase);             /* input / output */

/* called when overflow is encountered, to update the result and status */
void DFP_ConvertToFixed_Overflow
(const Environment* environment,           /* input */
 const DFP_Operand* Op1,                   /* input */
 long long *finalResult,                   /* output */
 enum DFP_Status* dfp_status,              /* output */
 RoundingCase* roundingCase);              /* output */

void DFP_ConvertFromFixed
(const Environment      *environment,           /* input */
 const long long        Op1,                    /* input */
 boolean                   updateFR_FI,            /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase,          /* output */
 DFP_Intermediate       *intermResult           /* output */
);
/*
PURPOSE:  Converts the input decimal FP operand to a 64-bit signed integer.
REQUIRES: Op1 of data type long long (64-bit integer)
RETURNS:  Updated values of:
          - finalResult = result of conversion of Op1 to decimal FP
          - dfp_status = status and exceptions
          - intermResult
EXCEPTIONS: Decimal FP exceptions (as enabled by environment) are returned in
            dfp_status
*/

void DFP_RoundFPInteger
(const Environment      *environment,           /* input */
 const DFP_Operand      *Op,                    /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase,          /* output */
 DFP_Intermediate       *intermResult           /* output */
);

void DFP_RoundFPIntegerQuad
(const DecimalFP64  Op[2],                /* input */
 const unsigned     R,                    /* input */
 const unsigned     RMC,                  /* input */
 boolean               withInexact,          /* input */
 DecimalFP64        Result[2],            /* output */
 DFP_System_Status  *dfp_sys_status)      /* input / output */
;

void DFP_RoundFPIntegerDouble
(const DecimalFP64  Op,                   /* input */
 const unsigned     R,                    /* input */
 const unsigned     RMC,                  /* input */
 boolean               withInexact,          /* input */
 DecimalFP64        *Result,              /* output */
 DFP_System_Status  *dfp_sys_status)      /* input / output */
;

void DFP_Decode
(const Environment      *environment,           /* input */
 const unsigned         SignPref,               /* input */
 const DFP_Operand      *Op,                    /* input */
 DFP_Operand            *Result,                /* output */
 enum DFP_Status        *dfp_status             /* output */
 )
;
/*
PURPOSE:  Converts a DFP format input to a BCD output.
REQUIRES: Op of data type DFP_Operand
RETURNS:  Updated values of:
          - Result = BCD decode of Op value
          - dfp_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions (as enabled by environmant) are returned in
            dfp_status
*/

void DFP_Shift_Coefficient_Right
(const Environment      *environment,           /* input */
 const unsigned         SH,                     /* input */
 const DFP_Operand      *Op,                    /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase           /* output */
);

void DFP_Shift_Coefficient_Left
(const Environment      *environment,           /* input */
 const unsigned         SH,                     /* input */
 const DFP_Operand      *Op,                    /* input */
 DFP_Operand            *finalResult,           /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase           /* output */
);

/*
PURPOSE:  Shifts and operand coefficient left or right SH decimal places
          finalResult, dfp_status
REQUIRES: Op of data type DFP_Operand, shift amount SH
RETURNS:  Updated values of:
          - finalResult = Op shifted right or left
          - dfp_status = status
EXCEPTIONS: none possible
*/

/***************************************************************/

void Pseries_BuildEnvironment
(
 const enum DFP_Op_Type         op_type,         /* type of op      --input */
 DFP_System_Status              *dfp_sys_status, /* system status   --input-output */
 Environment                    *environment     /*                 --output */
 );

void Pseries_UpdateEnvironmentByRplusRMC
(const Environment      *original,      /* input */
 const uint32           R,              /* input */
 const uint32           RMC,            /* input */
 Environment            *updated);      /* output */

void DFP_Status2Pseries
(const Environment      *environment,   /* environment               --input */
 const enum DFP_Status dfp_status,      /* status from operation     --input */
 const enum DFP_Result_Type res_type,   /* result type specifier     --input */
 DFP_Operand* finalResult,              /*                           --input/output */
 DFP_System_Status* dfp_sys_status)     /* System status structure   --input/output */
;

void Pseries_UpdateEnvironmentByRMC(const Environment* original,   /* input */
                                const uint32 RMC,                /* input */
                                Environment* updated);         /* output */

void
Pseries_DataClassSetStatusFromSign
(bitValue          sign,             /* input */
 int               input_code,       /* input - the input DCM or DGM code */
 int               output_code,      /* input - the dcm or dgm to be output */
 enum DFP_Status   *dfp_status);     /* input/output */

void
Pseries_InvalidEncodeUpdateStatus
(Environment            *environment,       /* input */
 enum DFP_Status        *dfp_status,        /* input/output */
 DFP_Operand            *FRT);              /* output */

void Pseries_TestSignificanceUpdateStatus
(unsigned               Actual,              /* input */
 unsigned               Ref,                 /* input */
 enum DFP_Status        *dfp_status);        /* input/output */

void Pseries_GetExceptionEnableBits
(unsigned long long fpscr,          /* input */
Environment     *environment)   /* output */
;

RoundingMode Pseries_GetRoundingMode
( unsigned long long fpscr,                /* input */
  boolean FPSCR_64_bit                        /* input */
 );

/******************************************************/

void Build_Environment
(
 const enum DFP_Op_Type         op_type,         /* type of op      --input */
 DFP_System_Status              *dfp_sys_status, /* system status   --input-output */
 Environment                    *environment     /*                 --output */
 ) ;

void DFP_CopyEnvironment(const Environment* src, Environment* dest);

void DFP_UpdateSystemStatus
(const Environment      *environment,   /* environment                --input */
 const enum DFP_Status dfp_status,      /* status from operation     --input */
 const enum DFP_Result_Type res_type,   /* result type specifier     --input */
 DFP_Operand* result,                   /* DFP result of operation   --input/output */
 DFP_System_Status* dfp_sys_status);    /* system status             --input/output */

void DFP_UpdateEnvironmentFromInput
(const Environment      *original,        /* input */
 const uint             R,                /* input */
 const uint             RoundingMode,     /* input */
 Environment            *updated);        /* output */

/* sets the exception flags in exceptionSignals using the information in dfp_status */
void DFP_ExtractExceptionSignals(enum DFP_Status dfp_status,
                                 ExceptionEnableBits* exceptionSignals);

/* InitRoundingCase is optional.  It is used to set the 4 flags of the roundingCase
   structure to false. */
void DFP_InitRoundingCase(RoundingCase* roundingCase);

/* Updates the ResultRange field of coverage_metrics.
   Instructions with no intermediate result should pass NULL as the "im" parameter.
   Instructions with no final result should pass NULL as the "final" parameter.
   We assume that the RoundingCase field of coverage_metrics has already been updated. */
void DFP_UpdateResultRange(const DFP_Intermediate* im,
                           const DFP_Operand* final,
                           enum DFP_Status dfp_status,
                           CoverageMetrics* coverage_metrics);

DFP_DataType GetDFP_DataType(const DFP_Operand* op);

/* Performs updates of non-instruction-specific fields of CoverageMetrics, except for
   roundingCase which is assumed to have been updated previously. Any of op1, op2, final,
   and im may be NULL if they are not relelvant to the instruction. */
void DFP_UpdateCoverageMetrics(InstType instType,
                               const Environment* environment,
                               enum DFP_Status dfp_status,
                               const DFP_Operand* op1,
                               const DFP_Operand* op2,
                               const DFP_Operand* final,
                               const DFP_Intermediate* im,
                               CoverageMetrics* coverage_metrics);

void DFP_RoundAndSetStatus(DFP_Intermediate         *intermResult, /* input/output */
                           int                      idealExponent, /* input */
                           const Environment        *environment,  /* input */
                           boolean                     updateFR_FI,   /* input */
                           DFP_Operand              *finalResult,  /* output */
                           enum DFP_Status          *dfp_status,   /* output */
                           RoundingCase             *roundingCase);/* output */

void DFP_SetStatus(const Environment        *environment,  /* input */
                   boolean                     updateFR_FI,   /* input */
                   DFP_Operand              *finalResult,  /* output */
                   enum DFP_Status          *dfp_status);  /* output */

/******************************************************/

void DFP_invalid_operation_exceptions
(const Environment *environment,        /*                         -- input */
 const enum DFP_Op_Type instType,       /* Instruction Type        -- input */
 const enum DFP_Status invOpType,       /* Type of Invalid Op      -- input */
 enum DFP_Status *dfp_status,           /* status                  -- output */
 DFP_Operand *finalResult               /*                         -- output */
 );

void DFP_Overflow_exception
(const Environment *environment,        /*                         -- input */
 enum DFP_Status *dfp_status,           /* status                  -- output */
 DFP_Operand *finalResult               /*                         -- output */
 );

void DFP_Underflow_exception
(const Environment *environment,        /*                         -- input */
 enum DFP_Status *dfp_status,           /* status                  -- output */
 DFP_Operand *finalResult               /*                         -- output */
 );

void DFP_Inexact_exception
(const Environment *environment,        /*                         -- input */
 enum DFP_Status *dfp_status,           /* status                  -- output */
 DFP_Operand *finalResult               /*                         -- output */
 );

void DFP_Zero_Divide_exception
(const Environment *environment,                /*                     -- input */
 enum DFP_Status *dfp_status,                   /* status              -- output */
 DFP_Operand *finalResult                       /*                     -- output */
 );

void DFP_set_post_operation_status
(const DFP_Operand *finalResult,                /* result of operation--input */
 enum DFP_Status *dfp_status)                   /* status             --input/output*/
;

boolean DFP_HandleNaNsAndSpecialNumbers
(
 const Environment              *environment,           /* input */
 const enum DFP_Arith_Op_Type   Arith_type,             /* input */
 const DFP_Operand              *Op1,                   /* input */
 const DFP_Operand              *Op2,                   /* input */
 DFP_Operand                    *finalResult,           /* output */
 enum DFP_Status                *dfp_status            /* output */
);
/*
PURPOSE:
REQUIRES: pointers to:
          Op1, Op2 operands
          dfp_status
          intermResult
RETURNS:  dfp_status and intermResult updated with proper status if
          one of Op1 and Op2 was a NAN or Infinity.
EXCEPTIONS: None
*/

boolean DFP_HandleNANs
(
 const Environment              *environment,           /* input */
 const DFP_Operand              *Op1,                   /* input */
 const DFP_Operand              *Op2,                   /* input */
 DFP_Operand                    *finalResult,           /* output */
 enum DFP_Status                *dfp_status            /* output */
);

void DFP_Form_QNAN_Result
(
 const DFP_Operand              *Op,                    /* input */
 DFP_Operand                    *Result                 /* output */
 );

void DFP_Form_SNAN_Result
(
 const DFP_Operand              *Op,                    /* input */
 DFP_Operand                    *Result                 /* output */
 );

int DFP_Default_SNAN_Unbiased_Exponent
(const DFP_Format*              format);                /* input */

unsigned DFP_HandleSpecialNumbers
(
 const Environment              *environment,           /* input */
 const enum DFP_Arith_Op_Type   Arith_type,             /* input */
 const DFP_Operand              *Op1,                   /* input */
 const DFP_Operand              *Op2,                   /* input */
 DFP_Operand                    *finalResult,           /* output */
 enum DFP_Status                *dfp_status             /* output */
);

/*
 Creates a default infinite result in the Operand.
 */
void DFP_SetInfiniteResult(bitValue        sign,          /* input */
                           DFP_Operand     *operand,      /* output */
                           enum DFP_Status *dfp_status);  /* output */

/*
 Coerces the input operand to an infinite result in the result Operand.
 */
void DFP_CoerceInfiniteResult
(const DFP_Operand      *Op,            /* input */
 DFP_Operand            *result,        /* output */
 enum DFP_Status *dfp_status);          /* input/output */

/**************************************************************/

/*
Three input /output forms need to be handled, to convert 4- 8- and 16-byte DFP formats
to /from the internal format.  The input forms are:
*/

enum DFP_Type DFP_Unpack_Single
( const unsigned long long      PackedSingle,   /* Unpacked Word DFP    -- input */
  const enum DFP_System System,                 /* which system         -- inpuy */
  DFP_Operand*                  Unpacked);      /* DFP structure        -- output */
/*
PURPOSE:  Updates a DFP_Operand structure with the information from
          a 32-bit Decimal FP binary representation
REQUIRES: - PackedSingle: a 32-bit binary DFP number (in low word of DW)
RETURNS:  Updated DFP_Operand structure with the values from the
          PackedSingle input
EXCEPTIONS: None
*/


enum DFP_Type DFP_Unpack_Double
( const unsigned long long Double,              /* Unpacked DW DFP      -- input */
  DFP_Operand* Unpacked);                       /* DFP structure        -- output */
/*
PURPOSE:  Updates a DFP_Operand structure with the information from
          a 64-bit Decimal FP binary representation
REQUIRES: - PackedSingle: a 64-bit binary DFP number
RETURNS:  Updated DFP_Operand structure with the values from the
          PackedSingle input.  The coefficient will contain  3 extra trailing
          digits, for Guard, Round and Sticky digits.
EXCEPTIONS: None
*/

enum DFP_Type DFP_Unpack_Quad
( const unsigned long long Quad[2],             /* Unpacked QW DFP      -- input */
  DFP_Operand* Unpacked);                       /* DFP structure        -- output */


/*
DFP_Unpack() selects one of DFP_Unpack_Single, DFP_Unpack_Double, or DFP_Unpack_Quad,
as appropriate for the output precision (the format of the output must already be correct
when DFP_Unpack is called).  If none of the three is suitable, the program is aborted.
For Single and Double precisions, the packed input is given in Packed[0], and
Packed[1] is unused.  For Quad, the representation is the same as in DFP_Unpack_Quad.
*/
void DFP_Unpack
(const unsigned long long Packed[2],    /* DPD (packed) representation -- input */
 const enum DFP_System    System,       /* which system         -- input */
 DFP_Operand* Unpacked);                /* DFP structure               -- output */


/* The output forms convert a DFP_Operand to a real-world format DFP number. */

void DFP_Pack_Single
( const DFP_Operand      *Unpacked,             /* DFP structure        -- input */
  const enum DFP_System  System,                /* which system         -- input */
  unsigned long long     *PackedSingle );       /* Unpacked 4-byte DFP  -- output */
/*
PURPOSE:  Updates a 32-bit Decimal FP binary representation from the
          DFP_Operand structure.
REQUIRES: -DFP_Operand structure containing a "generic" DFP number
          (without length restrictions)
RETURNS:  unsigned-long-long containing the 32-bit representation of the DFP
          number in the low word.
EXCEPTIONS: None
*/
void DFP_Pack_Double
( const DFP_Operand* Unpacked,                  /* DFP structure        -- input  */
  unsigned long long* Double );                 /* Unpacked 8-byte DFP  -- output */
/*
PURPOSE:  Updates a 64-bit Decimal FP binary representation from the
          DFP_Operand structure.
REQUIRES: -DFP_Operand structure containing a "generic" DFP number
          (without length restrictions).  It is assumed that there are 3
          extra digits to the right of the significant digits of the number
          which are the Guard, Round and Sticky digits.  Sticky should have
          a value of only 0 and 1.
RETURNS:  unsigned-long-long containing the 64-bit representation of the DFP
          number.
EXCEPTIONS: None
*/

void DFP_Pack_Quad
( const DFP_Operand* Unpacked,                  /* DFP structure        -- input */
  unsigned long long Quad[2]);                  /* Unpacked 16-byte DFP -- output */
/*
PURPOSE:  Updates a 128-bit Decimal FP binary representation from the
          DFP_Operand structure.
REQUIRES: -DFP_Operand structure containing a "generic" DFP number
          (without length restrictions)
RETURNS:  2-entry array of unsigned-long-long containing the 128-bit
          representation of the DFP number.
EXCEPTIONS: None
QUESTION: Does this function need to expect a length of
          DFP_Operand.
*/


/*
DFP_Pack() selects one of DFP_Pack_Single, DFP_Pack_Double, or DFP_Pack_Quad,
as appropriate for the input precision.  If none of the three is suitable, the program
is aborted.
For Single and Double precisions, the packed result is returned in Packed[0], and
   Packed[1] is unchanged.  For Quad, the representation is the same as in DFP_Pack_Quad.
*/
void DFP_Pack
(const DFP_Operand* Unpacked,           /* DFP structure               -- input */
 const enum DFP_System System,                 /* which system         -- input */
 unsigned long long Packed[2]);         /* DPD (packed) representation -- output */

/*
UnpackCombo() is an internal function which  will decode the combination
field into type, exponent most-significant 2 bits and coeficient
most-significant BCD digit.
*/
enum DFP_Type UnpackCombo
(const unsigned         Combo,          /* 5-bit combination field      -- input */
 const unsigned         snanFlag,       /* QNAN / SNAN bit              -- input */
 unsigned               *ExpMSB,        /* Exponent 2 MS Bits           -- output */
 unsigned               *CoefMSD        /* Coeficient MS Digit          -- output */
);

/*
PackCombo() is an internal function which will encode the combination
byte given input of the unpacked number.
*/
char PackCombo
(const enum DFP_Type    type,              /* Type flag                    -- input */
 const char             ExpMSB,            /* Exponent 2 MS Bits           -- input */
 const char             CoefMSD            /* Coeficient MS Digit          -- input */
);

unsigned DPD2BCD
(const int DPDin);              /* 10-bit right justified 3 digits -- input */


unsigned BCD2DPD
(const unsigned BCDin);         /* 3 BCD 4-bit digits              -- input */

/******************************************************************************/

void DFP_Extract_Biased_Exponent
(const Environment      *environment,           /* input */
 const DFP_Operand      *Op1,                   /* input */
 DecimalFP64            *binary,                /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase)          /* output */
;

void DFP_Extract_Exponent
(const Environment      *environment,           /* input */
 const DFP_Operand      *Op1,                   /* input */
 DecimalFP64            *binary,                /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase);         /* output */
/*
PURPOSE:    Extracts the exponent of Op1 and places it in finalResult
REQUIRES:
RETURNS:    Updated value of dfp_status
EXCEPTIONS: Decimal FP exceptions are returned in dfp_status
*/

void DFP_Insert_Biased_Exponent
(const Environment      *environment,           /* input */
 const long long int    *Op1,                   /* input */
 const DFP_Operand      *Op2,                   /* input */
 DFP_Operand            *Result,                /* output */
 enum DFP_Status        *dfp_status,            /* output */
 RoundingCase           *roundingCase)          /* output */
;

#   define dfp_assert(__EX) ((__EX) ? ((void)0) : DFP_Assert(# __EX, __FILE__, __LINE__))

void DFP_Assert_Set_Termination(int (*userfunc)(char *));
/*
PURPOSE:  Defines a user function to be called if a "dfp_assert()" macro is
          executed.  This function allows the user of the DFP RefMod to specify
          a routine to get control when a terminal error condition is detected,
          allowing a cleaner system termination than a core dump.
REQUIRES: -"userfunc" must be a function returning an int with a single char*
          parameter
RETURNS:  None. If this function is not called the function is defaulted
          to NULL and will not be called, with a default message (similar to the
          system assert() message) will be printed.
EXCEPTIONS: None.
*/

void DFP_Assert(const char *condition, const char *file, const int line);
/*
PURPOSE:  Function is called by the dfp_assert() macro when the assert condition
          is false.  DFP_Assert will print a meesage with the file and line number
          which posted the dfp_assert.  If the termination routine has been set
          to not NULL (via the DFP_Assert_Set_Termination function) then that
          function is called.  If termination is NULL it will call exit() to
          terminate the program.
          The user termination function may return from the call with a termination
          code which will be posted as the exit() code.
REQUIRES: condition, file name and line number as passed from the dfp_assert macro.
RETURNS:  No return from this call.  exit() will be called to terminate if
          the termination function returns or the termination function is NULL.
EXCEPTIONS: None.
*/

/**************************************************************************/

/*
This is a Decimal FP add routine which would be called by each simulator
to implement its version of the "decimal FP add Double" instruction.
*/
void DFP_Add_Double
(const DecimalFP64      operand1,                    /* input */
 const DecimalFP64      operand2,                    /* input */
 DecimalFP64            *Result,                     /* output */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Adds two DecimalFP64 operands and updates the
          Result and dfp_system_status parameters
REQUIRES: -operand1 and operand2 of data type DecimalFP64
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result = operand1 + operand2
          - dfp_system_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status
*/

void DFP_Add_Quad
(const DecimalFP64      operand1[2],                 /* input */
 const DecimalFP64      operand2[2],                 /* input */
 DecimalFP64            Result[2],                   /* output */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Adds two DecimalFP128 operands and updates the
          DecimalFP128 Result and dfp_system_status operands
          The  DecimalFP128 operands each consist of two DecimalFP64 parameters.
REQUIRES: -operand1 and operand2, each composed of 2 DecimalFP64 datatype
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result = operand1 + operand2
          - dfp_system_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status
*/

void DFP_Subtract_Double
(const DecimalFP64      operand1,                    /* input */
 const DecimalFP64      operand2,                    /* input */
 DecimalFP64            *Result,                     /* output */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Subtracts two DecimalFP64 operands and updates the
          Result and dfp_system_status parameters
REQUIRES: -operand1 and operand2 of data type DecimalFP64
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result = operand1 - operand2
          - dfp_system_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status
*/

void DFP_Subtract_Quad
(const DecimalFP64      operand1[2],                 /* input */
 const DecimalFP64      operand2[2],                 /* input */
 DecimalFP64            Result[2],                   /* output */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Subtracts two DecimalFP128 operands and updates the
          DecimalFP128 Result and dfp_system_status operands
          The  DecimalFP128 operands each consist of two DecimalFP64 parameters.
REQUIRES: -operand1 and operand2, each composed of 2 DecimalFP64 datatype
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result = operand1 - operand2
          - dfp_system_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status
*/


void DFP_Multiply_Double
(const DecimalFP64      operand1,                    /* input */
 const DecimalFP64      operand2,                    /* input */
 DecimalFP64            *Result,                     /* output */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Multiplies two DecimalFP64 operands and updates the
          Result, dfp_system_status and (optional) intermResult parameters
REQUIRES: operand1 and operand2 of data type DecimalFP64
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result = operand1 * operand2
          - dfp_system_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions (as enabled by dfp_system_status) are returned in
            dfp_system_status
*/

void DFP_Multiply_Quad
(const DecimalFP64      operand1[2],                 /* input */
 const DecimalFP64      operand2[2],                 /* input */
 DecimalFP64            Result[2],                   /* output */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Multiplies two DecimalFP128 operands and updates the
          DecimalFP128 Result and dfp_system_status operands.
          The  DecimalFP128 operands each consist of two DecimalFP64 parameters.
REQUIRES: operand1 and operand2 of data type DecimalFP128
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result = operand1 * operand2
          - dfp_system_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions (as enabled by dfp_system_status) are returned in
            dfp_system_status
*/

void DFP_Divide_Double
(const DecimalFP64      operand1,                    /* input */
 const DecimalFP64      operand2,                    /* input */
 DecimalFP64            *Result,                     /* output */
 DFP_System_Status      *dfp_system_status);         /* output */

void DFP_Divide_Quad
(const DecimalFP64      operand1[2],                 /* input */
 const DecimalFP64      operand2[2],                 /* input */
 DecimalFP64            Result[2],                   /* output */
 DFP_System_Status      *dfp_system_status);         /* output */

void DFP_Compare_Double
(const unsigned         Ordered,                     /* input */
 const DecimalFP64      operand1,                    /* input */
 const DecimalFP64      operand2,                    /* input */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Compares two DecimalFP64 operands and updates dfp_system_status
REQUIRES: operand1 and operand2 of data type DecimalFP64
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - dfp_system_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions (as enabled by dfp_system_status) are returned in
            dfp_system_status
*/

void DFP_Compare_Quad
(const unsigned         Ordered,                     /* input */
 const DecimalFP64      operand1[2],                 /* input */
 const DecimalFP64      operand2[2],                 /* input */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Compares two DecimalFP128 operands and updates dfp_system_status.
          The  DecimalFP128 operands each consist of two DecimalFP64 parameters.
REQUIRES: operand1 and operand2 of data type DecimalFP128
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - dfp_system_status = status and exceptions
EXCEPTIONS: Decimal FP exceptions (as enabled by dfp_system_status) are returned in
            dfp_system_status
*/

void DFP_Extract_Biased_Exponent_Double
(const DecimalFP64      Op1,                         /* input */
 DecimalFP64            *Result,                     /* output */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Extacts the biased exponent of Op1 and places it as an unsigned number
          into Result, updating dfp_system_status
REQUIRES: Op1 of data type DecimalFP64
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result = Op1 biased exponenet
          - dfp_system_status = status and exceptions
EXCEPTIONS: The FPCC field is set to FG if the input is not a NAN or infinity. FL
            is set if the input is NAN or infinity.
*/

void DFP_Extract_Biased_Exponent_Quad
(const DecimalFP64      Op1[2],                      /* input */
 DecimalFP64            *Result,                     /* output */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Extacts the biased exponent of Op1 and places it as an unsigned number
          into Result, updating dfp_system_status
REQUIRES: Op1 of data type DecimalFP128
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result = Op1 biased exponent of type DecimalFP128
          - dfp_system_status = status and exceptions
EXCEPTIONS: The FPCC field is set to FG if the input is not a NAN or infinity. FL
            is set if the input is NAN or infinity.
*/

void DFP_Insert_Biased_Exponent_Double
(const DecimalFP64      Op1,                         /* input */
 const DecimalFP64      Op2,                         /* input */
 DecimalFP64            *Result,                     /* output */
 DFP_System_Status      *dfp_system_status);         /* output */

void DFP_Insert_Biased_Exponent_Quad
(const DecimalFP64      Op1,                         /* input */
 const DecimalFP64      Op2[2],                      /* input */
 DecimalFP64            Result[2],                   /* output */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Replaces the biased exponent of Op1 with the biased exponent value in Op2
          and places it into Result, updating dfp_system_status
REQUIRES: Op1 of data type DecimalFP64/128
          Op2 containing a biased exponent binary value
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result = Op1 with new biased exponent
          - dfp_system_status = status and exceptions
          - FPCC field is set according to the type of the result written in Result.
EXCEPTIONS: None
*/

void DFP_Test_Data_Class_Single
(const DecimalFP64      Op1,                         /* input */
 const unsigned         DCM,                         /* input */
 DFP_System_Status      *dfp_system_status);         /* output */

void DFP_Test_Data_Class_Double
(const DecimalFP64      Op1,                         /* input */
 const unsigned         DCM,                         /* input */
 DFP_System_Status      *dfp_system_status);         /* output */

void DFP_Test_Data_Class_Quad
(const DecimalFP64      Op1[2],                      /* input */
 const unsigned         DCM,                         /* input */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Tests the value of Op1 for 6 possible conditions and sets the value
          of condition bits to indicate if the condition set in the DCM
          (Data Subclass Mask) field is present.
REQUIRES: Op1 of data type DecimalFP64/128
          DCM unsigned mask of conditions to be tested
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - dfp_system_status = status updated in CR/CC
          - FPCC field is set according to the type of the data in Op1
EXCEPTIONS: None
*/

void DFP_Test_Data_Group_Single
(const DecimalFP64      Op1,                         /* input */
 const unsigned         DGM,                         /* input */
 DFP_System_Status      *dfp_system_status);         /* output */

void DFP_Test_Data_Group_Double
(const DecimalFP64      Op1,                         /* input */
 const unsigned         DGM,                         /* input */
 DFP_System_Status      *dfp_system_status);         /* output */

void DFP_Test_Data_Group_Quad
(const DecimalFP64      Op1[2],                      /* input */
 const unsigned         DGM,                         /* input */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Tests the value of Op1 for 6 (?) possible conditions and sets the value
          of the condition bits in DFP_System_Status to indicate if the condition set
          in the DGM (Data Group Mask) field is present.
REQUIRES: Op1 of data type DecimalFP64/128
          DGM unsigned mask of conditions to be tested
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - dfp_system_status = status updated in CR/CC
EXCEPTIONS: None
*/



void DFP_Test_Exponent_Double
(const DecimalFP64      Op1,                         /* input */
 const DecimalFP64      Op2,                         /* input */
 DFP_System_Status      *dfp_system_status);         /* output */

void DFP_Test_Exponent_Quad
(const DecimalFP64      Op1[2],                      /* input */
 const DecimalFP64      Op2[2],                      /* input */
 DFP_System_Status      *dfp_system_status);         /* output */
/*
PURPOSE:  Compares the exponents of Op1 and Op2 and sets the value
          of the condition bits in DFP_System_Status indicate the
          relation between them
REQUIRES: Op1 and Op2 of data type DecimalFP64/128
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - dfp_system_status = status updated in CR/CC
          - status is set according to the Op1 to Op2 exp compare
EXCEPTIONS: None
*/


void DFP_Test_Significance_Double
(const DecimalFP64      Op1,                         /* input */
 const DecimalFP64      Op2,                         /* input */
 DFP_System_Status      *dfp_sys_status)             /* output */
;
void DFP_Test_Significance_Quad
(const DecimalFP64      Op1,                         /* input */
 const DecimalFP64      Op2[2],                      /* input */
 DFP_System_Status      *dfp_sys_status)             /* output */
;
/*
PURPOSE:  Compares the number of significant digits in Op2 to the fixed
          value in Op1 and sets the status bits in DFP_System_Status
REQUIRES: Op2 of data type DecimalFP64/128
          Op1 of type Decimal64 -
RETURNS:  Updated values of:
          - dfp_system_status = status bits updated
EXCEPTIONS: None
*/

void DFP_Convert_Single_To_Double
(const DecimalFP64      Op,                     /* input */
 DecimalFP64            *Result,                /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;
/*
PURPOSE:  Converts operand Op1 from DFP32 format to Op2 in DFP64 format.
REQUIRES: Op1 of data type DecimalFP32
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          -Op2 contains DFP64 format number
          - dfp_system_status = status bits updated
EXCEPTIONS: None
*/

void DFP_Convert_Double_To_Quad
(const DecimalFP64      Op,                     /* input */
 DecimalFP64            Result[2],              /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;
/*
PURPOSE:  Converts operand Op1 from DFP64 format to Op2 in DFP128 format.
REQUIRES: Op1 of data type DecimalFP64
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          -Op2 contains DFP128 format number
          - dfp_system_status = status bits updated
EXCEPTIONS: None
*/

void DFP_Round_Double_To_Single
(const DecimalFP64      Op,                     /* input */
 DecimalFP64            *Result,                /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;
/*
PURPOSE:  Rounds operand Op from DFP64 format and converts to Result in DFP32 format.
REQUIRES: Op of data type DecimalFP64
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          -Result contains DFP32 format number
          - dfp_system_status = status bits updated
EXCEPTIONS: None
*/

void DFP_Round_Quad_To_Double
(const DecimalFP64      Op[2],                  /* input */
 DecimalFP64            Result[2],              /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;
/*
PURPOSE:  Converts operand Op from DFP128 format to Result in DFP64 format.
REQUIRES: Op of data type DecimalFP128
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          -Op contains DFP64 format number in high DW and 0 in low DW or
           a DFP128 result if OX or UX enabled interrupts occur
          - dfp_system_status = status bits updated
EXCEPTIONS: None
*/

void DFP_Decode_DPD_To_BCD_Double
(const unsigned         SignPref,               /* input */
 const DecimalFP64      Op,                     /* input */
 DecimalFP64            *Result,                /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;
void DFP_Decode_DPD_To_BCD_Quad
(const unsigned         SignPref,               /* input */
 const DecimalFP64      Op[2],                  /* input */
 DecimalFP64            Result[2],              /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;
/*
PURPOSE:  Converts the coefficient of operand from DPD to BCD
REQUIRES: Op of data type DecimalFP64
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          -Result contains the BCD output
          - dfp_system_status = status bits updated
EXCEPTIONS: None
*/


void DFP_Encode_BCD_To_DPD_Double
(const unsigned         Signed,                 /* input */
 const DecimalFP64      Op,                     /* input */
 DecimalFP64            *Result,                /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;
void DFP_Encode_BCD_To_DPD_Quad
(const unsigned         Signed,                 /* input */
 const DecimalFP64      Op[2],                  /* input */
 DecimalFP64            Result[2],              /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;
/*
PURPOSE:  Converts the BCD input to the DPD coefficient
REQUIRES: Op of data type DecimalFP64
          -dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          -Result contains the updated coefficient (in DPD)
          - dfp_system_status = status bits updated
EXCEPTIONS: None
*/

void DFP_Convert_From_Fixed_Double
(const DecimalFP64     Op1,                 /* input */
 DecimalFP64           *Result,             /* output */
 DFP_System_Status     *dfp_sys_status);    /* input / output */

void DFP_Convert_From_Fixed_Quad
(const DecimalFP64     Op1,                 /* input */
 DecimalFP64           Result[2],           /* output */
 DFP_System_Status     *dfp_sys_status);    /* input / output */
/*
PURPOSE:  Converts a 64-bit signed integer to a decimal FP operand,
          updating dfp_system_status
REQUIRES: - Op1: signed 64-bit integer
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: result of the conversion of operand1 to decimalFP64/128
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status
*/

void DFP_Convert_To_Fixed_Double
(const DecimalFP64         Op1,               /* input */
 DecimalFP64               *Result,           /* output */
 DFP_System_Status         *dfp_sys_status);  /* input / output */

void DFP_Convert_To_Fixed_Quad
(const DecimalFP64         Op1[2],             /* input */
 DecimalFP64               *Result,           /* output */
 DFP_System_Status         *dfp_sys_status);  /* input / output */
/*
PURPOSE:  Converts a decimal FP operand to a 64-bit signed integer,
          updating dfp_system_status
REQUIRES: - Op1 of type DecimalFP64/128
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: result of the conversion of operand1 to 64-bit signed integer
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status
*/

void DFP_Convert_To_Fixed_with_RM_Double
(const DecimalFP64         Op1,               /* input */
 const unsigned            RM,                /* input */
 DecimalFP64               *Result,           /* output */
 DFP_System_Status         *dfp_sys_status);  /* input / output */

void DFP_Convert_To_Fixed_with_RM_Quad
(const DecimalFP64         Op1[2],            /* input */
 const unsigned            RM,                /* input */
 DecimalFP64               *Result,           /* output */
 DFP_System_Status         *dfp_sys_status);  /* input / output */
/*
PURPOSE:  Converts a decimal FP operand to a 64-bit signed integer,
          updating dfp_system_status
REQUIRES: - Op1 of type DecimalFP64/128
          - dfp_system_status: contains FPSCR/FPC value and system type
          - RM - rounding method - a 4 bit field
RETURNS:  Updated values of:
          - Result: result of the conversion of operand1 to 64-bit signed integer
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status
*/

void DFP_Quantize_Immediate_Double
(const unsigned     Exponent,          /* input */
 const DecimalFP64  Op1,               /* input */
 const unsigned     RMC,               /* input */
 DecimalFP64        *Result,           /* output */
 DFP_System_Status  *dfp_sys_status);  /* input / output */

/*
PURPOSE:  Adjusts the decimal FP operand Op1 to the form with the exponent specified
          in the "Exponent" parameter.  Updates the system status accordingly.
REQUIRES: - Exponent: a 5-bit signed integer in the lowest bits of the parameter.
          - Op1 of type DecimalFP64
          - RMC: rounding mode code (see DFP specification for details)
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: the value that corresponds to Op1, with the required target exponent
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status.
            The first routine sets the FI, FR, and XX status bits according to the usual
            rules, while the second one always resets them to Zero.
*/

void DFP_Quantize_Double
(const DecimalFP64  ExponentOperand,   /* input */
 const DecimalFP64  Op1,               /* input */
 const unsigned     RMC,               /* input */
 DecimalFP64        *Result,           /* output */
 DFP_System_Status  *dfp_sys_status);  /* input / output */


/*
PURPOSE:  Adjusts the decimal FP operand Op1 to the form with the exponent equal to that
          of the "ExponentOperand" parameter.  Updates the system status accordingly.
REQUIRES: - ExponentOperand of type DecimalFP64
          - Op1 of type DecimalFP64
          - RMC: rounding mode code (see DFP specification for details)
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: the value that corresponds to Op1, with the required target exponent
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status.
            The first routine sets the FI, FR, and XX status bits according to the usual
            rules, while the second one always resets them to Zero.
*/

void DFP_Quantize_Immediate_Quad
(const unsigned     Exponent,          /* input */
 const DecimalFP64  Op1[2],            /* input */
 const unsigned     RMC,               /* input */
 DecimalFP64        Result[2],         /* output */
 DFP_System_Status  *dfp_sys_status);  /* input / output */

/*
PURPOSE:  Adjusts the decimal FP operand Op1 to the form with the exponent specified
          in the "Exponent" parameter.  Updates the system status accordingly.
REQUIRES: - Exponent: a 5-bit signed integer in the lowest bits of the parameter.
          - Op1 of type DecimalFP128
          - RMC: rounding mode code (see DFP specification for details)
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: the value that corresponds to Op1, with the required target exponent
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status.
            The first routine sets the FI, FR, and XX status bits according to the usual
            rules, while the second one always resets them to Zero.
*/

void DFP_Quantize_Quad
(const DecimalFP64  ExponentOperand[2],   /* input */
 const DecimalFP64  Op1[2],               /* input */
 const unsigned     RMC,                  /* input */
 DecimalFP64        Result[2],            /* output */
 DFP_System_Status  *dfp_sys_status);     /* input / output */

/*
PURPOSE:  Adjusts the decimal FP operand Op1 to the form with the exponent equal to that
          of the "ExponentOperand" parameter.  Updates the system status accordingly.
REQUIRES: - ExponentOperand of type DecimalFP128
          - Op1 of type DecimalFP128
          - RMC: rounding mode code (see DFP specification for details)
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: the value that corresponds to Op1, with the required target exponent
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status.
            The first routine sets the FI, FR, and XX status bits according to the usual
            rules, while the second one always resets them to Zero.
*/

void DFP_Reround_Quad
(const unsigned     Precision,             /* input */
 const DecimalFP64  Op1[2],                /* input */
 const unsigned     RMC,                   /* input */
 DecimalFP64        Result[2],             /* output */
 DFP_System_Status  *dfp_sys_status);      /* input / output */
/*
PURPOSE:  Adjusts the decimal FP operand Op1 to a precision of p+1 digits, where p is
          the value of the "Precision" parameter.  Updates the system status accordingly.
REQUIRES: - Precision of type unsigned int in the range [0, 63] (6 bits)
          - Op1 of type DecimalFP128
          - RMC: rounding mode code (see DFP specification for details)
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: the value that corresponds to Op1, with the required target precision
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status.
*/

void DFP_Reround_Double
(const unsigned     Precision,            /* input */
 const DecimalFP64  Op1,                  /* input */
 const unsigned     RMC,                  /* input */
 DecimalFP64        *Result,               /* output */
 DFP_System_Status  *dfp_sys_status);     /* input / output */
/*
PURPOSE:  Adjusts the decimal FP operand Op1 to a precision of p+1 digits, where p is
          the value of the "Precision" parameter.  Updates the system status accordingly.
REQUIRES: - Precision of type unsigned int in the range [0, 63] (6 bits)
          - Op1 of type DecimalFP64
          - RMC: rounding mode code (see DFP specification for details)
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: the value that corresponds to Op1, with the required target precision
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status.
*/

void DFP_RoundFPInteger_With_Inexact_Quad
(const DecimalFP64  Op1[2],            /* input */
 const unsigned     R,                 /* input */
 const unsigned     RMC,               /* input */
 DecimalFP64        Result[2],         /* output */
 DFP_System_Status  *dfp_sys_status)   /* input / output */
;

void DFP_RoundFPInteger_Without_Inexact_Quad
(const DecimalFP64  Op1[2],            /* input */
 const unsigned     R,                 /* input */
 const unsigned     RMC,               /* input */
 DecimalFP64        Result[2],         /* output */
 DFP_System_Status  *dfp_sys_status)   /* input / output */
;
/*
PURPOSE:  Rounds the decimal FP operand Op1 to an integer with the rounding
          as specified in the R and RMC fields
REQUIRES: - Op1 of type DecimalFP128
          - R: primary/secondary RMC select field (see DFP specification for details)
          - RMC: rounding mode code (see DFP specification for details)
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: DecimalFP128 value that corresponds to Op1, with the required target precision
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status.
            The first routine sets the FI, FR, and XX status bits according to the usual
            rules, while the second one always resets them to Zero.
*/

void DFP_RoundFPInteger_With_Inexact_Double
(const DecimalFP64  Op1,               /* input */
 const unsigned     R,                 /* input */
 const unsigned     RMC,               /* input */
 DecimalFP64        *Result,           /* output */
 DFP_System_Status  *dfp_sys_status)   /* input / output */
;

void DFP_RoundFPInteger_Without_Inexact_Double
(const DecimalFP64  Op1,               /* input */
 const unsigned     R,                 /* input */
 const unsigned     RMC,               /* input */
 DecimalFP64        *Result,           /* output */
 DFP_System_Status  *dfp_sys_status)   /* input / output */
;
/*
PURPOSE:  Rounds the decimal FP operand Op1 to an integer with the rounding
          as specified in the R and RMC fields
REQUIRES: - Op1 of type DecimalFP64
          - R: primary/secondary RMC select field (see DFP specification for details)
          - RMC: rounding mode code (see DFP specification for details)
          - dfp_system_status: contains FPSCR/FPC value and system type
RETURNS:  Updated values of:
          - Result: DecimalFP64 value that corresponds to Op1, with the required target precision
          - dfp_system_status: status and exceptions
EXCEPTIONS: Decimal FP exceptions are returned in dfp_system_status.
            The first routine sets the FI, FR, and XX status bits according to the usual
            rules, while the second one always resets them to Zero.
*/


void DFP_Shift_Coefficient_Left_Double
(const unsigned         SH,                     /* input */
 const DecimalFP64      Op,                     /* input */
 DecimalFP64            *Result,                /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;

void DFP_Shift_Coefficient_Left_Quad
(const unsigned         SH,                     /* input */
 const DecimalFP64      Op[2],                  /* input */
 DecimalFP64            Result[2],              /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;

void DFP_Shift_Coefficient_Right_Double
(const unsigned         SH,                     /* input */
 const DecimalFP64      Op,                     /* input */
 DecimalFP64            *Result,                /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;

void DFP_Shift_Coefficient_Right_Quad
(const unsigned         SH,                     /* input */
 const DecimalFP64      Op[2],                  /* input */
 DecimalFP64            Result[2],              /* output */
 DFP_System_Status      *dfp_sys_status)        /* input / output */
;
/*
PURPOSE:  Shifts the input decimal FP operand Op1 right or left by SH bits
          without changing the coefficient
REQUIRES: - Op of type DecimalFP64 (Double) or
            Op of type DecimalFP128 (Quad)
          -SH unsigned integer shift count
          -DFP_System_Status
RETURNS:  Updated values of:
          - Result:  DecimalFP64 (Double) or DecimalFP128 (Quad) value that corresponds to shifted Op value
          - dfp_system_status: status and exceptions
EXCEPTIONS: None
*/

/************************************************************************************/

extern void simulate_and(uint32 *machine_code, struct testcase *test_case);
extern void simulate_add(uint32 *machine_code, struct testcase *test_case);
extern void simulate_mfcr(uint32 *machine_code, struct testcase *test_case);
extern void simulate_addi(uint32 *machine_code, struct testcase *test_case);
extern void simulate_addis(uint32 *machine_code, struct testcase *test_case);
extern void simulate_ori(uint32 *machine_code, struct testcase *test_case);
extern void simulate_stwx(uint32 *machine_code, struct testcase *test_case);
extern void simulate_lfs(uint32 *machine_code, struct testcase *test_case);
extern void simulate_lfd(uint32 *machine_code, struct testcase *test_case);
extern void simulate_lfsx(uint32 *machine_code, struct testcase *test_case);
extern void simulate_lfdx(uint32 *machine_code, struct testcase *test_case);
extern void simulate_stfs(uint32 *machine_code, struct testcase *test_case);
extern void simulate_stfd(uint32 *machine_code, struct testcase *test_case);
extern void simulate_stfsx(uint32 *machine_code, struct testcase *test_case);
extern void simulate_stfdx(uint32 *machine_code, struct testcase *test_case);


int dfpdadd( uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdaddq( uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdsub( uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdsubq( uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdmul( uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdmulq( uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpddiv( uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpddivq( uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdcmpo( uint64 *fpscr, uint64*cr, uint16 cr_bf, uint64 *opA, uint64 *opB);
int dfpdcmpoq( uint64 *fpscr, uint64* cr, uint16 cr_bf, uint64 *opA, uint64 *opB);
int dfpdcmpu( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint64 *opA, uint64 *opB);
int dfpdcmpuq( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint64 *opA, uint64 *opB);
int dfpdtstdc( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint16 DCM, uint64 *opA);
int dfpdtstdcq( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint16 DCM, uint64 *opA);
int dfpdtstdg( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint16 DCM, uint64 *opA);
int dfpdtstdgq( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint16 DCM, uint64 *opA);
int dfpdtstex( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint64 *opA, uint64 *opB);
int dfpdtstexq( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint64 *opA, uint64 *opB);
int dfpdtstsf( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint64 *opA, uint64 *opB);
int dfpdtstsfq( uint64 *fpscr, uint64 *cr, uint16 cr_bf, uint64 *opA, uint64 *opB);
int dfpdquai ( uint64 *fpscr, uint16 RMC, uint16 TE, uint64 *opB, uint64 *opT);
int dfpdquaiq( uint64 *fpscr, uint16 RMC, uint16 TE, uint64 *opB, uint64 *opT);
int dfpdqua( uint64 *fpscr, uint16 RMC,  uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdquaq( uint64 *fpscr, uint16 RMC,  uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdrrnd( uint64 *fpscr, uint16 RMC,  uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdrrndq( uint64 *fpscr, uint16 RMC,  uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdrintx (uint64 *fpscr, uint16 R, uint16 RMC, uint64 *opB, uint64 *opT);
int dfpdrintxq (uint64 *fpscr, uint16 R, uint16 RMC, uint64 *opB, uint64 *opT);
int dfpdrintn (uint64 *fpscr, uint16 R, uint16 RMC, uint64 *opB, uint64 *opT);
int dfpdrintnq (uint64 *fpscr, uint16 R, uint16 RMC, uint64 *opB, uint64 *opT);
int dfpdctdp (uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpdctqpq (uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpdrsp (uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpdrdpq (uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpdcffix( uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpdcffixq (uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpdctfix( uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpdctfixq( uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpddedpd (uint64 *fpscr, uint16 S, uint16 SP, uint64 *opB, uint64 *opT);
int dfpddedpdq (uint64 *fpscr, uint16 S, uint16 SP, uint64 *opB, uint64 *opT);
int dfpdenbcd (uint64 *fpscr, uint16 S, uint64 *opB, uint64 *opT);
int dfpdenbcdq(uint64 *fpscr, uint16 S, uint64 *opB, uint64 *opT);
int dfpdxex (uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpdxexq (uint64 *fpscr, uint64 *opB, uint64 *opT);
int dfpdiex (uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdiexq (uint64 *fpscr, uint64 *opA, uint64 *opB, uint64 *opT);
int dfpdscli (uint64 *fpscr, uint16 SH, uint64 *opA, uint64 *opT);
int dfpdscliq (uint64 *fpscr, uint16 SH, uint64 *opA, uint64 *opT);
int dfpdscri (uint64 *fpscr, uint16 SH, uint64 *opA, uint64 *opT);
int dfpdscriq (uint64 *fpscr, uint16 SH, uint64 *opA, uint64 *opT);

extern void set_CR_field(uint8 val, int fm, uint64 *cr);
int BCD2BCDnumber(const int, const int, const unsigned long long *, DFP_Operand *);
void SetFlags(struct dfp_context *);

/* 2463 - Decimal Floating-Point Support Operations */
int simulate_dtstsfi(uint32 * machine_code, struct testcase *test_case);
int simulate_dtstsfiq(uint32 * machine_code, struct testcase *test_case);

