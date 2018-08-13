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

/*
 * Debug messages levels (for displaying)
 * 0 (MUST DISPLAY MESSAGES), 1 (VERY IMPORTANT DISPLAY MESSAGES) ,
 * 2 (INFORMATIVE DISPLAY MESSAGES ), 3 (DEBUG PURPOSE DISPLAY MESSAGES ).
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
#ifdef __HTX_LINUX__
#include <asm/ioctl.h>
#endif
#include <limits.h>
#include "hxihtx64.h"
#include "htxsyscfg64.h"
#ifdef       _DR_HTX_
#include <sys/dr.h>
#endif

#define SUCCESS				0
#define FAILURE				-1

#define SET                 1
#define UNSET               0

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
#define DEFAULT_INSTANCE    0
#define MAX_STANZAS         41  /* Only 40 stanzas can be used */
#define MAX_NX_TASK         20
#define SW_PAT_OFF          0
#define SW_PAT_ON           1
#define SW_PAT_ALL          2
#define MIN_PATTERN_SIZE    8
#define MAX_PATTERN_SIZE    4096
#define MAX_RULE_LINE_SIZE  700     /* Inline with pattern text widths */

#define DO_NOT_ALLOCATE_MEM 0
#define ALLOCATE_MEM        1

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
#define STATS_UPDATE_INTERVAL 30
#define ADDR_PAT_SIGNATURE  0x414444525F504154  /* Hex equiv of "ADDR_PAT" */
#define RAND_PAT_SIGNATURE  0x52414E445F504154  /* Hex equiv of "RAND_PAT" */

#define MAX_CPUS_PER_SRAD   MAX_CPUS_PER_CHIP
#define MAX_L4_MASKS		2
#define MAX_CACHES			4
#define CACHE_LINE_SIZE		128
#define KB                  1024
#define MB                  ((unsigned long long)(1024*KB))
#define GB                  ((unsigned long long)(1024*MB))

#define POWER7               0x3f
#define POWER7P              0x4a
#define POWER8_MURANO        0x4b
#define POWER8_VENICE        0x4d
#define POWER9_NIMBUS        0x4e
#define POWER9_CUMULUS       0x4f
#define POWER8P_GARRISION    0x4c

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
/*cache exer specific*/
#define PAGE_FREE           0
#define CACHE_PAGE          1
#define PAGE_HALF_FREE      2
#define PREFETCH_PAGE       3
#define PREFETCH_MEM_SZ     8*MB

/* Maximum HUGE pages to be allocated */
#define MEM_MULT_FACTOR 4  /*extra memory mult factor to find desired contiguous memory*/
#define MAX_HUGE_PAGES_PER_CACHE_THREAD 10
#define MAX_CACHE_THREADS_PER_CORE      2
#define MAX_CACHE_THREADS_PER_CHIP      (MAX_CACHE_THREADS_PER_CORE * MAX_CORES_PER_CHIP)
#define MAX_HUGE_PAGES      (MAX_HUGE_PAGES_PER_CACHE_THREAD * MAX_CACHE_THREADS_PER_CHIP * MEM_MULT_FACTOR)
/* Prefetch Algorithms */
#define PREFETCH_OFF        0
#define PREFETCH_IRRITATOR  1
#define PREFETCH_NSTRIDE    2
#define PREFETCH_PARTIAL    4
#define PREFETCH_TRANSIENT  8
#define PREFETCH_NA                         16
#define RR_ALL_ENABLED_PREFETCH_ALGORITHMS  32
#define MAX_PREFETCH_ALGOS  5
#define MAX_PREFETCH_STREAMS        16
#define PREFETCH_MEMORY_PER_THREAD (8*MB)

/* DSCR types */
#define DSCR_DEFAULT        0
#define DSCR_RANDOM         1
#define DSCR_LSDISABLE      2

#define DBG_MUST_PRINT 0
#define DBG_IMP_PRINT 1
#define DBG_INFO_PRINT 2
#define DBG_DEBUG_PRINT 3

