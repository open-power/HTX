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

/*****************************************************************************
 *   COMPONENT_NAME: exer_tape
 *
 *   MODULE NAME: hxetape.c
 *
 *   DESCRIPTION: HTX Tape Exerciser Main Program
 *
 *   FUNCTIONS: main() - main function of tape exerciser program.
 *              start_msg() - puts out a rule stanza started message to htx.
 *              finish_msg() - puts out a rule stanza finished message to htx.
 *              finish_stanza() - puts out a count of seconds and bytes to htx.
 *              e_notation() - converts double form into engineering notation.
 *              SIGTERM_hdl() - shuts down exerciser upon receipt of SIGTERM
 *                              signal.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <malloc.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#ifndef __HTX_LINUX__
#include <odmi.h>
#include <sys/devinfo.h>
#include <sys/cfgodm.h>
#include <sys/scsi.h>
#include <sys/tape.h>
#include <sys/watchdog.h>
#include <sys/tapedd.h>

#else
#include <sys/mtio.h>
#include <scsi/sg.h>
#include <sys/stat.h>

#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#ifdef __HTX_LINUX__
#include <hxetape.h>
#else
#include "hxetape.h"
#endif

                            /********** Global Variable Definitions **********/
FILE *fptr;
int          Open_Mode;                 /* indicates open mode  according to */
                                        /* found state of media. If media is */
                                        /* write protected then              */
                                        /* Open_Mode = O_RDONLY else         */
                                        /*  Open_Mode = O_RDWR (default).    */
int          crash_on_mis = 0;       /* 0 = don't crash system on miscompare */
                                     /* 1 = crash system - see buf_oper.c    */
int          tape_error_code;
int          cnt, rule_stanza[250];
int			 found_fscsi = 0;
unsigned int BLK_SIZE;               /* BLK_SIZE is used by hxetape for the  */
                                     /* block size value in all opers where  */
                                     /* needed. BLK_SIZE is set from the de- */
                                     /* info blksize value unless the blksize*/
                                     /* is 0 in which case the BLK_SIZE is   */
                                     /* set to the default value of BLKSIZ.  */
unsigned long long tape_capacity;	 /* tape_capacity is used by hxetape for */
									 /* variable fixed size block testing	 */

char         pipe_name[15], rules_file_name[100], signal_flag = 'N';
struct timeval	timer1, timer2;
#ifdef __HTX_LINUX__
struct scsi_mapping	*sg_st_map;			/*generic scsi to st map*/
#endif

unsigned long long ASSUMED_TAPE_MIN_CAPACITY = 21474836480ULL;   /* The Maximum capacity of tape device is assumed to be 20GB */

double       total_bytes;
struct       blk_num_typ blk_num;       /* used to track current/relative    */
                                        /* block number locations            */
struct       sigaction sigdata;             /* set the signal structure      */
static       sigset_t mysigmask;            /* Holds the process signal mask */
struct       sigaction sigvector;
struct       ruleinfo *st_prule_info;  /* used for shutdown of system only   */
struct       htx_data *st_phtx_info;   /* used for shutdown of system only   */

void SIGTERM_hdl(int, int, struct sigcontext *);
void sig_function(int, int, struct sigcontext *);
void sig_function(int sig, int code, struct sigcontext *scp)
{
#ifdef _DEBUG_LINUX_
	printf("sig_fuction called\n");
#endif
  signal_flag = 'Y';
}
#ifndef __HTX_LINUX__
static int odm_fscsi_capacity(struct htx_data*);
static int randomize_fixedblk(struct htx_data *pHTX, struct ruleinfo *pRule);
#else
static void	restore_tape_config(int, void*);
static void create_scsi_map(struct htx_data *);
#endif

int random_blksize = 0;

int get_bus_width( char* pattern_id ); /* for buster pattern support */

#ifdef __HTX_LINUX__

unsigned int l_blk_limit = 1; /* blk size 0 is VBS mode */
unsigned int m_blk_limit = 0xFFFFFFFF; /* need to check MTSETBLK implm to find max value */
unsigned int gran; /* granularity as returned by RBL o/p */

void set_tape_config( int fildes, struct htx_data *pHTX );
void get_tape_config( int fildes, struct htx_data *pHTX );
static int randomize_fixedblk(struct htx_data *pHTX, struct ruleinfo *pRule);
#endif

int getline_new(char s[], int lim, FILE* fp);

int
main (int argc, char *argv[])
{
   int    done, i, rc, new_num_oper;
   char   dev_type[40], msg[220], tmp_msg[80], kdblevel[4];
   char   *rptr, *wptr;             /* read / write buffer pointers   */
   struct ruleinfo rule_info;
   struct htx_data htx_info;
#ifdef __HTX_LINUX__
	struct on_exit_st *exit_st;
#endif

	/* for buster pattern support */
	int bus_width = 0, bufrem = 0;

   sigemptyset(&(sigvector.sa_mask));    /* SIGTERM handler initializations */
   sigvector.sa_flags = 0 ;
   sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
   sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);
#ifdef __HTX_LINUX__
   sigaction(SIGINT, &sigvector, (struct sigaction*) NULL);
