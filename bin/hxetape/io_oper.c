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

/* @(#)12	1.46.2.5  src/htx/usr/lpp/htx/bin/hxetape/io_oper.c, exer_tape, htxubuntu 2/26/13 03:36:02 */

/******************************************************************************
 * COMPONENT_NAME: exer_tape
 *
 * MODULE NAME: io_oper.c
 *
 * FUNCTIONS : update_blk_nums() - updates blk number pointers
 *                  write_tape() - writes data to tape.
 *                   read_tape() - reads data from tape.
 *                 rewind_tape() - rewinds tape.
 *                   weof_tape() - writes end-of-file marker on tape.
 *                 search_file() - skips forward/backward over file marks.
 *                  search_rec() - skips forward/backward over records.
 *                   diag_tape() - request sense data using diagnostics mode
 *                  erase_tape() - erases the remainder of the tape.
 *                    do_sleep() - delay num_blks in seconds.
 *                  close_open() - close then re-open device.
 *                     t_close() - close of tape device.
 *                      t_open() - open of tape device.
 *                   write_eot() - write to end-of-tape.
 *                    read_eot() - reads to end-of-tape with compare of data.
 *                   read_teot() - reads to end-of-tape.
 *               prt_req_sense() - acquires and prints sense data from device.
 *                    set_dbug() - set debug monitor mode in 4mm4gb devices.
 *                      set_bs() - reset tape blocksize.
 *                   VBS_Write() - variable block size write test.
 *                    VBS_Read() - variable block size read test.
 *                   VBS_Readc() - variable block size read/compare test.
 *                init_element() - adante initialize element.
 *                 read_status() - adante read element status.
 *                 medium_load() - adante move medium tape load.
 *               medium_unload() - adante move medium tape unload.
 *                   loc_block() - adante locate to a specific block.
 *                  read_posit() - adante read position on tape.
 *                asearch_file() - adante skip file marks on tape.
 *                 asearch_rec() - adante skip record marks on tape.
 *                write_unload() - adante write EOT, unload tape, do it again.
 *                   twin_tape() - timberwolf initialize element.
 *                   twps_tape() - timberwolf position to element.
 *                   twrd_stat() - timberwolf read element status.
 *                   twmv_tape() - timberwolf move medium command.
 *                 tape_unload() - timberwolf write/unload tape/write
 *                unload_write() - timberwolf write EOT/unload tape/begin again.
 *                   cdmv_tape() - cdatwolf move medium command.
 *                   cdrd_stat() - cdatwolf read element status.
 *                      himove() - hidalgo move medium command.
 *                      hielem() - hidalgo read element status command.
 *                      hiinit() - hidalgo initialize element command.
 *                hidal_unload() - hidalgo write EOT/unload tape/begin again.
 *	NOTE:		All error checking after close call is disabled due to bug in AIX tape driver, which returns
 *	uninitalize variable as errno
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ioctl.h>
#ifndef __HTX_LINUX__
#include <sys/devinfo.h>
#include <sys/scsi.h>
#include <sys/scsi_buf.h>
#include <sys/scdisk.h>
#include <sys/tape.h>
#include <sys/tmscsi.h>
#include "Atape.h"
#include "4mm_auto.h"
#include "hxetape.h"
#else
#include <sys/mtio.h>
#include <stdio.h>
#include <unistd.h>
#include <scsi/sg.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <4mm_auto.h>
#include <hxetape.h>

#endif


#ifdef __HTX_LINUX__
#define SCSI_OFF sizeof(struct sg_header)
static  unsigned char cmd[SCSI_OFF + 18];
extern struct scsi_mapping *sg_st_map;

extern unsigned int l_blk_limit;
extern unsigned int m_blk_limit;

#endif

/**************************************************************************/
/* declare local sense and message buffers                                */
/**************************************************************************/
#define SENSE_BUF_SIZE	1024
#define MAX_SENSE 64
#define ELEMENT_REPORT_LEN 255
#define MAX_PASS_THRU_BUFF_LEN 255
#define MAX_PASS_THRU_TRACE_LEN 128

int           	tape_error_code, VBS_seed_initialized = 0;
int           	bytes_read, bytes_wrote;
static char   	msg_str[1024];
extern int		VBS_last_write;
extern unsigned int BLK_SIZE;
extern int          Open_Mode;
extern int          crash_on_mis;
#ifndef __HTX_LINUX__
static char		*sense_buf, *data_buffer;
static struct 	sc_iocmd iocmd_buf;
static struct 	scsi_iocmd fiocmd_buf;
extern int		found_fscsi;
static int buffer_initialized = 0;
#endif
extern int random_blksize;
extern double 	total_bytes;

/* Support for SIOC_PASSTHRU_COMMAND for fscsi drives */
#ifndef __HTX_LINUX__
static struct scsi_passthru_cmd scsi_passthru_cmd_buf;
char pass_thru_buff[MAX_PASS_THRU_BUFF_LEN];
#endif

static void update_blk_nums(struct blk_num_typ *pblk_num, int num_bytes);
extern int get_bus_width( char* pattern_id ); /* for buster pattern support */

/*************************************************************************/
/* update_blk_nums() - Calculate number of blocks contained in byte      */
/* count and update the block number pointer values.                     */
/*************************************************************************/
static void
update_blk_nums(struct blk_num_typ *pblk_num, int num_bytes)
{
   pblk_num->in_file = pblk_num->in_file + (num_bytes / BLK_SIZE);
   pblk_num->in_rule = pblk_num->in_rule + (num_bytes / BLK_SIZE);
   total_bytes += (double)num_bytes;
}

/**************************************************************************/
/* write_tape() - Writes data to tape.                                    */
/**************************************************************************/
int
write_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
           int loop, struct blk_num_typ *pblk_num, char *wbuf)
{
  int  rc;

  lseek(prule_info->fildes, 0, 1);
  rc = write(prule_info->fildes, wbuf, (prule_info->num_blks * BLK_SIZE));
  if ( rc == -1 ) {
     phtx_info->bad_writes = phtx_info->bad_writes + 1;
     if ( errno == ENXIO ) {
        prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                     "Write error attempting to write past the end-of-media.");
        tape_error_code = errno;
     } else if ( (errno == EBADF) && (Open_Mode == O_RDONLY) ) {
        prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                     "Write error due to Write-Protected media.");
        tape_error_code = errno;
     } else {
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                "Write error - ");
        tape_error_code = errno;
     }
  } else {
     bytes_wrote = rc;
     update_blk_nums(pblk_num, rc);
     phtx_info->good_writes = phtx_info->good_writes + 1;
     phtx_info->bytes_writ = phtx_info->bytes_writ + rc;
     rc = 0;
  }
  return(rc);
}

/**************************************************************************/
/* read_tape() - Reads data from the tape.                                */
/**************************************************************************/
int
read_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          int loop, struct blk_num_typ *pblk_num, char *rbuf)
{
  int  rc;
  char msg[120];

  memset(rbuf, 0xbb, (prule_info->num_blks * BLK_SIZE));
  lseek(prule_info->fildes, 0, 1);
  rc = read(prule_info->fildes, rbuf, (prule_info->num_blks * BLK_SIZE));
  if ( rc == -1 ) {
     phtx_info->bad_reads = phtx_info->bad_reads + 1;
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
             "Read error - ");
     tape_error_code = errno;
  } else if ( rc < (prule_info->num_blks * BLK_SIZE) ) {
     bytes_read = rc;
     phtx_info->bad_reads = phtx_info->bad_reads + 1;
     sprintf(msg, "READ returned less data than requested - \n"
                  "Bytes requested:  %d  >  Bytes received:  %d\n",
             (prule_info->num_blks * BLK_SIZE), rc);
     rc = tape_error_code = errno = 900;
     prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD, msg);
  } else {
     update_blk_nums(pblk_num, rc);
     phtx_info->good_reads = phtx_info->good_reads + 1;
     phtx_info->bytes_read = phtx_info->bytes_read + rc;
     rc = 0;
  }
  return(rc);
}

/**************************************************************************/
/* rewind_tape() - Rewind tape to beginning of media.                     */
/**************************************************************************/
int
rewind_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
            int loop, struct blk_num_typ *pblk_num)
{
  int    rc;
  struct stop tapeinfo;

#ifdef __HTX_LINUX__
   /*
	* need to debug this..
	* why its needed
	*/
  close(prule_info->fildes);
  prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
#endif

  pblk_num->in_file = 0;           /* zero block count pointers in rewinds */
  pblk_num->in_rule = 0;
  tapeinfo.st_op = STREW;
  rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
  if ( rc == -1 ) {
     phtx_info->bad_others = phtx_info->bad_others + 1;
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
             "Rewind error - ");
     tape_error_code = errno;
  }
  return(rc);
}

/**************************************************************************/
/* weof_tape() - Write end-of-file marker on tape.                        */
/**************************************************************************/
int
weof_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
          struct blk_num_typ *pblk_num)
{
  int    rc;
  struct stop tapeinfo;

  tapeinfo.st_op = STWEOF;
  tapeinfo.st_count = prule_info->num_blks;
  rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
  if ( rc == -1 ) {
     phtx_info->bad_writes = phtx_info->bad_writes + 1;
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
             "Write EOF error - ");
     tape_error_code = errno;
  } else {
     pblk_num->in_file = 0; /* zero block count pointers in write file marks */
     pblk_num->in_rule = 0;
  }
  return(rc);
}

/**************************************************************************/
/* search_file() - This function will skip forward prule_info->num_blks   */
/* of file marks. If prule_info->num_blks is negative, then the skipping  */
/* is backwards.                                                          */
/**************************************************************************/
int
search_file(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
            struct blk_num_typ *pblk_num)
{
  int    fwd_dir, rc;
  struct stop tapeinfo;

  if ( prule_info->num_blks >= 0 )              /* set direction pointer */
     fwd_dir = 1;
  else
     fwd_dir = 0;
  tapeinfo.st_op = STFSF;
  tapeinfo.st_count = prule_info->num_blks;
  rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
  if ( rc == -1 ) {
     phtx_info->bad_reads = phtx_info->bad_reads + 1;
     if ( errno == EIO ) {
        if ( fwd_dir )
           prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                        "End-of-media sensed during a forward "
                        "file skip operation.");
        else
           prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                        "End-of-media sensed during a backward "
                        "file skip operation.");
     } else if ( fwd_dir )
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                   "Forward file skip error - ");
     else
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                   "Backward file skip error - ");
     tape_error_code = errno;
  } else {
    pblk_num->in_file = 0;  /* zero block count pointers in search file marks */
    pblk_num->in_rule = 0;
  }
  return(rc);
}

/**************************************************************************/
/* search_rec() - This function will skip forward prule_info->num_blks    */
/* of records. If prule_info->num_blks is negative, then the skipping is  */
/* backwards.                                                             */
/**************************************************************************/
int
search_rec(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
           struct blk_num_typ *pblk_num)
{
  int  fwd_dir, rc;
  struct stop tapeinfo;

  if ( prule_info->num_blks >= 0 )                 /* set pointer direction */
     fwd_dir = 1;
  else
     fwd_dir = 0;
  tapeinfo.st_op = STFSR;
  tapeinfo.st_count = prule_info->num_blks;
  rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
  if ( rc == -1 ) {
     phtx_info->bad_reads = phtx_info->bad_reads + 1;
     if ( errno == EIO ) {
        if ( fwd_dir )
           prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                        "End-of-media sensed during a forward "
                        "record skip operation.");
        else
           prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                        "End-of-media sensed during a backward "
                        "record skip operation.");
     } else if ( fwd_dir )
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                "Forward record skip error - ");
     else
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                "Backward record skip error - ");
     tape_error_code = errno;
  } else {
     pblk_num->in_file = pblk_num->in_file + prule_info->num_blks;
     pblk_num->in_rule = pblk_num->in_rule + prule_info->num_blks;
  }
  return(rc);
}

/**************************************************************************/
/* diag_tape() - Do diagnostics.                                          */
/**************************************************************************/
int
diag_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
          struct blk_num_typ *pblk_num)
{
#ifndef __HTX_LINUX__
  int    i, rc;
  char   tmp_str[120];
  struct stop tapeinfo;

  rc = close(prule_info->fildes);        /* close and reopen in diagnostics */
  if ( /*rc == -1*/ 0 ) {
     strcpy(tmp_str, "Close error on ");
     strcat(tmp_str, phtx_info->sdev_id);
     strcat(tmp_str, " at start of Diag rule.\n");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     hxfmsg(phtx_info, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     prule_info->fildes = openx(phtx_info->sdev_id, Open_Mode, 0777,
                                SC_DIAGNOSTIC);
     if ( prule_info->fildes == -1 ) {
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                "Diagnostic Open Error - ");
        rc = -1;
        tape_error_code = errno;
     } else {
		if(found_fscsi == 1) {
			memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));   /* snd diag command */
			fiocmd_buf.timeout_value = 2400;           /* wait < 30 mins for diags */
			fiocmd_buf.flags = 0;                    /* use active dev driver mode */
			fiocmd_buf.command_length = 6;
			fiocmd_buf.scsi_cdb[0] = 0x1d;
			fiocmd_buf.scsi_cdb[1] = 7;
			rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
			if ( rc == -1 ) {
				phtx_info->bad_others = phtx_info->bad_others + 1;
				sprintf(msg_str,  "diagnostics error --\nstatus validity = %x   "
                             "scsi bus status = %x   adapter status = %x\n",
							 fiocmd_buf.status_validity, fiocmd_buf.scsi_bus_status,
							 fiocmd_buf.adapter_status);

				memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));     /* snd sns cmd */
				fiocmd_buf.data_length = sizeof(data_buffer);
				fiocmd_buf.buffer = data_buffer;
				fiocmd_buf.autosense_buffer_ptr = sense_buf;
				fiocmd_buf.autosense_length = sizeof(sense_buf);
				fiocmd_buf.timeout_value = 30;
				fiocmd_buf.flags = B_READ;
				fiocmd_buf.command_length = 6;
				fiocmd_buf.scsi_cdb[0] = 3;
				fiocmd_buf.scsi_cdb[4] = MAX_SENSE;
				rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
				if ( rc == -1 ) {
					phtx_info->bad_others = phtx_info->bad_others + 1;
					sprintf(msg_str,  "diagnostics request sense error --\n"
					"status validity = %x   scsi bus status"
					" = %x   adapter status = %x\n",
					fiocmd_buf.status_validity, fiocmd_buf.scsi_bus_status,
					fiocmd_buf.adapter_status);
					prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, msg_str);
					tape_error_code = errno;
				}
				else {
					strcpy(msg_str, "   Sense Data:");
					for ( i = 0; i < fiocmd_buf.autosense_length; i++ ) {
						if ( (i & 15) == 0 ) {
							sprintf(tmp_str, "\n    [%02X]", i);
							strcat(msg_str,tmp_str);
						}
						sprintf(tmp_str, "%02X", sense_buf[i]);
						strcat(msg_str, tmp_str);
					}
					prt_msg(phtx_info, prule_info, loop, pblk_num, errno, INFO,
							msg_str);
				}
			}

		}
		else {
			memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));   /* snd diag command */
			iocmd_buf.timeout_value = 2400;           /* wait < 30 mins for diags */
			iocmd_buf.flags = 0;                    /* use active dev driver mode */
			iocmd_buf.command_length = 6;
			iocmd_buf.scsi_cdb[0] = 0x1d;
			iocmd_buf.scsi_cdb[1] = 7;
			rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);
			if ( rc == -1 ) {
				phtx_info->bad_others = phtx_info->bad_others + 1;
				sprintf(msg_str,  "diagnostics error --\nstatus validity = %x   "
						"scsi bus status = %x   adapter status = %x\n",
						iocmd_buf.status_validity, iocmd_buf.scsi_bus_status,
						iocmd_buf.adapter_status);

				memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));     /* snd sns cmd */
				iocmd_buf.data_length = sizeof(sense_buf);
				iocmd_buf.buffer = sense_buf;
				iocmd_buf.timeout_value = 30;
				iocmd_buf.flags = B_READ;
				iocmd_buf.command_length = 6;
				iocmd_buf.scsi_cdb[0] = 3;
				iocmd_buf.scsi_cdb[4] = MAX_SENSE;
				rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);
				if ( rc == -1 ) {
					phtx_info->bad_others = phtx_info->bad_others + 1;
					sprintf(msg_str,  "diagnostics request sense error --\n"
							"status validity = %x   scsi bus status"
							" = %x   adapter status = %x\n",
							iocmd_buf.status_validity, iocmd_buf.scsi_bus_status,
							iocmd_buf.adapter_status);
					prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, msg_str);
					tape_error_code = errno;
				}
				else {
					strcpy(msg_str, "   Sense Data:");
					for ( i = 0; i < iocmd_buf.data_length; i++ ) {
						if ( (i & 15) == 0 ) {
							sprintf(tmp_str, "\n    [%02X]", i);
							strcat(msg_str,tmp_str);
						}
						sprintf(tmp_str, "%02X", sense_buf[i]);
						strcat(msg_str, tmp_str);
					}
					prt_msg(phtx_info, prule_info, loop, pblk_num, errno, INFO,
					msg_str);
				}
			}
		}

		rc = close(prule_info->fildes);
        if ( /*rc == -1 */ 0) {
           strcpy(tmp_str, "Close error on ");
           strcat(tmp_str, phtx_info->sdev_id);
           strcat(tmp_str, " at end of Diag rule.\n");
           strcat(tmp_str, strerror(errno));
           strcat(tmp_str,"\n");
           hxfmsg(phtx_info, errno, HARD, tmp_str);
           tape_error_code = errno;
        } else {
           prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
           if ( prule_info->fildes == -1 ) {
              prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                      "Open error - ");
              rc = -1;
              tape_error_code = errno;
           } else {
              rc = 0;
			  if(random_blksize) {
				if((rc=set_bs(phtx_info, prule_info, BLK_SIZE, 0))!= 0) {
					sprintf(tmp_str, "Could reset blksize to earlier value\n");
					hxfmsg(phtx_info, rc, HARD, tmp_str);
			  	}

	  		  }

		   }
        }
     }
  }
  return(rc);
