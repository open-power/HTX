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
 *   COMPONENT_NAME: exer_cd
 * 
 *   MODULE NAME: rnd_oper.c
 *
 *   FUNCTIONS: init_seed
 *              random_blkno
 *              random_dlen
 *
 *   DESCRIPTION: Random operation functions.
 ******************************************************************************/
#include "hxecd.h"

/**************************************************************************/
/* Initialize seed for random number generator (erand48)                  */
/**************************************************************************/
void init_seed(unsigned short seed[])
{
  long   clk;
  struct tm *tp;

  clk = time((long *) 0);
  tp = localtime(&clk);
  seed[0]  = (*tp).tm_sec;
  seed[1]  = (*tp).tm_min * (*tp).tm_sec;
  seed[2]  = (*tp).tm_hour * (*tp).tm_sec;
  return;
} 

/**************************************************************************/
/* returns random data length                                             */
/**************************************************************************/
int random_dlen( short bytpsec, long max_blkno,unsigned short seed[])
{
  int  dlen, i;

  i = 0;
  do {
     dlen = nrand48(seed) % ((BUF_SIZE / bytpsec) + 1);
     if ( dlen == 0 )
        dlen = bytpsec;
     else
        dlen = dlen * bytpsec;
     i++;
     if ( i > 100 )
        dlen = bytpsec;
  } while ( dlen > BUF_SIZE || (dlen / bytpsec) > max_blkno );
  return(dlen);
} 

/**************************************************************************/
/* sets random block number                                               */
/**************************************************************************/
void random_blkno( int *blkno, unsigned int dlen, short bytpsec, long max_blkno,unsigned short seed[], long min_blkno)
{
  int  i;

  i = 0;
  do {
     blkno[0] = nrand48(seed) % (max_blkno - min_blkno);
     i++;
     if ( i > 100 )
        blkno[0] = 0;
  } while( (blkno[0] + (dlen / bytpsec)) >= (max_blkno - min_blkno) );  
  blkno[0] = blkno[0] + min_blkno;
  return;
} 
