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

/* @(#)52	1.2.4.2  src/htx/usr/lpp/htx/bin/hxssup/mmenu.c, htx_sup, htxubuntu 8/4/14 08:49:52 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    mmenu.c                                               */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  Main Menu Functions                                   */
/*                                                                           */
/* COPYRIGHT =         (C) COPYRIGHT IBM CORPORATION 1988                    */
/*                     LICENSED MATERIAL - PROGRAM PROPERTY OF IBM           */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Main Menu Functions                                   */
/*            mmenu - displays the main menu and handles operator input      */
/*            mm_action - handles the response for "Action" key and mouse    */
/*                button input                                               */
/*            mm_cur_down - handles the response for cursor down key and     */
/*                mouse down movement input                                  */
/*            mm_cur_up - handles the response for cursor up key and mouse   */
/*                up movement input                                          */
/*                                                                           */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200   */
/*                     -Nt2000                                               */
/*                                                                           */
/* CHANGE ACTIVITY =                                                         */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                  */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM */
/*    06/17/88:1.0  :J BURGESS :INITIAL RELEASE                              */
/*    01/21/00:1.11 :R GEBHARDT:Feature 290676 Add/Terminate Exercisers      */
/*            :     :          :                                             */
/*                                                                           */
/*****************************************************************************/

#include <unistd.h>

#include "hxssup.h"

#ifndef	__HTX_LINUX__
#include <sys/access.h>
#endif

#define  MM_IN_ROW	(19)	/* input row for main menu       */
#define  MM_IN_COL      (52)	/* input column for main menu    */

