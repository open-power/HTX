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
 *   COMPONENT_NAME: EXER_CD
 *
 *   MODULE NAME: hxecd.c
 *
 *   DESCRIPTION: HTX CDROM Exerciser Main Program.
 *
 *   FUNCTIONS : main() - main function of cdrom exerciser program.
 *          start_msg() - puts out a rule started message to htx.
 *        SIGTERM_hdl   - used to shutdown exerciser.
 *
 ******************************************************************************/

#include "hxecd.h"

dev_t device_st_rdev;                      /* device id (see stat.h) */

int  crash_on_mis = 0;         /* 0 = don't crash system on a miscompare */
                               /* 1 = crash system - see buf_oper.c      */
int  cnt, rule_stanza[99];
FILE *fptr;
char device_subclass[32];  /* subclass of device */
char rules_file_name[100];                  /* argv[3] - Rules File name */
char pipe_name[15], signal_flag = 'N';
void sig_function(int);

void sig_function(int sig)
{
  signal_flag = 'Y';
}

static int get_device_subclass(char *, char *);

struct sigaction sigdata;                   /* set the signal structure  */
static sigset_t mysigmask;                  /* Holds the process sig mask*/
struct sigaction sigvector;
void SIGTERM_hdl(int, int, struct sigcontext *);
          /*- following pointers used during sigterm shutdown only ------*/
struct ruleinfo *st_prule_info;    /* pointer to active rule_info struct */
struct htx_data *st_phtx_info;     /* pointer to active htx_info struct  */

/* Env variable affecting the working of hxecd */
const char* MEDIAOVR = "HXECD_MEDIA_OVERRIDE"; /* whether to continue on finding invalid media / halt with error */

