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
/*
 * @(#)37       1.20  src/htx/usr/lpp/htx/bin/hxstats/hxstats.c, htx_stats,
 * htx61J 2/19/08 03:39:54
 */

/*
 * FUNCTIONS: SIGTERM_hdl SIGUSR1_hdl main
 */

#include "hxstats.h"
#include <scr_info.h>

#include <fcntl.h>
#include <setjmp.h>
#include <unistd.h>

#ifdef	__HTX_LINUX__
#include <sys/stat.h>
#else
#ifndef __OS400__
#include <sys/mode.h>
#endif
#endif				/* __HTX_LINUX__ */

/*
 * Error code #define's for main()
 */
#define BAD_MSGGET	(1)
#define BAD_USAGE	(2)
#define BAD_OPEN	(3)
#define BAD_DELAY_TIME	(4)
#define BAD_SHMGET	(5)
#define BAD_SHMAT	(6)
#define BAD_SET_SIGNAL_HDL1	(7)
#define BAD_SET_SIGNAL_HDL2	(8)

#define BAD_SHMDT	0x0010
#define BAD_CLOSE	0x0020

char           *program_name;	/* name of this program */
int             msgqid;		/* IPC message queue id */
int             sigtermflag = 0;/* SIGTERM signal flag */
int             siguserflag = 0;/* SIGUSR1 signal flag                          */
jmp_buf         sjbuf;		/* context switch data structure */
tmisc_shm      *rem_shm_addr;
struct htxshm_hdr *header;
int             errno_save;
char            error_msg[512];
int             exit_code;
int             savemask;
char		global_htx_log_dir[256];

/*
 * NAME: main()
 *
 * FUNCTION: Periodically updates the HTX statistics file (HE) programs.
 *
 * EXECUTION ENVIRONMENT:
 *
 * This procedure is invoked as the program "hxstats" by the HTX supervisor
 * program, "hxssup".  This statistics update program, "hxstats", is always a
 * child process of the HTX supervisor program, "hxssup".
 *
 * This "hxstats" program acts as a daemon which periodically gathers the latest
 * statistics information from the HTX IPC shared memory area and copies that
 * information to disk.
 *
 * NOTES:
 *
 * argv[] parameters: ------------------ argv[0] -- program name (hxstats)
 * argv[1] -- name of the statistics file. argv[2] -- time between file
 * updates (seconds).
 *
 *
 * operation: --------- get HTX IPC message queue check program (argv)
 * arguments open the stat file get HTX IPC shared memory attach shared
 * memory set SIGUSR1 and SIGTERM signal handlers
 *
 * do set file pointer to zero generate stat entry for current HE
 *
 *
 * exit program
 *
 *
 * RETURNS:
 *
 * Return Codes:
 * ------------------------------------------------------------------ 0 --
 * Normal exit. 1 -- Unable to msgget() HTX IPC message queue. 2 -- Invalid
 * number of call parameters. 3 -- Unable to open output file. 4 -- Invalid
 * delay time between updates (0<time<=300). 5 -- Unable to shmget() HTX IPC
 * shared memory. 6 -- Unable to shmat() (attach) HTX IPC shared memory. 7 --
 * Unable to set SIGUSR1 signal handler. 8 -- Unable to set SIGTERM signal
 * handler.
 *
 * 0x0010 -- Error on HTX IPC shared memory detach (shmdt()). 0x0020 -- Error on
 * close().
 *
 *
 */

