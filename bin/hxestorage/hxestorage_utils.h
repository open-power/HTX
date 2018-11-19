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
/* File name - hxestorage_utils.h                                      	*/
/* Header file to include all the strcuture/variables/functions     */
/* declaration associated with hxestorage_utils.c                      	*/
/********************************************************************/

#include <sys/stat.h>
#include "hxestorage.h"

/** Pre-prcoessor declarations for HEADER **/
#define HEADER_SIZE				64
#define LBA_POS					0
#define TIME_STAMP_POS			8
#define DEV_NAME_POS			12
#define STANZA_ID_POS			22
#define HOSTNAME_POS			34
#define WRITE_STAMP_POS			48
#define RESERVED_BYTES_POS		50
#define BUFSIG_POS				58
#define CHECKSUM_POS			62

#define LBA_LEN                 8
#define TIME_STAMP_LEN			4
#define DEV_NAME_LEN			10
#define STANZA_ID_LEN			12
#define HOSTNAME_LEN			14
#define WRITE_STAMP_LEN         2
#define RESERVED_BYTES_LEN		8
#define BUFSIG_LEN				4

#define BUFSIG              "MDHF"
#define OVERHEAD            (HEADER_SIZE/2)

#define IO_DONE             0
#define IO_IN_PROGRESS      1

#define UPDATE_LBA(buf, lba) \
 	{ *(unsigned long long *)buf = lba; \
 	}


#define UPDATE_TIME_STAMP(buf, time_stamp) \
	{ memcpy(buf, time_stamp, TIME_STAMP_LEN); \
	}


#define UPDATE_DEV_NAME(buf) \
	{ if (enable_state_table == YES) { \
          memcpy(buf, s_tbl.diskname, DEV_NAME_LEN); \
      } else { \
          memcpy(buf, dev_info.diskname, DEV_NAME_LEN); \
      } \
	}

#define UPDATE_STANZA_ID(buf, id) \
	{ memcpy(buf, id, STANZA_ID_LEN); \
	}


#define UPDATE_HOSTNAME(buf) \
	{ if (enable_state_table == YES) { \
	      memcpy(buf, s_tbl.hostname, HOSTNAME_LEN); \
	  } else { \
	      memcpy(buf, dev_info.hostname, HOSTNAME_LEN); \
      } \
	}

#define UPDATE_WRITE_STAMP(buf, write_stamp) \
	{ *(unsigned short *)buf = write_stamp; \
	}

#define RESERVE_BYTES(buf) \
	{ memcpy(buf, "RESERVED", 8); \
	}

#define UPDATE_BUFSIG(buf) \
	{ memcpy(buf, BUFSIG, BUFSIG_LEN); \
	}

#define UPDATE_CHECKSUM(buf, cksum) \
	{ *(unsigned short *)buf = ~(cksum & 0xffff ) + 1; \
	}


#ifdef __HTX_LINUX__
    #define STRERROR_R(err_num, buf, size) \
    {     char *err_msg; \
          if (err_num > 0) { \
              err_msg = strerror_r(err_num, buf, size); \
              strcpy(buf, err_msg); \
          }  else { \
              strcpy(buf, "Unknown error"); \
          } \
    }
#else
    #define STRERROR_R(err_num, buf, size) \
    {    int rc = 0; \
         if (err_num > 0) { \
             rc = strerror_r(err_num, buf, size); \
             if (rc != 0) { \
                 strcpy(buf,"unknown error"); \
             } \
         } else {  \
             strcpy(buf,"unknown error"); \
         } \
    }
#endif

#define AIO_COMPLETED       0
#define AIO_ERROR           1
#define AIO_INPROGRESS      EINPROGRESS

#define SEG_TABLE_LIMIT         20              /* Number of threads per segment table */

extern int max_thread_cnt;

extern unsigned long long saved_data_len;
extern int volatile collisions;

/************************************************/
/*      Structures to hold segment info         */
/************************************************/
/* segment_table_data struture holds the information of individual
 * thread before performing any IO.
 */
struct segment_table_data {
        unsigned long long th_context_addr; /* address of thread context updating the entry */
        unsigned long long flba;            /* start LBA no. */
        unsigned long long llba;            /* End LBA no. */
        unsigned long long seg_flba;        /* start LBA no of individual segment. */
        unsigned long long seg_llba;        /* End LBA no of individual segment. */
        pthread_t tid;                      /* thread id */
        time_t last_update_time;            /* Time when IO started */
        time_t io_update_time[8];           /* Time stamp for individual IO operation */
        pthread_mutex_t time_mutex;         /* Mutex lock for updating timestamp */
        short time_index;                   /* index into io_update_time */
        short hang_count;                   /* hang count */
        int io_in_progress: 1;              /* Flag to check if IO is in progress */
        int in_use: 1;                      /* flag to indicate if segment table entry is in use */
};

