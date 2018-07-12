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
/**************************************************************************/

#ifndef ATAPE_H
  #define ATAPE_H
  /*************************************************************************/
  /* IOCTL Structures                                                      */
  /*************************************************************************/

  #ifndef boolean
    #define boolean uchar
  #endif
  #ifndef BYTE
    #define BYTE uchar
  #endif
  #include <sys/types.h>
  #include <sys/ioctl.h>
  #include <sys/tape.h>
  /*************************************************************************
  * SCSI Tape Operations
  * STOFFL, STREW, STERASE, STWEOF, STFSF, STRSF, STFSR and STRSR perform as
  * defined in the AIX manuals. For AdStaR tape products STRETEN is a NOP
  * since they automatically perform retension on their own.
  *
  * STTUR issues a SCSI Test Unit Ready to device.
  * STLOAD issues a SCSI LOAD command
  * STSEOD issues a space forward to EOD command. After this command the
  *   tape is positioned to begin writing.
  * STFSSF issues a forward space sequential filemarks. Count is equal to
  *   the number of contiguous fiemarks to search for.
  * STRSSF issues a reverse space sequential filemarks. Count is equal to
  *   the number of contiguous fiemarks to search for.
  *IMPORTANT NOTE:
  * Any command that causes tape position to reverse. Will cause the close
  * entry point to not write file-marks. If you must position the tape
  * after writing data and before closing then you can manually write
  * file-marks with STWEOF.
  *************************************************************************
  *#define STIOCTOP        0x01             * tape commands *
  *  struct  stop
  *    {
  *    short   st_op;           * operations defined below *
  *    daddr_t st_count;        * how many of them to do(if applicable)*
  *    };
  *   * operations *
  *  #define STOFFL  5       * rewind and unload tape *
  *  #define STREW   6        * rewind *
  *  #define STERASE 7        * erase tape, leave at load point *
  *  #define STRETEN 8        * retension tape, leave at load point *
  *  #define STWEOF  10       * write an end-of-file record *
  *  #define STFSF   11       * forward space file *
  *  #define STRSF   12       * reverse space file *
  *  #define STFSR   13       * forward space record *
  *  #define STRSR   14       * reverse space record *
  ************************************************************************/

  /*************************************************************************
  * AIX SCSI Passthru command - defined in /usr/include/sys/tape.h
  *
  * #define STIOCMD         0x02
  *
  * Notes: The Atape SIOC_PASSTHRU_COMMAND ioctl should be used instead of this
  *        ioctl if possible. Support for STIOCMD is for legacy applications or
  *        applications that need run with other AIX tape drivers in addition to
  *        the Atape driver transparently.
  *
  *        This ioctl is supported on both SCSI adapter attached devices and
  *        FCP adapter attached devices.
  *
  *        For FCP adapter devices, the adapter_status field returned is
  *        converted from the FCP codes defined in /usr/include/sys/scsi_buf.h
  *        to the SCSI codes defined in /usr/include/sys/scsi.h if possible.
  *        This is to provide downward compatibility with existing applications
  *        that use the STIOCMD ioctl for SCSI attached devices.
  *
  *        This ioctl supports issuing a 16 byte SCSI-3 command to a SCSI
  *        or FCP attached device.  Use the following fields:
  *
  *           - set the command_length to 16
  *           - use the scsi_cdb[12] field for the first 12 bytes
  *           - use the resvd1 field for cdb byte 13
  *           - use the resvd2 field for cdb byte 14
  *           - use the adap_q_status field for cdb byte 15
  *           - use the q_tag_msg field for cdb byte 16
  *
  ************************************************************************/

  #define MAXSENSE						255
  #define SIOC_PASSTHRU_COMMAND         _IOWR('C',0x31,struct scsi_passthru_cmd)
  struct scsi_passthru_cmd {
  uchar  command_length;       /* Length of SCSI command 6, 10, 12 or 16      */
  uchar  scsi_cdb[16];         /* SCSI command descriptor block               */
  uint   timeout_value;        /* Timeout in seconds or 0 for command default */
  uint   buffer_length;        /* Length of data buffer or 0                  */
  char   *buffer;              /* Pointer to data buffer or NULL              */
  uint   number_bytes;         /* Number of bytes transferred to/from buffer   */
  uchar  sense_length;         /* Number of valid sense bytes                 */
  uchar  sense[MAXSENSE];      /* Sense data when sense length > 0            */
  uint   trace_length;         /* Number bytes in buffer to trace, 0 for none */
  char   read_data_command;    /* Input flag, set it to 1 for read type cmds  */
  char   reserved[27];
  };

    /*additional st_op's*/
    #define STTUR   30          /* test unit ready */
    #define STLOAD  31          /* load tape */
    #define STSEOD  32          /* space to end of data */
    #define STFSSF  33          /* forward space sequential file marks */
    #define STRSSF  34          /* reverse space sequential file marks */
    #define STERASEGAP 35       /* write erase gap on tape*/


  /*************************************************************************
  * SCSI Passthru command
  **************************************************************************
  *#define STIOCMD         0x02
  *************************************************************************/
  /*see scsi.h for structure*/

  /*************************************************************************
  * STIOCSETP changes device driver parameters
  *   The structure "stchgp" is used to set the current values.
  * STIOCQRYP queries device driver parameters
  *   The structure "stchgp" is filled in with the current values.
  * NOTE:
  * The changes made via this IOCTL are only effective during the scope
  * of the corresponding open/close they were made in. They revert to
  * default values(smit) after closing.
  *************************************************************************/
  #define STIOCSETP       _IOW('z',0x30,struct stchgp_s) /* change drive parameters */
  #define STIOCQRYP       _IOR('z',0x31,struct stchgp_s) /* query drive parameters */
    struct stchgp_s
      {
      int blksize;              /* new block size */
      struct sttrc_s
	{
	boolean trace;          /* TRUE=turn on trace */
	ulong hkwrd;            /* new value for hook word */
	} sttrc;
      int sync_count;           /* # of writes after which to sync buffers */
      boolean autoload;         /* on/off autoload feature */
      boolean buffered_mode;    /* on/off buffered mode */
      boolean compression;      /* on/off compression */
      boolean trailer_labels;   /* on/off allow writing after EOM */
      boolean rewind_immediate; /* on/off immediate rewinds */
      uchar reserved[64];
      };

  /*************************************************************************
  * This has no arguments. The tape buffers are flushed to tape.
  *************************************************************************/
  #define STIOCSYNC       _IO('z',0x37) /* synchronize buffers with tape */

  /*************************************************************************
  * STIOCDM displays a message on the 3490 display.
  *   dm_func is a flag that is built by oring the defines below.
  *************************************************************************/
  #define STIOCDM         _IOW('z',0x32,struct stdm_s) /*display message*/
    #define MAXMSGLEN     8
    struct stdm_s
      {
      char dm_func;             /* function code */
	/*Function Selection*/
	#define DMSTATUSMSG 0x00 /* General Status Msg */
	#define DMDVMSG     0x20 /* Demount/Verify Message */
	#define DMMIMMED    0x40 /* Mount With Immediate Action indicator */
	#define DMDEMIMMED  0xE0 /* Demount/Mount With Immediate Action indicator */
	/*Message Control*/
	#define DMMSG0      0x00 /* display msg 0 */
	#define DMMSG1      0x04 /* display msg 1 */
	#define DMFLASHMSG0 0x08 /* Flash msg 0 */
	#define DMFLASHMSG1 0x0C /* flash msg 1 */
	#define DMALTERNATE 0x10 /* alternate msg 0 and msg 1 */
      char dm_msg0[MAXMSGLEN];  /* message 1 */
      char dm_msg1[MAXMSGLEN];  /* message 2 */
      };

  /*************************************************************************
  * STIOCQRYPOS is used to query the position of the tape device on the
  *   tape.
  *   "block_type" indicates whether a logical position is wanted or
  *   whether a vendor specific position is wanted. The vendor specific
  *   position is a composite number that is fed into an STIOCSETPOS command
  *   to provide a much faster method of positioning. Unless a SETPOS
  *   operation is going to be used there is not much reason to use
  *   QP_PHYSICAL.
  *   "eot" is true if the tape position is after early warning.
  *   "curpos" is the position where the next write or read would take place.
  *   "lbot" is the number of the last block to be actually written from the
  *   buffer onto the tape. If no blocks have been written to tape, ie, all
  *   are still in the buffer then lbot=NONE
  *   NOTE: lbot is only valid immediately following a write command. Any tape
  *   positioning will cause this to become invalid.
  * STIOCSETPOS is used to position the tape directly to a particular block.
  *   Only "block_type" and "curpos" are used. All other fields are undefined.
  *************************************************************************/
  #define STIOCQRYPOS     _IOWR('z',0x33,struct stpos_s) /* query current tape position */
  #define STIOCSETPOS     _IOWR('z',0x34,struct stpos_s) /* set current tape position */
    typedef unsigned int blockid_t;
    struct stpos_s
      {
      char block_type;          /* format of block id information */
	#define QP_LOGICAL  0   /* scsi logical blockid format */
	#define QP_PHYSICAL 1   /* vendor specific blockid format */
      boolean eot;              /* after early warning, before physical end of tape */
      blockid_t curpos;         /* for qry current position, for set position to go to */
      blockid_t lbot;           /* last block written to tape */
	#define LBOT_NONE    0xFFFFFFFF /* no blocks have yet been written to tape */
	#define LBOT_UNKNOWN 0xFFFFFFFE /* unable to determine information*/
      uchar reserved[64];
      };

  /*************************************************************************
  * STIOCQRYSENSE returns raw sense data from device.
  *************************************************************************/
  #define MAXSENSE        255
  #define STIOCQRYSENSE   _IOWR('z',0x35,struct stsense_s)      /* retrieve sense data */
    struct stsense_s
      {
      /*INPUT*/
      char sense_type;          /* fresh(new sense) or error(last error) data */
	#define FRESH     1     /* initiate a new sense command */
	#define LASTERROR 2     /* return sense gathered from last scsi sense command*/
      /*OUTPUT*/
      uchar sense[MAXSENSE];    /* actual sense data */
      int len;                  /* length of valid sense data returned */
      uchar reserved[64];
      };

  /*************************************************************************
  * STIOCINQUIRY returns raw inquiry data from device.
  *************************************************************************/
  #define STIOCQRYINQUIRY   _IOWR('z',0x36,struct st_inquiry)   /* retrieve inquiry data */
  /*inquiry data info*/
  struct inq_data_s
    {
    BYTE b0;
    /*macros for accessing fields of byte 1*/
    #define PERIPHERAL_QUALIFIER(x)   ((x->b0 & 0xE0)>>5)
      #define PERIPHERAL_CONNECTED          0x00
      #define PERIPHERAL_NOT_CONNECTED      0x01
      #define LUN_NOT_SUPPORTED             0x03

    #define PERIPHERAL_DEVICE_TYPE(x) (x->b0 & 0x1F)
      #define DIRECT_ACCESS                 0x00
      #define SEQUENTIAL_DEVICE             0x01
      #define PRINTER_DEVICE                0x02
      #define PROCESSOR_DEVICE              0x03
      #define CD_ROM_DEVICE                 0x05
      #define OPTICAL_MEMORY_DEVICE         0x07
      #define MEDIUM_CHANGER_DEVICE         0x08
      #define UNKNOWN                       0x1F

    BYTE b1;
    /*macros for accessing fields of byte 2*/
    #define RMB(x) ((x->b1 & 0x80)>>7)              /*removable media bit  */
    #define FIXED     0
    #define REMOVABLE 1
    #define device_type_qualifier(x) (x->b1 & 0x7F) /* vendor specific */

    BYTE b2;
    /*macros for accessing fields of byte 3*/
    #define ISO_Version(x)  ((x->b2 & 0xC0)>>6)
    #define ECMA_Version(x) ((x->b2 & 0x38)>>3)
    #define ANSI_Version(x) (x->b2 & 0x07)
      #define NONSTANDARD     0
      #define SCSI1           1
      #define SCSI2           2

    BYTE b3;
    /*macros for accessing fields of byte 4*/
    #define AENC(x)    ((x->b3 & 0x80)>>7)          /*asynchronous event notification */
    #ifndef TRUE
      #define TRUE 1
    #endif
    #ifndef FALSE
      #define FALSE 0
    #endif
    #define TrmIOP(x)  ((x->b3 & 0x40)>>6)          /* support terminate I/O process msg? */
    #define Response_Data_Format(x)  (x->b3 & 0x0F)
      #define SCSI1INQ      0   /* scsi1 standard inquiry data format */
      #define CCSINQ        1   /* CCS standard inquiry data format  */
      #define SCSI2INQ      2   /* scsi2 standard inquiry data format  */

    BYTE additional_length;     /* number of bytes following this field minus 4 */
    BYTE res5;

    BYTE b6;
    #define MChngr(x)    ((x->b6 & 0x08)>>3)

    BYTE b7;
    /*macros for accessing fields of byte 7*/
    #define RelAdr(x)    ((x->b7 & 0x80)>>7) /* the following fields are true or false */
    #define WBus32(x)    ((x->b7 & 0x40)>>6)
    #define WBus16(x)    ((x->b7 & 0x20)>>5)
    #define Sync(x)      ((x->b7 & 0x10)>>4)
    #define Linked(x)    ((x->b7 & 0x08)>>3)
    #define CmdQue(x)    ((x->b7 & 0x02)>>1)
    #define SftRe(x)     (x->b7 & 0x01)

    char vendor_identification[8];
    char product_identification[16];
    char product_revision_level[4];
    };

  struct st_inquiry
    {
    struct inq_data_s standard;
    BYTE vendor_specific[255-sizeof(struct inq_data_s)];
    };


  /*************************************************************************
  * MTDEVICE ioctl command to return device number for library access
  *************************************************************************/
  #ifndef MTDEVICE
    #define MTDEVICE        _IOR('m', 0x45, int)
  #endif

  /*************************************************************************/
  /* Structures describing log record                                      */
  /*************************************************************************/
  /* log file policy:
     There is one file per device instance. This file ias created on first
     open. A record is written after each close if there was one generated.
     Any logfile is deleted on deconfiguration. The file is a wrap around
     file with tape->maxlogfilesize # of possible entries at any given time.
     The format is as follows:
       logfile_header
       tape->maxlogfilesize # of entries or less
  */
  struct logfile_header
    {
    char owner[16];             /* module that created the file */
    time_t when;                /* time when file created */
    unsigned long count;        /* # of entries in file*/
    unsigned long first;        /* entry # of first entry in wrap queue */
    unsigned long max;          /* maximum # of entries allowed before wrap*/
    unsigned long size;         /* size in bytes of entrys,entry size is fixed*/
    };
  struct log_record_header
    {
    time_t when;                /* time when log entry made */
    ushort type;                /* log entry type */
      #define LOGSENSE 2        /*   SCSI log sense data */
    char device_type[16];       /* device type that made entry */
    char volid[32];             /* volume id of entry */
    };

  #define NO_PAGE 0xFF
  struct log_page_header
    {
    char code;
    char res;
    unsigned short len;
    };

