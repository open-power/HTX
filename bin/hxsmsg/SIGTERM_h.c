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

/* @(#)06	1.7  src/htx/usr/lpp/htx/bin/hxsmsg/SIGTERM_h.c, htx_msg, htxubuntu 5/21/07 05:28:29 */

/*
 *   FUNCTIONS: SIGTERM_hdl
 */

#include <unistd.h>
#include <sys/types.h>

#include "hxsmsg.h"


/*
 * NAME: SIGTERM_hdl()
 *
 * FUNCTION: Handles the SIGTERM (death of a child process) signal.
 *
 * EXECUTION ENVIRONMENT:
 *
 *	This function is called asychronously by the operating system
 *      when the SIGTERM signal is delivered to this program, hxsmsg.
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
 *      send SIGUSR1 signal to supervisor to shut us down.
 *
 *      if unable to send SIGUSR1 signal
 *        put HTX_SYS_FINAL_MSG on message queue
 *
 *        if unable to put HTX_SYS_FINAL_MSG on queue
 *          call clean_up
 *          call exit()
 *
 *       return
 *
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

void SIGTERM_hdl(int signal_number, int code, struct sigcontext *scp)
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

  int rc;

  extern char *program_name; /* this program's name (argv[0])                */

  extern int main_exit_code; /* exit code for main() function                */


  /*
   ***  Beginning of Executable Code  *****************************************
   */

  if (signal_number != SIGTERM)
    {
      (void) sprintf(error_msg,
		     "%s -- Invalid signal (%d) passed to SIGTERM_hdl().\n",
		     program_name,
		     signal_number);

      (void) send_message(error_msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
    }
  else
  {
      main_exit_code |= SIGTERM_RECVD;

      (void) sprintf(error_msg,
		     "%s -- SIGTERM signal received.\n\
Sending the SIGUSR1 signal to the supervisor to shut us down...\n",
		     program_name);
      (void) send_message(error_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);

      if (send_signal(getppid(), SIGUSR1) != GOOD) {
	     (void) sprintf(error_msg,
			 "%s -- Error sending SIGUSR1 signal to the \
supervisor.\n\
Putting HTX_SYS_FINAL_MSG on IPC message queue to shut us down...\n",
			 program_name);

	     if (getppid() == 1)
	        rc = send_message(error_msg, 0, HTX_SYS_INFO, HTX_SYS_FINAL_MSG);
	     else
	        rc = send_message(error_msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_FINAL_MSG);
	     if ( rc != GOOD)  {
	        (void) fprintf(stderr,
			     "\n%s -- Unable to put HTX_SYS_FINAL_MSG on \
the IPC message queue.\n\
Calling clean_up() function and exit()...\n",
			     program_name);

	      (void) fflush(stderr);
	      if (clean_up() != GOOD)
		main_exit_code |= BAD_CLEAN_UP;

	      exit(main_exit_code);

	     } /* endif */
	  } /* endif */
    } /* endif */

  return;

} /* SIGTERM_hdl() */
