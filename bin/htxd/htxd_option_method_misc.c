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
/* @(#)56	1.6  src/htx/usr/lpp/htx/bin/htxd/htxd_option_method_misc.c, htxd, htxubuntu 9/15/15 20:28:32 */



#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "htxd.h"
#include "htxd_instance.h"
#include "htxd_ecg.h"
#include "htxd_ipc.h"
#include "htxd_util.h"
#include "htxd_option_methods.h"
#include "htxd_define.h"
#include "htxd_signal.h"
#include "htxd_trace.h"
#include "htxd_define.h"
#include "htxd_common_define.h"

#define SCREEN_MODE_2 2
#define SCREEN_MODE_4 4

extern int htxd_run_HE_script(char *, char *, int *);
extern int htxd_get_ecg_list_length(htxd_ecg_manager *);
extern int htxd_load_exerciser(struct htxshm_HE *);
extern int htxd_get_common_command_result(int, htxd_ecg_info *, char *, char *);


int htxd_expand_device_name_list(htxd_ecg_info *p_ecg_info_list, char *device_name_list)
{
	char temp_device_name_list[MAX_OPTION_LIST_LENGTH];
	char *p_device_name;
	int search_length;
	int i;
	struct htxshm_HE *p_HE;


	temp_device_name_list[0] = '\0';
	p_device_name = strtok(device_name_list, " ");
	
	while(p_device_name != NULL) {
		search_length = strlen(p_device_name) - 1;
		if(p_device_name[search_length] == '*') {
			p_HE = (struct htxshm_HE *)(p_ecg_info_list->ecg_shm_addr.hdr_addr + 1);
			for(i = 0; i < p_ecg_info_list->ecg_shm_exerciser_entries ; i++) {
				if(strncmp(p_HE->sdev_id, p_device_name, search_length) == 0) {
					strcat(temp_device_name_list, p_HE->sdev_id);
					strcat(temp_device_name_list, " ");
				}
				p_HE++;
			}
		} else {
			strcat(temp_device_name_list, p_device_name);
			strcat(temp_device_name_list, " ");
		}
		p_device_name = strtok(NULL, " ");
	}	
	strcpy(device_name_list, temp_device_name_list);

	return 0;
}

int htxd_is_list_regular_expression(char *device_name_list)
{
	char *temp_prt = NULL;

	temp_prt = strchr(device_name_list, '*');
	if(temp_prt != NULL) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int htxd_option_method_getactecg(char **result)
{
	htxd *htxd_instance;
	int active_ecg_count;


	htxd_instance = htxd_get_instance();

	*result = malloc(EXTRA_BUFFER_LENGTH);
	if(*result == NULL) {
		return 1;	
	}
	
	if( htxd_is_daemon_idle() == TRUE) {
		strcpy(*result, "No ECG/MDT is currently running, daemon is idle");
		return 0;
		
	}

	if(htxd_instance->p_ecg_manager->ecg_info_list == NULL) {
		strcpy(*result, "No ECG/MDT is currently running");
		return 0;
	}

	*result[0] = '\0';
		
	active_ecg_count = htxd_get_ecg_list_length(htxd_instance->p_ecg_manager);
	if(active_ecg_count > 0) {
		htxd_get_active_ecg_name_list(htxd_instance->p_ecg_manager, *result);
	}

	return 0;
}



int htxd_get_device_run_status(struct htxshm_HE *p_HE, htxd_ecg_info *p_ecg_info, int device_position, char *status_result)
{

	time_t epoch_time_now;
	int device_sem_status;
	int device_sem_id;


	if(status_result == NULL) {
		return -1;
	}

	device_sem_id = p_ecg_info->ecg_sem_id;
	epoch_time_now =  time( (time_t *) 0);
	
	if( (p_ecg_info->ecg_shm_addr.hdr_addr->started == 0)&& (p_HE->is_child) ) {
			strcpy(status_result, "  ");
	} else if(p_HE->PID == 0) {
		if(p_HE->DR_term == 1) {
			strcpy(status_result, "DT");
		} else if(p_HE->user_term == 0) {
			strcpy(status_result, "DD");
		} else {
			strcpy(status_result, "TM");
		}
	} else if(p_HE->no_of_errs > 0) {
		device_sem_status = htxd_get_device_error_sem_status(device_sem_id, device_position);
		if(device_sem_status == -1) {
			return -1;	
		} else if(device_sem_status == 0) {
			strcpy(status_result, "PR");
		} else {
			strcpy(status_result, "ER");
		}	
	} else if( (p_HE->equaliser_halt == 1) || ( (device_sem_status = htxd_get_device_run_sem_status(device_sem_id, device_position) ) != 0) || (htxd_get_global_activate_halt_sem_status(device_sem_id) == 1) ) {
		strcpy(status_result, "ST");
	} else if( (p_HE->max_cycles != 0) && (p_HE->cycles >= p_HE->max_cycles) ) {
		strcpy(status_result, "CP");
	} else if( (epoch_time_now - p_HE->tm_last_upd) > ( (long)(p_HE->max_run_tm + p_HE->idle_time) ) ) {  ////?????
		strcpy(status_result, "HG");
	} else if( (p_HE->max_cycles != 0 ) && (p_HE->cycles >= p_HE->max_cycles) ) {
		strcpy(status_result, "PR");
	} else {
		strcpy(status_result, "RN");	
	}

	return 0;
}


int htxd_get_device_coe_soe_status(struct htxshm_HE *p_HE, char *status_result)
{

	if(p_HE->cont_on_err == HTX_SOE) {
		strcpy(status_result, "SOE");	
	} else {
		strcpy(status_result, "COE");	
	}

	return 0;
}


int htxd_get_device_last_update_day_time(void)
{

	return 0;
}



int htxd_get_device_active_suspend_status(int device_sem_id, int device_position, char *result_status)
{

	int sem_status;

	sem_status = htxd_get_device_run_sem_status(device_sem_id, device_position);
	if(sem_status == 0) {
		strcpy(result_status, "ACTIVE");
	} else {
		strcpy(result_status, "SUSPEND");
	}

	return 0;
}



int htxd_get_query_row_entry(struct htxshm_HE *p_HE, htxd_ecg_info * p_ecg_info_to_query, int device_position, char *p_query_row_entry)
{
	char device_run_status[5];
	char device_coe_soe_status[5];
	char device_active_suspend_status[15];
	char last_update_day_of_year[10];
	char last_update_time[80];
	char last_error_day_of_year[10];
	char last_error_time[80];

	htxd_get_device_run_status(p_HE, p_ecg_info_to_query, device_position, device_run_status);
	htxd_get_device_active_suspend_status(p_ecg_info_to_query->ecg_sem_id, device_position, device_active_suspend_status);
	htxd_get_device_coe_soe_status(p_HE, device_coe_soe_status);
	htxd_get_time_details(p_HE->tm_last_upd, NULL, last_update_day_of_year, last_update_time);
	if(p_HE->tm_last_err != 0) {
		htxd_get_time_details(p_HE->tm_last_err, NULL, last_error_day_of_year, last_error_time);
	} else {
		strcpy(last_error_day_of_year, "NA");
		strcpy(last_error_time, "NA");
	}

	//sprintf (p_query_row_entry, "%c%-8s%c%3s%c%7s%c%3s%c%s%c%s%c%-8d%c%-3u%c%-4s%c%-8s%c%s%c%d", 
	//sprintf (p_query_row_entry, "%c%-8s%c%3s%c%7s%c%3s%c%s%c%s%c%-8llu%c%-3u%c%-4s%c%-8s%c%s%c%d", 
	sprintf (p_query_row_entry, "%c%-8s%c%3s%c%7s%c%3s%c%s%c%s%c%-8llu%c%-3llu%c%-4s%c%-8s%c%s%c%llu", 
		'\n',
		p_HE->sdev_id, ' ',
		device_run_status, ' ',
		device_active_suspend_status, ' ',
		device_coe_soe_status, ' ',
		last_update_day_of_year, ' ',
		last_update_time, ' ',
		(unsigned long long)p_HE->cycles, ' ',
		(unsigned long long)p_HE->test_id, ' ',
		last_error_day_of_year, ' ',
		last_error_time, ' ',
		p_HE->slot_port, ' ',
		(unsigned long long)p_HE->no_of_errs);


	return 0;
}


int htxd_query_device(htxd_ecg_info *p_ecg_info_to_query, char *p_command_option_list, char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;
	char *p_device_name = NULL;
	char query_row_entry[256];

	if(htxd_is_list_regular_expression(p_command_option_list) == TRUE) {
		htxd_expand_device_name_list(p_ecg_info_to_query, p_command_option_list);
	}

	p_device_name = strtok(p_command_option_list, " ");
	while(p_device_name != NULL) {

		p_HE = (struct htxshm_HE *)(p_ecg_info_to_query->ecg_shm_addr.hdr_addr + 1);
		for(i = 0; i < p_ecg_info_to_query->ecg_shm_exerciser_entries ; i++) {
			if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
				return_code = htxd_get_query_row_entry(p_HE, p_ecg_info_to_query, i, query_row_entry);
				strcat(command_result, query_row_entry);
				break;
			}
			p_HE++;
		}
		p_device_name = strtok(NULL, " ");
	}


	return return_code;
}

int htxd_query_all_device(htxd_ecg_info * p_ecg_info_to_query, char *command_result)
{
	struct htxshm_HE *p_HE;
	int i;
	char query_row_entry[256];
	int return_code = 0;


	p_HE = (struct htxshm_HE *)(p_ecg_info_to_query->ecg_shm_addr.hdr_addr + 1);

	for(i = 0; i < p_ecg_info_to_query->ecg_shm_exerciser_entries ; i++) {
		return_code = htxd_get_query_row_entry(p_HE, p_ecg_info_to_query, i, query_row_entry);
		strcat(command_result, query_row_entry);

		p_HE++;
	}
	
	return return_code;
}


int htxd_option_method_query(char **command_result)
{

	htxd_ecg_info * p_ecg_info_list;
	htxd_ecg_info * p_ecg_info_to_query = NULL;
	htxd_option_method_object query_method;
	int device_entries_present;


	htxd_init_option_method(&query_method);

	query_method.return_code = htxd_validate_command_requirements(query_method.htxd_instance, query_method.error_string);
	if(query_method.return_code != 0) {
		*command_result = malloc(EXTRA_BUFFER_LENGTH);
		strcpy(*command_result, query_method.error_string);
		return query_method.return_code;
	}

	device_entries_present = htxd_get_total_device_count();
	*command_result = malloc(EXTRA_BUFFER_LENGTH + device_entries_present * LINE_ENTRY_LENGTH);
	if(*command_result == NULL) {
		return 1;
	}

	strcpy(*command_result, "--------------------------------------------------------------------------------\n");
	strcat(*command_result, " Device   ST  ACTIVE COE Last Update  Count Stanza Last Error   Slot/  Num_errs\n");
	strcat(*command_result, "             SUSPEND SOE day Time                  Day  Time    Port\n");
	strcat(*command_result, "--------------------------------------------------------------------------------");

	p_ecg_info_list = htxd_get_ecg_info_list(query_method.htxd_instance->p_ecg_manager);
	if(query_method.command_ecg_name[0] != '\0') {
		while(p_ecg_info_list != NULL) {
			if( strcmp(query_method.command_ecg_name, p_ecg_info_list->ecg_name) == 0) {
				p_ecg_info_to_query = p_ecg_info_list;
				break;
			}
			 p_ecg_info_list = p_ecg_info_list->ecg_info_next;
		}

		if(p_ecg_info_to_query == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", query_method.command_ecg_name);
			return 0;
		}

		if(strlen(query_method.command_option_list) > 0) {
			query_method.return_code = htxd_query_device(p_ecg_info_to_query, query_method.command_option_list, *command_result);
		} else {
			query_method.return_code = htxd_query_all_device(p_ecg_info_to_query, *command_result);
		}
	} else {
		if(strlen(query_method.command_option_list) > 0) {
			query_method.return_code = htxd_process_all_active_ecg_device(htxd_query_device, query_method.command_option_list, *command_result);
		} else {
			query_method.return_code = htxd_process_all_active_ecg(htxd_query_all_device, *command_result);
		}
	}

	return query_method.return_code;
}


