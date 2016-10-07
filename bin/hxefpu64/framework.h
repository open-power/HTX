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

/* %Z%%M%	%I%  %W% %G% %U% */

#ifndef _HTX_FRAMEWORK_H
#define _HTX_FRAMEWORK_H
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <strings.h>
#include <memory.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef __HTX_LINUX__
	#include <sys/vminfo.h>
	#include <libperfstat.h>
	#include <sys/systemcfg.h>
#endif

#ifdef __HTX_LINUX__
	#include <sched.h>
	#include <sys/prctl.h>
	#include <sys/resource.h>
	#include <xscom.h>
#else
	#include <sys/dr.h>
#endif

#include <sys/stat.h>
#include <hxihtx64.h>
#include <htxsyscfg64.h>
#include "fpu_common_struct_prot.h"

#ifdef __HTX_LE__
#include "instr_form_LE.h"
#else
#include "instr_form_BE.h"
#endif

#ifdef SCTU
#define LOGFILE		"/tmp/sctu_log"
#define SCTU_PP_COUNT	(256)
#else
#define LOGFILE		"/tmp/fpu_log"
#endif

#define HSTRCMP strcasecmp
#define PROC_DD2   0x0200

typedef enum
{
	addi,
	addis,
	mfcr,
	or,
	ori,
	and,
	stfd,
	stxsdx,
	stxvd2x,
	mffs,
	vmx_stvx,
	stswi,
	oris,
	dcbt,
	mfspr,
	mtspr,
	crxor,
	creqv,
	branch,
	brlink,
	stbcx,
	sthcx,
	stwcx,
	stdcx,
	stswx,
	std,
	stmw,
	stqcx,
	lbzx,
	lhzx,
	lwzx,
	ldx,
	stbx,
	sthx,
	stwx,
	stdx,
	cmp
}dep_instr_list;



struct enabled_instruction {
	uint32 instr_index;
	uint8 run_flag;
	instruction_masks instr_table;
};

#define MAX_SRC_OPRS	(3)
struct decoded_instruction {
	int instr_type;					/* Load, store or other instruction type					*/
	int marked;						/* flag, indicating instruction influencing miscompare 		*/ 
	char mnemonic[50];				/* complete instruction in string format 					*/
    int ea_offset;					/* offset of load/store area in case of LS  instruction 	*/
	uint16 tgt_dtype;				/* target type 												*/
    uint16 tgt_val;					/* target value as updated by build routine 				*/
	uint16 op_dtype[MAX_SRC_OPRS];	/* source operand data type 								*/	
    uint16 op_val[MAX_SRC_OPRS];	/* source operand value 									*/
};
typedef struct decoded_instructions dc_instr_t;

struct reguse{
	uint16 dc_index;					/* instruction index in the decoded instruction table where the register is used as target 	*/
	int tgt_reg;						/* register number for this node 															*/
	struct reguse *src[MAX_SRC_OPRS]; 	/* operand register pointer who modify this register node 									*/
};

struct reguse_list {
	int num_entries;
	struct reguse *head;
};

#define OP_EOP_MASK_INDEX	0
#define OPERANDS_MASK_INDEX	1
#define OP1_DTYPE_INDEX		2
#define	OP1_POS_INDEX		3
#define OP2_DTYPE_INDEX		4
#define	OP2_POS_INDEX		5
#define OP3_DTYPE_INDEX		6
#define	OP3_POS_INDEX		7
#define	TGT_DTYPE_INDEX		8
#define	TGT_POS_INDEX		9
#define INS_CLASS_INDEX		10

/*
 * For now VSX_NUM_INSTRUCTIONS holds maximum no of VSX instructions.
 */
#define VSX_NUM_INSTRUCTIONS	250
#define MAX_INSTRUCTIONS		1500
#define NO_CPU_DR           	0x0

#ifndef __HTX_LINUX__
void sync(void);
void isync(void);
void tlbsync(void);
void icbi(volatile unsigned int *);
void dcbf(volatile unsigned int *);
void dcbi(volatile unsigned int *);


#pragma mc_func sync    { "7c0004ac" }          /* sync  */
#pragma mc_func isync   { "4c00012c" }          /* isync */
#pragma mc_func tlbsync { "7c00046c" }          /* tlbsync */
#pragma mc_func icbi    { "7C001FAC" }          /* icbi r3 */
#pragma mc_func dcbf    { "7C0018AC" }          /* dcbf r3 */
#pragma mc_func dcbi    { "7C001BAC" }          /* dcbi r3 */
#else
#define dcbf(x) __asm ("dcbf 0,%0\n\t": : "r" (x))
#define icbi(x) __asm ("icbi 0,%0\n\t": : "r" (x))
#define sync() __asm__ __volatile__ ("sync" : : : "memory");
#define isync() __asm__ __volatile__ ("isync" : : : "memory");
#endif

/*
 * Each of the array member points to a link list of VSRs containing
 * data types mentioned above.
 */
#define 	INS_TYPES	5
#define		VSX			0
#define		BFP			1
#define		DFP			2
#define 	VMX			3
#define 	CPU			4

#define 	INS_CAT		0xff00000000000000ULL

/*
 * Class of instructions. This helps in resolving some dependencies while building instructions stream.
 * It allows to let you take special care for set of instructions. Like pre, post dependencies.
 */
/* VSX Instruction Classes */
#define CLASS_VSX_NORMAL		0x0
#define CLASS_VSX_LOAD			0x1
#define CLASS_VSX_STORE			0x2
#define CLASS_VSX_MUL_ADD_SUB	0x3
#define CLASS_VSX_TEST_INS		0x4
#define CLASS_VSX_LOADU			0x5
#define CLASS_VSX_STOREU		0x6
#define CLASS_VSX_MOVE			0x7
#define CLASS_VSX_IMM			0x8
#define CLASS_VSX_LOAD_IMM		0x9
#define CLASS_VSX_STORE_IMM		0xa

/* BFP Instruction Classes */
#define CLASS_BFP_LOAD			0x10
#define CLASS_BFP_LOAD_IMM		0x11
#define CLASS_BFP_STORE			0x12
#define CLASS_BFP_STORE_IMM		0x13
#define CLASS_BFP_NORMAL		0x14
#define CLASS_BFP_FPSCR			0x15
#define CLASS_BFP_FPSCR_IMM		0x16
#define CLASS_BFP_CR_UPDATE		0x17
#define CLASS_BFP_FPSCR_2_CR	0x18
#define CLASS_BFP_FPSCR_BIT_SET_UNSET	0x19
#define CLASS_BFP_QP_ROUND		0x1A
#define CLASS_BFP_TEST_DATA		0x1B

/* DFP Instruction Classes */
#define CLASS_DFP_NORMAL		0x20
#define CLASS_DFP_CMP_INS		0x21
#define CLASS_DFP_CMP_TEST_INS	0x22
#define CLASS_DFP_RMC_QUAI		0x23
#define CLASS_DFP_RMC			0x24
#define CLASS_DFP_SH			0x25 /* Here we pick it as normal registers but we don't meedle the linklist*/
#define CLASS_DFP_LOAD_IMM		0x26
#define CLASS_DFP_LOAD			0x27
#define CLASS_DFP_STORE_IMM		0x28
#define CLASS_DFP_STORE			0x29

/* VMX Instruction classes */
#define CLASS_VMX_LOAD                  0x30
#define CLASS_VMX_STORE                 0x31
#define CLASS_VMX_NORMAL                0x32
#define CLASS_VMX_NORMAL_3INPUTS        0x33

