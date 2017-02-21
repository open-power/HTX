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
/* @(#)75       1.4.1.1  src/htx/usr/lpp/htx/bin/hxemem64/hxemem64.h, exer_mem64, htx61J 8/25/09 11:32:33 */

/* This header file contains only those that are used by both mpss testcase [mpss.c] and hxemem.c only */

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
#include "hxihtx64.h"
#include "htxsyscfg64.h"
#define BIND_TO_PROCESS     0
#define BIND_TO_THREAD      1
#define UNBIND_ENTITY       -1

#define KB                  1024
#define MB                  ((unsigned long long)(1024*KB))
#define GB                  ((unsigned long long)(1024*MB))

#define DEF_SEG_SZ_4K       256*MB
#define DEF_SEG_SZ_64K      DEF_SEG_SZ_4K
#define DEF_SEG_SZ_2M       DEF_SEG_SZ_4K
#define DEF_SEG_SZ_16M      DEF_SEG_SZ_4K
#define DEF_SEG_SZ_16G      16*GB
#define STATS_VAR_INC(var, val) \
    pthread_mutex_lock(&mem_info.tmutex);\
    stats.var += val; \
    pthread_mutex_unlock(&mem_info.tmutex);

#define STATS_VAR_INIT(var, val) \
    pthread_mutex_lock(&mem_info.tmutex); \
    stats.var = val; \
       pthread_mutex_unlock(&mem_info.tmutex);

#define STATS_HTX_UPDATE(stage)\
    pthread_mutex_lock(&mem_info.tmutex); \
    hxfupdate(stage,&stats); \
    pthread_mutex_unlock(&mem_info.tmutex);

#define DBG_MUST_PRINT 0
#define DBG_IMP_PRINT 1
#define DBG_INFO_PRINT 2
#define DBG_DEBUG_PRINT 3

/* Page indexes  0 - Index for page size 4k, 1 -Index for page size 64k,
 * 2 - Index for page size 2M, 3 - Index for page size 16M, 4 - Index for page size 16G.
 */
#define PAGE_INDEX_4K    0
#define PAGE_INDEX_64K   1
#define PAGE_INDEX_2M    2
#define PAGE_INDEX_16M   3
#define PAGE_INDEX_16G   4

#define LINUX_DMA_BLOCK_SIZE 1024

enum oper_type { OPER_MEM=0, OPER_DMA, OPER_RIM, OPER_TLB, OPER_MPSS, OPER_EMPSS, OPER_NX_MEM, OPER_NSTRIDE, OPER_L4_ROLL, OPER_MBA, OPER_CORSA };
enum run_mode_type { RUN_MODE_NORMAL=0, RUN_MODE_CONCURRENT, RUN_MODE_COHERENT };
enum pattern_type { PATTERN_SIZE_NORMAL=0, PATTERN_SIZE_BIG, PATTERN_ADDRESS, PATTERN_RANDOM };
enum nx_mem_oper_type { CDC_CRC=0, CDC, BYPASS, C_CRC, C_ONLY };

typedef unsigned long long       uint64;
typedef unsigned int             uint32;

#define MAX_NUM_PAGE_SIZES  5   /* 4K, 64K, 2M,16M, 16G */
#define SW_PAT_OFF			0
#define SW_PAT_ON			1
#define SW_PAT_ALL			2

#define MAX_NX_OPER         5
#define THREAD_OFFSET       32
#define MAX_NX_TASK         20
#define MAX_NX_FIELD        5
#define MAX_P7_NODE         8
#define MAX_P7_CHIP         32
#define DEFAULT_RULE        0
#define CHIP_SPECIFIC       1
#define THREAD_ACTIVE       1

struct nx_task_table {
    int nx_operation;
    float thread_percent;
    unsigned int nx_min_buf_size;
    unsigned int nx_max_buf_size;
    int chip;
};

