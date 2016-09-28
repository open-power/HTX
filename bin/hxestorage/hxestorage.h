/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

/********************************************************************/
/* File name - hxestorage.h                                         */
/* Header file to include all the strcuture/variables/functions     */
/* declaration associated with hxestorage.c                         */
/********************************************************************/

/*****************************************************************************
 *   The following is the definition of BLKNO[0,1,2]. This can get confusing.
 *      BLKNO[0] - this is the current/first block number (LBA) to be used.
 * In case of direction UP:
 *    BLKNO[1] - same as BLKNO[0], BLKNO[2] - Last LBA no. where to do oper.
 * In case of direction DOWN:
 *    BLKNO[1] - Last LBA no. where to do oper, BLKNO[2] - Same as BLKNO[0]
 * In case of direction IN or OUT:
 *    BLKNO[1] - this is the minimum/lower block number (LBA) to be used.
 *    BLKNL[2] - this is the maximum/upper block number (LBA) to be used.
 *   Stating the minimum and maximum does not mean the minimum and maximum on
 *   the disk. These two figures can change such as the case when reading/
 *   writing back and forth across a disk.
*******************************************************************************/

#include <pthread.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <stdint.h>
#ifndef __CAPI_FLASH__
    #include <aio.h>
#endif

#include "hxestorage_rf.h"
#ifndef __HTX_LINUX__
    #include <sys/scsi.h>
    #include <sys/scdisk.h>
    #include <cf.h>
    #include <sys/cfgodm.h>
    #include <odmi.h>
    #include <sys/cfgdb.h>
    #include <lvm.h>
    #include <sys/scsi_buf.h>
    #include <sys/mdio.h>
#endif

#define MAX_OPER_COUNT      6
#define NOP					-1
#define R                   0
#define W                   1
#define C                   2
#define V                   2
#define D                   3

#define R_CACHE             0
#define W_CACHE             1
#define C_CACHE             2

#define DEFAULT_EEH_RETRIES         5
#define DEFAULT_RETRY_COUNT         5
#define DEFAULT_WAIT_COUNT          20

#define MAX_BUFFERS                 256
#define MAX_PATTERN_LENGTH          32

#define MIN_SLEEP_TIME              15
#define MAX_SLEEP_TIME              90

/*
 * MAX_THREADS defines threads spawned by this exer, BITS_USED is
 * binary equvalent to represent those threads.
 * So keep them in sync, i.e. 2^BITS_USED = MAX_THREADS
 */

#ifdef __CAPI_FLASH__
	#define TESTCASE_TYPES              2
	#define VLUN_BLKSIZE				4096
	#define VLUN_TOTALBLOCKS			(1 * GB)
    #define DEFAULT_CHUNK_SIZE          (64 * KB)
#else
	#define TESTCASE_TYPES			4
#endif

/* AIO_ALL will get status for all the queued IOs in Async */
#define AIO_ALL 	0
/* AIO_SINGLE will return index of first IO from the queue that gets completed */
#define AIO_SINGLE 	1

#define DEFAULT_THRESHOLD           3
#define DEFAULT_HANG_TIME           600

#define SEEK_TYPES                  2

#define MIS_LOG_SIZE                50 * KB

#define MISCOM_ERR                  -1
#define NO_IO_ERR                   0
#define OPEN_ERR                    1
#define CLOSE_ERR                   2
#define READ_ERR                    3
#define WRITE_ERR                   4
#define IOCTL_ERR                   5
#define SEEK_ERR                    6
#define FSYNC_ERR                   7

#define RETURN(retcod, err_flag) \
    { \
        if ((dev_info.cont_on_err == YES && err_flag > NO_IO_ERR) || \
            (dev_info.cont_on_err == DATACHECK && err_flag == READ_ERR) || \
            (dev_info.cont_on_misc == YES && err_flag == MISCOM_ERR)) { \
            err_flag = NO_IO_ERR; \
            return (0); \
        } else { \
            err_flag = NO_IO_ERR; \
            return (retcod); \
        } \
    }

#define DEFAULT_MUTEX_ATTR_PTR      NULL
#define DEFAULT_COND_ATTR_PTR       NULL

