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
#include <errno.h>
#include "htxsyscfg64.h"

#define MAX_PATTERN		32
#define MAX_PATTERN_LENGTH 	(2 * MAX_PATTERN) + 2
#define MAX_UTIL_SEQ		10
#define NUM_OF_SETS		5
#define MAX_SMT			8

#define FXU 	    0
#define VSU	    1
#define ANY	    2

char pattern[MAX_PATTERN][MAX_PATTERN_LENGTH];
int util_seq[MAX_SMT][MAX_UTIL_SEQ];
char util_str[MAX_SMT][64];
char mem_eq_rulefile[32];

SYS_STAT sys_stat;
htxsyscfg_smt_t t;
htxsyscfg_memory_t v;
int pvr, smt_threads;
FILE *fp;

struct thread_info {
    int num_threads;
    int thread_num[MAX_SMT];
};

int open_file (char *);
void make_entry(int, int);
void create_pattern(void);
void create_cfg_maxpwr_switch(void);
void create_cfg_maxpwr_exp(void);
void create_cfg_th_trans_switch(void);
void create_cfg_th_trans_mix(void);
void create_th_switch_seq(void);
void create_cfg_100_75_50_25_util(void);
void create_cfg_cpu_mem_50(void);
void create_cfg_msrp_1p3m_28s_act_52s_idle(void);
void create_cfg_p8_8fd_char(void);
void create_cfg_p8_rdp_switch(void);
void create_cfg_p8_swicthing_cmp(void);
void get_htx_home_dir(char *);

int main(int argc, char **argv)
{
    int rc = 0, time_quantum, wof_test = 0;
    char cfg_file[128], eq_cfg_path[128], *tmp_ptr = NULL;

    rc = init_syscfg_with_malloc();
    if (rc == -1) {
        printf("Could not collect system config detail. Exiting...\n");
        exit(1);
    }

    get_hardware_stat(&sys_stat);
    get_smt_details(&t);
    smt_threads = t.min_smt_threads;
    pvr = get_cpu_version();
    pvr = pvr >> 16;

    /* Get eq_cfg_path from env. variable */
    tmp_ptr = getenv("HTXCFGPATH");
    if (tmp_ptr != NULL) {
		strcpy(eq_cfg_path, tmp_ptr);
    } else {
        printf("Looks like HTX env. is not setup. Hence exiting.\nPlease setup teh env. and re-run.\n");
        exit(1);
    }

    /* Get the base page size. Based on this, we will either use default.mem.eq.4k OR */
    /* default.mem.eq.64k rule file.                                                  */
    rc = get_memory_details(&v);
    if (v.page_details[PAGE_INDEX_4K].supported) {
        strcpy(mem_eq_rulefile, "default.mem.eq.4k");
    } else if (v.page_details[PAGE_INDEX_64K].supported) {
        strcpy(mem_eq_rulefile, "default.mem.eq.64k");
    }

    /***************************************************************************************/
    /* Type of workloads for below 2 cfgs:                                                 */
    /* The kind of workload will be such that when the core is in the list of active cores */
    /* it will consume maximum power. The type of workload whiich will be used for it is   */
    /* as below:                                                                           */
    /* 50% of threads of a core will be running hxefpu64 with VSX workload.                */
    /* 25% of threads of a core will be running hxecpu with fixed point type workload.     */
    /*  25% of threads of a core will be running hxemem64 to have cache and memory stress. */
    /***************************************************************************************/

    /*  Create cfg file for switching test case. */
    sprintf(cfg_file, "%s/htx_eq_maxpwr_switch.cfg", eq_cfg_path);
    open_file(cfg_file);
    time_quantum = 100;
    make_entry(time_quantum, wof_test); /* Will create common entries in cfg file. */
    create_cfg_maxpwr_switch(); /* will create exerciser entries in cfg file. */
    fclose(fp);

    /*  Create cfg file for exponential inc./dec. test case. */
    sprintf(cfg_file, "%s/htx_eq_maxpwr_exp.cfg", eq_cfg_path);
    open_file(cfg_file);
    time_quantum = 4000;
    make_entry(time_quantum, wof_test);
    create_cfg_maxpwr_exp();
    fclose(fp);

    /* Below 2 mdts are created only if SMT is multiple of 4 */
    if (smt_threads % 4 == 0) {
		/* Create cfg file for 100/75/50/25 % cpu load */
        sprintf(cfg_file, "%s/htx_eq_100_75_50_25_util.cfg", eq_cfg_path);
        open_file(cfg_file);
        time_quantum = 300000;
        make_entry(time_quantum, wof_test);
        create_cfg_100_75_50_25_util();
        fclose(fp);

        /* Create cfg file for 50% cpu & 50% memory usage */
        sprintf(cfg_file, "%s/htx_eq_cpu_mem_50.cfg", eq_cfg_path);
        open_file(cfg_file);
        time_quantum = 300000;
        make_entry(time_quantum, wof_test);
        create_cfg_cpu_mem_50();
        fclose(fp);
    }

    /************************************************************/
    /* Below 2 cfgs will be created to test "thread transition" */
    /* feature in P8. switching cfg will be created generic i.e.*/
    /* even for P7. These will be created if smt is > 1.        */
    /************************************************************/

    if (smt_threads > 1) {
		sprintf(cfg_file, "%s/htx_eq_th_trans_switch.cfg", eq_cfg_path);
        open_file(cfg_file);
        time_quantum = 6000;
        make_entry(time_quantum, wof_test);
        create_cfg_th_trans_switch();
        fclose(fp);

        if ((pvr == 75 || pvr == 76 || pvr == 77) && (smt_threads == 8)) { /* only for P8 */
            sprintf(cfg_file, "%s/htx_eq_th_trans_mix.cfg", eq_cfg_path);
            open_file(cfg_file);
            time_quantum = 6000;
            make_entry(time_quantum, wof_test);
            create_cfg_th_trans_mix();
            fclose(fp);
        }
    }

    /* Below are hxeewm cfgs */
    /* Below cfg is created only if SMT is multiple of 4 */
    if (smt_threads % 4 == 0) {
		sprintf(cfg_file, "%s/htx_ewm_msrp_1p3m_28s_act_52s_idle.cfg", eq_cfg_path);
        open_file(cfg_file);
        time_quantum = 4000;
        make_entry(time_quantum, wof_test);
        create_cfg_msrp_1p3m_28s_act_52s_idle();
        fclose(fp);
    }

    /* Below 3 mdts are created for P8 with SMT 8 */
    if ((pvr == 75 || pvr == 76 || pvr == 77) && (smt_threads == 8)) { /* only for P8 */
        sprintf(cfg_file, "%s/htx_ewm_p8_8fd_char.cfg", eq_cfg_path);
        open_file(cfg_file);
        time_quantum = 2000;
        make_entry(time_quantum, wof_test);
        create_cfg_p8_8fd_char();
        fclose(fp);

        sprintf(cfg_file, "%s/htx_ewm_p8_rdp_switch.cfg", eq_cfg_path);
        open_file(cfg_file);
        time_quantum = 400;
        make_entry(time_quantum, wof_test);
        create_cfg_p8_rdp_switch();
        fclose(fp);

        sprintf(cfg_file, "%s/htx_ewm_p8_swicthing_cmp.cfg", eq_cfg_path);
        open_file(cfg_file);
        time_quantum = 800;
        make_entry(time_quantum, wof_test);
        create_cfg_p8_swicthing_cmp();
        fclose(fp);
    }
    return 0;
}