#define CLASS_CPU_LOAD				0x40
#define CLASS_CPU_COND_LOG			0x41              /* conditional logical instructions */
#define CLASS_CPU_FIXED_ARTH        0x42              /* fixed arithmetic instructions 	  */
#define CLASS_CPU_FIXED_LOGIC_1		0x43              /* conditional logical instructions */
#define CLASS_CPU_FIXED_LOGIC_2		0x44              /* conditional logical instructions */
#define CLASS_CPU_FIXED_ARTH_1      0x45              /* fixed arithmetic instructions    */
#define CLASS_CPU_ROTATE       		0x46              /* rotate instructions              */
#define CLASS_CPU_ROTATE_1     		0x47              /* rotate instructions              */
#define CLASS_CPU_ROTATE_2     		0x48              /* rotate instructions              */
#define CLASS_CPU_ROTATE_3     		0x49              /* rotate instructions              */
#define CLASS_CPU_FIXED_ARTH_2 		0x50              /* shift instructions               */
#define CLASS_CPU_FIXED_ARTH_3 		0x51              /* shift instructions               */
#define CLASS_CPU_SPR_1        		0x52              /* special instructions             */
#define CLASS_CPU_SPR_2        		0x53              /* special instructions             */
#define CLASS_CPU_SPR_3        		0x54              /* special instructions             */
#define CLASS_CPU_STORE_1      		0x55              /* store instructions	              */
#define CLASS_CPU_STORE_2      		0x56              /* store instructions	              */
#define CLASS_CPU_FIXED_ARTH_4 		0x57              /* arithmetic instructions          */
#define CLASS_CPU_MIXED      		0x58              /* instructions without operands    */
#define CLASS_CPU_STORE_3      		0x59              /* store instructions	              */
#define CLASS_CPU_FIXED_LOGIC_3   	0x5A              /* compare like  instructions       */
#define CLASS_CPU_CACHE           	0x5B              /* mostly cache instructions        */
#define CLASS_CPU_CACHE_1          	0x5C              /* mostly cache instructions        */
#define CLASS_CPU_CACHE_2          	0x5D              /* mostly cache instructions        */
#define CLASS_CPU_BRANCH_1        	0x5E              /* branch instructions	      	  */
#define CLASS_CPU_BRANCH_2         	0x5F              /* branch instructions	      	  */
#define CLASS_CPU_BRANCH_3         	0x60              /* branch instructions	      	  */
#define CLASS_CPU_FIXED_LOAD_2		0x61		  	  /* Load instructions 		      	  */
#define CLASS_CPU_CACHE_3         	0x62              /* mostly cache instructions        */
#define CLASS_CPU_THREAD_PRI      	0x63              /* Thread Priority feature          */
#define CLASS_CPU_LOAD_1			0x64
#define CLASS_CPU_MIXED_1      		0x65              /* instructions without operands    */
#define CLASS_CPU_BRANCH_4          0x66              /* branch instructions          	  */

#define CLASS_CPU_LOAD_ATOMIC       0x67
#define CLASS_CPU_STORE_ATOMIC      0x68
#define CLASS_CPU_LOAD_RELATIVE     0x69
#define CLASS_CPU_STRING_OPS_FX     0x70
#define CLASS_CPU_MUL_ADD_DW     	0x71
#define CLASS_CPU_ARRAY_INDEX_SUPP  0x72

#define CLASS_LHL					0x100
#define CLASS_SHL					0x101
/*
 * 64 bit VSX reg file mask of dirty regs
 */
unsigned long vsx_reg_file_mask;
#define VSR_OP_TYPES 	15	/* Total no of data types dealt by VSU */
#define DUMMY			0
#define BFP_SP			1
#define BFP_DP			2
#define	BFP_QP			14
#define SCALAR_SP 		1	/* scalar SP data */
#define SCALAR_DP 		2	/* scalar DP data */
#define VECTOR_SP 		3	/* Vector SP data */
#define VECTOR_SP_HH	3	/* "SP || undefined || SP || undefined" data format */
#define VECTOR_DP 		4	/* Vector DP data */
#define GPR_T			5	/* 64 bit integer data */
#define GPR32			5	/* 32 bit integer data */
#define QGPR			5	/* 128 bit fixed point integer data */
#define QGPR_HH			5	/* "SW || Undefined || SW || undefined" data format */
#define SCALAR_HP		6	/* Half precision Scalar datatype */
#define VECTOR_HP		7	/* Half precision Vector datatype */
#define DFP_SHORT		8
#define	DFP_LONG		9
#define DFP_QUAD		10
#define CR_T			11	/* For test instructions where CR is target */
#define GR				12	/* Indicates GPR */
#define GRU				13	/* GPR which gets updates after instruction is completed */

#define BFP_OP_TYPES	VSR_OP_TYPES
#define VMX_OP_TYPES	VSR_OP_TYPES
#define DFP_OP_TYPES	VSR_OP_TYPES
#define GPR_OP_TYPES	1
#define	CPU_GPR			0
#define	SPR_REG			1
/* All IMM_DATA types should not conflict with any of VSR_OP_TYPES */
#define IMM_DATA		100
#define IMM_DATA_1BIT 	101
#define IMM_DATA_2BIT	102
#define IMM_DATA_3BIT  	103
#define IMM_DATA_4BIT	104
#define IMM_DATA_5BIT	105
#define IMM_DATA_7BIT	106
#define IMM_DATA_8BIT	107
#define IMM_DATA_12BIT	108
#define IMM_DATA_14BIT	109
#define SYNC_WORD 		8
#define TIME_STAMPS 	9
#define CPU_ID_MASKS 	10
#define SPRS_OFF 		0x500
#define CTX_OFF_REG 	4

#define ALIGN_B		1
#define ALIGN_HW	2
#define ALIGN_W		4
#define ALIGN_DW	8
#define ALIGN_QW	16
#define ALIGN_2QW   32

#define MOVE_VSR_TO_END(c, x, t)	{									\
			struct server_data *sdata = &global_sdata[INITIAL_BUF];		\
			struct vsr_list *vsrs;      								\
			struct vsr_node *local_tmp, *nxt_of_tail;					\
			vsrs = sdata->cdata_ptr[c]->vsrs; 							\
			if ( vsrs[x].head[t] != vsrs[x].tail[t] ) {					\
				local_tmp = vsrs[x].head[t];							\
				vsrs[x].head[t] = vsrs[x].head[t]->next;				\
				nxt_of_tail = vsrs[x].tail[t]->next;					\
				vsrs[x].tail[t]->next = local_tmp;						\
				vsrs[x].tail[t] = vsrs[x].tail[t]->next;				\
				vsrs[x].tail[t]->next = nxt_of_tail;					\
			}															\
		}

/*
 * GPRs which hold base and index for load/store. Tesecase will use GPR4 and GPR5 for load/store
 * base + index purpose.
 */
#define STORE_RA		5
#define STORE_RB		4
#define LOAD_RA			5
#define LOAD_RB			4
#define TC_BASE			3
#define GPR_TO_SAVE_CR	6
#define BASE_GPR		7
#define MAX_REG			31
#define MEM_OFFSET_BETWEEN_CLIENTS	(256*1024)
#define START_GPR_USED	11				/* GPR11 till GPR31 is used */
#define RANGE_GPRS		(NUM_GPRS - START_GPR_USED)     /* Range of GPRs used in CPU inst.*/

#define DEFAULT_SYNC_DISTANCE		10
#define DEFAULT_INS_STREAM_DEPTH	50
#define DEFAULT_NUM_SYNC_WORD    	(DEFAULT_INS_STREAM_DEPTH/DEFAULT_INS_STREAM_DEPTH)
#define MAX_NUM_SYNC_DWORDS			(MAX_INS_STREAM_DEPTH/2) /* every alternate ins is a sync point */

/* BFP Load and store macros */
#define STORE_BFP_DP(fpr, off)	(0xD8000000 | (fpr << 21) | (STORE_RA << 16) | off)
#define LOAD_BFP_SP(fpr, off)	(0xC0000000 | (fpr << 21) | (LOAD_RA << 16) | off)
#define LOAD_BFP_DP(fpr, off)	(0xC8000000 | (fpr << 21) | (LOAD_RA << 16) | off)

/* Using stxswx instruction to store 32 bit contents to memory pointed to by GPRs RA, RB */
#define STORE_32(v)	((0x7C000518 | (STORE_RA << 16) |  (STORE_RB << 11) ) | ((v & 0x1f) << 21) | ((v & 0x20) >> 5))
/* Using stxsdx instruction to store 64 bit contents to memory pointed to by GPRs RA, RB */
#define STORE_64(v)	(0x7C000598 | (STORE_RA << 16) |  (STORE_RB << 11) | ((v & 0x1f) << 21) | ((v & 0x20) >> 5))
/* Using stxvd2x instruction to store 128 bit contents to memory pointed to by GPRs RA, RB */
#define STORE_128(v)((0x7C000798) | (STORE_RA << 16) |  (STORE_RB << 11) | ((v & 0x1f) << 21) | ((v & 0x20) >> 5))

/* array and a macros together will provide a machine code for store instruction given the vsr no */
/* uint32 store_machine_code_mask[VSR_OP_TYPES] = {0, STORE_32, STORE_64, STORE_128, STORE_128, STORE_64,
												STORE_64, STORE_32, STORE_128, STORE_128}; */
/* #define STORE_MCODE(dt, v) (store_machine_code_mask[dt].mask | ((v & 0x1f) << 21) | ((v & 0x20) >> 5)) */

/* using instruction to store 128 bit contents to memory pointed by GPRs RA, RB */
#define STORE_VMX_128(x)       ((0x7C0001CE) | (STORE_RA << 16) |  (STORE_RB << 11) | ((x & 0x1f) << 21))

