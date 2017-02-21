/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */


#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include "hxihtx.h"
#include "htxd_ecg.h"
#include "htxd_instance.h"
#include "htxd_equaliser.h"
#include "htxd_define.h"
#include "htxd_util.h"
#include "htxsyscfg64.h"


#define MAX_LINE_SIZE 	500
#define MAX_PARAMS_IN_ROW	7
#define DELAY_TIME 30

test_config_struct test_config;
struct SYS sys_detail;

int num_tests = 0, total_tests=0;
int pattern_length = UTILIZATION_QUEUE_LENGTH;
int length_flag = 0;

void update_sys_details(void);
void update_exer_info(void);

void equaliser_sig_end(int signal_number, int code, struct sigcontext *scp);

void update_sys_details()
{
    int node_num, chip_num, core_num, cpu_num;
    int lcpu, num_of_chips = 0, num_of_cores = 0;
    char workstr[64];

    sprintf(workstr, "inside update_sys_details");
    htxd_send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
    sys_detail.num_nodes = get_num_of_nodes_in_sys();
    for (node_num = 0; node_num < sys_detail.num_nodes; node_num++) {
        sys_detail.nodes[node_num].logical_node_num = node_num;
        sys_detail.nodes[node_num].num_chips = get_num_of_chips_in_node(node_num);
        for (chip_num = 0; chip_num < sys_detail.nodes[node_num].num_chips; chip_num++, num_of_chips++) {
            sys_detail.nodes[node_num].chips[chip_num].logical_chip_num = num_of_chips;
            sys_detail.nodes[node_num].chips[chip_num].num_cores = get_num_of_cores_in_chip(node_num, chip_num);
            for (core_num = 0; core_num < sys_detail.nodes[node_num].chips[chip_num].num_cores; core_num++, num_of_cores++) {
                sys_detail.nodes[node_num].chips[chip_num].cores[core_num].logical_core_num = num_of_cores;
                sys_detail.nodes[node_num].chips[chip_num].cores[core_num].num_cpus = get_num_of_cpus_in_core(node_num, chip_num, core_num);
                for (cpu_num= 0; cpu_num < sys_detail.nodes[node_num].chips[chip_num].cores[core_num].num_cpus; cpu_num++) {
                    lcpu = get_logical_cpu_num(node_num, chip_num, core_num, cpu_num);
                    sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].lcpu = lcpu;
                #ifdef __HTX_LINUX__
                    sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].pcpu = get_logical_2_physical(lcpu);
                #endif
                }
            }
        }
    }
}

void update_config_detail(int index)
{
    union shm_pointers shm_addr;
    struct htxshm_HE *p_htxshm_HE;    /* pointer to a htxshm_HE struct     */
    union shm_pointers shm_addr_wk;     /* work ptr into shm                 */
    char *ptr, tmp_str[64], util_pattern[64];
    int sequence_length = 0;

    shm_addr = htxd_get_equaliser_shm_addr();
    shm_addr_wk.hdr_addr = shm_addr.hdr_addr;   /* copy addr to work space  */
    (shm_addr_wk.hdr_addr)++;         /* skip over header                  */
    p_htxshm_HE = shm_addr_wk.HE_addr;

    strcpy(util_pattern,((p_htxshm_HE + index)->cpu_utilization));
    if (strchr(util_pattern, '%') != NULL) {
        strcpy(tmp_str, util_pattern);
        ptr = strtok(tmp_str, "%");
        if (strcmp(ptr, "UTIL_LEFT") == 0) {
            test_config.exer_config[index].config.util_pattern = UTIL_LEFT;
        } else if (strcmp(ptr, "UTIL_RIGHT") == 0) {
            test_config.exer_config[index].config.util_pattern = UTIL_RIGHT;
        } else if (strcmp(ptr, "UTIL_RANDOM") == 0) {
            test_config.exer_config[index].config.util_pattern = UTIL_RANDOM;
        }
        while ((ptr = strtok(NULL, "_")) != NULL) {
            test_config.exer_config[index].config.utilization_sequence[sequence_length] = atoi(ptr);
            sequence_length++;
        }
        test_config.exer_config[index].config.sequence_length = sequence_length;
        test_config.exer_config[index].config.pattern_length = pattern_length;
    } else {
        test_config.exer_config[index].config.utilization_pattern = strtoul(util_pattern, NULL, 2);
        test_config.exer_config[index].config.util_pattern = UTIL_PATTERN;
        test_config.exer_config[index].config.pattern_length = strlen(util_pattern);
    }

}

