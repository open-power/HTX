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

/******************************************************************************
 * COMPONENT_NAME: exer_tape
 *
 * MODULE NAME: buf_oper.c
 *
 * FUNCTIONS: bldbuf() - fills write buffer with patterned data
 *            cmpbuf() - compare read buffer to write buffer
 *         init_seed() - initializes seed for random number generation
 *****************************************************************************/
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#ifndef __HTX_LINUX__
#include "hxetape.h"
#else
#include <hxetape.h>
#endif


extern unsigned int BLK_SIZE;
extern int          crash_on_mis;
/************************************************************************/
/* bldbuf - build write buffer for patterns of #001 or #002             */
/************************************************************************/
void
bldbuf(unsigned short *wbuf, unsigned int dlen, char *pattern_id,
       struct blk_num_typ *pblk_num)
{
   int            i, j = 1, k;
   int            a, b, c, half_block;
   unsigned short xsubi[3];

   half_block = (BLK_SIZE / 2);       /* determine size for pattern block */
   init_seed(xsubi);
   a = xsubi[0];
   b = xsubi[1];
   c = xsubi[2];
   dlen = dlen / BLK_SIZE;            /* dlen = number of blocks to write */
   i = 0;
   if ( (strcmp(pattern_id, "#001") == 0) ||
        (strcmp(pattern_id, "#002") == 0) ) {
      for ( ; dlen != 0; --dlen, j++ ) {
           wbuf[i+0] = pblk_num->in_file >> 16;
           wbuf[i+1] = pblk_num->in_file;
           wbuf[i+2] = a;
           wbuf[i+3] = b;
           wbuf[i+4] = c;
           wbuf[i+5] = nrand48(xsubi) % 65536;
           for ( k = 6; k <= half_block; ) {
               wbuf[i+k] = (i + k) / 2;
               if ( strcmp(pattern_id, "#001") == 0 )
                  wbuf[i+k+1] = j * 0x101;
               else
                  wbuf[i+k+1] = nrand48(xsubi) % 65536;
               k += 2;
           }
           i += half_block;
      }
   }
}

/************************************************************************/
/* cmpbuf - Compare read buffer to write buffer. If a miscompare is de- */
/* tected and crash_on_mis = YES, the system will crash and go into the */
/* the kernel debugger. The parameters to crash_sys() will be loaded in */
/* the CPU registers. The flag value 0xBEFFDEAD will in the first reg;  */
/* pointers to the wbuf and rbuf will be in the 2nd and 3rd regs. The   */
/* wbuf length will be in the 4th reg and the 5th reg will hold the     */
/* pointer of the device that caused the exerciser to go into the       */
/* debugger.                                                            */
/************************************************************************/
int
cmpbuf(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
       struct blk_num_typ *pblk_num, char wbuf[], char rbuf[])
{
  int  rc = 0;
  char misc_data[MAX_TEXT_MSG];

  rc = cmp_buf(phtx_info, wbuf, rbuf, (prule_info->num_blks * BLK_SIZE), misc_data);
  if ( rc != 0 ) {
     if ( crash_on_mis )
     {
        #ifndef __HTX_LINUX__
	setleds( 0x2010 );
	#endif
	trap(0xBEEFDEAD, wbuf, rbuf, (prule_info->num_blks * BLK_SIZE), phtx_info, prule_info);
     }
     phtx_info->bad_others = phtx_info->bad_others + 1;
     /* prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, msgp); */
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HTX_HE_MISCOMPARE, misc_data);
     rc = -1;
  }
  return(rc);
}

/************************************************************************/
/* init_seed - Initialize seed for random number generator (nrand48)    */
/************************************************************************/
void
init_seed(unsigned short seed[])
{
  time_t clk;
  struct tm *tp;

  clk = time((long *) 0);
  tp = localtime(&clk);
  seed[0]  = (*tp).tm_sec;
  seed[1]  = (*tp).tm_min * (*tp).tm_sec;
  seed[2]  = (*tp).tm_hour * (*tp).tm_sec;
  return;
}

