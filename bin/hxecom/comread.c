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
/* @(#)90       1.30.1.2  src/htx/usr/lpp/htx/bin/hxecom/comread.c, exer_com, htx610 8/1/07 07:16:53 */
/*
 *   COMPONENT_NAME: exer_com
 *
 *   FUNCTIONS: DRead
 *	    SendHaltToSemServ	
 *		RWmsg
 *		ReadExit
 *		SendResetToSemServ	
 *		SHUTDOWNTEST
 *		SRead
 *		SendTermToSemServ	
 *		WriteLocalStats
 *		bad_bytes_read
 *		comread
 *		good_bytes_read
 *		headerCkFail
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


#   include <stdio.h>
#   include <stdlib.h>
#   include <unistd.h>
#   include <string.h>
#   include <memory.h>
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/time.h>
#   include <sys/select.h>
#   include <sys/ioctl.h>
#   include <netinet/in.h>
#   include <netdb.h>
#   include "hxecomdef.h"
#   include "comrw.h"


/* define NO_BLOCK 1 */

#ifdef __HTX_LINUX__
#define NFDS(a)   (a)
#endif

static void SendResetToSemServ(int WriteIdx, int WriteAhead,unsigned char timestamp, int i, int bsize, int stanza, int pak, 
								struct sockaddr_in * dest, struct htx_data * stats, u_long state);
static int ReqPktSize(int WriteIdx, u_short bufmin, u_short bufmax, u_short num_oper, u_short * bsize, int * num_pktsizes,
            struct htx_data * stats, struct sockaddr_in dest, u_long state );
static void RWmsg(struct cum_rw * Lstats, struct rule_format * rule, int Stanza, struct htx_data * stats, int err, enum sev_code sev, char * msg_text);
static int  headerCkFail(char rbuf[], char MsgStr[]);
static int  not_running_bootme(void);
static int do_compare(char *rbuf_ptr,char *pattern_ptr,int pak_size, struct rule_format *rule, struct htx_data *stats,char ConnectStr[],int fixpat);
static void SendTermToSemServ(int WriteIdx, struct sockaddr_in * dest, struct htx_data * stats, u_long state); 
static void SendHaltToSemServ(int WriteIdx, struct sockaddr_in * dest, struct htx_data * stats, u_long state); 
static void bad_bytes_read(struct cum_rw * Lstats, struct htx_data * pstats,
                      struct htx_data * stats, int bytes, int BadCnt);
static int SRead(SOCKET fd, register char *ptr, register int nbytes, int nmax, struct rw_t * rwParms,struct htx_data * stats, char *connect_str);
static int DRead(SOCKET fd, register char *ptr, register int nbytes, int nmax, struct rw_t * rwParms,struct htx_data * stats,char *connect_str);
static void WriteLocalStats(struct cum_rw LSTATS[], int NoStanzas, char ConnectStr[], struct htx_data * stats, char *msg_text);
static void ReadExit(struct htx_data * stats, struct cum_rw * LSTATS, int NoStanzas, int exitNo,
                                    char msg[], char ConnectStr[]);
static void good_bytes_read(struct cum_rw * Lstats, struct htx_data * pstats,
                                     struct htx_data * stats, int bytes, char ConnectStr[]);
static void SendReadAckToSemServ(struct CoordMsg * CMsg, struct sockaddr_in * dest, struct htx_data * stats, u_long state);
 
SOCKET TestSock; 

extern char global_htx_log_dir[256];

/***************************************************************************
 * GCC Sucks !! It would not allow two pre-processor directives in a single 
 * line. So all this for a single line difference ..   
 **************************************************************************/ 
#ifndef __HTX_LINUX__ 
#define SHUTDOWNTEST(ShutdownFlag)  \
                    { \
                        if(ShutdownMask & ShutdownFlag || ShutdownFlag & SH_FORCE) { \
                            if(ShutdownMask & SH_CRASH_ON_ANY_ERROR) {   \
								trap((int) MISC, ShutdownFlag, stats,rule); \
							}   \
                            shutdown(ReaderTestSock, 2); \
                            shm_pHXECOM->SigTermFlag=1; \
                            ShutdownTest(*ServerIDptr, stats); \
                            sprintf(msg_text, "Shutting down testing due to error flag - %hx\n%s\n", ShutdownFlag,  ConnectStr); \
                            RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_SHUTDOWN,0), HTX_HE_SOFT_ERROR, msg_text); \
                            hxfupdate(UPDATE, stats); \
                            msg_text[0] = '\0'; \
                            ReadExit(stats, LSTATS, NoStanzas, HTXERROR(EX_SHUTDOWN,0), msg_text, ConnectStr); \
                        } \
                    }
#else 
#define SHUTDOWNTEST(ShutdownFlag)  \
                    { \
                        if(ShutdownMask & ShutdownFlag || ShutdownFlag & SH_FORCE) { \
                            if(ShutdownMask & SH_CRASH_ON_ANY_ERROR) {   \
                                do_trap_htx64((int) MISC, ShutdownFlag, stats,rule, 0, 0, 0, 0); \
                            }   \
                            shutdown(ReaderTestSock, 2); \
                            shm_pHXECOM->SigTermFlag=1; \
                            ShutdownTest(*ServerIDptr, stats); \
                            sprintf(msg_text, "Shutting down testing due to error flag - %hx\n%s\n", ShutdownFlag,  ConnectStr); \
                            RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_SHUTDOWN,0), HTX_HE_SOFT_ERROR, msg_text); \
                            hxfupdate(UPDATE, stats); \
                            msg_text[0] = '\0'; \
                            ReadExit(stats, LSTATS, NoStanzas, HTXERROR(EX_SHUTDOWN,0), msg_text, ConnectStr); \
                        } \
                    }
#endif 
/* changed to exit on header error, and on updated stanza packet error ver 11290702*/
/* ver is  modayr01  or 02 if second ver in same day. */
/* ver 11300701  trap on header errors too */
/* ver 01160801  no miscompare when do_compare index == -1 */
/* ver 01080901  new do_compare code more efficient for better thruput */
/* ver 02040901  attn call before trap on miscompare data section*/
/* ver 02040902  change SRead to set MSG_WAITALL on socket rcev so that */
/*               all data must be moved before recv returns and pulled */
/*               the code that altered ptr on EINTR */
/* ver 02060901  attn call before trap on miscompare headerCkFail section*/
/* ver 02060902  Add debug prints to htxmsg log in SRead on error paths */
/*               and force shutdown on bad headerCk, no recovery */
/* ver 03050901  Pull readexit on -1 from writer. now it's miscompare   */
/*               and writer retrys on bp 25 times in 12 hours          */
/*               and sleep on writer EPIPE to let EEH recoovery work    */
/* ver 03060901  change SRead to set MSG_WAITALL on socket rcev so that */
/*               all data must be moved before recv returns             */
/*               also returns -1 if all bytes are not received.         */
/* ver 03060902  more output in writer exit code... and change SWrite   */
/*               took out retry on -1 error  EINTR. NOTE this only works*/
/*               if alarm pop is the only type of EINTR that can happen */
/*               if you start seeing a bunch of IO_ALARM timeouts put it*/
/*               back like it was.                                      */
/* ver 03190901  Do retry on read 0 Unknown error fail..                */
/* ver 03190902  Put in no_compare flag in rules file                   */
/* ver 04160901  call do_comapre only once and increase bootme wait     */
/*               on miscompare */
/* ver 10140900  add rPak=atoi(rbuf) to set rPak from receieved data    */


#define MISC 0xBEEFDEAD
#define VER 0x10140900
#pragma mc_func trap { "7c810808" }
#pragma reg_killed_by trap

#ifndef __HTX_LINUX__
#pragma mc_func attn { "00000200" }
#pragma reg_killed_by attn
#else
void __attn (unsigned int a,
             unsigned int b,
             unsigned int c,
             unsigned int d,
	     unsigned int e,
	     unsigned int f){
        __asm__ volatile (".long 0x00000200":);
        }
#endif

/* global vars */
static pid_t pid=0;
char writer_dev[40];