int htxd_get_status_row_entry(struct htxshm_HE *p_HE, htxd_ecg_info * p_ecg_info_to_status, int device_position, char *p_status_row_entry)
{
	int return_code = 0;
	char device_run_status[5];
	char last_update_day_of_year[10];
	char last_update_time[80];
	char last_error_day_of_year[10];
	char last_error_time[80];


	htxd_get_device_run_status(p_HE, p_ecg_info_to_status, device_position, device_run_status);
	htxd_get_time_details(p_HE->tm_last_upd, NULL, last_update_day_of_year, last_update_time);
	if(p_HE->tm_last_err != 0) {
		htxd_get_time_details(p_HE->tm_last_err, NULL, last_error_day_of_year, last_error_time);
	} else {
		strcpy(last_error_day_of_year, "NA");
		strcpy(last_error_time, "NA");
	}

/*	sprintf (p_status_row_entry, "%c%-3s%c%-9s%c%-5s%c%-9s%c%-6u%c%-7u%c%-5s%c%s", 
		'\n',
		device_run_status, ' ',
		p_HE->sdev_id, ' ',
		last_update_day_of_year, ' ',
		last_update_time, ' ',
		p_HE->cycles, ' ',
		p_HE->test_id, ' ',
		last_error_day_of_year, ' ',
		last_error_time); */
	sprintf (p_status_row_entry, "%c%-3s%c%-9s%c%-5s%c%-9s%c%-6d%c%-7d%c%-5s%c%s", 
		'\n',
		device_run_status, ' ',
		p_HE->sdev_id, ' ',
		last_update_day_of_year, ' ',
		last_update_time, ' ',
		(int)p_HE->cycles, ' ',
		(int)p_HE->test_id, ' ',
		last_error_day_of_year, ' ',
		last_error_time); 
/*	sprintf (p_status_row_entry, "%c%-3s%c%-9s%c%-5s%c%-9s%c%-5s%c%s", 
		'\n',
		device_run_status, ' ',
		p_HE->sdev_id, ' ',
		last_update_day_of_year, ' ',
		last_update_time, ' ',
		last_error_day_of_year, ' ',
		last_error_time); 
*/
	return return_code;

}

int htxd_status_device(htxd_ecg_info * p_ecg_info_to_status, char *p_command_option_list,  char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;
	char *p_device_name = NULL;
	char status_row_entry[512];


	p_device_name = strtok(p_command_option_list, " ");
	while(p_device_name != NULL) {

		p_HE = (struct htxshm_HE *)(p_ecg_info_to_status->ecg_shm_addr.hdr_addr + 1);
		for(i = 0; i < p_ecg_info_to_status->ecg_shm_exerciser_entries ; i++) {
			if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
				htxd_get_status_row_entry(p_HE, p_ecg_info_to_status, i, status_row_entry);
				strcat(command_result, status_row_entry);
				break;
			}
			p_HE++;
		}
		p_device_name = strtok(NULL, " ");
	}

	return return_code;
}


int htxd_status_all_device(htxd_ecg_info * p_ecg_info_to_status, char *command_result)
{
	struct htxshm_HE *p_HE;
	int i;
	char status_row_entry[256];


	p_HE = (struct htxshm_HE *)(p_ecg_info_to_status->ecg_shm_addr.hdr_addr + 1);

	for(i = 0; i < p_ecg_info_to_status->ecg_shm_exerciser_entries ; i++) {
		htxd_get_status_row_entry(p_HE, p_ecg_info_to_status, i, status_row_entry);
		strcat(command_result, status_row_entry);

		p_HE++;
	}
	
	return 0;
}


int htxd_option_method_status(char **command_result)
{

	htxd_ecg_info * p_ecg_info_list;
	htxd_option_method_object status_method;
	int device_entries_present;

	htxd_init_option_method(&status_method);

	status_method.return_code = htxd_validate_command_requirements(status_method.htxd_instance, status_method.error_string);
	if(status_method.return_code != 0) {
		*command_result = malloc(EXTRA_BUFFER_LENGTH);
		strcpy(*command_result, status_method.error_string);
		return status_method.return_code;
	}

	device_entries_present = htxd_get_total_device_count();	
	*command_result = malloc(EXTRA_BUFFER_LENGTH + device_entries_present * LINE_ENTRY_LENGTH);
	if(*command_result == NULL) {
		return 1; 
	}


	strcpy(*command_result, "---------------------------------------------------------\n");
	strcat(*command_result, "              Last  Update    Cycle  Curr    Last  Error\n");
	strcat(*command_result, "ST  Device    Day   Time      Count  Stanza  Day   Time \n");
	strcat(*command_result, "---------------------------------------------------------");

	if(status_method.command_ecg_name[0] != '\0') {
		p_ecg_info_list = htxd_get_ecg_info_node(status_method.htxd_instance->p_ecg_manager, status_method.command_ecg_name);	
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", status_method.command_ecg_name);
			return -1;
		}

		if(strlen(status_method.command_option_list) > 0) {
			status_method.return_code = htxd_status_device(p_ecg_info_list, status_method.command_option_list, *command_result);	
		} else {
			status_method.return_code = htxd_status_all_device(p_ecg_info_list, *command_result);	
		}
	} else {
		if(strlen(status_method.command_option_list) > 0) {
			status_method.return_code = htxd_process_all_active_ecg_device(htxd_status_device, status_method.command_option_list, *command_result);
		} else {
			status_method.return_code = htxd_process_all_active_ecg(htxd_status_all_device, *command_result);
		}
	}

	return status_method.return_code;
}



int htxd_option_method_refresh(char **command_result)
{

	htxd *htxd_instance;
	int current_ecg_list_length;
	int result = 0;

	htxd_instance = htxd_get_instance();	

	*command_result = malloc(EXTRA_BUFFER_LENGTH);
	if(*command_result == NULL) {
		return 1;
	}
	current_ecg_list_length = htxd_get_ecg_list_length(htxd_instance->p_ecg_manager);
	if(current_ecg_list_length > 0) {
		sprintf(*command_result, "command failed: <%d> MDT(s) are running currently, stop all MDTs before system refresh", current_ecg_list_length);
		return 0;
	}

	result = htxd_reload_htx_profile(&(htxd_instance->p_profile) );


	if(result == 0) {	
		strcpy(*command_result, "system refresh is completed successfully");
	} else {
		 strcpy(*command_result, "refresh command failed");
	}

	return result;
}


int htxd_activate_device(htxd_ecg_info *p_ecg_info_to_activate, char *p_command_option_list, char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;
	char *p_device_name = NULL;


	if(htxd_is_list_regular_expression(p_command_option_list) == TRUE) {
		htxd_expand_device_name_list(p_ecg_info_to_activate, p_command_option_list);
	}

	p_device_name = strtok(p_command_option_list, " ");
	while(p_device_name != NULL) {

		p_HE = (struct htxshm_HE *)(p_ecg_info_to_activate->ecg_shm_addr.hdr_addr + 1);
		for(i = 0; i < p_ecg_info_to_activate->ecg_shm_exerciser_entries ; i++) {
			if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
				htxd_set_device_run_sem_status(p_ecg_info_to_activate->ecg_sem_id, i, 0);
				break;
			}
			p_HE++;
		}
		p_device_name = strtok(NULL, " ");
	}

	return return_code;
}



int htxd_activate_all_device(htxd_ecg_info * p_ecg_info_to_activate, char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;


	p_HE = (struct htxshm_HE *)(p_ecg_info_to_activate->ecg_shm_addr.hdr_addr + 1);

	for(i = 0; i < p_ecg_info_to_activate->ecg_shm_exerciser_entries ; i++) {
		htxd_set_device_run_sem_status(p_ecg_info_to_activate->ecg_sem_id, i, 0);
	}

	return return_code;
}



int htxd_option_method_activate(char **command_result)
{
	htxd_ecg_info *p_ecg_info_list = NULL;
	htxd_option_method_object activate_method;
	char *temp_option_list = NULL;	
	int option_list_length;
	int device_entries_present;


	htxd_init_option_method(&activate_method);

	activate_method.return_code = htxd_validate_command_requirements(activate_method.htxd_instance, activate_method.error_string);
	if(activate_method.return_code != 0) {
		*command_result = malloc(EXTRA_BUFFER_LENGTH);
		strcpy(*command_result, activate_method.error_string);
		return activate_method.return_code;
	}

	device_entries_present = htxd_get_total_device_count();
	*command_result = malloc(EXTRA_BUFFER_LENGTH + device_entries_present * LINE_ENTRY_LENGTH);
	if(*command_result == NULL) {
		return 1;
	}

	option_list_length = strlen(activate_method.command_option_list);
	if(option_list_length > 0) {
		temp_option_list = malloc(option_list_length + STRING_EXTRA_SPACE);
		if(temp_option_list == NULL) {
			return 1;
		}
		strcpy(temp_option_list, activate_method.command_option_list);
	}

	if(activate_method.command_ecg_name[0] != '\0') {
		p_ecg_info_list = htxd_get_ecg_info_node(activate_method.htxd_instance->p_ecg_manager, activate_method.command_ecg_name);	
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", activate_method.command_ecg_name);
			return -1;
		}

		if(option_list_length > 0) {
			activate_method.return_code = htxd_activate_device(p_ecg_info_list, activate_method.command_option_list, *command_result);	
			strcpy(temp_option_list, activate_method.command_option_list);
		} else {
			activate_method.return_code = htxd_activate_all_device(p_ecg_info_list, *command_result);	
		}
	} else {
		if(option_list_length > 0) {
			activate_method.return_code = htxd_process_all_active_ecg_device(htxd_activate_device, activate_method.command_option_list, *command_result);
			strcpy(temp_option_list, activate_method.command_option_list);
		} else {
			activate_method.return_code = htxd_process_all_active_ecg(htxd_activate_all_device, *command_result);
		}
	}

	htxd_get_common_command_result(ACTIVE_SUSPEND_STATE, p_ecg_info_list, temp_option_list, *command_result);

	if(temp_option_list != NULL) {
		free(temp_option_list);
	}

	return activate_method.return_code;

}




int htxd_suspend_device(htxd_ecg_info *p_ecg_info_to_suspend, char *p_command_option_list, char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;
	char *p_device_name = NULL;


	if(htxd_is_list_regular_expression(p_command_option_list) == TRUE) {
		htxd_expand_device_name_list(p_ecg_info_to_suspend, p_command_option_list);
	}	

	p_device_name = strtok(p_command_option_list, " ");
	while(p_device_name != NULL) {

		p_HE = (struct htxshm_HE *)(p_ecg_info_to_suspend->ecg_shm_addr.hdr_addr + 1);
		for(i = 0; i < p_ecg_info_to_suspend->ecg_shm_exerciser_entries ; i++) {
			if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
				htxd_set_device_run_sem_status(p_ecg_info_to_suspend->ecg_sem_id, i, 1);
				break;
			}
			p_HE++;
		}
		p_device_name = strtok(NULL, " ");
	}

	return return_code;
}

int htxd_suspend_all_device(htxd_ecg_info * p_ecg_info_to_suspend, char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;


	p_HE = (struct htxshm_HE *)(p_ecg_info_to_suspend->ecg_shm_addr.hdr_addr + 1);

	for(i = 0; i < p_ecg_info_to_suspend->ecg_shm_exerciser_entries ; i++) {
		htxd_set_device_run_sem_status(p_ecg_info_to_suspend->ecg_sem_id, i, 1);
	}

	return return_code;
}



