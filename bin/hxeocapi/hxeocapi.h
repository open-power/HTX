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
#include <errno.h>
#include <sys/mman.h>
#include "memcpy.h"
/*
 * Default stanza fields values
 */
#define DEFAULT_TEST_CASE           MEMCOPY_COMMAND_COPY
#define DEFAULT_BUFVAL_VALUE        CACHELINESIZE
#define DEFAULT_CMP_VALUE           TRUE
#define DEFAULT_NUM_THREADS         0x20
#define DEFAULT_NUM_OPER            100
#define DEFAULT_COMPLETION_METHOD   POLLING
#define DEFAULT_COMPLETION_TIMEOUT  10

#define MAX_STRING       128
#define MAX_STANZA       32
#define MAX_THREADS      32
#define MAX_COPY_SIZE    2048
#define MAX_MSG_DUMP     20
#define MAX_MISCOMPARES  10

#define MEMCOPY_COMMAND_COPY                    0x0
#define MEMCOPY_COMMAND_INTERRUPT               0x1

#define POLLING    0x0
#define INTERRUPT  0x1

extern int num_rules_defined;
extern char verbose;

/* Thread context structure */
struct thread_context {
    char device[64];
    char id[32];
    int th_num;
    pthread_t tid;
    int32_t thread_join_rc;
    int num_oper;
    uint32_t command;
    uint32_t compare;
    char *src;
    char *dst;
    uint32_t completion_timeout;
    uint32_t completion_method;
    int bufsize;
    ocxl_afu_h afu_h;
};

/*
 * Rule info strcuture used to store parsed rule file data
 */
struct rule_info {
    char        rule_id[MAX_STRING];
    uint32_t    testcase:1;
    uint32_t    compare:1;
    uint32_t    num_threads;
    uint32_t    bufsize;
    uint32_t    num_oper;
    uint32_t    completion_timeout;
    uint32_t    completion_method;
};
struct rule_info rule_data[MAX_STANZA];

int global_setup (void);
void * tc_worker_thread(void * thread_context);
int test_afu_memcpy (struct thread_context *tctx, struct htx_data *htx_d);
int compare_buffer (struct htx_data *, char *, char *, __u64);
void SIGTERM_hdl (int sig, int code, struct sigcontext *scp);
