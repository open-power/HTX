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
 * MODULE NAME: get_rule.c
 *
 * FUNCTIONS : get_rule() get a rule from rules file.
 *             set_defaults() sets default values into ruleinfo struct.
 *             get_line() used to read a line of data from stdin.
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef __HTX_LINUX__
#include "hxetape.h"
#else
#include <hxetape.h>
#endif

int        VBS_last_write;
extern int VBS_seed_initialized, cnt, rule_stanza[250];
extern int          crash_on_mis;
extern char signal_flag;
extern char pipe_name[15];
extern FILE *fptr;

/****************************************************************************/
/* get_rule - for each keyword parameter of the rule stanza:                */
/* for each keyword parameter of the rule stanza:                           */
/*    1. assign default value to corresponding program variable.            */
/*    2. assign specified value to corresponding program variable.          */
/*    3. validity check each variable.                                      */
/* return code =  0 valid stanza                                            */
/*             =  1 invalid stanza                                          */
/*             = -1 EOF                                                     */
/****************************************************************************/
int
get_rule(struct htx_data *phtx_info, struct ruleinfo *prule_info,
         char *rules_name)
{
  int          keywd_count, rc=0, i, d, k, found;
  int          first_line;
  char         *p, digit[3] = "   ", keywd[80], pipe_swtch[3], t[200];
  char         error, s[200], msg[221], tmp_msg[100];
  static int   line = 0;

    set_defaults(prule_info);
    keywd_count = 0;
    cnt = 0;
    error = 'n';
    first_line = line + 1;
    while ( (get_line(s, 200)) > 1 ) {     /* read in & process the next line */
      line = line + 1;
      if ( s[0] == '*' )
         continue;
      for ( i = 0; s[i] != '\n'; i++ ) {
         t[i] = s[i];
         s[i] = toupper(s[i]);
         if (s[i] == '=')
            s[i] = ' ';
      }
      keywd_count++;
      sscanf(s, "%s", keywd);
      if ( (strcmp(keywd, "RULE_ID")) == 0 ) {            /* Valid RULE_ID ? */
         sscanf(s, "%*s %s", prule_info->rule_id);
         if ( (strlen(prule_info->rule_id)) > 8 ) {
            sprintf(msg, "line# %d %s = %s (must be 8 characters or less) \n",
                    line, keywd, prule_info->rule_id);
            hxfmsg(phtx_info, 0, SYSERR, msg);
            error = 'y';
         } else {
            d = strlen(prule_info->rule_id);
            i = 0;
            found = 0;
            for ( k = 0; k < d; k++ ) {
               if ( isdigit(prule_info->rule_id[k]) ) {
                  digit[i] = prule_info->rule_id[k];
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
                              line, keywd, prule_info->rule_id);
                     hxfmsg(phtx_info, 0, SYSERR, msg);
                     error = 'y';
                  }
               }
            } else {
               sprintf(msg, "line# %d %s = %s (NO Stanza number found "
                            "in RULE_ID) \n",
                       line, keywd, prule_info->rule_id);
               hxfmsg(phtx_info, 0, SYSERR, msg);
               error = 'y';
            }
         }
      } else if ( (strcmp(keywd, "PATTERN_ID")) == 0 ) {/* Valid PATTERN_ID ? */
         sscanf(s, "%*s %s", prule_info->pattern_id);
         if ( (strlen(prule_info->pattern_id)) > 8 ) {
            sprintf(msg, "line# %d %s = %s (must be 8 characters or less) \n",
                    line, keywd, prule_info->pattern_id);
            hxfmsg(phtx_info, 0, SYSERR, msg);
            error = 'y';
         }
      } else if ( (strcmp(keywd, "NUM_OPER")) == 0 ) {   /* Valid NUM_OPER ? */
         sscanf(s, "%*s %d", &prule_info->num_oper);
         if ( prule_info->num_oper < 0 ) {
            sprintf(msg, "line# %d %s = %d (must be >= 0) \n",
                    line, keywd, prule_info->num_oper);
            hxfmsg(phtx_info, 0, SYSERR, msg);
            error = 'y';
         }
      } else if ( (strcmp(keywd, "OPER")) == 0 ) {           /* Valid OPER ? */
         sscanf(s, "%*s %s", prule_info->oper);
         if ( strcmp(prule_info->oper, "W") == 0     ||
              strcmp(prule_info->oper, "R") == 0     ||
              strcmp(prule_info->oper, "RC") == 0    ||
              strcmp(prule_info->oper, "D") == 0     ||
              strcmp(prule_info->oper, "RW") == 0    ||
              strcmp(prule_info->oper, "WEOF") == 0  ||
              strcmp(prule_info->oper, "S") == 0     ||
              strcmp(prule_info->oper, "E") == 0     ||
              strcmp(prule_info->oper, "SF") == 0    ||
              strcmp(prule_info->oper, "SR") == 0    ||
              strcmp(prule_info->oper, "CO") == 0    ||
              strcmp(prule_info->oper, "WEOT") == 0  ||
              strcmp(prule_info->oper, "C") == 0     ||
              strcmp(prule_info->oper, "O") == 0     ||
              strcmp(prule_info->oper, "RCEOT") == 0 ||
              strcmp(prule_info->oper, "REOT") == 0  ||          /* dgs */
              strcmp(prule_info->oper, "RS") == 0    ||
#ifndef __HTX_LINUX__
              strcmp(prule_info->oper, "DBUG") == 0  ||
              strcmp(prule_info->oper, "IE") == 0    ||
              strcmp(prule_info->oper, "RES") == 0   ||
              strcmp(prule_info->oper, "ML") == 0    ||
              strcmp(prule_info->oper, "MUL") == 0   ||
              strcmp(prule_info->oper, "ASF") == 0   ||
              strcmp(prule_info->oper, "ASR") == 0   ||
              strcmp(prule_info->oper, "LB") == 0    ||
              strcmp(prule_info->oper, "RP") == 0    ||
              strcmp(prule_info->oper, "ADUL") == 0  ||
              strcmp(prule_info->oper, "TWIE") == 0  ||
              strcmp(prule_info->oper, "TWPE") == 0  ||
              strcmp(prule_info->oper, "TWRE") == 0  ||
              strcmp(prule_info->oper, "TWUL") == 0  ||
              strcmp(prule_info->oper, "WUL") == 0   ||
              strcmp(prule_info->oper, "TWMM") == 0  ||
              strcmp(prule_info->oper, "CDRE") == 0  ||
              strcmp(prule_info->oper, "CDMM") == 0  ||
              strcmp(prule_info->oper, "HUNL") == 0  ||
              strcmp(prule_info->oper, "HINI") == 0  ||
              strcmp(prule_info->oper, "HREL") == 0  ||
              strcmp(prule_info->oper, "HWUN") == 0  ||
	          strcmp(prule_info->oper, "SPEC") == 0  ||
#endif
              strcmp(prule_info->oper, "VBSWR") == 0 ||
              strcmp(prule_info->oper, "VBSRD") == 0 ||
              strcmp(prule_info->oper, "VBSRDC") == 0||
                   strcmp(prule_info->oper, "XCMD") == 0 ) ;
         else {
           sprintf(msg, "ERROR in rules file %s:\n", rules_name);
           hxfmsg(phtx_info, 0, INFO, msg);
           sprintf(msg, " Line# %d %s = %s is an INVALID operation.\n",
                   line, keywd, prule_info->oper);
           hxfmsg(phtx_info, 0, INFO, msg);
#ifndef __HTX_LINUX__
           sprintf(msg, " Valid Operations are W,R,RC,D,RW,WEOF,S,E,SF,SR,CO"
                        "WEOT,C,O,RCEOT,\nRS,DBUG,IE,RES,ML,MUL,ASF,ASR,LB"
                        "RP,ADUL,TWIE,TWPE,TWRE,TWUL,\nWUL,TWMM,VBSWR,VBSRD, VBSRDC"
                        "SPEC, XCMD.\n");
#else
		   sprintf(msg, " Valid Operations are W,R,RC,D,RW,WEOF,S,E,SF,SR,CO"
						"WEOT,C,O,RCEOT,\nRS,VBSWR,VBSRD, VBSRDC, XCMD.\n");
#endif
           hxfmsg(phtx_info, 0, SYSERR, msg);
           error = 'y';
         }
      } else if ( (strcmp(keywd, "NUM_BLKS")) == 0 ) {
         sscanf(s, "%*s %d", &prule_info->num_blks);
         if ( strcmp(prule_info->oper,"SF") == 0 ||
              strcmp(prule_info->oper,"SR") == 0 )
            prule_info->pattern_id[0] = 0;
      } else if ( (strcmp(keywd, "#CLOSE_PIPE")) == 0 ) {
         sscanf(s, "%*s %s", pipe_swtch);
         if ( strcmp(pipe_swtch, "NO") == 0 )
            signal_flag = 'R';
         else {
            sprintf(msg, "rm %s", pipe_name);
            system(msg);
            sprintf(msg, "line# %d %s = %s (must be NO) \n",
                    line, keywd, pipe_swtch);
            hxfmsg(phtx_info, 0, SYSERR, msg);
            error = 'y';
         }
      } else if ((strcmp(keywd, "#CRASH_ON_MIS")) == 0) {
         char dummy[30];
		 if(crash_on_mis == 0) {
	         sscanf(s, "%*s %s", dummy);
    	     if      ( strcmp(dummy, "YES") == 0 ) crash_on_mis = 1;
        	 else if ( strcmp(dummy, "NO")  == 0 ) crash_on_mis = 0;
	         else {
    	        sprintf(msg, "rm %s", pipe_name);
        	    system(msg);
            	sprintf(msg,"line# %d %s = %s (must be YES or NO) \n",
                	    line, keywd, dummy);
	            hxfmsg(phtx_info, 0, SYSERR, msg);
    	        error = 'y';
        	 }
		 }
      } else if ( (strcmp(keywd, "#RULES_NAME")) == 0 ) {
         sscanf(s, "%*s %s", rules_name);
         sprintf(msg, "Using New Rules File > %s", rules_name);
         hxfmsg(phtx_info, 0, INFO, msg);
      } else if ( (strcmp(keywd, "UNLOAD_SLEEP")) == 0 ) {
         sscanf(s, "%*s %d", &prule_info->u_sleep);
      } else if ( (strcmp(keywd, "SOURCE_ID")) == 0 ) {
         sscanf(s, "%*s %d", &prule_info->source_id);
      } else if ( (strcmp(keywd, "DEST_ID")) == 0 ) {
         sscanf(s, "%*s %d", &prule_info->dest_id);
      } else if ( (strcmp(keywd, "CHS_FILE")) == 0 ) {
          for ( d = 0; s[d] != '\n'; d++ )
             s[d] = tolower(s[d]);
          sscanf(s, "%*s %s", prule_info->chs_file);
      } else if ( (strcmp(keywd, "SOURCE_ID1")) == 0 ) {
         sscanf(s, "%*s %d", &prule_info->source_id1);
      } else if ( (strcmp(keywd, "DEST_ID1")) == 0 ) {
         sscanf(s, "%*s %d", &prule_info->dest_id1);
      } else if ( strcmp(keywd, "CRASH_ON_MIS") == 0 ) {
         sscanf(s, "%*s %s", msg);
         crash_on_mis = (*msg == 'Y') ? 1 : 0;
      } else if ( strcmp(keywd, "VBS_SEED") == 0 ) {
         sscanf(s, "%*s %n", &i);
         while ( isspace(s[i]) ) i++;
         strcpy(msg, &s[i]);
         if ( strncmp(prule_info->oper, "VBS", 3) != 0 ) {
            sprintf(msg, "line# %d %s = %s (Only valid in VBSRD or VBSWR)",
                    line, keywd, msg);
            hxfmsg(phtx_info, 0, SYSERR, msg);
            error = 'y';
         } else if ( strncmp(msg, "RANDOM", 6) == 0 ) {
            init_seed(prule_info->seed);
            prule_info->VBS_seed_type = 'R';
            VBS_seed_initialized = 1;
         } else if ( strncmp(msg, "FIXED", 5) == 0 ) {
            VBS_seed_initialized = 1;
            prule_info->VBS_seed_type = 'F';
         } else {
            p = msg;
            i = 0;
            while ( *p && i < 4 )                   /* scan for first digit */
               if ( isdigit(*p) ) {
                  if ( i < 3 )
                     prule_info->seed[i++] = atoi(p);  /* convert the arg   */
                  else {
                     VBS_last_write = atoi(p);         /* convert the arg   */
                     i++;
                  }
                  while ( isdigit(*(++p)) ) ;          /* scan past the arg */
               } else
                  p++;
            if ( i != 4 ) {
               sprintf(tmp_msg,
                       "line# %d %s = %s (must be RANDOM or 4 integers)"
                       "/nThe last integer being the # of bytes in the last"
                       "Variable Write command",
                       line, keywd, msg);
               hxfmsg(phtx_info, 0, SYSERR, tmp_msg);
               error = 'y';
            } else {
               VBS_seed_initialized = 1;
               prule_info->VBS_seed_type = 'U';
            }
         }
      } else if ( (strcmp(keywd, "COMMAND")) == 0 ) {
         d = 0;
         for ( i = 10; s[i] != '\n' && s[i] != '\0'; i++, d++ ) {
             prule_info->cmd_list[d] = t[i];
         }
         prule_info->cmd_list[d] = '\0';
      } else {
         sprintf(msg, "line# %d keywd = %s (invalid) \n",
                 line, keywd);
         hxfmsg(phtx_info, 0, SYSERR, msg);
         error = 'y';
      }
    }

    line = line + 1;
    if ( keywd_count > 0 ) {
       if ( (htx_strcmp(prule_info->rule_id, "        ") == 0) &&
            (signal_flag != 'X') && (signal_flag != 'R') ) {
          sprintf(msg, "line# %d rule_id not specified \n",
                  first_line);
          hxfmsg(phtx_info, 0, SYSERR, msg);
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
/* set_defaults() - This routine sets the default values                     */
/*****************************************************************************/
void
set_defaults(struct ruleinfo *prule_info)
{
   strcpy(prule_info->rule_id, "        ");
   strcpy(prule_info->pattern_id, "#001");
   prule_info->num_oper = 1;
   strcpy(prule_info->oper, "R");
   prule_info->num_blks = 1;
   if ( VBS_seed_initialized )
      prule_info->VBS_seed_type = 'L'; /* default to last specified seed */
   else
      prule_info->VBS_seed_type = '?'; /* else seed is unspecified       */
}

/*****************************************************************************/
/* get_line - This routine reads a line from "rules" into the specified       */
/* string.  It returns the length of the string. If the length is 1 the line */
/* is blank. When EOF is encountered, the length returned is 0.              */
/*****************************************************************************/
int
get_line(char s[], int lim)
{
  s[0] = '\0';
  if ( fgets(s, lim, fptr) != NULL )
     return(strlen(s));
  else
     return(0);
}
