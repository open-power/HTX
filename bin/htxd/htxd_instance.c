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
/* @(#)48	1.9  src/htx/usr/lpp/htx/bin/htxd/htxd_instance.c, htxd, htxubuntu 11/24/15 23:59:21 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "htxd.h"
#include "htxd_common_define.h"
#include "htxd_util.h"
#include "htxd_trace.h"


extern volatile int htxd_shutdown_flag;
extern volatile int htxd_ecg_shutdown_flag;


htxd * htxd_get_instance(void)
{
	return htxd_global_instance;
}


int htxd_is_daemon_idle(void)
{
	if(htxd_global_instance->run_state == HTXD_DAEMON_IDLE) {
		return TRUE;
	}

	return FALSE;
}


int htxd_is_daemon_selected(void)
{
	if(htxd_global_instance->run_state == HTXD_DAEMON_SELECTED) {
		return TRUE;
	}

	return FALSE;
}


void htxd_set_daemon_idle(void)
{
	htxd_global_instance->run_state = HTXD_DAEMON_IDLE;
}


int htxd_get_daemon_state(void)
{
	return htxd_global_instance->run_state;
}


void htxd_set_daemon_state(int temp_state)
{
	htxd_global_instance->run_state = temp_state;
}




int htxd_get_test_running_state(void)
{
	return htxd_global_instance->is_test_active;
}


void htxd_set_test_running_state(int temp_state)
{
	htxd_global_instance->is_test_active = temp_state;
}



void htxd_set_init_syscfg_flag(int new_state)
{
	htxd_global_instance->init_syscfg_flag = new_state;
}

int htxd_is_init_syscfg(void)
{
	return htxd_global_instance->init_syscfg_flag;
}


int htxd_get_msg_queue_id(void)
{
	return htxd_global_instance->p_ecg_manager->message_queue_id;
}


void htxd_set_program_name(char *temp_name)
{
	strcpy(htxd_global_instance->program_name, temp_name);
}


char *htxd_get_program_name(void)
{
	return htxd_global_instance->program_name;
}


void htxd_set_htx_path(char *path_value)
{
	strcpy(htxd_global_instance->htx_path, path_value);
}

char *htxd_get_htx_path(void)
{
	return htxd_global_instance->htx_path;
}


void htxd_set_htx_msg_pid(pid_t new_htx_msg_pid)
{
	htxd_global_instance->htx_msg_pid = new_htx_msg_pid;
}

pid_t htxd_get_htx_msg_pid(void)
{
	return htxd_global_instance->htx_msg_pid;
}

void htxd_set_htx_stats_pid(pid_t new_htx_stats_pid)
{
	htxd_global_instance->htx_stats_pid = new_htx_stats_pid;
}


pid_t htxd_get_htx_stats_pid(void)
{
	return htxd_global_instance->htx_stats_pid;
}


void htxd_set_dr_child_pid(pid_t new_dr_child_pid)
{
	htxd_global_instance->dr_child_pid = new_dr_child_pid;
}

pid_t htxd_get_dr_child_pid(void)
{
	return htxd_global_instance->dr_child_pid;
}


void htxd_set_equaliser_pid(int new_equaliser_pid)
{
	htxd_global_instance->equaliser_pid = new_equaliser_pid;
}

pid_t htxd_get_equaliser_pid(void)
{
	return htxd_global_instance->equaliser_pid;
}


void htxd_set_dr_sem_id(int new_dr_sem_id)
{
	htxd_global_instance->dr_sem_id = new_dr_sem_id;
}


int htxd_get_dr_sem_id(void)
{
	return htxd_global_instance->dr_sem_id;
}

int htxd_get_dr_sem_key(void)
{
	return htxd_global_instance->dr_sem_key;
}


void htxd_set_dr_reconfig_restart(int value)
{
	htxd_global_instance->dr_reconfig_restart = value;
}


int htxd_get_dr_reconfig_restart(void)
{
	return htxd_global_instance->dr_reconfig_restart;
}



void htxd_set_equaliser_debug_flag(int value)
{
	htxd_global_instance->equaliser_debug_flag = value;
}


int htxd_get_equaliser_debug_flag(void)
{
	return htxd_global_instance->equaliser_debug_flag;
}

void htxd_set_equaliser_offline_cpu_flag(int value)
{
	htxd_global_instance->enable_offline_cpu = value;
}


int htxd_get_equaliser_offline_cpu_flag(void)
{
	return htxd_global_instance->enable_offline_cpu;
}


void htxd_set_equaliser_shm_addr(union shm_pointers value)
{
	htxd_global_instance->equaliser_shm_addr = value;
}


union shm_pointers htxd_get_equaliser_shm_addr(void)
{
	return htxd_global_instance->equaliser_shm_addr;
}




void htxd_set_equaliser_semhe_id(int value)
{
	htxd_global_instance->equaliser_semhe_id = value;
}


int htxd_get_equaliser_semhe_id(void)
{
	return htxd_global_instance->equaliser_semhe_id;
}




void htxd_set_equaliser_conf_file(char *value)
{
	htxd_global_instance->equaliser_conf_file = value;
}


char *htxd_get_equaliser_conf_file(void)
{
	return htxd_global_instance->equaliser_conf_file;
}


texer_list * htxd_get_exer_table(void)
{
	if(htxd_global_instance->p_ecg_manager == NULL) {
		return NULL;
	}

	return htxd_global_instance->p_ecg_manager->exer_table;
}


int htxd_get_exer_table_length(void)
{
	return htxd_global_instance->p_ecg_manager->exer_table_length;
}


void htxd_set_ecg_manager(htxd_ecg_manager * htxd_ecg_mgr)
{
	htxd *p_temp_instance;

	p_temp_instance = htxd_get_instance();

	p_temp_instance->p_ecg_manager = htxd_ecg_mgr;
}

htxd_ecg_manager * htxd_get_ecg_manager(void)
{
        htxd *p_temp_instance;

        p_temp_instance = htxd_get_instance();

        return p_temp_instance->p_ecg_manager;
}


void htxd_delete_ecg_manager(void)
{
	htxd *p_temp_instance;

	p_temp_instance = htxd_get_instance();

	free(p_temp_instance->p_ecg_manager);

	p_temp_instance->p_ecg_manager = NULL;
}


int htxd_is_hotplug_monitor_initialized(void)
{
	htxd *p_temp_instance;

	p_temp_instance = htxd_get_instance();

	if(p_temp_instance->p_hotplug_monitor_thread == 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}


void htxd_remove_hotplug_monitor(void)
{
	htxd *p_temp_instance;

	p_temp_instance = htxd_get_instance();

	p_temp_instance->p_hotplug_monitor_thread = 0;
}


int htxd_is_hang_monitor_initialized(void)
{
	htxd *p_temp_instance;

	p_temp_instance = htxd_get_instance();

	if(p_temp_instance->p_hang_monitor_thread == 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}


void htxd_remove_hang_monitor(void)
{
	htxd *p_temp_instance;

	p_temp_instance = htxd_get_instance();

	p_temp_instance->p_hang_monitor_thread = 0;
}



int htxd_is_stop_watch_monitor_initialized(void)
{
	htxd *p_temp_instance;

	p_temp_instance = htxd_get_instance();

	if(p_temp_instance->is_stop_watch_monitor_initailized == 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}


void htxd_remove_stop_watch_monitor(void)
{
	htxd *p_temp_instance;

	p_temp_instance = htxd_get_instance();

	p_temp_instance->is_stop_watch_monitor_initailized = 0;
}



int htxd_is_profile_initialized(htxd *htxd_instance)
{
    if(htxd_instance->p_profile == NULL) {
        return  FALSE;
    } else {
        return TRUE;
    }
}


htxd * htxd_create_instance(void)
{
	htxd *htxd_new_instance;
	char trace_string[256];


	if( htxd_global_instance != NULL) {
		return htxd_global_instance;
	}

	htxd_new_instance = malloc(sizeof(htxd));
	if(htxd_new_instance == NULL) {
		sprintf(trace_string, "htxd_create_instance: failed malloc with errno <%d>, exiting...", errno);	
		HTXD_TRACE(LOG_ON, trace_string);
		exit(0);
	}
	memset(htxd_new_instance, 0, sizeof(htxd) );

	return htxd_new_instance;

}

void initialize_command(htxd_command *p_htxd_command)
{
	p_htxd_command->command_index = 0;
	p_htxd_command->ecg_name[0] = '\0';
	p_htxd_command->option_list[0] = '\0';
}


htxd_command * htxd_create_command(void)
{
	htxd_command *p_htxd_command;
	char trace_string[256];


	p_htxd_command = malloc(sizeof(htxd_command));
	if(p_htxd_command == NULL) {
		sprintf(trace_string, "htxd_create_command: failed malloc with errno <%d>, exiting...", errno);	
		HTXD_TRACE(LOG_ON, trace_string);
		exit(0);
	}
	initialize_command(p_htxd_command);

	return p_htxd_command;
}

void init_htxd_instance(htxd *p_htxd_instance)
{
	p_htxd_instance->program_name[0]		= '\0';
	p_htxd_instance->htx_path[0]			= '\0';
	p_htxd_instance->shutdown_flag			= FALSE;
	p_htxd_instance->daemon_pid			= 0;
	p_htxd_instance->htx_msg_pid			= 0;
	p_htxd_instance->htx_stats_pid			= 0;
	p_htxd_instance->equaliser_pid			= 0;
	p_htxd_instance->port_number			= HTXD_DEFAULT_PORT;
	p_htxd_instance->run_level			= 0;
	p_htxd_instance->run_state			= HTXD_DAEMON_IDLE;
	p_htxd_instance->trace_level			= 0;
	p_htxd_instance->p_child_pid_list		= NULL;
	p_htxd_instance->p_profile			= NULL;
	p_htxd_instance->p_command			= htxd_create_command();
	p_htxd_instance->dr_sem_key			= 0xDEADBEEF;
	p_htxd_instance->dr_sem_id			= -1;
	p_htxd_instance->dr_reconfig_restart		= 0;
	p_htxd_instance->dr_is_done			= 0;
	p_htxd_instance->dr_child_pid			= 0;
	p_htxd_instance->p_hang_monitor_thread		= NULL;
	p_htxd_instance->p_hotplug_monitor_thread	= NULL;
	p_htxd_instance->equaliser_debug_flag		= 0;
	p_htxd_instance->is_auto_started		= 0;
	p_htxd_instance->init_syscfg_flag		= FALSE;
	p_htxd_instance->master_client_mode		= 0;
	p_htxd_instance->is_test_active			= 0;
	p_htxd_instance->is_stop_watch_monitor_initailized	= 0;
}



int htxd_reset_exer_pid(pid_t child_pid, char *exit_reason)
{

	texer_list *p_exer_table;
	int exer_position_in_exer_table;
	char exit_detail_string[512];
	char temp_str[100];

	p_exer_table = htxd_get_exer_table();
	if(p_exer_table == NULL) {
		return -1;
	}

	exer_position_in_exer_table = htxd_get_exer_position_in_exer_table_by_pid(p_exer_table, child_pid);
	if(exer_position_in_exer_table != -1) {
		sprintf(exit_detail_string, "%s HE program for %s terminated by %s",
			(p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->HE_name,
			(p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->sdev_id,
			exit_reason);

		if( (p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->user_term) {
			strcat(exit_detail_string, " because of user request");
		} else if ( (p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->DR_term ) {
			strcat(exit_detail_string, " because of DR operation");
		} else if ( (p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->test_run_period_expired ) {
			sprintf(temp_str, " because of run time (%d seconds) is completed", (p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->test_run_period);
			strcat(exit_detail_string, temp_str);
		}

		if( (htxd_ecg_shutdown_flag == FALSE) && (htxd_shutdown_flag == FALSE) ) {
			if ( ( (p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->user_term) ||
				( (p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->DR_term ) ||
				( (p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->test_run_period_expired )   ) {
				htxd_send_message(exit_detail_string, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			} else {
				htxd_send_message(exit_detail_string, 0, HTX_HE_HARD_ERROR, HTX_SYS_MSG);
			}
		}
		p_exer_table[exer_position_in_exer_table].exer_pid = 0;
/*		p_exer_table[exer_position_in_exer_table].exer_addr = NULL;
		p_exer_table[exer_position_in_exer_table].ecg_exer_addr = NULL; */
		(p_exer_table[exer_position_in_exer_table].ecg_exer_addr.HE_addr)->PID = 0;

	}

	return 0;
}