#define MAX_FILTERS             20
#define MAX_POSSIBLE_ENTRIES    200

#define THREADS_FAB		0
#define CORES_FAB		1
#define STATS_VAR_INC(var, val) \
    if(exer_err_halt_status(&g_data.htx_d)){ \
        STATS_HTX_UPDATE(UPDATE) \
    }else { \
        pthread_mutex_lock(&g_data.tmutex); \
        g_data.nest_stats.var += val; \
        pthread_mutex_unlock(&g_data.tmutex);\
    } \


#define STATS_VAR_INIT(var, val) \
    pthread_mutex_lock(&g_data.tmutex); \
    g_data.htx_d.var = val; \
    pthread_mutex_unlock(&g_data.tmutex);

/*#ifndef __HTX_LINUX__*/
#if 0
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
enum test_type { MEM=0,FABRICB,TLB,CACHE };
enum mem_oper_type { OPER_MEM=0,OPER_STRIDE, OPER_RIM, OPER_DMA, OPER_RAND_MEM, LATENCY_TEST, OPER_MPSS, OPER_L4_ROLL, OPER_MBA, OPER_CORSA };
enum caches	{ L1=0,L2,L3,L4 };
enum cache_opers {NONE=0,OPER_CACHE,OPER_PREFETCH};
enum cache_tests {CACHE_BOUNCE_ONLY=0,CACHE_BOUNCE_WITH_PREF,CACHE_ROLL_ONLY,CACHE_ROLL_WITH_PREF,PREFETCH_ONLY};
enum affinity { LOCAL=1,REMOTE_CHIP,FLOATING,INTRA_NODE,INTER_NODE,ABOSOLUTE,ALL_FABRIC};
enum pattern_size_type { PATTERN_SIZE_NORMAL=0, PATTERN_SIZE_BIG, PATTERN_ADDRESS, PATTERN_RANDOM,RANDOM_MEM_ACCESS };
enum data_oper {WRITE=0,READ,COMPARE,RIM};
enum width_oper {DWORD=0,WORD,BYTE};

#define WRC                     3
#define MAX_PATTERN_TYPES       5
#define MAX_TEST_TYPES          4 /*MEM,FABRICB,TLB,CACHE*/
#define MAX_MEM_OPER_TYPES	    10
#define MAX_DATA_OPER_TYPES     4 /*write,read,comp,rim*/
#define MAX_MEM_ACCESS_TYPES    3 /*width (8,4,1)*/

#define LATENCY_TEST_OPERS      1000000
#define LATENCY_TEST_SEGMENTS   3

typedef int (*exer_mem_alloc_fptr)(void);
typedef int (*exer_stanza_oper_fptr)(void);
typedef int (*mem_data_op_fptr)(unsigned long,void*,void*,int,void*,void*,void*,void*);
typedef int (*mem_op_fptr)(int);

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

