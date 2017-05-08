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
/* @(#)62	1.2  src/htx/usr/lpp/htx/bin/htxd/htxd_process_command.c, htxd, htxubuntu 7/8/15 00:09:13 */



#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "htxd_option_methods.h"
#include "htxd_common.h"
#include "htxd_instance.h"
#include "htxd_util.h"
#include "htxd_trace.h"
#include "htxd_define.h"

#define INDEX_STRING_LENGTH 10



/* process input command and generates output result string */
int htxd_process_command(char **result_string, htxd_command *p_command_object)
{
	int index;
	int return_code;
	int running_ecg_count;
	char *running_ecg_list = NULL;
	char *updated_result = NULL;
	char *temp_result = NULL;
	int alloc_size;
	char *header_line = "Currently running ECG/MDT :";
	char *under_line = "===========================";
	char	trace_str[512];
	int	daemon_state;
	char  daemon_state_string[128];
	char  daemon_state_details[128];


	HTXD_FUNCTION_TRACE(FUN_ENTRY, "htxd_process_command");

	index = p_command_object->command_index; 
	p_command_object->command_type = option_list[index].command_type;

	sprintf(trace_str, "command option string received to process <%s>, command index <%d>", option_list[index].option_string, index);
	HTXD_TRACE(LOG_OFF, trace_str);

	daemon_state = htxd_get_daemon_state();
	if( (daemon_state & option_list[index].command_state) == 0) {
		htxd_get_daemon_state_string(daemon_state, daemon_state_string, daemon_state_details);
		*result_string = malloc(1000);	
		memset(*result_string, 0, 1000);
		sprintf(*result_string, "Daemon state is <%s>:%s,\ncommand option <%s> is not supported at this daemon state", daemon_state_string, daemon_state_details, option_list[index].option_string );
		strcpy(trace_str, *result_string);
		HTXD_TRACE(LOG_ON, trace_str);
		return -1;
	}

	return_code = option_list[index].option_method(&temp_result, p_command_object);

	if (option_list[index].running_ecg_display_flag != 0){
		running_ecg_count = htxd_get_running_ecg_count();
		if(running_ecg_count > 0) {
			alloc_size = running_ecg_count * MAX_ECG_NAME_LENGTH;
			running_ecg_list = malloc(alloc_size); 
			if(running_ecg_list == NULL) {
				sprintf(trace_str, "Error: malloc failed while allocating running_ecg_list, alloc_size is <%d>, malloc failed with errno <%d>", alloc_size, errno);
				HTXD_TRACE(LOG_ON, trace_str);
				exit(-1);
			}

			htxd_get_running_ecg_list(running_ecg_list); 

			alloc_size = (strlen(temp_result) + strlen(running_ecg_list) + 256);
			updated_result = malloc(alloc_size);
			if(updated_result == NULL) {
				sprintf(trace_str, "Error: malloc failed while allocating updated_result, alloc_size is <%d>, malloc failed with errno <%d>", alloc_size, errno);
				HTXD_TRACE(LOG_ON, trace_str);
				exit(-1);
			}

			sprintf(updated_result, "%s %s\n%s\n%s", header_line, running_ecg_list, under_line, temp_result);

		} else {
			alloc_size = (strlen(temp_result) + 256);
			updated_result = malloc(alloc_size);
			if(updated_result == NULL) {
				sprintf(trace_str, "Error: malloc failed while allocating updated_result (else), alloc_size is <%d>, malloc failed with errno <%d>", alloc_size, errno);
				HTXD_TRACE(LOG_ON, trace_str);
				exit(-1);
			}
			sprintf(updated_result, "%s %s\n%s\n%s", header_line, "No MDT/ECG is currently running", under_line, temp_result);
		}

		*result_string = updated_result;
		if(temp_result != NULL) {
			free(temp_result);
		}

		if(running_ecg_list != NULL) {
			free(running_ecg_list);
		}
	} else {
		*result_string = temp_result;
	}

	HTXD_FUNCTION_TRACE(FUN_EXIT, "htxd_process_command");

	return return_code;
}



/* validate the initail requirements for a command */
int htxd_validate_command_requirements(htxd *htxd_instance, char *error_string)
{
	int daemon_state;

	daemon_state = htxd_get_daemon_state();
	if( (daemon_state != HTXD_DAEMON_STATE_RUNNING_MDT) && (daemon_state != HTXD_DAEMON_STATE_SELECTED_MDT)) {
		strcpy(error_string, "No ECG/MDT is currently running");
		return -1;
	}

	if(htxd_instance->p_ecg_manager == NULL) {
		strcpy(error_string, "No ECG/MDT is currently running");
		return -2;
	}

	if(htxd_instance->p_ecg_manager->ecg_info_list == NULL) {
		strcpy(error_string, "No ECG/MDT is currently running");
		return -3;
	}

	return 0;
}


void htxd_init_option_method(htxd_option_method_object *p_query_method, htxd_command *p_command)
{

	p_query_method->htxd_instance = htxd_get_instance();
	strcpy(p_query_method->command_ecg_name, p_command->ecg_name );
	strcpy(p_query_method->command_option_list, p_command->option_list );
	htxd_correct_device_list_for_all_devices(p_query_method->command_option_list);
	p_query_method->error_string[0] = '\0';
	p_query_method->return_code = 0;
	
}
