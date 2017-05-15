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
 *   MODULE NAME: io_oper.c
 *
 *   FUNCTIONS:   do_req_sense() perform a fast request sense.
 *                 audio_cdrom() play audio - scsi command type.
 *                    audio_mm() play audio through devicde driver.
 *              get_audio_type() determine and get play audio command type
 *                 audio_cdrom() audio command functions.
 *                  diag_cdrom() send diagnostics command.
 *                    do_sleep() wait sleep function.
 *                  init_iocmd() initialize io_cmd block.
 *               prt_req_sense() do req sense and print information.
 *                  read_cdrom() read specified block.
 *                    set_addr() set file pointer to address.
 *                      to_bcd() convert integer to bcd.
 *                close_reopen() close device and reopen in desired mode.
 *              get_audio_type() determine scsi audio command type.
 *                 get_disc_pn() read cdrom disc toc and determine p/n.
 *            halt_audio_cdrom() halt a play audio command with stop unit.
 *               halt_audio_mm() halt a play audio through device driver.
 *                      ms_get() get current device mode setting.
 *          read_write_pattern() read/write a read/compare data file.
 *                  show_stuff() show iocmd present values, do request sense.
 *                      do_cmd() run a command asif from a shell.
 *
 *   DESCRIPTION: Functions used to exercise the cdrom devices.
 ******************************************************************************/

#include "hxecd.h"
#define MAX_SENSE 64

#ifndef __HTX_LINUX__
static char sense_buf [MAX_SENSE];
static char msg_str [256];
#endif

extern char device_subclass[];  /* subclass of device */
extern dev_t device_st_rdev;    /* device id (see stat.h) */

/*************************************************************************/
/* close_reopen - close device and reopen in the passed mode.            */
/* The value passed will typically be either SC_DIAGNOSTIC mode or       */
/* SC_SINGLE.                                                            */
/* returns: 0 = successful close/reopen.                                  */
/*         -1 = close failed.                                            */
/*         -2 = open failed.                                             */
/*         -3 = currently selected mode set failure.                     */
/*  NOTES:  If the device_subclass is 'I' (ide) then the reopen will     */
/*          always be SC_SINGLE since SC_DIAGNOSTIC is not supported     */
/*          for ide atapi passthru.                                      */
/*************************************************************************/
#ifndef __HTX_LINUX__
int close_reopen(struct htx_data *ps, struct ruleinfo *pr, int omode,
                 char whodo[100])
{
  int  rc;
  unsigned int lcl_last_lba, lcl_blk_size;
  char tmpmsg[20], tmsg[200];

  strcpy(tmsg, whodo);
  rc = 0;
  close(pr->fildes);

   /***************************************
    * if the device is ide then only reopen
    * in SC_SINGLE mode.
    ***************************************/
  if ( device_subclass[0] == 'I' || strncmp(device_subclass, "sata", 16) == 0 ) omode = SC_SINGLE;

  switch (omode) {
     case SC_DIAGNOSTIC: {            /* open device in SC_DIAGNOSTIC mode */
        pr->fildes = openx(ps->sdev_id, O_RDONLY, 0, SC_DIAGNOSTIC);
        if ( pr->fildes == -1 ) {
           rc = -2;                                         /* open failed */
           strcat(tmsg, ": close_reopen(): reopen in diag mode failed.\n");
           hxfmsg(ps, 0, HARD, tmsg);
        }
        break;
     }
     default:                       /* open in the currently selected mode */
        pr->fildes = openx(ps->sdev_id, O_RDONLY, 0, SC_SINGLE);
        if ( pr->fildes == -1 ) {
           rc = -2;                                         /* open failed */
           strcat(tmsg, ": close_reopen(): reopen in single mode failed.\n");
           hxfmsg(ps, 0, HARD, tmsg);
        } else {          /* now set currently selected mode within device */
           strcpy(tmpmsg, pr->rule_id);            /* save current rule id */
           strcpy(pr->rule_id, "REOPEN");
           if ( ms_get(pr, ps, &lcl_last_lba, &lcl_blk_size) == -1 ) {
              strcpy(pr->rule_id, tmpmsg);              /* restore rule id */
              rc = -3;           /* set currently selected mode set failed */
              sprintf(tmsg,
                      "%s: close_reopen(): reopen set mode to %s failed.\n",
                      whodo, pr->mode);
              hxfmsg(ps, 0, HARD, tmsg);
           }
           strcpy(pr->rule_id, tmpmsg);                 /* restore rule id */
        }
  }
  return(rc);
}
#endif

/**********************************************************************/
/* get_audio_type: do an inquiry and look at Manufacturer and Product */
/* Type+Model Number data to see which type of play audio support is  */
/* needed, standard or vendor specific.                               */
/* Returns:                                                           */
/*    audio_type set = 0 for standard scsi audio play of '48'.        */
/*    audio_type set = 1 for vendor unique Toshiba commands of        */
/*                     'C0' search, 'C1' play audio.                  */
/*    audio_type set = 2 for vendor unique Toshiba commands of        */
/*                     'C0' search, 'C1' play audio. This value       */
/*                     represents TOSHIBA Model 3101 which does not   */
/*                     support read table of contents, x'43' command. */
/*    audio_type set = -1 for error.                                  */
/*                                                                    */
/**********************************************************************/
#ifndef __HTX_LINUX__
int get_audio_type(struct ruleinfo *pr, struct htx_data *ps)
{
  int    rc, i, xrc, x, xno;
  int    sens_len, retry_op, audio_type;
  char   sense_buf[120], tmp_str[120], rbuf[220];
  char   vendor_id[20], prodnum[30];
  struct sc_iocmd iocmd_buf;
  struct scsi_iocmd viocmd_buf;

  xno = 0;
  audio_type = 0;          /* default is standard scsi play audio type */

  /*********************************************************************
  * If this is an IDE CDROM (Maverick for now), then assume that it    *
  * support the standard audio ioctl()'s and just return type 0.       *
  **********************************************************************/
  if ( device_subclass[0] == 'I' || strncmp(device_subclass, "sata", 16) == 0 )
      return(audio_type);

  if ( close_reopen(ps, pr, SC_DIAGNOSTIC, "get_audio_type") != 0 )
     prt_msg(ps, pr, 0, 0, errno, HARD, "open in diagnostic mode failure - ");
  else {                                    /* get inquiry information */
    if(device_subclass[0] == 'S') {
     init_iocmd(&iocmd_buf);
     iocmd_buf.buffer = rbuf;
     iocmd_buf.data_length = 200;
     iocmd_buf.flags = B_READ;
     iocmd_buf.scsi_cdb[0] = 0x12;
     iocmd_buf.scsi_cdb[4] = iocmd_buf.data_length;
     iocmd_buf.command_length = 6;
    }
   if(device_subclass[0] == 'V') {
     /* printf("In get_audio_type, vscsi setup, using DKIOLCMD for INQUIRY\n"); */
     init_viocmd(&viocmd_buf);
     viocmd_buf.buffer = rbuf;
     viocmd_buf.data_length = 200;
     viocmd_buf.flags = B_READ;
     viocmd_buf.scsi_cdb[0] = 0x12;
     viocmd_buf.scsi_cdb[4] = viocmd_buf.data_length;
     viocmd_buf.command_length = 6;
   }
                /*******************************************************/
                /*- perform the inquiry command                       -*/
                /*- if a check condition is sensed then request sense -*/
                /*- data to clear conditon and retry the command once -*/
                /*******************************************************/
     rc = 0;
     retry_op = 2;
     while ( retry_op > 0 ) {
        retry_op--;

	if(device_subclass[0] == 'S')
         rc = ioctl(pr->fildes, DKIOCMD, &iocmd_buf);    /*- send command -*/
        else if(device_subclass[0] == 'V')
         rc = ioctl(pr->fildes, DKIOLCMD, &viocmd_buf);

        if ( rc == 0 )
           retry_op--;
        else {
           if ( rc == -1 ) {
              rc = 0;
              sense_buf[7] = 0;    /* reset sns buffer's additional sns byte */
              xrc = do_req_sense(pr->fildes, sense_buf, sizeof(sense_buf));
                    /*********************************************************/
                    /*- print out sense data if on last retry only          -*/
                    /*********************************************************/
              if ( (xrc == 0) && (retry_op == 0) ) {
                 strcpy(msg_str, " audio type inquiry command error - ");
                    strcat(msg_str, strerror(errno));
                 strcat(msg_str, "\n  Request Sense Data:");
                 sens_len = 8 + sense_buf[7];
                 for ( i = 0; i < sens_len; i++ ) {
                    if ( (i & 15) == 0 ) {
                       sprintf(tmp_str, "\n    [%02X] ", i);
                       strcat(msg_str, tmp_str);
                    }
                    sprintf(tmp_str, "%02X ", sense_buf[i]);
                    strcat(msg_str, tmp_str);
                 }
                 hxfmsg(ps, 0, SOFT, msg_str);
              } else {                         /* request sense failure     */
                 if ( retry_op == 0 ) {
                    strcpy(msg_str, " request sense error after audio "
                                    "type inquiry command retried once - ");
                    hxfmsg(ps, 0, SOFT, msg_str);
                 }
              }
           }
        }
     }
  }
  if ( rc != 0 )
     audio_type = -1;
  else {                                    /*--- check the inquiry data ---*/
           /*****************************************************************/
           /* determine vendor id and product number                        */
           /*****************************************************************/
     strcpy(vendor_id, "");
     strcpy(prodnum, "");
     for ( x = 0; x < 8; x++ )
        vendor_id[x] = rbuf[x+8];
     vendor_id[x] = '\0';
     for ( x = 0; x < 16; x++ )
        prodnum[x] = rbuf[x+16];
     prodnum[x] = '\0';
        /*--printf("vendor_id = [%s]\n",vendor_id);--*/
        /*--printf("prodnum = [%s]\n",prodnum);--*/
     if ( strncmp(vendor_id, "TOS", 3) == 0 )    /* if manufacturer = toshiba */
        audio_type = 1;                          /* TOSHIBA - NO TOC support  */
     else if ( strncmp(vendor_id, "IBM", 3) == 0 )  /* if manufacturer = IBM  */
        if ( strcmp(prodnum, "CDRM00201     !F") == 0 )
           audio_type = 2;
        else if ( strcmp(prodnum, "CDRM00201     !E") == 0 )
           audio_type = 0;
                                 /*--- close and reopen not in diagnostics ---*/
    if (close_reopen(ps, pr, SC_SINGLE, "get_audio_type") != 0 ) {
       prt_msg(ps, pr, 0, 0, errno, HARD, "get_audio_type(): reopen failure- ");
       audio_type = -1;
    }
  }  /*- end opened in sc_diagnostic mode -*/
  return(audio_type);
}
#endif

/**************************************************************************/
/* set file pointer                                                       */
/**************************************************************************/

void set_addr(struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno)
{
  /*off_t addr, rcode, lseek();*/
    offset_t addr, rcode;

#ifdef __HTX_LINUX__     /* Linux */
  /* No seek required for M2F1 M2F2 and DA */
  if(strncmp(pr->mode, "M1", 4) != 0) {
    return;
  }
#endif
  addr = (offset_t) blkno[0] * (offset_t) pr->bytpsec;
 /**********************************************************************
  * sprintf(msg, "DEBUG: set_addr: blkno[0] = %d  pr->bytpsec = %d "
  *              " addr = %d (x%04X)\n",
  *         blkno[0], pr->bytpsec, addr,addr);
  * prt_msg(ps, pr, loop, blkno, errno, INFO, msg);
  ***********************************************************************/
/*  rcode = lseek(pr->fildes, addr, 0);*/
#ifdef __HTX_LINUX__
  rcode = lseek64(pr->fildes, addr, 0);
#else
  rcode = llseek(pr->fildes, addr, 0);
#endif
  if ( rcode == -1 ) {
     ps->bad_others = ps->bad_others + 1;
     prt_msg(ps, pr, loop, blkno, errno, SYSERR, "lseek error - ");
  }
  return;
}