#endif

   sigprocmask(0, NULL, &mysigmask);          /* get current signal mask    */
   sigdata.sa_flags = SA_RESTART;             /* restart on signal calls    */
   sigdata.sa_mask = mysigmask;
   sigdata.sa_handler = (void(*)(int))sig_function;
   sigaction(SIGUSR1, &sigdata, NULL);      /* call on receipt of signal 30 */
   st_phtx_info = &htx_info;          /* pointer to active htx_info struct  */

        /********************************************************************/
        /* Check to make sure that the proper number of arguments are       */
        /* passed, then copy the passed values to the appropriate data      */
        /* structures.                                                      */
        /********************************************************************/
   if ( argc == 4 ) {                       /* four arguments passed? */
      strcpy(htx_info.HE_name, argv[0]);
      strcpy(htx_info.sdev_id, argv[1]);
      strcpy(htx_info.run_type, argv[2]);
      for ( i = 0; (i < 4) && (htx_info.run_type[i] != '\0'); i++ ) {
          htx_info.run_type[i] = (char) toupper((int) htx_info.run_type[i]);
      }
      strcpy(rules_file_name,argv[3]);
   } else {
      sprintf(msg, "Invalid number of arguments passed to %s.\n", argv[0]);
      hxfmsg(&htx_info, 0, SYSERR, msg);
      exit(1);
   }
   bzero(kdblevel, 4);
   if (getenv("HTXKDBLEVEL") != NULL) {
      htx_strcpy(kdblevel, getenv("HTXKDBLEVEL"));
      crash_on_mis = atoi(kdblevel);
   }

   i = strlen(htx_info.sdev_id);
   done = 0;
   for ( rc = 5; rc < i; rc++ ) {
      pipe_name[done] = htx_info.sdev_id[rc];
      done++;
   }
   strcat(pipe_name, ".pipe");
   hxfupdate(START, &htx_info);


                /************************************************************/
                /* open on rules file                                       */
                /************************************************************/
   if ( (fptr = fopen(rules_file_name, "r")) == NULL ) {
      strcpy(msg, "Error opening rules file - ");
      strcat(msg, rules_file_name);
      strcat(msg, "\n");
      strcat(msg, strerror(errno));
      strcat(msg, "\n");
      hxfmsg(&htx_info, errno, SYSERR, msg);
      exit(3);
   }


                   /*********************************************************/
                   /*  Set BLK_SIZE value from device driver information.   */
                   /*********************************************************/
   strcpy(dev_type, "not found!\0");
   BLK_SIZE = get_dd_blksize(htx_info.sdev_id, dev_type, &htx_info, &rule_info,
                             &blk_num);
#ifdef __HTX_LINUX__
	exit_st = (struct on_exit_st*)malloc(sizeof(struct on_exit_st));
   	if(exit_st == NULL) {
		sprintf(msg, "Error in memory allocation with error:%d = %s\n",errno, strerror(errno));
		hxfmsg(&htx_info, errno, SYSERR, msg);
   	}
   	exit_st->blk_size= BLK_SIZE;
	memset(exit_st->dev_id, 0, 64);
   	strcpy(exit_st->dev_id, argv[1]);
   	on_exit(restore_tape_config, (void*)exit_st);
	create_scsi_map(&htx_info);
