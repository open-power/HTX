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
/* @(#)21	1.3  src/htx/usr/lpp/htx/bin/htxd/htxcmd_parse_command.c, htxd, htxubuntu 8/23/15 23:34:04 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "htxd_common.h"
#include "htxd_common_define.h"
#include "htxcmd.h"


void initialize_command_object(htxcmd_command *p_command_object)
{
	strcpy(p_command_object->sut_hostname, DEFAULT_SUT_HOSTNAME);
	p_command_object->daemon_port_number = HTXD_DEFAULT_PORT;
	p_command_object->command_name[0] = '\0';
	p_command_object->command_index = -1;
	p_command_object->ecg_name[0] = '\0';
	p_command_object->option_flag = FALSE;
	p_command_object->option_list[0] = '\0';
	p_command_object->command_argument_vector_position = -1;
}



void initialize_response_object(htxcmd_response *p_response_object)
{
	p_response_object->response_buffer	= NULL;
	p_response_object->return_code		= 0;
}



/* validate command option with option list */
int htxcmd_validate_command_name(htxcmd_command *p_command_object)
{
	int option_count = sizeof(option_list)/sizeof(option);
	int i = 0;
	char temp_error_string[256];

	//printf("option_string <%s> option_count<%d> sizeof(option_list) <%d> sizeof(option) <%d>\n", option_string, option_count, sizeof(option_list), sizeof(option));
	if(p_command_object->command_name[0] != '-') {
		sprintf(temp_error_string, "Error : provided option <%s> is invalid", p_command_object->command_name);
		htxcmd_display_usage(temp_error_string);
		exit(1);
	}

	while( i < option_count ) {
//		printf("from array <%s>\n", option_list[i].option_string);
		if(strncmp(p_command_object->command_name, option_list[i].option_string, strlen(option_list[i].option_string) ) == 0 ) {
		/* command match found */
			p_command_object->command_index = i;
			if(option_list[i].parameter_flag == TRUE) {
				p_command_object->option_flag = TRUE;
			}
			return i;
		}
		i++;
	}
	if( i >= option_count) {
		sprintf(temp_error_string, "Error : invalid command option <%s> is provided", p_command_object->command_name);
		htxcmd_display_usage(temp_error_string);
		exit(1);
	}

	return -1;
}



void htxcmd_display_command_object(htxcmd_command *p_command_object)
{
	printf("[DEBUG] : htxcmd_display_command_object() : sut_hostname = <%s>\n", p_command_object->sut_hostname); 
	printf("[DEBUG] : htxcmd_display_command_object() : daemon_port_number  = <%d>\n", p_command_object->daemon_port_number); 
	printf("[DEBUG] : htxcmd_display_command_object() : command_name = <%s>\n", p_command_object->command_name); 
	printf("[DEBUG] : htxcmd_display_command_object() : ecg_name = <%s>\n", p_command_object->ecg_name); 
	printf("[DEBUG] : htxcmd_display_command_object() : option_list = <%s>\n", p_command_object->option_list); 
	fflush(stdout);
}



/* get the SUT hostname from command arguments */
char * htxcmd_get_sut_hostname(int argument_count, char *argument_vector[], char *sut_hostname)
{

	int i;
	char temp_error_string[256];


	for (i = 1; i < argument_count; i++) {
		if( (strncmp(HOSTNAME_OPTION, argument_vector[i], strlen(HOSTNAME_OPTION) ) ) == 0) { 
			i++;
			if( i >= argument_count) {
				htxcmd_display_usage("Error : SUT hostname not provided with option");
				exit(1);
			}
			if(strlen(argument_vector[i]) > MAX_SUT_HOSTNAME_LENGTH) {
				sprintf(temp_error_string, "Error : SUT hostname exceeded maximum length limit value <%d>", MAX_SUT_HOSTNAME_LENGTH);
				htxcmd_display_usage(temp_error_string);
				exit(1);
			} 
			if(argument_vector[i][0] != '-') {
				strcpy(sut_hostname, argument_vector[i]);
				break;
			}
			else {
				htxcmd_display_usage("Error : Invalid SUT hostname");
				exit(1);
			}
		}
	}

	return sut_hostname;
}


/* get daemon port number from command arguments */
int htxcmd_get_daemon_port_number(int argument_count, char *argument_vector[], int *daemon_port_number)
{

	int i;


	for (i = 1; i < argument_count; i++) {
		if( (strncmp(PORT_NUMBER_OPTION, argument_vector[i], strlen(PORT_NUMBER_OPTION) ) ) == 0) { 
			i++;
			if( i >= argument_count) {
				htxcmd_display_usage("Error : Invalid daemon port number");
				exit(1);
			}
			if(argument_vector[i][0] != '-') {
				*daemon_port_number = atoi(argument_vector[i]);
				break;
			}
			else {
				htxcmd_display_usage("Error : Invalid daemon port number");
				exit(1);
			}
		}
	}
	
	return *daemon_port_number;
}



