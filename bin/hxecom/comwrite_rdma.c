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
#    include <sys/types.h>
#    include <sys/socket.h>
#    include <sys/sem.h>
#    include <sys/select.h>
#    include <sys/ioctl.h>
#    include <netinet/in.h>
#    include <netinet/tcp.h>
#    include <netdb.h>
#    include <sys/shm.h>
#    include "hxecomdef.h"
#    include "comrw.h"
#	 include "comdapl.h"

#ifdef __HTX_LINUX__
#    include "fcntl.h"
#    include <sys/stat.h>
#    include <time.h>
#else 
#	include <sys/time.h>
#endif


#ifdef __HTX_LINUX__
#define ulong unsigned long
#define NFDS(a)   (a)
#endif

#define MISC 0xBEEFDEAD
#define VER 0x10140900

#define MAX_EEH_ERRORS  100
/*define NO_BLOCK 1 */

/******************************************************************************/
/***  Global Variable Definitions shared only among threads. ******************/
/******************************************************************************/
    static struct shm_comsem_t * shm_COMSEM;
    static int SemID;


static void RWmsg(struct cum_rw * Lstats, struct rule_format * rule, int Stanza, struct htx_data * stats, int err, enum sev_code sev, char * msg_text);
static struct id_t GetRdrID(struct sockaddr_in ServerID, struct id_t SemServID, u_long WriterIdx, struct id_t WriterID, struct htx_data * sstats);

static void ExitCleanup();
static void bad_bytes_wrote(struct cum_rw * Lstats, struct htx_data * pstats,
                                              struct htx_data * stats, int bytes, int BadCnt);
static void WriteLocalStats(struct cum_rw LSTATS[], int NoStanzas, char ConnectStr[], struct htx_data * stats, char * msg_text);
static void WriteExit(struct htx_data * stats, struct cum_rw * LSTATS, int NoStanzas, int exitNo, int WriterIdx,
                        char msg[], char ConnectStr[]);
static void Req_StopReader(struct sockaddr_in RemServersock,struct id_t SemServID, struct id_t ThisServerID, struct id_t ReaderID, struct htx_data * sstats);
static void good_bytes_wrote(struct cum_rw * Lstats, struct htx_data * stats, int bytes, char ConnectStr[]); 
static void good_bytes_read(struct cum_rw * Lstats, struct htx_data * sstats, int bytes, char ConnectStr[]);
static int GetUniqueWIdx(struct htx_data * stats);
static void RecvAddrExc(SOCKET msgsock, struct mr_triplet rmr_info, struct htx_data *stats ); 
static void hex_dump(unsigned char * buf, int dlen ); 
void Detach_COMSEM_RDMA(void);

extern char global_htx_log_dir[256];

/* Global udapl instance */
static struct dapl_device *dapl_instance_ptr = NULL;
static int WriterIdx; 

/* Global EEH stuff */ 
unsigned int    eeh_enabled = 0;
unsigned int    eeh_retries = 0;

#define MISC 0xBEEFDEAD
#define VER 0x10140900
#pragma mc_func trap { "7c810808" }
#pragma reg_killed_by trap

#ifndef __HTX_LINUX__
#pragma mc_func attn { "00000200" }
#pragma reg_killed_by attn
#else
static void __attn (unsigned int a,
             unsigned int b,
             unsigned int c,
             unsigned int d,
         unsigned int e,
         unsigned int f){
        __asm__ volatile (".long 0x00000200":);
        }
#endif


