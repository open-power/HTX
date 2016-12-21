/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "hxemem64.h"
#ifndef __HTX_LINUX__
#include <sys/vminfo.h>
#include <sys/processor.h>
#include <sys/thread.h>
#include <sys/systemcfg.h>
#include <sys/rset.h>
#include <stdlib.h>
#include <malloc.h>
#include "htx_nx.h"
#endif

#ifdef       _DR_HTX_
#include <sys/dr.h>
#endif

#ifdef __HTX_LINUX__
#include <sys/sysinfo.h>
#endif
/* Macro Definitions */

#define MAX_STANZAS			41	/* Only 40 stanzas can be used */

#define BIT_PATTERN_WIDTH	8	/* 8 BYTES wide */

#define BIND_TO_PROCESS		0
#define BIND_TO_THREAD		1
#define UNBIND_ENTITY		-1

#define UNSPECIFIED_LCPU	-1

#define MAX_RULE_LINE_SIZE	700	/* Inline with pattern text widths */

#define ADDR_PATTERN_IND	0xADD	/* put 0xADD default in bit_pattern */

#define BYTE_SIZE           8
#define MIN_SEG_SIZE        BYTE_SIZE

#define MIN_PATTERN_SIZE	8
#define MAX_PATTERN_SIZE	4096

#define LS_BYTE         	1		/* Load-Store Bytes */
#define LS_WORD         	4		/* Load-Store Words */
#define LS_DWORD         	8		/* Load-Store Double Words */

#define MIN_MEM_PERCENT     1
#define DEFAULT_MEM_PERCENT 70
#define MAX_MEM_PERCENT     99          /* Not implemented */

#define DEFAULT_DELAY		90

#define ADDR_PAT_SIGNATURE	0x414444525F504154	/* Hex equiv of "ADDR_PAT" */
#define RAND_PAT_SIGNATURE	0x52414E445F504154	/* Hex equiv of "RAND_PAT" */

#define NO_CPU_DR           0x0

#define MAX_SRADS   64

#define MAX_STRIDE_SZ 	4*KB

char  page_size[MAX_NUM_PAGE_SIZES][4]={"4K","64K","2M","16M","16G"};
struct rule_format r, stanza[MAX_STANZAS], *stanza_ptr;
char shm_max[50], shm_all[50];
unsigned int g_thread_count; /* thread count tracker to make stanza = 1 after mem allocation completion by all threads*/
int cpu_dr_set = -1;
pthread_mutex_t g_thread_count_lock;/*mutex lock to guard g_thread_count */
struct private_data priv;
struct memory_info mem_info;

int cpu_dr_tracker_count = NO_CPU_DR;
int physical_cpus[MAX_THREADS] = {[0 ... (MAX_THREADS- 1)] = -1} ;
volatile int logical_cpus[MAX_THREADS] = {[0 ... (MAX_THREADS- 1)] = -1} ;
extern int update_sys_detail_flag;
#ifdef __HTX_LINUX__
int update_srad_data_flag = 0;
#endif

#ifndef __HTX_LINUX__
volatile int mem_DR_done = 0;
void SIGCPUFAIL_handler (int sig, int code, struct sigcontext *scp);
#endif

int vrm_enabled = FALSE;
int AME_enabled = FALSE;

struct srad_info srad_info_array[MAX_SRADS];
struct htx_data stats;

int num_of_srads; /*has the number of srads value */
volatile int sig_flag; /* global flag to handle SIGRECONFIG handler between threads */

char bit_pattern[BIT_PATTERN_WIDTH];

enum lpage_implement {NONE		= 0,
                      BAT_TYPE	= 1,
                      GP_TYPE	= 2};


/*****************************************************************************
*  SUPPORT FUNCTIONS used in the MAIN                                         *
*****************************************************************************/
#ifdef    _DR_HTX_
	void SIGRECONFIG_handler (int sig, int code, struct sigcontext *scp);
	void NX_SIGRECONFIG_handler(dr_info_t *dr_info);
#endif

int read_cmd_line (int argc,char *argv[]);
void SIGTERM_hdl (int, int, struct sigcontext *);
#ifdef __HTX_LINUX__
void SIGUSR2_hdl(int, int, struct sigcontext *);
#endif
int get_lpage_type (void);
int get_supported_page_sizes (void);
int get_mem_config (void);
void get_pspace (void);
int run_aix_cmd ( char * cmd_name, char * result, int size);
int fill_pattern_details (int pi, char *str, int *line);
void free_pattern_buffers (void);
int read_rules (void);
void set_defaults (void);
int get_line ( char s[], int lim, FILE *fp);
int get_sonoras (void);
extern void gen_tlbie (void);
int allocate_buffers (void);
void re_initialize_global_vars (void);
void release_thread_memory(int ti);
void atexit_release_memory(void);
void report_shmstat (int  , char * );
int seg_comp_buffers_8 (int ti, int seg_num, int ps_index, int pass, int pi,
					    unsigned long seed);
int execute_stanza_case (void);
int fill_buffers (int ti);
int load_store_buffers (int);
int store_readim_buffers (int );
int n_stride(int );
int get_rule (int * line, FILE *fp);
void * run_test_operation (void *);
int create_n_run_test_operation (void);
int fill_segment_data (void);
int allocate_buffers (void);
int chek_if_ramfs(void);
int save_buffers (int ti, unsigned long rc, struct segment_detail sd,
                  int main_misc_count, unsigned long *seed_ptr, int pass,
				  int trap_flag, int pi);
int detailed_display();
int check_ame_enabled();
void test_L4_Rollover();

#ifdef MPSS_EXECUTE
    extern int execute_mpss_testcase ();
#endif
#ifdef EMPSS_EXECUTE
	extern int empss_testcase();
#endif

int fill_srad_data();

#ifndef __HTX_LINUX__
int mem_compress_decompress();
int  convert_64k(void *);
#endif

/* Assembly Routine declarations */
unsigned long mem_operation (int code, unsigned long count, char *buf,
                             char *pat, int tflag, struct segment_detail *,
                             struct rule_format *,unsigned long pl);

unsigned long rand_operation (int code, unsigned long count, char *buf,
                             char *pat, int tflag, struct segment_detail *,
                             struct rule_format *, unsigned long pl);

unsigned long pat_operation (int code, unsigned long count, char *buf,
                             char *pat, int tflag, struct segment_detail *,
                             struct rule_format *, unsigned long pl);
/* Assembly Routine Declarations End */

#ifndef __HTX_LINUX__
    int allocate_shm_with_specified_page_size (int ti,  int page_size_index);
    int fill_mem_info_data (void);
	struct time_real{
    	timebasestruct_t start;
    	timebasestruct_t finish;
   	 	double latency;
	};

	struct thread_parameters{
    	int thn;
    	unsigned long sz;
    	pthread_t tid;
    	struct time_real conv_time;
	} *thp;

	#pragma reg_killed_by trap

#else
    int allocate_shm_with_specified_page_size_linux (int ti,
													 int page_size_index);
    int fill_mem_info_data_linux (void);
    int do_trap_htx64 (unsigned long arg1,
                       unsigned long arg2,
                       unsigned long arg3,
                       unsigned long arg4,
                       unsigned long arg5,
                       unsigned long arg6);
    int bindprocessor (int What, int Who, int Where);
#endif

int dma_flag = 0; /* This flag is set if filesystem is diskless */
int base_pg_idx;
char *htxkdblevel;

/*****************************************************************************
*  MAIN: begin program main procedure                                        *
*****************************************************************************/
int main(int argc, char *argv[])
{

/* SIGTERM handler registering */
    sigemptyset(&(priv.sigvector.sa_mask));
    priv.sigvector.sa_flags = 0 ;
    priv.sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
    sigaction(SIGTERM, &priv.sigvector, (struct sigaction *) NULL);

#ifdef __HTX_LINUX__
/* Register SIGUSR2 for handling CPU hotplug */
	priv.sigvector.sa_handler = (void (*)(int)) SIGUSR2_hdl;
	sigaction(SIGUSR2, &priv.sigvector, (struct sigaction *) NULL);
#else
/* #ifdef    _DR_HTX_
    Register SIGRECONFIG handler */
    priv.sigvector.sa_handler = (void (*)(int)) SIGRECONFIG_handler;
    (void) sigaction(SIGRECONFIG, &priv.sigvector,
                     (struct sigaction *) NULL);

/* Register SIGCPUFIAL handler  */
	priv.sigvector.sa_handler = (void (*)(int)) SIGCPUFAIL_handler;
	(void) sigaction(SIGCPUFAIL, &priv.sigvector, (struct sigaction *) NULL);
#endif
    int retcode = atexit(atexit_release_memory);
    if(retcode != 0) {
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d] Error: Could not register for atexit!\n",__LINE__);
        return ( retcode );
    }

/* Read the command line parameters */
    read_cmd_line(argc,argv);

/* Store the pid of the process */
    priv.pid=getpid();

/* Initialize mutex variables */
    if ( pthread_mutex_init(&mem_info.tmutex,NULL) != 0) {
         displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"pthread init error (%d): %s\n",errno,strerror(errno));
    }
    mem_info.mutex = 1; /* flag set that means mutex variable exists*/

	if ( pthread_mutex_init(&g_thread_count_lock,NULL) != 0) {
		displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"pthread init error for g_thread_count_lock (%d): %s\n",errno,strerror(errno));
	}

/* set htx_data flag to make supervisor aware that we want to receive SIGUSR2 in case of hotplug add/remove */

/* Notify Supervisor of exerciser start */
#ifdef __HTX_LINUX__
	STATS_VAR_INIT(hotplug_cpu, 1)
#endif
	STATS_HTX_UPDATE(START)
	STATS_HTX_UPDATE(UPDATE)


    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"%s %s %s %s %d\n",stats.HE_name,stats.sdev_id,stats.run_type,\
                   priv.rules_file_name,priv.debug);

    if (priv.exit_flag == 1) {
        re_initialize_global_vars();
        exit(0);
    }
    if((!(strlen(stats.htx_log_dir))) || (!(strlen(stats.htx_exer_log_dir)))){
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"htx lib log direcories found to be htx_log_dir= %s and htx_exer_log_dir= %s, thus updating them to '/tmp/'\n",
            stats.htx_log_dir,stats.htx_exer_log_dir);
        strcpy(stats.htx_log_dir,"/tmp/");
        strcpy(stats.htx_exer_log_dir,"/tmp/");

    }


/* Check if AME is enabled on system */
#ifndef __HTX_LINUX__
	if ( check_ame_enabled() != 0 ) {
			exit(1);
	}
#endif

if ((read_rules()) < 0 ) {
		re_initialize_global_vars();
		exit(1);
	}

	/*
	 * Wait till the other exercisers start up before collecting mem statics
	 * Do this check only if run type is not standalone (OTH)
	 */
	if ((priv.standalone != 1) && (stanza[0].startup_delay) && stanza[0].operation != OPER_TLB) {
		displaym(HTX_HE_INFO, DBG_MUST_PRINT, "Waiting %d second(s), "
				 "while other exercisers allocate their memory.\n",
				 stanza[0].startup_delay);
		sleep(stanza[0].startup_delay);
	}

    if (priv.exit_flag == 1) {
        re_initialize_global_vars();
        exit(0);
    }
/* Checking if filesystem is on RAM if yes dma_flag would be set */
    chek_if_ramfs();

#ifndef __HTX_LINUX__
    fill_mem_info_data();
#else
    fill_mem_info_data_linux();
#endif

	if (priv.affinity_flag == 1) {
	if ((fill_srad_data()) != 0) {
		exit(1);
		}
	}

    do {

        STATS_VAR_INIT(test_id, 0)

        stanza_ptr=&stanza[0];
        display(HTX_HE_INFO,DBG_IMP_PRINT,"Before calling execute_ stanza_case(): rule_id = %s\n", \
                                                                                stanza_ptr->rule_id);
        while( strcmp(stanza_ptr->rule_id,"NONE") != 0)
        {
#ifndef __HTX_LINUX__
            /* Clear all bindings from the previous run, before the next */
            bindprocessor(BINDPROCESS, getpid(), PROCESSOR_CLASS_ANY);

			/* If mem_DR_done flag is set then recalculate memory details and fill all mem structures.*/
			if ( mem_DR_done ){
				/*fill_mem_info_data();
				if (priv.affinity_flag == 1) {
					if ((fill_srad_data()) != 0) {
						exit(1);
					}
				}
				*/
				re_initialize_global_vars();
				STATS_HTX_UPDATE(RECONFIG);
				mem_DR_done = 0;
				exit(0);
			}
#else
			if (update_srad_data_flag == 1) {
				if ((fill_srad_data()) != 0) {
					exit(1);
				}
				update_srad_data_flag = 0;
				displaym(HTX_HE_INFO, DBG_MUST_PRINT, "update_srad_data_flag: %d\n", update_srad_data_flag);
			}
#endif
            if (execute_stanza_case() != 0) {
                re_initialize_global_vars();
                exit(0);
            }
            if (priv.exit_flag == 1) {
                re_initialize_global_vars();
                exit(0);
            }
            stanza_ptr++;
        }
        /* For Standalone memory exerciser run don't call hxfupdate(FINISH.. */
        if (priv.standalone != 1) {
         STATS_HTX_UPDATE(FINISH)
        }

		if ((read_rules()) < 0 ) {
			re_initialize_global_vars();
			exit(1);
		}

    }while ( priv.standalone != 1); /* while run type is not standalone (OTH) */
    return(0);
} /* main function ends */

int execute_stanza_case(void)
{
    int res=0;

    int prev_debug_level; /* Variable to store the current debug level and restore it back at end */
    *(unsigned long *) (&bit_pattern[0]) = ADDR_PATTERN_IND; /* ADDRESS pattern
																marker */

    prev_debug_level = priv.debug; /* Store the present debug level */

    /* Set the debug level to the debug level specified in this stanza */
    if (stanza_ptr->debug_level) {
        priv.debug = stanza_ptr->debug_level;
    }

#ifdef CORSA_ENABLE
	if(stanza_ptr->operation == OPER_CORSA) {
			res = corsa_compress_decompress();
			if (res == -1 ) {
				priv.exit_flag = 1;
				return(-1);
			}
			return(res);
	}
#endif /* CORSA_ENABLE */

    /* In case of AIX 610 only */
    if(stanza_ptr->operation == OPER_NX_MEM) {
		#ifndef __HTX_LINUX__
            res = mem_compress_decompress();
            if (res == -1 ) {
                priv.exit_flag = 1;
                return(-1);
            }
            return(res);
        #else
           displaym(HTX_HE_INFO,DBG_MUST_PRINT,"OPER_NX_MEM id valid only for AIX versions \n");
		#endif
    }

      else if(stanza_ptr->operation == OPER_MPSS) {
        #ifdef MPSS_EXECUTE
            res = execute_mpss_testcase();
            if (res == -1 ) {
                priv.exit_flag = 1;
                return(-1);
            }
            return(res);
        #else
           displaym(HTX_HE_INFO,DBG_MUST_PRINT,"MPSS is only valid on  AIX 610 and above \n");
           return(-1);
        #endif
    }
	else if (stanza_ptr->operation == OPER_EMPSS) {
		#ifdef EMPSS_EXECUTE
			res = empss_testcase();
			if (res == -1) {
				priv.exit_flag = 1;
				return(-1);
			}
			return(res);
		#else
			displaym(HTX_HE_INFO,DBG_MUST_PRINT, "EMPSS testcase is only valid on P7 with EMPSS AIX support\n");
		#endif

	}
    else if (stanza_ptr->operation==OPER_TLB) {
		if(strcmp(stanza_ptr->tlbie_test_case,"RAND_TLB") == 0){
			tlbie_test_flag = 1;
		}
		else if(strcmp(stanza_ptr->tlbie_test_case,"SEQ_TLB")==0){
			tlbie_test_flag = 0;
		}
        gen_tlbie();
        if (priv.exit_flag == 1) {
            return(-1);
        }
        return(res);
    }
	else if (stanza_ptr->operation==OPER_L4_ROLL) {
		test_L4_Rollover();
        if (priv.exit_flag == 1) {
            return(-1);
        }
		return(res);
    }
	else if (stanza_ptr->operation==OPER_MBA) {
		test_L4_Rollover();
		if (priv.exit_flag == 1) {
			return(-1);
		}
		return(res);
	}
    else {
        displaym(HTX_HE_INFO, DBG_IMP_PRINT, "Starting execute_stanza_case "
                 "function, rule_id = %s, oper = %s\n", stanza_ptr->rule_id,
                 stanza_ptr->oper);
    }

    /* Initialize all pointers to NULL so that we don't have any previous or garbage values in them
    */
    re_initialize_global_vars();

    res = fill_segment_data();
	if (res == 1) {
		stanza_ptr->affinity = FALSE;
		res = 0;
		}

    if (res != 0) {
        re_initialize_global_vars();
        exit(1);
    }

    display(HTX_HE_INFO,DBG_INFO_PRINT,"Stanza_ptr->pattern_nm = %s\n",stanza_ptr->pattern_nm);

    /* Get the 8 byte bit pattern into the variable bit_pattern (not for ADDRESS pattern)
    if (strcmp(stanza_ptr->pattern_id,"ADDRESS") != 0 ) {
        if ((res=hxfpat(stanza_ptr->pattern_nm,(char *) &bit_pattern[0], 8) ) != 0) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"pattern fetching problem -error - %d",res);
            re_initialize_global_vars();
            exit(1);
        }
    }
    */
#if 0
    display(HTX_HE_INFO,DBG_IMP_PRINT, "Calling allocate_buffers , pattern fetched is 0x%llx\n",\
            *(unsigned long*)bit_pattern);

    res =  allocate_buffers();

    if (res != 0) {
        re_initialize_global_vars();
        exit(1);
    }
#endif


    display(HTX_HE_INFO,DBG_IMP_PRINT,"Calling create_n_run_test_operation\n");

	g_thread_count = mem_info.num_of_threads;
    res=  create_n_run_test_operation();

    displaym(HTX_HE_INFO, DBG_IMP_PRINT, "Stanza_ptr->rule_id = %s Completed\n",
             stanza_ptr->rule_id);
    re_initialize_global_vars();
    priv.debug = prev_debug_level; /* Restore the present debug level to previously stored value */
    return(res);
}

int fill_segment_data(void)
{
    int i=0,j=0,k=0, num_proc=0,ret, rc1=0, m=0, n=0;
    unsigned long free_pages=0;
    struct thread_data *tdata_hp=0;
	unsigned long max_mem = 0, radmem_free = 0, radmem_avail, free_4k_64k, th_free_4k_64k;
    int *num_pgs, num_pgs_per_seg, rem_pgs;
    int thr_index1,thr_index2,thr_num,srad_mem[num_of_srads],count = 0;
    char cmd[50];
    unsigned long total_no_segments = 0, new_shmmni;
    char shm_mni[20], oth_exer_segments[20];
    FILE *fp;
	void *tresult;
	int secs, n_secs;

    /* num_segs must not be unsigned */
    int num_segs;
#ifndef __HTX_LINUX__
	int early_lru = 1; /* 1 extends local affinity via paging space.*/
	int num_policies = VM_NUM_POLICIES;
	int policies[VM_NUM_POLICIES] = {/* -1 = don't change policy */
	-1, /* VM_POLICY_TEXT */
	-1, /* VM_POLICY_STACK */
	-1, /* VM_POLICY_DATA */
	P_FIRST_TOUCH, /* VM_POLICY_SHM_NAMED */
	P_FIRST_TOUCH, /* VM_POLICY_SHM_ANON */
	-1, /* VM_POLICY_MAPPED_FILE */
	-1, /* VM_POLICY_UNMAPPED_FILE */
	};
#endif

#ifdef  __HTX_LINUX__
long long hard_limt_mem_size = DEF_SEG_SZ_4K * (32 * KB); /* Linux has a hard limit of 32K for total number of shms (SHMNI)*/
long long new_seg_size		 = mem_info.total_mem_avail/(32 * KB);
#endif
    /*
     * If seg_size = 0 , the default value for segment size is 256
     * (for < 16G page segments)
     * for 16G page size segments default segment size is 16G
     */
    for(i = 0; i < MAX_NUM_PAGE_SIZES; i++) {
        if (stanza_ptr->seg_size[i] == 0 ) {
            if (i < PAGE_INDEX_16G) {
                stanza_ptr->seg_size[i]= DEF_SEG_SZ_4K; /* 256MB for < 16G */
#ifdef  __HTX_LINUX__
				if (mem_info.total_mem_avail > hard_limt_mem_size) {
					stanza_ptr->seg_size[i]= new_seg_size;
				}	
#endif
            } else {
                stanza_ptr->seg_size[i]= DEF_SEG_SZ_16G; /* 1 16GB page */
            }
        }
    }

    if (stanza_ptr->run_mode == RUN_MODE_NORMAL) {
        mem_info.num_of_threads = 1;
    } else {
        /* Get the number of processors in the partition */
        num_proc = get_num_of_proc();
        display(HTX_HE_INFO,DBG_IMP_PRINT,"number of logical processor = %d\n",
				num_proc);
        if ( stanza_ptr->num_threads == -1) {
            mem_info.num_of_threads = num_proc;
        } else {
            mem_info.num_of_threads = stanza_ptr->num_threads;
        }
    }

    display(HTX_HE_INFO,DBG_IMP_PRINT,"(T)number of threads= %d\n",
			mem_info.num_of_threads);

    /* Put NULL value in the thread data head pointer */
    mem_info.tdata_hp = NULL;

    /* Allocate 'mem_info.num_of_threads' number of "struct thread_data " */
	alocate_mem_for_mem_info_num_of_threads();
	if((unsigned long*)mem_info.tdata_hp == NULL) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
				 "fill_segment_data:(malloc failed) Creation of thread data "
				 "structures failed! errno = %d(%s)\n", errno, strerror(errno));
        return(-1) ;
    }

	tdata_hp = mem_info.tdata_hp;

    /* Allocate storage for temporarily storing page distribution */
    num_pgs = (int * ) malloc(mem_info.num_of_threads*(sizeof(int)));
    if (num_pgs == NULL) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
				 "fill_segment_data:(malloc failed) Creation of num_pgs space "
				 "failed! errno = %d(%s)\n", errno, strerror(errno));
        return(-1) ;
    }

    /* Making sure that if 64k pages are not supported for a machine then
    affinity feature is disabled sine AIX710 has some issues supporting the affinity for 4k pages*/

#ifndef __HTX_LINUX__
    if ((stanza_ptr->affinity == TRUE) && (mem_info.pdata[PAGE_INDEX_64K].supported != TRUE)) {
		base_pg_idx = PAGE_INDEX_4K;
	} else {
		base_pg_idx = PAGE_INDEX_64K;
	}
#endif


    for(i=0; i<MAX_NUM_PAGE_SIZES; i++) {
	/* Check for all supported page sizes */
	if (!mem_info.pdata[i].supported) {
	   continue;
	}

	if (stanza_ptr->max_mem_flag == TRUE ) {
	    if(stanza_ptr->affinity == FALSE) {
		if (i < PAGE_INDEX_2M) {
		    max_mem = (stanza_ptr->mem_percent/100.0) * mem_info.pdata[i].free;

	    } else {
				max_mem= mem_info.pdata[i].free;
	      }

            free_pages = max_mem/mem_info.pdata[i].psize;

            /* Distribute pages evenly */
            for(j=0; j< mem_info.num_of_threads; j++) {
				num_pgs[j] = free_pages/mem_info.num_of_threads;
			}

            /*
             * Find out the pages that remain unallocated
             * Remaining pages will always be less than num thr
             * Allocate one additional page to the 1st <rem_pgs> pages.
             */
			rem_pgs = free_pages%mem_info.num_of_threads;
            for(j=0; j<rem_pgs; j++) {
				num_pgs[j]++;
			}

			/* Find out how many pages would the required seg size need */
			num_pgs_per_seg = stanza_ptr->seg_size[i]/mem_info.pdata[i].psize;

			/*
			 * Now that we have the best allocation of pages across threads,
			 * Calculate segment details
			 */
            for(j=0; j<mem_info.num_of_threads; j++) {
				tdata_hp[j].thread_num = j;
				tdata_hp[j].page_wise_t[i].page_size = mem_info.pdata[i].psize;

				/*
				 * Stop if we hit a thread index with number of pages = 0
				 * Further threads will be allocated no pages.
				 */
				if (!num_pgs[j]) {
					break;
				}

				/* Prefer creation of seg size indicated by stanza */
				tdata_hp[j].page_wise_t[i].num_of_segments =
					num_pgs[j]/num_pgs_per_seg;

				rem_pgs = num_pgs[j]%num_pgs_per_seg;
				if (rem_pgs) {
					tdata_hp[j].page_wise_t[i].num_of_segments++;
				}
				if (!tdata_hp[j].page_wise_t[i].num_of_segments) {
					continue;
				}

				/* Allocate storage for shm size attributes */
#ifndef MMAP
				tdata_hp[j].page_wise_t[i].shm_sizes = (unsigned long *) malloc(sizeof(unsigned long)*tdata_hp[j].page_wise_t[i].num_of_segments);
				if ((unsigned long*)tdata_hp[j].page_wise_t[i].shm_sizes == NULL) 
#else
				tdata_hp[j].page_wise_t[i].shm_sizes =(unsigned long *) mmap(NULL,(sizeof(unsigned long)*tdata_hp[j].page_wise_t[i].num_of_segments),PROT_READ | PROT_WRITE | PROT_EXEC,MAP_ANONYMOUS,-1, 0);
				if ((unsigned long)tdata_hp[j].page_wise_t[i].shm_sizes == -1) 
#endif
				{
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
							 "fill_thread_data:(malloc/mmap failed) Creation of"
							 "shmsizes struct in thread data structures "
							 "failed! errno = %d(%s)\n", errno,strerror(errno));
					free(num_pgs);
                    return(-1);
                }

				total_no_segments+= tdata_hp[j].page_wise_t[i].num_of_segments;
				/*
				 * If number segs is 0, num_segs is negative
				 * In that case the loop below is skipped
				 */
				num_segs = tdata_hp[j].page_wise_t[i].num_of_segments -
							(!(!rem_pgs));
				/* If num_segs turns out to be -ve, the loop below is skipped */
				for(k=0; k<num_segs; k++) {
					tdata_hp[j].page_wise_t[i].shm_sizes[k] = stanza_ptr->seg_size[i];
				}
				/*
				 * Preserve value of K from above loop
				 * in case another segment of smaller size is possible
				 */
				if (rem_pgs) {
					tdata_hp[j].page_wise_t[i].shm_sizes[k] = rem_pgs * mem_info.pdata[i].psize;
				}

                for (k=0; k< tdata_hp[j].page_wise_t[i].num_of_segments; k++) {
                     if (stanza_ptr->operation == OPER_DMA) {
                        tdata_hp[j].page_wise_t[i].shm_sizes[k]=
                            (tdata_hp[j].page_wise_t[i].shm_sizes[k]/LINUX_DMA_BLOCK_SIZE);
                        tdata_hp[j].page_wise_t[i].shm_sizes[k]=
                            tdata_hp[j].page_wise_t[i].shm_sizes[k]*LINUX_DMA_BLOCK_SIZE;
                     }
                     display(HTX_HE_INFO,DBG_MUST_PRINT,"For thread %d,page size (%s),seg_num(%d)"
                             " seg_size = 0x%llx \n",j,page_size[i],k,\
                            tdata_hp[j].page_wise_t[i].shm_sizes[k]);
                }
#ifdef MMAP
				ret = mprotect(tdata_hp[j].page_wise_t[i].shm_sizes,(sizeof(unsigned long)*tdata_hp[j].page_wise_t[i].num_of_segments),PROT_READ | PROT_EXEC);
				if(ret !=0 ) {
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]Error (%d) mprotect() failed with rc = %d thread= %d page_index=%d num_segs=%d ptr=%p \n",
						__LINE__,errno,ret,j,i,tdata_hp[j].page_wise_t[i].num_of_segments,tdata_hp[j].page_wise_t[i].shm_sizes);
				}
#endif
            }/* end for (j=0; j< mem_info.num_of_threads; j++) */

        }/* END of affinity == FALSE */

        else {
	    /*  This means  stanza_ptr->affinity == TRUE */
            displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Am inside affinity condition \n");
#ifndef __HTX_LINUX__

	    rc1 = system("vmo -o enhanced_affinity_vmpool_limit=-1");
	    if(rc1 < 0){
	        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"vmo -o enhanced_affinity_vmpool_limit=-1"
		"failed errno %d rc1 = %d\n",errno,rc1);
		return(-1);
	    }
	    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"vmo -o enhanced_affinity_vmpool_limit=-1 rc1 = %d,errno =%d\n",rc1,errno);

		/* To get local memory set flag early_lru=1 to select P_FIRST_TOUCH policy(similar to setting MEMORY_AFFINITY environment variable to MCM)*/
		rc1 = vm_mem_policy(VM_SET_POLICY,&early_lru, &policies ,num_policies);
		if (rc1 != 0){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"vm_mem_policy() call failed with return value = %d\n",rc1);
		}	

	    thp = (struct thread_parameters *) malloc(mem_info.num_of_threads*(sizeof(struct thread_parameters)));
   		if(thp == NULL){
       		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
        	"fill_segment_data:(malloc failed) Creation of thread data "
        	"structures failed! errno = %d(%s)\n", errno, strerror(errno));
        	return(-1) ;
    	}