struct mem_info {
    unsigned long pspace_avail;/* total paging space available */ 
    unsigned long pspace_free; /* total paging space free */
    unsigned long total_mem_avail; /* Total memory (real) available */
    unsigned long total_mem_free;  /* Total memory (real) free */
    struct page_wise_data pdata[MAX_PAGE_SIZES];
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
    unsigned int num_cores;
	unsigned int lprocs[MAX_CPUS_PER_NODE];
    struct chip_info chip[MAX_CHIPS_PER_NODE];
};
/*This structure holds the srad related data */
struct chip_mem_pool_info{
	int has_cpu_and_mem;  /*flag,to check node has both cpus and  memory it,  0-only cpu or mem or no cpu and no mem.  1-has both cpu and mem*/
	int chip_id;
	int cpulist[MAX_CPUS_PER_SRAD];
	int num_cpus;  /* No of cpus in this srad */
	struct mem_info memory_details;
    int num_cores;
    struct core_info core_array[MAX_CORES_PER_CHIP];
	/*cpu filter specific data*/
	int in_use_num_cpus;
    int inuse_num_cores;
	int in_use_cpulist[MAX_CPUS_PER_SRAD];/* active cpus list for a chip after applying cpu filter*/
	int is_chip_mem_in_use; /* based on memory filter update this var, FALSE  - mem not in use, TRUE  - in use*/
    struct core_info inuse_core_array[MAX_CORES_PER_CHIP];
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
	struct cache_details cache_info[MAX_CACHES];
	struct mem_info memory_details; /*RV1: dont need to keep all mem deatils at system level*/
	int num_chip_mem_pools;
	struct chip_mem_pool_info chip_mem_pool_data[MAX_CHIPS];
	unsigned long shmmax;
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
    double global_alloc_mem_percent;
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
	int 		 global_fab_links_drive_type; /* flag to mark whether cores or threads to distribute accross X/A links  in fab exer algorithm*/
};
struct rule_info {
    char                    rule_id[32];                /* Rule Id                                        */
	int                     stanza_num; 
    char                    pattern_id[9];              /* /htx/pattern_lib/xxxxxxxx                      */
    int                     num_oper;                   /* number of operations to be performed           */
                                                        /* 1 = default for invalid or no value            */
    int                     num_writes;                 /* 1 is default value */
    int                     num_reads;                  /* 1 is default value */
    int                     num_compares;               /* 1 is default value */
    int                     operation;                  /* assigned to #defined values OPER_MEM.. etc    */
    int                     run_mode;                   /* assigned to #defined values MODE_NORMAL.. etc */
    int                     misc_crash_flag;            /* 1 = crash_on_misc = ON and 0 = OFF */
    int                     attn_flag;                  /* 1 = turn on attention , 0 = OFF */
    int                     compare_flag;               /* 1 = compare ON, 0 = compare OFF */
    int                     disable_cpu_bind;           /* 1 = BINDING OFF(YES in rule file), 0  = BINDING ON(NO in rule file) */
    int                     random_bind;                /* yes = random cpu bind on, no  = random cpu bind off */
    int                     switch_pat;                 /* 2 = Run (ALL) patterns for each segment,
                                                        1 = Switch pattern per segment is (ON)
                                                           use a different pattern for each new segment,
                                                        0 = (OFF) Use the same pattern for all segments.
                                                        Note: For this variable to be effective multiple
                                                        patterns have to be specified */
    char                    oper[8];                    /* DMA or MEM or RIM                              */
    char                    messages[4];                /* YES - put out info messages                    */
                                                        /* NO - do not put out info messages              */
    unsigned long           seed;
    int                     num_threads;                /* Number of threads */
    int                     percent_hw_threads;         /* percentage of available H/W threads*/
    char                    compare[4];                 /* YES - compare                                  */
                                                        /* NO -  no compare                               */
    int                     width;                      /* 1, 4, or 8 bytes at a time                     */
	int                     mem_percent;
	int	                    cpu_percent;
    int                     debug_level;                /* Only 0 1 2 3 value .See the #define DBG_MUST_PRINT for more*/
    char                    crash_on_mis[4];            /* Flag to enter the kdb in case of miscompare,   */
                                                        /* yes/YES/no/NO */
    char                    turn_attn_on[4];            /* Flag to enter the attn in case of miscompare,  */
                                                        /* yes/YES/no/NO */
    char                    pattern_nm[52];
    int                     num_patterns;
    unsigned int            pattern_size[MAX_STANZA_PATTERNS];
    int                     pattern_type[MAX_STANZA_PATTERNS]; /* pattern type (enum pattern_type) */
    char                    pattern_name[MAX_STANZA_PATTERNS][72];
    char                    *pattern[MAX_STANZA_PATTERNS];
    int                     affinity;
    /*
    char nx_mem_operations[72];
    int     number_of_nx_task;
    char nx_reminder_threads_flag[10];
    int nx_rem_th_flag;
    char nx_performance_data[10];
    int nx_perf_flag;
    char nx_async[10];
    int nx_async_flag;*/
    int                     stride_sz;
    char                    tlbie_test_case[20];
    char                    corsa_performance[10];
    int                     corsa_perf_flag;
	int                     mpss_seg_size;
    long                    seg_size[MAX_PAGE_SIZES];    /* segment size in bytes*/
    int                     num_cpu_filters;
    int                     num_mem_filters;
    char                    cpu_filter_str[MAX_FILTERS][MAX_POSSIBLE_ENTRIES];
    char                    mem_filter_str[MAX_FILTERS][MAX_POSSIBLE_ENTRIES];
    struct                  filter_info filter;

