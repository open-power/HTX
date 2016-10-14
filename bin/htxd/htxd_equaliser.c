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
/* @(#)44	1.2  src/htx/usr/lpp/htx/bin/htxd/htxd_equaliser.c, htxd, htxubuntu 12/16/14 01:55:36 */




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

#define AFFINITY_INDEX		0
#define TEST_NAME_INDEX		1
#define EQ_CONTROL_INDEX	2
#define UTIL_SEQ_INDEX		3
#define UTIL_PAT_INDEX		4
#define MODE_INDEX			5
#define RULE_INDEX			6

test_config_struct test_config;
/* extern int equaliser_debug_flag; */

int num_tests = 0, total_tests=0;
int pattern_length = UTILIZATION_QUEUE_LENGTH;
int length_flag = 0;

void equaliser_sig_end(int signal_number, int code, struct sigcontext *scp);

/************************************************************/
/*************   Functions to Parse config file  ************/
/************************************************************/

int parse_line(char s[])
{
	int i = 0;

	while(s[i] == ' ' || s[i] == '\t') {
		i++;
	}
	if(s[i] == '#') {
		return(0);
	}
	return((s[i] == '\n') ? 1: 2);
}

int get_line(char line[], int lim, FILE *fp)
{
	int rc, err_no;
	char *p, msg[256];

	p = fgets(line, lim, fp);
	err_no = errno;
	if (p == NULL && feof(fp)) {
		return (-2);
	}
	else if (p == NULL && ferror(fp)) {
		sprintf(msg, "Error (errno = %d) in reading config file.\nEqualiser process exiting.", err_no);
		htxd_send_message(msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
		return(-1);
	}
	else {
	  	rc = parse_line(line);
		return rc;
	}
}

void update_thread_config(char *affinity, thread_config_params cur_config)
{
    char *chr_ptr[4], *ptr, msg[128], dev_name_str[32];
    char tmp_str[10], tmp[4];
    int i, j, k, l, m, exer_found;
    int node_num, chip_num, core_num, thread_num;
    int num_of_nodes, num_of_chips, num_of_cores, num_of_threads;
    run_time_thread_config *th;

    union shm_pointers shm_addr;
    struct htxshm_HE *p_htxshm_HE;    /* pointer to a htxshm_HE struct     */
    union shm_pointers shm_addr_wk;     /* work ptr into shm                 */

    shm_addr = htxd_get_equaliser_shm_addr();

    i = 0;
    if ((chr_ptr[i++] = strtok(affinity, "NPCT")) != NULL) {
        for (; i < 4; i++) {
            if ((chr_ptr[i] = strtok(NULL, "NPCT")) == NULL) {
                printf("Improper data is provided in line\n");
                exit(1);
            }
        }
    }
    /* sprintf (msg, "Inside update_thread_config\n");
    send_message (msg, 0, HTX_SYS_INFO, HTX_SYS_MSG); */
    shm_addr_wk.hdr_addr = shm_addr.hdr_addr;  /* copy addr to work space  */
    (shm_addr_wk.hdr_addr)++;         /* skip over header                  */

    strcpy(tmp_str, chr_ptr[0]);
    if (tmp_str[0] == '*') {
        node_num = 0;
        num_of_nodes = get_num_of_nodes_in_sys();
    } else if (strchr(tmp_str, '[') != NULL) { /* means a range of nodes is given */
        ptr = strtok(tmp_str, "[]-");
        node_num = atoi(ptr);
        ptr = strtok(NULL, "[]-");
        strcpy(tmp, ptr);
        if (tmp[0] == 'n' || tmp[0] == 'N') {
            num_of_nodes = get_num_of_nodes_in_sys();
            num_of_nodes = num_of_nodes - node_num;
        } else {
            num_of_nodes = atoi(ptr) - node_num + 1;
        }
    } else {
        node_num = atoi(chr_ptr[0]);
        num_of_nodes = 1;
    }
    for (i = 0; i < num_of_nodes; i++, node_num++) {
        if (chr_ptr[1][0] == '*') {
            chip_num = 0;
            num_of_chips = get_num_of_chips_in_node(node_num);
        } else if (strchr(chr_ptr[1], '[') != NULL) { /* means a range of chips is given */
            strcpy(tmp_str, chr_ptr[1]);
            ptr = strtok(tmp_str, "[]-");
            chip_num = atoi(ptr);
            ptr = strtok(NULL, "[]-");
            strcpy(tmp, ptr);
            if (tmp[0] == 'n' || tmp[0] == 'N') {
                num_of_chips = get_num_of_chips_in_node(node_num);
                num_of_chips -= chip_num;
            } else {
                num_of_chips = atoi(ptr) - chip_num + 1;
            }
        } else {
           chip_num = atoi(chr_ptr[1]);
           num_of_chips = 1;
        }
        for (j = 0; j < num_of_chips; j++, chip_num++) {
            if (chr_ptr[2][0] == '*') {
                core_num = 0;
                num_of_cores = get_num_of_cores_in_chip(node_num, chip_num);
            } else if (strchr(chr_ptr[2], '[') != NULL) { /* means a range of cores is defined */
                strcpy(tmp_str, chr_ptr[2]);
                ptr = strtok(tmp_str, "[]-");
                core_num = atoi(ptr);
                ptr = strtok(NULL, "[]-");
                strcpy(tmp, ptr);
                if (tmp[0] == 'n' || tmp[0] == 'N') {
                    num_of_cores = get_num_of_cores_in_chip(node_num, chip_num);
                    num_of_cores -= core_num;
                } else {
                    num_of_cores = atoi(ptr) - core_num + 1;
                }
            } else {
                core_num = atoi(chr_ptr[2]);
                num_of_cores = 1;
            }
            for (l = 0; l < num_of_cores; l++, core_num++) {
            #ifdef __HTX_LINUX__
                /* Exclude N0P0C0T* if offlining cpu is enabled */
                if (test_config.offline_cpu && core_num == 0 && chip_num == 0 && node_num == 0) {
                    continue;
                }
            #endif
                if (chr_ptr[3][0] == '*') {
                    thread_num = 0;
                    num_of_threads = get_num_of_cpus_in_core(node_num, chip_num, core_num);
                } else if (strchr(chr_ptr[3], '[') != NULL) {
                    strcpy(tmp_str, chr_ptr[3]);
                    ptr = strtok(tmp_str, "[]-");
                    thread_num = atoi(ptr);
                    ptr = strtok(NULL, "[]-");
                    strcpy(tmp, ptr);
                    if (tmp[0] == 'n' || tmp[0] == 'N') {
                        num_of_threads = get_num_of_cpus_in_core(node_num, chip_num, core_num);
                        num_of_threads -= thread_num;
                    } else {
                        num_of_threads = atoi(ptr) - thread_num + 1;
                    }
                } else {
                    thread_num =  atoi(chr_ptr[3]);
                    num_of_threads = 1;
                }
                for (m = 0; m < num_of_threads; m++, thread_num++) {
                    th = &test_config.thread[num_tests];
                    memcpy(&(th->th_config), &cur_config, sizeof(thread_config_params));
                    th->th_config.lcpu = get_logical_cpu_num(node_num, chip_num, core_num, thread_num);
                #ifdef __HTX_LINUX__
                    th->th_config.pcpu = get_logical_2_physical(th->th_config.lcpu);
                #endif
                    sprintf(dev_name_str, "%s%d", th->th_config.dev_name, th->th_config.lcpu);
                    htxd_send_message (dev_name_str, 0, HTX_SYS_INFO, HTX_SYS_MSG);
                    strcpy(th->th_config.dev_name, dev_name_str);

                    /* Check if exer entry exist in mdt file being run, otherwise, exit out */
                    p_htxshm_HE = shm_addr_wk.HE_addr;
                    exer_found = 0;
                    for (k=0; k < (shm_addr.hdr_addr)->num_entries; k++, p_htxshm_HE++) {
                        if (strcmp(th->th_config.dev_name, p_htxshm_HE->sdev_id) == 0) {
                            exer_found = 1;
                            break;
                        }
                    }
                    if ((k == (shm_addr.hdr_addr)->num_entries) && (exer_found == 0)) {
                        sprintf(msg, "Exerciser %s defined in config file is not found in MDT file currently being run.\nHence, equaliser process exiting.", th->th_config.dev_name);
                        htxd_send_message(msg, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
                        return(-1);
                    }
                    num_tests++;
                }
            }
        }
    }
}

/***************************************************************************/
/****** Function to read config file and update testConfig structure *******/
/***************************************************************************/

int read_config(void)
{
	FILE *fp;
	int exer_found, i, j, k, rc = 0;
	char line[MAX_LINE_SIZE], *chr_ptr[MAX_PARAMS_IN_ROW], *buf, keywd[64], msg[256];
	char found_quantum = 0;
	char affinity_str[16], filename[160];
	char *cfg_file;
    thread_config_params cur_config;


	cfg_file = htxd_get_equaliser_conf_file();

	sprintf(filename, "%s/", global_htx_home_dir);
	strcat(filename, cfg_file);

	/* printf("filename : <%s>\n", filename); */
	fp = fopen(filename, "r");
	if(fp == NULL) {
		sprintf(msg, "Error (errno = %d(%s)) in opening config file %s.\nEqualiser process exiting.", errno, strerror(errno), filename);
		htxd_send_message(msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
		return(-1);
	}

	rc = init_syscfg_with_malloc();
	if (rc) {
	    exit(1);
	}
	test_config.offline_cpu = 0;

	while (1) {
		rc = get_line(line, MAX_LINE_SIZE, fp);  /* gets one line at a time from config file  */
		/* printf("line found : <%s> : rc = <%d>\n", line, rc); */
		if (rc == 0 || rc == 1) {  /* Line is either blank or a comment */
			continue;
		}
		else if (rc == -1) {  /* Error in reading file  */
			fclose(fp);
			return(rc);
		}
		else if (rc == -2) {  /* End of file  */
			break;
		}
		else {  /* got some good data  */
			i = j = k = 0;
			/* printf("line is: %s\n", line); */
			sscanf(line,"%s", keywd);
            if (strcmp(keywd, "time_quantum") == 0) {
                sscanf(line, "%*s %*s %d", &test_config.time_quantum);
                found_quantum = 1;
            }
            else if (strcmp(keywd, "startup_time_delay") == 0) {
                sscanf(line, "%*s %*s %d", &test_config.startup_time_delay);
            }
            else if (strcmp(keywd, "log_duration") == 0) {
                sscanf(line, "%*s %*s %d", &test_config.log_duration);
            }
            else if (strcmp(keywd, "pattern_length") == 0) {
                sscanf(line, "%*s %*s %d", &pattern_length);
                length_flag = 1;
            }
            else if (strcmp(keywd, "offline_cpu") == 0) {
                sscanf(line, "%*s %*s %d", &test_config.offline_cpu);
			}
			else if ((chr_ptr[i++] = strtok(line," \n\t")) != NULL) {
				/*sprintf(msg, "chr_ptr[0]: %s\n", chr_ptr[0]);
				htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG); */
				for(; i<MAX_PARAMS_IN_ROW; i++) {
					if((chr_ptr[i] = strtok(NULL, " \t\n")) == NULL) {
						sprintf(msg, "Improper data provided for %s exerciser in config file.\nHence, equaliser process exiting.", chr_ptr[0]);
						htxd_send_message(msg, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
						return(-1);
					}
				 	/* sprintf(msg, "chr_ptr[%d]: %s\n", i, chr_ptr[i]);
					htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG); */
				}
				if ((strcmp(chr_ptr[EQ_CONTROL_INDEX], "N") == 0) || (strcmp(chr_ptr[EQ_CONTROL_INDEX], "n") == 0)) {
					continue;
				}
				sscanf(chr_ptr[AFFINITY_INDEX], "%s", affinity_str);
				sscanf(chr_ptr[TEST_NAME_INDEX], "%s", cur_config.dev_name);
				buf = strtok(chr_ptr[UTIL_SEQ_INDEX], "[],");
				cur_config.utilization_sequence[j++] = atoi(buf);
				while (( buf = strtok(NULL, " ,]")) != NULL && j < MAX_UTIL_SEQUENCE_LENGTH) {
					cur_config.utilization_sequence[j++] = atoi(buf);
				}
				if (buf != NULL && j == MAX_UTIL_SEQUENCE_LENGTH) {
					sprintf(msg, "Only upto 10 values are valid for utlization sequence. For %s, more than 10 values are provided for it in config file.\nHence Equaliser process will exit.", cur_config.dev_name);
					htxd_send_message(msg, 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
					return(-1);
				}
				cur_config.sequence_length = j;
				if (chr_ptr[UTIL_PAT_INDEX] != NULL) {
					if ((buf = strtok(chr_ptr[UTIL_PAT_INDEX], "[]")) != NULL) {
						cur_config.pattern_length = pattern_length;
						if (strcmp(buf, "UTIL_LEFT") == 0) {
							cur_config.util_pattern = UTIL_LEFT;
						}
						else if (strcmp(buf, "UTIL_RIGHT") == 0) {
							cur_config.util_pattern = UTIL_RIGHT;
						}
						else if (strcmp(buf, "UTIL_RANDOM") == 0) {
							cur_config.util_pattern = UTIL_RANDOM;
						}
						else {
							cur_config.utilization_pattern = strtoul(buf, NULL, 2);
							cur_config.util_pattern = UTIL_PATTERN;
							cur_config.pattern_length = strlen(buf);
						}
					}
				}
				update_thread_config(affinity_str, cur_config);
			}
		}
	}
	if (found_quantum == 0) {
		htxd_send_message("The variable \"time_quantum\" is not defined in config file. Hence, equaliser will exit.", 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		return(-1);
	}
    if (length_flag == 0) {
        sprintf(msg, "pattern_length is not defined in cfg file. considering the default pattern length of size %d", UTILIZATION_QUEUE_LENGTH);
        htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
    }
	if( num_tests == 0) {
		htxd_send_message("There should be atleast one test configured in config file to start equaliser functionality. Since no test is configured. Hence equaliser process will exit now.", 0, HTX_SYS_SOFT_ERROR, HTX_SYS_MSG);
		return(-1);
	}
	test_config.num_tests_configured = num_tests;
	return(0);
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

void reset_utilization_queue(run_time_thread_config *run_time_th)
{
	int j;
	int target_active_in_decs;
	char buf[20], msg[128], workstr[512];

	target_active_in_decs = (run_time_th->data.target_utilization * run_time_th->th_config.pattern_length) / 100;
	run_time_th->th_config.utilization_pattern = 0;
	switch (run_time_th->th_config.util_pattern) {
		case 0:
			for (j = 1; j <= target_active_in_decs; j++) {
				run_time_th->th_config.utilization_pattern |= ( 1 << (run_time_th->th_config.pattern_length - j));
				strcpy(buf, "UTIL_LEFT");
			}
			break;

		case 1:
			for (j = 1; j <= target_active_in_decs; j++) {
				run_time_th->th_config.utilization_pattern |= ( 1 << (target_active_in_decs - j));
				strcpy(buf, "UTIL_RIGHT");
			}
			break;

		case 2:
			for (j = 1; j <= target_active_in_decs; j++) {
				run_time_th->th_config.utilization_pattern |= ( 1 << (run_time_th->th_config.pattern_length - j));
			}
			init_random_no();
			shuffle_pattern(run_time_th->th_config.pattern_length, &(run_time_th->th_config.utilization_pattern));
			strcpy(buf, "UTIL_RANDOM");
			break;
	}
	/* if(equaliser_debug_flag == 1) { */
	if(htxd_get_equaliser_debug_flag() == 1) {
		sprintf(workstr, "Device Name: %s\nUtilization Sequence:", run_time_th->th_config.dev_name);
		for (j=0; j< run_time_th->th_config.sequence_length; j++) {
			sprintf(msg,"%d ", run_time_th->th_config.utilization_sequence[j]);
			strcat(workstr, msg);
		}
		strcat(workstr, "\n");
		sprintf(msg, "Target Utilization: %d\nUtilization pattern: 0x%x(%s)", run_time_th->data.target_utilization, run_time_th->th_config.utilization_pattern, buf);
		strcat(workstr, msg);
		htxd_send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}
}

/***************************************************************************/
/**************  Function to initialize runtime data  **********************/
/***************************************************************************/

void initialize_threads_data(void)
{
	run_time_thread_config *th;
	int i, j;
	char workstr[512], msg[256];

        th = test_config.thread;
        for(i=0; i<test_config.num_tests_configured; i++, th++) {
		th->data.target_utilization = th->th_config.utilization_sequence[0];
        	if( th->th_config.util_pattern != UTIL_PATTERN) {
			reset_utilization_queue(th);
		}
		else {
			sprintf(workstr, "Device Name: %s\nUtilization Sequence:", th->th_config.dev_name);
			for (j=0; j< th->th_config.sequence_length; j++) {
				sprintf(msg,"%d ", th->th_config.utilization_sequence[j]);
				strcat(workstr, msg);
			}
			strcat(workstr, "\n");
			sprintf(msg, "Target Utilization: ignored as utilization pattern defined explicitly in config file\nUtilization pattern: 0x%x", th->th_config.utilization_pattern);
			strcat(workstr, msg);
			htxd_send_message(workstr, 0, HTX_SYS_INFO, HTX_SYS_MSG);
		}
		th->data.current_step = 0;
		th->data.current_seq_step = 0;
	}
}



void htxd_equaliser(void)
{
  /*
   ***************************  variable definitions  *****************************
   */
	int semhe_id;        /* semaphore id                      */
	union shm_pointers shm_addr;  /* shared memory union pointers      */

	char buf[128], workstr[512], str1[128], msg[256];
	FILE *fp, *log_fp;
 	int num_entries = 0, kill_return_code = 0;
  	int i, j, rc;               /* loop counter     */
	int action;
	int max_num_tests;
	useconds_t micro_seconds;
	long clock, tm_log_save;
	char *p_tm;
	struct stat buf1;

	static char process_name[] = "htx_equaliser";
	struct htxshm_HE *p_htxshm_HE;    /* pointer to a htxshm_HE struct     */
	struct semid_ds	sembuffer;       /* semaphore buffer                  */
	union shm_pointers shm_addr_wk;     /* work ptr into shm                 */
	struct sigaction sigvector;      /* structure for signals */
	run_time_thread_config *thread;
	char equaliser_log_file[300];
	char equaliser_log_file_save[300];

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
#ifndef __HTX_LINUX_

	sprintf(str1, "bindprocessor -q | awk '{ print $NF}'");
	fp = popen(str1,"r");
	fgets(buf, 100,fp);
	sscanf(buf, "%d", &max_num_tests);
	max_num_tests++;
#else
	sprintf(str1, "cat /proc/cpuinfo | grep processor | wc -l");
	fp = popen(str1,"r");
	sscanf(buf, "%d", &max_num_tests);
#endif

	pclose(fp);
	printf("DEBUG: point 1\n");
	test_config.thread = (run_time_thread_config *) malloc(sizeof(run_time_thread_config) * (shm_addr.hdr_addr)->max_entries);
	if( test_config.thread == NULL) {
		sprintf(msg, "Error (errno = %d) in allocating memory for equaliser process.\nProcess will exit now.", errno);
		htxd_send_message(msg, 0, HTX_SYS_HARD_ERROR, HTX_SYS_MSG);
		/* PRTMSG(MSGLINE, 0, ("ERROR:Equaliser process exiting. See /tmp/htxerr for more details.")); */
		exit(1);
	}

	printf("DEBUG: point 2\n");
	test_config.startup_time_delay = 0;
	test_config.log_duration = 0;
	rc = read_config();
	if(rc == -1) {
	printf("DEBUG: point 3\n");
		free(test_config.thread);
		/* PRTMSG(MSGLINE, 0, ("ERROR:Equaliser process exiting. See /tmp/htxerr for more details.")); */
		exit(1);
	}
	else {
	printf("DEBUG: point 4\n");
		htxd_send_message("Done reading config file", 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}
/*	if(equaliser_debug_flag == 1) {*/
	if(htxd_get_equaliser_debug_flag() == 1) {
		sprintf(msg, "Total tests configured:%d\nTests under equaliser control:%d", total_tests, test_config.num_tests_configured);
		htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
	}
	initialize_threads_data();

  /*********************************************************************************/
  /***********************  set up shared memory addressing  ***********************/
  /*********************************************************************************/

	printf("DEBUG: point 5\n");
 	shm_addr_wk.hdr_addr = shm_addr.hdr_addr;  /* copy addr to work space  */
  	(shm_addr_wk.hdr_addr)++;         /* skip over header                  */

	micro_seconds = test_config.time_quantum * 1000;
	if(test_config.startup_time_delay != 0) {
		sprintf(msg, "Equaliser will take %d sec(startup_time_delay) to be effective.", test_config.startup_time_delay);
		htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
		sleep(test_config.startup_time_delay);
	}
	log_fp = fopen(equaliser_log_file, "w");
	tm_log_save = time((long *) 0);
	printf("DEBUG: point 6\n");
	while(1) {

		clock = time((long *) 0);
		if((clock - tm_log_save) >= test_config.log_duration ) {
			sprintf(workstr, "cp %s %s", equaliser_log_file, equaliser_log_file_save);
			system(workstr);
			fclose(log_fp);
			tm_log_save = clock;
			log_fp = fopen(equaliser_log_file, "w");
		}
		thread = test_config.thread;
		for (i=0; i < test_config.num_tests_configured; i++, thread++) {
			p_htxshm_HE = shm_addr_wk.HE_addr;
			num_entries = (shm_addr.hdr_addr)->max_entries;
			for(j=0; j<num_entries; j++, p_htxshm_HE++) {
				if(strcmp(thread->th_config.dev_name, p_htxshm_HE->sdev_id) == 0) {
					if ((p_htxshm_HE->PID != 0) &&     /* Not Dead?  */
				            (p_htxshm_HE->tm_last_upd != 0) &&     /* HE started?  */
					    ((p_htxshm_HE->max_cycles == 0) || ((p_htxshm_HE->max_cycles != 0) && (p_htxshm_HE->cycles < p_htxshm_HE->max_cycles)))  &&  /* not completed  */
					    (semctl(semhe_id, 0, GETVAL, &sembuffer) == 0) &&   /* system active?   */
					    (semctl(semhe_id, ((j * SEM_PER_EXER) + SEM_GLOBAL + 1), GETVAL, &sembuffer) == 0) && /* Not errored out  */
					    (semctl(semhe_id, ((j * SEM_PER_EXER) + SEM_GLOBAL), GETVAL, &sembuffer) == 0))  /* Not stopped from option 2 */
					{
						action = ((thread->th_config.utilization_pattern) >> (thread->th_config.pattern_length - (thread->data.current_step + 1))) & 0x1;
					 	/* sprintf(msg, "Acth_config.tion is:%d", Action);
						htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);  */
						if((action == 0) && (p_htxshm_HE->equaliser_halt == 0)) {
							clock = time((long *) 0);
							p_tm = ctime(&clock);
							sprintf(msg, "Sending SIGSTOP to %s at %s", thread->th_config.dev_name, p_tm);
							fprintf (log_fp, "%s", msg);
							if(htxd_get_equaliser_debug_flag() == 1) {
								sprintf(msg, "htx_equaliser: Sending SIGSTOP to %s at %s", thread->th_config.dev_name, p_tm);
								htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
							}
							kill_return_code = 0;
							kill_return_code = killpg(p_htxshm_HE->PID, SIGSTOP);  /* transition to sleep */
							if(kill_return_code != 0) {
								sprintf(msg, "WARNING! Could not send SIGSTOP signal to the exerciser for %s.", p_htxshm_HE->sdev_id);
								htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
							}
							else {
								p_htxshm_HE->equaliser_halt = 1;
                            #ifdef __HTX_LINUX__
                                if (test_config.offline_cpu) {
                                    /* bring CPU offline. This will automatically make process to unbind */
                                    sprintf (str1, "echo 0 > /sys/devices/system/cpu/cpu%d/online", thread->th_config.pcpu);
                                    system(str1);
							    }
                            #endif
							}
						}
						else if((action == 1) && (p_htxshm_HE->equaliser_halt == 1)) {
							clock = time((long *) 0);
							p_tm = ctime(&clock);
                        #ifdef __HTX_LINUX__
                            if (test_config.offline_cpu) {
                                /* bring CPU online and bind the process to that */
                                sprintf (str1, "echo 1 > /sys/devices/system/cpu/cpu%d/online", thread->th_config.pcpu);
                                system(str1);
                                rc = bind_process(p_htxshm_HE->PID, thread->th_config.lcpu, thread->th_config.pcpu);
                                if (rc < 0) {
                                    sprintf(msg, "could not bind process %d to cpu %d. rc= %d. Exiting...\n", (int)p_htxshm_HE->PID, thread->th_config.pcpu, rc);
                                    htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
                                    exit(1);
                                }
                            }
                        #endif
							sprintf(msg, "Sending SIGCONT to %s at %s", thread->th_config.dev_name, p_tm);
							fprintf (log_fp, "%s", msg);
							if(htxd_get_equaliser_debug_flag() == 1) {
								sprintf(msg, "htx_equaliser: Sending SIGCONT to %s at %s", thread->th_config.dev_name, p_tm);
								htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
							}
							kill_return_code = 0;
							kill_return_code = killpg(p_htxshm_HE->PID, SIGCONT);  /* transition to wake  */
							if(kill_return_code != 0) {
								sprintf(msg, "WARNING! Could not send SIGCONT signal to the exerciser for %s.", p_htxshm_HE->sdev_id);
								htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG);
							}
							else {
								p_htxshm_HE->equaliser_halt = 0;
							}
						}
						thread->data.current_step = (++thread->data.current_step % thread->th_config.pattern_length);
						if ( thread->data.current_step == 0 && thread->th_config.util_pattern != UTIL_PATTERN) {
							thread->data.current_seq_step = (++thread->data.current_seq_step % thread->th_config.sequence_length);
 							thread->data.target_utilization = thread->th_config.utilization_sequence[thread->data.current_seq_step];
							/* sprintf(msg, "target cpu utilization: %d", thread->data.targetUtilization);
							htxd_send_message(msg, 0, HTX_SYS_INFO, HTX_SYS_MSG); */
							if (thread->th_config.sequence_length > 1 || thread->th_config.util_pattern == UTIL_RANDOM) {
								reset_utilization_queue(thread);
							}
						}
					}
				}
				else {
					continue;
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
    	if (test_config.thread != NULL) {
		free(test_config.thread);
	}
	exit(0);
} /* equaliser_sig_end() */