#endif

	if (i == base_pg_idx) {
		/* Here we are marking which cpu's are actually have the memory in their SRAD */
		thr_num=0;
		for(n = 0; n < num_of_srads; n++){
         	    srad_mem[n] = 1;
    		    if(!srad_info_array[n].total_srad_mem_free){
			        srad_mem[n] = 0;
					thr_num += srad_info_array[n].cpuidx;	
    		    }
				else {
	            	for(m = 0; m < srad_info_array[n].cpuidx; m++) {
						
		        		tdata_hp[thr_num].cpu_flag = 1;
        	     		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"fill_segment_data: for srad:%d thr_num = %d  \t tdata_hp[%d].cpu_flag"
						" = %d and m=%d\n",n,thr_num,thr_num,tdata_hp[thr_num].cpu_flag,m);
						thr_num++;
                	}
                }
		}

	     /*Here we are checking if the system has number of cpus for a srad is zero and memory in that srad is zero */
		count = 0;
		for(n = 0; n < num_of_srads; n++){
    		    if((srad_mem[n] == 0) && (srad_info_array[n].cpuidx == 0)){
			count++;
    		    }
		}

		if(count == num_of_srads){
    		    displaym(HTX_HE_INFO,DBG_INFO_PRINT,"In some SRAD's number of CPU's are zero"
    		    "and in some SRAD memory seems to be zero, Hence this rule file parameter affinity is over written to 'yes'(memmfg.p7.nodelay)\n"
    		    "count = %d \t num_of_srads = %d\n",count,num_of_srads);
		    return 1;
		}


		thr_num=0;
		thr_index1=0;
		thr_index2=0;
		for(n = 0; n < num_of_srads; n++){
    		radmem_free = srad_info_array[n].total_srad_mem_free;
		    radmem_avail = srad_info_array[n].total_srad_mem_avail;
            if(!radmem_avail) {
                displaym(HTX_HE_INFO,DBG_INFO_PRINT,"fill_segment_data():memory behind node %d is %d\n",n,radmem_avail);
                thr_num = thr_num + srad_info_array[n].cpuidx;
				thr_index1 = thr_index1 + srad_info_array[n].cpuidx;
				continue;
            }
		    max_mem = (stanza_ptr->mem_percent/100.0) * radmem_free;
		    free_pages = max_mem/mem_info.pdata[i].psize;

		    displaym(HTX_HE_INFO,DBG_INFO_PRINT,"in srad%d \t free_memory = %ld\t  max_mem = %ld \t"
		    "free_pages = %ld, segment size = %d \t page size = %d\n",n,radmem_free,max_mem,free_pages,
		    stanza_ptr->seg_size[i],mem_info.pdata[i].psize);

		   /*If number of cpu's in a SRAD is 0 then skip that srad and continue to next SRAD */
		    if(!srad_info_array[n].cpuidx){
			continue;
		    }

            thr_index2 = thr_index1;
		    /* Distribute pages evenly for all the threads in this srad */
		    for(m = 0; m < srad_info_array[n].cpuidx; m++){
		        num_pgs[thr_index1++] = free_pages/srad_info_array[n].cpuidx;
		    }

                    /*
                     * Find out the pages that remain unallocated
                     * Remaining pages will always be less than num thr
                     * Allocate one additional page to the 1st <rem_pgs> pages.
                     */
		     rem_pgs = free_pages % srad_info_array[n].cpuidx;
		     for(m = 0; m < rem_pgs; m++) {
			    num_pgs[thr_index2++]++;
		     }

       	             /* Find out how many pages would the required seg size need */
                     num_pgs_per_seg = stanza_ptr->seg_size[i]/mem_info.pdata[i].psize;

                     /*
                      * Now that we have the best allocation of pages across threads,
                      * Calculate segment details
	              */
		     for(m = 0; m < srad_info_array[n].cpuidx; m++) {
			 	tdata_hp[thr_num].thread_num = thr_num;
			 	tdata_hp[thr_num].page_wise_t[i].page_size = mem_info.pdata[i].psize;

                         /*
                          * Stop if we hit a thread index with number of pages = 0
                          * Further threads will be allocated no pages.
                          */
                         if (!num_pgs[thr_num]) {
                              break;
                         }

                         /* Prefer creation of seg size indicated by stanza */
                         tdata_hp[thr_num].page_wise_t[i].num_of_segments =
                         num_pgs[thr_num]/num_pgs_per_seg;

                         rem_pgs = num_pgs[thr_num]%num_pgs_per_seg;
                         if (rem_pgs) {
                             tdata_hp[thr_num].page_wise_t[i].num_of_segments++;
                         }
                         if (!tdata_hp[thr_num].page_wise_t[i].num_of_segments) {
                             continue;
                         }
                         /* Allocate storage for shm size attributes */
					#ifndef MMAP
                         tdata_hp[thr_num].page_wise_t[i].shm_sizes = (unsigned long *)
				 			malloc(sizeof(unsigned long)*tdata_hp[thr_num].page_wise_t[i].num_of_segments);
                         if ((unsigned long*)tdata_hp[thr_num].page_wise_t[i].shm_sizes == NULL) 
					#else
						tdata_hp[thr_num].page_wise_t[i].shm_sizes = (unsigned long *)
						mmap(NULL,(sizeof(unsigned long)*tdata_hp[thr_num].page_wise_t[i].num_of_segments),PROT_READ | PROT_WRITE | PROT_EXEC,MAP_ANONYMOUS,-1, 0);
						if ((unsigned long)tdata_hp[thr_num].page_wise_t[i].shm_sizes == -1) 
					#endif
						{
					
                              displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
                                      "fill_thread_data:(malloc failed) Creation of"
                                      "shmsizes struct in thread data structures "
                                      "failed! errno = %d(%s)\n", errno,strerror(errno));

                               free(num_pgs);
                  	       return(-1);
                         }


		         total_no_segments+= tdata_hp[thr_num].page_wise_t[i].num_of_segments;

                         /*
                          * If number segs is 0, num_segs is negative
                          * In that case the loop below is skipped
                          */
                         num_segs = tdata_hp[thr_num].page_wise_t[i].num_of_segments -
                                                        (!(!rem_pgs));
                         /* If num_segs turns out to be -ve, the loop below is skipped */
                         for(k = 0; k < num_segs; k++) {
                              tdata_hp[thr_num].page_wise_t[i].shm_sizes[k] = stanza_ptr->seg_size[i];
                         }

                         /*
                          * Preserve value of K from above loop
                          * in case another segment of smaller size is possible
                          */
                         if(rem_pgs) {
                             tdata_hp[thr_num].page_wise_t[i].shm_sizes[k] = rem_pgs * mem_info.pdata[i].psize;
                         }

			 for(k = 0; k < tdata_hp[thr_num].page_wise_t[i].num_of_segments; k++) {
                	     if(stanza_ptr->operation == OPER_DMA) {
                                 tdata_hp[thr_num].page_wise_t[i].shm_sizes[k]=
                             	 (tdata_hp[thr_num].page_wise_t[i].shm_sizes[k]/LINUX_DMA_BLOCK_SIZE);
                        	 tdata_hp[thr_num].page_wise_t[i].shm_sizes[k]=
                            	 tdata_hp[thr_num].page_wise_t[i].shm_sizes[k]*LINUX_DMA_BLOCK_SIZE;
                     	     }
			 }

 		        displaym(HTX_HE_INFO,DBG_INFO_PRINT,"thread no = %d \t num_pgs = %d \t number_of_segments = %d \t"
			 "shm_size[FIRST]=%d \t shm_size[LAST]= %d\n",thr_num,num_pgs[thr_num],
			 tdata_hp[thr_num].page_wise_t[i].num_of_segments,tdata_hp[thr_num].page_wise_t[i].shm_sizes[0],
			 tdata_hp[thr_num].page_wise_t[i].shm_sizes[k-1]);

#ifdef MMAP
				ret = mprotect(tdata_hp[thr_num].page_wise_t[i].shm_sizes,(sizeof(unsigned long)*tdata_hp[thr_num].page_wise_t[i].num_of_segments),PROT_READ | PROT_EXEC);
                if(ret !=0 ) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]Error (%d) mprotect() failed with rc = %d for thread=%d at ptr=%p "
						"and length=%d\n",__LINE__,errno, ret,thr_num,tdata_hp[thr_num].page_wise_t[i].shm_sizes,(sizeof(unsigned long)*tdata_hp[thr_num].page_wise_t[i].num_of_segments));
                }
#endif
				thr_num++;

	   	     }/*END of cpu loop */
                 }/*END of srads loop */

            }/*END of PAGE_INDEX_64K */

       }/* END of stanza_ptr->affinity == TRUE */

      }/*END of stanza_ptr->max_mem_flag == TRUE */
        else if ( mem_info.pdata[i].supported ) { /* max_mem = no case */
            /* Put the values what ever specified in the rules file */
            for (j=0; j< mem_info.num_of_threads; j++) {
		mem_info.tdata_hp[j].thread_num=j;
                /* Fill number of segments for this page size and the page size value */
                tdata_hp[j].page_wise_t[i].num_of_segments = stanza_ptr->num_seg[i];
                tdata_hp[j].page_wise_t[i].page_size=mem_info.pdata[i].psize;
                     display(HTX_HE_INFO,DBG_INFO_PRINT,"For thread %d , page size (%s) , "
                         "number of segments =%d\n", j,page_size[i], \
                         tdata_hp[j].page_wise_t[i].num_of_segments);
				if (!(tdata_hp[j].page_wise_t[i].num_of_segments)) {
					continue;
				}
                /* Allocate memory for storing the segment sizes of each segment */
#ifndef MMAP
                tdata_hp[j].page_wise_t[i].shm_sizes=(unsigned long *) \
                       malloc(sizeof(unsigned long)*tdata_hp[j].page_wise_t[i].num_of_segments);
                if ((unsigned long*)tdata_hp[j].page_wise_t[i].shm_sizes == NULL) {
#else
				tdata_hp[j].page_wise_t[i].shm_sizes=(unsigned long *) \
						mmap(NULL,(sizeof(unsigned long)*tdata_hp[j].page_wise_t[i].num_of_segments),PROT_READ | PROT_WRITE | PROT_EXEC,MAP_ANONYMOUS,-1, 0);
				if ((unsigned long)tdata_hp[j].page_wise_t[i].shm_sizes == -1) {
#endif
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,":(malloc/mmap failed) Creation of"
                              "shmsizes struct in thread data structures failed! for th=%d,page_size_index=%d,num_segs=%d errno = %d(%s)\n",\
                              j,i,tdata_hp[j].page_wise_t[i].num_of_segments,errno,strerror(errno));
                    return(-1);
                }
                /* Fill up the segment size for each segment of this page size */
                for (k=0; k< tdata_hp[j].page_wise_t[i].num_of_segments; k++) {
                    tdata_hp[j].page_wise_t[i].shm_sizes[k] = stanza_ptr->seg_size[i];
                }
#ifdef MMAP
                ret = mprotect(tdata_hp[j].page_wise_t[i].shm_sizes,(sizeof(unsigned long)*tdata_hp[j].page_wise_t[i].num_of_segments),PROT_READ | PROT_EXEC);
                if(ret !=0 ) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]Error (%d) mprotect() failed with rc = %d for thread=%d at ptr=%p "
                        "and length=%d\n",__LINE__,errno, ret,j,tdata_hp[j].page_wise_t[i].shm_sizes,(sizeof(unsigned long)*tdata_hp[j].page_wise_t[i].num_of_segments));
                }
#endif			
            }/* end for (j=0; j< mem_info.num_of_threads; j++) */
        }/* end of max_mem = no case */
    }/* end for (i = 0; i < MAX_NUM_PAGE_SIZES; i++) */

	detailed_display();
	free(num_pgs);

	#ifdef __HTX_LINUX__
	fp = popen("cat /proc/sys/kernel/shmmni","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
		return(-1);
	}
	ret  = fscanf(fp,"%s",shm_mni);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "Could not read page size config from pipe\n");
		return(-1);
	}
	pclose(fp);

	fp = popen("ipcs -m | grep root | wc -l", "r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
						"popen failed for ipcs command: errno(%d)\n", errno);
		return(-1);
	}
	ret  = fscanf(fp,"%s",oth_exer_segments);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
						"Could not read oth_exer_segments from pipe\n");
		return(-1);
	}
	pclose(fp);

	if (strtoul(shm_mni,NULL,10) < strtoul(oth_exer_segments, NULL, 10) + total_no_segments) {
		new_shmmni = strtoul(shm_mni,NULL,10) + strtoul(oth_exer_segments, NULL, 10) + total_no_segments;
    	displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"Changing the /proc/sys/kernel/shmmni variable to %lu\n",new_shmmni);
		sprintf(cmd,"echo %lu > /proc/sys/kernel/shmmni", new_shmmni);
    	ret = system (cmd);
    	if (ret < 0){
        	displaym(4,DBG_MUST_PRINT,"unable to change the /proc/sys/kernel/shmmni variable\n");
   		}
	}
    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"shmmni: %llu \n", strtoul(shm_mni,NULL,10));
	#endif
    return(0);
}/* end of function fill_segment_data */

void * run_test_operation(void *tn)
{
    int ti= *(int *)tn;
    int rc=0,bind_cpu_num;
    int bind_flag = FALSE;
	int i,j;
    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Thread(%d):Inside run_test_operation\n",ti);
    if (stanza_ptr->bind_proc == 1 && stanza_ptr->bind_method == 0){
		if (stanza_ptr->run_mode == RUN_MODE_NORMAL){
            if (priv.bind_proc != -1) {
                bind_cpu_num = priv.bind_proc;
                bind_flag    = TRUE;
            }
        } else {
            bind_cpu_num = ti;
            bind_flag    = TRUE;
        }
    }
	if ((stanza_ptr->run_mode == RUN_MODE_CONCURRENT) && (bind_flag == TRUE)) {
#ifndef __HTX_LINUX__
		if(cpu_dr_set != ti)
#endif
       		 do_the_bind_proc(BIND_TO_THREAD,ti,-1);
    }

	for(i = 0; i < MAX_NUM_PAGE_SIZES; i++) {
		if (mem_info.pdata[i].supported && mem_info.tdata_hp[ti].page_wise_t[i].num_of_segments) {
		#ifndef __HTX_LINUX__
            if ( allocate_shm_with_specified_page_size(ti,i) != 0)
		#else
            if ( allocate_shm_with_specified_page_size_linux(ti,i) != 0)
		#endif
            {
				displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Memory allocation failed for thread %d,exiting thread\n",ti);
				release_thread_memory(ti);
				pthread_exit((void *)1);
            }
        }
	}  

	pthread_mutex_lock(&g_thread_count_lock);

	g_thread_count--;
	
	/*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"####Thread:%d count = %d \n",ti,g_thread_count);*/
	if(g_thread_count == 0){
		stats.test_id++; 
		hxfupdate(UPDATE,&stats);	
	}
	pthread_mutex_unlock(&g_thread_count_lock);

    if (stanza_ptr->run_mode == RUN_MODE_CONCURRENT) {
        do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
    }


    if (bind_flag) {
#ifndef __HTX_LINUX__
		if(cpu_dr_set != ti){
#endif
	        rc = do_the_bind_proc(BIND_TO_THREAD, bind_cpu_num,-1);
			if ( rc < 0 && stanza_ptr->affinity == TRUE) {
				displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread:%d failed to bind on lcpu= %d thus exiting in affinity=yes case\n",ti,bind_cpu_num);
				release_thread_memory(ti);
            	pthread_exit((void *)0);
			}
#ifndef __HTX_LINUX__
		}
#endif
    } else {
        mem_info.tdata_hp[ti].bind_proc = UNBIND_ENTITY;
    }

	display(HTX_HE_INFO,DBG_IMP_PRINT,"memory allocation and setup is done for thread %d\n",ti);

    if ( stanza_ptr->operation == OPER_DMA) {
 	   if (dma_flag == 1) {
   			 display(HTX_HE_INFO, DBG_IMP_PRINT,"run_test_oper:OPER_DMA case: dma_flag = %d,exiting thread b4 dma call\n",dma_flag);
		    	release_thread_memory(ti);
                pthread_exit((void *)0);
       	   }
        display(HTX_HE_INFO,DBG_IMP_PRINT,"Calling fill_buffers \n");
        if (fill_buffers(ti) != 0) {
			release_thread_memory(ti);
            pthread_exit((void *)1);
        }
        display(HTX_HE_INFO,DBG_DEBUG_PRINT,"Returned from fill_buffers \n");
    } else {
        if ( stanza_ptr->operation == OPER_MEM) {
            display(HTX_HE_INFO,DBG_IMP_PRINT,"Calling load_store_buffers \n");
            if (load_store_buffers(ti) != 0) {
				release_thread_memory(ti);
                pthread_exit((void *)1);
            }
        } else {
            if ( stanza_ptr->operation == OPER_RIM) {
                display(HTX_HE_INFO,DBG_IMP_PRINT,"Calling store_readim_buffers \n");
                if ( store_readim_buffers(ti) != 0) {  /* store and rd immediate*/
					release_thread_memory(ti);
                    pthread_exit((void *)1);
                }
            }
            else {
				if (stanza_ptr->operation == OPER_NSTRIDE) {
					display(HTX_HE_INFO,DBG_IMP_PRINT,"Calling n_stride \n");
					if (n_stride(ti) != 0) {
						release_thread_memory(ti);
						pthread_exit((void *)1);
					}
				}
            	else {
                	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"The stanza's (%s) oper(%s) field"
                    	"is invalid.Please check the oper field \n", stanza_ptr->rule_id,stanza_ptr->oper);
					release_thread_memory(ti);
                    pthread_exit((void *)1);
			   	}
            }
        }
    }
	release_thread_memory(ti);
    pthread_exit((void *)0);
}

int create_n_run_test_operation(void)
{
    int i,res;
    void *tresult;
    int tnum;
    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"create_n_run_test_operation :num_of_threads = %d\n",mem_info.num_of_threads);
	for(i=0; i< mem_info.num_of_threads ; i++) {
	    /* In case of affinity = yes check mem_info.tdata_hp for cpu_flag to make sure we are not accessing any holes */
    	if(stanza_ptr->affinity == TRUE) {
        	if(mem_info.tdata_hp[i].cpu_flag != 1){
				pthread_mutex_lock(&g_thread_count_lock);
				g_thread_count--;
				pthread_mutex_unlock(&g_thread_count_lock);
            	continue;
        	}
    	}

        tnum = mem_info.tdata_hp[i].thread_num;
        tresult=(void *)&tnum;
        display(HTX_HE_INFO,DBG_IMP_PRINT,"Creating thread %d\n",tnum);
        res=pthread_create((pthread_t *)&mem_info.tdata_hp[i].tid,NULL,\
                        run_test_operation,\
                        &mem_info.tdata_hp[i].thread_num);
        if ( res != 0) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"pthread_create "
                "failed(errno %d):(%s): tnum=%d\n",errno,strerror(errno),i);
            return(res);
        }
    }
    for(i=0; i< mem_info.num_of_threads; i++) {

        /* In case of affinity = yes check mem_info.tdata_hp for cpu_flag to make sure we are not accessing any holes */
        if(stanza_ptr->affinity == TRUE) {
            if(mem_info.tdata_hp[i].cpu_flag != 1){
                continue;
            }
        }

        res=pthread_join(mem_info.tdata_hp[i].tid,&tresult);
        if ( res != 0) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"pthread_join "
                "failed(errno %d):(%s): tnum=%d\n",errno,strerror(errno),i);
            return(res);
        }
        display(HTX_HE_INFO,DBG_IMP_PRINT,"Thread %d Just Joined\n" ,i);
        mem_info.tdata_hp[i].thread_num= -1;
        mem_info.tdata_hp[i].tid = -1;

    }
    return(0);
}

int get_num_of_proc(void)
{
#ifndef __HTX_LINUX__
    priv.cpus_avail=_system_configuration.ncpus;
#else
	priv.cpus_avail=get_nprocs();
#endif

    if (cpu_dr_tracker_count) {
        /* DR is in progress, do not bind to CPUs under DR */
        return(priv.cpus_avail - cpu_dr_tracker_count);
    } else {
        return(priv.cpus_avail);
    }
}

int read_rules(void)
{
    struct stat fstat;
    int i =0;
    int line_number=0;

    display(HTX_HE_INFO,DBG_INFO_PRINT,"#1 read-rule\n");

    if ( -1 ==  stat (priv.rules_file_name, &fstat)) {
        displaym(HTX_HE_INFO,DBG_MUST_PRINT," read_rules: Error occoured while "
                "getting the rules file stats: errno %d: %s \n",errno,strerror(errno));
    }
    else {
        if (priv.rf_last_mod_time != 0) {
            if (priv.rf_last_mod_time == fstat.st_mtime) {
                display(HTX_HE_INFO,DBG_INFO_PRINT," No changes in the rules "
                        "file stanza so skipping the read again \n");
                return(0);
            }
        }
        if (priv.rf_last_mod_time) {
            free_pattern_buffers();
        }
        priv.rf_last_mod_time = fstat.st_mtime;
    } /* else ends */

    if ( (priv.rf_ptr= fopen(priv.rules_file_name,"r")) == NULL) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"The rules file name parameter to the hxemem64 binary is incorrect,fopen(),errno:%d\n",errno);
        return(-1);
    }

    i = 0;
    stanza_ptr=&stanza[0];
    while ( (get_rule(&line_number,priv.rf_ptr)) != EOF ) {
        stanza_ptr++;
        i++;
        if ( i == MAX_STANZAS-2 ) {
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"The number of stanzas is more than expected (MAX_STANZAS)) \n");
        }
    }

    strcpy(stanza_ptr->rule_id,"NONE");

    stanza_ptr=&stanza[0];
    i = 0;

    while( strcmp(stanza_ptr->rule_id,"NONE") != 0)
    {
    int pati,patj;
        i++;
        displaym(HTX_HE_INFO,DBG_INFO_PRINT,"stanza number #%d,\nrule_id=%s\n",i,stanza_ptr->rule_id);
        for (pati=0; pati<stanza_ptr->num_patterns; pati++) {
            display(HTX_HE_INFO,DBG_INFO_PRINT,"pattern name = %s\n "
                    "pattern_size =%d\npattern =0x",stanza_ptr->pattern_name[pati],\
                    stanza_ptr->pattern_size[pati]);
            for (patj=0; patj<r.pattern_size[pati]; patj++) {
                display(HTX_HE_INFO,DBG_INFO_PRINT,"%0x",r.pattern[pati][patj]);
            }
            display(HTX_HE_INFO,DBG_INFO_PRINT,"\n");
        }
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->pattern_id=%s\n",stanza_ptr->pattern_id);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->oper=%s\n",stanza_ptr->oper);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->messages=%s\n",stanza_ptr->messages);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->compare=%s\n",stanza_ptr->compare);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->max_mem=%s\n",stanza_ptr->max_mem);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->crash_on_mis=%s\n",stanza_ptr->crash_on_mis);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->turn_attn_on=%s\n",stanza_ptr->turn_attn_on);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->mcheck_rfile=%s\n",stanza_ptr->mcheck_rfile);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_oper=%d\n",stanza_ptr->num_oper);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_writes=%d\n",stanza_ptr->num_writes);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_reads=%d\n",stanza_ptr->num_reads);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_compares=%d\n",stanza_ptr->num_compares);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_seg_4k=%d\n",stanza_ptr->num_seg[0]);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_seg_64k=%d\n",stanza_ptr->num_seg[1]);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_seg_16m=%d\n",stanza_ptr->num_seg[2]);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_seg_16g=%d\n",stanza_ptr->num_seg[3]);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->seg_size_4k=0x%llx\n",stanza_ptr->seg_size[0]);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->seg_size_64k=0x%llx\n",stanza_ptr->seg_size[1]);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->seg_size_16m=0x%llx\n",stanza_ptr->seg_size[2]);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->seg_size_16g=0x%llx\n",stanza_ptr->seg_size[3]);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->width=%d\n",stanza_ptr->width);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->debug_level=%d\n",stanza_ptr->debug_level);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->mem_percent=%d\n",stanza_ptr->mem_percent);
        display(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->mcheck_mode=%d\n",stanza_ptr->mcheck_mode);
        stanza_ptr++;
    } /* while (strcmp(.... ENDS */

     return(0);
} /* read_rules function ends */

void free_pattern_buffers()
{
    struct rule_format *stnza_ptr=&stanza[0];
    int i=0;

    while( strcmp(stnza_ptr->rule_id,"NONE") != 0)
    {
        for (i=0; i< stnza_ptr->num_patterns; i++){
            if ( stnza_ptr->pattern[i]){
                free(stnza_ptr->pattern[i]);
                stnza_ptr->pattern[i]=NULL;
            }
            strcpy(stnza_ptr->pattern_name[i],"\0");
            stnza_ptr->pattern_size[i]=0;
        }
        stnza_ptr++;
    }
}

/* FUNCTION int fill_pattern_details(int pi , char *str, int *line)
 *      pi : pattern index in the rules stanza
 *     str : pattern name string
 *    line : pointer to line number currently parsed in the rules file
*/
int fill_pattern_details(int pi, char *str, int *line)
{
    char tmp_str[32],buff[128],*pstr=&buff[0],**ptr=&pstr,*strptr;
    int len  = 0, i,res=0;
    unsigned long val = 0;
    display(HTX_HE_INFO,DBG_INFO_PRINT,"#fill_pattern_details: pi=%d,str=%s,line=%d\n",\
                pi,str,*line);

    strncpy(r.pattern_name[pi],str,66);
    if( strncmp(str,"0X",2) == 0 ) {
        if (strcmp(r.oper,"DMA")==0){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line#%d pattern = %s "
                     "(DMA oper cannot have HEX pattern like 0xBEEFDEAD)\nOnly file"
                    "name is allowed in pattern field.\n",*line, str);
        }
        /* Pattern is specified as a HEX immediate value like 0xFFFFFFFF */
        len=strlen(str);
        len = len - 2;
        if (len == 0) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"HEX pattern cannot be zero sized\n");
            re_initialize_global_vars();
            exit(1);
        }
        if ( len%16 != 0 ) {
            /* Pattern should be multiple of 8 bytes i.e, 16 characters */
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"#line %d: %s pattern value "
                "should be in multiple of 8 bytes \n",*line,str);
            return(1);
        }
        /* Fill the size of the pattern */
        r.pattern_size[pi]=len/2; /* 0xFF 2 characters is 1 byte */
        if ( (r.pattern_size[pi]) & (r.pattern_size[pi]-1)) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"#line %d: %s pattern's length "
                "should be power of 2 bytes\n",*line,str);
            return(1);
        }
        r.pattern[pi]=(char *) malloc(r.pattern_size[pi]);
        if (r.pattern[pi] == NULL ) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"#line %d: %s pattern value,"
                    " pattern buffer malloc error - %d (%s)\n",*line,str,errno,strerror(errno));
        }

        display(HTX_HE_INFO,DBG_INFO_PRINT,"#fill_pattern_details: len =%d\n",len);
        str=str+2;
        i = 0;
        while (i < len) {
            strncpy(tmp_str,str,16);
            tmp_str[16]='\0';
            val=strtoul(tmp_str,NULL,16);
            if (val == 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"#line %d: %s pattern is not "
                        "proper hex value\n",*line,str);
            }
            display(HTX_HE_INFO,DBG_INFO_PRINT,"#fill_pattern_details: val =0x%lx\n",val);
            *(unsigned long *)(&r.pattern[pi][8*(i/16)])=val;
            i=i+16;
            str=str+16;
        }
        if (r.pattern_size[pi] > MIN_PATTERN_SIZE ) {
            r.pattern_type[pi]= PATTERN_SIZE_BIG;
        } else {
            r.pattern_type[pi]= PATTERN_SIZE_NORMAL;
        }

        for (i=0; i<r.pattern_size[pi]; i++) {
            display(HTX_HE_INFO,DBG_INFO_PRINT,"-%x-",r.pattern[pi][i]);
        }
    }
    else {
        strptr=strtok_r(str,"()",ptr);
        strptr=strtok_r(NULL,"()",ptr);
        if (strptr!=NULL){
            val=atoi(strptr);
               display(HTX_HE_INFO,DBG_INFO_PRINT,"#fill_pattern_details:In (%s) size of patttern "
                "to be extracted specified as %d\n",strptr,val);
            /* Size of the pattern cannot be specified for RANDOM and ADDRESS patterns*/
			if ((strcmp(r.pattern_name[pi],"RANDOM")==0 )&& (strcmp(r.pattern_name[pi],"ADDRESS")==0)) {
            	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"#line %d: %s Size of the pattern cannot"
                        " be specified for RANDOM and ADDRESS patterns \n",*line,str);
            	return(1);
            	/* Fill the size of the pattern with default value */
            }
            r.pattern_size[pi]=val;
        }
        else {
			if ((strcmp(r.pattern_name[pi],"RANDOM")!=0 )&& (strcmp(r.pattern_name[pi],"ADDRESS")!=0)) {
            	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"#line %d: %s pattern's length "
                	"should be specified with the pattern file name \n",*line,str);
            	return(1);
            	/* Fill the size of the pattern with default value */
			}
            /* If pattern is ADDRESS or RANDOM, size is assigned as 8 */
            r.pattern_size[pi] = MIN_PATTERN_SIZE;
        }
        if ( (r.pattern_size[pi]) & (r.pattern_size[pi]-1)) {
            displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                     "#line %d: %s pattern's length should be power of "
                     "2 bytes\n", *line, str);
            return(1);
        }
        if (r.pattern_size[pi] > MAX_PATTERN_SIZE ||
            r.pattern_size[pi] < MIN_PATTERN_SIZE) {
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                         "#line %d: %s pattern has improper pattern size "
                         "(should be 8 <= size <= 4096)\n", *line, str);
                return(1);
        } else {
            if (r.pattern_size[pi] % MIN_PATTERN_SIZE != 0) {
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                         "#line %d: %s pattern has improper pattern size "
                         "(should multiple of 8 bytes)\n", *line, str);
                return(1);
            }
            r.pattern[pi]=(char *) malloc(r.pattern_size[pi]);
            if (r.pattern[pi] == NULL ) {
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                        "#line %d: %s pattern value, pattern buffer malloc "
                        "error - %d (%s)\n",*line, str, errno, strerror(errno));
            }

            char* pat_tmp_ptr = getenv("HTXPATTERNS");
            if(pat_tmp_ptr == NULL){
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s(): env variable HTXPATTERNS is not set,pat_tmp_ptr=%p"
                ", thus exiting..\n",__LINE__,__FUNCTION__,pat_tmp_ptr);
                return (-1);
                
            }else{
                strcpy(buff,pat_tmp_ptr);
            }
            strcat(buff,r.pattern_name[pi]);
            display(HTX_HE_INFO, DBG_INFO_PRINT,
                     "#fill_pattern_details:In (pattern name = %s) size = %d\n",
                     buff, r.pattern_size[pi]);
            if (strcmp(r.pattern_name[pi],"ADDRESS") != 0 &&
                strcmp(r.pattern_name[pi],"RANDOM") != 0) {
                if ((hxfpat(buff, (char *)&r.pattern[pi][0],
                     r.pattern_size[pi])) != 0) {
                    displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                             "pattern fetching problem -error - %d", res);
                    return(1);
                }
                if (r.pattern_size[pi] > MIN_PATTERN_SIZE ) {
                    r.pattern_type[pi] = PATTERN_SIZE_BIG;
                } else {
                    r.pattern_type[pi] = PATTERN_SIZE_NORMAL;
                }
            } else {
                if (strcmp(r.pattern_name[pi],"ADDRESS") == 0) {
                    *(unsigned long *)r.pattern[pi] = ADDR_PAT_SIGNATURE;
                    r.pattern_type[pi] = PATTERN_ADDRESS;
                } else {
                    *(unsigned long *)r.pattern[pi] = RAND_PAT_SIGNATURE;
                    r.pattern_type[pi] = PATTERN_RANDOM;
                }
            }
        }
    }
    return(0);
}

