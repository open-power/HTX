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
/*@(#)15  1.4  src/htx/usr/lpp/htx/bin/hxsmsg/hxsmsg.h, htx_msg, htxubuntu 10/19/03 23:38:39*/
/* *   ORIGINS: 27 */


#ifndef HXSMSG_H
#define HXSMSG_H

/*
 ******************************************************************************
 ***  hxsmsg.h -- "hxsmsg" HTX Message Handler Include File  ******************
 ******************************************************************************
 */                                                          

#include <htx_local.h>
#include <hxiipc.h>

#include "hxsmsgdef.h"
#ifdef __HTX_LINUX__
#include "nfdef.h"
#endif

/*
 ***  miscellaneous #defines  *************************************************
 */

#define DISK_BUF_SIZE MSG_TEXT_SIZE

#define BAD_CLEAN_UP 0x0020    /* main() and SIGTERM_hdl() common error code */

#define SIGTERM_RECVD 0x0040   /* exit code bit set upon receipt of SIGTERM  */


/* 
 ***  file name #defines  *****************************************************
 */
extern char global_htx_home_dir[256];
extern char global_htx_log_dir[256];
extern char CKPT_FILE[256];
extern char ERR_LOG[256];
extern char ERR_SAVE_LOG[256];
extern char MSG_LOG[256];
extern char MSG_SAVE_LOG[256];


/* 
 *  check-point file structure  ***********************************************
 */

  typedef struct hxsmsg_ckpt
{
  /*
   * max file size threshold values 
   */
  off_t err_thres;              /* max error log file size threshold         */
  off_t err_save_thres;         /* max save err log file size threshold      */
  off_t msg_thres;              /* max msg log file size threshold           */
  off_t msg_save_thres;         /* max save msg log file size threshold      */
  
  /*
   * file offsets (pointers into files)
   */
  off_t err_offset;             /* error log file offset (pointer into file) */
  off_t err_save_offset;        /* save err log file offset (ptr into file)  */
  off_t msg_offset;             /* msg log file offset (pointer into file)   */
  off_t msg_save_offset;        /* save msg log file offset (ptr into file)  */
  
  /*
   * file modification times
   */
  time_t err_mod_time;          /* error log modification time               */
  time_t err_save_mod_time;     /* save error log modification time          */
  time_t msg_mod_time;          /* message log modification time             */
  time_t msg_save_mod_time;     /* save message log modification time        */
  
  /*
   * ckpt structure checksum
   */
  long   checksum;              /* ckpt structure checksum                   */
} ckpt;

void set_htx_home_log_path(void);

#endif  /* HXSMSG_H */