void * comread(void * Targ)
{

    int    rc = 0;
    int    i,index,b;
    int    o;
    struct CoordMsg CMsg;
    struct rule_format rule[MAX_STANZAS];
    SOCKET ReaderTestSock;
    int    stanza;
    int    rPak, pak = 0;
    struct id_t ToSemServerID;
    struct sockaddr_in ReaderID;
    int    pattern_max;
    int    pakLen, bufLen, delta;
    register struct sockaddr_in *ServerIDptr;
    unsigned char   timestamp = '\0';
    int    BadCnt = 0;
    int    ConsecErrCnt = 0;
    char * pattern, * rbuf, * patternfile, * rbuffile, * PATTERN, * pattern_trailer, * debug_ptr;
    struct cum_rw * LSTATS;
    struct cum_rw * lstats;
    struct htx_data  PseudoStats;
    struct htx_data  * pstats;
    struct htx_data  * stats;
    struct rw_t   RWPARMS;
    struct rw_t * rwParms = &RWPARMS;
    socklen_t  length;
    int    NoStanzas;
    int    loop;
	int    (*readfn)(SOCKET, char *, int, int, struct rw_t *,struct htx_data *,char *);
    char	msgp[MAX_TEXT_MSG];
    u_short ShutdownMask = 0;
    char   PseudoDeviceStr[32];
    int    WriteIdx;
    char   ConnectStr[CONNECTSTR_LEN] = "Reader Not Initialized.";
    char   MSGstr[1000];
    char * msg_text;
    int    LastAck;
    struct comread_argt * arg;
	char tmpaddr[32];
	char tmpaddr1[32];
    char * tmp_ptr;
	u_short * bsize;
	int num_pktsizes_req, bcnt, bcnt_max; 
	int num_oper, num_pktsizes; 
	register struct sockaddr_in * SemServID;

    arg             = (struct comread_argt * )Targ;
    ReaderTestSock  = arg->ReaderTestSock;
    WriteIdx        = arg->WriterIdx;
    stats           = &arg->stats;
    msg_text        = MSGstr;
    PseudoStats     = arg->stats;
    pstats          = &PseudoStats;

    ServerIDptr = &shm_HXECOM->exer[shm_pHXECOM->ExerNo].ServerID.sock;

/********************************************************************/
/* Get ReaderID -- to be used for ConnectStr.                       */
/********************************************************************/
    length = sizeof(ReaderID);
    if(getsockname(ReaderTestSock, (struct sockaddr *) &ReaderID, &length)) {
        sprintf(msg_text, "comread: Error getsockname() - %s.\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_SOCK2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_SOCK2);
    }

/********************************************************************/
/* Obtain string used for ID when writing messages.                 */
/********************************************************************/
    GetConnectStr(ConnectStr, CONNECTSTR_LEN, "R", ReaderID, " connected to W", arg->WriterTestSock, stats);


	/* examine the ConnectStr and make sure reader and writer are not the same
	  address */
	if( strcmp (InetNtoa(arg->WriterTestSock.sin_addr, tmpaddr, stats),
		InetNtoa(ReaderID.sin_addr, tmpaddr1, stats))==0) {
  		HE_exit(0);
	 }

/********************************************************************/
/* Obtain rules file stanzas as array of struct rule_format         */
/********************************************************************/
    NoStanzas = GetRules(*ServerIDptr, rule, MAX_STANZAS, stats);

/********************************************************************/
/* Find largest pattern to be read.                                 */
/********************************************************************/
    pattern_max = 0;
    for(i=0; i<NoStanzas; i++)
        if(rule[i].bufmax > pattern_max)
            pattern_max = rule[i].bufmax;

/********************************************************************/
/* Allocate buffer for pkt sizes                                    */
/********************************************************************/
    if((bsize = (u_short *)malloc(sizeof(u_short ) * USHRT_MAX)) == NULL) {
        sprintf(msg_text, "comwrite: Malloc of random buffer space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC9,ERRNO), HTX_HE_HARD_ERROR, msg_text);
        HE_exit(EX_MALLOC9);
    }


/********************************************************************/
/* Allocate space for pattern file and read buffer.                 */
/* Leave space for:                                                 */
/*                  ASCII sequence number of SEQ_LEN characters.    */
/*                  Null character.                                 */
/*                  Timestamp of 1 character.                       */
/*                  Check character.                                */
/*                                                                  */
/* Total length of header is TP_SEQ_LEN + 3                         */
/*                                                                  */
/* Even though packets should be delivered in order since we are on */
/* a local network, if the timeout for the read is too short or if  */
/* the write hangs but transmits later.                             */
/********************************************************************/
    bufLen = pattern_max + 2 * (TP_HDR_LEN);
    if((pattern = (char *)malloc(bufLen * sizeof(char))) == NULL) {
        sprintf(msg_text, "comread: Malloc of pattern space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC5);
    }
	memset(pattern, 0x00, (bufLen * sizeof(char)));

    if((rbuf = (char *)malloc(bufLen * sizeof(char))) == NULL) {
        sprintf(msg_text, "comread: Malloc of read buffer space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC6,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC6);
    }
	memset(rbuf, 0x00, (bufLen * sizeof(char))); 
    patternfile = pattern + TP_HDR_LEN;
    rbuffile    = rbuf + TP_HDR_LEN;

/********************************************************************/
/* Allocate space for copy of pattern file.                         */
/********************************************************************/
    if((PATTERN = (char *)malloc(pattern_max * sizeof(char))) == NULL) {
        sprintf(msg_text, "comread: Malloc of pattern file space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC7,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC7);
    }
	memset(PATTERN, 0x00, (pattern_max * sizeof(char)));

/********************************************************************/
/* Allocate space for local stats and zero.                         */
/********************************************************************/
    if((LSTATS = (struct cum_rw *)malloc(sizeof(struct cum_rw) * NoStanzas)) == NULL) {
        sprintf(msg_text, "comread: Malloc of local stats space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC8,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC8);
    }
    for(i=0; i< NoStanzas; i++) {
        LSTATS[i].totals.bad_others      = 0;
        LSTATS[i].totals.bad_rw          = 0;
        LSTATS[i].totals.bad_bytes_rw1   = 0;
        LSTATS[i].totals.bad_bytes_rw2   = 0;
        LSTATS[i].totals.good_bytes_rw1  = 0;
        LSTATS[i].totals.good_bytes_rw2  = 0;
        LSTATS[i].totals.good_others     = 0;
        LSTATS[i].totals.good_rw         = 0;
        LSTATS[i].current.bad_others     = 0;
        LSTATS[i].current.bad_rw         = 0;
        LSTATS[i].current.bad_bytes_rw1  = 0;
        LSTATS[i].current.bad_bytes_rw2  = 0;
        LSTATS[i].current.good_bytes_rw1 = 0;
        LSTATS[i].current.good_bytes_rw2 = 0;
        LSTATS[i].current.good_others    = 0;
        LSTATS[i].current.good_rw        = 0;
    }

/********************************************************************/
/* Obtain pattern file for write.                                   */
/********************************************************************/

    rc = GetPattern(*ServerIDptr, PATTERN, pattern_max, stats);


/*******************************************************************/
/*                                                                  */
/********************************************************************/
    if((int)rule[0].layer == UDP_LAYER) {
        TestSock = ReaderTestSock;
        readfn = DRead;
    }
    else if((int)rule[0].layer == TCP_LAYER) {
        listen(ReaderTestSock, 1);
        readfn = SRead;
    }
    else {
        sprintf(msg_text, "comread: Error specifying test layer - %d\n", (int)rule[0].layer);
        hxfmsg(stats, HTXERROR(EX_LAYER1,0), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_LAYER1);
    }

/********************************************************************/
/* Begin testing. Writer won't connect or write until we reset      */
/* semaphore.  This will prevent race conditions.                   */
/********************************************************************/

	SemServID = &arg->SemServerSock; 
	SendResetToSemServ(WriteIdx, rule[0].write_ahead, timestamp, 1, 2, 3, 4, SemServID, stats, 0); 	
 

/********************************************************************/
/* Accept connection from writer.  Previously enabled with listen() */
/********************************************************************/
    if((int)rule[0].layer == TCP_LAYER) {
        TestSock = accept(ReaderTestSock, 0, 0);
	}


/********************************************************************/
/*  Setup reader test socket as nonblocking.                        */
/********************************************************************/
#ifdef NO_BLOCK
    sprintf(msg_text, "%s\nBeginning test... NON BLOCKING READS-- verion %8x", ConnectStr,VER);
    hxfmsg(stats, HTXERROR(HEINFO10,0), HTX_HE_INFO, msg_text);
    i = 1;
    if(ioctlsocket(TestSock, FIONBIO, (unsigned long *) &i)) {
        sprintf(msg_text, "comread:  FIONBIO (Setting nonblocking) error - %s.\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_IOCTL2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_IOCTL2);
    }
#else
	sprintf(msg_text, "%s\nBeginning test...BLOCKING READS--version %8x, pid=%d", ConnectStr,VER,getpid());
    hxfmsg(stats, HTXERROR(HEINFO10,0), HTX_HE_INFO, msg_text);
#endif

    pak     = -1;
    LastAck = -1;
    rwParms->SigAlarmFlag = 0;
    rwParms->stats = stats;

    while(1) {
		for(stanza=0; stanza < NoStanzas; stanza++) {
            lstats = &LSTATS[stanza];
            rwParms->timeout.tv_sec  = rule[stanza].alarm;
            rwParms->timeout.tv_usec = 0;
            ShutdownMask = rule[stanza].shutdown;
			num_pktsizes_req = rule[stanza].num_oper;
			if(rule[stanza].bufinc == 0) { 
				bcnt_max = 0; 
			} else { 
				if(rule[stanza].bufinc > 0) { 
					bcnt_max = ((rule[stanza].bufmax - rule[stanza].bufmin ) / rule[stanza].bufinc); 
				} else { 
					bcnt_max = 0 ; 
				} 
			} 
			if(bcnt_max == 0 ) 
				bcnt_max = 1 ;

            stats->test_id = stanza + 1;
            hxfupdate(UPDATE, stats);

			for(bcnt = 0; bcnt < bcnt_max; bcnt++) {
            	/* I am starting a new packet size loop, check if stanza specified random packet types,
             	 * If yes, then send a message to comsem to get the random packet sizes
             	 * If no, increment bsize and go for num_oper loop
             	 */

            	if(rule[stanza].bufinc == RANDOM) {
                	rc = ReqPktSize(WriteIdx, rule[stanza].bufmin, rule[stanza].bufmax, rule[stanza].num_oper, bsize, &num_pktsizes, stats, *SemServID, 0);
                	if(rc) {
                    	sprintf(msg_text, "comread: Unable to get pktsize from Server, rc = %d \n", rc);
                    	hxfmsg(stats, HTXERROR(rc, ERRNO), HTX_HE_HARD_ERROR, msg_text);
                    	HE_exit(EX_RULEREAD5);
                	}
            	} else {
					num_pktsizes = rule[stanza].num_oper ;
                	for(i = 0; i < rule[stanza].num_oper; i ++) {
                        bsize[i] = rule[stanza].bufmin + bcnt * rule[stanza].bufinc ; 
					} 
				} 
				num_oper = rule[stanza].num_oper;
				if(num_pktsizes != rule[stanza].num_oper) 
					num_oper = num_pktsizes;

			#ifdef __DEBUG__
			   	sprintf(msg_text, "Reader,pid=%d, Idx = %d, bcnt = %d, \n", getpid(), WriteIdx, bcnt);
    			hxfmsg(stats, HTXERROR(HEINFO10,0), HTX_HE_INFO, msg_text);
			#endif 


                for(i=0; i<num_oper ; i++) {
				
                	memcpy(patternfile, PATTERN, bsize[i]);
                	pakLen = lstats->rw_size = bsize[i] + 2 * (TP_HDR_LEN);
                	pattern_trailer = pattern + bsize[i] + TP_HDR_LEN;

                	pak = (pak+1)%(TP_MAX_SEQNOS);
                	memset(rbuf, 'K', pakLen);
                	BadCnt = 0;
                	HDR_SEQNO(pattern, pattern_trailer, pak);
                	HDR_TS(pattern, pattern_trailer, timestamp);
                	HDR_CKSUM(pattern, pattern_trailer);
					/*if debug_pattern is set put pak seq in every 128 bytes*/
					/*put in pak and counter in wbuf (pattern) data that */
					/* will be compared to the data read on next read (rbuf) */
					if(rule[stanza].debug_pattern && pakLen >= 512) {
						for(o=128; o < pakLen-128; o+=128) {
							debug_ptr = pattern + o;
							INSERT_PAK_SEQNO(debug_ptr, pak);
							debug_ptr = pattern + o + TP_SEQ_LEN;
							INSERT_BLK_SEQNO(debug_ptr, o);
						}
						
						/* to force a miscompare uncomment the next two line */
						/* 
							debug_ptr = pattern + 210;
							INSERT_PAK_SEQNO(debug_ptr, pak);
						*/
					}

					index=-1;
                    if(shm_pHXECOM->SigTermFlag == 0
                       && shm_pHXECOM->SigUsr1Flag == 0
                       && (rc = (*readfn)(TestSock, (char *) rbuf, pakLen, bufLen, rwParms,stats,ConnectStr)) == pakLen
                       && (index=do_compare(rbuf, pattern, pakLen,&rule[stanza],stats,ConnectStr,1)) < 0) {
                        ConsecErrCnt = 0;
                        /****************************************************/
                        /* Statistic rules:                                 */
                        /*    max of 1 bad read per physical read()         */
                        /*    Number of error messages = BadCnt            */
                        /*    bad_bytes_read() OR  good_bytes_read() OR     */
                        /*       bad_other() called once per read()         */
                        /****************************************************/
                        good_bytes_read(lstats, pstats, stats, rc, ConnectStr);
                    }
                    else {
						rPak = atoi(rbuf); /*set the packet number read*/
                        if(shm_pHXECOM->SigTermFlag || shm_pHXECOM->SigUsr1Flag) {
                            /********************************************/
                            /* Give writer a chance to terminate before */
                            /* we shutdown socket.                      */
                            /********************************************/
							SendTermToSemServ(WriteIdx, SemServID, stats, 0);
                            hxfupdate(UPDATE, stats);
                            sleep(2);
                            shutdown(TestSock, 2);
                            sprintf(msg_text, "Fast socket shutdown due to SIGTERM signal.\n");
                            ReadExit(stats, LSTATS, NoStanzas, HTXERROR(EX_READ13,0), msg_text, ConnectStr);
                        }
                        if(rc == 0 && (int)rule[0].layer == (int)TCP_LAYER) {
                            /********************************************/
                            /* Nothing to read. QUIT.                   */
                            /********************************************/
                            sprintf(msg_text, "%s\nSocket has been closed - Exiting.\n", ConnectStr);
                            RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_SOCK4,0), HTX_HE_INFO, msg_text);
                            good_other(lstats, pstats, stats);
                            hxfupdate(UPDATE, stats);
                            msg_text[0] = '\0';
                            ReadExit(stats, LSTATS, NoStanzas, HTXERROR(EX_SOCK4,0), msg_text, ConnectStr);
                        } else  
                        if(rc== -1) {
                            if(++ConsecErrCnt == 3) {
                                sprintf(msg_text, "%s\n1. Read failed - %s.\n", ConnectStr, STRERROR(errno));
                                sprintf(msg_text + strlen(msg_text), "Writer not responding - Exiting.\n");
                                RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ1,ERRNO), HTX_HE_HARD_ERROR, msg_text);
                                bad_other(lstats, pstats, stats);
                                /********************************************/
                                /* Nothing to read. QUIT.                   */
                                /********************************************/
                                hxfupdate(UPDATE, stats);
                                msg_text[0] = '\0';
                                ReadExit(stats, LSTATS, NoStanzas, HTXERROR(EX_READ1,ERRNO), msg_text, ConnectStr);
                            }
                            if(rwParms->SigAlarmFlag) {
                                rwParms->SigAlarmFlag = 0;
                                sprintf(msg_text, "%s\n2. Read failed - %s.\n", ConnectStr, STRERROR(errno));
                                sprintf(msg_text + strlen(msg_text), "IO_ALARM_TIME = %d seconds was exceeded.\n", (int) rule[stanza].alarm);
								RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
                                bad_other(lstats, pstats, stats);
                                CMsg.ID.SemID.timeS = 0xff & timestamp;
                                SHUTDOWNTEST(SH_FORCE);
                            }
                            else if(ERRNO == WSAEINTR) {
                                sprintf(msg_text, "%s\n3. Read failed - %s.\n", ConnectStr, STRERROR(errno));
                                sprintf(msg_text + strlen(msg_text), "Unknown Interrupt\n");
                                RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ3,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
                                bad_other(lstats, pstats, stats);
                                /********************************************/
                                /* Reset i loop counter and try again.      */
                                /********************************************/
                                SHUTDOWNTEST(SH_EINTR);
                                i--;
                                pak--;
                                continue;
                            }
                            else if(ERRNO == ECONNRESET || ERRNO == EPIPE ) { 
								/* The writer went away this reader will stop */
								sprintf(msg_text,"Reader will just exit: Writer quit %s\n",STRERROR(errno));
								hxfmsg(stats, HTXERROR(0,0), HTX_HE_INFO, msg_text);
								HE_exit(0);
							}
                            else {
								SendTermToSemServ(WriteIdx, SemServID, stats, 0);
                                sprintf(msg_text, "%s\n4. Read failed - %s.\n", ConnectStr, STRERROR(errno));
                                sprintf(msg_text + strlen(msg_text), "Exiting\n");
                                RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ4,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
                                bad_other(lstats, pstats, stats);
                                /********************************************/
                                /* Quit since don't know what happen.       */
                                /********************************************/
                                hxfupdate(UPDATE, stats);
                                msg_text[0] = '\0';
                                ReadExit(stats, LSTATS, NoStanzas, HTXERROR(EX_READ4,ERRNO), msg_text, ConnectStr);
							}
                        } else {  
                      	  	if(rc > 0 && rc != pakLen) {
                            	if(ERRNO == ECONNRESET || ERRNO == EPIPE) {
                                	/* this won't happen with MSG_WAITALL set on the recv since it is supposed to get all
                                    	or none, and SRead will return -1 even if it gets some data. 
                                	*/
                                	sprintf(msg_text, "%s\n Receviced down messages from the other end of the socket. Error - %s, only received %d bytes expected %d\n",
                                                    ConnectStr, STRERROR(ERRNO),rc,pakLen);
                                	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ6,0), HTX_HE_INFO, msg_text);
                                	HE_exit(0);
                            	} else {
                                	sprintf(msg_text, "%s\n(Packet number, characters) (%d,%d) read, expected (%d,%d) .\n",
                                                                            ConnectStr, rPak, rc, pak, pakLen);
                                	sprintf(msg_text + strlen(msg_text), "5. Read failed - %s.\n", STRERROR(ERRNO));
                                	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ7,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
                                	bad_bytes_read(lstats, pstats, stats, rc, ++BadCnt);
                                	SHUTDOWNTEST(SH_PAKLEN);
                            	}
                        	}
						}
						/* If running with NO_COMPARE = 1 or with UDP packets, 
						   Header checking, pkt rcvd out of order checks not required */ 
						if(rule[stanza].no_compare ==1 || rule[stanza].layer == UDP_LAYER) 
							continue; 
                        ConsecErrCnt = 0;
						/* if there is a bad header and no miscompare */
						/* then report the bad header, but if theres a */
						/* misc let it fall through to be handled as misc */
                        if(headerCkFail(rbuf, stats->msg_text) && index < 0) {
							if(not_running_bootme()) {
								if(ShutdownMask & SH_ATTN_MISCOMPARE) {
	#ifndef __HTX_LINUX__
									attn((int) MISC,(int)&pattern,(int)&rbuf,(int)&msgp,(int)&stats,(int)&rule);
	#else
									__attn((int) MISC,(int)&pattern,(int)&rbuf,(int)&msgp,(int)&stats,(int)&rule);
	#endif
								}
								if(ShutdownMask & SH_KDB_MISCOMPARE) {
	#ifndef __HTX_LINUX__
									/*trap((int) MISC,(int)&pattern, (int)&rbuf,(int)&msgp,(int)&stats,(int)&rule,(int)&bsize); */
									trap((int) MISC,pattern, rbuf,index,stats,rule,bsize,(int)VER,i);
	#else
									/*do_trap_htx64((int) MISC,(int)&pattern, (int)&rbuf,(int)&msgp,(int)&stats,(int)&rule,(int)&bsize);*/
									/*              r3       r4     r5  r6     r7   r8   r9     r10 r11*/
                                                                        do_trap_htx64((int)MISC,pattern, rbuf,index,stats,rule,bsize,(int)VER);
	#endif
								}
								sprintf(msg_text, "%s\n%s\n", ConnectStr, stats->msg_text);
								RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ5,0), HTX_HE_SOFT_ERROR, msg_text);
								bad_bytes_read(lstats, pstats, stats, rc, ++BadCnt);
								sprintf(msg_text, "%s\nMiscompare info for header failure, data size = %d.\n",
																ConnectStr, bsize);
								memset(msgp, 0, MAX_TEXT_MSG);
								rc = hxfcbuf(stats, pattern, rbuf, pakLen, msgp);
								sprintf(msg_text+strlen(msg_text), "%s\n", msgp);
								RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ5,0), HTX_HE_SOFT_ERROR, msg_text);

								SHUTDOWNTEST(SH_BADHDR);
								/********************************************/
								/* Pause writer. write_ahead = 0.        */
								/********************************************/

								SendResetToSemServ(WriteIdx, 0, timestamp, i, bsize[i], stanza, pak, SemServID, stats, 0);
								/********************************************/
								/* Attempt to read all remaining data on    */
								/* socket.  i.e. flush, so can resync start */
								/* of header.                               */
								/********************************************/
								do {
									 rc = (*readfn)(TestSock, (char *) rbuf, bufLen,bufLen, rwParms,stats,ConnectStr);
								} while(rc != -1);

								/********************************************/
								/* reset and retart writer.                 */
								/********************************************/

								SendResetToSemServ(WriteIdx, rule[stanza].write_ahead, (timestamp = ++timestamp%256), i, bsize[i],
                                                        stanza, pak, SemServID, stats, 0);

								sprintf(msg_text, "Resetting writer to i=%d, bsize=%d, timestamp=%d, pak=%d.\n",
											 i, bsize, timestamp, pak);
								RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ5,0), HTX_HE_INFO, msg_text);


								bad_other(lstats, pstats, stats);
								CMsg.ID.SemID.timeS = 0xff & timestamp;
								/********************************************/
								/* Reset i loop counter and try again.      */
								/********************************************/
								i--;
								pak--;
								continue;
							}
                        }
						/* note on miscompare, index <0 rbuf can be corrupted */
						/* so rPak can be junk */
						/* if timestamp mismatch and no miscompare */
                        if(timestamp != rbuf[TP_TIME_POS] && index < 0) {
                            sprintf(msg_text, "%s\n(Timestamp,Seq no), read (%d,%d), expected (%d,%d).\n", ConnectStr, (int)rbuf[TP_TIME_POS], rPak, (int)timestamp, pak);
                            RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ6,0), HTX_HE_INFO, msg_text);
                            bad_bytes_read(lstats, pstats, stats, rc, ++BadCnt);
                            /************************************************/
                            /* Reset i loop counter and try again.          */
                            /************************************************/
                            SHUTDOWNTEST(SH_TIMESTAMP);
                            i--;
                            pak--;
                            continue;
                        }
						/*If the packet number is wrong and there is no 
						  miscompare report the pak seq error */
                        if(rPak != pak && index < 0) {
                            sprintf(msg_text, "%s\n(Packet number), read (%d),  expected (%d).\n", ConnectStr, rPak, pak);
                            /************************************************/
                            /* Check for a packet received out-of order.    */
                            /* If received out-of-order we have previously  */
                            /* counted this packet as a bad read. So, count */
                            /* this as a bad other and reset loop parameter */
                            /* to values before we read this packet.        */
                            /************************************************/
                            if(pak + TP_MAX_SEQNOS * (pak<rPak) - rPak <= rule[stanza].write_ahead) {
                                sprintf(msg_text + strlen(msg_text), "Duplicate packet or packet delivered out-of-order\n");
                                RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ8,0), HTX_HE_SOFT_ERROR, msg_text);
                                bad_other(lstats, pstats, stats);
                                SHUTDOWNTEST(SH_DUPPAK);
                                i--;
                                pak--;
                                continue;
                            }
                            else if(rPak + TP_MAX_SEQNOS * (pak>rPak) - pak <= rule[stanza].write_ahead) {
                                sprintf(msg_text + strlen(msg_text), "Packet(s) dropped.\n");
                                RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ9,0), HTX_HE_SOFT_ERROR, msg_text);
                                SHUTDOWNTEST(SH_DROPPAK);
                            }
                            else {
                                SendResetToSemServ(WriteIdx, rule[stanza].write_ahead, (timestamp = ++timestamp%256), i, bsize[i],
                                                        stanza, pak, SemServID, stats, 0);

                                sprintf(msg_text + strlen(msg_text), "Unknown error. Ignoring packet.\n");
                                sprintf(msg_text + strlen(msg_text),
                                    "Resetting writer to i=%d, bsize=%d, timestamp=%d, pak=%d.\n",
                                         i, bsize[i], timestamp, pak);
                                RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ10,0), HTX_HE_SOFT_ERROR, msg_text);
                                bad_other(lstats, pstats, stats);
                                CMsg.ID.SemID.timeS = 0xff & timestamp;
                                /********************************************/
                                /* Reset i loop counter and try again.      */
                                /********************************************/
								/* let it try to recover
                                SHUTDOWNTEST(SH_UNKPAK); 
								*/
                                i--;
                                pak--;
                                continue;
                            }
                            /************************************************/
                            /* We have probably received a good packet and  */
                            /* also failed to receive rPak - pak packets.   */
                            /* We will report to stats the dropped packets  */
                            /* and treat the received packet as a good one. */
                            /* At least for now.                            */
                            /************************************************/
                            delta = (rPak + TP_MAX_SEQNOS - pak)%(TP_MAX_SEQNOS);
                            BadCnt += 1;
                            for(loop=0; loop < delta; loop++) {
                                bad_bytes_read(lstats, pstats, stats, rc, BadCnt);
                                BadCnt = 1;
                            }
                            BadCnt = 0;
                            if((num_oper -i) > delta) {
                                i = (i+delta)%num_oper;
                                pak = rPak;
                                /********************************************/
                                /* Ack current packet.                      */
                                /********************************************/
                                CMsg.ID.SemID.ackNo = htons((u_short) 0xffff & pak);
                                CMsg.ID.SemID.timeS  = 0xff & timestamp;
								SendReadAckToSemServ(&CMsg, SemServID, stats, 0);	
                                LastAck = pak;
                                good_bytes_read(lstats, pstats, stats, rc, ConnectStr);
                            }
                            else {
                                /********************************************/
                                /* Not easy to recover when stanza has      */
                                /* incremented, so we will reset both the   */
                                /* reader and writer to a known state.      */
                                /********************************************/
                                sprintf(msg_text + strlen(msg_text),
                                    "Stanza incremented and reader got unknown packet. Exit test.\n");
                                RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ11,0), HTX_HE_HARD_ERROR, msg_text);
								/* don't try to recover */
                                SHUTDOWNTEST(SH_FORCE);
                            }
                        }
                        if(index >= 0) {
							if(not_running_bootme()) {

								if (stats->miscompare_count > 10) {
										stats->miscompare_count=0;
								}
								if(ShutdownMask & SH_ATTN_MISCOMPARE) {
								#ifndef __HTX_LINUX__
									attn((int) MISC,(int)&pattern,(int)&rbuf,(int)&msgp,(int)&stats,(int)&rule);
								#else
									__attn((int) MISC,(int)&pattern,(int)&rbuf,(int)&msgp,(int)&stats,(int)&rule);
								#endif
								}
								if(ShutdownMask & SH_KDB_MISCOMPARE) {

								#ifndef __HTX_LINUX__
									/*trap((int) MISC,(int)&pattern, (int)&rbuf,(int)&msgp,(int)&stats,(int)&rule,(int)&bsize[i]); */
									trap((int) MISC,pattern, rbuf,index,stats,rule,bsize[i],(int)VER,i);
								#else
									/*do_trap_htx64((int) MISC,(int)&pattern, (int)&rbuf,(int)&msgp,(int)&stats,(int)&rule,(int)&bsize[i]);*/
									/*              r3       r4     r5  r6   r7   r8 */
									do_trap_htx64((int)MISC,pattern, rbuf,index,stats,rule,bsize[i],(int)VER);
								#endif
								/*
									in kdb the regs have pointer to the location of the pointer, so you 
									have to find value in reg, ex: 000000002FF1E9E0,  the do 
									dw 000000002FF1E9E0    that will show you this:
									2FF1E9E0: 2FF1C1D4 1FFF0128 00000000 00000004  
									The do dw 2FF1C1D4 30
									to see 30 bytes of the data in rbuf. 
								*/

								}
								sprintf(msg_text, "%s\nMiscompare with packet %d, data size = %d. miscompare_count=%d pakLen = %d \n", 
																ConnectStr, rPak, bsize[i],stats->miscompare_count, pakLen);
								memset(msgp, 0, MAX_TEXT_MSG);
								rc = hxfcbuf(stats, pattern, rbuf, pakLen, msgp);
								sprintf(msg_text+strlen(msg_text), "%s\n", msgp);
								RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_READ11,0), HTX_HE_SOFT_ERROR, msg_text);
								bad_bytes_read(lstats, pstats, stats, rc, ++BadCnt);
								SHUTDOWNTEST(SH_MISCOMPARE);
							}
                        }
                    } /* end of else for error checking */ 

                } /* END OF         for(i=0;i<num_oper;i++)  */
                /********************************************************/
                /* Since we are possibly changing the buffer size,      */
                /* hxfupdate must be called before bsize is changed     */
                /* again.                                               */
                /********************************************************/

                hxfupdate(UPDATE, stats);
            } /* END OF               for(bcnt= ; bcnt < = ; bcnt++ ) */
			/* 
            sprintf(msg_text, "Reader %d, stanza %d, cum stats - good reads %d, bad reads %d\n",
                     (int)ntohs(ReaderID.sin_port), stanza, lstats->totals.good_rw, lstats->totals.bad_rw);
			hxfmsg(stats, HTXERROR(EX_WRITE8, ERRNO), HTX_HE_INFO, msg_text);
			*/ 
        } /* END OF           for(stanza=0; stanza < NoStanzas; stanza++) */
        hxfupdate(FINISH, stats);
    } /* END OF           while(1)                                        */
    return 0;
}



