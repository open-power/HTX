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

/* @(#)31	1.12.3.1  src/htx/usr/lpp/htx/bin/hxecd/proc_rule.c, exer_cd, htxubuntu 7/28/11 01:30:36 */

/******************************************************************************
 *   COMPONENT_NAME: exer_cd
 *
 *   MODULE NAME: proc_rule
 *
 *   FUNCTIONS: proc_rule
 *
 *   DESCRIPTION: Process a rules stanza.
 ******************************************************************************/
#include "hxecd.h"

extern char device_subclass[];

#define TMP_BUF_SIZE  (4*1024) /* 1 page */

int proc_rule(struct htx_data *ps, struct ruleinfo *pr, char *wbuf, char *rbuf, unsigned int *last_lba, unsigned int *pblk_size)
{
  int            blkno[3], blkno_save[3]; /* block # ptrs 0-curr,1-up,2-dwn */
  int            rc, i, rc_ptr, save_dlen, loop;
  unsigned short seed[3];
  char           path[100], msg[221], *tbuf, *tbuf_malloc;

  int read_res = -1;

  tbuf_malloc = malloc(TMP_BUF_SIZE + PAGE_SIZE);
  if(tbuf_malloc == NULL) { /* Out of memory? */
     sprintf(msg, "Cannot allocate memory for tbuf: %d bytes\n", TMP_BUF_SIZE + PAGE_SIZE);
     hxfmsg(ps, 0, SYSERR, msg);
     return(1);
  }

#ifndef __HTX_LINUX__     /* AIX */
  tbuf = tbuf_malloc;
#else                     /* Linux */
  /* Page align the tmp buffer for linux raw IO */
  tbuf = (char *)HTX_PAGE_ALIGN(tbuf_malloc);
#endif

  init_seed(seed);                /* init seed for random number generator */
  if ( strcmp(pr->addr_type, "SEQ") == 0 ||         /* init length of data */
       strcmp(pr->type_length, "FIXED") == 0)       /* to be transmitted   */
     pr->dlen = pr->num_blks * pr->bytpsec;
  else
     pr->dlen = random_dlen(pr->bytpsec, pr->tot_blks, seed);
  if ( strcmp(pr->oper, "RC") == 0 ) {      /* init write buff for oper RC */
                                   /* get HTXPATTERNS environment variable */
     strcpy(path, ""); /* Linux will need this initialization */
     if ((char *) getenv("HTXPATTERNS") != NULL) {
                 strcpy(path, (char *) getenv("HTXPATTERNS"));
     } else {
         strcpy(path, "../pattern/");                       /* default ONLY */
     }
     strcat (path, pr->pattern_id);
     rc = hxfpat(path, wbuf, pr->dlen);
     if ( rc == 1 ) {
        sprintf(msg, "Cannot open pattern file - %s\n", path);
        hxfmsg(ps, 0, SYSERR, msg);
        return(1);
     }
     if ( rc == 2 ) {
        sprintf(msg, "Cannot read pattern file - %s\n", path);
        hxfmsg(ps, 0, SYSERR, msg);
        return(1);
     }
  }
  if ( strcmp(pr->addr_type, "SEQ") == 0 )    /* init current block number */
     init_blkno(pr, blkno);
  else
     random_blkno(blkno, pr->dlen, pr->bytpsec, pr->max_blkno, seed,
                  pr->min_blkno);
  for ( i = 0; i < 3; i++ )
      blkno_save[i] = blkno[i];

  for ( loop = 1 ; loop <= pr->num_oper ; loop++ ) {
     if ( strcmp(pr->oper, "MS") == 0 ) {
        ms_get(pr, ps, last_lba, pblk_size);
    } else if ( strcmp(pr->oper, "R") == 0 ) {
        read_cdrom(ps, pr, loop, blkno, rbuf);
    } else if ( strcmp(pr->oper, "RWP") == 0 ) {
        read_write_pattern(ps, pr, loop, blkno, rbuf);
    } else if ( strcmp(pr->oper, "RRC") == 0 ) {
        /*read_cdrom(ps, pr, loop, blkno, wbuf);
        read_cdrom(ps, pr, loop, blkno, rbuf);*/

        if( (read_res = read_cdrom(ps, pr, loop, blkno, wbuf) ) == 0 ) {/* read success */
        	if( (read_res = read_cdrom(ps, pr, loop, blkno, rbuf) ) == 0 ) {/* read success, again */
        		cmpbuf(ps, pr, loop, blkno, wbuf, rbuf); /* safe to compare */
			} else { /* second read fail */
				 sprintf(msg, "read_cdrom() failed in Re-Read sequence of RRC oper\n"
				              "Skipping compare operation\n");
				 prt_msg(ps, pr, loop, blkno, 0, SOFT, msg);
			}
		} else { /* first read fail */
			 sprintf(msg, "read_cdrom() failed in Read sequence of RRC oper\n"
						  "Skipping compare operation\n");
			 prt_msg(ps, pr, loop, blkno, 0, SOFT, msg);
		}

    } else if ( strcmp(pr->oper, "RC") == 0 ) {
        for ( i = 0; i < 3; i++ )
           blkno[i] = blkno_save[i];      /* RC = always start at same blkno */
                       /******************************************************/
                       /*- First, read the successive data blocks into rbuf  -*/
                       /******************************************************/
        save_dlen = pr->dlen;
        rc_ptr = 0;                          /*- init read/compare pointer --*/
        pr->dlen = pr->bytpsec;      /*-- read 1 block at a time into rbuf --*/
        while ( rc_ptr <= pr->num_blks ) {
           /*read_cdrom(ps, pr, loop, blkno, tbuf);*/
           if( ( read_res = read_cdrom(ps, pr, loop, blkno, tbuf) ) != 0 ) /* read un-succesfull ?*/
           		break;

           for ( i = 0; i <= pr->dlen; i++ )
               rbuf[(pr->dlen*rc_ptr)+i] = tbuf[i];
           set_blkno(blkno, pr->direction, pr->increment, 1);
           rc_ptr++;
        }
        if( read_res == 0 ) {/* only if all reads are successful ... */
			pr->dlen = save_dlen;
			cmpbuf(ps, pr, loop, blkno_save, wbuf, rbuf);
		} else { /* read fail */
			 sprintf(msg, "read_cdrom() failed in Read sequence of RC oper\n"
						  "Skipping compare operation\n");
			 prt_msg(ps, pr, loop, blkno, 0, SOFT, msg);
		}

    } else if ( strcmp(pr->oper, "D") == 0 ) {
        diag_cdrom(ps, pr, loop, blkno);
    } else if ( strcmp(pr->oper, "A") == 0 ) {
      #ifndef __HTX_LINUX__
        audio_cdrom(ps, pr, loop, blkno);
      #endif
    } else if ( strcmp(pr->oper, "AMM") == 0 ) {
		if ( (0 != strncmp(device_subclass, "sata", 16)) && (0 != strncmp(device_subclass, "usbif", 16)) ) {
			/* PLAYUDIO MSF is not supported for SATA drives */
        	audio_mm(ps, pr, loop, blkno);
		}
		else {
			sprintf(msg, "Skipping AMM stanza for SATA/USB drive, rule_id = %s\n", pr->rule_id);
       		hxfmsg(ps, 0, INFO, msg);
		}
    } else if ( strcmp(pr->oper, "RS") == 0 ) {
        prt_req_sense(ps, pr, loop, blkno);
    } else if ( strcmp(pr->oper, "S") == 0 ) {
        do_sleep(ps, pr, loop, blkno);
    } else if ( strcmp(pr->oper, "XCMD") == 0 ) {
        rc = do_cmd(ps, pr);
    } else {
        ;
    }

    hxfupdate(UPDATE, ps);
    if ( ps->run_type[0] == 'O' ) {
       info_msg(ps, pr, loop, blkno, msg);
       hxfmsg(ps, 0, INFO, msg);
    }
    if ( strcmp(pr->type_length, "RANDOM") == 0 && /* set lgth of data trans */
         strcmp(pr->addr_type, "RANDOM") == 0 )    /* if random is specified */
       pr->dlen = random_dlen(pr->bytpsec, pr->tot_blks, seed);
    if ( strcmp(pr->addr_type, "RANDOM") == 0 ) /* set block # for next oper */
       random_blkno(blkno, pr->dlen, pr->bytpsec, pr->max_blkno, seed,
                    pr->min_blkno);
    else if ( (strcmp(pr->oper, "RC") != 0 ) /* set blkno if not RC or RWP */
		      && ( strcmp(pr->oper, "RWP") != 0 ))
       set_blkno(blkno, pr->direction, pr->increment, pr->num_blks);
    if ( strcmp(pr->addr_type, "SEQ") == 0 ) {
       rc = wrap(pr, blkno);
       if ( rc == 1 )
          init_blkno(pr,blkno);
    }
  }
  free(tbuf_malloc);
  return(0);
}