#define RROBIN_INDEX(i, max_count)  (i % max_count)
#define SEQ_INDEX(i, max_count) (i < max_count ? i : (max_count - 1))

#define INVALID                         0
#define VALID                           1

#define BITS_PER_BLOCK                  0x2
#define BITS_PER_BYTE                   0x8
#define BLOCK_PER_BYTE                  (BITS_PER_BYTE / BITS_PER_BLOCK)
#define WRITE_STATUS_MASK               0x3

#define STATE_TABLE_INFO_BLK            0x0
#define WRITE_STATUS_INFO_BLK           0x1

#define STATUS_OFFSET                   0x0
#define TABLE_SIZE_OFFSET               0x1
#define TIME_OFFSET                     0x9
#define DEV_NAME_OFFSET                 0xd
#define HOSTNAME_OFFSET                 0x1d

#define ID_VAR_LEN                      4
#define PATTERN_ID_VAR_LEN              12
#define PATTERN_BUFFER_VAR_LEN          16
#define PATTERN_SIZE_VAR_LEN            14
#define TID_VAR_LEN                     5
#define FD_VAR_LEN                      4
#define MODE_VAR_LEN                    6
#define TESTCASE_VAR_LEN                10
#define OPER_VAR_LEN                    6
#define OPEN_FPTR_VAR_LEN               11
#define CLOSE_FPTR_VAR_LEN              12
#define OPER_FPTR_VAR_LEN               11
#define OPER_COUNT_VAR_LEN              12
#define TH_NUM_VAR_LEN                  8
#define NUM_OPER_VAR_LEN                10
#define CUR_SEEK_TYPE_VAR_LEN           15
#define STARTING_BLOCK_VAR_LEN          16
#define DIRECTION_VAR_LEN               11
#define SEEK_BREAKUP_PRCNT_VAR_LEN      20
#define TRANSFER_SIZE_VAR_LEN           15
#define MIN_BLKNO_VAR_LEN               11
#define MAX_BLKNO_VAR_LEN               11
#define DLEN_VAR_LEN                    6
#define NUM_BLKS_VAR_LEN                10
#define BLK_HOP_VAR_LEN                 9
#define SAVED_DLEN_VAR_LEN              12
#define NUM_DISCARDS_VAR_LEN            14
#define NUM_WRITES_VAR_LEN              12
#define NUM_READS_VAR_LEN               11
#define NUM_WRITES_REMAINING_VAR_LEN    22
#define COMPARE_ENABLED_VAR_LEN         17
#define FIRST_BLK_VAR_LEN               11
#define CUR_BLKNO_VAR_LEN               11
#define ALIGN_VAR_LEN                   7
#define LBA_ALIGN_VAR_LEN               11
#define HOTNESS_VAR_LEN                 9
#define OFFSET_VAR_LEN                  8
#define NUM_MALLOCS_VAR_LEN             13
#define NUM_CACHE_THREADS_VAR_LEN       19
#define SEED_VAR_LEN                    6
#define DATA_SEED_VAR_LEN               11
#define LBA_SEED_VAR_LEN                10
#define PARENT_SEED_VAR_LEN             13
#define PARENT_DATA_SEED_VAR_LEN        18
#define PARENT_LBA_SEED_VAR_LEN         17
#define FENCEPOST_INDEX_VAR_LEN         17
#define BWRC_ZONE_INDEX_VAR_LEN         17
#define SEG_TABLE_INDEX_VAR_LEN         17
#define RUN_REREAD_VAR_LEN              12
#define RULE_OPTION_VAR_LEN             13
#define DO_PARTIAL_VAR_LEN              12
#define WPTR_VAR_LEN                    6
#define RPTR_VAR_LEN                    6
#define STRT_WPTR_VAR_LEN               11
#define STRT_RPTR_VAR_LEN               11
#define CUR_WBUF_VAR_LEN                10
#define CUR_RBUF_VAR_LEN                10
#define REREAD_BUF_VAR_LEN              12
#define AND_MASK_VAR_LEN                10
#define OR_MASK_VAR_LEN                 9
#define RAND_INDEX_VAR_LEN              12
#define BEGIN_DWORD_VAR_LEN             13
#define TRAILING_DWORD_VAR_LEN          16
#define AIO_REQ_QUEUE_VAR_LEN           14
#define FLAG_VAR_LEN					5

