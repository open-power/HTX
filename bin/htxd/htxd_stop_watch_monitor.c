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



#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include "htxd.h"
#include "htxd_thread.h"
#include "htxd_instance.h"
#include "htxd_util.h"
#include "htxd_signal.h"
#include "htxd_trace.h"

extern volatile int htxd_shutdown_flag;
extern htxd *htxd_global_instance;

int htxd_get_run_time_set_exer_count(htxd_ecg_info *p_ecg_info_list)
{
	int run_time_set_exer_count = 0;
	struct htxshm_HE *p_HE;
	int i;

	p_HE = (struct htxshm_HE *)(p_ecg_info_list->ecg_shm_addr.hdr_addr + 1);
	
	for(i = 0; i < p_ecg_info_list->ecg_shm_exerciser_entries ; i++) {
		if(p_HE->test_run_period != 0) {
			run_time_set_exer_count++;
		}
		p_HE++;
	}

	return run_time_set_exer_count;
}



void* htxd_stop_watch_monitor(void* dummy_ptr)
{
	htxd_ecg_info * p_ecg_info_list;
	int run_time_set_exer_count = 0;
	time_t	current_time;
	time_t	actual_run_time;
	struct htxshm_HE *p_HE;
	int i;
	htxd_ecg_manager *p_ecg_manager;
	char temp_str[512];


	htxd_enable_thread_cancel_state_type();
	p_ecg_manager = htxd_get_ecg_manager();
	p_ecg_info_list = htxd_get_ecg_info_list(p_ecg_manager);

	/* wait while ECG get active */
	while(p_ecg_info_list->ecg_status != ECG_ACTIVE) {
		sleep(5);
	}


	run_time_set_exer_count = htxd_get_run_time_set_exer_count(p_ecg_info_list);

	if(run_time_set_exer_count > 0){
		do {	
			p_HE = (struct htxshm_HE *)(p_ecg_info_list->ecg_shm_addr.hdr_addr + 1);
			current_time = time((time_t *) 0);
			
			for(i = 0; i < p_ecg_info_list->ecg_shm_exerciser_entries ; i++) {
				actual_run_time = current_time - p_HE->test_started_time; 
				if( (p_HE->test_run_period != 0) && (p_HE->started_delay_flag == 0) && (p_HE->test_started_time != 0) && (actual_run_time >= p_HE->test_run_period) && (p_HE->test_run_period_expired == 0) ) {
					p_HE->test_run_period_expired = 1;
					run_time_set_exer_count--;
					if(p_HE->PID != 0) {
						sprintf(temp_str, "<%s> is reeived SIGTERM from stop watch monitor after runtime of <%d> seconds, pid is <%d>", p_HE->HE_name, (int)p_HE->test_run_period, p_HE->PID);
						htxd_send_message(temp_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
						htxd_send_SIGTERM(p_HE->PID);
					}
					if(run_time_set_exer_count <= 0) {
						break;
					}
				} 
				p_HE++;
			} 

			sleep(1);
		}while((run_time_set_exer_count > 0) && (htxd_shutdown_flag == FALSE));
	}
		
	return NULL;
}



/* start stop_watch_monitor thread */
int htxd_start_stop_watch_monitor(htxd_thread *p_stop_watch_monitor_thread)
{
	int rc = 0, return_code = -1;
	char temp_str[256];

	memset(p_stop_watch_monitor_thread, 0, sizeof(htxd_thread));

	p_stop_watch_monitor_thread->thread_function = htxd_stop_watch_monitor;
	p_stop_watch_monitor_thread->thread_data = NULL;

	return_code = htxd_thread_create(p_stop_watch_monitor_thread);
#ifdef __HTX_LINUX__
	if ((htxd_get_equaliser_offline_cpu_flag()) == 1) {
		rc = do_the_bind_thread((p_stop_watch_monitor_thread)->thread_id);
		if (rc < 0) {
			sprintf(temp_str, "binding stop watch monitor thread to core 0 failed.\n");
			htxd_send_message(temp_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
		}

	}
#endif
	if(return_code == 0) {
		htxd_global_instance->is_stop_watch_monitor_initailized = TRUE;
	} else {
		sprintf(temp_str, "ERROR: while starting stop watch thread, error code = %d", return_code);
		htxd_send_message(temp_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}

	return return_code;
}



/* stop stop_watch_monitor thread */
int htxd_stop_stop_watch_monitor(htxd_thread *p_stop_watch_monitor_thread)
{
	int return_code = -1;
	char trace_string[256];


	return_code = htxd_thread_cancel(p_stop_watch_monitor_thread);
	if(return_code != 0) {
		sprintf(trace_string, "htxd_stop_stop_watch_monitor: htxd_thread_cancel returned with <%d>", return_code);
		HTXD_TRACE(LOG_ON, trace_string);
	}

	return_code = htxd_thread_join(p_stop_watch_monitor_thread);
	if(return_code != 0) {
		sprintf(trace_string, "htxd_stop_stop_watch_monitor: htxd_thread_join returned with <%d>", return_code);
		HTXD_TRACE(LOG_ON, trace_string);
	}

	htxd_global_instance->is_stop_watch_monitor_initailized = FALSE;

	return return_code;
}


