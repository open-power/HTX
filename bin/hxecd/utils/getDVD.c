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

/* static char sccsid[] = "@(#)16  1.12.4.2  src/htx/usr/lpp/htx/bin/hxecd/utils/getDVD.c, exer_cd, htxubuntu 7/29/14 23:35:55"; */

/******************************************************************************
 *   COMPONENT_NAME: EXER_CD
 *
 *   MODULE NAME: getDVD.c
 *
 *   DESCRIPTION: HTX CDROM utility.
 *
 *   FUNCTIONS : main() - Determines if media in the drive is DVD-RAM.
 *
 ******************************************************************************/

#ifndef __HTX_LINUX__

#include <sys/scdisk.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/cfgodm.h>
#include <stdio.h>
#include <sys/devinfo.h>
#include <sys/cdrom.h>
#include <sys/ide.h>

#define WRITE	   0
#define NO_WRITE   1
#define NO_DEV     2
#define WRONG_DEV  3
#define BAD_ARGS   4
#define BAD_IOCTL  5
#define BAD_OPEN   6
#define BAD_RESP   7


#define MEDIA_DVD_LASTLBA 0x3fa0df

#define MAX_BLKNO 269250               /* maximum block number */

#define cd_LASTLBA      269249
#define dvd_LASTLBA     4169951

#define RETRY_OPEN		3

int get_media_info(int fd, char * device_subclass);

#else /* For Linux */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>

#define RETRY_OPEN              5
#define SCSI_OFF sizeof(struct sg_header)
#define MAX_SENSE 64

#define INQUIRY_CMD     0x12
#define INQUIRY_CMDLEN  6
#define INQUIRY_REPLY_LEN 96
#define INQUIRY_VENDOR  8
#define GETCFG_CMD      0x46

/* RETURN CODES */

#define WRITE      0
#define NO_WRITE   1
#define NO_DEV     2
#define WRONG_DEV  3
#define BAD_ARGS   4
#define BAD_IOCTL  5
#define BAD_OPEN   6
#define BAD_RESP   7
#define INVALID    -99
#define NOMEDIA	   123
#define CDMEDIA         8    /* Read  only / Write once / Re-Writable CD  */
#define DVDROMMEDIA     10   /* Read  only  media                         */
#define DVDRMEDIA       11   /* Write Once  media                         */
#define DVDRAMMEDIA     12   /* Re-Writable media                         */
#define DVDRWMEDIA      13   /* Restricted Overwrite (Re-recordable)      */

char device[25], sgname[25];
int  bus_num, scsi_id, sid_lun, sid_channel;
int  found = 0;
int  findmedia = 0, query = 0;

   struct getcfg_resp {
        int dlen;
        unsigned char r1, r2;
        unsigned short cur_profile;
        unsigned short feature_code;
        unsigned char      b30: 1;
        unsigned char      b31: 1;
        unsigned char      b32: 1;
        unsigned char      b33: 1;
        unsigned char      b34: 1;
        unsigned char      b35: 1;
        unsigned char      b36: 1;
        unsigned char      b37: 1;
        unsigned       additional_length : 8;
        int            profile_descriptors;
    } ;

int swap32(int data) {
  int s_data;

  s_data = ((data & 0xff000000) >> 24) | ((data & 0x00ff0000) >> 8) |
           ((data & 0x0000ff00) << 8) | ((data & 0x000000ff) << 24);
  return(s_data);
}

int swap16(int data) {
  int s_data;

  s_data = ((data & 0x0000ff00) >> 8) | ((data & 0x000000ff) << 8);
  return(s_data);
}


