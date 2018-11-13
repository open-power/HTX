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

#include "hxestorage_utils.h"

/* Macros for miscompare type */
#define HXESTORAGE_NO_MISCOMPARE        -1
#define HXESTORAGE_PRE_INIT_DATA        1
#define HXESTORAGE_ALL_ZEROS_DATA       2
#define HXESTORAGE_STALE_DATA           3
#define HXESTORAGE_OLD_DATA             4
#define HXESTORAGE_JUNK_DATA            5
#define HXESTORAGE_DIFF_HOST_DATA       6
#define HXESTORAGE_DIFF_DEV_DATA        7
#define HXESTORAGE_DIFF_LBA_DATA        8
#define HXESTORAGE_DIFF_STANZA_DATA     9
#define HXESTORAGE_DIFF_PATTERN_DATA    10
#define HXESTORAGE_WRITE_STAMP_MISMATCH       11

#define UPDATE_DONE                     0
#define PARTIAL_BLOCK_MISCOMPARE        1
#define MIS_TYPE_MISMATCH               2

void get_string(char *wbuf_ptr, char *wbuf, char *rbuf_ptr, char *rbuf);

/* Function: generate_miscompare_report
   Purpose:  To generate the miscompare report
*/

void generate_miscompare_report (struct thread_context *tctx, int mis_offset, char *msg)
{
    int i, th_num, offset;
    int loop_count;
    char temp[256], msg1[512];
    char buf[16], stanza[10], *ptr;
    char *wbuf_ptr, *rbuf_ptr, wbuf[18], rbuf[18];
    unsigned int *wptr, *rptr;

    sprintf(msg, "Testcase Summary:\n=================\n");
    sprintf(temp, "Device name: %s\n", dev_info.dev_name);
    strcat(msg, temp);
    sprintf(temp, "Total blocks: 0x%llx, Block size: 0x%x\n", dev_info.maxblk, dev_info.blksize);
    strcat(msg, temp);
    sprintf(temp, "Rule file name: %s\n", dev_info.rules_file_name);
    strcat(msg, temp);
    sprintf(temp, "Number of Rulefile passes (cycle) completed: %d\n", read_rules_file_count - 1);
    strcat(msg, temp);
    strcpy(buf, tctx->id);
    ptr = strrchr(buf, '_');
    strncpy(stanza, tctx->id, (ptr - buf));
    stanza[ptr - buf] = '\0';
    th_num = atoi(ptr+1);
    sprintf(temp, "Stanza running: %s, Thread no.: %d\n", stanza, th_num);
    strcat(msg, temp);
    sprintf(temp, "Oper performed: %s, Current seek type: ", tctx->oper);
    if (tctx->cur_seek_type == RANDOM) {
        strcat(temp, "RANDOM\n");
    } else {
        strcat(temp, "SEQ\n");
    }
    strcat(msg, temp);
    sprintf(temp, "LBA no. where IO started: 0x%llx\n", tctx->blkno[0]);
    strcat(msg, temp);
    sprintf(temp, "Transfer size: 0x%llx\n", tctx->dlen);
    strcat(msg, temp);
    if (enable_state_table) {
        sprintf(temp, "State table feature is enabled.\n");
        strcat(msg, temp);
    }
    strcpy(tctx->mis_log_buf, msg);

    if (total_BWRC_threads != 0) {
        sprintf(temp, "num_BWRC_threads running: %d\n", total_BWRC_threads);
        strcat(tctx->mis_log_buf, temp);
        sprintf(temp, "LBA Fencepost detail:\n");
        strcat(tctx->mis_log_buf, temp);
        sprintf(temp, "th_num             min_lba                max_lba      status\n");
        strcat(tctx->mis_log_buf, temp);
        for (i = 0; i < total_BWRC_threads; i++) {
            sprintf(temp, "%6d   %16llx  %16llx  %c\n", i, lba_fencepost[i].min_lba, lba_fencepost[i].max_lba, lba_fencepost[i].status);
            strcat(tctx->mis_log_buf, temp);
        }
    }

    strcpy(msg1, "\nMiscompare Summary:\n===================\n");
    sprintf(temp, "LBA no. where miscompare started:     0x%llx\n", tctx->mis_detail.mis_start_LBA);
    strcat(msg1, temp);
    sprintf(temp, "LBA no. where miscompare ended:       0x%llx\n", tctx->mis_detail.mis_end_LBA);
    strcat(msg1, temp);
    sprintf(temp, "Miscompare start offset (in bytes):   0x%x\n", tctx->mis_detail.mis_start);
    strcat(msg1, temp);
    sprintf(temp, "Miscompare end offset (in bytes):     0x%x\n", tctx->mis_detail.mis_end);
    strcat(msg1, temp);
    sprintf(temp, "Miscompare size (in bytes):           0x%x\n", tctx->mis_detail.mis_size);
    strcat(msg1, temp);

    strcat(msg, msg1);
    strcat(tctx->mis_log_buf, msg1);

    strcat(msg, "\nExpected data (at miscompare offset): ");
    for (i = 0; i < 16; i++) {
        sprintf(msg1, "%.2x", tctx->wbuf[mis_offset + i]);
        strcat(msg, msg1);
    }
    strcat(msg, "\n");

    strcat(msg, "Actual data (at miscompare offset):   ");
    for (i = 0; i < 16; i++) {
        sprintf(msg1, "%.2x", tctx->rbuf[mis_offset + i]);
        strcat(msg, msg1 );
    }
    strcat(msg, "\n");

    switch(tctx->mis_detail.mis_type) {
        case HXESTORAGE_PRE_INIT_DATA:
            sprintf(temp, "\nRead buffer contains hxestorage pre-initailized data i.e. 0xbb."
                          "\n---------------------------------------------------------------\n");
            break;

        case HXESTORAGE_ALL_ZEROS_DATA:
            sprintf(temp, "\nRead buffer contains all 0s."
                          "\n----------------------------\n");
            break;

        case HXESTORAGE_DIFF_LBA_DATA:
            sprintf(temp, "\nData was expected from LBA no. 0x%llx, while it actually came from 0x%llx."
                          "\n--------------------------------------------------------------------------\n", tctx->mis_detail.mis_start_LBA, tctx->mis_detail.diff_LBA);
            break;

        case HXESTORAGE_DIFF_HOST_DATA:
            sprintf(temp, "\nActual data is coming from a different host(%s). This could be a setup issue."
                          "\n-----------------------------------------------------------------------------\n", tctx->mis_detail.diff_host);
            break;

        case HXESTORAGE_DIFF_DEV_DATA:
            sprintf(temp, "\nActual data is coming from a different device(%s). This could be a setup issue."
                          "\n-------------------------------------------------------------------------------\n", tctx->mis_detail.diff_dev);
            break;

        case HXESTORAGE_STALE_DATA:
            sprintf(temp, "\nData in read buffer is stale data i.e. was written even before the current hxestorage run."
                          "\nCurrent hxestorage test start time: 0x%x."
                          "\n------------------------------------------------------------------------------------------\n", (unsigned int)time_mark);
            break;

        case HXESTORAGE_OLD_DATA:
            sprintf(temp, "\nData obtained is from some previous write. Looks like Write for current stanza did not go through."
                          "\n--------------------------------------------------------------------------------------------------\n");
            break;

        case HXESTORAGE_DIFF_STANZA_DATA:
            sprintf(temp, "\nData obtained is from some other stanza. Looks like write did not go through."
                          "\n-----------------------------------------------------------------------------\n");
            break;

        case HXESTORAGE_DIFF_PATTERN_DATA:
            sprintf(temp, "\nExpected data was of pattern type %s, while actual data is of some different pattern type."
                          "\n------------------------------------------------------------------------------------------\n", tctx->pattern_id);
            break;

        case HXESTORAGE_WRITE_STAMP_MISMATCH:
            sprintf(temp, "\nWrite stamping mismatch for LBA: 0x%llx.\n"
                          "\n--------------------------------------------\n", tctx->mis_detail.mis_start_LBA);
            break;

        case HXESTORAGE_JUNK_DATA:
        default:
            sprintf(temp, "\nRead buffer is having junk data."
                          "\n--------------------------------\n");
            break;
    }

    strcat(msg, temp);
    strcat(tctx->mis_log_buf, temp);

    sprintf(temp, "\nExpected data buffer start addr: 0x%llx\n", (unsigned long long)tctx->wbuf);
    strcat(tctx->mis_log_buf, temp);
    sprintf(temp, "Actual data buffer start addr  : 0x%llx\n", (unsigned long long)tctx->rbuf);
    strcat(tctx->mis_log_buf, temp);

    wptr = (unsigned int *) (tctx->wbuf + (tctx->mis_detail.mis_start / dev_info.blksize) * dev_info.blksize);
    rptr = (unsigned int *) (tctx->rbuf + (tctx->mis_detail.mis_start / dev_info.blksize) * dev_info.blksize);

    sprintf(temp, "\nOffset\t  Expected data where miscompare started\t\t\t\t\t    Actual data where miscompare started\n");
    strcat(tctx->mis_log_buf, temp);
    sprintf(temp, "======\t  ========================================\t\t\t\t\t    ======================================\n");
    strcat(tctx->mis_log_buf, temp);

    /* If miscompare did not start at beginning of buffer, print the block previous to
     * miscompared block also.
     */
    if (tctx->mis_detail.mis_start / dev_info.blksize == 0) {
        wptr = (unsigned int *) (tctx->wbuf + (tctx->mis_detail.mis_start / dev_info.blksize) * dev_info.blksize);
        rptr = (unsigned int *) (tctx->rbuf + (tctx->mis_detail.mis_start / dev_info.blksize) * dev_info.blksize);
        loop_count = dev_info.blksize;
    } else {
        wptr = (unsigned int *) ((tctx->wbuf + (tctx->mis_detail.mis_start / dev_info.blksize) * dev_info.blksize) - dev_info.blksize);
        rptr = (unsigned int *) ((tctx->rbuf + (tctx->mis_detail.mis_start / dev_info.blksize) * dev_info.blksize) - dev_info.blksize);
        loop_count = 2 * dev_info.blksize;
    }
    offset = (int)((unsigned long long)wptr - (unsigned long long)tctx->wbuf);
    for (i = 0; i < loop_count / 4; ) {
        wbuf_ptr = (char *)(wptr + i);
        rbuf_ptr = (char *)(rptr + i);
        get_string(wbuf_ptr, wbuf, rbuf_ptr, rbuf);
        sprintf(temp, "0x%-6x: %08x %08x %08x %08x  %s\t\t\t%08x %08x %08x %08x  %s",
                     offset, wptr[i], wptr[i + 1], wptr[i + 2], wptr[i + 3], wbuf,
                     rptr[i], rptr[i + 1], rptr[i + 2], rptr[i + 3], rbuf);
        if ((tctx->mis_detail.mis_start >= offset) && (tctx->mis_detail.mis_start < offset + 16)) {
            sprintf(msg1, "%s        <<<<<\n", temp); /* Points to the miscompare start offset */
        } else {
            sprintf(msg1, "%s\n", temp);
        }
        if (strlen(tctx->mis_log_buf) + strlen(msg1) >= MIS_LOG_SIZE) {
            strcat(tctx->mis_log_buf, "Size limit exceeded\n");
            break;
        } else {
            strcat(tctx->mis_log_buf, msg1);
        }
        i += 4;
        if ((4 * i) % dev_info.blksize == 0) {
            strcat(tctx->mis_log_buf, "\n");
        }
        offset += 16;
    }

    sprintf(temp, "\nOffset\t  Expected data where miscompare ended\t\t\t\t\t\t    Actual data where miscompare ended\n");
    strcat(tctx->mis_log_buf, temp);
    sprintf(temp, "======\t  ======================================\t\t\t\t\t    =====================================\n");
    strcat(tctx->mis_log_buf, temp);
    wptr = (unsigned int *) (tctx->wbuf + (tctx->mis_detail.mis_end / dev_info.blksize) * dev_info.blksize);
    rptr = (unsigned int *) (tctx->rbuf + (tctx->mis_detail.mis_end / dev_info.blksize) * dev_info.blksize);
    offset = (int)((unsigned long long)wptr - (unsigned long long)tctx->wbuf);
    /* print block where miscomapre neded and the block next to it. */
    for (i = 0; i < (2 * dev_info.blksize) / 4; ) {
        wbuf_ptr = (char *)(wptr + i);
        rbuf_ptr = (char *)(rptr + i);
        if (wbuf_ptr == tctx->wbuf + tctx->dlen) {
            break;
        }
        get_string(wbuf_ptr, wbuf, rbuf_ptr, rbuf);
        sprintf(temp, "0x%-6x: %08x %08x %08x %08x  %s\t\t\t%08x %08x %08x %08x  %s",
                      offset, wptr[i], wptr[i + 1], wptr[i + 2], wptr[i + 3], wbuf,
                      rptr[i], rptr[i + 1], rptr[i + 2], rptr[i + 3], rbuf);
        if ((tctx->mis_detail.mis_end >= offset) && (tctx->mis_detail.mis_end < offset + 16)) {
            sprintf(msg1, "%s        <<<<<\n", temp); /* Points to the miscompare end offset */
        } else {
            sprintf(msg1, "%s\n", temp);
        }
        if (strlen(tctx->mis_log_buf) + strlen(msg1) >= MIS_LOG_SIZE) {
            strcat(tctx->mis_log_buf, "Size limit exceeded\n");
            break;
        } else {
            strcat(tctx->mis_log_buf, msg1);
        }
        i += 4;
        if ((4 * i) % dev_info.blksize == 0) {
            strcat(tctx->mis_log_buf, "\n");
        }
        offset += 16;
    }

}