/**************************************************************************/
/* read from cdrom                                                        */
/**************************************************************************/
int read_cdrom(struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno,
           char *rbuf)
{
  int     rc, attempt = 1;
  char    msg[200];
  #define NUM_ATTEMPTS 	5	/* Defect 231040 - change to 5 from 3 */
  #define PLD_DELAY    20	/* Defect 231040 - change to 20 from 2 */
#ifdef __HTX_LINUX__     /* Linux */
  /* Linux specific declarations */
  struct cdrom_read_audio cda_cmd;
  struct cdrom_msf cdmsf_cmd;
  unsigned int lba;
  /* Linux specific code */
  memset(&cda_cmd, 0, sizeof(struct cdrom_read_audio));
#endif
  memset(rbuf, 0xbb, pr->dlen);
  set_addr(ps, pr, loop, blkno);
  /* Set up a retry loop for read.  Necessary in case the CD is being hit
     with EPOWs due to the PLD test run by the EMC lab. */
  while ( attempt <= NUM_ATTEMPTS ) {

#ifndef __HTX_LINUX__     /* AIX */
     rc = read(pr->fildes, rbuf, pr->dlen);
#else                     /* Linux */

     if(strncmp(pr->mode, "M1", 4) == 0) {
         /* Mode 1 */
         rc = read(pr->fildes, rbuf, pr->dlen);
     } else if(strncmp(pr->mode, "DA", 4) == 0) {
         /* Digital Audio */
         cda_cmd.addr.lba = blkno[0];
         cda_cmd.addr_format = CDROM_LBA;
         cda_cmd.nframes = pr->dlen/pr->bytpsec;
         cda_cmd.nframes = 1;
         cda_cmd.buf = (unsigned char *)rbuf;
         rc = ioctl(pr->fildes, CDROMREADAUDIO, &cda_cmd);

         if( rc == 0 ) { /* Success */
             rc = pr->dlen;
         }

     } else if(strncmp(pr->mode, "M2F2", 4) == 0) {
          /* Mode 2 Form 2 */
          /* Convert start LBA */
         lba = blkno[0] + CD_MSF_OFFSET;
         cdmsf_cmd.cdmsf_min0 = lba / (CD_SECS*CD_FRAMES);
         lba %=  (CD_SECS*CD_FRAMES);
         cdmsf_cmd.cdmsf_sec0 = lba / CD_FRAMES;
         cdmsf_cmd.cdmsf_frame0 = lba % CD_FRAMES;
         /* Convert End LBA */
         lba = blkno[0] + CD_MSF_OFFSET + pr->dlen/pr->bytpsec;
         cdmsf_cmd.cdmsf_min1 = lba / (CD_SECS*CD_FRAMES);
         lba %=  (CD_SECS*CD_FRAMES);
         cdmsf_cmd.cdmsf_sec1 = lba / CD_FRAMES;
         cdmsf_cmd.cdmsf_frame1 = lba % CD_FRAMES;

         memcpy(rbuf, &cdmsf_cmd, sizeof(struct cdrom_msf));
         rc = ioctl(pr->fildes, CDROMREADMODE2, rbuf);

         if( rc == 0 ) { /* Success */
             rc = pr->dlen;
         }

     } else if(strncmp(pr->mode, "M2F1", 4) == 0) {
          /* Mode 2 Form 1 */
          /* Convert start LBA */
         lba = blkno[0] + CD_MSF_OFFSET;
         cdmsf_cmd.cdmsf_min0 = lba / (CD_SECS*CD_FRAMES);
         lba %=  (CD_SECS*CD_FRAMES);
         cdmsf_cmd.cdmsf_sec0 = lba / CD_FRAMES;
         cdmsf_cmd.cdmsf_frame0 = lba % CD_FRAMES;
         /* Convert End LBA */
         lba = blkno[0] + CD_MSF_OFFSET + pr->dlen/pr->bytpsec;
         cdmsf_cmd.cdmsf_min1 = lba / (CD_SECS*CD_FRAMES);
         lba %=  (CD_SECS*CD_FRAMES);
         cdmsf_cmd.cdmsf_sec1 = lba / CD_FRAMES;
         cdmsf_cmd.cdmsf_frame1 = lba % CD_FRAMES;

         memcpy(rbuf, &cdmsf_cmd, sizeof(struct cdrom_msf));
         rc = ioctl(pr->fildes, CDROMREADMODE2, rbuf);

         if( rc == 0 ) { /* Success */
             rc = pr->dlen;
         }

         /* Skip 1st 8 bytes. Assuming always only 1 block transfer will happen*/
         memcpy(rbuf, rbuf + 8, rc);

     }

#endif

     if ( rc < 0 && errno == EBUSY && attempt < NUM_ATTEMPTS ) {
        sprintf(msg, "read_cdrom(): EBUSY on attempt %d; waiting for "
                     "system stabilization\n", attempt);
        prt_msg(ps, pr, loop, blkno, errno, INFO, msg);
        sleep(PLD_DELAY);
        sprintf(msg, "read_cdrom(): resuming read...\n");
        prt_msg(ps, pr, loop, blkno, 0, INFO, msg);
        attempt++;
     } else
        break;
  }
  if ( rc == -1 ) {
     ps->bad_reads = ps->bad_reads + 1;
     sprintf(msg, "read_cdrom(): read error on attempt %d - ", attempt);
     prt_msg(ps, pr, loop, blkno, errno, HARD, msg);

     return -1; /* in case of continue on error */
  } else if ( rc != pr->dlen ) {
     ps->bad_reads = ps->bad_reads + 1;
     sprintf(msg, "read_cdrom(): Read request (%d) != read returned (%d)\n"
                  "on attempt %d", pr->dlen, rc, attempt);
     prt_msg(ps, pr, loop, blkno, 0, HARD, msg);

     return -1; /* in case of continue on error */
  } else {
     ps->good_reads = ps->good_reads + 1;
     ps->bytes_read = ps->bytes_read + pr->dlen;

     return 0; /* we are Go! for compare ! */
  }
}

/**************************************************************************/
/* read from cdrom and write out a new pattern file for read compare      */
/* NOTE: This function is intended for use of creating new read compare   */
/*       data files ONLY.                                                 */
/* INPUTS: Starting block, number of blocks, CD-ROM Mode, Pattern ID.     */
/*OUTPUTS: CDxxxxxx.info  = text description info of accompanying file.   */
/*         CDxxxxxx       = accompanying read compare data file.          */
/*          (xxxxxx) above = the supplied Pattern ID.                     */
/**************************************************************************/
void read_write_pattern(struct htx_data *ps, struct ruleinfo *pr, int loop,
                   int *blkno, char *rbuf)
{
  int  rc, r_ptr, save_dlen;
  int  mm, ss, bb, x, i;
  char CDpath[120], INFOpath[120], msg[200], xsg[100];
  char sMM[5], sSS[5], sBB[5], msb[40], tbuf[3000];
  static char open_type[4] = "wb";     /* for fopen() */
  FILE *fp;

  memset(rbuf, 0xbb, pr->dlen);
  save_dlen = pr->dlen;
  pr->dlen = pr->bytpsec;              /*-- read 1 block at a time --*/
  r_ptr = 0;
  rc = 0;
  while ( (rc != -1) && (r_ptr < pr->num_blks) ) { /* read 1 block sequen-    */
     set_addr(ps,pr,loop,blkno);                   /* tially into buffer      */
     rc = read(pr->fildes, tbuf, pr->dlen);        /* until all blocks read   */
     if ( rc != -1 ) {
        for ( i = 0; i <= pr->dlen; i++ )
           *(rbuf+((pr->dlen*r_ptr)+i)) = tbuf[i];
        set_blkno(blkno, pr->direction, pr->increment, 1);
     }
     r_ptr++;
  }
  pr->dlen = save_dlen;
  if ( rc == -1 )
     prt_msg(ps, pr, loop, blkno, errno, HARD, "read error - ");
  else {
                    /*--- write out new pattern id read compare data file ---*/
     if ( (int)strlen((char *) strcpy(CDpath,
                                      (char *)getenv("HTXPATTERNS"))) == 0 )
     strcpy(CDpath, "../pattern/");                       /* default ONLY */
     strcat (CDpath, "CD\0");
     strcat (CDpath, pr->pattern_id);
		/* set access type to pattern file so that it truncates the */
	    /* pattern file on the first loop and then appends to it    */
	 open_type[0] = loop == 1 ? 'w' : 'a' ;
     if ( (fp = fopen(CDpath, open_type)) == NULL ) {
        sprintf(msg, "Open error on pattern file %s.\n", CDpath);
        prt_msg(ps, pr, loop, blkno, errno, HARD, msg);
     } else {
        if ( fwrite(rbuf, pr->dlen, 1, fp) != 1 ) {
           sprintf(msg, "Write error to pattern file %s.\n", CDpath);
           prt_msg(ps, pr, loop, blkno, errno, HARD, msg);
        }
        fclose(fp);
     }
                        /*-- build decsription string for information file --*/
     sprintf(msg, " CD-ROM DISC P/N: %s\n", pr->rule_id);
     sprintf(xsg, "      Pattern ID: %s\n", pr->pattern_id);
     strcat(msg, xsg);
     sprintf(xsg, "     CD-ROM Mode: %s\n", pr->mode);
     strcat(msg, xsg);
     sprintf(xsg, "  Starting block: %s  (", pr->starting_block);
     strcat(msg, xsg);
                                      /**************************************/
                                      /*- calculate mm:ss:bb from block #  -*/
                                      /**************************************/
     mm = 0;
     ss = 0;
     bb = 0;
     x = atoi(pr->starting_block) + 150;
     if ( x >= 4500 )
        mm = (x / 4500);
     x = (x - (mm * 4500));
     if ( x >= 75 )
        ss = (x / 75);
     x = (x - (ss * 75));
     bb = x;
     sprintf(sMM, "%02d", mm);
     sprintf(sSS, ":%02d", ss);
     sprintf(sBB, ":%02d", bb);
     strcpy(msb, sMM);
     strcat(msb, sSS);
     strcat(msb, sBB);
     sprintf(xsg, "%s)\n",msb);
     strcat(msg, xsg);
     sprintf(xsg, "Number of blocks: %d\n", pr->num_blks);
     strcat(msg, xsg);
     strcpy(INFOpath, "/tmp/CD");
     strcat(INFOpath, pr->pattern_id);
     strcat(INFOpath, ".info");
     if ( (fp = fopen(INFOpath, "w")) == NULL ) {
        sprintf(msg, "Open error on information file %s.\n", INFOpath);
        prt_msg(ps, pr, loop, blkno, errno, HARD, msg);
     } else {
        if ( fwrite(msg, strlen(msg), 1, fp) != 1 ) {
           sprintf(msg, "Write error to information file %s.\n", INFOpath);
           prt_msg(ps, pr, loop, blkno, errno, HARD, msg);
        }
        fclose(fp);
     }
  }
}

