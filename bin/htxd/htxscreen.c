
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

#include <pthread.h>
#ifdef  __HTX_LINUX__
	#include <curses.h> 
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>

#include <hxiconv.h>
#include <htx_local.h>
#include "htxclient.h"
#include "htxd_define.h"


#ifdef  __HTX_LINUX__
	#include <wchar.h>
	typedef wchar_t NLSCHAR;
#endif

#define  MM_IN_ROW      (19)    /* input row for main menu       */
#define  MM_IN_COL      (52)    /* input column for main menu    */

#define NUM_COLS        (23)
#define NUM_ROWS        (81)
#define ART_NUM_COLS    (81)
#define ART_NUM_ROWS    (23)
#define MDT_LIST_LINE_LENGTH	70	

#define AH_IN_ROW       (21)
#define AH_IN_COL       (18)
#define CE_IN_ROW       (21)
#define CE_IN_COL       (18)
#define ARTD_IN_ROW     (19)
#define ARTD_IN_COL     (52)

#define HTX_SCREENS_DIR "screens"
#define HTXLINUXLEVEL	"htxlinuxlevel_info"
#define HTXAIXLEVEL	"htxaixlevel"

#define	MASTER_MODE_DISPLAY	"*** HTX [htxscreen mode : Master] ***"
#define	VIEW_MODE_DISPLAY	"*** HTX [htxscreen mode : View] ***"
#define MODE_DISPLAY_OFFSET	23
#define	SUT_DISPLAY		"*** SUT: %s ***"


extern int COLS;        /* number of cols in CURSES screen   */
extern int LINES;       /* number of lines in CURSES screen  */
extern WINDOW *stdscr;  /* standard screen window       */

htxclient_parameter global_parameter;
extern char htxclient_global_error_text[1024];

int editor_PID;
int shell_PID;
int global_htxscreen_local_mode = 1;
int global_exit_flag = 0;
int global_master_mode = 0;

char global_htx_level_info[512] = {'\0'};
char global_selected_mdt_name[MAX_ECG_NAME_LENGTH]  = {'\0'};
volatile int global_heart_beat_stop = 0;
char global_htxscreen_start_time[128] = {'\0'};

char global_htx_home_dir[256];
char global_htx_log_dir[256] = "/tmp";
char global_htxscreen_log_dir[256] = "/tmp";




int htxscreen_get_mdt_list_line_count(char *text)
{
	int text_length;
	int i;
	int line_count = 0;


	text_length = strlen(text);
	for(i =0; i < text_length; i++) {
		if( *(text++) == '\n') {
			line_count++;
		}
	}

	if(text_length > 0) {
		line_count++;
	}

	return line_count;
}


int htxscreen_select_mdt(char *mdt_name)
{
	htxclient_command command_object;
	htxclient_response response_object;
	int select_socket_fd;
	char command_string[512];


	initialize_command_object(&command_object, "-select");
	initialize_response_object(&response_object);
	select_socket_fd = htxclient_create_socket();
	htxclient_connect(select_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	sprintf(command_object.ecg_name, "%s/mdt/%s", global_htx_home_dir, mdt_name);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(select_socket_fd, command_string);

	htxclient_receive_response(select_socket_fd, &response_object);

	if(response_object.response_buffer != NULL) {
		free(response_object.response_buffer);
	}
	htxclient_close_socket(select_socket_fd);

	return response_object.return_code;
}


void htxscreen_display_htx_screen_header(void)
{

	char temp_string[256];
	int string_length;


	if(global_master_mode == 1) {
		sprintf(temp_string, MASTER_MODE_DISPLAY);
	} else {
		sprintf(temp_string, VIEW_MODE_DISPLAY);
	}
#ifdef  __HTX_LINUX__
	wcolorout(stdscr, NORMAL | BOLD | UNDERSCORE);
#else
	wcolorout(stdscr, STANDOUT | F_RED | B_BLACK | BOLD | UNDERSCORE);
#endif
	mvwaddstr(stdscr, 0, MODE_DISPLAY_OFFSET, temp_string);

#ifdef  __HTX_LINUX__
	wcolorout(stdscr, NORMAL | NORMAL );
#else
	wcolorout(stdscr, NORMAL | F_RED | B_BLACK);
#endif

#ifdef  __HTX_LINUX__
	wcolorout(stdscr, RED_BLACK | STANDOUT | BOLD);
#else
	wcolorout(stdscr, STANDOUT | F_RED | B_BLACK | BOLD);
#endif
	sprintf(temp_string, SUT_DISPLAY, global_parameter.sut_hostname);
	string_length = strlen(temp_string);
	if(string_length > 70) {
		temp_string[69] = '\0';
		string_length = 70;	
	}

	mvwaddstr(stdscr, 22, 40 - (string_length/2), temp_string);

#ifdef  __HTX_LINUX__
	wcolorout(stdscr, NORMAL | NORMAL );
#else
	wcolorout(stdscr, NORMAL | F_RED | B_BLACK);
#endif

}



void htxscreen_set_htxscreen_start_time(void)
{
	time_t current_time_stamp;		
	struct tm time_object;

	
	current_time_stamp = time(0);
	localtime_r(&current_time_stamp, &time_object);

	sprintf(global_htxscreen_start_time, "%.4d%.2d%.2d_%.2d%.2d%.2d", 
		time_object.tm_year+1900, 
		time_object.tm_mon+1,
		time_object.tm_mday+1,
		time_object.tm_hour,
		time_object.tm_min,
		time_object.tm_sec
	);
	
}



int htxscreen_get_daemon_states(daemon_states* p_states)
{

	char command_string[512];
	htxclient_command command_object;
	htxclient_response response_object;
	int socket_fd;


	initialize_command_object(&command_object, "-get_daemon_state");
	initialize_response_object(&response_object);

	socket_fd = htxclient_create_socket();
	htxclient_connect(socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	sprintf(command_object.option_list, "%s", " ");
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(socket_fd, command_string);

	htxclient_receive_response(socket_fd, &response_object);
	if(response_object.return_code == -1) {
		p_states->daemon_state = -1;
		p_states->daemon_state = -1;
	} else {
		sscanf(response_object.response_buffer, "%d %d", &(p_states->daemon_state), &(p_states->test_state));
//		daemon_state = atoi(response_object.response_buffer);
	}

	htxclient_close_socket(socket_fd);

	if(response_object.return_code == -1) {
		printf("%s", response_object.response_buffer);
	}	
	free(response_object.response_buffer);

	return response_object.return_code;
}



int htxscreen_get_total_device_count(void)
{
	int			device_count = 0;
	int			socket_fd_device_count;
	htxclient_command       command_object_device_count;
	htxclient_response      response_object_device_count;
	char                    command_string[COMMAND_STRING_LENGTH];



	initialize_command_object(&command_object_device_count, "-device_count");
	initialize_response_object(&response_object_device_count);
	socket_fd_device_count = htxclient_create_socket();
	htxclient_connect(socket_fd_device_count, command_object_device_count.sut_hostname, command_object_device_count.daemon_port_number);
	htxclient_prepare_command_string(&command_object_device_count, command_string);
	htxclient_send_command(socket_fd_device_count, command_string);	
	htxclient_receive_response(socket_fd_device_count, &response_object_device_count);
	htxclient_close_socket(socket_fd_device_count);
	
	if(response_object_device_count.return_code == 0) {
		device_count =  atoi(response_object_device_count.response_buffer);
	}

	if(response_object_device_count.response_buffer != NULL) {
		free(response_object_device_count.response_buffer);
	}

	return device_count;
}

void    htxscreen_init_screen(void)
{
#ifndef __HTX_LINUX__
	do_colors = TRUE;       /* reset colors on endwin()          */
#endif
	initscr();      /* standard CURSES init function     */
	crmode();       /* turn off canonical processing     */
	noecho();       /* turn off auto echoing             */
	nonl();         /* turn off NEWLINE mapping          */

	/*if(has_colors() == TRUE) {
		//start_color();
	} */

#ifdef  __HTX_LINUX__
	keypad((WINDOW *)stdscr, TRUE);
#else
	keypad(TRUE);   /* turn on key mapping               */
#endif
	
	clear();        /* clear screen                      */
	refresh();      /* display screen                    */

}



int htxscreen_get_string(WINDOW * curses_win,
		int input_row,
		int input_col,
		char *input_str,
		size_t max_str_len,
		char *good_chars,
		tbool echo_flag)
{
	int end_index;		/* index to last char in input_str */
	int i;			/* loop index */
	int input_index;	/* input index */
	int key_input;		/* character entered */
	tbool insert_flag;	/* Insert mode flag */

	/*
	 * Beginning of Executable Code
	 */
	insert_flag = FALSE;
	input_index = 0;
	end_index = strlen(input_str) - 1;

	wmove(curses_win, input_row, input_col);
	wrefresh(curses_win);

	for (;;) {
		/*
		 * Get the next input character -- don't get fooled by a signal...
		 */
		for (; (((key_input = wgetch(curses_win)) == -1) && (errno == EINTR)); )
			;	/* get next character */

		/*
		 * Handle the new character...
		 */
		switch (key_input) {

			case KEY_NEWL:	/* end of string? */
				wmove(curses_win, input_row, input_col);
				for (i = 0; i < (end_index + 1); i++)  {
					waddch(curses_win, ' ');
				}

				wmove(curses_win, input_row, input_col);
				wrefresh(curses_win);
				return (0);

			case -1:
				return (-1);

			case KEY_BACKSPACE:
				if (input_index > 0)  {
					input_index--;
				}
				else if (end_index < 0) {
					beep();
					break;
				}	/* endif */

			case KEY_DC:
				if (end_index >= input_index) {
					end_index--;
					if (end_index >= 0)  {
						strcpy((input_str + input_index), (input_str + input_index + 1));
					}

					*(input_str + end_index + 1) = '\0';
				}

				else  {
					beep();
				}

				if (echo_flag == TRUE) {
					mvwaddch(curses_win, input_row,	(input_col + end_index + 1), ' ');
				}	/* endif */
				break;

			case KEY_LEFT:
				if (input_index > 0)  {
					input_index--;
				}
				else  {
					beep();
				}
				break;

			case KEY_RIGHT:
				if (input_index < (end_index + 1))  {
					input_index++;
				}
				else  {
					beep();
				}
				break;

			case KEY_HOME:
				input_index = 0;
				break;

			case KEY_END:
				input_index = end_index;
				break;

			case KEY_IC:
				if (insert_flag == TRUE)  {
					insert_flag = FALSE;
				}
				else  {
					insert_flag = TRUE;
				}
				break;

			case KEY_EIC:
				insert_flag = FALSE;
				break;

#ifndef	__HTX_LINUX__
			case KEY_SLL:	 /* Locator Select (mouse buttons) */
			case KEY_LOCESC: /* Locator report coming...       */
				fflush(stdin);	/* Discard locator data in input */
				break;
#endif

			default:	/* echo char, position cursor */
				if (key_input < 128) {	/* printable ascii character? */
					if ((good_chars == NULL) || (strchr(good_chars, (char) key_input) != NULL)) {
						if (insert_flag == TRUE) {	/* insert mode? */
							if (end_index != (max_str_len - 2)) {
								end_index++;
								for (i = end_index; i > input_index; i--)  {
									*(input_str + i) = *(input_str + i - 1);
								}

								*(input_str + input_index) = (char) key_input;
								if (input_index < (max_str_len -  2))  {
									input_index++;
								}
							}

							else  {
								beep();	/* no more room in input string */
							}
						}

						else {	/* replace mode */

							if((input_index != end_index) || (input_index != (max_str_len - 2))) {
								*(input_str + input_index) = (char) key_input;
								if (end_index < input_index)  {
								end_index = input_index;
								}

								if (input_index < (max_str_len - 2))  {
									input_index++;
								}
							}

							else  {
								beep();	/* no more room in input string */
							}
						}	/* endif */
					}

					else  {
						beep();	/* char not in good_chars string */
					}
				}

				else  {
					beep();	/* non-printable character */
				}
				break;
		}		/* endswitch */

		if (echo_flag == TRUE) {
			mvwaddstr(curses_win, input_row, input_col, input_str);
			wmove(curses_win, input_row, (input_col + input_index));
			wrefresh(curses_win);
		}	/* endif */
	}		/* endfor */
}		


int htxscreen_display_sut_mdt_screen_5(void)
{
	char temp_string[256];
	int string_length;

#ifdef  __HTX_LINUX__
	wcolorout(stdscr, RED_BLACK | STANDOUT | BOLD);
#else
	wcolorout(stdscr, STANDOUT | F_RED | B_BLACK | BOLD);	
#endif
	sprintf(temp_string, "SUT: %s", global_parameter.sut_hostname);
	string_length = strlen(temp_string);
	if(string_length > 35) {
		temp_string[34] = '\0';
	}

	mvwaddstr(stdscr, 3, 2, temp_string);

	if(global_selected_mdt_name[0] != '\0') {
		sprintf(temp_string, "MDT: %s", global_selected_mdt_name);
		string_length = strlen(temp_string);
		if(string_length > 35) {
			temp_string[34] = '\0';
		}

		 mvwaddstr(stdscr, 3, 42, temp_string);
	}

#ifdef  __HTX_LINUX__
	wcolorout(stdscr, NORMAL | NORMAL );
#else
	wcolorout(stdscr, NORMAL | F_RED | B_BLACK);
#endif
	return 0;
}


int htxscreen_display_screen(WINDOW * winscr,
				int wrow,
				int wcol,
				int wmaxrow,
				int wmaxcol,
				char *scn_file,
				int page,
				int *arow,
				int *acol,
				int amaxrow,
				int amaxcol,
				char refrsh)
{


	char HTXSCREENS[100];   /* HTX file system path             */
	char workstr[128];      /* work string                       */
	int scn_id;             /* file id for scn_file (char. file) */
	int scna_id;            /* file id for .a file (attributes)  */
	char *scn_array;        /* pointer to scn_file char buffer   */
	char *scna_array;       /* pointer to .a file attr buffer    */
	char last_mode;         /* last known screen mode            */
	off_t fileptr;          /* pointer into a file (seek)        */
	int disp_cols;          /* number of cols to display         */
	int disp_rows;          /* number of rows to display         */
	int i;                  /* loop counter                      */
	int j;                  /* loop counter                      */
	char *env_ptr;
	


	/* check window parameters */	
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

	/* try to open scn_file (character file) */
	env_ptr = getenv("HTXSCREENS");
	if(env_ptr == NULL) {
		sprintf(HTXSCREENS, "%s/etc/%s/", global_htx_home_dir, HTX_SCREENS_DIR);
		sprintf(workstr, "HTXSCREENS=%s", HTXSCREENS);
		//putenv(workstr);
	} else {
		strcpy(HTXSCREENS, env_ptr);
	} 

	strcpy(workstr, HTXSCREENS);
	strcat(workstr, scn_file);
	if ((scn_id = open(workstr, O_RDONLY)) == -1)  {
		return (20);
	}

	/* try to open scn_file.a (attribute file) */
	strcat(workstr, ".a");
	if ((scna_id = open(workstr, O_RDONLY)) == -1) {
		close(scn_id);
		return (21);
	}

	/* check *arow value and adjust if necessary */
	if ((amaxrow - *arow) < (wmaxrow - 1)) {
		if (amaxrow > wmaxrow)  {
			*arow = amaxrow - wmaxrow;
		} else {
			*arow = 0;
		}
	}

	/* check *column value and adjust if necessary */
	if (((amaxcol - 1) - *acol) < (wmaxcol - 1)) {
		if ((amaxcol - 1) > wmaxcol)  {
			*acol = (amaxcol - 1) - wmaxcol;
		} else {
			*acol = 0;
		}
	}

	/* Go to proper starting point in the char and attr files. */
	fileptr = (off_t) (((page - 1) * (amaxrow * amaxcol)) + (*arow * amaxcol) + *acol);
	if (lseek(scn_id, fileptr, SEEK_SET) == (off_t) - 1) {
		close(scn_id);
		close(scna_id);
		return (30);
	}

	if (lseek(scna_id, fileptr, SEEK_SET) == (off_t) - 1) {
		close(scn_id);
		close(scna_id);
		return (31);
	}

	/* set the number of columns to be displayed for the given screen */
	if ((amaxcol - 1) < wmaxcol)  {
		disp_cols = amaxcol - 1;
	} else {
		disp_cols = wmaxcol;
	}

	/* set the number of rows to be displayed for the given screen */
	if (amaxrow < wmaxrow)  {
		disp_rows = amaxrow;
	} else {
		disp_rows = wmaxrow;
	}
	
	/* allocate character arrays for both the character and attribute files */
	if ((scn_array = (char *) calloc((size_t) amaxcol, (size_t) sizeof(char))) == NULL) {
		close(scn_id);
		close(scna_id);
		return (40);
	}

	if ((scna_array = (char *) calloc((size_t) amaxcol, (size_t) sizeof(char))) == NULL) { 
		close(scn_id);
		close(scna_id);
		free(scn_array);
		return (41);
	}

	/* force the first call to colorout() */
	last_mode = (char) 0xff;

	/* display the screen */
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
#ifdef __HTX_LINUX__
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
		}			/* endfor */
	}				/* endfor */

	wmove(winscr, i, 0);

#ifdef  __HTX_LINUX__
	wcolorout(winscr, NORMAL);
#else
	wcolorout(winscr, NORMAL);
#endif

	if( strcmp(scn_file, "dst_scn") == 0) {
		htxscreen_display_sut_mdt_screen_5();
	}

	if ((refrsh == 'y') || (refrsh == 'Y'))  {
		wrefresh(winscr);
	}

	close(scn_id);
	close(scna_id);
	free(scn_array);
	free(scna_array);

	return 0;
}