void get_string(char *wbuf_ptr, char *wbuf, char *rbuf_ptr, char *rbuf)
{
    int j;

    for (j = 0; j < 16; j++) {
        if (isprint(wbuf_ptr[j])) {
            wbuf[j] = wbuf_ptr[j];
        } else {
            wbuf[j] = '.';
        }
        if (isprint(rbuf_ptr[j])) {
            rbuf[j] = rbuf_ptr[j];
        } else {
            rbuf[j] = '.';
        }
    }
    wbuf[16] = '\0';
    rbuf[16] = '\0';
}
unsigned long long get_LBA (struct thread_context *tctx, int mis_blk)
{
    unsigned long long LBA_num;

    if (tctx->pattern_id[0] == '#') {
       LBA_num = * (unsigned long long *) (tctx->wbuf + (mis_blk * dev_info.blksize));
    } else {
       LBA_num = tctx->blkno[0] + mis_blk;
    }
    return (LBA_num);
}

void check_known_data_miscompare(struct thread_context *tctx, int mis_offset, struct miscompare_details *cur_mis, char flag)
{
    int i, j, data, max_loop_count;
    int blk, mis_offset_within_blk;
    unsigned int *ptr;
    char data_offset, *ptr_offset;

    blk = mis_offset / dev_info.blksize;
    mis_offset_within_blk = mis_offset % dev_info.blksize;
    /* Make it multiple of 4 (i.e. size of int) as comparison will be done in
     * terms of 4 bytes.
     */
    mis_offset_within_blk = (mis_offset_within_blk / 4) * 4;

    ptr = (unsigned int *)(tctx->rbuf + blk * dev_info.blksize);
    ptr_offset = (char *) ptr;
    if (flag == HXESTORAGE_PRE_INIT_DATA) {
        data = 0xbbbbbbbb;
        data_offset = 0xbb;
    } else {
        data = 0x0;
        data_offset = 0x0;
    }

    /* comparison is done in terms of 4 bytes at a time */
    for (i = mis_offset_within_blk / 4; i < dev_info.blksize / 4; i++) {
        if (ptr[i] == data) {
            if (cur_mis->mis_type == HXESTORAGE_NO_MISCOMPARE) {
                /* skip updation of mis detail if LBA no. is also having either pre-init
                 * OR zero data. Will be taken care if time-stamp is also miscomparing.
                 */
                if (4 * i < LBA_LEN)  {
                    continue;
                }
                cur_mis->mis_start = blk * dev_info.blksize + 4 * i;
                if (4 * i == LBA_LEN) {
                    max_loop_count = LBA_LEN;
                } else {
                    max_loop_count = 4;
                }
                /* Check if miscompare was hit somewhere in previous 4 OR 8 bytes */
                for (j = 1; j <= max_loop_count; j++) {
                    if (ptr_offset[4 * i - j] == data_offset) {
                        cur_mis->mis_start--;
                    } else {
                        break;
                    }
                }
                cur_mis->mis_type = flag;
            }
        } else {
            if (cur_mis->mis_type != HXESTORAGE_NO_MISCOMPARE) {
                /* Check at which byte offset within these 4 bytes miscompare ended */
                for (j = 0; j < 4; j++) {
                    if (ptr_offset[4 * i + j] != data_offset) {
                        cur_mis->mis_end = blk * dev_info.blksize + 4 * i + j;
                        break;
                    }
                }
            }
            break;
        }
    }

    if ((i == dev_info.blksize / 4) && (cur_mis->mis_type != HXESTORAGE_NO_MISCOMPARE)) {
        cur_mis->mis_end = ((blk + 1) * dev_info.blksize) - 1;
    }
}

