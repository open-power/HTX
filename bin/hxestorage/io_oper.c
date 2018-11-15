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

/****************************************************************/
/*  Filename:   io_oper.c                                       */
/*  contains all IO operation functions definition.             */
/****************************************************************/

#include "io_oper.h"

#define MAX_CACHE_PATTERNS 16

pthread_t cache_threads[MAX_NUM_CACHE_THREADS];

pthread_mutex_t cache_mutex;

extern pthread_mutex_t fencepost_mutex, dump_mutex;

#ifdef __HTX_LINUX__
void __attn(unsigned int a,
            unsigned int b,
            unsigned int c,
            unsigned int d,
            unsigned int e,
            unsigned int f,
            unsigned int g,
            unsigned int h) {
    __asm__ volatile (".long 0x00000200":);
}
#endif

#ifndef __HTX_LINUX__
    unsigned long long read_tb(void);
    #pragma mc_func read_tb {"7c6c42a6"}
    #pragma reg_killed_by read_tb gr3
#else
    #define read_tb(var) asm volatile ("mfspr %0, 268":"=r"(var));
#endif

#ifdef __CAPI_FLASH__
    extern volatile chunk_id_t shared_lun_id;
	extern int lun_type;
	extern char capi_device[MAX_STR_SZ];
	extern size_t chunk_size;
#endif

extern pthread_mutex_t thread_create_mutex;
extern pthread_cond_t threads_finished_cond_var;
extern int discard_enabled;
extern char *mmap_ptr;
extern int mmap_offset;

/***************************************************/
/****   Function to update the debug dump file  ****/
/***************************************************/
int update_dump_file (char *msg, char *err_str)
{
    int i, rc = 0;

    rc = pthread_mutex_lock(&dump_mutex);
    if (rc) {
        sprintf(err_str, "mutex lock failed in function update_dump_file, rc = %d\n", rc);
        return (-1);
    }

    if (mmap_offset + strlen(msg) >= MMAP_FILE_SIZE) {
         for (i = 0; i < MMAP_FILE_SIZE - mmap_offset; i++) {
              mmap_ptr[mmap_offset + i] = ' ';
         }
         mmap_offset = 0;
     }
     sprintf(mmap_ptr + mmap_offset, "%s", msg);
     mmap_offset += strlen(msg);

     rc = pthread_mutex_unlock(&dump_mutex);
     if (rc) {
        sprintf(err_str, "mutex unlock failed in function update_dump_file, rc = %d\n", rc);
        return (-1);
     }
    return (rc);
}

/**********************************************************************/
/****       Function to update Io timestamp in segment table       ****/
/**********************************************************************/
int update_timestamp (struct thread_context *tctx, char *err_str, int flag)
{
    int index, i = 0, rc = 0;
    time_t cur_time;

    cur_time = time(0);
    while (tctx->seg_table_num[i] != -1) {
        index = (seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].time_index;
        rc = pthread_mutex_lock (&((seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].time_mutex));
        if (rc != 0) {
            sprintf(err_str, "pthread_mutx_lock failed in update_timestamp for time_mutex. rc: %d\n", rc);
            return (rc);
        }
        /* Update timestamp in segment table */
        (seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].last_update_time = cur_time;
        (seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].io_update_time[index] = cur_time;
        (seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].time_index++;
        (seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].io_in_progress = flag;

        rc = pthread_mutex_unlock (&((seg_info.seg_table + tctx->seg_table_num[i])->seg_table_data[tctx->seg_table_index[i]].time_mutex));
        if (rc != 0) {
            sprintf(err_str, "pthread_mutx_unlock failed in update_timestamp for time_mutex. rc: %d\n", rc);
        }
        i++;
    }
    return (rc);
}

/***************************************************/
/****       Function to handle IOCTL            ****/
/***************************************************/
int do_ioctl (struct htx_data *htx_ds, int fd, int command, void *arg)
{
    int rcode = -1;
    char msg[256];
    int err_no = 0;
    unsigned int temp_retries = eeh_retries;

    do {
        rcode = ioctl(fd, command, arg);
        if ( rcode == -1) {
            err_no = errno;
            sprintf(msg, "Retry by hxestorage exerciser, on ioctl system call failure. errno: %d, retry number =  %d \n",
            err_no, (eeh_retries - (temp_retries - 1)));
            user_msg(htx_ds, err_no, 0, INFO, msg);
            sleep (5);
        }
        temp_retries--;
    } while (rcode == -1 && temp_retries > 0);

    return (rcode);
}

/************************************************************/
/*****      Function to handle open system call         *****/
/************************************************************/
int open_disk (struct htx_data *htx_ds, const char * pathname, struct thread_context * tctx)
{
    int rc = 0;
    char msg[256];
    int err_no = 0;
    unsigned int temp_retries = eeh_retries;

    do {
        rc = open (pathname, tctx->flag);
        if ( rc == -1 && temp_retries > 1) {
            err_no = errno;
            sprintf(msg, "Retry by hxestorage exerciser, on open system call failure  errno: %d. retry number =  %d \n",
            err_no, (eeh_retries - (temp_retries - 1)));
            user_msg(htx_ds, err_no, tctx->io_err_flag, INFO, msg);
            sleep (5);
        }
        temp_retries--;
    } while (rc == -1 && temp_retries > 0);

    if (rc == -1 && temp_retries == 0) {
        err_no = errno;
        tctx->io_err_flag = OPEN_ERR;
        sprintf(msg, "\nExerciser failed on max retries, open system call failed \n");
        user_msg(htx_ds, err_no, tctx->io_err_flag, HARD, msg);
    }
    RETURN(rc, tctx->io_err_flag);
}

/************************************************************/
/*****      Function to handle close system call        *****/
/************************************************************/
int close_disk (struct htx_data *htx_ds, struct thread_context * tctx)
{

    int rc = 0;
    char msg[256];
    int err_no = 0;
    unsigned int temp_retries = eeh_retries;

    do {
        rc = close (tctx->fd);
        if (rc == -1 && temp_retries > 1) {
            err_no = errno;
            sprintf(msg, "Retry by hxestorage exerciser, on close system call failure. errno: %d, retry number =  %d \n",
            err_no, (eeh_retries - (temp_retries - 1)));
            user_msg(htx_ds, err_no, tctx->io_err_flag, INFO, msg);
            sleep (5);
        }
        temp_retries--;
    } while (rc == -1 && temp_retries > 0);

    if (rc == -1 && temp_retries == 0) {
        err_no = errno;
        tctx->io_err_flag = CLOSE_ERR;
        sprintf(msg, "\nExerciser failed on max retries, close system call failed \n");
        user_msg(htx_ds, err_no, tctx->io_err_flag, HARD, msg);
    } else {
        tctx->fd = UNDEFINED;
    }

    RETURN(rc, tctx->io_err_flag);
}

#ifdef __CAPI_FLASH__

/******************************************************************************
 * Opens a CAPI attached Texan LUN, mode of open depends on
 * flag passed
 * lun_type - CBLK_OPN_VIRT_LUN/CBLK_OPN_PHY_LUN
 * open_flags - CBLK_OPN_VIRT_LUN/CBLK_OPN_PHY_LUN/UNDEFINED
 ******************************************************************************/
int open_lun(struct htx_data *htx_ds, const char * device, struct thread_context * tctx ) {

    int rc = 0;
    chunk_id_t chunk;
    char msg[1024];
    size_t size = 0;

	printf(" open_lun fr capi_device=%s, open_flags=%x, lun_type=%x \n", capi_device, tctx->open_flag, lun_type);
    if(tctx->open_flag == CBLK_OPN_VIRT_LUN) {
        /*
         * Check if rules specifies to open a LUN
         * Open a Virtual LUN on provided device
         */
        chunk = cblk_open(capi_device, MAX_THREADS, O_RDWR, 0, CBLK_OPN_VIRT_LUN | CBLK_OPN_SCRUB_DATA);
        if(chunk == NULL_CHUNK_ID) {
            sprintf(msg, " cblk_open for VLUN %s failed with rc = %d, errno= %d \n", capi_device, chunk, errno);
            user_msg(htx_ds, chunk, tctx->io_err_flag, HARD, msg);
            return(-1);
        }
        /*
         * Size of VLUN to open would be specified by user, allocate the VLUN of input provided
         */
        rc = cblk_set_size(chunk, chunk_size, CBLK_SCRUB_DATA_FLG);
        if(rc) {
            if(errno == ENOMEM) {
                rc = cblk_get_size(chunk, &size, CBLK_SCRUB_DATA_FLG);
                if(rc ) {
                    sprintf(msg, "cblk_get_size failed with rc = %d, errno = %d \n", rc, errno);
                    user_msg(htx_ds, errno, tctx->io_err_flag, HARD, msg);
                    return(-1);
                }
                if(size <=  chunk_size) {
                    sprintf(msg, "Cannot allocated requested size= %#lx, depricating to %#lx \n", chunk_size, size);
                    user_msg(htx_ds, 0, tctx->io_err_flag, INFO, msg);
                    chunk_size = size;
                    return(0);
                } else {
                    sprintf(msg, "Failed to allocated request size for VLUN, cblk_set_size failed wtih ENOMEM, device = %s \n", capi_device );
                    user_msg(htx_ds, ENOMEM, tctx->io_err_flag, HARD, msg);
                    return(-1);
                }
            } else {
                sprintf(msg, "cblk_set_size failed with rc = %d, errno = %d \n", rc, errno);
                user_msg(htx_ds, errno, tctx->io_err_flag, HARD, msg);
                return(-1);
            }
        }
        /*
         * All good, update the global LUN ID, so that other threads can share it ..
         */
		printf("open_VLUN for capi_dev=%s, successful for size=%ld  and chunk_id = %d \n", capi_device, chunk_size, chunk);
        shared_lun_id = chunk;
    } else if(tctx->open_flag == CBLK_OPN_PHY_LUN) {
        /* Open a Physical LUN on provided device */
        chunk = cblk_open(capi_device, MAX_THREADS, O_RDWR, 0, 0);
        if(chunk == NULL_CHUNK_ID) {
            sprintf(msg, " cblk_open for PHY_LUN %s failed with rc = %d, errno= %d \n", capi_device, chunk, errno);
            user_msg(htx_ds, chunk, tctx->io_err_flag, HARD, msg);
            return(-1);
        }
        /*
         * If physical LUN then return the size of PLUN created
         */
        rc = cblk_get_lun_size(chunk, &size, 0);
        if(rc ) {
            sprintf(msg, " cblk_get_lun_size for PLUN on %s failed with rc = %d, errno= %d \n", capi_device, chunk, errno);
            user_msg(htx_ds, chunk, tctx->io_err_flag, HARD, msg);
            return(-1);
        }
		printf("Successfully Opened PLUN, chunk=%#x, size returned = %#lx\n", chunk, size);
        chunk_size = size;
        shared_lun_id = chunk;
    } else {
        /* Some other thread has opened LUN, use the same */
        chunk = shared_lun_id;
        /* Race condition, thread opening LUN gets delayed then other threads need to wait. */
        while((chunk == NULL_CHUNK_ID)) {
            if(exit_flag == 1) {
                /** CleanUp...... **/
                return(1);
            }
            sleep(5);
            chunk = shared_lun_id;
        }
		printf("%s: Sharing LUN = %d, lun_type = %d \n", __FUNCTION__, chunk, lun_type);
        /*
         * Another thread, sharing LUN, then check if the chunk_size can satisfy our request else
         * depricate ..
         */
        if(lun_type == CBLK_OPN_VIRT_LUN) {
        /*
         * If LUN type which thread is going to share was VIRT or PHY
         */
            rc = cblk_get_size(chunk, &size, CBLK_SCRUB_DATA_FLG);
            if(rc ) {
                sprintf(msg, "cblk_get_size for SHARED VIRT LUN failed with rc = %d, errno = %d \n", rc, errno);
                user_msg(htx_ds, errno, tctx->io_err_flag, HARD, msg);
                return(-1);
            }
            chunk_size = size;
        } else {
            rc = cblk_get_lun_size(chunk, &size, 0);
            if(rc ) {
                sprintf(msg, " cblk_get_lun_size for SHARED VIRT LUN on %s failed with rc = %d, errno= %d \n", capi_device, chunk, errno);
                user_msg(htx_ds, chunk, tctx->io_err_flag, HARD, msg);
                return(-1);
            }
            chunk_size = size;
        }
    }
    return(chunk);
}

