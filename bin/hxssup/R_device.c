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

/* @(#)39	1.8.4.7  src/htx/usr/lpp/htx/bin/hxssup/R_device.c, htx_sup, htxfedora 12/16/14 03:58:11 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    R_device.c                                            */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  Activate/Halt system                                  */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Activates/Halts the HTX system including all hardware */
/*                     exercisers defined in the mdt.                        */
/*                                                                           */
/* COMPILER OPTIONS =                                                        */
/*                                                                           */
/* CHANGE ACTIVITY =                                                         */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                  */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM */
/*    11/08/99:1.0  :R GEBHARDT:INITIAL RELEASE                              */
/*            :     :          :                                             */
/*                                                                           */
/*****************************************************************************/

#include "hxssup.h"
#include <scr_info.h>

#include <unistd.h>

#ifdef	__HTX_LINUX__
#include <sys/wait.h>
#endif

#ifdef	__HTX_LINUX__
#define	SIGMAX	(SIGRTMAX)
#endif

#define	HE_SCRIPT_FILE	"hxsscripts"

#define NUM_COLS	(23)
#define NUM_ROWS	(81)

#define RD_IN_ROW	(21)
#define RD_IN_COL	(18)

extern union semun semctl_arg ;

static char *script_name;	/* path name of script use by SIGARLM_handler */
extern tecg_struct *ecg_info;

extern	char	*regcmp(char *, char *);	/* compile exp */
extern	char	*regex(char *, char *);

extern union shm_pointers shm_addr;	/* shared memory union pointers      */
extern int semhe_id;		/* semaphore id                      */
extern char HTXPATH[];		/* HTX file system path         */
extern volatile int system_call;		/* set to TRUE before system() */

/*****************************************************************************/
/*****  r u n _ c m d  *******************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     run_cmd                                               */
/*                                                                           */
/* DESCRIPTIVE NAME =  Run a system command and return the output            */
/*                                                                           */
/* FUNCTION =          Runs a shell command string and returns the exit      */
/*                     code and optionally contents of output file.          */
/*                     To return output from the file, it must be redirected */
/*                     into a file by the command string, and the name of    */
/*                     the file must be passed in.                           */
/*                                                                           */
/* INPUT =             cmd_string - command string to execute.               */
/*                     ct_output  - pointer to size of cmd_output.           */
/*                     cmdout_filename - name of file to read cmd output     */
/*                                from.                                      */
/*                                                                           */
/* OUTPUT =            ct_output  - pointer to length of output, can be      */
/*                                  larger than cmd_output, see notes.       */
/*                     cmd_output - points to where to put the contents of   */
/*                                the command output file.                   */
/*                                                                           */
/* NORMAL RETURN =     0 - command ran and returned 0 as an exit code.       */
/*                                                                           */
/* ERROR RETURNS =     < 0 - problem with running the system command.  No    */
/*                         output from command is available.   ct_output     */
/*                         and cmd_output are not affected.                  */
/*                                                                           */
/*                     > 0 - system command returned non-zero exit code.     */
/*                     -128 - system() call failed.                          */
/*                     -129 - failed to open command output file.            */
/*                                                                           */
/*    NOTES =          ct_output is an input and output parameter.  If it    */
/*                     returns larger than it was on input, this indicates   */
/*                     that the output was truncated.                        */
/*                                                                           */
/*                     If cmd_output is NULL or cmdout_filename is null      */
/*                     then no output is returned.                           */
/*                                                                           */
/*    EXTERNAL REFERENCES = system_call                                      */
/*                                                                           */
/*    DATA AREAS =                                                           */
/*                                                                           */
/*    MACROS =                                                               */
/*                                                                           */
/*****************************************************************************/
int run_cmd(char *cmd_string, char *cmd_output, int *ct_output,
	    char *cmdout_filename)
{
	FILE *cmdstdio;		/* pipe fd */
	char msg_text[MAX_TEXT_MSG];
	char *p_cmdout, cmdout[256], *p_cmdOutput;
	int ct_cmdOutput = 0;	/* counter for global output chars */
	int size_of_outbuf;
	int rc;

	size_of_outbuf = *ct_output;
	if (cmd_output != NULL)
		bzero(cmd_output, size_of_outbuf);
	p_cmdOutput = cmd_output;

	/* run the command with system() */
	system_call = TRUE;
	rc = system(cmd_string);
	system_call = FALSE;
	/* check rc for system here */
	if (rc == -1 || rc == 127) {	/* some problem system() */
		return (-128);
	}
	/* if */
	if (WIFSIGNALED(rc)) {	/* script killed by a signal */
		 sprintf(msg_text,
			       "Command killed by signal %d.",
			       WTERMSIG(rc));
		 send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		rc = -WTERMSIG(rc);
	} /* if */
	else
		rc = WEXITSTATUS(rc);

	if (cmd_output != NULL && cmdout_filename != NULL) {

		/* open output file */
		cmdstdio = fopen(cmdout_filename, "r");
		if (cmdstdio == NULL) {
			 sprintf(msg_text,
				       "Failed to open command output file: %s.   errno = %d\n",
				       cmdout_filename, errno);

			     send_message(msg_text, 0,
						HTX_SYS_SOFT_ERROR,
						HTX_SYS_MSG);
			return (-129);
		}

		/* read the output from the command until EOF or error */
		while (NULL != (p_cmdout = fgets(cmdout, 255, cmdstdio))) {
			ct_cmdOutput = ct_cmdOutput + htx_strlen(p_cmdout);
			ct_cmdOutput =
			    (ct_cmdOutput >=
			     size_of_outbuf) ? size_of_outbuf :
			    ct_cmdOutput;
			p_cmdOutput =
			    strncat(p_cmdOutput, p_cmdout,
				    size_of_outbuf - ct_cmdOutput);
			if (ct_cmdOutput >= size_of_outbuf) {	/* buffer full */
				cmd_output[size_of_outbuf - 1] = '\0';	/* make sure it is a string */
				cmd_output[size_of_outbuf - 2] = '\n';	/* and ends with a newline  */
				break;	/* bail out */
			}	/* if */
		}		/* while */
		fclose(cmdstdio);
		*ct_output = ct_cmdOutput;
	}			/* if */
	return (rc);
}				/* run_cmd */