#define	 NORMAL 		0
#define	 DENORMAL		1
#define	 QNAN			2
#define	 NEAR_INF		3
#define	 INF			4
#define	 SNAN			5
#define	 BFP_ZERO		6
#define	 DENORM_TOW_Z	7
#define	 DENORM_TOW_N	8
#define	 NORM_TOW_DN	9

#ifdef SCTU
#define FIRST_CORE			0
#define FIRST_CORE_INDEX	1
#define LAST_CORE			2
#define LAST_CORE_INDEX		3
#define MIN_SMT_GANG		4
#define MIN_SMT_CORE_INDEX	5
#define GANG_SIZE			6
#endif


#define MAX_BIAS_LIST	20
struct ins_biasing {
	uint64	vsx_mask;
	uint64	bfp_mask;
	uint64	dfp_mask;
	uint64  vmx_mask;
	uint64  cpu_mask;
	uint64  macro_mask;
	uint64	bias_mask[MAX_BIAS_LIST][2];
};
#define MAX_NUM_RULES		100
#define BFP_DATA_BIAS_TYPES 10
struct ruleinfo {
	uint32					num_oper;
    uint32		 			stream_depth;
    uint32					testcase_sync;
    uint32					sync_distance;
    uint8				 	db[MAX_NUM_CPUS][BFP_DATA_BIAS_TYPES];
	struct ins_biasing 		ib[MAX_NUM_CPUS];
	uint8					unaligned_data_pc[MAX_NUM_CPUS];
	int32					parent_seed;
	int32					seed[MAX_NUM_CPUS];
	uint32					dump_testcase[MAX_NUM_CPUS];
    uint32 					page_size;		/* Page size used for populating segment */
#define CONSISTENCY_CHECK	1
#define CORRECTNESS_CHECK	2
    uint32					test_method;	/* 1 - conistancy check, 2 - correctness check */
    uint64					fpscr[MAX_NUM_CPUS];
	uint32 					num_threads;    /* Number of threads to be created by the testcase */
	uint32					prot_sao;		/* Strong access ordering for testcase execution */
#define	SYSTEM_TEST			0
#define CHIP_TEST			1
#define NODE_TEST			2
#define INTERNODE_TEST		3
	uint32					testcase_type;	/* Default/system, chip, node, internode etc */
	uint32					enable_attn;
	uint32					enable_trap;
	char					rule_id[20];
	int                     num_pass2;      /* number of PASS2 within a num oper */
	unsigned int            tc_time;        /* testcase execution time */
	int                     log_seed;       /* flag to indicate seed to be logged into file */
	int                     compare_flag;   /* switch to enable / disable comparison */
};

struct common_data {
	uint32 sync_distance;	/* number of instructions between two sync points */
	uint32 stream_depth;	/* Number of test instructions to be generated */
   	uint32 num_sync_points;
   	volatile uint32 sync_words[MAX_NUM_SYNC_DWORDS];
   	uint32 tc_ins_size;	/* Size of text area for this client */
#ifdef SCTU
	char ls_ptr[2*MAX_INS_STREAM_DEPTH*16];
#endif
};

/*
 * server_data is one global struct which will have reach to most of the data structures.
 */

struct shm_buf {
    int id;
    char *ptr;
    key_t key;
    unsigned int page_size;
    unsigned int seg_size;
};

struct vsr_node {
	uint32 vsr_no;
	struct vsr_node *next;
};
struct vsr_list {
	struct vsr_node *head[INS_TYPES];
	struct vsr_node *tail[INS_TYPES];
	uint64 dirty_mask;
	uint64 used_mask;
	unsigned char num_vsrs;
};
struct vsr_data_type {
    uint32 data_type;
}vsrs_dt[NUM_VSRS];

struct ins_category {
     uint16 ptr[MAX_INSTRUCTIONS];
     uint32 start_index;
     uint32 end_index;
     uint64 mask;
     uint64 bias_num;
};

struct signal_info {
	uint32 *iar;
	int sig_num;
	int client_num;
	int pass_num;
	int32 client_seed;
	int32 parent_seed;
	struct tc_context tcc;
};

#define MAX_MASK_LIST	6
#define CLIENT_NUM_BYTES_IN_CL 16
#define MAX_ADDI_OFFSET 0x7fff
#define CACHE_LINE_SIZE 0x80
/*
 * Following is a layout of a shared memory. There are three copies of the same layout, one for
 * initial, pass1 and pass2 each.
 *
 *	----------------------------------
 *	client #1 testcase context
 *		VSRs
 *		GPRs
 *		SPRs
 *			FPSCR, CR, CTR, LR, XER, MSR
 *		compare status
 *		...
 *		testcase code
 *		...
 *		simulation pointers corresponding to each instructions in testcase code above
 *		...
 *		load memory area
 *		store memory area
 *	----------------------------------
 *		****
 *		****
 *	----------------------------------
 *	client #n testcase context
 *		VSRs
 *		GPRs
 *		SPRs
 *			FPSCR, CR, CTR, LR, XER, MSR
 *		compare status
 *		...
 *		testcase code
 *		...
 *		simulation pointer corresponding to each instructions in testcase code above
 *		...
 *		load memory area
 *		store memory area
 *	-------------------------------
 *	stream depth
 *	sync distance
 *	sync words
 *	-------------------------------
 */
struct client_data {
	struct testcase 	*tc_ptr[3];
	char 				*ls_current[3];
	char 				*ls_base[3];
	char				*ls_base_volatile[3]; /* hold LS base address after rebase */
#ifdef SCTU 
	char				*ls_cl_start[3];
#endif
	uint32				num_oper;
	uint64				num_pass2;
	uint32				client_no;
	uint32				bcpu;
#ifdef __HTX_LINUX__
	int				phy_bcpu;	/* Used for Hot Plug support on Linux. */
#endif
	pthread_t			tid;
	pthread_attr_t		attr;
	struct vsr_list		vsrs[VSR_OP_TYPES + 5];
	db_fptr				value_gen[VSR_OP_TYPES][BFP_DATA_BIAS_TYPES];
	struct drand48_data	rand_buf;
	int32				original_seed;
	uint32				prolog_size;
	uint32				epilog_size;
	uint32				num_ins_built;
	uint8				vsr_reg_wt[VSR_OP_TYPES];
	uint8				bfp_reg_wt[BFP_OP_TYPES + 1];
	uint8				vmx_reg_wt[BFP_OP_TYPES];
	uint16				vsx_reg[NUM_VSRS-1]; /* NUM_VSRS - 1 because VSR 0 is not used */
	/* will be passed to test case */
	uint64 				time_stamps[MAX_NUM_SYNC_DWORDS];
	/* will be passed to test case */
	uint64				cpu_id_mask;
	/* contains the bias category nos according to the biasing specified in the rule file */
	uint8				ins_bias_array[100];
	uint16				rand_nos[100];
	/* ptr to master instruction table */
	struct instruction_masks *ins_table_ptr;
	/* Number of enabled instructions after applying the mask */
	int 				num_enabled_ins;
	/* ptr to enabled instruction table which is derived after applying masks */
	struct enabled_instruction enabled_ins_table[MAX_INSTRUCTIONS];
	struct decoded_instruction dc_instr[MAX_INS_STREAM_DEPTH];
	struct reguse_list  vsr_usage;
	struct reguse_list  gpr_usage;
	struct reguse_list  ls_usage;
	int 				instr_index[MAX_INS_STREAM_DEPTH];
	/* Instructions masks list as read from rule file */
	/* Instructions bias list */
	uint64				bias_list[MAX_BIAS_LIST][2];
	struct ins_category manage_bias_list[MAX_BIAS_LIST];
	uint64				num_bias_cat;
	uint32				tc_jmp_off; 					/* This value is in bytes */
	uint32				sim_jmp_off;					/* This value is in 4B words */
	uint32				sim_end_off;					/* This value is in 4B words */
	uint8				data_alignment[100];
	uint8				data_alignment_ctr;
	uint32				last_ls_off;
	uint32				ls_miscom_off_list[MAX_INS_STREAM_DEPTH];
	FILE				*clog;
	char				logfile[200];
#define 	NUM_CAT			5
	uint16				bias_for_ins_cat[NUM_CAT];
	uint16				gpr_reg[RANGE_GPRS];
	uint16				bdy_req;					/* alignment requirement for cpu instructions */
	struct signal_info	sigdata;
	uint16				reinit_base; /* flag for LS area rebase */
#ifdef SCTU
	uint32				last_rebase_index;
	uint8				miscompare_in_numoper;
#endif
};
typedef struct client_data client_data;

/* per thread struct */
struct client_instr_stats {
	int num_enabled_instr;
   	int num_tested_instr;
   	int tested_percent;
};
   