int close_lun(struct htx_data *htx_ds, struct thread_context *tctx) {

    int rc = 0, open_flag = -1;
    char msg[1024];

    if(tctx->force_op) {
		exit_flag = 1;
    }

    open_flag = tctx->open_flag;

    if(strcasecmp(tctx->oper, "BWRC") == 0 )  {
		rc = pthread_mutex_lock(&fencepost_mutex);
        if (rc) {
           	sprintf(msg, "mutex lock failed for fencepost_mutex in function execute_thread_context, rc = %d\n", rc);
           	user_msg(htx_ds, rc, 0, HARD, msg);
           	exit(rc);
        }

        lba_fencepost[tctx->fencepost_index].status = 'F';

        rc = pthread_mutex_unlock(&fencepost_mutex);
        if (rc) {
           	sprintf(msg, "mutex unlock failed for fencepost_mutex in function execute_thread_context, rc = %d\n", rc);
           	user_msg(htx_ds, rc, 0, HARD, msg);
           	exit(rc);
        }
	}
	/* Need to take thread_create_mutex lock and then update variables */
    rc = pthread_mutex_lock(&thread_create_mutex);
    if (rc) {
       	sprintf(msg, "2nd mutex lock failed in function execute_thread_context, rc = %d\n", rc);
       	user_msg(htx_ds, rc, 0, HARD, msg);
       	exit(rc);
    }

    if (strcasecmp(tctx->oper, "BWRC") == 0) {
       	BWRC_threads_running--;
       	printf("%s:thread_id: %s - non_BWRC_threads_running=%d, BWRC_threads_running=%d, force_op=%#x\n", __FUNCTION__, tctx->id,   non_BWRC_threads_running,
       			BWRC_threads_running, tctx->force_op);
    } else {
       	non_BWRC_threads_running--;
       	printf("%s:thread_id: %s - non_BWRC_threads_running=%d, BWRC_threads_running=%d, force_op=%#x\n", __FUNCTION__, tctx->id,   non_BWRC_threads_running,
       			BWRC_threads_running, tctx->force_op);
       	if (non_BWRC_threads_running == 0) {
           	rc = pthread_cond_broadcast(&threads_finished_cond_var);
           	if (rc) {
               	sprintf(msg, "Cond broadcast failed threads_finished_cond_var for in function execute_thread_context, rc = %d\n", rc);
               	user_msg(htx_ds, rc, 0, HARD, msg);
               	exit(rc);
            	}
        	}
    	}

	rc = pthread_mutex_unlock(&thread_create_mutex);
    if (rc) {
       	sprintf(msg, "2nd mutex unlock failed in function execute_thread_context, rc = %d\n", rc);
       	user_msg(htx_ds, rc, 0, HARD, msg);
       	exit(rc);
    }
	printf("%s:non_BWRC_threads_running=%d, BWRC_threads_running=%d\n",__FUNCTION__, non_BWRC_threads_running, BWRC_threads_running);
  	/*
	 * This thread opened LUN, it has to stay alive until, we are closing the LUN
   	 */
    if(open_flag == CBLK_OPN_VIRT_LUN || open_flag == CBLK_OPN_PHY_LUN) {
        while ((exit_flag == 0 && int_signal_flag == 'N' && signal_flag == 'N') ||
       	       ((BWRC_threads_running + non_BWRC_threads_running) != 0)) {
            sleep(2);
        }
        rc = cblk_close(shared_lun_id, CBLK_SCRUB_DATA_FLG);
        if(rc ) {
            sprintf(msg, " cblk_close failed with errno = %d \n", errno);
            user_msg(htx_ds,  errno, 0, HARD, msg);
            return(-1);
        }
        printf("%s:Closed !! VirtLUN with chunk_id = %d \n", __FUNCTION__,  shared_lun_id);
        shared_lun_id = NULL_CHUNK_ID;
    }
	pthread_exit(&rc);
}

int
cflsh_write_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop ) {

    int rc = 0, dump_rc = 0;
    char msg[1024], err_str[128];
	uint64_t nblocks = 0;

	nblocks = (tctx->dlen/dev_info.blksize);
	/*
	printf("%s: Writing at blk=%#llx, wbuf=%#llx, nblocks=%#llx \n", tctx->id, tctx->blkno[0],tctx->wbuf, nblocks);
	*/
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_IN_PROGRESS);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    rc = cblk_write(tctx->fd,  tctx->wbuf,  tctx->blkno[0], nblocks, 0);
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_DONE);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    if(rc == -1) {
        STATS_VAR_INC (bad_writes, 1, htx_ds);
        prt_msg(htx_ds, tctx, loop, errno, HARD, "write error - ");
        return(1);
    } else if(rc != nblocks) {
        STATS_VAR_INC (bad_writes, 1, htx_ds);
        sprintf(msg, "Attempted bytes write %lld (0x%llx) not equal to actual bytes\n" "write %d (0x%x).  Rule_id: %s, max_blkno: %#llx\n"
                    "first_block: %#llx, tot_blks: %#llx, current blk: %#llx", tctx->dlen, tctx->dlen, rc, rc, tctx->id, tctx->max_blkno,
                     tctx->first_blk, dev_info.maxblk, tctx->blkno[0]);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        return(1);
    } else {
        STATS_VAR_INC (good_writes, 1, htx_ds);
        STATS_VAR_INC (bytes_writ, tctx->dlen, htx_ds);
    }
    return (0);
}

int
cflsh_read_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop ) {

    int rc = 0, dump_rc = 0;
	uint64_t nblocks = 0;
    char msg[1024], err_str[128];

	nblocks = (tctx->dlen/dev_info.blksize);
	/*
	printf("%s: Reading from  blk=%#llx, wbuf=%#llx, nblocks=%#llx \n", tctx->id, tctx->blkno[0],tctx->rbuf, nblocks);
	*/
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_IN_PROGRESS);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    rc = cblk_read(tctx->fd,  tctx->rbuf,  tctx->blkno[0], nblocks, 0);
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_DONE);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    if(rc == -1) {
        STATS_VAR_INC (bad_reads, 1, htx_ds);
        prt_msg(htx_ds, tctx, loop, errno, HARD, "read error - ");
        return(1);
    } else if(rc != nblocks ) {
        STATS_VAR_INC (bad_reads, 1, htx_ds);
        sprintf(msg, "Attempted bytes read %lld (0x%llx) not equal to actual bytes\n" "read %d (0x%x).  Rule_id: %s, max_blkno: %#llx\n"
                    "first_block: %#llx, tot_blks: %#llx, current blk: %#llx", tctx->dlen, tctx->dlen, rc, rc, tctx->id, tctx->max_blkno,
                     tctx->first_blk, dev_info.maxblk, tctx->blkno[0]);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        return(1);
    } else {
        STATS_VAR_INC (good_reads, 1, htx_ds);
        STATS_VAR_INC (bytes_read, tctx->dlen, htx_ds);
    }
    return (0);
}

int cflsh_unmap_operation(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
#ifdef __HTX_LINUX__
    int rc = 0, dump_rc = 0;
    uint64_t nblocks = 0;
    char msg[1024], err_str[128];

    bzero(tctx->wbuf, tctx->dlen);
    nblocks = (tctx->dlen/dev_info.blksize);
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_IN_PROGRESS);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    rc = cblk_unmap(tctx->fd, tctx->wbuf, tctx->blkno[0], nblocks, 0);
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_DONE);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    if (rc == -1) {
        prt_msg(htx_ds, tctx, loop, errno, HARD, "unmap error - ");
        return (rc);
    } else if (rc != nblocks) {
        sprintf(msg, "Attempted unmap of blocks: %ld (0x%lx) not equal to actual unmap blocks: %d (0x%x).",
                nblocks, nblocks, rc, rc);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        return(1);
    }
#endif
    return(0);
}

int
cflsh_awrite_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop ) {

    int rc = 0;
    char msg[1024];
    uint32_t index;

    rc = cflsh_aresult_operation(htx_ds, tctx, loop);
    if(rc < 0) {
        sprintf(msg, "%s:cflsh_aresult_operation failed for Write  cur_async_io=%#x, chunk_id=%#x, dlen=%#llx"
                            "Rule_id: %s, max_blkno: %#llx\n, first_block: %#llx, tot_blks: %#llx, current blk: %#llx",
                        __FUNCTION__, tctx->cur_async_io, tctx->fd, tctx->dlen, tctx->id, tctx->max_blkno,
                        tctx->first_blk, dev_info.maxblk, tctx->blkno[0]);
       user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
       return(rc);
    }
    index = (uint64_t)rc;

    if(tctx->aio_req_queue[index].trf_len >=0 || ((tctx->aio_req_queue[index].status != CBLK_ARW_STATUS_SUCCESS) || (tctx->aio_req_queue[index].op != NOP))) {
        sprintf(msg, "%s:cflsh_aresult_operation gave incorrect index, trf_len=%#lx, status = %#lx \n", __FUNCTION__, tctx->aio_req_queue[index].trf_len, tctx->aio_req_queue[index].status);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        return(-1);
    }

    /* Issue the IO now */
    tctx->aio_req_queue[index].tag = GENERATE_TAG(tctx->th_num, index);
    rc = cblk_awrite(tctx->fd, tctx->wbuf, tctx->blkno[0], tctx->dlen, &tctx->aio_req_queue[index].tag, NULL, (CBLK_ARW_WAIT_CMD_FLAGS | CBLK_ARW_USER_TAG_FLAG));
    if(rc) {
        STATS_VAR_INC (bad_writes, 1, htx_ds);
        sprintf(msg, "Unable to queue Write tag=%#x, index = %#x, cur_async_io=%#x, chunk_id=%#x, dlen=%#llx"
                     "Rule_id: %s, max_blkno: %#llx\n, first_block: %#llx, tot_blks: %#llx, current blk: %#llx",
                     tctx->aio_req_queue[index].tag, index, tctx->cur_async_io, tctx->fd, tctx->dlen, tctx->id, tctx->max_blkno,
                     tctx->first_blk, dev_info.maxblk, tctx->blkno[0]);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        return(1);
    } else {
        tctx->aio_req_queue[index].op = 'W';
        tctx->aio_req_queue[index].status = CBLK_ARW_STATUS_PENDING;
        tctx->aio_req_queue[index].trf_len = tctx->dlen;
        tctx->cur_async_io++;
    }

    return(0);
}

int
cflsh_aread_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop ) {

    int rc = 0;
    char msg[1024];
    unsigned long long int index;

    rc = cflsh_aresult_operation(htx_ds, tctx, loop);
    if(rc < 0) {
        sprintf(msg, "%s:cflsh_aresult_operation failed for Read cur_async_io=%#x, chunk_id=%#x, dlen=%#llx"
                            "Rule_id: %s, max_blkno: %#llx\n, first_block: %#llx, tot_blks: %#llx, current blk: %#llx",
                        __FUNCTION__, tctx->cur_async_io, tctx->fd, tctx->dlen, tctx->id, tctx->max_blkno,
                        tctx->first_blk, dev_info.maxblk, tctx->blkno[0]);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        return(rc);
    }
    index = (unsigned long long int)rc;

    if(tctx->aio_req_queue[index].trf_len >=0 || ((tctx->aio_req_queue[index].status != CBLK_ARW_STATUS_SUCCESS) || (tctx->aio_req_queue[index].op != NOP))) {
        sprintf(msg, "%s:cflsh_aresult_operation gave incorrect index, trf_len=%#lx, status = %#lx \n", __FUNCTION__, tctx->aio_req_queue[index].trf_len, tctx->aio_req_queue[index].status);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        return(-1);
    }
    /* Issue the IO now */
    tctx->aio_req_queue[index].tag = GENERATE_TAG(tctx->th_num, index);
    rc = cblk_aread(tctx->fd, tctx->rbuf, tctx->blkno[0], tctx->dlen, &tctx->aio_req_queue[index].tag, NULL, (CBLK_ARW_WAIT_CMD_FLAGS | CBLK_ARW_USER_TAG_FLAG));
    if(rc) {
        STATS_VAR_INC (bad_reads, 1, htx_ds);
        sprintf(msg, "Unable to queue Read tag=%#x, index=%#llx, cur_async_io=%#x, chunk_id=%#x, dlen=%#llx"
                     "Rule_id: %s, max_blkno: %#llx\n, first_block: %#llx, tot_blks: %#llx, current blk: %#llx",
                     tctx->aio_req_queue[index].tag, index, tctx->cur_async_io, tctx->fd, tctx->dlen, tctx->id, tctx->max_blkno,
                     tctx->first_blk, dev_info.maxblk, tctx->blkno[0]);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        return(1);
    } else {
        tctx->aio_req_queue[index].op = 'R';
        tctx->aio_req_queue[index].status = CBLK_ARW_STATUS_PENDING;
        tctx->aio_req_queue[index].trf_len = tctx->dlen;
        tctx->cur_async_io ++;
    }

    return(0);
}

/****************************************************************************************
 * rc >=0 means index in aio_req_queue which just completed
 * rc < 0 means failure
 ***************************************************************************************/
int
cflsh_aresult_operation(struct htx_data * htx_ds, struct thread_context *tctx, int loop ) {

    if(tctx->cur_async_io < tctx->num_async_io) {
        /* Let aread/awrite go ahead and queue more operations */
        return(tctx->cur_async_io);
    }

    /* Max Outstanding reached, now check for status of each operation */
    int i, rc = 0 ;
    uint64_t status = -1;
    uint64_t tag = -1;
    char msg[1024];

retry :

    for(i = 0; i < tctx->num_async_io; i++) {
        rc = cblk_aresult(tctx->fd, &tctx->aio_req_queue[i].tag, &status, CBLK_ARESULT_USER_TAG);
        if(rc == -1) {
            sprintf(msg, "cblk_aresult failed for tag = %#lx, errno = %#x \n", tag, errno);
            user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
            return(-1);
        } else if(rc == 0) {
            /* Expected if this IO is not yet complete */
            continue;
        } else {
            /* Operation successfull, Check status of IO issued and reduce cur_async_io */
            if(tctx->aio_req_queue[i].trf_len != rc) {
                sprintf(msg, "cblk_aresult: Attempted bytes read %lld (0x%llx) not equal to actual bytes\n" "read %d (0x%x).  Rule_id: %s, max_blkno: %#llx\n"
                    "first_block: %#llx, tot_blks: %#llx, current blk: %#llx", tctx->dlen, tctx->dlen, rc, rc, tctx->id, tctx->max_blkno,
                    tctx->first_blk, dev_info.maxblk, tctx->blkno[0]);
                user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
                tctx->cur_async_io --;
                return(-1);
            }
            if(tctx->aio_req_queue[i].op  == 'R') {
                STATS_VAR_INC (good_reads, 1, htx_ds);
                STATS_VAR_INC (bytes_read, tctx->dlen, htx_ds);
            } else if(tctx->aio_req_queue[i].op  == 'W') {
                STATS_VAR_INC (good_writes, 1, htx_ds);
                STATS_VAR_INC (bytes_writ, tctx->dlen, htx_ds);
            }
            tctx->aio_req_queue[i].status = CBLK_ARW_STATUS_SUCCESS;
            tctx->aio_req_queue[i].trf_len = -1;
            tctx->aio_req_queue[i].op = 0;
            tctx->cur_async_io-- ;
            return(i);
        }
    }
    if(i == tctx->num_async_io) {
        usleep(10);
        goto retry;
    }
    return(-1);
}

