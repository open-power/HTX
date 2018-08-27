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

#include <stdio.h>
#include <signal.h>
#include <hxihtx64.h>
#include "hxeocapi.h"

#define AFU_NAME "/dev/ocxl/IBM,MEMCPY3.0004:00:00.1.0"

extern int read_rf(struct htx_data *htx_ds, char *rf_name);

pthread_mutex_t create_thread_mutex;
pthread_cond_t  create_thread_cond, start_thread_cond;
pthread_attr_t worker_thread_attrs;

struct htx_data data;

int num_rules_defined = 0, exit_flag = 0;
struct thread_context th_ctx[MAX_THREADS];

char device_name[128], verbose = 'N';

int main (int argc, char * argv[])
{

    char rule_filepath[256], msg[MSG_TEXT_SIZE];
    struct sigaction sigvector;
    int i, num_threads, thread, rule_no, rc = 0;
    struct thread_context *current_tctx;

    sigemptyset(&(sigvector.sa_mask));
    sigvector.sa_flags = 0 ;
    sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
    sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

    /*
     * Copy Command line argument passed in out internal data structure
     */
     memset (&data, 0x00, sizeof(struct htx_data));

     if(argc < 4) {
        printf("Usage /usr/lpp/htx/bin/hxeocapi <dev_name> <REG/OTH> <rules_file> \n");
        return(0);
    }

    strcpy(data.HE_name, argv[0]);
    strcpy(data.sdev_id, argv[1]);
    strcpy(data.run_type,argv[2]);
    strcpy(rule_filepath, argv[3]);

    hxfupdate(START, &data);
    sprintf(msg, "%s %s %s %s \n", data.HE_name, data.sdev_id, data.run_type, rule_filepath);
    hxfmsg (&data, 0, HTX_HE_INFO, msg);

    pthread_attr_init(&worker_thread_attrs);
    pthread_attr_setdetachstate(&worker_thread_attrs, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setscope(&worker_thread_attrs, PTHREAD_SCOPE_PROCESS);
    pthread_mutex_init(&create_thread_mutex, NULL);
    pthread_cond_init(&create_thread_cond, NULL);
    pthread_cond_init(&start_thread_cond, NULL);

    rc = read_rf(&data, rule_filepath);
    if(rc == -1) {
        sprintf (msg, "Rule file parsing failed. rc = %d\n", rc);
        hxfmsg (&data, rc, HTX_HE_HARD_ERROR, msg);
        return(rc);
    }

    if (global_setup() != 0) {
        exit(1);
    }

    /*
     * LOOP infinitely if exer started with REG cmd line argument
     */
    do {
        for(rule_no = 0; rule_no < num_rules_defined; rule_no++) {

            num_threads = rule_data[rule_no].num_threads;

            /*
             * Update Stanza count for SUP
             */
            data.test_id = rule_no + 1;
            hxfupdate(UPDATE, &data);

            rc = pthread_mutex_lock(&create_thread_mutex);
            if (rc) {
                sprintf(msg, "Mutex lock for create_thread_mutex failed in MAIN, rc = %d\n", rc);
                hxfmsg(&data, rc, HTX_HE_HARD_ERROR, msg);
                exit(1);
            }
            for (thread = 0; thread < num_threads; thread++) {
                current_tctx = &th_ctx[thread];
                /*
                 * Populate this thread test case parameters from rules file
                 */
                current_tctx->th_num = thread;
                sprintf(current_tctx->id, "%s_%d", rule_data[rule_no].rule_id, thread);
                current_tctx->num_oper = rule_data[rule_no].num_oper;
                current_tctx->command = rule_data[rule_no].testcase;
                current_tctx->bufsize = rule_data[rule_no].bufsize;
                current_tctx->compare = rule_data[rule_no].compare;
                current_tctx->completion_timeout = rule_data[rule_no].completion_timeout;

                rc =  pthread_create(&(current_tctx->tid), &worker_thread_attrs, tc_worker_thread, (void *) current_tctx);
                if (rc) {
                    sprintf(msg, "rc %d, errno %d from main(): pthread_create failed for threads no: %d",
                           rc, errno, thread);
                    hxfmsg(&data, rc, HTX_HE_HARD_ERROR, msg);
                    break;
                }
                rc = pthread_cond_wait(&create_thread_cond, &create_thread_mutex);
                if (rc) {
                    sprintf(msg, "Cond wait failed in MAIN for thread no. %d, rc = %d\n", rc, thread);
                    hxfmsg(&data, rc, HTX_HE_HARD_ERROR, msg);
                    break;
                }
            }

            /*
             * check if we are here due to error in creating threads
             */
            if (rc) {
                for (i = 0; i < thread; i++) {
                    sprintf (msg, " Cancelling thread = %d, id = %lu coz of error \n", i, th_ctx[i].tid);
                    hxfmsg (&data, 0, HTX_HE_SOFT_ERROR, msg);
                    pthread_cancel (th_ctx[i].tid);
                }
                return(rc);
            } else {
                /**********************************************************************/
                /******      Send broadcast to all threads to start executing   *******/
                /*********************************************************************/
                rc = pthread_cond_broadcast(&start_thread_cond);
                if ( rc ) {
                    sprintf(msg,"pthread_cond_broadcast in MAIN failed for start_thread_cond. rc = %d\n", rc);
                    hxfmsg(&data, rc, HTX_HE_HARD_ERROR, msg);
                    exit(1);
                }

                rc = pthread_mutex_unlock(&create_thread_mutex);
                if (rc) {
                    sprintf (msg, "pthread_mutex_unlock failed with rc = %d, errno =  %d \n", rc, errno);
                    hxfmsg (&data, 0, HTX_HE_HARD_ERROR, msg);
                    return (rc);
                }

                /* Worker thread started, wait for completion */
                for (i = 0; i < thread; i++) {
                    rc = pthread_join (th_ctx[i].tid, (void *)&(th_ctx[i].thread_join_rc));
                    if (rc) {
                        break;
                    }
                }
            }
            if (exit_flag) {
                break;
            }
        } /* End for rule_no for loop */
    hxfupdate(FINISH, &data);
    } while ((strcmp(data.run_type, "REG") == 0) && (exit_flag == 0));

    return 0;
}

int global_setup ()
{
    ocxl_err err = OCXL_OK;
    ocxl_afu_h afu_h;
    char msg[256];
    uint64_t reg, cfg;
    ocxl_mmio_h global;
    pid_t pid;

    pid = getpid();

    if (verbose == 'Y') {
        ocxl_enable_messages(OCXL_ERRORS | OCXL_TRACING);
    } else {
        ocxl_enable_messages(OCXL_ERRORS);
    }

    /*Open AFU device*/
    err =  ocxl_afu_open_from_dev(data.sdev_id, &afu_h);
    if (err != OCXL_OK) {
        sprintf (msg, "Unable to open cxl device %s. rc = %d, errno = %d.\n", data.sdev_id, err, errno);
        hxfmsg (&data, 1, HTX_HE_HARD_ERROR, msg);
        return -1;
    }

    if (verbose == 'Y') {
        ocxl_afu_enable_messages(afu_h, OCXL_ERRORS | OCXL_TRACING);
    } else {
        ocxl_afu_enable_messages(afu_h, OCXL_ERRORS);
    }

    /* Map the full global MMIO area of the AFU */
    err = ocxl_mmio_map (afu_h, OCXL_GLOBAL_MMIO, &global);
    if (err != OCXL_OK ) {
        sprintf (msg, "pid: 0x%x, ocxl_global_mmio_map() failed: %d\n", pid, err);
        hxfmsg (&data, err, HTX_HE_HARD_ERROR, msg);
        goto err_status;
    }

    err = ocxl_mmio_read64 (global, MEMCPY_AFU_GLOBAL_CFG, OCXL_MMIO_LITTLE_ENDIAN, &cfg);
    if (err != OCXL_OK) {
        sprintf (msg, "pid: 0x%x, reading global config register failed \n", pid);
        hxfmsg (&data, err, HTX_HE_HARD_ERROR, msg);
        goto err_status;
    }
    sprintf (msg, "pid: 0x%x, global: %p, AFU config = 0x%lx\n", pid, global, cfg);
    hxfmsg (&data, 0, HTX_HE_INFO, msg);

    reg = 0x8008008000000000ull;
    err = ocxl_mmio_write64 (global, MEMCPY_AFU_GLOBAL_TRACE, OCXL_MMIO_LITTLE_ENDIAN, reg);
    if (err != OCXL_OK) {
        sprintf (msg, "pid: 0x%x, ocxl_mmio_write64(trace) failed: %d\n", pid, err);
        hxfmsg (&data, err, HTX_HE_HARD_ERROR, msg);
        goto err_status;
    }

    hxfmsg (&data, 0, HTX_HE_INFO, "traces reset and rearmed\n");

err_status:
    err = ocxl_afu_close (afu_h);
    if ( err != OCXL_OK) {
        sprintf (msg, "pid: 0x%x, closing the AFU failed with rc: %d\n", pid, err);
        hxfmsg (&data, err, HTX_HE_HARD_ERROR, msg);
        return -1;
    }

    return err;
}

void * tc_worker_thread(void *th_context)
{
    int rc = 0;
    char msg[MSG_TEXT_SIZE];
    struct thread_context *tctx = (struct thread_context *)th_context;
    struct htx_data htx_d;

    memcpy(&htx_d, &data, sizeof(struct htx_data));

    rc = pthread_mutex_lock(&create_thread_mutex);
    if (rc) {
        sprintf(msg, "[th:%s] : pthread_mutex_lock in worker thread failed with rc = %d\n", tctx->id, rc);
        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        exit(rc);
    }

    /*
     * notify main thread to proceed creating other worker threads
     */
    rc = pthread_cond_broadcast(&create_thread_cond);
    if (rc) {
        sprintf(msg, "[th:%s] :  pthread_cond_broadcast failed with rc = %d\n", tctx->id, rc);
        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        exit(rc);
    }
    /*
     * wait for start notification from main thread
     */
    rc = pthread_cond_wait(&start_thread_cond, &create_thread_mutex);
    if (rc) {
        sprintf(msg, "[th: %s] : pthread_cond_wait failed with rc = %d\n", tctx->id, rc);
        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        exit(rc);
    }

    rc = pthread_mutex_unlock(&create_thread_mutex);
    if (rc) {
        sprintf(msg, "[th:%s] : pthread_mutex_unlock failed with rc = %d\n", tctx->id, rc);
        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        exit(rc);
    }

    /*
     * Memcopy test case
     */
    rc = test_afu_memcpy(tctx, &htx_d);
    if(rc) {
       pthread_exit(&rc);
    }

    pthread_exit(NULL);
}

/* Error handler function for library internal error to be
   logged to a specific file.
static void copy_to_err_file(ocxl_err error, const char *message) {
    pthread_mutex_lock(&err_mutex);
    fprintf(err_fd, "%s: %s", ocxl_err_to_string(error), message);
    pthread_mutex_unlock(&err_mutex);

} */

int test_afu_memcpy (struct thread_context *tctx, struct htx_data *htx_d)
{
    char msg[MSG_TEXT_SIZE];
    uint64_t wed, status, afu_irq_handle = 0, err_irq_handle;
    struct memcpy_weq weq;
    struct memcpy_work_element memcpy_we, irq_we;
    struct memcpy_work_element *first_we = NULL, *last_we = NULL;
    struct timeval start, end;
    int t, rc, i, err_no, oper = 0;
    ocxl_mmio_h pp_mmio;
    ocxl_irq_h afu_irq, err_irq;
    ocxl_err err;

    /* Set the error handler to copy_to_err_file function
    ocxl_set_error_message_handler(copy_to_err_file); */

    if (verbose == 'Y') {
        ocxl_enable_messages(OCXL_ERRORS | OCXL_TRACING);
    } else {
        ocxl_enable_messages(OCXL_ERRORS);
    }

    /*Open AFU device*/
    rc =  ocxl_afu_open_from_dev(data.sdev_id, &tctx->afu_h);
    if (rc != OCXL_OK) {
        sprintf (msg, "[th:%s]: Unable to open cxl device %s. rc = %d.\n", tctx->id, data.sdev_id, rc);
        hxfmsg (htx_d, rc, HTX_HE_HARD_ERROR, msg);
        return rc;
    }

    if (verbose == 'Y') {
        ocxl_afu_enable_messages(tctx->afu_h, OCXL_ERRORS | OCXL_TRACING);
    } else {
        ocxl_afu_enable_messages(tctx->afu_h, OCXL_ERRORS);
    }

    /* Allocate memory areas for afu to copy to/from */
    tctx->src = aligned_alloc(CACHELINESIZE/2, MEMCPY_SIZE);
    if(tctx->src == NULL) {
        err_no = errno;
        sprintf (msg, "[th:%s]: aligned_alloc for src failed. errno: %d\n", tctx->id, err_no);
        hxfmsg (htx_d, err_no, HTX_HE_HARD_ERROR, msg);
        return -1;
    }

    tctx->dst = aligned_alloc(CACHELINESIZE/2, MEMCPY_SIZE);
    if (tctx->dst == NULL) {
        err_no = errno;
        sprintf(msg, "[th:%s]: mmap for dst buffer failed. errno; %d\n", tctx->id, err_no);
        hxfmsg(htx_d, err_no, HTX_HE_HARD_ERROR, msg);
        return -1;
    }


    /*Initialize WEQ*/
    memcpy3_init_weq (&weq, QUEUE_SIZE);

    /* Point the work element descriptor (wed) at the weq */
    wed = MEMCPY_WED (weq.queue, QUEUE_SIZE / CACHELINESIZE);

    /* Setup a work element in the queue */
    if (tctx->command == MEMCOPY_COMMAND_COPY) {
        memset (&memcpy_we, 0, sizeof(memcpy_we));
        memcpy_we.cmd = MEMCPY_WE_CMD(0, MEMCPY_WE_CMD_COPY);
        memcpy_we.length = htole16((uint16_t) tctx->bufsize);
        memcpy_we.src = htole64((uintptr_t) tctx->src);
        memcpy_we.dst = htole64((uintptr_t) tctx->dst);
    }

    /* Start the AFU */
    err = ocxl_afu_attach (tctx->afu_h, OCXL_ATTACH_FLAGS_NONE);
    if (err != OCXL_OK) {
        sprintf (msg, "[th:%s]: ocxl_afu_attach failed with err: %d\n", tctx->id, err);
        hxfmsg (htx_d, err, HTX_HE_HARD_ERROR, msg);
        goto err;
    }

    /*  Map the per-PASID MMIO space */
    err = ocxl_mmio_map(tctx->afu_h, OCXL_PER_PASID_MMIO, &pp_mmio);
    if (err != OCXL_OK) {
        sprintf (msg, "[th:%s]: ocxl_mmio_map failed for OCXL_PER_PASID_MMIO with err: %d\n", tctx->id, err);
        hxfmsg (htx_d, err, HTX_HE_HARD_ERROR, msg);
        goto err;
    }

    /* Setup the interrupt work element */
    if (tctx->completion_method == INTERRUPT) {
        err = ocxl_irq_alloc (tctx->afu_h, NULL, &afu_irq);
        if (err != OCXL_OK) {
            sprintf (msg, "[th:%s]: ocxl_irq_alloc failed for afu_irq with err: %d\n", tctx->id, err);
            hxfmsg (htx_d, err, HTX_HE_HARD_ERROR, msg);
            goto err;
        }
        afu_irq_handle = ocxl_irq_get_handle (tctx->afu_h, afu_irq);
        memset (&irq_we, 0, sizeof(irq_we));
        irq_we.cmd = MEMCPY_WE_CMD (1, MEMCPY_WE_CMD_IRQ);
        irq_we.src = htole64(afu_irq_handle);
    }

    /* Allocate an IRQ to report errors */
    err = ocxl_irq_alloc (tctx->afu_h, NULL, &err_irq);
    if (err < 0) {
        sprintf (msg, "[th:%s]: ocxl_irq_alloc failed for err_irq with err: %d\n", tctx->id, err);
        hxfmsg (htx_d, 1, HTX_HE_HARD_ERROR, msg);
        goto err;
    }
    /* Let the AFU know the handle to trigger for errors */
    err_irq_handle = ocxl_irq_get_handle (tctx->afu_h, err_irq);
    if (err_irq_handle ==  0) {
        sprintf (msg, "[th:%s]: ocxl_irq_get_handle returned invalid handle for err_irq\n", tctx->id);
        hxfmsg (htx_d, -1, HTX_HE_HARD_ERROR, msg);
        goto err;
    }
    err = ocxl_mmio_write64(pp_mmio, MEMCPY_AFU_PP_IRQ, OCXL_MMIO_LITTLE_ENDIAN, err_irq_handle);
    if (err != OCXL_OK) {
        sprintf (msg, "[th:%s]: pp_mmio: %p, ocxl_mmio_write64 failed for err_irq_handle with err: %d\n", tctx->id, pp_mmio,  err);
        hxfmsg (htx_d, err, HTX_HE_HARD_ERROR, msg);
        goto err;
    }

    /* memory barrier to ensure the descriptor is written to memory before we ask the AFU to use it */
    __sync_synchronize();

    /* Write the address of the work element descriptor to the AFU */
    err = ocxl_mmio_write64 (pp_mmio, MEMCPY_AFU_PP_WED, OCXL_MMIO_LITTLE_ENDIAN, wed);
    if (err != OCXL_OK) {
        sprintf (msg, "[th:%s]: ocxl_mmio_write64 for wed failed with err: %d\n", tctx->id, err);
        hxfmsg(htx_d, err, HTX_HE_HARD_ERROR, msg);
        goto err;
    }

    /* Initialise source buffer with unique(ish) value */
    for (i = 0; i < tctx->bufsize; i++) {
        if (i == 0) {
            *(tctx->src + i) = (char)tctx->th_num;
        } else {
            *(tctx->src + i) = 0xAA;
        }
    }
    gettimeofday (&start, NULL);

    for (oper = 0; oper < tctx->num_oper; oper++) {
        if(exit_flag) {
            break;
        }

        /* setup the work queue */
        if(tctx->command == MEMCOPY_COMMAND_COPY) {
            first_we = last_we = memcpy3_add_we(&weq, memcpy_we);
        }
        if (tctx->completion_method == INTERRUPT) {
            last_we = memcpy3_add_we(&weq, irq_we);
        }
        __sync_synchronize();

        /* press the big red 'go' button */
        first_we->cmd |= MEMCPY_WE_CMD_VALID;

        /* wait for the AFU to be done
         * if we're using an interrupt, we can go to sleep.
         * Otherwise, we poll the last work element status from memory
         */
        if (tctx->completion_method == INTERRUPT) {
           rc = wait_for_irq (tctx->completion_timeout, tctx->afu_h,
                               pp_mmio, afu_irq_handle, err_irq_handle);
        } else {
            rc = wait_for_status (tctx->completion_timeout, last_we, tctx->afu_h, err_irq_handle);
        }
        /* printf("done. oper_count: %d, rc : %d, status: %d\n", oper, rc, first_we->status); */

        if (rc != OCXL_OK) {
            goto err_status;
        }
        if (first_we->status != 1) {
            sprintf(msg, "[th:%s]: error - first_we->status is not equal to 1.\n", tctx->id);
            hxfmsg (htx_d, -1, HTX_HE_HARD_ERROR, msg);
            goto err_status;
        }
        if (last_we != first_we && last_we->status != 1) {
            sprintf (msg, "[th:%s]: error - last_we->status is not equal to 1.\n", tctx->id);
            hxfmsg (htx_d, -1, HTX_HE_HARD_ERROR, msg);
            goto err_status;
        }

        if (tctx->compare) {
            if(tctx->command == MEMCOPY_COMMAND_COPY) {
                rc = compare_buffer (htx_d, tctx->src, tctx->dst, tctx->bufsize);
                if(rc) {
                    sprintf(msg, "[th:%d]:[oper:%d]:[memcopy compare data mismatch]\n",tctx->th_num, oper);
                    hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
                    htx_d->bad_writes += 1;
                } else {
                    if((oper % 50) == 0) {
                        htx_d->good_writes += 1;
                        htx_d->bytes_writ += tctx->bufsize;
                    }
                }
                hxfupdate(UPDATE, htx_d);
            }
            /* goto err_status; */
        }

        restart_afu_if_stopped(pp_mmio);

        if (tctx->command == MEMCOPY_COMMAND_COPY) {
            i=0;
            for (i = 0; i < tctx->bufsize; i++){
                if(i==0)
                    *(tctx->dst + i) = (char)tctx->th_num;
                else
                    *(tctx->dst + i) = 0xBB;
            }
        }
    } /* End of num_oper loop */

    gettimeofday(&end, NULL);
    t = (end.tv_sec - start.tv_sec)*1000000 + end.tv_usec - start.tv_usec;
    sprintf (msg, "[th:%d]:[oper:%d]: %d loops in %d uS (%0.2f uS per loop)\n", tctx->th_num, oper, tctx->num_oper, t, ((float) t)/tctx->num_oper);
    hxfmsg (htx_d, 0, HTX_HE_INFO, msg);

    ocxl_afu_close (tctx->afu_h);
    return 0;

err_status:
    err = ocxl_mmio_read64 (pp_mmio, MEMCPY_AFU_PP_STATUS, OCXL_MMIO_LITTLE_ENDIAN, &status);
    if (err) {
        sprintf(msg, "[th:%d]: read of process status failed: %d\n", tctx->th_num, err);
        hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
    } else {
        sprintf(msg, "[th:%d]: process status at end of failed test=0x%lx\n", tctx->th_num, status);
        hxfmsg(htx_d, 0, HTX_HE_INFO, msg);
    }
err:
    ocxl_afu_close (tctx->afu_h);
    return -1;
}

int compare_buffer (struct htx_data * htx_d, char *wbuf, char *rbuf, __u64 len)
{
    char path[MAX_STRING];
    char s[3];
    static unsigned short miscompare_count = 0; /* miscompare count */
    char work_str[MAX_STRING], misc_data[MSG_TEXT_SIZE];
    char msg[128], *msg_ptr = NULL;
    uint32_t i = 0, j = 0, mis_flag = FALSE;
    int32_t cnt, rc = 0;

    if(memcmp(wbuf, rbuf, len)) {
        while ((mis_flag == FALSE) && (i < len)) {
            if (wbuf[i] != rbuf[i]) {
                mis_flag = TRUE;
            } else {
                i++;
            }
        }
    }
    if (mis_flag == TRUE) {         /* problem with the compare */
        rc = -1;
        msg_ptr = misc_data;        /* show bad compare */
        sprintf(msg_ptr, "Found miscompare at %d position.\n Miscompare Buffer address [rbuf:0x%llx] [wbuf:0x%llx]\n",
                i, (unsigned long long) &rbuf[i], (unsigned long long) &wbuf[i]);
        hxfmsg(htx_d, 0, HTX_HE_INFO, msg_ptr);
        sprintf(msg_ptr, "Miscompare at displacement = %d(%#x). Transfer Size=%lu(%#lx) \n",i, i, len, len);
        strcat(msg_ptr, "Expected Data = ");

        memset(work_str, '\0', MAX_STRING);
        for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++) {
        #ifdef  __HTX_LINUX__
            sprintf(s, "%x", wbuf[j]);
        #else
            sprintf(s, "%0.2x", wbuf[j]);
        #endif
            strcat(work_str, s);
        }
        sprintf(msg_ptr + strlen(msg_ptr), "%s\n", work_str);
        strcat(msg_ptr, "Actual Data   = ");

        memset(work_str, '\0', MAX_STRING);
        for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++) {
        #ifdef  __HTX_LINUX__
        sprintf(s, "%x", rbuf[j]);
        #else
            sprintf(s, "%0.2x", rbuf[j]);
        #endif
             strcat(work_str, s);
        }
        sprintf(msg_ptr + strlen(msg_ptr), "%s\n", work_str);

        rc = pthread_mutex_lock(&create_thread_mutex);
        if (rc) {
            sprintf (msg, "Mutex lock failed in function compare_buffer, rc = %d\n", rc);
            hxfmsg (htx_d, rc, HTX_HE_HARD_ERROR, msg);
            exit(1);
        }
        miscompare_count++;
        cnt = miscompare_count;
        rc = pthread_mutex_unlock (&create_thread_mutex);
        if (rc) {
            sprintf (msg, "Mutex unlock failed in function compare_buffers, rc = %d\n", rc);
            hxfmsg (htx_d, rc, HTX_HE_HARD_ERROR, msg);
            exit(1);
        }

        if (cnt < MAX_MISCOMPARES) {
          /*
           * Expected buffer path
           */
            sprintf (path, "%s/%s.expecetd", htx_d->htx_exer_log_dir, &(htx_d->sdev_id[5]));
            sprintf (work_str, "_%lu_%d", pthread_self(), cnt);
            strcat (path, work_str);
            hxfsbuf (wbuf, len, path, htx_d);
            sprintf (msg_ptr + strlen(msg_ptr), "The miscompare buffer dump files are %s,  ", path);
            /*
             * Actual buffer path
             */

            sprintf (path, "%s/%s.actual", htx_d->htx_exer_log_dir, &(htx_d->sdev_id[5]));
            sprintf (work_str, "_%lu_%d", pthread_self(), cnt);
            strcat (path, work_str);
            hxfsbuf (rbuf, len, path, htx_d);
            sprintf (msg_ptr + strlen(msg_ptr), " and %s\n", path);
        } else {
            sprintf (work_str, "The maximum number of saved miscompare \
                                    buffers (%d) have already\nbeen saved.  The read and write buffers for this \
                                    miscompare will\nnot be saved to disk.\n", MAX_MISCOMPARES);
            strcat (msg_ptr, work_str);
        }
        hxfmsg (htx_d, i, HTX_HE_MISCOMPARE, msg_ptr);
    } /* endif */
    return(rc);

}

void SIGTERM_hdl (int sig, int code, struct sigcontext *scp)
{
    exit_flag = 1;
}
