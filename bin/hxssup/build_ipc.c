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

/* @(#)41	1.5.4.9  src/htx/usr/lpp/htx/bin/hxssup/build_ipc.c, htx_sup, htxfedora 5/29/15 09:44:14 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    build_ipc.c                                           */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  Build LINUX IPC structures for use by HTX system      */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Allocates and initializes LINUX IPC structures for HTX*/
/*                                                                           */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200   */
/*                     -Nt2000                                               */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/


#include "hxssup.h"
#include <scr_info.h>

#ifdef	__HTX_LINUX__
#include "cfgclibdef.h"
#else
#include "cfgclibdef.h"
#endif

#include <sys/types.h>
#include <unistd.h>

/*
 * Define all other un-supported signals as SIGUNUSED
 */

/* Defining SIGUNUSED if not, since deprecated with glibc >= 2.26*/
#ifndef SIGUNUSED
#define SIGUNUSED 31
#endif

#ifdef	__HTX_LINUX__
#define	SIGMAX	(SIGRTMAX)
#define	SIGEMT	(SIGSEGV)
#define	SIGSYS	(SIGUNUSED)
#define	SIGMSG	(SIGUNUSED)
#define	SIGDANGER	(SIGUNUSED)
#define	SIGMIGRATE	(SIGUNUSED)
#define	SIGGRANT	(SIGUNUSED)
#define	SIGRETRACT	(SIGUNUSED)
#define	SIGSOUND	(SIGUNUSED)
#define	SIGSAK	(SIGUNUSED)

#endif

char default_mdt_snz[4096];	/* ptr to default mdt stanza    */

extern int cfgcrdsz(CFG__SFT *, char *, int, char *);
extern int cfgcsvdflt(CFG__SFT *, char *);
extern int cfgcskwd(char *, char *, char *);
extern CFG__SFT *cfgcopsf(char *);
extern int cfgcclsf(CFG__SFT *);
extern union semun semctl_arg;

extern void hotplug_reconfig_restart(int sig, int code, struct sigcontext *scp);

tecg_struct *ecg_info;
//int cur_ecg_pos = 0;
int wof_test_enabled = 0, equaliser_debug_flag = 0;
char cfg_file[30];

extern int MAX_EXER_ENTRIES;
extern int equaliser_flag;
extern volatile int system_call;

int sem_length;

/*****************************************************************************/
/*****  p r o c e s s _ m d t _ t o _ s h m  *********************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     process_mdt_to_shm()                                  */
/*                                                                           */
/* DESCRIPTIVE NAME =  Get attributes from MDT and set in HE share memory    */
/*                                                                           */
/* FUNCTION =          Reads the next stanza, merges it with the default     */
/*                     stanza (if passed in) and sets the values in the      */
/*                     HE shared memory structure based on the attributes    */
/*                     from the MDT file.                                    */
/*                                                                           */
/* INPUT =             ( O) stanza - output buffer for next stanza.          */
/*                     (I ) stanza_length - size of stanza buffer.           */
/*                     (I ) shm_point - points to target HE shm structure    */
/*                     (IO) default_stanza - pointer to pointer to default   */
/*                          stanza.  See notes.                              */
/*                     (I ) mdt_fd - cfg file pointer to MDT attribute file. */
/*                                                                           */
/* OUTPUT =            stanza - stanza just read.                            */
/*                     default_stanza - points to null pointer.              */
/*                                                                           */
/* NORMAL RETURN =     0 to indicate successful completion                   */
/*                                                                           */
/* ERROR RETURNS =     see cfgcrdsz.                                         */
/*                                                                           */
/* EXTERNAL REFERENCES =                                                     */
/*                                                                           */
/*    OTHER ROUTINES = cfgcrdza - reads a stanza from the MDT attribute file.*/
/*                     cfgcszdflt - sets (overrides) default stanza.         */
/*                     unquote -     strips double quotation marks (") off   */
/*                                   of character strings (misc.c)           */
/*                                                                           */
/*    DATA AREAS = indirectly by parameter.                                  */
/*                                                                           */
/*    NOTES: The parameter default_stanza is a (char **) so that the value   */
/*           which it points to may be set to NULL.  This allows the         */
/*           pointer to the default stanza which is to be used to be passed  */
/*           in and saved (cfgcsvdflt()) on the first call and ignored on    */
/*           subsequent calls.  This means that the same calling sequence    */
/*           may be used in a loop and the default stanza is set only on     */
/*           the first call.                                                 */
/*                                                                           */
/*           The *default_stanza pointer is always set to NULL.  It is only  */
/*           used if there is no default stanza present in the attribute     */
/*           file.                                                           */
/*                                                                           */
/*           An empty attribute file will result in CFG_EOF being returned.  */
/*                                                                           */
/*           The base assumption is that there is a device stanza in the     */
/*           attribute file.                                                 */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*    PSEUDOCODE:                                                            */
/*                                                                           */
/*    IF (default_stanza == NULL) or (*default_stanza == NULL)               */
/*        READ next stanza                                                   */
/*    ELSE                                                                   */
/*        READ default stanza                                                */
/*        SWITCH (rc)                                                        */
/*            OK:                                                            */
/*              READ next stanza                                             */
/*            STANZA_NOT_FOUND:                                              */
/*              Rewind the MDT file                                          */
/*              SET the default stanza using cfgcsvdflt()                    */
/*              READ next stanza                                             */
/*            DEFAULT: nothing to do                                         */
/*        SET *default_stanza = NULL                                         */
/*    ENDIF                                                                  */
/*    IF (rc != OK)                                                          */
/*      RETURN rc                                                            */
/*    SET HE shared memory from the attributes in the stanza                 */
/*    RETURN rc                                                              */
/*                                                                           */
/*****************************************************************************/

