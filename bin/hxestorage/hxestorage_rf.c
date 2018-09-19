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

/************************************************************/
/* File name - hxestorage_rf.c                              */
/* File containing all the functions definitions associated */
/* with rulefile info                                       */
/************************************************************/
#include "hxestorage_rf.h"

extern void user_msg(struct htx_data *ps, int err, int io_err_flag, int sev, char *text);

extern int sync_cache_flag, discard_enabled, randomize_sync_cache, total_BWRC_threads, max_thread_cnt, total_max_thread_cnt;
extern int turn_attention_on;
int seeds_were_saved = 0, num_sub_blks_psect = 0;
char fsync_flag ='N';
struct device_info dev_info;
#ifdef __CAPI_FLASH__
	extern int lun_type ;
	extern size_t chunk_size;
#endif
/****************************************************************************/
/*              Function to set default value of a rule                     */
/****************************************************************************/

void set_rule_defaults(struct ruleinfo *ruleptr, unsigned long long maxblk, unsigned int blksize)
{
    int i;

    bzero(ruleptr, sizeof(struct ruleinfo));

    ruleptr->num_oper.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->seek_breakup_prcnt.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->starting_block.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->direction.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->data_len.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->min_blkno.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->max_blkno.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->blk_hop.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->align.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->lba_align.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->num_mallocs.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->hotness.num_keywds = DEFAULT_NUM_KEYWDS;
    ruleptr->loop_on_offset.num_keywds = DEFAULT_NUM_KEYWDS;

    strcpy(ruleptr->rule_id, " ");
    ruleptr->num_oper_defined = 0;
    ruleptr->num_threads = DEFAULT_NUM_THREADS;
    ruleptr->num_BWRC_threads = 0;
    ruleptr->BWRC_th_mem_index = UNDEFINED;
    ruleptr->is_only_BWRC_stanza = 'Y';
    ruleptr->num_associated_templates = 0;
    ruleptr->associativity = DEFAULT_ASSOCIATIVITY;
    ruleptr->testcase = DEFAULT_TESTCASE;
    ruleptr->mode = DEFAULT_MODE;
    ruleptr->rule_option = DEFAULT_RULE_OPTION;
    ruleptr->section = NO;
    ruleptr->run_reread = 'Y';
    ruleptr->repeat_pos = 0;
    ruleptr->repeat_neg = 0;
    ruleptr->sleep = 0;
    ruleptr->num_cache_threads = DEFAULT_CACHE_THREADS;
    ruleptr->num_async_io = DEFAULT_AIO_REQ_QUEUE_DEPTH;
#ifdef __CAPI_FLASH__
	ruleptr->open_flag = UNDEFINED;
#endif
    for (i=0; i < MAX_INPUT_PARAMS; i++) {
        strcpy(ruleptr->oper[i], "");
        strcpy(ruleptr->pattern_id[i], "#003");
        ruleptr->num_oper.value[i] = DEFAULT_NUM_OPER;
        ruleptr->seek_breakup_prcnt.value[i] = UNDEFINED;
        ruleptr->starting_block.value[i] = DEFAULT_STARTING_BLOCK;
        ruleptr->direction.value[i] = DEFAULT_DIRECTION;
        ruleptr->min_blkno.value[i] = DEFAULT_MIN_BLKNO;
        ruleptr->max_blkno.value[i] = maxblk;
        ruleptr->blk_hop.value[i] = DEFAULT_INCREMENT;
        ruleptr->data_len.size[i].min_len = UNDEFINED;
        ruleptr->data_len.size[i].max_len = UNDEFINED;
        ruleptr->data_len.size[i].increment = 0;
        #ifdef __HTX_LINUX__
            ruleptr->align.value[i] = blksize;
        #else
            ruleptr->align.value[i] = DEFAULT_ALIGN;
        #endif
        ruleptr->lba_align.value[i] = DEFAULT_LBA_ALIGN;
        ruleptr->num_mallocs.value[i] = DEFAULT_NUM_MALLOCS;
        ruleptr->hotness.value[i] = DEFAULT_HOTNESS;
        ruleptr->loop_on_offset.value[i] = DEFAULT_LOOP_ON_OFFSET;
    }
}

/****************************************************************************/
/*               Function to set default value of a template                */
/****************************************************************************/
void set_template_defaults(template *tmplt_ptr)
{
    strcpy(tmplt_ptr->template_id, " ");
    tmplt_ptr->data_len.min_len = UNDEFINED;
    tmplt_ptr->data_len.max_len = UNDEFINED;
    tmplt_ptr->data_len.increment = 0;
    tmplt_ptr->seek_breakup_prcnt = UNDEFINED;
    strcpy(tmplt_ptr->oper, "R");
}

/************************************************************************/
/*           Function to get one line at a time of a rule stanza        */
/************************************************************************/
/*
 * rc = 0  indicates comment in a rule file
 * rc = 1  indicates only '\n' (newline char) on the line, i.e. change in stanza
 * rc > 1  more characters, i.e. go ahead and check for valid parameter name.
 * rc = -1 error while reading file.
 * rc = -2 End of File.
 */

int get_line(char *str, FILE *fp)
{

    char *s, str1[MAX_RULE_LINE_SZ];
    unsigned int str_len;
    int i;

    if (fgets(str, MAX_RULE_LINE_SZ, fp) == NULL) {
        if(feof(fp)) {
            return -2;
        }
        if (ferror(fp)) {
            return -1;
        }
    }

    strcpy(str1, str);
    if ((s = strtok(str1, " \n\t")) != NULL) {
        if (str[0] == '*') {
            return 0;
        }
        str_len = strlen(str);
        for ( i = 0; str[i] != '\n'; i++ ) {
            if ( str[i] == '=' ) {
                str[i] = ' ';
            }
        }
        return (str_len);
    } else {  /* blank line */
        return 1;
    }
}