#define NUM_ASYNC_IO_VAR_LEN            13
#define MIS_DETAIL_VAR_LEN              11
#define CUR_ASYNC_IO_VAR_LEN            13
#define OPEN_FLAG_VAR_LEN				10
#define MIS_LOG_BUF_VAR_LEN             12
#define LUN_TYPE_VAR_LEN				9
#define	CHUNK_SIZE_VAR_LEN				11

/*****************************************************************************
** Following pragma allows us to create some inline code that will checkstop
** the machine without having to go into a kernel extension.  This trap is
** invoked in the cmpbuf routine when a miscompare is detected and if attn
** is requested through "turn_attention_on" flag in the rules file. If attn
** instruction is not supported, then the exerciser will give signal 4, that is,
** illegal instruction.
******************************************************************************/
#ifndef __HTX_LINUX__

#pragma mc_func attn { "00000200" }
#pragma reg_killed_by attn
#pragma mc_func trap { "7c810808" }
#pragma reg_killed_by trap

#endif

struct thread_context; /* defined later */

/********************************************************/
/*  Function pointers for open/close/read/write/compare */
/*  disk.                                               */
/********************************************************/
typedef int (*open_fptr)(struct htx_data *, const char *, struct thread_context *);
typedef int (*close_fptr)(struct htx_data *, struct thread_context *);
typedef int (*oper_fptr)(struct htx_data *, struct thread_context *, int);

extern int read_rules_file_count;
extern pthread_mutex_t cache_mutex, segment_mutex, log_mutex;
extern int eeh_retries, turn_attention_on;
extern char misc_run_cmd[100], run_on_misc;
extern time_t time_mark;
extern volatile char exit_flag, signal_flag, int_signal_flag;
extern int total_BWRC_threads;

pthread_attr_t thread_attrs_detached;    /* threads created detached */

int volatile BWRC_threads_running, non_BWRC_threads_running;

/**************************************************/
/*  struct used to hold details of the miscompare */
/**************************************************/
struct miscompare_details
{
    int mis_start;
    int mis_end;
    int mis_size;
    unsigned long long mis_start_LBA;
    unsigned long long mis_end_LBA;
    int mis_type;
    unsigned long long diff_LBA;
    char diff_dev[MAX_STR_SZ];
    char diff_host[MAX_STR_SZ];
    char diff_stanza[MAX_STR_SZ];
};

struct random_seed_t {
    unsigned short xsubi[3];
    struct drand48_data drand_struct;
};