int htxd_option_method_suspend(char **command_result)
{

	htxd_ecg_info *p_ecg_info_list = NULL;
	htxd_option_method_object suspend_method;
	char temp_option_list[MAX_OPTION_LIST_LENGTH];
	int option_list_length;
	int device_entries_present;


	htxd_init_option_method(&suspend_method);

	suspend_method.return_code = htxd_validate_command_requirements(suspend_method.htxd_instance, suspend_method.error_string);
	if(suspend_method.return_code != 0) {
		*command_result = malloc(EXTRA_BUFFER_LENGTH);
		strcpy(*command_result, suspend_method.error_string);
		return suspend_method.return_code;
	}

	device_entries_present = htxd_get_total_device_count();
	*command_result = malloc(EXTRA_BUFFER_LENGTH + device_entries_present * LINE_ENTRY_LENGTH);
	if(*command_result == NULL) {
		return 1;
	}

	option_list_length = strlen(suspend_method.command_option_list);
	if(option_list_length > 0) {
		strcpy(temp_option_list, suspend_method.command_option_list);
	}

	if(suspend_method.command_ecg_name[0] != '\0') {
		p_ecg_info_list = htxd_get_ecg_info_node(suspend_method.htxd_instance->p_ecg_manager, suspend_method.command_ecg_name);	
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", suspend_method.command_ecg_name);
			return -1;
		}

		if( option_list_length > 0) {
			suspend_method.return_code = htxd_suspend_device(p_ecg_info_list, suspend_method.command_option_list, *command_result);	
		} else {
			suspend_method.return_code = htxd_suspend_all_device(p_ecg_info_list, *command_result);	
		}
	} else {
		if( option_list_length > 0) {
			suspend_method.return_code = htxd_process_all_active_ecg_device(htxd_suspend_device, suspend_method.command_option_list, *command_result);
		} else {
			suspend_method.return_code = htxd_process_all_active_ecg(htxd_suspend_all_device, *command_result);
		}
	}

	htxd_get_common_command_result(ACTIVE_SUSPEND_STATE, p_ecg_info_list, temp_option_list, *command_result);

	return suspend_method.return_code;

}


int htxd_set_device_terminate_status(htxd_ecg_info *p_ecg_info_to_terminate, int device_position)
{

	struct htxshm_HE *p_HE;	
	int save_run_status;
	int is_pause = 0;
	int return_code = 0;
	int kill_errno;
	pid_t HE_pid;
	char regular_expression[256];
	int dummy;

	
	p_HE = (struct htxshm_HE *)(p_ecg_info_to_terminate->ecg_shm_addr.hdr_addr + 1);
	p_HE += device_position;


	HE_pid = p_HE->PID;
	if( (HE_pid != 0) && (p_HE->tm_last_upd != 0) ) {
		save_run_status = htxd_get_device_run_sem_status(p_ecg_info_to_terminate->ecg_sem_id, device_position);
		htxd_set_device_run_sem_status(p_ecg_info_to_terminate->ecg_sem_id, device_position, 0);
		htxd_set_device_error_sem_status(p_ecg_info_to_terminate->ecg_sem_id, device_position, 0);
		
		p_HE->user_term = 1;
		p_HE->max_cycles = 0;

		alarm(htxd_get_slow_shutdown_wait());
		

		if( getpid() == getpgid(HE_pid) ) {
			is_pause = 1;	
		}
	
		return_code = htxd_send_SIGTERM(HE_pid); 
		kill_errno = errno;
		if(return_code == 0) {
			if(is_pause == 1) {
				pause();
			} else {
				/* external process death */
				HTXD_TRACE(LOG_OFF, "NOT calling pause()");
			}
			if(!alarm (0)) {
				htxd_send_SIGKILL(HE_pid);
			}
			sleep(1);
			strcpy (regular_expression, "^");
			strcat (regular_expression, p_HE->HE_name);
			strcat (regular_expression, ".*cleanup[\t ]*$");
			return_code = htxd_run_HE_script(regular_expression, p_HE->sdev_id, &dummy);
		} else {
			alarm ((unsigned) 0);
			p_HE->user_term = 0;
			p_HE->DR_term = 0;

			if (kill_errno == ESRCH) {
				printf("No such process.  Perhaps it is already deceased?\n");
			} else {
				printf("kill return error <%d>\n", kill_errno);
			}
			return_code = -1;
		}
		
		htxd_set_device_run_sem_status(p_ecg_info_to_terminate->ecg_sem_id, device_position, save_run_status);
	}

	
	return return_code;
}



int htxd_terminate_device(htxd_ecg_info *p_ecg_info_to_terminate, char *p_command_option_list, char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;
	char *p_device_name = NULL;


	p_device_name = strtok(p_command_option_list, " ");
	while(p_device_name != NULL) {

		p_HE = (struct htxshm_HE *)(p_ecg_info_to_terminate->ecg_shm_addr.hdr_addr + 1);
		for(i = 0; i < p_ecg_info_to_terminate->ecg_shm_exerciser_entries ; i++) {
			if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
				return_code = htxd_set_device_terminate_status(p_ecg_info_to_terminate, i);
				break;
			}
			p_HE++;
		}
		p_device_name = strtok(NULL, " ");
	}

	return return_code;
}



int htxd_terminate_all_device(htxd_ecg_info *p_ecg_info_to_terminate, char *command_result)
{

	int return_code = 0;
	int i;

	for(i = 0; i < p_ecg_info_to_terminate->ecg_shm_exerciser_entries ; i++) {
		htxd_set_device_terminate_status(p_ecg_info_to_terminate, i);
	}

	htxd_display_exer_table();
	htxd_display_ecg_info_list();

	return return_code;

}


int htxd_option_method_terminate(char **command_result)
{

	htxd_ecg_info *p_ecg_info_list = NULL;
	htxd_option_method_object terminate_method;
	char *temp_option_list = NULL;
	int option_list_length;
	int device_entries_present;


	htxd_init_option_method(&terminate_method);

	terminate_method.return_code = htxd_validate_command_requirements(terminate_method.htxd_instance, terminate_method.error_string);
	if(terminate_method.return_code != 0) {
		*command_result = malloc(EXTRA_BUFFER_LENGTH);
		strcpy(*command_result, terminate_method.error_string);
		return terminate_method.return_code;
	}

	device_entries_present = htxd_get_total_device_count();
	*command_result = malloc(EXTRA_BUFFER_LENGTH + device_entries_present * LINE_ENTRY_LENGTH);
	if(*command_result == NULL) {
		return 1;
	}

	option_list_length = strlen(terminate_method.command_option_list);
	if(option_list_length > 0) {
		temp_option_list = malloc(option_list_length + STRING_EXTRA_SPACE);
		if(temp_option_list == NULL) {
			return 1;
		}
		strcpy(temp_option_list, terminate_method.command_option_list);
	}

	if(terminate_method.command_ecg_name[0] != '\0') {
		p_ecg_info_list = htxd_get_ecg_info_node(terminate_method.htxd_instance->p_ecg_manager, terminate_method.command_ecg_name);	
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", terminate_method.command_ecg_name);
			return -1;
		}

		if(option_list_length > 0) {
			terminate_method.return_code = htxd_terminate_device(p_ecg_info_list, terminate_method.command_option_list, *command_result);	
			strcpy(temp_option_list, terminate_method.command_option_list);

		} else {
			terminate_method.return_code = htxd_terminate_all_device(p_ecg_info_list, *command_result);	
		}
	} else {
		if(option_list_length > 0) {
			terminate_method.return_code = htxd_process_all_active_ecg_device(htxd_terminate_device, terminate_method.command_option_list, *command_result);
			strcpy(temp_option_list, terminate_method.command_option_list);
		} else {
			terminate_method.return_code = htxd_process_all_active_ecg(htxd_terminate_all_device, *command_result);
		}
	}

	htxd_get_common_command_result(RUNNING_STATE, p_ecg_info_list, temp_option_list, *command_result);

	if(temp_option_list != NULL) {
		free(temp_option_list);
	}

	return terminate_method.return_code;
}

int htxd_set_device_restart_status(htxd_ecg_info *p_ecg_info_to_restart, int device_position)
{

	struct htxshm_HE *p_HE;
	char regular_expression[256];
	int dummy;


	p_HE = (struct htxshm_HE *)(p_ecg_info_to_restart->ecg_shm_addr.hdr_addr + 1);
	p_HE += device_position;	

	htxd_set_FD_close_on_exec_flag();		

	strcpy(regular_expression, "^");
	strcpy(regular_expression, p_HE->HE_name);
	strcpy(regular_expression, ".*setup[\t ]*$");
	htxd_run_HE_script(regular_expression,  p_HE->sdev_id, &dummy);
	
	htxd_load_exerciser(p_HE);	


	htxd_reset_FD_close_on_exec_flag();		

	return 0;
}




int htxd_restart_device(htxd_ecg_info *p_ecg_info_to_restart, char *p_command_option_list, char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;
	char *p_device_name = NULL;


	p_device_name = strtok(p_command_option_list, " ");
	while(p_device_name != NULL) {

		p_HE = (struct htxshm_HE *)(p_ecg_info_to_restart->ecg_shm_addr.hdr_addr + 1);
		for(i = 0; i < p_ecg_info_to_restart->ecg_shm_exerciser_entries ; i++) {
			if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
				return_code = htxd_set_device_restart_status(p_ecg_info_to_restart, i);
				break;
			}
			p_HE++;
		}
		p_device_name = strtok(NULL, " ");
	}

	return return_code;
}




int htxd_restart_all_device(htxd_ecg_info * p_ecg_info_to_restart, char *command_result)
{

	int return_code = 0;
	int i;

	for(i = 0; i < p_ecg_info_to_restart->ecg_shm_exerciser_entries ; i++) {
		htxd_set_device_restart_status(p_ecg_info_to_restart, i);
	}

	return return_code;

}

int htxd_option_method_restart(char **command_result)
{

	htxd_ecg_info *p_ecg_info_list = NULL;
	htxd_option_method_object restart_method;
	char *temp_option_list = NULL;
	int option_list_length;
	int device_entries_present;


	htxd_init_option_method(&restart_method);

	restart_method.return_code = htxd_validate_command_requirements(restart_method.htxd_instance, restart_method.error_string);
	if(restart_method.return_code != 0) {
		*command_result = malloc(EXTRA_BUFFER_LENGTH);
		strcpy(*command_result, restart_method.error_string);
		return restart_method.return_code;
	}

	device_entries_present = htxd_get_total_device_count();
	*command_result = malloc(EXTRA_BUFFER_LENGTH + device_entries_present * LINE_ENTRY_LENGTH);
	if(*command_result == NULL) {
		return 1;
	}

	option_list_length = strlen(restart_method.command_option_list);
	if(option_list_length > 0) {
		temp_option_list = malloc(option_list_length + STRING_EXTRA_SPACE);
		if(temp_option_list == NULL) {
			return 1;
		}
		strcpy(temp_option_list, restart_method.command_option_list);
	}

	if(restart_method.command_ecg_name[0] != '\0') {
		p_ecg_info_list = htxd_get_ecg_info_node(restart_method.htxd_instance->p_ecg_manager, restart_method.command_ecg_name);	
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", restart_method.command_ecg_name);
			return -1;
		}

		if(option_list_length > 0) {
			restart_method.return_code = htxd_restart_device(p_ecg_info_list, restart_method.command_option_list, *command_result);	
			strcpy(temp_option_list, restart_method.command_option_list);
		} else {
			restart_method.return_code = htxd_restart_all_device(p_ecg_info_list, *command_result);	
		}
	} else {
		if(option_list_length > 0) {
			htxd_process_all_active_ecg_device(htxd_restart_device, restart_method.command_option_list, *command_result);
			strcpy(temp_option_list, restart_method.command_option_list);
		} else {
			restart_method.return_code = htxd_process_all_active_ecg(htxd_restart_all_device, *command_result);
		}
	}

	htxd_get_common_command_result(RUNNING_STATE, p_ecg_info_list, temp_option_list, *command_result);

	if(temp_option_list != NULL) {
		free(temp_option_list);
	}


	return restart_method.return_code;
}



int htxd_coe_device(htxd_ecg_info *p_ecg_info_to_coe, char *p_command_option_list, char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;
	char *p_device_name = NULL;


	if(htxd_is_list_regular_expression(p_command_option_list) == TRUE) {
		htxd_expand_device_name_list(p_ecg_info_to_coe, p_command_option_list);
	}

	p_device_name = strtok(p_command_option_list, " ");
	while(p_device_name != NULL) {

		p_HE = (struct htxshm_HE *)(p_ecg_info_to_coe->ecg_shm_addr.hdr_addr + 1);
		for(i = 0; i < p_ecg_info_to_coe->ecg_shm_exerciser_entries ; i++) {
			if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
				p_HE->cont_on_err = HTX_COE;
				break;
			}
			p_HE++;
		}
		p_device_name = strtok(NULL, " ");
	}

	return return_code;
}