int	process_mdt_to_shm(char *stanza, int stanza_length,
		       union shm_pointers shm_point, char **default_stanza,
		       CFG__SFT * mdt_fd)
{
	char	workstr[512];	/* work string                  */
	int	rc;			/* general return code          */
	char	*shm_work_ptr;	/* char pointer into shm        */
	char	*stanza_ptr;	/* pointer into stanza          */

	if (default_stanza == NULL || *default_stanza == NULL)  {
		/*
		 * read the next stanza
		 */
		rc = cfgcrdsz(mdt_fd, stanza, stanza_length, (char *) NULL);
	}

	else {
		rc = cfgcrdsz(mdt_fd, stanza, stanza_length, "default");
		switch (rc) {
			case CFG_SUCC:	/* default found, read next stanza */
				rc = cfgcrdsz(mdt_fd, stanza, stanza_length, (char *) NULL);
				break;

			case CFG_SZNF:	/* not found, rewind, set it, and get next */
				cfgcrwnd(mdt_fd);
				 cfgcsvdflt(mdt_fd, *default_stanza);
				rc = cfgcrdsz(mdt_fd, stanza, stanza_length, (char *) NULL);
				break;

			default:	/* handle problem later */
				break;
		}		/* switch */

		*default_stanza = NULL;	/* don't want to see it again */
	}			/* if */

	/* get next stanza */
	if (rc != CFG_SUCC)  {
		return (rc);	/* no next stanza or some other problem */
	}

	/*
	 * Now setup the HE entry from the stanza that is in hand
	 */

	/* set Hardware Exerciser Name */
	cfgcskwd("HE_name", stanza, workstr);
	 htx_strcpy(shm_point.HE_addr->HE_name, unquote(workstr));

	/* set Adapter Description */
	cfgcskwd("adapt_desc", stanza, workstr);
	 htx_strcpy(shm_point.HE_addr->adapt_desc, unquote(workstr));

	/* set Device Description */
	cfgcskwd("device_desc", stanza, workstr);
	 htx_strcpy(shm_point.HE_addr->device_desc, unquote(workstr));

	/* set EMC Rules File Name */
	cfgcskwd("emc_rules", stanza, workstr);
	if( (strlen(workstr) ) >= (RULE_NAME_LENGTH_LIMIT -1) ) {
		return RULE_NAME_LENGTH_EXCEEDED ;
	}
	 htx_strcpy(shm_point.HE_addr->emc_rules, unquote(workstr));

	/* set REG Rules File Name */
	cfgcskwd("reg_rules", stanza, workstr);
	if( (strlen(workstr) ) >= (RULE_NAME_LENGTH_LIMIT -1) ) {
		return RULE_NAME_LENGTH_EXCEEDED ;
	}
	 htx_strcpy(shm_point.HE_addr->reg_rules, unquote(workstr));

        shm_point.HE_addr->dup_device = 0;
        shm_point.HE_addr->log_vpd = 0;


	/* set "/dev/????" Special File Name */
	shm_work_ptr = &(shm_point.HE_addr->sdev_id[0]);
	stanza_ptr = &(stanza[0]);

	do {
		*shm_work_ptr = *stanza_ptr;
		shm_work_ptr++;
		stanza_ptr++;
	} while (*stanza_ptr != ':');	/* enddo */

	*shm_work_ptr = '\0';	/* terminating 0 for text string    */

	/* set DMA Channel */
	cfgcskwd("dma_chan", stanza, workstr);
	shm_point.HE_addr->dma_chan = (unsigned short) atoi(unquote(workstr));

	/* set hft number */
#ifdef HTX_REL_tu320
	cfgcskwd("hft", stanza, workstr);
	workint = (unsigned short) atoi(unquote(workstr));
	if ((workint < 0) || (workint > 3))  {
		shm_point.HE_addr->hft_num = 0;
	}
	else  {
		shm_point.HE_addr->hft_num = workint;
	}
#endif

	/* set Idle Time */
	cfgcskwd("idle_time", stanza, workstr);
	shm_point.HE_addr->idle_time = (unsigned short) atoi(unquote(workstr));

	/* set Interrupt Level */
	cfgcskwd("intrpt_lev", stanza, workstr);
	shm_point.HE_addr->intrpt_lev = (unsigned short) atoi(unquote(workstr));

	/* set Load Sequence */
	cfgcskwd("load_seq", stanza, workstr);
	shm_point.HE_addr->load_seq = (unsigned short) atoi(unquote(workstr));

	/* set Maximum Run Time */
	cfgcskwd("max_run_tm", stanza, workstr);
	shm_point.HE_addr->max_run_tm = (unsigned short) atoi(unquote(workstr));

	/* set Port Number */
	cfgcskwd("port", stanza, workstr);
	shm_point.HE_addr->port = (unsigned short) atoi(unquote(workstr));

	/* set Priority */
	cfgcskwd("priority", stanza, workstr);
	shm_point.HE_addr->priority = (unsigned short) atoi(unquote(workstr));

	/* set Slot Number */
	cfgcskwd("slot", stanza, workstr);
	shm_point.HE_addr->slot = (unsigned short) atoi(unquote(workstr));

	/* clear PID Number */
	shm_point.HE_addr->PID = (int) NULL;

	/* set Max cycles */
	cfgcskwd("max_cycles", stanza, workstr);
	shm_point.HE_addr->max_cycles = (unsigned int) atoi(unquote(workstr));

	/* set Continue on Error Flag */
	cfgcskwd("cont_on_err", stanza, workstr);
	unquote(workstr);
	if ((workstr[0] == 'y') || (workstr[0] == 'Y'))
		shm_point.HE_addr->cont_on_err = 1;
	else {
		shm_point.HE_addr->cont_on_err = 0;
	}			/* endif */

	/* set Halt on Error Severity Code Level */
	cfgcskwd("halt_level", stanza, workstr);
	unquote(workstr);
	if (htx_strcmp(workstr, "0") == 0)  {
		shm_point.HE_addr->halt_sev_level = 0;
	}
	else if (htx_strcmp(workstr, "1") == 0)  {
		shm_point.HE_addr->halt_sev_level = 1;
	}
	else if (htx_strcmp(workstr, "2") == 0)  {
		shm_point.HE_addr->halt_sev_level = 2;
	}
	else if (htx_strcmp(workstr, "3") == 0)  {
		shm_point.HE_addr->halt_sev_level = 3;
	}
	else if (htx_strcmp(workstr, "4") == 0)  {
		shm_point.HE_addr->halt_sev_level = 4;
	}
	else if (htx_strcmp(workstr, "5") == 0)  {
		shm_point.HE_addr->halt_sev_level = 5;
	}
	else if (htx_strcmp(workstr, "6") == 0)  {
		shm_point.HE_addr->halt_sev_level = 6;
	}
	else if (htx_strcmp(workstr, "7") == 0)  {
		shm_point.HE_addr->halt_sev_level = 7;
	}
	else if (htx_strcmp(workstr, "HTX_HE_HARD_ERROR") == 0)  {
		shm_point.HE_addr->halt_sev_level = HTX_HE_HARD_ERROR;
	}
	else if (htx_strcmp(workstr, "HTX_HE_SOFT_ERROR") == 0)  {
		shm_point.HE_addr->halt_sev_level = HTX_HE_SOFT_ERROR;
	}
	else  {
		shm_point.HE_addr->halt_sev_level = HTX_HE_HARD_ERROR;
	}

	/* set error acknowledged flag */
	shm_point.HE_addr->err_ack = 1;

	/* set start_halted flag */
	/* If HE set to halt on startup, set the appropriate semaphone
	   AFTER the shared memory has been sorted. */
	cfgcskwd("start_halted", stanza, workstr);
	unquote(workstr);
	if ((workstr[0] == 'y') || (workstr[0] == 'Y'))  {
		shm_point.HE_addr->start_halted = 1;
	}
	else  {
		shm_point.HE_addr->start_halted = 0;
	}

	/* set virtual terminal number */
#ifdef HTX_REL_tu320
	shm_point.HE_addr->VT_num = 0;
#endif

	/* set Time of Last Error */
	shm_point.HE_addr->tm_last_err = 0;

	/* set Time of Last call to HTX_update() */
	shm_point.HE_addr->tm_last_upd = -1;

	/* set Number of Other Bad Operations */
	shm_point.HE_addr->bad_others = 0;

	/* set Number of Bad Reads */
	shm_point.HE_addr->bad_reads = 0;

	/* set Number of Bad Writes */
	shm_point.HE_addr->bad_writes = 0;

	/* set Number of Bytes Read */
	shm_point.HE_addr->bytes_read = 0;

	/* set Number of Bytes Written */
	shm_point.HE_addr->bytes_writ = 0;

	/* set Number of Other Good Operations */
	shm_point.HE_addr->good_others = 0;

	/* set Number of Good Reads */
	shm_point.HE_addr->good_reads = 0;

	/* set Number of Good Writes */
	shm_point.HE_addr->good_writes = 0;

	return (rc);		/* should always be CFG_SUCC here */

}				/* process_mdt_to_shm */