int ioctl_fn (char *sg)
{
        static  unsigned char cmd[SCSI_OFF + 18];
        unsigned int TimeOut = 3000; /* Actual value TimeOut * 10 ms = 30s */
        int fd, i;
        unsigned char cmdblk[10];
        struct sg_header *sg_hd;
        unsigned char outbuf[96 + SCSI_OFF];
        char tmp_str[1200];
        int out_size, cmd_len,in_size,status=0;
        char scsi_dev[20];
        unsigned char *tmp;
        struct getcfg_resp *sense_buf;
        int cur_prof;
        int cur_bit;

        strcpy(scsi_dev, sg);

        fd = open(scsi_dev,O_RDWR);
        if( fd < 0)     {
                printf("Open of parent dev %s failed\n", scsi_dev);
                perror("OPEN2");
                return(-1);
        }
        if (ioctl(fd ,SG_SET_TIMEOUT ,&TimeOut) < 0) {
                printf("Timeout set for %s failed\n", scsi_dev);
                perror("TIMEOUT");
                return(-1);
        }

        out_size = 128 - SCSI_OFF;
        cmd_len = 10;
        in_size = 0;

	cmdblk[0] = 0x46; /* GET CONFIGURATION COMMAND */
        cmdblk[1] = 0x02; /* only one feature required */
        cmdblk[2] = 0x00;
        cmdblk[3] = 0x00;
        cmdblk[4] = 0x00;
        cmdblk[5] = 0x00;
        cmdblk[6] = 0x00;
        cmdblk[7] = 0x00;
        cmdblk[8] = 0x80; /* 128 in Decimal */

        memcpy ( cmd + SCSI_OFF , cmdblk, sizeof(cmdblk));

        if ( query == 1 )
        {
                printf("\nDUMP of the GET CONFIGURATION command");
                printf("\nOperation code = 0x%x\n",cmd[36]);
                printf("\nLUN Reserved RT = 0x%x\n",cmd[37]);
                printf("\nStarting Feature Number SFN[0] = 0x%x\n",cmd[38]);
                printf("\nStarting Feature Number SFN[1] = 0x%x\n",cmd[39]);
                printf("\nReserved  = 0x%x\n",cmd[40]);
                printf("\nReserved  = 0x%x\n",cmd[41]);
                printf("\nReserved  = 0x%x\n",cmd[42]);
                printf("\nAllocation Length[0] = 0x%x\n",cmd[43]);
                printf("\nAllocation Length[1] = 0x%x\n",cmd[44]);
        }

	 sg_hd = (struct sg_header *) cmd;

        sg_hd->reply_len = SCSI_OFF + out_size;
        sg_hd->twelve_byte = cmd_len == 10;
        sg_hd->result = 0;

        status =  write( fd, cmd, SCSI_OFF + cmd_len + in_size );

        if ( status < 0 || status != ( SCSI_OFF + cmd_len + in_size) || sg_hd->result )
         {
                printf("SCSI cmd Write for %s failed\n", scsi_dev);
                perror("WRITE");
                return (-1);
        }

        if ( query == 1 )
                printf("\n Write Successful\n");

        status = read (fd , outbuf, SCSI_OFF + out_size);

        if ( status < 0 || status != ( SCSI_OFF + out_size) || sg_hd->result )
        {
                printf("SCSI cmd Read for %s failed\n", scsi_dev);
                perror("READ");
                return (-1);
        }

	 if ( query == 1 )
                printf("Bytes Read = %d\n", status);

        /*for(i= 36; i< sizeof(outbuf); i++)
                printf("\nRespBuf[%d] = %x", i, (unsigned char)outbuf[i]); */

          tmp = (unsigned char *) malloc(20);

//        memcpy((unsigned char *)sense_buf, outbuf + SCSI_OFF, out_size);
          memcpy(tmp, outbuf + SCSI_OFF, 16);

          sense_buf = (struct getcfg_resp *) tmp;
          close(fd);

          if ( query == 1 )
          {
                  printf("\nDUMP of the GET CONFIGURATION RESPONSE data");
	#ifdef __HTX_LE__
                  printf("\nData Length = 0x%x", swap32(sense_buf->dlen));
	#else
                  printf("\nData Length = 0x%x", sense_buf->dlen);
        #endif
	          printf("\nReserved 1  = 0x%x", sense_buf->r1);
                  printf("\nReserved 2  = 0x%x", sense_buf->r2);
	#ifdef __HTX_LE__
                  printf("\nCurrent Profile = 0x%x",swap16(sense_buf->cur_profile));
                  printf("\nFeature Code = 0x%x",swap16(sense_buf->feature_code));
	#else
                  printf("\nCurrent Profile = 0x%x",sense_buf->cur_profile);
                  printf("\nFeature Code = 0x%x",sense_buf->feature_code);
	#endif

          for(i= 0; i<16; i++)
                printf("\nRespBuf[%d] = 0x%x", i, tmp[i]);
        }

	/* Detecting the media based on the value of sense->cur_profile */

  /* to be deleted later */

          cur_bit = (int)sense_buf->b37;

#ifdef __HTX_LE__
        cur_bit = (int)sense_buf->b30;
#endif

         //printf("\n Current bit = %x\n",sense_buf->b30);
         //printf("\n Current bit = %x\n",(int)sense_buf->b30);

	/* to be deleted later */
        //   cur_bit = (int)sense_buf->b30;

       if(cur_bit == 0x0001)
        {
        //printf("\n CUR BIT IS SET, MEDIA PRESENT\n");

        /*if(swap16(sense_buf->cur_profile) == 0x0010)
          {
                //printf("\nDVD ROM MEDIA\n");

                //printf("return code from ioctl_fn = %d\n",DVDROMMEDIA);
                return(DVDROMMEDIA);
        }*/

        cur_prof = (int)sense_buf->cur_profile;



#ifdef __HTX_LE__
          cur_prof = swap16(sense_buf->cur_profile);
#endif


        if( (cur_prof == 0x0008) || (cur_prof == 0x0009) || (cur_prof == 0x000A))    /* CD MEDIA */
                return CDMEDIA;
        else if( cur_prof == 0x0010 )                                                /* DVD-ROM  */
                return DVDROMMEDIA;
        else if( cur_prof == 0x0011 )                                                /* DVD-R Write Once */
           return DVDRMEDIA;
        else if( cur_prof == 0x0012 )                                                /* DVD-RAM or DVD+RW */
           return DVDRAMMEDIA;
        else if( cur_prof == 0x0013 )                                                /* DVD-RW Restricted overwrite */
           return DVDRWMEDIA;
        else
           return INVALID;
     }
  return NOMEDIA;

}