extern pid_t editor_PID;
extern volatile int    refresh_screen_5;
/*****************************************************************************/
/*****  m m e n u ( )  *******************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     mmenu()                                               */
/*                                                                           */
/* DESCRIPTIVE NAME =  Main Menu Function                                    */
/*                                                                           */
/* FUNCTION =          Formats and displays the HTX main menu.               */
/*                                                                           */
/* INPUT =             keyboard input                                        */
/*                     autostart - auto-startup flag                         */
/*                                                                           */
/* OUTPUT =            Formatted main menu screen                            */
/*                     Miscellaneous operator requested functions.           */
/*                                                                           */
/* NORMAL RETURN =     0 to indicate succussful completion - normal shutdown */
/*                       requested.                                          */
/*                     1 - fast shutdown requested.                          */
/*                                                                           */
/*                                                                           */
/* ERROR RETURNS =     None (I'm an optimist)                                */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = do_option - performs requested option.                */
/*                     mmenu_scn - displays main menu screen (screens.c)     */
/*                     send_message - outputs a message to the screen and    */
/*                                    the message log file                   */
/*                                                                           */
/*    DATA AREAS =     HTXPATH - pointer to a string which contains the path */
/*                         to the top level of the HTX file hierarchy        */
/*                         (usually "/usr/lpp/htx").                         */
/*                     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                                                                           */
/*    MACROS =         CLRLIN - Clear line CURSES macro                      */
/*                              (defined in common/hxiconv.h)                */
/*                     PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/

int	mmenu(int autostart)
{
	char input1[2];		/* input string                      */
	char input2[3];		/* input string                      */
	char workstr[128];	/* work string                       */

	int main_option;	/* main option integer value         */
	int page = 1;		/* main menu page number             */
	int pseudo_key;		/* fake keyboard input for auto-start */

	/*
	 * display main menu
	 */
	 mmenu_scn(&page);

	if (autostart == 0)  {
		pseudo_key = 'n';
	}
	else  {
		pseudo_key = 'y';
	}

	/*
	 * Keyboard input loop...
	 */
	for (;;) {
		refresh();	/* refresh screen                  */
		switch (pseudo_key)  {
			case 'y':
				input2[0] = '1';
				input2[1] = '\0';
				pseudo_key = '1';
				break;

			case '1':
				input2[0] = '5';
				input2[1] = '\0';
				pseudo_key = '5';
				break;

			default:
				htx_strncpy(input2, "", DIM(input2));	/* clear input */
				 get_string(stdscr, MM_IN_ROW, MM_IN_COL, input2, (size_t) DIM(input2), "0123456789eEhHmMdDsS", (tbool) TRUE);
				CLRLIN(MSGLINE, 0);
				break;
		}	
		/* endswitch */
			
			switch (input2[0]) {
			case '\0':
				break;

			case 'b':
			case 'B':
				page--;
				mmenu_scn(&page);
				break;

			case 'e':
			case 'E':
				erpt();
				clear();
				refresh();
				mmenu_scn(&page);
				break;

			case 'f':
			case 'F':
				page++;
				mmenu_scn(&page);
				break;

			case 'h':
			case 'H':
				help(16, 50, 2, 2, "mmhelp_scn", 4);
				clear();
				mmenu_scn(&page);
				break;

			case 'd':
			case 'D':
				endwin();
				refresh();
				mmenu_scn(&page);
				break;

			case 'C':
			case 'c':
				htx_strncpy(workstr, "Do you really want me to shut down the " "system? ", DIM(workstr) - 1);
				PRTMSG(MSGLINE, 0, ("%s", workstr));
				htx_strncpy(input1, "", DIM(input1));	/* clear input */
				get_string(stdscr, MSGLINE, htx_strlen(workstr), input1, (size_t) DIM(input1), "yYnN", (tbool) TRUE);
				CLRLIN(MSGLINE, 0);
				if ((input1[0] == 'Y') || (input1[0] == 'y')) {
					send_message("System shutdown by operator request.", 0, HTX_SYS_INFO, HTX_SYS_MSG);
					PRTMSG(MSGLINE, 0, ("System shutdown has started.  Please wait...."));
					return (0);
				}

				else {
					PRTMSS(MSGLINE, 0, "                  ");
				}	/* endif */
				break;

			case 's':
       				htx_strncpy(workstr, "Do you really want me to shut down the " "system? ", DIM(workstr) - 1);
				PRTMSG(MSGLINE, 0, ("%s", workstr));
				htx_strncpy(input1, "", DIM(input1));	/* clear input */
				get_string(stdscr, MSGLINE, htx_strlen(workstr), input1, (size_t) DIM(input1), "yYnN", (tbool) TRUE);
				CLRLIN(MSGLINE, 0);
				if ((input1[0] == 'Y') || (input1[0] == 'y')) {
	    				send_message("System shutdown by operator request.", 0, HTX_SYS_INFO, HTX_SYS_MSG);
					PRTMSG(MSGLINE, 0, ("System shutdown has started.  Please wait...."));
					return (1);
				}

				else {
					PRTMSS(MSGLINE, 0, "                  ");
				}	/* endif */
				break;

			case 'S':
       				htx_strncpy(workstr, "Do you really want to " "shutdown? ", DIM(workstr) - 1);
				PRTMSG(MSGLINE, 0, ("%s", workstr));
				htx_strncpy(input1, "", DIM(input1));	/* clear input */
				get_string(stdscr, MSGLINE, htx_strlen(workstr), input1, (size_t) DIM(input1), "yYnN", (tbool) TRUE);
				CLRLIN(MSGLINE, 0);
				if ((input1[0] == 'Y') || (input1[0] == 'y')) {
		    			send_message("System shutdown by operator request.", 0, HTX_SYS_INFO, HTX_SYS_MSG);
					PRTMSG(MSGLINE, 0, ("System shutdown has started.  Please wait...."));
					return (2);
				}
				
				else {
					PRTMSS(MSGLINE, 0, "I didn't think so.");
				}	/* endif */
				break;

			case 'm':
			case 'M':	/* display man pages */
				edit("");
       				mmenu_scn(&page);
				break;

			default:
				main_option = -1;	/* set to invalid value */
				main_option = atoi(input2);
				do_option(main_option, autostart);
       				mmenu_scn(&page);
		}		/* endswitch */
	}			/* endfor */
}				/* mmenu() */

/*****************************************************************************/
/*****  d o _ o p t i o n ( )  ***********************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     do_option()                                           */
/*                                                                           */
/* DESCRIPTIVE NAME =  Main Menu Option handler                              */
/*                                                                           */
/* FUNCTION =          This function calls the appropriate function(s) for   */
/*                     the option selected.                                  */
/*                                                                           */
/* INPUT =             option - integer value of the selected option.        */
/*                     autoflag - auto startup flag for AH_system()          */
/*                                                                           */
/* OUTPUT =            Miscellaneous status messages.                        */
/*                     Miscellaneous operator requested functions.           */
/*                                                                           */
/* NORMAL RETURN =     None - void function.                                 */
/*                                                                           */
/* ERROR RETURNS =     None - void function.                                 */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = AH_device - Activates/Halts individual devices        */
/*                     AH_system - Activates/Halts the entire HTX system     */
/*                     disp_dst - displays the Device Status Table           */
/*                     send_message - outputs a msg to the screen and the    */
/*                         message log file                                  */
/*                                                                           */
/*    DATA AREAS =     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                                                                           */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/

