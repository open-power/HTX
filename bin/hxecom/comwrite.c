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

/* @(#)94	1.25  src/htx/usr/lpp/htx/bin/hxecom/comwrite.c, exer_com, htx52F 5/24/04 17:27:55 */

/*
 *   COMPONENT_NAME: exer_com
 *
 *   FUNCTIONS: DWrite
 *		DecWriteAhead
 *		Detach_COMSEM
 *		ExitCleanup
 *		GateOnWriteAhead
 *		GetRdrID
 *		Req_StopReader
 *		GetUniqueWIdx
 *		HaltWrite
 *		IncWriteAhead
 *		InitWriteVars
 *		RWmsg
 *		ReadAck
 *		ResetWrite
 *		SHUTDOWNTEST
 *		SWrite
 *		SetWriteAhead
 *		ShutdownWrite
 *		TerminateWrite
 *		Terminating
 *		WriteExit
 *		WriteLocalStats
 *		bad_bytes_wrote
 *		comsem
 *		comwrite
 *		getCkSum
 *		good_bytes_wrote
 */


#    include <stdio.h>
#    include <stdlib.h>
#    include <unistd.h>
#    include <string.h>
#    include <memory.h>
#    include <time.h>
#    include <sys/types.h>
#    include <sys/socket.h>
#    include <sys/sem.h>
#    include <sys/time.h>
#    include <sys/select.h>
#    include <sys/ioctl.h>
#    include <netinet/in.h>
#    include <netinet/tcp.h>
#    include <netdb.h>
#    include <sys/shm.h>
#    include "hxecomdef.h"
#    include "comrw.h"

#ifdef __HTX_LINUX__
#    include "fcntl.h"
#    include <sys/stat.h>
#endif


#ifdef __HTX_LINUX__
#define ulong unsigned long
#define NFDS(a)   (a)
#endif

/*define NO_BLOCK 1 */

/******************************************************************************/
/***  Global Variable Definitions shared only among threads. ******************/
/******************************************************************************/
    static struct shm_comsem_t * shm_COMSEM;
    static int SemID;


/******************************************************************************/
/***  End of Global Variable Definitions  *************************************/
/******************************************************************************/

static void RWmsg(struct cum_rw * Lstats, struct rule_format * rule, int Stanza, struct htx_data * stats, int err, enum sev_code sev, char * msg_text);
static int SWrite(SOCKET fd, register char *ptr, register int nbytes, struct rw_t *rwParms,int *bytes_written,struct htx_data * stats,char *connect_str);
static int DWrite(SOCKET fd, register char *ptr, register int nbytes, struct rw_t *rwParms,int *bytes_written,struct htx_data *stats,char *connect_str);
static unsigned char getCkSum(char * wbuf);
static struct id_t GetRdrID(struct sockaddr_in ServerID, struct id_t SemServID, u_long WriterIdx, struct id_t WriterID, struct htx_data * sstats);

static void Req_StopReader(struct sockaddr_in RemServersock,struct id_t SemServID, struct id_t ThisServerID, struct id_t ReaderID, struct htx_data * sstats);

static void GetPktSizes(SOCKET msgsock, struct bufsize_t Bsize, struct htx_data * stats); 

static void ExitCleanup(void);
static void bad_bytes_wrote(struct cum_rw * Lstats, struct htx_data * pstats,
                                              struct htx_data * stats, int bytes, int BadCnt);
static void WriteLocalStats(struct cum_rw LSTATS[], int NoStanzas, char ConnectStr[], struct htx_data * stats, char * msg_text);
static void WriteExit(struct htx_data * stats, struct cum_rw * LSTATS, int NoStanzas, int exitNo,
                        char msg[], char ConnectStr[]);
static void good_bytes_wrote(struct cum_rw * Lstats, struct htx_data * pstats,
                                          struct htx_data * stats, int bytes, char ConnectStr[]);
static int GetUniqueWIdx(struct htx_data * stats);

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
                            shutdown(WriterTestSock, 2); \
                            shm_pHXECOM->SigTermFlag=1; \
                            ShutdownTest(arg->RemoteServerSock, stats); \
                            sprintf(msg_text, "Shutting down testing due to error flag - %hx\n%s\n", ShutdownFlag,  ConnectStr); \
                            RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_SHUTDOWN,0), HTX_HE_SOFT_ERROR, msg_text); \
                            hxfupdate(UPDATE, stats); \
                            msg_text[0] = '\0'; \
                            WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_SHUTDOWN,0), msg_text, ConnectStr); \
                        } \
                    }
#else
#define SHUTDOWNTEST(ShutdownFlag)  \
                    { \
                        if(ShutdownMask & ShutdownFlag || ShutdownFlag & SH_FORCE) { \
                            if(ShutdownMask & SH_CRASH_ON_ANY_ERROR) {   \
                                do_trap_htx64((int) MISC, ShutdownFlag, stats,rule); \
                            }   \
                            shutdown(WriterTestSock, 2); \
                            shm_pHXECOM->SigTermFlag=1; \
                            ShutdownTest(arg->RemoteServerSock, stats); \
                            sprintf(msg_text, "Shutting down testing due to error flag - %hx\n%s\n", ShutdownFlag,  ConnectStr); \
                            RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_SHUTDOWN,0), HTX_HE_SOFT_ERROR, msg_text); \
                            hxfupdate(UPDATE, stats); \
                            msg_text[0] = '\0'; \
                            WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_SHUTDOWN,0), msg_text, ConnectStr); \
                        } \
                    }
#endif


#define MISC 0xBEEFDEAD
#pragma mc_func trap { "7c810808" }
#pragma reg_killed_by trap