void check_junk_data (struct thread_context *tctx, int mis_blk, struct miscompare_details *cur_mis)
{
    unsigned int *wbuf_ptr, *rbuf_ptr;
    char *wbuf, *rbuf;
    int i, j;

    if (strncmp(tctx->rbuf + (mis_blk * dev_info.blksize) + BUFSIG_POS, BUFSIG, BUFSIG_LEN) != 0) {
        cur_mis->mis_start = mis_blk * dev_info.blksize;
        cur_mis->mis_type = HXESTORAGE_JUNK_DATA;
        /* Check if whole block is having JUNK data.
         * 4 byte of comparison will be done at a time.
         */
        wbuf_ptr = (unsigned int *) (tctx->wbuf + mis_blk * dev_info.blksize);
        rbuf_ptr = (unsigned int *) (tctx->rbuf + mis_blk * dev_info.blksize);
        for (i = HEADER_SIZE / 4; i < dev_info.blksize / 4; i++) {
            if (wbuf_ptr[i] == rbuf_ptr[i]) { /* No miscomapre in these 4 bytes */
                /* Check where in previous 4 bytes miscompare ended */
                wbuf = (char *) wbuf_ptr;
                rbuf = (char *) rbuf_ptr;
                for (j = 1; j <= 4; j++) {
                    if (wbuf[4 * i - j] != rbuf[4 * i - j]) {
                        cur_mis->mis_end = mis_blk * dev_info.blksize + i  * 4 - j;
                        break;
                    }
                }
                break;
            }
        }
        if (i == dev_info.blksize / 4) {
            cur_mis->mis_end = ((mis_blk + 1) * dev_info.blksize) - 1;
        }
    }
}