/*****************************************************************************
*  for each keyword parameter of the rule stanza:                            *
*    1. assign default value to corresponding program variable.              *
*    2. assign specified value to corresponding program variable.            *
*    3. validity check each variable.                                        *
*                                                                            *
*  return code =  0 valid stanza                                             *
*              =  1 invalid stanza                                           *
*              = -1 EOF                                                      *
*****************************************************************************/
int get_rule(int * line, FILE *fp)
{
    char  s[MAX_RULE_LINE_SIZE],buff[500],*pstr=&buff[0],**ptr=&pstr;
    char  keywd[80];
    int   i,j;
    int   keywd_count;         /* keyword count                               */
    int   rc;                  /* return code                                 */
    int   first_line;          /* line number of first keyword in stanza      */

    display(HTX_HE_INFO,DBG_INFO_PRINT,"#1 get_rule\n");

    /* Set the default values in the stanza pointer variable */
    set_defaults();
    keywd_count = 0;
    first_line = *line + 1;
    while ((get_line(s,MAX_RULE_LINE_SIZE,fp)) > 1)
    {
        *line = *line + 1;
        if (s[0] == '*')
            continue;

        for (i=0; s[i]!='\n' && s[i] != '\0'; i++) {
            s[i] = toupper(s[i]);
            if (s[i] == '=')
                s[i] = ' ';
        } /* endfor */

        keywd_count++;
        sscanf(s,"%s",keywd);

        if ((strcmp(keywd,"RULE_ID")) == 0) {
                sscanf(s,"%*s %s",r.rule_id);
                if ((strlen(r.rule_id)) > 24) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s"
                            " (must be 8 characters or less)\n",*line,keywd,\
                            r.rule_id);
                    exit(1);
                } /* endif */
        }
        else if ((strcmp(keywd,"PATTERN_ID")) == 0)
        {
            char *tmp_str;
            int i = 0;

            tmp_str=strtok_r(s," \n",ptr);
            while (tmp_str != NULL && i <= 9 ){
                tmp_str=strtok_r(NULL," \n",ptr);
                display(HTX_HE_INFO,DBG_INFO_PRINT,"#tmp_str (%d) = %s\n",\
                        i,tmp_str);
                if (tmp_str == NULL ) {
                    break;
                }
                /* Ignore RANDOM pattern if AME is enabled as random pattern reduces performance of AME enabled system */
                if ((AME_enabled == 1)&& (strcmp(tmp_str, "RANDOM") == 0)) {
					displaym(HTX_HE_INFO, DBG_MUST_PRINT, "AME is enabled on system, hence ignoring RANDOM pattern as it degrades the performance.\n");
					continue;
				}
				if ((strlen(tmp_str)) > 66) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %s (must "
                            "be 34 characters or less)\n",s);
                }
                strcpy(r.pattern_name[i],tmp_str);
                if ( fill_pattern_details(i,r.pattern_name[i],line) != 0 ) {
                    exit(1);
                }
                i++;
            }
            if (tmp_str) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %s (more than "
                        "10 patterns cannot be specified in a single rule\n",s);
            }
            if ( i ) {
                r.num_patterns = i;
            }
            else {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line#%d %s (pattern not"
                    " specified properly )\n",*line,s);
                exit(1);
            }

            display(HTX_HE_INFO,DBG_INFO_PRINT,"num of patterns = %d\n",\
                    r.num_patterns );

            if ((strcmp(r.oper,"DMA")==0)&&(r.num_patterns > 1)){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line#%d %s "
                     "(DMA oper cannot have more than one pattern)\n",*line, s);
                exit(1);
            }

            for (i=0; i<r.num_patterns; i++) {
                display(HTX_HE_INFO,DBG_INFO_PRINT,"pattern name = %s\n "
                        "pattern_size =%d\npattern =0x",r.pattern_name[i],\
                        r.pattern_size[i]);
                if ((strcmp(r.oper,"DMA")==0) && ((strcmp(r.pattern_name[i],"ADDRESS")==0)\
                        || (strcmp(r.pattern_name[i],"RANDOM")==0))) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line#%d %s = %s "
                     "(DMA oper cannot have ADDRESS or RANDOM pattern test case in "
                     "the rules file %s)\n",*line,keywd,r.pattern_name[i],\
                     priv.rules_file_name);
                    exit(1);
                }
                for (j=0; j<r.pattern_size[i]; j++) {
                    display(HTX_HE_INFO,DBG_INFO_PRINT,"%0x",r.pattern[i][j]);
                }
                display(HTX_HE_INFO,DBG_INFO_PRINT,"\n");
            }

        }/*if (strcmp(keywd,pattern_id... ends */
        else if ((strcmp(keywd,"NUM_OPER")) == 0)
        {
            sscanf(s,"%*s %d", &r.num_oper);
            if (r.num_oper < 1) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        "(must be 1 or greater) \n",*line,keywd,r.num_oper);
                exit(1);
            } /* endif */
        }
        else if (strcmp(keywd,"MODE")== 0) {
            (void) sscanf(s,"%*s %s",r.mode);
            if (strcmp(r.mode,"CONCURRENT") == 0) {
                r.run_mode= RUN_MODE_CONCURRENT;
            }else  if ( strcmp(r.mode,"NORMAL") == 0 ) {
                r.run_mode= RUN_MODE_NORMAL;
            } else {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                        "(must be CONCURRENT or NORMAL ",*line, keywd, r.mode);
                exit(1);
            }
        }
        else if (strcmp(keywd,"SEED")==0) {
            char seed_str[128];
            (void) sscanf(s,"%*s %s",seed_str);
            if ((strncmp(seed_str,"0X",2) != 0)||( strlen(seed_str) > 18 )) {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s " "(must enter hexadecimal (like 0x456) which is less "
                        "or equal to 8 bytes\n",*line,keywd,seed_str);
            }else {
                (void) sscanf(seed_str,"0X%llu",&r.seed);
            }
            display(HTX_HE_INFO,DBG_INFO_PRINT,"seed = 0x%lx\n",r.seed);
        }
        else if (strcmp(keywd,"NUM_THREADS")==0) {
            int num_proc=0;
            sscanf(s,"%*s %d",&r.num_threads);
            num_proc = get_num_of_proc();
            if ((r.num_threads < 1) || (r.num_threads > num_proc)) {
                     displaym(HTX_HE_INFO,DBG_MUST_PRINT,"line# %d %s = %ld "
                         "(num of threads not in valid range(<%d))\nAdjusting"
                         "it to %d",*line,keywd, r.num_threads,\
                         num_proc,num_proc);
                        /* r.num_threads=num_proc; */
            }
        }
        else if ((strcmp(keywd,"OPER"))== 0)
        {
            int i = 0;
            (void) sscanf(s,"%*s %s",r.oper);
            if (strcmp(r.oper,"MEM") == 0 ) {
                r.operation=OPER_MEM;
            } else if (strcmp(r.oper,"DMA") == 0 ) {
                r.operation=OPER_DMA;
            } else if ( strcmp(r.oper,"RIM") == 0 ) {
                r.operation=OPER_RIM;
            } else if ( strcmp(r.oper,"TLB") == 0 ) {
                r.operation=OPER_TLB;
            } else if ( strcmp(r.oper,"MPSS") == 0) {
                r.operation=OPER_MPSS;
            } else if ( strcmp(r.oper,"EMPSS") == 0) {
			    r.operation=OPER_EMPSS;
	    	} else if ( strcmp(r.oper,"NX_MEM") == 0) {
			   r.operation=OPER_NX_MEM;
	    	} else if (strcmp(r.oper, "NSTRIDE") == 0) {
				r.operation=OPER_NSTRIDE;
			} else if (strcmp(r.oper, "L4_ROLL") == 0) {
				r.operation=OPER_L4_ROLL;
			} else if (strcmp(r.oper, "MBA") == 0) {
				r.operation=OPER_MBA;
			} else if (strcmp(r.oper, "CORSA") == 0) {
				r.operation=OPER_CORSA;
			}
            else {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line# %d %s =%s \
                         (must be MEM/DMA/RIM/TLB/MPSS/EMPSS/NSTRIDE)\n",*line,\
                         keywd, r.oper);
                  exit(1);
            } /* end else */
            if ((strcmp(r.oper,"DMA")==0) && ((r.num_patterns > 1) || \
                    (strncmp(r.pattern_name[0],"0X",2)==0))){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line#%d %s "
                     "(DMA oper cannot have more than one pattern)\n",*line, s);
                exit(1);
            }

            for (i=0; i<r.num_patterns; i++) {
                if ((strcmp(r.oper,"DMA")==0) && ((strcmp(r.pattern_name[i],"ADDRESS")==0)\
                            || (strcmp(r.pattern_name[i],"RANDOM")==0))) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line#%d %s = %s "
                             "(DMA oper cannot have ADDRESS or RANDOM pattern test case in "
                             "the rules file %s)\n",*line,keywd,r.oper,\
                             priv.rules_file_name);
                    exit(1);
                }
            }
        }
        else if ((strcmp(keywd,"MESSAGES"))== 0)
        {
            (void) sscanf(s,"%*s %s",r.messages);
            if (strcmp(r.messages,"NO") == 0 || strcmp(r.messages,"YES") == 0)
                  ;
            else {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                         "(must be NO or YES)\n",*line, keywd, r.messages);
                  exit(1);
            } /* end else */
        }
        else if ((strcmp(keywd,"BIND_PROC"))== 0)
        {
            char bp[20];
            (void) sscanf(s,"%*s %s",bp);
            if (strcmp(bp,"NO") == 0 ) {
                r.bind_proc = 0;
            }else if (strcmp(bp,"YES") == 0) {
                r.bind_proc = 1;
            }
            else {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                         "(must be NO or YES)\n",*line, keywd, bp);
                  exit(1);
            } /* end else */
        }
	else if ((strcmp(keywd,"BIND_METHOD"))== 0)
       {
           char bm[20];
           (void) sscanf(s,"%*s %s",bm);
             if (strcmp(bm,"NO") == 0 ) {
                 r.bind_method = 0;
             }else if (strcmp(bm,"YES") == 0) {
                 r.bind_method = 1;
             }
             else {
                  displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                          "(must be BIND_RANDOM  or BIND_FIXED)\n",*line, keywd, bm);
                   exit(1);
             } /* end else */
        }
	else if ((strcmp(keywd,"AFFINITY"))== 0)
       {
           char ay[20];
           (void) sscanf(s,"%*s %s",ay);
             if (strcmp(ay,"NO") == 0 ) {
                 r.affinity = 0;
             }else if (strcmp(ay,"YES") == 0) {
                 r.affinity = 1;
				 if (priv.affinity_flag == 0) {
					priv.affinity_flag = 1;
             	 }
             }
             else {
                  displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                          "(must be NO or YES)\n",*line, keywd, ay);
                   exit(1);
             } /* end else */
        }

        else if ((strcmp(keywd,"SWITCH_PAT_PER_SEG"))== 0)
        {
            char sp[20];
            (void) sscanf(s,"%*s %s",sp);
            if (strcmp(sp,"NO") == 0 ) {
                r.switch_pat = SW_PAT_OFF;
            }else if (strcmp(sp,"YES") == 0) {
                r.switch_pat = SW_PAT_ON;
            }else if (strcmp(sp,"ALL") == 0) {
                r.switch_pat = SW_PAT_ALL;
            }
            else {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                         "(must be NO or YES)\n",*line, keywd, sp);
                  exit(1);
            } /* end else */
        }
        else if ((strcmp(keywd,"COMPARE"))== 0)
        {
            (void) sscanf(s,"%*s %s",r.compare);
            if (strcmp(r.compare,"NO") == 0 ) {
                r.compare_flag = 0;
            } else if ( strcmp(r.compare,"YES") == 0 ) {
                r.compare_flag = 1;
            }
            else {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                         "(must be NO or YES)\n",*line, keywd, r.compare);
                exit(1);
            } /* end else  */
        }
        else if ((strcmp(keywd,"MAX_MEM"))== 0)
        {
            (void) sscanf(s,"%*s %s",r.max_mem);
            if (strcmp(r.max_mem,"NO") == 0) {
                r.max_mem_flag = 0;
            } else if ( strcmp(r.max_mem,"YES") == 0) {
                r.max_mem_flag = 1;
            }
            else {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                        "(must be NO or YES)\n",*line, keywd, r.max_mem);
                exit(1);
            } /* end else */
        }
        else if ((strcmp(keywd,"NUM_WRITES")) == 0)
        {
            sscanf(s,"%*s %d", &r.num_writes);
            if ((r.num_writes < 0) )
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(num_writes should be > 0)\n",*line, keywd, \
                        r.num_writes);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"NUM_READ_ONLY")) == 0)
        {
            sscanf(s,"%*s %d", &r.num_reads);
            if ((r.num_reads < 0) )
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.num_reads);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"NUM_READ_COMP")) == 0)
        {
            sscanf(s,"%*s %d", &r.num_compares);
            if ((r.num_compares < 0) )
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.num_compares);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"SEG_SIZE_4K")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_4K]);
            if (r.seg_size[PAGE_INDEX_4K] < MIN_SEG_SIZE)
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.seg_size[PAGE_INDEX_4K]);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"SEG_SIZE_64K")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_64K]);
            if (r.seg_size[PAGE_INDEX_64K] < MIN_SEG_SIZE)
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.seg_size[PAGE_INDEX_64K]);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"SEG_SIZE_2M")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_2M]);
            if (r.seg_size[PAGE_INDEX_2M] < (2*MB))
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.seg_size[PAGE_INDEX_2M]);
                exit(1);
            }              /* end else */
        }

        else if ((strcmp(keywd,"SEG_SIZE_16M")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_16M]);
            if (r.seg_size[PAGE_INDEX_16M] < (16*MB))
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.seg_size[PAGE_INDEX_16M]);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"SEG_SIZE_16G")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_16G]);
            if ((unsigned long) r.seg_size[PAGE_INDEX_16G] <
                (unsigned long)(16*GB))
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n", *line, keywd,\
                        r.seg_size[PAGE_INDEX_16G]);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"NUM_SEG_4K")) == 0)
        {
            sscanf(s,"%*s %d",&r.num_seg[PAGE_INDEX_4K]);
            if (r.num_seg[PAGE_INDEX_4K] < 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        "(number segments not in valid range)\n",*line, keywd,\
                        r.num_seg[PAGE_INDEX_4K]);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"NUM_SEG_64K")) == 0)
        {
            sscanf(s,"%*s %d",&r.num_seg[PAGE_INDEX_64K]);
            if (r.num_seg[PAGE_INDEX_64K] < 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        "(number segments not in valid range)\n",*line, keywd,\
                        r.num_seg[PAGE_INDEX_64K]);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"NUM_SEG_2M")) == 0)
        {
            sscanf(s,"%*s %d",&r.num_seg[PAGE_INDEX_2M]);
            if (r.num_seg[PAGE_INDEX_2M] < 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        "(number segments not in valid range)\n",*line, keywd,\
                        r.num_seg[PAGE_INDEX_2M]);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"NUM_SEG_16M")) == 0)
        {
            sscanf(s,"%*s %d",&r.num_seg[PAGE_INDEX_16M]);
            if (r.num_seg[PAGE_INDEX_16M] < 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        "(number segments not in valid range)\n",*line, keywd,\
                        r.num_seg[PAGE_INDEX_16M]);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"NUM_SEG_16G")) == 0)
        {
            sscanf(s,"%*s %d",&r.num_seg[PAGE_INDEX_16G]);
            if (r.num_seg[PAGE_INDEX_16G] < 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        "(number segments not in valid range)\n",*line, keywd,\
                        r.num_seg[PAGE_INDEX_16G]);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"WIDTH")) == 0)
        {
            sscanf(s,"%*s %d",&r.width);
            if ((r.width != LS_BYTE) &&
                (r.width != LS_WORD) &&
                (r.width != LS_DWORD)) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d"
                        " (width must be 1, 4, or 8)\n",*line, keywd, r.width);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"DEBUG_LEVEL")) == 0)
        {
            sscanf(s,"%*s %d",&r.debug_level);
            if ((r.debug_level < 0) || (r.debug_level > 3 ) ) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        " (debug_level must be >= 0 and <= 3)\n",*line, keywd,\
                        r.debug_level);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"STARTUP_DELAY")) == 0)
        {
            sscanf(s, "%*s %d", &r.startup_delay);
            if ((r.startup_delay < 0) || (r.startup_delay > 120)) {
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "line# %d %s = %d "
                        " (startup_delay must be >= 0 and <= 120 seconds)\n",
						 *line, keywd, r.startup_delay);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"MEM_PERCENT")) == 0)
        {
            sscanf(s,"%*s %d",&r.mem_percent);
            if (r.mem_percent < MIN_MEM_PERCENT) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                         "(mem_percent must be >= %d%)\n",*line, keywd,\
                         r.mem_percent, MIN_MEM_PERCENT);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"MCHECK_MODE")) == 0)
        {
            sscanf(s,"%*s %d",&r.mcheck_mode);
            if ((r.mcheck_mode < 0) || (r.mcheck_mode > 2) ) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        "(mcheck_mode must be 0,1, or 2)\n",*line, keywd, \
                        r.mcheck_mode);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"MCHECK_RFILE")) == 0)
        {
            sscanf(s,"%*s %s",&r.mcheck_rfile[0]);
        }
        else if ((strcmp(keywd,"CRASH_ON_MIS")) == 0)
        {
            sscanf(s,"%*s %s",r.crash_on_mis);
            if (strcmp(r.crash_on_mis,"YES") == 0 ) {
                r.misc_crash_flag = 1;
            } else if (strcmp(r.crash_on_mis,"NO") == 0 ) {
                r.misc_crash_flag = 0;
            } else {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s "
                        "(crash_on_mis flag must be set to YES/NO)\n",*line, \
                        keywd, r.crash_on_mis);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"TURN_ATTN_ON")) == 0)
        {
            sscanf(s,"%*s %s",r.turn_attn_on);
            if (strcmp(r.turn_attn_on,"YES")==0) {
                r.attn_flag = 1;
            } else if (strcmp(r.turn_attn_on,"NO")==0) {
                r.attn_flag = 0;
            } else {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s "
                        "(turn_attn_on flag must be set to YES/NO)\n",*line,\
                        keywd, r.turn_attn_on);
                exit(1);
            }              /* endif */
        }
		else if ((strcmp(keywd,"REMAP_P")) == 0)
		{
			sscanf(s,"%*s %d",&r.remap_p);
			if ((r.remap_p < 0) || (r.remap_p > 100)) {
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
						"(remap_p value should be in range 0-100\n",*line,\
						keywd, r.remap_p);
				exit(1);
			}
		}
		else if ((strcmp(keywd,"SAO_ENABLE")) == 0)
		{
			sscanf(s,"%*s %d",&r.sao_enable);
			if (( r.sao_enable != 0) && ( r.sao_enable != 1)){
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
						"(sao_enable value should be 0 or 1 \n",*line,\
						keywd, r.sao_enable);
				exit(1);
			}
		}
		else if ((strcmp(keywd,"I_SIDE_TEST")) == 0)
		{
			sscanf(s,"%*s %d",&r.i_side_test);
			if (( r.i_side_test != 0) && ( r.i_side_test != 1)){
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
						"(i_side_test value should be 0 or 1 \n",*line,\
						keywd, r.i_side_test);
				exit(1);
			}
		}
		else if ((strcmp(keywd,"D_SIDE_TEST")) == 0)
		{
			sscanf(s,"%*s %d",&r.d_side_test);
			if (( r.d_side_test != 0) && ( r.d_side_test != 1)){
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
						"(d_side_test value should be 0 or 1 \n",*line,\
						keywd, r.d_side_test);
				exit(1);
			}
		}
		else if ((strcmp(keywd,"ADDNL_I_SIDE_TEST")) == 0)
		{
			sscanf(s,"%*s %d",&r.addnl_i_side_test);
			if (( r.addnl_i_side_test != 0) && ( r.addnl_i_side_test != 1)){
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
						"(addnl_i_side_test value should be 0 or 1 \n",*line,\
						keywd, r.addnl_i_side_test);
				exit(1);
			}
		}
		else if ((strcmp(keywd,"BASE_PAGE_SIZE")) == 0)
		{
			sscanf(s,"%*s %s",r.base_page_size);
			if ((strcmp(r.base_page_size,"4K")) == 0)
				r.base_pg_sz = 4*1024;
			else if ((strcmp(r.base_page_size,"64K")) == 0)
				r.base_pg_sz = 64*1024;
			else {
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s "
					"(base page size value should be either 4K or 64K\n",*line,\
					keywd, r.base_page_size);
				exit(1);
			}
		}
		else if ((strcmp(keywd,"BASE_SEGMENT_SIZE")) == 0)
		{
			sscanf(s,"%*s %s",r.base_seg_size);
			if ((strcmp(r.base_seg_size,"256MB")) == 0)
				r.base_sg_sz = 256*1024*1024;
			else if ((strcmp(r.base_seg_size,"1TB")) == 0) {
				r.base_sg_sz = 1ULL << 40;
				printf(" 1TB: base_sg_sz: %llx\n",r.base_sg_sz);
			}
			else {
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s "
					"(base segment size value should be either 256MB or 1TB\n",*line,\
					keywd, r.base_seg_size);
				exit(1);
			}
		}
       		 else if ((strcmp(keywd,"NX_PERFORMANCE_DATA")) == 0)
        	{
            		sscanf(s,"%*s %s",r.nx_performance_data);
            		if ((strcmp(r.nx_performance_data,"NO")) == 0) {
                		r.nx_perf_flag = 0;
            		}
            		else {
						r.nx_perf_flag = 1;
            		}
        	}
       		 else if ((strcmp(keywd,"NX_ASYNC")) == 0)
        	{
            		sscanf(s,"%*s %s",r.nx_async);
            		if ((strcmp(r.nx_async,"NO")) == 0) {
                		r.nx_async_flag= 0;
            		}
            		else {
						r.nx_async_flag= 1;
            		}
        	}
       		 else if ((strcmp(keywd,"NX_REMINDER_THREADS_FLAG")) == 0)
        	{
            		sscanf(s,"%*s %s",r.nx_reminder_threads_flag);
            		if ((strcmp(r.nx_reminder_threads_flag,"NO")) == 0) {
                	r.nx_rem_th_flag = 0;
            		}
            		else {
				r.nx_rem_th_flag = 1;
            		}
        	}

		else if ((strcmp(keywd,"NX_MEM_OPERATIONS"))== 0)
                {
			displaym(HTX_HE_INFO,DBG_INFO_PRINT,"INSIDE NX_MEM_OPERATIONS\n");
			char *chr_ptr, *str1, tmp_buf[100];
			char  *s1, *s2;
			int task = 0, field = 0, val;
			float valf;
			str1 = s;

			while((task < MAX_NX_TASK) && (str1 != NULL)){
				chr_ptr = strsep(&str1, "[");
				if(str1 != NULL) {

					s1 = strsep(&str1, "]");
					if(str1 != NULL) {
						strcpy(tmp_buf, s1);
						displaym(HTX_HE_INFO,DBG_INFO_PRINT,"s1 = %s \n",s1);
					}
					else {
						displaym(HTX_HE_INFO,DBG_INFO_PRINT,"# task = %d \t tmp_buf  = %s\n",
						task,tmp_buf);
						return(-1);
					}

				}
				else {
					displaym(HTX_HE_INFO,DBG_INFO_PRINT," before TASK break \n");
					break;
				}
				s2 = tmp_buf;
				field = 0;
				while(field < MAX_NX_FIELD) {
					chr_ptr = strsep(&s2, ":");

 					if((s2 == NULL) && (field != 4)) {
						displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"\n All the fields of the task %d"
						"should be specified ",task);
						exit(1);
				}


				switch(field) {
					case 0: /*In this field we expect only nx_mem_oper name */
						if( strcmp(chr_ptr,"CDC-CRC") == 0){
							r.nx_task[task].nx_operation = CDC_CRC;
						}else
						if( strcmp(chr_ptr,"CDC") == 0){
							r.nx_task[task].nx_operation = CDC;
						}else
						if( strcmp(chr_ptr,"BYPASS") == 0){
							r.nx_task[task].nx_operation = BYPASS;
						}else
						if( strcmp(chr_ptr,"C-CRC") == 0){
							r.nx_task[task].nx_operation = C_CRC;
						}else
						if( strcmp(chr_ptr,"C-ONLY") == 0){
							r.nx_task[task].nx_operation = C_ONLY;
						}else {
							displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper nx_mem ope"
							"ration chr_ptr = %s\n",chr_ptr);
							exit(1);
						}
					break;

					case 1: /* In this field we look for percentage of threads/chip */
						valf = atof(chr_ptr);
						if( valf >= 0 && valf <= 100){
							r.nx_task[task].thread_percent = valf;
						}
						else {
							displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper thread"
							"percent  thread_percent = %d \t task = %d \t field = %d \t"
							" line = %s \n",valf,task,field,s);
							exit(1);
						}
					break;

					case 2: /* This field is for min_buf */
						val = atoi(chr_ptr);
						if( val > 0 ){
							r.nx_task[task].nx_min_buf_size = (unsigned int) val;
						}
						else {
							displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper min_buf_size = %s \t  task = %d \t field = %d \t line = %s \n",
							chr_ptr,task,field,s);
							exit(1);
						}
					break;

					case 3:  /* This field is for max_buf_size */
						val = atoi(chr_ptr);
						if( val >= r.nx_task[task].nx_min_buf_size){
							r.nx_task[task].nx_max_buf_size = (unsigned int) val;
						}
						else {
							displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper "
							"max_buf_size = %s \t task = %d \t field = %d \t line = %s \n"
							"max_buf_size should be >= min_buf_size",
							chr_ptr,task,field,s);
							exit(1);
						}
					break;

					case 4:
						/* We are here means reached the last field which is chip number */
						if(strcmp(chr_ptr,"*") == 0){
							r.nx_task[task].chip = -1;
							break;
						}

						val = atoi(chr_ptr);
						if( val >= 0 && val < MAX_P7_CHIP ){
							r.nx_task[task].chip = val;
						}
						else {
							displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper "
							"chip = %s \t task = %d \t field = %d \t line = %s \n",
							chr_ptr,task,field,s);
							exit(1);
						}
					break;

					default:
						displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper "
						"values for line  = %s \t task = %d \t field = %d \t line = %s \n",
						chr_ptr,task,field,s);
						 exit(1);

					} /* END of switch */

				field++;
				} /* END of field */

			task++;
			r.number_of_nx_task = task; /*saving number of tasks */
			} /* END of while i < 20 */

		/* This is a DEBUG TEST LOOP */
		int d;
		i = task;
		for(task = 0; task < i; task ++){
			d = task;
			displaym(HTX_HE_INFO,DBG_INFO_PRINT,"r.nx_task[%d].nx_operation = %d \t"
			"  r.nx_task[%d].thread_percent = %f \t r.nx_task[%d].nx_min_buf_size = %d \n"
			"r.nx_task[%d].nx_max_buf_size = %d \t \t"
			" r.nx_task[%d].chip = %d \n\n",task,r.nx_task[d].nx_operation,task,r.nx_task[d].thread_percent, \
			task,r.nx_task[d].nx_min_buf_size,task,r.nx_task[d].nx_max_buf_size,task,
			r.nx_task[d].chip );

	    	}

		}
        else if ((strcmp(keywd,"STRIDE_SZ"))== 0)
        {
			sscanf(s,"%*s %d",&r.stride_sz);
			if (r.stride_sz > MAX_STRIDE_SZ) {
				displaym(HTX_HE_INFO, DBG_MUST_PRINT, "Stride size (%d) is greater than MAX_STRIDE_SZ(%d).\n"
													  "Seeting it to MAX_STRIDE_SZ.\n",
													  r.stride_sz, MAX_STRIDE_SZ);
				r.stride_sz = MAX_STRIDE_SZ;
			}
		}
		else if ((strcmp(keywd,"CORSA_PERFORMANCE")) == 0)
		{
			sscanf(s,"%*s %s",r.corsa_performance);
			if ((strcmp(r.corsa_performance,"NO")) == 0) {
				r.corsa_perf_flag = 0;
			}
			else {
				r.corsa_perf_flag = 1;
			}
		}

		else if ((strcmp(keywd,"PERCENT_HW_THREADS"))== 0)
		{
			 sscanf(s,"%*s %d",&r.percent_hw_threads);
		}
		else if ((strcasecmp(keywd,"MASK")) == 0)
		{
			sscanf(s,"%*s %s",r.mcs);
		}
		else if ((strcmp(keywd,"MEM_L4_ROLL")) == 0)
		{
			sscanf(s,"%*s %d",&r.mem_l4_roll);
		}
        else if ((strcmp(keywd,"MCS_MASK")) == 0){
            sscanf(s,"%*s %lu",&r.mcs_mask);
        }
        else if ((strcmp(keywd,"BM_LENGTH")) == 0){
            sscanf(s,"%*s %u",&r.bm_length);
        }
        else if ((strcmp(keywd,"BM_POSITION"))== 0){
            sscanf(s,"%*s %u",&r.bm_position);
        }
		else if ((strcmp(keywd,"TLBIE_TEST_CASE"))==0){
			sscanf(s,"%*s %s",r.tlbie_test_case);
		}
		else
        {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d keywd = %s "
                    "(invalid)\n",*line,keywd);
            exit(1);
        }
} /* end while */
    *line = *line + 1;
    if (keywd_count > 0)
    {
        if ((strcmp(r.rule_id,"")) == 0)
        {
             displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line# %d rule_id not"
                     " specified \n",first_line);
             exit(1);
        }
         else rc=0;
    }
    else rc=EOF;
 /* printf("rulefile rc %d \n",rc); */


    if (rc != EOF ) {

        /* Doing a MEMCOPY .. instead of individual copy*/
        memcpy(stanza_ptr,&r,sizeof(r));

        display(HTX_HE_INFO,DBG_INFO_PRINT,"+All the keywords are read "
                "successfully\n");
    }

    return(rc);
}

/*****************************************************************************
 *  This routine sets the default values for the specified rule stanza. You  *
 *  will need to modify it to suit your needs.                               *
 ****************************************************************************/
void set_defaults(void)
{
    int i = 0;
    strcpy(r.rule_id,"");
    strcpy(r.pattern_id,"        ");
    r.num_oper = 1;
    r.num_writes = 1;
    r.bind_proc = 1;
    r.num_reads = 1;
    r.num_compares = 1;
    r.num_threads= -1;
	r.percent_hw_threads = 10;
    r.bind_method = 0;
    r.affinity = 0;
    strcpy(r.mode,"NORMAL");
    r.run_mode = RUN_MODE_NORMAL;
    strcpy(r.oper,"MEM");
    r.operation = OPER_MEM;
    strcpy(r.messages,"YES");
    strcpy(r.max_mem,"YES");
    r.max_mem_flag = 1;
    strcpy(r.compare,"YES");
    r.compare_flag = 1;
    strcpy(r.crash_on_mis,"YES");
    r.misc_crash_flag = 1;
    strcpy(r.turn_attn_on,"NO");
    r.attn_flag = 0;
    for (i=0; i< MAX_NUM_PAGE_SIZES; i ++) {
        r.seg_size[i]=0;
        r.num_seg[i]=0;
    }
    r.width = LS_DWORD;
    r.debug_level = 0;
    r.startup_delay = DEFAULT_DELAY;
    r.mem_percent = DEFAULT_MEM_PERCENT;
    r.mcheck_mode = 0;
    r.num_patterns = 0;
    strcpy(r.pattern_name[0]," ");
    strcpy(r.mcheck_rfile," ");
	r.sao_enable = 0;
	r.i_side_test = 0;
	r.d_side_test = 0;
	r.addnl_i_side_test = 0;
	r.base_pg_sz = 4*KB;
	r.base_sg_sz = 256*MB;
	r.nx_rem_th_flag = 1;
	r.nx_perf_flag = 0;
	r.nx_async_flag = 0;
	r.stride_sz = 128;
	r.mem_l4_roll = 134217728;
	r.bm_position = 7;
	r.bm_length = 2;
	r.mcs_mask = -1;
	strcpy(r.tlbie_test_case,"RAND_TLB");
}

/*****************************************************************************
*  This routine reads a line from "stdin" into the specified string.  It     *
*  returns the length of the string. If the length is 1 the line is blank.   *
*  When it reaches EOF the length is set to 0,                               *
*****************************************************************************/
int get_line( char s[], int lim, FILE *fp)
{
    int c=0,i;

    i=0;
    while (--lim > 0 && ((c = fgetc(fp)) != EOF) && c != '\n') {
        s[i++] = c;
     }
    if (c == '\n')
        s[i++] = c;
    s[i] = '\0';
     return(i);
}



#ifndef __HTX_LINUX__
/****************************************************************************
 *****************************************************************************
 AIX SPECIFIC MEMORY STATISTICS COLLECTION FUNCTIONS START HERE.
 *****************************************************************************
  *****************************************************************************/

/*****************************************************************************
* fill_mem_info_data fills the mem_info data structure
*****************************************************************************/
int fill_mem_info_data() {

    int ret=0,i=0;

 /*
     * detect type of large page implementation for earlier AIX releases than 53D
     */
    mem_info.lpage_type = get_lpage_type();

/*
 * For AIX 53D and above page sizes are detected using vmgetinfo call.
 * Use of various page sizes in here is controlled by rule file option.
 */
#ifdef GET_PAGESIZES
    display(HTX_HE_INFO,DBG_IMP_PRINT, "Calling get_supported_page_sizes\n");

    if((ret = get_supported_page_sizes()) != 0) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_supported_page_sizes "
                " failed with %d...Exiting...\n", ret);
        exit(1);
    }
#else
    mem_info.pdata[PAGE_INDEX_4K].supported = TRUE;
    mem_info.pdata[PAGE_INDEX_4K].psize=(4*KB);
    if(mem_info.lpage_type == 2) {
        mem_info.pdata[PAGE_INDEX_16M].supported = TRUE;
        mem_info.pdata[PAGE_INDEX_16M].psize=(16*MB);
    }
#endif

#ifndef AIX43X
    ret = get_mem_config();
#endif

    /*
     *  determine # of available CPUs
     */
    priv.cpus_avail=get_num_of_proc();

    get_pspace();

    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Paging space available=0x%llx, cpus"
            " avail =%d\n",mem_info.pspace_avail,priv.cpus_avail);

    for (i = 0; i < MAX_NUM_PAGE_SIZES; i++){
        if (mem_info.pdata[i].supported == 1 && mem_info.pdata[i].free == 0) {
            displaym(HTX_HE_INFO,DBG_IMP_PRINT,"There are no free pages for "
                    "this supported page size(%s) So continuing with the next"
                    " page size\n",page_size[i]);
        }
    }

    return(0);
}

