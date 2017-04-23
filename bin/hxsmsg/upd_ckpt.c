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

/* @(#)28	1.6  src/htx/usr/lpp/htx/bin/hxsmsg/upd_ckpt.c, htx_msg, htxubuntu 5/24/04 18:20:22 */

/*
 *   FUNCTIONS: update_ckpt
 */


#include "hxsmsg.h"

#include <unistd.h>

#if defined(__HTX_LINUX__) || defined(__OS400__)
#include <sys/stat.h>
#include <sys/types.h>
#endif /* __HTX_LINUX__ */

/*
 * update_ckpt() error code #defines
 */
#define BAD_LOG_FSTAT 0x0001
#define BAD_LOG_STAT 0x0002
#define BAD_CKPT_LSEEK 0x0004
#define BAD_CKPT_WRITE1 0x0008
#define BAD_CKPT_WRITE2 0x0010
#define BAD_CKPT_FSYNC 0x0020

/*
 * NAME: update_ckpt()
 *                                                                    
 * FUNCTION: Writes message to specified log file.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the write_log() function of the "hxsmsg"
 *      program.  The message handler program, "hxsmsg", is always a child
 *      process of the HTX supervisor program, "hxssup".
 *
 * NOTES: 
 *
 *      operation:
 *      ---------
 *
 *      if log file open
 *          use fstat() to get file mod time
 *      else
 *          use stat() to get file mod time
 *
 *      calculate the ckpt check sum
 *      reset the ckpt file's file offset pointer to zero
 *      write the new ckpt data structure to the checkpoint file
 *      fsync the checkpoint file to disk
 *
 *      return
 * 
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      ------------------------------------------------------------------
 *       0 (0x0000) -- Normal exit.
 *       1 (0x0001) -- Error on fstat() of the log file.
 *       2 (0x0002) -- Error on stat() of the log file.
 *       4 (0x0004) -- Error on lseek() SEEK_SET to 0 of the ckpt file.
 *       8 (0x0008) -- Error (-1) on write of the ckpt file (errno set).
 *      16 (0x0010) -- Incorrect number of bytes transferred with the
 *                     write() on the ckpt file.
 *      32 (0x0020) -- Error on fsync() of the ckpt file.
 *
 *      NOTE: Each of the error codes listed above take up a single bit
 *            position.  Thus, multiple error conditions are indicated
 *            by "OR'ing" the indicated error codes into a composite
 *            return code.
 *
 *            For example, if the "Incorrect number of bytes on write()"
 *            error (code 0x0010) occurred and the fsync() call failed
 *            (error code 0x0020) occurred, the final return code would
 *            be 0x0010 | 0x0020 = 0x0030 (decimal 48).
 *
 *
 */  