/**************************************************************************/
/* audio                                                                  */
/**************************************************************************/
#ifndef __HTX_LINUX__
void audio_cdrom(struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno)
{
  int    rc, xrc, i, sens_len;
  char   tmp_str[3], buf[1024];
  FILE   *fp;
  time_t t1;
  struct sc_iocmd iocmd_buf;

  rc = 0;
  if ( close_reopen(ps, pr, SC_DIAGNOSTIC, "audio_cdrom") != 0 )
     prt_msg(ps, pr, loop, blkno, errno, HARD,
             "open in diagnostic mode failure - ");
  else {
                                /* TOSHIBA device and/or vendor unique play */
                                /* audio scsi commands required             */
     if ( pr->master_audio_type > 0 ) {
        init_iocmd(&iocmd_buf);           /* send audio track search command */
        iocmd_buf.command_length = 10;
        iocmd_buf.scsi_cdb[0] = 0xC0;             /*--- opcode for search ---*/
        iocmd_buf.scsi_cdb[1] = 1;                /*--- play when found   ---*/
        iocmd_buf.scsi_cdb[2] = to_bcd(*blkno);       /*--- track to play ---*/
        iocmd_buf.scsi_cdb[9] = 0x80;              /*--- type for track # ---*/
        rc = ioctl(pr->fildes, CDIOCMD, &iocmd_buf);
        if ( rc == -1 ) {
           ps->bad_others = ps->bad_others + 1;
           prt_msg(ps, pr, loop, blkno, errno, HARD,
                   "vendor unique play audio track search error - ");
        } else {
                    /*--- send audio track play command to set last track ---*/
           init_iocmd(&iocmd_buf);
           iocmd_buf.command_length = 10;
           iocmd_buf.scsi_cdb[0] = 0xC1;            /*--- opcode for play ---*/
           iocmd_buf.scsi_cdb[1] = 3;                     /*--- play mode ---*/
           iocmd_buf.scsi_cdb[2] = to_bcd(*blkno + pr->num_blks);
           iocmd_buf.scsi_cdb[9] = 0x80;           /*--- type for track # ---*/
           rc = ioctl(pr->fildes, CDIOCMD, &iocmd_buf);
           if ( rc == -1 ) {
              ps->bad_others = ps->bad_others + 1;
              prt_msg(ps, pr, loop, blkno, errno, HARD,
                      "vendor unique play audio error - ");
           } else {
              pr->op_in_progress = 1;    /* set Toshiba audio op in progress */
              sense_buf [0] = 0;         /* wait for play to be finished     */
              while ( (sense_buf[0] != 3) & (rc != -1) ) { /* while not done */
                  init_iocmd(&iocmd_buf);
                  iocmd_buf.command_length = 10;
                  iocmd_buf.data_length = 10;      /*--- use sense buffer ---*/
                  iocmd_buf.flags = B_READ;
                  iocmd_buf.buffer = sense_buf;
                  iocmd_buf.scsi_cdb[0] = 0xC6;
                  iocmd_buf.scsi_cdb[1] = 10;            /*--- # of bytes ---*/
                  rc = ioctl(pr->fildes, CDIOCMD, &iocmd_buf);
                  if ( rc == -1 ) {
                     ps->bad_others = ps->bad_others + 1;
                     prt_msg(ps, pr, loop, blkno, errno, HARD,
                             "vendor unique play audio wait error - ");
                  }
                  hxfupdate(UPDATE,ps);
                  time(&t1);                         /*--- wait 5 seconds ---*/
                  while ( (time(NULL) - t1 < 5) & (rc !=-1) ) ;
              }
              pr->op_in_progress = 0;         /* set no audio op in progress */
           }
        }
     } else {    /* IBM device and/or standard play audio scsi cmds required */
        init_iocmd(&iocmd_buf); /* send audio track play cmd to set last trk */
        iocmd_buf.command_length = 10;
        iocmd_buf.scsi_cdb[0] = 0x48;               /*--- opcode for play ---*/
        iocmd_buf.scsi_cdb[1] = 0;
        iocmd_buf.scsi_cdb[2] = 0;
        iocmd_buf.scsi_cdb[3] = 0;
        iocmd_buf.scsi_cdb[4] = to_bcd(*blkno);          /*- starting track -*/
        iocmd_buf.scsi_cdb[5] = 1;                       /*  starting index  */
        iocmd_buf.scsi_cdb[6] = 0;
        iocmd_buf.scsi_cdb[7] = to_bcd(*blkno + pr->num_blks); /* ending trk */
        iocmd_buf.scsi_cdb[8] = 1;                        /*  ending index   */
        iocmd_buf.scsi_cdb[9] = 0;
        rc = ioctl(pr->fildes, CDIOCMD, &iocmd_buf);
        if ( rc == -1 ) {
           ps->bad_others = ps->bad_others + 1;
           prt_msg(ps, pr, loop, blkno, errno, HARD,
                   "standard play audio play error - ");
        } else {
           pr->op_in_progress = 2;          /* set scsi audio op in progress */
           sense_buf [1] = 0;                     /* wait for play to finish */
           while ( (sense_buf[1] != 0x15) & (rc != -1) ) { /* while not done */
           init_iocmd(&iocmd_buf);
           iocmd_buf.command_length = 10;
           iocmd_buf.data_length = 0x10;          /*--- use sense buffer ---*/
           iocmd_buf.flags = B_READ;
           iocmd_buf.buffer = sense_buf;
           iocmd_buf.scsi_cdb[0] = 0x42;
           iocmd_buf.scsi_cdb[1] = 0;                   /*--- # of bytes ---*/
           iocmd_buf.scsi_cdb[2] = 0x40;
           iocmd_buf.scsi_cdb[3] = 0x01;
           iocmd_buf.scsi_cdb[4] = 0;
           iocmd_buf.scsi_cdb[5] = 0;
           iocmd_buf.scsi_cdb[6] = to_bcd(*blkno);
           iocmd_buf.scsi_cdb[7] = 0;
           iocmd_buf.scsi_cdb[8] = 0x10;
           iocmd_buf.scsi_cdb[9] = 0;
           rc = ioctl(pr->fildes, CDIOCMD, &iocmd_buf);
           if ( rc == -1 ) {
              ps->bad_others = ps->bad_others + 1;
              prt_msg(ps, pr, loop, blkno, errno, HARD,
                      "standard play audio wait error - ");
           }
           hxfupdate(UPDATE,ps);
           time(&t1);                               /*--- wait 5 seconds ---*/
           while ( (time(NULL) - t1 < 5) & (rc != -1) ) ;
        }
        pr->op_in_progress = 0;              /* set no audio op in progress */
     }
  }
  if ( rc == -1 ) {               /*- do a request sense for info on error -*/
     sense_buf[7] = 0;        /* reset sense buffer's additional sense byte */
     xrc = do_req_sense(pr->fildes, sense_buf, sizeof(sense_buf));
     if ( xrc == 0 ) {        /* print out sense date if on last retry only */
        strcpy(msg_str, "Request Sense Data:");
        sens_len = (8 + sense_buf[7]);
        for ( i = 0; i < sens_len; i++ ) {
           if ( (i & 15) == 0 ) {
              sprintf(tmp_str, "\n    [%02X] ", i);
              strcat(msg_str,tmp_str);
           }
           sprintf(tmp_str, "%02X ", sense_buf[i]);
           strcat(msg_str, tmp_str);
        }
        prt_msg(ps, pr, loop, blkno, errno, HARD, msg_str);
     } else {
        strcpy(msg_str, " request sense error after audio command error - ");
        prt_msg(ps, pr, loop, blkno, errno, HARD, msg_str);
     }
  }
  if ( close_reopen(ps, pr, SC_SINGLE, "audio_cdrom") != 0 )
     prt_msg(ps, pr, loop, blkno, errno, HARD, "reopen failure - ");
  }
}
#endif

/**************************************************************************/
/* audio multimedia - play audio using multimedia mode of device driver   */
/**************************************************************************/
int audio_mm(struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno)
{
#ifndef __HTX_LINUX__ /* AIX */
  int    rc, i, num_blks;
  char   tmp_str[3], tmsg[100], buf[1024];
  FILE   *fp;
  uchar  first_track, first_index, last_track, last_index;
  time_t t1;
  struct cd_audio_cmd audio_ctl;
  char pa_failreason[20] = {0}; /* to hold the play audio failure reason */
  int pa_failflag = 0;

  first_track = pr->first_block & 0xff;
  num_blks = pr->num_blks;
  if ( num_blks > 0 )
     num_blks--;
  last_track = (num_blks + first_track) & 0xff;
  memset(&audio_ctl,0,sizeof(struct cd_audio_cmd));
  audio_ctl.audio_cmds = CD_PLAY_AUDIO;
  switch ( pr->msf_mode ) {
     case CD_MSF_MODE : {
		 int mm, ss, bb;	/* temps for lba to ss:mm:bb transform */
		 audio_ctl.msf_flag = 1;   /* msf flag = true, use msf  */
		 /* convert starting block to mm:ss:ff format */
		 ss = (pr->first_block + 150) / 75; /* the +150 compensates for a kluge   */
		 bb = (pr->first_block + 150) % 75; /* get_rule.c line 207, this way the  */
						/* mm:ss:ff can be correct in the     */
						/* rules file */
		 mm = ss / 60;
		 ss = ss % 60;
		 audio_ctl.indexing.msf.first_mins = (mm & 0xff);
		 audio_ctl.indexing.msf.first_secs = (ss & 0xff);
		 audio_ctl.indexing.msf.first_frames = (bb & 0xff);
		 /* convert number of blocks to last mm:ss:ff format */
		 ss = (pr->num_blks - 1) / 75;
		 bb = (pr->num_blks - 1) % 75;
		 mm = ss / 60;
		 ss = ss % 60;
		 audio_ctl.indexing.msf.last_mins = audio_ctl.indexing.msf.first_mins + (mm & 0xff);
		 audio_ctl.indexing.msf.last_secs = audio_ctl.indexing.msf.first_secs + (ss & 0xff);
		 audio_ctl.indexing.msf.last_frames = audio_ctl.indexing.msf.first_frames + (bb & 0xff);
	     pr->msf_mode = 1;      /* true = mm:ss:ff format for lba       */
		 break;
     }
     default:
        audio_ctl.msf_flag = 0;              /* msf flag = false, use trk # */
        audio_ctl.indexing.track_index.first_track = first_track;
        audio_ctl.indexing.track_index.first_index = DEFAULT_TRK_INDEX;
        audio_ctl.indexing.track_index.last_track =  last_track;
        audio_ctl.indexing.track_index.last_index = DEFAULT_TRK_INDEX;
	    pr->msf_mode = 0;                 /* true = mm:ss:ff format for lba       */
  }

  rc = ioctl(pr->fildes, DKAUDIO, &audio_ctl);

  if ( rc == -1 ) {
	 if( (EINVAL == errno) && (pr->msf_mode = CD_MSF_MODE ) && (device_subclass[0] == 'S') ) {
		 /* Try issuing Play Audio MSF passthru command. */
		 /* This is required since in newer SCSI drives ( Witchita ) play audio flag in C7 inquiry page is turned off */
		 /* So DKAUDIO can potentially fail with EINVAL as the errno, altho' the drive/system supports multimedia */
		 /* Hence passthru is the only way to test the play audio stanza */
		 /* See F432984 for more details */

         if ( close_reopen(ps, pr, SC_DIAGNOSTIC, "audio_mm") != 0 ) {
		 	prt_msg(ps, pr, loop, blkno, errno, HARD, "open in diagnostic mode failure - ");
		 	return 0;
		 }

		 rc = play_audio_msf( ps, pr, loop, blkno, &audio_ctl );

		 switch(rc) {
			 case 0x13: {
				 pr->op_in_progress = 0; /* set no audio op in progress */
				 break;	 				/* play audio successful, return success */
			 }

			 case 0x14: {					/* play audio stopped due to error */
			 		pr->op_in_progress = 0; /* set no audio op in progress */
					ps->bad_others = ps->bad_others + 1;
					prt_msg(ps, pr, loop, blkno, errno, HARD,
							"multimedia play audio (ioctl) stopped on error - ");
					break;
			 }

			 case -2: {						/* read sub-channel failed */
					ps->bad_others = ps->bad_others + 1;
					prt_msg(ps, pr, loop, blkno, errno, HARD,
							"multimedia play audio (ioctl) wait error - ");
					break;
			 }

			 default: {						/* play audio passthru ioctl failed ... etc */
					 ps->bad_others = ps->bad_others + 1;
					 prt_msg(ps, pr, loop, blkno, errno, HARD,
							 "ioctl: multimedia play audio (ioctl) error - ");
					 break;
			 }

		 }
		  if ( close_reopen(ps, pr, SC_SINGLE, "audio_mm") != 0 ) {
			 prt_msg(ps, pr, loop, blkno, errno, HARD, "reopen failure - ");
		  }

		  return 0;

 	 }

	 ps->bad_others = ps->bad_others + 1;
	 prt_msg(ps, pr, loop, blkno, errno, HARD,
			 "ioctl: multimedia play audio error - ");
	 return(0);
  }
  pr->op_in_progress = 3;  /* set device driver CD_PLAY_AUDIO op in progress */
  rc = 0;
  audio_ctl.status = 0;
                                       /*--- wait for play to be finished ---*/
  while ( (audio_ctl.status != CD_COMPLETED) && (rc != -1) ) {
     memset(&audio_ctl, 0, sizeof(struct cd_audio_cmd));
     audio_ctl.audio_cmds = CD_INFO_AUDIO;
     rc = ioctl(pr->fildes, DKAUDIO, &audio_ctl);
     if ( rc == -1 ) {
        ps->bad_others = ps->bad_others + 1;
        prt_msg(ps, pr, loop, blkno, errno, HARD,
                "multimedia play audio wait error - ");
        return(0);
     }
     hxfupdate(UPDATE,ps);

     /* Check whether play audio failed, which is possible in case the media is bad.
     If we do not check for this, then we would infinitely get hung in this as we
     never check for errors */

     /* If there are no errors, but the play audio is still not finished, then status
     should return CD_PLAY_AUDIO. This was confirmed to be the case, after talking
     with the DD folks */

     pa_failflag = 0;
     switch ( audio_ctl.status ) {
		 case CD_NO_AUDIO: 		strcpy( pa_failreason, "CD_NO_AUDIO"); pa_failflag = 1; break;
		 case CD_STATUS_ERROR: 	strcpy( pa_failreason, "CD_STATUS_ERROR"); pa_failflag = 1; break;
		 case CD_NOT_VALID: 	strcpy( pa_failreason, "CD_NOT_VALID"); pa_failflag = 1; break;

		 default:
			 if ( audio_ctl.status != CD_COMPLETED ) {
				time(&t1);                                    /*--- wait 5 seconds ---*/
				while ( time(NULL) - t1 < 5 ) ;
			 }
		 }

		 if( pa_failflag ) break;
  }
  pr->op_in_progress = 0; /* set no audio op in progress */
  switch ( audio_ctl.status ) {
     case CD_COMPLETED :     /*-- 0xa  Play operation successfully completed.*/
        break;
     default :                            /*-- flag all status errors here --*/
        ps->bad_others = ps->bad_others + 1;
        sprintf(tmsg, "multimedia audio status error [%02X], reason = %s ",
                audio_ctl.status, pa_failreason);
        prt_msg_asis(ps, pr, loop, blkno, errno, HARD, tmsg);
        return(0);
  }

#else  /* Linux */

  struct cdrom_msf cdmsf;
  struct cdrom_subchnl cdsc;
  int lba, rc;
  char tmsg[100];

  memset(&cdmsf, 0, sizeof(struct cdrom_msf));
  memset(&cdsc, 0, sizeof(struct cdrom_subchnl));
  switch( pr->msf_mode ) {
      case CD_MSF_MODE:
            /* Convert start LBA */
            lba = pr->first_block + CD_MSF_OFFSET;
            cdmsf.cdmsf_min0 = lba / (CD_SECS*CD_FRAMES);
            lba %=  (CD_SECS*CD_FRAMES);
            cdmsf.cdmsf_sec0 = lba / CD_FRAMES;
            cdmsf.cdmsf_frame0 = lba % CD_FRAMES;
            /* Convert End LBA -- start + num blks */
            lba = pr->first_block + pr->num_blks - 1 + CD_MSF_OFFSET;
            cdmsf.cdmsf_min1 = lba / (CD_SECS*CD_FRAMES);
            lba %=  (CD_SECS*CD_FRAMES);
            cdmsf.cdmsf_sec1 = lba / CD_FRAMES;
            cdmsf.cdmsf_frame1 = lba % CD_FRAMES;
            break;

      default:
            prt_msg(ps, pr, loop, blkno, errno, HARD,
             "Play audio rule not using correct MSF or LBA");
  }

  rc = ioctl(pr->fildes, CDROMPLAYMSF, &cdmsf);
  if ( rc == -1 ) {
     ps->bad_others = ps->bad_others + 1;
     prt_msg(ps, pr, loop, blkno, errno, HARD,
             "ioctl CDROMPLAYMSF: multimedia play audio error - ");
     return(0);
  }

  pr->op_in_progress = 3;  /* set device driver CD_PLAY_AUDIO op in progress */
  cdsc.cdsc_audiostatus = CDROM_AUDIO_PLAY;
  rc = 0;
  while((cdsc.cdsc_audiostatus == CDROM_AUDIO_PLAY) && (rc != -1)) {
     memset(&cdsc, 0, sizeof(struct cdrom_subchnl));
     cdsc.cdsc_format = CDROM_MSF;
     rc = ioctl(pr->fildes, CDROMSUBCHNL, &cdsc);
     hxfupdate(UPDATE,ps);
     sleep(5);
  }

  pr->op_in_progress = 0; /* set no audio op in progress */
  if ( rc == -1 ) {
     ps->bad_others = ps->bad_others + 1;
     prt_msg(ps, pr, loop, blkno, errno, HARD,
             "ioctl CDROMSUBCHNL: multimedia play audio wait error - ");
     return(0);
  }

  if(cdsc.cdsc_audiostatus == CDROM_AUDIO_ERROR) {
     ps->bad_others = ps->bad_others + 1;
     sprintf(tmsg, "multimedia audio status error [%02X] ",
                cdsc.cdsc_audiostatus);
     prt_msg(ps, pr, loop, blkno, errno, HARD, tmsg);
     return(0);
  }

  return 0;
#endif
}

