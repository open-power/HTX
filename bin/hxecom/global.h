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
/* @(#)97       1.9.2.6  src/htx/usr/lpp/htx/bin/hxecom/global.h, exer_com, htx53A 6/24/04 12:11:42 */
/*
 *   COMPONENT_NAME: exer_com
 *
 *   FUNCTIONS: HDR_CKSUM
 *		HDR_SEQNO
 *		HDR_TS
 *		HTXERROR
 *		NEW_STRERR
 *		NFDS
 *		STRERROR
 *
 *   ORIGINS: 27
 *
 *   IBM CONFIDENTIAL -- (IBM Confidential Restricted when
 *   combined with the aggregated modules for this product)
 *   OBJECT CODE ONLY SOURCE MATERIALS
 *
 *   (C) COPYRIGHT International Business Machines Corp. 1994
 *   All Rights Reserved
 *   US Government Users Restricted Rights - Use, duplication or
 *   disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#ifndef HXECOMGLOBAL_H
#define HXECOMGLOBAL_H

#ifdef __RELGR2600__
#define __NEED_PACK_MOD__
#endif

#define MAX_CHARS	131071
/* Maximum string length allowed for HTX pathnames.                           */
#define FNAME_MAX		100

#ifdef __RDMA_SUPPORTED__
#include <dat2/udat.h>
#endif 

#define HOST_NAME_LEN	32
#define DEV_NAME_LEN	40
/******************************************************************************/
/***  Global Variable Definitions  ********************************************/
/******************************************************************************/
/*                                                                            */
#ifdef DEBUG
extern int debug_flag;
#endif
/*                                                                            */
extern struct   shm_phxecom_t * shm_pHXECOM;
extern struct   shm_hxecom_t  * shm_HXECOM;
extern struct   shm_Private_t * shm_Private;
extern char comNetAddr[20];

/* STRERROR is required because strerror() is not thread-safe and strerror_t  */
/* is too cumbersome.                                                         */
extern int      sys_nerr;

#ifndef __HTX_LINUX__
extern char *   sys_errlist[];
#endif

/* We need thread specific place to generate err_msg when in NT.              */
/* stats->msg_text is much longer than ever used.  So partition its use.      */
#define ERR_LEN    (MAX_TEXT_MSG - 400)
#define	ERRNO errno
#define NEW_STRERR(E)  STRERROR(E)
#define STRERROR(E) (E >= 0 && E <= sys_nerr)? sys_errlist[E] : ""
#define DEMO_CONN_QUAL                                (16925)

/******************************************************************************
 **** This is a HACK, please do fix this for LITTLE endian machine , 
 ****  as long as ppc64 is big endian we are safe to do this.  
 *****************************************************************************/ 
#ifdef __HTX_LINUX__
#define ntohll(x) (x) 
#define htonll(x) (x) 
#endif 

#define HTXERROR(ex,no) ((ex<<16) +no)

/******************************************************************************/
/***  End of Global Variable Definitions  *************************************/
/******************************************************************************/

/* increase this if you have more than 200 network adapters to test */
#define EXER_MAX   	  512
#define TRACE_ARG_LEN	  300
#define MAXREMOTE	  50
/* max number of other hosts in the test */
#define MAXOTHERS	  8
#define MAX_THREADS       (2 * MAXREMOTE + 2)
#define MAXADDR           10
#define MAX_STANZAS	  10
/* increase this to 1/2 EXER_MAX */
#define MAX_COORD	  256
#define	MAXLOCAL 	  256
/* This is necessary because MAXHOSTNAMELEN changed
   form 32 bytes in 3.2 to 256 bytes in 4.1                                */
#define HXECOM_HOSTNAMELEN 32

/* Type of Master sems in use.   */
#define SLEEP_SEM               0
#define FILE_SEM	        1
#define COORD_SEM		2
#define WRITE_SEM               3
#define FORK_SEM                4
#define MASTER_NO_SEMS          5
#define RANDOM					-1
#define SEQUENTIAL				2
#define WRITEAHEAD              2 