/****************************************************************************/
/*                   Function to print stanza                               */
/****************************************************************************/
void print_stanza_input(void *current_stanza_ptr, unsigned int stanza_defined)
{
    struct ruleinfo *rule_stanza_ptr;
    template *tmplt_stanza_ptr;
    int i;

    if (stanza_defined == RULE_STANZA) {
        rule_stanza_ptr = (struct ruleinfo *) current_stanza_ptr;
        printf("-----------------------------\n");
        printf("rule_id: %s\n", rule_stanza_ptr->rule_id);
        for (i=0; i < rule_stanza_ptr->num_pattern_defined; i++) {
            printf("pattern_id: %s\n", rule_stanza_ptr->pattern_id[i]);
        }
        printf("Total num threads: %u\n", rule_stanza_ptr->num_threads);
        for(i=0; i < rule_stanza_ptr->num_associated_templates; i++) {
            tmplt_stanza_ptr = rule_stanza_ptr->tmplt[i].tmplt_ptr;
            printf("num_threads: %d, associated template: %s\n", rule_stanza_ptr->tmplt[i].num_threads, tmplt_stanza_ptr->template_id);
        }
        if (rule_stanza_ptr->num_BWRC_threads) {
            printf ("num_BWRC_threads: %d\n", rule_stanza_ptr->num_BWRC_threads);
        }
        printf ("BWRC_th_mem_index: %d\n", rule_stanza_ptr->BWRC_th_mem_index);

        printf("is_only_BWRC_stanza flag: %c\n", rule_stanza_ptr->is_only_BWRC_stanza);
        printf("associativity: %d\n", rule_stanza_ptr->associativity);
        printf("testcase: %d\n", rule_stanza_ptr->testcase);
        for(i=0; i < rule_stanza_ptr->num_oper_defined; i++) {
            printf("oper: %s\n", rule_stanza_ptr->oper[i]);
        }
        for(i=0; i < rule_stanza_ptr->num_oper.num_keywds; i++) {
            printf("num_oper: %lld\n", rule_stanza_ptr->num_oper.value[i]);
        }
        for(i=0; i < rule_stanza_ptr->seek_breakup_prcnt.num_keywds; i++) {
            printf("seek_breakup_prcnt: %lld\n", rule_stanza_ptr->seek_breakup_prcnt.value[i]);
        }
        for(i=0; i < rule_stanza_ptr->starting_block.num_keywds; i++) {
            printf("Starting block: %llu\n", rule_stanza_ptr->starting_block.value[i]);
        }
        for(i=0; i < rule_stanza_ptr->direction.num_keywds; i++) {
            printf("Direction: %llu\n", rule_stanza_ptr->direction.value[i]);
        }
        for (i=0; i < rule_stanza_ptr->min_blkno.num_keywds; i++) {
            printf("min_blkno: 0x%llx\n", rule_stanza_ptr->min_blkno.value[i]);
        }
        for (i=0; i < rule_stanza_ptr->max_blkno.num_keywds; i++) {
            printf("max_blkno: 0x%llx\n", rule_stanza_ptr->max_blkno.value[i]);
        }
        for (i=0; i < rule_stanza_ptr->blk_hop.num_keywds; i++) {
            printf("blk_hop: %llu\n", rule_stanza_ptr->blk_hop.value[i]);
        }
        for (i=0; i < rule_stanza_ptr->data_len.num_keywds; i++ ) {
            printf("min_len: %lld, max_len: %lld, increment: %d\n",
                    rule_stanza_ptr->data_len.size[i].min_len, rule_stanza_ptr->data_len.size[i].max_len,
                    rule_stanza_ptr->data_len.size[i].increment);
        }
        for (i=0; i < rule_stanza_ptr->align.num_keywds; i++) {
            printf("align: %llu\n", rule_stanza_ptr->align.value[i]);
        }
        for (i=0; i < rule_stanza_ptr->lba_align.num_keywds; i++) {
            printf("lba_align: %llu\n", rule_stanza_ptr->lba_align.value[i]);
        }
        for (i=0; i < rule_stanza_ptr->num_mallocs.num_keywds; i++) {
            printf("num_mallocs: %llu\n", rule_stanza_ptr->num_mallocs.value[i]);
        }
        printf("Mode: %d\n", rule_stanza_ptr->mode);
        printf ("rule_option: %d\n", rule_stanza_ptr->rule_option);
        printf("seed: %d %d %d, data_seed: %d %d %d\n", rule_stanza_ptr->user_defined_seed[0], rule_stanza_ptr->user_defined_seed[1],
                rule_stanza_ptr->user_defined_seed[2], rule_stanza_ptr->user_defined_seed[3], rule_stanza_ptr->user_defined_seed[4],
                rule_stanza_ptr->user_defined_seed[5]);
        printf("lba_seed: %lld\n", rule_stanza_ptr->user_defined_lba_seed);
        for (i=0; i < rule_stanza_ptr->hotness.num_keywds; i++) {
            printf("hotness: %llu\n", rule_stanza_ptr->hotness.value[i]);
        }
        for (i=0; i < rule_stanza_ptr->loop_on_offset.num_keywds; i++) {
             printf("loop_on_offset: %llu\n", rule_stanza_ptr->loop_on_offset.value[i]);
        }
        printf("Section: %u\n", rule_stanza_ptr->section);
        printf ("run_read flag: %c\n", rule_stanza_ptr->run_reread);
        printf ("sleep: %d\n", rule_stanza_ptr->sleep);
        printf ("repeat_pos: %d, repeat_neg: %d\n", rule_stanza_ptr->repeat_pos, rule_stanza_ptr->repeat_neg);
        printf ("num_cache_threads: %d\n", rule_stanza_ptr->num_cache_threads);

    } else {
        tmplt_stanza_ptr = (template *) current_stanza_ptr;
        printf("-----------------------------\n");
        printf("Template_id: %s\n", tmplt_stanza_ptr->template_id);
        printf("seek_breakup_prcnt: %hd\n", tmplt_stanza_ptr->seek_breakup_prcnt);
        printf("oper: %s\n", tmplt_stanza_ptr->oper);
        printf("min_len: %lld\n", tmplt_stanza_ptr->data_len.min_len);
        printf("max_len: %lld\n", tmplt_stanza_ptr->data_len.max_len);
        printf("increment: %d\n", tmplt_stanza_ptr->data_len.increment);
    }
}

/***************************************************************************/
/*  Function to check if the value assigned to various stanza parameters   */
/*  are good. Otherwise, either set them appropriately OR print an error   */
/*  This is done after whole stanza is read                                */
/***************************************************************************/
void check_n_update_rule_params(struct htx_data *htx_ds, struct ruleinfo *current_stanza_ptr)
{
    int i;
    int cur_stanza_BWRC, num_loop_completed, remaining;
    char msg[512];

    /* If there is no template associated with the rule and no operation defined,
     * Update stanza with default operation.
     */
    if (current_stanza_ptr->num_associated_templates == 0) {
        if (current_stanza_ptr->num_oper_defined == 0) {
            strcpy(current_stanza_ptr->oper[0], "R");
            current_stanza_ptr->num_oper_defined = 1;
            current_stanza_ptr->is_only_BWRC_stanza = 'N';
        }
    } else {
        /* If there is a template associated and OPER is defined in rule as well. Print a error */
        if (current_stanza_ptr->num_oper_defined != 0) {
            sprintf(msg, "OPER parameter can not be defined in a rule if there is a template associated"
                        "with it. Please fix the rule with id: %s.\n", current_stanza_ptr->rule_id);
            user_msg(htx_ds, 0, 0, HARD, msg);
            exit(1);
        }
    }

    /* If testcase type is PERFORMANCE, seek type can either be RANDOM(seek_breakup_prcnt = 100)
     * OR SEQ (seek_breakup_prcnt = 0).
     */
    if (current_stanza_ptr->mode == PERFORMANCE) {
        for (i=0; i < current_stanza_ptr->seek_breakup_prcnt.num_keywds; i++) {
            if (!(current_stanza_ptr->seek_breakup_prcnt.value[i] == 0 || current_stanza_ptr->seek_breakup_prcnt.value[i] == 100)) {
                sprintf(msg, "For PERFORMANCE testcase, seek_type can either be RANDOM (i.e. seek_breakup_prcnt = 100) OR SEQ (i.e. seek_breakup_prcnt = 0).\n"
                            "Please fix the rulefile parameter.\n");
                user_msg(htx_ds, 0, 0, HARD, msg);
                exit(1);
            }
        }
        /* Compare operation is not allowed in PERF mode, check for it and report error. */
        if (current_stanza_ptr->num_associated_templates == 0) {
            for (i=0; i < current_stanza_ptr->num_oper_defined; i++) {
                if (strchr(current_stanza_ptr->oper[i], 'C') != NULL) {
                    sprintf(msg, "For PERFORMANCE testcase, compare operation can not be defined.\n"
                    "Please fix the rulefile parameter for rule id: %s\n", current_stanza_ptr->rule_id);
                    user_msg(htx_ds, 0, 0, HARD, msg);
                    exit(1);
                }
            }
        } else {
            for (i=0; i < current_stanza_ptr->num_associated_templates; i++) {
                if (strchr(current_stanza_ptr->tmplt[i].tmplt_ptr->oper, 'C') != NULL) {
                    sprintf(msg, "Template with id: %s can not be associated with rule id: %s as the rule will be running in the PERFORMANCE mode.\n"
                                "And compare oper is not allowed in perf mode.\n", current_stanza_ptr->tmplt[i].tmplt_ptr->template_id, current_stanza_ptr->rule_id);
                    user_msg(htx_ds, 0, 0, HARD, msg);
                    exit(1);
                }
            }
        }
    }

    /* If testcase type is ASYNC, mode has to be PERFORMANCE */
    if (current_stanza_ptr->testcase == ASYNC && current_stanza_ptr->mode != PERFORMANCE) {
        sprintf(msg, "For ASYNC testcase, only PERFORMANCE mode is supported. Hence re-setting the mode\n");
        user_msg(htx_ds, 0, 0, INFO, msg);
        current_stanza_ptr->mode = PERFORMANCE;
    }

    /* If testcase type is CACHE, seek_prcnt can only be 0 (i.e. SEQ) */
    if (current_stanza_ptr->testcase == CACHE) {
        for(i=0; i < current_stanza_ptr->seek_breakup_prcnt.num_keywds; i++) {
            if (current_stanza_ptr->seek_breakup_prcnt.value[i] != 0) {
                sprintf(msg, "CACHE testcase is run only with SEQ access. Hence changing seek_prcnt to 0\n");
                user_msg(htx_ds, 0, 0, INFO, msg);
                current_stanza_ptr->seek_breakup_prcnt.value[i] = 0;
            }
        }
        /* For testcase type CACHE, max num_threads can be 32 */
        if (current_stanza_ptr->num_threads > MAX_NUM_CACHE_THREADS) {
            sprintf(msg, "For testcase type \"CACHE\", max num_threads can only be 32. Please fix the problem in rulefile.\n");
            user_msg(htx_ds, 0, 0, HARD, msg);
            exit(1);
	    }
    }

    /* If stanza is not BWRC only and has both BWRC as well as non-BWRC operations defined. And
     * if num_oper_defined is less than num_threads and associativity is ROUND ROBIN, there is
     * a possibility that, BWRC might get picked more than once. So, update BWRC threads accordingly.
     */
    if (current_stanza_ptr->num_oper_defined < current_stanza_ptr->num_threads && current_stanza_ptr->num_BWRC_threads != 0 &&
        current_stanza_ptr->is_only_BWRC_stanza == 'N' && current_stanza_ptr->associativity == RROBIN) {
        cur_stanza_BWRC = current_stanza_ptr->num_BWRC_threads;
        num_loop_completed = current_stanza_ptr->num_threads / current_stanza_ptr->num_oper_defined;
        current_stanza_ptr->num_BWRC_threads = current_stanza_ptr->num_BWRC_threads * num_loop_completed;
        remaining = current_stanza_ptr->num_threads  % current_stanza_ptr->num_oper_defined;
        for(i = 0; i < remaining; i++) {
            if (strcasecmp(current_stanza_ptr->oper[i], "BWRC") == 0) {
                current_stanza_ptr->num_BWRC_threads++;
            }
        }
        total_BWRC_threads = total_BWRC_threads + current_stanza_ptr->num_BWRC_threads - cur_stanza_BWRC;
    }

    if (current_stanza_ptr->is_only_BWRC_stanza == 'Y') {
        /* If num_threads not explicitly defined for BWRC stanza, set it to default */
        if (current_stanza_ptr->num_threads == DEFAULT_NUM_THREADS) {
            current_stanza_ptr->num_BWRC_threads =  current_stanza_ptr->num_threads = DEFAULT_BWRC_THREADS;
        } else {
            current_stanza_ptr->num_BWRC_threads = current_stanza_ptr->num_threads;
        }
        total_BWRC_threads += current_stanza_ptr->num_BWRC_threads;
        /* Also, update num_oper and seek_breakup_prcnt to 0 if is_only_BWRC_stanza flag is "Y" */
        if (current_stanza_ptr->seek_breakup_prcnt.value[0] != 0 || current_stanza_ptr->seek_breakup_prcnt.num_keywds != 1) {
            current_stanza_ptr->seek_breakup_prcnt.num_keywds = 1;
            current_stanza_ptr->seek_breakup_prcnt.value[0] = 0;
        }
        if (current_stanza_ptr->num_oper.value[0] != 0 || current_stanza_ptr->num_oper.num_keywds != 1) {
            current_stanza_ptr->num_oper.num_keywds = 1;
            current_stanza_ptr->num_oper.value[0] = 0;
        }
        current_stanza_ptr->section = YES;
    } else if (current_stanza_ptr->num_threads == DEFAULT_NUM_THREADS) {
        current_stanza_ptr->num_threads = 1;
    }
    if(max_thread_cnt < current_stanza_ptr->num_threads){
        max_thread_cnt = current_stanza_ptr->num_threads;
    }
}

