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

//   FUNCTIONS: SanityCkBaud
//		SetHwDefaults
//		SetStanzaDefaults1448
//		get_rule
//		get_line

#include"headers.h"

/*
 * Static functions
 */
static int  get_line(FILE *fd, char *s, int lim);
static void SetStanzaDefaults(RULE_T * r_ptr);
static void SetHwDefaults(RULE_T * r_ptr);


/****************************************************************************/
/*****  g e t _ r u l e ( )  ************************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     get_rule()                                           */
/*                                                                          */
/* DESCRIPTIVE NAME =  Get rule from rules file.                            */
/*                                                                          */
/* FUNCTION =          Reads a rule from the rules file, checks each        */
/*                     keyword value, and assigns value to rules data       */
/*                     structure.                                           */
/*                                                                          */
/* INPUT =             line - pointer to integer containing line number in  */
/*                            number in file where to begin rule            */
/*                            processing.                                   */
/*                     r_ptr - pointer to rules data structure.             */
/*                     pattern_nm - string array for name of pattern file.  */
/*                                                                          */
/* OUTPUT =            Integer pointed to by line arguement is updated to   */
/*                            value of line number at the end of the        */
/*                            current rule.                                 */
/*                     Updated rules data structure.                        */
/*                     pattern_nm array set to contain the name of the      */
/*                            specified pattern file.                       */
/*                                                                          */
/* NORMAL RETURN =     0 to indicate successful completion.                 */
/*                     -1 to indicate EOF.                                  */
/*                                                                          */
/* ERROR RETURNS =     1 - Invalid rule.                                    */
/*                                                                          */
/* EXTERNAL REFERENCES                                                      */
/*                                                                          */
/*    OTHER ROUTINES = hxsmsg() - updates data for supervisor and sends a   */
/*                                message to the message handler program    */
/*                                (hxsmsg).                                 */
/*                                                                          */
/****************************************************************************/

