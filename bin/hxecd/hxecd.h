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
/*  "@(#)30  1.17.4.2  src/htx/usr/lpp/htx/bin/hxecd/hxecd.h, exer_cd, htxubuntu 1/20/14 05:23:38" */
/******************************************************************************
 *   COMPONENT_NAME: exer_cd
 *
 *   MODULE NAME: hxecd.h
 *
 *   FUNCTIONS: none
 *
 ******************************************************************************/

#include <stdio.h>

#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "hxihtx.h"

#ifndef __HTX_LINUX__     /* AIX */

#include <sys/cfgodm.h>
#include <sys/devinfo.h>
#include <sys/cdrom.h>
#include <sys/ide.h>
#include <sys/scdisk.h>
#include <sys/scsi_buf.h>
#include <sys/scsi.h>
#include <sys/mode.h>
#else                     /* Linux */
#include <ctype.h>
#include <scsi/sg.h>
#include <linux/cdrom.h>
#endif

#define MAX_BLKNO 269250               /* maximum block number                */
#define MIN_BLKNO 050570               /* minimum block number                */
#define BUF_SIZE  (512*1024)           /* read/write buffer size              */
#define HARD 1
#define SOFT 4
#define INFO 7
#define SYSERR 0
#define CD_MSF_MODE  1          /*- define minutes, seconds, frames lba mode -*/
#define DEFAULT_TRK_INDEX 0x01;     /*- define track index for play audio trk */
#define cd_BLOCKSIZE    2048                  /* Disk blocksize               */
#define cd_M2F1_BLKSIZE 2048                  /* The block size for CD-ROM    */
                                              /* XA data mode 2 form 1.       */
#define cd_M2F2_BLKSIZE 2336                  /* The block size for CD-ROM    */
                                              /* XA data mode 2 form 2.       */
#define cd_CDDA_BLKSIZE 2352                  /* The block size for CD-DA     */

#define cd_LASTLBA      269249
#define dvd_LASTLBA     4169951

#define cd_TOC_ENTRY_LBA  264675

struct ruleinfo {
  int   op_in_progress;     /* operation in progress used with SIGTERM shutdwn*/
                            /* 0 = undefined operation, no shutdown needed    */
                            /* 1 = Toshiba vendor unique play audio active    */
                            /* 2 = scsi play play audio active                */
                            /* 3 = device driver CD_PLAY_AUDIO active         */
  int   master_audio_type;  /* 0 = standard play audio scsi commands          */
                            /* 1 = vendor unique audio cmds - TOSHIBA - 3101  */
                            /*     this model does not support read toc x'43' */
                            /* 2 = vndr unique audio cmds - TOSHIBA SLIM-LINE */
  struct cd_pns {
   char  disc_pn[20];       /* part number found fron toc data                */
   char  rule_disc_pn[20];  /* part number specified in rules file            */
  } cds;                    /*  with DISC_PN = xxxxxxxx                       */
  char  rule_id[9];         /* Rule Id                                        */
  char  pattern_id[9];      /* pattern library id                             */
  char  mode[10];           /* cd-rom mode for mode select                    */
  char  retries[5];         /* ON, OFF, BOTH                                  */
  char  addr_type[7];       /* SEQ, RANDOM                                    */
  int   num_oper;           /* number of operations to be performed           */
  char  oper[5];            /* type of operation to be performed              */
  char  starting_block[9];  /* n,mm/ss/bb,TOP,MID,BOT                         */
  int   first_block;        /* starting block converted to numeric            */
  int   msf_mode;           /* true = mm:ss:ff format for lba                 */
                            /* false = use track/block number block length    */
  char  direction[5];       /* UP, DOWN, IN, OUT           * seq oper only    */
  int   increment;          /* number of blocks to skip    *                  */
  char  type_length[7];     /* FIXED, RANDOM                                  */
  int   num_blks;           /* length of data to read in blocks               */
  int   fildes;             /* file descriptor for device to be exercised     */
  unsigned int dlen;        /* length of data to read in bytes                */
  long  tot_blks;           /* Total number of blocks on disc                 */
  long  min_blkno;          /* lowest block to be used                        */
  long  max_blkno;          /* highest block to be used                       */
  int   bytpsec;            /* number of bytes per sector ie. block length    */
  char  cmd_list[250];      /* array to hold commands to be run from shell    */
} ;


/* Header files different for AIX and Linux */

#ifdef __HTX_LINUX__     /* Linux */

/* Macro to page align pointer to next page boundary */
#ifdef __DEFINE_PAGE_MACROS__
#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE-1))
#endif

#define HTX_PAGE_ALIGN(_X_) ((((unsigned long)_X_)+PAGE_SIZE-1)&PAGE_MASK)

/* MODE DEFINITIONS */

#define CD_MODE1           0x1
#define CD_MODE2_FORM1     0x2
#define CD_MODE2_FORM2     0x3
#define CD_DA              0x4

typedef off64_t  offset_t;

/* Linux compatibility layer for AIX-HTX function calls */

/* Drop extended attributes with openx */
#define openx(_P1_, _P2_, _P3_, _P4_) open(_P1_, _P2_)

#endif

/* Function declarations */
int get_disc_capacity(struct htx_data *, struct ruleinfo *, unsigned int *, unsigned int *);
int get_audio_type(struct ruleinfo *, struct htx_data *);
int ms_get(struct ruleinfo *, struct htx_data *, unsigned int *, unsigned int *);
int get_disc_pn(struct htx_data *, struct ruleinfo *);
int get_rule(struct htx_data *, struct ruleinfo *);
void start_msg(struct htx_data *, struct ruleinfo *, char *);
int proc_rule(struct htx_data *, struct ruleinfo *, char *, char *, unsigned int *, unsigned int *);
int halt_audio_mm(int);
void set_defaults(struct ruleinfo *);
int htx_getline(char *, int);
int set_first_blk(struct ruleinfo *);
void init_seed(unsigned short *);
int random_dlen(short, long, unsigned short *);
void init_blkno(struct ruleinfo *, int *);
void random_blkno(int *, unsigned int, short, long, unsigned short *, long);
int read_cdrom(struct htx_data *, struct ruleinfo *, int, int *, char *);
void read_write_pattern(struct htx_data *, struct ruleinfo *, int, int *, char *);
char cmpbuf(struct htx_data *, struct ruleinfo *, int, int *, char *, char *);
void prt_msg(struct htx_data *, struct ruleinfo *, int, int*, int, int, char *);
void set_blkno(int *, char *, int, int);
void diag_cdrom(struct htx_data *, struct ruleinfo *, int, int *);
void audio_cdrom(struct htx_data *, struct ruleinfo *, int, int *);
int audio_mm(struct htx_data *, struct ruleinfo *, int, int *);
void prt_req_sense(struct htx_data *, struct ruleinfo *, int, int *);
void do_sleep(struct htx_data *, struct ruleinfo *, int, int *);
int do_cmd(struct htx_data *, struct ruleinfo *);
void info_msg(struct htx_data *,struct ruleinfo *, int, int *, char *);
int wrap(struct ruleinfo *, int *);
void init_seed(unsigned short *);
void prt_msg_asis(struct htx_data *, struct ruleinfo *, int, int *, int, int, char *);
int read_subchannel(struct htx_data *, struct ruleinfo *, int, int *);
#ifndef __HTX_LINUX__
int halt_audio_cdrom(int);
int play_audio_msf(struct htx_data *, struct ruleinfo *, int, int *, struct cd_audio_cmd *);
void init_iocmd (struct sc_iocmd *);
void init_viocmd (struct scsi_iocmd *);
#endif
