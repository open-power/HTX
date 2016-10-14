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
/* @(#)18	1.5  src/htx/usr/lpp/htx/bin/htxd/htxcmd.c, htxd, htxubuntu 11/4/15 01:02:46 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "htxcmd.h"



/* htx command line usage */
void htxcmd_display_usage(char *error_text)
{
	if(error_text != NULL) {
		printf("%s\n", error_text);
	}
	printf("Usage: htxcmdline [-sut <host name>] [-port <port number>] OPTIONS\n");
	printf("\tFollowing are the OPTIONS\n");
	printf("\t-set_eeh [<EEH flag( 1 0r 0)>]\n");
	printf("\t-set_kdblevel [<kdb level flag ( 1 or 0)>]\n");
	printf("\t-set_htx_env <variable> <value>\n");
	printf("\t-get_htx_env <variable>\n");
	printf("\t-bootme [ on | off | status ]\n");
	printf("\t-createmdt\n");
	printf("\t-listmdt\n");
	printf("\t-refresh\n");
	printf("\t-select [-mdt|-ecg <mdt_name>]\n");
	printf("\t-run [-mdt|-ecg <mdt_name>]\n");
	printf("\t-shutdown [-mdt|-ecg <mdt_name>]\n");
	printf("\t-getactecg\n");
	printf("\t-query [<device_name1> <device_name2> ...] [-mdt|-ecg <mdt_name>]\n");
	printf("\t-activate [<device_name1> <device_name2> ...] [mdt|-ecg <mdt_name>]\n");
	printf("\t-suspend [<device_name1> <device_name2> ...] [-mdt|-ecg <mdt_name>]\n");
	printf("\t-terminate [<device_name1> <device_name2> ...] [-mdt|-ecg <mdt_name>]\n");
	printf("\t-restart [<device_name1> <device_name2> ...] [-mdt|-ecg <mdt_name>]\n");
	printf("\t-coe [<device_name1> <device_name2> ...] [-mdt|-ecg <mdt_name>]\n");
	printf("\t-soe [<device_name1> <device_name2> ...] [-mdt|-ecg <mdt_name>]\n");
	printf("\t-status [<device_name1> <device_name2> ...] [-mdt|-ecg <mdt_name>]\n");
	printf("\t-getstats [-mdt|-ecg <mdt_name>]\n");
	printf("\t-getecgsum [-mdt|-ecg <met_name>]\n");
	printf("\t-exersetupinfo [-mdt|-ecg <mdt_name>]\n");
	printf("\t-getecglist\n");
	printf("\t-geterrlog\n");
	printf("\t-clrerrlog\n");
	printf("\t-getvpd\n");
	printf("\t-cmd <command to execute>\n");
	printf("\nPlease Note: -ecg option is provided for backward compatibility, -mdt option is preferred\n\n");

}



/* htx command line main() */
int main(int argc, char *argv[])
{
	htxcmd_command	command;
	htxcmd_response	response;

	initialize_command_object(&command);	
	initialize_response_object(&response);	
	htxcmd_parse_command(&command, argc, argv);
	/* htxcmd_display_command_object(&command);  */ /* DEBUG */

	htxcmd_process_command(&command, &response);

	htxcmd_display_result(response.response_buffer);

	if(response.response_buffer != NULL) {
		free(response.response_buffer);
	}
	
	return response.return_code;
}
