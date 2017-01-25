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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/ipc.h>
#include <pthread.h>
#include <memory.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>  /* memcpy .. etc */
#include <sys/shm.h>
#include <limits.h>
#include "hxihtx64.h"
#include "htxsyscfg64.h"
#ifdef       _DR_HTX_
#include <sys/dr.h>
#endif

#define SUCCESS				0
#define FAILURE				-1

#define DEVICE_NAME  g_data.htx_d.sdev_id
#define RUN_MODE    g_data.htx_d.run_type
#define EXER_NAME   g_data.htx_d.HE_name

#define RUN_TYPE_LENGTH		4
#define MAX_STRING_LENTGH	32
#define RULE_FILE_NAME_LENGTH	80
#define BIND_TO_PROCESS     0
#define BIND_TO_THREAD      1
#define UNBIND_ENTITY       -1
#define UNSPECIFIED_LCPU    -1
#define MAX_STANZAS         41  /* Only 40 stanzas can be used */
#define MAX_NX_TASK         20
#define SW_PAT_OFF          0
#define SW_PAT_ON           1
#define SW_PAT_ALL          2
#define MIN_PATTERN_SIZE    8
#define MAX_PATTERN_SIZE    4096
#define MAX_RULE_LINE_SIZE  700     /* Inline with pattern text widths */

#define LS_BYTE             1       /* Load-Store Bytes */
#define LS_WORD             4       /* Load-Store Words */
#define LS_DWORD            8       /* Load-Store Double Words */

#define MIN_MEM_PERCENT     1
#define MIN_CPU_PERCENT		10
#define DEFAULT_MEM_PERCENT 100
#define DEFAULT_CPU_PERCENT 100
#define DEFAULT_MEM_ALOC_PERCENT 70
#define MAX_MEM_PERCENT     99          /* Not implemented */
#define MAX_CPU_PERCENT   	100
#define DEFAULT_DELAY       90
#define ADDR_PAT_SIGNATURE  0x414444525F504154  /* Hex equiv of "ADDR_PAT" */
#define RAND_PAT_SIGNATURE  0x52414E445F504154  /* Hex equiv of "RAND_PAT" */

#define MAX_CPUS_PER_SRAD   MAX_CPUS_PER_CHIP
#define MAX_L4_MASKS		2
#define MAX_CACHES			4
#define CACHE_LINE_SIZE		128
#define KB                  1024
#define MB                  ((unsigned long long)(1024*KB))
#define GB                  ((unsigned long long)(1024*MB))

/* Page indexes  0 - Index for page size 4k, 1 -Index for page size 64k,
 * 2 - Index for page size 2M,3 - Index for page size 16M,4 - Index for page size 16G
 */
#if 0
#define MAX_PAGE_SIZES	 5			/*RV:move to syscfg lib*/
#define PAGE_INDEX_4K    0			/* duplicate if syscfg library included*/
#define PAGE_INDEX_64K   1
#define PAGE_INDEX_2M	 2
#define PAGE_INDEX_16M   3
#define PAGE_INDEX_16G   4
#endif

/*move below macros to mem specific file later*/
#define MAX_STRIDE_SZ  4*KB

#define MIN_SEG_SIZE		8					/* 1 byte size*/
#define DEF_SEG_SZ_4K       256*MB
#define DEF_SEG_SZ_64K      DEF_SEG_SZ_4K
#define DEF_SEG_SZ_2M		DEF_SEG_SZ_4K
#define DEF_SEG_SZ_16M      DEF_SEG_SZ_4K
#define DEF_SEG_SZ_16G      16*GB

#define DBG_MUST_PRINT 0
#define DBG_IMP_PRINT 1
#define DBG_INFO_PRINT 2
#define DBG_DEBUG_PRINT 3

#define MAX_FILTERS             20
#define MAX_POSSIBLE_ENTRIES    200

#define STATS_VAR_INC(var, val) \
    pthread_mutex_lock(&g_data.tmutex);\
    g_data.htx_d.var += val; \
    pthread_mutex_unlock(&g_data.tmutex);

#define STATS_VAR_INIT(var, val) \
    pthread_mutex_lock(&g_data.tmutex); \
    g_data.htx_d.var = val; \
    pthread_mutex_unlock(&g_data.tmutex);

#ifndef __HTX_LINUX__
    #define STATS_HTX_UPDATE(stage)\
        pthread_mutex_lock(&g_data.tmutex); \
        hxfupdate(stage,&g_data.htx_d); \
        pthread_mutex_unlock(&g_data.tmutex);