int open_file (char *file_name)
{
    fp = fopen (file_name, "w");
    if (fp == NULL) {
        printf("Error opening config file %s. errno: %d\n", file_name, errno);
        exit(1);
    }
    return 0;
}

void get_htx_home_dir(char *path)
{
    FILE *fp_tmp = NULL;

    fp_tmp = fopen("/var/log/htx_install_path", "r");
    if (fp_tmp == NULL) {
	    printf("ERROR: HTX environment variable HTX_HOME_DIR is not, please export HTX_HOME_DIR with value as HTX installation directory and re-try...\n");
	    exit(1);
    }
    fscanf(fp_tmp, "%s", path);
    fclose(fp_tmp);
}

void make_entry(int time_quantum, int wof_test)
{
    fprintf(fp, "# if first letter in the line is \"#\", its taken as a comment\n\n");
    fprintf(fp, "#timeQuantum in milliseconds\n");
    fprintf(fp, "time_quantum = %d\n\n", time_quantum);
    fprintf(fp, "#startup time delay specified in seconds\n");
    fprintf(fp, "startup_time_delay = 30\n\n");
    fprintf(fp, "# Log equalizer status for the last <log_duration> (in secs) only\n");
    fprintf(fp, "log_duration = 60\n\n");
    if (wof_test == 1) {
        fprintf(fp, "wof_test = 1\n\n");
    }
    fprintf(fp, "# In utilizationSequence,upto 10 steps are allowed.\n#\n");
    fprintf(fp, "#\tResource Name\tComma separated device list with various input params\n");
    fprintf(fp, "#\t             \t<dev_name>[<rule_file>:start_delay:runtime:utilzation]\n");
    fprintf(fp, "#\t-------------\t------------------------------------------------------\n");
}