/*****************************************************************************/
/*****  b u i l d _ i p c ( )  ***********************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     build_ipc()                                           */
/*                                                                           */
/* DESCRIPTIVE NAME =  Build LINUX IPC structures for use by HTX system        */
/*                                                                           */
/* FUNCTION =          Allocates and initializes LINUX IPC structures (message */
/*                     queue, semaphore, and shared memory) as defined by    */
/*                     the Master Device Table (normally the attribute file  */
/*                     "/usr/lpp/htx/mdt/mdt")                               */
/*                                                                           */
/* INPUT =             The Master Device Table (normally                     */
/*                     "/usr/lpp/htx/mdt/mdt")                               */
/*                                                                           */
/* OUTPUT =            The required HTX LINUX IPC structures                   */
/*                                                                           */
/* NORMAL RETURN =     0 to indicate successful completion                   */
/*                                                                           */
/* ERROR RETURNS =     10 - Unable to open mdt file                          */
/*                     11 - No entries in the mdt file                       */
/*                     12 - Error reading mdt file                           */
/*                     13 - Unable to close mdt file                         */
/*                                                                           */
/*                     20 - Unable to allocate IPC message queue             */
/*                                                                           */
/*                     30 - Unable to allocate IPC semaphore structure       */
/*                                                                           */
/*                     40 - Unable to allocate IPC shared memory segment     */
/*                     41 - Unable to attach IPC shared memory segment       */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = DRT_compare - used in qsort to order the entries in   */
/*                                   the shared memory segment (misc.c)      */
/*                     unquote -     strips double quotation marks (") off   */
/*                                   of character strings (misc.c)           */
/*                     process_mdt_to_shm - interprets mdt attributes and    */
/*                                   constructs the shared memory structure  */
/*                                   for each stanza.                        */
/*    DATA AREAS =     HTXPATH - pointer to a string which contains the path */
/*                         to the top level of the HTX file hierarchy        */
/*                         (usually "/usr/lpp/htx").                         */
/*                     MSGLOGKEY message queue (identified by the msgqid     */
/*                        variable)                                          */
/*                     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                     SEMHEKEY semaphores (identified by the semhe_id       */
/*                        variable)                                          */
/*                                                                           */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/

