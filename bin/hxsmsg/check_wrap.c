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

/* @(#)08	1.6.4.1  src/htx/usr/lpp/htx/bin/hxsmsg/check_wrap.c, htx_msg, htxubuntu 8/3/12 07:53:39 */

/*
 *   FUNCTIONS: check_wrap
 */


#include <sys/types.h>
#include <unistd.h>

#include "hxsmsg.h"

/*
 * check_wrap() error code #defines
 */
#ifdef __HTX_LINUX__
#define    UBSIZE      512              /* file size is returned in blocks in linux */
#endif

#define BAD_LOG_LSEEK1 0x0001
#define BAD_LOG_CLOSE 0x0002
#define BAD_SEND_SIGNAL 0x0004
#define BAD_LOG_FTRUNCATE 0x0008
#define BAD_WRAP_FSYNC 0x0010
#define BAD_LOG_LSEEK2 0x0020

int system_call;

/*
 * NAME: check_wrap()
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
 *      get current file pointer value
 *
 *      if log threshold has been met
 *      l
 *      l   if file should not be wrapped
 *      l   l
 *      l   l   if signal to ppid should not be sent
 *      l   l   l
 *      l   l   l   close the log file
 *      l   l   l
 *      l   l   else (signal to ppid should be sent)
 *      l   l   l
 *      l   l   l   send a signal to the ppid
 *      l   l   ---
 *      l   else (file should be wrapped)
 *      l   l
 *      l   l   truncate end of log file
 *      l   l   fsync() file to disk
 *      l   l   reset file pointer to beginning of file (offset 0)  
 *      l   ---
 *      ---
 *
 *      return
 * 
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      ------------------------------------------------------------------
 *       0 (0x0000) -- Normal exit.
 *       1 (0x0001) -- Error on lseek() SEEK_CUR to get file pointer offset.
 *       2 (0x0002) -- Error on close_file() of log file.
 *       4 (0x0004) -- Error with send_signal() function.
 *       8 (0x0008) -- Error on ftruncate() of log file.
 *      16 (0x0010) -- Error on fsync() of log file.
 *      32 (0x0020) -- Error on lseek() SEEK_SET to set file ptr to zero.
 *
 *      NOTE: Each of the error codes listed above take up a single bit
 *            position.  Thus, multiple error conditions are indicated
 *            by "OR'ing" the indicated error codes into a composite
 *            return code.
 *
 *            For example, if the lseek() SEEK_CUR call for the log file
 *            failed (error code 0x0001) and the fsync() call failed (error
 *            code 0x0010), the return code would be 0x0001 | 0x0010 =
 *            0x0011 (decimal 17).
 *
 *
 */  

short check_wrap(int *fileid_ptr, char *filename, int bytes_last_write,
		 off_t threshold, tbool wrap_flag, tbool signal_on_thres_flag,
		 off_t ckpt_offset, off_t *file_offset_ptr, tbool msg_archive_flag)
     /*
      * fileid_ptr -- pointer to the file descriptor of log file
      * filename -- name of the file
      * bytes_last_write -- number of bytes transferred with the last
      *             call to write()
      * threshold -- max size/wrap threshold value
      * wrap_flag -- flag which specifies whether to wrap or not if the
      *              threshold is met
      * signal_on_thres_flag -- flag which specifies whether to send a signal
      *              to the parent process if the threshold is met
      * ckpt_offset -- value of last offset saved in ckpt structure
      * file_offset_ptr -- ptr to value of the current file offset
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char error_msg[128];       /* error message string                    */
  
  	extern char *program_name; /* the name of this program                */

  	int errno_save;            /* errno save area                         */

  	short exit_code;           /* exit program return code                */

	static int archive_sequence_count = 0;

	char command_str[100];


  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	exit_code = GOOD;
  
  /*
   * Get the current file offset value
   */
  	errno = 0;
  	if((*file_offset_ptr = lseek(*fileid_ptr, (off_t) 0, SEEK_CUR)) == -1)
    	{
      		errno_save = errno;

      		(void) sprintf(error_msg, 
		"\n%s -- error on lseek() of %s.\nerrno: %d (%s).\n",
		program_name,
		filename,
		errno_save,
		strerror(errno_save));

      		(void) fprintf(stderr, "%s", error_msg);
      		(void) fflush(stderr);
      		exit_code |= BAD_LOG_LSEEK1;
      
      		if (ckpt_offset <= 0)
		{
			*file_offset_ptr = bytes_last_write;
		}
      		else
		{
			*file_offset_ptr = ckpt_offset + bytes_last_write;
		}
    	} /* endif -- lseek() */

  
  /*
   * Check the offset to see if it meets or exceeds the threshold.
   * If it does, DO THE RIGHT THING!
   */