#else
  int i,size=3600*HZ;
  int fd,rc;
  unsigned char cmdblk[6];
  struct sg_header *sg_hd;
  unsigned char outbuf[96 + SCSI_OFF];
  char tmp_str[1200];
  int out_size, cmd_len,in_size,status;
  char scsi_dev[20];

  tmp_str[0]= '\0';
  rc = close(prule_info->fildes);        /* close and reopen in diagnostics */
  if ( rc == -1 ) {
	strcpy(tmp_str, "Close error on ");
	strcat(tmp_str, phtx_info->sdev_id);
	strcat(tmp_str, " at start of Diag rule.\n");
	strcat(tmp_str, strerror(errno));
	strcat(tmp_str, "\n");
	hxfmsg(phtx_info, errno, HARD, tmp_str);
	tape_error_code = errno;
	return( -1);
  }

  if(get_scsi_devname(scsi_dev, phtx_info->sdev_id)== -1) {
	prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"SCSI dev Open Error - ");
	tape_error_code = ENODEV;
	return(-1);
  }
  out_size = sizeof( outbuf) - SCSI_OFF;
  cmd_len = 6;
  in_size = 0;

  fd = open(scsi_dev,O_RDWR);
  if( fd < 0) {
	prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"Diagnostic Open Error - ");
	tape_error_code = errno;
	return(-1);
  }
  if (ioctl(fd ,SG_SET_TIMEOUT ,&size) < 0)  {
	prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"Diagnostic Ioctl Error - ");
			tape_error_code = errno;
	return(-1);
  }

  cmdblk[0] = 0x1d;
  cmdblk[1] = 0x7;
  cmdblk[2] = 0;
  cmdblk[3] = 0;
  cmdblk[4] = 0;
  cmdblk[5] = 0;
  memcpy ( cmd + SCSI_OFF , cmdblk, sizeof(cmdblk));
  sg_hd = (struct sg_header *) cmd;
  sg_hd->reply_len = SCSI_OFF + ( SCSI_OFF + out_size); /* reply_len shud be 64 + SCSI_OFF */
  sg_hd->twelve_byte = cmd_len == 12;
  sg_hd->result = 0;

  status =  write( fd, cmd, SCSI_OFF + cmd_len + in_size );
  if ( status < 0 || status != ( SCSI_OFF + cmd_len + in_size) || sg_hd->result ) {
	prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"Diagnostic Write Error - ");
	tape_error_code = errno;
	return (-1);
  }

  status = read (fd , outbuf, SCSI_OFF + SCSI_OFF + out_size );
  if ( status < 0 || status != ( SCSI_OFF + SCSI_OFF + out_size) || sg_hd->result ) {
	phtx_info->bad_others = phtx_info->bad_others + 1;

	out_size = 64 - SCSI_OFF;
	cmd_len = 6;
	in_size = 0;

	cmdblk[0] = 0x3;
	cmdblk[1] = 0x0;
	cmdblk[2] = 0;
	cmdblk[3] = 0;
	cmdblk[4] = 0x40;
	cmdblk[5] = 0;
	memcpy ( cmd + SCSI_OFF , cmdblk, sizeof(cmdblk));

	sg_hd = (struct sg_header *) cmd;
	sg_hd->reply_len = SCSI_OFF + ( SCSI_OFF + out_size); /* reply_len shud be 64 + SCSI_OFF */
	sg_hd->twelve_byte = cmd_len == 6;
	sg_hd->result = 0;

	status =  write( fd, cmd, SCSI_OFF + cmd_len + in_size );
	if ( status < 0 || status != ( SCSI_OFF + cmd_len + in_size) || sg_hd->result ) 	{
		prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"Diagnostic Write Error - ");
		tape_error_code = errno;
		return (-1);
	}

	status = read (fd , outbuf, SCSI_OFF + ( SCSI_OFF + out_size));
	if ( status < 0 || status != ( SCSI_OFF + SCSI_OFF + out_size) || sg_hd->result ) {
		prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"Diagnostic Request sense Error - ");
		tape_error_code = errno;
		return (-1);
	}
	else {
		strcpy(msg_str,"\n        Sense data :");
		for ( i=0; i< out_size+SCSI_OFF ;i++) {
			if( (i&15) == 0) {
				sprintf(tmp_str,"\n   [%02X]", i);
				strcat( msg_str,tmp_str);
			}
			sprintf(tmp_str,"%02X",outbuf[SCSI_OFF + i]);
			strcat(msg_str, tmp_str);
		}
		prt_msg(phtx_info, prule_info, loop, pblk_num, errno, INFO, msg_str);

	}

  }

  close(fd);
  prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
  return(0);
#endif
}

/**************************************************************************/
/* erase_tape() - Erase the tape.                                         */
/**************************************************************************/
int
erase_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
           struct blk_num_typ *pblk_num)
{
  int    rc;
  struct stop tapeinfo;

  tapeinfo.st_op = STERASE;
  tapeinfo.st_count = 1;
  rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
  if ( rc == -1 ) {
     phtx_info->bad_others = phtx_info->bad_others + 1;
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
             "Erase tape error - ");
     tape_error_code = errno;
  }
  return(rc);
}

/**************************************************************************/
/* do_sleep() - Sleep.                                                    */
/**************************************************************************/
int
do_sleep(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
         struct blk_num_typ *pblk_num)
{
  int    rc;
  time_t t1;

  time(&t1);
  while ( (time(NULL) - t1) < prule_info->num_blks ) ;
  rc = 0;
  return(rc);
}

/**************************************************************************/
/* close_open() - Close and reopen device.                                */
/**************************************************************************/
int
close_open(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
           struct blk_num_typ *pblk_num)
{
  int     rc, attempt;
  char    tmp_str[4*80];       /* Allow up to 4 lines at 80 char/line */
  #define NUM_ATTEMPTS 3

  rc = 0;
  for ( attempt = 1; attempt <= NUM_ATTEMPTS; attempt++)
    if ( (rc = close(prule_info->fildes)) ) {
	  if (errno == EBADF || errno == EIO)
        rc = 0;
      else if(0){
        sprintf(tmp_str, "Close error in Close/Open "
                         "rule on attempt %d of %d:\n",
                attempt, NUM_ATTEMPTS);
        tape_error_code = errno;
        phtx_info->bad_others++;
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        sleep(1);
      }
	  else
		break;
	}
    else
      break;
  if ( rc )
    return(rc);
  for ( attempt = 1; attempt <= NUM_ATTEMPTS; attempt++)
    if ( (rc = prule_info->fildes =
               open(phtx_info->sdev_id, Open_Mode)) == -1) {
      sprintf(tmp_str, "Open error in Close/Open rule on attempt %d of %d:\n",
              attempt, NUM_ATTEMPTS);
      tape_error_code = errno;
      phtx_info->bad_others++;
      prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
      sleep(1);
    } else {
      rc = 0;
/*#ifndef __HTX_LINUX__*/
	  if(random_blksize) {
		if((rc=set_bs(phtx_info, prule_info, BLK_SIZE, 0))!= 0) {
			sprintf(tmp_str, "Could reset blksize to earlier value\n");
			hxfmsg(phtx_info, rc, HARD, tmp_str);
		}

	  }
/*#endif*/
      break;
    }
  return(rc);
}

/**************************************************************************/
/* tclose() - Close of tape device.                                       */
/**************************************************************************/
int
tclose(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
       struct blk_num_typ *pblk_num)
{
  int     rc, attempt;
  char    tmp_str[4*80];
  #define NUM_ATTEMPTS 3

  for (attempt = 1; attempt <= NUM_ATTEMPTS; attempt++)
    if (( rc = close(prule_info->fildes)) ) {
      if (errno == EBADF)
        rc = 0;
      else if(0){
        sprintf(tmp_str, "Close error on attempt %d of %d:\n",
                attempt, NUM_ATTEMPTS);
        tape_error_code = errno;
        phtx_info->bad_others++;
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        sleep(1);
      }
  	  else
		break;
	}
    else
      break;
  return(rc);
}

/**************************************************************************/
/* topen() - Open of tape device.                                         */
/**************************************************************************/
int
topen(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
      struct blk_num_typ *pblk_num)
{
  int  rc;
/*#ifndef __HTX_LINUX__*/
  char tmp_str[80];
/*#endif*/

  rc = 0;
  prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
  if ( prule_info->fildes == -1 ) {
#ifndef __HTX_LINUX__
     if ( errno == ENOTREADY ) {
        prule_info->fildes = openx(phtx_info->sdev_id, Open_Mode, NULL,
                                   SC_FORCED_OPEN);
        if ( prule_info->fildes == -1 ) {
           rc = -1;
           phtx_info->bad_others = phtx_info->bad_others + 1;
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                   "reopen error - ");
           tape_error_code = errno;
        }
     } else {
        rc = -1;
        phtx_info->bad_others = phtx_info->bad_others + 1;
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                "reopen error - ");
        tape_error_code = errno;
     }
#else
	 rc = -1;
	 phtx_info->bad_others = phtx_info->bad_others + 1;
	 prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
					 "reopen error - ");
	 tape_error_code = errno;
#endif
  }
/*#ifndef __HTX_LINUX__*/
  else {

  	if(random_blksize) {
		if((rc=set_bs(phtx_info, prule_info, BLK_SIZE, 0))!= 0) {
			sprintf(tmp_str, "Could reset blksize to earlier value\n");
			hxfmsg(phtx_info, rc, HARD, tmp_str);
		}

	}
  }
/*#endif*/
  return(rc);
}

/**************************************************************************/
/* write_eot() - Write to end of tape.                                    */
/**************************************************************************/
int
write_eot(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
          struct blk_num_typ *pblk_num, char *wbuf)
{
  int    rc;
  struct stop tapeinfo;

  while ( (rc = write(prule_info->fildes, wbuf,
                      (prule_info->num_blks * BLK_SIZE))) != -1 ) {
     lseek(prule_info->fildes, 0, 1);
     bytes_wrote = rc;
     phtx_info->good_writes = phtx_info->good_writes + 1;
     phtx_info->bytes_writ = phtx_info->bytes_writ + bytes_wrote;
     update_blk_nums(pblk_num, bytes_wrote);
     hxfupdate(UPDATE,phtx_info);
  }
  if ( errno != ENXIO
#ifdef __HTX_LINUX__
		&& errno != EIO
#endif
	 ) {
     phtx_info->bad_writes = phtx_info->bad_writes + 1;
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
             "Write to end-of-tape error - ");
     tape_error_code = errno;
  } else if ( (errno == EBADF) && (Open_Mode == O_RDONLY) ) {
     prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                  "Write error due to Write-Protected media.");
     tape_error_code = errno;
  } else {
     tapeinfo.st_op = STWEOF;          /* write an EOF mark at end of tape */
     tapeinfo.st_count = 1;
     rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
     if ( rc == -1 ) {
        phtx_info->bad_writes = phtx_info->bad_writes + 1;
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                "Write EOF err after WEOT - ");
        tape_error_code = errno;
     }
  }
  return(rc);
}

/**************************************************************************/
/* read_eot() - Read / Compare to end of tape.                            */
/**************************************************************************/
int
read_eot(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
         struct blk_num_typ *pblk_num, char *wbuf, char *rbuf)
{
  int  rc, save;

  memset(rbuf, 0xbb, (prule_info->num_blks * BLK_SIZE));
  while ( (rc = read(prule_info->fildes, rbuf,
                     (prule_info->num_blks * BLK_SIZE))) > 0 ) {
     lseek(prule_info->fildes, 0, 1);
     bytes_read = rc;
     phtx_info->good_reads = phtx_info->good_reads + 1;
     phtx_info->bytes_read = phtx_info->bytes_read + bytes_read;
     update_blk_nums(pblk_num, bytes_read);
     hxfupdate(UPDATE, phtx_info);
     if ( bytes_read == (prule_info->num_blks * BLK_SIZE) )
        rc = cmpbuf(phtx_info, prule_info, loop, pblk_num, wbuf, rbuf);
     else {
        save = prule_info->num_blks;
        prule_info->num_blks = bytes_read / BLK_SIZE;
        rc = cmpbuf(phtx_info, prule_info, loop, pblk_num, wbuf, rbuf);
        prule_info->num_blks = save;
     }
     memset(rbuf, 0xbb, (prule_info->num_blks * BLK_SIZE));
     if ( rc != 0 )
        break;
  }
  if ( rc < 0 ) {
     phtx_info->bad_reads = phtx_info->bad_reads + 1;
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
             "read compare to end-of-tape error - ");
     tape_error_code = errno;
  } else
     rc = 0;
  return(rc);
}

/**************************************************************************/
/* read_eot() - Read to end of tape.                                      */
/**************************************************************************/
int
read_teot(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
         struct blk_num_typ *pblk_num, char *wbuf, char *rbuf)
{
  int  rc;

  memset(rbuf, 0xbb, (prule_info->num_blks * BLK_SIZE));
  while ( (rc = read(prule_info->fildes, rbuf,
                     (prule_info->num_blks * BLK_SIZE))) > 0 ) {
     lseek(prule_info->fildes, 0, 1);
     bytes_read = rc;
     phtx_info->good_reads = phtx_info->good_reads + 1;
     phtx_info->bytes_read = phtx_info->bytes_read + bytes_read;
     update_blk_nums(pblk_num, bytes_read);
     hxfupdate(UPDATE, phtx_info);
     memset(rbuf, 0xbb, (prule_info->num_blks * BLK_SIZE));
  }
  if ( rc < 0 ) {
     phtx_info->bad_reads = phtx_info->bad_reads + 1;
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
             "read to end-of-tape error - ");
     tape_error_code = errno;
  } else
     rc = 0;
  return(rc);
}

/*************************************************************************/
/* prt_req_sense() - Print the request sense data.                       */
/**************************************************************************/
int
prt_req_sense(struct htx_data *phtx_info, struct ruleinfo * prule_info,
              int loop, struct blk_num_typ *pblk_num)
{
#ifndef __HTX_LINUX__

  int    i, rc;
  char   tmp_str[120];
  struct stop tapeinfo;

  rc = close(prule_info->fildes);     /* close and reopen in diagnostic mode */
  if ( /*rc == -1*/ 0) {
     strcpy(tmp_str, "Close error on ");
     strcat(tmp_str, phtx_info->sdev_id);
     strcat(tmp_str, " at start of Request Sense rule.\n");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str,"\n");
     hxfmsg(phtx_info, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     prule_info->fildes = openx(phtx_info->sdev_id, Open_Mode, 0777,
                                SC_DIAGNOSTIC);
     if ( prule_info->fildes == -1 ) {
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                "Diagnostic Open Error - ");
        rc = -1;
        tape_error_code = errno;
     }
	 else {
		if(found_fscsi == 1) {
			printf("Issuing request sense to FCS drive\n");

#if 0
			memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));     /* snd sns cmd */
			fiocmd_buf.data_length =0xff ;
			fiocmd_buf.buffer = data_buffer;
			fiocmd_buf.autosense_buffer_ptr = sense_buf;
			fiocmd_buf.autosense_length = MAX_SENSE;
			fiocmd_buf.timeout_value = 30;
			fiocmd_buf.flags = B_READ;
			fiocmd_buf.command_length = 6;
			fiocmd_buf.scsi_cdb[0] = 3;
			fiocmd_buf.scsi_cdb[4] = 0xff;
			rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);

			if ( rc == -1 ) {
				phtx_info->bad_others = phtx_info->bad_others + 1;
				sprintf(msg_str, "Request Sense command error -\nstatus validity "
							"= %x   scsi bus status = %x  adapter status = %x",
							fiocmd_buf.status_validity, fiocmd_buf.scsi_bus_status,
							fiocmd_buf.adapter_status );
				prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, msg_str);
				tape_error_code = errno;
			}
			else {
				strcpy(msg_str, "   Sense Data:");
				for ( i = 0; i < fiocmd_buf.autosense_length; i++ ) {
					if ( (i & 15) == 0 ) {
						sprintf(tmp_str, "\n    [%02X]", i);
						strcat(msg_str, tmp_str);
					}
					sprintf(tmp_str, "%02X", sense_buf[i]);
					strcat(msg_str, tmp_str);
				}
			}
#endif
			/* Support for STIOCMD has been removed, have to use SIOC_PASSTHRU_COMMAND to FCS drives */
			memset(&scsi_passthru_cmd_buf, 0, sizeof(struct scsi_passthru_cmd));
			scsi_passthru_cmd_buf.command_length = 16;
			scsi_passthru_cmd_buf.scsi_cdb[0] = SCSI_REQUEST_SENSE;
			scsi_passthru_cmd_buf.scsi_cdb[4] = 0xff;
			scsi_passthru_cmd_buf.timeout_value = 200;
			scsi_passthru_cmd_buf.buffer = pass_thru_buff;
			scsi_passthru_cmd_buf.buffer_length = MAX_PASS_THRU_BUFF_LEN;
			scsi_passthru_cmd_buf.trace_length = MAX_PASS_THRU_TRACE_LEN;
			rc = ioctl(prule_info->fildes, SIOC_PASSTHRU_COMMAND, &scsi_passthru_cmd_buf);

			if ( rc == -1 ) {
                phtx_info->bad_others = phtx_info->bad_others + 1;
                sprintf(msg_str, "Request Sense command error while issuing SIOC_PASSTHRU_COMMAND to FC drive-");
                prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, msg_str);
                tape_error_code = errno;
            }
            else {
				/*
 				printf("  Errno....................... %d\n",errno);
  				printf("  Sense length ............... %d\n",scsi_passthru_cmd_buf.sense_length);
				printf("  Timeout value .............. %d\n",scsi_passthru_cmd_buf.timeout_value);
  				printf("  Buffer length .............. %d\n",scsi_passthru_cmd_buf.buffer_length);
  				printf("  Number bytes transferred.... %d\n",scsi_passthru_cmd_buf.number_bytes);
				*/

                strcpy(msg_str, "   Sense Data:");
                for ( i = 0; i < scsi_passthru_cmd_buf.buffer_length; i++ ) {
                    if ( (i & 15) == 0 ) {
                        sprintf(tmp_str, "\n    [%02X]", i);
                        strcat(msg_str, tmp_str);
                    }
                    sprintf(tmp_str, "%02X", pass_thru_buff[i]);
                    strcat(msg_str, tmp_str);
                }
            }


		}
		else {

			printf("Issuing request sense to non-FCS drive\n");
			memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));
			iocmd_buf.data_length = 0xff;
			iocmd_buf.buffer = sense_buf;
			iocmd_buf.timeout_value = 30;
			iocmd_buf.flags = B_READ;
			iocmd_buf.command_length = 6;
			iocmd_buf.scsi_cdb[0] = 3;
			iocmd_buf.scsi_cdb[4] = 0xff;
			rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);
			if ( rc == -1 ) {
				phtx_info->bad_others = phtx_info->bad_others + 1;
				sprintf(msg_str, "Request Sense command error -\nstatus validity "
							"= %x   scsi bus status = %x  adapter status = %x",
							iocmd_buf.status_validity, iocmd_buf.scsi_bus_status,
							iocmd_buf.adapter_status );
				prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, msg_str);
				tape_error_code = errno;
			}
			else {
				strcpy(msg_str, "   Sense Data:");
				for ( i = 0; i < iocmd_buf.data_length; i++ ) {
					if ( (i & 15) == 0 ) {
						sprintf(tmp_str, "\n    [%02X]", i);
						strcat(msg_str, tmp_str);
					}
					sprintf(tmp_str, "%02X", sense_buf[i]);
					strcat(msg_str, tmp_str);
				}
			}
		}
		prt_msg(phtx_info, prule_info, loop, pblk_num, 0, INFO, msg_str);
		rc = close(prule_info->fildes);
		if ( /*rc == -1*/ 0) { /*check for errno, bug in driver which incorrectly returns -1*/
			strcpy(tmp_str, "Close error on ");
			strcat(tmp_str, phtx_info->sdev_id);
			strcat(tmp_str, " at end of Request Sense rule.\n");
			strcat(tmp_str, strerror(errno));
			strcat(tmp_str,"\n");
			hxfmsg(phtx_info, errno, HARD, tmp_str);
			tape_error_code = errno;
		} else {
			prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
			if ( prule_info->fildes == -1 ) {
				prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
							"Open error - ");
				rc = -1;
				tape_error_code = errno;
			} else {
				rc = 0;
				if(random_blksize) {
					if((rc=set_bs(phtx_info, prule_info, BLK_SIZE, 0))!= 0) {
						sprintf(tmp_str, "Could reset blksize to earlier value\n");
						hxfmsg(phtx_info, rc, HARD, tmp_str);
					}

	  			}
			  }
			}
		}
	}
	return(rc);
