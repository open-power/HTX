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
/* File name - io_oper.h                                            */
/* Header file to include all the strcuture/variables/functions     */
/* declaration associated with io_oper.c                            */
/********************************************************************/

#include "hxestorage_utils.h"
#include <sys/mman.h>

#ifdef __HTX_LINUX__
    #include <scsi/sg.h>
    #include <scsi/scsi.h>
#endif

#define DUMP_PATH "/tmp/"
#define MAX_MSG_DUMP 20
#define MAX_DUMP_DATA_LEN MAX_TEXT_MSG
#define MAX_MISCOMPARES 11

#ifdef __HTX_LINUX__
typedef loff_t  offset_t;
#endif

#define MMAP_FILE_SIZE (200 * KB)
extern char fsync_flag;

#ifdef __CAPI_FLASH__


#define BITS_USED               12
#define THREAD_ID_MASK          0xFFF0000000000000ULL
#define INDEX_MASK              0x000FFFFFFFFFFFFFULL

#define GENERATE_TAG(__thread_id__, __index__) 	 ( {               \
    uint64_t mask = 0;                                              \
    mask |= __thread_id__ << (sizeof(uint64_t) - BITS_USED);         \
    mask |= __index__;                                              \
    mask;                                                   \
    } );

#define GET_INDEX_FROM_TAG(__tag__) (__tag__ & INDEX_MASK) \


#endif


struct cache_thread {
    int th_num;
    int parent_th_num;
    struct htx_data cache_htx_ds;
};

/*************************/
/* Function Declarations */
/*************************/
int do_ioctl (struct htx_data *, int, int, void *);
int set_addr(struct htx_data *, struct thread_context *, int);
int open_disk (struct htx_data *, const char *, struct thread_context *);
int close_disk (struct htx_data *, struct thread_context *);

/* SYNC IO functions */
int read_disk (struct htx_data *, struct thread_context *, int);
int write_disk (struct htx_data *, struct thread_context *, int);
int compare_buffers (struct htx_data *, struct thread_context *, int);
int do_compare_with_discard(struct htx_data *, struct thread_context *, char *, char *, int, char *);
int do_compare (struct htx_data *, struct thread_context *, char *, char *, int, int *, int, char *);
int discard (struct htx_data *, struct thread_context *, int);

/* ASYNC IO functions */
#ifndef __CAPI_FLASH__
int read_async_disk(struct htx_data *, struct thread_context *, int);
int write_async_disk(struct htx_data *, struct thread_context *, int);
int cmp_async_disk (struct htx_data *, struct thread_context *, int);
#endif

/* PASSTHROUGH functions */
int open_passth_disk (struct htx_data *, const char *, struct thread_context *);
int close_passth_disk (struct htx_data *, struct thread_context *);
int read_passth_disk(struct htx_data *, struct thread_context *, int);
int write_passth_disk(struct htx_data *, struct thread_context *, int);
int verify_passth_disk (struct htx_data *, struct thread_context *, int);

/* CACHE functions */
int read_cache (struct htx_data *, struct thread_context *, int);
int write_cache (struct htx_data *, struct thread_context *, int);
int compare_cache (struct htx_data *, struct thread_context *, int);
int read_cache_disk(struct htx_data *, struct thread_context *);
int write_cache_disk(struct htx_data *, struct thread_context *);
void write_mem(struct cache_thread *);
void read_mem(struct cache_thread *);

/* SYNC cache functions */
int sync_cache(struct htx_data *, struct thread_context *, int);
int sync_cache_operation(struct htx_data *, int);

int run_cmd(struct htx_data *, char *);

#ifdef __CAPI_FLASH__
int open_lun(struct htx_data *htx_ds, const char * capi_device, struct thread_context * );
int close_lun(struct htx_data *htx_ds, struct thread_context * );
int cflsh_write_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop );
int cflsh_read_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop );
int cflsh_awrite_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop );
int cflsh_aread_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop );
int cflsh_aresult_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop );
#endif