int get_rule(   FILE *fd, 
				int *line, 
				RULE_T * r_ptr,
				ARGS_T *ArgDataPtr)
{
   char            error;       /* error flag                       */
   char            keywd[80];   /* keyword buffer                   */
   char            s[200];      /* rules file line buffer           */
   char            ss[5][80];   /* buffer for operands              */
   int             first_line;  /* line # of 1st stanza keyword     */
   int             i;           /* loop counter                     */
   int             keywd_count; /* keyword count                    */
   int             rc;          /* return code                      */
   int             NoItems;     /* Number items read by sscanf      */
   int             LoadDefFlag; /* Only load defaults once ea stanza*/
   // get link to htx for messages  
   Cmsg *ToHtx=(Cmsg *)ArgDataPtr->MainPtr;


/****************************************************************************/
/* The hardware parameters can only be set once.  If multiple HW values are */
/* in the rule file, the last value will be used.                           */
/****************************************************************************/
   if(*line == 0) {
      SetHwDefaults(r_ptr);
   }
   LoadDefFlag = 1;
   keywd_count = 0;
   error = 'n';
   first_line = *line + 1;
   while ((get_line(fd, s, 200)) > 1) {
      /* We don't want to disturb settings on a  blank line         */
      if(LoadDefFlag) {
		 LoadDefFlag = 0;
		 SetStanzaDefaults(r_ptr);
      }
      *line = *line + 1;
      if (s[0] == '*')
	  continue;
      for (i = 0; s[i] != '\n'; i++) {
		 s[i] = (char) toupper((int) s[i]);
		 if (s[i] == '=')
		 s[i] = ' ';
      }                         /* endfor */
      keywd_count++;
      (void) sscanf(s, "%s", keywd);
      if ((strcmp(keywd, "RULE_ID")) == 0) {
		 (void) sscanf(s, "%*s %s", r_ptr->rule_id);
		 if ((strlen(r_ptr->rule_id)) > 8) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %s (must be 8 characters or less)\n",
				   *line, keywd, r_ptr->rule_id);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }                      /* endif */
      } else if ((strcmp(keywd, "PATTERN_ID")) == 0) {
		 (void) sscanf(s, "%*s %s", r_ptr->pattern_id);
		 if ((strlen(r_ptr->pattern_id)) > 8) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %s (must be 8 characters or less) \n",
				   *line, keywd, r_ptr->pattern_id);
			ToHtx->SendMsg( 1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }
      } else if ((strcmp(keywd, "SLEWRATE")) == 0) {
		 (void) sscanf(s, "%*s %s", r_ptr->slewrate);
		/* only 3 valid values..... SLOW, MEDIUM and FAST */
		 if ( (strcmp(r_ptr->slewrate,"SLOW")!=0) && 
			   (strcmp(r_ptr->slewrate,"MEDIUM")!=0) && 
			(strcmp(r_ptr->slewrate,"FAST")!=0) ) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %s (must be slow, medium or fast) \n",
				   *line, keywd, r_ptr->slewrate);
			ToHtx->SendMsg( 1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }
      } else if ((strcmp(keywd, "NUM_OPER")) == 0) {
		 (void) sscanf(s, "%*s %d", &(r_ptr->num_oper));
		 if (r_ptr->num_oper < 1) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %d (must be 1 or greater) \n",
				   *line, keywd, r_ptr->num_oper);
			ToHtx->SendMsg( 1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }                      /* endif */
      } else if ((strcmp(keywd, "BUFSIZE")) == 0) {
		 (void) sscanf(s, "%*s %d", &(r_ptr->bufsize));
		 if (r_ptr->bufsize < 1) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %d (must be 1 or greater) \n",
				   *line, keywd, r_ptr->bufsize);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }                      /* endif */
      } else if ((strcmp(keywd, "IOCTL_SLEEP")) == 0) {
		 NoItems = sscanf(s, "%*s %d", &(r_ptr->ioctl_sleep));
		 if (NoItems != 1 || r_ptr->ioctl_sleep <= 0) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s (%s must be a number > 0)  \n",
				   *line, s, keywd);
			ToHtx->SendMsg( 1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }                      /* endif */
      } else if ((strcmp(keywd, "WRITE_SLEEP")) == 0) {
		 NoItems = sscanf(s, "%*s %d", &(r_ptr->write_sleep));
		 if (NoItems != 1 || r_ptr->write_sleep <= 0) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s (%s must be a number > 0)  \n",
				   *line, s, keywd);
			ToHtx->SendMsg( 1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }                      /* endif */
      } else if (strcmp(keywd, "SPECIAL") == 0) {
		 for (i = 0; i < 4; i++) r_ptr->special[i] = ss[0][i] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 for (i = 0; i < 4; i++) {
			if (ss[0][i] != '\0') {
			   r_ptr->special[i] = ss[0][i];
			}                   /* endif */
			switch (r_ptr->special[i]) {
			case '\0':
			   break;
			case 'W':
				ArgDataPtr->Flags |= WRITE_ONLY;
	       break;
			default:
			   (void) sprintf(ToHtx->GetMsgAddr(),
					  "line# %d %s = %c (SPECIAL must be  W(write))\n",
					  *line, keywd, r_ptr->special[i]);
			   ToHtx->SendMsg( 100, HTX_HE_HARD_ERROR);
			   error = 'y';
			   break;
			}                   /* endswitch */
		 }                      /* endfor */
	  } else if ((strcmp(keywd, "CLOCAL")) == 0) {
		 ss[0][0] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 if (ss[0][0] != '\0') {
			r_ptr->clocal = ss[0][0];
		 }                      /* endif */
		 switch (r_ptr->clocal) {
		 case '\0':
		 case 'Y':
		 case 'N':
			break;
		 default:
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %c (value must be Y(yes) or N(no))\n",
				   *line, keywd, r_ptr->clocal);
			ToHtx->SendMsg( 1, HTX_HE_SOFT_ERROR);
			error = 'y';
			break;
		 }                      /* endswitch */
      } else if ((strcmp(keywd, "IGN_BRK")) == 0) {
		 ss[0][0] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 if (ss[0][0] != '\0') {
			r_ptr->ign_brk = ss[0][0];
		 }                      /* endif */
		 switch (r_ptr->ign_brk) {
		 case '\0':
		 case 'Y':
		 case 'N':
			break;
		 default:
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %c (value must be Y(yes) or N(no))\n",
				   *line, keywd, r_ptr->ign_brk);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
			break;
		 }                      /* endswitch */
      } else if ((strcmp(keywd, "HUPCL")) == 0) {
		 ss[0][0] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 if (ss[0][0] != '\0') {
			r_ptr->hupcl = ss[0][0];
		 }                      /* endif */
		 switch (r_ptr->hupcl) {
		 case '\0':
		 case 'Y':
		 case 'N':
			break;
		 default:
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %c (value must be Y(yes) or N(no))\n",
				   *line, keywd, r_ptr->hupcl);
			ToHtx->SendMsg( 1, HTX_HE_SOFT_ERROR);
			error = 'y';
			break;
		 }                      /* endswitch */
      } else if ((strcmp(keywd, "BRK_INT")) == 0) {
		 ss[0][0] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 if (ss[0][0] != '\0') {
			r_ptr->brk_int = ss[0][0];
		 }                      /* endif */
		 switch (r_ptr->brk_int) {
		 case '\0':
		 case 'Y':
		 case 'N':
			break;
		 default:
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %c (value must be Y(yes) or N(no))\n",
				   *line, keywd, r_ptr->brk_int);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
			break;
		 }                      /* endswitch */
      } else if ((strcmp(keywd, "IGN_PAR")) == 0) {
		 ss[0][0] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 if (ss[0][0] != '\0') {
			r_ptr->ign_par = ss[0][0];
		 }                      /* endif */
		 switch (r_ptr->ign_par) {
		 case '\0':
		 case 'Y':
		 case 'N':
			break;
		 default:
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %c (value must be Y(yes) or N(no))\n",
				   *line, keywd, r_ptr->ign_par);
			ToHtx->SendMsg( 1, HTX_HE_SOFT_ERROR);
			error = 'y';
			break;
		 }                      /* endswitch */
      } else if ((strcmp(keywd, "PAR_MARK")) == 0) {
		 ss[0][0] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 if (ss[0][0] != '\0') {
			r_ptr->par_mark = ss[0][0];
		 }                      /* endif */
		 switch (r_ptr->par_mark) {
		 case '\0':
		 case 'Y':
		 case 'N':
			break;
		 default:
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %c (value must be Y(yes) or N(no))\n",
				   *line, keywd, r_ptr->par_mark);
			ToHtx->SendMsg( 1, HTX_HE_SOFT_ERROR);
			error = 'y';
			break;
		 }                      /* endswitch */
      } else if ((strcmp(keywd, "IXON")) == 0) {
		 ss[0][0] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 if (ss[0][0] != '\0') {
			r_ptr->ixon = ss[0][0];
		 }                      /* endif */
		 switch (r_ptr->ixon) {
		 case '\0':
		 case 'Y':
		 case 'N':
			break;
		 default:
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %c (value must be Y(yes) or N(no))\n",
				   *line, keywd, r_ptr->ixon);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
			break;
		 }                      /* endswitch */
      } else if ((strcmp(keywd, "IXOFF")) == 0) {
		 ss[0][0] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 if (ss[0][0] != '\0') {
			r_ptr->ixoff = ss[0][0];
		 }                      /* endif */
		 switch (r_ptr->ixoff) {
		 case '\0':
		 case 'Y':
		 case 'N':
			break;
		 default:
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %c (value must be Y(yes) or N(no))\n",
				   *line, keywd, r_ptr->ixoff);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
			break;
		 }                      /* endswitch */
      } else if ((strcmp(keywd, "CBAUD")) == 0) {
		 NoItems = sscanf(s, "%*s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
				   &(r_ptr->cbaud[0]), &(r_ptr->cbaud[1]),
				   &(r_ptr->cbaud[2]), &(r_ptr->cbaud[3]),
				   &(r_ptr->cbaud[4]), &(r_ptr->cbaud[5]),
				   &(r_ptr->cbaud[6]), &(r_ptr->cbaud[7]),
				   &(r_ptr->cbaud[8]), &(r_ptr->cbaud[9]),
				   &(r_ptr->cbaud[10]), &(r_ptr->cbaud[11]),
				   &(r_ptr->cbaud[12]), &(r_ptr->cbaud[13]),
				   &(r_ptr->cbaud[14]), &(r_ptr->cbaud[15]));
		 for (i = 0; i < 16; i++) {
			if(r_ptr->cbaud[i] == -1)
			   continue;
			switch ((r_ptr->cbaud[i] > 0)? r_ptr->cbaud[i] : (- r_ptr->cbaud[i])) {
			   case 50:
			   case 75:
			   case 110:
			   case 134:
			   case 150:
			   case 200:
			   case 300:
			   case 600:
			   case 1200:
			   case 1800:
			   case 2400:
			   case 4800:
			   case 9600:
			   case 19200:
			   case 38400:
			   case 57600:
			   case 115200:
			   case 230000:
			   case 230400:
			   case 460800:
			  break;
			   default:
			  (void) sprintf(ToHtx->GetMsgAddr(),
					  "line# %d %s = %d (not a valid baud rate \
						between 50 & 460800)\n",
					  *line, keywd, r_ptr->cbaud[i]);
			  ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			  error = 'y';
			  break;
			}                   /* endswitch */
		 }                      /* endfor */
      } else if ((strcmp(keywd, "IHOG")) == 0) {
		 NoItems = sscanf(s, "%*s %d", &(r_ptr->ihog));
		 if(NoItems != 1 || r_ptr->ihog <= 512) {
			sprintf(ToHtx->GetMsgAddr(), "line# %d %s = %d (ihog > 512)\n",
					  *line, keywd, r_ptr->ihog);
			ToHtx->SendMsg(1,HTX_HE_SOFT_ERROR);
			error = 'y';
		 }
      } else if ((strcmp(keywd, "CHSIZE")) == 0) {
		 NoItems = sscanf(s, "%*s %d %d %d %d",
				   &(r_ptr->chsize[0]), &(r_ptr->chsize[1]),
				   &(r_ptr->chsize[2]), &(r_ptr->chsize[3]));
		 for (i = 0; i < 4; i++) {
			switch (r_ptr->chsize[i]) {
			case -1:
			case 5:
			case 6:
			case 7:
			case 8:
			   break;
			default:
			   (void) sprintf(ToHtx->GetMsgAddr(),
					  "line# %d %s = %d (not a valid character \
						size between 5 and 8)\n",
					  *line, keywd, r_ptr->chsize[i]);
			   ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			   error = 'y';
			   break;
			}                   /* endswitch */
		 }                      /* endfor */
      } else if ((strcmp(keywd, "CSTOPB")) == 0) {
		 NoItems = sscanf(s, "%*s %d %d",
				   &(r_ptr->cstopb[0]), &(r_ptr->cstopb[1]));
		 for (i = 0; i < 2; i++) {
			switch (r_ptr->cstopb[i]) {
			case -1:
			case 1:
			case 2:
			   break;
			default:
			   (void) sprintf(ToHtx->GetMsgAddr(),
					  "line# %d %s = %d (number of stop bits must be 1 or 2)\n",
					  *line, keywd, r_ptr->cstopb[i]);
			   ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			   error = 'y';
			   break;
			}                   /* endswitch */
		 }                      /* endfor */
      } else if ((strcmp(keywd, "PARODD")) == 0) {
		 for (i = 0; i < 3; i++) {
			ss[i][0] = '\0';
		 }                      /* endfor */
		 NoItems = sscanf(s, "%*s %s %s %s", ss[0], ss[1], ss[2]);
		 for (i = 0; i < 3; i++) {
			if (ss[i][0] != '\0') {
			   r_ptr->parodd[i] = ss[i][0];
			}                   /* endif */
			switch (r_ptr->parodd[i]) {
			case '\0':
			case 'O':
			case 'E':
			case 'N':
			   break;
			default:
			   (void) sprintf(ToHtx->GetMsgAddr(),
					  "line# %d %s = %c (parity must be O, E, or \
						N)\n",
					  *line, keywd, r_ptr->parodd[i]);
			   ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			   error = 'y';
			   break;
			}                   /* endswitch */
		 }                      /* endfor */
      } else if ((strcmp(keywd, "NUM_CHARS")) == 0) {
		 (void) sscanf(s, "%*s %d", &(r_ptr->num_chars));
		 if (r_ptr->num_chars < 0 || r_ptr->num_chars > MAX_CHARS) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %d (number of characters must be \
					between 0 and %d)\n",
				   *line, keywd, r_ptr->num_chars, MAX_CHARS);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }  /* endif */
      } else if ((strcmp(keywd, "VMIN")) == 0) {
		 (void) sscanf(s, "%*s %d", &(r_ptr->vmin));
		 if (r_ptr->vmin < 0 || r_ptr->vmin > MAX_CHARS) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %d (VMIN must be between 0 and %d)\n",
				   *line, keywd, r_ptr->vmin, MAX_CHARS);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }                      /* endif */
      } else if ((strcmp(keywd, "CRASH")) == 0) {
		 (void) sscanf(s, "%*s %d", &(r_ptr->crash));
		 if (r_ptr->crash < 0 || r_ptr->crash > 2) {
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %d (CRASH must be 0 - 2)\n",
				   *line, keywd, r_ptr->crash);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
		 }                      /* endif */
      } else if ((strcmp(keywd, "HW_PACING")) == 0) {
		 ss[0][0] = '\0';
		 if((NoItems = sscanf(s, "%*s %s", ss[0])) == 1 || NoItems == -1) {
			if(NoItems == -1 || strcmp("NONE", ss[0]) == 0) {
			   r_ptr->rts = 0;
			   r_ptr->dtr = 0;
			}
			else if(strcmp("RTS", ss[0]) == 0){
			   r_ptr->rts = 1;
			   r_ptr->dtr = 0;
			}
			else if(strcmp("DTR", ss[0]) == 0) {
			   r_ptr->rts = 0;
			   r_ptr->dtr = 1;
			}
			else {
			   (void) sprintf(ToHtx->GetMsgAddr(),
				 "line# %d %s = %s (HW pacing must be RTS, DTR, or NONE)\n",
					  *line, keywd, ss[0]);
			   ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			   error = 'y';
			}
		 }
      } else if ((strcmp(keywd, "128P")) == 0) {
		 for (i = 0; i < 3; i++) {
			ss[i][0] = '\0';
		 }                      /* endfor */
		 NoItems = sscanf(s, "%*s %s %s %s", ss[0], ss[1], ss[2]);
		 for(i=0; i < NoItems; i++) {
			if(strcmp("FASTCOOK_ON", ss[i]) == 0){
			   r_ptr->fastcook = 1;
			}
			else if(strcmp("FASTCOOK_OFF", ss[i]) == 0){
			   r_ptr->fastcook = 0;
			}
			else if(strcmp("ALTPIN_ON", ss[i]) == 0) {
			   r_ptr->altpin = 1;
			}
			else if(strcmp("ALTPIN_OFF", ss[i]) == 0) {
			   r_ptr->altpin = 0;
			}
			else if(strcmp("DTRPACE_ON", ss[i]) == 0) {
			   r_ptr->dtrpace = 1;
			}
			else if(strcmp("DTRPACE_OFF", ss[i]) == 0) {
			   r_ptr->dtrpace = 0;
			}
			else {
			   (void) sprintf(ToHtx->GetMsgAddr(),
				 "line# %d %s = %s (128 port must be FASTCOOK_ON or FASTCOOK_OFF, ALTPIN_ON or ALTPIN_OFF,\nDTRPACE_ON or DTRPACE_OFF)\n",
					  *line, keywd, ss[i]);
			   ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			   error = 'y';
			}
		 }
      } else if ((strcmp(keywd, "DIRECTION")) == 0) {
		 for (i = 0; i < 2; i++) {
			 ss[i][0] = '\0';
		 }                      /* endfor */
		 NoItems = sscanf(s, "%*s %s %s", ss[0], ss[1]);
		 for(i=0; i < NoItems; i++) {
			if(strcmp("DEFAULT", ss[i]) == 0){
			   r_ptr->default_dir = 1;
			}
			else if(strcmp("REVERSED", ss[i]) == 0){
			   r_ptr->default_dir = 0;
			}
			else {
			   (void) sprintf(ToHtx->GetMsgAddr(),
				 "line# %d %s = %s (DIRECTION must be DEFAULT or REVERSED)\n",
					  *line, keywd, ss[i]);
			   ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			   error = 'y';
			}
		 }
      } else if ((strcmp(keywd, "TRACE")) == 0) {
		 ss[0][0] = '\0';
		 (void) sscanf(s, "%*s %s", ss[0]);
		 switch (ss[0][0]) {
		 case '\0':
		 case 'N':
			r_ptr->trace_flag = 0;
		 case 'Y':
			r_ptr->trace_flag = 1;
			break;
		 default:
			(void) sprintf(ToHtx->GetMsgAddr(),
				   "line# %d %s = %c (value must be Y(yes) or \
	N(no))\n",
				   *line, keywd, r_ptr->ixoff);
			ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
			error = 'y';
			break;
		 }                      /* endswitch */
      } else {
		 (void) sprintf(ToHtx->GetMsgAddr(), "line# %d keywd = %s (invalid) \n",
				*line, keywd);
		 ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
		 error = 'y';
      }                         /* endif */
   }                            /* endwhile */
   *line = *line + 1;
   if (keywd_count > 0) {
      if ((strcmp(r_ptr->rule_id, "        ")) == 0) {
	 (void) sprintf(ToHtx->GetMsgAddr(), "line# %d rule_id not specified \n",
			first_line);
	 rc = 1;
      } else
	 rc = 0;
   } else {
      rc = EOF;
   }                            /* endif */
   if (error == 'y') {
      rc = 1;
   }                            /* endif */
   return (rc);
}                               /* get_rule() */



