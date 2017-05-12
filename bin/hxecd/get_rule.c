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

/* @(#)29       1.14  src/htx/usr/lpp/htx/bin/hxecd/get_rule.c, exer_cd, htxubuntu 5/24/04 17:15:24 */

/******************************************************************************
 *   COMPONENT_NAME: exer_cd
 *
 *   MODULE NAME: get_rule.c
 *
 *   FUNCTIONS: get_rule
 *              htx_getline
 *              numeric
 *              set_defaults
 *
 *   DESCRIPTION: Get a rule stanza from the rules file.
 *                For each keyword parameter of the rule stanza:
 *                  1. assign default value to corresponding program variable.
 *                  2. assign specified value to corresponding program variable.
 *                  3. validity check each variable.
 *                Return codes =  0 valid stanza
 *                             =  1 invalid stanza
 *                             = -1 EOF
 ******************************************************************************/
#include "hxecd.h"

extern      FILE *fptr;
extern int  cnt, crash_on_mis, rule_stanza[99];
extern char signal_flag, pipe_name[15], rules_file_name[100];

int get_rule(struct htx_data *ps, struct ruleinfo *pr)
{
  int        i, j, k, x, rc, max, first_line, keywd_count;
  int        c_1, c_2, found;
  static int line;
  char       s[200], t[200], msg[221], keywd[80];
  char       digit[3] = "   ", error, pipe_swtch[3], sSS[10];
  char       sMM[10], sBB[10], msb[40], sMODE[10];

  line = 0;
  set_defaults(pr);
  keywd_count = 0;
  error = 'n';
  first_line = line + 1;
  while ( (htx_getline(s, 200)) > 1 ) {
     line = line + 1;
     if ( s[0] == '*' )
        continue;
     for ( i = 0; s[i] != '\n'; i++ ) {
         t[i] = s[i];
         s[i] = toupper(s[i]);
         if ( s[i] == '=' ) {
            s[i] = ' ';
            t[i] = ' ';
         }
     }
     keywd_count++;
     sscanf(s, "%s", keywd);
     if ( (strcmp(keywd, "DISC_PN")) == 0) { /* DISC_PN is used for rules and */
                                             /* cdrom test disc compatibility */
        sscanf(s, "%*s %s", pr->cds.rule_disc_pn);
        if ( (strlen(pr->cds.rule_disc_pn)) > 20 ) {
           sprintf(msg, "line# %d %s = %s (must be 20 characters or less) \n",
                   line, keywd, pr->cds.rule_disc_pn);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "RULE_ID")) == 0 ) {
        sscanf(s, "%*s %s", pr->rule_id);
        if ( (strlen(pr->rule_id)) > 8 ) {
           sprintf(msg, "line# %d %s = %s (must be 8 characters or less) \n",
                   line, keywd, pr->rule_id);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        } else {
           j = strlen(pr->rule_id);
           i = 0;
           found = 0;
           for ( k = 0; k < j; k++ ) {
              if ( isdigit(pr->rule_id[k]) ) {
                 digit[i] = pr->rule_id[k];
                 i++;
                 found = 1;
              }
           }
           if ( found ) {
              rule_stanza[cnt] = atoi(digit);
              for ( i = 0; i < cnt; i++ ) {
                 if ( rule_stanza[cnt] == rule_stanza[i] ) {
                    sprintf(msg, "line# %d %s = %s (Duplicate Stanza "
                                 "number found in RULE_ID) \n",
                            line, keywd, pr->rule_id);
                    hxfmsg(ps, 0, SYSERR, msg);
                    error = 'y';
                 }
              }
           } else {
              sprintf(msg, "line# %d %s = %s (NO Stanza number found "
                           "in RULE_ID) \n",
                      line, keywd, pr->rule_id);
              hxfmsg(ps, 0, SYSERR, msg);
              error = 'y';
           }
        }
     } else if ( (strcmp(keywd, "MIN_BLKNO")) == 0 ) {
        sscanf(s, "%*s %ld", &pr->min_blkno);
        if ( pr->min_blkno < 0 || pr->min_blkno > pr->tot_blks ) {
           sprintf(msg, "line# %d %s = %ld (must be > 0 and <= %ld) \n",
                   line, keywd, pr->min_blkno, pr->tot_blks);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "MAX_BLKNO")) == 0 ) {
        sscanf(s, "%*s %ld", &pr->max_blkno);
        if ( pr->max_blkno < 0 || pr->max_blkno > pr->tot_blks ) {
           sprintf(msg, "line# %d %s = %ld (must be > 0 and <= %ld) \n",
                   line, keywd, pr->max_blkno, pr->tot_blks);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
                    /*-- check for pattern id for read compare rule stanza --*/
     } else if ( (strcmp(keywd, "PATTERN_ID")) == 0 ) {
        sscanf(s, "%*s %s", pr->pattern_id);
        if ( (strlen(pr->pattern_id)) > 8 ) {
           sprintf(msg, "line# %d %s = %s (must be 8 characters or less) \n",
                   line, keywd, pr->pattern_id);
           hxfmsg(ps,0,SYSERR,msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "RETRIES")) == 0 ) {
        sscanf(s, "%*s %s", pr->retries);
        if ( strcmp(pr->retries, "ON") == 0  ||
             strcmp(pr->retries, "OFF") == 0 ||
             strcmp(pr->retries,"BOTH") == 0 ) ;
        else {
           sprintf(msg, "line# %d %s = %s (must be ON,OFF, or BOTH) \n",
                   line, keywd, pr->retries);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "ADDR_TYPE")) == 0 ) {
        sscanf(s, "%*s %s", pr->addr_type);
        if ( strcmp(pr->addr_type, "SEQ") == 0 ||
             strcmp(pr->addr_type, "RANDOM") == 0 ) ;
        else {
           sprintf(msg, "line# %d %s = %s (must be SEQ, or RANDOM) \n",
                   line, keywd, pr->addr_type);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "NUM_OPER")) == 0 ) {
        sscanf(s, "%*s %d", &pr->num_oper);
        if ( pr->num_oper < 0 ) {
           sprintf(msg, "line# %d %s = %d (must be >= 0) \n",
                   line, keywd, pr->num_oper);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd,"OPER")) == 0 ) {
        sscanf(s, "%*s %s", pr->oper);
        if ( strcmp(pr->oper, "R") == 0  || strcmp(pr->oper, "D") == 0   ||
             strcmp(pr->oper, "A") == 0  || strcmp(pr->oper, "AMM") == 0 ||
             strcmp(pr->oper, "S") == 0  || strcmp(pr->oper, "RC") == 0  ||
             strcmp(pr->oper, "MS") == 0 || strcmp(pr->oper, "RWP") == 0 ||
             strcmp(pr->oper, "RS") == 0 || strcmp(pr->oper, "RRC") == 0 ||
             strcmp(pr->oper, "XCMD") == 0 ) ;
        else {
           sprintf(msg, "line# %d %s = %s must be \nD, R, A, AMM, RS, S, "
                        "RC, RWP, RRC, MS, or XCMD \n",
                   line, keywd, pr->oper);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "STARTING_BLOCK")) == 0 ) {
        strcpy(sMM, "\0");
        strcpy(sSS, "\0");
        strcpy(sBB, "\0");
        sscanf(s, "%*s %s", msb);
        strcpy(pr->starting_block, msb);
        if ( strrchr(msb, ':') > 0 ) {    /* check for mm:ss:bb string first */
           c_1 = 0;
           c_2 = 0;
           for ( i = 0; i < strlen(msb); i++ ) {
              if ( msb[i] == ':' ) {
                 if ( c_1 == 0 )
                    c_1 = i;
                 else if ( c_2 == 0 )
                    c_2 = i;
              }
           }
           if ( (c_1 == 2) && (c_2 == 5) ) {
              i = 0;
              while ( i < c_1 ) {
                 sMM[i] = msb[i];
                 i++;
              }
              sMM[i] = '\0';
              x = 0;
              i = c_1 + 1;
              while ( i < c_2 ) {
                 sSS[x] = msb[i];
                 i++;
                 x++;
              }
              sSS[x] = '\0';
              x = 0;
              i = c_2 + 1;
              while ( i <= strlen(msb) ) {
                 sBB[x] = msb[i];
                 i++;
                 x++;
              }
              sBB[x] = '\0';
              pr->first_block = (atoi(sMM) * 4500) + (atoi(sSS) * 75) +
                                (atoi(sBB)-150);
              strcpy(pr->starting_block, "\0");
              sprintf(pr->starting_block, "%d", pr->first_block);
              pr->msf_mode = CD_MSF_MODE;  /* minutes,seconds,frames lba mode */
           } else {                        /* else invalid format             */
              sprintf(msg, "line# %d Invalid %s format = %s. \n",
                      line, keywd, pr->starting_block);
              hxfmsg(ps, 0, SYSERR, msg);
              error = 'y';
           }
        } else
           pr->first_block = set_first_blk(pr);
        if ( pr->first_block < pr->min_blkno ||
             pr->first_block >= pr->max_blkno) {
           sprintf(msg, "line# %d %s = %s (must be >= 0 and < %ld) \n",
                   line, keywd, pr->starting_block, pr->max_blkno);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "DIRECTION")) == 0 ) {
        sscanf(s, "%*s %s", pr->direction);
        if ( strcmp(pr->direction, "UP") == 0   ||
             strcmp(pr->direction, "DOWN") == 0 ||
             strcmp(pr->direction, "OUT") == 0  ||
             strcmp(pr->direction, "IN") == 0 ) ;
        else {
           sprintf(msg, "line# %d %s = %s (must be UP,DOWN,IN, or OUT) \n",
                   line, keywd, pr->direction);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "INCREMENT")) == 0 ) {
        sscanf(s, "%*s %d", &pr->increment);
        if ( pr->increment < 0 ) {
           sprintf(msg, "line# %d %s = %d (must be >= 0) \n",
                   line, keywd, pr->increment);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "TYPE_LENGTH")) == 0 ) {
        sscanf(s, "%*s %s", pr->type_length);
        if ( strcmp(pr->type_length, "FIXED") == 0 ||
             strcmp(pr->type_length, "RANDOM") == 0 ) ;
        else {
           sprintf(msg, "line# %d %s = %s (must be FIXED, or RANDOM) \n",
                   line, keywd, pr->type_length);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "NUM_BLKS")) == 0 ) {
        sscanf(s, "%*s %d", &pr->num_blks);
		/* ignore buffer size constraints for AMM operations, buffers are  */
		/* not required.                                                   */
		if ( (strcmp(pr->oper, "AMM") == 0) )
			max = pr->num_blks;
		  else
			max = BUF_SIZE / pr->bytpsec;
        if ( pr->num_blks < 1 || pr->num_blks > max ) {
           sprintf(msg, "line# %d %s = %d (must be > 1 and <= %d) \n",
                   line, keywd, pr->num_blks, max);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "MODE")) == 0 ) {
        strcpy(sMODE, " ");
        sscanf(s, "%*s %s", sMODE);
        strcpy(pr->mode, sMODE);      /* use pattern_id var for mode select */
        if ( (strcmp(sMODE, "M1") == 0)   ||
             (strcmp(sMODE, "M2F1") == 0) ||
             (strcmp(sMODE, "M2F2") == 0) ||
             (strcmp(sMODE, "DA") == 0) )
           strcpy(pr->mode, sMODE);
        else {
           sprintf(msg, "line# %d Invalid Mode Select Requested - %s) \n",
                   line, sMODE);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "#CLOSE_PIPE")) == 0 ) {
        sscanf(s, "%*s %s", pipe_swtch);
        if ( strcmp(pipe_swtch, "NO") == 0 )
           signal_flag = 'R';
        else {
           sprintf(msg, "rm %s", pipe_name);
           system(msg);
           sprintf(msg, "line # %d %s = %s ( must be NO ) \n",
                   line, keywd, pipe_swtch);
           hxfmsg(ps, 0, SYSERR, msg);
           error = 'y';
        }
     } else if ( (strcmp(keywd, "#CRASH_ON_MIS")) == 0 ) {
        sscanf(s, "%*s %s", msg);
        crash_on_mis = (*msg == 'Y') ? 1 : 0;
     } else if ( (strcmp(keywd, "#RULES_NAME")) == 0 ) {
        sscanf(t, "%*s %s", rules_file_name);
        sprintf(msg, "Using New Rules File > %s", rules_file_name);
        hxfmsg(ps, 0, INFO, msg);
     } else if ( (strcmp(keywd, "CRASH_ON_MIS")) == 0 ) {
        sscanf(s, "%*s %s", msg);
        crash_on_mis = (*msg == 'Y') ? 1 : 0;
     } else if ( (strcmp(keywd, "COMMAND")) == 0 ) {
        j = 0;
        for ( i = 10; s[i] != '\n' && s[i] != '\0'; i++, j++ ) {
            pr->cmd_list[j] = t[i];
        }
        pr->cmd_list[j] = '\0';
     } else {
        sprintf(msg, "line# %d keywd = %s (invalid) \n",
                line, keywd);
        hxfmsg(ps, 0, SYSERR, msg);
        error = 'y';
     }
  }

  line = line + 1;
  if ( keywd_count > 0 ) {
     if ( (strcmp(pr->rule_id, "        ") == 0) &&
           (signal_flag != 'X') && (signal_flag != 'R') ) {
        sprintf(msg, "line# %d rule_id not specified \n",
                first_line);
        hxfmsg(ps, 0, SYSERR, msg);
        error = 'y';
     } else
        rc = 0;
  } else {
     rc = EOF;
     line = 0;
  }
  if ( error == 'y' )
     rc = 1;
  return(rc);
}

