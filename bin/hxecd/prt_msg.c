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

/* @(#)36	1.6  src/htx/usr/lpp/htx/bin/hxecd/prt_msg.c, exer_cd, htxubuntu 5/24/04 17:24:55 */

/******************************************************************************
 *   COMPONENT_NAME: exer_cd
 * 
 *   MODULE NAME: prt_msg.c
 *
 *   FUNCTIONS: info_msg
 *              prt_msg
 *              prt_msg_asis
 *
 *   DESCRIPTION: Functions to format messages and send to HTX.
 ******************************************************************************/
#include "hxecd.h"

/**************************************************************************/
/* format information message                                             */
/**************************************************************************/
void info_msg (struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno, char *msg_text)
{
  sprintf(msg_text, "%s: loop = %5d,  blk = %5d, len = %4d\n",
          pr->rule_id, loop, blkno[0], pr->dlen);
  return;
} 

/**************************************************************************/
/* send message to HTX                                                    */
/**************************************************************************/
void prt_msg (struct htx_data *ps, struct ruleinfo *pr, int loop,int *blkno, int err, int sev, char *text)
{
  char msg[550];

  info_msg(ps, pr, loop, blkno, msg);
  strcat(msg, text);
  strcat(msg, strerror(err));
  hxfmsg(ps, err, sev, msg);
  return;
} 

/****************************************************************/
/* Send message to HTX as-is to bypass system error messages    */
/****************************************************************/
void prt_msg_asis (struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,int *pblk_num, int err, int sev, char *text)
{
  char msg[550];

  info_msg(phtx_info, prule_info, loop, pblk_num, msg);
  strncat(msg, text, (sizeof(msg) - strlen(msg)) );
  hxfmsg(phtx_info, err, sev, msg);
  return;
} 

