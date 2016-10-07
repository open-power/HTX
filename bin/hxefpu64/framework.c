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

#include <errno.h>
#include <string.h>
#include "framework.h"
#include "miscomp_debug.h"
#include <ucontext.h>
#include <semaphore.h>
/* syscfg prototype */
extern void get_env_details(htxsyscfg_env_details_t* e);
/* misc library call */
#ifdef __HTX_LINUX__
extern int get_cpu_version(void);
extern int htx_bind_thread(int, int);
#else
/* AIX */
/* kernel extension: etc/kernext/hxesct2/sctKernExt.c */
extern unsigned long getTruePvr(void);
/* VM policy to allocate memory local to core */
#include <sys/rset.h>
#include <sys/vminfo.h> 
#endif

void attn (unsigned int a,
uint64 b,
uint64 c,
uint64 d,
uint64 e,
uint64 f,
uint64 g,
uint64 h) {
    __asm__ volatile (".long 0x00000200":);
}


#ifdef AWAN
FILE *fp_sim;
char fname_sim[100];
unsigned long long insts_sim, insts_sim_thread;
struct signal_info nontcsigdata;
#endif

int shm_flag_for_malloc = 0;

struct shm_buf vsx_mem[3];
struct server_data global_sdata[3];
struct instruction_masks master_instructions_array[MAX_INSTRUCTIONS];

int num_cpus; 		/* Number of cpus online at start. */
int dev_num, num_cpus_to_test;
int update_bcpu_info = 0;	/* Used for cpu recalculation for hotplug. */

htxsyscfg_env_details_t proc_env;

db_fptr fp_value_gen_fptr_array[VSR_OP_TYPES][BFP_DATA_BIAS_TYPES];
extern uint32 vsr_reg_wt[], bfp_reg_wt[];
struct htx_data hd;
struct ruleinfo rule;
char msg[1000];
volatile int exit_flag;          /* flag to indicate if SIGTERM received */
volatile int sighandler_exit_flag = 0; /* flag to terminate signal handler */
pthread_mutex_t exit_flag_lock;  
int equaliser_flag;     /* flag to show if running under equaliser */
struct ruleinfo rule_list[MAX_NUM_RULES]; /* link list of rules. Filled in by read_rf. */
uint32 num_active_rules; /* should be populated by read_rf function */
uint32 pvr, shifted_pvr_os, shifted_pvr_hw; /* global var: populated by get_PVR routine */
struct sigaction sigvector;
htxsyscfg_smt_t smt_detail;
struct client_data *client_mem = NULL; /* global area to keep all clients related info */
struct dev_info {
    char device_name[50];
    char run_type[4];
    char rule_file_name[200];
} dinfo;
FILE *hlog = NULL;

/*
 * core_num is a number encoded in device name. For e.g. if device name is /dev/fpu5, core_num = 5.
 * Thread specific log files will be created in the format of vsx_log.<core_num>.<thread_num>.
 */
uint32 core_num;
pthread_mutex_t clients_sync_lock;
pthread_cond_t clients_sync_cond;
volatile int clients_sync_count = 0;

/* signal handler thread related declarations */
sigset_t myset;
pthread_t sighdlr_th_tid;
pthread_attr_t attr;

#ifdef SCTU

#define SCTU_GANG_SIZE (8)
#ifdef __HTX_LINUX__
	#define SCTU_GANG_SIZE_PERF (2)
#else
	#define SCTU_GANG_SIZE_PERF SCTU_GANG_SIZE
#endif
int sctu_exit_flag;          /* Flag to indicate if SIGTERM received */
int orig_priority;
int cores_in_gang[SCTU_GANG_SIZE];
int cpus_in_core_gang[SCTU_GANG_SIZE][MAX_THREADS_PER_CORE];
int bcpus_in_gang[SCTU_GANG_SIZE];
int cores_in_node[MAX_NODES][MAX_CORES_PER_NODE];
extern GLOBAL_SYSCFG *global_ptr;

SYS_STAT Sys_stat;

pthread_mutex_t testcase_built_lock;
pthread_mutex_t pass1_exec_lock;
pthread_mutex_t pass2_ready_count_lock;
pthread_mutex_t rerunpp_ready_count_lock;
pthread_mutex_t compare_ls_lock;

sem_t pass1_exec_sem;
sem_t pass2_exec_sem;
sem_t rerun_pp_sem;

volatile uint32 threads_built_testcase;
volatile uint32 threads_ready_for_pass2;
volatile uint32 threads_ready_for_rerunpp;
volatile uint32 compare_ls_threads;
volatile uint32 miscompare_in_numoper;

#define MIN_CPUS_TO_TEST		2

#else

int cpus_in_core[MAX_NUM_CPUS];

#define MIN_CPUS_TO_TEST		1

#endif


/***************************************************/
/* support timer based testcase execution with seed logging */
FILE *fp_tblog;                     /* file handle for logging seed                 */
volatile int alarm_expired = 0;     /* flag to indicate SIGALRM received    	 	*/
unsigned int thread_count = 0; 		/* count goes from 1 to rules.num_threads 		*/
pthread_cond_t thread_count_cond;   /* serialize seed logging by threads into file 	*/
pthread_mutex_t thread_count_lock; 	/* atomic update to log count by threads  		*/
pthread_mutex_t seed_log_lock; 		/* thread level lock to update seed file 		*/
unsigned int rule_run_time = 0;     /* alarm time per num operation              	*/
uint64 cycle_count         = 0;		/* cycle count after finishing all stanza       */
/***************************************************/
/*
 * copy_prolog_fptr & copy_epilog_fptr are function ptrs that are populated by get_PVR function
 * based on PVR value. For now different functions exist for P6 and P7.
 */
copy_fptr copy_prolog_fptr, copy_epilog_fptr;

/*
 * Total no of instructions count as calculated by merge_instructions_table
 */
unsigned short total_num_instructions;
/*
 * Parent seed derived by reading timebase reg. This in turn generated 4 seeds and assign them to
 * threads as seed.
 */
int32 original_seed;
extern struct instruction_masks vsx_instructions_array[];
extern struct instruction_masks bfp_instructions_array[];
extern struct instruction_masks bfp_p7_instructions_array[];
extern struct instruction_masks dfp_instructions_array[];
extern struct instruction_masks vmx_instructions_array[];
extern struct instruction_masks cpu_instructions_array[];
extern struct instruction_masks cpu_p7_instructions_array[];
extern struct instruction_masks cpu_p8_instructions_array[];
extern struct instruction_masks bfp_p8_instructions_array[];
extern struct instruction_masks vmx_p8_instructions_array[];
extern struct instruction_masks vsx_p8_instructions_array[];
extern struct instruction_masks cpu_macro_array[];
extern struct instruction_masks vmx_p8_dd2_instructions_array[];
extern struct instruction_masks cpu_p9_instructions_array[];
extern struct instruction_masks vmx_p9_instructions_array[];
extern struct instruction_masks vsx_p9_instructions_array[];
extern struct instruction_masks bfp_p9_instructions_array[];
extern struct instruction_masks dfp_p9_instructions_array[];

db_fptr fp_value_gen_fptr_array[VSR_OP_TYPES][BFP_DATA_BIAS_TYPES] = {
	{&sp_z,       &sp_z,          &sp_z,          &sp_z,       &sp_z,         &sp_z,          &sp_z,       &sp_z,          &sp_z,       &sp_z},
	{&sp_n,       &sp_d,          &sp_qnan,       &sp_nti,     &sp_i,         &sp_snan,       &sp_z,       &sp_dtz,        &sp_dtn,     &sp_ntd},
	{&dp_n,       &dp_d,          &dp_qnan,       &dp_nti,     &dp_i,         &dp_snan,       &dp_z,       &dp_dtz,        &dp_dtn,     &dp_ntd},
	{&sp_n,       &sp_d,          &sp_qnan,       &sp_nti,     &sp_i,         &sp_snan,       &sp_z,       &sp_dtz,        &sp_dtn,     &sp_ntd},
	{&dp_n,       &dp_d,          &dp_qnan,       &dp_nti,     &dp_i,         &dp_snan,       &dp_z,       &dp_dtz,        &dp_dtn,     &dp_ntd},
	{&gen_i128,   &gen_i128,      &gen_i128,      &gen_i128,   &gen_i128,     &gen_i128,      &gen_i128,   &gen_i128,      &gen_i128,   &gen_i128},
	{&hp_n,       &hp_d,          &hp_qnan,       &hp_nti,     &hp_i,         &hp_snan,       &hp_z,       &hp_dtz,        &hp_dtn,     &hp_ntd},
	{&hp_n,       &hp_d,          &hp_qnan,       &hp_nti,     &hp_i,         &hp_snan,       &hp_z,       &hp_dtz,        &hp_dtn,     &hp_ntd},
	{&dfp_shrt_n, &dfp_shrt_subn, &dfp_shrt_qnan, &dfp_shrt_l, &dfp_shrt_inf, &dfp_shrt_snan, &dfp_shrt_z, &dfp_shrt_subn, &dfp_shrt_s, &dfp_shrt_t},
	{&dfp_long_n, &dfp_long_subn, &dfp_long_qnan, &dfp_long_l, &dfp_long_inf, &dfp_long_snan, &dfp_long_z, &dfp_long_subn, &dfp_long_s, &dfp_long_t},
	{&dfp_quad_n, &dfp_quad_subn, &dfp_quad_qnan, &dfp_quad_l, &dfp_quad_inf, &dfp_quad_snan, &dfp_quad_z, &dfp_quad_subn, &dfp_quad_s, &dfp_quad_t},
	{NULL,        NULL,           NULL,           NULL,        NULL,          NULL,           NULL,        NULL,           NULL,        NULL},
	{NULL,        NULL,           NULL,           NULL,        NULL,          NULL,           NULL,        NULL,           NULL,        NULL},
	{NULL,        NULL,           NULL,           NULL,        NULL,          NULL,           NULL,        NULL,           NULL,        NULL},
	{&qp_n,       &qp_d,          &qp_qnan,       &qp_nti,     &qp_i,         &qp_snan,       &qp_z,       &qp_dtz,        &qp_dtn,     &qp_ntd}
};


store_macro_fptr st_fptrs[VSR_OP_TYPES];
uint32 sizes_for_types[VSR_OP_TYPES] = {0, 8, 8, 16, 16, 16, 8, 8, 16, 16, 16, 16};
#ifndef __HTX_LINUX__
	uint64 read_tb(void);
	#pragma mc_func read_tb {"7c6c42a6"}
	#pragma reg_killed_by read_tb gr3

	#ifndef COMPARE_METHOD_CRC
		uint64 add_logical(uint64, uint64);
		#pragma mc_func add_logical {"7c632014" "7c630194"}
		/*   addc       r3 <- r3, r4           */
		/*   addze      r3 <- r3, carry bit    */
		#pragma reg_killed_by add_logical gr3, xer
		/* only gpr3 and the xer are altered by this function */
	#endif
#else
	#define read_tb(var) asm("mfspr %0, 268":"=r"(var));
	uint64 exclude_cat_mask = 0;
#endif


int
main(int argc, char *argv[])
{
	int rc, rc1, i, j;
	uint32 prev_sao_val = 0;
	char tb_fname[128];

#ifdef __HTX_LINUX__
	/* Register common signal handler for hardware generated signals */
	sigvector.sa_flags = SA_SIGINFO | SA_RESETHAND;
#ifdef AWAN
	sigvector.sa_flags = SA_SIGINFO;
#endif
	sigvector.sa_sigaction = SIGHW_handler;
	sigaction(SIGSEGV, &sigvector, (struct sigaction *) NULL);
	sigaction(SIGILL, &sigvector, (struct sigaction *) NULL);
	sigaction(SIGBUS, &sigvector, (struct sigaction *) NULL);
#endif

	/* Below signal set i.e myset will be used after hxfupdate(START) */
	/* so that failures can be reported using hxfmsg as it can't be called before hxfupdate(START) */
	sigemptyset(&myset);
	sigaddset(&myset, SIGALRM);
	sigaddset(&myset, SIGTERM);
	sigaddset(&myset, SIGINT);
#ifndef __HTX_LINUX__
	sigaddset(&myset, SIGRECONFIG);
	sigaddset(&myset, SIGCPUFAIL);
#else
	sigaddset(&myset, SIGUSR2);
#endif

	strcpy(dinfo.run_type, "OTH");

#ifdef SCTU
	if ((argc == 2) && (strcasecmp(argv[1], "SCTU_DEV") == 0)) {
		int i;

		strcpy(dinfo.device_name, argv[1]);
		init_syscfg_with_malloc();
		shm_flag_for_malloc = 1;
		rc = chip_testcase();
		rc1 = node_testcase();
		shm_flag_for_malloc = 0;

		return(rc | rc1);
	}
#endif
		
	if ( argc < 4 ) {
		/* standalone invocation without all arguments. Put proper warning and return. */
		printf("###########################################################################\n");
		printf("#                                                                         #\n");
		printf("#        WARNING : Exerciser invoked without proper arguments             #\n");
		printf("#                                                                         #\n");
		printf("#          Please invoke exerciser with proper arguments in               #\n");
		printf("#                      following format :                                 #\n");
		printf("#              <exerciser> <device> <mode> <rulefile>                     #\n");
		printf("#                                                                         #\n");
		printf("# eg : hxefpu64 /dev/fpu0 OTH /usr/lpp/htx/rules/reg/hxefpu64/default.p8  #\n");
		printf("#                                                                         #\n");
		printf("###########################################################################\n");

		return(-1);
	}
	/*  Parse command line arguments */
	strcpy(dinfo.device_name, argv[1]);
	strcpy(dinfo.run_type, argv[2]);
	strcpy(dinfo.rule_file_name, argv[3]);

	/* Set htx_data structure parameters */
	strcpy(hd.HE_name, argv[0]);
	strcpy(hd.sdev_id, argv[1]);
	strcpy(hd.run_type, argv[2]);
	atexit(cleanup_mem_atexit);

#ifdef __HTX_LINUX__
	hd.hotplug_cpu = 1; 	/* Register with htx supervisor for hot plug event intimation. */
#endif

	/* Extract device id. */
	if (argv[1]) {
		char *ptr = argv[1];
		i = 0;
		dev_num = 0;
		while (i < strlen(argv[1])) {
			if (*(ptr + i) >= '0' && *(ptr + i) <= '9') {
				dev_num = dev_num*10 + *(ptr + i) - '0';
			}
			i++;
		}
	}

	core_num = dev_num;		/* For fpu/cpu exerciser dev_num will indicate core number. */
	hxfupdate(START, &hd);
   	sprintf(msg, "Exerciser started.\n%s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);
#ifndef AWAN
	hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#else
	/* For AWAN */
	printf("%s", msg);

	system("mkdir -p /tmp/stats");
	sprintf(fname_sim, "/tmp/stats/%s_insts",  (char *)(argv[1]+5));
	printf("AWAN stats file is %s\n",fname_sim);
	fflush(stdout);

	errno=0;
	fp_sim=fopen(fname_sim, "w");
	if (fp_sim == NULL) {
		printf("fopen: %s\n",strerror(errno));
		printf("Failed to open %s file. errno = %d\n", fname_sim, errno);
		exit(0);
	}

	/* Write 0 in awan stats file upon start. */
	insts_sim=0;
	fprintf(fp_sim, "%llu\n", insts_sim);
	fflush(fp_sim);
#endif

#ifdef __HTX_LINUX__
	num_cpus = get_nprocs();	/* Record number of cpus online at start. */
	/* check Crypto disabled */
	/* Bit 32 = 1 = tp_nx_allow_crypto_dc_int (NX Crypto Allowed) 
	 * Bit 33 = 0 = tp_ex_fuse_vmx_crypto_dis_dc_int (VMX Crypto NOT Disabled) or ie VMX Crypto Enabled
	 */
	#define SCOM_REG_ADDR               0x00000000000F000FULL
	#define NX_CRYPTO_SCOM_BIT          0x0000000100000000ULL /* NX Crypto Enabled */
	#define VMX_CRYPTO_SCOM_BIT         0x0000000200000000ULL /* VMX Crypto Disabled */

	uint64 scom_val = 0, scom_addr = SCOM_REG_ADDR;
	uint32 chip_id = 0xffffffff;

	chip_id = xscom_init();
	if (chip_id == 0xffffffff) {
		fprintf(stderr, "No valid XSCOM chip found\n");
		sprintf(msg, "xscom_init failed, No valid XSCOM chip found\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}
	else {
		rc = xscom_read(chip_id, scom_addr, (uint64_t *)&scom_val);
		if (rc) {
			sprintf(msg, "xscom_read failed, Error reading XSCOM, RC: %d\n", rc);
			hxfmsg(&hd, 0, HTX_HE_INFO, msg);
		}
		if (!(scom_val & VMX_CRYPTO_SCOM_BIT)) {
			exclude_cat_mask = 0;
			sprintf(msg, "Crypto enabled: chip id: %X, SCOM Addr: 0x%016llx, Value: %llx\n", chip_id, scom_addr, scom_val);
		}
		else {
			/* exclude nstruction category for crypto */
			exclude_cat_mask = VMX_MISC_ONLY;
			sprintf(msg, "Crypto disabled: chip id: %X, SCOM Addr: 0x%016llx, Value: %llx, exclude category: 0x%016llX\n", chip_id, scom_addr, scom_val, exclude_cat_mask);
		}
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}

#else
	num_cpus = _system_configuration.ncpus;
#endif
    rc = repopulate_syscfg(&hd);
    if ( rc ) {
        sprintf(msg,"repopulate_syscfg failed with error code= %d \n",rc);
        hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
        exit_flag = 1;
        return -1;
    }


	get_env_details(&proc_env); 	/* Tracking shared processor mode */

	pthread_mutex_init(&thread_count_lock, NULL);
	pthread_mutex_init(&seed_log_lock, NULL);
	pthread_mutex_init(&exit_flag_lock, NULL);
	pthread_mutex_init(&clients_sync_lock, NULL);
	pthread_cond_init(&thread_count_cond, NULL);
	pthread_cond_init(&clients_sync_cond, NULL);

#ifndef SCTU
	#if defined(DEBUG) && !defined(AWAN)
    hlog = fopen(LOGFILE, "w");
    if (hlog == NULL) {
		sprintf(msg, "Error opening fpu_log file: %s, errno: 0x%X, %s\n", LOGFILE, errno, strerror(errno));
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		exit(1);
	}
	DPRINT(hlog, "core_num = %d\n", core_num);
	#endif

	/* open seed file in append mode */
	sprintf(tb_fname, "/tmp/%s_seedlist.txt", argv[1] + 5);
	fp_tblog = fopen(tb_fname, "a");
    if (fp_tblog == NULL) {
		sprintf(msg, "Error opening file: %s!!!, 0x%X, %s\n", tb_fname, errno, strerror(errno));
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		exit(1);
	}
#else
	sprintf(msg, "%s_%d", LOGFILE, core_num);
    hlog = fopen(msg, "w");
    if (hlog == NULL) {
		sprintf(msg, "Error opening fpu_log file \n");
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		exit(1);
	}
	DPRINT(hlog, "core_num = %d\n", core_num);
#endif

	rc = pthread_sigmask(SIG_BLOCK, &myset, NULL);
	if (rc) {
		sprintf(msg, "pthread_sigmask failed...exiting \n");
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		exit(-1);
	}

	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	/* Create signal handler thread */
	void * sighandler_thread(void *arg);
	if(pthread_create(&sighdlr_th_tid, &attr, sighandler_thread, (void *)&core_num)){
		sprintf(msg, "pthread_create failed for signal handling thread...exiting \n");
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		exit(-1);
	}


/* we want no effect of equaliser_flag on sctu */
#ifndef SCTU
	/* Get equaliser_flag */
	{
		char *env_var;

		env_var = getenv("EQUALISER_FLAG");
		if ( env_var != NULL ) {
			equaliser_flag = atoi(env_var);
		}
		else {
			equaliser_flag = 0;
		}
	}
#endif

#ifdef __HTX_LINUX__
	/*
 	 * For Linux, prctl will set MSR(FE0, FE1) to (0,0).
 	 */
	#if defined(PR_FP_EXC_DISABLED)
	rc = prctl(PR_SET_FPEXC,PR_FP_EXC_DISABLED,0,0,0);
    if (rc != 0) {
		sprintf(msg, "prctl failed with %d. Exiting...", errno);
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		exit(1);
	}
	#endif
#endif
	/*
	 * get_PVR determines PVR value and populates arch specific function pointers.
	 */
	if (get_PVR()) {
		sprintf(msg, "get_PVR failed ! Unsupported arch ! Exiting... \n");
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		cleanup_mem(1);
		exit(1);
	}
	/*
	 * Read rule file parameters.
	 */
	rc = read_rf();
	if (rc) {
		sprintf(msg, "Error reading rule file. Exiting ! \n");
		hxfmsg(&hd, rc, HTX_HE_HARD_ERROR, msg);
		exit(1);
	}
	
	/* populate cpu lists array */
#ifdef SCTU
    /* initialize sctu mutexes and semaphores */
    initialize_sctu_locks();
	/* Get list of core and cpu numbers in gang. */
    get_bind_core_and_cpus_list();
#else
	/* Get list of cpus in core. */
    get_bind_cpus_list();
#endif

	/*
	 * Allocate local memory buffer for testcase and related data structures.
	 * allocate_mem should be called for Initial, pass1 and pass2 buffers.
	 */
#ifndef __HTX_LINUX__
	/* AIX Memory affinity policy */
    int early_lru = 1; /* 1 extends local affinity via paging space.*/
    int num_policies = VM_NUM_POLICIES;
    int policies[VM_NUM_POLICIES] = {/* -1 = don't change policy */
    	P_FIRST_TOUCH, /* VM_POLICY_TEXT */
    	P_FIRST_TOUCH, /* VM_POLICY_STACK */
    	P_FIRST_TOUCH, /* VM_POLICY_DATA */
    	P_FIRST_TOUCH, /* VM_POLICY_SHM_NAMED */
    	P_FIRST_TOUCH, /* VM_POLICY_SHM_ANON */
    	-1, /* VM_POLICY_MAPPED_FILE */
    	-1, /* VM_POLICY_UNMAPPED_FILE */
    };

    /* To get local memory set flag early_lru = 1 to select P_FIRST_TOUCH policy
	 * (similar to setting MEMORY_AFFINITY environment variable to MCM)
	 */
    rc = vm_mem_policy(VM_SET_POLICY, &early_lru, policies, num_policies);
    if (rc != 0){
        sprintf(msg,"vm_mem_policy() call failed with return value = %d\n", rc);
        hxfmsg(&hd, rc, HTX_HE_SOFT_ERROR, msg);
    }
#ifdef SCTU
	bind_thread(bcpus_in_gang[0]);
#else
    bind_thread(cpus_in_core[0]);
#endif
#else
	/* Linux: default memory allocation policy is native only */
	/*htx_bind_thread();*/
#endif
	
	vsx_mem[INITIAL_BUF].key 	= dinfo.device_name[5] << 24 | dinfo.device_name[6] << 16 | (core_num & 0xfff);
	vsx_mem[PASS1_BUF].key 		= dinfo.device_name[5] << 24 | dinfo.device_name[6] << 16 | 0x1000 | (core_num & 0xfff);
	vsx_mem[PASS2_BUF].key 		= dinfo.device_name[5] << 24 | dinfo.device_name[6] << 16 | 0x2000 | (core_num & 0xfff);

	for (i = 0; i < 3; i++) {
		vsx_mem[i].seg_size = calculate_seg_size();
		vsx_mem[i].page_size = 4096;
		rc = allocate_mem(&vsx_mem[i]);
		if(rc) {
			cleanup_mem(0);
			sprintf(msg, "Error creating shared memory. cleanup and exit ! \n");
			hxfmsg(&hd, rc, HTX_HE_HARD_ERROR, msg);
			exit(1);
		}
	}
#ifndef AWAN
	hxfmsg(&hd, 0, HTX_HE_INFO, "shared memory create success\n");
#endif

	/*
	 * Allocate memory for client_data struct and set ptrs in global_sdata[] for each of the
	 * cdata_ptr array member.
	 */
	client_mem = (struct client_data *)malloc(MAX_NUM_CPUS * sizeof(struct client_data));
	if (client_mem == NULL) {
		sprintf(msg, "Malloc error for client_mem. errno = %d\n", errno);
		hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
		cleanup_mem(0);
		exit(1);
	}
	bzero(client_mem, (MAX_NUM_CPUS * sizeof(struct client_data)));
	DPRINT(hlog,"fpu%d: Memory allocated for client_mem and initilized with zero\n", core_num);
	FFLUSH(hlog);

	/* Updating Client pointer in global server data structure */
	for (i = 0; i < 3; i++) {
		for(j = 0; j < MAX_NUM_CPUS; j++) {
			global_sdata[i].cdata_ptr[j] = (struct client_data *)((uint64)client_mem + j*sizeof(struct client_data));
			DPRINT(hlog,  "cdata_ptr[%d] = 0x%llx \n",j, (uint64)global_sdata[i].cdata_ptr[j]);
		}
	}

	/*
	 * Initialize shared memory pointers inside struct server_data.
	 */
	for (i = 0; i < 3; i++) {
		global_sdata[i].shm_buf_ptr = &vsx_mem[i];
	}
	DPRINT(hlog, "Number of clients = 0x%x\n", global_sdata[INITIAL_BUF].num_clients);
	/*
	 * Initialize sdata->cdata_ptr[] array and sdata->cdata_ptr[]->tc_ptr[]. This is basically a
	 * mapping between client_data structures and shared memory area.
	 */
	initialize_client_mem();
	/*
	 * allocate_vsr_mem should be called for INITIAL_BUF only.
	 */
	for (i = 0; i < MAX_NUM_CPUS; i++) {
		int rc;

		rc = allocate_vsr_mem(i);
		if(rc) {
			sprintf(msg, "allocate_vsr_mem failed with = %d. Exiting...\n", errno);
			hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
			cleanup_mem(0);
			exit(1);
		}
#if defined(DEBUG) && !defined(AWAN)
		/*
		 * open debug file pointers for client specific debug file.
		 */
		sprintf(global_sdata[INITIAL_BUF].cdata_ptr[i]->logfile, "/tmp/fpu_debug.%d.%d", core_num, i);
		global_sdata[INITIAL_BUF].cdata_ptr[i]->clog = fopen((const char *)global_sdata[INITIAL_BUF].cdata_ptr[i]->logfile, "w");
		if(global_sdata[INITIAL_BUF].cdata_ptr[i]->clog == NULL) {
			sprintf(msg, "Error opening log file for client #%d. Exiting... \n", i);
			hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
			cleanup_mem(0);
			exit(1);
		}
		else {
			DPRINT(hlog, "cno: %d logfile: %s\n", i, global_sdata[INITIAL_BUF].cdata_ptr[i]->logfile);
		}
#endif

#if 0
#if !defined(DEBUG)
		/*
		 * open null file pointers(/dev/null) for client specific debug file.
		 */
		sprintf(global_sdata[INITIAL_BUF].cdata_ptr[i]->logfile,"/dev/null");
		global_sdata[INITIAL_BUF].cdata_ptr[i]->clog = fopen((const char *)global_sdata[INITIAL_BUF].cdata_ptr[i]->logfile, "w");
		if(global_sdata[INITIAL_BUF].cdata_ptr[i]->clog == NULL) {
			sprintf(msg, "Error opening null log file for client #%d. Exiting... \n", i);
			hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
			cleanup_mem(0);
			exit(1);
		}
		else {
			DPRINT(hlog, "cno: %d logfile: %s log: 0x%llx\n", i, global_sdata[INITIAL_BUF].cdata_ptr[i]->logfile, global_sdata[INITIAL_BUF].cdata_ptr[i]->clog);
		}
#endif
#endif
	}

#ifdef __HTX_LINUX__
	/* Initialize physical bcpu fields for clients. */
    for (i=0; i < MAX_NUM_CPUS; i++) {
       	global_sdata[INITIAL_BUF].cdata_ptr[i]->phy_bcpu = -1;
	}
#endif

	if (exit_flag) {
		cleanup_mem(0);
		exit(-1);
	}

	/* Check if min number of cpus are online for test to run. */
	if (num_cpus_to_test < MIN_CPUS_TO_TEST) {
		sprintf(msg, "Insufficient number of cpus to run %s. Num_cpus_to_test = %d. Exiting!!\n", hd.sdev_id, num_cpus_to_test);
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		cleanup_mem(0);
		exit(0);
	}


	/*
	 * Collate various instruction tables like VSX table, BFP table into master_instructions_array.
	 */
	merge_instruction_tables();

	/*
	 * Forever loop. This loop runs till exer receives SIGTERM.
	 */
	do {
		uint32 rule_no;
		struct client_data *cptr = NULL;
		struct stanza_instr_stats stanza_stats[MAX_NUM_RULES];
		bzero(stanza_stats, sizeof(struct stanza_instr_stats) * MAX_NUM_RULES);

		for(rule_no = 0; rule_no < num_active_rules; rule_no++) {
			/* Update current stanza at the beginning only - Defect #815072 */
			hd.test_id = rule_no + 1;
			hxfupdate(UPDATE, &hd);
			rule_run_time = 0;
			uint64 total_num_oper = 0;	
			uint64 avg_num_oper = 0;	
			uint64 total_num_pass2 = 0;	
			uint64 avg_num_pass2 = 0;	

			/*
			 * All work done in this loop is repeated for every rule in rule file.
			 */

			memcpy(&rule, (const char *)&rule_list[rule_no], sizeof(struct ruleinfo));


			/* Recalculate cpus to test info if there was a config changes due to hotplug/DR. */
			if (update_bcpu_info == 1) {
				#ifdef __HTX_LINUX__
				/* Initialize physical bcpu fields for clients. */
    				for (i=0; i < MAX_NUM_CPUS; i++) {
       					global_sdata[INITIAL_BUF].cdata_ptr[i]->phy_bcpu = -1;
				}
				#endif

				sprintf(msg, "Recalculating bind cpus list.\n");
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);

			#ifdef SCTU
				get_bind_core_and_cpus_list();
			#else
				get_bind_cpus_list();
			#endif

				/* Check if min number of cpus are online for test to run. */
				if (num_cpus_to_test < MIN_CPUS_TO_TEST) {
					sprintf(msg, "Terminating device instance due to Hot Plug/DR.\n");
                    hxfmsg(&hd, -1, HTX_HE_INFO, msg);
					exit_flag = 2;
				}
				update_bcpu_info = 0;		/* Reset update_bcpu flag. */
			}

			if(exit_flag) {
				cleanup_mem(0);
				if(exit_flag == 2) {
					hxfupdate(RECONFIG, &hd);
				}
				exit(0);
			}


		#ifdef SCTU
			rule.num_threads = num_cpus_to_test;
		#else
			if ( rule.num_threads <= 0 || rule.num_threads > num_cpus_to_test) {
				rule.num_threads = num_cpus_to_test;
			}
		#endif

		#ifdef AWAN
			printf("\nStanza = %s\n", rule.rule_id);
    		printf("Pass1 = %s\n", (rule.test_method == CORRECTNESS_CHECK) ? "SIM" : "HW");
       		printf("Pass2 = HW\n");
			printf("num_oper = %d\n", rule.num_oper);
			printf("stream_depth = %d\n", rule.stream_depth);
			printf("test_method = %d\n", rule.test_method);
			printf("num_threads = %d\n", rule.num_threads);
			fflush(stdout);
		#endif

			/* This check is for equaliser. Run single threaded under equaliser. */
			if (equaliser_flag) {
				rule.num_threads = 1;
			}

			/*
			DPRINT(hlog, "num_oper = %d\n", rule.num_oper);
			DPRINT(hlog, "stream_depth = %d\n", rule.stream_depth);
			DPRINT(hlog, "test_method = %d\n", rule.test_method);
			DPRINT(hlog, "num_threads = %d\n", rule.num_threads);
			*/



			/*
			 * Read rule file parameters and update client_data structures.
			 */
			apply_rule_file_settings();

			if (rule.testcase_sync) {
				struct common_data *cdptr = NULL;
				cdptr = global_sdata[PASS1_BUF].common_data_ptr;
				cdptr->sync_distance = rule.sync_distance;
				cdptr->stream_depth = rule.stream_depth;
				if (rule.sync_distance == MAX_INS_STREAM_DEPTH || rule.sync_distance > rule.stream_depth) {
					/* use sync points in prolog and epilog */
					cdptr->num_sync_points = 2;
				}
				else {
					cdptr->num_sync_points = rule.stream_depth/rule.sync_distance + 2;
				}
			#ifndef SCTU
				for (j = 0; j < cdptr->num_sync_points; j++) {
					cdptr->sync_words[j] = ((2 << (rule.num_threads - 1)) - 1);
				}
			#else
				for(j = 0; j < cdptr->num_sync_points; j++) {
					cdptr->sync_words[j] = 0;
				}
			#endif
				cdptr = global_sdata[PASS2_BUF].common_data_ptr;
				cdptr->sync_distance = rule.sync_distance;
				cdptr->stream_depth = rule.stream_depth;
				if (rule.sync_distance == MAX_INS_STREAM_DEPTH || rule.sync_distance > rule.stream_depth) {
					cdptr->num_sync_points = 2; /* use sync points in prolog and epilog */
				}
				else {
					cdptr->num_sync_points = rule.stream_depth/rule.sync_distance + 2;
				}
				for(j = 0; j < cdptr->num_sync_points; j++) {
					cdptr->sync_words[j] = ((2 << (rule.num_threads - 1)) - 1);
				}
			} /* if rule.testcase_sync ends */

			/* Initialize annd generate initial seeds for all clients */
			generate_and_set_initial_seeds(rule.parent_seed);

			if (exit_flag) {
				cleanup_mem(0);
				if (exit_flag == 2) {
					hxfupdate(RECONFIG, &hd);
				}
				exit(0);
			}

			/* Initialize all the TIDs to 0.
			 * This will help us while cancelling threads in case of DR.
			 */
			for (i = 0; i < rule.num_threads; i++) {
				global_sdata[INITIAL_BUF].cdata_ptr[i]->tid = 0;
			}

#if defined(__HTX_LINUX__) && !defined(RHEL5)

			uint64 pg_addr, ls_pgoffset, page_size;

			if( (rule.prot_sao) && (prev_sao_val != rule.prot_sao) ) {

				page_size = getpagesize();

				/* Mark load/store area as SAO for all clients
				 * Make page_addr page aligned for mprotect requirement and
				 * add offset that was masked to load/store area size for len arg */
				for( i=0; i< MAX_NUM_CPUS; i++) {

					ls_pgoffset = (uint64)global_sdata[INITIAL_BUF].cdata_ptr[i]->ls_base[PASS2_BUF] & (page_size - 1);
					pg_addr = (uint64)global_sdata[INITIAL_BUF].cdata_ptr[i]->ls_base[PASS2_BUF] & (uint64) ~(page_size - 1);
					rc = mprotect((void *)pg_addr, ls_pgoffset + MAX_INS_STREAM_DEPTH*16 , PROT_SAO|PROT_READ|PROT_WRITE|PROT_EXEC);
					if(rc) {
						sprintf(msg, "mprotect(1) failed: error = %d(%s) ,client no = %d, rule id = %s Exiting...\n", errno, strerror(errno), i, rule.rule_id);
						hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
						cleanup_mem(0);
						exit(1);
					}
				}
				/* update previous sao value */
				prev_sao_val = rule.prot_sao;
			}
			else if ( (!rule.prot_sao) && (prev_sao_val != rule.prot_sao) ) {

				page_size = getpagesize();

				/* Mark pass2 shm segment as non SAO for all clients */
				for( i=0; i< MAX_NUM_CPUS; i++) {

					ls_pgoffset = (uint64)global_sdata[INITIAL_BUF].cdata_ptr[i]->ls_base[PASS2_BUF] & (page_size - 1);
					pg_addr = (uint64)global_sdata[INITIAL_BUF].cdata_ptr[i]->ls_base[PASS2_BUF] & (uint64) ~(page_size - 1);
					rc = mprotect((void *)pg_addr, ls_pgoffset + MAX_INS_STREAM_DEPTH*16 , PROT_READ|PROT_WRITE|PROT_EXEC);
					if(rc) {
						sprintf(msg, "mprotect(2) failed: error = %d(%s) ,client no = %d, rule id = %s Exiting...\n", errno, strerror(errno), i, rule.rule_id);
						hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
						cleanup_mem(0);
						exit(1);
					}
				}
				/* update previous sao value */
				prev_sao_val = rule.prot_sao;
			}
#endif

			/* setup timer now if specified in rule file */
			/* ----------------------------------------- */
			if (rule.tc_time) {
				if (rule.num_oper == 0) {
					rule_run_time = rule.tc_time;
				} else {
					rule_run_time = rule.tc_time / rule.num_oper;
				}
			}

			if (rule_run_time != 0) {
				rc = alarm(rule_run_time);
				sprintf(msg, "setting up alaram time: %d sec for rule number: %d, alarm RC: %d\n", rule_run_time, rule_no, rc);
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);
				if (rc) {
					sleep(rc);
				}
			}
			/* ----------------------------------------- */

			/* Create one pthread for each client to do build/simulate/execute function num_oper times.
			 */
			for(i = 0; i < rule.num_threads; i++) {
				cptr = global_sdata[INITIAL_BUF].cdata_ptr[i];
				cptr->client_no = i;

			#ifdef SCTU
				cptr->bcpu = bcpus_in_gang[i];
			#else
                /* Bind cpu is the logical cpu returned by syscfg */
                if (strstr("no", proc_env.proc_shared_mode)) {
                    cptr->bcpu = cpus_in_core[i];
                    if (cptr->bcpu == -1) {
                        sprintf(msg, "Error for core: %d: syscfg return cpu[%d] = %d\n", core_num, i, cpus_in_core[i]);
                        hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
                        exit_flag = 1;
                    }
                } else {
                   	cptr->bcpu = core_num;
                }

                /* This check is for equaliser. Run directly on the core number in the device. */
                if ( equaliser_flag ) {
                  	cptr->bcpu = core_num;
                }
			#endif

				if( exit_flag ) {
					cleanup_mem(0);
					if ( exit_flag == 2 ) {
						hxfupdate(RECONFIG, &hd);
					}
					exit(0);
				}

				/*DPRINT(hlog, "cptr..client no = %d\n",cptr->client_no);
				FFLUSH(hlog);*/
				pthread_attr_init(&cptr->attr);
				pthread_attr_setdetachstate(&cptr->attr, PTHREAD_CREATE_JOINABLE);

				rc = pthread_create(&cptr->tid, &cptr->attr, (void *(*)(void *))&client_func, (void *)cptr);
				if (rc) {
					int j;
					sprintf(msg, "pthread_create call failed with rc: %d, errno: %d, %s\n", rc, errno, strerror(errno));
					hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
					for(j = 0; j < i; j++) {
						pthread_cancel(global_sdata[INITIAL_BUF].cdata_ptr[j]->tid);
					}
					cleanup_mem(0);
					exit(1);
				}
			} /* for rule.num_threads ends */

			for(i = 0; i < rule.num_threads; i++) {
				cptr = global_sdata[INITIAL_BUF].cdata_ptr[i];
				rc = pthread_join(cptr->tid, NULL);
				if( rc && rc != ESRCH ) {
					sprintf(msg, "pthread_join failed with %d for client %d thread = %d\n", rc, i, (int)cptr->tid);
					hxfmsg(&hd, errno, HTX_HE_SOFT_ERROR, msg);
				}
				else {
					cptr->tid = 0;
				}
			}
			for(i = 0; i < rule.num_threads; i++) {
				cptr = global_sdata[INITIAL_BUF].cdata_ptr[i];
				total_num_oper += cptr->num_oper;
				total_num_pass2 += cptr->num_pass2;
			}

			avg_num_oper = total_num_oper / rule.num_threads;
			avg_num_pass2 = total_num_pass2 / rule.num_threads;
			/* cancel alarm if pending */
			if (rule_run_time != 0) {
				alarm(0);
				alarm_expired = 0;
				sprintf(msg, "Timer flag reset to %d after thread join\n", alarm_expired);
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);
			}

			if (exit_flag) {
				cleanup_mem(0);
				if ( exit_flag == 2 ) {
					hxfupdate(RECONFIG, &hd);
				}
				exit(0);
			}
			/*-------------------------------------------------*/
			/********          update stats            *********/
			/*-------------------------------------------------*/
    		#ifndef SCTU
			hd.good_others = avg_num_oper * avg_num_pass2;
    		hd.num_instructions = avg_num_oper * rule.stream_depth * avg_num_pass2;
    		#else
			hd.good_others = avg_num_oper * SCTU_PP_COUNT;
    		hd.num_instructions = avg_num_oper * rule.stream_depth * SCTU_PP_COUNT;
    		#endif

#ifdef AWAN
			insts_sim += hd.num_instructions;
			rewind(fp_sim);
			fprintf(fp_sim, "%llu\n", (unsigned long long)(insts_sim / 1024));
			fflush(fp_sim);

			printf("Stanza %s completed, PASSES = %d.\n", rule.rule_id, rule.num_oper);
			fflush(stdout);
#endif
			hxfupdate(UPDATE, &hd);
			/* save instruction execution stats for current stanza */
			stanza_stats[rule_no].test_id = hd.test_id; 
			for (i = 0; i < rule.num_threads; i++) {
				cptr = global_sdata[INITIAL_BUF].cdata_ptr[i];
				stanza_stats[rule_no].client_stats[i].num_enabled_instr = cptr->num_enabled_ins;
				for (j = 0; j < cptr->num_enabled_ins; j++) {
					if (cptr->enabled_ins_table[j].run_flag) {
						stanza_stats[rule_no].client_stats[i].num_tested_instr++;
					}
				}
				stanza_stats[rule_no].client_stats[i].tested_percent = stanza_stats[rule_no].client_stats[i].num_tested_instr / stanza_stats[rule_no].client_stats[i].num_enabled_instr;
			}
			
		} /* stanza loop end here */

		hxfupdate(FINISH, &hd);
		cycle_count++;
	
		/* dump instruction execution stats for the test cycle */
		char instr_fname[128];
		sprintf(instr_fname, "/tmp/%s_instr_stats", (char *)(dinfo.device_name+5));
		FILE * fptr = fopen((const char *)instr_fname,"w");
		if (fptr != NULL) {
			fprintf(fptr, "Cycle # %lld\n", cycle_count);
			for (i = 0; i < num_active_rules; i++) {
				sprintf(msg, "Test id: %d ", stanza_stats[i].test_id);
				for (j = 0; j < rule.num_threads; j++) {
					sprintf(msg, "%s client[%d]: %d ", msg, j, stanza_stats[i].client_stats[j].tested_percent); 
				}
				fprintf(fptr, "\n%s\n", msg);
				fflush(fptr);
			}
			fclose(fptr);
		}

#if defined(AWAN) && !defined(RUNALLRFS)
		printf("\nFINISHED ALL STANZAS IN RULE FILE.\n");
		printf("TOTAL RULE FILE PASSES COMPLETED = %d.\n", ++rule_file_passes);
		fflush(stdout);
	} while (1);
#else
	}while ( (rc = strcmp(dinfo.run_type, "REG") == 0) || (rc1 = strcmp(dinfo.run_type, "EMC") == 0) );
#endif
	cleanup_mem(0);
	exit(0);
}

#ifndef __HTX_LINUX__
/**************************************************************************/
/* Get the output of passed shell command                                 */
/**************************************************************************/
int get_cmd_result(char cmd[1000])
{
	char buf[1024], *rc;
	FILE  *fp = NULL;
	int exer, cnt=0;

	do {
		fp = popen(cmd,"r");
		cnt++;
	} while ( fp == NULL && errno == EINTR && cnt < NUM_RETRIES );

	if ( fp == NULL ) {
		if ( errno == EINTR ) {
			sprintf(msg, "%s:popen failed for command \"%s\" with error %d(%s) even after %d retries.\n",
						  __FUNCTION__, cmd, errno, strerror(errno), cnt);
			hxfmsg(&hd, 0, HTX_HE_INFO, msg);
			return(-1);
		}
		else {
			sprintf(msg, "%s:popen failed for command \"%s\" with error %d(%s).\n",
						  __FUNCTION__, cmd, errno, strerror(errno));
			hxfmsg(&hd, 0, HTX_HE_HARD_ERROR, msg);
			return(-1);
		}

	}

	cnt = 0;

	do {
		rc = fgets(buf,1024,fp);
		cnt++;
	} while ( rc == NULL && errno == EINTR && cnt < NUM_RETRIES );

	if (rc == NULL) {
		if ( errno == EINTR ) {
			sprintf(msg, "%s: fgets failed for command \"%s\" with error %d(%s) even after %d retries.\n",
						  __FUNCTION__, cmd, errno, strerror(errno), cnt);
			hxfmsg(&hd, 0, HTX_HE_INFO, msg);
			pclose(fp);
			return(-1);
		}
		else if ( errno == 0 ) {
			sprintf(msg, "%s: fgets failed for \"%s\" with error %d(%s)\n",
						  __FUNCTION__, cmd, errno, strerror(errno));
			hxfmsg(&hd, 0, HTX_HE_INFO, msg);
			pclose(fp);
			return(-2);
		}
		else {
			sprintf(msg, "%s:fgets failed for \"%s\" with error %d(%s)\nHTX Limitation. Refer webpage.",
						  __FUNCTION__, cmd, errno, strerror(errno));
			hxfmsg(&hd, 0, HTX_HE_HARD_ERROR, msg);
			pclose(fp);
			return(-1);
		}
	}

	sscanf(buf,"%d",&exer);
	pclose(fp);
	return(exer);
}
#endif

void SIGTERM_hdl(int sig)
{
	exit_flag = 1 ;
	sprintf(msg, "%s received a SIGTERM\n", hd.HE_name);
	hxfmsg(&hd, 0, HTX_HE_INFO, msg);
}

void SIGINT_handler(int sig)
{
	exit_flag = 1 ;
	sprintf(msg, "%s received a SIGINT\n", hd.HE_name);
	hxfmsg(&hd, 0, HTX_HE_INFO, msg);
}

void SIGALRM_hdl(int sig)
{
	/*int rc;*/
	alarm_expired = 1;
	sprintf(msg, "%s received a SIGALRM, alarm_expired: %d\n", hd.HE_name, alarm_expired);
	hxfmsg(&hd, 0, HTX_HE_INFO, msg);
}

#ifdef __HTX_LINUX__
void SIGHW_handler(int sig, siginfo_t *si, void *ucontext)
{
	int i, pass = -1;
	volatile int client_num = -1 ;
#ifdef AWAN
	static volatile sig_atomic_t nontcsig = 0;
#endif
	ucontext_t *u = (ucontext_t *)ucontext;
	uint32* iar =  (uint32*)u->uc_mcontext.regs->nip;
	struct client_data *cptr, *nextcptr;

	/* Find which client received the signal */
	for( i=0; i<(MAX_NUM_CPUS - 1); i++ ) {

		cptr = global_sdata[INITIAL_BUF].cdata_ptr[i];
		nextcptr = global_sdata[INITIAL_BUF].cdata_ptr[i+1];

		/* check if ptrs are initialized by now
			else this could be a non tc signal */
		if( cptr && (cptr->tc_ptr[PASS1_BUF]) ) {
			;
		}
		else {
			break;
		}

		if ((((uint32 *)cptr->tc_ptr[PASS1_BUF]) < iar) && (((uint32 *)nextcptr->tc_ptr[PASS1_BUF]) > iar)) {
			client_num = i;
			pass = PASS1_BUF;
			break;
		}
		else if ((((uint32 *)cptr->tc_ptr[PASS2_BUF]) < iar) && (((uint32 *)nextcptr->tc_ptr[PASS2_BUF]) > iar)) {
			client_num = i;
			pass = PASS2_BUF;
			break;
		}

	}

	if( client_num == -1 ) { /* check if the signal was due to the last client testcase */

		cptr = global_sdata[INITIAL_BUF].cdata_ptr[MAX_NUM_CPUS - 1];
		if( cptr && cptr->tc_ptr[PASS1_BUF] )  { /*check for valid ptrs*/

			if ((((uint32 *)cptr->tc_ptr[PASS1_BUF]) < iar) && (iar < ((uint32 *)global_sdata[PASS1_BUF].common_data_ptr))) {
				client_num = MAX_NUM_CPUS - 1;
				pass = PASS1_BUF;
			}
			else if ((((uint32 *)cptr->tc_ptr[PASS2_BUF]) < iar) && (iar < ((uint32 *)global_sdata[PASS2_BUF].common_data_ptr))) {
				client_num = MAX_NUM_CPUS - 1;
				pass = PASS2_BUF;
			}
		}
	}

	/* signal received due to tc instruction, capture first signal data for each client */
	if( (client_num != -1) && !global_sdata[INITIAL_BUF].cdata_ptr[client_num]->sigdata.iar ){

		struct signal_info *sigdataptr = &(global_sdata[INITIAL_BUF].cdata_ptr[client_num]->sigdata);
		cptr = global_sdata[INITIAL_BUF].cdata_ptr[client_num];
		sigdataptr->iar = iar;
		sigdataptr->sig_num = sig;
		sigdataptr->pass_num = pass;
		sigdataptr->client_num = client_num;
		sigdataptr->client_seed = cptr->original_seed;
		sigdataptr->parent_seed = original_seed;

		for( i=0; i<NUM_GPRS; i++){
			sigdataptr->tcc.gprs[i] = u->uc_mcontext.regs->gpr[i];
			sigdataptr->tcc.fprs[i][0] = u->uc_mcontext.fp_regs[i];
		}
		sigdataptr->tcc.sprs.fpscr = u->uc_mcontext.fp_regs[32];
		sigdataptr->tcc.sprs.lr = u->uc_mcontext.regs->link;
		sigdataptr->tcc.sprs.xer = u->uc_mcontext.regs->xer;
		sigdataptr->tcc.sprs.ctr = u->uc_mcontext.regs->ctr;
		sigdataptr->tcc.sprs.msr = u->uc_mcontext.regs->msr;
		exit_flag = 1;
	}

#ifdef AWAN
	/* signal didn't come from tc , capture first non tc signal info */
	if( (client_num == -1) && !nontcsig ) {
		nontcsig = 1;
		nontcsigdata.iar = iar;
		nontcsigdata.sig_num = sig;
		exit_flag = 1;
	}

	u->uc_mcontext.regs->nip = (char *)u->uc_mcontext.regs->nip + 4;
#endif
}
#endif

#ifndef __HTX_LINUX__
void SIGCPUFAIL_handler(int sig)
{

	if ( exit_flag ) {
		return;
	}

	exit_flag = 1 ;
	hxfupdate(RECONFIG_CPUFAIL, &hd);

	sprintf(msg, "%s received a SIGCPUFAIL\n", hd.HE_name);
	hxfmsg(&hd, 0, HTX_HE_INFO, msg);
}

void SIGRECONFIG_handler(int sig, int code, struct sigcontext *scp)
{
	char dr_info_msg[512];
    int rc, i, bound_cpu;
    uint32  client_no;
    dr_info_t dr_info;          /* Info about DR operation, if any */

	/*
	 * sprintf(msg, "DR: SIGRECONFIG signal received by %s.\n", hd.HE_name);
     * hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	 */

    errno = 0;

    do {
    	rc = dr_reconfig(DR_QUERY, &dr_info);
    } while (rc < 0 && errno == EINTR);

    if (rc == -1) {
        if (errno != ENXIO){
            hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, "dr_reconfig(DR_QUERY) call failed.\n");
        }
        return;
    }

	/* Process DR signal.
     * CPU Add:
     * It is taken care by supervisor for FPU.
     * For SCTU will set flag to recalculate cpus list.
	 * CPU Remove:
	 * Unbind thread on outgoing cpu and set flag to
	 * recalculate cpus list.
     * Other supported operations:
 	 * Acknowledge DR signal.
     */

	sprintf(dr_info_msg, "DR: DLPAR details.\n"
                 "Phase - check:%d, pre:%d, post:%d, posterror:%d\n"
                 "Type - add:%d, rem:%d, cpu:%d, mem:%d. CPU - lcpu:%d, bcpu:%d\n",
                 dr_info.check, dr_info.pre, dr_info.post, dr_info.posterror,
                 dr_info.add, dr_info.rem, dr_info.cpu, dr_info.mem, dr_info.lcpu, dr_info.bcpu);

	/* Check for CPU removal in DR CHECK phase. */
	if (dr_info.check && dr_info.cpu && dr_info.rem) {

		/* Look for the thread bound to the CPU getting removed.*/
		for (i = 0; i < rule.num_threads; i++) {
			if (dr_info.bcpu == global_sdata[INITIAL_BUF].cdata_ptr[i]->bcpu) {
				bound_cpu = global_sdata[INITIAL_BUF].cdata_ptr[i]->bcpu;
				client_no = global_sdata[INITIAL_BUF].cdata_ptr[i]->client_no;

				rc = unbind_thread(client_no);
				if (rc  > 0 ) {
					strcat(msg, dr_info_msg);
					sprintf(msg, "unbind_thread failed, client: %d, bcpu: %d, errno = %d\n", client_no, bound_cpu, errno);
					strcat(msg, dr_info_msg);
					hxfmsg(&hd, 0, HTX_HE_SOFT_ERROR, msg);
				}
				exit_flag = 2;

				sprintf(msg, "Action - Will unbind bcpu %d, due to cpu getting removed.\n", dr_info.bcpu);
				strcat(msg, dr_info_msg);
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);

                if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)) {
                    sprintf(msg, "dr_reconfig(DR_RECONFIG_DONE) failed in check phase for cpu %d !!! errno: %d\n", dr_info.bcpu, errno);
				} else {
                    sprintf(msg, "DR: DR_RECONFIG_DONE successful in check phase for cpu %d.\n", dr_info.bcpu);
				}
				return;
			}
		}

		/* Acknowledge DR signal. log msg only in case of failure */
		if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)) {
			sprintf(msg, "dr_reconfig(DR_RECONFIG_DONE) failed. errno: %d\n", errno);
			strcat(msg, dr_info_msg);
			hxfmsg(&hd, 0, HTX_HE_SOFT_ERROR, msg);
        }

		return;
	}


	/* Process other DR operations. */
	if (dr_info.mem || dr_info.hibernate || dr_info.cpu || dr_info.ent_cap) {

        /* Recalculate cpus for CPU add. */
        if (dr_info.post && dr_info.cpu && dr_info.add) {
		  #ifdef SCTU
            if (num_cpus_to_test < SCTU_GANG_SIZE) {
				sprintf(msg, "Action - Recalculating cpus in gang for bcpu: %d, added\n", dr_info.bcpu);
                update_bcpu_info = 1;
            }
		  #else
			sprintf(msg, "Action - Recalculating cpus for bcpu: %d, added\n", dr_info.bcpu);
            update_bcpu_info = 1;
		  #endif
			strcat(msg, dr_info_msg);
			hxfmsg(&hd, 0, HTX_HE_INFO, msg);
        }

		/* check again for cpu remove if missed out in check phase */
		if (dr_info.cpu && dr_info.rem) {
            for (i = 0; i < rule.num_threads; i++) {
                if (dr_info.bcpu == global_sdata[INITIAL_BUF].cdata_ptr[i]->bcpu) {
                    if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)) {
                    	sprintf(msg, "dr_reconfig(DR_RECONFIG_DONE) failed in check phase for cpu %d !!! errno: %d\n", dr_info.bcpu, errno);
					} else {
                        sprintf(msg, "DR: DR_RECONFIG_DONE successful on pre/post case for cpu %d.\n", dr_info.bcpu);
					}
                    hxfmsg(&hd,0,HTX_HE_INFO,msg);
                    return;
                }
            }
        }

		/* Acknowledge DR signal. log msg only in case of failure */
		if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)){
			sprintf(msg,"dr_reconfig(DR_RECONFIG_DONE) failed.error : %d \n", errno);
			strcat(msg, dr_info_msg);
			hxfmsg(&hd,0,HTX_HE_INFO,msg);
        }
	}

	return;
}
#endif

/* SIGUSR2 handler for Hotplug (DR) handling on Linux.
   HTX supervisor would send SIGUSR2 after detecting any CPU add/remove.
   This handler calculates number of cpus currently online to decide
   if hotplug operation done was for CPU add or remove. It also sets
   flag for recalculating cpus for cpu add/remove operation.
*/

#ifdef __HTX_LINUX__
int SIGUSR2_handler(void)
{
        int prev_num_cpus;
        char temp_str[1024];

        strcpy(temp_str, "Unknown");

        /* Check number of cpus currently online and compare with previous number of cpu count. */
	prev_num_cpus = num_cpus;
        num_cpus = get_nprocs();

        /* Check for CPU add/Remove. Exerciser would need to recalculate cpus in core/gang for cpu hotplug. */
        if(num_cpus > prev_num_cpus) {
                strcpy(temp_str, "Cpu Add");
		update_bcpu_info = 1;
        }

        if(num_cpus < prev_num_cpus) {
                strcpy(temp_str, "Cpu Remove");
		update_bcpu_info = 1;
        }

        sprintf(msg, "SIGUSR2 signal received for Hot Plug(DR) - %s.\n", temp_str);
        sprintf(temp_str, "Current cpu count = %d, previous cpu count = %d\n", num_cpus, prev_num_cpus);
        strcat(msg, temp_str);
        hxfmsg(&hd, 0, HTX_HE_INFO, msg);

        return 0;
}
#endif

/*
 * Thread function. Thread will get birth and get killed here !
 */
void
client_func(void *cinfo)
{
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = ((struct client_data *)cinfo);

	volatile uint32 client_no = cptr->client_no, bcpu_no = cptr->bcpu, num_oper = 0, i;
	int rc, miscomparing_num; /*a=0;*/
	uint64 tick_start, tick_end;
	int32 seed_again, num_pass2 = 0;
	uint16 virt_cpu = 0;
	struct flock lockp;
	time_t ltime;
   	struct tm *tm;
   	char timestamp[16];
	int sync_timed_out = 0;

#ifdef SCTU
	int pp_cnt;
#endif

	DPRINT(cptr->clog, "Entering Client#%d function.\n", client_no);

	rc = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if ( rc ) {
		sprintf(msg, "%s:setting of cancel state failed for %d with %d(%s)\n",
					 __FUNCTION__, client_no, errno, strerror(errno));
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}

	rc = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	if ( rc ) {
		sprintf(msg, "%s:setting of cancel type failed for %d with %d(%s)\n",
					 __FUNCTION__, client_no, errno, strerror(errno));
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}

#ifndef SCTU
	if (exit_flag) {
		if (!rule.testcase_sync) {
			pthread_exit(NULL);
		}
	}
#endif
	/*
	 * Read instructions filter masks from rule file and apply them on instruction table.
	 */
    rc = filter_masked_instruction_categories(client_no, 0, master_instructions_array);
	if (rc == 0) {/* num enabled instructions */
		sprintf(msg, "Client: %d: filter_masked_instruction_categories: failed to select any instruction for the mask in rule file !!!\n", client_no);
        hxfmsg(&hd, rc, HTX_HE_SOFT_ERROR, msg);
        pthread_mutex_lock(&exit_flag_lock);
        exit_flag = 1;
        pthread_mutex_unlock(&exit_flag_lock);
        if (!rule.testcase_sync) {
            pthread_exit(&rc);
        }
    }
    /*
 	 * Create category wise instruction lists to manage the biasing.
     */
	rc = create_ins_cat_wise_tables(client_no);
    if (rc) { /* Error condition */
    	sprintf(msg, "create_ins_cat_wise_tables: failed for client# %d!\n", client_no);
        hxfmsg(&hd, rc, HTX_HE_SOFT_ERROR, msg);
		pthread_mutex_lock(&exit_flag_lock);
		exit_flag = 1;
		pthread_mutex_unlock(&exit_flag_lock);
		if (!rule.testcase_sync) {
        	pthread_exit(&rc);
		}
    }
    /*
	 * Create infrastructure to manage instructions biasing.
     */
	set_ins_bias_array(client_no);
	/*
 *   * Initialize client specific data, VSRs, sim fptr etc
 *   */
	rc = initialize_client(client_no);
    if (rc) { /* Error condition */
    	sprintf(msg, "initialize_client: failed for client# %d!\n", client_no);
        hxfmsg(&hd, rc, HTX_HE_SOFT_ERROR, msg);
		pthread_mutex_lock(&exit_flag_lock);
		exit_flag = 1;
		pthread_mutex_unlock(&exit_flag_lock);
		if (!rule.testcase_sync) {
        	pthread_exit(&rc);
		}
    }

#ifndef __HTX_LINUX__
	/* AIX part */
	rc = bind_thread(bcpu_no);
	if (rc) {
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, "bind failed. Exiting....\n");
		pthread_mutex_lock(&exit_flag_lock);
		exit_flag = 1;
		pthread_mutex_unlock(&exit_flag_lock);
	#ifndef SCTU
		if (!rule.testcase_sync) {
			pthread_exit((char *)NULL);
		}
	#endif
	}
#else
	/* Linux: syscfg library call */
	if (strstr("no", proc_env.proc_shared_mode)) {
		rc = htx_bind_thread(bcpu_no, cptr->phy_bcpu);
	} else {
		/* In shared mode, use only lcpu as pcpu keeps on changing */
		rc = htx_bind_thread(bcpu_no, -1);
	}

	/* Handle bind failure cases. */
    if(rc < 0) {
		if (rc == -2 || rc == -1) {
		    sprintf(msg, "CPU(lcpu=%d, pcpu=%d) appears to be offline. Binding to that cpu is not possible for client=%d\n", bcpu_no, cptr->phy_bcpu, client_no);
                	hxfmsg(&hd, rc, HTX_HE_INFO, msg);
		    update_bcpu_info = 1;
			if (strstr("yes", proc_env.proc_shared_mode)) {
				exit_flag = 2;
			}
	        #ifndef SCTU
			if (!rule.testcase_sync) {
		    	pthread_exit((char *)&rc);
			}
	        #endif

	    } else  {
		    sprintf(msg, "Bind failed for lcpu: %d, pcpu: %d Client: %d with rc=%d.\n", bcpu_no, cptr->phy_bcpu, client_no,rc);
		    hxfmsg(&hd, rc, HTX_HE_SOFT_ERROR, msg);
			pthread_mutex_lock(&exit_flag_lock);
		    exit_flag = 1;
			pthread_mutex_unlock(&exit_flag_lock);
	        #ifndef SCTU
			if (!rule.testcase_sync) {
		    	pthread_exit((char *)&rc);
			}
	        #endif
        }
	} else {
		/* Bind SUCCESS */
		cptr->phy_bcpu = rc;
		DPRINT(cptr->clog, "\nCore#: %d, Client: %d with lbcpu: %d bound to pbcpu: %d successfully\n", core_num, client_no, bcpu_no, cptr->phy_bcpu);

	}
#endif


	for(i = 0; i < rule.unaligned_data_pc[client_no]; i++) {
		cptr->data_alignment[i] = 0;
	}
	for(i = rule.unaligned_data_pc[client_no]; i < 100; i++) {
		cptr->data_alignment[i] = 1;
	}
	#ifndef __HTX_LINUX__
		tick_start = read_tb();
	#else
		read_tb(tick_start);
	#endif
	
	/***************************************************
 	 *              Num Oper Loop                      *
     ***************************************************/
	while( num_oper < rule.num_oper  || rule.num_oper == 0 ) {
		/*
		 * All work done in this while loop is repeated for every num_oper, so be creaful it will
		 * affect performance.
		 */
		int rc;

#ifndef SCTU
        /*
		 * for characterization special requirement to log seed into system wide file: seedfile.txt
		 * rule parameter log_seed flag is set to true
 		 * if valid testcase execution time is specified in stanza where NPASS2 is infinite
 		 * and rule.num_oper is a fixed value
 		 * This num oper for which the seed being logged should be executed exactly for time assigned to it
 		 */
		if (rule.log_seed) {
			/* log info into file and continue next num_oper
			 *--------------------------------------------------------------
			 *<exer_instance>     <seed>      <start timestamp>   <Duration>
			 */
			ltime = time(NULL);
   			tm = localtime(&ltime);
   			sprintf(timestamp, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
			/* locks at thread level and then process level */
			pthread_mutex_lock(&seed_log_lock);
			/*flock(fp_tblog, LOCK_EX);*/
			lockp.l_type = F_WRLCK;
			lockp.l_whence = SEEK_CUR;
			lockp.l_start = 0;
			lockp.l_len = 0;
			lockp.l_pid = getpid();
			fcntl((uint64)fp_tblog, F_GETLK, &lockp);
			#ifdef __HTX_LINUX__
			virt_cpu = cptr->phy_bcpu;
			#else
			virt_cpu = bcpu_no;
			#endif
			fprintf(fp_tblog, "vCpu: %02d, seed: 0x%08x, log time: %s, run time: %d, rule id: %d, cycle: %lld\n", virt_cpu, cptr->original_seed, timestamp, rule_run_time, hd.test_id, cycle_count);
			fflush(fp_tblog);
			lockp.l_type = F_UNLCK;
			fcntl((uint64)fp_tblog, F_SETLK, &lockp);
			/*flock(fp_tblog, LOCK_UN);*/
			pthread_mutex_unlock(&seed_log_lock);
		}

		if (exit_flag) {
			if (!rule.testcase_sync) {
				pthread_exit((void *)NULL);
			}
		}
#endif
		rc = create_context(client_no);
		if (rc) {
			pthread_mutex_lock(&exit_flag_lock);
        	exit_flag = 1;
    		pthread_mutex_unlock(&exit_flag_lock);
			sprintf(msg, "create_context failed for client no - %d", client_no);
			hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
			#ifndef SCTU
			if (!rule.testcase_sync) {
				pthread_exit((void *)NULL);
			}
			#endif
		}
		/*
		 * Build testcase
		 */
		sdata = &global_sdata[INITIAL_BUF];
		for(i = 0; i < 100; i++) {
			cptr->rand_nos[i] = i;
		}
		shuffle(client_no, cptr->rand_nos, 100);
		/* data_alignment_ctr is used as an index into random_nos array to fetch next random no. */
		cptr->data_alignment_ctr = 0;
		bzero(cptr->tc_ptr[INITIAL_BUF], sizeof(struct testcase));
		(copy_prolog_fptr)(client_no);
		rc = build_testcase(client_no);
		if (rc) {
			pthread_mutex_lock(&exit_flag_lock);
        	exit_flag = 1;
    		pthread_mutex_unlock(&exit_flag_lock);
			#ifndef SCTU
            if (!rule.testcase_sync) {
                pthread_exit(&rc);
            }
        	#endif
		}
		(copy_epilog_fptr)(client_no);
		impart_context_in_memory(client_no);

		/*update_ea_off_array(client_no); for SCTU*/
		reinit_ls_base(client_no);    /* feature:946581 making common with fpu */

#ifdef SCTU

		pthread_mutex_lock(&testcase_built_lock);
		threads_built_testcase++;
		if( threads_built_testcase == rule.num_threads ) {
			copy_common_data(PASS1_BUF, client_no);
			miscompare_in_numoper = 0;
			if( exit_flag ) {
				sctu_exit_flag = 1;
			}
			for( i=0; i<rule.num_threads; i++ ){
				sem_post(&pass1_exec_sem);
			}
		}
		pthread_mutex_unlock(&testcase_built_lock);
		sem_wait(&pass1_exec_sem);
		/* make all threads exit for sctu */
		if( sctu_exit_flag ){
			pthread_exit(NULL);
		}

		pthread_mutex_lock(&pass1_exec_lock);

#endif

		if(rule.test_method == CORRECTNESS_CHECK) {
			/*
			 * run testcase simulation
			 */
			DPRINT(cptr->clog, "fpu%d: In SIM\n", core_num);
			FFLUSH(cptr->clog);

			sdata = &global_sdata[PASS1_BUF];
			populate_buf(PASS1_BUF, client_no);
			simulate_testcase(client_no);
			adjust_vsrs_memory_image(client_no, PASS1_BUF);
		}
		else {
			/*
			 * Execute test case on hardware
			 */
			DPRINT(cptr->clog, "fpu%d: In HW\n", core_num);
			FFLUSH(cptr->clog);

			sdata = &global_sdata[PASS1_BUF];
			populate_buf(PASS1_BUF, client_no);
			execute_testcase(client_no, PASS1_BUF);
			adjust_vsrs_memory_image(client_no, PASS1_BUF);
		}

#ifdef SCTU
		pthread_mutex_unlock(&pass1_exec_lock);
#else

		if (rule.testcase_sync) {
			/* Synchronization point */
			pthread_mutex_lock(&clients_sync_lock);
            if (clients_sync_count < rule.num_threads) {
                clients_sync_count++;
            }
            if (clients_sync_count == rule.num_threads) {
                clients_sync_count = 0;
                pthread_cond_broadcast(&clients_sync_cond);
            }
            else {
                pthread_cond_wait(&clients_sync_cond, &clients_sync_lock);
            }
            pthread_mutex_unlock(&clients_sync_lock);

            if (exit_flag) {
                sprintf(msg, "Client: %d, Num Oper: %d, Exit Point: PASS2, errno: %0X (%s)\n", client_no, num_oper, errno, strerror(errno));
                hxfmsg(&hd, 0, HTX_HE_INFO, msg);
                for(i = 0; i < rule.num_threads; i++) {
                    if (i == client_no) continue;
                    /*pthread_kill(global_sdata[INITIAL_BUF].cdata_ptr[client_no]->tid, SIGKILL);*/
                    pthread_cancel(global_sdata[INITIAL_BUF].cdata_ptr[client_no]->tid);
                }
                pthread_exit(NULL);
            }
		}
#endif

#ifdef SCTU
		/************************************************************/
		/* 				parallel pass loop 							*/
		/************************************************************/
		for(pp_cnt = 0; pp_cnt < SCTU_PP_COUNT; pp_cnt++) {

		pthread_mutex_lock(&pass2_ready_count_lock);
		threads_ready_for_pass2++;

		if( threads_ready_for_pass2 == rule.num_threads ) {
			if( rule.testcase_sync ){
				initialize_sync_words(PASS2_BUF);
			}
			copy_common_data(PASS2_BUF, client_no);

			if( pp_cnt == 0 ){
#ifdef __HTX_LINUX__
				errno = 0; /* Clear any errno set earlier */
				orig_priority = getpriority(PRIO_PROCESS, 0 );
				if ( orig_priority == -1 && errno ) {
					sprintf(msg, "getpriority(at testcase execn start) call failed with errno = %d (%s) \n", errno, strerror(errno));
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					exit_flag = 1;
				}

				rc = setpriority (PRIO_PROCESS, 0, 0);
				if ( rc == -1 && errno ) {
					sprintf(msg, "setpriority(at testcase execn start) call failed with errno = %d (%s) \n", errno, strerror(errno));
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					exit_flag = 1;
				}

#else
				/*
				   Using nice syscall for AIX. AIX seems to have only one nice value per process,
				   so it should change priority for all threads. AIX priority formulae is as below
				   priority = base_pri + nice + cpu_penalty
				*/
				errno = 0;
				rc = nice(-10);
				if(rc == -1)
				{
					sprintf(msg, "nice -10 failed. errno = %d\n", errno);
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					exit_flag = 1;
				}
#endif
			}

			if( exit_flag ) {
				sctu_exit_flag = 1;
			}
			for( i=0; i<rule.num_threads; i++ ){
				sem_post(&pass2_exec_sem);
			}
		}

		pthread_mutex_unlock(&pass2_ready_count_lock);
		sem_wait(&pass2_exec_sem);
		/* make all threads exit for sctu */
		if( sctu_exit_flag ){
			pthread_exit(NULL);
		}

		/*
		 * Execute test case on hardware
		 */
		sdata = &global_sdata[PASS2_BUF];
		populate_buf(PASS2_BUF, client_no);
		execute_testcase(client_no, PASS2_BUF);
		adjust_vsrs_memory_image(client_no, PASS2_BUF);
		/*
		 * Compare results
		 */
		rc = 0;
		if (rule.compare_flag) {
			rc = compare_results(client_no, &miscomparing_num);
			DPRINT(cptr->clog,"Comparison result = %d\n",rc);
			FFLUSH(cptr->clog);
		}

		cptr->miscompare_in_numoper = rc;
#else  /* SCTU part ends */
		/* Multiple pass2 for FPU and CPU only */
		if (rule.log_seed == 1) {
			if ((rule_run_time != 0)) {
				while (1) {
					/* PASS2 unlimited, contolled by timer only */
					sdata = &global_sdata[PASS2_BUF];
					populate_buf(PASS2_BUF, client_no);
					execute_testcase(client_no, PASS2_BUF);
					adjust_vsrs_memory_image(client_no, PASS2_BUF);
					/*
		 			 * Compare results
		 			 */
					rc = 0;
					if (rule.compare_flag) {
						rc = compare_results(client_no, &miscomparing_num);
						DPRINT(cptr->clog,"Comparison result = %d\n",rc);
						FFLUSH(cptr->clog);
					}
					if (rc) {
						break;
					}
					num_pass2++;
					if (exit_flag) {
						if (!rule.testcase_sync) {
							pthread_exit(NULL);
						}
					}
					if (alarm_expired) {
						pthread_mutex_lock(&thread_count_lock);
						thread_count++;
						if (thread_count == rule.num_threads) {
							thread_count = 0;
							alarm_expired = 0;
							alarm(rule_run_time);
							pthread_cond_broadcast(&thread_count_cond);
						}
						else {
							/* wait for the thread_count to become rule.num_threads */
							pthread_cond_wait(&thread_count_cond, &thread_count_lock);
						}
						pthread_mutex_unlock(&thread_count_lock);
						break;
					}
				}
			}
			else {
				/* this part have beeen taken care in rule file validation */
				exit_flag = 1;
				sprintf(msg, "Inalid rule file configuration found, log_seed: %d, testcase time: %d\n", rule.log_seed, rule_run_time);
				hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
				if (!rule.testcase_sync) {
					pthread_exit((void *)&exit_flag);
				}
			}
		}
		else {
			if ((rule_run_time != 0)) {
				sdata = &global_sdata[PASS2_BUF];
				populate_buf(PASS2_BUF, client_no);
				execute_testcase(client_no, PASS2_BUF);
				adjust_vsrs_memory_image(client_no, PASS2_BUF);
				/*
		 		 * Compare results
		 		 */
				rc = 0;
				if (rule.compare_flag) {
					rc = compare_results(client_no, &miscomparing_num);
					DPRINT(cptr->clog,"Comparison result = %d\n",rc);
					FFLUSH(cptr->clog);
				}
				if (alarm_expired) {
					pthread_mutex_lock(&thread_count_lock);
					thread_count++;
					if (thread_count == rule.num_threads) {
						thread_count = 0;
						alarm_expired = 0;
						alarm(rule_run_time);
						pthread_cond_broadcast(&thread_count_cond);
					}
					else {
						/* wait for the thread_count to become rule.num_threads */
						pthread_cond_wait(&thread_count_cond, &thread_count_lock);
					}
					pthread_mutex_unlock(&thread_count_lock);
					if (!rc) {
						num_pass2++;
						/* break infinite num_oper loop and move to next stanza */
						break;
					}
				}
			}
			else {
				/* default for rule.num_pass2 is 1 */
				for (num_pass2 = 0; num_pass2 < rule.num_pass2; num_pass2++) {
					sdata = &global_sdata[PASS2_BUF];
					populate_buf(PASS2_BUF, client_no);
					execute_testcase(client_no, PASS2_BUF);
					adjust_vsrs_memory_image(client_no, PASS2_BUF);
					/*
		 			 * Compare results
		 			 */
					rc = 0;
					if (rule.compare_flag) {
						rc = compare_results(client_no, &miscomparing_num);
						DPRINT(cptr->clog,"Comparison result = %d\n",rc);
						FFLUSH(cptr->clog);
					}
					if (rc) break;
					if (exit_flag) {
						if (!rule.testcase_sync) {
							pthread_exit(NULL);
						}
					}
				}
			}
		}

#endif
		/*************************************************************
		 * Evaluate Miscompare RC: common code for SCTU, FPU and CPU *
		 *************************************************************/
		if(rc) {
#ifndef __HTX_LINUX__
			if ( rule.enable_trap == 1 ) {
				struct client_data *cptr = global_sdata[PASS1_BUF].cdata_ptr[client_no];
				struct testcase*ptr1 = global_sdata[PASS1_BUF].cdata_ptr[client_no]->tc_ptr[PASS1_BUF];
				struct testcase*ptr2 = global_sdata[PASS2_BUF].cdata_ptr[client_no]->tc_ptr[PASS2_BUF];

				trap(0xBEEFDEAD,rc, ptr1->tcc, ptr2->tcc, ptr1->tc_ins, cptr->ls_base[PASS1_BUF], cptr->ls_base[PASS2_BUF]);
			}
#endif
			char miscomp_name[50];

			if(shifted_pvr_os <= 0x3e) {
				dump_testcase_p6(client_no, num_oper, rc, miscomparing_num);
			}
			else {
				dump_testcase_p7(client_no, num_oper, rc, miscomparing_num);
			}
			switch (rc) {
				case 1: sprintf(miscomp_name,"VSR");
						break;
				case 2: sprintf(miscomp_name,"GPR");
						break;
				case 3: sprintf(miscomp_name,"FPSCR");
						break;
				case 4: sprintf(miscomp_name,"VSCR");
						break;
				case 5: sprintf(miscomp_name,"CR");
						break;
				case 6: sprintf(miscomp_name,"Load/Store region");
						break;
			}
#ifndef __HTX_LINUX__
			{
				/* Location code currently supported in AIX only */
				FILE *tmp_fp;
				char tmp_str[200], loc_code1[50], loc_code2[100];
				sprintf(msg, "cat /tmp/sctProcMap | awk 'NR==%d'", ((cptr->bcpu)+1));
				tmp_fp = popen(msg, "r");
				fgets(tmp_str, 100, tmp_fp);
				sscanf(tmp_str, "%*s %s %s", loc_code1, loc_code2);
				pclose(tmp_fp);
				sprintf(msg, "%s miscompare detected. Testcase data dumped in: /tmp/%s_miscompare.%d.%x \
					    \nNum_oper = %d, Seed = 0x%x, Parent Seed = 0x%x \
						\nFailing Processor details - Logical CPU: %d, Physical CPU: %d, Location Code: %s %s\n",
						miscomp_name, (char *)(dinfo.device_name+5), core_num, cptr->original_seed, \
					    num_oper, cptr->original_seed, original_seed, \
					 	cptr->bcpu, core_num, loc_code1, loc_code2);
				hxfmsg(&hd, 0, HTX_HE_MISCOMPARE, msg);
			}
#else
			sprintf(msg, "%s miscompare detected. Testcase data dumped in: /tmp/%s_miscompare.%d.%x \
					\nNum_oper = %d, Seed = 0x%x, Parent Seed = 0x%x \
					\nFailing Processor details - Logical CPU: %d, Physical CPU: %d\n",
					miscomp_name, (char *)(dinfo.device_name+5), core_num, cptr->original_seed, \
					num_oper, cptr->original_seed, original_seed, \
					cptr->bcpu, core_num);
			hxfmsg(&hd, 0, HTX_HE_MISCOMPARE, msg);
#ifdef AWAN
			printf("Terminating exerciser instance due to Test failure.\n");
			fflush(stdout);
			exit_flag=1;
			pthread_exit(NULL);
#endif

#endif /* __HTX_LINUX__ */
		} else {
		/* Compare SUCCESS part */

		#ifdef AWAN

					/* Below code make thread 0 (T0) update stats file with executed instructions count after every
					   10 passes. Using volatile for both conditions in 'if' statement to turn off compiler optimization
					   as it used make '&&' operator return false even though both conditions were true!!
					 */

						volatile int cond1, cond2;
                        cond1 = (client_no==0);
                        cond2 = ((num_oper %10) == 0);
                        if(cond1 && cond2) {
                                insts_sim_thread = insts_sim + ((num_oper+1) * rule.stream_depth * rule.num_threads * 2);
                                rewind(fp_sim);
                                fprintf(fp_sim, "%llu\n", (unsigned long long) (insts_sim_thread / 1024));
                                fflush(fp_sim);
                        }

                        /* Below code makes sure that each thread updates completed PASS count after every 100 passes.
                           Like for SMT2 T0 will update 100, T1 200 and then T0 updates 300 as PASS count, making
                           sure that screen in not flooded with PASS count prints.
                        */
                        if((num_oper + 1) % ((100*(client_no+1))+ (100*a)) == 0) {
                                printf("T%d: PASSES Completed: %d\n", client_no, (num_oper + 1));
                                fflush(stdout);
                                a += rule.num_threads;
                        }
		#else
			#ifndef SCTU
			if(rule.dump_testcase[client_no]) {
				if(shifted_pvr_os <= 0x3e) {
					dump_testcase_p6(client_no, num_oper, rc, miscomparing_num);
				}
				else {
					dump_testcase_p7(client_no, num_oper, rc, miscomparing_num);
				}
#ifndef __HTX_LINUX__
				{
					/* Location code currently supported in AIX only */
					FILE *tmp_fp;
					char tmp_str[200], loc_code1[50], loc_code2[100];
					sprintf(msg, "cat /tmp/sctProcMap | awk 'NR==%d'", ((cptr->bcpu)+1));
					tmp_fp = popen(msg, "r");
					fgets(tmp_str, 100, tmp_fp);
					sscanf(tmp_str, "%*s %s %s", loc_code1, loc_code2);
					pclose(tmp_fp);
					sprintf(msg, "Log dumped.\nProcessor detail - Logical Processor : %d, Physical Core : %d, Location Code : %s %s \
								 \nLog file created : /tmp/%s_miscompare.%d.%x\n", cptr->bcpu,core_num,loc_code1,loc_code2,(char *)(dinfo.device_name+5), core_num,cptr->original_seed);
				}
#else /* __HTX_LINUX__ */
				sprintf(msg, "Log dumped.\n Processor details - Logical Processor : %d, Physical Core : %d\
							  \nLog file created : /tmp/%s_miscompare.%d.%x\n", cptr->bcpu, core_num, (char *)(dinfo.device_name+5), core_num, cptr->original_seed);
#endif /* __HTX_LINUX__ */
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);
			}
			#endif /* end ifndef SCTU */
		#endif
		}

#ifdef SCTU
		pthread_mutex_lock(&rerunpp_ready_count_lock);
		threads_ready_for_rerunpp++;

		if( threads_ready_for_rerunpp == rule.num_threads ) {

			if( pp_cnt == 255 ){
				dump_testcase_sctu(client_no, num_oper, cptr->miscompare_in_numoper, miscomparing_num);
			}

			/* Reset counters*/
			threads_ready_for_pass2=0;
			threads_ready_for_rerunpp=0;

			for( i=0; i<rule.num_threads; i++ ){
				miscompare_in_numoper |= cptr->miscompare_in_numoper;
			}

			if( miscompare_in_numoper || pp_cnt == 255 )	{
				/* Reset counters*/
				threads_built_testcase=0;
				/*miscompare_in_numoper = 0;*/
			}

			if( pp_cnt == 255 ){
				/* restore original priority now as testcase execution is complete */
#ifdef __HTX_LINUX__
				errno = 0;
				rc = setpriority ( PRIO_PROCESS, 0, orig_priority );
				if ( rc == -1 && errno ) {
					sprintf(msg, "setpri (at testcase execn end) call failed with errno = %d (%s) \n", errno, strerror(errno));
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					exit_flag = 1;
				}
#else
				/* Renice back. */
				errno = 0;
				rc = nice(+10);
				if ( rc == -1 ) {
					sprintf(msg, "nice 10 failed. errno = %d\n", errno);
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					exit_flag = 1;
				}
#endif
			}

			if( exit_flag ) {
				sctu_exit_flag = 1;
			}

			for( i=0; i<rule.num_threads; i++ ){
				sem_post(&rerun_pp_sem);
			}
		}
		pthread_mutex_unlock(&rerunpp_ready_count_lock);

		sem_wait(&rerun_pp_sem);

		/* make all threads exit for sctu */
		if( sctu_exit_flag ){
			pthread_exit(NULL);
		}

		/* exit out of pp iterations if miscompare found and */
		/* move to next num oper */
		if( miscompare_in_numoper ) {
			break;
		}

		} /***************** end of pp_cnt for loop **********************************/



		cptr = sdata->cdata_ptr[client_no];
		cptr->num_ins_built = 0;
		for(i = 0; i < 3; i++) {
			cptr->ls_base_volatile[i] = cptr->ls_base[i];
			cptr->ls_current[i] =  cptr->ls_cl_start[i] = cptr->ls_base[i] + client_no * CLIENT_NUM_BYTES_IN_CL;
		}
		cptr->miscompare_in_numoper = 0;

#else

		cptr = sdata->cdata_ptr[client_no];
		cptr->num_ins_built = 0;
		for(i = 0; i < 3; i++) {
			cptr->ls_current[i] = cptr->ls_base[i] = cptr->tc_ptr[i]->ls_ptr;
			cptr->ls_base_volatile[i] = cptr->ls_base[i];
		}
		if (rule.testcase_sync) {
			#if 1
            pthread_mutex_lock(&clients_sync_lock);
            if (clients_sync_count < rule.num_threads) {
            	clients_sync_count++;
			}
            if (clients_sync_count == rule.num_threads) {
                clients_sync_count = 0;
				sync_timed_out = 0;
                pthread_cond_broadcast(&clients_sync_cond);
            }
            else {
				/* wait for max 10 sec for  thread_count to become rule.num_threads */
                struct timespec ts_now;
                if (clock_gettime(CLOCK_REALTIME, &ts_now) == -1) {
                    perror( "clock gettime" );
                }
                ts_now.tv_sec += 180;
               	rc = pthread_cond_timedwait(&clients_sync_cond, &clients_sync_lock, &ts_now);
				if (rc && (errno == ETIMEDOUT)) {
					sync_timed_out = 1;
				}
               	/*pthread_cond_wait(&thread_count_cond, &thread_count_lock);*/
            }
            pthread_mutex_unlock(&clients_sync_lock);

            if (exit_flag || sync_timed_out) {
                sprintf(msg, "Client %d, exiting, thread_count = %d, timed_out: %d, errno: %s\n", client_no, thread_count, sync_timed_out, strerror(errno));
                hxfmsg(&hd, 0, HTX_HE_INFO, msg);
                for(i = 0; i < rule.num_threads; i++) {
                    if (i == client_no) continue;
                    pthread_kill(global_sdata[INITIAL_BUF].cdata_ptr[client_no]->tid, SIGKILL);
                    /*pthread_cancel(global_sdata[INITIAL_BUF].cdata_ptr[client_no]->tid);*/
                }
                pthread_exit(NULL);
            }
			#endif
		}
#endif
		/*************one test cycle completed************************/
		num_oper++;
		/*-----------------------------------------------------------*/
		if(rule.parent_seed == 0) {
			if (rule.seed[client_no] == 0) {
				seed_again = get_random_no_32(client_no);
				set_seed(client_no, seed_again);
				init_random_no_generator(client_no);
			}
			else {
				set_seed(client_no, rule.seed[client_no]);
				init_random_no_generator(client_no);
			}
		}
		else {
			if (cptr->original_seed == rule.seed[client_no]) {
				break;
			}
			else {
				seed_again = get_random_no_32(client_no);
				set_seed(client_no, seed_again);
				init_random_no_generator(client_no);
			}
		}
	} /* End of num_oper loop */

	cptr->num_oper = num_oper;
	cptr->num_pass2 = num_pass2;

	#ifndef __HTX_LINUX__
	tick_end = read_tb();
	#else
	read_tb(tick_end);
	#endif
}

void dump_testcase_p7(int cno, int num_oper, int type, int num)
{
	struct testcase *tc1, *tc2, *tci;
	uint64 *ptr1, *ptr2, *ptri, vsr1_h, vsr1_l, vsr2_h, vsr2_l /*temp_vsr1, temp_vsr2*/;
	/*uint32 *insti, *offseti, *offset1, *offset2;*/
	uint32 seed, miscom_index = 0, type_list, ins_cat;
	char  *basei, *base1, *base2;
	struct vsr_node *list_h, *list_t, *tmp_vsrs_list;
	int i, j/*, total_ins*/;
	int thread_start_index, thread_end_index;
	char dump_file[50], temp_val1[35], temp_val2[35], host_name[50];
	FILE *dump;
	time_t timer;
	struct tm current_time;
	struct client_data *tmp_cptr, *cptr = global_sdata[INITIAL_BUF].cdata_ptr[cno];
#ifdef SCTU
	uint64 max_ls_off;
#endif

	seed = global_sdata[INITIAL_BUF].cdata_ptr[cno]->original_seed;
	tci = global_sdata[INITIAL_BUF].cdata_ptr[cno]->tc_ptr[INITIAL_BUF];
	tc1 = global_sdata[PASS1_BUF].cdata_ptr[cno]->tc_ptr[PASS1_BUF];
	tc2 = global_sdata[PASS2_BUF].cdata_ptr[cno]->tc_ptr[PASS2_BUF];
	ptri = (uint64 *)tci->tcc.vsrs;
	ptr1 = (uint64 *)tc1->tcc.vsrs;
	ptr2 = (uint64 *)tc2->tcc.vsrs;

	basei = global_sdata[INITIAL_BUF].cdata_ptr[cno]->ls_base[INITIAL_BUF];
	base1 = global_sdata[PASS1_BUF].cdata_ptr[cno]->ls_base[PASS1_BUF];
	base2 = global_sdata[PASS2_BUF].cdata_ptr[cno]->ls_base[PASS2_BUF];
#ifdef AWAN
	dump = stdout;
#else
	sprintf(dump_file, "/tmp/%s_miscompare.%d.%x", (char *)(dinfo.device_name+5), core_num, seed);
	dump = fopen((const char *)dump_file,"w");
	if(dump == NULL) {
		sprintf(msg, "Error opening dump file %s, errno: 0x%X, %s\n", dump_file, errno, strerror(errno));
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		exit(1);
	}
#endif

	get_hostname(host_name);
	fprintf(dump, "################################################################################\n");
	fprintf(dump, "#                                                                              #\n");
	fprintf(dump, "#                         hxefpu64 Exerciser Dump File                         #\n");
	fprintf(dump, "#                                                                              #\n");
	fprintf(dump, "################################################################################\n\n");
	fprintf(dump, "===========================Common Configuration Data============================\n");
#ifndef __HTX_LINUX__
	fprintf(dump, "Total Logical CPUs : %d\n", _system_configuration.ncpus);
#else
#endif

	fprintf(dump, "Host name          : %s\n", host_name);

#ifdef __HTX_LINUX__
	fprintf(dump, "Operating System   : Linux\n");
#else
	fprintf(dump, "Operating System   : AIX\n");
#endif
	fprintf(dump, "CPU PVR value      : 0x%08x\n", pvr);
	fprintf(dump, "Failing PID        : %ull\n", getpid());
	fprintf(dump, "Failing CPU number : %d\n", cptr->bcpu);
	fprintf(dump, "Failing Thread no  : %d\n", cno);
	fprintf(dump, "CPUs of this gang  : ");
	for ( i = 0; i < rule.num_threads; i++ ) {
		tmp_cptr = global_sdata[INITIAL_BUF].cdata_ptr[i];
		fprintf(dump,"%d ", tmp_cptr->bcpu);
	}
	fprintf(dump, "\n");
	fprintf(dump, "Failing/Total loop : %d/%d\n", num_oper+1, rule.num_oper);
	fprintf(dump, "Parent Seed        : 0x%x\n", original_seed);
	fprintf(dump, "Failing Seed       : 0x%x\n", seed);
	fprintf(dump, "Rule File path     : %s\n",dinfo.rule_file_name);
	fprintf(dump, "Stanza name        : %s\n", rule.rule_id);
	if(rule.test_method == CORRECTNESS_CHECK) {
		fprintf(dump, "Test Method        : CORRECTNESS CHECK\n");
	}
	else {
		fprintf(dump, "Test Method        : CONSISTENCY CHECK\n");
	}
	fprintf(dump, "Input FPSCR        :\n");
	for ( i = 0; i < rule.num_threads; i++ ) {
		fprintf(dump, "                     Thread[%d] - 0x%016llx\n", i, rule.fpscr[i]);
	}

	timer = time(NULL);
	localtime_r(&timer, &current_time);
	asctime_r(&current_time, temp_val1);
	fprintf(dump, "File creation time : %s\n\n", temp_val1);
	fprintf(dump, "==========================Miscompare Summary===================================\n");
	fflush(dump);
	switch (type) {
		case 1:
			fprintf(dump, "Miscompare Type    : VSR Miscompare.\n");
			fprintf(dump, "Mismatching VSR no : %d\n", num);
			vsr1_h = *(ptr1+(num*2)); vsr1_l = *(ptr1+(num*2)+1);
			vsr2_h = *(ptr2+(num*2)); vsr2_l = *(ptr2+(num*2)+1);

			fprintf(dump, "                       VSR[%d]\n", num);
			fprintf(dump, "          --------------------------------\n");
			fprintf(dump, "Initial : 0x%016llx%016llx\n", *(ptri+(num*2)), *(ptri+(num*2)+1));
			fprintf(dump, "Pass1   : 0x%016llx%016llx\n", vsr1_h, vsr1_l);
			fprintf(dump, "Pass2   : 0x%016llx%016llx\n", vsr2_h, vsr2_l);
			for ( i = 0; i < 16; i++) {
				temp_val1[i] = ((vsr1_l & 0xfULL) + '0');
				temp_val1[i+16] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_l & 0xfULL) + '0');
				temp_val2[i+16] = ((vsr2_h & 0xfULL) + '0');
				vsr1_l >>= 4;
				vsr1_h >>= 4;
				vsr2_l >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i+16]='\0';
			temp_val2[i+16]='\0';

			fprintf(dump, "            ");
			for ( i = 31; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 2:
			vsr1_h = tc1->tcc.gprs[num];
			vsr2_h = tc2->tcc.gprs[num];
			fprintf(dump, "Miscompare Type    : GPR Miscompare.\n");
			fprintf(dump, "Mismatching GPR no : %d\n", num);
			fprintf(dump, "              GPR[%d]\n", num);
			fprintf(dump, "          ----------------\n");
			fprintf(dump, "Initial : 0x%016llx\n", tci->tcc.gprs[num]);
			fprintf(dump, "Pass1   : 0x%016llx\n", vsr1_h);
			fprintf(dump, "Pass2   : 0x%016llx\n", vsr2_h);
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "            ");
			for ( i = 15; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 3:
			vsr1_h = tc1->tcc.sprs.fpscr;
			vsr2_h = tc2->tcc.sprs.fpscr;
			fprintf(dump, "Miscompare Type    : FPSCR Miscompare.\n");
			fprintf(dump, "                FPSCR\n");
			fprintf(dump, "          ------------------\n");
			fprintf(dump, "Initial : 0x%016llx\n",tci->tcc.sprs.fpscr);
			fprintf(dump, "Pass1   : 0x%016llx\n",vsr1_h);
			fprintf(dump, "Pass2   : 0x%016llx\n",vsr2_h);
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "            ");
			for ( i = 15; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 4:
			fprintf(dump, "Miscompare Type    : VSCR Miscompare.\n");
			fprintf(dump, "                        VSCR\n");
			fprintf(dump, "          ----------------------------------\n");
			fprintf(dump, "Initial : 0x%016llx%016llx\n",tci->tcc.sprs.vscr[0], tci->tcc.sprs.vscr[1]);
			fprintf(dump, "Pass1   : 0x%016llx%016llx\n",tc1->tcc.sprs.vscr[0], tc1->tcc.sprs.vscr[1]);
			fprintf(dump, "Pass2   : 0x%016llx%016llx\n",tc2->tcc.sprs.vscr[0], tc2->tcc.sprs.vscr[1]);
			vsr1_h = tc1->tcc.sprs.vscr[0];
			vsr2_h = tc2->tcc.sprs.vscr[0];
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			vsr1_h = tc1->tcc.sprs.vscr[1];
			vsr2_h = tc2->tcc.sprs.vscr[1];
			for ( i = 16; i < 32; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "            ");
			for ( i = 31; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 5:
			vsr1_h = tc1->tcc.sprs.cr;
			vsr2_h = tc2->tcc.sprs.cr;
			fprintf(dump, "Miscompare Type    : CR Miscompare.\n");
			fprintf(dump, "                 CR\n");
			fprintf(dump, "          ------------------\n");
			fprintf(dump, "Initial : 0x%016llx\n",tci->tcc.sprs.cr);
			fprintf(dump, "Pass1   : 0x%016llx\n",vsr1_h);
			fprintf(dump, "Pass2   : 0x%016llx\n",vsr2_h);
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "            ");
			for ( i = 15; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 6:
			vsr1_h=*(uint64 *)(base1+num);
			vsr2_h=*(uint64 *)(base2+num);
			fprintf(dump, "Miscompare Type    : Load/Store Area Miscompare.\n");
			fprintf(dump, "Mismatching Offset : 0x%x bytes\n", num);
			fprintf(dump, "Mismatching Address: 0x%llx\n", (uint64)(base1 + num));
			fprintf(dump, "            0x%-16llx 0x%-16llx 0x%-16llx\n", (uint64)(base1 + num - 8),
					(uint64)(base1 + num), (uint64)(base1 + num + 8));
			fprintf(dump, "          ------------------ ------------------ ------------------\n");
			fprintf(dump, "Initial : 0x%016llx 0x%016llx 0x%016llx\n", *((uint64 *)(basei+num-8)),
 					*((uint64 *)(basei+num)), *((uint64 *)(basei+num+8)));
			fprintf(dump, "Pass1   : 0x%016llx 0x%016llx 0x%016llx\n", *((uint64 *)(base1+num-8)),
 					vsr1_h, *((uint64 *)(base1+num+8)));
			fprintf(dump, "Pass2   : 0x%016llx 0x%016llx 0x%016llx\n", *((uint64 *)(base2+num-8)),
 					vsr2_h, *((uint64 *)(base2+num+8)));
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "                               ");
			for ( i = 15; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;
	}
	fflush(dump);

	fprintf(dump, "==========================Register Allocation===================================\n");

	for(type_list = SCALAR_SP; type_list <= BFP_QP; type_list++) {
		fprintf(dump, "\n Data Type: %d -->", type_list);
		for(ins_cat = VSX; ins_cat <= VMX; ins_cat++) {
			list_h = cptr->vsrs[type_list].head[ins_cat];
			list_t = cptr->vsrs[type_list].tail[ins_cat];
			fprintf(dump, "\n Type: %d -->", ins_cat);
			tmp_vsrs_list = list_h;
			while( tmp_vsrs_list != list_t ) {
				fprintf(dump, " %d",tmp_vsrs_list->vsr_no);
				tmp_vsrs_list = tmp_vsrs_list->next;
			}
			if ((tmp_vsrs_list != NULL) && (tmp_vsrs_list->vsr_no != 0)) {
				fprintf(dump, " %d",tmp_vsrs_list->vsr_no);
			}
		}
	}
	fprintf(dump, "\n\n");


	fprintf(dump, "\n==========================Detailed Dumps per thread============================");

	if ( rule.testcase_sync ) {
		thread_start_index = 0;
		thread_end_index = rule.num_threads;
	}
	else {
		thread_start_index = cno;
		thread_end_index = cno+1;
	}

	for(j = thread_start_index; j < thread_end_index; j++) {

		bzero(global_sdata[INITIAL_BUF].cdata_ptr[j]->ls_miscom_off_list, sizeof(global_sdata[INITIAL_BUF].cdata_ptr[j]->ls_miscom_off_list));
		fprintf(dump, "\n\n<################################# Thread %d ##################################>\n", j);
		tci = global_sdata[INITIAL_BUF].cdata_ptr[j]->tc_ptr[INITIAL_BUF];
		tc1 = global_sdata[PASS1_BUF].cdata_ptr[j]->tc_ptr[PASS1_BUF];
		tc2 = global_sdata[PASS2_BUF].cdata_ptr[j]->tc_ptr[PASS2_BUF];
		ptri = (uint64 *)&tci->tcc.vsrs;
		ptr1 = (uint64 *)&tc1->tcc.vsrs;
		ptr2 = (uint64 *)&tc2->tcc.vsrs;
		fprintf(dump, "\nVSRs[thread#%d]:\n", j);
		fprintf(dump, "---------------\n");
		fprintf(dump, "                   INITIAL                             PASS1                               PASS2\n");
		fprintf(dump, "       ---------------------------------- ---------------------------------- ----------------------------------\n");

		for(i = 0; i < NUM_VSRS; i++) {
			if((*ptr1 != *ptr2) || (*(ptr1+1)) != (*(ptr2+1))) {
				fprintf(dump, "[%02d]:  0x%016llx%016llx 0x%016llx%016llx 0x%016llx%016llx <<\n", i, *ptri, *(ptri+1),
					*ptr1, *(ptr1+1), *ptr2, *(ptr2+1));
			}
			else {
				fprintf(dump, "[%02d]:  0x%016llx%016llx 0x%016llx%016llx 0x%016llx%016llx\n", i, *ptri, *(ptri+1),
					*ptr1, *(ptr1+1), *ptr2, *(ptr2+1));
			}
			ptri++; ptr1++; ptr2++;
		}

		ptri = (uint64 *)&tci->tcc.gprs;
		ptr1 = (uint64 *)&tc1->tcc.gprs;
		ptr2 = (uint64 *)&tc2->tcc.gprs;
		fprintf(dump, "\nGPRs[thread#%d]:\n", j);
		fprintf(dump, "---------------\n");
		fprintf(dump, "           INITIAL              PASS1              PASS2\n");
		fprintf(dump, "      ------------------ ------------------ ------------------\n");
		for(i = 0; i < NUM_GPRS; i++) {
			fprintf(dump, "[%02d]: 0x%016llx 0x%016llx 0x%016llx\n", i, *(ptri++), *(ptr1++), *(ptr2++));
		}
		fprintf(dump, "\nFPSCR[thread#%d]:\n", j);
		fprintf(dump, "----------------\n");
		fprintf(dump, "            INITIAL              PASS1              PASS2\n");
		fprintf(dump, "        ------------------ ------------------ ------------------\n");
		fprintf(dump, "FPSCR : 0x%016llx 0x%016llx 0x%016llx\n", tci->tcc.sprs.fpscr,
				tc1->tcc.sprs.fpscr, tc2->tcc.sprs.fpscr);

		fprintf(dump, "\nVSCR[thread#%d]:\n", j);
		fprintf(dump, "--------------\n");
		fprintf(dump, "                    INITIAL                              PASS1                              PASS2\n");
		fprintf(dump, "        ---------------------------------- ---------------------------------- ----------------------------------\n");
		fprintf(dump, "VSCR : 0x%016llx%016llx 0x%016llx%016llx 0x%016llx%016llx\n", 
				tci->tcc.sprs.vscr[0], tci->tcc.sprs.vscr[1],
				tc1->tcc.sprs.vscr[0], tc1->tcc.sprs.vscr[1],
				tc2->tcc.sprs.vscr[0], tc2->tcc.sprs.vscr[1]);

		fprintf(dump, "\nCR[thread#%d]:\n", j);
		fprintf(dump, "-------------\n");
		fprintf(dump, "         INITIAL              PASS1              PASS2\n");
		fprintf(dump, "     ------------------ ------------------ ------------------\n");
		fprintf(dump, "CR : 0x%016llx 0x%016llx 0x%016llx\n", tci->tcc.sprs.cr,
				tc1->tcc.sprs.cr, tc2->tcc.sprs.cr);



		/*ptri = (uint64 *)tci->ls_ptr; ptr1 = (uint64 *)tc1->ls_ptr; ptr2 = (uint64 *)tc2->ls_ptr;*/
		ptri = (uint64 *)basei; ptr1 = (uint64 *)base1; ptr2 = (uint64 *)base2;
		fprintf(dump, "\nLoad/Store Memory region[thread#%d]:\n", j);
		fprintf(dump, "-----------------------------------\n");
		fprintf(dump, " OFFSET          INITIAL               PASS1                  PASS2\n");
		fprintf(dump, "           [0x%016llx]  [0x%016llx]  [0x%016llx]\n", (uint64)ptri, (uint64)ptr1, (uint64)ptr2);

		fprintf(dump, "--------   ------------------   ------------------   ------------------\n");
#ifdef AWAN
		for(i = 0; i < 1000; i += 8) {
			if((*ptr1 != *ptr2)) {
				fprintf(dump, "[0x%04lx] : 0x%016llx   0x%016llx   0x%016llx <<\n", i, *ptri++, *ptr1++, *ptr2++);
				global_sdata[PASS1_BUF].cdata_ptr[cno]->ls_miscom_off_list[miscom_index++] = i;
			}
			else {
				fprintf(dump, "[0x%04lx] : 0x%016llx   0x%016llx   0x%016llx \n", i, *ptri++, *ptr1++, *ptr2++);
			}
		}
#else
	#ifdef SCTU
		max_ls_off = global_sdata[PASS1_BUF].cdata_ptr[0]->last_ls_off;
		for( i=1; i<rule.num_threads; i++){
			if( max_ls_off < global_sdata[PASS1_BUF].cdata_ptr[i]->last_ls_off ){
				max_ls_off = global_sdata[PASS1_BUF].cdata_ptr[i]->last_ls_off;
			}
		}
		for(i = 0; i < (max_ls_off + 8); i += 8) {
	#else
		for(i = 0; i < (global_sdata[PASS1_BUF].cdata_ptr[cno]->last_ls_off + 8); i += 8) {
	#endif
			if((*ptr1 != *ptr2)) {
				fprintf(dump, "[0x%04x] : 0x%016llx   0x%016llx   0x%016llx <<\n", i, *ptri++, *ptr1++, *ptr2++);
				global_sdata[PASS1_BUF].cdata_ptr[cno]->ls_miscom_off_list[miscom_index++] = i;
			}
			else {
				fprintf(dump, "[0x%04x] : 0x%016llx   0x%016llx   0x%016llx \n", i, *ptri++, *ptr1++, *ptr2++);
			}
		}
#endif
		dump_instructions_p7(j, dump);
		/* Append miscompare analysis into dump file */
		if (type) {
			fprintf(dump, "==========================Miscompare Analysis===================================\n");
		}
		switch (type) {
        	case 1:
            	fprintf(dump, "Miscompare Type    : VSR Miscompare.\n");
            	fprintf(dump, "Mismatching VSR no : %d\n", num);
				cptr->vsr_usage.head = create_instr_tree(dump, j, num, VSR_DTYPE, -1);
				delete_reg_use_list(cptr->vsr_usage.head);
				break;
        	case 2:
				fprintf(dump, "Miscompare Type    : GPR Miscompare.\n");
				fprintf(dump, "Mismatching GPR no : %d\n", num);
				cptr->gpr_usage.head = create_instr_tree(dump, j, num, GPR_DTYPE, -1);
				delete_reg_use_list(cptr->gpr_usage.head);
				break;
        	case 6:
				fprintf(dump, "Miscompare Type    : Load/Store Area Miscompare.\n");
				fprintf(dump, "Mismatching Offset : 0x%x bytes\n", num);
				cptr->ls_usage.head = prepare_ls_call_list(dump, j, num);
				delete_reg_use_list(cptr->ls_usage.head);
				break;
		}
		fflush(dump);
	}
	fflush(dump);
#ifdef AWAN
	exit(-1);
#endif
	fclose(dump);
}

void dump_instructions_p7(int cno, FILE *df)
{
	uint32 *instr, *offset, *initbase_offset;
	client_data *cptr;
	int prolog_ins, stream_ins, epilog_ins, i;

	instr = (uint32 *)global_sdata[INITIAL_BUF].cdata_ptr[cno]->tc_ptr[INITIAL_BUF]->tc_ins;
	offset = global_sdata[INITIAL_BUF].cdata_ptr[cno]->tc_ptr[INITIAL_BUF]->ea_off;
	initbase_offset = global_sdata[INITIAL_BUF].cdata_ptr[cno]->tc_ptr[INITIAL_BUF]->ea_initbase_off;
	cptr = global_sdata[INITIAL_BUF].cdata_ptr[cno];

	prolog_ins = global_sdata[INITIAL_BUF].cdata_ptr[cno]->prolog_size;
	epilog_ins = global_sdata[INITIAL_BUF].cdata_ptr[cno]->epilog_size;
	stream_ins = global_sdata[INITIAL_BUF].cdata_ptr[cno]->num_ins_built;

    fprintf(df, "\nInstruction List[thread#%d]:\n", cno);
    fprintf(df, "---------------------------\n");
#ifndef AWAN
    fprintf(df, "----Prolog starts here-----\n");
	for ( i = 0; i < prolog_ins; i++ ) {
		fprintf(df, "0x%08x  0x%04x\n", *instr++, *offset++);
		initbase_offset++;
	}
    fprintf(df, "------Prolog ends here-----\n");
#endif

    fprintf(df, "-Random instruction stream-\n");
	decode_tc_instructions(cno);
	for (i = 0; i < stream_ins; i++) {
		fprintf(df,  "%-4d:0x%08x  0x%04x \t 0x%04x \t\t%s\n", (i + 1), *instr++, *offset++, *initbase_offset++, cptr->dc_instr[i].mnemonic);
	}
   	fprintf(df, "--Random instruction ends--\n");

#ifndef AWAN
    fprintf(df, "----Epilog startes here----\n");
	for ( i = (prolog_ins + stream_ins); i < (prolog_ins + stream_ins + epilog_ins); i++ ) {
		fprintf(df, "0x%08x  0x%04x\n", *instr++, *offset++);
	}
    fprintf(df, "------Epilog ends here-----\n");
#endif
}

void dump_instructions_p6(int cno, FILE *df)
{
	uint32 *instr, *offset;
	instruction_masks *ins_tuple;
	client_data *cptr;
	int prolog_ins, stream_ins, epilog_ins, i;
	char mnemonic[30];

	instr = global_sdata[INITIAL_BUF].cdata_ptr[cno]->tc_ptr[INITIAL_BUF]->tc_ins;
	offset = global_sdata[INITIAL_BUF].cdata_ptr[cno]->tc_ptr[INITIAL_BUF]->ea_off;
	cptr = global_sdata[INITIAL_BUF].cdata_ptr[cno];

	prolog_ins = global_sdata[INITIAL_BUF].cdata_ptr[cno]->prolog_size;
	epilog_ins = global_sdata[INITIAL_BUF].cdata_ptr[cno]->epilog_size;
	stream_ins = global_sdata[INITIAL_BUF].cdata_ptr[cno]->num_ins_built;

    fprintf(df, "\nInstruction List[thread#%d]:\n", cno);
    fprintf(df, "---------------------------\n");

    fprintf(df, "----Prolog starts here-----\n");
	for ( i = 0; i < prolog_ins; i++ ) {
		fprintf(df, "0x%08x  0x%04x\n", *instr++, *offset++);
	}
    fprintf(df, "------Prolog ends here-----\n");

    fprintf(df, "-Random instruction stream-\n");
	for ( i = prolog_ins; i < (prolog_ins + stream_ins); i++ ) {
		if ( cptr->instr_index[i] & 0x10000000 ) {
			ins_tuple = &(cptr->enabled_ins_table[(cptr->instr_index[i] & ~(0x10000000))].instr_table);

			switch ( ins_tuple->insfrm ) {
				case D_FORM_RT_RA_D:
				{
					d_form_rt_ra_d *ptr;
					ptr = (d_form_rt_ra_d *)instr;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					if ( ins_tuple->tgt_dtype != DUMMY ) {
						sprintf(mnemonic, "%sf%d", mnemonic, ptr->rt);
					}
					if ( ins_tuple->op1_dtype != DUMMY ) {
						sprintf(mnemonic,"%s,0x%x(r%d)", mnemonic, ptr->d, ptr->ra);
					}
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case X_FORM_RT_RA_RB_eop_rc:
				{
					x_form_rt_ra_rb_eop_rc *ptr;
					ptr = (x_form_rt_ra_rb_eop_rc *)instr;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					if ( ins_tuple->tgt_dtype != DUMMY ) {
						sprintf(mnemonic, "%sf%d", mnemonic, ptr->rt);
					}
					else {
						sprintf(mnemonic, "%s%d", mnemonic, ptr->rt);
					}

					if ( ins_tuple->op1_dtype != DUMMY ) {
						if ( ins_tuple->op1_dtype == GRU || ins_tuple->op1_dtype == GR ) {
							sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
						}
						else {
							sprintf(mnemonic, "%s,f%d", mnemonic, ptr->ra);
						}
					}
					if ( ins_tuple->op2_dtype != DUMMY ) {
						if ( ins_tuple->op2_dtype == GRU || ins_tuple->op2_dtype == GR ) {
							sprintf(mnemonic, "%s,r%d", mnemonic, ptr->rb);
						}
						else {
							sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);
						}
					}
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case X_FORM_BF_RA_RB_eop_rc:
				{
					x_form_bf_ra_rb_eop_rc *ptr;
					ptr = (x_form_bf_ra_rb_eop_rc *)instr;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->bf);
					if ( ins_tuple->op1_dtype != DUMMY ) {
						sprintf(mnemonic, "%s,f%d", mnemonic, ptr->ra);
					}
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case X_FORM_BF_BFA_eop_rc:
				{
					x_form_bf_bfa_eop_rc *ptr;
					ptr = (x_form_bf_bfa_eop_rc *)instr;
					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->bf);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->bfa);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case X_FORM_BF_W_U_eop_rc:
				{
					x_form_bf_w_u_eop_rc *ptr;
					ptr = (x_form_bf_w_u_eop_rc *)instr;
					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->bf);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->u);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->w);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case XFL_FORM_L_FLM_W_RB_eop_rc:
				{
					xfl_form_l_flm_w_rb_eop_rc *ptr;
					ptr = (xfl_form_l_flm_w_rb_eop_rc *)instr;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
					sprintf(mnemonic, "%s%d", mnemonic, ptr->flm);
					sprintf(mnemonic, "%s,f%d", mnemonic, ptr->rb);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->l);
					sprintf(mnemonic, "%s,%d", mnemonic, ptr->w);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case A_FORM_RT_RA_RB_RC_eop_rc:
				{
					a_form_rt_ra_rb_rc_eop_rc *ptr;
					ptr = (a_form_rt_ra_rb_rc_eop_rc *)instr;

					sprintf(mnemonic, "%s ", ins_tuple->ins_name);
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

					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}
				default:
					fprintf(df,  "0x%08x  0x%04x\n", *instr++, *offset++);
			}
		}
		else if ( cptr->instr_index[i] & 0x20000000 ) {
			switch ( (cptr->instr_index[i] & ~(0x20000000)) ) {
				case addi:
				{
					d_form_rt_ra_d *ptr;
					ptr = (d_form_rt_ra_d *)instr;

					sprintf(mnemonic, "%s ", "addi");
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->rt);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->d);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case addis:
				{
					d_form_rt_ra_d *ptr;
					ptr = (d_form_rt_ra_d *)instr;

					sprintf(mnemonic, "%s ", "addis");
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->rt);
					sprintf(mnemonic, "%s,r%d", mnemonic, ptr->ra);
					sprintf(mnemonic, "%s,0x%x", mnemonic, ptr->d);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case mfcr:
				{
					xfx_form_rt_fxm_eop_rc *ptr;
					ptr = (xfx_form_rt_fxm_eop_rc *)instr;

					sprintf(mnemonic, "%s ", "mfcr");
					sprintf(mnemonic, "%sr%d", mnemonic, ptr->rt);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case stwx:
				{
					x_form_rt_ra_rb_eop_rc *ptr;
					ptr = (x_form_rt_ra_rb_eop_rc *)instr;

					sprintf(mnemonic, "%s ", "stwx");
					sprintf(mnemonic, "%sr%d,r%d,r%d",mnemonic,  ptr->rt, ptr->ra, ptr->rb);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case or:
				{
					x_form_rt_ra_rb_eop_rc *ptr;
					ptr = (x_form_rt_ra_rb_eop_rc *)instr;

					sprintf(mnemonic, "%s ", "or");
					sprintf(mnemonic, "%sr%d,r%d,r%d", mnemonic, ptr->ra, ptr->rt, ptr->rb);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case ori:
				{
					d_form_rt_ra_d *ptr;
					ptr = (d_form_rt_ra_d *)instr;

					sprintf(mnemonic, "%s ", "ori");
					sprintf(mnemonic, "%sr%d,r%d,0x%x",  mnemonic,ptr->ra, ptr->rt, ptr->d);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case and:
				{
					x_form_rt_ra_rb_eop_rc *ptr;
					ptr = (x_form_rt_ra_rb_eop_rc *)instr;

					sprintf(mnemonic, "%s ", "and");
					sprintf(mnemonic, "%sr%d,r%d,r%d", mnemonic, ptr->ra, ptr->rt, ptr->rb);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				case stfd:
				{
					d_form_rt_ra_d *ptr;
					ptr = (d_form_rt_ra_d *)instr;

					sprintf(mnemonic, "%s ", "stfd");
					sprintf(mnemonic, "%sf%d,0x%x(r%d)", mnemonic, ptr->rt, ptr->d, ptr->ra);
					fprintf(df,  "0x%08x  0x%04x\t\t%s\n", *instr++, *offset++, mnemonic);

					break;
				}

				default :
					fprintf(df,  "0x%08x  0x%04x\n", *instr++, *offset++);
			}
		}
	}
    fprintf(df, "--Random instruction ends--\n");

    fprintf(df, "----Epilog startes here----\n");
	for ( i = (prolog_ins + stream_ins); i < (prolog_ins + stream_ins + epilog_ins); i++ ) {
		fprintf(df, "0x%08x  0x%04x\n", *instr++, *offset++);
	}
    fprintf(df, "------Epilog ends here-----\n");
}
void dump_testcase_p6(int cno, int num_oper, int type, int num)
{
	struct testcase *tc1, *tc2, *tci;
	uint64 *ptr1, *ptr2, *ptri, vsr1_h, vsr2_h; 
	uint32 seed, type_list, ins_cat;
	char *basei, *base1, *base2;
	struct vsr_node *list_h, *list_t, *tmp_vsrs_list;
	int i, j;
	int thread_start_index, thread_end_index;
	char dump_file[25], temp_val1[35], temp_val2[35];
	FILE *dump;
	time_t timer;
	struct tm current_time;
	struct client_data *tmp_cptr, *cptr = global_sdata[INITIAL_BUF].cdata_ptr[cno];


	seed = global_sdata[INITIAL_BUF].cdata_ptr[cno]->original_seed;
	tci = global_sdata[INITIAL_BUF].cdata_ptr[cno]->tc_ptr[INITIAL_BUF];
	tc1 = global_sdata[PASS1_BUF].cdata_ptr[cno]->tc_ptr[PASS1_BUF];
	tc2 = global_sdata[PASS2_BUF].cdata_ptr[cno]->tc_ptr[PASS2_BUF];
	ptri = (uint64 *)tci->tcc.vsrs;
	ptr1 = (uint64 *)tc1->tcc.vsrs;
	ptr2 = (uint64 *)tc2->tcc.vsrs;
	basei = global_sdata[INITIAL_BUF].cdata_ptr[cno]->ls_base[INITIAL_BUF];
	base1 = global_sdata[PASS1_BUF].cdata_ptr[cno]->ls_base[PASS1_BUF];
	base2 = global_sdata[PASS2_BUF].cdata_ptr[cno]->ls_base[PASS2_BUF];

	sprintf(dump_file, "/tmp/%s_miscompare.%d.%x", (char *)(dinfo.device_name+5), core_num, seed);
	dump = fopen((const char *)dump_file,"w");
    if(dump == NULL) {
		sprintf(msg, "Error opening dump file.\n");
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		exit(1);
	}

	fprintf(dump, "################################################################################\n");
	fprintf(dump, "#                                                                              #\n");
	fprintf(dump, "#                         hxefpu64 Exerciser Dump File                         #\n");
	fprintf(dump, "#                                                                              #\n");
	fprintf(dump, "################################################################################\n\n");
	fprintf(dump, "===========================Common Configuration Data============================\n");
#ifndef __HTX_LINUX__
	fprintf(dump, "Total Logical CPUs : %d\n", _system_configuration.ncpus);
#else
#endif

#ifdef __HTX_LINUX__
	fprintf(dump, "Operating System   : Linux\n");
#else
	fprintf(dump, "Operating System   : AIX\n");
#endif
	fprintf(dump, "CPU PVR value      : 0x%08x\n", pvr);
	fprintf(dump, "Failing PID        : %ull\n", getpid());
	fprintf(dump, "Failing CPU number : %d\n", cptr->bcpu);
	fprintf(dump, "Failing Thread no  : %d\n", cno);
	fprintf(dump, "CPUs of this gang  : ");
	for ( i = 0; i < rule.num_threads; i++ ) {
		tmp_cptr = global_sdata[INITIAL_BUF].cdata_ptr[i];
		fprintf(dump,"%d ", tmp_cptr->bcpu);
	}
	fprintf(dump, "\n");
	fprintf(dump, "Failing/Total loop : %d/%d\n", num_oper+1, rule.num_oper);
	fprintf(dump, "Parent Seed        : 0x%x\n", original_seed);
	fprintf(dump, "Seed               : 0x%x\n", seed);
	fprintf(dump, "Rule File path     : %s\n",dinfo.rule_file_name);
	fprintf(dump, "Stanza name        : %s\n", rule.rule_id);
	if(rule.test_method == CORRECTNESS_CHECK) {
		fprintf(dump, "Test Method        : CORRECTNESS CHECK\n");
	}
	else {
		fprintf(dump, "Test Method        : CONSISTENCY CHECK\n");
	}
	fprintf(dump, "Input FPSCR        :\n");
	for ( i = 0; i < rule.num_threads; i++ ) {
		fprintf(dump, "                     Thread[%d] - 0x%016llx\n", i, rule.fpscr[i]);
	}

	timer = time(NULL);
	localtime_r(&timer, &current_time);
	asctime_r(&current_time, temp_val1);
	fprintf(dump, "File creation time : %s\n\n", temp_val1);
	fprintf(dump, "==========================Miscompare Summary===================================\n");
	fflush(dump);
	switch (type) {
		case 1:
			fprintf(dump, "Miscompare Type    : FPR Miscompare.\n");
			fprintf(dump, "Mismatching FPR no : %d\n", num);
			vsr1_h = *(ptr1 + (num * 2));
			vsr2_h = *(ptr2 + (num * 2));

			fprintf(dump, "               FPR[%d]\n", num);
			fprintf(dump, "          ----------------\n");
			fprintf(dump, "Initial : 0x%016llx\n", *(ptri + (num*2)));
			fprintf(dump, "Pass1   : 0x%016llx\n", vsr1_h);
			fprintf(dump, "Pass2   : 0x%016llx\n", vsr2_h);
			for ( i = 0; i < 8; i++) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "            ");
			for ( i = 15; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 2:
			vsr1_h = tc1->tcc.gprs[num];
			vsr2_h = tc2->tcc.gprs[num];
			fprintf(dump, "Miscompare Type    : GPR Miscompare.\n");
			fprintf(dump, "Mismatching GPR no : %d\n", num);
			fprintf(dump, "              GPR[%d]\n", num);
			fprintf(dump, "          ----------------\n");
			fprintf(dump, "Initial : 0x%016llx\n", tci->tcc.gprs[num]);
			fprintf(dump, "Pass1   : 0x%016llx\n", vsr1_h);
			fprintf(dump, "Pass2   : 0x%016llx\n", vsr2_h);
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "            ");
			for ( i = 15; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 3:
			vsr1_h = tc1->tcc.sprs.fpscr;
			vsr2_h = tc2->tcc.sprs.fpscr;
			fprintf(dump, "Miscompare Type    : FPSCR Miscompare.\n");
			fprintf(dump, "                FPSCR\n");
			fprintf(dump, "          ------------------\n");
			fprintf(dump, "Initial : 0x%016llx\n",tci->tcc.sprs.fpscr);
			fprintf(dump, "Pass1   : 0x%016llx\n",vsr1_h);
			fprintf(dump, "Pass2   : 0x%016llx\n",vsr2_h);
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "            ");
			for ( i = 15; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 4:
			fprintf(dump, "Miscompare Type    : VSCR Miscompare.\n");
			fprintf(dump, "                        VSCR\n");
			fprintf(dump, "          ----------------------------------\n");
			fprintf(dump, "Initial : 0x%016llx%016llx\n",tci->tcc.sprs.vscr[0], tci->tcc.sprs.vscr[1]);
			fprintf(dump, "Pass1   : 0x%016llx%016llx\n",tc1->tcc.sprs.vscr[0], tc1->tcc.sprs.vscr[1]);
			fprintf(dump, "Pass2   : 0x%016llx%016llx\n",tc2->tcc.sprs.vscr[0], tc2->tcc.sprs.vscr[1]);
			vsr1_h = tc1->tcc.sprs.vscr[0];
			vsr2_h = tc2->tcc.sprs.vscr[0];
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			vsr1_h = tc1->tcc.sprs.vscr[1];
			vsr2_h = tc2->tcc.sprs.vscr[1];
			for ( i = 16; i < 32; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "            ");
			for ( i = 31; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 5:
			vsr1_h = tc1->tcc.sprs.cr;
			vsr2_h = tc2->tcc.sprs.cr;
			fprintf(dump, "Miscompare Type    : CR Miscompare.\n");
			fprintf(dump, "                 CR\n");
			fprintf(dump, "          ------------------\n");
			fprintf(dump, "Initial : 0x%016llx\n",tci->tcc.sprs.cr);
			fprintf(dump, "Pass1   : 0x%016llx\n",vsr1_h);
			fprintf(dump, "Pass2   : 0x%016llx\n",vsr2_h);
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "            ");
			for ( i = 15; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;

		case 6:
			vsr1_h=*(uint64 *)(base1+num);
			vsr2_h=*(uint64 *)(base2+num);
			fprintf(dump, "Miscompare Type    : Load/Store Area Miscompare.\n");
			fprintf(dump, "Mismatching Offset : 0x%x bytes\n", num);
			fprintf(dump, "Mismatching Address: 0x%llx\n", (uint64)(base1 + num));
			fprintf(dump, "            0x%-16llx 0x%-16llx 0x%-16llx\n", (uint64)(base1 + num - 8),
					(uint64)(base1 + num), (uint64)(base1 + num + 8));
			fprintf(dump, "          ------------------ ------------------ ------------------\n");
			fprintf(dump, "Initial : 0x%016llx 0x%016llx 0x%016llx\n", *((uint64 *)(basei+num-8)),
 					*((uint64 *)(basei+num)), *((uint64 *)(basei+num+8)));
			fprintf(dump, "Pass1   : 0x%016llx 0x%016llx 0x%016llx\n", *((uint64 *)(base1+num-8)),
 					vsr1_h, *((uint64 *)(base1+num+8)));
			fprintf(dump, "Pass2   : 0x%016llx 0x%016llx 0x%016llx\n", *((uint64 *)(base2+num-8)),
 					vsr2_h, *((uint64 *)(base2+num+8)));
			for ( i = 0; i < 16; i++ ) {
				temp_val1[i] = ((vsr1_h & 0xfULL) + '0');
				temp_val2[i] = ((vsr2_h & 0xfULL) + '0');
				vsr1_h >>= 4;
				vsr2_h >>= 4;
			}
			temp_val1[i]='\0';
			temp_val2[i]='\0';

			fprintf(dump, "                               ");
			for ( i = 15; i >= 0; i-- ) {
				if ( temp_val1[i] != temp_val2[i] ) {
					fprintf(dump, "^");
				}
				else {
					fprintf(dump, " ");
				}
			}
			fprintf(dump, "\n");
			break;
	}
	fflush(dump);

	fprintf(dump, "==========================Register Allocation===================================\n");

	for(type_list = SCALAR_SP; type_list <= DFP_QUAD; type_list++) {
		fprintf(dump, "\n Data Type: %d -->", type_list);
		for(ins_cat = VSX; ins_cat <= DFP; ins_cat++) {
			list_h = cptr->vsrs[type_list].head[ins_cat];
			list_t = cptr->vsrs[type_list].tail[ins_cat];
			fprintf(dump, "\n Type: %d -->", ins_cat);
			tmp_vsrs_list = list_h;
			while( tmp_vsrs_list != list_t ) {
				fprintf(dump, " %d",tmp_vsrs_list->vsr_no);
				tmp_vsrs_list = tmp_vsrs_list->next;
			}
			if ((tmp_vsrs_list != NULL) && (tmp_vsrs_list->vsr_no != 0)) {
				fprintf(dump, " %d",tmp_vsrs_list->vsr_no);
			}
		}
		/* Handling VMX separately because in P6 VRs register file doesn't
		 * map to VSRs (32-63) registers. So, we need to subtract 32 from
		 * each register entry.
		 */
		list_h = cptr->vsrs[type_list].head[VMX];
		list_t = cptr->vsrs[type_list].tail[VMX];
		fprintf(dump, "\n Type: %d -->", VMX);
		tmp_vsrs_list = list_h;
		while( tmp_vsrs_list != list_t ) {
			fprintf(dump, " %d",tmp_vsrs_list->vsr_no);
			tmp_vsrs_list = tmp_vsrs_list->next;
		}
		if ((tmp_vsrs_list != NULL) && (tmp_vsrs_list->vsr_no != 0)) {
			fprintf(dump, " %d",tmp_vsrs_list->vsr_no);
		}
	}
	fprintf(dump, "\n\n");

	fprintf(dump, "\n==========================Detailed Dumps per thread============================");
	if ( rule.testcase_sync ) {
		thread_start_index = 0;
		thread_end_index = rule.num_threads;
	}
	else {
		thread_start_index = cno;
		thread_end_index = cno+1;
	}

	for(j = thread_start_index; j < thread_end_index; j++) {
		fprintf(dump, "\n\n<################################# Thread %d ##################################>\n", j);
		tci = global_sdata[INITIAL_BUF].cdata_ptr[j]->tc_ptr[INITIAL_BUF];
		tc1 = global_sdata[PASS1_BUF].cdata_ptr[j]->tc_ptr[PASS1_BUF];
		tc2 = global_sdata[PASS2_BUF].cdata_ptr[j]->tc_ptr[PASS2_BUF];
		ptri = (uint64 *)&tci->tcc.vsrs;
		ptr1 = (uint64 *)&tc1->tcc.vsrs;
		ptr2 = (uint64 *)&tc2->tcc.vsrs;
		fprintf(dump, "\nFPRs[thread#%d]:\n", j);
		fprintf(dump, "---------------\n");
		fprintf(dump, "           INITIAL              PASS1             PASS2\n");
		fprintf(dump, "       ------------------ ------------------ ------------------\n");

		for(i = 0; i < NUM_FPRS; i++) {
			if((*ptr1 != *ptr2)) {
				fprintf(dump, "[%02d]:  0x%016llx 0x%016llx 0x%016llx <<\n", i,
									*ptri,	*ptr1, *ptr2);
			}
			else {
				fprintf(dump, "[%02d]:  0x%016llx 0x%016llx 0x%016llx\n", i,
									*ptri,	*ptr1, *ptr2);
			}
			ptri += 2;
			ptr1 += 2;
			ptr2 += 2;
		}

		ptri = (uint64 *)&tci->tcc.gprs;
		ptr1 = (uint64 *)&tc1->tcc.gprs;
		ptr2 = (uint64 *)&tc2->tcc.gprs;
		fprintf(dump, "\nGPRs[thread#%d]:\n", j);
		fprintf(dump, "---------------\n");
		fprintf(dump, "           INITIAL              PASS1              PASS2\n");
		fprintf(dump, "      ------------------ ------------------ ------------------\n");
		for(i = 0; i < NUM_GPRS; i++) {
			fprintf(dump, "[%02d]: 0x%016llx 0x%016llx 0x%016llx\n", i, *(ptri++), *(ptr1++), *(ptr2++));
		}
		fprintf(dump, "\nFPSCR[thread#%d]:\n", j);
		fprintf(dump, "----------------\n");
		fprintf(dump, "            INITIAL              PASS1              PASS2\n");
		fprintf(dump, "        ------------------ ------------------ ------------------\n");
		fprintf(dump, "FPSCR : 0x%016llx 0x%016llx 0x%016llx\n", tci->tcc.sprs.fpscr,
				tc1->tcc.sprs.fpscr, tc2->tcc.sprs.fpscr);

		fprintf(dump, "\nCR[thread#%d]:\n", j);
		fprintf(dump, "-------------\n");
		fprintf(dump, "         INITIAL              PASS1              PASS2\n");
		fprintf(dump, "     ------------------ ------------------ ------------------\n");
		fprintf(dump, "CR : 0x%016llx 0x%016llx 0x%016llx\n", tci->tcc.sprs.cr,
				tc1->tcc.sprs.cr, tc2->tcc.sprs.cr);

		ptri = (uint64 *)basei; ptr1 = (uint64 *)base1; ptr2 = (uint64 *)base2;
		fprintf(dump, "\nLoad/Store Memory region[thread#%d]:\n", j);
		fprintf(dump, "-----------------------------------\n");
		fprintf(dump, " OFFSET          INITIAL               PASS1                  PASS2\n");
		fprintf(dump, "           [0x%016llx]  [0x%016llx]  [0x%016llx]\n", (uint64)ptri, (uint64)ptr1, (uint64)ptr2);

		fprintf(dump, "--------   ------------------   ------------------   ------------------\n");
		for(i = 0; i < (global_sdata[PASS1_BUF].cdata_ptr[cno]->last_ls_off + 8); i += 8) {
			if((*ptr1 != *ptr2)) {
				fprintf(dump, "[0x%04x] : 0x%016llx   0x%016llx   0x%016llx <<\n", i, (uint64)*ptri++, (uint64)*ptr1++, (uint64)*ptr2++);
			}
			else {
				fprintf(dump, "[0x%04x] : 0x%016llx   0x%016llx   0x%016llx \n", i, (uint64)*ptri++, (uint64)*ptr1++, (uint64)*ptr2++);
			}
		}
		dump_instructions_p7(j, dump);
		fflush(dump);
	}
	fflush(dump);
}


void dumpsiginfo(int client_num)
{
#ifdef AWAN
	if( client_num!= -1 ) {

		uint32 *iar,i;
		struct signal_info *si = &(global_sdata[INITIAL_BUF].cdata_ptr[client_num]->sigdata);
		printf("\n Signal %d(%s) received for client %d  \n", si->sig_num, strsignal(si->sig_num), si->client_num);
		printf(" due to Instruction = 0x%x  \n", *(si->iar));
		printf(" by %s Exerciser \n",hd.HE_name);

		printf(" client_seed = 0x%x parent_seed = 0x%x  \n", si->client_seed, si->parent_seed);

		printf(" Few instructions(with offset from faulty instruction) around the faulty instruction :  \n");
		for( iar = si->iar - 4 ; iar <= si->iar + 2; iar++)
			printf(" 0x%x (%d) at address: %p\n",*iar, iar - si->iar, iar);

		printf("\n ***** Contents of GPRs **** \n");
		for( i=0; i<NUM_GPRS; i++ ) {
			printf(" GPR[%d] - 0x%llx \n", i, si->tcc.gprs[i]);
		}

		printf(" \n***** Contents of FPRs **** \n");
		for( i=0; i<NUM_VSRS/2; i++ ) {
			printf(" FPR[%d] - 0x%llx \n", i, si->tcc.fprs[i][0]);
		}
	}
	else if( nontcsigdata.iar ) {
		printf("\n Signal %d(%s) received (from outside testcase) for instruction at address 0x%x  \n", nontcsigdata.sig_num, strsignal(nontcsigdata.sig_num), nontcsigdata.iar);
	}
#endif
}


void
set_rule_defaults()
{
	struct ruleinfo *r;
	int i, j;

	for(i = 0; i < MAX_NUM_RULES; i++) {
		r = &rule_list[i];
		strcpy(r->rule_id, " ");
		r->stream_depth = 1024;
		r->num_threads = MAX_NUM_CPUS;
		r->page_size = 4096;
		r->test_method = CONSISTENCY_CHECK;
		r->testcase_sync = 0;
		r->sync_distance = MAX_INS_STREAM_DEPTH;
		r->parent_seed = 0;
		r->prot_sao = 0;
		r->testcase_type = SYSTEM_TEST;
		for(j = 0; j < r->num_threads; j++) {
			r->db[j][NORMAL] = 10;
			r->db[j][DENORMAL] = 0;
			if(shifted_pvr_os <= 0x3e) {
				r->ib[j].vsx_mask = 0;
				r->ib[j].bfp_mask = BFP_ALL;
				r->ib[j].dfp_mask = DFP_ALL;
				r->ib[j].vmx_mask = VMX_ALL;
				r->ib[j].cpu_mask = CPU_ALL;
				r->ib[j].bias_mask[0][0] = BFP_ALL;
				r->ib[j].bias_mask[0][1] = 100;
			}
			else {
				r->ib[j].vsx_mask = VSX_ALL;
				r->ib[j].bfp_mask = BFP_ALL;
				r->ib[j].dfp_mask = DFP_ALL;
				r->ib[j].vmx_mask = VMX_ALL;
				r->ib[j].cpu_mask = CPU_ALL;
				r->ib[j].macro_mask = MACRO_ALL;
				r->ib[j].bias_mask[0][0] = VSX_ALL;
				r->ib[j].bias_mask[0][1] = 100;
			}
			r->unaligned_data_pc[j] = 0;
			r->seed[j] = 0;
			r->dump_testcase[j] = 0;
			r->fpscr[j] = 0;
		}
		r->num_oper = 1;
		r->num_pass2 = 1;
		r->tc_time = 0;
		r->log_seed = 0;
		r->compare_flag = 1;
	}
}


int
read_rf()
{
	int /*i,*/ line_number = 0, rc;
	struct ruleinfo *r;
	FILE *rule_file_ptr;

	set_rule_defaults();

	rule_file_ptr = fopen((const char *)dinfo.rule_file_name, "r");
	if(rule_file_ptr == NULL) {
		sprintf(msg, "Error opening rule file \n");
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		return(1);
	}
	num_active_rules = 0;
	r = &rule_list[num_active_rules];
	while (num_active_rules < MAX_NUM_RULES) {
		rc = get_rule(&line_number, rule_file_ptr, r);
		if(rc == -1) {
			fclose(rule_file_ptr);
			return(rc);
		}
		else if (rc == -2) { /* EOF received */
			break;
		}
		else if (rc > 0) { /* End of stanza so move stanza ptr to next */
			/* validate testcase for log_seed and tc_time */
			if (r->log_seed) {
				/* num_pass2 is not used in case of log_seed, pass2 runs infinitely, and num_oper corresponds number of seeds */
				if (r->num_oper == 0) {
					sprintf(msg, "Invalid testcase config: log_seed: %d, num_oper: %d\n", r->log_seed, r->num_oper);
					hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
					return(-1);
				}
  				else if ((r->tc_time / r->num_oper) == 0) {
					sprintf(msg, "Invalid testcase config: log_seed: %d, tc_time: %d, num_oper: %d",  r->log_seed, r->tc_time, r->num_oper);
					hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
					return(-1);
  				}
			}
			else {
  				if (r->tc_time != 0) {
              		/* num_pass2 is not used in this case, it is executed only once, num_oper (0) runs infinitely */
					if ((r->num_oper != 0) || (r->num_pass2 > 1)) {
						sprintf(msg, "Invalid testcase config: tc_time: %d, num_pass2: %d, num_oper: %d",  r->tc_time, r->num_pass2, r->num_oper);
						hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
						return (-1);
  					}
				}
			}

			num_active_rules++;
			r++;
		}
		/* just a new line in rule file so continue */
	}
	if(num_active_rules == MAX_NUM_RULES) {
		sprintf(msg, "Num of stanzas used (%d) are more than supported(%d)",num_active_rules, MAX_NUM_RULES);
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}

	fclose(rule_file_ptr);
	return(0);
}

int
calculate_seg_size()
{
	return (2*2*1024*1024);
}

int
simulate_testcase(int cno)
{
	uint16 prolog_size, num_ins_built, ins, sim_end_off;
	struct client_data *cptr = global_sdata[PASS1_BUF].cdata_ptr[cno];
	void (*sim_ptr1)(uint32 *, struct testcase *);
	uint64 old_fpscr, /*mod_fpscr,*/ f_mask;
	instruction_masks *ins_tuple = NULL;
	sim_ptr1 = NULL;
	/*uint8 val;*/

	prolog_size = cptr->sim_jmp_off;
	num_ins_built = cptr->num_ins_built;
	sim_end_off = cptr->sim_end_off;
	DPRINT(cptr->clog, "\nProlog size = %d, Num-ins = %d Sim end off = %d\n", prolog_size, num_ins_built, sim_end_off);
	for(ins = prolog_size; ins < sim_end_off; ins++) {
		sim_ptr1 = (sim_fptr)(cptr->tc_ptr[PASS1_BUF]->sim_ptr[ins]);
		if(sim_ptr1 == NULL) continue;

		if ( cptr->instr_index[ins] & 0x10000000 ) {
		ins_tuple = &(cptr->enabled_ins_table[(cptr->instr_index[ins] & ~(0x10000000))].instr_table);
		}
		else {
			ins_tuple = NULL;
		}
#if 0
		DPRINT(hlog,"\nSIm_ptr = %lx",sim_ptr1);
		DPRINT(hlog,"\nSimptr = %lx",*sim_ptr1);
		DPRINT(hlog,"\nInstr = %lx", cptr->tc_ptr[PASS1_BUF]->tc_ins[ins]);
		FFLUSH(hlog);
#endif
		old_fpscr = cptr->tc_ptr[PASS1_BUF]->tcc.sprs.fpscr;
		(sim_ptr1)(&(cptr->tc_ptr[PASS1_BUF]->tc_ins[ins]), cptr->tc_ptr[PASS1_BUF]);

		if(ins_tuple != NULL) {
			f_mask = ins_tuple->fpscr_mask;
			adjust_fpscr(cno, old_fpscr, f_mask);
		}
	}
	return (0);
}


void
adjust_fpscr(int cno, uint64 old_fpscr, uint64 fpscr_mask)
{
	uint64 *fpscr = &(global_sdata[PASS1_BUF].cdata_ptr[cno]->tc_ptr[PASS1_BUF]->tcc.sprs.fpscr);
	uint64 fpscr_sticky, modified_fpscr;

	/* fpscr_mask = 0xffffffff for those instruction which can directly modify FPSCR bits.
	 * instructions like mtfsf, mtfsfi, mtfsb0, mtfsb1.
	 * In that case we don't need to do anything here. Simply return.
	 */
	if(fpscr_mask == 0xffffffff) {
		return;
	}

	/* fpscr_mask = 0 for those instructions which can not modify any bit of FPSCR.
	 * Instructions like load/store, frm, fsel etc.
	 * In that case, just restore the initial value of FPSCR.
	 */
	else if(fpscr_mask == 0) {
		*fpscr = old_fpscr;
	}

	/* Else the normal case, where we need to adjust final value in FPSCR
	 * so that only those bits which are to be modified by that instruction are actually touched.
	 * Also, FX bit is handled separately as it might have been set wrongly i.e.
	 * (Setting up of some intermediate bits which this instruction was not supposed to touch)
	 */
	else {
		fpscr_sticky = ((old_fpscr) & ((~fpscr_mask) | FPSCR_STICKY ));
		*fpscr &= fpscr_mask;

		modified_fpscr = ( *fpscr & ~(old_fpscr));
		/* Check if FX is set wrongly */
		if ( ((modified_fpscr) & FPS_EXCEPTIONS) && (fpscr_mask & FPS_FX) ) {
			*fpscr |= FPS_FX;
		}
		else if (fpscr_mask & FPS_FX) {
			*fpscr &= ~(FPS_FX);
		}
		*fpscr |= fpscr_sticky;
		Update_FPSCR(fpscr);
	}
}



/*
 * This function copies entire test case from INITIAL_BUF to either PASS1_BUF or PASS2_BUF and
 * adjusts GPR base pointers for all clients.
 */
int
populate_buf(int pass, int cno)
{
	char *from_ptr, *to_ptr /*,i*/;
	struct server_data *i_buf = &global_sdata[INITIAL_BUF], *pass_buf = &global_sdata[pass];
	struct tc_context *tcc;

	from_ptr = (char *)i_buf->cdata_ptr[cno]->tc_ptr[INITIAL_BUF];
	to_ptr = (char *)pass_buf->cdata_ptr[cno]->tc_ptr[pass];

	memcpy(to_ptr, from_ptr, sizeof(struct testcase));
	/*
	 * Copy comman data
	 */
#if 0
	if(cno == 0) {
		from_ptr = (char *)i_buf->common_data_ptr;
		to_ptr = (char *)pass_buf->common_data_ptr;
		memcpy(to_ptr, from_ptr, sizeof(struct common_data));
		if(pass == PASS2_BUF) {
			int i;
			for(i = 0; i < 2; i++) {
				printf("cno: %d pass: %d sync words = 0x%x 0x%x \n", cno, pass, pass_buf->common_data_ptr->sync_words[0], pass_buf->common_data_ptr->sync_words[1]);
			}
		}
	}
#endif
	/*
	 * Adjust the GPR base pointers.
	 */
	tcc =  &(pass_buf->cdata_ptr[cno]->tc_ptr[pass]->tcc);
	tcc->gprs[3] = (uint64)(pass_buf->cdata_ptr[cno]->tc_ptr[pass]);
	tcc->gprs[4] = 0;
	tcc->gprs[5] = (uint64)(pass_buf->cdata_ptr[cno]->ls_base[pass]);
	tcc->gprs[6] = 0;
	tcc->gprs[7] = (uint64)(pass_buf->cdata_ptr[cno]->ls_base[pass]);
	tcc->sprs.fpscr = rule.fpscr[cno];
	tcc->sprs.cr = 0;
	return(0);
}

void
initialize_client_mem(void)
{
	int i, j/*, k*/;
	struct client_data *cptr = NULL;
	struct server_data *s;
	/*struct common_data *cd;*/

#ifndef SCTU
	for(i = 0; i < 3; i++) {
		s = &global_sdata[i];
		for (j = 0; j < MAX_NUM_CPUS; j++) {
			cptr = s->cdata_ptr[j];
			cptr->tc_ptr[i] = (struct testcase *)((char *)s->shm_buf_ptr->ptr + (j * /*MEM_OFFSET_BETWEEN_CLIENTS*/ sizeof(struct testcase)));
			cptr->ls_current[i] = cptr->ls_base[i] = cptr->tc_ptr[i]->ls_ptr;
			cptr->ls_base_volatile[i] = cptr->tc_ptr[i]->ls_ptr;
			DPRINT(hlog,  "%s: c no: %d tc_ptr: %llx ls_base: %llx \n", __FUNCTION__, j, (uint64)cptr->tc_ptr[i], (uint64)cptr->ls_base[i]);
		}

		/*
		 * assign memory for common data. Common data are situated after all client specific data.
		 */
		s->common_data_ptr = (struct common_data *)((char *)s->cdata_ptr[j - 1]->tc_ptr[i] + /*MEM_OFFSET_BETWEEN_CLIENTS*/ sizeof(struct testcase));
		DPRINT(hlog, "%s: testcase ptr]%d] of cdata_ptr[%d] = %llx, common_data_ptr: %llx\n", __FUNCTION__, i, j-1, (uint64)s->cdata_ptr[j - 1]->tc_ptr[i], (uint64)s->common_data_ptr);

	}
#else
	for(i = 0; i < 3; i++) {
		s = &global_sdata[i];
		/*
		 * assign memory for common data. Common data are situated after all client specific data.
		 */
		s->common_data_ptr = (struct common_data *) ((char *)s->shm_buf_ptr->ptr + (MAX_NUM_CPUS * sizeof(struct testcase) /*MEM_OFFSET_BETWEEN_CLIENTS*/));
		DPRINT(hlog,  "BUFFER/PASS : %d \n", i);
		DPRINT(hlog,  "common_data_ptr: %llx \n",s->common_data_ptr);
		DPRINT(hlog,  "common_data_ptr->ls_ptr: %llx \n",s->common_data_ptr->ls_ptr);
		DPRINT(hlog,  "common_data_ptr->sync_words: %llx \n",s->common_data_ptr->sync_words);
	}

	for(i = 0; i < 3; i++) {
		s = &global_sdata[i];
		DPRINT(hlog,  "BUFFER/PASS : %d \n", i);
		for (j = 0; j < MAX_NUM_CPUS; j++) {
			cptr = s->cdata_ptr[j];
			cptr->tc_ptr[i] = (struct testcase *)((char *)s->shm_buf_ptr->ptr + (j * sizeof(struct testcase) /*MEM_OFFSET_BETWEEN_CLIENTS*/));
			cptr->ls_base[i] = cptr->ls_base_volatile[i] =  (char *)(((uint64)s->common_data_ptr->ls_ptr + CACHE_LINE_SIZE - 1) & (uint64)~(CACHE_LINE_SIZE - 1));
			cptr->ls_current[i] =  cptr->ls_cl_start[i] = cptr->ls_base[i] + j * CLIENT_NUM_BYTES_IN_CL;
			DPRINT(hlog,  "c no: %d tc_ptr: %llx ls_base: %llx \n", j, cptr->tc_ptr[i], cptr->ls_base[i]);
			DPRINT(hlog,  "c no: %d ls_current: %llx ls_cl_start: %llx \n", j, cptr->ls_current[i], cptr->ls_cl_start[i]);
			DPRINT(hlog,  "c no: %d ls_base: %llx \n", j, cptr->ls_base[i]);
		}
	}
#endif

}
int
initialize_client(int cno)
{
	int i, /*j,*/ k, l, /*m,*/ n;
	/*struct client_data *cptr = NULL;*/
	struct server_data *s;
	/*struct common_data *cd;*/

	s = &global_sdata[INITIAL_BUF];
	/*  VSR 0 is not used */
	for (i = 1; i < NUM_VSRS; i++) {
		s->cdata_ptr[cno]->vsx_reg[i-1] = i;
	}

	for (i = 0; i < VSR_OP_TYPES; i++) {
		/*
		 * Copy the data generation routines based on biasing specified in rule file.
		 */
		for (k = 0, n = 0; k < BFP_DATA_BIAS_TYPES; k++) {
			for (l = 0; l < rule.db[cno][k]; l++) {
				s->cdata_ptr[cno]->value_gen[i][n] = fp_value_gen_fptr_array[i][k];
				n++;
			}
		}
	}

	for (i = 0; i < 100; i++) {
		s->cdata_ptr[cno]->rand_nos[i] = i;
	}

	s->cdata_ptr[cno]->cpu_id_mask = (0x1ULL << cno);



#if 0
		for(i = 0; i < VSR_OP_TYPES; i++) {
			s->cdata_ptr[j]->vsr_reg_wt[i] = vsr_reg_wt[i];
		}
		for(i = 0; i < (BFP_OP_TYPES + 1); i++) {
			s->cdata_ptr[j]->bfp_reg_wt[i] = bfp_reg_wt[i];
		}
#else
	if (distribute_vsrs_based_on_ins_bias(cno)) {
		return (-1);
	}
#endif
	return(0);
}

int
initialize_gprs_n_sprs(int client_no)
{
	int i;
	struct vsr_node *tmp, *prev;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	/*
	 * Fill CR list
	 */
	tmp = sdata->cdata_ptr[client_no]->vsrs[CR_T].head[VSX];
	/* CR0 is used by prolog code, hence using only CR1-CR7 */
	/* CR1 is implicitly used by all the BFP Dot instructions, hence not using that also */
	/* Also when VMX is integrated, CR6 should also not be used in this list of CRs */
	for(i = 2; i < 8; i++) {
		if ( i== 6)
			continue;
		tmp->vsr_no = i;
		prev = tmp;
		tmp = tmp->next;
	}
	sdata->cdata_ptr[client_no]->vsrs[CR_T].num_vsrs = 6;
	sdata->cdata_ptr[client_no]->vsrs[CR_T].tail[VSX] = prev;
	sdata->cdata_ptr[client_no]->vsrs[CR_T].tail[BFP] = prev;
	sdata->cdata_ptr[client_no]->vsrs[CR_T].tail[DFP] = prev;
	sdata->cdata_ptr[client_no]->vsrs[CR_T].dirty_mask = 0;
	return(0);
}

int get_mem_and_offset(uint32 client_no, int memsize, char* ptr, char **updated_ptr)
{
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	uint64 offset;

#ifndef SCTU
	cptr->ls_current[INITIAL_BUF] = ptr + memsize;
	offset = (uint64)ptr - (uint64)cptr->ls_base_volatile[INITIAL_BUF];
	cptr->last_ls_off = (cptr->ls_base_volatile[INITIAL_BUF] - cptr->ls_base[INITIAL_BUF]) + offset;
	*updated_ptr = ptr;
	DPRINT(cptr->clog, "%s: memsize = %X, ptr = %llx, cptr->ls_base_volatile[INITIAL_BUF] = %llx, offset = 0x%llx\n", __FUNCTION__, memsize, (uint64)ptr, (uint64)cptr->ls_base_volatile[INITIAL_BUF], offset);
	if (offset > (MAX_ADDI_OFFSET - 16)) {
		DPRINT(cptr->clog, "client_no: %d, core_num: %d ********Setting  reinit_base flag**************\n", client_no, core_num);
		cptr->reinit_base = 1;
	}
#else
	uint64 align_size;
	char *align_ptr;

	if (cptr->ls_current[INITIAL_BUF] == cptr->ls_base_volatile[INITIAL_BUF] ){
		/* advance ls_current by word to be able to display full offset in dump file properly as we update only non zero offsets*/
		cptr->ls_current[INITIAL_BUF] = cptr->ls_current[INITIAL_BUF] + WORD;
		if( client_no != 0 ){
			sprintf(msg, "client_no != 0 while condition cptr->ls_current[INITIAL_BUF] == cptr->ls_base_volatile[INITIAL_BUF] became true \n");
			hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			exit(1);
		}
	}

	char * cl_ptr = cptr->ls_cl_start[INITIAL_BUF];
	/* reset ptr to undo effects of fpu/cpu aligment/unalignment */
	ptr = cptr->ls_current[INITIAL_BUF];
	align_size = memsize;
	align_ptr = (char *)(((uint64)ptr + align_size -1 ) & (~(align_size - 1)));

	/* if we have space in this cache line, allocate it */
	if( (align_ptr + memsize - 1) <= (cl_ptr + CLIENT_NUM_BYTES_IN_CL - 1)) {
		ptr = (char *)(((uint64)ptr + align_size -1 ) & (~(align_size - 1)));
		*updated_ptr = ptr;
		cptr->ls_current[INITIAL_BUF] = ptr + memsize;
		offset = (uint64)ptr - (uint64)cptr->ls_base_volatile[INITIAL_BUF];
		cptr->last_ls_off = (cptr->ls_base_volatile[INITIAL_BUF] - cptr->ls_base[INITIAL_BUF]) + offset;
		/*if offset > (0x7fff - 0x80) i.e we are in the last cache line of offsetable area, update base*/
		if( offset > (MAX_ADDI_OFFSET - (2 * CACHE_LINE_SIZE)) ){
			cptr->reinit_base = 1;
		}
	}
	else {
		cl_ptr = cl_ptr + (rule.num_threads * CLIENT_NUM_BYTES_IN_CL);
		cptr->ls_cl_start[INITIAL_BUF] = cptr->ls_current[INITIAL_BUF] = ptr = cl_ptr;
		ptr = (char *)(((uint64)ptr + align_size -1 ) & (~(align_size - 1)));
		*updated_ptr = ptr;
		cptr->ls_current[INITIAL_BUF] = ptr + memsize;
		offset = (uint64)ptr - (uint64)cptr->ls_base_volatile[INITIAL_BUF];
		cptr->last_ls_off = (cptr->ls_base_volatile[INITIAL_BUF] - cptr->ls_base[INITIAL_BUF]) + offset;
		/*if offset > (0x7fff - 0x80) i.e we are in the last cache line of offsetable area, update base*/
		if( offset >  (MAX_ADDI_OFFSET - (2 * CACHE_LINE_SIZE)) ){
			cptr->reinit_base = 1;
		}
	}
#endif

	return(offset);
}

/*
 * Initialize memory area pointed to by ptr with specified data type.
 */
void
gen_data_pat(int client_no, char *ptr, int dtype)
{
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	(*sdata->cdata_ptr[client_no]->value_gen[dtype][get_random_no_32(client_no) % BFP_DATA_BIAS_TYPES])(client_no, (char *)ptr, dtype);
}

uint32
init_mem_for_vsx_store(int client_no, int tgt_dtype)
{
	/*
	 * Reserve memory for store op depending upon data type. Init memory with client_no pattern.
	 */
	char *ptr, *updated_ptr;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	uint32 offset;
	uint64 alignment;

	ptr = cptr->ls_current[INITIAL_BUF];
	/*DPRINT(cptr->clog, "\n%s: ls current=%llx\n", __FUNCTION__, ptr);*/
	switch(tgt_dtype) {
		case BFP_SP:
		case BFP_DP:
		case BFP_QP:
		case VECTOR_SP:
		case VECTOR_DP:
		case QGPR:
		case CR_T:
		case GR:
		case DFP_SHORT:
		case DFP_LONG:
		case DFP_QUAD:
		case SCALAR_HP:
		case VECTOR_HP:
			alignment = cptr->data_alignment[cptr->rand_nos[cptr->data_alignment_ctr]];
			cptr->data_alignment_ctr++;
			cptr->data_alignment_ctr = cptr->data_alignment_ctr % 100;
			ptr = (char *)(((uint64)ptr + ALIGN_W - 1) & ((uint64)~(ALIGN_W - 1)));
			if(alignment == 0) {
				ptr = ptr + 1;
			}
			DPRINT(cptr->clog, "%s: alignment: %lld, cptr->ls_current[INITIAL_BUF] = %llx, aligned ptr: %llx\n", __FUNCTION__, alignment, (uint64)cptr->ls_current[INITIAL_BUF], (uint64)ptr);
			/*DPRINT(cptr->clog, "%s: client_no: %d, sizes_for_types[%d] = %d, data_alignment_ctr =%d\n", __FUNCTION__, client_no, tgt_dtype, sizes_for_types[tgt_dtype], cptr->data_alignment_ctr);*/
			offset = get_mem_and_offset(client_no, sizes_for_types[tgt_dtype], ptr, &updated_ptr);
			break;
		default:
			sprintf(msg, "%s: wrong target data type(%d) for store memory request",__FUNCTION__, tgt_dtype);
			hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			return(-1);
	}
	/*cptr->last_ls_off = offset;*/
	DPRINT(cptr->clog, "%s: type=%d, ls_current = 0x%llx, offset =  0x%x\n", __FUNCTION__, tgt_dtype, (uint64)ptr, offset);
	return(offset);
}

uint32
init_mem_for_vsx_load(int client_no, int tgt_dtype)
{
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	uint32 offset;
	uint64 alignment;
	char *ptr, *updated_ptr;
	/*
	 * Reserve memory for load op depending upon data type and initialize the same.
	 */
	ptr = cptr->ls_current[INITIAL_BUF];
	switch(tgt_dtype) {
		case BFP_SP:
		case BFP_DP:
		case BFP_QP:
		case VECTOR_SP:
		case VECTOR_DP:
		case QGPR:
		case CR_T:
		case GR:
		case DFP_SHORT:
		case DFP_LONG:
		case DFP_QUAD:
		case SCALAR_HP:
		case VECTOR_HP:

			alignment = cptr->data_alignment[cptr->rand_nos[cptr->data_alignment_ctr]];
			cptr->data_alignment_ctr++;
			cptr->data_alignment_ctr = cptr->data_alignment_ctr % 100;
			ptr = (char *)(((uint64)ptr + ALIGN_W - 1) & ((uint64)~(ALIGN_W - 1)));
			if(alignment == 0) {
				ptr = ptr + 1;
			}
			offset = get_mem_and_offset(client_no, sizes_for_types[tgt_dtype], ptr, &updated_ptr);
			gen_data_pat(client_no, updated_ptr, tgt_dtype);
			break;
		default:
			sprintf(msg, "%s: wrong target data type(%d) for store memory request",__FUNCTION__, tgt_dtype);
			hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			return(-1);
	}
	/*cptr->last_ls_off = offset;*/
	DPRINT(cptr->clog, "%s: type=%d, 0x%llx 0x%x\n", __FUNCTION__, tgt_dtype, (uint64)ptr, offset);
	return(offset);
}

/*
 * allocate_vsr_mem initializes link list for all data types.
 */
uint32
init_mem_for_gpr(int client_no, int memsize)
{
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	uint32 offset;
	uint64 alignment, BASE_ADDR;
	char *ptr, *updated_ptr;
	char last_byte=0,offset_ref = 0x10;
	struct tc_context	*tcc;
	tcc =  &(cptr->tc_ptr[INITIAL_BUF]->tcc);
	/*
	 * Reserve memory for load op depending upon data type and initialize the same.
	 */
	ptr = cptr->ls_current[INITIAL_BUF];
	alignment = cptr->data_alignment[cptr->rand_nos[cptr->data_alignment_ctr]];
	cptr->data_alignment_ctr++;
	cptr->data_alignment_ctr = cptr->data_alignment_ctr % 100;
	if(cptr->bdy_req == 16){							/* need Quadword bndy*/
		ptr = (char *)(((uint64)ptr + ALIGN_QW - 1) & ((uint64)~(ALIGN_QW - 1)));
	} else if(cptr->bdy_req == 8){							/* need double word bndy*/
		ptr = (char *)(((uint64)ptr + ALIGN_DW - 1) & ((uint64)~(ALIGN_DW - 1)));
	} else if(cptr->bdy_req == 4){							/* need word bndy*/
		ptr = (char *)(((uint64)ptr + ALIGN_W - 1) & ((uint64)~(ALIGN_W - 1)));
	} else if(cptr->bdy_req == 2){							/* need half word bndy*/
		ptr = (char *)(((uint64)ptr + ALIGN_HW - 1) & ((uint64)~(ALIGN_HW - 1)));
	} else if(cptr->bdy_req == 1){							/* need byte bndy*/
		ptr = (char *)ptr + 1;
	}else if(cptr->bdy_req == ALIGN_2QW){							
		ptr = (char *)(((uint64)ptr + ALIGN_2QW - 1) & ((uint64)~(ALIGN_2QW - 1)));
	} else{
		ptr = (char *)(((uint64)ptr + ALIGN_W - 1) & ((uint64)~(ALIGN_W - 1)));
		if(alignment == 0) {
			ptr = ptr + 1;
		}
	}
	/* For instructions which need word and doubleword bdy, the base address will be a source of problem.
	 * Because,the offset aboveis set to satisfy bdy requirements but the base address may not satisfy the
	 * conditions.Like, offset = 0x600 and base address is fff7a31d18c,then the effective address will not
	 * be satisfy double word bdy requirements.
	 * Hence, modify offset such that the base address + offset will satisfy boundary requirements
	 * Procedure: Take last byte of address = 0xc and subtract from 0x10 which will give 0x4.
	 * Add this to offset which gives 0x604 as offset.Effective address = 0xfff7a31d18c + 0x604
	 * = FFF7A31D790 which will satisfy bdy requirements.
	 * THIS LOGIC WILL HOLD GOOD FOR ALL BDY REQUIREMENTS TILL QUADWORD BDY
	 */

	BASE_ADDR = (uint64)(sdata->cdata_ptr[client_no]->ls_base[INITIAL_BUF]);
	if( (cptr->bdy_req == 16) || (cptr->bdy_req == 8) ){
		if( (BASE_ADDR & 0x000000000000000fULL ) > 0 ) {	/* if this is zero,then it means that base address
								 satisfies all bdy requirements */
		    /* Subtract the last byte of address from 0x10 and get the offset */
		    last_byte= BASE_ADDR & 0x000000000000000fULL;       /* get the last byte */
		    ptr += (offset_ref - last_byte);          /* If 0xc is last byte, then we get a value of 4
								   which is added to ptr.This difference will
								   go into offset*/
	    }
	}
	offset = get_mem_and_offset(client_no, memsize, ptr, &updated_ptr);
	/*
	DPRINT(cptr->clog, "%s: type=%d, 0x%llx %d\n", __FUNCTION__, tgt_dtype, ptr, offset);
	*/
	return(offset);
}

int
allocate_vsr_mem(int client_no)
{
	int i, j, rc = 0;
	struct vsr_node *tmp=NULL;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];

	for(i = 0; i < VSR_OP_TYPES; i++) {
		cptr->vsrs[i].head[VSX] = (struct vsr_node *)malloc(sizeof(struct vsr_node));
		tmp = cptr->vsrs[i].head[VSX];
		if(tmp == NULL) {
			rc = errno;
			sprintf(msg, "%s: malloca failure: %d",__FUNCTION__, errno);
			hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
			free_vsr_mem(client_no);
			return(rc);
		}
		cptr->vsrs[i].tail[VSX] = tmp;
		bzero(tmp, sizeof(struct vsr_node));
		for(j = 1; j < NUM_VSRS; j++) {
			tmp->next = (struct vsr_node *)malloc(sizeof(struct vsr_node));
			if(!(tmp->next)) {
				rc = errno;
				sprintf(msg, "%s: malloca failure: %d",__FUNCTION__, errno);
				hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
				free_vsr_mem(client_no);
				return(rc);
			}
			tmp = tmp->next;
			bzero(tmp, sizeof(struct vsr_node));
		}
		tmp->next = NULL;
	}
	for(i = 1; i <= BFP_OP_TYPES; i++) {
		cptr->vsrs[i].head[BFP] = (struct vsr_node *)malloc(sizeof(struct vsr_node));
		tmp = cptr->vsrs[i].head[BFP];
		cptr->vsrs[i].tail[BFP] = tmp;
		if(tmp == NULL) {
			rc = errno;
			sprintf(msg, "%s: malloca failure: %d",__FUNCTION__, errno);
			hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
			free_vsr_mem(client_no);
			return(rc);
		}
		bzero(tmp, sizeof(struct vsr_node));
		for(j = 1; j < NUM_FPRS; j++) {
			tmp->next = (struct vsr_node *)malloc(sizeof(struct vsr_node));
			if(!(tmp->next)) {
				rc = errno;
				sprintf(msg, "%s: malloca failure: %d",__FUNCTION__, errno);
				hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
				free_vsr_mem(client_no);
				return(rc);
			}
			tmp = tmp->next;
			bzero(tmp, sizeof(struct vsr_node));
		}
		tmp->next = NULL;
	}
	for(i = DFP_SHORT; i <= DFP_QUAD; i++) {
		cptr->vsrs[i].head[DFP] = (struct vsr_node *)malloc(sizeof(struct vsr_node));
		tmp = cptr->vsrs[i].head[DFP];
		cptr->vsrs[i].tail[DFP] = tmp;
		if(tmp == NULL) {
			rc = errno;
			sprintf(msg, "%s: malloca failure: %d",__FUNCTION__, errno);
			hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
			free_vsr_mem(client_no);
			return(rc);
		}
		bzero(tmp, sizeof(struct vsr_node));
		for(j = 1; j < NUM_FPRS; j++) {
			tmp->next = (struct vsr_node *)malloc(sizeof(struct vsr_node));
			if(!(tmp->next)) {
				rc = errno;
				sprintf(msg, "%s: malloca failure: %d",__FUNCTION__, errno);
				hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
				free_vsr_mem(client_no);
				return(rc);
			}
			tmp = tmp->next;
			bzero(tmp, sizeof(struct vsr_node));
		}
		tmp->next = NULL;
	}

	for(i = VECTOR_SP; i <= QGPR; i++) {
		if (i == VECTOR_DP)
			continue;

		cptr->vsrs[i].head[VMX] = (struct vsr_node *)malloc(sizeof(struct vsr_node));
		tmp = cptr->vsrs[i].head[VMX];
		cptr->vsrs[i].tail[VMX] = tmp;
		if(tmp == NULL) {
			rc = errno;
			sprintf(msg, "%s: malloca failure: %d",__FUNCTION__, errno);
			hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
			free_vsr_mem(client_no);
			return(rc);
		}
		bzero(tmp, sizeof(struct vsr_node));
		for(j = 1; j < NUM_FPRS; j++) {
			tmp->next = (struct vsr_node *)malloc(sizeof(struct vsr_node));
			if(!(tmp->next)) {
				rc = errno;
				sprintf(msg, "%s: malloca failure: %d",__FUNCTION__, errno);
				hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
				free_vsr_mem(client_no);
				return(rc);
			}
			tmp = tmp->next;
			bzero(tmp, sizeof(struct vsr_node));
		}
		tmp->next = NULL;
	}

	/* Since we also require some CR fields for BFP instructions, hence initializing CR list for BFP is
	 * required. For that, a workaround is making it to point to VSX CR list.
	 */
	cptr->vsrs[CR_T].head[BFP] = cptr->vsrs[CR_T].head[VSX];
	cptr->vsrs[CR_T].tail[BFP] = cptr->vsrs[CR_T].tail[VSX];
	cptr->vsrs[DUMMY].head[BFP] = cptr->vsrs[DUMMY].head[VSX];
	cptr->vsrs[DUMMY].tail[BFP] = cptr->vsrs[DUMMY].tail[VSX];
	cptr->vsrs[CR_T].head[DFP] = cptr->vsrs[CR_T].head[VSX];
	cptr->vsrs[CR_T].tail[DFP] = cptr->vsrs[CR_T].tail[VSX];
	cptr->vsrs[DUMMY].head[DFP] = cptr->vsrs[DUMMY].head[VSX];
	cptr->vsrs[DUMMY].tail[DFP] = cptr->vsrs[DUMMY].tail[VSX];
	cptr->vsrs[CR_T].head[VMX] = cptr->vsrs[CR_T].head[VSX];
	cptr->vsrs[CR_T].tail[VMX] = cptr->vsrs[CR_T].tail[VSX];
	cptr->vsrs[DUMMY].head[VMX] = cptr->vsrs[DUMMY].head[VSX];
	cptr->vsrs[DUMMY].tail[VMX] = cptr->vsrs[DUMMY].tail[VSX];
	return(rc);
}

void
shuffle (int c, uint16 *ptr, uint16 range)
{
	int i = 0, j = 0, k = (range - 1), limit1, limit2;
	uint64 local_tmp;
	int swap_tmp, a, b;

	if(range/64 == 0) {
		limit1 = 1;
	}
	else {
		if(range % 64 == 0) limit1 = range / 64;
		else limit1 = (range / 64) + 1;
	}

	for(a = 0; a < limit1; a++) {
		local_tmp = (uint32)get_random_no_32(c);
		local_tmp <<= 32;
		local_tmp |= (uint32)get_random_no_32(c);
		if( a == (limit1 - 1)) {
			limit2 = range % 64;
		}
		else {
			limit2 = 64;
		}
		for(b = 0; b < limit2; b++) {
			i = local_tmp % range;
			j = (range - 1) - i;
			local_tmp  = local_tmp >> 1;
			swap_tmp = ptr[k];
			ptr[k] = ptr[i];
			ptr[i] = ptr[j];
			ptr[j] = swap_tmp;
			k--;
		}
	}
}

/*
 * free_vsr_mem frees up vsr data types link list memory.
 */
int
free_vsr_mem(int client_no)
{
	int i, j;
	struct vsr_node *tmp=NULL, *prev=NULL;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];

	for(i = 0; i < VSR_OP_TYPES; i++) {
		tmp = sdata->cdata_ptr[client_no]->vsrs[i].head[VSX];
		for(j = 0; j < NUM_VSRS; j++) {
			prev = tmp;
			if(tmp) {
				tmp = tmp->next;
				free(prev);
			}
		}
		sdata->cdata_ptr[client_no]->vsrs[i].head[VSX] = NULL;
		sdata->cdata_ptr[client_no]->vsrs[i].tail[VSX] = NULL;
		sdata->cdata_ptr[client_no]->vsrs[i].num_vsrs = 0;
	}
	for(i = 1; i <= BFP_OP_TYPES; i++) {
		tmp = sdata->cdata_ptr[client_no]->vsrs[i].head[BFP];
		for(j = 0; j < NUM_FPRS; j++) {
			prev = tmp;
			if(tmp) {
				tmp = tmp->next;
				free(prev);
			}
		}
	}
	for(i = DFP_SHORT; i <= DFP_QUAD; i++) {
		tmp = sdata->cdata_ptr[client_no]->vsrs[i].head[DFP];
		for(j = 0; j < NUM_FPRS; j++) {
			prev = tmp;
			if(tmp) {
				tmp = tmp->next;
				free(prev);
			}
		}
	}

	for( i = VECTOR_SP; i <= QGPR; i++) {
		if (i == VECTOR_DP)
				continue;

		tmp = sdata->cdata_ptr[client_no]->vsrs[i].head[VMX];
		for(j = 0; j < NUM_FPRS; j++) {
			prev = tmp;
			if(tmp) {
				tmp = tmp->next;
				free(prev);
			}
		}
	}
	return (0);
}

/*
 * free_bfp_mem frees up bfp data types link list memory.
 */
int
free_bfp_mem(int client_no)
{
	int i, j;
	struct vsr_node *tmp=NULL, *prev=NULL;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];

	for(i = 1; i <= BFP_OP_TYPES; i++) {
		tmp = sdata->cdata_ptr[client_no]->vsrs[i].head[BFP];
		for(j = 0; j < NUM_FPRS; j++) {
			prev = tmp;
			tmp = tmp->next;
			free(prev);
		}
		sdata->cdata_ptr[client_no]->vsrs[i].head[BFP] = NULL;
		sdata->cdata_ptr[client_no]->vsrs[i].tail[BFP] = NULL;
		sdata->cdata_ptr[client_no]->vsrs[i].num_vsrs = 0;
	}
	return (0);
}

int
impart_context_in_memory(int cno)
{
	int i ;
	uint8 j;
	struct vsr_list 	*vrl;
	struct vsr_node 	*vsn;
	struct tc_context	*tcc;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[cno];

	/*
	 * Initialize INITIAL_BUF with VSRs, FPSCR, GPRs, CRs
	 */
	tcc =  &(cptr->tc_ptr[INITIAL_BUF]->tcc);
	/* BFP_QP registers are not shared with VSX. That is why need to update memory for them separately.
	 * Rest BFP_SP and BFP_DP will get updated with SCALAR_SP and SCALAR_DP types respectively.
	 */
	
	vrl = &(cptr->vsrs[BFP_QP]);
	vsn = vrl->head[BFP];
	j = vrl->num_vsrs;
	DPRINT(cptr->clog, "%s: type: %d num vsrs: %d \n", __FUNCTION__, i, j);
	while((char *)vsn != NULL && j > 0) {
		if ( vsn->vsr_no > 63 ) {
			sprintf(msg, "Bug in VSR no allocation: %d type - BFP_QP\n j - %d vsn - 0x%llx vrl - 0x%llx\n", vsn->vsr_no, j, (uint64)vsn, (uint64)vrl);
			hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			return(-1);
		}
		gen_data_pat(cno, (char *)&tcc->vsrs[vsn->vsr_no*4], BFP_QP);
		DPRINT(cptr->clog, "Initializing VSR no: %d for type BFP_QP\n",vsn->vsr_no);
		vsn = vsn->next; j--;
	}

	for(i = SCALAR_SP; i <= VECTOR_HP; i++) {
		vrl = &(cptr->vsrs[i]);
		vsn = vrl->head[VSX];
		j = vrl->num_vsrs;
		DPRINT(cptr->clog, "%s: type: %d num vsrs: %d \n", __FUNCTION__, i, j);
		while(((char *)vsn != NULL) && (j > 0)) {
			if(vsn->vsr_no > 63) {
				sprintf(msg, "Bug in VSR allocation: vsr_no: %d, type - %d\n j - %d vsn - 0x%llx vrl - 0x%llx\n", vsn->vsr_no, i, j, (uint64)vsn, (uint64)vrl);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			gen_data_pat(cno, (char *)&tcc->vsrs[vsn->vsr_no*4], i);
			DPRINT(cptr->clog, "Initializing VSR no: %d for type %d\n",vsn->vsr_no, i);
			vsn = vsn->next; j--;
		}
	}
	for(i = DFP_SHORT; i <= DFP_QUAD; i++) {
		vrl = &(sdata->cdata_ptr[cno]->vsrs[i]);
		vsn = vrl->head[DFP];
		j = vrl->num_vsrs;
		DPRINT(cptr->clog, "%s: type: %d num vsrs: %d \n", __FUNCTION__, i, j);
		while((char *)vsn != NULL && j > 0) {
		    if(vsn->vsr_no > 31) {
			sprintf(msg, "Bug in VSR no allocation: %d type - %d\n j - %d vsn - 0x%llx vrl - 0x%llx\n", vsn->vsr_no, i, j, (uint64)vsn, (uint64)vrl);
			hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			return(-1);
		    }
		    gen_data_pat(cno, (char *)&tcc->vsrs[vsn->vsr_no*4], i);
			DPRINT(cptr->clog, "Initializing VSR no: %d for type %d\n",vsn->vsr_no, i);
		    vsn = vsn->next; j--;
		    if(i == DFP_QUAD) {
				j--;
		    }
		}
	}

	tcc->gprs[3] = (uint64)(sdata->cdata_ptr[cno]->tc_ptr[INITIAL_BUF]);
	tcc->gprs[4] = 0;
	tcc->gprs[5] = (uint64)(sdata->cdata_ptr[cno]->ls_base[INITIAL_BUF]);
	tcc->gprs[6] = 0;
	tcc->gprs[7] = (uint64)(sdata->cdata_ptr[cno]->ls_base[INITIAL_BUF]);
	for (i = 11; i < NUM_GPRS; i++) {
		tcc->gprs[i] = get_random_no_64(cno);
    }
	tcc->sprs.cr = 0;
	tcc->sprs.fpscr = rule.fpscr[cno];
	tcc->sprs.vscr[0] = tcc->sprs.vscr[1] = 0x0;
	tcc->sprs.comp_fpscr = TRUE;
	return(0);
}

/*
 * initialize_vsrs assigns vsr nos to each of the data types link list.
 */
int
initialize_vsrs(int client_no)
{
	struct vsr_node *tmp1 = NULL, *tmp2 = NULL, *tmp3 = NULL;
	struct vsr_node *prev1 = NULL, *prev2 = NULL, *prev3 = NULL, *prev_ary[VSR_OP_TYPES];
	struct vsr_node *tmp = NULL,*prev_cpu = NULL;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	uint32 i, j, k, l = 0;
	uint32 vsx_reg_used[NUM_VSRS], gpr_reg_used[NUM_GPRS - START_GPR_USED];

	bzero(vsx_reg_used, sizeof(vsx_reg_used));
	bzero(gpr_reg_used, sizeof(gpr_reg_used));

	for(i = 1; i < NUM_VSRS; i++) {
		cptr->vsx_reg[i-1] = i;
	}
	shuffle(client_no, cptr->vsx_reg, (NUM_VSRS - 1));

	for(i = 0; i < VSR_OP_TYPES; i++) {
		prev_ary[i] = cptr->vsrs[i].head[VSX];
	}
	for(i = 0; i < (NUM_VSRS); i++) {
		vsx_reg_used[i] = 0;
	}

	/*
	 * i runs up to VSR_OP_TYPES because for type CR_T, these two variables are
	 * initialized in initialize_gprs_n_sprs functions.
	 */
	for(i = 0; i < (VSR_OP_TYPES); i++) {
		cptr->vsrs[i].num_vsrs = 0;
		cptr->vsrs[i].dirty_mask = 0;
		cptr->vsrs[i].tail[VSX] = cptr->vsrs[i].head[VSX];
		cptr->vsrs[i].tail[BFP] = cptr->vsrs[i].head[BFP];
		cptr->vsrs[i].tail[DFP] = cptr->vsrs[i].head[DFP];
		cptr->vsrs[i].tail[VMX] = cptr->vsrs[i].head[VMX];
		if ( i <= BFP_DP || i == BFP_QP ) {
			cptr->vsrs[i].head[BFP]->vsr_no = 0;
		}
		if ( i >= DFP_SHORT && i <= DFP_QUAD  ) {
			cptr->vsrs[i].head[DFP]->vsr_no = 0;
		}
		if (( i == VECTOR_SP) || (i == QGPR)) {
			cptr->vsrs[i].head[VMX]->vsr_no = 0;
		}
		cptr->vsrs[i].head[VSX]->vsr_no = 0;
	}
	/* Only GPRs starting START_USED_GPR till MAX_REG is used i.e. only GPR 11 till GPR 31 will
	 * will be part of linked list
	 */
	for(i = 0; i < RANGE_GPRS ; i++) {
		cptr->gpr_reg[i] = i + START_GPR_USED;
	}
	shuffle(client_no, cptr->gpr_reg,RANGE_GPRS);

	cptr->vsrs[GR].tail[BFP] = cptr->vsrs[GR].head[BFP];
	cptr->vsrs[GR].head[BFP]->vsr_no = 0;
	cptr->vsrs[GR].num_vsrs = 32;
	cptr->vsrs[GR].dirty_mask = 0;
	tmp = cptr->vsrs[GR].head[BFP];
	/* now create the list */
	for(l = 0 ; l < RANGE_GPRS; l++) {
		tmp->vsr_no = cptr->gpr_reg[l];
		gpr_reg_used[l]=1;
		prev_cpu = tmp;
		tmp = tmp->next;
	}
	cptr->vsrs[GR].tail[BFP] = prev_cpu;
	cptr->vsrs[GR].tail[BFP]->next=cptr->vsrs[GR].head[BFP];

	/*
	 * This loop is placed 1st because for quad instructions, 2 registers are required.
	 * Hence most stringent of all allocation.
	 */

	k = 0;
	/* this block is for DFP_QUAD only. */
	{
		DPRINT(cptr->clog, "DFP_QUAD_REG_WT[%d] = %d\n", DFP_QUAD, cptr->vsr_reg_wt[DFP_QUAD]);
		tmp3 = cptr->vsrs[DFP_QUAD].head[DFP];
		prev3 = tmp3;
		j = 0;
		while(j < (cptr->vsr_reg_wt[DFP_QUAD]/2)) {
			if(cptr->vsx_reg[k] < 31 && (cptr->vsx_reg[k] % 2 == 0) && (vsx_reg_used[k] == 0)) {
				int l;
				for(l = 0; l < (NUM_VSRS - 1); l++) {
					if((cptr->vsx_reg[l] == (cptr->vsx_reg[k] + 1)) && (vsx_reg_used[l] == 0)) {
						vsx_reg_used[l] = vsx_reg_used[k] = 1;
						tmp3->vsr_no = cptr->vsx_reg[k];
						DPRINT(cptr->clog,  "%s: DFP_QUAD: vsr no: %d\n", __FUNCTION__, tmp3->vsr_no);
						j++;
						k++;
						prev3 = tmp3;
						tmp3 = tmp3->next;
						break;
					}
				}
				if(l == (NUM_VSRS - 1)) {
					sprintf(msg,"Could not find reg pair for DFP_QUAD");
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					return(1);
				}
			}
			else {
				k++;
				if ( k > (NUM_VSRS - 2)) {
					sprintf(msg,"Using more than 31 registers for DFP_QUAD");
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					return(1);
				}
			}
		}
		cptr->vsrs[DFP_QUAD].num_vsrs = cptr->vsr_reg_wt[DFP_QUAD];
		DPRINT(cptr->clog, "%s: VSR: Initializing %d vsrs for DFP_QUAD type\n",__FUNCTION__, cptr->vsr_reg_wt[DFP_QUAD]);
		FFLUSH(cptr->clog);
		if (cptr->vsr_reg_wt[DFP_QUAD] != 0) {
			cptr->vsrs[DFP_QUAD].tail[DFP] = prev3;
		}
	}


	k=0;
	for(i = DFP_SHORT; i <= DFP_LONG; i++) {
		tmp1 = cptr->vsrs[i].head[DFP];
		prev1 = tmp1;

		DPRINT(cptr->clog,"DFP_REG_WT[%d] = %d\n", i, cptr->vsr_reg_wt[i]);
		FFLUSH(cptr->clog);
		for (j = 0; j < (cptr->vsr_reg_wt[i]); j++) {
			while (cptr->vsx_reg[k] > 31 || (vsx_reg_used[k] == 1)) {
				k++;
				if ( k > (NUM_VSRS - 2)) {
					sprintf(msg,"Using more than 31 registers for DFP");
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					return(1);
				}
			}

			if(vsx_reg_used[k] == 1 ) {
			    /*
			     * Error Condition. If we are hitting this case means wrong weight has been assigned and
			     * we end up using more than 32 registers for BFP instruction. This error needs flagging
			     */
			    sprintf(msg,"Using more than 31 registers for DFP");
			    hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			    return(1);
			}
			else {
				tmp1->vsr_no = cptr->vsx_reg[k];
				DPRINT(cptr->clog,  "op type: %d vsr no: %d\n",i, tmp1->vsr_no);
				FFLUSH(cptr->clog);
				vsx_reg_used[k] = 1;
				k++;
				prev1 = tmp1;
				tmp1 = tmp1->next;
			}
		}
		sdata->cdata_ptr[client_no]->vsrs[i].num_vsrs = cptr->vsr_reg_wt[i];
		DPRINT(cptr->clog, "%s: VSR: Initializing %d vsrs for %d data type\n",__FUNCTION__, cptr->vsr_reg_wt[i], i);
		FFLUSH(cptr->clog);
		if (cptr->vsr_reg_wt[i] != 0) {
			sdata->cdata_ptr[client_no]->vsrs[i].tail[DFP] = prev1;
		}
    }

	k = 0;
	/* this block is for BFP_QP only. */
	{
		tmp1 = cptr->vsrs[BFP_QP].head[BFP];
		prev1 = tmp1;

		DPRINT(cptr->clog,"BFP_REG_WT = %d\n", cptr->bfp_reg_wt[BFP_QP]);
		FFLUSH(cptr->clog);
		for (j = 0; j < (cptr->bfp_reg_wt[BFP_QP]); j++) {
			while (cptr->vsx_reg[k] > 31 || (vsx_reg_used[k] == 1)) {
				k++;
				if ( k > (NUM_VSRS - 2)) {
					sprintf(msg,"Using more than 31 registers for BFP_QP");
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					return(1);
				}
			}

			if(vsx_reg_used[k] == 1 ) {
			    /*
			     * Error Condition. If we are hitting this case means wrong weight has been assigned and
			     * we end up using more than 32 registers for BFP instruction. This error needs flagging
			     */
			    sprintf(msg,"Using more than 31 registers for BFP_QP");
			    hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			    return(1);
			}
			else {
				tmp1->vsr_no = cptr->vsx_reg[k];
				DPRINT(cptr->clog,  "op type: BFP_QP vsr no: %d\n",tmp1->vsr_no);
				FFLUSH(cptr->clog);
				vsx_reg_used[k] = 1;
				k++;
				prev1 = tmp1;
				tmp1 = tmp1->next;
			}
		}
		sdata->cdata_ptr[client_no]->vsrs[BFP_QP].num_vsrs = cptr->bfp_reg_wt[BFP_QP];
		DPRINT(cptr->clog, "%s: Initializing %d vsrs for BFP_QP data type \n",__FUNCTION__, cptr->bfp_reg_wt[BFP_QP]);
		FFLUSH(cptr->clog);
		if (cptr->bfp_reg_wt[BFP_QP] != 0) {
			sdata->cdata_ptr[client_no]->vsrs[BFP_QP].tail[BFP] = prev1;
		}
    }


	/*
	 * This loop will populate SP n DP registers for both VSX n BFP. Half of that VSX SP n DP list will
	 * be filled here, whereas whole BFP SP n DP list will be populated. The remaining VSX list will be
	 * filled in the next for loop.
	 */
	k = 0;
	for(i = SCALAR_SP; i <= SCALAR_DP; i++) {
		tmp1 = sdata->cdata_ptr[client_no]->vsrs[i].head[VSX];
		tmp2 = sdata->cdata_ptr[client_no]->vsrs[i].head[BFP];

		DPRINT(cptr->clog,"BFP_REG_WT[%d] = %d\n", i, cptr->bfp_reg_wt[i]);
		FFLUSH(cptr->clog);
		for (j = 0; j < (cptr->bfp_reg_wt[i]); j++) {
			while (cptr->vsx_reg[k] > 31 || (vsx_reg_used[k] == 1)) {
				k++;
				if ( k > (NUM_VSRS - 2)) {
					sprintf(msg,"Using more than 31 registers for SCALAR: type: %d, bfp_reg_wt[%d]: %d, reg_num: %d\n", i, i, cptr->bfp_reg_wt[i], k);
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					return(1);
				}
			}

			if(vsx_reg_used[k] == 1 ) {
			    /*
			     * Error Condition. If we are hitting this case means wrong weight has been assigned and
			     * we end up using more than 32 registers for BFP instruction. This error needs flagging
			     */
			    sprintf(msg,"Using more than 31 registers for BFP");
			    hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			    return(1);
			}
			else {
				tmp1->vsr_no = cptr->vsx_reg[k];
				tmp2->vsr_no = cptr->vsx_reg[k];
				DPRINT(cptr->clog,  "op type: %d vsr no: %d\n",i, tmp1->vsr_no);
				FFLUSH(cptr->clog);
				vsx_reg_used[k] = 1;
				k++;
				if(i <= SCALAR_DP) {
					prev1 = tmp1;
					prev2 = tmp2;
					tmp1 = tmp1->next;
					tmp2 = tmp2->next;
				}
			}
		}
		sdata->cdata_ptr[client_no]->vsrs[i].num_vsrs = cptr->bfp_reg_wt[i];
		DPRINT(cptr->clog, "%s: BFP: Initializing %d vsrs for %d data type \n",__FUNCTION__, cptr->bfp_reg_wt[i], i);
		FFLUSH(cptr->clog);
		if (cptr->bfp_reg_wt[i] != 0) {
			sdata->cdata_ptr[client_no]->vsrs[i].tail[BFP] = prev2;
			/*
			 * prev1 is pointing to the last valid node in the link list.
			 */
			prev_ary[i] = prev1;
		}
    }

	k=0;
	for(i = VECTOR_SP; i <= QGPR; i++) {
		if (i == VECTOR_DP)
			continue;

		tmp1 = sdata->cdata_ptr[client_no]->vsrs[i].head[VSX];
		tmp2 = sdata->cdata_ptr[client_no]->vsrs[i].head[VMX];

		DPRINT(cptr->clog,"VMX_REG_WT[%d] = %d\n", i, cptr->vmx_reg_wt[i]);
		FFLUSH(cptr->clog);
		for (j = 0; j < (cptr->vmx_reg_wt[i]); j++) {
			while (cptr->vsx_reg[k] < 32 || (vsx_reg_used[k] == 1)) {
				k++;
				if ( k > (NUM_VSRS - 2)) {
					sprintf(msg,"Using more than 31 registers for VMX");
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					return(1);
				}
			}

			if(vsx_reg_used[k] == 1 ) {
	 			/*
				 * Error Condition. If we are hitting this case means wrong weight has been assigned and
				 * we end up using more than 32 registers for BFP instruction. This error needs flagging
				 */
				sprintf(msg,"Using more than 31 registers for VMX");
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(1);
			}
			else {
				tmp1->vsr_no = cptr->vsx_reg[k];
				tmp2->vsr_no = cptr->vsx_reg[k];
				DPRINT(cptr->clog,  "op type: %d vsr no: %d\n",i, tmp1->vsr_no);
				FFLUSH(cptr->clog);
				vsx_reg_used[k] = 1;
				k++;
				prev1 = tmp1;
				prev2 = tmp2;
				tmp1 = tmp1->next;
				tmp2 = tmp2->next;
			}
		}
		sdata->cdata_ptr[client_no]->vsrs[i].num_vsrs = cptr->vmx_reg_wt[i];
		DPRINT(cptr->clog, "%s: VMX: Initializing %d vsrs for %d data type \n",__FUNCTION__, cptr->vmx_reg_wt[i], i);
		FFLUSH(cptr->clog);
		if (cptr->vmx_reg_wt[i] != 0) {
			sdata->cdata_ptr[client_no]->vsrs[i].tail[VMX] = prev2;
			/*
			 * prev1 is pointing to the last valid node in the link list.
			 */
			prev_ary[i] = prev1;
		}
	}

	/* For remaining list of VSX datatypes */
	k=0;
	for(i = SCALAR_SP; i <= VECTOR_HP; i++) {
		switch(i) {
			case SCALAR_SP:
			case SCALAR_DP:
	    		/* This assignment is for SCALAR_SP n SCALAR_DP
				 * We have to take care of these two datatypes
				 * because they at shared between BFP n VSX.
				 */
				if(cptr->bfp_reg_wt[i] != 0) {
					prev1 = prev_ary[i];
					tmp1 = prev_ary[i]->next;
				}
				else {
					prev1 = tmp1 = sdata->cdata_ptr[client_no]->vsrs[i].head[VSX];
				}
				break;

			case VECTOR_SP:
			case QGPR:
	    		/* This assignment is for VECTOR_SP and QGPR
				 * We have to take care of these two datatypes
				 * because they at shared between VSX and VMX.
				 */
				if(cptr->vmx_reg_wt[i] != 0) {
					prev1 = prev_ary[i];
					tmp1 = prev_ary[i]->next;
				}
				else {
					prev1 = tmp1 = sdata->cdata_ptr[client_no]->vsrs[i].head[VSX];
				}
				break;

			default:
				/* This is for rest of datatypes */
				prev1 = tmp1 = sdata->cdata_ptr[client_no]->vsrs[i].head[VSX];
		}

		DPRINT(cptr->clog,"VSX_REG_WT[%d] = %d\n", i, cptr->vsr_reg_wt[i]);
		FFLUSH(cptr->clog);
		for(j = 0; j < cptr->vsr_reg_wt[i]; j++) {
			while (vsx_reg_used[k] == 1) {
				k++;
				if ( k > 62 ) {
					/*
					 * Error Condition. If we are hitting this case means wrong weight has been assigned and
					 * we end up using more than 32 registers for BFP instruction. This error needs flagging
					 */
					sprintf(msg, "VSX: Using more than 62 VSRs, type = %d, reg_wt = %d, reg_num = %d\n", i, j, k);
			    	hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg); 
			    	return(1);
				}
			}
			tmp1->vsr_no = cptr->vsx_reg[k];

			DPRINT(cptr->clog,  "op type: %d vsr no: %d\n",i, tmp1->vsr_no);
			FFLUSH(cptr->clog);
			vsx_reg_used[k] = 1;
			k++;
			prev1 = tmp1;
			tmp1 = tmp1->next;
		}
		if(i == SCALAR_SP || i == SCALAR_DP || i == VECTOR_SP || i == QGPR) {
			sdata->cdata_ptr[client_no]->vsrs[i].num_vsrs += cptr->vsr_reg_wt[i];
			DPRINT(cptr->clog, "%s: VSX: Initializing %d vsrs for %d data type \n",__FUNCTION__, sdata->cdata_ptr[client_no]->vsrs[i].num_vsrs, i);
			FFLUSH(cptr->clog);
		}
		else {
			sdata->cdata_ptr[client_no]->vsrs[i].num_vsrs = cptr->vsr_reg_wt[i];
			DPRINT(cptr->clog, "%s: VSX: Initializing %d vsrs for %d data type \n",__FUNCTION__, sdata->cdata_ptr[client_no]->vsrs[i].num_vsrs, i);
		}
		sdata->cdata_ptr[client_no]->vsrs[i].tail[VSX] = prev1;
	}
	return(0);
}

int
create_context(int client_no)
{
	int rc;
	/*
	 * Initialize GPRs and SPRs
	 */
	 initialize_gprs_n_sprs(client_no);
	/*
	 * Initialize VSRs
	 */
	 rc = initialize_vsrs(client_no);
	 if(rc) {
		 return(rc);
	 }
	 return(0);
}

void cleanup_mem_atexit(void)
{
    cleanup_mem(0);
}

int
cleanup_mem(int type)
{
	int rc, i;
	struct shm_buf *mem;
	struct server_data *sdata;
	struct client_data *cptr;

#ifdef AWAN
			if( nontcsigdata.iar ) {
				dumpsiginfo(-1);
			}

			for(i = 0; i < rule.num_threads; i++) {
				cptr = global_sdata[INITIAL_BUF].cdata_ptr[i];
				if(cptr->sigdata.iar){
					dumpsiginfo(i);
				}
			}
#endif

	if (hlog) {
		fclose(hlog);
		hlog = NULL;
	}
	for (i = 0; i < MAX_NUM_CPUS; i++) {
		sdata = &global_sdata[INITIAL_BUF];
		cptr = sdata->cdata_ptr[i];
		if(client_mem != NULL) {
			if(cptr && (cptr->clog)) {
				fclose(cptr->clog);
				cptr->clog = NULL;
			}
		}
	}
	if ( type == 0 ) {
		for ( i = 0; i < 3; i++) {
			mem = &(vsx_mem[i]);
			if(mem->ptr) {
				rc = shmdt(mem->ptr);
				if(rc) {
					sprintf(msg, "shmdt failed with errno: %d", errno);
					hxfmsg(&hd, rc, HTX_HE_INFO, msg);
				}
				else
					mem->ptr = NULL;
			}
			if(mem->id != -1) {
				rc = shmctl(mem->id, IPC_RMID, (struct shmid_ds *) NULL);
				if(rc != 0) {
					sprintf(msg,"shmctl\'IPC_RMID\' failed with errno: %d", errno);
					hxfmsg(&hd, rc, HTX_HE_INFO, msg);
				}
				else
					mem->id = -1;
			}
		}
	}

	if (client_mem != NULL) {
		free(client_mem);
		client_mem = NULL;
		int i, j;
		for (i = 0; i < 3; i++) {
        	for(j = 0; j < MAX_NUM_CPUS; j++) {
            	global_sdata[i].cdata_ptr[j] = NULL; 
        	}
    	}
	}
	sighandler_exit_flag = 1;
	return(0);
}


int
allocate_mem(struct shm_buf *mem)
{
    unsigned int shm_flag;
    /*key_t key;*/
    /*char *ptr;*/
    int rc, ret;
#ifndef __HTX_LINUX__
    uint64 page_size;
    struct vminfo_psize vminfo = { 0 };
#endif

#ifndef __HTX_LINUX__
	struct shmid_ds sbuf = { 0 };
#endif


	mem->id = -1;
	shm_flag = (IPC_CREAT | IPC_EXCL | S_IRWXU| S_IRWXG | S_IRWXO );
#ifndef __HTX_LINUX__
	sbuf.shm_pagesize = page_size = mem->page_size;
#endif


	rc = shmget(mem->key /* IPC_PRIVATE */, mem->seg_size, shm_flag);
	if ( rc == -1 && errno == EEXIST ) {
		int tmp_rc;
		ret = shmget(mem->key, mem->seg_size, (IPC_CREAT | S_IRWXU| S_IRWXG | S_IRWXO));
		tmp_rc = shmctl(ret, IPC_RMID, (struct shmid_ds *) NULL);
		if(tmp_rc != 0) {
			sprintf(msg,"shmctl \'IPC_RMID\' failed while removing already existing share memory with errno: %d", errno);
			hxfmsg(&hd, rc, HTX_HE_HARD_ERROR, msg);
			return(-1);
		}

		ret = shmget(mem->key, mem->seg_size, shm_flag);

		rc = ret;
	}
	else if(rc == -1) {
		sprintf(msg, "shmget failed for ID = %x with errno: %d\n", mem->key, errno);
		hxfmsg(&hd, rc, HTX_HE_HARD_ERROR, msg);
		return(rc);
	}

	mem->id = rc;
	DPRINT(hlog,  "shm id - %d \t", mem->id);

#ifndef __HTX_LINUX__
	sbuf.shm_pagesize = page_size = mem->page_size;

	if(shmctl(mem->id, SHM_PAGESIZE, &sbuf)) {
		sprintf(msg, "shmctl failed with errno: %d \n", errno);
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		return(-1);
	}
#endif
#ifndef __HTX_LINUX__
	mem->ptr = shmat(mem->id, (char *)0, 0);
#else
#define SHM_EXEC        0100000
		mem->ptr = shmat(mem->id, (char *)0, SHM_EXEC);
#endif
	if(mem->ptr == NULL) {
		sprintf(msg, "shmat failed with errno: %d \n", errno);
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		return(-1);
	}
	else {
		DPRINT(hlog,  "Shared mem ptr - %llx \n", (uint64)mem->ptr);
	}
#ifndef __HTX_LINUX__
	if (mlock(mem->ptr, mem->seg_size) == -1) {
		sprintf(msg, "mlock failed with errno: %d \n", errno);
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		return(-1);
	}
#endif
	return(0);
}

int
build_testcase(uint32 client_no)
{
	int rc = 0;
	uint32 random_no, index, ctr = 0, cat_index, i;
	struct server_data *s = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = s->cdata_ptr[client_no];
	struct ins_category *ic;
	struct instruction_masks *ins;
	uint32 nth_sync_todo=1, num_sync_points =0;;

	cptr->num_ins_built = 0;
	/* is bzero really needed */
	bzero(cptr->instr_index, MAX_INS_STREAM_DEPTH * sizeof(int));

	if( rule.testcase_sync ){
		num_sync_points = rule.stream_depth/rule.sync_distance;
	}

	while (cptr->num_ins_built < rule.stream_depth || nth_sync_todo <= num_sync_points) {
		/*
		 * Generate pseudo random no to pick instruction stream from active instructions array.
		 */
		cat_index = cptr->ins_bias_array[cptr->rand_nos[ctr]];
		ctr++;
		if(ctr == 100) {
			for(i = 0; i < 100; i++) {
			 	cptr->rand_nos[i] = i;
			}
			shuffle(client_no, cptr->rand_nos, 100);
			ctr = 0;
		}
		ic = &cptr->manage_bias_list[cat_index];
		random_no = get_random_no_32(client_no);
		index = (ic->ptr[random_no % ic->end_index]);
		cptr->enabled_ins_table[index].run_flag = 1; /* true */
		ins = &(cptr->enabled_ins_table[index].instr_table);

		/*
		 * check the instructions biasing here to see if its allwed to pick this instruction.
		 */
		switch(ins->ins_class) {
			case CLASS_VSX_LOAD:
				class_vsx_load_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VSX_STORE:
				class_vsx_store_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VSX_TEST_INS:
				class_vsx_test_ins_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VSX_NORMAL:
				class_vsx_normal_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VSX_MUL_ADD_SUB:
				class_vsx_mul_add_sub_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VSX_MOVE:
				class_vsx_move_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VSX_IMM:
				class_vsx_imm_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VSX_LOAD_IMM:
				class_vsx_load_gen2(client_no, random_no, ins, index);
				break;

			case CLASS_VSX_STORE_IMM:
				class_vsx_store_gen2(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_LOAD:
				class_bfp_load_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_LOAD_IMM:
				class_bfp_load_imm_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_STORE:
				class_bfp_store_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_STORE_IMM:
				class_bfp_store_imm_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_NORMAL:
				class_bfp_normal_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_FPSCR:
				class_bfp_fpscr_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_FPSCR_IMM:
				class_bfp_fpscr_imm_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_CR_UPDATE:
				class_bfp_cr_update_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_FPSCR_2_CR:
				class_bfp_fpscr_2_cr_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_FPSCR_BIT_SET_UNSET:
				class_bfp_fpscr_bit_set_unset(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_QP_ROUND:
				class_bfp_qp_round_gen(client_no, random_no, ins, index);
				break;

			case CLASS_BFP_TEST_DATA:
				class_bfp_qp_test_data_class_gen(client_no, random_no, ins, index);
				break;

			case CLASS_DFP_NORMAL:
				class_dfp_normal_gen(client_no, random_no, ins, index);
				break;

			case CLASS_DFP_CMP_TEST_INS:
			case CLASS_DFP_CMP_INS:
				class_dfp_cmp_test_ins_gen(client_no, random_no, ins, index);
				break;

			case CLASS_DFP_RMC:
			case CLASS_DFP_RMC_QUAI:
				class_dfp_qua_rmc_gen(client_no, random_no, ins, index);
				break;

			case CLASS_DFP_SH:
				class_dfp_shift_gen(client_no, random_no, ins, index);
				break;

			case CLASS_DFP_LOAD_IMM:
				class_dfp_load_imm_gen(client_no, random_no, ins, index);
				break;

			case CLASS_DFP_LOAD:
				class_dfp_load_gen(client_no, random_no, ins, index);
				break;

			case CLASS_DFP_STORE_IMM:
				class_dfp_store_imm_gen(client_no, random_no, ins, index);
				break;

			case CLASS_DFP_STORE:
				class_dfp_store_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VMX_LOAD:
				class_vmx_load_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VMX_STORE:
				class_vmx_store_gen(client_no, random_no, ins, index);
				break;

			case CLASS_VMX_NORMAL:
			case CLASS_VMX_NORMAL_3INPUTS:
				class_vmx_normal_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_LOAD:
				class_cpu_load_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_LOAD_1:
				class_cpu_load_1_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_COND_LOG:
				class_cpu_cond_log_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_FIXED_ARTH:
				class_cpu_fixed_arth_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_FIXED_LOGIC_1:
				class_cpu_fixed_logic_1_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_FIXED_LOGIC_2:
				class_cpu_fixed_logic_2_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_FIXED_ARTH_1:
				class_cpu_fixed_arth_1_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_ROTATE:
				class_cpu_rotate_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_ROTATE_1:
				class_cpu_rotate_1_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_ROTATE_2:
				class_cpu_rotate_2_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_ROTATE_3:
				class_cpu_rotate_3_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_FIXED_ARTH_2:
				class_cpu_fixed_arth_2_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_FIXED_ARTH_3:
				class_cpu_fixed_arth_3_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_SPR_1:
				class_cpu_fixed_spr_1_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_SPR_2:
				class_cpu_fixed_spr_2_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_SPR_3:
				class_cpu_fixed_spr_3_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_STORE_1:
				class_cpu_fixed_store_1_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_STORE_2:
				class_cpu_fixed_store_2_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_FIXED_ARTH_4:
				class_cpu_fixed_arth_4_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_MIXED:
				class_cpu_fixed_all_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_MIXED_1:
				class_cpu_fixed_all_1_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_STORE_3:
				class_cpu_fixed_store_3_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_FIXED_LOGIC_3:
				class_cpu_fixed_logic_3_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_CACHE:
				class_cpu_cache_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_CACHE_1:
				class_cpu_cache_1_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_CACHE_2:
				class_cpu_cache_2_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_BRANCH_1:
				class_cpu_branch_1_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_BRANCH_2:
				class_cpu_branch_2_gen(client_no, random_no, ins, index);
				break;

			case CLASS_CPU_BRANCH_3:
				class_cpu_branch_3_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_BRANCH_4:
				class_cpu_branch_4_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_FIXED_LOAD_2:
				class_cpu_fixed_load_2_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_CACHE_3:
				class_cpu_cache_3_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_THREAD_PRI:
				class_cpu_thread_pri_gen(client_no, random_no, ins, index);
				break;
			case CLASS_LHL:
				class_cpu_lhl_gen(client_no, random_no, ins, index);
				break;
			case CLASS_SHL:
				class_cpu_shl_gen(client_no, random_no, ins, index);
				break;
#ifndef SCTU
			case CLASS_CPU_LOAD_ATOMIC:
				class_cpu_load_atomic_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_STORE_ATOMIC:
				class_cpu_store_atomic_gen(client_no, random_no, ins, index);
				break;
#endif
			case CLASS_CPU_LOAD_RELATIVE:
				class_cpu_load_relative_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_STRING_OPS_FX:
				class_cpu_string_operations_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_MUL_ADD_DW:
				class_cpu_mul_add_gen(client_no, random_no, ins, index);
				break;
			case CLASS_CPU_ARRAY_INDEX_SUPP:
				class_cpu_array_index_support_gen(client_no, random_no, ins, index);
				break;
			default:
				sprintf(msg, "%s: Unknown class: %lld\n", __FUNCTION__, ins->ins_class);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
		}

		if( cptr->reinit_base ) { /* 0x7f80 offset has reached last cache line in 32k space covered by addi */
			DPRINT(cptr->clog, "%s: calling reinit_ls_base\n", __FUNCTION__);
			rc = reinit_ls_base( client_no );
			if (rc) {
				return (rc);
			}
		}

		if( rule.testcase_sync ){
			if( nth_sync_todo <= num_sync_points ) {
				if( cptr->num_ins_built >= (rule.sync_distance*nth_sync_todo) ) {
					build_sync_point(client_no);
					nth_sync_todo++;
				}
			}
		}

	#ifdef FPSCR_DEBUG
		{
		uint32 *tc_memory, prolog_size, num_ins_built, store_off;
		struct server_data *sdata = &global_sdata[INITIAL_BUF];
		struct client_data *cptr = sdata->cdata_ptr[client_no];

		prolog_size = cptr->prolog_size;
		num_ins_built = cptr->num_ins_built;
		tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
		*tc_memory = MFFS(0);
		cptr->instr_index[prolog_size + num_ins_built] = mffs | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_mffs;
		tc_memory++;
		num_ins_built++;

		store_off = init_mem_for_vsx_store(client_no, BFP_DP);
#if 0
		*tc_memory = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
		cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		tc_memory++;
		num_ins_built++;
#endif
		*tc_memory = STORE_BFP_DP(0, store_off);
		cptr->instr_index[prolog_size + num_ins_built] = stfd | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_stfd;
		tc_memory++;
		num_ins_built++;
		cptr->num_ins_built = num_ins_built;
		}
	#endif
	}
	return(0);
}

/*
 * This function will initialize the random number generator with the seed from the test case.
 * The buffer used here needs to be declared in the test case datastructure along with seed n
 * the used(&buffer) needs to be replaced with proper call (&sdata->...->tc_ptr->buffer).
 * Also, this function needs to be called ONCE ONLY and not each time random number has to be
 * generated. Thats y its kept as a separate function.
 *
 * srand48_r is a thread safe libc call.
 */
void
init_random_no_generator(uint32 cno)
{
	struct server_data *sdata = &global_sdata[INITIAL_BUF];

	srand48_r(sdata->cdata_ptr[cno]->original_seed, &(sdata->cdata_ptr[cno]->rand_buf));
	DPRINT(sdata->cdata_ptr[cno]->clog, "in %s, client: %d\n", __FUNCTION__, cno);
	FFLUSH(sdata->cdata_ptr[cno]->clog);
}

/*
 * This function generates the actual random number. This is in-turn used in 64-bit random no
 * generator function, which is eventually used in 128-bit random number generator sunroutine.
 *
 * The mrand48_r is a thread safe version of mrand48 and is a libc subroutine. It has a range
 * eventually distributed over [-2^31 to 2^31].
 */
uint32
get_random_no_32(int cno)
{
	long int temp;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];

	mrand48_r(&(sdata->cdata_ptr[cno]->rand_buf), &temp);
	return ((uint32)temp);
}

/*
 * This function generates 64-bit random number. The logic here is to generate 2 32-bit random
 * number using get_random_no_32() and append one after the other.
 */
uint64
get_random_no_64(int cno)
{
	long int tmp;
	uint64 rand_no;

	tmp = get_random_no_32(cno);

	rand_no = ((uint64)tmp << 32);
	rand_no |= get_random_no_32(cno);

	return (rand_no);
}

/* GPR 0 to 8 -> used by the framework for various purposes.GPR9 -> Reserved for future use
 * GPR's 10 to 31 is used by CPU instructions.GPR10 is used to hold addresses of link register and
 * is also used by some instructions like dcbt and dcbtst which holds the address.This address
 * should not be compared since the LSB's are inconsistent across passes.Hence GPR10 is not
 * compared.GPR11 to GPR31 is used for source and target.
 * Every time a register is picked, move the pointer.This is necessary because cpu instructions need more than one
 * register for most instructions.The CPU build routines assume that consecutive two calls to this routine
 * will return two different GPRs
 * Arguments: cno <- client number, type <- GR or GRU, tgt <- is GPR a target or source.? 
 * if it is a target, then store it if dirty and also mark it as dirty..(since it will be loaded.)
 * If it is a source, mark it as clean, since it will be stored.
 */
uint32
get_random_gpr(int cno, int type, int tgt)
{
	uint32 gpr_no = 0;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[cno];
	uint32 num_ins_built, prolog_size, *tc_memory, store_off, mcode;
	struct vsr_list *vsrs;

	vsrs = &cptr->vsrs[GR];
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_memory = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);

	if (type == GRU) {
		type = GR; /* GRU is same as GR */
	}
    vsrs = &cptr->vsrs[GR];
	gpr_no = vsrs->head[BFP]->vsr_no;
	/* Move register to end of list */
	MOVE_VSR_TO_END(cno, type, BFP);
	
	if (tgt) {
		/* Request for register, to be used as target */
		/* if the register is dirty then store it or else just mark it as dirty */
		if ((0x1ULL << gpr_no) & vsrs->dirty_mask) {		
			/* save target register since it is  dirty */
			store_off = init_mem_for_gpr(cno, 8);
			mcode = GEN_ADDI_MCODE(STORE_RB, 0, store_off);
			*tc_memory = mcode;
			cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
			tc_memory++;
			num_ins_built++;

			mcode = STDX(gpr_no, STORE_RA, STORE_RB);
			*tc_memory = mcode;
			cptr->tc_ptr[INITIAL_BUF]->ea_off[prolog_size + num_ins_built] = store_off;
			cptr->instr_index[prolog_size + num_ins_built] = stdx | 0x20000000;
			tc_memory++;
			num_ins_built++;

		}
		/* mark it as dirty */
		vsrs->dirty_mask |= ((0x1ULL << gpr_no));
	}
	cptr->num_ins_built = num_ins_built;
	return (gpr_no);
}
/*
 * This function generates 128-bit random number. Internally it calls 64-bit random no generator
 * two times and stores these two 64-bit numbers in consecutive memory locations. So, if we read
 * 128-bit number starting from the initial address, we get a 128-bit random number.
 */
void
get_random_no_128(int cno, uint64 *rand_no)
{
	*rand_no = get_random_no_64(cno);
	rand_no++;
	*rand_no = get_random_no_64(cno);
}

/*************************************************************************	*
 *	Frxn = Any non-zero value												*
 *	       For biasing, the fraction is masked with 0x000000ff.				*
 *																			*
 ****************************************************************************/
void
sp_ntd(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp = get_random_no_32(cno) & 0x818000ff;
	if(tmp == 0) {
		tmp = get_random_no_32(cno) & 0x818000ff;
	}
	switch(tgt_dtype) {
		case VECTOR_SP:
	*lptr = tmp; lptr++;
	*lptr = tmp; lptr++;
	*lptr = tmp; lptr++;
		case BFP_SP:
	*lptr = tmp; lptr++;
			break;
	}
}
/**************************************************************************
 * SP denormal nos generation
 *
 *
 **************************************************************************/
void
sp_d(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp =  get_random_no_32(cno);
	tmp = tmp & 0x807fffff;
	switch(tgt_dtype) {
		case VECTOR_SP:
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
		case BFP_SP:
			*lptr = tmp; lptr++;
			break;
	}
}
void
sp_dtz(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp =  get_random_no_32(cno);
	tmp = tmp & 0x800000ff;
	if(tmp == 0) {
		tmp =  get_random_no_32(cno);
		tmp = tmp & 0x800000ff;
	}
	switch(tgt_dtype) {
		case VECTOR_SP:
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
		case BFP_SP:
			*lptr = tmp; lptr++;
			break;
	}
}
void
sp_dtn(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp =  get_random_no_32(cno) & 0x807fffff;
	tmp = tmp | 0x007fffc0;
	switch(tgt_dtype) {
		case VECTOR_SP:
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
		case BFP_SP:
			*lptr = tmp; lptr++;
			break;
	}
}

/****************************************************************************
 *	This is for GENERAL NORM NO. generation.								*
 *	Sign = 0 or 1															*
 *	Exp  = 0<exp<EXP_MAX													*
 *	Frxn = Any non-zero value												*
 *																			*
 ****************************************************************************/
void
sp_n(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp = get_random_no_32(cno) & 0xBfffffff;
	tmp = tmp | 0x04000000 ;
	switch(tgt_dtype) {
		case VECTOR_SP:
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
		case BFP_SP:
			*lptr = tmp; lptr++;
			break;
	}

}

/****************************************************************************
 *	This is for NORM NO. TOWARDS INFINITY generation.						*
 *	Sign = 0 or 1															*
 *	Exp  = 0<exp<EXP_MAX													*
 *	Frxn = Any non-zero value												*
 *	       For biasing, the fraction is masked with 0x007fff00.				*
 *																			*
 ****************************************************************************/
void
sp_nti(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp = get_random_no_32(cno);
	tmp = tmp | 0x3fffff00;
	switch(tgt_dtype) {
		case VECTOR_SP:
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
		case BFP_SP:
			*lptr = tmp; lptr++;
			break;
	}
}

/****************************************************************************
 *	This is for Singalling NaN generation.  								*
 *	Sign = 0 or 1															*
 *	Exp  = All 1s															*
 *	Frxn = Any non-zero value with MSB=0									*
 *																			*
 ****************************************************************************/
void
sp_snan(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp = get_random_no_32(cno) & 0xffbfffff;
	tmp =  tmp | 0x7f800000;
	switch(tgt_dtype) {
		case VECTOR_SP:
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
		case BFP_SP:
			*lptr = tmp; lptr++;
			break;
	}
}

/****************************************************************************
 *	This is for Quiet NaN generation.					 					*
 *	Sign = 0 or 1															*
 *	Exp  = All 1s															*
 *	Frxn = Any non-zero value with MSB=1									*
 *																			*
 ****************************************************************************/
void
sp_qnan(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp = get_random_no_32(cno);
	tmp = tmp | 0x7fc00000;
	switch(tgt_dtype) {
		case VECTOR_SP:
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
		case BFP_SP:
			*lptr = tmp; lptr++;
			break;
	}
}

/****************************************************************************
 *	This is for ZERO NO. generation.										*
 *	Sign = 0 or 1															*
 *	Exp  = All 0s															*
 *	Frxn = All 0s															*
 *																			*
 ****************************************************************************/
void
sp_z(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp = 0x00000000;
	switch(tgt_dtype) {
		case VECTOR_SP:
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
		case BFP_SP:
			*lptr = tmp; lptr++;
			break;
	}
}

/****************************************************************************
 *	This is for INFINITY generation.										*
 *	Sign = 0 or 1															*
 *	Exp  = All 1s															*
 *	Frxn = All 0s															*
 *																			*
 ****************************************************************************/
void
sp_i(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp = 0x7f800000;
	switch(tgt_dtype) {
		case VECTOR_SP:
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
			*lptr = tmp; lptr++;
		case BFP_SP:
			*lptr = tmp; lptr++;
			break;
	}
}

void hp_n(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp32;
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp32 = get_random_no_32(cno);
	tmp = tmp32 >> 16;
	tmp &= 0x3fff;  /* Reset topmost exp bit. Make sure its not extremely large number and its output is normal as well. */
	tmp |= 0x1000;	/* Explicitly set 1 exp bit. Makes sure its no Dnorm or NaN */

	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}
		

void hp_d(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp32;
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp32 = get_random_no_32(cno);
	tmp = tmp32 >> 16;

	tmp &= 0x03FF; /* Clear 1st 6 bits. i.e. 1 bit sign and 5 bits exp */
	tmp |= 0x0010; /* Make sure atleast 1 bit is set in fraction part */

	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}

void hp_qnan(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp32;
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp32 = get_random_no_32(cno);
	tmp = tmp32 >> 16;

	tmp |= 0x7E10;	/* Max exp, non-0 fraction. with rightmost fraction bit set */
	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}

void hp_nti(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp32;
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp32 = get_random_no_32(cno);
	tmp = tmp32 >> 16;

	tmp |= 0x3FE0;	/* Max norm exp, big fraction (1/2 fraction bits set) */
	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}

void hp_i(int cno, char *ptr, int tgt_dtype)
{
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp = 0x7C00;
	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}

void hp_snan(int cno, char *ptr, int tgt_dtype)
{
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp = 0x7C10;	/* MAX exp, non-0 fraction with rightmost bit 0 */
	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}

void hp_z(int cno, char *ptr, int tgt_dtype)
{
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp = 0x0000;
	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}

void hp_dtz(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp32;
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp32 = get_random_no_32(cno);
	tmp = tmp32 >> 16;

	tmp &= 0x000f;	/* Retaining only last 4 bits of fraction. */
	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}	


void hp_dtn(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp32;
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp32 = get_random_no_32(cno);
	tmp = tmp32 >> 16;

	tmp |= 0x01F8; /* Set majority bits in fraction. */
	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}	

void hp_ntd(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp32;
	uint16 tmp, *lptr = (uint16 *)ptr;
	int i;

	tmp32 = get_random_no_32(cno);
	tmp = tmp32 >> 16;
	
	tmp &= 0x0C07;
	for ( i=0; i<8; i++ ) {
		*lptr = tmp; lptr++;
	}
}
void
dp_dtz(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp =  get_random_no_64(cno);
	tmp = tmp & 0x800000000000ffffULL;
	if(tmp == 0) {
		tmp =  get_random_no_64(cno);
		tmp = tmp & 0x800000000000ffffULL;
	}
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}
void
dp_d(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp =  get_random_no_64(cno);
	tmp = tmp & 0x800fffffffffffffULL;
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}

void
dp_dtn(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp =  get_random_no_64(cno) & 0x800fffffffffffffULL;
	tmp = tmp | 0x000fffffffff0000ULL;
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}

void
dp_ntd(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp =  get_random_no_64(cno) & 0x80f000000000ffffULL;
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}

void
dp_n(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp =  get_random_no_64(cno) & 0xbfffffffffffffffULL;
	tmp = tmp | 0x0100000000000000ULL;
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}

void
dp_nti(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp =  get_random_no_64(cno);
	tmp = tmp | 0x7f0fffffffff0000ULL;
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}

void
dp_snan(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp =  get_random_no_64(cno) & 0xfff7ffffffffffffULL;
	tmp = tmp | 0x7ff0000000000000ULL;
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}

void
dp_qnan(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp =  get_random_no_64(cno);
	tmp = tmp | 0x7ff8000000000000ULL;
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}

void
dp_z(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = 0x0000000000000000ULL;
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}

void
dp_i(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = 0x7ff0000000000000ULL;
	switch(tgt_dtype) {
		case VECTOR_DP:
			*lptr = tmp; lptr++;
		case SCALAR_DP:
			*lptr = tmp; lptr++;
	}
}

void qp_n(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = get_random_no_64(cno);
	tmp |= 0x0080000000000000ULL;
	*lptr = tmp; lptr++;

	tmp = get_random_no_64(cno);
	*lptr = tmp;
}

void qp_d(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = get_random_no_64(cno);
	tmp &= 0x0000FFFFFFFFFFFFULL;
	*lptr = tmp; lptr++;

	tmp = get_random_no_64(cno);
	*lptr = tmp;
}

void qp_qnan(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = get_random_no_64(cno);
	tmp |= 0x7FFF800000000000ULL;	
	*lptr = tmp; lptr++;

	tmp = get_random_no_64(cno);
	*lptr = tmp;
}

void qp_snan(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = get_random_no_64(cno);
	tmp |= 0x7FFF000000000000ULL;
	*lptr = tmp; lptr++;

	tmp = get_random_no_64(cno);
	*lptr = tmp;
}

void qp_nti(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = get_random_no_64(cno);
	tmp |= 0x7FF0000000000000ULL;
	*lptr = tmp; lptr++;

	tmp = get_random_no_64(cno);
	*lptr = tmp;
}

void qp_i(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = 0x7FFF000000000000ULL;
	*lptr = tmp; lptr++;

	tmp = 0x0ULL;
	*lptr = tmp;
}

void qp_z(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = 0x0ULL;
	*lptr = tmp; lptr++;
	*lptr = tmp;
}

void qp_ntd(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = get_random_no_64(cno);
	tmp &= 0x800FFFFFFFFFFFFFULL;
	*lptr = tmp; lptr++;

	tmp = get_random_no_64(cno);
	*lptr = tmp;
}

void qp_dtz(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp=0x0ULL;
	*lptr = tmp; lptr++;

	tmp = get_random_no_64(cno);
	tmp &= 0x00000000FFFFFFFFULL;
	*lptr = tmp;
}

void qp_dtn(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp = 0x0000FFFFFFFFFFFFULL;
	*lptr = tmp; lptr++;

	tmp = get_random_no_64(cno);
	*lptr = tmp;
}


void
gen_i32(int cno, char *ptr, int tgt_dtype)
{
	uint32 tmp, *lptr = (uint32 *)ptr;

	tmp =  get_random_no_32(cno);
	*lptr = tmp; lptr++;
	*lptr = tmp; lptr++;
	*lptr = tmp; lptr++;
	*lptr = tmp; lptr++;
}

void
gen_i64(int cno, char *ptr, int tgt_dtype)
{
	uint64 tmp, *lptr = (uint64 *)ptr;

	tmp =  get_random_no_64(cno);
	*lptr = tmp; lptr++;
	*lptr = tmp; lptr++;
}

void
gen_i128(int cno, char *ptr, int tgt_dtype)
{
	uint64 *lptr = (uint64 *)ptr;

	get_random_no_128(cno, lptr);
}


int compare_results(int cno, int *num)
{
	int i, rc = 0, data_miscom = 0, max_ls_off;
	uint64 *ptr1, *ptr2, *save_ptr;
#ifdef COMPARE_METHOD_CRC
	uint64 crc1, crc2;
#endif

	struct testcase *tc1, *tc2;
	struct client_data *cptr = global_sdata[INITIAL_BUF].cdata_ptr[cno];

	tc1 = global_sdata[PASS1_BUF].cdata_ptr[cno]->tc_ptr[PASS1_BUF];
	tc2 = global_sdata[PASS2_BUF].cdata_ptr[cno]->tc_ptr[PASS2_BUF];
	DPRINT(cptr->clog,"Thread[%d]: Testxase Addr1: %llx, Testcase Addr2: %llx\n", cno, (uint64)tc1,  (uint64)tc2);

	ptr1 = (uint64 *)tc1->tcc.vsrs;
	ptr2 = (uint64 *)tc2->tcc.vsrs;

	/***********Validate VSRs********/
	for(i = 0; i < NUM_VSRS*2; i++) {
		DPRINT(cptr->clog,"Thread[%d], Pass1 VSR[%d] Addr: %llx, Pass1Value64: %llx, Pass2 VSR[%d] Addr: %llx, Pass2Value64: %llx\n", cno, i, (uint64)ptr1, *ptr1, i, (uint64)ptr2, *ptr2);
		if(*ptr1 != *ptr2) {
			/* Miscompare */
			sprintf(msg, "Miscompare in VSR[%d]: %llx %llx\n", i/2, *ptr1, *ptr2);
			hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
			rc = 1;
			*num = i/2;
			if (rule.enable_attn) {
				attn(0xBEEFDEAD, 1, *ptr1, *ptr2, i, (uint64)ptr1, (uint64)ptr2, (uint64)cptr);
			}
			break;
		}
		ptr1++; ptr2++;
	}

	/***********Validate GPRs********/
	for(i = 11; i < NUM_GPRS; i++) {
		if (tc1->tcc.gprs[i] != tc2->tcc.gprs[i]) {
			/* Miscompare */
			sprintf(msg, "Miscompare in GPR[%d]: %llx %llx \n", i, tc1->tcc.gprs[i], tc2->tcc.gprs[i]);
			hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
			rc = 2;
			*num = i;
			if (rule.enable_attn) {
				attn(0xBEEFDEAD, 2, tc1->tcc.gprs[i], tc2->tcc.gprs[i], i, (uint64)&(tc1->tcc.gprs[0]), (uint64)&(tc2->tcc.gprs[0]), (uint64)cptr);
			}
		}
	}

	/***********Validate FPSCRs********/
	if((tc1->tcc.sprs.fpscr != tc2->tcc.sprs.fpscr) && tc1->tcc.sprs.comp_fpscr) {
		/* Miscompare */
		sprintf(msg, "Miscompare in FPSCR: 0x%llx Vs 0x%llx\n", tc1->tcc.sprs.fpscr, tc2->tcc.sprs.fpscr);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		if (rule.enable_attn) {
			attn(0xBEEFDEAD, 3, tc1->tcc.sprs.fpscr, tc2->tcc.sprs.fpscr, i, (uint64)&(tc1->tcc.sprs.fpscr), (uint64)&(tc2->tcc.sprs.fpscr), (uint64)cptr);
		}
		rc = 3;
	}

	/***********Validate VSCRs********/
	if( tc1->tcc.sprs.vscr[0] != tc2->tcc.sprs.vscr[0] || tc1->tcc.sprs.vscr[1] != tc2->tcc.sprs.vscr[1] ) {
		/* Miscompare */
		sprintf(msg, "Miscompare in VSCR: 0x%016llx%016llx Vs 0x%016llx%016llx\n", tc1->tcc.sprs.vscr[0], tc1->tcc.sprs.vscr[1], tc2->tcc.sprs.vscr[0], tc2->tcc.sprs.vscr[1]);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		if (rule.enable_attn) {
			attn(0xBEEFDEAD, 4, tc1->tcc.sprs.vscr[0], tc2->tcc.sprs.vscr[0], i, (uint64)&(tc1->tcc.sprs.vscr[0]), (uint64)&(tc2->tcc.sprs.vscr[0]), (uint64)cptr);
		}
		rc = 4;
	}

	/***********Validate CRs********/
	if( (tc1->tcc.sprs.cr & 0xffffff) != (tc2->tcc.sprs.cr & 0xffffff) ) {
		/* Miscompare */
		sprintf(msg, "Miscompare in CR: 0x%llx Vs 0x%llx\n", tc1->tcc.sprs.cr, tc2->tcc.sprs.cr);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		if (rule.enable_attn) {
			attn(0xBEEFDEAD, 5, tc1->tcc.sprs.cr, tc2->tcc.sprs.cr, i, (uint64)&(tc1->tcc.sprs.cr), (uint64)&(tc2->tcc.sprs.cr), (uint64)cptr);
		}
		rc = 5;
	}

	max_ls_off = global_sdata[PASS1_BUF].cdata_ptr[cno]->last_ls_off;
#ifdef SCTU
	pthread_mutex_lock(&compare_ls_lock);
	compare_ls_threads++;
	if(compare_ls_threads == rule.num_threads){
		int cnt,client_num;
		max_ls_off = global_sdata[PASS1_BUF].cdata_ptr[0]->last_ls_off;
		for( cnt=1; cnt<rule.num_threads; cnt++){
			if( max_ls_off < global_sdata[PASS1_BUF].cdata_ptr[cnt]->last_ls_off ){
				max_ls_off = global_sdata[PASS1_BUF].cdata_ptr[cnt]->last_ls_off;
				client_num = cnt;
			}
		}
#endif
#ifndef COMPARE_METHOD_CRC
	/***********Validate Load / Store area********/
	ptr1 = (uint64 *)global_sdata[PASS1_BUF].cdata_ptr[cno]->ls_base[PASS1_BUF];
	save_ptr = ptr2 = (uint64 *)global_sdata[PASS2_BUF].cdata_ptr[cno]->ls_base[PASS2_BUF];
	DPRINT(cptr->clog,"LS PASS1 Base Address = %llx, LS PASS2 Base Address = %llx\n", (uint64)global_sdata[PASS1_BUF].cdata_ptr[cno]->ls_base[PASS1_BUF],
																					  (uint64)global_sdata[PASS1_BUF].cdata_ptr[cno]->ls_base[PASS2_BUF]);
	for(i = 0; i < (max_ls_off + 8); i += 8) {
		DPRINT(cptr->clog,"Pass1 LS Addr: %llx, Pass1Value: %llx, Pass2 LS Addr: %llx, Pass2Value: %llx\n",(uint64)ptr1, *ptr1, (uint64)ptr2, *ptr2);
		if(*ptr1 != *ptr2) {
			/* Miscompare */
			DPRINT(cptr->clog,"LS Miscompare: Off = %llx, Pass1 Value = %llx, Pass2 Value = %llx\n",
				  ((uint64)ptr2 - (uint64)save_ptr), *ptr1, *ptr2);
			rc = 6;
			*num = ((uint64)ptr2 - (uint64)save_ptr);
			data_miscom = 1;
			if (rule.enable_attn) {
				attn(0xBEEFDEAD, 6, *ptr1, *ptr2, *num, (uint64)save_ptr, (uint64)ptr1, (uint64)ptr2);
			}
			break;
		}
		ptr1++; ptr2++;
	}
	if(1 == data_miscom){
		sprintf(msg, "Miscompare in Load/Store area.\n");
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
	}
#else
	/***********Validate CRC********/
	ptr1 = (uint64 *)global_sdata[PASS1_BUF].cdata_ptr[cno]->ls_base[PASS1_BUF];
	save_ptr = ptr2 = (uint64 *)global_sdata[PASS2_BUF].cdata_ptr[cno]->ls_base[PASS2_BUF];
	crc1 = 0; crc2 = 0;
	for(i = 0; i < (max_ls_off + 8); i += 8) {
		crc1 = add_logical(crc1, *ptr1); ptr1++;
		crc2 = add_logical(crc2, *ptr2); ptr2++;
	}
	if(crc1 != crc2) {
		DPRINT(cptr->clog, "\n Miscompare (CRC)");
		if (rule.enable_attn) {
			attn(0xBEEFDEAD, 7, crc1, crc2, ptr1, save_ptr, cptr);
		}
		rc = 6;
	}
	if((crc1 != crc2) && (data_miscom == 0) || (data_miscom == 1) && (crc1 == crc2)) {
		DPRINT(cptr->clog,"\n CRC logic bug: crc1 = 0x%llx crc2 = 0x%llx data_miscom = %d \n", crc1, crc2, data_miscom);
	}
#endif
#ifdef SCTU
	compare_ls_threads=0;
	}
	pthread_mutex_unlock(&compare_ls_lock);
#endif
	return(rc);
}

int execute_testcase(int cno, int pass)
{
/*
 * Initialize 64 VSRs according to data biasing inputs from rule file.
 * Initialize FPSCR, MSR:VSX, MSR:FE0,FE1, NIA run_testcase(cno);
 */
	struct client_data *cptr = global_sdata[pass].cdata_ptr[cno];
	struct server_data *sdata;
	int /*ret_flag,*/ i;
	void (*fptr)(struct testcase *, ...);
	char *ptr = (char *)cptr->tc_ptr[pass]->tc_ins;
	char *temp;


	sdata = &global_sdata[pass];
	temp = ((char*)(((char *)(cptr->tc_ptr[pass]->tc_ins)) + cptr->tc_jmp_off));

	for(i = 0; i < (cptr->prolog_size + cptr->num_ins_built + cptr->epilog_size + 10)*4; i += 128) {
		dcbf((volatile unsigned int *)(ptr + i));
	}
	sync();
	ptr = (char *)cptr->tc_ptr[pass]->tc_ins;
	for(i = 0; i < (cptr->prolog_size + cptr->num_ins_built + cptr->epilog_size + 10)*4; i += 128) {
		icbi((volatile unsigned int *)(ptr + i));
	}
	isync();

#ifdef __HTX_LE__
	fptr = (void (*)(struct testcase *, ...))temp;
#else
	fptr = (void (*)(struct testcase *, ...))&temp;
#endif
	DPRINT(cptr->clog, "\n r3 = 0x%llx r5 = 0x%llx\n", (uint64)cptr->tc_ptr[pass], (uint64)cptr->ls_base[pass]);
	FFLUSH(cptr->clog);
	(*fptr)(cptr->tc_ptr[pass], 0, cptr->ls_base[pass], 0, cptr->ls_base[pass], sdata->common_data_ptr->sync_words, cptr->time_stamps, cptr->cpu_id_mask);
	DPRINT(cptr->clog, "\nfpu%d: Returned back from testcase...hurray !!\n", core_num);
	FFLUSH(cptr->clog);
	return (0);
}

void generate_and_set_initial_seeds(int32 parent_seed)
{
	int32 initial_seeds[MAX_NUM_CPUS], threads;
	uint64 tb;

	if (parent_seed == 0) {
#ifndef __HTX_LINUX__
		tb = read_tb();
#else
		read_tb(tb);
#endif
		original_seed = tb & 0xffffffff;
	}
	else {
		original_seed = parent_seed;
	}
	DPRINT(hlog, "%s: original_seed = 0x%08x\n", __FUNCTION__, original_seed);
	FFLUSH(hlog);

	/* use original_seed or parent seed to set seed for client 0 */
	/* client 0 seed is used to generate seed for all clients including client 0 */
	set_seed(0, original_seed);
	init_random_no_generator(0);

	/* generate initial seed for each thread from original_seed */
	for(threads = 0; threads < MAX_NUM_CPUS; threads++) {
		initial_seeds[threads] = get_random_no_32(0);
	}

	for(threads = 0; threads < MAX_NUM_CPUS; threads++) {
		if (parent_seed == 0 && rule.seed[threads] != 0) {
			set_seed(threads, rule.seed[threads]);
		}
		else {
			set_seed(threads, initial_seeds[threads]);
		}
		init_random_no_generator(threads);
	}
}

void
set_seed(int cno, int32 seed)
{
	uint64 tb;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	/*
	 * Store the original seed for each client in the client data structure.
	 */
	if(seed == 0) {
#ifndef __HTX_LINUX__
		tb = read_tb();
#else
		read_tb(tb);
#endif
		sdata->cdata_ptr[cno]->original_seed = (tb & 0xffffffff) + core_num + cno;
	}
	else {
		sdata->cdata_ptr[cno]->original_seed = seed;
	}
	DPRINT(sdata->cdata_ptr[cno]->clog, "In %s: Original seed to = %x\n", __FUNCTION__, sdata->cdata_ptr[cno]->original_seed);
	FFLUSH(sdata->cdata_ptr[cno]->clog);
}
void
apply_rule_file_settings()
{
	struct server_data *s = &global_sdata[INITIAL_BUF];
	struct client_data *cptr;
	int i, k, l;

	for(i = 0; i < rule.num_threads; i++) {
		l = k = 0;
		cptr = s->cdata_ptr[i];
		bzero(cptr->bias_list, sizeof(cptr->bias_list));
		while(k < MAX_BIAS_LIST) {
			if ((rule.ib[i].bias_mask[k][0] & 0x00ffffffffffffffULL) != 0 && rule.ib[i].bias_mask[k][1] != 0) {
				cptr->bias_list[l][0] = rule.ib[i].bias_mask[k][0];
				cptr->bias_list[l][1] = rule.ib[i].bias_mask[k][1];
				l++;
			}
			k++;
		}
	}
	/* mode check for CPU running in correctness method */
	if (rule.test_method == CORRECTNESS_CHECK) {
		for(i = 0; i < rule.num_threads; i++) {
			k = 0;
			cptr = s->cdata_ptr[i];
			while(k < MAX_BIAS_LIST) {
				if ((rule.ib[i].bias_mask[k][0] & INS_CAT) == CPU_ONLY) {
					rule.test_method = CONSISTENCY_CHECK;
					sprintf(msg, "Thread: %d, Incorrect rule file config: changing test_method to consistency for CPU instructions !!!\n", i);
					hxfmsg(&hd, 0, HTX_HE_INFO, msg);
				}
				k++;
			}
		}
	}
}

int
copy_prolog_p6(int cno)
{
	uint32 *ptr, i, *save_ptr, sim_off /*,off_set*/;
	int offset = 0;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[cno];
	/*struct vsr_node *reg;*/

	/*  copy Prolog */
	save_ptr = ptr = (uint32 *)cptr->tc_ptr[INITIAL_BUF]->tc_ins;

	*ptr=ORI(0,0,0);                                    ptr++;
	*ptr=MFSPR(6, 392);                                 ptr++;
	*ptr=STD(6,0,TIME_STAMPS);                          ptr++;
	*ptr=GEN_ADDI_MCODE(TIME_STAMPS, TIME_STAMPS, 8);   ptr++;
	*ptr=LWARX(0,0,SYNC_WORD,0);                        ptr++;
	*ptr=ANDC(0, 0, CPU_ID_MASKS, 0);					ptr++;
	*ptr=STWCX(0,0,SYNC_WORD);                          ptr++;
	*ptr=BC(4,2,(short)-12);							ptr++;
	*ptr=SYNC(0);                                       ptr++;
	*ptr=ISYNC;											ptr++;
	*ptr=MFCR(0);										ptr++;
	*ptr=LWZ(6,SYNC_WORD,0);                            ptr++;
	*ptr=CMPLI(0,0,6,0);                                ptr++;
	*ptr=BC(12,2,(short)16);			                ptr++;
	*ptr=AND(0,6,CPU_ID_MASKS,1);                       ptr++;
	*ptr=BC(12,2,((short)(-16))); 		             	ptr++;
	*ptr=0x1;											ptr++;
	*ptr=ORI(0,0,0);                                    ptr++;
#if 0
	*ptr=ADDIC(SYNC_WORD,SYNC_WORD,4);                  ptr++;
#else
	*ptr=GEN_ADDI_MCODE(SYNC_WORD,SYNC_WORD,4);			ptr++;
#endif
	*ptr=BLR;                                           ptr++;
	*ptr=ORI(0,0,0);                                    ptr++;
	*ptr=ORI(0,0,0);                                    ptr++;
	cptr->tc_jmp_off = ((uint64)ptr - (uint64)save_ptr);
	*ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,0);				ptr++;
	*ptr=MTCRF(0xff, CTX_OFF_REG); 						ptr++;
	*ptr=MFSPR(0,256);                                  ptr++;
	*ptr=STD(0,0x518,TC_BASE);							ptr++;
	*ptr=BL((int)(-((uint64)ptr - (uint64)save_ptr)));	ptr++;
	*ptr=MTSPR(256,0);                                  ptr++;
	*ptr=LFD(1,TC_BASE,SPRS_OFF);                       ptr++;
	*ptr=MTFSF(0xff,1,1,0,0);                           ptr++;
	*ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,SPRS_OFF+0x30);	ptr++;
	*ptr=LVX(1,TC_BASE, CTX_OFF_REG);					ptr++;
    *ptr=MTVSCR(1);                                     ptr++;
	*ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,0);		    ptr++;
	*ptr=MTSPR(32,CTX_OFF_REG);			    ptr++;		/* load zero to xer */
	offset = offsetof(struct testcase,context_gprs);
	/* Store the Current Contents of GPR in context_gprs element of struct testcase */
	for(i =0; i < NUM_GPRS; ++i) {
		*ptr = STD(i,offset,TC_BASE); ptr++;
		offset += 8;
	}
	/* Load the context in to GPRs */
	/* Loading 0 to 8 is Not required because from C function (execute_testcase) args are passed appropriately */
	offset = offsetof(struct testcase,tcc.gprs);
	offset = offset + (8 * 11);						/* Point to GPR10 */
	for(i = 11; i < 32 ; i++) {
		*ptr = LD(i, TC_BASE, offset); ptr++;
		offset += 8;
	}
	cptr->sim_jmp_off = ((uint64)ptr - (uint64)save_ptr)/4;
	sim_off = cptr->sim_jmp_off;
	/* reinitialize..very important */
	offset = 0x0;
    for(i = 0; i < 32; i++) {
		if ( is_reg_sp(i, cno) ) {
#if 0
			cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = &simulate_addi; sim_off++;
            *ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,offset);ptr++;
#endif
            cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_lfs; sim_off++;
            *ptr=LFS(i,TC_BASE,offset);ptr++;
            offset += 16;
        }
        else {
#if 0
			cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = &simulate_addi; sim_off++;
            *ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,offset);ptr++;
#endif
            cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_lfd; sim_off++;
            *ptr=LFD(i,TC_BASE,offset);ptr++;
            offset += 16;
        }
    }

	for ( i = 32; i < 64; i++ ) {
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_addi; sim_off++;
		*ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,offset); ptr++;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_lvx; sim_off++;
		*ptr=LVX(i,TC_BASE,CTX_OFF_REG); ptr++;
		offset += 16;
	}
#if 0
	/* Not required because from C function (execute_testcase) args are passed appropriately */
	offset = 0x420;
    for(i = 4; i < 8; i++) {
		*ptr = LD(i, TC_BASE, offset); ptr++;
		offset += 8;
	}
#endif

	sdata->cdata_ptr[cno]->prolog_size = ((uint64)ptr - (uint64)save_ptr)/4;

#if 0
	/* Debug prints only */
    ptr = save_ptr;
    for(i = 0; i < sdata->cdata_ptr[cno]->prolog_size; i++) {
		DPRINT(log, "\n Prolog[%d] - %x \n",i, *ptr);
		ptr++;
	}
	FFLUSH(hlog);
#endif
	return (0);
}


int
copy_epilog_p6(int cno)
{
	uint32 *ptr, *save_ptr, sim_off, i, offset = 0;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[cno];
    /*struct vsr_node *reg;*/

    /*  copy Epilog */
    save_ptr = ptr = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[cptr->prolog_size + cptr->num_ins_built]);
	sim_off = cptr->prolog_size + cptr->num_ins_built;
	for ( i=0; i<32; i++) {
        cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_stfd; sim_off++;
		*ptr = STFD(i,TC_BASE,offset); ptr++;
		offset += 16;
	}

	for ( i = 32; i < 64; i++ ) {
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_addi; sim_off++;
		*ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,offset); ptr++;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_stvx; sim_off++;
		*ptr=STVX(i,TC_BASE,CTX_OFF_REG); ptr++;
		offset += 16;
	}

	cptr->sim_end_off = sim_off;
	offset = 0x420;
	for( i=4; i<8; i++) {
		*ptr = STD(i, offset, TC_BASE); ptr++;
		offset +=8;
	}

	*ptr = MFFS(1); 									ptr++;
	*ptr = STFD(1,TC_BASE, SPRS_OFF);					ptr++;
    *ptr = MFVSCR(1);                                   ptr++;
	*ptr = GEN_ADDI_MCODE(CTX_OFF_REG,0,SPRS_OFF+0x30); ptr++;
    *ptr = STVX(1,TC_BASE,CTX_OFF_REG);                 ptr++;
	*ptr = MFCR(4);										ptr++;
	/*
	 * Next 3 instructions are to clear out CR0 and CR1 fields before
	 * storing the content of CR into memory.
	 */
	*ptr = ADDIS(0,0,0x00ff);                           ptr++;
	*ptr = ORI(0,0,0xffff);                             ptr++;
	*ptr = AND(4,4,0,0);                                ptr++;
	*ptr = STD(4,(SPRS_OFF+8), TC_BASE);				ptr++; /* SPRS_OFF+8=CR address in tc_context */
	*ptr = MFSPR(6,392);								ptr++;
	*ptr = STD(6, 0, TIME_STAMPS);						ptr++;
	*ptr = GEN_ADDI_MCODE(TIME_STAMPS, TIME_STAMPS, 8); ptr++;
	*ptr = ORI(0,0,0);									ptr++;
	/* Adding this sync point so that all client sync and come out */
	*ptr = BL((int)(-((uint64)ptr - (uint64)cptr->tc_ptr[INITIAL_BUF]->tc_ins)));	ptr++;
	*ptr = LD(0, TC_BASE, 0x518);						ptr++;
	*ptr = MTSPR(256,0);								ptr++;
	offset = offsetof(struct testcase,tcc.gprs);
	offset = offset + (8*11);
	for( i=11; i<NUM_GPRS; ++i) {
		*ptr = STD(i, offset, TC_BASE); ptr++;
		offset +=8;
	}
	/* Store the Current Contents of GPR in context_gprs element of struct testcase */
	offset = offsetof(struct testcase,context_gprs);
	offset  = offset + 80;
	for(i =10; i < NUM_GPRS; ++i) {
		*ptr = LD(i,TC_BASE,offset); ptr++;
		offset += 8;
	}
	*ptr = BLR;											ptr++;
	*ptr = 0x02;										ptr++;
	cptr->epilog_size = ((uint64)ptr - (uint64)save_ptr)/4;
	return (0);
}

int
is_reg_sp(uint32 num, int cno)
{
    struct vsr_node *head = global_sdata[INITIAL_BUF].cdata_ptr[cno]->vsrs[BFP_SP].head[BFP];
    struct vsr_node *tail = global_sdata[INITIAL_BUF].cdata_ptr[cno]->vsrs[BFP_SP].tail[BFP];
	int i;
	for(i = 0; (head != tail); i++) {
		if (num == head->vsr_no) return(1);
		head = head->next;
	}

	if( (head->vsr_no != 0) && (head->vsr_no == num)) return(1);
	return(0);
}

int
copy_prolog_p7(int cno)
{
    uint32 *ptr, i, *save_ptr, sim_off;
    int offset=0;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[cno];
    /*struct vsr_node *reg;*/

    /*  copy Prolog */
    save_ptr = ptr = (uint32 *)sdata->cdata_ptr[cno]->tc_ptr[INITIAL_BUF]->tc_ins;
    *ptr=MFSPR(6, 392);                                 ptr++;
    *ptr=STD(6,0,TIME_STAMPS);                          ptr++;
    *ptr=GEN_ADDI_MCODE(TIME_STAMPS, TIME_STAMPS, 8);   ptr++;
    *ptr=LWARX(0,0,SYNC_WORD,0);                        ptr++;
	*ptr=ANDC(0, 0, CPU_ID_MASKS, 0);					ptr++;
    *ptr=STWCX(0,0,SYNC_WORD);                          ptr++;
    *ptr=BC(4,2,(short)-12);							ptr++;
    *ptr=SYNC(0);                                       ptr++;
    *ptr=ISYNC;											ptr++;
    *ptr=LWZ(6,SYNC_WORD,0);                            ptr++;
    *ptr=CMPLI(0,0,6,0);                                ptr++;
    *ptr=BC(12,2,(short)16);			                ptr++;
    *ptr=AND(0,6,CPU_ID_MASKS,1);                       ptr++;
    *ptr=BC(12,2,((short)(-16))); 		             	ptr++;
    *ptr=0x1;											ptr++;
    *ptr=ORI(0,0,0);                                    ptr++;
#if 0
    *ptr=ADDIC(SYNC_WORD,SYNC_WORD,4);                  ptr++;
#else
	*ptr=GEN_ADDI_MCODE(SYNC_WORD,SYNC_WORD,4);			ptr++;
#endif
    *ptr=BLR;                                           ptr++;
    *ptr=ORI(0,0,0);                                    ptr++;
    *ptr=ORI(0,0,0);                                    ptr++;
    sdata->cdata_ptr[cno]->tc_jmp_off = ((uint64)ptr - (uint64)save_ptr);
    *ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,0);				ptr++;
	*ptr=MTCRF(0xff, CTX_OFF_REG); 						ptr++;
    *ptr=MFSPR(0,256);                                  ptr++;
	*ptr=STD(0,0x518,TC_BASE);							ptr++;
    *ptr=BL((int)(-((uint64)ptr - (uint64)save_ptr)));	ptr++;
    *ptr=MTSPR(256,0);                                  ptr++;
    *ptr=LFD(1,TC_BASE,SPRS_OFF);                       ptr++;
    *ptr=MTFSF(0xff,1,1,0,0);							ptr++;
	*ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,SPRS_OFF+0x30);	ptr++;
	*ptr=LVX(1,TC_BASE, CTX_OFF_REG);					ptr++;
	*ptr=MTVSCR(1);										ptr++;
	*ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,0);		    ptr++;
	*ptr=MTSPR(32,CTX_OFF_REG);			    ptr++;		/* load zero to xer */
	/* Store the Current Contents of GPR in context_gprs element of struct testcase */
	offset = offsetof(struct testcase,context_gprs);
	for(i =0; i < NUM_GPRS; ++i) {
		*ptr = STD(i,offset,TC_BASE); ptr++;
		offset += 8;
	}
	/* Load the context in to GPRs */
	/* Loading 0 to 8 is Not required because from C function (execute_testcase) args are passed appropriately */
	offset = offsetof(struct testcase,tcc.gprs);
	offset = offset + (8 * 11);						/* Point to GPR10 */
	for(i = 11; i < NUM_GPRS ; i++) {
		*ptr = LD(i, TC_BASE, offset); ptr++;
		offset += 8;
	}
	cptr->sim_jmp_off = ((uint64)ptr - (uint64)save_ptr)/4;
	sim_off = cptr->sim_jmp_off;
	offset = 0;
    for(i = 0; i < NUM_VSRS; i++) {
		if ( is_reg_sp(i, cno) ) {
#if 0
			cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = &simulate_addi; sim_off++;
			*ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,offset);ptr++;
#endif
			cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_lfs; sim_off++;
			*ptr=LFS(i,TC_BASE,offset);ptr++;
			offset += 16;
		}
		else {
			cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_addi; sim_off++;
			*ptr=GEN_ADDI_MCODE(CTX_OFF_REG,0,offset); ptr++;
			cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_lxvd2x; sim_off++;
			*ptr=LXVD2X(i,TC_BASE,CTX_OFF_REG); ptr++;
			offset += 16;
		}
    }
    sdata->cdata_ptr[cno]->prolog_size = ((uint64)ptr - (uint64)save_ptr)/4;

#if 0
	offset = 0x420;
    for(i=4; i<8; i++) {
		*ptr = LD(i, TC_BASE, offset); ptr++;
		offset += 8;
	}
#endif

#if 0
    ptr = save_ptr;
    for(i = 0; i < sdata->cdata_ptr[cno]->prolog_size; i++) {
		DPRINT(hlog, "\n Prolog[%d] - %x \n",i, *ptr);
		ptr++;
	}
	FFLUSH(hlog);
#endif

#ifdef SCTU
	cptr->last_rebase_index = cptr->prolog_size;
#endif
	return (0);
}

void adjust_vsrs_memory_image(int cno, int pass)
{
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[cno];
	struct vsr_node /**head,*/ *tail, *tmp;
	char *ptr = (char *) cptr->tc_ptr[pass]->tcc.vsrs;
	uint64 *ptr64;
	int data_type;


	for(data_type = BFP_SP; data_type <= SCALAR_DP; data_type++) {
		tmp = cptr->vsrs[data_type].head[VSX];
		tail = cptr->vsrs[data_type].tail[VSX];
		while (tmp != NULL && tmp != tail) {
			ptr64 = (uint64 *)(ptr + (tmp->vsr_no * 16) + 8);
			*ptr64 = 0;
			tmp = tmp->next;
		}
		ptr64 = (uint64 *)(ptr + (tmp->vsr_no * 16) + 8);
		*ptr64 = 0;
	}

	/*
	 * This loop is for clearing bits 64-127 of DFP registers.
	 */
	for(data_type = DFP_SHORT; data_type <= DFP_QUAD; data_type++) {
		tmp = cptr->vsrs[data_type].head[DFP];
		tail = cptr->vsrs[data_type].tail[DFP];
		while (tmp != NULL && tmp != tail) {
			ptr64 = (uint64 *)(ptr + (tmp->vsr_no * 16) + 8);
			*ptr64 = 0ULL;
			if ( data_type == DFP_QUAD ) {
				/* Next reg of even-odd pair */
				ptr64 = (uint64 *)(ptr + (tmp->vsr_no * 16) + 24);
				*ptr64 = 0ULL;
			}
			tmp = tmp->next;
		}
		ptr64 = (uint64 *)(ptr + (tmp->vsr_no * 16) + 8);
		*ptr64 = 0ULL;
		if ( data_type == DFP_QUAD ) {
			/* Next reg of even-odd pair */
			ptr64 = (uint64 *)(ptr + (tmp->vsr_no * 16) + 24);
			*ptr64 = 0ULL;
		}
	}
}
int
copy_epilog_p7(int cno)
{
	uint32 *ptr, *save_ptr, offset = 0, i, sim_off;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[cno];
    /*struct vsr_node *reg;*/

    /*  copy Epilog */
    save_ptr = ptr = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[cptr->prolog_size + cptr->num_ins_built]);
	sim_off = cptr->prolog_size + cptr->num_ins_built;

	for(i = 0; i < NUM_VSRS; i++) {
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_addi; sim_off++;
		*ptr = GEN_ADDI_MCODE(CTX_OFF_REG,0,offset); ptr++;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[sim_off] = (sim_fptr)&simulate_stxvd2x; sim_off++;
		*ptr = STXVD2X(i,TC_BASE,CTX_OFF_REG); ptr++;
		offset += 16;
    }
    cptr->sim_end_off = sim_off;
	offset = 0x420;
	for( i=4; i<8; i++) {
		*ptr = STD(i, offset, TC_BASE); ptr++;
		offset +=8;
	}

	*ptr = MFFS(1); 									ptr++;
	*ptr = STFD(1,TC_BASE, offsetof(struct testcase, tcc.sprs.fpscr));					ptr++;
	*ptr = MFVSCR(1);									ptr++;
	*ptr = GEN_ADDI_MCODE(CTX_OFF_REG,0,offsetof(struct testcase, tcc.sprs.vscr[0]));	ptr++;
	*ptr = STVX(1,TC_BASE,CTX_OFF_REG);					ptr++;
	*ptr = MFCR(4);										ptr++;
	/*
	 * Next 3 instructions are to clear out CR0 and CR1 fields before
	 * storing the content of CR into memory.
	 */
	*ptr = ADDIS(0,0,0x0fff);                           ptr++;
	*ptr = ORI(0,0,0xffff);                             ptr++;
	*ptr = AND(4,4,0,0);                                ptr++;
	*ptr = STD(4,(SPRS_OFF+8), TC_BASE);				ptr++; /* SPRS_OFF+8=CR address in tc_context */
	/* Store the Current Contents of GPR in context_gprs element of struct testcase */
	offset = offsetof(struct testcase,tcc.gprs);
	offset = offset + (8*11);
	for( i=11; i<NUM_GPRS; ++i) {
		*ptr = STD(i, offset, TC_BASE); ptr++;
		offset +=8;
	}
	*ptr = MFSPR(6,392);								ptr++;
	*ptr = STD(6, 0, TIME_STAMPS);						ptr++;
	*ptr = GEN_ADDI_MCODE(TIME_STAMPS, TIME_STAMPS, 8); ptr++;
	*ptr = ORI(0,0,0);									ptr++;
	/* Adding this sync point so that all client sync and come out */
	*ptr = BL((int)(-((uint64)ptr - (uint64)cptr->tc_ptr[INITIAL_BUF]->tc_ins)));	ptr++;
	*ptr = LD(0, TC_BASE, 0x518);						ptr++;
	*ptr = MTSPR(256,0);								ptr++;
	/* Store the Current Contents of GPR in context_gprs element of struct testcase */
	offset = offsetof(struct testcase,context_gprs);
	offset  = offset + 80;
	for(i =10; i < NUM_GPRS; ++i) {
		*ptr = LD(i,TC_BASE,offset); ptr++;
		offset += 8;
	}
	*ptr = BLR;											ptr++;
	*ptr = 0x02;										ptr++;
	cptr->epilog_size = ((uint64)ptr - (uint64)save_ptr)/4;
	return (0);
}

#ifndef __HTX_LINUX__
int
bind_thread(uint32 cno)
{
	int rc;

	rc = bindprocessor(BINDTHREAD, thread_self(), cno);
    if(rc == -1) {
		if (( errno == EINVAL ) && ( cno >= _system_configuration.ncpus )) {
			sprintf(msg, "Bind failed on non existent cpu: %d.\n", cno);
			hxfmsg(&hd, errno, HTX_HE_INFO, msg);
			hxfupdate(RECONFIG_CPUFAIL, &hd);
			exit_flag = 1;
		} else {
			sprintf(msg, "bindprocessor() failed on cpu:%d with errno %d(%s)\n", cno, errno, strerror(errno));
			hxfmsg(&hd, errno, HTX_HE_HARD_ERROR, msg);
		}
    }
    return(rc);
}

int
unbind_thread(uint32 cno)
{
	int rc = 0;

	pthread_t thread_id = global_sdata[INITIAL_BUF].cdata_ptr[cno]->tid;
	if ( thread_id != 0 ) {
		rc = bindprocessor(BINDPROCESS, getpid() , PROCESSOR_CLASS_ANY);
		if(rc) {
		#if 0
			if ( errno == ESRCH ) {
				/* This means the thread we are trying to unbind does not exist.
				 * Which means that the thread already exited or is not created yet.
				 * Its safe to return success here.
				 */
				return(0);
			}
		#endif
			return(errno);
		}
	}
	return(rc);
}
#endif

int
parse_line(char s[])
{
	int len, i = 0, j = 0;

	while(s[i] == ' ' || s[i] == '\t') {
		i++;
	}
	if(s[i] == '*') {
		return(0);
	}
	len = strlen(s);
	for(; i < len && s[i] != '\0'; i++) {
		s[j++] = s[i];
	}
	s[j] = '\0';
	return((s[0] == '\n')? 1 : j);
}

int
get_line( char s[], int lim, FILE *fp, char delimiter, int *line)
{
	int c = 0, i = 0, rc;

	while (1) {
		c = fgetc(fp);
		if (c != EOF && c != '\n' && c != delimiter && i < lim) {
			if (c == '=') {
				s[i++] = ' ';
			}
			else {
				s[i++] = c;
			}
		}
		else {
			if (c == delimiter || c == EOF) break;
			else if (c == '\n' && i < lim) {
				*line = *line + 1;
				continue;
			}
			else if (i >= lim) {
				sprintf(msg, "line#: %d. Too big line to fit in !!", *line);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-3);
			}
		}
	}
	if(c == EOF && feof(fp)) {
		if(c == EOF && i == 0) return(-2);
		s[i] = '\0';
	}
	else if(c == EOF && ferror(fp)) {
		return(-1);
	}
	else if(c == delimiter) {
		s[i++] = c;
		s[i] = '\0';
	}
	else if (i >= lim) {
		s[i-1] = '\0';
	}
	rc = parse_line(s);
	return(rc);
}

#define MAX_RULE_LINE_SIZE 2000
/* #define mprintf printf */
/*
 * Returns:
 * +ve integer  - End of stanza
 * -1 - Error
 * -2 - EOF
 */
int
get_rule(int *line, FILE *fp, struct ruleinfo *r)
{
	char  s[MAX_RULE_LINE_SIZE], keywd[80];
	int   rc, keywd_count = 0;

	/*
	 * Loop will break if EOF is received.
	 */
	while(1) {
		rc = get_line(s, MAX_RULE_LINE_SIZE, fp, '\n', line);
		/*
		 * rc = 0  indicates comment in a rule file
		 * rc = 1  indicates only '\n' (newline char) on the line, i.e. change in stanza
		 * rc > 1  more characters, i.e. go ahead and check for valid parameter name.
		 * rc = -1 error while reading file.
		 * rc = -2 End of File.
	 	 */
		if(rc == 0) {
			*line = *line + 1;
			continue;
		}
		else if(rc == 1) {
			return(keywd_count);
		}
		else if(rc == -1 || rc == -3) return(-1);
		else if(rc == -2 && keywd_count > 0) return(keywd_count);
		else if(rc == -2 && keywd_count == 0) return(-2);

		*line = *line + 1;


		sscanf(s,"%s", keywd);

		if ((HSTRCMP(keywd,"RULE_ID")) == 0) {
			sscanf(s,"%*s %s",r->rule_id);
			if ((strlen(r->rule_id)) > 20) {
				sprintf(msg, "line# %d %s = %s must be 16 characters or less", *line, keywd, r->rule_id);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %s", keywd, r->rule_id);
		}
        else if ((HSTRCMP(keywd,"NUM_OPER")) == 0) {
			sscanf(s,"%*s %d", &r->num_oper);
			if (r->num_oper == 0) {
				sprintf(msg, "Warning ! num_oper = 0. stanza will run forever.");
				hxfmsg(&hd, 0, HTX_HE_INFO, msg);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->num_oper);
		}
		else if ((HSTRCMP(keywd,"THREADS_SYNC")) == 0) {
			sscanf(s,"%*s %d", &r->testcase_sync);
			if (r->testcase_sync != 0 && r->testcase_sync != 1) {
				sprintf(msg, "line# %d %s = %d must be either 0 or 1", *line, keywd, r->testcase_sync);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->testcase_sync);
		}
		else if ((HSTRCMP(keywd,"SYNC_DISTANCE")) == 0) {
			sscanf(s,"%*s %d", &r->sync_distance);
			if (r->sync_distance < 1) {
				sprintf(msg, "line# %d %s = %d must be positive integer",*line, keywd, r->sync_distance);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->sync_distance);
		}
		else if ((HSTRCMP(keywd,"NUM_THREADS")) == 0) {
			sscanf(s,"%*s %d", &r->num_threads);
			if (r->num_threads > MAX_NUM_CPUS) {
				sprintf(msg, "line# %d %s = %d must be positive integer(<= smt) or 0",*line, keywd, r->num_threads);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->num_threads);
		}
		else if ((HSTRCMP(keywd,"UNALIGNED_LOADS_STORES_%")) == 0) {
			char *chr_ptr;
			uint32 index = 0;
			if(strtok(s, "[")) {
				while(index < MAX_NUM_CPUS && (chr_ptr = strtok(NULL, ",]")) != NULL) {
					errno = 0;
					r->unaligned_data_pc[index] = atoi(chr_ptr);
					if(errno) {
						sprintf(msg, "On line#: %d, atoi failed with %d", *line, errno);
						hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
						return(-1);
					}
					else {
						mprintf("\n Param Name: %s Val: %d", keywd, r->unaligned_data_pc[index]);
						if (r->unaligned_data_pc[index] < 0 || r->unaligned_data_pc[index] > 100) {
							sprintf(msg, "line# %d %s = %d must be between 0 and 100",*line, keywd, r->unaligned_data_pc[0]);
							hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
							return(-1);
						}
					}
					index++;
				}
			}
			else {
				sprintf(msg, "Improper format in line# %d. Couldnt find [ char.", *line);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
		}
		else if ((HSTRCMP(keywd,"PARENT_SEED")) == 0) {
			sscanf(s,"%*s %x", &r->parent_seed);
			mprintf("\n Param Name: %s Val: 0x%x", keywd, r->parent_seed);
		}
		else if ((HSTRCMP(keywd,"SEED")) == 0) {
			char *chr_ptr, *end_ptr;
			uint32 index = 0;
			if(strtok(s, "[")) {
				while(index < MAX_NUM_CPUS && (chr_ptr = strtok(NULL, ",]")) != NULL) {
					errno = 0;
					r->seed[index] = strtol(chr_ptr, &end_ptr, 16);
					if(errno) {
						sprintf(msg, "On line#: %d, strtol failed with %d", *line, errno);
						hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
						return(-1);
					}
					else {
						mprintf("\n Param Name: %s Val: 0x%08x", keywd, r->seed[index]);
					}
					index++;
				}
			}
			else {
				sprintf(msg, "Improper format in line# %d. Couldnt find [ char.", *line);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
		}
		else if ((HSTRCMP(keywd,"DUMP_TESTCASE")) == 0) {
			char *chr_ptr;
			uint32 index = 0;
			if(strtok(s, "[")) {
				while(index < MAX_NUM_CPUS && (chr_ptr = strtok(NULL, ",]")) != NULL) {
					errno = 0;
					r->dump_testcase[index] = atoi(chr_ptr);
					if(errno) {
						sprintf(msg, "On line#: %d, atoi failed with %d", *line, errno);
						hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
						return(-1);
					}
					else {
						mprintf("\n Param Name: %s Val: 0x%08x", keywd, r->dump_testcase[index]);
					}
					index++;
				}
			}
			else {
				sprintf(msg, "Improper format in line# %d. Couldnt find [ char.", *line);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
		}
        else if ((HSTRCMP(keywd,"PAGE_SIZE")) == 0) {
			sscanf(s,"%*s %d", &r->page_size);
			if (r->page_size != 4096 && r->page_size != (16*1024*1024) && r->page_size != (64*1024)) {
				sprintf(msg, "line# %d %s = %d must be 4K, 64K or 16M",*line, keywd, r->page_size);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->page_size);
		}
		else if ((HSTRCMP(keywd,"STREAM_DEPTH")) == 0) {
			sscanf(s,"%*s %d", &r->stream_depth);
			if ((r->stream_depth == 0) || (r->stream_depth >= (MAX_INS_STREAM_DEPTH - 1024))) {
				sprintf(msg, "line# %d %s = %d must be between 0 and %d", *line, keywd, r->stream_depth, (MAX_INS_STREAM_DEPTH - 1024));
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->stream_depth);
		}
		else if ((HSTRCMP(keywd,"TEST_METHOD")) == 0) {
			sscanf(s,"%*s %d", &r->test_method);
			if (r->test_method != CONSISTENCY_CHECK && r->test_method != CORRECTNESS_CHECK) {
				sprintf(msg, "line# %d %s = %d must be either CORRECTNESS_CHECK or CONSISTENCY_CHECK", *line, keywd, r->test_method);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->test_method);
		}
		else if ((HSTRCMP(keywd,"FPSCR")) == 0) {
			char *chr_ptr, *end_ptr;
			uint32 index = 0;
			if(strtok(s, "[")) {
				while(index < MAX_NUM_CPUS && (chr_ptr = strtok(NULL, ",]")) != NULL) {
					errno = 0;
					r->fpscr[index] = strtoul(chr_ptr, &end_ptr, 16);
					if(errno) {
						sprintf(msg, "On line#: %d, strtoul failed with %d", *line, errno);
						hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
						return(-1);
					}
					else {
						mprintf("\n Param Name: %s Val: %llx", keywd, r->fpscr[index]);
					}
					index++;
				}
			}
			else {
				sprintf(msg, "Improper format in line# %d. Couldnt find [ char.", *line);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
		}
		else if ((HSTRCMP(keywd,"DATA_BIAS_MASK")) == 0) {
			char *chr_ptr, *end_ptr;
			uint32 index = 0;
			uint64 tmp;
			if(strtok(s, "[")) {
				while(index < MAX_NUM_CPUS && (chr_ptr = strtok(NULL, ",]")) != NULL) {
					errno = 0;
					tmp = strtoul(chr_ptr, &end_ptr, 16);
					if(errno) {
						sprintf(msg, "On line#: %d, strtoul failed with %d", *line, errno);
						hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
						return(-1);
					}
					else {
						mprintf("\n Param Name: %s Val: %llx", keywd, tmp);
						int i;
						for(i = 0; i < BFP_DATA_BIAS_TYPES; i++) {
							r->db[index][i] = (tmp >> (i*4)) & (uint64)0xf;
							mprintf("\n Param Name: %s data type: %d Val: %d", keywd, i, r->db[index][i]);
						}
					}
					index++;
				}
			}
			else {
				sprintf(msg, "Improper format in line# %d. Couldnt find [ char.", *line);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
		}
		else if ((HSTRCMP(keywd,"INS_BIAS_MASK")) == 0) {
			char *chr_ptr, *end_ptr, tmp_line[MAX_RULE_LINE_SIZE], *str1;
			uint32 index = -1, thread_num;
			uint64 tmp;
#if 0
			rc = get_line(tmp_line, MAX_RULE_LINE_SIZE, fp, ']', line);
			/*
			 * rc = 0  indicates comment in a rule file
			 * rc = 1  indicates only '\n' (newline char) on the line, i.e. change in stanza
			 * rc > 1  more characters, i.e. go ahead and check for valid parameter name.
			 * rc = -1 system call error while reading file.
			 * rc = -2 End of File.
			 * rc = -3 Buffer Overflow
			 */
			if(rc == 0) {
				*line = *line + 1;
				continue;
			}
			else if(rc == 1) {
				return(keywd_count);
			}
			else if(rc == -1 || rc == -3) return(-1);
			else if(rc == -2) return(-2);
			strcat(s, tmp_line);
#endif
			str1 = s;
			chr_ptr = strsep(&str1, "[");
			if(str1 != NULL) {
				char tok_ary[MAX_NUM_CPUS][MAX_RULE_LINE_SIZE], *s1, *s2;

				s1 = strsep(&str1, "]");
				if(str1 != NULL) {
					strcpy(tmp_line, s1);
				}
				else {
					sprintf(msg, "Improper format in line# %d. Couldnt find char ']'", *line);
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					return(-1);
				}
				s2 = tmp_line;
				while(1) {
					chr_ptr = strsep(&s2, ":");
					if(s2 == NULL) {
						index++;
						if (index >= MAX_NUM_CPUS) break;
						strcpy(&tok_ary[index][0], chr_ptr);
						mprintf("tok_ary[%d] = %s %llx s2 = %llx\n", index, &tok_ary[index][0], (uint64)chr_ptr, (uint64)s2);
						break;
					}
					index++;
					if (index >= MAX_NUM_CPUS) break;
					strcpy(&tok_ary[index][0], chr_ptr);
					mprintf("tok_ary[%d] = %s %llx s2 = %llx\n", index, &tok_ary[index][0], (uint64)chr_ptr, (uint64)s2);
				}
				if (index >= MAX_NUM_CPUS) {
					sprintf(msg, "line# %d: Bias rules for more than %d cpu threads", *line, MAX_NUM_CPUS);
					hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
					return(-1);
				}
				for(thread_num = 0; thread_num  <= index; thread_num++) {
					char *ptr;
					uint32 mask_no, tokens;

					ptr = &tok_ary[thread_num][0];
					mask_no = 0;
					while (mask_no < MAX_BIAS_LIST) {
						mprintf("ptr = %s\n", ptr);
						chr_ptr = strsep(&ptr, "(");
						if (ptr != NULL) {
							tokens = 0;
							while (tokens < 2) {
								uint64 tmp1, mask;
								errno = 0;

								mprintf("ptr = %s\n", ptr);
								chr_ptr = strsep(&ptr, ",");
								if (ptr != NULL) {
									tmp = strtoul(chr_ptr, &end_ptr, 16);
									if(errno) {
										sprintf(msg, "On line#: %d, strtoul failed with %d", *line, errno);
										hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
										return(-1);
									}
									tokens++;
									tmp1 = tmp & (0xffULL << 56);
									mask = tmp;
									mprintf("ptr = %s\n", ptr);
									chr_ptr = strsep(&ptr, ")");
									if (ptr != NULL) {
										mprintf("ptr = %s\n", ptr);
										tmp = atoi(chr_ptr);
										if(errno) {
											sprintf(msg, "On line#: %d, atoi failed with %d", *line, errno);
											hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
											return(-1);
										}
										tokens++;
										if(tmp1 == VSX_ONLY || tmp1 == BFP_ONLY || tmp1 == DFP_ONLY || tmp1 == VMX_ONLY || tmp1 == CPU_ONLY || tmp1 == MACRO_ONLY)  {
											r->ib[thread_num].bias_mask[mask_no][0] = mask;
											if ((mask & P9_ONLY) && (shifted_pvr_os < SHIFTED_PVR_OS_P9)) {
												sprintf(msg, "P9_ONLY flag set for hardware: %X,  mask value: 0x%llx in line# %d.", shifted_pvr_os, mask, *line);
                                            	hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
												return(-1);
											}
											r->ib[thread_num].bias_mask[mask_no][1] = tmp;
											mask_no++;
										}
										else {
											sprintf(msg, "Unsupported mask value: 0x%llx in line# %d.", mask, *line);
											hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
											return(-1);
										}
										mprintf("Mask: %llx wt: %lld mask_no = %d\n", mask, tmp, mask_no);
									}
									else {
										sprintf(msg, "Improper format in line# %d. Couldnt find ')' char.", *line);
										hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
										return(-1);
									}
								}
								else {
									sprintf(msg, "Improper format in line# %d. str: %s. Couldnt find ',' char.", *line, chr_ptr);
									hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
									return(-1);
								}
							}
						}
						else {
							if (mask_no > 0) {
								break;
							}
							else {
								sprintf(msg, "Improper format in line# %d. str: %s. Couldnt find '(' char.", *line, chr_ptr);
								hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
								return(-1);
							}
						}
					}
				}
			}
			else {
				sprintf(msg, "Improper format in line# %d. Couldnt find ']' char.", *line);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
		}
		else if ((HSTRCMP(keywd,"PROT_SAO")) == 0) {
			sscanf(s,"%*s %d", &r->prot_sao);
			if (r->prot_sao != 0 && r->prot_sao != 1) {
				sprintf(msg, "line# %d %s = %d must be either 0 or 1", *line, keywd, r->test_method);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->prot_sao);
		}
		else if ((HSTRCMP(keywd,"TESTCASE_TYPE")) == 0) {
			char local_str[20];
			sscanf(s,"%*s %s", local_str);
			if ( strncasecmp(local_str, "default", 7) == 0 ) {
				r->testcase_type = SYSTEM_TEST;
			}
			else if ( strncasecmp(local_str, "chip", 4) == 0 ) {
				r->testcase_type = CHIP_TEST;
			}
			else if ( strncasecmp(local_str, "node", 4) == 0 ) {
				r->testcase_type = NODE_TEST;
			}
			else if ( strncasecmp(local_str, "internode", 9) == 0 ) {
				r->testcase_type = INTERNODE_TEST;
			}
			else {
				sprintf(msg, "line#%d %s has invalid value. Possible values are default/chip/node/internode. Value provided = %s", *line, keywd, local_str);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->testcase_type);
		}
		else if ((HSTRCMP(keywd,"ENABLE_ATTN")) == 0) {
			sscanf(s,"%*s %d", &r->enable_attn);
			if (r->enable_attn != 0 && r->enable_attn != 1) {
				sprintf(msg, "line# %d %s = %d must be either 0 or 1", *line, keywd, r->enable_attn);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->enable_attn);
		}
		else if ((HSTRCMP(keywd,"ENABLE_TRAP")) == 0) {
			sscanf(s,"%*s %d", &r->enable_trap);
			if (r->enable_trap != 0 && r->enable_trap != 1) {
				sprintf(msg, "line# %d %s = %d must be either 0 or 1", *line, keywd, r->enable_trap);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->enable_trap);
		}
		else if ((HSTRCMP(keywd,"NUM_PASS2")) == 0) {
			sscanf(s,"%*s %d", &r->num_pass2);
			if (r->num_pass2 == 0) {
				r->num_pass2 = 1;
			}
			mprintf("\n Param Name: %s Val: %d", keywd, r->num_pass2);
		}
		else if ((HSTRCMP(keywd, "LOG_SEED")) == 0) {
			sscanf(s,"%*s %d", &r->log_seed);
			mprintf("Param Name: %s Val: %d\n", keywd, r->log_seed);
		}
		else if ((HSTRCMP(keywd,"TC_TIME")) == 0) {
			sscanf(s,"%*s %d", &r->tc_time);
			mprintf("Param Name: %s Val: %d\n", keywd, r->tc_time);
		}
		else if ((HSTRCMP(keywd,"COMPARE")) == 0) {
			sscanf(s,"%*s %d", &r->compare_flag);
			mprintf("Param Name: %s Val: %d\n", keywd, r->compare_flag);
		}
		else {
			sprintf(msg, "line# %d Unsupported keyword: %s", *line, keywd);
			hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			return(-1);
		}
		keywd_count++;
	}
	return(0);
}


int
get_PVR()
{
	/*
	 * In this function we will call AIX getPvr() function loaded in kernel extension. This function will give us the
	 * deatil of actual hardware that the code is running on. Depending on the hardware various function pointers
	 * are assinged to the platform specific implementation of that function. Here is a list of platform specific
	 * functions that needs to be handled specially here.
	 */

#ifdef __HTX_LINUX__
	__asm __volatile ("mfspr %0, 287" : "=r" (pvr));

	#ifdef AWAN
	printf("PVR = 0x%08x\n", pvr);
	fflush(stdout);
	#endif

	shifted_pvr_hw = pvr >> 16;
#else
	pvr = (unsigned int) getTruePvr();
	shifted_pvr_hw = pvr >> 16;
#endif

	/* This is done only because P7+ and P7 cores are identical.
	 * So, on P7+, FPU will behave as running on P7 hardware and will decide the OS PVR value accordingly.
	 * Needs to be taken care in future for P8 n above separately.
	 * This will affect simulation at a lot of places. So be _careful_ while making changes for P8 & above.
	 */
	if ( shifted_pvr_hw == 0x4a ) {
		shifted_pvr_hw = 0x3f;
	}


#ifndef __HTX_LINUX__
	shifted_pvr_os = (unsigned int) (getPvr() >> 16);
#else
	shifted_pvr_os = (get_cpu_version() >> 16);
	if ( shifted_pvr_os == -1 ) {
		sprintf(msg,"%s[%d]:get_cpu_version returned -1. Unsupported architecture.\n", __FUNCTION__, __LINE__);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		return(-1);
	}
#endif

    sprintf(msg, "PVR details: \npvr = 0x%x , shifted_pvr_hw = 0x%x , shifted_pvr_os = 0x%x\n ", pvr, shifted_pvr_hw, shifted_pvr_os);
	hxfmsg(&hd, 0, HTX_HE_INFO, msg);


	if(shifted_pvr_os <= 0x3e) { /* Its a P6 or lower */
		copy_prolog_fptr = &copy_prolog_p6;
		copy_epilog_fptr = &copy_epilog_p6;
		st_fptrs[BFP_SP]= &func_store_64_p6;
		st_fptrs[BFP_DP]= &func_store_64_p6;
	}
	else if (shifted_pvr_os >= 0x3f) { /* Its a P7 or higher */
		copy_prolog_fptr = &copy_prolog_p7;
		copy_epilog_fptr = &copy_epilog_p7;
		st_fptrs[BFP_SP] = &func_store_64_p7;
		st_fptrs[BFP_DP] = &func_store_64_p7;
		st_fptrs[BFP_QP] = &func_store_128_p7;
		st_fptrs[VECTOR_SP] = &func_store_128_p7;
		st_fptrs[VECTOR_DP] = &func_store_128_p7;
		st_fptrs[QGPR] = &func_store_128_p7;
		st_fptrs[SCALAR_HP] = &func_store_128_p7;
		st_fptrs[VECTOR_HP] = &func_store_128_p7;
	}
	else { /* Its an Error. This code should not run on any other arch */
		return(-1);
	}
	return(0);
}

/*
 * Walk through vsx_instructions_array, apply category mask and filter out the masked instruction
 * categories. Save the instructions in vsx_enabled_instructions_array.
 * Look at biasing category and create separate array for each category.
 * Create a biased category index list per client.
 */
void
merge_instruction_tables()
{
	int i = 0, j = 0;
	char str[1024];
	uint16 proc_rev_num = (pvr & 0xFFFF);

	i = 0;
	while(j < MAX_INSTRUCTIONS && bfp_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
		memcpy(&master_instructions_array[j], &bfp_instructions_array[i], sizeof(struct instruction_masks));
		i++; j++;
	}
	sprintf(str, "Added %d BFP P6 instructions\n", i);
	if (shifted_pvr_os >= 0x3e) { /* If P6 or above */
		i = 0;
		while(j < MAX_INSTRUCTIONS && dfp_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &dfp_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d DFP instructions\n", str, i);
		i = 0;
		while(j < MAX_INSTRUCTIONS && vmx_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
		 	memcpy(&master_instructions_array[j], &vmx_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d VMX instructions\n", str, i);
	}
	if (shifted_pvr_os >= 0x3f) { /* If P7 or above */
		i = 0;
		while(j < MAX_INSTRUCTIONS && bfp_p7_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &bfp_p7_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d BFP P7 instructions\n", str, i);
		i = 0;
		while(j < MAX_INSTRUCTIONS && vsx_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
				memcpy(&master_instructions_array[j], &vsx_instructions_array[i], sizeof(struct instruction_masks));
				i++; j++;
		}
		sprintf(str, "%sAdded %d VSX instructions\n", str, i);
	}
	if (shifted_pvr_os >= 0x3e) { /* If P6 or above */
		i = 0;
		while(j < MAX_INSTRUCTIONS && cpu_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &cpu_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d P6 CPU instructions\n", str, i);
	}
	if (shifted_pvr_os >= 0x3f) { /* If P7 or above */
		i = 0;
		while(j < MAX_INSTRUCTIONS && cpu_p7_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &cpu_p7_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d P7 CPU instructions\n", str, i);

		i = 0;
		while(j < MAX_INSTRUCTIONS && cpu_macro_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &cpu_macro_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d Macro P8 instructions\n", str, i);
	}
	if (shifted_pvr_os >= SHIFTED_PVR_OS_P8) { 					/* If salerno or above processor versions */
		i = 0;
		while(j < MAX_INSTRUCTIONS && cpu_p8_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &cpu_p8_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d P8 CPU instructions\n", str, i);

		i = 0;
		while(j < MAX_INSTRUCTIONS && bfp_p8_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &bfp_p8_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d BFP P8 instructions\n", str, i);

		i = 0;
		while(j < MAX_INSTRUCTIONS && vmx_p8_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &vmx_p8_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d VMX P8 instructions\n", str, i);

		i = 0;
		while(j < MAX_INSTRUCTIONS && vsx_p8_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &vsx_p8_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		if (proc_rev_num >= PROC_DD2) {
			i = 0;
			while(j < MAX_INSTRUCTIONS && vmx_p8_dd2_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
				memcpy(&master_instructions_array[j], &vmx_p8_dd2_instructions_array[i], sizeof(struct instruction_masks));
				i++; j++;
			}
		}
		sprintf(str, "%sAdded %d VSX P8 instructions\n", str, i);
	}
	if (shifted_pvr_os >= SHIFTED_PVR_OS_P9) {	/* IBM P9 instructions */
		i = 0;
		while(j < MAX_INSTRUCTIONS && cpu_p9_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &cpu_p9_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d CPU P9 instructions\n", str, i);

		i=0;
		while(j < MAX_INSTRUCTIONS && vmx_p9_instructions_array[i].op_eop_mask != 0xDEADBEEF) {
			memcpy(&master_instructions_array[j], &vmx_p9_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d VMX P9 instructions\n", str, i);

		i = 0;
		while(j < MAX_INSTRUCTIONS && vsx_p9_instructions_array[i].op_eop_mask != 0xDEADBEEF ) {
			memcpy(&master_instructions_array[j], &vsx_p9_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d VSX P9 instructions\n", str, i);

		i = 0;
		while(j < MAX_INSTRUCTIONS && bfp_p9_instructions_array[i].op_eop_mask != 0xDEADBEEF ) {
			memcpy(&master_instructions_array[j], &bfp_p9_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d BFP P9 instructions\n", str, i);

		i = 0;
		while(j < MAX_INSTRUCTIONS && dfp_p9_instructions_array[i].op_eop_mask != 0xDEADBEEF ) {
			memcpy(&master_instructions_array[j], &dfp_p9_instructions_array[i], sizeof(struct instruction_masks));
			i++; j++;
		}
		sprintf(str, "%sAdded %d DFP P9 instructions\n", str, i);
	}

	total_num_instructions = j;
	sprintf(str, "%sTOTAL NUM INSTRUCTIONS = %d\n", str, total_num_instructions);

#ifdef AWAN
	printf("%s", str);
#else
	DPRINT(hlog, "%s", str);
#endif
}

/*
 * filter out instructions from global merged instruction table based on mask specified
 * in rule file.
 * copy enabled instruction to per thread enabled instruction table
 */
int filter_masked_instruction_categories(int cno, uint64 mask, struct instruction_masks *table)
{
    int i, j, enabled_instr = 0;

    struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[cno];

    struct enabled_instruction *enabled_instr_ptr = cptr->enabled_ins_table;
	bzero(cptr->enabled_ins_table, sizeof(struct enabled_instruction) * MAX_INSTRUCTIONS); 

    for(i = 0; i < total_num_instructions; i++) {
		for(j = 0; j < MAX_BIAS_LIST; j++) {
			uint64 mask = cptr->bias_list[j][0];
			uint64 sub_mask = (mask & ~(INS_CAT)) & ~(P9_ONLY);
			if ((mask == 0) || (sub_mask == 0)) {
				continue;
			}
		#ifdef __HTX_LINUX__
			if ((mask & exclude_cat_mask) == (VMX_MISC_ONLY)) {
                mask = mask & (~((VMX_MISC_ONLY)&  ~(INS_CAT)));
            }
		#endif
		    if ((table[i].ins_cat_mask & INS_CAT) == (mask & INS_CAT) ) {
		    	if ((table[i].ins_cat_mask & 0x00ffffffffffffffULL) & (mask & 0x00ffffffffffffffULL)) {
					/* check for P9 only instructions */
					
					if (mask & P9_ONLY) {
						/* only select P9 instructions */
						if ((table[i].ins_cat_mask & P9_ONLY) && (((table[i].ins_cat_mask & ~(P9_ONLY)) & ~(INS_CAT)) & sub_mask)) {
		    				memcpy(&enabled_instr_ptr->instr_table, &table[i], sizeof(struct instruction_masks));
		    				enabled_instr_ptr++;
		    				enabled_instr++;
						}
					}
					else {
		    			memcpy(&enabled_instr_ptr->instr_table, &table[i], sizeof(struct instruction_masks));
		    			enabled_instr_ptr++;
		    			enabled_instr++;
					}
				}
		    } /* end of  if mask in table match mask in rule */
		}
    }
    cptr->num_enabled_ins = enabled_instr;
    DPRINT(hlog,"%s: Num of enabled instructions = %d\n", __FUNCTION__, cptr->num_enabled_ins);
	FFLUSH(hlog);
	return (cptr->num_enabled_ins);
}
/*
 * set_ins_bias_array populates cptr->ins_bias_array according to the bias provided through rule file.
 */
void
set_ins_bias_array(int cno)
{
	uint8 *bias_array;
	int i, index = 0, j;

	bias_array = global_sdata[INITIAL_BUF].cdata_ptr[cno]->ins_bias_array;
	for(i = 0; i < global_sdata[INITIAL_BUF].cdata_ptr[cno]->num_bias_cat; i++) {
		for (j = 0; j < global_sdata[0].cdata_ptr[cno]->manage_bias_list[i].bias_num; j++) {
			if(index > 99) {
				sprintf(msg, "%s: biasing beyond 100 percentage !\n",__FUNCTION__);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				return;
			}
			bias_array[index] = i;
			index++;
		}
	}
}

int distribute_vsrs_based_on_ins_bias(int cno)
{
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[cno];
	struct ins_category *ins_cat;
	uint32 vsr_wt_array[VSR_OP_TYPES], bfp_wt_array[BFP_OP_TYPES], dfp_wt_array[DFP_OP_TYPES];
	uint32 vmx_wt_array[VMX_OP_TYPES], total_wt_array[VSR_OP_TYPES], total_cat_wt[INS_TYPES];
	uint32 accum_wt, reg_to_use;
	int extra_regs, extra_per_op;
	uint32 i, j, alloted_vsrs = 0, reg_31, reg_63;
	uint64 vsx_ins_masks_array[VSR_OP_TYPES], bfp_ins_masks_array[BFP_OP_TYPES], vmx_ins_masks_array[VMX_OP_TYPES];
	uint64 cpu_ins_masks_array;  									/* only cpu instructions  which act on GPRs*/

	bfp_ins_masks_array[BFP_SP] =  BFP_SP_ALL;
	bfp_ins_masks_array[BFP_DP] =  BFP_DP_ALL;
	bfp_ins_masks_array[BFP_QP] =  BFP_QP_ALL;
	vsx_ins_masks_array[DFP_SHORT] = DFP_SHORT_ONLY;
	vsx_ins_masks_array[DFP_LONG] =  DFP_LONG_ONLY;
	vsx_ins_masks_array[DFP_QUAD] =  DFP_QUAD_ONLY;
	vsx_ins_masks_array[SCALAR_SP] =  VSX_SCALAR_SP_ONLY;
	vsx_ins_masks_array[SCALAR_DP] =  VSX_SCALAR_DP_ONLY | VSX_HP_ONLY; /* Scalar HP convert to<->from Scalar DP */
	vsx_ins_masks_array[VECTOR_SP] =  VSX_VECTOR_SP_ONLY | VSX_HP_ONLY; /* Vector HP convert to<->from Vector SP */
	vsx_ins_masks_array[VECTOR_DP] =  VSX_VECTOR_DP_ONLY;
	vsx_ins_masks_array[QGPR]      =  VSX_QGPR_MASK;
	vsx_ins_masks_array[SCALAR_HP] =  VSX_HP_ONLY;
	vsx_ins_masks_array[VECTOR_HP] =  VSX_HP_ONLY;

	vmx_ins_masks_array[VECTOR_SP] = VMX_FP_ALL;
	vmx_ins_masks_array[QGPR]      = VMX_INT_ALL | VMX_MISC_ONLY | VMX_DFP_ARITHMETIC | P9_VMX_DFP_ARITHMETIC;

	cpu_ins_masks_array		= CPU_ALL;

	DPRINT(cptr->clog, "\n**********Entry: %s: for Client: %d**********\n", __FUNCTION__, cno);

	/* Running only a single for loop for all categories as its just initializing to 0.
	 * Performance wise better than having multiple loops one for each category with specific limits
	 */
	for(j = DUMMY; j < VSR_OP_TYPES; j++) {
		vsr_wt_array[j] = bfp_wt_array[j] = vmx_wt_array[j] = dfp_wt_array[j] = total_wt_array[j] = 0;
	}

	bzero(cptr->vsr_reg_wt, sizeof(cptr->vsr_reg_wt));
	bzero(cptr->bfp_reg_wt, sizeof(cptr->vsr_reg_wt));
	bzero(cptr->vmx_reg_wt, sizeof(cptr->vsr_reg_wt));

	for ( j=0; j<INS_TYPES; j++ ) {
		total_cat_wt[j]=0;
	}

	for(i = 0; i < cptr->num_bias_cat; i++) {
		uint64 tmp, tmp1;
		ins_cat = &cptr->manage_bias_list[i];
		tmp = ins_cat->mask & INS_CAT;
		if(tmp == VSX_CAT) {
			for(j = SCALAR_SP; j <= VECTOR_HP; j++) {
				tmp1 = vsx_ins_masks_array[j] & VSX_ALL;
				tmp = ins_cat->mask & VSX_ALL;
				DPRINT(cptr->clog, "VSX: bias num for type: %d is %lld. mask = %llx mask_ary = %llx tmp1 = %llx\n", j, ins_cat->bias_num, ins_cat->mask, vsx_ins_masks_array[j], tmp1);
				if(tmp & tmp1) {
					vsr_wt_array[j] += ins_cat->bias_num;
					total_wt_array[j] += ins_cat->bias_num;
					total_cat_wt[VSX] += ins_cat->bias_num;
					DPRINT(cptr->clog, "VSX: Adding vsx wt: %lld to type %d, total wt = %d\n", ins_cat->bias_num, j, total_wt_array[j]);
				}
			}
		}
		else if(tmp == BFP_ONLY) {
			j=0;
			while ( j < VSR_OP_TYPES ) {
				switch ( j ) {
					case BFP_SP:
					case BFP_DP:
					case BFP_QP:
						tmp1 = bfp_ins_masks_array[j] & 0x00ffffffffffffffULL;
						tmp = ins_cat->mask & 0x00ffffffffffffffULL;
						DPRINT(cptr->clog, "\n bias num for type: %d is %lld. mask = %llx mask_ary = %llx tmp1 = %llx", j, ins_cat->bias_num, ins_cat->mask, bfp_ins_masks_array[j], tmp1);
						if ( tmp & tmp1 ) {
							bfp_wt_array[j] += ins_cat->bias_num;
							total_wt_array[j] += ins_cat->bias_num;
							total_cat_wt[BFP] += ins_cat->bias_num;
							DPRINT(cptr->clog, "\n Adding bfp wt: %lld to type %d, total wt = %d", ins_cat->bias_num, j, total_wt_array[j]);
						}
				}
				j++;
			}
		}
		else if(tmp == DFP_ONLY) {
			for(j = DFP_SHORT; j <= DFP_QUAD; j++) {
				tmp1 = vsx_ins_masks_array[j] & VSX_ALL;
				tmp = ins_cat->mask & DFP_ALL;
				DPRINT(cptr->clog, "DFP: bias num for type: %d is %lld. mask = %llx mask_ary = %llx tmp1 = %llx tmp = %llx\n", j, ins_cat->bias_num, ins_cat->mask, vsx_ins_masks_array[j], tmp1, tmp);
				if(tmp & tmp1) {
					if(j == DFP_QUAD) {
						dfp_wt_array[j] += (2 * ins_cat->bias_num);
						total_wt_array[j] += (2 * ins_cat->bias_num);
						total_cat_wt[DFP] += (2 * ins_cat->bias_num);
					}
					else {
						dfp_wt_array[j] += ins_cat->bias_num;
						total_wt_array[j] += ins_cat->bias_num;
						total_cat_wt[DFP] += ins_cat->bias_num;
					}
					DPRINT(cptr->clog, "DFP: Adding dfp wt: %lld to type %d, total wt = %d dfp wt[%d] = %d\n", ins_cat->bias_num, j, total_wt_array[j], j, vsr_wt_array[j]);
				}
			}
		}
		else if(tmp == VMX_ONLY) {
			for(j = VECTOR_SP; j <= QGPR; j++) {
				if (j == VECTOR_DP)
					continue;
				tmp1 = vmx_ins_masks_array[j] & VMX_ALL;
				tmp = ins_cat->mask & VMX_ALL;
				DPRINT(cptr->clog, "VMX: bias num for type: %d is %lld. mask = %llx mask_ary = %llx tmp1 = %llx tmp = %llx\n", j, ins_cat->bias_num, ins_cat->mask, vmx_ins_masks_array[j], tmp1, tmp);
				if(tmp & tmp1) {
					vmx_wt_array[j] += ins_cat->bias_num;
					total_wt_array[j] += ins_cat->bias_num;
					total_cat_wt[VMX] += ins_cat->bias_num;
					DPRINT(cptr->clog, "VMX: Adding vmx wt: %lld to type %d, total wt = %d\n", ins_cat->bias_num, j, total_wt_array[j]);
				}
			}
		}
	}
	alloted_vsrs = 0;
	accum_wt = 0;

	for(j = SCALAR_SP; j < VSR_OP_TYPES; j++) {
		accum_wt += total_wt_array[j];
	}

	j = 0;
	while ( j < VSR_OP_TYPES ) {
		switch(j) {
			case BFP_SP:
			case BFP_DP:
			case BFP_QP:
				cptr->bfp_reg_wt[j] = (int)( ( ((float)bfp_wt_array[j] / accum_wt) * (NUM_FPRS - 1) ) );
				DPRINT(cptr->clog, "Adding %d to bfp_reg_wt[%d] with wt=%d, accum_wt=%d\n", cptr->bfp_reg_wt[j], j, bfp_wt_array[j], accum_wt);
				if ( bfp_wt_array[j] != 0 && cptr->bfp_reg_wt[j] < 4 ) {
					cptr->bfp_reg_wt[j] = 4;
				}
				alloted_vsrs += cptr->bfp_reg_wt[j];
		}
		j++;
	}
#if 0
	for (j = BFP_SP; j <= BFP_DP; j++ ) {
		/* cptr->bfp_reg_wt[j] = ( ( (bfp_wt_array[j] / accum_wt) * (NUM_FPRS - 1) ) / 100 ); */
		cptr->bfp_reg_wt[j] = (int)( ( ((float)bfp_wt_array[j] / accum_wt) * (NUM_FPRS - 1) ) );

		if ( bfp_wt_array[j] != 0 && cptr->bfp_reg_wt[j] < 4 ) {
			cptr->bfp_reg_wt[j] = 4;
		}
		alloted_vsrs += cptr->bfp_reg_wt[j];
	}
#endif

	for ( j = DFP_SHORT; j <= DFP_QUAD; j++ ) {
		if (dfp_wt_array[j] == 0) {
			continue;
		}
		cptr->vsr_reg_wt[j] = ( ( ((float)dfp_wt_array[j] / accum_wt) * (NUM_FPRS - 1) ) );
		if ( dfp_wt_array[j] != 0 && cptr->vsr_reg_wt[j] < 4 ) {
			cptr->vsr_reg_wt[j] = 4;
		}

		alloted_vsrs += cptr->vsr_reg_wt[j];
	}

	for ( j = VECTOR_SP; j <= QGPR; j++ ) {
		if ((j == VECTOR_DP) || (vmx_wt_array[j] == 0)) {
			continue;
		}
		cptr->vmx_reg_wt[j] = ( ( ((float)vmx_wt_array[j] / accum_wt) * (NUM_FPRS - 1) ) );
		if ( vmx_wt_array[j] !=0 && cptr->vmx_reg_wt[j] < 4 ) {
			cptr->vmx_reg_wt[j] = 4;
		}
		alloted_vsrs += cptr->vmx_reg_wt[j];
	}

	for ( j = SCALAR_SP; j <= VECTOR_HP; j++) {
		if (vsr_wt_array[j] == 0) {
			continue;
		}
		cptr->vsr_reg_wt[j] = ( ( ((float)vsr_wt_array[j] / accum_wt) * (NUM_FPRS -1) ) );
		if ( vsr_wt_array[j] !=0 && cptr->vsr_reg_wt[j] < 4 ) {
			cptr->vsr_reg_wt[j] = 4;
		}
		alloted_vsrs += cptr->vsr_reg_wt[j];
	}

	if ( total_cat_wt[VSX] != 0 || total_cat_wt[VMX] != 0 ) {
		reg_to_use = NUM_VSRS - 1;
	}
	else {
		reg_to_use = NUM_FPRS - 1;
	}

	/* Corrective action */
	while(alloted_vsrs < reg_to_use ) {
		extra_regs = reg_to_use - alloted_vsrs;
		extra_per_op = extra_regs / VSR_OP_TYPES;
		if ( extra_per_op <= 0 )
			extra_per_op = 1;

		for ( j = SCALAR_SP; j <= DFP_QUAD; j++ ) {
			reg_31 = reg_63 = 0;
			reg_31 = cptr->bfp_reg_wt[BFP_SP] + cptr->bfp_reg_wt[BFP_DP] + cptr->vsr_reg_wt[DFP_SHORT] + cptr->vsr_reg_wt[DFP_LONG] + cptr->vsr_reg_wt[DFP_QUAD] ;
			for ( j = SCALAR_SP; j <= DFP_QUAD; j++ ) {
				reg_63 += cptr->bfp_reg_wt[j];
				reg_63 += cptr->vmx_reg_wt[j];
				reg_63 += cptr->vsr_reg_wt[j];
			}

			if ((total_wt_array[j] != 0) && (cptr->bfp_reg_wt[j] != 0) && (reg_31 < NUM_FPRS)) {
				cptr->bfp_reg_wt[j]++;
				total_cat_wt[BFP]++;
				alloted_vsrs++;
				if (alloted_vsrs == reg_to_use ) {
					break;
				}
			}
			else if ( total_wt_array[j] != 0 && cptr->vmx_reg_wt[j] != 0 ) {
				cptr->vmx_reg_wt[j]++;
				total_cat_wt[VMX]++;
				alloted_vsrs++;
				if ( alloted_vsrs == reg_to_use ) {
					break;
				}
			}
			else if ( j <= DFP_SHORT && j >= DFP_QUAD && cptr->vsr_reg_wt[j] != 0 && reg_31 < NUM_FPRS ) {
				cptr->vsr_reg_wt[j]++;
				total_cat_wt[DFP]++;
				alloted_vsrs++;
				if ( alloted_vsrs == reg_to_use ) {
					break;
				}
			}
			else {
				cptr->vsr_reg_wt[j]++;
				alloted_vsrs++;
				if ( alloted_vsrs == reg_to_use ) {
					break;
				}
			}
		}
	}

	while(alloted_vsrs > reg_to_use ) {
		extra_regs = (alloted_vsrs - reg_to_use);
		extra_per_op = extra_regs / VSR_OP_TYPES;
		if ( extra_per_op <= 0 )
			extra_per_op = 1;

		for ( j = SCALAR_SP; j <= DFP_QUAD; j++ ) {
			if ( total_wt_array[j] != 0 && cptr->bfp_reg_wt[j] != 0 ) {
				cptr->bfp_reg_wt[j]--;
				alloted_vsrs--;
				if ( alloted_vsrs == reg_to_use ) {
					break;
				}
			}
			else {
				cptr->vsr_reg_wt[j]--;
				alloted_vsrs--;
				if ( alloted_vsrs == reg_to_use ) {
					break;
				}
			}
		}
	}

	DPRINT(cptr->clog, "\n**********Exit: %s: for Client: %d**********\n", __FUNCTION__, cno);
	return(0);
}

int
create_ins_cat_wise_tables(int cno)
{
    uint16 i, j, ins = 0, num_cat = 0, total_bias = 0, remaining = 0, bias, bli;
    uint64 mask;
    struct enabled_instruction *enabled_instr_ptr;
    struct ins_category *ins_cat;
    struct server_data *sdata = &global_sdata[INITIAL_BUF];
    struct client_data *cptr = sdata->cdata_ptr[cno];

	ins_cat = cptr->manage_bias_list;

	for(i = 0; i < NUM_CAT; i++) {
		cptr->bias_for_ins_cat[i] = 0;
	}

	enabled_instr_ptr = cptr->enabled_ins_table;

	for(bli = 0; bli < MAX_BIAS_LIST; bli++) {
		mask = cptr->bias_list[bli][0];
		bias = cptr->bias_list[bli][1];

		/* skip with mask list entries having no mask or bias number entry */
		if (((mask & 0x00ffffffffffffffULL) == 0) || ((bias & 0x00ffffffffffffffULL) == 0)) {
			continue;
		}

		DPRINT(cptr->clog, "Thread: %d Mask: %llx, bias: %lld\n", cno, cptr->bias_list[bli][0], cptr->bias_list[bli][1]);
		for(j = 0; j < cptr->num_enabled_ins; j++) {
			if ((enabled_instr_ptr[j].instr_table.ins_cat_mask & INS_CAT) == (mask & INS_CAT)) {
				if ((enabled_instr_ptr[j].instr_table.ins_cat_mask & 0x00ffffffffffffffULL) & mask) {
					ins_cat[num_cat].ptr[ins] = j;
					ins++;
				}
			}
		}
		if (ins != 0) {
			if ((mask & INS_CAT) == VSX_CAT) {
				cptr->bias_for_ins_cat[VSX] += bias;
			}
			else if((mask & INS_CAT) == BFP_ONLY) {
				cptr->bias_for_ins_cat[BFP] += bias;
			}
			else if((mask & INS_CAT) == DFP_ONLY) {
				cptr->bias_for_ins_cat[DFP] += bias;
			}
			else if((mask & INS_CAT) == VMX_ONLY) {
				cptr->bias_for_ins_cat[VMX] += bias;
			}
			else if((mask & INS_CAT) == CPU_ONLY) {
				cptr->bias_for_ins_cat[CPU] += bias;
			}

			ins_cat[num_cat].end_index = ins;
			ins_cat[num_cat].start_index = 0;
			ins_cat[num_cat].bias_num = cptr->bias_list[bli][1];
			ins_cat[num_cat].mask = mask;
			total_bias += ins_cat[num_cat].bias_num;
			DPRINT(cptr->clog,"%s: Thread: %d, end_index: %d, bias_num: %lld, total_bias: %d\n", __FUNCTION__, cno, ins, ins_cat[num_cat].bias_num, total_bias);
			FFLUSH(cptr->clog);
			num_cat++;
			ins = 0;
		}
	}

	if(total_bias > 100) {
		sprintf(msg, "%s: Rule file config error for thread %d! Trying to specify more than 100 pc ins bias\n",__FUNCTION__, cno);
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		return(-1);
	}
	else if (total_bias < 100) {
		/*
		 * For now, if the rule file is not configured to sum up the bias nos to 100 % then remaining
		 * bias % will be assigned to BFP_ALL.
		 */
		remaining = 100 - total_bias;
		mask = BFP_ALL;

		for(j = 0; j < cptr->num_enabled_ins; j++) {
			if ((enabled_instr_ptr[j].instr_table.ins_cat_mask & (uint64)INS_CAT) == (mask & (uint64)INS_CAT)) {
				if ((enabled_instr_ptr[j].instr_table.ins_cat_mask & (uint64)0x00ffffffffffffffULL) & mask) {
					ins_cat[num_cat].ptr[ins] = j;
					ins++;
				}
			}
		}
		if (ins != 0) {
			if ((mask & INS_CAT) == VSX_CAT) {
				cptr->bias_for_ins_cat[VSX] += remaining;
			}
			else if((mask & INS_CAT) == BFP_ONLY) {
				cptr->bias_for_ins_cat[BFP] += remaining;
			}
			else if((mask & INS_CAT) == DFP_ONLY) {
				cptr->bias_for_ins_cat[DFP] += remaining;
			}
			else if((mask & INS_CAT) == VMX_ONLY) {
				cptr->bias_for_ins_cat[VMX] += remaining;
			}
			else if((mask & INS_CAT) == CPU_ONLY) {
				cptr->bias_for_ins_cat[CPU] += remaining;
			}

			ins_cat[num_cat].end_index = ins;
			ins_cat[num_cat].start_index = 0;
			ins_cat[num_cat].bias_num = remaining;
			ins_cat[num_cat].mask = mask;
			DPRINT(cptr->clog,"%s: Thread: %d, end_index: %d, bias_num: %lld, total_bias: %d\n", __FUNCTION__, cno, ins, ins_cat[num_cat].bias_num, total_bias);
			FFLUSH(cptr->clog);
			num_cat++;
			ins = 0;
		}
	}
	cptr->num_bias_cat = num_cat;
	return(0);
}



#ifdef SCTU
int get_bind_core_and_cpus_list()
{
	char temp_str[256];
	int core_gang_num, num_cores_in_gang;
	int i, t, c, n, l, rc, hw_smt, num_cores, num_nodes;

    rc = repopulate_syscfg(&hd);
    if (rc != 0 ) {
        sprintf(msg,"repopulate_syscfg failed with error code= %d \n",rc);
        hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
        exit_flag = 1;
        return -1;
    }

	if ( rule_list[0].testcase_type == CHIP_TEST ) {
		rc = chip_testcase();
		return(rc);
	}

	if ( rule_list[0].testcase_type == NODE_TEST ) {
		rc = node_testcase();
		return(rc);
	}

	rc = init_sctu_cpu_array();
	if ( rc ) {
		sprintf(msg,"init_sctu_cpu_array failed with error code= %d \n",rc);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		exit_flag = 1;
		return -1;
	}
		
	rc = get_hardware_stat(&Sys_stat);
	if( rc ){
		sprintf(msg, "get_hardware_stat call failed! rc = %d\n",rc);
		hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
		exit_flag = 1;
		return -1;
	}

	num_cores = Sys_stat.cores;
	num_nodes = Sys_stat.nodes;

	get_smt_details(&smt_detail);
	hw_smt = smt_detail.smt_threads;
	core_gang_num = dev_num / hw_smt;


	/* Now find actual cores in gang. */
	if (rule_list[0].testcase_type != INTERNODE_TEST ) {
		int first_core_num, last_core_num;

		num_cores_in_gang = SCTU_GANG_SIZE_PERF;

		first_core_num = core_gang_num * SCTU_GANG_SIZE_PERF;
		last_core_num = first_core_num + num_cores_in_gang - 1;

		if (last_core_num >= num_cores) {
			last_core_num = num_cores - 1;
			num_cores_in_gang = num_cores % SCTU_GANG_SIZE_PERF;
		}

		if(last_core_num < first_core_num) {
			last_core_num = first_core_num;
			num_cores_in_gang = 0;
		}

		l = sprintf(msg, "Total cpus = %d, HW_SMT = %d, cores = %d, nodes = %d\n", num_cpus, hw_smt, num_cores, num_nodes);
		sprintf(msg+l, "Core_gang_num = %d, num_cores_in_gang = %d, first_core_num = %d, last_core_num = %d\n", core_gang_num, num_cores_in_gang, first_core_num, last_core_num);
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);

		/* Calculating core numbers in gang. */
		c = first_core_num;
		for	(i=0; i<num_cores_in_gang; i++){
			cores_in_gang[i] = c;
			c++;
		}

	} else {
		/* Internode run */
		int max_cores_from_each_node, start_core;

		/* Find out core config for each node. */
		c = 0;
		sprintf(msg, "Node config details:\n");
		for(n=0; n < num_nodes; n++) {
			rc = get_cores_in_node(n, cores_in_node[n]);
			sprintf(temp_str, "Cores in Node%d: %d-%d\n", n, c, c+rc-1);
			strcat(msg, temp_str);
			c = c+rc;
			if (rc == -1) {
				sprintf(msg, "get_cores_in_node Failed for node = %d\n.", n);
				hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
				exit(-1);
			}
		}
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);

		/* Now calculate core numbers in gang. */
		max_cores_from_each_node = SCTU_GANG_SIZE / num_nodes;
		for(n=0, i=0; n < num_nodes; n++) {
			start_core = core_gang_num*max_cores_from_each_node;
			for(c=0; c <max_cores_from_each_node; c++) {
				if (cores_in_node[n][c+start_core] == -1)
					break;
				cores_in_gang[i] = cores_in_node[n][c+start_core];
				i++;
			}
		}
		num_cores_in_gang = i;

		l = sprintf(msg, "Total cpus = %d, HW_SMT = %d, cores = %d, nodes = %d\n", num_cpus, hw_smt, num_cores, num_nodes);
		sprintf(msg+l, "Core_gang_num = %d, num_cores_in_gang = %d, max_cores_from_each_node = %d, start_core = %d\n", 
			core_gang_num, num_cores_in_gang, max_cores_from_each_node, start_core);
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);

		sprintf(msg, "Cores in gang: ");
		for(i=0; i<num_cores_in_gang; i++) {
			sprintf(temp_str, "%d ", cores_in_gang[i]);
			strcat(msg, temp_str);
		}
		strcat(msg, "\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}

	/* Each sub-array in Cpus_in_core array will have cpu numbers from a core*/
	sprintf(msg, "Core config details:\n");
	for(c=0; c<num_cores_in_gang; c++)
	{
		sprintf(temp_str, "Cpus in Core%d: ", cores_in_gang[c]);
		strcat(msg, temp_str);
		n = get_cpus_in_core(cores_in_gang[c], cpus_in_core_gang[c]);
		for(t=0; t<n; t++)
		{
			sprintf(temp_str, "%d ", cpus_in_core_gang[c][t]);
			strcat(msg, temp_str);
		}
		strcat(msg, "\n");
	}
	hxfmsg(&hd, 0, HTX_HE_INFO, msg);

	/* Calculate bind cpus in gang. */
	/* Also handle cases where core is up but all hw threads(cpus) to bind are not up. Remove such cpus. */
	sprintf(msg, "Cpus in gang: ");
	for(c=0,i=0; c<num_cores_in_gang; c++) {
		t = dev_num % hw_smt;
		if(cpus_in_core_gang[c][t] != -1 ){
			bcpus_in_gang[i] = cpus_in_core_gang[c][t];
			sprintf(temp_str, "%d ", bcpus_in_gang[i]);
			strcat(msg, temp_str);
			i++;
		}
	}

	num_cpus_to_test = i;
	sprintf(temp_str, "\nTotal Cpus to test = %d\n", num_cpus_to_test);
	strcat(msg, temp_str);

	hxfmsg(&hd, 0, HTX_HE_INFO, msg);

	return 0;
}


void initialize_sctu_locks( )
{
	sem_init(&pass1_exec_sem, 0, 0);
	sem_init(&pass2_exec_sem, 0, 0);
	sem_init(&rerun_pp_sem, 0, 0);
	pthread_mutex_init(&testcase_built_lock, NULL);
	pthread_mutex_init(&pass1_exec_lock, NULL);
	pthread_mutex_init(&pass2_ready_count_lock, NULL);
	pthread_mutex_init(&rerunpp_ready_count_lock, NULL);
	pthread_mutex_init(&compare_ls_lock, NULL);

}
#endif

int reinit_ls_base(uint32 client_no)
{
	uint64 ls_max_size;
	uint32 num_ins_built, prolog_size, *tc_mem;
	struct server_data *s = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = s->cdata_ptr[client_no];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;
	tc_mem = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
#ifdef SCTU
	uint32 i, last_rebase_index;
	ls_max_size = 2 * MAX_INS_STREAM_DEPTH * 16; 
#else
	ls_max_size = MAX_INS_STREAM_DEPTH * 16; 
	DPRINT(cptr->clog, "%s: prolog_size: %d, num_ins_built: %d, ls_base_volatile[INITIAL_BUF]:%llx, ls_base[INITIAL_BUF]: %llx\n", __FUNCTION__, prolog_size, num_ins_built, (uint64)cptr->ls_base_volatile[INITIAL_BUF], (uint64)cptr->ls_base[INITIAL_BUF]);
#endif
 
	if( cptr->reinit_base ){
		*tc_mem = GEN_ADDI_MCODE(LOAD_RA, LOAD_RA, MAX_ADDI_OFFSET); /*0x7fff*/
		cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
		tc_mem++;
		num_ins_built++;
		*tc_mem = GEN_ADDI_MCODE(LOAD_RA, LOAD_RA, 0x1);
		cptr->instr_index[prolog_size + num_ins_built] = addi | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_addi;
		tc_mem++;
		num_ins_built++;
		*tc_mem = OR(LOAD_RA, BASE_GPR);             /* copy value R5->R7 */
		cptr->instr_index[prolog_size + num_ins_built] = or | 0x20000000;
		cptr->tc_ptr[INITIAL_BUF]->sim_ptr[prolog_size + num_ins_built] = (sim_fptr)&simulate_or;
		tc_mem++;
		num_ins_built++;
		cptr->num_ins_built = num_ins_built;
#ifdef SCTU
		for( i= cptr->last_rebase_index; i<(prolog_size+num_ins_built); i++){
			if( cptr->tc_ptr[INITIAL_BUF]->ea_off[i] != 0){
				cptr->tc_ptr[INITIAL_BUF]->ea_initbase_off[i] = cptr->tc_ptr[INITIAL_BUF]->ea_off[i] + (cptr->ls_base_volatile[INITIAL_BUF] - cptr->ls_base[INITIAL_BUF]);
			}
		}
		cptr->last_rebase_index = prolog_size + cptr->num_ins_built;
#endif

		/* update volatile base to current base + 0x8000(32k) to be able to offset into more load/store area */
		cptr->ls_base_volatile[INITIAL_BUF] = cptr->ls_base_volatile[INITIAL_BUF] + MAX_ADDI_OFFSET + 1; /*0x8000*/
		/* check for ls_base_volatile reaching instruction depth limit */
		/* sctu ls area is twice big, make it double */
		if ((((uint64)cptr->ls_base_volatile[INITIAL_BUF] - (uint64)cptr->ls_base[INITIAL_BUF]) + cptr->prolog_size + cptr->num_ins_built + cptr->epilog_size) >= ls_max_size) { 
			sprintf(msg, "Client: %d, Unable to continue testcase, LS Base Addr: %llx, New LS Rebase Addr: %llx !!!\n", client_no, (uint64)cptr->ls_base[INITIAL_BUF], (uint64)cptr->ls_base_volatile[INITIAL_BUF]);
            hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
            exit_flag = 1;
			return (-1);
		}
		else {
			/* Successful Rebase Msg */
			#if 0
			sprintf(msg, "Client: %d, Current LS Base Addr: %llx, New LS Rebase Addr: %llx\n", client_no, (uint64)cptr->ls_base[INITIAL_BUF], (uint64)cptr->ls_base_volatile[INITIAL_BUF]);
            hxfmsg(&hd, 0, HTX_HE_INFO, msg);
			#endif
		}	
#ifdef SCTU
		cptr->ls_current[INITIAL_BUF] =  cptr->ls_cl_start[INITIAL_BUF] = cptr->ls_base_volatile[INITIAL_BUF] + (client_no * CLIENT_NUM_BYTES_IN_CL);
#else
		DPRINT(cptr->clog, "%s: cptr->ls_current[INITIAL_BUF] = %llx, cptr->ls_base_volatile[INITIAL_BUF] = %llx, MAX_INS_STREAM_DEPTH = 0x%X\n", __FUNCTION__, (uint64)cptr->ls_current[INITIAL_BUF], (uint64)cptr->ls_base_volatile[INITIAL_BUF], MAX_INS_STREAM_DEPTH);
        cptr->ls_current[INITIAL_BUF] = cptr->ls_base_volatile[INITIAL_BUF]; 
#endif

		DPRINT(cptr->clog, "client_no: %d, core_num: %d ********Resetting reinit_base flag**************\n", client_no, core_num);
		cptr->reinit_base = 0;
	}
	else {

#ifdef SCTU
		for( i= cptr->last_rebase_index; i<(prolog_size+num_ins_built); i++){
			if( cptr->tc_ptr[INITIAL_BUF]->ea_off[i] != 0){
				cptr->tc_ptr[INITIAL_BUF]->ea_initbase_off[i] = cptr->tc_ptr[INITIAL_BUF]->ea_off[i] + (cptr->ls_base_volatile[INITIAL_BUF] - cptr->ls_base[INITIAL_BUF]);
			}
		}
		cptr->last_rebase_index = prolog_size + cptr->num_ins_built;
#endif
	}

	return (0);
}

int update_ea_off_array(uint32 client_no)
{
	uint32 num_ins_built, prolog_size, i, /**tc_mem,*/ last_off;
	struct server_data *s = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = s->cdata_ptr[client_no];
	char *temp_base = cptr->ls_base[INITIAL_BUF];

	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;

	last_off = 0;
	for( i= prolog_size; i<(prolog_size+num_ins_built); i++){
		if( cptr->tc_ptr[INITIAL_BUF]->ea_off[i] != 0){
			if( cptr->tc_ptr[INITIAL_BUF]->ea_off[i] < last_off ) {
				temp_base = temp_base + MAX_ADDI_OFFSET + 1;
			}
			cptr->tc_ptr[INITIAL_BUF]->ea_initbase_off[i] = cptr->tc_ptr[INITIAL_BUF]->ea_off[i] + (temp_base - cptr->ls_base[INITIAL_BUF]);
			last_off = cptr->tc_ptr[INITIAL_BUF]->ea_off[i];
		}

	}
	return (0);
}


#ifndef SCTU
int get_bind_cpus_list(void)
{
	int i, t;
	int rc = -1;

 	rc = repopulate_syscfg(&hd);
	if (rc) {
		sprintf(msg, "repopulate_syscfg failed with error code = %d\n", rc);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		exit_flag = 1;
		return -1;
	}

	if (strstr("no", proc_env.proc_shared_mode) && (!equaliser_flag))
	{
		t = get_cpus_in_core(core_num, cpus_in_core);
		if (t < 0) {
			sprintf(msg, "Unable to find cpus in core%d. Exiting.\n", core_num);
			hxfmsg(&hd, 0, HTX_HE_HARD_ERROR, msg);
			exit_flag = 1;
			return -1;
		}

		num_cpus_to_test = t;

		sprintf(msg, "Cpus in Core%d: ", core_num);
		for(i=0; i<t; i++)
			sprintf(msg, "%s%d ", msg, cpus_in_core[i]);
		strcat(msg, "\n");
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);

	} else {
		num_cpus_to_test = 1;
		if(core_num >= num_cpus)
			num_cpus_to_test = 0;
	}
	return 0;
}
#endif

int build_sync_point(uint32 client_no)
{
	uint32 num_ins_built, prolog_size, *tc_mem, *save_ptr;
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr = sdata->cdata_ptr[client_no];
	prolog_size = cptr->prolog_size;
	num_ins_built = cptr->num_ins_built;

	save_ptr = (uint32 *)sdata->cdata_ptr[client_no]->tc_ptr[INITIAL_BUF]->tc_ins;
	tc_mem = &(cptr->tc_ptr[INITIAL_BUF]->tc_ins[prolog_size + num_ins_built]);
	*tc_mem = BL((int)(-((uint64)tc_mem - (uint64)save_ptr)));
	cptr->instr_index[prolog_size + num_ins_built] = brlink | 0x20000000;
	tc_mem++;
	num_ins_built++;

	cptr->num_ins_built = num_ins_built;
	return (0);
}

void initialize_sync_words(int pass)
{
	struct server_data *sdata = &global_sdata[pass];
	struct common_data *cd = sdata->common_data_ptr;
	int j/*, num_sync_points*/;

	for(j = 0; j < cd->num_sync_points; j++) {
		cd->sync_words[j] = ((2 << (rule.num_threads - 1)) - 1);
		dcbf(&(cd->sync_words[j]));
	}

}


void * sighandler_thread(void *arg)
{
	int rc;
	int sig;

	DPRINT(hlog, "running sighandler_thread with arg = %d\n", *((int *)arg));

	for( ; ; ) {

		rc = sigwait(&myset, &sig);
		if (rc) {
			sprintf(msg, "sigwait failed: %s ...exiting\n", strerror(errno));
			hxfmsg(&hd, -1, HTX_HE_HARD_ERROR, msg);
			exit(-1);
		}
#if 0
		DPRINT(hlog, "got signal %s (num = %d) in sighandler_thread \n", strsignal(sig), sig);
		sprintf(msg, "got signal %s (num = %d) in sighandler_thread \n", strsignal(sig), sig);
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
#endif
		switch(sig){
			case SIGALRM:
				SIGALRM_hdl(sig);
				break;
			case SIGINT:
				SIGINT_handler(sig);
				break;

			case SIGTERM:
				SIGTERM_hdl(sig);
				break;

#ifndef __HTX_LINUX__

			case SIGRECONFIG:
				SIGRECONFIG_handler(sig, sig, NULL);
				break;

			case SIGCPUFAIL:
				SIGCPUFAIL_handler(sig);
				break;
#else
			case SIGUSR2:
				SIGUSR2_handler();
				break;
#endif
		}
		if (sighandler_exit_flag) {
			break;	
		}
	}

	pthread_exit(NULL);
}


int copy_common_data(int pass, int cno)
{
	char *from_ptr, *to_ptr /*,i*/;
	struct server_data *i_buf = &global_sdata[INITIAL_BUF], *pass_buf = &global_sdata[pass];
	/*struct tc_context *tcc;*/

	from_ptr = (char *)i_buf->cdata_ptr[cno]->tc_ptr[INITIAL_BUF];
	to_ptr = (char *)pass_buf->cdata_ptr[cno]->tc_ptr[pass];

	/*
	 * Copy common data
	 */
#ifdef SCTU
	from_ptr = (char *)i_buf->common_data_ptr->ls_ptr;
	to_ptr = (char *)pass_buf->common_data_ptr->ls_ptr;
	memcpy(to_ptr, from_ptr, 2*MAX_INS_STREAM_DEPTH*16);
#endif
	return (0);
}

#ifdef SCTU
/* last sctu client should dump testcase for all clients */
/* this is done to avoid adding extra synchronization for all threads to be stopped/synced */
/* so that some clients don't move to next num oper when others are dumping their testcase */
/* causing improper dump as clients reset their num_instructions etc */
void dump_testcase_sctu(int client_num, int num_oper, int rc, int miscomparing_num)
{
	struct server_data *sdata = &global_sdata[INITIAL_BUF];
	struct client_data *cptr;
	int client_no;

	for( client_no=0; client_no<rule.num_threads; client_no++) {

		cptr = sdata->cdata_ptr[client_no];
		if(rule.dump_testcase[client_no]) {
			if(shifted_pvr_os <= 0x3e) {
				dump_testcase_p6(client_no, num_oper, rc, miscomparing_num);
			}
			else {
				dump_testcase_p7(client_no, num_oper, rc, miscomparing_num);
			}
#ifndef __HTX_LINUX__
			{
				/* Location code currently supported in AIX only */
				FILE *tmp_fp;
				char tmp_str[200], loc_code1[50], loc_code2[100];
				sprintf(msg, "cat /tmp/sctProcMap | awk 'NR==%d'", ((cptr->bcpu)+1));
				tmp_fp = popen(msg, "r");
				fgets(tmp_str, 100, tmp_fp);
				sscanf(tmp_str, "%*s %s %s", loc_code1, loc_code2);
				pclose(tmp_fp);
				sprintf(msg, "Log dumped.\nProcessor detail - Logical Processor : %d, Physical Core : %d, Location Code : %s %s \
						\nLog file created : /tmp/%s_miscompare.%d.%x\n", cptr->bcpu,core_num,loc_code1,loc_code2,(char *)(dinfo.device_name+5), core_num,cptr->original_seed);
				hxfmsg(&hd, 0, HTX_HE_MISCOMPARE, msg);
			}
#else /* __HTX_LINUX__ */
			sprintf(msg, "Log dumped.\n Processor details - Logical Processor : %d, Physical Core : %d\
					\nLog file created : /tmp/%s_miscompare.%d.%x\n", cptr->bcpu, core_num, (char *)(dinfo.device_name+5), core_num, cptr->original_seed);
			hxfmsg(&hd, 0, HTX_HE_MISCOMPARE, msg);
#endif /* __HTX_LINUX__ */
		}

	}

}

int chip_testcase(void)
{
	FILE *fptr;
	char temp_msg[20];
	int i, j, k, rc, tmp, residue, server_num;
	int num_chips, sctu_gang_num, core_sequential_num, server_sequential_num;
	int cores_in_chip[MAX_CHIPS][MAX_CORES_PER_CHIP], sctu_gangs[MAX_CHIPS*2][8];
	int cpus_in_sctu_server[MAX_CHIPS*2*MAX_CPUS_PER_CORE][SCTU_GANG_SIZE], sctu_server_size[MAX_CHIPS*2*MAX_CPUS_PER_CORE];

	if ( strcasecmp(dinfo.device_name, "SCTU_DEV") == 0 ) {
		fptr = fopen("/tmp/htx_sctu_chip_dev","w");

		if ( fptr == NULL ) {
			/* Below printf is correct. It should not be replaces with sprintf/hxfmsg. 
			 * That is because this code will run only with script. not during exerciser's
			 * regular run. And with script(devconfig) stdout is open. 
			 */
			printf("Unable to open /tmp/htx_sctu_chip_dev. errno=%d(%s)", errno, strerror(errno));
			return(errno);
		}
	}

	if (shm_flag_for_malloc == 0) {
		rc = repopulate_syscfg(&hd);
		if ( rc ) {
			sprintf(msg,"repopulate_syscfg failed with error code= %d \n",rc);
			hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
			exit_flag = 1;
			return -1;
		}
	}


	rc = init_sctu_cpu_array();
	if ( rc ) {
		sprintf(msg,"init_sctu_cpu_array failed with error code= %d \n",rc);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		exit_flag = 1;
		return -1;
	}
		
	rc = get_hardware_stat(&Sys_stat);
	if ( rc ) {
		sprintf(msg, "get_hardware_stat call failed! rc = %d\n",rc);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		exit_flag = 1;
		return -1;
	}

	num_chips = Sys_stat.chips;

	sctu_gang_num = 0;
	core_sequential_num=0;
	server_sequential_num=0;

	for ( i=0; i<num_chips; i++ ) {
		rc = get_core_info(cores_in_chip[i], i);
		tmp = rc / SCTU_GANG_SIZE;
		residue = rc % SCTU_GANG_SIZE;

		if ( rc > 1 ) { /* Avoid single-core chips. */
			/* Assign start core number in the gang as the 1st core in that chip */
			sctu_gangs[sctu_gang_num][FIRST_CORE] = cores_in_chip[i][0];
			sctu_gangs[sctu_gang_num][FIRST_CORE_INDEX] = 0; /* Keep the index in cores_in_chip array */
	
			/* Last core number is lesser of residue and SCTU_GANG_SIZE */
			sctu_gangs[sctu_gang_num][LAST_CORE] = (tmp>0) ? cores_in_chip[i][SCTU_GANG_SIZE-1] : cores_in_chip[i][residue-1] ;
			sctu_gangs[sctu_gang_num][LAST_CORE_INDEX] = (tmp>0) ? (SCTU_GANG_SIZE-1) : (residue-1) ;
	
			/* Keep minimum smt of the gang as 4th member. */
			sctu_gangs[sctu_gang_num][MIN_SMT_GANG] = get_smt_status(sctu_gangs[sctu_gang_num][0]);
			sctu_gangs[sctu_gang_num][MIN_SMT_CORE_INDEX] = 0; /* index of min smt core */
			for ( j=sctu_gangs[sctu_gang_num][FIRST_CORE_INDEX]+1; j<=sctu_gangs[sctu_gang_num][LAST_CORE_INDEX]; j++ ) {
				if ( sctu_gangs[sctu_gang_num][MIN_SMT_GANG] > get_smt_status(cores_in_chip[i][j]) ) {
					sctu_gangs[sctu_gang_num][MIN_SMT_GANG] = get_smt_status(cores_in_chip[i][j]);
					sctu_gangs[sctu_gang_num][MIN_SMT_CORE_INDEX] = j;
				}
			}
	
			sctu_gangs[sctu_gang_num][GANG_SIZE] = ((sctu_gangs[sctu_gang_num][LAST_CORE_INDEX] - sctu_gangs[sctu_gang_num][FIRST_CORE_INDEX]) + 1 );
			sctu_gang_num++;
			
	
			/* Now check for remainder */
			/* Below look is just 'if' and not 'while' because of the assumption that there
 			 * can not be more that 16 cores (2 gangs) in a chip. If this assumption changes
 			 * in future, make sure to tweak below loop accordingly. 
 			 */
			if ( tmp>0 && residue>1 ) {
				sctu_gangs[sctu_gang_num][FIRST_CORE] = cores_in_chip[i][SCTU_GANG_SIZE];
				sctu_gangs[sctu_gang_num][FIRST_CORE_INDEX] = SCTU_GANG_SIZE;
	
				sctu_gangs[sctu_gang_num][LAST_CORE] = cores_in_chip[i][rc-1];
				sctu_gangs[sctu_gang_num][LAST_CORE_INDEX] = rc-1;
	
				sctu_gangs[sctu_gang_num][MIN_SMT_GANG] = get_smt_status(sctu_gangs[sctu_gang_num][0]);
				sctu_gangs[sctu_gang_num][MIN_SMT_CORE_INDEX] = 0; /* index od min smt core */
				for ( j=sctu_gangs[sctu_gang_num][1]+1; j<=sctu_gangs[sctu_gang_num][3]; j++ ) {
					if ( sctu_gangs[sctu_gang_num][MIN_SMT_GANG] > get_smt_status(cores_in_chip[i][j]) ) {
						sctu_gangs[sctu_gang_num][MIN_SMT_GANG] = get_smt_status(cores_in_chip[i][j]);
						sctu_gangs[sctu_gang_num][MIN_SMT_CORE_INDEX] = j;
					}
				}
	
				sctu_gangs[sctu_gang_num][GANG_SIZE] = ((sctu_gangs[sctu_gang_num][LAST_CORE_INDEX] - sctu_gangs[sctu_gang_num][FIRST_CORE_INDEX]) + 1);
				sctu_gang_num++;
			} 
		}
		else {
			sprintf(msg, "Only 1 core enabled in chip#%d. This instance can not run. Please enable more cores.\n", i);
			hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
			exit_flag = 1;
			return -1;
		}
	}

	/* Now total number of gangs are clear. Walk over each gang and figure out number of servers
 	 * and their cpu threads and keep it in an array. 
 	 */
	server_num = 0;
	for ( i=0; i<sctu_gang_num; i++ ) {
		/* Get all CPU thread details in this gang. */
		for ( j=sctu_gangs[i][FIRST_CORE], k=0; j<=sctu_gangs[i][LAST_CORE]; j++, k++ ) {
			get_cpus_in_core(j, cpus_in_core_gang[k]);
		}

		for ( j=0; j<sctu_gangs[i][MIN_SMT_GANG]; j++, server_num++ ) {
			for ( k=0; k<sctu_gangs[i][GANG_SIZE]; k++ ) {
				cpus_in_sctu_server[server_num][k] = cpus_in_core_gang[k][j];
			}
			sctu_server_size[server_num] = k;
		}
	}	

#if 0
	/* Printing logic. Only for debug. Comment out later */
	{
		for ( i=0; i<sctu_gang_num; i++ ) {
			printf("sctu_gang[%d] details :\nStart Core=%d, index=%d, end core=%d, index=%d, min_smt=%d, index=%d, total cores=%d\n",
			i, sctu_gangs[i][0], sctu_gangs[i][1], sctu_gangs[i][2], sctu_gangs[i][3], sctu_gangs[i][4], sctu_gangs[i][5], sctu_gangs[i][6]);
			printf("Cores in chip[%d] : ",i);
			for ( j=0;j<MAX_CORES_PER_CHIP; j++ ) {
				printf("%d ", cores_in_chip[i][j]);
			}
			printf("\n");
		}
	}
#endif

	/* Update required information in a file to be used by htxconf.awk */
	if ( strcasecmp(dinfo.device_name, "SCTU_DEV") == 0 ) {
		for ( i=0; i<server_num; i++ ) {
			fprintf(fptr, "sctu%-3d  : ", i);
			for ( j=0; j<sctu_server_size[i]; j++ ) {
				fprintf(fptr, "%4d  ", cpus_in_sctu_server[i][j]);
			}
			fprintf(fptr, "\n");
		}
		fclose(fptr);
	}
	else { /*Regular run. Figure out the server number from device number and populate bcpus */
		/* Error check. if dev number is greater than total server number, then some problem in device creation. Return with error */
		if ( dev_num >= server_num ) {
			sprintf(msg, "Device does not match any sctu server number. Wrong testcase_type associated. Check rulefile\nserver_num=%d, dev_num=%d\n", server_num, dev_num);
			hxfmsg(&hd, 0, HTX_HE_SOFT_ERROR, msg);
			return(-1);
		}

		num_cpus_to_test = sctu_server_size[dev_num];
		for ( i=0; i<num_cpus_to_test; i++ ) {
			bcpus_in_gang[i] = cpus_in_sctu_server[dev_num][i];
		}

		sprintf(msg, "Clients for %s are bound on cpus : ", dinfo.device_name);
		for ( i=0; i<num_cpus_to_test; i++ ) {
			sprintf(temp_msg, "%d ", bcpus_in_gang[i]);
			strcat(msg, temp_msg);
		}
		sprintf(temp_msg, "\n");
		strcat(msg, temp_msg);
		
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}
	return(0);
}


int node_testcase(void)
{
	int i, j, rc, rc1, rc2;
	volatile int stop_cond;
	int node, chip, core, thread, cpu_index, server_cpu_index;
	int num_nodes, sctu_server_num, core_sequential_num, server_sequential_num;
	int sctu_server[MAX_CORES][8], sctu_server_size[MAX_CORES];
	char temp_msg[20];
	FILE *fptr;

	sctu_server_num = 0;
	core_sequential_num=0;
	server_sequential_num=0;
	cpu_index = server_cpu_index = stop_cond = 0;
	num_nodes = Sys_stat.nodes;

	if ( strcasecmp(dinfo.device_name, "SCTU_DEV") == 0 ) {
		fptr = fopen("/tmp/htx_sctu_node_dev","w");

		if ( fptr == NULL ) {
			/* Below printf is correct. It should not be replaces with sprintf/hxfmsg. 
			 * That is because this code will run only with script. not during exerciser's
			 * regular run. And with script(devconfig) stdout is open. 
			 */
			printf("Unable to open /tmp/htx_sctu_node_dev. errno=%d(%s)", errno, strerror(errno));
			return(errno);
		}
	}
	if (shm_flag_for_malloc == 0) {
		rc = repopulate_syscfg(&hd);
		if ( rc ) {
			sprintf(msg,"repopulate_syscfg failed with error code= %d \n",rc);
			hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
			exit_flag = 1;
			return -1;
		}
	}


	rc = init_sctu_cpu_array();
	if ( rc ) {
		sprintf(msg,"init_sctu_cpu_array failed with error code= %d \n",rc);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		exit_flag = 1;
		return -1;
	}

	rc = get_hardware_stat(&Sys_stat);
	if ( rc ) {
		sprintf(msg, "get_hardware_stat call failed! rc = %d\n",rc);
		hxfmsg(&hd, -1, HTX_HE_SOFT_ERROR, msg);
		exit_flag = 1;
		return -1;
	}
		
#if 0
	printf("node=%d, chip=%d, core=%d, th=%d\n", Sys_stat.nodes, Sys_stat.chips, Sys_stat.cores, Sys_stat.cpus);
	for(node = 0; node < MAX_NODES; node++) {
		for(chip = 0; chip < MAX_CHIPS_PER_NODE; chip++) {
			for(core = 0; core < MAX_CORES_PER_CHIP; core++ ) {
				for(thread = 0; thread < MAX_CPUS_PER_CORE; thread++) {
					if ( global_ptr->syscfg.node[node].chip[chip].core[core].lprocs[thread] != -1 ) {
						printf("syscfg[%d][%d][%d][%d]=%d\n", node, chip, core, thread, global_ptr->syscfg.node[node].chip[chip].core[core].lprocs[thread]);
					}
				}
			}
		}
	} 
#endif
	rc1 = pthread_rwlock_rdlock(&(global_ptr->syscfg.rw));
	if (rc1 != 0) {
		sprintf(msg,"lock inside framework.c failed with errno=%d,in function: %s at line :[%d]\n",rc1, __FUNCTION__, __LINE__);
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}


	for ( node = 0; node < MAX_NODES && node < Sys_stat.nodes && stop_cond == 0; node++ ) {
		for ( thread = 0; thread < MAX_CPUS_PER_CORE && thread < Sys_stat.cpus && stop_cond == 0; thread++ ) {
			for ( core = 0; core < MAX_CORES_PER_CHIP && core < Sys_stat.cores && stop_cond == 0; core++ ) {
				for ( chip = 0; chip < MAX_CHIPS_PER_NODE && chip < Sys_stat.chips && stop_cond == 0; chip++ ) {
					if ( cpu_index >= Sys_stat.cpus ) {
						stop_cond = 1;
					}
						/* printf("node=%d, chip=%d, core=%d, th=%d, sctu_server_num=%d, server_cpu_index=%d, cpu_index=%d, cpu=%d\n", 
						node, chip, core, thread, sctu_server_num, server_cpu_index, cpu_index,
						global_ptr->syscfg.node[node].chip[chip].core[core].lprocs[thread]); */
					if ( global_ptr->syscfg.node[node].chip[chip].core[core].lprocs[thread] != -1 ) {
						sctu_server[sctu_server_num][server_cpu_index] = global_ptr->syscfg.node[node].chip[chip].core[core].lprocs[thread];
						sctu_server_size[sctu_server_num] = server_cpu_index;
						server_cpu_index++;
						cpu_index++;
						if ( (server_cpu_index % SCTU_GANG_SIZE) == 0 ) {
							sctu_server_num++; server_cpu_index=0;
						}
					}
				}
			}
		}
	}
	rc2 = pthread_rwlock_unlock(&(global_ptr->syscfg.rw));
	if (rc2 != 0) {
		sprintf(msg,"unlock inside framework.c failed with errno=%d,in function: %s at line :[%d]\n",rc2, __FUNCTION__, __LINE__);
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}


	if ( strcasecmp(dinfo.device_name, "SCTU_DEV") == 0 ) {
		for ( i = 0; i < sctu_server_num; i++ ) {
			fprintf(fptr, "sctu%-3d  : ", i);
			for ( j = 0; j < SCTU_GANG_SIZE; j++ ) {
				fprintf(fptr, "%4d  ", sctu_server[i][j]);
			}
			fprintf(fptr, "\n");
		}
		fclose(fptr);
	}
	else {
		/* Error check. if dev number is greater than total server number, then some problem in device creation. Return with error */
		if ( dev_num >= sctu_server_num ) {
			sprintf(msg, "Device does not match any sctu server number. Wrong testcase_type associated. Check rulefile\nserver_num=%d, dev_num=%d\n", sctu_server_num, dev_num);
			hxfmsg(&hd, 0, HTX_HE_SOFT_ERROR, msg);
			return(-1);
		}
		num_cpus_to_test = sctu_server_size[dev_num];

		for ( i = 0; i < num_cpus_to_test; i++ ) {
			bcpus_in_gang[i] = sctu_server[dev_num][i];
		}

		sprintf(msg, "Clients for %s are bound on cpus : ", dinfo.device_name);
		for ( i=0; i<num_cpus_to_test; i++ ) {
			sprintf(temp_msg, "%d ", bcpus_in_gang[i]);
			strcat(msg, temp_msg);
		}
		sprintf(temp_msg, "\n");
		strcat(msg, temp_msg);
		
		hxfmsg(&hd, 0, HTX_HE_INFO, msg);
	}
	return(0);
}

int init_sctu_cpu_array(void)
{
	int i, j,rc,rc1,rc2;
	int node, chip, core, thread;


	Sys_stat.nodes = 0;
	Sys_stat.chips = 0;
	Sys_stat.cores = 0;

	/* Initialize structures with -1*/
	for(i=0; i<SCTU_GANG_SIZE; i++) {
		cores_in_gang[i] = -1;
		bcpus_in_gang[i] = -1;
		for(j=0; j<MAX_THREADS_PER_CORE; j++)
			cpus_in_core_gang[i][j] = -1;
	}

	for(i=0; i<MAX_NODES; i++) {
		for(j=0; j <MAX_CORES_PER_NODE; j++) {
			cores_in_node[i][j] = -1;
		}
	}

	return(0);
}


#endif /* SCTU */