#endif


/******************************************************/
/* Function actually doing the read operation on disk */
/******************************************************/
int disk_read_operation(struct htx_data *htx_ds, int filedes, void *buf, unsigned int len, offset_t addr)
{
    int rc = -1;
    char msg[512];
    int err_no = 0;
    unsigned int temp_retries = eeh_retries;

    do {
        rc = pread(filedes, buf, len, addr);
        if (rc == -1) {
            err_no = errno;
            sprintf(msg, "Retry by hxestorage exerciser, on read system call failure. errno: %d, retry number =  %d \n",
            err_no, (eeh_retries - (temp_retries - 1)));
            user_msg(htx_ds, err_no, 0, INFO, msg);
            sleep (5);
        }
        temp_retries--;
    } while (rc == -1 && temp_retries > 0);
    return rc;
}

/********************************************************/
/****   Function to read from disk for SYNC I/O     *****/
/********************************************************/
int read_disk (struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    long long rc = 0, dump_rc;
    char msg[512], *rbuf, info[256], err_str[128];
    unsigned long long time;
    offset_t addr;

    rbuf = tctx->rbuf;

    /****************************************************/
    /** initialize buffer only if compare is enabled    */
    /****************************************************/
    if(tctx->compare_enabled == 'y') {
        clrbuf(rbuf, tctx->dlen);
    }


#ifndef __HTX_LINUX__
    time = read_tb();
#else
    read_tb(time);
#endif

    sprintf(info, "thread_id: %s - time: 0x%llx, oper: %s, cur_oper: read, blkno: 0x%llx, num_blks: %d, rbuf addr: 0x%llx\n", tctx->id, time,
                tctx->oper, tctx->blkno[0], tctx->num_blks, (unsigned long long)tctx->rbuf);

    dump_rc = update_dump_file(info, err_str);
    if (dump_rc == -1) {
        prt_msg(htx_ds, tctx, loop, -1, HARD, err_str);
    }

    /***************************************************/
    /*****  Do read operation for eeh_retries time. ****/
    /***************************************************/
    addr = (offset_t)(tctx->blkno[0]) * (offset_t)(dev_info.blksize);
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_IN_PROGRESS);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    rc = disk_read_operation(htx_ds, tctx->fd, rbuf, tctx->dlen, addr);
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_DONE);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    if (rc == -1) {
        tctx->io_err_flag = READ_ERR;
        STATS_VAR_INC (bad_reads, 1, htx_ds);
        prt_msg(htx_ds, tctx, loop, errno, HARD, "read system call failed.\n");
        RETURN(rc, tctx->io_err_flag);
    } else if (rc != tctx->dlen) {
        tctx->io_err_flag = READ_ERR;
        sprintf(msg, "Attempted bytes read %lld (0x%llx) not equal to actual bytes\n" "read %lld (0x%llx).  Rule_id: %s, max_blkno: %#llx\n"
                    "first_block: %#llx, tot_blks: %#llx, current blk: %#llx", tctx->dlen, tctx->dlen, rc, rc, tctx->id, tctx->max_blkno,
                     tctx->first_blk, dev_info.maxblk, tctx->blkno[0]);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        RETURN(rc, tctx->io_err_flag);
    } else {
        STATS_VAR_INC (good_reads, 1, htx_ds);
        STATS_VAR_INC (bytes_read, tctx->dlen, htx_ds);
    }
    return (0);
}

/******************************************************/
/* Function actually doing the write operation on disk */
/******************************************************/
int disk_write_operation(struct htx_data *htx_ds, int filedes, void *buf, unsigned int len, offset_t addr)
{
    int rc = -1;
    int err_no = 0;
    char msg[512];
    unsigned int temp_retries = eeh_retries;

    do {
        rc = pwrite(filedes, buf, len, addr);
        if (rc == -1) {
            err_no = errno;
            sprintf(msg, "Retry by hxestorage exerciser, on write system call failure. errno: %d, retry number =  %d \n",
            err_no, (eeh_retries - (temp_retries - 1)));
            user_msg(htx_ds, err_no, 0, INFO, msg);
            sleep (5);
        }
        temp_retries--;
    } while (rc == -1 && temp_retries > 0);
    return rc;
}

/****************************************************/
/****   Function to write to disk for SYNC I/O  *****/
/****************************************************/
int write_disk (struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    long long rc = 0, dump_rc;
    char msg[512], info[256], err_str[128];
    unsigned long long time;
    offset_t addr;

#ifndef __HTX_LINUX__
    time = read_tb();
#else
    read_tb(time);
#endif

    sprintf(info, "thread_id: %s - time: 0x%llx, oper: %s, cur oper: write, blkno: 0x%llx, num_blks: %d, wbuf addr: 0x%llx\n", tctx->id, time,
                tctx->oper, tctx->blkno[0], tctx->num_blks, (unsigned long long)tctx->wbuf);
    dump_rc = update_dump_file(info, err_str);
    if (dump_rc == -1) {
        prt_msg(htx_ds, tctx, loop, -1, HARD, err_str);
    }

    /****************************************************/
    /*****  Do write operation for eeh_retries time. ****/
    /****************************************************/

    addr = (offset_t)(tctx->blkno[0]) * (offset_t)(dev_info.blksize);
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_IN_PROGRESS);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    rc = disk_write_operation (htx_ds, tctx->fd, tctx->wbuf, tctx->dlen, addr);
    if (tctx->mode != PERFORMANCE) {
        /* Update timestamp in segment table */
        dump_rc = update_timestamp(tctx, err_str, IO_DONE);
        if (dump_rc != 0) {
            user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
        }
    }
    if (rc == -1) {
        tctx->io_err_flag = WRITE_ERR;
        STATS_VAR_INC (bad_writes, 1, htx_ds);
        prt_msg(htx_ds, tctx, loop, errno, HARD, " write system call failed.\n");
        RETURN(rc, tctx->io_err_flag);
    } else if (rc != tctx->dlen) {
        tctx->io_err_flag = WRITE_ERR;
        if (enable_state_table) {
            /* update state table for the no. of blks for which write was successful */
            update_state_table(tctx->blkno[0], (rc / dev_info.blksize));
        }
        sprintf(msg, "Attempted bytes written %lld (0x%llx) not equal to actual bytes\n" "write %lld (0x%llx).  Rule_id: %s, max_blkno: %#llx\n"
                    "first_block: %#llx, tot_blks: %#llx, current blk: %#llx", tctx->dlen, tctx->dlen, rc, rc, tctx->id, tctx->max_blkno,
                    tctx->first_blk, dev_info.maxblk, tctx->blkno[0]);
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        RETURN(rc, tctx->io_err_flag);
    } else {
        if (enable_state_table) {
            update_state_table(tctx->blkno[0], tctx->num_blks);
        }
        STATS_VAR_INC (good_writes, 1, htx_ds);
        STATS_VAR_INC (bytes_writ, tctx->dlen, htx_ds);
    }
    return (0);
}

/********************************************************************/
/* discard - Function to do discard of the number of block specified*/
/* This is using BLKDISCARD ioctl.                                  */
/********************************************************************/

int discard (struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
#ifdef __HTX_LINUX__
    int rc = 0;
    unsigned long long range[2];

    range[0] = tctx->blkno[0] * dev_info.blksize;
    range[1] = tctx->dlen;

    rc = htx_ioctl(htx_ds, tctx->fd, DISCARD, (void *)range);
    return rc;
#else
    #if 0
        /* Currently adapter driver is not supporting this, so commented. */
        struct sc_passthru scpt;

        memset(&scpt, 0, sizeof(struct sc_passthru));
        scpt.version = 0x02;
        scpt.timeout_value = 60; /* default to 30 secs */

        /* Initialise scsi_cdb with the reqd data */
        scpt.scsi_cdb[0] = 0x93;  /* Write SAME with UNMAP cmd opcode */
        scpt.scsi_cdb[1] = (char) (0x1 << 3); /* sets the UNMAP bit */

        /* Update LBA no. from where to begin UNMAP */
        scpt.scsi_cdb[2] = (char)(tctx->blkno[0] >> 56 ) & 0xff;
        scpt.scsi_cdb[3] = (char)(tctx->blkno[0] >> 48 ) & 0xff;
        scpt.scsi_cdb[4] = (char)(tctx->blkno[0] >> 40 ) & 0xff;
        scpt.scsi_cdb[5] = (char)(tctx->blkno[0] >> 32 ) & 0xff;
        scpt.scsi_cdb[6] = (char)(tctx->blkno[0] >> 24 ) & 0xff;
        scpt.scsi_cdb[7] = (char)(tctx->blkno[0] >> 16 ) & 0xff;
        scpt.scsi_cdb[8] = (char)(tctx->blkno[0] >> 8 ) & 0xff;
        scpt.scsi_cdb[9] = (char)tctx->blkno[0] & 0xff;

        /* Update num of logical blocks to UNMAP */
        scpt.scsi_cdb[10] = (char)(tctx->num_blks >> 24 ) & 0xff;
        scpt.scsi_cdb[11] = (char)(tctx->num_blks >> 16 ) & 0xff;
        scpt.scsi_cdb[12] = (char)(tctx->num_blks >> 8 ) & 0xff;
        scpt.scsi_cdb[13] = (char)tctx->num_blks & 0xff;

        scpt.buffer = NULL;
        scpt.data_length = 0;
        scpt.command_length = 16;

        rc = ioctl(tctx->fd, DK_PASSTHRU, &scpt);
        if (rc == -1) {
            printf("einval_arg: %d, errno: %d\n", scpt.einval_arg, errno);
            exit(1);
	    }

    #endif
    return 0;
#endif
}

int check_zero(unsigned long long *ptr)
{
    int rc = -1, i;

    for (i = 0; i < dev_info.blksize / 8; i++) {
        if (ptr[i] != 0x0) {
            rc = i;
            break;
        } else {
            continue;
        }
    }
    return rc;
}

int check_one(unsigned long long *ptr)
{
    int rc = -1, i;

    for (i = 0; i < dev_info.blksize / 8; i++) {
        if (ptr[i] != 0xffffffffffffffffull) {
            rc = i;
            break;
        } else {
            continue;
        }
    }
    return rc;
}

int do_compare_with_discard(struct htx_data *htx_ds, struct thread_context *tctx, char *wbuf, char *rbuf,
                            int loop, char *msg)
{
    int offset, rc = -1;
    int cksum = 0;
    unsigned long long *ptr;

    for (offset = 0; offset < tctx->dlen; offset += dev_info.blksize) {
        ptr = (unsigned long long *) (rbuf + offset);
        if ((check_zero(ptr) == -1) ||
            (check_one(ptr) == -1)) {
            continue;
        } else {
            rc = do_compare (htx_ds, tctx, wbuf, rbuf, loop, &cksum, offset, msg);
            if (rc == -1) {
                continue;
            } else {
                break;
            }
        }
    }
    return rc;
}

