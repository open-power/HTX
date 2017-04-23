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

/* @(#)48	1.11  src/htx/usr/lpp/htx/bin/hxssup/hxssup.c, htx_sup, htxltsbml 5/30/07 00:49:04 */

/*****************************************************************************/
/*                                                                           */
/* MODULE NAME    =    hxssup.c                                              */
/* COMPONENT NAME =    hxssup (supervisor)                                   */
/* LPP NAME       =    HTX                                                   */
/*                                                                           */
/* DESCRIPTIVE NAME =  HTX supervisor main module                            */
/*                                                                           */
/* STATUS =            Release 1 Version 0                                   */
/*                                                                           */
/* FUNCTION = Main HTX function + a few small support functions              */
/*    main = the main HTX function                                           */
/*    end_it = deletes IPC structures, resets display, and ends program      */
/*    htx_profile = reads in HTX environment parameters                      */
/*    init_screen = initializes display for use with CURSES routines         */
/*    sig_end = signal activated function which ends the program             */
/*                                                                           */
/* COMPILER OPTIONS =  -I/src/master/htx/common -g -Nn3000 -Nd4000 -Np1200   */
/*                     -Nt2000                                               */
/*                                                                           */
/* CHANGE ACTIVITY =                                                         */
/*    DATE    :LEVEL:PROGRAMMER:DESCRIPTION                                  */
/*    MMMMMMMMNMMMMMNMMMMMMMMMMNMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM */
/*    06/16/88:1.0  :J BURGESS :INITIAL RELEASE                              */
/*    04/02/97:1.1  :T Homer   :eliminated killing exercisers                */
/*    02/21/00:1.16 :R GEBHARDT:Feature 290676 Add/Terminate Exercisers      */
/*            :     :          :                                             */
/*                                                                           */
/*****************************************************************************/

#include "hxssup.h"

#ifdef	__HTX_LINUX__
#include "cfgclibdef.h"
#else
#include "cfgclibdef.h"
#endif

#include <unistd.h>

#include <sys/resource.h>

#include <sys/types.h>
#include <sys/wait.h>

void exer_running_msg(void);
int get_exer(void);
extern int stop_hotplug_monitor(void);
extern int sem_length;
extern int init_syscfg();
extern int detach_syscfg();


char ERR_WRAP[8];		/* htxerr wrap keyword               */
char HTXPATH[50];		/* HTX file system path spec.        */
char HTXSCREENS[50];		/* HTX file system path spec.        */
char MAX_ERR_SAVE[16];		/* htxerr.save max file size         */
char MAX_ERR_THRES[16];		/* error log file wrap threshold.    */
char MAX_MSG_SAVE[16];		/* htxmsg.save max file size         */
char MAX_MSG_THRES[16];		/* message log file wrap threshold.  */
char MSG_ARCHIVE[8];		/* message archival mode	     */
char MSG_WRAP[8];		/* htxmsg wrap keyword               */
char level_str[80];		/* HTX and LINUX levels */
char obuf[BUFSIZ];		/* buffer for stdout                 */
char *program_name;		/* the name of this program (argv[0]) */
char save_dir[PATH_MAX + 1];	/* original current directory   */
char stress_cycle[32];		/* # of seconds between "heartbeats" */
char stress_dev[64];		/* "heartbeat" device for stress test */

int MAX_EXER_ENTRIES;  /* maximum number of exercisers      */
int alarm_sig = FALSE;		/* set to TRUE on receipt of SIGALRM */
int random_ahd;                 /* to randomly activate/halt the devices  */
int rand_halt_low;              /* time after which to activate the devices  */
int hotplug_signal_delay;       /* delay time to send signal after hotplug */
int hotplug_restart_delay;      /* delay time to restart exercisers after hotplug */
int rand_halt_hi;               /* time after which to halt the devices  */
int editor_PID;			/* editor process id             */
int HANG_MON_PERIOD = 0;	/* Hang monitor period (0=no mon)  */
int hft_flag = FALSE;		/* set to TRUE if run on hft display */
int MAX_ADDED_DEVICES = 0;	/* set from htx_profile          */
int msgqid = -1;		/* Message Log message queue id      */
int profile_emc_mode;		/* for build_ipc()                  */
int semhe_id = -1;		/* HE semaphore id                   */
int semot_id = -1;		/* Order Table semaphore id          */
int shm_id = -1;		/* system shared memory id           */
int shutd_wait = SD_DEFAULT;	/* time from SIGTERM to SIGKILL */
int slow_shutd_wait = SLOW_SD_DEFAULT;	/* time from SIGTERM to SIGKILL */
volatile int system_call = FALSE;	/* set to TRUE before any system() */
int autostart;			    /* autostart flag  */
int sig_end_flag = 0;
int run_type_disp = 1 ;

