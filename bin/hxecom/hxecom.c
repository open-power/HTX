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

/* @(#)98	1.27  src/htx/usr/lpp/htx/bin/hxecom/hxecom.c, exer_com, htx53A 5/24/04 17:28:35 */

/*
 *   COMPONENT_NAME: exer_com
 *
 *   FUNCTIONS: 
 *		CkPath
 *		CoordReady
 *		Detach_pHXECOM
 *		ExitCleanup
 *		GetLengthOfFile
 *		GetPath
 *		HE_exit
 *		NeedWriter
 *		PrtSyntax
 *		RemoteServer
 *		RemoveCoord
 *		ReqPattern
 *		ReqRdrId
 *		StopReader
 *		ReqRules
 *		SIGALRM_hdl
 *		SIGPIPE_hdl
 *		SIGTERM_hdl
 *		SIGINT_hdl
 *		SetUpCoord
 *		SetUpReader
 *		SetUpSemServ
 *		SetUpWriter
 *		Shutdown
 *		WSACleanUP
 *		WriteConfig
 *		main
 *		prtHostent
 *		prtSockaddrIn
 *      com_hxfmsg
 */

/************************************************************************/
/*****  I B M   I n t e r n a l   U s e   O n l y  **********************/
/************************************************************************/
/*                                                                      */
/* MODULE NAME    =    hxecom.c                                         */
/* COMPONENT NAME =    exer_com                                         */
/* LPP NAME       =    HTX                                              */
/*                                                                      */
/* DESCRIPTIVE NAME =  HTX Commo Hardware Exersiser                     */
/*                                                                      */
/* COMPILER OPTIONS =                                                   */
/*                                                                      */
/* CHANGE ACTIVITY =                                                    */
/*    DATE    |LEVEL|PROGRAMMER|DESCRIPTION                             */
/*    --------+-----+----------+----------------------------------------*/
/*    3/24/94 |1.00 | jmiles   | Initial release.                       */
/*            |     |          |                                        */
/*            |     |          |                                        */
/************************************************************************/
#ifndef __HTX_LINUX__
#    include <sys/cred.h>
#endif
#    include <stdlib.h>
#    include <string.h>
#    include <unistd.h>
#    include <fcntl.h>
#    include <ctype.h>
#    include <memory.h>
#    include <time.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <sys/socket.h>
#    include <netinet/in.h>
#    include <netdb.h>
#    include <arpa/inet.h>
#    include <sys/wait.h>
#    include <sys/shm.h>
#    include "hxecomdef.h"

/******************************************************************************/
/***  Global Variable Definitions  ********************************************/
/******************************************************************************/

struct htx_data htx_ds;
int in_wait_loop=0;

char global_htx_log_dir[256] = "/tmp";
char global_htx_home_dir[256] = {'\0'};

struct shm_phxecom_t  * shm_pHXECOM = 0;

struct shm_hxecom_t   * shm_HXECOM  = NULL;
struct shm_Private_t  * shm_Private = NULL;

static struct sockaddr_in   coordSock;
static struct sockaddr_in * coordSockPtr = &coordSock;
static SOCKET ServerSock = INVALID_SOCKET;

/******************************************************************************/
/***  End of Global Variable Definitions  *************************************/
/******************************************************************************/

int com_hxfmsg(struct htx_data *p, int err, enum sev_code  sev, char *text);
static void prtHostent(struct hostent * host);
static void prtSockaddrIn(char * name, struct sockaddr_in data);
static void Shutdown(struct CoordMsg CMsg, struct sockaddr_in * Coord);
static void ExitCleanup(void);
static void SetUpWriter(struct htx_data * Stats, char * TestNetName, int layer, struct id_t * ServerID,
                         SOCKET ServerSock, int exer_idx, SOCKET msgsock);
static void RemoteServer(struct htx_data * Stats, char * TestNetName, struct id_t * server, int layer, int exer_idx,
                              SOCKET ServerSock, SOCKET msgsock);
static void SetUpReader(struct htx_data * Stats, char TestInetName[], struct id_t * SemServID, 
                     struct id_t * ReaderID, int exer_idx, int WriteIdx, struct id_t * WriterTestID, int layer,
                      SOCKET ServerSock, SOCKET msgsock);
static void ReqRdrId(struct htx_data * Stats, struct id_t * SemServerID, char *TestNetName, int layer,SOCKET ServerSock, int exer_idx, SOCKET msgsock);
static void StopReader(struct htx_data * Stats, struct id_t * RemServerID, char *TestNetName, int layer,SOCKET ServerSock, SOCKET msgsock);
static void SetUpSemServ(struct htx_data * Stats, char * ComInetName, unsigned int seed, unsigned int layer, struct id_t * SemServID);
static void SetUpCoord(struct rule_format *rules, int exer_idx);
static void ReqPattern(char *pattern, int pattern_len, int max_size, SOCKET msgsock);
static void ReqRules(char rules_path[], uint32_t size, SOCKET msgsock);
static void RemoveCoord(void);
static void SetUpOtherHosts(char *ThisComName);
static void Detach_pHXECOM(struct shm_phxecom_t * ptr);
static int  CoordReady(u_short ComStreamPort);
static int  CkPath(const char path[], char * err_msg_text);
static void PrtSyntax(char * ProgramName);
static int  GetPath(char path[], const char pathin[], int len, const char envirVar[], const char defaultPath[]);
static void WriteConfig(char * remoteIP);

static void SIGALRM_hdl(int sig, int code, struct sigcontext * scp);
static void SIGPIPE_hdl(int sig, int code, struct sigcontext * scp);
static void SIGINT_hdl(int sig, int code, struct sigcontext * scp);
static void SIGTERM_hdl(int sig, int code, struct sigcontext * scp);
static void WSACleanUP(void);
static int GetLengthOfFile(char * path);
static int NeedWriter(u_short RemoteTransact, u_short LocalTransact);

/************************************************************************/
/*****  m a i n ( )  ****************************************************/
/************************************************************************/
/*                                                                      */
/* FUNCTION NAME =     main()                                           */
/*                                                                      */
/* DESCRIPTIVE NAME =  The main commo hardware exerciser function.      */
/*                                                                      */
/* FUNCTION =          Initiates commo HW identified on the network.    */
/*                                                                      */
/* INPUT =             Arguments at invocation, messages from other     */
/*                     exercisers.                                      */
/*                                                                      */
/* OUTPUT =            Spawned test processes.                          */
/*                                                                      */
/* NORMAL RETURN =     0 to indicate successful completion              */
/*                                                                      */
/* ERROR RETURNS =     see file "exit.h"                                */
/*                                                                      */
/* EXTERNAL REFERENCES                                                  */
/*                                                                      */
/*    OTHER ROUTINES = hxfupdate() - communicates with HTX supervisor.  */
/*                                                                      */
/*                     hxsmsg() - updates data for supervisr and sends a*/
/*                                message to the message handler program*/
/*                                (hxsmsg).                             */
/*                                                                      */
/************************************************************************/
	char tmpaddr[30];