#else
	int i,size=3600*HZ;
	int fd,rc;
	unsigned char cmdblk[6];
	struct sg_header *sg_hd;
	unsigned char outbuf[96 + SCSI_OFF];
	char tmp_str[1200];
	int out_size, cmd_len,in_size,status;
	char scsi_dev[20];

	tmp_str[0] ='\0';
   	rc = close(prule_info->fildes);        /* close and reopen in diagnostics */
   	if ( rc == -1 ) {
		strcpy(tmp_str, "Close error on ");
		strcat(tmp_str, phtx_info->sdev_id);
		strcat(tmp_str, " at start of Diag rule.\n");
		strcat(tmp_str, strerror(errno));
		strcat(tmp_str, "\n");
		hxfmsg(phtx_info, errno, HARD, tmp_str);
		tape_error_code = errno;
		return( -1);
	}

	if(get_scsi_devname(scsi_dev, phtx_info->sdev_id)) {
		strcpy(tmp_str, "Unable to map tape device=>");
		strcat(tmp_str, phtx_info->sdev_id);
		strcat(tmp_str, "  to correct generic scsi device\n");
		hxfmsg(phtx_info, errno, HARD, tmp_str);
		tape_error_code = ENXIO;
		return(-1);
	}

	fd = open(scsi_dev,O_RDWR);
	if( fd < 0)	{
		prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"Diagnostic Open Error - ");
		tape_error_code = errno;
		return(-1);
	}
	if (ioctl(fd ,SG_SET_TIMEOUT ,&size) < 0) {
		prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"Diagnostic Ioctl Error - ");
		tape_error_code = errno;
		return(-1);
	}

	out_size = 64 - SCSI_OFF;
	cmd_len = 6;
	in_size = 0;

	cmdblk[0] = 0x3;
	cmdblk[1] = 0x0;
	cmdblk[2] = 0;
	cmdblk[3] = 0;
	cmdblk[4] = 0x40;
	cmdblk[5] = 0;
	memcpy ( cmd + SCSI_OFF , cmdblk, sizeof(cmdblk));

	sg_hd = (struct sg_header *) cmd;

	sg_hd->reply_len = SCSI_OFF + ( SCSI_OFF + out_size); /* reply_len shud be 64 + SCSI_OFF */
	sg_hd->twelve_byte = cmd_len == 6;
	sg_hd->result = 0;

	status =  write( fd, cmd, SCSI_OFF + cmd_len + in_size );
	if ( status < 0 || status != ( SCSI_OFF + cmd_len + in_size) || sg_hd->result )	{
		prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"Diagnostic Write Error - ");
		tape_error_code = errno;
		return (-1);
	}

	status = read (fd , outbuf, SCSI_OFF + ( SCSI_OFF + out_size));
	if ( status < 0 || status != ( SCSI_OFF + SCSI_OFF + out_size) || sg_hd->result ) 	{
		prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,"Diagnostic Request sense Error - ");
		tape_error_code = errno;
		return (-1);
	}
	else 	{
		strcpy(msg_str,"\n        Sense data :");
		for ( i=0; i< out_size+SCSI_OFF ;i++) {
			if( (i&15) == 0) {
				sprintf(tmp_str,"\n   [%02X]", i);
				strcat( msg_str,tmp_str);
			}
			sprintf(tmp_str,"%02X",outbuf[SCSI_OFF + i]);
			strcat(msg_str, tmp_str);
		}
		prt_msg(phtx_info, prule_info, loop, pblk_num, errno, INFO, msg_str);

	}
	close(fd);
	prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);

  	return(0);
#endif
}

/**************************************************************************/
/* get_dd_blksize() - This function will acquire the current device       */
/* information blocksize for the active device.                           */
/**************************************************************************/
int
get_dd_blksize(char *sdev_id, char *dev_type, struct htx_data *phtx_info,
                   struct ruleinfo *prule_info, struct blk_num_typ *pblk_num)
{
#ifndef __HTX_LINUX__
  int    fildes, lcl_dd_blksiz, use_default_blksiz, rc;
  char   msg[220], tmp_str[120];
  struct devinfo dev_info;
  struct stop tapeinfo;

#ifdef __DEBUG__
  sprintf(msg, "get_dd_blksize::Entering\n");
  hxfmsg(phtx_info, 0, INFO, msg);
#endif
  if(buffer_initialized == 0) {
	printf("initialising buffer\n");
		sense_buf = (char*)malloc(SENSE_BUF_SIZE);
		data_buffer = (char*)malloc(SENSE_BUF_SIZE);
		if(sense_buf == NULL || data_buffer == NULL) {
			sprintf(msg, "malloc failed for buffers\n");
			hxfmsg(phtx_info, 0, INFO, msg);
			return -1;
		}
		buffer_initialized = 1;
  }

  use_default_blksiz = 1;                     /* init for default blksiz */
  lcl_dd_blksiz = BLKSIZ;              /* init to use default block size */
  fildes = openx(sdev_id, O_RDONLY, 0777, SC_DIAGNOSTIC);
  if ( fildes == -1 && errno == EAGAIN )
     fildes = openx(sdev_id, O_RDONLY, 0777, SC_DIAGNOSTIC);
  if ( fildes == -1 ) {
     use_default_blksiz = 1;         /* if any error then use default blksiz */
     sprintf(msg, "Open on device %s failed when trying to acquire Blocksize.\n"
             ,sdev_id);
     sprintf(tmp_str, " System error number value : (%d) %s.\n",
                errno, strerror(errno));
     strcat(msg,tmp_str);
     hxfmsg(phtx_info, 0, HARD, msg);
  } else {
     dev_info.un.scmt.blksize = 0;
     rc = ioctl(fildes, IOCINFO, &dev_info);

#ifdef __DEBUG__
	sprintf(msg, "get_dd_blksize::IOCINFO ioctl returned:%d blksize:%d devtype:%c\n",
					rc, dev_info.un.scmt.blksize, dev_info.devtype);
	hxfmsg(phtx_info, 0, INFO, msg);
#endif


     if ( rc == -1 )
        use_default_blksiz = 1;    /* if any error then use default blksiz */
     else
        use_default_blksiz = 0;                 /* no error, use dd blksiz */
     rc = close(fildes);
     if ( /*rc == -1*/ 0 ) {	/*spurios error ?????*/
        strcpy(tmp_str, "Close error on ");
        strcat(tmp_str, sdev_id);
        strcat(tmp_str, " at end of get blocksize function.\n");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str,"\n");
        hxfmsg(phtx_info, errno, HARD, tmp_str);
     }
     if ( use_default_blksiz )   /* assure actual or default blksize returned */
        lcl_dd_blksiz = BLKSIZ;     /*   use default block size if any errors */
     else
        lcl_dd_blksiz = dev_info.un.scmt.blksize;  /* use returned block size */
     switch ( dev_info.devtype ) {       /* determine device type information */
       case DD_SCTAPE : {                         /* set device type string */
            strcpy(dev_type, " SCSI Tape Unit.");
            break;
       }
       default : {               /* force default blksize on non-scsi tapes */
            lcl_dd_blksiz = BLKSIZ;
            strcpy(dev_type, " NON-SCSI Tape Unit.");
            break;
       }
     }
  }
#ifdef __DEBUG__
  sprintf(msg, "get_dd_blksize::Exiting\n");
  hxfmsg(phtx_info, 0, INFO, msg);
#endif

  return(lcl_dd_blksiz);
#else /*__HTX_LINUX__*/
  int rc,hdl;
  struct mtget tmp;
  char tmp_str[500];

  hdl =  open( sdev_id, O_RDONLY);
  if( hdl <0 )
  {

        strcpy(tmp_str, "open error on ");
        strcat(tmp_str, sdev_id);
        strcat(tmp_str, " at end of get blocksize function.\n");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str,"\n");
        hxfmsg(phtx_info, errno, HARD, tmp_str);
  }

  rc = ioctl( hdl, MTIOCGET, &tmp);
  if(rc < 0)
  {
	  tmp.mt_dsreg = BLKSIZ;

  }

  rc = close(hdl);
  if ( rc <0)
  {
        strcpy(tmp_str, "Close error on ");
        strcat(tmp_str, sdev_id);
        strcat(tmp_str, " at end of get blocksize function.\n");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str,"\n");
        hxfmsg(phtx_info, errno, HARD, tmp_str);
  }

  tmp.mt_dsreg = tmp.mt_dsreg & 0xffffff;
  return(tmp.mt_dsreg);
#endif
}
#ifndef __HTX_LINUX__
/**************************************************************************/
/* set_dbug -- set debug monitor mode in 4mm4gb devices                   */
/**************************************************************************/
int
set_dbug(struct htx_data *phtx_info, struct ruleinfo *prule_info,
         struct blk_num_typ *pblk_num)
{
  struct stop tapeinfo;
  int    i, rc;
  char   tmp_str[120];

  rc = close(prule_info->fildes);         /* close and reopen in diagnostics */
  if ( /*rc == -1*/ 0 ) {
     strcpy(tmp_str, "Close error on ");
     strcat(tmp_str, phtx_info->sdev_id);
     strcat(tmp_str, " at Set DBUG operation.\n");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     hxfmsg(phtx_info, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     prule_info->fildes = openx(phtx_info->sdev_id, Open_Mode, 0,
                                SC_DIAGNOSTIC);
     if ( prule_info->fildes == -1 ) {
        prt_msg(phtx_info, prule_info, 0, pblk_num, errno, HARD,
                "DBUG Open error - ");
        rc = -1;
        tape_error_code = errno;
     } else {
		if(found_fscsi == 1) {
			memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));
			fiocmd_buf.timeout_value = 30;
			fiocmd_buf.command_length = 10;
			fiocmd_buf.scsi_cdb[0] = 0xee;
			fiocmd_buf.scsi_cdb[1] = 0x02;
			rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
			if ( rc == -1 ) {
				phtx_info->bad_others = phtx_info->bad_others + 1;
				sprintf(msg_str, "Set Debug command error -\nstatus validity = %x"
								"   scsi bus status = %x   adapter status = %x",
								fiocmd_buf.status_validity, fiocmd_buf.scsi_bus_status,
								fiocmd_buf.adapter_status );
				prt_msg(phtx_info, prule_info, 0, pblk_num, errno, HARD, msg_str);
				tape_error_code = errno;
				return(rc);
			}
		}
		else {
			memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));
			iocmd_buf.timeout_value = 30;
			iocmd_buf.command_length = 10;
			iocmd_buf.scsi_cdb[0] = 0xee;
			iocmd_buf.scsi_cdb[1] = 0x02;
			rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);
			if ( rc == -1 ) {
				phtx_info->bad_others = phtx_info->bad_others + 1;
				sprintf(msg_str, "Set Debug command error -\nstatus validity = %x"
								"   scsi bus status = %x   adapter status = %x",
								iocmd_buf.status_validity, iocmd_buf.scsi_bus_status,
								iocmd_buf.adapter_status );
				prt_msg(phtx_info, prule_info, 0, pblk_num, errno, HARD, msg_str);
				tape_error_code = errno;
				return(rc);
			}
		}
        rc = close(prule_info->fildes);
        if ( /*rc == -1*/ 0 ) {
        	strcpy(tmp_str, "Close error on ");
	        strcat(tmp_str, phtx_info->sdev_id);
                strcat(tmp_str, " at Set DBUG rule.\n");
                strcat(tmp_str, strerror(errno));
    	        strcat(tmp_str, "\n");
                hxfmsg(phtx_info, errno, HARD, tmp_str);
                tape_error_code = errno;
	         }
			else {
            	prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
	            if ( prule_info->fildes == -1 ) {
    	             prt_msg(phtx_info, prule_info, 0, pblk_num, errno, HARD,
        	                "Open error - ");
            	     rc = -1;
                	 tape_error_code = errno;
	            }
				else {
        	         rc = 0;
					 if(random_blksize) {
						if((rc=set_bs(phtx_info, prule_info, BLK_SIZE, 0))!= 0) {
							sprintf(tmp_str, "Could reset blksize to earlier value\n");
							hxfmsg(phtx_info, rc, HARD, tmp_str);
						}

	  				 }
				}
           	}
       }
  }
  return(rc);
}
#endif
/**************************************************************************/
/* set_bs -- reset the tape blocksize                                     */
/**************************************************************************/
int
set_bs(struct htx_data *pHTX, struct ruleinfo *pRule, int size, int marker)
{
  char   sWork[512];
  int    rc;

#ifndef __HTX_LINUX__
  struct stchgp stchgp;

  stchgp.st_ecc = ST_ECC;
  stchgp.st_blksize = size;
  if ( (rc = ioctl(pRule->fildes, STIOCHGP, &stchgp)) == -1 ) {
     sprintf(sWork, "VBS: Error %d (%s) changing block size to %d (marker %d)",
             errno, strerror(errno), size, marker);
     hxfmsg(pHTX, errno, HARD, sWork);
     tape_error_code = errno;
  }
#else

  struct mtop tmp;
  tmp.mt_op = MTSETBLK ;
  tmp.mt_count = size ;
  sWork[0]= '\0';
  if ( (rc = ioctl(pRule->fildes, MTIOCTOP, &tmp)) == -1 ) {
     sprintf(sWork, "VBS: Error %d (%s) changing block size to %d (marker %d)",
             errno, strerror(errno), size, marker);
     hxfmsg(pHTX, errno, HARD, sWork);
     tape_error_code = errno;
  }
#endif
#ifdef __DEBUG__
  else
		sprintf(sWork,"set_bs: ioctl returned :%d and size specified was:%d\n",
					rc,size);
	    hxfmsg(pHTX, 0, INFO, sWork);

#endif
  return(rc);
}

/**************************************************************************/
/* VBS_Write -- variable block size write test                            */
/**************************************************************************/
int
VBS_Write(struct htx_data *pHTX, struct ruleinfo *pRule,
              struct blk_num_typ *pblk_num)
{
  int            i, rc, loop_count, max_loops;
  int            max_xfer, found_eot, num_bytes;
  char           sWork[1024], *Wbuf;
  unsigned short seed[3];
  struct         stop tapeinfo;

	/* for buster pattern support */
	int bus_width = 0, bufrem = 0;

  if ( Open_Mode != O_RDWR ) {
     /* hxfmsg(pHTX, 0, SOFT, "VBS: RD/WRITE mode required for VBS write test..."); */
     hxfmsg(pHTX, 0, HARD, "VBS: The Media could be write-protected. RD/WRITE mode required for VBS write test...");
     tape_error_code = errno;
     return(-1);
  }
  if ( !VBS_seed_initialized ) {
     hxfmsg(pHTX, 0, SYSERR, "VBS write: operation called but seed not\n"
                             "initialized in this or preceding stanza");
     tape_error_code = errno;
     return(-1);
  }
                             /* Set variable block size mode (block_size = 0) */
  if ( set_bs(pHTX, pRule, 0, 1) ) {
     tape_error_code = errno;
     return(-1);
  }
                                       /* Copy the initial random number seed */
  for ( i = 0; i < 3; i++ )
      seed[i] = pRule->seed[i];
     /* Set the maximum length of a transfer to the buffer size if not spec'd */
  if ( pRule->num_blks == 1 ) /* this means num_blks was not specified in the rules */
     max_xfer = 1024 * 1024;
  else
     max_xfer = pRule->num_blks;

  #ifdef __HTX_LINUX__
	  /* adjust as per blk limits */
	  if( max_xfer < l_blk_limit ) max_xfer = l_blk_limit;

	  if( max_xfer > m_blk_limit ) max_xfer = m_blk_limit;
  #endif

  loop_count = 1;
  VBS_last_write = -1;
  found_eot = 0;

  if ( pRule->VBS_seed_type != 'F' )
     sprintf(sWork, "Starting VBS write rule.  Operation count=%d; pattern %s\n"
                    "Initial seed values: %d,%d,%d; max_xfer = %d\n",
             pRule->num_oper, pRule->pattern_id,
             seed[0], seed[1], seed[2], max_xfer);
  else
     sprintf(sWork, "Starting VBS write rule.  Operation count=%d; pattern %s\n"
                    "max_xfer = %d\n",
             pRule->num_oper, pRule->pattern_id, max_xfer);
  hxfmsg(pHTX, 0, INFO, sWork);
       /* Next line sets max_loops to maxint if num_oper spec'd to 0 in rule */
  max_loops = (pRule->num_oper==0) ? ((unsigned)(-1) >> 1) : pRule->num_oper;
                                                /* Set up the pattern buffer */

  /* support for buster pattern */
  if( strncmp( pRule->pattern_id, "BSTR", 4 ) == 0 ) {
      sprintf(sWork,"Buster pattern detected in current stanza\n");
	  hxfmsg(pHTX, 0, INFO, sWork);

	  bus_width = get_bus_width( pRule->pattern_id );
	  sprintf(sWork,"Buster pattern width deduced = %d\n", bus_width);
	  hxfmsg(pHTX, 0, INFO, sWork);

  }

  /*Wbuf = (char *) malloc(max_xfer);*/
  Wbuf = (char *) malloc(max_xfer + bus_width);

  if ( Wbuf == NULL ) {
     sprintf(sWork, "Malloc error in VBS Write Stanza\n");
     hxfmsg(pHTX, errno, SYSERR, sWork);
     tape_error_code = errno;
     return(-1);
  }

  if( bus_width ) {
    sprintf(sWork,"Buster pattern malloc'ed Wbuf = %llx\n", (unsigned long long)Wbuf);
    hxfmsg(pHTX, 0, INFO, sWork);

	bufrem = (int)((unsigned long long) Wbuf  % bus_width);
	if( bufrem != 0 ) {
		Wbuf = Wbuf + ( bus_width - bufrem );
	}

	sprintf(sWork,"Buster pattern aligned Wbuf = %llx\n", (unsigned long long)Wbuf);
	hxfmsg(pHTX, 0, INFO, sWork);
  }