void *comwrite_rdma(void * writer_arg, int exer_idx)
{
    DAT_RETURN ret;
	DAT_DTO_COOKIE cookie;
    int i, rc, stanza, num_stanza;
    char *strt_rbuf = NULL, *rbuf = NULL;
    char *strt_sbuf = NULL, *sbuf = NULL;
	char * PATTERN = NULL, * debug_ptr = NULL, * patternfile = NULL, * pattern_trailer = NULL; 
    unsigned int buflen = 0;
    struct comwrite_argt *arg = NULL;
    struct rule_format rule[MAX_STANZAS];
	int bsize, pakLen; 
	char msgp[MAX_TEXT_MSG];
	struct id_t ReaderID;
	struct id_t WriterID;
	char connect_str[CONNECTSTR_LEN];
    socklen_t addrlen;
    struct sockaddr_in *server_sockaddr = NULL;
    struct sockaddr_in reader_sockaddr;
    struct htx_data *stats = NULL;
	char tmpaddr[32];
    char tmpaddr1[32];
	char addr_str[INET_ADDRSTRLEN];
	unsigned char timestamp = '\0';
	int    sleep_time;
	int    pak=0, o;
	int    write_try=1 ;
    int thetime, dummy_time;
    int elasped_time;
    int twelve_hours=43200;
	unsigned int pid = getpid(); 
	struct cum_rw * LSTATS;
	struct cum_rw * lstats;
	u_short ShutdownMask = 0;
#ifdef __RDMA_SUPPORTED__
	#ifdef __HTX_LINUX__
		struct timespec start, finish; 
	#else 
		timebasestruct_t start, finish;
	#endif 
	int secs, n_secs;
	double thruput, latency; 
	char unit; 
#endif 
	char msg_text[2048], msg_text1[2048]; 

    arg = (struct comwrite_argt *)writer_arg;
    stats = &arg->stats;
    addrlen = sizeof(reader_sockaddr);

    atexit(ExitCleanup);

	/********************************************************************/
	/* Attach to comsem's shared memory.                                */
	/********************************************************************/

    if ((int)(shm_COMSEM = (struct shm_comsem_t *)shmat(shm_pHXECOM->mID_COMSEM, (char *)0, 0)) ==  -1) {
        sprintf(msg_text, "comwrite_rdma: Error attaching shared memory - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_SHMAT2,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_SHMAT2);
    }

	/********************************************************************/
	/* Setup temparary ConnectStr just in case we are terminated.       */
	/********************************************************************/
    sprintf(connect_str, "Writer @ %s not initialized.\n", arg->TestNetName);

	/********************************************************************/
	/* Need unique index number to access own WRITE_AHEAD semaphore.    */
	/********************************************************************/
    WriterIdx = GetUniqueWIdx(stats);
	
	/*********************************************************************/
	/* Due to EEH, writer/reader pair may reset, 						 */ 
	/* this writer process may get older WriterIdx. So reset older writer*/ 
	/* specific data 													 */
	/*********************************************************************/ 
	memset(&shm_COMSEM->loop[WriterIdx], 0, sizeof(struct loop_t)); 
	
	/********************************************************************/
	/* Initialize ConnectStr 											*/ 
	/********************************************************************/
	SOCKET WriterTestSock; 

	struct sockaddr_in sock_in;
	WriterTestSock = socket(AF_INET, SOCK_STREAM, 0);
   	if(WriterTestSock == INVALID_SOCKET) {
       	sprintf(msg_text, "[%s]:[%s]:[%d]: Error opening socket - %s.\n", __FILE__,__FUNCTION__, __LINE__, STRERROR(errno));
       	sprintf(msg_text + strlen(msg_text), "Inet Name = \"%s\".\n", arg->TestNetName);
       	hxfmsg(stats, HTXERROR(EX_SOCK8,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
   	 	HE_exit(EX_SOCK8);
   	}

   	memset(&sock_in, '\0', sizeof(sock_in));
   	if(GetIP(arg->TestNetName, &sock_in.sin_addr, msg_text+strlen(msg_text))== -1) {
       	hxfmsg(stats, HTXERROR(EX_GETH3,h_errno), HTX_HE_SOFT_ERROR, msg_text);
       	closesocket(WriterTestSock);
       	HE_exit(EX_GETH3);
   	}
	WriterID.sock.sin_family = AF_INET;
	WriterID.sock.sin_port = DEMO_CONN_QUAL + WriterIdx + 2 * MAXREMOTE * exer_idx ;
	memcpy(&WriterID.sock.sin_addr, &sock_in.sin_addr, sizeof(struct in_addr));

	printf("WriterTestSock = %x \n", WriterTestSock); 

    printf("comwrite_rdma() %d : getting info about reader\n",getpid());


	/********************************************************************/
	/* Obtain rules file stanzas as array of struct rule_format         */
	/********************************************************************/

    num_stanza = GetRules(arg->RemoteServerSock, rule, MAX_STANZAS, stats);

	
	/********************************************************************/
	/* Find largest pattern needed.                                     */
	/********************************************************************/
	for(i = 0; i < num_stanza; i++) {
        if (rule[i].bufmax > buflen) {
            buflen = rule[i].bufmax;
        }
    }
	

	close (WriterTestSock);
	/********************************************************************/
	/* Allocate space for local stats and zero.                         */
	/********************************************************************/
    if((LSTATS = (struct cum_rw *)malloc(sizeof(struct cum_rw) * num_stanza)) == NULL) {
        sprintf(msg_text, "comwrite: Malloc of local stats space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC11,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC11);
    }
    for(i=0; i< num_stanza; i++) {
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

    if ((strt_sbuf = (char *)malloc(buflen + 2 * (TP_HDR_LEN) + DAT_OPTIMAL_ALIGNMENT)) == NULL) {
        sprintf(msg_text, "comwrite_rdma: Malloc of write buffer space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC9,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC9);
    }
	if ((strt_rbuf = (char *)malloc(buflen + 2 * (TP_HDR_LEN) + DAT_OPTIMAL_ALIGNMENT)) == NULL) {
        sprintf(msg_text, "comwrite_rdma: Malloc of read buffer space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC6,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC6);
    }
    memset(strt_sbuf, 0xBB, buflen + 2 * (TP_HDR_LEN) + DAT_OPTIMAL_ALIGNMENT);
    memset(strt_rbuf, 0xBB, buflen + 2 * (TP_HDR_LEN) + DAT_OPTIMAL_ALIGNMENT);

	/********************************************************************/ 
	/* Align the send/recv buffer to DAT_OPTIMAL_ALIGNMENT 				*/ 
	/********************************************************************/ 
	if(DAT_OPTIMAL_ALIGNMENT) {
        sbuf = (char *)((unsigned long long)((unsigned long long)strt_sbuf + (unsigned long long)(DAT_OPTIMAL_ALIGNMENT - 1)) & 
					  (unsigned long long)(~((unsigned long long)(DAT_OPTIMAL_ALIGNMENT - 1))));
    } else { 
		sbuf = strt_sbuf; 
	}
	if(DAT_OPTIMAL_ALIGNMENT) { 
		 rbuf = (char *)((unsigned long long)((unsigned long long)strt_rbuf + (unsigned long long)(DAT_OPTIMAL_ALIGNMENT - 1)) &
	                     (unsigned long long)(~((unsigned long long)(DAT_OPTIMAL_ALIGNMENT - 1))));
    } else {
         rbuf = strt_rbuf;
    }


	/********************************************************************/
	/* Allocate space for copy of pattern file.                         */
	/********************************************************************/
    if((PATTERN = (char *)malloc(buflen * sizeof(char))) == NULL) {
        sprintf(msg_text, "comwrite: Malloc of pattern file space failed - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_MALLOC10,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_MALLOC10);
    }
    patternfile = sbuf + (TP_HDR_LEN);

	/********************************************************************/
	/* Obtain pattern file for write.                                   */
	/********************************************************************/
    rc = GetPattern(arg->RemoteServerSock, PATTERN, buflen, stats);


	/********************************************************************/
	/* Request connection information for a reader.                     */
	/* Send ID info relating to SemServer.                              */
	/* Send WriterIdx, identifying WRITE_AHEAD semaphore index.         */
	/* Send TestNetName and TestPort for this writer.                   */
	/* Read ReaderID identifying connection info to write test partner. */
	/********************************************************************/
#ifdef __DEBUG__
	printf("[%s]:[%s]:[%d] Writer = %d Caling GetRdrID \n", __FILE__, __FUNCTION__, __LINE__, WriterIdx); 
#endif
    ReaderID = GetRdrID(arg->RemoteServerSock, shm_pHXECOM->SemServID, (u_long)WriterIdx, WriterID, stats);

	/********************************************************************/
	/* Obtain string used for ID when writing messages.                 */
	/********************************************************************/
    GetConnectStr(connect_str, CONNECTSTR_LEN, "W", WriterID.sock, " connected to R", ReaderID.sock, stats);
#ifdef __DEBUG__
    printf("comwrite_rdma() connect_str: %s\n", connect_str);
#endif

	/*********************************************************************/ 
	/* examine the ConnectStr and make sure reader and writer are not the*/ 
	/* same address 													 */
	/*********************************************************************/
#if 0 
	if(strcmp(InetNtoa(WriterID.sock.sin_addr, tmpaddr, stats),
		InetNtoa(ReaderID.sock.sin_addr, tmpaddr1, stats))==0) {
	#ifdef __DEBUG__
		printf("[%s]:[%s]:[%d] Exiting bcoz tmpaddr = %s, tmpaddr1 = %s are equal \n",  __FILE__, __FUNCTION__, __LINE__, tmpaddr, tmpaddr1); 
	#endif
		HE_exit(0);
	}
#endif 
    ret = dapl_init_instance(stats->sdev_id + 5, WriterIdx,  ntohs(ReaderID.sock.sin_port), &dapl_instance_ptr, stats, DAT_TIMEOUT_INFINITE);
	if (ret != DAT_SUCCESS) {
		sprintf(msg_text, "comwrite_rdma: failed dapl_init_instance : ret = %d \n", ret); 
		hxfmsg(stats, HTXERROR(EX_DAPL_INIT,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
		HE_exit(EX_DAPL_INIT);
	}
	/********************************************************************/ 
	/* register rbuf, we are going to recv RDMA packet in this buffer   */ 
	/********************************************************************/
    ret = dapl_register_mem(dapl_instance_ptr, rbuf, (buflen + 2 * (TP_HDR_LEN)), &dapl_instance_ptr->rcv_mr, stats);
	if (ret != DAT_SUCCESS) {
		sprintf(msg_text, "comwrite_rdma: failed dapl_register_mem for rbuf = 0x%x, ret = %d \n", rbuf, ret); 
		hxfmsg(stats, HTXERROR(EX_DAPL_REGISTER, ERRNO), HTX_HE_SOFT_ERROR, msg_text);
		HE_exit(EX_DAPL_REGISTER);
	}
	/********************************************************************/
	/* register sbuf, we are going to send RDMA packet from this buffer */ 
	/********************************************************************/
    ret = dapl_register_mem(dapl_instance_ptr, sbuf, (buflen + 2 * (TP_HDR_LEN)), &dapl_instance_ptr->snd_mr, stats);
	if (ret != DAT_SUCCESS) {
		sprintf(msg_text, "comwrite_rdma: failed dapl_register_mem for sbuf = 0x%x, ret = %d \n", sbuf, ret); 
		hxfmsg(stats, HTXERROR(EX_DAPL_REGISTER, ERRNO), HTX_HE_SOFT_ERROR, msg_text); 
		HE_exit(EX_DAPL_REGISTER);
	}
		
	/*******************************************************************/
	/* Populate shared memory with our recv_iov, this is going to be   */ 
	/* used during address exchange. We are going to recv RDMA packet  */
	/* in rbuf, so we need to communicate this to our reader		   */ 
	/*******************************************************************/ 
	shm_COMSEM->loop[WriterIdx].recv_iov.lmr_context = dapl_instance_ptr->rcv_mr.lmr_ctx;
	shm_COMSEM->loop[WriterIdx].recv_iov.virtual_address = (DAT_VADDR)rbuf; 
	shm_COMSEM->loop[WriterIdx].recv_iov.segment_length = sizeof(DAT_LMR_TRIPLET);

	/********************************************************************/
	/* Wait for reset to start.  Don't attempt to communicate with      */
	/* reader process until it indicates that it is ready (i.e. reset). */
	/********************************************************************/
	#ifdef __DEBUG__
		sprintf(msg_text, "comwrite_rdma: Waiting For Reset, pid = %d, SemId = %d, WriteIDx = %d\n",getpid(), SemID, WriterIdx); 
		hxfmsg(stats, HTXERROR(EX_LAYER2,0), HTX_HE_INFO, msg_text);
	#endif
    GateSem(SemID, WriterIdx, stats);  
    if ((int)shm_COMSEM->loop[WriterIdx].WriterReset == 1) {
        shm_COMSEM->loop[WriterIdx].WriterReset = 0;
    }

	#ifdef __DEBUG__
		printf("%s: DAPL Writer calling connect_ep to Reader on port = 0x%x\n", __FUNCTION__, dapl_instance_ptr->conn_id);
	#endif

	/********************************************************************/ 
	/* Connect with Reader's endpoint 									*/ 
	/********************************************************************/
	ret = dapl_connect_ep(dapl_instance_ptr, &ReaderID.sock, NULL, 0, stats);
	if (ret != DAT_SUCCESS) {
		sprintf(msg_text, "comwrite_rdma: dapl_connect_ep failed ret = %d \n", ret); 
		hxfmsg(stats, HTXERROR(EX_DAPL_CONNECT, ERRNO), HTX_HE_SOFT_ERROR, msg_text);
		HE_exit(EX_DAPL_CONNECT);
	}
#ifdef __DEBUG__
    sprintf(msg_text, "Writer : Idx = %d, Connected ! \n %s \n", WriterIdx, connect_str);
    hxfmsg(stats, HTXERROR(HEINFO10,0), HTX_HE_INFO, msg_text);
#endif

    ret = dat_ep_query(dapl_instance_ptr->h_dev.ep, DAT_EP_FIELD_ALL, &dapl_instance_ptr->ep_param);
    if (ret != DAT_SUCCESS) {
        fprintf(stderr, "%d Error dat_ep_query: %s\n", getpid(), dapl_ret_tostr(ret));
		HE_exit(EX_DAPL_CONNECT);
    } else {
        printf("%d EP queried %p\n", getpid(), dapl_instance_ptr->h_dev.ep);
        dapl_dump_ep_param(&dapl_instance_ptr->ep_param);
		inet_ntop(AF_INET, &((struct sockaddr_in *)dapl_instance_ptr->ep_param.local_ia_address_ptr)->sin_addr,
					addr_str, sizeof(addr_str));
		printf("[%d] Query EP: LOCAL addr %s port  %lx\n", getpid(), addr_str, dapl_instance_ptr->ep_param.local_port_qual);
		inet_ntop(AF_INET, &((struct sockaddr_in *)dapl_instance_ptr->ep_param.remote_ia_address_ptr)->sin_addr,
					addr_str, sizeof(addr_str));
		printf("[%d] Query EP: REMOTE addr %s port %lx\n", getpid(), addr_str, dapl_instance_ptr->ep_param.remote_port_qual);
    }
	/********************************************************************/
	/* Exchange Remote DMA location and key information 				*/
	/********************************************************************/	
	SetSem(SemID, WriterIdx, 0, stats);
	shm_COMSEM->loop[WriterIdx].halt = 1;
	GateSem(SemID, WriterIdx, stats);
	/* Second check to verify that we are good to use remote_iov from shm_COMSEM 
	 * halt = 1, once comsem completes the address exchange successfully  */ 
	while(shm_COMSEM->loop[WriterIdx].halt != 0) { 
		sleep(1);
	} 	
	/********************************************************************/ 
	/* Copy the remote buffer information obtained . 					*/ 
	/********************************************************************/
	dapl_instance_ptr->rbuf_info.virtual_address = shm_COMSEM->loop[WriterIdx].remote_iov.virtual_address ; 
	dapl_instance_ptr->rbuf_info.segment_length = shm_COMSEM->loop[WriterIdx].remote_iov.segment_length ; 
	dapl_instance_ptr->rbuf_info.rmr_context = shm_COMSEM->loop[WriterIdx].remote_iov.rmr_context;  	
	

#ifdef __DEBUG__
	sprintf(msg_text, " WriterIdx = %d, Writer Info : RDMA_WRITE send_buf = 0x%llx, lmr_context = %d \n " 
															" RDMA_READ  recv_buf = 0x%llx, lmr_context = %d \n",
							 WriterIdx, dapl_instance_ptr->snd_mr.region.for_va, dapl_instance_ptr->snd_mr.lmr_ctx, 
										dapl_instance_ptr->rcv_mr.region.for_va, dapl_instance_ptr->rcv_mr.lmr_ctx);   														 
	hxfmsg(stats, HTXERROR(EX_WRITE2,0), HTX_HE_INFO, msg_text); 							
	sprintf(msg_text, " WriterIdx = %d, Reader Info : remote_buffer addr = 0x%llx, segment_length = %d, rmr_context = %d \n", WriterIdx,  
							 dapl_instance_ptr->rbuf_info.virtual_address, dapl_instance_ptr->rbuf_info.segment_length, dapl_instance_ptr->rbuf_info.rmr_context);  
	hxfmsg(stats, HTXERROR(EX_WRITE2,0), HTX_HE_INFO, msg_text); 							
#endif 
    sprintf(msg_text, "%s\nWriter : %d, Beginning RDMA test...: EEH Enabled =%d, Retry Count=%d, OneSysFlag=%d,pid=%d", 
								connect_str, WriterIdx, eeh_enabled, shm_pHXECOM->BrokenPipes, shm_HXECOM->OneSysFlag, getpid());
    hxfmsg(stats, HTXERROR(EX_WRITE2,0), HTX_HE_INFO, msg_text);

	/* Need to sleep for sometime, so that comsem finishes off its job */ 
	sleep(5);

    while(1) {
        for(stanza=0; stanza < num_stanza; stanza++) {
            sleep_time = rule[stanza].write_sleep * 1000;
			lstats     = &LSTATS[stanza];
			ShutdownMask = rule[stanza].shutdown;
			/* Rules timeout is in seconds, dat needs in microseconds, Convert ... */
            dapl_instance_ptr->alarm = rule[stanza].alarm * 1000 * 1000;
				
			stats->test_id = stanza + 1;
            hxfupdate(UPDATE, stats);

            for(bsize = rule[stanza].bufmin; bsize <= rule[stanza].bufmax; bsize += rule[stanza].bufinc) {
                memcpy(patternfile, PATTERN, bsize);
                pakLen = lstats->rw_size = bsize + 2 * (TP_HDR_LEN) ;
                pattern_trailer = sbuf + bsize + TP_HDR_LEN;
				/* get the time before the operation begins */
			#ifdef __HTX_LINUX__
				clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start); 
			#else
   				read_real_time(&start, TIMEBASE_SZ);
			#endif 
	            for(i=0; ((i < rule[stanza].num_oper) || (rule[stanza].num_oper == 0)); i++) {

					if(shm_pHXECOM->SigTermFlag || shm_COMSEM->loop[WriterIdx].TERM ) { 
						hxfupdate(UPDATE, stats);
						sprintf(msg_text, "Fast socket shutdown due to server request.\n");
						WriteExit(stats, LSTATS, num_stanza, HTXERROR(EX_WRITE13,ERRNO), WriterIdx, msg_text, connect_str);
					}
                    pak = (pak+1)%(TP_MAX_SEQNOS);
					
					/*********************************************************/
					/* Populate Header and trailer in send buffer			 */ 
					/*********************************************************/
                    HDR_SEQNO(sbuf, pattern_trailer, pak);
                    HDR_TS(sbuf, pattern_trailer, timestamp);
                    HDR_CKSUM(sbuf, pattern_trailer);
					/**********************************************************/	
                    /* if debug_pattern is set put pak seq in every 128 bytes */
					/**********************************************************/
                    if(rule[stanza].debug_pattern && pakLen >= 512 && write_try == 1) {
                    	for(o=128; o < pakLen-128; o+=128) {
                        	debug_ptr = sbuf + o;
                            INSERT_PAK_SEQNO(debug_ptr, pak);
                            debug_ptr = sbuf + o + TP_SEQ_LEN;
                            INSERT_BLK_SEQNO(debug_ptr, o);
                                   
                            /* to help with retry debug, put the retry count */
                            /* in every 128+16th byte */
                            debug_ptr = sbuf + o + 16;
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
                            debug_ptr = sbuf + o + 32;
                            INSERT_PID(debug_ptr,pid);
                        }
                    }
			
					ret = dapl_rdma_write(dapl_instance_ptr, pakLen, stats);
    				if (ret != DAT_SUCCESS) {
						if(eeh_enabled) { 
							/* EEH is enabled, so Errors are expected. 
 							 * We would have retried in above functions as an expectency for 
							 * RDMA packet to flow. Coming Here if we still see error, 
							 * then IA is in bad state, just restart this writer/reader pair 
							 */ 
							break; 
						} else { /* EEH not enabled, throw error and exit. */ 
        					sprintf(msg_text, "dapl_rdma_write: Error doing RDMA WRITE - %s\n", dapl_ret_tostr(ret));
							RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE13,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
							bad_other(lstats, NULL, stats);	
							hxfupdate(UPDATE, stats);
							shm_pHXECOM->SigTermFlag=1;
							ShutdownTest(arg->RemoteServerSock, stats);
							msg_text[0] = '\0';
							WriteExit(stats, LSTATS, num_stanza, HTXERROR(EX_WRITE13,ERRNO), WriterIdx, msg_text, connect_str);
						} 
    				}

					if(sleep_time) 
						usleep(sleep_time);

					/* wait for receive indication from reader */
					ret = dapl_rdma_read(dapl_instance_ptr, pakLen, stats);

    				if (ret != DAT_SUCCESS ) {
                        if(eeh_enabled) {
                            /* EEH is enabled, so Errors are expected.
                             * We would have retried in above functions as an expectency for
                             * RDMA packet to flow. Coming Here if we still see error,
                             * then IA is in bad state, just restart this writer/reader pair
                             */
                            break;
                        } else { /* EEH not enabled, throw error and exit. */
        					sprintf(msg_text, "dapl_rdma_read: Error doing RDMA READ - %s\n", dapl_ret_tostr(ret));
							RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE13,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
							bad_other(lstats, NULL, stats); 
							hxfupdate(UPDATE, stats); 
							shm_pHXECOM->SigTermFlag=1;
                        	ShutdownTest(arg->RemoteServerSock, stats);
							msg_text[0] = '\0'; 
							WriteExit(stats, LSTATS, num_stanza, HTXERROR(EX_READ13,ERRNO), WriterIdx, msg_text, connect_str); 
						}	
					}

					/* compare receive buffer with standard send buffer, 
					 */
					if (!(rule[stanza].no_compare)) {
						if (memcmp(rbuf, sbuf, pakLen) != 0) {
							int index = -1, a = 0; 
							for(a=0;a<pakLen;a++) {
			                	if(rbuf[a] != sbuf[a]) {
                    				index = a; 
									break; 
                				}
							}

							sprintf(msg_text, "%s\nMiscompare pakLen = %d \n", connect_str, pakLen);
							if(ShutdownMask & SH_KDB_MISCOMPARE) {
							    #ifndef __HTX_LINUX__
                                    /*trap((int) MISC,(int)&pattern, (int)&rbuf,(int)&msgp,(int)&stats,(int)&rule,(int)&bsize); */
                                    trap((int) MISC, sbuf, rbuf,index,stats,rule,bsize,(int)VER,i);
    							#else
                                    /*do_trap_htx64((int) MISC,(int)&pattern, (int)&rbuf,(int)&msgp,(int)&stats,(int)&rule,(int)&bsize);*/
                                    /*              r3       r4     r5  r6     r7   r8   r9     r10 r11*/
                                    do_trap_htx64((int)MISC,sbuf, rbuf,index,stats,rule,bsize,(int)VER);
 							    #endif

                            }
                            sprintf(msg_text, "%s\nMiscompare, data size = %d. miscompare_count=%d pakLen = %d \n",
                                                                connect_str, bsize,stats->miscompare_count, pakLen );
							memset(msgp, 0 , MAX_TEXT_MSG);
                            rc = hxfcbuf(stats, sbuf, rbuf, pakLen, msgp);
                            sprintf(msg_text+strlen(msg_text), "%s\n", msgp);
							bad_other(lstats, NULL, stats);
							shm_pHXECOM->SigTermFlag=1;
                        	ShutdownTest(arg->RemoteServerSock, stats);
                            WriteExit(stats, LSTATS, num_stanza, HTXERROR(EX_READ13,ERRNO), WriterIdx, msg_text, connect_str); 
                        }
					}

					good_bytes_wrote(lstats, stats, pakLen, connect_str);
					good_bytes_read(lstats, stats, pakLen, connect_str);

                } /* end of the for loop writing num oper bzise buffers */
				/* Check, if we came out because of error due to EEH !!. 
				 * Handle the error at one place. 
				 */ 
				if(eeh_enabled && ret != DAT_SUCCESS) { 

                   
					if(++shm_pHXECOM->BrokenPipes < eeh_retries) {
				    	sprintf(msg_text, "%s\nEEH Error : Restart writer/reader = %d, EEH error count=%d\n", connect_str,WriterIdx, shm_pHXECOM->BrokenPipes);
                        RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE22,ret), HTX_HE_INFO, msg_text);
                        thetime=time(&dummy_time);
                        elasped_time=thetime-shm_pHXECOM->starttime;
                        if(elasped_time > twelve_hours) {
                        	shm_pHXECOM->BrokenPipes=0;
                            shm_pHXECOM->starttime=thetime;
                        }
                        Req_StopReader(arg->RemoteServerSock,shm_pHXECOM->SemServID, shm_HXECOM->exer[shm_pHXECOM->ExerNo].ServerID, ReaderID, stats);
						shm_COMSEM->loop[WriterIdx].TERM=1; 
                        HE_exit(0);
                   	} else {
                   		/* Too many breaks shutdown */
                   		sprintf(msg_text, "%s\nRDMA Operation failed - %s.\nToo Many errors due to EEH!! Exiting\n", connect_str, STRERROR(ret));
                   		RWmsg(lstats, &rule[stanza], stanza, stats, HTXERROR(EX_WRITE22,ret), HTX_HE_HARD_ERROR, msg_text);
                        shm_pHXECOM->SigTermFlag=1;
                        ShutdownTest(arg->RemoteServerSock, stats);
                        WriteExit(stats, LSTATS, num_stanza, HTXERROR(EX_READ13,ERRNO), WriterIdx, msg_text, connect_str);
				    }
				} 
				
		
				#ifdef __HTX_LINUX__
					clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &finish); 
					secs = finish.tv_sec - start.tv_sec; 
					n_secs = finish.tv_nsec - start.tv_nsec;  
				#else _
                    read_real_time(&finish, TIMEBASE_SZ);
                    time_base_to_time(&start, TIMEBASE_SZ);
                    time_base_to_time(&finish, TIMEBASE_SZ);
                    /* subtract the starting time from the ending time */
                    secs = finish.tb_high - start.tb_high;
                    n_secs = finish.tb_low - start.tb_low;
				#endif 
                    /*
                     * If there was a carry from low-order to high-order during
                     * the measurement, we may have to undo it.
                     */
                    if (n_secs < 0)  {
                        secs--;
                        n_secs += 1000000000;
                    }
					latency = (double)secs + ((double)n_secs / 1000000000); 
					thruput = (((double)(2 * rule[stanza].num_oper * pakLen)) / (double)latency); 
					if(thruput > (double)1024) { 
						thruput = (thruput / (double)1024); 
						unit = 'K'; 
					} 
					if(thruput > (double)1024) {
                        thruput = (thruput / (double)1024);
                        unit = 'M'; 
					} 
					if(thruput > (double)1024) {
                        thruput = (thruput / (double)1024);
                        unit = 'G'; 
					}
					shm_COMSEM->loop[WriterIdx].thruput = thruput; 	
					shm_COMSEM->loop[WriterIdx].unit = unit; 
					shm_COMSEM->loop[WriterIdx].WriterPid = getpid(); 
					if(WriterIdx == 0) { 
						int writer = 0; 
						double total = 0; 
						memset(msg_text1, 0, 2048);
						for(writer = 0; writer < rule[stanza].replicates; writer ++) { 
							total += shm_COMSEM->loop[writer].thruput; 
							sprintf(msg_text, "[%#d] WriterID = %d, thruput= %#lf %cBPS\n", shm_COMSEM->loop[writer].WriterPid, 
										writer, shm_COMSEM->loop[writer].thruput, shm_COMSEM->loop[writer].unit); 
							strncat(msg_text1, msg_text, 2048); 
						}
						
						printf("***************************\n%s\nTotal Throughput = %lf %cBPS\n***************************\n", msg_text1, total, unit); 
						fflush(stdout); 
					}
                /********************************************************/
                /* Since we are possibly changing the buffer size,      */
                /* hxfupdate must be called before bsize is changed     */
                /* again.                                               */
                /********************************************************/
                hxfupdate(UPDATE, stats);
			}	
        }
		hxfupdate(FINISH, stats);
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
		fprintf(config_des, "good reads         = %18u\n", LSTATS[stanza].totals.good_rw);
        fprintf(config_des, "bad writes         = %18u\n", LSTATS[stanza].totals.bad_rw);
		fprintf(config_des, "bad reads          = %18u\n", LSTATS[stanza].totals.bad_rw);
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


static void WriteExit(struct htx_data * sstats, struct cum_rw * LSTATS, int NoStanzas, int exitNo, int WriterIdx,
                        char msg[], char ConnectStr[])
{
   	WriteLocalStats(LSTATS, NoStanzas, ConnectStr, sstats, msg + strlen(msg) +1);
   	sprintf(msg + strlen(msg), "%s\nStats Updated. Exiting.\n", ConnectStr);
   	hxfmsg(sstats, exitNo, HTX_HE_INFO, msg);
	/* We need to wait until writer sends the TERM */ 
	if(shm_pHXECOM->SigTermFlag && shm_COMSEM->loop[WriterIdx].TERM) { 
		HE_exit(exitNo>>16);		
	} else {
		if(shm_pHXECOM->SigTermFlag && !shm_COMSEM->loop[WriterIdx].TERM) { 
			/* Our reader has not yet send TERM */ 
			int i = 0; 
			while(i <= 5 && !shm_COMSEM->loop[WriterIdx].TERM) {
				sleep(2);		 
				i++;
			}
			HE_exit(exitNo>>16);
		} 				 
		/* Can't rely, comsem may have exited before, just exit */ 
		HE_exit(exitNo>>16);
	}	
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



static void good_bytes_wrote(struct cum_rw * Lstats, struct htx_data * sstats, int bytes, char ConnectStr[]) { 
	

    if((unsigned int)bytes != Lstats->rw_size) {
		char msg_text[1024];
        sprintf(msg_text, "%s\nLast rw_size=%d, New rw_size=%d, Must be equal.\n",
                           ConnectStr, Lstats->rw_size, bytes);
        hxfmsg(sstats, HTXERROR(EX_WRITE9,0), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_WRITE9);
    }
    Lstats->totals.good_rw  += 1;
    Lstats->current.good_rw += 1;
    sstats->good_writes      += 1;
    sstats->bytes_writ       += bytes;
    addw(Lstats->totals.good_bytes_rw1, Lstats->totals.good_bytes_rw2, bytes);
    addw(Lstats->current.good_bytes_rw1, Lstats->current.good_bytes_rw2, bytes);
}

static void good_bytes_read(struct cum_rw * Lstats, struct htx_data * sstats, int bytes, char ConnectStr[]) 
{
    if((unsigned int)bytes != Lstats->rw_size) {
		char msg_text[1024];
        sprintf(msg_text, "%s\nLast rw_size=%d, New rw_size=%d, Must be equal.\n",
                             ConnectStr, Lstats->rw_size, bytes);
        hxfmsg(sstats, HTXERROR(EX_READ15,0), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_READ15);
    }
    Lstats->totals.good_rw  += 1;
    Lstats->current.good_rw += 1;
    sstats->good_reads       += 1;
    sstats->bytes_read       += bytes;
    addw(Lstats->totals.good_bytes_rw1, Lstats->totals.good_bytes_rw2, bytes);
    addw(Lstats->current.good_bytes_rw1, Lstats->current.good_bytes_rw2, bytes);
}

static void RWmsg(struct cum_rw * Lstats, struct rule_format * rule, int Stanza, struct htx_data * sstats, int err, enum sev_code sev, char * msgtxt)
{

    sprintf(msgtxt + strlen(msgtxt),
           "For stanza %d, RDMA connection,  bufmin=%d, bufmax=%d, bufinc=%d\nConsecutive prior good writes=%9u,  good bytes written=",
               Stanza, rule->bufmin, rule->bufmax, rule->bufinc, Lstats->current.good_rw);
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





static struct id_t GetRdrID(struct sockaddr_in ServerID, struct id_t SemServID,
                                 u_long WriterIdx, struct id_t WriterID, struct htx_data * stats)
{
    struct id_t ReaderID;
    int rc;
    SOCKET ToServerSock;
    struct CoordMsg  CMsg;
	char tmpaddr[30];
	char msg_text[1024]; 

    ToServerSock = SetUpConnect(&ServerID, stats, 0);

    memset(&CMsg, '\0', sizeof(CMsg));
    CMsg.msg_type = htonl(CM_REQ_RDR_ID);
    memcpy(&CMsg.ID.server, &SemServID, sizeof(struct id_t));
    HostToNetId_t(&CMsg.ID.server);
    rc = StreamWrite(ToServerSock, (char *) &CMsg, sizeof(CMsg));
    if(rc == -1) {
        sprintf(msg_text, "%s:[%s]:[%d]:: Error getting reader ID - %s\n", __FILE__,  __FUNCTION__, __LINE__,STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_WRITE5);
    }

    WriterIdx = htonl(WriterIdx);
    rc = StreamWrite(ToServerSock, (char *) &WriterIdx, sizeof(WriterIdx));
    if(rc == -1) {
        sprintf(msg_text, "%s:[%s]:[%d]:: Error getting reader ID - %s\n", __FILE__,  __FUNCTION__, __LINE__,STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE6,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_WRITE6);
    }
    HostToNetId_t(&WriterID);
    rc = StreamWrite(ToServerSock, (char *) &WriterID, sizeof(struct id_t));
    if(rc == -1) {
        sprintf(msg_text, "%s:[%s]:[%d]:: Error getting reader ID - %s\n", __FILE__,  __FUNCTION__, __LINE__,STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE7,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_WRITE7);
    }

    rc = StreamRead(ToServerSock, (char *) &CMsg, sizeof(CMsg));
    if(rc == -1 || ntohl(CMsg.msg_type) != CM_READER_ID) {
        sprintf(msg_text, "%s:[%s]:[%d]:: Error getting reader ID - %s\n", __FILE__,  __FUNCTION__, __LINE__,STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE8,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
		while(1); 
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


/********************************************************************/
/* Since there is only one instance of comsem active for each       */
/* hxecom, we can use global structure without locks for            */
/* shm_COMSEM->loop[].halt, timestamp, and last.                    */
/********************************************************************/
static void Terminating(struct htx_data * stats);
static void ResetWrite(struct semid_t ID, struct htx_data * stats);
static void TerminateWrite(struct semid_t ID, struct htx_data * stats);

static void NetToHostTriplet_t(struct mr_triplet * t) { 

	t->WriterIdx = ntohs(t->WriterIdx); 
	t->local_iov.lmr_context = ntohl(t->local_iov.lmr_context); 
	t->local_iov.virtual_address = ntohll(t->local_iov.virtual_address); 
	t->local_iov.segment_length = ntohl(t->local_iov.segment_length); 

}

void * comsem_rdma(void * Targ)
{
    struct CoordMsg  CMsg;
    SOCKET    SemServSock;
    int    i, rc = 0;
    char 	msg_text[1024];
    struct htx_data * stats;
    struct comsem_argt * arg;
    unsigned int seed;
    SOCKET msgsock;

    arg         = (struct comsem_argt * )Targ;
    SemServSock = arg->SemServSock;
    stats       = &arg->stats;

/********************************************************************/
/*                                                                  */
/********************************************************************/
    for(i=0; i<MAXREMOTE; i++) {
        shm_COMSEM->loop[i].last = -1;
        memset(shm_COMSEM->loop[i].rand_pktsize, 0, (sizeof(u_short) * USHRT_MAX));
    }

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
        sprintf(msg_text, "comsem_rdma : %d, recvd packet =%d \n",getpid(), CMsg.msg_type);
        hxfmsg(stats, 0, HTX_HE_INFO, msg_text);
    #endif
        switch(CMsg.msg_type) {
            case CM_RESET_WRITE:
                NetToHostSemid_t(&CMsg.ID.SemID);
                ResetWrite(CMsg.ID.SemID ,stats);
                break;
			case DAPL_RMR_XCHG: 
				NetToHostTriplet_t(&CMsg.ID.rmr_info); 
				RecvAddrExc(msgsock, CMsg.ID.rmr_info, stats ); 
				break; 

            case CM_TERM_WRITE:
                NetToHostSemid_t(&CMsg.ID.SemID);
                TerminateWrite(CMsg.ID.SemID, stats);
                break;

            default:
                sprintf(msg_text, "comsem: Unknown msg_type %d\n", (int)CMsg.msg_type);
                hxfmsg(stats, HTXERROR(EX_COMSEM1,0), HTX_HE_INFO, msg_text);
                break;
        }
        closesocket(msgsock);

    } while(1);

    return 0;
}

static void RecvAddrExc(SOCKET msgsock, struct mr_triplet rmr_info, struct htx_data *stats ) { 

	u_short WriterIdx ; 
	struct CoordMsg CMsg;
	char msg_text[4096];
	int rc;


	WriterIdx = rmr_info.WriterIdx; 

	/* First verify that message is intact, If yes then block this writer */
	if((rmr_info.local_iov.lmr_context == 0x00) && (rmr_info.local_iov.virtual_address == 0xFF) && (rmr_info.local_iov.segment_length == 0x00)) { 
		/* Check if Writer has halted itself before Us */ 
		if(shm_COMSEM->loop[WriterIdx].halt != 1) {
			/* Halt Writer */ 
			shm_COMSEM->loop[WriterIdx].halt = 1;
			SetSem(SemID, rmr_info.WriterIdx, 0, stats);   
		}	
		/* Writer Blocked Now, lets copy the rmr info */ 
	}

	memset(&shm_COMSEM->loop[WriterIdx].remote_iov, 0, sizeof(DAT_RMR_TRIPLET)); 
	memset(&CMsg, 0, sizeof(struct CoordMsg));
	rc = StreamRead(msgsock, (char *)&CMsg, sizeof(struct CoordMsg));   
	if((rc == -1) || (ntohl(CMsg.msg_type) != DAPL_RMR_XCHG)) { 
		sprintf(msg_text, "Error : %s: failed to recvd DAPL_RMR_XCHG from Reader = %d \n", __FUNCTION__, WriterIdx); 
		hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
		return; 
	} 
#ifdef __DEBUG__ 
	sprintf(msg_text, "%s : WriterIdx = %d, Recvd DAPL_RMR_XCHG, lmr_context = %d, virtual_address = 0x%llx \n", 
				__FUNCTION__, WriterIdx, CMsg.ID.rmr_info.local_iov.lmr_context, CMsg.ID.rmr_info.local_iov.virtual_address); 
	hxfmsg(stats, 0, HTX_HE_INFO, msg_text); 
#endif 
 	
	NetToHostTriplet_t(&CMsg.ID.rmr_info); 
	shm_COMSEM->loop[WriterIdx].remote_iov.rmr_context = CMsg.ID.rmr_info.local_iov.lmr_context; 
	shm_COMSEM->loop[WriterIdx].remote_iov.virtual_address = CMsg.ID.rmr_info.local_iov.virtual_address; 
	shm_COMSEM->loop[WriterIdx].remote_iov.segment_length = CMsg.ID.rmr_info.local_iov.segment_length; 

	/* Now send our local recv_iov info, this is going to be used for recv RDMA data. */ 
	memset(&CMsg, 0, sizeof(struct CoordMsg));
	CMsg.msg_type = htonl(DAPL_RMR_XCHG);
    CMsg.ID.rmr_info.WriterIdx = htons((u_short) 0xffff  & WriterIdx);
	CMsg.ID.rmr_info.local_iov.lmr_context = htonl(shm_COMSEM->loop[WriterIdx].recv_iov.lmr_context); 
	CMsg.ID.rmr_info.local_iov.virtual_address = htonll(shm_COMSEM->loop[WriterIdx].recv_iov.virtual_address); 
	CMsg.ID.rmr_info.local_iov.segment_length = htonl(sizeof(DAT_LMR_TRIPLET));
#ifdef __DEBUG__
	sprintf(msg_text, "%s, WriterIdx = %d,  Sending DAPL_RMR_XCHG, lmr_context = %d, virtual_address = 0x%llx \n", 
						__FUNCTION__, WriterIdx, shm_COMSEM->loop[WriterIdx].recv_iov.lmr_context, 
							shm_COMSEM->loop[WriterIdx].recv_iov.virtual_address); 
	hxfmsg(stats, 0, HTX_HE_INFO, msg_text);
#endif

	rc = StreamWrite(msgsock, (char *)&CMsg, sizeof(CMsg));
	if(rc == -1) {
    	sprintf(msg_text, "Error : %s: Sending DAPL_RMR_XCHG to SemServSock - %s \n", __FUNCTION__, STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        return;
    }
	
	/* Expecting ACK from Reader so that we can unblock writer */ 
	memset(&CMsg, 0, sizeof(struct CoordMsg)); 
	rc = StreamRead(msgsock, (char *)&CMsg, sizeof(CMsg));
	if(rc == -1) {
   		sprintf(msg_text, "Error : %s: Receiving RMR info from writer - %s \n", __FUNCTION__, STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        return;
    }


	if(ntohl(CMsg.msg_type) != DAPL_RMR_XCHG_ACK) { 
		sprintf(msg_text, "Error : %s: Receiving DAPL_RMR_XCHG_ACK from writer - %s, CMsg.msg_type = %#llx \n", __FUNCTION__, STRERROR(errno), ntohl(CMsg.msg_type)); 
		hxfmsg(stats, HTXERROR(EX_WRITE5,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
		return; 
	}
#ifdef __DEBUG__
	sprintf(msg_text, "%s Recvd DAPL_RMR_XCHG_ACK, Unblocking Writer \n", __FUNCTION__); 
	hxfmsg(stats, 0, HTX_HE_INFO, msg_text);
#endif 
	/* Unblock Writer Now */ 
	SetSem(SemID, WriterIdx, WRITEAHEAD, stats);	
	shm_COMSEM->loop[WriterIdx].halt = 0; 	
	return; 
}  

static void ResetWrite(struct semid_t ID, struct htx_data * stats)
{
	char msg_text[1024];

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
    sprintf(msg_text,
       "comsem: Received reset for writer %d - i=%d, timestamp=%d, stanza=%d.\nSetting write_ahead=%d, pak=%d, pid = %d, SemID= %d\n",
             (int)ID.WriteIdx, (int)ID.Iloop, (int)ID.timeS, (int)ID.Stanza, (int)ID.WriteAhead, (int)ID.ackNo, getpid(), SemID);
    hxfmsg(stats, HTXERROR(EX_COMSEM2,0), HTX_HE_INFO, msg_text);
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

static void TerminateWrite(struct semid_t ID, struct htx_data * stats)
{
/*
    sprintf(msg_text, "TerminatWrite called: \n");
    hxfmsg(stats, HTXERROR(EX_COMSEM3,0), HTX_HE_INFO, msg_text);
*/
    shm_COMSEM->loop[(int)ID.WriteIdx].halt = 1;
    shm_COMSEM->loop[(int)ID.WriteIdx].TERM = 1;
    shm_COMSEM->loop[(int)ID.WriteIdx].CheckReset = 1;
    UpSem(SemID, (int)ID.WriteIdx, stats);
}



static void Terminating(struct htx_data * stats)
{
   int i;

/*
    sprintf(msg_text, "Writer Terminating called: \n");
    hxfmsg(stats, HTXERROR(EX_COMSEM3,0), HTX_HE_INFO, msg_text);
*/
   /* This guarantees the writer will see flags and exit */
   for(i=0; i<shm_COMSEM->NoWriters; i++)
        shm_COMSEM->loop[i].CheckReset = 1;
       SetSem(SemID, i, 1, stats);
   HE_exit(0);
}





static int GetUniqueWIdx(struct htx_data * stats)
{
   	int No, i = 0;
	char msg_text[1024]; 
    GlobalWait(WRITE_SEM,stats);
   	No = shm_COMSEM->NoWriters++;
    GlobalSignal(WRITE_SEM, stats);

   	if(No >= MAXREMOTE) {
		/* Before declaring this as error let me check, if
		 * I can use older Writer Id's that have already exited. 
		 */ 
		if(shm_pHXECOM->BrokenPipes < eeh_retries) { 
			GlobalWait(WRITE_SEM,stats);
			for(i = 0; i < MAXREMOTE; i++) { 
				if(shm_COMSEM->loop[i].TERM) { 
					No=i; 
					(void) sprintf(msg_text, "GetUniqueWIdx: Too many writer processes.Using Old writerId = %d \n",i); 
					(void) hxfmsg(stats, 0, HTX_HE_INFO, msg_text);
					shm_COMSEM->loop[i].TERM = 0;  
					break; 
				} 
			}
			GlobalSignal(WRITE_SEM, stats); 
		} else { 
      		(void) sprintf(msg_text, "GetUniqueWIdx: Too many writer processes. There are probably restarts of writers due to EEH. No = %d, BrokenPipes = %d, eeh_retries = %d\n", 
				No, shm_pHXECOM->BrokenPipes, eeh_retries);
      		(void) hxfmsg(stats, HTXERROR(EX_COMSEM7,0), HTX_HE_SOFT_ERROR, msg_text);
      		HE_exit(EX_COMSEM7);
   		}
	}
   return No;
}

static void ExitCleanup()
{
    static int cleanup_in_progress = FALSE;

    if (!cleanup_in_progress) {
        cleanup_in_progress = TRUE;
        Detach_COMSEM_RDMA();
		dapl_cleanup_instance(dapl_instance_ptr);
    }
}

void Detach_COMSEM_RDMA(void)
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


void InitWriteVarsRdma(struct htx_data *stats)
{
    int i;
	char msg_text[1024];

/********************************************************************/
/* Setup comsem memory to share with writers.                       */
/********************************************************************/
    if((shm_pHXECOM->mID_COMSEM = shmget(IPC_PRIVATE, sizeof(struct shm_comsem_t),
                        IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                                                     S_IROTH | S_IWOTH)) == -1) {
        sprintf(msg_text, "InitWriteVars: Error getting shared memory - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_COMSEM17,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_COMSEM17);
    }
    if((int)(shm_COMSEM = (struct shm_comsem_t *) shmat(shm_pHXECOM->mID_COMSEM, 0, 0)) ==  -1) {
        sprintf(msg_text, "InitWriteVars: Error attaching shared memory - %s\n", STRERROR(errno));
        hxfmsg(stats, HTXERROR(EX_COMSEM18,ERRNO), HTX_HE_SOFT_ERROR, msg_text);
        HE_exit(EX_COMSEM18);
    }
    shm_COMSEM->mID = shm_pHXECOM->mID_COMSEM;
    SemID = DefineMySems(shm_pHXECOM->ExerNo);
}


static void hex_dump(unsigned char *data, int size)
{
    /* dumps size bytes of *data to stdout. Looks like:
     * [0000] 75 6E 6B 6E 6F 77 6E 20
     *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
     * (in a single line of course)
     */

    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }
            
        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) { 
            /* line completed */
            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}