#else
    #define STATS_HTX_UPDATE(stage) \
    hxfupdate(stage,&g_data.htx_d);
#endif

#ifdef DEBUG_ON
	#define debug(...)     displaym(__VA_ARGS__)
#else
	#define debug(...)
#endif
enum test_type { MEM=0, CACHE, FABRICB, TLB };
enum oper_type { OPER_MEM=0, OPER_DMA, OPER_RIM, OPER_TLB, OPER_MPSS, OPER_STRIDE, OPER_L4_ROLL, OPER_MBA, OPER_CORSA };
enum run_mode_type { RUN_MODE_NORMAL=0, RUN_MODE_CONCURRENT, RUN_MODE_COHERENT };
enum caches	{ L1=0,L2,L3,L4 };
enum affinity { LOCAL=1,REMOTE_CHIP,FLOATING,INTRA_NODE,INTER_NODE,ABOSOLUTE };

#define WRC                     3
#define MAX_PATTERN_TYPES       4
#define MAX_OPER_TYPES          4
#define MAX_MEM_ACCESS_TYPES    3 
enum pattern_size_type { PATTERN_SIZE_NORMAL=0, PATTERN_SIZE_BIG, PATTERN_ADDRESS, PATTERN_RANDOM };
enum basic_oper {WRITE=0,READ,COMPARE,RIM};
enum width_oper {DWORD=0,WORD,BYTE};

typedef int (*op_fptr)(int,void*,void*,int,void*,void*,void*,void*);

struct page_wise_seg_info {
	unsigned long seg_num;
	unsigned long shm_size; /* value get modified for segs if user gives mem filetr with size less 256MB(for4k and 64k pages)*/
    unsigned long original_shm_size;/*update "shm_size" variable with "original_shm_size" after every stanza*/
	unsigned long shmid;
	char *shm_mem_ptr;
	signed int  owning_thread	 : 12;
	unsigned int page_size_index : 5;/*0-4k,1-64k,2-2M,3-16M,4-16G*/
	unsigned int in_use		     : 1;
	signed int  cpu_chip_num	 : 8;
	signed int  mem_chip_num	 : 8;
};	
	
/*
 * page wise data structure having page wise free,
 * total and other page wise data.
 */
struct page_wise_data {
    int supported;
    unsigned long free;
    unsigned long avail;
    unsigned long psize;
	unsigned int num_of_segments;
	struct page_wise_seg_info *page_wise_seg_data;	
	/*memory filter specific data*/
	int  page_wise_usage_mpercent;
	unsigned long page_wise_usage_mem;
	int  page_wise_usage_npages; 
    unsigned int in_use_num_of_segments;
};

/*RV1: re check first 4 variable does make any sense at chip level mem details
create a new system level meminfo and embed chip level meminfo ( another structure in to it)*/
struct mem_info {
    unsigned long pspace_avail;/* total paging space available */ 
    unsigned long pspace_free; /* total paging space free */
    unsigned long total_mem_avail; /* Total memory (real) available */
    unsigned long total_mem_free;  /* Total memory (real) free */
    struct page_wise_data pdata[MAX_PAGE_SIZES];
};

/*This structure holds the srad related data */
struct chip_mem_pool_info{
	int has_cpu_and_mem;  /*flag,to check node has both cpus and  memory it,  0-only cpu or mem or no cpu and no mem.  1-has both cpu and mem*/
	int chip_id;
	int cpulist[MAX_CPUS_PER_SRAD];
	int num_cpus;  /* No of cpus in this srad */
	struct mem_info memory_details;
	/*cpu filter specific data*/
	int in_use_num_cpus;
	int in_use_cpulist[MAX_CPUS_PER_SRAD];/* active cpus list for a chip after applying cpu filter*/
	int is_chip_mem_in_use; /* based on memory filter update this var, FALSE  - mem not in use, TRUE  - in use*/
};

struct core_info{
    unsigned int core_num;
    unsigned int num_procs;
    unsigned int lprocs[MAX_CPUS_PER_CORE];
};

struct chip_info{
    unsigned int chip_num;
    unsigned int num_cores;
	unsigned int lprocs[MAX_CPUS_PER_CHIP];
    struct core_info core[MAX_CORES_PER_CHIP];
    struct mem_info mem_details;
};

struct node_info{
    unsigned int node_num;
    unsigned int num_chips;
	unsigned int lprocs[MAX_CPUS_PER_NODE];
    struct chip_info chip[MAX_CHIPS_PER_NODE];
};

