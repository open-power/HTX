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

/* @(#)33       1.17.3.2  src/htx/usr/lpp/htx/bin/hxecd/buf_oper.c, exer_cd, htxubuntu 1/20/14 05:23:35 */


/******************************************************************************
 *   COMPONENT_NAME: exer_cd
 *
 *   MODULE NAME: buf_oper.c
 *
 *   FUNCTIONS: bldbuf
 *              cmpbuf
 *
 *   DESCRIPTION: Fill write buffer with pattern, clear read buffer, compare
 *                read / write buffers
 ******************************************************************************/
#include "hxecd.h"

#pragma mc_func trap { "7c810808" }
#pragma reg_killed_by trap

extern int crash_on_mis;

/**************************************************************************/
/* build write buffer                                                     */
/**************************************************************************/
#include <time.h>
#include <string.h>

void bldbuf(unsigned short *wbuf, unsigned int dlen, int buf_size,
       char *pattern_id, int *blkno)
{
   int            i, j = 1, k, dlen_static, a, b, c;
   unsigned short xsubi[3];

   dlen_static = dlen;
   init_seed(xsubi);
   a = xsubi[0];
   b = xsubi[1];
   c = xsubi[2];
   dlen = dlen / 512;
   i = 0;
   if ( (strcmp(pattern_id, "#001") == 0) ||
        (strcmp(pattern_id, "#002") == 0) ) {
      for ( ; dlen != 0; --dlen, j++ ) {
         wbuf[i]   = blkno[0] >> 16;
         wbuf[i+1] = blkno[0];
         wbuf[i+2] = a;
         wbuf[i+3] = b;
         wbuf[i+4] = c;
         wbuf[i+5] = nrand48(xsubi) % 65536;
         for ( k = 6; k <= 256; ) {
            wbuf[i+k] = (i + k) / 2;
            if ( strcmp(pattern_id, "#001") == 0 )
               wbuf[i+k+1] = j * 0x101;
            else
               wbuf[i+k+1] = nrand48(xsubi) % 65536;
            k += 2;
         }
         i += 256;
      }
      while ( (buf_size - dlen_static) >=0 ) {
          wbuf[i] = 0xffff;
          i++;
          buf_size -= 2;
      }
   }
}

/************************************************************************/
/* cmpbuf - Compare read buffer to write buffer. If a miscompare is de- */
/* tected and crash_on_mis = YES, the system will crash and go into the */
/* the kernel debugger. The parameters to crash_sys() will be loaded in */
/* the CPU registers. The flag value 0xBEFFDEAD will in the first reg;  */
/* pointers to the wbuf and rbuf will be in the 2nd and 3rd regs. The   */
/* offset ot the miscompare will be in the 4th reg and the 5th reg will */
/* hold a pointer to the device that caused the exerciser to go into    */
/* the kernel debugger.                                                 */
/************************************************************************/
#include <fcntl.h>
#include <time.h>
#define DUMP_PATH "/tmp/"
#define MISC_THRESH 10
#define M2F2_MISCOM_THRESH 3

