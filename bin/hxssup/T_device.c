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

/* @(#)40	1.9.3.3  src/htx/usr/lpp/htx/bin/hxssup/T_device.c, htx_sup, htxubuntu 12/16/14 03:58:14 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    T_device.c                                            */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  Terminate device                                      */
/*                                                                           */
/* COPYRIGHT =         (C) COPYRIGHT IBM CORPORATION 2000                    */
/*                     LICENSED MATERIAL - PROGRAM PROPERTY OF IBM           */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Terminates exerciser on user request.                 */
/*                                                                           */
/* COMPILER OPTIONS =                                                        */
/*                                                                           */
/* CHANGE ACTIVITY =                                                         */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                  */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM */
/*    11/08/99:1.0  :R GEBHARDT:INITIAL RELEASE                              */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*            :     :          :                                             */
/*****************************************************************************/

#include <unistd.h>

#include "hxssup.h"

#define NUM_COLS	(23)
#define NUM_ROWS	(81)

#define TD_IN_ROW	(21)
#define TD_IN_COL	(18)

extern	char	*regcmp(char *, char *);	/* compile exp */
extern	char	*regex(char *, char *);


extern union shm_pointers shm_addr;	/* shared memory union pointers      */
extern union semun semctl_arg;
extern int semhe_id;		/* semaphore id                      */
extern int slow_shutd_wait;	/* configure from .htx_profile       */
extern char HTXPATH[];		/* HTX file system path         */
extern volatile int system_call;

/*****************************************************************************/
/*****  g e t _ d s t  *******************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     get_dst                                               */
/*                                                                           */
/* DESCRIPTIVE NAME =  Get Device Status                                     */
/*                                                                           */
/* FUNCTION =          Determines the status of a device and returns it.     */
/*                                                                           */
/* INPUT =             Hardware Exerciser entry in shared memory structure.  */
/*                     Position of HE entry in shared memory structure.      */
/*                                                                           */
/* OUTPUT =            Device status.                                        */
/*                                                                           */
/* NORMAL RETURN =     Pointer to status string.                             */
/*                                                                           */
/* ERROR RETURNS =     None                                                  */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES =                                                       */
/*                                                                           */
/*    DATA AREAS =     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                     SEMHEKEY semaphores (identified by the semhe_id       */
/*                        variable)                                          */
/*                                                                           */
/*    MACROS =                                                               */
/*                                                                           */
/*****************************************************************************/

char *get_dst(struct htxshm_HE *p_HE, int shm_pos, enum t_dev_status *status)
{
	static char *devstatus[9] = {	/* storage for returned status string */
		/* 0 */ "UK",
		/* 1 */ "CP",
		/* 2 */ "DD",
		/* 3 */ "ER",
		/* 4 */ "HG",
		/* 5 */ "RN",
		/* 6 */ "SG",
		/* 7 */ "ST",
		/* 8 */ "TM"
	};

	struct semid_ds sembuffer;	/* semaphore buffer                  */
	long clock;		/* current time in seconds           */

	*status = DST_UK;	/* init return status */

	if (p_HE->PID == 0) {
			if (p_HE->user_term == 0)  {	/* did operator rq termination */
					*status = DST_DD;	/* dead */
			}

			else  {
					*status = DST_TM;	/* terminated */
			}

	}	/* if */

	/* its not dead yet */

	else {
			clock = time((long *) 0);	/* get current time */
			if (p_HE->user_term)  {
					*status = DST_SG;	/* SIGTERM'd but not dead */
			}

			/* system running ? HE running ? */
			else if ((semctl(semhe_id, 0, GETVAL, &sembuffer) != 0) || (semctl(semhe_id, ((shm_pos * SEM_PER_EXER) + 6), GETVAL, &sembuffer) != 0)) {
					*status = DST_ST;	/* if not, HE is stopped. */
			}

			/* stopped on error ? */
			else if (semctl(semhe_id, ((shm_pos * SEM_PER_EXER) + 6 + 1), GETVAL, &sembuffer) != 0)  {
					*status = DST_ER;	/* HE is stopped on an error. */
			}

			/* complete */
			else if ((p_HE->max_cycles != 0) &&(p_HE->cycles >= p_HE->max_cycles)) {
					*status = DST_CP;	/* HE is done. */
			}

			else if ((clock - p_HE->tm_last_upd) > ((long)(p_HE->max_run_tm + p_HE->idle_time))) {
					*status = DST_HG;	/* HE is hung. */
			}

			else {
					*status = DST_RN;	/* HE is running. */
			}
	}			/* else */

	return (devstatus[*status]);
}				/* get_dst */

