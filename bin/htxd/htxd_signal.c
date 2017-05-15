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
/* @(#)65	1.8  src/htx/usr/lpp/htx/bin/htxd/htxd_signal.c, htxd, htxubuntu 1/4/16 03:18:42 */



#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>

#include <htxd_ecg.h>
#include <htxd.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifndef __HTX_LINUX__
  #include <sys/dr.h>
#endif

#define PARMS(x) x
#include <cfgclibdef.h>

#include "htxd_define.h"
#include "htxd_instance.h"
#include "htxd_util.h"

extern volatile int htxd_shutdown_flag;



void sig_end(int sig, int code, struct sigcontext *scp)
{

	htxd_shutdown_flag = TRUE;	

	sleep(1);
}

void alarm_signal(int sig, int code, struct sigcontext *scp)
{
	printf("DEBUG: alarm_signal handler\n"); fflush(stdout);
}


void child_death(int sig, int code, struct sigcontext *scp)
{
	pid_t child_pid;
	int child_process_exit_status;	
	char exit_reason[120];


	while( (child_pid = wait3(&child_process_exit_status, WNOHANG, (struct rusage *) NULL) ) > 0  ) {
		if ( WIFEXITED(child_process_exit_status) ) {
			sprintf(exit_reason, "exit(%d) call", WEXITSTATUS(child_process_exit_status) );
		} else if ( WIFSIGNALED(child_process_exit_status) )  {
			sprintf(exit_reason, "signal %d", WTERMSIG(child_process_exit_status) );
		}	

		if(child_pid == htxd_get_htx_stats_pid() ) {
			htxd_set_htx_stats_pid(0);
		}

		else if(child_pid == htxd_get_htx_msg_pid() ) {
			htxd_set_htx_msg_pid(0);
		}
#ifdef __HTXD_DR__
		else if(child_pid == htxd_get_dr_child_pid() ) {
			htxd_set_dr_child_pid(0);
		}
#endif
		else if(child_pid == htxd_get_equaliser_pid() ) {
			htxd_set_equaliser_pid(0);
		}

		else {
			htxd_reset_exer_pid(child_pid, exit_reason);
		}

	/*	if(is_ipc_cleanup_required == 1) {
			htxd_ipc_cleanup_on_process_exit(child_pid);
		} */
	} 

}



void htxd_sig_usr2_handler(int sig)
{
	struct sigaction sigvector;

	
	htxd_set_value_FD_close_on_exec_flag(1);

	sigvector.sa_handler = SIG_DFL;
	sigemptyset(&(sigvector.sa_mask));
	sigvector.sa_flags = 0;

	/* end_it() */

	htxd_set_value_FD_close_on_exec_flag(0);
	
}

#ifdef __HTXD_DR__

void pids_of_hxssup_alarm_handler()
{
	char         temp_string[500];


	sprintf(temp_string,"echo \"==========# Pids of all htx-processes at `date` #========= \n\" >> %s/htxps", global_htxd_log_dir);
	system(temp_string);

	sprintf(temp_string,"ps -flL %d >> %s/htxps &",getpid(), global_htxd_log_dir);
	system(temp_string);
}