void update_exer_info()
{
    union shm_pointers shm_addr;
    struct htxshm_HE *p_htxshm_HE;    /* pointer to a htxshm_HE struct     */
    union shm_pointers shm_addr_wk;     /* work ptr into shm                 */
    int node_num, chip_num, core_num, cpu_num;
    int i, k, lcpu, bind_cpu_num, num_tests_configured = 0;
    char *ptr, workstr[64];

    sprintf(workstr, "inside update_exer_info");
    htxd_send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);

    shm_addr = htxd_get_equaliser_shm_addr();
    shm_addr_wk.hdr_addr = shm_addr.hdr_addr;   /* copy addr to work space  */
    (shm_addr_wk.hdr_addr)++;         /* skip over header                  */
    p_htxshm_HE = shm_addr_wk.HE_addr;

    for (k=0; k < (shm_addr.hdr_addr)->num_entries; k++, p_htxshm_HE++) {
        /* Extract cpu number. */
        ptr = p_htxshm_HE->sdev_id;
        i = 0;
        bind_cpu_num = 0;
        while (i < strlen(p_htxshm_HE->sdev_id)) {
            if (*(ptr + i) >= '0' && *(ptr + i) <= '9') {
                bind_cpu_num = bind_cpu_num * 10 + *(ptr + i) - '0';
            }
            i++;
        }
        for (node_num = 0; node_num < sys_detail.num_nodes; node_num++) {
            for (chip_num = 0; chip_num < sys_detail.nodes[node_num].num_chips; chip_num++) {
                for (core_num = 0; core_num < sys_detail.nodes[node_num].chips[chip_num].num_cores; core_num++) {
                    for (cpu_num= 0; cpu_num < sys_detail.nodes[node_num].chips[chip_num].cores[core_num].num_cpus; cpu_num++) {
                        lcpu = get_logical_cpu_num(node_num, chip_num, core_num, cpu_num);
                        if (lcpu == bind_cpu_num) {
                            num_tests_configured = sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].num_tests_configured;
                            sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].exer_info_index[num_tests_configured] = k;
                            num_tests_configured++;
                            update_config_detail(k);
                            sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].num_tests_configured = num_tests_configured;
                        }
                    }
                }
            }
        }
    }
}

int init_random_no(void)
{
	srand((unsigned)(time(0)));
	return 0;
}

/*****************************************************************************/
/*****************  Function to shuffle utilization Pattern  *****************/
/*****************************************************************************/

void shuffle_pattern(int n, uint32 *pattern)
{
	uint16 first_pos, second_pos, first_digit, second_digit;
	uint32 digits_to_exch, digits_aft_exch;
	uint32 final_pattern;

	final_pattern = *pattern;
	for(first_pos=0; first_pos<n; first_pos++) {
		second_pos = first_pos + (rand()% (n - first_pos));
		digits_to_exch = (1 << first_pos) | (1 << second_pos);
		first_digit = ((final_pattern) >> first_pos) & 0x1;
		second_digit = ((final_pattern) >> second_pos) & 0x1;
		digits_aft_exch = ((first_digit << second_pos) | (second_digit << first_pos));
		final_pattern = ((~digits_to_exch) & final_pattern) | digits_aft_exch;
	}
	*pattern = final_pattern;
}

/*****************************************************************************/
/*****************  Function to generate utilization Pattern  ****************/
/*****************************************************************************/