/**************************************************************/
/*  Create cfg file for switching test case. Workload pattern */
/*  will be such that the power consumption for all the cores */
/* (together) will be switching instantly between 0 and Pmax. */
/**************************************************************/
void create_cfg_maxpwr_switch()
{
    int fpu_inst = 0, cpu_inst = 0, mem_inst = 0;
    int j, k, left;

    /***************************************************************************************/
    /* Type of workloads:                                                                  */
    /* The kind of workload will be such that when the core is in the list of active cores */
    /* it will consume maximum power. The type of workload whiich will be used for it is   */
    /* as below:                                                                           */
    /* 50% of threads of a core will be running hxefpu64 with VSX workload.                */
    /* 25% of threads of a core will be running hxecpu with fixed point type workload.     */
    /*  25% of threads of a core will be running hxemem64 to have cache and memory stress. */
    /***************************************************************************************/
    fpu_inst = smt_threads / 2;
    if (fpu_inst == 0) {
        fpu_inst = 1;
    } else {
        cpu_inst = fpu_inst / 2;
        if (cpu_inst == 0) {
            cpu_inst = 1;
        } else {
            mem_inst = cpu_inst;
        }
        left = smt_threads % (fpu_inst + cpu_inst + mem_inst);
        while (left != 0) {
            if (mem_inst == 0 || left % 2 == 0) {
                mem_inst++;
            } else {
                cpu_inst++;
            }
            left--;
        }
    }
    for (j = 0, k = 0; j < fpu_inst; j++, k++) {
        fprintf(fp, "\tN*P*C*T%d\t\tfpu[default.fpu.eq:0:0:UTIL_LEFT%%50]\n", k);
    }
    for (j = 0; j < cpu_inst; j++, k++) {
        fprintf(fp, "\tN*P*C*T%d\t\tcpu[default.cpu.eq:0:0:UTIL_LEFT%%50]\n", k);
    }
    for (j = 0; j < mem_inst; j++, k++) {
        fprintf(fp, "\tN*P*C*T%d\t\tmem[%s:0:0:UTIL_LEFT%%50]\n", k, mem_eq_rulefile);
    }
}

void create_pattern()
{
    int num_of_patterns, num_of_cores, pattern_length;
    int i, j, k, picked;

    num_of_cores = sys_stat.cores;
    num_of_patterns = num_of_cores;
    if (num_of_patterns > MAX_PATTERN) {
        num_of_patterns = MAX_PATTERN;
    }
    pattern_length = 2 * num_of_patterns;

    for (i = 0; i < num_of_patterns; i++) {
        for (j = 0; j < pattern_length; ) {
            picked = 0;
            if (i == j) {
                picked = 1;
            }
            if (picked == 1) {
                for (k = 0; k < num_of_patterns; k++, j++) {
                    pattern[i][j] = '1';
                }
            } else {
                pattern[i][j] = '0';
                j++;
            }
        }
        pattern[i][j] = '\0';
/*      printf("pattern: %s\n", pattern[i]);
 *          for (l=0; l < pattern_length; l++) {
 *              printf("%c", pattern[i][l]);
 *          }
 *          printf("\n");
 */
    }
}

