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
/* @(#)54	1.5  src/htx/usr/lpp/htx/bin/htxd/htxd_option_method_create_mdt.c, htxd, htxubuntu 11/9/15 10:26:37 */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "htxd_define.h"
#include "htxd_util.h"
#include "htxd_trace.h"
#include "htxd.h"
#include "htxd_instance.h"


int htxd_option_method_create_mdt(char **result_string, htxd_command *p_command)
{
	int return_code;
	char trace_string[300];
	int stanza_count;
	FILE *fp;
	htxd *htxd_instance;


	htxd_set_daemon_state(HTXD_DAEMON_STATE_CREATING_MDT);
	htxd_instance = htxd_get_instance();
	if(htxd_instance->is_mdt_created == 0) {
		htxd_instance->is_mdt_created = 1;
	}
	*result_string = malloc(512);
	if(*result_string == NULL) {
		sprintf(trace_string, "Error: malloc failed with errno <%d>", errno);	
		HTXD_TRACE(LOG_ON, trace_string);
		htxd_set_daemon_state(HTXD_DAEMON_STATE_CREATING_MDT);
		return -1;
	}


	return_code = htxd_execute_shell_profile();
	sprintf(trace_string, "grep HE_name %s/mdt/mdt.bu 2>/dev/null | wc -l", global_htx_home_dir);
	fp = popen(trace_string, "r");
	if(fp == NULL) {
		sprintf(trace_string, "Error: popen failed with errno <%d>", errno);
		strcpy(*result_string, trace_string);
		HTXD_TRACE(LOG_ON, trace_string);
		htxd_set_daemon_state(HTXD_DAEMON_STATE_IDLE);
		return -1;
	}

	fscanf(fp, "%d", &stanza_count);
	return_code = pclose(fp);
	if(return_code == -1 && errno != 10) {
		sprintf(trace_string, "Error: pclose failed with errno <%d>", errno);
		strcpy(*result_string, trace_string);
		HTXD_TRACE(LOG_ON, trace_string);
		htxd_set_daemon_state(HTXD_DAEMON_STATE_IDLE);
		return -1;
	}
	
	if(stanza_count > 2) {	
		sprintf(*result_string, "mdts are created successfully.");
		strcpy(trace_string, *result_string);
		HTXD_TRACE(LOG_ON, trace_string);
	} else {
		sprintf(*result_string, "Error: mdt creation is failed, please check error at log file <%s/htxd_bash_profile_output>.", global_htxd_log_dir);
		strcpy(trace_string, *result_string);
		HTXD_TRACE(LOG_ON, trace_string);
		htxd_set_daemon_state(HTXD_DAEMON_STATE_IDLE);
		return -1;
	}

	htxd_set_daemon_state(HTXD_DAEMON_STATE_IDLE);

	return 0;
}