/* segment_table structure holds the uppper and lower lba range of individual segments */
struct segment_table {
        pthread_mutex_t segment_mutex;		/* mutex lock for a given seg_table */
    	pthread_cond_t segment_do_oper;		/* Condition variable used while waiting to get lock */
        unsigned long long LBA_lower_limit;     /* Lower limit of LBA range for a given segment table */
        unsigned long long LBA_upper_limit;     /* Upper limit of LBA range for a given segment table */
        struct segment_table_data seg_table_data[MAX_THREADS + MAX_BWRC_THREADS];
};

/* segment_info structure hold number of segment tables and pointer to segment tables */
struct segment_info {
        int num_of_seg_tables;          /*Total number of segment tables created based of thread limit[SEG_TABLE_LIMIT] */
        struct segment_table *seg_table;
}seg_info;

/**********************************/
/***	Function Declarations 	***/
/**********************************/
unsigned long long set_first_blk(struct htx_data *, struct thread_context *);
void init_blkno (struct htx_data *, struct thread_context *);
void set_blkno (struct htx_data *, struct thread_context *);
void random_blkno (struct htx_data *, struct thread_context *tctx, unsigned long long len);
void random_dlen (struct thread_context *);
void set_seed (struct htx_data *htx_ds, struct thread_context *tctx);
void init_seed (struct htx_data *htx_ds, struct random_seed_t *seed);
unsigned int get_random_number (struct random_seed_t *);
void update_num_oper(struct thread_context *);
int wrap (struct thread_context *);
void do_boundary_check (struct htx_data *htx_ds, struct thread_context *tctx, int loop);
void do_fencepost_check (struct htx_data *, struct thread_context *);
void wait_for_fencepost_catchup (struct thread_context *tctx);
int allocate_buffers (struct htx_data *htx_ds, struct thread_context *tctx, unsigned int alignment, int malloc_count);
void free_buffers(int *malloc_count, struct thread_context *tctx);
void free_pattern_buffers (struct thread_context *tctx);
void clrbuf (char buf[], unsigned long long dlen);
void bld_header (struct htx_data *, struct thread_context *, unsigned char *, unsigned long long, char, unsigned short, unsigned int);
int bldbuf (struct htx_data *htx_ds, struct thread_context *tctx, unsigned short write_stamping);
int bld_buf (struct htx_data *htx_ds, struct thread_context *tctx, unsigned short write_stamping);
int update_header(struct thread_context *tctx, register unsigned short write_stamping);
void generate_pattern3 (unsigned short *, int , register unsigned int);
void generate_pattern4 (unsigned short *, int);
void generate_pattern5 (unsigned short *, int);
void generate_pattern7 (char *, int, unsigned int);
int generate_pattern8 (struct htx_data *, struct thread_context *, char *, int);
int generate_pattern9 (struct htx_data *, struct thread_context *, char *, int);
int generate_patternA(struct htx_data *, struct thread_context *, char *, int);

void prt_msg(struct htx_data *htx_ds, struct thread_context *tctx, int loop, int err, int sev, char *text);
void user_msg(struct htx_data *ps, int err, int io_err_flag, int sev, char *text);

#ifndef __HTX_LINUX__
int get_lun(struct htx_data *htx_ds, uchar *dev_name);
int get_parent_lun(uchar *dev_name);
void dump_iocmd(struct htx_data *htx_ds, struct sc_passthru *s);
#endif

void update_cache_threads_info(struct thread_context *tctx);
void wait_for_cache_threads_completion(struct htx_data *htx_ds, struct thread_context *tctx);

void seg_lock(struct htx_data *htx_ds, struct thread_context *tctx, unsigned long long flba, unsigned long long llba);
void seg_unlock(struct htx_data *htx_ds, struct thread_context *tctx, unsigned long long flba, unsigned long long llba);
void segment_table_init(void);
void initialize_fencepost (struct thread_context *);
unsigned int get_buf_alignment(struct thread_context *tctx);
void check_alignment(struct htx_data *, struct thread_context *);
void hang_monitor (struct htx_data *);

char get_write_status(unsigned long long);
void set_write_status(unsigned long long, char);
void update_state_table (unsigned long long, int);

#ifndef __CAPI_FLASH__
void update_aio_req_queue(int index, struct thread_context *tctx, char *buf);
int wait_for_aio_completion(struct htx_data *htx_ds, struct thread_context *tctx, char flag);
#endif