/******************************************************************/
/*  Create cfg file for exponential inc./dec. test case. workload */
/*  pattern will be such that power consumption of the whole      */
/* system will increase/decrease to Pmax./0 gradually. This will  */
/* be achieved by adding/deleting 1 core to the active list of    */
/* cores (which are having workload running) at regular intervals.*/
/******************************************************************/
void create_cfg_maxpwr_exp()
{
    int cores_per_set[MAX_PATTERN] = {0};
    int num_of_cores, pattern_num;
    int allocated_cores, remaining_cores;
    int i, j, k, left;
    int fpu_inst = 0, cpu_inst = 0, mem_inst = 0;
    int num_nodes, num_chips, num_cores;
    char cur_pattern[MAX_PATTERN_LENGTH];

    /***************************************************************************************/
    /* Type of workloads:                                                                  */
    /* The kind of workload will be such that when the core is in the list of active cores */
    /* it will consume maximum power. The type of workload whiich will be used for it is   */
    /* as below:                                                                           */
    /* 50% of threads of a core will be running hxefpu64 with VSX workload.                */
    /* 25% of threads of a core will be running hxecpu with fixed point type workload.     */
    /* 25% of threads of a core will be running hxemem64 to have cache and memory stress.  */
    /***************************************************************************************/

    create_pattern();

    /* max. no. of utilization seq. pattern created will be 32. If no. of cores are
     * more than 32, we do grouping of cores.
     */
    num_of_cores = sys_stat.cores;
    if (num_of_cores >= MAX_PATTERN) {
        for (i = 0; i < MAX_PATTERN; i++) {
            cores_per_set[i] = num_of_cores / MAX_PATTERN;
        }
    }
    remaining_cores = num_of_cores % MAX_PATTERN;
    for (i = 0; i < remaining_cores; i++) {
        cores_per_set[i]++;
    }

    fpu_inst = smt_threads / 2;
    if (fpu_inst == 0) {
        fpu_inst = 1;
    } else {
        cpu_inst = fpu_inst / 2;
        if (cpu_inst == 0) {
            cpu_inst = 1;
        } else {
            mem_inst = cpu_inst;
        }
        left = smt_threads % (fpu_inst + cpu_inst + mem_inst);
        while (left != 0) {
            if (mem_inst == 0 || left % 2 == 0) {
                mem_inst++;
            } else {
                cpu_inst++;
            }
            left--;
        }
    }

    pattern_num = 0;
    allocated_cores = 0;
    strcpy(cur_pattern, pattern[pattern_num]);
    num_nodes = get_num_of_nodes_in_sys();
    for (i = 0; i < num_nodes; i++) {
        num_chips = get_num_of_chips_in_node(i);
        for (j = 0; j < num_chips; j++) {
            num_cores = get_num_of_cores_in_chip(i, j);
            for (k = 0; k < num_cores; k++) {
                if (fpu_inst == 1) {
                    fprintf(fp, "\tN%dP%dC%dT%d\t\tfpu[default.fpu.eq:0:0:%s]\n", i, j, k, (fpu_inst-1), cur_pattern);
                } else {
                    fprintf(fp, "\tN%dP%dC%dT[0-%d]\t\tfpu[default.fpu.eq:0:0:%s]\n", i, j, k, (fpu_inst-1), cur_pattern);
                }
                if (cpu_inst != 0) {
                    if (cpu_inst == 1) {
                        fprintf(fp, "\tN%dP%dC%dT%d\t\tcpu[default.cpu.eq:0:0:%s]\n", i, j, k, fpu_inst, cur_pattern);
                    } else {
                        fprintf(fp, "\tN%dP%dC%dT[%d-%d]\t\tcpu[default.cpu.eq:0:0:%s]\n", i, j, k, fpu_inst, (fpu_inst + cpu_inst -1), cur_pattern);
                    }
                }
                if (mem_inst != 0) {
                    if (mem_inst == 1) {
                        fprintf(fp, "\tN%dP%dC%dT%d\t\tmem[%s:0:0:%s]\n", i, j, k, (fpu_inst + cpu_inst), mem_eq_rulefile, cur_pattern);
                    } else {
                        fprintf(fp, "\tN%dP%dC%dT[%d-%d]\t\tmem[%s:0:0:%s]\n", i, j, k, (fpu_inst + cpu_inst), (fpu_inst + cpu_inst + mem_inst -1), mem_eq_rulefile, cur_pattern);
                    }
                }
                allocated_cores++;
                if (allocated_cores == cores_per_set[pattern_num]) {
                    allocated_cores = 0;
                    pattern_num++;
                    if (pattern_num < MAX_PATTERN) {
                        strcpy(cur_pattern, pattern[pattern_num]);
                    }
                }
            }
        }
    }
}

