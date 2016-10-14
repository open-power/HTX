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

/* @(#)14	1.6.4.1  src/htx/usr/lpp/htx/bin/hxsmsg/hxsmsg.c, htx_msg, htxubuntu 8/3/12 07:57:25 */

/*
 *   FUNCTIONS: main
 */


#include "hxsmsg.h"

#define CYCLE_TEXT_SIZE 32

/*
 * main() error code definitions
 */
#define BAD_DO_ARGV 1
#define BAD_OPEN_LOGS 2
#define BAD_OPEN_IPC 3
#define BAD_START_HEART 4
#define BAD_SET_SIGTERM 5
#define BAD_SET_SIGCHLD 6
#define BAD_START_DEVICE 7

/*
 *  Global Variable Definitions  **********************************************
 */

char *program_name;        /* This program's name (argv[0])                */

ckpt checkpt = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0}; /* log files chkpnt struct  */

int ckpt_file = -1;        /* Checkpoint File File Descriptor              */
int err_log = -1;          /* Error Log File Descriptor                    */
int err_save_log = -1;     /* Error Save Log File Descriptor               */
int main_exit_code = 0;    /* exit program return code                     */
int msg_log = -1;          /* Message Log File Descriptor                  */
int msg_save_log = -1;     /* Message Save Log File Descriptor             */
int msgqid = -1;           /* Message queue id                             */
int queue_id = -1;
int pipe_dev[2] = {-1, -1}; /* pipe device array                           */
char global_htx_log_dir[256]={'\0'};
char global_htx_home_dir[256]={'\0'};
char CKPT_FILE[256];
char ERR_LOG[256];
char ERR_SAVE_LOG[256];
char MSG_LOG[256];
char MSG_SAVE_LOG[256];

pid_t hxsstress_PID = -1;  /* hxsstress "heartbeat" program PID            */


extern void set_htx_home_log_path(void);
/*
 * NAME: main()
 *                                                                    
 * FUNCTION: Handles messages from the HTX supervisor and Hardware Exerciser
 *           (HE) programs. 
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This procedure is invoked as the program "hxsmsg" by the HTX
 *      supervisor program, "hxssup".  The message handler program,
 *      "hxsmsg", is always a child process of the HTX supervisor 
 *      program, "hxssup".
 *
 *      This "hxsmsg" program acts as a daemon which handles messages
 *      initiated from the HTX supervisor and Hardware Exerciser programs.
 *      The way in which these messages are handled is determined by the
 *      parameters passed in the argv[] array.
 *
 *      Typically, the messages are saved to a message log file; and, in the
 *      case of error messages, the messages are also saved to a separate 
 *      error log file.
 *                                                                   
 * NOTES: 
 *
 *      argv[] parameters:
 *      ------------------
 *        argv[0] -- program name (hxsmsg)
 *        argv[1] -- max error log file size threshold (converted to long int
 *                   max_err_thres).  If the error log file is set to 
 *                   wrap, it does so after this file size threshold (num
 *                   of bytes) is met.
 *        argv[2] -- max message log file size threshold (converted to long
 *                   int max_msg_thres).  This works the same as argv[1].
 *        argv[3] -- save error log file size (converted to the long
 *                   integer max_err_save).  This specifies the size in 
 *                   bytes of a file which contains the first part of the
 *                   error log.  This "save" file does not wrap.  Once it has 
 *                   grown to the size specified by "max_err_save", nothing
 *                   else is added.
 *        argv[4] -- save message log file size (converted to the long
 *                   integer max_msg_save).  This works the same as argv[3].
 *        argv[5] -- error log wrap flag (converted to the short int 
 *                   err_wrap_flag).  This variable flag will contain a 
 *                   non-zero value if the error log file should wrap.
 *        argv[6] -- message log wrap flag (converted to the short integer
 *                   msg_wrap_flag).  This works the same as argv[5].
 *        argv[7] -- message log archive flag (converted to the short integer
 *                   msg_archive_flag).  This works the same as argv[5].
 *        argv[8] -- system auto-startup flag (converted to the short int
 *                   variable auto_start_flag).  This variable flag is set
 *                   to a non-zero value if the HTX system was started with
 *                   the auto-startup feature.
 *        argv[9] -- stress lab output device (character string is copied
 *                   to the "stress_dev" character array).  This array
 *                   contains the name of the output device for stress lab
 *                   error messages and system "heartbeat" messages.  A 
 *                   "heartbeat" message lets the stress lab monitoring
 *                   equipment know that the HTX system is still running.
 *                   A "/dev/null" device means non-stress test.
 *        argv[10] -- stress lab "heartbeat" cycle time.  This is the cycle time
 *                   time in seconds of the stress lab "heartbeat" message.
 *                   A cycle time of "0" means that this is a non-stress
 *                   test.
 *                                     
 *
 *      operation:
 *      ---------
 *
 *      check argv[] parameters
 *      open log files
 *      open ipc message queue
 *      if Stress Lab environment
 *          start "hxsstress" program
 *
 *      while more ipc messages
 *          read ipc message
 *          copy message to file/stress-pipe
 *
 *      clean_up (close files, ipc message queue)
 *
 *      exit program
 * 
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      ------------------------------------------------------------------
 *       0 -- Normal exit.
 *       1 -- Error in do_argv() function.
 *       2 -- Error in open_logs() function.
 *       3 -- Error in open_ipc() function.
 *       4 -- Error in start_heart() function.
 *       5 -- Error in set_signal_hdl for SIGTERM signal.
 *       6 -- Error in set_signal_hdl for SIGCHLD signal.
 *      32 (0x0020) -- Error in clean_up() function.
 *      64 (0x0040) -- SIGTERM signal received.
 *
 *      NOTE: The BAD_CLEAN_UP and SIGTERM_RECVD error codes take up a
 *            single bit position.  Thus, multiple error conditions may
 *            be indicated by "OR'ing" the indicated error codes into a
 *            composit return code.
 *
 *            For example, if the start_heart() function failed (error code
 *            4 -- 0x0004) and the clean_up() function failed (error code
 *            0x0010),the return code would be 0x0004 | 0x0020 = 0x0024
 *            (decimal 36).
 *
 *
 */  