/* daemon SIGUSR1 signal handler : DR child process send SIGUSR1 to restart required devices under DR */
void htxd_sig_usr1_handler(int sig)
{
	struct  sigaction sigvector, sigvector2;
	int     cpus_dr;
	char    cmd[500];
	char	mdt_path[200];
	char	mdt_dr[128];
	char	mdtfile[128];
	char	msg_text[512];
	char	*running_mdt_name;
	int	rc;
	char	temp_string[300];


	sprintf(msg_text, "htxd_sig_usr1_handler: entered SIGUSR1 handler");
	htxd_send_message (msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);


	if(htxd_get_running_ecg_count() != 1) {
		return;
	}

	running_mdt_name = htxd_get_running_ecg_name();


	htxd_set_dr_sem_value(1);

	sigemptyset(&(sigvector2.sa_mask));
	sigvector2.sa_flags = 0;
	sigvector2.sa_handler = (void (*)(int))pids_of_hxssup_alarm_handler;
	sigaction(SIGALRM, &sigvector2, (struct sigaction *) NULL);

	cpus_dr = _system_configuration.ncpus;
	
	/* Create a new MDT having only processor stanzas */
	sprintf(cmd,"cat /dev/null | htxconf.awk > %s/mdt/mdt.%d",global_htx_home_dir, getpid());
	system(cmd);

	rc = update_syscfg();
	if(rc != 0) {
		sprintf(msg_text,"Error: delete_syscfg return with error code <%d>\n", rc);
		htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}

	/* This code is added for defect(775699) to handle 'smtctl' scaleup problem in AIX */
	sprintf(cmd, "smtctl > %s/smtctl", global_htx_log_dir);
	system(cmd);

	/* Changes for the feature(664034) hxssup dr enhancement .The intention of this code is to restart
	   only those exers which is present in the initial mdt that is selected after doing 'htx'. */
	sprintf(msg_text, "mdt file selected is: %s", running_mdt_name);
	htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);

	sprintf(cmd,"cat %s | awk '/:/ {sub(/[0-9]*:/, \"\", $0);print }' | sort | uniq | awk '{printf(\"%%s \",$0);}END {printf(\"\\n\")}' > %s/devices_dr ",running_mdt_name, global_htxd_log_dir);
	system(cmd);

	sprintf(mdtfile,"mdt.%d",getpid());

	sprintf(cmd,"cat %s/mdt/%s | create_mdt_with_devices.awk `cat %s/devices_dr | awk '{ print $0}'` >%s/mdt/%s",global_htx_home_dir, mdtfile, global_htxd_log_dir, global_htx_home_dir,DR_MDT_NAME);
	system(cmd);

	sprintf(mdt_dr, "%s/mdt/%s", global_htx_home_dir,DR_MDT_NAME);

	if(strncmp( htxd_get_dr_restart_flag(), "yes", 3) == 0) {
		alarm(0);  /* Disable previous alarm */
		/* reconfig_restart(mdt_dr); */
		htxd_set_shm_with_exercisers_values_for_dr_restart(mdt_dr); 
		alarm(SIGALRM_EXP_TIME);
		/* DR_done = 1; */
	} else {
		printf("DEBUG: htxd_sig_usr1_handler() : dr retstart flag is NO\n"); fflush(stdout); 
	}
	
	htxd_set_dr_sem_value(0);

}



/* (not being used)daemon DR signal hamdler */
void htxd_sig_dr_handler(int sig)
{

	int return_code = 0; 
	dr_info_t DRinfo;
	int dr_sem_value;
	int n_cpus_dr;
	char msg_text[512];

	printf("DEBUG: htxd - htxd_sig_dr_handler is called\n"); fflush(stdout);


	do {
		return_code = dr_reconfig (DR_QUERY, &DRinfo);
	} while( (return_code < 0) && (errno == 4) );

	if(return_code < 0) {
		sprintf (msg_text, "dr_reconfig system call failed..errno = %d", errno);
		if(errno == 6) {
			htxd_send_message (msg_text, errno, HTX_SYS_INFO, HTX_SYS_MSG);	
		} else {
			htxd_send_message (msg_text, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
		}	
		return;
	}

	sprintf (msg_text, "dr_reconfig output:pre:%d, check:%d, doit:%d, post:%d, posterror:%d,add: %d, rem:%d, cpu:%d, mem:%d, lcpu:%d, bcpu:%d\n", DRinfo.pre, DRinfo.check, DRinfo.doit, DRinfo.post, DRinfo.posterror, DRinfo.add, DRinfo.rem, DRinfo.cpu, DRinfo.mem, DRinfo.lcpu, DRinfo.bcpu);
	htxd_send_message (msg_text, errno, HTX_SYS_INFO, HTX_SYS_MSG);

#ifdef __ENT_CAP__
	if (DRinfo.cpu != 1 && DRinfo.ent_cap != 1) {
#else
	if (DRinfo.cpu != 1) {
#endif
		if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)){
		}
		return;
	}

#ifdef __ENT_CAP__
	if (DRinfo.ent_cap == 1) {
	}
#endif

	if (DRinfo.check == 1) {
	}

	if (DRinfo.pre == 1) {
		n_cpus_dr = _system_configuration.ncpus;
		dr_sem_value = htxd_get_dr_sem_value();
		if(n_cpus_dr == 1) {
		}
	}

	if ((DRinfo.post == 1) || (DRinfo.posterror == 1)) {
		sleep (1);
		n_cpus_dr = _system_configuration.ncpus;
		
	}

	if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)){
	}

	return;

}
#endif