#define SHM_HXECOM_KEY          (key_t)0x0bbb0000
#define SHM_HXECOM_PRIV_KEY     (key_t)0x0bbb0001
#define MASTER_HXECOM_KEY       (key_t)0x0bbb0002
#define BACKOUT_SEM_KEY         (key_t)0x0bbb0003

#define UDP_LAYER		((unsigned char) 1)
#define TCP_LAYER		((unsigned char) 2)
#define RDMA_LAYER		((unsigned char) 3)
#define ACK_MAX                 10

/* Each process that uses cables creates semaphores using
   (MY_HXECOM_KEY + exer_no) as a semaphore key                               */
#define MY_HXECOM_KEY            (key_t)0x0bbb0004

/* Type of process semaphores in use.                                         */
#define WRITE_AHEAD0    	0
#define MY_NO_SEMS		MAXREMOTE

#define SLASH_CHR       '/'
#define SLASH_TXT       "/"
#define CONFIG_FILE	    "htxcom.config"
#define LSTAT_FILE      "htxstats2"
#define RULE_PATH	    "../rules/reg/hxecom"
#define PATTERN_PATH	"../pattern"


/* This use to fill the read buffer with '\0'.  This is not
   a good choice since the device may put a zero in a
   position to represent a parity error, etc.                                 */
#define BUF_FILL_CHAR	        '.'
#define  NO_OPER       5


struct rule_format {
   /*  Default is class C network.  last byte of TestNetName is 0xff          */
   uint32_t	   ComNetMask;
   /*  Default is COM_STREAM_DEFAULT                                          */
   u_short         ComPortStream;
   /*  Default is COM_DGRAM_DEFAULT                                           */
   u_short         ComPortDgram;
   u_short         ComPortDapl;
   /*  Default is COM_UDAPL_DEFAULT                                           */
   u_short	   nettype;
   u_short	   write_ahead;
   u_short         ack_trig;
   u_short	   num_oper;
   u_short         bufmin;
   u_short         bufmax;
   short           bufinc;
   u_short    bufseed; 	
   u_short         replicates;
   u_short         write_sleep;  /* in ms.                                    */
   u_short         alarm;        /* in seconds.                               */
   u_short         idle_time;    /* in seconds.                               */
   u_short	   shutdown;
   /* If master we accept connections form other nodes.                       */
   /* The type of connection established will be determined by the type of    */
   /* transaction specified by the other node.  i.e. R, W, or RW              */
   /* transact field consists of the following bits:                          */
   /* bit 0        master  If a master, make R/W threads based on other nodes.*/
   /* bit 1        Reader threads are formed to other master nodes.           */
   /* bit 2        Writer threads are formed to other master nodes.           */
   /* bit 3        TCP_NODELAY option, TCP only.                              */
   /* bit 4        SO_LINGER option, TCP only.                                */
   u_short         transact;
   u_short         onesys;
   u_short         debug_pattern;
   u_short         no_compare;
   /*  No default, This is the name of the test network                       */
   char		   TestNetName[HXECOM_HOSTNAMELEN];
   /*  Default is the hostname                                                */
   char		   ComName[HXECOM_HOSTNAMELEN];
   char            rule_id[9];	/* Rule Id                                    */
   char            pattern_id[9];	/* /htx/pattern_lib/xxxxxxxx          */
   unsigned char   layer;
   char            pad[13];
};


/******************************************************************************/
/* Bit values for transact field in struct rule_format and struct id_t.       */
/******************************************************************************/
#define MASTER_VAL       1
#define READ_VAL         2
#define WRITE_VAL        4
#define TCP_NODELAY_VAL  8
#define SO_LINGER_VAL   16

/******************************************************************************/
/* The following 3 structures have been padded so that any compiler/machine   */
/* that accesses char on 1 byte boundaries, u_shorts on 2 byte boundaries     */
/* and u_longs on 4 byte boundaries will be able to read these structures     */
/* when written through sockets.  Also assumes little-endian, big-endian is   */
/* handled.  They have also been hand padded to the same length to eliminate  */
/* union problems.   Assumes u_short is 16 bits, u_long is 32 bits which is   */
/* the assumption with BSD socket routines.  All messages are 24 bytes.       */
/******************************************************************************/
struct id_t {
    u_short	nettype;
    u_short     replicates;
    u_short     transact;
    u_short     ExerVersion;
	pid_t		the_pid;
    struct sockaddr_in sock;
};

