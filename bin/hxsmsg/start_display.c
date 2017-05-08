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

/* @(#)18	1.2  src/htx/usr/lpp/htx/bin/hxsmsg/start_display.c, htx_msg, htxubuntu 5/24/04 13:39:43 */

#include <unistd.h>
#include "hxsmsg.h"

int start_display_device(void)
{
	pid_t pid;
	if((pid = fork()) == 0)
	{
		execl("/usr/bin/htx/bin/nf", "/usr/bin/htx/bin/nf", (char *) NULL);
		exit(1);
	}
	return(GOOD);
}
