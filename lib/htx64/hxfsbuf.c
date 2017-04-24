/* "@(#)75  1.2  src/htx/usr/lpp/htx/lib/htx64/hxfsbuf.c, htx_libhtxmp, htxubuntu 10/8/10 04:38:42"; */
/*
 * COMPONENT_NAME: (HTXLIB) HTX Libraries
 *
 * FUNCTIONS: hxfcbuf(), hxfsbuf()
 *
 * ORIGINS: 27
 *
 * IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 * combined with the aggregated modules for this product)
 * OBJECT CODE ONLY SOURCE MATERIALS
 * (C) COPYRIGHT International Business Machines Corp. 1988, 1990, 1991
 * All Rights Reserved
 *
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#include "htx_local.h"
#include "hxihtx64.h"
#ifndef __HTX_LINUX__
#include <sys/mode.h>
#else
#include <sys/stat.h>
#endif


#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#define DUMP_PATH "/tmp/"

#define MAX_MISCOMPARES 10
#define MAX_MSG_DUMP 20


/*---------------------------------------------------------------------
*  We need to go thru this function with mutexes because hxfsbuf write 
*  into files and we do not want two threads write into the same file. 
*---------------------------------------------------------------------*/

/*
 * NAME: hxfsbuf_tefficient()
 *
 * FUNCTION: Saves a buffer to a specified file on disk.
 *
 * EXECUTION ENVIRONMENT:
 *
 *      This function is called by the hxfsbuf_tsafe() function on the libhtxmp.a
 *      library.  Typically it will be called under an HTX Hardware
 *      Exerciser (HE) process.
 *
 *      This function is exactly same as hxfsbuf() except that this one calls
 *      hxfmsg instead of hxfmsg. This was done to make hxfupdate thread safe.
 *
 *      Return Codes:
 *      ----------------------------------------------------------------------
 *                   0 -- Normal exit; buffers compare OK.
 *       exit_code > 0 -- The errno value at the time of the error.
 *
 *
 */

int hxfsbuf_tefficient(char *buf, size_t len, char *fname, struct htx_data *ps)
     /*
      * buf -- pointer to the buffer to be copied to disk
      * int -- the length of the buffer
      * fname -- the name of the file the buffer is to be saved to
      * ps -- pointer to the HE's htx_data structure
      */
{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  char  err_msg[MAX_TEXT_MSG];  /* error message string                      */

  int   exit_code;           /* exit code                                    */
  int   fileid;              /* file id                                      */
  int   mode;                /* file mode                                    */
  int   num_bytes;           /* write() return code                          */
  int   oflag;               /* open() flag                                  */


  /*
   ***  Beginning of Executable Code  *****************************************
   */
  exit_code = GOOD;          /* set good exit code                           */
  errno = 0;                 /* clear errno                                  */

  oflag = O_CREAT | O_WRONLY | O_TRUNC;
  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
                             /* set permission to -rw-r--r--                 */

  if ((fileid = open(fname, oflag, mode)) == -1)
    {
      exit_code = errno;
      (void) sprintf(err_msg, "Error opening %s.", fname);
      (void) hxfmsg(ps, exit_code, -10, err_msg);
    }
  else                       /* open() OK                                    */
    {
      if ((num_bytes = write(fileid, buf, (int) len)) == -1)
        {
          exit_code = errno;
          (void) sprintf(err_msg, "Error writing to %s.", fname);
          (void) hxfmsg(ps, exit_code, 1, err_msg);
        }
      else                       /* write() OK                               */
        {
          if (num_bytes != (int) len)
            {
              (void) sprintf(err_msg, "Error writing to %s.\nOnly %d of %lu \
bytes successfully transfered on write() system call.",
                             fname, num_bytes, len);
              (void) hxfmsg(ps, exit_code, -10, err_msg);
            } /* endif */

          if (fsync(fileid) != GOOD)
            {
              exit_code = errno;
              (void) sprintf(err_msg, "Error on fsync() to %s.", fname);
              (void) hxfmsg(ps, exit_code, -10, err_msg);
            } /* endif */

        } /* endif */

      if (close(fileid) != GOOD)
        {
          exit_code = errno;
          (void) sprintf(err_msg, "Error on close() of %s.", fname);
          (void) hxfmsg(ps, exit_code, -10, err_msg);
        } /* endif */

    } /* endif */

  return(exit_code);

} /* hxfsbuf_tefficient() */