/******************************************************************************/
/* Increment this number for release of incompatible versions.                */
/******************************************************************************/
#define EXER_VERSION  3

struct semid_t {
    u_short  WriteIdx;
    u_short  ackNo;
    u_short  NumOper;
    u_short  bsize;
    u_short  Iloop;
    unsigned char timeS;
    unsigned char Stanza;
    unsigned char WriteAhead;
	unsigned char dummy; 
    char     pad[14];
};

struct wsize_t {
    uint32_t size;
	char pad1[4]; 
    char pad[20];
};

struct bufsize_t { 
	u_short WriterIdx; 
	u_short bufmin; 
	u_short bufmax; 
	u_short num_oper; 
	u_short num_pktsizes; 
	char pad[18]; 
}; 

#ifdef __RDMA_SUPPORTED__
struct mr_triplet {
	u_short WriterIdx;		  	/* 2 bytes		 */  
	DAT_LMR_TRIPLET local_iov; 	/* 16  byte long */ 
	char pad[10]; 
};
#endif 
struct CoordMsg {
    uint32_t  msg_type;
    union {
        struct id_t    server;
        struct semid_t SemID;
        struct wsize_t Wsize;
		struct bufsize_t Bsize; 
	#ifdef __RDMA_SUPPORTED__
		struct mr_triplet rmr_info;
	#endif 
    }ID;
};

struct CoordID_t {
    int         CoordmID;
    pid_t       CoordPID;
    u_short      ComPortStream;
    u_short      ExerCnt;
    unsigned int dead_flag : 1;
};

struct shm_comsem_t {
	int                 mID;
    int                 NoWriters;
    struct loop_t {
      	unsigned int        CheckReset : 1;
      	unsigned int        WriterReset : 1;
      	unsigned int        halt        : 1;
      	unsigned char       timestamp;
      	unsigned char       WriterStanza;
      	unsigned char       timeS;
      	unsigned char       TERM;
     	unsigned short int  WriterIloop;
      	unsigned short int  WriterBsize;
      	unsigned short int  WriterPak;
      	int                 last;
	  	u_short rand_pktsize[USHRT_MAX]; 	
	  	u_short num_pktsizes; 
	#ifdef __RDMA_SUPPORTED__	
	  	DAT_LMR_TRIPLET recv_iov;
		DAT_RMR_TRIPLET remote_iov;
		pid_t WriterPid; 
		double thruput; 
		char unit ; 
	#endif  
    }loop[MAXREMOTE];
};

/* Variables shared between Parent and child   */
struct shm_coord_t {
    int          mID;
    int          NoRemotes;
    struct id_t  RemoteIDs[MAXREMOTE];
    int          NoRemoteAddr;
    uint32_t       RemoteAddr[MAXADDR];
    int		 NoLocals;
    int		 LocalExerNo[MAXLOCAL];
    struct id_t  LocalIDs[MAXLOCAL];
    int          BroadcastInterval;
    int          NumberBroadcast;
    int          BroadcastAlarm;
    int          BroadcastIntervalsRemaining;
};

/* Variables shared only between parent exercisers. */
struct shm_Private_t {
   int          mID;
   int          BackoutSemid;
};

/* shared by all hxecom exercisers running on one host */
struct shm_hxecom_t {
   int          mID;
   unsigned int global_sem_ready : 1;
   int          MasterSemid;
   int          NoExer;			/* current number of exercisers running */
   int          DeadCnt;		/* number of dead exercisers */
   int          OneSysFlag;     /* set to one for one system test */
   struct in_addr    Other_ips [MAXOTHERS];
   int          HostCnt;
   char         mynetid[4];     /* last octet of this hosts comnet ip addr */
   struct htx_data TempStats;
   unsigned char NoCoord;
   struct CoordID_t Coord[MAX_COORD];
   struct exer_t {
        int          mID_exer;
        pid_t        EXER_PID;
        unsigned int dead_flag : 1;
        unsigned int ServerIDinit : 1;    /* Not generally available until init */
        unsigned int SigChildFlag : 1;
        struct id_t  ServerID;            /* Identifies socket for exerciser.   */
    }exer[EXER_MAX];
};