pid_t hang_mon_PID = 0;		/* hang monitor PID                  */
pid_t random_ahd_PID = 0;       /* random ahd process id  */
pid_t hxsdst_PID = 0;		/* hxsdst PID                        */
pid_t hxsmsg_PID = 0;		/* message handler process id        */
pid_t hxstats_PID = 0;		/* statistics program process id     */
pid_t hotplug_monitor_ID = 0;

pid_t PID = 0;			/* HTX supervisor pid		   */
pid_t             equaliser_PID = 0;    /* Equaliser process id  */
int               equaliser_flag = 0;    /* Flag to start equaliser process  */

struct tm start_time;		/* System startup time.            */

unsigned int max_wait_tm = 0;	/* maximum semop wait time         */

union shm_pointers shm_addr;	/* shared memory union pointers    */
union semun semctl_arg;

/*****************************************************************************/
/*****  m a i n ( )  *********************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     main()                                                */
/*                                                                           */
/* DESCRIPTIVE NAME =  The main HTX function.                                */
/*                                                                           */
/* FUNCTION =          Invokes initialization functions, then calls          */
/*                     functions which run the system.                       */
/*                                                                           */
/* INPUT =             None - all input goes to lower level functions.       */
/*                                                                           */
/* OUTPUT =            None - all output generated by lower level functions. */
/*                                                                           */
/* NORMAL RETURN =     0 to indicate successful completion                   */
/*                                                                           */
/* ERROR RETURNS =     1 - Problem with build_ipc function                   */
/* 		       2 - Process not being executed by superuser	     */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    OTHER ROUTINES = build_ipc - allocates and initializes HTX IPC         */
/*                         structures                                        */
/*                     end_it - sends SIGTERM to all hardware exerciser      */
/*                         programs, deletes HTX IPC structures, resets      */
/*                         display, ends program when all exercisers         */
/*                         terminate.                                        */
/*                     htx_profile - reads in HTX environment parameters     */
/*                     init_screen - initializes display for CURSES routines */
/*                                                                           */
/*****************************************************************************/

int	main(int argc, char *argv[])
{
	extern short start_msg_hdl(int);
	extern int mmenu(int);
	char *tmp;
	char mdt_path[200];
	struct htx_msg_buf temp_msg_buf;

	char work_dir[PATH_MAX + 1];	/* string for chdir command      */
	int rc = 0;		/* return code                       */

	setpriority(PRIO_PROCESS, 0, -1);

#ifdef	SUPERUSER_ONLY
	if(getuid() != (uid_t) 0)  {
		fprintf(stderr, "HTX: %s process being executed by non-superuser, quitting ... have a nice day !\n", argv[0]);
		/*
		 * Process not being executed by superuser ... retval = 2
		 */
		exit(2);
	}
#endif

	/*
	 * Make sure that we're the only copy of hxssup running...
	 * */
#ifdef	__HTX_LINUX__
	rc = system("test `ps -ef | grep hxssup | grep -v \"grep hxssup\" | " "grep -v \"gdb hxssup\" | wc -l` -gt 2");

#else
	rc = system("test `ps -ef | grep hxssup | grep -v \"grep hxssup\" | " "grep -v \"dbx hxssup\" |wc -l` -gt 1");
#endif

	if (rc == 0) {		/* More than 1 hxssup?               */
		fprintf(stderr, "\nMore than one copy of hxssup running...\n" "This copy exiting...\n");
		fflush(stderr);
		exit(1);
	}	/* endif */

	program_name = argv[0];	/*  setup program_name variable.     */
	setbuf(stdout, obuf);	/*  dedicate static buffer to stdout */

#ifdef LIC_ENABLE
        rc = stx_license();
                if (rc != 0) {
                        printf("Contact HTX Support to acquire a new HTX license key.\n");
                        sleep(2);
                        exit(4);
                }
#endif

	init_screen();		/*  initialize default screen        */


	/*
	 * get HTXPATH environment variable
	 */
#ifdef	__HTX_LINUX__
	if(htx_strlen(htx_strcpy(HTXPATH, getenv("HTXPATH"))) == 0)  {
		 htx_strcpy(HTXPATH, "/usr/lpp/htx");
	}
#else
	if(htx_strlen(htx_strcpy(HTXPATH, getenv("HTXPATH"))) == 0)  {
		 htx_strcpy(HTXPATH, "/usr/lpp/htx");
	}
#endif

#ifndef __HTX_LINUX__
	getwd(save_dir);	/*  get current directory                    */
#else
	htx_strcpy(save_dir,"/usr/lpp/htx");
#endif

	htx_strncpy(work_dir, HTXPATH, PATH_MAX + 1);
	strncat(work_dir, "/bin", PATH_MAX + 1);
	chdir(work_dir);	/*  change current directory                 */

	htx_scn();			/* display htx logo screen */
	PID = getpid();

	htx_profile(&autostart);	/* process profile */
/*core_exclusion feature */

/*    FILE *myFile;
    myFile = fopen("/usr/lpp/htx/bin/core_exc.txt", "r");
    int numberArray[16];
    extern int numberArray_copy[16];
    int core_excluded = 0;
    int modified_core_count=0;
    //read file into array
    int array_index =0;
    while (array_index<16){
        numberArray[array_index]=-2;
        array_index++;
    }
    array_index=0;

    int c,core_exclusion_count;
        c=core_exclusion_count=0;

    if (myFile == NULL)
    {
        printf("Error Reading File\n");
        exit (0);
    }
        while (!feof (myFile)){
        fscanf(myFile, "%d,", &(numberArray[c]) );
        c++;
    }

    for (core_exclusion_count = 0; core_exclusion_count < c-1; core_exclusion_count++)
    {
           printf("Number is: %d\n\n",numberArray[core_exclusion_count]);
    }
    fclose(myFile);*/

    /*pass_core_exclusion(&numberArray);*/

	rc = init_syscfg();           /* initialize syscfg*/ 
    if (rc != 0) {
     printf("Error: init_syscfg  failed with error code <%d>, exiting...\n", rc);
    }


	if(build_ipc() != 0)  {	/*  build IPC data structures */
		end_it(1);
	}

	if(start_msg_hdl(autostart) != 0)  {/*  start the msg handler program */
		end_it(1);
	}

	sprintf(mdt_path,"The mdt file selected is:%s/mdt/",HTXPATH);
	tmp=(char *)getenv("MDTFILE");
	strcat(mdt_path,tmp);
	(void) send_message(mdt_path, errno, HTX_SYS_INFO, HTX_SYS_MSG);

	rc = mmenu(autostart);
	end_it(0);		/* normal shutdown */

	return (0);		/* this statement only for lint      */

}				/* main() */



