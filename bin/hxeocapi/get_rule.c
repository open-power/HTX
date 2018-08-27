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

#include <ctype.h>
#include <hxihtx64.h>
#include "hxeocapi.h"

static int get_line(char *str, FILE *fp)
{
    char *s, str1[256];
    unsigned int str_len;
    int i;

    if (fgets(str, 256, fp) == NULL) {
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
            str[i] = toupper(str[i]);
        }
        return (str_len);
    } else {  /* blank line */
        return 1;
    }
}

void set_defaults (struct rule_info *rule_ptr)
{
    strcpy(rule_ptr->rule_id, " ");
    rule_ptr->testcase     =    DEFAULT_TEST_CASE;
    rule_ptr->bufsize      =    DEFAULT_BUFVAL_VALUE;
    rule_ptr->compare      =    DEFAULT_CMP_VALUE;
    rule_ptr->num_threads  =    DEFAULT_NUM_THREADS;
    rule_ptr->num_oper     =    DEFAULT_NUM_OPER;
    rule_ptr->completion_method = POLLING;
    rule_ptr->completion_timeout = DEFAULT_COMPLETION_TIMEOUT;
}

int read_rf(struct htx_data *htx_ds, char *rf_name)
{
    FILE *fp;
    char keywd[32], str[256], msg[MSG_TEXT_SIZE];
    int keywd_count =0, line=0, rc=0, str_len, error_no;
    char varstr[64], eof = 'N';
    struct rule_info *rule;

    /*********************************************/
    /****    Open rule file in read only mode ****/
    /*********************************************/
    if ((fp = fopen(rf_name, "r")) == NULL ) {
        error_no = errno;
        sprintf(msg, "Error opening file %s. Error no. set is %d.\n", rf_name, error_no);
        hxfmsg(htx_ds, error_no, HTX_HE_SOFT_ERROR, msg);
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
            hxfmsg(htx_ds, errno, HTX_HE_SOFT_ERROR, msg);
            return -1;
        }
        if (rc == -2) { /* Recieved end of file */
            eof = 'Y';
            fclose(fp);
        }
        if (rc == 0 || (rc == 1 && keywd_count == 0)) { /* Either comment or a blank line without any keywd read before */
            line = line + 1;
            continue;
        }
        str_len = rc;
        line = line + 1;
        if (str_len > 1) { /* Recieved some good data*/
            if (keywd_count == 0) {
                rule = &rule_data[num_rules_defined];
                set_defaults(rule);
            }
            sscanf(str, "%s", keywd);
            if (strcmp(keywd, "RULE_ID") == 0) {
                sscanf(str, "%*s %s", rule->rule_id);
                if (((strlen(rule->rule_id)) < 1) || ((strlen(rule->rule_id)) > MAX_STRING)) {
                    sprintf (msg, "line# %d %s = %s (Length must be 1 to 16 characters) \n", line, keywd, rule->rule_id);
                    hxfmsg (htx_ds, 0, HTX_HE_SOFT_ERROR, msg);
                    return -1;
                }
            } else if ((strcmp(keywd, "TESTCASE")) == 0 ) {
                sscanf(str, "%*s %s", varstr);
                if ((strcmp(varstr, "COPY")) == 0 ) {
                    rule->testcase = MEMCOPY_COMMAND_COPY;
                } else if ((strcmp(varstr, "INTERRUPT")) == 0 ) {
                    rule->testcase = MEMCOPY_COMMAND_INTERRUPT;
                } else {
                    sprintf(msg, "line# %d %s = %s (must be COPY or INTERRUPT) \n", line, keywd, varstr);
                    hxfmsg (htx_ds, 0, HTX_HE_SOFT_ERROR, msg);
                    return -1;
                }
            } else if ((strcmp(keywd, "BUFSIZE")) == 0) {
                sscanf (str, "%*s %d", &rule->bufsize);
                if(((int)rule->bufsize < 0) || (rule->bufsize > MAX_COPY_SIZE) ) {
                    sprintf (msg,  "line# %d  %s bufsize = %d (must be >= 0 and <= %d) \n", line, keywd, rule->bufsize, MAX_COPY_SIZE);
                    hxfmsg (htx_ds, 0, HTX_HE_SOFT_ERROR, msg);
                    return -1;
                }
            } else if ((strcmp(keywd, "COMPARE")) == 0 ) {
                sscanf(str, "%*s %s", varstr);
                if ((strcmp(varstr, "TRUE")) == 0 ) {
                    rule->compare = TRUE;
                } else if ((strcmp(varstr, "FALSE")) == 0 ) {
                    rule->compare = FALSE;
                } else {
                    sprintf (msg, "line# %d %s = %s (must be TRUE or FALSE) \n", line, keywd, varstr);
                    hxfmsg (htx_ds, 0, HTX_HE_SOFT_ERROR, msg);
                    return -1;
                }
            } else if ((strcmp(keywd, "NUM_OPER")) == 0 ) {
                sscanf (str, "%*s %d", &rule->num_oper);
                if ((int)rule->num_oper < 0 ) {
                    sprintf (msg, "line# %d %s = %d (must be >= 1 ) \n", line, keywd, rule->num_oper);
                    hxfmsg (htx_ds, 0, HTX_HE_SOFT_ERROR, msg);
                    return -1;
                }
            } else if ((strcmp(keywd, "NUM_THREADS")) == 0 ) {
                sscanf (str, "%*s %d", &rule->num_threads);
                if(rule->num_threads < 1 || rule->num_threads > MAX_THREADS) {
                    sprintf (msg, "line# %d %s = %d (must be >= 1 and <= 32)", line, keywd, rule->num_threads);
                    hxfmsg (htx_ds, 0, HTX_HE_SOFT_ERROR, msg);
                    return -1;
                }
            } else if ((strcmp(keywd, "COMPLETION_METHOD"))  == 0 ) {
                sscanf (str, "%*s %s", varstr);
                if (strcmp (varstr, "POLLING") == 0) {
                    rule->completion_method = POLLING;
                } else if (strcmp(varstr, "INTERRUPT") == 0) {
                    rule->completion_method = INTERRUPT;
                }
            } else if ((strcmp(keywd, "COMPLETION_TIMEOUT"))  == 0 ) {
                sscanf (str, "%*s %d", &rule->completion_timeout);
            } else if ((strcmp(keywd, "VERBOSE"))  == 0 ) {
                sscanf (str, "%*s %s", varstr);
                if ((strcmp(varstr, "YES")) == 0 ) {
                    verbose = 'Y';
                }
            }
            keywd_count++;
        } else if (keywd_count != 0) {
            num_rules_defined++;
            keywd_count = 0;
        }
    }
    return 0;
}