int htxd_coe_all_device(htxd_ecg_info * p_ecg_info_to_coe, char *command_result)
{

	int return_code = 0;
	int i;
	struct htxshm_HE *p_HE;

	p_HE = (struct htxshm_HE *)(p_ecg_info_to_coe->ecg_shm_addr.hdr_addr + 1);

	for(i = 0; i < p_ecg_info_to_coe->ecg_shm_exerciser_entries ; i++) {
		p_HE->cont_on_err = HTX_COE;
		p_HE++;
	}

	return return_code;

}



int htxd_option_method_coe(char **command_result)
{

	htxd_ecg_info * p_ecg_info_list = NULL;
	htxd_option_method_object coe_method;
	char *temp_option_list = NULL;
	int option_list_length;
	int device_entries_present;


	htxd_init_option_method(&coe_method);

	coe_method.return_code = htxd_validate_command_requirements(coe_method.htxd_instance, coe_method.error_string);
	if(coe_method.return_code != 0) {
		*command_result = malloc(EXTRA_BUFFER_LENGTH);
		strcpy(*command_result, coe_method.error_string);
		return coe_method.return_code;
	}

	device_entries_present = htxd_get_total_device_count();
	*command_result = malloc(EXTRA_BUFFER_LENGTH + device_entries_present * LINE_ENTRY_LENGTH);
	if(*command_result == NULL) {
		return 1;
	}

	option_list_length = strlen(coe_method.command_option_list);

	if(option_list_length > 0) {
		temp_option_list = malloc(option_list_length + STRING_EXTRA_SPACE);	
		if(temp_option_list == NULL) {
			return 1;
		}
		strcpy(temp_option_list, coe_method.command_option_list);
	}

	if(coe_method.command_ecg_name[0] != '\0') {
		p_ecg_info_list = htxd_get_ecg_info_node(coe_method.htxd_instance->p_ecg_manager, coe_method.command_ecg_name);	
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", coe_method.command_ecg_name);
			return -1;
		}

		if( option_list_length > 0) {
			coe_method.return_code = htxd_coe_device(p_ecg_info_list, temp_option_list, *command_result);	
			strcpy(temp_option_list, coe_method.command_option_list);
		} else {
			coe_method.return_code = htxd_coe_all_device(p_ecg_info_list, *command_result);	
		}
	} else {
		if(option_list_length > 0) {
			strcpy(temp_option_list, coe_method.command_option_list);
			coe_method.return_code = htxd_process_all_active_ecg_device(htxd_coe_device, temp_option_list, *command_result);
		} else {
			coe_method.return_code = htxd_process_all_active_ecg(htxd_coe_all_device, *command_result);
		}
	}

	htxd_get_common_command_result(COE_SOE_STATE, p_ecg_info_list, temp_option_list, *command_result); 

	if(temp_option_list != NULL) {
		free(temp_option_list);
	}		

	return coe_method.return_code;
}



int htxd_soe_device(htxd_ecg_info *p_ecg_info_to_soe, char *p_command_option_list, char *command_result)
{
	struct htxshm_HE *p_HE;
	int return_code = 0;
	int i;
	char *p_device_name = NULL;


	if(htxd_is_list_regular_expression(p_command_option_list) == TRUE) {
		htxd_expand_device_name_list(p_ecg_info_to_soe, p_command_option_list);
	}

	p_device_name = strtok(p_command_option_list, " ");
	while(p_device_name != NULL) {

		p_HE = (struct htxshm_HE *)(p_ecg_info_to_soe->ecg_shm_addr.hdr_addr + 1);
		for(i = 0; i < p_ecg_info_to_soe->ecg_shm_exerciser_entries ; i++) {

			if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
				p_HE->cont_on_err = HTX_SOE;
				break;
			}
			p_HE++;
		}
		p_device_name = strtok(NULL, " ");
	}

	return return_code;
}



int htxd_soe_all_device(htxd_ecg_info * p_ecg_info_to_soe, char *command_result)
{

	int return_code = 0;
	int i;
	struct htxshm_HE *p_HE;

	p_HE = (struct htxshm_HE *)(p_ecg_info_to_soe->ecg_shm_addr.hdr_addr + 1);

	for(i = 0; i < p_ecg_info_to_soe->ecg_shm_exerciser_entries ; i++) {
		p_HE->cont_on_err = HTX_SOE;
		p_HE++;
	}

	return return_code;

}




int htxd_option_method_soe(char **command_result)
{

	htxd_ecg_info * p_ecg_info_list = NULL;
	htxd_option_method_object soe_method;
	char *temp_option_list = NULL;
	int option_list_length;
	int device_entries_present;


	htxd_init_option_method(&soe_method);

	soe_method.return_code = htxd_validate_command_requirements(soe_method.htxd_instance, soe_method.error_string);
	if(soe_method.return_code != 0) {
		*command_result = malloc(EXTRA_BUFFER_LENGTH);
		strcpy(*command_result, soe_method.error_string);
		return soe_method.return_code;
	}

	device_entries_present = htxd_get_total_device_count();
	*command_result = malloc(EXTRA_BUFFER_LENGTH + device_entries_present * LINE_ENTRY_LENGTH);
	if(*command_result == NULL) {
		return 1;
	}

	option_list_length = strlen(soe_method.command_option_list);
	if(option_list_length > 0) {
		temp_option_list = malloc(option_list_length + STRING_EXTRA_SPACE);
		if(temp_option_list == NULL) {
			return 1;
		}
		strcpy(temp_option_list, soe_method.command_option_list);
	}

	if(soe_method.command_ecg_name[0] != '\0') {
		p_ecg_info_list = htxd_get_ecg_info_node(soe_method.htxd_instance->p_ecg_manager, soe_method.command_ecg_name);	
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", soe_method.command_ecg_name);
			return -1;
		}

		if(option_list_length > 0) {
			soe_method.return_code = htxd_soe_device(p_ecg_info_list, temp_option_list, *command_result);	
			strcpy(temp_option_list, soe_method.command_option_list);
		} else {
			soe_method.return_code = htxd_soe_all_device(p_ecg_info_list, *command_result);	
		}
	} else {
		if(option_list_length > 0) {
			soe_method.return_code = htxd_process_all_active_ecg_device(htxd_soe_device, temp_option_list, *command_result);
			strcpy(temp_option_list, soe_method.command_option_list);
		} else {
			soe_method.return_code = htxd_process_all_active_ecg(htxd_soe_all_device, *command_result);
		}
	}

	htxd_get_common_command_result(COE_SOE_STATE, p_ecg_info_list, temp_option_list, *command_result);
	if(temp_option_list != NULL) {
		free(temp_option_list);
	}

	return soe_method.return_code;
}


void htxd_bootme_error_string(int error_code, char ** result_string)
{
	switch(error_code) {
	case 2:
		sprintf(*result_string, "Error: Not sufficient space, failed to start bootme");
		break;
	case 11:
		sprintf(*result_string, "Error: Please check the value of REBOOT in %s/rules/reg/bootme/default, failed to start bootme", global_htx_home_dir);
		break;
	case 12:
		sprintf(*result_string, "Error: Please check the value of BOOT_CMD in %s/rules/reg/bootme/default, failed to start bootme", global_htx_home_dir);
		break;
	case 13:
		sprintf(*result_string, "Error: Please check the value of BOOT_WAIT in %s/rules/reg/bootme/default, failed to start bootme", global_htx_home_dir);
		break;
	case 21:
		sprintf(*result_string, "bootme is already on");
		break;
	case 31:
		sprintf(*result_string, "bootme flag file <%s/.htxd_autostart> was missing", global_htx_home_dir);
		break;
	case 32:
		sprintf(*result_string, "bootme is already off");
		break;
	case 41:
		sprintf(*result_string, "bootme status : off");
		break;
	case 42:
		sprintf(*result_string, "bootme status : inconsistant state, bootme cron entry is present, bootme flag file <%s/.htxd_autostart> was missing", global_htx_home_dir);
		break;
	case 0:
		break;
	default: 
		sprintf(*result_string, "Error: unknown error from bootme, error code = <%d>", error_code);
		break;	
	}
}


int htxd_option_method_bootme(char **command_result)
{

	htxd_option_method_object bootme_method;
	char *temp_option_list = NULL;
	int option_list_length;
	char trace_string[256];
	FILE *p_boot_flag;
	int bootme_status;
	char *running_mdt_name;
	int bootme_return_code;
	char temp_string[300];


	htxd_init_option_method(&bootme_method);

	*command_result = malloc( 2 * 1024);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return 1;
	}

	option_list_length = strlen(bootme_method.command_option_list);
	if(option_list_length > 0) {
		temp_option_list = malloc(option_list_length + STRING_EXTRA_SPACE);
		if(temp_option_list == NULL) {
			sprintf(trace_string, "temp_option_list: malloc failed with errno = <%d>", errno);
			HTXD_TRACE(LOG_ON, trace_string);
			return 1;
		}
		strcpy(temp_option_list, bootme_method.command_option_list);
		
		if( strcmp(temp_option_list, "on") == 0 ) {
			bootme_method.return_code = htxd_validate_command_requirements(bootme_method.htxd_instance, bootme_method.error_string);
			if(bootme_method.return_code != 0) {
				strcpy(*command_result, bootme_method.error_string);

				if(temp_option_list != NULL) {
					free(temp_option_list);
				}

				return bootme_method.return_code;
			}
			sprintf(trace_string, "%s/etc/scripts/%s on", global_htx_home_dir, HTXD_BOOTME_SCRIPT);
			bootme_status = system(trace_string);
			bootme_return_code = WEXITSTATUS(bootme_status);
			if(bootme_return_code == 0) {
				running_mdt_name = htxd_get_running_ecg_name();
				sprintf(temp_string, "%s/%s", global_htx_home_dir, HTXD_AUTOSTART_FILE);
				p_boot_flag = fopen(temp_string, "w");
				if(p_boot_flag == NULL) {
					sprintf(trace_string, "fopen failed with errno = <%d>", errno);
					HTXD_TRACE(LOG_ON, trace_string);
					sprintf(*command_result, "bootme on failed, could not set bootme flag file <%s>", temp_string);
					sprintf(trace_string, "%s/etc/scripts/%s off", global_htx_home_dir, HTXD_BOOTME_SCRIPT);
					system(trace_string);
				} else {
					fprintf(p_boot_flag, "%s", running_mdt_name);
					fclose(p_boot_flag);
					strcpy(*command_result, "bootme on is completed successfully");
					sprintf(trace_string, "cp %s %s-htxd_bootme", running_mdt_name, running_mdt_name);
					system(trace_string);	
				}
			} else {
				htxd_bootme_error_string(bootme_return_code, command_result);
			}

		} else if(  strcmp(temp_option_list, "off") == 0 ) {
			sprintf(trace_string, "%s/etc/scripts/%s off", global_htx_home_dir, HTXD_BOOTME_SCRIPT);
			bootme_status = system(trace_string);
			bootme_return_code = WEXITSTATUS(bootme_status);
			if(bootme_return_code == 0) {
				strcpy(*command_result, "bootme off is completed successfully");
				sprintf(trace_string, "rm -f %s/mdt/*-htxd_bootme >/dev/null 2>&1", global_htx_home_dir);
				system(trace_string);
			} else {
				htxd_bootme_error_string(bootme_return_code, command_result);
			}
		} else if( strcmp(temp_option_list, "status") == 0 ) {
			sprintf(trace_string, "%s/etc/scripts/%s status", global_htx_home_dir, HTXD_BOOTME_SCRIPT);
			bootme_status = system(trace_string);
			bootme_return_code = WEXITSTATUS(bootme_status);
			if(bootme_return_code == 0) {
				strcpy(*command_result, "bootme status: on");
			} else {
				htxd_bootme_error_string(bootme_return_code, command_result);	
			}
		} else {
			strcpy(*command_result, "Error: bootme unknow option\nUsage: bootme (on|off|status)");
		}
	} else {
		strcpy(*command_result, "Error: invalid bootme command\nUsage: bootme (on|off|status)");
	}
 

	if(temp_option_list != NULL) {
		free(temp_option_list);
	}

	return 0;
}