  if ( (pRule->pattern_id[0] != '#') && (pRule->pattern_id[0] != '\0') ) {
	 sWork[0] = '\0';
	 htx_strcpy(sWork, getenv("HTXPATTERNS"));
	 if ( htx_strlen(sWork) == 0 )
        htx_strcpy(sWork, "../pattern/");                   /* For default ONLY */
     htx_strcat (sWork, pRule->pattern_id);
  }
           /* Initialize the write buffer if a pattern file was specified. */
  if ( pRule->pattern_id[0] != '#' ) {
     if ( (rc = hxfpat(sWork, Wbuf, max_xfer)) == 1 ) {
        sprintf(sWork, "Cannot open pattern file - %s\n", pRule->pattern_id);
        hxfmsg(pHTX, 0, SYSERR, sWork);
        tape_error_code = errno;
        return(-1);
     } else if ( rc == 2 ) {
        sprintf(sWork, "Cannot read pattern file - %s\n", pRule->pattern_id);
        hxfmsg(pHTX, 0, SYSERR, sWork);
        tape_error_code = errno;
        return(-1);
     }
  }
  else
     bldbuf((unsigned short*)Wbuf, max_xfer, pRule->pattern_id, pblk_num);

                                                    /* Start the write loop */
  while ( (loop_count <= max_loops) && !found_eot ) {
            /* Generate the length of data to write on this loop */
     if ( pRule->VBS_seed_type != 'F' ) {
        while ( (num_bytes = (int) nrand48(seed) % max_xfer) == 0 ) ;

        #ifdef __HTX_LINUX__
			/* Take into account the blk limits */
			if( num_bytes < l_blk_limit ) num_bytes += l_blk_limit;
        #endif

	} else
        num_bytes = max_xfer;
     if ( num_bytes < 4 )
        num_bytes = 5;
                                                     /* Write the buffer out */
    if ( (rc = write(pRule->fildes, Wbuf, num_bytes)) != -1 ) {
       bytes_wrote = rc;
       rc = 0;
       pHTX->good_writes++;
       pHTX->bytes_writ += bytes_wrote;
       total_bytes += (double)bytes_wrote;
       if ( bytes_wrote != num_bytes )
          VBS_last_write = bytes_wrote;
       loop_count++;
#ifdef _DEBUG_LINUX_
	   if((loop_count % 256) == 0) {
			printf("\e[1AVBS_Write====> in loop << \e[31m\e[1m0x%.8x\e[0m >>\n",(unsigned int)loop_count);
	   }
#endif
       update_blk_nums(pblk_num,1);    /* update number of blocks pointers */
       hxfupdate(UPDATE,pHTX);
    } else if ( errno != ENXIO
#ifdef __HTX_LINUX__
				/*
				 * Linux returned ENOSPC for past end of media write
				 */
				&& errno != ENOSPC && errno != EIO
#endif
					) {
       pHTX->bad_writes++;
       sprintf(sWork, "VBS write: errno %d (%s) in loop iteration #%d\n"
                      "Bytes attempted=%d; VBS_last_write=%d\n"
                      "initial seed values: %d,%d,%d; bytes_written=%f",
               errno, strerror(errno), loop_count, num_bytes, VBS_last_write,
               pRule->seed[0], pRule->seed[1], pRule->seed[2], total_bytes);
       hxfmsg(pHTX, errno, HARD, sWork);
       tape_error_code = errno;
       rc = -1;
       break;
    } else {                /* got here by trying to write past End-Of-Media */
#ifdef _DEBUG_LINUX_
		printf("VBS Write: max_xfer:%d errno is:%d ==> %s\n",max_xfer, errno, strerror(errno));
#endif
       found_eot = 1;
       if ( VBS_last_write == -1 )
          VBS_last_write = 0;
       tapeinfo.st_op = STWEOF;                    /* write a final EOF mark */
       tapeinfo.st_count = 1;
       if ( (rc = ioctl(pRule->fildes, STIOCTOP, &tapeinfo)) == -1 ) {
          pHTX->bad_writes++;
          sprintf(sWork,"VBS write: errno %d (%s) in write eof, iteration #%d\n"
                        "Bytes attempted=%d; VBS_last_write=%d\n"
                        "Initial seed values: %d,%d,%d, bytes_written=%f",
                 errno, strerror(errno), loop_count, num_bytes, VBS_last_write,
                 pRule->seed[0], pRule->seed[1], pRule->seed[2], total_bytes);
          hxfmsg(pHTX, errno, HARD, sWork);
          tape_error_code = errno;
          break;
       }
    }
  }
  if ( rc != -1 ) {
     sprintf(sWork, "VBS write: loop completed.  loop_count=%d\n"
                    "max_loops=%d; VBS_last_write=%d; found_eot=%d\n"
                    "Total bytes written this stanza: %f",
             loop_count-1, max_loops, VBS_last_write, found_eot, total_bytes);
     hxfmsg(pHTX, 0, INFO, sWork);
  }
  if ( set_bs(pHTX, pRule, BLK_SIZE, 2) ) {
#ifdef _DEBUG_LINUX_
	 printf("set_bs failed after VBS write\n");
#endif
     tape_error_code = errno;
     rc = -1;
  }
  free(Wbuf);

#ifdef _DEBUG_LINUX_
	printf("VBS_Write returning :%d\n",rc);
#endif
  return(rc);
}

/**************************************************************************/
/* VBS_read -- variable block size read test                              */
/**************************************************************************/
int
VBS_Read(struct htx_data *pHTX, struct ruleinfo *pRule,
             struct blk_num_typ *pblk_num)
{
  int            i, rc, loop_count, max_loops;
  int            found_eot, num_bytes, max_xfer;
  char           sWork[1024];
  char           *Rbuf;
  unsigned short seed[3];

	/* for buster pattern support */
	int bus_width = 0, bufrem = 0;

    /* The VBS random number seed must be initialized in this or prior stanza */
  if ( !VBS_seed_initialized ) {
     hxfmsg(pHTX, 0, SYSERR, "VBS read: operation called but seed not\n"
                             "initialized in this or preceding stanza");
     tape_error_code = errno;
     return(-1);
  }
                             /* Set variable block size mode (block_size = 0) */
  if ( set_bs(pHTX, pRule, 0, 3) ) {
     tape_error_code = errno;
     return(-1);
  }
                                       /* Copy the initial random number seed */
  for ( i = 0; i < 3; i++ )
     seed[i] = pRule->seed[i];
  if ( pRule->num_blks == 1 )
     max_xfer = BUF_SIZE;
  else
     max_xfer = pRule->num_blks;

  #ifdef __HTX_LINUX__
	  /* adjust as per blk limits */
	  if( max_xfer < l_blk_limit ) max_xfer = l_blk_limit;

	  if( max_xfer > m_blk_limit ) max_xfer = m_blk_limit;
  #endif

  /* support for buster pattern */
  if( strncmp( pRule->pattern_id, "BSTR", 4 ) == 0 ) {
      sprintf(sWork,"Buster pattern detected in current stanza\n");
	  hxfmsg(pHTX, 0, INFO, sWork);

	  bus_width = get_bus_width( pRule->pattern_id );
	  sprintf(sWork,"Buster pattern width deduced = %d\n", bus_width);
	  hxfmsg(pHTX, 0, INFO, sWork);

  }

  /*Rbuf = (char *) malloc(max_xfer);*/
  Rbuf = (char *) malloc(max_xfer + bus_width);

  if ( Rbuf == NULL ) {
     sprintf(sWork, "Malloc error of Read buffer in VBS Read Stanza \n");
     hxfmsg(pHTX, errno, HARD, sWork);
     free(Rbuf);
     tape_error_code = errno;
     return(-1);
  }

  if( bus_width ) {
    sprintf(sWork,"Buster pattern malloc'ed Rbuf = %llx\n", (unsigned long long)Rbuf);
    hxfmsg(pHTX, 0, INFO, sWork);

	bufrem = (int)((unsigned long long) Rbuf  % bus_width);
	if( bufrem != 0 ) {
		Rbuf = Rbuf + ( bus_width - bufrem );
	}

	sprintf(sWork,"Buster pattern aligned Rbuf = %llx\n", (unsigned long long)Rbuf);
	hxfmsg(pHTX, 0, INFO, sWork);
  }

  loop_count = 1;
  found_eot = 0;
  if ( pRule->VBS_seed_type != 'F' )
     sprintf(sWork, "Starting VBS read rule. Operation count = %d; pattern %s\n"
                    "Initial seed values: %d,%d,%d; max_xfer = %d\n",
             pRule->num_oper, pRule->pattern_id,
             seed[0], seed[1], seed[2], max_xfer);
  else
     sprintf(sWork, "Starting VBS read rule. Operation count = %d; pattern %s\n"
                    "max_xfer = %d\n",
             pRule->num_oper, pRule->pattern_id, max_xfer);
  hxfmsg(pHTX, 0, INFO, sWork);
        /* Next line sets max_loops to maxint if num_oper spec'd to 0 in rule */
  max_loops = (pRule->num_oper==0) ? ((unsigned)(-1) >> 1) : pRule->num_oper;

  loop_count = 1;
  found_eot = 0;
  while ( (loop_count <= max_loops) && !found_eot ) {
                         /* Generate the length of data to read on this loop */
    if ( pRule->VBS_seed_type != 'F' ) {
       while ( (num_bytes = (int) nrand48(seed) % max_xfer) == 0 ) ;

        #ifdef __HTX_LINUX__
			/* Take into account the blk limits */
			if( num_bytes < l_blk_limit ) num_bytes += l_blk_limit;
        #endif
   }
   else
       num_bytes = max_xfer;
            /* Read the buffer in.  Note that we ask for the maximum amount our
               buffer can hold, but we should only get back what we initially
               wrote.  We could possibly get less than we wrote if the EOM
               had occurred, but NEVER should we get more. */
    if ( num_bytes < 4 )
       num_bytes = 5;
    if ( (rc = read(pRule->fildes, Rbuf, num_bytes)) != -1 ) {
      bytes_read = rc;
      rc = 0;
      total_bytes += (double)bytes_read;
      if ( bytes_read != num_bytes ) {
           /* Didn't get all we expected - EOM maybe? */
        if ( bytes_read != VBS_last_write ) {
           prt_req_sense(pHTX, pRule ,loop_count, pblk_num);
           sprintf(sWork, "VBS: Iteration %d read %d bytes.  Unable to match\n"
                          "request value (%d bytes) nor VBS_last_write of %d\n"
                          "errno = %d\n"
                      "Check that VBS_SEED is correct, else suspect tape error",
                   loop_count, bytes_read, num_bytes, VBS_last_write, errno);
           hxfmsg(pHTX, 0, HARD, sWork);
           tape_error_code = errno;
           rc = -1;
           break;
        } else
           found_eot = 1;
        num_bytes = bytes_read;
      }
      pHTX->good_reads++;
      pHTX->bytes_read += bytes_read;
      loop_count++;
      update_blk_nums(pblk_num, 1);    /* update number of blocks pointers */
      hxfupdate(UPDATE, pHTX);
    } else {
      pHTX->bad_reads++;
      sprintf(sWork, "VBS read: errno %d (%s) in loop iteration #%d\n"
                     "Bytes attempted=%d; VBS_last_write=%d\n"
                     "Initial seed values: %d,%d,%d; bytes_read=%d",
              errno, strerror(errno), loop_count, num_bytes, VBS_last_write,
              pRule->seed[0], pRule->seed[1], pRule->seed[2], bytes_read);
      hxfmsg(pHTX, errno, HARD, sWork);
      tape_error_code = errno;
      rc = -1;
      break;
    }
  }
  free(Rbuf);
  if ( rc == -1 ) {
     set_bs(pHTX, pRule, BLK_SIZE, 4);
     return(-1);
  } else {
     sprintf(sWork, "VBS read: readback loop completed.  loop_count=%d\n"
                    "max_loops=%d; VBS_last_write=%d; found_eot=%d\n"
                    "Total bytes read this stanza: %f",
             loop_count-1, max_loops, VBS_last_write, found_eot, total_bytes);
     hxfmsg(pHTX, 0, INFO, sWork);
  }
  if ( set_bs(pHTX, pRule, BLK_SIZE, 5) )
     return(-1);
  return(0);
}

/**************************************************************************/
/* VBS_Readc -- variable block size read / compare test                    */
/**************************************************************************/
int
VBS_Readc(struct htx_data *pHTX, struct ruleinfo *pRule,
              struct blk_num_typ *pblk_num)
{
  int      i, rc, loop_count, max_loops;
  int      found_eot, num_bytes, max_xfer;
  char     sWork[1024],  misc_data[MAX_TEXT_MSG];
  char     *Rbuf, *Wbuf;
  unsigned short seed[3];

	/* for buster pattern support */
	int bus_width = 0, bufrem = 0;

    /* The VBS random number seed must be initialized in this or prior stanza */
  if ( !VBS_seed_initialized ) {
     hxfmsg(pHTX, 0, SYSERR, "VBS read: operation called but seed not\n"
                             "initialized in this or preceding stanza");
     tape_error_code = errno;
     return(-1);
  }
    /* Set variable block size mode (block_size = 0) */
  if ( set_bs(pHTX, pRule, 0, 3) ) {
     tape_error_code = errno;
     return(-1);
  }
                                     /* Copy the initial random number seed */
  for ( i = 0; i < 3; i++ )
     seed[i] = pRule->seed[i];
  if ( pRule->num_blks == 1 )
     max_xfer = BUF_SIZE;
  else
     max_xfer = pRule->num_blks;

  #ifdef __HTX_LINUX__
	  /* adjust as per blk limits */
	  if( max_xfer < l_blk_limit ) max_xfer = l_blk_limit;

	  if( max_xfer > m_blk_limit ) max_xfer = m_blk_limit;
  #endif

  /* support for buster pattern */
  if( strncmp( pRule->pattern_id, "BSTR", 4 ) == 0 ) {
      sprintf(sWork,"Buster pattern detected in current stanza\n");
	  hxfmsg(pHTX, 0, INFO, sWork);

	  bus_width = get_bus_width( pRule->pattern_id );
	  sprintf(sWork,"Buster pattern width deduced = %d\n", bus_width);
	  hxfmsg(pHTX, 0, INFO, sWork);

  }

/*  Rbuf = (char *) malloc(max_xfer);
  Wbuf = (char *) malloc(max_xfer);*/
  Rbuf = (char *) malloc(max_xfer + bus_width);
  Wbuf = (char *) malloc(max_xfer + bus_width);

  if ( Rbuf == NULL || Wbuf == NULL ) {
     sprintf(sWork, "Malloc error of buffers in VBS Read Compare Stanza\n");
     hxfmsg(pHTX, errno, HARD, sWork);
     free(Rbuf);
     tape_error_code = errno;
     return(-1);
  }

  if( bus_width ) {
    sprintf(sWork,"Buster pattern malloc'ed Rbuf = %llx, Wbuf = %llx\n", (unsigned long long)Rbuf, (unsigned long long)Wbuf);
    hxfmsg(pHTX, 0, INFO, sWork);

	bufrem = (int)((unsigned long long) Wbuf  % bus_width);
	if( bufrem != 0 ) {
		Wbuf = Wbuf + ( bus_width - bufrem );
	}

	bufrem = (int)((unsigned long long) Rbuf % bus_width);
	if( bufrem != 0 ) {
		Rbuf = Rbuf + ( bus_width - bufrem );
	}

	sprintf(sWork,"Buster pattern aligned Rbuf = %llx, Wbuf = %llx\n", (unsigned long long)Rbuf, (unsigned long long)Wbuf);
	hxfmsg(pHTX, 0, INFO, sWork);
  }

  loop_count = 1;
  found_eot = 0;

  if ( pRule->VBS_seed_type != 'F' )
     sprintf(sWork, "Starting VBS read rule. Operation count = %d; pattern %s\n"
                    "Initial seed values: %d,%d,%d; max_xfer = %d\n",
             pRule->num_oper, pRule->pattern_id,
             seed[0], seed[1], seed[2], max_xfer);
  else
     sprintf(sWork, "Starting VBS read rule. Operation count = %d; pattern %s\n"
                    "max_xfer = %d\n",
             pRule->num_oper, pRule->pattern_id, max_xfer);
  hxfmsg(pHTX, 0, INFO, sWork);
     /* Next line sets max_loops to maxint if num_oper spec'd to 0 in rule */
  max_loops = (pRule->num_oper==0) ? ((unsigned)(-1) >> 1) : pRule->num_oper;
     /* Set up the pattern buffer */
  if ( (pRule->pattern_id[0] != '#') && (pRule->pattern_id[0] != '\0') ) {
       /* get HTXPATTERNS environment variable */
	sWork[0] = '\0';
    if ( htx_strlen(htx_strcpy(sWork, getenv("HTXPATTERNS"))) == 0 )
      strcpy(sWork, "../pattern/");                     /* For default ONLY */
    strcat (sWork, pRule->pattern_id);
  }

      /* Initialize the write buffer if a pattern file was specified. */
      /* Fill the entire buffer with the pattern if so. */
  if ( pRule->pattern_id[0] != '#' ) {
    if ( (rc = hxfpat(sWork, Wbuf, max_xfer)) == 1 ) {
       sprintf(sWork, "Cannot open pattern file - %s\n", pRule->pattern_id);
       hxfmsg(pHTX, 0, SYSERR, sWork);
       return(-1);
    } else if ( rc == 2 ) {
       sprintf(sWork, "Cannot read pattern file - %s\n", pRule->pattern_id);
       hxfmsg(pHTX, 0, SYSERR, sWork);
       return(-1);
    }
    /* memcpy(Rbuf, Wbuf, max_xfer); */
  } else
    bldbuf((unsigned short*)Wbuf, max_xfer, pRule->pattern_id, pblk_num);

