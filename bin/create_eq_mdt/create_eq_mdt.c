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

/*************************************************************/
/* create_eq_mdt binary takes equaliser CFG file as a input  */
/* and generates corresponding mdt whose name is also given  */
/* as an argument.                                           */
/*************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include "htxsyscfg64.h"


#define NUMERIC       0
#define STRING        1

#define MAX_DEV_IN_ROW      10
#define MAX_DEV_INPUT       4
#define MAX_LINE_SIZE       1024
#define DEFAULT_PATTERN_LENGTH  20

struct dev_exer_map {
    char dev[16];
    char exer[64];
    char adapt_desc[16];
    char dev_desc[24];
};

struct dev_info {
    char dev_name[32];
    char rule_file_name[64];
    int start_delay;
    int runtime;
    char cpu_utilization[128];
};

struct dev_exer_map map_array[] = {{"mem", "hxemem64", "64bit", "memory"},
                               {"fpu", "hxefpu64", "core", "floating_point"},
                               {"cpu", "hxecpu", "processor", "processor"},
                               {"cache", "hxecache", "", "Processor_cache"},
                               {"tdp", "hxeewm", "", "energy workload"},
                               {"rdp", "hxeewm", "", "energy workload"},
                               {"adds", "hxeewm", "", "energy workload"},
                               {"divds", "hxeewm", "", "energy workload"},
                               {"fdaxpy", "hxeewm", "", "energy workload"},
                               {"mdaxpy", "hxeewm", "", "energy workload"},
                               {"daxpy", "hxeewm", "", "energy workload"},
                               {"mwr", "hxeewm", "", "energy workload"},
                               {"mrd", "hxeewm", "", "energy workload"},
                               {"triad", "hxeewm", "", "energy workload"},
                               {"ddot", "hxeewm", "", "energy workload"},
                               {"pv", "hxeewm", "", "energy workload"},
                               {"nvidia", "hxenvidia", "NVIDIA_GPU", "Graphics_Device"},
                               {"disk_", "hxestorage", "storage", " Storage Device"},
                               {"net_", "hxecom", "network", "Network Device"},
                               {"fabc", "hxefabricbus", "", "chip to chip"},
                               {"fabn", "hxefabricbus", "", "node to node"},
                               {"sctu", "hxesctu", "cache coherency", "coherence_test"}
                              };
int line_num = 0;
int pvr, num_devs = 0, net_configured = 0;
char scripts_dir[128], mdt_dir[128], mdt_file_name[256];
static int num_mem_instances = 0;

void create_default_stanza(FILE *fp, char *eq_file);
int create_dev_stanza (FILE *fp, char *resource_str, struct dev_info *dev_input);
int get_line(char line[], int lim, FILE *fp);
void create_string_entry(FILE *fp, char *param_name, char *param_value, char *desc);
void create_numeric_entry(FILE *fp, char *param_name, char *param_value, char *desc);

int main(int argc, char **argv)
{
    int rc, i, j, err_no;
    char eq_file_name[256], mdt_file_name[256];
    char line[MAX_LINE_SIZE], tmp_str[128], resource_str[64];
    struct dev_info dev_input[MAX_DEV_IN_ROW];
    char *dev_info_ptr[MAX_DEV_IN_ROW], *dev_ptr[MAX_DEV_INPUT], keywd[32];
    char *ptr, *resource_ptr;

    FILE *eq_fp, *mdt_fp;

    if (argc != 3) {
        printf ("Usage error: below is the syntax to call create_eq_mdt\n");
        printf ("create_eq_mdt <cfg_file> <mdt file>\n");
        printf ("NOTE: You need to give absolute path for cfg and mdt file\n");
        exit(1);
    }

    pvr = get_cpu_version() >> 16;

    strcpy(eq_file_name, argv[1]);
    strcpy(mdt_file_name, argv[2]);

    eq_fp = fopen(eq_file_name, "r");
    if (eq_fp == NULL) {
        err_no = errno;
        printf("Error opening the equaliser file %s. errno: %d\n", eq_file_name, err_no);
        exit(1);
    }

    mdt_fp = fopen(mdt_file_name, "w");
    if (mdt_fp == NULL) {
        err_no = errno;
        printf("Error opening the mdt file %s. errno: %d\n", mdt_file_name, err_no);
        exit(1);
    }

    rc = init_syscfg_with_malloc();
    if (rc == -1) {
        printf("Could not collect system config detail. Exiting...\n");
        exit(1);
    }
    ptr = getenv("HTXSCRIPTS");
    if (ptr != NULL) {
         strcpy(scripts_dir, ptr);
    }

        ptr = getenv("HTXMDT");
    if (ptr != NULL) {
         strcpy(mdt_dir, ptr);
    }

    create_default_stanza(mdt_fp, eq_file_name);
    while (1) {
        line_num++;
        rc = get_line(line, MAX_LINE_SIZE, eq_fp);  /* gets one line at a time from config file  */
        if (rc == 0 || rc == 1) {  /* Line is either blank or a comment */
            continue;
        }
        else if (rc == -1) {  /* Error in reading file  */
            fclose(eq_fp);
            return(rc);
        }
        else if (rc == -2) {  /* End of file  */
            break;
        }
        else {  /* got some good data  */
            /* We need to get only device entries. otherwise, skip rest of the
             * code and get next line.
             */
            sscanf(line,"%s", keywd);
            if ((strcmp(keywd, "time_quantum") == 0) ||
                (strcmp(keywd, "startup_time_delay") == 0) ||
                (strcmp(keywd, "log_duration") == 0) ||
                (strcmp(keywd, "offline_cpu") == 0) ||
                (strcmp(keywd, "cpu_util") == 0) ||
                (strcmp(keywd, "pattern_length") == 0)) {
                continue;
           } else {
                num_devs = 0;
                if ((resource_ptr = strtok(line," \t")) != NULL) {
                    sscanf(resource_ptr, "%s", resource_str);
                    while ((dev_info_ptr[num_devs] = strtok(NULL, " ,\t\n")) != NULL) {
                        num_devs++;
                    }
                    for (i = 0; i < num_devs; i++) {
                        strcpy(tmp_str, dev_info_ptr[i]);
                        if (strcasecmp(tmp_str, "sleep") == 0) {
                            continue;
                        }
                        dev_ptr[0] = strtok(tmp_str, "[");
                        sscanf(dev_ptr[0], "%s", dev_input[i].dev_name);
                        for (j = 0; j < MAX_DEV_INPUT; j++) {
                            if ((dev_ptr[j] = strtok(NULL, ":]")) == NULL) {
                                printf("Improper data provided in line num %d.\n%s Exiting\n", line_num, line);
                                exit(1);
                            }
                        }
                        sscanf(dev_ptr[0], "%s", dev_input[i].rule_file_name);
                        sscanf(dev_ptr[1], "%d", &dev_input[i].start_delay);
                        sscanf(dev_ptr[2], "%d", &dev_input[i].runtime);
                        sscanf(dev_ptr[3], "%s", dev_input[i].cpu_utilization);
                    }
                    create_dev_stanza(mdt_fp, resource_str, dev_input);
                }
            }
        }
    }
    fclose(eq_fp);
    fclose(mdt_fp);
    if (net_configured == 1) {
        sprintf(tmp_str, "%s/mdt_net %s", mdt_dir, basename(mdt_file_name));
        system (tmp_str);
        net_configured = 0;
    }
    return 0;
}

