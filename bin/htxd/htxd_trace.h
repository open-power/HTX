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
/* @(#)71	1.2  src/htx/usr/lpp/htx/bin/htxd/htxd_trace.h, htxd, htxubuntu 8/23/15 23:34:51 */


#ifndef HTXD__TRACE__HEADER
#define HTXD__TRACE__HEADER


#include <pthread.h>
#include <stdio.h>

extern int htxd_trace_level;
extern FILE *htxd_trace_fp;
extern FILE *htxd_log_fp;
extern pthread_mutex_t htxd_trace_log_lock;

#define FUN_ENTRY	0
#define FUN_EXIT	1

#define TRACE_OFF	0
#define TRACE_ON	1
#define TRACE_DEBUG 2

#define LOG_ON		1	
#define LOG_OFF		0	

#define HTXD_TRACE(mode, trace_text) {\
	pthread_mutex_lock(&htxd_trace_log_lock);\
	if(mode == LOG_ON) fprintf(htxd_log_fp, "[htxd_log: %s]\n", trace_text);  fflush(htxd_log_fp); \
	if(htxd_trace_level == TRACE_ON) {\
		fprintf(htxd_trace_fp, "[htxd_trace: %s]\n", trace_text); fflush(htxd_trace_fp); \
	} else if(htxd_trace_level == TRACE_DEBUG) {\
		fprintf(htxd_trace_fp, "[file: %s] [line: %d] [htxd_trace: %s]\n", __FILE__, __LINE__, trace_text); fflush(htxd_trace_fp); \
	}\
	pthread_mutex_unlock(&htxd_trace_log_lock);\
}

#define HTXD_FUNCTION_TRACE(mode, function_name) {\
	pthread_mutex_lock(&htxd_trace_log_lock);\
	if(htxd_trace_level == TRACE_ON) {\
		fprintf(htxd_trace_fp, "[htxd_trace: %s%s]\n", (mode == 0 ? "ENTRY : " : "EXIT : "), function_name); fflush(htxd_trace_fp); \
	} else if(htxd_trace_level == TRACE_DEBUG) {\
		fprintf(htxd_trace_fp, "[file: %s] [line: %d] [htxd_trace: %s%s]\n", __FILE__, __LINE__, (mode == FUN_ENTRY ? "ENTRY : " : "EXIT : "), function_name); fflush(htxd_trace_fp); \
	}\
	pthread_mutex_unlock(&htxd_trace_log_lock);\
}	

#endif

