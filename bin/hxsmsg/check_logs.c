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

/* @(#)07	1.6  src/htx/usr/lpp/htx/bin/hxsmsg/check_logs.c, htx_msg, htxubuntu 5/24/04 18:13:25 */

/*
 *   FUNCTIONS: check_ckpt
 *		check_logs
 *		check_time
 *   ORIGINS: 27
 */


#include "hxsmsg.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#if !defined(__HTX_LINUX__) && !defined(__OS400__)
#include <sys/mode.h>
#else
#endif /* __HTX_LINUX__ */

/*
 * Error code #define's for check_logs() and check_ckpt()
 */
#define BAD_CKPT_OPEN 0x0001
#define BAD_CKPT_READ 0x0002
#define BAD_CKPT_WRITE1 0x0004
#define BAD_CKPT_WRITE2 0x0008
#define BAD_CKPT_TRUNC 0x0010
#define BAD_ERR_RM 0x0020
#define BAD_MSG_RM 0x0040
#define BAD_ERR_SAVE_RM 0x0080
#define BAD_MSG_SAVE_RM 0x0100
#define BAD_ERR_TIME 0x0200
#define BAD_ERR_SAVE_TIME 0x0400
#define BAD_MSG_TIME 0x0800
#define BAD_MSG_SAVE_TIME 0x1000

/*
 * Error code #define's for check_time()
 */
#define BAD_STAT 1
#define BAD_RM 2


extern ckpt checkpt;       /* log files checkpoint structure          */

extern int ckpt_file;      /* checkpoint file file descriptor         */

/*
 * NAME: check_logs()
 *                                                                    
 * FUNCTION: Checks the pre-existing logs in an auto-start environment.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the open_logs() function of the "hxsmsg"
 *      program which is a child process of the HTX supervisor program,
 *      "hxssup". 
 *
 * NOTES: 
 *
 *      *** This function is only called when the program is operating ***
 *      *** in auto-start mode.                                        ***
 *
 *
 *      operation:
 *      ---------
 *
 *      open the check-point file (creating it if necessary, i.e.,
 *                                 first time in auto-start mode)
 *      read the check-point file (zero bytes read if first time
 *                                 in auto-start mode)
 *      check the just read check-point file
 *      check the log files' modification times
 *
 *      return
 * 
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      --------------------------------------------------------------------
 *         0 (0x0000) -- Normal exit.  No errors.
 *         1 (0x0001) -- Unable to open checkpoint file
 *         2 (0x0002) -- Unable to read checkpoint file
 *         4 (0x0004) -- Error writing to check-point file
 *         8 (0x0008) -- Incorrect number of bytes transferred in write() to
 *                       check-point file
 *        16 (0x0010) -- ftruncate() failed on checkpoint file
 *        32 (0x0020) -- Unable to remove error log file
 *        64 (0x0040) -- Unable to remove message log file
 *       128 (0x0080) -- Unable to remove error log save file
 *       256 (0x0100) -- Unable to remove message log save file
 *       512 (0x0200) -- check_time() failure on error log file
 *      1024 (0x0400) -- check_time() failure on error log save file
 *      2048 (0x0800) -- check_time() failure on message log file
 *      4096 (0x1000) -- check_time() failure on message log save file
 *
 *      NOTE: Each of the error codes listed above take up a single bit
 *            position.  Thus, multiple error conditions are indicated
 *            by "OR'ing" the indicated error codes into a composite
 *            return code.
 *
 *            For example, if the check_time() call for the error log file
 *            failed (error code 0x0200) and the check_time() call for the
 *            message log file failed (error code 0x0800), the return code
 *            would be 0x0200 | 0x0800 = 0x0A00 (decimal 2560).
 *
 *
 */  