/********************************************************************/
/* do_compare - actually does the compare of two buffers.  Checks   */
/* for a valid header at the start of each sector as is found in    */
/* pattern types 3, 4, and 5. Returns -1 if compare is OK, offset   */
/* of error if not.                                                 */
/********************************************************************/
int do_compare (struct htx_data *htx_ds, struct thread_context *tctx, char *wbuf, char *rbuf,
                int loop, int *cksum, int cmp_offset, char *msg)
{
    int rc = 0, error_found = 0, offset = -1;
    time_t time_buffer;
    register unsigned int j;
    unsigned long long i, compare_len;
    unsigned short *sptr;
    char cmd[256];
    int cmp_bytes = dev_info.blksize / (num_sub_blks_psect + 1);

#ifdef __CAPI_FLASH__
    compare_len = tctx->dlen;
#else
    if (discard_enabled == YES) {
        compare_len = dev_info.blksize;
    } else {
        compare_len = tctx->dlen;
    }
#endif

    for (i = cmp_offset; i < (cmp_offset + compare_len) && !error_found; i += cmp_bytes) {
        if (enable_state_table == YES && (*(unsigned short *) (wbuf + i + WRITE_STAMP_POS) == 0)) {
            /* means no previous write has been done for this LBA. So, don't compare */
            continue;
        }
        /* if the oper is a write followed by a read/compare. If so,    */
        /* then check all bytes including the header.                   */
        if (tctx->num_writes_remaining > 0) {
            offset = 0;
        } else if (memcmp( rbuf + i + BUFSIG_POS, BUFSIG, strlen(BUFSIG)) ||
            memcmp( wbuf + i + BUFSIG_POS, BUFSIG, strlen(BUFSIG)) ||
            memcmp( rbuf + i + HOSTNAME_POS, wbuf + i + HOSTNAME_POS, HOSTNAME_LEN) ||
            memcmp( rbuf + i + DEV_NAME_POS, wbuf + i + DEV_NAME_POS, DEV_NAME_LEN)) {
            /* Check for a valid host name and device name in this lba.     */
            /* Also, both the wbuf and the rbuf have to have the signature  */
            /* before we will ignore it in the comparison.                  */
            offset = 0;  /* Bad signature - start compare at 0 */
        } else {
            /* Check time stamp to make sure we aren't doing   */
            /* a compare against stale data.                   */
            #ifdef  __HTX_LE__
                time_buffer = rbuf[i + 11] << 24 | rbuf[i + 10] << 16 | rbuf[i + 9] << 8 | rbuf[i + 8];
            #else
                time_buffer = rbuf[i + 8] << 24 | rbuf[i + 9] << 16 | rbuf[i + 10] << 8 | rbuf[i + 11];
            #endif

            if (enable_state_table == YES) {
                if (memcmp(wbuf + i + WRITE_STAMP_POS, rbuf + i + WRITE_STAMP_POS, WRITE_STAMP_LEN)) {
                    sprintf(msg, "Data write state miscompare detected.\n"
                                 "expecetd write stamping: %d, Actual write stamping: %d.\n",
                                 *(unsigned short *) (wbuf + i + WRITE_STAMP_POS), *(unsigned short *) (rbuf + i + WRITE_STAMP_POS));
                    offset = WRITE_STAMP_POS;
                } else if (time_buffer < s_tbl.enablement_time) {
                    error_found = 1;
                }
		    } else {
                if ( time_buffer < time_mark ) {
                    error_found = 1;
                }
            }

            if (error_found) {
                sprintf(msg, "The buffer time stamp  (0x%x) is earlier than\n" "the control time stamp (0x%x).  Possible stale data"
                             " was written to\n" "the disk or disk was not initalized during this run.\n", (unsigned int)time_buffer,
                             (unsigned int)time_mark);
                prt_msg(htx_ds, tctx, loop, 0, SOFT, msg);
                offset = 8; /* time stamp */
            } else {
                /* First calc the sum of HEADER shorts. Note: the  */
                /* sum was gen'd from an array of shorts so use a  */
                /* (short *) to index it for calculating the sum.  */
                *cksum = 0;
                sptr = (unsigned short *) &rbuf[i];
                for (offset = 0; offset < OVERHEAD; offset++) {
                    *cksum += sptr[offset];
                }
                /* If sum is 0 then we don't want to compare the        */
                /* initial HEADER_SIZE bytes, else we want to compare   */
                /* the whole block. Also, if sum is 0 then the          */
                /* LBA fields should be compared as a final check       */
                /* to make sure the header can be ignored.              */
                if ((*cksum & 0xffff) || (*((unsigned long long *)(rbuf+i)) != *((unsigned long long *)(wbuf+i)))) {
                    offset = 0;
                } else {
                    offset = HEADER_SIZE ;
			    }
            }
        }

        j = i + offset;
        /* Perf : First compare using memcmp, if mismatch found then
         * return offset on char boundary
         */
        if (memcmp(&wbuf[j], &rbuf[j], (cmp_bytes - offset))) {
            while (offset++ < cmp_bytes) {
                if (wbuf[j] != rbuf[j]) {
                     if (tctx->mis_log_buf == NULL) {
                        /* For 1 block, it takes apprx. blk_sz * 8 bytes and we need space for
                         * 4 such blocks. Extra 4K is for additional info in the beginning.
                         * So, wee need to malloc those many bytes.
                         */
                        tctx->mis_log_buf = (char *) malloc((dev_info.blksize * 8 * 4) + (4 * KB));
                        if (tctx->mis_log_buf == NULL) {
                            sprintf(msg, "Malloc failed while allocating memory for miscompare log buffer. errno: %d\n", errno);
                            user_msg(htx_ds, errno, 0, HARD, msg);
                        } else {
                            analyze_miscompare(tctx, j, msg);
                        }
                    }
                   if (turn_attention_on == 1) {
                    #ifndef __HTX_LINUX__
                        attn( 0xBEEFDEAD, (unsigned long long)wbuf, (unsigned long long)rbuf, j, (unsigned long long)lba_fencepost,
                              (unsigned long long)tctx, tctx->dlen, (unsigned long long)tctx->mis_log_buf);
                    #else
                        __attn( 0xBEEFDEAD, (unsigned long long)wbuf, (unsigned long long)rbuf, j, (unsigned long long)lba_fencepost,
                                (unsigned long long)tctx, tctx->dlen, (unsigned long long)tctx->mis_log_buf);
                    #endif
                    }
                    if (run_on_misc == 'Y') {
                        sprintf(cmd, "%s %s %s 0x%llx %d > %s/pre_kdb_script_output", misc_run_cmd, dev_info.diskname, tctx->oper,
                                      tctx->blkno[0], tctx->num_blks, htx_ds->htx_exer_log_dir);
                        rc = system(cmd);
                        if (!WIFEXITED(rc)) {
                            sprintf(msg, "cmd %s could not be executed before entering into kDB.\n", misc_run_cmd);
                            /* user_msg(htx_ds, -1, 0, INFO, msg); */
						}
                    }
                    if (dev_info.crash_on_miscom) {
                    #ifndef __HTX_LINUX__
                        trap (0xBEEFDEAD, wbuf, rbuf, j, &lba_fencepost[0], tctx, tctx->dlen, tctx->mis_log_buf);
                    #else
                        do_trap_htx64 (0xBEEFDEAD, (unsigned long)wbuf, (unsigned long)rbuf, (unsigned long)j, (unsigned long)&lba_fencepost[0],
                                       (unsigned long)tctx, (unsigned long)tctx->dlen, (unsigned long)tctx->mis_log_buf);
                    #endif
                    }
                    error_found = 1;
                    break;
                }
                ++j;
            }
        }
    }
    return( error_found ? j : -1 );
}