  loop_count = 1;
  found_eot = 0;
  while ( (loop_count <= max_loops) && !found_eot ) {
                         /* Generate the length of data to read on this loop */
    if ( pRule->VBS_seed_type != 'F' ) {
       while ( (num_bytes = (int) nrand48(seed) % max_xfer) == 0 ) ;

        #ifdef __HTX_LINUX__
			/* Take into account the blk limits */
			if( num_bytes < l_blk_limit ) num_bytes += l_blk_limit;
        #endif
   }
    else
       num_bytes = max_xfer;
                    /* initialize the write buffer if using dynamic patterns */
    if ( num_bytes < 4 )
       num_bytes = 5;
        /* Read the buffer in.  Note that we ask for the maximum amount our
           buffer can hold, but we should only get back what we initially
           wrote.  We could possibly get less than we wrote if the EOM
           had occurred, but NEVER should we get more. */
    if ( (rc = read(pRule->fildes, Rbuf, max_xfer)) != -1 ) {
      bytes_read = rc;
      rc = 0;
      total_bytes += (double)bytes_read;
      if ( bytes_read != num_bytes ) {
        if ( bytes_read < num_bytes ) {
                                 /* Didn't get all we expected - EOM maybe? */
           if ( bytes_read != VBS_last_write ) {
              sprintf(sWork, "VBS:Iteration %d read %d bytes. Unable to match\n"
                           "request value (%d bytes) nor VBS_last_write of %d\n"
                      "Check that VBS_SEED is correct, else suspect tape error",
                      loop_count, bytes_read, num_bytes, VBS_last_write);
              hxfmsg(pHTX, 0, HARD, sWork);
              tape_error_code = errno;
              rc = -1;
              break;
           } else
              found_eot = 1;
           num_bytes = bytes_read;
        } else {
           sprintf(sWork, "VBS:Iteration %d read %d bytes which was more than\n"
                          "the requested value (%d bytes)\n",
                   loop_count, bytes_read, num_bytes);
           hxfmsg(pHTX, 0, HARD, sWork);
           tape_error_code = errno;
           rc = -1;
           break;
        }
      }
          /* Compare against what we expected... */
      if ( (rc=cmp_buf(pHTX, Wbuf, Rbuf, num_bytes, misc_data)) != 0 ) {
         if ( crash_on_mis )
	 {
	    #ifndef __HTX_LINUX__
	    setleds( 0x2010 );
	    #endif
            trap(0xBEEFDEAD, Wbuf, Rbuf, num_bytes, pHTX, pRule);
         }
	 sprintf(sWork, "VBS read: ******* miscompare error in readback!\n"
                        "loop #%d of %d, read %d bytes of %d expected.\n"
                        "VBS_last_write flag=%d, found_eot flag=%d\n"
                        "Total bytes read (so far): %lld\n",
                 loop_count, max_loops, bytes_read, num_bytes,
                 VBS_last_write, found_eot, pHTX->bytes_read);
         strcat(sWork, misc_data);
        /*  hxfmsg(pHTX, -1, HARD, sWork); */
         hxfmsg(pHTX, -1, HTX_HE_MISCOMPARE, sWork);
         tape_error_code = errno;
      }
      pHTX->good_reads++;
      pHTX->bytes_read += bytes_read;
      loop_count++;
      update_blk_nums(pblk_num, 1);    /* update number of blocks pointers */
      hxfupdate(UPDATE, pHTX);
    } else {
      pHTX->bad_reads++;
      sprintf(sWork, "VBS read: errno %d (%s) in loop iteration #%d\n"
                     "Bytes attempted=%d; VBS_last_write=%d\n"
                     "Initial seed values: %d,%d,%d; bytes_read=%d",
              errno, strerror(errno), loop_count, num_bytes, VBS_last_write,
              pRule->seed[0], pRule->seed[1], pRule->seed[2], bytes_read);
      hxfmsg(pHTX, errno, HARD, sWork);
      tape_error_code = errno;
      rc = -1;
      break;
    }
  }
  if ( rc == -1 ) {
     set_bs(pHTX, pRule, BLK_SIZE, 4);
     return(-1);
  } else {
     sprintf(sWork, "VBS read/compare: readback loop completed. loop_count=%d\n"
                    "max_loops=%d; VBS_last_write=%d; found_eot=%d\n"
                    "Total bytes read this stanza: %f",
             loop_count-1, max_loops, VBS_last_write, found_eot, total_bytes);
     hxfmsg(pHTX, 0, INFO, sWork);
  }
  if ( set_bs(pHTX, pRule, BLK_SIZE, 5) )
     return(-1);
  return(0);
}