int main(int argc, char *argv[])

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */

  	register mtyp_t read_rc;   /* read_message() return code (msg type)   */

  
  	char heartbeat_cycle[CYCLE_TEXT_SIZE];
        	                  /* Stress Lab "heartbeat" cyc time (sec)   */
  	char *heartbeat_dev;       /* ptr to "heartbeat" device string        */

  	off_t err_thres;           /* max error log file size threshold       */
  	off_t err_save_thres;      /* max save err log file size threshold    */
  	off_t msg_thres;           /* max msg log file size threshold         */
  	off_t msg_save_thres;      /* max save msg log file size threshold    */

  	static struct htx_msg_xbuf message_buffer;
                                   /* IPC message buffer                      */
  	static struct htx_msg_xbuf *p_message_buffer = &message_buffer;
                                   /* pointer to IPC message buffer           */

  	tbool err_wrap_flag;       /* error log wrap flag                     */
  	tbool msg_wrap_flag;       /* message log wrap flag                   */
  	tbool msg_archive_flag;    /* message archive flag                    */
  	tbool auto_start_flag;     /* system auto-start flag                  */

  /*
   ***  Beginning of Executable Code  *****************************************
   */

	set_htx_home_log_path();
	sprintf(CKPT_FILE, "%s/hxsmsg_ckpt", global_htx_log_dir);
	sprintf(ERR_LOG, "%s/htxerr", global_htx_log_dir);
	sprintf(ERR_SAVE_LOG, "%s/htxerr_save", global_htx_log_dir);
	sprintf(MSG_LOG, "%s/htxmsg", global_htx_log_dir);
	sprintf(MSG_SAVE_LOG, "%s/htxmsg_save", global_htx_log_dir);

  	if (set_signal_hdl(SIGTERM, SIGTERM_hdl) != GOOD)
	{
    		main_exit_code |= BAD_SET_SIGTERM;
	}
 
  /* 
   * Convert and/or copy argv[] parameters to meaningful variables.
   */
  	else if (do_argv(argv, argc, &err_thres, &msg_thres, &err_save_thres,
		   &msg_save_thres, &err_wrap_flag, &msg_wrap_flag, &msg_archive_flag, 
		   &auto_start_flag, &heartbeat_dev, heartbeat_cycle,
		   (size_t) (CYCLE_TEXT_SIZE -1))
		   != GOOD)
	{
    		main_exit_code |= BAD_DO_ARGV;
	}

  	else if (open_logs(err_thres, msg_thres, err_save_thres, msg_save_thres,
		     auto_start_flag)
		     != GOOD)
	{
    		main_exit_code |= BAD_OPEN_LOGS;
	}

  	else if (open_ipc() != GOOD)
	{
    		main_exit_code |= BAD_OPEN_IPC;
	}
  	/*else if(start_display_device() != GOOD )
        {
                main_exit_code |= BAD_START_DEVICE;
        }*/

	else if (htx_strcmp(heartbeat_dev, "/dev/null") != 0)
    	{
#ifndef __OS400__
      		if((hxsstress_PID = start_heart(heartbeat_dev, heartbeat_cycle))
	  	== -1)
		{
			main_exit_code |= BAD_START_HEART;
		}
      		else if (set_signal_hdl(SIGCHLD, SIGCHLD_hdl) != GOOD)
		{
			main_exit_code |= BAD_SET_SIGCHLD;
		}
#endif
    	}

  	if (main_exit_code == GOOD)  /* everything still OK?                  */
    	{
      /*
       * Read messages and copy them as specified until the last
       * message.
       */
      		read_rc = 0;
      		while (read_rc != HTX_SYS_FINAL_MSG)
		{
	  		if ((read_rc = read_message(p_message_buffer)) != -1)
	    		{
	      			(void) copy_message(p_message_buffer, err_thres, msg_thres,
				err_save_thres, msg_save_thres,
				err_wrap_flag, msg_wrap_flag, msg_archive_flag);
	    		} /* endif */
		} /* end_while */
    	} /* endif */


  /*
   * Release resources and exit program.
   */
  	if (clean_up() != GOOD)
	{
    		main_exit_code |= BAD_CLEAN_UP;
	}

  	exit(main_exit_code);           /* terminate program                */

} /* main() */



/* set HTX home and log directory */
void set_htx_home_log_path(void)
{
	char *temp_env_val_ptr = NULL;
	char *default_log_dir = "/tmp";

	
	temp_env_val_ptr = getenv("HTX_HOME_DIR");
	if(temp_env_val_ptr == NULL) {
		printf("HTX_HOME_DIR is not set, exiting...\n");
		exit(11);
	} else {
		if(strlen(temp_env_val_ptr) == 0) {
			printf("HTX_HOME_DIR is not set, exiting...\n");
			exit(12);
		} else {
			strcpy(global_htx_home_dir, temp_env_val_ptr);
		}
	}
	
	temp_env_val_ptr = getenv("HTX_LOG_DIR");
	if(temp_env_val_ptr == NULL) {
		strcpy(global_htx_log_dir, default_log_dir);
	} else {
		if(strlen(temp_env_val_ptr) == 0) {
			strcpy(global_htx_log_dir, default_log_dir);
		} else {
			strcpy(global_htx_log_dir, temp_env_val_ptr);
		}
	}
}
