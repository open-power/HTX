/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

/*static char sccsid[] = "@(#)74	1.1  src/htx/usr/lpp/htx/lib/htxmp64/hxfpatmp_new.c, htx_libhtxmp, htxubuntu 9/22/10 07:32:53";*/

#include <pthread.h>

#include "htx_local.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <memory.h>

/*
 * NAME: hxfpat_tefficient()
 *                                                                    
 * FUNCTION: Opens and reads the specified pattern file into the specified
 *           buffer.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *	This function is called by any Hardware Exerciser (HE) program and
 *      is included as part of the libhtx.a library.
 *
 * NOTES: 
 *
 *      operation:
 *      ---------
 *      set good return code
 *
 *      if (unable to open pattern file)
 *
 *        set return code to 1
 *
 *      else
 *        
 *        clear pattern buffer
 *
 *        if (unable to read pattern file)
 *
 *          set return code to 2
 *
 *        else
 *
 *          copy read pattern into pattern buffer until pattern
 *          buffer is full
 *
 *        endif
 *
 *        if (error on close of pattern file)
 *       
 *          set return code to 3
 *
 *      endif
 *
 *      return(return code)
 *
 *               
 * RETURNS: 
 *
 *      Return Codes:
 *      ----------------------------------------------------------------------
 *      0 -- Normal return; no errors.
 *      1 -- Unable to open pattern file.
 *      2 -- Unable to read pattern file.
 *      3 -- Error closing pattern file.
 *
 *
 */  


int hxfpat_tefficient(char *filename, char *pattern_buf, int num_chars)
     
{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  int chars_read;             /* number of characters from read() system call*/
  int errno_save;             /* save area for errno                         */
  int fildes;                 /* file descriptor of pattern file             */
  int mode_flag;              /* mode flag for open() system call            */
  int return_code;            /* return code for function                */
  int pattern, modulus, num_words, *pattern_buf_intp;
  int i;
  size_t chars_copied;        /* the number of chars copied                  */
  size_t chars_left_to_copy;  /* the number of chars left to copy            */
  size_t chars_this_copy;     /* the number of chars to copy this time in    */
  char *s;
  int str_len, hex_flag;
  /*                             the loop                                    */
  
  /*
   ***  Beginning of Executable Code  *****************************************
   */
   
     
  return_code = 0;
  mode_flag = S_IWUSR | S_IWGRP | S_IWOTH;
  pattern=0;
  
  hex_flag=0;
  str_len=strlen(filename);
  if ((!strncmp("0x",&(filename[str_len-4]),2)) ||
      (!strncmp("0X",&(filename[str_len-4]),2))) {
        hex_flag=1;   
        errno = -1;
        /*pthread_Seterrno(-1);*/
        pattern=strtoul(&(filename[str_len-2]),NULL,16);
        errno_save = errno;
        }

  modulus=num_chars%4;
  num_words=num_chars/4;

#ifdef DEBUG
         (void) sprintf(error_message,
         "hxfpat_tefficient()-Pattern file(%s) errno_save=%d,hex_flag=%d,num_chars=%d,num_words=%d,modulus=%d\n",
             filename,errno_save,hex_flag,num_chars,num_words,modulus);
         (void) fprintf(stderr, "%s", error_message);
         (void) fflush(stderr);
#endif /* DEBUG 1*/

  if (hex_flag && !modulus) {
     if ((pattern || ((!pattern) && (errno_save<=0))) ) {
        s=(char *)&pattern;
        s[0]=s[3];
        s[1]=s[3];
        s[2]=s[3];

        pattern_buf_intp=(int *)pattern_buf;

#ifdef DEBUG
         (void) sprintf(error_message,
         "hxfpat_tefficient()-Pattern file(%s) pattern_buf=%x,pattern_buf_intp=%x\n",
             filename,pattern_buf,pattern_buf_intp);
         (void) fprintf(stderr, "%s", error_message);
         (void) fflush(stderr);
#endif /* DEBUG 1*/

        for (i=0; i<num_words; i++)
          {
            *pattern_buf_intp=pattern;
            pattern_buf_intp++;
          }
        
#ifdef DEBUG
         errno_save = errno;
         (void) sprintf(error_message,
         "hxfpat_tefficient()-Pattern file(%s) errno_save=%d,pattern=%x,hex_flag=%d,num_chars=%d,num_words=%d,modulus=%d,i=%d,buf_intp=%x\n",
             filename,errno_save,pattern,hex_flag,num_chars,num_words,modulus,i,pattern_buf_intp);
         (void) fprintf(stderr, "%s", error_message);
         (void) fflush(stderr);
#endif /* DEBUG */

  
        }
     } /* end if hex_flag */
  else
  /*
   * Open the pattern file and check for errors...
   */
  if ((fildes = open(filename, O_RDONLY, mode_flag)) == -1)
    {
#ifdef DEBUG
      errno_save = errno;
      (void) sprintf(error_message,
		     "\nhxfpat_tefficient() -- Error opening pattern file (%s).\n\
errno = %d (%s).\n",
		     filename,
		     errno_save,
		     strerror(errno_save));
      (void) fprintf(stderr, "%s", error_message);
      (void) fflush(stderr);
#endif /* DEBUG */
      return_code = 1;
    }
  else                        /* If here, pattern file opened OK.            */
    {
      
      /*
       * Clear the pattern buffer...
       */
      (void) memset((void *) pattern_buf, 0, num_chars);
      
      errno = 0;
      /*
       * Read the pattern file and check for errors...
       */
      if ((chars_read = read(fildes, pattern_buf, num_chars)) == -1)
	{
#ifdef DEBUG
	  errno_save = errno;
	  (void) sprintf(error_message,
			 "\nhxfpat_tefficient() -- Error reading pattern file (%s).\n\
errno = %d (%s).\n",
			 filename,
			 errno_save,
			 strerror(errno_save));
	  (void) fprintf(stderr, "%s", error_message);
	  (void) fflush(stderr);
#endif /* DEBUG */
	  return_code = 2;
	}
      else                        /* If here, read() was OK.                 */
	{
	  /*
	   * Copy the pattern until the buffer is full...
           */
	  chars_copied = chars_read;
	  while (chars_copied < num_chars)
	    {
	      chars_left_to_copy = num_chars - chars_copied;
	      
	      if (chars_left_to_copy < chars_copied)
		chars_this_copy = chars_left_to_copy;
	      else
		chars_this_copy = chars_copied;
	      
	      (void) memcpy((void *) (pattern_buf + chars_copied),
			    (void *) pattern_buf,
			    chars_this_copy);
	      
	      chars_copied += chars_this_copy;
	    } /* endwhile */
	} /* endif */
      /*
       * Close the pattern file.
       */
      errno = 0;
      if (close(fildes) != 0)
	{
#ifdef DEBUG
	  errno_save = errno;
	  (void) sprintf(error_message,
			 "\nhxfpat_tefficient() -- Error closing pattern file (%s).\n\
errno = %d (%s).\n",
			 filename,
			 errno_save,
			 strerror(errno_save));
	  (void) fprintf(stderr, "%s", error_message);
	  (void) fflush(stderr);
#endif /* DEBUG */
	  return_code = 3;
	} /* endif */
    } /* endif */
  
  return(return_code);
  
} /* hxfpat_tefficient() */