/*****************************************************************************
* This function is used to get the type of processor running and AIX release
* As AIX release earlier to 51D have some page types. In short rel > 51D and
* proc type is POWER4 it is GP type or else it type NONE. This function
* returns the large page type.
*****************************************************************************/
int get_lpage_type()
{
    char buf[1024];
    FILE  *fp;
    int i, fgets_success=FALSE, retries=5;
    int kern_type;
    int lpage_type = NONE;
    int rc,sup32 ;
    char system_type[12];


    fgets_success = run_aix_cmd("bootinfo -K",buf,1024);
    if (fgets_success == FALSE) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_lpage_type: fgets error"
                "(%d) in finding kernel mode \n", errno);
        exit(1);
    }

    sscanf(buf, "%d", &kern_type);

    if( !(kern_type == 32 || kern_type == 64) ) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_lpage_type: unexpected value in buf : %s  \n", buf);
        exit(1);
    }

    /* printf("buf %s system_type %s kern_type %d  \n",buf,system_type,kern_type);*/

    strcpy(priv.release_name, getenv("AIXREL"));
    if (strcmp(priv.release_name,"51D") >= 0)  /* l page 32bit support in  51D and higher*/
        sup32 = 1;
    else
        sup32 = 0;

    if ( kern_type == 64 || sup32) {
        /* check for gp */
        fgets_success = run_aix_cmd("/usr/lib/boot/bin/dmpdt_chrp | grep \"/cpus/PowerPC,POWER4\"",
                                 buf,1024);
        if (fgets_success != FALSE) {
            lpage_type = GP_TYPE;
        }

    }
    else {
        lpage_type = NONE;
    }
    /* lpage_type = GP_TYPE; */
    priv.kern_type=kern_type;

    return( lpage_type);

} /* end get_lpage_type*/

/*
 * Function Name: get_supported_page_sizes()
 *
 * Description:
 *    Queries AIX to get supported pages sizes, i.e. 4K, 64K, 16M, 16G.
 * Input:
 *    Nothing
 * Output:
 *    return 0 on success or -1 on error
 * Notes:
 *    Sets supported(4k), supported(64k), supported(16m), supported(16g) to 1
 *    if these pages sizes are supported. Unsupported sizes are set to 0.
 *    Also fills in the psize (page size ) is also filled with the supported page
 *    size values.
 */
#ifdef GET_PAGESIZES
int  get_supported_page_sizes()
{
    int num_psizes, i, ret;
    psize_t *psizes;    /* Determine the number of supported page sizes */
    char tmpstr[500],msg_text[4096];

    num_psizes = vmgetinfo(NULL, VMINFO_GETPSIZES, 0);
    if (num_psizes == -1) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s: vmgetinfo error(%d) \n", __FUNCTION__, errno);
        return(-1);
    }

    if ((psizes = malloc(num_psizes*sizeof(psize_t))) == NULL) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s: malloc error (%d) \n", __FUNCTION__, errno);
        return(-1);    /* Get the page sizes */
    }

    if ((ret = vmgetinfo(psizes, VMINFO_GETPSIZES, num_psizes)) == -1) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s: vmgetinfo error(%d) \n", __FUNCTION__, errno);
        free(psizes);
        return(-1);
    }

    /* psizes[0] = smallest page size
     * psizes[1] = next smallest page size...
     * psizes[num_psizes-1] = largest supported page size
     */
    sprintf(msg_text,"%s: supported page sizes = ", __FUNCTION__);
    for (i = 0; i < num_psizes; i++ ) {
        sprintf(tmpstr,"     0x%llx", psizes[i]);
        strcat(msg_text, tmpstr);
    }
    sprintf(tmpstr,"\n");
    strcat(msg_text, tmpstr);
    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%s",msg_text);

    for (i = 0; i < num_psizes; i++ ) {
        if (psizes[i] == (4*KB)) {
            mem_info.pdata[PAGE_INDEX_4K].supported = TRUE;
            mem_info.pdata[PAGE_INDEX_4K].psize = psizes[i];
        }
        if (psizes[i] == (64*KB)) {
            mem_info.pdata[PAGE_INDEX_64K].supported= TRUE;
            mem_info.pdata[PAGE_INDEX_64K].psize = psizes[i];
        }
        if (psizes[i] == (16*MB)) {
            mem_info.pdata[PAGE_INDEX_16M].supported = TRUE;
            mem_info.pdata[PAGE_INDEX_16M].psize= psizes[i];
        }
        if (psizes[i] == (unsigned long long)(16*GB)) {
            mem_info.pdata[PAGE_INDEX_16G].supported = TRUE;
            mem_info.pdata[PAGE_INDEX_16G].psize= psizes[i];
        }
    }
    free(psizes);
    return(0);
}
#endif /* ifdef GET_PAGESIZES ends */


#ifndef AIX43X
/*
 * Function Name: get_mem_config()
 *
 * Description:
 *    Queries AIX to get memory configuration like total memory, free
 *    memory, and the same stats per page size. Uses vmgetinfo - VMINFO
 *    to find system wide stats and vmgetinfo - VMINFO_PSIZE for specific
 *    page size stats. System wide free memory (in 4K pages) does not
 *    include large pages (16M and 16G) free memory because they are not
 *    considered really as "free".
 * Input:
 *    Nothing
 * Output:
 *    returns 0 on success.
 *    returns return code(rc) on failure.
 * Notes:
 */
int get_mem_config()
{
    struct vminfo vmi;
    int rc=0;
    int i;
    char tmpstr[500], msg_text[4096];


    rc = vmgetinfo(&vmi, VMINFO, sizeof(struct vminfo));
    if(rc != 0) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s: vmgetinfo call for VMINFO cmd failed with \
errno %d \n", __FUNCTION__, errno);
        exit(1);
    }

    mem_info.total_mem_avail = (vmi.true_memsizepgs*(4*KB));

    mem_info.total_mem_free = (vmi.true_numfrb*(4*KB)) + vmi.lgpg_numfrb*vmi.lgpg_size;

    mem_info.pdata[PAGE_INDEX_4K].avail = (vmi.true_memsizepgs * (4*KB)) - (vmi.lgpg_cnt * vmi.lgpg_size);

    mem_info.pdata[PAGE_INDEX_4K].free = vmi.true_numfrb*(4*KB);

    mem_info.pdata[PAGE_INDEX_16M].avail = vmi.lgpg_cnt*vmi.lgpg_size;

    mem_info.pdata[PAGE_INDEX_16M].free = vmi.lgpg_numfrb*vmi.lgpg_size;

    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"total memory=0x%llx free memory=0x%llx\n total large=0x%llx\
free large = 0x%llx \n", mem_info.total_mem_avail, mem_info.total_mem_free, mem_info.pdata[2].avail,\
mem_info.pdata[2].free);

#ifdef SET_PAGESIZE
    {   /* Block begins */
    struct vminfo_psize info[4];
    int i,ret;
    char tmpstr[500];
    FILE *fp;
    unsigned long avail_16m,avail_16g,free_16m,free_16g,file_exists=0;
	char log_dir[256];
	strcpy(log_dir,stats.htx_log_dir);

	strcat(log_dir,"/freepages");
    /*if ((fp=fopen("/tmp/freepages","r"))==NULL) {*/
    if ((fp=fopen(log_dir,"r"))==NULL) {
         displaym(HTX_HE_INFO,DBG_MUST_PRINT, "fopen of memory free pages (%s) failed with errno:%d\n",log_dir,errno);
    }
    else {
        ret=fscanf(fp,"avail_16M=%lu,avail_16G=%lu,free_16M=%lu,free_16G=%lu",&avail_16m,\
                    &avail_16g,&free_16m,&free_16g);

        if (ret == 0 || ret == EOF) {
            displaym(HTX_HE_INFO,DBG_MUST_PRINT, "Error while reading file (%s) \n",log_dir);
            file_exists=0;
        }
        else {
            file_exists=1;
        }
    }

    mem_info.total_mem_free = 0;
    mem_info.total_mem_avail= 0;

    for(i = 0; i < MAX_NUM_PAGE_SIZES; i++) {
    if(mem_info.pdata[i].supported) {
        /* mem_info.pdata[i].psize = supported_pagesizes[i]; */
        info[i].psize=mem_info.pdata[i].psize;
        rc = vmgetinfo(&info[i],VMINFO_PSIZE, sizeof(struct vminfo_psize));
        if(rc != 0) {
            sprintf(msg_text,"%s: vmgetinfo call for VMINFO_PSIZE"
            " cmd failed with errno %d for page size =%s and pagesize=0x%llx\n",\
            __FUNCTION__, errno, page_size[i],mem_info.pdata[i].psize);
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s",msg_text);
            exit(1);
        }
        else {
            mem_info.pdata[i].avail = (unsigned long )info[i].true_numframes*mem_info.pdata[i].psize;
            mem_info.pdata[i].free = (unsigned long ) info[i].true_numfrb*mem_info.pdata[i].psize;
            if (file_exists == 1 && priv.standalone!=1) {
                if (i == PAGE_INDEX_16M ) {
                    mem_info.pdata[i].avail = avail_16m*mem_info.pdata[i].psize;
                    mem_info.pdata[i].free =  free_16m*mem_info.pdata[i].psize;
                }
                if (i == PAGE_INDEX_16G ) {
                    mem_info.pdata[i].avail = avail_16g*mem_info.pdata[i].psize;
                    mem_info.pdata[i].free = free_16g*mem_info.pdata[i].psize;
                }
            }

            mem_info.total_mem_free += mem_info.pdata[i].free;
            mem_info.total_mem_avail += mem_info.pdata[i].avail;
        }
    } /* if (mem_info.pdata[i].supported ... ends */
    } /* for (i=0; i< ... ends */
    } /* Block ends */
#endif

    sprintf(msg_text, "\t Total Memory \t Free Memory \n\n");
    for(i = 0; i < MAX_NUM_PAGE_SIZES; i++) {
        if (mem_info.pdata[i].supported) {
            sprintf(tmpstr, "0x%llx \t 0x%llx \t 0x%llx \n",mem_info.pdata[i].psize,\
               mem_info.pdata[i].avail, mem_info.pdata[i].free );
            strcat(msg_text, tmpstr);
        }
    }
    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%s", msg_text);
    return(0);
}
#endif

/*
 * Function Name: get_pspace()
 *
 * Description:
 *
 * Input:
 *
 * Output:
 *
 * Notes:
 *
 */
void get_pspace()
{
    char msg[221];
    char buf[1024];
    char tmp1[50],tmp2[50];
    FILE  *fp;
    int i,j, fgets_success=FALSE, retries=5;
    long pspace;
    float float_space;

/* Note: fgets returns NULL when sigterm is received and no bytes have been
 * read. If this happens retry a few times.
 */
    for (i=0; i<retries; i++) {
        fflush(stdout);
        errno = 0;  /* Reset the error number */

        /*if ((fp = popen("lsps -s | grep MB | sed 's/MB/  /' | cut -d\" \" -f1-18", "r")) == NULL) {*/
        if ((fp = popen("lsps -s | grep MB | sed 's/MB/  /' ", "r")) == NULL) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"popen error (%d)finding the paging space available\n", errno);
            exit(1);
        } /* if ((fp=... ends */

        if (fgets(buf,1024,fp) != NULL) {
            fgets_success = TRUE;
            break;
        }
        else {
            pclose(fp);
        }
    } /* for(i=0; ... ends */

    if (fgets_success == FALSE) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"fgets error (%d)finding the paging space available\n", errno);
        exit(1);
    } /* endif */

    sscanf(buf," %s %s",tmp1,tmp2);
    sscanf(tmp1,"%d",&i);
    sscanf(tmp2,"%d",&j);

    mem_info.pspace_avail=(unsigned long)i*1024*1024;
    mem_info.pspace_free= (mem_info.pspace_avail*(1.0-(j/100.0)));
    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Paging space= 0x%llx, free =0x%llx\n",mem_info.pspace_avail,mem_info.pspace_free);

} /* end get_pspace */

/****************************************************************************
 *****************************************************************************
 AIX SPECIFIC MEMORY STATISTICS COLLECTION FUNCTIONS END HERE.
*****************************************************************************
*****************************************************************************/

#else

/****************************************************************************
 *****************************************************************************
 LINUX SPECIFIC MEMORY STATISTICS COLLECTION FUNCTIONS START HERE.
*****************************************************************************
*****************************************************************************/

/*****************************************************************************
* fill_mem_info_data_linux  fills the mem_info data structure in Linux
*****************************************************************************/
int fill_mem_info_data_linux(void)
{
    int ret, cmo_check, i=0;
	unsigned long avail_16m,free_16m,avail_16g,free_16g,file_exists=0,new_shmall,new_shmall_4K;
	char cmd_str[128];

    FILE *fp=0;

	/* Set shmmax value to 256MB only if the value on current system is less than that */
	fp = popen("cat /proc/sys/kernel/shmmax","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret  = fscanf(fp,"%s",shm_max);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "Could not read page size config from pipe\n");
		goto error_exit;
	}
	pclose(fp);

	if (strtoul(shm_max,NULL,10) < 268435456) {
    	displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"Changing the /proc/sys/kernel/shmmax variable to 256MB\n");
    	ret = system ("echo 268435456 > /proc/sys/kernel/shmmax");
    	if (ret < 0){
        	displaym(4,DBG_MUST_PRINT,"unable to change the /proc/sys/kernel/shmmax variable\n");
   		}
		displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Changing the /proc/sys/kernel/shmmax variable to 256MB\n");
	}
    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"shmmax: %llu \n", strtoul(shm_max,NULL,10));

	/*
	 * Determine the page size config for the system
	 */
	fp=popen("getconf PAGESIZE", "r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret=fscanf(fp,"%lu\n",&mem_info.base_page_sz);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "Could not read page size config from pipe\n");
		goto error_exit;
	}
	pclose(fp);

	/* Initialize all page sizes to unsupported */
	mem_info.pdata[PAGE_INDEX_4K].supported  = FALSE;
	mem_info.pdata[PAGE_INDEX_64K].supported = FALSE;
	mem_info.pdata[PAGE_INDEX_2M].supported = FALSE;
	mem_info.pdata[PAGE_INDEX_16M].supported = FALSE;
	mem_info.pdata[PAGE_INDEX_16G].supported = FALSE;

	if (mem_info.base_page_sz == (4*KB)) {
		base_pg_idx = PAGE_INDEX_4K;
	} else if (mem_info.base_page_sz == (64*KB)) {
		base_pg_idx = PAGE_INDEX_64K;
	} else {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "Hxemem for linux does not have support for the base page size"
				 " %d\n", mem_info.base_page_sz);
		return(1);
	}

	/*
     *  determine # of available CPUs
     */
    priv.cpus_avail=get_num_of_proc();
    display(HTX_HE_INFO,DBG_MUST_PRINT,"cpus avail=%d\n",priv.cpus_avail);

    /* Filling the mem_info data structure from the OS by shell commands .
     * NOTE: All return values are in KB so we need to multiply by 1024
     */
	fp=popen("cat /proc/meminfo | grep SwapTotal | awk '{print $2}' ","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret=fscanf(fp,"%lu\n",&mem_info.pspace_avail);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "Could not read pspace_avail from pipe\n");
		goto error_exit;
	}
	pclose(fp);

	fp=popen("cat /proc/meminfo | grep SwapFree | awk '{print $2}' ","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret=fscanf(fp,"%lu\n",&mem_info.pspace_free);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "Could not read pspace_free from pipe\n");
		goto error_exit;
	}
	pclose(fp);

	fp=popen("cat /proc/meminfo | grep MemTotal | awk '{print $2}' ","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret=fscanf(fp,"%lu\n",&mem_info.total_mem_avail);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "Could not read total_mem_avail from pipe\n");
		goto error_exit;
	}
	pclose(fp);

	/* Set the shmall to 95% of available memory. /proc/sys/kernel/shmall has memory in terms of 4K pages. */
	new_shmall = ( mem_info.total_mem_avail * 95 ) / 100;
	new_shmall_4K = new_shmall / 4;
	sprintf(cmd_str, "echo %lu > /proc/sys/kernel/shmall", new_shmall_4K);
	ret = system(cmd_str);
	if (ret < 0) {
		displaym(4,DBG_MUST_PRINT,"unable to change the /proc/sys/kernel/shmall variable\n");
	} else {
		displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Changing the /proc/sys/kernel/shmall variable to %d(In terms of 4K)\n", new_shmall_4K);

	}

	fp=popen("cat /proc/meminfo | grep MemFree  | awk '{print $2}' ","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret=fscanf(fp,"%lu\n",&mem_info.total_mem_free);
   	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "Could not read total_mem_free from pipe\n");
		goto error_exit;
	}
	pclose(fp);

	fp=popen("free -k | awk 'NR==2 {print $2}' ","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret=fscanf(fp,"%lu\n",&mem_info.pdata[base_pg_idx].avail);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "Could not read num avail pages from pipe\n");
		goto error_exit;
	}
	pclose(fp);

	fp=popen("free -k | awk 'NR==2 {print $4}' ","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret=fscanf(fp,"%lu\n",&mem_info.pdata[base_pg_idx].free);
	if (ret == 0 || ret == EOF) {
   		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
   				 "Could not read num free pages from pipe\n");
   		goto error_exit;
	}
	pclose(fp);
#ifdef KERNEL_2_6

	/* Assume support for huge pages */
	mem_info.lpage_type = TRUE;
    int huge_page_idx,huge_page_size;

	/*
	 * Check if /proc/ppc64/lparcfg contains the string: cmo_enabled
	 * if cmo_enabled is seen, and is set to 1, then disable hugepges
	 */
	fp=popen("cat /proc/ppc64/lparcfg 2> /dev/null | grep cmo_enabled ","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				 "popen failed: errno(%d)\n", errno);
	    goto error_exit;
	}

	/* search for cmo_enabled string and find the value */
	ret=fscanf(fp,"cmo_enabled=%d\n",&cmo_check);
	if (ret != 0 && ret != EOF && cmo_check == 1) {
	    /* cmo_enabled found, and is set to 1: Kernel supports VRM */
	    vrm_enabled = TRUE;
	    /* VRM is enabled, do not work on large pages */
	    mem_info.lpage_type = FALSE;
	} /* else VRM is disabled; vrm_enabled already set to FALSE */
	pclose(fp);

	display(HTX_HE_INFO,DBG_MUST_PRINT,"VRM enabled=%d\n", vrm_enabled);

	fp=popen("cat /proc/meminfo | grep Hugepagesize | awk '{print $2}' ","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
			"popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret=fscanf(fp,"%lu\n",&huge_page_size);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_INFO, DBG_MUST_PRINT,
							 "Could not read huge_page_size from pipe(Hugepagesize not found\n");
		mem_info.lpage_type = FALSE;
	}
	huge_page_size  = huge_page_size * KB;
    if(huge_page_size == (2 * MB)){
        huge_page_idx = PAGE_INDEX_2M;
    }else if (huge_page_size == (16* MB)){
        huge_page_idx = PAGE_INDEX_16M;
    }
    else{
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"huge_page_size=%d, is neither 2M or 16M,ignoring huge page size\n",huge_page_size);
    }
    mem_info.pdata[huge_page_idx].psize = huge_page_size;
	pclose(fp);

	fp=popen("cat /proc/meminfo | grep HugePages_Total | awk '{print $2}' ","r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
			"popen failed: errno(%d)\n", errno);
		goto error_exit;
	}
	ret=fscanf(fp,"%lu\n",&mem_info.pdata[huge_page_idx].avail);
	if (ret == 0 || ret == EOF) {
    	displaym(HTX_HE_INFO, DBG_MUST_PRINT,
    			 "Could not read num avail HugePages_Total  from pipe (HugePages_Total  not found)\n");
	 mem_info.lpage_type = FALSE;
	}
	pclose(fp);
	if (!vrm_enabled && ((huge_page_size == (2 * MB)) || (huge_page_size == (16 *MB)))) {
		fp=popen("cat /proc/meminfo | grep HugePages_Free | awk '{print $2}' ","r");
		if (fp == NULL) {
			displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				"popen failed: errno(%d)\n", errno);
			goto error_exit;
		}
	ret=fscanf(fp,"%lu\n",&mem_info.pdata[huge_page_idx].free);
	if (ret == 0 || ret == EOF) {
		displaym(HTX_HE_INFO, DBG_MUST_PRINT,
			"Could not read num free HugePages_Free from pipe(HugePages_Free not found)\n");
	}
	pclose(fp);

	char log_dir[256];
	strcpy(log_dir,stats.htx_log_dir);
	strcat(log_dir,"/freepages");
	if ((fp=fopen(log_dir,"r"))==NULL) {
		displaym(HTX_HE_INFO,DBG_MUST_PRINT, "fopen of memory free pages (%s) failed with errno:%d\n",log_dir,errno);
	}
	else {
		ret=fscanf(fp,"avail_16M=%lu,avail_16G=%lu,free_16M=%lu,free_16G=%lu",&avail_16m,\
				&avail_16g,&free_16m,&free_16g);
		if (ret == 0 || ret == EOF) {
			displaym(HTX_HE_INFO,DBG_MUST_PRINT, "Error while reading file (%s) \n");
			file_exists=0;
		}
		else {
			file_exists=1;
		}
		fclose(fp);
	}
	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%s  free_16M = %lu \t avail_16M = %lu \n"
						,log_dir,free_16m,avail_16m);

	} /* if ! vrm_enabled */
	else {
		mem_info.lpage_type = FALSE;
	}
#else
	/* Setting the lpage type to 0; meaning no support for large pages */
    mem_info.lpage_type = FALSE;

#endif

	/* Enable support for base page size; fill in data structure */
	mem_info.pdata[base_pg_idx].supported = TRUE;
	mem_info.pdata[base_pg_idx].psize = mem_info.base_page_sz;

    mem_info.pspace_avail = KB * mem_info.pspace_avail;
    mem_info.pspace_free = KB * mem_info.pspace_free;
    mem_info.total_mem_avail = KB * mem_info.total_mem_avail;
    mem_info.total_mem_free = KB * mem_info.total_mem_free;
    mem_info.pdata[base_pg_idx].avail = KB * mem_info.pdata[base_pg_idx].avail;
    mem_info.pdata[base_pg_idx].free = KB * mem_info.pdata[base_pg_idx].free;
	if (mem_info.lpage_type == TRUE) {
		mem_info.pdata[huge_page_idx].avail = huge_page_size * mem_info.pdata[huge_page_idx].avail;
		mem_info.pdata[huge_page_idx].free =  huge_page_size * mem_info.pdata[huge_page_idx].free;
        
		mem_info.pdata[huge_page_idx].supported = TRUE;
		if( mem_info.pdata[huge_page_idx].psize != (huge_page_size)) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"huge_page_size = %d, unsupported huge page size, exiting!\n",mem_info.pdata[huge_page_idx].psize);
			exit(1);
		}
                if(file_exists){
                mem_info.pdata[huge_page_idx].avail = huge_page_size * avail_16m;
                mem_info.pdata[huge_page_idx].free = huge_page_size * (free_16m);
                }
	}

/* XXXXXX */

    display(HTX_HE_INFO,DBG_MUST_PRINT,"ps_avail=%lu\nps_free =%lu\n",\
								mem_info.pspace_avail,mem_info.pspace_free);

    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"tot_mem_avail=%lu\n",mem_info.total_mem_avail);

    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"tot_mem_free =%lu\n",mem_info.total_mem_free);

    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"64k/4k mem avail =%lu\n",mem_info.pdata[base_pg_idx].avail);

    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"64k/4k mem free  =%lu\n",mem_info.pdata[base_pg_idx].free);

	if ( mem_info.lpage_type ) {
    	displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Large page is supported \n");
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Large page size  = %lu\n",mem_info.pdata[huge_page_idx].psize);
    	displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Large page avail = %lu\n",mem_info.pdata[huge_page_idx].avail);
    	displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Large page free  = %lu\n",mem_info.pdata[huge_page_idx].free);
	}

   	for (i = 0; i < MAX_NUM_PAGE_SIZES; i++){
    	if (mem_info.pdata[i].supported == 1 && mem_info.pdata[i].free == 0) {
        	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"[%d]:There are no free pages for this supported page size(%s) "
            	"So continuing with the next page size\n",__LINE__, page_size[i]);
    	}
	}

    return(0);

error_exit:
	if (fp != NULL) {
		pclose(fp);
		fp = NULL;
	}
    exit(1);

}

/****************************************************************************
 *****************************************************************************
 LINUX SPECIFIC MEMORY STATISTICS COLLECTION FUNCTIONS END HERE
*****************************************************************************
*****************************************************************************/
#endif


/*****************************************************************************
*   read_cmd_line function reads the main programs command line
*   params into the priv data structure variable
*****************************************************************************/
int read_cmd_line(int argc,char *argv[]) {
    int i=-1,j=0;
    char tmp[30],msg[200];

    priv.sig_flag = 0;
	priv.affinity_flag = 0;

    /* Collecting the command line arguments */
    if (argc > 1)
        strcpy (stats.sdev_id, argv[1]);
    else
        strcpy (stats.sdev_id, "/dev/mem");

    if (argc > 2) {
        strcpy (stats.run_type, argv[2]);
        for (i = 0; (i < 4) && (stats.run_type[i] != '\0'); i++)
            stats.run_type[i] = toupper (stats.run_type[i]);

        if ((strcmp (stats.run_type, "EMC") != 0) &&(strcmp (stats.run_type, "REG") != 0))
        {
            strcpy (stats.run_type, "OTH");
            priv.standalone = 1;
        }
    }
    else  {
        strcpy (stats.run_type, "OTH");
        priv.standalone = 1;
    }

    strcpy (stats.HE_name, argv[0]);
    if (argc > 3) {
        strcpy (priv.rules_file_name, argv[3]);
    }
    else { /*Default value */
        char* rules_tmp_ptr = getenv("HTXREGRULES");
        if(rules_tmp_ptr == NULL){
            displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s(): env variable HTXREGRULES is not set,rules_tmp_ptr=%p"
            ", thus exiting..\n",__LINE__,__FUNCTION__,rules_tmp_ptr);
            exit(1);
        }else{
            strcpy(priv.rules_file_name,rules_tmp_ptr);
        }
		strcat(priv.rules_file_name,"/hxemem64/maxmem");
    }

    if (argc > 4) {
        sscanf(argv[4],"%d",&priv.debug);
    }
    else {
        priv.debug=0;
    }

    /*  Stored the device id, run_type and Hardware exerciser name in priv data structure
    */
    strcpy(priv.dev_id, stats.sdev_id);
    strcpy(priv.run_type, stats.run_type);
    strcpy(priv.exer_name, stats.HE_name);

/*
 *  Environment Variable stating whether to enter kdb in case if miscompare.
 */
    htxkdblevel = getenv("HTXKDBLEVEL");
    if (htxkdblevel) {
        priv.htxkdblevel = atoi(htxkdblevel);
    }
    else {
        priv.htxkdblevel = 0;
    }

    /* Get the logical processor number in the device name like /dev/mem2 means
    2nd logical procesor
    */
    sscanf(priv.dev_id,"%[^0-9]s",tmp);
    strcat(tmp,"%d");

    i = UNSPECIFIED_LCPU;
    if ((j=sscanf(priv.dev_id,tmp,&i)) < 1) {
        strcpy(msg,"read_cmd_line: No binding specified with the device name.\n");
        if(priv.debug == DBG_DEBUG_PRINT) {
            sprintf(msg,"%sDebug info:%s,i=%d,j=%d\n",msg,tmp,i,j);
        }
        i = UNSPECIFIED_LCPU;
    }
    else {
        sprintf(msg,"Binding is specified proc=%i\n",i);
    }

    if(priv.standalone == 1) {
        displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%s",msg);
    }
    priv.bind_proc=i;
    return(0);

} /* End of read_cmd_line() function */


/*****************************************************************************
*      display(int sev,int debug, const char *format) which print the message on
*   the screen for stand alone mode run or in the /tmp/htxmsg file otherwise.
*****************************************************************************/
int displaym(int sev,int  debug, const char *format,...) {
    va_list varlist;
    char msg[4096];
    int err; /* error number */

   /* Collect the present error number into err local variable*/
    err = errno;

   /*
    * If debug level is 0 (MUST PRINT) we sud always print.
    * If debug level is > 0  and level of this print is less or equal to
    * priv.debug (cmd line global debug level variable) then display this message else
    * don't display this message. And zero can't be greater than any other debug level
    * so it always prints below.*/
    if( debug > priv.debug ) {
            return(0);
     }

    va_start(varlist,format);
    if (mem_info.mutex)
        pthread_mutex_lock(&mem_info.tmutex);

    if (priv.standalone == 1) {
        vprintf(format,varlist);
        fflush(stdout);
    }
    else {
        vsprintf(msg,format,varlist);
        hxfmsg(&stats,err,sev,msg);
    }

    if (mem_info.mutex)
        pthread_mutex_unlock(&mem_info.tmutex);

    va_end(varlist);
    return(0);
}

/***********************************************************************
* SIGTERM signal handler                                               *
************************************************************************/

void SIGTERM_hdl (int sig, int code, struct sigcontext *scp)
{
    /* Don't use display function inside signal handler.
     * the mutex lock call will never return and process
     * will be stuck in the display function */
    hxfmsg(&stats,0,HTX_HE_INFO,"hxemem64: Sigterm Received!\n");
    priv.exit_flag = 1;
}