/****************************************************************************/
/*****  g e t l i n e ( )  **************************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     get_line()                                            */
/*                                                                          */
/* DESCRIPTIVE NAME =  Get a line from stdin.                               */
/*                                                                          */
/* FUNCTION =          This routine reads a line from "stdin" into the      */
/*                     specified string.  It returns the length of the      */
/*                     string.  If the length is 1 the line is blank.  When */
/*                     it reaches EOF the length is set to 0,               */
/*                                                                          */
/* INPUT =             Specified by rules file.                             */
/*                                                                          */
/* OUTPUT =            Specified by rules file.                             */
/*                                                                          */
/* NORMAL RETURN =     0 to indicate EOF.                                   */
/*                     1 to indicate blank line.                            */
/*                     Otherwise, the length of the line.                   */
/*                                                                          */
/* ERROR RETURNS =     None.                                                */
/*                                                                          */
/* EXTERNAL REFERENCES = None.                                              */
/*                                                                          */
/****************************************************************************/

static int get_line(FILE *fd, char *s, int lim)
{
   int             c=0;           /* input character                         */
   int             i;           /* array index                             */

   i = 0;                       /* set array index to 0                    */
   while ((--lim > 0) && ((c = getc(fd)) != EOF) && (c != '\n')) {
      s[i++] = c;               /* copy char to array                      */
   }                            /* endwhile                                */
   if (c == '\n') {             /* newline character?                      */
      s[i++] = c;               /* copy char to array                      */
   }                            /* endif                                   */
   s[i] = '\0';                 /* copy string terminator to array         */
   return (i);                  /* return number of chars in line          */
}                               /* get_line()                               */



