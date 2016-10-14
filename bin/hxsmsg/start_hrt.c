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

/* @(#)26	1.5  src/htx/usr/lpp/htx/bin/hxsmsg/start_hrt.c, htx_msg, htxubuntu 5/24/04 18:19:36 */

/*
 *   FUNCTIONS: start_heart
 */


#include <unistd.h>
#include "hxsmsg.h"

#define BAD_RETURN (pid_t) -1

/*
 * NAME: start_heart()
 *                                                                    
 * FUNCTION: Starts the hxsstress (heartbeat) program for stress lab 
 *           environment runs.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the main() function of the hxsmsg program.
 *
 *      This message handler program, "hxsmsg", is always a child process
 *      of the HTX supervisor program, "hxssup".
 *                                                                   
 * NOTES: 
 *
 *      operation:
 *      ----------
 *
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      ------------------------------------------------------------------
 *       -1 -- Unable to start hxsstress program
 *      n>0 -- process ID of the hxsstress program
 *
 *
 */  

pid_t start_heart(char *heartbeat_dev, char *heartbeat_cycle)
     /*
      * heartbeat_dev -- the "heartbeat" device
      * heartbeat_cycle -- the "heartbeat" cycle time
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char error_msg[1024];      /* error message string                    */
  	char exec_string[192];     /* string for exec() system call           */
  	char pipe_input_id[32];    /* input device id for pipe                */

  	extern char *program_name;  /* this program's name (argv[0])          */

  	extern int pipe_dev[];     /* pipe device int array                   */

  	int errno_save;            /* save area for errno value               */

  	pid_t exit_code;           /* exit code for return() statement        */
  	pid_t fork_pid;       /* process ID returned from fork() system call  */

  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	errno = 0;
  	if (pipe(pipe_dev) != GOOD)
    	{
      		errno_save = errno;

      		(void) sprintf(error_msg,
		 "\n%s -- Error on pipe() system call in start_heart().\n\
		errno: %d (%s).\n",
		program_name,
		errno_save,
		strerror(errno_save));
      
      		(void) fprintf(stderr, "%s", error_msg);
      		(void) fflush(stderr);
     	 	exit_code = BAD_RETURN;
    	}
  	else 
    	{
      		fork_pid = fork();
      		switch(fork_pid)
		{
			case 0:     /* child process                          */
	  			(void) set_signal_hdl(SIGCHLD,
			    	(void (*)(int, int, struct sigcontext *)) SIG_DFL);
	  			(void) set_signal_hdl(SIGTERM,
			    	(void (*)(int, int, struct sigcontext *)) SIG_DFL);
	  
				sprintf(exec_string, "%s/bin/hxsstress", global_htx_home_dir);

	  			errno = 0;
	  			if (sprintf(pipe_input_id, "%d", pipe_dev[0]) < 0)
	    			{
	      				errno_save = errno;
	      
	      				(void) sprintf(error_msg, 
			     		"\n%s (child process of fork()) -- Error on \
					sprintf() of\npipe_dev[0].\nerrno: %d (%s).\n",
			     		program_name,
			     		errno_save,
			     		strerror(errno_save));
	      
	      				(void) fprintf(stderr, "%s", error_msg);
	      				(void) fflush(stderr);
	    			}
	  			else
	    			{

	      				errno = 0;
	      				if (execl(exec_string, "hxsstress", heartbeat_dev, heartbeat_cycle, pipe_input_id, (char *) 0) == -1)

					{
		  				errno_save = errno;

		  				(void) sprintf(error_msg, 
				 		"\n%s (child process of fork()) -- Error on \
						exec().\nerrno: %d (%s).\n",
				 		program_name,
				 		errno_save,
				 		strerror(errno_save));
	      
		  				(void) fprintf(stderr, "%s", error_msg);
		  				(void) fflush(stderr);
					} /* endif */
	    			} /* endif */

	  			exit(1);
	  			break;

			case -1:
	  			errno_save = errno;

	  			(void) sprintf(error_msg, 
			 	"\n%s -- Error on fork().\nerrno: %d (%s).\n",
			 	program_name,
			 	errno_save,
			 	strerror(errno_save));
	      
	  			(void) fprintf(stderr, "%s", error_msg);
	  			(void) fflush(stderr);

	  			exit_code = BAD_RETURN;
	  			break;
	  
			default:
	  			exit_code = fork_pid;
	  			(void) sleep(1);       
				  /* give the child process a chance to start */
	  			break;
		} /* endswitch */
    	} /* endif */
  
  	return(exit_code);

} /* start_heart() */