struct aio_ops {
	#ifndef __CAPI_FLASH__
		struct aiocb aio_req;
	#else
		int32_t tag;
		uint64_t trf_len;
	#endif
	uint64_t status;
	char op;
};
/*************************************************************/
/* Thread context structure - Keep all information of thread */
/*************************************************************/
struct thread_context {
    char id_var[ID_VAR_LEN];
    char id[32];                                        /* Thread id string */
    char pattern_id_var[PATTERN_ID_VAR_LEN];
    char pattern_id[MAX_STR_SZ];                        /* Pattern Id */
    char oper_var[OPER_VAR_LEN];
    char oper[OPER_STR_SZ];                             /* Oper to be performed - WRC,RWRC,R,W,RC,S etc */
    char num_oper_var[NUM_OPER_VAR_LEN];
    int num_oper[SEEK_TYPES];                           /* num of operations defined for SEQ/RANDOM seek types ( oper loop count) */
    char seek_breakup_prcnt_var[SEEK_BREAKUP_PRCNT_VAR_LEN];
    short seek_breakup_prcnt;                           /* Percentage of SEQ/RANDOM operations */
    char transfer_sz_var[TRANSFER_SIZE_VAR_LEN];
    xfer_size transfer_sz;                              /* Transfer size in bytes */
    char th_num_var[TH_NUM_VAR_LEN];
    int th_num;                                         /* Thread number */
    char min_blkno_var[MIN_BLKNO_VAR_LEN];
    unsigned long long min_blkno;                       /* Lowest block to be used */
    char max_blkno_var[MAX_BLKNO_VAR_LEN];
    unsigned long long max_blkno;                       /* Highest block to be used */
    char dlen_var[DLEN_VAR_LEN];
    unsigned long long dlen;                            /* transfer length for the current IO operation - changes at run time */
    char num_blks_var[NUM_BLKS_VAR_LEN];
    unsigned int num_blks;                              /* Transfer size in terms of no. of blocks */
    char mode_var[MODE_VAR_LEN];
    unsigned short mode;                                /* Testcase mode - VALIDATION/PERFORMANCE */
    char testcase_var[TESTCASE_VAR_LEN];
    unsigned short testcase;                            /* Testcase type - SYNC/ASYNC/PASSTH/CACHE */
    char direction_var[DIRECTION_VAR_LEN];
    unsigned short direction;                           /* For Seq. oper only - UP, DOWN, IN, OUT */
    char blk_hop_var[BLK_HOP_VAR_LEN];
    int blk_hop;                                        /* Number of blocks to skip */
    char pattern_buffer_var[PATTERN_BUFFER_VAR_LEN];
    char *pattern_buffer;                               /* Pattern buffer */
    char pattern_size_var[PATTERN_SIZE_VAR_LEN];
    unsigned int pattern_size;                          /* Pattern size */
    char tid_var[TID_VAR_LEN];
    pthread_t tid;                                      /* Thread tid */
    char fd_var[FD_VAR_LEN];
    int fd;                                             /* File descriptor for device to be exercised */
    char open_fptr_var[OPEN_FPTR_VAR_LEN];
    open_fptr open_func;                                /* Function pointer for opening the device  */
    char close_fptr_var[CLOSE_FPTR_VAR_LEN];
    close_fptr close_func;                              /* Function pointer for closing the device */
    char oper_fptr_var[OPER_FPTR_VAR_LEN];
    oper_fptr operations[OPER_STR_SZ];                  /* Function pointers for the opers to be done */
    char oper_count_var[OPER_COUNT_VAR_LEN];
    int oper_count;                                     /* num of oper defined in oper string */
    char cur_seek_type_var[CUR_SEEK_TYPE_VAR_LEN];
    int cur_seek_type;                                  /* Tells whether currently running SEQ OR RANDOM seek operation */
    char starting_block_var[STARTING_BLOCK_VAR_LEN];
    long long starting_block;                           /* For Seq. oper only - N, TOP, BOT, MID */
    char saved_dlen_var[SAVED_DLEN_VAR_LEN];
    unsigned long long saved_dlen;                      /* original transfer size to be saved. Needed in case of partial transfer */
    char num_discards_var[NUM_DISCARDS_VAR_LEN];
    unsigned int num_discards;                          /* no. of discards based on operation defined */
    char num_writes_var[NUM_WRITES_VAR_LEN];
    unsigned int num_writes;                            /* no. of writes based on operation defined */
    char num_reads_var[NUM_READS_VAR_LEN];
    unsigned int num_reads;                             /* no. of reads based on operation defined */
    char num_writes_remaining_var[NUM_WRITES_REMAINING_VAR_LEN];
    int num_writes_remaining;                           /* Updated at runtime. needed to ciheck if HEADER comparison is required or not */
    char compare_enabled_var[COMPARE_ENABLED_VAR_LEN];
    char compare_enabled;                               /* 'y' or 'n' based on compare operation is defined or not */
    char first_blk_var[FIRST_BLK_VAR_LEN];
    unsigned long long first_blk;                       /* first block set based on starting block */
    char cur_blkno_var[CUR_BLKNO_VAR_LEN];
    unsigned long long blkno[3];                        /* Explained at the beginning of this file */
    char align_var[ALIGN_VAR_LEN];
    int align;                                          /* Alignment for IO buffers requested */
    char lba_align_var[LBA_ALIGN_VAR_LEN];
    int lba_align;                                      /* user supplied IOP alignement */
    char hotness_var[HOTNESS_VAR_LEN];
    unsigned int hotness;                               /* Number of IO to be done to make a band HOT */
    char offset_var[OFFSET_VAR_LEN];
    unsigned short offset;                              /* offset count if loop_on_offset defined */
    char num_mallocs_var[NUM_MALLOCS_VAR_LEN];
    int num_mallocs;                                    /* Number of mallocs before freeing */
    char num_cache_threads_var[NUM_CACHE_THREADS_VAR_LEN];
    int num_cache_threads;
    char seed_var[SEED_VAR_LEN];
    struct random_seed_t seed;                          /* seed used by program for number generator */
    char data_seed_var[DATA_SEED_VAR_LEN];
    struct random_seed_t data_seed;                     /* seed used by program for number generator for data*/
    char lba_seed_var[LBA_SEED_VAR_LEN];
    unsigned long long lba_seed;                        /* seed used by program for max lba */
    char parent_seed_var[PARENT_SEED_VAR_LEN];
    unsigned short parent_seed[3];                      /* used as parent seed */
    char parent_data_seed_var[PARENT_DATA_SEED_VAR_LEN];
    unsigned short parent_data_seed[3];                 /* used as parent data seed */
    char parent_lba_seed_var[PARENT_LBA_SEED_VAR_LEN];
    unsigned long long parent_lba_seed;                 /* used as parent lba seed */
    char fencepost_index_var[FENCEPOST_INDEX_VAR_LEN];
    unsigned int fencepost_index;                       /* Used by BWRC threads for indexing into lba_fencepost array */
    char BWRC_zone_index_var[BWRC_ZONE_INDEX_VAR_LEN];
    int BWRC_zone_index;                                /* Used by Non-BWRC threads for finding the BWRC zone corresponding to LBAs where it is going to do operations */
    char seg_table_index_var[SEG_TABLE_INDEX_VAR_LEN];
    int seg_table_index;                                /* index into segment_table */
    char run_reread_var[RUN_REREAD_VAR_LEN];
    char run_reread;                                    /* if need to do re-read in case of miscompare - YES or NO */
    char rule_option_var[RULE_OPTION_VAR_LEN];
    unsigned int rule_option;                           /* Rule option for SAVE_SEED, RESTORE_SEED and USER_DEFINED_SEED */
    char do_partial_var[DO_PARTIAL_VAR_LEN];
    int do_partial;                                     /* Set to 1 if current IO operation is partial transfer */
    char wptr_var[WPTR_VAR_LEN];
    char *wptr[MAX_BUFFERS];                            /* Pointers to all Write buffers */
    char rptr_var[RPTR_VAR_LEN];
    char *rptr[MAX_BUFFERS];                            /* Pointers to all Read buffers */
    char strt_wptr_var[STRT_WPTR_VAR_LEN];
    char *strt_wptr[MAX_BUFFERS];                       /* pointers to aligned write buffers */
    char strt_rptr_var[STRT_RPTR_VAR_LEN];
    char *strt_rptr[MAX_BUFFERS];                       /* Pointers to aligned read buffers */
    char cur_wbuf_var[CUR_WBUF_VAR_LEN];
    char *wbuf;                                         /* wbuf used for current operation */
    char cur_rbuf_var[CUR_RBUF_VAR_LEN];
    char *rbuf;                                         /* rbuf used for current operation */
    char reread_buf_var[REREAD_BUF_VAR_LEN];
    char *reread_buf;                                   /* pointer to reread buffer */
    char mis_detail_var[MIS_DETAIL_VAR_LEN];
    struct miscompare_details mis_detail;               /* Used to hold the detail of miscompare */
    char mis_log_buf_var[MIS_LOG_BUF_VAR_LEN];
    char *mis_log_buf;                                  /* holds miscompare detailed analysis data */
    char and_mask_var[AND_MASK_VAR_LEN];
    unsigned long long and_mask;                        /* and mask for pattern 8,9,A */
    char or_mask_var[OR_MASK_VAR_LEN];
    unsigned long long or_mask;                         /* or mask for pattern 8,9,A */
    char rand_index_var[RAND_INDEX_VAR_LEN];
    unsigned long long rand_index;                      /* random index into rand_buf */
    char begin_dword_var[BEGIN_DWORD_VAR_LEN];
    unsigned int begin_dword;                           /* beginning dword */
    char trailing_dword_var[TRAILING_DWORD_VAR_LEN];
    signed long long trailing_dword;                    /* Trailing dword where to write pattern  */
	char aio_req_queue_var[AIO_REQ_QUEUE_VAR_LEN];
	struct aio_ops *aio_req_queue;                       /* Request queue for aysnc IO */
	char num_async_io_var[NUM_ASYNC_IO_VAR_LEN];
	int num_async_io;	                     	         /* max number of asyns IO that can be kept in-flight */
	char cur_async_io_var[CUR_ASYNC_IO_VAR_LEN];
	volatile int cur_async_io;                           /* current num of asyns IO in progress */
	char flag_var[FLAG_VAR_LEN];
	int flag;
	int io_err_flag;                                     /* Flag used to find if read/write/open/close/ioctl error */
	char force_op_var[LUN_TYPE_VAR_LEN];
	int force_op;
#ifdef __CAPI_FLASH__
	char open_flag_var[OPEN_FLAG_VAR_LEN];
	unsigned int open_flag;								/* */
#endif
};