#ifndef __HTX_LINUX__
/**************************************************************************/
/* init_element -- adante initialize element                              */
/**************************************************************************/
int
init_element(struct htx_data *phtx_info, struct ruleinfo *prule_info,
             int loop, struct blk_num_typ *pblk_num)
{
  int  rc;
  char tmp_str[120];

  tmp_str[0] ='\0';
  rc = ioctl(prule_info->fildes, INIT_ELEMENT, NULL);
  if ( rc == -1 ) {
     phtx_info->bad_others = phtx_info->bad_others + 1;
     strcpy(tmp_str, "Adante Init Element Error - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  }
  return(rc);
}

/**************************************************************************/
/* read_status -- adante read element status                              */
/**************************************************************************/
int
read_status(struct htx_data *phtx_info, struct ruleinfo *prule_info,
            int loop, struct blk_num_typ *pblk_num)
{
  int    rc, i;
  char   tmp_str[120], element_buf[ELEMENT_REPORT_LEN];
  struct element_status el;

  tmp_str[0]='\0';
  el.n_elements = prule_info->num_blks;
  el.stat_len = (el.n_elements * 0x10) + 41;
  el.element_stat = element_buf;
  rc = ioctl(prule_info->fildes, READ_ELEMENT_STATUS, &el);
  if ( rc == -1 ) {
     phtx_info->bad_others = phtx_info->bad_others + 1;
     strcpy(tmp_str, "Adante Read Element Status Error - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
    strcpy(msg_str, "   Element Status Data:");
    for ( i = 0; i < el.stat_len; i++ ) {
       if ( (i & 15) == 0 ) {
          sprintf(tmp_str, "\n[%02X]", i);
          strcat(msg_str, tmp_str);
       }
       sprintf(tmp_str, "%02X", element_buf[i]);
       strcat(msg_str, tmp_str);
    }
    hxfmsg(phtx_info, 0, INFO, msg_str);
  }
  return(rc);
}

/**************************************************************************/
/* medium_load -- adante move medium tape load                            */
/**************************************************************************/
int
medium_load(struct htx_data *phtx_info, struct ruleinfo *prule_info,
            int loop, struct blk_num_typ *pblk_num)
{
  int  rc;
  char tmp_str[120];

  rc = ioctl(prule_info->fildes, MOVE_MEDIUM_LOAD, prule_info->source_id);
  if ( rc == -1 ) {
     phtx_info->bad_others = phtx_info->bad_others + 1;
     strcpy(tmp_str, "Adante Move Medium Load Error - ");
     strcat(tmp_str, strerror((errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  }
  return(rc);
}

/**************************************************************************/
/* medium_unload -- adante move medium tape unload                        */
/**************************************************************************/
int
medium_unload(struct htx_data *phtx_info, struct ruleinfo *prule_info,
              int loop, struct blk_num_typ *pblk_num)
{
  int  rc;
  char tmp_str[120];

  rc = ioctl(prule_info->fildes, MOVE_MEDIUM_UNLOAD, prule_info->source_id);
  if ( rc == -1 ) {
     phtx_info->bad_others = phtx_info->bad_others + 1;
     strcpy(tmp_str, "Adante Move Medium UnLoad Error - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  }
  return(rc);
}

/**************************************************************************/
/* loc_block -- adante locate to a specific block                         */
/**************************************************************************/
int
loc_block(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          int loop, struct blk_num_typ *pblk_num)
{
  int  rc;
  char tmp_str[120];

  rc = ioctl(prule_info->fildes, DDS2_LOCATE, prule_info->num_blks);
  if ( rc == -1 ) {
     phtx_info->bad_others = phtx_info->bad_others + 1;
     strcpy(tmp_str, "Adante Locate Block Error - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  }
  return(rc);
}

/**************************************************************************/
/* read_posit -- adante read position on tape                             */
/**************************************************************************/
int
read_posit(struct htx_data *phtx_info, struct ruleinfo *prule_info,
           int loop, struct blk_num_typ *pblk_num)
{
  int  rc;
  char tmp_str[120];

  rc = ioctl(prule_info->fildes, DDS2_READ_POS, prule_info->num_blks);
  if ( rc == -1 ) {
     phtx_info->bad_others = phtx_info->bad_others + 1;
     strcpy(tmp_str, "Adante Read Position Error - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  }
  return(rc);
}

/**************************************************************************/
/* asearch_file -- adante function that will skip forward prule_info->    */
/* num_blks of file marks. If prule_info->num_blks is negative, then      */
/* the skipping will be backwards.                                        */
/**************************************************************************/
int
asearch_file(struct htx_data *phtx_info, struct ruleinfo *prule_info,
             int loop, struct blk_num_typ *pblk_num)
{
  int    rc;
  char   tmp_str[120];
  struct stop tapeinfo;

  tapeinfo.st_op = STRSF;    /* set direction pointer for error message */
  tapeinfo.st_count = prule_info->num_blks;
  rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
  if ( rc == -1 ) {
     phtx_info->bad_reads = phtx_info->bad_reads + 1;
     if ( errno == EIO )
        strcpy(tmp_str, "Adante End-of-Media sensed during a backward "
                        " file skip operation - ");
     else
        strcpy(tmp_str, "Adante Backward File Skip Error - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     pblk_num->in_file = 0;
     pblk_num->in_rule = 0;
  }
  return(rc);
}

/**************************************************************************/
/* asearch_rec -- adante function that will skip forward prule_info->     */
/* num_blks of records. If prule_info->num_blks is negative, then the     */
/* skipping will be backwards.                                            */
/**************************************************************************/
int
asearch_rec(struct htx_data *phtx_info, struct ruleinfo *prule_info,
            int loop, struct blk_num_typ *pblk_num)
{
  int    rc;
  char   tmp_str[120];
  struct stop tapeinfo;

  tapeinfo.st_op = STRSR;    /* set direction pointer for error message */
  tapeinfo.st_count = prule_info->num_blks;
  rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
  if ( rc == -1 ) {
     phtx_info->bad_reads = phtx_info->bad_reads + 1;
     if ( errno == EIO )
        strcpy(tmp_str, "Adante End-of-Media sensed during a  "
                        "record skip operation - ");
     else
        strcpy(tmp_str, "Adante Record Skip Error - ");
        strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     tape_error_code = errno;
  } else {
     pblk_num->in_file = pblk_num->in_file + prule_info->num_blks;
     pblk_num->in_rule = pblk_num->in_rule + prule_info->num_blks;
  }
  return(rc);
}

/**************************************************************************/
/* write_unload -- adante write EOT and then unload tape and begin again  */
/**************************************************************************/
int
write_unload(struct htx_data *phtx_info, struct ruleinfo *prule_info,
             int loop, struct blk_num_typ *pblk_num, char *wbuf)
{
  struct stop tapeinfo;
  int    rc, continue_loop = 0;
  char   tmp_str[120];

  while ( continue_loop == 0 ) {
     while ( (rc = write(prule_info->fildes, wbuf,
                        (prule_info->num_blks * BLK_SIZE))) != -1 ) {
        lseek(prule_info->fildes,0,1);
        phtx_info->good_writes = phtx_info->good_writes + 1;
        phtx_info->bytes_writ = phtx_info->bytes_writ + rc;
        update_blk_nums(pblk_num, rc);    /* update number of blocks pointers */
        hxfupdate(UPDATE,phtx_info);
     }
     if ( errno != ENXIO ) {
        continue_loop = 1;
        phtx_info->bad_writes = phtx_info->bad_writes + 1;
        strcpy(tmp_str, "Write to End-of-Tape Error - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     } else if ( (errno == EBADF) && (Open_Mode == O_RDONLY) ) {
        continue_loop = 1;
        strcpy(tmp_str, "Write Error due to Write-Protected Media - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                     tmp_str);
        tape_error_code = errno;
     } else {
        tapeinfo.st_op = STWEOF;      /* write an EOF mark at end of the tape */
        tapeinfo.st_count = 1;
        rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
        if ( rc == -1 ) {
           continue_loop = 1;
           phtx_info->bad_writes = phtx_info->bad_writes + 1;
           strcpy(tmp_str, "Write EOF Error after WEOT - ");
           strcat(tmp_str, strerror(errno));
           strcat(tmp_str, "\n");
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                   tmp_str);
           tape_error_code = errno;
        } else {
           tapeinfo.st_op = STOFFL;
           rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
           if ( rc == -1 ) {
              continue_loop = 1;
              phtx_info->bad_writes = phtx_info->bad_writes + 1;
              strcpy(tmp_str, "Tape Unload Error - ");
              strcat(tmp_str, strerror(errno));
              strcat(tmp_str, "\n");
              prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                      tmp_str);
              continue_loop = 1;
              tape_error_code = errno;
           } else {
                       /*--- close and reopen in diagnostics ---*/
              sleep(prule_info->u_sleep);
              rc = close(prule_info->fildes);
              if ( /*rc == -1*/ 0 ) {
                 strcpy(tmp_str, "Close error on ");
                 strcat(tmp_str, phtx_info->sdev_id);
                 strcat(tmp_str, " in Adante Unload Request Sense command.\n");
                 strcat(tmp_str, strerror(errno));
                 strcat(tmp_str,"\n");
                 hxfmsg(phtx_info, errno, HARD, tmp_str);
                 continue_loop = 1;
                 tape_error_code = errno;
              } else {
                 prule_info->fildes = openx(phtx_info->sdev_id, Open_Mode, 0777,
                                            SC_DIAGNOSTIC);
                 if ( prule_info->fildes == -1 ) {
                    strcpy(tmp_str, "Diagnostic Open Error - ");
                    strcat(tmp_str, strerror(errno));
                     strcat(tmp_str, "\n");
                     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                             tmp_str);
                     continue_loop = 1;
                     tape_error_code = errno;
                 } else {
                                   /*--- send a request command ---*/
					if(found_fscsi == 1){
						memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));
						fiocmd_buf.data_length = sizeof(sense_buf);
						fiocmd_buf.buffer = sense_buf;
						fiocmd_buf.timeout_value = 30;
						fiocmd_buf.flags = B_READ;
						fiocmd_buf.command_length = 6;
						fiocmd_buf.scsi_cdb[0] = 3;
						fiocmd_buf.scsi_cdb[4] = MAX_SENSE;
						rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
						if ( rc == -1 ) {
							continue_loop = 1;
							phtx_info->bad_others = phtx_info->bad_others + 1;
							strcpy(tmp_str, "Request Sense Error - ");
							strcat(tmp_str, strerror(errno));
							strcat(tmp_str, "\n");
							prt_msg(phtx_info, prule_info, loop, pblk_num, errno,HARD, msg_str);
							tape_error_code = errno;
						}
						else {
							if ( sense_buf[2] == 0x06 )
								continue_loop = 0;
							else {
								if ( sense_buf[2] == 0x00 ) {
								rc = 55;
								continue_loop = 1;
								}
								else {
									continue_loop = 1;
									phtx_info->bad_others = phtx_info->bad_others + 1;
									sprintf(msg_str, "diagnostics request sense error"
													"-\nstatus validity = %x   scsi"
													"bus status = %x   adapter "
													"status = %x\n",
													fiocmd_buf.status_validity,
													fiocmd_buf.scsi_bus_status,
													fiocmd_buf.adapter_status);
									prt_msg(phtx_info, prule_info, loop, pblk_num,
													errno, HARD, msg_str);
									tape_error_code = errno;
								}
							}
						}
					}
					else {
						memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));
						iocmd_buf.data_length = sizeof(sense_buf);
						iocmd_buf.buffer = sense_buf;
						iocmd_buf.timeout_value = 30;
						iocmd_buf.flags = B_READ;
						iocmd_buf.command_length = 6;
						iocmd_buf.scsi_cdb[0] = 3;
						iocmd_buf.scsi_cdb[4] = MAX_SENSE;
						rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);
						if ( rc == -1 ) {
							continue_loop = 1;
							phtx_info->bad_others = phtx_info->bad_others + 1;
							strcpy(tmp_str, "Request Sense Error - ");
							strcat(tmp_str, strerror(errno));
							strcat(tmp_str, "\n");
							prt_msg(phtx_info, prule_info, loop, pblk_num, errno,HARD, msg_str);
							tape_error_code = errno;
						}
						else {
							if ( sense_buf[2] == 0x06 )
								continue_loop = 0;
							else {
								if ( sense_buf[2] == 0x00 ) {
									rc = 55;
									continue_loop = 1;
								}
								else {
									continue_loop = 1;
									phtx_info->bad_others = phtx_info->bad_others + 1;
									sprintf(msg_str, "diagnostics request sense error"
														"-\nstatus validity = %x   scsi"
														"bus status = %x   adapter "
														"status = %x\n",
														iocmd_buf.status_validity,
														iocmd_buf.scsi_bus_status,
														iocmd_buf.adapter_status);
									prt_msg(phtx_info, prule_info, loop, pblk_num,
															errno, HARD, msg_str);
									tape_error_code = errno;
								}
							}
						}
					}
				}
				/*--- close and reopen not in diagnostics ---*/
                 rc = close(prule_info->fildes);
                 if ( /*rc == -1*/ 0) {
                    strcpy(tmp_str, "Close error on ");
                    strcat(tmp_str, phtx_info->sdev_id);
                    strcat(tmp_str, " at end of Request Sense rule.\n");
                    strcat(tmp_str, strerror(errno));
                    strcat(tmp_str, "\n");
                    hxfmsg(phtx_info, errno, HARD, tmp_str);
                    continue_loop = 1;
                    tape_error_code = errno;
                 } else {
                    prule_info->fildes = open(phtx_info->sdev_id,Open_Mode);
                    if ( prule_info->fildes == -1 ) {
                       strcpy(tmp_str, "Open Error in UnLoad Command - ");
                       strcat(tmp_str, strerror(errno));
                       strcat(tmp_str, "\n");
                       prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                               HARD, tmp_str);
                       continue_loop = 1;
                       tape_error_code = errno;
                    }
                 }
              }
           }
        }
     }
  }
  if ( rc == 55 ) {
     rc = 0;
     return(rc);
  } else
     return(rc);
}

/**************************************************************************/
/* twin_tape -- timberwolf initialize element                             */
/**************************************************************************/
int
twin_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          int loop, struct blk_num_typ *pblk_num)
{
  int    count, rc, i, sffd;
  char   tmp_str[120];

  if ( (sffd = open(prule_info->chs_file, O_RDWR | O_NDELAY)) < 0 ) {
     strcpy(tmp_str, "Error on Open in TimberWolf Initialize Element - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     rc = -1;
     tape_error_code = errno;
  } else {
     if ( !ioctl (sffd, SMCIOC_INIT_ELEM_STAT, 0) ) {
         sprintf(msg_str, "Initialize Element command succeeded \n");
         hxfmsg(phtx_info, 0, INFO, msg_str);
         rc = close(sffd);                       /* close changer device */
         if ( rc == -1 ) {
            strcpy(tmp_str, "Close error on ");
            strcat(tmp_str, phtx_info->sdev_id);
            strcat(tmp_str, " at end of initialize element rule.\n");
            strcat(tmp_str, strerror(errno));
            strcat(tmp_str, "\n");
            hxfmsg(phtx_info, errno, HARD, tmp_str);
            tape_error_code = errno;
         }
     } else {
         phtx_info->bad_others = phtx_info->bad_others + 1;
         strcpy(tmp_str, "TimberWolf Initialize Element Error - ");
         strcat(tmp_str, strerror(errno));
         strcat(tmp_str, "\n");
         prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
         tape_error_code = errno;
         rc = close(sffd);                       /* close changer device */
         if ( rc == -1 ) {
            strcpy(tmp_str, "Close error on ");
            strcat(tmp_str, phtx_info->sdev_id);
            strcat(tmp_str, " at end of initialize element rule.\n");
            strcat(tmp_str, strerror(errno));
            strcat(tmp_str, "\n");
            hxfmsg(phtx_info, errno, HARD, tmp_str);
            tape_error_code = errno;
         }
     }
  }
  if ( tape_error_code != 0 )
     rc = -1;
  else
     rc = 0;
  return(rc);
}

/**************************************************************************/
/* twps_tape -- timberwolf position to element                            */
/**************************************************************************/
int
twps_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          int loop, struct blk_num_typ *pblk_num)
{
  int    count, rc, i, sffd;
  char   tmp_str[120];
  struct pos_to_elem pos_to_elem;

  if ( (sffd = open(prule_info->chs_file, O_RDWR | O_NDELAY)) < 0 ) {
     strcpy(tmp_str, "TimberWolf Position Element Open Error - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     pos_to_elem.destination = prule_info->dest_id;   /* destination address */
     pos_to_elem.robot = 0;
     pos_to_elem.invert = 0;
     if ( !ioctl (sffd, 0x80054305, &pos_to_elem) ) {
        sprintf(msg_str, "Position to Element command succeeded \n");
        hxfmsg(phtx_info, 0, INFO, msg_str);
     } else {
        phtx_info->bad_others = phtx_info->bad_others + 1;
        strcpy(tmp_str, "TimberWolf Position Element Error - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        sprintf(msg_str, "Position to Element command error - \n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     }
     rc = close(sffd);       /* close and reopen not in diagnostics mode */
     if ( rc == -1 ) {
        strcpy(tmp_str, "Close error on ");
        strcat(tmp_str, phtx_info->sdev_id);
        strcat(tmp_str, " at end of position to element rule.\n");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        hxfmsg(phtx_info, errno, HARD, tmp_str);
        tape_error_code = errno;
     }
  }
  if ( tape_error_code != 0 )
     rc = -1;
  else
     rc = 0;
  return(rc);
}

/**************************************************************************/
/* twrd_stat -- timberwolf read element status                            */
/**************************************************************************/
int
twrd_stat(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          int loop, struct blk_num_typ *pblk_num)
{
  int    count, rc, i, j, sffd;
  char   tmp_str[512];
  uchar  *data;
  struct telement_status robot_status[1];
  struct telement_status slot_status[22];
  struct telement_status ie_status[1];
  struct telement_status drive_status[2];
  struct inventory inventory;
  struct element_info element_info;

  if ( (sffd = open(prule_info->chs_file, O_RDWR | O_NDELAY)) < 0 ) {
     strcpy(tmp_str, "TimberWolf Read Element Status Open Error - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     bzero( (caddr_t)robot_status, sizeof(struct telement_status) );
     for ( i = 0; i < 22; i++ )
        bzero( (caddr_t)(&slot_status[i]), sizeof(struct telement_status) );
     bzero( (caddr_t)ie_status, sizeof(struct telement_status) );
     for ( i = 0; i < 2; i++ )
        bzero( (caddr_t)(&drive_status[i]), sizeof(struct telement_status) );
     if ( !ioctl (sffd, 0x40104303, &element_info) ) {
        sprintf(msg_str, "Element Information command succeeded \n");
        hxfmsg(phtx_info, 0, INFO, msg_str);
        inventory.robot_status = robot_status;
        inventory.slot_status = slot_status;
        inventory.ie_status = ie_status;
        inventory.drive_status = drive_status;
        if ( !ioctl (sffd, 0x80104307, &inventory) ) {
           sprintf(msg_str, "Inventory Information command succeeded \n");
           hxfmsg(phtx_info, 0, INFO, msg_str);
           count = sizeof(struct telement_status);
           data = (uchar *)&inventory.robot_status[0];
           strcpy(msg_str, " Robot Status Data:\n");
           for ( i = 0; i < count; i++ ) {
              if ( (i & 15) == 0 ) {
                 sprintf(tmp_str, "\n [%02X]", i);
                 strcat(msg_str, tmp_str);
              }
              sprintf(tmp_str, " %02X", data[i]);
              strcat(msg_str, tmp_str);
           }
           hxfmsg(phtx_info, 0, INFO, msg_str);
           for ( j = 0; j < element_info.slots; j++ ) {
              data = (uchar *)&inventory.slot_status[j];
              sprintf(tmp_str, " Slot %d Status Data:\n", j+1);
              strcpy(msg_str, tmp_str);
              for ( i = 0; i < count; i++ ) {
                 if ( (i & 15) == 0 ) {
                    sprintf(tmp_str, "\n [%02X]", i);
                    strcat(msg_str, tmp_str);
                 }
                 sprintf(tmp_str, " %02X", data[i]);
                 strcat(msg_str, tmp_str);
              }
              hxfmsg(phtx_info, 0, INFO, msg_str);
           }
           data = (uchar *)&inventory.ie_status[0];
           strcpy(msg_str, " IE Status Data:\n");
           for ( i = 0; i < count; i++ ) {
              if ( (i & 15) == 0 ) {
                 sprintf(tmp_str, "\n [%02X]", i);
                 strcat(msg_str, tmp_str);
              }
              sprintf(tmp_str, " %02X", data[i]);
              strcat(msg_str, tmp_str);
           }
           hxfmsg(phtx_info, 0, INFO, msg_str);
           for ( j = 0; j < element_info.drives; j++ ) {
              data = (uchar *)&inventory.drive_status[j];
              sprintf(tmp_str, " Drive %d Status Data:\n", j+1);
              strcpy(msg_str, tmp_str);
              for ( i = 0; i < count; i++ ) {
                 if ( (i & 15) == 0 ) {
                    sprintf(tmp_str, "\n [%02X]", i);
                    strcat(msg_str, tmp_str);
                 }
                 sprintf(tmp_str, " %02X", data[i]);
                 strcat(msg_str, tmp_str);
              }
              hxfmsg(phtx_info, 0, INFO, msg_str);
           }
        } else {
           phtx_info->bad_others = phtx_info->bad_others + 1;
           strcpy(tmp_str, "TimberWolf Read Element Status Error - ");
           strcat(tmp_str, strerror((errno));
           strcat(tmp_str, "\n");
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
           tape_error_code = errno;
        }
     } else {
        phtx_info->bad_others = phtx_info->bad_others + 1;
        strcpy(tmp_str, "TimberWolf Read Element Status Error - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     }
     rc = close(sffd);
     if ( rc == -1 ) {
        strcpy(tmp_str, "Close error on ");
        strcat(tmp_str, phtx_info->sdev_id);
        strcat(tmp_str, " at end of read element status rule.\n");
        strcat(tmp_str, strerror((errno));
        strcat(tmp_str, "\n");
        hxfmsg(phtx_info, errno, HARD, tmp_str);
        tape_error_code = errno;
    }
  }
  if ( tape_error_code != 0 )
     rc = -1;
  else
     rc = 0;
  return(rc);
}

/**************************************************************************/
/* twmv_tape -- timberwolf move medium command                            */
/**************************************************************************/
int
twmv_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          int loop, struct blk_num_typ *pblk_num)
{
  int    count, rc, i, sffd;
  char   tmp_str[120];
  uchar  *data;
  struct move_medium move_medium;
  struct stop tapeinfo;
  struct request_sense sense_data;

  if ( prule_info->source_id == 23 || prule_info->source_id == 24 ) {
     tapeinfo.st_op = STOFFL;
     rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
     if ( rc == -1 ) {
        phtx_info->bad_others = phtx_info->bad_others + 1;
        strcpy(tmp_str, "TimberWolf UnLoad Error - ");
        strcat(tmp_str, strerror((errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     } else
       sleep(10);
  }
  if ( tape_error_code == 0 ) {
     if ( (sffd = open(prule_info->chs_file, O_RDWR | O_NDELAY)) < 0 ) {
        strcpy(tmp_str, "TimberWolf Move Tape Open Error - ");
        strcat(tmp_str, strerror((errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     } else {
        move_medium.source = prule_info->source_id;
        move_medium.destination = prule_info->dest_id;
        move_medium.robot = 0;
        move_medium.invert = 0;
        if ( !ioctl (sffd, 0x80074304, &move_medium) ) {
           sprintf(msg_str, "Move Medium succeeded\n");
           hxfmsg(phtx_info, 0, INFO, msg_str);
        } else {
           phtx_info->bad_others = phtx_info->bad_others + 1;
           strcpy(tmp_str, "TimberWolf Move Medium Error - ");
           strcat(tmp_str, strerror((errno));
           strcat(tmp_str, "\n");
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
           tape_error_code = errno;
        }
        rc = close(sffd);
        if ( rc < 0 ) {
           strcpy(tmp_str, "Close error on ");
           strcat(tmp_str, phtx_info->sdev_id);
           strcat(tmp_str, " at end of move medium rule.\n");
           strcat(tmp_str, strerror((errno));
           strcat(tmp_str, "\n");
           hxfmsg(phtx_info, errno, HARD, tmp_str);
           tape_error_code = errno;
        }
     }
  }
  if ( tape_error_code != 0 )
     rc = -1;
  else
     rc = 0;
  return(rc);
}

/**************************************************************************/
/* tape_unload -- timberwolf write to tape and unload tape and write to   */
/* next tape.                                                             */
/**************************************************************************/
int tape_unload(struct htx_data *phtx_info, struct ruleinfo *prule_info,
            int loop, struct blk_num_typ *pblk_num, char *wbuf)
{
  int    rc, sffd, continue_loop = 0;
  char   tmp_str[120];
  uchar  *data;
  struct stop tapeinfo;
  struct request_sense sense_data;

  while ( continue_loop == 0 ) {
     lseek(prule_info->fildes, 0, 1);
     rc = write(prule_info->fildes, wbuf, (prule_info->num_blks * BLK_SIZE));
     if ( rc == -1 ) {
        phtx_info->bad_writes = phtx_info->bad_writes + 1;
        if ( errno == ENXIO ) {
           strcpy(tmp_str, "TimberWolf Write Error Past End-of-Media - ");
           strcat(tmp_str, strerror(errno));
           strcat(tmp_str, "\n");
           prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                        tmp_str);
           continue_loop = 1;
           tape_error_code = errno;
        } else if ( (errno == EBADF) && (Open_Mode == O_RDONLY) ) {
           strcpy(tmp_str, "TimberWolf Write Error - Write-Protected Media - ");
           strcat(tmp_str, strerror((errno));
           strcat(tmp_str, "\n");
           prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                        tmp_str);
           continue_loop = 1;
           tape_error_code = errno;
        } else {
           strcpy(tmp_str, "TimberWolf Write Error - ");
           strcat(tmp_str, strerror(errno));
           strcat(tmp_str, "\n");
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
           continue_loop = 1;
           tape_error_code = errno;
        }
     } else {
        update_blk_nums(pblk_num,rc);
        phtx_info->good_writes = phtx_info->good_writes + 1;
        phtx_info->bytes_writ = phtx_info->bytes_writ + rc;
        hxfupdate(UPDATE,phtx_info);
        tapeinfo.st_op = STWEOF;           /* write an EOF mark on the file */
        tapeinfo.st_count = 1;
        rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
        if ( rc == -1 ) {
           continue_loop = 1;
           phtx_info->bad_writes = phtx_info->bad_writes + 1;
           strcpy(tmp_str, "TimberWolf Write EOF Error after WEOT - ");
           strcat(tmp_str, strerror((errno));
           strcat(tmp_str, "\n");
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                   tmp_str);
           tape_error_code = errno;
        } else {
           tapeinfo.st_op = STOFFL;
           rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
           if ( rc == -1 ) {
              continue_loop = 1;
              phtx_info->bad_writes = phtx_info->bad_writes + 1;
              strcpy(tmp_str, "TimberWolf Tape UnLoad Error - ");
              strcat(tmp_str, strerror((errno));
              strcat(tmp_str, "\n");
              prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                      tmp_str);
              tape_error_code = errno;
           } else {
              if ( (sffd = open(prule_info->chs_file,O_RDWR | O_NDELAY)) < 0 ) {
                 strcpy(tmp_str, "TimberWolf Tape Open Error - ");
                 strcat(tmp_str, strerror((errno));
                 strcat(tmp_str, "\n");
                 prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                         tmp_str);
                 tape_error_code = errno;
              } else {
                 if ( !ioctl (sffd, SIOC_REQSENSE, &sense_data) ) {
                    data = (uchar *)&sense_data;
                    if ( data[2] == 0x06 )
                       continue_loop = 0;
                    else {
                       if ( data[2] == 0x00 ) {
                          rc = 55;
                          continue_loop = 1;
                       } else {
                          continue_loop = 1;
                          phtx_info->bad_others = phtx_info->bad_others + 1;
                          sprintf(msg_str, "diagnostics request sense error -\n"
                                           "status validity = %x  scsi bus "
                                           "status = %x  adapter status = %x\n",
                                  iocmd_buf.status_validity,
                                  iocmd_buf.scsi_bus_status,
                                  iocmd_buf.adapter_status);
                          prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                                  HARD, msg_str);
                       }
                    }
                 } else {
                    continue_loop = 1;
                    phtx_info->bad_others = phtx_info->bad_others + 1;
                    strcpy(tmp_str, "TimberWolf Request Sense Error - ");
                    strcat(tmp_str, strerror((errno));
                    strcat(tmp_str, "\n");
                    prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                            HARD, tmp_str);
                 }
                 rc = close(sffd);
                 if ( rc < 0 ) {
                    strcpy(tmp_str, "Close error on ");
                    strcat(tmp_str, phtx_info->sdev_id);
                    strcat(tmp_str, " at end of tape unload rule.\n");
                    strcat(tmp_str, strerror((errno));
                    strcat(tmp_str, "\n");
                    hxfmsg(phtx_info, errno, HARD, tmp_str);
                    tape_error_code = errno;
                 }
              }
           }
        }
     }
  }
  if ( rc == 55 )
     rc = 0;
  else
     rc = -1;
  return(rc);
}

/**************************************************************************/
/* unload_write -- timberwolf write end of tape and then unload tape and  */
/* begin again                                                            */
/**************************************************************************/
int
unload_write(struct htx_data *phtx_info, struct ruleinfo *prule_info,
             int loop, struct blk_num_typ *pblk_num, char *wbuf)
{
  int    rc, sffd, continue_loop = 0;
  char   tmp_str[120];
  uchar  *data;
  struct stop tapeinfo;
  struct request_sense sense_data;

 while ( continue_loop == 0 ) {
    while ( (rc = write(prule_info->fildes, wbuf,
                        (prule_info->num_blks * BLK_SIZE))) != 1 ) {
       lseek(prule_info->fildes, 0, 1);
       phtx_info->good_writes = phtx_info->good_writes + 1;
       phtx_info->bytes_writ = phtx_info->bytes_writ + rc;
       update_blk_nums(pblk_num,rc);    /* update number of blocks pointers */
       hxfupdate(UPDATE,phtx_info);
    }
    if ( errno != ENXIO ) {
       continue_loop = 1;
       phtx_info->bad_writes = phtx_info->bad_writes + 1;
       strcpy(tmp_str, "TimberWolf Write EOT Error - ");
       strcat(tmp_str, strerror((errno));
       strcat(tmp_str, "\n");
       prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
       tape_error_code = errno;
    } else if ( (errno == EBADF) && (Open_Mode == O_RDONLY) ) {
       continue_loop = 1;
       strcpy(tmp_str, "TimberWolf Write Error - Write Protected Media - ");
       strcat(tmp_str, strerror((errno));
       strcat(tmp_str, "\n");
       prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                    tmp_str);
       tape_error_code = errno;
    } else {
       tapeinfo.st_op = STWEOF;         /* write an EOF mark at end of tape */
       tapeinfo.st_count = 1;
       rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
       if ( rc == -1 ) {
          continue_loop = 1;
          phtx_info->bad_writes = phtx_info->bad_writes + 1;
          strcpy(tmp_str, "TimberWolf Write EOF Error after WEOT - ");
          strcat(tmp_str, strerror((errno));
          strcat(tmp_str, "\n");
          prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
          tape_error_code = errno;
       } else {
          tapeinfo.st_op = STOFFL;
          rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
          if ( rc == -1 ) {
             continue_loop = 1;
             phtx_info->bad_writes = phtx_info->bad_writes + 1;
             strcpy(tmp_str, "TimberWolf Tape UnLoad Error - ");
             strcat(tmp_str, strerror((errno));
             strcat(tmp_str, "\n");
             prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                     tmp_str);
             tape_error_code = errno;
          } else {
             sleep(prule_info->u_sleep);
             if ( (sffd = open(prule_info->chs_file, O_RDWR | O_NDELAY)) < 0 ) {
                strcpy(tmp_str, "TimberWolf Open Error - ");
                strcat(tmp_str, strerror((errno));
                strcat(tmp_str, "\n");
                prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                        tmp_str);
                tape_error_code = errno;
             } else {
                if ( !ioctl(sffd, SIOC_REQSENSE, &sense_data) ) {
                   data = (uchar *)&sense_data;
                   if ( data[2] == 0x06 )
                      continue_loop = 0;
                   else {
                      if ( data[2] == 0x00 ) {
                         rc = 55;
                         continue_loop = 1;
                      } else {
                         continue_loop = 1;
                         phtx_info->bad_others = phtx_info->bad_others + 1;
                         sprintf(msg_str, "diagnostics request sense error - \n"
                                          "status validity = %x   scsi bus "
                                          "status = %x   adapter status = %x\n",
                                 iocmd_buf.status_validity,
                                 iocmd_buf.scsi_bus_status,
                                 iocmd_buf.adapter_status);
                         prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                                 HARD, msg_str);
                         tape_error_code = errno;
                      }
                   }
                } else {
                   continue_loop = 1;
                   phtx_info->bad_others = phtx_info->bad_others + 1;
                   strcpy(tmp_str, "TimberWolf Request Sense Error - ");
                   strcat(tmp_str, strerror(errno));
                   strcat(tmp_str, "\n");
                   prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                           HARD, tmp_str);
                   tape_error_code = errno;
                }
                rc = close(sffd);
                if ( rc < 0 ) {
                   strcpy(tmp_str, "Close error on ");
                   strcat(tmp_str, phtx_info->sdev_id);
                   strcat(tmp_str, " at end of move medium rule.\n");
                   strcat(tmp_str, strerror((errno));
                   strcat(tmp_str, "\n");
                   hxfmsg(phtx_info, errno, HARD, tmp_str);
                   tape_error_code = errno;
                }
             }
          }
       }
    }
 }
 if ( rc == 55 ) {
    rc = 0;
 } else
    rc = -1;
 return(rc);
}

/**************************************************************************/
/* cdmv_tape -- cdatwolf move medium                                      */
/**************************************************************************/
int
cdmv_tape(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          int loop, struct blk_num_typ *pblk_num)
{
  int    count, rc, i, sffd;
  char   tmp_str[120];
  uchar  *data;
  struct stop tapeinfo;
  struct move_medium move_medium;

  if ( prule_info->source_id == 31 || prule_info->source_id == 32 ) {
     tapeinfo.st_op = STOFFL;
     rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
     if ( rc == -1 ) {
        phtx_info->bad_others = phtx_info->bad_others + 1;
        strcpy(tmp_str, "CdatWolf Error in UnLoad Command - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     } else
        sleep(10);
  }
  if ( tape_error_code == 0 ) {
     if ( (sffd = open(prule_info->chs_file, O_RDWR | O_NDELAY)) < 0 ) {
        strcpy(tmp_str, "CdatWolf Open Error - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     } else {
        move_medium.source = prule_info->source_id;
        move_medium.destination = prule_info->dest_id;
        move_medium.robot = 0;
        move_medium.invert = 0;
        if ( !ioctl(sffd, 0x80074304, &move_medium) ) {
           sprintf(msg_str, "Move Medium succeeded\n");
           hxfmsg(phtx_info, 0, INFO, msg_str);
        } else {
           phtx_info->bad_others = phtx_info->bad_others + 1;
           strcpy(tmp_str, "CdatWolf Move Medium Error - ");
           strcat(tmp_str, strerror(errno));
           strcat(tmp_str, "\n");
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
           tape_error_code = errno;
        }
        rc = close(sffd);
        if ( rc < 0 ) {
           strcpy(tmp_str, "Close error on ");
           strcat(tmp_str, phtx_info->sdev_id);
           strcat(tmp_str, " at end of move medium rule.\n");
           strcat(tmp_str, strerror(errno));
           strcat(tmp_str, "\n");
           hxfmsg(phtx_info, errno, HARD, tmp_str);
           tape_error_code = errno;
        }
     }
  }
  if ( tape_error_code != 0 )
     rc = -1;
  else
     rc = 0;
  return(rc);
}

/**************************************************************************/
/* cdrd_stat -- cdatwolf read element status                              */
/**************************************************************************/
int
cdrd_stat(struct htx_data *phtx_info, struct ruleinfo *prule_info,
          int loop, struct blk_num_typ *pblk_num)
{
  int    count, rc, i, j, sffd;
  char   tmp_str[512];
  uchar  *data;
  struct inventory inventory;
  struct element_info element_info;
  struct telement_status robot_status[1];
  struct telement_status slot_status[30];
  struct telement_status ie_status[1];
  struct telement_status drive_status[2];

  if ( (sffd = open(prule_info->chs_file, O_RDWR|O_NDELAY)) < 0 ) {
     strcpy(tmp_str, "CdatWolf Open Error - ");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     bzero( (caddr_t)robot_status, sizeof(struct telement_status) );
     for ( i = 0; i < 30; i++ )
        bzero( (caddr_t)(&slot_status[i]), sizeof(struct telement_status) );
     bzero( (caddr_t)ie_status, sizeof(struct telement_status) );
     for ( i = 0; i < 2; i++ )
        bzero( (caddr_t)(&drive_status[i]), sizeof(struct telement_status) );
     if ( !ioctl(sffd, 0x40104303, &element_info) ) {
        sprintf(msg_str, "Element Information command succeeded \n");
        hxfmsg(phtx_info, 0, INFO, msg_str);
        inventory.robot_status = robot_status;
        inventory.slot_status = slot_status;
        inventory.ie_status = ie_status;
        inventory.drive_status = drive_status;
        if ( !ioctl(sffd, 0x80104307, &inventory) ) {
           sprintf(msg_str, "Inventory Information command succeeded \n");
           hxfmsg(phtx_info, 0, INFO, msg_str);
           count = sizeof(struct telement_status);
           data = (uchar *)&inventory.robot_status[0];
           strcpy(msg_str, " Robot Status Data:\n");
           for ( i = 0; i < count; i++ ) {
              if ( (i & 15) == 0 ) {
                 sprintf(tmp_str, "\n [%02X]", i);
                 strcat(msg_str, tmp_str);
              }
              sprintf(tmp_str, " %02X", data[i]);
              strcat(msg_str, tmp_str);
           }
           hxfmsg(phtx_info, 0, INFO, msg_str);
           for ( j = 0; j < element_info.slots; j++ ) {
              data = (uchar *)&inventory.slot_status[j];
              sprintf(tmp_str, " Slot %d Status Data:\n", j+1);
              strcpy(msg_str, tmp_str);
              for ( i = 0; i < count; i++ ) {
                 if ( (i & 15) == 0 ) {
                    sprintf(tmp_str, "\n [%02X]", i);
                    strcat(msg_str, tmp_str);
                 }
                 sprintf(tmp_str, " %02X", data[i]);
                 strcat(msg_str, tmp_str);
              }
              hxfmsg(phtx_info, 0, INFO, msg_str);
           }
           data = (uchar *)&inventory.ie_status[0];
           strcpy(msg_str, " IE Status Data:\n");
           for ( i = 0; i < count; i++ ) {
              if ( (i & 15) == 0 ) {
                 sprintf(tmp_str, "\n [%02X]", i);
                 strcat(msg_str, tmp_str);
              }
              sprintf(tmp_str, " %02X", data[i]);
              strcat(msg_str, tmp_str);
           }
           hxfmsg(phtx_info, 0, INFO, msg_str);
           for ( j = 0; j < element_info.drives; j++ ) {
              data = (uchar *)&inventory.drive_status[j];
              sprintf(tmp_str, " Drive %d Status Data:\n", j+1);
              strcpy(msg_str, tmp_str);
              for ( i = 0; i < count; i++ ) {
                 if ( (i & 15) == 0 ) {
                    sprintf(tmp_str, "\n [%02X]", i);
                    strcat(msg_str, tmp_str);
                 }
                 sprintf(tmp_str, " %02X", data[i]);
                 strcat(msg_str, tmp_str);
              }
              hxfmsg(phtx_info, 0, INFO, msg_str);
           }
        } else {
           phtx_info->bad_others = phtx_info->bad_others + 1;
           strcpy(tmp_str, "CdatWolf Element Status Error - ");
           strcat(tmp_str, strerror(errno));
           strcat(tmp_str, "\n");
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
           tape_error_code = errno;
        }
     } else {
        phtx_info->bad_others = phtx_info->bad_others + 1;
        strcpy(tmp_str, "CdatWolf Element Status Error - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     }
     rc = close(sffd);
     if ( rc == -1 ) {
        strcpy(tmp_str, "Close error on ");
        strcat(tmp_str, phtx_info->sdev_id);
        strcat(tmp_str, " at end of read element status rule.\n");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        hxfmsg(phtx_info, errno, HARD, tmp_str);
        tape_error_code = errno;
     }
  }
  if ( tape_error_code != 0 )
     rc = -1;
  else
     rc = 0;
  return(rc);
}

/**************************************************************************/
/* himove -- hidalgo move medium command                                  */
/**************************************************************************/
int
himove(struct htx_data *phtx_info, struct ruleinfo *prule_info,
       int loop, struct blk_num_typ *pblk_num)
{
  int    i, rc;
  char   tmp_str[120];
  uchar  *data;

  rc = close(prule_info->fildes);
  if ( rc == -1 ) {
     strcpy(tmp_str, "Close error on ");
     strcat(tmp_str, phtx_info->sdev_id);
     strcat(tmp_str, " in Hidalgo Move Medium Command.\n");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     hxfmsg(phtx_info, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     prule_info->fildes = openx(phtx_info->sdev_id, Open_Mode, 0777,
                                SC_DIAGNOSTIC);
     if ( prule_info->fildes == -1 ) {
        strcpy(tmp_str, "Hidalgo Open Error - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     } else {
                            /*--- send a move medium command ---*/
		if(found_fscsi == 1) {
			memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));
			fiocmd_buf.timeout_value = 120;
			fiocmd_buf.command_length = 12;
			fiocmd_buf.scsi_cdb[0] = 0xa5;
			fiocmd_buf.scsi_cdb[5] = (uint)prule_info->source_id;
			fiocmd_buf.scsi_cdb[7] = (uint)prule_info->dest_id;
			rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
		}
		else {
			memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));
			iocmd_buf.timeout_value = 120;
			iocmd_buf.command_length = 12;
			iocmd_buf.scsi_cdb[0] = 0xa5;
			iocmd_buf.scsi_cdb[5] = (uint)prule_info->source_id;
			iocmd_buf.scsi_cdb[7] = (uint)prule_info->dest_id;
			rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);

		}
		if ( rc == -1 ) {
			phtx_info->bad_others = phtx_info->bad_others + 1;
			strcpy(tmp_str, "Hidalgo Move Medium Error - ");
			strcat(tmp_str, strerror(errno));
			strcat(tmp_str, "\n");
			prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
													HARD, tmp_str);
			tape_error_code = errno;
		} else {
		if(	found_fscsi == 1) {
			memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));
			fiocmd_buf.timeout_value = 120;
			fiocmd_buf.command_length = 12;
			fiocmd_buf.scsi_cdb[0] = 0xa5;
			fiocmd_buf.scsi_cdb[5] = (uint)prule_info->source_id1;
			fiocmd_buf.scsi_cdb[7] = (uint)prule_info->dest_id1;
			rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
		}
		else {
			memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));
			iocmd_buf.timeout_value = 120;
			iocmd_buf.command_length = 12;
			iocmd_buf.scsi_cdb[0] = 0xa5;
			iocmd_buf.scsi_cdb[5] = (uint)prule_info->source_id1;
			iocmd_buf.scsi_cdb[7] = (uint)prule_info->dest_id1;
			rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);
		}
		if ( rc == -1 ) {
			phtx_info->bad_others = phtx_info->bad_others + 1;
			strcpy(tmp_str, "Hidalgo Move Medium Error - ");
			strcat(tmp_str, strerror(errno));
			strcat(tmp_str, "\n");
			prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
													HARD, tmp_str);
			tape_error_code = errno;
		} else {
			rc = close(prule_info->fildes);
			if ( /*rc == -1*/ 0) {
				strcpy(tmp_str, "Close error on ");
				strcat(tmp_str, phtx_info->sdev_id);
				strcat(tmp_str, " at end of Hidalgo Move Medium rule.\n");
				strcat(tmp_str, strerror(errno));
				strcat(tmp_str, "\n");
				hxfmsg(phtx_info, errno, HARD, tmp_str);
				tape_error_code = errno;
			} else {
				prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
				if ( prule_info->fildes == -1 ) {
					strcpy(tmp_str, "Hidalgo Open Error - ");
					strcat(tmp_str, strerror((errno));
					strcat(tmp_str, "\n");
					prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
																		tmp_str);
					tape_error_code = errno;
				}
			}
		}
		}
	}
	}
	if ( tape_error_code != 0 )
		rc = -1;
	else
		rc = 0;
	return(rc);
}
/**************************************************************************/
/* hielem -- hidalgo read element status command                          */
/**************************************************************************/
int hielem(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
       struct blk_num_typ *pblk_num)
{
  int    i, rc;
  char   sns_buf[220], tmp_str[220];

  rc = close(prule_info->fildes);
  if ( /*rc == -1*/ 0 ) {
     strcpy(tmp_str, "Close error on ");
     strcat(tmp_str, phtx_info->sdev_id);
     strcat(tmp_str, " in Hidalgo Read Element Status Command.\n");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     hxfmsg(phtx_info, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     prule_info->fildes = openx(phtx_info->sdev_id, Open_Mode, 0777,
                                SC_DIAGNOSTIC);
     if ( prule_info->fildes == -1 ) {
        strcpy(tmp_str, "Hidalgo Open Error - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     } else {
                             /*--- send a read element status command ---*/
		if(found_fscsi == 1) {
	        memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));
    	    fiocmd_buf.timeout_value = 30;
        	fiocmd_buf.flags = B_READ;
	        fiocmd_buf.data_length = (prule_info->num_blks * 0x10) + 0x18;
    	    fiocmd_buf.buffer = sns_buf;
	        fiocmd_buf.command_length = 12;
    	    fiocmd_buf.scsi_cdb[0] = 0xb8;
        	fiocmd_buf.scsi_cdb[3] = prule_info->source_id;
	        fiocmd_buf.scsi_cdb[5] = prule_info->num_blks;
    	    fiocmd_buf.scsi_cdb[9] = (prule_info->num_blks * 0x10) + 0x18;
        	rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
		}
		else {
			memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));
    	    iocmd_buf.timeout_value = 30;
        	iocmd_buf.flags = B_READ;
	        iocmd_buf.data_length = (prule_info->num_blks * 0x10) + 0x18;
    	    iocmd_buf.buffer = sns_buf;
	        iocmd_buf.command_length = 12;
    	    iocmd_buf.scsi_cdb[0] = 0xb8;
        	iocmd_buf.scsi_cdb[3] = prule_info->source_id;
	        iocmd_buf.scsi_cdb[5] = prule_info->num_blks;
    	    iocmd_buf.scsi_cdb[9] = (prule_info->num_blks * 0x10) + 0x18;
        	rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);
		}
        if ( rc == -1 ) {
           strcpy(tmp_str, "Hidalgo Read Element Status Error - ");
           strcat(tmp_str, strerror(errno));
           strcat(tmp_str, "\n");
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                   HARD, tmp_str);
           tape_error_code = errno;
        } else {
           strcpy(msg_str, "   Read Element Status Data:");
           for ( i = 0; i < ((found_fscsi ==1)?fiocmd_buf.scsi_cdb[9]:iocmd_buf.scsi_cdb[9]); i++ ) {
              if ( (i & 15) == 0 ) {
                 sprintf(tmp_str, "\n    [%02X]", i);
                 strcat(msg_str,tmp_str);
              }
              sprintf(tmp_str, "%02X", sns_buf[i]);
              strcat(msg_str, tmp_str);
           }
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                   INFO, msg_str);
           rc = close(prule_info->fildes);
           if ( /*rc == -1*/ 0 ) {
              strcpy(tmp_str, "Close error on ");
              strcat(tmp_str, phtx_info->sdev_id);
              strcat(tmp_str, " at end of Hidalgo Read Element rule.\n");
              strcat(tmp_str, strerror(errno));
              strcat(tmp_str, "\n");
              hxfmsg(phtx_info, errno, HARD, tmp_str);
              tape_error_code = errno;
           } else {
              prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
              if ( prule_info->fildes == -1 ) {
                 strcpy(tmp_str, "Hidalgo Open Error - ");
                 strcat(tmp_str, strerror(errno));
                 strcat(tmp_str, "\n");
                 prt_msg(phtx_info,prule_info, loop, pblk_num, errno, HARD,
                         tmp_str);
                 tape_error_code = errno;
              }
           }
        }
     }
  }
  if ( tape_error_code != 0 )
     rc = -1;
  else
     rc = 0;
  return(rc);
}

/**************************************************************************/
/* hiinit -- hidalgo initialize element command                           */
/**************************************************************************/
int hiinit(struct htx_data *phtx_info, struct ruleinfo *prule_info, int loop,
       struct blk_num_typ *pblk_num)
{
  int    i, rc;
  char   tmp_str[120];

  rc = close(prule_info->fildes);
  if ( rc == -1 ) {
     strcpy(tmp_str, "Close error on ");
     strcat(tmp_str, phtx_info->sdev_id);
     strcat(tmp_str, " in Hidalgo Initialize Element Command.\n");
     strcat(tmp_str, strerror(errno));
     strcat(tmp_str, "\n");
     hxfmsg(phtx_info, errno, HARD, tmp_str);
     tape_error_code = errno;
  } else {
     prule_info->fildes = openx(phtx_info->sdev_id, Open_Mode, 0777,
                                SC_DIAGNOSTIC);
     if ( prule_info->fildes == -1 ) {
        strcpy(tmp_str, "Hidalgo Open Error - ");
        strcat(tmp_str, strerror(errno));
        strcat(tmp_str, "\n");
        prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
        tape_error_code = errno;
     } else {
                            /*--- send an initialize element command ---*/
		if(found_fscsi == 1) {
	        memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));
    	    fiocmd_buf.timeout_value = 30;
        	fiocmd_buf.command_length = 6;
	        fiocmd_buf.scsi_cdb[0] = 0x07;
    	    rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
		}
		else {
		    memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));
    	    iocmd_buf.timeout_value = 30;
        	iocmd_buf.command_length = 6;
	        iocmd_buf.scsi_cdb[0] = 0x07;
    	    rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);

		}
        if ( rc == -1 ) {
           strcpy(tmp_str, "Hidalgo Initialize Element Status Error - ");
           strcat(tmp_str, strerror(errno));
           strcat(tmp_str, "\n");
           prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                   HARD, tmp_str);
           tape_error_code = errno;
        } else {
           rc = close(prule_info->fildes);
           if ( /*rc == -1*/ 0 ) {
              strcpy(tmp_str, "Close error on ");
              strcat(tmp_str, phtx_info->sdev_id);
              strcat(tmp_str, " at end of Hidalgo Init Element rule.\n");
              strcat(tmp_str, strerror(errno));
              strcat(tmp_str, "\n");
              hxfmsg(phtx_info, errno, HARD, tmp_str);
           } else {
              prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
              if ( prule_info->fildes == -1 ) {
                 strcpy(tmp_str, "Hidalgo Open Error - ");
                 strcat(tmp_str, strerror(errno));
                 strcat(tmp_str, "\n");
                 prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                         tmp_str);
                 tape_error_code = errno;
              }
           }
        }
     }
  }
  if ( tape_error_code != 0 )
     rc = -1;
  else
     rc = 0;
  return(rc);
}

/**************************************************************************/
/* hidal_unload -- hidalgo write EOT and then unload tape and begin again */
/**************************************************************************/
int
hidal_unload(struct htx_data *phtx_info, struct ruleinfo *prule_info,
             int loop, struct blk_num_typ *pblk_num, char *wbuf)
{
  int    i, rc, continue_loop = 0;
  char   tmp_str[120];
  struct stop tapeinfo;

 while ( continue_loop == 0 ) {
    while ( (rc = write(prule_info->fildes, wbuf,
                        (prule_info->num_blks * BLK_SIZE))) >= 0 ) {
       lseek(prule_info->fildes, 0, 1);
       phtx_info->good_writes = phtx_info->good_writes + 1;
       phtx_info->bytes_writ = phtx_info->bytes_writ + rc;
       update_blk_nums(pblk_num,rc);    /* update number of blocks pointers */
       hxfupdate(UPDATE,phtx_info);
    }
    if ( errno != ENXIO ) {
       continue_loop = 1;
       phtx_info->bad_writes = phtx_info->bad_writes + 1;
       strcpy(tmp_str, "Hidalgo Write EOT Error - ");
       strcat(tmp_str, strerror(errno));
       strcat(tmp_str, "\n");
       prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD, tmp_str);
       tape_error_code = errno;
    } else if ( (errno == EBADF) && (Open_Mode == O_RDONLY) ) {
       continue_loop = 1;
       strcpy(tmp_str, "Hidalgo Write Error due to Write-Protected Media - ");
       strcat(tmp_str, strerror(errno));
       strcat(tmp_str, "\n");
       prt_msg_asis(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                    tmp_str);
       tape_error_code = errno;
    } else {
       tapeinfo.st_op = STWEOF;         /* write an EOF mark at end of tape */
       tapeinfo.st_count = 1;
       rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
       if ( rc == -1 ) {
          continue_loop = 1;
          phtx_info->bad_writes = phtx_info->bad_writes + 1;
          prt_msg(phtx_info,prule_info,loop,pblk_num,errno,HARD,
                  "Write EOF err after WEOT - ");
          tape_error_code = errno;
       } else {
          tapeinfo.st_op = STOFFL;
          rc = ioctl(prule_info->fildes, STIOCTOP, &tapeinfo);
          if ( rc == -1 ) {
             continue_loop = 1;
             phtx_info->bad_writes = phtx_info->bad_writes + 1;
             strcpy(tmp_str, "Hidalgo Tape UnLoad Error - ");
             strcat(tmp_str, strerror(errno));
             strcat(tmp_str, "\n");
             prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                     tmp_str);
             tape_error_code = errno;
          } else {
             sleep(prule_info->u_sleep);
             rc = close(prule_info->fildes);
             prule_info->fildes = openx(phtx_info->sdev_id, Open_Mode, 0777,
                                        SC_DIAGNOSTIC);
             if ( prule_info->fildes == -1 ) {
                continue_loop = 1;
                strcpy(tmp_str, "Hidalgo Open Error - ");
                strcat(tmp_str, strerror(errno));
                strcat(tmp_str, "\n");
                prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                        tmp_str);
                tape_error_code = errno;
             } else {
				if(found_fscsi == 1) {
	                memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));
    	            fiocmd_buf.data_length = sizeof(sense_buf);
        	        fiocmd_buf.buffer = sense_buf;
            	    fiocmd_buf.timeout_value = 30;
                	fiocmd_buf.flags = B_READ;
	                fiocmd_buf.command_length = 6;
    	            fiocmd_buf.scsi_cdb[0] = 3;
        	        fiocmd_buf.scsi_cdb[4] = MAX_SENSE;
            	    rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
				}
				else {
				    memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));
    	            iocmd_buf.data_length = sizeof(sense_buf);
        	        iocmd_buf.buffer = sense_buf;
            	    iocmd_buf.timeout_value = 30;
                	iocmd_buf.flags = B_READ;
	                iocmd_buf.command_length = 6;
    	            iocmd_buf.scsi_cdb[0] = 3;
        	        iocmd_buf.scsi_cdb[4] = MAX_SENSE;
            	    rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);
				}
                if ( rc == -1 ) {
                   continue_loop = 1;
                   phtx_info->bad_others = phtx_info->bad_others + 1;
                   strcpy(tmp_str, "Hidalgo Request Sense Error - ");
                   strcat(tmp_str, strerror(errno));
                   strcat(tmp_str, "\n");
                   prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                           HARD, tmp_str);
                   tape_error_code = errno;
                } else {
                   if ( sense_buf[2] == 0x40 )
                      continue_loop = 0;
                   else {
                      if ( sense_buf[2] == 0x00 ) {
                         rc = 55;
                         continue_loop = 1;
						 if(found_fscsi == 1) {
	                         memset(&fiocmd_buf, 0, sizeof(struct scsi_iocmd));
    	                     fiocmd_buf.timeout_value = 120;
        	                 fiocmd_buf.command_length = 12;
            	             fiocmd_buf.scsi_cdb[0] = 0xa5;
                	         fiocmd_buf.scsi_cdb[5] = (uint)prule_info->source_id;
                    	     fiocmd_buf.scsi_cdb[7] = (uint)prule_info->dest_id;
                        	 rc = ioctl(prule_info->fildes, STIOCMD, &fiocmd_buf);
						 }
						 else {
	                         memset(&iocmd_buf, 0, sizeof(struct sc_iocmd));
    	                     iocmd_buf.timeout_value = 120;
        	                 iocmd_buf.command_length = 12;
            	             iocmd_buf.scsi_cdb[0] = 0xa5;
                	         iocmd_buf.scsi_cdb[5] = (uint)prule_info->source_id;
                    	     iocmd_buf.scsi_cdb[7] = (uint)prule_info->dest_id;
                        	 rc = ioctl(prule_info->fildes, STIOCMD, &iocmd_buf);
						 }
                         if ( rc == -1 ) {
                            continue_loop = 1;
                            strcpy(tmp_str, "Hidalgo Move Medium Error - ");
                            strcat(tmp_str, strerror(errno));
                            strcat(tmp_str, "\n");
                            prt_msg(phtx_info, prule_info, loop, pblk_num,
                                    errno, HARD, tmp_str);
                            tape_error_code = errno;
                         }
                      } else {
                         continue_loop = 1;
                         phtx_info->bad_others = phtx_info->bad_others + 1;
                         strcpy(tmp_str, "Hidalgo Request Sense Error - ");
                         strcat(tmp_str, strerror(errno));
                         strcat(tmp_str, "\n");
                         prt_msg(phtx_info, prule_info, loop, pblk_num, errno,
                                 HARD, tmp_str);
                         tape_error_code = errno;
                      }
                   }
                }
             }
             rc = close(prule_info->fildes);
             if ( /*rc == -1*/ 0 ) {
                continue_loop = 1;
                strcpy(tmp_str, "Close error on ");
                strcat(tmp_str, phtx_info->sdev_id);
                strcat(tmp_str, " at end of Request Sense rule.\n");
                strcat(tmp_str, strerror(errno));
                strcat(tmp_str,"\n");
                hxfmsg(phtx_info, errno, HARD, tmp_str);
                tape_error_code = errno;
             } else {
                prule_info->fildes = open(phtx_info->sdev_id,Open_Mode);
                if ( prule_info->fildes == -1 ) {
                   continue_loop = 1;
                   strcpy(tmp_str, "Hidalgo Open Error - ");
                   strcat(tmp_str, strerror(errno));
                   strcat(tmp_str, "\n");
                   prt_msg(phtx_info, prule_info, loop, pblk_num, errno, HARD,
                           tmp_str);
                   tape_error_code = errno;
                }
             }
          }
       }
    }
 }
 if ( rc == 55 ) {
    rc = 0;
 }
    rc = -1;
 return(rc);
}
#endif
/**************************************************************************/
/* Execute a system command from a pseudo command line                    */
/**************************************************************************/
int
do_cmd(struct htx_data *phtx_info, struct ruleinfo *prule_info,
       struct blk_num_typ *pblk_num)
{
   int    a, b, c, d, rc = 0, filedes;
   char   dev_type[40], tmsg[600], cmd_line[300], msg[650];
   char   filenam[30] = "/tmp/cmdout.";
/*   struct ruleinfo *pr;*/