/* for per stanza update */
struct stanza_instr_stats {
	int test_id;
	struct client_instr_stats client_stats[MAX_NUM_CPUS];
};
   

struct server_data {
    struct client_data 		*cdata_ptr[MAX_NUM_CPUS];
    struct common_data		*common_data_ptr;
    struct shm_buf		*shm_buf_ptr;
    uint32			num_clients;
    pthread_mutex_t		clients_sync_lock;
};
extern struct server_data global_sdata[];

/*
 * structure for keeping shared memory related info. There will be three such shared mem areas viz
 * Ref, pass1 and pass2.
 */
#define INITIAL_BUF		0
#define PASS1_BUF		1
#define PASS2_BUF		2

/*
 * client_data is created for each client that runs the test on specific cpu.
 */
#define GEN_ADDI_MCODE(rt, ra, offset)	((14U << 26)| (rt << 21) | (ra << 16) | (offset))
#define ADDIS(rt,ra,si)					(15U << 26 | rt << 21 | ra << 16 | si)
#define MFCR(rt)   					 	(31<<26 | rt << 21 | 19<<1)
#define MTCRF(FXM,rs)					(31U << 26 | rs << 21 | FXM << 12 | 144U << 1)
#define STORE_GPR(rt,ra,rb) 			(31<<26 | rt  << 21 | ra << 16 | rb << 11 | 151<<1)
#define STORE_GPR64(rt,ra,rb) 			(31<<26 | rt  << 21 | ra << 16 | rb << 11 | 149<<1)
#define MFFS(rt)						(63U << 26 | rt << 21 |  583U << 1)
#define OR(rs, rt)						(0x7C000378 | rs << 21 | rt << 16 | rs << 11)
#define MFSPR(rt, spr)					(31U << 26 | rt << 21 | spr << 11 | 339U << 1)
#define LD(rd, ra, d)					(58U << 26 | rd << 21 | ra << 16 | (((short)d) & 0xfffc))
#define LDX(rd, ra, rb)					(31U << 26 | rd << 21 | ra << 16 | rb << 11 | 21U << 1)
#define LXZX(rd, ra, rb, eopcode)		(31U << 26 | rd << 21 | ra << 16 | rb << 11 | eopcode << 1)
#define STXX(rs, ra, rb, eopcode)		(31U << 26 | rs << 21 | ra << 16 | rb << 11 | eopcode << 1)
#define STD(rs, ds, ra)					(62U << 26 | rs << 21 | ra << 16 | (((short)ds) & 0xfffc))
#define STDX(rs, ra, rb)				(31U << 26 | rs << 21 | ra << 16 | rb << 11 | 149U << 1)
#define LWARX(rt, ra, rb, eh)			(31U << 26 | rt << 21 | ra << 16 | rb << 11 | 20U << 1 | eh)
#define ANDC(rs, ra, rb, rc)			(31U << 26 | rs << 21 | ra << 16 | rb << 11 | 60U << 1 | rc)
#define STBCX(rs, ra, rb)				(31U << 26 | rs << 21 | ra << 16 | rb << 11 | 694U << 1 | 1)
#define STHCX(rs, ra, rb)				(31U << 26 | rs << 21 | ra << 16 | rb << 11 | 726U << 1 | 1)
#define STWCX(rs, ra, rb)				(31U << 26 | rs << 21 | ra << 16 | rb << 11 | 150U << 1 | 1)
#define BC(bo, bi, bd)					(16U << 26 | bo << 21 | bi << 16 | (bd & 0xfffc))
#define SYNC(l)							(31U << 26 | l << 22 | 598U << 1)
#define ISYNC 							0x4c00012c
#define LWZ(rt, ra, d)					(32U << 26 | rt << 21 | ra << 16 | (((short)d) & 0x0000ffff))
#define CMP(crf, l, ra, rb)				(31U << 26 | crf << 23 | l << 21 | ra << 16 | rb << 11)
#define CMPLI(bf, l, ra, ui)			(10U << 26 | bf << 23 | l << 21 | ra << 16 | ui)
#define ADDIC(rt, ra, d)				(12U << 26 | rt < 21 | ra << 16 | (short)d)
#define BL(li)							(18U << 26 | (((int)li) & 0x3fffffc) | 1)
#define BLR								0x4e800020
#define MTFSF(flm, frb, l , w, rc)		(63U << 26 | (l << 25) | (flm & 0xff) << 17 | (w << 16) | frb << 11 | 711U << 1 | rc)
#define MTVSCR(vrb)						(4U << 26 | vrb << 11 | 1604)
#define MFVSCR(vrt)						(4U << 26 | vrt << 21 | 1540)
#define LFS(frt, ra, d)					(48U << 26 | frt << 21 | ra << 16 | (((short)d) & 0xffff))
#define LFD(frt, ra, d)					(50U << 26 | frt << 21 | ra << 16 | (((short)d) & 0xffff))
#define LFSX(frt,ra,rb) 				(31U << 26 | frt << 21 | ra << 16 | rb << 11 | 535U << 1)
#define LFDX(frt,ra,rb) 				(31U << 26 | frt << 21 | ra << 16 | rb << 11 | 599U << 1)
#define LVX(vrt, ra, rb)				(31U << 26 | vrt << 21 | ra << 16 | rb << 11 | 103U << 1)
#define STFS(frs, ra, d)				(52U << 26 | frs << 21 | ra << 16 | (((short)d) & 0xffff))
#define STFD(frs, ra, d)				(54U << 26 | frs << 21 | ra << 16 | (((short)d) & 0xffff))
#define STFSX(frs,ra,rb)				(31U << 26 | frs << 21 | ra << 16 | rb << 11 | 663U << 1)
#define STFDX(frs,ra,rb)				(31U << 26 | frs << 21 | ra << 16 | rb << 11 | 727U << 1)
#define STVX(vrs,ra,rb)					(31U << 26 | vrs << 21 | ra << 16 | rb << 11 | 231U << 1)
#define AND(ra,rs,rb,rc)				(31U << 26 | rs << 21 | ra << 16 | rb << 11 | 28U << 1 | rc)
#define ORI(ra,rs,ui)					(24U << 26 | rs << 21 | ra << 16 | ui)
#define MTSPR(spr,rs)					(31U << 26 | rs << 21 | spr << 11 | 467U << 1)
#define LXVD2X(xt, ra, rb)				(31U << 26 | (xt & 0x1f) << 21 | ra << 16 | rb << 11 | 844U << 1 | (xt & 0x20) >> 5)
#define STXVD2X(xs, ra, rb) 			(31U << 26 | (xs & 0x1f) << 21 | ra << 16 | rb << 11 | 972U << 1 | (xs & 0x20) >> 5)
#define STXSDX(xs, ra, rb)				(31U << 26 | (xs & 0x1f) << 21 | ra << 16 | rb << 11 | 716U << 1 | (xs & 0x20) >> 5)
#define STSWI(nb,ra,rt)               	(31U << 26 | rt << 21 | ra << 16 | nb << 11 | 725U << 1)
#define ORIS(ra,rs,ui)					(25U << 26 | rs << 21 | ra << 16 | ui)
#define DCBT(th,ra,rb)   				(31U << 26 | th << 21 | ra << 16 | rb << 11 | 278U << 1)
#define CREQV(crd,cra,crb)   			(19U << 26 | crd << 21 | cra << 16 | crb << 11 | 289U << 1)
#define CRXOR(crd,cra,crb)   			(19U << 26 | crd << 21 | cra << 16 | crb << 11 | 193U << 1)
#define CRXOR(crd,cra,crb)   			(19U << 26 | crd << 21 | cra << 16 | crb << 11 | 193U << 1)
#define BRANCH(distance)   			    ((18U << 26) | ((distance) << 2))
#define BRLINK(distance)   			    (18U << 26 | distance << 2 | 0x00000001)
#define STDCX(RT,RA,RB)    			    (31U << 26 | RT << 21 | RA << 16 | RB << 11 | 429)
#define STQCX(RT,RA,RB)    			    (31U << 26 | RT << 21 | RA << 16 | RB << 11 | 365)
#define STSWX(op1,op2,tgt)              (31U << 26 | tgt << 21 | op2 << 16 | op1 << 11 | 661U << 1)
#define STMW(op1,op2,tgt)               (47U << 26 | tgt | op1 << 21 | op2 << 16 )

#define GET_REGISTER_VALUE(MSB, LSB)   	(( MSB<<5 )| LSB)

#define	FC_ADDI	     		0x1
#define FC_STORE			0x2
#define	FC_ADDI_IMM    		0x3
#define FC_INST_LOAD    	0x4
#define FC_INST_STORE   	0x5
#define FC_INST_LOAD_EH    	0x6