int htxd_option_method_exersetupinfo(char **command_result)
{

	htxd_ecg_info * p_ecg_info_list = NULL;
	htxd_option_method_object exersetupinfo_method;
	struct htxshm_HE *p_HE;
	int exer_setup_info_flag = 1;
	int i;
	char trace_string[256];


	htxd_init_option_method(&exersetupinfo_method);
	*command_result = malloc(EXTRA_BUFFER_LENGTH);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	exersetupinfo_method.return_code = htxd_validate_command_requirements(exersetupinfo_method.htxd_instance, exersetupinfo_method.error_string);
	if(exersetupinfo_method.return_code != 0) {
		strcpy(*command_result, exersetupinfo_method.error_string);
		return exersetupinfo_method.return_code;
	}

	if(exersetupinfo_method.command_ecg_name[0] != '\0') {
		p_ecg_info_list = htxd_get_ecg_info_node(exersetupinfo_method.htxd_instance->p_ecg_manager, exersetupinfo_method.command_ecg_name);	
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", exersetupinfo_method.command_ecg_name);
			return -1;
		}
	} else {
		p_ecg_info_list = htxd_get_ecg_info_node(exersetupinfo_method.htxd_instance->p_ecg_manager, exersetupinfo_method.htxd_instance->p_ecg_manager->running_ecg_name);
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Internal Error: Failed to get a running ecg<%s>", exersetupinfo_method.command_ecg_name);
			return -1;
		}
	}

	p_HE = (struct htxshm_HE *)(p_ecg_info_list->ecg_shm_addr.hdr_addr + 1);
	for(i = 0; i < p_ecg_info_list->ecg_shm_exerciser_entries ; i++) {
		if(p_HE->upd_test_id == 0 ) {
			exer_setup_info_flag = 0;
			break;
		}
		p_HE++;
	}

	sprintf(*command_result, "setup_of_all_exers_done = %d", exer_setup_info_flag);


	return 0;
}



int htxd_getstats_ecg(htxd_ecg_info * p_ecg_info_to_getstats, char **command_result)
{

	int return_code = 0;
	int  htx_stats_pid;
	char temp_string[300];

	/* get HTX stats program PID */
	htx_stats_pid = htxd_get_htx_stats_pid();

	/* truncate the existing stats file */
	sprintf(temp_string, "%s/%s", global_htx_log_dir, HTX_STATS_SEND_FILE);
	if(truncate(temp_string, 0) == -1) {
		if(errno == ENOENT ) {
			if(creat(temp_string, 0) >= 0) {
			} else {
			}
				
		} else {
		}
	}

	/* have to set shm key to system header shared memory so that htx stats is able to connect */	
	htxd_set_system_header_info_shm_with_current_shm_key(p_ecg_info_to_getstats->ecg_shm_key);	

	/* sending signal SIGUSR1 to htx stats program to trigger the statistics collection*/
	htxd_send_SIGUSR1(htx_stats_pid);

	/* wait till stats file is updated */
	sleep(5);

	/* read the HTX stats file to string buffer */
	sprintf(temp_string, "%s/%s", global_htx_log_dir, HTX_STATS_FILE);
	return_code = htxd_read_file(temp_string, command_result); 
	
	return return_code;
}


int htxd_option_method_getstats(char **command_result)
{

	htxd *htxd_instance;
	char error_string[512];
	int return_code = 0;
	htxd_ecg_info * p_ecg_info_list;
	char command_ecg_name[MAX_ECG_NAME_LENGTH];

	htxd_instance = htxd_get_instance();
	strcpy(command_ecg_name, htxd_get_command_ecg_name() );

	*command_result = NULL;

	if(command_ecg_name[0] == '\0') {
		sprintf(command_ecg_name, "%s/mdt/%s", global_htx_home_dir, DEFAULT_ECG_NAME );
	}

	return_code = htxd_validate_command_requirements(htxd_instance, error_string);


	if(return_code != 0) {
		*command_result = malloc(512);
		strcpy(*command_result, error_string);
		return return_code;
	}

	p_ecg_info_list = htxd_get_ecg_info_node(htxd_instance->p_ecg_manager, command_ecg_name);	
	if(p_ecg_info_list == NULL) {
		*command_result = malloc(512);
		HTXD_TRACE(LOG_OFF, *command_result);
		sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", command_ecg_name);
		return -1;
	}

	return_code = htxd_getstats_ecg(p_ecg_info_list, command_result);	
	if(return_code != 0) {
		if(*command_result != 0) {
			free(*command_result);
		}
		*command_result = malloc(512);
		sprintf(*command_result, "Error : htxd_option_method_getstats() htxd_getstats_ecg() return <%d>", return_code);
		HTXD_TRACE(LOG_ON, *command_result);
		sprintf(*command_result, "Error : while getting test statistics details, error code = <%d>", return_code);
		return -1;
	}

	return 0;

}



int htxd_option_method_geterrlog(char **command_result)
{
	int return_code;
	char temp_string[300];

	sprintf(temp_string, "%s/%s", global_htx_log_dir, HTX_ERR_LOG_FILE);
	return_code = htxd_read_file(temp_string, command_result); 
	if(return_code != 0) {
		if(*command_result != 0) {
			free(*command_result);
		}
		*command_result = malloc(512);
		sprintf(*command_result, "Error : htxd_option_method_getstats() htxd_getstats_ecg() return <%d>", return_code);
		HTXD_TRACE(LOG_ON, *command_result);
		sprintf(*command_result, "Error : while getting error log, error code = <%d>", return_code);
	}

	return return_code;
}



int htxd_option_method_clrerrlog(char **command_result)
{
	int return_code;
	char temp_string[300];

	sprintf(temp_string, "%s/%s", global_htx_log_dir, HTX_ERR_LOG_FILE);
	*command_result = malloc(512);
	if(*command_result == NULL) {
		return -1;
	}

	return_code = truncate(temp_string, 0);
	if(return_code == -1) {
		sprintf(*command_result, "Error : failed to truncate htx stats file to send <%s>, errno = <%d>", temp_string, errno);
	
	} else {
		strcpy(*command_result, "HTX error log file cleared successfully");
	}

	return return_code;
}



int htxd_option_method_get_file(char **command_result)
{
	int return_code = 0;
	htxd *htxd_instance;
	char filename[512];
	char *running_mdt_name;
	int  htx_stats_pid;
	char command_string[128];


	htxd_instance = htxd_get_instance();

	strcpy(filename, htxd_instance->p_command->option_list);

	if(strcmp(STATS_FILE_STRING,  filename) == 0) {
		htx_stats_pid = htxd_get_htx_stats_pid();
		htxd_send_SIGUSR1(htx_stats_pid);
		sleep(5);

		sprintf(filename,"%s/%s", global_htx_log_dir, HTX_STATS_FILE);

	} else if(strcmp(RUNNING_MDT_STRING, filename) == 0) {
		running_mdt_name = htxd_get_running_ecg_name();	
		strcpy(filename, running_mdt_name);

	} else if(strcmp(MDT_LIST, filename) == 0) {
		sprintf(command_string, "ls %s/%s > %s/%s", global_htx_home_dir, MDT_DIR, global_htxd_log_dir, MDT_LIST_FILE);
		system(command_string);
		sprintf(filename, "%s/%s", global_htxd_log_dir, MDT_LIST_FILE);
	}

	return_code = htxd_read_file(filename, command_result);	
	if(return_code != 0) {
		if(*command_result != 0) {
			free(*command_result);
		}
		*command_result = malloc(HTX_ERR_MESSAGE_LENGTH);
		if(*command_result == NULL) {
			return 1;
		}
		sprintf(*command_result,"Error: failed to get command result");
		
	}

	return return_code;	
}



int htxd_option_method_cmd(char **command_result)
{

	int return_code;
	char command_string[512];
	htxd *htxd_instance;
	char temp_string[300];

	htxd_instance = htxd_get_instance();

	sprintf(temp_string, "%s/%s", global_htxd_log_dir, HTX_CMD_RESULT_FILE);
	sprintf(command_string, "echo \" Error: failed to execute command \<%s\> \" >%s; sync",  htxd_instance->p_command->option_list, temp_string);
	system(command_string);

	sprintf(command_string, " (%s) > %s 2>&1 ; sync", htxd_instance->p_command->option_list, temp_string);
	system(command_string);

	return_code = htxd_read_file(temp_string, command_result);
	if(return_code != 0) {
		if(*command_result != 0) {
			free(*command_result);
		}
		*command_result = malloc(HTX_ERR_MESSAGE_LENGTH);
		if(*command_result == NULL) {
			return 1;
		}
		sprintf(*command_result,"Error: failed to get command result");
		
	}

	return return_code;
}


int htxd_option_method_set_eeh(char **command_result)
{

	int return_code = 0;
	htxd *htxd_instance;
	char *ptr_env = NULL;
	char trace_string[256];


	htxd_instance = htxd_get_instance();

	*command_result = malloc(512);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	if(strlen(htxd_instance->p_command->option_list) == 0) {
		ptr_env = getenv("HTXEEH");	
		if(ptr_env == NULL) {
			strcpy(*command_result, "HTXEEH is not set");
		} else {
			sprintf(*command_result, "HTXEEH value is %s", ptr_env);
		}
	} else  if(strcmp(htxd_instance->p_command->option_list, "1") == 0)  {
		return_code = setenv("HTXEEH", "1", 1);
		if(return_code != 0) {
			sprintf(*command_result, "Error while setting HTXEEH environment variable, return code =<%d>, errono=<%d>", return_code, errno);
		} else {
			strcpy(*command_result, "set eeh flag successfully");
		}
	} else if(strcmp(htxd_instance->p_command->option_list, "0") == 0) {
		return_code = setenv("HTXEEH", "0", 1);
		if(return_code != 0) {
			sprintf(*command_result, "Error while setting HTXEEH environment variable, return code =<%d>, errono=<%d>", return_code, errno);
		} else {
			strcpy(*command_result, "unset eeh flag successfully");
		}
	} else {
		strcpy(*command_result, "Error : failed while setting eeh flag (HTXEEH) because of invalid argument, valid argument is 1 or 0");
		return_code = -1;
	}

	return return_code;
}


int htxd_option_method_set_kdblevel(char **command_result)
{

	int return_code = 0;
	htxd *htxd_instance;
	char *ptr_env = NULL;
	char trace_string[256];


	htxd_instance = htxd_get_instance();

	*command_result = malloc(512);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	if(strlen(htxd_instance->p_command->option_list) == 0) {
		ptr_env = getenv("HTXKDBLEVEL");	
		if(ptr_env == NULL) {
			strcpy(*command_result, "HTXKDBLEVEL is not set");
		} else {
			sprintf(*command_result, "HTXKDBLEVEL value is %s", ptr_env);
		}
	} else  if(strcmp(htxd_instance->p_command->option_list, "1") == 0)  {
		return_code = setenv("HTXKDBLEVEL", "1", 1);
		if(return_code != 0) {
			sprintf(*command_result, "Error while setting HTXKDBLEVEL environment variable, return code =<%d>, errono=<%d>", return_code, errno);
		} else {
			strcpy(*command_result, "set kdb level flag successfully");
		}
	} else if(strcmp(htxd_instance->p_command->option_list, "0") == 0) {
		return_code = setenv("HTXKDBLEVEL", "0", 1);
		if(return_code != 0) {
			sprintf(*command_result, "Error while setting HTXKDBLEVEL environment variable, return code =<%d>, errono=<%d>", return_code, errno);
		} else {
			strcpy(*command_result, "unset kdb level flag successfully");
		}
	} else {
		strcpy(*command_result, "Error : failed while setting kdb level flag (HTXKDBLEVEL) because of invalid argument, valid argument is 1 or 0");
		return_code = -1;
	}

	return return_code;

	return 0;
}




int htxd_option_method_set_hxecom(char **command_result)
{
	char temp_string[300];

	*command_result = malloc(512);
	if(*command_result == NULL) {
		sprintf(temp_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, temp_string);
		return -1;
	}


	if( htxd_is_file_exist("/build_net") == FALSE) {
		strcpy(*command_result,"Error : hxecom needs the build_net script, network setup for hxecom cannot be done");
		return -1;
	}

	sprintf(temp_string, "%s/ecg/ecg_net", global_htx_home_dir);
	system(temp_string);
	strcpy(*command_result,"hxecom setup completed successfully (ecg_net is appeneded to ecg.bu");
	
	return 0;
}



