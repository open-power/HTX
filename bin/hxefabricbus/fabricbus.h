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
/* @(#)06	1.12  src/htx/usr/lpp/htx/bin/hxefabricbus/fabricbus.h, exer_ablink, htxubuntu 1/7/16 00:26:55 */


#include <pthread.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#ifndef __HTX_LINUX__
	#include <sys/systemcfg.h>
	#include <sys/thread.h>
	#include <sys/vminfo.h>
	#include <sys/processor.h>
	#include <sys/rset.h>
#endif
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include "hxihtx64.h"
#include "hxihtxmp.h"
#include <strings.h>
#include <htxsyscfg64.h> 
#ifdef _DR_HTX_
    #include <sys/types.h>
    #include <sys/dr.h>
    #include <sys/procfs.h>
#endif

#ifdef DEBUG
    #define DEBUGON printf
    #define FPRINTF(a,x,...)
#else
    #define DEBUGON
    #define DEBUGOFF(x, ...)
    #define FPRINTF(a,x,...)
#endif
#define  B     1
#define  K     1024*B
#define  M     1024*K

#define MAX_TC 10
#define MAX_STANZA_NAME 100
#define MAX_STANZA_PATTERNS 9
#define MIN_PATTERN_SIZE    8
#define MAX_PATTERN_SIZE    4096
#define PG_SZ_16M 		(16*M)
#define ADDR_PAT_SIGNATURE  0x414444525F504154  /* Hex equiv of "ADDR_PAT" */
#define RAND_PAT_SIGNATURE  0x52414E445F504154  /* Hex equiv of "RAND_PAT" */
#define SMT_ENABLED 0x2
#define BIT_PATTERN_WIDTH   8   /* 8 BYTES wide */
#define NO_CPU_DEFINED -1
#define HOST_CPU 0
#define DEST_CPU 1

#define BIND_THE_PROCESS  0
#define BIND_THE_THREAD  1
#define UNBIND_THE_ENTITY -1
#define PVR_POWER6               0x3e
#define P6_L3CACHE_SIZE         32*M
#define P6_MAX_CORES_PER_CHIP		2
#define P6_MAX_SMT_STATUS			2

#define PVR_POWER7               0x3f
#define P7_L3CACHE_SIZE         4*M
#define P7_MAX_CORES_PER_CHIP 	8

#define PVR_POWER7PLUS 			0x4a
#define P7PLUS_L3CACHE_SIZE     10*M    
#define P7PLUS_MAX_CORES_PER_CHIP P7_MAX_CORES_PER_CHIP 

#define PVR_POWER8_MURANO        0x4b
#define P8_L3CACHE_SIZE          8*M

#define PVR_POWER8_VENICE 		 0x4d
#define P8_L3CACHE_SIZE   		 8*M

#define P8_MURANO_MAX_CORES_PER_CHIP 6 
#define P8_VENICE_MAX_CORES_PER_CHIP 12

#define PVR_POWERP8P_GARRISION 0x4c 

#define PVR_POWER9_NIMBUS        0x4e  /* P9 */
#define PVR_POWER9_CUMULUS       0x4f  /* P9 */
#define P9_NIMBUS_MAX_CORES_PER_CHIP     12 
#define P9_CUMULUS_MAX_CORES_PER_CHIP    24
#define P9_L3CACHE_SIZE          10*M
#define L3CACHE_LINESIZE       128
#define MAX_NODES   8
#define MAX_CHIPS_PER_NODE 8
#define MAX_CPUS_PER_CHIP 96 
#define MAX_SMT_THREAD_PER_CHIP MAX_CPUS_PER_CHIP 

#define MAX_CPUS    (MAX_NODES * MAX_CHIPS_PER_NODE * MAX_CPUS_PER_CHIP )
#define MAX_CPUS_PER_NODE (MAX_CHIPS_PER_NODE * MAX_CPUS_PER_CHIP)
#define MAX_CHIPS (MAX_NODES * MAX_CHIPS_PER_NODE)


enum pattern_types { PATTERN_FIXED=0, PATTERN_ADDRESS, PATTERN_RANDOM };

/*
 * Global data structures
 */

typedef struct {
    char  pattern_name[MAX_PATTERN_SIZE];
    char actual_pattern[MAX_PATTERN_SIZE];
    unsigned int pattern_type;
    unsigned int pattern_size;
} PATTERN;