void * comwrite(void * Targ)
{
    int    i, rc, flag;
    struct rule_format rule[MAX_STANZAS];
    SOCKET  WriterTestSock;
    int    stanza;
    struct id_t        ReaderID;
    struct id_t        WriterID;
    struct linger      linger;
    int    pak=0;
    int    pattern_max;
    int    pakLen,this_pakLen;
	int    bytes_written=0;
	pid_t  pid;
	int	   write_try=1;
    int    sleep_time;
    unsigned char timestamp = '\0';
    int    BadCnt = 0;
    char * wbuf, * patternfile, * PATTERN, * pattern_trailer, * debug_ptr, * this_write_buf;
    struct cum_rw * LSTATS;
    struct cum_rw * lstats;
    struct htx_data  PseudoStats;
    struct htx_data * pstats;
    struct htx_data * stats;
    struct rw_t   RWPARMS;
    struct rw_t * rwParms = &RWPARMS;
    int    NoStanzas;
    u_short   * bsize;
	int    (*writefn)(SOCKET, char *, int, struct rw_t *, int *,struct htx_data *,char *);
    int    ConsecErrCnt = 0;
    u_short ShutdownMask = 0;
    char   PseudoDeviceStr[32];
    int    WriterIdx;
    int    SigAlarmFlag =  0;
    char   ConnectStr[CONNECTSTR_LEN];
    char   MSGstr[1000];
    char * msg_text;
    int    TerminatingFlag = 0;
    struct comwrite_argt * arg;
    fd_set wd;
    int    nfd;
    int    save_errno;
    char tmpaddr[32];
    char tmpaddr1[32];
	int thetime;
	int elasped_time;
	int twelve_hours=43200;
	int pingrc=0;
	int o;
	int num_pktsizes_req, bcnt, bcnt_max;
    int num_oper;



    arg         = (struct comwrite_argt *) Targ;
    stats       = &arg->stats;
    msg_text    = MSGstr;
    PseudoStats = arg->stats;
    pstats      = &PseudoStats;

/********************************************************************/
/* Attach to comsem's shared memory.                                */
/********************************************************************/
    if((int)(shm_COMSEM = (struct shm_comsem_t *)shmat(shm_pHXECOM->mID_COMSEM, (char *)0, 0)) ==  -1) {
        sprintf(msg_text, "comwrite: Error attaching shared memory - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_SHMAT2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_SHMAT2);
    }
    atexit(ExitCleanup);

/********************************************************************/
/* Setup temparary ConnectStr just in case we are terminated.       */
/********************************************************************/
    sprintf(ConnectStr, "Writer @ %s not initialized.\n", arg->TestNetName);


/********************************************************************/
/* Need unique index number to access own WRITE_AHEAD semaphore.    */
/********************************************************************/
    WriterIdx = GetUniqueWIdx(stats);

/********************************************************************/
/* Obtain rules file stanzas as array of struct rule_format         */
/********************************************************************/
    NoStanzas = GetRules(arg->RemoteServerSock, rule, MAX_STANZAS, stats);

/********************************************************************/
/* Find largest pattern needed.                                     */
/********************************************************************/
    pattern_max = 0;
    for(i=0; i<NoStanzas; i++)
        if(rule[i].bufmax > pattern_max)
            pattern_max = rule[i].bufmax;

/********************************************************************/
/* Allocate space for pattern file.                                 */
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
    if((wbuf = (char *)malloc((pattern_max + 2 * (TP_HDR_LEN)) * sizeof(char))) == NULL) {
        sprintf(msg_text, "comwrite: Malloc of write buffer space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC9,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC9);
    }
    patternfile = wbuf + TP_HDR_LEN;
	memset(wbuf, 0, (pattern_max + 2 * (TP_HDR_LEN)) * sizeof(char));

/********************************************************************/
/* Allocate space for copy of pattern file.                         */
/********************************************************************/
    if((PATTERN = (char *)malloc(pattern_max * sizeof(char))) == NULL) {
        sprintf(msg_text, "comwrite: Malloc of pattern file space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC10,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC10);
    }
	memset(PATTERN, 0, (pattern_max * sizeof(char))); 

/********************************************************************/
/* Allocate space for local stats and zero.                         */
/********************************************************************/
    if((LSTATS = (struct cum_rw *)malloc(sizeof(struct cum_rw) * NoStanzas)) == NULL) {
        sprintf(msg_text, "comwrite: Malloc of local stats space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC11,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC11);
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
    rc = GetPattern(arg->RemoteServerSock, PATTERN, pattern_max, stats);

/********************************************************************/
/* Open test socket for writer.                                     */
/********************************************************************/

#ifdef __DEBUG__
	printf("comwrite.c:: calling openport with arrg:%s and size:%d\n",
					arg->TestNetName , strlen(arg->TestNetName));
#endif
    OpenPort(arg->TestNetName, &WriterID, &WriterTestSock, rule[0].layer, stats);


/********************************************************************/
/* Request connection information for a reader.                     */
/* Send ID info relating to SemServer.                              */
/* Send WriterIdx, identifying WRITE_AHEAD semaphore index.         */
/* Send TestNetName and TestPort for this writer.                   */
/* Read ReaderID identifying connection info to write test partner. */
/********************************************************************/
#ifdef __DEBUG__
                printf("%d : getting info about reader\n",getpid());
#endif
    ReaderID = GetRdrID(arg->RemoteServerSock, shm_pHXECOM->SemServID, WriterIdx, WriterID, stats);

pid = getpid();  /* need this to add to pattern for data identification */

#ifndef __RELGR2600__
#ifdef __HTX_LINUX__
 if(shm_HXECOM->OneSysFlag == 1)
  {
#ifdef __DEBUG__
		printf("the ipaddress connecting for writing:%s,0x%x\n",
			inet_ntoa(ReaderID.sock.sin_addr), ReaderID.sock.sin_addr.s_addr);
#endif
		if ( ((ReaderID.sock.sin_addr.s_addr) & (0x0000000ful)) == 0x01 ) {
			ReaderID.sock.sin_addr.s_addr &= (0xfffffff0ul);
			ReaderID.sock.sin_addr.s_addr |= (0x00000002ul);
	} else if ( (ReaderID.sock.sin_addr.s_addr & (0x0000000ful)) == 0x02 ) {
			ReaderID.sock.sin_addr.s_addr &= (0xfffffff0ul);
			ReaderID.sock.sin_addr.s_addr |= (0x00000001ul);
	} else {
			printf("wrong ip address obtained in last nibble:%x\n",
						((ReaderID.sock.sin_addr.s_addr) & (0x0000000ful))	);
			return -1 ;
	}
	if ( (ReaderID.sock.sin_addr.s_addr & (0x00000f00ul)) == 0x0100 ) {
			ReaderID.sock.sin_addr.s_addr &= (0xfffff0fful);
			ReaderID.sock.sin_addr.s_addr |= (0x00000200ul);
	} else if ( (ReaderID.sock.sin_addr.s_addr & (0x00000f00ul)) == 0x0200 ) {
			ReaderID.sock.sin_addr.s_addr &= (0xfffff0fful);
			ReaderID.sock.sin_addr.s_addr |= (0x00000100ul);
	} else {
			printf("wrong ip address obtained in last but 2 nibble:%x\n",
						(ReaderID.sock.sin_addr.s_addr & (0x00000f00ul))	);
			return -1 ;
	}

	/*ReaderID.sock.sin_addr.s_addr++;*/
#ifdef __DEBUG__
		printf("the ipaddress connecting for writing:%s,0x%x\n",
			inet_ntoa(ReaderID.sock.sin_addr), ReaderID.sock.sin_addr.s_addr);
#endif
} /* End if OneSysFlag*/
#endif
#endif /* __RELGR2600__*/

/********************************************************************/
/* Obtain string used for ID when writing messages.                 */
/********************************************************************/

    GetConnectStr(ConnectStr, CONNECTSTR_LEN, "W", WriterID.sock, " connected to R", ReaderID.sock, stats);


/* examine the ConnectStr and make sure reader and writer are not the same */
/* address */
if( strcmp (InetNtoa(WriterID.sock.sin_addr, tmpaddr, stats),
	InetNtoa(ReaderID.sock.sin_addr, tmpaddr1, stats))==0) {
		HE_exit(0);
}

/********************************************************************/
/*  Setup writer test socket as nonblocking.                        */
/********************************************************************/
#ifdef NO_BLOCK
    i = 1;
    if(ioctlsocket(WriterTestSock, FIONBIO, (unsigned long *) &i)) {
        sprintf(msg_text, "comwrite:  FIONBIO (Setting nonblocking) error - %s.\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_IOCTL1,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_IOCTL1);
    }
#endif

    if((int)rule[0].layer == UDP_LAYER) {
        writefn = DWrite;
    }
    else if((int)rule[0].layer == TCP_LAYER) {
        /************************************************************/
        /* If set TCP_NODELAY,  Don't coalesce packets. Best way to */
        /* test device driver, HW boundary conditions.              */
        /*                                                          */
        /* If set TCP_NODELAY off, best way to check performance.   */
        /* For large mtu sizes, on results in small fragments on    */
        /* net.                                                     */
        /************************************************************/
        flag = rule[0].transact & TCP_NODELAY_VAL;
        if(setsockopt(WriterTestSock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(flag))) {
            sprintf(msg_text, "comwrite: TCP_NODELAY error - %s.\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_SETB2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
            HE_exit(EX_SETB2);
        }
        /************************************************************/
        /* If set SO_LINGER off --                                  */
        /* Don't delivery data after close.  If we are having a     */
        /* problem in getting acks delivered across connection,     */
        /* this setting will allow socket to close.  This shouldn't */
        /* hide a HW problem since if acks aren't getting           */
        /* delivered, we can't test.                                */
        /*                                                          */
        /* If leaving SO_LINGER on --                               */
        /* Test ability of socket to close with data.  May have to  */
        /* force closed by using shutdown or ifconfig detach        */
        /************************************************************/
        if((int)(rule[0].transact & SO_LINGER_VAL) == 0) {
            linger.l_onoff = 1;
            linger.l_linger = 0;
            if(setsockopt(WriterTestSock, SOL_SOCKET, SO_LINGER, (char *) &linger, sizeof(linger))) {
                sprintf(msg_text, "comwrite: SO_LINGER error - %s.\n", STRERROR(errno));
                hxfmsg(stats, HTXERROR(EX_SETB3,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
                HE_exit(EX_SETB3);
            }
        }
        writefn = SWrite;
    }
    else {
        sprintf(msg_text, "comwrite: Error specifying test layer - %d\n", (int)rule[0].layer);
        hxfmsg(stats, HTXERROR(EX_LAYER2,0), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_LAYER2);
    }

/********************************************************************/
/* Wait for reset to start.  Don't attempt to communicate with      */
/* reader process until it indicates that it is ready (i.e. reset). */
/********************************************************************/
#ifdef __DEBUG__
	sprintf(msg_text, "comwrite:  Waiting For Reset, pid = %d, SemId = %d, WriteIDx = %d \n",getpid(), SemID, WriterIdx); 
	hxfmsg(stats, HTXERROR(EX_LAYER2,0), HTX_HE_INFO, msg_text);
#endif
    GateSem(SemID, WriterIdx, stats);  
    if((int)shm_COMSEM->loop[WriterIdx].WriterReset == 1) {
        shm_COMSEM->loop[WriterIdx].WriterReset = 0;
    }
 
/********************************************************************/
/* Connect writer socket to reader socket.                          */
/********************************************************************/
    rc = connect(WriterTestSock, (struct sockaddr *) &ReaderID.sock, sizeof(ReaderID.sock));
    if((rc == SOCKET_ERROR) &&
#ifdef NO_BLOCK
                            (errno != EWOULDBLOCK) &&
#endif
                            (errno != EINPROGRESS) ) {
		save_errno = errno;
		sprintf(msg_text, "pingcheck %s", inet_ntoa(ReaderID.sock.sin_addr));
		pingrc = system(msg_text);
		if(pingrc) {
			/* If network will ping report soft error */
			sprintf(msg_text, "%s\nError connecting to the reader socket - %s\n", ConnectStr, STRERROR(save_errno));
			hxfmsg(stats, HTXERROR(EX_WRITE1,save_errno), HTX_HE_SOFT_ERROR, msg_text);
		} else {
			/* If network will not ping  stop the test, port is dead */
			sprintf(msg_text, "%s\nTest network will not ping! Caused Error connecting to reader socket - %s\n", ConnectStr, STRERROR(save_errno));
			hxfmsg(stats, HTXERROR(EX_WRITE1,save_errno), HTX_HE_HARD_ERROR, msg_text);
			SHUTDOWNTEST(SH_FORCE);
		}
    }

    rwParms->timeout.tv_sec  = rule[0].alarm;
    rwParms->timeout.tv_usec = 0;
#ifdef NO_BLOCK
    errno = 0;
    nfd = WriterTestSock + 1;
    FD_ZERO(&wd);
    FD_SET(WriterTestSock, &wd);
    if((NFDS(rc = select(nfd, NULL, &wd, NULL, &rwParms->timeout))) != 1) {
        sprintf(msg_text, "%s\nSelect: Error connecting to reader socket %d - %s\n", ConnectStr, rc, STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_SELECT2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_SELECT2);
    }
#endif

/********************************************************************/
/* Allocate buffer for pkt sizes                                    */
/********************************************************************/
    if((bsize = (u_short *)malloc(sizeof(u_short) * USHRT_MAX)) == NULL) {
        sprintf(msg_text, "comwrite: Malloc of random buffer space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC9,ERRNO), HTX_HE_HARD_ERROR, msg_text);
        HE_exit(EX_MALLOC9);
    }
/********************************************************************/
/* Send message informing of the start of testing.                  */
/********************************************************************/
#ifdef NO_BLOCK
    sprintf(msg_text, "%s\nBeginning test...NON BLOCKING WRITES: BrokenPipe Count=%d, pid=%d", ConnectStr,shm_pHXECOM->BrokenPipes,pid);
#else
    sprintf(msg_text, "%s\nBeginning test...BLOCKING WRITES: BrokenPipe Count=%d,OneSysFlag=%d,pid=%d", ConnectStr,shm_pHXECOM->BrokenPipes,shm_HXECOM->OneSysFlag,pid);
#endif
    hxfmsg(stats, HTXERROR(EX_WRITE2,0), HTX_HE_INFO, msg_text);

    pak = -1;

    rwParms->SigAlarmFlag = 0;
    rwParms->stats  = stats;

    while(1) {
        for(stanza=0; stanza < NoStanzas; stanza++) {
            sleep_time = rule[stanza].write_sleep * 1000;
            lstats     = &LSTATS[stanza];
            rwParms->timeout.tv_sec  = rule[stanza].alarm;
            rwParms->timeout.tv_usec = 0;
            ShutdownMask = rule[stanza].shutdown;
			num_pktsizes_req = rule[stanza].num_oper;
			shm_COMSEM->loop[WriterIdx].halt = 0;

			if(rule[stanza].bufinc == 0) { /* Fixed Packet size */ 
				bcnt_max = 0; 
			} else { 
				if(rule[stanza].bufinc > 0) { /* Linear Increment */
            		bcnt_max = ((rule[stanza].bufmax - rule[stanza].bufmin ) / rule[stanza].bufinc);
				} else { 
					bcnt_max = 0;  /* Random */ 
				}
			}
            if(bcnt_max == 0 )
                bcnt_max = 1 ;

            stats->test_id = stanza + 1;
            hxfupdate(UPDATE, stats);

			for(bcnt = 0; bcnt < bcnt_max; bcnt++) { 
			
				if(rule[stanza].bufinc == RANDOM) {
					SetSem(SemID, WriterIdx, 0, stats);
					shm_COMSEM->loop[WriterIdx].halt = 1; 
					GateSem(SemID, WriterIdx, stats);	
					shm_COMSEM->loop[WriterIdx].halt = 0; 
				#ifdef __DEBUG__
					sprintf(msg_text, "WriterIdx: %d,  Got Random pkt sizes,bcnt = %d \n",WriterIdx, bcnt); 
					hxfmsg(stats, HTXERROR(EX_WRITE2,0), HTX_HE_INFO, msg_text);
				#endif 
					if(shm_COMSEM->loop[WriterIdx].num_pktsizes != num_pktsizes_req)
						num_pktsizes_req = shm_COMSEM->loop[WriterIdx].num_pktsizes; 
						/*memcpy(bsize, shm_COMSEM->loop[WriterIdx].rand_pktsize, (sizeof(u_short) * num_pktsizes_req)); */ 
						for( i = 0; i < num_pktsizes_req; i++) {
							bsize[i] = ntohs(shm_COMSEM->loop[WriterIdx].rand_pktsize[i]);
						}
				} else { 
					num_pktsizes_req = rule[stanza].num_oper ; 
					for(i = 0; i < rule[stanza].num_oper; i ++) {
                        bsize[i] = rule[stanza].bufmin + bcnt * rule[stanza].bufinc ;
                    }
                }

                num_oper = rule[stanza].num_oper;
                if(num_pktsizes_req != rule[stanza].num_oper)
                    num_oper = num_pktsizes_req;

			    
                for(i=0; i < (int)num_oper; i++) {

                	memcpy(patternfile, PATTERN, bsize[i]);
                	pakLen = lstats->rw_size = bsize[i] + 2 * (TP_HDR_LEN);
                	pattern_trailer = wbuf + bsize[i] + TP_HDR_LEN;
                    pak = (pak+1)%(TP_MAX_SEQNOS);
				#ifdef __DEBUG__
					sprintf(msg_text, "WiterIdx=%d, WriterPID = %d,bcnt = %d, oper = %d, bsize=%d \n", WriterIdx, getpid(), bcnt, i, bsize[i]); 
					hxfmsg(stats, HTXERROR(EX_WRITE2,0), HTX_HE_INFO, msg_text);  
   				#endif
					/* Moved this sem call back from once per stanza */ 
					/* Aix kernel will bog down with sem calls */
					/* and hurt performace, but you won't get miscompares */
					/* moved this back, the code gets out of sync when this */
					/* is only called once per stanza and invalid miscompares */
					/* can occur. */
					if((int)shm_COMSEM->loop[WriterIdx].CheckReset == 1 ) {
						/* note this only happens on reset from reader so */
						/* this sleep will not deter performance during */
						/* normal processing , but the sleep is needed */
						/* to give the ResetWrite routine in comsem time */
						/* to get the sem set to 0, to stop the write here*/
						/* during a reset */
					#ifdef __DEBUG__	
						sprintf(msg_text, "%s\nCalling GateSem in writer, write_try=%d\n,pak=%d", ConnectStr,write_try,pak);
						RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(1003,ERRNO), HTX_HE_INFO, msg_text);
					#endif 
						usleep(5000000); 
						GateSem(SemID, WriterIdx, stats);
						shm_COMSEM->loop[WriterIdx].CheckReset = 0;
					}

					HDR_SEQNO(wbuf, pattern_trailer, pak);
					HDR_TS(wbuf, pattern_trailer, timestamp);
					HDR_CKSUM(wbuf, pattern_trailer);

					if(rule[stanza].debug_pattern && pakLen >= 512 && write_try == 1) {
						for(o=128; o < pakLen-128; o+=128) {
							debug_ptr = wbuf + o;
                            INSERT_PAK_SEQNO(debug_ptr, pak);
                            debug_ptr = wbuf + o + TP_SEQ_LEN;
							INSERT_BLK_SEQNO(debug_ptr, o);
					
							/* to help with retry debug, put the retry count */
							/* in every 128+16th byte */
							debug_ptr = wbuf + o + 16;
							INSERT_RETRY_CNT(debug_ptr, write_try);
							debug_ptr[1]=0x00;
							debug_ptr += 2;
							/* put in the dev name of writer */
							strcpy(debug_ptr,stats->sdev_id);
							debug_ptr += strlen(stats->sdev_id);
							/* need this to make sure there is no real */
							/* data between dev and pid */
							debug_ptr[0]=0x00;
							debug_ptr[1]=0x00;
							debug_ptr[2]=0x00;
							debug_ptr[3]=0x00;
							debug_ptr = wbuf + o + 16 + 2 + strlen(stats->sdev_id) + 4 ;
							INSERT_PID(debug_ptr,pid);
							/* reader must get the dev and pid from first */  
 							/* packet and then overlay the pattern with it */
						}
					}
					if(write_try > 1 && bytes_written > 0) {
						if(write_try == 0xFF) {
                            sprintf(msg_text, "%s\nExcessive write retries: Retry count is %d\n", ConnectStr, write_try);
                            RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(1003,ERRNO), HTX_HE_INFO, msg_text);
                            WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_WRITE13,ERRNO), msg_text, ConnectStr);
						}
						this_write_buf = wbuf + bytes_written;
						this_pakLen = pakLen - bytes_written;
                        sprintf(msg_text, "%s\nPartial write of %d characters, trying to complete the write of %d characters\n", ConnectStr, bytes_written, this_pakLen);
                        RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(1002,ERRNO), HTX_HE_INFO, msg_text);
						/* now check to see if write_try is wrapping and set
						   it back to 0 */
					} else {
						this_write_buf = wbuf;
						this_pakLen = pakLen;
					}
                    rc = -2;
                    if((int)shm_COMSEM->loop[WriterIdx].WriterReset == 0
                       && (shm_pHXECOM->SigTermFlag == 0 && shm_COMSEM->loop[WriterIdx].TERM == 0)
                       && shm_pHXECOM->SigUsr1Flag == 0
                       && (rc = (*writefn)(WriterTestSock, (char *) this_write_buf, this_pakLen, rwParms, &bytes_written,stats,ConnectStr)) == this_pakLen) {
                        ConsecErrCnt = 0;
                        good_bytes_wrote(lstats, pstats, stats, rc, ConnectStr);

						/* if the write code gets to here a full write was done
						   so reset the write_try flag and bytes_written */
						write_try=1;
						bytes_written =0;

                        if(sleep_time)
                            usleep(sleep_time);
                    }
                    else { 
                        if(shm_COMSEM->loop[WriterIdx].TERM || shm_pHXECOM->SigTermFlag || shm_pHXECOM->SigUsr1Flag == 1){
                            hxfupdate(UPDATE, stats);
                            sprintf(msg_text, "Fast socket shutdown due to reader request.\n");
                            shutdown(WriterTestSock, 2);
                            WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_WRITE21,0), msg_text, ConnectStr);
                        }
                        if((int)shm_COMSEM->loop[WriterIdx].WriterReset == 1) {
                            shm_COMSEM->loop[WriterIdx].WriterReset = 0;
                            /************************************************/
                            /* Generate print message for debug.            */
                            /************************************************/
                            sprintf(msg_text,
                                "%s\nResetting Writer to - \ni=%4d, timestamp=%4d, bsize=%6d, stanza=%3d, pak=%4d.\n",
                                    ConnectStr, (int) shm_COMSEM->loop[WriterIdx].WriterIloop, shm_COMSEM->loop[WriterIdx].timeS,
                                     (int) shm_COMSEM->loop[WriterIdx].WriterBsize, (int) shm_COMSEM->loop[WriterIdx].WriterStanza,
                                         (int) shm_COMSEM->loop[WriterIdx].WriterPak);
                            sprintf(msg_text + strlen(msg_text),
                                "Parameters for next write prior to reset - \ni=%4d, timestamp=%4d, bsize=%6d, stanza=%3d, pak=%4d.\n",
                                    i, timestamp, bsize[i], stanza, pak);
                            /************************************************/
                            /* Set timestamp.                               */
                            /************************************************/
                            timestamp = shm_COMSEM->loop[WriterIdx].timeS;

                            /************************************************/
                            /* Reset stanza and all its loop parameters.    */
                            /************************************************/
                            stanza = (int) shm_COMSEM->loop[WriterIdx].WriterStanza;
                            sleep_time = rule[stanza].write_sleep * 1000;
                            lstats = &LSTATS[stanza];
                            rwParms->timeout.tv_sec  = rule[stanza].alarm;

                            /************************************************/
                            /* Reset bsize and all its loop parameters.     */
                            /************************************************/
                            bsize[i] = (int) shm_COMSEM->loop[WriterIdx].WriterBsize;
                            memcpy(patternfile, PATTERN, bsize[i]);
                            pakLen = lstats->rw_size = bsize[i] + 2 * (TP_HDR_LEN);
                            pattern_trailer = wbuf + bsize[i] + TP_HDR_LEN;

                            /************************************************/
                            /* Reset i and pak.                             */
                            /************************************************/
                            i = (int) shm_COMSEM->loop[WriterIdx].WriterIloop;
                            pak = (int) shm_COMSEM->loop[WriterIdx].WriterPak;
							/* this is a reset from the reader, and reader  */
							/* will have dumped all its data so clear */
							/* bytes_written and write_try to send full */
							/* packet requested by the reader */

                            RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE10,0), HTX_HE_INFO, msg_text);
                            bad_other(lstats, pstats, stats);

                            /************************************************/
                            /* Don't proceed until count variable reset.    */
                            /************************************************/
                            GateSem(SemID, WriterIdx, stats);
                            /********************************************/
                            /* Reset i (loop counter) and try again.    */
                            /********************************************/
                            i--;
                            pak--;
							bytes_written =0;
							write_try = 1;
                            continue;
                        }
                        if(rc == 0) {  
							if( ERRNO == EPIPE) {
                            	sprintf(msg_text, "%s\nSocket returned SIGPIPE signal - Exiting.\n", ConnectStr);
                            	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE13,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
                            	bad_other(lstats, pstats, stats);
                            	SHUTDOWNTEST(SH_PIPE);
                            	/********************************************/
                            	/* Nothing to write. QUIT.                  */
                            	/********************************************/
                            	hxfupdate(UPDATE, stats);
                            	msg_text[0] = '\0';
                            	WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_WRITE13,ERRNO), msg_text, ConnectStr);
                        	}
                        	if((int)rule[0].layer == (int)TCP_LAYER) {
                            	sprintf(msg_text, "%s\nSocket is closed - Exiting.\n", ConnectStr);
                            	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE14,0), HTX_HE_SOFT_ERROR, msg_text);
                            	bad_other(lstats, pstats, stats);
                            	/********************************************/
                            	/* Nothing to write. QUIT.                  */
                            	/********************************************/
                            	hxfupdate(UPDATE, stats);
                            	msg_text[0] = '\0';
                            	WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_WRITE14,0), msg_text, ConnectStr);
                        	}
						} else { 
                        	if(rc== -1) {
                            	if(rwParms->SigAlarmFlag || ERRNO == ENOBUFS)
                                	ConsecErrCnt++;
                            	else
                                	ConsecErrCnt = 0;
                            	if(ConsecErrCnt == 3) {
                                	sprintf(msg_text, "%s\nWrite failed - %s.\n", ConnectStr, STRERROR(errno));
                                	sprintf(msg_text + strlen(msg_text), "Unable to write - Exiting.\n");
                                	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE15,0), HTX_HE_HARD_ERROR, msg_text);
                                	bad_other(lstats, pstats, stats);
                                	/********************************************/
                                	/* Nothing to write. QUIT.                  */
                                	/********************************************/
                                	hxfupdate(UPDATE, stats);
                                	msg_text[0] = '\0';
                                	WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_WRITE15,ERRNO), msg_text, ConnectStr);
                            	}
                            	if(rwParms->SigAlarmFlag) {
                                	rwParms->SigAlarmFlag = 0;
                                	sprintf(msg_text, "%s\nWrite failed - %s.\nPacket %d\n", ConnectStr, STRERROR(errno),pak);
                                	sprintf(msg_text + strlen(msg_text), "IO_ALARM_TIME = %d seconds was exceeded. " 
												" Resend %d chars, since %d bytes were already written to socket.",
                                                                                           (int) rule[stanza].alarm,(this_pakLen-bytes_written),bytes_written);
                                	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE16,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
                                	bad_other(lstats, pstats, stats);
                                	/********************************************/
                                	/* Reset i (loop counter),                  */
                                	/*          and try again.  Since we timed  */
                                	/* out, we will shortly be reset by the     */
                                	/* reader process.                          */
                                	/********************************************/
									/* don't allow retry on write timeout */
									SHUTDOWNTEST(SH_FORCE);
                            	}
                            	if(ERRNO == EPIPE) {
                                	/* give port time to recover if EEH */
                                	sleep(10);
									/* If the network still pings restart reader */
									/* and writer, else stop */
									save_errno = errno;
									sprintf(msg_text, "pingcheck %s", inet_ntoa(ReaderID.sock.sin_addr));
									pingrc = system(msg_text);
									if(pingrc) {
                                    	/* This allows 25 broken pipes in 12 hour*/
                                    	/* period, anymore and I stop */
                                    	/* note shm_pHXECOM is for all readers and
                                       	writes on any one port pair so if reps
                                       	is 5 , it goes up by 5 if all 5 break */
 
										if(++shm_pHXECOM->BrokenPipes < 25) {

                                        	/* don't ask me why, but don't remove*/
                                        	/* this next debug statement. the */
                                        	/* restart process stops working??? */
                                        	sprintf(msg_text, "%s\nBroken Pipe: Restart writer/reader BrokenPipe count=%d\n", ConnectStr,shm_pHXECOM->BrokenPipes);
                                        	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE22,save_errno), HTX_HE_INFO, msg_text);
											thetime=time(NULL);
											elasped_time=thetime-shm_pHXECOM->starttime;
											if(elasped_time > twelve_hours) {
												shm_pHXECOM->BrokenPipes=0;
												shm_pHXECOM->starttime=thetime;
											}

											Req_StopReader(arg->RemoteServerSock,shm_pHXECOM->SemServID, shm_HXECOM->exer[shm_pHXECOM->ExerNo].ServerID, ReaderID, stats);
											HE_exit(0);
										} else {
											/* Too many breaks shutdown */
											sprintf(msg_text, "%s\nWrite failed - %s.\nToo Many Broken Pipes! Exiting\n", ConnectStr, STRERROR(save_errno));
											RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE22,save_errno), HTX_HE_HARD_ERROR, msg_text);
											SHUTDOWNTEST(SH_FORCE);
										}
									} else {
										sprintf(msg_text, "%s\nWrite failed - %s.\nConnection will no longer ping, its locked! Exiting\n", 
																ConnectStr, STRERROR(save_errno));
										RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE22,save_errno), HTX_HE_HARD_ERROR, msg_text);
										SHUTDOWNTEST(SH_FORCE);
									}
                            	}
                            	if(ERRNO == ENOBUFS) {
                                	sprintf(msg_text, "%s\nWrite failed - %s.\n", ConnectStr, STRERROR(errno));
                                	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE22,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
                                	bad_other(lstats, pstats, stats);
                                	/********************************************/
                                	/* Reset i (loop counter),                  */
                                	/*          and try again.                  */
                                	/********************************************/
                                	i--;
                                	pak--;
									++write_try;
                                	continue;
                            	}
                            	if(ERRNO == WSAEINTR) {
#ifdef NO_BLOCK
                                	sprintf(msg_text, "%s\nWrite failed - %s.\n", ConnectStr, STRERROR(errno));
                                	sprintf(msg_text + strlen(msg_text), "Unknown Interrupt.\n");
                                	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE17,ERRNO), HTX_HE_HARD_ERROR, msg_text);
                                	bad_other(lstats, pstats, stats);
                                	/********************************************/
                                	/* Reset i (loop counter),                  */
                                	/*          and try again.                  */
                                	/********************************************/
                                	i--;
                                	pak--;
									++write_try;
                                	continue;