    /*cache exer specific*/
    int                     tgt_cache;
    int                     target_set;
    int                     cache_test_case;                    /*CACHE_BOUNCE_ONLY,CACHE_BOUNCE_WITH_PREF,CACHE_ROLL_ONLY,CACHE_ROLL_WITH_PREF,PREFETCH_ONLY*/
    unsigned int            pf_irritator;                       /* Prefetch irritator on/off                                    */
    unsigned int            pf_nstride;                         /* Prefetch n-stride  on/off                                    */
    unsigned int            pf_partial;                         /* Prefetch partial  on/off                                     */
    unsigned int            pf_transient;                       /* Prefetch transient  on/off                                   */
    unsigned int            pf_dcbtna;                          /* Prefetch DCBTNA on/off                                       */
    unsigned int            pf_dscr;                            /* Prefetch Randomise DSCR on/off                               */
    unsigned int            pf_conf;                            /* Prefetch configuration, a unique state of a combination of   */
    unsigned int            prefetch_memory_size;               /* The amount of memory which will be prefetched.               */
    unsigned int            cache_memory_size;                  /* The amount of memory that will be written by cache threads   */
    int                     num_cache_threads_created;          /* Number of cache threads actually created in this rule.       */
    int                     num_prefetch_threads_created;       /* Number of prefetch threads actually created in this rule.    */
    int                     num_cache_threads_to_create;        /* Number of cache threads needed to be created in this rule.   */
    int                     num_prefetch_threads_to_create;     /* Number of prefetch threads needed to be created in this rule.*/
    int                     use_contiguous_pages;               /* TRUE or FALSE, indicates we are looking for contiguous pages */

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
    double write_latency;
    double read_latency;
    unsigned long th_seed;
    unsigned long rand_expected_value;           /*expected value track in random pattern,usefull in miscompare print*/
	struct drand48_data buffer;
    /*rand_debug*/
    unsigned long rc_pass_count;
    unsigned long rc_offset;
};
	
/****************************************************
*Cache exerciser specific thread structure         *
****************************************************/

struct cache_exer_thread_info{
    unsigned long long      pattern;
    int                     oper_type;                        /* Possible values PREFETCH, CACHE, ALL*/
    int                     prefetch_algorithm;                 /* Used by prefetch threads to determine which prefetch algorithm to run.   */
    int                     start_class;
    int                     end_class;
    int                     walk_class_jump;
    int                     offset_within_cache_line;
    unsigned char           *contig_mem[MAX_HUGE_PAGES_PER_CACHE_THREAD];
    unsigned char           *prefetch_memory_start_address;
    int                     found_cont_pages;
    int                     prefetch_streams;                   /* The maximum number of prefetch streams for the thread. Valid only for prefetch threads.  */
    unsigned long long      read_dscr_val;                      /* The DSCR value that is read from SPR 3.                                                  */
    unsigned long long      written_dscr_val;                   /* The DSCR value that is written to SPR 3.                                                 */
    int                     pages_to_write;                     /* Number of HUGEB pages written by ( cache ) thread.                                       */
    int                     num_mem_sets;                       /* Number of sets of contiguous memory to be written by ( cache ) thread.                   */
    struct drand48_data     buffer;
    long int                random_pattern;
    long int                seedval;
    unsigned int            prev_seed;
    unsigned int            saved_seed;
    unsigned long long      prefetch_scratch_mem[64/sizeof(unsigned long long)];
};

/*****************************************************
*A single thread structure collective of all nest exers
*****************************************************/