/*****************************************************************************/
/*****  S I G A L R M _ h a n d l e r  ***************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     SIGALRM_handler                                       */
/*                                                                           */
/* DESCRIPTIVE NAME =  alarm signal handler for script time out              */
/*                                                                           */
/* FUNCTION =          Upon receipt of an alarm signal, kills the script     */
/*                     that is name in script_name.                          */
/*                                                                           */
/* INPUT =             script_name - global pointer to script path name      */
/*                                                                           */
/* OUTPUT =            None.                                                 */
/*                                                                           */
/* NORMAL RETURN =     None - void function.                                 */
/*                                                                           */
/* ERROR RETURNS =     None - void function.                                 */
/*                                                                           */
/* EXTERNAL REFERENCES: None.                                                */
/*                                                                           */
/* GLOBAL REFERENCES: script_name - path name of startup/cleanup script      */
/*                     which is running.                                     */
/*                                                                           */
/*****************************************************************************/

void SIGALRM_handler(int sig, int code, struct sigcontext *scp)
{
	char *cmd_string;

	cmd_string = malloc(PATH_MAX);
	sprintf(cmd_string,
		"ps -eo \"pid args\" | awk '/%s */ { if ($2 != \"awk\") {print \"kill -9 \" $1 }}' | ksh",
		script_name);
	 system(cmd_string);
	free(cmd_string);
	return;			/* exit signal handler               */

}				/* SIGALRM_handler() */

