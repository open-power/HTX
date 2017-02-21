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
/* @(#)52	1.1  src/htx/usr/lpp/htx/bin/htxd/htxd_option.h, htxd, htxubuntu 7/17/13 08:57:48 */



#ifndef HTXD__OPTION__HEADER
#define HTXD__OPTION__HEADER

#include "htxd.h"

#define HTXCMDLINE_COMMAND	1
#define HTXSCREEN_COMMAND	2

typedef struct
{
	char	option_string[128];
	int	(*option_method)(char **, htxd_command *);
	int		parameter_flag;
	int		running_ecg_display_flag;
	int	command_type;
	int	command_state;
}option;




#endif

