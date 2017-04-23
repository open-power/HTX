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

/* @(#)42	1.8.3.7  src/htx/usr/lpp/htx/bin/hxssup/disp_dst.c, htx_sup, htxubuntu 12/16/14 03:57:44 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    disp_dst.c                                            */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  Display the Device Status Table                       */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Displays all devices defined by the mdt               */
/*                     (/usr/lpp/htx/mdt/mdt attribute file) and the time    */
/*                     of their last update and last error                   */
/*                                                                           */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200   */
/*                     -Nt2000                                               */
/*                                                                           */
/* CHANGE ACTIVITY =                                                         */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                  */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM */
/*    06/15/88:1.0  :J BURGESS :INITIAL RELEASE                              */
/*    11/18/99:1.13 :R GEBHARDT:Added TM status for hotplug support.         */
/*            :     :          :                                             */
/*                                                                           */
/*****************************************************************************/

#include "hxssup.h"
#include <unistd.h>
#include <poll.h>
#include <sys/select.h>


volatile int	refresh_screen_5;
int glbl_cycles_flag = 1;


/*****************************************************************************/
/*****  d i s p _ d s t ( )  *************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     disp_dst()                                            */
/*                                                                           */
/* DESCRIPTIVE NAME =  Display the Device Status Table                       */
/*                                                                           */
/* FUNCTION =          Displays all devices defined by the mdt               */
/*                     (/usr/lpp/htx/mdt/mdt attribute file) and the time    */
/*                     of their last update and last error                   */
/*                                                                           */
/* INPUT =             Operator keyboard and/or mouse input                  */
/*                                                                           */
/* OUTPUT =            Device Status screen(s)                               */
/*                                                                           */
/* NORMAL RETURN =     0 to indicate successful completion                   */
/*                                                                           */
/* ERROR RETURNS =     -1 to indicate no hardware exerciser programs         */
/*                         currently in memory.                              */
/*                     -2 to indicate an unsuccessful attempt to allocate    */
/*                         memory for the device status table sort.          */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = display_scn -  displays screen image                  */
/*                     init_dst_tbl - initializes and sorts the list of      */
/*                                    devices                                */
/*                     send_message - outputs a message to the screen and    */
/*                                    the message log file                   */
/*                                                                           */
/*    DATA AREAS =     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                     SEMHEKEY semaphores (identified by the semhe_id       */
/*                        variable)                                          */
/*                                                                           */
/*    MACROS =         CLRLIN - Clear line CURSES macro                      */
/*                              (defined in common/hxiconv.h)                */
/*                     PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/

int	disp_dst(void)
{
	char status[3];		/* HE status.                        */
	char workstr[128];	/* work string                       */
	char page_str[15];
	char page_char;

	extern char level_str[];	/* HTX and AIx levels.               */

	extern int COLS;	/* number of cols in CURSES screen   */
	extern int LINES;	/* number of lines in CURSES screen  */
	extern int semhe_id;	/* semaphore id                      */
	extern struct tm start_time;	/* System startup time.              */
	extern WINDOW *stdscr;	/* standard screen window            */

	extern union shm_pointers shm_addr;	/* shared memory union pointers      */

	int col = 0;		/* first column displayed on screen  */
	int errno_save;		/* errno save area. */
	int error = 0;		/* error flag (1 = not ack'ed)       */
	int i;			/* loop counter                      */
	int j;			/* loop counter                      */
	int k;			/* loop counter                      */
	int rc;
	int dev_no=-1;
	int dev_page_no;
	int key_input;		/* input from keyboard               */
	int max_page;		/* max dst page number               */
	int num_disp;		/* number of dst entries to show     */
	int num_entries = 0;	/* local number of shm HE entries    */
	int page = 1;		/* current dst page                  */
	int poll_rc;
	int row = 0;		/* first row displayed on screen     */
	unsigned long long min_cycles_done = 0xffffffffffffffff;
	unsigned long long max_cycles_done = 0;
	int yr2000;		/* year -100 if >1999                */
	int temp_page;

	long clock;		/* current time in seconds           */

	static int dst_beep = 1;	/* beep on error flag (0=no, 1=yes)  */

	struct dst_entry *p_dst_table = NULL;	/* points to begin dst seq tbl  */
	struct dst_entry *p_into_table;	/* points into dst sequence table    */
	struct pollfd poll_fd[1];	/* poll() file descriptor structure  */
	struct htxshm_HE *p_htxshm_HE;	/* pointer to a htxshm_HE struct     */
	struct semid_ds sembuffer;	/* semaphore buffer                  */
	struct tm *p_tm = NULL;	/* pointer to tm structure           */
	union shm_pointers shm_addr_wk;	/* work ptr into shm                 */
	time_t temp_time;


	/*
	 * set up shared memory addressing
	 */
	shm_addr_wk.hdr_addr = shm_addr.hdr_addr; /* copy addr to work space  */
	(shm_addr_wk.hdr_addr)++;	/* skip over header                  */

	/*
	 * find out how many entries we need to display
	 */

	if ((shm_addr.hdr_addr)->max_entries == 0) { /* no HE programs? */
		PRTMSG(MSGLINE, 0, ("There are no Hardware Exerciser programs defined in the mdt file."));
		if (p_dst_table != NULL)  {
			free((char *) p_dst_table);	/* release memory for dst order tbl */
		}

		return (-1);
	}

	else {
		num_entries = init_dst_tbl(&p_dst_table);
		if (num_entries <= 0) {
			return (num_entries);
		}

		else {
			max_page = ((num_entries - 1) / 34) + 1;
		}		/* endif */
	}			/* endif */

	/*
	 * Setup pollfd structure...
	 */
	poll_fd[0].fd = 0;	/* Poll stdin (0) for */