void create_th_switch_seq()
{
    int i, j, k, threads;
    int cur_no_act_threads, next_no_act_threads, picked_act_thread;
    int smt_array[MAX_SMT];
    int smt_array_size, picked, rand_num, thread_num[MAX_SMT];
    char tmp_str[6];

    /* create array which contain the SMT values to which a core can switch. */
    for (smt_array_size = 0, threads = smt_threads; threads > 1; smt_array_size++) {
        threads = threads / 2;
        smt_array[smt_array_size] = threads;
    }

    /* Initally all the threads should be running.So, all having utilzation as 100% */
    cur_no_act_threads = smt_threads;
    for (i = 0; i < smt_threads; i++) {
        util_seq[i][0] = 100;
        sprintf(tmp_str, "%d_", util_seq[i][0]);
        strcpy(util_str[i], tmp_str);
    }

    /* Create a 2 dimensional array of size m X n. where m is smt_threads and n will be num
     * of % utilization i.e. 10  for each thread.
     */
    for (i =1; i < MAX_UTIL_SEQ; i++) {

        /* Initialize the %utiliation to zero */
        for (k = 0; k < smt_threads; k++) {
            util_seq[k][i] = 0;
        }
        for (k=0; k < smt_threads; k++) {
            thread_num[k] = k;
        }

        /* pick a value from smt_array to which SMT switching will happen. That
         * will be next num of active threads.
         */
        picked = rand() % smt_array_size;
        next_no_act_threads = smt_array[picked];

        /* replace the next_no_act_threads value in smt_array with cur_no_act_threads so that
         * it can be picked again
         */
        /* printf ("next_no_act_threads: %d\n", next_no_act_threads); */
        smt_array[picked] = cur_no_act_threads;
        cur_no_act_threads = next_no_act_threads;

        /* picked randomly next_no_act_threads from thread_num array and make their % utilization
         * as 100. Rest will be 0.
         */
        for (j = 0; j < next_no_act_threads; j++) {
            rand_num = rand() % (smt_threads - j);
            picked_act_thread = thread_num[rand_num];
            util_seq[picked_act_thread][i] = 100;
            for (k = rand_num; k < (smt_threads - j - 1); k++) {
                thread_num[k] = thread_num[k+1];
            }
        }
        for (j = 0; j < smt_threads; j++) {
            sprintf(tmp_str, "%d_", util_seq[j][i]);
            strcat(util_str[j], tmp_str);
        }
    }
/*  for (i = 0; i < smt_threads; i++) {
        strcat(util_str[i], "}");
        for (j = 0; j < MAX_UTIL_SEQ; j++) {
            printf ("%3d\,",util_seq[i][j]);
        }
        printf ("%40s\n", util_str[i]);
    }
*/
}

/********************************************************************/
/* This function will create a equaliser config file to test "thread */
/* transition" feature in P8 with switching scenario. The utilzation*/
/* sequence for cpu workload on each hardware thread will be created*/
/* such that cpu cores will keep switching SMT.                     */
/********************************************************************/
void create_cfg_th_trans_switch()
{
   int i, j, k, l, cores_per_set[NUM_OF_SETS] = {0};
   int num_of_cores, remaining_cores, allocated_cores, allocated_cores_per_set;
   int set_num, num_nodes, num_chips, num_cores;

    /* only 5 different group of sequences will be genrated. Each group will
     * be further having no. of sequence equal to no. of SMT threads enabled.
     * The available cores will be grouped together for these sequences.
     */
    num_of_cores = sys_stat.cores;
    if (num_of_cores >= NUM_OF_SETS) {
        for (i = 0; i < NUM_OF_SETS; i++) {
            cores_per_set[i] = num_of_cores / NUM_OF_SETS;
        }
    }
    remaining_cores = num_of_cores % NUM_OF_SETS;
    for (i = 0; i < remaining_cores; i++) {
        cores_per_set[i]++;
    }

    allocated_cores = 0;
    allocated_cores_per_set = 0;
    set_num = 0;
    create_th_switch_seq();
    num_nodes = get_num_of_nodes_in_sys();
    for (i = 0; i < num_nodes; i++) {
        num_chips = get_num_of_chips_in_node(i);
        for (j = 0; j < num_chips; j++) {
            num_cores = get_num_of_cores_in_chip(i, j);
            for (k = 0; k < num_cores; k++) {
                for (l = 0; l < smt_threads; l++) {
                    fprintf(fp, "\tN%dP%dC%dT%d\t\tcpu[default.cpu.eq:0:0:UTIL_LEFT%%%s]\n", i, j, k, l, util_str[l]);
                }
                allocated_cores++;
                allocated_cores_per_set++;
                if (allocated_cores_per_set ==  cores_per_set[set_num] && allocated_cores < num_cores) {
                    allocated_cores_per_set = 0;
                    set_num++;
                    create_th_switch_seq();
                }
            }
        }
    }
}