/* Medium Changer Ioctl's
 *
 *  Note:  It is extremely important that these structures be PACKED!!!
 *         The reason is that the members of these structures map exactly
 *         to hardware returned bit-fields and bytes (SCSI-2 standard).
 */

#define DD_MEDIUM_CHANGER       'j'
#define DS_INTEGRATED           'i'
#define DS_INDEPENDENT          'd'

#define SIOC_INQUIRY           _IOR('C',1,struct inquiry_data)
#define SIOC_REQSENSE          _IOR('C',2,struct request_sense)
#define SMCIOC_ELEMENT_INFO    _IOR('C',3,struct element_info)
#define SMCIOC_MOVE_MEDIUM     _IOW('C',4,struct move_medium)
#define SMCIOC_POS_TO_ELEM     _IOW('C',5,struct pos_to_elem)
#define SMCIOC_INIT_ELEM_STAT  _IO('C',6)
#define SMCIOC_INVENTORY       _IOW('C',7,struct inventory)

#pragma options align=packed
struct inquiry_data
{
    uint  qual:3,               /* peripheral qualifier */
	  type:5;               /* device type */
    uint  rm:1,                 /* removable medium */
	  mod:7;                /* device type modifier */
    uint  iso:2,                /* ISO version */
	  ecma:3,               /* EMCA version */
	  ansi:3;               /* ANSI version */
    uint  aenc:1,               /* asynchronous event notification */
	  trmiop:1,             /* terminate I/O process message */
	  :2,                   /* reserved */
	  rdf:4;                /* response data format */
    uchar len;                  /* additional length */
    uchar resvd1;               /* reserved */
    uint  :4,                   /* reserved */
	  mchngr:1,             /* medium changer mode (SCSI 3 only) */
	  :3;                   /* reserved */
    uint  reladr:1,             /* relative addressing */
	  wbus32:1,             /* 32 bit wide data transfers */
	  wbus16:1,             /* 16 bit wide data transfers */
	  sync:1,               /* synchronous data transfers */
	  linked:1,             /* linked commands */
	  :1,                   /* reserved */
	  cmdque:1,             /* command queueing */
	  sftre:1;              /* soft reset */
    uchar vid[8];               /* vendor ID */
    uchar pid[16];              /* product ID */
    uchar revision[4];          /* product revision level */
    uchar vendor1[20];          /* vendor specific */
    uchar resvd2[40];           /* reserved */
    uchar vendor2[31];          /* vendor specific (padded to 127) */
};

