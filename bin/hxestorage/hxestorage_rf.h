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
/* File name - hxestorage_rf.h                                      */
/* Header file to include all the strcuture/variables/functions     */
/* declaration associated with hxestorage_rf.c                           */
/********************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include "hxiipc64.h"
#include <sys/time.h>
#ifdef __HTX_LINUX__
    /* Linux Specific header files */
    #include "devinfo.h"
    #include <unistd.h>
#else
    /* AIX Specific header files */
    #include <sys/devinfo.h>
#endif

#ifdef __CAPI_FLASH__
    #ifdef __HTX_LINUX__
        #include <capiblock.h>
    #else
        #include <sys/capiblock.h>
    #endif
#endif

#define MAX_THREADS             4096
#define MAX_BWRC_THREADS        1024

#define MAX_RULE_LINE_SZ        8192
#define MAX_TEMPLATES           16
#define MAX_RULES               64
#define MAX_INPUT_PARAMS        64
#define MAX_STR_SZ              32
#define OPER_STR_SZ             12
#define ERR_STR_SZ              48
#define MAX_NUM_CACHE_THREADS   32

#define KB                      1024
#define MB                      1024*1024
#define GB                      1024*1024*1024
#define TB						(1024 * GB)

#define NO                      0
#define YES                     1
#define DATACHECK               2

#define SEQ         0
#define RANDOM      1
#define RROBIN      2
#ifdef __CAPI_FLASH__
    #define CBLK_OPN_PHY_LUN            0x00
    #define CBLK_CLOSE_LUN              0x4
#endif

/**********************************************/
/* Default values for various rule parameters */
/**********************************************/
#define DEFAULT_NUM_KEYWDS      1
#define UNDEFINED               -1
#define UNINITIALIZED           -1
#define DEFAULT_STARTING_BLOCK  BOT
#define DEFAULT_DIRECTION       UP
#define DEFAULT_MIN_BLKNO       0
#define DEFAULT_INCREMENT       0
#define DEFAULT_NUM_THREADS     0
#define DEFAULT_ASSOCIATIVITY   RROBIN
#define DEFAULT_NUM_OPER        0
#define DEFAULT_SEEK_PRCNT      0
#define DEFAULT_NUM_BLKS        1
#define DEFAULT_ALIGN           0
#define DEFAULT_LBA_ALIGN       0
#define DEFAULT_NUM_MALLOCS     2
#define DEFAULT_MODE            VALIDATION
#define DEFAULT_TESTCASE        SYNC
#define DEFAULT_HOTNESS         1
#define DEFAULT_LOOP_ON_OFFSET  0
#define DEFAULT_RULE_OPTION     0
#define DEFAULT_BWRC_THREADS    4
#define DEFAULT_QUEUE_DEPTH     16
#define DEFAULT_CACHE_THREADS   2
#define DEFAULT_AIO_REQ_QUEUE_DEPTH 32
#define TEMPLATE_STANZA         0
#define RULE_STANZA             1

#define HARD        HTX_HE_HARD_ERROR
#define MISCOM      HTX_HE_MISCOMPARE
#define SOFT        HTX_HE_SOFT_ERROR
#define INFO        HTX_HE_INFO

#define DPRINT printf

enum direction {UP, DOWN, IN, OUT};
enum starting_block {BOT = -3, MID, TOP};
enum mode {VALIDATION, PERFORMANCE};
enum testcase {SYNC, ASYNC, PASSTHROUGH, CACHE};

#define SAVE_SEEDS_FLAG  0x1 /* LJR - add rule_options */
#define RESTORE_SEEDS_FLAG 0x2 /* LJR - add rule_options */
#define USER_SEEDS_FLAG 0x4 /* DGS - add rule_options */

/*****************************************************/
/* Function pointer for parsing rule/template stanza */
/*****************************************************/
typedef int (*parse_stanza_fptr)(struct htx_data *, char *, void *, int *, unsigned long long, unsigned int);

extern char misc_run_cmd[256], run_on_misc;
extern int threshold, hang_time, enable_state_table;
extern int num_sub_blks_psect;

/**********************************************/
/* Structure containing transfer size info    */
/**********************************************/
typedef struct {
    unsigned long long min_len;
    unsigned long long max_len;
    int increment;
} xfer_size;

typedef struct {
    unsigned int num_keywds;
    xfer_size size[MAX_INPUT_PARAMS];
} xfer_info;

/************************************/
/* Structure to hold template info  */
/************************************/
typedef struct {
    char template_id[MAX_STR_SZ];       /* Template_id */
    xfer_size data_len;                 /* Transfer size in bytes */
    unsigned short seek_breakup_prcnt;  /* Percentage of SEQ/RANDOM operations */
    char oper[OPER_STR_SZ];             /* Oper defined - WRC RC W etc. */
} template;
template tmplt_list[MAX_TEMPLATES];

typedef struct {
    unsigned int  num_threads;  /* No. of threads defined for the template */
    template *tmplt_ptr;        /* Pointer to template structure */
} template_info;

typedef struct {
    unsigned int num_keywds;
    unsigned long long value[MAX_INPUT_PARAMS];
} rule_line;