int	build_ipc(void)
{
	CFG__SFT *mdt_fd;	/* pointer to MDT attribute file */

	char msg[128];		/* message string               */
	char stanza[4096];	/* attribute file stanza        */
	char tag[40];		/* add-on char string           */
	char     workstr[128];
	char *mdt_name;

	extern int MAX_ADDED_DEVICES;	/* shm_HE entries for new devs  */
	extern int profile_emc_mode;	/* default mode setting in htx_profile */
	/* from hxssup.c                */

	extern int msgqid;	/* Message Log message queue id    */
	extern int semhe_id;	/* HE semaphore id              */
	extern int shm_id;	/* system shared memory id      */

	extern union shm_pointers shm_addr;	/* shared memory address      */
	extern unsigned int max_wait_tm;	/* maximum semop wait time      */

	int i;			/* general counter              */
        int index;                      /* index into shared memory     */
	int rc, retcod;			/* return code                  */
	int workint;		/* work integer                 */

	struct sigaction sigvector;	/* structure for signals    */
	struct sigaction old_SIGHUP_vector; /* old structure for signals */

	union shm_pointers shm_point;	/* shared memory address        */

	unsigned int max_entries;	/* max number of entries in shm */
	unsigned int num_entries;	/* current number of entries in shm  */

       /*
        ***  beginning of executable code  **********************************
        */
        shm_addr.hdr_addr = (struct htxshm_hdr *) 0;  /* no shm yet.         */
        init_rem_ipc();

	/*
	 * open MDT cfg file
	 */
	mdt_fd = cfgcopsf("../mdt/mdt");
	if (mdt_fd == (CFG__SFT *) NULL) {
		PRTMSG(0, 0, ("ERROR: Unable to open MDT file."));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		return (10);
	}

	/* endif */
	for ((num_entries = (unsigned int) 0); ((rc = cfgcrdsz(mdt_fd, stanza, sizeof(stanza), (char *) NULL)) == CFG_SUCC); (num_entries++))
		;	/* get number of MDT entries */

	if (rc == CFG_EOF) {	/* normal EOF? */
		if (num_entries == (unsigned int) 0) {	/*  no entries? */
			PRTMSG(0, 0, ("ERROR: No entries in the MDT."));
			PRTMSG(MSGLINE, 0,("Press any key to continue (program will end)..."));
			 getch();
			return (11);
		}

		else  {
			max_entries = num_entries + EXTRA_ENTRIES;	/* room for extras   */
		}

		/* endif */
	}

	else {		/* Problem with MDT.                 */
		 sprintf(msg, "ERROR: MDT file read failure CFG_");
		switch (rc) {
			case CFG_SZNF:	/* req stanza not found */
				 sprintf(tag, "SZNF");
				break;

			case CFG_SZBF:	/* stanza too long      */
				 sprintf(tag, "SZBF");
				break;

			case CFG_UNIO:	/* I/O error            */
				 sprintf(tag, "UNIO");
				break;

			default:	/* unexpected error     */
				 sprintf(tag, "????");
				break;
		}		/* endswitch */

		 htx_strcat(msg, tag);
		 sprintf(tag, ", rc = %d.", rc);
		PRTMSG(0, 0, (htx_strcat(msg, tag)));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		return (12);
	}			/* endif */

	/* close MDT cfg file */
	if (cfgcclsf(mdt_fd) != CFG_SUCC) {
		PRTMSG(0, 0, ("ERROR: Unable to close MDT file."));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		CLRLIN(MSGLINE, 0);
		return (13);
	}	/* endif */

	/*
	 * allocate message queue structure
	 */
	msgqid = msgget(MSGLOGKEY, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);	/* Message Log queue              */
	if (msgqid == -1) {	/* problem?                          */
		PRTMSG(0, 0, ("ERROR: Unable to get Message Log message queue."));
		PRTMSG(1, 0, ("       errno = %d.", errno));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		return (20);
	}	/* endif */


	/*
	 * allocate semaphore structures
	 */
	sem_length = ((int) (max_entries + MAX_ADDED_DEVICES) * SEM_PER_EXER) + 6;
	semhe_id = semget(SEMHEKEY, sem_length, IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);	/* HE semaphores                   */
	if (semhe_id == -1) {	/* problem?                          */
		PRTMSG(0, 0, ("ERROR: Unable to get HE semaphore structure."));
		PRTMSG(1, 0, ("       errno = %d.", errno));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		return (30);
	}	/* endif */

	/*
	 * allocate shared memory
	 */

	shm_id = shmget(HTXSHMKEY, (int) (sizeof(struct htxshm_hdr) + ((max_entries + PSEUDO_EXTRA_ENTRIES + MAX_ADDED_DEVICES) * sizeof(struct htxshm_HE))), IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |S_IROTH | S_IWOTH);	/* system shared memory              */
	if (shm_id == -1) {	/* problem?                          */
		PRTMSG(0, 0, ("ERROR: Unable to get shared memory."));
		PRTMSG(1, 0, ("       errno = %d.", errno));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		return (40);
	}	/* endif */

	/*
	 * attach shared memory
	 */
	shm_addr.hdr_addr = (struct htxshm_hdr *) shmat(shm_id, (char *) 0, 0);
	if ((int) shm_addr.hdr_addr == -1) {	/* problem? */
		PRTMSG(0, 0, ("ERROR: Unable to attach shared memory."));
		PRTMSG(1, 0, ("       errno = %d.", errno));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		return (41);
	}	/* endif */

	/*
	 * set up signal processing
	 */
	for (i = 1; i <= SIGMAX; i++) {
		sigemptyset(&(sigvector.sa_mask)); /* do not block signals */
		sigvector.sa_flags = 0;	/* do not restart syscalls on sigs */

		switch (i) {
			case SIGHUP:	/* hangup */
				sigvector.sa_handler = SIG_IGN;
				break;

			case SIGINT:	/* interrupt (rubout)  */
				sigvector.sa_handler = SIG_IGN;
				break;

			case SIGQUIT:	/* quit (ASCII FS)     */
				sigvector.sa_handler = SIG_IGN;
				break;

			case SIGILL:	/* illegal instruction */
				sigvector.sa_handler = (void (*)(int)) sig_end;
				break;

			case SIGTRAP:	/* trace trap          */
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGABRT:	/* abort process       */
				sigvector.sa_handler = SIG_DFL;
				break;

#ifndef	__HTX_LINUX__
			case SIGEMT:	/* EMT instruction     */
				sigvector.sa_handler = SIG_DFL;
				break;
#endif

			case SIGFPE:	/* floating point exception    */
			//	sigvector.sa_handler = (void (*)(int)) sig_end;
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGKILL:	/* kill (cannot be caught)     */
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGBUS:	/* bus error                   */
				sigvector.sa_handler = (void (*)(int)) sig_end;
				break;

			case SIGSEGV:	/* segmentation violation      */
				sigvector.sa_handler = (void (*)(int)) sig_end;
				break;

			case SIGSYS:	/* bad argument to system call */
				sigvector.sa_handler = (void (*)(int)) sig_end;
				break;

			case SIGPIPE:	/* write on a pipe with no one t */
				sigvector.sa_handler = (void (*)(int)) sig_end;
				break;

			case SIGALRM:	/* alarm clock                 */
				sigvector.sa_handler = (void (*)(int)) alarm_signal;
				break;

			case SIGTERM:	/* software termination signal */
				sigvector.sa_handler = (void (*)(int)) sig_end;
				break;

			case SIGURG:	/* urgent condition on I/O channel */
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGSTOP:	/* stop (cannot be caught or ignored) */
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGTSTP:	/* interactive stop */
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGCONT:	/* continue (cannot be caught or ignored) */
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGCHLD:	/* sent to parent on child stop or exit */
				sigvector.sa_handler = (void (*)(int)) child_death;
				sigvector.sa_flags = SA_RESTART;
				break;

			case SIGTTIN:	/* background read attempted from control terminal */
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGTTOU:	/* background write attempted to control terminal */
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGIO:	/* I/O possible or completed */
				sigvector.sa_handler = SIG_IGN;
				break;

			case SIGXCPU:	/* cpu time limit exceeded (see setrlimit())  */
				sigvector.sa_handler = SIG_DFL;
				break;

			case SIGXFSZ:	/* file size limit exceeded (see setrlimit()) */
				sigvector.sa_handler = SIG_DFL;
				break;

#ifndef	__HTX_LINUX__
			case SIGMSG:	/* input data is in the hft ring buffer */
				sigvector.sa_handler = SIG_IGN;
				break;
#endif

			case SIGWINCH:	/* window size change */
				sigvector.sa_handler = SIG_IGN;
				break;

			case SIGPWR:	/* power-fail restart          */
				sigvector.sa_handler = (void (*)(int)) sig_end;
				break;

			case SIGUSR1:	/* user defined signal         */
				sigvector.sa_handler = (void (*)(int)) sig_end;
				break;

			case SIGUSR2:	/* user defined signal         */
				sigvector.sa_handler = (void (*)(int)) hotplug_reconfig_restart;
				break;

			case SIGPROF:	/* profiling time alarm (see setitimer) */
				sigvector.sa_handler = SIG_DFL;
				break;

#ifndef	__HTX_LINUX__
			case SIGDANGER:	/* system crash imminent (maybe) */
				sigvector.sa_handler = (void (*)(int)) sig_end;
				break;
#endif

			case SIGVTALRM:	/* virtual time alarm (see setitimer) */
				sigvector.sa_handler = SIG_DFL;
				break;

#ifndef	__HTX_LINUX__
			case SIGMIGRATE:	/* migrate process (see TCF) */
				sigvector.sa_handler = SIG_DFL;
				break;
#endif

#ifndef	__HTX_LINUX__
			case SIGGRANT:	/* monitor mode granted        */
				sigvector.sa_handler = SIG_IGN;
				break;
#endif

#ifndef	__HTX_LINUX__
			case SIGRETRACT:	/* monitor mode retracted      */
				sigvector.sa_handler = SIG_IGN;
				break;
#endif

#ifndef	__HTX_LINUX__
			case SIGSOUND:	/* sound ack                   */
				sigvector.sa_handler = SIG_IGN;
				break;
#endif

#ifndef	__HTX_LINUX__
			case SIGSAK:	/* secure attention key */
				sigvector.sa_handler = SIG_IGN;
				break;
#endif

			default:
				sigvector.sa_handler = SIG_DFL;
		}		/* endswitch */

		 sigaction(i, &sigvector, (struct sigaction *) NULL);

	}			/* endfor */

	/*
	 * initialize semaphores
	 */
	for (i = 0; i < (((int) (max_entries + MAX_ADDED_DEVICES) * SEM_PER_EXER) + 6); i++)
		 semctl_arg.val = 0;
		 semctl(semhe_id, i, SETVAL, semctl_arg);

	/*
	 * initialize shared memory area
	 */
	bzero(shm_addr.hdr_addr, (int) (sizeof(struct htxshm_hdr) + ((max_entries + PSEUDO_EXTRA_ENTRIES + MAX_ADDED_DEVICES) * sizeof(struct htxshm_HE))));

	/* initialize header flags */
	shm_addr.hdr_addr->emc = profile_emc_mode; /* set from htx_profile */

	/* set initial # entries values */
	shm_addr.hdr_addr->max_entries = max_entries;
	shm_addr.hdr_addr->num_entries = num_entries;
	shm_addr.hdr_addr->pseudo_entry_0 = max_entries + MAX_ADDED_DEVICES;

	/* clear hft_devices  (hft information structure) array */
#ifdef HTX_REL_tu320
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 32; j++) {
			shm_addr.hdr_addr->hft_devices[i].fileid[j] = 0;
			shm_addr.hdr_addr->hft_devices[i].hf_devid[j] = 0;
		}		/* endfor */
	}			/* endfor */