void create_th_mix_seq(struct thread_info *FXU_threads, struct thread_info *VSU_threads, char removal_flag, int threads_to_remove)
{
    int i, j, k, picked, thread_num[MAX_SMT];
    int th_num, next_no_act_threads, cur_no_act_threads;
    int picked_act_thread;
    struct thread_info removal_array, fixed_array;
    int threads, smt_array_size, smt_array[MAX_SMT];
    char tmp_str[32];

    for (k = 0; k < smt_threads; k++) {
        thread_num[k] = k;
    }

    for (i = 0; i < smt_threads; i++) {
        strcpy(util_str[i], "");
    }

    /* allocate thread nos. to run VSU or FXU on */
    for (i = 0; i < FXU_threads->num_threads; i++) {
        picked = rand() % (smt_threads - i);
        FXU_threads->thread_num[i] = thread_num[picked];
        for (j = picked; j < (smt_threads - i - 1); j++) {
            thread_num[j] = thread_num[j + 1];
        }
    }
    for (i = 0; i < VSU_threads->num_threads; i++) {
        VSU_threads->thread_num[i] = thread_num[i];
    }

    /* Now, we will see from which workload type (i.e. FXU or VSU), we need to remove the threads
     * and set the utilization % accordingly.
     */
    /* Each element of utilization sequence array should have 9 values.During each pass 3 values
     * will be generated.
     */
    for (j = 0; j < 9; j++) {
        /* initially, all the threads should be running. So, %utilization is 100 for all of them */
        for (i = 0; i < smt_threads; i++) {
            util_seq[i][j] = 100;
            sprintf(tmp_str, "%d_", util_seq[i][j]);
            strcat(util_str[i], tmp_str);
        }
        j++;
        /* Get the workload type for which threads need to be removed */
        if (removal_flag == ANY) {
            picked = rand() % 2;
            if (picked == 0) {
                removal_array = *FXU_threads;
                fixed_array = *VSU_threads;
            } else {
                removal_array = *VSU_threads;
                fixed_array = *FXU_threads;
            }
        } else if (removal_flag == VSU) {
            removal_array = *VSU_threads;
            fixed_array = *FXU_threads;
        } else {
            removal_array = *FXU_threads;
            fixed_array = *VSU_threads;
        }
        /* now randomly choose the no. of threads to be removed from the specified workload type
         * and make their utilization seq. as 0.
         */
        for (k = 0; k < threads_to_remove; k++) {
            picked = rand() % (removal_array.num_threads - k);
            th_num = removal_array.thread_num[picked];
            util_seq[th_num][j] = 0;
            for (i = picked; i < (removal_array.num_threads - k - 1); i++) {
                removal_array.thread_num[i] = removal_array.thread_num[i + 1];
            }
        }
        /* now, for remianing threads, utilization sequence will be "100". */
        for (i = 0; i < removal_array.num_threads - threads_to_remove; i++) {
            th_num = removal_array.thread_num[i];
            util_seq[th_num][j] = 100;
        }
        /* Utilization sequence for the threads in fixed array will always be 100 */
        for (i = 0; i < fixed_array.num_threads; i++) {
            th_num = fixed_array.thread_num[i];
            util_seq[th_num][j] = 100;
        }
        for (i = 0; i < smt_threads; i++) {
            sprintf(tmp_str, "%d_", util_seq[i][j]);
            strcat(util_str[i], tmp_str);
        }
        j++;

        /* Above code was for creating thread re-balancing scenario, now we will have switching scenario
		 * create array having values to which switching can happen.
         */
        for (smt_array_size = 0, threads = smt_threads; threads > 1; smt_array_size++) {
            threads = threads / 2;
            smt_array[smt_array_size] = threads;
        }
        for (k=0; k < smt_threads; k++) {
            thread_num[k] = k;
        }
        /* pick a value from smt_array to which SMT switching will happen. That
         * will be next num of active threads.
         */
        picked = rand() % smt_array_size;
        next_no_act_threads = smt_array[picked];

        /* Replace the next_no_act_threads value in smt_array with cur_no_act_threads so that
         * it can be picked again.
         */
        smt_array[picked] = cur_no_act_threads;
        cur_no_act_threads = next_no_act_threads;

        /* picked randomly next_no_act_threads from thread_num array and make their % utilization
         * as 100. Rest will be 0.
         */
        for (k = 0; k < next_no_act_threads; k++) {
            picked = rand() % (smt_threads - k);
            picked_act_thread = thread_num[picked];
            util_seq[picked_act_thread][j] = 100;
            for (i = picked; i < (smt_threads - k - 1); i++) {
                thread_num[i] = thread_num[i+1];
            }
        }
        for (k = 0; k < (smt_threads - next_no_act_threads); k++) {
            th_num = thread_num[k];
            util_seq[th_num][j] = 0;
        }
        for (i = 0; i < smt_threads; i++) {
            sprintf(tmp_str, "%d_", util_seq[i][j]);
            strcat(util_str[i], tmp_str);
        }
    }
/*  for (i = 0; i < smt_threads; i++) {
       strcat(util_str[i], "}");
       printf("util_seq[%d]:", i);
       printf("%s\n", util_str[i]);
       for (j = 0; j < 9; j++) {
           printf("%3d, ", util_seq[i][j]);
       }
       printf("\n");
    } */
}