void	do_option(int option, int autoflag)
{
      	/*
	 * variable/function declarations
	 */
	char workstr[128];	/* work string area.        */
	extern int hxstats_PID;	/* stats program process id          */
	extern union shm_pointers shm_addr; /* shared memory union pointers */
	extern int run_type_disp;

	switch (option) {
		case 1:		/* Activate/Halt system          */
			AH_system(autoflag);
			break;

		case 2:		/* Activate/Halt Devices         */
       			AH_device();
			break;

		case 3:		/* Set Run Type (REG/EMC)        */
			if ((shm_addr.hdr_addr)->started == 0) {	/* sys not started yet?  */
				if (((shm_addr.hdr_addr->emc) == 0) && ((shm_addr.hdr_addr->sp1) ==0 )) {	/* EMC?                   */
					shm_addr.hdr_addr->emc = 1;	/* change to REG        */
				}
				else
				if (((shm_addr.hdr_addr->emc) == 1) && ((shm_addr.hdr_addr->sp1) == 0)) {	/* EMC?                   */
					shm_addr.hdr_addr->emc = 0;	/* change to REG        */
					shm_addr.hdr_addr->sp1 = 1;
				}
				else
				{ 
					shm_addr.hdr_addr->sp1 = 0;	/* change to EMC        */
					shm_addr.hdr_addr->emc = 0;
				}	/* endif */
			}
			
			else {	/* system already started     */
				PRTMSG(MSGLINE, 0, ("Sorry, you are not allowed to change the run type after " "the system has started."));
       				getch();
       				fflush(stdin);
				CLRLIN(MSGLINE, 0);
			}		/* endif */
			break;

		case 4:
       			COE_device();
			break;

		case 5:		/* Display Device Status Table */
			while(1) {
       			disp_dst();
				if (refresh_screen_5 == 1) {
					refresh_screen_5 = 0;
					continue;
				} else {
					break;
				}
			}

			break;

		case 6:		/* Update & Display Statistics */
			run_type_disp = 0 ; 
			if (hxstats_PID != 0) {
       				kill(hxstats_PID, SIGUSR1);	/* tell stats prog to upd file */
				PRTMSG(MSGLINE, 0, ("Updating HTX statistics file..."));
       				sleep(5);      /* give stats prog time to upd */
				edit("/tmp/htxstats");
			}
			
			else {
				PRTMSG(MSGLINE, 0, ("System must be started before statistics can " "be collected."));
       				getch();
			}		/* endif */
			run_type_disp = 1 ; 
			break;

		case 7:		/* Edit HTX Error Log */
			run_type_disp = 0 ; 
			edit("/tmp/htxerr");
			run_type_disp = 1 ; 
			break;

		case 8:		/* Edit HTX Message Log */
			
			run_type_disp = 0 ; 
			edit("/tmp/htxmsg");
			run_type_disp = 1 ; 
			break;

		case 9:		/* Edit MDT file */
       			
			run_type_disp = 0 ; 
			htx_strcpy(workstr, "../mdt/mdt");
			edit(workstr);
			run_type_disp = 1 ; 
			break;

		case 10:		/* Add/Restart/Terminate */
			if ((shm_addr.hdr_addr)->started == 1)  {
				ART_device();
			}
			else {
				PRTMSG(MSGLINE, 0, ("The system must be started before performing these " "operations."));
       				getch();
       				fflush(stdin);
				CLRLIN(MSGLINE, 0);
			}		/* else */
			break;

		case 11:		/* Start $SHELL (shell-out) */
			run_type_disp = 0 ; 
			editor_PID = 111;
			shell();
			run_type_disp = 1 ; 
			break;

		default:
			PRTMSG(MSGLINE, 0, ("Sorry, you entered an invalid option.  Please try again."));
			break;
	}			/* endswitch */

	return;
}				/* do_option() */

