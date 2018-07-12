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
 * MODULE NAME: prt_msg.c
 *
 * FUNCTIONS: info_msg()     - formats variables into an information message
 *            prt_msg()      - sends formatted information messages to HTX
 *            prt_msg_asis() - sends messages asis to bypass system error msgs
 *
 *****************************************************************************/
#include <string.h>
#include <stdio.h>
#ifdef __HTX_LINUX__
#include <hxetape.h>
#else
#include "hxetape.h"
#endif

extern unsigned int BLK_SIZE;

static  char msg[1024];

/**************************************************************************/
/* info_msg() - Format current rule information into the msg string       */
/**************************************************************************/
void
info_msg(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, char *msg_text)
{
   int byt_len;     /* length in bytes of (number of blocks * BLK_SIZE) */

   byt_len = 0;
   if ( pblk_num->in_rule > 0 )
      byt_len = prule_info->num_blks * BLK_SIZE;
   sprintf(msg_text, " %s:  Loop#: %d   Rule Blks: %d   File Blks: %d   "
                     "Bytes: %d   Pattern: %s\n",
           prule_info->rule_id, loop, pblk_num->in_rule, pblk_num->in_file,
           byt_len, prule_info->pattern_id);
   return;
}

/**************************************************************************/
/* prt_msg() - Send message to HTX along with deciphered system error msg */
/**************************************************************************/
void
prt_msg(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, int err, int sev,char *text)
{
   info_msg(phtx_info, prule_info, loop, pblk_num, msg);
   strncat(msg, text, (sizeof(msg) - strlen(msg)) );
   strncat(msg, strerror(err), (sizeof(msg) - strlen(msg)) );
   hxfmsg(phtx_info, err, sev, msg);
   return;
}

/**************************************************************************/
/* prt_msg_asis() - Send message to HTX as-is to bypass system error msgs */
/**************************************************************************/
void
prt_msg_asis(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
				struct blk_num_typ *pblk_num, int err, int sev, char *text)
{
   info_msg(phtx_info, prule_info, loop, pblk_num, msg);
   strncat(msg, text, (sizeof(msg) - strlen(msg)) );
   hxfmsg(phtx_info, err, sev, msg);
   return;
}

