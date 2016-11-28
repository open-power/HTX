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
/* @(#)19	1.1  src/htx/usr/lpp/htx/bin/htxd/htxcmd.h, htxd, htxubuntu 7/17/13 01:58:48 */



#ifndef HTXCMD__HEADER
#define HTXCMD__HEADER

#include "htxd_common_define.h"

#define COMMAND_LENGTH 1024

#define DEFAULT_SUT_HOSTNAME "localhost"

#define HOSTNAME_OPTION			"-sut"
#define PORT_NUMBER_OPTION		"-port"
#define ECG_NAME_OPTION			"-ecg"
#define MDT_NAME_OPTION			"-mdt"

#define CMD_COMMAND "-cmd"

#define MAX_SUT_HOSTNAME_LENGTH 256
#define MAX_COMMAND_NAME_LENGTH 50
#define COMMAND_STRING_LENGTH (MAX_COMMAND_NAME_LENGTH + MAX_ECG_NAME_LENGTH + MAX_OPTION_LIST_LENGTH + 1)

typedef struct 
{
	char	sut_hostname[MAX_SUT_HOSTNAME_LENGTH];
	int		daemon_port_number;
	char	command_name[MAX_COMMAND_NAME_LENGTH];
	int		command_index;
	char	ecg_name[MAX_ECG_NAME_LENGTH];
	int		option_flag;
	char	option_list[MAX_ECG_NAME_LENGTH];
	int		command_argument_vector_position;
	
}htxcmd_command;

typedef struct
{
	char	*response_buffer;
	int		return_code;
}htxcmd_response;

extern void initialize_command_object(htxcmd_command *);
extern void initialize_response_object(htxcmd_response *);
extern int htxcmd_parse_command(htxcmd_command *, int, char *[]);
extern int htxcmd_process_command(htxcmd_command *, htxcmd_response *);
extern void htxcmd_display_result(char *);
extern void htxcmd_display_command_object(htxcmd_command *);
extern void htxcmd_display_usage(char *);


#endif