struct request_sense
{
    uint        valid:1,        /* sense data is valid */
	        err_code:7;     /* error code */
    uchar       segnum;         /* segment number */
    uint        fm:1,           /* file mark detected */
	        eom:1,          /* end of medium */
	        ili:1,          /* incorrect length indicator */
	        :1,             /* reserved */
	        key:4;          /* sense key */
    signed int  info;           /* information bytes */
    uchar       addlen;         /* additional sense length */
    uint        cmdinfo;        /* command specific information */
    uchar       asc;            /* additional sense code */
    uchar       ascq;           /* additional sense code qualifier */
    uchar       fru;            /* field replaceable unit code */
    uint        sksv:1,         /* sense key specific valid */
	        cd:1,           /* control/data */
	        :2,             /* reserved */
	        bpv:1,          /* bit pointer valid */
	        sim:3;          /* system information message */
    uchar       field[2];       /* field pointer */
    uchar       vendor[109];    /* vendor specific (padded to 127) */
};

struct element_info
{
    ushort robot_addr;          /* first robot address */
    ushort robots;              /* number medium transport elements */
    ushort slot_addr;           /* first medium storage element address */
    ushort slots;               /* number medium storage elements */
    ushort ie_addr;             /* first import/export element address */
    ushort ie_stations;         /* number import-export elements */
    ushort drive_addr;          /* first data transfer element address */
    ushort drives;              /* number data transfer elements */
};

