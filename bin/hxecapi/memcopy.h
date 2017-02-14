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

#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/errno.h>
#include <string.h>
#include <poll.h>

#include <libcxl.h>
#include <errno.h>
#include <getopt.h>
#include <linux/types.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>

#ifdef _64BIT_
#include "hxihtx64.h"
#else
#include "hxihtx.h"
#endif

#ifdef __HTX_LINUX__
#include <sys/stat.h>
#endif

#define DEVICE "/dev/cxl/afu0.0d"
#define CACHELINE_BYTES 128
#define AFU_MMIO_REG_SIZE 0x4000000
#define MMIO_STAT_CTL_REG_ADDR 0x0000000
#define MMIO_GO_ADDR      0x0000000
#define MMIO_STOP_ADDR    0x0000008
#define MMIO_DMY1_ADDR    0x0000010
#define MMIO_TRACE_ADDR   0x3FFFFF8

#define CACHE_LINE_SIZE 	128 

#ifdef DEBUGON 
	#define DEBUG  1 
#else 
	#define DEBUG  0 
#endif 

/*
 * Rule file path length
 */
#define PATH_SIZE 				MSG_TEXT_SIZE

/*
 * Rule ID String length
 */
#define MAX_STRING              MSG_TEXT_SIZE

/* 
 * Dumping Miscompare Information 
 */ 
#define MAX_MISCOMPARES 		8
#define MAX_MSG_DUMP 			0x10

/* 
 * Max stanza count 
*/
#define MAX_STANZA            	32 

/*
 * Each thread needs these arguments, clubbed together in same structure... 
 */ 
struct thno_htxd { 
	struct htx_data htx_d; 
	uint32_t thread_no;  
	uint32_t helper_thread_no;  
}; 

/*
 * Rule info strcuture used to store parsed rule file data
 */
struct rule_info {  
    char 		rule_id[MAX_STRING];  
    uint32_t 	compare:1;  
	uint32_t 	num_oper;
    uint32_t 	buffer_cl;  
    uint32_t 	timeout;  
	uint32_t    aligned;
};
struct rule_info rule_data[MAX_STANZA];

int get_rule_capi(struct htx_data * , char [], uint32_t *);

void print_rule_file_data(int num_stanz);