int main (int argc, char **argv)                    /* Begin MAIN line code   */
{
  int    i, done, rc, disc_code;
  unsigned int last_lba = 0, blk_size = 0;
  char   msg[220], rules_file_name[100];
  char   *wptr, *rptr;  /* read / write / buffer pointers (aligned) */
  char   *wptr_malloc, *rptr_malloc;  /* read / write / malloc buffer pointers  */
/*struct devinfo b; Unused */
  struct stat *p_dev_stat;
  struct ruleinfo r;
  struct htx_data s;
  char   *strhtxkdblevel = NULL;
  int    htxkdblevel = 0;

  /* set up for use in SIGTERM_handler.                        */
  st_prule_info = &r;    /* pointer to active rule_info struct */
  st_phtx_info = &s;     /* pointer to active htx_info struct  */

  sigprocmask(0, NULL, &mysigmask);          /* get current signal mask   */
  sigdata.sa_flags = SA_RESTART;             /* restart on signal call    */
  sigdata.sa_mask = mysigmask;
  sigdata.sa_handler = sig_function;
  sigaction(SIGUSR1, &sigdata, NULL);        /* do on receipt of Sig. 30  */

  wptr_malloc = malloc(BUF_SIZE+PAGE_SIZE);
  rptr_malloc = malloc(BUF_SIZE+PAGE_SIZE);

#ifndef __HTX_LINUX__     /* AIX */
  wptr = wptr_malloc;
  rptr = rptr_malloc;
#else                     /* Linux */
  /* Page align the buffers for linux raw IO */
  wptr = (char *)HTX_PAGE_ALIGN(wptr_malloc);
  rptr = (char *)HTX_PAGE_ALIGN(rptr_malloc);
#endif

  memset(&r, 0, sizeof(struct ruleinfo));
  strcpy(s.HE_name, argv[0]);
  strcpy(s.sdev_id, argv[1]);
  strcpy(s.run_type, argv[2]);
  strcpy(rules_file_name, argv[3]);
  hxfupdate(START, &s);

  /* Check for memory allocation */
  if(wptr == NULL || rptr == NULL) {
    sprintf(msg, "Unable to allocate memory of %d bytes\n", BUF_SIZE+PAGE_SIZE);
	hxfmsg(&s, 0, SYSERR, msg);
  }
  /****************************************/
  /*- SIGTERM handler initializations    -*/
  /****************************************/
  sigemptyset(&(sigvector.sa_mask));
  sigvector.sa_flags = 0 ;
  sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
  (void) sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

  sprintf(msg, "%s %s %s %s \n", s.HE_name, s.sdev_id, s.run_type,
          rules_file_name);
  hxfmsg(&s, 0, INFO, msg);

              /****************************************************
               * Validity check device id                         *
               ****************************************************/
  if ( strncmp(s.sdev_id, "/dev/rcd", 8) != 0 &&
              strncmp(s.sdev_id, "/dev/cdrom", 10) != 0 &&
              strncmp(s.sdev_id, "/dev/sr", 7) != 0 &&
              strncmp(s.sdev_id, "/dev/rsr", 8) != 0 &&
              strncmp(s.sdev_id, "/dev/hd", 7) != 0 &&
              strncmp(s.sdev_id, "/dev/rhd", 8) != 0 &&
              strncmp(s.sdev_id, "/dev/scd", 8) != 0 &&
              strncmp(s.sdev_id, "/dev/raw", 8) != 0 ) {
     sprintf(msg, "Invalid device name %s not allowed \n", s.sdev_id);
     hxfmsg(&s, 0, SYSERR, msg);
     exit(1);
  }
  i = strlen(s.sdev_id);
  done = 0;
  for ( rc = 5; rc < i; rc++ ) {
     pipe_name[done] = s.sdev_id[rc];
     done++;
  }
  strcat(pipe_name, ".pipe");
  if ( (fptr = fopen(rules_file_name, "r")) == NULL ) {
     strcpy(msg, "error opening rules file - ");
     strcat(msg, strerror(errno));
     strcat(msg, "\n");
     hxfmsg(&s, errno, SYSERR, msg);
     exit(1);
  }

        /***************************************************
         * Get the subclass of the drive (scsi or ide)     *
         * for use in decisions about how to handle        *
         * passthru commands.                              *
         **************************************************/

  rc = get_device_subclass(&s.sdev_id[6], device_subclass);
  if ( rc != 0 ) {
     strcpy(msg, "Can't get subclass of device.\n");
     hxfmsg(&s, rc, SYSERR, msg);
     exit(1);
  } else { /* assume that its either scsi or ide, but verify it. */
      if ( 0 == strncmp(device_subclass, "ide", 16)) device_subclass[0] = 'I';
      if ( 0 == strncmp(device_subclass, "scsi", 16)) device_subclass[0] = 'S';
      if ( 0 == strncmp(device_subclass, "vscsi", 16)) device_subclass[0] = 'V';
          /* and uncapitalized subclass indicates an invalid subclass */
  }

          /*************************************************
           * Open CD-ROM in SC_SINGLE mode initially.      *
           *************************************************/
  #ifndef __GR2600__
  	r.fildes = openx(s.sdev_id, O_RDONLY, 0, SC_SINGLE);
  #else
  	r.fildes = openx(s.sdev_id, O_RDONLY|O_DIRECT, 0, SC_SINGLE);
  #endif

  if ( r.fildes == -1 ) {
     strcpy(msg, "Initial open error - ");
     strcat(msg, strerror(errno));
     strcat(msg, "\n");
     hxfmsg(&s, errno, SYSERR, msg);
     exit(1);
  }
  strcpy(r.mode, "M1");             /* Initialize mode to CD-ROM Mode 1 */

          /*************************************************
           * Get the st_rdev from fstat().  This is used in *
           * IDE ATAPI passthru commands.
           *************************************************/
  p_dev_stat = (struct stat *) malloc(sizeof (struct stat));
  if ( 0 != (rc = fstat(r.fildes, p_dev_stat))) {
     strcpy(msg, "fstat() error - ");
     strcat(msg, strerror(errno));
     strcat(msg, "\n");
     hxfmsg(&s, errno, SYSERR, msg);
     exit(1);
  } else {
    device_st_rdev = p_dev_stat->st_rdev;
  }
  free(p_dev_stat);

          /*************************************************
           * Determine DISC Capacity -- get_disc_capacity() *
           * SCSI/IDE passthru command -- 0x25
           *************************************************/

  if( get_disc_capacity(&s, &r, &last_lba, &blk_size) != 0 ) {
     strcpy(msg, "Warning: get_disc_capacity failed \n");
     hxfmsg(&s, errno, SOFT, msg);
  }else {
  printf("Last lba = %d, Blk size = %d\n", last_lba, blk_size);

  }
  r.bytpsec  = blk_size;
  r.tot_blks = last_lba;

           /*************************************************
            * determine type of play audio scsi commands    *
            * required by the device                        *
            *  0 = standard play audio scsi commands        *
            *  1 = vendor unique play audio scsi commands   *
            *  2 = vendor unique play audio scsi commands   *
            *************************************************/
#ifndef __HTX_LINUX__
   r.master_audio_type = get_audio_type(&r, &s);
#else
   r.master_audio_type = 0;
#endif
   if ( r.master_audio_type == -1 ) {
      strcpy(msg, "Get Audio Type error - ");
      strcat(msg, strerror(errno));
      strcat(msg,"\n");
      hxfmsg(&s,errno,SYSERR,msg);
      exit(1);
   }

                    /***************************************************
                     * perform a default mode select of CDROM Mode 1   *
                     * and acquire capacity information                *
                     ***************************************************/
  set_defaults(&r);
  strcpy(r.rule_id, "INIT_");
  strncat(r.rule_id, device_subclass,1);
  ms_get(&r, &s, &last_lba, &blk_size);
       /**********************************************************************
        * get the current cdrom disc's part number                           *
        * NOTE: TOSHIBA Model 3101 does not support scsi read table of       *
        *       contents command, so a NULL part number is set.              *
        **********************************************************************/

        /*
        CDROM and DVDROM part nos. have changed in recent times, altho' in the code we still use
        the same old part nos, still while we error out we need to print the correct part nos, so
        that the tester can get the part without problems. Altho' we use different part nos in the
        code it doesn't matter, since technically the data ( content ) in the disk hasn't changed, so
        we can still continue to use the same old part nos. in the code, especially in the get_disc_pn
        code, but while printing the P/N we need to print the new P/Ns also.

        To clarify ....
        81F8902 stand for Multimedia CDROM test disk.
        03K9982 and 53P2519 stand for DVDROM test disk.
        53F3088 stands for old Standard CDROM test disk, which is no longer supported.
        */

  strcpy(r.cds.rule_disc_pn, "");          /* set pn null, initial pass only */
  strcpy(r.cds.disc_pn, "");
  if ( r.master_audio_type != 1 ) {
     rc  = get_disc_pn(&s,&r);
     if (rc == -2) {
        strcpy(msg,"Warning, encountered unsupported cdrom/dvdrom test disc!\n");
        strcat(msg," supported P/Ns are 03K9982/53P2519 for dvdrom for and 81F8902 for cdrom.\n");

         if( ( NULL != getenv(MEDIAOVR) ) &&
             ( 0 == strcmp( getenv(MEDIAOVR), "1" ))
           )
                    hxfmsg(&s, errno, SOFT, msg);
         else
         {
             strcat(msg," Pls set the env var 'HXECD_MEDIA_OVERRIDE' to 1, to continue on this error \n");
             hxfmsg(&s, errno, SYSERR, msg);
             exit(1);
         }

     } else if ( rc == -1 ) {
        strcpy(msg,"read table of contents error - ");

         if( ( NULL != getenv(MEDIAOVR) ) &&
             ( 0 == strcmp( getenv(MEDIAOVR), "1" ))
           )
                    hxfmsg(&s, errno, SOFT, msg);
         else
         {
             strcat(msg," Pls set the env var 'HXECD_MEDIA_OVERRIDE' to 1, to continue on this error \n");
             hxfmsg(&s, errno, SYSERR, msg);
             exit(1);
         }

        strcat(msg, strerror(errno));
        strcat(msg,"\n");
        hxfmsg(&s, errno, HARD, msg);
     } else {
        strcpy(msg, "CD-ROM Disc Part Number = ");
        strcat(msg, r.cds.disc_pn);
        strcat(msg, "\n");
        hxfmsg(&s, errno, 6, msg);
     }
  }

            /************************************************************
             *  validity check all rule stanzas                         *
             ************************************************************/
  cnt = 0;
  while ( (rc = get_rule(&s, &r)) != EOF ) {
    cnt++;
    if ( rc == 1 ) {
       exit(1);
    }
  }

            /************************************************************
             *  Look for HTXKDBLEVEL environment variable               *
             ************************************************************/

  htxkdblevel = 0;
  strhtxkdblevel = getenv("HTXKDBLEVEL");
  if(strhtxkdblevel != NULL) {
    htxkdblevel = atoi(strhtxkdblevel);
  }

                 /***************************************************
                  * verify cdrom disc P/N compatible with rules     *
                  * Allowed combinations are:                       *
                  *                                                 *
                  * disc_pn == rules_pn = compatible                *
                  * disc_pn = 5xxxxx AND rules_pn = NULL = OK       *
                  * disc_pn = 8xxxxx AND rules_pn = NULL = OK       *
                  * disc_pn = Unsupported = Allowed                 *
                  * else flag error as incomaptible cd/rules.       *
                  *                                                 *
                  ***************************************************/
  disc_code = 0;                                      /* SUPPORTED */
  if ( strcmp(r.cds.disc_pn, r.cds.rule_disc_pn) != 0 ) {
     if ( strcmp(r.cds.disc_pn, "Unknown") == 0 )
        disc_code = 1;                              /* UNSUPPORTED */
     else if ( (/*(strcmp(r.cds.disc_pn, "53F3088") == 0) ||*/
                (strcmp(r.cds.disc_pn, "81F8902") == 0) ||
                (strcmp(r.cds.disc_pn, "03K9982") == 0)) &&
                (strlen(r.cds.rule_disc_pn) == 0) )
       disc_code = 0;
     else
       disc_code = 2;                  /* all others NOT COMPATIBLE */
     switch (disc_code) {
        case 0 : break;
        case 1 : {
           sprintf(msg, "WARNING: Unsupported CD-ROM/DVDROM test Disc!\n");
           strcat(msg, "Supported PN's are 81F8902 for CD-ROM test disc");
           strcat(msg, "and\n");
           /*strcat(msg, "media CD-ROM devices.\n");*/
           strcat(msg, "                   03K9982/53P2519 for DVDROM test disc\n");

			 if( ( NULL != getenv(MEDIAOVR) ) &&
				 ( 0 == strcmp( getenv(MEDIAOVR), "1" ))
			   )
						hxfmsg(&s, errno, INFO, msg);
			 else
			 {
				 strcat(msg,"\n Pls set the env var 'HXECD_MEDIA_OVERRIDE' to 1,\n\
				 to continue on this error. \n");

				 hxfmsg(&s, errno, SYSERR, msg);
				 exit(1);
			 }

          break;
        }
        default:
           sprintf(msg, "CD-ROM Disc Part Number %s NOT compatible with "
                        "rules file,\n", r.cds.disc_pn);
           strcat(msg, " Supported PN is 81F8902 for CD-ROM test disc");
           strcat(msg, "and\n");
           /*strcat(msg, "ROM rules.\n");*/
           strcat(msg, "                  03K9982/53P2519 for DVDROM test disc\n");
           hxfmsg(&s, errno, HARD, msg);
     }
  }
                         /*******************************************
                          * Process each rule stanza                *
                          *******************************************/

  /* Reset crash_on_mis, which could be set, because of a false run through the rule file, above */
  crash_on_mis = 0;

  do {
     rewind(fptr);
     for ( rc = 0; rc < cnt; rc++ )
         rule_stanza[rc] = 0;
     cnt = 0;
     sprintf(msg, "using rules file %s \n", rules_file_name);
     hxfmsg(&s, 0, INFO, msg);
     while ( ((rc = get_rule(&s, &r)) != EOF) && (rc != -1) ) {
        s.test_id = rule_stanza[cnt];
        hxfupdate(UPDATE, &s);

        /* Set crash_on_mis if htxkdblevel is 1 or 2 */
        /* If HTXKDBLEVEL is not present or if HTXKDBLEVEL=0, then let the rule file value dominate, makes sense when exer run from cmd line  or when HTX is run using -k option */

        /* If HTXKDBLEVEL is set, then irrespective of rule file crash_on_mis keyword, we need to set crash_on_mis */

        /* When invoked only as 'htx', HTXKDBLEVEL=1*/
        /* When invoked as   'htx -k', HTXKDBLEVEL=0*/
        if( strhtxkdblevel != NULL ) {
			if( (htxkdblevel > 0) || ( crash_on_mis == 1 ) ) crash_on_mis = 1;
			else crash_on_mis = 0;
		}

		sprintf(msg, "%s: crash_on_mis = %d\n", r.rule_id, crash_on_mis);
		hxfmsg(&s, 0, INFO, msg);

        cnt++;
        if ( s.run_type[0] == 'O')
           start_msg(&s, &r, msg);
        rc = proc_rule(&s, &r, wptr, rptr, &last_lba, &blk_size);
        if ( rc != 0 )
           return(1);
        if ( signal_flag == 'Y' )
           rc = -1;
     }
     if ( signal_flag == 'Y' || signal_flag == 'R' ) {
        if ( signal_flag == 'Y' ) {
           sprintf(msg, "mkfifo -m 600 %s", pipe_name);
           system(msg);
        }
        fclose(fptr);
        if ( (fptr = fopen(pipe_name, "r")) == NULL ) {
           strcpy(msg, "error opening pipe file - ");
           strcat(msg, strerror(errno));
           strcat(msg, "\n");
           hxfmsg(&s, errno, SYSERR, msg);
           return(1);
        } else
           signal_flag = 'X';
     } else if ( signal_flag == 'X' ) {
        fclose(fptr);
        sprintf(msg, "rm %s", pipe_name);
        system(msg);
        signal_flag = 'N';
        if ( (fptr = fopen(rules_file_name, "r")) == NULL ) {
           strcpy(msg, "error opening rules file - ");
           strcat(msg, strerror(errno));
           strcat(msg, "\n");
           hxfmsg(&s, errno, SYSERR, msg);
           return(1);
        }
     } else
        hxfupdate(FINISH,&s);

  } while ( strcmp(s.run_type, "REG") == 0 ||
            strcmp(s.run_type, "EMC") == 0 ||
            strcmp(s.run_type, "LOP") == 0 );
                         /*************************************
                          * close device, terminate exerciser *
                          *************************************/
  close(r.fildes);
  exit(0);
}