void check_diff_host_data (struct thread_context *tctx, int blk_num, struct miscompare_details *cur_mis)
{
    if (strncmp(tctx->wbuf + (blk_num * dev_info.blksize) + HOSTNAME_POS, tctx->rbuf + (blk_num * dev_info.blksize) + HOSTNAME_POS, HOSTNAME_LEN) != 0) {
        cur_mis->mis_start = blk_num * dev_info.blksize;
        cur_mis->mis_end = (blk_num + 1) * dev_info.blksize - 1;
        cur_mis->mis_type = HXESTORAGE_DIFF_HOST_DATA;
        strncpy(cur_mis->diff_host, tctx->rbuf + (blk_num * dev_info.blksize) + HOSTNAME_POS, HOSTNAME_LEN);
    }
}

void check_diff_dev_data (struct thread_context *tctx, int blk_num, struct miscompare_details *cur_mis)
{
    if (strncmp(tctx->wbuf + (blk_num * dev_info.blksize) + DEV_NAME_POS,  tctx->rbuf + (blk_num * dev_info.blksize) + DEV_NAME_POS, DEV_NAME_LEN) != 0) {
        cur_mis->mis_start = blk_num * dev_info.blksize;
        cur_mis->mis_end = (blk_num + 1) * dev_info.blksize - 1;
        cur_mis->mis_type = HXESTORAGE_DIFF_DEV_DATA;
        strncpy(cur_mis->diff_dev, tctx->rbuf + (blk_num * dev_info.blksize) + DEV_NAME_POS, DEV_NAME_LEN);
    }
}