/* get command option name */
int htxcmd_get_command_name(int argument_count, char *argument_vector[], htxcmd_command *p_command_object)
{
	int i;
	char error_string[512];

	p_command_object->command_name[0] = '\0';

	for (i = 1; i < argument_count; i++) {
		if( 	( (strcmp(argument_vector[i], HOSTNAME_OPTION) ) == 0) 		||
			( (strcmp(argument_vector[i], PORT_NUMBER_OPTION) ) == 0) 	||
			( (strcmp(argument_vector[i], MDT_NAME_OPTION) ) == 0)		||
			( (strcmp(argument_vector[i], ECG_NAME_OPTION) ) == 0) ) {
			i++;
			continue;
		}
		strcpy(p_command_object->command_name, argument_vector[i]);
		p_command_object->command_argument_vector_position = i;
		break;
	}
	if(i >= argument_count) {
		htxcmd_display_usage("Error : no valid command option is provided");
		exit(1);
	}
	htxcmd_validate_command_name(p_command_object);

	i++;
	while(i < argument_count) {
		if( ( (strcmp(argument_vector[i], "-cmd")) != 0) && ( (strcmp(argument_vector[i], "-set_htx_env")) != 0)  ){
			if(argument_vector[i][0] == '-') {
				if( ( (strcmp(argument_vector[i], MDT_NAME_OPTION) ) != 0) && ( (strcmp(argument_vector[i], ECG_NAME_OPTION) ) != 0) ) {
					sprintf(error_string, "Error : invalid option %s is provided, please check usage", argument_vector[i]);
					htxcmd_display_usage(error_string);
					exit(1);	
				}
			}
		}
		i++;
	}

	return 0;
} 


/* get ecg name  from command provided */
char * htxcmd_get_ecg_name(int argument_count, char *argument_vector[], char *ecg_name)
{

	int i;
	char temp_error_string[512];

	ecg_name[0] = '\0';

	for (i = 1; i < argument_count; i++) {
		if( ( (strncmp(ECG_NAME_OPTION, argument_vector[i], strlen(ECG_NAME_OPTION) ) ) == 0) || ( (strncmp(MDT_NAME_OPTION, argument_vector[i], strlen(MDT_NAME_OPTION) ) ) == 0)){ 
			i++;
			if( i >= argument_count) {
				htxcmd_display_usage("Error : ECG name is not provided with <-ecg> option");
				exit(1);
			}
			if(strlen(argument_vector[i]) > MAX_ECG_NAME_LENGTH) {
				sprintf(temp_error_string, "Error : provided ECG name exceeded maximum length limit value <%d>", MAX_SUT_HOSTNAME_LENGTH);
				htxcmd_display_usage(temp_error_string);
				exit(1);
			} 
			if(argument_vector[i][0] != '-') {
				strcpy(ecg_name, argument_vector[i]);
				break;
			}
			else {
				htxcmd_display_usage("Error : invalid ECG name is provided");
				exit(1);
			}
		}
	}

	return ecg_name;
}



/* get option list for the command */
int htxcmd_get_argument_list(int argument_count, char *argument_vector[], htxcmd_command *p_command_object)
{
	int i;
	int option_list_length;

	p_command_object->option_list[0] = '\0';

	for (i = (p_command_object->command_argument_vector_position + 1); i < argument_count; i++) {
		if(argument_vector[i][0] == '-') {
			break;
		}
		strcat(p_command_object->option_list, argument_vector[i]);
		strcat(p_command_object->option_list, " ");
	}

	option_list_length = strlen(p_command_object->option_list);
	if(option_list_length > 0) {
		p_command_object->option_list[option_list_length - 1] = '\0';
	}


	return 0;
}

/* extracts command details and fill the values in command object */
int htxcmd_extract_command_details(int argument_count, char *argument_vector[], htxcmd_command *p_command_object)
{

	htxcmd_get_sut_hostname(argument_count, argument_vector, p_command_object->sut_hostname);
	htxcmd_get_daemon_port_number(argument_count, argument_vector, &(p_command_object->daemon_port_number) );
	htxcmd_get_command_name(argument_count, argument_vector, p_command_object);
	if( (strcmp(p_command_object->command_name, CMD_COMMAND)) !=0) {
		/* validate all options */
	}
	if(p_command_object->option_flag == TRUE) {
		htxcmd_get_ecg_name(argument_count, argument_vector, p_command_object->ecg_name);
		htxcmd_get_argument_list(argument_count, argument_vector, p_command_object);
	}


	return 0;
}



int htxcmd_parse_command(htxcmd_command *p_command_object, int argument_count, char *argument_vector[])
{
	int result;

	htxcmd_extract_command_details(argument_count, argument_vector, p_command_object);

	result = 0;	

	return result;
}
