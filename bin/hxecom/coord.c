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
static char sccsid[] = "@(#)95  1.17.4.6  src/htx/usr/lpp/htx/bin/hxecom/coord.c, exer_com, htx53A 6/11/04 09:41:38";
/*
 *   COMPONENT_NAME: exer_com
 *
 *   FUNCTIONS: AddrRemote
 *		BroadcastLocals
 *		BroadcastMsg
 *		Coord_exit
 *		Detach_COORD
 *		ExitCleanup
 *		HE_exit
 *		Kill
 *		LocalExerTerm
 *		LocalServer
 *		NewRemoteAddr
 *		RemServer
 *		RemoteExerTerm
 *		RemoveRemoteAddr
 *		SIGALRM_hdl
 *		SIGPIPE_hdl
 *		SIGTERM_hdl2
 *		SIGINT_hdl2
 *		SendStreamMsg
 *		SetUpCoordShm
 *		ShouldFork
 *		Shutdown
 *		StartThread
 *		WSACleanUP
 *		main
 *		processMsg
 *		processMsg_t
 *              com_hxfmsg
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
#    include <stdio.h>
#    include <stdlib.h>
#    include <unistd.h>
#    include <time.h>
#    include <string.h>
#    include <memory.h>
#    include <sys/types.h>
#    include <sys/sem.h>
#    include "hxecomdef.h"
#    include <sys/time.h>
#    include <sys/select.h>
#    include <sys/shm.h>
#    include <sys/ioctl.h>
#    include "comrw.h"

#ifdef __HTX_LINUX__
#    include <sys/stat.h>
#else
#    include <fcntl.h>
#endif

int com_hxfmsg(struct htx_data *p, int err, enum sev_code  sev, char *text);
static int  SendStreamMsg(struct sockaddr_in ID, struct CoordMsg CMsg, struct resp_t * resp);
static void ExitCleanup(void);
static void BroadcastMsg(struct CoordMsg CMsg, struct sockaddr_in BroadcastDest, int ComDgramSock, struct resp_t * resp);
static void Shutdown(struct CoordMsg CMsg, struct sockaddr_in BroadcastDest, int ComDgramSock, struct resp_t * resp);
static void LocalServer(struct CoordMsg CMsg, struct sockaddr_in BroadcastDest, int ComDgramSock, struct resp_t * resp);
static int  NewRemoteAddr(uint32_t RemAddr, uint32_t LocalAddr, struct resp_t * resp);
static void RemoveRemoteAddr(uint32_t RemAddr);
static int  AddrRemote(uint32_t Addr, uint32_t LocalAddr);
static int  ShouldFork(struct CoordMsg CMsg, uint32_t LocalAddr, struct resp_t * resp);
static void RemServer(struct CoordMsg CMsg, uint32_t LocalAddr, u_short ComStreamPort, struct resp_t * resp);
static void processMsg(struct CoordMsg CMsg, struct sockaddr_in BroadcastDest, int ComDgramSock, 
                           u_short ComStreamPort, u_short ComDgramPort, uint32_t LocalAddr, struct resp_t * resp);
static void LocalExerTerm(struct CoordMsg CMsg, u_short ComStreamPort, struct resp_t * resp);
static void RemoteExerTerm(struct CoordMsg CMsg, u_short ComStreamPort, struct resp_t * resp);
static void BroadcastLocals(struct sockaddr_in BroadcastDest, int ComDgramSock, struct resp_t * resp);
static void Coord_exit(int exitNo);
static void Detach_COORD(struct shm_coord_t * ptr);

static void SIGALRM_hdl(int sig, int code, struct sigcontext * scp);
static void SIGPIPE_hdl(int sig, int code, struct sigcontext * scp);
static void Kill(pid_t PID);
static void WSACleanUP(void);
static void SetUpCoordShm(struct resp_t * resp);
static void SIGTERM_hdl2(int sig, int code, struct sigcontext * scp);
static void SIGINT_hdl2(int sig, int code, struct sigcontext * scp);

/******************************************************************************/
/***  Global Variable Definitions  ********************************************/
/******************************************************************************/

static struct shm_coord_t  * shm_COORD  = (struct shm_coord_t *)NULL;

struct shm_hxecom_t        * shm_HXECOM = (struct shm_hxecom_t *)NULL;

char comhostname[32];
char global_htx_log_dir[256] = "/tmp";
char global_exer_log_dir[256] = "/tmp";
char global_htx_home_dir[256] = {'\0'};
/******************************************************************************/
/***  End of Global Variable Definitions  *************************************/
/******************************************************************************/

/* Used by semaphore  */
#define GLOBAL_SEMID  shm_HXECOM->MasterSemid

static int    mID;
static pid_t  ParentPID;
static int    ExerIdx;
static SOCKET ComStreamSock;

#define STATS      &resp->stats
#define MSG_TEXT   resp->stats.msg_text

struct processMsg_argt  {
    struct  CoordMsg CMsg; 
    struct sockaddr_in BroadcastDest; 
    int     ComDgramSock; 
    u_short ComStreamPort; 
    u_short ComDgramPort; 
    uint32_t  LocalAddr; 
    struct resp_t RESPONSE;
};

