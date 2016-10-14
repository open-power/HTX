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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "htxd_define.h"
#include "htxd_util.h"
#include "htxd_trace.h"
#include "htxd_profile.h"
#include "htxd_instance.h"
#include "htxd_signal.h"


extern int htxd_option_method_run_mdt(char **);

int htxd_get_autostart_mdt_name(char *flagfile, char *mdt_name)
{
	FILE *p_flag_file;
	char trace_str[256];



	p_flag_file = fopen(flagfile, "r");
	if(p_flag_file == NULL) {
		sprintf(trace_str, "fopen failed with errno = <%d>", errno);
		HTXD_TRACE(LOG_OFF, trace_str);
		return -1;
	}
	fscanf(p_flag_file, "%s", mdt_name);
	fclose(p_flag_file);
	return 0;
}



int htxd_autostart(htxd *htxd_instance)
{
	int return_code;
	char *result_str = NULL;
	char trace_str[256];
	char autostart_mdt_name[256] = {'\0'};
	char temp_string[300];



	sprintf(temp_string, "%s/%s", global_htx_home_dir, HTXD_AUTOSTART_FILE);
	return_code = htxd_is_file_exist(temp_string);
	if(return_code == FALSE) {
		sprintf(trace_str, "auto start mode is not enabled, continue normal mode");
		HTXD_TRACE(LOG_OFF, trace_str);
		return -1;
	} else {
		sprintf(trace_str, "found autostart flag file <%s>, start with auto start mode", temp_string);
		HTXD_TRACE(LOG_OFF, trace_str);
	}
	
	return_code = htxd_get_autostart_mdt_name(temp_string, autostart_mdt_name);
	if(return_code != 0) {
		sprintf(trace_str, "failed to get auto start MDT name, htxd_get_autostart_mdt_name retuned with <%d>", return_code);
		HTXD_TRACE(LOG_OFF, trace_str); 
		return return_code;
	}
	if( strlen(autostart_mdt_name) < 1) {
		sprintf(trace_str, "auto start MDT name is not present in <%s>, continue normal mode", temp_string);
		HTXD_TRACE(LOG_OFF, trace_str);
		return -1;
	}	
	sprintf(trace_str, "auto start MDT name <%s>", autostart_mdt_name);
	HTXD_TRACE(LOG_ON, trace_str)

#ifdef __HTX_LINUX__
	return_code = htxd_execute_shell_profile();
	if(return_code != 0) {
		sprintf(trace_str, "MDT creation failed with error code <%d> whlie doing autostart", return_code);
		HTXD_TRACE(LOG_ON, trace_str);
		return -1;
	}

	if(strcmp( basename(autostart_mdt_name), "net.mdt") != 0) {
		sprintf(trace_str, "%s/etc/scripts/htxd_bootme net", global_htx_home_dir);
		system(trace_str);
	}
 #endif	

	if(htxd_is_profile_initialized(htxd_instance) != TRUE) {
		HTXD_TRACE(LOG_ON, "initialize HTX profile details from auto start");
		htxd_init_profile(&(htxd_instance->p_profile));
		register_signal_handlers();	
	}

	htxd_set_command_ecg_name(autostart_mdt_name);

	return_code = htxd_option_method_run_mdt(&result_str);
	if(return_code != 0) {
		sprintf(trace_str, "autostart failed to start MDT <%s>, result str<%s>, return code <%d>", autostart_mdt_name, result_str, return_code);
		HTXD_TRACE(LOG_ON, trace_str);
	} else {
		sprintf(trace_str, "autostart successfully started  MDT <%s>", autostart_mdt_name);
		HTXD_TRACE(LOG_ON, trace_str);
	}

	if(result_str != NULL) {
		free(result_str);
	}

	return 0;
}