int htxscreen_main_menu_screen_display(daemon_states current_state)
{

	static int col = 0;     /* column number                     */
	static int row = 0;     /* row number                        */
	int page = 1;
	char temp_str[256];


	if(LINES < 24 || COLS < 80) {
		PRTMSG(1,0,("Increase the window resolution to 23 - 81\n"));
		sleep(2);
	}

	htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "htxscreen_mmenu_scn", page, &row, &col, 23, 81, 'n');

#ifdef  __HTX_LINUX__
	wmove(stdscr, 1, 1);
	wcolorout(stdscr, NORMAL);
#else
	wmove(stdscr, 1, 32);
	wcolorout(stdscr, NORMAL);
#endif
	waddstr(stdscr, global_htx_level_info);
	
	wmove(stdscr, MSGLINE, 0);
#ifdef  __HTX_LINUX__
	wcolorout(stdscr, NORMAL);
#else
	wcolorout(stdscr, NORMAL);
#endif

	memset(temp_str, 0, sizeof(temp_str));
	wmove(stdscr, 3, 33);
	if(current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) {
		sprintf(temp_str, "SELECTED <%s>", global_selected_mdt_name);
		waddstr(stdscr, temp_str);
	} else if(current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT) {
		if(current_state.test_state == 0) {
			sprintf(temp_str, "HALTED <%s>", global_selected_mdt_name);
		} else {
			sprintf(temp_str, "RUNNING <%s>", global_selected_mdt_name);
		}
		waddstr(stdscr, temp_str);
	} else {
		waddstr(stdscr, "IDLE       ");
	}

	htxscreen_display_htx_screen_header();	

	wrefresh(stdscr);


	return 0;
}