void register_signal_handlers(void)
{
	int singal_number;
	//struct sigaction signal_action;
	struct sigaction sigvector;
	struct sigaction old_SIGHUP_vector;

	for(singal_number = 1; singal_number <= SIGMAX; singal_number++)
	{
	sigemptyset (&(sigvector.sa_mask));       /* do not block signals       */
	sigvector.sa_flags = 0;   /* do not restart system calls on
				 * sigs */
	switch (singal_number) {
	    case SIGHUP:             /* hangup              */
		(void) sigaction (singal_number, (struct sigaction *) NULL, &old_SIGHUP_vector);
	 /*if (old_SIGHUP_vector.sa_handler == SIG_IGN) */  /* nohup in effect? */
		sigvector.sa_handler = (void (*)(int)) SIG_IGN;
	 /*else
	    sigvector.sa_handler = (void (*)(int)) sig_end;*/
		break;
	    case SIGINT:             /* interrupt (rubout)  */
		sigvector.sa_handler = (void (*)(int)) sig_end;
		break; 
	    case SIGQUIT:            /* quit (ASCII FS)     */
		sigvector.sa_handler = SIG_IGN;
		break;
	 /*   case SIGILL:             / * illegal instruction * /
		sigvector.sa_handler = (void (*)(int)) sig_end;
		break; */
	    case SIGTRAP:            /* trace trap          */
		sigvector.sa_handler = SIG_DFL;
		break;
	    case SIGABRT:            /* abort process       */
		sigvector.sa_handler = SIG_DFL;
		break;
#if !defined(__HTX_LINUX__) && !defined(__OS400__)	/* 400 */
	    case SIGEMT:             /* EMT instruction     */
		sigvector.sa_handler = SIG_DFL;
		break;
#endif
	 /*   case SIGFPE:             / * floating point exception    * /
		sigvector.sa_handler = (void (*)(int)) sig_end;
		break; */
	    case SIGKILL:            /* kill (cannot be caught)     */
		sigvector.sa_handler = SIG_DFL;
		break;
	    case SIGBUS:             /* bus error                   */
		sigvector.sa_handler = SIG_DFL;
		break;
		//case SIGSEGV:         /* segmentation violation      */
		    //  sigvector.sa_handler = (void (*)(int)) sig_end;
		    //  break;
		case SIGSYS:             /* bad argument to system call */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGPIPE:            /* write on a pipe with no one t */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGALRM:            /* alarm clock                 */
		    sigvector.sa_handler = (void (*)(int)) alarm_signal;
		    break;
		case SIGTERM:            /* software termination signal */
		    sigvector.sa_handler = (void (*)(int)) sig_end;
		    break;
		case SIGURG:             /* urgent condition on I/O channel */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGSTOP:            /* stop (cannot be caught or ignored) */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGTSTP:            /* interactive stop */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGCONT:            /* continue (cannot be caught or ignored) */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGCHLD:            /* sent to parent on child stop or exit */
		    sigvector.sa_handler = (void (*)(int)) child_death;
			sigvector.sa_flags = SA_RESTART;
#ifndef __OS400__                           /* 400 */
		    sigvector.sa_flags = SA_RESTART;
#endif
		    break;
		case SIGTTIN:            /* background read attempted from control
				 * terminal */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGTTOU:            /* background write attempted to control
				 * terminal */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGIO:              /* I/O possible or completed */
		    sigvector.sa_handler = SIG_IGN;
		    break;
		case SIGXCPU:            /* cpu time limit exceeded (see setrlimit())  */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGXFSZ:            /* file size limit exceeded (see setrlimit()) */
		    sigvector.sa_handler = SIG_DFL;
		    break;
#if !defined(__HTX_LINUX__) && !defined(__OS400__)	/* 400 */
		case SIGMSG:             /* input data is in the hft ring buffer */
		    sigvector.sa_handler = SIG_IGN;
		    break;
#endif
		case SIGWINCH:           /* window size change */
		    sigvector.sa_handler = SIG_IGN;
		    break;
#ifndef __OS400__               /* 400 */
		case SIGPWR:             /* power-fail restart          */
		    sigvector.sa_handler = SIG_DFL;
		    break;
#endif
#ifdef __HTXD_DR__
		case SIGUSR1:            /* user defined signal 1       */
		    sigvector.sa_handler = (void (*)(int)) htxd_sig_usr1_handler;
		    break;
#else
		case SIGUSR1:            /* user defined signal 1       */
		    sigvector.sa_handler = SIG_DFL;
		    break;
#endif
		case SIGUSR2:            /* user defined signal 2       */
		    sigvector.sa_handler = (void (*)(int)) htxd_sig_usr2_handler;
		    break;

		case SIGPROF:            /* profiling time alarm (see setitimer) */
		    sigvector.sa_handler = SIG_DFL;
		    break;
#if !defined(__HTX_LINUX__) && !defined(__OS400__)	/* 400 */
		case SIGDANGER:          /* system crash imminent (maybe) */
		    sigvector.sa_handler = SIG_DFL;
		    break;
#endif
		case SIGVTALRM:          /* virtual time alarm (see setitimer) */
		    sigvector.sa_handler = SIG_DFL;
		    break;
#if !defined(__HTX_LINUX__) && !defined(__OS400__)	/* 400 */
		case SIGMIGRATE:         /* migrate process (see TCF) */
		    sigvector.sa_handler = SIG_DFL;
		    break;
		case SIGGRANT:           /* monitor mode granted        */
		    sigvector.sa_handler = SIG_IGN;
		    break;
		case SIGRETRACT:         /* monitor mode retracted      */
		    sigvector.sa_handler = SIG_IGN;
		    break;
		case SIGSOUND:           /* sound ack                   */
		    sigvector.sa_handler = SIG_IGN;
		    break;
		case SIGSAK:             /* secure attention key */
		    sigvector.sa_handler = SIG_IGN;
		    break;
#endif
		default:
		    sigvector.sa_handler = SIG_DFL;
		    ;
	}                         /* endswitch */
	(void) sigaction (singal_number, &sigvector, (struct sigaction *) NULL);
    }
}