struct thread_data {
    int thread_num;
    pthread_t tid;
    pthread_t kernel_tid;
    pthread_attr_t  thread_attrs;
    int current_num_oper;
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
    int memory_allocation;              /*used as flag to decide memory allocation is needed or not*/
};

/*fabric bus exerciser specific structures*/
struct dest_chip_details{
	long chip_num;
	long seg_num;
};

struct fabb_exer_info {/* RFB1: keep a pointer of this strucure in global str*/
    unsigned long seg_size;
    long          fab_chip_L3_sz[MAX_CHIPS];
	int 		  fab_cores[MAX_CHIPS][MAX_CORES_PER_CHIP];
    unsigned long segs_per_chip[MAX_CHIPS];
    struct dest_chip_details dest_chip[MAX_CPUS];

};
/********************************************************************
*cache exerciser specific structures*
********************************************************************/
struct cache_mem_info {
    int                 huge_page_index;
    unsigned long       huge_page_size;
    int                 prefetch_page_index;
    unsigned long long  ra[MAX_HUGE_PAGES];
    unsigned char       *ea[MAX_HUGE_PAGES];
    unsigned char       *prefetch_ea;
    int                 num_pages;
    int                 page_status[MAX_HUGE_PAGES];
};

struct huge_page_info {
    int page_num;
    int in_use;
    unsigned char *ea;
};
struct contiguous_mem_info{
    int num_pages;
    struct huge_page_info* huge_pg;
    int in_use_mem_chunk;
};
struct cache_exer_info {
    unsigned long contiguous_mem_required;
    unsigned long worst_case_cache_memory_required;
    unsigned long worst_case_prefetch_memory_required;
    unsigned long max_possible_cache_threads;
    unsigned long max_possible_prefetch_threads;
    struct  cache_mem_info cache_pages;
    struct  contiguous_mem_info *cont_mem;
    int     num_cont_mem_chunks;
    struct cache_exer_thread_info *cache_th_data;
	unsigned int cache_instance;   /* used to over write static cache insance with srad no.,incase of imbalced config*/
};

struct nest_stats_info {
    unsigned long bytes_writ;
    unsigned long bytes_read;
    unsigned long bad_others;

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

    int num_stanzas;
	pthread_mutex_t tmutex;/*RV1: check to remove this*/
    int msg_n_err_logging_lock;/* Remove from and displaym function once updated with htx library */
	FILE *rf_ptr;
	struct sigaction sigvector; 				/* Signal handler data structure */
	struct rule_info stanza[MAX_STANZAS];		/* stores rule file stanza info*/
    struct global_rule gstanza;
	struct rule_info *stanza_ptr;				/* current rule staza pointer*/
	struct htx_data htx_d;
	struct sys_info sys_details;				/*syscfg library provided system information*/

