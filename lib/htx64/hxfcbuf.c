/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

static char sccsid[] = "@(#)73  1.2  src/htx/usr/lpp/htx/lib/htxmp64/hxfcbufmp_new.c, htx_libhtxmp, htxfedora 10/8/10 04:38:39";
#include "htx_local.h"
#include "hxihtx64.h"

#include <fcntl.h>
#ifndef __HTX_LINUX__
#include <sys/mode.h>
#endif 
#include <pthread.h>

#define MAX_MISCOMPARES 10
#define MAX_MSG_DUMP 20

/*
 * NAME: hxfcbuf_calling_hxfsbuf_tsafe()
 *
 *
 * FUNCTION: Compares two buffers against each other.
 *
 * EXECUTION ENVIRONMENT:
 *
 *      This function is called by any Hardware Exerciser (HE) program and
 *      is included as part of the libhtx.a library.
 *
 * NOTES:
 *
 *      This routine compares two buffers and if the compare is good returns
 *      a NULL pointer.
 *
 *      If the buffers do not compare a pointer to a character string in the
 *      following format is returned:
 *
 *          miscompare at displacement (decimal) = xxxxxxxx
 *          wbuf = xxxxxxxxxxxxxxxxxxxxxxx.... (20 bytes in hex)
 *          rbuf = xxxxxxxxxxxxxxxxxxxxxxx....
 *                 |
 *                 _ Byte that did not compare
 *
 *      And the two buffers are written out to the HTX dump directory.
 *
 *
 *      operation:
 *      ---------
 *
 *      set good return code (NULL error message pointer)
 *
 *      compare buffers
 *
 *      if buffers do not compare OK
 *          set pointer to error message
 *          build error message
 *
 *          save buffers to disk
 *
 *      return(message pointer)
 *
 *
 * RETURNS:
 *
 *      Return Codes:
 *      ----------------------------------------------------------------------
 *                   0 -- Normal exit; buffers compare OK.
 *       exit_code > 0 -- Problem on compare; pointer to error msg returned.
 *
 *
 */

int hxfcbuf_calling_hxfsbuf_tsafe(struct htx_data *ps, char *wbuf, char *rbuf, size_t len, char *msg)
     /*
      * ps -- pointer to the HE's htx_data structure
      * wbuf -- pointer to the write buffer
      * rbuf -- pointer to the read buffer
      * len -- the length in bytes of the buffers
      */
{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  register long i;           /* compare loop counter                         */
  register long j;           /* error message loop counter                   */

	char *msg_ptr;             /* pointer to error message (NULL if good comp) */
	char path[128];            /* dump files path                              */
	char s[3];                 /* string segment used when building error msg  */
	char work_str[512];        /* work string                                  */

	int mis_flag, rc = 0;              /* miscompare flag: boolean                     */
        char tmp_path[64];
        char *tmp_path_temp;

        tmp_path_temp = getenv("HTX_LOG_DIR");
        if (tmp_path_temp == NULL){
                strcpy(tmp_path, "/tmp/");
	}
        else{
                strcpy(tmp_path,getenv("HTX_LOG_DIR"));
        }


	/*static ushort miscompare_count = 0;*//* miscompare count         */

	/*
	***  Beginning of Executable Code  *****************************************
	*/
	msg_ptr = 0;               /* set good return code (NULL pointer)          */

  mis_flag = FALSE;          /* set miscompare flag to FALSE                 */
  i = 0;

	while ((mis_flag == FALSE) && (i < len))
	{
		if (wbuf[i] != rbuf[i])
		{
			mis_flag = TRUE;
 		}
		else
  		{
			i++;
		}
	} /* endwhile */

  if (mis_flag == TRUE)      /* problem with the compare?                    */
    {
   		rc = -1;
   		msg_ptr = msg;             /* show bad compare                         */
		(void) sprintf(msg_ptr, "Miscompare at displacement (decimal) = %d ",(int)i);

		(void) strcat(msg_ptr, "  wbuf = ");

      		for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++)
        	{

#ifdef	__HTX_LINUX__
			(void) sprintf(s, "%x", wbuf[j]);
#else
			(void) sprintf(s, "%0.2x", wbuf[j]);
#endif
       			(void) strcat(msg_ptr, s);
       		} /* endfor */

      		(void) strcat(msg_ptr, "  rbuf = ");

      		for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++)
        	{

#ifdef	__HTX_LINUX__
			(void) sprintf(s, "%x", rbuf[j]);
#else
			(void) sprintf(s, "%0.2x", rbuf[j]);
#endif

          		(void) strcat(msg_ptr, s);
        	} /* endfor */

      		/* (void) strcat(msg, "\n"); */


      		if (ps->miscompare_count < MAX_MISCOMPARES)
		{
	  		/*
	   		* Copy write and read buffers to dump file.
	   		*/
	  		ps->miscompare_count++;
	  		(void) strcpy(path, tmp_path);
	  		(void) strcat(path, "htx");
	  		(void) strcat(path, &(ps->sdev_id[5]));
	  		(void) strcat(path, ".wbuf");
	  		(void) sprintf(work_str, "_%d_%-d",getpid(), ps->miscompare_count);
	  		(void) strcat(path, work_str);

            (void) strcat(msg_ptr, ". The miscompare buffer dump files are ");
            (void) strcat(msg_ptr, path);

	  		(void) hxfsbuf(wbuf, len, path, ps);

	  		(void) strcpy(path, tmp_path);
	  		(void) strcat(path, "htx");
	  		(void) strcat(path, &(ps->sdev_id[5]));
	  		(void) strcat(path, ".rbuf");
	  		(void) strcat(path, work_str);

            (void) strcat(msg_ptr, " and ");
            (void) strcat(msg_ptr, path);

	  		(void) hxfsbuf(rbuf, len, path, ps);
		}
      		else
		{
	  		(void) sprintf(work_str, "The maximum number of saved miscompare \
			buffers (%d) have already\nbeen saved.  The read and write buffers for this \
			miscompare will\nnot be saved to disk.\n", MAX_MISCOMPARES);
	  		(void) strcat(msg_ptr, work_str);
		} /* endif */

    } /* endif */
    if (mis_flag == TRUE) 
       return(-1);
    else 
       return(0);

} /* hxfcbuf_calling_hxfsbuf_tsafe() */