#ifdef __HTX_LINUX__
  	if ((*file_offset_ptr)/UBSIZE >= threshold) { /* threshold met?                 */
#else
  	if ((*file_offset_ptr)>= threshold) {  /* threshold met?                 */
#endif
      		if (wrap_flag == FALSE) {    /* do not wrap file?              */
	  		if (signal_on_thres_flag == FALSE) { 
					    /* do not send a signal?         */
              		/*
               		* Close the file.
               		*/
	      			if (close_file(fileid_ptr, filename) != GOOD)
				{
		  			exit_code |= BAD_LOG_CLOSE;
				}
	      			*fileid_ptr = -1;  
			                   /* fileid no longer valid         */
	    		}

	  		else               /* send a signal                  */
	    		{
	      			if (send_signal(getppid(), SIGUSR1) != GOOD)
				{
					exit_code |= BAD_SEND_SIGNAL;
				}
	    		} /* endif */
		}
      		else                       /* wrap file                      */
		{
	  	/*
	   	 * Truncate any leftover garbage at the end of file
          	 */
	  		errno = 0;
	  		if (ftruncate(*fileid_ptr, *file_offset_ptr) != GOOD)
	    		{	
	      			errno_save = errno;

	      			(void) sprintf(error_msg, 
			     	"\n%s -- error on ftruncate() of %s.\n\
errno: %d (%s).\n",
			     	program_name,
			     	filename,
			     	errno_save,
			     	strerror(errno_save));

	      			(void) fprintf(stderr, "%s", error_msg);
	      			(void) fflush(stderr);
	      			exit_code |= BAD_LOG_FTRUNCATE;
	    		}
	  		else 
	    		{
              		/* 
               		 * If here,file has been modified by ftruncate(),fsync()
		         * so that it is really written to the disk.
               		 */
	      			errno = 0;
	      			if (fsync(*fileid_ptr) != GOOD)
				{
		  			errno_save = errno;

		  			(void) sprintf(error_msg, 
				 	"\n%s -- error on fsync() of %s.\n\
errno: %d (%s).\n",
					program_name,
					filename,
				 	errno_save,
				 	strerror(errno_save));

		  			(void) fprintf(stderr, "%s", error_msg);
		  			(void) fflush(stderr);
		  			exit_code |= BAD_WRAP_FSYNC;
				} /* endif -- fsync() */
	    		} /* endif -- ftruncate() */

	  /*
	   * Reset file pointer to beginning of the log file.
           */
			if(msg_archive_flag == TRUE)
			{ /* copy the message file to next archive file */
				sprintf(command_str, "cp %s/htxmsg %s/htxmsg.%d", global_htx_log_dir, global_htx_log_dir, archive_sequence_count);
				system_call = TRUE;
				system(command_str);
				system_call = FALSE;
				archive_sequence_count++;
			}
	  		errno = 0;
	  		if (lseek(*fileid_ptr, (off_t) 0, SEEK_SET) != 0)
	    		{
	      			errno_save = errno;

	      			(void) sprintf(error_msg,
			     	"\n%s -- error on lseek() of%s.\n\
errno: %d (%s).\n",
			     	program_name,
			    	filename,
			     	errno_save,
			     	strerror(errno_save));

	      			(void) fprintf(stderr, "%s", error_msg);
	      			(void) fflush(stderr);
	      			exit_code |= BAD_LOG_LSEEK2;
	    		}
	  		else
	    		{
	      			*file_offset_ptr = 0;
	    		} /* endif -- lseek() */

		} /* endif */

    	} /* endif */

  	return(exit_code);

} /* check_wrap() */