struct move_medium
{
    ushort robot;               /* robot address */
    ushort source;              /* move from location */
    ushort destination;         /* move to location */
    char invert;                /* invert before placement bit */
};

struct pos_to_elem
{
    ushort robot;               /* robot address */
    ushort destination;         /* move to location */
    char invert;                        /* invert before placement bit */
};

struct telement_status
{
    ushort address;             /* element address */
    uint   :2,                  /* reserved */
	   inenab:1,            /* media into changer's scope */
	   exenab:1,            /* media out of changer's scope */
	   access:1,            /* robot access allowed */
	   except:1,            /* abnormal element state */
	   :1,                  /* reserved */
	   full:1;              /* element contains medium */
    uchar  resvd1;              /* reserved */
    uchar  asc;                 /* additional sense code */
    uchar  ascq;                /* additional sense code qualifier */
    uint   notbus:1,            /* element not on same bus as robot */
	   :1,                  /* reserved */
	   idvalid:1,           /* element address valid */
	   luvalid:1,           /* logical unit valid */
	   :1,                  /* reserved */
	   lun:3;               /* logical unit number */
    uchar  scsi;                /* scsi bus address */
    uchar  resvd2;              /* reserved */
    uint   svalid:1,            /* element address valid */
	   invert:1,            /* medium inverted */
	   :6;                  /* reserved */
    ushort source;              /* source storage element address */
    uchar  volume[36];          /* primary volume tag */
    uchar  resvd3[4];           /* reserved */
};

struct inventory
{
    struct telement_status *robot_status; /* medium transport element pages */
    struct telement_status *slot_status;  /* medium storage element pages */
    struct telement_status *ie_status;    /* import-export element pages */
    struct telement_status *drive_status; /* data transfer element pages */
};


#pragma options align=reset
#endif