#endif

   sprintf(tmp_msg, "  Device blocksize = %d.\n", BLK_SIZE);
   strcat(msg, tmp_msg);
   hxfmsg(&htx_info, 0, INFO, msg);
   if ( BLK_SIZE == 0 ) {
		strcpy(msg, "Block size is set to ZERO...hxetape will randomize fixed block size selection during each pass.\n");
	    hxfmsg(&htx_info, 0, INFO, msg);
		random_blksize = 1;
   }

       /*********************************************************************/
       /* Open Tape drive                                                   */
       /*  If media is file protected then set Open_Mode = O_RDONLY         */
       /*   else set Open_Mode = O_RDWR.                                    */
       /*********************************************************************/
   Open_Mode = O_RDWR;               /* set default open mode of read/write */
   rule_info.fildes = open(htx_info.sdev_id, Open_Mode);
   if ( rule_info.fildes == -1 ) {
#ifndef __HTX_LINUX__
      if ( errno == EWRPROTECT ) {       /* check for write protected media */
#else
	  if ( errno == EROFS) {       /* check for write protected media */
#endif
         strcpy(msg, "Media is write protected. Opening device in READ "
                     "ONLY mode \n");
         sprintf(tmp_msg, " System error number value : (%d) %s.\n",
                    errno, strerror(errno));
         strcat(msg, tmp_msg);
         hxfmsg(&htx_info, 0, INFO, msg);
         Open_Mode = O_RDONLY;              /*  set open mode for read only  */
         rule_info.fildes = open(htx_info.sdev_id, Open_Mode);
      }
   }
   if ( rule_info.fildes == -1 ) {
      strcpy(msg, "Open error on device - ");
      strcat(msg, htx_info.sdev_id);
      strcat(msg, "\n");
      strcat(msg, strerror(errno));
      strcat(msg, "\n");
      hxfmsg(&htx_info, errno, SYSERR, msg);
      exit(5);
   }
   st_prule_info = &rule_info;        /* pointer to active rule_info struct */
   rule_info.tot_blks = 100000000;                 /* Max tape blocks       */

   cnt = 0;
   while ( (rc = get_rule(&htx_info, &rule_info, rules_file_name)) != EOF ) {
     cnt++;
     if ( rc == 1 ) {
        exit(4);
     }
   }
   sprintf(msg, "Using rules file %s with number of rules %d\n", rules_file_name,cnt);
   hxfmsg(&htx_info, 0, INFO, msg);
   rewind(fptr);
#ifndef __HTX_LINUX__
   if(odm_fscsi_capacity(&htx_info) <0 ) {
		strcpy(msg, "ODM routines failed on device -");
		strcat(msg, htx_info.sdev_id);
		strcat(msg, "\n");
		hxfmsg(&htx_info, 0, HARD, msg);
		exit(6);
   }
#endif

#ifdef __HTX_LINUX__
/*set_tape_config( rule_info.fildes, &htx_info );*/
get_tape_config( rule_info.fildes, &htx_info );
#endif

                  /*********************************/
                  /* Begin the MAIN process        */
                  /*********************************/
   do {
      tape_error_code = 0;
      blk_num.in_rule = 0;
      blk_num.in_file = 0;
      for ( rc = 0; rc < cnt; rc++ )
          rule_stanza[rc] = 0;
      cnt = 0;
      rc = 0;
	  htx_info.test_id = 0;
	/*
	 * here starts random fixed size block testing
	 */
/*#ifndef __HTX_LINUX__*/
	 if(random_blksize)	 {
		if((rc=randomize_fixedblk(&htx_info, &rule_info))< 0){
	   		sprintf(msg, "Block size couldn't be set to random value\n");
			hxfmsg(&htx_info, rc, HARD, msg);
			exit(7);
		};
	 }
/*#endif*/
	 while ( ((get_rule(&htx_info, &rule_info, rules_file_name)) != EOF) &&
              (rc == 0) ) {

	     total_bytes = 0.0;
         start_msg(&htx_info, &rule_info, msg);
         htx_info.test_id = rule_stanza[cnt];
         hxfupdate(UPDATE, &htx_info);
/*#ifndef __HTX_LINUX__*/
		if(random_blksize && (strncmp(rule_info.oper, "VBS", 3) != 0) && rule_info.num_blks >=0 ) {
			unsigned long long expected_capacity;

			/*
			 * If user specified too many blocks which multiplied by BLKSIZE, exceeds total tape capacity,
			 * it would cause problem as we may read or write past end of media.
			 * To avoid this problem, callibrate the no of operations or no of blocks to remain within tape capacity.
			 */
			expected_capacity = (unsigned long long)((unsigned long long)BLK_SIZE * (unsigned long long)rule_info.num_blks*(unsigned long long)rule_info.num_oper);
#ifdef __DEBUG__
			sprintf(msg,"Number of blocks specified is:%d, num of operations:%d\n",
						rule_info.num_blks, rule_info.num_oper);
			hxfmsg(&htx_info, 0, INFO, msg);
#endif
			/* changing this eqn back to original. see defect 401716 for more details */
		    /*
			tmp1 = BLK_SIZE / 1024;
			if(rule_info.num_oper > 1)
				rule_info.num_oper = rule_info.num_oper /tmp1;
			*/
			if(expected_capacity > ASSUMED_TAPE_MIN_CAPACITY) { /* Time to change the number of operations */
			     new_num_oper = (ASSUMED_TAPE_MIN_CAPACITY / (BLK_SIZE * (unsigned long long)rule_info.num_blks)) - 1;
			     printf("new_num_oper = %d\n",new_num_oper);

			     if(new_num_oper < rule_info.num_oper)
       			        rule_info.num_oper = new_num_oper;

			   sprintf(msg,"Calibrated number of operations are ==>%d\n",rule_info.num_oper);
			   hxfmsg(&htx_info, 0, INFO, msg);
		    }
		}
/*#endif*/
         if ( (strncmp(rule_info.oper, "VBS", 3) != 0) &&
              (rule_info.num_blks > 0) ) {

			/* support for buster pattern */
			if( strncmp( rule_info.pattern_id, "BSTR", 4 ) == 0 ) {
				sprintf(msg,"Buster pattern detected in current stanza\n");
				hxfmsg(&htx_info, 0, INFO, msg);

				bus_width = get_bus_width( rule_info.pattern_id );
				sprintf(msg,"Buster pattern width deduced = %d\n", bus_width);
				hxfmsg(&htx_info, 0, INFO, msg);

			}
            /*wptr = (char *) malloc(rule_info.num_blks * BLK_SIZE);
            rptr = (char *) malloc(rule_info.num_blks * BLK_SIZE);*/
            wptr = (char *) malloc((rule_info.num_blks * BLK_SIZE) + bus_width);
            rptr = (char *) malloc((rule_info.num_blks * BLK_SIZE) + bus_width);

            if ( wptr == NULL || rptr == NULL ) {
               sprintf(msg, "Malloc error in Stanza ID > %s\n",
                       rule_info.rule_id);
               hxfmsg(&htx_info, errno, HARD, msg);
               free(wptr);
               free(rptr);
            }

            if( bus_width ) {
				sprintf(msg,"Buster pattern malloc'ed wptr = %llx, rptr = %llx\n", (unsigned long long)wptr, (unsigned long long)rptr);
				hxfmsg(&htx_info, 0, INFO, msg);

				bufrem = (int) ((unsigned long long) wptr % bus_width);
				if( bufrem != 0 ) {
					wptr = wptr + ( bus_width - bufrem );
				}

				bufrem = (int) ((unsigned long long) rptr % bus_width);
				if( bufrem != 0 ) {
					rptr = rptr + ( bus_width - bufrem );
				}

				sprintf(msg,"Buster pattern aligned wptr = %llx, rptr = %llx\n", (unsigned long long)wptr, (unsigned long long)rptr);
				hxfmsg(&htx_info, 0, INFO, msg);
			}
         }
		 gettimeofday(&timer1, NULL);
         if ( strcmp(rule_info.oper, "VBSWR") == 0 )
            rc = VBS_Write(&htx_info, &rule_info, &blk_num);
         else if ( strcmp(rule_info.oper, "VBSRD") == 0 )
            rc = VBS_Read(&htx_info, &rule_info, &blk_num);
         else if ( strcmp(rule_info.oper, "VBSRDC") == 0 )
            rc = VBS_Readc(&htx_info, &rule_info, &blk_num);
         else {
            rc = proc_rule(&htx_info, &rule_info, wptr, rptr, &blk_num);

		 }

         if ( signal_flag == 'Y' )
            rc = -1;

		 gettimeofday(&timer2, NULL);

         finish_stanza(&htx_info, &rule_info, total_bytes, msg);
         if ( (strncmp(rule_info.oper, "VBS", 3) != 0) &&
              (rule_info.num_blks > 0) ) {
            free(wptr);
            free(rptr);
         }
      }
      if ( (signal_flag == 'Y') || (signal_flag == 'R') ) {
         if ( signal_flag == 'Y' ) {
            sprintf(msg, "mkfifo -m 600 %s", pipe_name);
            system(msg);
         }
         fclose(fptr);
         if ( (fptr = fopen(pipe_name, "r")) == NULL ) {
            strcpy(msg, "error opening pipe file - ");
            strcat(msg, strerror(errno));
            strcat(msg, "\n");
            hxfmsg(&htx_info, errno, SYSERR, msg);
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
            hxfmsg(&htx_info, errno, SYSERR, msg);
            return(1);
         }
      } else {
         finish_msg(&htx_info, msg);
         hxfupdate(FINISH, &htx_info);
         rewind(fptr);
      }
   } while ( strcmp(htx_info.run_type,"REG") == 0 ||
             strcmp(htx_info.run_type,"EMC") == 0 ||
             strcmp(htx_info.run_type,"LOP") == 0 );

             /****************************************************************/
             /* close tape, terminate exerciser                              */
             /****************************************************************/
   rc = close(rule_info.fildes);
   if ( rc == -1 ) {
      strcpy(msg, "Close error on ");
      strcat(msg, htx_info.sdev_id);
      strcat(msg, " at exerciser termination.\n");
      strcat(msg, strerror(errno));
      strcat(msg, "\n");
      hxfmsg(&htx_info, errno, HARD, msg);
   }
   exit(0);
}