int htxscreen_start_selected_mdt(void)
{

	htxclient_command command_object;
	htxclient_response response_object;
	int run_socket_fd;
	char command_string[512];
	daemon_states	current_state;


	htxscreen_get_daemon_states(&current_state);

	if(current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) { 
		PRTMSG(MSGLINE, 0, ("Starting MDT : %s, please wait...", global_selected_mdt_name) );
		initialize_command_object(&command_object, "-run");
		initialize_response_object(&response_object);
		run_socket_fd = htxclient_create_socket();
		htxclient_connect(run_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
		sprintf(command_object.ecg_name, "%s/mdt/%s", global_htx_home_dir, global_selected_mdt_name);
		htxclient_prepare_command_string(&command_object, command_string);
		htxclient_send_command(run_socket_fd, command_string);

		htxclient_receive_response(run_socket_fd, &response_object);
		if(response_object.return_code == -1) {
		} else {
			PRTMSG(MSGLINE, 0, ("Running MDT : %s", global_selected_mdt_name) );
		}	

		if(response_object.response_buffer != NULL) {
			free(response_object.response_buffer);
		}

		htxclient_close_socket(run_socket_fd);

	}
	return 0;
}



int htxscreen_process_activate_halt(int device_position, int operation_mode, int operation)
{
	htxclient_command	command_object_activate_halt;
	htxclient_response	response_object_activate_halt;
	char			command_string[COMMAND_STRING_LENGTH];
	int			socket_fd_activate_halt;	
	int 			return_code = 0;



	initialize_command_object(&command_object_activate_halt, "-activate_halt");
	initialize_response_object(&response_object_activate_halt);
	sprintf(command_object_activate_halt.option_list, "%d %d %d", device_position, operation_mode, operation);
	socket_fd_activate_halt = htxclient_create_socket();
	htxclient_connect(socket_fd_activate_halt, command_object_activate_halt.sut_hostname, command_object_activate_halt.daemon_port_number);

	htxclient_prepare_command_string(&command_object_activate_halt, command_string);
	htxclient_send_command(socket_fd_activate_halt, command_string);
	return_code = htxclient_receive_response(socket_fd_activate_halt, &response_object_activate_halt);
	htxclient_close_socket(socket_fd_activate_halt);

	if(response_object_activate_halt.response_buffer != NULL) {
		free(response_object_activate_halt.response_buffer);
		response_object_activate_halt.response_buffer = NULL;
	}

	return return_code;
}



int htxscreen_activate_halt_device(void)
{

	int			row = 0;                /* first row displayed on screen     */
	int			col = 0;                /* first column displayed on screen  */
	int			i;
	char			input[32];              /* input string                      */
	htxclient_command	command_object_screen_2;
	htxclient_response	response_object_screen_2;
	int			socket_fd_screen_2;	
	int			device_display_start_position = 0;
	char			command_string[COMMAND_STRING_LENGTH];
	activate_halt_entry	*p_activate_halt_row;
	int			return_code = 0;
	int			errno_save;
	int			device_position;
	char			temp_str[128];
	int                     screen_refresh_flag = 1;
	int			total_device_count;
	int			device_count_on_screen;
	int			device_count_per_page = SCREEN_2_4_DEVICES_PER_PAGE;
	int			column_adjust;	



#ifdef  __HTX_LINUX__
	column_adjust = 0;
#else
	column_adjust = 2;
#endif
	total_device_count = htxscreen_get_total_device_count();

	while(1) {
		if(screen_refresh_flag == 1) {
			initialize_command_object(&command_object_screen_2, "-screen_2");
			initialize_response_object(&response_object_screen_2);
			socket_fd_screen_2 = htxclient_create_socket();
			htxclient_connect(socket_fd_screen_2, command_object_screen_2.sut_hostname, command_object_screen_2.daemon_port_number);

			/*sprintf(command_object_screen_2.option_list, "%d", device_display_start_position);
			htxclient_prepare_command_string(&command_object_screen_2, command_string);
			htxclient_send_command(socket_fd_screen_2, command_string);
			*/

			sprintf(command_object_screen_2.option_list, "%d", device_display_start_position);
			htxclient_prepare_command_string(&command_object_screen_2, command_string);
			htxclient_send_command(socket_fd_screen_2, command_string);
			return_code = htxclient_receive_response(socket_fd_screen_2, &response_object_screen_2);
			p_activate_halt_row = (activate_halt_entry*)response_object_screen_2.response_buffer;
			htxclient_close_socket(socket_fd_screen_2);

			clear();
			refresh();
			htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "AHD_scn", 1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
	
			htxscreen_display_htx_screen_header();

			device_count_on_screen = 0;
			for( i = 0; i < device_count_per_page; i++) {
#ifdef  __HTX_LINUX__
				wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
				wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
				wmove(stdscr, (i + 5), 3);
				wprintw(stdscr, "%2d", i + 1);

#ifdef  __HTX_LINUX__
				wcolorout(stdscr, RED_BLACK | NORMAL | BOLD);
#else
				wcolorout(stdscr, NORMAL | F_RED | B_BLACK | BOLD);
#endif	

				if(strlen(p_activate_halt_row->device_name) != 0) {
					if (strncmp(p_activate_halt_row->device_status, " HALTED", 7) == 0) {
#ifdef  __HTX_LINUX__
						wcolorout(stdscr, RED_BLACK | STANDOUT | BOLD);
#else
						wcolorout(stdscr, STANDOUT | F_RED | B_BLACK | BOLD);
#endif
					}
					mvwaddstr(stdscr, (i + 5), 6, p_activate_halt_row->device_status);
					sprintf(temp_str, " %.4d ", p_activate_halt_row->slot);
					mvwaddstr(stdscr, (i + 5), 15, temp_str);
			
					sprintf(temp_str, " %.4d ", p_activate_halt_row->port);
					mvwaddstr(stdscr, (i + 5), 22, temp_str);
					device_count_on_screen++;
				} else {
					mvwaddstr(stdscr, (i + 5), 6, "        ");
					mvwaddstr(stdscr, (i + 5), 15, "      ");
					mvwaddstr(stdscr, (i + 5), 22, "      ");
				}	
				sprintf(temp_str, " %-7s ", p_activate_halt_row->device_name);
				mvwaddstr(stdscr, (i + 5 ), (29 + column_adjust), temp_str);
			
				sprintf(temp_str, " %-7s ", p_activate_halt_row->adapt_desc);
				mvwaddstr(stdscr, (i + 5), (39 + column_adjust), temp_str);
			
				sprintf(temp_str, " %-7s ", p_activate_halt_row->device_desc);
				mvwaddstr(stdscr, (i + 5), (53 + column_adjust), temp_str);
			
				p_activate_halt_row++;	
			}

			if(response_object_screen_2.response_buffer != NULL) {
				free(response_object_screen_2.response_buffer);
				response_object_screen_2.response_buffer = NULL;
			}

			if (wrefresh(stdscr) == -1) {
				errno_save = errno;
				PRTMSG(MSGLINE, 0, ("Error on wrefresh().  errno = %d.",errno_save));
				sleep((unsigned) 10);
			}
		}

		strncpy(input, "", DIM(input));     /* clear input */
		htxscreen_get_string(stdscr, AH_IN_ROW, AH_IN_COL, input, (size_t) DIM(input), (char *) NULL, (tbool) TRUE);
		CLRLIN(MSGLINE, 0);
		screen_refresh_flag = 1;

		switch (input[0]) {
		case 'q':
		case 'Q':
			return 0;

		case 'd':
		case 'D':
			screen_refresh_flag = 1;
			break;

		case 'b':
		case 'B':
			if( (device_display_start_position - device_count_per_page) >= 0) {
				device_display_start_position -= device_count_per_page;
			} else {
				PRTMSG(MSGLINE, 0, ("Already at first page"));
				screen_refresh_flag = 0;
			}
			break;

		case 'f':
		case 'F':
			if( (device_display_start_position + device_count_per_page) < total_device_count) {
				device_display_start_position += device_count_per_page;
			} else {
				PRTMSG(MSGLINE, 0, ("Already at last page"));
				screen_refresh_flag = 0;
			}
			break;

		case 'a':
			if(global_master_mode == 1) {
				htxscreen_process_activate_halt(device_display_start_position, HTX_SCREEN_DEVICES, 0);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
			break;

		case 'A':
			if(global_master_mode == 1) {
				htxscreen_process_activate_halt(0, HTX_ALL_DEVICES, 0);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
			break;

		case 'h':
			if(global_master_mode == 1) {
				htxscreen_process_activate_halt(device_display_start_position, HTX_SCREEN_DEVICES, 1);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
			break;

		case 'H':
			if(global_master_mode == 1) {
				htxscreen_process_activate_halt(0, HTX_ALL_DEVICES, 1);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
			break;

		default:
			device_position = atoi(input);
			if( (device_position < 1) ) {
				PRTMSG(MSGLINE, 0, ("Invalid key is entered"));
				screen_refresh_flag = 0;
				break;
			}
			if( device_position > device_count_on_screen) {
				PRTMSG(MSGLINE, 0, ("Exceeded entry"));
				screen_refresh_flag = 0;
				break;
			}

			if(global_master_mode == 1) {
				htxscreen_process_activate_halt(device_display_start_position + device_position -1, HTX_SINGLE_DEVICE, 0);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
		}

	}

	return return_code;
}



int htxscreen_process_coe_hoe(int device_position, int operation_mode, int operation)
{
	htxclient_command	command_object_coe_soe;
	htxclient_response	response_object_coe_soe;
	char			command_string[COMMAND_STRING_LENGTH];
	int			socket_fd_coe_soe;	
	int 			return_code = 0;



	initialize_command_object(&command_object_coe_soe, "-coe_soe");
	initialize_response_object(&response_object_coe_soe);
	sprintf(command_object_coe_soe.option_list, "%d %d %d", device_position, operation_mode, operation);
	socket_fd_coe_soe = htxclient_create_socket();
	htxclient_connect(socket_fd_coe_soe, command_object_coe_soe.sut_hostname, command_object_coe_soe.daemon_port_number);

	htxclient_prepare_command_string(&command_object_coe_soe, command_string);
	htxclient_send_command(socket_fd_coe_soe, command_string);
	return_code = htxclient_receive_response(socket_fd_coe_soe, &response_object_coe_soe);
	htxclient_close_socket(socket_fd_coe_soe);

	if(response_object_coe_soe.response_buffer != NULL) {
		free(response_object_coe_soe.response_buffer);
		response_object_coe_soe.response_buffer = NULL;
	} 

	return return_code;
}



int htxscreen_continue_on_error_device(void)
{
	int			row = 0;                /* first row displayed on screen     */
	int			col = 0;                /* first column displayed on screen  */
	int			i;
	char			input[32];              /* input string                      */
	htxclient_command	command_object_screen_4;
	htxclient_response	response_object_screen_4;
	int			socket_fd_screen_4;	
	int			device_display_start_position = 0;
	char			command_string[COMMAND_STRING_LENGTH];
	activate_halt_entry	*p_activate_halt_row;
	int			return_code = 0;
	int			device_count_per_page = 15;
	int			errno_save;
	char			temp_str[128];
	int			device_position;
	int			total_device_count;
	int			start_device_row = 6;
	int			screen_refresh_flag = 1;
	int			device_count_on_screen;
	int			column_adjust;


#ifdef  __HTX_LINUX__
	column_adjust = 0;
#else
	column_adjust = 2;
#endif

	total_device_count = htxscreen_get_total_device_count();

	while(1) {
		if(screen_refresh_flag == 1) {
			initialize_command_object(&command_object_screen_4, "-screen_4");
			initialize_response_object(&response_object_screen_4);
			socket_fd_screen_4 = htxclient_create_socket();
			htxclient_connect(socket_fd_screen_4, command_object_screen_4.sut_hostname, command_object_screen_4.daemon_port_number);

			sprintf(command_object_screen_4.option_list, "%d", device_display_start_position);
			htxclient_prepare_command_string(&command_object_screen_4, command_string);
			htxclient_send_command(socket_fd_screen_4, command_string);
			return_code = htxclient_receive_response(socket_fd_screen_4, &response_object_screen_4);
			p_activate_halt_row = (activate_halt_entry*)response_object_screen_4.response_buffer;
			htxclient_close_socket(socket_fd_screen_4);

			clear();
			refresh();
			htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "COE_scn", 1, &row, &col, NUM_COLS, NUM_ROWS, 'n');
	
			htxscreen_display_htx_screen_header();			

			device_count_on_screen = 0;
			for( i = 0; i < device_count_per_page; i++) {
#ifdef  __HTX_LINUX__
				wcolorout(stdscr, WHITE_BLUE | NORMAL);
#else
				wcolorout(stdscr, NORMAL | F_WHITE | B_BLUE);
#endif
				wmove(stdscr, (i + start_device_row), 3);
				wprintw(stdscr, "%2d", i + 1);

#ifdef  __HTX_LINUX__
				wcolorout(stdscr, RED_BLACK | NORMAL | BOLD);
#else
				wcolorout(stdscr, NORMAL | F_RED | B_BLACK | BOLD);
#endif	

				if(strlen(p_activate_halt_row->device_name) != 0) {
					mvwaddstr(stdscr, (i + start_device_row), 6, p_activate_halt_row->device_status);
					sprintf(temp_str, " %.4d ", p_activate_halt_row->slot);
					mvwaddstr(stdscr, (i + start_device_row), 15, temp_str);
				
					sprintf(temp_str, " %.4d ", p_activate_halt_row->port);
					mvwaddstr(stdscr, (i + start_device_row), 22, temp_str);
					device_count_on_screen++;
				} else {
					mvwaddstr(stdscr, (i + start_device_row), 6, "        ");
					mvwaddstr(stdscr, (i + start_device_row), 15, "      ");
					mvwaddstr(stdscr, (i + start_device_row), 22, "      ");
				}	
				sprintf(temp_str, " %-7s ", p_activate_halt_row->device_name);
				mvwaddstr(stdscr, (i + start_device_row), (29 + column_adjust), temp_str);
			
				sprintf(temp_str, " %-7s ", p_activate_halt_row->adapt_desc);
				mvwaddstr(stdscr, (i + start_device_row), (39 + column_adjust), temp_str);
			
				sprintf(temp_str, " %-7s ", p_activate_halt_row->device_desc);
				mvwaddstr(stdscr, (i + start_device_row), (53 + column_adjust), temp_str);
			
				p_activate_halt_row++;	
			}

			if(response_object_screen_4.response_buffer != NULL) {
				free(response_object_screen_4.response_buffer);
				response_object_screen_4.response_buffer = NULL;
			}

			if (wrefresh(stdscr) == -1) {
				errno_save = errno;
				PRTMSG(MSGLINE, 0, ("Error on wrefresh().  errno = %d.",errno_save));
				sleep((unsigned) 10);
			}
		}

		strncpy(input, "", DIM(input));     /* clear input */
		htxscreen_get_string(stdscr, CE_IN_ROW, CE_IN_COL, input, (size_t) DIM(input), (char *) NULL, (tbool) TRUE);
		CLRLIN(MSGLINE, 0);
		screen_refresh_flag = 1;

		switch (input[0]) {
		case 'q':
		case 'Q':
			return 0;

		case 'b':
		case 'B':
			if( (device_display_start_position - device_count_per_page) >= 0) {
				device_display_start_position -= device_count_per_page;
			} else {
				PRTMSG(MSGLINE, 0, ("Already at first page"));
				screen_refresh_flag = 0;
			}
			break;

		case 'f':
		case 'F':
			if( (device_display_start_position + device_count_per_page) < total_device_count) {
				device_display_start_position += device_count_per_page;
			} else {
				PRTMSG(MSGLINE, 0, ("Already at last page"));
				screen_refresh_flag = 0;
			}
			break;

		case 'd':
		case 'D':
			screen_refresh_flag = 1;
			break;

		case 'c':
			if(global_master_mode == 1) {
				htxscreen_process_coe_hoe(device_display_start_position, HTX_SCREEN_DEVICES, HTX_COE);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
			break;

		case 'C':
			if(global_master_mode == 1) {
				htxscreen_process_coe_hoe(0, HTX_ALL_DEVICES, HTX_COE);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
			break;

		case 'h':
			if(global_master_mode == 1) {
				htxscreen_process_coe_hoe(device_display_start_position, HTX_SCREEN_DEVICES, HTX_SOE);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
			break;

		case 'H':
			if(global_master_mode == 1) {
				htxscreen_process_coe_hoe(0, HTX_ALL_DEVICES, HTX_SOE);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
			break;

		default:
			device_position = atoi(input);
			if( (device_position < 1) ) {
				PRTMSG(MSGLINE, 0, ("Invalid key is entered"));
				screen_refresh_flag = 0;
				break;
			}
			if( device_position > device_count_on_screen) {
				PRTMSG(MSGLINE, 0, ("Exceeded entry"));
				screen_refresh_flag = 0;
				break;
			}
			if(global_master_mode == 1) {
				htxscreen_process_coe_hoe(device_display_start_position + device_position - 1, HTX_SINGLE_DEVICE, 0);
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
				screen_refresh_flag = 0;
			}
		}
	} 

	return return_code;
}



int htxscreen_display_screen_5(void)
{
	
	int	row = 0;                /* first row displayed on screen     */
	int	col = 0;                /* first column displayed on screen  */
	int	poll_rc;
	int	key_input;		/* input from keyboard               */
	int	errno_save;		/* errno save area. */
	char	workstr[128];		/* work string                       */
	int	yr2000;			/* year -100 if >1999                */
	long	clock;			/* current time in seconds           */
	int return_code = 0;
	struct pollfd poll_fd[1];       /* poll() file descriptor structure  */
	struct tm *p_tm = NULL;		/* pointer to tm structure           */
	int	screen_5_socket_fd;
	htxclient_command command_object;
	htxclient_response response_object;
	char command_string[COMMAND_STRING_LENGTH];
	int screen_5_toggle_flag = 0;
	int i;
	int j;
	query_device_entry temp_device_entry;
	int device_entry_count;
	int device_display_start_position = 0;
	int current_page_count;
	int total_page_count;
	int requested_page_count;
	char *result_buffer_ptr;
	char *temp_result_buffer_ptr;
	int display_device_count;
	int device_display_counter;
	query_header header_object;
	query_device_entry device_entry_object[SCREEN_5_SECTIONS * SCREEN_5_ROWS];
	int mdt_stopped_flag = 0;
	static int error_beep_flag = 1;



	initialize_command_object(&command_object, "-screen_5");
	initialize_response_object(&response_object);

	poll_fd[0].fd = 0;      /* Poll stdin (0) for */
#ifdef  __HTX_LINUX__
	poll_fd[0].events = POLLIN;
#else
	poll_fd[0].reqevents = POLLIN;  /* input.             */
#endif

	clear();
	refresh();
	htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "dst_scn", 1, &row, &col, 23, 81, 'n');

	for (;;) {

		screen_5_socket_fd = htxclient_create_socket();

		htxclient_connect(screen_5_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);

		sprintf(command_object.option_list, "%d", device_display_start_position);
		htxclient_prepare_command_string(&command_object, command_string);
		htxclient_send_command(screen_5_socket_fd, command_string);

		return_code = htxclient_receive_response(screen_5_socket_fd, &response_object);
		htxclient_close_socket(screen_5_socket_fd);

		if(response_object.return_code == -1) {
			if(response_object.response_buffer != NULL) {
				free(response_object.response_buffer);
				response_object.response_buffer= NULL;
			}

			clear();
			refresh();
			global_selected_mdt_name[0] = '\0';
			htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "dst_scn", 1, &row, &col, 23, 81, 'n');
			PRTMSG(MSGLINE, 0, ("No MDT is running now"));
			mdt_stopped_flag = 1;
			if (wrefresh(stdscr) == -1) {
				errno_save = errno;
				PRTMSG(MSGLINE, 0, ("Error on wrefresh().  errno = %d.",errno_save));
				sleep((unsigned) 10);
			}

			if((poll_rc = poll(poll_fd, (unsigned long) 1, (long) 2000)) > 0) {
				key_input = getch();
				CLRLIN(MSGLINE, 0);

				switch (key_input) {
					case 'q':
					case 'Q':
						return 0;
				}
			}
			continue;
		}
	
		if(mdt_stopped_flag == 1) {
			CLRLIN(MSGLINE, 0);
			mdt_stopped_flag = 0;
		}

		memset(&header_object, 0, sizeof(query_header));

		result_buffer_ptr = response_object.response_buffer;
		sscanf(result_buffer_ptr, "%s %s %s %s %s %s", 
			header_object.display_device_count,
			header_object.system_error_flag,
			header_object.device_entry_count,
			header_object.running_mdt_name,
			header_object.current_system_time,
			header_object.test_started_time
		);

		if(global_selected_mdt_name[0] == '\0') {
			strcpy(global_selected_mdt_name, basename(header_object.running_mdt_name));
			htxscreen_display_sut_mdt_screen_5();
		}

		clock = atol(header_object.current_system_time);

		p_tm = localtime(&clock);
		if (p_tm->tm_year > 99) {
			yr2000 = p_tm->tm_year - 100;
		} else {
			yr2000 = p_tm->tm_year;
		}

		sprintf(workstr, " %.3d  %.2d/%.2d/%.2d  %.2d:%.2d:%.2d ", (p_tm->tm_yday + 1), (p_tm->tm_mon + 1), p_tm->tm_mday, yr2000, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);

#ifdef  __HTX_LINUX__
		wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif		
		mvwaddstr(stdscr, 0, 55, workstr);

		clock = atol(header_object.test_started_time);
		p_tm = localtime(&clock);
		if (p_tm->tm_year > 99) {
			yr2000 = p_tm->tm_year - 100;
		} else {
			yr2000 = p_tm->tm_year;
		}
		sprintf(workstr, " %.3d  %.2d/%.2d/%.2d  %.2d:%.2d:%.2d ", (p_tm->tm_yday + 1), (p_tm->tm_mon + 1), p_tm->tm_mday, yr2000, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);

#ifdef  __HTX_LINUX__
		wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
		mvwaddstr(stdscr, 1, 55, workstr);

		if(screen_5_toggle_flag == 0) {
			sprintf(workstr, "Cycle Curr. ");
			mvwaddstr(stdscr, 4, 26, workstr);
			mvwaddstr(stdscr, 4, 68, workstr);
			sprintf(workstr, "Count Stanza");
#ifdef  __HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | BOLD | UNDERSCORE);
#else
			wcolorout(stdscr, F_GREEN | B_BLACK | BOLD | UNDERSCORE);
#endif
			mvwaddstr(stdscr, 5, 26, workstr);
#ifdef  __HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | BOLD | UNDERSCORE);
#else
			wcolorout(stdscr, F_GREEN | B_BLACK | BOLD | UNDERSCORE);
#endif
			mvwaddstr(stdscr, 5, 68, workstr);
		} else {
			sprintf(workstr, "Last Error");
			mvwaddstr(stdscr, 4, 26, workstr);
			mvwaddstr(stdscr, 4, 68, workstr);
			sprintf(workstr, "Day   Time");
#ifdef  __HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | BOLD | UNDERSCORE);
#else
			wcolorout(stdscr, F_GREEN | B_BLACK | BOLD | UNDERSCORE);
#endif
			mvwaddstr(stdscr, 5, 26, workstr);
#ifdef  __HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | BOLD | UNDERSCORE);
#else
			wcolorout(stdscr, F_GREEN | B_BLACK | BOLD | UNDERSCORE);
#endif
			mvwaddstr(stdscr, 5, 68, workstr);
		}

		wmove(stdscr, 0, 0);
		if( strcmp(header_object.system_error_flag, "0") != 0) {
#ifdef  __HTX_LINUX__
			wcolorout(stdscr, BLACK_RED | BOLD | STANDOUT);
#else
			wcolorout(stdscr, STANDOUT | F_BLACK | B_RED | BOLD);
#endif
			waddstr(stdscr, "*** ERROR ***");
			if(error_beep_flag == 1) {
				beep();
			}
		} else {
#ifdef  __HTX_LINUX__
			wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
			wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
			waddstr(stdscr, "             ");
		}

#ifdef  __HTX_LINUX__
		wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif

		display_device_count = atoi(header_object.display_device_count);
		device_entry_count = atoi(header_object.device_entry_count);

		total_page_count = (device_entry_count / SCREEN_5_ENTRIES_PER_PAGE ) + 1;
		if(device_entry_count % SCREEN_5_ENTRIES_PER_PAGE == 0 ) {
			total_page_count--;
		}

		current_page_count = ( (device_display_start_position + 1) / SCREEN_5_ENTRIES_PER_PAGE ) + 1;
		if( (device_display_start_position + 1) % SCREEN_5_ENTRIES_PER_PAGE == 0) {
			current_page_count--;
		}

#ifdef  __HTX_LINUX__
		wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
		wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif
		sprintf(workstr, "Page Number(Cur/Max)=%d/%d", current_page_count, total_page_count);
		mvwaddstr(stdscr, 2, 0, workstr);
		

		result_buffer_ptr = strchr(response_object.response_buffer, '|');
		result_buffer_ptr++;

		memset(&device_entry_object, 0, sizeof(device_entry_object));
		for(i = 0; i < display_device_count; i++) {
			memset(&temp_device_entry, 0, sizeof(query_device_entry));
			sscanf(result_buffer_ptr, "%s %s %s %s %s %s %s %s", 
					temp_device_entry.device_name,
					temp_device_entry.run_status,
					temp_device_entry.day_of_year,
					temp_device_entry.last_update_time,
					temp_device_entry.cycle_count,
					temp_device_entry.stanza_count,
					temp_device_entry.last_error_day,
					temp_device_entry.last_error_time
			); 
			strcpy(device_entry_object[i].device_name, temp_device_entry.device_name + 1);
			strcpy(device_entry_object[i].run_status, temp_device_entry.run_status + 1);
			strcpy(device_entry_object[i].day_of_year, temp_device_entry.day_of_year + 1);
			strcpy(device_entry_object[i].last_update_time, temp_device_entry.last_update_time + 1);
			strcpy(device_entry_object[i].cycle_count, temp_device_entry.cycle_count + 1);
			strcpy(device_entry_object[i].stanza_count, temp_device_entry.stanza_count + 1);
			strcpy(device_entry_object[i].last_error_day, temp_device_entry.last_error_day + 1);
			strcpy(device_entry_object[i].last_error_time, temp_device_entry.last_error_time + 1);
			
			device_entry_object[i].device_name[strlen(temp_device_entry.device_name) - 2] = '\0';
			device_entry_object[i].run_status[strlen(temp_device_entry.run_status) - 2] = '\0';
			device_entry_object[i].day_of_year[strlen(temp_device_entry.day_of_year) - 2] = '\0';	
			device_entry_object[i].last_update_time[strlen(temp_device_entry.last_update_time) - 2] = '\0';
			device_entry_object[i].cycle_count[strlen(temp_device_entry.cycle_count) - 2] = '\0';
			device_entry_object[i].stanza_count[strlen(temp_device_entry.stanza_count) - 2] = '\0';
			device_entry_object[i].last_error_day[strlen(temp_device_entry.last_error_day) - 2] = '\0';
			device_entry_object[i].last_error_time[strlen(temp_device_entry.last_error_time) - 2] = '\0';

			temp_result_buffer_ptr = strchr(result_buffer_ptr, '|');
			result_buffer_ptr = temp_result_buffer_ptr + 1;
		}

		for(i = 0, device_display_counter = 0; i < SCREEN_5_SECTIONS; i++) {
			for (j = 1; j <= SCREEN_5_ROWS; j++) {

#ifdef  __HTX_LINUX__
				wcolorout(stdscr, GREEN_BLACK | NORMAL);
#else
				wcolorout(stdscr, NORMAL | F_GREEN | B_BLACK);
#endif

				if( device_display_counter >= (device_entry_count) ) {
					i = SCREEN_5_SECTIONS + 1;
					break;
				}
				
				if(	(strncmp(device_entry_object[device_display_counter].run_status, "ST", 2) == 0 ) ||
					(strncmp(device_entry_object[device_display_counter].run_status, "ER", 2) == 0 ) ||
					(strncmp(device_entry_object[device_display_counter].run_status, "DD", 2) == 0 ) 
				) {
#ifdef  __HTX_LINUX__
					wcolorout(stdscr, RED_BLACK | STANDOUT | BOLD);
#else
					wcolorout(stdscr, STANDOUT | B_RED | F_BLACK | BOLD);
#endif				
				}


				wmove(stdscr, (j + 5), (i * 42));
				waddstr(stdscr, device_entry_object[device_display_counter].run_status);

				wmove(stdscr, (j + 5), ((i * 42) + 3));
				waddstr(stdscr, device_entry_object[device_display_counter].device_name);
				
				wmove(stdscr, (j + 5), ((i * 42) + 12));
				waddstr(stdscr, device_entry_object[device_display_counter].day_of_year);
				
				wmove(stdscr, (j + 5), ((i * 42) + 17));
				waddstr(stdscr, device_entry_object[device_display_counter].last_update_time);

				if(screen_5_toggle_flag == 0) {
					wmove(stdscr, (j + 5), ((i * 42) + 26));
					waddstr(stdscr, device_entry_object[device_display_counter].cycle_count);

					wmove(stdscr, (j + 5), ((i * 42) + 32));
					waddstr(stdscr, device_entry_object[device_display_counter].stanza_count);
				} else {
					wmove(stdscr, (j + 5), ((i * 42) + 26));
					waddstr(stdscr, device_entry_object[device_display_counter].last_error_day);

					wmove(stdscr, (j + 5), ((i * 42) + 32));
					waddstr(stdscr, device_entry_object[device_display_counter].last_error_time);
				}
		
				device_display_counter++;
			}

		}  

		if (wrefresh(stdscr) == -1) {
			errno_save = errno;
			PRTMSG(MSGLINE, 0, ("Error on wrefresh().  errno = %d.",errno_save));
			sleep((unsigned) 10);
		}

		if (fflush(stdout) == -1) {
			errno_save = errno;
			PRTMSG(MSGLINE, 0, ("Error on fflush(stdout).  errno = %d.", errno_save));
			sleep((unsigned) 10);
			clearerr(stdout);       /* clear error flag on stdout stream */
		}


		if((poll_rc = poll(poll_fd, (unsigned long) 1, (long) 2000)) > 0) {
			key_input = getch();
			CLRLIN(MSGLINE, 0);

			switch (key_input) {
				case 'q':
				case 'Q':
					return 0;

				case 'c':
				case 'C':
					if(screen_5_toggle_flag == 0) {
						screen_5_toggle_flag = 1;
					} else {
						screen_5_toggle_flag = 0;
					}
					break;

				case 'd':
				case 'D':
					clear();
					refresh();
					htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "dst_scn", 1, &row, &col, 23, 81, 'n');
					break;

				case 'f':
				case 'F':
					clear();
					refresh();
					htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "dst_scn", 1, &row, &col, 23, 81, 'n');

					if(current_page_count < total_page_count) {
						device_display_start_position = device_display_start_position + SCREEN_5_ENTRIES_PER_PAGE;
					} else {
						PRTMSG(MSGLINE, 0, ("Already at last page"));
					}
					break;

				case 'b':
				case 'B':
					clear();
					refresh();
					htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "dst_scn", 1, &row, &col, 23, 81, 'n');

					if(current_page_count > 1) {
						device_display_start_position = device_display_start_position - SCREEN_5_ENTRIES_PER_PAGE;
					} else {
						PRTMSG(MSGLINE, 0, ("Already at first page"));
					}
					break;
				
				case 's':
				case 'S':
					if(error_beep_flag != 0) {
						error_beep_flag = 0;
						PRTMSG(MSGLINE, 0, ("Beeping turned off by operator request."));
					} else {
						PRTMSG(MSGLINE, 0, ("Beeping turned on by operator request."));
						error_beep_flag = 1;
					}	
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
					sprintf(workstr, "%c", key_input);
					requested_page_count = atoi(workstr);
					if(requested_page_count <= total_page_count) {
						device_display_start_position = (requested_page_count - 1 ) * SCREEN_5_ENTRIES_PER_PAGE;
						//PRTMSG(MSGLINE, 0, ("Page count %d  <= %d.", requested_page_count, total_page_count));
					} else {
						//PRTMSG(MSGLINE, 0, ("Page count execeeded : %d  <= %d.", requested_page_count, total_page_count));
					}
					break;

				default:
					PRTMSG(MSGLINE, 0, ("Sorry, that key is not valid with this screen."));
					refresh();
					break;
			}
		}	


		if(response_object.response_buffer != NULL) {
			free(response_object.response_buffer);
			response_object.response_buffer= NULL;
		}
	}


	return return_code;
}



int htxscreen_add_restart_terminate(void)
{

	int	row = 0;                /* first row displayed on screen     */
	int	col = 0;                /* first column displayed on screen  */
	char    input[32];              /* input string                      */


	clear();
	refresh();

	htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "ARTD_scn", 1, &row, &col, ART_NUM_ROWS, ART_NUM_COLS, 'n');

	strncpy(input, "", DIM(input));     /* clear input */
	htxscreen_get_string(stdscr, ARTD_IN_ROW, ARTD_IN_COL, input, (size_t) (2), "123dDqQ", (tbool) TRUE);

	switch (input[0]) {
		case 'q':
		case 'Q':
			return 0;

		default:
			break;
	}

	return 0;
}



int htxscreen_open_with_vi(char *filename)
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
		case 0:		/* child process                     */
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

			/*if ((int) strlen((char *) strcpy(editor, getenv("HTX_EDITOR"))) == 0) */
			/* if ((int) strlen((char *) strcpy(editor, getenv("EDITOR"))) == 0)    */
			strcpy(editor, "vi");

			if (*filename == '\0') {	/* no specified filename? run man */
				 strcpy(workstr, "-M");

 				 if (strlen(strcat(workstr, getenv("HTXPATH"))) == 0)  {
					 strcat(workstr, global_htx_home_dir);
				}

				 strcat(workstr, "/man");
				exec_rc = execl("/bin/man", "man", workstr, "htxhp", (char *) 0);
				sleep(5);
			}

			else  {
				exec_rc = execlp(editor, editor, "-R", filename, (char *) 0);
			}

			if (exec_rc == -1) {
				 sprintf(workstr, "Unable to load %s.  errno = %d", editor, errno);
				 /* send_message(workstr, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG); */
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

	return 0;
}


/* display the htx message log in a vi session */
int htxscreen_display_file(char *filename)
{

	int get_file_socket_fd;
	htxclient_command command_object;
	htxclient_response response_object;
	char dump_file[512];
	char command_string[512];
	int file_length;
	FILE *fp_dump_file;
	int return_code = 0;
	int bytes_written;
	char temp_string1[300];
	char temp_string2[300];
	

	sprintf(temp_string1, "%s/%s", global_htx_log_dir, HTX_ERR_LOG_FILE);	
	sprintf(temp_string2, "%s/%s", global_htx_log_dir, HTX_MSG_LOG_FILE);	
	if( (global_htxscreen_local_mode == 1) && ( (strcmp(filename, temp_string1) == 0 ) || (strcmp(filename, temp_string2) == 0 ) ) ) {
		htxscreen_open_with_vi(filename);
		return return_code;
	}

	initialize_command_object(&command_object, "-get_file");
	initialize_response_object(&response_object);

	get_file_socket_fd = htxclient_create_socket();
	htxclient_connect(get_file_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	sprintf(command_object.option_list, "%s", filename);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(get_file_socket_fd, command_string);
	
	return_code = htxclient_receive_response(get_file_socket_fd, &response_object);

	sprintf(dump_file, "%s/%s_%s_%s", global_htxscreen_log_dir, basename(filename),command_object.sut_hostname, global_htxscreen_start_time);
	file_length = strlen(response_object.response_buffer);
	
	fp_dump_file = fopen( dump_file, "w" );
	bytes_written = fwrite(response_object.response_buffer, 1, file_length, fp_dump_file);	
	if(bytes_written == file_length) {
	} else {
	}

	htxclient_close_socket(get_file_socket_fd);
	fclose(fp_dump_file);
	free(response_object.response_buffer);

	htxscreen_open_with_vi(dump_file);

	return return_code;
}



void	htxscreen_open_local_shell(void)
{
	char *shellname;	/* shell program name       */
	char workstr[256];	/* work string              */

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

	clear();
	refresh();

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
			shellname = getenv("SHELL");
			if( (shellname == NULL) || strlen(shellname) == 0)  {
				sprintf(workstr, ". %s/etc/scripts/htx_env.sh; /bin/sh", global_htx_home_dir);
			} else {
				sprintf(workstr, ". %s/etc/scripts/htx_env.sh; %s", global_htx_home_dir, shellname);
			}
			system(workstr);
#else
			shellname = getenv("SHELL");
			if( (shellname == NULL) || strlen(shellname) == 0)  {
				sprintf(workstr, ". %s/etc/scripts/htx_env; /bin/ksh", global_htx_home_dir);
			} else {
				sprintf(workstr, ". %s/etc/scripts/htx_env; %s", global_htx_home_dir, shellname);
			}
			system(workstr);
#endif			
			exit(0);
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
}

int htxscreen_go_to_shell(void)
{

	if(global_htxscreen_local_mode == 1) {
		htxscreen_open_local_shell();
	} else {
		PRTMSG(MSGLINE, 0, ("Not able to open system shell at remote mode"));
	}

	return 0;
}


void htxscreen_clean_selected_mdt_name(char *mdt_name, char *cleaned_mdt_name) 
{
	
	cleaned_mdt_name[0] = '\0';
	while(*mdt_name) {
		if(! isspace( *(mdt_name) ) ) {
			*cleaned_mdt_name++ = *mdt_name++;
		} else {
			mdt_name++;
		}
	}
	*cleaned_mdt_name = '\0';
}



void htxscreen_clean_string(char *string_value) 
{
	char cleaned_string[256] = {'\0'};;
	int i = 0;
	char *p_string;

	p_string = string_value;	
	cleaned_string[0] = '\0';
	while(*p_string) {
		if(! isspace( *(p_string) ) ) {
			cleaned_string[i++] = *p_string;
		}
		p_string++;
	}

	strcpy(string_value, cleaned_string);
}



int htxd_get_mdt_name_from_number(char *mdt_list, int mdt_number)
{
	char search_string[80];
	char *location_found;

	
	sprintf(search_string, "(%2d)", mdt_number);
	location_found = strstr(mdt_list, search_string);
	if(location_found == NULL) {
		return -1;
	}

	sscanf(location_found+strlen(search_string), "%s", global_selected_mdt_name);

	return 0;
}


char * htxscreen_get_mdt_list_line_start(char *mdt_list, int page_count, int lines_per_page)
{
	int text_length;
	int i;
	int line_count = 1;
	char *temp_str = mdt_list;


	if(page_count == 0) {
		return mdt_list;
	}
	
	text_length = strlen(mdt_list);
	for(i =0; i < text_length; i++) {
		if( *(mdt_list++) == '\n') {
			if(line_count == page_count * lines_per_page){
				return mdt_list;
			}
			line_count++;
		}
	}	
	return temp_str;
}



int htxscreen_mdt_list_display(int previous_return_code)
{

	int return_code = 0;
	int row	= 0;
	int column = 0;
	char command_string[512];
	htxclient_command command_object;
	htxclient_response response_object;
	int get_file_socket_fd;
	int total_lines;
	int total_pages;
	int lines_per_page = 17;
	int i;
	int page_count = 0;
	int refresh_flag = 1;
	char temp_mdt_name[MAX_ECG_NAME_LENGTH] = {'\0'};
	char temp_string[128];
	char input[32];
	int quit_flag = 0;
	char *line_start;
	char *line_end;
	int mdt_number = 0;
	char error_tag[100] = {'\0'};




	initialize_command_object(&command_object, "-listmdt");
	initialize_response_object(&response_object);

	get_file_socket_fd = htxclient_create_socket();
	htxclient_connect(get_file_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(get_file_socket_fd, command_string);

	htxclient_receive_response(get_file_socket_fd, &response_object);
	htxclient_close_socket(get_file_socket_fd);

	if(response_object.return_code == -1) {
		global_selected_mdt_name[0] = '\0';
	} else {
		total_lines = htxscreen_get_mdt_list_line_count(response_object.response_buffer);
		total_pages = (total_lines/lines_per_page);

		do {
			if(refresh_flag == 1) {
				htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "mdt_list", 1, &row, &column, 23, 81, 'n');
				htxscreen_display_htx_screen_header();

				sprintf(temp_string, "[%2d/%2d]", page_count+1, total_pages+1);	
				mvwaddstr(stdscr, 0, 70, temp_string);

				line_start = htxscreen_get_mdt_list_line_start(response_object.response_buffer, page_count, lines_per_page);
				for ( i = 0; i < lines_per_page; i++) {
					if( page_count * lines_per_page + i + 2 > total_lines) {
						break;
					}
					memset(temp_string, 0, sizeof(temp_string));
					line_end = strchr(line_start, '\n');
					strncpy(temp_string, line_start, line_end - line_start);
					mvwaddstr(stdscr, 2 + i, 3, temp_string);	
					line_start = line_end + 1;
				}

				refresh();
				if(previous_return_code == 0) {
					PRTMSG(MSGLINE, 0, ("%s Please enter a valid option (s, f, b q)", error_tag));
				} else {
					previous_return_code = 0;
				}
			}

			memset(input, 0, DIM(input));     /* clear input */
			htxscreen_get_string(stdscr, 21, 11, input, (size_t) DIM(input), "sSfFbBqQDd1234567890", (tbool) TRUE);			
			CLRLIN(MSGLINE, 0);
			error_tag [0] = '\0';
			switch (input[0]) {
			case 's':
			case 'S':
				if(global_master_mode == 1) {
					echo();
					do {
						PRTMSG(MSGLINE, 0, ("Enter MDT to select: "));	
						getstr(temp_mdt_name);
						htxscreen_clean_selected_mdt_name(temp_mdt_name, global_selected_mdt_name);
						if(global_selected_mdt_name[0] == '\0') {
							PRTMSG(MSGLINE, 0, ("Invalid entry, please enter a valid MDT"));
						}
							
					} while (global_selected_mdt_name[0] == '\0');
					PRTMSG(MSGLINE, 0, ("Selected MDT is <%s>", global_selected_mdt_name));
					noecho();	
					quit_flag = 1;
				} else {
					PRTMSG(MSGLINE, 0, ("MDT is unbale selected in view mode") );
					refresh_flag = 0;
				}
				break;

			case 'f':
			case 'F':
				if(page_count >= total_pages) {
					PRTMSG(MSGLINE, 0, ("Already at the last page") );
					refresh_flag = 0;
				} else {
					page_count++;
					refresh_flag = 1;
				}
				break;

			case 'b':
			case 'B':
				if(page_count <= 0) {
					PRTMSG(MSGLINE, 0, ("Already at the first page") );
					refresh_flag = 0;
				} else {
					page_count--;
					refresh_flag = 1;
				}
				break;

			case 'q':
			case 'Q':
				CLRLIN(MSGLINE, 0);
				return_code = 0;
				quit_flag = 1;
				break;

			case 'd':
			case 'D':
				refresh_flag = 1;

			default:
				if(strlen(input) > 0) {
					mdt_number = atoi(input);
					if(mdt_number != 0) {
						return_code = htxd_get_mdt_name_from_number(response_object.response_buffer, mdt_number);
						if(return_code != -1) {
							PRTMSG(MSGLINE, 0, ("Selected MDT is <%s>", global_selected_mdt_name));
							quit_flag = 1;
						} else {
							sprintf(error_tag, "Invalid MDT number <%s>", input);
							PRTMSG(MSGLINE, 0, ("%s, please provide a valid input", error_tag)); 
						}
					} else {
						sprintf(error_tag, "Invalid MDT number <%s>", input);
					}
				}
			}
		} while(quit_flag == 0);
	}

	if(response_object.response_buffer != NULL) {
		free(response_object.response_buffer);
	}

	if(global_selected_mdt_name[0] != '\0') {
		return_code = htxscreen_select_mdt(global_selected_mdt_name);
		if(return_code == HTXD_HTX_PROCESS_FOUND) {
			PRTMSG((MSGLINE - 2), 0, ("HTX test is already running on the system, please shutdown the running test"));
			PRTMSG((MSGLINE - 1), 0, ("%s/htx/htxd/%s contains the HTX process details, exiting..", global_htx_log_dir, HTXD_PROCESS_CHECK_LOG));
			PRTMSG(MSGLINE, 0, ("Press any key to exit..."));
			getch();
			exit(return_code);
		} else if(return_code != 0) {
			PRTMSG(MSGLINE, 0, ("Selected MDT <%s> is invalid", global_selected_mdt_name));
			global_selected_mdt_name[0] = '\0';
			return return_code;
		}
	}

	return return_code;
}



int htxscreen_activate_halt_test(char *result_string)
{
	htxclient_command command_object;
	htxclient_response response_object;
	int select_socket_fd;
	char command_string[512];


	initialize_command_object(&command_object, "-test_activate_halt");
	initialize_response_object(&response_object);
	select_socket_fd = htxclient_create_socket();
	htxclient_connect(select_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(select_socket_fd, command_string);

	htxclient_receive_response(select_socket_fd, &response_object);

	strcpy(result_string, response_object.response_buffer);

	if(response_object.response_buffer != NULL) {
		free(response_object.response_buffer);
	}
	htxclient_close_socket(select_socket_fd);

	return response_object.return_code;
}



int htxscreen_configure_test_network(char *p_bpt_file)
{
	htxclient_command command_object;
	htxclient_response response_object;
	int select_socket_fd;
	char command_string[512];
	char result_string[256];


	initialize_command_object(&command_object, "-config_net");
	initialize_response_object(&response_object);
	if(p_bpt_file[0] == '\0') {
		sprintf(command_object.option_list, "%s", NO_BPT_NAME_PROVIDED);
	} else {
		sprintf(command_object.option_list, "%s", p_bpt_file);
	}
	select_socket_fd = htxclient_create_socket();
	htxclient_connect(select_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(select_socket_fd, command_string);

	htxclient_receive_response(select_socket_fd, &response_object);

	strcpy(result_string, response_object.response_buffer);

	if(response_object.response_buffer != NULL) {
		free(response_object.response_buffer);
	}
	htxclient_close_socket(select_socket_fd);
	PRTMSG(MSGLINE, 0, (result_string));

	return response_object.return_code;
}



int htxscreen_append_network_mdt(char *p_mdt_file)
{
	htxclient_command command_object;
	htxclient_response response_object;
	int select_socket_fd;
	char command_string[512];
	char result_string[256];


	initialize_command_object(&command_object, "-append_net_mdt");
	initialize_response_object(&response_object);
	sprintf(command_object.option_list, "%s", p_mdt_file);
	select_socket_fd = htxclient_create_socket();
	htxclient_connect(select_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(select_socket_fd, command_string);

	htxclient_receive_response(select_socket_fd, &response_object);

	strcpy(result_string, response_object.response_buffer);

	if(response_object.response_buffer != NULL) {
		free(response_object.response_buffer);
	}
	htxclient_close_socket(select_socket_fd);
	PRTMSG(MSGLINE, 0, (result_string));

	return response_object.return_code;
}



int htxscreen_test_network(void)
{
	htxclient_command command_object;
	htxclient_response response_object;
	int select_socket_fd;
	char command_string[512];
	char result_string[256];


	initialize_command_object(&command_object, "-test_net");
	initialize_response_object(&response_object);
	select_socket_fd = htxclient_create_socket();
	htxclient_connect(select_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(select_socket_fd, command_string);

	htxclient_receive_response(select_socket_fd, &response_object);

	strcpy(result_string, response_object.response_buffer);

	if(response_object.response_buffer != NULL) {
		free(response_object.response_buffer);
	}
	htxclient_close_socket(select_socket_fd);
	PRTMSG(MSGLINE, 0, (result_string));

	return response_object.return_code;
}



int htxscreen_get_mdt_name_to_append(char *p_mdt_name)
{

	int return_code = 0;
	int row	= 0;
	int column = 0;
	char command_string[512];
	htxclient_command command_object;
	htxclient_response response_object;
	int get_file_socket_fd;
	int total_lines;
	int total_pages;
	int lines_per_page = 17;
	int i;
	int page_count = 0;
	int refresh_flag = 1;
	char temp_mdt_name[MAX_ECG_NAME_LENGTH] = {'\0'};
	char temp_string[256];
	char input[32];
	int quit_flag = 0;
	char *line_start;
	char *line_end;


	initialize_command_object(&command_object, "-listmdt");
	initialize_response_object(&response_object);

	get_file_socket_fd = htxclient_create_socket();
	htxclient_connect(get_file_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(get_file_socket_fd, command_string);

	htxclient_receive_response(get_file_socket_fd, &response_object);
	htxclient_close_socket(get_file_socket_fd);
	if(response_object.return_code == -1) {
		p_mdt_name[0] = '\0';
	} else {
		total_lines = htxscreen_get_mdt_list_line_count(response_object.response_buffer);
		total_pages = (total_lines/lines_per_page);
		
		do {
			if(refresh_flag == 1) {
				htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "mdt_list", 1, &row, &column, 23, 81, 'n');

				htxscreen_display_htx_screen_header();

				sprintf(temp_string, "[%2d/%2d]", page_count+1, total_pages+1);	
				mvwaddstr(stdscr, 0, 70, temp_string);

				line_start = htxscreen_get_mdt_list_line_start(response_object.response_buffer, page_count, lines_per_page);
				for ( i = 0; i < lines_per_page; i++) {
					if( page_count * lines_per_page + i + 2 > total_lines) {
						break;
					}
					memset(temp_string, 0, sizeof(temp_string));
					line_end = strchr(line_start, '\n');
					strncpy(temp_string, line_start, line_end - line_start);
					mvwaddstr(stdscr, 2 + i, 3, temp_string);
					line_start = line_end + 1;
				}
				refresh();
			}
		
			PRTMSG(MSGLINE, 0, ("Please enter a valid option (s, f, b or q"));
	
			strncpy(input, "", DIM(input));     /* clear input */
			htxscreen_get_string(stdscr, 21, 11, input, (size_t) DIM(input), "sSfFbBqQDd", (tbool) TRUE);
			CLRLIN(MSGLINE, 0);
			switch (input[0]) {

			case 's':
			case 'S':
				if(global_master_mode == 1) {
					echo();
					do {
						PRTMSG(MSGLINE, 0, ("Enter MDT to select: "));	
						getstr(temp_mdt_name);
						htxscreen_clean_selected_mdt_name(temp_mdt_name, p_mdt_name);
						if(p_mdt_name[0] == '\0') {
							PRTMSG(MSGLINE, 0, ("Invalid entry, please enter a valid MDT"));
						}
							
					} while (p_mdt_name[0] == '\0');
					PRTMSG(MSGLINE, 0, ("Selected MDT is <%s>", p_mdt_name));
					noecho();	
					quit_flag = 1;
				} else {
					PRTMSG(MSGLINE, 0, ("MDT is unbale selected in view mode") );
					refresh_flag = 0;
				}
				break;

			case 'f':
			case 'F':
				if(page_count >= total_pages) {
					PRTMSG(MSGLINE, 0, ("Already at the last page") );
					refresh_flag = 0;
				} else {
					page_count++;
					refresh_flag = 1;
				}
				break;

			case 'b':
			case 'B':
				if(page_count <= 0) {
					PRTMSG(MSGLINE, 0, ("Already at the first page") );
					refresh_flag = 0;
				} else {
					page_count--;
					refresh_flag = 1;
				}
				break;

			case 'q':
			case 'Q':
				CLRLIN(MSGLINE, 0);
				return_code = 0;
				quit_flag = 1;
				break;

			case 'd':
			case 'D':
				refresh_flag = 1;
			}
		} while(quit_flag == 0);
	}

	if(response_object.response_buffer != NULL) {
		free(response_object.response_buffer);
	}

	return return_code;
}




int htxscreen_setup_test_network(void)
{
	int row = 0;
	int column = 0;
	char bpt_file[128]={'\0'};
	char temp_bpt_name[128];
	char mdt_name[MAX_ECG_NAME_LENGTH];
	char input[32];
	int exit_flag = 0;


	do{
		htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "config_net", 1, &row, &column, 23, 81, 'y');

		strncpy(input, "", DIM(input));     /* clear input */
		htxscreen_get_string(stdscr, 21, 11, input, (size_t) DIM(input), "12345DdqQ", (tbool) TRUE);
		CLRLIN(MSGLINE, 0);

		switch (input[0]) {
		case '1':
			bpt_file[0] = '\0';
			PRTMSG(MSGLINE, 0, ("Configuring test network, please wait..."));
			htxscreen_configure_test_network(bpt_file);
			break;

		case '2':
			/* get bpt name */
			do{
				bpt_file[0] = '\0';
				memset(temp_bpt_name, '\0', sizeof(temp_bpt_name));
				PRTMSG(MSGLINE, 0, ("Enter bpt file name: "));
				echo();
				getstr(temp_bpt_name);
				noecho();
				if(strcmp(temp_bpt_name, "q")  == 0) {
					break;
				}
				htxscreen_clean_string(temp_bpt_name);
			} while(temp_bpt_name[0] == '\0'); 
			if(strcmp(temp_bpt_name, "q")  != 0) {
				strcpy(bpt_file, temp_bpt_name);
				PRTMSG(MSGLINE, 0, ("Configuring test network with bpt <%s>, please wait...", bpt_file));
				htxscreen_configure_test_network(bpt_file);
			}

			break;
			
		case '3':
			memset(mdt_name, '\0', sizeof(mdt_name));
			htxscreen_get_mdt_name_to_append(mdt_name);
			if(mdt_name[0] != '\0') {	
				htxscreen_append_network_mdt(mdt_name);
			}
			break;

		case '4':
			PRTMSG(MSGLINE, 0, ("ping test is in progress, please wait..."));
			htxscreen_test_network();
			break;

		case '5':
		case 'q':
		case 'Q':
			exit_flag = 1;
			break;

		case 'd':
		case 'D':
			break;

		}
	} while(exit_flag == 0);

	return 0;
}



int htxscreen_main_menu_execute_option(int option, daemon_states current_state)
{

	int return_code = 0;
	char result_string[256];
	char temp_string[300];


	switch (option) {

		case 1:
			if(global_master_mode == 1) {
				if( current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) {
					htxscreen_start_selected_mdt();
				} else if (current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT){
					if(current_state.test_state == 1) {
						PRTMSG(MSGLINE, 0, ("Halting the test, please wait..."));
					} else {
						PRTMSG(MSGLINE, 0, ("Activating the test, please wait..."));
					}
					htxscreen_activate_halt_test(result_string);
					PRTMSG(MSGLINE, 0, (result_string));
				} else {
					PRTMSG(MSGLINE, 0, ("MDT should be selected/running for option 1"));
				}
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
			}
			break;

		case 2:
			if( (current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT) || (current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) ) {
				htxscreen_activate_halt_device();
			} else {
				PRTMSG(MSGLINE, 0, ("MDT should be selected/running for option 2"));
			}
			break;

		case 3:
			if( (current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT) || (current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) ) {
				PRTMSG(MSGLINE, 0, ("MDT is already selected, shutdown the current MDT to select another MDT"));
			} else {
				clear();
				refresh();
				return_code = 0;
				do { /* if invalid MDT is select, try for a valid MDT */
					return_code = htxscreen_mdt_list_display(return_code);	
				} while(return_code != 0);
			}
			break;

		case 4:
			if( (current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT) || (current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) ) {
				htxscreen_continue_on_error_device();
			} else {
				PRTMSG(MSGLINE, 0, ("MDT should be selected/running for option 4"));
			}
			break;

		case 5:
			if( (current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT) || (current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) ) {
				htxscreen_display_screen_5();
			} else {
				PRTMSG(MSGLINE, 0, ("MDT should be selected/running for option 5"));
			}
			break;

		case 6:
			if(current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT) {
				PRTMSG(MSGLINE, 0, ("Updating HTX statistics file..."));
				htxscreen_display_file(STATS_FILE_STRING);
			} else {
				PRTMSG(MSGLINE, 0, ("MDT should be running for option 6"));
			}
			break;

		case 7:
			PRTMSG(MSGLINE, 0, ("Getting HTX error log ..."));
			sprintf(temp_string, "%s/%s", global_htx_log_dir, HTX_ERR_LOG_FILE);
			htxscreen_display_file(temp_string);
			break;

		case 8:
			PRTMSG(MSGLINE, 0, ("Getting HTX message log ..."));
			sprintf(temp_string, "%s/%s", global_htx_log_dir, HTX_MSG_LOG_FILE);
			htxscreen_display_file(temp_string);
			break;

		case 9:
			if(current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT || current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) {
				PRTMSG(MSGLINE, 0, ("Getting running MDT ..."));
				htxscreen_display_file(RUNNING_MDT_STRING);
			} else {
				PRTMSG(MSGLINE, 0, ("MDT should be selected/running for option 9"));
			}
			break;

		case 10:
			if(global_master_mode == 1) {
				htxscreen_add_restart_terminate();
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
			}
			break;

		case 11:
			htxscreen_go_to_shell();
			break;

		case 12:
			if(global_master_mode == 1) {
				htxscreen_setup_test_network();
			} else {
				PRTMSG(MSGLINE, 0, ("Operation is not allowed in view mode"));
			}
			break;


		default:
			break;
	}

	return 0;
}




char * htxscreen_get_selected_mdt(void)
{

	char command_string[512];
	htxclient_command command_object;
	htxclient_response response_object;
	int socket_fd;
	char temp_mdt_name[256];
	char *temp_ptr;
	int i;


	initialize_command_object(&command_object, "-getactecg");
	initialize_response_object(&response_object);

	global_selected_mdt_name[0] = '\0';

	socket_fd = htxclient_create_socket();
	htxclient_connect(socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	sprintf(command_object.option_list, "%s", " ");
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(socket_fd, command_string);

	htxclient_receive_response(socket_fd, &response_object);
	if(response_object.return_code != -1) {
		temp_ptr = strchr(response_object.response_buffer, '\n');
		temp_ptr++;
		temp_ptr = strchr(temp_ptr, '\n');
		temp_ptr++;

		i = 0;
		while(*temp_ptr != '\n') {
			temp_mdt_name[i] = *temp_ptr;
			i++;
			temp_ptr++;
		}
		temp_mdt_name[i] = '\0';
		strcpy(global_selected_mdt_name, (char *)basename(temp_mdt_name));
	}

	htxclient_close_socket(socket_fd);
	if(response_object.response_buffer != NULL) {
		free(response_object.response_buffer);
	}

	return global_selected_mdt_name;
}



int htxscreen_main_menu(void)
{

	char input1[2];         /* input string                      */
	char input2[3];         /* input string                      */
	char workstr[128];      /* work string                       */
	int main_menu_option;        /* main option integer value         */
	int shutdown_socket_fd;
	htxclient_command command_object;
	htxclient_response response_object;
	char command_string[512];
	daemon_states   current_state;



	htxscreen_get_daemon_states(&current_state);
	if( (current_state.daemon_state != HTXD_DAEMON_STATE_RUNNING_MDT) && (current_state.daemon_state != HTXD_DAEMON_STATE_SELECTED_MDT) ) {
		global_selected_mdt_name[0] = '\0';	
	} else {
		htxscreen_get_selected_mdt();
	}

	htxscreen_main_menu_screen_display(current_state);

	for (;;) {
		refresh();      /* refresh screen                  */
		strncpy(input2, "", DIM(input2));   /* clear input */
		htxscreen_get_string(stdscr, MM_IN_ROW, MM_IN_COL, input2, (size_t) DIM(input2), "0123456789eEhHmMdDsSxX", (tbool) TRUE);

		switch (input2[0]) {
			case '\0':
				break;

			case 'h':
			case 'H':
				break;

			case 'd':
			case 'D':
				clear();
				refresh();
				break;

			case 's':
				if(current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT || current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) {
					if(global_master_mode == 1) {
						sprintf(workstr, "Do you really want to shutdown MDT <%s> ? (y/n) : ", global_selected_mdt_name);
						PRTMSG(MSGLINE, 0, (workstr));
						strncpy(input1, "", DIM(input1));   /* clear input */
						htxscreen_get_string(stdscr, MSGLINE, strlen(workstr), input1, (size_t) DIM(input1), "yYnN", (tbool) TRUE);
						CLRLIN(MSGLINE, 0);
						if ((input1[0] == 'Y') || (input1[0] == 'y')) {
							PRTMSG(MSGLINE, 0, ("Shutting MDT <%s>. Please wait....", global_selected_mdt_name));
							initialize_command_object(&command_object, "-shutdown");
							initialize_response_object(&response_object);
							shutdown_socket_fd = htxclient_create_socket();
							htxclient_connect(shutdown_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
							htxclient_prepare_command_string(&command_object, command_string);
							htxclient_send_command(shutdown_socket_fd, command_string);

							htxclient_receive_response(shutdown_socket_fd, &response_object);
							if(response_object.return_code == 0) {
								PRTMSG(MSGLINE, 0, ("MDT <%s> is shutdown", global_selected_mdt_name) );
								global_selected_mdt_name[0] = '\0';
							}	

							if(response_object.response_buffer != NULL) {
								free(response_object.response_buffer);
							}

							htxclient_close_socket(shutdown_socket_fd);

						} else {
							PRTMSS(MSGLINE, 0, "                  ");
						}
					} else {
						PRTMSG(MSGLINE, 0, ("This operation is not allowed at view mode"));
					}
				} else {
					PRTMSG(MSGLINE, 0, ("No MDT is currently running"));
				}
				break;

			case 'x':
				strncpy(workstr, "Do you really want to shutdown the HTX screen ? (y/n) : ", DIM(workstr) - 1);
				PRTMSG(MSGLINE, 0, ("%s", workstr));
				strncpy(input1, "", DIM(input1));   /* clear input */
				htxscreen_get_string(stdscr, MSGLINE, strlen(workstr), input1, (size_t) DIM(input1), "yYnN", (tbool) TRUE);
				CLRLIN(MSGLINE, 0);
				if ((input1[0] == 'Y') || (input1[0] == 'y')) {
					PRTMSG(MSGLINE, 0, ("HTX screen exit has started.  Please wait...."));
					global_exit_flag = 1;
					return (1);
				} else {
					PRTMSS(MSGLINE, 0, "                  ");
				}
				break;

			default:
				main_menu_option = -1;       /* set to invalid value */
				main_menu_option = atoi(input2);
				htxscreen_main_menu_execute_option(main_menu_option, current_state);
		}

		if(global_exit_flag ==  1) {
			break;
		}

		htxscreen_get_daemon_states(&current_state);

		if( (current_state.daemon_state != HTXD_DAEMON_STATE_RUNNING_MDT) && (current_state.daemon_state != HTXD_DAEMON_STATE_SELECTED_MDT) ) {
			global_selected_mdt_name[0] = '\0';	
		} else {
			htxscreen_get_selected_mdt();
		}


		htxscreen_main_menu_screen_display(current_state);

	}

	return 0;
}



void htxscreen_display_usage(char *program_name)
{
	printf("usage: %s [-s <SUT hostname or IP] [-f <mdt name>]] -m -v\n", program_name);
	printf("default SUT hostname is %s\n", DEFAULT_SUT_HOSTNAME);
	printf("-m : master mode\n");
	printf("-v : view mode\n");
	printf("-q : quick start\n");
}



void htxscreen_clean_exit(void)
{
	char command_string[128];


	if(stdscr != NULL) {
		clear();
		refresh();
		resetterm();
	}

	if(htxclient_global_error_text[0] != '\0') {
		printf("%s", htxclient_global_error_text);
	}

	sprintf(command_string, "rm -f %s/*%s > /dev/null 2>&1", global_htxscreen_log_dir, global_htxscreen_start_time);
	system(command_string);

	printf("\n");	
}


int htxscreen_get_level_details(void)
{

	int return_code = 0;
	char level_filename[512];
	char command_string[512];
	char temp_level_string[512];
	htxclient_command command_object;
	htxclient_response response_object;
	int get_file_socket_fd;
	int level_info_length;

#ifdef  __HTX_LINUX__
	sprintf(level_filename, "%s/%s", global_htx_home_dir, HTXLINUXLEVEL);
#else
	sprintf(level_filename, "%s/%s", global_htx_home_dir, HTXAIXLEVEL);
#endif

	initialize_command_object(&command_object, "-get_file");
	initialize_response_object(&response_object);

	get_file_socket_fd = htxclient_create_socket();
	htxclient_connect(get_file_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	sprintf(command_object.option_list, "%s", level_filename);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(get_file_socket_fd, command_string);

	return_code = htxclient_receive_response(get_file_socket_fd, &response_object);
	if(response_object.return_code == -1) {
		global_htx_level_info[0] = '\0';
	} else {
		strcpy(temp_level_string, response_object.response_buffer);
#ifndef  __HTX_LINUX__
		htxscreen_clean_string(temp_level_string);	
#endif
		level_info_length = strlen(temp_level_string);
		if(level_info_length > 70) {
			temp_level_string[70] = '\0';
		} else {
			temp_level_string[level_info_length -1 ] = '\0';
		}
		sprintf(global_htx_level_info, "  [ %s ]", temp_level_string);
	}

	htxclient_close_socket(get_file_socket_fd);
	free(response_object.response_buffer);

	return return_code;
}


#if 0
int htxscreen_get_daemon_states(daemon_states* p_states)
{

	int return_code = 0;
	char command_string[512];
	htxclient_command command_object;
	htxclient_response response_object;
	int socket_fd;


	initialize_command_object(&command_object, "-get_daemon_state");
	initialize_response_object(&response_object);

	socket_fd = htxclient_create_socket();
	htxclient_connect(socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	sprintf(command_object.option_list, "%s", " ");
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(socket_fd, command_string);

	return_code = htxclient_receive_response(socket_fd, &response_object);
	if(response_object.return_code == -1) {
		daemon_state = -1;
		p_states->daemon_state = -1;
	} else {
		sprintf(response_object.response_buffer, "%d %d", &(p_states->daemon_state), &(p_states->test_state));
//		daemon_state = atoi(response_object.response_buffer);
	}

	htxclient_close_socket(socket_fd);
	free(response_object.response_buffer);

	return 0;
}
#endif


void htxscreen_get_parameters(int argument_count, char *argument_vector[])
{
	int parameter_switch;
	global_parameter.sut_hostname[0] = '\0';
	global_parameter.ecg_name[0] = '\0';
	global_parameter.daemon_port_number = 0;
	global_parameter.master_mode = 0;
	global_parameter.view_mode = 0;
	global_parameter.quick_start = 0;

	
	while ((parameter_switch = getopt (argument_count, argument_vector, "s:p:f:mvq")) != -1) {
		switch (parameter_switch) {
			case 's':
				strcpy(global_parameter.sut_hostname, optarg);
				global_htxscreen_local_mode = 0;
				break;

			case 'p':
				global_parameter.daemon_port_number = atoi(optarg);
				if(global_parameter.daemon_port_number == 0) {
					printf("ERROR: invalid port number\n");
					htxscreen_display_usage(argument_vector[0]);
				}
				break;

			case 'f':
				strcpy(global_parameter.ecg_name, optarg);
				break;

			case 'm':
				if(global_parameter.view_mode == 1) {
					printf("ERROR: Can note set master and client mode together, exiting...");
					exit(1);
				}
				global_parameter.master_mode = 1;
				break;

			case 'v':
				if(global_parameter.master_mode == 1) {
					printf("ERROR: Can note set master and client mode together, exiting...");
					exit(1);
				}
				global_parameter.view_mode = 1;
				break;
			
			case 'q':
				global_parameter.quick_start = 1;
				break;

			case '?':
				htxscreen_display_usage(argument_vector[0]);
				exit(1);
			
			default:
				htxscreen_display_usage(argument_vector[0]);
				exit(1);
			
		}
	}

	if(global_parameter.sut_hostname[0] == '\0') {
		strcpy(global_parameter.sut_hostname, DEFAULT_SUT_HOSTNAME);
	}

	if(global_parameter.daemon_port_number == 0) {
		global_parameter.daemon_port_number = HTXD_DEFAULT_PORT;
	}
}



#if 0
char * htxscreen_get_mdt_list_line_start(char *mdt_list, int page_count, int lines_per_page)
{
	int text_length;
	int i;
	int line_count = 0;
	char *temp_str = mdt_list;


	if(page_count == 0) {
		return mdt_list;
	}
	
	text_length = strlen(mdt_list);
	for(i =0; i < text_length; i++) {
		if( *(mdt_list++) == '\n') {
			if(line_count == page_count * lines_per_page){
				mdt_list++;
				return mdt_list;
			}
			line_count++;
		}
	}	
	return temp_str;
}
#endif

int htxscreen_create_mdt(void)
{
	htxclient_command command_object;
	htxclient_response response_object;
	int create_mdt_socket_fd;
	char command_string[512];


	initialize_command_object(&command_object, "-createmdt");
	initialize_response_object(&response_object);
	create_mdt_socket_fd = htxclient_create_socket();
	htxclient_connect(create_mdt_socket_fd, command_object.sut_hostname, command_object.daemon_port_number);
	htxclient_prepare_command_string(&command_object, command_string);
	htxclient_send_command(create_mdt_socket_fd, command_string);

	htxclient_receive_response(create_mdt_socket_fd, &response_object);
	if(response_object.return_code == -1) { 
	} else {
	}

	if(response_object.response_buffer != NULL) {
		free(response_object.response_buffer);
	}
	htxclient_close_socket(create_mdt_socket_fd);

	return 0;
}



int htxscreen_htx_logo_screen_display(void)
{
	int	row	= 0;
	int	column = 0;
	int	return_code = 0;
	char	temp_string[128];
	
	
	htxscreen_display_screen(stdscr, 0, 0, LINES, COLS, "htx_scn", 1, &row, &column, 23, 81, 'n');
	mvwaddstr(stdscr, 21, 1, global_htx_level_info);
	if(global_master_mode == 1) {
		sprintf(temp_string, MASTER_MODE_DISPLAY);
	} else {
		sprintf(temp_string, VIEW_MODE_DISPLAY);
	}
	mvwaddstr(stdscr, 0, MODE_DISPLAY_OFFSET, temp_string);

	wrefresh(stdscr);

	PRTMSG(MSGLINE, 0, ("Creating system MDTs, please wait..."));
	htxscreen_create_mdt();

	clear();
	refresh();
	return_code = 0;
	if(global_parameter.ecg_name[0] != '\0') {
		strcpy(global_selected_mdt_name, global_parameter.ecg_name);
		return_code = htxscreen_select_mdt(global_selected_mdt_name);
		if(return_code == HTXD_HTX_PROCESS_FOUND) {
			PRTMSG((MSGLINE - 2), 0, ("HTX test is already running on the system, please shutdown the running test"));
			PRTMSG((MSGLINE - 1), 0, ("%s/htx/htxd/%s contains the HTX process details, exiting..", global_htx_log_dir, HTXD_PROCESS_CHECK_LOG));
			PRTMSG(MSGLINE, 0, ("Press any key to exit..."));
			getch();
			exit(return_code);
		} else if(return_code != 0) {
			PRTMSG(MSGLINE, 0, ("Provided MDT <%s> is invalid", global_selected_mdt_name));
			global_selected_mdt_name[0] = '\0';
		}
		htxscreen_start_selected_mdt();
		//htxscreen_run_paramter_mdt();	
	} else {
		do { /* if invalid MDT is selected, try for a valid MDT again */
			return_code = htxscreen_mdt_list_display(return_code);
		} while(return_code != 0);
	}

	htxscreen_main_menu();

	return 0;
}


/* request for master mode access */
int htxscreen_get_master_mode_access(void)
{
	int master_mode;
	int get_master_mode_socket_fd;
	char response_buffer[2];


	get_master_mode_socket_fd = htxclient_create_socket();
	htxclient_connect(get_master_mode_socket_fd, global_parameter.sut_hostname, global_parameter.daemon_port_number + 1);
	htxclient_send_command(get_master_mode_socket_fd, "1");

	memset(response_buffer, 0, 2);
	htxd_receive_bytes(get_master_mode_socket_fd, response_buffer, 1);
	
	htxclient_close_socket(get_master_mode_socket_fd);	

	master_mode = atoi(response_buffer);
	
	return master_mode;
}



void *htx_screen_heart_beat(void *thread_arg)
{
	int heart_beat_socket_fd;
	char response_buffer[2];


	do {    /* sending heart beats */
		heart_beat_socket_fd = htxclient_create_socket();
		htxclient_connect(heart_beat_socket_fd, global_parameter.sut_hostname, global_parameter.daemon_port_number + 1);
		htxclient_send_command(heart_beat_socket_fd, "1");

		memset(response_buffer, 0, 2);
		htxd_receive_bytes(heart_beat_socket_fd, response_buffer, 1);
		htxclient_close_socket(heart_beat_socket_fd);

		sleep(5);  /* sleep for heart beat interval */
	} while(global_heart_beat_stop != 1);

	/* ack the daemon about heart beat stop */
	heart_beat_socket_fd = htxclient_create_socket();
	htxclient_connect(heart_beat_socket_fd, global_parameter.sut_hostname, global_parameter.daemon_port_number + 1);
	htxclient_send_command(heart_beat_socket_fd, "0");
	memset(response_buffer, 0, 2);
	htxd_receive_bytes(heart_beat_socket_fd, response_buffer, 1);
	htxclient_close_socket(heart_beat_socket_fd);

	return NULL;
}



pthread_t htxscreen_start_heart_beat(void)
{
	int return_code;
	pthread_t heart_beat_thread_id;
	pthread_attr_t thread_attrs;


	pthread_attr_init(&thread_attrs);
	pthread_attr_setdetachstate(&thread_attrs, PTHREAD_CREATE_JOINABLE);

	return_code = pthread_create(&heart_beat_thread_id, &thread_attrs, htx_screen_heart_beat, NULL);
	if(return_code != 0) {
		printf("htxscreen_start_heart_beat: pthread_create() returned with error code <%d>", return_code);
		exit(-1);	
	}

	return heart_beat_thread_id;	
}



int htxd_verify_home_path(void)
{
	struct stat file_status;
	int return_code;
	char temp_string[300];

	sprintf(temp_string, "%s/.htx_profile", global_htx_home_dir);
	return_code = stat(temp_string, &file_status);
	
	return return_code;
}


int htxscreen_get_home_htx_install_path(void) 
{
	FILE *fp_htx_install_path = NULL;


	memset(global_htx_home_dir, 0, sizeof(global_htx_home_dir));
	fp_htx_install_path = fopen("/var/log/htx_install_path", "r");
	if(fp_htx_install_path == NULL) {
		return -1;
	}
	
	fscanf(fp_htx_install_path, "%s", global_htx_home_dir);

	fclose(fp_htx_install_path);
	return 0;
}



int main(int argc, char *argv[])
{
	int master_mode_request;
	daemon_states   current_state;
	pthread_t heart_beat_thread_id;
	char *temp_env_val_ptr = NULL;
	char temp_string[300];
	int return_code;


	htxclient_global_error_text[0] = '\0';
	atexit(htxscreen_clean_exit);

	temp_env_val_ptr = getenv("TERM");
	if(strncmp("ansi", temp_env_val_ptr, 4) == 0) {
		setenv("TERM", "vt100", 1);
	}

	temp_env_val_ptr = getenv("HTX_HOME_DIR");
	if( (temp_env_val_ptr == NULL ) || (strlen(temp_env_val_ptr) == 0) ) {
		if(htxscreen_get_home_htx_install_path() != 0) {
			printf("ERROR: system environment variable HTX_HOME_DIR is not, please export HTX_HOME_DIR with value as HTX installation directory and re-try...\n");
			printf("       sample command: export HTX_HOME_DIR=<HTX installation path>\n");
			printf("exiting...\n\n");
			exit(1);
		}
	} else if( (temp_env_val_ptr != NULL) && (strlen(temp_env_val_ptr) > 0) ) {
		strcpy(global_htx_home_dir, temp_env_val_ptr);
	}

	return_code = htxd_verify_home_path();
	if(return_code != 0) {
		if( (temp_env_val_ptr != NULL ) && (strlen(temp_env_val_ptr) > 0) ) {
			printf("ERROR: system environment variable HTX_HOME_DIR is set with value <%s>, which is not HTX installation path\n", global_htx_home_dir);
		}
		printf(" please export HTX_HOME_DIR with HTX installation path and re-try...\n");
		printf("        sample command: export HTX_HOME_DIR=<HTX installation path>\n");
		printf("exiting...\n\n");
		exit(2);
	}

	temp_env_val_ptr = getenv("HTX_LOG_DIR");
	if( (temp_env_val_ptr != NULL) && (strlen(temp_env_val_ptr) > 0) ) {
		strcpy(global_htx_log_dir, temp_env_val_ptr);
		sprintf(global_htxscreen_log_dir, "%s/htxscreen", global_htx_log_dir);
	}
	sprintf(temp_string, "mkdir -p %s > /dev/null 2>&1", global_htxscreen_log_dir);
	system(temp_string);

	htxscreen_get_parameters(argc, argv);

	htxscreen_get_daemon_states(&current_state);
	if(current_state.daemon_state == -1){
		printf("unknown daemon state, exiting...\n");
		exit(-1);
	} else if( ( current_state.daemon_state != HTXD_DAEMON_STATE_IDLE) && (global_parameter.ecg_name[0] != 0) ) {
		printf("HTX daemon is not at IDLE state, shutdown the currently choosen MDT to start MDT from command option, exiting...\n");
		exit(-1);
	} else if(global_parameter.view_mode == 1) {
		master_mode_request = 0;
	} else if(current_state.daemon_state == HTXD_DAEMON_STATE_IDLE) {
		master_mode_request = 1;
	} else if(global_parameter.master_mode == 1) {
		master_mode_request = 1;
	}

	if(master_mode_request == 1) {
		global_master_mode = htxscreen_get_master_mode_access();
		if(global_master_mode == 1) {
			heart_beat_thread_id = htxscreen_start_heart_beat();
		}	
	}

	htxscreen_set_htxscreen_start_time();
	htxscreen_init_screen();
	htxscreen_get_level_details();
	if( (current_state.daemon_state == HTXD_DAEMON_STATE_SELECTED_MDT) || (current_state.daemon_state == HTXD_DAEMON_STATE_RUNNING_MDT) || (global_master_mode == 0) || (global_parameter.quick_start == 1) ) {
		htxscreen_main_menu();
	} else {
		htxscreen_htx_logo_screen_display();
	}

	if(global_master_mode == 1) {
		global_heart_beat_stop = 1;
		pthread_join(heart_beat_thread_id, NULL);   /* wait until heart beat cycle is closed */
	}

	return 0;
}