#else
									/* this was alarm timeout.. stop the run. */
                                	sprintf(msg_text, "%s\nWrite failed - ALARM TIMEOUT ON WRITE. %s seconds\n", ConnectStr,rule[stanza].alarm);
                                	sprintf(msg_text + strlen(msg_text), "Exiting. errno = %d\n",ERRNO);
                                	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE18,ERRNO), HTX_HE_HARD_ERROR, msg_text);
                                	bad_other(lstats, pstats, stats);
                                	SHUTDOWNTEST(SH_FORCE);
                                	hxfupdate(UPDATE, stats);
                                	msg_text[0] = '\0';
                                	WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_WRITE18,ERRNO), msg_text, ConnectStr);
#endif
        	                  } else {
            	                    sprintf(msg_text, "%s\nWrite failed - %s.\n", ConnectStr, STRERROR(errno));
                	                sprintf(msg_text + strlen(msg_text), "Exiting. errno = %d\n",ERRNO);
                    	            RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE18,ERRNO), HTX_HE_HARD_ERROR, msg_text);
                        	        bad_other(lstats, pstats, stats);
                            	    SHUTDOWNTEST(SH_WR_OTHER);
                                	/********************************************/
                                	/* Quit since don't know what happen.       */
                                	/********************************************/
                                	hxfupdate(UPDATE, stats);
                                	msg_text[0] = '\0';
                                	WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_WRITE18,ERRNO), msg_text, ConnectStr);
                            	}
                        	} else {  
                        		if((rc > 0 && rc != this_pakLen)) {
    	                        	sprintf(msg_text, "%s\nWrote %d characters, expected %d\n", ConnectStr, rc, this_pakLen);
        	                    	sprintf(msg_text + strlen(msg_text), "Write failed - %s.\n", STRERROR(errno));
            	                	RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE19,ERRNO), HTX_HE_HARD_ERROR, msg_text);
                	            	bad_bytes_wrote(lstats, pstats, stats, rc, 1);
									SHUTDOWNTEST(SH_PAKLEN);
                        		}
							}	
                        	/****************************************************/
                        	/* If here, error in logic.                         */
                        	/****************************************************/
                        	hxfupdate(UPDATE, stats);
                        	sprintf(msg_text, "Error handler logic error. Writer isexiting!rc = %d, this_paklen = %d \n", rc, this_pakLen);
                        	WriteExit(stats, LSTATS, NoStanzas, HTXERROR(EX_WRITE20,0), msg_text, ConnectStr);
						}
                    } /* end of the writefn if processing for errors */
                } /* end of the for loop writing num oper bzise buffers */
                /********************************************************/
                /* Since we are possibly changing the buffer size,      */
                /* hxfupdate must be called before bsize is changed     */
                /* again.                                               */
                /********************************************************/
                hxfupdate(UPDATE, stats);
				
            }  /* end of loop through bsize. */
        }
		/* This shld be done from reader, not by both */ 
        /* hxfupdate(FINISH, stats); */
	}
    return 0;
}