/**************************************************************************/
/* init_iocmd -- sets up a scsi pass through command                      */
/**************************************************************************/
#ifndef __HTX_LINUX__
init_iocmd(struct sc_iocmd *iocmd_buf)
{
   memset(iocmd_buf, 0, sizeof(struct sc_iocmd));
   iocmd_buf->buffer = NULL;
   iocmd_buf->timeout_value = 30;
   iocmd_buf->status_validity = 0;
   iocmd_buf->scsi_bus_status = 0;
   iocmd_buf->adapter_status = 0;
   iocmd_buf->command_length = 6;
   iocmd_buf->flags = 0;
}

/**************************************************************************/
/* init_viocmd -- sets up a scsi pass through command                      */
/**************************************************************************/
init_viocmd(struct scsi_iocmd *viocmd_buf)
{
   memset(viocmd_buf, 0, sizeof(struct scsi_iocmd));
#ifndef __HTX43X__
   viocmd_buf->version = SCSI_VERSION_2;
#endif
   viocmd_buf->buffer = NULL;
   viocmd_buf->timeout_value = 30;
   viocmd_buf->status_validity = 0;
   viocmd_buf->scsi_bus_status = 0;
   viocmd_buf->adapter_status = 0;
   viocmd_buf->command_length = 6;
   viocmd_buf->flags = 0;
}

#endif

/**************************************************************************/
/* to_bcd -- convert an integer to bcd                                    */
/**************************************************************************/
int to_bcd(int i)
{
  return( (16 * (i / 10)) + (i % 10) );
}

/**************************************************************************/
/* do diagnostics                                                         */
/**************************************************************************/
void diag_cdrom(struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno)
{
#ifndef __HTX_LINUX__
  int    rc, i;
  char   tmp_str [3];
  struct sc_iocmd iocmd_buf;
  struct scsi_iocmd viocmd_buf;

  if ( close_reopen(ps, pr, SC_DIAGNOSTIC, "diag_cdrom") != 0 )
     prt_msg(ps, pr, loop, blkno, errno, HARD, "diagnostic error - ");
  else {
   if ( device_subclass[0] == 'S' ) { /* scsi */
                                        /*--- send do diagnostics command ---*/
     iocmd_buf.data_length = 0;
     iocmd_buf.buffer = NULL;
     iocmd_buf.timeout_value = 30;
     iocmd_buf.flags = B_READ;       /*-- use active device driver mode --*/
     iocmd_buf.status_validity = 0;
     iocmd_buf.scsi_bus_status = 0;
     iocmd_buf.adapter_status = 0;
     iocmd_buf.command_length = 6;
     iocmd_buf.scsi_cdb[0] = 0x1d;
     iocmd_buf.scsi_cdb[1] = 4;
     iocmd_buf.scsi_cdb[2] = 0;
     iocmd_buf.scsi_cdb[3] = 0;
     iocmd_buf.scsi_cdb[4] = 0;
     iocmd_buf.scsi_cdb[5] = 0;
     rc = ioctl(pr->fildes, DKIOCMD, &iocmd_buf);
    }
   else if ( device_subclass[0] == 'V' ) { /* vscsi */
     /* printf("\ndiag_cdrom, vscsi setup, using DKIOLCMD for do diagnostics command\n"); */
     init_viocmd(&viocmd_buf);
     viocmd_buf.data_length = 0;
     viocmd_buf.buffer = NULL;
     viocmd_buf.timeout_value = 30;
     viocmd_buf.flags = B_READ;       /*-- use active device driver mode --*/
     viocmd_buf.status_validity = 0;
     viocmd_buf.scsi_bus_status = 0;
     viocmd_buf.adapter_status = 0;
     viocmd_buf.command_length = 6;
     viocmd_buf.scsi_cdb[0] = 0x1d;
     viocmd_buf.scsi_cdb[1] = 4;
     viocmd_buf.scsi_cdb[2] = 0;
     viocmd_buf.scsi_cdb[3] = 0;
     viocmd_buf.scsi_cdb[4] = 0;
     viocmd_buf.scsi_cdb[5] = 0;
     rc = ioctl(pr->fildes, DKIOLCMD, &viocmd_buf);
    }

     if ( rc == -1 ) {
        ps->bad_others = ps->bad_others + 1;
        prt_msg(ps, pr, loop, blkno, 0, HARD, "send diagnostics error - ");
     }
     if ( close_reopen(ps, pr, SC_SINGLE, "diag_cdrom") != 0 )
        prt_msg(ps, pr, loop, blkno, errno, HARD, "reopen failure - ");
  }
#endif
}

/**************************************************************************/
/* sleep                                                                  */
/**************************************************************************/
void do_sleep(struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno)
{
  time_t t1;

  time(&t1);
  while ( time(NULL) - t1 < pr->num_blks ) {
     hxfupdate(UPDATE,ps);
  }
}

/**************************************************************************/
/* prt_req_sense -- print the request sense data                         */
/**************************************************************************/
void prt_req_sense(struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno)
{
#ifndef __HTX_LINUX__
  int    rc, i;
  char   tmp_str[3];
  struct ide_atapi_passthru atapi_pt;
  struct sc_iocmd iocmd_buf;
  struct scsi_iocmd viocmd_buf;

  if ( close_reopen(ps, pr, SC_DIAGNOSTIC, "prt_req_sense") != 0 )
     prt_msg(ps, pr, loop, blkno, errno, HARD, "request sense error - ");
  else {
     if ( device_subclass[0] == 'S' ) { /* scsi */
		 iocmd_buf.data_length = sizeof(sense_buf);
		 iocmd_buf.buffer = sense_buf;
		 iocmd_buf.timeout_value = 5;
		 iocmd_buf.flags = B_READ;       /*-- use active device driver mode --*/
		 iocmd_buf.status_validity = 0;
		 iocmd_buf.scsi_bus_status = 0;
		 iocmd_buf.adapter_status = 0;
		 iocmd_buf.command_length = 6;
		 iocmd_buf.scsi_cdb[0] = 3;
		 iocmd_buf.scsi_cdb[1] = 0;
		 iocmd_buf.scsi_cdb[2] = 0;
		 iocmd_buf.scsi_cdb[3] = 0;
		 iocmd_buf.scsi_cdb[4] = MAX_SENSE;
		 iocmd_buf.scsi_cdb[5] = 0;
		 rc = ioctl(pr->fildes, DKIOCMD, &iocmd_buf);
	 }
	else if ( device_subclass[0] == 'V' ) { /* vscsi */
		 /* printf("\n prt_req_sense, VIOS setup, using DKIOLCMD to get request sense data\n"); */
		 init_viocmd(&viocmd_buf);
		 viocmd_buf.data_length = sizeof(sense_buf);
                 viocmd_buf.buffer = sense_buf;
                 viocmd_buf.timeout_value = 5;
                 viocmd_buf.flags = B_READ;       /*-- use active device driver mode --*/
                 viocmd_buf.status_validity = 0;
                 viocmd_buf.scsi_bus_status = 0;
                 viocmd_buf.adapter_status = 0;
                 viocmd_buf.command_length = 6;
                 viocmd_buf.scsi_cdb[0] = 3;
                 viocmd_buf.scsi_cdb[1] = 0;
                 viocmd_buf.scsi_cdb[2] = 0;
                 viocmd_buf.scsi_cdb[3] = 0;
                 viocmd_buf.scsi_cdb[4] = MAX_SENSE;
                 viocmd_buf.scsi_cdb[5] = 0;
                 rc = ioctl(pr->fildes, DKIOLCMD, &viocmd_buf);
	 }

	else { /* ide atapi */
		 memset(&atapi_pt, 0, sizeof(struct ide_atapi_passthru));
		 memset(sense_buf, 0, MAX_SENSE);
		 atapi_pt.ide_device 		= device_st_rdev & 0x1;
		 atapi_pt.flags 			= 0x4;	/* IDE_PASSTHRU_READ */
		 atapi_pt.timeout_value	= 5; /* seconds */
		 atapi_pt.buffsize			= sizeof(sense_buf);
		 atapi_pt.data_ptr			= sense_buf;
		 atapi_pt.atapi_cmd.length = 12;
		 atapi_pt.atapi_cmd.packet.op_code = 0x3;
		 atapi_pt.atapi_cmd.packet.bytes[3] = MAX_SENSE;
	     rc = ioctl(pr->fildes, IDEPASSTHRU, &atapi_pt);
	 }

     if ( rc == -1 ) {
        ps->bad_others = ps->bad_others + 1;
        prt_msg(ps, pr, loop, blkno, errno, HARD, "request sense error - ");
     } else {
        strcpy(msg_str, "request sense_buf:");
        for ( i = 0; i < iocmd_buf.data_length; i++ ) {
           sprintf(tmp_str, i%16 ? " %2x ":"\n%2x", sense_buf[i]);
           strcat(msg_str, tmp_str);
        }
        hxfmsg(ps,rc,INFO,msg_str);
     }
     if ( close_reopen(ps, pr, SC_SINGLE, "prt_req_sense") != 0 )
        prt_msg(ps, pr, loop, blkno, errno, HARD, "reopen failure - ");
  }
#endif
}