struct cache_details {
	unsigned int cache_size;
	unsigned int cache_associativity;
	unsigned int cache_line_size;
};

struct sys_info {
	unsigned int os_pvr;
	unsigned int true_pvr;
	int smt_threads;
	int tot_cpus;
    unsigned int nodes;
    unsigned int chips;
    unsigned int cores;
	int shared_proc_mode;
    int unbalanced_sys_config;/* will be set, when any chip in lapr has only mem or only cpu it,not both*/
	struct node_info node[MAX_NODES];
	struct cache_details cinfo[MAX_CACHES];/*RV1: cinfo ===> cache_info*/
	struct mem_info memory_details; /*RV1: dont need to keep all mem deatils at system level*/
	int num_chip_mem_pools;
	struct chip_mem_pool_info chip_mem_pool_data[MAX_CHIPS];
	int shmmax;
	int shmall;
	int shmmni;
};

/*rule information data structure*/
struct cpu_filter_info{
    struct node_info node[MAX_NODES];
    int num_nodes;
};
struct mem_filter_info{
    struct node_info node[MAX_NODES];
    int num_nodes;
};
struct filter_info{
    struct cpu_filter_info cf;
    struct mem_filter_info mf;
};

struct global_rule{
    unsigned int global_alloc_mem_percent;
	unsigned int global_mem_4k_use_percent;
	unsigned int global_mem_64k_use_percent;
    int          global_alloc_huge_page;
	long 		 global_alloc_mem_size; 
    long         global_alloc_segment_size;
    int          global_debug_level;
    int          global_disable_cpu_bind;
    int			 global_startup_delay;    /* Seconds to sleep before starting the test */
	int 		 global_disable_filters;
	int 		 global_num_threads; /* used only in case of disable_filters = yes*/
};
/*RV1: Keep mem specific parameters only, remove others*/
struct rule_info {
    char  rule_id[16];      /* Rule Id                                        */
    char  pattern_id[9];   /* /htx/pattern_lib/xxxxxxxx                      */
    int   num_oper;        /* number of operations to be performed           */
                            /* 1 = default for invalid or no value            */
    int   num_writes;       /* 1 is default value */
    int   num_reads;        /* 1 is default value */
    int   num_compares;     /* 1 is default value */
    int   operation;        /* assigned to #defined values OPER_MEM.. etc    */
    int   run_mode;         /* assigned to #defined values MODE_NORMAL.. etc */
    int   misc_crash_flag;  /* 1 = crash_on_misc = ON and 0 = OFF */
    int   attn_flag;        /* 1 = turn on attention , 0 = OFF */
    int   compare_flag;     /* 1 = compare ON, 0 = compare OFF */
    int   disable_cpu_bind;     /* 1 = BINDING OFF(YES in rule file), 0  = BINDING ON(NO in rule file) */
    int   random_bind;      /* yes = random cpu bind on, no  = random cpu bind off */
    int   switch_pat;       /* 2 = Run (ALL) patterns for each segment,
                               1 = Switch pattern per segment is (ON)
                                   use a different pattern for each new segment,
                               0 = (OFF) Use the same pattern for all segments.
                               Note: For this variable to be effective multiple
                               patterns have to be specified */
    char  oper[8];         /* DMA or MEM or RIM                              */
    char  messages[4];     /* YES - put out info messages                    */
                            /* NO - do not put out info messages              */
    unsigned long seed;
    int    num_threads; /* Number of threads */
    int    percent_hw_threads; /* percentage of available H/W threads*/
    char  compare[4];      /* YES - compare                                  */
                            /* NO -  no compare                               */
    int   width;           /* 1, 4, or 8 bytes at a time                     */
	int   mem_percent;
	int	  cpu_percent;
    int   debug_level; /* Only 0 1 2 3 value .See the #define DBG_MUST_PRINT for more*/
    char  crash_on_mis[4];    /* Flag to enter the kdb in case of miscompare,   */
                    /* yes/YES/no/NO */
    char  turn_attn_on[4];    /* Flag to enter the attn in case of miscompare,  */
                        /* yes/YES/no/NO */
    char pattern_nm[52];
    int   num_patterns;
    unsigned int pattern_size[MAX_STANZA_PATTERNS];
    int   pattern_type[MAX_STANZA_PATTERNS]; /* pattern type (enum pattern_type) */
    char  pattern_name[MAX_STANZA_PATTERNS][72];
    char  *pattern[MAX_STANZA_PATTERNS];
    int affinity;
    char nx_mem_operations[72];
    int     number_of_nx_task;
    /*struct nx_task_table nx_task[MAX_NX_TASK];  NX specific*/
    char nx_reminder_threads_flag[10];
    int nx_rem_th_flag;
    char nx_performance_data[10];
    int nx_perf_flag;
    char nx_async[10];
    int nx_async_flag;
    int stride_sz;
    char mcs[8];
    int mem_l4_roll;
    long mcs_mask;
    unsigned int bm_position;
    unsigned int bm_length;
    char tlbie_test_case[20];
    char corsa_performance[10];
    int corsa_perf_flag;
	int mpss_seg_size;
    long  seg_size[MAX_PAGE_SIZES];    /* segment size in bytes*/
    int num_cpu_filters;
    int num_mem_filters;
    char cpu_filter_str[MAX_FILTERS][MAX_POSSIBLE_ENTRIES];
    char mem_filter_str[MAX_FILTERS][MAX_POSSIBLE_ENTRIES];
    struct filter_info filter;

};

