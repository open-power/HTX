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
/* @(#)47	1.4  src/htx/usr/lpp/htx/bin/htxd/htxd_hang_monitor.c, htxd, htxubuntu 8/23/15 23:34:23 */



#include <stdio.h>


#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "htxd.h"
#include "htxd_ecg.h"
#include "htxd_thread.h"
#include "htxd_profile.h"

extern htxd *htxd_global_instance;
extern volatile int htxd_shutdown_flag;

int htxd_hang_monitor_ecg(htxd_ecg_info *p_ecg_info_to_hang_monitor, char *command_result)
{
	struct htxshm_HE *p_HE;
	int i;
	int return_code = 0;
	time_t epoch_time_now;
	long elasped_time;
	long max_update_lapse;
	char hang_monitor_log_entry[512];
	short int           hung_toggle;
	int cycles_complete_flag = FALSE;

	/* wait while ECG get active */
	while(p_ecg_info_to_hang_monitor->ecg_status != ECG_ACTIVE) {
		sleep(5);
	}

	epoch_time_now = time( (time_t *) 0);

	p_HE = (struct htxshm_HE *)(p_ecg_info_to_hang_monitor->ecg_shm_addr.hdr_addr + 1);

	for(i = 0; i < p_ecg_info_to_hang_monitor->ecg_shm_exerciser_entries ; i++) {

		if(p_HE->max_cycles != 0) {
			if(p_HE->cycles <  p_HE->max_cycles) {
				cycles_complete_flag = FALSE;
			}else {
				cycles_complete_flag = TRUE;
			}
		} else {
			cycles_complete_flag = FALSE;
		}

		if(	(p_HE->PID != 0) &&
			(p_HE->tm_last_upd != 0) &&
			(cycles_complete_flag == FALSE)  &&
			(htxd_get_device_run_sem_status(p_ecg_info_to_hang_monitor->ecg_sem_id, i) == 0) &&
			(htxd_get_device_error_sem_status(p_ecg_info_to_hang_monitor->ecg_sem_id, i) == 0) ) {

			elasped_time = epoch_time_now - p_HE->tm_last_upd;
			max_update_lapse = (long) (p_HE->max_run_tm + p_HE->idle_time);
			if (elasped_time > max_update_lapse) {
				hung_toggle = (elasped_time / max_update_lapse) % 2;
				if (p_HE->hung_flag != hung_toggle) {
					p_HE->hung_flag = hung_toggle;
					sprintf(hang_monitor_log_entry, "%s for %s is HUNG!\n"
						"Max run time (set in mdt) = %d seconds.\n"
						"Current elasped time = %d seconds.",
						p_HE->HE_name, p_HE->sdev_id,
						p_HE->max_run_tm,
						(int)(clock - p_HE->tm_last_upd));
				}
			} else {
				p_HE->hung_flag = 0;
			}
		}
		p_HE++;
	}

	return return_code;
}




void *htxd_hang_monitor(void *data)
{
	int hang_monitor_period;


	htxd_enable_thread_cancel_state_type();

	hang_monitor_period = htxd_get_hang_monitor_period();

	sleep(10); /* wait for system start up */

	do {
		htxd_process_all_active_ecg(htxd_hang_monitor_ecg, NULL);
		sleep(hang_monitor_period);
	} while(htxd_shutdown_flag == FALSE);

	return NULL;
}



/* start hang monitor thread */
int htxd_start_hang_monitor(htxd_thread **hang_monitor_thread)
{
	int rc = 0, return_code = -1;
	char temp_str[128];

	if(*hang_monitor_thread == NULL) {
		*hang_monitor_thread = malloc(sizeof(htxd_thread));
		if(*hang_monitor_thread == NULL) {
			exit(1);
		}
		memset(*hang_monitor_thread, 0, sizeof(htxd_thread));

		(*hang_monitor_thread)->thread_function = htxd_hang_monitor;
		(*hang_monitor_thread)->thread_data = NULL;

		return_code = htxd_thread_create(*hang_monitor_thread);
	#ifdef __HTX_LINUX__
		if ((htxd_get_equaliser_offline_cpu_flag()) == 1) {
			rc = do_the_bind_thread((*hang_monitor_thread)->thread_id);
			if (rc < 0) {
	            sprintf(temp_str, "binding hang monitor process to core 0 failed.\n");
	            htxd_send_message(temp_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			}

		}
	#endif
	}
	return return_code;
}



/* stop hang monitor thread */
int htxd_stop_hang_monitor(htxd_thread **hang_monitor_thread)
{
	int return_code = -1;


	return_code = htxd_thread_cancel(*hang_monitor_thread);

	if(*hang_monitor_thread != NULL) {
		free(*hang_monitor_thread);
	}

	return return_code;
}