int SanityCkBaud(       enum wtype wrap_type,
					int cbaud[],
					ARGS_T *ArgDataPtr)
{
   int i;
   int flag = 0;
      // get link to htx for messages 
   Cmsg *ToHtx=(Cmsg *)ArgDataPtr->MainPtr;

   
   if(wrap_type == eCable)
      return(0);
   for(i=0;i<16;i++) {
      if(cbaud[i] > 0)
	 flag = 1;
      if(cbaud[i] == -1)
	 break;
   }
   if(flag)
      return(0);
   else {
      (void) strcpy(ToHtx->GetMsgAddr(), "Can't do half-duplex on a wrap-plug.  \
There must be at least\n1 full-duplex baud specified on wrap plugs.\n");
      ToHtx->SendMsg(1, HTX_HE_SOFT_ERROR);
      return(-1);
	}
}



/****************************************************************************/
/*****  S e t S t a n z a D e f a u l t s ( ) *******************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     SetStanzaDefaults                                    */
/*                                                                          */
/* DESCRIPTIVE NAME =  Sets the stanza default values in the rules          */
/*                     structure.                                           */
/*                                                                          */
/* FUNCTION =          Sets the default values in the rules structure.      */
/*                                                                          */
/* INPUT =             r_ptr - pointer value to the rules structure.        */
/*                                                                          */
/* OUTPUT =            Modified rules structure.                            */
/*                                                                          */
/* NORMAL RETURN =     None - void function.                                */
/*                                                                          */
/* ERROR RETURNS =     None - void function.                                */
/*                                                                          */
/* EXTERNAL REFERENCES = None.                                              */
/*                                                                          */
/****************************************************************************/