int get_sg(char * sg)
{

int fd, rc;
struct sg_scsi_id sid;
        fd = open(sg, O_RDONLY);
        //printf("In get_sg(), fd = %d\n",fd);
        if( fd < 0) {
                printf("Open Failed\n"); //return -1;
                 printf("errno = %d\n",errno);
                 perror("open failed");
                }


        rc = ioctl(fd, SG_GET_SCSI_ID, &sid);
        if(rc < 0) {
                printf("IOCTL Failed: rc = %d\n", rc);
                 printf("errno = %d\n",errno);
                 perror("open failed");
                }

        if( (bus_num == sid.host_no) && (scsi_id == sid.scsi_id) && (sid_lun == sid.lun) && (sid_channel == sid.channel)) {
                found = 1;
                strcpy(sgname,sg);
                if ( query == 1 )
                        printf("\nDev = %s, bus %d, id %d, lun %d, ch %d\n", sg, sid.host_no, sid.scsi_id, sid.lun, sid.channel);
                }
}


int get_sr(char * sr)
{
int did[2], bus;
int fd, rc;
unsigned int busno, chno, id, lun;
char temp[10],comm[25];
int maj, min;
int retry_count = RETRY_OPEN;
char *ma, *mi;
FILE *fp;

        did[0] = 0; did[1] = 0;

	//printf("in get_sr = %s\n", sr);
	while( retry_count-- ) {
	   //printf("Retry count: %d\n",retry_count);
           if((fd = open(sr, O_RDONLY)) != -1) {
              //close(fd);
              break;
            }
           else {
              //fprintf(stderr,"Open() attempt: %d, errno = %d, %s\n",
              //    RETRY_OPEN - retry_count, errno, strerror(errno));
            }
        }

        //fd = open(sr, O_RDONLY);
        //printf("In get_sr(),fd = %d, errno = %d\n",fd, errno);

        if( fd < 0) {

                if(errno == 123)
		{
			printf("In get_sr,received err 123");
                        system("echo \"No nedia found\" > /tmp/htxdvdlog");
			return (errno);
		}
#if 0
		else if(errno == 2)
		  {
		    fd = open("/dev/scd0", O_RDONLY);
		    if(fd < 0)
			{
                	printf("Open Failed\n");
	                printf("errno = %d\n",errno);
        	        perror("open failed");
			return(errno);
			}
			fp = popen("ls -l /dev/scd0 | awk '{print $5}' ","r");
			fscanf(fp,"%s",temp);
			ma = strtok(temp,",");
			//printf("ma = %s\n",ma);
			maj = atoi(ma);
			//printf("maj = %d\n",maj);
			fp = popen("ls -l /dev/scd0 | awk '{print $6}' ","r");
			fscanf(fp,"%s",temp);
			min = atoi(temp);
			//printf("min = %d\n",min);
			//sprintf(comm,"mknod /dev/sr0 b %d %d",maj,min);
			sprintf(comm,"mknod %s b %d %d",sr,maj,min);
			system(comm);
			//printf("\n Created /dev/sr0");

			rc = ioctl(fd, SCSI_IOCTL_GET_IDLUN, did);
		        if(rc < 0) {
                		printf("IOCTL Failed: rc = %d\n", rc);
                 		printf("errno = %d\n",errno);
                		perror("get id failed");
        		}

        		rc = ioctl(fd, SCSI_IOCTL_GET_BUS_NUMBER, &bus);
        		if(rc < 0) {
                		printf("IOCTL Failed: rc = %d\n", rc);
                 		printf("errno = %d\n",errno);
                		perror("get busno failed");
        		}

        		strcpy(device,sr);
        		busno =   bus_num     = bus;
        		id    =   scsi_id     = did[0] & 0xff;
        		lun   =   sid_lun     = (did[0] >> 8) & 0xff;
        		chno  =   sid_channel = (did[0] >> 16) & 0xff;

        		if(query == 1)
                		printf("Dev = %s, bus %d, id %d, lun %d, ch %d\n", sr, busno, id, lun, chno);

			return(0);
		 }
#endif
                return (errno);

        }

        rc = ioctl(fd, SCSI_IOCTL_GET_IDLUN, did);
        if(rc < 0) {
                printf("IOCTL Failed: rc = %d\n", rc);
                 printf("errno = %d\n",errno);
                perror("get id failed");
        }

        rc = ioctl(fd, SCSI_IOCTL_GET_BUS_NUMBER, &bus);
        if(rc < 0) {
                printf("IOCTL Failed: rc = %d\n", rc);
                 printf("errno = %d\n",errno);
                perror("get busno failed");
        }

        strcpy(device,sr);
        busno =   bus_num     = bus;
        id    =   scsi_id     = did[0] & 0xff;
        lun   =   sid_lun     = (did[0] >> 8) & 0xff;
        chno  =   sid_channel = (did[0] >> 16) & 0xff;

        if(query == 1)
                printf("Dev = %s, bus %d, id %d, lun %d, ch %d\n", sr, busno, id, lun, chno);

       return 0;
}