short check_logs(off_t err_thres, off_t msg_thres, off_t err_save_thres,
		 off_t msg_save_thres)
     /*
      * err_thres -- max err log file size threshold
      * msg_thres -- max msg log file size threshold
      * err_save_thres -- max save err log file size threshold
      * msg_save_thres -- max save msg log file size threshold
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char error_msg[128];       /* string for error messages               */

  	extern char *program_name; /* this program's name (argv[0])           */

  	int errno_save;            /* errno save area                         */
  	int mode;                  /* file mode (permissions)                 */
  	int num_bytes;             /* number of bytes read from ckpt file     */
  	int oflag;                 /* open flag (type of open)                */

  	short exit_code;           /* function exit code                      */
  	short rc;                  /* generic return code                     */


  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	exit_code = GOOD;
  	oflag = O_RDWR | O_CREAT;   /* open read/wrt, create if necessary    */
  	mode = S_IRUSR | S_IWUSR;   /* set permission to -rw-------          */

  	errno = 0;
  	if ((ckpt_file = open_file(CKPT_FILE, oflag, mode)) == -1)
	{
      		exit_code = BAD_CKPT_OPEN;
	}
  	else if ((num_bytes = read(ckpt_file, (char *) &checkpt, sizeof(ckpt)))
 == -1) 
    	{
      		errno_save = errno;

      		(void) sprintf(error_msg, 
		"\n%s -- Error: Unable to read %s.\nerrno: %d (%s).\n",
		 program_name, 
		 MSG_LOG,
		 errno_save,
		 strerror(errno_save));

      		(void) fprintf(stderr, "%s", error_msg);
      		(void) fflush(stderr);
      		exit_code = BAD_CKPT_READ;
    	}
  	else if ((rc = check_ckpt(err_thres, msg_thres, err_save_thres,msg_save_thres, num_bytes))!= GOOD)
	{
    		exit_code = rc;
	}
  	else 
    	{
      		if (check_time(ERR_LOG, &checkpt.err_mod_time,&checkpt.err_offset)!= GOOD)
		{
			exit_code |= BAD_ERR_TIME;
		}
      		else if ((err_save_thres > 0) && (check_time(ERR_SAVE_LOG,
				   &checkpt.err_save_mod_time,
				   &checkpt.err_save_offset)
					!= GOOD) )
		{
			exit_code |= BAD_ERR_SAVE_TIME;
		}
      		else if (check_time(MSG_LOG, &checkpt.msg_mod_time,
			  &checkpt.msg_offset)
	       	!= GOOD)
		{
			exit_code |= BAD_MSG_TIME;
		}
      		else if ((msg_save_thres > 0) && (check_time(MSG_SAVE_LOG,
					   &checkpt.msg_save_mod_time,
					   &checkpt.msg_save_offset)
					!= GOOD) )
		{
			exit_code |= BAD_MSG_TIME;
		}
      		else if ((err_save_thres == 0) && (rm_file(ERR_SAVE_LOG) != GOOD))
		{
			exit_code |= BAD_ERR_SAVE_RM;
		}
      		else if ((msg_save_thres == 0) && (rm_file(MSG_SAVE_LOG) != GOOD))
		{
			exit_code |= BAD_MSG_SAVE_RM;
		}
    	} /* endif */
  
  	return(exit_code);
	
} /* check_logs() */