static void SetStanzaDefaults(struct rule_format * r_ptr)
{
   int             i;           /* loop counter                     */

   (void) strcpy(r_ptr->rule_id, "        ");
   (void) strcpy(r_ptr->pattern_id, "        ");
   (void) strcpy(r_ptr->slewrate, "FAST");
   r_ptr->num_oper = 1;
   r_ptr->ign_brk = 'N';
   r_ptr->clocal = 'N';
   r_ptr->hupcl = 'Y';
   r_ptr->brk_int = 'N';
   r_ptr->ign_par = 'N';
   r_ptr->par_mark = 'N';
   r_ptr->ixon = 'N';
   r_ptr->ixoff = 'N';
   r_ptr->cbaud[0] = 9600;
   for (i = 1; i < 16; i++) {
      r_ptr->cbaud[i] = -1;
   }                            /* endfor */
   r_ptr->chsize[0] = 7;
   for (i = 1; i < 4; i++) {
      r_ptr->chsize[i] = -1;
   }                            /* endfor */
   r_ptr->cstopb[0] = -1;
   r_ptr->cstopb[1] = -1;
   r_ptr->parodd[0] = 'N';
   r_ptr->parodd[1] = '\0';
   r_ptr->parodd[2] = '\0';
   r_ptr->num_chars = 255;
   r_ptr->vmin = 1;
   r_ptr->crash = 0;
   r_ptr->bufsize = 10;
   r_ptr->ioctl_sleep = 1000000; /* 1 second */
   r_ptr->write_sleep = 0; /* 0 second */
   r_ptr->default_dir =1;
   r_ptr->ihog = 0;
   r_ptr->trace_flag = 0;
   return;
}                               /* set_defaults */



/****************************************************************************/
/*****  S e t H w D e f a u l t s ( ) ***************************************/
/****************************************************************************/
/*                                                                          */
/* FUNCTION NAME =     SetHwDefaults                                        */
/*                                                                          */
/* DESCRIPTIVE NAME =  Sets the hardware default values in the rules        */
/*                     structure.                                           */
/*                                                                          */
/* FUNCTION =          Sets the default values in the rules structure.      */
/*                                                                          */
/* INPUT =             r_ptr - pointer value to the rules structure.        */
/*                                                                          */
/* OUTPUT =            Modified rules structure.                            */
/*                                                                          */
/* NORMAL RETURN =     None - void function.                                */
/*                                                                          */
/* ERROR RETURNS =     None - void function.                                */
/*                                                                          */
/* EXTERNAL REFERENCES = None.                                              */
/*                                                                          */
/****************************************************************************/

static void SetHwDefaults(struct rule_format * r_ptr)
{
      r_ptr->rts = 1;
      r_ptr->dtr = 0;
      /* The following only apply to 128 port    */
      r_ptr->fastcook =1;
      r_ptr->altpin = 1;
      r_ptr->dtrpace = 0;
}
