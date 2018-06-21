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
/* @(#)46	1.2  src/htx/usr/lpp/htx/bin/htxd/htxd_exer.c, htxd, htxubuntu 8/23/15 23:34:21 */



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "htxd_define.h"
#include "hxihtx.h"
#include "htxd_trace.h"


static char     *script_name;

int run_cmd(char *cmd_string, char *cmd_output, int *ct_output,
			char *cmdout_filename)
{
    FILE    *cmdstdio;	/* pipe fd */
    char 	msg_text[MAX_TEXT_MSG];
    char    *p_cmdout, cmdout[256], *p_cmdOutput;
    int 	ct_cmdOutput = 0; /* counter for global output chars */
    int 	size_of_outbuf;
    int 	rc;
    struct stat out_stat;
//    DBTRACE(DBENTRY,("enter R_device.c run_cmd\n"));

/*
 ********  Begin executable code ********
 */

    size_of_outbuf = *ct_output;
    if (cmd_output != NULL) bzero(cmd_output, size_of_outbuf);
    p_cmdOutput = cmd_output;

	/* run the command with system() */
    //system_call = TRUE;
    rc = system(cmd_string);
    //system_call = FALSE;
	/* check rc for system here */
    if (rc == -1 || rc == 127 ) { /* some problem system() */
//	DBTRACE(DBEXIT,("return/a -128 R_device.c run_cmd\n"));
	return -128;
    } /* if */

    if (WIFSIGNALED(rc)) {  /* script killed by a signal */
	(void) sprintf(msg_text,
		       "Command killed by signal %d.",
		       WTERMSIG(rc));
//	(void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
	rc = -WTERMSIG(rc);
    } /* if */
    else
	rc = WEXITSTATUS(rc);

    if (cmd_output != NULL && cmdout_filename != NULL) {

		/* open output file */
	cmdstdio = fopen(cmdout_filename, "r");
	if (cmdstdio == NULL) {
	    (void) sprintf(msg_text,
			   "Failed to open command output file: %s.   errno = %d\n",
			   cmdout_filename, errno);
//	    (void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
//	    DBTRACE(DBEXIT,("return/b -129 R_device.c run_cmd\n"));
	    return -129;
	}


	if (stat(cmdout_filename, &out_stat) == -1) {
	    (void) sprintf(msg_text, "Error getting the stat data for %s. errno=%d (%s)\n",cmdout_filename, errno, strerror(errno));
//	    print_log(LOGMSG,"%s\n", msg_text);
//	    (void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
	    fclose(cmdstdio);
//	    DBTRACE(DBEXIT,("return/c -140 R_device.c run_cmd\n"));
	    return -140;
	}
	else if (out_stat.st_size<=0) {
	    (void) sprintf(msg_text, "Output file size found = %d. \n",(int)out_stat.st_size);
//	    print_log(LOGMSG,"%s", msg_text);
//	    (void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
	    fclose(cmdstdio);
//	    DBTRACE(DBEXIT,("return/d -141 R_device.c run_cmd\n"));
	    return -141;
	}

		/* read the output from the command until EOF or error */
	while (  NULL != (p_cmdout = fgets(cmdout,255, cmdstdio)) ) {
	    ct_cmdOutput = ct_cmdOutput + strlen(p_cmdout);
	    ct_cmdOutput = (ct_cmdOutput >= size_of_outbuf) ? size_of_outbuf : ct_cmdOutput;
	    p_cmdOutput = strncat(p_cmdOutput, p_cmdout, size_of_outbuf - ct_cmdOutput);
	    if (ct_cmdOutput >= size_of_outbuf) { /* buffer full */
		cmd_output[size_of_outbuf-1]='\0'; /* make sure it is a string */
		cmd_output[size_of_outbuf-2]='\n'; /* and ends with a newline  */
		break;	/* bail out */
	    } /* if */
	} /* while */
	*ct_output = ct_cmdOutput;
	fclose(cmdstdio);
    } /* if */

  //  DBTRACE(DBEXIT,("return R_device.c run_cmd\n"));
    return(rc);
} /* run_cmd */


void SIGALRM_handler(int sig, int code, struct sigcontext *scp)
{
    char *cmd_string;

    cmd_string = malloc(HTXD_PATH_MAX);
    sprintf(cmd_string, "ps -eo \"pid args\" | awk '/%s */ { if ($2 != \"awk\") {print \"kill -9 \" $1 }}' | ksh", script_name);
    (void) system(cmd_string);
    free(cmd_string);
    cmd_string = NULL;
    return;                              /* exit signal handler               */
} /* SIGALRM_handler() */




