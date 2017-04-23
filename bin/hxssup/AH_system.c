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

/* @(#)34	1.4.4.3  src/htx/usr/lpp/htx/bin/hxssup/AH_system.c, htx_sup, htxfedora 8/5/14 04:12:42 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    AH_system.c                                           */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  Activate/Halt system                                  */
/*                                                                           */
/* COPYRIGHT =         (C) COPYRIGHT IBM CORPORATION 1988                    */
/*                     LICENSED MATERIAL - PROGRAM PROPERTY OF IBM           */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Activates/Halts the HTX system including all hardware */
/*                     exercisers defined in the mdt.                        */
/*                                                                           */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200   */
/*                     -Nt2000                                               */
/*                                                                           */
/* CHANGE ACTIVITY =                                                         */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                  */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM */
/*    06/14/88:1.0  :J BURGESS :INITIAL RELEASE                              */
/*    11/08/99:1.23 :R GEBHARDT:Feature 290676 - Add/Restart/Term device     */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*                                                                           */
/*****************************************************************************/

#include "hxssup.h"
#include <stdlib.h>
#include <unistd.h>

/*
 * For Linux compatibility
 */
#define	SIGMAX	(SIGRTMAX)

/*
 ***  Externals global to this module **************************************
 */
extern union shm_pointers shm_addr;	/* shared memory union pointers      */
extern int semhe_id;		/* semaphore id                      */
extern char HTXPATH[];		/* HTX file system path         */
extern int load_exerciser(struct htxshm_HE *p_HE, struct sigaction *sigvec);
extern union semun semctl_arg;
extern int start_hotplug_monitor(void);
extern pid_t hotplug_monitor_ID;
extern int sem_length;

/*
 * The following sembuf structure must be global for correct child death
 * processing.
 */

/*****************************************************************************/
/*****  A H _ s y s t e m ( )  ***********************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     AH_system.c                                           */
/*                                                                           */
/* DESCRIPTIVE NAME =  Activate/Halt the htx system                          */
/*                                                                           */
/* FUNCTION =          Activates/Halts the htx system (all hardware          */
/*                     exercisers defined in the mdt)                        */
/*                                                                           */
/* INPUT =             autoflag - auto startup flag.                         */
/*                     mdt attribute file (/usr/lpp/htx/mdt/mdt).            */
/*                                                                           */
/* OUTPUT =            Loads hardware exerciser programs on start-up         */
/*                                                                           */
/*                     Updated system semaphore structure to activate/halt   */
/*                     all hardware exerciser programs                       */
/*                                                                           */
/* NORMAL RETURN =     N/A - void function                                   */
/*                                                                           */
/* ERROR RETURNS =     N/A - void function                                   */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = load_compar() - compares mdt entries for load         */
/*                                     sequence sort                         */
/*                     send_message() - outputs a message to the screen and  */
/*                                     the message log file                  */
/*                     load_exerciser() - runs setup scripts and spawns      */
/*                                     hardware exerciser.                   */
/*                                                                           */
/*    DATA AREAS =     HTXPATH - pointer to a string which contains the path */
/*                         to the top level of the HTX file hierarchy        */
/*                         (usually "/usr/lpp/htx").                         */
/*                     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                     SEMHEKEY semaphores (identified by the semhe_id       */
/*                        variable)                                          */
/*                     stress_dev - pointer to a string which contains the   */
/*                         name of the "heartbeat" device for stress lab     */
/*                         testing.  A null string means that no "heartbeat" */
/*                         is require -- non-stress test.                    */
/*                     stress_cycle - pointer to a string which contains the */
/*                         number of seconds between "heartbeats".           */
/*                         A null string means that no "heartbeat" is        */
/*                         required.                                         */
/*                                                                           */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/