#endif

	/*
	 * initialize a section of shared memory for each MDT stanza
	 */

	/* re-open MDT cfg file */
	mdt_fd = cfgcopsf("../mdt/mdt");
	if (mdt_fd == (CFG__SFT *) NULL) {
		PRTMSG(0, 0, ("ERROR: Unable to open MDT file."));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		CLRLIN(MSGLINE, 0);
		return (10);
	}	/* endif */

	/*
	 * read the default stanza and save it
	 */
	rc = cfgcrdsz(mdt_fd, default_mdt_snz, sizeof(default_mdt_snz), "default");

	if (rc == CFG_SUCC) {
		/* set Equaliser flag */
		retcod = cfgcskwd("equaliser_flag", default_mdt_snz, workstr);
		if(retcod ==  CFG_SUCC) {
			equaliser_flag = atoi(unquote(workstr));
		}
		else {
			equaliser_flag = 0;
		}
		mdt_name = (char *)getenv("MDTFILE");
		if ((equaliser_flag == 1) || (strcmp(mdt_name, "mdt.maxpwr") == 0 )){
			putenv("EQUALISER_FLAG=1");
			/* Read config file name */
			retcod = cfgcskwd("cfg_file", default_mdt_snz, workstr);
			if(retcod ==  CFG_SUCC) {
				strcpy(cfg_file, unquote(workstr));
			}
			else {
				strcpy(cfg_file, " ");
			}
			/*  Read equaliser_debug_flag  */
			retcod = cfgcskwd("equaliser_debug_flag", default_mdt_snz, workstr);
			if(retcod ==  CFG_SUCC) {
				equaliser_debug_flag = atoi(unquote(workstr));
			}
			else {
				equaliser_debug_flag = 0;
			}
			/* Check if WOF testing is enabled */
			retcod = cfgcskwd("wof_test", default_mdt_snz, workstr);
			if(retcod ==  CFG_SUCC) {
				wof_test_enabled = atoi(unquote(workstr));
				if (wof_test_enabled == 1) {
					rc = do_the_bind_proc(getpid());
					if (rc < 0) {
						PRTMSG(MSGLINE, 0, ("Unable to bind hxssup process to core 0. \n"));
					}
				}
			}
		} else {
			system_call = TRUE;
			system("unset EQUALISER_FLAG");
			system_call = FALSE;
		}
	}

	/*
	 * read each stanza and set values in shm
	 */
	if (rc == CFG_SUCC)
		for ((shm_point.hdr_addr = shm_addr.hdr_addr + 1), index=0; ((rc = process_mdt_to_shm(stanza, sizeof(stanza), shm_point, (char **) NULL, mdt_fd)) == CFG_SUCC); (shm_point.HE_addr++),index++) {	/* check/reset maximum time allowed on a semop system wait */
			workint = (shm_point.HE_addr->max_run_tm * 2) + shm_point.HE_addr->idle_time;
                        shm_point.HE_addr->idle_time;
                        strcpy(EXER_NAME(index),shm_point.HE_addr->sdev_id);
                        ECGEXER_SHMKEY(index) = HTXSHMKEY;
                        ECGEXER_SEMKEY(index) = SEMHEKEY;
			if (workint > max_wait_tm) {	/* new maximum wait time? */
				max_wait_tm = workint;
			}	/* endif */
		}	/* endfor */
                NUM_EXERS = index;

	if (rc != CFG_EOF) {	/* not normal EOF?                   */
		 sprintf(msg, "ERROR: MDT file read failure CFG_");
		switch (rc) {
			case CFG_SZNF:	/* req stanza not found         */
				 sprintf(tag, "SZNF");
				break;

			case CFG_SZBF:	/* stanza too long              */
				 sprintf(tag, "SZBF");
				break;

			case CFG_UNIO:	/* I/O error                    */
				 sprintf(tag, "UNIO");
				break;

			case RULE_NAME_LENGTH_EXCEEDED:
				  sprintf(msg, "ERROR: MDT file read failure\nrule file name exceeded for exerciser <%s>", shm_point.HE_addr->HE_name);
				  sprintf(tag, " ");
				  break;

			default:	/* unexpected error             */
				 sprintf(tag, "????");
				break;
		}		/* endswitch */

		 htx_strcat(msg, tag);
		 sprintf(tag, ", rc = %d.", rc);
		PRTMSG(0, 0, (htx_strcat(msg, tag)));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		CLRLIN(MSGLINE, 0);
		return (12);
	}	/* endif */

	/*
	 * close MDT cfg file
	 */
	if (cfgcclsf(mdt_fd) != CFG_SUCC) {
		PRTMSG(0, 0, ("ERROR: Unable to close MDT file."));
		PRTMSG(MSGLINE, 0, ("Press any key to continue (program will end)..."));
		 getch();
		CLRLIN(MSGLINE, 0);
		return (13);
	}		/* endif */

	/*
	 * sort shared memory
	 */
	shm_point.hdr_addr = shm_addr.hdr_addr + 1;