/* This shared memory is private to 1 exerciser and all Its spawned processes */
/* Spawned processes are:
	coordinator
	semaphore and shared mem process
	group of readers and writers, the number of which depends on replicates */

struct shm_phxecom_t {
      int                mID;		    /*shared memory id */
      int                mID_COMSEM;    /*sh mem id for shm_comsem_t */
      int                ExerNo;
      unsigned short int NoWriters;     /* How many readers we have.          */
      struct id_t        SemServID;
      int                SigTermFlag;
      int                SigAlarmFlag;
      int                SigUsr1Flag;
      int                BrokenPipes;
      int                starttime;
      int                bootme;
};

struct resp_t {
    struct htx_data stats;
};

#define NET_ADDR_LEN		16
#define COM_STREAM_DEFAULT	5101
#define COM_DGRAM_DEFAULT	5102
#define COM_UDAPL_DEFAULT	5103
#define COM_NET_MASK		0xffffff00

/* The following defines are for Test Packet header.                           */
/* TP_MAX_SEQNOS and TP_SEQ_LEN must be consistent.                            */
#define TP_MAX_SEQNOS   (0x7fffffffffffffffLL)
#define TP_SEQ_LEN      8
#define TP_TIME_POS     (TP_SEQ_LEN )
#define TP_CKSUM_POS            (TP_SEQ_LEN  + 1)
#define TP_HDR_LEN      (TP_SEQ_LEN  + 2)

#define COM_FIX_SIZ_DATA_LEN (HOST_NAME_LEN + DEV_NAME_LEN + TP_HDR_LEN)
#define HDR_SEQNO(buf, end, num)  \
        {                                           \
          *(unsigned long long *)buf = (signed long long)num; \
          *(unsigned long long *)end = (signed long long)num; \
        }
#define INSERT_PAK_SEQNO(buf, num)  \
        { *(unsigned long long *)(buf) = num;  \
        }

#define INSERT_RETRY_CNT(buf, num) \
          buf[0] = (num)

#define INSERT_PID(buf, num)  \
        { buf[8] =  (int)(num)     %10 + 48; \
          buf[7] = ((int)(num)/10) %10 + 48; \
          buf[6] = ((int)(num)/100) %10   + 48; \
          buf[5] = ((int)(num)/1000) %10   + 48; \
          buf[4] = ((int)(num)/10000) %10    + 48; \
          buf[3] = ((int)(num)/100000) %10   + 48; \
          buf[2] = ((int)(num)/1000000) %10   + 48; \
          buf[1] = ((int)(num)/10000000) %10   + 48; \
          buf[0] = ((int)(num)/100000000)    + 48; \
          buf[9] = '\0'; \
        }

#define INSERT_BLK_SEQNO(buf, num)  \
        { buf[4] =  (int)(num)     %10 + 48; \
          buf[3] = ((int)(num)/10) %10 + 48; \
          buf[2] = ((int)(num)/100) %10   + 48; \
          buf[1] = ((int)(num)/1000) %10   + 48; \
          buf[0] = ((int)(num)/10000)    + 48; \
          buf[5] = '\0'; \
        }

#define HDR_TS(buf, end, ts) \
          end[TP_TIME_POS] = buf[TP_TIME_POS] = (ts)


#define HDR_CKSUM(buf, end) \
          end[TP_CKSUM_POS] = buf[TP_CKSUM_POS] = (char)(buf[0]+buf[1]+buf[2]+buf[3]+buf[4]+buf[5]+buf[6]+buf[7]+buf[8])


struct writer_id {
    u_short	nettype;
    struct sockaddr_in reader;
    int		writer_sock;
};

