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

/* @(#)37	1.4  src/htx/usr/lpp/htx/bin/hxecd/seq_oper.c, exer_cd, htxubuntu 5/24/04 17:25:32 */

/******************************************************************************
 *   COMPONENT_NAME: exer_cd
 * 
 *   MODULE NAME: seq_oper.c
 *
 *   FUNCTIONS: init_blkno
 *              set_blkno
 *              set_first_blk
 *              wrap
 *
 *   DESCRIPTION: Functions used in sequential operations.
 ******************************************************************************/
#include "hxecd.h"

/**************************************************************************/
/* initialize block number                                                */
/**************************************************************************/
void init_blkno(struct ruleinfo *pr, int *blkno)
{
  int blk, over;

  pr->first_block = set_first_blk(pr);
  over = pr->first_block + pr->num_blks - pr->max_blkno;
  if ( over > 0 )
     blk = pr->first_block - over;
  else
     blk = pr->first_block;
  if ( strcmp(pr->direction, "UP") == 0 ) 
     blkno[0] = blkno[1] = blk;
  else if ( strcmp(pr->direction, "DOWN") == 0 ) 
     blkno[0] = blkno[2] = blk;
  else if ( strcmp(pr->direction, "IN") == 0 ) {
     if ( blk > (pr->min_blkno + (pr->max_blkno - pr->min_blkno) / 2) ) {
        blkno[0] = blkno[2] = blk;
        blkno[1] = pr->max_blkno + pr->min_blkno - blk -1 - pr->num_blks ;
     } else {
        blkno[0] = blkno[1] = blk;
        blkno[2] = pr->max_blkno + pr->min_blkno - blk -1 + pr->num_blks ;
     } 
  } else 
     blkno[0] = blkno[1] = blkno[2] = blk;
  return;
} 

/*************************************************************************/
/* set first_block number for sequential operations                      */
/*************************************************************************/
int set_first_blk(struct ruleinfo *pr)
{
  int   blk; 

  if ( (strcmp(pr->starting_block, "BOT")) == 0 )
     blk = pr->min_blkno;
  else if ( (strcmp(pr->starting_block, "MID")) == 0 )
     blk = pr->min_blkno + (pr->max_blkno - pr->min_blkno) / 2;
  else if ( (strcmp(pr->starting_block, "TOP")) == 0 )
     blk = pr->max_blkno - 1;
  else {
     blk = atoi(pr->starting_block);
     blk = blk + pr->min_blkno;
  }
  return(blk);
} 

/**************************************************************************/
/* set next block number                                                  */
/**************************************************************************/
void set_blkno( int *blkno, char *direction, int increment, int num_blks)
{
  if ( strcmp(direction, "UP") == 0 ) 
     blkno[0] = blkno[1] = blkno[1] + num_blks + increment;
  else if ( strcmp(direction, "DOWN") == 0 ) 
     blkno[0] = blkno[2] = blkno[2] - num_blks - increment;
  else if ( blkno[0] == blkno[1] )
     blkno[0] = blkno[2] = blkno[2] - num_blks - increment;
  else
     blkno[0] = blkno[1] = blkno[1] + num_blks + increment;
} 

/**************************************************************************/
/* check for file wrap around on sequential operations                    */
/**************************************************************************/
int wrap(struct ruleinfo *pr, int *blkno)
{
  int rc;

  if ( strcmp(pr->direction, "UP") == 0 ) {
     if ( (blkno[1] + pr->num_blks + pr->increment) > pr->max_blkno )
        rc = 1;
     else
        rc = 0;
  } else if ( strcmp(pr->direction, "DOWN") == 0 ) {
     if ( blkno[2] < pr->min_blkno )
        rc = 1;
     else
        rc = 0;
  } else if ( strcmp(pr->direction, "IN") == 0 ) {
     if ( blkno[1] > (pr->min_blkno + (pr->max_blkno - pr->min_blkno) / 2) ||
          blkno[2] < (pr->min_blkno + (pr->max_blkno - pr->min_blkno) / 2) )
        rc = 1;
     else
        rc = 0;
  } else {
    if ( ((blkno[1] + pr->num_blks + pr->increment) > pr->max_blkno) ||
         (blkno[2] < pr->min_blkno) )
       rc = 1;
    else
       rc = 0;
  }
  return(rc);
} 