int htxd_send_SIGTERM(pid_t target_pid)
{
	int return_code = 0;

	return_code = kill(target_pid, SIGTERM);
	
	return return_code;
}



int htxd_send_SIGKILL(pid_t target_pid)
{
	int return_code = 0;

	return_code = kill(target_pid, SIGKILL);
	
	return return_code;
}


int htxd_send_SIGUSR1(pid_t target_pid)
{
	int return_code = 0;

	return_code = kill(target_pid, SIGUSR1);
	
	return return_code;
}

#ifdef __HTXD_DR__



/* DR child process DR signal handler */
void htxd_dr_child_sig_dr_handler(int sig)
{
	int return_code = 0;
	dr_info_t         DRinfo;
	static  int     n_cpus_dr = 0;  /* number of cpus before DR */
	int dr_sem_value;
	int dr_alarm_value;
	char msg_text[512];
	char temp_msg[128];

	printf("DEBUG: DR child - htxd_dr_child_sig_dr_handler is called\n");


	strcpy(msg_text,"DR_child_handler: entered SIGRECONFIG handler");
	htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);

	dr_alarm_value = htxd_get_dr_alarm_value();

	do {
		return_code = dr_reconfig(DR_QUERY, &DRinfo);
	} while( (return_code < 0) && (errno == 4));

	if (return_code < 0) {
		sprintf(msg_text,"DR_child_handler: dr_reconfig call failed..errno(%d) (%s)", errno,strerror(errno));
		if (errno == 6) {
			htxd_send_message(msg_text, errno, HTX_SYS_INFO, HTX_SYS_MSG);
		} else {
			htxd_send_message(msg_text, errno, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
			return;
		}
	}

    sprintf(msg_text,"DR: reconfig output from DR_child_handler:");
    if ( DRinfo.add ) {
            sprintf(temp_msg,"add=%d,",DRinfo.add);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.rem ) {
            sprintf(temp_msg,"rem=%d,",DRinfo.rem);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.cpu ) {
            sprintf(temp_msg,"cpu=%d,",DRinfo.cpu);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.mem ) {
            sprintf(temp_msg,"mem=%d,",DRinfo.mem);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.check ) {
            sprintf(temp_msg,"check=%d,",DRinfo.check);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.pre ) {
            sprintf(temp_msg,"pre=%d,",DRinfo.pre);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.doit ) {
            sprintf(temp_msg,"doit=%d,",DRinfo.doit);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.post ) {
            sprintf(temp_msg,"post=%d,",DRinfo.post);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.posterror ) {
            sprintf(temp_msg,"posterror=%d,",DRinfo.posterror);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.force ) {
            sprintf(temp_msg,"force=%d,",DRinfo.force);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.bindproc ) {
            sprintf(temp_msg,"bindproc=%d,",DRinfo.bindproc);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.softpset ) {
            sprintf(temp_msg,"softpset=%d,",DRinfo.softpset);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.hardpset ) {
            sprintf(temp_msg,"hardpset=%d,",DRinfo.hardpset);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.plock ) {
            sprintf(temp_msg,"plock=%d,",DRinfo.plock);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.pshm ) {
            sprintf(temp_msg,"pshm=%d,",DRinfo.pshm);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.ent_cap ) {
            sprintf(temp_msg,"ent_cap=%d,",DRinfo.ent_cap);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.var_wgt ) {
            sprintf(temp_msg,"var_wgt=%d,",DRinfo.var_wgt);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.splpar_capable ) {
            sprintf(temp_msg,"splpar_capable=%d,",DRinfo.splpar_capable);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.splpar_shared ) {
            sprintf(temp_msg,"splpar_shared=%d,",DRinfo.splpar_shared);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.splpar_capped ) {
            sprintf(temp_msg,"splpar_capped=%d,",DRinfo.splpar_capped);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.cap_constrained ) {
            sprintf(temp_msg,"cap_constrained=%d,",DRinfo.cap_constrained);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.migrate ) {
            sprintf(temp_msg,"migrate=%d,",DRinfo.migrate);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.hibernate ) {
            sprintf(temp_msg,"hibernate=%d,",DRinfo.hibernate);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.partition ) {
            sprintf(temp_msg,"partition=%d,",DRinfo.partition);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.workload_partition ) {
            sprintf(temp_msg,"workload_partition=%d,",DRinfo.workload_partition);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.checkpoint ) {
            sprintf(temp_msg,"checkpoint=%d,",DRinfo.checkpoint);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.restart ) {
            sprintf(temp_msg,"restart=%d,",DRinfo.restart);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.topology_update ) {
            sprintf(temp_msg,"topology_update=%d,",DRinfo.topology_update);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.acc_update ) {
            sprintf(temp_msg,"acc_update=%d,",DRinfo.acc_update);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.chlmb ) {
            sprintf(temp_msg,"chlmb=%d,",DRinfo.chlmb);
            strcat(msg_text, temp_msg);
    }
    if ( DRinfo.lcpu ) {
        sprintf(temp_msg,"lcpu=%d,",DRinfo.lcpu);
        strcat(msg_text, temp_msg);
    }
    if ( DRinfo.bcpu ) {
        sprintf(temp_msg,"bcpu=%d,",DRinfo.bcpu);
        strcat(msg_text, temp_msg);
    }
    if( DRinfo.req_memsz_change ) {
            sprintf(temp_msg,"req_memsz_change=%ll,",DRinfo.req_memsz_change);
            strcat(msg_text, temp_msg);
    }

	htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);

