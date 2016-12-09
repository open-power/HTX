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
#include <stddef.h>
#include <stdlib.h>
#include <string.h> 
#include <libgen.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <ctype.h>
#include <hxihtx64.h>
#include <errno.h> 
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <net/if.h>



/*****************************************************
 * Set of test that adapter diagnostic can support
 ****************************************************/ 

#define NUM_RETRIES 16
#define MAX_TEST 16
#define MAX_STRING_SIZE 256
#define MAX_TC   16

#define ETHERNET 				1 
#define ROCE	 				2
#define INFINIBAND 				3
#define IB_BER_TEST 			7 
#define IB_DMA_TEST 			8 
#define IB_HCA_SELFTEST 		9

#define HYDEPARK 				1
#define GLACIERPARK 			2
#define TRAVIS_3EN				3
#define	SHINER					4
#define GLACIERPARK_1PORT	 	5

#define MAX_RETRY	0x10

#define NVRAM_MASK 	0x01
#define LINK_MASK 	0x02
#define REGISTER_MASK 	0x04 
#define MEMORY_MASK 	0x08
#define MAC_MASK 	0x10 
#define PHY_MASK 	0x20
#define INT_LOOP_MASK 	0x40
#define EXT_MASK 	0x80
#define INTERRUPT_MASK 	0x100
#define SPEED_MASK 	0x200
#define LOOPBACK_MASK 	0x400

#ifdef DEBUGON
	#define DEBUG TRUE 
#else
    #define DEBUG FALSE
#endif

/****************************************************
 * Global variable declarations ... 
 ***************************************************/ 
unsigned int device_type = ETHERNET; 
unsigned int device_name = SHINER; 
/********************************************************
 * If Infininband Device, how many ports this adapter has 
 ********************************************************/
unsigned int num_ports = 1; 
/****************************************************
 * Sturcutre defintions ...... 
 * *************************************************/

typedef struct { 
	unsigned long long pass; 
	unsigned long long fail; 
	unsigned long long total; 
} status; 

struct self_test { 

	status  nvram_test; 					/* driver can read from adapter NVRAM successfully */  
	status  link_test; 						/* driver can train the links					   */ 
	status  register_test; 					/* driver can read commonly used adapter registers */ 
	status  memory_test; 					/* driver can read/write to config space of register */ 
	status  mac_loopback; 					/* Mac loop back test was done */ 
	status  phy_loopback; 					/* PHY loop back test was done */ 
	status 	int_loopback;					/* Internal loop back test 		*/ 
	status  ext_loopback; 					/* DMA r/w test */ 
	status  interrupt_test; 				/* Interrupt test with NOP 	   */ 
	status  speed_test; 
	status loopback_test;				

}; 

struct rule_info { 

	char rule_id[MAX_STRING_SIZE]; 
	int test; 
	int num_oper; 
	int sleep; 
	int threshold; 
	int mask;
}; 

/****************************************************
 * Function declarations .. 
 * *************************************************/
struct ethtool_gstrings * get_stringset(enum ethtool_stringset set_id, size_t drvinfo_offset, int fd, struct htx_data * htx_d);  

int issue_ioctl(int fd, void  * command , struct htx_data * htx_d); 

char * strupr(char * str); 

int update_result(struct ethtool_test *test, int num_tests,  char supported_test[][MSG_TEXT_SIZE], int error_mask, struct self_test * self_test, struct htx_data * htx_d);

int dump_test(struct ethtool_test *test, struct ethtool_gstrings *strings);

int rf_read_rules(const char rf_name[], struct rule_info rf_info[], unsigned int * num_stanzas, struct htx_data * htx_d); 

void SIGTERM_hdl (int sig); 

int  get_cmd_result(char cmd[], char result[], int size, struct htx_data * htx_d); 

int get_cmd_rc(char cmd[], struct htx_data * htx_d) ; 

char * trim(char * str); 