#ifdef	__HTX_LINUX__
	poll_fd[0].events = POLLIN;
#else
	poll_fd[0].reqevents = POLLIN;	/* input.             */
#endif

	/*
	 * loop until operator says to quit
	 */
	for (;;) {
			if(refresh_screen_5 == 1) {
				 if (p_dst_table != NULL)  {
					free((char *) p_dst_table);
					dev_no = -1;
				}
				return (0);
			}

		/*
		 * display screen
		 */
		 display_scn(stdscr, 0, 0, LINES, COLS, "dst_scn", 1, &row, &col, 23, 81, 'n');

		/*
		 * Current time
		 */
		clock = time((long *) 0);
		p_tm = localtime(&clock);
		if (p_tm->tm_year > 99) {
			yr2000 = p_tm->tm_year - 100;
		}

		else {
			yr2000 = p_tm->tm_year;
		}		/* endif */

		sprintf(workstr, " %.3d  %.2d/%.2d/%.2d  %.2d:%.2d:%.2d ", (p_tm->tm_yday + 1), (p_tm->tm_mon + 1), p_tm->tm_mday, yr2000, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
#ifdef	__HTX_LINUX__
		wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
		 mvwaddstr(stdscr, 0, 55, workstr);

		/*
		 * System startup time
		 */

		if (start_time.tm_year > 99) {
			yr2000 = start_time.tm_year - 100;
		}

		else {
			yr2000 = start_time.tm_year;
		}		/* endif */

		sprintf(workstr, " %.3d  %.2d/%.2d/%.2d  %.2d:%.2d:%.2d ", (start_time.tm_yday + 1), (start_time.tm_mon + 1), start_time.tm_mday, yr2000, start_time.tm_hour, start_time.tm_min, start_time.tm_sec);
#ifdef	__HTX_LINUX__
		wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
		mvwaddstr(stdscr, 1, 55, workstr);
#ifdef	__HTX_LINUX__
		wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
		/* mvwaddstr(stdscr, 2, 20, level_str); */

		/* min_cycles_done = 0x7fffffff;
		max_cycles_done = 0; */

		if (glbl_cycles_flag) {
			sprintf(workstr, "Cycle Curr. ");
			mvwaddstr(stdscr, 4, 26, workstr);
			mvwaddstr(stdscr, 4, 68, workstr);
			sprintf(workstr, "Count Stanza");
#ifdef	__HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | BOLD | UNDERSCORE);
#else
			wcolorout(stdscr, F_GREEN | B_BLACK | BOLD | UNDERSCORE);
#endif
			mvwaddstr(stdscr, 5, 26, workstr);
#ifdef	__HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | BOLD | UNDERSCORE);
#else
			wcolorout(stdscr, F_GREEN | B_BLACK | BOLD | UNDERSCORE);
#endif
			mvwaddstr(stdscr, 5, 68, workstr);
		}

		else {
			sprintf(workstr, "Last Error");
			mvwaddstr(stdscr, 4, 26, workstr);
			mvwaddstr(stdscr, 4, 68, workstr);
			sprintf(workstr, "Day   Time");
#ifdef	__HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | BOLD | UNDERSCORE);
#else
			wcolorout(stdscr, F_GREEN | B_BLACK | BOLD | UNDERSCORE);
#endif
			mvwaddstr(stdscr, 5, 26, workstr);
#ifdef	__HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | BOLD | UNDERSCORE);
#else
			wcolorout(stdscr, F_GREEN | B_BLACK | BOLD | UNDERSCORE);
#endif
			mvwaddstr(stdscr, 5, 68, workstr);
		}

		/*
		 * build screen data for the current page
		 */
		p_into_table = p_dst_table + ((page - 1) * 34);
		num_disp = num_entries - ((page - 1) * 34);

		if (num_disp > 34)  {
			num_disp = 34;
		}

		for (i = 1; i <= 2; i++) {
			for (j = 1; j <= 17; j++) {

				if ((((i - 1) * 17) + j) <= num_disp) {	/* something there?       */
					p_htxshm_HE = shm_addr_wk.HE_addr + p_into_table->shm_pos;

					/*
					 * determine HE status
					 */
					if(shm_addr.hdr_addr->started == 0) {
						 htx_strcpy(status, "  ");	/* System not started. */
					}

					else if (p_htxshm_HE->PID == 0) {
						if (p_htxshm_HE->DR_term == 1) {
							(void) strcpy(status, "DT");		/* HE dead by hotplug */
						} else if (p_htxshm_HE->user_term == 0)  {
							htx_strcpy(status, "DD");		/* HE is dead. */
						} else  {
							htx_strcpy(status, "TM");		/* HE term by user rq */
						}
					}

					else if(semctl(semhe_id,((p_into_table->shm_pos * SEM_PER_EXER) + 6 + 1),  GETVAL, &sembuffer) != 0) {
                                                htx_strcpy(status, "ER");       /* HE is stopped on an error. */
                                        }

					else if ((semctl(semhe_id, 0, GETVAL, &sembuffer) != 0) || (p_htxshm_HE->equaliser_halt == 1) || (semctl(semhe_id, ((p_into_table-> shm_pos * SEM_PER_EXER) + 6), GETVAL, &sembuffer) != 0) && (semctl(semhe_id, ((p_into_table->shm_pos * SEM_PER_EXER) + 6), GETZCNT, &sembuffer) == 1) && (p_htxshm_HE->rand_halt == 0)) {
						htx_strcpy(status, "ST");	/* HE is stopped. */
					}

					else if ((semctl(semhe_id, ((p_into_table-> shm_pos * SEM_PER_EXER) + 6), GETVAL, &sembuffer) != 0) && (semctl(semhe_id, ((p_into_table->shm_pos * SEM_PER_EXER) + 6), GETZCNT, &sembuffer) == 1) && (p_htxshm_HE->rand_halt == 1)) {
                                                htx_strcpy(status, "HR");  /* Halted by random_ahd process  */
                                        }

					else if((p_htxshm_HE->max_cycles != 0) && (p_htxshm_HE->cycles >= p_htxshm_HE->max_cycles)) {
						htx_strcpy(status, "CP");	/* HE is done. */
					}

					else if((clock > p_htxshm_HE->tm_last_upd) && ((clock - p_htxshm_HE->tm_last_upd) > ((long)(p_htxshm_HE->max_run_tm + p_htxshm_HE->idle_time)))) {
						htx_strcpy(status, "HG");	/* HE is hung. */
					}

					else {
						 htx_strcpy(status, "RN");	/* HE is running. */
					}

					/*
					 * build output characters for entry
					 */
					if ((htx_strcmp(status, "ST") != 0)) {
						if (p_htxshm_HE->cycles < min_cycles_done)  {
							min_cycles_done = p_htxshm_HE->cycles;
						}

						if (p_htxshm_HE->cycles > max_cycles_done)  {
							    max_cycles_done = p_htxshm_HE->cycles;
						}
					}

					/*
					 * HE status
					 */

					wmove(stdscr, (j + 5), ((i - 1) * 42));
					if ((htx_strcmp(status, "DD") == 0) || (htx_strcmp(status, "HG") == 0) || (htx_strcmp(status, "ER") == 0)) {
#ifdef	__HTX_LINUX__
						wcolorout(stdscr, BLACK_RED | BOLD | STANDOUT);
#else
						wcolorout(stdscr, STANDOUT | F_BLACK | B_RED | BOLD);
#endif
					}

					else {
#ifdef	__HTX_LINUX__
						wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
						wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
					}
					waddstr(stdscr, status);

					/*
					 * /dev/????? device id
					 */
					wmove(stdscr, (j + 5), (((i - 1) * 42) + 3));
					if (htx_strcmp(status, "HG") == 0) {
#ifdef	__HTX_LINUX__
						wcolorout(stdscr, BLACK_RED | STANDOUT | BOLD);
#else
						wcolorout(stdscr, STANDOUT | F_BLACK | B_RED | BOLD);
#endif
					}

					else if ( (dev_no != -1) && ((dev_no+1) == (((i-1)*17)+j)) ) {
						wcolorout(stdscr, BLACK_RED | STANDOUT | BOLD);
					}
					else {
#ifdef	__HTX_LINUX__
						wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
						wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
					}
					waddstr(stdscr, p_htxshm_HE->sdev_id);

					/*
					 * time of last update
					 */
					wmove(stdscr, (j + 5), (((i - 1) * 42) + 12));
					if(((htx_strcmp(status, "ST") == 0) || (htx_strcmp(status, "TM") == 0)) && (p_htxshm_HE->tm_last_upd != 0)) {
						wcolorout(stdscr, BLACK_RED | STANDOUT | BOLD);
					}
					else if ( (dev_no != -1) && ((dev_no+1) == (((i-1)*17)+j)) ) {
#ifdef	__HTX_LINUX__
						wcolorout(stdscr, BLACK_RED | STANDOUT | BOLD);
#else
						wcolorout(stdscr, STANDOUT | F_BLACK | B_RED | BOLD);
#endif
					}

					else {
#ifdef	__HTX_LINUX__
						wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
						wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
					}

					if (p_htxshm_HE->tm_last_upd != 0) {	/* update called? */
						temp_time = (time_t)(p_htxshm_HE->tm_last_upd);
						p_tm = localtime(&(temp_time));
						sprintf(workstr, "%.3d %.2d:%.2d:%.2d", p_tm->tm_yday + 1, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
						waddstr(stdscr, workstr);
					}

					else {
						waddstr(stdscr, "              ");
					}	/* endif */

					/*
					 * time of last error or cycle count
					 */
					if (glbl_cycles_flag) {
						sprintf(workstr, "%9llu %4d", p_htxshm_HE->cycles, p_htxshm_HE->test_id);
						if (p_htxshm_HE->test_id == 0) {
							workstr[13] = '-';
						}	/* endif */

#ifdef	__HTX_LINUX__
						wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
						wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
						waddstr(stdscr, workstr);
					}

					else if (p_htxshm_HE->tm_last_err != 0) {
						wmove(stdscr, (j + 5), (((i - 1) * 42) + 26));
						if ((p_htxshm_HE->err_ack == 0) || (htx_strcmp(status, "ER") == 0)) {
#ifdef	__HTX_LINUX__
							wcolorout(stdscr, BLACK_RED | BOLD | STANDOUT);
#else
							wcolorout(stdscr, STANDOUT | F_BLACK | B_RED | BOLD);
#endif
						}

						else {
#ifdef	__HTX_LINUX__
							wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
							wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
						}

						p_tm = localtime(&(p_htxshm_HE->tm_last_err));
						sprintf(workstr, "%.3d %.2d:%.2d:%.2d", p_tm->tm_yday + 1, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
						waddstr(stdscr, workstr);
					}
				}

				p_into_table++;
			}	/* endfor */
		}		/* endfor */

		sprintf(workstr, "Cycle Count(Min/Max)=%llu/%llu", min_cycles_done, max_cycles_done);
#ifdef	__HTX_LINUX__
		wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
		mvwaddstr(stdscr, 1, 0, workstr);

		sprintf(workstr, "Page Number(Cur/Max)=%d/%d", page, max_page);
 		wcolorout(stdscr, GREEN_BLACK | NORMAL);
		mvwaddstr(stdscr, 2, 0, workstr);

		/*
		 * check for unack'ed errors
		 */
		if (error == 0) {
			for (i = 0; i < num_entries; i++) {
				if ((shm_addr_wk.HE_addr + i)->err_ack == 0) {
					error = 1;
					i = num_entries;
				}	/* endif */
			}	/* endfor */
		}	/* endif */

		wmove(stdscr, 0, 0);

		if (error != 0) {	/* un-ack'ed error? */
#ifdef	__HTX_LINUX__
			wcolorout(stdscr, BLACK_RED | BOLD | STANDOUT);
#else
			wcolorout(stdscr, STANDOUT | F_BLACK | B_RED | BOLD);
#endif
			waddstr(stdscr, "*** ERROR ***");
			if (dst_beep != 0) {	/* beep on error? */
				beep();
			}	/* endif */
		}

		else {	/* no un-ack'ed error */
#ifdef	__HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
			wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
			waddstr(stdscr, "             ");
		}		/* endif */


		/*
		 * update screen
		 */

		wmove(stdscr, MSGLINE, 0);
#ifdef	__HTX_LINUX__
		wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
		if (wrefresh(stdscr) == -1) {
			errno_save = errno;
			PRTMSG(MSGLINE, 0, ("Error on wrefresh().  errno = %d.",errno_save));
			sleep((unsigned) 10);
		}		/* endif */

		if (fflush(stdout) == -1) {
			errno_save = errno;
			PRTMSG(MSGLINE, 0, ("Error on fflush(stdout).  errno = %d.", errno_save));
			sleep((unsigned) 10);
			clearerr(stdout);	/* clear error flag on stdout stream */
		}		/* endif */

		/*
		 * read input from keyboard
		 */
		if((poll_rc = poll(poll_fd, (unsigned long) 1, (long) 2000)) > 0) {
			key_input = getch();
			CLRLIN(MSGLINE, 0);

			switch (key_input) {
				case KEY_NEWL:
				case ' ':
					break;

				case 'f':
				case 'F':
					if (page < max_page)  {
						page++;
						dev_no = -1;
					}
					else  {
						beep();
					}
					break;

				case 'b':
				case 'B':
					if (page > 1)  {
						page--;
						dev_no = -1;
					}
					else  {
						beep();
					}
					break;

				case 'e':
				case 'E':
					dev_no = -1;
					erpt();
					clear();
					refresh();
					break;

				case 'h':
				case 'H':
					dev_no = -1;
					help(16, 50, 2, 2, "ddhelp_scn", 8);
					clear();
					break;

				case 'd':
				case 'D':
					dev_no = -1;
					clear();
					refresh();
					break;

				case 'q':
				case 'Q':
					if (p_dst_table != NULL)  {
						free((char *) p_dst_table);	/* rel mem for dst table */
						dev_no = -1;
					}
					return (0);

				case 'a':
				case 'A':
					error = 0;	/* clear error flag */
					for (i = 0; i < num_entries; i++) {
						(shm_addr_wk.HE_addr + i)->err_ack = 1;
					}	/* endfor */
					break;

				case 'c':
				case 'C':
					if (glbl_cycles_flag)  {
						glbl_cycles_flag = 0;
					}
					else  {
						glbl_cycles_flag = 1;
					}
					break;

				case 's':
				case 'S':
					if (dst_beep != 0) {	/* currently set to beep? */
						dst_beep = 0;	/* turn beep off. */
						PRTMSG(MSGLINE, 0, ("Beeping turned off by operator request."));
						send_message("Device Status Table \"beep\" on error disabled by operator request.", 0, HTX_SYS_INFO, HTX_SYS_MSG);
					}

					else {	/* currently set for NO beep */
						dst_beep = 1; /* turn beep on.*/
						PRTMSG(MSGLINE, 0, ("Beeping turned on by operator request."));
						send_message("Device Status Table \"beep\" on error re-enabled by operator request.", 0, HTX_SYS_INFO, HTX_SYS_MSG);
					}	/* endif */
					break;

#ifndef	__HTX_LINUX__
				case KEY_SLL:	/* Locator select (mouse buttons) */
				case KEY_LOCESC:	/* Locator report following...    */
					fflush(stdin);	/* Discard following locator data in the input buffer. */
					break;

#endif
				case '/':
					dev_no = -1;
					rc = search_win(16, 50, 6, 12, "ddsrch_scn", 1, page_str);
					if ( rc == -1 ) {
						(void) clear();
						PRTMSG(MSGLINE, 0, ("No String entered.."));
						wrefresh(stdscr);
						break;
					}

					if ( rc == 0 ) {
						(void) clear();
						wrefresh(stdscr);
						break;
					}

					for ( k=0; k<num_entries; k++) {
						struct dst_entry    *p_into_table_temp;
						struct htxshm_HE    *p_htxshm_HE_temp;

						p_into_table_temp = p_dst_table + k;
						p_htxshm_HE_temp = shm_addr_wk.HE_addr + p_into_table_temp->shm_pos;

						if ( strncmp(p_htxshm_HE_temp->sdev_id, page_str, rc) == 0 ) {
							break;
						}
					}

					if ( k >= num_entries ) {
						(void) clear();
						PRTMSG(MSGLINE, 0, ("Device not found."));
						wrefresh(stdscr);
						break;
					}

					dev_no = k%34;
					dev_page_no = ((k/34) + 1);

					page = dev_page_no;

					/*
					PRTMSG(MSGLINE, 0, ("%d, %d, %d, %d, %s", num_entries, k, dev_no, dev_page_no, page_str));
					wrefresh(stdscr);
					sleep(3);
					*/

					(void) clear();
					wrefresh(stdscr);
					break;

				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':

					k = 1;
					dev_no = -1;
					page_str[0] = page_char = key_input;
					PRTMSG(MSGLINE, 0, ("%c", page_str[0]));

					do {
						page_char = getchar();
						if ( isdigit(page_char) ) {
							page_str[k++] = page_char;
							page_str[k+1] = '\0';
							PRTMSG(MSGLINE, 0, ("%s", page_str));
						}
						else if ( ((int)page_char) == ENTER_KEY ) {
							break;
						}
						else {
							beep();
							PRTMSG(MSGLINE, 0, ("Invalid number. Enter fresh input."));
							break;
						}
					} while(1);

					page_str[k]='\0';
					temp_page = atoi(page_str);

					if ( temp_page <= max_page ) {
						page = temp_page;
					}
					else {
						beep();
						PRTMSG(MSGLINE, 0, ("Page count limit exceeded"));
					}
					break;

				default:
					dev_no = -1;
					beep();
					PRTMSG(MSGLINE, 0, ("Sorry, that key is not valid with this screen."));
					break;
			}	/* endswitch */
		}
		/* endif */
	}			/* endfor */
}	/* disp_dst() */


/*****************************************************************************/
/*****  i n i t _ d s t _ t b l ( )  *****************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME  =    init_dst_tbl()                                        */
/*                                                                           */
/* DESCRIPTIVE NAME =  Initialize Device Status Table                        */
/*                                                                           */
/* FUNCTION =          Initializes and sorts the devices status table        */
/*                                                                           */
/* INPUT =             p_dst_tbl_ptr: pointer to a pointer which will (after */
/*                         the function completes) point to the device       */
/*                         status table                                      */
/*                                                                           */
/* OUTPUT =            The sorted device status table                        */
/*                                                                           */
/* NORMAL RETURN =     The number of entries in the device status table      */
/*                                                                           */
/* ERROR RETURN =      -1 to indicate no hardware exercisers currently in    */
/*                         memory.                                           */
/*                     -2 to indicate an unsuccessful attempt to allocate    */
/*                         memory for the device status table.               */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = DST_compar() - compares hardware exerciser entries    */
/*                                    for sequence sort                      */
/*                                                                           */
/*    DATA AREAS =     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                                                                           */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

int	init_dst_tbl(struct dst_entry **p_dst_tbl_ptr)
{
	char workstr[128];	/* work string                       */

	extern union shm_pointers shm_addr; /* shared memory union pointers */

	int i;			/* loop counter                      */
	int j;			/* loop counter                      */
	int num_entries;	/* local number of entries           */

	union shm_pointers shm_addr_wk;	/* work ptr into shm                 */

	/*
	 * allocate space for dst order table
	 */
	if (*p_dst_tbl_ptr != NULL)  {
		free((char *) *p_dst_tbl_ptr);
	}

	if ((num_entries = (shm_addr.hdr_addr)->max_entries) == 0) {
		PRTMSG(MSGLINE, 0, ("There are no Hardware Exerciser programs currently executing."));
		return (-1);
	}		/* endif */

	*p_dst_tbl_ptr = (struct dst_entry *) calloc((size_t) num_entries, (size_t) sizeof(struct dst_entry));

	if (*p_dst_tbl_ptr == NULL) {
		 sprintf(workstr, "Unable to allocate memory for DST sort.  errno = %d.", errno);
		PRTMSG(MSGLINE, 0, ("%s", workstr));
		send_message(workstr, errno, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		return (-2);
	}	/* endif */

	/*
	 * build dst display order table
	 */
	shm_addr_wk.hdr_addr = shm_addr.hdr_addr; /* copy addr to work space */
	(shm_addr_wk.hdr_addr)++;		/* skip over header */

	i = 0;
	for (j = 0; j < num_entries; j++) {
		if ((shm_addr_wk.HE_addr + i)->sdev_id[0] == '\0')	/* blank entry?   */  {
			j--;
		}

		else {
			(*p_dst_tbl_ptr + j)->shm_pos = i;
			 htx_strcpy(((*p_dst_tbl_ptr + j)->sdev_id), ((shm_addr_wk.HE_addr + i)->sdev_id));
		}		/* endif */

		i++;
	}			/* endfor */

#ifdef	__HTX_LINUX__
	qsort((void *) *p_dst_tbl_ptr, (size_t) num_entries, (size_t) sizeof(struct dst_entry), (int (*)(const void *, const void *)) DST_compar);
#else
	qsort((char *) *p_dst_tbl_ptr, (size_t) num_entries, (size_t) sizeof(struct dst_entry), (int (*)(void *, void *)) DST_compar);
#endif

	return (num_entries);

}				/* init_dst_tbl() */

