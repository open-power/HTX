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

/* @(#)55	1.3.4.1  src/htx/usr/lpp/htx/bin/hxssup/screens.c, htx_sup, htxubuntu 6/8/10 05:42:21 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    screens.c                                             */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  Display Screen Functions                              */
/*                                                                           */
/* COPYRIGHT =         (C) COPYRIGHT IBM CORPORATION 1988                    */
/*                     LICENSED MATERIAL - PROGRAM PROPERTY OF IBM           */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION =          Miscellaneous Display Screen Functions                */
/*            display_scn - displays all or a portion of a given screen data */
/*                file.                                                      */
/*            htx_scn - displays the HTX logo screen.                        */
/*            mmenu_scn - displays the HTX main menu screens.                */
/*                                                                           */
/*                                                                           */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200   */
/*                     -Nt2000                                               */
/*                                                                           */
/* CHANGE ACTIVITY =                                                         */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                  */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM */
/*    06/17/88:1.0  :J BURGESS :INITIAL RELEASE                              */
/*    01/18/00:1.16 :R GEBHARDT:Feature 290676 Add/Terminate Exercisers      */
/*            :     :          :                                             */
/*                                                                           */
/*****************************************************************************/

#include <unistd.h>

#include "hxssup.h"

#ifdef	__HTX_LINUX__
#include <wchar.h>
typedef	wchar_t NLSCHAR;
#endif


/*****************************************************************************/
/*****  d i s p l a y _ s c n ( )  *******************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     display_scn()                                         */
/*                                                                           */
/* DESCRIPTIVE NAME =  Display a screen                                      */
/*                                                                           */
/* FUNCTION =          Based on the parameters passed from the calling       */
/*                     function, display all or some portion of the passed   */
/*                     screen data file.                                     */
/*                                                                           */
/* INPUT =             winscr - CURSES window for display.                   */
/*                     wrow - Physical place on the display screen where the */
/*                         CURSES window, winscr, begins.                    */
/*                     wcol - Physical place on the display screen where the */
/*                         CURSES window, winscr, begins.                    */
/*                     wmaxrow - Number of rows on the CURSES window, winscr.*/
/*                     wmaxcol - Number of cols on the CURSES window, winscr.*/
/*                                                                           */
/*                     scn_file - pointer to the name of the screen data     */
/*                         file.                                             */
/*                     page - page number in the screen file to be shown.    */
/*                     arow - beginning row in the screen file to be shown.  */
/*                     acol - beginning col in the screen file to be shown.  */
/*                     amaxrow - number of rows per page in the screen file. */
/*                     amaxcol - number of cols per page in the screen file. */
/*                     refresh - logical variable:                           */
/*                                 'y' == refresh the screen when done.      */
/*                                 'n' == do not refresh.                    */
/*                                                                           */
/* OUTPUT =            All or some portion of the screen data file           */
/*                     displayed on the screen.                              */
/*                                                                           */
/* NORMAL RETURN =     0 - Successful completion.                            */
/*                                                                           */
/* ERROR RETURNS =     10 - Invalid wrow (beginning row of window) value.    */
/*                     11 - Invalid wcol (beginning column of window) value. */
/*                     12 - Invalid wmaxrow (last row of window) value.      */
/*                     13 - Invalid wmaxcol (last column of window) value.   */
/*                                                                           */
/*                     20 - Unable to access scn_file file (character file). */
/*                     21 - Unable to access scn_file.a file (attribute      */
/*                          file).                                           */
/*                                                                           */
/*                     30 - Invalid data/format of scn_file file (character  */
/*                          file).                                           */
/*                     31 - Invalid data/format of scn_file.a file           */
/*                          (attribute file).                                */
/*                                                                           */
/*                     40 - Unable to allocate scn_file buffer.              */
/*                     41 - Unable to allocate scn.a_file buffer.            */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    DATA AREAS =     LINES - number of lines on CURSES screen.             */
/*                     COLS  - number of columns on CURSES screen.           */
/*                                                                           */
/*****************************************************************************/

