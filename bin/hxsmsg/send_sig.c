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

/* @(#)24	1.6  src/htx/usr/lpp/htx/bin/hxsmsg/send_sig.c, htx_msg, htxubuntu 10/10/05 05:38:47 */

/*
 *   FUNCTIONS: send_signal
 */


#include "hxsmsg.h"

#include <signal.h>

#ifdef __HTX_LINUX__
#define SIGMAX 32
#endif /* __HTX_LINUX__ */

#define BAD_PROCESS_ID 1
#define BAD_SIGNAL_NUMBER 2
#define BAD_KILL 3

/*
 * NAME: send_signal()
 *                                                                    
 * FUNCTION: Sends a signal to the specified process.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the check_wrap() function of the "hxsmsg"
 *      program.  The message handler program, "hxsmsg", is always a child
 *      process of the HTX supervisor program, "hxssup".
 *
 * NOTES: 
 *
 *      operation:
 *      ---------
 *
 *      check process_id
 *      check signal_number
 *      call kill() system call to send signal
 *      check for errors
 *
 *      return
 * 
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      -------------
 *       0 -- Normal exit.
 *       1 -- Bad process_id parameter passed to function.
 *       2 -- Bad signal_number parameter passed to function.
 *       3 -- Error from kill() system call.
 *
 *
 */  

short send_signal(pid_t process_id, int signal_number)
     /*
      * process_id -- the process id of the process to get the signal
      * signal_number -- the signal number of the signal to be sent
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char error_msg[128];       /* error message string                    */

  	extern char *program_name; /* this program's name (argv[0])           */

  	int errno_save;            /* errno save area                         */

  	short exit_code;           /* exit program return code                */

	FILE *fp;
	
	char temp_str[256];


  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	exit_code = GOOD;

  	errno = 0;
  	if (process_id <= 2)       /* invalid process id?                    */
    	{
      		(void) sprintf(error_msg,
		"%s -- error in send_signal(),\n\
		Passed process id = %d (decimal).\n",
		program_name, 
		process_id);
		
		errno = 0;
		sprintf(temp_str, "%s/.autostart",global_htx_home_dir);
		fp = fopen(temp_str, "r");
		errno_save = errno;
		if (fp == NULL && errno_save == 2)
		{
      		   (void) send_message(error_msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
      		exit_code = BAD_PROCESS_ID;
		}
		else
		{
		   (void) send_message(error_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
                }
		fclose(fp);
    	}

  	else if ((signal_number <= 0) || (signal_number > SIGMAX))
    	{
      		(void) sprintf(error_msg,
		"%s -- error in send_signal(),\n\
		Passed signal number = %d (decimal).\n",
		program_name, 
		signal_number);

      		(void) send_message(error_msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
      		exit_code = BAD_SIGNAL_NUMBER;
    	}

  	else if (kill(process_id, signal_number) != GOOD)
    	{
      		errno_save = errno;

      		(void) sprintf(error_msg,
		"%s -- error on kill() in send_signal()\n\
		errno: %d (%s).\n",
		program_name,
		errno_save,
		strerror(errno_save));

     		(void) send_message(error_msg, errno_save, HTX_SYS_HARD_ERROR,
		HTX_SYS_MSG);
      		exit_code = BAD_KILL;
   	}	

  	return(exit_code);

} /* send_signal() */