int  main(int argc, char *argv[])
{
	int index=0;
	int lent=0;
    char            error;      /* rule stanza error flag               */
    char pattern_path[FNAME_MAX];/* path/pattern_file_name              */
    char rules_path[FNAME_MAX]; /* path/rules_file_name                 */
    int       line_number;      /* cur line # in rules file             */
    int                rc;      /* return code                          */
    int                 i;      /* temp loop variable                   */
    int                 ti;      /* temp loop variable                   */
    int                 tit;      /* temp loop variable                   */
    int                 savit;      /* temp loop variable                   */
    SOCKET         ToLocalCoordSock;
    struct rule_format rules;
    struct id_t    SemServID;
    struct CoordMsg     CMsg;
    int    exer_idx;
    struct sockaddr_in src;
    char         * pattern;
    int            pattern_len;
    SOCKET         msgsock;
    FILE         * rfd;
    char           msg_text[MAX_TEXT_MSG];
    int            ExecFlag;
    socklen_t         length;
    int            mpID;
    int            c;           /* input option character               */
    pid_t          fPID;
    struct sigaction sigvector; /* structure for signals                */
	char           addrStr[32];
	struct htx_data * stats = (struct htx_data *)&htx_ds;

	int   deff;
	char *temp_env_val_ptr = NULL;

	temp_env_val_ptr = getenv("HTX_HOME_DIR");
	if( (temp_env_val_ptr != NULL) && (strlen(temp_env_val_ptr) > 0) ) {
		strcpy(global_htx_home_dir, temp_env_val_ptr);
	} else {
		printf("HTX environment is not setup, please setup HTX environment and re-tr\ny");
		printf("to setup HTX environment on same shell execute: . htx_setup.sh\n");
		printf("exiting...\n\n");
		exit(11);
	}

	temp_env_val_ptr = getenv("HTX_LOG_DIR");
	if( (temp_env_val_ptr != NULL) && (strlen(temp_env_val_ptr) > 0) ) {
		strcpy(global_htx_log_dir, temp_env_val_ptr);
	}

/************************************************************************/
/* Get/test for valid option flags.                                     */
/************************************************************************/
    errno    = 0;
    ExecFlag = 0;
    while((int)(c = getopt(argc, argv, "e")) != EOF) {
        switch(c) {
            case 'e':
                ExecFlag = 1;
                break;
            case '?':
                PrtSyntax(argv[0]);
                HE_exit(EX_INV_ARGV);
        }
    }


/********************************************************************/
/* Test for correct number of arguments.                            */
/********************************************************************/
    if(argc - optind != 3) {
        PrtSyntax(argv[0]);
        HE_exit(EX_INV_ARGV);
    }

/********************************************************************/
/* Copy arguments.                                                  */
/********************************************************************/
    strncpy(stats->HE_name, argv[0], 32);
    stats->HE_name[31] = '\0';
    strncpy(stats->sdev_id, argv[optind], 32);
    stats->sdev_id[31] = '\0';
    strncpy(stats->run_type, argv[optind+1], 4);
    stats->run_type[3] = '\0';
    i = 0;
	 
    while(*(stats->run_type+i) != '\0') {
        *(stats->run_type+i) = toupper(*(stats->run_type+i));
        i++;
    }
	 
/********************************************************************/
/*** FOR STANDALONE MODE -- must fork so we can be a session leader.*/
/*** This process can't receive signals without forking.  This also */
/*** has a good side effect.   Process is automatically put in      */
/*** the background.                                                */
/*** For a threaded environment, a fork must be followed by an exec */
/********************************************************************/
    if(strcmp(stats->run_type, "OTH") == 0 && ExecFlag == 0) {
        fPID = fork();
        switch(fPID) {
            case 0:
				printf("Child with args:argv[0]:%s, argv[optind]:%s argv[optind+1]:%s argv[optind+2] :%s\n",
				argv[0], argv[optind], argv[optind+1], argv[optind+2]);

                execlp(argv[0], argv[0], "-e", argv[optind], argv[optind+1], argv[optind+2], 0);
                fprintf(stderr, "%s: exec error in standalone mode - %s.\n", argv[0], STRERROR(errno));
                exit(EX_EXEC5);
                break;
            case -1:
                fprintf(stderr, "%s: fork error in standalone mode - %s.\n", argv[0], STRERROR(errno));
                exit(EX_FORK0);
                break;
            default:
                exit(0);
        }
    }


    atexit(ExitCleanup);   

/********************************************************************/
/* Call hxfupdate() to initialize htx_data structure and to         */
/* initialize the hxfupdate procedure.                              */
/********************************************************************/
   hxfupdate(START, stats);

/************************************************************************/
/* Allocate shared memory used by this exerciser and its children.      */
/************************************************************************/

    if((mpID = shmget(IPC_PRIVATE, sizeof(struct shm_phxecom_t), 
                      IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | 
                                                   S_IROTH | S_IWOTH)) == -1) {
        sprintf(msg_text, "%s: Unable to create shm_Private - %s", argv[0], STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_SHMGET1,ERRNO), HTX_HE_HARD_ERROR, msg_text);
        HE_exit(EX_SHMGET1);
    }
    if((int)(shm_pHXECOM = (struct shm_phxecom_t *) shmat(mpID, (char *)0, 0)) ==  -1) {
        sprintf(msg_text, "%s: Unable to attach to shm_Private - %s", argv[0], STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_SHMAT1,ERRNO), HTX_HE_HARD_ERROR, msg_text);
        HE_exit(EX_SHMAT1);
    }
    shm_pHXECOM->mID = mpID;
    shm_pHXECOM->BrokenPipes = 0;
    shm_pHXECOM->starttime = time(NULL);
	/* see if we are running bootme and set flag. */
	sprintf(msg_text,"%s/etc/scripts/pscheck bootme", global_htx_home_dir);
    shm_pHXECOM->bootme = system(msg_text);


/********************************************************************/
/* Check for valid run_type argument.                               */
/********************************************************************/
    if( strcmp(stats->run_type, "OTH") && 
        strcmp(stats->run_type, "REG") &&
        strcmp(stats->run_type, "EMC") ) {
        PrtSyntax(argv[0]);
        HE_exit(EX_INV_ARGV);
    }   

/********************************************************************/
/* Check rules path for validity.                                   */
/********************************************************************/
    if(GetPath(rules_path, argv[argc -1], FNAME_MAX, "HTXRULES", 
           (stats->run_type[0] == 'E') ? "../rules/emc/hxecom" : RULE_PATH) == 0)
        HE_exit(EX_FNAME);
 
 

/********************************************************************/
/* Write ID information for this process.                           */
/********************************************************************/
   sprintf(msg_text, "%s: %s %s %s \n", argv[0], 
		  stats->sdev_id, stats->run_type, rules_path);
   hxfmsg(stats, HTXERROR(HEINFO1,0), HTX_HE_INFO, &(msg_text[0]));

/********************************************************************/
/*   Set up SIGTERM, SIGINT asynchronous signals                    */
/*          SIGALRM, SIGPIPE synchronous signals                    */
/*   All asynchronous signals should be handled by this parent      */
/*   thread.  All other threads must set up signal mask to block.   */
/*   All synchronous signals are setup in this thread but are       */
/*   handled on a per thread basis.                                 */
/*   SIGWAITING added as 9404 workaround.                           */
/*                                                                  */
/*   Block signals while servicing interrupt.                       */
/*   Do not restart system calls.                                   */
/********************************************************************/
    sigemptyset(&(sigvector.sa_mask));
    sigvector.sa_flags = 0;

    sigvector.sa_handler = (void(*)(int))SIGTERM_hdl;
    sigaction(SIGTERM, &sigvector,  NULL);

    sigvector.sa_handler = (void(*)(int))SIGINT_hdl;
    sigaction(SIGINT, &sigvector,  NULL);

    sigvector.sa_handler = (void(*)(int))SIGALRM_hdl;
    sigaction(SIGALRM, &sigvector, NULL);

    sigvector.sa_handler = (void(*)(int))SIGPIPE_hdl;
    sigaction(SIGPIPE, &sigvector, NULL);

    sigvector.sa_handler = SIG_IGN;
    sigaction(SIGQUIT, &sigvector, NULL);
    sigaction(SIGHUP,  &sigvector, NULL);
    sigaction(SIGCLD,  &sigvector, NULL);

/********************************************************************/
/* DefineShm guarantees that shared memory is ready to be           */
/* used.  All flags have been zeroed.  All global semaphores are    */
/* available.  All Master processs initializations have been        */
/* performed.  Returned variable "exer_idx" indexes our             */
/* structure.  This variable must be passed to functions that       */
/* reference shared memory.                                         */
/********************************************************************/
    if(gethostname((char *)msg_text, HXECOM_HOSTNAMELEN)){
             strcpy(msg_text, "NO_HOSTNAME");
     }
    exer_idx = DefineShm(msg_text);
	printf(" HXECOM_HOSTNAME = %s, exer_idx = %d \n", msg_text, exer_idx); 

/* shm_pHXECOM->ExerNo is exer_idx */

/********************************************************************/
/* Open rules file for reading.                                     */
/********************************************************************/
    if ((rfd = fopen(rules_path, "r")) ==  NULL) {
        sprintf(msg_text, "%s: Error opening rules file - %s \n", argv[0], STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_RULEREAD1,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_RULEREAD1);
    }

/********************************************************************/
/* Check each rule stanza for validity.                             */
/********************************************************************/
    error = 'n';
    line_number = 0;
    while ((rc = get_rule(rfd, &line_number, &rules)) != EOF) {
        if (rc == 1 || GetPath(pattern_path, rules.pattern_id, FNAME_MAX, "HTXPATTERNS", PATTERN_PATH) == 0)
	    error = 'y';
    }				
    if (error == 'y') {
        HE_exit(EX_RULEREAD2);
    }

    fclose(rfd);


