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

/* one option entry : option name, option method, parameter list flag, running ecg list display flag */
option option_list[] =
	{
		{"-createmdt", 0, FALSE, 0, HTXCMDLINE_COMMAND},
		{"-listmdt", 0, FALSE, 0, HTXCMDLINE_COMMAND},
		{"-run", 0, TRUE, 0, HTXCMDLINE_COMMAND},
		{"-getactecg", 0, FALSE, 1, HTXCMDLINE_COMMAND},
		{"-shutdown", 0, TRUE, 0, HTXCMDLINE_COMMAND},
		{"-refresh", 0, FALSE, 1, HTXCMDLINE_COMMAND},
		{"-activate", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-suspend", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-terminate", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-restart", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-coe", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-soe", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-status", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-getstats", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-geterrlog", 0, FALSE, 1, HTXCMDLINE_COMMAND},
		{"-clrerrlog", 0, FALSE, 1, HTXCMDLINE_COMMAND},
		{"-cmd", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-set_eeh", 0, TRUE, 0, HTXCMDLINE_COMMAND},
		{"-set_kdblevel", 0, TRUE, 0, HTXCMDLINE_COMMAND},
		{"-set_hxecom", 0, TRUE, 0, HTXCMDLINE_COMMAND},
		{"-getvpd", 0, FALSE, 1, HTXCMDLINE_COMMAND},
		{"-getecgsum", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-getecglist", 0, FALSE, 1, HTXCMDLINE_COMMAND},
		{"-select", 0, TRUE, 0, HTXCMDLINE_COMMAND},
		{"-exersetupinfo", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-bootme", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-query", 0, TRUE, 1, HTXCMDLINE_COMMAND},
		{"-screen_2", 0, TRUE, 0, HTXSCREEN_COMMAND},
		{"-screen_4", 0, TRUE, 0, HTXSCREEN_COMMAND},
		{"-screen_5", 0, TRUE, 0, HTXSCREEN_COMMAND},
		{"-get_file", 0, TRUE, 0, HTXCMDLINE_COMMAND},
		{"-get_daemon_state", 0, TRUE, 0, HTXSCREEN_COMMAND},
		{"-coe_soe", 0, TRUE, 0, HTXSCREEN_COMMAND},
		{"-activate_halt", 0, TRUE, 0, HTXSCREEN_COMMAND},
		{"-device_count", 0, TRUE, 0, HTXSCREEN_COMMAND},
		{"-test_activate_halt", 0, FALSE, 0, HTXSCREEN_COMMAND},
		{"-config_net", 0, TRUE, 0, HTXSCREEN_COMMAND},
		{"-append_net_mdt", 0, TRUE, 0, HTXSCREEN_COMMAND},
		{"-test_net", 0, FALSE, 0, HTXSCREEN_COMMAND},
		{"-set_htx_env", 0, TRUE, 0, HTXCMDLINE_COMMAND},
		{"-get_htx_env", 0, TRUE, 0, HTXCMDLINE_COMMAND}
	};

#endif

