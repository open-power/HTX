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
/* @(#)61	1.3  src/htx/usr/lpp/htx/bin/htxd/htxd_option_methods.h, htxd, htxubuntu 9/15/15 20:28:35 */



#ifndef HTXD__OPTION__METHODS__HEADER
#define HTXD__OPTION__METHODS__HEADER
#include <netinet/in.h>
#include <errno.h>

#include "htxd.h"


typedef struct
{
	htxd *htxd_instance;
	char command_ecg_name[MAX_ECG_NAME_LENGTH];
	char command_option_list[MAX_OPTION_LIST_LENGTH];
	char error_string[512];
	int return_code;
} htxd_option_method_object;


#define OPTION_METHOD_ARGS (char **)

//typedef int(*ecg_process_function)(char*);

extern void init_option_list(void);
extern int htxd_validate_command_requirements(htxd *, char *);
extern void htxd_init_option_method(htxd_option_method_object *);
extern int htxd_process_command(char **, int *);
//int htxd_process_all_active_ecg(ecg_process_function, char *);

extern int htxd_option_method_create_mdt OPTION_METHOD_ARGS;
extern int htxd_option_method_list_mdt OPTION_METHOD_ARGS;
extern int htxd_option_method_run_mdt OPTION_METHOD_ARGS;
extern int htxd_option_method_getactecg OPTION_METHOD_ARGS;
extern int htxd_option_method_shutdown_mdt OPTION_METHOD_ARGS;
extern int htxd_option_method_refresh OPTION_METHOD_ARGS;
extern int htxd_option_method_activate OPTION_METHOD_ARGS;
extern int htxd_option_method_suspend OPTION_METHOD_ARGS;
extern int htxd_option_method_terminate OPTION_METHOD_ARGS;
extern int htxd_option_method_restart OPTION_METHOD_ARGS;
extern int htxd_option_method_coe OPTION_METHOD_ARGS;
extern int htxd_option_method_soe OPTION_METHOD_ARGS;
extern int htxd_option_method_status OPTION_METHOD_ARGS;
extern int htxd_option_method_getstats OPTION_METHOD_ARGS;
extern int htxd_option_method_geterrlog OPTION_METHOD_ARGS;
extern int htxd_option_method_clrerrlog OPTION_METHOD_ARGS;
extern int htxd_option_method_cmd OPTION_METHOD_ARGS;
extern int htxd_option_method_set_eeh OPTION_METHOD_ARGS;
extern int htxd_option_method_set_kdblevel OPTION_METHOD_ARGS;
extern int htxd_option_method_set_hxecom OPTION_METHOD_ARGS;
extern int htxd_option_method_getvpd OPTION_METHOD_ARGS;
extern int htxd_option_method_getecgsum OPTION_METHOD_ARGS;
extern int htxd_option_method_getecglist OPTION_METHOD_ARGS;
extern int htxd_option_method_select_mdt OPTION_METHOD_ARGS;
extern int htxd_option_method_exersetupinfo OPTION_METHOD_ARGS;
extern int htxd_option_method_bootme OPTION_METHOD_ARGS;
extern int htxd_option_method_query OPTION_METHOD_ARGS;
extern int htxd_option_method_screen_2 OPTION_METHOD_ARGS;
extern int htxd_option_method_screen_4 OPTION_METHOD_ARGS;
extern int htxd_option_method_screen_5 OPTION_METHOD_ARGS;
extern int htxd_option_method_get_file OPTION_METHOD_ARGS;
extern int htxd_option_method_get_daemon_state OPTION_METHOD_ARGS;
extern int htxd_option_method_coe_soe OPTION_METHOD_ARGS;
extern int htxd_option_method_activate_halt OPTION_METHOD_ARGS;
extern int htxd_option_method_device_count OPTION_METHOD_ARGS;
extern int htxd_option_method_test_activate_halt OPTION_METHOD_ARGS;
extern int htxd_option_method_config_net OPTION_METHOD_ARGS;
extern int htxd_option_method_append_net_mdt OPTION_METHOD_ARGS;
extern int htxd_option_method_test_net OPTION_METHOD_ARGS;
extern int htxd_option_method_set_htx_env OPTION_METHOD_ARGS;
extern int htxd_option_method_get_htx_env OPTION_METHOD_ARGS;
extern int htxd_option_method_get_run_time OPTION_METHOD_ARGS;
extern int htxd_option_method_get_last_update_time OPTION_METHOD_ARGS;
extern int htxd_option_method_get_fail_status OPTION_METHOD_ARGS;
extern int htxd_option_method_get_dev_cycles OPTION_METHOD_ARGS;

#endif
