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
/* @(#)69	1.3  src/htx/usr/lpp/htx/bin/htxd/htxd_thread.c, htxd, htxubuntu 8/23/15 23:34:48 */

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "htxd_thread.h"
#include "htxd_trace.h"


extern volatile int htxd_shutdown_flag;
pthread_mutex_t htxd_trace_log_lock;
extern pthread_t global_main_thread_id;


int htxd_thread_create(htxd_thread *p_thread_info)
{	
	int return_code = 0;
	char error_string[256];
	sigset_t	thread_signal_mask;


	sigfillset(&thread_signal_mask);
	return_code = pthread_sigmask (SIG_BLOCK, &thread_signal_mask, NULL);
	if(return_code != 0) {
		sprintf(error_string, "pthread_sigmask failed while blocking with return value <%d> , errno <%d>", return_code, errno);
		HTXD_TRACE(LOG_ON, error_string);
		return return_code;
	}

	return_code = pthread_create(&(p_thread_info->thread_id), NULL, p_thread_info->thread_function, p_thread_info->thread_data);
	if(return_code != 0) {
		sprintf(error_string, "pthread_create failed with return value <%d> , errno <%d>", return_code, errno);
		HTXD_TRACE(LOG_ON, error_string);
		return return_code;
	}

	sigfillset(&thread_signal_mask);
	return_code = pthread_sigmask (SIG_UNBLOCK, &thread_signal_mask, NULL);
	if(return_code != 0) {
		sprintf(error_string, "pthread_sigmask failed while unblocking with return value <%d> , errno <%d>", return_code, errno);
		HTXD_TRACE(LOG_ON, error_string);
		return return_code;
	}

	return return_code;
}


int htxd_create_command_thread(htxd_thread *p_thread_info)
{	
	int		return_code = 0;
	pthread_attr_t	thread_attribute_value;
	char		error_string[256];
	sigset_t	thread_signal_mask;


	if(p_thread_info->thread_stack_size != 0) {
		return_code = pthread_attr_init(&thread_attribute_value);
		if(return_code != 0) {
			sprintf(error_string, "pthread_attr_init failed with return value <%d>", return_code);
			HTXD_TRACE(LOG_ON, error_string);
			return return_code;
		}

		return_code = pthread_attr_setdetachstate(&thread_attribute_value, PTHREAD_CREATE_DETACHED);
		if(return_code != 0) {
			sprintf(error_string, "pthread_attr_setdetachstate failed with return value <%d>", return_code);
			HTXD_TRACE(LOG_ON, error_string);
			return return_code;
		}  

		return_code = pthread_attr_setstacksize(&thread_attribute_value, p_thread_info->thread_stack_size);
		if(return_code != 0) {
			sprintf(error_string, "pthread_attr_setstacksize failed with return value <%d>", return_code);
			HTXD_TRACE(LOG_ON, error_string);
			return return_code;
		}

		sigfillset(&thread_signal_mask);
		return_code = pthread_sigmask (SIG_BLOCK, &thread_signal_mask, NULL);
		if(return_code != 0) {
			sprintf(error_string, "pthread_sigmask failed while blocking with return value <%d> , errno <%d>", return_code, errno);
			HTXD_TRACE(LOG_ON, error_string);
			return return_code;
		}

		return_code = pthread_create(&(p_thread_info->thread_id), &thread_attribute_value, p_thread_info->thread_function, p_thread_info->thread_data);
		if(return_code != 0) {
			sprintf(error_string, "pthread_create failed with return value <%d> , errno <%d>", return_code, errno);
			HTXD_TRACE(LOG_ON, error_string);
			return return_code;
		}

		sigfillset(&thread_signal_mask);
		return_code = pthread_sigmask (SIG_UNBLOCK, &thread_signal_mask, NULL);
		if(return_code != 0) {
			sprintf(error_string, "pthread_sigmask failed while unblocking with return value <%d> , errno <%d>", return_code, errno);
			HTXD_TRACE(LOG_ON, error_string);
			return return_code;
		}

	} else {
		sigfillset(&thread_signal_mask);
		return_code = pthread_sigmask (SIG_BLOCK, &thread_signal_mask, NULL);
		if(return_code != 0) {
			sprintf(error_string, "pthread_sigmask failed while blocking with return value <%d> , errno <%d>", return_code, errno);
			HTXD_TRACE(LOG_ON, error_string);
			return return_code;
		}

		return_code = pthread_create(&(p_thread_info->thread_id), NULL, p_thread_info->thread_function, p_thread_info->thread_data);
		if(return_code != 0) {
			sprintf(error_string, "pthread_create failed with return value <%d> , errno <%d>", return_code, errno);
			HTXD_TRACE(LOG_ON, error_string);
			return return_code;
		}

		sigfillset(&thread_signal_mask);
		return_code = pthread_sigmask (SIG_UNBLOCK, &thread_signal_mask, NULL);
		if(return_code != 0) {
			sprintf(error_string, "pthread_sigmask failed while unblocking with return value <%d> , errno <%d>", return_code, errno);
			HTXD_TRACE(LOG_ON, error_string);
			return return_code;
		}

	}

	return return_code;
}