int	main(int argc, char *argv[])
{
	char            file_buffer[2048];
	char            workstr[512];
	int             delay_time = 0;
	int             fileid = 0;
	int             i = 0;
	int             N = 0;
	int             shm_id, mem_id, shmkey;
	off_t           filelength = 0;
	struct htxshm_HE *HE = NULL, *HE_ptr = NULL;
	char		*temp_env_val_ptr = NULL;
	char		*default_log_dir = "/tmp";


	exit_code = GOOD;
	program_name = argv[0];

#ifdef	SUPERUSER_ONLY
	if (getuid() != (uid_t) 0) {
		fprintf(stderr, "HTX: %s process being executed by non-superuser, \
		quitting ... have a nice day !\n", argv[0]);
		/*
		 * Process not being executed by superuser ... retval = 2
		 */
		exit(2);
	}
#endif
	temp_env_val_ptr = getenv("HTX_LOG_DIR");
	if(temp_env_val_ptr == NULL) {
		strcpy(global_htx_log_dir, default_log_dir);
	} else {
		if(strlen(temp_env_val_ptr) == 0) {
			strcpy(global_htx_log_dir, default_log_dir);
		} else {
			strcpy(global_htx_log_dir, temp_env_val_ptr);
		}
	}


	/*
	 * First things first.  Let's go ahead and access the IPC message
	 * queue so that we can send messages to the HTX error log if we have
	 * any initialization problem.
	 */

	/*
	 * MSGLOGKEY is a key designated in HTX to represent messages sent to
	 * and received by the HTX message processing program. This key is
	 * used in the MSGGET system call to get the message queue identifier
	 * used for interprocess message switching.  See MSGGET system call
	 * in AIX Technical Reference Manual.
	 */

	errno = 0;
	if ((msgqid = msgget(MSGLOGKEY, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
			     S_IROTH | S_IWOTH)) == -1) {
		errno_save = errno;
		sprintf(error_msg,
			"%s -- Unable to access (msgget()) \
				HTX IPC message queue.\nerrno = %d (%s).\n",
			program_name, errno_save, strerror(errno_save));
		fprintf(stderr, "%s", error_msg);
		fflush(stderr);
		exit_code |= BAD_MSGGET;
	}
	/*
	 * Make sure you have all the arguments you need.
	 */

	else if (argc != 3) {
		send_message("Usage: hxstats outputfile delaytime.", 0,
			     HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
		exit_code |= BAD_USAGE;
		return (exit_code);
	}
	/*
	 * Open the output file (argv[1]).
	 */
	else if ((fileid = open(argv[1], O_RDWR | O_CREAT,
			S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1) {
		errno_save = errno;
		sprintf(error_msg, "Unable to open %s file.\nerrno = %d (%s).",
			argv[1], errno_save, strerror(errno_save));
		send_message(error_msg, errno_save, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
		exit_code |= BAD_OPEN;
	}
	else {
		/*
		 * Check for valid delay time (0 < time <= 300)
		 */
		delay_time = atoi(argv[2]);
		if ((delay_time <= 0) || (delay_time > 300)) {
			sprintf(error_msg, "Invalid delay time (0 < delay <= 300). \
								Specified delay (argv[2]) = %s.", argv[2]);
			send_message(error_msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
			exit_code = BAD_DELAY_TIME;
		}
		/***  set up signal handler functions. 88888****************************/
		else if (set_signal_hdl(SIGUSR1, SIGUSR1_hdl) != GOOD)
			exit_code = BAD_SET_SIGNAL_HDL1;
		else if (set_signal_hdl(SIGTERM, SIGTERM_hdl) != GOOD)
			exit_code = BAD_SET_SIGNAL_HDL2;

		else if ((mem_id = shmget(REMSHMKEY, 0,
					  S_IRUSR | S_IWUSR | S_IRGRP |
				      S_IWGRP | S_IROTH | S_IWOTH)) == -1) {
			errno_save = errno;
			(void) sprintf(error_msg, "Unable to get HTX shared memory for key = %d. \
						\nerrno = %d (%s)", SHMKEY, errno_save, strerror(errno_save));
			(void) send_message(error_msg, errno_save, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
			exit_code = BAD_SHMGET;
		}
		 /* endif */
		else if ((rem_shm_addr = (tmisc_shm *) malloc(4 * 4096)) == NULL) {
			errno_save = errno;
			(void) sprintf(error_msg, "Unable to get malloc rem_shm_addr\nerrno = %d (%s)",
				       errno_save, strerror(errno_save));
			(void) send_message(error_msg, errno_save, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
			exit_code = BAD_SHMGET;
		} else if ((rem_shm_addr->sock_hdr_addr = (tsys_hdr *) shmat(
			      mem_id, (char *) 0, 0)) == (tsys_hdr *) - 1) {
			errno_save = errno;
			(void) sprintf(error_msg, "Unable to attach (ex_ipc) HTX shared memory. \
							\n errno = %d (%s)", errno_save, strerror(errno_save));
			(void) send_message(error_msg, errno_save, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
			exit_code = BAD_SHMAT;
		} else {
			/*** main output loop *********************************************/
			do {
#ifndef __HTX_LINUX__
				(void) setjmp(sjbuf);
#else
				savemask = sigsetjmp(sjbuf, 1);
#endif
				/* If sig-term is already received then there is
				 * no need to do any other operation.
				 */
				if ( sigtermflag == -1 ) {
					break;
				}

				if (sigtermflag == 0 && siguserflag == 0) {	/* loop while no SIGTERM */
					(void) sleep((unsigned int) delay_time);
				}
				/*** let the hxstats start here if it gets a SIGUSR1 from stx *************/

				shmkey = rem_shm_addr->sock_hdr_addr->cur_shm_key;
				if (!shmkey)
					continue;
				if ((shm_id = shmget(shmkey, 0, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
						S_IROTH | S_IWOTH)) == -1) {
					if (sigtermflag == 0) {
						errno_save = errno;
						(void) sprintf(error_msg, "Unable to get HTX shared memory for key = %d.\
										\nerrno = %d (%s)", shmkey, errno_save, strerror(errno_save));
						(void) send_message(error_msg, errno_save, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
						exit_code = BAD_SHMGET;
					} else
						break;
				}
				/*
				 * The SHMAT system call attaches the shared
				 * memory segment associated with the shared
				 * memory identifier mem_id (from the SHMGET
				 * system call).  See SHMAT system call in
				 * AIX Technical Reference Manualfor further
				 * details.  The return value from SHMAT is
				 * cast to the structure type for hardware
				 * exerciser statistical accumulators and
				 * error messages.
				 */

				else if ((header = (struct htxshm_hdr *) shmat(shm_id, (char *) 0, 0)) ==
					 (struct htxshm_hdr *) - 1) {
					if (sigtermflag == 0) {
						errno_save = errno;
						sprintf(error_msg, "Unable to attach HTX shared memory.\nerrno = %d (%s)",
							errno_save, strerror(errno_save));
						send_message(error_msg, errno_save, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
						exit_code = BAD_SHMAT;
					} else
						break;
				}
				else {
					/*
					 * Set up parameters for the loop.
					 */
					N = header->pseudo_entries + header->pseudo_entry_0;

					/*
					 * stats start just after header
					 */
					HE = (struct htxshm_HE *) (header + 1);

					errno = 0;
					if (lseek(fileid, (off_t) 0, SEEK_SET) == -1) {
						errno_save = errno;
						sprintf(error_msg, "Error on lseek(fileid, 0, SEEK_SET). \
								\nerrno = %d (%s).", errno_save, strerror(errno_save));
						send_message(error_msg, errno_save, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
					}
					else {

						/*
						 * The increment in the for
						 * loop is designed to jump
						 * over the unused "added
						 * devices" entries to the
						 * pseudo device entries
						 * which follow.  So the
						 * logic for the increment
						 * looks like this:
						 *
						 * if (just did last std dev
						 * entry) then i = index to
						 * 1st pseudo entry else
						 * increment i
						 *
						 * Note that pseudo_entry_0 is
						 * the index to the first
						 * pseudo entry.
						 */

						for (i = 0; i < N; (++i == header->max_entries) ? (i = header->pseudo_entry_0) : i) {

							/*
							 * set HE_ptr to next
							 * HE memory space.
							 */
							HE_ptr = HE + i;

							if (HE_ptr->sdev_id[0]) {
								/*
								 * set file
								 * buffer to
								 * zero
								 * length str
								 */

								file_buffer[0] = '\0';
								sprintf(
								    workstr,
								    "%s:\n",
									HE_ptr->sdev_id);
								htx_strcat(
								file_buffer,
								   workstr);
								sprintf(
								    workstr,
									"  cycles                              =              %5llu\n",
									HE_ptr->cycles);
								htx_strcat(
								file_buffer,
								   workstr);
								sprintf(
								    workstr,
									"  # good reads                        =              %5llu\n",
									HE_ptr->good_reads);
								htx_strcat(
								file_buffer,
								   workstr);

								sprintf(
								    workstr,
									"  # bytes read                        = ");
								htx_strcat(
								file_buffer,
								   workstr);

								if (HE_ptr->bytes_read1) {
									sprintf(
										workstr,
										"%9llu%09llu\n",
										HE_ptr->bytes_read1,
										HE_ptr->bytes_read2);
									htx_strcat(
										   file_buffer,
										   workstr);
								} else {
									sprintf(
										workstr,
										"         %9lu\n",
										HE_ptr->bytes_read2);
									htx_strcat(
										   file_buffer,
										   workstr);
								}	/* endif */

								sprintf(
								    workstr,
									"  # good writes                       =              %5llu\n",
									HE_ptr->good_writes);

								htx_strcat(
								file_buffer,
								   workstr);
								sprintf(
								    workstr,
									"  # total instructions                =              %5llu\n",
									HE_ptr->total_num_instructions);

								htx_strcat(
								file_buffer,
								   workstr);
								sprintf(
								    workstr,
									"  # bytes written                     = ");
								htx_strcat(
								file_buffer,
								   workstr);
								if (HE_ptr->bytes_writ1) {
									sprintf(
										workstr,
										"%9llu%09llu\n",
										HE_ptr->bytes_writ1,
										HE_ptr->bytes_writ2);
									htx_strcat(
										   file_buffer,
										   workstr);
								} else {
									sprintf(
										workstr,
										"         %9llu\n",
										HE_ptr->bytes_writ2);
									htx_strcat(
										   file_buffer,
										   workstr);
								}	/* endif */

								sprintf(
								    workstr,
									"  # good others                       =              %5llu\n",
									HE_ptr->good_others);
								htx_strcat(
								file_buffer,
								   workstr);

								sprintf(
								    workstr,
									"  # bad others                        =              %5llu\n",
									HE_ptr->bad_others);
								htx_strcat(
								file_buffer,
								   workstr);
								sprintf(
								    workstr,
									"  # bad reads                         =              %5llu\n",
									HE_ptr->bad_reads);

								htx_strcat(
								file_buffer,
								   workstr);
								sprintf(
								    workstr,
									"  # bad writes                        =              %5llu\n",
									HE_ptr->bad_writes);
								htx_strcat(
								file_buffer,
								   workstr);
								sprintf(
								    workstr,
									"  # data transfer rate(bytes_wrtn/s)  =        %11.2f\n",
									HE_ptr->data_trf_rate1);

								htx_strcat(
								file_buffer,
								   workstr);
								sprintf(
								    workstr,
									"  # data transfer rate(bytes_read/s)  =        %11.2f\n",
									HE_ptr->data_trf_rate2);

								htx_strcat(
								file_buffer,
								   workstr);
								sprintf(
								    workstr,
									"  # instruction throughput(MIPS)      =        %11Lf\n\n",
									(HE_ptr->throughput
								/ 1000000));
								htx_strcat(
								file_buffer,
								   workstr);
								errno = 0;

								if (write(fileid, file_buffer,
									  (unsigned int) htx_strlen(file_buffer)) == -1) {
									errno_save = errno;
									sprintf(
										error_msg,
										"Error on write(fileid, file_buffer, n).\nerrno = %d (%s).",
										errno_save,
										strerror(errno_save));
									send_message(
										     error_msg,
										     errno_save,
										     HTX_SYS_SOFT_ERROR,
										     HTX_SYS_MSG);
								}	/* endif */
							}	/* endif */
						}	/* endfor */

						errno = 0;
						if ((filelength = lseek(fileid,
								  (off_t) 0,
								  SEEK_CUR))
						    == -1) {
							errno_save = errno;
							sprintf(
								error_msg,
								"Error on lseek(fileid, 0, SEEK_CUR).\nerrno = %d (%s).",
								errno_save,
								strerror(errno_save));
							send_message(
								  error_msg,
								 errno_save,
							 HTX_SYS_SOFT_ERROR,
							       HTX_SYS_MSG);
						} else {
							errno = 0;
							if (ftruncate(fileid,
								      (unsigned long) filelength)
							    == -1) {
								errno_save
									= errno;
								sprintf(
								  error_msg,
									"Error on ftruncate(fileid, filelength).\nerrno = %d (%s).",
								 errno_save,
									strerror(errno_save));
								send_message(
								  error_msg,
								 errno_save,
									     HTX_SYS_SOFT_ERROR,
								HTX_SYS_MSG);
							}	/* endif */
						}	/* endif */

						errno = 0;
						if (fsync(fileid) == -1) {
							errno_save = errno;
							sprintf(
								error_msg,
								"Error on fsync(fileid).\nerrno = %d (%s).",
								errno_save,
								strerror(errno_save));
							send_message(
								  error_msg,
								 errno_save,
							 HTX_SYS_SOFT_ERROR,
							       HTX_SYS_MSG);
						}	/* endif */
						sprintf(workstr, "touch %s/htxstats_done", global_htx_log_dir);
						system(workstr);
					}	/* endif */
				}	/* endif */
				if (((char *) header) && sigtermflag == 0) {
					if (shmdt((char *) header) != GOOD) {
						errno_save = errno;
						(void) sprintf(
							       error_msg,
							       "Unable to detach shared memory (shmdt()).\nerrno = %d (%s).",
							       errno_save,
						      strerror(errno_save));
						(void) send_message(
								  error_msg,
								 errno_save,
							 HTX_SYS_SOFT_ERROR,
							       HTX_SYS_MSG);
						exit_code |= BAD_SHMDT;
					}	/* endif */
				}	/* endif */
				siguserflag = 0;
			} while (sigtermflag == 0 && siguserflag == 0);	/* loop while no SIGTERM    */

			/*
			 * perform cleanup activities and exit
			 */

			errno = 0;
			if ( (sigtermflag == 0) && shmdt((char *) rem_shm_addr->sock_hdr_addr) != GOOD ) {
				errno_save = errno;
				(void) sprintf(
					       error_msg,
					       "Unable to detach ex_ipc shared memory\
								(shmdt()).\nerrno = %d (%s).",
					       errno_save,
					       strerror(errno_save));
				(void) send_message(error_msg, errno_save, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
				exit_code |= BAD_SHMDT;
			}	/* endif */
			free(rem_shm_addr);

			if (((char *) header) && (sigtermflag == 0) && (shmkey != 0)) {
				if (shmdt((char *) header) != GOOD) {
					errno_save = errno;
					sprintf(
						error_msg,
						"Unable to detach shared memory(shmdt()).\nerrno = %d (%s).",
						errno_save,
						strerror(errno_save));
					send_message(error_msg, errno_save,
						     HTX_SYS_SOFT_ERROR,
						     HTX_SYS_MSG);
					exit_code |= BAD_SHMDT;
				}	/* endif */
			}
		}		/* endif */

		errno = 0;
		if (close(fileid) == -1) {
			errno_save = errno;
			sprintf(
				error_msg,
				"Error on close(fileid).\nerrno = %d (%s).",
				errno_save, strerror(errno_save));
			send_message(error_msg, errno_save, HTX_SYS_SOFT_ERROR,
				     HTX_SYS_MSG);
			exit_code |= BAD_CLOSE;
		}		/* endif */
	}			/* endif */

 	exit(exit_code);
} /* main() */

/*****************************************************************************/
/*****  s i g u s r 1 h d l ( )  *********************************************/
/*****************************************************************************/
/* */
/* FUNCTION NAME =     sigusr1hdl()                                          */
/* */
/* DESCRIPTIVE NAME =  SIGUSR1 signal handler                                */
/* */
/* FUNCTION =          Upon receipt of the SIGUSR1 signal, this function     */
/* will jump to the beginning of the loop which outputs  */
/* the statistics file.  Thus, a current version of the  */
/* file is generated immediately.                        */
/* */
/*****************************************************************************/

void	SIGUSR1_hdl(int sig, int code, struct sigcontext *scp)
{
	siguserflag = -1;
#ifndef __HTX_LINUX__
	longjmp(sjbuf, 0);
#else
	siglongjmp(sjbuf, savemask);
#endif

}				/* SIGUSR1_hdl() */

/*****************************************************************************/
/*****  s i g t e r m h d l ( )  *********************************************/
/*****************************************************************************/
/* */
/* FUNCTION NAME =     sigtermhdl()                                          */
/* */
/* DESCRIPTIVE NAME =  SIGTERM signal handler                                */
/* */
/* FUNCTION =          Upon receipt of the SIGTERM signal, this function     */
/* will set the sigtermflag variable.                    */
/* */
/*****************************************************************************/

void	SIGTERM_hdl(int sig, int code, struct sigcontext *scp)
{
	sigtermflag = -1;
	(void) sprintf(error_msg, "SIGTERM received. sigtermflag = %d\n", sigtermflag);
	(void) send_message(error_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);

#ifndef __HTX_LINUX__
	longjmp(sjbuf, 0);
#else
	siglongjmp(sjbuf, savemask);
#endif
}				/* SIGTERM_hdl() */