static int
ReqPktSize(int WriteIdx, u_short bufmin, u_short bufmax, u_short num_oper, u_short * bsize, int * num_pktsizes, 
			struct htx_data * stats, struct sockaddr_in dest, u_long state ) { 
	
	/* Reader sends GET_PKTSIZES to comsem, wait for ack from writer which should contain
     * new set of random packet sizes
	 */ 
	struct CoordMsg CMsg;
	int rc = 0 , i ; 
	char msg_text[100]; 
	int num_pktsize_req; 
	memset(bsize, 0, (sizeof(u_short) * USHRT_MAX));
	SOCKET SemServSock ; 

    if(bufmax == bufmin) { 
		bufmin = 10 ;
		sprintf(msg_text, "R : stress_level = 6 and buf_min = buf_max = %u. Considering buf_min = %d \n", bufmax, bufmin);  
		hxfmsg(stats, 0, HTX_HE_INFO, msg_text); 
	}
	SemServSock = SetUpConnect(&dest, stats, state); 

	num_pktsize_req = num_oper ; 	
	CMsg.msg_type = htonl(GET_PKTSIZES);
	CMsg.ID.Bsize.WriterIdx = htons((u_short) 0xffff & WriteIdx);
	CMsg.ID.Bsize.bufmin = htons((u_short) 0xffff & bufmin);
	CMsg.ID.Bsize.bufmax = htons((u_short) 0xffff & bufmax); 
	CMsg.ID.Bsize.num_oper = htons((u_short) 0xffff & num_oper) ; 
	CMsg.ID.Bsize.num_pktsizes = htons((u_short) 0xffff & num_pktsize_req) ; 
#ifdef __DEBUG__
	sprintf(msg_text, "Reader Sending : GET_PKTSIZES to SemServSock \n"); 
	hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_INFO, msg_text); 