/*****************************************************************/
/*           Function to read rulefile                           */
/*****************************************************************/
int read_rf(struct htx_data *htx_ds, char *rf_name, unsigned long long maxblk, unsigned int blksize)
{
    FILE *fp;
    char keywd[32], str[MAX_RULE_LINE_SZ], msg[MSG_TEXT_SIZE];
    int keywd_count =0, line=0, rc=0, str_len, error_no;
    char eof = 'N';
    unsigned int stanza_defined = UNINITIALIZED;
    void *current_stanza_ptr;
    template *tmplt_ptr;
    parse_stanza_fptr fptr;

    /*********************************************/
    /****    Open rule file in read only mode ****/
    /*********************************************/
    if ((fp = fopen(rf_name, "r")) == NULL ) {
        error_no = errno;
        sprintf(msg, "Error opening file %s. Error no. set is %d.\n", rf_name, error_no);
        user_msg(htx_ds, errno, 0, SOFT, msg);
        return -1;
    }

    while (eof != 'Y') {
            rc = get_line(str, fp);
            /*
             * rc = 0  indicates comment in a rule file
             * rc = 1  indicates only '\n' (newline char) on the line, i.e. change in stanza
             * rc > 1  more characters, i.e. go ahead and check for valid parameter name.
             * rc = -1 error while reading file.
             * rc = -2 End of File.
             */
        if (rc == -1) {  /* Error in reading line */
            fclose(fp);
            sprintf(msg, "Error reading rule file %s.\n", rf_name);
            user_msg(htx_ds, errno, 0, SOFT, msg);
            return -1;
        }
        if (rc == -2) { /* Received end of file */
            eof = 'Y';
            fclose(fp);
        }
        if (rc == 0 || (rc == 1 && keywd_count == 0)) { /* Either comment or a blank line without any keywd read before */
            line = line + 1;
            continue;
        }
        str_len = rc;
        line = line + 1;
        if (str_len > 1) { /* Received some good data*/
            sscanf(str, "%s", keywd);
            if (keywd_count == 0 ) {
                if (strcasecmp(keywd, "RULE_ID") == 0) {
                    current_stanza_ptr = (struct ruleinfo *)&rule_list[num_rules_defined];
                    set_rule_defaults(current_stanza_ptr, maxblk, blksize);
                    fptr = &parse_rule_parameter;
                    stanza_defined = RULE_STANZA;
                } else if (strcasecmp(keywd, "TEMPLATE_ID") == 0) {
                    current_stanza_ptr = (template *)&tmplt_list[num_templates_defined];
                    set_template_defaults(current_stanza_ptr);
                    fptr = &parse_template_parameter;
                    stanza_defined = TEMPLATE_STANZA;
                } else {
                    sprintf(msg, "First line of each stanza must be either 'rule_id' OR 'template_id'.\n");
                    user_msg(htx_ds, errno, 0, SOFT, msg);
                    return -1;
                }
            }
            rc = (fptr)(htx_ds, str, (void *)current_stanza_ptr, &line, maxblk, blksize);
            if (rc != 0) {
                return rc;
            }
            keywd_count++;
        } else if (keywd_count != 0) {
            if (stanza_defined == RULE_STANZA && current_stanza_ptr != NULL ) {
                check_n_update_rule_params(htx_ds, current_stanza_ptr);
                num_rules_defined++;
            } else {
                tmplt_ptr = (template *)current_stanza_ptr;
                if (strcmp(tmplt_ptr->oper, " ") == 0) {
                    sprintf(msg, "OPER parameter is a MUST for template stanza.\nSince, it is not defined for"
                                "template with id: %s. Please fix this in rule file.\n", tmplt_ptr->template_id);
                    user_msg(htx_ds, 0, 0, HARD, msg);
                    exit(1);
                }
                num_templates_defined++;
            }
            /* print_stanza_input(current_stanza_ptr, stanza_defined);
            DPRINT("\n**** Stanza changed ***\n\n"); */
            keywd_count = 0;
        }
    }
    return 0;
}

/************************************************************************************/
/*              Function to parse the keywds of a rule line                         */
/************************************************************************************/
int parse_keywd(char *str, char *value[])
{
    char tmp_str[MAX_RULE_LINE_SZ];
    int i=0;

    strcpy(tmp_str, str);
    strtok(tmp_str, " \n\t");
    while ((value[i] = strtok(NULL, " \n\t")) != NULL) {
        i++;
    }

    return i;
}