/*****************************************************************************/
/* compare_buffers - Compare read buffer to write buffer. If a miscompare is */
/* detected and crash_on_mis = YES, the system will crash and go into the    */
/* the kernel debugger. The parameters to trap() will be loaded in the CPU   */
/* registers. The flag value 0xBEFFDEAD will in the first reg;  pointers     */
/* to the wbuf and rbuf will be in the 2nd and 3rd regs. The offset into the */
/* miscompare will be in the 4th reg and the 5th reg will hold a pointer to  */
/* the device causing the exerciser to go into the debugger.                 */
/*****************************************************************************/
int compare_buffers(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    long long offs, offs_reread = -1; /* Offset of miscompare if found                */
    int cksum = 0, dump_rc = 0, rc = 0, err_no = 0, bufrem;
    char *save_reread_buf = NULL; /* Pointer to a reread-buffer (if necessary) */
    static unsigned short miscompare_count = 0; /* miscompare count */
    unsigned int alignment, cnt = 0;
    char *rbuf = tctx->rbuf;
    char *wbuf = tctx->wbuf;
#ifndef __CAPI_FLASH__
    offset_t addr;
#endif
    char msg[MAX_TEXT_MSG], msg1[512];
    char path[128], path_w[128], path_r[128]; /* dump files path */
    char id_save[32];
    char err_str[ERR_STR_SZ], log_dir[32];
    FILE *fp;

    strcpy(msg, "");
#ifdef __CAPI_FLASH__
    offs = do_compare(htx_ds, tctx, wbuf, rbuf, loop, &cksum, 0, msg);
#else
    if (discard_enabled == YES) {
        offs = do_compare_with_discard(htx_ds, tctx, wbuf, rbuf, loop, msg);
    } else {
        offs = do_compare(htx_ds, tctx, wbuf, rbuf, loop, &cksum, 0, msg);
    }
#endif

    if (offs != -1) { /* problem with the compare?  */
        htx_ds->bad_others++;


        /********* Below section of code if reread is defined  ***********/
        if (tctx->run_reread == 'Y') {
            alignment = get_buf_alignment(tctx);
            tctx->reread_buf = (char *) NULL;
            tctx->reread_buf = malloc (tctx->dlen +  alignment + 128);
            if (tctx->reread_buf == NULL) {
                err_no = errno;
                STRERROR_R(err_no, err_str, ERR_STR_SZ);
                sprintf(msg1, "Can't malloc re-read buffer. errno: %d (%s) - re-read not done.\n",
                        err_no, err_str);
                strcat(msg, msg1);
            } else {
                save_reread_buf = tctx->reread_buf;
                /* Align re-read bufer if required */
                if (alignment) {
                    bufrem = ((unsigned long long)tctx->reread_buf % alignment);
                    if (bufrem != 0) {
                        tctx->reread_buf += alignment - bufrem;
                    }
                }
                /* Save the original stanza name and replace it with "!Re-Read"
                 * so that if read_disk gets an error and HTX halts it before
                 * it returns control to us, it's obvious that the error occurred
                 * during a re-read operation.  After read_disk returns, we'll
                 * copy the original stanza name back.
                 */
                strcpy(id_save, tctx->id);
                strcpy(tctx->id, "Re-read");
                clrbuf(tctx->reread_buf, tctx->dlen);

                /* Update timestamp in segment table */
                dump_rc = update_timestamp(tctx, err_str, IO_IN_PROGRESS);
                if (dump_rc != 0) {
                    user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
                }
            #ifdef __CAPI_FLASH__
                rc = cblk_read(tctx->fd,  tctx->reread_buf,  tctx->blkno[0], (tctx->dlen/dev_info.blksize), 0);
                /* Update timestamp in segment table */
                dump_rc = update_timestamp(tctx, err_str, IO_DONE);
                if (dump_rc != 0) {
                    user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
                }
                strcpy(tctx->id, id_save);
                if (rc != tctx->num_blks) {
            #else
                addr = (offset_t)(tctx->blkno[0]) * (offset_t)(dev_info.blksize);
			    strcpy(tctx->id, id_save);
                rc = disk_read_operation(htx_ds, tctx->fd, tctx->reread_buf, tctx->dlen, addr);
                /* Update timestamp in segment table */
                dump_rc = update_timestamp(tctx, err_str, IO_DONE);
                if (dump_rc != 0) {
                    user_msg(htx_ds, dump_rc, tctx->io_err_flag, HARD, err_str);
                }
                if (rc != tctx->dlen) {
            #endif
                    tctx->io_err_flag = READ_ERR;
                    sprintf (msg1, "***> Error trying to re-read disk bfr!\n");
                    strcat(msg, msg1);
                    user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
                    free(save_reread_buf);
                    tctx->reread_buf = (char *) 0;
                    goto free_mis_log_buf;
                } else {
                    rc = 0;
                #ifdef __CAPI_FLASH__
                    offs_reread = do_compare(htx_ds, tctx, wbuf, tctx->reread_buf, loop, &cksum, 0, msg);
                #else
                    if (discard_enabled == YES) {
                        offs_reread = do_compare_with_discard(htx_ds, tctx, wbuf, tctx->reread_buf, loop, msg);
                    } else {
                        offs_reread = do_compare(htx_ds, tctx, wbuf, tctx->reread_buf, loop, &cksum, 0, msg);
				    }
                #endif
                    if (offs_reread != -1) {
                        /* If reread fails then we treat it as a real miscompae */
                        rc = pthread_mutex_lock(&cache_mutex);
                        if (rc) {
                            sprintf(msg, "Mutex lock failed in function compare_buffer, rc = %d\n", rc);
                            user_msg(htx_ds, rc, 0, HARD, msg);
                            free(save_reread_buf);
                            exit(1);
                        }
                        miscompare_count++;
                        cnt = miscompare_count;
                        rc = pthread_mutex_unlock(&cache_mutex);
                        if (rc) {
                            sprintf(msg, "Mutex unlock failed in function compare_buffers, rc = %d\n", rc);
                            user_msg(htx_ds, rc, 0, HARD, msg);
                            free(save_reread_buf);
                            exit(1);
                        }
                    }
                }
            }
        }

        /*** Save buffers if miscomapre count is less than MAX_MISCOMPARES ***/
        if (cnt < MAX_MISCOMPARES) {
            /*** Save the miscompare analysis ***/
            strcpy(log_dir, htx_ds->htx_exer_log_dir);
            if (tctx->mis_log_buf != NULL) {
                sprintf(path, "%s/htx%s_mis_%-d", log_dir, &(htx_ds->sdev_id[5]), cnt);
            #if 0
                strcpy(path, log_dir);
                strcat(path, "htx");
                strcat(path, &(htx_ds->sdev_id[5]));
                strcat(path, "_mis_");
                sprintf(msg1, "%-d", cnt);
                strcat(path, msg1);
            #endif
                fp = fopen(path, "w");
                if (fp == NULL) {
                    sprintf(msg1, "fopen failed for %s. errno: %d\n", path, errno);
                    user_msg(htx_ds, errno, 0, HARD, msg1);
                } else {
                    fprintf(fp, "%s", tctx->mis_log_buf);
                    fclose(fp);
                    sprintf(msg1, "\nPlease see %s for detailed miscompare analysis\n", path);
                    strcat(msg, msg1);
                }
            }

            /*** Save the wbuf ***/
            sprintf(path_w, "%s/htx%s.wbuf%-d", log_dir, &(htx_ds->sdev_id[5]), cnt);
            hxfsbuf(wbuf, tctx->dlen, path_w, htx_ds);
            sprintf(msg1, "\nWrite buffer saved in %s\n", path_w);
            strcat(msg, msg1);

            /*** Save the rbuf ***/
            sprintf(path_r, "%s/htx%s.rbuf%-d", log_dir, &(htx_ds->sdev_id[5]), cnt);
            hxfsbuf(rbuf, tctx->dlen, path_r, htx_ds);
            sprintf(msg1, "Read buffer saved in %s\n", path_r);
            strcat(msg, msg1);

            /*** Save the reread buf ***/
            if (tctx->run_reread == 'Y') {
                if (tctx->reread_buf != (char *) NULL) {
                    if (offs_reread != -1) {
                        sprintf(path, "%s/htx%s.rerd%-d", log_dir, &(htx_ds->sdev_id[5]), cnt);
                        hxfsbuf(tctx->reread_buf, tctx->dlen, path, htx_ds);
                        strcat( msg, "Re-read fails compare at offset");
                        sprintf(msg1, "%lld; buffer saved in %s\n", offs_reread, path);
                        strcat(msg, msg1);
                    } else {
                        strcat(msg, "Re-read compares OK; buffer not saved.\n");
                    }
                }
                if (tctx->reread_buf) {
                    free(save_reread_buf);
                }
            }
            if (dev_info.cont_on_misc == NO) {
                user_msg(htx_ds, -1, 0, HARD, msg);
            } else {
				user_msg(htx_ds, -1, 0, MISCOM, msg);
            }
        } else {
            sprintf(msg1, "The maximum number of saved miscompare "
                            "buffers (%d) have already\nbeen saved. "
                            "The read and write buffers for this miscompare "
                            "will\nnot be saved to disk.\n", MAX_MISCOMPARES);
            strcat(msg, msg1);
            if (dev_info.cont_on_misc == NO) {
                user_msg(htx_ds, -1, 0, HARD, msg);
            } else {
                user_msg(htx_ds, -1, 0, MISCOM, msg);
                if (dev_info.cont_on_misc == YES) {
                    tctx->io_err_flag = MISCOM_ERR;
				}
            }
            if (tctx->run_reread == 'Y') {
                if (tctx->reread_buf != NULL) {
                    free(save_reread_buf);
                }
            }
            rc = -1;
        }
 free_mis_log_buf:
        if (tctx->mis_log_buf != NULL) {
            free(tctx->mis_log_buf);
            tctx->mis_log_buf = NULL;
        }
        RETURN(rc, tctx->io_err_flag);
    }
    return (0);
}

#ifndef __CAPI_FLASH__
/*******************************************************/
/*****  Function to read from disk for ASYNC I/O    ****/
/*******************************************************/
int read_async_disk(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    int index, err_no, rc = 0;
    char msg[128];

    index = wait_for_aio_completion(htx_ds, tctx, AIO_SINGLE);
    if (index == -1) {
        return(index);
    }

    update_aio_req_queue(index, tctx, tctx->rbuf);
    rc = aio_read(&tctx->aio_req_queue[index].aio_req);
    if (rc != 0) {
        err_no = errno;
        sprintf(msg, "aio_read error\n");
        prt_msg(htx_ds, tctx, loop, err_no, HARD, msg);
        return rc;
    } else {
        tctx->cur_async_io++;
    }
    return rc;
}

/*******************************************************/
/*****  Function to write to disk for ASYNC I/O     ****/
/*******************************************************/
int write_async_disk(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    int index, err_no, rc = 0;
    char msg[128];

    index = wait_for_aio_completion(htx_ds, tctx, AIO_SINGLE);
    if (index == -1) {
        return(index);
    }

    update_aio_req_queue(index, tctx, tctx->wbuf);
    rc = aio_write(&tctx->aio_req_queue[index].aio_req);
    if (rc != 0) {
        err_no = errno;
        sprintf(msg, "aio_write error.\n");
        prt_msg(htx_ds, tctx, loop, err_no, HARD, msg);
        return rc;
    } else {
        tctx->cur_async_io++;
    }
    return rc;
}

/*******************************************************/
/*****  Function to compare disk for ASYNC I/O     ****/
/*******************************************************/
int cmp_async_disk(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    return 0;
}
#endif

/***************************************************************/
/****   Function to handle open system call for passthrough ****/
/***************************************************************/
int open_passth_disk(struct htx_data *htx_ds, const char *pathname, struct thread_context * tctx)
{
#ifndef __HTX_LINUX__
    char tmp[32], msg[128];
    int rc, adapter, err_no;
    struct scsi_sciolst sciolst;

    strcpy(tmp, pathname);

    if ((adapter = open_disk (htx_ds, tmp, tctx)) == -1) {
        return (-1);
    }

    /** Issue SCIOSTART now */
    if (dev_info.parent_info.devsubtype == DS_SAS || dev_info.parent_info.devsubtype == 'D') {
        memset(&sciolst, 0, sizeof(struct scsi_sciolst));
        sciolst.version = SCSI_VERSION_1;
        sciolst.scsi_id = dev_info.scsi_id;
        sciolst.lun_id = dev_info.lun_id;
        sciolst.flags = (ISSUE_LOGIN | FORCED);
        rc = do_ioctl(htx_ds, adapter, (uint)SCIOLSTART, (char *)&sciolst);
    } else {
        rc = do_ioctl(htx_ds, adapter, SCIOSTART, (void *)IDLUN(dev_info.scsi_id_parent, dev_info.lun_id_parent));
    }
    if (rc != 0) {
        tctx->io_err_flag = IOCTL_ERR;
        err_no = errno;
        close(adapter);
        sprintf(msg, "SCIOLSTART failed.\n");
        user_msg(htx_ds, err_no, tctx->io_err_flag, HARD, msg);
        RETURN(-1, tctx->io_err_flag);
    }

    return adapter;
#else
    return 0;
#endif
}

/**************************************************************/
/***    Function to handle close system call for passth     ***/
/**************************************************************/
int close_passth_disk (struct htx_data *htx_ds, struct thread_context * tctx)
{
#ifndef __HTX_LINUX__
    int rc = 0, err_no = 0;
    char msg[128];
    struct scsi_sciolst sciolst;

    /* Issue SCIOSTOP */
    if(dev_info.parent_info.devsubtype == DS_SAS || dev_info.parent_info.devsubtype == 'D') {
        memset(&sciolst, 0, sizeof(struct scsi_sciolst));
        sciolst.version = SCSI_VERSION_1;
        sciolst.scsi_id = dev_info.scsi_id;
        sciolst.lun_id = dev_info.lun_id;
        rc = do_ioctl (htx_ds, tctx->fd, SCIOLSTOP, &sciolst);
    } else {
        rc = do_ioctl (htx_ds, tctx->fd, SCIOLSTOP, (void *)IDLUN(dev_info.scsi_id_parent, dev_info.lun_id_parent));
    }
    if( rc !=0 ) {
        tctx->io_err_flag = IOCTL_ERR;
        err_no = errno;
        close(tctx->fd);
        sprintf(msg, "SCIOLSTOP failed.\n");
        user_msg(htx_ds, err_no, tctx->io_err_flag, HARD, msg);
        RETURN(-1, tctx->io_err_flag);
    }
    if((close_disk(htx_ds, tctx)) == -1) {
        return(-1);
    }
    return rc;
#else
    return 0;
#endif
}

/*******************************************************/
/****   Function to read from disk for passthrough  ****/
/*******************************************************/
int read_passth_disk(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
#ifndef __HTX_LINUX__
    struct  scsi_iocmd iocmd;
    int i, err_no, rc = 0;
    char msg[256], msg_str[512];

    /*******************************************/
    /***    initialise sc_passthru struct   ****/
    /*******************************************/
    memset(&iocmd, 0, sizeof(struct scsi_iocmd));
    iocmd.version = SCSI_VERSION_2;
    iocmd.timeout_value = 30; /* default to 30 secs */
    iocmd.status_validity = 0;
    iocmd.scsi_id = dev_info.scsi_id; /* Device scsi_id */
    iocmd.lun_id = dev_info.lun_id;   /* Device lun_id  */
    iocmd.flags |= B_READ;
    iocmd.data_length =  tctx->dlen;
    iocmd.command_length = 10;
    iocmd.buffer = tctx->rbuf;   /* point iocmd buffer to the data we need to read from */

    iocmd.scsi_cdb[0] = 0x28; /* set operation code */
    iocmd.scsi_cdb[1] = (dev_info.lun_id << 5); /* obsolete field now */
    iocmd.scsi_cdb[2] = (uchar) (tctx->blkno[0] >> 24) & 0xff; /* LBA MSB */
    iocmd.scsi_cdb[3] = (uchar) (tctx->blkno[0] >> 16) & 0xff; /* LBA MSB */
    iocmd.scsi_cdb[4] = (uchar) (tctx->blkno[0] >> 8) & 0xff; /* LBA */
    iocmd.scsi_cdb[5] = (uchar) (tctx->blkno[0] & 0xff); /* LBA LSB */
    iocmd.scsi_cdb[7] = (tctx->num_blks >> 8) & 0xff;  /* transfer lgnth MSB */
    iocmd.scsi_cdb[8] = (tctx->num_blks & 0xff);       /* transfer lgnth LSB */
    iocmd.scsi_cdb[9] = 0;

    if (dev_info.debug_flag == 1) {
        sprintf(msg, "Attempting to send the passthru command- read-extended.\n");
        user_msg(htx_ds, 0, 0, INFO, msg);
    }
    rc = do_ioctl(htx_ds, tctx->fd, SCIOLCMD, (void *)&iocmd);
    if ( rc != 0 ) {
        err_no = errno;
        tctx->io_err_flag = IOCTL_ERR;
        sprintf( msg, "errno = %d, adap_set_flags = %d, status_validity = %d, scsi_bus_status = %d,"
                      " adapter_status = %d, sense_data_len = %d, sense_data_ptr: %llx\n ", err_no, iocmd.adap_set_flags,
                      iocmd.status_validity, iocmd.scsi_bus_status, iocmd.adapter_status, iocmd.autosense_length, (unsigned long long)iocmd.autosense_buffer_ptr);
        user_msg(htx_ds, 0, tctx->io_err_flag, INFO, msg);
        if (iocmd.autosense_buffer_ptr != NULL) {
            sprintf(msg_str, "Sense data:");
            for (i=0; i < iocmd.autosense_length; i++) {
                if (i % 16 == 0) {
                    sprintf(msg, "\n %02x", iocmd.autosense_buffer_ptr[i]);
                } else {
                    sprintf(msg, " %02x ", iocmd.autosense_buffer_ptr[i]);
                }
                strcat(msg_str, msg);
            }
            user_msg(htx_ds, 0, tctx->io_err_flag, INFO, msg_str);
        }

        iocmd.buffer = 0;
        STATS_VAR_INC (bad_reads, 1, htx_ds);
        prt_msg(htx_ds, tctx, loop, err_no, HARD, "read error(pass_thru) - ");
        RETURN(rc, tctx->io_err_flag);
    } else {
        if (dev_info.debug_flag == 1) {
            sprintf(msg, "Sent passthru command read-extended successfully.\n");
            user_msg(htx_ds, 0, 0, INFO, msg);
        }
        STATS_VAR_INC (good_reads, 1, htx_ds);
        STATS_VAR_INC (bytes_read, tctx->dlen, htx_ds);
        iocmd.buffer = 0;
        return 0;
    }
#else
    return 0;
#endif
}

/*******************************************************/
/*****  Function to write to disk for passthrough   ****/
/*******************************************************/
int write_passth_disk(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
#ifndef __HTX_LINUX__
    struct scsi_iocmd iocmd;
    int i, err_no, rc = 0;
    char msg[256], msg_str[512];

    /***********************************************/
    /****   initialise scsi iocmd structure     ****/
    /***********************************************/
    memset(&iocmd, 0, sizeof(struct scsi_iocmd));
    iocmd.version = (ushort)SCSI_VERSION_2;
    iocmd.timeout_value = 30; /* default to 30 secs */
    iocmd.status_validity = 0;
    iocmd.scsi_id = (unsigned long long)dev_info.scsi_id; /* Device scsi_id */
    iocmd.lun_id = (unsigned long long)dev_info.lun_id;   /* Device lun_id  */
    iocmd.flags |= B_WRITE;
    iocmd.data_length =  tctx->dlen;
    iocmd.command_length = 10;
    iocmd.buffer = tctx->wbuf;  /* Point iocmd buffer to the data we need to write */

    iocmd.scsi_cdb[0] = 0x2e; /* set operation code */
    iocmd.scsi_cdb[1] = 0x0; /* BYTE CHECK */
    iocmd.scsi_cdb[2] = (uchar) (tctx->blkno[0] >> 24) & 0xff; /* LBA MSB */
    iocmd.scsi_cdb[3] = (uchar) (tctx->blkno[0] >> 16) & 0xff; /* LBA MSB */
    iocmd.scsi_cdb[4] = (uchar) (tctx->blkno[0] >> 8) & 0xff; /* LBA */
    iocmd.scsi_cdb[5] = (uchar) (tctx->blkno[0] & 0xff); /* LBA LSB */
    iocmd.scsi_cdb[7] = (tctx->num_blks >> 8) & 0xff; /* transfer lgnth MSB */
    iocmd.scsi_cdb[8] = (tctx->num_blks & 0xff); /* transfer lgnth LSB */
    iocmd.scsi_cdb[9] = 0; /* Control */

    if (dev_info.debug_flag == 1) {
        sprintf(msg, "Attempting to send the passthru command- Write_Verify \n");
        user_msg(htx_ds, 0, 0, INFO, msg);
    }

    rc = do_ioctl(htx_ds, tctx->fd, SCIOLCMD, &iocmd);
    if (rc != 0 ) {
        tctx->io_err_flag = IOCTL_ERR;
        err_no = errno;
        sprintf( msg, "errno = %d, adap_set_flags = %d, status_validity = %d, scsi_bus_status = %d,"
                      " adapter_status = %d, sense_data_len = %d, sense_data_ptr: %llx\n ", err_no, iocmd.adap_set_flags,
                      iocmd.status_validity, iocmd.scsi_bus_status, iocmd.adapter_status, iocmd.autosense_length, (unsigned long long)iocmd.autosense_buffer_ptr);
        user_msg(htx_ds, 0, tctx->io_err_flag, INFO, msg);
        if (iocmd.autosense_buffer_ptr != NULL) {
            sprintf(msg_str, "Sense data:");
            for (i=0; i < iocmd.autosense_length; i++) {
                if (i % 16 == 0) {
                    sprintf(msg, "\n %02x", iocmd.autosense_buffer_ptr[i]);
                } else {
                    sprintf(msg, " %02x ", iocmd.autosense_buffer_ptr[i]);
                }
                strcat(msg_str, msg);
            }
            user_msg(htx_ds, 0, tctx->io_err_flag, INFO, msg_str);
        }
        iocmd.buffer = 0;
        STATS_VAR_INC (bad_writes, 1, htx_ds);
        prt_msg(htx_ds, tctx, loop, err_no, HARD, "Write error (pass_thru) - ");
        RETURN(rc, tctx->io_err_flag);
    } else {
        if (dev_info.debug_flag == 1) {
            sprintf(msg, "Sent passthru command-Write_Verify successfully.\n");
            user_msg(htx_ds, 0, 0, INFO, msg);
        }
        STATS_VAR_INC (good_writes, 1, htx_ds);
        STATS_VAR_INC (bytes_writ, tctx->dlen, htx_ds);

        iocmd.buffer = 0;
        return(0);
    }
#else
    return 0;
#endif
}

/***************************************************/
/****   Function to verify for passthrough      ****/
/***************************************************/
int verify_passth_disk (struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
#ifndef __HTX_LINUX__
    int rc, err_no, adapter;
    char msg[256], tmp[64];
    struct sc_passthru scpt;
    int temp_retries = eeh_retries;

    strcpy(tmp,"/dev/");
    strcat(tmp, dev_info.disk_parent);
	tctx->flag = O_RDWR;
    if ((adapter = open_disk(htx_ds, tmp,  tctx) ) == -1 ) {
        tctx->io_err_flag = OPEN_ERR;
        err_no = errno;
        sprintf(msg, "Open error in verify_passth_disk.\n");
        user_msg(htx_ds, err_no, tctx->io_err_flag, HARD, msg);
        RETURN(-1, tctx->io_err_flag);
    }

    rc = do_ioctl(htx_ds, adapter, SCIOSTOP, (void *)IDLUN(dev_info.scsi_id, dev_info.lun_id));
    if (rc !=0 ) {
        tctx->io_err_flag = IOCTL_ERR;
        err_no = errno;
        close(adapter);
        sprintf(msg, "first SCIOSTOP failed: errno.\n");
        user_msg(htx_ds, err_no, tctx->io_err_flag, HARD, msg);
        RETURN(rc, tctx->io_err_flag);
    }

    rc = do_ioctl(htx_ds, adapter, SCIOSTART, (void *)IDLUN(dev_info.scsi_id, dev_info.lun_id));
    if (rc !=0 ) {
        tctx->io_err_flag = IOCTL_ERR;
        err_no = errno;
        close(adapter);
        sprintf(msg, "SCIO START failed.\n");
        user_msg(htx_ds, err_no, tctx->io_err_flag, HARD, msg);
        RETURN(rc, tctx->io_err_flag);
    }

    /* initialise sc_passthru struct */
    memset(&scpt, 0, sizeof(struct sc_passthru));
    scpt.version = 0x02;
    scpt.timeout_value = 60; /* default to 30 secs */
    scpt.scsi_id = dev_info.scsi_id; /* Device scsi_id */
    scpt.lun_id = dev_info.lun_id;   /* Device lun_id  */
    scpt.q_tag_msg = SC_NO_Q;

    /* Initialise iocmd structure with the reqd data */
    scpt.scsi_cdb[0] = 0x2f; /* set operation code */
    scpt.scsi_cdb[1] = (dev_info.lun_id << 5); /* Obsolete field now */
    scpt.scsi_cdb[2] = (uchar) (tctx->blkno[0] >> 24) & 0xff; /* LBA MSB */
    scpt.scsi_cdb[3] = (uchar) (tctx->blkno[0] >>  16) & 0xff; /* LBA MSB */
    scpt.scsi_cdb[4] = (uchar) (tctx->blkno[0] >> 8) & 0xff; /* LBA */
    scpt.scsi_cdb[5] = (uchar) (tctx->blkno[0] >> 0xff); /* LBA LSB */
    scpt.scsi_cdb[6] = 0;
    scpt.scsi_cdb[7] = (tctx->num_blks >> 8) & 0xff; /* transfer lgnth MSB */
    scpt.scsi_cdb[8] = (tctx->num_blks & 0xff); /* transfer lgnth LSB */
    scpt.scsi_cdb[9] = 0;
    scpt.data_length =  0;
    scpt.command_length = 10;
    scpt.buffer = NULL;

    if (dev_info.debug_flag == 1) {
        sprintf(msg, "Attempting to send the passthru command- Verify.\n");
        user_msg(htx_ds, 0, 0, INFO, msg);
    }

    do { /* need to understand retries here */
        if (rc = do_ioctl(htx_ds, tctx->fd, SCIOCMD, &scpt) != 0 ) {
            tctx->io_err_flag = IOCTL_ERR;
            sprintf (msg, "error sending ioctl command = errno %d  einval_arg = %d \n" "status_validity = %d scsi_bus_status = %d",
                    errno, scpt.einval_arg, scpt.status_validity, scpt.scsi_bus_status );
            user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);

            if ((scpt.status_validity & 0x03) == SC_SCSI_ERROR) {
                if (scpt.scsi_bus_status == SC_CHECK_CONDITION) {
                    sprintf(msg, "\nSCSI status validity error\n");
                    user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
                }
            } else {
                if((scpt.status_validity & 0x03) == SC_ADAPTER_ERROR) {
                    sprintf(msg, "\nSCSI adapter status validity error\n");
                    user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
                }
            }
        } else {
            if (dev_info.debug_flag == 1) {
                sprintf(msg, "Sent passthru command- Verify successfully.\n");
                user_msg(htx_ds, 0, 0, INFO, msg);
            }
        }
    } while (temp_retries > 0 );
#else
    return 0;
#endif
}

