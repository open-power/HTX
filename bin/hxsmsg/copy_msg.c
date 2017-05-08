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

/* @(#)12	1.5.4.1  src/htx/usr/lpp/htx/bin/hxsmsg/copy_msg.c, htx_msg, htxubuntu 8/3/12 07:54:57 */

/*
 *   FUNCTIONS: copy_message
 *		write_pipe
 */

#include <unistd.h>

#include "hxsmsg.h"

/*
 * copy_msg() error code #defines
 */
#define BAD_MSG_WRITE 0x0001
#define BAD_MSG_SAVE_WRITE 0x0002
#define BAD_ERR_WRITE 0x0004
#define BAD_ERR_SAVE_WRITE 0x0008
#define BAD_PIPE_WRITE 0x0010

/*
 * NAME: copy_message()
 *                                                                    
 * FUNCTION: Copies ipc messages to appropriate log/pipe files.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the main() function of the "hxsmsg"
 *      program.  The message handler program, "hxsmsg", is always a child
 *      process of the HTX supervisor program, "hxssup".
 *
 * NOTES: 
 *
 *      operation:
 *      ---------
 *
 *      if msg log open
 *        write message to msg log
 *      if msg save log open
 *        write message to msg save log
 *
 *      if this is an error message
 *        if err log open
 *          write message to err log
 *        if err save log open
 *          write message to err save log
 *        if hxsstress pipe open && hxsstress program running
 *          send info to the hxsstress pipe
 *
 *      return
 * 
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      ------------------------------------------------------------------
 *       0 (0x0000) -- Normal exit.
 *       1 (0x0001) -- Error from write_log() of the message log.
 *       2 (0x0002) -- Error from write_log() of the message save log.
 *       4 (0x0004) -- Error from write_log() of the error log.
 *       8 (0x0008) -- Error from write_log() of the error save log.
 *      16 (0x0010) -- Error from write_pipe() call.
 *
 *      NOTE: Each of the error codes listed above take up a single bit
 *            position.  Thus, multiple error conditions are indicated
 *            by "OR'ing" the indicated error codes into a composite
 *            return code.
 *
 *            For example, if the write_log() call for the error log failed
 *            (error code 0x0004) and the write_pipe() call failed (error
 *            code 0x0010), the return code would be 0x0004 | 0x0010 =
 *            0x0014 (decimal 20).
 *
 */  

short copy_message(struct htx_msg_xbuf *p_message_buffer, off_t err_thres,
		   off_t msg_thres, off_t err_save_thres,
		   off_t msg_save_thres, tbool err_wrap_flag,
		   tbool msg_wrap_flag, tbool msg_archive_flag)
     /*
      * p_message_buffer -- pointer to ipc message buffer
      * err_thres -- error log max file size/wrap threshold
      * msg_thres -- message log max file size/wrap threshold
      * err_save_thres -- error save log max file size/wrap threshold
      * msg_save_thres -- message save log max file size/wrap threshold
      * err_wrap_flag -- error log wrap flag
      * msg_wrap_flag -- message log wrap flag
      */
{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	extern ckpt checkpt;       /* Checkpoint file structure               */
  	extern int err_log;        /* Error Log File Descriptor               */
  	extern int err_save_log;   /* Error Save Log File Descriptor          */
  	extern int msg_log;        /* Message Log File Descriptor             */
  	extern int msg_save_log;   /* Message Save Log File Descriptor        */
  	extern int pipe_dev[];     /* pipe device fileno array                */

  	extern pid_t hxsstress_PID;  /* hxsstress program PID                 */

  	short exit_code;           /* exit program return code                */

  	size_t text_str_len;       /* the string length of the message text   */

#ifdef __HTX_LINUX__
        struct disp_msg msg_buf;   /* display device structure                */        extern int queue_id;
#endif

  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	exit_code = GOOD;
  	text_str_len = strlen(p_message_buffer->htx_data.msg_text);
	
	
#ifdef __HTX_LINUX__
	if(p_message_buffer->htx_data.severity_code == HE_STATUS )
        {
                msg_buf.mtype = p_message_buffer->mtype;
                htx_strcpy( msg_buf.msg ,p_message_buffer->htx_data.msg_text);
                if(msgsnd(queue_id, &msg_buf, sizeof(msg_buf), 0))
                {
                        fprintf(stderr,"\nSend to display device failed");
                        exit_code = BAD_MSG_WRITE;
                }
                return(exit_code);
        }
#endif


  	if (msg_log != -1)
	{
    		if (write_log(&msg_log, MSG_LOG, p_message_buffer->htx_data.msg_text,
		  text_str_len, msg_thres, msg_wrap_flag, TRUE, 
		  &(checkpt.msg_offset), &(checkpt.msg_mod_time), msg_archive_flag)
		!= GOOD)
		{
      			exit_code |= BAD_MSG_WRITE;
		}
	}


  	if (msg_save_log != -1)
	{
    		if (write_log(&msg_save_log, MSG_SAVE_LOG,
		  p_message_buffer->htx_data.msg_text,
		  text_str_len, msg_save_thres, FALSE, FALSE,
		  &(checkpt.msg_save_offset), &(checkpt.msg_save_mod_time), msg_archive_flag)
		!= GOOD)
		{
      			exit_code |= BAD_MSG_SAVE_WRITE;
		}
	}

  	if (p_message_buffer->htx_data.severity_code < HTX_SYS_INFO)
    	{

      		if (err_log != -1)
		{
			if (write_log(&err_log, ERR_LOG, p_message_buffer->htx_data.msg_text,
			text_str_len, err_thres, err_wrap_flag, TRUE,
			&(checkpt.err_offset), &(checkpt.err_mod_time), msg_archive_flag)
	    		!= GOOD)
			{
	  			exit_code |= BAD_ERR_WRITE;
			}
		}
	

      		if (err_save_log != -1)
		{
			if (write_log(&err_save_log, ERR_SAVE_LOG,
		      	p_message_buffer->htx_data.msg_text,
		      	text_str_len, err_save_thres, FALSE, FALSE,
		      	&(checkpt.err_save_offset),
		      	&(checkpt.err_save_mod_time), msg_archive_flag)
	    	      	!= GOOD)
			{
	  			exit_code |= BAD_ERR_SAVE_WRITE;
			}
		}


      /*
       * If the hxsstress program is running, we need to send some info
       * to the hxsstress pipe.
       */
      		if ((pipe_dev[1] != -1) && (hxsstress_PID != -1))
		{
			if (write_pipe(p_message_buffer) != GOOD)
			{
	  			exit_code |= BAD_PIPE_WRITE;
			}
		}

    	} /* endif */

  	return(exit_code);

} /* copy_message() */