/**************************************************************************/
/* mode select / get parms                                                */
/**************************************************************************/
int ms_get(struct ruleinfo *pr, struct htx_data *ps, unsigned int *p_last_lba,
           unsigned int *p_blk_size)
{
  int rc=0;
  char   msg[150], tmp_msg[150];
  char   sMode[10], mode_descr[40];
  unsigned char  cd_mode_val;
#ifndef __HTX_LINUX__
  struct mode_form_op dk_cd_mode;
#endif
     /************************************************************************/
     /* Structure for CD-ROM Data Modes                                      */
     /************************************************************************/
     /*      uchar   action;                  * Determines if this operation */
     /*                                       * is to get the current CD-ROM */
     /*                                       * data mode or to change it to */
     /*                                       * the values specified  below. */
     /*define CD_GET_MODE             0x1     * Get current CD-ROM data mode.*/
     /*define CD_CHG_MODE             0x2     * Change CD-ROM data mode.     */
     /*      uchar   cd_mode_form;            * Specifies the CD-ROM data    */
     /*                                       * mode                         */
     /*define CD_MODE1                0x1     * CD-ROM Data Mode 1.          */
     /*define CD_MODE2_FORM1          0x2     * CD-ROM XA Data Mode 2 Form 1 */
     /*                                       * The device block size used   */
     /*                                       * for this mode is 2048 bytes  */
     /*                                       * per block.                   */
     /*define CD_MODE2_FORM2          0x3     * CD-ROM XA Data Mode 2 Form 2 */
     /*                                       * The device block size used   */
     /*                                       * for this mode is 2336 bytes  */
     /*                                       * per block.                   */
     /*define CD_DA                   0x4     * CD-DA. The device block size */
     /*                                       * used for this mode is 2352   */
     /*                                       * bytes per block.             */
     /************************************************************************/
  strcpy(sMode, pr->mode);                   /* get mode select string value */
  if ( strcmp(sMode, "M1") == 0 )
     cd_mode_val = CD_MODE1;
  else if ( strcmp(sMode, "M2F1") == 0 )
     cd_mode_val = CD_MODE2_FORM1;
  else if ( strcmp(sMode, "M2F2") == 0 )
     cd_mode_val = CD_MODE2_FORM2;
  else if ( strcmp(sMode, "DA") == 0 )
     cd_mode_val = CD_DA;
  else {
     sprintf(msg, "Invalid Mode Select Requested - %s) \n", sMode);
     hxfmsg(ps, 0, SYSERR, msg);
     return(-1);
  }
  switch ( cd_mode_val ) {
     case CD_MODE1       : { strcpy(mode_descr, "CD-ROM Data Mode 1");
                             break;
     }
     case CD_MODE2_FORM1 : { strcpy(mode_descr, "CD-ROM XA Data Mode 2 Form 1");
                             break;
     }
     case CD_MODE2_FORM2 : { strcpy(mode_descr, "CD-ROM XA Data Mode 2 Form 2");
                             break;
     }
     case CD_DA          : { strcpy(mode_descr,"CD-DA Mode");
                             break;
     }
     default : strcpy(mode_descr, "UNDEFINED Mode");
  }
  rc = 0;
#ifndef __HTX_LINUX__

  /* Support for Liberty IDE DVDROM drive where Mode 1 on DVD Media is NOT supported */
  /* Try to change to DVD_ROM mode, and if you recv EINVAL, then its NOT a liberty drive */
  /* In such a case, fall back to Mode 1 as that is true for CDROM and DVDROM media */

  if( cd_mode_val == CD_MODE1 ) {

	  cd_mode_val = DVD_ROM;

	  strcpy(mode_descr,"DVD_ROM Mode");

	  dk_cd_mode.action = CD_CHG_MODE;
	  dk_cd_mode.cd_mode_form = cd_mode_val;
	  dk_cd_mode.resvd1 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd2 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd3 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd4 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd5 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd6 = 0;             /* reserved for future expansion*/

	  rc = ioctl(pr->fildes, DK_CD_MODE, &dk_cd_mode);

	  if( ( rc == -1 ) && ( errno == EINVAL ) ) {

		  sprintf( msg, "DVD_ROM mode change failed... Not a liberty drive or DVDROM media\n");
		  hxfmsg(ps, errno, INFO, msg);

		  cd_mode_val = CD_MODE1; /* fall back */

		  strcpy(mode_descr,"CD-ROM Data Mode 1");

		  dk_cd_mode.action = CD_CHG_MODE;
		  dk_cd_mode.cd_mode_form = cd_mode_val;
		  dk_cd_mode.resvd1 = 0;             /* reserved for future expansion*/
		  dk_cd_mode.resvd2 = 0;             /* reserved for future expansion*/
		  dk_cd_mode.resvd3 = 0;             /* reserved for future expansion*/
		  dk_cd_mode.resvd4 = 0;             /* reserved for future expansion*/
		  dk_cd_mode.resvd5 = 0;             /* reserved for future expansion*/
		  dk_cd_mode.resvd6 = 0;             /* reserved for future expansion*/

		  rc = ioctl(pr->fildes, DK_CD_MODE, &dk_cd_mode);

	  } else {

		  sprintf( msg, "DVD_ROM mode change success... This is a liberty drive !\n");
		  hxfmsg(ps, errno, INFO, msg);

	  }
  } else {

	  dk_cd_mode.action = CD_CHG_MODE;
	  dk_cd_mode.cd_mode_form = cd_mode_val;
	  dk_cd_mode.resvd1 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd2 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd3 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd4 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd5 = 0;             /* reserved for future expansion*/
	  dk_cd_mode.resvd6 = 0;             /* reserved for future expansion*/

	  rc = ioctl(pr->fildes, DK_CD_MODE, &dk_cd_mode);

  }

#endif
  if ( rc != 0 ) {
     sprintf(msg, "%s : Mode Select error : %s.\n", pr->rule_id, mode_descr);
        strcat(msg, strerror(errno));
     strcat(msg,"\n");
     hxfmsg(ps, errno, HARD, msg);
     rc = -1;
  } else {
     sprintf(tmp_msg, "%s : Mode Select : ", pr->rule_id);
  rc = 0;
#ifndef __HTX_LINUX__
     dk_cd_mode.action = CD_GET_MODE;                    /* get current mode */
     rc = ioctl(pr->fildes, DK_CD_MODE, &dk_cd_mode);
     cd_mode_val = dk_cd_mode.cd_mode_form;
#endif
     if ( rc != 0 ) {
        sprintf(msg, "Mode Get error : %s.\n", mode_descr);
        strcat(tmp_msg, msg);
           strcat(tmp_msg, strerror(errno));
        strcat(tmp_msg,"\n");
        hxfmsg(ps, errno, HARD, tmp_msg);
        rc = -1;
     } else {
        switch ( cd_mode_val ) {
           case CD_MODE1       : { strcpy(mode_descr, "Mode 1");
                                   *p_blk_size = cd_BLOCKSIZE;
                                   break;
           }
           case CD_MODE2_FORM1 : { strcpy(mode_descr, "XA-Mode 2 Form 1");
                                   *p_blk_size = cd_M2F1_BLKSIZE;
                                   break;
           }
           case CD_MODE2_FORM2 : { strcpy(mode_descr, "XA-Mode 2 Form 2");
                                   *p_blk_size = cd_M2F2_BLKSIZE;
                                   break;
           }
           case CD_DA          : { strcpy(mode_descr, "DA Mode");
                                   *p_blk_size = cd_CDDA_BLKSIZE;
                                   break;
           }

	#ifndef __HTX_LINUX__
	   	   case DVD_ROM	       : { strcpy(mode_descr, "DVD_ROM Mode");
          	 		   			   *p_blk_size = cd_BLOCKSIZE; /* DVD blksize is the same as CD Mode1 blksize=2048*/
                                   break;
	       }
	#endif

           default : strcpy(mode_descr, "UNDEFINED Mode");
                     *p_blk_size = 0;
        }
        pr->bytpsec  = *p_blk_size;
        *p_last_lba  = pr->tot_blks;  /* This function does not find
                                         last lba. Use get_disc_capacity.
                                         pr->tot_blks is just returned. */
        if ( *p_blk_size == 0 ) {
           sprintf(msg, "Mode Get error : %s.\n", mode_descr);
           strcat(tmp_msg, msg);
              strcat(tmp_msg, strerror(errno));
           strcat(tmp_msg,"\n");
           hxfmsg(ps, errno, HARD, tmp_msg);
           rc = -1;
        } else {
           sprintf(msg, "%s  Last lba = %ld  Blksize = %d.\n",
                   mode_descr, pr->tot_blks, *p_blk_size);
           strcat(tmp_msg, msg);
           hxfmsg(ps, errno, INFO, tmp_msg);
        }
     }
  }
  return(rc);

}

/**************************************************************************/
/* show_stuff - show iocmd_buf present values and do a request sense     */
/**************************************************************************/
#ifndef __HTX_LINUX__
void show_stuff(struct htx_data *ps, struct ruleinfo *pr, int loop,
           int *blkno, struct sc_iocmd *piocmd_buf)
{
   int    rc, i;
   char   tmp_str[100];
   struct sc_iocmd iocmd_buf;

   strcpy(msg_str, "iocmd_buf:\n");         /* show present iocmd_buf values */
   sprintf(tmp_str, " data_length[%d] timeout_value[%d] flags[x%02X] "
                    " command_length[%d] \n",
           iocmd_buf.data_length, iocmd_buf.timeout_value, iocmd_buf.flags,
           iocmd_buf.command_length);
   strcat(msg_str, tmp_str);
   sprintf(tmp_str, " status_validity[x%02X] scsi_bus_status[x%02X] "
                    "adapter_status[x%02X] \n",
           iocmd_buf.status_validity, iocmd_buf.scsi_bus_status,
           iocmd_buf.adapter_status);
   strcat(msg_str, tmp_str);
   strcpy(tmp_str, "scsi_cdb: ");
   strcat(msg_str, tmp_str);
   for ( i = 0; i < iocmd_buf.command_length; i++ ) {
      sprintf(tmp_str, "%02X ", iocmd_buf.scsi_cdb[i]);
      strcat(msg_str, tmp_str);
   }
   strcpy(tmp_str, "\n");
   strcat(msg_str, tmp_str);
   hxfmsg(ps, 0, INFO, msg_str);
   if ( close_reopen(ps, pr, SC_DIAGNOSTIC, "show_stuff") != 0 )
      prt_msg(ps, pr, loop, blkno, errno, HARD,
              "open in diagnostic mode failure - ");
   else {
      iocmd_buf.data_length = sizeof(sense_buf);    /* send req sns command */
      iocmd_buf.buffer = sense_buf;
      iocmd_buf.timeout_value = 5;
      iocmd_buf.flags = B_READ;
      iocmd_buf.status_validity = 0;
      iocmd_buf.scsi_bus_status = 0;
      iocmd_buf.adapter_status = 0;
      iocmd_buf.command_length = 6;
      iocmd_buf.scsi_cdb[0] = 3;
      iocmd_buf.scsi_cdb[1] = 0;
      iocmd_buf.scsi_cdb[2] = 0;
      iocmd_buf.scsi_cdb[3] = 0;
      iocmd_buf.scsi_cdb[4] = MAX_SENSE;
      iocmd_buf.scsi_cdb[5] = 0;
      rc = ioctl(pr->fildes, DKIOCMD, &iocmd_buf);
      if ( rc == -1 ) {
         ps->bad_others = ps->bad_others + 1;
         prt_msg(ps, pr, loop, blkno, errno, HARD,
                 "show_stuff : request sense error - ");
      } else {
         strcpy(msg_str, "show_stuff : request sense_buf:");
         for ( i = 0; i < iocmd_buf.data_length; i++ ) {
            sprintf(tmp_str, i%16 ? " %2x ":"\n%2x", sense_buf[i]);
            strcat(msg_str, tmp_str);
         }
         hxfmsg(ps, rc, INFO, msg_str);
      }
      if ( close_reopen(ps, pr, SC_SINGLE, "show_stuff") != 0 )
         prt_msg(ps, pr, loop, blkno, errno, HARD, "reopen failure - ");
   }
}
#endif