static void WriteLocalStats(struct cum_rw LSTATS[], int NoStanzas, char ConnectStr[], struct htx_data * stats, char * msg_text)
{
    int   stanza;
    FILE *config_des = NULL;
    char temp_string[300];
	/*eturn;*/


    sprintf(temp_string, "%s/%s", global_htx_log_dir, LSTAT_FILE);
    config_des = fopen(temp_string, "a");

    GlobalWait(FILE_SEM, stats);

    fprintf(config_des, "%s\n", ConnectStr);
    for(stanza=0; stanza<NoStanzas; stanza++) {
        fprintf(config_des, "STANZA %d\n", stanza);
        fprintf(config_des, "good writes        = %18u\n", LSTATS[stanza].totals.good_rw);
        fprintf(config_des, "bad writes         = %18u\n", LSTATS[stanza].totals.bad_rw);
        fprintf(config_des, "good others        = %18u\n", LSTATS[stanza].totals.good_others);
        fprintf(config_des, "bad others         = %18u\n", LSTATS[stanza].totals.bad_others);
        if(LSTATS[stanza].totals.bad_bytes_rw1 > 0)
            fprintf(config_des, "bad bytes written  = %9lu%09lu\n",
                      LSTATS[stanza].totals.bad_bytes_rw1, LSTATS[stanza].totals.bad_bytes_rw2);
        else
            fprintf(config_des, "bad bytes written  = %18lu\n", LSTATS[stanza].totals.bad_bytes_rw2);
        if(LSTATS[stanza].totals.good_bytes_rw1 > 0)
            fprintf(config_des, "good bytes written = %9lu%09lu\n",
                      LSTATS[stanza].totals.good_bytes_rw1, LSTATS[stanza].totals.good_bytes_rw2);
        else
            fprintf(config_des, "good bytes written = %18lu\n", LSTATS[stanza].totals.good_bytes_rw2);
        fprintf(config_des, "\n");
    }

    GlobalSignal(FILE_SEM, stats);

    fclose(config_des);
}