/*
 * Following are categories for Instructions masking
 */
#define P9_ONLY								0x0010000000000000ULL
#define VSX_CAT								0x0100000000000000ULL
#define BFP_ONLY							0x0200000000000000ULL
#define DFP_ONLY							0x0300000000000000ULL
#define VSX_ONLY 							VSX_CAT
#define CPU_ONLY							0x0500000000000000ULL
#define MACRO_ONLY							0x0600000000000000ULL

#define VSX_SCALAR_DP_LOAD_ONLY				0x1				|VSX_CAT
#define VSX_SCALAR_DP_STORE_ONLY			0x2				|VSX_CAT
#define VSX_SCALAR_DP_MOVE_ONLY				0x4				|VSX_CAT
#define VSX_SCALAR_DP_FP_ARITHMETIC_ONLY	0x8				|VSX_CAT
#define VSX_SCALAR_DP_FP_MUL_ADD_ONLY		0x10			|VSX_CAT
#define VSX_SCALAR_DP_FP_COMP_ONLY			0x20			|VSX_CAT
#define VSX_SCALAR_DP_FP_MAX_MIN_ONLY		0x40			|VSX_CAT
#define VSX_SCALAR_SP_CONV_DP_ONLY		 	0x80			|VSX_CAT
#define VSX_SCALAR_DP_CONV_SP_ONLY			0x100			|VSX_CAT
#define VSX_SCALAR_DP_CONV_FP2INT_ONLY		0x200			|VSX_CAT
#define VSX_SCALAR_DP_CONV_INT2FP_ONLY		0x400			|VSX_CAT
#define VSX_SCALAR_DP_RND_2_FPINT_ONLY		0x800			|VSX_CAT

#define VSX_VECTOR_SP_LOAD_ONLY				0x1000			|VSX_CAT
#define VSX_VECTOR_DP_LOAD_ONLY				0x2000			|VSX_CAT
#define VSX_VECTOR_SP_STORE_ONLY			0x4000			|VSX_CAT
#define VSX_VECTOR_DP_STORE_ONLY			0x8000			|VSX_CAT
#define VSX_VECTOR_SP_MOVE_ONLY				0x10000			|VSX_CAT
#define VSX_VECTOR_DP_MOVE_ONLY				0x20000			|VSX_CAT
#define VSX_VECTOR_SP_FP_ARITHMETIC_ONLY	0x40000			|VSX_CAT
#define VSX_VECTOR_DP_FP_ARITHMETIC_ONLY	0x80000			|VSX_CAT
#define VSX_VECTOR_SP_FP_MUL_ADD_ONLY		0x100000		|VSX_CAT
#define VSX_VECTOR_DP_FP_MUL_ADD_ONLY		0x200000		|VSX_CAT
#define VSX_VECTOR_SP_FP_COMP_ONLY			0x400000		|VSX_CAT
#define VSX_VECTOR_DP_FP_COMP_ONLY			0x800000		|VSX_CAT
#define VSX_VECTOR_SP_FP_MAX_MIN_ONLY		0x1000000		|VSX_CAT
#define VSX_VECTOR_DP_FP_MAX_MIN_ONLY		0x2000000		|VSX_CAT
#define VSX_VECTOR_SP_CONV_DP_ONLY			0x4000000		|VSX_CAT
#define VSX_VECTOR_DP_CONV_SP_ONLY			0x8000000		|VSX_CAT
#define VSX_VECTOR_SP_CONV_FP2INT_ONLY		0x10000000		|VSX_CAT
#define VSX_VECTOR_DP_CONV_FP2INT_ONLY		0x20000000		|VSX_CAT
#define VSX_VECTOR_SP_CONV_INT2FP_ONLY		0x40000000		|VSX_CAT
#define VSX_VECTOR_DP_CONV_INT2FP_ONLY		0x80000000		|VSX_CAT
#define VSX_VECTOR_SP_RND_2_FPINT_ONLY		0x100000000		|VSX_CAT
#define VSX_VECTOR_DP_RND_2_FPINT_ONLY		0x200000000		|VSX_CAT
#define VSX_MISC_ONLY						0x400000000		|VSX_CAT /* logical, merge, splat, permute, shift */
#define VSX_SCALAR_SP_LOAD_ONLY				0x800000000     |VSX_CAT
#define VSX_SCALAR_SP_STORE_ONLY			0x1000000000    |VSX_CAT
#define VSX_SCALAR_CONV_HP_DP_ONLY			0x2000000000	|VSX_CAT	| P9_ONLY
#define VSX_VECTOR_CONV_HP_SP_ONLY			0x4000000000	|VSX_CAT	| P9_ONLY
#define VSX_VECTOR_LENGTH_LOAD_ONLY			0x8000000000	|VSX_CAT	| P9_ONLY
#define VSX_VECTOR_LENGTH_STORE_ONLY		0x10000000000	|VSX_CAT	| P9_ONLY
#define P9_VSX_SCALAR_DP_MOVE_ONLY			(VSX_SCALAR_DP_MOVE_ONLY | P9_ONLY)
#define P9_VSX_MISC_ONLY				 	(VSX_MISC_ONLY  | P9_ONLY)


#define VSX_LOAD_ALL VSX_SCALAR_SP_LOAD_ONLY | VSX_SCALAR_DP_LOAD_ONLY | VSX_VECTOR_SP_LOAD_ONLY | VSX_VECTOR_DP_LOAD_ONLY
#define VSX_STORE_ALL VSX_SCALAR_SP_STORE_ONLY | VSX_SCALAR_DP_STORE_ONLY | VSX_VECTOR_SP_STORE_ONLY | VSX_VECTOR_DP_STORE_ONLY
#define VSX_LOAD_STORE_ALL VSX_LOAD_ALL | VSX_STORE_ALL

#define VSX_MOVE_ALL VSX_SCALAR_DP_MOVE_ONLY | VSX_VECTOR_SP_MOVE_ONLY | VSX_VECTOR_DP_MOVE_ONLY

#define VSX_ARITHMETIC_ALL VSX_SCALAR_DP_FP_ARITHMETIC_ONLY|VSX_VECTOR_SP_FP_ARITHMETIC_ONLY|VSX_VECTOR_DP_FP_ARITHMETIC_ONLY
#define VSX_MAX_MIN_ALL VSX_SCALAR_DP_FP_MAX_MIN_ONLY | VSX_VECTOR_SP_FP_MAX_MIN_ONLY | VSX_VECTOR_DP_FP_MAX_MIN_ONLY
#define VSX_MUL_ADD_ALL VSX_SCALAR_DP_FP_MUL_ADD_ONLY | VSX_VECTOR_SP_FP_MUL_ADD_ONLY |VSX_VECTOR_DP_FP_MUL_ADD_ONLY
#define VSX_CMP_ALL VSX_SCALAR_DP_FP_COMP_ONLY | VSX_VECTOR_SP_FP_COMP_ONLY | VSX_VECTOR_DP_FP_COMP_ONLY
#define VSX_RND_ALL VSX_SCALAR_DP_RND_2_FPINT_ONLY | VSX_VECTOR_SP_RND_2_FPINT_ONLY | VSX_VECTOR_DP_RND_2_FPINT_ONLY
#define VSX_CONV_ALL VSX_SCALAR_SP_CONV_DP_ONLY | VSX_SCALAR_DP_CONV_SP_ONLY | VSX_SCALAR_DP_CONV_FP2INT_ONLY | VSX_SCALAR_DP_CONV_INT2FP_ONLY | VSX_VECTOR_SP_CONV_DP_ONLY | VSX_VECTOR_DP_CONV_SP_ONLY | VSX_VECTOR_SP_CONV_FP2INT_ONLY | VSX_VECTOR_DP_CONV_FP2INT_ONLY | VSX_VECTOR_SP_CONV_INT2FP_ONLY | VSX_VECTOR_DP_CONV_INT2FP_ONLY
/*
 * Following are categories for VSX instructions biasing
 */