#endif


int main(int argc, char *argv[])
{

#ifndef __HTX_LINUX__

int fd, rc, exit_code;
char	cmdbuf[32];
char    * devname;
char    device_subclass[32];
int	    findmedia = 0;
int     retry_count = 0;
struct sc_iocmd cmdb;
/* support for IDE DVD RAMs */
struct ide_atapi_passthru atapi_pt;
struct stat dev_stat;

struct devinfo info;
struct getcfg_resp {
	int dlen;
	unsigned char r1, r2;
	unsigned short cur_profile;
	unsigned short feature_code;
	unsigned       r3: 2;
	unsigned      ver: 4;
	unsigned       p0: 1;
    	unsigned  current: 1;	/* this bit means that a disk is present */
	unsigned char  flen;
	unsigned int   last_lba;
	unsigned int   block_size;
	unsigned short blocking;
	unsigned char  r4, r5;
    } buf;

    if (argc <= 1) {
        fprintf(stderr,"Usage %s: query DVD-RAM drive media\n", argv[0]);
        fprintf(stderr,"\t%s /dev/cd?\n", argv[0]);
        fprintf(stderr,"\t  The following responses are possible ( and exit codes):\n");
        fprintf(stderr,"\t  (0) WRITE - inserted media is writeable\n");
        fprintf(stderr,"\t  (1) NO_WRITE - inserted media not writeable\n");
        fprintf(stderr,"\t  (2) NO_DEV - device not available.\n");
        fprintf(stderr,"\t  (3) WRONG_DEV - device does not support write.\n");
        fprintf(stderr,"\t  BAD_ARGS(4), BAD_IOCTL(5), BAD_OPEN(6), BAD_RESP(7)  - unexpected.\n");
        fprintf(stderr,"\t%s -m /dev/cd?\n", argv[0]);
        fprintf(stderr,"\t  Detect media in DVDROM drive\n");
        fprintf(stderr,"\t  (1)MEDIA_CD, (1)MEDIA_DVD, (1)MEDIA_NONE\n");
        exit(BAD_ARGS);
    }

	devname = argv[1];
	findmedia = 0;
	if( argc == 3 ) /* Then enhanced mode */
	{
		if(strncmp(argv[1], "-m", 2) == 0)
		{
			findmedia = 1;
			devname = argv[2];
		}
	}
	/* Try open RETRY_OPEN times on device to spin it up */
	/* This is workaround for open failing just after insertion of media
       into DVDROM drive or after system reboot.  This will not be required
	   after final device qualification of Zapata */
	retry_count = RETRY_OPEN;
	while( retry_count-- ) {
		if((fd = open(devname, O_RDONLY)) != -1) {
			close(fd);
			break;
		}
		else {
			fprintf(stderr,"Open() attempt: %d, errno = %d, %s\n",
					RETRY_OPEN - retry_count, errno, strerror(errno));
 		}
	}

    if ( (fd = openx(devname, O_RDONLY, NULL, SC_DIAGNOSTIC)) == -1 ) {
		fprintf(stderr,"openx() error on %s; errno = %d %s\n",
                       devname, errno, strerror(errno));
        if (errno == ENODEV) {
			 printf("NO_DEV\n");
             exit_code = NO_DEV;
        }
        else {
            printf("BAD_OPEN\n");
            exit_code =  BAD_OPEN;
        }
		exit(exit_code);
	}
    if (ioctl(fd, IOCINFO, &info) == -1) {
		 fprintf(stderr,"ioctl() error on %s; errno = %d %s\n", devname, errno, strerror(errno));
         printf("BAD_IOCTL\n");
		 exit(BAD_IOCTL);
	}

	/* for IDE DVDRAM support */
	exit_code = get_device_subclass(&devname[5], device_subclass);
	if(exit_code)
	{
		fprintf(stderr,"get_device_subclass error on %s; errno = %d %s\n", devname, errno, strerror(errno));
		printf("BAD_RESP");
	}

	if(findmedia)
	{
		exit_code = get_media_info(fd, device_subclass);
		close(fd);
		exit(exit_code);
	}

	if(strncmp(device_subclass, "scsi", 16) == 0 )
	{
		/* build scsi command */
		bzero(cmdb, sizeof(cmdb));
		bzero(&buf, sizeof(buf));

		cmdb.data_length = sizeof(buf);
		cmdb.flags = B_READ;
		cmdb.command_length = 10;
		cmdb.timeout_value = 30;
		cmdb.buffer = (char *) &buf;
		cmdb.scsi_cdb[0] = 0x46;	/* get config */
		cmdb.scsi_cdb[1] = 0x02;	/* RT         */
		cmdb.scsi_cdb[3] = 0x20;	/* random writable ? */
		cmdb.scsi_cdb[8] = sizeof(buf);	/* alloc len  */

		rc = ioctl(fd, CDIOCMD, &cmdb);
	}
	else if(strncmp(device_subclass, "ide", 16) == 0 )
	{
		/* build ide command */
		bzero(atapi_pt, sizeof(atapi_pt));
		bzero(&buf, sizeof(buf));

		memset(&dev_stat, 0, sizeof(struct stat));
		if ( fstat(fd, &dev_stat) != 0 )
		{
			fprintf(stderr,"fstat() error; errno = %d %s\n", errno, strerror(errno));
			printf("BAD_RESP\n");
			/* may be quit */
  		}

  		memset(&atapi_pt, 0, sizeof(struct ide_atapi_passthru));
  		atapi_pt.ide_device 		= dev_stat.st_rdev & 0x1;
  		atapi_pt.flags 				= 0x4;	/* IDE_PASSTHRU_READ */
  		atapi_pt.timeout_value		= 10; /* seconds */
  		atapi_pt.buffsize			= sizeof(buf);
  		atapi_pt.data_ptr			= (char *)&buf;
  		atapi_pt.atapi_cmd.length 	= 12;
  		atapi_pt.atapi_cmd.packet.op_code = 0x46; /* get config */
  		atapi_pt.atapi_cmd.packet.bytes[0] = 0x02;
  		atapi_pt.atapi_cmd.packet.bytes[2] = 0x20;
  		atapi_pt.atapi_cmd.packet.bytes[7] = sizeof(buf);

		rc = ioctl(fd, IDEPASSTHRU, &atapi_pt); /* send command */
	}
	else
	{
		fprintf(stderr,"Invalid device sub class: %s\n",device_subclass);
		printf("BAD_RESP\n");

	}

	if (rc == -1) {
		fprintf(stderr,"ioctl() returned -1, errno = %d\n", errno);
        if (errno == EIO) {
            printf("WRONG_DEV\n");
            exit_code = WRONG_DEV;
        }
        else {
            printf("BAD_IOCTL\n");
		    exit_code = BAD_IOCTL;
        }
	}
	close(fd);
	if (exit_code != 0 ) exit(exit_code);

    if (buf.dlen >= 20) {
        if (buf.current) {
            printf("WRITE\n");
            exit_code = 0;
        }
        else {
            printf("NO_WRITE\n");
		    exit_code = 1;
        }
    }
    else { /* unexpected length */
        printf("BAD_RESP\n");
		exit_code = 7;
    }

/*
	printf("data length of response = %d\n", buf.dlen);
	printf("last lba = %4.4x\n", buf.last_lba);
	printf("profile entry  = %4.4x\n", buf.cur_profile);
	printf("writable = %2.2x\n", buf.current);
	printf("blocksize = %4.4x\n", buf.block_size);
*/

	exit(exit_code);

#else  /* code for Linux */

	int fd, rc;
        char msg[100], devname[50];
        struct stat st;
        struct sg_scsi_id sid;
        int i,r=0;
        int did[2], bus;

    if (argc <= 1) {
        fprintf(stderr,"Usage %s: query DVD-RAM drive media\n", argv[0]);
        fprintf(stderr,"\t%s /dev/cd?\n", argv[0]);
        fprintf(stderr,"\t  The following responses are possible ( and exit codes):\n");
        fprintf(stderr,"\t  (8)   CD-MEDIA  - inserted media is a CD\n");
        fprintf(stderr,"\t  (10)  DVD_ROM   - inserted media is a DVD-ROM and not writeable\n");
        fprintf(stderr,"\t  (12)  DVD-RAM   - inserted medai is a DVD-RAM and is writable\n");
        fprintf(stderr,"\t  (-99) WRONG_DEV - device unknown.\n");
	fprintf(stderr,"\t  (123) NOMEDIA   - no media present in the drive\n");
        fprintf(stderr,"\t%s -m /dev/cd?\n", argv[0]);
        fprintf(stderr,"\t  Detect media in DVDROM drive\n");
        fprintf(stderr,"\t  (1)MEDIA_CD, (1)MEDIA_DVD, (1)MEDIA_NONE\n");
        exit(BAD_ARGS);
    }

        if( argc == 3 ) /* Enhanced mode */
        {
                if(strncmp(argv[1], "-m", 2) == 0)
                {
                        findmedia = 1;
                        query = 0;
                        strcpy(devname, argv[2]);
                }
                else if(strncmp(argv[1], "-q", 2) == 0)
                {
                        findmedia = 0;
                        query = 1;
                        strcpy(devname, argv[2]);
                }
        } else{
                strcpy(devname, argv[1]);
        }

        system("rm -f /tmp/htxdvdlog &> /dev/null");

        rc = get_sr(devname);
        if( rc !=  0 )
        {
                if(rc != 123 && (query == 1 || findmedia == 1))
                     printf("\nError getting bus, channel, lun and id.\n");
		else if(rc==123 && (query == 1 || findmedia == 1))
			printf("No Media Present\n");

                exit(BAD_RESP);
        }

        /*if ( query == 1)
                printf("\nSG_MAX_QUEUE = %d\n",SG_MAX_QUEUE);*/

        /*for(i = 0; (i < SG_MAX_QUEUE) && (found == 0); i++) {*/
		for(i = 0; (i < 256) && (found == 0); i++) {
                sprintf(msg, "/dev/sg%d", i);
                rc = get_sg(msg);
                        /*if( rc != 0 )
                        {
                                printf("\nError getting bus, channel, lun and id.\n");
                                exit(BAD_RESP);
                        }*/
                }

       if(found) {
                if ( query == 1 )
                        {
                                printf("\nThe %s corresponds to %s",device,sgname);
                                printf("\nTrying to issue the GET CONFIGURATION pass through command");
                        }
                r = ioctl_fn(sgname);

                if(findmedia)   /* Print the type of media */
                {
                        if(r == CDMEDIA)
                                printf("\nTHE MEDIA IS CD_MEDIA\n");
                        else if(r == DVDROMMEDIA || r == DVDRMEDIA )
                                printf("\nTHE MEDIA IS  DVD-ROM MEDIA\n");
			else if(r == DVDRAMMEDIA || r == DVDRWMEDIA)
                                printf("\nTHE MEDIA IS  DVD-RAM MEDIA\n");
                        else
                                printf("\n MEDIA UNKNOWN or THERE IS NO DISK IN THE DRIVE\n");
                }

                //printf ("rc from getDVD2 = %d\n",r);
                return(r);
                }

        else
         {
                               if(query == 1 || findmedia == 1)
				 printf("\nError getting bus, channel, lun and id.");
                                exit(BAD_RESP);
          }


     return 0;

#endif

}