/**************************************************************************/
/* finish_msg() - Finish message                                          */
/**************************************************************************/
void
finish_msg(struct htx_data *phtx_info, char *msg_text)
{
   if ( tape_error_code ) {
      sprintf(msg_text, "Rule stanza finished w/err #%d - ",
              tape_error_code);
      if ( tape_error_code == 900 )
         strcat(msg_text, "Not enough data returned on a READ");
      else
         strcat(msg_text, strerror(tape_error_code));
      strcat(msg_text, "\nExerciser will sleep 7 seconds then "
                       "attempt a restart.\n");
   } else
      sprintf(msg_text, "Rule File Completed.");
   hxfmsg(phtx_info, 0, INFO, msg_text);
   if (tape_error_code)
     sleep(7);
}

/**************************************************************************/
/* start_msg() - Send Rule started information to HTX.                    */
/**************************************************************************/
void
start_msg(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          char *msg_text)
{
  int  VBS_flag = 0;
  char cmd_function[35], *tmp_msg;

  if ( strcmp(prule_info->oper, "W") == 0 )
     strcpy(cmd_function, "Write");
  else if ( strcmp(prule_info->oper, "R") == 0 )
     strcpy(cmd_function, "Read");
  else if ( strcmp(prule_info->oper, "RC") == 0 )
     strcpy(cmd_function, "Read/Compare");
  else if ( strcmp(prule_info->oper, "D") == 0 )
     strcpy(cmd_function, "Send Diagnostics");
  else if ( strcmp(prule_info->oper, "RW") == 0 )
     strcpy(cmd_function, "Rewind");
  else if ( strcmp(prule_info->oper, "WEOF") == 0 )
     strcpy(cmd_function, "Write End-of-File Mark");
  else if ( strcmp(prule_info->oper, "S") == 0 )
     strcpy(cmd_function, "Delay");
  else if ( strcmp(prule_info->oper, "E") == 0 )
     strcpy(cmd_function, "Erase to End-of-Tape");
  else if ( strcmp(prule_info->oper, "SF") == 0 )
      strcpy(cmd_function, "Skip File Marks");
  else if ( strcmp(prule_info->oper, "SR") == 0 )
      strcpy(cmd_function, "Skip Records");
  else if ( strcmp(prule_info->oper, "C") == 0 )
      strcpy(cmd_function, "Close Device");
  else if ( strcmp(prule_info->oper, "O") == 0 )
      strcpy(cmd_function, "Open Device");
  else if ( strcmp(prule_info->oper, "CO") == 0 )
      strcpy(cmd_function, "Close-Reopen");
  else if ( strcmp(prule_info->oper, "WEOT") == 0 )
      strcpy(cmd_function, "Write to End-of-Tape");
  else if ( strcmp(prule_info->oper, "REOT") == 0 )
      strcpy(cmd_function, "Read to End-of-Tape");
  else if ( strcmp(prule_info->oper, "RCEOT") == 0 )
      strcpy(cmd_function, "Read/Compare to End-of-Tape");
  else if ( strcmp(prule_info->oper, "RS") == 0 )
      strcpy(cmd_function, "Request Sense Data");
  else if ( strcmp(prule_info->oper, "IE") == 0 )
      strcpy(cmd_function, "Adante Initialize Element");
  else if ( strcmp(prule_info->oper, "RES") == 0 )
      strcpy(cmd_function, "Adante Read Element Status");
  else if ( strcmp(prule_info->oper, "ML") == 0 )
      strcpy(cmd_function, "Adante Medium Load");
  else if ( strcmp(prule_info->oper, "MUL") == 0 )
      strcpy(cmd_function, "Adante Medium Unload");
  else if ( strcmp(prule_info->oper, "LB") == 0 )
      strcpy(cmd_function, "Adante Locate Block");
  else if ( strcmp(prule_info->oper, "RP") == 0 )
      strcpy(cmd_function, "Adante Read Tape Position");
  else if ( strcmp(prule_info->oper, "ASF") == 0 )
      strcpy(cmd_function, "Adante Skip File Marks");
  else if ( strcmp(prule_info->oper, "ASR") == 0 )
      strcpy(cmd_function, "Adante Skip Records");
  else if ( strcmp(prule_info->oper, "ADUL") == 0 )
      strcpy(cmd_function, "Adante Serially Unload and Write Tapes");
  else if ( strcmp(prule_info->oper, "TWIE") == 0 )
      strcpy(cmd_function, "Timberwolf Initialize Element");
  else if ( strcmp(prule_info->oper, "TWPE") == 0 )
      strcpy(cmd_function, "Timberwolf Position to Element");
  else if ( strcmp(prule_info->oper, "TWRE") == 0 )
      strcpy(cmd_function, "Timberwolf Read Element Status");
  else if ( strcmp(prule_info->oper, "TWMM") == 0 )
      strcpy(cmd_function, "Timberwolf Move Medium");
  else if ( strcmp(prule_info->oper, "TWUL") == 0 )
      strcpy(cmd_function, "Timberwolf Unload Command");
  else if ( strcmp(prule_info->oper, "WUL") == 0 )
      strcpy(cmd_function, "Timberwolf Write Unload Command");
  else if ( strcmp(prule_info->oper, "CDRE") == 0 )
      strcpy(cmd_function, "Cdat Wolf Read Element Status");
  else if ( strcmp(prule_info->oper, "CDMM") == 0 )
      strcpy(cmd_function, "Cdat Wolf Move Medium Command");
  else if ( strcmp(prule_info->oper, "HUNL") == 0 )
      strcpy(cmd_function, "Hidalgo Move Medium");
  else if ( strcmp(prule_info->oper, "HINI") == 0 )
      strcpy(cmd_function, "Hidalgo Initialize Element");
  else if ( strcmp(prule_info->oper, "HREL") == 0 )
      strcpy(cmd_function, "Hidalgo Read Element Status");
  else if ( strcmp(prule_info->oper, "HWUN") == 0 )
      strcpy(cmd_function, "Hidalgo Write / Unload Tape");
  else if ( strcmp(prule_info->oper, "DBUG") == 0 )
      strcpy(cmd_function, "Set 4mm Debug mode");
  else if ( strcmp(prule_info->oper, "SPEC") == 0 )
      strcpy(cmd_function, "Special Operation");
  else if ( strcmp(prule_info->oper, "XCMD") == 0 )
      strcpy(cmd_function, "Shell Command ");
  else if ( strcmp(prule_info->oper, "VBSWR") == 0 ) {
          strcpy(cmd_function, "Variable Block Size Write");
          VBS_flag = 1;
       }
  else if ( strcmp(prule_info->oper, "VBSRD") == 0 ) {
          strcpy(cmd_function, "Variable Block Size Read");
          VBS_flag = 1;
       }
  else if ( strcmp(prule_info->oper, "VBSRDC") == 0 ) {
          strcpy(cmd_function, "Variable Block Size Read/Compare");
          VBS_flag = 1;
       }
  else strcpy( cmd_function, "Unknown-Command");

  if ( VBS_flag ) {
     if ( prule_info->VBS_seed_type == 'U' )
        tmp_msg = "User-Specified";
     else if ( prule_info->VBS_seed_type == 'R' )
        tmp_msg = "Random";
     else
        tmp_msg = "Previous VBS Stanza";
     sprintf(msg_text, "HXETAPE - %s: %s - %s\nNum_loops: %d; max xfer: %d;"
                       " Pattern: %s\nVBS Seed Type = %s, crash_on_miscompare"
                       " = %d\n",
             prule_info->rule_id, prule_info->oper, cmd_function,
             prule_info->num_oper, prule_info->num_blks, prule_info->pattern_id,
             tmp_msg, crash_on_mis);
  } else
     sprintf(msg_text, "HXETAPE - %s: %s - %s\nNum_Ops: %d; Blocks: %d; "
                       "Pattern: %s; Bytes: %d;\n",
             prule_info->rule_id, prule_info->oper, cmd_function,
             prule_info->num_oper, prule_info->num_blks, prule_info->pattern_id,
             (prule_info->num_blks * BLK_SIZE));
  hxfmsg(phtx_info, 0, INFO, msg_text);
}