/**************************************************************************/
/* do_req_sense -- do a request sense operation and return sense data     */
/**************************************************************************/
int do_req_sense(int fildes, char *sense_buf, int sense_buf_len)
{
#ifndef __HTX_LINUX__
  int    rc;
  struct sc_iocmd iocmd_buf;
  struct ide_atapi_passthru atapi_pt;
  struct scsi_iocmd viocmd_buf;

  if ( device_subclass[0] == 'S' ) { /* scsi */
	  init_iocmd(&iocmd_buf);
	  iocmd_buf.data_length = sense_buf_len;
	  iocmd_buf.buffer = sense_buf;
	  iocmd_buf.timeout_value = 5;
	  iocmd_buf.flags = B_READ;
	  iocmd_buf.command_length = 6;
	  iocmd_buf.scsi_cdb[0] = 3;
	  iocmd_buf.scsi_cdb[4] = MAX_SENSE;
	  rc = ioctl(fildes, DKIOCMD, &iocmd_buf);
  }
  else if ( device_subclass[0] == 'V' ) { /* vscsi */
	  printf("\ndo_req_sense, VIOD setup, using DKIOLCMD to obtain request sense data\n");
	  init_viocmd(&viocmd_buf);
	  viocmd_buf.data_length = sense_buf_len;
          viocmd_buf.buffer = sense_buf;
          viocmd_buf.timeout_value = 5;
          viocmd_buf.flags = B_READ;
          viocmd_buf.command_length = 6;
          viocmd_buf.scsi_cdb[0] = 3;
          viocmd_buf.scsi_cdb[4] = MAX_SENSE;
          rc = ioctl(fildes, DKIOLCMD, &viocmd_buf);
  }
  else { /* ide atapi */
	  memset(&atapi_pt, 0, sizeof(atapi_pt));
	  atapi_pt.ide_device 		= device_st_rdev && 0x1;
	  atapi_pt.flags 			= 0x4;	/* IDE_PASSTHRU_READ */
	  atapi_pt.timeout_value	= 10; /* seconds */
	  atapi_pt.buffsize			= sense_buf_len;
	  atapi_pt.data_ptr			= sense_buf;
	  atapi_pt.atapi_cmd.length = 12;
	  atapi_pt.atapi_cmd.packet.op_code = 3;
	  atapi_pt.atapi_cmd.packet.bytes[3] = MAX_SENSE;
	  rc = ioctl(fildes, IDEPASSTHRU, &atapi_pt);
  }
  return(rc);
#else
  return 0;
#endif
}