static void RWmsg(struct cum_rw * Lstats, struct rule_format * rule, int Stanza, struct htx_data * sstats, int err, enum sev_code sev, char * msgtxt)
{

    sprintf(msgtxt + strlen(msgtxt),
           "For stanza %d, %s connection,  bufmin=%d, bufmax=%d, bufinc=%d\nConsecutive prior good writes=%9u,  good bytes written=",
              Stanza, (rule->layer == (int) UDP_LAYER) ? "UDP" : "TCP",
               rule->bufmin, rule->bufmax, rule->bufinc, Lstats->current.good_rw);
    if(Lstats->current.good_bytes_rw1 == 0)
        sprintf(msgtxt + strlen(msgtxt),"%18lu\n",  Lstats->current.good_bytes_rw2);
    else
        sprintf(msgtxt + strlen(msgtxt),"%9lu%09lu\n", Lstats->current.good_bytes_rw1, Lstats->current.good_bytes_rw2);

	if(Lstats) {
		Lstats->current.bad_others     = 0;
		Lstats->current.bad_rw         = 0;
		Lstats->current.bad_bytes_rw1  = 0;
		Lstats->current.bad_bytes_rw2  = 0;
		Lstats->current.good_bytes_rw1 = 0;
		Lstats->current.good_bytes_rw2 = 0;
		Lstats->current.good_others    = 0;
		Lstats->current.good_rw        = 0;
	}

    hxfmsg(sstats, err, sev, msgtxt);
}



static void WriteExit(struct htx_data * sstats, struct cum_rw * LSTATS, int NoStanzas, int exitNo,
                        char msg[], char ConnectStr[])
{
    WriteLocalStats(LSTATS, NoStanzas, ConnectStr, sstats, msg + strlen(msg) +1);
    sprintf(msg + strlen(msg), "%s\nStats Updated. Exiting.\n", ConnectStr);
    hxfmsg(sstats, exitNo, HTX_HE_INFO, msg);
    HE_exit(exitNo>>16);
}