/**************************************************************************/
/* finish_stanza() - Finish message                                       */
/**************************************************************************/
void
finish_stanza(struct htx_data *phtx_info, struct ruleinfo *prule_info,
              double total, char *msg_text)
{
   char   tim[20], tot[20], tmpmsg[220];
   double timer3;
   double tmp1, tmp2;

   if ( total > 0 ) {
	  /*
	   * combine secs and microsecs
	   */
	  tmp1 = (double)timer1.tv_sec + (double)timer1.tv_usec * (double)0.000001;
	  tmp2 = (double)timer2.tv_sec + (double)timer2.tv_usec * (double)0.000001;
	  timer3 = (double)tmp2 -(double)tmp1;

      sprintf(tmpmsg, "Stanza %s:  ", prule_info->rule_id);
      strcpy(msg_text, tmpmsg);
      if ( e_notation(timer3, tim) ) {
         sprintf(tmpmsg, "Unable to convert Total Time (%.10e) into "
                         "engineering notation!\n", timer3);
         strcat(msg_text, tmpmsg);
         if ( e_notation(total, tot) )
            sprintf(tmpmsg, "Unable to convert Total Bytes (%.10e) into "
                            "engineering notation!\n", total);
         else
            sprintf(tmpmsg, "Total Bytes = %s bytes\n", tot);
         strcat(msg_text, tmpmsg);
      } else {
         sprintf(tmpmsg, "Total Time = %s seconds;  \n", tim);
         strcat(msg_text, tmpmsg);
         if ( e_notation(total, tot) )
            sprintf(tmpmsg, "Unable to convert Total Bytes (%.10e) into "
                            "engineering notation!\n", total);
         else
            sprintf(tmpmsg, "Total Bytes = %s bytes\n", tot);
         strcat(msg_text, tmpmsg);
      }
      if ( timer3 >= 1.0 ) {
         timer3 =  total / timer3;
         if ( e_notation(timer3, tim) )
            sprintf(tmpmsg, "Unable to convert Average Bytes/Sec (%s) into engineering notation!\n", tim);
         else
            sprintf(tmpmsg, "Stanza averaged %s bytes per second transfer\n",
                    tim);
         strcat(msg_text, tmpmsg);
      }
      hxfmsg(phtx_info, 0, INFO, msg_text);
   }
}