/*****************************************************************************/
/*****  e n d _ i t ( )  *****************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     end_it()                                              */
/*                                                                           */
/* DESCRIPTIVE NAME =  Ends the HTX programs                                 */
/*                                                                           */
/* FUNCTION =          Sends SIGTERM to all hardware exerciser programs,     */
/*                     deletes HTX IPC structures, resets display, and ends  */
/*                     HTX supervisor program when all exercisers terminate. */
/*                                                                           */
/* INPUT =             exit_code - value to be used with the exit() system   */
/*                         call                                              */
/*                     fast_flag - f = fast shutdown; s = slow (old) shutd.  */
/*                                                                           */
/* OUTPUT =            SIGTERM to all hardware exerciser programs            */
/*                         exerciser programs                                */
/*                     IPC delete requests                                   */
/*                     exit_code value passed to operating system upon end   */
/*                                                                           */
/* NORMAL RETURN =     0 to indicate successful completion                   */
/*                                                                           */
/* ERROR RETURNS =     1 - problem with build_ipc function                   */
/*                     0x80 | signal number - program stopped due to signal  */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*    OTHER ROUTINES = child_death - handles SIGCLD signal                   */
/*                     send_message - sends messages to msg handler program  */
/*                                                                           */
/*    DATA AREAS =     MSGLOGKEY message queue (identified by the msgqid     */
/*                        variable)                                          */
/*                     SHMKEY shared memory segment (this segment is pointed */
/*                        to by the shm_addr pointer)                        */
/*                     SEMHEKEY semaphores (identified by the semhe_id       */
/*                        variable)                                          */
/*                                                                           */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/