int htxd_run_HE_script(char *pattern, char *device_name, int *conf_emsg)
{
    struct sigaction sigvector;          /* structure for signal specs */
    struct sigaction old_SIGALRM_vector;  /* structure for signal specs */
    int	rc;
    char	*script_list_file;	/* path name of HE_SCRIPT_FILE */
    char	*msg_text;		/* HTX log message text */
    char	*cmdOutput;		/* output from AIX commands */
    char	*cmd_string;	/* command to pass to shell */
    char	*script_cmd;	/* formatted script command */
    char	*p_script_cmd;
    char	*p_temp_filename = "/tmp/htx_temp_file_script";	/* name of temporary file to hold awk output */

    int		ct_cmdOutput;	/* byte count of output in cmdOutput */
    int		ct_script_cmd;
    unsigned int	rc_alarm;

/*
 *	Get some memory to work with.
 */
    //print_log(LOGMSG,"HTXPATH = %s\n", HTXPATH);
    script_list_file = malloc(strlen(global_htx_home_dir) + strlen(HE_SCRIPT_FILE) + 3);
    msg_text = malloc(MSG_TEXT_SIZE);
    cmdOutput = malloc(MSG_TEXT_SIZE);
    cmd_string = malloc(HTXD_PATH_MAX);
    script_cmd = malloc(MSG_TEXT_SIZE);

    *conf_emsg = FALSE;


/*
 *	check access to HE_SCRIPT_FILE file
 */
    script_list_file = strcpy(script_list_file, global_htx_home_dir);
    script_list_file = strcat(script_list_file, "/");
    script_list_file = strcat(script_list_file, HE_SCRIPT_FILE);
    if (0 != (rc = access(script_list_file, R_OK))) {
	sprintf(msg_text, "Cannot access %s. Errno = %d", script_list_file, errno);
	HTXD_TRACE(LOG_ON, msg_text);
//	print_log(LOGERR,"Cannot access %s. Errno = %d", script_list_file, errno);
//	(void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
	//if (!editor_PID && !shell_PID) print_log(LOGERR,"Cannot access %s for %s.  See message log.\n",
	//				      script_list_file, device_name );
	rc = -1;
	*conf_emsg = TRUE;
	goto getout;
    } /* if */

/*
 *	Create a temporary file name for the script command
 */
//    p_temp_filename = tempnam(NULL, "hxs");
//    if (p_temp_filename == NULL) {
//	(void) sprintf(msg_text, "Problem getting the temporary file name for awk output.  errno = %d", errno);
//	HTXD_TRACE(LOG_ON, msg_text);
	//(void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
//	if (!editor_PID && !shell_PID) print_log(LOGMSG,"%s\n", msg_text);
//	rc = -1;
//	*conf_emsg = TRUE;
//	goto getout;
//    } /* if */

/*
 *	Use awk to get find the script and build the command line.
 *
 *  This is a long command, but it should functionally look something like
 *  this, using "^hxecrick.*\.runsetup$" and "tok0" as the pattern and
 *  device name, respectively:
 *
 *  awk 'BEGIN {f=1}  \
 *		/^hxecrick.*\.runsetup$/ { \
 *			f=0	\
 *			print "/usr/lpp/htx/" $2 "tok0 >/tmp" substr($2,index($2,"/")) \
 *					".tok0.out 2>&1" } \
 *		END { exit f }' \
 *		/usr/lpp/htx/hxsscripts > /tmp/hxssupj234j23k 2>&1
 *
 *	And this should result in a line in /tmp/hxssupj234j23k like:
 *
 * /usr/lpp/htx/runsetup/crick.runsetup tok0 >/tmp/crick.runsetup.tok0.out 2>&1
 *
 *	Clear?  Ok, so the idea is to let awk do all that horrible string stuff
 *  and just read that from /tmp/hxssupj234j23k and execute it with the
 *  output going to a file in /tmp which is unique to the run.
 */
    (void) sprintf(cmd_string,
		   "awk 'BEGIN {f=1} /%s/ { f=0; print \"%s/\" $2 \" %s >%s\" substr($2,index($2,\"/\")) \".%s.out 2>&1\" } END {exit f}' %s > %s 2>&1",
		   pattern, global_htx_home_dir, device_name, global_htxd_log_dir, device_name, script_list_file, p_temp_filename);

    ct_script_cmd = MSG_TEXT_SIZE;
    rc = run_cmd(cmd_string, script_cmd, &ct_script_cmd, p_temp_filename);

    if (rc != 0 && rc != 1) { /* some problem with running awk  */
	(void) sprintf(msg_text,
		       "Problem getting the script name from %s.\nPattern =\"%s\"\nrun_cmd() returns %d.  errno=%d   See %s for output.",
		       script_list_file, pattern, rc, errno, p_temp_filename);
	HTXD_TRACE(LOG_ON, msg_text);
//	(void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
//	if (!editor_PID && !shell_PID) print_log(LOGERR,"Failed to get script name for %s.  See message log.\n", device_name);
	rc = -1;
	*conf_emsg = TRUE;
	goto getout;
    } /* if */

    if (rc == 1) { /* nothing for this pattern, leave quietly */
	rc = 0;
	goto getout;
    } /* if */

    /********************************************************************/
	/* If we get here, then there is a script command line to run.      */
	/* awk has built the command line(s), so just run the command(s).   */
    /********************************************************************/

    if ( script_name != (char*)NULL ) {
	free(script_name);
	script_name = NULL;
    }
    script_name = malloc(strlen(script_cmd)); /* SIGALRM_handler uses this */

	/* setup SIGALRM handler */
    sigemptyset(&(sigvector.sa_mask));     /* do not block signals       */
    sigvector.sa_flags = 0;       /* do not restart system calls on sigs */
    sigvector.sa_handler = (void (*)(int)) SIGALRM_handler;
    (void) sigaction(SIGALRM, &sigvector, &old_SIGALRM_vector);

	/* get first line from the command buffer */
    p_script_cmd = strtok( script_cmd, "\n");

    while (p_script_cmd != NULL) {

		/* get the pathname of the script */
	(void) sscanf(p_script_cmd, "%[^ ]", script_name);

		/* Check to make sure that access is OK */
	if (0 != (rc = access(script_name, (F_OK | X_OK | R_OK)))) {

	    switch(errno) {
		case ENOENT:
		    sprintf(msg_text, "Can't find %s\n", script_name);
		    break;
		case EACCES:
		    sprintf(msg_text, "Can't run %s: permission denied.\n",
			    script_name);
		    break;
		default:
		    sprintf(msg_text, "Can't run %s\n: errno = %d",
			    script_name, errno);

	    } /* end switch */

	    //(void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		HTXD_TRACE(LOG_OFF, msg_text);
	   // if (!editor_PID && !shell_PID) print_log(LOGERR,"Cannot run %s for %s.  See message log.\n",
	//					  script_name, device_name);
	    *conf_emsg = TRUE;
	} /* if */

	else {

			/* get the base name of the script for SIGALRM_handler */
	    do {
		rc = sscanf(script_name, "/%s", script_name);
		rc = sscanf(script_name, "%*[^/]%s", script_name);
	    } while ( rc != 0 && rc != EOF );

	    (void) sprintf(msg_text,
			   "Running %s for %s, command line:\n%s",
			   script_name, device_name, p_script_cmd);
	 //   (void) send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);

	    (void) alarm(60);	/* time to wait for script to complete */
	    ct_cmdOutput = 0;
	    rc = run_cmd(p_script_cmd, NULL, &ct_cmdOutput, NULL);
	    rc_alarm = alarm(0);

	    if ( rc_alarm == 0 ) { /* timed out while waiting for script exec */
		(void) sprintf(msg_text,
			       "Killed %s after waiting for %d seconds",
			       script_name, 60);
	//	(void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
	    } /* if */

	    if ( rc < 0 ) { /* problem with running it */
		(void) sprintf(msg_text,
			       "Problem running %s for %s.\nrun_cmd() returns %d.  errno=%d",
			       script_name, device_name, rc, errno);
	//	(void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
	//	if (!editor_PID && !shell_PID) print_log(LOGERR,"Failed to run %s for %s.  See message log.\n",
		//				      script_name, device_name );
		*conf_emsg = TRUE;
	    } /* if */

	    if ( rc >= 0 ) {	/* returned non-negative */
		(void) sprintf(msg_text,
			       "%s for %s exit code = %d.",  script_name, device_name, rc);
	    } /* if */

	    if ( rc > 0 ) {	/* script returned non-zero exit code */
		//(void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
	//	if (!editor_PID && !shell_PID) print_log(LOGMSG,"%s for %s returned %d.  See message log.\n",
	//					      script_name, device_name, rc );
	    } /* if */

	    if ( rc == 0 ) {	/* script returned zero exit code */
		//(void) send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	    } /* if */
	} /* end else */

		/* get subsequent lines one at a time from the command buffer */
	p_script_cmd = strtok( NULL, "\n");
    } /* while */

	/* restore SIGALRM handler to default */
    (void) sigaction(SIGALRM, &old_SIGALRM_vector, (struct sigaction *) NULL);

/*
 *	Free the memory
 */
    getout:

    free(cmdOutput);
    free(msg_text);
    free(script_cmd);
    free(script_list_file);
    free(cmd_string);

    cmdOutput = NULL;
    msg_text = NULL;
    script_cmd = NULL;
    script_list_file = NULL;
    cmd_string = NULL;

    return rc;
} /* exec_HE_script */