/**********************************************
* Structures to hold system resource related  *
*  information 								  *
**********************************************/


/*memory page wise pool segment info*/
/*struct page_wise_seg_info {
    unsigned long page_size;
	unsigned int seg_num;
    unsigned int *shmids;
    unsigned int *shm_keys;
    unsigned long *shm_sizes;
    char **shr_mem_hptr;
	unsigned int segs_per_thred_srad[MAX_CPUS_PER_SRAD];
};*/


/****************************************************
*Memory exerciser specific thread structure			*
****************************************************/

struct mem_exer_thread_info{
    int num_segs;
	int cpu_flag;                               /*to mark,if thread is accessing any hole*/
    int bit_mask[MAX_L4_MASKS];					 /*L4*/
    unsigned int start_offset; 					 /*offset address for every thread in L4*/
    struct page_wise_seg_info *seg_details;
};
	

/*****************************************************
*A single thread structure collective of all nest exers
*****************************************************/

struct thread_data {
    int thread_num;
    pthread_t tid;
    pthread_t kernel_tid;
    pthread_attr_t  thread_attrs;
    int num_oper;
    int bind_proc;	/* logical cpu number to bind*/
    int thread_type;     /*mem,cache,fabricbus,tlbie.. etc*/

    void* testcase_thread_details; /* dynamically point to test case specific thread structures*/
	
};

/********************************************************
*Memory exer function structure							*
********************************************************/
/* Structure to print the encapsulated values in KDB trap */
struct segment_detail {
    unsigned long oper;
    unsigned long seg_num;
    unsigned long page_index;
    unsigned long width;
    unsigned long seg_size;
    unsigned long owning_thread;
    unsigned long cpu_owning_chip;
    unsigned long mem_owning_chip;
    unsigned long affinity_index;
    unsigned long thread_ptr_addr;
    unsigned long global_ptr_addr;    
    unsigned long sub_seg_num;
    unsigned long sub_seg_size;
    unsigned long num_times;
};


struct mem_exer_info {
	int bind_proc;								/*-1 in concurrent mode, locical cpu number in normal mode*/
	int dma_flag ; 								/* This flag is set if filesystem is diskless */
	int affinity;  						/*flag is set when affinity test is enabled*/
	int AME_enabled;
	int max_mem_percent;
    struct mem_exer_thread_info *mem_th_data;
    unsigned long long dummy_read_data;
    unsigned long total_segments;
    int shm_cleanup_done;
};
/*******************************************************
*GLOBAL structure:									   *
*******************************************************/
struct nest_global {  
	int dev_id;
	char rules_file_name[RULE_FILE_NAME_LENGTH];
	time_t rf_last_mod_time; 
	
	pid_t pid;
	int kdb_level;
	int test_type;		/* any of CACHE,MEM,FABRICBUS,TLBIE etc*/
	int exit_flag;
	int standalone;
	int cpu_DR_flag;
	int mem_DR_flag;

	int stanza_num; /* --- move to rule info */
	pthread_mutex_t tmutex;/*RV1: check to remove this*/
    int msg_n_err_logging_lock;/* Remove from and displaym function once updated with htx library */
	FILE *rf_ptr;
	struct sigaction sigvector; 				/* Signal handler data structure */
	struct rule_info stanza[MAX_STANZAS];		/* stores rule file stanza info*/
    struct global_rule gstanza;
	struct rule_info *stanza_ptr;				/* current rule staza pointer*/
	struct htx_data htx_d;
	struct sys_info sys_details;				/*syscfg library provided system information*/
    