/********************************************************************/
/* Get stats on pattern file.                                       */
/********************************************************************/
    pattern_len = GetLengthOfFile(pattern_path);

/********************************************************************/
/* Allocate space for pattern file.                                 */
/********************************************************************/
   if((pattern = (char *) malloc(pattern_len * sizeof(char))) == NULL) {
        sprintf(msg_text, "%s: Malloc of pattern space failed - %s\n", argv[0], STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC1,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC1);
    }

    errno = 0;
    rc = hxfpat(pattern_path, pattern, (unsigned int)pattern_len);
    if (rc == 1) {
        sprintf(msg_text, "%s: open error %s - %s\n",argv[0], 
			   pattern_path, STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_PAT2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_PAT2);
    } else if (rc == 2) {
        sprintf(msg_text, "%s: read error %s - %s\n",argv[0], 
			   pattern_path, STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_PAT3,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_PAT3);
    }

/********************************************************************/
/* Setup Coordinator.                                               */
/********************************************************************/
	/* since shm_HXECOM is shared by all hxecoms on this one host */
	/* there is no need to do thie setupotherhosts routine but once */
	
	printf("[%s]:%d, Setting Up Coordinator. \n", __FUNCTION__, __LINE__); 
	
	GlobalWait(COORD_SEM, stats);
    if(rules.onesys == 1) {
      shm_HXECOM->OneSysFlag=1;
	} else {
      shm_HXECOM->OneSysFlag=0;
	  if(shm_HXECOM->HostCnt==0) {
		  SetUpOtherHosts(rules.ComName);
	  }
	}
	GlobalSignal(COORD_SEM, stats);
 

    SetUpCoord(&rules, exer_idx);

    SetUpSemServ(stats, rules.ComName, rules.bufseed, rules.layer, &SemServID);
    SaveSemServID(SemServID);

/********************************************************************/
/* Setup Server socket.                                             */
/********************************************************************/
    ServerSock = socket(AF_INET, SOCK_STREAM, 0);
    if(ServerSock == INVALID_SOCKET) {
        sprintf(msg_text, "%s: Error opening server stream socket - %s.\n", argv[0], STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_SOCK1,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_SOCK1);
    }

/*
   sprintf(msg_text, "%s:\n", rules.TestNetName);
   hxfmsg(stats, HTXERROR(HEINFO1,0), HTX_HE_INFO, &(msg_text[0]));
*/