   dev_type[0]= '\0';
   rc = close(prule_info->fildes);
   if ( /*rc == -1*/ 0 ) {
      sprintf(msg, "Close Error on %s in do_cmd() operation:\n",
              phtx_info->sdev_id);
      strcat(msg, strerror(errno));
      strcat(msg, "\n");
      hxfmsg(phtx_info, errno, HARD, msg);
      tape_error_code = errno;
   } else {
      b = 0;
      for ( a = 0; a <= strlen(prule_info->cmd_list); a++ ) {
          cmd_line[b] = prule_info->cmd_list[a];
          if ( cmd_line[b] == '$' ) {
             a++;
             if ( prule_info->cmd_list[a] == 'D' ||
                  prule_info->cmd_list[a] == 'P' ||
                  prule_info->cmd_list[a] == 'o' ) {
                switch ( prule_info->cmd_list[a] ) {
                case 'D': { for ( c = 5; c < strlen(phtx_info->sdev_id); c++ ) {
                                  cmd_line[b] = phtx_info->sdev_id[c];
                                  b++;
                             }
                             break;
                }
                case 'P': { for ( c = 0; c < strlen(phtx_info->sdev_id); c++ ) {
                                  cmd_line[b] = phtx_info->sdev_id[c];
                                  b++;
                            }
                }
                default :  for ( c = 0; c < 12; c++ ) {
                               cmd_line[b] = filenam[c];
                               b++;
                           }
                           d = 12;
                           for ( c = 5; c < strlen(phtx_info->sdev_id); c++ ) {
                               cmd_line[b] = phtx_info->sdev_id[c];
                               filenam[d] = phtx_info->sdev_id[c];
                               b++;
                               d++;
                           }
                           filenam[d] = '\0';
                }
             } else {
                b++;
                cmd_line[b] = prule_info->cmd_list[a];
                b++;
             }
          } else
             b++;
      }
      sprintf(msg, "Command to be Executed > \n %s\n", cmd_line);
      hxfmsg(phtx_info, 0, INFO, msg);
      if ( (rc = system(cmd_line)) != 0 ) {
         if ( (filedes = open(filenam, O_RDONLY)) == -1 ) {
            sprintf(msg, "Command FAILED rc = %d > \n No Error Information "
                         "returned from command:\n %s\n",
                    rc, cmd_line);
            hxfmsg(phtx_info, -1, HARD, msg);
         } else {
            sprintf(msg, "COMMAND: %s FAILED\n with the Following Error "
                         "Information:\n", cmd_line);
            memset(tmsg, '\0', 600);
            read(filedes, tmsg, 600);
            strcat(msg, tmsg);
            close(filedes);
            sprintf(tmsg, "rm %s", filenam);
            system(msg);
            hxfmsg(phtx_info, -1, HARD, msg);
         }
      } else {
         strcpy(dev_type, "not found!\0");
         BLK_SIZE = get_dd_blksize(phtx_info->sdev_id, dev_type, phtx_info,
                                   prule_info, pblk_num);
         sprintf(msg, "  Device blocksize = %d.\n", BLK_SIZE);
         hxfmsg(phtx_info, 0, INFO, msg);
         prule_info->fildes = open(phtx_info->sdev_id, Open_Mode);
         if ( prule_info->fildes == -1 ) {
            prt_msg(phtx_info, prule_info, 1, 0, errno, HARD,
                    "Open error in do_cmd() - ");
            rc = -1;
            tape_error_code = errno;
         } else
            rc = 0;
      }
   }
   return(rc);
}
/*
 * NAME: cmp_buf()
 *
 * FUNCTION: Compares two buffers against each other.
 *
 * EXECUTION ENVIRONMENT:
 *
 *      This function is called by any Hardware Exerciser (HE) program and
 *      is included as part of the libhtx.a library.
 *
 * NOTES:
 *
 *      This routine compares two buffers and if the compare is good returns
 *      a NULL pointer.
 *
 *      If the buffers do not compare a pointer to a character string in the
 *      following format is returned:
 *
 *          miscompare at displacement (decimal) = xxxxxxxx
 *          wbuf = xxxxxxxxxxxxxxxxxxxxxxx.... (20 bytes in hex)
 *          rbuf = xxxxxxxxxxxxxxxxxxxxxxx....
 *                 |
 *                 _ Byte that did not compare
 *
 *      And the two buffers are written out to the HTX dump directory.
 *
 *
 *      operation:
 *      ---------
 *
 *      set good return code (NULL error message pointer)
 *
 *      compare buffers
 *
 *      if buffers do not compare OK
 *          set pointer to error message
 *          build error message
 *
 *          save buffers to disk
 *
 *      return(message pointer)
 *
 *
 * RETURNS:
 *
 *      Return Codes:
 *      ----------------------------------------------------------------------
 *                   0 -- Normal exit; buffers compare OK.
 *       exit_code > 0 -- Problem on compare; pointer to error msg returned.
 *
 *
 */