#ifdef	__HTX_LINUX__
#include <stdlib.h>
	qsort((void *) shm_point.HE_addr, (size_t) num_entries, (size_t) sizeof(struct htxshm_HE), (int (*)(const void *, const void *)) DRT_compar);
#else
	qsort((char *) shm_point.HE_addr, num_entries, sizeof(struct htxshm_HE), DRT_compar);
#endif

	/*
	 * Now that the shared memory has been sorted, we can check for any
	 * HE's that are set to be halted on startup and set the appropriate
	 * semaphore value to accomplish this.
	 */
	shm_point.hdr_addr = shm_addr.hdr_addr + 1;
	for (i = 0; i < num_entries; i++) {
		if ((shm_point.HE_addr + i)->start_halted == 1) {
			 semctl_arg.val = 1 ;
			 semctl(semhe_id, ((i * SEM_PER_EXER) + 6), SETVAL, semctl_arg);
		}		/* endif */
	}			/* endfor */

	return (0);

}
/* build_IPC() */



int init_rem_ipc()
{
  int rem_shm_id;  /* system shared memory id      */
  tmisc_shm *rem_shm_addr;
  char *result_msg;
  int shm_id;

  rem_shm_addr = (tmisc_shm *)malloc(sizeof(tmisc_shm));
  rem_shm_addr->sock_hdr_addr = (tsys_hdr *) 0;  /* no shm yet. */

  rem_shm_id = shmget(REMSHMKEY, 4096,IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |S_IROTH | S_IWOTH);
  if (rem_shm_id == -1) {     /* problem?                          */
     sprintf(result_msg,"init_rem_ipc:Error creating shared memory Errno = %d \n",errno);
     printf("Error creating shared memory Errno = %d \n",errno);
     return (40);
 }       /* endif */

  rem_shm_addr->sock_hdr_addr = (tsys_hdr  *) shmat(rem_shm_id, (char *) 0, 0);
  if ((int) rem_shm_addr->sock_hdr_addr == -1) {    /* problem? */
     sprintf(result_msg,"init_rem_ipc:Error Attaching to shared memory errno=%d\n",errno);
     printf("Error Attaching to shared memory(%d, 0x%x). errno=%d\n",rem_shm_id,rem_shm_id, errno);
     return (41);
  }       /* endif */
  rem_shm_addr->sock_hdr_addr->cur_shm_key = HTXSHMKEY;
  rem_shm_addr->sock_hdr_addr->max_exer_entries = MAX_EXER_ENTRIES;
  rem_shm_addr->cur_shm_addr =  (tcurr_info *)malloc(sizeof(tcurr_info));

  ecg_info = (tecg_struct *) malloc(80*sizeof(tecg_struct));
  if (ecg_info == NULL)  /* problem?                          */
    {
    sprintf(result_msg,"####ERROR: Unable to Malloc for ecg_info.###ERRNO = %d (%s)",errno,strerror(errno));
    return(45);
    } /* endif */


  shm_id = shmget(SHMKEY, MAX_EXER_ENTRIES*sizeof(texer_list), IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);  /* system shared memory */
  if (shm_id == -1)
    { /* problem?                          */
    sprintf(result_msg,"####ERROR: Unable to get shared memory for exers.###ERRNO= %d (%s)",errno,strerror(errno));
    printf("#######ERROR: Unable to get shared memory.#######Errno = %d (%s)",errno,strerror(errno)); fflush(stdout);
    return (42);
    } /* endif */

  /* attach shared memory */
  ecg_info[0].exer_list = (texer_list *) shmat(shm_id, (char *) 0, 0);
  /*printf("init_rem_ipc: addr = 0x%x id = %d\n",ecg_info,shm_id); fflush(stdout);*/
  if ((int) ecg_info[0].exer_list ==  -1) { /* problem?                       */
    sprintf(result_msg,"Unable to attach shared memory(%d, 0x%x). Errno = %d (%s)\n",shm_id, shm_id, errno, strerror(errno));
    printf("Unable to attach shared memory(%d, 0x%x). Errno = %d (%s)\n",shm_id,shm_id, errno, strerror(errno)); fflush(stdout);
    return (43);
    } /* endif */
  /*exer_ipcaddr->exer_num = 0;*/
  return 0;
}