void AH_system(int autoflag)
{
	char workstr[512];	/* work string                       */

/*	char workstr2[64];	work string                       */
/*	char workstr3[64];	work string                       */
/*	char workstr4[64];	work string                       */
/*	char workstr5[64];	work string                       */

	extern int alarm_sig;	/* set to TRUE on receipt of SIGALRM */
	/* function to compare load seq      */
	extern int HANG_MON_PERIOD;	/* hang monitor time period (sec) */
        extern  int random_ahd;         /* To randomly activate and halt exercisers.  */

	extern  int equaliser_flag;           /* Flag to start equaliser process      */
	extern int wof_test_enabled;          /* Flag to check if WOF testing is enabled in equaliser */
	extern pid_t hang_mon_PID;	/* Hang monitor process PID */
	extern pid_t hxstats_PID;	/* statistics program process id     */
	extern pid_t random_ahd_PID;    /* random ahd process id  */
    extern  pid_t equaliser_PID;          /* Equaliser process id  */
    extern struct tm start_time;	/* System start time.                */
	extern unsigned int max_wait_tm; /* max allowed semop wait time       */

	int errno_save = 0;		/* errno save area                   */
	int frkpid = 0;		/* fork PID                          */
	int i = 0;			/* loop counter                      */
	int j = 0;			/* loop counter                      */
	int num_entries = 0;	/* local number of entries variable  */
	int rc = 0;			/* general return code               */
	int semval = 0;		/* semaphore value                   */

	long clock = 0;		/* current time in seconds.          */

	int delay_counter;
	int delay_slice = 2;
	int exer_system_halted;
	int exer_error_halted;
	int exer_operation_halted;
	int exer_exited;
	int exer_count;
	int exer_shm_entries;
	int all_exer_stopped;

#ifdef HTX_REL_tu320
	char display_str[64];	/* display device name               */
	int hf_devid = 0;		/* hf device id */
	int hft = 0;		/* hft number */
	int hft_fileid = 0;		/* hft file descriptor. */
	int tcseta_rc = 0;		/* return code for TCSETA ioctl()    */
	int VT[4];		/* virtual terminal array of indexes */

#ifdef	__HTX_LINUX__
#include <termios.h>
#include <sys/ioctl.h>
	struct termios get_io, put_io;	/* termio structures.                */
#else
	struct termio get_io, put_io;	/* termio structures.                */
#endif

	static struct hfchgdsp hfchgdsp = {  HFINTROESC, HFINTROLBR, HFINTROEX, 0, 0, 0, sizeof(struct hfchgdsp) - 3, HFCHGDSPCH, HFCHGDSPCL, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  };
#endif

	static struct sembuf halt_sops[1] = {	/* halt system semaphore operation */
		{0, 1, SEM_UNDO}
	};

#ifdef HTX_REL_tu320
	struct hftgetid hfid;	/* get id structure */

	struct hftsmgrcmd mgrcmd;	/* command struct to screen manager. */
#endif

	struct htxshm_HE *p_HE;	/* pointer to HE struct in shm       */
	struct load_tbl *load_tbl_ptr;	/* pointer to load sequence table    */
	struct semid_ds sembuffer;	/* semaphore buffer                  */
	struct sigaction sigvector;	/* structure for signal specs */
	union shm_pointers shm_addr_wk;	/* shared memory union pointers      */

	if ((shm_addr.hdr_addr)->started == 0) {	/* first time system started?  */
		/*
		 * get system startup time
		 */
		clock = time((long *) 0);
		start_time = *(localtime(&clock));

		/*
		 * Send "System started" message
		 */
		if (autoflag == 0) {
			send_message("System started by Operator request.", 0, HTX_SYS_INFO, HTX_SYS_MSG);
		}

		else {
			send_message("System started by AUTOSTART option.", 0, HTX_SYS_INFO, HTX_SYS_MSG);
		}

		/*
		 * Look for display devices and open virtual terminals
		 */

		num_entries = shm_addr.hdr_addr->num_entries;

#ifdef HTX_REL_tu320

		for (hft = 0; hft < 4; hft++)  {
			VT[hft] = 0;	/* # of vir terms = 0 */
		}

		p_HE = (struct htxshm_HE *) (shm_addr.hdr_addr + 1);	/* skip header */
		for (i = 0; i < num_entries; i++) {
			display_str[0] = '\0';
			if (htx_strncmp(p_HE->sdev_id, "gem", (size_t) 3) == 0)  {
				 htx_strcpy(display_str, "hispd3d");	/* Gemini */
			}
			else if (htx_strncmp(p_HE->sdev_id, "ppr", (size_t) 3) == 0)  {
				 htx_strcpy(display_str, "ppr");	/* Pedenales / Lega Family */
			}
			else if (htx_strncmp(p_HE->sdev_id, "sab", (size_t) 3) == 0)  {
				 htx_strcpy(display_str, "hiprfmge");	/* Sabine */
			}
			else if (htx_strncmp(p_HE->sdev_id, "sga", (size_t) 3) == 0)  {
				 htx_strcpy(display_str, "sga");	/* Salmon Graphics Adapter */
			}
			else if (htx_strncmp(p_HE->sdev_id, "skyc", (size_t) 4) == 0)  {
				 htx_strcpy(display_str, "gda");	/* Skyway -- Color */
			}
			else if (htx_strncmp(p_HE->sdev_id, "skym", (size_t) 4) == 0)  {
				 htx_strcpy(display_str, "gda");	/* Skyway -- Monochrome */
			}
			else if (htx_strncmp(p_HE->sdev_id, "wga", (size_t) 3) == 0)  {
				 htx_strcpy(display_str, "wga");	/* Skyway -- Monochrome */
			}
			else if (htx_strncmp(p_HE->sdev_id, "bbl", (size_t) 3) == 0)  {
				 htx_strcpy(display_str, "bbl");	/* baby blue */
			}
			else if (htx_strncmp(p_HE->sdev_id, "rby", (size_t) 3) == 0)  {
				 htx_strcpy(display_str, "rby");	/* baby blue */
			}
			else if (htx_strncmp(p_HE->sdev_id, "nep", (size_t) 3) == 0)  {
				 htx_strcpy(display_str, "nep");	/* baby blue */
			}
			else if (htx_strncmp(p_HE->sdev_id, "mag", (size_t) 3) == 0)  {
				 htx_strcpy(display_str, "mag");	/* Magenta */
			}

			if (display_str[0] != '\0') {	/* display device ? */
				htx_strcat(display_str, strpbrk(p_HE->sdev_id, "0123456789"));
				if ((hf_devid = get_dispid(display_str)) <  0) {
					sprintf(workstr, "Unable to get display_id for %s (mdt entry:%s).\nget_dispid() rc = %d.", display_str, p_HE->sdev_id, hf_devid);
					send_message(workstr, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
					sprintf(workstr, "Unable to get display_id for %s (mdt entry:%s).", display_str, p_HE->sdev_id);
					PRTMSG(MSGLINE, 0, ("%s", workstr));
					getch();
				}

				else if ((hft_fileid = open("/dev/hft", O_RDWR)) == -1) {
					sprintf(workstr, "Unable to open hft for %s.  errno = %d.", p_HE->sdev_id, errno);
					send_message(workstr, errno, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
					PRTMSG(MSGLINE, 0, ("%s", workstr));
					getch();
				}

				else {
					sprintf(workstr, "display_id for %s (mdt entry:%s) = %x.", display_str, p_HE->sdev_id, hf_devid);
					send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
					hft = p_HE->hft_num;
					tcseta_rc = -1;

					/* save line discipline settings */
					if (ioctl(hft_fileid, TCGETA, &get_io) != 0) {
						sprintf(workstr, "Unable to get termio structure for %s.  errno = %d.", p_HE->sdev_id, errno);
						send_message(workstr, errno, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
						PRTMSG(MSGLINE, 0, ("%s", workstr));
						getch();
					}

					else {
						put_io = get_io;
						put_io.c_oflag &= ~OPOST;

						/* turn off OPOST */
						if ((tcseta_rc = ioctl(hft_fileid, TCSETA, &put_io)) != 0) {
							sprintf(workstr, "Unable to set termio structure for %s.  errno = %d.", p_HE->sdev_id, errno);
							send_message(workstr, errno, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
							PRTMSG(MSGLINE, 0, ("%s", workstr));
							getch();
						}
					}

					shm_addr.hdr_addr->hft_devices[hft].fileid[VT[hft]] = hft_fileid;
					shm_addr.hdr_addr->hft_devices[hft].hf_devid[VT[hft]] = hf_devid;
					p_HE->VT_num = VT[hft];
					VT[hft]++;

					/* Assign virtual terminal to proper hf_devid */
					hfchgdsp.hf_mode[0] = HFNONDEF;
					hfchgdsp.hf_devid[0] = hf_devid >> 24;
					hfchgdsp.hf_devid[1] = hf_devid >> 16;
					hfchgdsp.hf_devid[2] = hf_devid >> 8;
					hfchgdsp.hf_devid[3] = hf_devid;

					if (write(hft_fileid, (char *) &hfchgdsp, sizeof(hfchgdsp)) == -1) {
						sprintf(workstr, "Unable assign virtual terminal to 0x%x for %s.  errno = %d.", hf_devid, p_HE->sdev_id, errno);

						send_message(workstr, errno, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
						PRTMSG(MSGLINE, 0, ("%s", workstr));
						getch();
					}	/* endif */

					if (tcseta_rc == 0) {	/*successful TCSETA ioctl call before? */
						if (ioctl(hft_fileid, TCSETA, &get_io) != 0) {
							sprintf(workstr, "Unable to restore termio structure for %s.  errno = %d.", p_HE->sdev_id, errno);
							send_message(workstr, errno, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
							PRTMSG(MSGLINE, 0, ("%s", workstr));
							getch();
						}	/* endif */
					}	/* endif */
				}	/* endif */
			}	/* endif */

			p_HE++;
		}		/* endfor */

		/*
		 * Assign virtual terminal file id to ktsm/gio devices
		 */

		p_HE = (struct htxshm_HE *) (shm_addr.hdr_addr + 1);	/* skip header */
		for (i = 0; i < num_entries; i++) {
			if ((htx_strncmp(p_HE->sdev_id, "gio", (size_t) 3) ==  0)
			    || (htx_strncmp(p_HE->sdev_id, "lpf", (size_t) 3) == 0)
			    || (htx_strncmp(p_HE->sdev_id, "kbd", (size_t) 3) == 0)
			    || (htx_strncmp(p_HE->sdev_id, "dials", (size_t) 5) == 0)
			    || (htx_strncmp(p_HE->sdev_id, "mous", (size_t) 4) == 0)
			    || (htx_strncmp(p_HE->sdev_id, "tab", (size_t) 3) == 0)
			    || (htx_strncmp(p_HE->sdev_id, "sal_kbd", (size_t) 7)  == 0)
			    || (htx_strncmp(p_HE->sdev_id, "sal_mous", (size_t) 8) == 0)
			    || (htx_strncmp(p_HE->sdev_id, "sal_tab", (size_t) 7) == 0)) {
				hft = p_HE->hft_num;
				if (shm_addr.hdr_addr->hft_devices[hft].fileid[0] == 0) {
					rc = open("/dev/hft", O_RDWR);
					if (rc == -1) {
						 sprintf(workstr, "Unable to open hft for %s.  errno = %d.", p_HE->sdev_id, errno);
						 send_message(workstr, errno, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
						 PRTMSG(MSGLINE, 0, ("%s", workstr));
						 getch();
					}

					else {
						shm_addr.hdr_addr->hft_devices[hft].fileid[0] = rc;
						p_HE->VT_num = 0;
					}	/* endif */
				}	/* endif */
			}	/* endif */
			p_HE++;
		}		/* endfor */

		/*
		 * if this program is running on an hft, make sure that
		 * this program has the active vertual terminal.
		 */

		rc = ioctl(0, HFTGETID, &hfid);
		if ((long) hfid.hf_chan < 0)  {
			rc = -1;
		}

		if (rc == 0) {	/* hft display? */
			/* Make this program's virtual terminal the ACTIVE terminal. */
			mgrcmd.hf_cmd = SMACT;
			mgrcmd.hf_vsid = 0;
			mgrcmd.hf_vtid = hfid.hf_chan;
			if (ioctl(0, HFTCSMGR, &mgrcmd) != 0) {
				 sprintf(workstr, "ioctl(0, HF(T)CSMGR , &mgrcmd) call failed.  errno = %d.", errno);
				 send_message(workstr, errno, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
				 PRTMSG(MSGLINE, 0, ("%s", workstr));
				 getch();
			}	/* endif */
		}	/* endif */
#endif

		/*
		 * allocate space for load sequence table
		 */
		load_tbl_ptr = (struct load_tbl *) calloc((unsigned) num_entries, sizeof(struct load_tbl));
		if (load_tbl_ptr == NULL) {
			sprintf(workstr, "Unable to allocate memory for load sequence table.  errno = %d.", errno);
			PRTMSG(MSGLINE, 0, ("%s", workstr));
			send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
			return;
		}	/* endif */

		/*
		 * build load sequence table
		 */
		shm_addr_wk.hdr_addr = shm_addr.hdr_addr;	/* copy addr to work space */
		(shm_addr_wk.hdr_addr)++;	/* skip over header */
		for (i = 0; i < num_entries; i++) {	/* build tbl */
			(load_tbl_ptr + i)->shm_pos = i;
			(load_tbl_ptr + i)->load_seq = (shm_addr_wk.HE_addr + i)->load_seq;
		}		/* endfor */

#ifdef	__HTX_LINUX__
#include <stdlib.h>
		qsort((void *) load_tbl_ptr, (size_t) num_entries, (size_t) sizeof(struct load_tbl), (int (*)(const void *, const void *)) load_compar);
#else
		qsort((char *) load_tbl_ptr, (size_t) num_entries, (size_t) sizeof(struct load_tbl), load_compar);
#endif

		 fcntl(fileno(stdin), F_SETFD, 1);	/* stdin to close on exec  */
		 fcntl(fileno(stdout), F_SETFD, 1);	/* stdout to close on exec */
		 fcntl(fileno(stderr), F_SETFD, 1);	/* stderr to close on exec */

		/*
		 * set up sigvector structure
		 */
		sigvector.sa_handler = SIG_DFL;		/* set default action */
		sigemptyset(&(sigvector.sa_mask));	/* do not block other signals      */
		sigvector.sa_flags = 0;			/* No special flags */


		/*
		 * load HE's
		 */
		for (i = 0; i < num_entries; i++) {
			p_HE = shm_addr_wk.HE_addr + (load_tbl_ptr + i)->shm_pos;
			rc = load_exerciser(p_HE, &sigvector);
		}		/* endfor */

		free((char *) load_tbl_ptr);	/* free load table memory */
		(shm_addr.hdr_addr)->started = 1; /* show system started */

		/*
		 * load program to periodically record system statistics
		 */
		frkpid = fork();

		switch (frkpid) {
			case 0:	/* child process                     */
				for (j = 1; j <= SIGMAX; j++)  {	/* set default signal processing   */
					sigaction(j, &sigvector, (struct sigaction *) NULL);
				}

				/*
				 * change the process group so we don't get
				 * signals meant for the supervisor program
				 */
				setpgid(getpid(), getpid());

				htx_strcpy(workstr, HTXPATH);
				htx_strcat(workstr, "/bin/hxstats");

				if ((execl(workstr, "hxstats", "/tmp/htxstats", "30", (char *) 0)) == -1) {
					sprintf(workstr, "Unable to load hxstats program.  errno = %d", errno);
					PRTMSG(MSGLINE, 0, ("%s", workstr));
					send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
					exit(errno);
				}	/* endif */
				break;

			case -1:	/* problem with fork() call          */
				sprintf(workstr, "Unable to fork for hxstats program.  errno = %d", errno);
				PRTMSG(MSGLINE, 0, ("%s", workstr));
				send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
				break;

			default:	/* parent process                  */
				hxstats_PID = frkpid;
				if (wof_test_enabled) { /* Bind to core any thread of core 0 */
				    rc = do_the_bind_proc(hxstats_PID);
				    if (rc < 0) {
						sprintf(workstr, "Unable to bind hxstats process to core 0.\n");
						send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
					}
				}
				break;

		}		/* endswitch */


		/* start hotplug monitor only in case wof_test_enabled flag is set to 0
		 * as During WOF test, equaliser process is making cpu go offline, which invokes
		 * hotplug handler and un-necessiarly fills up htxmsg file.
		 */
		if (wof_test_enabled == 0) {
			hotplug_monitor_ID = start_hotplug_monitor();
		    if(hotplug_monitor_ID == -1) {
			    sprintf(workstr, "Unable to fork for hotplug monitor process.  errno = %d", errno);
			    PRTMSG(MSGLINE, 0, ("%s", workstr));
			    send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
		    }
	    }


		/*
		 * fork process to watch for hung exercisers
		 */

		if (HANG_MON_PERIOD > 0) {
			frkpid = fork();

			switch (frkpid) {

				case 0:	/* child process                     */
					/* Let HE's do 1st hxfupdate...    */

					sleep((HANG_MON_PERIOD > 5) ? HANG_MON_PERIOD : 5);
					hang_monitor();
					exit(0);
					break;

				case -1:	/* problem with fork() call */
					sprintf(workstr, "Unable to fork for hang_monitor process.  errno = %d", errno);
					PRTMSG(MSGLINE, 0, ("%s", workstr));
					send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
					break;

				default:	/* parent process */
					hang_mon_PID = frkpid;
					if (wof_test_enabled) { /* bind to any thread of core 0 */
					    rc = do_the_bind_proc(hang_mon_PID);
					    if (rc < 0) {
							sprintf(workstr, "Unable to bind hang_monitor process to core 0.\n");
							send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
						}
					}
					break;
			}	/* endswitch */
		}	/* endif */

		/* fork the process to randomly activate or halt the exercisers except mem */

                if (random_ahd != 0)  {
                        frkpid = fork();
                        switch (frkpid)  {

                                case 0:  /* child process       */
                                        /*  let exercisers do 1st hxfupdate   */

                                        sleep(5);
                                        rand_ahd();
                                        exit(0);
                                        break;

                                case -1:     /* problem with fork()   */
                                        sprintf(workstr, "Unable to fork for random_ahd process.  errno = %d", errno);
                                        PRTMSG(MSGLINE, 0, ("%s", workstr));
                                        send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
                                        break;

                                default:        /* parent process */
                                        random_ahd_PID =  frkpid;
                                        if (wof_test_enabled) { /* bind to any thread of core 0 */
                                            rc = do_the_bind_proc(random_ahd_PID);
										    if (rc < 0) {
												sprintf(workstr, "Unable to bind random_ahd process to core 0.\n");
												send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
											}
                                        }
                                        break;
                        }       /* endswitch     */
                }    /* endif   */

		/* fork the process for equaliser   */
		if (equaliser_flag != 0) {
			frkpid = fork();
			switch (frkpid)  {
				case 0:  /* child process       */
					/*  let exercisers do 1st hxfupdate   */
					equaliser();
					exit(0);
					break;
				case -1:     /* problem with fork()   */
					sprintf(workstr, "Unable to fork for equaliser process.  errno = %d", errno);
					PRTMSG(MSGLINE, 0, ("%s", workstr));
					send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
					break;

				default:        /* parent process */
					equaliser_PID = frkpid;
					if (wof_test_enabled) { /* bind to any thread of core 0 */
					    rc = do_the_bind_proc(equaliser_PID);
					    if (rc < 0) {
							sprintf(workstr, "Unable to bind equaliser process to core 0.\n");
							send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
						}
					}
					break;
			}       /* endswitch     */
	}

		/*
		 * set semaphore to show all HE's exec'd
		 */
		errno = 0;	/* clear errno. */
		semctl_arg.val = 1;
		if (semctl(semhe_id, 3, SETVAL, semctl_arg) != 0) {
			errno_save = errno;
			sprintf(workstr, "Error trying to set \"All HE's started\" semaphore to 1.\nerrno = %d (%s).", errno_save, strerror(errno_save));
			send_message(workstr, errno_save, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
		}	/* endif */

		fcntl(fileno(stdin), F_SETFD, 0);	/* stdin NOT to close on exec */
		fcntl(fileno(stdout), F_SETFD, 0);	/*stdout NOT to clse on exec */
		fcntl(fileno(stderr), F_SETFD, 0);	/*stderr NOT to clse on exec */

	}

	else {		/* system already started            */
		semval = semctl(semhe_id, 0, GETVAL, &sembuffer);

		switch (semval) {
			case -1:	/* error getting sem value          */
				sprintf(workstr, "Unable to get Global Stop/Start semaphore value.  errno = %d.", errno);
				PRTMSG(MSGLINE, 0, ("%s", workstr));
				send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
				return;

			case 0:	/* system active                    */
				semop(semhe_id, halt_sops, (unsigned int) 1);
				PRTMSG(MSGLINE, 0, ("Waiting for all Hardware Exercisors to stop..."));
				send_message("Operator has requested a system halt.", 0, HTX_SYS_INFO, HTX_SYS_MSG);

				exer_shm_entries = shm_addr.hdr_addr->max_entries;

				semctl_arg.array = (ushort*) malloc(sem_length * sizeof(ushort) );
				if(semctl_arg.array == NULL) {
					sprintf(workstr, "Unable to allocate memory for semctl GETVAL array.  errno = %d.", errno);
					PRTMSG(MSGLINE, 0, ("%s", workstr));
					send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
					return;
				}


				p_HE = (struct htxshm_HE *) (shm_addr.hdr_addr + 1);
				all_exer_stopped = delay_counter = 0;
				while(delay_counter < max_wait_tm) {
					memset(semctl_arg.array, 0, (sem_length * sizeof(ushort) ) );

					semctl(semhe_id, 0, GETALL, semctl_arg);

					exer_count = exer_system_halted = exer_error_halted = exer_operation_halted = exer_exited = 0;
					while(exer_count < exer_shm_entries) {
						if(semctl_arg.array[(exer_count * SEM_PER_EXER + SEM_GLOBAL + 2)] == 1) {
							exer_system_halted++;
						} else if(semctl_arg.array[(exer_count * SEM_PER_EXER + SEM_GLOBAL + 1)] == 1) {
							exer_error_halted++;
						} else if(semctl_arg.array[(exer_count * SEM_PER_EXER + SEM_GLOBAL )] == 1) {
							exer_operation_halted++;
						} else if( (p_HE + exer_count) -> PID == 0) {
							exer_exited++;
						} else {
							break;
						}
						exer_count++;

					}
					if(exer_shm_entries  <= (exer_system_halted + exer_error_halted + exer_operation_halted + exer_exited) ) {

						all_exer_stopped = 1;
						break;
					}

					sleep(delay_slice);
					delay_counter += delay_slice;
				}

				if ( all_exer_stopped == 0) {
					PRTMSG(MSGLINE, 0, ("Warning: Not all HE's stopped within allowed time.  System will continue..."));
					send_message("Warning: Not all of the HE's stopped within the allowed time.\nSystem will continue...", 0, HTX_SYS_INFO, HTX_SYS_MSG);
				}

				else {
					PRTMSG(MSGLINE, 0, ("All Hardware Exercisors stopped."));
					send_message("System halted by Operator request.", 0, HTX_SYS_INFO, HTX_SYS_MSG);
				}	/* endif */
				free(semctl_arg.array);
				semctl_arg.array = NULL;
				break;

			case 1:	/* system halted                    */
				semctl_arg.val = 0;
				semctl(semhe_id, 0, SETVAL, semctl_arg);
				PRTMSG(MSGLINE, 0, ("System activated."));

				send_message("System re-activated by Operator request.", 0, HTX_SYS_INFO, HTX_SYS_MSG);
				break;
		}		/* endswitch */

	}			/* endif */

	return;
}				/* AH_system() */


