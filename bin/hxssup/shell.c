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

/* @(#)56	1.2  src/htx/usr/lpp/htx/bin/hxssup/shell.c, htx_sup, htxubuntu 5/24/04 13:46:44 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    shell.c                                               */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  shell out                                             */
/*                                                                           */
/* COPYRIGHT =         (C) COPYRIGHT IBM CORPORATION 1990                    */
/*                     LICENSED MATERIAL - PROGRAM PROPERTY OF IBM           */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Invokes $SHELL shell program.                         */
/*                                                                           */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200   */
/*                     -Nt2000                                               */
/*                                                                           */
/* CHANGE ACTIVITY =                                                         */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                  */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM */
/*    06/04/90:1.0  :J BURGESS :INITIAL RELEASE                              */
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
/*            :     :          :                                             */
/*                                                                           */
/*****************************************************************************/

#include "hxssup.h"
#include <unistd.h>
#include <sys/wait.h>

#ifdef	__HTX_LINUX__
#define	SIGMAX	(SIGRTMAX)
#endif

pid_t shell_PID = 0;		/* global shell PID for signal       */
					/* routines when no virtual term's   */
					/* are in use.                       */

/*****************************************************************************/
/*****  s h e l l ( )  *******************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     shell()                                               */
/*                                                                           */
/* DESCRIPTIVE NAME =  shell out                                             */
/*                                                                           */
/* FUNCTION =          Invokes $SHELL shell program.                         */
/*                                                                           */
/* INPUT =             $SHELL - Environment variable which contains the name */
/*                         of the desired shell program.                     */
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

void	shell(void)
{
	char shellname[128];	/* shell program name       */
	char workstr[128];	/* work string              */

	int exec_rc;		/* exec return code.        */
	int frkpid;		/* child PID for shell.     */
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
	resetterm();
#else
	csavetty(FALSE);	/* save current CURSES tty state. */
	resetty(TRUE);		/* reset original tty state and clear screen. */
#endif
	

      	/*
	 * Load the shell program...
	 */
	frkpid = fork();
	switch (frkpid) {
		case 0:		/* child process                     */
		  	/*
			 * set up sigvector structure
			 */
			sigvector.sa_handler = SIG_DFL;	/* set default action */
			sigemptyset(&(sigvector.sa_mask));	/* do not block other signals     */
			sigvector.sa_flags = 0;	/* No special flags */

			for (i = 1; i <= SIGMAX; i++)  {	/* set default signal processing   */
		   		sigaction(i, &sigvector, (struct sigaction *) NULL);
			}

			/*
			 * get $SHELL environment variable
			 */

#ifdef	__HTX_LINUX__
			htx_strcpy(shellname, getenv("SHELL"));
			if(htx_strlen(shellname) == 0)  {
				htx_strcpy(shellname, "/bin/sh");
			}
#else
			if (htx_strlen(htx_strcpy(shellname, getenv("SHELL"))) == 0)  {
			      	htx_strcpy(shellname, "/bin/ksh");
			}
#endif

			exec_rc = execl(shellname, shellname, (char *) 0);
			if (exec_rc == -1) {
	   			sprintf(workstr, "Unable to load %s.  errno = %d", shellname, errno);
       				send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
				exit(-1);
			}	/* endif */

			break;

		case -1:
#ifdef	__HTX_LINUX__
			resetterm();
#else
			cresetty(FALSE);	/* restore CURSES tty state. */
#endif
			PRTMSG(MSGLINE, 0, ("Unable to fork for message handler.  errno = %d", errno));
       			getch();
       			fflush(stdin);
			CLRLIN(MSGLINE, 0);
			break;

		default:
			shell_PID = frkpid;
       			waitpid(frkpid, (void *) 0, 0);
	   		sigaction(SIGINT, &old_SIGINT_vector, (struct sigaction *) 0);
       			sigaction(SIGQUIT, &old_SIGQUIT_vector, (struct sigaction *) 0);

#ifdef	__HTX_LINUX__
			fixterm();
#else
			resetty(TRUE);	/* reset original tty state and clear screen. */
       			cresetty(FALSE);	/* reset original tty state and clear screen. */
#endif
       			crmode();	/* turn off canonical processing */
       			noecho();	/* turn off auto echoing */
       			nonl();	/* turn off NEWLINE mapping */

#ifdef	__HTX_LINUX__
			keypad((WINDOW *) stdscr, TRUE);
#else
			keypad(TRUE);	/* turn on key mapping */
#endif
       			clear();	/* clear screen                      */
       			refresh();	/* display screen                    */
			break;
	}			/* endswitch */

	return;
}				/* shell() */