/*
 * NAME: check_ckpt()
 *                                                                    
 * FUNCTION: Checks checkpoint file structure.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the check_logs() function of the "hxsmsg"
 *      program which is a child process of the HTX supervisor program,
 *      "hxssup". 
 *
 * NOTES: 
 *
 *  The following conditions are tested:
 *
 *      (1) first time checkpoint file has been created
 *          (file length will be zero),
 *      (2) checkpoint file is the wrong size, and,
 *      (3) checkpoint file checksum is wrong,
 *
 *  If any of those conditions are true, then the checkpoint file
 *  is not usable for restarting the error and message logs where
 *  they left off at the end of the last run.  In this case, the 
 *  checkpoint file is initialized and all the log files are erased.
 *
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      --------------------------------------------------------------------
 *         0 (0x0000) -- Normal exit.  No errors.
 *         4 (0x0004) -- Error writing to check-point file
 *         8 (0x0008) -- Incorrect number of bytes transferred in write() to
 *                       check-point file
 *        16 (0x0010) -- ftruncate() failed on checkpoint file
 *        32 (0x0020) -- Unable to remove error log file
 *        64 (0x0040) -- Unable to remove message log file
 *       128 (0x0080) -- Unable to remove error log save file
 *       256 (0x0100) -- Unable to remove message log save file
 *
 *      NOTE: Each of the error codes listed above take up a single bit
 *            position.  Thus, multiple error conditions are indicated
 *            by "OR'ing" the indicated error codes into a composite
 *            return code.
 *
 *            For example, if the remove of the error log file failed
 *            (error code 0x0020) and the remove of the message log save
 *            file failed (error code 0x0100), the return code would be
 *            0x0020 | 0x0100 = 0x0120 (decimal 288).
 *
 *
 */  

short check_ckpt(off_t err_thres, off_t msg_thres, off_t err_save_thres,
		 off_t msg_save_thres, int num_bytes)
     /*
      * err_thres -- max err log file size threshold
      * msg_thres -- max msg log file size threshold
      * err_save_thres -- max save err log file size threshold
      * msg_save_thres -- max save msg log file size threshold
      * num_bytes -- number of bytes read from checkpoint file
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char error_msg[128];       /* string for error messages               */

  	extern char *program_name; /* this program's name (argv[0])           */

  	int errno_save;            /* errno save area                         */
  	int num_write_bytes;       /* number of bytes transferred with write()*/

  	short exit_code;           /* function exit code                      */


  /*
   ***  Beginning of Executable Code  *****************************************
   */
  exit_code = GOOD;

  	if ((num_bytes == 0) ||
      	(num_bytes != sizeof(ckpt)) ||
      	(checkpt.checksum != sum((char *) &checkpt,
			       (sizeof(ckpt) - sizeof(checkpt.checksum)))) ||
      	(checkpt.err_thres != err_thres) ||
      	(checkpt.err_save_thres != err_save_thres) ||
      	(checkpt.msg_thres != msg_thres) ||
      	(checkpt.msg_save_thres != msg_save_thres))
    	{
      /*
       * Set the ckpt threshold values to match this run's values.
       */
      		checkpt.err_thres = err_thres;
      		checkpt.err_save_thres = err_save_thres;
      		checkpt.msg_thres = msg_thres;
      		checkpt.msg_save_thres = msg_save_thres;
      /*
       * Set the error log file offsets to zero.
       */
      		checkpt.err_offset = 0;
      		checkpt.err_save_offset = 0;
      		checkpt.msg_offset = 0;
      		checkpt.msg_save_offset = 0;
      /*
       * Set the last modification times to zero.
       */
      		checkpt.err_mod_time = 0;
      		checkpt.err_save_mod_time = 0;
      		checkpt.msg_mod_time = 0;
      		checkpt.msg_save_mod_time = 0;
      
      		checkpt.checksum = sum((char *) &checkpt,
			     (sizeof(ckpt) - sizeof(checkpt.checksum)));
      
      /*
       * Write the new checksum file to disk and truncate any excess.
       */
      		errno = 0;
      		if ((num_write_bytes = write(ckpt_file, (char *) &checkpt, 
				   sizeof(ckpt)))
	  	== -1)
		{
	  		errno_save = errno;

	  		(void) sprintf(error_msg,
			"\n%s -- Error: Unable to write %s.\n\
			errno: %d (%s).\n",
			program_name,
			CKPT_FILE,
			errno_save,
			strerror(errno_save));

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code = BAD_CKPT_WRITE1;
		}
      		else if (num_write_bytes != sizeof(ckpt))
		{
	  		(void) sprintf(error_msg,
			"\n%s -- Error: Only %d out of %d bytes written to \
%s.\n",
			program_name,
			num_write_bytes,
			sizeof(ckpt), 
			CKPT_FILE);

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code = BAD_CKPT_WRITE2;
		}
      		else if (ftruncate(ckpt_file, (off_t) sizeof(ckpt)) != GOOD)
		{
	  		errno_save = errno;

	  		(void) sprintf(error_msg,
			"\n%s -- Error: Unable to truncate %s.\n\
errno: %d (%s).\n",
			program_name,
			CKPT_FILE,
			errno_save,
			strerror(errno_save));

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code = BAD_CKPT_TRUNC;
		}
      		else 
		{
	  		if (rm_file(ERR_LOG) != GOOD)
			{
	    			exit_code |= BAD_ERR_RM;
			}
	  		else if (rm_file(MSG_LOG) != GOOD)
			{
	    			exit_code |= BAD_MSG_RM;
			}
	  		else if (rm_file(ERR_SAVE_LOG) != GOOD)
			{
	    			exit_code |= BAD_ERR_SAVE_RM;
			}
	  		else if (rm_file(MSG_SAVE_LOG) != GOOD)
			{
	    			exit_code |= BAD_ERR_SAVE_RM;
			}
		} /* endif */
    	} /* endif */
  
  	return(exit_code);

} /* check_ckpt() */





