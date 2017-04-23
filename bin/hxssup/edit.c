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

/* @(#)43	1.3.4.1  src/htx/usr/lpp/htx/bin/hxssup/edit.c, htx_sup, htxubuntu 11/22/11 04:46:34 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    edit.c                                                */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  edit file                                             */
/*                                                                           */
/* COPYRIGHT =         (C) COPYRIGHT IBM CORPORATION 1988                    */
/*                     LICENSED MATERIAL - PROGRAM PROPERTY OF IBM           */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Invokes HTX system editor on specified file.          */
/*                                                                           */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200   */
/*                     -Nt2000                                               */
/*                                                                           */
/* CHANGE ACTIVITY =                                                         */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                  */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM */
/*    06/14/88:1.0  :J BURGESS :INITIAL RELEASE                              */
/*    11/18/99:1.7  :R GEBHARDT:Added TM status for hotplug support.         */
/*            :     :          :                                             */
/*                                                                           */
/*****************************************************************************/

#define CHILD	(0)

#ifdef	__HTX_LINUX__
#define	SIGMAX	(SIGRTMAX)
#endif

#include <unistd.h>

#ifdef	__HTX_LINUX__
#include <sys/wait.h>
#endif

#include "hxssup.h"


pid_t editor_PID = 0;		/* global editor PID   */

/*****************************************************************************/
/*****  e d i t ( )  *********************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     edit()                                                */
/*                                                                           */
/* DESCRIPTIVE NAME =  edits a file                                          */
/*                                                                           */
/* FUNCTION =          Invokes HTX system editor on specified file.          */
/*                                                                           */
/* INPUT =                                                                   */
/*                     file - pointer to a character string containing the   */
/*                         full pathname specification of the file to be     */
/*                         edited.                                           */
/*                                                                           */
/* OUTPUT =            N/A                                                   */
/*                                                                           */
/* NORMAL RETURN =     none - void function.                                 */
/*                                                                           */
/* ERROR RETURNS =     none - void function.                                 */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES =                                                       */
/*                                                                           */
/*    DATA AREAS =                                                           */
/*                                                                           */
/*    MACROS =         CLRLIN - Clear line CURSES macro                      */
/*                              (defined in common/hxiconv.h)                */
/*                     PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/

void	edit(char *file)
{

	char editor[128];	/* editor program           */
	char workstr[128];	/* work string              */

	int exec_rc;		/* exec return code.        */
	int frkpid;		/* child PID for editor     */
	int i;			/* loop counter.            */

	struct sigaction sigvector;	/* structure for signal specs */
	struct sigaction old_SIGINT_vector;	/* structure for signal specs */
	struct sigaction old_SIGQUIT_vector;	/* structure for signal specs */

	/*
	 * Ignore SIGINT and SIGQUIT signals
	 */
	sigvector.sa_handler = SIG_IGN;		/* set default action */
	sigemptyset(&(sigvector.sa_mask));	/* do not blk other sigs */
	sigvector.sa_flags = 0;			/* No special flags */

	sigaction(SIGINT, &sigvector, &old_SIGINT_vector);
	sigaction(SIGQUIT, &sigvector, &old_SIGQUIT_vector);

#ifdef	__HTX_LINUX__
	saveterm();
#else
	csavetty(FALSE);	/* save current CURSES tty state. */
#endif
#ifdef	__HTX_LINUX__
	fixterm();
#else
	resetty(TRUE);	/* reset original tty state and clear screen. */
#endif

	/*
	 * load HTX system editor
	 */
	frkpid = fork();
	switch (frkpid) {
		case CHILD:		/* child process                     */
			/*
			 * set up sigvector structure
			 */
			sigvector.sa_handler = SIG_DFL;	/* set default action */
			sigemptyset(&(sigvector.sa_mask));	/* do not blk other sigs */
			sigvector.sa_flags = 0;	/* No special flags          */

			for (i = 1; i <= SIGMAX; i++)  {	/* set default signal proc   */
				 sigaction(i, &sigvector, (struct sigaction *) NULL);
			}

			/*
			 * get $EDITOR environment variable
			 */

			/*if ((int) strlen((char *) htx_strcpy(editor, getenv("HTX_EDITOR"))) == 0) */
			/* if ((int) strlen((char *) htx_strcpy(editor, getenv("EDITOR"))) == 0)    */
			htx_strcpy(editor, "vi");

			if (*file == '\0') {	/* no specified file? run man */
				 htx_strcpy(workstr, "-M");

 				 if (htx_strlen(htx_strcat(workstr, getenv("HTXPATH"))) == 0)  {
					 htx_strcat(workstr, "/usr/lpp/htx");
				}

				 htx_strcat(workstr, "/man");
				exec_rc = execl("/bin/man", "man", workstr, "htxhp", (char *) 0);
				sleep(5);
			}

			else  {
				exec_rc = execlp(editor, editor, "-R", file, (char *) 0);
			}

			if (exec_rc == -1) {
				 sprintf(workstr, "Unable to load %s.  errno = %d", editor, errno);
				 send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
				exit(-1);
			}		/* endif */

			break;


		case -1:
#ifdef	__HTX_LINUX__
			fixterm();
#else
			cresetty(FALSE);	/* restore CURSES tty state. */
#endif

			PRTMSG(MSGLINE, 0, ("Unable to fork for message handler.  errno = %d",errno));
			 getch();
			 fflush(stdin);
			CLRLIN(MSGLINE, 0);
			break;

		default:
			editor_PID = frkpid;
			 waitpid(frkpid, (void *) 0, 0);
			 sigaction(SIGINT, &old_SIGINT_vector, (struct sigaction *) 0);
			 sigaction(SIGQUIT, &old_SIGQUIT_vector, (struct sigaction *) 0);

#ifdef	__HTX_LINUX__
			 fixterm();
#else
			 resetty(TRUE);	/* reset orig tty state & clr scn */
			 cresetty(FALSE);	/* reset orig tty state & clr scr */
#endif
			 crmode();	/* turn off canonical processing  */
			 noecho();	/* turn off auto echoing */
			 nonl();	/* turn off NEWLINE mapping */

#ifdef	__HTX_LINUX__
			 keypad((WINDOW *) stdscr, TRUE);	/* turn on key mapping */
#else
			 keypad(TRUE);	/* Surprising this works under AIX !! */
#endif
			 clear();	/* clear screen      */
			 refresh();	/* display screen              */
			 break;
	}			/* endswitch */

	return;
}				/* edit() */