/**************************************************************************/
/* e_notation() - Routine to generate a string representation of a number */
/*                in a format similar to engineering notation where the   */
/*                exponent is always a power of 3. This routine is limited*/
/*                to positive numbers. It does not round the number based */
/*                on precision; instead it truncates digits past the pre- */
/*                cision. It doesn't require logarithms so runs pretty    */
/*                fast. Best suited for numbers greater the 999.          */
/**************************************************************************/
int e_notation(double d, char *s)
{
#ifdef __HTX_LINUX__
  sprintf(s,"%g",d);

#else

  #define DIGITS 4	                            /* Sets the precision */
  int i, j, k, dig;

  if ( d < 0.0 ) {
     *s = '\0';
     return(-1);
  }
  if ( d < 1000.0 )
     sprintf(s, "%.3f", d);
  else {
     /*************************************************************************
     ** The number is >= 1000 so we need to convert to engineering notation.
     ** First, convert the number to a string, figure out how many digits it
     ** has, put the decimal place in the right place after shifting rightmost
     ** digits to the right one place, add the proper exponent, and we're done.
     **************************************************************************/
     sprintf(s, "%.0lf", d);
     i = strlen(s);
     dig = (i < DIGITS) ? i : DIGITS;
     j = ((i - 4) % 3) + 1;
     for ( k = dig - 1; k >= j; k-- )
        s[k+1] = s[k];
     s[j]='.';
     s[dig+1]='e';
     sprintf(s+dig+2, "%d", ((i - dig) / 3 + 1) * 3);
  }
#endif
  return(0);
}


/**************************************************************************/
/* SIGTERM_hdl() - SIGTERM Handler Routine                                */
/**************************************************************************/
void SIGTERM_hdl (int sig, int code, struct sigcontext *scp)
{
#ifdef _DEBUG_LINUX_
   printf("Inside signal handler of signal:%d\n",sig);
#endif
   close(st_prule_info->fildes);
   exit(0);
}

#ifndef __HTX_LINUX__
/*
 * Subroutine to find wether tape device is connected via Fiber Channel or Parallel SCSI
 * Also finds tape capacity
 */

int
odm_fscsi_capacity(struct htx_data *htx_info)
{
	char   s[50],msg[50],att[50];
	struct Class *pClass;
	struct CuDv cusobj;
	struct CuAt	*attobj;
	char *start, *end;
	int rc,len=0;
	int get_all=0, how_many;


	if ( odm_initialize() == -1 ) {
    	sprintf(msg,"Error initializing ODM\n");
		hxfmsg(htx_info, 0, INFO, msg);
		return -1;
	}
	if ( (pClass = odm_open_class(CuDv_CLASS)) == NULL ) {
    	sprintf(msg,"odm_open_class(CuDv_CLASS) failed\n");
		hxfmsg(htx_info, 0, INFO, msg);
		return -1;
	}
	start = &(htx_info->sdev_id[5]);
	end = strstr(start, ".1");
	len = (int)(end - start);
	strcpy(s, "name = ");

	if(end == NULL) {
		strcat(s, &(htx_info->sdev_id[5]));
	}
	else {
		strncat(s, &(htx_info->sdev_id[5]), len);
	}
	if ( (rc = (int) odm_get_first(pClass, s, &cusobj)) == 0 ) {
    	sprintf(msg,"Couldn't find %s in CuDv\n", s);
		hxfmsg(htx_info, 0, INFO, msg);
		return -1;
	}
	else if (rc == -1) {
    	sprintf(msg,"odm_get_first() failed\n");
	    hxfmsg(htx_info, 0, INFO, msg);
		return -1;
  	}
	if (odm_close_class(CuDv_CLASS) == -1) {
    	sprintf(msg,"odm_close_class(CuDv_CLASS) failed\n");
	    hxfmsg(htx_info, 0, INFO, msg);
		return -1;
  	}
 	sprintf(msg,"Parent of device is:%s\n",cusobj.parent);
    hxfmsg(htx_info, 0, INFO, msg);


	if(strncmp(cusobj.parent, "fscsi",5)== 0) {
		sprintf(msg,"Found SCSI tape device connected through FC\n");
	    hxfmsg(htx_info, 0, INFO, msg);

		found_fscsi =1;
	}

	/*if((strncmp(cusobj.parent , "scsi",4) != 0)&& (strncmp(cusobj.parent, "fscsi",5)!=0) && random_blksize) { */
	if((strncmp(cusobj.parent , "scsi",4) != 0)&& (strncmp(cusobj.parent, "fscsi",5)!=0) && (strncmp(cusobj.parent , "sas",3) != 0) && random_blksize) {
		sprintf(msg,"block size of 0 only supported for SCSI and FSCSI devices\n");
	    hxfmsg(htx_info, 0, HARD, msg);
		exit(1);
	}

	if(	random_blksize == 0)
		return 0;


	/*
	 * now find out the capacity of tape from CuAt
	 * FC devices doesn't this field registered into CuAt
	 * for now assuming default 20 GB
	 */

	tape_capacity = 0;

	strcpy(s, &(htx_info->sdev_id[5]));
	strcpy(att, "size_in_mb");
	attobj = (struct CuAt*)getattr(s, att, get_all, &how_many );
	if(attobj == NULL) {
		if(found_fscsi == 0) {
		 	sprintf(msg,"Couldn't find %s in CuAt with errno:%d\n", s,errno);
			hxfmsg(htx_info, 0, INFO, msg);
			return -1;
		}
		else {
			tape_capacity = (unsigned long long)(20ull*1024ull*1024ull*1024ull);
			sprintf(msg, "Couldn't find capacity in CuAt. Assuming default 0x%llx",tape_capacity);
			hxfmsg(htx_info, 0, INFO, msg);
			return 0;
		}

	}

	tape_capacity = (unsigned long long)((unsigned long long)atol(attobj->value) *1024ull *1024ull);

	sprintf(msg,"Tape capacity is:0x%llx\n",tape_capacity);
	hxfmsg(htx_info, 0, INFO, msg);

	return 0;
}