int display_scn(WINDOW * winscr, int wrow, int wcol, int wmaxrow,
		int wmaxcol, char *scn_file, int page, int *arow,
		int *acol, int amaxrow, int amaxcol, char refrsh)

     /* WINDOW *         winscr;        -- window specification              */
     /* int              wrow;          -- row where the window starts       */
     /* int              wcol;          -- col where the window starts       */
     /* int              wmaxrow;       -- number of rows in the window      */
     /* int              wmaxcol;       -- number of columns in the window   */
     /* char *           scn_file;      -- screen character file             */
     /* int              page;          -- page number in file to be shown   */
     /* int *            arow;          -- beginning row to be shown         */
     /* int *            acol;          -- beginning column to be shown      */
     /* int              amaxrow;       -- number of rows per page in file   */
     /* int              amaxcol;       -- number of cols per page in file   */
     /* char             refrsh;        -- logical refresh screen variable   */
{

	char *scn_array;	/* pointer to scn_file char buffer   */
	char *scna_array;	/* pointer to .a file attr buffer    */
	char last_mode;		/* last known screen mode            */
	char workstr[128];	/* work string                       */

/*	extern char HTXPATH[];	 HTX file system path              */
	extern char HTXSCREENS[];	/* HTX file system path             */

	extern int COLS;	/* number of cols in CURSES screen   */
	extern int LINES;	/* number of lines in CURSES screen  */

	int disp_cols;		/* number of cols to display         */
	int disp_rows;		/* number of rows to display         */
	int i;			/* loop counter                      */
	int j;			/* loop counter                      */
	int scn_id;		/* file id for scn_file (char. file) */
	int scna_id;		/* file id for .a file (attributes)  */

	off_t fileptr;		/* pointer into a file (seek)        */

	/*
	 * check window parameters
	 */
	if ((wrow < 0) || (wrow > (LINES - 5)))  {
		return (10);
	}
	if ((wcol < 0) || (wcol > (COLS - 5)))  {
		return (11);
	}
	if ((wmaxrow < 5) || (wmaxrow > (LINES - wrow)))  {
		return (12);
	}
	if ((wmaxcol < 5) || (wmaxcol > (COLS - wcol)))  {
		return (13);
	}

	/*
	 * try to open scn_file (character file)
	 */

	/* remove hard coded paths */
	/*
	    htx_strcpy(workstr, HTXPATH);
	    htx_strcat(workstr, "/screens/");
	 */

#ifdef	__HTX_LINUX__
	htx_strcpy(HTXSCREENS, getenv("HTXSCREENS"));
	if(htx_strlen(HTXSCREENS) == 0)  {
		htx_strcpy(HTXSCREENS, "/usr/lpp/htx/screens/");
	}

#else
	if (htx_strlen(htx_strcpy(HTXSCREENS, getenv("HTXSCREENS")))  == 0)  {
		 htx_strcpy(HTXSCREENS, "/usr/lpp/htx/screens");
	}
#endif

       	htx_strcpy(workstr, HTXSCREENS);
	htx_strcat(workstr, scn_file);
	if ((scn_id = open(workstr, O_RDONLY)) == -1)  {
		return (20);
	}

	/*
	 * try to open scn_file.a (attribute file)
	 */
	htx_strcat(workstr, ".a");
	if ((scna_id = open(workstr, O_RDONLY)) == -1) {
		close(scn_id);
		return (21);
	}	/* endif */

	/*
	 * check *arow value and adjust if necessary
	 */
	if ((amaxrow - *arow) < (wmaxrow - 1)) {
		if (amaxrow > wmaxrow)  {
			*arow = amaxrow - wmaxrow;
		}
		else  {
			*arow = 0;
		}
	}	/* endif */

	/*
	 * check *column value and adjust if necessary
	 */

	if (((amaxcol - 1) - *acol) < (wmaxcol - 1)) {
		if ((amaxcol - 1) > wmaxcol)  {
			*acol = (amaxcol - 1) - wmaxcol;
		}
		else  {
			*acol = 0;
		}
	}	/* endif */

	/*
	 * Go to proper starting point in the char and attr files.
	 */
	fileptr = (off_t) (((page - 1) * (amaxrow * amaxcol)) + (*arow * amaxcol) + *acol);
	if (lseek(scn_id, fileptr, SEEK_SET) == (off_t) - 1) {
		close(scn_id);
		close(scna_id);
		return (30);
	}	/* endif */

	if (lseek(scna_id, fileptr, SEEK_SET) == (off_t) - 1) {
		close(scn_id);
		close(scna_id);
		return (31);
	}	/* endif */

	/*
	 * set the number of columns to be displayed for the given screen
	 */
	if ((amaxcol - 1) < wmaxcol)  {
		disp_cols = amaxcol - 1;
	}
	else  {
		disp_cols = wmaxcol;
	}

	/*
	 * set the number of rows to be displayed for the given screen
	 */
	if (amaxrow < wmaxrow)  {
		disp_rows = amaxrow;
	}
	else  {
		disp_rows = wmaxrow;
	}

	/*
	 * allocate character arrays for both the character and attribute files
	 */
	if ((scn_array = (char *) calloc((size_t) amaxcol, (size_t) sizeof(char))) == NULL) {
		close(scn_id);
		close(scna_id);
		return (40);
	}	/* endif */

	if ((scna_array = (char *) calloc((size_t) amaxcol, (size_t) sizeof(char))) == NULL) {
		close(scn_id);
		close(scna_id);
		free(scn_array);
		return (41);
	}	/* endif */

	/*
	 * force the first call to colorout()
	 */
	last_mode = (char) 0xff;

	/*
	 * display the screen
	 */
	for (i = *arow; i < disp_rows; i++) {
		if (read(scn_id, scn_array, (unsigned int) amaxcol) !=  amaxcol) {
			close(scn_id);
			close(scna_id);
			free(scn_array);
			free(scna_array);
			return (30);
		}

		if (read(scna_id, scna_array, (unsigned int) amaxcol) !=  amaxcol) {
			close(scn_id);
			close(scna_id);
			free(scn_array);
			free(scna_array);
			return (31);
		}

		wmove(winscr, i, 0);
		for (j = *acol; j < disp_cols; j++) {
			if (*(scna_array + j) != last_mode) {
				switch (*(scna_array + j)) {

#ifdef	__HTX_LINUX__
					case '0':
       						wcolorout(winscr, NORMAL);
						break;

					case '1':
						wcolorout(winscr, BLACK_BLUE | NORMAL);
						break;

					case '2':
       						wcolorout(winscr, BLACK_BLUE | NORMAL | BOLD | UNDERSCORE);
						break;

					case '3':
	   					wcolorout(winscr, BLACK_RED | NORMAL | BOLD);
						break;

					case '4':
	   					wcolorout(winscr, BLUE_BLACK | NORMAL);
						break;

					case '5':
       						wcolorout(winscr, BLUE_BLACK | NORMAL | BOLD | UNDERSCORE);
						break;

					case '6':
   						wcolorout(winscr, GREEN_BLACK | NORMAL);
						break;

					case '7':
       						wcolorout(winscr, GREEN_BLACK | BOLD | UNDERSCORE | NORMAL);
						break;

					case '8':
   						wcolorout(winscr, RED_BLACK | NORMAL);
						break;

					case '9':
       						wcolorout(winscr, RED_BLUE | BOLD | UNDERSCORE | NORMAL);
						break;

					case 'A':
   						wcolorout(winscr, WHITE_BLUE | NORMAL);
						break;

					case 'a':
   						wcolorout(winscr, BLACK_RED | STANDOUT);
						break;

					case 'b':
	   					wcolorout(winscr, BLACK_RED | BOLD | STANDOUT);
						break;

					case 'c':
       						wcolorout(winscr, BLACK_RED | BOLD | UNDERSCORE | STANDOUT);
						break;

					default:
       						wcolorout(winscr, NORMAL);
						break;


#else
					case '0':
       						wcolorout(winscr, NORMAL);
						break;

					case '1':
						wcolorout(winscr, NORMAL | F_BLACK | B_BLUE);
						break;

					case '2':
       						wcolorout(winscr, NORMAL | F_BLACK | B_BLUE | BOLD | UNDERSCORE);
						break;

					case '3':
	   					wcolorout(winscr, NORMAL | F_BLACK | B_RED | BOLD);
						break;

					case '4':
	   					wcolorout(winscr, NORMAL | F_BLUE | B_BLACK);
						break;

					case '5':
       						wcolorout(winscr, NORMAL | F_BLUE | B_BLACK | BOLD | UNDERSCORE);
						break;

					case '6':
   						wcolorout(winscr, NORMAL | F_GREEN | B_BLACK);
						break;

					case '7':
       						wcolorout(winscr, NORMAL | F_GREEN | B_BLACK | BOLD | UNDERSCORE);
						break;

					case '8':
   						wcolorout(winscr, NORMAL | F_RED | B_BLACK);
						break;

					case '9':
       						wcolorout(winscr, NORMAL | F_RED | B_BLUE | BOLD | UNDERSCORE);
						break;

					case 'A':
   						wcolorout(winscr, NORMAL | F_WHITE | B_BLUE);
						break;

					case 'a':
   						wcolorout(winscr, STANDOUT | F_BLACK | B_RED);
						break;

					case 'b':
	   					wcolorout(winscr, STANDOUT | F_BLACK | B_RED | BOLD);
						break;

					case 'c':
       						wcolorout(winscr, STANDOUT | F_BLACK | B_RED | BOLD | UNDERSCORE);
						break;

					default:
       						wcolorout(winscr, NORMAL);
						break;
#endif
				}	/* endswitch */

				last_mode = *(scna_array + j);
			}		/* endif */

			waddch(winscr, (NLSCHAR) * (scn_array + j));
		}		/* endfor */
	}			/* endfor */

	wmove(winscr, i, 0);
#ifdef	__HTX_LINUX__
	wcolorout(winscr, NORMAL);
#else
	wcolorout(winscr, NORMAL);
#endif
	if ((refrsh == 'y') || (refrsh == 'Y'))  {
		wrefresh(winscr);
	}

	close(scn_id);
	close(scna_id);
	free(scn_array);
	free(scna_array);

	return (0);
}				/* display_scn() */