void create_device_entries(FILE *fp, char *dev_name_str, struct dev_exer_map cur_dev_map, struct dev_info cur_dev_input )
{
    char tmp_str[256];

    fprintf(fp, "%s:\n", dev_name_str);
    create_string_entry(fp, "HE_name", cur_dev_map.exer, "* Hardware Exerciser name, 14 char");
    create_string_entry(fp, "adapt_desc", cur_dev_map.adapt_desc, "* adapter description, 11 char max");
    create_string_entry(fp, "device_desc", cur_dev_map.dev_desc, "* device description, 15 char max.");
    create_string_entry(fp, "reg_rules", cur_dev_input.rule_file_name, "* reg");
    create_string_entry(fp, "cont_on_err", "NO", "* continue on error (YES/NO)");
    if ((cur_dev_input.start_delay) != 0) {
        sprintf(tmp_str, "%d", cur_dev_input.start_delay);
        create_numeric_entry(fp, "start_delay", tmp_str, "* Exerciser start delay.");
    }
    if ((cur_dev_input.runtime) != 0) {
        sprintf(tmp_str, "%d", cur_dev_input.runtime);
        create_numeric_entry(fp, "run_time", tmp_str, "* Exerciser run time.");
    }
    sprintf(tmp_str, "%s", cur_dev_input.cpu_utilization);
    create_string_entry(fp, "cpu_utilization", tmp_str, "* cpu utilization info for the exerciser");
    fprintf(fp, "\n");
}