/************************************************************************
*		SIGUSR2 signal handler for cpu hotplug add/remove 				*
*************************************************************************/
#ifdef __HTX_LINUX__
void SIGUSR2_hdl(int sig, int code, struct sigcontext *scp)
{
	int i, rc = 0;

	hxfmsg(&stats,0,HTX_HE_INFO,"Received SIGUSR2 signal\n");

	if (priv.affinity_flag == 1 ) {
		/*for (i=0; i < mem_info.num_of_threads; i++) {
			if (mem_info.tdata_hp[i].tid != -1) {
				rc = check_cpu_status_sysfs(mem_info.tdata_hp[i].physical_cpu);
				if (rc = 0) {
					displaym(HTX_HE_INFO,DBG_MUST_PRINT," cpu %d has been removed, thread:%d exiting in affinity yes case \n",mem_info.tdata_hp[i].physical_cpu,mem_info.tdata_hp[i].thread_num);
					pthread_cancel(mem_info.tdata_hp[i].tid);
				}
			}
		}*/
		update_srad_data_flag = 1;
	}
	update_sys_detail_flag = 1;
}
#endif
/***********************************************************************
* This function does the binding of the process to the logical processor
* specified by priv.bind_proc variable which is filled while reading
* command line parameters.
* id - 0 (BIND_TO_PROCESS), 1 (thread bind)
* bind_proc = logical processor number or -1 (to unbind)
***********************************************************************/
int do_the_bind_proc(int id,int bind_proc, int pcpu)
{
    int rc=0;
    pid_t              pid;
    pthread_t          tid;
    int                ti;

    if (stanza_ptr->run_mode == RUN_MODE_NORMAL) {
        /* for RUN_MODE_NORMAL, num_of_threads is always 1 */
        ti = 0x0;
    } else {
        ti = bind_proc;
    }

    if (bind_proc != UNBIND_ENTITY) {
 	    bind_proc = bind_proc%get_num_of_proc();
    }

    pid=getpid();
    #ifndef __HTX_LINUX__
    if (bind_proc == UNBIND_ENTITY) {
        bind_proc = PROCESSOR_CLASS_ANY;
    }
    if (id ==  BIND_TO_PROCESS) {
        rc = bindprocessor(BINDPROCESS, pid, bind_proc);
    } else {
        tid = thread_self();
        rc = bindprocessor(BINDTHREAD, tid, bind_proc);
		if ( ti != UNBIND_ENTITY) {
        	mem_info.tdata_hp[ti].kernel_tid = tid;
    		mem_info.tdata_hp[ti].bind_proc = bind_proc;
		}
    }

    #else
    if (id == BIND_TO_PROCESS) {
        pid=getpid();
    } else {
        tid=pthread_self();
    }
    /*if (oldmask_collected == 0 ) {
        if (id == BIND_TO_PROCESS) {
            rc = sched_getaffinity(pid, sizeof(cpu_set_t), &oldcpumask);
        } else {
            rc = pthread_getaffinity_np(tid, sizeof(cpu_set_t), &oldcpumask);
        }
        oldmask_collected = 1;
    }*/

    if (bind_proc != UNBIND_ENTITY ) {
		/*
        CPU_ZERO(&newcpumask);
        phy_cpu = get_logical_2_physical(bind_proc);
        if (phy_cpu != -1) {
            CPU_SET(phy_cpu, &newcpumask);
        } else {
            displaym(HTX_HE_INFO, DBG_MUST_PRINT,"Warning! Error converting "
                     "logical cpu number to physical cpu number for cpu%d.\n", bind_proc);
            newcpumask = oldcpumask;
        }X_THREADS

        if (id == BIND_TO_PROCESS) {
            rc = sched_setaffinity(pid, sizeof(cpu_set_t), &newcpumask);
        } else {
			rc = pthread_setaffinity_np(tid, sizeof(cpu_set_t),&newcpumask);
        }*/
		if(physical_cpus[ti] != -1 && stanza_ptr->operation != OPER_TLB && stanza_ptr->operation != OPER_L4_ROLL && stanza_ptr->operation != OPER_MBA) {
			pcpu = physical_cpus[ti];
		}
		displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"physical_cpus[%d]=%d and pcpu = %d\n",ti,physical_cpus[ti],pcpu);
		rc = htx_bind_thread(bind_proc,pcpu);
		/*printf("bind_proc = %d   pcpu = %d   rc = %d\n",bind_proc,pcpu,rc);*/
		if ( rc < 0) {
			if (rc == -2 || rc == -1) {
				if (id == BIND_TO_THREAD) {
					displaym(HTX_HE_INFO,DBG_IMP_PRINT,"lcpu : %d ,pcpu: %d has been hot removed\n",bind_proc,pcpu);
				}
				else {
					displaym(HTX_HE_INFO,DBG_IMP_PRINT,"lcpu=%d,pcpu=%d  has been removed\n", bind_proc,pcpu);
				}
			} else {
				displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "Error binding to processor lcpu=%d, bcpu = %d,rc =%d\n", bind_proc,pcpu,rc);
			}
		}
		else {
			if(physical_cpus[ti] == -1 && stanza_ptr->operation != OPER_TLB && stanza_ptr->operation != OPER_L4_ROLL && stanza_ptr->operation != OPER_MBA){
            	physical_cpus[ti]= rc;
			}
		}
		/*printf("returning rc =%d\n",rc);*/
		return rc;
    } else {
		rc = htx_unbind_thread();
		if ( rc != 0) {
			displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, " htx_unbind_thread() failed with rc = %d\n", rc );
		}
    }
    #endif

    if (rc != 0) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Warning! error binding "
                 "to processor %d.", bind_proc);
    } else {
        display(HTX_HE_INFO,DBG_INFO_PRINT,"Successfully binded process "
                "(pid =%d) to the logical proc %d\n", pid, bind_proc);
    }

    return(rc);
}

/* runs any aix command and returns command output */
int run_aix_cmd( char * cmd_name, char * result, int size)
{
    FILE  *fp;
    int i, fgets_success=FALSE, retries=5;

/* Note: fgets returns NULL when sigterm is received and no bytes have been
   read. If this happens retry a few times */

    for (i=0; i<retries; i++) {
        fflush(stdout);
        errno = 0;  /* Reset the error number */
        if ((fp = popen(cmd_name, "r")) == NULL) {
             displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"run_aix_cmd: command : %s popen error(%d) \n", cmd_name,errno);
            return -1;
        }

        if (fgets(result,size,fp) != NULL) {
            fgets_success = TRUE;
              pclose(fp);
              break;
        } else
            pclose(fp);
    } /* end for */

    return fgets_success;
}

/*
 * Function Name : get_sonoras()
 *
 * Description: Determines the number of Sonora graphic adapters.
 *              It is called to calculate an additional 16Mb per
 *              adapter for malloc'ing by the memory hogging TU's
 *
 * Note:
 *
 */

int get_sonoras(void)
{
  int  sonoras;
  char buf[1024], msg[221];
  FILE  *fp;

   if ((fp = popen("/usr/sbin/lsdev -C -Sa|grep son|wc -l|awk '{print $1}'", "r")) == NULL) {
      sprintf(stats.msg_text,"get_sonoras: popen error(%d) in finding # of available sonora graphics adapters\n", errno);
      strcat(stats.msg_text,strerror(errno));
      displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s",stats.msg_text);
      return(1);
   }
   if ( fgets(buf, 1024, fp) == NULL ) {
      strcpy(stats.msg_text, "fgets error getting # of available graphics adapters - ");
      strcat(stats.msg_text,strerror(errno));
      strcat(stats.msg_text, "\n");
      displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s",stats.msg_text);
      return(1);
   }
   sscanf(buf, "%d", &sonoras);
   if ( (pclose(fp)) == -1 ) {
      strcpy(stats.msg_text, "pclose error getting # of available graphics adapters - ");
      strcat(stats.msg_text,strerror(errno));
      strcat(msg, "\n");
      displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s",stats.msg_text);
      return(1);
   }
   return(sonoras);
}

/*
 * Function Name: allocate_buffers
 *
 * Description:
 *    For all threads based on no of segments for each page sizes and calls shared
 *    memory segment creation funcion.
 * Input:
 *    Nothing
 * Output:
 *    return 0 on success.
 * Notes:
 */
int allocate_buffers()
{
    int i,j=0,allocate_flag=0;

	for (j=0; j < mem_info.num_of_threads; j++){

      /* In case of affinity = yes check mem_info.tdata_hp for cpu_flag to make sure we are not accessing any holes */
     if(stanza_ptr->affinity == TRUE && mem_info.tdata_hp[j].cpu_flag != 1) {
	   	  continue;
     }

     /*
      * Bind this process to the physical processor for concurrent mode
      * Unbind after each allocate_shm_with_specified_page_size* call below
      * This would help the exerciser use as much local memory as possible.
      * Current implementation uses the vm_mem_policy() sys call with early_lru=1 setting
      */
/*    if (r.run_mode == RUN_MODE_CONCURRENT) {*/
	if ((stanza_ptr->run_mode == RUN_MODE_CONCURRENT) && (stanza_ptr->bind_proc == TRUE)) {
        do_the_bind_proc(BIND_TO_PROCESS, j,-1);
    }
    for(i = 0; i < MAX_NUM_PAGE_SIZES; i++) {
        if (mem_info.pdata[i].supported && mem_info.tdata_hp[j].page_wise_t[i].num_of_segments) {
            display(HTX_HE_INFO,DBG_IMP_PRINT,"Before allocate_shm_with_specified_page_size function(%d)"
                "segs=%d, page size = %s\n ",i,
                mem_info.tdata_hp[j].page_wise_t[i].num_of_segments, page_size[i]);
#ifndef __HTX_LINUX__
            if ( allocate_shm_with_specified_page_size(j,i) != 0)
#else
            if ( allocate_shm_with_specified_page_size_linux(j,i) != 0)
#endif
            {
                return(-1);
            }
            allocate_flag = 1;
        }
    } /* End of for loop of page sizes */
    if (stanza_ptr->run_mode == RUN_MODE_CONCURRENT) {
        do_the_bind_proc(BIND_TO_PROCESS, UNBIND_ENTITY,-1);
    }
} /* End for loop of num_of_threads */

    /* allocate_flag = 0 signifies the stanza rule is such that we couldn't allocate at least one segment,
     * Most probable the rule's number of segments for 4k is wrong or sth in the stanza is wrong
     */

    if(allocate_flag == 0 && (stanza_ptr->operation != OPER_MPSS ) ) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"For the current stanza, stanza rule_id=%s, No segments could be"
                 "allocated. Check the number of segments and other fields."
                "So skipping this stanza (Check your rules file)\n ",stanza_ptr->rule_id);
        return(-1);
    }

    return(0);
} /* end allocate_thread_buffers */

#ifndef __HTX_LINUX__
/*
 * Function Name: allocate_shm_with_specified_page_size
 *
 * Description:
 *    Creates specified no of shared memory segments with specified page
 *    size. It also pins the pages if needed. Attaches to shared memory.
 *
 * Input:
 *    ti - thread index
 *    page_size_index - specifies if the page size is 4K, 64K, 16M or 16G.
 *
 * Output:
 *    returns 0 on success.
 * Notes:
 */
int
allocate_shm_with_specified_page_size(int ti,  int page_size_index)
{
    int i, memflg,rc, tmp_errno, try_again =1;
    char msg[1024];
    char **shr_mem_ptr;
    int *pshmid;
    unsigned int pvr;
    unsigned long *shm_size_ptr=mem_info.tdata_hp[ti].page_wise_t[page_size_index].shm_sizes;
	int rc1=0,rc2=0,rc3=0;

#ifdef SET_PAGESIZE
    struct vminfo_psize vminfo_64k = { 0 };
    struct shmid_ds shm_buf = { 0 };
#endif

#ifndef AIX43X
    psize_t psize;
#else
    unsigned long psize;
#endif

    display(HTX_HE_INFO,DBG_IMP_PRINT,"Start of allocate_shm_with_specified_page_size(%d)\n",page_size_index);
#ifndef MMAP 
    if ( (mem_info.tdata_hp[ti].page_wise_t[page_size_index].shmids=(int *)\
        malloc(sizeof(int )*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments)) == NULL) 
#else
	mem_info.tdata_hp[ti].page_wise_t[page_size_index].shmids=(int *)\
		mmap(NULL,(sizeof(int )*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments),PROT_READ | PROT_WRITE | PROT_EXEC,MAP_ANONYMOUS,-1, 0);
	if(mem_info.tdata_hp[ti].page_wise_t[page_size_index].shmids == -1)
#endif
    {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"malloc for shr_mem_ptr_hp errored with %d \n",errno);
        return(-1);
    }

    /* Mallocating the pointers for shared memory pointers and id pointers */
#ifndef MMAP
    if ( (mem_info.tdata_hp[ti].page_wise_t[page_size_index].shr_mem_hptr=(char **)\
        malloc((sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments))) == NULL) 
#else

	mem_info.tdata_hp[ti].page_wise_t[page_size_index].shr_mem_hptr=(char **)mmap(NULL,(sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments),\
	PROT_READ | PROT_WRITE | PROT_EXEC,MAP_ANONYMOUS,-1, 0);
	if(mem_info.tdata_hp[ti].page_wise_t[page_size_index].shr_mem_hptr == -1)
#endif
    	{
        	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"malloc for shr_mem_ptr_hp errored with %d \n",errno);
        	return(-1);
    	}

    pshmid = mem_info.tdata_hp[ti].page_wise_t[page_size_index].shmids;
    shr_mem_ptr = mem_info.tdata_hp[ti].page_wise_t[page_size_index].shr_mem_hptr;
#ifdef MMAP
	rc1 = mprotect (shr_mem_ptr,(sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments),PROT_WRITE | PROT_READ | PROT_EXEC);
	rc2 = mprotect (pshmid,(sizeof(int)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments),PROT_WRITE | PROT_READ | PROT_EXEC);
	if( (rc1 | rc2) != 0 )
	{
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]Error (%d) mprotect() failed with rc1 = %d rc2=%d for thread=%d and length=%d\n",
			__LINE__,errno, rc1,rc2,ti,(sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments));
	}
#endif

    /* Making all the shared memory pointers NULL first */
    memset(mem_info.tdata_hp[ti].page_wise_t[page_size_index].shr_mem_hptr, 0x00 ,\
                            (sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments));

    memflg = (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (page_size_index == PAGE_INDEX_4K) psize = (4*KB);
    else if (page_size_index == PAGE_INDEX_64K) psize = (64*KB);
    else if (page_size_index == PAGE_INDEX_16M) {
        psize = (16*MB);
        #ifndef SET_PAGESIZE
        memflg |= (SHM_LGPAGE | SHM_PIN);
        #endif
    }
    else if (page_size_index == PAGE_INDEX_16G) {
        psize = (16*GB);
    }


    for (i = 0; i < mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments;  i++) {
        errno = 0;
        pshmid[i] = shmget (IPC_PRIVATE, shm_size_ptr[i], memflg);
        if (pshmid[i] == -1) {
            tmp_errno = errno; /*save error number */
            errno = 0;
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Error (%d) getting shared memory(%d) - (%ld bytes) %s"
              " and page size %s\n",tmp_errno,i,shm_size_ptr[i],msg,page_size[page_size_index]);
            return(-1);
        } /* endif */
        else {
            display(HTX_HE_INFO,DBG_INFO_PRINT,"created shared memory -  (%ld bytes),"
                " shmid[%d] = %d\n",shm_size_ptr[i],i,pshmid[i]);
        }

#ifdef SET_PAGESIZE
        if ( shm_size_ptr[i] > (256*MB)) {
            if (shmctl(pshmid[i], SHM_GETLBA, &shm_buf))   {
                tmp_errno = errno;
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Error (%d): seg_num = %d Getting LBA for "
                       "page size (0x%llx)  (0x%llx bytes)\n",tmp_errno,i, psize, shm_size_ptr[i] );
                return(-2);
            }
        }
#endif

#ifdef SET_PAGESIZE
        if (page_size_index == PAGE_INDEX_16G) {
            shm_buf.shm_pagesize = (16*GB);
        }
		else {
            shm_buf.shm_pagesize = psize;
        }
/* Below code is added due to tlbie limitation on P7 hardware. If 4K pages are large, no. of tlbies floating become huge
   Which slows down the system performance. This mainly happens on MFG system with large configuration. See defect 882528
   for more detail */
		if (mem_info.pdata[PAGE_INDEX_64K].supported == TRUE) {
			pvr = (unsigned int) getTruePvr();
			pvr = pvr >> 16;
			if ((page_size_index == PAGE_INDEX_4K) && (pvr  == 0x3f || pvr == 0x4a)) {
				shm_buf.shm_pagesize = (64*KB);
			}
		}

        if (shmctl(pshmid[i], SHM_PAGESIZE, &shm_buf))    {
            tmp_errno = errno;
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Error (%d) seg_num = %d, setting page size (0x%llx),"
                "(0x%llx bytes)\n",tmp_errno, i,psize, shm_size_ptr[i]);
            return(-3);
        }
        else {
            display(HTX_HE_INFO,DBG_INFO_PRINT,"shmctl done - shmid[%d] = %d page size = 0x%llx\n",i,\
                        pshmid[i], shm_buf.shm_pagesize);
        }
#endif

/*
 * The SHMAT system call attaches the shared memory segment associated
 * with the shared memory identifier mem_id (from the SHMGET system
 * call).
 */
        errno = 0;
        shr_mem_ptr[i] = (void *) shmat(pshmid[i], (char *) 0, 0);
        if ( (int)shr_mem_ptr[i] == -1) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Error (%d) attaching shared memory - shmid[%d] = %d"
                   "page size %s \n", errno,i, pshmid[i], page_size[page_size_index]);
            return(-4);
        }


        if (strcmp(stanza_ptr->messages,"YES") == 0) {
            display(HTX_HE_INFO,DBG_IMP_PRINT,"Allocated shared memory buffer %d\n , id = %d, shr_memp = "
                "0x%lx and page size =%s\n",i,pshmid[i],shr_mem_ptr[i], page_size[page_size_index]);
        } /* endif */

    /*
     * Pin the memory if page size is 16MB. Paging for 16 MB pages is not
     * supported by AIX kernel. This could be true for 16 GB pages also.
     * 16 GB pages are supported from AIX53E and later kernel.
     */

/* Removing mlock() as it is slowing downing exer progress on MFG large mem configured systems */
#if 0
#ifdef SET_PAGESIZE
        if (page_size_index > PAGE_INDEX_64K) {
            if (mlock(shr_mem_ptr[i], shm_size_ptr[i]) == -1 ) {
                tmp_errno = errno;
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Error (%d) seg_num= %d, "
                    "pinning page size (%s) -(%ld bytes) addr = 0x%lx \n",\
                    tmp_errno, i, page_size[page_size_index], shm_size_ptr[i], shr_mem_ptr[i]);
                return(-5);
            }
        }
#endif
#endif

    } /* end of for loop*/
#ifdef MMAP
	rc1 = mprotect (mem_info.tdata_hp[ti].page_wise_t[page_size_index].shr_mem_hptr,(sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments),PROT_READ | PROT_EXEC);
	rc2 = mprotect (mem_info.tdata_hp[ti].page_wise_t[page_size_index].shmids,(sizeof(int)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments),PROT_READ | PROT_EXEC);
	if( (rc1 | rc2 ) != 0 )
	{
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"(%d)Error (%d) mprotect() failed with rc1 = %d rc2 = %d ti=%d and length=%d\n",
			__LINE__,errno, rc1,rc2,ti,(sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments));
	}

#endif
    return(0);
}

#else

/*
 * Function Name: allocate_shm_with_specified_page_size_linux
 *
 * Description:
 *    Creates specified no of shared memory segments with specified page
 *    size. It also pins the pages if needed. Attaches to shared memory.
 *
 * Input:
 *   ti - thread index
 *    page_size_index - specifies if the page size is 4K, 64K, 16M or 16G.
 *
 * Output:
 *    returns 0 on success.
 * Notes:
 */
int allocate_shm_with_specified_page_size_linux( int ti, int page_size_index)
{
    int i, memflg, tmp_errno;
    char msg[1024];
    char **shr_mem_ptr;
    int *pshmid;
    unsigned long *shm_size_ptr=mem_info.tdata_hp[ti].page_wise_t[page_size_index].shm_sizes;

    display(HTX_HE_INFO,DBG_IMP_PRINT,"Start of allocate_shm_with_specified_page_size(%d)\n",page_size_index);

    if ( (mem_info.tdata_hp[ti].page_wise_t[page_size_index].shmids=(int *)\
        malloc(sizeof(int)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments)) == NULL)
    {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"malloc for shr_mem_ptr_hp errored with %d \n",errno);
        return(-1);
    }

    /* Mallocating the pointers for shared memory pointers , key and id pointers */
    if ( (mem_info.tdata_hp[ti].page_wise_t[page_size_index].shr_mem_hptr=(char **)\
        malloc(sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments)) == NULL)
    {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"malloc for shr_mem_ptr_hp errored with %d \n",errno);
        return(-1);
    }

    pshmid = mem_info.tdata_hp[ti].page_wise_t[page_size_index].shmids;
    shr_mem_ptr = mem_info.tdata_hp[ti].page_wise_t[page_size_index].shr_mem_hptr;

    /* Making all the shared memory pointers NULL first */
    memset(mem_info.tdata_hp[ti].page_wise_t[page_size_index].shr_mem_hptr, 0x00 ,\
                        (sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments));

    memflg = (IPC_CREAT | IPC_EXCL |SHM_R | SHM_W);
    if ((page_size_index == PAGE_INDEX_16M) || (page_size_index == PAGE_INDEX_2M)) {
        #ifdef KERNEL_2_6
        memflg |= (SHM_HUGETLB);
        #endif
    }

    for (i = 0; i < mem_info.tdata_hp[ti].page_wise_t[page_size_index].num_of_segments;  i++) {
    /*
    * mem0 keys = 20000 - 20999
    * mem1 keys = 21000 - 21999
    * ...
    */
        errno = 0;
        pshmid[i] = shmget (IPC_PRIVATE, shm_size_ptr[i], memflg);
        if (pshmid[i] == -1) {
            tmp_errno = errno; /*save error number */
            errno = 0;
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Error (%d) getting shared memory(%d) - (%ld bytes) %s"
              " and page size %s\n",tmp_errno,i,shm_size_ptr[i],msg,page_size[page_size_index]);
            return(-1);
        } /* endif */
        else {
            display(HTX_HE_INFO,DBG_INFO_PRINT,"created shared memory -  (%ld bytes),"
            " shmid[%d] = %d\n",shm_size_ptr[i],i,pshmid[i]);
    }

/*
 * The SHMAT system call attaches the shared memory segment associated
 * with the shared memory identifier mem_id (from the SHMGET system
 * call).
 */

        errno = 0;
        shr_mem_ptr[i] = (void *) shmat(pshmid[i], (char *) 0, 0);
        if ( (int *)shr_mem_ptr[i] == (int *)-1) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Error (%d) attaching thread shared memory - shmid[%d] = %d"
                   "page size %s \n", errno,i, pshmid[i], page_size[page_size_index]);
            return(-4);
        }

        if (strcmp(stanza_ptr->messages,"YES") == 0) {
            display(HTX_HE_INFO,DBG_IMP_PRINT,"Allocated shared memory buffer %d\n,tid = %d, id = %d, shr_memp = "
            "0x%lx and page size =%s\n",ti,i,pshmid[i],shr_mem_ptr[i], page_size[page_size_index]);
        } /* endif */
    } /* end for (i = 0; i < mem_info.pdata[page_size_index].num_of_segments;  i++).......*/

    return(0);
}


#endif

/************************************************************************
* thread fill buffers by reading pattern file to check dma                     *
************************************************************************/
int fill_buffers(int ti)
{
    long i,j,pi=0;
    int ps,rc,chars_read,fildes,file_size,mode_flag;
    unsigned long seed = 0;
    char *ptr;
    char pattern_nm[256];
    unsigned long pat_size;

    char **shr_mem_ptr;
    int *pshmid;
    unsigned long *shm_size_ptr;
    struct stat fstat;
    int nr=0,nw=0,nc=0;


    char* pat_tmp_ptr = getenv("HTXPATTERNS");
    if(pat_tmp_ptr == NULL){
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s(): env variable HTXPATTERNS is not set,pat_tmp_ptr=%p"
        ", thus exiting..\n",__LINE__,__FUNCTION__,pat_tmp_ptr);
        return(-1);
    }else{
        strcpy(pattern_nm,pat_tmp_ptr);
    }
    strcat(pattern_nm,stanza_ptr->pattern_name[0]);
    pat_size = stanza_ptr->pattern_size[0];
    unsigned int * seed1,* seed2;
    unsigned int j1=12, j2=13;
    int bind_cpu_rand;
    seed1 = &j1;
    seed2 = &j2;

    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"\n(T=%d)Entered fill_buffers, pattern name=%s,nam=%s , size=%u \n",\
            ti,pattern_nm,stanza_ptr->pattern_name[0],pat_size);


    mode_flag = S_IWUSR | S_IWGRP | S_IWOTH;
    /* Opening the file in Direct I/O mode ( or what can be called RAW mode ) */
    #ifndef __HTX_BML__
    if ((fildes = open(pattern_nm, O_DIRECT | O_RDONLY , mode_flag)) == -1) {
    #else
    if ((fildes = open(pattern_nm, O_RDONLY , mode_flag)) == -1) {
    #endif

        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"T(%d):Cannot open pattern file - %s,errno:%d\n",\
            ti,pattern_nm,errno);
        return(1);
    } /* endif */

    for (i = stanza_ptr->num_oper; i > 0; i--) {
		for (ps = 0; ps < MAX_NUM_PAGE_SIZES; ps++) {
        	shr_mem_ptr =mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr;
        	pshmid=mem_info.tdata_hp[ti].page_wise_t[ps].shmids;
        	shm_size_ptr=mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes;

        	if (mem_info.pdata[ps].supported) {/*If the present page size is supported */

        	    display(HTX_HE_INFO,DBG_IMP_PRINT,"T(%d):Filling the shared memory segments of page size(%s)"
                            "with pattern name =%s , num of segments =%d, num_oper = %d \n",
                           ti,page_size[ps],stanza_ptr->pattern_name[0],\
                            mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments, \
                            stanza_ptr->num_oper);

        	    if ( -1 ==  stat (pattern_nm, &fstat)) {
        	        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"T(%d):fill_buffers: Error occoured while getting "
                        "the pattern (%s) file stats: errno %d \n",ti,stanza_ptr->pattern_name[0],errno);
        	        return(-1);
        	    }
        	    else {
        	        file_size=fstat.st_size;
        	    } /* else ends */

        	    if (pat_size > file_size ) {
        	        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"T(%d):fill_buffers: "
                        "pat_size(%ld) cannot be greater than file's (%s) size %d",\
                        ti,pat_size, stanza_ptr->pattern_name[0],file_size);
        	        return(-1);
        	    }

                for (j=0; j<mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments; j=j+1) {
#ifndef __HTX_LINUX__
					if (mem_DR_done) {
						displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d is exiting for current stanza on mem DR operation \n",ti);
						release_thread_memory(ti);
						pthread_exit((void *)1);
					}
#endif
                    long seg_size = shm_size_ptr[j];
                    unsigned long total_bytes_read = 0; /* Temporary variable */
                    unsigned int read_size = 0;


		    if((stanza_ptr->num_writes)&& (stanza_ptr->bind_method == 1)&& (stanza_ptr->bind_proc == 0)){
                       bind_cpu_rand = rand_r(seed1);
                       bind_cpu_rand = bind_cpu_rand%get_num_of_proc();
		       rc = do_the_bind_proc(BIND_TO_THREAD, bind_cpu_rand,-1);
		       displaym(HTX_HE_INFO,DBG_INFO_PRINT,"bind_method:DMA: write bind  rand cpu = %d\n,errno = %d,\
                       bindprocrc=%d\n",bind_cpu_rand ,errno,rc);
                   }

                    for (nw=0; nw< stanza_ptr->num_writes; nw++) {
                        seg_size = shm_size_ptr[j];
                        total_bytes_read = 0; /* Temporary variable */
                        ptr = shr_mem_ptr[j];

                        display(HTX_HE_INFO,DBG_IMP_PRINT,"Filling the shared memory segments of page size(%s),"
                                "seg_ptr =0x%llx,seg_size=0x%llx, with pattern name =%s , seg num =%d,"
                                " num_oper = %d write count=%d,pat_size=%d \n",\
                                page_size[ps],ptr,seg_size,stanza_ptr->pattern_name[0], \
                                j,\
                                stanza_ptr->num_oper,nw,pat_size);
                        total_bytes_read = 0;
                        while ( seg_size > 0 ) {
                            /* Rewind file before each read */
                            if(total_bytes_read% pat_size == 0 ) {
                                rc = lseek(fildes,0,0);
                                if (rc == -1) {
                                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"fill_buffers: lseek error");
                                    return(-1);
                                }
                            }
                            if ( (seg_size/pat_size) == 0) {
                                read_size = seg_size%pat_size;
                            } else {
                                read_size = pat_size;
                            }

                            if ((chars_read = read(fildes, ptr, read_size)) == -1) {
                                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"error reading from disk(err:%s) - %s, "
                                "file_size =%d, chars_read =%d, err=%d, page size"
                                " =%s, seg_num=%ld, seg_addr=0x%llx\n", strerror(errno),\
                                stanza_ptr->pattern_name[0],file_size, chars_read,errno,\
                                page_size[ps],j,ptr);
                                return(-1);
                            }
                            if (priv.exit_flag == 1) {
								release_thread_memory(ti);
                                pthread_exit((void *)0);
                            } /* endif */
                            if (chars_read < read_size) {
                                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Not able to read from disk - %s, "
                                "properly patt_size=%d,file_size =%d, chars_read =%d, err=%d, page size =%s,"
                                " seg_num=%ld\n", \
                                stanza_ptr->pattern_name[0],pat_size,file_size, chars_read,errno,page_size[ps],j);
                                return(-1);
                            }
                            else {
                                STATS_HTX_UPDATE(UPDATE)
                            }

                            /* Now we have successfully read chars_read bytes */
                            total_bytes_read += chars_read;
                            ptr+=chars_read;
                            seg_size-=chars_read;
                            if ((total_bytes_read % mem_info.pdata[PAGE_INDEX_16M].psize) == 0) {
                                STATS_HTX_UPDATE(UPDATE)
                                /* hxfupdate(UPDATE,&stats); */
                            }
                        }/* ends while (seg_size != 0) */

                        display(HTX_HE_INFO,DBG_DEBUG_PRINT,"#fill_buffers, Filling"
                                " segment complete for write count %d\n",nw);

                        STATS_VAR_INC(bytes_writ, shm_size_ptr[j])
                        /* stats.bytes_writ += shm_size_ptr[j]; */
                    } /* for loop on num_writes is complete */

		    if((stanza_ptr->num_writes)&& (stanza_ptr->bind_method == 1)&& (stanza_ptr->bind_proc == 0)){
 			 rc = do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
			 displaym(HTX_HE_INFO,DBG_IMP_PRINT,"bind_method:DMA:write unbind \n, errno = %d,unbind \
			 rc = %d \n",errno,rc);
                    }

		    if((stanza_ptr->num_reads || stanza_ptr->num_compares)&& (stanza_ptr->bind_method == 1)&& \
                       (stanza_ptr->bind_proc == 0)){
                         bind_cpu_rand = rand_r(seed2);
                         bind_cpu_rand = bind_cpu_rand%get_num_of_proc();
			 rc = do_the_bind_proc(BIND_TO_THREAD, bind_cpu_rand,-1);
                   	 displaym(HTX_HE_INFO,DBG_INFO_PRINT,"bind_method:DMA: compare bind rand cpu = %d \n,\
			 errno = %d ,bindprocrc = %d\n",bind_cpu_rand ,errno,rc);
		    }

                    /*if (stanza_ptr->num_reads > 1 && stanza_ptr->num_reads != stanza_ptr->num_compares) {*/
                        for (nr=0; nr<stanza_ptr->num_reads;nr++) {
                            if (stanza_ptr->width == LS_DWORD) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"fill_buffers:"
                                        " Calling read_dword.. \n");
                                rc = mem_operation(READ_DWORD,\
                                        (seg_size)/LS_DWORD, \
                                        shr_mem_ptr[j],\
                                        stanza_ptr->pattern[pi], \
                                        0,\
                                        0, \
                                        stanza_ptr,\
                                        (unsigned long)&stanza_ptr->pattern_size[pi]);
                            }
                            else if( stanza_ptr->width == LS_WORD) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"fill_buffers:"
                                        " Calling  read_word.. \n");
                                rc = mem_operation(READ_WORD,\
                                        (seg_size)/LS_WORD,\
                                        shr_mem_ptr[j],\
                                        stanza_ptr->pattern[pi],\
                                        0, \
                                        0, \
                                        stanza_ptr,\
                                        (unsigned long)&stanza_ptr->pattern_size[pi]);
                            }
                            else if(stanza_ptr->width == LS_BYTE) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"fill_buffers: "
                                        "Calling read_byte.. \n");
                                rc = mem_operation(READ_BYTE,\
                                        (seg_size),\
                                        shr_mem_ptr[j],\
                                        stanza_ptr->pattern[pi],\
                                        0,\
                                        0,\
                                        stanza_ptr,\
                                        (unsigned long)&stanza_ptr->pattern_size[pi]);
                            }
                            if (priv.exit_flag == 1) {
								release_thread_memory(ti);
                                pthread_exit((void *)0);
                            } /* endif */
                        }


                    /* If compare is YES in the stanza then
                     *  Compare the present segment buffer which is just written with the pattern.
                     *  We assume the pattern files are 8 byte repeatative
                     */
                    for (nc=0; nc<stanza_ptr->num_compares;nc++) {
                        if (seg_comp_buffers_8(ti,j,ps,i,pi,seed) != 0) {
                            return(1);
                        }
                        display(HTX_HE_INFO,DBG_DEBUG_PRINT,"#fill_buffers, "
                                "Returned from seg_comp_buffers_8 (compare "
                                "count =%d)\n",nc);
                        if (priv.exit_flag == 1) {
							release_thread_memory(ti);
                            pthread_exit((void *)0);
                        } /* endif */
                    }

	           if((stanza_ptr->num_compares || stanza_ptr->num_reads)&& (stanza_ptr->bind_method == 1)&& \
                     (stanza_ptr->bind_proc == 0)){
		     rc = do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
                     displaym(HTX_HE_INFO,DBG_INFO_PRINT,"bind_method:DMA:compare unbind \n, errno = %d , rc = %d\n",
                     errno,rc);
                   }

                    if (priv.exit_flag == 1) {
						release_thread_memory(ti);
                        pthread_exit((void *)0);
                    } /* endif */
                    STATS_HTX_UPDATE(UPDATE)
                    /* hxfupdate(UPDATE,&stats); */
                } /* end for j < number of segments */
            } /* End of if page size is supported */
        }/* End of for ps < max page sizes */
    }/* end for i < num of operations  */
    close(fildes);
    return(0);
} /* end fill_buffers */