/*****************************************************************************/
/*****  h t x _ s c n ( )  ***************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     htx_scn()                                             */
/*                                                                           */
/* DESCRIPTIVE NAME =  Display the HTX logo screen.                          */
/*                                                                           */
/* FUNCTION =          This function displays the HTX logo screen via the    */
/*                     display_scn() function.                               */
/*                                                                           */
/* INPUT =             None.                                                 */
/*                                                                           */
/* OUTPUT =            The displayed HTX logo screen.                        */
/*                     level_str - external string which has the HTX and LINUX */
/*                                 levels.                                   */
/*                                                                           */
/* NORMAL RETURN =     None - void function.                                 */
/*                                                                           */
/* ERROR RETURNS =     None - void function.                                 */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = display_scn - the display screen function.            */
/*                                                                           */
/*    DATA AREAS =     LINES - number of lines on CURSES screen.             */
/*                     COLS  - number of columns on CURSES screen.           */
/*                     level_str - contains LINUX and HTX levels.              */
/*                                                                           */
/*****************************************************************************/

void	htx_scn(void)
{
	extern char level_str[];	/* HTX and LINUX levels. */
	extern int COLS;	/* number of cols in CURSES screen   */
	extern int LINES;	/* number of lines in CURSES screen  */
	extern WINDOW *stdscr;	/* standard screen window            */

	int column = 0;		/* column number                     */
	int fileid = 0;		/* file id.                          */
	int row = 0;		/* row number                        */
	int level_str_len = 0;


	display_scn(stdscr, 0, 0, LINES, COLS, "htx_scn", 1, &row, &column, 23, 81, 'n');
	htx_strncpy(level_str, "", 79);

#ifdef	__HTX_LINUX__
	if ((fileid = open(getenv("HTXLINUXLEVEL"), O_RDONLY)) != -1) {
		if ((level_str_len = read(fileid, level_str, 78)) <= 0) {
			 htx_strcpy(level_str, "Linux ???? -- HTX ????");
		}

		close(fileid);
	}

	else  if ((fileid = open("/etc/redhat-release", O_RDONLY)) != -1) {
		if ((level_str_len = read(fileid, level_str, 78)) <= 0) {
			 htx_strcpy(level_str, "Linux ???? -- HTX ????");
		}

		close(fileid);
	}

#else
	/*if ((fileid = open("/usr/lpp/htx/htxlevel", O_RDONLY)) != -1)*/
	if ((fileid = open(getenv("HTXAIXLEVEL"), O_RDONLY)) != -1) {
		if ((level_str_len = read(fileid, level_str, 78)) <= 0) {
			 htx_strcpy(level_str, "AIX ???? -- HTX ????");
		}

		close(fileid);
	}
	else {
		 htx_strcpy(level_str, "AIX ???? -- HTX ????");
	}

#endif


	level_str[level_str_len] = '\0';	/* terminate it */

	/*
	 * write LINUX and HTX levels on htx screen and refresh
	 */
	mvwaddstr(stdscr, 21, 1, level_str);
	wrefresh(stdscr);

	return;
}				/* htx_scn */


