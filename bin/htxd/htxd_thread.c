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

#include "htxd_thread.h"
#include "htxd_trace.h"


int htxd_thread_create(htxd_thread *p_thread_info)
{	
	int return_code = 0;
	sigset_t   thread_signal_mask;
	char error_string[256];


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

	return_code = pthread_join(p_thread_info->thread_id, &error_code);
	if(return_code != 0) {
		sprintf(trace_string, "htxd_thread_join: pthread_join returned with return code <%d>, error code <%d>", return_code, (int) error_code);
		HTXD_TRACE(LOG_ON, trace_string);
	}

	return return_code;
}