/*****************************************************************************/
/*****  e x e c _ H E _ s c r i p t  *****************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     exec_HE_script                                        */
/*                                                                           */
/* DESCRIPTIVE NAME =  Execute Hardware Exerciser setup/cleanup Script       */
/*                                                                           */
/* FUNCTION =          Runs a script taken from the list of HE scripts       */
/*                                                                           */
/* INPUT =             pattern - a regular expression                        */
/*                     device_name - HTX logical device name (like in MDT).  */
/*                                                                           */
/* OUTPUT =            Runs the selected setup or cleanup script or for      */
/*                     that matter any script executable from the home dir.  */
/*                                                                           */
/*                     conf_emsg - hints that inportant error message was    */
/*                                 posted for operator on console message    */
/*                                 line.                                     */
/*                                                                           */
/* NORMAL RETURN =     0 - no script found for exerciser or it was found     */
/*                         and it returned 0.                                */
/*                                                                           */
/* ERROR RETURNS =     < 0 - problem with finding a script for the exerciser */
/*                           see the htxmsg log.                             */
/*                     > 0 - script returned non-zero exit code, see htxmsg. */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*                                                                           */
/*    DATA AREAS =     HTXPATH - pointer to a string which contains the path */
/*                         to the top level of the HTX file hierarchy        */
/*                         (usually "/usr/lpp/htx").                         */
/*                                                                           */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/* NOTES = Please note that this function contains those hated constructs,   */
/*         the goto.  The idea is to contain as much clean up at the end     */
/*         of the function as possible without getting the indent level      */
/*         very great.  So I check first for failures, issue message and     */
/*         go to cleanup.                                                    */
/*                                                                           */
/*****************************************************************************/
int exec_HE_script(char pattern[], char device_name[],
		   boolean_t * conf_emsg)
{
	struct sigaction sigvector;	/* structure for signal specs */
	struct sigaction old_SIGALRM_vector;	/* structure for signal specs */
	int	rc = 0;
	char *script_list_file = NULL;	/* path name of HE_SCRIPT_FILE */
	char *msg_text = NULL;		/* HTX log message text */
	char *cmdOutput = NULL;		/* output from AIX commands */
	char *cmd_string = NULL;	/* command to pass to shell */
	char *script_cmd = NULL;	/* formatted script command */
	char *p_script_cmd = NULL;
	char *p_temp_filename = NULL;	/* name of temporary file to hold awk output */

	int ct_cmdOutput = 0;	/* byte count of output in cmdOutput */
	int ct_script_cmd = 0;
	unsigned int rc_alarm = 0;

	/*
	 *	Get some memory to work with.
	 */
	script_list_file = malloc(htx_strlen(HTXPATH) + htx_strlen(HE_SCRIPT_FILE) + 3);
	msg_text = malloc(MSG_TEXT_SIZE);
	cmdOutput = malloc(MSG_TEXT_SIZE);
	cmd_string = malloc(PATH_MAX);
	script_cmd = malloc(MSG_TEXT_SIZE);

	*conf_emsg = FALSE;

	/*
	 *	check access to HE_SCRIPT_FILE file
	 */
	script_list_file = htx_strcpy(script_list_file, HTXPATH);
	script_list_file = htx_strcat(script_list_file, "/");
	script_list_file = htx_strcat(script_list_file, HE_SCRIPT_FILE);
	if (0 != (rc = access(script_list_file, R_OK))) {
			sprintf(msg_text, "Cannot access %s. Errno = %d", script_list_file, errno);
			send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
			PRTMSG(MSGLINE, 0, ("Cannot access %s for %s.  See message log.\n", script_list_file, device_name));
			rc = -1;
			*conf_emsg = TRUE;
			goto getout;
	}

	/*
	 *   Create a temporary file name for the script command
	 */
	p_temp_filename = tempnam(NULL, "hxs");
	if (p_temp_filename == NULL) {
			sprintf(msg_text, "Problem getting the temporary file name for awk output.  errno = %d", errno);
			send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
			PRTMSG(MSGLINE, 0, (msg_text));
			rc = -1;
			*conf_emsg = TRUE;
			goto getout;
	}

	/*
	 *   Use awk to get find the script and build the command line.
	 *
	 *  This is a long command, but it should functionally look something like
	 *  this, using "^hxecrick.*\.runsetup$" and "tok0" as the pattern and
	 *  device name, respectively:
	 *
	 *  awk 'BEGIN {f=1}  \
	 *           /^hxecrick.*\.runsetup$/ { \
	 *                   f=0     \
	 *                   print "/usr/lpp/htx/" $2 "tok0 >/tmp" substr($2,index($2,"/")) \
	 *                                   ".tok0.out 2>&1" } \
	 *           END { exit f }' \
	 *           /usr/lpp/htx/hxsscripts > /tmp/hxssupj234j23k 2>&1
	 *
	 *   And this should result in a line in /tmp/hxssupj234j23k like:
	 *
	 * /usr/lpp/htx/runsetup/crick.runsetup tok0 >/tmp/crick.runsetup.tok0.out 2>&1
	 *
	 *   Clear?  Ok, so the idea is to let awk do all that horrible string stuff
	 *  and just read that from /tmp/hxssupj234j23k and execute it with the
	 *  output going to a file in /tmp which is unique to the run.
	 */
	 //sprintf(cmd_string, "awk 'BEGIN {f=1} /%s/ { f=0; print \"%s\" $2 \" %s >/tmp\" substr($2,index($2,\"/\")) \".%s.out 2>&1\" } END {exit f}' %s > %s 2>&1", pattern, HTXPATH, device_name, device_name, script_list_file, p_temp_filename);

	 sprintf(cmd_string, "awk 'BEGIN {f=1} /%s/ { f=0; print \"%s/\" $2 \" %s >/tmp\" substr($2,index($2,\"/\")) \".%s.out 2>&1\" } END {exit f}' %s > %s 2>&1", pattern, HTXPATH, device_name, device_name, script_list_file, p_temp_filename);
	 sprintf(msg_text,"The awk command string is: %s\n",cmd_string);
     send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);

	 ct_script_cmd = MSG_TEXT_SIZE;
	 rc = run_cmd(cmd_string, script_cmd, &ct_script_cmd, p_temp_filename);

	 if (rc != 0 && rc != 1) {	/* some problem with running awk  */
			 //sprintf(msg_text, "Problem getting the script name from %s.\nPattern =\"%s\"\nrun_cmd() returns %d.  errno=%d   See %s for output.", script_list_file, pattern, rc, errno, p_temp_filename);
			 sprintf(msg_text, "Problem getting the script name from %s. \nThere may not be any cleanup script for %s.\nPattern =\"%s\"\nrun_cmd() returns %d.  errno=%d   See %s for output.", script_list_file, device_name, pattern, rc, errno, p_temp_filename);
			 send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			 PRTMSG(MSGLINE, 0, ("Failed to get script name for %s.  See message log.\n", device_name));
			 rc = -1;
			 *conf_emsg = TRUE;
			 goto getout;
	 }


	 remove(p_temp_filename);	/* cleanup temp file */
	 if (rc == 1) {		/* nothing for this pattern, leave quietly */
			 rc = 0;
			 goto getout;
	 }

	 /*
	  * If we get here, then there is a script command line to run. awk has
	  * built the command line(s), so just run the command(s).
	  */
	 script_name = malloc(htx_strlen(script_cmd));	/* SIGALRM_handler uses this */

	 /* setup SIGALRM handler */
	 sigemptyset(&(sigvector.sa_mask));	/* do not block signals       */
	 sigvector.sa_flags = 0;	/* do not restart system calls on sigs */
	 sigvector.sa_handler = (void (*)(int)) SIGALRM_handler;
	 sigaction(SIGALRM, &sigvector, &old_SIGALRM_vector);

	/* get first line from the command buffer */
	 p_script_cmd = strtok(script_cmd, "\n");

	 while (p_script_cmd != NULL) {
			 /* get the pathname of the script */
			 sscanf(p_script_cmd, "%[^ ]", script_name);

			 /* Check to make sure that access is OK */
			 if (0 != (rc = access(script_name, (F_OK | X_OK | R_OK)))) {
					 switch (errno) {
							 case ENOENT:
									 sprintf(msg_text, "Can't find %s\n", script_name);
									 break;

							 case EACCES:
									 sprintf(msg_text, "Can't run %s: permission denied.\n", script_name);
									 break;

							 default:
									 sprintf(msg_text, "Can't run %s\n: errno = %d", script_name, errno);
					 }	/* end switch */

					 send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
					 PRTMSG(MSGLINE, 0, ("Cannot run %s for %s.  See message log.\n", script_name, device_name));
					 *conf_emsg = TRUE;
			 }			 /* if */

			 else {

					 /* get the base name of the script for SIGALRM_handler */
					 do {
							 rc = sscanf(script_name, "/%s", script_name);
							 rc = sscanf(script_name, "%*[^/]%s", script_name);
					 } while (rc != 0 && rc != EOF);

					 sprintf(msg_text, "Running %s for %s, command line:\n%s", script_name, device_name, p_script_cmd);
					 send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);

					 alarm(60);	/* time to wait for script to complete */
					 ct_cmdOutput = 0;
					 rc = run_cmd(p_script_cmd, NULL, &ct_cmdOutput, NULL);
					 rc_alarm = alarm(0);
					 if (rc_alarm == 0) {	/* timed out while waiting for script exec */
							 sprintf(msg_text, "Killed %s after waiting for %d seconds", script_name, 60);
							 send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
					 }		/* if */

					 if (rc < 0) {	/* problem with running it */
							 sprintf(msg_text, "Problem running %s for %s.\nrun_cmd() returns %d.  errno=%d", script_name, device_name, rc, errno);
							 send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
							 PRTMSG(MSGLINE, 0, ("Failed to run %s for %s.  See message log.\n", script_name, device_name));
							 *conf_emsg = TRUE;
					 }		/* if */

					 if (rc >= 0) {	/* returned non-negative */
							 sprintf(msg_text, "%s for %s exit code = %d.", script_name, device_name, rc);
					 }		/* if */

					 if (rc > 0) {	/* script returned non-zero exit code */
							 send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
							 PRTMSG(MSGLINE, 0, ("%s for %s returned %d.  See message log.\n", script_name, device_name, rc));
					 }			/* if */

					 if (rc == 0) {	/* script returned zero exit code */
							 send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
					 }	/* if */
			 }		/* end else */

			 /* get subsequent lines one at a time from the command buffer */
			 p_script_cmd = strtok(NULL, "\n");
	 }			/* while */

	/* restore SIGALRM handler to default */
	 sigaction(SIGALRM, &old_SIGALRM_vector, (struct sigaction *) NULL);

	 /*
	  *	Free the memory
	  */
      getout:
	 free(p_temp_filename);
	 free(cmdOutput);
	 free(msg_text);
	 free(script_cmd);
	 free(script_list_file);
	 free(cmd_string);

	 return (rc);
}				/* exec_HE_script */