int main(int argc, char *argv[])
{
    int    on=1;
    u_short ComDgramPort;
    uint32_t  BroadcastAddr;
    struct sockaddr_in BroadcastDest;
    struct CoordMsg  CMsg;
    fd_set rd;
    struct sockaddr_in src;
    char   * ComName;
    int    rc;
    SOCKET    ComDgramSock, msgsock;
    uint32_t  LocalAddr;
    u_short ComStreamPort;
    struct resp_t RESPONSE;
    struct resp_t * resp = &RESPONSE;
    struct timeval timeout, *tvptr;
    struct sigaction sigvector;
    pid_t  fPID;
    int    nfd;
	int    tmpi;
	char   tmper[HXECOM_HOSTNAMELEN];
	FILE *f_data;
    char value[12];
	int netmask;
	int mymask;
	char addrStr[30];
	char *temp_env_val_ptr = NULL;
	char temp_string[300];
	
	sprintf(tmper,"%s",argv[1]);
	ComName = tmper;
    ComStreamPort     = atoi(argv[2]);
    ComDgramPort      = atoi(argv[3]);
    ExerIdx           = atoi(argv[6]);
    atexit(ExitCleanup); 


    AttachShmHxecom();

    memcpy(STATS, &shm_HXECOM->TempStats, sizeof(struct htx_data));
    AttachGLOBALSems(STATS);


/********************************************************************/
/*                                                                  */
/********************************************************************/
    ParentPID = getpid();

/********************************************************************/
/* Parents STATS file is now a copy we can use.                     */
/********************************************************************/

/********************************************************************/
/*** Set up SIGTERM, SIGCLD, etc.                                   */
/********************************************************************/
    sigemptyset(&(sigvector.sa_mask));
    sigvector.sa_flags = 0;

    sigvector.sa_handler = (void(*)(int))SIGTERM_hdl2;
    sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

    sigvector.sa_handler = (void(*)(int))SIGINT_hdl2;
    sigaction(SIGINT, &sigvector, (struct sigaction *) NULL);

    sigvector.sa_handler = SIG_IGN;
    sigaction(SIGCLD,  &sigvector, NULL);

    sigvector.sa_handler = (void(*)(int))SIGALRM_hdl;
    sigaction(SIGALRM, &sigvector, NULL);

    sigvector.sa_handler = (void(*)(int))SIGPIPE_hdl;
    sigaction(SIGPIPE, &sigvector, NULL);

    sigvector.sa_handler = SIG_IGN;
    sigaction(SIGQUIT, &sigvector, NULL);
    sigaction(SIGHUP,  &sigvector, NULL);

	temp_env_val_ptr = getenv("HTX_HOME_DIR");
	if( (temp_env_val_ptr != NULL) && (strlen(temp_env_val_ptr) > 0) ) {
		strcpy(global_htx_home_dir, temp_env_val_ptr);
	} else {
		printf("HTX environment is not setup, please setup HTX environment and re-tr\ny");
		printf("to setup HTX environment on same shell execute: . htx_setup.sh\n");
		printf("exiting...\n\n");
		exit(1);
	}

	temp_env_val_ptr = getenv("HTX_LOG_DIR");
	if( (temp_env_val_ptr != NULL) && (strlen(temp_env_val_ptr) > 0) ) {
		strcpy(global_htx_log_dir, temp_env_val_ptr);
	}
	
	if( (STATS.htx_exer_log_dir != NULL) && (strlen(STATS.htx_exer_log_dir) > 0) ) {
		strcpy(global_exer_log_dir, STATS.htx_exer_log_dir);
	}

/********************************************************************/
/* Setup Coordinator sockets                                        */
/********************************************************************/
	printf("%d : setup coordinateor sockets\n",getpid());  /*TWM*/
#ifdef __DEBUG__
	printf("%d : setup coordinateor sockets\n",getpid());
#endif
    ComStreamSock = socket(AF_INET, SOCK_STREAM, 0);
    if(ComStreamSock == INVALID_SOCKET) {
        sprintf(MSG_TEXT, "coord: Error opening stream socket - %s.\n", NEW_STRERR(errno));
        com_hxfmsg(STATS, HTXERROR(EX_SOCK5,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
        HE_exit(EX_SOCK5);
    }

/********************************************************************/
/* Setup to allow reuse of sock address on local machine.           */
/********************************************************************/
    on = 1;
    if(setsockopt(ComStreamSock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on))) {
        sprintf(MSG_TEXT, "coord: Error enabling address reuse on stream socket - %s.\n", NEW_STRERR(errno));
        com_hxfmsg(STATS, HTXERROR(EX_SETREUSE2,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
        Coord_exit(EX_SETREUSE2);
    }

    ComDgramSock = socket(AF_INET, SOCK_DGRAM, 0);
    if(ComDgramSock == INVALID_SOCKET) {
        sprintf(MSG_TEXT, "coord: Error opening datagram socket - %s.\n", NEW_STRERR(errno));
        com_hxfmsg(STATS, HTXERROR(EX_SOCK5,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
        HE_exit(EX_SOCK5);
    }

/********************************************************************/
/* Setup to allow reuse of sock address on local machine.           */
/********************************************************************/
    on = 1;
    if(setsockopt(ComDgramSock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on))) {
        sprintf(MSG_TEXT, "coord: Error enabling address reuse on datagram socket - %s.\n", NEW_STRERR(errno));
        com_hxfmsg(STATS, HTXERROR(EX_SETREUSE1,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
        Coord_exit(EX_SETREUSE1);
    }

/********************************************************************/
/* Setup to receive from any Coordinator that has ComPort number.   */
/********************************************************************/
    memset(&src, '\0', sizeof(src));
    src.sin_addr.s_addr = INADDR_ANY;
    src.sin_family = AF_INET;
#ifndef __HTX_LINUX__
    src.sin_len = sizeof(src);
#endif
    src.sin_port = htons(ComStreamPort);

    if(bind(ComStreamSock, (struct sockaddr *) &src, sizeof(src)) ) {
        /************************************************************/
        /* If coordinator is running, exit.                         */
        /* We allow multiple coordinators so network can be         */
        /* partitioned.  But only want one coordinator for each     */
        /* well-known port.                                         */
        /************************************************************/

        if(ERRNO == EADDRINUSE) {
            sprintf(MSG_TEXT, "coord: Address in use coord exit .\n");
            com_hxfmsg(STATS, HTXERROR(EX_BIND2,ERRNO), HTX_HE_INFO, MSG_TEXT);
            Coord_exit(EX_BIND4);
        }
        else {
            sprintf(MSG_TEXT, "coord: Error binding stream socket - %s.\n", NEW_STRERR(errno));
            com_hxfmsg(STATS, HTXERROR(EX_BIND2,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
            Coord_exit(EX_BIND2);
        }
    }

    memset(&src, '\0', sizeof(src));
    src.sin_addr.s_addr = INADDR_ANY;
    src.sin_family = AF_INET;
#ifndef __HTX_LINUX__
    src.sin_len = sizeof(src);
#endif
    src.sin_port = htons(ComDgramPort);
    if(bind(ComDgramSock, (struct sockaddr *) &src, sizeof(src)) ) {
        sprintf(MSG_TEXT, "coord: Error binding datagram socket - %s.\n", NEW_STRERR(errno));
        com_hxfmsg(STATS, HTXERROR(EX_BIND3,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
        Coord_exit(EX_BIND3);
    }
	
/********************************************************************/
/* Setup to enable broadcast on Coordinator datagram socket.        */
/********************************************************************/
	if(shm_HXECOM->OneSysFlag) {
		on = 1;
		if(setsockopt(ComDgramSock, SOL_SOCKET, SO_BROADCAST, (char *) &on, sizeof(on))) {
			sprintf(MSG_TEXT, "coord: Error enabling broadcast on datagram socket - %s.\n", NEW_STRERR(errno));
			com_hxfmsg(STATS, HTXERROR(EX_SETB1,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
			Coord_exit(EX_SETB1);
		}
	}

	strcpy(MSG_TEXT, "coord: ");
	if(GetIP(ComName, &BroadcastDest.sin_addr, MSG_TEXT+strlen(MSG_TEXT))) {   
		com_hxfmsg(STATS, HTXERROR(EX_GETH2,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
		Coord_exit(EX_GETH2);
	}
/********************************************************************/
/* Calculate broadcast address on the Com Net.                      */
/* NOTE: with the new version , broadcast is only done in one system test mode*/
/* for multi system test, I use the other_ids file to get a list of other */
/* systems in the test and just send connect requests to the system in the */
/* list */
/********************************************************************/
	LocalAddr = BroadcastAddr = BroadcastDest.sin_addr.s_addr;
	BroadcastAddr = ntohl(BroadcastAddr);
	InetNtoa(BroadcastDest.sin_addr, addrStr, STATS);
/*        printf(" comName addrstr:%s\n",addrStr); */
#ifndef __HTX_LINUX__ 
	/*TWM2*/
	/* get the netmask and use it to set up the Broadcast address */

	sprintf(temp_string, "%s/out", global_exer_log_dir);
	sprintf(MSG_TEXT, "netstat -in| grep %s | awk {'print $1'} > %s", addrStr, temp_string);
	system(MSG_TEXT);
	f_data=fopen(temp_string, "r");
	fscanf(f_data,"%s",value);    /* get interface name */
	fclose(f_data);

	sprintf(MSG_TEXT, "ifconfig %s | awk {'print $4'} > %s", value, temp_string);
	system(MSG_TEXT);
	f_data=fopen(temp_string, "r");
	fscanf(f_data,"%s",value);    /* get mask value  form 0xfffffe00*/
	fclose(f_data);
	sscanf(value,"%x",&netmask);

	mymask = 0xffffffff;
	mymask ^= netmask;
	BroadcastAddr |= mymask;
		BroadcastAddr = htonl(BroadcastAddr);
#else
	if ( getBroadcastAddr( addrStr, &BroadcastAddr) != 0 ) {
			 com_hxfmsg(STATS, HTXERROR(EX_GETH2,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
			 Coord_exit(EX_GETH2);
		}
#endif

/********************************************************************/
/* Setup to broadcast destination on Coordinator datagram socket.   */
/********************************************************************/
	memset(&BroadcastDest, '\0', sizeof(BroadcastDest));
	BroadcastDest.sin_addr.s_addr = BroadcastAddr; 
#ifndef __HTX_LINUX__ 
	BroadcastDest.sin_len = sizeof(BroadcastDest);
#endif
	BroadcastDest.sin_family = AF_INET;
	BroadcastDest.sin_port = htons(ComDgramPort);

    if(listen(ComStreamSock, MAXREMOTE)) {
        sprintf(MSG_TEXT, "coord: Error in listen() - %s\n", NEW_STRERR(errno));
        com_hxfmsg(STATS, HTXERROR(EX_LISTEN1,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
        Coord_exit(EX_LISTEN1);
    }

    SetUpCoordShm(resp);

    shm_COORD->mID = mID;
    shm_COORD->BroadcastInterval = atoi(argv[4]);
    shm_COORD->NumberBroadcast   = atoi(argv[5]);
    shm_COORD->BroadcastAlarm    = 0;
    shm_COORD->BroadcastIntervalsRemaining = 0;

/********************************************************************/
/* Inform hxecom(s) that we are ready to communicate.               */
/********************************************************************/
#ifdef __DEBUG__
	printf("%d : informing hxecom we are ready to communicate\n",getpid());
#endif
    if(shm_HXECOM->NoCoord >= MAX_COORD) {
        sprintf(MSG_TEXT, "coord: Maximum number of coordinators exceeded - %d limit\n", MAX_COORD);
        com_hxfmsg(STATS, HTXERROR(EX_COORD1,0), HTX_HE_SOFT_ERROR, MSG_TEXT);
        Coord_exit(EX_COORD1);
    }
    GlobalWait(COORD_SEM, STATS);
    shm_HXECOM->Coord[shm_HXECOM->NoCoord].CoordPID = getpid();
    shm_HXECOM->Coord[shm_HXECOM->NoCoord].ComPortStream = (u_short) ComStreamPort;
    shm_HXECOM->NoCoord++;
    GlobalSignal(COORD_SEM, STATS);


/********************************************************************/
/* Connect to sockets wishing to communicate and respond.           */
/********************************************************************/
    do {
        memset(&CMsg, '\0', sizeof(CMsg));
        FD_ZERO(&rd);
        FD_SET(ComStreamSock, &rd);
        FD_SET(ComDgramSock, &rd);
		if(shm_COORD->BroadcastAlarm == 0) {
			tvptr = NULL;
		}
		else {
				timeout.tv_sec   = shm_COORD->BroadcastAlarm;
				timeout.tv_usec = 0;
			tvptr = &timeout;
		}
        nfd = ComDgramSock + 1;
        rc = select(nfd, &rd, NULL, NULL, tvptr);
        if(rc == 0) {
            if(shm_COORD->BroadcastAlarm > 0 && 
                              shm_COORD->BroadcastIntervalsRemaining > 0) {
                CMsg.msg_type = CM_BROADCAST_LOCALS;
                if(ShouldFork(CMsg, LocalAddr, resp)) {
#ifdef __DEBUG__
    printf("%d : forking to process msg\n",getpid());
#endif

                    fPID = fork();
                    switch(fPID) {
                        case 0:
                            processMsg(CMsg, BroadcastDest, ComDgramSock, ComStreamPort, ComDgramPort, LocalAddr, resp);
                            Coord_exit(0);
                        case -1:
                            sprintf(MSG_TEXT, "coord: Error forking Coord process - %s.\n", NEW_STRERR(errno));
                            com_hxfmsg(STATS, HTXERROR(EX_FORK5,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
                            Coord_exit(EX_FORK5);
                        default:
                            break;
                    }
                }
            }
        }
        else if(rc == SOCKET_ERROR) {
            sprintf(MSG_TEXT, "coord: Error in select() - %s.\n", NEW_STRERR(errno));
            com_hxfmsg(STATS, HTXERROR(EX_SELECT1,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
            Coord_exit(EX_SELECT1);
        }
/********************************************************************/
/* Give Datagrams a higher priority than Streams.                   */
/********************************************************************/
        else if(FD_ISSET(ComDgramSock, &rd)) {
            rc = recvfrom(ComDgramSock, (char *)&CMsg, sizeof(CMsg), 0, NULL, NULL);
			if(rc == SOCKET_ERROR) {
			    sprintf(MSG_TEXT, "coord: Error in recvfrom() - %s.\n", NEW_STRERR(errno));
                com_hxfmsg(STATS, HTXERROR(EX_SELECT1,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
                Coord_exit(EX_SELECT1);
            }    
            CMsg.msg_type = ntohl(CMsg.msg_type);
            if(ShouldFork(CMsg, LocalAddr, resp)) {
                fPID = fork();
                switch(fPID) {
                    case 0:
                        processMsg(CMsg, BroadcastDest, ComDgramSock, ComStreamPort, ComDgramPort, LocalAddr, resp);
                        Coord_exit(0);
                    case -1:
                        sprintf(MSG_TEXT, "Error forking Coord process - %s.\n", NEW_STRERR(errno));
                        com_hxfmsg(STATS, HTXERROR(EX_FORK6,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
                        Coord_exit(EX_FORK6);
                    default:
                        break;
                }
            }
            else {
                processMsg(CMsg, BroadcastDest, ComDgramSock, ComStreamPort, ComDgramPort, LocalAddr, resp);
            }
        }
        else if(FD_ISSET(ComStreamSock, &rd)) {
            msgsock = accept(ComStreamSock, NULL, NULL);
            if(msgsock == INVALID_SOCKET) {
                sprintf(MSG_TEXT, "coord: Stream process Error on accept() - %s.\n", NEW_STRERR(errno));
                com_hxfmsg(STATS, HTXERROR(EX_COORD2,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
                break;
            }
            if((rc = StreamReadEof(msgsock, (char *) &CMsg, sizeof(CMsg))) == 0) {
                closesocket(msgsock);
                continue;
            }
            else if(rc == -1) {
                closesocket(msgsock);
                sprintf(MSG_TEXT, "Error reading message - %s.\n", NEW_STRERR(errno));
                com_hxfmsg(STATS, HTXERROR(EX_COORD3,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
                continue;
            }
            closesocket(msgsock);
            CMsg.msg_type = ntohl(CMsg.msg_type);
            if(ShouldFork(CMsg, LocalAddr, resp)) {
                fPID = fork();
                switch(fPID) {
                    case 0:
                        closesocket(ComStreamSock);
                        processMsg(CMsg, BroadcastDest, ComDgramSock, ComStreamPort, ComDgramPort, LocalAddr, resp);
                        Coord_exit(0);
                    case -1:
                        sprintf(MSG_TEXT, "coord: Error forking Coord process - %s.\n", NEW_STRERR(errno));
                        com_hxfmsg(STATS, HTXERROR(EX_FORK7,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
                        Coord_exit(EX_FORK7);
                    default:
                        break;
                }
            }
            else {
                processMsg(CMsg, BroadcastDest, ComDgramSock, ComStreamPort, ComDgramPort, LocalAddr, resp);
            }
        }
        else {
            sprintf(MSG_TEXT, "coord: Error in selecting. rc=%d, errno=%s - %s\n", rc, ERRNO, NEW_STRERR(errno));
            com_hxfmsg(STATS, HTXERROR(EX_COORD4,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
        }
    } while(1);
    sprintf(MSG_TEXT, "coord: Exiting\n");    
    com_hxfmsg(STATS, HTXERROR(EX_COORD5,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
    Coord_exit(EX_COORD5);
    return 0;
}

static void processMsg(struct CoordMsg CMsg, struct sockaddr_in BroadcastDest, int ComDgramSock, 
                           u_short ComStreamPort, u_short ComDgramPort, uint32_t LocalAddr, struct resp_t * resp)
{
  
    switch(CMsg.msg_type) {
        case CM_SIGTERM:
            shutdown(ComStreamSock, 2);
            Coord_exit(EX_COORD6);
        case CM_LOCALSERVER:
            LocalServer(CMsg, BroadcastDest, ComDgramSock, resp);
            break;
        case CM_SHUTDOWN:
        case CM_BROADCAST_SHUTDOWN:
            Coord_exit(EX_COORD7);
        case CM_REMOTESERVER:
            RemServer(CMsg, LocalAddr, ComStreamPort, resp);
            break;
        case CM_LOCAL_EXER_TERM:
            LocalExerTerm(CMsg, ComStreamPort, resp);
            break;
        case CM_REMOTE_EXER_TERM:
            RemoteExerTerm(CMsg, ComStreamPort, resp);
            break;
        case CM_BROADCAST_LOCALS:
            BroadcastLocals(BroadcastDest, ComDgramSock, resp);
            break;
        default:
            sprintf(MSG_TEXT, "coord: Unknown msg_type %d\n", (int)CMsg.msg_type);
            com_hxfmsg(STATS, HTXERROR(EX_COORD8,0), HTX_HE_SOFT_ERROR, MSG_TEXT);
            break;
    }
}



static int ShouldFork(struct CoordMsg CMsg, uint32_t LocalAddr, struct resp_t * resp)
{
    int temp, i;

    /****************************************************************************/
    /* If can be handled quickly, handle here. Otherwise return 1 so can fork   */
    /* a new process.                                                           */
    /****************************************************************************/
    switch(CMsg.msg_type) {
        case CM_SIGTERM:
            return 0;
        case CM_SHUTDOWN:
        case CM_BROADCAST_SHUTDOWN:
            return 0;
        case CM_LOCALSERVER:
        case CM_LOCAL_EXER_TERM:
        case CM_REMOTE_EXER_TERM:
        case CM_BROADCAST_LOCALS:
            return 1;
        case CM_REMOTESERVER:
            if(AddrRemote(CMsg.ID.server.sock.sin_addr.s_addr, LocalAddr) == 0)
                return 0;
            /********************************************************************/
            /* If old info, ignore. Don't need to lock for quick look.          */
            /********************************************************************/
            temp = shm_COORD->NoRemotes;
            for(i=0; i<temp; i++) {
                if(shm_COORD->RemoteIDs[i].sock.sin_port == CMsg.ID.server.sock.sin_port &&
                   shm_COORD->RemoteIDs[i].sock.sin_addr.s_addr == CMsg.ID.server.sock.sin_addr.s_addr) {
                    return 0;
                }
            }
            return 1;
        default:
            sprintf(MSG_TEXT, "coord: Unknown msg_type %d\n", (int)CMsg.msg_type);
            com_hxfmsg(STATS, HTXERROR(EX_COORD9,0), HTX_HE_SOFT_ERROR, MSG_TEXT);
            return 0;
    }
}



static void Shutdown(struct CoordMsg CMsg, struct sockaddr_in BroadcastDest, int ComDgramSock, struct resp_t * resp)
{
    int i;
    int againFlag = 0;

    if(CMsg.msg_type == CM_SHUTDOWN) {
        CMsg.msg_type = htonl(CM_BROADCAST_SHUTDOWN);
        BroadcastMsg(CMsg, BroadcastDest, ComDgramSock, resp);
        againFlag = 1;
    }
    for(i=0; i < shm_HXECOM->NoExer; i++) {
        if(shm_HXECOM->exer[i].dead_flag)
            continue;
        Kill(shm_HXECOM->exer[i].EXER_PID);
    }
    if(againFlag)
        BroadcastMsg(CMsg, BroadcastDest, ComDgramSock, resp);
}



static void LocalExerTerm(struct CoordMsg CMsg, u_short ComStreamPort, struct resp_t * resp)
{
    int i, rc;
    int Index = -1;
    struct sockaddr_in dest;

/********************************************************************/
/* Find exerciser in local list.                                    */
/********************************************************************/
    GlobalWait(COORD_SEM, STATS);
    for(i=0; i < shm_COORD->NoLocals; i++)
        if(CMsg.ID.server.sock.sin_port == shm_COORD->LocalIDs[i].sock.sin_port) {
            Index = i;
            break;
        }

    if(Index == -1) {
        sprintf(MSG_TEXT, "LocalExerTerm: Local Server (Terminating) not found in table\n");
        com_hxfmsg(STATS, HTXERROR(EX_COORD10,0), HTX_HE_SOFT_ERROR, MSG_TEXT);
        GlobalSignal(COORD_SEM, STATS);
        return;
    }

/********************************************************************/
/* Remove local terminating exerciser from local list.              */
/********************************************************************/
    shm_COORD->NoLocals--;
    for(i=Index; i< shm_COORD->NoLocals; i++) {
        memcpy(&shm_COORD->LocalIDs[i], &shm_COORD->LocalIDs[i+1], sizeof(struct id_t));
        shm_COORD->LocalExerNo[i+1] = shm_COORD->LocalExerNo[i];
    }

/********************************************************************/
/* Inform remote coordinators of local exerciser termination.       */
/********************************************************************/
    memset(&dest, '\0', sizeof(dest));
    dest.sin_family = AF_INET;
#ifndef __HTX_LINUX__
    dest.sin_len = sizeof(dest);
#endif
    dest.sin_port = htons(ComStreamPort);
    CMsg.msg_type = htonl(CM_REMOTE_EXER_TERM);
    for(i=0; i < shm_COORD->NoRemoteAddr; i++) {
        dest.sin_addr.s_addr = shm_COORD->RemoteAddr[i];
        if((rc = SendStreamMsg(dest, CMsg, resp)) < 0) {
            GlobalSignal(COORD_SEM, STATS);
            Coord_exit(-rc);
        }
    }
    GlobalSignal(COORD_SEM, STATS);
}



static void RemoteExerTerm(struct CoordMsg CMsg, u_short ComStreamPort, struct resp_t * resp)
{
    int i;
    int Index = -1;

/********************************************************************/
/* CleanUp received struct sockaddr_in structure.  If a message     */
/* is from a remote host, it may not be AIX.                        */
/********************************************************************/
    FixSockAddr(&CMsg.ID.server.sock);

/********************************************************************/
/* Find exerciser in remote list.                                   */
/********************************************************************/
    GlobalWait(COORD_SEM, STATS);
    for(i=0; i < shm_COORD->NoRemotes; i++)
        if(CMsg.ID.server.sock.sin_port == shm_COORD->RemoteIDs[i].sock.sin_port &&
           CMsg.ID.server.sock.sin_addr.s_addr == shm_COORD->RemoteIDs[i].sock.sin_addr.s_addr) {
            Index = i;
            break;
        }

    if(Index == -1) {
        sprintf(MSG_TEXT, "RemoteExerTerm: Remote Server (Terminating) not found in table\n");
        com_hxfmsg(STATS, HTXERROR(EX_COORD11,0), HTX_HE_SOFT_ERROR, MSG_TEXT);
        GlobalSignal(COORD_SEM, STATS);
        return;
    }

/********************************************************************/
/* Remove remote terminating exerciser from local list.             */
/********************************************************************/
    shm_COORD->NoRemotes--;
    for(i=Index; i < shm_COORD->NoRemotes; i++) {
        memcpy(&shm_COORD->RemoteIDs[i], &shm_COORD->RemoteIDs[i+1], sizeof(struct id_t));
    }

/********************************************************************/
/* Remove remote terminating exerciser address from RemoteAddr list.*/
/* If address doesn't exist in RemoteIDs, must remove from          */
/* RemoteAddr list.                                                 */
/********************************************************************/
    for(i=0; i < shm_COORD->NoRemotes; i++)
        if(shm_COORD->RemoteIDs[i].sock.sin_addr.s_addr == CMsg.ID.server.sock.sin_addr.s_addr)
            break;
    if(i >= shm_COORD->NoRemotes)
        RemoveRemoteAddr(CMsg.ID.server.sock.sin_addr.s_addr);

    GlobalSignal(COORD_SEM, STATS);
}



static void LocalServer(struct CoordMsg CMsg, struct sockaddr_in BroadcastDest, int ComDgramSock, struct resp_t * resp)
{
    struct CoordMsg tMsg;
    int i, rc;
    int Index = -1;

    GlobalWait(COORD_SEM, STATS);
    for(i=0; i<shm_HXECOM->NoExer; i++) {
        if(CMsg.ID.server.sock.sin_port == shm_HXECOM->exer[i].ServerID.sock.sin_port) {
            Index = i;
#ifdef __DEBUG__
    printf("%d : my index is %d, ComDgramSock = 0x%x\n",
           getpid(), Index, ComDgramSock);
#endif
            break;
        }
    }
    if(Index == -1) {
        sprintf(MSG_TEXT, "LocalServer: Local Server not found in table, Cmsg port = %d, Server port = %d\n", CMsg.ID.server.sock.sin_port, 
										shm_HXECOM->exer[i].ServerID.sock.sin_port);
        com_hxfmsg(STATS, HTXERROR(EX_COORD12,0), HTX_HE_SOFT_ERROR, MSG_TEXT);
        GlobalSignal(COORD_SEM, STATS);
        return;
    }

/********************************************************************/
/* Add server to Local list.                                        */
/********************************************************************/
    if(shm_COORD->NoLocals >= (MAXLOCAL)) {
        sprintf(MSG_TEXT, "LocalServer: Maximum Local Servers exceeded. MAXLOCAL=%d\n", MAXLOCAL);
        com_hxfmsg(STATS, HTXERROR(EX_COORD22,0), HTX_HE_SOFT_ERROR, MSG_TEXT);
        GlobalSignal(COORD_SEM, STATS);
        Coord_exit(EX_COORD22);
    }
    shm_COORD->LocalExerNo[shm_COORD->NoLocals] = Index;
    memcpy(&shm_COORD->LocalIDs[shm_COORD->NoLocals++], &CMsg.ID.server, sizeof(struct id_t));

/********************************************************************/
/* Notify local server of all known remote servers.                 */
/********************************************************************/
/*
    sprintf(MSG_TEXT, "LocalServer: CM_LOCAL SERVER  @ (%s, %d)\n", inet_ntoa(CMsg.ID.server.sock.sin_addr), 
                                              (int) ntohs(CMsg.ID.server.sock.sin_port));
    com_hxfmsg(STATS, HTXERROR(EX_COORD13,0), HTX_HE_INFO, MSG_TEXT);
*/

    tMsg.msg_type = htonl(CM_REMOTESERVER);
    for(i=0; i<shm_COORD->NoRemotes; i++) {
        memcpy(&tMsg.ID.server, &shm_COORD->RemoteIDs[i], sizeof(struct id_t));
        if((rc = SendStreamMsg(CMsg.ID.server.sock, tMsg, resp)) < 0) {
            GlobalSignal(COORD_SEM, STATS);
            Coord_exit(-rc);
        }
    }

#ifdef __DEBUG__
    printf("%s:%d : sending REMOTESERVER request to all servers sbt new server\n", __FUNCTION__,
           getpid());
#endif
/********************************************************************/
/* Inform all other initialized local servers of new local server.  */
/********************************************************************/
	CMsg.msg_type = htonl(CM_LOCALSERVER);
    for(i=0; i<shm_COORD->NoLocals; i++) {
		if(shm_HXECOM->OneSysFlag==1) {
			if((shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].ServerIDinit == 0) || shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].dead_flag) {
				continue;
			}
		} else {
			if(Index == i || (shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].ServerIDinit == 0) || 
										  shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].dead_flag) {
				continue;
			}
		}
#ifdef __DEBUG__
    printf("%d : my index: %d sending CM_LOCALSERVER to indx:%d port:%d addr:0x%x  \n",
       getpid(),Index,i, shm_COORD->LocalIDs[i].sock.sin_port,
       shm_COORD->LocalIDs[i].sock.sin_addr.s_addr);
#endif
        if((rc = SendStreamMsg(shm_COORD->LocalIDs[i].sock, CMsg, resp)) < 0) {
            GlobalSignal(COORD_SEM, STATS);
            Coord_exit(-rc);
        }
    }

/********************************************************************/
/* Inform new local server of any other initialized local servers.  */
/********************************************************************/
    tMsg.msg_type = htonl(CM_LOCALSERVER);
    for(i=0; i<shm_COORD->NoLocals; i++) {
		if(shm_HXECOM->OneSysFlag==1) {
			if((shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].ServerIDinit == 0) || shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].dead_flag) {
				continue;
			}
		} else {
			if(Index == i || (shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].ServerIDinit == 0) || 
										  shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].dead_flag) {
				continue;
			}
		}
        
        memcpy(&tMsg.ID.server, &shm_COORD->LocalIDs[i], sizeof(struct id_t));
#ifdef __DEBUG__
       printf( "%d: send CM_LOCALSERVER of %d to Indx: %d\n",
           getpid(),i, Index);
#endif
        if((rc = SendStreamMsg(CMsg.ID.server.sock, tMsg, resp)) < 0) {
            GlobalSignal(COORD_SEM, STATS);
            Coord_exit(-rc);
        }
    }

    shm_HXECOM->exer[Index].ServerIDinit = 1;

/********************************************************************/
/* Inform all remote coordinators of new server.                    */
/********************************************************************/
    CMsg.msg_type = htonl(CM_REMOTESERVER);
    BroadcastMsg(CMsg, BroadcastDest, ComDgramSock, resp);
	/* trun other coords loose */
    GlobalSignal(COORD_SEM, STATS);

/********************************************************************/
/* Initiate routine broadcast of all local servers known.           */
/********************************************************************/
    shm_COORD->BroadcastAlarm = shm_COORD->BroadcastInterval;
    shm_COORD->BroadcastIntervalsRemaining = shm_COORD->NumberBroadcast;
}


static void RemServer(struct CoordMsg CMsg, uint32_t LocalAddr, u_short ComStreamPort, struct resp_t * resp)
{
    int i, rc;
    struct CoordMsg Msg;
    struct sockaddr_in dest;
    int NewRemoteCoordFlag = 0;

	if(AddrRemote(CMsg.ID.server.sock.sin_addr.s_addr, LocalAddr) == 0) {
		return;
	}

/********************************************************************/
/* CleanUp received struct sockaddr_in structure.  If a message     */
/* is from a remote host, it may not be AIX.                        */
/********************************************************************/
    FixSockAddr(&CMsg.ID.server.sock);

    GlobalWait(COORD_SEM, STATS);
/********************************************************************/
/* If old info, ignore.                                             */
/********************************************************************/
    for(i=0; i<shm_COORD->NoRemotes; i++) {
        if(shm_COORD->RemoteIDs[i].sock.sin_port == CMsg.ID.server.sock.sin_port &&
           shm_COORD->RemoteIDs[i].sock.sin_addr.s_addr == CMsg.ID.server.sock.sin_addr.s_addr) {
            GlobalSignal(COORD_SEM, STATS);
#ifdef __DEBUG__
    printf("%d : obtained REMOTESERVER request from old server\n",
           getpid());
#endif
            return;
        }
    }
/********************************************************************
 * If co-ordinator receives un solicited packets then it may lead to
 * corrupted Remote Server Information, leading to Hxecom Server error
 * out. Remote server packet should be validated against /tmp/other_ids
 * and if request is from one of registered remote server then only accept
 * it.
 ********************************************************************/
    for(i=0; i < shm_HXECOM->HostCnt; i++) {
		if(shm_HXECOM->Other_ips[i].s_addr == CMsg.ID.server.sock.sin_addr.s_addr)  { 
			break; 
		}
    }
	if(i == shm_HXECOM->HostCnt) { 
		printf(MSG_TEXT, "RemServer: Unsolicited packet recvd  from %s \n", inet_ntoa(CMsg.ID.server.sock.sin_addr)); 	
		com_hxfmsg(STATS, 0, HTX_HE_INFO, MSG_TEXT);
		GlobalSignal(COORD_SEM, STATS);
        return;
    }
	
/*********************************************************************
 * Check for MAX_REMOTE SERVERs 
 *********************************************************************/
	
    if(shm_COORD->NoRemotes == MAXREMOTE) {
        sprintf(MSG_TEXT, "RemServer: can't accept any more remotes. Increase size of MAXCM_REMOTE.\n");
        com_hxfmsg(STATS, HTXERROR(EX_COORD14,0), HTX_HE_SOFT_ERROR, MSG_TEXT);
        GlobalSignal(COORD_SEM, STATS);
        return;
    }
/********************************************************************/
/* New exerciser, update lists: shm_COORD->RemoteIDs, shm_COORD->RemoteAddr[]             */
/********************************************************************/
    memcpy(&shm_COORD->RemoteIDs[shm_COORD->NoRemotes++], &CMsg.ID.server, sizeof(CMsg.ID.server));
    NewRemoteCoordFlag = NewRemoteAddr(CMsg.ID.server.sock.sin_addr.s_addr, LocalAddr, resp);
    if(NewRemoteCoordFlag == -1) {
        GlobalSignal(COORD_SEM, STATS);
        Coord_exit(1);
    }

/********************************************************************/
/* Inform all initialized local servers of new remote server.       */
/********************************************************************/
/*
    sprintf(MSG_TEXT, "RemServer: CM_REMOTE SERVER @ (%s, %d)\n", inet_ntoa(CMsg.ID.server.sock.sin_addr), 
                                                    (int) ntohs(CMsg.ID.server.sock.sin_port));
    com_hxfmsg(STATS, HTXERROR(EX_COORD15,0), HTX_HE_INFO, MSG_TEXT);
*/

    CMsg.msg_type = htonl(CMsg.msg_type);
    for(i=0; i<shm_COORD->NoLocals; i++) {
        if((shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].ServerIDinit == 0) || 
                            shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].dead_flag)
            continue;
        if((rc = SendStreamMsg(shm_COORD->LocalIDs[i].sock, CMsg, resp)) < 0) {
            GlobalSignal(COORD_SEM, STATS);
            Coord_exit(-rc);
        }
    }

/********************************************************************/
/* Inform new remote server of all local initialized servers.       */
/********************************************************************/
/*
        sprintf(MSG_TEXT, "RemServer: inform new remote server \n");
        com_hxfmsg(STATS, HTXERROR(EX_COORD1,0), HTX_HE_INFO, MSG_TEXT);
*/
    Msg.msg_type = htonl(CM_REMOTESERVER);
    memset(&dest, '\0', sizeof(dest));
    dest.sin_family = AF_INET;
#ifndef __HTX_LINUX__
    dest.sin_len = sizeof(dest);
#endif
    dest.sin_port = htons(ComStreamPort);
    dest.sin_addr.s_addr = CMsg.ID.server.sock.sin_addr.s_addr;
    for(i=0; i<shm_COORD->NoLocals; i++) {
        if((shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].ServerIDinit == 0) || 
                              shm_HXECOM->exer[shm_COORD->LocalExerNo[i]].dead_flag)
            continue;
        memcpy(&Msg.ID.server, &shm_COORD->LocalIDs[i], sizeof(struct id_t));
#ifdef __DEBUG__
    printf("%d : sending local serverinfo: %x to %x \n",
           getpid(), shm_COORD->LocalIDs[i].sock.sin_addr.s_addr,
           dest.sin_addr.s_addr);
#endif
        if((rc = SendStreamMsg(dest, Msg, resp)) < 0) {
            GlobalSignal(COORD_SEM, STATS);
            Coord_exit(-rc);
        }
    }
    

/********************************************************************/
/* Replicate new remote message to all known coordinators.          */
/* Broadcast of message  is not reliable. This may be redundant     */
/* info when received but we know all identified coordinators have  */
/* the same info.  This also means that once we are on any          */
/* coordinators list we can stop broadcasting.                      */
/********************************************************************/
    if(NewRemoteCoordFlag) {
/*
        sprintf(MSG_TEXT, "RemServer: replicate to know coords\n");
        com_hxfmsg(STATS, HTXERROR(EX_COORD1,0), HTX_HE_INFO, MSG_TEXT);
*/
        memset(&dest, '\0', sizeof(dest));
        dest.sin_family = AF_INET;
#ifndef __HTX_LINUX__
        dest.sin_len = sizeof(dest);
#endif
        dest.sin_port = htons(ComStreamPort);
    
        for(i=0; i< shm_COORD->NoRemoteAddr; i++) {
            if(dest.sin_addr.s_addr == CMsg.ID.server.sock.sin_addr.s_addr)
                continue;
            dest.sin_addr.s_addr = shm_COORD->RemoteAddr[i];
#ifdef __DEBUG__
    printf("%d : sending local serverinfo: %d to %d \n",
           getpid(),CMsg.ID.server.sock.sin_addr.s_addr ,
           dest.sin_addr.s_addr);
#endif
            if((rc = SendStreamMsg(dest, CMsg, resp)) < 0) {
                GlobalSignal(COORD_SEM, STATS);
                Coord_exit(-rc);
            }
        }
    }
    GlobalSignal(COORD_SEM, STATS);
}



static void BroadcastLocals(struct sockaddr_in BroadcastDest, int ComDgramSock, struct resp_t * resp)
{
    struct CoordMsg CMsg;
    int i;

    GlobalWait(COORD_SEM, STATS);
    CMsg.msg_type = htonl(CM_REMOTESERVER);   
    for(i=0; i<shm_COORD->NoLocals; i++) {
        memcpy(&CMsg.ID.server, &shm_COORD->LocalIDs[i], sizeof(struct id_t));
        BroadcastMsg(CMsg, BroadcastDest, ComDgramSock, resp);
    }
    if(--shm_COORD->BroadcastIntervalsRemaining == 0)
        shm_COORD->BroadcastAlarm = 0;
    GlobalSignal(COORD_SEM, STATS);
}



static void RemoveRemoteAddr(uint32_t RemAddr)
{
    int i;
    int Index = -1;

    for(i=0; i<shm_COORD->NoRemoteAddr; i++)
        if(RemAddr == shm_COORD->RemoteAddr[i]) {
            Index = i;
            break;
        }
    if(Index == -1)
        return;

    shm_COORD->NoRemoteAddr--;
    for(i=Index; i < shm_COORD->NoRemoteAddr; i++)
        shm_COORD->RemoteAddr[i] = shm_COORD->RemoteAddr[i+1];
}



static int NewRemoteAddr(uint32_t RemAddr, uint32_t LocalAddr, struct resp_t * resp)
{
    int i;

    if(LocalAddr == RemAddr)
        return 0;

    for(i=0; i<shm_COORD->NoRemoteAddr; i++)
        if(RemAddr == shm_COORD->RemoteAddr[i])
            return 0;

    if(i >= MAXADDR) {
        sprintf(MSG_TEXT, " NewRemoteAddr: Maximum allowed remote addresses exceeded\n");
        com_hxfmsg(STATS, HTXERROR(EX_COORD16,0), HTX_HE_SOFT_ERROR, MSG_TEXT);
        return -1;
    }
    shm_COORD->RemoteAddr[shm_COORD->NoRemoteAddr++] = RemAddr;
    return 1;
}



static int SendStreamMsg(struct sockaddr_in dest, struct CoordMsg CMsg, struct resp_t * resp)
{
    SOCKET Sock;
    int    rc, i;
    char addrStr[30];
    long   num;
    char ConAddr[32];


    struct timeval tv;
    struct timezone tz;

    if ( !shm_HXECOM->OneSysFlag) {
       strcpy(ConAddr,InetNtoa(dest.sin_addr, addrStr, STATS));
       if(CantConnect(ConAddr,STATS) ) {
/*
			sprintf(MSG_TEXT, "Send Stream: can't connect to address %s, otherhost count = %d\n",ConAddr,shm_HXECOM->HostCnt);
			com_hxfmsg(STATS, HTXERROR(EX_RULEREAD1,ERRNO), HTX_HE_INFO, MSG_TEXT);
*/

              return 0;
       }
    }


#define NORETRIES	1
    for(i=0; i<NORETRIES; i++) {
        Sock = socket(AF_INET, SOCK_STREAM, 0);
        if(Sock == INVALID_SOCKET) {
            sprintf(MSG_TEXT, "SendStreamMsg: Error opening socket for (%s,%d)- %s.\n", 
                       InetNtoa(dest.sin_addr, addrStr, STATS) , (int) ntohs(dest.sin_port), NEW_STRERR(errno));
            com_hxfmsg(STATS, HTXERROR(EX_SOCK7,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
            return (-EX_SOCK7);
        }
        rc = connect(Sock, (struct sockaddr *) &dest, sizeof(struct sockaddr_in));

        if(rc == 0 || (ERRNO != ECONNREFUSED && ERRNO != ETIMEDOUT))
            break;
        closesocket(Sock);
        if(i < NORETRIES-1) {
            gettimeofday(&tv, &tz);
            num = (tv.tv_usec % 300 + 100) * 10000;
            sprintf(MSG_TEXT, "SendStreamMsg: Error Connecting to (%s,%d).\nRetrying... (#%d) rc=%d\n", 
                        InetNtoa(dest.sin_addr, addrStr, STATS) , (int) ntohs(dest.sin_port), i+1,rc);
            com_hxfmsg(STATS, HTXERROR(EX_COORD17,ERRNO), HTX_HE_INFO, MSG_TEXT);
            usleep(num);
        }
    }
    if(rc == SOCKET_ERROR) {
        sprintf(MSG_TEXT, 
              "SendStreamMsg: Error Connecting to (%s,%d) - %s.\n", 
                        InetNtoa(dest.sin_addr, addrStr, STATS), (int) ntohs(dest.sin_port), NEW_STRERR(errno));
        com_hxfmsg(STATS, HTXERROR(EX_COORD18,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
        closesocket(Sock);
        return (-EX_COORD18);
    }
#ifdef CDEBUG
    strcpy(temp, inet_ntoa(CMsg.ID.server.sock.sin_addr));
    sprintf(MSG_TEXT, "SendStreamMsg: (%s, %d) to (%s, %d)\n", temp, (int) ntohs(CMsg.ID.server.sock.sin_port), 
                                      inet_ntoa(dest.sin_addr), (int) ntohs(dest.sin_port));
    com_hxfmsg(STATS, HTXERROR(EX_COORD19,0), HTX_HE_INFO, MSG_TEXT);
#endif

    if((rc = StreamWrite(Sock, (char *) &CMsg, sizeof(CMsg))) < 0) {   
        sprintf(MSG_TEXT, "SendStreamMsg: error writing - %s.\n", NEW_STRERR(errno));
        com_hxfmsg(STATS, HTXERROR(EX_COORD19,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
    }
    else if(rc < sizeof(CMsg)) {
        sprintf(MSG_TEXT, "SendStreamMsg: error - Wrote only %d of %d bytes\n", rc, sizeof(CMsg));
        com_hxfmsg(STATS, HTXERROR(EX_COORD20,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
    }
    closesocket(Sock);
    return 0;
}


static int AddrRemote(uint32_t Addr, uint32_t LocalAddr)
{
    return (LocalAddr != Addr);
}



static void BroadcastMsg(struct CoordMsg CMsg, struct sockaddr_in BroadcastDest, int ComDgramSock, struct resp_t * resp)
{
    int n;
	int index;
	struct sockaddr_in TempDestAddr;
	char addrStr[32];

/*TWM in multisystem test mode don't broadcast, just use address in 
  /tmp/other_ids file to send connect requests too */
/*NOTE: dose not hurt to broadcast when doing onesystem test.. then the
  comnet is the test net, not the site ring. So broacasts just go out on
  testnet. */

	if(shm_HXECOM->OneSysFlag) {
		n = sendto(ComDgramSock, (char *) &CMsg, sizeof(CMsg), 0, (struct sockaddr *) &BroadcastDest, sizeof(BroadcastDest));
/*
		if(n == SOCKET_ERROR) {
			sprintf(MSG_TEXT, "BroadcastMsg: Couldn't Broadcast - %s.\n", NEW_STRERR(errno));
			com_hxfmsg(STATS, HTXERROR(EX_COORD21,ERRNO), HTX_HE_INFO, MSG_TEXT);
		}
*/
	} else {
		memset(&TempDestAddr, '\0', sizeof(TempDestAddr));
		TempDestAddr.sin_family = AF_INET;
#ifndef __HTX_LINUX__
		TempDestAddr.sin_len = sizeof(TempDestAddr);
#endif
		TempDestAddr.sin_port = BroadcastDest.sin_port;

		for(index=0; index < shm_HXECOM->HostCnt; index++) {
			TempDestAddr.sin_addr.s_addr = shm_HXECOM->Other_ips[index].s_addr;
			n = sendto(ComDgramSock, (char *) &CMsg, sizeof(CMsg), 0, (struct sockaddr *) &TempDestAddr, sizeof(TempDestAddr));
/*
			if(n == SOCKET_ERROR) {
				InetNtoa(shm_HXECOM->Other_ips[index], addrStr, STATS);
				sprintf(MSG_TEXT, "BroadcastMsg: Couldn't sendto for %s - %s.\n", addrStr,NEW_STRERR(errno));
				com_hxfmsg(STATS, HTXERROR(EX_COORD21,ERRNO), HTX_HE_INFO, MSG_TEXT);
			}
*/
		}
	}
}


static int cleanup_in_progress = FALSE;

static void ExitCleanup(void)
{

    if (!cleanup_in_progress) {
        cleanup_in_progress = TRUE;
        Detach_HXECOM(shm_HXECOM);
        Detach_COORD(shm_COORD);
    }
}


static void SIGTERM_hdl2(int sig, int code, struct sigcontext * scp)
{

	Coord_exit(0);
}

static void SIGINT_hdl2(int sig, int code, struct sigcontext * scp)
{
	/* hxecom a will send SIGINT. */
	Coord_exit(0);
}


static void Coord_exit(int exitNo)
{
	if(cleanup_in_progress) 
		return; 

    if(ParentPID == getpid()) 
        shutdown(ComStreamSock, 2);
    else
        closesocket(ComStreamSock);
    HE_exit(exitNo);
}


static void Detach_COORD(struct shm_coord_t * ptr)
{
    int MID;
    struct shmid_ds ShmDs;
    
    if(ptr == (struct shm_coord_t *)NULL)
        return;

    MID = ptr->mID;
    shmdt((char *)ptr);

	ptr = NULL ; 
    shmctl(MID, IPC_STAT, &ShmDs);
    if(ShmDs.shm_nattch == 0) {
         shmctl(MID, IPC_RMID, 0);
    }
}



static void SIGALRM_hdl(int sig, int code, struct sigcontext * scp)
{
   return;
}



static void SIGPIPE_hdl(int sig, int code, struct sigcontext * scp)
{
   return;
}


void HE_exit(int exit_no)
{
    if(shm_HXECOM != NULL && ParentPID == getpid())
        shm_HXECOM->exer[ExerIdx].SigChildFlag = 1;
    exit(exit_no);
}



static void Kill(pid_t PID)
{
    kill(PID, SIGTERM);
}


static void SetUpCoordShm(struct resp_t * resp)
{
/********************************************************************/
/* Setup Coordinators memory to share with children.                */
/********************************************************************/
   if((mID = shmget(IPC_PRIVATE, sizeof(struct shm_coord_t), 
                        IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | 
                                                     S_IROTH | S_IWOTH)) == -1) {
      sprintf(MSG_TEXT, "coord: Error getting shared memory 'shm_coord_t' - %s\n", NEW_STRERR(errno));
      com_hxfmsg(STATS, HTXERROR(EX_SHMGET2,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
      Coord_exit(EX_SHMGET2);
   }
   if((int)(shm_COORD = (struct shm_coord_t *)shmat(mID, (char *)0, 0)) == -1) {
      sprintf(MSG_TEXT, "coord: Error attaching shm_COORD - %s\n", NEW_STRERR(errno));
      com_hxfmsg(STATS, HTXERROR(EX_SHMAT3,ERRNO), HTX_HE_SOFT_ERROR, MSG_TEXT);
      Coord_exit(EX_SHMAT3);
   }

}



int  getBroadcastAddr( char * ipaddr, int *brdcst )
{ 

  char brdstr[25];
 char * strptr;
 char msgtxt[128];
 FILE *fd;
 char temp_string[300];

 sprintf(temp_string, "%s/out", global_exer_log_dir);
 sprintf(msgtxt, "ip addr show | awk ' /%s/ { print $4 }' >%s ", ipaddr, temp_string);
 system(msgtxt);

 fd = fopen( temp_string, "r");
 fscanf(fd, "%s", brdstr );
#ifdef __DEBUG__
 printf("ip_addr = %s, brdstr: %s\n",ipaddr, brdstr);
#endif
 fclose(fd);

	strptr = brdstr;  

 *brdcst = (int)inet_addr(strptr);
/* printf(" brdcst = 0x%x,  brdstr: %s\n",*brdcst,strptr); */
 return 0;
 
}

  
int com_hxfmsg(struct htx_data *p, int err, enum sev_code  sev, char *text)
{

  static int firsttime = 1;
  static FILE *comlogfd;
  char    disp_time[30], disp_time1[30];
  struct  tm   *asc_time, asc_time1;
  long    call_time;
  char temp_string[300];

  if (*(p->run_type) == 'O') {
     hxfmsg(p,err, sev, text);
  } else {
     if ( firsttime ) {
         sprintf(temp_string, "%s/coordstats", global_exer_log_dir);
         comlogfd = fopen(temp_string, "w");
         firsttime = 0;
     }

     call_time = time((long *) 0);
     asc_time = localtime_r(&call_time, &asc_time1);
     (void) strcpy(disp_time, asctime_r(asc_time, disp_time1));
     disp_time[24] = '\0';

      fprintf(comlogfd, "\n%-18s%-20s err=%-8.8x sev=%-1.1u %-14s\n%-s\n",
                    p->sdev_id, &disp_time[4],p->error_code,
                    p->severity_code, p->HE_name, p->msg_text);
  }

}