static void Req_StopReader(struct sockaddr_in RemServerSock, struct id_t SemServID, struct id_t ThisServerID, struct id_t ReaderID, struct htx_data * stats)
{
	/*********************************************************************/
	/*This code will send CM_STOP_READER to the remote reader, that will*/
	/*Cause the reader to stop and then this routine calls SetUpwriter  */
	/*To kick off another writer which will request another reader        */
	/*********************************************************************/
    int rc;
    SOCKET ToServerSock;
    struct CoordMsg  CMsg;
	char tmpaddr[30];


    ToServerSock = SetUpConnect(&RemServerSock, stats, 0);

    memset(&CMsg, '\0', sizeof(CMsg));
    CMsg.msg_type = htonl(CM_STOP_READER);
    memcpy(&CMsg.ID.server, &SemServID, sizeof(struct id_t));
    HostToNetId_t(&CMsg.ID.server);
    rc = StreamWrite(ToServerSock, (char *) &CMsg, sizeof(CMsg));
    if(rc == -1) {
        sprintf(stats->msg_text, "comwrite: Error getting reader ID - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
        HE_exit(EX_WRITE5);
    }
	HostToNetId_t(&ReaderID);
	rc = StreamWrite(ToServerSock, (char *) &ReaderID, sizeof(struct id_t));
	if(rc == -1) {
		sprintf(stats->msg_text, "comwrite: Error getting reader ID - %s\n", STRERROR(errno));
		hxfmsg(stats, HTXERROR(EX_WRITE7,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
		HE_exit(EX_WRITE7);
	}

	HostToNetId_t(&ThisServerID);
	rc = StreamWrite(ToServerSock, (char *) &ThisServerID, sizeof(struct id_t));
	if(rc == -1) {
		sprintf(stats->msg_text, "comwrite: Error getting reader ID - %s\n", STRERROR(errno));
		hxfmsg(stats, HTXERROR(EX_WRITE7,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
		HE_exit(EX_WRITE7);
	}
    /* The StopReader() code in hxecom will stop the reader then send back
        a CM_READER_STOPPED msg, so read it before closing the socket.
    */
	rc = StreamRead(ToServerSock, (char *) &CMsg, sizeof(CMsg));
	if(rc == -1 || ntohl(CMsg.msg_type) != CM_READER_STOPPED) {
		sprintf(stats->msg_text, "comwrite: Error getting reader ID - %s\n", STRERROR(errno));
		hxfmsg(stats, HTXERROR(EX_WRITE8,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
		HE_exit(EX_WRITE8);
	}

    closesocket(ToServerSock);
    return;
}


static struct id_t GetRdrID(struct sockaddr_in ServerID, struct id_t SemServID,
                                 u_long WriterIdx, struct id_t WriterID, struct htx_data * stats)
{
    struct id_t ReaderID;
    int rc;
    SOCKET ToServerSock;
    struct CoordMsg  CMsg;
	char tmpaddr[30];

    ToServerSock = SetUpConnect(&ServerID, stats, 0);

    memset(&CMsg, '\0', sizeof(CMsg));
    CMsg.msg_type = htonl(CM_REQ_RDR_ID);
    memcpy(&CMsg.ID.server, &SemServID, sizeof(struct id_t));
    HostToNetId_t(&CMsg.ID.server);
    rc = StreamWrite(ToServerSock, (char *) &CMsg, sizeof(CMsg));
    if(rc == -1) {
        sprintf(stats->msg_text, "comwrite: Error getting reader ID - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
        HE_exit(EX_WRITE5);
    }

    WriterIdx = htonl(WriterIdx);
    rc = StreamWrite(ToServerSock, (char *) &WriterIdx, sizeof(WriterIdx));
    if(rc == -1) {
        sprintf(stats->msg_text, "comwrite: Error getting reader ID - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE6,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
        HE_exit(EX_WRITE6);
    }
    HostToNetId_t(&WriterID);
    rc = StreamWrite(ToServerSock, (char *) &WriterID, sizeof(struct id_t));
    if(rc == -1) {
        sprintf(stats->msg_text, "comwrite: Error getting reader ID - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE7,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
        HE_exit(EX_WRITE7);
    }

    rc = StreamRead(ToServerSock, (char *) &CMsg, sizeof(CMsg));
    if(rc == -1 || ntohl(CMsg.msg_type) != CM_READER_ID) {
        sprintf(stats->msg_text, "comwrite: Error getting reader ID - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE8,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
        HE_exit(EX_WRITE8);
    }
    closesocket(ToServerSock);
    memcpy(&ReaderID, &CMsg.ID.server, sizeof(struct id_t));
/********************************************************************/
/* CleanUp received struct sockaddr_in structure.  If a message     */
/* is from a remote host, it may not be AIX.                        */
/********************************************************************/
    FixSockAddr(&ReaderID.sock);
    NetToHostId_t(&ReaderID);

    return ReaderID;
}

static int SWrite(SOCKET fd, register char *ptr, register int nbytes, struct rw_t *rwParms,int *bytes_written,struct htx_data * stats, char *connect_str)
{
    size_t  nleft;
    int nwritten;
    int src;
    fd_set wd;
    struct timeval timeout;
    int nfd;
    /* I need to know if the writer ever sends a -1 on start of packet in */
    /* order to tell the reader to shutdown, its not supposed to.         */
	/* note if writer is ever allowed to retry, this could print here since */
	/* there could be -1 in middlet of packet somewhere.     */
	/*
	int first_int;
    first_int = atoi(ptr);
    if(first_int == -1) {
        sprintf(stats->msg_text, "SWrite: %d\nfirst_int on start of write packet was %s\n", first_int,connect_str);
        hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
    }
	*/
    errno = 0;
    nleft = nbytes;
    while(nleft > 0) {
/*
        sprintf(stats->msg_text, "SWrite: %s\nIn the write loop, alarm set to %d,bytes to send=%d\n",rwParms->timeout.tv_sec,connect_str,nbytes);
        hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
*/
#ifndef NO_BLOCK
		alarm(rwParms->timeout.tv_sec);		/* set alarm timeout */
#endif
        if((nwritten = send(fd, ptr, nleft, 0)) > 0) {
            nleft -= nwritten;
            ptr += nwritten;
			*bytes_written += nwritten;
#ifndef NO_BLOCK
			alarm(0);	/* cancel alarm timeout */
#endif
/*
			if(nleft > 0) {
				sprintf(stats->msg_text, "SWrite1: Retrying send, nleft was %d,returned by send call=%d, total bytes written the SWrite call=%d\n", nleft,nwritten,*bytes_written);
				hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
			}
*/
            /* this will keep trying to write until and error occurs or */
            /* 0 bytes are written within the alarm timeout period */
			/* or until the test is terminated by sigterm or user.   */
			if(rwParms->SigAlarmFlag == 0 && shm_pHXECOM->SigTermFlag ==0) {
				continue;
			} else {
				if((size_t)nbytes == nleft) {
					return -1;
				} else {
					break;
				}
			}
        }
#ifndef NO_BLOCK
        else if(errno == EINTR) {
            /* here nwritten can be either 0 or -1 */
            /* if it has not written anything in 10 minutes, default alarm */
            /* time, then something is broke to report -1 error and let  */
            /* main code body error handling report it and stop writer */
/*
            sprintf(stats->msg_text, "SWriteEINTR: %s\nbytes written= %d,\n",rwParms->timeout.tv_sec,connect_str,nwritten);
            hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
*/
			rwParms->SigAlarmFlag = 1;
			errno = WSAEINTR;
			alarm(0);	/* cancel alarm timeout */
			return -1;
		}
#else
        else if(nwritten == SOCKET_ERROR &&
                            ((errno == EWOULDBLOCK) ||
                             (errno == EINPROGRESS) ) ) {
            nfd = fd +1;
            FD_ZERO(&wd);
            FD_SET(fd, &wd);
            timeout = rwParms->timeout;
            if((NFDS(src = select(nfd, NULL, &wd, NULL, &timeout))) == 1) {
               continue;
            }
            else if(src == 0) {
                errno = WSAEINTR;
                if((size_t)nbytes == nleft) {
                    rwParms->SigAlarmFlag = 1;
				/* trap((int) MISC,(int) rwParms->SigAlarmFlag,
                     (int) ptr,
					(int) &wd);
				*/
                    return -1;
                }
                else {
                    break;
                }
            }
            else if(src == SOCKET_ERROR) {
		if((size_t)nbytes == nleft) {
                    return -1;
                }
                else {
                    break;
                }
            }
            else {
                errno = EIO;
		if((size_t)nbytes == nleft) {
                    return -1;
                }
                else {
                    break;
                }
            }
        }
#endif
        else if(nwritten == SOCKET_ERROR) {
#ifndef NO_BLOCK
			alarm(0);	/* cancel alarm timeout */
#endif
    	    if((size_t)nbytes == nleft) {
/*
                sprintf(stats->msg_text, "SWrite3: %s\nSOCKERR: return -1, nleft=%d,nwritten=%d, total written=%d\n",connect_str,nleft,nwritten,*bytes_written);
                hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
*/
                return -1;
            }
            else {
/*
                sprintf(stats->msg_text, "SWrite4:SOCKERR break %s\nbytes written= %d,\n",rwParms->timeout.tv_sec,connect_str,nwritten);
                hxfmsg(stats, HTXERROR(1110,ERRNO), HTX_HE_INFO, stats->msg_text);
*/
                break;
            }
        }
        else {
#ifndef NO_BLOCK
			alarm(0);	/* cancel alarm timeout */
#endif
            errno = EIO;
    	    if((size_t)nbytes == nleft) {
                return -1;
            }
            else {
                break;
            }
        }
    }
#ifndef NO_BLOCK
	alarm(0);	/* cancel alarm timeout */
#endif
    return (nbytes - nleft);
}

static int DWrite(SOCKET fd, register char *ptr, register int nbytes, struct rw_t *rwParms,int *bytes_written,struct htx_data *stats,char *connect_str)
{
    size_t  nleft;
    int nwritten;
    int src;
    fd_set wd;
    struct timeval timeout;
    int nfd;
    int first_int;

    errno = 0;
	nleft = nbytes;

    while(1) {
#ifndef NO_BLOCK
		alarm(rwParms->timeout.tv_sec);		/* set alarm timeout */
#endif
        if((nwritten = send(fd, ptr, nleft, 0)) > 0) {
            nleft -= nwritten;
            ptr += nwritten;
            *bytes_written += nwritten;
#ifndef NO_BLOCK
            alarm(0);   /* cancel alarm timeout */
#endif
            if(rwParms->SigAlarmFlag == 0 && shm_pHXECOM->SigTermFlag ==0) {
                continue;
            } else {
                if((size_t)nbytes == nleft) {
                    return -1;
                } else {
                    break;
                }
            }
        }
#ifndef NO_BLOCK
        else if(errno == EINTR) {
            rwParms->SigAlarmFlag = 1;
            errno = WSAEINTR;
            alarm(0);   /* cancel alarm timeout */
            return -1;
        }
#else
        else if(nwritten == SOCKET_ERROR &&
                            ((errno == EWOULDBLOCK) ||
                             (errno == EINPROGRESS) ) ) {
            nfd = fd +1;
            FD_ZERO(&wd);
            FD_SET(fd, &wd);
            timeout = rwParms->timeout;
            if((NFDS(src = select(nfd, NULL, &wd, NULL, &timeout))) == 1) {
               continue;
            }
            else if(src == 0) {
                errno = WSAEINTR;
                if((size_t)nbytes == nleft) {
                    rwParms->SigAlarmFlag = 1;
                    return -1;
                }
                else {
                    break;
                }
            }
            else if(src == SOCKET_ERROR) {
        		if((size_t)nbytes == nleft) {
                    return -1;
                }
                else {
                    break;
                }
            }
            else {
                errno = EIO;
        		if((size_t)nbytes == nleft) {
                    return -1;
                }
                else {
                    break;
                }
            }
        }
#endif
        else if(nwritten == SOCKET_ERROR) {
#ifndef NO_BLOCK
            alarm(0);   /* cancel alarm timeout */
#endif
            if((size_t)nbytes == nleft) {
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
            errno = EIO;
            if((size_t)nbytes == nleft) {
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
    return (nbytes - nleft);
}




static unsigned char getCkSum(char * wbuf)
{
    int i;
    int sum =0;

    for(i=0; i<TP_HDR_LEN-1; i++)
        sum +=(unsigned char)wbuf[i];
    return (0xff & sum);
}



static void bad_bytes_wrote(struct cum_rw * Lstats, struct htx_data * pstats,
                                              struct htx_data * sstats, int bytes, int BadCnt)
{
    if(BadCnt == 1) {
        sstats->bad_writes            += 1;
/*        pstats->bad_writes           += 1; */
        Lstats->totals.bad_rw        += 1;
        Lstats->current.bad_rw       += 1;
        sstats->bytes_writ            += bytes;
 /*       pstats->bytes_writ           += bytes; */
        addw(Lstats->totals.bad_bytes_rw1, Lstats->totals.bad_bytes_rw2, bytes);
        addw(Lstats->current.bad_bytes_rw1, Lstats->current.bad_bytes_rw2, bytes);
    }
    else {
        Lstats->totals.bad_others  += 1;
        Lstats->current.bad_others += 1;
        sstats->bad_others          += 1;
/*        pstats->bad_others         += 1; */
    }
}



static void good_bytes_wrote(struct cum_rw * Lstats, struct htx_data * pstats,
                                          struct htx_data * sstats, int bytes, char ConnectStr[])
{
    if((unsigned int)bytes != Lstats->rw_size) {
        sprintf(sstats->msg_text, "%s\nLast rw_size=%d, New rw_size=%d, Must be equal.\n",
                           ConnectStr, Lstats->rw_size, bytes);
        hxfmsg(sstats, HTXERROR(EX_WRITE9,0), HTX_HE_SOFT_ERROR, sstats->msg_text);
        HE_exit(EX_WRITE9);
    }
    Lstats->totals.good_rw  += 1;
    Lstats->current.good_rw += 1;
    sstats->good_writes      += 1;
 /*   pstats->good_writes     += 1; */
    addw(Lstats->totals.good_bytes_rw1, Lstats->totals.good_bytes_rw2, bytes);
    addw(Lstats->current.good_bytes_rw1, Lstats->current.good_bytes_rw2, bytes);
    sstats->bytes_writ       += bytes;
/*    pstats->bytes_writ      += bytes; */
}




static void ReadAck(struct semid_t ID, struct htx_data * stats);
static void ResetWrite(struct semid_t ID, struct htx_data * stats);
static void HaltWrite(struct semid_t ID, struct htx_data * stats);
static void ShutdownWrite(struct htx_data * stats);
static void Terminating(struct htx_data * stats);
static void TerminateWrite(struct semid_t ID, struct htx_data * stats);

/********************************************************************/
/* Since there is only one instance of comsem active for each       */
/* hxecom, we can use global structure without locks for            */
/* shm_COMSEM->loop[].halt, timestamp, and last.                    */
/********************************************************************/

void * comsem(void * Targ)
{
    struct CoordMsg  CMsg;
    SOCKET    SemServSock;
    int    i, rc = 0;
    char   MSGstr[1000];
    char * msg_text;
    struct htx_data * stats;
    struct comsem_argt * arg;
	unsigned int seed; 
	SOCKET msgsock; 

    arg         = (struct comsem_argt * )Targ;
    SemServSock = arg->SemServSock;
    stats       = &arg->stats;
    msg_text    = MSGstr;

/********************************************************************/
/*                                                                  */
/********************************************************************/
    for(i=0; i<MAXREMOTE; i++) { 
        shm_COMSEM->loop[i].last = -1;
		memset(shm_COMSEM->loop[i].rand_pktsize, 0, (sizeof(u_short) * USHRT_MAX));
	} 

    atexit(ExitCleanup);
	seed = arg->bufseed; 
	if(seed == 0) { 
		time(&seed);
	} 
	srandom(seed); 

/********************************************************************/
/* Become active server.                                            */
/********************************************************************/
	listen(SemServSock, MAXREMOTE);
    do {
        memset(&CMsg, '\0', sizeof(CMsg));
		 
        if(shm_pHXECOM->SigTermFlag || shm_pHXECOM->SigUsr1Flag)
            Terminating(stats);
		if(!shm_pHXECOM->SigTermFlag)
            msgsock = accept(SemServSock, 0, 0);
        if(msgsock == -1) {
            sprintf(msg_text, "COMSEM: Error accepting message - %s.\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(HEINFO6,0), HTX_HE_SOFT_ERROR, msg_text);
            continue;
        }
        memset(&CMsg, '\0', sizeof(CMsg));
        if((rc = StreamRead(msgsock, (char *) &CMsg, sizeof(CMsg))) == 0) {
            closesocket(msgsock);
            continue;
        }
        else if(rc == -1) {
            sprintf(msg_text, "%s: Error reading message - %s.\n", comsem, STRERROR(errno));
            hxfmsg(stats, HTXERROR(HEINFO7,0), HTX_HE_SOFT_ERROR, msg_text);
            continue;
        }

        CMsg.msg_type = ntohl(CMsg.msg_type);
	#ifdef __DEBUG__
		sprintf(msg_text, "comsem : %d, recvd packet =%d \n",getpid(), CMsg.msg_type); 
		hxfmsg(stats, 0, HTX_HE_INFO, msg_text);
	#endif
        switch(CMsg.msg_type) {
            case CM_READACK:
        		NetToHostSemid_t(&CMsg.ID.SemID);
                ReadAck(CMsg.ID.SemID, stats);
                break;
            case CM_HALT_WRITE:
        		NetToHostSemid_t(&CMsg.ID.SemID);
                HaltWrite(CMsg.ID.SemID, stats);
                break;
            case CM_RESET_WRITE:
        		NetToHostSemid_t(&CMsg.ID.SemID);
                ResetWrite(CMsg.ID.SemID ,stats);
                break;
            case CM_SHUTDOWN:
        		NetToHostSemid_t(&CMsg.ID.SemID);
                ShutdownWrite(stats);
                break;
            case CM_SIGTERM:
    			NetToHostSemid_t(&CMsg.ID.SemID);
                Terminating(stats);
                break;
            case CM_TERM_WRITE:
        		NetToHostSemid_t(&CMsg.ID.SemID);
                TerminateWrite(CMsg.ID.SemID, stats);
                break;
			case GET_PKTSIZES: 
				NetToHostBsize_t(&CMsg.ID.Bsize); 
				GetPktSizes(msgsock, CMsg.ID.Bsize, stats); 
				break ; 
            default:
                sprintf(msg_text, "comsem: Unknown msg_type %d\n", (int)CMsg.msg_type);
                hxfmsg(stats, HTXERROR(EX_COMSEM1,0), HTX_HE_INFO, msg_text);
                break;
        }
		closesocket(msgsock);
	 
    } while(1);
    return 0;
}


static void ResetWrite(struct semid_t ID, struct htx_data * stats)
{
    if(shm_COMSEM->loop[(int)ID.WriteIdx].halt)
        return;

    /* set CheckReset so the writer code will call the GateSem so it can see */
    /* the stop about to be set below by SetSem */
    /* I put this code in because of slow performance on large net configs */
    /* the sempaphore calls were getting  bogged down with 1000's of calls */
    shm_COMSEM->loop[(int)ID.WriteIdx].CheckReset = 1;


    shm_COMSEM->loop[(int)ID.WriteIdx].timestamp = ID.timeS;
    shm_COMSEM->loop[(int)ID.WriteIdx].last = ID.ackNo - 1;
    SetSem(SemID, ID.WriteIdx, 0, stats);

#ifdef __DEBUG__
    sprintf(stats->msg_text,
       "comsem: Received reset for writer %d - i=%d, timestamp=%d, stanza=%d.\nSetting write_ahead=%d, pak=%d, pid = %d, SemID= %d\n",
             (int)ID.WriteIdx, (int)ID.Iloop, (int)ID.timeS, (int)ID.Stanza, (int)ID.WriteAhead, (int)ID.ackNo, getpid(), SemID);
    hxfmsg(stats, HTXERROR(EX_COMSEM2,0), HTX_HE_INFO, stats->msg_text);
#endif 

    shm_COMSEM->loop[(int)ID.WriteIdx].timeS = ID.timeS;
    shm_COMSEM->loop[(int)ID.WriteIdx].WriterStanza = ID.Stanza;
    shm_COMSEM->loop[(int)ID.WriteIdx].WriterIloop  = ID.Iloop;
    shm_COMSEM->loop[(int)ID.WriteIdx].WriterBsize  = ID.bsize;
    shm_COMSEM->loop[(int)ID.WriteIdx].WriterPak    = ID.ackNo;

/********************************************************************/
/* Set WriterReset last, after all fields initialized.              */
/********************************************************************/
   	shm_COMSEM->loop[(int)ID.WriteIdx].WriterReset = 1;
   	SetSem(SemID, ID.WriteIdx, (int)ID.WriteAhead, stats);

}


static void 
GetPktSizes(SOCKET msgsock, struct bufsize_t Bsize, struct htx_data * stats) { 

	/* Reader had requested for new set of random packet sizes, 
	 * 1. Wait here until Writer has set its sem to 0 and blocked on GateSem. 
	 * 2. Generate fresh set of random packet sizes. 
	 * 3. Respond back to Reader with new set. 
	 * 4. Wait for Reader to send back Ack, reset WriterIdx semaphore. 
	 */  
	
	 int i, rc = 0 ; 
	 u_short WriterIdx, num_pktsize_req, bufmin, bufmax, num_oper, temp_buf_size; 
	 struct CoordMsg CMsg; 

	 WriterIdx 		 = Bsize.WriterIdx; 
	 bufmin 		 = Bsize.bufmin; 
	 bufmax 		 = Bsize.bufmax; 
	 num_oper 		 = Bsize.num_oper; 
	 num_pktsize_req = Bsize.num_pktsizes ; 
	 /* 1 */ 
#ifdef __DEBUG__
     sprintf(stats->msg_text, "WriterIdx=%d, GetPktSizes: 1 \n", WriterIdx);
     hxfmsg(stats, HTXERROR(EX_COMSEM2,0), HTX_HE_INFO, stats->msg_text);
#endif 
	while(shm_COMSEM->loop[WriterIdx].halt != 1) { 
		usleep(1); 
	}
	if(shm_COMSEM->loop[WriterIdx].halt != 0 ) 
		shm_COMSEM->loop[WriterIdx].halt = 0; 
	/* At this time we are sure Writer is blocked on GateSem, Reader/Writer are thus 
	 * in sysnc when starting a new random packet size loop 
	 */ 
	 /* sleep(2); */  
     /* 2 */
#ifdef __DEBUG__
	 /* Now done in comwrite function */  
     sprintf(stats->msg_text, "WriterIdx=%d, GetPktSizes: 2 \n", WriterIdx);
     hxfmsg(stats, HTXERROR(EX_COMSEM2,0), HTX_HE_INFO, stats->msg_text);
#endif

	 memset(shm_COMSEM->loop[WriterIdx].rand_pktsize, 0, (sizeof(u_short) * USHRT_MAX));
	 if((num_pktsize_req > USHRT_MAX) || (num_pktsize_req != num_oper)) {
		num_pktsize_req = USHRT_MAX ;  
		sprintf(stats->msg_text, "GetPktSizes: NUm Random pkt size requested > USHRT_MAX. Generating = %d pkts size. \n",USHRT_MAX); 
		hxfmsg(stats, HTXERROR(EX_COMSEM2,0), HTX_HE_INFO, stats->msg_text);
	 }
	 
	 
   	if(bufmax == bufmin) {
        bufmin = 10 ;
        sprintf(stats->msg_text, "W : stress_level = 6 and buf_min = buf_max = %u. Considering buf_min = %d \n", bufmax, bufmin);
        hxfmsg(stats, 0, HTX_HE_INFO, stats->msg_text);
    }

	 for(i =0 ; i < num_pktsize_req; i++) { 
		temp_buf_size = bufmin + random() % (bufmax - bufmin);
		shm_COMSEM->loop[WriterIdx].rand_pktsize[i] = htons((u_short) 0xffff & temp_buf_size);   
	 }
	 /* 3 */ 
#ifdef __DEBUG__
     sprintf(stats->msg_text, "WriterIdx=%d, GetPktSizes: 3 \n", WriterIdx);
     hxfmsg(stats, HTXERROR(EX_COMSEM2,0), HTX_HE_INFO, stats->msg_text);
#endif 
	 memset(&CMsg, 0, sizeof(struct CoordMsg));
	 /* Inform reader which packet is coming up, how many random packets we generated     
	  * I need to connect writer socket before sending packets packets 
	  */   
	
	CMsg.msg_type = htonl(PKTSIZES);
        CMsg.ID.Bsize.WriterIdx = htons((u_short) 0xffff & WriterIdx);
        CMsg.ID.Bsize.bufmin = htons((u_short) 0xffff & bufmin);
        CMsg.ID.Bsize.bufmax = htons((u_short) 0xffff & bufmax);
        CMsg.ID.Bsize.num_oper = htons((u_short) 0xffff & num_oper) ;
        CMsg.ID.Bsize.num_pktsizes = htons((u_short) 0xffff & num_pktsize_req) ;


#ifdef __DEBUG__
 	 sprintf(stats->msg_text, "WriterIdx=%d, GetPktSizes: 3.1 \n", WriterIdx);
     hxfmsg(stats, HTXERROR(EX_COMSEM2,0), HTX_HE_INFO, stats->msg_text);
#endif 
	 StreamWrite(msgsock, (char *)&CMsg, sizeof(struct CoordMsg)); 

	 /* Now send the actual random packet sizes */ 
	 memset(&CMsg, 0, sizeof(struct CoordMsg));
	 CMsg.msg_type = htonl(PKTSIZES);
	 shm_COMSEM->loop[WriterIdx].num_pktsizes = num_pktsize_req; 
	 StreamWrite(msgsock, (char *)shm_COMSEM->loop[WriterIdx].rand_pktsize, (sizeof(u_short) * num_pktsize_req));

	 /* Writer still waiting on WriterIdx Semaphore. This Semaphore gets resets when we receive ACK_PKTSIZES */
	memset(&CMsg, 0, sizeof(struct CoordMsg)); 
	rc = StreamRead(msgsock, (char *)&CMsg, sizeof(struct CoordMsg));
	if(rc == -1 || ntohl(CMsg.msg_type ) != ACK_PKTSIZES) { 
		sprintf(stats->msg_text, "Error : getting ACK PKTSIZES from reader, rc = %d", rc); 
		hxfmsg(stats, HTXERROR(EX_WRITE8,ERRNO), HTX_HE_HARD_ERROR, stats->msg_text);
    }
	NetToHostBsize_t(&CMsg.ID.Bsize);
    /* 4 */
#ifdef __DEBUG__
     sprintf(stats->msg_text, "WriterIdx=%d, GetPktSizes: 4 \n", CMsg.ID.Bsize.WriterIdx);
     hxfmsg(stats, HTXERROR(EX_COMSEM2,0), HTX_HE_INFO, stats->msg_text);
#endif
	SetSem(SemID, CMsg.ID.Bsize.WriterIdx, WRITEAHEAD, stats); 
}

static void HaltWrite(struct semid_t ID, struct htx_data * stats)
{
    if(shm_COMSEM->loop[(int)ID.WriteIdx].halt)
        return;
    shm_COMSEM->loop[(int)ID.WriteIdx].CheckReset = 1;

    SetSem(SemID, ID.WriteIdx, 0, stats);
    shm_COMSEM->loop[(int)ID.WriteIdx].halt = 1;
/*
    sprintf(stats->msg_text, "comsem: Received HALT for writer index %d\n", (int)ID.WriteIdx);
    hxfmsg(stats, HTXERROR(EX_COMSEM3,0), HTX_HE_INFO, stats->msg_text);
*/
}

static void TerminateWrite(struct semid_t ID, struct htx_data * stats)
{
/*
    sprintf(stats->msg_text, "TerminatWrite called: \n");
    hxfmsg(stats, HTXERROR(EX_COMSEM3,0), HTX_HE_INFO, stats->msg_text);
*/
    shm_COMSEM->loop[(int)ID.WriteIdx].halt = 1;
    shm_COMSEM->loop[(int)ID.WriteIdx].TERM = 1;
    shm_COMSEM->loop[(int)ID.WriteIdx].CheckReset = 1;
    UpSem(SemID, (int)ID.WriteIdx, stats);
}



static void ShutdownWrite(struct htx_data * stats)
{
   int i;

/*
    sprintf(stats->msg_text, "ShutdownWrite called: \n");
    hxfmsg(stats, HTXERROR(EX_COMSEM3,0), HTX_HE_INFO, stats->msg_text);
*/

   for(i=0; i<shm_COMSEM->NoWriters; i++) {
		shm_COMSEM->loop[i].CheckReset = 1;
		SetSem(SemID, i, 0, stats);
		shm_COMSEM->loop[i].halt = 1;
   }
}



static void Terminating(struct htx_data * stats)
{
   int i;

/*
    sprintf(stats->msg_text, "Writer Terminating called: \n");
    hxfmsg(stats, HTXERROR(EX_COMSEM3,0), HTX_HE_INFO, stats->msg_text);
*/
   /* This guarantees the writer will see flags and exit */
   for(i=0; i<shm_COMSEM->NoWriters; i++)
		shm_COMSEM->loop[i].CheckReset = 1;
       SetSem(SemID, i, 1, stats);
   HE_exit(0);
}



static void ReadAck(struct semid_t ID, struct htx_data * stats)
{
    int NoAcks;

    if(shm_COMSEM->loop[(int)ID.WriteIdx].halt)
        return;

/********************************************************************/
/* If we have reset, don't recognize old information.               */
/********************************************************************/
    if((int)shm_COMSEM->loop[(int)ID.WriteIdx].timestamp != (int)ID.timeS)
        return;

    NoAcks = ((int)ID.ackNo + TP_MAX_SEQNOS - shm_COMSEM->loop[(int)ID.WriteIdx].last) % TP_MAX_SEQNOS;
/********************************************************************/
/* If we calculate too many acks, just return and force a reset.    */
/********************************************************************/
    if(NoAcks > (int)ID.WriteAhead) {
        sprintf(stats->msg_text, "comsem: ACK>%d ,writer %d - ack.no=%d, lastack=%d, numoper=%d, timestamp=%d.\n",
            (int)ID.WriteAhead, (int)ID.WriteIdx, (int)ID.ackNo, (int)shm_COMSEM->loop[(int)ID.WriteIdx].last, (int)ID.NumOper, (int)ID.timeS);
        hxfmsg(stats, HTXERROR(EX_COMSEM4,0), HTX_HE_INFO, stats->msg_text);
        return;
    }
    shm_COMSEM->loop[(int)ID.WriteIdx].last = ID.ackNo;

    IncSem(SemID, ID.WriteIdx, NoAcks, stats);
}



static void ExitCleanup(void)
{
    static int cleanup_in_progress = FALSE;

    if (!cleanup_in_progress) {
        cleanup_in_progress = TRUE;
        Detach_COMSEM();
    }
}



static int GetUniqueWIdx(struct htx_data * stats)
{
   int No;

    GlobalWait(WRITE_SEM,stats);
   No = shm_COMSEM->NoWriters++;
    GlobalSignal(WRITE_SEM, stats);

   if(No >= (MAXREMOTE)) {
      (void) sprintf(stats->msg_text, "GetUniqueWIdx: Too many writer processes. There are probably restarts of writers due to broken pipes.\n");
      (void) hxfmsg(stats, HTXERROR(EX_COMSEM7,0), HTX_HE_SOFT_ERROR, stats->msg_text);
      HE_exit(EX_COMSEM7);
   }
   return No;
}



void Detach_COMSEM(void)
{
    int MID;
    struct shmid_ds ShmDs;

    MID = shm_COMSEM->mID;
    shmdt((char *)shm_COMSEM);

    shmctl(MID, IPC_STAT, &ShmDs);
    if(ShmDs.shm_nattch == 0) {
         shmctl(MID, IPC_RMID, 0);
    }
}


void InitWriteVars(struct htx_data *stats)
{
    int i;

/********************************************************************/
/* Setup comsem memory to share with writers.                       */
/********************************************************************/
    if((shm_pHXECOM->mID_COMSEM = shmget(IPC_PRIVATE, sizeof(struct shm_comsem_t),
                        IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                                                     S_IROTH | S_IWOTH)) == -1) {
        sprintf(stats->msg_text, "InitWriteVars: Error getting shared memory - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_COMSEM17,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
        HE_exit(EX_COMSEM17);
    }
    if((int)(shm_COMSEM = (struct shm_comsem_t *) shmat(shm_pHXECOM->mID_COMSEM, 0, 0)) ==  -1) {
        sprintf(stats->msg_text, "InitWriteVars: Error attaching shared memory - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_COMSEM18,ERRNO), HTX_HE_SOFT_ERROR, stats->msg_text);
        HE_exit(EX_COMSEM18);
    }
    shm_COMSEM->mID = shm_pHXECOM->mID_COMSEM;
    SemID = DefineMySems(shm_pHXECOM->ExerNo);
}