/*****************************************************************************/
/*****  l o a d _ e x e r s i c e r  *****************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     load_exerciser                                        */
/*                                                                           */
/* DESCRIPTIVE NAME =  Load HTX Exerciser                                    */
/*                                                                           */
/* FUNCTION =          Spawns the hardware exercisers.                       */
/*                                                                           */
/* INPUT =             p_HE - pointer to htxshm_HE structure.                */
/*                     sigvec - pointer to sigvector structure               */
/*                                                                           */
/* OUTPUT =            Loads hardware exerciser program and sets the PID     */
/*                     in the htxshm_HE structure.                           */
/*                                                                           */
/* NORMAL RETURN =     0 - OK                                                */
/*                                                                           */
/* ERROR RETURNS =     1 - problem with fork()                               */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = N/A                                                   */
/*                                                                           */
/*    DATA AREAS =     HTXPATH - pointer to a string which contains the path */
/*                         to the top level of the HTX file hierarchy        */
/*                         (usually "/usr/lpp/htx").                         */
/*                     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                                                                           */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/

int load_exerciser(struct htxshm_HE *p_HE, struct sigaction *sigvec)
{

	char workstr[512];	/* work string                       */
	char workstr2[64];	/* work string                       */
	char workstr3[64];	/* work string                       */
	char workstr4[64];	/* work string                       */
	char workstr5[RULE_NAME_LENGTH_LIMIT + 20];	/* work string                       */
	int workint = 0;		/* work integer                      */
	int frkpid = 0;		/* fork PID                          */
	int j = 0;
	int null_fd;

	frkpid = fork();

	switch (frkpid) {
	case 0:		/* child process                 */
		/* do not close std fds since the closing causes fd pollution for exerciser std fds operations */
		fcntl(fileno(stdin), F_SETFD, 0);
		fcntl(fileno(stdout), F_SETFD, 0);
		fcntl(fileno(stderr), F_SETFD, 0);

		/* enabling stdin,stdout and stderr for dummy use of printf in exercisors */
		null_fd = open("/dev/null", O_RDWR);
		dup2(null_fd, fileno(stdin) );
		dup2(null_fd, fileno(stdout) );
		dup2(null_fd, fileno(stderr) );

		for (j = 1; j <= SIGMAX; j++)	/* set default signal processing */
			 sigaction(j, sigvec, (struct sigaction *) NULL);

		 setsid();	/* change the process group so HE's don't get
					   signals meant for the supervisor program */

		workint = p_HE->priority;
		 nice(workint);

		sleep(1);	/* let the supervisor update shared memory */

		 htx_strcpy(workstr, HTXPATH);
		 htx_strcat(workstr, "/bin/");

		 htx_strcpy(workstr2, p_HE->HE_name);

		 htx_strcat(workstr, workstr2);

		 htx_strcpy(workstr3, "/dev/");
		 htx_strcat(workstr3, p_HE->sdev_id);

		if (shm_addr.hdr_addr->emc == 1)
			 htx_strcpy(workstr4, "EMC");
		else
			 htx_strcpy(workstr4, "REG");

		if (shm_addr.hdr_addr->emc == 1)
		{
			if (p_HE->emc_rules[0] != '/')
			{  /* in case of relative path */
				htx_strcpy(workstr5, HTXPATH);
				htx_strcat(workstr5, "/rules/");
				htx_strcat(workstr5,"emc/");
				htx_strcat(workstr5, p_HE->emc_rules);
			}
			else
			{  /* in case of absolute path no change */
				htx_strcpy(workstr5, p_HE->emc_rules);
			}
		}
		else
		{
			if (p_HE->reg_rules[0] != '/')
			{  /* in case of relative path */
				htx_strcpy(workstr5, HTXPATH);
				htx_strcat(workstr5, "/rules/");
				htx_strcat(workstr5,"reg/");
				htx_strcat(workstr5, p_HE->reg_rules);
			}
			else
			{  /* in case of absolute path no change */
				htx_strcpy(workstr5, p_HE->reg_rules);
			}
		}

		if ((execl(workstr, workstr2, workstr3, workstr4, workstr5,
			   (char *) 0)) == -1) {
			 sprintf(workstr, "Unable to load %s hardware "
				       "exerciser.  errno = %d", workstr2,
				       errno);
			PRTMSG(MSGLINE, 0, ("%s", htx_strcat(workstr, "\n")));
			 send_message(workstr, errno, HTX_SYS_HARD_ERROR,
					    HTX_SYS_MSG);
			exit(-1);
		}		/* endif */
		break;

	case -1:		/* problem with fork() call       */
		 htx_strcpy(workstr2, p_HE->HE_name);

		 sprintf(workstr, "Unable to fork for %s hardware "
			       "exerciser.  errno = %d", workstr2, errno);
		PRTMSG(MSGLINE, 0, ("%s", htx_strcat(workstr, "\n")));
		 send_message(workstr, errno, HTX_SYS_HARD_ERROR,
				    HTX_SYS_MSG);
		return (1);
		break;

	default:		/* parent process               */
                add_pid_to_shm(frkpid, p_HE );
		/*p_HE->PID = frkpid;*/
		break;
	}			/* endswitch */
	return (0);
}				/* load_exerciser */