/*****************************************************/
/***  Function to issue the write command to disk. ***/
/***  Data here will go to write cache             ***/
/*****************************************************/
int write_cache(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    long long rc = 0;
    char msg[MSG_TEXT_SIZE];
    offset_t addr;
    int i;

    update_cache_threads_info(tctx);
    memcpy(tctx->rbuf, tctx->wbuf, tctx->dlen);

    rc = pthread_mutex_lock(&(c_th_info[tctx->th_num].cache_mutex));
    if (rc) {
        sprintf(msg, "Mutex lock failed in function  write_cache, rc = %d\n", (int)rc);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }
    /* send signal to all cache threads to get started */
    rc = pthread_cond_broadcast(&c_th_info[tctx->th_num].do_oper_cond);
    if (rc) {
        sprintf(msg, "pthread_cond_broadcast failed for do_oper_cond in read_cache, rc = %d\n", (int)rc);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }
    rc = pthread_mutex_unlock(&c_th_info[tctx->th_num].cache_mutex);
    if (rc) {
        sprintf(msg, "Mutex unlock failed in function  write_cache, rc = %d\n", (int)rc);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(rc);
     }

    while(c_th_info[tctx->th_num].num_cache_threads_waiting > 0) {
        usleep(10000);
    }

    addr = (offset_t)(tctx->blkno[0]) * (offset_t)(dev_info.blksize);
    rc = disk_write_operation(htx_ds, tctx->fd, tctx->wbuf, tctx->dlen, addr);
    if (rc == -1) {
        c_th_info[tctx->th_num].cache_cond = errno;
    } else if (rc != tctx->dlen) {
        c_th_info[tctx->th_num].cache_cond = 2;
    } else {
        c_th_info[tctx->th_num].cache_cond = 0;
    }

    c_th_info[tctx->th_num].DMA_running = 0;
    while (c_th_info[tctx->th_num].num_cache_threads_waiting != c_th_info[tctx->th_num].cache_threads_created) {
        usleep(10000);
    }

#ifdef __HTX_LINUX__
    if (fsync_flag == 'Y') {
        rc = fsync(tctx->fd);
        if (rc == -1) {
            c_th_info[tctx->th_num].cache_cond = 50;
        }
    }
#endif

    if (c_th_info[tctx->th_num].cache_cond == 0) {
        for ( i = 0; i <= tctx->dlen; i++) {
			tctx->wbuf[i] = tctx->rbuf[i];
        }
        clrbuf(tctx->rbuf, tctx->dlen);
	    rc = disk_read_operation (htx_ds, tctx->fd, tctx->rbuf, tctx->dlen, addr);
	    if (rc == -1)  {
            tctx->io_err_flag = READ_ERR;
            STATS_VAR_INC (bad_reads, 1, htx_ds);
            prt_msg(htx_ds, tctx, loop, errno, HARD, "CACHE read error - ");
            RETURN(rc, tctx->io_err_flag);
	    } else if (rc != tctx->dlen ) {
            tctx->io_err_flag = READ_ERR;
            sprintf(msg, "Attempted bytes read %lld not equal to actual bytes read %lld\n", tctx->dlen, rc);
            user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
            RETURN(rc, tctx->io_err_flag);
	    } else {
			STATS_VAR_INC (good_reads, 1, htx_ds);
			STATS_VAR_INC (bytes_writ, tctx->dlen, htx_ds);
			return(0);
	    }
    } else if (c_th_info[tctx->th_num].cache_cond == 2) {
        tctx->io_err_flag = WRITE_ERR;
        sprintf(msg, "Attempted bytes written did not equal actual bytes written\n");
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        RETURN(c_th_info[tctx->th_num].cache_cond, tctx->io_err_flag);
#ifdef __HTX_LINUX_
    } else if (c_th_info[tctx->th_num].cache_cond == 50) {
	    tctx->io_err_flag = FSYNC_ERR;
	    sprintf(msg, "\nfsync failed...\n");
	    user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
	    RETURN(-1, tctx->io_err_flag);
#endif
    } else {
        STATS_VAR_INC (bad_writes, 1, htx_ds);
        prt_msg(htx_ds, tctx, loop, c_th_info[tctx->th_num].cache_cond, HARD, "CACHE write error - ");
        RETURN(c_th_info[tctx->th_num].cache_cond, tctx->io_err_flag);
    }
    return 0;
}

/**********************************************************************/
/***  Function to issue the write command to disk. Data here will   ***/
/***  come from to  cache because of previous write                 ***/
/**********************************************************************/
int read_cache (struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    long long rc = 0;
    offset_t addr;
    char msg[MSG_TEXT_SIZE];

    update_cache_threads_info(tctx);
    memset(tctx->rbuf, 0xbb, tctx->dlen);

    rc = pthread_mutex_lock(&(c_th_info[tctx->th_num].cache_mutex));
    if (rc) {
        sprintf(msg, "Mutex lock failed in function  read_cache, rc = %d\n", (int)rc);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }
    /* send signal to all cache threads to get started */
    rc = pthread_cond_broadcast(&c_th_info[tctx->th_num].do_oper_cond);
    if (rc) {
        sprintf(msg, "pthread_cond_broadcast failed for do_oper_cond in read_cache, rc = %d\n", (int)rc);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }
    rc = pthread_mutex_unlock(&c_th_info[tctx->th_num].cache_mutex);
    if (rc) {
        sprintf(msg, "Mutex unlock failed in function  read_cache, rc = %d\n", (int)rc);
        user_msg(htx_ds, rc, 0, HARD, msg);
        exit(rc);
    }

    while(c_th_info[tctx->th_num].num_cache_threads_waiting > 0) {
        usleep(10000);
    }

    addr = (offset_t)(tctx->blkno[0]) * (offset_t)(dev_info.blksize);
    rc = disk_read_operation(htx_ds, tctx->fd, tctx->rbuf, tctx->dlen, addr);
    if (rc == -1) {
        c_th_info[tctx->th_num].cache_cond = errno;
    } else if (rc != tctx->dlen) {
        c_th_info[tctx->th_num].cache_cond = 2;
    } else {
        c_th_info[tctx->th_num].cache_cond = 0;
    }
    c_th_info[tctx->th_num].DMA_running = 0;

    while (c_th_info[tctx->th_num].num_cache_threads_waiting != c_th_info[tctx->th_num].cache_threads_created) {
        usleep(10000);
    }

    if (c_th_info[tctx->th_num].cache_cond == 0) {
        STATS_VAR_INC (good_reads, 1, htx_ds);
        STATS_VAR_INC (bytes_read, tctx->dlen, htx_ds);
    } else if (c_th_info[tctx->th_num].cache_cond == 2) {
        tctx->io_err_flag = READ_ERR;
        sprintf(msg, "CACHE - attempted bytes read not equal actual bytes read");
        user_msg(htx_ds, -1, tctx->io_err_flag, HARD, msg);
        RETURN(c_th_info[tctx->th_num].cache_cond, tctx->io_err_flag);
    } else {
        tctx->io_err_flag = READ_ERR;
        STATS_VAR_INC (bad_reads, 1, htx_ds);
        prt_msg(htx_ds, tctx, loop, c_th_info[tctx->th_num].cache_cond, HARD, "read error - ");
        RETURN(c_th_info[tctx->th_num].cache_cond, tctx->io_err_flag);
    }
    return 0;
}

