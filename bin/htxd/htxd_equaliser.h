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
/* @(#)45	1.1  src/htx/usr/lpp/htx/bin/htxd/htxd_equaliser.h, htxd, htxubuntu 7/17/13 08:48:07 */

#ifndef HTXD__EQUALISER__HEADER
#define HTXD__EQUALISER__HEADER

#include "htxsyscfg64.h"

#define UTILIZATION_QUEUE_LENGTH     20
#define MAX_UTIL_SEQUENCE_LENGTH     10
#define MAX_TESTS_PER_CPU            20

#define LOGFILE       "eq_status"
#define LOGFILE_SAVE  "eq_status_save"

#define UTIL_LEFT	    0
#define UTIL_RIGHT	    1
#define UTIL_RANDOM  	2
#define UTIL_PATTERN	3

typedef char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

struct CPU {
    int lcpu;
    int pcpu;
    int num_tests_configured;
    int exer_info_index[MAX_TESTS_PER_CPU];
};

struct CORE {
    int num_cpus;
    int logical_core_num;
    struct CPU cpus[MAX_CPUS_PER_CORE];
};

struct CHIP {
    int num_cores;
    int logical_chip_num;
    struct CORE cores[MAX_CORES_PER_CHIP];
};

struct NODE {
    int num_chips;
    int logical_node_num;
    struct CHIP chips[MAX_CHIPS_PER_NODE];
};

struct SYS {
    int num_nodes;
    struct NODE nodes[MAX_NODES];
};

struct run_time_data {
    uint32 target_utilization;
    uint32 current_seq_step;
    uint8 current_step;
};
typedef struct run_time_data run_time_data;

struct config_parameters {
    uint32      util_pattern;        /* UTIL_LEFT/UTIL_RIGHT/UTIL_RANDOM/UTIL_PATTERN */
    uint32      utilization_pattern; /* Bit pattern */
    uint16      pattern_length;      /* length of bit pattern */
    uint16      sequence_length;     /* Num of %age util defined */
    uint16      utilization_sequence[MAX_UTIL_SEQUENCE_LENGTH];
};
typedef struct config_parameters config_params;

struct run_time_config_structure {
    config_params    config;
    run_time_data    data;
};
typedef struct run_time_config_structure run_time_config;

struct test_config_structure {
    uint32                      time_quantum;
    uint32                      offline_cpu;           /* Flag to make cpu offline. only supported for Linux */
    uint32                      startup_time_delay;    /* Time delay for equaliser to be effective */
    uint32                      log_duration;          /* Time duration foe which logs are collected. */
    run_time_config             *exer_config;          /* info for each exerciser running under equaliser control */
};
typedef struct test_config_structure test_config_struct;

extern void htxd_equaliser(void);

#endif