void htxd_exit_command_thread(void)
{
/*	pthread_detach(pthread_self()); */
	pthread_exit(0);
}


int htxd_enable_thread_cancel_state_type(void)
{
	int old_value;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_value);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_value);

	return 0;
}


int htxd_thread_cancel(htxd_thread *p_thread_info)
{
	int return_code;

	return_code = pthread_cancel(p_thread_info->thread_id);

	return return_code;
}



int htxd_thread_join(htxd_thread *p_thread_info)
{
	int return_code;
	void *error_code;
	char trace_string[256];
	int temp_error_code;

	return_code = pthread_join(p_thread_info->thread_id, &error_code);
	if(return_code != 0) {
		memcpy(&temp_error_code, &error_code, sizeof(int));
		sprintf(trace_string, "htxd_thread_join: pthread_join returned with return code <%d>, error code <%d>", return_code, temp_error_code);
		HTXD_TRACE(LOG_ON, trace_string);
	}

	return return_code;
}



int htxd_thread_unblock_all_signals(void)
{
	int return_code = 0;
	sigset_t   thread_signal_mask;
	char error_string[256];


	sigfillset(&thread_signal_mask);
	return_code = pthread_sigmask (SIG_UNBLOCK, &thread_signal_mask, NULL);
	if(return_code != 0) {
		sprintf(error_string, "pthread_sigmask failed while unblocking with return value <%d> , errno <%d>", return_code, errno);
		HTXD_TRACE(LOG_ON, error_string);
	} 
	return return_code;
}


void htxd_trace_log_lock_init(void)
{
	pthread_mutex_init(&htxd_trace_log_lock, NULL);
}


void htxd_trace_log_lock_destroy(void)
{
	pthread_mutex_destroy(&htxd_trace_log_lock);
}


void *htxd_signal_handler_thread(void *thread_arg)
{
	int signal_received;
	int return_code;
	sigset_t *signal_set = thread_arg;
	char error_string[256];


	sprintf(error_string, "htxd_signal_handler_thread is started");
	HTXD_TRACE(LOG_ON, error_string);
	
	return_code = sigwait(signal_set, &signal_received);
	if(return_code != 0) {
		sprintf(error_string, "htxd_signal_handler_thread: sigwait returned with error code <%d>", return_code);
		HTXD_TRACE(LOG_ON, error_string);
	}

	if(signal_received == SIGTERM) {
		sprintf(error_string, "htxd_signal_handler_thread: received SIGTERM");
		HTXD_TRACE(LOG_ON, error_string);
		htxd_shutdown_flag = TRUE;	
		sleep(1);
		free(signal_set);
		sprintf(error_string, "htxd_signal_handler_thread: going to sent SIGINT and exit signal handler thread");
		HTXD_TRACE(LOG_ON, error_string);
		pthread_kill(global_main_thread_id, SIGINT);
	} else {
		sprintf(error_string, "htxd_signal_handler_thread: received unexpected signal <%d>, exiting signal handler thread", signal_received);
		HTXD_TRACE(LOG_ON, error_string);
	}

	return NULL;
}



void htxd_register_signal_handler_thread(void)
{
	sigset_t *signal_set;
	pthread_t signal_handler_thread_object;
	int return_code;
	char error_string[256];


	signal_set = (sigset_t *)malloc(sizeof(sigset_t));
	sigemptyset(signal_set);
	sigaddset(signal_set, SIGTERM);

	return_code = pthread_sigmask(SIG_BLOCK, signal_set, NULL);
	if(return_code != 0) {
		sprintf(error_string, "htxd_register_signal_handler_thread: pthread_sigmask returned with error code <%d>", return_code);
		HTXD_TRACE(LOG_ON, error_string);
	}

	return_code = pthread_create(&signal_handler_thread_object, NULL, htxd_signal_handler_thread, (void *)signal_set);
	if(return_code != 0) {
		sprintf(error_string, "htxd_register_signal_handler_thread: pthread_create returned with error code <%d>", return_code);
		HTXD_TRACE(LOG_ON, error_string);
	}

}