#ifndef __HTX_LINUX__

/***********************************************************************
 *
 *	get_device_subclass
 *
 *	Looks in the ODM for the subclass of the drive under test
 *	and returns its subclass.
 *
 *	INPUT:	devname - logical name of device
 *	OUTPUT: device_subclass - pointer to ODM subclass.
 *	RETURNS: 0 - OK
 *          <0 - serious failure - programming problems
 *		    >0 - odmerrno - in this case device_subclass points to the
 *			     search criteria which failed.
 *
 ***********************************************************************/
int get_device_subclass(char *devname, char *device_subclass)
{
extern int odmerrno;

int	ret_code  = 0;
struct CuDv CuDv_obj, *CuDv_obj_ptr;
struct PdDv PdDv_obj, *PdDv_obj_ptr;
char	criteria[60];

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
		strcpy(device_subclass, criteria);
		(void) odm_terminate();
		return(odmerrno);
	} /* endif */

	/* get the PdDv structure for the subclass using the link */
	sprintf(criteria, "uniquetype = '%s'",CuDv_obj.PdDvLn_Lvalue);
	PdDv_obj_ptr = odm_get_first(PdDv_CLASS,criteria,&PdDv_obj);
	ret_code = (int)PdDv_obj_ptr;
	if (!ret_code) {
	    /************ handle error here and return *********/
	    strcpy(device_subclass, criteria);
		(void) odm_terminate();
	    return(odmerrno);
	    } /* endif */

	/* return the adapter name and subclass */
	strncpy( device_subclass, PdDv_obj.subclass, 16);
	ret_code = 0;

    (void) odm_terminate();

    return(ret_code);
}