void htxd_display_exer_table(void)
{
	texer_list * p_exer_table;
	int exer_table_length;
	int i;

	p_exer_table = htxd_get_exer_table();
	exer_table_length = htxd_get_exer_table_length();
	printf("[DEBUG] : htxd_display_exer_table() exer_table_length = <%d>\n", exer_table_length); fflush(stdout);

	for(i = 0; i < exer_table_length; i++) {
		printf("[DEBUG] : <=====================================================================\n");
		printf("[DEBUG] : htxd_display_exer_table() index = <%d>\n", i);
		printf("[DEBUG] : htxd_display_exer_table() exer_name = <%s>\n", p_exer_table[i].dev_name);
		printf("[DEBUG] : htxd_display_exer_table() exer_addr = <%p>\n", p_exer_table[i].exer_addr.HE_addr);
		printf("[DEBUG] : htxd_display_exer_table() ecg_exer_addr = <%p>\n", p_exer_table[i].ecg_exer_addr.HE_addr);
		printf("[DEBUG] : htxd_display_exer_table() exer_pid = <%d>\n", p_exer_table[i].exer_pid);
		printf("[DEBUG] : htxd_display_exer_table() ecg_shm_key = <%d>\n", p_exer_table[i].ecg_shm_key);
		printf("[DEBUG] : htxd_display_exer_table() ecg_sem_key = <%d>\n", p_exer_table[i].ecg_sem_key);
		printf("[DEBUG] : htxd_display_exer_table() ecg_semhe_id = <%d>\n", p_exer_table[i].ecg_semhe_id);
		printf("[DEBUG] : htxd_display_exer_table() exer_pos = <%d>\n", p_exer_table[i].exer_pos);
		printf("[DEBUG] : =====================================================================>\n");

		fflush(stdout);
	}
}