struct rule_format {
    char  rule_id[9];      /* Rule Id                                        */
    char  pattern_id[9];   /* /htx/pattern_lib/xxxxxxxx                      */
    int   num_oper;        /* number of operations to be performed           */
                            /* 1 = default for invalid or no value            */
	int	  num_writes;       /* 1 is default value */
	int   num_reads;		/* 1 is default value */
	int   num_compares;		/* 1 is default value */
	int   operation;		/* assigned to #defined values OPER_MEM.. etc	 */
	int   run_mode;			/* assigned to #defined values MODE_NORMAL.. etc */
	int   max_mem_flag; 		/* 1 = max_mem= yes and 0 NO	*/
	int   misc_crash_flag;	/* 1 = crash_on_misc = ON and 0 = OFF */
	int   attn_flag; 		/* 1 = turn on attention , 0 = OFF */
	int   compare_flag;		/* 1 = compare ON, 0 = compare OFF */
	int   bind_proc;		/* 1 = BINDING ON, 0 = BINDING OFF */
	int   bind_method;              /* 1 = random cpu bind on, 0 = random cpu bind off */
	int   switch_pat;		/* 2 = Run (ALL) patterns for each segment,
							   1 = Switch pattern per segment is (ON)
							       use a different pattern for each new segment,
							   0 = (OFF) Use the same pattern for all segments.
							   Note: For this variable to be effective multiple
							   patterns have to be specified */
    char  oper[4];         /* DMA or MEM or RIM                              */
    char  messages[4];     /* YES - put out info messages                    */
                            /* NO - do not put out info messages              */
    char mode[16]; /* CONCURRENT, SINGLE(NORMAL) */
    int    num_threads; /* Number of threads */
	int    percent_hw_threads; /* percentage of available H/W threads*/
    char  compare[4];      /* YES - compare                                  */
                            /* NO -  no compare                               */
    char  max_mem[4];      /* YES - Use 70% of available memory by default    */
                            /* NO -  Use specified seg_size/num_seg           */
    long  seg_size[MAX_NUM_PAGE_SIZES];        /* segment size in bytes                          */
    int   num_seg[MAX_NUM_PAGE_SIZES];         /* number of segments (default 1 segment)         */

    int   width;           /* 1, 4, or 8 bytes at a time                     */
    int   debug_level; /* Only 0 1 2 3 value .See the #define DBG_MUST_PRINT for more*/
    int   startup_delay;	/* Seconds to sleep before starting the test */
    int   mem_percent; /* Percentage of free memory to be used for test case */
    char  crash_on_mis[4];    /* Flag to enter the kdb in case of miscompare,   */
                    /* yes/YES/no/NO */
    char  turn_attn_on[4];    /* Flag to enter the attn in case of miscompare,  */
                        /* yes/YES/no/NO */
    int   mcheck_mode;     /* 0 only mem, 1 mem+mcheck, 2 mcheck             */
    char  mcheck_rfile[16];/* memcheck rule file name                        */
    char pattern_nm[52];
	int   num_patterns;
	unsigned int pattern_size[MAX_STANZA_PATTERNS];
	int   pattern_type[MAX_STANZA_PATTERNS]; /* pattern type (enum pattern_type) */
	char  pattern_name[MAX_STANZA_PATTERNS][72];
	char  *pattern[MAX_STANZA_PATTERNS];
	int		remap_p; /* used for empss testcase only */
	unsigned long long seed;
	int		sao_enable;
	int		i_side_test;
	int		d_side_test;
	int		addnl_i_side_test;
	char	base_page_size[10];
	unsigned int base_pg_sz;
	char	base_seg_size[10];
	unsigned long long base_sg_sz;
	int affinity;/*1=affinty is on 0=affinty is off */
    char nx_mem_operations[72];
    int     number_of_nx_task;
    struct nx_task_table nx_task[MAX_NX_TASK];
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
};