#endif 
	rc = StreamWrite(SemServSock, (char *)&CMsg, sizeof(CMsg));
	if(rc == -1) { 
		sprintf(msg_text, "Error : Sending GET_PKTSIZES to SemServSock - %s \n", STRERROR(errno));
		hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
      	return(rc);  
    }
	memset(&CMsg, 0, sizeof(struct CoordMsg));
#ifdef __DEBUG__
	sprintf(msg_text, "Reader Waiting for packet from SemServer \n"); 
	hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_INFO, msg_text);
#endif
	rc = StreamRead(SemServSock, (char *)&CMsg, sizeof(struct CoordMsg));
	/*NetToHostBsize_t(&CMsg.ID.Bsize); */

	if(rc == -1 || ntohl(CMsg.msg_type ) != PKTSIZES) { 
		sprintf(msg_text, "Error : getting number of packets random packet sizes generated -rc = %d, %s \n", rc, STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE8,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        return(EX_WRITE8);
    }
	if(ntohs(CMsg.ID.Bsize.bufmin) != bufmin || ntohs(CMsg.ID.Bsize.bufmax) != bufmax || ntohs(CMsg.ID.Bsize.num_oper) != num_oper) { 
		sprintf(msg_text, "Error : PKTSIZES corrupted. Actual buffers = %d, %d, %d. Expected = %d, %d, %d \n",
		ntohs(CMsg.ID.Bsize.bufmin), ntohs(CMsg.ID.Bsize.bufmax), ntohs(CMsg.ID.Bsize.num_oper), bufmin, bufmax, num_oper);
		hxfmsg(stats, HTXERROR(EX_WRITE8,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        return(EX_WRITE8);
    }
 
	if(ntohs(CMsg.ID.Bsize.num_pktsizes) != num_pktsize_req) { 
		sprintf(msg_text, "Fewer Number of random packet sizes generated Requested = %d, Actual = %d \n",num_pktsize_req, CMsg.ID.Bsize.num_pktsizes); 
		hxfmsg(stats, 0, HTX_HE_INFO, msg_text);
		num_pktsize_req = CMsg.ID.Bsize.num_pktsizes ; 
	}
	rc = StreamRead(SemServSock, (char *)bsize, (sizeof(u_short) * num_pktsize_req)) ;
#ifdef __DEBUG__
	sprintf(msg_text, "Reader :  Receiving Random pktsizes rc = %d \n",rc); 
	hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_INFO, msg_text);
#endif
	if(rc == -1 || ntohl(CMsg.msg_type ) != PKTSIZES) { 
		sprintf(msg_text, "Error : getting PKTSIZES from SemServSock - %s \n", STRERROR(errno));
		hxfmsg(stats, HTXERROR(EX_WRITE8,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
        return(EX_WRITE8);
    }
	for(i = 0; i < num_pktsize_req; i++) { 
		bsize[i] = ntohs(bsize[i]);
	} 
	/* Send an ACK_PKTSIZES to writer, to unblock writer */ 
	memset(&CMsg, 0, sizeof(struct CoordMsg)); 
	CMsg.msg_type = htonl(ACK_PKTSIZES); 
	CMsg.ID.Bsize.WriterIdx = htons((u_short) 0xffff & WriteIdx);
	CMsg.ID.Bsize.num_pktsizes = htons((u_short) 0xffff & num_pktsize_req) ;
	rc = StreamWrite(SemServSock, (char *)&CMsg, sizeof(CMsg)); 
#ifdef __DEBUG__
	sprintf(msg_text, "Reader :  Sending ACK_PKTSIZES to Writer \n"); 
	hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_INFO, msg_text); 
#endif 
	if(rc == -1) {
		sprintf(msg_text, "Error : Sending ACK_PKTSIZES to SemServSock - %s \n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        return(EX_WRITE5);
    }
	*num_pktsizes = num_pktsize_req ; 
	close(SemServSock);
	return(0); 
} 		

static void
SendResetToSemServ(int WriteIdx, int WriteAhead, unsigned char timestamp , int i, int bsize, int stanza , int pak, 
					struct sockaddr_in * dest, struct htx_data * stats, u_long state) { 

	SOCKET SemServSock; 
	struct CoordMsg CMsg;
	char msg_text[100]; 
	int rc ; 

	SemServSock = SetUpConnect(dest, stats, state); 
	CMsg.msg_type = htonl(CM_RESET_WRITE);
    CMsg.ID.SemID.WriteIdx   = htons((u_short) 0xffff & WriteIdx);
    CMsg.ID.SemID.WriteAhead = (unsigned char)  0xff & WriteAhead;
    CMsg.ID.SemID.timeS      = (unsigned char)  0xff & timestamp;
    CMsg.ID.SemID.Stanza     = (unsigned char)  0xff & stanza;
    CMsg.ID.SemID.bsize      = htons((u_short) 0xffff & bsize);
    CMsg.ID.SemID.Iloop      = htons((u_short) 0xffff & i);
    CMsg.ID.SemID.ackNo      = htons((u_short) 0xffff & pak);

	rc = StreamWrite(SemServSock, (char *)&CMsg, sizeof(CMsg));
#ifdef __DEBUG__
	sprintf(msg_text, "sending CM_RESET_WRITE to SemServ, rc = %d write_ahead= %d\n", rc, CMsg.ID.SemID.WriteAhead); 
	hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_INFO, msg_text);
#endif
	if(rc == -1) {
        sprintf(msg_text, "Error : Sending CM_RESET_WRITE to SemServSock - %s \n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
    }

	closesocket(SemServSock); 
}

static void
SendTermToSemServ(int WriteIdx, struct sockaddr_in * dest, struct htx_data * stats, u_long state) { 
	SOCKET SemServSock;
	struct CoordMsg CMsg;
    char msg_text[100];
	int rc ; 

	SemServSock = SetUpConnect(dest, stats, state);
	CMsg.msg_type = htonl(CM_TERM_WRITE);
    CMsg.ID.SemID.WriteIdx   = htons((u_short) 0xffff & WriteIdx);
	
	rc = StreamWrite(SemServSock, (char *)&CMsg, sizeof(CMsg));
	sprintf(msg_text, "sending CM_TERM_WRITE to SemServ, rc = %d \n", rc);
    hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_INFO, msg_text);

    if(rc == -1) {
        sprintf(msg_text, "Error : Sending CM_RESET_WRITE to SemServSock - %s \n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
    }
	closesocket(SemServSock);

}

static void
SendHaltToSemServ(int WriteIdx, struct sockaddr_in * dest, struct htx_data * stats, u_long state) {
	SOCKET SemServSock;
    struct CoordMsg CMsg;
    char msg_text[100];
	int rc ; 

    SemServSock = SetUpConnect(dest, stats, state);
	CMsg.msg_type = htonl(CM_HALT_WRITE);
    CMsg.ID.SemID.WriteIdx   = htons((u_short) 0xffff & WriteIdx);
	rc = StreamWrite(SemServSock, (char *)&CMsg, sizeof(CMsg));
    sprintf(msg_text, "sending CM_TERM_WRITE to SemServ, rc = %d \n", rc);
    hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_INFO, msg_text);

    if(rc == -1) {
        sprintf(msg_text, "Error : Sending CM_RESET_WRITE to SemServSock - %s \n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
    }
    closesocket(SemServSock);

}

static void 
SendReadAckToSemServ(struct CoordMsg * CMsg, struct sockaddr_in * dest, struct htx_data * stats, u_long state) { 

	SOCKET SemServSock;
	char msg_text[100];
	int rc ; 

	SemServSock = SetUpConnect(dest, stats, state);
	rc = StreamWrite(SemServSock, (char *)CMsg, sizeof(struct CoordMsg)); 
	if(rc == -1) {
        sprintf(msg_text, "Error : Sending CM_RESET_WRITE to SemServSock - %s \n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
    }

	closesocket(SemServSock);
    return;
}

static void WriteLocalStats(struct cum_rw LSTATS[], int NoStanzas, char ConnectStr[], struct htx_data * stats, char *msg_text)
{
    int    stanza;
    FILE * config_des = NULL;
    char temp_string[300];
	/*return;*/


    sprintf(temp_string, "%s/%s", global_htx_log_dir, LSTAT_FILE);
    config_des = fopen(temp_string, "a");

    GlobalWait(FILE_SEM, stats);

    fprintf(config_des, "%s\n", ConnectStr);
    for(stanza=0; stanza<NoStanzas; stanza++) {
        fprintf(config_des, "STANZA %d\n", stanza);
        fprintf(config_des, "good reads         = %18u\n", LSTATS[stanza].totals.good_rw);
        fprintf(config_des, "bad reads          = %18u\n", LSTATS[stanza].totals.bad_rw);
        fprintf(config_des, "good others        = %18u\n", LSTATS[stanza].totals.good_others);
        fprintf(config_des, "bad others         = %18u\n", LSTATS[stanza].totals.bad_others);
        if(LSTATS[stanza].totals.bad_bytes_rw1 > 0)
            fprintf(config_des, "bad bytes read     = %9lu%09lu\n",
                      LSTATS[stanza].totals.bad_bytes_rw1, LSTATS[stanza].totals.bad_bytes_rw2);
        else {
            fprintf(config_des, "bad bytes read     = %18lu\n", LSTATS[stanza].totals.bad_bytes_rw2);
        }
        if(LSTATS[stanza].totals.good_bytes_rw1 > 0)
            fprintf(config_des, "good bytes read    = %9lu%09lu\n",
                      LSTATS[stanza].totals.good_bytes_rw1, LSTATS[stanza].totals.good_bytes_rw2);
        else
            fprintf(config_des, "good bytes read    = %18lu\n", LSTATS[stanza].totals.good_bytes_rw2);
        fprintf(config_des, "\n");
    }

    GlobalSignal(FILE_SEM, stats);
    fclose(config_des);
}



static void RWmsg(struct cum_rw * Lstats, struct rule_format * rule, int Stanza,
                                struct htx_data * sstats, int err, enum sev_code sev, char * msgtxt)
{
    sprintf(msgtxt + strlen(msgtxt),
           "For stanza %d, %s connection, bufmin=%d, bufmax=%d, bufinc=%d\nConsecutive prior good reads=%9u,  good bytes read=",
              Stanza, (rule->layer == (int) UDP_LAYER) ? "UDP" : "TCP",
                rule->bufmin, rule->bufmax, rule->bufinc, Lstats->current.good_rw);
    if(Lstats->current.good_bytes_rw1 == 0)
        sprintf(msgtxt + strlen(msgtxt),"%18lu\n",  Lstats->current.good_bytes_rw2);
    else
        sprintf(msgtxt + strlen(msgtxt),"%9lu%09lu\n", Lstats->current.good_bytes_rw1, Lstats->current.good_bytes_rw2);

    Lstats->current.bad_others     = 0;
    Lstats->current.bad_rw         = 0;
    Lstats->current.bad_bytes_rw1  = 0;
    Lstats->current.bad_bytes_rw2  = 0;
    Lstats->current.good_bytes_rw1 = 0;
    Lstats->current.good_bytes_rw2 = 0;
    Lstats->current.good_others    = 0;
    Lstats->current.good_rw        = 0;

    hxfmsg(sstats, err, sev, msgtxt);
}



static void ReadExit(struct htx_data * sstats, struct cum_rw * LSTATS, int NoStanzas, int exitNo,
                                    char msg[], char ConnectStr[])
{
    WriteLocalStats(LSTATS, NoStanzas, ConnectStr, sstats, msg + strlen(msg) + 1);
    sprintf(msg + strlen(msg), "%s\nStats Updated. Exiting.\n", ConnectStr);
    hxfmsg(sstats, exitNo, HTX_HE_INFO, msg);
    HE_exit(exitNo>>16);
}



static int SRead(SOCKET fd, register char *ptr, register int nbytes, int nmax, struct rw_t * rwParms,struct htx_data * stats, char *connect_str)
{
    int nleft;
    int nread;
    int src;
    fd_set rd;
    struct timeval timeout;
    int nfd;

    errno = 0;
    nleft = nbytes;
    while(nleft > 0) {
#ifndef NO_BLOCK
        alarm(rwParms->timeout.tv_sec);     /* set alarm timeout */
#endif
        /* changed flag from 0 to MSG_WAITALL */
        if((nread = recv(fd, ptr, nleft, MSG_WAITALL)) > 0) {
            nleft -= nread;
            ptr += nread;
#ifndef NO_BLOCK
            alarm(0);   /* cancel alarm timeout */
#endif
            /* I have to call recv again to get the real errno , it will not */
            /* set errno if it returns any data.  keep reading as long as 
				we are not terminating the test */
            continue;
        }
        else if (nread == SOCKET_ERROR && errno == EINTR && shm_pHXECOM->SigTermFlag == 0 && shm_pHXECOM->SigUsr1Flag == 0 && shm_pHXECOM->SigAlarmFlag == 0) {
            ptr = ptr +nleft - nbytes;
            nleft = nbytes;
/*
            sprintf(stats->msg_text, "SRead2: %s\nSocket err, EINTR, retry recv,nleft=%d,nread =%d,nbytes=%d\n",connect_str,nleft,nread,nbytes);
            hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
*/

#ifndef NO_BLOCK
            alarm(0);   /* cancel alarm timeout */
#endif
            continue;
        }
#ifndef NO_BLOCK
        else if(errno == EINTR) {
            rwParms->SigAlarmFlag = 1;
            errno = WSAEINTR;
            alarm(0);   /* cancel alarm timeout */
            return -1;
        }
#else

        else if(nread == SOCKET_ERROR &&
                            ((errno == EWOULDBLOCK) ||
                             (errno == EINPROGRESS) ) ) {
            nfd = fd +1;
            FD_ZERO(&rd);
            FD_SET(fd, &rd);
            timeout = rwParms->timeout;
            if((NFDS(src = select(nfd, &rd, NULL, NULL, &timeout))) == 1) {
                continue;
            }
            else if(src == 0) {
                errno = WSAEINTR;
                if(nbytes == nleft) {
                    rwParms->SigAlarmFlag = 1;
                    return -1;
                }
                else {
                    break;
                }
            }
            else if(src == SOCKET_ERROR) {
                if(nbytes == nleft) {
                    return -1;
                }
                else {
                    break;
                }
            }
            else {
                errno = EIO;
                if(nbytes == nleft) {
                    return -1;
                }
                else {
                    break;
                }
            }
        }
#endif
        else if(nread == SOCKET_ERROR) {
#ifndef NO_BLOCK
			alarm(0);   /* cancel alarm timeout */
#endif
/*
            sprintf(stats->msg_text, "SRead_Serr: %s\nSocket_error, nleft=%d,nread=%d,nbytes=%d\n",connect_str,nleft,nread,nbytes);
            hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
*/
			/* read nothing return error*/
            if(nbytes == nleft) {
                return -1;
            }
            else {
                break;
            }
        }
        else if(nread == 0) {
#ifndef NO_BLOCK
			alarm(0);   /* cancel alarm timeout */
#endif
/*
            sprintf(stats->msg_text, "SRead_read0: %s\nnread == 0, nleft=%d,nread=%d,nbytes=%d\n",connect_str,nleft,nread,nbytes);
            hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
*/

            errno = WSAESHUTDOWN;
            if(nbytes == nleft) {
                return -1;
            }
            else {
                break;
            }
		}
        else {
#ifndef NO_BLOCK
			alarm(0);   /* cancel alarm timeout */
#endif
/*
            sprintf(stats->msg_text, "SRead_EIO: %s\nSet EIO, nleft=%d,nread=%d,nbytes=%d\n",connect_str,nleft,nread,nbytes);
            hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
*/

            errno = EIO;
            if(nbytes == nleft) {
                return -1;
            }
            else {
                break;
            }
        }
    }
#ifndef NO_BLOCK
	alarm(0);   /* cancel alarm timeout */
#endif
/*
    sprintf(stats->msg_text, "SRead_return: %s\nread %d bytes\n",connect_str,(nbytes-nleft));
    hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
*/

    return (nbytes -nleft);
}

static int DRead(SOCKET fd, register char *ptr, register int nbytes, int nmax, struct rw_t * rwParms, struct htx_data * stats, char *connect_str)
{

	int nleft;
    int nread;
    int src;
    fd_set rd;
    struct timeval timeout;
	int nfd; 

    errno = 0;
	nleft = nbytes;
    while(1) {
#ifndef NO_BLOCK
        alarm(rwParms->timeout.tv_sec);     /* set alarm timeout */
#endif
        if((nread = recvfrom(fd, ptr, nleft, NULL, NULL, 0)) > 0) {
			nleft -= nread;
            ptr += nread;
            continue;  
        }

        if (nread == SOCKET_ERROR && errno == EINTR && shm_pHXECOM->SigTermFlag == 0 && shm_pHXECOM->SigUsr1Flag == 0 && shm_pHXECOM->SigAlarmFlag == 0) {
            ptr = ptr +nleft - nbytes;
            nleft = nbytes;
#ifndef NO_BLOCK
            alarm(0);
#endif
		continue; 
		} 
#ifndef NO_BLOCK
        else if(errno == EINTR) {
            rwParms->SigAlarmFlag = 1;
            errno = WSAEINTR;
            alarm(0);   /* cancel alarm timeout */
            return -1;
        }
#else
        else if(nread == SOCKET_ERROR &&
                            ((errno == EWOULDBLOCK) ||
                             (errno == EINPROGRESS) ) ) {
            nfd = fd +1;
            FD_ZERO(&rd);
            FD_SET(fd, &rd);
            timeout = rwParms->timeout;
            if((NFDS(src = select(nfd, &rd, NULL, NULL, &timeout))) == 1) {
                continue;
            }
            else if(src == 0) {
                errno = WSAEINTR;
                rwParms->SigAlarmFlag = 1;
                return -1;
            }
            else if(src == SOCKET_ERROR) {
                return -1;
            }
            else {
                errno = EIO;
               	return -1; 
            }
        }
#endif
	   else if(nread == SOCKET_ERROR) {
            break;
        }
        else {
#ifndef NO_BLOCK
            alarm(0);   /* cancel alarm timeout */
#endif

            errno = EIO;
            return -1;
        }
    }
#ifndef NO_BLOCK
    alarm(0);   /* cancel alarm timeout */
#endif
    return (nbytes - nleft);
}


static int headerCkFail(char rbuf[], char MsgStr[])
{
    int i;
    int error=0;
    int sum = 0;

    for(i=0; i<TP_SEQ_LEN; i++) {
        switch(rbuf[i]) {
            case '-':
            case ' ':
            case '1':
            case '0':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                sum += (unsigned char)rbuf[i];
                break;
            default:
                error = 1;
        }
    }
    if(rbuf[TP_SEQ_LEN] != '\0')
        error = 1;
    sum += (unsigned char)rbuf[TP_TIME_POS];
    if((0xff & sum) != (unsigned char)rbuf[TP_HDR_LEN -1])
        error = 1;
    if(error) {
        sprintf(MsgStr, "Header Error \"");
        for(i=0; i<TP_HDR_LEN; i++)
            /*sprintf(MsgStr + strlen(MsgStr), "%2x%2x", rbuf[i],'\0');*/
            sprintf(MsgStr + strlen(MsgStr), "%2x", rbuf[i]);
        sprintf(MsgStr + strlen(MsgStr), "\"\n");
    }
    return error;
}



static void bad_bytes_read(struct cum_rw * Lstats, struct htx_data * pstats, struct htx_data * sstats,
                             int bytes, int BadCnt)
{
    if(BadCnt == 1) {
        sstats->bad_reads             += 1;
        pstats->bad_reads            += 1;
        Lstats->totals.bad_rw        += 1;
        Lstats->current.bad_rw       += 1;
        sstats->bytes_read            += bytes;
        /*pstats->bytes_read           += bytes; */
        addw(Lstats->totals.bad_bytes_rw1, Lstats->totals.bad_bytes_rw2, bytes);
        addw(Lstats->current.bad_bytes_rw1, Lstats->current.bad_bytes_rw2, bytes);
    }
    else {
        Lstats->totals.bad_others  += 1;
        Lstats->current.bad_others += 1;
        sstats->bad_others          += 1;
        /* pstats->bad_others         += 1; */
    }
}



static void good_bytes_read(struct cum_rw * Lstats, struct htx_data * pstats,
                                     struct htx_data * sstats, int bytes, char ConnectStr[])
{
    if((unsigned int)bytes != Lstats->rw_size) {
        sprintf(sstats->msg_text, "%s\nLast rw_size=%d, New rw_size=%d, Must be equal.\n",
                             ConnectStr, Lstats->rw_size, bytes);
        hxfmsg(sstats, HTXERROR(EX_READ15,0), HTX_HE_SOFT_ERROR, sstats->msg_text);
        HE_exit(EX_READ15);
    }
    Lstats->totals.good_rw  += 1;
    Lstats->current.good_rw += 1;
    sstats->good_reads       += 1;
    /* pstats->good_reads      += 1; */
    addw(Lstats->totals.good_bytes_rw1, Lstats->totals.good_bytes_rw2, bytes);
    addw(Lstats->current.good_bytes_rw1, Lstats->current.good_bytes_rw2, bytes);
    sstats->bytes_read       += bytes;
    /* pstats->bytes_read      += bytes; */
}

static int not_running_bootme() {
    int count=0;
	int sleep_cnt;
	/*if running bootme wait to see if we are at end of test. Miscompare is */
	/* not valid during shutdown */
    /*This routine will wait two minutes to see if the test is being stopped*/
	/*if running bootme */
    /*IF not it will let the miscompare be reported as usual but one minute*/
    /*later.. This lets the other side (on multi systems test) have time to */
	/* signal me if its running bootme, so I know to shutdown. */
	/* Which does not matter at all since you can't debug miscompares */
    /* by stopping in hxecom anyway. */
	if(shm_pHXECOM->bootme)  {
		sleep_cnt=180;   /* sleep 3 minutes */
	} else {
		sleep_cnt=60;   /* sleep 1 minute */
	}
	while(1) {
		if(shm_pHXECOM->SigUsr1Flag || shm_pHXECOM->SigTermFlag) {
			return(0);
		}
		usleep(500000);
		if(++count > sleep_cnt) {
			break;
		}
	}
    return(1);
}

static int do_compare(char *rbuf_ptr,char *pattern_ptr,int pak_size,struct rule_format *rule, struct htx_data *stats,char ConnectStr[],int fixpat) {
    int a,o;
	int d32, d8;
	/* cast char pointer to 32bit int pointer for 32bit diff */
	const unsigned int *p1 = (uint*)rbuf_ptr,   
			*p2 = (uint*)pattern_ptr;
	char *debug_ptr;
	char str_pid[16];
	int rc=-1;

    /* used to get max thruput, don't do data comapare for UDP packets */
    if(rule->no_compare || rule->layer == UDP_LAYER) 
		return(-1);

	d32 = pak_size / 4;  /* number of 32bit number */
	d8  = pak_size % 4;  /* remain byte need compare to handle old byte size */

	/* returns -1 if buffers match, so I can return the index to miscompare
	   if there is a miscompare */ 
	/* This version will ignore every 144th byte where the write puts the
       retry count when writes timeout and resend the full packet or part of
	   the packet */  
	if(rule->debug_pattern && fixpat && pak_size >= 512) {
		if(pid == 0) {
			/* this is first packet get the pid and devname */
			/* devname is at index 146 for and pid is at 160 */
			debug_ptr = rbuf_ptr+146;
			strcpy(writer_dev,debug_ptr);	
			debug_ptr = rbuf_ptr + 128 + 16 + 2 + strlen(writer_dev) + 4;
			strcpy(str_pid,debug_ptr);
			pid = atoi(str_pid);
		}
		/* insert the devname and pid in pattern so compare will work. */
		for(o=128; o < pak_size-128; o+=128) {
			debug_ptr = pattern_ptr + o + 16;
			debug_ptr[0]=0x01;  /* put in retry 1, this is what it should be*/
			debug_ptr[1]=0x00;  /* this byte is zeroed by writer */
			debug_ptr += 2;
			/* put in the dev name of writer */
			strcpy(debug_ptr,writer_dev);
			debug_ptr += strlen(writer_dev);
			/* need this to make sure there is no real */
			/* data between dev and pid */
			debug_ptr[0]=0x00;
			debug_ptr[1]=0x00;
			debug_ptr[2]=0x00;
			debug_ptr[3]=0x00;
			debug_ptr = pattern_ptr + o + 16 + 2 + strlen(writer_dev) + 4;
			INSERT_PID(debug_ptr,pid);
		}
	}

	if(memcmp(rbuf_ptr, pattern_ptr, pak_size) != 0) {  
		/* If there is a miscompare I have to return the index into the */
            /* buffer on a char boundary to find that index now */
            for(a=0;a<pak_size;a++) {
                if(rbuf_ptr[a] != pattern_ptr[a]) {
                    return (a);
                }
            }
	} 
	
	return(rc);
}