int
cmp_buf(struct htx_data *ps, char *wbuf, char *rbuf, size_t len, char *misc_data)
     /*
      * ps -- pointer to the HE's htx_data structure
      * wbuf -- pointer to the write buffer
      * rbuf -- pointer to the read buffer
      * len -- the length in bytes of the buffers
      */
{
#define MAX_MISCOMPARES	10
#define DUMP_PATH "/tmp/"
#define MAX_MSG_DUMP 20
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  register long i;           /* compare loop counter                         */
  register long j;           /* error message loop counter                   */

  char *msg_ptr;             /* pointer to error message (NULL if good comp) */
  char path[128];            /* dump files path                              */
  char s[3];                 /* string segment used when building error msg  */
  char work_str[512];        /* work string                                  */

  int mis_flag, rc = 0;              /* miscompare flag: boolean                     */

    /*static ushort miscompare_count = 0;*//* miscompare count         */

  /*
   ***  Beginning of Executable Code  *****************************************
   */
  path[0]= '\0';
  work_str[0] ='\0';
  msg_ptr = 0;               /* set good return code (NULL pointer)          */

  mis_flag = FALSE;          /* set miscompare flag to FALSE                 */
  i = 0;

  while ((mis_flag == FALSE) && (i < len))
  {
    if (wbuf[i] != rbuf[i])
      mis_flag = TRUE;
    else
      i++;
  } /* endwhile */

  if (mis_flag == TRUE)      /* problem with the compare?                    */
    {
      msg_ptr = misc_data;             /* show bad compare                         */
      rc = -1;
      (void) sprintf(msg_ptr, "Miscompare at displacement (decimal) = %ld ",i);

      (void) strcat(msg_ptr, "\nwbuf = ");

      for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++)
        {
          (void) sprintf(s, "%.2x", wbuf[j]);
          (void) strcat(msg_ptr, s);
        } /* endfor */

      (void) strcat(msg_ptr, "\nrbuf = ");

      for (j = i; ((j - i) < MAX_MSG_DUMP) && (j < len); j++)
        {
          (void) sprintf(s, "%.2x", rbuf[j]);
          (void) strcat(msg_ptr, s);
        } /* endfor */

      (void) strcat(msg_ptr, "\n");


      if (ps->miscompare_count < MAX_MISCOMPARES)
	{
	  /*
	   * Copy write and read buffers to dump file.
	   */
	  ps->miscompare_count++;
	  (void) strcpy(path, DUMP_PATH);
	  (void) strcat(path, "htx");
	  (void) strcat(path, &(ps->sdev_id[5]));
	  (void) strcat(path, ".wbuf");
	  (void) sprintf(work_str, "_%d_%-d",getpid(), ps->miscompare_count);
	  (void) strcat(path, work_str);

	  (void) savebuf_tofile(wbuf, len, path, ps);

	  (void) strcpy(path, DUMP_PATH);
	  (void) strcat(path, "htx");
	  (void) strcat(path, &(ps->sdev_id[5]));
	  (void) strcat(path, ".rbuf");
	  (void) strcat(path, work_str);

	  (void) savebuf_tofile(rbuf, len, path, ps);
	}
      else
	{
	  (void) sprintf(work_str, "The maximum number of saved miscompare \
buffers (%d) have already\nbeen saved.  The read and write buffers for this \
miscompare will\nnot be saved to disk.\n", MAX_MISCOMPARES);
	  (void) strcat(msg_ptr, work_str);
	} /* endif */

    } /* endif */

  return(rc);

} /* cmp_buf() */

/*
 * NAME: savebuf_tofile()
 *
 * FUNCTION: Saves a buffer to a specified file on disk.
 *
 * EXECUTION ENVIRONMENT:
 *
 *      This function is called by the cmp_buf() function on the libhtx.a
 *      library.  Typically it will be called under an HTX Hardware
 *      Exerciser (HE) process.
 *
 * NOTES:
 *
 *      operation:
 *      ---------
 *
 *      set good return code
 *
 *      if file opens OK
 *
 *          if write() goes OK
 *
 *              fsync() the file to disk
 *
 *          close file
 *
 *      return
 *
 *
 * RETURNS:
 *
 *      Return Codes:
 *      ----------------------------------------------------------------------
 *                   0 -- Normal exit; buffers compare OK.
 *       exit_code > 0 -- The errno value at the time of the error.
 *
 *
 */

int
savebuf_tofile(char *buf, size_t len, char *fname, struct htx_data *ps)
     /*
      * buf -- pointer to the buffer to be copied to disk
      * int -- the length of the buffer
      * fname -- the name of the file the buffer is to be saved to
      * ps -- pointer to the HE's htx_data structure
      */
{
  /*
   ***  Data and Functions Definitions/Declarations  **************************
   */
  char  err_msg[MAX_TEXT_MSG];  /* error message string                      */

  int   exit_code;           /* exit code                                    */
  int   fileid;              /* file id                                      */
  int   mode;                /* file mode                                    */
  int   num_bytes;           /* write() return code                          */
  int   oflag;               /* open() flag                                  */


  /*
   ***  Beginning of Executable Code  *****************************************
   */
#ifndef GOOD
#define GOOD 0
#endif
  exit_code = GOOD;          /* set good exit code                           */
  errno = 0;                 /* clear errno                                  */

  oflag = O_CREAT | O_WRONLY | O_TRUNC;
  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
                             /* set permission to -rw-r--r--                 */

  if ((fileid = open(fname, oflag, mode)) == -1)
    {
      exit_code = errno;
      (void) sprintf(err_msg, "Error opening %s.", fname);
      (void) hxfmsg(ps, exit_code, HTX_SYS_HARD_ERROR, err_msg);
    }
  else                       /* open() OK                                    */
    {
      if ((num_bytes = write(fileid, buf, (int) len)) == -1)
        {
          exit_code = errno;
          (void) sprintf(err_msg, "Error writing to %s.", fname);
          (void) hxfmsg(ps, exit_code, HTX_SYS_HARD_ERROR, err_msg);
        }
      else                       /* write() OK                               */
        {
          if (num_bytes != (int) len)
            {
              (void) sprintf(err_msg, "Error writing to %s.\nOnly %d of %ld \
bytes successfully transferred on write() system call.",
                             fname, num_bytes, len);
              (void) hxfmsg(ps, exit_code, HTX_SYS_HARD_ERROR, err_msg);
            } /* endif */

          if (fsync(fileid) != GOOD)
            {
              exit_code = errno;
              (void) sprintf(err_msg, "Error on fsync() to %s.", fname);
              (void) hxfmsg(ps, exit_code, HTX_SYS_HARD_ERROR, err_msg);
            } /* endif */

        } /* endif */

      if (close(fileid) != GOOD)
        {
          exit_code = errno;
          (void) sprintf(err_msg, "Error on close() of %s.", fname);
          (void) hxfmsg(ps, exit_code, HTX_SYS_HARD_ERROR, err_msg);
        } /* endif */

    } /* endif */

  return(exit_code);

} /* savebuf_tofile() */

#ifdef __HTX_LINUX__
int
get_scsi_devname(char *scsi_dev, char *tapename)
{
	struct scsi_mapping *tmp;
	int  len;
	char tapeno[2];

	tmp = sg_st_map;
	while(tmp->next) {
		if(strcmp(tmp->st, tapename) == 0) {
			strcpy(scsi_dev,tmp->sg);
			return 0;
		}
		tmp = tmp->next;
	}
#ifdef _DEBUG_LINUX_
	printf("get_scsi_devname::Mapping for :%s Not found, will revert to raw mode..\n",tapename);
#endif
	tmp = sg_st_map;
	len = strlen(tapename);
	while(tmp->next) {
		tapeno[0] = tapename[len-1];
		tapeno[1] = '\0';
#ifdef _DEBUG_LINUX_
		printf("get_scsi_devname:tapeno:%s\n",tapeno);
#endif
		if(strcmp(tmp->st, tapeno) == 0) {
			strcpy(scsi_dev,tmp->sg);
			return 0;
		}
		tmp = tmp->next;
	}
	return -1;
}
#endif