/**********************************************************************/
/* get_disc_pn - get disk part number by reading the disc table of    */
/* contents and comparing the toc data against known disc P/N data.   */
/* Returns: disc_pn = found part number of disc or Unknown.           */
/*          rc = 0 = found a supported part number.                   */
/*          rc = -1 = failure trying to acquire toc data.             */
/*          rc = -2 = found unsupported cdrom disc.                   */
/**********************************************************************/
int get_disc_pn(struct htx_data *ps, struct ruleinfo *pr)
{
#ifndef __HTX_LINUX__
               /***********************************************************/
               /*- known cdrom test disc'c toc data                      -*/
               /***********************************************************/
                                /*--- toc data for blocksize of 2048   ---*/
const char toc2048_53F3088[200] =
  "00420107001401000000000000100200000012C00014030000001C6B\
0010040000002F2B0010050000004155001006000000537F00140700\
000065F40014AA0000041BC2";
                                /*--- toc data for blocksize of 2048   ---*/
const char toc512_53F3088[200] =
  "0042010700140100000000000010020000004B0000140300000071AC\
001004000000BCAC00100500000105540010060000014DFC00140700\
000197D00014AA0000106F08";
                                 /*--- toc data for blocksize of 512    ---*/
const char toc512_81F8902[200] =
  "0042010700140100000000000010020000004B0000140300000071AC\
001004000000BCAC00100500000105540014060000014F2800140700\
0010278C0014AA0000106F08";
                                  /*--- toc data for blocksize of 2048   ---*/
const char toc2048_81F8902[200] =
  "00420107001401000000000000100200000012C00014030000001C6B\
0010040000002F2B001005000000415500140600000053CA00140700\
000409E30014AA0000041BC2";

const char toc_dvd_03K998x[50] = "0012010100140100000000000014AA00003FA0E0";
#endif

#ifndef __HTX_LINUX__
   int    rc, i, xrc, x, xno;
   int    retry_op, toc_length, sens_len;
   char   ch2[4], parm_str[20], tmp_str[120], toc_dat[220], rbuf[220];
   struct ide_atapi_passthru atapi_pt;
   struct sc_iocmd iocmd_buf;
   struct scsi_iocmd viocmd_buf;

   /*char *rbuf, *alloc;
   alloc = (char *) malloc(2 * 0x1000); *//* 2 pages */
   /* Now align it */
   /*rbuf = (char *) (((unsigned long) alloc + 0x1000) & ~0xfff);*/
   /*printf("############## alloc = %p, rbuf = %p\n", alloc, rbuf);*/
   /*sprintf( msg_str, "############## alloc = %p, rbuf = %p\n", alloc, rbuf );
   hxfmsg(ps, 0, INFO, msg_str);*/

   xno = 0;
   if ( close_reopen(ps, pr, SC_DIAGNOSTIC, "get_disc_pn") != 0 )
      prt_msg(ps, pr, 0, &xno, errno, HARD, "open in get_disc_pn failure - ");
   else {
      if ( device_subclass[0] == 'S' ) { /* scsi */
		  init_iocmd(&iocmd_buf);       /* get toc information */
		  iocmd_buf.buffer = rbuf;
		  iocmd_buf.data_length = 200;
		  iocmd_buf.flags = B_READ;
		  iocmd_buf.scsi_cdb[0] = 0x43;
		  iocmd_buf.scsi_cdb[6] = 0x01;
		  iocmd_buf.scsi_cdb[8] = iocmd_buf.data_length;
		  iocmd_buf.command_length = 10;
	  }
      else if ( device_subclass[0] == 'V' ) { /* vscsi */
                  /* printf("\nget_disc_pn, vscsi Setup, using DKIOLCMD to get TOC\n"); */
                  init_viocmd(&viocmd_buf);       /* get toc information */
                  viocmd_buf.buffer = rbuf;
                  viocmd_buf.data_length = 200;
                  viocmd_buf.flags = B_READ;
                  viocmd_buf.scsi_cdb[0] = 0x43;
                  viocmd_buf.scsi_cdb[6] = 0x01;
                  viocmd_buf.scsi_cdb[8] = viocmd_buf.data_length;
                  viocmd_buf.command_length = 10;
          }
	else { /* ide atapi */
		  memset(&atapi_pt, 0, sizeof(struct ide_atapi_passthru));
		  atapi_pt.ide_device 		= device_st_rdev & 0x1;
		  atapi_pt.flags 			= 0x4;	/* IDE_PASSTHRU_READ */
		  atapi_pt.timeout_value	= 10; /* seconds */
		  atapi_pt.buffsize			= 2; /* get the first 2 bytes to determine the toc length */
		  atapi_pt.data_ptr			= rbuf;
		  atapi_pt.atapi_cmd.length = 12;
		  atapi_pt.atapi_cmd.packet.op_code = 0x43;
		  atapi_pt.atapi_cmd.packet.bytes[5] = 0x01;
		  atapi_pt.atapi_cmd.packet.bytes[7] = 2;

          sprintf( msg_str, "########### atapi_pt.atapi_cmd.packet.bytes[7] = %d bytes\n", atapi_pt.atapi_cmd.packet.bytes[7] );
          hxfmsg(ps, 0, INFO, msg_str);

		  rc = 0;
		  retry_op = 2;
		  while ( retry_op > 0 ) {
			 retry_op--;

			 rc = ioctl(pr->fildes, IDEPASSTHRU, &atapi_pt);

			 if ( rc == 0 )
				retry_op--;
			 else if ( rc == -1 ) {

				 sprintf( msg_str, "ioctl fail, errno = %d, retry left = %d \n", errno, retry_op);
				 hxfmsg(ps, 0, INFO, msg_str);

				rc = 0;
				sense_buf[7] = 0;  /* reset sense buffer's additional sense byte */
				xrc = do_req_sense(pr->fildes, sense_buf, sizeof(sense_buf));
				if ( (xrc == 0) && (retry_op == 0) ) {
				   sprintf(tmp_str, " Unable to determine toc_length, errno = %d\n", errno );
                   strcpy(msg_str, tmp_str);
					  strcat(msg_str, strerror(errno));
				   strcat(msg_str, "\n  Request Sense Data:");
				   sens_len = (8 + sense_buf[7]);
				   for ( i = 0; i < sens_len; i++ ) {
					  if ( (i & 15) == 0 ) {
						 sprintf(tmp_str, "\n    [%02X] ", i);
						 strcat(msg_str,tmp_str);
					  }
					  sprintf(tmp_str, "%02X ", sense_buf[i]);
					  strcat(msg_str, tmp_str);
				   }
				   hxfmsg(ps, 0, SOFT, msg_str);
				} else {
				   if ( retry_op == 0 ) {
					  strcpy(msg_str, " request sense error after read "
									  "table of contents for toc length retried once - ");
					  hxfmsg(ps, 0, HARD, msg_str);
				   }
				}
			 }
		  }
		   if( rc != 0 ) {
			   return (-1);
		   } else {
			 toc_length = (rbuf[0] * 256) + rbuf[1];
			 sprintf( msg_str, "######## toc_length = %d \n", toc_length  );
			 hxfmsg(ps, 0, INFO, msg_str);
			 sprintf(msg_str, "rbuf[0] = %d, rbuf[1] = %d, rbuf[2] = %d, rbuf[3] = %d\n",rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
			 hxfmsg(ps, 0, INFO, msg_str);

			/* This means that the ioctl returned 0 but the toc length obtained is 0. This might be a legacy Maverick1 drive.
			   Let's verify this by issuing READ-TOC with buffsize set to 220 bytes. Now, if we get the toc_length, it is
			   confirmed that the drive is Maverick1.
			*/

			if(toc_length == 0) {

			  sprintf( msg_str,"This might be Maverick1 drive, so now issuing READ TOC with buffsize as 220\n");
			  hxfmsg(ps, 0, INFO, msg_str);
			  memset(&atapi_pt, 0, sizeof(struct ide_atapi_passthru));
 	                  atapi_pt.ide_device           = device_st_rdev & 0x1;
         	          atapi_pt.flags                        = 0x4;  /* IDE_PASSTHRU_READ */
                 	  atapi_pt.timeout_value        = 10; /* seconds */
                 	  atapi_pt.buffsize                     = 220; /* passing the max rbuf length */
              	 	  atapi_pt.data_ptr                     = rbuf;
            	          atapi_pt.atapi_cmd.length = 12;
                 	  atapi_pt.atapi_cmd.packet.op_code = 0x43;
                 	  atapi_pt.atapi_cmd.packet.bytes[5] = 0x01;
                 	  atapi_pt.atapi_cmd.packet.bytes[7] = 220;

	                  sprintf( msg_str, "########### atapi_pt.atapi_cmd.packet.bytes[7] = %d bytes\n", atapi_pt.atapi_cmd.packet.bytes[7] );
          		  hxfmsg(ps, 0, INFO, msg_str);

                  	  rc = 0;
                  	  retry_op = 2;
                  	   while ( retry_op > 0 ) {
                         	retry_op--;

 	                        rc = ioctl(pr->fildes, IDEPASSTHRU, &atapi_pt);

				if ( rc == 0 )
                                retry_op--;
                         	else if ( rc == -1 ) {

                                    sprintf( msg_str, "ioctl fail, errno = %d, retry left = %d \n", errno, retry_op);
                                    hxfmsg(ps, 0, INFO, msg_str);

                                    rc = 0;
                                    sense_buf[7] = 0;  /* reset sense buffer's additional sense byte */
                                    xrc = do_req_sense(pr->fildes, sense_buf, sizeof(sense_buf));
                                    if ( (xrc == 0) && (retry_op == 0) ) {
                                       sprintf(tmp_str, " Unable to determine toc_length, errno = %d\n", errno );
                                       strcpy(msg_str, tmp_str);
                                          strcat(msg_str, strerror(errno));
                                          strcat(msg_str, "\n  Request Sense Data:");
                                          sens_len = (8 + sense_buf[7]);
                                             for ( i = 0; i < sens_len; i++ ) {
                                                if ( (i & 15) == 0 ) {
                                                  sprintf(tmp_str, "\n    [%02X] ", i);
                                                  strcat(msg_str,tmp_str);
                                                 }
                                              sprintf(tmp_str, "%02X ", sense_buf[i]);
                                              strcat(msg_str, tmp_str);
                                   	     }
                                   	hxfmsg(ps, 0, SOFT, msg_str);
                                    } else {
                                   	if ( retry_op == 0 ) {
                                          strcpy(msg_str, " request sense error after read "
                                                                          "table of contents for toc length retried once - ");
                                          hxfmsg(ps, 0, HARD, msg_str);
                                        }
                               	      }
                              }
                          }

          	         if( rc != 0 ) {
                           return (-1);
                   	 } else {
                           toc_length = (rbuf[0] * 256) + rbuf[1];
                           sprintf( msg_str, "######## toc_length after setting buffsize to 220 = %d \n", toc_length  );
                           hxfmsg(ps, 0, INFO, msg_str);
			   sprintf(msg_str, "rbuf[0] = %d, rbuf[1] = %d, rbuf[2] = %d, rbuf[3] = %d\n",rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
			   hxfmsg(ps, 0, INFO, msg_str);
			   /* In spite of setting the buffsize to 220, if we get the toc_length as 0, we return with -1 */

			   if(toc_length == 0) {
				strcpy(msg_str, "toc_length = 0 even after setting the buffsize to 220. Unable to dertemine the drive type..exiting\n");
				hxfmsg(ps, 0, HARD, msg_str);
				return -1;
			   }
			}


 		      } /* if toc_length = 0 when buf_length = 4 */

		   else {


                      /*******************************************************/
                      /*- perform the table of contents command             -*/
                      /*- if a check condition is sensed then request sense -*/
                      /*- data to clear conditon and retry the command once -*/
                      /*******************************************************/

	/* D406339 - It was observed that if we pass a buffer of larger size than reqd to read toc command, it fails. The fix was to pass
	the correct number of bytes in the size. In order to determine the toc length, which in turn depends on the media inserted, we need
	to first find the toc length, and then issue the read toc command with the correct length.

	The buffer size was never a issue before, but something seems to have changed at the driver level, for which we need this change.
	Also this issue was observed only on the IDE devices.
	*/

	  /* if ( device_subclass[0] == 'I' ) {  ide atapi */
		  sprintf(msg_str, "Trying to get the TOC contents through READTOC\n");
		  hxfmsg(ps, 0, INFO, msg_str);
		  memset(&atapi_pt, 0, sizeof(struct ide_atapi_passthru));
		  atapi_pt.ide_device 		= device_st_rdev & 0x1;
		  atapi_pt.flags 			= 0x4;	/* IDE_PASSTHRU_READ */
		  atapi_pt.timeout_value	= 10; /* seconds */
		  atapi_pt.buffsize			= toc_length + 2;  /* pass the exact length to read toc */
		  atapi_pt.data_ptr			= rbuf;
		  atapi_pt.atapi_cmd.length = 12;
		  atapi_pt.atapi_cmd.packet.op_code = 0x43;
		  atapi_pt.atapi_cmd.packet.bytes[5] = 0x01;
		  atapi_pt.atapi_cmd.packet.bytes[7] =  toc_length + 2;

		   sprintf( msg_str, "########### atapi_pt.atapi_cmd.packet.bytes[7] = %d bytes\n", atapi_pt.atapi_cmd.packet.bytes[7] );
		   hxfmsg(ps, 0, INFO, msg_str);
	 /* } */

      rc = 0;
      retry_op = 2;
      while ( retry_op > 0 ) {
         retry_op--;
	         rc = ioctl(pr->fildes, IDEPASSTHRU, &atapi_pt);

         if ( rc == 0 )
            retry_op--;
         else if ( rc == -1 ) {

			 sprintf( msg_str, "ioctl fail, errno = %d, retry left = %d \n", errno, retry_op);
			 hxfmsg(ps, 0, INFO, msg_str);

            rc = 0;
            sense_buf[7] = 0;  /* reset sense buffer's additional sense byte */
            xrc = do_req_sense(pr->fildes, sense_buf, sizeof(sense_buf));
            if ( (xrc == 0) && (retry_op == 0) ) {
               strcpy(msg_str, " read table of contents error - ");
                  strcat(msg_str, strerror(errno));
               strcat(msg_str, "\n  Request Sense Data:");
               sens_len = (8 + sense_buf[7]);
               for ( i = 0; i < sens_len; i++ ) {
                  if ( (i & 15) == 0 ) {
                     sprintf(tmp_str, "\n    [%02X] ", i);
                     strcat(msg_str,tmp_str);
                  }
                  sprintf(tmp_str, "%02X ", sense_buf[i]);
                  strcat(msg_str, tmp_str);
               }
               hxfmsg(ps, 0, SOFT, msg_str);
            } else {
               if ( retry_op == 0 ) {
                  strcpy(msg_str, " request sense error after read "
                                  "table of contents retried once - ");
                  hxfmsg(ps, 0, HARD, msg_str);
               }
            }
         }
      }

    } /* else of toc_length = 0 */
  }
}

      if ( device_subclass[0] == 'S' || device_subclass[0] == 'V' )  {  /* scsi */

	rc = 0;
        retry_op = 2;
        while ( retry_op > 0 ) {
          retry_op--;

       if ( device_subclass[0] == 'S' )
	  rc = ioctl(pr->fildes, DKIOCMD, &iocmd_buf); /*- send command -*/
       else if ( device_subclass[0] == 'V' )
	  rc = ioctl(pr->fildes, DKIOLCMD, &viocmd_buf);

	if ( rc == 0 )
            retry_op--;
         else if ( rc == -1 ) {

                         sprintf( msg_str, "ioctl fail, errno = %d, retry left = %d \n", errno, retry_op);
                         hxfmsg(ps, 0, INFO, msg_str);

            rc = 0;
            sense_buf[7] = 0;  /* reset sense buffer's additional sense byte */
            xrc = do_req_sense(pr->fildes, sense_buf, sizeof(sense_buf));
            if ( (xrc == 0) && (retry_op == 0) ) {
               strcpy(msg_str, " read table of contents error - ");
                  strcat(msg_str, strerror(errno));
               strcat(msg_str, "\n  Request Sense Data:");
               sens_len = (8 + sense_buf[7]);
               for ( i = 0; i < sens_len; i++ ) {
                  if ( (i & 15) == 0 ) {
                     sprintf(tmp_str, "\n    [%02X] ", i);
                     strcat(msg_str,tmp_str);
                  }
                  sprintf(tmp_str, "%02X ", sense_buf[i]);
                  strcat(msg_str, tmp_str);
               }
               hxfmsg(ps, 0, SOFT, msg_str);
            } else {
               if ( retry_op == 0 ) {
                  strcpy(msg_str, " request sense error after read "
                                  "table of contents retried once - ");
                  hxfmsg(ps, 0, HARD, msg_str);
               }
            }
         }
      } /* while retry_op */

    } /* If scsi */



      if ( rc != 0 )
         return(-1);
      else {                         /*--- check for a valid part number ---*/

      	 if ( device_subclass[0] == 'S' || device_subclass[0] == 'V' ) {
		  /* Update toc_length for 'S' type device here */
		  /* For 'I' type device, toc_length is already updated above */
		  toc_length = (rbuf[0] * 256) + rbuf[1];
		 }

         x = 0;                                    /* determine part number */
         strcpy(toc_dat, "");
         while ( (x < toc_length + 2) && (x < 200) ) {
            sprintf(ch2, "%02X", rbuf[x++]);
            strcat(toc_dat, ch2);
         }
         strcpy(pr->cds.disc_pn, "Unknown");
         rc = -2;
         if ( (toc_length > strlen(toc512_53F3088)) ||
              (toc_length > strlen(toc512_81F8902)) )
            strcpy(pr->cds.disc_pn, "Unknown");
         else {
            if ( strcmp(toc_dat, toc512_53F3088)==0 ) {
               strcpy(pr->cds.disc_pn, "53F3088");
               rc = 0;
            } else if ( strcmp(toc_dat, toc512_81F8902) == 0 ) {
               strcpy(pr->cds.disc_pn, "81F8902");
               rc = 0;
            } else if ( strcmp(toc_dat, toc2048_81F8902) == 0 ) {
               strcpy(pr->cds.disc_pn, "81F8902");
               rc = 0;
            } else if ( strcmp(toc_dat, toc_dvd_03K998x) == 0 ) {
               strcpy(pr->cds.disc_pn, "03K9982");
               rc = 0;
            } else
               strcpy(pr->cds.disc_pn,"Unknown");
         }
      }
      if ( close_reopen(ps, pr, SC_SINGLE, "get_disc_pn") != 0 )
         prt_msg(ps, pr, 0, &xno, errno, HARD, "get_disc_pn reopen failure - ");
   }
#else /* Linux */
	struct cdrom_tochdr toc_header;
	struct cdrom_tocentry toc_entry;
	int rc = 0;

	/* Force values */
    strcpy(pr->cds.disc_pn, "81F8902");

	/* Read TOC Header to get 1st and last track */
	rc = ioctl(pr->fildes, CDROMREADTOCHDR, &toc_header);
	if(rc != 0) return rc;

	toc_entry.cdte_track = toc_header.cdth_trk1;
	toc_entry.cdte_format = CDROM_LBA;

	/* Get end address of last track */
	rc = ioctl(pr->fildes, CDROMREADTOCENTRY, &toc_entry);
	if(rc != 0) return rc;

	/* Guess values */
	/* CD media will return cd_TOC_ENTRY_LBA while dvd media will return zero! */
	if(toc_entry.cdte_addr.lba == cd_TOC_ENTRY_LBA) {
		/* Must be a cd media */
		strcpy(pr->cds.disc_pn, "81F8902");
	} else {
		/* Must be a DVDROM media */
		strcpy(pr->cds.disc_pn, "03K9982");
	}

   return 0;
#endif
}

/**********************************************************************/
/* get_disc_capacity - get the last lba and blk size using            */
/*                     READ CAPACITY command 25h                      */
/* Returns: lastlba and blk size as returned by READ CAPACITY cmd     */
/*          rc = 0  = command successful                              */
/*          rc = -1 = command failure                                 */
/**********************************************************************/
int get_disc_capacity(struct htx_data *ps, struct ruleinfo *pr,
                      unsigned int * plastlba, unsigned int * pblksize)
{
#ifndef __HTX_LINUX__
    int rc = 0, xno = 0;

    struct ide_atapi_passthru atapi_pt;
    struct sc_iocmd iocmd_buf;
    struct scsi_iocmd viocmd_buf;
    char buf[8];
    char msg[200];

    memset(buf, 0, 8);

    *plastlba = 0;
    *pblksize = 0;

    if ( close_reopen(ps, pr, SC_DIAGNOSTIC, "get_disc_capacity") != 0 )
        prt_msg(ps, pr, 0, &xno, errno, HARD,
                                 "open in get_disc_capacity failure - ");

    if ( device_subclass[0] == 'S' ) { /* scsi */
        init_iocmd(&iocmd_buf);       /* get disc capacity */
        iocmd_buf.buffer = buf;
        iocmd_buf.data_length = 8;
        iocmd_buf.flags = B_READ;
        iocmd_buf.scsi_cdb[0] = 0x25; /* READ CAPACITY */
        iocmd_buf.command_length = 10;

        rc = ioctl(pr->fildes, DKIOCMD, &iocmd_buf); /*- send command -*/

    }
   else if ( device_subclass[0] == 'V' ) { /* vscsi */
        /* printf("vscsi, using DKIOLCMD to get the disc capacity\n"); */
        sprintf(msg,"vscsi, using DKIOLCMD to get the disc capacity\n");
        hxfmsg(ps, 0, INFO, msg);

        init_viocmd(&viocmd_buf);       /* get disc capacity */
        viocmd_buf.buffer = buf;
        viocmd_buf.data_length = 8;
        viocmd_buf.flags = B_READ;
        viocmd_buf.scsi_cdb[0] = 0x25; /* READ CAPACITY */
        viocmd_buf.command_length = 10;

        rc = ioctl(pr->fildes, DKIOLCMD, &viocmd_buf);
    }
    else { /* ide atapi */
        memset(&atapi_pt, 0, sizeof(struct ide_atapi_passthru));
        atapi_pt.ide_device       = device_st_rdev & 0x1;
        atapi_pt.flags            = 0x4;    /* IDE_PASSTHRU_READ */
        atapi_pt.timeout_value    = 10;     /* seconds */
        atapi_pt.buffsize         = 8;
        atapi_pt.data_ptr         = buf;
        atapi_pt.atapi_cmd.length = 12;
        atapi_pt.atapi_cmd.packet.op_code = 0x25; /* READ CAPACITY */

        rc = ioctl(pr->fildes, IDEPASSTHRU, &atapi_pt); /* send command */

      }

    /* Extract the value */
    if(rc == 0) {
        *plastlba  = *((unsigned int *) buf);
        *pblksize = *((unsigned int *) (buf + 4));

        /* Store actual value in msg for hxfupdate */
        sprintf(msg,
        "Warning: READ CAPACITY received LastLba = %d, BlkSize = %d\n",
                        *plastlba, *pblksize);

        /* Check for 512 bytes block size and correct lastlba */
        if( *pblksize == 512 ) {
            *pblksize = 2048;
            *plastlba = *plastlba / 4;
        }
        /* Check for unknown/invalid disc capacity and override it */
        if( *plastlba != cd_LASTLBA && *plastlba != dvd_LASTLBA ) {
            /* Put a warning on htxmsg */
            hxfmsg(ps, 0, INFO, msg);

            /* Force values */
            *plastlba = MAX_BLKNO; /* LastLba of cd */
        }
    }

    if ( close_reopen(ps, pr, SC_SINGLE, "get_disc_capacity") != 0 )
       prt_msg(ps, pr, 0, &xno, errno, HARD,
                              "get_disc_capacity reopen failure - ");
    return rc;
#else /* Linux */
	struct cdrom_tochdr toc_header;
	struct cdrom_tocentry toc_entry;
	int rc = 0;

	/* Force values */
    *plastlba = MAX_BLKNO; /* LastLba of cd */

	/* Read TOC Header to get 1st and last track */
	rc = ioctl(pr->fildes, CDROMREADTOCHDR, &toc_header);
	if(rc != 0) return rc;

	toc_entry.cdte_track = toc_header.cdth_trk1;
	toc_entry.cdte_format = CDROM_LBA;

	/* Get end address of last track */
	rc = ioctl(pr->fildes, CDROMREADTOCENTRY, &toc_entry);
	if(rc != 0) return rc;

	/* Guess values */
	/* CD media will return cd_TOC_ENTRY_LBA while dvd media will return zero! */
	if(toc_entry.cdte_addr.lba == cd_TOC_ENTRY_LBA) {
		/* Must be a cd media */
		*plastlba = MAX_BLKNO; /* LastLba of cd */
	} else {
		/* Must be a DVDROM media */
    	*plastlba = dvd_LASTLBA; /* LastLba of dvd */
	}
	return 0;
#endif
}

/***************************************************************************/
/* HALT audio multimedia operation in progress                             */
/***************************************************************************/
int halt_audio_mm(int fildes)
{
 int    rc;
#ifndef __HTX_LINUX__
 struct cd_audio_cmd audio_ctl;
 memset(&audio_ctl, 0, sizeof(struct cd_audio_cmd));
 audio_ctl.audio_cmds = CD_STOP_AUDIO;
 rc = ioctl(fildes, DKAUDIO, &audio_ctl);
#else
 rc = ioctl(fildes, CDROMSTOP, 0);
#endif
 return (rc);
}

/***************************************************************************/
/* HALT scsi play audio operation in progress                              */
/***************************************************************************/
#ifndef __HTX_LINUX__
int halt_audio_cdrom(int fildes)
{
   int    rc = 0, i;
   struct sc_iocmd iocmd_buf;

   init_iocmd(&iocmd_buf);
   iocmd_buf.command_length = 6;
   iocmd_buf.scsi_cdb[0] = 0x1b;
   rc = ioctl(fildes, CDIOCMD, &iocmd_buf);
   return(rc);
}
#endif

/**************************************************************************/
/* Execute a system command from a pseudo command line                    */
/**************************************************************************/
int do_cmd(struct htx_data *ps, struct ruleinfo *pr)
{
   int    a, b, c, d, rc = 0, filedes;
   char   tmsg[600], cmd_line[300], msg[650];
   char   filenam[30] = "/tmp/errout.";

   close(pr->fildes);
   b = 0;
   for ( a = 0; a <= strlen(pr->cmd_list); a++ ) {
       cmd_line[b] = pr->cmd_list[a];
       if ( cmd_line[b] == '$' ) {
          a++;
          if ( pr->cmd_list[a] == 'd' || pr->cmd_list[a] == 'D' ||
               pr->cmd_list[a] == 'p' || pr->cmd_list[a] == 'P' ) {
             switch ( pr->cmd_list[a] ) {
                case 'd' : {  for ( c = 6; c < strlen(ps->sdev_id); c++ ) {
                                  cmd_line[b] = ps->sdev_id[c];
                                  b++;
                              }
                              break;
                }
                case 'D' : {  for ( c = 5; c < strlen(ps->sdev_id); c++ ) {
                                  cmd_line[b] = ps->sdev_id[c];
                                  b++;
                              }
                              break;
                }
                case 'p' : {  for ( c = 0; c < strlen(ps->sdev_id); c++ ) {
                                  cmd_line[b] = ps->sdev_id[c];
                                  if ( cmd_line[b] == 'r' &&
                                       cmd_line[b-1] == '/' ) ;
                                  else
                                     b++;
                              }
                              break;
                }
                case 'P' : {  for ( c = 0; c < strlen(ps->sdev_id); c++ ) {
                                  cmd_line[b] = ps->sdev_id[c];
                                  b++;
                              }
                }
                default :  for ( c = 0; c < 12; c++ ) {
                               cmd_line[b] = filenam[c];
                               b++;
                           }
                           d = 12;
                           for ( c = 6; c < strlen(ps->sdev_id); c++ ) {
                               cmd_line[b] = ps->sdev_id[c];
                               filenam[d] = ps->sdev_id[c];
                               b++;
                               d++;
                           }
             }
          } else {
             b++;
             cmd_line[b] = pr->cmd_list[a];
             b++;
          }
       } else
          b++;
   }
   if ( ps->run_type[0] == 'O' ) {
      sprintf(msg, "Command to be Executed > \n %s\n", cmd_line);
      hxfmsg(ps, 0, INFO, msg);
   }
   if ((rc = system(cmd_line)) != 0 ) {
      if ( (filedes = open(filenam, O_RDONLY)) == -1 ) {
         sprintf(msg, "Command FAILED rc = %d > \n No Error Information "
                      "returned from command:\n %s\n",
                 rc, cmd_line);
         hxfmsg(ps, -1, HARD, msg);
      } else {
         sprintf(msg, "COMMAND: %s FAILED\n with the Following Error "
                      "Information:\n", cmd_line);
         memset(tmsg, '\0', 600);
         read(filedes, tmsg, 600);
         strcat(msg, tmsg);
         close(pr->fildes);
         sprintf(tmsg, "rm %s", filenam);
         system(tmsg);
         hxfmsg(ps, -1, HARD, msg);
      }
   }
   #ifndef __GR2600__
   		pr->fildes = openx(ps->sdev_id, O_RDONLY, 0, SC_SINGLE);
   #else
   		pr->fildes = openx(ps->sdev_id, O_RDONLY|O_DIRECT, 0, SC_SINGLE);
   #endif

   if ( pr->fildes == -1 ) {
      rc = -2;
      strcat(msg, "do_cmd(): open in single mode failed.\n");
      hxfmsg(ps, 0, HARD, msg);
   }
   return(rc);
}

#ifndef __HTX_LINUX__ /* AIX */
int play_audio_msf(struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno, struct cd_audio_cmd *audio_ioctl)
{
	struct sc_iocmd paudiomsf_ctl;
	struct scsi_iocmd vio_paudiomsf_ctl;
	int audio_status = 0;
	time_t t1;
	int rc;

      if ( device_subclass[0] == 'S' ) { /* scsi */
	memset(&paudiomsf_ctl,0,sizeof(struct sc_iocmd ));

	paudiomsf_ctl.command_length = 10;
	paudiomsf_ctl.scsi_cdb[0] = 0x47;
	paudiomsf_ctl.scsi_cdb[1] = 0x00;
	paudiomsf_ctl.scsi_cdb[2] = 0x00;
	paudiomsf_ctl.scsi_cdb[3] = audio_ioctl->indexing.msf.first_mins;
	paudiomsf_ctl.scsi_cdb[4] = audio_ioctl->indexing.msf.first_secs;
	paudiomsf_ctl.scsi_cdb[5] = audio_ioctl->indexing.msf.first_frames;
	paudiomsf_ctl.scsi_cdb[6] = audio_ioctl->indexing.msf.last_mins;
	paudiomsf_ctl.scsi_cdb[7] = audio_ioctl->indexing.msf.last_secs;
	paudiomsf_ctl.scsi_cdb[8] = audio_ioctl->indexing.msf.last_frames;
	paudiomsf_ctl.scsi_cdb[9] = 0x00;

	paudiomsf_ctl.timeout_value = 30;
	paudiomsf_ctl.flags |= B_READ;

	rc = ioctl(pr->fildes, DKIOCMD, &paudiomsf_ctl);

       }
      else if ( device_subclass[0] == 'V' ) { /* vscsi */
        init_viocmd(&vio_paudiomsf_ctl);
	vio_paudiomsf_ctl.command_length = 10;
        vio_paudiomsf_ctl.scsi_cdb[0] = 0x47;
        vio_paudiomsf_ctl.scsi_cdb[1] = 0x00;
        vio_paudiomsf_ctl.scsi_cdb[2] = 0x00;
        vio_paudiomsf_ctl.scsi_cdb[3] = audio_ioctl->indexing.msf.first_mins;
        vio_paudiomsf_ctl.scsi_cdb[4] = audio_ioctl->indexing.msf.first_secs;
        vio_paudiomsf_ctl.scsi_cdb[5] = audio_ioctl->indexing.msf.first_frames;
        vio_paudiomsf_ctl.scsi_cdb[6] = audio_ioctl->indexing.msf.last_mins;
        vio_paudiomsf_ctl.scsi_cdb[7] = audio_ioctl->indexing.msf.last_secs;
        vio_paudiomsf_ctl.scsi_cdb[8] = audio_ioctl->indexing.msf.last_frames;
        vio_paudiomsf_ctl.scsi_cdb[9] = 0x00;

        vio_paudiomsf_ctl.timeout_value = 30;
        vio_paudiomsf_ctl.flags |= B_READ;

        rc = ioctl(pr->fildes, DKIOLCMD, &vio_paudiomsf_ctl);
       }

	if( rc == -1 ) return rc;

    pr->op_in_progress = 3;  /* set device driver CD_PLAY_AUDIO op in progress */
    rc = 0;

    /* wait for play to be finished */
    while( ( audio_status = read_subchannel( ps, pr, loop, blkno ) ) != -1 ) {

		hxfupdate(UPDATE,ps);

		if( audio_status == 0x13 ) { 		/* play audio completed successfully */
			break;              			/* return successfully */
		}
		else if( audio_status == 0x14 ) {	/* play audio stopped due to an error */
			break;							/* break and return audio error */
		}
		else {								/* wait for 5 secs and continue to loop */
            time(&t1);
            while ( time(NULL) - t1 < 5 ) ;
			continue;
		}

	} /* while */

	if( audio_status == -1 ) return -2; /* in case read_subchannel fails, return -2 */

	return audio_status; /* else return audio status */

}

int read_subchannel(struct htx_data *ps, struct ruleinfo *pr, int loop, int *blkno)
{
	struct sc_iocmd readsc_ctl;
	struct scsi_iocmd vio_readsc_ctl;
	char buffer[100];
	int rc;

     if ( device_subclass[0] == 'S' ) { /* scsi */

	memset(&readsc_ctl,0,sizeof(struct sc_iocmd ));

	readsc_ctl.command_length = 10;
	readsc_ctl.scsi_cdb[0] = 0x42;
	readsc_ctl.scsi_cdb[1] = 0x02;
	readsc_ctl.scsi_cdb[2] = 0x40;
	readsc_ctl.scsi_cdb[3] = 0x01;
	readsc_ctl.scsi_cdb[4] = 0x00;
	readsc_ctl.scsi_cdb[5] = 0x00;
	readsc_ctl.scsi_cdb[6] = 0x00;
	readsc_ctl.scsi_cdb[7] = 0x00;
	readsc_ctl.scsi_cdb[8] = 0x30;
	readsc_ctl.scsi_cdb[9] = 0x00;

	readsc_ctl.data_length = 0x30;
	readsc_ctl.timeout_value = 30;
	readsc_ctl.buffer = buffer;
	readsc_ctl.flags |= B_READ;

	rc = ioctl(pr->fildes, DKIOCMD, &readsc_ctl);
      }
     else if ( device_subclass[0] == 'V' ) { /* vscsi */
	/* printf("\n read_subchannel, VIOS setup, using DKIOLCMD to read subchannel\n"); */
	init_viocmd(&vio_readsc_ctl);
	vio_readsc_ctl.command_length = 10;
        vio_readsc_ctl.scsi_cdb[0] = 0x42;
        vio_readsc_ctl.scsi_cdb[1] = 0x02;
        vio_readsc_ctl.scsi_cdb[2] = 0x40;
        vio_readsc_ctl.scsi_cdb[3] = 0x01;
        vio_readsc_ctl.scsi_cdb[4] = 0x00;
        vio_readsc_ctl.scsi_cdb[5] = 0x00;
        vio_readsc_ctl.scsi_cdb[6] = 0x00;
        vio_readsc_ctl.scsi_cdb[7] = 0x00;
        vio_readsc_ctl.scsi_cdb[8] = 0x30;
        vio_readsc_ctl.scsi_cdb[9] = 0x00;

        vio_readsc_ctl.data_length = 0x30;
        vio_readsc_ctl.timeout_value = 30;
        vio_readsc_ctl.buffer = buffer;
        vio_readsc_ctl.flags |= B_READ;

        rc = ioctl(pr->fildes, DKIOLCMD, &vio_readsc_ctl);
      }

	if( rc == -1 ) return rc;

    return buffer[1]; /* audio status byte */

}

#endif
