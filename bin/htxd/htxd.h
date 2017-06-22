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
/* @(#)35	1.5  src/htx/usr/lpp/htx/bin/htxd/htxd.h, htxd, htxubuntu 11/24/15 23:59:18 */



#ifndef HTXD__HEADER
#define HTXD__HEADER


#include <unistd.h>

#include "htxd_profile.h"
#include "htxd_ecg.h"
#include "htxd_common_define.h"
#include "htxd_thread.h"
#include "htxd_define.h"


#define		HTXD_LIGHTWEIGHT		10
#define		HTXD_SHUTDOWN			20


#define		HTXD_TRACE_NO			0
#define		HTXD_TRACE_LOW			1
#define		HTXD_TRACE_MEDIUM		2
#define		HTXD_TRACE_HIGH			3


typedef struct
{
	int	command_index;
	int	command_type;
	char	ecg_name[MAX_ECG_NAME_LENGTH];
	char	option_list[MAX_OPTION_LIST_LENGTH];
} htxd_command;



typedef struct
{
	htxd_ecg_manager *	p_ecg_manager;
	htxd_thread *		p_hang_monitor_thread;
	htxd_thread *		p_hotplug_monitor_thread;
	htxd_thread 		stop_watch_monitor_thread;
	char		program_name[80];
	char		htx_path[80];
	int			shutdown_flag;
	pid_t		daemon_pid;
	pid_t		htx_msg_pid;
	pid_t		htx_stats_pid;
	pid_t		dr_child_pid;
	pid_t		equaliser_pid;
	int			run_level;
	int			run_state;
	int			trace_level;
	int			port_number;
	pid_t       * p_child_pid_list;
	htxd_profile *		p_profile;
	int			dr_sem_key;
	int			dr_sem_id;
	int			dr_reconfig_restart;
	int			dr_is_done; /* check usage */
	int 		is_dr_test_on;/*flag will set to 1 if user exported HTX_DR_TEST=1 env variable*/
	int			equaliser_debug_flag;
	int         equaliser_time_quantum;
	int         equaliser_startup_time_delay;
	int         equaliser_log_duration;
	int         equaliser_pattern_length;
	int         enable_offline_cpu;
	union       shm_pointers	equaliser_shm_addr;
	int			equaliser_semhe_id;
	char *		equaliser_conf_file;
	char *      equaliser_sys_cpu_util;
	int			is_auto_started;
	int			init_syscfg_flag;
	int			master_client_mode;
	int			is_test_active;
	int			is_mdt_created;
	int			is_stop_watch_monitor_initailized;
} htxd;

extern htxd *htxd_global_instance;

extern int htxd_start_daemon(htxd*);

#ifdef __HTX_LINUX__
	extern int smt, bind_th_num;
#endif

#endif
