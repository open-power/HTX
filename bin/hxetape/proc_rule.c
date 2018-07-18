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
 * MODULE NAME: proc_rule.c
 *
 * FUNCTIONS: proc_rule() processes a stanza from the rules file.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#ifndef __HTX_LINUX
#include "hxetape.h"
#else
#include <hxetape.h>
#endif

extern unsigned int BLK_SIZE;
int    tape_error_code;

int
proc_rule(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          char *wbuf, char *rbuf, struct blk_num_typ *pblk_num)
{
  int            dlen, loop, rc;
  char           msg[220], path[100];
  unsigned short seed[3];

  rc = 0;
  init_seed(seed);            /* initialize seed for random number generator */
  dlen = prule_info->num_blks * BLK_SIZE;       /* initialize length of data */
                                           /*-------------------------------*/
                                           /* initialize the write buffer   */
                                           /*-------------------------------*/
  if ( (prule_info->pattern_id[0] != '#') &&
       (prule_info->pattern_id[0] != 0) ) {
	 path[0] ='\0';
     if ( getenv("HTXPATTERNS") != NULL) {
        htx_strcpy(path, getenv("HTXPATTERNS"));
     } else {
        htx_strcpy(path, "../pattern/");         /* default ONLY */
     }
     htx_strcat (path, prule_info->pattern_id);
     rc = hxfpat(path, wbuf, dlen);
     if ( rc == 1 ) {
        sprintf(msg, "cannot open pattern file - %s\n", path);
        hxfmsg(phtx_info, 0, SYSERR, msg);
        return(1);
     }
     if ( rc == 2 ) {
        sprintf(msg, "cannot read pattern file - %s\n", path);
        hxfmsg(phtx_info, 0, SYSERR, msg);
        return(1);
     }
  } else if ( prule_info->pattern_id[0] == '#' )
     bldbuf((unsigned short*)wbuf, dlen, prule_info->pattern_id, pblk_num);
  pblk_num->in_rule = 0;      /* initialize block number within current rule */
  rc = 0;
  tape_error_code = 0;
  for ( loop = 1; loop <= prule_info->num_oper; loop++ ) {
     if ( strcmp(prule_info->oper, "R") == 0 ) {
        rc = read_tape(phtx_info, prule_info, loop, pblk_num, rbuf);
     } else if ( strcmp(prule_info->oper, "W") == 0 ) {
        rc = write_tape(phtx_info, prule_info, loop, pblk_num, wbuf);
     } else if ( strcmp(prule_info->oper, "RC") == 0 ) {
        rc = read_tape(phtx_info, prule_info, loop, pblk_num, rbuf);
        if ( rc >= 0 )
           rc = cmpbuf(phtx_info, prule_info, loop, pblk_num, wbuf, rbuf);
     } else if ( strcmp(prule_info->oper, "RW") == 0 ) {
        rc = rewind_tape(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "WEOF") == 0 ) {
        rc = weof_tape(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "SF") == 0 ) {
        rc = search_file(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "SR") == 0 ) {
        rc = search_rec(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "D") == 0 ) {
        rc = diag_tape(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "E") == 0 ) {
        rc = erase_tape(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "S") == 0 ) {
        rc = do_sleep(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "CO") == 0 ) {
        rc = close_open(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "C") == 0 ) {
        rc = tclose(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "O") == 0 ) {
        rc = topen(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "WEOT") == 0 ) {
        rc = write_eot(phtx_info, prule_info,loop, pblk_num, wbuf);
     } else if ( strcmp(prule_info->oper, "RCEOT") == 0 ) {
        rc = read_eot(phtx_info, prule_info, loop,pblk_num, wbuf, rbuf);
     } else if ( strcmp(prule_info->oper, "REOT") == 0 ) {
        rc = read_teot(phtx_info, prule_info, loop, pblk_num, wbuf, rbuf);
     } else if ( strcmp(prule_info->oper, "RS") == 0 ) {
        rc = prt_req_sense(phtx_info, prule_info, loop, pblk_num);
	 }
#ifndef __HTX_LINUX__
     else if ( strcmp(prule_info->oper, "ML") == 0 ) {
        rc = medium_load(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "MUL") == 0 ) {
        rc = medium_unload(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "RES") == 0 ) {
        rc = read_status(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "IE") == 0 ) {
        rc = init_element(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "RP") == 0 ) {
        rc = read_posit(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "LB") == 0 ) {
        rc = loc_block(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "ASF") == 0 ) {
        rc = asearch_file(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "ASR") == 0 ) {
        rc = asearch_rec(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "ADUL") == 0 ) {
        rc = write_unload(phtx_info, prule_info, loop, pblk_num, wbuf);
     } else if ( strcmp(prule_info->oper, "TWIE") == 0 ) {
        rc = twin_tape(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "TWPE") == 0 ) {
        rc = twps_tape(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "TWRE") == 0 ) {
        rc = twrd_stat(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "TWMM") == 0 ) {
        rc = twmv_tape(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "TWUL") == 0 ) {
        rc = unload_write(phtx_info, prule_info, loop, pblk_num, wbuf);
     } else if ( strcmp(prule_info->oper, "WUL") == 0 ) {
        rc = tape_unload(phtx_info, prule_info, loop, pblk_num, wbuf);
     } else if ( strcmp(prule_info->oper, "CDRE") == 0 ) {
        rc = cdrd_stat(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "CDMM") == 0 ) {
        rc = cdmv_tape(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "HUNL") == 0 ) {
        rc = himove(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "HINI") == 0 ) {
        rc = hiinit(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "HREL") == 0 ) {
        rc = hielem(phtx_info, prule_info, loop, pblk_num);
     } else if ( strcmp(prule_info->oper, "HWUN") == 0 ) {
        rc = hidal_unload(phtx_info, prule_info, loop, pblk_num, wbuf);
     } else if ( strcmp(prule_info->oper, "DBUG") == 0 ) {
       rc = set_dbug(phtx_info, prule_info, pblk_num);
     }
#endif
	 else if ( strcmp(prule_info->oper, "XCMD") == 0 ) {
       rc = do_cmd(phtx_info, prule_info, pblk_num);
     } else {
        ;
     }
     hxfupdate(UPDATE, phtx_info);
     if ( phtx_info->run_type[0] == 'O' ) {
        info_msg(phtx_info, prule_info, loop, pblk_num, msg);
        hxfmsg(phtx_info, 0, INFO, msg);
     }
     if ( rc != 0 )
        break;
  }
  return(rc);
}