int htxd_get_string_value(char *command_string, char *string_value)
{
	int processed_length = 0;
	while(*command_string != ':') {
		*string_value = *command_string;
		string_value++;
		command_string++;
		processed_length++;
	}
	*string_value = '\0';

	return processed_length;
}


int htxd_update_command_object(char *command_string)
{

	char temp_string[512];
	htxd *p_htxd_instance;
	int start_position = 1;

	p_htxd_instance = htxd_get_instance();

	memset(temp_string, 0, sizeof(temp_string));

	start_position += htxd_get_string_value(command_string + start_position, temp_string);
	p_htxd_instance->p_command->command_index = atoi(temp_string);

	start_position++;

	start_position += htxd_get_string_value(command_string + start_position, temp_string);
	if( (strlen(temp_string) > 0) && (strchr(temp_string, '/') == NULL) ) {
		sprintf(p_htxd_instance->p_command->ecg_name, "%s/mdt/%s", global_htx_home_dir, temp_string);
	} else {
		strcpy(p_htxd_instance->p_command->ecg_name, temp_string);
	}

	start_position++;

	strcpy(p_htxd_instance->p_command->option_list, command_string + start_position);
	p_htxd_instance->p_command->option_list[(strlen(command_string + start_position ) - 1)] = 0;  /* deleting the last : symbol */


	return 0;
}


