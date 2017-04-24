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

/* @(#)46	1.7.4.1  src/htx/usr/lpp/htx/bin/hxssup/hang_monitor.c, htx_sup, htxubuntu 12/16/14 03:57:47 */

/*
 *   FUNCTIONS: hang_monitor
 *              hang_sig_end
 *
 */


/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    hang_monitor.c                                        */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  Monitors HE's for hang condition.                     */
/*                                                                           */
/* COPYRIGHT =         (C) COPYRIGHT IBM CORPORATION 1992                    */
/*                     LICENSED MATERIAL - PROGRAM PROPERTY OF IBM           */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/*                                                                           */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200   */
/*                     -Nt2000                                               */
/*                                                                           */
/*****************************************************************************/

#include <unistd.h>
#include "hxssup.h"

#ifdef	__HTX_LINUX__
#define	SIGMAX	(SIGRTMAX)
#endif


/*****************************************************************************/
/*****  h a n g _ m o n i t o r ( )  *****************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     hang_monitor()                                        */
/*                                                                           */
/* DESCRIPTIVE NAME =  Monitors HE's for hang condition.                     */
/*                                                                           */
/* OUTPUT =            Error messages in HTX error log for hangs.            */
/*                                                                           */
/* NORMAL RETURN =     0 to indicate successful completion                   */
/*                                                                           */
/* ERROR RETURNS =     -1 to indicate no hardware exerciser programs         */
/*                         currently in memory.                              */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = send_message - outputs a message to the screen and    */
/*                                    the message log file                   */
/*                                                                           */
/*    DATA AREAS =     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                     SEMHEKEY semaphores (identified by the semhe_id       */
/*                        variable)                                          */
/*                                                                           */
/*****************************************************************************/