short update_ckpt(int fileid, char *filename, off_t *ckpt_offset_ptr,
		  off_t file_offset, time_t *ckpt_mod_time_ptr)
     /*
      * fileid -- file descriptor for log file
      * filename -- name of the log file
      * ckpt_offset_ptr -- pointer to ckpt structure file offset save area
      * file_offset -- value of the log file's current offset
      * ckpt_mod_time_ptr -- pointer to ckpt structure file modification
      *             time save area
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char error_msg[128];       /* error message string                    */

  	extern ckpt checkpt;       /* Checkpoint file structure               */

  	extern char *program_name; /* this program's name (argv[0])           */

  	extern int ckpt_file;      /* Checkpoint File File Descriptor         */

  	int bytes_written;         /* number of bytes transferred with write()*/
  	int errno_save;            /* errno save area                         */

  	short exit_code;           /* exit program return code                */

  	struct stat stat_buffer;   
			      /* file status buffer for stat(), fstat() calls */

  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	exit_code = GOOD;
  
  	*ckpt_offset_ptr = file_offset;  
				   /* save new offset value in ckpt struct   */

  	if (fileid >= 0)      /* log file still open?                         */
    	{
      /*
       * If here, the log file is still open; so, let's use the quicker
       * fstat() system call to get the latest modification time for the
       * log file.
       */
      		errno = 0;
      		if (fstat(fileid, &stat_buffer) != GOOD)
		{
	  		errno_save = errno;

	  		(void) sprintf(error_msg, 
			"\n%s -- error on fstat() of %s.\nerrno: %d (%s).\n",
			program_name, 
			filename,
			errno_save,
			strerror(errno_save));

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code |= BAD_LOG_FSTAT;
		}
      		else
		{
			*ckpt_mod_time_ptr = stat_buffer.st_mtime;
		}
    	}
  	else
    	{
      /*
       * If here, the log file has been closed; so, we have to use the 
       * slower stat() system call to get the last modification time for
       * the log file.
       */
      		errno = 0;
      		if (stat(filename, &stat_buffer) != GOOD)
		{
	  		errno_save = errno;

	  		(void) sprintf(error_msg,
			"\n%s -- error on stat() of %s.\nerrno: %d (%s).\n",
			program_name,
			filename,
			errno_save,
			strerror(errno_save));

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code |= BAD_LOG_STAT;
		}
      		else
		*ckpt_mod_time_ptr = stat_buffer.st_mtime;
    	} /* endif */
  

  /*
   * Calculate the new checksum value for the modified ckpt structure.
   */
  	checkpt.checksum = sum((char *) &checkpt,
			 (sizeof(ckpt) - sizeof(checkpt.checksum)));

  
  /*
   * Reset the checkpoint file's file pointer to zero.
   */
  	errno = 0;
  	if (lseek(ckpt_file, (off_t) 0, SEEK_SET) != 0)
    	{
      		errno_save = errno;

      		(void) sprintf(error_msg,
		"\n%s -- error on lseek() of %s.\nerrno: %d (%s).\n",
		program_name,
		CKPT_FILE,
		errno_save,
		strerror(errno_save));

      		(void) fprintf(stderr, "%s", error_msg);
      		(void) fflush(stderr);
      		exit_code |= BAD_CKPT_LSEEK;
    	}
  	else 
    	{
      /*
       * If here, then the lseek() successfully reset the checkpoint file's
       * file pointer to zero and it's OK to write out the updated ckpt
       * data structure.
       */
      		errno = 0;
      		if ((bytes_written = write(ckpt_file, (char *) &checkpt, sizeof(ckpt))) == -1)
		{
	  		errno_save = errno;

	  		(void) sprintf(error_msg, 
			"\n%s -- error on write to %s.\nerrno: %d (%s).\n",
			program_name,
			CKPT_FILE,
			errno_save,
			strerror(errno_save));

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code |= BAD_CKPT_WRITE1;
		}
      		else 
		{	
	  		if (bytes_written != sizeof(ckpt))
	    		{
	      			(void) sprintf(error_msg,
			     	"\n%s -- error writing to %s.\n\
				Only %d bytes out of a %d byte buffer were written to disk.\n",
			     	program_name,
			     	CKPT_FILE,
			    	bytes_written,
			     	sizeof(ckpt));

	      			(void) fprintf(stderr, "%s", error_msg);
	      			(void) fflush(stderr);
	     			exit_code |= BAD_CKPT_WRITE2;
	    		} /* endif */

	  /*
	   * If here, the ckeckpoint file has been modified, so it's
           * time to fsync() it out to disk.
           */
	  		errno = 0;
	  		if (fsync(ckpt_file) != GOOD)
	    		{
	      			errno_save = errno;

	      			(void) sprintf(error_msg,
			     	"\n%s -- error on fsync() to %s.\n\
				errno: %d (%s).\n",
			     	program_name, 
			     	CKPT_FILE,
			     	errno_save,
			     	strerror(errno_save));

	      			(void) fprintf(stderr, "%s", error_msg);
	     	 		(void) fflush(stderr);
	      			exit_code |= BAD_CKPT_FSYNC;
	    		} /* endif -- fsync() */

		} /* endif -- write() */
      
    	} /* endif -- lseek() */

  	return(exit_code);

} /* update_ckpt() */