/*****************************************************************************/
/*****  m m e n u _ s c n ( )  ***********************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     mmenu_scn()                                           */
/*                                                                           */
/* DESCRIPTIVE NAME =  Display the HTX main menu screen.                     */
/*                                                                           */
/* FUNCTION =          This function displays the HTX main menu screen via   */
/*                     the display_scn() function.                           */
/*                                                                           */
/* INPUT =             page - indicates which page of the main menu to show. */
/*                                                                           */
/* OUTPUT =            The displayed HTX main menu screen.                   */
/*                                                                           */
/* NORMAL RETURN =     None - void function.                                 */
/*                                                                           */
/* ERROR RETURNS =     None - void function.                                 */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = display_scn - the display screen function.            */
/*                                                                           */
/*    DATA AREAS =     LINES - number of lines on CURSES screen.             */
/*                     COLS  - number of columns on CURSES screen.           */
/*                                                                           */
/*                     level_str - HTX and LINUX levels.                       */
/*                     semhe_id - main htx system semaphore id.              */
/*                     shm_addr - shared memory address.                     */
/*                     stdscr - standard CURSES screen.                      */
/*                                                                           */
/*****************************************************************************/

void	mmenu_scn(int *page)
{
	extern char level_str[];	/* HTX and LINUX levels                */
	extern int COLS;	/* number of cols in CURSES screen   */
	extern int LINES;	/* number of lines in CURSES screen  */
	extern WINDOW *stdscr;	/* standard screen window       */

	extern int semhe_id;	/* semaphore id                      */

	extern union shm_pointers shm_addr;	/* shared memory union pointers      */

	int semval;		/* semaphore value                   */

	static int col = 0;	/* column number                     */
	static int row = 0;	/* row number                        */

	struct semid_ds sembuffer;	/* semaphore buffer                  */
	extern void end_it(int );


	if (*page < 1)  {
		*page = 1;
	}
	if (*page > 1)  {
		*page = 1;
	}

	if(LINES < 24 || COLS < 80)
	{
		PRTMSG(1,0,("Increase the window resolution to 23 - 81\n"));
		sleep(2);
	}
	display_scn(stdscr, 0, 0, LINES, COLS, "mmenu_scn", *page, &row, &col, 23, 81, 'n');

	if (shm_addr.hdr_addr->started != 0) {	/* system started. */
		semval = semctl(semhe_id, 0, GETVAL, &sembuffer);
		wmove(stdscr, 3, 36);
#ifdef	__HTX_LINUX__
		wcolorout(stdscr, CYAN_BLACK | BOLD | UNDERSCORE | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_CYAN | B_BLACK | BOLD | UNDERSCORE);
#endif
		if (semval == 0) {	/* system active?                */
			mvwaddstr(stdscr, 3, 36, "Running ");
		}

		else {	/* system halted.                */
			mvwaddstr(stdscr, 3, 36, "Halted  ");
		}		/* endif */
	}	/* endif */

	wmove(stdscr, 5, 38);
#ifdef	__HTX_LINUX__
	wcolorout(stdscr, CYAN_BLACK | BOLD | UNDERSCORE | NORMAL);
#else
	wcolorout(stdscr, NORMAL | F_CYAN | B_BLACK | BOLD | UNDERSCORE);
#endif
	if (shm_addr.hdr_addr->sp1 == 1) {
		mvwaddstr(stdscr, 5, 38, "STRESS");
	}

	else
	if(shm_addr.hdr_addr->emc == 1) {
		mvwaddstr(stdscr, 5, 38, "EMC");
	}
	else
	if(shm_addr.hdr_addr->emc == 0) {
		mvwaddstr(stdscr, 5, 38, "REG");
	}			/* endif */

	wmove(stdscr, 1, 1);
#ifdef	__HTX_LINUX__
	wcolorout(stdscr, NORMAL);
#else
	wcolorout(stdscr, NORMAL);
#endif
	waddstr(stdscr, level_str);

	wmove(stdscr, MSGLINE, 0);
#ifdef	__HTX_LINUX__
	wcolorout(stdscr, NORMAL);
#else
	wcolorout(stdscr, NORMAL);
#endif
	wrefresh(stdscr);

	return;
}				/* mmenu_scn */