void check_diff_lba_data (struct thread_context *tctx, int blk_num, struct miscompare_details *cur_mis)
{
    unsigned long long wbuf_LBA, rbuf_LBA;

    /* get LBA number in wbuf */
    wbuf_LBA = *(unsigned long long *) (tctx->wbuf + (blk_num * dev_info.blksize));

    /* get LBA number in rbuf */
    rbuf_LBA = *(unsigned long long *) (tctx->rbuf + (blk_num * dev_info.blksize));

    if (wbuf_LBA != rbuf_LBA) {
        cur_mis->mis_start = blk_num * dev_info.blksize;
        cur_mis->mis_end = (blk_num + 1) * dev_info.blksize - 1;
        cur_mis->mis_type = HXESTORAGE_DIFF_LBA_DATA;
        cur_mis->diff_LBA = rbuf_LBA;
    }
}

void check_write_stamp (struct thread_context *tctx, int blk_num, struct miscompare_details *cur_mis)
{
    char *rbuf, *wbuf;
    unsigned short rbuf_write_status, wbuf_write_status;

    rbuf = tctx->rbuf + blk_num * dev_info.blksize;
    wbuf = tctx->wbuf + blk_num * dev_info.blksize;

#ifndef __HTX_LE__
    rbuf_write_status = rbuf[WRITE_STAMP_POS] << 8  | rbuf[WRITE_STAMP_POS + 1];
    wbuf_write_status = wbuf[WRITE_STAMP_POS] << 8  | wbuf[WRITE_STAMP_POS + 1];
#else
    rbuf_write_status = rbuf[WRITE_STAMP_POS + 1] << 8  | rbuf[WRITE_STAMP_POS];
    wbuf_write_status = wbuf[WRITE_STAMP_POS + 1] << 8  | wbuf[WRITE_STAMP_POS];
#endif
    if (wbuf_write_status != rbuf_write_status) {
        cur_mis->mis_start = blk_num * dev_info.blksize + WRITE_STAMP_POS;
        cur_mis->mis_end = (blk_num + 1) * dev_info.blksize - 1;
        cur_mis->mis_type = HXESTORAGE_WRITE_STAMP_MISMATCH;
    }
}