void reset_utilization_queue(int index)
{
    int j;
    run_time_config *exer_conf;
    int target_active_in_decs;
    char buf[20], msg[128], workstr[512];
    union shm_pointers shm_addr_wk, shm_addr;
    struct htxshm_HE *p_htxshm_HE;

    shm_addr = htxd_get_equaliser_shm_addr();
    shm_addr_wk.hdr_addr = shm_addr.hdr_addr;  /* copy addr to work space  */
    (shm_addr_wk.hdr_addr)++;         /* skip over header                  */
    p_htxshm_HE = shm_addr_wk.HE_addr;

    exer_conf = test_config.exer_config;

    target_active_in_decs = (exer_conf[index].data.target_utilization * exer_conf[index].config.pattern_length) / 100;
    exer_conf[index].config.utilization_pattern = 0;
    switch (exer_conf[index].config.util_pattern) {
        case 0:
            for (j = 1; j <= target_active_in_decs; j++) {
                exer_conf[index].config.utilization_pattern |= ( 1 << (exer_conf[index].config.pattern_length - j));
                strcpy(buf, "UTIL_LEFT");
            }
            break;

        case 1:
            for (j = 1; j <= target_active_in_decs; j++) {
                exer_conf[index].config.utilization_pattern |= ( 1 << (target_active_in_decs - j));
                strcpy(buf, "UTIL_RIGHT");
            }
            break;

        case 2:
            for (j = 1; j <= target_active_in_decs; j++) {
                exer_conf[index].config.utilization_pattern |= ( 1 << (exer_conf[index].config.pattern_length - j));
            }
            init_random_no();
            shuffle_pattern(exer_conf[index].config.pattern_length, &(exer_conf[index].config.utilization_pattern));
            strcpy(buf, "UTIL_RANDOM");
            break;
    }
    if(htxd_get_equaliser_debug_flag() == 1) {
        sprintf(workstr, "Device Name: %s\nUtilization Sequence:", (p_htxshm_HE + index)->sdev_id);
        for (j=0; j< exer_conf[index].config.sequence_length; j++) {
            sprintf(msg,"%d ", exer_conf[index].config.utilization_sequence[j]);
            strcat(workstr, msg);
        }
        strcat(workstr, "\n");
        sprintf(msg, "Target Utilization: %d\nUtilization pattern: 0x%x(%s)", exer_conf[index].data.target_utilization, exer_conf[index].config.utilization_pattern, buf);
        strcat(workstr, msg);
        htxd_send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
    }
}

/***************************************************************************/
/**************  Function to initialize runtime data  **********************/
/***************************************************************************/

void initialize_run_time_data(void)
{
    run_time_config *exer_conf;
    int node_num, chip_num, core_num, cpu_num;
    int i, index, num_tests_configured;
    char workstr[512], msg[256];
    union shm_pointers shm_addr_wk, shm_addr;
    struct htxshm_HE *p_htxshm_HE;

    sprintf(workstr, "inside initialize_run_time_data");
    htxd_send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);

    shm_addr = htxd_get_equaliser_shm_addr();
    shm_addr_wk.hdr_addr = shm_addr.hdr_addr;  /* copy addr to work space  */
    (shm_addr_wk.hdr_addr)++;         /* skip over header                  */
    p_htxshm_HE = shm_addr_wk.HE_addr;

    exer_conf = test_config.exer_config;
    for (node_num = 0; node_num < sys_detail.num_nodes; node_num++) {
        for (chip_num = 0; chip_num < sys_detail.nodes[node_num].num_chips; chip_num++) {
            for (core_num = 0; core_num < sys_detail.nodes[node_num].chips[chip_num].num_cores; core_num++) {
                for (cpu_num= 0; cpu_num < sys_detail.nodes[node_num].chips[chip_num].cores[core_num].num_cpus; cpu_num++) {
                    num_tests_configured = sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].num_tests_configured;
                    if (num_tests_configured == 0) {
                        continue;
                    } else {
                        for (i = 0; i < num_tests_configured; i++) {
                            index = sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].exer_info_index[i];
                            exer_conf[index].data.target_utilization = exer_conf[index].config.utilization_sequence[0];
                            if (exer_conf[index].config.util_pattern != UTIL_PATTERN) {
                                reset_utilization_queue(index);
                            } else {
                                sprintf(workstr, "Device Name: %s\n", (p_htxshm_HE + index)->sdev_id);
                                sprintf(msg, "Utilization pattern: 0x%x\n", exer_conf[index].config.utilization_pattern);
                                strcat(workstr, msg);
                                htxd_send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
                                /* printf("%s", workstr);  */
                            }
                            exer_conf[index].data.current_step = 0;
                            exer_conf[index].data.current_seq_step = 0;
                        }
                    }
                }
            }
        }
    }
}