/********************************************************************/
/* Setup to receive on private port.                                */
/********************************************************************/
    memset(&src, '\0', sizeof(src));
    sprintf(msg_text, "%s: ", argv[0]);
    if(GetIP(rules.ComName, &src.sin_addr, msg_text+strlen(msg_text))) {    
        hxfmsg(stats, HTXERROR(EX_GETH1,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_GETH1);
    }
    src.sin_family = AF_INET;
#ifndef __HTX_LINUX__
    src.sin_len = sizeof(src);
#endif
    src.sin_port = htons(0);

    if(bind(ServerSock, (struct sockaddr *) &src, sizeof(src)) ) {
        sprintf(msg_text, "%s: Error binding server stream socket - %s.\n", argv[0], STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_BIND1,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_BIND1);
    }

/********************************************************************/
/* Put socket info in shared memory structure.                      */
/********************************************************************/
    length = sizeof(src);
    if(getsockname(ServerSock, (struct sockaddr *) &src, &length)) {
        sprintf(msg_text, "%s: Error getsockname() - %s.\n", argv[0], STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_GETS2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_GETS2);
    }
    memset(&CMsg, '\0', sizeof(CMsg));
    CMsg.ID.server.nettype = rules.nettype;
    CMsg.ID.server.transact = rules.transact;
    CMsg.ID.server.replicates = rules.replicates;
    CMsg.ID.server.ExerVersion = EXER_VERSION;
    CMsg.ID.server.sock.sin_addr.s_addr = src.sin_addr.s_addr;
    CMsg.ID.server.sock.sin_family = AF_INET;
#ifndef __HTX_LINUX__
    CMsg.ID.server.sock.sin_len = sizeof(struct sockaddr_in);
#endif
    CMsg.ID.server.sock.sin_port = src.sin_port;
	/* 
    SaveSockID(CMsg.ID.server);
	*/ 
	bcopy((char *)&CMsg.ID.server, (char *) &shm_HXECOM->exer[shm_pHXECOM->ExerNo].ServerID, sizeof(struct id_t));
    CMsg.msg_type = CM_LOCALSERVER;

/********************************************************************/
/* Update CONFIG_FILE with ComNet IP address.                       */
/********************************************************************/
    WriteConfig(InetNtoa(src.sin_addr, msg_text, stats));

/********************************************************************/
/* Since reader and server use same comport, src.sin_addr           */
/* contains address that is needed.                                 */
/********************************************************************/
    memset(coordSockPtr, '\0', sizeof(*coordSockPtr));
    coordSockPtr->sin_family      = AF_INET;
    coordSockPtr->sin_addr.s_addr = src.sin_addr.s_addr;
#ifndef __HTX_LINUX__
    coordSockPtr->sin_len         = sizeof(*coordSockPtr);
#endif
    coordSockPtr->sin_port        = htons(rules.ComPortStream);
    ToLocalCoordSock              = SetUpConnect(coordSockPtr, stats, 0);

/********************************************************************/
/* Send server info to coordinator.                                 */
/********************************************************************/
    CMsg.msg_type = htonl(CMsg.msg_type);
    HostToNetId_t(&CMsg.ID.server);
    if((rc = StreamWrite(ToLocalCoordSock, (char *) &CMsg, sizeof(CMsg))) < 0) {
        sprintf(msg_text, "%s: Server to Coordinator error - %s.\n", argv[0], STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_SERV1,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_SERV1);
    }
    else if(rc < sizeof(CMsg)) {
        sprintf(msg_text, "%s: Server to Coordinator error - %s\nWrote only %d of %d bytes\n", 
                                               argv[0], STRERROR(errno), rc, sizeof(CMsg));
        hxfmsg(stats, HTXERROR(EX_SERV2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_SERV2);
    }
    closesocket(ToLocalCoordSock);

/********************************************************************/
/* Become active server.                                            */
/********************************************************************/
    listen(ServerSock, MAXREMOTE);
    do {
		in_wait_loop=1;
        if(!shm_pHXECOM->SigTermFlag) 
            msgsock = accept(ServerSock, 0, 0);
        if(shm_pHXECOM->SigTermFlag) {
            struct id_t ID;
            SOCKET SemServSock;
			
            sprintf(msg_text, "%s: Initiating shutdown due to SIGTERM. pid %d\n", argv[0],shm_HXECOM->exer[shm_pHXECOM->ExerNo].EXER_PID);
            hxfmsg(stats, HTXERROR(HEINFO2,0), HTX_HE_INFO, msg_text);

			/*this should kill all the children that have not shutdown by now.*/
			/*It needs the process id of this hxecom so that it does not kill */
    		/* this parent */
			sprintf(msg_text,"%s/etc/scripts/pscheck %d",global_htx_home_dir, getpid());
			system(msg_text);
            HE_exit(0);
        }
        if(msgsock == -1) {
            sprintf(msg_text, "%s: Error accepting message - %s.\n", argv[0], STRERROR(errno));
            hxfmsg(stats, HTXERROR(HEINFO6,0), HTX_HE_SOFT_ERROR, msg_text);
            continue;
        }
        memset(&CMsg, '\0', sizeof(CMsg));
        if((rc = StreamRead(msgsock, (char *) &CMsg, sizeof(CMsg))) == 0) {
            closesocket(msgsock);
            continue;
        }
        else if(rc == -1) {
            sprintf(msg_text, "%s: Error reading message - %s.\n", argv[0], STRERROR(errno));
            hxfmsg(stats, HTXERROR(HEINFO7,0), HTX_HE_SOFT_ERROR, msg_text);
            continue;
        }
        CMsg.msg_type = ntohl(CMsg.msg_type);
        switch(CMsg.msg_type) {
            case CM_REQ_RDR_ID:
                FixSockAddr(&CMsg.ID.server.sock);
                NetToHostId_t(&CMsg.ID.server);
                ReqRdrId(stats, &CMsg.ID.server, rules.TestNetName, rules.layer, ServerSock, exer_idx, msgsock);
                break;
            case CM_STOP_READER:
                FixSockAddr(&CMsg.ID.server.sock);
                NetToHostId_t(&CMsg.ID.server);
                StopReader(stats, &CMsg.ID.server, rules.TestNetName, rules.layer, ServerSock, msgsock);
                break;
            case CM_START_WRITER:
                FixSockAddr(&CMsg.ID.server.sock);
                NetToHostId_t(&CMsg.ID.server);
				SetUpWriter(stats, rules.TestNetName, rules.layer, &CMsg.ID.server, ServerSock, exer_idx, msgsock);
                break;
            case CM_LOCALSERVER:
				if(shm_HXECOM->OneSysFlag==1) {
					FixSockAddr(&CMsg.ID.server.sock);
					NetToHostId_t(&CMsg.ID.server);
					if(CMsg.ID.server.ExerVersion != (u_short)EXER_VERSION) {
						shm_pHXECOM->SigTermFlag = 1;
						sprintf(msg_text, "%s: Received request from incompatible version of HXECOM.\nShutting down.\n", argv[0]);
						hxfmsg(stats, HTXERROR(HEINFO8,0), HTX_HE_HARD_ERROR, msg_text);
						break;
					}
					if(NeedWriter(CMsg.ID.server.transact, rules.transact)) {
						RemoteServer(stats,  rules.TestNetName, &CMsg.ID.server, rules.layer, exer_idx, ServerSock, msgsock);
					}
				}
                break;
            case CM_REMOTESERVER:
				if(shm_HXECOM->OneSysFlag != 1) {
					FixSockAddr(&CMsg.ID.server.sock);

					NetToHostId_t(&CMsg.ID.server);
					if(CMsg.ID.server.ExerVersion != (u_short)EXER_VERSION) {
						shm_pHXECOM->SigTermFlag = 1;
						sprintf(msg_text, "%s: Received request from incompatible version of HXECOM.\nShutting down.\n", argv[0]);
						hxfmsg(stats, HTXERROR(HEINFO8,0), HTX_HE_HARD_ERROR, msg_text);
						break;	
					}
					if(NeedWriter(CMsg.ID.server.transact, rules.transact)) {
						RemoteServer(stats,  rules.TestNetName, &CMsg.ID.server, rules.layer, exer_idx, ServerSock, msgsock);
					}
				}
        		        break;
            case CM_REQ_RULES:
                CMsg.ID.Wsize.size = ntohl(CMsg.ID.Wsize.size);
                ReqRules(rules_path, CMsg.ID.Wsize.size, msgsock);
                break;
            case CM_REQ_PATTERN:
                CMsg.ID.Wsize.size = ntohl(CMsg.ID.Wsize.size);
                ReqPattern(pattern, pattern_len, CMsg.ID.Wsize.size, msgsock);
                break;
            case CM_SHUTDOWN:
            case CM_BROADCAST_SHUTDOWN:
                Shutdown(CMsg, coordSockPtr);
                break;
            default:
                sprintf(msg_text, "%s: Received unknown message type - %d.\n", argv[0], (int)CMsg.msg_type);
                hxfmsg(stats, HTXERROR(HEINFO8,0), HTX_HE_SOFT_ERROR, msg_text);
                break;
        }
        closesocket(msgsock);
    }while(1);
    return 0;
} /* End of main()  */



static int NeedWriter(u_short RemoteTransact, u_short LocalTransact)
{
/********************************************************************/
/* If neither the local or the remote is a master then a connection */
/* can't be formed.                                                 */
/********************************************************************/
    if( ((int)RemoteTransact & MASTER_VAL) == 0 && ((int)LocalTransact & MASTER_VAL) == 0 )
        return 0;
/********************************************************************/
/* A connection can be formed if local is a writer and remote is a  */
/* reader.  If the local is a reader and remote is a writer, then   */
/* a connection will be initiated from the remote node.             */
/********************************************************************/
    else if((int)RemoteTransact & READ_VAL && (int)LocalTransact & WRITE_VAL)
        return 1;
    else
        return 0;
}


static void StopReader(struct htx_data * Stats, struct id_t * ServerID, char * TestNetName, int layer,
                      SOCKET ServerSock, SOCKET msgsock)
{
	/* This routine will get the ReaderID from the other side and then */
	/* Stop that reader, kill -9. Then do nothing else. The restart    */
	/* is initiated by the other side (the writer side).               */
    struct id_t ReaderID;
    struct CoordMsg CMsg;
    struct id_t ReaderTestID;
    struct id_t RemServerID;
	SOCKET ToServerSock;
	int rc;

    
    StreamRead(msgsock, (char *) &ReaderTestID, sizeof(struct id_t));
    StreamRead(msgsock, (char *) &RemServerID, sizeof(struct id_t));
/********************************************************************/
/* CleanUp received struct sockaddr_in structure.  If a message     */
/* is from a remote host, it may not be AIX.                        */
/********************************************************************/
    FixSockAddr(&ReaderTestID.sock);
    NetToHostId_t(&ReaderTestID);

	sprintf(Stats->msg_text,"%s/etc/scripts/pscheck %d check", global_htx_home_dir, ReaderTestID.the_pid);
	rc=system(Stats->msg_text);
	if(rc) {
		/* stop the reader */
		sprintf(Stats->msg_text, "kill Reader with pid %d, with SIGINT\n",ReaderTestID.the_pid);
		hxfmsg(Stats, HTXERROR(0,0), HTX_HE_INFO, Stats->msg_text);  
		kill(ReaderTestID.the_pid,SIGINT);
	}

    /* the writer that call for this restart from comwrite.c Req_StopReader()
        is waiting for the CM_READER_STOPPED msg so it can close the socket
        so send it
    */

    CMsg.msg_type = htonl(CM_READER_STOPPED);
    memcpy(&CMsg.ID.server, &ReaderID, sizeof(struct id_t));
    HostToNetId_t(&CMsg.ID.server);
    StreamWrite(msgsock, (char *) &CMsg, sizeof(CMsg));

	/*now I have to connect to the remoter server socket to send the */
	/* CM_START_WRITER request */

    FixSockAddr(&RemServerID.sock);
    NetToHostId_t(&RemServerID);

    /*twm added sleep 60 to give the eeh events time to be over before starting
        the new writer for the one that just detected broken pipe */
    sleep(60);

	ToServerSock = SetUpConnect(&RemServerID.sock, Stats, 0); 

    memset(&CMsg, '\0', sizeof(CMsg));
    CMsg.msg_type = htonl(CM_START_WRITER);
    memcpy(&CMsg.ID.server, &shm_HXECOM->exer[shm_pHXECOM->ExerNo].ServerID, sizeof(struct id_t));
    HostToNetId_t(&CMsg.ID.server);
    rc = StreamWrite(ToServerSock, (char *) &CMsg, sizeof(CMsg));
    if(rc == -1) {
        sprintf(Stats->msg_text, "StopReader: Error sending CM_START_WRITER - %s\n", STRERROR(errno));
        hxfmsg(Stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, Stats->msg_text);
        HE_exit(EX_WRITE5);
    }
	closesocket(ToServerSock);
}


static void ReqRdrId(struct htx_data * Stats, struct id_t * SemServerID, char * TestNetName, int layer,
                      SOCKET ServerSock, int exer_idx, SOCKET msgsock)
{
    struct id_t ReaderID;
    struct CoordMsg CMsg;
    u_long WriteIdx;
    struct id_t WriterTestID;
    
    /* SemServerID & WRITE_AHEAD index info is piggybacked with request.  */
    /* reader and writer processes must have the same unique number.      */
	/* Writer sends me WriteIdx of size u_long */ 
    StreamRead(msgsock, (char *) &WriteIdx, sizeof(u_long));
    WriteIdx = ntohl(WriteIdx);
	printf("%s:[%#d] WriterIDx Recvd = %d \n", __FUNCTION__, __LINE__, WriteIdx); 
	memset(&WriterTestID, 0x00, sizeof(struct id_t));
    StreamRead(msgsock, (char *) &WriterTestID, sizeof(struct id_t));

/********************************************************************/
/* CleanUp received struct sockaddr_in structure.  If a message     */
/* is from a remote host, it may not be AIX.                        */
/********************************************************************/
	printf("%s:%d Writer Test Sock = %s \n", __FUNCTION__, __LINE__, inet_ntoa(WriterTestID.sock.sin_addr)); 
    FixSockAddr(&WriterTestID.sock);

/* NOTE: you don't need to check for can't connect here.. the send message
	     from the other side will never be sent as long as the other side
		 has the new code on it that matches this code... */

	printf("%s:%d Writer Test Sock = %s \n", __FUNCTION__, __LINE__, inet_ntoa(WriterTestID.sock.sin_addr)); 
    NetToHostId_t(&WriterTestID);
 	memset(&ReaderID, 0xBB, sizeof(struct id_t));

	printf("%s:%d Writer Test Sock = %s \n", __FUNCTION__, __LINE__, inet_ntoa(WriterTestID.sock.sin_addr)); 
    SetUpReader(Stats, TestNetName, SemServerID, &ReaderID, exer_idx, WriteIdx, &WriterTestID, layer, ServerSock, msgsock);
    CMsg.msg_type = htonl(CM_READER_ID);
    memcpy(&CMsg.ID.server, &ReaderID, sizeof(struct id_t));
    HostToNetId_t(&CMsg.ID.server);
    StreamWrite(msgsock, (char *) &CMsg, sizeof(CMsg));
    StreamWrite(msgsock, (char *) &WriteIdx, sizeof(WriteIdx));
}



static void ReqPattern(char *pattern, int pattern_len, int max_size, SOCKET msgsock)
{
    struct CoordMsg CMsg;
    int size;

    size = (pattern_len > max_size) ? max_size : pattern_len;
    CMsg.msg_type = htonl(CM_PATTERN);
    CMsg.ID.Wsize.size  = htonl(size);
    StreamWrite(msgsock, (char *) &CMsg, sizeof(CMsg));
    StreamWrite(msgsock, (char *) pattern, size);   
}



static void ReqRules(char rules_path[], uint32_t size, SOCKET msgsock)
{
    struct rule_format rules;
    int line_number = 0, rc=0 ;
    struct CoordMsg CMsg;
    FILE * rfd;
	struct htx_data * stats = (struct htx_data *)&htx_ds;
  	char msg_text[1024];  

    if(size != (u_int32_t)sizeof(rules)) {
        CMsg.msg_type = htonl(CM_RULES_ERROR);
        CMsg.ID.Wsize.size  = htonl((uint32_t)sizeof(rules));
        StreamWrite(msgsock, (char *) &CMsg, sizeof(CMsg));
        sprintf(msg_text, "ReqRules: size error - request %lu bytes, s/b %d bytes.\n", size, (int)sizeof(rules));
        hxfmsg(stats, HTXERROR(EX_RULEREAD5,0), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_RULEREAD5);
    }

/********************************************************************/
/* Open rules file for reading.                                     */
/********************************************************************/
    if ((rfd = fopen(rules_path, "r")) ==  NULL) {
        sprintf(msg_text, "ReqRules: Error opening rules file - %s \n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_RULEREAD3,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_RULEREAD3);
    }

    CMsg.msg_type = htonl(CM_RULES_STANZA);
    CMsg.ID.Wsize.size  = htonl((uint32_t)sizeof(rules));
#ifdef __DEBUG__
	sprintf(msg_text, "Sending CMsg.msg_type = %#x to originator of size = %#x, pid = %d \n", CMsg.msg_type, sizeof(CMsg), getpid()); 
	hxfmsg(stats, 0, 7, msg_text); 
#endif 
    StreamWrite(msgsock, (char *) &CMsg, sizeof(CMsg));

    while( (rc = get_rule(rfd, &line_number, &rules)) != EOF ) {  
        HostToNetRules(&rules);
#ifdef __DEBUG__
	sprintf(msg_text, "Sending.. Rules CMsg.msg_type = %#x to originator of size = %#x, line_number=%d, rc = %d \n", CMsg.msg_type, sizeof(CMsg.msg_type), line_number, rc); 
	hxfmsg(stats, 0, 7, msg_text); 
#endif 

		memset(&CMsg, 0, sizeof(CMsg));
		CMsg.msg_type = htonl(CM_RULES_STANZA);
   		CMsg.ID.Wsize.size  = htonl((uint32_t)sizeof(rules));

        StreamWrite(msgsock, (char *) &CMsg.msg_type, sizeof(CMsg.msg_type));
        StreamWrite(msgsock, (char *) &rules, sizeof(rules));
    }  
    CMsg.msg_type = htonl(CM_RULES_FINISHED);
    StreamWrite(msgsock, (char *) &CMsg.msg_type, sizeof(CMsg.msg_type));

    fclose(rfd);
}



static void RemoteServer(struct htx_data * Stats, char * TestNetName, struct id_t * server, int layer, int exer_idx, 
                              SOCKET ServerSock, SOCKET msgsock)
{
    int i;

	char addrStr[30];
	struct sockaddr_in temp;
	char ConAddr[32];

	memcpy(&temp, &server->sock, sizeof(struct sockaddr_in));

        if (!shm_HXECOM->OneSysFlag ) {
			strcpy(ConAddr,InetNtoa(temp.sin_addr, addrStr, Stats));
		   if(CantConnect(ConAddr,Stats) ) {
/*
			sprintf(msg_text, "Writers Requested: Can't connect %s\n",ConAddr);
			hxfmsg(Stats, HTXERROR(HEINFO8,0), HTX_HE_INFO, msg_text);
*/
		
			return;
		   }
        }

    for(i=0; i < server->replicates; i++) {
        SetUpWriter(Stats, TestNetName, layer, server, ServerSock, exer_idx,  msgsock);
	}
}



static void prtHostent(struct hostent *host)
{
    struct in_addr addr;
    char addrStr[30];
	struct htx_data * stats = (struct htx_data *)&htx_ds;
    memcpy(&addr, host->h_addr, host->h_length);
    printf("name='%s'\n", host->h_name);
    printf("addr='%s'\n", InetNtoa(addr, addrStr, stats));
}


static void prtSockaddrIn(char * name, struct sockaddr_in data)
{
    char addrStr[30];
	struct htx_data * stats = (struct htx_data *)&htx_ds;
    printf("%s Socket has port #%d\n", name, (int) ntohs(data.sin_port));
    printf("%s Socket local address is %s\n", name, InetNtoa(data.sin_addr, addrStr, stats));

}


static void SetUpCoord(struct rule_format *rules, int exer_idx)
{
    char    	CoordStreamPortStr[6];
    char    	CoordDgramPortStr[6];
    char	BroadcastIntStr[6];
    char	BroadcastIntRemainStr[6];
    char        ExerIdxStr[6];
    pid_t   	fPID;
	char    comnametopass[HXECOM_HOSTNAMELEN];
	char msg_text[1024]; 
	struct htx_data * stats = (struct htx_data *)&htx_ds;

    GlobalWait(FORK_SEM, stats);
    if(CoordReady(rules->ComPortStream)) {
        GlobalSignal(FORK_SEM, stats);
        return;
    }
    memcpy(&shm_HXECOM->TempStats, stats, sizeof(struct htx_data));
/********************************************************************/
/* Fork Coordinator process.                                        */
/********************************************************************/
	
	printf("[%s];%d \n", __FUNCTION__, __LINE__); 
    fPID = fork();
    switch(fPID) {
        case 0:
            setpgrp();
            shmdt((char *) shm_Private);
            sprintf(comnametopass, "%s", rules->ComName);
            sprintf(CoordStreamPortStr, "%d", (int)rules->ComPortStream);
            sprintf(CoordDgramPortStr, "%d", (int)rules->ComPortDgram);
            sprintf(BroadcastIntStr, "%d", 15);
            sprintf(BroadcastIntRemainStr, "%d", 2);
            sprintf(ExerIdxStr, "%d", exer_idx);
        #ifndef _64BIT_
            execlp("hxecom2", "hxecom2", comnametopass, CoordStreamPortStr, CoordDgramPortStr, BroadcastIntStr,BroadcastIntRemainStr, ExerIdxStr,0);
        #else
            execlp("hxecom2_64", "hxecom2_64", comnametopass, CoordStreamPortStr, CoordDgramPortStr, BroadcastIntStr,BroadcastIntRemainStr, ExerIdxStr,0);
        #endif
            sprintf(msg_text, "SetUpCoord:  coord() returned. - %s\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_EXEC1,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
            HE_exit(EX_EXEC1);
            break;
        case -1:
            sprintf(msg_text, "SetUpCoord: Error forking Coord process - %s.\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_FORK1,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
            GlobalSignal(FORK_SEM, stats);
            HE_exit(EX_FORK1);
            break;
        default: 
            break;
    }
    while(CoordReady(rules->ComPortStream) == 0) {
        if(shm_HXECOM->exer[exer_idx].SigChildFlag == 1) {
            sprintf(msg_text, "SetUpCoord: Forking of Coord process failed\n");
            hxfmsg(stats, HTXERROR(EX_FORK8,0), HTX_HE_SOFT_ERROR, msg_text);
            GlobalSignal(FORK_SEM, stats);
            HE_exit(EX_FORK8);
        }
        sleep(1);
    }

    GlobalSignal(FORK_SEM, stats);
}



static void SetUpWriter(struct htx_data * stats, char * TestNetName, int layer, struct id_t * ServerID,
                         SOCKET ServerSock,int exer_idx, SOCKET msgsock)
{

    pid_t   fPID;
    struct  comwrite_argt * arg;
	char msg_text[1024]; 

    if((arg = (struct comwrite_argt *)malloc(sizeof(struct comwrite_argt))) == NULL) {
        sprintf(msg_text, "SetUpWriter: Malloc of Writer argument space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC2);
    }
    memcpy(&arg->stats, stats, sizeof(struct htx_data));
    memcpy(&arg->RemoteServerSock, &ServerID->sock, sizeof(struct sockaddr_in));
    strcpy(arg->TestNetName,TestNetName);

/********************************************************************/
/* Update CONFIG_FILE with remote IP address.                       */
/********************************************************************/
    WriteConfig(InetNtoa(arg->RemoteServerSock.sin_addr, msg_text, stats));


/********************************************************************/
/* Fork writer process.                                             */
/********************************************************************/
    fPID = fork();
    switch(fPID) {
        case 0:
            shmdt((char *) shm_Private);
            closesocket(ServerSock);
            closesocket(msgsock);
			#ifdef __RDMA_SUPPORTED__
			if(layer == RDMA_LAYER ) { 	
				printf("hxecom main server[%d] calling comwrite_rdma()\n", getpid());
				comwrite_rdma(arg, exer_idx);
			} else { /* default is TCP */
            	comwrite(arg); 
			}
			#else 
			comwrite(arg);
			#endif 
            sprintf(msg_text, "SetUpWriter: comwrite() returned.\n");
            hxfmsg(stats, HTXERROR(EX_EXEC2,0), HTX_HE_SOFT_ERROR, msg_text);
            HE_exit(EX_EXEC2);
            break;
        case -1:
            sprintf(msg_text, "SetUpWriter: Error forking writer process - %s.\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_FORK2,0), HTX_HE_SOFT_ERROR, msg_text);
            HE_exit(EX_FORK2);
            break;
        default:
            free(arg);
            break;
    }
}



static void SetUpReader(struct htx_data * stats, char TestInetName[], struct id_t * SemServID, 
                     struct id_t * ReaderID, int exer_idx, int WriteIdx, struct id_t * WriterTestID, int layer,
                      SOCKET ServerSock, SOCKET msgsock)
{
    pid_t   fPID;
    SOCKET     ReaderTestSock;
    struct comread_argt * arg;
	char msg_text[1024]; 

    if((arg = (struct comread_argt *)malloc(sizeof(struct comread_argt))) == NULL) {
        sprintf(msg_text, "SetUpReader: Malloc of reader space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC3,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC3);
    }
#ifdef __RDMA_SUPPORTED__ 
	if(layer == RDMA_LAYER) { 
    	struct sockaddr_in sock_in;
    	ReaderTestSock = socket(AF_INET, SOCK_STREAM, 0);
    	if(ReaderTestSock == INVALID_SOCKET) {
        	sprintf(msg_text, "[%s]:[%s]:[%d]: Error opening socket - %s.\n", __FILE__,__FUNCTION__, __LINE__, STRERROR(errno));
        	sprintf(msg_text + strlen(msg_text), "Inet Name = \"%s\".\n", TestInetName);
        	hxfmsg(stats, HTXERROR(EX_SOCK8,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        	HE_exit(EX_SOCK8);
    	}

 	   	memset(&sock_in, '\0', sizeof(sock_in));
		strcpy(msg_text, "SetUpReader :"); 	
    	if(GetIP(TestInetName, &sock_in.sin_addr, msg_text+strlen(msg_text))== -1) {
        	hxfmsg(stats, HTXERROR(EX_GETH3,h_errno), HTX_HE_SOFT_ERROR, msg_text);
        	closesocket(ReaderTestSock);
        	HE_exit(EX_GETH3);
    	}
    	ReaderID->sock.sin_family = AF_INET; 
		ReaderID->sock.sin_port = DEMO_CONN_QUAL - WriteIdx - 1 - 2 * MAXREMOTE * exer_idx; 
		memcpy(&ReaderID->sock.sin_addr, &sock_in.sin_addr, sizeof(struct in_addr)); 
	#ifdef __DEBUG__
    	printf("[%s]:[%s]:[%d] ByPasswd Open Port, Reader Listening on %s.%d \n", __FILE__,__FUNCTION__, __LINE__, inet_ntoa(ReaderID->sock.sin_addr), ReaderID->sock.sin_port);
	#endif
	} else { 
		OpenPort(TestInetName, ReaderID, &ReaderTestSock, layer, stats);
	}
#else
#ifdef __DEBUG__
    printf("comwrite_rdma() %d : getting info about reader\n",getpid());
#endif 
    OpenPort(TestInetName, ReaderID, &ReaderTestSock, layer, stats);
#endif
	printf("%s:%d Writer Test Sock = %s \n", __FUNCTION__, __LINE__, inet_ntoa(WriterTestID->sock.sin_addr)); 
    memcpy(&arg->SemServerSock, &SemServID->sock, sizeof(struct sockaddr_in));
    arg->WriterIdx = WriteIdx;
    memcpy(&arg->WriterTestSock, &WriterTestID->sock, sizeof(struct sockaddr_in));
    arg->ReaderTestSock = ReaderTestSock;
    memcpy(&arg->stats, stats, sizeof(struct htx_data));
	memcpy(&arg->ReaderID, &ReaderID->sock, sizeof(struct sockaddr_in));

/********************************************************************/
/* Update CONFIG_FILE with remote IP address.                       */
/********************************************************************/
    WriteConfig(InetNtoa(arg->SemServerSock.sin_addr, msg_text, stats));

/********************************************************************/
/* Fork reader process.                                             */
/********************************************************************/
    fPID = fork();
    switch(fPID) {
        case 0:
            shmdt((char *) shm_Private);
            closesocket(ServerSock);
            closesocket(msgsock);

			#ifdef __RDMA_SUPPORTED__	
			if(layer == RDMA_LAYER ) {
				printf("hxecom main server[%d] calling comread_rdma()\n", getpid());
				comread_rdma(arg, exer_idx);
			} else { /* default is TCP */ 
            	comread(arg);
			}
			#else 
				comread(arg); 
			#endif 
            sprintf(msg_text, "SetUpReader: comread() returned.\n");
            hxfmsg(stats, HTXERROR(EX_EXEC3,0), HTX_HE_SOFT_ERROR, msg_text);
            HE_exit(EX_EXEC3);
            break;
        case -1:
            sprintf(msg_text, "SetUpReader: Error forking reader process - %s.\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_FORK3,0), HTX_HE_SOFT_ERROR, msg_text);
            HE_exit(EX_FORK3);
            break;
        default:
            closesocket(ReaderTestSock);
            free(arg);
            break;
    }
	ReaderID->the_pid=fPID;
}


static void SetUpSemServ(struct htx_data * Stats, char * ComInetName, unsigned int bufseed, unsigned int layer, struct id_t * SemServID)
{
    pid_t   fPID;
    SOCKET     SemServSock;
    struct comsem_argt * arg;
	char msg_text[1024]; 
	struct htx_data * stats = (struct htx_data *)&htx_ds;

    if((arg = (struct comsem_argt *)malloc(sizeof(struct comsem_argt))) == NULL) {
        sprintf(msg_text, "SetUpSemServ: Malloc of SemServer argument space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC4,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC4);
    }

    OpenPort(ComInetName, SemServID, &SemServSock, (int)TCP_LAYER, stats);

    arg->SemServSock = SemServSock;
	arg->bufseed = bufseed; 
    memcpy(&arg->stats, Stats, sizeof(struct htx_data));

/********************************************************************/
/*  Create shared memory and sync variables needed by writer/comsem */
/********************************************************************/
#ifndef __RDMA_SUPPORTED__
    InitWriteVars(stats);
#else 
	if(layer == RDMA_LAYER ) {
		InitWriteVarsRdma(stats); 
	} else { 
		InitWriteVars(stats);
	}
#endif 

/********************************************************************/
/* Fork semserver process.                                          */
/********************************************************************/
    fPID = fork();
    switch(fPID) {
        case 0:
            shmdt((char *) shm_Private);
		#ifndef __RDMA_SUPPORTED__
            comsem(arg);
		#else 
			if(layer == RDMA_LAYER ) {
				comsem_rdma(arg);
			} else { 
				comsem(arg);
			} 
		#endif 
            sprintf(msg_text, "SetUpSemServ: comsem() returned.\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_EXEC4,0), HTX_HE_SOFT_ERROR, msg_text);
            HE_exit(EX_EXEC4);
            break;
        case -1:
            /********************************************************/
            /* Detach from shared memory created in InitWriteVars() */
            /********************************************************/
		#ifndef __RDMA_SUPPORTED__
            Detach_COMSEM();
		#else 
			if(layer == RDMA_LAYER ) {
				Detach_COMSEM_RDMA();
			} else { 
				Detach_COMSEM();
			} 
		#endif 
            sprintf(msg_text, "SetUpSemServ: Error forking comsem process - %s.\n", STRERROR(errno));
            hxfmsg(stats, HTXERROR(EX_FORK4,0), HTX_HE_SOFT_ERROR, msg_text);
            HE_exit(EX_FORK4);
            break;
        default:
            closesocket(SemServSock);
            /********************************************************/
            /* Detach from shared memory created in InitWriteVars() */
            /********************************************************/
		#ifndef __RDMA_SUPPORTED__
             Detach_COMSEM();
        #else
			if(layer == RDMA_LAYER ) {
             	Detach_COMSEM_RDMA();
			} else { 
				Detach_COMSEM();
			}
        #endif
            free(arg);
            break;
    }
}

static void SIGALRM_hdl(int sig, int code, struct sigcontext * scp)
 {
   shm_pHXECOM->SigAlarmFlag = 1;
   return;
}


static void SIGPIPE_hdl(int sig, int code, struct sigcontext * scp)
{
   return;
}

static void SIGINT_hdl(int sig, int code, struct sigcontext * scp)
{
	/* sigint sent by partent to children when parent gets sigterm. */
	HE_exit(0);
}

static void SIGTERM_hdl(int sig, int code, struct sigcontext * scp)
{
	char msg_text[1024];

	if(in_wait_loop) {
		shm_pHXECOM->SigTermFlag = 1;
	} else {
		/*this should kill all the children that have not shutdown by now.*/
		/*It needs the process id of this hxecom so that it does not kill */
		/* this parent */
		sprintf(msg_text,"%s/etc/scripts/pscheck %d", global_htx_home_dir, getpid());
		system(msg_text);
		HE_exit(0);
	}
}


static void ExitCleanup(void) 
{
    int ParentFlag = 0;

    if(shm_HXECOM  != (struct shm_hxecom_t  *) 0) {
        /* Only main exerciser hxecom should execute these lines.      */
        if(shm_HXECOM->exer[shm_pHXECOM->ExerNo].EXER_PID == getpid()) {
            ParentFlag = 1;
        }
        Detach_HXECOM(shm_HXECOM);
    }
    if(shm_pHXECOM != (struct shm_phxecom_t *) 0)
        Detach_pHXECOM(shm_pHXECOM);


    if(ParentFlag) {
        if(shm_Private != (struct shm_Private_t *) 0)
            Detach_Private(shm_Private);
	}
}


static void Shutdown(struct CoordMsg CMsg, struct sockaddr_in * Coord)
{
	struct htx_data * stats = (struct htx_data *)&htx_ds;
    int ToLocalCoordSock;
	char msg_text[1024]; 

    shm_pHXECOM->SigUsr1Flag = 1;
    ToLocalCoordSock = SetUpConnect(Coord, stats, SH_FORCE);
    StreamWrite(ToLocalCoordSock, (char *)&CMsg, sizeof(CMsg));
    closesocket(ToLocalCoordSock);
	/* take everyone else with you here */
	sprintf(msg_text,"%s/etc/scripts/pscheck %d", global_htx_home_dir, getpid());
	system(msg_text);
    HE_exit(0);
}


static void RemoveCoord(void)
{
    int i, ToLocalCoordSock;
	struct htx_data * stats = (struct htx_data *)&htx_ds;
    struct CoordMsg CMsg;
    u_short ComStreamPort;

    ComStreamPort = ntohs(coordSockPtr->sin_port);

    GlobalWait(COORD_SEM, stats);
    for(i=0; i<shm_HXECOM->NoCoord; i++) {
        if(shm_HXECOM->Coord[i].ComPortStream == ComStreamPort)
            break;
    }
    if(i < shm_HXECOM->NoCoord) {
        shm_HXECOM->Coord[i].ExerCnt--;
        if(shm_HXECOM->Coord[i].ExerCnt == 0) {
            CMsg.msg_type = htonl(CM_SIGTERM);
            ToLocalCoordSock = SetUpConnect(coordSockPtr, stats, 0);
            StreamWrite(ToLocalCoordSock, (char *)&CMsg, sizeof(CMsg));
            closesocket(ToLocalCoordSock);
        }
    }
    GlobalSignal(COORD_SEM, stats);
}


static void Detach_pHXECOM(struct shm_phxecom_t * ptr)
{
    int MID;
    struct shmid_ds ShmDs;

    if(ptr == (struct shm_phxecom_t *)NULL)
        return;

    MID = ptr->mID;
    shmdt((char *)ptr);

    shmctl(MID, IPC_STAT, &ShmDs);
    if(ShmDs.shm_nattch == 0) {
         shmctl(MID, IPC_RMID, 0);
    }
}


static int CoordReady(u_short ComStreamPort)
{
    int i, rc;
	struct htx_data * stats = (struct htx_data *)&htx_ds;

    GlobalWait(COORD_SEM, stats);
    for(i=0; i<shm_HXECOM->NoCoord; i++) {
        if(shm_HXECOM->Coord[i].ComPortStream == (u_short) ComStreamPort)
            break;
    }
    if(i < shm_HXECOM->NoCoord) {
        shm_HXECOM->Coord[i].ExerCnt++;
        rc = 1;
    }
    else 
        rc = 0;
    GlobalSignal(COORD_SEM, stats);
    return rc;
}

static int GetPath(char path[], const char pathin[], int len, const char envirVar[], const char defaultPath[])
{  
   	int    envirLen;
   	int    inLen;
   	int    defLen;
   	char * Eptr;
	char msg_text[1024]; 
	struct htx_data * stats = (struct htx_data *)&htx_ds;

   if((inLen = (int)strlen(pathin)) > len) {
      strcpy(msg_text, "GetPath: Path parameter is too long.\n");
      hxfmsg(stats, HTXERROR(HEINFO3,0), HTX_HE_SOFT_ERROR, msg_text);
      path[0] = '0';
      return 0; 
   }
   strcpy(path, pathin);
   if(CkPath(path, msg_text)) {
      return 1;
   }
   
   /* If pathin begins with '.' or '/', CkPath failed and we are through.     */
   if(pathin[0] == '.' || pathin[0] == SLASH_CHR) {
      /* print error message formatted by CkPath funcion.                     */
      hxfmsg(stats, HTXERROR(HEINFO3,0), HTX_HE_SOFT_ERROR, msg_text);
      path[0] = '0';
      return 0;
   }

   if((defLen = (int)strlen(defaultPath)) != 0) {
      if(defLen + inLen +2 > len) {
         sprintf(msg_text, "GetPath: Total default path + name is longer than %d characters\n.", len);
         hxfmsg(stats, HTXERROR(HEINFO3,0), HTX_HE_SOFT_ERROR, msg_text);
         path[0] = '0';
         return 0;
      }
      strcpy(path, defaultPath);
      strcat(path, SLASH_TXT);
      strcat(path, pathin);
      if(CkPath(path, msg_text + strlen(msg_text)))
         return 1;
   }

   /* If environmental variable doesn't exist, Ckpath has failed.             */
   if((Eptr = getenv(envirVar)) == NULL) {
      /* Append to message formatted by CkPath function.                      */
      sprintf(msg_text + strlen(msg_text), "Environmental variable %s doesn't exist or is not exported.\n", envirVar);
      hxfmsg(stats, HTXERROR(HEINFO3,0), HTX_HE_SOFT_ERROR, msg_text);
      path[0] = '0';
      return 0;
   }
   envirLen = (int)strlen(Eptr);
   if(envirLen + inLen +2 > len) {
      sprintf(msg_text, "GetPath: Total environ path + name is longer than %d characters\n.", len);
      hxfmsg(stats, HTXERROR(HEINFO3,0), HTX_HE_SOFT_ERROR, msg_text);
      path[0] = '0';
      return 0;
   }
   strcpy(path, getenv(envirVar));
   strcat(path, SLASH_TXT);
   strcat(path, pathin);
   if(CkPath(path, msg_text + strlen(msg_text)) == 0) {
      /* Print error message formatted by CkPath function.                    */
      hxfmsg(stats, HTXERROR(HEINFO3,0), HTX_HE_SOFT_ERROR, msg_text);
      path[0] = '0';
      return 0;
   }
   return 1;
}



static int CkPath(const char path[], char * err_msg_text)
{
   int des;

   errno = 0;
   if((des = open(path, O_RDONLY, 0)) == -1) {
      /* Since this is a check, we won't print error message -- just make it  */
      /* available.                                                           */
      sprintf(err_msg_text, "CkPath: open error: \"%s%s\" - %s\n", 
             (path[0] == '.' || path[0] == SLASH_CHR) ? "" : SLASH_TXT, path, STRERROR(errno));
      return 0;
   } else {
      close(des);
      return 1;
   }
}



static int GetLengthOfFile(char * path)
{
   	int des, len;
	char msg_text[1024]; 
	struct htx_data * stats = (struct htx_data *)&htx_ds;

   if((des = open(path, O_RDONLY, 0)) == -1) {
      sprintf(msg_text, "CkPath: open error: \"%s%s\" - %s\n",
             (path[0] == '.' || path[0] == SLASH_CHR) ? "" : SLASH_TXT, path, STRERROR(errno));
      hxfmsg(stats, HTXERROR(EX_OPEN1, ERRNO), HTX_HE_SOFT_ERROR, msg_text);
      HE_exit(EX_OPEN1);
   }
   if((len = lseek(des, 0, SEEK_END)) == -1) {
      sprintf(msg_text, "GetLengthOfFile: lseek error - %s\n", STRERROR(errno));
      hxfmsg(stats, HTXERROR(EX_SEEK1, ERRNO), HTX_HE_SOFT_ERROR, msg_text);
      HE_exit(EX_SEEK1);
   }
   close(des);
   return len;
}



static void PrtSyntax(char * ProgramName)
{
   fprintf(stderr, "usage: %s /dev/XXXXX [REG EMC] rule_path\n", ProgramName);
   fprintf(stderr, "   or: %s /dev/XXXXX OTH rule_path\n", ProgramName);
}

static void SetUpOtherHosts(char *ThisComName)
{
    FILE *f_other;
    char inline_s[64];
    char id[32];
    char save_id[32];
    char this_save_id[32];
    char theaddr[32];
    char subnetbase[32];
    char usesubnet[32];
    char ch;
	in_addr_t tmpadr;
    int t;
	int fullip;
    char *com1;
    char *com2;
    char *com3;
    char *com4;
    char *oct1;
    char *oct2;
    char *oct3;
    char *oct4;
	char addrStr[32];
	char msg_text[1024];
	struct sockaddr_in thiscom;
	struct htx_data * stats = (struct htx_data *)&htx_ds;
	char	temp_string[300];

	if(shm_HXECOM->OneSysFlag) return;

	/* note: onesys test does not use this. broadcasts are done on testnets */
	/* for multi system tests I get the other ids in the test from the 
	   /tmp/other_ids file that build_net creates. If only the last nibble is
	   included in the file, then all systems must be on the same subnet for   
	   or the connections will not happen, test will hang. The use must give   
	   build_net the full ip of each comnet for each host in the test if not  
	   using hosts on the same subnet. */


    /* if other_ids files exists */
    sprintf(temp_string, "%s/other_ids", global_htx_log_dir);	
    if( (f_other=fopen(temp_string,"r"))==NULL) {
      sprintf(msg_text, "Error opening %s: %s\n", temp_string, STRERROR(errno));
      hxfmsg(stats, HTXERROR(EX_OTHERID, ERRNO), HTX_HE_HARD_ERROR, msg_text);
      HE_exit(EX_OTHERID);
    } 

    strcpy(msg_text, "SetUpOtherHosts: ");
    if(GetIP(ThisComName, &thiscom.sin_addr, msg_text+strlen(msg_text))) {
        hxfmsg(stats, HTXERROR(EX_OTHERID1,ERRNO), HTX_HE_HARD_ERROR, msg_text)
;
        HE_exit(EX_OTHERID1);
    }

	InetNtoa(thiscom.sin_addr, addrStr, stats);
	strcpy(this_save_id,addrStr);

	/* addrStr has the ip address of this host comnet. */
	com1 = strtok(addrStr,".");
	strcpy(subnetbase,com1);
	strcat(subnetbase,".");
	com2 = strtok(NULL,".");
	strcat(subnetbase,com2);
	strcat(subnetbase,".");
	com3 = strtok(NULL,".");
	strcat(subnetbase,com3);
	strcat(subnetbase,".");
	com4 = strtok(NULL,".");

	strcpy(shm_HXECOM->mynetid,com4);

	/* fgets reads to end of line or 32 bytes whichever comes first */
	while (fgets(inline_s,32,f_other) != NULL) {
		/* this loop pulls off the end of line */
		fullip=0;
		for(t=0; t<32; t++) {
			ch = inline_s[t];
			if(ch == '\n') break;
			if(ch == '.') fullip=1;
			id[t] = inline_s[t];
		}
		id[t]='\0';

		if(fullip) {
			if(strcmp(this_save_id,id)!=0) {
				if((tmpadr = inet_addr(id)) < 0) {
					sprintf(msg_text+strlen(msg_text),"Bad ip address found in %s file\n", temp_string);
					hxfmsg(stats, HTXERROR(EX_OTHERID2,ERRNO), HTX_HE_HARD_ERROR, msg_text);
					HE_exit(EX_OTHERID2);
				
				}
				shm_HXECOM->Other_ips[shm_HXECOM->HostCnt++].s_addr=(in_addr_t)tmpadr;
			}
		} else {
			/* note if you don't have all test systems on the same subnet
			   this will cause a problem if the other ids file does not have
			   the full ip. So use full ip in other_ids file if all hosts are
			   not on the same subnet */
			if(strcmp(com4,id)!=0) {
				strcpy(usesubnet,subnetbase);
				strcat(usesubnet,id);
				if((tmpadr = inet_addr(usesubnet)) < 0) {
					sprintf(msg_text+strlen(msg_text),"Bad ip address found in %s file\n", temp_string);
					hxfmsg(stats, HTXERROR(EX_OTHERID3,ERRNO), HTX_HE_HARD_ERROR, msg_text);
					HE_exit(EX_OTHERID3);
				
				}
				shm_HXECOM->Other_ips[shm_HXECOM->HostCnt++].s_addr=(in_addr_t)tmpadr;
			}
		}
	}
	fclose(f_other);
    return;
}


static void WriteConfig(char * remoteIP)
{
   	FILE *config_des;
   	char Line[81];
	struct htx_data * stats = (struct htx_data *)&htx_ds;
	char temp_string[300];


    sprintf(temp_string,"%s/%s", global_htx_log_dir, CONFIG_FILE);
    config_des = fopen(temp_string, "r+");
    GlobalWait(FILE_SEM, stats);
    while(fgets(Line, 80, config_des) != NULL) {
        /* Skip comments in file.                    */
        if(Line[0] == '#')
            continue;
        /* Only write unique IP addresses to file.    */
        if(memcmp(remoteIP, Line, strlen(remoteIP)) == 0) {
            GlobalSignal(FILE_SEM, stats);
            fclose(config_des);
            return;
        }
    }
    fprintf(config_des, "%s\n", remoteIP);
    GlobalSignal(FILE_SEM, stats);
    fclose(config_des);
}



void HE_exit(int exit_no)
{
    exit(exit_no);
}


int com_hxfmsg(struct htx_data *p, int err, enum sev_code  sev, char *text)
{
     hxfmsg(p, err, sev, text);
}


