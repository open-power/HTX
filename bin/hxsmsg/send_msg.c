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

/* @(#)23	1.7  src/htx/usr/lpp/htx/bin/hxsmsg/send_msg.c, htx_msg, htxubuntu 5/24/04 18:18:33 */

/*
 *   FUNCTIONS: send_message
 */


#include "hxsmsg.h"

#include <time.h>
#include <sys/time.h>

/*
 * Error code #define's for send_message()
 */
#define MSG_TOO_LONG 0x0001
#define BAD_GETTIMER 0x0002
#define BAD_MSGSND 0x0004
#define NO_MSG_QUEUE 0x0008


/*
 * NAME: send_message()
 *                                                                    
 * FUNCTION: Sends a message to the HTX IPC message queue for inclusion
 *           in the htxmsg/htxerr log files.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the SIGCHLD_hdl(), and SIGTERM_hdl()
 *      signal handler functions.
 *
 *      This message handler program, "hxsmsg", is always a child process
 *      of the HTX supervisor program, "hxssup".
 *                                                                   
 * NOTES: 
 *
 *      operation:
 *      ----------
 *      if IPC message queue available
 *        check the message length
 *        build a character time string
 *        set up the htx_data structure and format the message
 *        put the message on the message queue (msgsnd() call)
 *      else
 *        write message to stderr
 *      endif
 *
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      ------------------------------------------------------------------
 *       0 (0x0000) -- Normal exit.
 *       1 (0x0001) -- msg_text message was too long (message truncated).
 *       2 (0x0002) -- gettimer() system call failed (used 0).
 *       4 (0x0004) -- msgsnd() system call failed.
 *       8 (0x0008) -- IPC message queue not available.
 *
 *      NOTE: Each of the error codes listed above takes up a single bit
 *            position.  Thus, multiple error conditions are indicated
 *            by "OR'ing" the indicated error codes into a composite
 *            return code.
 *
 *            For example, if the msg_text message was too long (error
 *            code 0x0001) and the msgsnd() system call failed (error
 *            code 0x0004), the return code would be 0x0001 | 0x0004 =
 *            0x0005 (decimal 5).
 *
 *
 */  

short send_message(char *msg_text, int errno_val, int severity,
		   mtyp_t msg_type)
     /*
      * msg_text -- the text of the message
      * errno_val -- the error number value
      * sev_code -- the severity code
      * msg_type -- the message type (SYS_STATUS, SYS_FINAL_MSG)
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char char_time[22];   /* variable string for formatted system time    */
  	char error_msg[128];  /* error message string                         */
  	char *str_time_ptr;   /* pointer to string format of system time      */

  	extern char *program_name;  /* this program's name (argv[0])          */

  	extern int msgqid;         /* IPC message queue id                    */

  	int errno_save;            /* save area for errno value               */

  	short exit_code;           /* exit program return code                */

  	size_t str_length;         /* string length variable                  */
 
  	struct htx_msg_buf msg_buffer;  
		                   /* message buffer for HTX IPC msg queue    */

  	#if defined(__HTX_LINUX__) || defined(__OS400__)
  	time_t the_time;
  	#else
  	struct timestruc_t system_time; /* system time value from gettimer()  */
  	#endif /* __HTX_LINUX__ */

  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	exit_code = GOOD;
  
  	if (msgqid != -1)          /* message queue set up?                   */
    	{
      /*
       * Check message length...
       */
      		if (strlen(msg_text) > MAX_TEXT_MSG)
		{
	  		msg_text[MAX_TEXT_MSG] = '\0';  
					/* truncate to max length          */
	  		exit_code |= MSG_TOO_LONG;      
					/* set message too log bit         */
		} /* endif */
      
      
      /*
       * Build the character time string...
       */
      		errno = 0;
      		#if defined(__HTX_LINUX__) || defined(__OS400__)
      		if ( (the_time = time((time_t *) 0)) == 0)
      		#else
      		if(gettimer(TIMEOFDAY , &system_time) != GOOD)
      		#endif /* __HTX_LINUX__ */
		{
	  		errno_save = errno;

	  		(void) sprintf(error_msg,
			"\n%s -- Error in the gettimer() system call of the \
			send_message() function.\nerrno: %d (%s).\n",
			program_name,
			errno_save, 
			strerror(errno_save));

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code |= BAD_GETTIMER;
	  		(void) htx_strcpy(char_time, "gettimer() error");
		}
      		else
		{
	 		#if defined(__HTX_LINUX__) || defined(__OS400__)
	  		str_time_ptr = ctime((time_t *)(&the_time)); 
	  		#else
	  		str_time_ptr = ctime((time_t *)&(system_time.tv_sec));
	  		#endif /* __HTX_LINUX__ */
	  
	  		(void) htx_strncpy(char_time, "", sizeof(char_time));
	  		(void) htx_strncpy(char_time, (str_time_ptr + 4), 20);
		} /* endif */
      
      
      /*
       * Set up the "htx_data" portion of the message buffer...
       */
      		(void) htx_strncpy(msg_buffer.htx_data.sdev_id, "",sizeof(msg_buffer.htx_data.sdev_id));

      		(void) htx_strncpy(msg_buffer.htx_data.sdev_id, "htx_messages",(sizeof(msg_buffer.htx_data.sdev_id) - 1));

      		msg_buffer.htx_data.error_code = errno_val;

      		msg_buffer.htx_data.severity_code = severity;

      		(void) htx_strncpy(msg_buffer.htx_data.HE_name, "",sizeof(msg_buffer.htx_data.HE_name));

      		(void) htx_strncpy(msg_buffer.htx_data.HE_name, program_name,(sizeof(msg_buffer.htx_data.HE_name) - 1));
      
      /*
       * Format the message...
       */
      		(void) sprintf(msg_buffer.htx_data.msg_text,
		     "%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n%-s\n",
		     msg_buffer.htx_data.sdev_id,
		     char_time, 
		     msg_buffer.htx_data.error_code,
		     msg_buffer.htx_data.severity_code,
		     msg_buffer.htx_data.HE_name,
		     msg_text);
      
      /*
       * Make sure that the last two characters are '\n'...
       */
      		str_length = strlen(msg_buffer.htx_data.msg_text);
      		if (msg_buffer.htx_data.msg_text[str_length - 2] != '\n')
		{
			(void) htx_strcat(msg_buffer.htx_data.msg_text, "\n");
		}
      
      		msg_buffer.mtype = msg_type;
      
      		errno = 0;
      		if (msgsnd(msgqid, &msg_buffer, (sizeof(msg_buffer) - sizeof(mtyp_t)), 0)
	  != GOOD)
		{
	  		errno_save = errno;

	  		(void) sprintf(error_msg,
			"\n%s -- Error in msgsnd() system call of the \
			send_message() function.\nerrno: %d (%s).\n",
			program_name,
			errno_save,
			strerror(errno_save));

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code |= BAD_MSGSND;
		} /* endif */
    	}
  	else                   /* message queue not set up -- write to stderr */
    	{
      		(void) fprintf(stderr, "%s", msg_text);
      		(void) fflush(stderr);
      		exit_code |= NO_MSG_QUEUE;
    	} /* endif */

  	return(exit_code);

} /* send_message() */