int create_dev_stanza (FILE *fp, char *resource_str, struct dev_info *dev_input)
{
    int i, j, num_entries, len;
    int node, chip, core, cpu;
    char dev_name_str[16], rule_file_name[64], tmp_str[64], cmd_str[256];
    char filter_str[2][128], filename[64], *ptr;
    FILE *fp1, *tmp_ptr;
    struct dev_exer_map cur_dev_map;
    struct dev_info cur_dev_input;
    struct resource_filter_info filter;

    strcpy (filter_str[0], resource_str);
    num_entries = sizeof(map_array) / sizeof(struct dev_exer_map);
    for (i = 0; i < num_devs; i++) {
        for (j = 0; j < num_entries; j++) {
             if ((strcasecmp (map_array[j].dev, dev_input[i].dev_name) == 0) ||
                 (strncasecmp (dev_input[i].dev_name, "nvidia", 6) == 0) ||
                 ((strncasecmp (dev_input[i].dev_name, "disk_", 5) == 0 ) && (strncasecmp (map_array[j].dev, dev_input[i].dev_name, 5) == 0)) ||
                 ((strncasecmp (dev_input[i].dev_name, "net_", 4) == 0) && (strncasecmp (map_array[j].dev, dev_input[i].dev_name, 4) == 0))) {
                strcpy(cur_dev_map.exer, map_array[j].exer);
                strcpy(cur_dev_map.adapt_desc, map_array[j].adapt_desc);
                strcpy(cur_dev_map.dev_desc, map_array[j].dev_desc);
                break;
            }
        }

        if (j == num_entries) {
            printf ("dev %s is not supported under equaliser.\n", dev_input[i].dev_name);
            exit(1);
        }

        /* If device name is mem, we need not to parse the resource str. Resource str will be updated in
           the rulefile used by the device and exerciser will parse that string. "mem" device name will
           also be updated to distinguish between multiple mem device entries.
         */
        if ((pvr >= PV_POWER9_NIMBUS) && (strcasecmp(dev_input[i].dev_name, "mem") == 0)) {
			/* create new rulefile and update the resource str there */
            sprintf(dev_name_str, "%s%d", dev_input[i].dev_name, num_mem_instances);
            /* sprintf(dev_name_str, "%s%c", dev_input[i].dev_name, mem_instance_str[num_mem_instances]); */
            sprintf(dev_input[i].rule_file_name, "default.eq.%s", dev_name_str);
            num_mem_instances++;
            strcpy(rule_file_name, cur_dev_map.exer);
            strcat(rule_file_name, "/");
            strcat(rule_file_name, dev_input[i].rule_file_name);
            memcpy(&cur_dev_input, &dev_input[i], sizeof(struct dev_info));
            strcpy(cur_dev_input.rule_file_name, rule_file_name);
            create_device_entries(fp, dev_name_str, cur_dev_map, cur_dev_input);
        } else {
            strcpy(rule_file_name, cur_dev_map.exer);
            strcat(rule_file_name, "/");
            strcat(rule_file_name, dev_input[i].rule_file_name);
            memcpy(&cur_dev_input, &dev_input[i], sizeof(struct dev_info));
            strcpy(cur_dev_input.rule_file_name, rule_file_name);
            if (filter_str[0][0] == '*') {
                if (strncasecmp (dev_input[i].dev_name, "disk_", 5) == 0) {
                    strcpy(tmp_str, &dev_input[i].dev_name[5]);
                    if (tmp_str[0] == '*') {
                    #ifdef __HTX_LINUX__
                        strcpy(filename, "/tmp/rawpart");
                        fp1 = fopen (filename, "r");
                        if (fp1 != NULL) {
                            while ((fscanf (fp1, "%s\n", tmp_str)) != EOF) {
                                strcpy(dev_name_str, tmp_str);
                                create_device_entries(fp, dev_name_str, cur_dev_map, cur_dev_input);
                            }
                            fclose(fp1);
                        }
                        strcpy(filename, "/tmp/mpath_parts");
                        fp1 = fopen (filename, "r");
                        if (fp1 != NULL) {
                            while ((fscanf (fp1, "%s\n", tmp_str)) != EOF) {
                                strcpy(dev_name_str, tmp_str);
                                create_device_entries(fp, dev_name_str, cur_dev_map, cur_dev_input);
                            }
                            fclose(fp1);
                        }
                    #else
                        sprintf(cmd_str, "cat %s/mdt.bu | grep rhdisk | grep :", mdt_dir);
                        tmp_ptr = popen (cmd_str, "r");
                        if (tmp_ptr == NULL) {
                            printf("Error for popen for disk devices in create_eq_mdt. Exiting...");
                            exit(1);
                        }
                        while (fgets(tmp_str, 32, tmp_ptr) != NULL) {
                            ptr = strtok(tmp_str, ":");
                            strcpy(dev_name_str, ptr);
                            create_device_entries(fp, dev_name_str, cur_dev_map, cur_dev_input);
                        }
                        pclose(tmp_ptr);
                    #endif
                    } else {
                        sprintf(cmd_str, "cat %s/mdt.bu | grep %s | grep :", mdt_dir, tmp_str);
                        tmp_ptr = popen (cmd_str, "r");
                        if (tmp_ptr == NULL) {
                            printf("Error for popen in create_eq_mdt. Exiting...");
                            exit(1);
                        }
                        if (fgets(tmp_str, 32, tmp_ptr) != NULL) {
                            ptr = strtok(tmp_str, ":");
                            strcpy(dev_name_str, ptr);
                            create_device_entries(fp, dev_name_str, cur_dev_map, cur_dev_input);
                        }
                        pclose(tmp_ptr);
                    }
                } else if (strncasecmp (dev_input[i].dev_name, "net", 3) == 0) {
                    system ("build_net help y y");
                    net_configured = 1;
                } else if (strncasecmp (dev_input[i].dev_name, "fab", 3)  == 0) {
                    strcpy(dev_name_str, dev_input[i].dev_name);
                    create_device_entries(fp, dev_name_str, cur_dev_map, cur_dev_input);
                } else {
                    len = strlen (dev_input[i].dev_name);
                    if (dev_input[i].dev_name[len - 1] == '*') {
                        strncpy (tmp_str, dev_input[i].dev_name, len - 1); /* Will remove '*' from end */
                    } else {
                        strcpy (tmp_str, dev_input[i].dev_name);
                    }
                    sprintf(cmd_str, "cat %s/mdt.bu | grep %s | grep :", mdt_dir, tmp_str);
                    tmp_ptr = popen (cmd_str, "r");
                    if (tmp_ptr == NULL) {
                        printf("Error for popen in create_eq_mdt. Exiting...");
                        exit(1);
                    }
                    while (fgets(tmp_str, 32, tmp_ptr) != NULL) {
                        ptr = strtok(tmp_str, ":");
                        strcpy(dev_name_str, ptr);
                        create_device_entries(fp, dev_name_str, cur_dev_map, cur_dev_input);
                    }
                }
            } else {
                parse_filter(filter_str, &filter, 1);
                for (node = 0; node < filter.num_nodes; node++) {
                    if (filter.node[node].node_num == -1) {
                        continue;
                    }
                    for (chip = 0; chip < filter.node[node].num_chips; chip++) {
                        if (filter.node[node].chip[chip].chip_num == -1) {
                            continue;
                        }
                        for (core = 0; core < filter.node[node].chip[chip].num_cores; core++) {
                            if (filter.node[node].chip[chip].core[core].core_num == -1) {
                                continue;
                            }
                            for (cpu = 0; cpu < filter.node[node].chip[chip].core[core].num_procs; cpu++) {
                                if (filter.node[node].chip[chip].core[core].lprocs[cpu] == -1 ) {
                                    continue;
                                }
                                sprintf(dev_name_str, "%s%d", dev_input[i].dev_name, filter.node[node].chip[chip].core[core].lprocs[cpu]);
                                create_device_entries(fp, dev_name_str, cur_dev_map, cur_dev_input);
                            }
                        }
                    }
                }
		    }
        }
    }
    return 0;
}