/* Message types for Coordinator messages  */
#define CM_LOCALSERVER		((unsigned int)1)
#define CM_REMOTESERVER		((unsigned int)2)
#define CM_REQ_RULES		((unsigned int)3)
#define CM_RULES_STANZA		((unsigned int)4)
#define CM_RULES_FINISHED	((unsigned int)5)
#define CM_REQ_RDR_ID		((unsigned int)6)
#define CM_READER_ID		((unsigned int)7)
#define CM_REQ_PATTERN		((unsigned int)8)
#define CM_PATTERN		((unsigned int)9)
#define CM_READACK	       ((unsigned int)10)
#define CM_HALT_WRITE	       ((unsigned int)11)
#define CM_RESET_WRITE         ((unsigned int)12)
#define CM_HTXMSG	       ((unsigned int)13)
#define CM_SHUTDOWN	       ((unsigned int)14)
#define CM_BROADCAST_SHUTDOWN  ((unsigned int)15)
#define CM_SIGTERM             ((unsigned int)16)
#define CM_LOCAL_EXER_TERM     ((unsigned int)17)
#define CM_REMOTE_EXER_TERM    ((unsigned int)18)
#define CM_TERM_WRITE          ((unsigned int)19)
#define CM_BROADCAST_LOCALS    ((unsigned int)20)
#define CM_RULES_ERROR         ((unsigned int)21)
#define CM_STOP_READER         ((unsigned int)22)
#define CM_READER_STOPPED      ((unsigned int)23)
#define CM_START_WRITER        ((unsigned int)24)
#define GET_PKTSIZES		   ((unsigned int)25) 	
#define PKTSIZES			   ((unsigned int)26)
#define ACK_PKTSIZES		   ((unsigned int)27) 	

#define DAPL_RMR_XCHG			((unsigned int)28)
#define DAPL_RMR_XCHG_ACK		((unsigned int)29)

#define CONNECTSTR_LEN         80

/* CM_SHUTDOWN ACTIVATORS                 */
#define SH_ANY		   (u_short) 0x0fff
#define SH_MISCOMPARE      (u_short) 0x0001
#define SH_SIGALARM        (u_short) 0x0002
#define SH_EINTR           (u_short) 0x0004
#define SH_TIMESTAMP       (u_short) 0x0008
#define SH_PAKLEN          (u_short) 0x0010
#define SH_DROPPAK         (u_short) 0x0020
#define SH_DUPPAK          (u_short) 0x0040
#define SH_OUTSEQPAK       SH_DUPPAK
#define SH_UNKPAK	   (u_short) 0x0080
#define SH_BADHDR	   (u_short) 0x0100
#define SH_PIPE  	   (u_short) 0x0200
#define SH_WR_OTHER        (u_short) 0x0400
#define SH_FORCE           (u_short) 0x0800
#define SH_KDB_MISCOMPARE  (u_short) 0x1000
#define SH_ATTN_MISCOMPARE  (u_short) 0x2000
#define SH_CRASH_ON_ANY_ERROR (u_short) 0x4000


typedef int SOCKET;
#define INVALID_SOCKET  -1
#define SOCKET_ERROR    -1
#define closesocket  close
#define ioctlsocket  ioctl
#define WSAEINTR     EINTR
#define WSAESHUTDOWN EPIPE


struct comwrite_argt {
    struct sockaddr_in RemoteServerSock;
    struct htx_data stats;
	int exer_idx; 
    char   TestNetName[HXECOM_HOSTNAMELEN];
};



struct comread_argt {
    struct sockaddr_in SemServerSock;
    int       WriterIdx;
    struct sockaddr_in WriterTestSock;
    SOCKET    ReaderTestSock;
	struct sockaddr_in ReaderID; 
    struct htx_data stats;
};



struct coord_argt {
    struct htx_data stats;
};



struct comsem_argt {
    SOCKET    SemServSock;	/* this is file descriptor for the socket */
	unsigned int bufseed ;  /* Initial seed passed through rules file stanza 0 */ 
    struct htx_data stats;
};



/* Used by semaphore  */
extern struct sembuf Sunlock;
extern struct sembuf Slock;


#endif /* HXECOMGLOBAL_H */
