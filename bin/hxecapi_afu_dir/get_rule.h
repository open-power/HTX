/* @(#)72	1.1  src/htx/usr/lpp/htx/bin/hxecapi_afu_dir/get_rule.h, exer_capi, htxrhel7 10/20/16 01:11:10 */
/****************************************************************************
*File Name:            get_rule_capi.h
*File Description:     Header file which contains macros, typedefs and 
*                      structure definition related to get_rule_capi.c file
****************************************************************************/

#include "memcopy_afu_directed.h"

/* 
 * Macro definitions 
 */

/* 
 * Default stanza fields values
 */
#define DEFAULT_RULE_ID         "MEMCOPY0"
#define DEFAULT_TEST_CASE     	MEMCOPY_COMMAND_INTERRUPT 
#define DEFAULT_BUFVAL_VALUE   	CACHELINESIZE 
#define DEFAULT_CMP_VALUE       TRUE
#define DEFAULT_NUM_THREADS     0x20
#define DEFAULT_NUM_OPER	    10
#define DEFAULT_NUM_Q_ELMNTS    0x10

/*
 * Stanza field tags
 */
#define RULE_TAG                "RULE_ID"
#define TESTCASE_TAG            "TESTCASE"
#define BUFVAL_TAG              "BUFSIZE"
#define COMPARE_TAG             "COMPARE"
#define NUM_THREADS_TAG         "NUM_THREADS"
#define NUM_OPER_TAG			"NUM_OPER"
#define NUM_Q_ELMNTS_TAG		"NUM_Q_ELMNTS" 


/* 
 * Bufinc range 
 */
#define BUFINC_LOW              -1
#define BUFINC_HIGH             MAX_COPY_SIZE

/* 
 * Function declarations 
 */
static int parse_line(char []);

static int get_line(FILE *, char *, int);

static void SetDefaults(struct rule_info *);


