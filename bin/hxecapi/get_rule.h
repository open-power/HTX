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

/****************************************************************************
*File Name:            get_rule.h
*File Description:     Header file which contains macros, typedefs and 
*                      structure definition related to get_rule_capi.c file
****************************************************************************/

#include "memcopy.h"

/* 
 * Macro definitions 
 */

/* 
 * Default stanza fields values
 */
#define DEFAULT_RULE_ID         		"MEMCOPY0"
#define DEFAULT_CMP_VALUE       		TRUE
#define DEFAULT_NUM_OPER	    		10
#define DEFAULT_BUFFER_CACHELINE_VALUE  2 
#define DEFAULT_ALIGNMENT				1
#define DEFAULT_TIMEOUT_VALUE			1

/*
 * Stanza field tags
 */
#define RULE_TAG                "RULE_ID"
#define COMPARE_TAG             "COMPARE"
#define NUM_OPER_TAG			"NUM_OPER"
#define BUFFER_CL_TAG			"BUFFER_CL"
#define ALIGNMENT_TAG			"ALIGNMENT"
#define TIMEOUT_TAG				"TIMEOUT"

/* 
 * Function declarations 
 */
static int parse_line(char []);

static int get_line(FILE *, char *, int);

static void SetDefaults(struct rule_info *);