int htxd_option_method_set_htx_env(char **command_result)
{

	int return_code = 0;
	htxd *htxd_instance;
	char variable[128] = "";
	char value[128] = "";
	char trace_string[256];


	htxd_instance = htxd_get_instance();

	*command_result = malloc(512);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}


	if(strlen(htxd_instance->p_command->option_list) == 0) {
		strcpy(*command_result, "Please provide environment variable and value");
		return 1;
	}

	sscanf(htxd_instance->p_command->option_list, "%s %s", variable, value);
	if(strlen(variable) == 0) {
		strcpy(*command_result, "could not find environment variable");
		return 1;
	}

	return_code = setenv(variable, value, 1);
	if(return_code  != 0) {
		sprintf(*command_result, "Error : setenv returned while setting environment varibale <%s> with value <%s>, return  code <%d> and errno <%d>", variable, value, return_code, errno);
		return return_code;
	}

	sprintf(*command_result, "environment variable <%s> is set with <%s> successfully", variable, value);

	return return_code;
}



int htxd_option_method_get_htx_env(char **command_result)
{

	htxd *htxd_instance;
	char variable[128] = "";
	char *value = NULL;
	char trace_string[256];


	htxd_instance = htxd_get_instance();

	*command_result = malloc(512);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}


	if(strlen(htxd_instance->p_command->option_list) == 0) {
		strcpy(*command_result, "Please provide environment variable");
		return 1;
	}

	sscanf(htxd_instance->p_command->option_list, "%s", variable);
	if(strlen(variable) == 0) {
		strcpy(*command_result, "could not find environment variable");
		return 1;
	}

	value = getenv(variable);
	if(value  == NULL) {
		sprintf(*command_result, "environment variable <%s> is not defined", variable);
		return 1;
	}

	sprintf(*command_result, "environment variable <%s> value is <%s>", variable, value);

	return 0;
}



int htxd_option_method_getvpd(char **command_result)
{
	int return_code;
	char vpd_command_string[512];
	char trace_string[256];
	char temp_string[300];


	sprintf(temp_string, "%s/%s", global_htxd_log_dir, HTX_VPD_FILE);
	sprintf(vpd_command_string, "%s/etc/scripts/%s > %s", global_htx_home_dir, HTX_VPD_SCRIPT, temp_string);

	system(vpd_command_string);

	return_code = htxd_read_file(temp_string, command_result);
	if(return_code != 0) {
		if(*command_result != 0) {
			free(*command_result);
		}
		*command_result = malloc(512);
		if(*command_result == NULL) {
			sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
			HTXD_TRACE(LOG_ON, trace_string);
			return -1;
		}

		sprintf(*command_result, "Error : while getting VPD information");
	}
	return return_code;
}


void init_ecg_summary(htxd_ecg_summary *p_summary_details)
{
	p_summary_details->total_device			= 0;
	p_summary_details->running_device		= 0;
	p_summary_details->suspended_device		= 0;
	p_summary_details->error_device			= 0;
	p_summary_details->partially_running_device	= 0;
	p_summary_details->terminated_device		= 0;
	p_summary_details->died_device			= 0;
	p_summary_details->hung_device			= 0;
	p_summary_details->completed_device		= 0;
	p_summary_details->inactive_device		= 0;
}



int htxd_getecgsum_all_device(htxd_ecg_info * p_ecg_info_to_getecgsum, char *command_result)
{
	struct htxshm_HE *p_HE;
	int i;
	char device_run_status[5];
	int return_code = 0;
	htxd_ecg_summary summary_details;

	init_ecg_summary(&summary_details);


	p_HE = (struct htxshm_HE *)(p_ecg_info_to_getecgsum->ecg_shm_addr.hdr_addr + 1);

	for(i = 0; i < p_ecg_info_to_getecgsum->ecg_shm_exerciser_entries ; i++) {
		htxd_get_device_run_status(p_HE, p_ecg_info_to_getecgsum, i, device_run_status);
		if( strcmp(device_run_status, "RN") == 0) {
			summary_details.running_device++;
		} else if( strcmp(device_run_status, "DD") == 0) {
			summary_details.died_device++;
		} else if( strcmp(device_run_status, "DT") == 0) {
			summary_details.dr_terminated_device++;
		} else if( strcmp(device_run_status, "TM") == 0) {
			summary_details.terminated_device++;
		} else if( strcmp(device_run_status, "ER") == 0) {
			summary_details.error_device++;
		} else if( strcmp(device_run_status, "ST") == 0) {
			summary_details.suspended_device++;
		} else if( strcmp(device_run_status, "CP") == 0) {
			summary_details.completed_device++;
		} else if( strcmp(device_run_status, "HG") == 0) {
			summary_details.hung_device++;
		} else if( strcmp(device_run_status, "PR") == 0) {
			summary_details.partially_running_device++;
		} else if( strcmp(device_run_status, "  ") == 0) {
			summary_details.inactive_device = p_ecg_info_to_getecgsum->ecg_shm_exerciser_entries;
			break;
		} 
		p_HE++;
	}

	summary_details.total_device = p_ecg_info_to_getecgsum->ecg_shm_exerciser_entries;

	sprintf (command_result,
		"%s is in %s state\n"
		"Total Devices       = %d\n"
		" Running            = %d\n"
		" Suspended          = %d\n"
		" Error              = %d\n"
		" Partially Running   = %d\n"
		" Terminated         = %d\n"
		" Died               = %d\n"
		" Hung               = %d\n"
		" Completed          = %d\n"
		" Inactive           = %d\n\n", p_ecg_info_to_getecgsum->ecg_name,  "ACTIVE", 
						summary_details.total_device,
						summary_details.running_device,
						summary_details.suspended_device,
						summary_details.error_device,
						summary_details.partially_running_device,
						summary_details.terminated_device,
						summary_details.died_device,
						summary_details.hung_device,
						summary_details.completed_device,
						summary_details.inactive_device  );
			
	
	return return_code;
}



int htxd_option_method_getecgsum(char **command_result)
{
	htxd *htxd_instance;
	char error_string[512];
	int return_code = 0;
	htxd_ecg_info * p_ecg_info_list;
	char command_ecg_name[MAX_ECG_NAME_LENGTH];
	char trace_string[256];


	htxd_instance = htxd_get_instance();
	strcpy(command_ecg_name, htxd_get_command_ecg_name() );

	*command_result = malloc(EXTRA_BUFFER_LENGTH *4);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}


	return_code = htxd_validate_command_requirements(htxd_instance, error_string);
	if(return_code != 0) {
		strcpy(*command_result, error_string);
		return return_code;
	}

	if(command_ecg_name[0] != '\0') {
		p_ecg_info_list = htxd_get_ecg_info_node(htxd_instance->p_ecg_manager, command_ecg_name);	
		if(p_ecg_info_list == NULL) {
			sprintf(*command_result, "Specified ECG/MDT<%s> is not currently running", command_ecg_name);
			return -1;
		}

		return_code = htxd_getecgsum_all_device(p_ecg_info_list, *command_result);	
	} else {
		return_code = htxd_process_all_active_ecg(htxd_getecgsum_all_device, *command_result);
	}



	return return_code;
}



int htxd_option_method_getecglist(char **command_result)
{
	int mdt_count = 0;
	int return_code;
	char trace_string[256];
	char temp_string1[300];
	char temp_string2[300];


	sprintf(temp_string1, "%s/%s", global_htx_home_dir, MDT_DIR); 
	sprintf(temp_string2, "%s/%s", global_htxd_log_dir, MDT_LIST_FILE); 
	return_code = htxd_get_regular_file_count(temp_string1);
	if(return_code == -1) {
		*command_result = malloc(256);
		if(*command_result == NULL) {
			sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
			HTXD_TRACE(LOG_ON, trace_string);
			return -1;
		}

		strcpy(*command_result, "Error while accessing MDT directory");
		HTXD_TRACE(LOG_ON, *command_result);
		return -1;
	}
	mdt_count = return_code;

	if(mdt_count > 0) {

		return_code = htxd_wrtie_mdt_list(temp_string1, temp_string2, mdt_count, "w", " ");
		if(return_code == -1) {
			HTXD_TRACE(LOG_ON, "htxd_wrtie_mdt_list returns qith -1");
			return -1;
		}
		return_code = htxd_read_file(temp_string2, command_result);
		if(return_code != 0) {
			sprintf(trace_string, "htxd_read_file() returned with %d", return_code);
			HTXD_TRACE(LOG_ON, trace_string);
			return -1;
		}
	} else {
		*command_result = malloc(256);
		if(*command_result == NULL) {
			sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
			HTXD_TRACE(LOG_ON, trace_string);
			return -1;
		}

		strcpy(*command_result, "No files present in MDT directory");
 
	}

	return 0;
}



int htxd_get_common_command_result_row_entry(int mode, struct htxshm_HE *p_HE, htxd_ecg_info * p_ecg_info_to_query, int device_position, char *p_query_row_entry)
{


	char device_coe_soe_status[5];
	char device_active_suspend_status[15];

	if(mode == COE_SOE_STATE){
		htxd_get_device_coe_soe_status(p_HE, device_coe_soe_status);

		sprintf (p_query_row_entry, "%c%-7s%c%-7s%c%-12s%c%-19s%c%s", 
			'\n',
			device_coe_soe_status, ' ',
			p_HE->sdev_id, ' ',
			p_HE->adapt_desc, ' ',
			p_HE->device_desc, ' ',
			p_HE->slot_port);
	} else {
		htxd_get_device_active_suspend_status(p_ecg_info_to_query->ecg_sem_id, device_position, device_active_suspend_status);

		sprintf (p_query_row_entry, "%c%-7s%c%-7s%c%-12s%c%-19s%c%s", 
			'\n',
			device_active_suspend_status, ' ',
			p_HE->sdev_id, ' ',
			p_HE->adapt_desc, ' ',
			p_HE->device_desc, ' ',
			p_HE->slot_port);
	}

	return 0;	
}



int htxd_get_common_command_result_ecg(int mode, htxd_ecg_info *p_ecg_info, char *option_list, char *command_result)
{

	struct htxshm_HE *p_HE;
	int i;
	char result_row_entry[256];
	char *p_device_name;


	p_HE = (struct htxshm_HE *)(p_ecg_info->ecg_shm_addr.hdr_addr + 1);

	if(option_list == NULL)  {
		for(i = 0; i < p_ecg_info->ecg_shm_exerciser_entries ; i++) {
			htxd_get_common_command_result_row_entry(mode, p_HE, p_ecg_info, i, result_row_entry);
			strcat(command_result, result_row_entry);

			p_HE++;
		}
	} else {
		if(htxd_is_list_regular_expression(option_list) == TRUE) {
			htxd_expand_device_name_list(p_ecg_info, option_list);
		}

		p_device_name = strtok(option_list, " ");
		while(p_device_name != NULL) {
			p_HE = (struct htxshm_HE *)(p_ecg_info->ecg_shm_addr.hdr_addr + 1);
			for(i = 0; i < p_ecg_info->ecg_shm_exerciser_entries ; i++) {
				if(strcmp(p_HE->sdev_id, p_device_name) == 0) {
					htxd_get_common_command_result_row_entry(mode, p_HE, p_ecg_info, i, result_row_entry);
					strcat(command_result, result_row_entry);
					break;
				}
				p_HE++;
			}
			p_device_name = strtok(NULL, " ");
		}
	}

	return 0;
}





int htxd_get_common_command_result_all_active_ecg(int mode, char *device_list, char *command_result)
{
	htxd *htxd_instance;
	htxd_ecg_info * p_ecg_info_list;
	int return_code = 0;


	htxd_instance = htxd_get_instance();

	p_ecg_info_list = htxd_get_ecg_info_list(htxd_instance->p_ecg_manager);

	while(p_ecg_info_list != NULL) {
		return_code = htxd_get_common_command_result_ecg(mode, p_ecg_info_list, device_list, command_result);
		p_ecg_info_list = p_ecg_info_list->ecg_info_next;
	}

	return return_code;
}