/*******************************************************************************/
/** Process to compare the buffers                                             */
/*******************************************************************************/
/** Notes on this routine:                                                    **/
/** The normal buffer comparison routine checks the header of each block of   **/
/** data to see if it's a pattern 3, 4, or 5 header.  In this routine we      **/
/** don't do that because we are always comparing the disk blocks against     **/
/** themselves, whether we just put them there or they were there already.    **/
/** The only thing we are concerned about is the appearance of the thread     **/
/** byte in the read buffer, and it's behavior is well defined.               **/
/*******************************************************************************/
int compare_cache(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    int  i, j, cmp_cnt, save_reread, err_no;
    int rc, err_cond;
    char s[3], path[128], cmp_str[512], msg[MAX_TEXT_MSG];
    char *save = NULL, stanza_save[9];
    char *rbuf, *wbuf, err_str[ERR_STR_SZ], log_dir[32];
    offset_t addr;
    unsigned int alignment, bufrem;
    static ushort misc_count = 0;

    err_cond = 0;
    rbuf = tctx->rbuf;
    wbuf = tctx->wbuf;
    for ( i = 0; i < tctx->dlen; i++ ) {
        if ((rbuf[i] == wbuf[i])                    ||
            ((rbuf[i] == 0x11) && (i % MAX_CACHE_PATTERNS == 0))    ||
            ((rbuf[i] == 0x22) && (i % MAX_CACHE_PATTERNS == 1))    ||
            ((rbuf[i] == 0x33) && (i % MAX_CACHE_PATTERNS == 2))    ||
            ((rbuf[i] == 0x44) && (i % MAX_CACHE_PATTERNS == 3))    ||
            ((rbuf[i] == 0x55) && (i % MAX_CACHE_PATTERNS == 4))    ||
            ((rbuf[i] == 0x66) && (i % MAX_CACHE_PATTERNS == 5))    ||
            ((rbuf[i] == 0x77) && (i % MAX_CACHE_PATTERNS == 6))    ||
            ((rbuf[i] == 0x88) && (i % MAX_CACHE_PATTERNS == 7))    ||
            ((rbuf[i] == 0x99) && (i % MAX_CACHE_PATTERNS == 8))    ||
            ((rbuf[i] == 0xaa) && (i % MAX_CACHE_PATTERNS == 9))    ||
            ((rbuf[i] == 0xbb) && (i % MAX_CACHE_PATTERNS == 10))    ||
            ((rbuf[i] == 0xcc) && (i % MAX_CACHE_PATTERNS == 11))    ||
            ((rbuf[i] == 0xdd) && (i % MAX_CACHE_PATTERNS == 12))    ||
            ((rbuf[i] == 0xee) && (i % MAX_CACHE_PATTERNS == 13))    ||
            ((rbuf[i] == 0xff) && (i % MAX_CACHE_PATTERNS == 14))    ||
            ((rbuf[i] == 0x5a) && (i % MAX_CACHE_PATTERNS == 15)) );
        else {
            cmp_cnt = i;
            err_cond = 1;
            if (dev_info.crash_on_miscom) {
            #ifdef __HTX_LINUX__
                do_trap_htx64( 0xBEEFDEAD, (unsigned long)tctx->wbuf, (unsigned long)tctx->rbuf, (unsigned long)i, (unsigned long)htx_ds, (unsigned long)tctx, 0 , 0);
            #else
                trap(0xBEEFDEAD, tctx->wbuf, tctx->rbuf, i, htx_ds, tctx);
            #endif
            }
            i = tctx->dlen;
        }
    }

    if ( err_cond == 0 ) {
        return (0);
    } else {
        sprintf(msg, "\nMiscompare in oper %s at displacement %d (0x%x)\n" "Block Number = %lld (%llx), Maximum LBA = %lld (%llx)",
                tctx->oper, cmp_cnt, cmp_cnt, tctx->blkno[0], tctx->blkno[0], tctx->max_blkno, tctx->max_blkno);
        sprintf(cmp_str,"\nCompare had to match 0x11, 0x22, 0x33, 0x44," " 0x55, 0x66, 0x77, 0x88, 0x99, "
                        "0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x5a \nor the pattern that was  "
                        "read from the disk.");
        strcat(msg, cmp_str);
        sprintf(cmp_str, "\nwbuf(0x%llx)", (unsigned long long)tctx->wbuf);
        strcat(msg, cmp_str);
        for ( j = cmp_cnt; ((j - cmp_cnt) < MAX_MSG_DUMP) && (j < tctx->dlen); j++ ) {
            sprintf(s, "%.2x", tctx->wbuf[j]);
            strcat(msg, s);
        }
        sprintf(cmp_str, "\nrbuf(0x%llx)", (unsigned long long)tctx->rbuf);
        strcat(msg, cmp_str);
        for ( j = cmp_cnt; ((j - cmp_cnt) < MAX_MSG_DUMP) && (j < tctx->dlen); j++) {
            sprintf(s, "%.2x", tctx->rbuf[j]);
            strcat(msg, s);
        }
        strcat(msg, "\n");

        alignment = get_buf_alignment(tctx);
        tctx->reread_buf= (char *) 0; /* This buffer will hold the re-read block      */
        save_reread = 0;
        tctx->reread_buf = (char *) malloc(tctx->dlen + alignment + 128);
        if(tctx->reread_buf == NULL) {
            err_no = errno;
            STRERROR_R(err_no, err_str, ERR_STR_SZ);
            sprintf(cmp_str,"** Can't malloc re-read buf in compare_cache: errno %d (%s)\n",
                    err_no, err_str);
            strcat(msg, cmp_str);
        } else {
            save = tctx->reread_buf;
            /* do alignment setup */
            if (alignment) {
                bufrem = (unsigned long long)tctx->reread_buf % alignment;
                if (bufrem != 0) {
                    tctx->reread_buf += alignment - bufrem;
                }
            }

            /* Save the original stanza name and replace it with "!Re-Read"
             * so that if read_disk gets an error and HTX halts it before
             * it returns control to us, it's obvious that the error occurred
             * during a re-read operation.  After read_disk returns, we'll
             * copy the original stanza name back.
             */
            strcpy(stanza_save, tctx->id);
            strcpy(tctx->id, "#Re-Read");
            clrbuf(tctx->reread_buf, tctx->dlen);
            addr = (offset_t)(tctx->blkno[0]) * (offset_t)(dev_info.blksize);
            rc = disk_read_operation(htx_ds, tctx->fd, tctx->reread_buf, tctx->dlen, addr);
            strcpy(tctx->id, stanza_save);
            if (rc != tctx->dlen) {
                sprintf(cmp_str, "CACHE > Error trying to re-read disk buffer!\n");
                strcat(msg, cmp_str);
                free(save);
                tctx->reread_buf = (char *) 0;
            } else {
                err_cond = 0;
                for ( i = 0; i < tctx->dlen; i++ ) {
                    if ((tctx->reread_buf[i] == wbuf[i])                    ||
                        ((tctx->reread_buf[i] == 0x11) && (i % MAX_CACHE_PATTERNS == 0))    ||
                        ((tctx->reread_buf[i] == 0x22) && (i % MAX_CACHE_PATTERNS == 1))    ||
                        ((tctx->reread_buf[i] == 0x33) && (i % MAX_CACHE_PATTERNS == 2))    ||
                        ((tctx->reread_buf[i] == 0x44) && (i % MAX_CACHE_PATTERNS == 3))    ||
                        ((tctx->reread_buf[i] == 0x55) && (i % MAX_CACHE_PATTERNS == 4))    ||
                        ((tctx->reread_buf[i] == 0x66) && (i % MAX_CACHE_PATTERNS == 5))    ||
                        ((tctx->reread_buf[i] == 0x77) && (i % MAX_CACHE_PATTERNS == 6))    ||
                        ((tctx->reread_buf[i] == 0x88) && (i % MAX_CACHE_PATTERNS == 7))    ||
                        ((tctx->reread_buf[i] == 0x99) && (i % MAX_CACHE_PATTERNS == 8))    ||
                        ((tctx->reread_buf[i] == 0xaa) && (i % MAX_CACHE_PATTERNS == 9))    ||
                        ((tctx->reread_buf[i] == 0xbb) && (i % MAX_CACHE_PATTERNS == 10))    ||
                        ((tctx->reread_buf[i] == 0xcc) && (i % MAX_CACHE_PATTERNS == 11))    ||
                        ((tctx->reread_buf[i] == 0xdd) && (i % MAX_CACHE_PATTERNS == 12))    ||
                        ((tctx->reread_buf[i] == 0xee) && (i % MAX_CACHE_PATTERNS == 13))    ||
                        ((tctx->reread_buf[i] == 0xff) && (i % MAX_CACHE_PATTERNS == 14))    ||
                        ((tctx->reread_buf[i] == 0x5a) && (i % MAX_CACHE_PATTERNS == 15)) );
                    else {
                        err_cond = 1;
                        i = tctx->dlen;
                    }
                }
                save_reread = err_cond;
            }
        }
        misc_count++;
        if (misc_count < MAX_MISCOMPARES) {
            strcpy(log_dir, htx_ds->htx_exer_log_dir);
            sprintf(path, "%s/htx%s.crbuf%-d", log_dir, &(htx_ds->sdev_id[5]), misc_count);
        #if 0
            strcpy(path, log_dir);    /* set up path for the read buffer */
            strcat(path, "/htx");
            strcat(path, &(htx_ds->sdev_id[5]));
            strcat(path, ".crbuf");
            sprintf(cmp_str, "%-d", misc_count);
            strcat(path, cmp_str);
        #endif
            hxfsbuf(rbuf, tctx->dlen, path, htx_ds);
            sprintf(path, "%s/htx%s.cwbuf%-d", log_dir, &(htx_ds->sdev_id[5]), misc_count);
        #if 0
            strcpy(path, log_dir);    /* set up path for the write buffer */
            strcat(path, "/htx");
            strcat(path, &(htx_ds->sdev_id[5]));
            strcat(path, ".cwbuf");
            sprintf(cmp_str, "%-d", misc_count);
            strcat(path, cmp_str);
        #endif
            hxfsbuf(wbuf, tctx->dlen, path, htx_ds);
            if (tctx->reread_buf != (char *) 0) {
                if (save_reread) {
                    sprintf(path, "%s/htx%s.crerd%-d", log_dir, &(htx_ds->sdev_id[5]), misc_count);
                #if 0
                    strcpy(path, log_dir);
                    strcat(path, "/htx");
                    strcat(path, &(htx_ds->sdev_id[5]));
                    strcat(path, ".crerd");
                    sprintf(cmp_str, "%-d", misc_count);
                    strcat(path, cmp_str);
                #endif
                    hxfsbuf(tctx->reread_buf, tctx->dlen, path, htx_ds);
                    strcat(msg, "Re-read fails to compare; ");
                    sprintf(cmp_str, "buffer saved in %s\n", path);
                    strcat(msg, cmp_str);
                } else {
                    strcat(msg, "Re-read compares OK; buffer not saved.\n");
                }
                free(save);
                tctx->reread_buf = NULL;
            }
            user_msg(htx_ds, -1, 0, MISCOM, msg);
        } else {
            sprintf(cmp_str, "The maximum number of saved miscompare buffers " "in the CACHE rules (%d) have already\nbeen saved."
                    " The read (CRBUF) and write (CWBUF) buffers for this" "\nmiscompare will not be saved to the disk.\n", MAX_MISCOMPARES);
            strcat(msg, cmp_str);
            user_msg(htx_ds, -1, 0, HARD, msg);
            if (tctx->reread_buf != (char *) 0) {
                free(save);
                tctx->reread_buf = NULL;
            }
        }
        return(1);
    }
}

