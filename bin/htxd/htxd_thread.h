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
/* @(#)70	1.2  src/htx/usr/lpp/htx/bin/htxd/htxd_thread.h, htxd, htxubuntu 9/2/14 09:03:34 */



#ifndef __HTXD_THREAD__
#define __HTXD_THREAD__


#include <pthread.h>

typedef struct{
	pthread_t	thread_id;
	void		*(*thread_function)(void *);
	void		*thread_data;
	int		thread_stack_size;
	} htxd_thread;



extern int htxd_thread_create(htxd_thread *);
extern int htxd_thread_cancel(htxd_thread *);
extern int htxd_enable_thread_cancel_state_type(void);
extern int htxd_thread_join(htxd_thread *);
extern int htxd_start_hang_monitor(htxd_thread **);
extern int htxd_start_time_driven_run_monitor(htxd_thread **);
extern int htxd_stop_hang_monitor(htxd_thread **);
extern int htxd_thread_unblock_all_signals(void);
extern void htxd_trace_log_lock_init(void);
extern void htxd_trace_log_lock_destroy(void);
extern int htxd_create_command_thread(htxd_thread *p_thread_info);
extern void htxd_exit_command_thread(void);
extern void htxd_register_signal_handler_thread(void);
extern void htxd_exit_autostart_thread(void);


#endif