int htxd_get_common_command_result(int mode, htxd_ecg_info *p_ecg_info, char *option_list, char *command_result)
{
	int return_code = 0;


	strcpy(command_result, "State   Dev     Adapt Desc   Device Desc         Slot Port\n");
	strcat(command_result, "-----------------------------------------------------------");

	if(p_ecg_info == NULL) {
		htxd_get_common_command_result_all_active_ecg(mode, option_list, command_result);
	} else {
		htxd_get_common_command_result_ecg(mode, p_ecg_info, option_list, command_result);	
	}
	return return_code;
}



void htxd_get_screen_5_row_entry(struct htxshm_HE *p_HE, htxd_ecg_info * p_ecg_info_to_query, int device_position, char *query_device_entry)
{
	char device_run_status[5];
	char device_coe_soe_status[5];
	char device_active_suspend_status[15];
	char last_update_day_of_year[10];
	char last_update_time[80];
	char last_error_day_of_year[10];
	char last_error_time[80];

	htxd_get_device_run_status(p_HE, p_ecg_info_to_query, device_position, device_run_status);
	htxd_get_device_active_suspend_status(p_ecg_info_to_query->ecg_sem_id, device_position, device_active_suspend_status);
	htxd_get_device_coe_soe_status(p_HE, device_coe_soe_status);
	htxd_get_time_details(p_HE->tm_last_upd, NULL, last_update_day_of_year, last_update_time);
	if(p_HE->tm_last_err != 0) {
		htxd_get_time_details(p_HE->tm_last_err, NULL, last_error_day_of_year, last_error_time);
	} else {
		strcpy(last_error_day_of_year, "NA");
		strcpy(last_error_time, "NA");
	}

	sprintf (query_device_entry, "<%s> <%s> <%s> <%s> <%d> <%d> <%s> <%s> |",
		p_HE->sdev_id,
		device_run_status,
		last_update_day_of_year,
		last_update_time,
		p_HE->cycles,
		p_HE->test_id,
		last_error_day_of_year,
		last_error_time
	);

}



int htxd_screen_5_all_device(htxd_ecg_info * p_ecg_info_to_screen_5, int device_start_position, char *command_result)
{
	struct htxshm_HE *p_HE;
	int i;
	char screen_5_row_entry[256];
	int return_code = 0;
	int display_device_count;
	int error_count;



	display_device_count = p_ecg_info_to_screen_5->ecg_shm_exerciser_entries - device_start_position;
	if(display_device_count > (SCREEN_5_ENTRIES_PER_PAGE)) {
		display_device_count = SCREEN_5_ENTRIES_PER_PAGE;
	}

	error_count = htxd_get_system_header_info_error_count();

	sprintf(command_result, "%d %d %d %s %lu %s |", 
		display_device_count,
		error_count,
		p_ecg_info_to_screen_5->ecg_shm_exerciser_entries, 
		p_ecg_info_to_screen_5->ecg_name,
		time((long *) 0),
		htxd_get_ecg_start_time()
	);

	p_HE = (struct htxshm_HE *)(p_ecg_info_to_screen_5->ecg_shm_addr.hdr_addr + 1);

	p_HE += device_start_position;

	for(i = 0; ( (i < p_ecg_info_to_screen_5->ecg_shm_exerciser_entries) && ((i < SCREEN_5_ENTRIES_PER_PAGE)) ); i++) {
		htxd_get_screen_5_row_entry(p_HE, p_ecg_info_to_screen_5, i + device_start_position, screen_5_row_entry);
		strcat(command_result, screen_5_row_entry);

		p_HE++;
	}
	
	return return_code;

}



int htxd_option_method_screen_5(char **command_result)
{

	htxd_ecg_info * p_ecg_info_list;
	htxd_option_method_object screen_5_method;
	int device_display_start_position;
	int result_size = 1024 * 10;
	char trace_string[256];



	htxd_init_option_method(&screen_5_method);

	*command_result = malloc(result_size);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	memset(*command_result, 0, (result_size) );

	screen_5_method.return_code = htxd_validate_command_requirements(screen_5_method.htxd_instance, screen_5_method.error_string);
	if(screen_5_method.return_code != 0) {
		strcpy(*command_result, screen_5_method.error_string);
		return screen_5_method.return_code;
	}

	p_ecg_info_list = htxd_get_ecg_info_list(screen_5_method.htxd_instance->p_ecg_manager);

	if(p_ecg_info_list == NULL) {
		sprintf(*command_result, "Internal error, could not find any running ecg");
		return 1;	
	}

	device_display_start_position = atoi(screen_5_method.command_option_list);

	if(device_display_start_position > p_ecg_info_list->ecg_shm_exerciser_entries) {
		sprintf(*command_result, "Internal error, device position crossing the max limit");
		return 1;
	}

	screen_5_method.return_code = htxd_screen_5_all_device(p_ecg_info_list, device_display_start_position, *command_result);

	return screen_5_method.return_code;
}




void htxd_get_screen_2_4_row_entry(struct htxshm_HE *p_HE, htxd_ecg_info * p_ecg_info, int device_position, activate_halt_entry *p_activate_halt_row, int screen_mode)
{
	char device_run_status[5];
	char device_active_suspend_status[15];
	int sem_status;


	if(screen_mode == SCREEN_MODE_2) {
		sem_status = htxd_get_device_run_sem_status(p_ecg_info->ecg_sem_id, device_position);
		if(sem_status == 0) {
			strcpy(p_activate_halt_row->device_status, " ACTIVE ");
		} else{
			strcpy(p_activate_halt_row->device_status, " HALTED ");
		}
	} else if(screen_mode == SCREEN_MODE_4) {
		if(p_HE->cont_on_err == HTX_SOE) {
			strcpy(p_activate_halt_row->device_status, "  HOE   ");
		} else {
			strcpy(p_activate_halt_row->device_status, "  COE   ");
		}
	}

	strcpy(p_activate_halt_row->device_name, p_HE->sdev_id);
	p_activate_halt_row->slot = p_HE->slot;
	p_activate_halt_row->port = p_HE->port;
	strcpy(p_activate_halt_row->adapt_desc, p_HE->adapt_desc);
	strcpy(p_activate_halt_row->device_desc, p_HE->device_desc);
}	

	

int htxd_screen_2_4_all_device(htxd_ecg_info * p_ecg_info, int device_start_position, char *command_result, int screen_mode)
{
	struct htxshm_HE	*p_HE;
	activate_halt_entry	*p_activate_halt_row;
	int			i;
	int			return_code = 0;


	p_activate_halt_row = (activate_halt_entry*) command_result;

	p_HE = (struct htxshm_HE *)(p_ecg_info->ecg_shm_addr.hdr_addr + 1);

	p_HE += device_start_position;

	for(i = device_start_position; ( (i < p_ecg_info->ecg_shm_exerciser_entries) && (i < (SCREEN_2_4_DEVICES_PER_PAGE+device_start_position)) ); i++) {
		htxd_get_screen_2_4_row_entry(p_HE, p_ecg_info, i, p_activate_halt_row, screen_mode);

		p_HE++;
		p_activate_halt_row++;
	}
	
	return return_code;

}



int htxd_option_method_screen_4(char **command_result)
{
	
	htxd_ecg_info * p_ecg_info_list;
	htxd_option_method_object screen_4_method;
	int device_display_start_position;
	char trace_string[256];


	htxd_init_option_method(&screen_4_method);

	*command_result = malloc(1024 * 10);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return 1;
	}

	memset(*command_result, 0, (1024 * 10) );

	screen_4_method.return_code = htxd_validate_command_requirements(screen_4_method.htxd_instance, screen_4_method.error_string);
	if(screen_4_method.return_code != 0) {
		strcpy(*command_result, screen_4_method.error_string);
		return screen_4_method.return_code;
	}

	p_ecg_info_list = htxd_get_ecg_info_list(screen_4_method.htxd_instance->p_ecg_manager);

	if(p_ecg_info_list == NULL) {
		sprintf(*command_result, "Internal error, could not find any running ecg");
		return 1;	
	}

	device_display_start_position = atoi(screen_4_method.command_option_list);

	if(device_display_start_position > p_ecg_info_list->ecg_shm_exerciser_entries) {
		sprintf(*command_result, "Internal error, device position crossing the max limit");
		return 1;
	}

	screen_4_method.return_code = htxd_screen_2_4_all_device(p_ecg_info_list, device_display_start_position, *command_result, SCREEN_MODE_4);

	return screen_4_method.return_code;
}



void htxd_get_screen_2_row_entry(struct htxshm_HE *p_HE, htxd_ecg_info * p_ecg_info, int device_position, activate_halt_entry *p_activate_halt_row)
{
	char device_run_status[5];
	char device_active_suspend_status[15];
	int sem_status;


	sem_status = htxd_get_device_run_sem_status(p_ecg_info->ecg_sem_id, device_position);
	if(sem_status == 0) {
		strcpy(p_activate_halt_row->device_status, " ACTIVE ");
	} else{
		strcpy(p_activate_halt_row->device_status, " HALTED ");
	}

	strcpy(p_activate_halt_row->device_name, p_HE->sdev_id);
	p_activate_halt_row->slot = p_HE->slot;
	p_activate_halt_row->port = p_HE->port;
	strcpy(p_activate_halt_row->adapt_desc, p_HE->adapt_desc);
	strcpy(p_activate_halt_row->device_desc, p_HE->device_desc);
}	

	

int htxd_screen_2_all_device(htxd_ecg_info * p_ecg_info_to_screen_2, int device_start_position, char *command_result)
{
	struct htxshm_HE *p_HE;
	int i;
//	char screen_5_row_entry[256];
	int return_code = 0;
	activate_halt_entry *p_activate_halt_row;


	p_activate_halt_row = (activate_halt_entry*) command_result;

	p_HE = (struct htxshm_HE *)(p_ecg_info_to_screen_2->ecg_shm_addr.hdr_addr + 1);

	p_HE += device_start_position;

	for(i = device_start_position; ( ( i < p_ecg_info_to_screen_2->ecg_shm_exerciser_entries) && (i < SCREEN_5_ENTRIES_PER_PAGE) ) ; i++) {
		htxd_get_screen_2_row_entry(p_HE, p_ecg_info_to_screen_2, i, p_activate_halt_row);
		//strcat(command_result, screen_5_row_entry);

		p_HE++;
		p_activate_halt_row++;
	}
	
	return return_code;

}



int htxd_option_method_screen_2(char **command_result)
{
	
	htxd_ecg_info * p_ecg_info_list;
	htxd_option_method_object screen_2_method;
	int device_display_start_position;
	char trace_string[256];


	htxd_init_option_method(&screen_2_method);

	*command_result = malloc(1024 * 10);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return 1;
	}

	memset(*command_result, 0, (1024 * 10) );

	screen_2_method.return_code = htxd_validate_command_requirements(screen_2_method.htxd_instance, screen_2_method.error_string);
	if(screen_2_method.return_code != 0) {
		strcpy(*command_result, screen_2_method.error_string);
		return screen_2_method.return_code;
	}

	p_ecg_info_list = htxd_get_ecg_info_list(screen_2_method.htxd_instance->p_ecg_manager);

	if(p_ecg_info_list == NULL) {
		sprintf(*command_result, "Internal error, could not find any running ecg");
		return 1;	
	}

	device_display_start_position = atoi(screen_2_method.command_option_list);

	if(device_display_start_position > p_ecg_info_list->ecg_shm_exerciser_entries) {
		sprintf(*command_result, "Internal error, device position crossing the max limit");
		return 1;
	}

	screen_2_method.return_code = htxd_screen_2_4_all_device(p_ecg_info_list, device_display_start_position, *command_result, SCREEN_MODE_2);

	return screen_2_method.return_code;
}



int htxd_toggle_coe_soe(htxd_ecg_info *p_ecg_info, int device_position, int operation_mode, int operation)
{

	struct htxshm_HE	*p_HE;
	int			i;
	

	p_HE = (struct htxshm_HE *)(p_ecg_info->ecg_shm_addr.hdr_addr + 1);

	if(operation_mode == HTX_SINGLE_DEVICE) {
		p_HE+=device_position;	
		if(p_HE->cont_on_err == 1) {
			p_HE->cont_on_err = 0;
		} else {
			p_HE->cont_on_err = 1;
		}
	} else if (operation_mode == HTX_SCREEN_DEVICES) {
		p_HE+=device_position;
		for(i = 0; (i < p_ecg_info->ecg_shm_exerciser_entries) && (i < 15); i++) {
			p_HE->cont_on_err = operation;
			p_HE++;
		}
	} else if (operation_mode == HTX_ALL_DEVICES) {
		for(i = 0; i < p_ecg_info->ecg_shm_exerciser_entries; i++) {
			p_HE->cont_on_err = operation;
			p_HE++;
		}
	}

	return 0;
}