#else /* Linux */


void
restore_tape_config(int arg1, void *arg2)
{
	struct on_exit_st *local= (struct on_exit_st*)arg2;
	struct mtop tmp;
	int fd;

#ifdef _DEBUG_LINUX_
	printf("restore_tape_config called with exit no:%d\n",arg1);
	printf("Saved blocksize:%u\n",local->blk_size);
#endif
	/*
	 * we can't call set_bs 'us file descriptor is closed
	 *
	set_bs(local->htx_info, local->rule_info, local->blk_size, 0);
	 */

	/*
	 * set default open mode of read/write
	 */
   fd = open(local->dev_id, O_RDWR);
   tmp.mt_op = MTSETBLK ;
   tmp.mt_count = local->blk_size ;
   ioctl(fd, MTIOCTOP, &tmp);
   close(fd);
}

static void
create_scsi_map(struct htx_data *htx_info)
{
	int scsi_id=0,st_id=0;
	struct scsi_mapping *tmp;
	char tmp_str[80], str[80],x[32];
	FILE* fp;


	sg_st_map = (struct scsi_mapping*)malloc(sizeof(struct scsi_mapping));
	if(sg_st_map == NULL) {
		sprintf(tmp_str,"Unable to malloc for SCSI mapping errno: %d(%s)\n",errno, strerror(errno));
		hxfmsg(htx_info, errno, SYSERR, tmp_str);
		exit(3);
	}

	tmp = sg_st_map;
	tmp->next = NULL;

	/*fd = open("/proc/scsi/scsi", O_RDONLY);*/
	if(( fp = fopen("/proc/scsi/scsi", "r")) == NULL ) {
		sprintf(tmp_str,"Unable to open /proc/scsi/scsi file errno= %d(%s)\n",errno, strerror(errno));
		hxfmsg(htx_info, errno, SYSERR, tmp_str);
		exit(4);
	}

	/*
	 * here starts the reading of /proc/scsi/scsi file
	 */

	/*
	rc = read(fd, &ch,1);
		while(ch != '\n') {
			str[count++] = ch;
			rc = read(fd, &ch,1);
	}
	str[count] ='\0';
	while(rc != 0) {
		count = 0;
		rc = read(fd, &ch,1);
		if(rc == 0)
			break;

		while(ch != '\n') {
			str[count++] = ch;
			rc = read(fd, &ch,1);
		}
		str[count] ='\0';
		count =0;
		rc = read(fd, &ch,1);
		while(ch != '\n') {
			str[count++] = ch;
			rc = read(fd, &ch,1);
		}
		str[count] ='\0';
		count =0;
		rc = read(fd, &ch,1);
		while(ch != '\n') {
			str[count++] = ch;
			rc = read(fd, &ch,1);
		}
		str[count] ='\0';
		sscanf(str,"%s",x);
		sscanf(str+7,"%s",x); */

		getline_new( str, 80, fp ); /* dummy read to bypass first line */

		while( getline_new( str, 80, fp ) > 0 ) {

			getline_new( str, 80, fp );
			getline_new( str, 80, fp );

			if(strstr(str,"Sequential-Access") != NULL ) {
	#ifdef _DEBUG_LINUX_
				printf("Mapping is SCSI_ID:%d==>TAPE:%d\n",scsi_id, st_id);
	#endif
				tmp->st[0] ='\0';
				tmp->sg[0] ='\0';
				if(strncmp(htx_info->sdev_id, "/dev/st", 7) == 0)
					sprintf(x,"/dev/st%d",st_id);
				else if(strncmp(htx_info->sdev_id, "/dev/nst",8) == 0)
					sprintf(x, "dev/nst%d",st_id);
				else if(strncmp(htx_info->sdev_id, "/dev/IBMtape",12) == 0)
					sprintf(x,"/dev/IBMtape%d",st_id);
				else {
	#ifdef _DEBUG_LINUX_
					printf("tape device <%s>not listed, still creating the mapping\n",htx_info->sdev_id);
	#endif
					sprintf(x,"%d",st_id);
				}
				strcpy(tmp->st,x);
				sprintf(x,"/dev/sg%d",scsi_id);
				strcpy(tmp->sg,x);


	#ifdef _DEBUG_LINUX_
				printf("\n\nCreated Mappings\n\n");
				printf("%s <==> %s\n",tmp->sg,tmp->st);
	#endif
				tmp->next = (struct scsi_mapping*)malloc(sizeof(struct scsi_mapping));
				if(tmp->next == NULL) {
					sprintf(tmp_str,"Unable to malloc for SCSI mapping errno: %d(%s)\n",errno, strerror(errno));
					hxfmsg(htx_info, errno, SYSERR, tmp_str);
					exit(3);
				}
				tmp = tmp->next;
				tmp->next = NULL;

				st_id++;
			}
			scsi_id++;
		}

	fclose(fp);
}



void set_tape_config( int fildes, struct htx_data *pHTX )
{
  struct mtop tmp;
  char sWork[512];
  int rc;


  tmp.mt_op = MTSETDRVBUFFER;
  tmp.mt_count = MT_ST_SETBOOLEANS | MT_ST_NO_BLKLIMS;

  sWork[0]= '\0';
  if ( (rc = ioctl(fildes, MTIOCTOP, &tmp)) == -1 ) {
     sprintf(sWork, "set_tape_config: Error %d (%s) changing to NO block limits\n",
             errno, strerror(errno));
     hxfmsg(pHTX, errno, HARD, sWork);
     tape_error_code = errno;
  } else {
     sprintf(sWork, "set_tape_config: NO BLKLIMS set successfully\n");
     hxfmsg(pHTX, errno, INFO, sWork);
  }
}