void end_it(int exit_code)
{
	int i;			/* loop counter                      */
	int exer;		/* # exercisers still running        */
	struct sigaction sigvector;	/* structure for signals */
	union shm_pointers shm_wrk_ptr;	/* pointer into shared memory        */
	int get_exer_retry_count = 0;	
	int rc = 0;
	char msg_text[512];

	chdir(save_dir);	/* restore original current directory */

	/*
	 * Send a SIGTERM signal to all the HE's, and wait until ALL exercisers
	 * have terminated.
	 */
	if (shm_addr.hdr_addr != 0) {
		(shm_addr.hdr_addr)->shutdown = 1; /* Shutdown in progress */

		/*
		 * cancel SIGCLD signal processing
		 */
		sigvector.sa_handler = SIG_IGN;
		sigvector.sa_flags = 0;	/* do not restart syscalls on sigs */
		sigaction(SIGCHLD, &sigvector, (struct sigaction *) NULL);

		if (equaliser_PID != 0) {
			(void)kill(equaliser_PID, SIGTERM);
		}

		/* release all semphore locks */
		semctl_arg.array = (ushort*) malloc(sem_length * sizeof(ushort) );
		if(semctl_arg.array == NULL) {
			sprintf(msg_text, "Unable to allocate memory for semctl SETVAL array.  errno = %d.", errno);
			PRTMSG(MSGLINE, 0, ("%s", msg_text));
			send_message(msg_text, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
			return;
		}
		memset(semctl_arg.array, 0, (sem_length * sizeof(ushort) ) );

		semctl(semhe_id, 0, SETALL, semctl_arg);

		free(semctl_arg.array);
		semctl_arg.array = NULL;

		/*
		 * send SIGTERM signals
		 */
		/*
		 * HE's
		 */
		shm_wrk_ptr.hdr_addr = shm_addr.hdr_addr + 1;	/* skip over header     */
		for (i = 0; i < (shm_addr.hdr_addr)->max_entries; i++) {
			if (((shm_wrk_ptr.HE_addr + i)->PID) != 0) {	/* Process ID defined?  */
				if((equaliser_flag == 1) && ((shm_wrk_ptr.HE_addr + i)->equaliser_halt == 1)) {
					(void) kill((shm_wrk_ptr.HE_addr + i)->PID, SIGCONT);
					sleep(2);
				}
				kill((shm_wrk_ptr.HE_addr + i)->PID, SIGTERM);
			}	/* endif */
		}		/* endfor */

		/*
		 * Wait for all exercisers to terminate
		 */
		/*
		 * HE's
		 */
		exer = 999;
		while (exer > 0 && sig_end_flag == 0) {
			/*
			 * send SIGTERM signals
			 */
			/*
			 * HE's
			 */
			shm_wrk_ptr.hdr_addr = shm_addr.hdr_addr + 1;	/* skip over header     */
			exer = get_exer();
			if(exer == -1) {
				break;
			}
			if(exer == 999) {
				if(get_exer_retry_count++  >= 500) {
					sprintf(msg_text,"popen fails and get_exer tried %d times, exiting...", get_exer_retry_count);
					send_message(msg_text, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
					break;
				}
			}
			exer--;
			if(exer<=0)
				exer = 0;
			CLRLIN(MSGLINE, 0);
			PRTMSG1(MSGLINE, 0, ("Number of exercisers still running %d", exer));
			sleep(2);
			exer_running_msg();
			sleep(2);
		}		/* while */
	}	/* endif */

	/*
	 * program
	 */
	if (hxstats_PID != 0) {
		 kill(hxstats_PID, SIGTERM);
	}	/* endif */

	if (hotplug_monitor_ID != 0) {
		kill(hotplug_monitor_ID, SIGTERM);
	} 

	/*
	 * send signal to the hang_monitor task
	 */
	if (hang_mon_PID != 0) {
		 kill(hang_mon_PID, SIGTERM);
	}	/* endif */

	waitpid(hang_mon_PID,(void *)0, 0);

	/*
         * send signal to random_ahd process
         */
        if (random_ahd_PID != 0) {
           kill(random_ahd_PID, SIGTERM);
        } /* endif  */

        waitpid(random_ahd_PID, (void *)0, 0);

	/*
	 * message handler
	 */
	sigvector.sa_handler = SIG_IGN;	/* Ignore SIGCHLD signal */
	sigaction(SIGCHLD, &sigvector, (struct sigaction *) NULL);
	send_message("This is the final system message.", 0, HTX_SYS_INFO, HTX_SYS_FINAL_MSG);

	/*
	 * get rid of all IPC structures
	 * get rid of semaphore structures
	 */

	if(semhe_id != -1)  {
//		semctl(semhe_id, (int) NULL, IPC_RMID, (struct semid_ds *) NULL);
		semctl(semhe_id, (int) NULL, IPC_RMID, semctl_arg);
	}

	if (semot_id != -1)  {
//		semctl(semot_id, (int) NULL, IPC_RMID, (struct semid_ds *) NULL);
		semctl(semot_id, (int) NULL, IPC_RMID, semctl_arg);
	}

	/*
	 * get rid of shared memory structure
	 */
	if (shm_id != -1)  {
		shmctl(shm_id, IPC_RMID, (struct shmid_ds *) NULL);
	}
	
	rc = detach_syscfg();
   if(rc != 0) {
      printf("Error: delete_syscfg return with error code <%d>\n", rc);
   }

	/*
	 * clear screen and end CURSES support
	 */
	if (editor_PID == 0) {
#ifndef	__HTX_LINUX__
		colorout(NORMAL);	/*  normal mode                   */
#endif
		CLRLIN(MSGLINE, 0);	/*  write something to really set the screen back to normal. */
		clear();		/*  clean up screen               */
		refresh();		/*  update screen                 */
		endwin();		/*  end CURSES            */
	}	/* endif */

	/*
	 * Run HTX post processing script
	 */
	resetterm();

        system("htx.cleanup");
	/*
	 * exit program
	 */
	exit(exit_code);	/*  exit main()                      */

	return;			/*  this statement only for lint     */
}				/* end_it() */



/*****************************************************************************/
/*****  h t x _ p r o f i l e ( )  *******************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     htx_profile()                                         */
/*                                                                           */
/* DESCRIPTIVE NAME =  Reads ".htx_profile" file                             */
/*                                                                           */
/* FUNCTION =          Reads HTX environment parameters from the file        */
/*                        /usr/lpp/htx/.htx_profile.                         */
/*                                                                           */
/* INPUT =             .htx_profile file - an attribute file which contains  */
/*                         the HTX environment parameter specifications.     */
/*                                                                           */
/* OUTPUT =            autostart - a string which contains the value of the  */
/*                         auto-startup flag.                                */
/*                                                                           */
/* NORMAL RETURN =     None - function type is void                          */
/*                                                                           */
/* ERROR RETURNS =     None - function type is void                          */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                                                                           */
/*    DATA AREAS =     HTXPATH - pointer to a string which contains the path */
/*                         to the top level of the HTX file hierarchy        */
/*                         (usually "/usr/lpp/htx").                         */
/*                     stress_dev - pointer to a string which contains the   */
/*                         name of the "heartbeat" device for stress lab     */
/*                         testing.  A null string means that no "heartbeat" */
/*                         is required -- non-stress test.                   */
/*                     stress_cycle - pointer to a string which contains the */
/*                         number of seconds between "heartbeats".           */
/*                         A null string means that no "heartbeat" is        */
/*                         required.                                         */
/*                                                                           */
/*****************************************************************************/

void	htx_profile(int *autostart)
{
	CFG__SFT *pro_fd;	/* pointer to MDT attribute file     */

	char stanza[4096];	/* stanza area for attribute file    */
	char workstr[128];	/* work string                       */

	htx_strcpy(workstr, HTXPATH);	/* copy HTX file system path         */
	pro_fd = cfgcopsf(htx_strcat(workstr, "/.htx_profile"));

	if ((pro_fd != (CFG__SFT *) NULL) && (cfgcrdsz(pro_fd, stanza, sizeof(stanza), (char *) NULL) == CFG_SUCC)) {
		cfgcskwd("run_mode", stanza, workstr);
		unquote(workstr);	/* no quotes                     */

		switch (*workstr) {
			case 'r':
			case 'R':
				profile_emc_mode = 0;
				break;

			default:
				profile_emc_mode = 1;
				break;
		}		/* endswitch */

		cfgcskwd("max_err_file", stanza, MAX_ERR_THRES);
		unquote(MAX_ERR_THRES);
		cfgcskwd("max_err_save", stanza, MAX_ERR_SAVE);
		unquote(MAX_ERR_SAVE);
		cfgcskwd("err_wrap", stanza, ERR_WRAP);
		unquote(ERR_WRAP);	/* no quotes */

		if ((*ERR_WRAP != 'y') && (*ERR_WRAP != 'Y')) {
			htx_strcpy(ERR_WRAP, "no");
		}

		else {
			htx_strcpy(ERR_WRAP, "yes");
		}		/* endif */

		cfgcskwd("max_msg_file", stanza, MAX_MSG_THRES);
		unquote(MAX_MSG_THRES);
		(void) cfgcskwd("msg_archive", stanza, MSG_ARCHIVE);
      		(void) unquote(MSG_ARCHIVE);
		if ((*MSG_ARCHIVE != 'y') && (*MSG_ARCHIVE != 'Y')) {
			htx_strcpy(MSG_ARCHIVE, "no");
		}
		else {
			htx_strcpy(MSG_ARCHIVE, "yes");
		}

		cfgcskwd("max_msg_save", stanza, MAX_MSG_SAVE);
		unquote(MAX_MSG_SAVE);
		cfgcskwd("msg_wrap", stanza, MSG_WRAP);
		unquote(MSG_WRAP);	/* no quotes */

		if ((*MSG_WRAP != 'y') && (*MSG_WRAP != 'Y')) {
			htx_strcpy(MSG_WRAP, "no");
		}
		else {
			htx_strcpy(MSG_WRAP, "yes");
		}		/* endif */

		cfgcskwd("htx_autostart", stanza, workstr);
		unquote(workstr);	/* no quotes */
		if ((workstr[0] != 'y') && (workstr[0] != 'Y')) {
			*autostart = 0;
		}

		else {
			*autostart = 1;
		}		/* endif */

		cfgcskwd("stress_dev", stanza, stress_dev);
		unquote(stress_dev);	/* no quotes */

		if (htx_strlen(stress_dev) == 0)  {	/* null string? */
			 htx_strcpy(stress_dev, "/dev/null");
		}

		cfgcskwd("stress_cycle", stanza, stress_cycle);
		unquote(stress_cycle);	/* no quotes */
		if (htx_strlen(stress_cycle) == 0)  {  /* null string? */
			 htx_strcpy(stress_cycle, "0");
		}

		cfgcskwd("hang_mon_period", stanza, workstr);
		unquote(workstr);	/* no quotes */
		HANG_MON_PERIOD = atoi(workstr);

		cfgcskwd("slow_shutd_wait", stanza, workstr);
		unquote(workstr);	/* no quotes */
		slow_shutd_wait = atoi(workstr);

		htx_strcpy(workstr, "0");
		cfgcskwd("max_added_devices", stanza, workstr);
		unquote(workstr);	/* no quotes */
		MAX_ADDED_DEVICES = atoi(workstr);

		(void) cfgcskwd("max_exer_entries", stanza, workstr);
		(void) unquote(workstr);               /* no quotes */
		MAX_EXER_ENTRIES = atoi(workstr);

		cfgcskwd("random_ahd", stanza, workstr);
                unquote(workstr);     /* no quotes  */
                random_ahd = atoi(workstr);

                cfgcskwd("rand_halt_hi", stanza, workstr);
                unquote(workstr);     /* no quotes  */
                rand_halt_hi = atoi(workstr);

                cfgcskwd("rand_halt_low", stanza, workstr);
                unquote(workstr);     /* no quotes  */
                rand_halt_low = atoi(workstr);

                cfgcskwd("hotplug_signal_delay", stanza, workstr);
                unquote(workstr);     /* no quotes  */
                hotplug_signal_delay = atoi(workstr);

                cfgcskwd("hotplug_restart_delay", stanza, workstr);
                unquote(workstr);     /* no quotes  */
                hotplug_restart_delay = atoi(workstr);

		/*
		 * close .htx_profile cfg file
		 */
		if (cfgcclsf(pro_fd) != CFG_SUCC) {
			PRTMSG(0, 0, ("ERROR: Unable to close .htx_profile file."));
			PRTMSG(MSGLINE, 0, ("Press any key to continue ..."));
			getch();
			fflush(stdin);
			CLRLIN(MSGLINE, 0);
		}	/* endif */
	}

	else {
		profile_emc_mode = 0;	/* REG run mode                   */
		htx_strcpy(MAX_ERR_THRES, "2000000");
		htx_strcpy(MAX_ERR_SAVE, "10000");
		htx_strcpy(ERR_WRAP, "yes");
		htx_strcpy(MAX_MSG_THRES, "4000000");
		(void) strcpy(MSG_ARCHIVE, "yes");
		htx_strcpy(MAX_MSG_SAVE, "10000");
		htx_strcpy(MSG_WRAP, "yes");
		htx_strcpy(stress_dev, "");
		htx_strcpy(stress_cycle, "");
		MAX_ADDED_DEVICES = 0;
		random_ahd = 0;
                rand_halt_hi = 5400;
                rand_halt_low = 3600;
		*autostart = 0;
		shutd_wait = SD_DEFAULT;
		HANG_MON_PERIOD = 0;
	}			/* endif */

	return;
}				/* htx_profile() */


/*****************************************************************************/
/*****  i n i t _ s c r e e n ( )  *******************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     init_screen()                                         */
/*                                                                           */
/* DESCRIPTIVE NAME =  Initializes the display screen                        */
/*                                                                           */
/* FUNCTION =          Initializes the display screen for use with the       */
/*                     various CURSES routines.                              */
/*                                                                           */
/* INPUT =             None                                                  */
/*                                                                           */
/* OUTPUT =            Various CURSES initialization routine are called      */
/*                                                                           */
/* NORMAL RETURN =     None - function type is void                          */
/*                                                                           */
/* ERROR RETURNS =     None - function type is void                          */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*                     None.                                                 */
/*                                                                           */
/*****************************************************************************/

void	init_screen(void)
{
#ifndef	__HTX_LINUX__
	do_colors = TRUE;	/* reset colors on endwin()          */
#endif
	initscr();	/* standard CURSES init function     */
	crmode();	/* turn off canonical processing     */
	noecho();	/* turn off auto echoing             */
	nonl();		/* turn off NEWLINE mapping          */

#ifdef	__HTX_LINUX__
	keypad((WINDOW *)stdscr, TRUE);
#else
	keypad(TRUE);	/* turn on key mapping               */
#endif
	clear();		/* clear screen                      */
	refresh();	/* display screen                    */

	/*
	 * This is a serious hack. Since Linux uses the POSIX compliant
	 * curses library, it does not provide the 'wcolorout()' function;
	 * it also does not provide explicit foreground and background colors
	 * as LINUX does. So we have to set up each possible combination of
	 * foreground and background colors to facilitate the use through
	 * the wcolorout() function, which is actually a macros that expands
	 * the 'wattrset()' in the Linux port.
	 *
	 * Remember, all these combinations may not work, they have just been
	 * defined. It is up to the programmer to select proper foreground
	 * and background combinations.
	 *
	 * Ashutosh S. Rajekar.
	 */
#ifdef	__HTX_LINUX__
	if(has_colors())  {
		start_color();
		init_pair(1, COLOR_BLACK, COLOR_BLACK);
		init_pair(2, COLOR_BLACK, COLOR_RED);
		init_pair(3, COLOR_BLACK, COLOR_GREEN);
		init_pair(4, COLOR_BLACK, COLOR_YELLOW);
		init_pair(5, COLOR_BLACK, COLOR_BLUE);
		init_pair(6, COLOR_BLACK, COLOR_MAGENTA);
		init_pair(7, COLOR_BLACK, COLOR_CYAN);
		init_pair(8, COLOR_BLACK, COLOR_WHITE);

		init_pair(9, COLOR_RED, COLOR_BLACK);
		init_pair(10, COLOR_RED, COLOR_RED);
		init_pair(11, COLOR_RED, COLOR_GREEN);
		init_pair(12, COLOR_RED, COLOR_YELLOW);
		init_pair(13, COLOR_RED, COLOR_BLUE);
		init_pair(14, COLOR_RED, COLOR_MAGENTA);
		init_pair(15, COLOR_RED, COLOR_CYAN);
		init_pair(16, COLOR_RED, COLOR_WHITE);

		init_pair(17, COLOR_GREEN, COLOR_BLACK);
		init_pair(18, COLOR_GREEN, COLOR_RED);
		init_pair(19, COLOR_GREEN, COLOR_GREEN);
		init_pair(20, COLOR_GREEN, COLOR_YELLOW);
		init_pair(21, COLOR_GREEN, COLOR_BLUE);
		init_pair(22, COLOR_GREEN, COLOR_MAGENTA);
		init_pair(23, COLOR_GREEN, COLOR_CYAN);
		init_pair(24, COLOR_GREEN, COLOR_WHITE);

		init_pair(25, COLOR_YELLOW, COLOR_BLACK);
		init_pair(26, COLOR_YELLOW, COLOR_RED);
		init_pair(27, COLOR_YELLOW, COLOR_GREEN);
		init_pair(28, COLOR_YELLOW, COLOR_YELLOW);
		init_pair(29, COLOR_YELLOW, COLOR_BLUE);
		init_pair(30, COLOR_YELLOW, COLOR_MAGENTA);
		init_pair(31, COLOR_YELLOW, COLOR_CYAN);
		init_pair(32, COLOR_YELLOW, COLOR_WHITE);

		init_pair(33, COLOR_BLUE, COLOR_BLACK);
		init_pair(34, COLOR_BLUE, COLOR_RED);
		init_pair(35, COLOR_BLUE, COLOR_GREEN);
		init_pair(36, COLOR_BLUE, COLOR_YELLOW);
		init_pair(37, COLOR_BLUE, COLOR_BLUE);
		init_pair(38, COLOR_BLUE, COLOR_MAGENTA);
		init_pair(39, COLOR_BLUE, COLOR_CYAN);
		init_pair(40, COLOR_BLUE, COLOR_WHITE);

		init_pair(41, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(42, COLOR_MAGENTA, COLOR_RED);
		init_pair(43, COLOR_MAGENTA, COLOR_GREEN);
		init_pair(44, COLOR_MAGENTA, COLOR_YELLOW);
		init_pair(45, COLOR_MAGENTA, COLOR_BLUE);
		init_pair(46, COLOR_MAGENTA, COLOR_MAGENTA);
		init_pair(47, COLOR_MAGENTA, COLOR_CYAN);
		init_pair(48, COLOR_MAGENTA, COLOR_WHITE);

		init_pair(49, COLOR_CYAN, COLOR_BLACK);
		init_pair(50, COLOR_CYAN, COLOR_RED);
		init_pair(51, COLOR_CYAN, COLOR_GREEN);
		init_pair(52, COLOR_CYAN, COLOR_YELLOW);
		init_pair(53, COLOR_CYAN, COLOR_BLUE);
		init_pair(54, COLOR_CYAN, COLOR_MAGENTA);
		init_pair(55, COLOR_CYAN, COLOR_CYAN);
		init_pair(56, COLOR_CYAN, COLOR_WHITE);

		init_pair(57, COLOR_WHITE, COLOR_BLACK);
		init_pair(58, COLOR_WHITE, COLOR_RED);
		init_pair(59, COLOR_WHITE, COLOR_GREEN);
		init_pair(60, COLOR_WHITE, COLOR_YELLOW);
		init_pair(61, COLOR_WHITE, COLOR_BLUE);
		init_pair(62, COLOR_WHITE, COLOR_MAGENTA);
		init_pair(63, COLOR_WHITE, COLOR_CYAN);
		init_pair(64, COLOR_WHITE, COLOR_WHITE);
	}
#endif

	return;
}				/* init_screen() */


/*****************************************************************************/
/*****  s i g _ e n d ( )  ***************************************************/
/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME =     sig_end()                                             */
/*                                                                           */
/* DESCRIPTIVE NAME =  Processes signals which end the program               */
/*                                                                           */
/* FUNCTION =          Sends the proper exit code to the end_it function to  */
/*                     show that the supervisor is ending due to a signal.   */
/*                                                                           */
/* INPUT =             sig - the value of the signal                         */
/*                                                                           */
/* OUTPUT =            The exit code which shows why the supervisor is       */
/*                     ending.                                               */
/*                                                                           */
/* NORMAL RETURN =     0x80 | sig - the exit code.                           */
/*                                                                           */
/* ERROR RETURNS =     None                                                  */
/*                                                                           */
/* EXTERNAL REFERENCES                                                       */
/*    OTHER ROUTINES = end_it - clean-up processing for ending program       */
/*                     send_message - sends messages to message handler prog */
/*                                                                           */
/*    MACROS =         PRTMSG - Print message CURSES macro                   */
/*                              (defined in common/hxiconv.h)                */
/*                                                                           */
/*****************************************************************************/
void	sig_end(int sig, int code, struct sigcontext *scp)
{
	char workstr[81];	/* work string                       */
	int pid;

	pid = getpid();
        if ( pid == PID )  {
	   if ( sig_end_flag == 0 ) {
		sprintf(workstr, "HTX supervisor canceled by signal %d.", sig);
		send_message(workstr, sig, HTX_SYS_INFO, HTX_SYS_MSG);
		sig_end_flag = 1;
		end_it((0x00000080 | sig));
	   }			/* endif */
 	   else
	       return;
	}
        else
	   exit(0);
}				/* sig_end() */

int	get_exer(void)
{
	char	buf[1024];
	FILE	*fp;
	int	exer = 999 ;
	char msg_text[512];

	if ((fp = popen("ps -ef | grep hxe | wc -l", "r")) == NULL) {
		CLRLIN(MSGLINE, 0);
		PRTMSG(MSGLINE, 0, ("popen error in get_exer"));
		sleep(5);
	} else {

		if (fgets(buf, 1024, fp) == NULL) {
			if(errno != 4) { /* skip reporting the error in case of interrupted system call */
				CLRLIN(MSGLINE, 0);
				PRTMSG(MSGLINE, 0, ("fgets error in get_exer"));
				sprintf(msg_text,"fgets error in get_exer... errno(%d)", errno);
				send_message(msg_text, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
				sleep(5);
			}
			system_call = TRUE;
			pclose(fp);
			system_call = FALSE;
			return -1;
		} else {
			sscanf(buf, "%d", &exer);
		}
	}
	system_call = TRUE;
	pclose(fp);
	system_call = FALSE;
	exer -= 2 ;
	return (exer);
}				/* end get_exer */

/*
 * Put out message if exercisers still running
 */

void exer_running_msg(void)
{
	char	buf[1024];
	FILE	*fp;

	if ((fp = popen("ps -ef | grep hxe | grep -v \"grep hxe\" | awk '{print $9}'", "r")) == NULL) {
		CLRLIN(MSGLINE, 0);
		PRTMSG(MSGLINE, 0, ("popen error in exer_running_msg"));
		sleep(5);
		return;
	}			/* endif */

	if (fgets(buf, 1024, fp) == NULL) {
		return;
	}			/* endif */

	CLRLIN(MSGLINE, 0);
	PRTMSG1(MSGLINE, 0, ("Still running - %s", buf));
	system_call = TRUE;
	pclose(fp);
	system_call = FALSE;

	return;
}				/* end exer_running_msg */

