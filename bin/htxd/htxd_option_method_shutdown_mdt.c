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
/* @(#)59	1.8  src/htx/usr/lpp/htx/bin/htxd/htxd_option_method_shutdown_mdt.c, htxd, htxubuntu 8/23/15 23:34:38 */



#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "htxd.h"
#include "htxd_ipc.h"
#include "htxd_define.h"
#include "htxd_instance.h"
#include "htxd_util.h"
#include "htxd_trace.h"
#include "htxd_signal.h"


extern int htxd_idle_daemon(void);

extern int sem_length;
extern volatile int htxd_ecg_shutdown_flag;


/* cleanup the IPC */
int htxd_cleanup_ecg_ipc(htxd_ecg_info *p_ecg_info)
{

	htxd_cleanup_sem(p_ecg_info->ecg_sem_id);

	htxd_cleanup_shm(p_ecg_info->ecg_shm_id,p_ecg_info->ecg_shm_addr.hdr_addr);

	return 0;
}

/* Cleanup equaliser realted setting */
int htxd_cleanup_equaliser_setting()
{
	htxd_set_equaliser_offline_cpu_flag(0);
	#ifdef __HTX_LINUX__
	    htx_unbind_process();
	#endif
}

/* count running exercisers */
int htxd_count_exer_still_running(htxd_ecg_info *p_ecg_info_list)
{
	int					stopped_exer_count = 0;
	union shm_pointers	shm_union_pointer;
	int					number_of_exercisers_to_stop;
	int					i;


	shm_union_pointer.hdr_addr = (p_ecg_info_list->ecg_shm_addr).hdr_addr + 1;
	number_of_exercisers_to_stop = p_ecg_info_list->ecg_exerciser_entries;

	for(i = 0; i < number_of_exercisers_to_stop; i++) {
		if( (shm_union_pointer.HE_addr + i)->PID == 0) {
			stopped_exer_count++;
		}
	}

	return (number_of_exercisers_to_stop - stopped_exer_count);
}



/* stop execution of exercisers */
int htxd_unload_exercisers(htxd_ecg_info *p_ecg_info_list)
{

	union shm_pointers	shm_union_pointer;
	int					pid_to_unload;
	int					number_of_exercisers_to_stop;
	int					i;
	int					exer_still_running;
	char				trace_str[256];
	htxd_ecg_manager *		p_ecg_manager;


	HTXD_FUNCTION_TRACE(FUN_ENTRY, "htxd_unload_exercisers");

	(p_ecg_info_list->ecg_shm_addr).hdr_addr->shutdown = 1;
	shm_union_pointer.hdr_addr = (p_ecg_info_list->ecg_shm_addr).hdr_addr + 1;
	number_of_exercisers_to_stop = p_ecg_info_list->ecg_exerciser_entries;
	sprintf(trace_str, "htxd_unload_exercisers() number_of_exercisers_to_stop <%d>", number_of_exercisers_to_stop);
	HTXD_TRACE(LOG_OFF, trace_str);

	/* release all semphore locks */
	htxd_release_all_semaphore(p_ecg_info_list->ecg_sem_id);

	for(i = 0; i < number_of_exercisers_to_stop; i++) {
		pid_to_unload = (shm_union_pointer.HE_addr + i)->PID;
		if(pid_to_unload != 0) {
			kill(pid_to_unload, SIGTERM);
			sprintf(trace_str, "htxd_unload_exercisers() sent SIGTERM to PID <%d>", pid_to_unload);
			HTXD_TRACE(LOG_OFF, trace_str);
		//	sleep(5);
		}
	}

	while(1) {
		sleep(WAIT_TIME_TO_STOP_EXER);
		exer_still_running = htxd_count_exer_still_running(p_ecg_info_list);
		if(exer_still_running == 0) {
			HTXD_TRACE(LOG_OFF, "all exercisers are stopped");
			break;
		} else {
			sprintf(trace_str, "still running exerciser count <%d>\n", exer_still_running);
			HTXD_TRACE(LOG_OFF, trace_str);
		}
		HTXD_TRACE(LOG_OFF, "wait to stop all exercisers");
	}

	p_ecg_manager = htxd_get_ecg_manager();
	p_ecg_manager->loaded_device_count -= number_of_exercisers_to_stop;
	p_ecg_manager->ecg_device_count -= number_of_exercisers_to_stop;

	HTXD_FUNCTION_TRACE(FUN_EXIT, "htxd_unload_exercisers");
	return 0;
}



/* execute post run script for exerciser */
void htxd_execute_post_run_script(char *ecg_name)
{
	char post_script_command_line[256];

	sprintf(post_script_command_line, "%s/etc/scripts/exer_cleanup %s >%s/exer_cleanup.ouput 2>&1", global_htx_home_dir, ecg_name, global_htxd_log_dir);
	system(post_script_command_line);
}