#define VSX_ALL										0x010001ffffffffffULL
#define VSX_VECTOR_SP_ONLY							0x0100000155555000ULL
#define VSX_VECTOR_DP_ONLY							0x01000002aaaaa000ULL
#define VSX_SCALAR_DP_ONLY							0x0100018000000fffULL
#define VSX_SCALAR_SP_ONLY							0x0100001800000180ULL
#define VSX_HP_ONLY									VSX_SCALAR_CONV_HP_DP_ONLY | VSX_VECTOR_CONV_HP_SP_ONLY
#define VSX_SP_ONLY									VSX_VECTOR_SP_ONLY_CAT | VSX_SCALAR_SP_ONLY_CAT
#define VSX_DP_ONLY									VSX_VECTOR_DP_ONLY_CAT | VSX_SCALAR_DP_ONLY_CAT
#define VSX_VECTOR_ONLY								VSX_VECTOR_SP_ONLY_CAT | VSX_VECTOR_DP_ONLY_CAT
#define VSX_SCALAR_ONLY								VSX_SCALAR_DP_ONLY_CAT | VSX_SCALAR_SP_ONLY_CAT
#define VSX_QGPR_MASK (VSX_CAT | VSX_MISC_ONLY | VSX_SCALAR_DP_CONV_FP2INT_ONLY | VSX_SCALAR_DP_CONV_INT2FP_ONLY | VSX_VECTOR_SP_FP_COMP_ONLY | VSX_VECTOR_DP_FP_COMP_ONLY | VSX_VECTOR_SP_CONV_FP2INT_ONLY | VSX_VECTOR_DP_CONV_FP2INT_ONLY | VSX_VECTOR_SP_CONV_INT2FP_ONLY | VSX_VECTOR_DP_CONV_INT2FP_ONLY)


#define			BFP_LOAD_SP				0x1						|BFP_ONLY
#define 		BFP_LOAD_DP				0x2						|BFP_ONLY
#define 		BFP_LOAD_INT			0x4						|BFP_ONLY
#define 		BFP_STORE_SP			0x8						|BFP_ONLY
#define 		BFP_STORE_DP			0x10					|BFP_ONLY
#define 		BFP_STORE_INT			0x20					|BFP_ONLY
#define			BFP_ELEM_ARITH_SP		0x40					|BFP_ONLY
#define			BFP_ELEM_ARITH_DP		0x80					|BFP_ONLY
#define			BFP_ADD_MUL_SP			0x100					|BFP_ONLY
#define			BFP_ADD_MUL_DP			0x200					|BFP_ONLY
#define			BFP_MOVE_ONLY			0x400					|BFP_ONLY
#define			BFP_ROUND_2_SP			0x800					|BFP_ONLY
#define			BFP_ROUND_2_INT			0x1000					|BFP_ONLY
#define			BFP_CONV_2_INT			0x2000					|BFP_ONLY
#define			BFP_CONV_FROM_INT		0x4000					|BFP_ONLY
#define			BFP_COMPARE_ONLY		0x8000					|BFP_ONLY
#define			BFP_SELECT_ONLY			0x10000					|BFP_ONLY
#define			BFP_FPSCR_ONLY			0x20000					|BFP_ONLY
#define 		BFP_TEST_ONLY			0x40000					|BFP_ONLY
#define 		P9_BFP_MOVE_ONLY		BFP_MOVE_ONLY			|BFP_ONLY	|P9_ONLY
#define			P9_BFP_COMPARE_ONLY		BFP_COMPARE_ONLY		|BFP_ONLY   |P9_ONLY
#define			P9_BFP_CONV_ONLY		0x80000					|BFP_ONLY	|P9_ONLY
#define			P9_BFP_ELEM_ARITH_QP	0x100000				|BFP_ONLY	|P9_ONLY
#define			P9_BFP_ADD_MUL_QP		0x200000				|BFP_ONLY   |P9_ONLY
#define			P9_BFP_ROUND_2_INT		BFP_ROUND_2_INT			|BFP_ONLY   |P9_ONLY
#define			P9_BFP_TEST_ONLY		BFP_TEST_ONLY			|BFP_ONLY   |P9_ONLY

#define			BFP_LOAD_ALL			0x7						|BFP_ONLY
#define			BFP_STORE_ALL			0x38					|BFP_ONLY
#define			BFP_LOAD_STORE_ALL		0x3f					|BFP_ONLY
#define			BFP_LOAD_STORE_SP_ONLY	0x9						|BFP_ONLY
#define			BFP_LOAD_STORE_DP_ONLY	0x12					|BFP_ONLY
#define			BFP_LOAD_STORE_INT_ONLY	0x24					|BFP_ONLY
#define			BFP_ARITH_SP_ALL		0x140					|BFP_ONLY
#define			BFP_ARITH_DP_ALL		0x280					|BFP_ONLY
#define			BFP_ARITH_ALL			0x3c0					|BFP_ONLY
#define			BFP_MISC_ALL			0x3fc00					|BFP_ONLY
#define			BFP_SP_ALL				BFP_LOAD_STORE_SP_ONLY  |BFP_ARITH_SP_ALL |BFP_ROUND_2_SP|BFP_CONV_2_INT|BFP_CONV_FROM_INT
#define			BFP_DP_ALL				BFP_LOAD_STORE_DP_ONLY  |BFP_ARITH_DP_ALL |0x7fc24
#define			BFP_QP_ALL				P9_BFP_MOVE_ONLY|P9_BFP_COMPARE_ONLY|P9_BFP_CONV_ONLY|P9_BFP_ELEM_ARITH_QP|P9_BFP_ADD_MUL_QP|P9_BFP_ROUND_2_INT|P9_BFP_TEST_ONLY
#define			BFP_ALL					0x3fffff				|BFP_ONLY
#define			CPU_ALL					(0x3fff					|CPU_ONLY)

#define	DFP_ONLY				0x0300000000000000ULL
/* Add , subtract , multiply , divide */
#define DFP_AIRTH_QUAD 				0x1				|DFP_ONLY
#define	DFP_AIRTH_LONG				0x2				|DFP_ONLY

#define	DFP_CMP_QUAD				0x4				|DFP_ONLY
#define	DFP_CMP_LONG				0x8				|DFP_ONLY

#define	DFP_TEST_QUAD				0x10			|DFP_ONLY
#define	DFP_TEST_LONG				0x20			|DFP_ONLY

#define	DFP_QUAN_QUAD				0x40			|DFP_ONLY
#define	DFP_QUAN_LONG				0x80			|DFP_ONLY

#define	DFP_RERND_QUAD				0x100			|DFP_ONLY
#define	DFP_RERND_LONG				0x200 			|DFP_ONLY

#define	DFP_RND_QUAD_2FP			0x400			|DFP_ONLY
#define	DFP_RND_LONG_2FP			0x800			|DFP_ONLY

#define	DFP_CONV_S2LONG				0x1000			|DFP_ONLY
#define	DFP_CONV_L2QUAD				0x2000			|DFP_ONLY
#define	DFP_CONV_L2SRT				0x4000			|DFP_ONLY
#define	DFP_CONV_Q2LNG				0x8000			|DFP_ONLY

#define	DFP_CONV_FIXED_FROM_QUAD	0x10000			|DFP_ONLY
#define	DFP_CONV_2FIXED_LONG		0x20000			|DFP_ONLY
#define	DFP_CONV_2FIXED_QUAD		0x40000			|DFP_ONLY

#define	DFP_DPD_2BCD_QUAD			0x80000			|DFP_ONLY
#define	DFP_DPD_2BCD_LONG			0x100000		|DFP_ONLY

#define	DFP_BCD_2DPD_QUAD			0x200000		|DFP_ONLY
#define	DFP_BCD_2DPD_LONG			0x400000		|DFP_ONLY

#define	DFP_INRT_EXRT_BIAS_LONG		0x800000		|DFP_ONLY
#define	DFP_INRT_EXRT_BIAS_QUAD		0x1000000		|DFP_ONLY

#define	DFP_SHIFT_LONG 				0x2000000		|DFP_ONLY
#define	DFP_SHIFT_QUAD				0x4000000		|DFP_ONLY

#define DFP_LOAD_LONG				0x8000000		|DFP_ONLY
#define DFP_STORE_LONG				0x10000000		|DFP_ONLY

#define	DFP_AIRTH_ALL				0x3				|DFP_ONLY
#define	DFP_CMP_ALL					0xC				|DFP_ONLY
#define	DFP_TEST_ALL				0x30			|DFP_ONLY
#define	DFP_QUAN_ALL				0xC0			|DFP_ONLY

#define	DFP_RERND_ALL				0x300			|DFP_ONLY
#define	DFP_RND_2FP_ALL 			0xC00			|DFP_ONLY
#define	DFP_CONV_SLQ				0xf000			|DFP_ONLY
#define	DFP_CONV_FIXED				0x70000			|DFP_ONLY
#define	DFP_CONV_ALL				0x7f000			|DFP_ONLY
#define	DFP_RND_CONV_ALL			0x7ff00			|DFP_ONLY

#define	DFP_DPD_2BCD_ALL			0x180000		|DFP_ONLY
#define	DFP_BCD_2DPD_ALL			0x600000		|DFP_ONLY
#define	DFP_BCD_DPD_CONV			0x780000		|DFP_ONLY
#define	DFP_INRT_EXRT_BIAS_ALL		0x1800000		|DFP_ONLY
#define	DFP_SHIFT_ALL				0x6000000		|DFP_ONLY
#define DFP_MISC                    0x7F80000       |DFP_ONLY

