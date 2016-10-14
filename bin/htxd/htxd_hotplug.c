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
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <sched.h>
#include <errno.h>

#include "htxd_define.h"
#include "htxd_util.h"
#include "htxd_profile.h"
#include "htxd_instance.h"
#define PARMS(x) x
#include "htxsyscfg64.h"
#include "cfgclibdef.h"
#include "scr_info.h"

#ifdef __HTX_LINUX__

typedef struct
{
	int cpu;
	int mem;
	int io;
} hotplug_mode;

union shm_pointers	hotplug_global_shm_ptr;
int			hotplug_run = 0;
struct sigaction	sigvector_hotplug;

extern int	load_exerciser(struct htxshm_HE *, struct sigaction *);
extern int	process_mdt_to_shm(char *, int, union shm_pointers, char **, CFG__SFT *);
int		htxd_set_shm_with_exercisers_values_for_dr_restart(char *);
int   semhe_id;



void hotplug_debug_log(char *debug_text)
{
	char msg_str[1024];
	sprintf(msg_str, "hotplug : %s", debug_text);
	htxd_send_message(msg_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
}



void hotplug_log_cpu_mask(cpu_set_t *previous_mask, cpu_set_t *current_mask, int mask_size)
{
	char mask_log_str[2048];
	char mask_str[128];
	unsigned long long * ptr_pre;
	unsigned long long * prt_cur;
	int no_mask_field;
	int mask_field_count;


	sprintf(mask_log_str, "start dumping cpu mask: cpu mask size = <%d> ############################", mask_size);
	htxd_send_message(mask_log_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);

	ptr_pre = (unsigned long long*) previous_mask;
	prt_cur = (unsigned long long*) current_mask;

	no_mask_field = mask_size / sizeof(unsigned long long);

	sprintf(mask_str, "\nprev mask :");
	strcpy(mask_log_str,mask_str);
	for (mask_field_count = 0; mask_field_count < no_mask_field; mask_field_count++) {
		if(*ptr_pre == 0) {
			strcat(mask_log_str, "<>");
		} else {
			sprintf(mask_str, "<%llX>", *ptr_pre);
			strcat(mask_log_str, mask_str);
		}
		ptr_pre++;
	}
	htxd_send_message(mask_log_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);

	sprintf(mask_str, "\ncurr mask :" );
	strcpy(mask_log_str, mask_str);
	for (mask_field_count = 0; mask_field_count < no_mask_field; mask_field_count++) {
		if(*prt_cur == 0) {
			strcat(mask_log_str, "<>");
		} else {
			sprintf(mask_str, "<%llX>", *prt_cur);
			strcat(mask_log_str, mask_str);
		}
		prt_cur++;
	}
	htxd_send_message(mask_log_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	
		
	sprintf(mask_log_str, "end dumping cpu mask: dumped mask size = <%zd>  ############################", no_mask_field * sizeof(unsigned long long));
	htxd_send_message(mask_log_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);

	if( mask_size != (no_mask_field * sizeof(unsigned long long)) ) {
		sprintf(mask_log_str, "all mask bits are not dumped, actual mask size = <%d>, dumped mask size = <%zd>", mask_size, (no_mask_field * sizeof(unsigned long long) ) );
		htxd_send_message(mask_log_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}
}


/* re-create MDTs and restart the new MDT */
void hotplug_reconfig_restart(void)
{
	char			msg_text[MSG_TEXT_SIZE];
	union shm_pointers	shm_pointers_itr;
	char			*running_ecg_name;


	shm_pointers_itr.hdr_addr = hotplug_global_shm_ptr.hdr_addr;

	htxd_send_message("repopulate mdts", 0, HTX_SYS_INFO, HTX_SYS_MSG);

	/* set default file descriptors close on exec */
	htxd_set_value_FD_close_on_exec_flag(1);

	sigfillset(&(sigvector_hotplug.sa_mask));
	sigprocmask(SIG_UNBLOCK, &(sigvector_hotplug.sa_mask), NULL);
	sigvector_hotplug.sa_flags = 0;
	sigvector_hotplug.sa_handler = SIG_DFL;

	(shm_pointers_itr.hdr_addr)++;  /* skipping shm header */

	running_ecg_name = htxd_get_running_ecg_name();

	sprintf(msg_text, "cp %s %s/mdt/mdt", running_ecg_name, global_htx_home_dir);
	system(msg_text);

	sprintf(msg_text, "%s/mdt/mdt", global_htx_home_dir);
	htxd_set_shm_with_exercisers_values_for_dr_restart(msg_text);

	/* set default file descriptors reopen on exec */
	htxd_set_value_FD_close_on_exec_flag(0);

	htxd_send_message("hotplug operation processing completed", 0, HTX_SYS_INFO, HTX_SYS_MSG);
}


/* send sigal to all registered process */
int hotplug_monitor_send_signal(hotplug_mode* p_mode_state)
{
	union shm_pointers shm_pointers_itr;
	int exer_position;
	int exer_max_count;
	char msg_text[512];

	
	shm_pointers_itr.hdr_addr = hotplug_global_shm_ptr.hdr_addr;	
	exer_max_count = shm_pointers_itr.hdr_addr->max_entries;

	(shm_pointers_itr.hdr_addr)++;  /* skipping shm header */

	for(exer_position = 0; exer_position < exer_max_count; exer_position++) {
		if(shm_pointers_itr.HE_addr->PID != 0) {
			if( (shm_pointers_itr.HE_addr->hotplug_cpu == 1) && (p_mode_state->cpu == 1) ){
				kill(shm_pointers_itr.HE_addr->PID, SIGUSR2);
				sprintf(msg_text, "hotplug: SIGUSR2 signal has been sent to device <%s> with process ID [%d]", shm_pointers_itr.HE_addr->sdev_id, shm_pointers_itr.HE_addr->PID);
				htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			}

			if( (shm_pointers_itr.HE_addr->hotplug_mem == 1) && (p_mode_state->mem == 1) && (p_mode_state->cpu != 1) ){
				kill(shm_pointers_itr.HE_addr->PID, SIGUSR2);
				sprintf(msg_text, "hotplug: SIGUSR2 signal has been sent to device <%s> with process ID [%d]", shm_pointers_itr.HE_addr->sdev_id, shm_pointers_itr.HE_addr->PID);
				htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			}
	
			if( (shm_pointers_itr.HE_addr->hotplug_io == 1) && (p_mode_state->io == 1) && (p_mode_state->cpu != 1) && (p_mode_state->mem != 1)){
				kill(shm_pointers_itr.HE_addr->PID, SIGUSR2);
				sprintf(msg_text, "hotplug: SIGUSR2 signal has been sent to device <%s> with process ID [%d]", shm_pointers_itr.HE_addr->sdev_id, shm_pointers_itr.HE_addr->PID);
				htxd_send_message(msg_text, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			}
		}

		(shm_pointers_itr.HE_addr)++;
	}
	
	return 0;
}

/* populates mdt.hotplug */
int hotplug_monitor_create_mdt(void)
{
	char mdt_path[250];
	char hotplug_str[1024];


	sprintf(hotplug_str, "start recreating mdt files");
	hotplug_debug_log(hotplug_str);

	sprintf(hotplug_str, "cd %s/etc/scripts ; %s/bin/show_syscfg > %s/htx_syscfg; devconfig > %s/hotplug_devconfig_out 2>&1", global_htx_home_dir, global_htx_home_dir, global_htx_log_dir, global_htxd_log_dir);
	system(hotplug_str);

	return 0;	
}


int hotplug_monitor_process(hotplug_mode* p_mode_state)
{
	char hotplug_msg[200];
	int hotplug_restart_delay;

	hotplug_restart_delay = htxd_get_hotplug_signal_delay();


	hotplug_monitor_send_signal(p_mode_state); 

	sleep(hotplug_restart_delay);

	/* need a restart operation */	
	hotplug_monitor_create_mdt();

	sprintf(hotplug_msg, "ready to restart MDT, sending SIGUSR2 to supervisor pid <%d>", getppid());
	htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	hotplug_reconfig_restart();
	
	return 0;
}



/* hotplug monitor functionality */
void* htxd_hotplug_monitor(void* vptr)
{
	hotplug_mode mode_state;
	cpu_set_t *previous_online_cpus_mask	= NULL;	
	cpu_set_t *current_online_cpus_mask	= NULL;	
	cpu_set_t *original_online_cpus_mask    = NULL;
	size_t cpu_mask_size;
	int number_config_processor;
	int previous_cpu_count;
	int current_cpu_count;
	char hotplug_msg[1024];
	int hotplug_wait_counter = 0;
	union shm_pointers shm_pointers_itr;
	int return_code;
	int hotplug_signal_delay;
	htxd_ecg_manager *ecg_manager;
	int rc =0;
	union semun hotplug_semctl_arg;

	

	ecg_manager = htxd_get_ecg_manager();
	hotplug_run = 1;
	hotplug_global_shm_ptr.hdr_addr = shm_pointers_itr.hdr_addr = (htxd_get_ecg_info_list(ecg_manager))->ecg_shm_addr.hdr_addr;

	previous_cpu_count = get_nprocs();

	number_config_processor = sysconf(_SC_NPROCESSORS_CONF);
	if(number_config_processor == -1) {
		sprintf(hotplug_msg, "ERROR: sysconf() returns <%d>", number_config_processor);
		htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}

	cpu_mask_size = CPU_ALLOC_SIZE(number_config_processor);

	previous_online_cpus_mask = CPU_ALLOC(number_config_processor);
	if(previous_online_cpus_mask == NULL) {
		sprintf(hotplug_msg, "ERROR: CPU_ALLOC for number_config_processor <%d>", number_config_processor);	
		htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}

	CPU_ZERO_S(cpu_mask_size, previous_online_cpus_mask);
	return_code = sched_getaffinity(getpid(), cpu_mask_size, previous_online_cpus_mask);
	if(return_code != 0) {
		sprintf(hotplug_msg, "ERROR: sched_getaffinity() returned with return code <%d>, and errno as <%d>", return_code, errno);
		htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}

    original_online_cpus_mask = CPU_ALLOC(number_config_processor);
    if(original_online_cpus_mask == NULL) {
        sprintf(hotplug_msg, "ERROR: CPU_ALLOC for number_config_processor <%d>", number_config_processor);
        htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
    }

    CPU_ZERO_S(cpu_mask_size, original_online_cpus_mask);
    return_code = sched_getaffinity(getpid(), cpu_mask_size, original_online_cpus_mask);
    if(return_code != 0) {
        sprintf(hotplug_msg, "ERROR: sched_getaffinity() returned with return code <%d>, and errno as <%d>", return_code, errno);
        htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
    }

	current_online_cpus_mask = CPU_ALLOC(number_config_processor);
	if(current_online_cpus_mask == NULL) {
		sprintf(hotplug_msg, "ERROR: CPU_ALLOC for number_config_processor <%d>, current_online_cpus_mask is NULL", number_config_processor);	
		htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}

	CPU_ZERO_S(cpu_mask_size, current_online_cpus_mask);
	return_code = sched_getaffinity(getpid(), cpu_mask_size, current_online_cpus_mask);
	if(return_code != 0) {
		sprintf(hotplug_msg, "ERROR: for current_online_cpus_mask sched_getaffinity() returned with return code <%d>, and errno as <%d>", return_code, errno);
		htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}

	hotplug_signal_delay = htxd_get_hotplug_signal_delay();
	mode_state.cpu = mode_state.mem = mode_state.io = 0;
	while(hotplug_run == 1) {  /* main loop */
		if( CPU_EQUAL_S( cpu_mask_size, previous_online_cpus_mask, current_online_cpus_mask ) == 0) {
			current_cpu_count = get_nprocs();
			sprintf(hotplug_msg, "found change in online cpu mask: previous cpu count is <%d>, current cpu count is <%d>", previous_cpu_count, current_cpu_count);
			htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			hotplug_log_cpu_mask(previous_online_cpus_mask, current_online_cpus_mask, cpu_mask_size);

			if(hotplug_wait_counter == 0) {
				sprintf(hotplug_msg, "hotplug operation detected and start processing");
				htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			} else {
				sprintf(hotplug_msg, "found another change in online cpu mask, resetting the hotplug wait counter");
				htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
			}   
			hotplug_wait_counter = 0;

			mode_state.cpu = 1;
			memcpy( previous_online_cpus_mask, current_online_cpus_mask, cpu_mask_size );
			previous_cpu_count = current_cpu_count;
			rc =0;
	        hotplug_semctl_arg.val = 1;
	        rc = semctl(semhe_id, SEM_POSITION_SYSCFG, SETVAL, hotplug_semctl_arg);
	        if(rc == -1) {
	            sprintf(hotplug_msg, "semctl is failed with return code <%d>, errno = <%d>", rc, errno);
	            htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	        }

            rc = update_syscfg();
            if(rc != 0) {
                sprintf(hotplug_msg, "ERROR :update_syscfg unsuccessful inside hotplug with %d \n",rc);
                htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
            }
            else{
                sprintf(hotplug_msg, "update_syscfg successful inside hotplug with %d \n",rc);
                htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
            }
            hotplug_semctl_arg.val = 0;
            rc = semctl(semhe_id, SEM_POSITION_SYSCFG, SETVAL, hotplug_semctl_arg);


		} 

		if (mode_state.cpu == 1 ||  mode_state.mem == 1 || mode_state.io == 1) {
			if(hotplug_wait_counter < hotplug_signal_delay) {
				hotplug_wait_counter++;
			} else {
				sprintf(hotplug_msg, "hotplug: start processing hotplug");
				hotplug_debug_log(hotplug_msg);
				hotplug_monitor_process(&mode_state); 
				mode_state.cpu = mode_state.mem = mode_state.io = 0;
				hotplug_wait_counter = 0;
			}
		}

		sleep(1);

		CPU_ZERO_S(cpu_mask_size, current_online_cpus_mask);
		return_code = sched_getaffinity(getppid(), cpu_mask_size, current_online_cpus_mask);
		if(return_code != 0) {
			sprintf(hotplug_msg, "ERROR: sched_getaffinity() returned with return code <%d>, and errno as <%d>", return_code, errno);
			htxd_send_message(hotplug_msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
		}
	}

	if(previous_online_cpus_mask != NULL) {
		CPU_FREE(previous_online_cpus_mask);
	}
	if(current_online_cpus_mask != NULL) {
		CPU_FREE(current_online_cpus_mask);
	}

	return NULL;
}



/* hotplug thread create */
int htxd_start_hotplug_monitor(htxd_thread **hotplug_monitor_thread)
{
	int return_code = 0;


	if(*hotplug_monitor_thread == NULL) {
		*hotplug_monitor_thread = malloc(sizeof(htxd_thread));
		if(*hotplug_monitor_thread == NULL) {
			exit(1);
		}
	}	

	memset(*hotplug_monitor_thread, 0, sizeof(htxd_thread));
	(*hotplug_monitor_thread)->thread_function = htxd_hotplug_monitor;
	(*hotplug_monitor_thread)->thread_data = NULL;
	
	return_code = htxd_thread_create(*hotplug_monitor_thread);
	
	return return_code;
}



/* hotplug thread cancel */
int htxd_stop_hotplug_monitor(htxd_thread **hotplug_monitor_thread)
{
	int return_code = -1;

	return_code = htxd_thread_cancel(*hotplug_monitor_thread);

	if(*hotplug_monitor_thread != NULL) {
		free(*hotplug_monitor_thread);
	}

	return return_code;
}


#endif