/**************************************************************************/
/* put out rule started message                                           */
/**************************************************************************/
void start_msg(struct htx_data *ps, struct ruleinfo *pr, char *msg_text)
{
  sprintf(msg_text, "HXECD: %s - %s  Num_Ops: %d  Blocks: %d  Pattern: "
                    "%s  Bytes: %d\n",
          pr->oper, pr->rule_id, pr->num_oper, pr->num_blks,
          pr->pattern_id, (pr->num_blks * pr->bytpsec));
  hxfmsg(ps, 0, INFO, msg_text);
 }

/**************************************************************************/
/* SIGTERM Handler Routine                                                */
/**************************************************************************/
void SIGTERM_hdl (int sig, int code, struct sigcontext *scp)
{
                    /*-- check for an audio play operation in progress --*/
                    /*-- do a shutdown of operation if one is active   --*/
  switch ( st_prule_info->op_in_progress ) {
     case 1 :  {             /*-Toshiba vendor unique play audio active -*/
          #ifndef __HTX_LINUX__
          halt_audio_cdrom(st_prule_info->fildes);
          #endif
          break;
     }
     case 2 : {                         /*- scsi play play audio active -*/
          #ifndef __HTX_LINUX__
          halt_audio_cdrom(st_prule_info->fildes);
          #endif
          break;
     }
     case 3 : {                  /*- device driver CD_PLAY_AUDIO active -*/
          halt_audio_mm(st_prule_info->fildes);
          break;
     }
     default : ;
  }
  hxfmsg(st_phtx_info, 0, INFO, "HXECD Program terminated by SIGTERM signal.");
  close(st_prule_info->fildes);
  exit(0);
}