/*****************************************************************************/
/*****  t e r m i n a t e _ e x e r c i s e r  *******************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     terminate_exerciser                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  Terminate hardware exerciser                          */
/*                                                                           */
/* FUNCTION =          Kills a hardware exerciser by sending it a SIGTERM    */
/*                     signal.                                               */
/*                                                                           */
/* INPUT =             p_HE       - points to HE share memory entry          */
/*                     shm_pos - position of HE entry in shared memory.      */
/*                               This is used to compute the number of the   */
/*                               activate/halt and the stop-on-error sema-   */
/*                               phore numbers for that HE.                  */
/*                                                                           */
/* OUTPUT =            kills hardware exerciser programs                     */
/*                                                                           */
/*                     Updated system semaphore structure to reflect the     */
/*                     state of the HE's which are killed.                   */
/*                                                                           */
/* NORMAL RETURN =     N/A - void function                                   */
/*                                                                           */
/* ERROR RETURNS =     N/A - void function                                   */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*                                                                           */
/*    OTHER ROUTINES =                                                       */
/*                     send_message() - outputs a message to the screen and  */
/*                                     the message log file                  */
/*                                                                           */
/*    DATA AREAS =     HTXPATH - pointer to a string which contains the path */
/*                         to the top level of the HTX file hierarchy        */
/*                         (usually "/usr/lpp/htx").                         */
/*                     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                                                                           */
/*    MACROS =         PRTMSG1 - Print message CURSES macro                  */
/*                              (defined in common/hxiconv.h)                */
/*                     CLRLIN                                                */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
void	terminate_exerciser(struct htxshm_HE *p_HE, int shm_pos)
{
	struct semid_ds sembuffer;	/* semaphore buffer                  */
	boolean_t dummy;
	char	*msg_text = NULL;
	char	reg_expr[256];
	int	i, no_of_inst, j = 0, rc = 0;
	int	k, ppid[10], kill_errno = 0;
	int	max_entries = 0, PID = 0;
	int	semval = 0;
	char    buf[1024], cmd_str[1024];
    FILE    *fp;

	extern union shm_pointers shm_addr; /* shared memory union pointers */

	msg_text = malloc(MSG_TEXT_SIZE);

	/* make max_entries local so that it does not change on us */
	max_entries = shm_addr.hdr_addr->max_entries;

	/* save run state */
	semval = semctl(semhe_id, (shm_pos * SEM_PER_EXER + 6), GETVAL, &sembuffer);

	PID = p_HE->PID;	/* improve atomicity */
	/* if the exerciser is not dead */
	/* and system is running */
	if (PID != 0 && p_HE->tm_last_upd != 0 && (semctl(semhe_id, 0, GETVAL, &sembuffer) == 0))  {			/* global wait */
		/*
		   && (semctl(semhe_id, (shm_pos * SEM_PER_EXER + 6), GETVAL, &sembuffer) == 0)      )
		 */
		/* HE running */

		sprintf(msg_text, "Termination of %s requested by operator", p_HE->sdev_id);
		send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);

		sprintf(cmd_str, "ps -ef | grep %d | grep -v grep | awk '{print $3}'", PID);
                if ((fp = popen(cmd_str, "r")) == NULL)  {
                   printf("Error in popen");
                   exit(1);
                }
                while (fgets(buf, 1024, fp) != NULL) {
                   sscanf(buf, "%d", &ppid[j]);
                   j++;
                }
		system_call = TRUE;
                pclose(fp);
		system_call = FALSE;

/*		alarm(slow_shutd_wait);	 config in .htx_profile */
		p_HE->user_term = 1;	/* affects status display */
#ifdef	__HTX_LINUX__
		wcolorout(stdscr, BLACK_WHITE | NORMAL);
#else
		wcolorout(stdscr, NORMAL | B_BLACK | F_WHITE);
#endif
		CLRLIN(MSGLINE, 0);
		PRTMSG1(MSGLINE, 69, ("waiting..."));
		rc = kill(PID, SIGTERM);
		kill_errno = errno;	/* save it for possible error msg */

		if (rc == 0) {
		/* make sure that HE is set to running state */
		semctl_arg.val = 0 ;
		semctl(semhe_id, (shm_pos * SEM_PER_EXER + 6), SETVAL, semctl_arg);
		semctl(semhe_id, (shm_pos * SEM_PER_EXER + 6 + 1), SETVAL, semctl_arg);
		for( i = 0; i < j; i++)  {
                          if (ppid[i] == 1)  {
                            p_HE->PID = 0;
                            shm_addr.hdr_addr->num_entries--;
                            PRTMSG(MSGLINE, 0, ("%s", msg_text));
                            break;
                          }
                        }

                        if (p_HE->PID != 0)  {
  			  pause();	/* wait for any signal */
/*			  if (!alarm(0)) {
				 sprintf(msg_text, "Sending SIGKILL to %s for %s after %d seconds.", p_HE->HE_name, p_HE->sdev_id, slow_shutd_wait);
				 send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
				 kill(PID, SIGKILL);*/	/* no mercy */
		/*	}*/	/* if */
			}
			sleep(1);
/* ((p_shm_hdr)->num_entries)++; *//* let child_death do it */
            if (strncmp(p_HE->HE_name, "hxesct", 6) == 0)
               sprintf(cmd_str, "ps -ef | grep hxesct | grep -v grep | wc -l");
            else
               sprintf(cmd_str, "ps -ef | grep %s | grep -v grep | wc -l", p_HE->HE_name);
			if ((fp = popen(cmd_str, "r")) == NULL)  {
			   (void) sprintf(msg_text,"popen error in terminate_exerciser");
			   (void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		    }
			if (fgets(buf,1024,fp) == NULL) {
			   (void) sprintf(msg_text,"fgets error in terminate_exerciser");
			   (void) send_message(msg_text, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		    }
			sscanf(buf, "%d", &no_of_inst);
			system_call = TRUE;
			pclose(fp);
			system_call = FALSE;
			if(no_of_inst == 0) {
				htx_strcpy(reg_expr, "^");
				htx_strcat(reg_expr, p_HE->HE_name);
				htx_strcat(reg_expr, ".*cleanup[\t ]*$");
				rc = exec_HE_script(reg_expr, p_HE->sdev_id,
					   &dummy);
			}
		} /* endif */

		else {
			alarm((unsigned) 0);	/* cancel alarm               */
			p_HE->user_term = 0;	/* affects status display */
			sprintf(msg_text, "Failed to terminate %s (PID %d) for %s\n", p_HE->HE_name, PID, p_HE->sdev_id);
			send_message(msg_text, 0, HTX_HE_SOFT_ERROR, HTX_SYS_MSG);

			msg_text[htx_strlen(msg_text) - 1] = '\0';	/* clip off new line */
			PRTMSG(MSGLINE, 0, ("%s.  See HTX error log.", msg_text));
			if (kill_errno == ESRCH)  {	/* no such process */
				sprintf(msg_text, "No such process.  Perhaps it is already deceased?\n");
			}

			else  {
				sprintf(msg_text, "Errno = %d.\n", kill_errno);
			}

			send_message(msg_text, 0, HTX_HE_SOFT_ERROR, HTX_SYS_MSG);
		}		/* else */

		/* restore HE activate/halt state  */
		semctl_arg.val = 0;
		semctl(semhe_id, (shm_pos * SEM_PER_EXER + 6), SETVAL, semctl_arg);

	} /* endif PID != 0 */

	else {
		PRTMSG(MSGLINE, 0, ("System must be running to terminate exercisers."));
	}			/* else */

	free(msg_text);
}				/* terminate_exerciser */

/*****************************************************************************/
/*****  T _ d e v i c e  *****************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     T_device                                              */
/*                                                                           */
/* DESCRIPTIVE NAME =  Terminate hardware exerciser                          */
/*                                                                           */
/* FUNCTION =          Kills a hardware exerciser by sending it a SIGTERM    */
/*                     signal.                                               */
/*                                                                           */
/* INPUT =             Operator specifies device to terminate.               */
/*                                                                           */
/* OUTPUT =            kills hardware exerciser programs                     */
/*                                                                           */
/*                     Updated system semaphore structure to reflect the     */
/*                     state of the HE's which are killed.                   */
/*                                                                           */
/* NORMAL RETURN =     0 to indicate successful completion                   */
/*                                                                           */
/* ERROR RETURNS =     -1 to indicate no hardware exerciser programs         */
/*                         currently in memory.                              */
/*                     -2 to indicate an unsuccessful attempt to allocate    */
/*                         memory for the activate/halt device table.        */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = display_scn -  displays screen image                  */
/*                     init_ahd_tbl - initializes and sorts the list of      */
/*                                    devices (HEs)                          */
/*                     send_message - outputs a message to the screen and    */
/*                                    the message log file                   */
/*                     get_dst - returns status of a device in test.         */
/*                     terminate_exerciser - kills an HE and runs cleanup    */
/*                                    scripts by exec_HE_scripts().          */
/*                                                                           */
/*    DATA AREAS =     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                     SEMHEKEY semaphores (identified by the semhe_id       */
/*                        variable)                                          */
/*                     __loc1 - pointer to reg. expression matching string.  */
/*                                                                           */
/*    MACROS =         CLRLIN - Clear line CURSES macro                      */
/*                              (defined in common/hxiconv.h)                */
/*                     PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/

int	T_device(void)
{
	char *cmp_regexp;	/* ptr to compiled reg expression    */
	char input[32];		/* input string                      */
	char ret[3];		/* return array for regex()          */
	char *work;		/* work int */
	char workstr[128];	/* work string                       */
	char page_str[15];

	enum t_dev_status dev_status;	/* return from get_dst               */
	extern char *__loc1;	/* beginning of reg exp match        */

	extern int COLS;	/* number of cols in CURSES screen   */
	extern int LINES;	/* number of lines in CURSES screen  */
	extern WINDOW *stdscr;	/* standard screen window            */

	extern union shm_pointers shm_addr; /* shared memory union pointers */

	int col = 0;		/* first column displayed on screen  */
	int i = 0;		/* loop counter                      */
	int k;               /* loop counter                      */
	int rc;              /* return value.                     */
	int dev_len;         /* loop counter                      */
	int dev_no=-1;       /* loop counter                      */
	int max_strt_ent = 0;	/* max starting entry                */
	int num_disp = 0;		/* number of THE entries to show     */
	int num_entries = 0;	/* local number of shm HE entries    */
	int row = 0;		/* first row displayed on screen     */
	int strt_ent = 1;	/* first entry on screen             */
	int update_screen = 0;	/* update screen flag                */
	int workint = 0;

	struct ahd_entry *p_td_table = NULL; /* points to begin td seq tbl    */
	struct ahd_entry *p_into_table;	/* points into td sequence table     */

	struct htxshm_HE *p_htxshm_HE;	/* pointer to a htxshm_HE struct     */
	union shm_pointers shm_addr_wk;	/* work ptr into shm                 */

	shm_addr_wk.hdr_addr = shm_addr.hdr_addr; /* copy addr to work space */
	(shm_addr_wk.hdr_addr)++;	/* skip over header                   */

	/*
	 * display screen outline
	 */
	clear();
	refresh();
	display_scn(stdscr, 0, 0, LINES, COLS, "TD_scn", 1, &row, &col, NUM_COLS, NUM_ROWS, 'n');

	/*
	 * loop until operator says to quit
	 */

	for (;;) {
		if ((shm_addr.hdr_addr)->max_entries == 0) {	/* no HE programs?        */
			PRTMSG(MSGLINE, 0, ("There are no Hardware Exerciser programs currently defined."));
			getch();
			fflush(stdin);
			CLRLIN(MSGLINE, 0);
			if (p_td_table != NULL)  {
				free((char *) p_td_table);	/* release memory for td order tbl */
			}
			return (-1);
		}

		else {	/* some HE's defined.                */
			if ((shm_addr.hdr_addr)->max_entries != num_entries) {	/* # HE change? */
				num_entries = init_ahd_tbl(&p_td_table);

				if (num_entries <= 0)  {
					return (num_entries);
				}

				/* problem in init fcn - bye!      */
				else {	/* init fcn ran ok.                */
					if (num_entries <= 13) {	/* 13 or fewer entries? */
						max_strt_ent = 1;
					}

					else {	/* more than 13 entries */
						max_strt_ent = num_entries;
					}	/* endif */
				}	/* endif */
			}	/* endif */
		}		/* endif */

		/*
		 * build screen data for the current strt_ent
		 */
		p_into_table = p_td_table + strt_ent - 1;
		num_disp = num_entries - strt_ent + 1;
		if (num_disp > 13)  {
			num_disp = 13;
		}

		for (i = 1; i <= 13; i++) {
			if (i <= num_disp) {	/* something there? */
				p_htxshm_HE = shm_addr_wk.HE_addr + p_into_table->shm_pos;

				/*
				 * build output characters & attributes for
				 * entry
				 */
				/*
				 * screen entry number
				 */
#ifdef	__HTX_LINUX__
				wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
				wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
				wmove(stdscr, (i + 6), 3);
				wprintw(stdscr, "%2d", i);

				/*
				 * Device Status
				 */
				work = get_dst(p_htxshm_HE, p_into_table->shm_pos, &dev_status);
				workint = htx_strlen(work);	/* length of status string */
				htx_strncpy(workstr, "        ", 8 - workint);	/* spaces */
				workstr[8 - workint] = '\0';

				/* set display attributes based on status */
				if (dev_status == DST_CP || dev_status == DST_ST || dev_status == DST_RN)  {
#ifdef	__HTX_LINUX__
					wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
					wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
				}

				if (dev_status == DST_UK || dev_status == DST_SG || dev_status == DST_TM)  {
#ifdef	__HTX_LINUX__
					wcolorout(stdscr, WHITE_BLUE | STANDOUT);
#else
					wcolorout(stdscr, STANDOUT | F_WHITE | B_BLUE);
#endif
				}

				if (dev_status == DST_DD)  {
#ifdef	__HTX_LINUX__
					wcolorout(stdscr, RED_BLACK | STANDOUT);
#else
					wcolorout(stdscr, STANDOUT | F_RED | B_BLACK);
#endif
				}

				if (dev_status == DST_ER || dev_status == DST_HG)  {
#ifdef	__HTX_LINUX__
					wcolorout(stdscr, RED_BLACK | NORMAL);
#else
					wcolorout(stdscr, NORMAL | F_RED | B_BLACK);
#endif
				}

				/* display status */
				mvwaddstr(stdscr, (i + 6), 6, work);

				/* pad field with spaces */

#ifdef	__HTX_LINUX__
				wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
				wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
				mvwaddstr(stdscr, (i + 6), 6 + workint, workstr);

				/*
				 * Slot, Port, /dev/id, Adapt Desc, and Device
				 * Desc
				 */
				wmove(stdscr, i + 6, 15);
#ifdef	__HTX_LINUX__
				wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
				wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif

				sprintf(workstr, " %.4d ", p_htxshm_HE->slot);
				mvwaddstr(stdscr, (i + 6), 15, workstr);

				sprintf(workstr, " %.4d ", p_htxshm_HE->port);
				mvwaddstr(stdscr, (i + 6), 22, workstr);

				/* set color for device id based on status */
				if (dev_status == DST_CP || dev_status == DST_ST || dev_status == DST_RN || (dev_no+1) == i )  {
#ifdef	__HTX_LINUX__
					wcolorout(stdscr, WHITE_BLUE | STANDOUT);
#else
					wcolorout(stdscr, STANDOUT | F_WHITE | B_BLUE);
#endif
				}

				else  {
#ifdef	__HTX_LINUX__
					wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
					wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
				}

				sprintf(workstr, " %-7s ", p_htxshm_HE->sdev_id);
				mvwaddstr(stdscr, (i + 6), 29, workstr);
#ifdef	__HTX_LINUX__
				wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
				wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
				sprintf(workstr, " %-11s ", p_htxshm_HE->adapt_desc);
				mvwaddstr(stdscr, (i + 6), 39, workstr);
				sprintf(workstr, " %-18s ", p_htxshm_HE->device_desc);
				mvwaddstr(stdscr, (i + 6), 53, workstr);
			}

			else {	/* no HE entry for this area of screen */
#ifdef	__HTX_LINUX__
				wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
				wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
				mvwaddstr(stdscr, (i + 6), 3, "  ");	/* scn entry num */
				mvwaddstr(stdscr, (i + 6), 6, "        ");	/* COE Flag  */
				mvwaddstr(stdscr, (i + 6), 15, "      ");	/* slot fld */
				mvwaddstr(stdscr, (i + 6), 22, "      ");	/* port fld */
				mvwaddstr(stdscr, (i + 6), 29, "         ");	/* sdev_id */
				mvwaddstr(stdscr, (i + 6), 39, "             ");
				/* adapt_desc field */

				mvwaddstr(stdscr, (i + 6), 53, "                    ");			/* device_desc field */

			}	/* endif */

			p_into_table++;
		}		/* endfor */

#ifdef	__HTX_LINUX__
		wcolorout(stdscr, NORMAL);
#else
		wcolorout(stdscr, NORMAL);
#endif

		for (update_screen = FALSE; (update_screen == FALSE);) {

			/*
			 * Read input from keyboard...
			 */
			htx_strncpy(input, "", DIM(input));	/* clear input */
			get_string(stdscr, TD_IN_ROW, TD_IN_COL, input, (size_t) DIM(input), (char *) NULL, (tbool) TRUE);

			switch (input[0]) {
			case 'f':
			case 'F':
				dev_no = -1;
				if (strt_ent < max_strt_ent) {
					strt_ent += 13;

					if (strt_ent > max_strt_ent)  {
						strt_ent = max_strt_ent;
					}

					update_screen = TRUE;
				}

				else {
					beep();
				}	/* endif */
				break;

			case 'b':
			case 'B':
				dev_no = -1;
				if (strt_ent > 1) {
					strt_ent -= 13;

					if (strt_ent < 1)  {
						strt_ent = 1;
					}

					update_screen = TRUE;
				}

				else {
					beep();
				}	/* endif */
				break;

			case 'q':
			case 'Q':
				if (p_td_table != NULL)  {
					free((char *) p_td_table);	/* rel mem for td table */
				}
				return (0);

			case 'd':
			case 'D':
				dev_no = -1;
				clear();
				refresh();
				display_scn(stdscr, 0, 0, LINES, COLS, "TD_scn", 1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
				update_screen = TRUE;
				break;

			case '/':
				rc = search_win(16, 50, 2, 2, "ddsrch_scn", 1, page_str);
				dev_len = strlen(page_str);

				if ( rc == -1 ) {
					(void) clear();
					wrefresh(stdscr);
					/* (void) refresh(); */
					(void) display_scn(stdscr, 0, 0, LINES, COLS, "TD_scn",
						1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
					PRTMSG(MSGLINE, 0, ("No String entered."));
					update_screen = TRUE;
					dev_no = -1;
					break;
				}

				if ( rc == 0 ) {
					(void) clear();
					wrefresh(stdscr);
					(void) display_scn(stdscr, 0, 0, LINES, COLS, "TD_scn",
						1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
					update_screen = TRUE;
					dev_no = -1;
					break;
				}

				for ( k=0; k<num_entries; k++) {
					struct ahd_entry    *p_into_table_temp;
					struct htxshm_HE    *p_htxshm_HE_temp;

					p_into_table_temp = p_td_table + k;
					p_htxshm_HE_temp = shm_addr_wk.HE_addr + p_into_table_temp->shm_pos;

					if ( strncmp(p_htxshm_HE_temp->sdev_id, page_str, dev_len) == 0 ) {
						break;
					}
				}

				if ( k >= num_entries ) {
					(void) clear();
					wrefresh(stdscr);
					(void) refresh();
					(void) display_scn(stdscr, 0, 0, LINES, COLS, "TD_scn",
						1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
					PRTMSG(MSGLINE, 0, ("Device not found."));
					update_screen = TRUE;
					dev_no = -1;
					break;
				}

				dev_no = k%13;

				/* Points to the 1st entry of 13-entry-page
				 * in which this device will be displayed.
				 */
				strt_ent = (((k/13) * 13) + 1);
				if (strt_ent > max_strt_ent) strt_ent = max_strt_ent;
				if (strt_ent <= 0) strt_ent = 1;

				(void) clear();
				(void) refresh();
				(void) display_scn(stdscr, 0, 0, LINES, COLS, "TD_scn",
						1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
				update_screen = TRUE;
				break;

			default:
				dev_no = -1;
				cmp_regexp = regcmp(input, (char *) 0);	/* compile exp */
				if (cmp_regexp != NULL) {	/* regular expression OK? */
					for (i = 1; i <= num_disp; i++) {
						sprintf(workstr, "%-d", i);
						work = regex(cmp_regexp, workstr);
						if (work != NULL) {
							htx_strncpy(ret, " ", (size_t) sizeof(ret));	/* clear return (ret) array */
							htx_strncpy(ret, __loc1, (size_t) (work - __loc1));

							if (htx_strcmp(workstr, ret) == 0) {	/* matched reg exp? */
								update_screen = TRUE;
								p_into_table = p_td_table + (strt_ent - 1) + (i - 1);
								p_htxshm_HE = shm_addr_wk.HE_addr + p_into_table->shm_pos;

								if(p_htxshm_HE->PID != 0) {
#ifdef	__HTX_LINUX__
									wcolorout(stdscr, WHITE_BLUE | STANDOUT);
#else
									wcolorout(stdscr, STANDOUT | F_WHITE | B_BLUE);
#endif
									mvwaddstr(stdscr, (i + 6), 6, "SIGNALED");
									terminate_exerciser(p_htxshm_HE, p_into_table->shm_pos);
								} /* endif */

								else {
									sprintf(workstr, "The exerciser for %s, %s is already deceased.", p_htxshm_HE->sdev_id, p_htxshm_HE->HE_name);
									PRTMSG(MSGLINE, 0, (workstr));
								}	/* else */
							}	/* endif */
						}	/* endif */
					}	/* endfor */

					if (update_screen != TRUE) {	/* no match? */
						beep();
					}	/* endif */
				}

				else {	/* invalid entry_number */
					beep();
				}	/* endif */
				break;

			}	/* endswitch */
		}		/* endfor */
	}			/* endfor */
}				/* T_device */