int get_media_info(int fd, char * device_subclass)
{

	struct ide_atapi_passthru atapi_pt;
	struct sc_iocmd iocmd_buf;
	char buf[8];
	int rc = 0;
	unsigned int lastlba = 0, blksize = 0;
	struct stat dev_stat;

	if(strncmp(device_subclass, "scsi", 16) == 0 )
	{ /* SCSI */

	    memset(&iocmd_buf, 0, sizeof( struct sc_iocmd));
		iocmd_buf.timeout_value = 30;
		iocmd_buf.buffer = buf;
		iocmd_buf.data_length = 8;
		iocmd_buf.flags = B_READ;
		iocmd_buf.scsi_cdb[0] = 0x25; /* READ CAPACITY */
		iocmd_buf.command_length = 10;
		rc = ioctl(fd, DKIOCMD, &iocmd_buf); /*- send command -*/
	}
	else if(strncmp(device_subclass, "ide", 16) == 0 )
	{ /* IDE */

		memset(&dev_stat, 0, sizeof(struct stat));
		if ( fstat(fd, &dev_stat) != 0 )
		{
			fprintf(stderr,"fstat() error; errno = %d %s\n", errno, strerror(errno));
			printf("BAD_RESP\n");
			/* may be quit */
  		}

  		memset(&atapi_pt, 0, sizeof(struct ide_atapi_passthru));
  		atapi_pt.ide_device 		= dev_stat.st_rdev & 0x1;
  		atapi_pt.flags 				= 0x4;	/* IDE_PASSTHRU_READ */
  		atapi_pt.timeout_value		= 10; /* seconds */
  		atapi_pt.buffsize			= 8;
  		atapi_pt.data_ptr			= buf;
  		atapi_pt.atapi_cmd.length 	= 12;
  		atapi_pt.atapi_cmd.packet.op_code = 0x25; /* READ CAPACITY */

		rc = ioctl(fd, IDEPASSTHRU, &atapi_pt); /* send command */
	}
	else
	{
		fprintf(stderr,"Invalid device sub class: %s\n",device_subclass);
		printf("BAD_RESP\n");

	}


		/* Extract the value */
	if(rc == 0)
	{
		lastlba  = *((unsigned int *) buf);
		blksize = *((unsigned int *) (buf + 4));

        /* Check for 512 bytes block size and correct lastlba */
        if( blksize == 512 ) {
            blksize = 2048;
            lastlba = lastlba / 4;
        }
        /* Check for unknown/invalid disc capacity and override it */
        if( lastlba != cd_LASTLBA && lastlba != dvd_LASTLBA ) {
            /* Force values */
            lastlba = MAX_BLKNO; /* LastLba of cd */
        }

	}

/*	printf("Device_subclass = %s, LastLBA = %d, BlkSize = %d\n",
									device_subclass, lastlba, blksize); */

	if( lastlba >= MEDIA_DVD_LASTLBA )
	{
	    printf("MEDIA_DVD\n");
	}
	else if (lastlba > 0 )
	{
		printf("MEDIA_CD\n");
	}
	else
	{
		printf("MEDIA_NONE\n");
	}

	return 1;
}

#endif