void htxd_equaliser(void)
{
  /*
   ***************************  variable definitions  *****************************
   */
    int semhe_id;        /* semaphore id                      */
    union shm_pointers shm_addr;  /* shared memory union pointers      */

    char workstr[512], str1[128], msg[256];
    FILE *log_fp;
    int i, rc, kill_return_code = 0, lcpu, pcpu; /* loop counter     */
    int node_num, chip_num, core_num, cpu_num;
    int action, num_tests_configured, index;
    useconds_t micro_seconds;
    char global_htxd_log_dir[256] = "/tmp/";
    long clock, tm_log_save;
    struct stat buf1;

    static char process_name[] = "htx_equaliser";
    struct htxshm_HE *p_htxshm_HE;  /* pointer to a htxshm_HE struct     */
    struct semid_ds sembuffer;      /* semaphore buffer                  */
    union shm_pointers shm_addr_wk; /* work ptr into shm                 */
    struct sigaction sigvector;     /* structure for signals             */
    run_time_config *exer_conf;
    char equaliser_log_file[300], equaliser_log_file_save[300];
    struct tm new_time;
    char str_time[50];

  /*
   ***********************  beginning of executable code  *******************
   */
    printf("DEBUG: equaliser process started\n");
    htxd_set_program_name(process_name);
    (void) htxd_send_message("Equaliser process started.", 0, HTX_SYS_INFO, HTX_SYS_MSG);

    shm_addr = htxd_get_equaliser_shm_addr();
    semhe_id = htxd_get_equaliser_semhe_id();

    sprintf(equaliser_log_file, "%s/%s", global_htxd_log_dir, LOGFILE);
    sprintf(equaliser_log_file_save, "%s/%s", global_htxd_log_dir, LOGFILE_SAVE);

  /*
   *************************  Set default SIGNAL handling  **********************
   */
  	sigemptyset(&(sigvector.sa_mask));  /* do not block signals         */
  	sigvector.sa_flags = 0;         /* do not restart system calls on sigs */
  	sigvector.sa_handler = SIG_DFL;

  	for (i = 1; i <= SIGMAX; i++) {
    		(void) sigaction(i, &sigvector, (struct sigaction *) NULL);
	}
  	sigaddset(&(sigvector.sa_mask), SIGINT);
  	sigaddset(&(sigvector.sa_mask), SIGQUIT);
  	sigaddset(&(sigvector.sa_mask), SIGTERM);

  	sigvector.sa_handler = (void (*)(int)) equaliser_sig_end;
  	(void) sigaction(SIGINT, &sigvector, (struct sigaction *) NULL);
  	(void) sigaction(SIGQUIT, &sigvector, (struct sigaction *) NULL);
  	(void) sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

  	rc = stat(equaliser_log_file, &buf1);
  	if ((rc == 0) || ((rc == -1 ) && (errno != ENOENT))) {
		remove(equaliser_log_file);
	}

	rc = stat(equaliser_log_file_save, &buf1);
	if ((rc == 0) || ((rc == -1 ) && (errno != ENOENT))) {
		remove(equaliser_log_file_save);
	}


  /*
   *************************  Get config data  *********************************
   */
    printf("DEBUG: point 1\n");
    test_config.exer_config = (run_time_config *) malloc(sizeof(run_time_config) * (shm_addr.hdr_addr)->max_entries);
    if( test_config.exer_config == NULL) {
        sprintf(msg, "Error (errno = %d) in allocating memory for equaliser process.\nProcess will exit now.", errno);
        htxd_send_message(msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
        exit(1);
    }

    rc = init_syscfg_with_malloc();
    if (rc) {
        exit(1);
    }

    printf("DEBUG: point 2\n");
    test_config.time_quantum = htxd_get_equaliser_time_quantum();
    test_config.startup_time_delay = htxd_get_equaliser_startup_time_delay();
    test_config.log_duration = htxd_get_equaliser_log_duration();
    test_config.offline_cpu = htxd_get_equaliser_offline_cpu_flag();
    pattern_length = htxd_get_equaliser_pattern_length();

    /* Update the structure with system config details */
    update_sys_details();
    update_exer_info();
    initialize_run_time_data();

  /*********************************************************************************/
  /***********************  set up shared memory addressing  ***********************/
  /*********************************************************************************/

	printf("DEBUG: point 3\n");
 	shm_addr_wk.hdr_addr = shm_addr.hdr_addr;  /* copy addr to work space  */
  	(shm_addr_wk.hdr_addr)++;         /* skip over header                  */
  	p_htxshm_HE = shm_addr_wk.HE_addr;

	micro_seconds = test_config.time_quantum * 1000;
	if(test_config.startup_time_delay != 0) {
		sprintf(msg, "Equaliser will take %d sec(startup_time_delay) to be effective.", test_config.startup_time_delay);
		htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
		sleep(test_config.startup_time_delay);
	}
	log_fp = fopen(equaliser_log_file, "w");
	tm_log_save = time((long *) 0);
	printf("DEBUG: point 4\n");
	while(1) {

		clock = time((long *) 0);
		if((clock - tm_log_save) >= test_config.log_duration ) {
			sprintf(workstr, "cp %s %s", equaliser_log_file, equaliser_log_file_save);
			system(workstr);
			fclose(log_fp);
			tm_log_save = clock;
			log_fp = fopen(equaliser_log_file, "w");
		}
		exer_conf = test_config.exer_config;
        for (node_num = 0; node_num < sys_detail.num_nodes; node_num++) {
            for (chip_num = 0; chip_num < sys_detail.nodes[node_num].num_chips; chip_num++) {
                for (core_num = 0; core_num < sys_detail.nodes[node_num].chips[chip_num].num_cores; core_num++) {
                    for (cpu_num= 0; cpu_num < sys_detail.nodes[node_num].chips[chip_num].cores[core_num].num_cpus; cpu_num++) {
                        num_tests_configured = sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].num_tests_configured;
                        if (num_tests_configured == 0) {
                            continue;
                        } else {
                            for (i=0; i < num_tests_configured; i++) {
			                    index = sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].exer_info_index[i];
			                    if (((p_htxshm_HE + index)->PID != 0 ) &&     /* Not Dead?  */
                                   ((p_htxshm_HE + index)->tm_last_upd != 0) &&     /* HE started?  */
                                   ((p_htxshm_HE + index)->started_delay_flag == 0) &&     /* HE started?  */
                                   (((p_htxshm_HE + index)->max_cycles == 0) || (((p_htxshm_HE + index)->max_cycles != 0) && ((p_htxshm_HE + index)->cycles < (p_htxshm_HE + index)->max_cycles))) &&  /* not completed  */
                                   (semctl(semhe_id, 0, GETVAL, &sembuffer) == 0) &&   /* system active?   */
                                   (semctl(semhe_id, ((index * SEM_PER_EXER) + SEM_GLOBAL + 1), GETVAL, &sembuffer) == 0) && /* Not errored out  */
                                   (semctl(semhe_id, ((index * SEM_PER_EXER) + SEM_GLOBAL), GETVAL, &sembuffer) == 0)) {  /* Not stopped from option 2 */
                                    action = ((exer_conf[index].config.utilization_pattern) >> (exer_conf[index].config.pattern_length - (exer_conf[index].data.current_step + 1))) & 0x1;
                                    /* sprintf(msg, "Action is:%d", Action);
                                    htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);  */
						            if((action == 0) && ((p_htxshm_HE + index)->equaliser_halt == 0)) {
								    clock = time((long *) 0);
								    localtime_r(&clock, &new_time);
								    asctime_r(&new_time, str_time);
							            sprintf(msg, "Sending SIGSTOP to %s at %s", (p_htxshm_HE + index)->sdev_id, str_time);
							            fprintf (log_fp, "%s", msg);
							            if(htxd_get_equaliser_debug_flag() == 1) {
								            sprintf(msg, "htx_equaliser: Sending SIGSTOP to %s at %s", (p_htxshm_HE + index)->sdev_id, str_time);
								            htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
							            }
							            kill_return_code = 0;
							            kill_return_code = killpg((p_htxshm_HE + index)->PID, SIGSTOP);  /* transition to sleep */
							            if (kill_return_code != 0) {
							 	            sprintf(msg, "WARNING! Could not send SIGSTOP signal to the exerciser for %s.", (p_htxshm_HE + index)->sdev_id);
								            htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
							            } else {
								            (p_htxshm_HE + index)->equaliser_halt = 1;
                                        #ifdef __HTX_LINUX__
                                            if (test_config.offline_cpu) {
                                            /* bring CPU offline. This will automatically make process to unbind */
                                                pcpu = sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].pcpu;
                                                sprintf (str1, "echo 0 > /sys/devices/system/cpu/cpu%d/online", pcpu);
                                                system(str1);
							                }
                                        #endif
							            }
						            } else if((action == 1) && ((p_htxshm_HE + index)->equaliser_halt == 1)) {
							            clock = time((long *) 0);
								    localtime_r(&clock, &new_time);
								    asctime_r(&new_time, str_time);
                                    #ifdef __HTX_LINUX__
                                        if (test_config.offline_cpu) {
                                        /* bring CPU online and bind the process to that */
                                            pcpu = sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].pcpu;
                                            lcpu = sys_detail.nodes[node_num].chips[chip_num].cores[core_num].cpus[cpu_num].lcpu;
                                            sprintf (str1, "echo 1 > /sys/devices/system/cpu/cpu%d/online", pcpu);
                                            system(str1);
                                            rc = bind_process((p_htxshm_HE + index)->PID, lcpu, pcpu);
                                            if (rc < 0) {
                                                sprintf(msg, "could not bind process %d to cpu %d. rc= %d. Exiting...\n", (int)(p_htxshm_HE + index)->PID, pcpu, rc);
                                                htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
                                                exit(1);
                                            }
                                        }
                                    #endif
						                sprintf(msg, "Sending SIGCONT to %s at %s", (p_htxshm_HE + index)->sdev_id, str_time);
							            fprintf (log_fp, "%s", msg);
							            if(htxd_get_equaliser_debug_flag() == 1) {
							  	            sprintf(msg, "htx_equaliser: Sending SIGCONT to %s at %s", (p_htxshm_HE + index)->sdev_id, str_time);
								            htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
							            }
							            kill_return_code = 0;
							            kill_return_code = killpg((p_htxshm_HE + index)->PID, SIGCONT);  /* transition to wake  */
							            if(kill_return_code != 0) {
								            sprintf(msg, "WARNING! Could not send SIGCONT signal to the exerciser for %s.", (p_htxshm_HE + index)->sdev_id);
								            htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
							            } else {
								            (p_htxshm_HE + index)->equaliser_halt = 0;
							            }
						            }
                                    exer_conf[index].data.current_step = (exer_conf[index].data.current_step + 1) % exer_conf[index].config.pattern_length;
                                    if ( exer_conf[index].data.current_step == 0 && exer_conf[index].config.util_pattern != UTIL_PATTERN) {
                                        exer_conf[index].data.current_seq_step = (exer_conf[index].data.current_seq_step + 1) % exer_conf[index].config.sequence_length;
                                        exer_conf[index].data.target_utilization = exer_conf[index].config.utilization_sequence[exer_conf[index].data.current_seq_step];
                                        /* sprintf(msg, "target cpu utilization: %d", thread->data.targetUtilization);
                                        htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG); */
                                        if (exer_conf[index].config.sequence_length > 1 || exer_conf[index].config.util_pattern == UTIL_RANDOM) {
                                            reset_utilization_queue(index);
                                        }
                				    }
                                } else {
					                continue;
			    	            }
			                }
		                }
			        }
			    }
		    }
	    }
        fflush(log_fp);
		usleep(micro_seconds); /* scheduling quantum */
	}
}


/***********************************************************************/
/****** Function to Handles SIGTERM,SIGQUIT and SIGINT signals  ********/
/***********************************************************************/

void equaliser_sig_end(int signal_number, int code, struct sigcontext *scp)
     /*
      * signal_number -- the number of the received signal
      * code -- unused
      * scp -- pointer to context structure
      */

{
    	char workstr[512];

    	(void) sprintf(workstr, "Received signal %d.  Exiting...", signal_number);
    	(void) htxd_send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
        if (test_config.exer_config != NULL) {
        free(test_config.exer_config);
	}
	exit(0);
} /* equaliser_sig_end() */