    struct nest_stats_info nest_stats;    

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
void print_partition_config(int);
int displaym(int,int , const char *,...);
int read_rules(void);
int nest_framework_fun(void);
int fill_per_chip_segment_details(void);
int fill_system_segment_details(void);
int run_mem_stanza_operation(void);
void SIGCPUFAIL_handler(int,int, struct sigcontext *);
void reg_sig_handlers(void);
int read_cmd_line(int argc,char *argv[]);
void get_test_type(void);
void get_cache_details(void);
int get_system_details(void);
int apply_process_settings(void);
int modify_shared_mem_limits_linux(unsigned long);
int dump_miscompared_buffers(int,unsigned long,int,int,unsigned long *,int,int,int,struct segment_detail*);
int log_mem_seg_details(void);
int parse_cpu_filter(char[MAX_FILTERS][MAX_POSSIBLE_ENTRIES]);
int parse_mem_filter(char[MAX_FILTERS][MAX_POSSIBLE_ENTRIES]);
int allocate_mem_for_threads(void);
void release_thread_resources(int);
int remove_shared_memory(void);
int apply_filters(void);
void print_memory_allocation_seg_details(int);
int fun_NULL(void);
#ifdef __HTX_LINUX__
extern void SIGUSR2_hdl(int, int, struct sigcontext *);
#endif

/*Normal pattern size operation functions*/
int mem_operation_write_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_write_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_write_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

int mem_operation_read_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_read_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_read_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

int mem_operation_comp_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_comp_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_comp_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

int mem_operation_rim_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_rim_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_rim_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

/*Big size pattern file operation functions*/
int pat_operation_write_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int pat_operation_write_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int pat_operation_write_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

int pat_operation_comp_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int pat_operation_comp_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int pat_operation_comp_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

int pat_operation_rim_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int pat_operation_rim_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int pat_operation_rim_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

/*Address pattern operation functions*/
int mem_operation_write_addr(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_comp_addr(unsigned long,void *,void *,int,void *,void *,void *,void*);
int mem_operation_write_addr_comp(unsigned long,void *,void *,int,void *,void *,void *,void*);

/*Random type pattern operation functions*/
int rand_operation_write_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_operation_write_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_operation_write_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

int rand_operation_comp_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_operation_comp_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_operation_comp_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

int rand_operation_rim_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_operation_rim_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_operation_rim_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

/*Random memory access/latency test operation functions*/
int rand_mem_access_wr_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_mem_access_rd_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_mem_access_cmp_dword(unsigned long,void *,void *,int,void *,void *,void *,void*);

int rand_mem_access_wr_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_mem_access_rd_word(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_mem_access_cmp_word(unsigned long,void *,void *,int,void *,void *,void *,void*);

int rand_mem_access_wr_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_mem_access_rd_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);
int rand_mem_access_cmp_byte(unsigned long,void *,void *,int,void *,void *,void *,void*);

/*unsigned long get_random_no_64(unsigned int*);*/
unsigned long get_random_no_64(struct drand48_data*);
unsigned int  get_random_no_32(struct drand48_data*);
unsigned char get_random_no_8(struct drand48_data*);

/*nstride operation related functions*/
int do_stride_operation(int);
int write_dword(void*,unsigned long ,int,int,struct mem_exer_thread_info*);
int read_dword(void*,unsigned long,int);
int read_comp_dword(void*,unsigned long,int, int,int,struct segment_detail*);
int write_word(void*,unsigned long,int,int,struct mem_exer_thread_info*);
int read_word(void*,unsigned long,int);
int read_comp_word(void*,unsigned long,int,int,int,struct segment_detail*);
int write_byte(void*,unsigned long,int,int,struct mem_exer_thread_info*);
int read_byte(void*,unsigned long,int);
int read_comp_byte(void*,unsigned long,int,int,int,struct segment_detail*);

/*fabricbus specific modules*/
int set_fabricb_exer_page_preferances(void);
int memory_segments_calculation(void);
int fill_fabb_segment_details(int);
int modify_fabb_shmsize_based_on_cpufilter(int,int);
int fill_fabb_thread_structure(struct chip_mem_pool_info*,int);
int run_tlb_operaion(void);
/*cache exer specific modules*/
int fill_cache_exer_mem_req(void);
int setup_memory_to_use(void);
int cache_exer_operations(void);
void* prefetch_thread_function(void*);
void* cache_thread_function(void*);
int find_EA_to_RA(unsigned long ,unsigned long long* );
int prefetch_randomise_dscr(unsigned long long , unsigned int , unsigned int);
void  partial_dcbt(unsigned long long , unsigned long long , unsigned long long ,unsigned long long*);
int do_prefetch( unsigned long long,unsigned long long,unsigned long long,unsigned int,unsigned long long,unsigned long long);
int get_random_number_pref(int);
int prefetch(unsigned long long, unsigned long long,unsigned long long);
int transient_dcbt(unsigned long long,unsigned long long,unsigned long long);
int prefetch_dcbtna(unsigned long long,unsigned long long,unsigned long long);
int n_stride(unsigned long long,unsigned long long,unsigned long long,long long unsigned int *);
int dump_miscompare_data(int,void*);