/*****************************************************************************/
/*****  s i g _ r e s t a r t _ H E  *****************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     sig_restart_HE                                        */
/*                                                                           */
/* DESCRIPTIVE NAME =  Restart hardware exerciser by signal                  */
/*                                                                           */
/* FUNCTION =          Spawns a hardware exerciser which has died or was     */
/*                     terminated.  SIGUSR2 handler.                         */
/*                                                                           */
/* INPUT =             p_shm_hdr - points to header of shared memory         */
/*                                                                           */
/* OUTPUT =            Loads hardware exerciser programs                     */
/*                                                                           */
/*                     Updated system semaphore structure to reflect the     */
/*                     state of the HE's which are restarted.                */
/*                                                                           */
/* NORMAL RETURN =     N/A - void function                                   */
/*                                                                           */
/* ERROR RETURNS =     N/A - void function                                   */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*                                                                           */
/*    OTHER ROUTINES = load_exerciser () - loads the exerciser.              */
/*                     send_message() - outputs a message to the screen and  */
/*                                     the message log file                  */
/*                                                                           */
/*    DATA AREAS =                                                           */
/*                     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                                                                           */
/*    MACROS =                                                               */
/*                                                                           */
/*****************************************************************************/
void sig_restart_HE(int sig, int code, struct sigcontext *scp)
{
    boolean_t dummy;
    struct htxshm_HE *p_shm_addr;
    struct htxshm_hdr *p_shm_hdr;
    struct sigaction sigvector;	/* structure for signal specs */
    char *msg_text;		/* HTX log message text */
    char reg_expr[256];
    int i, no_of_inst, rc;
    FILE *fp;
    char cmd_str[1024], buf[1024];

    msg_text = malloc(MSG_TEXT_SIZE);
    p_shm_hdr = shm_addr.hdr_addr;
    p_shm_addr = ((struct htxshm_HE *) ((struct htxshm_hdr *) shm_addr.hdr_addr + 1));	/* skip over header */

    /* set file descriptor to close on exec */

    fcntl(fileno(stdin), F_SETFD, 1);	/* stdin to close on exec  */
    fcntl(fileno(stdout), F_SETFD, 1);	/* stdout to close on exec */
    fcntl(fileno(stderr), F_SETFD, 1);	/* stderr to close on exec */

    /* setup signal vector structure for HE */
    /************************************************************/
    /******  Set signals to SIG_DFL  ****************************/
    /******  for the newly restarted HE's  **********************/
    /************************************************************/
    sigvector.sa_handler = SIG_DFL;	/* set default action */
    sigemptyset(&(sigvector.sa_mask));	/* do not block other signals */
    sigvector.sa_flags = 0;	/* No special flags */

    for (i = 0; i < (p_shm_hdr)->max_entries; i++) {
	if ((((p_shm_addr + i)->PID) == 0) && (((p_shm_addr + i)->restart_HE) == 1)) {
		sprintf(msg_text, "Restart of %s signaled by remote.", (p_shm_addr + i)->sdev_id);
		send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
        if (strncmp((p_shm_addr+i)->HE_name, "hxesct", 6) == 0)
            sprintf(cmd_str, "ps -ef | grep hxesct | grep -v grep | wc -l");
        else
            sprintf(cmd_str, "ps -ef | grep %s | grep -v grep | wc -l", (p_shm_addr+i)->HE_name);
		if ((fp = popen(cmd_str, "r")) == NULL)  {
		   (void) sprintf(msg_text,"popen error in sig_restart_HE");
		   (void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		}
		if (fgets(buf,1024,fp) == NULL) {
		   (void) sprintf(msg_text,"fgets error in sig_restart_HE");
		   (void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
	    }
		sscanf(buf, "%d", &no_of_inst);
		system_call = TRUE;
		pclose(fp);
		system_call = FALSE;
		if(no_of_inst == 0) {
			htx_strcpy(reg_expr, "^");
		    htx_strcat(reg_expr, (p_shm_addr + i)->HE_name);
		    htx_strcat(reg_expr, ".*setup[\t ]*$");
		    rc = exec_HE_script(reg_expr, (p_shm_addr + i)->sdev_id, &dummy);
		}
		semctl_arg.val = 1;
		if ((p_shm_addr + i)->start_halted)  {
			semctl(semhe_id, ((i * SEM_PER_EXER) + 6), SETVAL, semctl_arg);
		}

		(p_shm_addr + i)->user_term = 0;	/* reset */
		(p_shm_addr + i)->restart_HE = 0;	/* reset */
		rc = load_exerciser(p_shm_addr + i, &sigvector);	/* fork & exec */

		if (rc == 0) {
			((p_shm_hdr)->num_entries)++;
			(p_shm_addr + i)->tm_last_upd = -1;
			sleep(2);
		}			/* endif */
	}			/* endif */
    }				/* endfor */

    fcntl(fileno(stdin), F_SETFD, 0);	/* stdin NOT to close on exec */
    fcntl(fileno(stdout), F_SETFD, 0);	/*stdout NOT to clse on exec */
    fcntl(fileno(stderr), F_SETFD, 0);	/*stderr NOT to clse on exec */
    free(msg_text);
}				/* sig_restart_HE */

/*****************************************************************************/
/*****  r e s t a r t _ e x e r c i s e r  ***********************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     restart_exerciser                                     */
/*                                                                           */
/* DESCRIPTIVE NAME =  Restart hardware exerciser                            */
/*                                                                           */
/* FUNCTION =          Spawns a hardware exerciser which has died or was     */
/*                     terminated.                                           */
/*                                                                           */
/* INPUT =             p_shm_hdr - points to header of shared memory         */
/*                     HE_pos - position of HE in shared memory, used also   */
/*                              for the semaphore in the HE semaphore set.   */
/*                                                                           */
/* OUTPUT =            Loads hardware exerciser programs                     */
/*                                                                           */
/*                     Updated system semaphore structure to reflect the     */
/*                     state of the HE's which are restarted.                */
/*                                                                           */
/* NORMAL RETURN =     N/A - void function                                   */
/*                                                                           */
/* ERROR RETURNS =     N/A - void function                                   */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*                                                                           */
/*    OTHER ROUTINES = load_exerciser () - loads the exerciser.              */
/*                     send_message() - outputs a message to the screen and  */
/*                                     the message log file                  */
/*                                                                           */
/*    DATA AREAS =                                                           */
/*                     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                                                                           */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/
void restart_exerciser(struct htxshm_hdr *p_shm_hdr, int HE_pos)
{
	boolean_t dummy;
	struct htxshm_HE *p_shm_addr;
	struct sigaction sigvector;	/* structure for signal specs */
	struct sigaction old_SIGINT_vector;	/* structure for signal specs */
	struct sigaction old_SIGQUIT_vector;	/* structure for signal specs */
	char *msg_text = NULL;		/* HTX log message text */
	char reg_expr[256];
	int no_of_inst, i = 0, rc = 0;
    char cmd_str[1024], buf[1024];
    FILE *fp;

	if (!p_shm_hdr->started) {
		PRTMSG(MSGLINE, 0, ("%s\n", "System must be running to restart exercisers."));
		return;
	}				/* if */

	p_shm_addr = ((struct htxshm_HE *) ((struct htxshm_hdr *) p_shm_hdr + 1));	/* skip over header */

	msg_text = malloc(MSG_TEXT_SIZE);

	/* set file descriptor to close on exec */
	fcntl(fileno(stdin), F_SETFD, 1);	/* stdin to close on exec  */
	fcntl(fileno(stdout), F_SETFD, 1);	/* stdout to close on exec */
	fcntl(fileno(stderr), F_SETFD, 1);	/* stderr to close on exec */

	/* setup signal vector structure for HE */
	/***  Ignore SIGINT and SIGQUIT signals  ********************/
	sigvector.sa_handler = SIG_IGN;	/* set default action */
	sigemptyset(&(sigvector.sa_mask));	/* do not block other signals */
	sigvector.sa_flags = 0;		/* No special flags */

	sigaction(SIGINT, &sigvector, &old_SIGINT_vector);
	sigaction(SIGQUIT, &sigvector, &old_SIGQUIT_vector);

	/*
	 * Set signals to SIG_DFL for the newly restarted HE's
	 */
	sigvector.sa_handler = SIG_DFL;	/* set default action */
	sigemptyset(&(sigvector.sa_mask)); /* do not block other signals */
	sigvector.sa_flags = 0;	/* No special flags                    */

	i = HE_pos;
	if (((p_shm_addr + i)->PID) == 0) {
		sprintf(msg_text, "Restart of %s requested by operator.", (p_shm_addr + i)->sdev_id);
		send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
        if (strncmp((p_shm_addr+i)->HE_name, "hxesct", 6) == 0)
           sprintf(cmd_str, "ps -ef | grep hxesct | grep -v grep | wc -l");
        else
           sprintf(cmd_str, "ps -ef | grep %s | grep -v grep | wc -l", (p_shm_addr+i)->HE_name);
		if ((fp = popen(cmd_str, "r")) == NULL)  {
		    (void) sprintf(msg_text,"popen error in restart_exerciser");
		    (void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		}
		if (fgets(buf,1024,fp) == NULL) {
		   (void) sprintf(msg_text,"fgets error in restart_exerciser");
		   (void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		}
		sscanf(buf, "%d", &no_of_inst);
		system_call = TRUE;
		pclose(fp);
		system_call = FALSE;
		if(no_of_inst == 0) {
			htx_strcpy(reg_expr, "^");
			htx_strcat(reg_expr, (p_shm_addr + i)->HE_name);
			htx_strcat(reg_expr, ".*setup[\t ]*$");
			rc = exec_HE_script(reg_expr, (p_shm_addr + i)->sdev_id, &dummy);
		}
	semctl_arg.val=1;
		if ((p_shm_addr + i)->start_halted)  {
			semctl(semhe_id, ((i * SEM_PER_EXER) + 6), SETVAL, semctl_arg);
		}

		(p_shm_addr + i)->user_term = 0;	/* reset */
		rc = load_exerciser(p_shm_addr + i, &sigvector);	/* fork & exec */

		if (rc == 0) {
			((p_shm_hdr)->num_entries)++;
			(p_shm_addr + i)->tm_last_upd = -1;
		}	/* endif */
	}    /* endif */

	fcntl(fileno(stdin), F_SETFD, 0);	/* stdin NOT to close on exec */
	fcntl(fileno(stdout), F_SETFD, 0);	/*stdout NOT to clse on exec */
	fcntl(fileno(stderr), F_SETFD, 0);	/*stderr NOT to clse on exec */

	sigaction(SIGINT, &old_SIGINT_vector, (struct sigaction *) 0);
	sigaction(SIGQUIT, &old_SIGQUIT_vector, (struct sigaction *) 0);

	free(msg_text);
}				/* restart_exerciser */

/*****************************************************************************/
/*****  R _ d e v i c e  *****************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     R_device                                              */
/*                                                                           */
/* DESCRIPTIVE NAME =  Restart hardware exerciser                            */
/*                                                                           */
/* FUNCTION =          Builds and displays the Restart exerciser menu and    */
/*                     handles operators requests.                           */
/*                                                                           */
/* INPUT =             No parameters                                         */
/*                                                                           */
/* OUTPUT =            Restarts HE's and tracks progress.                    */
/*                                                                           */
/*                     Updated screen to reflect the                         */
/*                     state of the HE's which are restarted.                */
/*                                                                           */
/* NORMAL RETURN =     0 - OK                                                */
/*                                                                           */
/* ERROR RETURNS =     non zero - some fatal programming flaw.               */
/*                     Operational errors, such as failures to exec hardware */
/*                     exercisers are handled in lower functions and result  */
/*                     in error messages.                                    */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*                                                                           */
/*    OTHER ROUTINES = display_scn() - displays the restart screen template  */
/*                     get_dst() - returns the device status.                */
/*                     get_string() - reads operator input.                  */
/*                     regcmp - compile regular expression                   */
/*                     regex - compare regular expression                    */
/*                     send_message() - outputs a message to the screen and  */
/*                                     the message log file                  */
/*                     restart_exerciser () - restarts the exerciser.        */
/*                                                                           */
/*    DATA AREAS =     standard curses library externals, LINES, COLS.       */
/*                     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                     CLRLIN - Clears the message line.                     */
/*                                                                           */
/*****************************************************************************/
int R_device(void)
{
    char *cmp_regexp;		/* ptr to compiled reg expression    */
    char input[32];		/* input string                      */
    char ret[3];		/* return array for regex()          */
    char *work;			/* work int */
    char workstr[128];		/* work string                       */
	char page_str[15];

    enum t_dev_status dev_status;	/* return from get_dst               */
    extern char *__loc1;	/* beginning of reg exp match        */

    extern int COLS;		/* number of cols in CURSES screen   */
    extern int LINES;		/* number of lines in CURSES screen  */
    extern WINDOW *stdscr;	/* standard screen window            */

    int col = 0;		/* first column displayed on screen  */
    int i = 0;			/* loop counter                      */
	int k;               /* loop counter                      */
	int rc;              /* return value.                     */
	int dev_len;         /* loop counter                      */
	int dev_no=-1;       /* loop counter                      */
    int max_strt_ent = 0;		/* max starting entry                */
    int num_disp = 0;		/* number of THE entries to show     */
    int num_entries = 0;	/* local number of shm HE entries    */
    int row = 0;		/* first row displayed on screen     */
    int strt_ent = 1;		/* first entry on screen             */
    int update_screen = 0;		/* update screen flag                */
    int workint = 0;

    struct ahd_entry *p_td_table = NULL;	/* points to begin td seq tbl    */
    struct ahd_entry *p_into_table;	/* points into td sequence table     */

    struct htxshm_HE *p_htxshm_HE;	/* pointer to a htxshm_HE struct     */

    union shm_pointers shm_addr_wk;	/* work ptr into shm                 */


    shm_addr_wk.hdr_addr = shm_addr.hdr_addr;	/* copy addr to work space   */
    (shm_addr_wk.hdr_addr)++;	/* skip over header                   */

    /*
     * display screen outline
     */
    clear();
    refresh();
    display_scn(stdscr, 0, 0, LINES, COLS, "RD_scn", 1, &row, &col, NUM_COLS, NUM_ROWS, 'n');

    /*
     * loop until operator says to quit
     */
    for (;;) {
	    if ((shm_addr.hdr_addr)->max_entries == 0) {	/* no HE programs?        */
		    PRTMSG(MSGLINE, 0, ("There are no Hardware Exerciser programs currently defined."));
		    getch();
		    fflush(stdin);
		    CLRLIN(MSGLINE, 0);

		    if (p_td_table != NULL)  {
			    free((char *) p_td_table);	/* release memory for td order tbl */
		    }

		    return (-1);
	    }

	    else {		/* some HE's defined.                */
		    if ((shm_addr.hdr_addr)->max_entries != num_entries) {	/* # HE change? */
			    num_entries = init_ahd_tbl(&p_td_table);

			    if (num_entries <= 0)  {
				    return (num_entries);
			    }	    /* problem in init fcn - bye!      */

			    else {		/* init fcn ran ok. */
				    if (num_entries <= 13) {	/* 13 or fewer entries?          */
					    max_strt_ent = 1;
				    }
				    else {	/* more than 13 entries */
					    max_strt_ent = num_entries;
				    }		/* endif */
			    }		/* endif */
		    }			/* endif */
	    }			/* endif */

	    /*
	     * build screen data for the current strt_ent
	     */
	    p_into_table = p_td_table + strt_ent - 1;
	    num_disp = num_entries - strt_ent + 1;
	    if (num_disp > 13)  {
		    num_disp = 13;
	    }

	    for (i = 1; i <= 13; i++) {
		    if (i <= num_disp) {	/* something there? */

			    p_htxshm_HE = shm_addr_wk.HE_addr + p_into_table->shm_pos;
			    /*
			     * build output characters & attributes for entry
			     */
			    /*
			     * screen entry number
			     */
#ifdef	__HTX_LINUX__
			    wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
			    wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
			    wmove(stdscr, (i + 6), 3);
			    wprintw(stdscr, "%2d", i);

			    /*
			     * Device Status
			     */

			    work =  get_dst(p_htxshm_HE, p_into_table->shm_pos, &dev_status);
			    workint = htx_strlen(work);	/* length of status string */
			    htx_strncpy(workstr, "        ", 8 - workint);	/* spaces */
			    workstr[8 - workint] = '\0';

			    /* set display attributes based on status */
			    if (dev_status == DST_CP || dev_status == DST_ST || dev_status == DST_RN)  {
#ifdef	__HTX_LINUX__
				    wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
				    wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
			    }

			    if (dev_status == DST_UK || dev_status == DST_SG || dev_status == DST_TM)  {
#ifdef	__HTX_LINUX__
				    wcolorout(stdscr, WHITE_BLUE | STANDOUT);
#else
				    wcolorout(stdscr, STANDOUT | F_WHITE | B_BLUE);
#endif
			    }

			    if (dev_status == DST_DD)  {
#ifdef	__HTX_LINUX__
				    wcolorout(stdscr, RED_BLACK | STANDOUT);
#else
				    wcolorout(stdscr, STANDOUT | F_RED | B_BLACK);
#endif
			    }

			    if (dev_status == DST_ER || dev_status == DST_HG)   {
#ifdef	__HTX_LINUX__
				    wcolorout(stdscr, RED_BLACK | NORMAL);
#else
				    wcolorout(stdscr, NORMAL | F_RED | B_BLACK);
#endif
			    }

			    /* display status */
			    mvwaddstr(stdscr, (i + 6), 6, work);

			    /* pad field with spaces */
#ifdef	__HTX_LINUX__
			    wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
			    wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
			    mvwaddstr(stdscr, (i + 6), 6 + workint, workstr);

			    /*
			     * Slot, Port, /dev/id, Adapt Desc, and Device
			     * Desc
			     */
			    wmove(stdscr, i + 6, 15);
#ifdef	__HTX_LINUX__
			    wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
			    wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif

			    sprintf(workstr, " %.4d ", p_htxshm_HE->slot);
			    mvwaddstr(stdscr, (i + 6), 15, workstr);
			    sprintf(workstr, " %.4d ", p_htxshm_HE->port);
			    mvwaddstr(stdscr, (i + 6), 22, workstr);

			    /* set color for device id based on status */
			    if (dev_status == DST_TM || dev_status == DST_DD || dev_status == DST_UK || (dev_no+1) == i )  {
#ifdef	__HTX_LINUX__
				    wcolorout(stdscr, WHITE_BLUE | STANDOUT);
#else
				    wcolorout(stdscr, STANDOUT | F_WHITE | B_BLUE);
#endif
			    }

			    else  {
#ifdef	__HTX_LINUX__
				    wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
				    wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
			    }

			    sprintf(workstr, " %-7s ", p_htxshm_HE->sdev_id);
			    mvwaddstr(stdscr, (i + 6), 29, workstr);
#ifdef	__HTX_LINUX__
			    wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
			    wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif

			    sprintf(workstr, " %-11s ", p_htxshm_HE->adapt_desc);
			    mvwaddstr(stdscr, (i + 6), 39, workstr);

			    sprintf(workstr, " %-18s ", p_htxshm_HE->device_desc);
			    mvwaddstr(stdscr, (i + 6), 53, workstr);

		    }

		    else {		/* no HE entry for this area of screen */
#ifdef	__HTX_LINUX__
			    wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
			    wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
			    mvwaddstr(stdscr, (i + 6), 3, "  ");	/* scn entry num */
			    mvwaddstr(stdscr, (i + 6), 6, "        ");	/* COE Flag  */
			    mvwaddstr(stdscr, (i + 6), 15, "      ");	/* slot fld */
			    mvwaddstr(stdscr, (i + 6), 22, "      ");	/* port fld */
			    mvwaddstr(stdscr, (i + 6), 29, "         ");	/* sdev_id */
			    mvwaddstr(stdscr, (i + 6), 39, "             ");
		/* adapt_desc field */
			    mvwaddstr(stdscr, (i + 6), 53, "                    ");	/* device_desc field */
		    }			/* endif */
		    p_into_table++;
	    }			/* endfor */

#ifdef	__HTX_LINUX__
	    wcolorout(stdscr, NORMAL);
#else
	    wcolorout(stdscr, NORMAL);
#endif

	    for (update_screen = FALSE; (update_screen == FALSE);) {

		    /*
		     * Read input from keyboard...
		     */
		    htx_strncpy(input, "", DIM(input));	/* clear input */
		    get_string(stdscr, RD_IN_ROW, RD_IN_COL, input, (size_t) DIM(input), (char *) NULL, (tbool) TRUE);

		    switch (input[0]) {
			    case 'f':
			    case 'F':
					dev_no = -1;
				    if (strt_ent < max_strt_ent) {
					    strt_ent += 13;
					    if (strt_ent > max_strt_ent)  {
						    strt_ent = max_strt_ent;
					    }
					    update_screen = TRUE;
				    }

				    else {
					    beep();
				    }		/* endif */
				    break;

			    case 'b':
			    case 'B':
					dev_no = -1;
				    if (strt_ent > 1) {
					    strt_ent -= 13;
					    if (strt_ent < 1)  {
						    strt_ent = 1;
					    }
					    update_screen = TRUE;
				    }

				    else {
					    beep();
				    }		/* endif */
				    break;

			    case 'q':
			    case 'Q':
				    if (p_td_table != NULL)  {
					    free((char *) p_td_table);	/* rel mem for td table */
				    }
				    return (0);

			    case 'd':
			    case 'D':
					dev_no = -1;
				    clear();
				    refresh();
				    display_scn(stdscr, 0, 0, LINES, COLS, "RD_scn", 1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
				    update_screen = TRUE;
				    break;

				case '/':
					rc = search_win(16, 50, 2, 2, "ddsrch_scn", 1, page_str);
					dev_len = strlen(page_str);
	
					if ( rc == -1 ) {
						(void) clear();
						wrefresh(stdscr);
						/* (void) refresh(); */
						(void) display_scn(stdscr, 0, 0, LINES, COLS, "RD_scn",
							1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
						PRTMSG(MSGLINE, 0, ("No String entered."));
						update_screen = TRUE;
						dev_no = -1;
						break;
					}
	
					if ( rc == 0 ) {
						(void) clear();
						wrefresh(stdscr);
						(void) display_scn(stdscr, 0, 0, LINES, COLS, "RD_scn",
							1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
						update_screen = TRUE;
						dev_no = -1;
						break;
					}
	
					for ( k=0; k<num_entries; k++) {
						struct ahd_entry    *p_into_table_temp;
						struct htxshm_HE    *p_htxshm_HE_temp;
	
						p_into_table_temp = p_td_table + k;
						p_htxshm_HE_temp = shm_addr_wk.HE_addr + p_into_table_temp->shm_pos;
	
						if ( strncmp(p_htxshm_HE_temp->sdev_id, page_str, dev_len) == 0 ) {
							break;
						}
					}
	
					if ( k >= num_entries ) {
						(void) clear();
						wrefresh(stdscr);
						(void) refresh();
						(void) display_scn(stdscr, 0, 0, LINES, COLS, "RD_scn",
							1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
						PRTMSG(MSGLINE, 0, ("Device not found."));
						update_screen = TRUE;
						dev_no = -1;
						break;
					}
	
					dev_no = k%13;
	
					/* Points to the 1st entry of 13-entry-page
					 * in which this device will be displayed.
					 */
					strt_ent = (((k/13) * 13) + 1);
					if (strt_ent > max_strt_ent) strt_ent = max_strt_ent;
					if (strt_ent <= 0) strt_ent = 1;
	
					(void) clear();
					(void) refresh();
					(void) display_scn(stdscr, 0, 0, LINES, COLS, "RD_scn",
							1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
					update_screen = TRUE;
					break;

			    default:
					dev_no = -1;
				    cmp_regexp = regcmp(input, (char *) 0);	/* compile exp */
				    if (cmp_regexp != NULL) {	/* regular expression OK? */
					    for (i = 1; i <= num_disp; i++) {
						    sprintf(workstr, "%-d", i);
						    work = regex(cmp_regexp, workstr);
						    if (work != NULL) {
							    htx_strncpy(ret, " ", (size_t)
									    sizeof(ret));
							    /* clear return (ret) array */
							    htx_strncpy(ret, __loc1, (size_t)
									    (work - __loc1));
							    if (htx_strcmp(workstr, ret) == 0) {	/* matched reg exp? */
								    update_screen = TRUE;
								    p_into_table = p_td_table + (strt_ent - 1) + (i - 1);
								    p_htxshm_HE = shm_addr_wk.HE_addr + p_into_table->shm_pos;
								    if (p_htxshm_HE->PID == 0) {
#ifdef	__HTX_LINUX__
									    wcolorout(stdscr, WHITE_BLUE | STANDOUT);
#else
									    wcolorout(stdscr, STANDOUT | F_WHITE | B_BLUE);
#endif
									    mvwaddstr(stdscr, (i + 6), 6, "RESTART");
#ifdef	__HTX_LINUX__
									    wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
									    wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
									    CLRLIN(MSGLINE, 0);
									    restart_exerciser(shm_addr.hdr_addr, p_into_table->shm_pos);
									    sleep(2);
								    } /* endif */
								    else {
									    sprintf(workstr, "The exerciser for %s, %s is already running.", p_htxshm_HE->sdev_id, p_htxshm_HE->HE_name);
									    PRTMSG(MSGLINE, 0, (workstr));
								    }	/* else */
							    }	/* endif */
						    }	/* endif */
					    }		/* endfor */

					    if (update_screen != TRUE) {	/* no match? */
						    beep();
					    }		/* endif */
				    }

				    else {	/* invalid entry_number */
					    beep();
				    }		/* endif */

				    break;
		    }			/* endswitch */

	    }			/* endfor */
    }				/* endfor */
}				/* R_device */

/* vi: set ts=4*/


int add_pid_to_shm( int pid, struct htxshm_HE * p_he )
{
//  int cur_ecg_pos = 0;
  int i, rc = -1;

  p_he->PID  = pid;
  NUM_EXERS = shm_addr.hdr_addr->max_entries;
  for (i = 0; i < NUM_EXERS; i++) {
     if( strcmp(p_he->sdev_id, EXER_NAME(i) ) == 0 ) {
           rc = 0;
           EXER_PID(i) = pid;
           /*printf("added the pid:%d for %s in ecg.all\n",EXER_PID(i), EXER_NAME(i));*/
      break;
         }
  }
  return rc;

}