int htxd_get_command_index(void)
{
	htxd *p_htxd_instance;

	p_htxd_instance = htxd_get_instance();

	return p_htxd_instance->p_command->command_index;
}

void htxd_set_command_ecg_name(char *ecg_name)
{
	htxd *p_htxd_instance;

	p_htxd_instance = htxd_get_instance();

	strcpy(p_htxd_instance->p_command->ecg_name, ecg_name);
}

char * htxd_get_command_ecg_name(void)
{
	htxd *p_htxd_instance;

	p_htxd_instance = htxd_get_instance();

	return p_htxd_instance->p_command->ecg_name;
}

char * htxd_get_command_option_list(void)
{
	htxd *p_htxd_instance;

	p_htxd_instance = htxd_get_instance();

	return p_htxd_instance->p_command->option_list;
}


int htxd_get_number_of_running_ecg(void)
{
	int running_ecg_count = 0;

	running_ecg_count = htxd_get_ecg_list_length(htxd_global_instance->p_ecg_manager);

	return running_ecg_count;
}


void htxd_set_system_header_info_shm_id(int system_header_info_shm_id)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	p_ecg_manager->system_header_info_shm_id = system_header_info_shm_id;
}



int htxd_get_system_header_info_error_count(void)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	return p_ecg_manager->system_header_info->error_count;
}



int htxd_get_system_header_info_shm_id(void)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	return p_ecg_manager->system_header_info_shm_id;
}



void htxd_set_exer_table_shm_id(int exer_table_shm_id)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	p_ecg_manager->exer_table_shm_id = exer_table_shm_id;
}



int htxd_get_exer_table_shm_id(void)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	return p_ecg_manager->exer_table_shm_id;
}


tsys_hdr * htxd_get_system_header_info(void)
{
	htxd_ecg_manager *p_ecg_manager;

	p_ecg_manager = htxd_get_ecg_manager();

	return p_ecg_manager->system_header_info;
}


int htxd_get_ecg_sem_id(void)
{
	htxd_ecg_manager *p_ecg_manager;


	p_ecg_manager = htxd_get_ecg_manager();

	return p_ecg_manager->ecg_info_list->ecg_sem_id;
}