/***********************************************************************
* load a double word/word/byte  and compare immediately after the load
************************************************************************/
int store_readim_buffers(int ti) /* store and rd immediate*/
{
    int  i,j,k,ps;
    int pi=0;
    unsigned long rc=0;
    int trap_flag=0;
    static int main_misc_count=0;
    int nw=0;

    char **shr_mem_ptr ;
    int *pshmid;
    unsigned long *shm_size_ptr;
    unsigned long seg_size;
    struct segment_detail sd;
    unsigned long seed = stanza_ptr->seed;
    unsigned long seg_seed=0;

    if( (stanza_ptr->misc_crash_flag) && (priv.htxkdblevel) )
        trap_flag = 1;    /* Trap is set */
    else
        trap_flag = 0;    /* Trap is not set */

    if(stanza_ptr->attn_flag)
        trap_flag = 2;    /* attn is set */

    for (i = stanza_ptr->num_oper; i > 0; i--) {
		for (ps = 0; ps < MAX_NUM_PAGE_SIZES; ps++) {
        	if (mem_info.pdata[ps].supported != 1) {
        	    continue;
        	}
        	shr_mem_ptr =mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr;
        	pshmid=mem_info.tdata_hp[ti].page_wise_t[ps].shmids;
        	shm_size_ptr=mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes;

            for (j = 0; j< mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments;) {
#ifndef __HTX_LINUX__
				if (mem_DR_done) {
					displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d is exiting for current stanza on mem DR operation \n",ti);
					release_thread_memory(ti);
					pthread_exit((void *)1);
				}
#endif
                seg_seed = seed;
                for (nw=0; nw < stanza_ptr->num_writes; nw++) {
                    seed = seg_seed;
                    k=0;
                    /* Fill segment detail struct */
                    sd.seg_num = j;
                    sd.page_index = ps;
                    sd.width= stanza_ptr->width;
                    sd.seg_size = shm_size_ptr[j];
                    sd.sub_seg_num = -1;
                    sd.sub_seg_size = DEF_SEG_SZ_4K;

                    /* 16G page segment is logically operated in 256MB chunks
                     *  to test for RIM operation only*/
                    if ( ps == PAGE_INDEX_16G ) {
                        seg_size = sd.sub_seg_size;
                    } else {
                        seg_size = shm_size_ptr[j];
                    }

                    while ( k*seg_size < shm_size_ptr[j] ) {
                        sd.sub_seg_num = k;
                        display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                "Now Sub Segment number = %d\n",k);

                        if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                    " Calling wr_addr_cmp_dword.. \n");
                            rc = mem_operation(WR_ADDR_COMP,\
                                    (seg_size)/LS_DWORD, \
                                    shr_mem_ptr[j]+k*seg_size,\
                                    stanza_ptr->pattern[pi], \
                                    trap_flag,\
                                    &sd, \
                                    stanza_ptr,\
                                    (unsigned long)&stanza_ptr->pattern_size[pi]);
                        } else if( stanza_ptr->pattern_type[pi] == PATTERN_SIZE_NORMAL ) {
                            if (stanza_ptr->width == LS_DWORD) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                        " Calling wr_cmp_dword.. \n");
                                rc = mem_operation(RIM_DWORD,\
                                        (seg_size)/LS_DWORD, \
                                        shr_mem_ptr[j]+k*seg_size,\
                                        stanza_ptr->pattern[pi], \
                                        trap_flag,\
                                        &sd, \
                                        stanza_ptr,\
                                        (unsigned long)&stanza_ptr->pattern_size[pi]);
                            }
                            else if( stanza_ptr->width == LS_WORD) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                        " Calling  wr_cmp_word.. \n");
                                rc = mem_operation(RIM_WORD,\
                                        (seg_size)/LS_WORD,\
                                        shr_mem_ptr[j]+k*seg_size,\
                                        stanza_ptr->pattern[pi], \
                                        trap_flag, \
                                        &sd, \
                                        stanza_ptr,\
                                        (unsigned long)&stanza_ptr->pattern_size[pi]);
                            }
                            else if(stanza_ptr->width == LS_BYTE) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers: "
                                        "Calling wr_cmp_byte.. \n");
                                rc = mem_operation(RIM_BYTE,\
                                        (seg_size),\
                                        shr_mem_ptr[j]+k*seg_size,\
                                        stanza_ptr->pattern[pi], \
                                        trap_flag,\
                                        &sd,\
                                        stanza_ptr,\
                                        (unsigned long)&stanza_ptr->pattern_size[pi]);
                            }
                         } else if( stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
                            if (stanza_ptr->width == LS_DWORD) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers: "
                                        "Calling RAND wr_cmp_dword.. \n");
                                rc = rand_operation(RIM_DWORD,\
                                        (seg_size)/LS_DWORD,\
                                        shr_mem_ptr[j]+k*seg_size,\
                                        stanza_ptr->pattern[pi],\
                                        trap_flag, \
                                        &sd,\
                                        stanza_ptr,\
                                        (unsigned long)&seed);
                            }
                            else if(stanza_ptr->width == LS_WORD) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                        " Calling RAND wr_cmp_word.. \n");
                                rc = rand_operation(RIM_WORD,\
                                        (seg_size)/LS_WORD,\
                                        shr_mem_ptr[j]+k*seg_size,\
                                        stanza_ptr->pattern[pi],\
                                        trap_flag,\
                                        &sd,\
                                        stanza_ptr,\
                                        (unsigned long)&seed);
                            }
                            else if(stanza_ptr->width == LS_BYTE) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                        " Calling RAND wr_cmp_byte.. \n");
                                rc =rand_operation(RIM_BYTE,\
                                        (seg_size),\
                                        shr_mem_ptr[j]+k*seg_size,\
                                        stanza_ptr->pattern[pi], \
                                        trap_flag, \
                                        &sd, \
                                        stanza_ptr,\
                                        (unsigned long)&seed);
                            }
                         } else if( stanza_ptr->pattern_type[pi] == PATTERN_SIZE_BIG ) {
                            if (stanza_ptr->width == LS_DWORD) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                        " Calling BIG wr_cmp_dword.. \n");
                                rc = pat_operation(RIM_DWORD,\
                                        (seg_size)/LS_DWORD,\
                                        shr_mem_ptr[j]+k*seg_size,\
                                        stanza_ptr->pattern[pi],\
                                        trap_flag,\
                                        &sd, \
                                        stanza_ptr,\
                                        (unsigned long)&stanza_ptr->pattern_size[pi]);
                            }
                            else if(stanza_ptr->width == LS_WORD) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                        " Calling BIG wr_cmp_word.. \n");
                                rc = pat_operation(RIM_WORD,\
                                        (seg_size)/LS_WORD, \
                                        shr_mem_ptr[j]+k*seg_size,\
                                        stanza_ptr->pattern[pi],\
                                        trap_flag,\
                                        &sd, \
                                        stanza_ptr,\
                                        (unsigned long)&stanza_ptr->pattern_size[pi]);
                            }
                            else if(stanza_ptr->width == LS_BYTE) {
                                display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                        " Calling BIG wr_cmp_byte.. \n");
                                rc =pat_operation(RIM_BYTE,\
                                        (seg_size),\
                                        shr_mem_ptr[j]+k*seg_size,\
                                        stanza_ptr->pattern[pi],\
                                        trap_flag,\
                                        &sd,\
                                        stanza_ptr,\
                                        (unsigned long)&stanza_ptr->pattern_size[pi]);
                            }
                         }

                        if (priv.exit_flag == 1) {
							release_thread_memory(ti);
                            pthread_exit((void *)0);
                        } /* endif */

                        if (rc != 0) {
                            /* Report the Miscompare error details and save the
                             *  required buffers into files */
							displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,
									 "store_readim_buffers:"
									 " Calling save_buffers.. \n");
                            save_buffers(ti,rc,sd,main_misc_count,&seed,i,\
                                    trap_flag,pi);
                            STATS_VAR_INC(bad_others, 1)
                            /* stats.bad_others += 1; */
                            main_misc_count++;
                        } else {
                            STATS_VAR_INC(bytes_read,seg_size);
                            /* stats.bytes_read += (seg_size); */
                            STATS_VAR_INC(bytes_writ,seg_size);
                            /* stats.bytes_writ += (seg_size); */
                            STATS_HTX_UPDATE(UPDATE)
                            /* hxfupdate(UPDATE,&stats); */
                        } /* endif */

                        display(HTX_HE_INFO,DBG_DEBUG_PRINT,"store_readim_buffers:"
                                " Incrementing k.. \n");
                        k++; /* Incrementing to operate on next sub segment */
                    } /* while loop ends */
                } /* End of for loop on num_writes */

                 switch(stanza_ptr->switch_pat) {
                 case SW_PAT_ALL:             /* SWITCH_PAT_PER_SEG = ALL */
                     /* Stay on this segment until all patterns are tested.
                        Advance segment index once for every num_patterns */
                     pi++;
                     if (pi >= stanza_ptr->num_patterns) {
                         pi = 0;  /* Go back to the 1st pattern */
                         j++;     /* Move to the new Seg */
                     }
                     break;
                 case SW_PAT_ON:              /* SWITCH_PAT_PER_SEG = YES */
                     /* Go back to the 1st pattern */
                     pi++;
                     if (pi >= stanza_ptr->num_patterns) {
                         pi = 0;
                     }
                     /* Fall through */
                 case SW_PAT_OFF:             /* SWITCH_PAT_PER_SEG = NO */
                     /* Fall through */
                 default:
                     j++;     /* Increment Seg idx: case 1,0 and default */
                 } /* end of switch */
             } /* endfor j */

             STATS_HTX_UPDATE(UPDATE)
              /* hxfupdate(UPDATE,&stats); */
        } /* end for ps = 0 */
    }/* end for i */
    return(0);
} /* end store and read immediate */

/***********************************************************************
* thread load/store to buffers                                                *
************************************************************************/
int load_store_buffers( int ti)
{
    int  i,j,ps,pi=0; /* pi is pattern index */
    char **shr_mem_ptr;
    int *pshmid;
    unsigned long *shm_size_ptr,rc=0;
    int addr_flag = 0;
    struct segment_detail sd;
    unsigned long seed = stanza_ptr->seed;
    unsigned long seg_seed = seed;
    int nw=0,nr=0,nc=0;
    int trap_flag = 0;

    unsigned int * seed1,* seed2;
    unsigned int j1=12, j2=13;
    int bind_cpu_rand;
    seed1 = &j1;
    seed2 = &j2;

    display(HTX_HE_INFO,DBG_IMP_PRINT,"Starting load_store_buffers,"
            " num_oper = %d\n",stanza_ptr->num_oper);
    if( strcmp(stanza_ptr->pattern_id,"ADDRESS") ==0 ) {
        addr_flag = 1;
        stanza_ptr->width = LS_DWORD;
    }

    for (i = stanza_ptr->num_oper; i > 0; i--) {
		for (ps = 0; ps < MAX_NUM_PAGE_SIZES; ps++) {
        	if (mem_info.pdata[ps].supported != 1) {
        	    continue;
        	}
        	shr_mem_ptr =mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr;
        	pshmid=mem_info.tdata_hp[ti].page_wise_t[ps].shmids;
        	shm_size_ptr=mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes;

            for (j=0;j< mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments;){
#ifndef __HTX_LINUX__
				if (mem_DR_done) {
					displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d is exiting for current stanza on mem DR operation \n",ti);
					release_thread_memory(ti);
					pthread_exit((void *)1);
				}
#endif
                sd.page_index=ps;
                sd.seg_num=j;
                sd.seg_size=mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes[j];
                sd.sub_seg_num=0;
                sd.sub_seg_size=0;
                sd.width=stanza_ptr->width;
                seg_seed = seed;

		if((stanza_ptr->num_writes)&& (stanza_ptr->bind_method == 1)&& (stanza_ptr->bind_proc == 0)){
                     bind_cpu_rand = rand_r(seed1);
                     bind_cpu_rand = bind_cpu_rand%get_num_of_proc();
		     rc = do_the_bind_proc(BIND_TO_THREAD, bind_cpu_rand,-1);
                     displaym(HTX_HE_INFO,DBG_IMP_PRINT,"bind_method:MEM: write  bind rand cpu = %d \n,\
		     errno = %d ,bindprocrc = %d\n",bind_cpu_rand ,errno,rc);
               }

                for(nw=0; nw < stanza_ptr->num_writes; nw++){
                    seed = seg_seed;
                    display(HTX_HE_INFO,DBG_INFO_PRINT,"1)load_store_buffers: seg_seed = 0x%lx\n",seed);

                    if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                        display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: Calling mem_dword_addr.. \n");
                        rc = mem_operation(ADDR_WRITE,\
                                (sd.seg_size)/LS_DWORD,\
                                shr_mem_ptr[j],\
                                stanza_ptr->pattern[pi],\
                                trap_flag,\
                                &sd,\
                                stanza_ptr,\
                                (unsigned long)&stanza_ptr->pattern_size[pi]);
                    } else if(stanza_ptr->pattern_type[pi]==PATTERN_SIZE_NORMAL){
                        if (stanza_ptr->width == LS_DWORD) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers:Calling mem_dword.. \n");
                            rc = mem_operation(MEM_DWORD,\
                                    (sd.seg_size)/LS_DWORD,\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    trap_flag,\
                                    &sd,\
                                    stanza_ptr,\
                                    (unsigned long)&stanza_ptr->pattern_size[pi]);
                        }
                        else if(stanza_ptr->width == LS_WORD) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: Calling mem_word.. \n");
                            rc = mem_operation(MEM_WORD,\
                                    (sd.seg_size)/LS_WORD,\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    trap_flag,\
                                    &sd,\
                                    stanza_ptr,\
                                   (unsigned long)&stanza_ptr->pattern_size[pi]);
                        }
                        else if(stanza_ptr->width == LS_BYTE) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: Calling mem_byte.. \n");
                            rc = mem_operation(MEM_BYTE,\
                                    (sd.seg_size),\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    trap_flag,\
                                    &sd, \
                                    stanza_ptr,\
                                    (unsigned long)&stanza_ptr->pattern_size[pi]);
                        }
                    } else if( stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
                        if (stanza_ptr->width == LS_DWORD) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: Calling RAND mem_dword.. \n");
                            rc = rand_operation(MEM_DWORD,\
                                    (sd.seg_size)/LS_DWORD,\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    trap_flag,\
                                    &sd,\
                                    stanza_ptr,\
                                    (unsigned long)&seed);
                        }
                        else if(stanza_ptr->width == LS_WORD) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: Calling RAND mem_word.. \n");
                            rc = rand_operation(MEM_WORD,\
                                    (sd.seg_size)/LS_WORD,\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    trap_flag,\
                                    &sd,\
                                    stanza_ptr,\
                                    (unsigned long)&seed);
                        }
                        else if(stanza_ptr->width == LS_BYTE) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: Calling RAND mem_byte.. \n");
                            rc =rand_operation(MEM_BYTE,\
                                    (sd.seg_size), \
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi], \
                                    trap_flag, \
                                    &sd, \
                                    stanza_ptr,\
                                    (unsigned long)&seed);
                        }
                    } else if( stanza_ptr->pattern_type[pi] == PATTERN_SIZE_BIG ) {
                        if (stanza_ptr->width == LS_DWORD) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: Calling BIG mem_dword.. \n");
                            rc = pat_operation(MEM_DWORD,\
                                    (sd.seg_size)/LS_DWORD,\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    trap_flag,\
                                    &sd, \
                                    stanza_ptr,\
                                    (unsigned long)&stanza_ptr->pattern_size[pi]);
                        }
                        else if(stanza_ptr->width == LS_WORD) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: Calling BIG mem_word.. \n");
                            rc = pat_operation(MEM_WORD,\
                                    (sd.seg_size)/LS_WORD,\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    trap_flag, \
                                    &sd, \
                                    stanza_ptr,\
                                    (unsigned long)&stanza_ptr->pattern_size[pi]);
                        }
                        else if(stanza_ptr->width == LS_BYTE) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: Calling BIG mem_byte.. \n");
                            rc =pat_operation(MEM_BYTE,\
                                    (sd.seg_size),\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    trap_flag, \
                                    &sd, \
                                    stanza_ptr,\
                                    (unsigned long)&stanza_ptr->pattern_size[pi]);
                        }
                    }
#ifdef DEBUG_MEM64
                    for (k = 0; k<shm_size_ptr[j]/MIN_PATTERN_SIZE;
                         k=k+MIN_PATTERN_SIZE){
                        if (k < 124) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"Value at Address shr_mem_ptr[%d][%d](Addr=0x%lx)=0x%lx\n"\
                                ,j,k,(shr_mem_ptr[j]+k),\
                                *(unsigned long *)(shr_mem_ptr[j]+k) );
                        }
                    }
#endif
                    display(HTX_HE_INFO,DBG_DEBUG_PRINT,"2.0)load_store_buffers:j=%ld, seed = 0x%lx\n",j,seed);

                    if (priv.exit_flag == 1) {
						release_thread_memory(ti);
                        pthread_exit((void *)0);
                    } /* endif */
                    STATS_VAR_INC(bytes_writ, shm_size_ptr[j])
                       /*  stats.bytes_writ += shm_size_ptr[j]; */
                    STATS_HTX_UPDATE(UPDATE)
                } /* For loop on num_writes ends */

		if((stanza_ptr->num_writes)&& (stanza_ptr->bind_method == 1)&& (stanza_ptr->bind_proc == 0)){
		    rc = do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
                    display(HTX_HE_INFO,DBG_INFO_PRINT,"bind_method:MEM:write unbind \n, errno = %d,unbind  \
		    rc = %d \n",errno,rc);
               }

		if((stanza_ptr->num_reads || stanza_ptr->num_compares)&& (stanza_ptr->bind_method == 1)&& \
                     (stanza_ptr->bind_proc == 0)){
                        bind_cpu_rand = rand_r(seed2);
                        bind_cpu_rand = bind_cpu_rand%get_num_of_proc();
			rc = do_the_bind_proc(BIND_TO_THREAD, bind_cpu_rand,-1);
                        display(HTX_HE_INFO,DBG_INFO_PRINT,"bind_method:MEM:compare bind rand cpu = %d \n,\
			errno = %d ,bindprocrc = %d\n",bind_cpu_rand ,errno,rc);
                }

        /*if (stanza_ptr->num_reads >= 1 && stanza_ptr->num_reads != stanza_ptr->num_compares) {*/
        	for (nr=0; nr<stanza_ptr->num_reads;nr++) {
            	if (stanza_ptr->width == LS_DWORD) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers:"
                                    " Calling read_dword.. \n");
                            rc = mem_operation(READ_DWORD,\
                                    (sd.seg_size)/LS_DWORD, \
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi], \
                                    0,\
                                    0, \
                                    stanza_ptr,\
                                    (unsigned long)&stanza_ptr->pattern_size[pi]);
               }
               else if( stanza_ptr->width == LS_WORD) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers:"
                                    " Calling  read_word.. \n");
                            rc = mem_operation(READ_WORD,\
                                    (sd.seg_size)/LS_WORD,\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    0, \
                                    0, \
                                    stanza_ptr,\
                                    (unsigned long)&stanza_ptr->pattern_size[pi]);
               }
               else if(stanza_ptr->width == LS_BYTE) {
                            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"load_store_buffers: "
                                    "Calling read_byte.. \n");
                            rc = mem_operation(READ_BYTE,\
                                    (sd.seg_size),\
                                    shr_mem_ptr[j],\
                                    stanza_ptr->pattern[pi],\
                                    0,\
                                    0,\
                                    stanza_ptr,\
                                    (unsigned long)&stanza_ptr->pattern_size[pi]);
               }
               if (priv.exit_flag == 1) {
					release_thread_memory(ti);
               		pthread_exit((void *)0);
               } /* endif */

				/* defect 750522 */
		    	STATS_VAR_INC(bytes_read, shm_size_ptr[j])
    		    /* stats.bytes_read += shm_size_ptr[seg_num]; */
                STATS_HTX_UPDATE(UPDATE)
		    }

       /* If compare is YES in the stanza then
       *  Compare the present segment buffer which is just written
       *  with the pattern.
       *  We assume the pattern files are 8 byte repeatative
       */
       for (nc=0; nc < stanza_ptr->num_compares; nc++) {
/*     		display(HTX_HE_INFO, DBG_DEBUG_PRINT,"#load_store_buffers,"
            	"Calling seg_comp_buffers_8 (compare_count = %d)\n",nc);
*/
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"2)load_store_buffers:j=%ld, seg_seed = 0x%lx\n",j,seg_seed);
            if (seg_comp_buffers_8(ti,j,ps,i,pi,seg_seed) != 0) {
            	display(HTX_HE_INFO,DBG_DEBUG_PRINT,\
                                "#load_store_buffers,seg_comp_buffers_8 failed (compare_count = %d)\n",nc);
                return(1);
            }
            if (priv.exit_flag == 1) {
				release_thread_memory(ti);
                pthread_exit((void *)0);
            } /* endif */
            display(HTX_HE_INFO, DBG_DEBUG_PRINT,"#load_store_buffers,seg_comp_buffers_8 returned\n");
       }

       if((stanza_ptr->num_reads || stanza_ptr->num_compares)&& (stanza_ptr->bind_method == 1)&& \
       		(stanza_ptr->bind_proc == 0)){
		     rc = do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
                     display(HTX_HE_INFO,DBG_INFO_PRINT,"bind_method:MEM:compare unbind \n, errno = %d , rc = %d\n",
                     errno,rc);
       }

       display(HTX_HE_INFO,DBG_IMP_PRINT,"#1 load_store_buffers,num_oper = %d\n",stanza_ptr->num_oper);

       if (priv.exit_flag == 1) {
			release_thread_memory(ti);
       		pthread_exit((void *)0);
       } /* endif */
       STATS_HTX_UPDATE(UPDATE)
       /* hxfupdate(UPDATE,&stats);*/

       switch(stanza_ptr->switch_pat) {
      		 case SW_PAT_ALL:               /* SWITCH_PAT_PER_SEG = ALL */
       			/* Stay on this segment until all patterns are tested.
            	Advance segment index once for every num_patterns */
            	pi++;
            	if (pi >= stanza_ptr->num_patterns) {
            		pi = 0; /* Go back to the 1st pattern */
               		j++;        /* Move to the new Seg */
            	}
           		 break;
            case SW_PAT_ON:                /* SWITCH_PAT_PER_SEG = YES */
            	/* Go back to the 1st pattern */
               	pi++;
                if (pi >= stanza_ptr->num_patterns) {
                	pi = 0;
                }
                /* Fall through */
            case SW_PAT_OFF:                /* SWITCH_PAT_PER_SEG = NO */
            	/* Fall through */
                default:
               	j++;        /* Increment Seg idx: case 1,0 and default */
       } /* end of switch */
            } /* end for j */
    } /* end for ps */
  }/* end for i */
    return(0);
} /* end load_store_buffers */

/*******************************************************************************
 *  fucntions to store a double word for nstride test case
 *******************************************************************************/
int write_dword(void *addr, int no_of_strides, int oper_per_stride, int pi, unsigned int *seed)
{
	unsigned long rand_no, *ptr = (unsigned long *) addr;
	int k, l=0, m, n=0, i;

	for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
		for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < stanza_ptr->pattern_size[pi]; i+=stanza_ptr->width) { /* Loop for pattern_size */
				if (stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS) {
					 ptr[l + i] = (unsigned long)(ptr + l + i);
				} else if (stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
					rand_no = (unsigned long)rand_r(seed);
					ptr[l + i] = rand_no;
					*seed = (unsigned int) rand_no;
				} else {
					ptr[l + i] = *((unsigned long *)(stanza_ptr->pattern[pi] + i)); /* access pattern */
				}
			} /* End loop i */
			l = l + stanza_ptr->stride_sz/stanza_ptr->width;
		} /* End loop m */
		n = n + stanza_ptr->pattern_size[pi]/stanza_ptr->width;
		l = n;
	}  /* End loop k */
	return 0;
} /* end write_dword */

/*******************************************************************************
 *  fucntions to read a double word for nstride test case
 *******************************************************************************/
int read_dword(void *addr, int no_of_strides, int oper_per_stride)
{
	unsigned long buf, *ptr = (unsigned long *) addr;
	int k, l=0, m, n=0;

	for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
		for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			buf = ptr[l];
			l = l + stanza_ptr->stride_sz/stanza_ptr->width;
		} /* End loop m */
		n = n + 1;
		l = n;
	}  /* End loop k */
	return 0;
} /* end read_dword */

/*******************************************************************************
 *  fucntions to read/Comp a double word for nstride test case
 *******************************************************************************/
int read_comp_dword(void *addr, int no_of_strides, int oper_per_stride, int pi, unsigned int *seed)
{
	unsigned long buf, expected_val, *ptr = (unsigned long *) addr;
	int k, l=0, m, n=0, rc = 0, i;

	for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
		for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < stanza_ptr->pattern_size[pi]; i+=stanza_ptr->width) { /* Loop for pattern_size */
				buf = ptr[l + i];
				if (stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS) {
					expected_val = (unsigned long)(ptr + l + i);
				} else if (stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
					expected_val = (unsigned long)rand_r(seed);
					*seed = (unsigned int) expected_val;
				} else {
					expected_val = *((unsigned long *)(stanza_ptr->pattern[pi] + i)); /* access pattern */
				}
				if (buf != expected_val) {
					/* printf("miscompare happened. expected_val: %lx, mem ptr[%d]: %lx\n", expected_val, (l + i), ptr[l + i]); */
					rc = l + i + 1;
					return rc;
				}
			} /* End loop i */
			l = l + stanza_ptr->stride_sz/stanza_ptr->width;
		} /* End loop m */
		n = n + stanza_ptr->pattern_size[pi]/stanza_ptr->width;
		l = n;
	}  /* End loop k */
	return rc;
} /* end read_comp_dword */

/*******************************************************************************
 *  fucntions to store a word for nstride test case
 *******************************************************************************/
int write_word(void *addr, int no_of_strides, int oper_per_stride, int pi, unsigned int *seed)
{
	unsigned int rand_no, *ptr = (unsigned int *) addr;
	int k, l=0, m, n=0, i;

	for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
		for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < stanza_ptr->pattern_size[pi]; i+=stanza_ptr->width) { /* Loop for pattern_size */
				if (stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
					rand_no = (unsigned int)rand_r(seed);
					ptr[l + i] = rand_no;
					*seed = (unsigned int) rand_no;
				} else {
					ptr[l + i] = *((unsigned int *)(stanza_ptr->pattern[pi] + i)); /* access pattern */
				}
			} /* End loop i */
			l = l + stanza_ptr->stride_sz/stanza_ptr->width;
		} /* End loop m */
		n = n + stanza_ptr->pattern_size[pi]/stanza_ptr->width;
		l = n;
	}  /* End loop k */
	return 0;
} /* end write_word */

/*******************************************************************************
 *  fucntions to read a word for nstride test case
 *******************************************************************************/
int read_word(void *addr, int no_of_strides, int oper_per_stride)
{
	unsigned int buf, *ptr = (unsigned int *) addr;
	int k, l=0, m, n=0;

	for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
		for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			buf = ptr[l];
			l = l + stanza_ptr->stride_sz/stanza_ptr->width;
		} /* End loop m */
		n = n + 1;
		l = n;
	}  /* End loop k */
	return 0;
} /* end read_word */

/*******************************************************************************
 *  fucntions to read/Comp a word for nstride test case
 *******************************************************************************/
int read_comp_word(void *addr, int no_of_strides, int oper_per_stride, int pi, unsigned int *seed)
{
	unsigned int buf, expected_val, *ptr = (unsigned int *) addr;
	int k, l=0, m, n=0, rc = 0, i;

	for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
		for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < stanza_ptr->pattern_size[pi]; i+=stanza_ptr->width) { /* Loop for pattern_size */
				buf = ptr[l + i];
				if (stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
					expected_val = (unsigned int)rand_r(seed);
					*seed = (unsigned int) expected_val;
				} else {
					expected_val = *((unsigned int *)(stanza_ptr->pattern[pi] + i)); /* access pattern */
				}
				if (buf != expected_val) {
					/* printf("miscompare happened. expected_val: %lx, mem ptr[%d]: %lx\n", expected_val, (l + i), ptr[l + i]); */
					rc = l + i + 1;
					return rc;
				}
			} /* End loop i */
			l = l + stanza_ptr->stride_sz/stanza_ptr->width;
		} /* End loop m */
		n = n + stanza_ptr->pattern_size[pi]/stanza_ptr->width;
		l = n;
	}  /* End loop k */
	return rc;
} /* end read_comp_word */

/*******************************************************************************
 *  fucntions to store a byte for nstride test case
 *******************************************************************************/
int write_byte(void *addr, int no_of_strides, int oper_per_stride, int pi, unsigned int *seed)
{
	char rand_no, *ptr = (char *) addr;
	int k, l=0, m, n=0, i;

	for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
		for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < stanza_ptr->pattern_size[pi]; i+=stanza_ptr->width) { /* Loop for pattern_size */
				if (stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
					rand_no = (char)rand_r(seed);
					ptr[l + i] = rand_no;
					*seed = (unsigned int) rand_no;
				} else {
					ptr[l + i] = *((char *)(stanza_ptr->pattern[pi] + i)); /* access pattern */
				}
			} /* End loop i */
			l = l + stanza_ptr->stride_sz/stanza_ptr->width;
		} /* End loop m */
		n = n + stanza_ptr->pattern_size[pi]/stanza_ptr->width;
		l = n;
	}  /* End loop k */
	return 0;
} /* end write_byte */

/*******************************************************************************
 *  fucntions to read a byte for nstride test case
 *******************************************************************************/
int read_byte(void *addr, int no_of_strides, int oper_per_stride)
{
	char buf, *ptr = (char *) addr;
	int k, l=0, m, n=0;

	for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
		for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			buf = ptr[l];
			l = l + stanza_ptr->stride_sz/stanza_ptr->width;
		} /* End loop m */
		n = n + 1;
		l = n;
	}  /* End loop k */
	return 0;
} /* end read_byte */

/*******************************************************************************
 *  fucntions to read/Comp a byte for nstride test case
 *******************************************************************************/
int read_comp_byte(void *addr, int no_of_strides, int oper_per_stride, int pi, unsigned int *seed)
{
	char buf, expected_val, *ptr = (char *) addr;
	int k, l=0, m, n=0, rc = 0, i;

	for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
		for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < stanza_ptr->pattern_size[pi]; i+=stanza_ptr->width) { /* Loop for pattern_size */
				buf = ptr[l + i];
				if (stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
					expected_val = (char)rand_r(seed);
					*seed = (unsigned int) expected_val;
				} else {
					expected_val = *((char *)(stanza_ptr->pattern[pi] + i)); /* access pattern */
				}
				if (buf != expected_val) {
					/* printf("miscompare happened. expected_val: %lx, mem ptr[%d]: %lx\n", expected_val, (l + i), ptr[l + i]); */
					rc = l + i + 1;
					return rc;
				}
			} /* End loop i */
			l = l + stanza_ptr->stride_sz/stanza_ptr->width;
		} /* End loop m */
		n = n + stanza_ptr->pattern_size[pi]/stanza_ptr->width;
		l = n;
	}  /* End loop k */
	return rc;
} /* end read_comp_byte */