	void* test_type_ptr;						/* dynamic pointer to any of test function structers*/
	struct thread_data *thread_ptr;
#if 0
	

	/*struct cache_test_info  *cache_test_ptr;
	struct fabric_test_info *fab_test_ptr;
	struct l4_mba_test_info *l4_mba_test_ptr; 
	<add other test case ptrs>
	*/
#endif
	void *current_test_ptr;  /* dynamic pointer to any of test sub  function structers*/
	

};

int test_var;

int fill_exer_huge_page_requirement(void);
void print_partition_config(void);
int displaym(int,int , const char *,...);
int read_rules(void);
int mem_exer_opearation(void);

void SIGCPUFAIL_handler(int,int, struct sigcontext *);
void reg_sig_handlers(void);
int read_cmd_line(int argc,char *argv[]);
void get_test_type(void);
void get_cache_details(void);
int get_system_details(void);
int apply_process_settings(void);
int modify_shared_mem_limits_linux(unsigned long);

#ifdef __HTX_LINUX__
extern void SIGUSR2_hdl(int, int, struct sigcontext *);
#endif

/*Normal pattern size operation functions*/
int mem_operation_write_dword(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_write_word(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_write_byte(int,void *,void *,int,void *,void *,void *,void*);

int mem_operation_read_dword(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_read_word(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_read_byte(int,void *,void *,int,void *,void *,void *,void*);

int mem_operation_comp_dword(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_comp_word(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_comp_byte(int,void *,void *,int,void *,void *,void *,void*);

int mem_operation_rim_dword(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_rim_word(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_rim_byte(int,void *,void *,int,void *,void *,void *,void*);

/*Big size pattern file operation functions*/
int pat_operation_write_dword(int,void *,void *,int,void *,void *,void *,void*);
int pat_operation_write_word(int,void *,void *,int,void *,void *,void *,void*);
int pat_operation_write_byte(int,void *,void *,int,void *,void *,void *,void*);

int pat_operation_comp_dword(int,void *,void *,int,void *,void *,void *,void*);
int pat_operation_comp_word(int,void *,void *,int,void *,void *,void *,void*);
int pat_operation_comp_byte(int,void *,void *,int,void *,void *,void *,void*);

int pat_operation_rim_dword(int,void *,void *,int,void *,void *,void *,void*);
int pat_operation_rim_word(int,void *,void *,int,void *,void *,void *,void*);
int pat_operation_rim_byte(int,void *,void *,int,void *,void *,void *,void*);

/*Address pattern operation functions*/
int mem_operation_write_addr(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_comp_addr(int,void *,void *,int,void *,void *,void *,void*);
int mem_operation_write_addr_comp(int,void *,void *,int,void *,void *,void *,void*);

/*Random type pattern operation functions*/
int rand_operation_write_dword(int,void *,void *,int,void *,void *,void *,void*);
int rand_operation_write_word(int,void *,void *,int,void *,void *,void *,void*);
int rand_operation_write_byte(int,void *,void *,int,void *,void *,void *,void*);

int rand_operation_comp_dword(int,void *,void *,int,void *,void *,void *,void*);
int rand_operation_comp_word(int,void *,void *,int,void *,void *,void *,void*);
int rand_operation_comp_byte(int,void *,void *,int,void *,void *,void *,void*);

int rand_operation_rim_dword(int,void *,void *,int,void *,void *,void *,void*);
int rand_operation_rim_word(int,void *,void *,int,void *,void *,void *,void*);
int rand_operation_rim_byte(int,void *,void *,int,void *,void *,void *,void*);

/*nstride operation related functions*/
int do_stride_operation(int);
int write_dword(void*, int,int,int, unsigned int*);
int read_dword(void*,int,int);
int read_comp_dword(void*,int,int, int, unsigned int*,int,struct segment_detail*);
int write_word(void*,int,int,int,unsigned int*);
int read_word(void*,int,int);
int read_comp_word(void*,int,int,int,unsigned int*,int,struct segment_detail*);
int write_byte(void*,int,int,int,unsigned int*);
int read_byte(void*,int,int);
int read_comp_byte(void*,int,int,int,unsigned int*,int,struct segment_detail*);
#ifdef __HTX_LINUX__
int do_trap_htx64 (unsigned long arg1,
                       unsigned long arg2,
                       unsigned long arg3,
                       unsigned long arg4,
                       unsigned long arg5,
                       unsigned long arg6,
                       unsigned long arg7);
#endif