/***********************************/
/* Structure to hold rulefile info */
/***********************************/
struct ruleinfo {
    char rule_id[MAX_STR_SZ];                       /* Rule Id */
    char pattern_id[MAX_INPUT_PARAMS][MAX_STR_SZ];  /* Pattern Id */
    unsigned int num_pattern_defined;               /* Total no. of patterns defined */
    char oper[MAX_INPUT_PARAMS][OPER_STR_SZ];       /* Oper defined - WRC RC W etc. */
    unsigned int num_oper_defined;                  /* Total no. of opers defined */
    unsigned int num_threads;                       /* No. of threads to create */
    unsigned int num_BWRC_threads;                  /* No. of BWRC threads defined */
    unsigned int BWRC_th_mem_index;                 /* Start index in BWRC threads mem */
    char is_only_BWRC_stanza;                       /* Set if only BWRC oper defined for the stanza */
    template_info tmplt[MAX_TEMPLATES];             /* Associated template info */
    short num_associated_templates;                 /* No. of templates associated */
    short associativity;                            /* Defined how threads are associated with each rule parameter - SEQ, RROBIN */
    short testcase;                                 /* Testcase type - SYNC/ASYNC/PASSTH */
    short mode;                                     /* Mode - PERFORMANCE/VALIDATION */
    unsigned int rule_option;                       /* Rule option for SAVE_SEED, RESTORE_SEED and USER_DEFINED_SEED */
    rule_line num_oper;                             /* Number of operations to perform - Can be N, NQD (QD: Queue Depth) */
    rule_line seek_breakup_prcnt;                   /* Percentage of SEQ/RANDOM operations */
    xfer_info data_len;                             /* Transfer size in bytes */
    rule_line starting_block;                       /* For Seq. oper only - N, TOP, BOT, MID */
    rule_line direction;                            /* For Seq. oper only - UP, DOWN, IN, OUT */
    rule_line max_blkno;                            /* Highest block to be used */
    rule_line min_blkno;                            /* Lowest block to be used */
    rule_line blk_hop;                              /* Number of blocks to skip */
    rule_line align;                                /* Alignment for IO buffers requested */
    rule_line lba_align;                            /* user supplied IOP alignement */
    rule_line num_mallocs;                          /* Number of mallocs before freeing */
    rule_line hotness;                              /* Number of IO to be done to make a band HOT */
    rule_line loop_on_offset;                       /* YES or NO */
    int num_async_io;                            	/* queue depth in case of ASYNC IO */
    unsigned short section;                         /* Make section of disk(equal to num_threads defined) - YES or NO */
    unsigned int sleep;                             /* Number of micro seconds to sleep */
    char run_reread;                                /* if need to do re-read in case of miscompare 0 YES or NO */
    int repeat_pos;                                 /* Number of times to skip stanza */
    int repeat_neg;                                 /* Number of times to skip stanza */
    int num_cache_threads;                          /* Num of cache threads to create (only for cache stanzas) */
    char cmd_list[256];                             /* string to hold shell command line */
    unsigned short user_defined_seed[6];            /* User defined seed value for number generator */
    unsigned long long user_defined_lba_seed;       /* User defined seed value for max lba */
#ifdef __CAPI_FLASH__
	unsigned int open_flag;
	unsigned long long chunk_size;
#endif
};
struct ruleinfo rule_list[MAX_RULES];

/****************************************************/
/*  Structure to hold Global info for the device    */
/****************************************************/
struct device_info {
    char hostname[MAX_STR_SZ];                  /* System hostname where exerciser is running */
    char diskname[MAX_STR_SZ];                  /* Disk name */
    char disk_parent[MAX_STR_SZ];               /* Disk Parent device name */
    char dev_name[MAX_STR_SZ];                  /* Device name */
    #define RULE_FILE_NAME_SZ   80
    char rules_file_name[RULE_FILE_NAME_SZ];    /* Rule file name */
    int crash_on_anyerr;                        /* Flag to trap to KDB on any error */
    int crash_on_miscom;                        /* flag to trap to KDB on miscomapre */
    int crash_on_hang;                          /* flag to trap to KDB on HANG */
    int write_cache;                            /* flag to enable syncing of write cache */
    int cont_on_err;                            /* flag to enable continue test on error */
    int cont_on_misc;                           /* whether to continue on miscompare or NOT */
    int debug_flag;                             /* flag to enable debug prints */
    unsigned int blksize;                       /* Block size for the device */
    unsigned long long maxblk;                  /* Max. no. of blocks on the disk */
    unsigned long long lun_id;                  /* LUN id of the device */
    unsigned long long scsi_id;                 /* SCSI id of the device */
    unsigned long long lun_id_parent;           /* LUN id of device's parent */
    unsigned long long scsi_id_parent;          /* SCSI id of devcie's parent */
    struct devinfo parent_info;                 /* Parent device info - structure defined in devinfo.h */
};
extern struct device_info dev_info;

unsigned int num_rules_defined, num_templates_defined;

/*************************/
/* Function Declarations */
/*************************/
int read_rf(struct htx_data *htx_ds, char *rf_name, unsigned long long maxblk, unsigned int blksize);
void set_rule_defaults(struct ruleinfo *ruleptr, unsigned long long maxblk, unsigned int blksize);
void set_template_defaults(template *);
int parse_rule_parameter(struct htx_data *ps, char *str, void *stanza_ptr, int *line, unsigned long long maxblk, unsigned int blksize);
int parse_template_parameter(struct htx_data *ps, char *str, void *stanza_ptr, int *line, unsigned long long maxblk, unsigned int blksize);
void check_n_update_rule_params(struct htx_data *, struct ruleinfo *);
int parse_keywd(char *str, char **value);
int get_line(char *str, FILE *fp);
long long get_value(char *, unsigned int);
int get_queue_depth(struct htx_data *ps);