void check_stale_data (struct thread_context *tctx, int blk_num, struct miscompare_details *cur_mis)
{
    char *rbuf, *wbuf, buf[10];
    time_t rbuf_time_stamp, wbuf_time_stamp;

    rbuf = tctx->rbuf + blk_num * dev_info.blksize;
    wbuf = tctx->wbuf + blk_num * dev_info.blksize;
    #ifndef __HTX_LE__
        rbuf_time_stamp = rbuf[TIME_STAMP_POS] << 24 | rbuf[TIME_STAMP_POS + 1] << 16 | rbuf[TIME_STAMP_POS + 2] << 8 | rbuf[TIME_STAMP_POS + 3];
        wbuf_time_stamp = wbuf[TIME_STAMP_POS] << 24 | wbuf[TIME_STAMP_POS + 1] << 16 | wbuf[TIME_STAMP_POS + 2] << 8 | wbuf[TIME_STAMP_POS + 3];
    #else
        rbuf_time_stamp = rbuf[TIME_STAMP_POS + 3] << 24 | rbuf[TIME_STAMP_POS + 2 ] << 16 | rbuf[TIME_STAMP_POS + 1] << 8 | rbuf[TIME_STAMP_POS];
        wbuf_time_stamp = wbuf[TIME_STAMP_POS + 3] << 24 | wbuf[TIME_STAMP_POS + 2 ] << 16 | wbuf[TIME_STAMP_POS + 1] << 8 | wbuf[TIME_STAMP_POS];
    #endif
    if (rbuf_time_stamp < time_mark || ((strchr (tctx->oper, 'W') != 0 || strchr (tctx->oper, 'w') != 0) && rbuf_time_stamp < wbuf_time_stamp)) {
        cur_mis->mis_start = blk_num * dev_info.blksize + TIME_STAMP_POS;
        cur_mis->mis_end = (blk_num + 1) * dev_info.blksize - 1;
        strcpy(buf, tctx->oper);
        if (strtok (buf, "Ww") != 0 && rbuf_time_stamp < wbuf_time_stamp) {
            cur_mis->mis_type = HXESTORAGE_OLD_DATA;
        }
        if (rbuf_time_stamp < time_mark) {
            cur_mis->mis_type = HXESTORAGE_STALE_DATA;
        }
    }
}

