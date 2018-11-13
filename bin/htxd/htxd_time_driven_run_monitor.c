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


/* @(#)47   1.9  src/htx/usr/lpp/htx/bin/htxd/htxd_hang_monitor.c, htxd, htxubuntu 11/23/17 23:36:55 */



#include <stdio.h>


#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include "htxd.h"
#include "htxd_ecg.h"
#include "htxd_thread.h"
#include "htxd_profile.h"
#include "htxd_trace.h"
#include "htxd_util.h"
#include "htxd_instance.h"

#define LOG_ENTRY_COUNT 6  /* 5 + 1 */


extern htxd *htxd_global_instance;
extern volatile int htxd_shutdown_flag;
int cycles_complete_flag = FALSE;

int htxd_time_driven_run_monitor_ecg(htxd_ecg_info *p_ecg_info_to_time_driven_run_monitor, char *command_result)
{
    struct htxshm_HE *p_HE;
    struct htxshm_hdr *p_hdr;
    int i;
    time_t epoch_time_now;
    char time_driven_run_monitor_log_entry[512];
    int first_time_TE_flag = 0;

    /* wait while ECG get active */
    while(p_ecg_info_to_time_driven_run_monitor->ecg_status != ECG_ACTIVE) {
        sleep(5);
    }

    epoch_time_now = time( (time_t *) 0);

    p_HE = (struct htxshm_HE *)(p_ecg_info_to_time_driven_run_monitor->ecg_shm_addr.hdr_addr + 1);

    p_hdr = p_ecg_info_to_time_driven_run_monitor->ecg_shm_addr.hdr_addr;

    for(i = 0; i < p_ecg_info_to_time_driven_run_monitor->ecg_shm_exerciser_entries ; i++) {

        if(p_HE->max_cycles != 0) {
            if(p_HE->cycles <  p_HE->max_cycles) {
                cycles_complete_flag = FALSE;
            }else {
                cycles_complete_flag = TRUE;
            }
        } else {
            cycles_complete_flag = FALSE;
        }

        if(p_HE->PID == 0) {
            if(p_HE->sp3 == 1) {/*set_stop_flag_in_exer_shm sets the sp3 after time_driven value is reached*/
                first_time_TE_flag = 1;
            }
        }
        if( (p_HE->PID != 0) &&
            (p_HE->tm_last_upd != 0) &&
            (cycles_complete_flag == FALSE)  &&
            (htxd_get_device_run_sem_status(p_ecg_info_to_time_driven_run_monitor->ecg_sem_id, i) == 0) &&
            (htxd_get_device_error_sem_status(p_ecg_info_to_time_driven_run_monitor->ecg_sem_id, i) == 0) &&
            (p_HE->hung_exer == 0) &&
            (p_HE->sp2 == 0) &&  /*sp2 flag is set for exercisers which supports time driven, it's done inside library when htx_start is called, hence those exercisers will exit by itself*/
            (p_ecg_info_to_time_driven_run_monitor->ecg_shm_addr.hdr_addr->started != 0)&&
            ( ((epoch_time_now -  atoi(htxd_get_ecg_start_time())) > (p_hdr->time_of_exec)) && (p_hdr->time_of_exec != 0) &&( first_time_TE_flag ==1) )) {
                p_HE->sp3 = 1;
                htxd_send_SIGTERM(p_HE->PID);
                sprintf(time_driven_run_monitor_log_entry, "%s for %s received SIGTERM from htxd_time_driven_run_monitor!\n", p_HE->HE_name, p_HE->sdev_id);
                htxd_send_message(time_driven_run_monitor_log_entry, 0, HTX_HE_SOFT_ERROR, HTX_SYS_MSG);
        }
        p_HE++;
    }

    return 0;
}

void *htxd_time_driven_run_monitor(void *data)
{

    htxd_enable_thread_cancel_state_type();

    sleep(10); /* wait for system start up */

    do {
        htxd_process_all_active_ecg(htxd_time_driven_run_monitor_ecg, NULL);
        sleep(TIME_DRIVEN_RUN_MONITOR_PERIOD);
    } while(htxd_shutdown_flag == FALSE);

    return NULL;
}

/* start time_driven_run_monitor thread */
int htxd_start_time_driven_run_monitor(htxd_thread **time_driven_run_monitor_thread)
{
    int rc = 0, return_code = -1;
    char temp_str[128];

    if(*time_driven_run_monitor_thread == NULL) {
        *time_driven_run_monitor_thread= malloc(sizeof(htxd_thread));
        if(*time_driven_run_monitor_thread== NULL) {
            sprintf(temp_str, "time_driven_run_monitor_thread: filed malloc with errno <%d>.\n", errno);
            htxd_send_message(temp_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
            exit(1);
        }
        memset(*time_driven_run_monitor_thread, 0, sizeof(htxd_thread));

        (*time_driven_run_monitor_thread)->thread_function = htxd_time_driven_run_monitor;
        (*time_driven_run_monitor_thread)->thread_data = NULL;

        return_code = htxd_thread_create(*time_driven_run_monitor_thread);
    #ifdef __HTX_LINUX__
        if ((htxd_get_equaliser_offline_cpu_flag()) == 1) {
            rc = do_the_bind_thread((*time_driven_run_monitor_thread)->thread_id);
            if (rc < 0) {
                sprintf(temp_str, "binding time_driven_run monitor process to core 0 failed.\n");
                htxd_send_message(temp_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
            }

        }
    #endif
    }
    return return_code;
}

/* stop time_driven_run  monitor thread */
int htxd_stop_time_driven_run_monitor(htxd_thread **time_driven_run_monitor_thread)
{
    int return_code = -1;
    char trace_string[256];


    return_code = htxd_thread_cancel(*time_driven_run_monitor_thread);
    if(return_code != 0) {
        sprintf(trace_string, "htxd_stop_time_driven_run_monitor_thread : htxd_thread_cancel returned with <%d>", return_code);
        HTXD_TRACE(LOG_ON, trace_string);
    }

    return_code = htxd_thread_join(*time_driven_run_monitor_thread);
    if(return_code != 0) {
        sprintf(trace_string, "htxd_stop_time_driven_run_monitor_thread: htxd_thread_join returned with <%d>", return_code);
        HTXD_TRACE(LOG_ON, trace_string);
    }

    if(*time_driven_run_monitor_thread!= NULL) {
        free(*time_driven_run_monitor_thread);
        *time_driven_run_monitor_thread= NULL;
    }

    return return_code;
}