/************************************************/
/*  Structure to maintain fencepost information */
/************************************************/
struct BWRC_range {
    volatile unsigned long long min_lba; /* Min LBA till where this instance of BWRC has written */
    volatile unsigned long long max_lba; /* Max LBA till where this instance of BWRC has written */
    unsigned short direction;            /* Direction of BWRC thread - UP or DOWN  */
    char status;                         /* Still running or exited?? */
};
struct BWRC_range lba_fencepost[MAX_BWRC_THREADS];

/***************************************************/
/* Structure to hold info needed for cache threads */
/* This wstructure will be update by main thread   */
/* which creates cache threads.                    */
/***************************************************/
struct cache_thread_info {
    unsigned long long dlen;
    char *buf;
    int DMA_running;
    int cache_cond;
    int testcase_done;
    int cache_threads_created;
    int num_cache_threads_waiting;
    pthread_mutex_t cache_mutex;
    pthread_cond_t do_oper_cond;
};
volatile struct cache_thread_info c_th_info[MAX_NUM_CACHE_THREADS];

/*****************************************/
/*   Structure to hold state table  info */
/*****************************************/
struct state_table {
    char status;                    /* tells whether state table is VALID or not?? */
    unsigned long long size;        /* Size of state table */
    unsigned int enablement_time;   /* Time when state table was made valid */
    char diskname[16];              /* Diskname used in header */
    char hostname[32];              /* Hostname used in header */
    char *blk_write_status;         /* memory for keeping write status of each block */
};
struct state_table s_tbl;