#ifdef __ENT_CAP__
	if (DRinfo.cpu != 1 && DRinfo.ent_cap != 1) {
#else
	if (DRinfo.cpu != 1) {
#endif
		if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)){
			sprintf(msg_text,"dr_reconfig(DR_RECONFIG_DONE) failed. error no %d \n", errno);
			htxd_send_message(msg_text, errno, HTX_SYS_INFO, HTX_SYS_MSG);
		}
		return;
	}
	
	if (DRinfo.hibernate) {
		sprintf(msg_text," eservd, DR hibernation operation\n");
		htxd_send_message(msg_text, errno, HTX_SYS_INFO, HTX_SYS_MSG);
		if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)){
			sprintf(msg_text,"dr_reconfig(DR_RECONFIG_DONE) failed. error no %d \n", errno);
			htxd_send_message(msg_text, errno, HTX_SYS_INFO, HTX_SYS_MSG);
		}
		return;
	}

	if (DRinfo.check == 1) {
		return_code = alarm(0);  /* Clearing the alarm if exists , as fresh DR signal is received */
		if(return_code != 0) {
			sprintf(msg_text,"DR_child_handler: Another SIGRECONFIG received [ Check phase ] within %d seconds Clearing the alarm \n",dr_alarm_value);
			htxd_send_message(msg_text, 0 , HTX_SYS_INFO, HTX_SYS_MSG);
		}
		sprintf(msg_text,"DR_child_handler: DR operation in check phase\n");
		htxd_send_message(msg_text, 0 , HTX_SYS_INFO, HTX_SYS_MSG);
	}

	if (DRinfo.pre == 1) {
		n_cpus_dr = _system_configuration.ncpus;
		return_code = alarm(0);  /* Clearing the alarm if exists , as fresh DR signal is received */
		if(return_code != 0) {
			sprintf(msg_text,"DR_child_handler: Another SIGRECONFIG received [ Pre phase ] within %d seconds Clearing the alarm \n",dr_alarm_value);
			htxd_send_message(msg_text, 0 , HTX_SYS_INFO, HTX_SYS_MSG);
		}
		sprintf(msg_text,"DR operation in PRE phase..No. of CPUs = %d", n_cpus_dr);
		htxd_send_message(msg_text, 0 , HTX_SYS_INFO, HTX_SYS_MSG);
	}

	if ((DRinfo.post == 1) || (DRinfo.posterror == 1)) {
		sprintf(msg_text,"DR_child_handler: In the post / post-error phase\n");
		htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);

		n_cpus_dr = _system_configuration.ncpus;
		
		sprintf(msg_text,"DR operation in POST / POST-ERROR phase..No. of CPUs = %d", n_cpus_dr);
		htxd_send_message(msg_text, errno, HTX_SYS_INFO, HTX_SYS_MSG);