/****************************************************************************
 *				Function to run the nstride testcase 						*
 ****************************************************************************/

int n_stride(int ti)
{
	int i,j,ps,pi=0;
 	char **shr_mem_ptr;
 	unsigned long *shm_size_ptr, rc=0;
 	struct segment_detail sd;
	void *shr_mem_cur_ptr;
	int bind_cpu_rand, nw=0,nr=0,nc=0;
	int no_of_strides, oper_per_stride, count;
	static int main_misc_count = 0;
	int miscompare_count, trap_flag;
	unsigned int seg_seed, seed;
	unsigned int *seed1, *seed2;
	unsigned int j1=12, j2=13;

	seed1 = &j1;
	seed2 = &j2;

	if (stanza_ptr->seed == 0 ) {
		seed = (unsigned int)time(NULL);
	} else {
		seed = (unsigned int)stanza_ptr->seed;
	}

	display(HTX_HE_INFO,DBG_IMP_PRINT,"Starting n_stride,"
	" num_oper = %d\n",stanza_ptr->num_oper);

	if( strcmp(stanza_ptr->pattern_id,"ADDRESS") ==0 ) {
		stanza_ptr->width = LS_DWORD;
	}
	if( (stanza_ptr->misc_crash_flag) && (priv.htxkdblevel))
		trap_flag = 1;    /* Trap is set */
	else
		trap_flag = 0;    /* Trap is not set */

	if(stanza_ptr->attn_flag)
		trap_flag = 2;    /* attn is set */

	for (i = stanza_ptr->num_oper; i > 0; i--) {
		for (ps = 0; ps < MAX_NUM_PAGE_SIZES; ps++) {
			if (mem_info.pdata[ps].supported != 1) {
					continue;
			}
			shr_mem_ptr =mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr;
			shm_size_ptr=mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes;

			for (j=0;j< mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments;) {
#ifndef __HTX_LINUX__
				if (mem_DR_done) {
					displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d is exiting for current stanza on mem DR operation \n",ti);
					release_thread_memory(ti);
					pthread_exit((void *)1);
				}
#endif
				shr_mem_cur_ptr = (void *) shr_mem_ptr[j];
				sd.page_index=ps;
				sd.seg_num=j;
				sd.seg_size=mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes[j];
				sd.sub_seg_num=0;
				sd.sub_seg_size=0;
				sd.width=stanza_ptr->width;
				no_of_strides = sd.seg_size / stanza_ptr->stride_sz;
				oper_per_stride = stanza_ptr->stride_sz / stanza_ptr->width;
				seg_seed = seed;
				 /* printf("thread: %d, no_of_oper: %d, page_size: %d, segment num: %d, seg_sz: 0x%x, shr_mem_hptr: %lx, shr_mem_ptr[%d]: %lx, shr_mem_cur_ptr: %lx, pattern:%llx\n",
				 		ti, stanza_ptr->num_oper, ps, j, sd.seg_size, mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr, j, shr_mem_ptr[j], shr_mem_cur_ptr, *(unsigned long *)stanza_ptr->pattern[pi]); */

				if((stanza_ptr->num_writes)&& (stanza_ptr->bind_method == 1)&& (stanza_ptr->bind_proc == 0)){
					bind_cpu_rand = rand_r(seed1);
					bind_cpu_rand = bind_cpu_rand%get_num_of_proc();
					rc = do_the_bind_proc(BIND_TO_THREAD, bind_cpu_rand,-1);
					display(HTX_HE_INFO,DBG_IMP_PRINT,"bind_method:MEM: write  bind rand cpu = %d \n,\
							errno = %d ,bindprocrc = %d\n",bind_cpu_rand ,errno,rc);
				}

				for(nw=0; nw < stanza_ptr->num_writes; nw++){
					seed = seg_seed;
					switch (stanza_ptr->width) {
						case LS_DWORD:
							rc = write_dword(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi, &seed);
							break;
						case LS_WORD:
							rc = write_word(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi, &seed);
							break;
						case LS_BYTE:
							rc = write_byte(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi, &seed);
							break;
					}
					if (priv.exit_flag == 1) {
						release_thread_memory(ti);
						pthread_exit((void *)0);
					}
					STATS_VAR_INC(bytes_writ, shm_size_ptr[j])
					STATS_HTX_UPDATE(UPDATE)
				}
				if((stanza_ptr->num_writes)&& (stanza_ptr->bind_method == 1)&& (stanza_ptr->bind_proc == 0)){
					rc = do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
					display(HTX_HE_INFO,DBG_INFO_PRINT,"bind_method:MEM:write unbind \n, errno = %d,unbind  \
							rc = %d \n",errno,rc);
				}

				if((stanza_ptr->num_reads || stanza_ptr->num_compares)&& (stanza_ptr->bind_method == 1)&& \
					(stanza_ptr->bind_proc == 0)){
					bind_cpu_rand = rand_r(seed2);
					bind_cpu_rand = bind_cpu_rand%get_num_of_proc();
					rc = do_the_bind_proc(BIND_TO_THREAD, bind_cpu_rand,-1);
					display(HTX_HE_INFO,DBG_INFO_PRINT,"bind_method:MEM:compare bind rand cpu = %d \n,\
						errno = %d ,bindprocrc = %d\n",bind_cpu_rand ,errno,rc);
				}
				for (nr=0; nr < stanza_ptr->num_reads; nr++) {
					switch (stanza_ptr->width) {
						case LS_DWORD:
							rc = read_dword(shr_mem_cur_ptr, no_of_strides, oper_per_stride);
							break;
						case LS_WORD:
							rc = read_word(shr_mem_cur_ptr, no_of_strides, oper_per_stride);
							break;
						case LS_BYTE:
							rc = read_byte(shr_mem_cur_ptr, no_of_strides, oper_per_stride);
							break;
					}
					if (priv.exit_flag == 1) {
						release_thread_memory(ti);
						pthread_exit((void *)0);
					}
					STATS_VAR_INC(bytes_read, shm_size_ptr[j])
					STATS_HTX_UPDATE(UPDATE)
				}
				for (nc=0; nc < stanza_ptr->num_compares; nc++) {
					seed = seg_seed;
					switch (stanza_ptr->width) {
						case LS_DWORD:
							rc = read_comp_dword(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi, &seed);
							break;
						case LS_WORD:
							rc = read_comp_word(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi, &seed);
							break;
						case LS_BYTE:
							rc = read_comp_byte(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi, &seed);
							break;
					}
					if (priv.exit_flag == 1) {
						release_thread_memory(ti);
						pthread_exit((void *)0);
					}
					if (rc != 0) { /* if there is miscompare, rc will have offset */
						displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,
								"NSTRIDE testcase showed miscompare\n");
						if (trap_flag == 1) {
							#ifndef __HTX_LINUX__
								trap(0xBEEFDEAD, rc, shr_mem_cur_ptr, stanza_ptr->pattern[pi], ((char *)shr_mem_cur_ptr)+ ((rc -1) * stanza_ptr->width), &sd, stanza_ptr );
							#endif
						}
						miscompare_count = save_buffers(ti,rc,sd,main_misc_count,(unsigned long *)&seed,i,trap_flag,pi);
						STATS_VAR_INC(bad_others, 1)
						main_misc_count++;
					}
					else {
						STATS_VAR_INC(bytes_read, shm_size_ptr[j])
						STATS_HTX_UPDATE(UPDATE)
					}
				}
				if((stanza_ptr->num_reads || stanza_ptr->num_compares)&& (stanza_ptr->bind_method == 1)&& \
					(stanza_ptr->bind_proc == 0)){
					rc = do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
					display(HTX_HE_INFO,DBG_INFO_PRINT,"bind_method:MEM:compare unbind \n, errno = %d , rc = %d\n",
							errno,rc);
				}

				switch(stanza_ptr->switch_pat) {
					case SW_PAT_ALL:               /* SWITCH_PAT_PER_SEG = ALL */
						/* Stay on this segment until all patterns are tested.
						   Advance segment index once for every num_patterns */
						pi++;
						if (pi >= stanza_ptr->num_patterns) {
							pi = 0; /* Go back to the 1st pattern */
							j++;        /* Move to the new Seg */
						}
						break;
					case SW_PAT_ON:                /* SWITCH_PAT_PER_SEG = YES */
						/* Go back to next pattern for next segment */
						pi++;
						if (pi >= stanza_ptr->num_patterns) {
							pi = 0;   /* Go back to the 1st pattern */
						}
						/* Fall through */
					case SW_PAT_OFF:                /* SWITCH_PAT_PER_SEG = NO */
						/* Fall through */
					default:
						j++;        /* Increment Seg idx: case 1,0 and default */
				} /* end of switch */
			} /* end loop for j */
		} /* end loop for ps */
	} /* end loop for i */
	return 0;
}

 /***************************************************************************************
  * int seg_comp_buffers_8(int ti,int seg_num, int ps_index, int pass,int pattern_index,unsigned long seg_seed)
  *  Compares buffers for the present segment which is just written .
  *  Params:   ti (thread index )
  *            seg_num (present segment number whose buffers we need to compare)
  *                  ps (page size index)
  *                  pass count
  *    Returns: 0 on success
  *            1 on error
  *            If miscompare give a HARD error and creates some files in temp.
  *   See the code for more .
  *****************************************************************************************/
int seg_comp_buffers_8(int ti, int seg_num, int ps, int pass,int pi,unsigned long seed)
{
    int  miscompare_count;
    static int main_misc_count=0;
    int trap_flag;
    unsigned long rc=0;
    char **shr_mem_ptr ;
    int *pshmid;
    unsigned long *shm_size_ptr;
    struct segment_detail sd;

    display(HTX_HE_INFO,DBG_IMP_PRINT,"#1 seg_comp_buffers_8,pattern_name=%s,"
            "pi=%d, bit_pattern=0x%llx,seg_num= %d and page index =%d\n",\
            stanza_ptr->pattern_name[pi],pi,stanza_ptr->pattern[pi],seg_num,ps);

    shr_mem_ptr =mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr;
    pshmid=mem_info.tdata_hp[ti].page_wise_t[ps].shmids;
    shm_size_ptr=mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes;

    /* Fill up the segment details structure */
    sd.seg_num = seg_num;
    sd.page_index = ps;
    if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
        sd.width = LS_DWORD;
    } else {
        sd.width = stanza_ptr->width;
    }
    sd.seg_size = shm_size_ptr[seg_num];
    sd.sub_seg_num=0;
    sd.sub_seg_size=0;

    if( (stanza_ptr->misc_crash_flag) && (priv.htxkdblevel) )
        trap_flag = 1;    /* Trap is set */
    else
        trap_flag = 0;    /* Trap is not set */

    if(stanza_ptr->attn_flag)
        trap_flag = 2;    /* attn is set */

    display(HTX_HE_INFO,DBG_IMP_PRINT,"#seg_comp_buffers_8: trap_flag = %d,"
            "crash_on_mis =%s,htxkdblevel=%d,&pattern[pi][0]=0x%p and "
            "pattern[pi]=0x%p\n",trap_flag,stanza_ptr->crash_on_mis, \
            priv.htxkdblevel,&(stanza_ptr->pattern[pi][0]),stanza_ptr->pattern[pi]);

    if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
        display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8:"
                " Calling wr_addr_cmp_dword.. \n");
        rc = mem_operation(ADDR_COMP,\
                (sd.seg_size)/LS_DWORD,\
                shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], \
                trap_flag, \
                &sd, \
                stanza_ptr,\
                (unsigned long)&stanza_ptr->pattern_size[pi]);
    } else if( stanza_ptr->pattern_type[pi] == PATTERN_SIZE_NORMAL ) {
        if (stanza_ptr->width == LS_DWORD) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8:"
                    "Calling wr_cmp_dword.. \n");
            rc = mem_operation(COMP_DWORD,(sd.seg_size)/LS_DWORD, shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,\
                (unsigned long)&stanza_ptr->pattern_size[pi]);
        }
        else if(stanza_ptr->width == LS_WORD) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8: "
                    "Calling wr_cmp_word.. \n");
            rc = mem_operation(COMP_WORD,(sd.seg_size)/LS_WORD, shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,\
                (unsigned long)&stanza_ptr->pattern_size[pi]);
        }
        else if(stanza_ptr->width == LS_BYTE) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8: "
                    "Calling wr_cmp_byte.. \n");
            rc = mem_operation(COMP_BYTE,(sd.seg_size), shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,\
                (unsigned long)&stanza_ptr->pattern_size[pi]);
        }
    } else if( stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
        if (stanza_ptr->width == LS_DWORD) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8: "
                    "Calling RAND cmp_dword.. \n");
            rc = rand_operation(COMP_DWORD,(sd.seg_size)/LS_DWORD, shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,\
                (unsigned long)&seed);
        }
        else if(stanza_ptr->width == LS_WORD) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8: "
                    "Calling RAND cmp_word.. \n");
            rc = rand_operation(COMP_WORD,(sd.seg_size)/LS_WORD, shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,\
                (unsigned long)&seed);
        }
        else if(stanza_ptr->width == LS_BYTE) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8: "
                    "Calling RAND cmp_byte.. \n");
            rc =rand_operation(COMP_BYTE,(sd.seg_size), shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,\
                (unsigned long)&seed);
        }
    } else if( stanza_ptr->pattern_type[pi] == PATTERN_SIZE_BIG ) {
        if (stanza_ptr->width == LS_DWORD) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8: "
                    "Calling BIG cmp_dword.. \n");
            rc = pat_operation(COMP_DWORD,(sd.seg_size)/LS_DWORD, shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,\
                (unsigned long)&stanza_ptr->pattern_size[pi]);
        }
        else if(stanza_ptr->width == LS_WORD) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8:"
                    " Calling BIG cmp_word.. \n");
            rc = pat_operation(COMP_WORD,(sd.seg_size)/LS_WORD, shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,\
                (unsigned long)&stanza_ptr->pattern_size[pi]);
        }
        else if(stanza_ptr->width == LS_BYTE) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_comp_buffers_8:"
                    " Calling BIG cmp_byte.. \n");
            rc =pat_operation(COMP_BYTE,(sd.seg_size), shr_mem_ptr[seg_num],\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,\
                (unsigned long)&stanza_ptr->pattern_size[pi]);
        }
    }

    if (priv.exit_flag == 1) {
		release_thread_memory(ti);
        pthread_exit((void *)0);
    } /* endif */

    display(HTX_HE_INFO,DBG_IMP_PRINT,"#seg_comp_buffers_8: Returned from compare"
                                         " routine with %d return value \n",rc);

    if (rc != 0) {    /* If there is any miscompare, rc will have the offset */
		displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,
				 "#seg_comp_buffers_8: looks like return"
				 " from comp_dword* is not zero\n");
        miscompare_count = save_buffers(ti,rc,sd,main_misc_count,&seed,pass,trap_flag,pi);
        STATS_VAR_INC(bad_others, 1)
        /* stats.bad_others += 1; */
        main_misc_count++;
    } else {
        STATS_VAR_INC(bytes_read, shm_size_ptr[seg_num])
        /* stats.bytes_read += shm_size_ptr[seg_num]; */
        STATS_HTX_UPDATE(UPDATE)
        /* hxfupdate(UPDATE,&stats); */
    } /* endif */


    STATS_HTX_UPDATE(UPDATE)
    /* hxfupdate(UPDATE,&stats); */
    display(HTX_HE_INFO,DBG_INFO_PRINT,"#5 seg_comp_buffers_8\n");
    return(0);
}


/*
 * Function Name: release_thread_memory()
 *
 * Description: It detaches and removes all shared memory segments of calling thread 
 *	and initilaze other parameters to -1.
 *
 */
void release_thread_memory(int ti)
{
	int ps,i,rc,rc1=0,rc2=0,rc3=0;
    char **shr_mem_ptr;
    int *pshmid;
    unsigned long *shm_size_ptr;
	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"release_thread_memory() called by thread:%d\n",ti);

	for (ps = 0; ps < MAX_NUM_PAGE_SIZES; ps++) {
		display(HTX_HE_INFO, DBG_INFO_PRINT,"Thread(%d),page = %s\n",ti,page_size[ps]);
		shm_size_ptr=mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes;
		pshmid = mem_info.tdata_hp[ti].page_wise_t[ps].shmids;
		shr_mem_ptr = mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr;
#ifdef MMAP
		rc1 = mprotect (shr_mem_ptr,(sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments),PROT_WRITE | PROT_READ | PROT_EXEC);
		rc2 = mprotect (pshmid,(sizeof(int)*mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments),PROT_WRITE | PROT_READ | PROT_EXEC);
		rc3 = mprotect (shm_size_ptr,(sizeof(unsigned long)* mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments),PROT_WRITE | PROT_READ | PROT_EXEC);
		if( (rc1 | rc2 | rc3) != 0 )
			{
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"(%d)Error (%d) mprotect() failed with rc1 = %d rc2=%d rc3=%d,ti=%d and num_segs=%d shm_size_ptr=%p pshmid=%p shr_mem_ptr=%p\n",
					__LINE__,errno, rc1,rc2,rc3,ti,mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments,shm_size_ptr,pshmid,shr_mem_ptr);
			}
#endif

		if (shr_mem_ptr) {/* If shared memory head pointer is allocated */
			for (i=0; i<mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments; i++) {
				if ( shr_mem_ptr[i] !=NULL) {/* Checkin if each segment is allocated */
					display(HTX_HE_INFO,DBG_INFO_PRINT,"Removing segment #%d of page size %s "
							"shr_mem id=0x%x, shr_memp = 0x%lx\n",i,page_size[ps],pshmid[i],\
							(unsigned long int )shr_mem_ptr[i]);
							errno = 0;
					rc = shmdt(shr_mem_ptr[i]);
					if (rc != 0) {
						displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"release_thread_memory for thread(%d): errno (%d) "
							"detaching shared memory segment %d rc=%d\n"
							" id = %d, shr_memp = 0x%lx, page size =%s\n",\
						ti,errno, i,rc,pshmid[i],(unsigned long int )shr_mem_ptr[i],\
						page_size[ps]);
					}
					errno = 0;
					display(HTX_HE_INFO,DBG_DEBUG_PRINT,"shmdt successful \n");
					rc = shmctl(pshmid[i], IPC_RMID, (struct shmid_ds *) NULL);
					display(HTX_HE_INFO,DBG_DEBUG_PRINT,"shm RMID completed,ret = %d \n",rc);
					if (rc != 0) {
						displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"release_thread_memory(%d): errno (%d) "
							"removing shared memory segment %d rc=%d\n"
							" id = %d, shr_memp = 0x%lx, page size = %s\n",\
						ti,errno, i,rc,pshmid[i],(unsigned long int)shr_mem_ptr[i],\
						page_size[ps]);
					}

					display(HTX_HE_INFO,DBG_IMP_PRINT,"Released shared memory buffer %d\n"
						"id = %d, shr_memp = 0x%lx page size = %s\n",
						i,pshmid[i],(unsigned long int)shr_mem_ptr[i], page_size[ps]);
					shr_mem_ptr[i]= (void *) NULL;
				}/*end if ( *(shr_mem_ptr... */
			} /* end for i <= num of segments */
		}/*end of if (shr_mem_ptr)... */

		display(HTX_HE_INFO,DBG_INFO_PRINT,"Release the head pointers now \n");

		/* free the data structures used for references the shared memory */
		if(mem_info.tdata_hp [ti].page_wise_t[ps].shr_mem_hptr) {
#ifndef MMAP
				free(mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr);
#else
				rc = munmap (mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr,(sizeof(char *)*mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments));
				if ( rc != 0) {
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"munmap failed for em_info.tdata_hp[%d].page_wise_t[%d].shr_mem_hptr errno=%d with rc = %d\n",\
						ti,ps,mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr,errno,rc);
				}
#endif
		}
		mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr=NULL;
		if(mem_info.tdata_hp[ti].page_wise_t[ps].shmids) {
#ifndef MMAP
				free(mem_info.tdata_hp[ti].page_wise_t[ps].shmids);
#else
				rc =  munmap (mem_info.tdata_hp[ti].page_wise_t[ps].shmids,(sizeof(int)*mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments));
				if ( rc != 0) {
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"munmap failed for mem_info.tdata_hp[%d].page_wise_t[%d].shmids=%lx errno=%d with rc = %d\n",\
						ti,ps,mem_info.tdata_hp[ti].page_wise_t[ps].shmids,errno,rc);
				}
#endif
		}
		mem_info.tdata_hp[ti].page_wise_t[ps].shmids=NULL;

		if(mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes) {
#ifndef MMAP
				free(mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes);
#else
				rc =  munmap(mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes,(sizeof(unsigned long)*mem_info.tdata_hp[ti].page_wise_t[ps].num_of_segments));
				if ( rc != 0) {
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"munmap failed for mem_info.tdata_hp[%d].page_wise_t[%d].shm_sizes = %lx errno=%d with rc = %d\n",\
						ti,ps,mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes,errno,rc);
				}
#endif
		}
		mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes=NULL;
	}/* end for loop on pagesizes */
	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"release_thread_memory() Completed by thread:%d\n",ti);
}

/*
 * Function Name: re_initialize_global_vars()
 *
 * Description: It initializes global pointers and frees global malloc 
 *
 */
void re_initialize_global_vars()
{
    int ti, rc;

    #ifndef __HTX_LINUX__
    /* In case of affinity == TRUE is executed set the enhanced_affinity_vmpool_limit value to default  */
    if(stanza_ptr->affinity == TRUE) {
        rc = system("vmo -d enhanced_affinity_vmpool_limit");
        displaym(HTX_HE_INFO,DBG_IMP_PRINT,"re_initialize_global_vars():vmo -d enhanced_affinity_vmpool_limit rc=%d \t errno =%d\n",\
        rc,errno);
    }
    #endif
	for (ti = 0; ti <mem_info.num_of_threads; ti++) {
		if (mem_info.tdata_hp[ti].tid!= -1) {
			rc=pthread_cancel(mem_info.tdata_hp[ti].tid);
			if (rc != 0) {
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"pthread cancel failed "
					"for thread num %d, id = %d\n",mem_info.tdata_hp[ti].thread_num,\
					mem_info.tdata_hp[ti].tid);
			}
			mem_info.tdata_hp[ti].tid = -1;
			mem_info.tdata_hp[ti].thread_num= -1;
		}
	}

    display(HTX_HE_INFO,DBG_INFO_PRINT,"Removing thread data of %d threads \n",\
            mem_info.num_of_threads);
    /* Free the thread structure pointer */
    if (mem_info.tdata_hp) {
        free (mem_info.tdata_hp);
    }
    mem_info.tdata_hp = NULL;
    /* Make the number of threads variable 0*/
    mem_info.num_of_threads = 0;

    display(HTX_HE_INFO,DBG_INFO_PRINT,"re_initialize_global_vars()completed \n");
    if (priv.dr_cpu_flag == 1) {
         /*  To show the status as DT instead of DD*/
        STATS_HTX_UPDATE(RECONFIG)
        /* hxfupdate(RECONFIG,&stats);*/
        priv.dr_cpu_flag = 0;
    }

} /* end of re_initialize_global_vars*/

void report_shmstat(int shm_id, char * cptr)
{
   int rc;
   char msg[1024],buf[1024];
   struct shmid_ds shm_stat;

   rc = shmctl(shm_id,IPC_STAT,&shm_stat);
   if ( rc == -1) {
      sprintf(msg,"shmctl failed : errono %d \n",errno);
      strcat(cptr,msg);
      return;
   }

   sprintf(msg,"shm info : size %ld, pid of last shmop %d, pid of "
               "creater %d, nattch %d",
               shm_stat.shm_segsz,
               shm_stat.shm_lpid,
               shm_stat.shm_cpid,
               (int )shm_stat.shm_nattch);
   strcat(cptr,msg);

   if ( shm_stat.shm_nattch != 0)  {
      sprintf(msg,", Segment attached on %s",ctime(&shm_stat.shm_atime));
      strcat(cptr,msg);
   }

   strcat(cptr,"\nSegment created by process :\n");
   sprintf(msg,"ps ew %d | grep %d \n",shm_stat.shm_cpid,shm_stat.shm_cpid);
   rc = run_aix_cmd(msg,buf,1024);
   if ( rc == 0)  {
      sprintf(buf,"Failed to get created process info : %s",msg);
   }

   strcat(cptr,buf);

  return;
}