#define DFP_LOAD_STORE_ALL			0x18000000		|DFP_ONLY
#define	DFP_QUAD_ONLY				0x52DA555		|DFP_ONLY
#define	DFP_LONG_ONLY				0x1BD2FAAA		|DFP_ONLY
#define DFP_SHORT_ONLY				0x8005000		|DFP_ONLY

#define	P9_DFP_TEST_QUAD			(DFP_TEST_QUAD	|P9_ONLY)
#define	P9_DFP_TEST_LONG			(DFP_TEST_LONG	|P9_ONLY)

#define	DFP_ALL						(0x1FFFFFFF		|DFP_ONLY)

#define         VMX_ONLY            0x0400000000000000ULL

#define         VMX_LOAD_ONLY                           (0x1             | VMX_ONLY)
#define         VMX_STORE_ONLY                          (0x2             | VMX_ONLY)
#define         VMX_LOAD_ALIGNMENT_ONLY                 (0x4             | VMX_ONLY)

#define         VMX_INT_ARITHMETIC_SIGNED_ONLY          (0x8             | VMX_ONLY)
#define         VMX_INT_ARITHMETIC_UNSIGNED_ONLY        (0x10            | VMX_ONLY)

#define         VMX_INT_MUL_ADD_SIGNED_ONLY             (0x20            | VMX_ONLY)
#define         VMX_INT_MUL_ADD_UNSIGNED_ONLY           (0x40            | VMX_ONLY)

#define         VMX_INT_SUM_ACROSS_SIGNED_ONLY          (0x80            | VMX_ONLY)
#define         VMX_INT_SUM_ACROSS_UNSIGNED_ONLY        (0x100           | VMX_ONLY)

#define         VMX_INT_AVERAGE_SIGNED_ONLY             (0x200           | VMX_ONLY)
#define         VMX_INT_AVERAGE_UNSIGNED_ONLY           (0x400           | VMX_ONLY)

#define         VMX_INT_MAX_MIN_SIGNED_ONLY             (0x800           | VMX_ONLY)
#define         VMX_INT_MAX_MIN_UNSIGNED_ONLY           (0x1000          | VMX_ONLY)

#define         VMX_INT_CMP_SIGNED_ONLY                 (0x2000          | VMX_ONLY)
#define         VMX_INT_CMP_UNSIGNED_ONLY               (0X4000          | VMX_ONLY)

#define         VMX_INT_LOGICAL_ONLY                    (0x8000          | VMX_ONLY)

#define         VMX_INT_ROTATE_SHIFT_ONLY               (0x10000         | VMX_ONLY)

#define         VMX_FP_ARITHMETIC_ONLY                  (0x20000         | VMX_ONLY)
#define         VMX_FP_MUL_ADD_SUB_ONLY                 (0x40000         | VMX_ONLY)

#define         VMX_FP_MAX_MIN_ONLY                     (0x80000         | VMX_ONLY)
#define         VMX_FP_ROUND_CONV_ONLY                  (0x100000        | VMX_ONLY)
#define         VMX_FP_CMP_ONLY                         (0x200000        | VMX_ONLY)
#define         VMX_FP_ESTIMATE_ONLY                    (0x400000        | VMX_ONLY)

#define         VMX_VSCR_ONLY                           (0x800000        | VMX_ONLY)
#define         VMX_MISC_ONLY                           (0x1000000       | VMX_ONLY)
#define         VMX_DFP_ARITHMETIC                      (0x2000000       | VMX_ONLY)
#define         P9_VMX_DFP_ARITHMETIC                   (VMX_DFP_ARITHMETIC | P9_ONLY)

#define         VMX_LOAD_STORE_ALIGN                    (0x7             | VMX_ONLY)

#define         VMX_INT_ARITHMETIC_SIGNED_ALL           (0xAA8           | VMX_ONLY)
#define         VMX_INT_ARITHMETIC_UNSIGNED_ALL         (0x1550          | VMX_ONLY)
#define         VMX_INT_ARITHMETIC_ALL                  (0x1FF8          | VMX_ONLY)

#define         VMX_INT_CMP_ALL                         (0x6000          | VMX_ONLY)
#define         VMX_INT_SIGNED_ALL                      (0x2AA8          | VMX_ONLY)
#define         VMX_INT_UNSIGNED_ALL                    (0x1D550         | VMX_ONLY)

#define         VMX_FP_ARITHMETIC_ALL                   (0x60000         | VMX_ONLY)

#define         VMX_INT_ALL                             (0x1FFF8         | VMX_ONLY)
#define         VMX_FP_ALL                              (0x7E0000        | VMX_ONLY)
#define         VMX_ALL                                 (0x3FFFFFF       | VMX_ONLY)
#define         P9_VMX_INT_LOGICAL_ONLY                	VMX_INT_LOGICAL_ONLY | P9_ONLY 
#define			P9_VMX_INT_ARITHMETIC_UNSIGNED_ONLY		VMX_INT_ARITHMETIC_UNSIGNED_ONLY | P9_ONLY
#define			P9_VMX_MISC_ONLY						VMX_MISC_ONLY	| P9_ONLY

#define		CPU_FIXED_LOAD				(0x1		|CPU_ONLY)
#define		CPU_COND_LOG				(0x2		|CPU_ONLY)
#define		CPU_FIXED_ARTH				(0x4		|CPU_ONLY)
#define		CPU_FIXED_LOGIC				(0x8		|CPU_ONLY)
#define		CPU_FIXED_ROTATE			(0x10		|CPU_ONLY)
#define		CPU_FIXED_SPR  				(0x20		|CPU_ONLY)
#define		CPU_FIXED_STORE				(0x40		|CPU_ONLY)
#define		CPU_CACHE					(0x80		|CPU_ONLY)
#define		CPU_STORAGE					(0x100		|CPU_ONLY)
#define		CPU_EXTERNAL				(0x200 		|CPU_ONLY)
#define		CPU_BRANCH					(0x400		|CPU_ONLY)
#define		CPU_THREAD_PRI				(0x800		|CPU_ONLY)
#define		CPU_ATOMIC_LOAD				(0x1000		|CPU_ONLY)
#define		CPU_ATOMIC_STORE			(0x2000		|CPU_ONLY)


#define		P9_CPU_ATOMIC_LOAD			(0x1000		|CPU_ONLY 	|P9_ONLY)
#define		P9_CPU_ATOMIC_STORE			(0x2000		|CPU_ONLY 	|P9_ONLY)
#define		P9_CPU_FIXED_ARTH         	(0x4        |CPU_ONLY	|P9_ONLY)
#define     P9_CPU_FIXED_LOGIC			(0x8        |CPU_ONLY  	|P9_ONLY)
#define		P9_CPU_FIXED_LOAD			(0x1		|CPU_ONLY   |P9_ONLY)

#define		MACRO_LHL					0x0001		|MACRO_ONLY
#define		MACRO_SHL					0x0002		|MACRO_ONLY
#define		MACRO_BC8					0x0004		|MACRO_ONLY
#define		MACRO_ALL					0x0007		|MACRO_ONLY

#define SHIFTED_PVR_OS_P8 0x4b
#define SHIFTED_PVR_OS_P9 0x4e  /* Nimbus PVR as shown in fpu tests on BML on Simics */

typedef struct
{
	int ld_eopcode;
	int st_eopcode;
	int ld_index;
	int st_index;
}macro_load_store_class;

/* Major #define declarations */

#define CAST(t,e) ((t)(e))

#ifndef FALSE
#define FALSE           (0)
#endif /* #ifndef FALSE */
#ifndef TRUE
#define TRUE            (1)
#endif /* #ifndef TRUE */

#define NUM_RETRIES		10


/* Function prototypes */
#ifndef __HTX_LINUX__
void SIGRECONFIG_handler (int, int, struct sigcontext *);
void SIGCPUFAIL_handler(int sig);
#endif
uint32 get_random_no_32(int);
uint64 get_random_no_64(int);
uint32 get_random_gpr(int,int,int);
void SIGTERM_hdl(int sig);
void SIGINT_handler(int sig);
void SIGHW_handler(int sig, siginfo_t *si, void *ucontext);
void SIGALRM_hdl(int sig);

void apply_rule_file_settings(void);
void get_random_no_128(int, uint64 *);

