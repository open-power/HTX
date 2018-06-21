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

/* @(#)05	1.6.4.1  src/htx/usr/lpp/htx/bin/hxsmsg/SIGCHLD_h.c, htx_msg, htxubuntu 8/3/12 07:51:36 */

/*
 *   FUNCTIONS: SIGCHLD_hdl
 */


#include "hxsmsg.h"           
                                               

#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>


/*
 * NAME: SIGCHLD_hdl()
 *                                                                    
 * FUNCTION: Handles the SIGCHLD (death of a child process) signal.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called asychronously by the operating system
 *      when the SIGCHLD signal is delivered to this program, hxsmsg.
 *
 *      The message handler program, "hxsmsg", is always a child process
 *      of the HTX supervisor program, "hxssup".
 *                                                                   
 * NOTES: 
 *
 *      operation:
 *      ----------
 *      check the passed signal number
 *      
 *      call wait3 WNOHANG to get status of killed child process
 *
 *      if child PID == 0 ===> No children dead.
 *        print error message
 *
 *      else if child == hxsstress
 *        print cause of child death
 *        send SIGUSR1 signal to supervisor to shut us down.
 *
 *        if unable to send SIGUSR1 signal
 *          put HTX_SYS_FINAL_MSG on message queue
 * 
 *          if unable to put HTX_SYS_FINAL_MSG on queue
 *            call clean_up
 *            call exit()
 *
 *       return
 *
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      -------------
 *      None -- void function.
 *
 *
 */  

void SIGCHLD_hdl(int signal_number, int code, struct sigcontext *scp)
     /*
      * signal_number -- the number of the received signal
      * code -- unused
      * scp -- pointer to context structure
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  char error_msg[128];       /* error message string                         */
  char tag[64];              /* string area                                  */

  extern char *program_name; /* this program's name (argv[0])                */

  extern int main_exit_code; /* main() function's exit code                  */

  extern pid_t hxsstress_PID; /* hxsstress program process ID                */

  extern int system_call;     /* system() call flag                          */     

  int errno_save;            /* save area for errno                          */
  int status;                /* status area for wait3()                      */

  pid_t child_PID;           /* the PID of the child process that died       */


  /*
   ***  Beginning of Executable Code  *****************************************
   */

  if (signal_number != SIGCHLD)
    {
      (void) sprintf(error_msg,
		     "%s -- Invalid signal (%d) passed to SIGCHLD_hdl().\n",
		     program_name, 
		     signal_number);

      (void) send_message(error_msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
    }
  else if(system_call == TRUE)
    {
      return;
    }
  else
    {
      errno = 0;
#ifdef __OS400__
      if ((child_PID = waitpid(-1, &status, WNOHANG)) == -1)
#else
      if ((child_PID = wait3(&status, WNOHANG, (struct rusage *) 0)) == -1)
#endif
	{
	  errno_save = errno;

	  (void) sprintf(error_msg,
			 "%s -- Error on wait3() WNOHANG call in \
SIGCHLD_hdl().\nerrno: %d (%s).\n",
			 program_name,
			 errno_save,
			 strerror(errno_save));

	  (void) send_message(error_msg, errno_save, HTX_SYS_HARD_ERROR,
			      HTX_SYS_MSG);
	}
      else if (child_PID == 0)
	{
	  (void) sprintf(error_msg,
			 "%s -- wait3() WNOHANG call in SIGCHLD_hdl() \
returned 0.\nNo child processes have stopped or exited!\n",
			 program_name);

	  (void) send_message(error_msg, 0, HTX_SYS_SOFT_ERROR,
			      HTX_SYS_MSG);
	}
      else if ((hxsstress_PID != -1) && (child_PID == hxsstress_PID))
	{
	  hxsstress_PID = -1;
	  if ((status & 0x000000ff) == 0x00000000)  /* term by exit() call? */
	    (void) sprintf(tag, "exit(%d) call", ((status & 0x0000ff00)>>8));
	  else if ((status & 0x0000ff00) == 0x00000000)
	    (void) sprintf(tag, "signal %d", (status & 0x0000007f));
	  
	  (void) sprintf(error_msg, 
			 "%s -- hxsstress program has been terminated by a \
%s.\nSending SIGUSR1 signal to hxssup to terminate test...\n",
			 program_name, 
			 tag);

	  (void) send_message(error_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);

	  if (send_signal(getppid(), SIGUSR1) != GOOD)
	    {
	      (void) sprintf(error_msg,
			     "%s -- Error sending SIGUSR1 signal to the \
supervisor.\n\
Putting HTX_SYS_FINAL_MSG on IPC message queue to shut us down...\n",
			     program_name);
	  
	      if (send_message(error_msg, 0, HTX_SYS_HARD_ERROR, 
			       HTX_SYS_FINAL_MSG)
		  != GOOD)
		{
		  (void) fprintf(stderr, 
				 "\n%s -- Unable to put HTX_SYS_FINAL_MSG \
on the IPC message queue.\n\
Calling clean_up() function and exit()...\n",
				 program_name);

		  (void) fflush(stderr);
		  if (clean_up() != GOOD)
		    main_exit_code |= BAD_CLEAN_UP;
		  
		  exit(main_exit_code);
		  
		} /* endif */
	    } /* endif */
	}
      else
	{
	  (void) sprintf(error_msg, 
			 "%s -- Error: Child process id (%d) returned by \
the wait3()\nsystem call is not any child process that I recognize.\n",
			 program_name, 
			 child_PID);

	  (void) send_message(error_msg, 0, HTX_SYS_SOFT_ERROR,
			      HTX_SYS_MSG);
	}
    } /* endif */

  return;

} /* SIGCHLD_hdl() */