struct private_data {
    int  debug; /* Debug flag when 1 prints the debug messages */
    int cpus_avail; /* Number of cpus available */
    int stanza_num;
    int exit_flag;  /* Termination indicator flag */
    int dr_cpu_flag;  /* DR CPU Reconfig indicator flag */
    int standalone; /* If "OTH" (standalone) mode then standalone = 1 */
    int kern_type; /* Kernel type 64 or 32 */
    int bind_proc;
    int pid;
	int pi; /* Pattern index */
    char exer_name[32]; /* Exerciser binary name hxemem64 */
    char dev_id[40]; /* Device id command line param #2 /dev/mem */
    char run_type[4]; /* Run type command line param #3 OTH,REG,EMC */
    char rules_file_name[80]; /* Rules file name */
    FILE *rf_ptr;
    time_t rf_last_mod_time;
    struct sigaction sigvector; /* Signal handler data structure */
    char release_name[10]; /* Release name */
    int htxkdblevel;
    int sig_flag;
	int affinity_flag;
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
};

struct page_wise_thread_data {
        unsigned long page_size;
        int num_of_segments;
        int *shmids;
        int *shm_keys;
        unsigned long *shm_sizes;
        char **shr_mem_hptr;
};

/*
 * Page wise segment details per thread . There are 4 instances of this
 * structure per thread for all the 4 page sizes
 */
struct thread_data {
    int thread_num;
    int cpu_flag;
    pthread_t tid;
    pthread_t kernel_tid;
    int bind_proc;
	int tlbie_shm_id;
	int sec_shm_id;
	int *tlbie_shm_ptr;
	int *sec_shm_ptr;
    struct page_wise_thread_data page_wise_t[MAX_NUM_PAGE_SIZES];
	long rand_page_sz;
};

struct memory_info {
    int lpage_type;     /* 53D Determines the pages supported for
                        earlier AIX releases */
    int num_of_threads;
    int mutex;          /* mutex flag */
    pthread_mutex_t tmutex;

    unsigned long base_page_sz; /* sles11 onwards: page size 4K or 64K */
    unsigned long pspace_avail;/* total paging space available */
    unsigned long pspace_free; /* total paging space free */
    unsigned long total_mem_avail; /* Total memory (real) available */
    unsigned long total_mem_free;  /* Total memory (real) free */

    struct thread_data *tdata_hp;
    struct page_wise_data pdata[MAX_NUM_PAGE_SIZES];
};

/*This structure holds the srad related data */
struct srad_info {
        int sradid;
        int cpulist[256];
        int cpuidx;  /* No of cpus in this srad */
        unsigned long total_srad_mem_avail;
        unsigned long total_srad_mem_free;
};

/* Structure to print the encapsulated values in KDB trap */
struct segment_detail {
    unsigned long seg_num;
    unsigned long page_index;
    unsigned long width;
    unsigned long seg_size;
    unsigned long sub_seg_num;
    unsigned long sub_seg_size;
    unsigned long num_times;
};
int tlbie_test_flag;


#ifdef DEBUG_MEM64
    #define display(...)     displaym(__VA_ARGS__)
#else
#ifndef AIX52S
    #define display(...)
#else
    #define display     displaym
#endif
#endif

/* Definition constants to identify the current operation */
enum test_type {MEM_DWORD       = 1,
                MEM_WORD        = 2,
                MEM_BYTE        = 3,
                RIM_DWORD       = 4,
                RIM_WORD        = 5,
                RIM_BYTE        = 6,
                COMP_DWORD      = 7,
                COMP_WORD       = 8,
                COMP_BYTE       = 9,
                ADDR_WRITE      = 10,
                ADDR_COMP       = 11,
                WR_ADDR_COMP    = 12,
                READ_DWORD      = 13,
                READ_WORD       = 14,
                READ_BYTE       = 15};


/*Function prototypes*/
int do_the_bind_proc (int id, int bind_proc,int pcpu);
int displaym (int sev, int debug,  const char *format,...);
void alocate_mem_for_mem_info_num_of_threads();
int get_num_of_proc (void);