/*****************************************************************************/
/* This routine sets defaults                                                */
/*****************************************************************************/
void set_defaults(struct ruleinfo *pr)
{
                    /************************************************/
                    /* DON'T do a memset to clear this structure as */
                    /* parts of it are used across different rules. */
                    /************************************************/
   pr->op_in_progress = 0;      /* init to undefined op in progress */
   pr->min_blkno = 0;
   pr->max_blkno = pr->tot_blks;
   strcpy(pr->rule_id, "        ");
   strcpy(pr->pattern_id, "    ");
   strcpy(pr->retries, "ON");
   strcpy(pr->addr_type, "SEQ");
   pr->num_oper = 1;
   strcpy(pr->oper, "R");
   strcpy(pr->starting_block, "BOT");
   strcpy(pr->direction, "UP");
   pr->num_blks = 1;
   strcpy(pr->type_length, "FIXED");
   pr->increment = 0;
   pr->first_block = 0;     /* starting block converted to numeric  */
   pr->dlen = 0;            /* length of data to read in bytes      */
   pr->msf_mode = 0;        /* true = mm:ss:ff format for lba       */
                            /* false = use track/block number       */
}

/*****************************************************************************/
/* This routine reads a line from "rules" into the specified string.  It     */
/* returns the length of the string. If the length is 1 the line is blank.   */
/* When EOF is encountered, the length returned is 0.                        */
/*****************************************************************************/
int htx_getline(char *s, int lim)
{
  s[0] = '\0';
  if ( fgets(s, lim, fptr) != NULL )
     return(strlen(s));
  else
     return(0);
}

/**************************************************************************/
/* check if specified string is numeric                                   */
/**************************************************************************/
int numeric(char *s)
{
  int num;
  int i;

  num = 0;
  for ( i = 0; s[i] != '\0'; i++ )
     if ( (isdigit(s[i])) == 0 )
        num = 1;
  return(num);
}