void create_string_entry(FILE *fp, char *param_name, char *param_value, char *desc)
{
    int start_index;
    char comment[128];
    char tmp[2] = "";

    start_index = 60 - strlen(param_name) - strlen(param_value);
    sprintf(comment, "%*s", start_index, tmp);
    fprintf(fp, "\t%s = \"%s\" %s%s\n", param_name, param_value, comment, desc);
}

void create_numeric_entry(FILE *fp, char *param_name, char *param_value, char *desc)
{
    int start_index;
    char comment[128], tmp[2] = "";

    start_index = 62 - strlen(param_name) - strlen(param_value);
    sprintf(comment, "%*s", start_index, tmp);
    fprintf(fp, "\t%s = %s %s%s\n", param_name, param_value, comment, desc);
}

int get_string_value(char *eq_file, char *string, char type, char *str_val)
{
    int val = 0;
    char str[256];
    FILE *tmp_fp;

    sprintf(str, "cat %s | grep %s | grep -v '#' | awk '{ print $NF}'", eq_file, string);
    tmp_fp = popen (str, "r");
    if (tmp_fp == NULL) {
        printf("Error for popen in create_eq_mdt. Exiting...");
        exit(1);
    }
    if (fgets(str_val,128,tmp_fp) != NULL && type == NUMERIC) {
        sscanf(str_val, "%d", &val);
    } else {
		str_val[strlen(str_val) -1] = '\0';
	}
    pclose(tmp_fp);
    return val;
}