#ifdef __ENT_CAP__
		if((!DRinfo.ent_cap) && (!DRinfo.hibernate)) {
#else
		if (!DRinfo.hibernate) {
#endif
			sprintf(msg_text,"DR_child_handler: Setting do_dr_reconfig_restart to 1\n");
			htxd_send_message(msg_text, errno, HTX_SYS_INFO, HTX_SYS_MSG);
			htxd_set_dr_reconfig_restart(1);
		}

		dr_sem_value = htxd_get_dr_sem_value();
		if(dr_sem_value == 1) {
			sprintf(msg_text,"DR_child_handler: another DR operation before exer restart\n");
			strcat(msg_text,"DR operation may fail\n");
			htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);	
			dr_reconfig(DR_EVENT_FAIL, &DRinfo);
		}

		return_code = alarm(0);	
		if(return_code == 0) {
			sprintf(msg_text,"DR_child_handler: setting alarm for the first time to %d sec\n",dr_alarm_value);
			htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			alarm(dr_alarm_value);
		} else {
			sprintf(msg_text,"DR_child_handler: Another SIGRECONFIG received within %d seconds. Setting the alarm again to %d minutes\n",dr_alarm_value, dr_alarm_value);
			htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			alarm(dr_alarm_value);
		}
	}
	
	if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)){
		sprintf(msg_text,"dr_reconfig(DR_RECONFIG_DONE) failed. error no %d \n", errno);
		htxd_send_message(msg_text, errno, HTX_SYS_INFO, HTX_SYS_MSG);
		return;
	}
	
	return;
}



/* DR child process signal SIGALRM handler */
void htxd_dr_child_sig_alarm_handler(int sig)
{
	char	msg_text[256];


	sprintf(msg_text, "htxd_dr_child_sig_alarm_handler: sending SIGUSR1 to supervisor");
	htxd_send_message (msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	kill(getppid(),SIGUSR1);	/* sending SIGUSR1 to supervisor */
}



/* register DR child process signal hamdlers */
int htxd_init_dr_child_signal_handler(void)
{
	struct  sigaction sigvector, sigvector2;
	int i;

	sigemptyset(&(sigvector2.sa_mask));
	sigvector2.sa_flags = 0;
	sigvector.sa_handler = SIG_DFL;

	sigemptyset(&(sigvector2.sa_mask));
	sigvector2.sa_flags = 0;
	sigvector2.sa_handler = SIG_DFL;

	/* initialize all signal handlers */
	for (i = 1; i <= SIGMAX; i++) {
		sigaction(i, &sigvector, (struct sigaction *) NULL);
	}	

	/* register DR signal */
	sigemptyset(&(sigvector.sa_mask));
	sigvector.sa_flags = 0;
	sigvector.sa_handler = (void (*)(int))htxd_dr_child_sig_dr_handler;
	sigaction(SIGRECONFIG, &sigvector, (struct sigaction *) NULL);

	/* register alarm signal */
	sigemptyset(&(sigvector2.sa_mask));
	sigvector2.sa_flags = 0;
	sigvector2.sa_handler = (void (*)(int))htxd_dr_child_sig_alarm_handler;
	sigaction(SIGALRM, &sigvector2, (struct sigaction *) NULL);

	return 0;
}



#endif