void check_diff_stanza_id (struct thread_context *tctx, int blk_num, struct miscompare_details *cur_mis)
{
    if (strncmp(tctx->wbuf + (blk_num * dev_info.blksize) + STANZA_ID_POS,  tctx->rbuf + (blk_num * dev_info.blksize) + STANZA_ID_POS, STANZA_ID_LEN) != 0) {
        cur_mis->mis_start = blk_num * dev_info.blksize + STANZA_ID_POS;
        cur_mis->mis_end = (blk_num + 1) * dev_info.blksize - 1;
        cur_mis->mis_type = HXESTORAGE_DIFF_STANZA_DATA;
        strncpy (cur_mis->diff_stanza, tctx->rbuf + STANZA_ID_POS, STANZA_ID_LEN);
    }
}

void validate_header_field (struct thread_context *tctx, int mis_blk, struct miscompare_details *cur_mis)
{
    char buf[8];

    check_junk_data(tctx, mis_blk, cur_mis);

    if (cur_mis->mis_type == HXESTORAGE_NO_MISCOMPARE) {
        /* Check if data is coming from different host */
        check_diff_host_data(tctx, mis_blk, cur_mis);
    }

    if (cur_mis->mis_type == HXESTORAGE_NO_MISCOMPARE) {
        /* Check if data is coming from different device */
        check_diff_dev_data(tctx, mis_blk, cur_mis);
    }

    if (cur_mis->mis_type == HXESTORAGE_NO_MISCOMPARE) {
        /* Check if data is coming from different LBA */
        check_diff_lba_data(tctx, mis_blk, cur_mis);
    }

    if (cur_mis->mis_type == HXESTORAGE_NO_MISCOMPARE && enable_state_table == 1) {
        check_write_stamp(tctx, mis_blk, cur_mis);
    }

    if (cur_mis->mis_type == HXESTORAGE_NO_MISCOMPARE) {
        check_stale_data(tctx, mis_blk, cur_mis);
    }

    if (cur_mis->mis_type == HXESTORAGE_NO_MISCOMPARE) {
        strcpy(buf, tctx->oper);
        if (strtok(buf, "wW") != NULL) {
            check_diff_stanza_id(tctx, mis_blk, cur_mis);
        }
    }
}

int update_mis_detail (struct thread_context *tctx, struct miscompare_details cur_mis)
{
    if (tctx->mis_detail.mis_type == HXESTORAGE_NO_MISCOMPARE) {
        tctx->mis_detail.mis_start = cur_mis.mis_start;
        tctx->mis_detail.mis_type = cur_mis.mis_type;
        switch (tctx->mis_detail.mis_type) {
            case HXESTORAGE_DIFF_HOST_DATA:
                strcpy(tctx->mis_detail.diff_host, cur_mis.diff_host);
                break;

            case HXESTORAGE_DIFF_DEV_DATA:
                strcpy(tctx->mis_detail.diff_dev, cur_mis.diff_dev);
                break;

            case HXESTORAGE_DIFF_LBA_DATA:
                tctx->mis_detail.diff_LBA = cur_mis.diff_LBA;
                break;

            default:
                break;
        }
    }
    if (tctx->mis_detail.mis_type == HXESTORAGE_NO_MISCOMPARE) {
        return HXESTORAGE_NO_MISCOMPARE;
    } else if (tctx->mis_detail.mis_type != cur_mis.mis_type) {
        return MIS_TYPE_MISMATCH;
    } else {
        tctx->mis_detail.mis_end = cur_mis.mis_end;
        if ((tctx->mis_detail.mis_end + 1) % dev_info.blksize != 0) { /* Means miscompare ended within the block */
            return PARTIAL_BLOCK_MISCOMPARE;
        } else {
            return UPDATE_DONE;
        }
    }
}