/*
 * NAME: hxfcbuf_tefficient()
 *
 *
 * FUNCTION: Compares two buffers against each other.
 *
 * EXECUTION ENVIRONMENT:
 *
 *      This function is called by any Hardware Exerciser (HE) program and
 *      is included as part of the libhtx.a library.
 *
 * NOTES:
 *
 *      This routine compares two buffers and if the compare is good returns
 *      a NULL pointer.
 *
 *      If the buffers do not compare a pointer to a character string in the
 *      following format is returned:
 *
 *          miscompare at displacement (decimal) = xxxxxxxx
 *          wbuf = xxxxxxxxxxxxxxxxxxxxxxx.... (20 bytes in hex)
 *          rbuf = xxxxxxxxxxxxxxxxxxxxxxx....
 *                 |
 *                 _ Byte that did not compare
 *
 *      And the two buffers are written out to the HTX dump directory.
 *
 *
 *      operation:
 *      ---------
 *
 *      set good return code (NULL error message pointer)
 *
 *      compare buffers
 *
 *      if buffers do not compare OK
 *          set pointer to error message
 *          build error message
 *
 *          save buffers to disk
 *
 *      return(message pointer)
 *
 *
 * RETURNS:
 *
 *      Return Codes:
 *      ----------------------------------------------------------------------
 *                   0 -- Normal exit; buffers compare OK.
 *       exit_code > 0 -- Problem on compare; pointer to error msg returned.
 *
 *
 */

int hxfcbuf_tefficient(struct htx_data *ps, char *wbuf, char *rbuf, size_t len, char *msg)
     /*
      * ps -- pointer to the HE's htx_data structure
      * wbuf -- pointer to the write buffer
      * rbuf -- pointer to the read buffer
      * len -- the length in bytes of the buffers
      */
{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  register long i;           /* compare loop counter                         */
  register long j;           /* error message loop counter                   */

  char path[128];            /* dump files path                              */
  char s[3];                 /* string segment used when building error msg  */
  char work_str[512];        /* work string                                  */

  int mis_flag;              /* miscompare flag: boolean                     */

        char tmp_path[64];
        char *tmp_path_temp;

        tmp_path_temp = getenv("HTX_LOG_DIR");
        if (tmp_path_temp == NULL){
                strcpy(tmp_path, "/tmp/");
        }
        else{
                strcpy(tmp_path,getenv("HTX_LOG_DIR"));
        }


  /*
   ***  Beginning of Executable Code  *****************************************
   */

  mis_flag = FALSE;          /* set miscompare flag to FALSE                 */
  i = 0;

  while ((mis_flag == FALSE) && (i < len))
  {
    if (wbuf[i] != rbuf[i])
      mis_flag = TRUE;
    else
      i++;
  } /* endwhile */

  if (mis_flag == TRUE)      /* problem with the compare?                    */
    {
      (void) sprintf(msg, "Miscompare at displacement (decimal) = %ld ",i);

      (void) strcat(msg, "\nwbuf = ");

      for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++)
        {
          (void) sprintf(s, "%0.2x", wbuf[j]);
          (void) strcat(msg, s);
        } /* endfor */

      (void) strcat(msg, "\nrbuf = ");

      for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++)
        {
          (void) sprintf(s, "%0.2x", rbuf[j]);
          (void) strcat(msg, s);
        } /* endfor */

      (void) strcat(msg, "\n");


      if (ps->miscompare_count < MAX_MISCOMPARES)
	{
	  /*
	   * Copy write and read buffers to dump file.
	   */
	  ps->miscompare_count++;
	  (void) strcpy(path, tmp_path);
	  (void) strcat(path, "htx");
	  (void) strcat(path, &(ps->sdev_id[5]));
	  (void) strcat(path, ".wbuf");
	  (void) sprintf(work_str, "_%-d", ps->miscompare_count);
	  (void) strcat(path, work_str);

	  (void) hxfsbuf_tefficient(wbuf, len, path, ps);
	  
	  (void) strcpy(path, tmp_path);
	  (void) strcat(path, "htx");
	  (void) strcat(path, &(ps->sdev_id[5]));
	  (void) strcat(path, ".rbuf");
	  (void) strcat(path, work_str);
	  
	  (void) hxfsbuf_tefficient(rbuf, len, path, ps);
	}
      else
	{
	  (void) sprintf(work_str, "The maximum number of saved miscompare \
buffers (%d) have already\nbeen saved.  The read and write buffers for this \
miscompare will\nnot be saved to disk.\n", MAX_MISCOMPARES);
	  (void) strcat(msg, work_str);
	} /* endif */

    } /* endif */
    if (mis_flag == TRUE) 
       return(-1);
    else 
       return(0);

} /* hxfcbuf_tefficient() */

