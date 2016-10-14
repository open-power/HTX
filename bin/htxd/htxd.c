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
/* @(#)34	1.5  src/htx/usr/lpp/htx/bin/htxd/htxd.c, htxd, htxubuntu 8/4/15 03:37:17 */



#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>


#include "htxd.h"
#include "htxd_instance.h"
#include "htxd_trace.h"
#include "htxd_thread.h"
#include "htxd_socket.h"


htxd *htxd_global_instance = NULL;
volatile int htxd_shutdown_flag = FALSE;
volatile int htxd_ecg_shutdown_flag = FALSE;

char global_htx_home_dir[256]={'\0'};
char global_htx_log_dir[256] = "/tmp";
char global_htxd_log_dir[256] = "/tmp";
int htxd_trace_level = 2;
FILE *htxd_trace_fp;
FILE *htxd_log_fp;

#ifdef __HTX_LINUX__
	int smt, bind_th_num;
#endif

int htxd_validate_arguments(int argc, char *argv[], htxd *p_htxd_instance)
{
	int arg_option;
	int ret = 0;;


	while ((arg_option = getopt(argc, argv, "ast:")) != -1)
	{
    	switch(arg_option)
		{
		case 's':
			p_htxd_instance->run_level = HTXD_SHUTDOWN;
			break;
		case 't':
			p_htxd_instance->trace_level = atoi(optarg);
			if(p_htxd_instance->trace_level < HTXD_TRACE_NO || p_htxd_instance->trace_level > HTXD_TRACE_HIGH)
			{
				return -1;
			}
			break;
		case 'p':
			p_htxd_instance->port_number = atoi(optarg);
		default :
			ret = -1;
		}
	}

	return ret;
}


void* htxd_heart_beat_monitor(void* vptr)
{

	int heart_beat_socket_fd;
	struct sockaddr_in      local_address;
	struct sockaddr_in      client_address;
	socklen_t                       address_length;
	int result;
	char receive_buffer[2];
	char response_buffer[2];
	int new_fd;
	
	

	
	heart_beat_socket_fd = htxd_create_socket();

	local_address.sin_family = AF_INET;
	local_address.sin_port = htons ((htxd_global_instance->port_number) + 1);
	local_address.sin_addr.s_addr = INADDR_ANY;
	memset (&(local_address.sin_zero), '\0', 8);
	
	result = htxd_bind_socket(heart_beat_socket_fd, &local_address, htxd_global_instance->port_number + 1);

	result = htxd_listen_socket(heart_beat_socket_fd);

	do {

		do {
			result = htxd_select_timeout(heart_beat_socket_fd, 10);
			if(htxd_shutdown_flag == TRUE) {
				break;
			}

			if( result == 0) {
				htxd_global_instance->master_client_mode = 0;
				continue;
			}

		} while( (result == -1) && (errno == EINTR) );
		if(htxd_shutdown_flag == TRUE) {
			break;
		}

		new_fd = htxd_accept_connection(heart_beat_socket_fd, &client_address, &address_length);
		if(new_fd == -1 || new_fd == 0) {
			if(htxd_shutdown_flag == TRUE) {
				break;
			}

			continue;
		}

		memset(receive_buffer, '0', sizeof(receive_buffer) );
		result = htxd_receive_bytes(new_fd, receive_buffer, 1);
		if(receive_buffer[0] == '1') {
			if(htxd_global_instance->master_client_mode == 0) {
				htxd_global_instance->master_client_mode = 1;
				response_buffer[0] = '1';
			}else {
				response_buffer[0] = '0';
			}
		} else {
			if(htxd_global_instance->master_client_mode == 1) {
				htxd_global_instance->master_client_mode = 0;
			}
		}

		result = htxd_send_heart_beat_response(new_fd, response_buffer);
		
		close(new_fd);	
	}while(htxd_shutdown_flag == FALSE);

	close(heart_beat_socket_fd);

	return NULL;
}


int htxd_start_heart_beat_monitor(void)
{

	htxd_thread heart_beat_monitor_thread;
	int return_code;

	memset(&heart_beat_monitor_thread, 0, sizeof(htxd_thread));
	heart_beat_monitor_thread.thread_function = htxd_heart_beat_monitor;

	return_code = htxd_thread_create(&heart_beat_monitor_thread);

	return return_code;
	
}