void dfp_shrt_z(int, char *, int);
void dfp_shrt_qnan(int, char *, int);
void dfp_shrt_snan(int, char *, int);
void dfp_shrt_inf(int, char *, int);
void dfp_shrt_n(int, char *, int);
void dfp_shrt_subn(int, char *, int);
void dfp_shrt_l(int, char *, int);
void dfp_shrt_s(int, char *, int);
void dfp_shrt_t(int, char *, int);
void dfp_long_z(int, char *, int);
void dfp_long_qnan(int, char *, int);
void dfp_long_snan(int, char *, int);
void dfp_long_inf(int, char *, int);
void dfp_long_n(int, char *, int);
void dfp_long_subn(int, char *, int);
void dfp_long_l(int, char *, int);
void dfp_long_s(int, char *, int);
void dfp_long_t(int, char *, int);
void dfp_quad_z(int, char *, int);
void dfp_quad_qnan(int, char *, int);
void dfp_quad_snan(int, char *, int);
void dfp_quad_inf(int, char *, int);
void dfp_quad_n(int, char *, int);
void dfp_quad_subn(int, char *, int);
void dfp_quad_l(int, char *, int);
void dfp_quad_s(int, char *, int);
void dfp_quad_t(int, char *, int);

void hp_z (int, char *, int);
void hp_n (int, char *, int);
void hp_ntd(int, char *, int);
void hp_qnan(int, char *, int);
void hp_snan(int, char *, int);
void hp_d(int, char *, int);
void hp_dtz(int, char *, int);
void hp_dtn(int, char *, int);
void hp_nti(int, char *, int);
void hp_i(int, char *, int);
void sp_z (int, char *, int);
void sp_n (int, char *, int);
void sp_ntd(int, char *, int);
void sp_qnan(int, char *, int);
void sp_snan(int, char *, int);
void sp_d(int, char *, int);
void sp_dtz(int, char *, int);
void sp_dtn(int, char *, int);
void sp_nti(int, char *, int);
void sp_i(int, char *, int);
void dp_z (int, char *, int);
void dp_n (int, char *, int);
void dp_ntd(int, char *, int);
void dp_qnan(int, char *, int);
void dp_snan(int, char *, int);
void dp_d(int, char *, int);
void dp_dtz(int, char *, int);
void dp_dtn(int, char *, int);
void dp_nti(int, char *, int);
void dp_i(int, char *, int);
void qp_z (int, char *, int);
void qp_n (int, char *, int);
void qp_ntd(int, char *, int);
void qp_qnan(int, char *, int);
void qp_snan(int, char *, int);
void qp_d(int, char *, int);
void qp_dtz(int, char *, int);
void qp_dtn(int, char *, int);
void qp_nti(int, char *, int);
void qp_i(int, char *, int);
void gen_i32(int, char *, int);
void gen_i64(int, char *, int);
void gen_i128(int, char *, int);
void set_seed(int, int32);
void dump_testcase_p7(int, int, int, int);
void dump_testcase_p6(int, int, int, int);
void dump_instructions_p7(int, FILE *);
void dump_instructions_p6(int, FILE *);
void dumpsiginfo(int);
int create_context(int);
int copy_prolog(int);
int copy_prolog_p6(int);
int copy_prolog_p7(int);
int copy_epilog_p6(int);
int copy_epilog_p7(int);
int is_reg_sp(uint32,int);
int copy_epilog(int);
int build_testcase(uint32);
int impart_context_in_memory(int);
int execute_testcase(int, int);
int compare_results(int, int *);
int allocate_mem(struct shm_buf *);
int cleanup_mem(int);
void cleanup_mem_atexit(void);
int read_rf(void);
void set_rule_defaults(void);
int get_rule(int *, FILE *, struct ruleinfo *);
int get_line( char *, int, FILE *, char, int *);
int parse_line(char *);
int calculate_seg_size(void);
int simulate_testcase(int);
int populate_buf(int, int);
void initialize_client_mem(void);
int initialize_client(int client_num);
int initialize_gprs_n_sprs(int);
void gen_data_pat(int, char *, int);
uint32 init_mem_for_vsx_store(int, int);
uint32 init_mem_for_vsx_load(int, int);
int allocate_vsr_mem(int);
int free_vsr_mem(int);
void shuffle (int, uint16 *, uint16);
void apply_rule_file_settings(void);
int create_ins_cat_wise_tables(int cno);
void set_ins_bias_array(int cno);
int filter_masked_instruction_categories(int cno, uint64 mask, struct instruction_masks *table);
void merge_instruction_tables(void);
void init_random_no_generator(uint32);
void client_func(void *cno);
void htx_sync(volatile uint32 *, uint64, int, int, int);
void htx_sync_new(uint32 *, uint64, uint32 *, int, int);
int copy_prolog_p6(int );
int is_reg_sp(uint32, int);
int get_logical_to_physical(int cpu);
int bind_thread(uint32 cpu);
int unbind_thread(uint32 cpu);
int distribute_vsrs_based_on_ins_bias(int cno);
void generate_and_set_initial_seeds(int32);

void class_vsx_load_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_vsx_store_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_vsx_test_ins_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_vsx_normal_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_vsx_mul_add_sub_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int);
void class_vsx_move_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_vsx_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index);
void class_vsx_load_gen2(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index);
void class_vsx_store_gen2(uint32 client_no, uint32 random_no, struct instruction_masks *temp, int index);

void class_bfp_load_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_bfp_load_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins,int);
void class_bfp_store_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins,int);
void class_bfp_store_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins,int);
void class_bfp_normal_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins,int);
void class_bfp_fpscr_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins,int);
void class_bfp_fpscr_imm_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins,int);
void class_bfp_cr_update_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins,int);
void class_bfp_fpscr_2_cr_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins,int);
void class_bfp_fpscr_bit_set_unset(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_bfp_qp_round_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_bfp_qp_test_data_class_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);

void class_dfp_load_imm_gen(uint32, uint32, struct instruction_masks *, int);
void class_dfp_load_gen(uint32, uint32, struct instruction_masks *, int);
void class_dfp_store_imm_gen(uint32, uint32, struct instruction_masks *, int);
void class_dfp_store_gen(uint32, uint32, struct instruction_masks *, int);
void class_dfp_normal_gen(uint32, uint32, struct instruction_masks *, int);
void class_dfp_cmp_test_ins_gen(uint32, uint32, struct instruction_masks *, int);
void class_dfp_qua_rmc_gen(uint32, uint32, struct instruction_masks *, int);
void class_dfp_shift_gen(uint32, uint32, struct instruction_masks *, int);

void class_vmx_load_gen(uint32, uint32, struct instruction_masks *, int);
void class_vmx_store_gen(uint32, uint32, struct instruction_masks *, int);
void class_vmx_normal_gen(uint32, uint32, struct instruction_masks *, int);
void class_cpu_load_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_load_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_cond_log_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_arth_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_logic_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_logic_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_arth_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_rotate_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_rotate_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_rotate_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_rotate_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_arth_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_arth_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_spr_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_spr_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_spr_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_store_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_store_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_arth_4_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_store_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_all_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_all_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_logic_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_cache_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_cache_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_cache_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_branch_1_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_branch_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_branch_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_branch_4_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_fixed_load_2_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_cache_3_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_thread_pri_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_lhl_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_shl_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int);
void class_cpu_load_atomic_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index);
void class_cpu_store_atomic_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index);
void class_cpu_load_relative_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index);
void class_cpu_string_operations_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index);
void class_cpu_mul_add_gen(uint32 client_no, uint32 random_no, struct instruction_masks *instr, int index);
void class_cpu_array_index_support_gen(uint32 client_no, uint32 random_no, struct instruction_masks *ins, int index);
uint32 getdistance(uint32 client_no);
uint32 takebackward_branch(uint32 client_no);
unsigned long getPvr(void);
int get_PVR(void);
void adjust_vsrs_memory_image(int cno, int pass);
uint32 func_store_64_p7(uint32 reg);
uint32 func_store_64_p6(uint32 reg);
uint32 func_store_128_p7(uint32 reg);
uint32 init_mem_for_gpr(int, int);
uint32 cpu_build_module(int,int,int,int,int);

void initialize_sync_words(int pass);
int build_sync_point(uint32 client_no);
int get_mem_and_offset(uint32 client_no, int memsize, char* ptr, char **updated_ptr);

int reinit_ls_base(uint32 client_no);

#ifdef SCTU
/*List of core names that sctu clients need to bind to*/
int get_bind_core_and_cpus_list(void);
void initialize_sctu_locks(void);
int init_sctu_cpu_array(void);
int copy_common_data(int pass, int cno);
void dump_testcase_sctu(int client_num, int num_oper, int rc, int miscomparing_num);
int clear_sctu_th(void);
int get_bind_core_and_cpus_list();
int chip_testcase(void);
int node_testcase(void);
#else
int get_bind_cpus_list(void);
#endif

#endif