void create_default_stanza(FILE *fp, char *eq_file)
{
    char str[128], *tmp_str, cpu_util[64] = "";
    int log_duration, time_quantum, pattern_length;
    int offline_cpu = 0, startup_time_delay;

    time_quantum = get_string_value(eq_file, "time_quantum", NUMERIC, str);
    startup_time_delay = get_string_value(eq_file, "startup_time_delay", NUMERIC, str);
    log_duration = get_string_value(eq_file, "log_duration", NUMERIC, str);
    pattern_length = get_string_value(eq_file, "pattern_length", NUMERIC, str);
    if (pattern_length == 0) {
        pattern_length = DEFAULT_PATTERN_LENGTH;
    }
    get_string_value(eq_file, "cpu_util", STRING, cpu_util);
    offline_cpu = get_string_value(eq_file, "offline_cpu", NUMERIC, str);

    fprintf(fp, "default:\n");
    create_string_entry(fp, "HE_name", "", "* Hardware Exerciser name, 14 char");
    create_string_entry(fp, "adapt_desc", "", "* adapter description, 11 char max");
    create_string_entry(fp, "device_desc", "", "* Hardware Exerciser name, 14 char");
    create_string_entry(fp, "reg_rules", "", "* reg rules");
    create_string_entry(fp, "emc_rules", "", "* emc rules");
    create_numeric_entry(fp, "dma_chan", "0", "* DMA channel number");
    create_numeric_entry(fp, "idle_time", "0", "* idle time (secs)");
    create_numeric_entry(fp, "intrpt_lev", "0", "* interrupt level");
    create_numeric_entry(fp, "load_seq", "32768", "* load sequence (1 - 65535)");
    create_numeric_entry(fp, "max_run_tm", "7200", "* max run time (secs)");
    create_string_entry(fp, "port", "0", "* port number");
    create_numeric_entry(fp, "priority", "17", "* priority (1=highest to 19=lowest");
    create_string_entry(fp, "slot", "0", "* slot number");
    create_string_entry(fp, "max_cycles", "0", "* max cycles");
    create_numeric_entry(fp, "hft", "0", "* hft number");
    create_string_entry(fp, "cont_on_err", "NO", "* continue on error (YES/NO)");
    create_string_entry(fp, "halt_level", "1", "* level <= which HE halts");
    create_string_entry(fp, "start_halted", "n", "* exerciser halted at startup");
    create_string_entry(fp, "dup_device", "n", "* duplicate the device");
    create_string_entry(fp, "log_vpd", "y", "* Show detailed error log");
    create_numeric_entry(fp, "equaliser_flag", "1", "* Equaliser flag enabled for supervisor");
    create_numeric_entry(fp, "equaliser_debug_flag", "0", "* Equaliser debug flag for supervisor");
    sprintf(str, "%d", time_quantum);
    create_numeric_entry(fp, "eq_time_quantum", str, "* Equaliser time quantum");
    if ( offline_cpu != 0) {
        sprintf(str, "%d", offline_cpu);
        create_numeric_entry(fp, "offline_cpu", str, "* offlining the cpus is enabled");
    }
    sprintf(str, "%d", startup_time_delay);
    create_numeric_entry(fp, "eq_startup_time_delay", str, "* Equaliser startup time delay");
    sprintf(str, "%d", log_duration);
    create_numeric_entry(fp, "eq_log_duration", str, "* Equaliser log duration");
    if (strcmp (cpu_util, "") != 0) {
        create_string_entry(fp, "cpu_util", cpu_util, "* System level cpu utilization");
    }
    sprintf(str, "%d", pattern_length);
    create_numeric_entry(fp, "eq_pattern_length", str, "* Equaliser default pattern length");
    strcpy(str, eq_file);
    tmp_str = basename(str);
    create_string_entry(fp, "cfg_file", tmp_str, "* Corresponding cfg file for this mdt");
    fprintf(fp, "\n");
}

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
                printf(msg, "Error (errno = %d) in reading config file. exiting...\n", err_no);
                exit(1);
        }
        else {
                rc = parse_line(line);
                return rc;
        }
}