int htxd_option_method_coe_soe(char **command_result)
{
	
	htxd_ecg_info			*p_ecg_info_list;
	htxd_option_method_object	coe_soe_method;
	int				device_position;
	int				operation_mode;
	int				operation;
	char				trace_string[256];


	htxd_init_option_method(&coe_soe_method);

	*command_result = malloc(1024 * 10);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return 1;
	}

	memset(*command_result, 0, (1024 * 10) );

	coe_soe_method.return_code = htxd_validate_command_requirements(coe_soe_method.htxd_instance, coe_soe_method.error_string);
	if(coe_soe_method.return_code != 0) {
		strcpy(*command_result, coe_soe_method.error_string);
		return coe_soe_method.return_code;
	}

	p_ecg_info_list = htxd_get_ecg_info_list(coe_soe_method.htxd_instance->p_ecg_manager);

	if(p_ecg_info_list == NULL) {
		sprintf(*command_result, "Internal error, could not find any running ecg");
		return 1;	
	}

	sscanf(coe_soe_method.command_option_list, "%d %d %d", &device_position, &operation_mode, &operation);

	if(device_position > p_ecg_info_list->ecg_shm_exerciser_entries) {
		sprintf(*command_result, "Internal error, device position crossing the max limit");
		return 1;
	}

	htxd_toggle_coe_soe(p_ecg_info_list, device_position, operation_mode, operation);

	return coe_soe_method.return_code;
}




int htxd_toggle_activate_halt(htxd_ecg_info *p_ecg_info, int device_position, int operation_mode, int operation)
{

	int device_run_status;
	int                     i;


	if(operation_mode == HTX_SINGLE_DEVICE) {
		device_run_status = htxd_get_device_run_sem_status(p_ecg_info->ecg_sem_id, device_position);	
		if(device_run_status == 1) {
			htxd_set_device_run_sem_status(p_ecg_info->ecg_sem_id, device_position, 0);
		} else {
			htxd_set_device_run_sem_status(p_ecg_info->ecg_sem_id, device_position, 1);
		}
	} else if (operation_mode == HTX_SCREEN_DEVICES) {
		for(i = 0; (i < p_ecg_info->ecg_shm_exerciser_entries) && (i < 15); i++) {
			htxd_set_device_run_sem_status(p_ecg_info->ecg_sem_id, device_position + i, operation);
		}
	} else if (operation_mode == HTX_ALL_DEVICES) {
		for(i = 0; i < p_ecg_info->ecg_shm_exerciser_entries; i++) {
			htxd_set_device_run_sem_status(p_ecg_info->ecg_sem_id, device_position + i, operation);
		}
	}

	return 0;
}



int htxd_option_method_activate_halt(char **command_result)
{
	
	htxd_ecg_info * p_ecg_info_list;
	htxd_option_method_object activate_halt_method;
	int device_position;
	int operation_mode;
	int operation;
	char trace_string[256];


	htxd_init_option_method(&activate_halt_method);

	*command_result = malloc(1024 * 10);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return 1;
	}

	memset(*command_result, 0, (1024 * 10) );

	activate_halt_method.return_code = htxd_validate_command_requirements(activate_halt_method.htxd_instance, activate_halt_method.error_string);
	if(activate_halt_method.return_code != 0) {
		strcpy(*command_result, activate_halt_method.error_string);
		return activate_halt_method.return_code;
	}

	p_ecg_info_list = htxd_get_ecg_info_list(activate_halt_method.htxd_instance->p_ecg_manager);

	if(p_ecg_info_list == NULL) {
		sprintf(*command_result, "Internal error, could not find any running ecg");
		return 1;	
	}

	sscanf(activate_halt_method.command_option_list, "%d %d %d", &device_position, &operation_mode, &operation);

	if(device_position > p_ecg_info_list->ecg_shm_exerciser_entries) {
		sprintf(*command_result, "Internal error, device position crossing the max limit");
		return 1;
	}

	htxd_toggle_activate_halt(p_ecg_info_list, device_position, operation_mode, operation);

	return activate_halt_method.return_code;
}




int htxd_option_method_device_count(char **command_result)
{
	
	int device_count;
	char trace_string[256];


	*command_result = malloc(64);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	memset(*command_result, 0, (64) );

	device_count = htxd_get_loaded_device_count();
	sprintf(*command_result, "%d", device_count);


	return 0;
}



int htxd_option_method_get_daemon_state(char **command_result)
{
	int daemon_state;
	int test_running_state;
	char trace_string[256];


	*command_result = malloc(64);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	memset(*command_result, 0, (64) );

	daemon_state = htxd_get_daemon_state();
	test_running_state = htxd_get_test_running_state();
	sprintf(*command_result, "%d %d", daemon_state, test_running_state);


	return 0;
}



int htxd_option_method_test_activate_halt(char **command_result)
{

#ifdef __HTX_LINUX__
	union semun semctl_arg;
#else
	union semun
	{
		int val;
		struct semid_ds *buf;
		unsigned short *array;
	} semctl_arg;
#endif

	int ecg_sem_id;
	int ecg_test_sem_status;
	int sem_length;
	int total_number_of_device;
	int running_exer_count;
	int exer_counter;
	int exer_system_halted;
	int exer_error_halted;
	int exer_operation_halted;
	int exer_exited;
	int is_all_exer_stopped = 0;
	int delay_counter = 0;
	int max_halt_wait_time = 600;
	int delay_slice = 3;
	struct htxshm_HE *p_HE;
	htxd *p_htxd_instance;
	char trace_string[256];



	*command_result = malloc(256);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	memset(*command_result, 0, (256) );

	ecg_sem_id = htxd_get_ecg_sem_id();	
	ecg_test_sem_status = htxd_get_global_activate_halt_sem_status(ecg_sem_id);
	if(ecg_test_sem_status == 0) {
		htxd_set_global_activate_halt_sem_status(ecg_sem_id, 1);

		running_exer_count = htxd_get_total_device_count();
		total_number_of_device = running_exer_count + htxd_get_max_add_device();
		sem_length = (total_number_of_device * SEM_PER_EXER) + SEM_GLOBAL;
		semctl_arg.array = (ushort*) malloc((sem_length * sizeof(ushort) ));
		if(semctl_arg.array == NULL) {
			sprintf(*command_result, "Unable to allocate memory for semctl GETVAL array.  errno = %d.", errno);
			HTXD_TRACE(LOG_ON, *command_result);
			return 1;
		}
	
		p_htxd_instance = htxd_get_instance();
		p_HE = (struct htxshm_HE *) (p_htxd_instance->p_ecg_manager->ecg_info_list->ecg_shm_addr.hdr_addr + 1);

		while(delay_counter < max_halt_wait_time) {
			semctl(ecg_sem_id, 0, GETALL, semctl_arg);
			exer_counter = exer_system_halted = exer_error_halted = exer_operation_halted = exer_exited = 0;

			while(exer_counter < running_exer_count) {
				if(semctl_arg.array[(exer_counter * SEM_PER_EXER + SEM_GLOBAL + 2)] == 1) {
					exer_system_halted++;
				} else if(semctl_arg.array[(exer_counter * SEM_PER_EXER + SEM_GLOBAL + 1)] == 1) {
					exer_error_halted++;
				} else if(semctl_arg.array[(exer_counter * SEM_PER_EXER + SEM_GLOBAL )] == 1) {
					exer_operation_halted++;
				} else if( (p_HE + exer_counter) -> PID == 0) {
					exer_exited++;
				} else {
					break;
				}

				exer_counter++;
			}

			if(running_exer_count  <= (exer_system_halted + exer_error_halted + exer_operation_halted + exer_exited) ) {
				is_all_exer_stopped = 1;
				break;
			}	

			sleep(delay_slice);
			delay_counter += delay_slice;
		}

		free(semctl_arg.array);

		htxd_set_test_running_state(0);

		if ( is_all_exer_stopped == 0) {
			sprintf(*command_result, "Warning: Not all HE's stopped within allowed time.  System will continue...");
			return 1;	
		} else {
			sprintf(*command_result, "All Hardware Exercisors are stopped");	
			return 0;
		}

	} else {
		htxd_set_global_activate_halt_sem_status(ecg_sem_id, 0);
		htxd_set_test_running_state(1);
		sprintf(*command_result, "Test is activated");
	}
	

	return 0;
}



int htxd_option_method_config_net(char **command_result)
{
	
	htxd_option_method_object config_net_method;
	char bpt_name[256];
	char command_string[256];
	char trace_string[256];


	htxd_init_option_method(&config_net_method);

	*command_result = malloc(256);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	memset(*command_result, 0, (256) );

	sscanf(config_net_method.command_option_list, "%s", bpt_name);

	if(strcmp(bpt_name, NO_BPT_NAME_PROVIDED) == 0) {
#ifdef __HTX_LINUX__
		sprintf(command_string, "build_net help y y > %s/%s 2>&1", global_htxd_log_dir, BUILD_NET_LOG);
#else
		sprintf(command_string, "cd /; build_net help y y > %s/%s 2>&1", global_htxd_log_dir, BUILD_NET_LOG);
#endif
	} else {
		if(htxd_is_file_exist(bpt_name) == FALSE) {
			sprintf(*command_result, "provided bpt file <%s> is not present", bpt_name);
			return 1;
		}
#ifdef __HTX_LINUX__
		sprintf(command_string, "build_net %s > %s/%s 2>&1", bpt_name, global_htxd_log_dir, BUILD_NET_LOG);
#else
		sprintf(command_string, "cd /; build_net %s > %s/%s 2>&1", bpt_name, global_htxd_log_dir, BUILD_NET_LOG);
#endif
	}
	system(command_string); 


	sprintf(*command_result, "%s", "test network config is completed, please verify with ping test");

	return 0;
}



int htxd_option_method_append_net_mdt(char **command_result)
{
	htxd_option_method_object append_net_mdt_method;
	char mdt_name[300];
	char temp_string[256];
	char command_string[256];
	char trace_string[256];


	htxd_init_option_method(&append_net_mdt_method);

	*command_result = malloc(384);
	if(*command_result == NULL) {
		sprintf(trace_string, "command_result: malloc failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_ON, trace_string);
		return -1;
	}

	memset(*command_result, 0, (384) );

	sscanf(append_net_mdt_method.command_option_list, "%s", mdt_name);
	if( (strlen(mdt_name) > 0) && (strchr(mdt_name, '/') == NULL) ) {
		sprintf(temp_string, "%s/mdt/%s", global_htx_home_dir, mdt_name);
	} else {
		strcpy(temp_string, mdt_name);
	}
	if(htxd_is_file_exist(temp_string) == FALSE) {
		sprintf(*command_result, "provided mdt <%s> is not present", mdt_name);
		return 1;
	}

	sprintf(command_string, "%s/mdt/mdt_net %s >%s/%s  2>&1", global_htx_home_dir, mdt_name, global_htxd_log_dir, MDT_NET_LOG);
	system(command_string); 

	sprintf(*command_result, "net.mdt is appaned to mdt <%s>", mdt_name);

	return 0;
}



int htxd_option_method_test_net(char **command_result)
{

	char command_string[64];
	FILE *fp_ping_test;
	int test_status;


	*command_result = malloc(384);
	if(*command_result == NULL) {
		return -1;
	}

	memset(*command_result, 0, (384) );

	sprintf(command_string, "pingum | grep -i \"All networks ping Ok\" | wc -l");
	fp_ping_test = popen(command_string, "r");
	if(fp_ping_test == NULL) {
		sprintf(*command_result, "pingum command execution failed");
		return 2;
	}
	fscanf(fp_ping_test, "%d", &test_status);
	pclose(fp_ping_test);

	if(test_status == 1) {
		sprintf(*command_result, "pingum test is success");
		return 0;
	} else {
		sprintf(*command_result, "pingum test is failed");
		return 1;
	}
	return 0;
}