/*
 * NAME: check_time()
 *                                                                    
 * FUNCTION: Checks the file modification time of the specified log.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by the check_logs() function of the "hxsmsg"
 *      program which is a child process of the HTX supervisor program,
 *      "hxssup". 
 *
 * NOTES: 
 *
 *  The file modification time of the specified file is obtained via the 
 *  stat() system call.
 *
 *  If this modification time is different from the passed expected mod time,
 *  the specified file is removed.
 * 
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      -------------
 *         0 -- Normal exit.  No errors.
 *         1 -- Unable to get modification time via stat()
 *         2 -- Unable to remove specified file
 *
 *
 */  

short check_time(char *filename, time_t *ckpt_mod_time, off_t *ckpt_offset)
     /*
      * filename -- name of the file
      * ckpt_mod_time -- ptr to file mod time saved in checkpoint file
      * ckpt_offset -- ptr to file offset saved in checkpoint file
      */

{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  	char error_msg[128];       /* string for error messages               */

  	extern char *program_name; /* this program's name (argv[0])           */

  	int errno_save;            /* errno save area                         */

  	short exit_code;           /* function exit code                      */

  	struct stat stat_buffer;   /* structure for stat() system call        */


  /*
   ***  Beginning of Executable Code  *****************************************
   */
  	exit_code = GOOD;

  	errno = 0;
  	if (stat(filename, &stat_buffer) != GOOD)
    	{
      		errno_save = errno;
      		if (errno_save == ENOENT)   /* file does not exist            */
		{
	  		*ckpt_mod_time = 0; /* clear mod time                */
	  		*ckpt_offset = 0;   /* clear offset pointer          */
		}
      		else              /* some kind of error with stat()          */
		{
	  		(void) sprintf(error_msg,
			"\n%s -- Error: Unable to get modification  time \
(stat()) of %s.\nerrno: %d (%s).\n",
                        program_name,
                        filename,
			errno_save,
			strerror(errno_save));

	  		(void) fprintf(stderr, "%s", error_msg);
	  		(void) fflush(stderr);
	  		exit_code = BAD_STAT;
		} /* endif */
    	} /* endif */

  	else if (stat_buffer.st_mtime != *ckpt_mod_time)
    	{
      		*ckpt_mod_time = 0;   /* clear mod time                      */
	        *ckpt_offset = 0;     /* clear offset pointer                */
      		if (rm_file(filename) != GOOD)
		exit_code = BAD_RM;
    	} /* endif */
  
 	 return(exit_code);

} /* check_time() */