int main(int argc, char *argv[])
{
	int ret = 0;
	char trace_str[512];
	char	*temp_env_val_ptr;
	char	temp_str[300];
	

	temp_env_val_ptr = getenv("HTX_HOME_DIR");
	if( (temp_env_val_ptr == NULL ) || (strlen(temp_env_val_ptr) == 0) ) {
		printf("ERROR: system environment variable HTX_HOME_DIR is not, please export HTX_HOME_DIR with value as HTX installation directory and re-try...\n");
		printf("exiting...\n\n");
		exit(1);
	} else if( (temp_env_val_ptr != NULL) && (strlen(temp_env_val_ptr) > 0) ) {
		strcpy(global_htx_home_dir, temp_env_val_ptr);	
	}

	ret = htxd_verify_home_path();
	if(ret != 0) {
		 printf("ERROR: system environment variable HTX_HOME_DIR is set with value <%s>, which is not HTX installation path, please export HTX_HOME_DIR with HTX installation path and re-try...\n", global_htx_home_dir);
		printf("exiting...\n\n");
		exit(2);
	}
	
	temp_env_val_ptr = getenv("HTX_LOG_DIR");
	if( (temp_env_val_ptr != NULL) && (strlen(temp_env_val_ptr) > 0) ) {
		strcpy(global_htx_log_dir, temp_env_val_ptr);
		sprintf(global_htxd_log_dir, "%s/htxd", global_htx_log_dir);
	}
	sprintf(temp_str, "mkdir -p %s > /dev/null 2>&1", global_htxd_log_dir);
	system(temp_str);

	sprintf(trace_str, "date +\"HTX Daemon (htxd) was started on %%x at %%X %%Z\">> %s/%s", global_htxd_log_dir, HTXD_START_STOP_LOG);
	system(trace_str);

	sprintf(temp_str, "%s/%s", global_htxd_log_dir, HTXD_TRACE_LOG);
	htxd_trace_fp = fopen(temp_str, "w");

	sprintf(temp_str, "%s/%s", global_htxd_log_dir, HTXD_LOG_FILE);
	htxd_log_fp = fopen(temp_str, "w");

	HTXD_FUNCTION_TRACE(FUN_ENTRY, "main");

	htxd_global_instance = htxd_create_instance();
	init_htxd_instance(htxd_global_instance);

	HTXD_TRACE(LOG_ON, "validate arguments to daemon launcher");
	ret = htxd_validate_arguments(argc, argv, htxd_global_instance);
	if (ret == -1)
	{
		HTXD_TRACE(LOG_ON, "htxd_validate_arguments() failed, exiting");
		exit(1);
	}

	if(htxd_global_instance->run_level == HTXD_SHUTDOWN)
	{
/* shutdown all currently running ECGc */
		exit(0);
	}

#if !defined(__HTX_LINUX__) && !defined(__OS400__)
	system("errlogger --- HTXD Started ---");
#endif

	HTXD_TRACE(LOG_ON, "forking daemon process");
	htxd_global_instance->daemon_pid = fork();
	switch (htxd_global_instance->daemon_pid)
	{
	case -1:
		sprintf (trace_str, "ERROR: fork failed with errno = %d", errno);
		HTXD_TRACE(LOG_ON, trace_str);
		exit (-1);
	case 0:
		setpgrp();
#ifndef __HTX_LINUX__
		setpriority(PRIO_PROCESS, 0, -1);
#endif
		htxd_set_program_name(argv[0]);
		htxd_set_htx_path(global_htx_home_dir);

		htxd_start_heart_beat_monitor();

		HTXD_TRACE(LOG_ON, "starting daemon function");
		htxd_start_daemon(htxd_global_instance);
		break;

	default:
		HTXD_TRACE(LOG_ON, "daemon launcher process is about to exit");
		return 0;
	}

	sprintf(trace_str, "date +\"HTX Daemon (htxd) was stopped on %%x at %%X %%Z\">>%s/%s", global_htxd_log_dir, HTXD_START_STOP_LOG);
	system(trace_str);
	HTXD_FUNCTION_TRACE(FUN_EXIT, "main");
	return 0;
}