/*
 * NAME: write_pipe()
 *                                                                    
 * FUNCTION: Writes message to the hxstress pipe.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the copy_msg() function of the "hxsmsg"
 *      program.  The message handler program, "hxsmsg", is always a child
 *      process of the HTX supervisor program, "hxssup".
 *
 * NOTES: 
 *
 *      operation:
 *      ---------
 *
 *      if msg log open
 *        write message to msg log
 *      if msg save log open
 *        write message to msg save log
 *
 *      if this is an error message
 *        if err log open
 *          write message to err log
 *        if err save log open
 *          write message to err save log
 *        if hxsstress pipe open && hxsstress program running
 *          write dev id and error code to hxsstress pipe
 *
 *      return
 * 
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      -------------
 *         0 -- Normal exit.
 *         1 -- Error on write().
 *         2 -- Incorrect number of bytes transferred on write().
 *
 *
 */  

short write_pipe(struct htx_msg_xbuf *p_message_buffer)
     /*
      * p_message_buffer -- pointer to ipc message buffer
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char error_msg[128];       /* error message string                    */
  	char stress_msg[128];      /* string for message to hxsstress pipe    */

  	extern char *program_name; /* this program's name (argv[0])           */

  	extern int pipe_dev[];     /* pipe device fileno array                */

  	int errno_save;            /* errno save area                         */
  	int pipe_write_rc;         /* return code from write() to pipe device */

  	short exit_code;           /* exit program return code                */

  	size_t text_str_len;       /* the string length of the message text   */

  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	exit_code = GOOD;

  /*
   * Write a small subset of the error message (devid:errno) to the
   * hxsstress pipe.
   */
  	(void) sprintf(stress_msg, "%s:%-d\n", p_message_buffer->htx_data.sdev_id,
		 p_message_buffer->htx_data.error_code);
  		 text_str_len = strlen(stress_msg);

 	errno = 0;
 	if ((pipe_write_rc = write(pipe_dev[1], stress_msg,
	     (unsigned int) text_str_len)) == -1)
	{
		errno_save = errno;

		(void) sprintf(error_msg, 
	     	"\n%s -- Error writing to pipe.\nerrno: %d (%s).\n",
	     	program_name,
	     	errno_save,
	     	strerror(errno_save));

		(void) fprintf(stderr, "%s", error_msg);
      		(void) fflush(stderr);
      		exit_code = 1;
    	}
  	else if (pipe_write_rc != text_str_len)
    	{
      		(void) sprintf(error_msg,
	    	"\n%s -- Error writing to pipe.\n\
		Only %d of %zd bytes actually written.\n",
	     	program_name,
	     	pipe_write_rc,
	     	text_str_len);

 		(void) fprintf(stderr, "%s", error_msg);
      		(void) fflush(stderr);
      		exit_code = 2;
    	}
  
  	return(exit_code);

} /* write_pipe() */
  