/***********************************************************************
 *
 *  get_device_subclass
 *
 *  Looks in the ODM for the subclass of the drive under test
 *  and returns its subclass.
 *
 *  INPUT:  devname - logical name of device
 *  OUTPUT: device_subclass - pointer to ODM subclass.
 *  RETURNS: 0 - OK
 *          <0 - serious failure - programming problems
 *          >0 - odmerrno - in this case device_subclass points to the
 *               search criteria which failed.
 *
 ***********************************************************************/
int get_device_subclass(char *devname, char *device_subclass)
{

#ifndef __HTX_LINUX__  /* AIX */

extern int odmerrno;

int ret_code  = 0;
struct CuDv CuDv_obj, *CuDv_obj_ptr;
struct PdDv PdDv_obj, *PdDv_obj_ptr;
char    criteria[60];

    /* perform and verify correct ODM initialization */
    ret_code = (int)odm_initialize();
    if (ret_code) {
        /************ handle error here *********/
        return(odmerrno);
    } /* endif */

    /* Find the CuDv object corresponding to logical dev_name */
    sprintf(criteria, "name = '%s'",devname);
    CuDv_obj_ptr = odm_get_first(CuDv_CLASS,criteria,&CuDv_obj);
    ret_code = (int)CuDv_obj_ptr;
    if (!ret_code) {
        /************ handle error here and return *********/
        device_subclass = strcpy(device_subclass, criteria);
        (void) odm_terminate();
        return(odmerrno);
    } /* endif */

    /* get the PdDv structure for the subclass using the link */
    sprintf(criteria, "uniquetype = '%s'",CuDv_obj.PdDvLn_Lvalue);
    PdDv_obj_ptr = odm_get_first(PdDv_CLASS,criteria,&PdDv_obj);
    ret_code = (int)PdDv_obj_ptr;
    if (!ret_code) {
        /************ handle error here and return *********/
        device_subclass = strcpy(device_subclass, criteria);
        (void) odm_terminate();
        return(odmerrno);
        } /* endif */

    /* return the adapter name and subclass */
    strncpy( device_subclass, PdDv_obj.subclass, 16);
    ret_code = 0;

    (void) odm_terminate();

    return(ret_code);

#else /* Linux */

    /* Force SCSI CDROM for Linux HTX */
    strcpy( device_subclass, "scsi" );
    return 0;
#endif

}