/********************************************/
/*  Function to write directly to memory.   */
/********************************************/
void write_mem(struct cache_thread *c_th)
{
    int rc = 0;
    long long i;
    char pattern = 'x', *buf;
    char msg[MSG_TEXT_SIZE];
    if (c_th->th_num % MAX_CACHE_PATTERNS == 0) {
        pattern = 0x11;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 1) {
        pattern = 0x22;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 2) {
        pattern = 0x33;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 3) {
        pattern = 0x44;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 4) {
        pattern = 0x55;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 5) {
        pattern = 0x66;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 6) {
        pattern = 0x77;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 7) {
        pattern = 0x88;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 8) {
        pattern = 0x99;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 9) {
        pattern = 0xaa;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 10) {
        pattern = 0xbb;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 11) {
        pattern = 0xcc;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 12) {
        pattern = 0xdd;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 13) {
        pattern = 0xef;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 14) {
        pattern = 0xff;
    } else if (c_th->th_num % MAX_CACHE_PATTERNS == 15) {
        pattern = 0x5a;
    }
    while (1) {
        /* wait to get the signal from parent thread to get started */
        rc = pthread_mutex_lock(&c_th_info[c_th->parent_th_num].cache_mutex);
        if (rc) {
            sprintf(msg, "Mutex lock failed in write_mem. rc = %d\n", rc);
            user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
            exit(1);
        }
        c_th_info[c_th->parent_th_num].num_cache_threads_waiting++;
        rc = pthread_cond_wait(&c_th_info[c_th->parent_th_num].do_oper_cond, &c_th_info[c_th->parent_th_num].cache_mutex);
        if (rc) {
            sprintf(msg, "pthread_cond_wait failed in write_mem. rc = %d\n", rc);
            user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
            exit(1);
        }
        c_th_info[c_th->parent_th_num].num_cache_threads_waiting--;
        rc = pthread_mutex_unlock(&c_th_info[c_th->parent_th_num].cache_mutex);
        if (rc) {
            sprintf(msg, "Mutex unlock failed in write_mem. rc = %d\n", rc);
            user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
            exit(1);
        }
        /* Exit if testcase is done */
        if (c_th_info[c_th->parent_th_num].testcase_done == 1) {
            break;
        }
        buf = (char *) c_th_info[c_th->parent_th_num].buf;
        i = c_th->th_num;
        while (c_th_info[c_th->parent_th_num].DMA_running) {
            buf[i] = pattern;
            i += 128;
            if (i >= c_th_info[c_th->parent_th_num].dlen) {
                i = c_th->th_num;
            }
        }
    }
    rc = pthread_mutex_lock(&c_th_info[c_th->parent_th_num].cache_mutex);
    if (rc) {
        sprintf(msg, "Mutex lock failed in write_mem. rc = %d\n", rc);
        user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
        exit(1);
    }
    c_th_info[c_th->parent_th_num].cache_threads_created--;
    rc = pthread_mutex_unlock(&c_th_info[c_th->parent_th_num].cache_mutex);
    if (rc) {
        sprintf(msg, "Mutex unlock failed in write_mem. rc = %d\n", rc);
        user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
        exit(1);
    }
    pthread_exit(NULL);
}

/****************************************************/
/*   Function to read directly from memory.     *****/
/****************************************************/
void read_mem(struct cache_thread *c_th)
{
    int rc = 0;
    long long i;
    char hold_pattern, msg[MSG_TEXT_SIZE];

    while (1) {
        /* wait to get the signal from parent thread to get started */
        rc = pthread_mutex_lock(&c_th_info[c_th->parent_th_num].cache_mutex);
        if (rc) {
            sprintf(msg, "Mutex lock failed in read_mem. rc = %d\n", rc);
            user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
            exit(1);
        }
        c_th_info[c_th->parent_th_num].num_cache_threads_waiting++;
        rc = pthread_cond_wait(&c_th_info[c_th->parent_th_num].do_oper_cond, &c_th_info[c_th->parent_th_num].cache_mutex);
        if (rc) {
            sprintf(msg, "pthread_cond_wait failed in read_mem. rc = %d\n", rc);
            user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
            exit(1);
        }
        c_th_info[c_th->parent_th_num].num_cache_threads_waiting--;
        rc = pthread_mutex_unlock(&c_th_info[c_th->parent_th_num].cache_mutex);
        if (rc) {
            sprintf(msg, "Mutex unlock failed in read_mem. rc = %d\n", rc);
            user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
            exit(1);
        }
        /* Exit if testcase is done */
        if (c_th_info[c_th->parent_th_num].testcase_done == 1) {
            break;

        }
        i = c_th->th_num;
        while (c_th_info[c_th->parent_th_num].DMA_running) {
            hold_pattern = c_th_info[c_th->parent_th_num].buf[i];
            hold_pattern++; /* Due to optimization, Doing any operation will actually read from memory  */
            i += 128;
            if (i >= c_th_info[c_th->parent_th_num].dlen) {
                i = c_th->th_num;
            }
        }
    }
    rc = pthread_mutex_lock(&c_th_info[c_th->parent_th_num].cache_mutex);
    if (rc) {
        sprintf(msg, "Mutex lock failed in read_mem. rc = %d\n", rc);
        user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
        exit(1);
    }
    c_th_info[c_th->parent_th_num].cache_threads_created--;
    rc = pthread_mutex_unlock(&c_th_info[c_th->parent_th_num].cache_mutex);
    if (rc) {
        sprintf(msg, "Mutex unlock failed in read_mem. rc = %d\n", rc);
        user_msg(&c_th->cache_htx_ds, rc, 0, HARD, msg);
        exit(1);
    }
    pthread_exit(NULL);
}

/*****************************************************************************/
/*    Function to Read Disk and Write to Memory at the same time. Checks     */
/* cache processing.                                                         */
/*****************************************************************************/
int read_cache_disk(struct htx_data *htx_ds, struct thread_context *tctx)
{
    char msg[256];
    struct cache_thread *c_th = c_th_info[tctx->th_num].cache_th;
    int rc = 0, th_index = 0;

    pthread_mutex_init(&(c_th_info[tctx->th_num].cache_mutex), DEFAULT_MUTEX_ATTR_PTR);
    pthread_cond_init(&(c_th_info[tctx->th_num].do_oper_cond), DEFAULT_COND_ATTR_PTR);

    c_th_info[tctx->th_num].cache_threads_created = 0;

    while (th_index < tctx->num_cache_threads - 1) {
        c_th_info[tctx->th_num].cache_threads_created++;
        c_th[th_index].th_num = th_index;
        c_th[th_index].parent_th_num = tctx->th_num;
        memcpy(&(c_th[th_index].cache_htx_ds), htx_ds, sizeof(struct htx_data));

        if (strcasecmp(tctx->oper, "CARW") == 0) {
            rc = pthread_create(&(cache_threads[th_index]), &thread_attrs_detached, (void *(*)(void *))write_mem,
                (void*)(&c_th[th_index]));
        } else {
            rc = pthread_create(&(cache_threads[th_index]), &thread_attrs_detached, (void *(*)(void *))read_mem,
                (void*)(&c_th[th_index]));
        }
        if (rc) {
            sprintf(msg, "pthread_create failed for creating cache threads in read_cache_disk()."
                         " errno is: %d\n", rc);
            user_msg(htx_ds, rc, 0, HARD, msg);
            exit(1);
	    }
        th_index++;
    }
    while (c_th_info[tctx->th_num].num_cache_threads_waiting != c_th_info[tctx->th_num].cache_threads_created) {
        usleep(10000);
    }
    return rc;
}

/****************************************************************************/
/* Function to write to disk and at the same time write from the CPU to     */
/* memory. To check cache processing.                                       */
/****************************************************************************/
int write_cache_disk(struct htx_data *htx_ds, struct thread_context *tctx)
{
    int th_index = 0;
    int rc = 0;
    char msg[128];
    struct cache_thread *c_th = c_th_info[tctx->th_num].cache_th;

    pthread_mutex_init(&(c_th_info[tctx->th_num].cache_mutex), DEFAULT_MUTEX_ATTR_PTR);
    pthread_cond_init(&(c_th_info[tctx->th_num].do_oper_cond), DEFAULT_COND_ATTR_PTR);

    c_th_info[tctx->th_num].cache_threads_created = 0;

    while (th_index < tctx->num_cache_threads - 1) {
        c_th_info[tctx->th_num].cache_threads_created++;
        c_th[th_index].th_num = th_index;
        c_th[th_index].parent_th_num = tctx->th_num;
        memcpy(&(c_th[th_index].cache_htx_ds), htx_ds, sizeof(struct htx_data));

        if (strcasecmp(tctx->oper,"CAWW") == 0) {
            rc = pthread_create(&(cache_threads[th_index]), &thread_attrs_detached, (void *(*)(void *))write_mem,
                                (void *)(&c_th[th_index]));
        } else {
            rc = pthread_create(&(cache_threads[th_index]), &thread_attrs_detached, (void *(*)(void *))read_mem,
                                (void *)(&c_th[th_index]));
        }
        if (rc != 0) {
            sprintf(msg, "pthread_create failed for creating cache threads in write_cache_disk()."
                         " errno is: %d\n", rc);
            user_msg(htx_ds, rc, 0, HARD, msg);
            exit(1);
		}
		th_index++;
    }
    while (c_th_info[tctx->th_num].num_cache_threads_waiting != c_th_info[tctx->th_num].cache_threads_created) {
        usleep(10000);
    }

    return rc;
}

#ifdef __HTX_LINUX__
/*******************************************************************/
/*   Function to do SYNC_CACHE ioctl if write cache is enabled     */
/*******************************************************************/
int sync_cache_operation(struct htx_data *htx_ds, int fd)
{
    unsigned char cdb[10] = {SYNCHRONIZE_CACHE, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    struct sg_io_hdr io_hdr;
    int rc = 0;
    char msg[128];

    memset(&io_hdr, 0, sizeof(struct sg_io_hdr));
    io_hdr.interface_id = 'S';
    io_hdr.dxfer_direction = SG_DXFER_NONE;
    io_hdr.cmdp = cdb;
    io_hdr.cmd_len = sizeof(cdb);

    /* printf("sync_cache cdb: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n",
                     cdb[0], cdb[1], cdb[2], cdb[3], cdb[4],
                     cdb[5], cdb[6], cdb[7], cdb[8], cdb[9]);
    */
    rc = ioctl(fd, SG_IO, &io_hdr);
    if (rc) {
        sprintf(msg, "SG_IO ioctl for SYNC_CACHE failed. errno: %d\n", errno);
        hxfmsg(htx_ds, errno, HARD, msg);
        return -1;
    }
    return rc;
}

int sync_cache(struct htx_data *htx_ds, struct thread_context *tctx, int loop)
{
    int rc = 0;

    rc = sync_cache_operation(htx_ds, tctx->fd);
    return rc;
}
#endif

/*******************************************************************/
/****   Execute a system command from a psuedo command line     ****/
/*******************************************************************/
int run_cmd(struct htx_data *htx_ds, char *cmd_line)
{
    char msg[256], cmd[256], msg1[512];
    int a, b, c, d, rc;
    char filename[64], log_dir[32];
    int filedes;

    sprintf(log_dir, "%s/", htx_ds->htx_exer_log_dir);

    strcpy(filename, log_dir);
    strcat(filename, "cmdout.");

    if (strncmp (htx_ds->sdev_id, "/dev/rlv", 8) == 0) {
        sprintf(msg, "Command %s\nCANNOT be run against a logical volume!", cmd_line);
        user_msg(htx_ds, 0, 0, INFO, msg);
        return (0);
    }

    b = 0;
    for (a = 0; a <= strlen(cmd_line); a++ ) {
        cmd[b] = cmd_line[a];
        if (cmd[b] == '$') {
            a++;
            if (cmd_line[a] == 'd' || cmd_line[a] == 'D' || cmd_line[a] == 'p' ||
                cmd_line[a] == 'P' || cmd_line[a] == 'o') {
                switch(cmd_line[a]) {
                case 'd' :
                    for (c = 6; c < strlen(htx_ds->sdev_id); c++ ) {
                        cmd[b] = htx_ds->sdev_id[c];
                        b++;
                    }
                    break;

                case 'D':
                    for (c = 5; c < strlen(htx_ds->sdev_id); c++ ) {
                        cmd[b] = htx_ds->sdev_id[c];
                        b++;
                    }
                    break;

                case 'p':
                    for (c = 0; c < strlen(htx_ds->sdev_id); c++ ) {
                        cmd[b] = htx_ds->sdev_id[c];
                        if (cmd[b] == 'r' && cmd[b-1] == '/' ) ;
                        else {
                            b++;
                        }
                    }
                    break;

                case 'P':
                    for (c = 0; c < strlen(htx_ds->sdev_id); c++) {
                        cmd[b] = htx_ds->sdev_id[c];
                        b++;
                    }
                    break;

                default:
                    for (c = 0; c < strlen(filename); c++) {
                        cmd[b] = filename[c];
                        b++;
                    }
                    d = strlen(filename);
                    for (c = 6; c < strlen(htx_ds->sdev_id); c++) {
                        cmd[b] = htx_ds->sdev_id[c];
                        filename[d] = htx_ds->sdev_id[c];
                        b++;
                        d++;
                    }
                    break;
               }
            } else {
                b++;
                cmd[b] = cmd_line[a];
                b++;
            }
        } else {
            b++;
        }
    }
    if (htx_ds->run_type[0] == 'O' || dev_info.debug_flag == 1) {
        DPRINT("Command to be Executed > \n %s\n", cmd);
    }

    if ((rc = system(cmd)) != 0) {
        if ((filedes = open(filename, O_RDONLY)) == -1) {
            sprintf(msg, "Command FAILED rc = %d > \n No Error Information " "returned from command:\n %s\n",
                    rc, cmd);
        } else {
            sprintf(msg, "COMMAND: %s FAILED\n with the Following Error " "Information:\n", cmd);
            rc = read(filedes, msg1, 512);
            strcat(msg, msg1);
            close(filedes);
        }
        user_msg(htx_ds, -1, 0, HARD, msg);
    }
    return 0;
}