void	hang_monitor(void)
{
	char workstr[256];	/* work string                       */

	extern char *program_name;	/* this program's name (argv[0])     */

	extern int HANG_MON_PERIOD;	/* hang monitor period               */
	extern int semhe_id;	/* semaphore id                      */

	extern union shm_pointers shm_addr;	/* shared memory union pointers      */
        
        int num_entries = 0; /* local number of shm HE entries    */
	int i;			/* loop counter     */
	long clock;		/* current time in seconds           */
	long elasped_time;	/* current elased time in seconds    */
	long max_update_lapse;	/* max time between updates        */

	short int hung_toggle;	/* toggles between 0 and 1 each time
				   a multiple of max_update_lapse
				   is reached by a hung HE.          */

	static char monitor_name[] = "hang_monitor";

	struct htxshm_HE *p_htxshm_HE;	/* pointer to a htxshm_HE struct     */

	struct semid_ds sembuffer;	/* semaphore buffer                  */

	struct sigaction sigvector;	/* structure for signals */

	union shm_pointers shm_addr_wk;	/* work ptr into shm                 */

	program_name = monitor_name;	/* to make "hang_monitor appear as program name in messages */
	 send_message("hang_monitor process started.", 0, HTX_SYS_INFO, HTX_SYS_MSG);

	/*
	 * Set default SIGNAL handling
	 */
	sigemptyset(&(sigvector.sa_mask));	/* do not block signals */
	sigvector.sa_flags = 0;	/* do not restart system calls on sigs */
	sigvector.sa_handler = SIG_DFL;
	for (i = 1; i <= SIGMAX; i++)  {
		 sigaction(i, &sigvector, (struct sigaction *) NULL);
	}

	sigaddset(&(sigvector.sa_mask), SIGINT);
	sigaddset(&(sigvector.sa_mask), SIGQUIT);
	sigaddset(&(sigvector.sa_mask), SIGTERM);

	sigvector.sa_handler = (void (*)(int)) hang_sig_end;
	sigaction(SIGINT, &sigvector, (struct sigaction *) NULL);
	sigaction(SIGQUIT, &sigvector, (struct sigaction *) NULL);
	sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

	/*
	 * set up shared memory addressing
	 */
	shm_addr_wk.hdr_addr = shm_addr.hdr_addr; /* copy addr to work space  */
	(shm_addr_wk.hdr_addr)++;		  /* skip over header */

	/*
	 * Loop to check each HE for hang
	 */
	while (TRUE) {
		clock = time((long *) 0);
		p_htxshm_HE = shm_addr_wk.HE_addr;
                num_entries = (shm_addr.hdr_addr)->max_entries;
                for(i=0; i<num_entries; i++, p_htxshm_HE++)
                { 	
			/*
			 * determine HE status
			 */

			if ((p_htxshm_HE->PID != 0)
					&& (p_htxshm_HE->tm_last_upd != 0)
					&& ((p_htxshm_HE->max_cycles == 0) || ((p_htxshm_HE->max_cycles != 0) && (p_htxshm_HE->cycles < p_htxshm_HE->max_cycles)))
					&& ((semctl(semhe_id, 0, GETVAL, &sembuffer) == 0)
						&& (semctl(semhe_id, ((i * SEM_PER_EXER) + 6), GETVAL, &sembuffer) == 0)
						&& (semctl(semhe_id, ((i * SEM_PER_EXER) + 6 + 1), GETVAL, &sembuffer) == 0))) {	
				/* If here, then the HE we are looking at
				 * (1) Is not DEAD!
				 * (2) Is not stopped by main memu option 1 or 2!
				 * (3) Has not reached max_cycles!, and
				 * (4) Is not stopped on an ERROR!
				 */
				elasped_time = clock - p_htxshm_HE->tm_last_upd;
				max_update_lapse = (long) (p_htxshm_HE->max_run_tm + p_htxshm_HE->idle_time);

				if ((elasped_time > max_update_lapse) && (elasped_time < (6*max_update_lapse)))   {	/* Overdue? */
					hung_toggle = (elasped_time / max_update_lapse) % 2;
					if (p_htxshm_HE->hung_flag != hung_toggle) {	/* Issue message? */
						p_htxshm_HE->hung_flag = hung_toggle;	/* Marked flag */
						p_htxshm_HE->hung_exer = 1;
				   		sprintf(workstr, "%s for %s is HUNG!\n" "Max run time (set in mdt) = %d seconds.\n" "Current elasped time = %ld seconds.", p_htxshm_HE->HE_name, p_htxshm_HE->sdev_id, p_htxshm_HE->max_run_tm, (clock - p_htxshm_HE->tm_last_upd));

						send_message(workstr, 0, HTX_HE_SOFT_ERROR, HTX_SYS_MSG);
					}	/* endif */
				} /* endif */

				else {	/* If here, HE marked hung but no longer overdue! */
					if (elasped_time < max_update_lapse)
					{
					   p_htxshm_HE->hung_flag = 0;	/* clear hung flag */
					   if (p_htxshm_HE->hung_exer == 1)
					   {
   						 sprintf(workstr, "%s for %s is now RUNNING!", p_htxshm_HE->HE_name, p_htxshm_HE->sdev_id);
						 send_message(workstr, 0, HTX_HE_SOFT_ERROR, HTX_SYS_MSG);
					   }
				           p_htxshm_HE->hung_exer = 0;		
					}	
				}	/* endif */
			}	/* endif */
		}		/* end for */

		 sleep(HANG_MON_PERIOD);
	}			/* end while */
}				/* hang_monitor() */


/*
 * NAME: hang_sig_end()
 *
 * FUNCTION: Handles SIGTERM, SIGQUIT, and SIGINT signals.
 *
 */

void hang_sig_end(int signal_number, int code, struct sigcontext *scp)
     /*
      * signal_number -- the number of the received signal
      * code -- unused
      * scp -- pointer to context structure
      */
{
	char workstr[512];
	sprintf(workstr, "Received signal %d.  Exiting...", signal_number);
	send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	exit(0);
	return;
}				/* hang_sig_end() */