int save_buffers(int ti, unsigned long rc, struct segment_detail sd, \
                int main_misc_count, unsigned long *seed_ptr,int pass,int trap_flag,int pi)
{
    char fname[128];
    char msg_txt[4096], msg_text[4096];
    int oflag,mode,j=sd.seg_num;
    int miscompare_count=0, page_offset;
    int  tmp_shm_size=0,buffer_size;
    int ps = sd.page_index; /* page index */
    unsigned long new_offset=0;
    unsigned long pat_offset=0; /* pattern offset when pat len > 8 we need this */
    unsigned long expected_pat = 0; /* pattern value which is expected */
    char **shr_mem_ptr =mem_info.tdata_hp[ti].page_wise_t[ps].shr_mem_hptr;
    unsigned long *shm_size_ptr=mem_info.tdata_hp[ti].page_wise_t[ps].shm_sizes;
    int *pshmid=mem_info.tdata_hp[ti].page_wise_t[ps].shmids;
    int width; /* temporary width variable used */

    if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
        width = LS_DWORD;
    }
    else {
        width = stanza_ptr->width;
    }

	displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,
			 "#1.save_buffers, seg_num=%d,page_index=%d,"
			 "width=%d ,shm_size = 0x%lx and seed = 0x%lx,ptr=%p,rc=%ld\n",
			 j,ps,width, sd.seg_size,*seed_ptr,shr_mem_ptr[j],rc);

    oflag = O_CREAT | O_WRONLY | O_TRUNC;
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    if (ps == PAGE_INDEX_16G) {
        new_offset = sd.sub_seg_num*sd.sub_seg_size;
    }


    do
    {
		displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "#1.0.save_buffers\n");
        pat_offset = ((rc-1+(new_offset/width))*(width))\
                                    %(stanza_ptr->pattern_size[pi]);
        if ( stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
            expected_pat = *seed_ptr;
        }else {
            if ( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                expected_pat =  (unsigned long)(shr_mem_ptr[j]+((rc-1)*(width)+new_offset));
            }
            else {
                expected_pat = *(unsigned long *)(&stanza_ptr->pattern[pi][pat_offset]);
            }
        }
		memcpy(bit_pattern,(char*)&expected_pat,sizeof(unsigned long));

        switch(width){
            case LS_DWORD: {
                        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "miscomparing dword(0x%lx)"
                                 " at dword Address(0x%p)\nActual misc Addr=0x%p,misc dword data=0x%lx",\
                                *(unsigned long *)(shr_mem_ptr[j]+((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),
                                (shr_mem_ptr[j]+((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),\
                                (shr_mem_ptr[j]+(rc-1)*(width)+new_offset),\
                                *(unsigned long*)(shr_mem_ptr[j]+(rc-1)*(width)+new_offset));
                        break;
                    }
            case LS_WORD: {
                        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "miscomparing word(0x%x)"
                                 " at dword Address(0x%p)\nActual misc Addr=0x%p,misc data word=0x%x",\
                                *(unsigned int*)(shr_mem_ptr[j]+((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),
                                (shr_mem_ptr[j]+((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),\
                                (shr_mem_ptr[j]+(rc-1)*(width)+new_offset),\
                                *(unsigned int*)(shr_mem_ptr[j]+(rc-1)*(width)+new_offset));
                        break;
                    }
            case LS_BYTE: {
                        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "miscomparing dword(0x%x)"
                                 " at dword Address(0x%p)\nActual misc Addr=0x%p,misc data byte=0x%x",\
                                *(unsigned char *)(shr_mem_ptr[j]+((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),
                                (shr_mem_ptr[j]+((((rc-1)*(width))/LS_DWORD)*MIN_PATTERN_SIZE)+new_offset),\
                                (shr_mem_ptr[j]+(rc-1)*(width)+new_offset),\
                                *(unsigned char*)(shr_mem_ptr[j]+(rc-1)*(width)+new_offset));
                        break;
                    }
        }

        sprintf(msg_text,"MEMORY MISCOMPARE(hxemem64) in rule %s,pass= %d,Rules file=%s\n"
                "Shared memory id(shmid[%d])=%d,Shared memory Segment Starting EA(shr_memp[%d])=0x%p,\n"
                "Miscompare Offset in the Shared memory segment is (%ld x %d) %ld bytes from the "
                "starting location,\nData expected = 0x%lx\nData retrieved "
                "= 0x%lx\nMemory Miscompare location EA = 0x%p \nShared memory segment"
                " size = %ld \nShared memory segment consists of pages of size =%s\n"
                "read/write width = %d, Trap_Flag = %d Sub-Segment Number=%lu, Thread=%d,num_of_threads=%d\n",\
                stanza_ptr->rule_id,\
                (pass-(stanza_ptr->num_oper))+1,priv.rules_file_name,j,pshmid[j],j,shr_mem_ptr[j],\
                (rc-1+(new_offset/width)),width, \
                (rc-1+(new_offset/width))*(width),\
                expected_pat,\
                *(unsigned long *)(shr_mem_ptr[j]+((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),\
                (shr_mem_ptr[j]+((rc-1)*(width)+new_offset)),shm_size_ptr[j],page_size[ps],\
                width,trap_flag,sd.sub_seg_num,ti,mem_info.num_of_threads);

        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "#1.1.save_buffers\n%s", msg_text);

        #ifdef __HTX_LINUX__
        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "#2.save_buffers\n");
        if (trap_flag) {
                do_trap_htx64 (    0xBEEFDEADFFFFFFFF,\
                                (unsigned long) shr_mem_ptr[j],\
                                (unsigned long) (shr_mem_ptr[j]+(rc-1)*(width)+new_offset),\
                                expected_pat, \
                                (unsigned long)(shm_size_ptr[j]),\
                                (unsigned long) &msg_text[0]\
                            );
        }
        #endif

        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "#2.save_buffers\n");
        /* If pattern is RANDOM cannot generate dumps and
         * If operation is RIM it does write and read compare immediately so cannot dump buffer files
         */
        if ((stanza_ptr->pattern_type[pi] != PATTERN_RANDOM ) && (stanza_ptr->operation != OPER_RIM )) {

            page_offset = (rc-1)*width;/* Offset of miscompare in bytes */
            new_offset = new_offset + (page_offset/(4*KB))*(4*KB); /* The 4k page aligned offset in bytes */
            page_offset &= 0xfff;

            tmp_shm_size = shm_size_ptr[j] - new_offset; /* Size of the remaining segment to be compared now */
            if (ps == PAGE_INDEX_16G ) {
                tmp_shm_size = tmp_shm_size % sd.sub_seg_size;
            }

            displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "#3.save_buffers\n");

            /* Size of the page=4096. If it is last page, it can be less */
			char log_dir[256];
			strcpy(log_dir,stats.htx_exer_log_dir);
			strcat(log_dir,"/hxemem");
            buffer_size = (tmp_shm_size>=(4*KB)) ? (4*KB):tmp_shm_size;
            sprintf(fname,"%s.%s.shmem.%d.%d_addr_0x%016lx_offset_%d",log_dir,stanza_ptr->rule_id,\
                        main_misc_count,miscompare_count, (unsigned long)(shr_mem_ptr[j]+new_offset), page_offset);
            displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "miscompare file name = %s\n",fname);

              if( (main_misc_count<11) && (miscompare_count < 10))    /* Number of miscompares to be saved is 10 */
              {
                sprintf(fname,"%s.%s.shmem.%d.%d_addr_0x%016lx_offset_%d",log_dir,stanza_ptr->rule_id,\
                        main_misc_count,miscompare_count, (unsigned long )(shr_mem_ptr[j]+new_offset), page_offset);
                hxfsbuf((shr_mem_ptr[j]+new_offset),buffer_size, fname, &stats);
                sprintf(msg_txt,"The miscomparing data in data buffer segment is in the file %s.\n",fname);
                strcat(msg_text,msg_txt);
                sprintf(fname,"%s.%s.pat.%d.%d_addr_0x%016lx_offset_%d",log_dir,stanza_ptr->rule_id,\
                        main_misc_count,miscompare_count, (unsigned long )(shr_mem_ptr[j]+new_offset),page_offset);
                sprintf(msg_txt,"The pattern data which is expected is in the file %s.\n",fname);
                strcat(msg_text,msg_txt);
                hxfsbuf(bit_pattern, MIN_PATTERN_SIZE, fname,&stats);
                miscompare_count++;
            }
            if(main_misc_count > 10) {
                sprintf(msg_txt,"Maximum number of miscompare buffers(10) have already been saved.\n"
                        " No Dump file correspoding to this miscompare will be found in the /tmp directory.\n");
                strcat(msg_text,msg_txt);
            }

            displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "#4.save_buffers\n");

            displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "%s",msg_text);

            new_offset = new_offset +(4*KB);    /* Bytes already compared till now (4k page aligned) */

            if (ps != PAGE_INDEX_16G) {
                if(new_offset >= shm_size_ptr[j])
                    break;
            } else {
                if(new_offset >= (sd.sub_seg_num+1)*sd.sub_seg_size)
                    break;
            }

        } else { /* pattern_type == PATTERN_RANDOM */
            page_offset = (rc-1)*width;/* Offset of miscompare in bytes */
            new_offset = new_offset+page_offset+width;
            sprintf(msg_txt,"PATTERN is RANDOM or it is a RIM Operation so hxemem64 is not generating dump.\n"
                        "hxemem64 starts compare from the next dword/word/byte\n");
            strcat(msg_text,msg_txt);
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"%s",msg_text);
        }

        tmp_shm_size = shm_size_ptr[j] - new_offset; /* Size of the remaining segment to be compared now */
        if (ps == PAGE_INDEX_16G ) {
            tmp_shm_size = tmp_shm_size % sd.sub_seg_size;
        }
        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "#save_buffers:ptr=0x%lx,shm_size=0x%lx.. \n",\
                shr_mem_ptr[j]+new_offset,tmp_shm_size);

      	if (stanza_ptr->operation == OPER_NSTRIDE) {
			return miscompare_count;
		} else if ( stanza_ptr->operation != OPER_RIM ) {
            /* NOT RIM so normal COMPARE routines */
            if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling wr_addr_cmp_dword.. \n");
                rc = mem_operation(ADDR_COMP,(tmp_shm_size)/LS_DWORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
            } else if( stanza_ptr->pattern_type[pi] == PATTERN_SIZE_NORMAL ) {
                if (stanza_ptr->width == LS_DWORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling wr_cmp_dword.. \n");
                    rc = mem_operation(COMP_DWORD,(tmp_shm_size)/LS_DWORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
                else if(stanza_ptr->width == LS_WORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling wr_cmp_word.. \n");
                    rc = mem_operation(COMP_WORD,(tmp_shm_size)/LS_WORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
                else if(stanza_ptr->width == LS_BYTE) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling wr_cmp_byte.. \n");
                    rc = mem_operation(COMP_BYTE,(tmp_shm_size), shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
            } else if( stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
                if (stanza_ptr->width == LS_DWORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling RAND wr_cmp_dword.. \n");
                    rc = rand_operation(COMP_DWORD,(tmp_shm_size)/LS_DWORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)seed_ptr);
                }
                else if(stanza_ptr->width == LS_WORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling RAND wr_cmp_word.. \n");
                    rc = rand_operation(COMP_WORD,(tmp_shm_size)/LS_WORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)seed_ptr);
                }
                else if(stanza_ptr->width == LS_BYTE) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling RAND wr_cmp_byte.. \n");
                    rc =rand_operation(COMP_BYTE,(tmp_shm_size), shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)seed_ptr);
                }
            } else if( stanza_ptr->pattern_type[pi] == PATTERN_SIZE_BIG ) {
                if (stanza_ptr->width == LS_DWORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling BIG wr_cmp_dword.. \n");
                    rc = pat_operation(COMP_DWORD,(tmp_shm_size)/LS_DWORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
                else if(stanza_ptr->width == LS_WORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling BIG wr_cmp_word.. \n");
                    rc = pat_operation(COMP_WORD,(tmp_shm_size)/LS_WORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
                else if(stanza_ptr->width == LS_BYTE) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling BIG wr_cmp_byte.. \n");
                    rc =pat_operation(COMP_BYTE,(tmp_shm_size), shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
            }
        } else {
            /* RIM TEST CASE */
            if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling wr_addr_cmp_dword.. \n");
                rc = mem_operation(WR_ADDR_COMP,(tmp_shm_size)/LS_DWORD, shr_mem_ptr[j]+new_offset,\
                stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
            } else if( stanza_ptr->pattern_type[pi] == PATTERN_SIZE_NORMAL ) {
                if (stanza_ptr->width == LS_DWORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling wr_cmp_dword.. \n");
                    rc = mem_operation(RIM_DWORD,(tmp_shm_size)/LS_DWORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
                else if(stanza_ptr->width == LS_WORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling wr_cmp_word.. \n");
                    rc = mem_operation(RIM_WORD,(tmp_shm_size)/LS_WORD, shr_mem_ptr[j]+new_offset,\
                       stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
                else if(stanza_ptr->width == LS_BYTE) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling wr_cmp_byte.. \n");
                    rc = mem_operation(RIM_BYTE,(tmp_shm_size), shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
            } else if( stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
                if (stanza_ptr->width == LS_DWORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling RAND wr_cmp_dword.. \n");
                    rc = rand_operation(RIM_DWORD,(tmp_shm_size)/LS_DWORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)seed_ptr);
                }
                else if(stanza_ptr->width == LS_WORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling RAND wr_cmp_word.. \n");
                    rc = rand_operation(RIM_WORD,(tmp_shm_size)/LS_WORD, shr_mem_ptr[j]+new_offset,\
                       stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)seed_ptr);
                }
                else if(stanza_ptr->width == LS_BYTE) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling RAND wr_cmp_byte.. \n");
                    rc =rand_operation(RIM_BYTE,(tmp_shm_size), shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)seed_ptr);
                }
            } else if( stanza_ptr->pattern_type[pi] == PATTERN_SIZE_BIG ) {
                if (stanza_ptr->width == LS_DWORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling BIG wr_cmp_dword.. \n");
                    rc = pat_operation(RIM_DWORD,(tmp_shm_size)/LS_DWORD, shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
                else if(stanza_ptr->width == LS_WORD) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling BIG wr_cmp_word.. \n");
                    rc = pat_operation(RIM_WORD,(tmp_shm_size)/LS_WORD, shr_mem_ptr[j]+new_offset,\
                       stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
                else if(stanza_ptr->width == LS_BYTE) {
                    displaym(HTX_HE_SOFT_ERROR, DBG_DEBUG_PRINT, "save_buffers: Calling BIG wr_cmp_byte.. \n");
                    rc =pat_operation(RIM_BYTE,(tmp_shm_size), shr_mem_ptr[j]+new_offset,\
                        stanza_ptr->pattern[pi], trap_flag, &sd, stanza_ptr,(unsigned long)&stanza_ptr->pattern_size[pi]);
                }
            }
        }  /* else < RIM TEST CASE ENDS */

	displaym(HTX_HE_MISCOMPARE, DBG_MUST_PRINT,"MISCOMPARE: hxemem64 miscompare hit, rc = 0x%lx \n",rc);
        if(rc==0) {
            break;
        }
        if (priv.exit_flag == 1) {
				release_thread_memory(ti);
                pthread_exit((void *)0);
        } /* endif */

    } while(1);
    return miscompare_count;
}

/*This function checks if the filesystem is located on RAM, if yes then dma_flag will be set
 so that DMA case can be excluded in such case*/

int chek_if_ramfs(void)
{
    int filedes=0;
    unsigned int mode_flag;
    char pattern_nm[256];
    char* pat_ptr = getenv("HTXPATTERNS");
    if(pat_ptr == NULL){
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s(): env variable HTXPATTERNS is not set,pat_ptr=%p"
        ", thus exiting..\n",__LINE__,__FUNCTION__,pat_ptr);
        exit(1);
    }else{
        strcpy(pattern_nm,pat_ptr);
    }
    strcat(pattern_nm,"/HEXFF");

    mode_flag = S_IWUSR | S_IWGRP | S_IWOTH;
    errno = 0 ;

    displaym(HTX_HE_INFO, DBG_IMP_PRINT, "chek_if_ramfs : Entered chek_if_ramfs \n");
    filedes = open(pattern_nm, O_DIRECT | O_RDONLY, mode_flag);
    if ( errno == 22) {
        displaym(HTX_HE_INFO, DBG_MUST_PRINT,"chek_if_ramfs :open call returned with EINVAL(errno = %d).This can\
be because if filesystem is located on RAM,then  O_DIRECT flag is considered as invalid, in such case we will skip DMA test\
case. \n",errno);

         dma_flag = 1;
         errno = 0 ;
    }

    if(filedes > 0 ) {
           displaym(HTX_HE_INFO, DBG_IMP_PRINT, "chek_if_ramfs : closing filedes = %d \n",filedes);
           close (filedes);
    }

return(0);
}

#ifndef __HTX_LINUX__

int check_ame_enabled()
{
	int rc = 0;
	lpar_info_format2_t info;

	rc = lpar_get_info(LPAR_INFO_FORMAT2, &info, sizeof(info));
	if (rc) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "lpar_get_info failed with errno: %d\n", errno);
		return -1;
	}
	if ((info.lpar_flags & LPAR_INFO2_AME_ENABLED) == LPAR_INFO2_AME_ENABLED) {
		AME_enabled = 1;
	} else {
		AME_enabled = 0;
	}
	return rc;
}

/*********SRAD functions ******/
/*The function convert_64k converts the input size of bytes into 64k pages
 * INPUT : size of bytes to be converted in 64k PAGES
 *OUTPUT: It will convert 4k pages to 64k pages
 *RETURN VALUE: on success returns 0
 * on error condition returns -1
 *NOTE:This function can be neglected once AIX handles 64k page conversion efficiently.
 */
int  convert_64k(void *tnum)
{
    int    rc, id;
    struct shmid_ds shm;
    void  *o_addr;
    ulong *addr;
	int tn = *(int *)tnum;
	ulong sz = thp[tn].sz;
    displaym(HTX_HE_INFO, DBG_IMP_PRINT,"convert_64k:convert_64k is started coversion");
    displaym(HTX_HE_INFO, DBG_IMP_PRINT,"convert_64k:Size to convert to 64k is %ld  bytes\n", sz);
	displaym(HTX_HE_INFO, DBG_IMP_PRINT,"convert_64k:tn = %d \t sz = %ul \n",tn,sz);

	do_the_bind_proc(BIND_TO_THREAD, tn,-1);

    id = rc = shmget(IPC_PRIVATE, sz, IPC_CREAT|S_IRUSR|S_IWUSR);
        if (rc == -1) {
	    displaym(HTX_HE_INFO, DBG_MUST_PRINT,"convert_64k:shmget fail rc = %d,errno \n",rc,errno);
            return(rc);
        }

        shm.shm_pagesize = 1 << 16;

        rc = shmctl(id, SHM_PAGESIZE, &shm);
        if (rc == -1) {
	    displaym(HTX_HE_INFO, DBG_MUST_PRINT,"convert_64k:shmctl fail rc = %d,errno \n",rc,errno);
            return(rc);
        }

        o_addr = addr = shmat(id, 0, 0);
        if (o_addr == (void*)-1) {
	    displaym(HTX_HE_INFO, DBG_MUST_PRINT,"convert_64k:shmat fail errno \n",errno);
            return(rc);
        }

        while (addr < o_addr + sz) {
            *addr = 0xDEADBEEFF00DCAFEULL;
            addr+= 4096;
        }

        rc = shmdt(o_addr);
        if (rc == -1) {
	    displaym(HTX_HE_INFO, DBG_MUST_PRINT,"convert_64k:shmdt fail rc = %d,errno \n",rc,errno);
            return(rc);
        }

        rc = shmctl(id, IPC_RMID, NULL);
        if (rc == -1){
	    displaym(HTX_HE_INFO, DBG_MUST_PRINT,"convert_64k:2nd shmctl fail rc = %d,errno \n",rc,errno);
            return(rc);
        }

		displaym(HTX_HE_INFO, DBG_IMP_PRINT,"convert_64k:coversion DONE \n");
		release_thread_memory(tn);
		pthread_exit((void *)0);
        /* return 0; */
}
#endif

/*The function fill_srad_data Fills the all SRAD data to the structure srad_info
 * RETURN VALUE:
 * return 0 on successful
 * return -l on error condition
 */
int fill_srad_data()
{
    int numrads,sradidx,rc,cpu;
    int i,j,cpu_count;

#ifndef __HTX_LINUX__
	int sradsdl, ref1sdl,vmget_ret,maxprocs,tmpcpid;
    rsethandle_t  sysrset = NULL;
    rsethandle_t srad = NULL;
    struct vm_srad_meminfo srad_info = { 0 };

    sradsdl = rs_getinfo(NULL, R_SRADSDL, 0);
    /* Allocate the system rset */
    sysrset = rs_alloc(RS_SYSTEM);
    numrads = rs_numrads(sysrset, sradsdl, 0);
    if (numrads < 0) {
        displaym(HTX_HE_INFO, DBG_MUST_PRINT,"fill_srad_data:numsrads failed with errno = %d\n",errno);
	return -1;
    }
    displaym(HTX_HE_INFO, DBG_IMP_PRINT,"fill_srad_data:numrads = %d \n",numrads);
    num_of_srads = numrads; /*save the number of srads value in global variable */

    srad = rs_alloc(RS_EMPTY);
    for (sradidx = 0; (sradidx < numrads) && (sradidx < MAX_SRADS); sradidx++) {
        rc = rs_getrad(sysrset, srad, sradsdl, sradidx, 0);
        if (rc < 0){
            displaym(HTX_HE_INFO, DBG_MUST_PRINT,"fill_srad_data:rs_getrad failed for sradd = %d"
	    " \t errno = %d \t rc = %d \n",sradidx,errno,rc);
	    return -1;
        }

        srad_info_array[sradidx].sradid = sradidx;
        srad_info.vmsrad_in_size=sizeof(struct vm_srad_meminfo);
        vmget_ret = vmgetinfo(&srad_info, VM_SRAD_MEMINFO, sradidx);
        if (vmget_ret){
            displaym(HTX_HE_INFO, DBG_MUST_PRINT,"fill_srad_data:vmgetinfo  failed with retun value %d and errno = %d \n",\
	    vmget_ret,errno);
        }
        displaym(HTX_HE_INFO, DBG_IMP_PRINT,"vmgetinfo : srad_info.vmsrad_total=%lld \t,srad_info.vmsrad_free=%lld \n",\
	srad_info.vmsrad_total,srad_info.vmsrad_free);
        srad_info_array[sradidx].total_srad_mem_avail = srad_info.vmsrad_total;
        srad_info_array[sradidx].total_srad_mem_free  = srad_info.vmsrad_free;

        /* if srad_info.vmsrad_total or srad_info.vmsrad_free is 0 then don't store those processors skip to next srad */
       /* if(srad_info.vmsrad_total == 0 || srad_info.vmsrad_free == 0){
		    continue;
		}*/

        srad_info_array[sradidx].cpuidx = 0;
        maxprocs = rs_getinfo(NULL, R_MAXPROCS, 0);
        displaym(HTX_HE_INFO, DBG_IMP_PRINT,"maxproc = %d\n",maxprocs);
        for (cpu=0; cpu < maxprocs; cpu++) {
            /* Is the cpu in the SRAD?  */
            if (rs_op(RS_TESTRESOURCE, srad, NULL, R_PROCS, cpu)){
                tmpcpid = srad_info_array[sradidx].cpuidx;
                srad_info_array[sradidx].cpulist[tmpcpid] = cpu;
                srad_info_array[sradidx].cpuidx++;
             }
        }/*End of cpu loop */

    }/* End of SRAD loop */
#else
	FILE *fp;
	char command[200],fname[100];
	int cur_node, num_procs, lcpu;
	int srad_mem_avail, srad_mem_free;

    strcpy(fname,stats.htx_exer_log_dir);
	strcat(fname,"/mem_node_details");
    char* scripts_tmp_ptr = getenv("HTXSCRIPTS");
    if(scripts_tmp_ptr == NULL){
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s(): env variable HTXSCRIPTS is not set,scripts_tmp_ptr=%p"
        ", thus exiting..\n",__LINE__,__FUNCTION__,scripts_tmp_ptr);
        return (-1);
    }else{
        strcpy(command,scripts_tmp_ptr);
    }
	sprintf(command, "%s/get_mem_node_details.sh > %s", command, fname);
	
	if ( (rc = system(command)) == -1 ) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "system command to get_node_details failed with %d", rc);
		return(-1);
	}

	if ((fp=fopen(fname,"r"))==NULL){
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "fopen of file %s failed with errno=%d",fname,errno);
		return(-1);
	}

	/* Get total number of chips */
	rc = fscanf(fp,"num_nodes= %d\n",&numrads);
	if (rc == 0 || rc == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "fscanf of num_nodes on file %s failed with errno =%d",fname,errno);
		return(-1);
	}
	displaym(HTX_HE_INFO, DBG_IMP_PRINT,"fill_srad_data:numrads = %d \n",numrads);
	num_of_srads = numrads; /*save the number of srads value in global variable */

	for (sradidx = 0; (sradidx < numrads) && (sradidx < MAX_SRADS); sradidx++) {
		/* Fetch the current chip and the num procs in the chip */
		rc = fscanf(fp,"node_num=%d,mem_avail=%d,mem_free=%d,cpus_in_node=%d,cpus",&cur_node,&srad_mem_avail,&srad_mem_free, &num_procs);
		displaym(HTX_HE_INFO, DBG_MUST_PRINT, "node_num=%d,mem_avail=%d,mem_free=%d,cpus_in_node=%d\n", cur_node, srad_mem_avail, srad_mem_free, num_procs);
		if (rc == 0 || rc == EOF) {
			displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "fscanf: sradidx=%d,Fetching of cpus in node error from"
								" file %s failed with rc = %d, errno =%d",sradidx,fname,rc, errno);
			return(-1);
		}

		srad_info_array[sradidx].sradid = cur_node;
		srad_info_array[sradidx].total_srad_mem_avail = (unsigned long long)srad_mem_avail * (KB);
		srad_info_array[sradidx].total_srad_mem_free  = (unsigned long long)srad_mem_free * (KB);
		/*if (srad_info_array[sradidx].total_srad_mem_free == 0 ) {
			fscanf(fp, "%*s\n");
			continue;
		}*/
		srad_info_array[sradidx].cpuidx = num_procs;
		for(cpu=0; cpu < num_procs; cpu++) {
			rc = fscanf(fp,":%d",&lcpu);
			if ( rc == 0 || rc == EOF) {
				displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "fscanf: cpu fetch for ndoe=%d caused error from "
								"file %s failed with errno =%d",cur_node,fname,errno);
				return(-1);
			}
			srad_info_array[sradidx].cpulist[cpu] = lcpu;
		}
		rc = fscanf(fp,"\n");
		if (rc == EOF) {
			displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "fscanf: expecting new line character at the end of"
							" node %d data reading from file %s failed with errno =%d",sradidx, fname, errno);
			return -1;
		}
	}
	fclose(fp);

#endif

	/*Display routines prints the deatiled data */
    displaym(HTX_HE_INFO, DBG_IMP_PRINT,"****** fill_srad_data():  number of srads = %d *******\n",numrads);
    for(i = 0;i < numrads; i++){
        displaym(HTX_HE_INFO, DBG_IMP_PRINT,"srad no = %d \t  available_mem = %ld \t "
        "free_mem = %ld \t num_of_cpus = %d  \n", srad_info_array[i].sradid,srad_info_array[i].total_srad_mem_avail,
	srad_info_array[i].total_srad_mem_free,srad_info_array[i].cpuidx);
        cpu_count  = srad_info_array[i].cpuidx;
        for(j = 0; j < cpu_count; j++) {
            displaym(HTX_HE_INFO, DBG_IMP_PRINT,"srad_info_array[%d].cpulist[%d] = %d \n",i,j,srad_info_array[i].cpulist[j]);
        }
    }

    return(0);
}/*End of fill_srad_data */



/*********SRAD FUNCTIONS finishes *****************/

#ifndef __HTX_LINUX__

void SIGCPUFAIL_handler(int sig, int code, struct sigcontext *scp)
{
	char hndlmsg[512];
	int no_of_proc;

	hxfmsg(&stats,0,HTX_HE_INFO,"SIGCPUFAIL signal received.\n");

	priv.sigvector.sa_handler = (void (*)(int))SIG_IGN;
	(void) sigaction(SIGCPUFAIL, &priv.sigvector, (struct sigaction *) NULL);

	no_of_proc = get_num_of_proc();
	if (mem_info.tdata_hp[mem_info.num_of_threads - 1].thread_num == (no_of_proc - 1) && mem_info.tdata_hp[mem_info.num_of_threads - 1].tid != -1) {
		if (bindprocessor(BINDTHREAD, mem_info.tdata_hp[mem_info.num_of_threads - 1].kernel_tid, PROCESSOR_CLASS_ANY)) {
			sprintf(hndlmsg,"Unbind failed. errno %d, for thread num %d, TID %d.\n", errno, mem_info.num_of_threads - 1, mem_info.tdata_hp[mem_info.num_of_threads - 1].tid);
		}
	}
	priv.sigvector.sa_handler = (void (*)(int))SIGCPUFAIL_handler;
	(void) sigaction(SIGCPUFAIL, &priv.sigvector, (struct sigaction *) NULL);
}
#endif

#ifdef    _DR_HTX_

void SIGRECONFIG_handler(int sig, int code, struct sigcontext *scp)
{
    int rc;
    char hndlmsg[512];
    dr_info_t dr_info;          /* Info about DR operation, if any */
    int i, bound_cpu;

	sig_flag = 1;    
    hxfmsg(&stats,0,HTX_HE_INFO,"DR: SIGRECONFIG signal received \n");
		
    do {
        rc = dr_reconfig(DR_QUERY,&dr_info);
    } while ( rc < 0 && errno == EINTR);
    if ( rc == -1) {
        if ( errno != ENXIO){
    		hxfmsg(&stats,errno,HTX_HE_HARD_ERROR, "dr_reconfig(DR_QUERY) call failed. \n");
        }
		sig_flag = 0;
        return;
    }

    if (dr_info.mem == 1){
    	sprintf(hndlmsg,"DR: DLPAR details"
		    "Phase - Check:  %d, Pre: %d, Post: %d, Post Error: %d\n"\
		    "Type - Mem add: %d remove: %d, ent_cap = %d, hibernate = %d \n",\
		     dr_info.check, dr_info.pre, dr_info.post, dr_info.posterror, dr_info.add, dr_info.rem, dr_info.ent_cap, dr_info.hibernate); 
		hxfmsg(&stats,0,HTX_HE_INFO,hndlmsg);
    }

    if (dr_info.cpu == 1 && (dr_info.rem || dr_info.add) ) {
		if(dr_info.rem && dr_info.check){
			cpu_dr_set = dr_info.bcpu;
		}
    	sprintf(hndlmsg,"DR: DLPAR details"
		    "Phase - Check:  %d, Pre: %d, Post: %d, Post Error: %d\n"\
		    "Type - Cpu add: %d remove: %d, bcpu = %d \n",\
		     dr_info.check, dr_info.pre, dr_info.post, dr_info.posterror, dr_info.add, dr_info.rem, dr_info.bcpu); 
		hxfmsg(&stats,0,HTX_HE_INFO,hndlmsg);
    }
	if ( stanza_ptr == NULL ) {
		stanza_ptr = &stanza[0];
	}

	if ( stanza_ptr->operation == OPER_NX_MEM ) {
		/* This is case of OPER_NX_MEM. Need to handle DR separately for NX operations as fill_segment_data has not been called for it. */
		NX_SIGRECONFIG_handler(&dr_info);
		return;
	}

    /*
     * Check-phase for CPU DR
     * Handle only CPU removals, new CPUs will be used from the next stanza.
     */
    if (dr_info.check && dr_info.cpu && dr_info.rem) {



        /* Check phase; set tracker count  */
        cpu_dr_tracker_count++;
		logical_cpus[dr_info.bcpu]=0;	

        /* Look for the thread bound to the CPU in question */
        for(i=0; i<mem_info.num_of_threads; i++) {
            /* Also check if the running thread has already completed. */
            if ((mem_info.tdata_hp[i].bind_proc == dr_info.bcpu) &&
                (mem_info.tdata_hp[i].tid != -1)) {
                /* Unbind thread from the CPU under DR */
                if (bindprocessor(BINDTHREAD, mem_info.tdata_hp[i].kernel_tid,
                    PROCESSOR_CLASS_ANY)) {
                    if (errno == ESRCH) {
                        continue;
                    }
                    sprintf(hndlmsg,"Unbind failed. errno %d, for thread num %d"
                            ", TID %d.\n", errno, i, mem_info.tdata_hp[i].tid);
            		hxfmsg(&stats,0,HTX_HE_INFO,hndlmsg);
                }
        		/*
        		 * More than one thread could be bound to the same CPU.
        		 * as bind_proc = bind_proc%get_num_of_proc()
        		 * Run through all the threads, don't break the loop.
        		 */
            }
        }
    	sprintf(hndlmsg,"DR: In check phase and cpu:%d is removed, we unbind threads which are affectd\n",dr_info.bcpu);
		hxfmsg(&stats,0,HTX_HE_INFO,hndlmsg);

        /* priv.dr_cpu_flag = TRUE; */
        if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)){
            sprintf(hndlmsg,"dr_reconfig(DR_RECONFIG_DONE) failed."
                    " error %d \n", errno);
            hxfmsg(&stats,0,HTX_HE_INFO,hndlmsg);
        }
		else{
			sprintf(hndlmsg,"DR:DR_RECONFIG_DONE Success!!,in check phase for cpu=%d \n",dr_info.bcpu);
			hxfmsg(&stats,0,HTX_HE_INFO,hndlmsg);

		}
		sig_flag = 0;
        return;
    }

    /* Post/error phase; reset tracker count  */
    if ((dr_info.post || dr_info.posterror) && dr_info.cpu && dr_info.rem) {
		update_sys_detail_flag = 1;  /*falg is set to recollect cpu details for tlbie test case*/
		cpu_dr_set = -1;			/*unset global dr lcpu so that thread can bind to respective lcpu*/
        if (cpu_dr_tracker_count > NO_CPU_DR) {
            cpu_dr_tracker_count--;
        }
    }

    /* For any other signal check/Pre/Post-phase, respond with DR_RECONFIG_DONE */
	if (dr_info.check || dr_info.pre || dr_info.post || dr_info.posterror) {
		if (dr_info.mem && dr_info.check) {
			sprintf(hndlmsg,"DR:Mem DR operation performed,setting  mem_DR_done = 1");
			hxfmsg(&stats,0,HTX_HE_INFO,hndlmsg);
			mem_DR_done = 1;
		}
        if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)){
            sprintf(hndlmsg,"dr_reconfig(DR_RECONFIG_DONE) failed."
                    " error %d ,check=%d,pre=%d,post=%d\n",dr_info.check,dr_info.pre,dr_info.post,errno);
    		hxfmsg(&stats,0,HTX_HE_INFO,hndlmsg);
        }
		else{
			sprintf(hndlmsg,"DR:DR_RECONFIG_DONE Successfully after check=%d, pre=%d, post=%d   phhase !! \n",dr_info.check,dr_info.pre,dr_info.post);
			hxfmsg(&stats,0,HTX_HE_INFO,hndlmsg);
		}
    }
	sig_flag = 0;

    return;
}
#endif

int detailed_display(){

	int i,j,k,m,n,thr_num;
    char dump_file[100],msg[1000];
    FILE *fp;

    strcpy(dump_file,stats.htx_exer_log_dir);
	strcat(dump_file,"/htx_mem_detail");


    fp = fopen(dump_file, "w");
    if ( fp == NULL) {
        sprintf(msg, "Error opening %s file,errno:%d \n",dump_file,errno);
        hxfmsg(&stats, -1, HTX_HE_HARD_ERROR, msg);
        exit(-1);
    }

	fprintf(fp, "STANZA:rule_id = %s\n", stanza_ptr->rule_id);
	if(stanza_ptr->affinity == TRUE) {
		fprintf(fp,"*******************************************************************************\n");
		fprintf(fp," Thread \t page size \t segment no \t segment size \n");
		thr_num=0;
 		for(n = 0; n < num_of_srads; n++){
    		for(m = 0; m < srad_info_array[n].cpuidx; m++) {

				for(i=0; i<MAX_NUM_PAGE_SIZES; i++) {
					for (k=0; k< mem_info.tdata_hp[thr_num].page_wise_t[i].num_of_segments; k++) {

                     	fprintf(fp," %d \t\t\t %s \t\t\t %d \t\t\t %lu \n",\
						 	thr_num,page_size[i], k, mem_info.tdata_hp[thr_num].page_wise_t[i].shm_sizes[k]);
                	}

           		}
						thr_num++;
      		}
		}
	fprintf(fp,"***********************************************************************************\n");
	}else
	{
     fprintf(fp,"*******************************************************************************\n");
	 fprintf(fp,"\t Thread \t page size \t segment no \t segment size \n");

     for (j=0; j< mem_info.num_of_threads; j++){

		   for(i=0; i<MAX_NUM_PAGE_SIZES; i++) {

				for (k=0; k< mem_info.tdata_hp[j].page_wise_t[i].num_of_segments; k++) {

                     fprintf(fp,"\t %d \t\t\t %s \t\t\t %d \t\t\t %lu \n",\
						 j,page_size[i], k, mem_info.tdata_hp[j].page_wise_t[i].shm_sizes[k]);
                }

            }

      }

      fprintf(fp,"*****************************************************************************\n");
	}

	fclose(fp);
	return 0;
}

#ifdef    _DR_HTX_
void NX_SIGRECONFIG_handler(dr_info_t *dr_info_ptr)
{
	/* hxenx842 should exit only when any CPU is getting removed.
	 * In all other cases, just ack the signal. No operations needed.
	 */

	if ( dr_info_ptr->cpu & dr_info_ptr->rem ) {
		priv.exit_flag = 1;
	}

	dr_reconfig(DR_RECONFIG_DONE,dr_info_ptr);
	hxfupdate(RECONFIG, &stats);
}
#endif
void alocate_mem_for_mem_info_num_of_threads(){
	int ti =0;
	mem_info.tdata_hp = (struct thread_data * ) malloc(mem_info.num_of_threads*(sizeof(struct thread_data)));
	if((unsigned long*)mem_info.tdata_hp == NULL) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
			"alocate_mem_for_mem_info_num_of_threads:(malloc failed) Creation of thread data "
			"structures failed! errno = %d(%s)\n", errno, strerror(errno));
	}
	/* memset the whole of the tdata_hp pointer area..i.e, n segments of struct thread_data to zero*/
	memset(mem_info.tdata_hp, 0x00, mem_info.num_of_threads*(sizeof(struct thread_data)));
	for(ti=0;ti<mem_info.num_of_threads;ti++)
	{
		mem_info.tdata_hp[ti].tid = -1;
		mem_info.tdata_hp[ti].thread_num= -1;
	}
}


void atexit_release_memory()
{
	int ti;
	for (ti = 0; ti <mem_info.num_of_threads; ti++) {
		release_thread_memory(ti);
	}
	re_initialize_global_vars();
}