/*************************/
/* Function Declarations */
/*************************/
void initialize_threads(struct thread_context *, int);
int execute_validation_test (struct htx_data *, struct thread_context *);
int execute_performance_test (struct htx_data *, struct thread_context *);
void * execute_thread_context (void *);

int fill_pattern_details(struct htx_data *, struct thread_context *);
int populate_operations(struct thread_context *);
int populate_thread_context(struct htx_data *, struct ruleinfo *, struct thread_context *, struct thread_context *);
int populate_BWRC_thread_context(struct htx_data *, struct ruleinfo *, struct thread_context *);
void apply_template_settings (struct thread_context *, template *);
void apply_rule_settings (struct htx_data *, struct thread_context *, struct thread_context *, struct ruleinfo *, int);
int create_cache_threads(struct htx_data *, struct thread_context *);
int get_disk_info(struct htx_data *, char *);
int check_disk(unsigned char *, char *, int);

void update_blkno(struct htx_data *, struct ruleinfo *, struct thread_context *);
void update_min_blkno(void);
void initialize_fencepost(struct thread_context *);
int initialize_state_table (struct htx_data *, char *);
int sync_state_table(struct htx_data *, char *);

int check_write_cache(struct htx_data *);
int sync_cache_thread(struct htx_data *);

void analyze_miscompare(struct thread_context *, int, char *);
void sig_function(int, int, struct sigcontext *);
void int_sig_function(int, int, struct sigcontext *);
void SIGTERM_hdl(int, int, struct sigcontext *);
void cleanup_threads_mem(void);
void cleanup_resources(void);
void print_thread_context(struct thread_context *);

