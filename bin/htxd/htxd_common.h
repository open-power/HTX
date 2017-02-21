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
/* @(#)36	1.3  src/htx/usr/lpp/htx/bin/htxd/htxd_common.h, htxd, htxubuntu 9/15/15 20:27:57 */



/*
 * Note:	1. in case of any change in this file, please rebuild both htxd and htxcmd
		 	2. option_list defined in this header file is a global object, so it can not include in multiple file in a same program
        
 */

#ifndef HTXD__COMMON__HEADER
#define HTXD__COMMON__HEADER


#include "htxd_option.h"

/* one option entry : option name, option method, parameter list flag, running ecg list display flag, command type */
option option_list[] =
	{
		{"-createmdt", 0, FALSE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_IDLE},
		{"-listmdt", 0, FALSE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-run", 0, TRUE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_IDLE_SELECTED},
		{"-getactecg", 0, FALSE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-shutdown", 0, TRUE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-refresh", 0, FALSE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_IDLE},
		{"-activate", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-suspend", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-terminate", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_RUNNING},
		{"-restart", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_RUNNING},
		{"-coe", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-soe", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-status", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-getstats", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_RUNNING},
		{"-geterrlog", 0, FALSE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-clrerrlog", 0, FALSE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-cmd", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-set_eeh", 0, TRUE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_IDLE_SELECTED_RUNNING},
		{"-set_kdblevel", 0, TRUE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_IDLE_SELECTED_RUNNING},
		{"-set_hxecom", 0, TRUE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_IDLE_SELECTED_RUNNING},
		{"-getvpd", 0, FALSE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-getecgsum", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_RUNNING},
		{"-getecglist", 0, FALSE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-select", 0, TRUE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_IDLE},
		{"-exersetupinfo", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_RUNNING},
		{"-bootme", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-query", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-screen_2", 0, TRUE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-screen_4", 0, TRUE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-screen_5", 0, TRUE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-get_file", 0, TRUE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-get_daemon_state", 0, TRUE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-coe_soe", 0, TRUE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-activate_halt", 0, TRUE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_SELECTED_RUNNING},
		{"-device_count", 0, TRUE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-test_activate_halt", 0, FALSE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-config_net", 0, TRUE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-append_net_mdt", 0, TRUE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-test_net", 0, FALSE, 0, HTXSCREEN_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-set_htx_env", 0, TRUE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-get_htx_env", 0, TRUE, 0, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_ALL},
		{"-get_run_time", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_RUNNING},
		{"-get_last_update_time", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_RUNNING},
		{"-get_fail_status", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_RUNNING},
		{"-get_dev_cycles", 0, TRUE, 1, HTXCMDLINE_COMMAND, HTXD_COMMND_STATE_RUNNING}
	};

#endif