/*********************************************************************/
/* create a equaliser config file to test "thread transition" feature*/
/* in P8 (enabled with SMT 8) with mixing scenario. The utilzation   */
/* sequence for cpu/fpu workload on each hardware thread of a core   */
/* will be created such that mixing/balancing of threads happens.    */
/*********************************************************************/
void create_cfg_th_trans_mix()
{
    int i, j, k, l, removal_flag, cores_per_set[NUM_OF_SETS];
    int num_of_cores, remaining_cores, allocated_cores, allocated_cores_per_set = 0;
    int threads_to_remove, create_new_seq;
    int set_num, num_nodes, num_chips, num_cores;
    struct thread_info FXU_threads, VSU_threads;

    /* only 5 different group of sequences will be genrated. Each group will
     * be further having no. of sequence equal to no. of SMT threads enabled.
     * The available cores will be grouped together for these sequences.
     */
    num_of_cores = sys_stat.cores;
    if (num_of_cores >= NUM_OF_SETS) {
        for (i = 0; i < NUM_OF_SETS; i++) {
            cores_per_set[i] = num_of_cores / NUM_OF_SETS;
        }
    }
    remaining_cores = num_of_cores % NUM_OF_SETS;
    for (i = 0; i < remaining_cores; i++) {
        cores_per_set[i]++;
    }

    allocated_cores = 0;
    set_num = 0;
    create_new_seq = 1;

    /********************************************************************/
    /* Looked for various scenarios for varying the workload such that  */
    /* mixing/re-balancing of threads happens. Picked only 5 scenraios  */
    /* which had the max. probability of thread re-balancing as below:  */
    /* 1. 1 thread running FXU(Fixed point unit) and 7 threads running  */
    /*      VSU (vector scalar unit) workload. Removing 3 VSU, threads  */
    /*      will have 75% probablity of re-balancing.                   */
    /* 2. 3 threads running FXU workload and 5 threads running VSU      */
    /*    workload. Removing 1 thread of ANY workload type will have 50%*/
    /*    probability of thread re-balancing.                           */
    /* 3. 4 threads running FXU workload and rest 4 running VSU workload*/
    /*    Removing 2 threads of ANY workload (both of same type) will   */
    /*    have 66% probability of thread re-balancing.                  */
    /* 4. 5 threads running FXU workload and 3 threads running VSU      */
    /*    workload. Removing 1 thread of ANY workload tyoe will have 50%*/
    /*    probability of thread re-balancing.                           */
    /* 5. 7 threads running FXU workload and 1 thread running VSU       */
    /*    workload. Removing 3 FXU threads will have 75% probability of */
    /*    thread re-balancing.                                          */
    /********************************************************************/

    num_nodes = get_num_of_nodes_in_sys();
    for (i = 0; i < num_nodes; i++) {
        num_chips = get_num_of_chips_in_node(i);
        for (j = 0; j < num_chips; j++) {
            num_cores = get_num_of_cores_in_chip(i, j);
            for (k = 0; k < num_cores; k++) {
                if (create_new_seq == 1) {
                    switch (set_num) {
                        case 0:
                            FXU_threads.num_threads = 1;
                            VSU_threads.num_threads = 7;
                            removal_flag = VSU;
                            threads_to_remove = 3;
                            break;
                        case 1:
                            FXU_threads.num_threads = 3;
                            VSU_threads.num_threads = 5;
                            removal_flag = ANY;
                            threads_to_remove = 1;
                            break;
                        case 2:
                            FXU_threads.num_threads = 4;
                            VSU_threads.num_threads = 4;
                            removal_flag = ANY;
                            threads_to_remove = 2;
                            break;
                        case 3:
                            FXU_threads.num_threads = 5;
                            VSU_threads.num_threads = 3;
                            removal_flag = ANY;
                            threads_to_remove = 1;
                            break;
                        case 4:
                            FXU_threads.num_threads = 7;
                            VSU_threads.num_threads = 1;
                            removal_flag = FXU;
                            threads_to_remove = 3;
                            break;
                    }
                    create_th_mix_seq(&FXU_threads, &VSU_threads, removal_flag, threads_to_remove);
                    create_new_seq = 0;
                }
                for (l = 0; l < smt_threads; l++) {
                    fprintf(fp, "\tN%dP%dC%dT%d\t\tcpu[default.cpu.eq:0:0:UTIL_LEFT%%%s]\n", i, j, k, l, util_str[l]);
                }
                allocated_cores++;
                allocated_cores_per_set++;
                if (allocated_cores_per_set ==  cores_per_set[set_num] && allocated_cores < num_cores) {
                    allocated_cores_per_set = 0;
                    set_num++;
                    create_new_seq = 1;
                }
            }
        }
    }
}

