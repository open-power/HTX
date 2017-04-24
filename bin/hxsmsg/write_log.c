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

/* @(#)29	1.5.4.1  src/htx/usr/lpp/htx/bin/hxsmsg/write_log.c, htx_msg, htxubuntu 8/3/12 08:00:32 */

/*
 *   FUNCTIONS: write_log
 */


#include "hxsmsg.h"

#include <unistd.h>
#include <sys/types.h>

#ifdef __HTX_LINUX__
#include <sys/stat.h>
#include <sys/types.h>
#endif /* __HTX_LINUX__ */

/*
 * write_log() error code #defines
 */
#define BAD_LOG_WRITE1 0x0001
#define BAD_LOG_WRITE2 0x0002
#define BAD_LOG_FSYNC 0x0004
#define BAD_WRAP_CHECK 0x0008
#define BAD_UPDATE_CKPT 0x0010

/*
 * NAME: write_log()
 *                                                                    
 * FUNCTION: Writes message to specified log file.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the copy_message() function of the "hxsmsg"
 *      program.  The message handler program, "hxsmsg", is always a child
 *      process of the HTX supervisor program, "hxssup".
 *
 * NOTES: 
 *
 *      operation:
 *      ---------
 *
 *      if write() to log is BAD
 *        set error code
 *      else
 *        check number of bytes written
 *        fsync() the log file
 *        check for wrap condition
 *        if checkpoint file is open
 *          update checkpoint file
 *
 *      return
 * 
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      ------------------------------------------------------------------
 *       0 (0x0000) -- Normal exit.
 *       1 (0x0001) -- write() on the log file returned a -1.
 *       2 (0x0002) -- write() on the log file transferred wrong num of bytes.
 *       4 (0x0004) -- Error on fsync() of the log file.
 *       8 (0x0008) -- Error from wrap_check() of the log file.
 *      16 (0x0010) -- Error from update_ckpt() call.
 *
 *      NOTE: Each of the error codes listed above takes up a single bit
 *            position.  Thus, multiple error conditions are indicated
 *            by "OR'ing" the indicated error codes into a composite
 *            return code.
 *
 *            For example, if the fsync() call for the log file failed
 *            (error code 0x0004) and the update_ckpt() call failed (error
 *            code 0x0010), the return code would be 0x0004 | 0x0010 =
 *            0x0014 (decimal 20).
 *
 *
 */  

short write_log(int *fileid_ptr, char *filename, char *message,
		size_t message_len, off_t threshold, tbool wrap_flag,
		tbool signal_on_thres_flag, off_t *ckpt_offset_ptr,
		time_t *ckpt_mod_time_ptr, tbool msg_archive_flag)
     /*
      * fileid_ptr -- pointer to the file descriptor of log file
      * filename -- name of the file
      * message -- message buffer
      * message_len -- length of buffer
      * threshold -- max size/wrap threshold value
      * wrap_flag -- flag which specifies whether to wrap or not if the
      *              threshold is met
      * signal_on_thres_flag -- flag which specifies whether to send a signal
      *              to the parent process if the threshold is met
      * ckpt_offset_ptr -- pointer to ckpt structure file offset save area
      * ckpt_mod_time_ptr -- pointer to ckpt structure file modification time
      *              save area
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char error_msg[128];       /* error message string                    */

  	extern char *program_name; /* this program's name (argv[0])           */

  	extern int ckpt_file;      /* Checkpoint File File Descriptor         */

 	int bytes_written;         /* number of bytes transferred with write()*/
  	int errno_save;            /* errno save area                         */

  	off_t file_offset;         /* offset of the log file                  */

  	short exit_code;           /* exit program return code                */


  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	exit_code = GOOD;

  	errno = 0;
  	if ((bytes_written = write(*fileid_ptr, message,
			     (unsigned int) message_len)) 
      	== -1)
   	{
      		errno_save = errno;

      		(void) sprintf(error_msg,
                "\n%s -- error writing to %s.\nerrno: %d (%s).\n",
		program_name,
                filename,
                errno_save,
		strerror(errno_save));

      		(void) fprintf(stderr, "%s", error_msg);
      		(void) fflush(stderr);
      		exit_code = BAD_LOG_WRITE1;
    	}
  	else
    	{
      		if (bytes_written != message_len)
		{
	  		(void) sprintf(error_msg,
                        "\n%s -- error writing to %s.\n\
			Only %d bytes out of a %d byte buffer were written to disk.\n",
                        program_name,
                        filename,
			bytes_written,
                        message_len);

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code |= BAD_LOG_WRITE2;
		} /* endif */

      		errno = 0;
      		if (fsync(*fileid_ptr) != GOOD)
		{
	  		errno_save = errno;

	  		(void) sprintf(error_msg,
			"\n%s -- error on fsync() to %s.\nerrno: %d (%s).\n",
			program_name,
			filename,
			errno_save,
			strerror(errno_save));

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code |= BAD_LOG_FSYNC;
		} /* endif -- fsync() */
 

      /*
       * check for log wrap condition 
       */
      		if (check_wrap(fileid_ptr, filename, bytes_written, threshold, wrap_flag,signal_on_thres_flag, *ckpt_offset_ptr, &file_offset, msg_archive_flag) != GOOD)
		{

			exit_code |= BAD_WRAP_CHECK;
		}


      /*
       * update ckpt structure and save to disk
       */
      		if (ckpt_file != -1)
		{
			if (update_ckpt(*fileid_ptr, filename, ckpt_offset_ptr, file_offset,ckpt_mod_time_ptr) != GOOD)
			{

	  			exit_code |= BAD_UPDATE_CKPT;
			}
		}

    	} /* endif -- write() */

  	return(exit_code);

} /* write_log() */