void get_tape_config( int fildes, struct htx_data *pHTX )
{
	char sWork[512];

	/* First, get the blk limits and update l/m_blk_limit vars. */

	#define RBL_CMD_CODE 0x05
	#define RBL_CMD_LEN  6
	#define RBL_REPLY_LEN 6

	char rbuf[RBL_REPLY_LEN] = {0};
	unsigned char cdb[RBL_CMD_LEN] = {RBL_CMD_CODE, 0, 0, 0, 0, 0 };

	unsigned int rbl_gran, rbl_l_limit, rbl_m_limit;

	sg_io_hdr_t rbl;

	memset(&rbl, 0, sizeof(sg_io_hdr_t));

	rbl.interface_id = 'S';
	rbl.cmd_len = sizeof(cdb);
	/* rbl.iovec_count = 0; */  /* memset takes care of this */
	rbl.mx_sb_len = 0;
	rbl.dxfer_direction = SG_DXFER_FROM_DEV;
	rbl.dxfer_len = RBL_REPLY_LEN;
	rbl.dxferp = rbuf;
	rbl.cmdp = cdb;
	rbl.sbp = 0;
	rbl.timeout = 20000;     /* 20000 millisecs == 20 seconds */
	/* rbl.flags = 0; */     /* take defaults: indirect IO, etc */
	/* rbl.pack_id = 0; */
	/* rbl.usr_ptr = NULL; */

	if (ioctl(fildes, SG_IO, &rbl) < 0) {
		sprintf(sWork, "get_tape_config: READ_BLK_LIMITS SG_IO ioctl error, errno=%d\n", errno);
		hxfmsg(pHTX, errno, HARD, sWork);
		tape_error_code = errno;
	 } else {

		sprintf(sWork, "get_tape_config: READ_BLK_LIMITS SG_IO ioctl success\n");
		hxfmsg(pHTX, errno, INFO, sWork);

		rbl_gran = ( unsigned int )( ( rbuf[0] & 0x1F ) );
		rbl_m_limit = ( unsigned int ) ( ( rbuf[1] << 16 ) | ( rbuf[2] << 8 ) | ( rbuf[3] ) ) ;
		rbl_l_limit = ( unsigned int ) ( ( rbuf[4] << 8 ) | ( rbuf[5] ) ) ;

		sprintf(sWork, "get_tape_config: READ_BLK_LIMITS gran=%d, l-limit=%d, m-limit=%d\n", rbl_gran, rbl_l_limit, rbl_m_limit);
		hxfmsg(pHTX, errno, INFO, sWork);

		gran = rbl_gran;
		l_blk_limit = rbl_l_limit;
		m_blk_limit = rbl_m_limit;

	 }

}

int getline_new(char s[], int lim, FILE* fp)
{
	  s[0] = '\0';
	  if ( fgets(s, lim, fp) != NULL )
	     return(strlen(s));
	  else
	     return(0);
}

#endif

/* This function returns the width of the buster pattern from the pattern id specified */
/* The buster pattern width is made part of the pattern id, as in ... */
/* BSTR_2 -> buster pattern of width 2 bytes */
/* This function assumes that the pattern_id passed is of type BSTR_XXX, and hence parses only the digit part */

int get_bus_width( char* pattern_id )
{
	char digit[3] = "   ";
	int i = 0, k = 0, d, found = 0;

	d = strlen( pattern_id );

	for ( k = 0; k < d; k++ ) {
	   if ( isdigit(pattern_id[k]) ) {
		  digit[i] = pattern_id[k];
		  i++;
		  found = 1;
	   }
	}

	if( found ) {
		return atoi(digit);
	} else {
		return 0; /* was not able to deduce the bus width */
	}

}

/*
 * This function sets tape block size to random fixed block size
 */

static int
randomize_fixedblk(struct htx_data *pHTX, struct ruleinfo *pRule)
{
	static int first=0;
	static unsigned short blkseed[3];
	unsigned long blksize,tmp;
	int rc;
	char msg[50];

	if(first == 0) { /*initialise seed only first time*/
		init_seed(blkseed);
		first = 1;
	}

	/*
	 * blksize should be in range 512-->TAPE_MAXREQUEST
	 * if 0 is returned make blksize as TAPE_MAXREQUEST
	 */

	#ifndef __HTX_LINUX__
		blksize = nrand48(blkseed) % TAPE_MAXREQUEST;
	#else
		/* respect the block limits */
		/* !!! IMP NOTE: Here its assumed that l_blk_limt and m_blk_limit are already initialised !!! */
		blksize = l_blk_limit + ( nrand48(blkseed) % m_blk_limit);
	#endif

#ifdef __DEBUG__
	sprintf(msg, "first blksize after call to nrand48:%d\n",blksize);
	hxfmsg(pHTX, 0, INFO, msg);
#endif

	tmp = blksize%512;
	blksize = blksize - tmp;	/*should be in multiples of 512*/
#ifdef __DEBUG__
	sprintf(msg, "intermediate blksize:%d and tmp:%d\n",blksize,tmp);
	hxfmsg(pHTX, 0, INFO, msg);
#endif

	if(blksize == 0) {
		#ifndef __HTX_LINUX__
			blksize = TAPE_MAXREQUEST;
		#else
			blksize = m_blk_limit;
		#endif
	}
#ifdef __DEBUG__
	sprintf(msg, "final blksize =%d\n",blksize);
	hxfmsg(pHTX, 0, INFO, msg);
#endif
	if((rc=set_bs(pHTX, pRule, blksize, 0)) == 0) {
		BLK_SIZE = blksize;
		sprintf(msg, "BLKSIZE set successfully:%d\n",BLK_SIZE);
    	hxfmsg(pHTX, 0, INFO, msg);
	}
	return rc;
}