/************************************************************************/
/*              Function to parse rule stanza                           */
/************************************************************************/
int parse_rule_parameter(struct htx_data *ps, char *str, void *stanza_ptr, int *line, unsigned long long maxblk, unsigned int blksize)
{
    int i=0, j, num_oper_index, num_keywds, queue_depth, BWRC_threads;
    char *input_rule[MAX_INPUT_PARAMS], varstr[32];
    char keywd[32], chr_ptr[3][32], *val, *ptr;
    char tmp_str[64], msg[MSG_TEXT_SIZE];
    double input_val;
    unsigned long long len;
    char optswtch = 'N';
    struct ruleinfo *rule = (struct ruleinfo *)stanza_ptr;

    sscanf(str, "%s", keywd);
    if (strcasecmp(keywd, "RULE_ID") == 0) {
        sscanf(str, "%*s %s", rule->rule_id);
        if (((strlen(rule->rule_id)) < 1) || ((strlen(rule->rule_id)) > 16)) {
            sprintf(msg, "line# %d %s = %s (Length must be 1 to 16 characters) \n", *line, keywd, rule->rule_id);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
    } else if (strcasecmp(keywd, "PATTERN_ID") == 0) {
        rule->num_pattern_defined = parse_keywd(str, &input_rule[0]);
        for(i=0; i < rule->num_pattern_defined; i++) {
            strcpy(rule->pattern_id[i], input_rule[i]);
        }
    } else if (strcasecmp(keywd, "NUM_THREADS") == 0) {
        /* Need to check if template is associated to the rule. Then need to parse the input to update
         *  template structure pointer for the rule. Otherwise, just update num_threads.
         */
        num_keywds = parse_keywd(str, &input_rule[0]);
        if (num_keywds == 1 && strchr(input_rule[0], '(') == NULL) { /* Only no. of threads defined. NO template attached */
            if ((strstr(input_rule[0], "QD") != NULL) || (strstr(input_rule[0], "qd") != NULL)) {
                strcpy(varstr, input_rule[0]);
                val = strtok(varstr, "qQ");
                input_val = atof(val); /* queue depth multiplier can be in fraction also */
                queue_depth = get_queue_depth(ps);
                input_val *= queue_depth;
                rule->num_threads = (unsigned int)input_val;
            } else {
                rule->num_threads = atoi(input_rule[0]);
            }
        } else { /* Template is associated with no. of threads. Parse that to fill template structure ptr */
            rule->num_threads = 0;
            BWRC_threads = 0;
            for(i=0; i < num_keywds; i++) {
                strcpy(varstr, input_rule[i]);
                val = strtok(varstr, "()");
                strcpy(tmp_str, val);
                if ((strstr(input_rule[i], "QD") != NULL) || (strstr(input_rule[i], "qd") != NULL)) {
                    val = strtok(tmp_str, "qQ");
                    input_val = atof(val);
                    queue_depth = get_queue_depth(ps);
                    input_val *= queue_depth;
                    rule->tmplt[i].num_threads = (unsigned int) input_val;
                } else {
                    rule->tmplt[i].num_threads = atoi(tmp_str);
                }
                rule->num_threads += rule->tmplt[i].num_threads;
                strcpy(varstr, input_rule[i]);
                val = strtok(varstr, "()");
                val = strtok(NULL, "()");
                strcpy(chr_ptr[0], val);
                for (j=0; j < num_templates_defined; j++) {
                    if (strcmp(chr_ptr[0], tmplt_list[j].template_id) == 0) {
                        rule->tmplt[rule->num_associated_templates].tmplt_ptr = &tmplt_list[j];
                        if (strcasecmp(tmplt_list[j].oper, "BWRC") != 0) {
                            rule->is_only_BWRC_stanza = 'N';
                        } else {
                            BWRC_threads += rule->tmplt[i].num_threads;
                        }
                        rule->num_associated_templates++;
                        break;
                    }
                }
                if (j == num_templates_defined) {
                    sprintf(msg, "line# %d %s = %s is incorrect.\nNo such template is defined.", *line, keywd, chr_ptr[0]);
                    user_msg(ps, 0, 0, SOFT, msg);
                    return -1;
                }
            }
            if (rule->is_only_BWRC_stanza == 'N') {
                rule->num_BWRC_threads = BWRC_threads;
                total_BWRC_threads += BWRC_threads;
            }
        }
        if (rule->num_threads < 0 || rule->num_threads > MAX_THREADS) {
            sprintf(msg, "line# %d %s = %u is incorrrect.\nIt must be between 0 and 4096.", *line, keywd, rule->num_threads);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
	    if(max_thread_cnt < rule->num_threads){
	        max_thread_cnt = rule->num_threads;
	    }
    } else if (strcasecmp(keywd, "ASSOCIATIVITY") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "SEQ") == 0) {
            rule->associativity = SEQ;
        } else if (strcasecmp(varstr, "RROBIN") == 0) {
            rule->associativity = RROBIN;
        } else if (strcasecmp(varstr, "RANDOM") == 0) {
            rule->associativity = RANDOM;
        } else {
            sprintf(msg, "line# %d %s = %s is incorrect.\nValid values are SEQ/RROBIN/RANDOM only.", *line, keywd, varstr);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
    } else if (strcasecmp(keywd, "TESTCASE") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "SYNC") == 0) {
            rule->testcase = SYNC;
        } else if (strcasecmp(varstr, "ASYNC") == 0) {
            rule->testcase = ASYNC;
        } else if (strcasecmp(varstr, "PASSTHROUGH") == 0) {
            rule->testcase = PASSTHROUGH;
        } else if (strcasecmp(varstr, "CACHE") == 0) {
            rule->testcase = CACHE;
        } else {
            sprintf(msg, "line# %d %s = %s is incorrect.\nValid values are SYNC/ASYNC/PASSTHROUGH only.", *line, keywd, varstr);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
    } else if (strcasecmp(keywd, "OPER") == 0) {
        rule->num_oper_defined = parse_keywd(str, &input_rule[0]);
        BWRC_threads = 0;
        for (i=0, num_oper_index = 0; i < rule->num_oper_defined; i++) {
            if ((strcasecmp(input_rule[i], "R") == 0) ||
                (strcasecmp(input_rule[i], "S") == 0) ||
                (strcasecmp(input_rule[i], "RC") == 0) ||
                (strcasecmp(input_rule[i], "WRC") == 0) ||
                (strcasecmp(input_rule[i], "RWRC") == 0) ||
                (strcasecmp(input_rule[i], "BWRC") == 0) ||
                (strcasecmp(input_rule[i], "W") == 0) ||
                (strcasecmp(input_rule[i], "WR") == 0) ||
                (strcasecmp(input_rule[i], "RW") == 0) ||
                (strcasecmp(input_rule[i], "RS") == 0) ||
                (strcasecmp(input_rule[i], "WS") == 0) ||
                (strcasecmp(input_rule[i], "CARR") == 0) ||
                (strcasecmp(input_rule[i], "CARW") == 0) ||
                (strcasecmp(input_rule[i], "CAWW") == 0) ||
                (strcasecmp(input_rule[i], "CAWR") == 0) ||
                (strcasecmp(input_rule[i], "XCMD") == 0) ||
            #ifdef __CAPI_FLASH__
                (strcasecmp(input_rule[i], "U") == 0) ||
            #else
                (strcasecmp(input_rule[i], "D") == 0) ||
                (strcasecmp(input_rule[i], "DRC") == 0) ||
            #endif
                (strchr (input_rule[i], '[') != NULL)) {
            #ifdef __CAPI_FLASH__
                if (strcasecmp(input_rule[i], "U") == 0) {
                    if (discard_enabled == 0) {
                        sprintf(msg, "unmap is not enabled for the device. Rule line# %d has unmap oper\n"
                                     "defined. Will ignore it.\n", *line);
                        user_msg(ps, 0, 0, INFO, msg);
                        continue;
                    }
                }
            #else
                if ((strcasecmp(input_rule[i], "DRC") == 0) || (strcasecmp(input_rule[i], "D") == 0)) {
                    discard_enabled = 1;
                }
            #endif
                strcpy(rule->oper[i], input_rule[i]);
                num_oper_index++;
                if (strcasecmp(input_rule[i], "BWRC") == 0) {
                    BWRC_threads++;
                } else {
                    rule->is_only_BWRC_stanza = 'N';
                }
            } else {
                sprintf(msg, "line# %d %s = %s is incorrect. Only allowed values are:\n"
                             "R,S,W,RC,RS,RW,WR,WS,WRC,BWRC,D, DRC,CARR,CARW,CAWR,CAWW or XCMD.\n",
                             *line, keywd, input_rule[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        }
        rule->num_oper_defined = num_oper_index;
        if (rule->is_only_BWRC_stanza == 'N' && BWRC_threads != 0) {
                rule->num_BWRC_threads = BWRC_threads;
                total_BWRC_threads += BWRC_threads;
        }
    } else if (strcasecmp(keywd, "NUM_OPER") == 0) {
        rule->num_oper.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->num_oper.num_keywds; i++) {
            rule->num_oper.value[i] = strtoull(input_rule[i], (char **) NULL, 10);
            if (rule->num_oper.value[i] < 0 ) {
                sprintf(msg, "line# %d %s = %lld is incorrect.\nIt must be >= 0.", *line, keywd, rule->num_oper.value[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        }
    } else if (strcasecmp(keywd, "SEEK_BREAKUP_PRCNT") == 0) {
        rule->seek_breakup_prcnt.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->seek_breakup_prcnt.num_keywds; i++) {
            rule->seek_breakup_prcnt.value[i] = strtoull(input_rule[i], (char **) NULL, 10);
            if (rule->seek_breakup_prcnt.value[i] < 0 || rule->seek_breakup_prcnt.value[i] > 100) {
                sprintf(msg, "line# %d %s = %lld is incorrect.\nIt must be >= 0 and <= 100.", *line, keywd, rule->seek_breakup_prcnt.value[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        }
    } else if (strcasecmp(keywd, "TRANSFER_SIZE") == 0) {
        rule->data_len.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->data_len.num_keywds; i++) {
            if (input_rule[i][0] != '[') {
                rule->data_len.size[i].min_len = get_value(input_rule[i], blksize);
                rule->data_len.size[i].max_len = rule->data_len.size[i].min_len;
                rule->data_len.size[i].increment = 0;
            } else {
                strcpy(tmp_str, input_rule[i]);
                num_keywds = 0;
                val = strtok(tmp_str, "[,]");
                strcpy(chr_ptr[num_keywds++], val);
                while ((val = strtok(NULL, "[,]")) != NULL ) {
                    strcpy(chr_ptr[num_keywds], val);
                    num_keywds++;
                }
                strcpy(tmp_str, input_rule[i]);
                if ((val = strchr(tmp_str, ',')) == NULL) {  /* means increment not given */
                    rule->data_len.size[i].increment = 0;
                } else {
                    rule->data_len.size[i].increment = get_value(chr_ptr[num_keywds-1], blksize);
                }
                strcpy(tmp_str, chr_ptr[0]);
                if ((val = strchr(tmp_str, '-')) == NULL) {
                    rule->data_len.size[i].min_len = get_value(chr_ptr[0], blksize);
                    rule->data_len.size[i].max_len = rule->data_len.size[i].min_len;
                } else {
                    val = strtok(tmp_str, "-");
                    rule->data_len.size[i].min_len = get_value(val, blksize);
                    val = strtok(NULL, "-");
                    rule->data_len.size[i].max_len = get_value(val, blksize);
                }
                if (rule->data_len.size[i].increment == 0) {
                    rule->data_len.size[i].max_len = rule->data_len.size[i].min_len;
                }
            }

            /* Check if min_len, max_len and increment is not multiple of blksize. Set it as
             * multiple of blksize and put an info msg .
             */
            if (rule->data_len.size[i].min_len < blksize) {
                rule->data_len.size[i].min_len = blksize;
            } else if (rule->data_len.size[i].min_len % blksize != 0) {
                len = rule->data_len.size[i].min_len;
                rule->data_len.size[i].min_len = (len / blksize) * blksize; /* making it multiple of blksize */
                sprintf(msg, "line# %d %s(min_len) = %lld is not multiple of block size. Setting it to %lld (multiple of block size).\n",
                            *line, keywd, len, rule->data_len.size[i].min_len);
                user_msg(ps, 0, 0, INFO, msg);
            }

            if (rule->data_len.size[i].max_len < rule->data_len.size[i].min_len) {
                sprintf(msg, "line# %d %s(max_len) = %lld is less than min_len. Setting it equal to min_len(%lld).\n",
                              *line, keywd, rule->data_len.size[i].max_len, rule->data_len.size[i].min_len);
                user_msg(ps, 0, 0, INFO, msg);
                rule->data_len.size[i].max_len = rule->data_len.size[i].min_len;
                rule->data_len.size[i].increment = 0;
            } else if (rule->data_len.size[i].max_len % blksize != 0) {
                len = rule->data_len.size[i].max_len;
                rule->data_len.size[i].max_len = (len / blksize) * blksize; /* making it multiple of blksize */
                sprintf(msg, "line# %d %s(max_len) = %lld is not multiple of block size. Setting it to %lld (multiple of block size).\n",
                            *line, keywd, len, rule->data_len.size[i].max_len);
                user_msg(ps, 0, 0, INFO, msg);
            }

            if (rule->data_len.size[i].increment != -1 && rule->data_len.size[i].increment % blksize != 0) {
                len = rule->data_len.size[i].increment;
                rule->data_len.size[i].increment = (len / blksize) * blksize; /* making it multiple of blksize */
                sprintf(msg, "line# %d %s(increment) = %d is not multiple of block size. Setting it to %d(multiple of block size).\n",
                            *line, keywd, (unsigned int)len, rule->data_len.size[i].increment);
                user_msg(ps, 0, 0, INFO, msg);
            }
        }
    } else if (strcasecmp(keywd, "STARTING_BLOCK") == 0) {
        rule->starting_block.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->starting_block.num_keywds; i++) {
            if (strcasecmp(input_rule[i], "BOT") == 0) {
                 rule->starting_block.value[i] = BOT;
            } else if (strcasecmp(input_rule[i], "TOP") == 0) {
                rule->starting_block.value[i] = TOP;
            } else if (strcasecmp(input_rule[i], "MID") == 0) {
                rule->starting_block.value[i] = MID;
            } else {
                rule->starting_block.value[i] = strtoull(input_rule[i], (char **) NULL, 10);
            }
        }
    } else if (strcasecmp(keywd, "DIRECTION") == 0) {
        rule->direction.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->direction.num_keywds; i++) {
            if (strcasecmp(input_rule[i], "UP") == 0 ) {
                rule->direction.value[i] = UP;
            } else if (strcasecmp(input_rule[i], "DOWN") == 0) {
                rule->direction.value[i] = DOWN;
            } else if (strcasecmp(input_rule[i], "IN") == 0) {
                rule->direction.value[i] = IN;
            } else if (strcasecmp(input_rule[i], "OUT") == 0) {
                rule->direction.value[i] = OUT;
            } else {
                sprintf(msg,"line# %d %s = %s is incorrect.\nValid values are UP/DOWN/IN/OUT only", *line, keywd, input_rule[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        }
    } else if (strcasecmp(keywd, "MAX_BLKNO") == 0) {
        rule->max_blkno.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->max_blkno.num_keywds; i++) {
            if (strcmp (input_rule[i], ".100") == 0) {
                rule->max_blkno.value[i] = dev_info.maxblk;
            } else {
                input_val = atof(input_rule[i]);
                if (input_val > 0 && input_val <= 1) {
                    rule->max_blkno.value[i] = (unsigned long long) (input_val * maxblk);
                } else {
                    rule->max_blkno.value[i] = (unsigned long long)input_val;
                }
                if (rule->max_blkno.value[i] < 0) {
                    sprintf(msg, "line# %d %s = %lld is incorrect.\n It must be > 0.", *line, keywd, rule->max_blkno.value[i]);
                    user_msg(ps, 0, 0, SOFT, msg);
                    return -1;
                }
            }
            if (rule->max_blkno.value[i] > dev_info.maxblk) {
                sprintf(msg, "line# %d %s = %lld is > max_blkno(0x%llx). Setting it to maxblk", *line, keywd, rule->max_blkno.value[i], dev_info.maxblk);
                user_msg(ps, 0, 0, INFO, msg);
                rule->max_blkno.value[i] = dev_info.maxblk;
            }
        }
    } else if (strcasecmp(keywd, "MIN_BLKNO") == 0) {
        rule->min_blkno.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->min_blkno.num_keywds; i++) {
            input_val = atof(input_rule[i]);
            if (input_val > 0 && input_val < 1) {
                rule->min_blkno.value[i] = (unsigned long long) (input_val * maxblk);
            } else {
                rule->min_blkno.value[i] = (unsigned long long)input_val;
            }
            if (rule->min_blkno.value[i] < 0) {
                sprintf(msg, "line# %d %s = %lld is incorrect.\n It must be > 0.", *line, keywd, rule->min_blkno.value[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        }
    } else if (strcasecmp(keywd, "BLK_HOP") == 0) {
        rule->blk_hop.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->blk_hop.num_keywds; i++) {
            rule->blk_hop.value[i] = strtoull(input_rule[i], (char **) NULL, 10);
        }
    } else if (strcasecmp(keywd, "ALIGN") == 0) {
        rule->align.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->align.num_keywds; i++) {
            rule->align.value[i] = strtoull(input_rule[i], (char **) NULL, 10);
        #ifdef __HTX_LINUX__
            if (rule->align.value[i] < blksize || (rule->align.value[i] % blksize) != 0) {
                sprintf(msg, "line# %d %s = %lld is incorrect.\nLinux expects IO Buffers to be minimum block align.", *line, keywd, rule->align.value[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        #else
            if (rule->align.value[i] < 0) {
                sprintf(msg, "line# %d %s = %lld is incorrect.\nIt must be > 0.", *line, keywd, rule->align.value[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        #endif
        }
    } else if (strcasecmp(keywd, "LBA_ALIGN") == 0) {
        rule->lba_align.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->lba_align.num_keywds; i++) {
            rule->lba_align.value[i] = strtoull(input_rule[i], (char **) NULL, 10);
            if (rule->lba_align.value[i] < 0) {
                sprintf(msg, "line# %d %s = %lld is incorrect.\nIt must be > 0.", *line, keywd, rule->lba_align.value[i]);
                return -1;
            }
        }
    } else if (strcasecmp(keywd, "NUM_MALLOCS") == 0) {
        rule->num_mallocs.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->num_mallocs.num_keywds; i++) {
            rule->num_mallocs.value[i] = strtoull(input_rule[i], (char **) NULL, 10);
            if (rule->num_mallocs.value[i] < 0 || rule->num_mallocs.value[i] > 100) {
                sprintf(msg, "line# %d %s = %lld is incorrect.\nIt must be >=0 and <= 100.", *line, keywd, rule->num_mallocs.value[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        }
    } else if (strcasecmp(keywd, "MODE") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "PERFORMANCE") == 0) {
            rule->mode = PERFORMANCE;
        } else if (strcmp(varstr, "VALIDATION") == 0) {
            rule->mode = VALIDATION;
        } else {
            sprintf(msg, "line# %d %s = %s is incorrect,\n Valid values are VALIDATION/PERFORMANCE\n", *line, keywd, varstr);
            user_msg(ps, 0, 0, SOFT, msg);
            return (-1);
        }
    } else if (strcasecmp(keywd, "RULE_OPTION") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "SAVE_SEEDS") == 0) {
            if (optswtch == 'N') {
                    rule->rule_option |= SAVE_SEEDS_FLAG;
                    seeds_were_saved = 1;
                    optswtch = 'S';
            } else if (optswtch == 'R') {
                sprintf(msg, "line #%d: Restore Seeds and Save Seeds is NOT allowed in the same stanza!\n",
                             *line);
                user_msg(ps, 0, 0, SOFT, msg);
                return(-1);
            } else {
                sprintf(msg, "line #%d: Specify Seeds and Save Seeds is NOT allowed in the same stanza!\n",
                        *line);
                user_msg(ps, 0, 0, SOFT, msg);
                return(-1);
            }
        } else if (strcasecmp(varstr, "RESTORE_SEEDS") == 0) {
            if (optswtch == 'N') {
                if (seeds_were_saved) {
                    rule->rule_option |= RESTORE_SEEDS_FLAG;
                    optswtch = 'R';
                } else {
                    sprintf(msg, "line #%d - RESTORE_SEEDS option not allowed unless saved in previous stanza.",
                            *line);
                    user_msg(ps, 0, 0, SOFT, msg);
                    return(-1);
                }
            } else if (optswtch == 'S') {
                sprintf(msg, "line #%d: Save Seeds and Restore Seeds is NOT allowed in the same stanza!\n",
                        *line);
                user_msg(ps, 0, 0, SOFT, msg);
                return(-1);
            } else {
                sprintf(msg, "line #%d: Specify Seeds and Restore Seeds is NOT allowed in the same stanza!\n",
                        *line);
                user_msg(ps, 0, 0, SOFT, msg);
                return(-1);
            }
        } else if (strcasecmp(varstr, "SEEDS") == 0) {
            if (optswtch == 'N') {
                sscanf(str, "%*s %*s %n", &i);
                while ( isspace(str[i]) ) i++;
                strcpy(varstr, &str[i]);
                ptr = varstr;
                i = 0;
                 while (*ptr && i < 6 ) {
                    if (isdigit(*ptr)) {
                        /* code folded from here */
                        rule->user_defined_seed[i] = atoi(ptr);
                        i++;
                        while (isdigit(*(ptr++)));
                        /* unfolding */
                    } else {
                        /* code folded from here */
                        ptr++;
                        /* unfolding */
                    }
                }
                rule->user_defined_lba_seed = strtoull(ptr, (char **)NULL, 10);
                if (rule->user_defined_lba_seed == 0 ) {
                    rule->user_defined_lba_seed = maxblk;
                }
				rule->rule_option |= USER_SEEDS_FLAG;
                optswtch = 'U';
            } else if (optswtch == 'S') {
                sprintf(msg, "line #%d: Save Seeds and Specify Seeds is NOT allowed in the same stanza!\n",
                            *line);
                user_msg(ps, 0, 0, SOFT, msg);
                return(-1);
            } else {
                sprintf(msg, "line #%d: Restore Seeds and Specify Seeds is NOT allowed in the same stanza!\n",
                            *line);
                user_msg(ps, 0, 0, SOFT, msg);
                return(-1);
            }
        } else {
            sprintf(msg, "line #%d %s = %s (invalid option " "for RULE_OPTIONS)\n",
                        *line, keywd, varstr);
            user_msg(ps, 0, 0, SOFT, msg);
            return (-1);
        }
    } else if (strcasecmp(keywd, "HOTNESS") == 0) {
        rule->hotness.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->hotness.num_keywds; i++) {
            rule->hotness.value[i] = strtoull(input_rule[i], (char **) NULL, 10);
            if (rule->hotness.value[i] < 1 || rule->hotness.value[i] > 32768) {
                sprintf(msg, "line# %d %s = %lld is incorrect,\nIt must be >= 1 and <= 32768.", *line, keywd, rule->hotness.value[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        }
    } else if (strcasecmp(keywd, "LOOP_ON_OFFSET") == 0) {
    #ifdef __HTX_LINUX__
        /* loop_on_offsst is not a valid paramater on linux, as we need block align buffer. So, ignore this. */
         rule->loop_on_offset.value[0] = 0;
    #else
        rule->loop_on_offset.num_keywds = parse_keywd(str, &input_rule[0]);
        for (i=0; i < rule->loop_on_offset.num_keywds; i++) {
            if (strcasecmp(input_rule[i], "YES") == 0) {
                rule->loop_on_offset.value[i] = 1;
            } else if (strcasecmp(input_rule[i], "NO") == 0) {
                rule->loop_on_offset.value[i] = 0;
            } else {
                sprintf(msg, "line# %d %s = %s is incorrect.\nIt must be YES or NO.", *line, keywd, input_rule[i]);
                user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        }
    #endif
    } else if ( strcasecmp(keywd, "RUN_ON_MISC") == 0 ) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            run_on_misc = 'Y';
        }
    } else if ( strcasecmp(keywd, "MISC_CMD") == 0 ) {
        sscanf(str, "%*s %[^\n]s",  misc_run_cmd);
    } else if (strcasecmp(keywd, "RUN_REREAD") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp (varstr, "NO") == 0) {
            rule->run_reread = 'N';
        }
    } else if (strcasecmp(keywd, "SECTION") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            rule->section = YES;
        } else if (strcasecmp(varstr, "NO") == 0) {
            rule->section = NO;
        }
    } else if (strcasecmp(keywd, "UNIQUE_PATTERN") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            num_sub_blks_psect = (blksize / 128) - 1;
        } else if (strcasecmp(varstr, "YES") == 0) {
            num_sub_blks_psect = 0;
        }
    } else if (strcasecmp(keywd, "SLEEP") == 0) {
        sscanf(str, "%*s %s", varstr);
        rule->sleep = (unsigned int)strtoul(varstr, NULL, 10);
    } else if (strcasecmp(keywd, "SKIP") == 0) {
        sscanf(str, "%*s %s", varstr);
        if ( varstr[0] == '-' ) {
            varstr[0] = '0';
            rule->repeat_neg = atoi(varstr);
        } else {
            rule->repeat_pos = atoi(varstr);
        }
    } else if (strcasecmp(keywd, "CRASH_ON_MIS") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            dev_info.crash_on_miscom = 1;
        } else if (strcasecmp(varstr, "NO") == 0) {
            dev_info.crash_on_miscom = 0;
        } else {
            sprintf(msg, "line# %d keywd = %s (invalid). Valid values are YES/NO.\n", *line, keywd);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
    } else if (strcasecmp(keywd, "ENABLE_ATTN") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            turn_attention_on = 1;
        } else if (strcasecmp(varstr, "NO") == 0) {
            turn_attention_on = 0;
        }
    } else if (strcasecmp(keywd, "SYNC_CACHE") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            sync_cache_flag = 1;
        } else if (strcasecmp(varstr, "NO") == 0) {
            sync_cache_flag = 0;
        } else {
            sprintf(msg, "line# %d keywd = %s (invalid). Valid values are YES/NO.\n", *line, keywd);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
    } else if (strcasecmp(keywd, "RANDOMIZE_SYNC_CACHE") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            randomize_sync_cache = 1;
        } else if (strcasecmp(varstr, "NO") == 0) {
            randomize_sync_cache = 0;
        } else {
            sprintf(msg, "line# %d keywd = %s (invalid). Valid values are YES/NO.\n", *line, keywd);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
	#ifdef __CAPI_FLASH__
	} else if ( (strcasecmp(keywd, "OPEN_FLAG")) == 0) {
    	sscanf(str, "%*s %s", varstr);
		printf("keywd=%s, varstr=%s \n", keywd, varstr);
        if ( strcasecmp(varstr, "PHYSICAL_LUN") == 0) {
        	rule->open_flag = CBLK_OPN_PHY_LUN;
			if(lun_type != rule->open_flag) {
				sprintf(msg, "line# %d keywd = %s (invalid). lun_type(=%d) shd be same as open flag(=%d) \n", *line, keywd, lun_type, rule->open_flag);
				sprintf(msg + strlen(msg), "Check if file /tmp/test_lun_mode exists (with value 1) for enabling PLUN testing, Create file and relogin to HTX \n");
				user_msg(ps, 0, 0, SOFT, msg);
				return -1;
			}
        } else if ( strcasecmp(varstr, "VIRTUAL_LUN") == 0) {
        	rule->open_flag = CBLK_OPN_VIRT_LUN;
			if(lun_type != rule->open_flag) {
				sprintf(msg, "line# %d keywd = %s (invalid). lun_type(=%d) shd be same as open flag(=%d) \n", *line, keywd, lun_type, rule->open_flag);
			#ifndef __HTX_LINUX__
			    sprintf(msg + strlen(msg), "Check if file /tmp/test_lun_mode exists (with value >1) for enabling VLUN testing, and relogin to HTX \n");
			#else
			    sprintf(msg + strlen(msg), "Either remove file /tmp/test_lun_mode OR make aure it has value >1 for enabling VLUN testing, and relogin to HTX \n");
			#endif
			    user_msg(ps, 0, 0, SOFT, msg);
                return -1;
            }
        } else {
        	sprintf(msg, "line# %d %s = %s (must be PHYSICAL_LUN/VIRTUAL_LUN/CBLK_CLOSE_LUN ) \n", *line, keywd, varstr);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
		printf("%s:lun_type = %x, open_flag = %#x\n", __FUNCTION__, lun_type, rule->open_flag);
	} else if ( (strcasecmp(keywd, "CHUNK_SIZE")) == 0) {
    	sscanf(str, "%*s %s", varstr);
        chunk_size = atoi(varstr);
        if(chunk_size < 0) {
        	sprintf(msg, "line# %d %s = %lx (must be positive integer) \n", *line, keywd, chunk_size);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
	#endif
    } else if (strcasecmp(keywd, "CRASH_ON_ANYERR") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            dev_info.crash_on_anyerr = 1;
        } else if (strcasecmp(varstr, "NO") == 0) {
            dev_info.crash_on_anyerr = 0;
        }
    } else if (strcasecmp(keywd, "CRASH_ON_HANG") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            dev_info.crash_on_hang = 1;
        } else if (strcasecmp(varstr, "NO") == 0) {
            dev_info.crash_on_hang = 0;
        }
    } else if (strcasecmp(keywd, "HANG_TIME") == 0) {
        sscanf(str, "%*s %s", varstr);
        hang_time = atoi(varstr);
    } else if (strcasecmp(keywd, "HANG_THRESHOLD") == 0) {
        sscanf(str, "%*s %s", varstr);
        threshold = atoi(varstr);
    } else if (strcasecmp(keywd, "DEBUG_FLAG") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            dev_info.debug_flag = YES;
        } else if (strcasecmp(varstr, "NO") == 0) {
            dev_info.debug_flag = NO;
        }
	} else if (strcasecmp(keywd, "NUM_ASYNC_IO") == 0) {
		sscanf(str, "%*s %s", varstr);
		rule->num_async_io = atoi(varstr);
    } else if (strcasecmp(keywd, "NUM_CACHE_THREADS") == 0) {
        sscanf(str, "%*s %s", varstr);
        rule->num_cache_threads = atoi(varstr);
        if (rule->num_cache_threads > MAX_NUM_CACHE_THREADS) {
            sprintf(msg, "line# %d %s = %d is incorrect. Max value for it can be 32.\n", *line, keywd, rule->num_cache_threads);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
	    }
    } else if (strcasecmp(keywd, "CONT_ON_ERR") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "NO") == 0) {
            dev_info.cont_on_err = NO;
        } else if (strcasecmp(varstr, "YES") == 0) {
            dev_info.cont_on_err = YES;
        } else if (strcasecmp(varstr, "DATACHECK") == 0) {
            dev_info.cont_on_err = DATACHECK;
        }
    } else if (strcasecmp(keywd, "CONT_ON_MISC") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "NO") == 0) {
            dev_info.cont_on_misc = NO;
        } else if (strcasecmp(varstr, "YES") == 0) {
            dev_info.cont_on_misc = YES;
        }
    } else if (strcasecmp(keywd, "ENABLE_STATE_TABLE") == 0) {
        sscanf(str, "%*s %s", varstr);
        if (strcasecmp(varstr, "YES") == 0) {
            enable_state_table = YES;
	    } else if (strcasecmp(varstr, "NO") == 0) {
            enable_state_table = NO;
	    }
    } else if ((strcasecmp(keywd, "COMMAND")) == 0 ) {
        j = 0;
        for (i = 10; str[i] != '\n' && str[i] != '\0'; i++, j++) {
            rule->cmd_list[j] = str[i];
        }
        rule->cmd_list[j] = '\0';
    } else {
        sprintf(msg,"line# %d keywd = %s (invalid)\n", *line, keywd);
        user_msg(ps, 0, 0, SOFT, msg);
        return -1;
    }
    return 0;
}

/******************************************************/
/*   Function to get queue_depth of the disk device   */
/******************************************************/
int get_queue_depth(struct htx_data *ps)
{
    int i, j=0, q_depth;
    char dev_name[32], cmd_str[128];
    char buf[16], msg[256];
#ifndef __HTX_LINUX__
    char disk_num[6];
#endif
    FILE *fp;

#ifdef __HTX_LINUX__
    for (i = 5; i < strlen(ps->sdev_id); i++) {
        if (isdigit(ps->sdev_id[i])) {
            break;
        } else {
            dev_name[j] = ps->sdev_id[i];
            j++;
        }
    }
    dev_name[j] = '\0';
    if (strncmp(dev_name, "sd", 2) != 0) {
        sprintf(msg, "Setting the queue depth to default (i.e. 16)\n");
        user_msg(ps, 0, 0, INFO, msg);
        q_depth = DEFAULT_QUEUE_DEPTH;
        return q_depth;
   } else {
        sprintf(cmd_str,"cat /sys/block/%s/device/queue_depth 2>/dev/null", dev_name);
   }
#else
    if (strncmp(&ps->sdev_id[5], "rlv",3) == 0) { /* means LV, get queue depth for disk on which LV is created */
        for (i = 8; i < strlen(ps->sdev_id); i++) {
			if (isdigit(ps->sdev_id[i])) {
				disk_num[j] = ps->sdev_id[i];
				j++;
			} else {
				break;
			}
		}
		sprintf(cmd_str, "lsattr -El hdisk%s -a queue_depth | awk '{print $2}'", disk_num);
	} else {
		sprintf(cmd_str, "lsattr -El %s -a queue_depth | awk '{print $2}'", dev_info.diskname);
    }
#endif
    fp = popen(cmd_str, "r");
    if (fp == NULL) {
        sprintf(msg, "popen failed while getting queue depth. errno: %d\n. Setting it to default value (i.e. 16)\n", errno);
        user_msg(ps, 0, 0, INFO, msg);
        q_depth = DEFAULT_QUEUE_DEPTH;
    } else {
        if (fgets(buf, 16, fp) == NULL) {
            sprintf(msg, "fgets failed while getting queue depth. errno: %d\nSetting queue depth to default (i.e. 16)\n", errno);
            user_msg(ps, 0, 0, INFO, msg);
            pclose(fp);
            q_depth = DEFAULT_QUEUE_DEPTH;
        } else {
            pclose(fp);
            q_depth = atoi(buf);
	    }
    }
    return q_depth;
}

/*************************************************/
/* Function to convert string into integer value */
/*************************************************/
long long get_value (char *str, unsigned int blksize)
{
    int str_len;
    int i, j=0;
    char tmp_str[64];
    long long size;

    str_len = strlen(str);
    i = 0;
    if (str[0] == '-') {
        tmp_str[j++] = str[i++];
    }
    for(; i < str_len; i++) {
        if(isdigit(str[i])) {
            tmp_str[j] = str[i];
            j++;
        } else {
            break;
        }
    }
    tmp_str[j] = '\0';
    size = strtoll(tmp_str, (char **)NULL, 10);
    if (i != str_len) {
#if 0
        if (str[i] == 'Q' || str[i] == 'q') {
            if (size == 0) {
                size = 1;
            }
        } else if (str[i] == 'K' || str[i] == 'k') {
#endif
        if (str[i] == 'K' || str[i] == 'k') {
            size *= KB;
        } else if (str[i] == 'M' || str[i] == 'm') {
            size *= MB;
        } else if (str[i] == 'G' || str[i] == 'g') {
            size *= GB;
        } else if (strcasecmp(&str[i], "BLK") == 0) {
            size *= blksize;
        }
    }
    return size;
}

/************************************************************************
*                   Function to parse template stanza                   *
*************************************************************************/
int parse_template_parameter(struct htx_data *ps, char *str, void *stanza_ptr, int *line, unsigned long long maxblk, unsigned int blksize)
{
    char keywd[32], input_keywd[64];
    char tmp_str[64], msg[256];
    int num_input=0;
    char chr_ptr[3][32], *val;
    unsigned long long len;
    template *tmplt = (template *)stanza_ptr;

    sscanf(str, "%s", keywd);
    if (strcasecmp(keywd, "TEMPLATE_ID") == 0) {
        sscanf(str, "%*s %s", tmplt->template_id);
    } else if (strcasecmp(keywd, "TRANSFER_SIZE") == 0) {
        sscanf(str, "%*s %s", input_keywd);
        if (input_keywd[0] != '[') {
            tmplt->data_len.min_len = get_value(input_keywd, blksize);
            tmplt->data_len.max_len = tmplt->data_len.min_len;
            tmplt->data_len.increment = 0;
        } else {
            strcpy(tmp_str, input_keywd);
            val = strtok(tmp_str, "[,]");
            strcpy(chr_ptr[num_input++], val);
            while ((val = strtok(NULL, "[,]")) != NULL ) {
                strcpy(chr_ptr[num_input], val);
                num_input++;
            }
            if (num_input == 1) {
                tmplt->data_len.increment = 0;
            } else {
                tmplt->data_len.increment = get_value(chr_ptr[num_input-1], blksize);
            }
            num_input = 0;
            strcpy(tmp_str, chr_ptr[0]);
            val = strtok(tmp_str, "-");
            strcpy(chr_ptr[num_input++], val);
            while((val = strtok(NULL, "-")) != NULL ) {
                strcpy(chr_ptr[num_input], val);
                num_input++;
            }
            tmplt->data_len.min_len = get_value(chr_ptr[0], blksize);
            if (num_input == 1) {
                tmplt->data_len.max_len = tmplt->data_len.min_len;
            } else {
                tmplt->data_len.max_len = get_value(chr_ptr[1], blksize);
            }
            if (tmplt->data_len.increment == 0) {
                tmplt->data_len.max_len = tmplt->data_len.min_len;
            }

        }
        /* Check if min_len, max_len and increment is not multiple of blksize. Set it as
         * multiple of blksize and put an info msg.
         */
        if (tmplt->data_len.min_len < blksize) {
            tmplt->data_len.min_len = blksize;
        } else if (tmplt->data_len.min_len % blksize != 0) {
            len = tmplt->data_len.min_len;
            tmplt->data_len.min_len = (len / blksize) * blksize; /* making it multiple of blksize */
            sprintf(msg, "line# %d %s(min_len) = %lld is not multiple of block size. Setting it to %lld (multiple of block size).\n",
                        *line, keywd, len, tmplt->data_len.min_len);
            user_msg(ps, 0, 0, INFO, msg);
        }

        if (tmplt->data_len.max_len < tmplt->data_len.min_len) {
            tmplt->data_len.max_len = tmplt->data_len.min_len;
            tmplt->data_len.increment = 0;
        } else if (tmplt->data_len.max_len % blksize != 0) {
            len = tmplt->data_len.max_len;
            tmplt->data_len.max_len = (len / blksize) * blksize; /* making it multiple of blksize */
            sprintf(msg, "line# %d %s(max_len) = %lld is not multiple of block size. Setting it to %lld (multiple of block size).\n",
                        *line, keywd, len, tmplt->data_len.max_len);
            user_msg(ps, 0, 0, INFO, msg);
        }
        if (tmplt->data_len.increment != -1 && tmplt->data_len.increment % blksize != 0) {
            len = tmplt->data_len.increment;
            tmplt->data_len.increment = (len / blksize) * blksize; /* making it multiple of blksize */
            sprintf(msg, "line# %d %s(increment) = %d is not multiple of block size. Setting it to %d(multiple of block size).\n",
                        *line, keywd, (unsigned int)len, tmplt->data_len.increment);
            user_msg(ps, 0, 0, INFO, msg);
        }
    } else if (strcasecmp(keywd, "SEEK_BREAKUP_PRCNT") == 0) {
        sscanf(str, "%*s %hu", &(tmplt->seek_breakup_prcnt));
        if (tmplt->seek_breakup_prcnt < 0 || tmplt->seek_breakup_prcnt > 100) {
            sprintf(msg, "line# %d %s = %u is incorrect.\nIt must be >= 0 and <= 100.", *line, keywd, tmplt->seek_breakup_prcnt);
            user_msg(ps, 0, 0, SOFT, msg);
            return -1;
        }
    } else if (strcasecmp(keywd, "OPER") == 0) {
        sscanf(str, "%*s %s", tmplt->oper);
    }
    return 0;
}

