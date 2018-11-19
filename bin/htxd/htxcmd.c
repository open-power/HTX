/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* htxubuntu src/htx/usr/lpp/htx/bin/htxd/htxcmd.c 1.13                   */
/*                                                                        */
/* Licensed Materials - Property of IBM                                   */
/*                                                                        */
/* COPYRIGHT International Business Machines Corp. 2013,2018              */
/* All Rights Reserved                                                    */
/*                                                                        */
/* US Government Users Restricted Rights - Use, duplication or            */
/* disclosure restricted by GSA ADP Schedule Contract with IBM Corp.      */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
/* %Z%%M%	%I%  %W% %G% %U% */



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
	printf("\t-set_eeh [<EEH flag( 1 or 0)>]\n");
	printf("\t-set_kdblevel [<kdb level flag ( 1 or 0)>]\n");
	printf("\t-set_htx_env <variable> <value>\n");
	printf("\t-get_htx_env <variable>\n");
	printf("\t-bootme [ on | off | status | help ]\n");
	printf("\t-createmdt\n");
	printf("\t-listmdt\n");
	printf("\t-refresh\n");
	printf("\t-select [-mdt <mdt_name>]\n");
	printf("\t-run [test_time=<time in seconds>] [-mdt <mdt_name>]\n");
	printf("\t-shutdown\n");
	printf("\t-getactmdt\n");
	printf("\t-query [<device_name1> <device_name2> ...]\n");
	printf("\t-activate [<device_name1> <device_name2> ...]\n");
	printf("\t-suspend [<device_name1> <device_name2> ...]\n");
	printf("\t-terminate [<device_name1> <device_name2> ...]\n");
	printf("\t-restart [<device_name1> <device_name2> ...]\n");
	printf("\t-coe [<device_name1> <device_name2> ...]\n");
	printf("\t-soe [<device_name1> <device_name2> ...]\n");
	printf("\t-status [<device_name1> <device_name2> ...]\n");
	printf("\t-getstats\n");
	printf("\t-getmdtsum \n");
	printf("\t-exersetupinfo\n");
	printf("\t-getmdtlist\n");
	printf("\t-geterrlog\n");
	printf("\t-clrerrlog\n");
	printf("\t-getvpd\n");
	printf("\t-get_fail_status\n");
	printf("\t-get_run_time\n");
	printf("\t-get_dev_cycles <device_name_prefix>\n");
	printf("\t-get_last_update_time\n");
	printf("\t-cmd <command to execute>\n");
	printf("\nPlease Note: -ecg option is supported for backward compatibility, -mdt option is preferred\n\n");

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