char cmpbuf(struct htx_data *ps, struct ruleinfo *pr, int loop,
            int *blkno, char wbuf[], char rbuf[])
{
  int           c, mis_flag = FALSE;
  char          s[3], ctime[9], path[128], log_dir[64];
  char          work_str[512], msg1[MAX_TEXT_MSG];
  static int    cnt_flag = FALSE;
  register long i, j;
  static ushort cnt = 0, m2f2_cnt = 0;
  static struct {
     int     start_lba;
     int     offset;
     time_t  dtime;
     char    cpath[128];
     char    rpath[128];
  } m2f2[M2F2_MISCOM_THRESH];

  i = 0;
  while ( (mis_flag == FALSE) && (i < pr->dlen) ) {
     if ( wbuf[i] != rbuf[i] ) {
        if ( crash_on_mis && ( strcmp(pr->mode, "M2F2") != 0 ) ) {
		#ifndef __HTX_LINUX__  /* AIX */
	   		setleds( 0x2010 );
        		trap(0xBEEFDEAD, wbuf, rbuf, i, ps, pr);
	   	#else
                        do_trap_htx64( 0xBEEFDEAD, (unsigned long)wbuf, (unsigned long)rbuf, (unsigned long)i, (unsigned long)ps, (unsigned long)pr, 0, 0);
	   	#endif
	   }
           /* crash_sys(0xBEEFDEAD, wbuf, rbuf, i, ps->sdev_id); */
        mis_flag = TRUE;
     } else
        i++;
  }

  if ( mis_flag == TRUE ) {
      ps->bad_others = ps->bad_others + 1;
      sprintf(msg1, "Miscompare at buffer offset %ld (0x%lx)  "
                    "\ncbuf (baseaddr 0x%llx) ", i, i, (unsigned long long)wbuf);
      for ( j = i; ((j - i) < 20) && (j < pr->dlen); j++ ) {
          sprintf(s, "%.2x", wbuf[j]);
          strcat(msg1, s);
      }
      sprintf(work_str, "\nrbuf (baseaddr 0x%llx) ",(unsigned long long)rbuf);
      strcat(msg1, work_str);
      for ( j = i; ((j - i) < 20) && (j < pr->dlen); j++ ) {
          sprintf(s, "%.2x", rbuf[j]);
          strcat(msg1, s);
      }
      strcat(msg1, "\n");
      if ( strcmp(pr->mode, "M2F2") != 0 ) {
          cnt++;
          if ( cnt < 10 ) {
             strcpy(path, ps->htx_exer_log_dir);
             strcat(path, "htx");
             strcat(path, &(ps->sdev_id[5]));
             strcat(path, ".cbuf");
             sprintf(work_str, "%-d", cnt);
             strcat(path, work_str);
             hxfsbuf(wbuf, pr->dlen, path, ps);
             sprintf(work_str, "Compare buffer saved in %s\n", path);
             strcat(msg1, work_str);
             strcpy(path, log_dir);
             strcat(path, "htx");
             strcat(path, &(ps->sdev_id[5]));
             strcat(path, ".rbuf");
             sprintf(work_str, "%-d", cnt);
             strcat(path, work_str);
             hxfsbuf(rbuf, pr->dlen, path, ps);
             sprintf(work_str, "Read buffer saved in %s\n", path);
             strcat(msg1, work_str);
          } else {
             sprintf(work_str, "The maximum number of saved miscompare "
                               "buffers (10) have already\n been saved "
                               "The compare and read buffers for this "
                               "miscompare will\nnot be saved to disk");
             strcat(msg1, work_str);
          }
          /* prt_msg_asis(ps, pr, loop, blkno, 100, HARD, msg1); */
          prt_msg_asis(ps, pr, loop, blkno, 100, HTX_HE_MISCOMPARE, msg1);
          return(1);
      } else {
          if ( cnt_flag == TRUE ) {
             if ( m2f2_cnt == 1 ) {
                for ( c = 0; c < (M2F2_MISCOM_THRESH - 1); c++ ) {
                   unlink(m2f2[c].cpath);
                   unlink(m2f2[c].rpath);
                }
                rename(m2f2[2].cpath, m2f2[0].cpath);
                rename(m2f2[2].rpath, m2f2[0].rpath);
                m2f2[0].start_lba = m2f2[2].start_lba;
                m2f2[0].offset    = m2f2[2].offset;
                m2f2[0].dtime     = m2f2[2].dtime;
             } else {
                for ( c = 0; c < (M2F2_MISCOM_THRESH - 1); c++ ) {
                   unlink(m2f2[c].cpath);
                   unlink(m2f2[c].rpath);
                   rename(m2f2[c+1].cpath, m2f2[c].cpath);
                   rename(m2f2[c+1].rpath, m2f2[c].rpath);
                   m2f2[c].start_lba = m2f2[c+1].start_lba;
                   m2f2[c].offset    = m2f2[c+1].offset;
                   m2f2[c].dtime     = m2f2[c+1].dtime;
                }
             }
             cnt_flag = FALSE;
          }
          m2f2[m2f2_cnt].start_lba = blkno[0];
          m2f2[m2f2_cnt].offset = i;
          (void) time(&m2f2[m2f2_cnt].dtime);
          m2f2_cnt++;
          strcpy(path, log_dir);
          strcat(path, "m2f2");
          strcat(path, &(ps->sdev_id[5]));
          strcat(path, ".cbuf");
          sprintf(work_str, "%-d", m2f2_cnt);
          strcat(path, work_str);
          strcpy(m2f2[m2f2_cnt-1].cpath, path);
          hxfsbuf(wbuf, pr->dlen, path, ps);
          sprintf(work_str, "M2F2 Compare buffer saved in %s\n", path);
          strcat(msg1, work_str);
          strcpy(path, log_dir);
          strcat(path, "m2f2");
          strcat(path, &(ps->sdev_id[5]));
          strcat(path, ".rbuf");
          sprintf(work_str, "%-d", m2f2_cnt);
          strcat(path, work_str);
          strcpy(m2f2[m2f2_cnt-1].rpath, path);
          hxfsbuf(rbuf, pr->dlen, path, ps);
          sprintf(work_str, "M2F2 Read buffer saved in %s\n", path);
          strcat(msg1, work_str);
          if ( m2f2_cnt < M2F2_MISCOM_THRESH ) {
             strcat(msg1,"\n NOTE: The above miscompare is reported as a SOFT error because \n 3 miscompares are allowed in one hour for M2F2 data. \n We trap to kdb / xmon if the number of miscompares exceeds 3 in one hour.");
             prt_msg_asis(ps, pr, loop, blkno, 4, SOFT, msg1);
             return(0);
          } else {
             if ( m2f2[2].dtime > (m2f2[0].dtime + 3600) ) {
                if ( m2f2[2].dtime > (m2f2[1].dtime + 3600) )
                   m2f2_cnt = 1;
                else
                   m2f2_cnt--;
                cnt_flag = TRUE;
                prt_msg_asis(ps, pr, loop, blkno, 0, SOFT, msg1);
                return(0);
            } else {

				if ( crash_on_mis ) {
				#ifndef __HTX_LINUX__  /* AIX */
					setleds( 0x2010 );
						trap(0xBEEFDEAD, wbuf, rbuf, i, ps, pr);
				#else
								do_trap_htx64( 0xBEEFDEAD, (unsigned long)wbuf, (unsigned long)rbuf, (unsigned long)i, (unsigned long)ps, (unsigned long)pr, 0, 0);
				#endif
			   }

                sprintf(work_str, "There have been %d M2F2 miscompares in"
                                  " the past HOUR!\n", M2F2_MISCOM_THRESH);
                strcat(msg1, work_str);
                strftime(ctime, 9, "%T", localtime(&m2f2[0].dtime));
                sprintf(work_str, "Start LBA = %d  Offset = %d  Time = %s\n",
                        m2f2[0].start_lba, m2f2[0].offset, ctime);
                strcat(msg1, work_str);
                strftime(ctime, 9, "%T", localtime(&m2f2[1].dtime));
                sprintf(work_str, "Start LBA = %d  Offset = %d  Time = %s\n",
                        m2f2[1].start_lba, m2f2[1].offset, ctime);
                strcat(msg1, work_str);
                strftime(ctime, 9, "%T", localtime(&m2f2[2].dtime));
                sprintf(work_str, "Start LBA = %d  Offset = %d  Time = %s\n",
                        m2f2[2].start_lba, m2f2[2].offset, ctime);
                strcat(msg1, work_str);
                m2f2_cnt--;
                cnt_flag = TRUE;
                /* prt_msg_asis(ps, pr, loop, blkno, 100, HARD, msg1); */
                prt_msg_asis(ps, pr, loop, blkno, 100, HTX_HE_MISCOMPARE, msg1);
                return(-1);
            }
          }
      }
  }
  return(0);
}