/*********************************************************************/
/****    Function to create cfg file for 100/75/50/25 scenario    ****/
/****    create this only if SMT is 4 or more                     ****/
/*********************************************************************/
void create_cfg_100_75_50_25_util()
{
    int i, j, loop_count;

    loop_count =  smt_threads / 4;
    if (loop_count == 0) {
        loop_count = 1;
    }

    for (i = 0; i < loop_count; i++) {
        j = 4 * i;
        fprintf (fp, "\tN*P*C*T%d\t\tmem[%s:0:0:1111]\n", j, mem_eq_rulefile);
        j++;
        fprintf (fp, "\tN*P*C*T%d\t\tcpu[default.cpu.eq:0:0:1110]\n", j);
        j++;
        fprintf (fp, "\tN*P*C*T%d\t\tcpu[default.cpu.eq:0:0:1100]\n", j);
        j++;
        fprintf (fp, "\tN*P*C*T%d\t\tcpu[default.cpu.eq:0:0:1000]\n", j);
    }
}

void create_cfg_cpu_mem_50()
{
    int i;
    int num_nodes, num_chips, num_cores;

    fprintf (fp, "\tN0P0C0T0\t\tmem[mem.eq.50:0:0:1010]\n");
    fprintf (fp, "\tN0P0C0T1\t\tcpu[default.cpu.eq:0:0:0101]\n");
    for (i = 2; i < smt_threads; i++) {
        if (i % 2 == 0) {
            fprintf (fp, "\tN0P0C0T%d\t\tcpu[default.cpu.eq:0:0:1010]\n", i);
        } else {
            fprintf (fp, "\tN0P0C0T%d\t\tcpu[default.cpu.eq:0:0:0101]\n", i);
        }
    }

    num_nodes = get_num_of_nodes_in_sys();
    num_chips = get_num_of_chips_in_node(0);
    num_cores = get_num_of_cores_in_chip(0, 0);

    if (num_cores > 1) {
        for (i = 0; i < smt_threads; i++) {
            if (i % 2 == 0) {
                fprintf (fp, "\tN0P0C[1-n]T%d\tcpu[default.cpu.eq:0:0:1010]\n", i);
            } else {
                fprintf (fp, "\tN0P0C[1-n]T%d\tcpu[default.cpu.eq:0:0:0101]\n", i);
            }
        }
    }

    if (num_chips > 1) {
        for (i = 0; i < smt_threads; i++) {
            if (i % 2 == 0) {
                fprintf (fp, "\tN0P[1-n]C*T%d\tcpu[default.cpu.eq:0:0:1010]\n", i);
            } else {
                fprintf (fp, "\tN0P0C[1-n]T%d\tcpu[default.cpu.eq:0:0:0101]\n", i);
            }
        }
    }

    if (num_nodes > 1) {
        for (i = 0; i < smt_threads; i++) {
            if (i % 2 == 0) {
                 fprintf (fp, "\tN[1-n]P*C*T%d\tcpu[default.cpu.eq:0:0:1010]\n", i);
            } else {
                fprintf (fp, "\tN[1-n]P*C*T%d\tcpu[default.cpu.eq:0:0:0101]\n", i);
            }
        }
    }
}

void create_cfg_msrp_1p3m_28s_act_52s_idle()
{
    fprintf (fp, "\tN*P*C*T0\t\tpv[rule.pv:0:0:UTIL_LEFT%%35]\n");
    fprintf (fp, "\tN*P*C*T[1-3]\tddot[rule.ddot_200M:0:0:UTIL_LEFT%%35]\n");
}

void create_cfg_p8_8fd_char()
{
    fprintf (fp, "\tN*P*C*T[0-3]\tfdaxpy[rule.fdaxpy:0:0:UTIL_LEFT%%25]\n");
    fprintf (fp, "\tN*P*C*T[4-7]\tfdaxpy[rule.fdaxpy:0:0:UTIL_LEFT%%100]\n");
}

void create_cfg_p8_rdp_switch()
{
    fprintf (fp, "\tN*P*C*T[0-3]\t\trdp[rule.rdp:0:0:UTIL_LEFT%%50]\n");
    fprintf (fp, "\tN*P*C*T[4-7]\t\trdp[rule.rdp:0:0:UTIL_LEFT%%0]\n");
}

void create_cfg_p8_swicthing_cmp()
{
    fprintf (fp, "\tN*P*C*T*\t\tfdaxpy[rule.fdaxpy:0:0:UTIL_LEFT%%50]\n");
}