typedef struct {
    PATTERN pattern[MAX_STANZA_PATTERNS];
    unsigned int num_patterns;
} FIXED_PATTERNS;

typedef struct {
	unsigned int host_cpu;									/* Host cpu on which below mask's are to applied */  
	unsigned int dest_cpu; 									/* Dest cpu on which below mask's are to applied */
	unsigned long long and_mask[4];							/* user defined and mask for random_numbers generated */
	unsigned int and_mask_len;								/* Len of and mask in terms of 8 bytes */
	unsigned long long or_mask[4];							/* user defined or mask for random_number generated */
	unsigned int or_mask_len;								/* Len of or mask in terms of 8 bytes */ 
} MASK_STRUCT; 

struct rf_stanza {
    char rule_id[200];   									/* Test case name */
    int compare;    										/* Yes - 1, No - 0 */
    int memory_allocation ; 								/* Memory allocation type Possible values FOR A-B 1,2,3 & X-Y value 4*/
    int crash_on_misc;  									/* 0 - No, 1 - Yes, i.e. Crash to KDB on Miscompare */
    int num_oper;   										/* 0 - infinite, +ve integer */
    unsigned long long seed ;      							/* 0 -default . Used for debug */
    int memory_configure ; 									/* 0 - RAD calls used 1 - user defined memory mapping */
	int mask_configure; 									/* Allow usage of link based And, Or masks			  */
	unsigned int threads_per_node;							/* If mem_alloc -2 then number of threads to create per node */
	FIXED_PATTERNS fixed_pattern;							/* fixed Patterns  specified through rules file */
    int wrc_data_pattern_iterations;         				/* Number of times each pattern should be repeated*/
    int wrc_iterations;       								/* WRC algo iterations 	*/
    int copy_iterations;      								/* Copy algo iterations */
    int add_iterations;       								/* Add algo iterations  */
    int daxpy_iterations;     								/* Daxpy algo iterations */
    int daxpy_scalar;         								/* Daxpy scalar value */
	unsigned int cec_nodes;									/* user defined number of nodes */
	unsigned int chips_per_node; 							/* user defined number of chips per node */
	unsigned int cores_per_chip; 							/* user defined cores per chip */
	MASK_STRUCT mask_strct;									/* If User specifies global masks 	*/ 
	unsigned int randbuf_size;								/* Random buffer size specified by user */


};


typedef struct {
	unsigned int num_procs;
	unsigned int lprocs[MAX_CPUS_PER_CHIP];
} CHIP_TYP;

typedef struct {
	unsigned int num_chips;
	CHIP_TYP chip[MAX_CHIPS_PER_NODE];
} NODE_TYP;

typedef struct {
	unsigned int num_nodes;
	NODE_TYP node[MAX_NODES];
} SYS_CONF_TYP;

typedef struct {
	int shm_id;
	char * seg_addr;
	int memory_set_size;
} MEMORY_SET;

struct thread_context {
    pthread_t tid;
    pthread_attr_t  thread_attrs;
	unsigned int tc_id;
	unsigned int bind_to_cpu;
	unsigned long long * random_no_buffer;
	unsigned int num_oper;
	char * seg_addr;
	unsigned long long * rand_buffer;
	unsigned int memory_set_size;
	unsigned int compare;
	unsigned int crash_on_misc ;
	unsigned int wrc_iterations;
	unsigned int add_iterations;
	unsigned int wrc_data_pattern_iterations;
	unsigned int copy_iterations;
	unsigned int daxpy_iterations ;
	unsigned int daxpy_scalar;
	unsigned int randbuf_size; 
    unsigned long long and_mask[4];                         
    unsigned int and_mask_len;
    unsigned long long or_mask[4]; 
    unsigned int or_mask_len;     
	struct htx_data th_htx_d;
	FIXED_PATTERNS fixed_pattern;
};

/* Function declarations */
void SIGTERM_hdl (int sig);

void SIGUSR2_hdl (int sig);

#ifdef _DR_HTX_
        void DR_handler(int sig, int code, struct sigcontext *scp);
#endif

void run_test_stages(void *);
int get_line(char s[], FILE *fp); 
int
read_hardware_config(SYS_CONF_TYP * scfg, unsigned int tot_cpus, unsigned int pvr);
long long int
GetMemoryDetails(char string[], struct htx_data * htx_d);