/* ecg shutdown command function */
int htxd_option_method_shutdown_mdt(char **result)
{
	htxd *htxd_instance;
	htxd_ecg_info * p_ecg_info_list;
	htxd_ecg_info * p_ecg_info_node_to_remove = NULL;
	char command_ecg_name[MAX_ECG_NAME_LENGTH];
	char temp_str[512];


	HTXD_FUNCTION_TRACE(FUN_ENTRY, "htxd_option_method_shutdown_mdt");
	htxd_instance = htxd_get_instance();
	strcpy(command_ecg_name, htxd_get_command_ecg_name() );

	*result = malloc(100);

	if( htxd_is_daemon_idle() == TRUE) {
		strcpy(*result, "No ECG/MDT is currently running");
		HTXD_TRACE(LOG_OFF, *result);
		HTXD_FUNCTION_TRACE(FUN_EXIT, "htxd_option_method_shutdown_mdt");
		return 1;
	}

	/* if MDT name is not provided try to get it from system */
	if(command_ecg_name[0] == '\0') {
		if(htxd_is_daemon_selected()  == TRUE) {
			strcpy(command_ecg_name, htxd_instance->p_ecg_manager->selected_ecg_name);
		} else {
			strcpy(command_ecg_name, htxd_get_running_ecg_name());
		}
	} else if(htxd_is_file_exist(command_ecg_name)	== FALSE) { /* checking provided MDT name */
		sprintf(*result, "specified mdt<%s> is invalid", command_ecg_name);
		HTXD_TRACE(LOG_OFF, *result);
		HTXD_FUNCTION_TRACE(FUN_EXIT, "htxd_option_method_shutdown_mdt");
		return 1;
	}

	p_ecg_info_list = htxd_get_ecg_info_list(htxd_instance->p_ecg_manager);
	while(p_ecg_info_list != NULL) {
		if( strcmp(command_ecg_name, p_ecg_info_list->ecg_name) == 0) {
			p_ecg_info_node_to_remove = p_ecg_info_list;
			//break;  ???
		}
		p_ecg_info_list = p_ecg_info_list->ecg_info_next;
	}

	if(p_ecg_info_node_to_remove == NULL) {
		sprintf(*result, "specified mdt(%s) is already inactive", command_ecg_name);
		HTXD_TRACE(LOG_OFF, *result);
		HTXD_FUNCTION_TRACE(FUN_EXIT, "htxd_option_method_shutdown_mdt");
		return 1;
	}

	htxd_ecg_shutdown_flag = TRUE;

	HTXD_TRACE(LOG_OFF, "shutdown detaching ecg_info node");
	htxd_remove_ecg_info_node(htxd_instance->p_ecg_manager, p_ecg_info_node_to_remove);

	HTXD_TRACE(LOG_OFF, "shutdown unload exercisers");
	htxd_unload_exercisers(p_ecg_info_node_to_remove);

	HTXD_TRACE(LOG_OFF, "shutdown cleanup ipc");
	htxd_cleanup_ecg_ipc(p_ecg_info_node_to_remove);

	if (htxd_get_equaliser_offline_cpu_flag() == 1) {
		HTXD_TRACE(LOG_OFF, "Unload equaliser setting");
		htxd_cleanup_equaliser_setting();
	}

	HTXD_TRACE(LOG_OFF, "shutdown execute post run scripts");
	htxd_execute_post_run_script(p_ecg_info_node_to_remove->ecg_name);

	sprintf(temp_str, "MDT <%s> is shutdhown", p_ecg_info_node_to_remove->ecg_name);
	htxd_send_message (temp_str, 0, HTX_SYS_INFO, HTXD_MDT_SHUTDOWN_MSG);

	free(p_ecg_info_node_to_remove);

	if(htxd_get_running_ecg_count() == 0) {
		HTXD_TRACE(LOG_OFF, "no mdt is currently running, daemon goes to idle state");
		htxd_idle_daemon();
	}

	/* delete bootme entries */
	sprintf(temp_str, "%s/etc/scripts/%s off", global_htx_home_dir, HTXD_BOOTME_SCRIPT);
	system(temp_str);

/*	htxd_display_exer_table();	 */

	sprintf(*result, "ECG (%s) shutdown successfully", command_ecg_name);
	sprintf(temp_str, "date +\"ECG (%s) was shutdown on %%x at %%X %%Z\" >>%s/%s", command_ecg_name, global_htxd_log_dir, HTXD_START_STOP_LOG);
	system(temp_str);

	HTXD_TRACE(LOG_ON, *result);
	HTXD_FUNCTION_TRACE(FUN_EXIT, "htxd_option_method_shutdown_mdt");
	return 0;
}



/* shutdown all the running mdts */
int htxd_shutdown_all_mdt(void)
{

	htxd *htxd_instance;
	htxd_ecg_info * p_ecg_info_list;
	htxd_ecg_info * p_ecg_info_node_to_remove = NULL;


	HTXD_FUNCTION_TRACE(FUN_ENTRY, "htxd_shutdown_all_mdt");
	htxd_instance = htxd_get_instance();

	if (htxd_get_daemon_state() == HTXD_DAEMON_IDLE)  {
		HTXD_FUNCTION_TRACE(FUN_EXIT, "htxd_shutdown_all_mdt");
		return 1;
	}

	p_ecg_info_list = htxd_get_ecg_info_list(htxd_instance->p_ecg_manager);
	while(p_ecg_info_list != NULL) {
		p_ecg_info_node_to_remove = p_ecg_info_list;
		htxd_remove_ecg_info_node(htxd_instance->p_ecg_manager, p_ecg_info_node_to_remove);
		htxd_unload_exercisers(p_ecg_info_node_to_remove);
		htxd_cleanup_ecg_ipc(p_ecg_info_node_to_remove);
		htxd_execute_post_run_script(p_ecg_info_node_to_remove->ecg_name);
		p_ecg_info_list = p_ecg_info_list->ecg_info_next;
		free(p_ecg_info_node_to_remove);
	}

	htxd_idle_daemon();

/*	htxd_display_exer_table();	 */

	HTXD_FUNCTION_TRACE(FUN_EXIT, "htxd_shutdown_all_mdt");
	return 0;
}