void analyze_miscompare(struct thread_context *tctx, int mis_offset, char *msg)
{
    int mis_blk, offset, mis_offset_within_blk;
    int blk, mis_end, rc, i, j;
    int save_mis_offset = mis_offset;
    struct miscompare_details cur_mis;
    unsigned int *wbuf_ptr, *rbuf_ptr;
    char *wbuf, *rbuf;

    if (strcmp(tctx->id, "Re-read") == 0) {
        return;
    }
    mis_blk = mis_offset / dev_info.blksize;
    mis_offset_within_blk = mis_offset % dev_info.blksize;
    /* printf("mis. LBA: %llx\n", get_LBA(tctx, mis_blk)); */
    tctx->mis_detail.mis_start_LBA = get_LBA(tctx, mis_blk);
    tctx->mis_detail.mis_type = HXESTORAGE_NO_MISCOMPARE;

    for (blk = mis_blk; blk < tctx->num_blks; blk++) {
        cur_mis.mis_type = HXESTORAGE_NO_MISCOMPARE;
        /* re-calculating mis_offset as for first iteration it will be
         * where we detected miscompare and afterwards, it will be from beginning
         * of the block.
         */
        mis_offset = (blk * dev_info.blksize) + mis_offset_within_blk;
        offset = 0;

        /* First fo all check we have have pre-initialized data i.e. 0xbb
         * in the read buffer.
         */
        check_known_data_miscompare(tctx, mis_offset, &cur_mis, HXESTORAGE_PRE_INIT_DATA);

        /* If no miscompare, check for all zero data */
        if (cur_mis.mis_type == HXESTORAGE_NO_MISCOMPARE) {
            check_known_data_miscompare(tctx, mis_offset, &cur_mis, HXESTORAGE_ALL_ZEROS_DATA);
        }

        /* Check the header fields */
        if (cur_mis.mis_type == HXESTORAGE_NO_MISCOMPARE && tctx->pattern_id[0] == '#' && mis_offset_within_blk < HEADER_SIZE) {
            validate_header_field(tctx, blk, &cur_mis);
            offset = HEADER_SIZE;
        }

        /* Now check the data section for Junk Data.
         * Comparison will be done with 4 byte at a time.
         */
        if (cur_mis.mis_type == HXESTORAGE_NO_MISCOMPARE) {
            /* In here means HEADER is matching. */
            wbuf_ptr = (unsigned int *) (tctx->wbuf + blk * dev_info.blksize);
            rbuf_ptr = (unsigned int *) (tctx->rbuf + blk * dev_info.blksize);
            /* For comparison at byte level within int size, use wbuf and rbuf */
            wbuf = (char *) wbuf_ptr;
            rbuf = (char *) rbuf_ptr;
            mis_end = 0;
            for (i = offset / 4; i < dev_info.blksize / 4; i++) {
                if (wbuf_ptr[i] != rbuf_ptr[i]) {
                    if (cur_mis.mis_type == HXESTORAGE_NO_MISCOMPARE) {
                        /* check which byte within these 4 bytes started miscompare */
                        for (j = 0; j < 4; j++) {
                            if (wbuf[4 * i + j] != rbuf[4 * i + j]) {
                                cur_mis.mis_start = blk * dev_info.blksize + 4 * i + j;
                                break;
                            }
                        }
                        /* Check if it is a RESTORE_STANZA and pattern_id is of type "#", means
                         * it is not a JUNK data, But actually data is of different pattern type.
                         */
                        if (tctx->pattern_id[0] == '#' && tctx->rule_option == RESTORE_SEEDS_FLAG) {
                             cur_mis.mis_type = HXESTORAGE_DIFF_PATTERN_DATA;
                        } else {
                            cur_mis.mis_type = HXESTORAGE_JUNK_DATA;
                        }
                    }
                } else {
                    if (cur_mis.mis_type == HXESTORAGE_JUNK_DATA) {
                        /* Check where in previous 4 bytes miscompare ended */
                        for (j = 1; j <= 4; j++) {
                            if (wbuf[4 * i - j] != rbuf[4 * i - j]) {
                                cur_mis.mis_end = blk * dev_info.blksize + i  * 4 - j;
                                mis_end = 1;
                                break;
                            }
                        }
                    }
                }
                if (mis_end) {
                    break;
                }
            }
            if (i == dev_info.blksize / 4 && cur_mis.mis_type != HXESTORAGE_NO_MISCOMPARE) {
                cur_mis.mis_end = (blk + 1) * dev_info.blksize -  1;
            }
        }
        rc = update_mis_detail(tctx, cur_mis);
        if (rc == UPDATE_DONE) {
            mis_offset_within_blk = 0;
        } else {
            break;
        }
    }
    tctx->mis_detail.mis_end_LBA = get_LBA(tctx, tctx->mis_detail.mis_end / dev_info.blksize);
    tctx->mis_detail.mis_size = tctx->mis_detail.mis_end - tctx->mis_detail.mis_start + 1;

    generate_miscompare_report(tctx, save_mis_offset, msg);
}

