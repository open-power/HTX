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
/* IBM_PROLOG_END_TAG */

#include "hxecache.h"
#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

/* Global data structures and variables go here */

/* Global data structures */
struct ruleinfo		h_r[MAX_TC];
struct ruleinfo 	*current_rule;
struct ruleinfo 	*prev_rule;
struct ruleinfo 	*next_rule;
int					num_testcases;
int					log_level = MINIMUM;

int                         huge_page_size;
int                         half_huge_page_size;
int  						crash_on_misc_global;
char 						*htxkdblevel;
int							shm_id,cache_page_req=0,prefetch_page_req=0;
int 						update_sys_detail_flag = 0;
int				   			test_l2=0, test_l3=0;
int							gang_size = GANGSIZE;
char						msg[1500];
char						err_string[1500];
unsigned int				cache_rollover_testcase, cache_bounce_testcase;
unsigned int				shm_size;
pthread_attr_t				thread_attr;
struct htx_data				h_d;
struct thread_context 		th_array[MAX_CPUS];
int pcpus_thread_wise[MAX_THREADS_PER_CHIP] = {[0 ... (MAX_THREADS_PER_CHIP - 1)] = -1} ;		 /*array to hold physical cpus thtreadwise:Hotplug */
int 						tot_thread_count =  -1;												 /* counter to keep track of remaining threads during Hotplug*/
#ifndef __HTX_LINUX__
struct sigaction 			sigvector_dr;
sigset_t 					mask;
int 						dr_rc;
#endif

struct memory_set 			mem;
struct sys_info   			system_information;														/* Structure to store all system config related  info		  	*/
struct sigaction  			sigvector;
struct sigaction  			sigvector_usr2;
CPU_MAP		   				cpu_map_for_instance;
int							start_of_prefetch_memory_index;										 	/* Index into the contiguous memory array, where			   	*/
																									/* prefetch memory starts									  	*/
int							instance_number;
int							num_cache_threads, total_prefetch_threads, total_cache_threads;		   	/* Total no of cache threads & prefetch threads					*/
char					   	rule_file_name[100], device_name[50], run_type[5];
long long				  	worst_case_memory_required, worst_case_prefetch_memory_required;		/* Largest (i.e worst case) memory required						*/
																									/* for running all of the rule stanzas sequentially				*/
unsigned int			   	worst_case_contig_mem;
volatile int			   	exit_flag = 0;														  	/* flag to indicate if SIGTERM received							*/
volatile int			   	cleanup_done=0;														 	/* Flag to indicate if mem cleanup is done					 	*/
SYS_STAT				   	sys_stat;															   	/* To hold the hardware statistics							 	*/
signed int				 	cpus_in_instance[MAX_THREADS_PER_CHIP];								 	/* Array of logical CPU numbers in this instance			   	*/
int							total_cpus;														   		/* Total number of logical CPUS in the instance					*/
int							cores_in_instance[MAX_CORES_PER_CHIP];
int							num_cores;
pthread_mutex_t 			mutex	  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t 			dr_mutex  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t 			sync_mutex ;															/* Mutex to protect manipulation of sync_cond variable			*/
pthread_cond_t 				sync_cond  ;															/* Variable to synchronize threads								*/
int 						rollover_testcase_flag = 0;			 									/* Flag determines if rollover test case is to run 				*/
int 						thread_sync_in_use = TRUE;												/* Flag to determine if thread sync mechanism is used or not	*/
FILE						*log_fp;
char 						log_file_name[256];
int							use_contiguous_pages = TRUE;
int							run_indefinite = FALSE;													/* Flag to indicate if run has to continue indefinitely.		*/
struct drand48_data 		sbuf;

void get_enabled_prefetch_algos ( unsigned int prefetch_conf , char enabled_algos[] ) ;
void populate_cores_to_disable(struct ruleinfo *ruleptr, int disable_cores[], int num_cores_disabled);
void SIGUSR2_hdl(int, int, struct sigcontext *);
int find_EA_to_RA(unsigned long ea,unsigned long long* ra);
/* Now the main function */

int main(int argc, char **argv) {
	int retcode 		= SUCCESS;
	int rc;
	int test_case		= 0;
	int i,j;
	FILE				*run_log_fp;
#if defined(RUNLOG) || defined(RUNLOG_SCREEN)
	int counter			= 0;
#endif
	
#ifdef __HTX_LINUX__
	h_d.hotplug_cpu = 1; /* Flag used to registering pid so that can receive sigusr2 from supervisor*/
#endif
	retcode = atexit(memory_set_cleanup);
	if(retcode != SUCCESS ) {
		sprintf(msg,"[%d] Error: Could not register for atexit!\n",__LINE__);
		hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
		return ( retcode );
	}
	retcode = do_initial_setup(argv);
	hxfupdate(UPDATE, &h_d);

	if ( retcode != SUCCESS ) {
		sprintf(msg,"[%d] Error:  while performing initial setup. Exitting !!\n",__LINE__);
		hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
		return ( retcode );
	}


	strcpy(log_file_name,h_d.htx_exer_log_dir);
	sprintf(log_file_name,"%s/hxecache%d.runlog",log_file_name,get_device_instance(argv[1]));
	run_log_fp = fopen(log_file_name,"w");

	if ( run_log_fp == NULL ) {
		sprintf(msg,"[%d] Error: Could not open log file (%s)for logging runtime info. Exitting! \n",__LINE__, log_file_name);
		hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
		return (-1);
	}

	log_fp = run_log_fp;

	sprintf(msg,"[%04d] Info: Hxecache run log file = %s\n",__LINE__,log_file_name);
	hxfmsg(&h_d, 0, HTX_HE_INFO, msg);

	retcode = register_signal_handlers();
	
	if ( retcode != SUCCESS ) {
		sprintf(msg,"[%d] Error: Could not register signal handlers. Exitting !!\n",__LINE__);
		hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
		return ( retcode );
	}

	retcode = get_system_hardware_details();

	if ( retcode != SUCCESS ) {
		sprintf(msg,"[%d]: Error while collecting system information. Exitting !!\n",__LINE__);
		hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
		return ( retcode );
	}
	/* Read rule file information */
	retcode = read_rule_file();
	if (retcode != SUCCESS) {
		sprintf(msg, "[%d] Error reading rule file \n",__LINE__);
		hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
		return (retcode);
	}

	if(gang_size == 1){ /* If gang size = 1, For Equaliser support */
        system_information.start_cpu_number = instance_number;
        system_information.end_cpu_number   = instance_number;
        system_information.num_cores = 1;
        system_information.number_of_logical_cpus = 1;
        total_cpus = 1;
    }
	


	retcode = find_memory_requirement();

	if ( retcode != SUCCESS ) {
		sprintf(msg,"[%d] Error: Unable to calculate memory_requirement (retcode = %d)\n",__LINE__,retcode);
		hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
		return ( retcode );
	}

	do {
		retcode = allocate_worst_case_memory();

		if ( retcode == E_NOT_ENOUGH_CONT_PAGES ) {
			cleanup_done = 0;
			if ( (rc = memory_set_cleanup())!= SUCCESS) {
				sprintf(msg,"[%d] Error: Unable to cleanup memory set\n",__LINE__);
				hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
				exit (rc);
			}
		
			for(i=0 ; i<num_testcases ;i++) {
				for ( j=0 ; j<MAX_HUGE_PAGES ; j++ ) {
					h_r[i].mem_page_status[j] = PAGE_FREE;
				}
			}
		} else if ( retcode != SUCCESS ) {
			sprintf(msg,"[%d]: Error while allocating memory (returned = %d) !!\n",__LINE__,retcode);
			hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
			cleanup_done = 0;
			if ( (rc = memory_set_cleanup())!= SUCCESS) {
				sprintf(msg,"[%d]: Error while releasing memory. Exitting !!\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				exit (rc);
			}
			return (rc);
		}

	}while ( retcode != SUCCESS );

	cleanup_done = 0;
	/* Now dump the system wide running information */
	dump_system_info();

	/* Now dump the rule structure */
	dump_rule_structure(NULL);

	/* Check whether the exerciser should proceed or not */
	if(exit_flag ) {
		sprintf(msg,"[%d] Received exit_flag, exitting\n",__LINE__);
		hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
		retcode = memory_set_cleanup();
		if ( retcode != SUCCESS ) {
			sprintf(msg,"[%d]: Error while releasing memory. Exitting !!\n",__LINE__);
			hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
			exit (retcode);
		}
		exit(retcode);
	}

    if (thread_sync_in_use == TRUE ) {
        pthread_mutex_init(&sync_mutex, NULL);
        pthread_cond_init(&sync_cond, NULL);
    }

	do {
		for ( test_case = 0 ; test_case < num_testcases ; test_case++) {
			current_rule 		= &h_r[test_case];
			prev_rule	 		= &h_r[test_case - 1];
			next_rule			= &h_r[test_case + 1];
#ifdef __HTX_LINUX__
			tot_thread_count = current_rule->num_cache_threads_to_create + current_rule->num_prefetch_threads_to_create; /* total threads need to be created*/
			if(update_sys_detail_flag){
				retcode = get_update_syschanges_hotplug();
				 if ( retcode != SUCCESS ) {
					sprintf(msg,"[%d]: Error in get_update_syschanges_hotplug()(returned = %d) !!\n",__LINE__,retcode);
					hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
				}
			}
#endif
#if defined(__HTX_MAMBO__) || defined(AWAN)
			printf("[%d] Starting stanza number %d :  %s\n",__LINE__, test_case+1, &(current_rule->rule_id[0]));
#endif
			retcode = create_and_dispatch_threads(test_case);							/* Calculate number of cache and threads to be created, allocate memory for */
																						/* them and then spawn them.												*/

			if ( retcode != SUCCESS ) {
				sprintf(msg,"[%d]: Error while Spawning threads. Exitting !!\n",__LINE__);
				hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
				do_thread_cleanup(ALL);

				if ( (retcode = memory_set_cleanup()) != SUCCESS ) {
					sprintf(msg,"[%d]: Error while releasing memory. Exitting !!\n",__LINE__);
					hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
					exit (retcode);
				}
				return ( retcode );
			}

#ifdef RUNLOG
			if ( counter < num_testcases ) {
				print_once_thread_data(current_rule);
				counter++;
			}
#endif

			/* Initialise read/write stats */
			h_d.good_reads	= 0;
			h_d.good_writes	= 0;
#if 0
			h_d.test_id		= get_test_id(&(current_rule->rule_id[0]));
#endif
			h_d.test_id		= test_case + 1;
			hxfupdate(UPDATE, &h_d);

			retcode = wait_for_threads_termination(test_case);			/* Wait for the threads to terminate.	*/

			if ( retcode != SUCCESS ) {
				sprintf(msg,"[%d] Error while waiting for threads to terminate. Exitting !!\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);

				if ( (retcode = memory_set_cleanup()) != SUCCESS ) {
					sprintf(msg,"[%d] Error while releasing memory. Exitting !!\n",__LINE__);
					hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
					exit (retcode);
				}
			}
#ifdef __HTX_LINUX__
			/*If total threads are zero meaning all cpus have been removed, then  set flag to re-calculate everythig*/
			if(tot_thread_count == 0){
				sprintf(msg,"tot_thread_count = %d \n",tot_thread_count);
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
				update_sys_detail_flag = 1;
			
			}	
			/*for ( i=0;i<MAX_THREADS_PER_CHIP;i++) {
				if(pcpus_thread_wise[i] != -1)
					printf("pcpus_thread_wise[%d]===%d\n",i,pcpus_thread_wise[i]);
			}*/
#endif

			/* Check if exit_flag is set */
			if(exit_flag){
				sprintf(msg,"[%d] Caught exit flag, exitting %s\n",__LINE__,__FUNCTION__);
				hxfmsg(&h_d,0,HTX_HE_INFO,msg);
				
				if ( (retcode = do_thread_cleanup(ALL)) != SUCCESS ) {
					sprintf(msg,"[%d] Error in cleaning Cache threads context. Exitting !!",__LINE__);
					hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
					return (retcode);
				}

				if ( exit_flag == DR_EXIT)
					hxfupdate(RECONFIG, &h_d);
				break;
			}

			/* Everything finished without error. Hence update stats and proceed to next testcase */
			retcode = update_hxecache_stats(test_case);
			if ( retcode != SUCCESS ) {
				sprintf(msg,"[%d]: Error while updating exerciser statistics !!. \n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_SOFT_ERROR, msg);
			}
			hxfupdate(UPDATE, &h_d);
#if defined(__HTX_MAMBO__) || defined(AWAN)
			printf("[%d] Completed stanza number %d :  %s\n",__LINE__, test_case+1, &(current_rule->rule_id[0]));
#endif
		}

		/* Process exit_flag to check if we need to abort */
		if(exit_flag) {
			sprintf(msg,"[%d] Received exit_flag, exitting\n",__LINE__);
			hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
			if ( exit_flag == DR_EXIT)
				hxfupdate(RECONFIG, &h_d);
			break;
		}

		/* Indicate that this stanza is over */
		h_d.test_id = 0;
		hxfupdate(FINISH, &h_d);		

#if defined(AWAN)
		run_indefinite = TRUE;
#endif
	} while((rc = strcmp(run_type, "REG") == 0) || (rc = strcmp(run_type, "EMC") == 0) || (run_indefinite == TRUE));

	/* If control reaches here, it means it is exitting normally. Hence cleanup and exit */
	cleanup_done = 0; /*hotplug change*/
	retcode = memory_set_cleanup();
	if ( retcode != SUCCESS ) {
		sprintf(msg,"[%d]: Error while releasing memory. Exitting !!\n",__LINE__);
		hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
		return ( retcode );
	}

	/* Cleanup mutexes that have been taken */
	retcode = pthread_mutex_destroy(&mutex);
	if (retcode != 0) {
		sprintf(msg,"[%d] Unable to destroy mutex \n", __LINE__);
		hxfmsg(&h_d, retcode,  HTX_HE_SOFT_ERROR, msg);
	}	

	if ( thread_sync_in_use == TRUE ) {
		rc = pthread_mutex_destroy(&sync_mutex);
		if (retcode != 0) {
			sprintf(msg,"[%d] Unable to destroy mutex \n", __LINE__);
			hxfmsg(&h_d,retcode ,  HTX_HE_SOFT_ERROR  , msg);
		}	

		retcode = pthread_cond_destroy(&sync_cond);
		if (retcode != 0) {
			sprintf(msg,"[%d] Unable to destroy condition variable \n", __LINE__);
			hxfmsg(&h_d,retcode ,  HTX_HE_SOFT_ERROR  , msg);
		}
	}	

	return ( retcode );
}

/* Core Functions definitions follow */

int get_system_hardware_details(void) {
	int rc = SUCCESS,i=0,count=0;
	htxsyscfg_smt_t  	system_smt_information;
	htxsyscfg_cpus_t 	system_cpu_information;


    char *env_var;

	env_var = getenv("EQUALISER_FLAG");
	if ( env_var != NULL ) {
		gang_size = 1;
	}
	/* Ask for the library to update its info */
    rc = repopulate_syscfg(&h_d);
    if( rc != 0) {
        while(count < 10){
			sleep(1);
            rc = repopulate_syscfg(&h_d);
            if(!rc){
                break;
            }
            else {
                count++;
            }
        }
        if(count == 10){
				sprintf(msg,"[%d] Repopulation of syscfg unsuccessful. Exiting\n",__LINE__);
				hxfmsg(&h_d, rc, HTX_HE_SOFT_ERROR, msg);
				exit_flag = ERR_EXIT;
				return (FAILURE);
		}
    }


	/* Get smt status & threads  */
#if defined(__HTX_LINUX__) && defined(AWAN)
	system_information.smt_threads = 4;
#else
	get_smt_details(&system_smt_information);

	/* Check if smt value returned is valid. If not then exit. */
	if(system_smt_information.smt_threads >= 1) {
		system_information.smt_threads = system_smt_information.smt_threads;
	}
	else {
		sprintf(msg,"Incorrect value of smt_threads obtained : %d, Exiting\n",system_smt_information.smt_threads);
		hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
		return (FAILURE);
	}
#endif

	/* Get pvr information */
#if defined(__HTX_LINUX__) && defined(AWAN)
	system_information.pvr = POWER8_MURANO;	/* Hardcode p8 value for AWAN */
#elif defined(__HTX_LINUX__) && !defined(AWAN)
	/*system_information.pvr = get_pvr();*/
	system_information.pvr=get_cpu_version();
	system_information.pvr = (system_information.pvr)>>16 ;
#else
	system_information.pvr = (unsigned int ) getPvr();
	system_information.pvr = (system_information.pvr)>>16 ;
#endif

	/*
	 * Collect cache releated information like:
	 * Total L2/L3 cache size, L2/L3 line size, L2/L3 associativity
	 * and fill cache_info data structure for L2 and L3
	 */
	find_cache_details();

	/* Get the number of logical cpu's */
#if defined(__HTX_LINUX__) && defined(AWAN)
	system_information.number_of_logical_cpus = 8;
#else
	phy_logical_virt_cpus(&system_cpu_information);
	system_information.number_of_logical_cpus = system_cpu_information.l_cpus;
#endif


	/* Find the instance of cache running 	*/
	/* One Cache exerciser is created for	*/
	/* each chip in the system.				*/
	/* 0th instance runs on chip 0			*/
	/* 1st instance runs on chip 1			*/
	/* ...									*/
	/* nth instance runs on chip n			*/
	/* Hence instance number of cache exer	*/
	/* informs us the chip number where it 	*/
	/* is currently executing.				*/

	instance_number 					= get_device_instance(h_d.sdev_id);
	system_information.instance_number	= instance_number;										/* Populate the instance number								*/
	
	rc = get_hardware_stat( &sys_stat );														/* Get the hardware statistics								*/
	if (rc == -1) {
		sprintf(msg,"[%d]Error: syscfg library returned -1. Exitting !!",__LINE__);
		hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
		return (FAILURE);
	}

	get_cpu_map(M_CHIP, &cpu_map_for_instance, system_information.instance_number );

	memcpy(&system_information.cpu_in_chip_map,&cpu_map_for_instance,sizeof(CPU_MAP));					/* Populate the CPU map for this instance						*/ 

	total_cpus	= get_cpus_in_chip(system_information.instance_number,system_information.cpus_in_instance);	  								/* Get the number of CPUs and the actual CPUs in this instance 	*/
	memcpy(&system_information.sys_stat , &sys_stat , sizeof(sys_stat));							 	/* Populate the hardware statistics structure					*/

	num_cores	= get_core_info( system_information.cores_in_instance, system_information.instance_number );
	/*sprintf(msg,"[%d]#####total_cpus = %d\n",__LINE__,total_cpus);
	hxfmsg(&h_d,0,HTX_HE_INFO, msg);*/


	system_information.num_cores				= num_cores;	/* Number of cores in this instance.	*/
	system_information.number_of_logical_cpus 	= total_cpus;	/* Number of CPUs in this instance.		*/

	if(gang_size > 1) {
		system_information.start_cpu_number = instance_number * total_cpus;
		system_information.end_cpu_number   = system_information.start_cpu_number + total_cpus - 1;
		if (total_cpus == 0 ) {
			cleanup_done = 0;
			rc = memory_set_cleanup();
			if ( rc != SUCCESS ) {
				sprintf(msg,"[%d]: Error while releasing memory. Exitting !!\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return ( rc);
			}
			#ifdef __HTX_LINUX__
				hxfupdate(RECONFIG, &h_d);
			#endif
				sprintf(msg,"[%d]:No cpus in this instance, thus exiting... \n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
				exit(0);
		}
	}
	else { /* If gang size = 1, For Equaliser support */
		system_information.start_cpu_number = instance_number;
		system_information.end_cpu_number   = instance_number;
		system_information.num_cores = 1;
		system_information.number_of_logical_cpus = 1;
		total_cpus = 1;
	}

	/* Now store the SMT values of all the cores, and CPUs in each core */
	for (i=0 ; i<system_information.num_cores; i++) {
		int smt, ret, core,j;
		core = system_information.cores_in_instance[i];
		smt = get_smt_status(core);
		system_information.core_smt_array[i] = smt;
		DEBUG_LOG("/n system_information.cpus_in_core_array[%d] =",i);
		for (j=0 ; j<smt; j++) {
			ret = get_cpus_in_core(core,system_information.cpus_in_core_array[i]);
			DEBUG_LOG("%d  ",system_information.cpus_in_core_array[i][j]);
		}
#ifdef __HTX_MAMBO__
		DEBUG_LOG("[%d] SMT of core number %d = %d\n",__LINE__,core, smt);
#endif
	}

	return (rc);
}

int create_and_dispatch_threads(int current_test_case) {
	int rc 				= SUCCESS;
	int	current_tc_type	= current_rule->testcase_type;  
	int num_prefetch_threads = current_rule->num_prefetch_threads_to_create;


	/* Create Cache threads here, if required
	 * Only if cache testcase is disabled (PREFETCH_ONLY) then we skip creation
	 * of cache threads.
	 */

	if( current_tc_type != PREFETCH_ONLY ) {
		if ( (rc = setup_thread_context_array(CACHE)) != SUCCESS) {
			sprintf(msg,"[%d] Error in setting up Cache threads. Exitting !!",__LINE__);
			hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
			return (rc);
		}

		/* If create threads fails, exerciser cannot continue, therefore exit */
		if( (rc = create_threads(CACHE)) != SUCCESS) {
			sprintf(msg,"[%d] Error in creating Cache threads. Exitting !!",__LINE__);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return (rc);
		}
	}
	/* Setup thread context for prefetch threads if required */
	if ( num_prefetch_threads > 0 ) {
		if ( (rc = setup_thread_context_array(PREFETCH)) != SUCCESS ) {
			sprintf(msg,"[%d] Error in setting up Prefetch threads . Exitting !!",__LINE__);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return (rc);
		}

		/* If create threads fails, exerciser cannot continue, therefore exit */
		if( (rc = create_threads(PREFETCH)) != SUCCESS) {
			sprintf(msg,"[%d] Error in creating Prefetch threads. Exitting !!",__LINE__);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return (rc);
		}
	}
#if defined(RUNLOG_SCREEN) || defined(RUNLOG)
	print_once_thread_data(current_rule);
#endif

	return (rc);
}

int do_thread_cleanup(int thread_type) {
	int rc = SUCCESS;

	switch( thread_type ) {
		case CACHE:
		case PREFETCH:
			if ( (rc = cleanup_threads(thread_type)) != SUCCESS ) {
				if ( thread_type == CACHE) {
					sprintf(msg,"[%d] Error in cleaning Cache threads (rc = %d). Exitting !!",__LINE__,rc);
				} else if ( thread_type == PREFETCH ) {
					sprintf(msg,"[%d] Error in cleaning Prefetch threads. Exitting !!",__LINE__);
				}
				hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
				return (rc);
			}
			if ( (rc = cleanup_thread_context_array(thread_type)) != SUCCESS ) {
				if ( thread_type == CACHE) {
					sprintf(msg,"[%d] Error in cleaning Cache threads context. Exitting !!",__LINE__);
				} else if ( thread_type == PREFETCH ) {
					sprintf(msg,"[%d] Error in cleaning Prefetch threads context. Exitting !!",__LINE__);
				}
				hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
				return (rc);
			}
			break;
		case ALL:
			if (current_rule->num_cache_threads_created != 0) {
				if( (rc = do_thread_cleanup(CACHE)) != SUCCESS) {
					sprintf(msg,"[%d] Error in cleaning Cache threads. Exitting !!",__LINE__);
					hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
					return (rc);
				}
			}
			if (current_rule->num_prefetch_threads_created != 0) {
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
				if( (rc = do_thread_cleanup(PREFETCH)) != SUCCESS) {
					sprintf(msg,"[%d] Error in cleaning Prefetch threads. Exitting !!",__LINE__);
					hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
					return (rc);
				}
			}
			break;
		default:
			sprintf(msg,"[%d] Unknown Thread type !!",__LINE__);
			hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
			rc = E_UNKNWON_THR_TYPE;
			break;
	}

	return (rc);
}

int wait_for_threads_termination(int current_test_case) {
	int	rc 					= SUCCESS;								/* Return value 							*/
	int	current_tc_type		= current_rule->testcase_type;  		/* Current test case type					*/
	int	current_pf_conf 	= current_rule->pf_conf;				/* current stanza Prefetch configuration	*/

	/* If Cache testcase is disabled ,then there is no need to clean up cache threads
	* as they were not created in this rule stanza pass.
	*/
	if( current_tc_type != PREFETCH_ONLY ) {
		/* Cleanup Cache threads here */
		PRINT_LOG("\n[%d] cleaning up cache threads\n",__LINE__);
		if( (rc = do_thread_cleanup(CACHE)) != SUCCESS) {
			sprintf(msg,"[%d] Error in cleaning Cache threads. Exitting !!",__LINE__);
			hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
			return (rc);
		}
	}

	/* Cleanup Prefetch threads here if required		*/
	/* If prefetch threads are currently running 		*/
	/* Prefetch threads are cleaned up iff				*/
	/* 1. it is the last test case stanza				*/
	/* OR												*/
	/* 2. It is not the last stanza but the next		*/
	/*    stanza has a different prefetch configuration	*/
	/*    OR next stanza is different test case			*/

	if(current_pf_conf != PREFETCH_OFF ) {
		/* If condition for Prefetch cleanup holds true, then cleanup */
		DEBUG_LOG("[%d] cleaning up prefetch threads\n",__LINE__);

		if ( (rc = do_thread_cleanup(PREFETCH)) != SUCCESS ) {
			sprintf(msg,"[%d] Error in cleaning Prefetch threads. Exitting !!",__LINE__);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return (rc);
		}
	}

	current_rule->num_cache_threads_created = 0;
	current_rule->num_prefetch_threads_created = 0;

	return (rc);
}

int get_shared_memory(int shm_size, uint32_t memflg, int page_size) {
	int 				rc = SUCCESS, i=0, j;
  	unsigned char 		*p, *addr; 
	int 				seg;
	struct shmid_ds		shm_buf;
#ifdef SET_PAGESIZE
	struct vminfo_psize	vminfo_64k	= { 0 };
	psize_t			 	psize	  	= page_size/*huge_page_size*/;
#else
	unsigned long long 	psize 		= page_size/*huge_page_size*/;
#endif

	seg = (page_size == huge_page_size) ? 0 : 1;
	memset(&shm_buf,0,sizeof(struct shmid_ds));

	mem.shm_id[seg] = shmget(IPC_PRIVATE,shm_size, memflg);     /* hotplug changes:multiple instances were resulting to same key after going DT*/
		if(mem.shm_id[seg] == -1) {
			sprintf(msg,"[%d] Error: shmget failed with %d while allocating seg size = %u!\n",__LINE__, errno,shm_size);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return(E_NOT_ENOUGH_MEM);
		}

#if !defined(SET_PAGESIZE) && defined(__HTX_LINUX__)
		/* Using SHM_LOCK (Linux Specific) flag to lock the shm area into memory */
		if ((rc = shmctl(mem.shm_id[seg], SHM_LOCK, &shm_buf)) == -1)   {
			sprintf(msg,"[%d] Error: shmctl failed to get minimum alignment requirement - %d\n",__LINE__, errno);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return(E_SHMCTL_FAIL);
		}
#endif

#if defined(SET_PAGESIZE) && !defined(__HTX_LINUX__)
		if ( shm_size > 256*M) {
			if ((rc = shmctl(mem.shm_id[seg], SHM_GETLBA, &shm_buf)) == -1)   {
				sprintf(msg,"[%d] Error: shmctl failed to get minimum alignment requirement - %d\n", __LINE__,errno);
				hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
				return(E_SHMCTL_FAIL);
			}
		}
		shm_buf.shm_pagesize = psize;
		if((rc = shmctl(mem.shm_id[seg], SHM_PAGESIZE, &shm_buf)) == -1) {
			sprintf(msg,"[%d] Error : shmctl failed with %d while setting page size.\n",__LINE__,errno);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return(E_SHMCTL_FAIL);
		}
#endif
		addr = (unsigned char *) shmat(mem.shm_id[seg], 0, 0);
		if((unsigned char *)-1 == addr) {
			sprintf(msg,"[%d] shmat failed with %d\n",__LINE__,errno);
			hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
			return(E_SHMAT_FAIL);
		}
		mem.seg_addr[seg] = addr;

#ifdef SET_PAGESIZE
		rc = mlock(addr, shm_size);
		if(rc == -1) {
			sprintf(msg,"[%d] mlock failed with %d \n", __LINE__,errno);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return (E_MLOCK_FAIL);
		}
#endif
		/* Size of the total memory set, i.e size of memory requested from shmget() */
	mem.memory_set_size[seg] = shm_size;

	if ( page_size == huge_page_size ) {
		for(p=addr; p < addr+shm_size; p+=huge_page_size) {
			mem.ea[i] = p;
            *p=0xBBBBBBBB;
#ifdef __HTX_LINUX__
            find_EA_to_RA(mem.ea[i],&mem.real_addr[i]);
#else
			getRealAddress(p, 0, &mem.real_addr[i]);
#endif
			i++;
			mem.num_pages = i;
			mem.page_status[i] = PAGE_FREE;
		}
	} else {
		mem.prefetch_4k_memory = mem.seg_addr[seg];
	}

	DEBUG_LOG("\n[%d] mem.num_pages = %d",__LINE__,mem.num_pages);

	for (j=0 ; j< mem.num_pages ; j++) {
			DEBUG_LOG("\n[%d] Real address = 0x%llx , Effective Address = %p",__LINE__,mem.real_addr[j],mem.ea[j]);
	}

	return (rc);
}

#if 0
int get_cont_phy_mem(unsigned long long size_requested) {
	int					i=0, rc = SUCCESS, j, k,ret;
	char				shmval[50];
	FILE				*fp = 0,*testfp=0;
	char				*addr;
	key_t				key;
	uint32_t			memflg;
	unsigned long long	current_shmmax=0,new_shmmax = 0;
	unsigned long long	shm_size;

	/* Only ONE 16M page required in case we're running on P7 */
	/* Set shm_size based on whether P6 or P7 or P7+ */
	if(size_requested < huge_page_size) {
		/* If required size is < one 16M page ,then get one 16M page*/
		shm_size = huge_page_size;
	}
	else {
		/* Calculate required number of 16M pages to span this memory requirement
		 * and request for that much memory
		 */
		shm_size =  size_requested;

		if (size_requested%(huge_page_size) != 0) {
			shm_size += ( (huge_page_size) - (size_requested%(huge_page_size)) ) ;  /* round off to the next highest 16M page */
		}
	}

	if ((system_information.pvr == POWER7 ) || ( (system_information.pvr == POWER7P) && (test_L2 == TRUE) )) {
		/* If P7 or P7+ L2 test case  then contiguous memory requirements falls within 16M page boundaries. Therefore
		 * here we don't need the entire "size" to be truly contiguous (in physical memory), only that
		 * it be allocated in distinct 16M pages, so that threads operating on each physical cache
		 * instance can operate within the page boundary assigned to them.
		 */

		/* Set index number for start of exclusive prefetch memory */
		start_of_prefetch_memory_index = 1;
		/* Mark pages as used */
		for (k=0; k < start_of_prefetch_memory_index; k++)
			mem.page_status[k] = CACHE_PAGE;
	}
	else {
		/* If P6, then contiguous memory requirements are beyond one 16M page boundaries. Therefore
		 * here we require "size" to be truly contiguous (in physical memory). Therefore we allocate
		 * a large enough segment (SEG_SIZE), within which a contiguous block of "size" can be found.
		 */

		/* Set index number for start of exclusive prefetch memory */
		start_of_prefetch_memory_index = 0;
	}

	memflg = (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

#if defined(__HTX_LINUX__)
	memflg |= (SHM_HUGETLB);

	/* Check if SHMMAX value configured is less than what we need, then set it as per our requirement */
	rc = set_shmmax_val(shm_size);

	if ( rc != SUCCESS ) {
		sprintf(msg,"[%d] Error in setting SHMMAX value \n",__LINE__);
		hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
		return (rc);
	}
#endif

	/* Now allocate the shared memory of required size */
	if ( (rc = get_shared_memory(shm_size,memflg)) != SUCCESS ) {
		sprintf(msg,"[%d] Error in allocating memory \n",__LINE__);
		hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
		return (rc);
	}

	/* Now setup the memory address array in contiguous order and find required contiguous pages in it */
	if( (rc = setup_contiguous_memory()) != SUCCESS) {
		sprintf(msg,"[%d] Error in getting required contiguous memory \n",__LINE__);
		hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
		return (rc);
	}

	return(rc);
}
#endif

int setup_memory_to_use(void) {
	int 				rc						= SUCCESS;
	struct ruleinfo 	*rule 					= &h_r[0];
	int					test_case 				= 0;
	int					contig_mem 				= 0;
	int					thread_num				= 0;
	struct mempool_t	*memory_pool			= NULL;
	int					mem_set					= 0;
	int 				found_cache_page		= 0;
	unsigned char		*found_prefetch_page	= NULL;
	int					j;

 	/* It will sort the memory set in physically contiguous order.	*/
	setup_memory_set();

	for (j=0 ; j< mem.num_pages ; j++) {
		DEBUG_LOG("\n[%d] Real address = 0x%llx , Effective Address = %p",__LINE__,mem.real_addr[j],mem.ea[j]);
	}

	for ( test_case = 0 ; test_case < num_testcases ; test_case++ ) {
		memory_pool = &(rule->cont_memory_pool);
		contig_mem = memory_pool->contiguous_mem_required;

		DEBUG_LOG("[%d] Contiguous memory sets = %d\n",__LINE__,rule->cont_memory_pool.num_sets);
		/* Setup contiguous page for cache threads	*/
		if ( rule->use_contiguous_pages == FALSE ) {
			sprintf(msg,"[%d] Warning: Could not find sufficient contiguous HUGE pages. Will be using non contiguous pages for rule: %s ( rule no %d ) \n",__LINE__, &(rule->rule_id[0]), test_case+1);
			hxfmsg(&h_d,0,HTX_HE_INFO,msg);
		}

		for ( mem_set = 0 ; mem_set < rule->cont_memory_pool.num_sets ; mem_set++ ) {
			found_cache_page= find_reqd_cache_mem(contig_mem, rule, mem_set);													/* Find the starting address of the required contiguous memory.			*/
			if (found_cache_page == -1) {																							/* If no such address is found, then skip this test case.				*/
				rule->use_contiguous_pages = FALSE;																				/* non contiguous pages may be used instead.							*/
				rc = E_NOT_ENOUGH_CONT_PAGES;
				return (rc);
			}
		}	/* End of num_mem_sets for loop */

		/* Now setup prefetch pages	if contiguous memory for cache has been allocated. If not then skip. */
			for ( thread_num = 0 ; thread_num < rule->num_prefetch_threads_to_create; thread_num++ ) {
			if ( system_information.pvr >= POWER8_MURANO ) {
				found_prefetch_page = find_prefetch_mem_p8(thread_num);																	
			} else {
				found_prefetch_page = find_prefetch_mem(rule);																			/* Find a free page which can be allocated to the prefetch thread.		*/
			}

			DEBUG_LOG("\n[%d] Found page for prefetch @ %p",__LINE__,found_prefetch_page);
				if (found_prefetch_page == NULL) {
				rc = E_NOT_ENOUGH_FREE_PAGES;
				return (rc);
				} else {
					memory_pool->prefetch_mem_set[thread_num] = found_prefetch_page;												/* If free page is found then put the address in the prefetch mem set.	*/
				}
			}		/* End of thread_num for loop 	*/

		rule++;		/* Proceed to the next rule	 	*/

		/* Reset some values before proceeding.	*/
		found_cache_page 	= 0;
		found_prefetch_page = NULL;

	}				/* End of test_case for loop 	*/

	return (rc);

}

unsigned int get_max_cache_mem_req() {

	int 			test_case 				= 0;
	struct ruleinfo *rule 					= &h_r[0];
	int				stanza_page_requirement = 0;

	for ( test_case = 0 ; test_case < num_testcases ; test_case++ ) {

		if ( stanza_page_requirement < rule->cache_HUGE_pages_required ) {
			stanza_page_requirement = rule->cache_HUGE_pages_required;
		}
		rule++;
	}

	return (stanza_page_requirement * huge_page_size) ;
}

unsigned int get_max_prefetch_mem_req() {
	int 			test_case 				= 0;
	struct ruleinfo *rule 					= &h_r[0];
	int				stanza_page_requirement = 0;

	for ( test_case = 0 ; test_case < num_testcases ; test_case++ ) {

			if ( stanza_page_requirement < rule->prefetch_HUGE_pages_required ) {
				stanza_page_requirement = rule->prefetch_HUGE_pages_required;
		}
		rule++;
	}
	return (stanza_page_requirement * huge_page_size) ;
}
unsigned int get_max_total_mem_req() {

	int 			test_case 				= 0;
	struct ruleinfo *rule 					= &h_r[0];
	int				stanza_page_requirement = 0;

	for ( test_case = 0 ; test_case < num_testcases ; test_case++ ) {

		if ( stanza_page_requirement < rule->total_HUGE_pages_required ) {
			stanza_page_requirement = rule->total_HUGE_pages_required;
		}

		rule++;
	}

	return (stanza_page_requirement * huge_page_size) ;
}
/* This function calculates the highest requirement among all the stanzas in the rulefile.		*/
/* If highest requirement comes out to be 0, then something is wrong and it flags FAILURE.		*/
/* While calculation, it skips the stanza which is marked to be skipped, as it is determined	*/
/* that there is not enough contiguous pages available to run this stanza.						*/
/* Input params : NONE																			*/
/* Output		: SUCCESS if everything is OK.													*/
/*				  FAILURE otherwise.															*/

int allocate_worst_case_memory(void) {
	int 				rc 						= SUCCESS;
	static unsigned int	worst_case_memory 		= 0;
	static unsigned int worst_case_prefetch_memory = 0;
	uint32_t			memflg, prefetch_memflg;
#ifndef __HTX_LINUX__
    int early_lru = 1; /* 1 extends local affinity via paging space.*/
    int num_policies = VM_NUM_POLICIES;
    int policies[VM_NUM_POLICIES] = {/* -1 = don't change policy */
    P_FIRST_TOUCH, /* VM_POLICY_TEXT */
    P_FIRST_TOUCH, /* VM_POLICY_STACK */
    P_FIRST_TOUCH, /* VM_POLICY_DATA */
    P_FIRST_TOUCH, /* VM_POLICY_SHM_NAMED */
    P_FIRST_TOUCH, /* VM_POLICY_SHM_ANON */
    P_FIRST_TOUCH, /* VM_POLICY_MAPPED_FILE */
    P_FIRST_TOUCH, /* VM_POLICY_UNMAPPED_FILE */
    };
#endif

	if ( system_information.pvr >= POWER8_MURANO ) {
		printf("[%d] Prefetch thread memory will be in 4KB pages and cache thread memory will be in HUGE pages \n",__LINE__);
		worst_case_memory = get_max_cache_mem_req();
		worst_case_prefetch_memory = get_max_prefetch_mem_req();
		prefetch_memflg = (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	} else {
		PRINT_LOG("[%d] Prefetch and cache thread mem will be in HUGE pages.\n",__LINE__);
		worst_case_memory = get_max_total_mem_req();
		PRINT_LOG("[%d] Total memory required = %d MB , cache memory = %d MB and prefetch memory = %d MB\n",
				__LINE__,(worst_case_memory / (M)), (get_max_cache_mem_req()/(M)), (get_max_prefetch_mem_req()/(M)) );
	}

	DEBUG_LOG("[%d] Worst case memory required = %d",__LINE__,worst_case_memory);
	/*worst_case_memory = get_max_total_mem_req();*/
	if ( worst_case_memory == 0 ) {
		sprintf(msg,"[%d] Error: Worst case memory required is 0. Exitting !!\n",__LINE__);
		hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
		rc = FAILURE;
	}

	if(worst_case_memory < huge_page_size) {
		shm_size = huge_page_size;
	}
	else {
		shm_size = worst_case_memory;

		if (worst_case_memory%(huge_page_size) != 0) {
			shm_size += ( (huge_page_size) - (worst_case_memory%(huge_page_size)) ) ;  /* round off to the next highest HUGE page */
		}
	}

	memflg = (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

#if defined(__HTX_LINUX__)
	memflg |= (SHM_HUGETLB);

	/* Check if SHMMAX value configured is less than what we need, then set it as per our requirement */
	if ( shm_size >= worst_case_prefetch_memory ) {
	rc = set_shmmax_val(shm_size);
	} else {
		rc = set_shmmax_val(worst_case_prefetch_memory);
	}

	if ( rc != SUCCESS ) {
		sprintf(msg,"[%d] Error: Unable to set SHMMAX value \n",__LINE__);
		hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
		return (rc);
	}
#endif
#if defined(__HTX_LINUX__)
    htx_unbind_thread();
    rc = htx_bind_thread(system_information.cpus_in_instance[0],system_information.cpus_in_instance[0]);
#else
    bindprocessor(BINDPROCESS, getpid(), PROCESSOR_CLASS_ANY);
	
    /* To get local memory set flag early_lru=1 to select P_FIRST_TOUCH policy(similar to setting MEMORY_AFFINITY environment variable to MCM)*/
    rc = vm_mem_policy(VM_SET_POLICY,&early_lru, &policies ,num_policies);
    if (rc != 0){
        sprintf(msg,"vm_mem_policy() call failed with return value = %d\n",rc);
		hxfmsg(&h_d, rc, HTX_HE_SOFT_ERROR, msg);
    }
    rc = bindprocessor(BINDPROCESS, getpid(), system_information.cpus_in_instance[0]);
#endif
    if(rc < 0){
		sprintf(msg, "Binding to cpu:%d  failed with rc= %d ,Local memory may not be allocated to this instance\n",system_information.cpus_in_instance[0],rc);
        hxfmsg(&h_d,0, HTX_HE_INFO, msg);
	}
	/* Now allocate the shared memory of required size */
	if ( (rc = get_shared_memory(shm_size,memflg,huge_page_size)) != SUCCESS ) {
		sprintf(msg,"[%d] Error: Unable to allocate memory (ret val = %d)\n",__LINE__,rc);
		hxfmsg(&h_d, rc, HTX_HE_SOFT_ERROR, msg);
		return (rc);
	}
	if ( system_information.pvr >= POWER8_MURANO && worst_case_prefetch_memory != 0) {
		if ( (rc = get_shared_memory(worst_case_prefetch_memory,prefetch_memflg,PREF_PG_SIZE)) != SUCCESS ) {
			sprintf(msg,"[%d] Error: Unable to allocate memory for prefetch (ret val = %d)\n",__LINE__,rc);
			hxfmsg(&h_d, rc, HTX_HE_SOFT_ERROR, msg);
			return (rc);
		}
	}
#if defined(__HTX_LINUX__)
    rc =htx_unbind_thread();
#else
    rc = bindprocessor(BINDPROCESS, getpid(), PROCESSOR_CLASS_ANY);
#endif
	/* Now setup the memory address array in contiguous order and find required contiguous pages in it */
	rc = setup_memory_to_use();
	
	return (rc);
}

int get_memory_to_use(int test_case) {
	int rc	= SUCCESS;

	/* Will fill in later */	

	return (rc);
}

/* Helper functions definitions follow */

int register_signal_handlers(void) {
	int rc = SUCCESS;
	/*  Register SIGTERM handler */
	sigemptyset((&sigvector.sa_mask));
	sigvector.sa_flags   = 0;
	sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
	sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

	/*  Register DR handler */
#ifndef __HTX_LINUX__
	sigprocmask(SIG_UNBLOCK, NULL, &mask);
	sigaddset(&mask, SIGRECONFIG);
	sigprocmask (SIG_UNBLOCK, &mask , NULL);

	sigemptyset(&(sigvector_dr.sa_mask));
	sigvector_dr.sa_flags   = 0;
	sigvector_dr.sa_handler = (void (*)(int))DR_handler;
	dr_rc = sigaction(SIGRECONFIG, &sigvector_dr, (struct sigaction *) NULL);
	if (dr_rc != 0) {
		sprintf(msg,"sigaction failed(%d) for SIGRECONFIG\n",errno);
		hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
		return (FAILURE);
	}
	sigprocmask(SIG_UNBLOCK, NULL, &mask);

	if (sigismember(&mask , SIGRECONFIG) == 1) {
		sprintf(msg," SIGRECONFIG signal is blocked\n");
		hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
		return (FAILURE);
	}
#else
	    /* Register handler for SIGUSR2 for cpu hotplug add/remove */
	sigemptyset((&sigvector_usr2.sa_mask));
	/*sigfillset(&(sigvector_usr2.sa_mask));
    sigprocmask(SIG_UNBLOCK, &(sigvector_usr2.sa_mask), NULL);*/
	sigvector_usr2.sa_flags   = 0;
    sigvector_usr2.sa_handler = (void (*)(int)) SIGUSR2_hdl;
    sigaction(SIGUSR2, &sigvector_usr2, (struct sigaction *) NULL);
#endif

	return rc;
}

int do_initial_setup(char **arg_list) {
    int rc = SUCCESS;
    FILE* fp;
    char fname[64];

	/*  Parse command line arguments */
#if defined(__HTX_LINUX__) && defined(AWAN)
		strcpy(device_name, "/dev/cache0");
		strcpy(run_type, "OTH");
		strcpy(rule_file_name, "../rules/reg/hxecache/default.p8");
#else
		strcpy(device_name, arg_list[1]);
		strcpy(run_type, arg_list[2]);
		strcpy(rule_file_name, arg_list[3]);
#endif

	/*  Set the program behaviour in case of miscompare */
	htxkdblevel = getenv("HTXKDBLEVEL");
	if (htxkdblevel) {
		crash_on_misc_global = atoi(htxkdblevel);
	}
	else {
		crash_on_misc_global = 0;
	}

	/*  Set htx_data structure parameters */
#if defined(__HTX_LINUX__) && defined(AWAN)
	strcpy(h_d.HE_name, "./hxecache");
	strcpy(h_d.sdev_id, "/dev/cache0");
	strcpy(h_d.run_type, "OTH");
#else
	if(arg_list[0]) strcpy(h_d.HE_name, arg_list[0]);
	if(arg_list[1]) strcpy(h_d.sdev_id, arg_list[1]);
	if(arg_list[2]) strcpy(h_d.run_type, arg_list[2]);
#endif

	/* Indicate that the exerciser has started */
	hxfupdate(START, &h_d);

    huge_page_size = 16 * M; /* default to 16MB(in AIX)*/
#ifdef __HTX_LINUX__
    char temp[500];
    char *ret_ptr;
    huge_page_size = -1;
    sprintf(fname,"/proc/meminfo");
    if ((fp=fopen(fname,"r"))==NULL){
        sprintf(msg,"fopen of file %s failed with errno=%d",fname,errno);
        hxfmsg(&h_d,0, HTX_HE_INFO, msg);       
    }
    else {
        do
        {
            ret_ptr = fgets(temp, sizeof(temp), fp);
            sscanf(temp, "Hugepagesize: %d kB", &huge_page_size);
        } while ((ret_ptr != NULL) && (huge_page_size == -1));
        if ( (ret_ptr == NULL) || huge_page_size == -1 ) {
            sprintf(msg,"could not find Hugepagesize in file %s,errno =%d",fname,errno);
            hxfmsg(&h_d,0, HTX_HE_INFO, msg);
        }
        fclose(fp);
    }

    if(huge_page_size == 2048){
        sprintf(msg," hug pages of size 2MB will be used for cache opearions\n"); 
        hxfmsg(&h_d,0, HTX_HE_INFO, msg);
    }
    else if(huge_page_size == 16384){   
        sprintf(msg,"hug pages of size 16MB will be used for cache opearions\n");
        hxfmsg(&h_d,0, HTX_HE_INFO, msg);
    }
    else {
        sprintf(msg," huge_page_size = %d, unsupported huge page size is detected, exiting! \n", (huge_page_size * K));
        hxfmsg(&h_d,0, HTX_HE_HARD_ERROR, msg);
        exit(1);
    }
    huge_page_size = (huge_page_size * 1024);
#endif
    half_huge_page_size = (huge_page_size/2);
    if((!(strlen(h_d.htx_log_dir))) || (!(strlen(h_d.htx_exer_log_dir)))){
        sprintf(msg,"htx lib log direcories found to be htx_log_dir= %s and htx_exer_log_dir= %s, thus updating them to '/tmp/'\n",h_d.htx_log_dir,h_d.htx_exer_log_dir);
    	hxfmsg(&h_d,0, HTX_HE_INFO, msg);
        strcpy(h_d.htx_log_dir,"/tmp/");
        strcpy(h_d.htx_exer_log_dir,"/tmp/");

    }

	return (rc);
}

int dump_system_info(void){
	int rc 	= SUCCESS;
	int i	= 0;

	PRINT_LOG("\n\n[%d]#############################################################",__LINE__);
	PRINT_LOG("\n[%d] 				System information",__LINE__);
	PRINT_LOG("\n[%d]#############################################################",__LINE__);

	PRINT_LOG("\n[%d] PVR                       = 0x%x",__LINE__,system_information.pvr);
	PRINT_LOG("\n[%d] Logical CPUs              = %d",__LINE__,system_information.number_of_logical_cpus);
	PRINT_LOG("\n[%d] Start CPU number          = %d",__LINE__,system_information.start_cpu_number);
	PRINT_LOG("\n[%d] End CPU number            = %d",__LINE__,system_information.end_cpu_number);
	PRINT_LOG("\n[%d] Instance number           = %d",__LINE__,system_information.instance_number);
	PRINT_LOG("\n[%d] Number of cores           = %d",__LINE__,system_information.num_cores);
	PRINT_LOG("\n[%d] Cores in this instance	= ",__LINE__);
	for (i=0; i<system_information.num_cores; i++){
		PRINT_LOG("%d  ",system_information.cores_in_instance[i]);
	}

	return (rc);
}

int set_shmmax_val(unsigned long long shm_size) {
	int 				rc 	= SUCCESS;
	FILE 				*fp = NULL;
	char			 	shmval[50];
	unsigned long long	current_shmmax=0,new_shmmax = 0;
	
	fp = popen("cat /proc/sys/kernel/shmmax","r");
	if ( fp == NULL ) {
		sprintf(msg,"[%d] Failed opening /proc/sys/kernel/shmmax with %d\n ",__LINE__,errno);
		hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
		return (E_NO_FILE);
	} else {
		fscanf(fp,"%s",shmval);
		current_shmmax = strtoull(shmval,NULL,10);
		DEBUG_LOG("\n[%d]Info: Current SHMMAX val = %llu and shm_size = %llu\n",__LINE__,current_shmmax,shm_size);
		pclose(fp);

		if(current_shmmax < shm_size) {
			sprintf(msg,"[%d] Increasing SHMMAX value to %llu\n ",__LINE__,(shm_size+current_shmmax));
			hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
			fp = fopen("/proc/sys/kernel/shmmax","w");
			if ( fp == NULL ) {
				sprintf(msg,"[%d] Failed opening /proc/sys/kernel/shmmax with %d\n ",__LINE__,errno);
				hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
				return (E_NO_FILE);
			}
			else {
				new_shmmax = (current_shmmax + shm_size);
				fprintf(fp,"%llu",new_shmmax);
				fclose(fp);
			}
	  
			/* Verify SHM size has changed */ 
			fp = popen("cat /proc/sys/kernel/shmmax","r");
			if ( fp == NULL ) {
				sprintf(msg,"[%d] Failed opening /proc/sys/kernel/shmmax with %d\n ",__LINE__,errno);
				hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
				return (E_NO_FILE);
			} else {
				new_shmmax = 0;
				fscanf(fp,"%s",shmval);
				new_shmmax = strtoull(shmval,NULL,10);   
				sprintf(msg,"New SHMMAX val = %llu and shm_size = %llu\n",new_shmmax,shm_size);
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
				pclose(fp);
				if( new_shmmax != (shm_size+current_shmmax)) {
					sprintf(msg,"[%d] Failed to change SHMMAX value\n ",__LINE__);
					hxfmsg(&h_d, 1, HTX_HE_HARD_ERROR, msg);
					return (E_NO_SET_SHMMAX);
				}
			}
		}
	}

	return (rc);
}

int update_hxecache_stats(int current_test_case) {
	int 			rc 					= SUCCESS;
	int 			current_test 		= current_rule->tgt_cache;
	int				current_tc_type		= current_rule->testcase_type; 
	int				current_cache_sz	= system_information.cinfo[current_test].cache_size;
	int				current_line_sz		= system_information.cinfo[current_test].line_size;
	int 			data_width_in_bytes , num_writes_per_oper = 0 , num_reads_per_oper = 0;
	int				num_lines			= current_cache_sz/current_line_sz;
			
	/* Update HTX stats here. */
	if( current_rule->compare ) {

		/* Good reads and writes */
		h_d.good_reads  = current_rule->num_oper;
		h_d.good_writes = current_rule->num_oper;

		if(current_rule->data_width != RANDOM_WIDTH) {
			switch(current_rule->data_width ) {
				case BYTE_WIDTH:
					data_width_in_bytes = sizeof(char);
					break;

				case SHORT_WIDTH:
					data_width_in_bytes = sizeof(short);
					break;

				case INT_WIDTH:
					data_width_in_bytes = sizeof(int);
					break;

				case LONG_WIDTH:
					data_width_in_bytes = sizeof(long);
					break;

				default:
					sprintf(msg,"[%d] Unknown width of data chosen. Exitting\n ",__LINE__);
					hxfmsg(&h_d, 1, HTX_HE_HARD_ERROR, msg);
					return (E_UNKNOWN_DATA_WIDTH);
			}

			if ( current_tc_type == CACHE_BOUNCE_ONLY || current_tc_type == CACHE_BOUNCE_WITH_PREF ) {
				num_writes_per_oper = num_reads_per_oper = current_rule->gang_size *  2 * num_lines ;
			} else if ( current_tc_type == CACHE_ROLL_ONLY || current_tc_type == CACHE_ROLL_WITH_PREF ) {				
				num_writes_per_oper = num_reads_per_oper = (2 * num_lines );
			}

			h_d.bytes_writ = h_d.good_reads * num_writes_per_oper * data_width_in_bytes ;
			h_d.bytes_read = h_d.good_reads * num_reads_per_oper * data_width_in_bytes ;
		}
	}else {
		h_d.good_writes = current_rule->num_oper;									/* Good writes only */
	}

	return ( rc );
}

void SIGTERM_hdl(int sig) {
	if ( exit_flag == FALSE ) {
		exit_flag = NORMAL_EXIT ;
	}
}

#ifdef __HTX_LINUX__
void SIGUSR2_hdl(int sig, int code, struct sigcontext *scp)
{
	if(tot_thread_count != 0){
    	update_sys_detail_flag = 1;
	}
	sprintf(msg,"[%d] %s : Received SIGUSR2\n",__LINE__,__FUNCTION__);
	hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
}
#endif

int find_reqd_cache_mem(unsigned int mem_req, struct ruleinfo *rule, int memory_set) {
	int		  		i;
	int 			rc = 0;
	int				current_page_status ;

	DEBUG_LOG("\n[%d] Contiguous memory required = %d",__LINE__,mem_req);

	 if ( mem_req <= ((huge_page_size)/2) ) {

		for( i=0 ; i<mem.num_pages ; i++) {
			current_page_status = rule->mem_page_status[i];
	
			if( current_page_status == PAGE_FREE ) {
				rule->mem_page_status[i] = PAGE_HALF_FREE;
				DEBUG_LOG("\n[%d] returning page_number/total pages = %d/%d, status = %d address = %p\n",__LINE__,i+1,mem.num_pages, rule->mem_page_status[i], mem.ea[i]);
				rule->cont_memory_pool.cont_mem_set[memory_set][0] = mem.ea[i];
				return(0);
			} else if ( current_page_status == PAGE_HALF_FREE ) {
				rule->mem_page_status[i] = CACHE_PAGE;
				DEBUG_LOG("\n[%d] returning page_number/total pages = %d/%d, status = %d address = %p\n",__LINE__,i+1,mem.num_pages, rule->mem_page_status[i], mem.ea[i]);
				rule->cont_memory_pool.cont_mem_set[memory_set][0] = (mem.ea[i] + half_huge_page_size);
				return(0);
			}
		}

	} else if ( mem_req > ((huge_page_size)/2) && mem_req <= (huge_page_size)) {
		for (i=0 ; i<mem.num_pages ; i++) {
			current_page_status = rule->mem_page_status[i];
			if( current_page_status == PAGE_FREE ) {
				rule->mem_page_status[i] = CACHE_PAGE;
				DEBUG_LOG("\n[%d] returning page_number/total pages = %d/%d, status = %d address = %p\n",__LINE__,i+1,mem.num_pages, rule->mem_page_status[i], mem.ea[i]);
				rule->cont_memory_pool.cont_mem_set[memory_set][0] = mem.ea[i]; 
				return(0);
			}
		}

	} else if (mem_req > (huge_page_size)) {
		if ( rule->use_contiguous_pages == TRUE ) {
			rc = find_reqd_contig_pages( mem_req, rule, memory_set );
		} else if ( rule->use_contiguous_pages == FALSE ) {
			rc = find_reqd_pages( mem_req, rule, memory_set );
            if(rc == -1){
                sprintf(msg,"[%d] find_reqd_pages() failed with -1\n");
                hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
                exit(1);
		    }
		}
	}
	
	 return (rc);
}

int find_reqd_contig_pages( unsigned int mem_req, struct ruleinfo *rule, int memory_set ) {
		
	int				next_page_distance;
	int				current_page_status , next_page_status;
	int 			i, save_index = -1, contig_mem_size = huge_page_size, j, k;

	for(i = 0; i <= mem.num_pages; i++) {
		next_page_distance 	= (mem.real_addr[i + 1] - mem.real_addr[i]) * getpagesize() ;
		current_page_status	= rule->mem_page_status[i];
		next_page_status	= rule->mem_page_status[i+1];

		if (next_page_distance == huge_page_size && current_page_status == PAGE_FREE && next_page_status == PAGE_FREE)  {
			contig_mem_size += huge_page_size;
			if(contig_mem_size >= mem_req) {
				save_index = i;
				for(j = 0, k = save_index; k <= (i+1) ; j++, k++) {
					rule->mem_page_status[k]	= CACHE_PAGE;
					rule->cont_memory_pool.cont_mem_set[memory_set][j] = mem.ea[k];
				}

				for ( j = 0; j<rule->cache_HUGE_pages_required ;j++) {
					DEBUG_LOG("\n[%d] j = %d ea: %p page_status :%d",__LINE__,j,mem.ea[j],rule->mem_page_status[j]);
				}

				DEBUG_LOG("[%d] returning page address = %p\n",__LINE__,mem.ea[save_index]);
				return 0;
			}
		} else {
			contig_mem_size = huge_page_size;
			save_index	  = -1;
		}
	}
	return -1;
}

int find_reqd_pages( unsigned int mem_req, struct ruleinfo *rule, int memory_set ) {
	unsigned long	allocated_mem = 0;
	int 			pages_found = 0;
	int				i;

	for( i=0 ; i<=mem.num_pages ; i++ ) {
		if ( rule->mem_page_status[i] == PAGE_FREE ) {

			rule->mem_page_status[i] = CACHE_PAGE;
			rule->cont_memory_pool.cont_mem_set[memory_set][pages_found] = mem.ea[i];
			allocated_mem += huge_page_size ;
			pages_found++;

			if ( allocated_mem >= mem_req ) {
				return 0;
			}
		}
	}

	return -1;
}

int find_cache_details()
{

#if defined(__HTX_LINUX__) && defined(AWAN)													/* Hardcoding p7+ values for AWAN */

	/* Assign L2 cache information */
	system_information.cinfo[L2].line_size  = 128;
	system_information.cinfo[L2].asc		= 8;
	system_information.cinfo[L2].cache_size = 256*1024;

	/* Assign L3 cache information */
	system_information.cinfo[L3].line_size  = 128;
	system_information.cinfo[L3].asc		= 8;
	system_information.cinfo[L3].cache_size = 10*1024*1024;

#elif !defined(AWAN)

	htxsyscfg_cache_t cpu_cache_information;

	/* Fetch cache information */
#ifdef __HTX_LINUX__
		L2L3cache( &cpu_cache_information);
#else
		L2cache( &cpu_cache_information);
		L3cache( &cpu_cache_information);
#endif

	/* Assign L2 cache information */
	system_information.cinfo[L2].line_size  =  cpu_cache_information.L2_line;
	system_information.cinfo[L2].asc		=  cpu_cache_information.L2_asc;
	system_information.cinfo[L2].cache_size =  cpu_cache_information.L2_size;

	/* Assign L3 cache information */
	system_information.cinfo[L3].line_size  =  cpu_cache_information.L3_line;
	system_information.cinfo[L3].asc		=  cpu_cache_information.L3_asc;
	system_information.cinfo[L3].cache_size =  cpu_cache_information.L3_size;

#endif

	PRINT_LOG("\n CACHE Details:   L2        L3\
		       \n Line size        %-8d       %-8d\
		       \n Associativity    %-8d       %-8d\
		       \n Cache size       %-8x       %-8x\n"
			, system_information.cinfo[0].line_size, system_information.cinfo[1].line_size,
			system_information.cinfo[0].asc, system_information.cinfo[1].asc,
			system_information.cinfo[0].cache_size, system_information.cinfo[1].cache_size);

	return 0;
}

/* This function is meant to setup contiguous memory pointers for use by threads to
 * read/write to later on.
 * It does 2 things depending on the pvr:
 * 1. If P7 then this function will just setup the mem.contig_mem[] pointers to 16M
 * page boundaries.
 * 2. If P6/P7+ then it will sort the memory set in physically contiguous order.
 */
void setup_memory_set() {
	int					i, j, min_index = -1;
	unsigned char		*min_ea = mem.ea[0];
	unsigned long long	min, tmp;
	unsigned char		*tmp_ea;

	for(i = 0; i < (mem.num_pages-1) ; i++) {
		min = mem.real_addr[i];
		min_index = -1;
		for(j = i + 1; j < mem.num_pages; j++) {
			if(mem.real_addr[j] < min) {
				min	   = mem.real_addr[j];
				min_ea	= mem.ea[j];
				min_index = j;
			}
		}
		if(min_index != -1) {
			/* swap real_addr array */
			tmp					  		= mem.real_addr[i];															
			mem.real_addr[i]		 	= min;
			mem.real_addr[min_index] 	= tmp;

			/* swap effective addr array */
			tmp_ea			   			= mem.ea[i];										
			mem.ea[i]		 			= min_ea;
			mem.ea[min_index] 			= (unsigned char *)tmp_ea;
		}
	}
}


unsigned char* find_prefetch_mem(struct ruleinfo *rule) {
	/* mem.page_status array is initialised to zero.
	 * If allocated to cache   value  is 1
	 * If 8MB is allocated to prefetch value is 2
	 * If whole page is allocated value would be 3
	 */
	int	  			i;
	int	  			index		= 0;
	unsigned char	*ret		= NULL;
	
	DEBUG_LOG("[%d] num pages = %d\n",__LINE__,mem.num_pages);
	for (i = index; i < mem.num_pages; i++) {
		DEBUG_LOG("[%d] page no = %d, status = %d\n",__LINE__,i,rule->mem_page_status[i]);
		switch ( rule->mem_page_status[i])
		{
			case PAGE_FREE: 
				rule->mem_page_status[i] = PAGE_HALF_FREE;
				return mem.ea[i];

			case PAGE_HALF_FREE:
				rule->mem_page_status[i] = PREFETCH_PAGE;
				return (mem.ea[i] + PREFETCH_MEM_SZ);

			default:
#if 0
				sprintf(msg,"[%d] Unknown page status (%d). Exitting \n",__LINE__,rule->mem_page_status[i]);
				hxfmsg(&h_d, E_UNKNOWN_PAGE_STATUS, HTX_HE_HARD_ERROR, msg);
				return ret;
#endif
				break;
		}
	}

	/* No pages available then pass the address of the first prefetch page */
	/* can this happen */

	return ret;
}

unsigned char* find_prefetch_mem_p8(int thread_num) {

	unsigned char *ret = NULL;
	ret = (mem.prefetch_4k_memory + thread_num*PREFETCH_MEM_SZ);

	return ret;
}
int memory_set_cleanup() {
	int rc = SUCCESS, i;

	DEBUG_LOG("[%d] %s called \n",__LINE__,__FUNCTION__);

	if( cleanup_done == 1 ) {
		sprintf(msg,"[%d] %s : Cleanup already done. Exitting \n",__LINE__,__FUNCTION__);
		hxfmsg(&h_d, rc, HTX_HE_INFO, msg);
		return ( rc );
	}   

	for(i = 0; i < NUM_SEGS; i++) {
		if( NULL != mem.seg_addr[i]) {
			rc = shmdt(mem.seg_addr[i]);
			if(rc != 0) {
				sprintf(msg,"%s: shmdt failed with %d\n",__FUNCTION__, errno);
				hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
				return(E_SHM_DETACH_FAIL);
			}
            else {
                mem.seg_addr[i] = NULL;
            }


#ifdef __HTX_LINUX__
			rc = shmctl(mem.shm_id[i], SHM_UNLOCK, (struct shmid_ds *) NULL);
			if(rc != 0) {
				sprintf(msg,"%s: shmctl for SHM_UNLOCK failed with %d\n",__FUNCTION__, errno);
				hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
				return(E_SHM_UNLOCK_FAIL);
			}
#endif
			rc = shmctl(mem.shm_id[i], IPC_RMID, (struct shmid_ds *) NULL);
			if(rc != 0) {
				sprintf(msg,"%s: shmctl for IPC_RMID failed with %d\n",__FUNCTION__, errno);
				hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
				return(E_IPC_RMID_FAIL);
			}
            else {
                mem.shm_id[i] = -1;
            }

		}
	}
	cleanup_done = 1;
	return(rc);
}

void write_byte(void *addr, int index ,int tid)
{
	char *ptr	 = (char *)addr;
	char *pat_ptr = (char *)&(th_array[index].pattern);

	*ptr = *(char *)pat_ptr;

}

void write_short(void *addr, int index ,int tid)
{
	char		   *pat_ptr = (char *)&th_array[index].pattern;
	unsigned short *ptr	 = (unsigned short *)addr;

	*ptr = *(unsigned short *)pat_ptr;

}

void write_int(void *addr,int index , int tid)
{
	char		 *pat_ptr = (char *)&th_array[index].pattern;
	unsigned int *ptr	 = (unsigned int *)addr;

	*ptr = *(unsigned int *)pat_ptr;

}

void write_ull(void *addr,int index , int tid)
{
	char			   *pat_ptr = (char *)&th_array[index].pattern;
	unsigned long long *ptr	 = (unsigned long long *)addr;

	*ptr = *(unsigned long long *)pat_ptr;

}

int write_mem(int index , int tid) {
	int				walk_class, walk_line, memory_per_set, lines_per_set, ct, current_line_size;
	int				data_width;
	char			*addr = NULL;
	unsigned int	offset1, offset2, pvr;
	int 			rc = SUCCESS;
	int 			current_asc;
	int				offset_sum;

	/*
	 * cache_type decides whether calling write_mem for L2 or L3.
	 */

	ct					= current_rule->tgt_cache;
	pvr					= system_information.pvr;
	current_line_size 	= system_information.cinfo[ct].line_size;
	current_asc			= system_information.cinfo[ct].asc;
	data_width	 		= current_rule->data_width;
	memory_per_set 		= system_information.cinfo[ct].cache_size/system_information.cinfo[ct].asc;
	lines_per_set  		= memory_per_set/system_information.cinfo[ct].line_size;

	if (current_rule->target_set != -1) {
		if(current_rule->target_set > (( 2 * current_asc ) - 1)) {
			sprintf(msg,"Specified target class-%d is out of range-%d \n", current_rule->target_set,(( 2 * current_asc) - 1));
			hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
			sprintf(msg, "limitting class to %d \n", (( 2 * current_asc ) - 1));
			hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
			th_array[index].end_class = ( 2 * current_asc) - 1;
		}
		else {
			th_array[index].start_class = th_array[index].end_class = current_rule->target_set;
		}
	}

	/*
	 * If data_width == 4 then it will randomize the type of store (i.e. byte,
	 * short, int or long long.
	 */
	if(data_width == RANDOM_WIDTH) data_width = (get_random_number(index)) % RANDOM_WIDTH;

	/*
	 * The external loop targets line in set.
	 * Offset1 decides the line in set.
	 */
	for(walk_line = 0; walk_line < lines_per_set; walk_line++) {

		offset1 = walk_line*current_line_size + th_array[index].offset_within_cache_line;

		/*
		 * The inner loop here targets specific class and writes onto it twice the size
		 * so the class rolls over. offset2 jumps by set.
		 */

		for(walk_class = th_array[index].start_class;
			walk_class <= th_array[index].end_class;
			walk_class += th_array[index].walk_class_jump) {


			offset2	= walk_class*memory_per_set;
			offset_sum 		= offset1 + offset2;
#if 0

			if(( pvr == POWER7P ) && (current_rule->tgt_cache == L3)) {
			} else /*if (( pvr == POWER7P ) || ( pvr == POWER7))*/ {
				/* Calculate address to write to */
				addr = th_array[index].start_of_contiguous_memory + offset_sum;
			}
#endif
			addr = th_array[index].contig_mem[offset_sum/(huge_page_size)] + (offset_sum%(huge_page_size));

			switch(data_width) {
				case BYTE_WIDTH:
					write_byte(addr, index, tid);
					break;

				case SHORT_WIDTH:
					write_short(addr, index, tid);
					break;

				case INT_WIDTH:
					write_int(addr, index, tid);
					break;

				case LONG_WIDTH:
					write_ull(addr, index, tid);
					break;

				default:
					sprintf(msg, "[%d] Unknown Data width ( %d ). Exitting\n",__LINE__,data_width);
					hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
					return ( E_UNKNOWN_DATA_WIDTH );
			}
		}
	}

	/*
	 * Write_mem should have written over entire cache twice and hence would have
	 * rolled over the cache.
	 */
	return(rc);
}

int read_byte(void *addr, int index,  int tid)
{
	char *mem_addr_ptr		= (char *)addr;
	char *pattern_ptr 		= (char *) &th_array[index].pattern;
	char *thread_context	= (char *)&th_array[index];
	int  rc					= SUCCESS;

	if( *(char *)pattern_ptr != *mem_addr_ptr ) {

		if(crash_on_misc_global == TRUE && current_rule->crash_on_misc == TRUE) {
#ifdef __HTX_LINUX__
			do_trap_htx64((unsigned long)0xBEEFDEAD, (unsigned long) addr, (unsigned long) pattern_ptr,
						  (unsigned long) &mem, (unsigned long) &system_information , (unsigned long) index,
						  (unsigned long) thread_context, (unsigned long)current_rule);
#else
			trap(0xBEEFDEAD, addr, pattern_ptr, &mem,&system_information , index,
				 thread_context, current_rule);
#endif
		}

		sprintf(msg,"[%d] RB:TID:%d miscompare!!EA = %p data = 0x%x pattern = 0x%x thread_index : %d rule id = %d\n",__LINE__,tid,addr, \
				*((char*)addr) ,*((char*)pattern_ptr) , index, current_rule->testcase_type);
		hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);

		rc = dump_miscompare_data(index,addr);
		exit(E_MISCOMPARE);
	}
	else {
		return(rc);
	}
}

int read_short(void *addr,int index , int tid)
{
	char		  	*pattern_ptr	= (char *) &th_array[index].pattern;
	unsigned short	*mem_addr_ptr	= (unsigned short *)addr;
	char		  	*thread_context	= (char *)&th_array[index];
	int				current_tc_type	= current_rule->testcase_type;
	int				rc				= SUCCESS;

	if( *(unsigned short *)pattern_ptr != *mem_addr_ptr) {

		if(crash_on_misc_global == TRUE && current_rule->crash_on_misc == TRUE) {
#ifdef __HTX_LINUX__
			do_trap_htx64((unsigned long)0xBEEFDEAD, (unsigned long) addr, (unsigned long) pattern_ptr,
						  (unsigned long) &mem, (unsigned long) &system_information , (unsigned long) index,
						  (unsigned long) thread_context, (unsigned long)current_rule);
#else
			trap(0xBEEFDEAD, addr, pattern_ptr, &mem,&system_information , index,
				 thread_context, current_rule);
#endif
		}

		sprintf(msg,"[%d] RS: TID: %d  miscompare !! EA = %p data = 0x%x pattern = 0x%x thread index : %d rule id = %d\n",__LINE__, tid,\
				addr, *((unsigned short *)addr), *((unsigned short *)pattern_ptr) ,index, current_tc_type);
		hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);

		rc = dump_miscompare_data(index,addr);
		exit(E_MISCOMPARE);
	}
	else {
		return(rc);
	}
}

int read_int(void *addr, int index ,int tid) {
	char		  	*pattern_ptr	= (char *) &th_array[index].pattern;
	unsigned int	*mem_addr_ptr	= (unsigned int *)addr;
	char		  	*thread_context	= (char *)&th_array[index];
	int				current_tc_type	= current_rule->testcase_type;
	int				rc				= SUCCESS;

	if( *(unsigned int *)pattern_ptr != *mem_addr_ptr) {

		if(crash_on_misc_global == TRUE && current_rule->crash_on_misc == TRUE) {
#ifdef __HTX_LINUX__
			do_trap_htx64((unsigned long)0xBEEFDEAD, (unsigned long) addr, (unsigned long) pattern_ptr,
						  (unsigned long) &mem, (unsigned long) &system_information , (unsigned long) index,
						  (unsigned long) thread_context, (unsigned long)current_rule);
#else
			trap(0xBEEFDEAD, addr, pattern_ptr, &mem,&system_information , index,
				 thread_context, current_rule);
#endif
		}

		sprintf(msg,"[%d] RS: TID: %d  miscompare !! EA = %p data = %x pattern = %x thread index : %d test case type = %d\n",__LINE__, tid,\
				addr, *((unsigned int *)addr), *((unsigned int *)pattern_ptr) ,index, current_tc_type);
		hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);

		rc = dump_miscompare_data(index,addr);
		exit(E_MISCOMPARE);

	}
	else {
		return(rc);
	}
	return (rc);
}

int read_ull(void *addr,int index , int tid) {
	char		  		*pattern_ptr	= (char *) &th_array[index].pattern;
	unsigned long long	*mem_addr_ptr	= (unsigned long long*)addr;
	char		  		*thread_context	= (char *)&th_array[index];
	int					current_tc_type	= current_rule->testcase_type;
	int					rc				= SUCCESS;

	if( *(unsigned long long *)pattern_ptr != *mem_addr_ptr) {

		if(crash_on_misc_global == TRUE && current_rule->crash_on_misc == TRUE) {
#ifdef __HTX_LINUX__
			do_trap_htx64((unsigned long)0xBEEFDEAD, (unsigned long) addr, (unsigned long) pattern_ptr,
						  (unsigned long) &mem, (unsigned long) &system_information , (unsigned long) index,
						  (unsigned long) thread_context, (unsigned long)current_rule);
#else
			trap(0xBEEFDEAD, addr, pattern_ptr, &mem,&system_information , index,
				 thread_context, current_rule);
#endif
		}

		sprintf(msg,"[%d] RS: TID: %d  miscompare !! EA = %p data = %llx pattern = %llx thread index : %d rule id = %d\n",__LINE__, tid,\
				addr, *((unsigned long long*)addr), *((unsigned long long *)pattern_ptr) ,index, current_tc_type);
		hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);

		rc = dump_miscompare_data(index,addr);
		exit(E_MISCOMPARE);

	}
	else {
		return(rc);
	}
}

int read_and_compare_mem(int index , int tid) {
	int 			walk_class, walk_line, memory_per_set, lines_per_set, cache_type;
	int 			data_width, rc = SUCCESS;
	unsigned int 	offset1, offset2 , offset_sum;
	char 			*addr = NULL;
	int				cache_size, associativity, line_size;

	/*
	 * cache_type decides whether calling for L2 or L3.
	 */

	cache_type 		= current_rule->tgt_cache;
	data_width 		= current_rule->data_width;
	cache_size		= system_information.cinfo[cache_type].cache_size;
	associativity	= system_information.cinfo[cache_type].asc;
	line_size		= system_information.cinfo[cache_type].line_size;

	memory_per_set 	= cache_size/associativity;
	lines_per_set 	= memory_per_set/line_size;

	/*
	 * If target_set is specified in rule file.
	 */
	if (current_rule->target_set != -1) {
		if(current_rule->target_set > ((2*associativity) - 1)) {
			sprintf(msg,"Specified target class-%d is out of range-%d \n", current_rule->target_set,((2*associativity) - 1));
			hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);

			sprintf(msg, "limitting class to %d \n", ((2*associativity) - 1));
			hxfmsg(&h_d, 0, HTX_HE_INFO, msg);

			th_array[index].end_class = (2*associativity) - 1;
		}
		else {
			th_array[index].start_class = th_array[index].end_class = current_rule->target_set;
		}
	}

	/*
	 * If data_width == 4 then it will randomize the type of load (i.e. byte,
	 * short, int or long long.
	 */
	if(data_width == RANDOM_WIDTH) data_width = (get_random_number(index))%4;

	/*
	 * The external loop targets line in set.
	 */
	for(walk_line = 0; walk_line < lines_per_set; walk_line++) {

		offset1 = walk_line * line_size + th_array[index].offset_within_cache_line;

		/*
		   * The inner loop here targets specific class and read/compare onto it.
		   */
		for(walk_class = th_array[index].start_class;
			walk_class <= th_array[index].end_class;
			walk_class += th_array[index].walk_class_jump) {

			offset2 = walk_class*memory_per_set;

			/* Calculate address to read/compare from */
			offset_sum	= offset1+offset2;

				addr = th_array[index].contig_mem[offset_sum/(huge_page_size)] + (offset_sum%(huge_page_size));
#if 0
			if (system_information.pvr == POWER7P ) {
			} else {
				addr = th_array[index].start_of_contiguous_memory + (offset1 + offset2);
			}
#endif

			switch(data_width) {
				case BYTE_WIDTH:
					rc = read_byte(addr, index, tid);
					break;

				case SHORT_WIDTH:
					rc = read_short(addr, index, tid);
					break;

				case INT_WIDTH:
					rc = read_int(addr, index, tid);
					break;

				case LONG_WIDTH:
					rc = read_ull(addr, index, tid);
					break;

				default:
					sprintf(msg, "[%d] Unknown Data width ( %d ). Exitting\n",__LINE__,data_width);
					hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
					return ( E_UNKNOWN_DATA_WIDTH );
			}

			if ( rc != SUCCESS ) {
				sprintf(msg,"%d: Miscompare found \n",__LINE__);
				hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
				return(rc);
			}
		}
	}
	return(rc);
}

void set_defaults()
{
	/*
	 * This function sets default values for every test case in rule file.
	 * Following are the defaults :
	 *
	 * rule_id - NULL string
	 * test - 0		i.e. L2 testing
	 * compare - 1		i.e. compare the results
	 * bound - -1		i.e. No binding
	 * data_width - 4	i.e. random data_width values
	 * target_set - -1	i.e. all classes
	 * crash_on_misc - 1	i.e. drop to KDB incase of miscompare
	 */
	int i,j;

	for(i = 0; i < MAX_TC; i+=3) {
		/*
		 * h_r being a global, everything defaults to zero.
		 */
		h_r[i].compare = 1;
		h_r[i].bound = -1;
		h_r[i].data_width = 4;
		h_r[i].target_set = -1;
		h_r[i].crash_on_misc = 1;
		h_r[i].pf_irritator = OFF;
		h_r[i].pf_nstride = OFF;
		h_r[i].pf_partial = OFF;
		h_r[i].pf_transient = OFF;
		h_r[i].pf_dcbtna = OFF;
		h_r[i].pf_dscr = DSCR_DEFAULT;
		h_r[i].testcase_type = CACHE_ROLL_WITH_PREF;
		h_r[i].gang_size = GANG_SIZE;
		h_r[i].thread_sync= TRUE;
		h_r[i].pf_conf =  PREFETCH_OFF;
		h_r[i].cache_memory_size = 0;
		h_r[i].prefetch_memory_size = PREFETCH_MEM_SZ;
		h_r[i].use_contiguous_pages = TRUE;

		for ( j=0 ; j<MAX_CORES_PER_CHIP ; j++ ) {
			h_r[i].exclude_cores[j] = -1;
			h_r[i+1].exclude_cores[j] = -1;
			h_r[i+2].exclude_cores[j] = -1;
		}

		h_r[i].num_excluded_cores = 0;
		h_r[i+1].num_excluded_cores = 0;
		h_r[i+2].num_excluded_cores = 0;
#if defined(__HTX_LINUX__) && defined(AWAN)
		h_r[i].tgt_cache = L3;
		h_r[i].num_oper = 15;
#endif
		for ( j=0 ; j<MAX_HUGE_PAGES ; j++ ) {
			h_r[i].mem_page_status[j] = PAGE_FREE;
			h_r[i+1].mem_page_status[j] = PAGE_FREE;
			h_r[i+2].mem_page_status[j] = PAGE_FREE;
		}

		h_r[i+1].compare = 1;
		h_r[i+1].bound = -1;
		h_r[i+1].data_width = 3;
		h_r[i+1].target_set = -1;
		h_r[i+1].crash_on_misc = 1;
		h_r[i+1].pf_irritator = OFF;
		h_r[i+1].pf_nstride = ON;
		h_r[i+1].pf_partial = ON;
		h_r[i+1].pf_transient = ON;
		h_r[i+1].pf_dcbtna = OFF;
		h_r[i+1].pf_dscr = DSCR_DEFAULT;
		h_r[i+1].testcase_type = CACHE_ROLL_WITH_PREF;
		h_r[i+1].gang_size = GANG_SIZE;
		h_r[i+1].thread_sync= TRUE;
		h_r[i+1].pf_conf =  0x7;
		h_r[i+1].cache_memory_size = 0;
		h_r[i+1].prefetch_memory_size = PREFETCH_MEM_SZ;
		h_r[i+1].use_contiguous_pages = TRUE;

#if defined(__HTX_LINUX__) && defined(AWAN)
		h_r[i+1].tgt_cache= L3;
		h_r[i+1].num_oper = 15;
#endif

		h_r[i+2].compare = 1;
		h_r[i+2].bound = -1;
		h_r[i+2].data_width = 3;
		h_r[i+2].target_set = -1;
		h_r[i+2].crash_on_misc = 1;
		h_r[i+2].pf_irritator = OFF;
		h_r[i+2].pf_nstride = ON;
		h_r[i+2].pf_partial = ON;
		h_r[i+2].pf_transient = ON;
		h_r[i+2].pf_dcbtna = OFF;
		h_r[i+2].pf_dscr = DSCR_DEFAULT;
		h_r[i+2].testcase_type = CACHE_BOUNCE_WITH_PREF;
		h_r[i+2].gang_size = GANG_SIZE;
		h_r[i+2].thread_sync= TRUE;
		h_r[i+2].pf_conf =  0x7;
		h_r[i+2].cache_memory_size = 0;
		h_r[i+2].prefetch_memory_size = PREFETCH_MEM_SZ;
		h_r[i+2].use_contiguous_pages = TRUE;

#if defined(__HTX_LINUX__) && defined(AWAN)
		h_r[i+2].tgt_cache = L3;
		h_r[i+2].num_oper = 15;
#endif

	}
}

int get_line( char s[], FILE *fp) {
	 int c,i;

	 i=0;
	 while (((c = fgetc(fp)) != EOF) && c != '\n') {
		s[i++] = c;
	 }
	 if (c == '\n')
		s[i++] = c;
	s[i] = '\0';
	 return(i);
}

int parse_line(char s[]){
	int len, i = 0, j = 0;

	while(s[i] == ' ' || s[i] == '\t') {
		i++;
	}
	if(s[i] == '*') {
		return(0);
	}
	len = strlen(s);
	for(; i < len && s[i] != '\0'; i++) {
		s[j++] = s[i];
	}
	s[j] = '\0';
	return((s[0] == '\n')? 1 : j);
}

int read_rule_file()
{

#if defined(__HTX_LINUX__) && defined(AWAN)
	set_defaults();
	num_testcases = 3;
#else

	FILE 			*fptr;
	int				rc = SUCCESS;
	char 			line[200], keywd[200];
	int 			eof_flag = 0, num_tc = 0, change_tc = 1;
	struct ruleinfo *current_ruleptr = &h_r[0];
	int disable_cores[MAX_CORES_PER_CHIP];
	int i=0;
	srand48_r(time(NULL),&sbuf);

	/* Set defaults is called below to fill in the rule file parameter structure just in case user makes mistakes.
	 * No prefetch would run, also see line 1534  .. very crucial
	 */

	set_defaults();
	errno = 0;
	if ( (fptr = fopen(rule_file_name, "r")) == NULL ) {
		sprintf(msg,"error open %s \n",rule_file_name);
		hxfmsg(&h_d, errno, HTX_HE_HARD_ERROR, msg);
		return(E_NO_FILE);
	}
	do {
		rc = get_line(line, fptr);
		/*
		 * rc = 0 indicates End of File.
		 * rc = 1 indicates only '\n' (newline char) on the line.
		 * rc > 1 more characters.
		 */
		if(rc == 0) {
			eof_flag = 1;
			break;
		}
		/*
		 * rc = 1 indicates a newline which means end of current testcase.
		  */
		if(rc == 1) {
			change_tc = 1;
			continue;
		}
		/*
		  * rc = 0 indicates comment line in rule file.
		  * rc = 1 indicates some white spaces and newline.
		  * rc > 1 indicates there may be valid test case parameter.
		  */
		rc = parse_line(line);
		
		if(rc == 0 || rc == 1) {
			if(rc == 1) change_tc = 1;
			continue;
		}
		else if(rc > 1 && change_tc == 1) {

			current_ruleptr = &h_r[num_tc];
			current_ruleptr->pf_conf = 0 ;
			if(num_tc >= MAX_TC) {
				sprintf(msg,"Max num of test cases allowed are %d \n", MAX_TC);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(FAILURE);
			}
			num_tc++;
			change_tc = 0;
		}

		sscanf(line, "%s",keywd);
		if ( (strcmp(keywd, "rule_id")) == 0 ) {
			char tmp[MAX_RULEID_LEN +1];
			sscanf(line, "%*s %s", tmp);
			if (((strlen(tmp)) < 1) || ((strlen(tmp)) > MAX_RULEID_LEN) ) {
				sprintf(msg, "rule_id string (%s) length should be in the range"
						" 1 < length < %d \n", current_ruleptr->rule_id,MAX_RULEID_LEN);
				hxfmsg(&h_d, 0, HTX_HE_SOFT_ERROR, msg);
				strncpy(current_ruleptr->rule_id, tmp, MAX_RULEID_LEN);
			}
			else {
				strcpy(&(current_ruleptr->rule_id[0]), tmp);
			}

			DEBUG_LOG("test id = %s\n",&(current_ruleptr->rule_id[0]));
		}
		else if ((strcmp(keywd, "target_cache")) == 0 ) {
			char tmp[4];
			sscanf(line, "%*s %s", tmp);
			if ( (strlen(tmp)) != 2 ) {
				sprintf(msg, "for 'target' possible values are L2 or L3 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(E_RF_PARSE_FAIL);
			} else if ( (strcmp(tmp,"L2") != 0) && ( strcmp(tmp,"L3") != 0) ) {
				sprintf(msg, "for 'target' possible values are L2 or L3 \n");
				hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
				return(E_RF_PARSE_FAIL);
			} else {
				if ( strcmp(tmp,"L2") == 0 ) {
					current_ruleptr->tgt_cache = L2;
				} else if ( strcmp(tmp,"L3") == 0 ) {
					current_ruleptr->tgt_cache = L3;
				}
				DEBUG_LOG("target cache - %d \n", current_ruleptr->tgt_cache);
			}
		}
		else if ((strcmp(keywd, "compare")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->compare);
			if (current_ruleptr->compare != 0 && current_ruleptr->compare != 1) {
				sprintf(msg, "for 'compare' possible values are 0 or 1 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("compare - %d \n", current_ruleptr->compare);
			}
		}
		else if ((strcmp(keywd, "bound")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->bound);
			if (current_ruleptr->bound < 0 ) {
				sprintf(msg, "For 'bound' possible values are valid proc nos \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("bound - %d \n", current_ruleptr->bound);
			}
		}
		else if ((strcmp(keywd, "data_width")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->data_width);
			if (current_ruleptr->data_width < 0 || current_ruleptr->data_width > 4) {
				sprintf(msg, "For 'data_width' possible values are 0 to 4 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("data_width - %d \n", current_ruleptr->data_width);
			}
		}
		else if ((strcmp(keywd, "target_set")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->target_set);
			if (current_ruleptr->target_set < -1) {
				sprintf(msg, "target_set can not be less than -1 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("target_set - %d \n", current_ruleptr->target_set);
			}
		}
		else if ((strcmp(keywd, "crash_on_misc")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->crash_on_misc);
			if (current_ruleptr->crash_on_misc < -1) {
				sprintf(msg, "crash_on_misc must be either 0 or 1 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("crash_on_misc - %d \n", current_ruleptr->crash_on_misc);
			}
		}
		else if ((strcmp(keywd, "num_oper")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->num_oper);
			if (current_ruleptr->num_oper < -1) {
				sprintf(msg, "num_oper must be either 0 or +ve integer \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("num_oper - %d \n", current_ruleptr->num_oper);
			}
		}
		else if ((strcmp(keywd, "prefetch_irritator")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->pf_irritator);

			if ( current_ruleptr->pf_irritator != 0 && current_ruleptr->pf_irritator != 1) {
				sprintf(msg, "pf_irritator should have value of either 0 or 1 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("pf_irritator - %d \n",current_ruleptr->pf_irritator);
			}

			/* Set the appropriate field in pf_conf */
			current_ruleptr->pf_conf +=  current_ruleptr->pf_irritator * PREFETCH_IRRITATOR;
		}
		else if ((strcmp(keywd, "prefetch_nstride")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->pf_nstride);

			if ( current_ruleptr->pf_nstride != 0 && current_ruleptr->pf_nstride != 1) {
				sprintf(msg, "pf_nstride should have value of either 0 or 1 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("pf_nstride - %d \n",current_ruleptr->pf_nstride);
			}

			if ( current_ruleptr->pf_nstride == ON && system_information.pvr == 0x3E) {
				/* Check if prefetch algorithm is supported on P6 */
				sprintf(msg, "prefetch n-stride not supported for P6\n");
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg );
				current_ruleptr->pf_nstride = OFF;
			}

			/* Set the appropriate field in pf_conf */
			current_ruleptr->pf_conf += current_ruleptr->pf_nstride * PREFETCH_NSTRIDE;
		}
		else if ((strcmp(keywd, "prefetch_partial")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->pf_partial);

			if ( current_ruleptr->pf_partial != 0 && current_ruleptr->pf_partial != 1) {
				sprintf(msg, "pf_partial should have value of either 0 or 1 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("pf_partial - %d \n",current_ruleptr->pf_partial);
			}

			if ( current_ruleptr->pf_partial == ON && system_information.pvr == 0x3E) {
				/* Check if prefetch algorithm is supported on P6 */
				sprintf(msg, "prefetch partial not supported for P6\n");
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg );
				current_ruleptr->pf_partial = OFF;
			}

			/* Set the appropriate field in pf_conf */
			current_ruleptr->pf_conf +=  current_ruleptr->pf_partial * PREFETCH_PARTIAL;
		}
		else if ((strcmp(keywd, "prefetch_transient")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->pf_transient);

			if ( current_ruleptr->pf_transient != 0 && current_ruleptr->pf_transient != 1) {
				sprintf(msg, "pf_transient should have value of either 0 or 1 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("pf_transient - %d \n",current_ruleptr->pf_transient);
			}

			if ( current_ruleptr->pf_transient == ON && system_information.pvr == 0x3E) {
				/* Check if prefetch algorithm is supported on P6 */
				sprintf(msg, "prefetch transient not supported for P6\n");
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg );
				current_ruleptr->pf_transient = OFF;
			}

			/* Set the appropriate field in pf_conf */
			current_ruleptr->pf_conf += current_ruleptr->pf_transient * PREFETCH_TRANSIENT;
		}
		else if ((strcmp(keywd, "prefetch_dcbtna")) == 0 ) {
			sscanf(line, "%*s %d", &current_ruleptr->pf_dcbtna);

			if ( current_ruleptr->pf_dcbtna != 0 && current_ruleptr->pf_dcbtna!= 1) {
				sprintf(msg, "pf_transient should have value of either 0 or 1 \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}
			else {
				DEBUG_LOG("pf_dcbtna- %d \n",current_ruleptr->pf_dcbtna);
			}

			if ( current_ruleptr->pf_dcbtna == ON &&  system_information.pvr <  POWER8_MURANO  ){
				sprintf(msg, "[%04d] Warning: prefetch dcbtna is supported only on P8 and above, current pvr = 0x%x\n",__LINE__, system_information.pvr);
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg );
				current_ruleptr->pf_dcbtna = OFF;
			}

			/* Set the appropriate field in pf_conf */
			current_ruleptr->pf_conf += current_ruleptr->pf_dcbtna * PREFETCH_NA;
		}
		else if ((strcmp(keywd, "dscr")) == 0 ) {
			char test[20];
			sscanf(line, "%*s %s", test);

			if ( strcmp(test,"DSCR_DEFAULT") == 0) {
				current_ruleptr->pf_dscr = DSCR_DEFAULT; 
			} else if ( strcmp( test, "DSCR_RANDOM") == 0) {
				current_ruleptr->pf_dscr = DSCR_RANDOM; 
			} else if ( strcmp( test, "DSCR_LSDISABLE" ) == 0 ) {
				current_ruleptr->pf_dscr = DSCR_LSDISABLE;
			} else {
				sprintf(msg, "pf_dscr should have value of either DSCR_DEFAULT / DSCR_RANDOM / DSCR_LSDISABLE \n");
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}

			if ( current_ruleptr->pf_dscr != DSCR_DEFAULT && system_information.pvr < POWER8_MURANO  ) {
				sprintf(msg, "[%04d] Warning: DSCR is writable from user space only on P8 and above, current pvr = 0x%x. Ignoring.\n",__LINE__,system_information.pvr);
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg );
				current_ruleptr->pf_dscr = DSCR_DEFAULT;
			}
			else {
				DEBUG_LOG( "pf_dscr - %d \n",current_ruleptr->pf_dscr );
			}

		}
		else if ((strcmp(keywd, "seed")) == 0 ) {
			sscanf(line, "%*s %x", &current_ruleptr->seed);
		}
		else if ((strcmp(keywd, "testcase_type")) == 0 ) {
			char test[20];
			sscanf(line, "%*s %s", test);

			if ( strcmp(test,"CACHE_BOUNCE_ONLY") == 0) {
				current_ruleptr->testcase_type = CACHE_BOUNCE_ONLY;
			} else if ( strcmp(test,"CACHE_BOUNCE_WITH_PREF") == 0) {
				current_ruleptr->testcase_type = CACHE_BOUNCE_WITH_PREF;
			} else if ( strcmp(test,"CACHE_ROLL_ONLY") == 0) {
				current_ruleptr->testcase_type = CACHE_ROLL_ONLY;
			} else if ( strcmp(test,"CACHE_ROLL_WITH_PREF") == 0) {
				current_ruleptr->testcase_type = CACHE_ROLL_WITH_PREF;
			} else if ( strcmp(test,"PREFETCH_ONLY") == 0) {
				current_ruleptr->testcase_type = PREFETCH_ONLY;
			} else {
				/* If Testcase type is not CACHE_BOUNCE/CACHE_ROLLOVER/PREFETCH_ONLY,
				 * report error and exit
				 */
				sprintf(msg, "[%d] Error: Invalid value for testcase_type (%s). Please refer README for valid values.\n", __LINE__, test);
				hxfmsg(&h_d, E_RF_PARSE_FAIL, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}

			if ( ( current_ruleptr->testcase_type == CACHE_ROLL_ONLY || current_ruleptr->testcase_type == CACHE_ROLL_WITH_PREF )  &&
				 system_information.pvr == 0x3E) {
				/* If Testcase type is CACHE_ROLLOVER and P6 */
				sprintf(msg, "testcase_type CACHE_ROLLOVER supported"
						" only on P7.Will run CACHE_BOUNCE testcase instead \n");
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg );
				current_ruleptr->testcase_type = CACHE_BOUNCE_WITH_PREF;
			}
		}
		else if ((strcmp(keywd, "gang_size")) == 0 ) {
			sscanf(line, "%*s %d", &gang_size);
			current_ruleptr->gang_size = gang_size;
			if( gang_size != 1 && gang_size != 16 && gang_size != 8) {
				/* If gang size is not 1 (for requaliser) AND
				 * If gang size is not 16 (default value ) , then the gang size is
				 * configured incorrectly. Print an error message and set gang size to
				 * default value
				 */
				sprintf(msg,"Gang Size is incorrectly configured to %d, should be either 1, 8 or 16 \n",gang_size);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}
		}
		else if ((strcmp(keywd, "thread_sync")) == 0) {
			char tmp_str[30];
			int str_len;

			sscanf(line, "%*s %s",tmp_str);
			str_len = strlen(tmp_str);
			
			if ( (str_len < 1) || (str_len > 3) ) {
				sprintf(msg,"Invalid value for keyword - %s. The valid values are yes/no (case insensitive)\n",keywd);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR,msg);
				return (E_RF_PARSE_FAIL);
			}

			if ( (strcmp(tmp_str, "yes") == 0) || (strcmp(tmp_str, "YES") == 0) ){
				current_ruleptr->thread_sync = TRUE;
				thread_sync_in_use = TRUE;
			} else if ( (strcmp(tmp_str, "no") == 0) || (strcmp(tmp_str, "NO") == 0) ) {
				current_ruleptr->thread_sync = FALSE;
				thread_sync_in_use = FALSE;
			} else {
				sprintf(msg,"Invalid value for keyword - %s. The valid values are yes/no (case insensitive)\n",keywd);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR,msg);
				return (E_RF_PARSE_FAIL);
			}
			
		} else if ((strcmp(keywd, "exclude_cores")) == 0) {
			char tmp_str[30];
			int result = FAILURE;
			char *token;
			const char delims[] =",";
			long int num_cores_to_disable = 0;
			int i;
			int percentage = 0;
			char remaining_string[50];

			sscanf(line, "%*s %s",tmp_str);

			/*
			 * exclude_cores field can be specified in 2 ways :
			 * 1. exclude_cores		RANDOM		( this will randomly disable cores )
			 * 2. exclude_cores		50%			( this will disable mentioned percentage of cores in the chip )
			 */

			/* Check if it is specified in the first way */
			if ( strcmp(tmp_str,"RANDOM") == 0 ) {
				int i=0;

				lrand48_r(&sbuf,&num_cores_to_disable);
				num_cores_to_disable = num_cores_to_disable%system_information.num_cores;
				PRINT_LOG("[%d] %d of total %d cores will be randomly disabled for rule id : %s\n",__LINE__,num_cores_to_disable,system_information.num_cores,&(current_ruleptr->rule_id[0]));

			} else {
				/* Check if the exclude_cores field is entered in a percentage */

				token = strtok(tmp_str,"%");
				if ( token != NULL ) {
					percentage = atoi(token);
					num_cores_to_disable = (percentage * system_information.num_cores / 100 );
					PRINT_LOG("[%d] %d of total %d cores will be disabled for rule id : %s\n", __LINE__,num_cores_to_disable,system_information.num_cores,&(current_ruleptr->rule_id[0]));

				} else {
		
#if 0
					printf("input = %s\n",tmp_str);
					token = strtok(tmp_str,delims);

					printf("[%d] input = %s, token = %s\n",__LINE__,tmp_str, token);
					while (token != NULL) {
						if ( atoi(token) < system_information.num_cores ) {
							disable_cores[num_cores_to_disable++] = atoi(token);
						} else {
							sprintf(msg,"[%d] Warning: relative core number %d is more that the number of cores in this instance (%d). Ignored\n",__LINE__,atoi(token),system_information.num_cores);
							hxfmsg(&h_d, 0, HTX_HE_SOFT_ERROR,msg);
						}
						/*current_ruleptr->exclude_cores[count] = atoi(token);*/
						DEBUG_LOG("[%d] count = %d, token = %s, core num = %d\n",__LINE__, num_cores_to_disable, token, current_ruleptr->exclude_cores[count]);
				token = strtok(NULL,delims);
			}

					result = num_cores_to_disable;
			if ( result == FAILURE ) {
				sprintf(msg,"[%d] Error: Invalid value for keyword - %s. Please refer README\n",__LINE__,keywd);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR,msg);
				return (E_RF_PARSE_FAIL);
			}
#endif
					sprintf(msg,"[%d] Invalid format for exclude_cores field. Only \"RANDOM\" or percentage value is supported. \n",__LINE__);
					hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
					return(E_RF_PARSE_FAIL);
				}
			}
			result = num_cores_to_disable;
			populate_cores_to_disable(current_ruleptr, (int *)NULL /*disable_cores*/, num_cores_to_disable);
#if 0
			for (i=0 ; i<result ; i++) {
				printf(" core = %d \n",current_ruleptr->exclude_cores[i]);
			}
#endif
			current_ruleptr->num_excluded_cores = result;
		} else if ((strcmp(keywd, "prefetch_mem_size")) == 0) {
			unsigned int prefetch_mem_size;
			sscanf(line, "%*s %d", &prefetch_mem_size);
			current_ruleptr->prefetch_memory_size = prefetch_mem_size;
			if( prefetch_mem_size <= 0) {
				sprintf(msg,"Prefetch memory size is incorrectly configured to %d, should be more than 0 \n",prefetch_mem_size);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}
		} else if ((strcmp(keywd, "cache_mem_size")) == 0) {
			unsigned int cache_mem_size;
			sscanf(line, "%*s %d", &cache_mem_size);
			
			if ( current_ruleptr->tgt_cache == L3 && (current_ruleptr->testcase_type == CACHE_ROLL_ONLY || current_ruleptr->testcase_type == CACHE_ROLL_WITH_PREF )) {
				current_ruleptr->cache_memory_size = cache_mem_size;	/* Since this value is applicable only in case we are rolling L3 cache memory */
			}

			if( cache_mem_size <= 0) {
				sprintf(msg,"Cache memory size is incorrectly configured to %d, should be more than 0 \n",cache_mem_size);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg );
				return(E_RF_PARSE_FAIL);
			}
		} else {
			sprintf(msg, "[%d]Error: Wrong keyword - %s specified in rule file. Exiting !! \n",__LINE__, keywd);
			hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
			return(E_RF_PARSE_FAIL);
		}
	} while(eof_flag == 0);
	num_testcases = num_tc;
#endif

	return(rc);
}

void write_read_and_compare(void *arg) {
	struct thread_context  	*t		= (struct thread_context *)arg;
	int						index	 = t->thread_no;
	int						cpu	   = t->bind_to_cpu;
	int						pcpu    = pcpus_thread_wise[index]; 
	int						tid	   = t->tid ;
	int						rc , oper;
	int						current_num_oper = current_rule->num_oper;
	int 					physical_cpu;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

#ifdef __HTX_LINUX__
		/*printf(" calling htx_bind with pcpu=%d for thread_no= %d\n",pcpu,index);*/
	if(pcpu == -1){
		pcpu = htx_bind_thread(cpu, -1);
		rc = pcpu;
		pcpus_thread_wise[index]=pcpu;
		if(pcpu < 0){
			pcpus_thread_wise[index]= -1;	
		}
	}
	else {
		rc = htx_bind_thread(cpu,pcpu);
	}
#else
	rc = bindprocessor(BINDTHREAD, thread_self(), cpu);
#endif
	DEBUG_LOG("[%d] thread %d, binding to cpu %d \n",__LINE__,index,cpu);
	if(rc < 0) {
#ifdef __HTX_LINUX__
			/*rc == -2 indicates cpu has been hot removed */
            if( rc == -2 || rc == -1) {
				tot_thread_count--;
                sprintf(msg,"lcpu:%d(pcpu=%d) has been hot removed, thread will be terminating, now tot_thread_count = %d\n",cpu,pcpu,tot_thread_count);
                hxfmsg(&h_d, errno, HTX_HE_INFO, msg);
				pthread_exit(NULL);
            }
            else {
                sprintf(msg, " line:%d: Bindprocessor for cache rwc operation on lcpu:%d and corresponding pcpu = %d failed with  and rc = %d\n", __LINE__,cpu,pcpu,rc);
                hxfmsg(&h_d, errno, HTX_HE_HARD_ERROR, msg);
			}

              
#else 
		sprintf(msg, "Binding to cpu:%d  failed with errno: %d \n",cpu,errno);
		hxfmsg(&h_d, errno, HTX_HE_SOFT_ERROR, msg);

#endif

	}
	else {
			/*sprintf(msg,"::physical cpu:%d for log cpu:%d\n",pcpu,cpu);
			hxfmsg(&h_d, rc , HTX_HE_INFO, msg);*/
		DEBUG_LOG("Binding to cpu:%d by thread successful ! \n",cpu );
	}

	/*
	 * If num_oper is 0 then run infinitely else num_oper times.
	 */
#ifdef DEBUG  
	unsigned int ct					= current_rule->tgt_cache;
	unsigned int pvr				= system_information.pvr;
	unsigned int current_line_size 	= system_information.cinfo[ct].line_size;
	unsigned int current_asc		= system_information.cinfo[ct].asc;
	unsigned int data_width	 		= current_rule->data_width;
	unsigned int memory_per_set 	= system_information.cinfo[ct].cache_size/system_information.cinfo[ct].asc;
	unsigned int lines_per_set  	= memory_per_set/system_information.cinfo[ct].line_size;


	if ( current_rule->tgt_cache == L2 ) {
		printf("[%d] target cache	= L2\n",__LINE__);
	} else if ( ct == L3 ) {
		printf("[%d] target cache	= L3\n",__LINE__);
	}

	printf("[%d] PVR             = 0x%x\n",	__LINE__,	pvr					);
	printf("[%d] Associativity   = %d\n",	__LINE__,	current_asc			);
	printf("[%d] Cache Line size = %d\n",	__LINE__,	current_line_size	);
	printf("[%d] Data width      = %d\n",	__LINE__,	data_width			);
	printf("[%d] Memory per set  = %d\n",	__LINE__,	memory_per_set		);
	printf("[%d] Lines per set   = %d\n",	__LINE__,	lines_per_set		);
#endif

	if (thread_sync_in_use == TRUE) {

		/* 
		 *	The exit_flag checking takes priority. If exit_flag is set (i.e. not FALSE), the loop has to be terminated
		 *  unconditionally. If however, the exit_flag is not set, then the loop continues as long as oper is less
		 *  than surrent_stanza_num_oper OR surrent_stanza_num_oper is set to 0 (i.e. infinite loop).
		 */
	
		for(oper = 0; ( exit_flag == FALSE && (oper < current_num_oper || current_num_oper == 0) ) ; oper++) {

			if (t->seedval == 0 ) {
				t->seedval = time(NULL);
			}

			srand48_r(t->seedval,&t->buffer);
			th_array[index].current_oper = oper;

			/* Sync up before starting the pass */
			synchronize_threads(index);
			
			write_mem(index ,tid);

			/* Sync up before starting read */
			synchronize_threads(index);
		
 		   	if (current_rule->compare == 1 ) {
				srand48_r(t->seedval,&t->buffer);
				read_and_compare_mem(index , tid);
			}

#if defined(__HTX_LINUX__) && defined(AWAN)
			printf("write_read_and_compare: Completed pass number %d\n",oper);
#endif

			/* Sync up for all threads to complete */
			synchronize_threads(index);
		
		}	/* End of oper loop */

	} else if ( thread_sync_in_use == FALSE ) {
		for(oper = 0; (exit_flag == FALSE && (oper < current_num_oper || current_num_oper == 0)) ; oper++) {

			if (t->seedval == 0 ) {
				t->seedval = time(NULL);
			}

			srand48_r(t->seedval,&t->buffer);
			th_array[index].current_oper = oper;
	
			write_mem(index ,tid);

 		   	if (current_rule->compare == 1 ) {
				srand48_r(t->seedval,&t->buffer);
				read_and_compare_mem(index , tid);
			}

#if defined(__HTX_LINUX__) && defined(AWAN)
			printf("[%d] write_read_and_compare: Thread no %d , Completed pass number %d\n",__LINE__,index,oper);
#endif

		}	/* End of oper loop */
		
	}

#ifdef __HTX_LINUX__
	/* Restore original/default CPU affinity so that it binds to ANY available processor */

	rc = htx_unbind_thread();
#else
	rc = bindprocessor(BINDTHREAD, thread_self(), PROCESSOR_CLASS_ANY);
#endif
	if(rc == -1) {
		sprintf(msg, "%d: Unbinding from cpu:%d failed with errno %d \n",__LINE__, cpu, errno);
		hxfmsg(&h_d, errno, HTX_HE_SOFT_ERROR, msg);
	}
#if defined(__HTX_LINUX__) && ( defined(__HTX_MAMBO__) || defined(AWAN) )
	printf("[%d] Thread no: %d, completed passes : %d\n",__LINE__, index, oper);
#endif
	return ;
}

#ifndef __HTX_LINUX__
void DR_handler(int sig, int code, struct sigcontext *scp) {
	
	int	   		rc = 0,i,j;
	pid64_t   	pid ;
	char	  	tmp[128], workstr[80];
	dr_info_t 	DRinfo;
	pthread_t 	current_tid ;
	int 		check_flag = 0, pre_flag = 0, post_flag = 0, max_cpus;

	pid = getpid();

	current_tid = thread_self();
	do {
		rc = dr_reconfig(DR_QUERY, &DRinfo);
	} while ((rc < 0) && (errno == EINTR));

	if( (rc < 0) && (errno == ENXIO) ) {
		return;
	}
	else if (rc < 0) {
		sprintf(msg, "DR: %s for %s: Error in DR_QUERY.  \n\
		dr_reconfig system call returned error(%d).\n",
				h_d.HE_name, h_d.sdev_id, errno);
		hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
		return;
	   }

	if (exit_flag != DR_EXIT) {
  		sprintf(msg,"DR: SIGRECONFIG  signal received !!\n") ;
   		hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
	 
		if (DRinfo.mem == 1 || DRinfo.ent_cap == 1 || DRinfo.hibernate == 1){
   			sprintf(msg, "DR: DLPAR Details - \n"\
						"Phase - Check:  %d, Pre: %d, Post: %d, Post Error: %d\n"\
						"Type - Mem add: %d remove: %d, ent_cap = %d, hibernate = %d \n"\
						"Action - No action taken. Doing DR_RECONFIG_DONE \n", \
						DRinfo.check, DRinfo.pre, DRinfo.post, DRinfo.posterror, DRinfo.add, DRinfo.rem, DRinfo.ent_cap, DRinfo.hibernate);
			hxfmsg(&h_d,0,HTX_HE_INFO,msg);

			if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)){
				sprintf(msg,"dr_reconfig(DR_RECONFIG_DONE) failed with errno %d \n", errno);
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
				return;
			}

			sprintf(msg,"DR: DR_RECONFIG_DONE Success!!\n");
			hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
		}

	/* cpu remove or add operation */

		if (DRinfo.cpu == 1 && (DRinfo.rem || DRinfo.add) ) {

			/* Check if a cpu in this instance is removed. If it is, then shutdown this instance. */
			/* If it is not, then no need to kill this instance. Reply with DR_RECONFIG done.	  */

 		   if(DRinfo.check == 1) {
				for( i=0; i<MAX_CPUS_PER_CHIP; i++) {

					if (system_information.cpus_in_instance[i] == DRinfo.bcpu || (DRinfo.add && total_cpus < MAX_CPUS_PER_CHIP)){
	
						rc = pthread_mutex_lock( &dr_mutex );
						if(rc != 0) {
							sprintf(msg,"[%d] %s: Could not acquire lock\n",__LINE__,__FUNCTION__);
							hxfmsg(&h_d,errno,HTX_HE_SOFT_ERROR,msg);
						}

						exit_flag = DR_EXIT ;

						/* Release the mutex lock */

						rc = pthread_mutex_unlock( &dr_mutex );		   
						if(rc != 0) {
							sprintf(msg,"%s:[%d] %s: Could not release lock\n",__FUNCTION__,__LINE__);
							hxfmsg(&h_d,errno,HTX_HE_SOFT_ERROR,msg);
	   	 				}

	   					sprintf(msg, "DR: DLPAR Details - \n"\
								"Phase - Check:  %d, Pre: %d, Post: %d, Post Error: %d\n"\
								"Type - CPU remove : %d, add : %d bcpu = %d thread_id = 0x%x\n"\
								"Action - cleaning up and exit \n", \
								DRinfo.check, DRinfo.pre, DRinfo.post, DRinfo.posterror, DRinfo.rem, DRinfo.add, DRinfo.bcpu, current_tid);
						hxfmsg(&h_d,0,HTX_HE_INFO,msg);

 		  				if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)) {
								sprintf(msg,"dr_reconfig(DR_RECONFIG_DONE) failed for check phase.  error no %d \n", errno);
								hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
								return;
						} else {
								sprintf(msg,"DR: DR_RECONFIG_DONE Success for check phase!!\n");
								hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
						}
 			   			hxfupdate(RECONFIG, &h_d);

						return;
					}
				} 
   			}

 	  		sprintf(msg, "DR: DLPAR Details - \n"\
						"Phase - Check: %d, Pre: %d, Post: %d, Posterr: %d\n"\
						"Type - CPU remove : %d, add : %d bcpu = %d thread_id = 0x%x\n",\
						DRinfo.check, DRinfo.pre, DRinfo.post, DRinfo.posterror,\
						DRinfo.rem, DRinfo.add,DRinfo.bcpu,current_tid);

			if(DRinfo.check == 1)
				strcat(msg, "Action - Necessary steps taken in check phase.\n");
 			else if(DRinfo.pre || DRinfo.post || DRinfo.posterror) 
				strcat(msg, "Action - Action taken in check phase \n");

 		   	hxfmsg(&h_d,0,HTX_HE_INFO,msg);

 		  	if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)) {
			   	sprintf(msg,"dr_reconfig(DR_RECONFIG_DONE) failed.  error no %d \n", errno);
			   	hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
				return;
	   		} else {
				sprintf(msg,"DR: DR_RECONFIG_DONE Success!!\n");
				hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
			}

 	   } /* rem / add cpu */
	}

	/* For any other signal check/Pre/Post/posterror-phase, respond with DR_RECONFIG_DONE */
	if (DRinfo.check || DRinfo.pre || DRinfo.post || DRinfo.posterror){
		if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)) {
			sprintf(msg,"dr_reconfig(DR_RECONFIG_DONE) failed for check=%d,pre=%d,post=%d,posterr=%d.error no %d \n",DRinfo.check,DRinfo.pre,DRinfo.post,DRinfo.posterror,errno);
			hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
			return;
		} else {
			sprintf(msg,"DR: DR_RECONFIG_DONE Success!!\n");
			hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
		}
	}
	return;
}
#endif

unsigned long int get_random_number(int seg)
{
	unsigned long int val_8 ;

	/* lrand48_r returns 4 byte random number . Generate one random number shift it by 4bytes and OR it
	 * 4 bytes random number generated by second lrand48_r call
	 */
	lrand48_r(&th_array[seg].buffer, &th_array[seg].random_pattern);
	val_8 = (unsigned long)th_array[seg].random_pattern << 32 ;
	lrand48_r(&th_array[seg].buffer, &th_array[seg].random_pattern);
	val_8 = val_8 | th_array[seg].random_pattern ;
	return val_8 ;
}

int setup_prefetch_thread_context(void) {
	int rc = SUCCESS;
	int prefetch_thread_number=0,i=0, k=0;
	int j=0, index=0, bound_cpu, core, smt, core_num;
	int testcase = current_rule->testcase_type;
	char *ptr;

	if ( current_rule->pf_conf == PREFETCH_OFF ) {
		sprintf(msg,"[%d] Prefetch is not enabled, hence no need to setup prefetch context\n",__LINE__);
		hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
		return (rc);
	}

	switch( testcase) {
		case PREFETCH_ONLY:
			total_prefetch_threads							= get_num_threads(system_information.instance_number, testcase, PREFETCH, current_rule);
			current_rule->num_prefetch_threads_to_create	= total_prefetch_threads;
			core = 0;
			for(; index < current_rule->num_prefetch_threads_to_create; ) {
				core_num									= system_information.cores_in_instance[core];
				if ( core_to_exclude(&(current_rule->exclude_cores[0]), core_num) == TRUE ) {
					continue;
				}

				DEBUG_LOG("[%d] calling get_next_cpu \n",__LINE__);
				smt 										= system_information.core_smt_array[core];
				bound_cpu				   					= get_next_cpu( PREFETCH, PREFETCH_ONLY, index, core ) ;
				if ( bound_cpu == E_CORE_EXCLUDED ) {
					continue;
				}
				th_array[index].thread_no   				= index;
				th_array[index].bind_to_cpu					= bound_cpu;
				th_array[index].thread_type 				= PREFETCH;
				th_array[index].current_rule				= current_rule;
				th_array[index].prefetch_algorithm		 	= derive_prefetch_algorithm_to_use(index);
				th_array[index].start_of_contiguous_memory 	= (char *)current_rule->cont_memory_pool.prefetch_mem_set[index];
				th_array[index].prefetch_streams			= MAX_PREFETCH_STREAMS / smt ;

				ptr = (char *)&th_array[index].pattern;
				/*if ( index >= 0xf ) {
					high_nibble = index % FOUR_BITS_ON;
					low_nibble = high_nibble + 2;
				} else {
					high_nibble = index;
					low_nibble = high_nibble + 1;
				}

				DEBUG_LOG("[%d] high_nibble = 0x%x, low_nibble = 0x%x\n",__LINE__,high_nibble, low_nibble);
				tmp = ((high_nibble & FOUR_BITS_ON) << 4);*/
				for (k=0; k<sizeof(th_array[index].pattern); k++) {
					*ptr++ = index+1;
				}

				pthread_attr_init(&th_array[index].thread_attrs_detached);
				pthread_attr_setdetachstate(&th_array[index].thread_attrs_detached, PTHREAD_CREATE_JOINABLE);

				/*smt = get_smt_status(system_information.cores_in_instance[core]);*/
				smt = system_information.core_smt_array[core];
				if( (index+1)%smt == 0 ) {
					core++;
				}

				index++;
			} /* end of for loop */
			break;
		case CACHE_BOUNCE_WITH_PREF:
		case CACHE_ROLL_WITH_PREF:
			total_prefetch_threads							= get_num_threads(system_information.instance_number, testcase, PREFETCH, current_rule);
			current_rule->num_prefetch_threads_to_create	= total_prefetch_threads;
			for(core=0;core < num_cores ; core++){
			core_num  										= system_information.cores_in_instance[core];
				if ( core_to_exclude(&(current_rule->exclude_cores[0]), core_num) == TRUE ) {
					continue;
				}
				core_num									= system_information.cores_in_instance[core];
				/*smt = get_smt_status(core_num);*/
				smt = system_information.core_smt_array[core];

				for(j = 1; j<smt ; j++) {
					index					   					= i + j;
                    if(th_array[index].thread_type == CACHE)continue;
					bound_cpu				   					= get_next_cpu( PREFETCH, testcase, index, core);
#ifdef __HTX_MAMBO__
					DEBUG_LOG("[%d] Prefetch thread number %d will be bound to CPU no %d\n",__LINE__,index,bound_cpu);
#endif
					if ( bound_cpu == E_CORE_EXCLUDED ) {
						continue;
					}
					th_array[index].bind_to_cpu 				= bound_cpu;
					th_array[index].thread_no   				= index;
					th_array[index].thread_type 				= PREFETCH;
					th_array[index].current_rule				= current_rule;
					th_array[index].prefetch_algorithm 			= derive_prefetch_algorithm_to_use(index);
					th_array[index].start_of_contiguous_memory 	= (char *)current_rule->cont_memory_pool.prefetch_mem_set[prefetch_thread_number];
					th_array[index].prefetch_streams			= MAX_PREFETCH_STREAMS / ( smt-1 ) ;
	 	
					ptr = (char *)&th_array[index].pattern;

					for (k=0; k<sizeof(th_array[index].pattern); k++) {
						*ptr++ = index+1;
					}
				
					pthread_attr_init(&th_array[index].thread_attrs_detached);
					pthread_attr_setdetachstate(&th_array[index].thread_attrs_detached, PTHREAD_CREATE_JOINABLE);

					prefetch_thread_number++;

				} /* end of for loop */
				i += smt; 
			}
			break;
		case CACHE_BOUNCE_ONLY:
		case CACHE_ROLL_ONLY:
			current_rule->num_prefetch_threads_to_create	= 0;
			rc = E_WRONG_TEST_CASE;
			sprintf(msg,"[%d] Error: Current testcase (%d) does not support prefetch. Exitting!\n",__LINE__,current_rule->testcase_type);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return (rc);
			
		default:
			rc = E_INVALID_TEST_CASE;
			sprintf(msg,"[%d] Invalid Testcase %d\n",__LINE__,current_rule->testcase_type);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			break;
	}
	return (rc);
}


int setup_cache_thread_context(void) {
	int 		thd,i=0, j=0, index=0, mem_per_thread, bound_cpu,core,smt,core_num,k,l;
	int			num_cores		= system_information.num_cores;
	int 		rc 				= SUCCESS;
	int 		tgt_cache_type 	= current_rule->tgt_cache;
	int 		cache_asc		= system_information.cinfo[tgt_cache_type].asc;
	int 		line_size		= system_information.cinfo[tgt_cache_type].line_size;
	int 		testcase		= current_rule->testcase_type;
	int			pnum 				= 0;
	char 		*ptr ;			
	int			set_count			= 0;
	int			pg_size			= huge_page_size;
	int			contiguous_pages 	= ceil( (double)current_rule->cont_memory_pool.contiguous_mem_required / (double) pg_size);
	int			enabled_cores		= system_information.num_cores - current_rule->num_excluded_cores;

	if( testcase == CACHE_BOUNCE_ONLY || testcase == CACHE_BOUNCE_WITH_PREF ) {

		/* In case of Cache Bounce test case, only one instance on cache thread runs per core 	*/
		/* irrespective of the fact whether prefetch is running or not.							*/
		/* In case of bounce testcase, only L2 cache memory is targeted.						*/
		/* A cache line width is split amongst the Cache threads. 								*/

		tgt_cache_type 								= L2;
		total_cache_threads							= get_num_threads(system_information.instance_number, testcase, CACHE, current_rule);
		current_rule->num_cache_threads_to_create	= total_cache_threads;
		mem_per_thread 								= line_size/current_rule->num_cache_threads_to_create;

        for(core=0,thd=0;thd<total_cache_threads;thd++,core++){
            if(system_information.pvr == POWER9_CUMULUS){
                if(core == num_cores){
                    core = 0;
                    index = (system_information.core_smt_array[core]/2);
                }        
            }
			core_num  										= system_information.cores_in_instance[core];
			if ( core_to_exclude(&(current_rule->exclude_cores[0]), core_num) == TRUE ) {
				continue;
			}
			/*smt	   											= get_smt_status(system_information.cores_in_instance[core]);*/
			smt												= system_information.core_smt_array[core];
			bound_cpu 										= get_next_cpu(CACHE, testcase, index, core ) ;
			pnum											= 0;
			if ( bound_cpu == E_CORE_EXCLUDED ) {
				continue;
			}
			th_array[index].bind_to_cpu 					= bound_cpu;
			th_array[index].thread_no   					= index;
			th_array[index].seedval	 						= current_rule->seed;
			th_array[index].thread_type 					= CACHE;
			th_array[index].current_rule					= current_rule;
			th_array[index].start_class	 					= 0;
			th_array[index].end_class	   					= cache_asc - 1;
			th_array[index].walk_class_jump 				= 1;
			th_array[index].cache_instance_under_test 		= j;
			th_array[index].memory_starting_address_offset 	= 0;
			/*th_array[index].start_of_contiguous_memory 		= (char *)current_rule->cont_memory_pool.cont_mem_set[0][0];*/
			th_array[index].num_mem_sets					= 1;/*(current_rule->cache_HUGE_pages_required / enabled_cores) / contiguous_pages;*/
			th_array[index].contig_mem[pnum++] 				= (char *)current_rule->cont_memory_pool.cont_mem_set[0][0];
			th_array[index].pages_to_write 					= 1;
#if 0
			if ( core == (num_cores-1) ) {
				th_array[index].num_mem_sets				+= (current_rule->cache_HUGE_pages_required % enabled_cores) / contiguous_pages;
			}

			for (l=j*th_array[index].num_mem_sets , set_count=0 ; set_count < th_array[index].num_mem_sets ; l++, set_count++ ) {
				for (k=0 ; k<contiguous_pages; k++) {
					th_array[index].contig_mem[pnum++] = (char *)current_rule->cont_memory_pool.cont_mem_set[l][k];
				}
			}
#endif
			ptr = (char *)&th_array[index].pattern;
			for (i=0; i<sizeof(th_array[index].pattern); i++) {
				*ptr++ = index+1;
			}

			th_array[index].offset_within_cache_line 		= mem_per_thread*(j);

			pthread_attr_init(&th_array[index].thread_attrs_detached);
			pthread_attr_setdetachstate(&th_array[index].thread_attrs_detached, PTHREAD_CREATE_JOINABLE);

			j++;
			/*index += get_smt_status(system_information.cores_in_instance[core]);*/
			index += system_information.core_smt_array[core];
		}
	} else if( testcase == CACHE_ROLL_WITH_PREF || testcase == CACHE_ROLL_ONLY) {  /* Roll over supported for P7 and above only */

		total_cache_threads							= get_num_threads(system_information.instance_number, testcase, CACHE, current_rule);
		current_rule->num_cache_threads_to_create	= total_cache_threads;

        for(core=0,thd=0;thd<total_cache_threads;thd++,core++){
            if(system_information.pvr == POWER9_CUMULUS){
                if(core == num_cores){
                    core = 0;
                    index = (system_information.core_smt_array[core]/2);
                }
            }
			core_num  										= system_information.cores_in_instance[core];
			if ( core_to_exclude(&(current_rule->exclude_cores[0]), core_num) == TRUE ) {
				DEBUG_LOG("[%d] excluding core index = %d , core number = %d \n",__LINE__,core,core_num);
				continue;
			}
			DEBUG_LOG("[%d] calling get_next_cpu \n",__LINE__);
			bound_cpu 										= get_next_cpu(CACHE, testcase, index, core ) ;
			pnum											= 0;
			if ( bound_cpu == E_CORE_EXCLUDED ) {
				continue;
			}
			th_array[index].bind_to_cpu 					= bound_cpu;
			th_array[index].thread_no   					= index;
			th_array[index].seedval	 						= current_rule->seed;
			th_array[index].thread_type 					= CACHE;
			th_array[index].current_rule					= current_rule;
			th_array[index].start_class	 					= 0;
			th_array[index].end_class	   					= (2*cache_asc) - 1;
			th_array[index].walk_class_jump 				= 1;
			th_array[index].cache_instance_under_test 		= j;
			th_array[index].memory_starting_address_offset 	= /*th_array[index].cache_instance_under_test * 2*cache_size*/ 0 ;
			th_array[index].num_mem_sets					= (current_rule->cache_HUGE_pages_required / enabled_cores) / contiguous_pages;

			/* In case this is the last core, assign all remaining pages to this core.	*/
			if ( core == (num_cores-1) ) {
				th_array[index].num_mem_sets				+= (current_rule->cache_HUGE_pages_required % enabled_cores) / contiguous_pages;
			}

			/* This is the offset within each cache line where this thread will write/read */
			th_array[index].offset_within_cache_line =	0;
			/* This is the start of contiguous memory where each thread will write 	*/
			/* This field does not make any sense anymore for cache threads.		*/
			/* Instead now contig_mem array is used.								*/
			/*th_array[index].start_of_contiguous_memory 		= (char *)current_rule->cont_memory_pool.cont_mem_set[j][0];*/

			/*for (k=0 ; k<MAX_HUGE_PAGES_PER_CORE ; k++) {
				th_array[index].contig_mem[k] = (char *)current_rule->cont_memory_pool.cont_mem_set[j][k];
			}*/

			for (l=j, set_count=0 ; set_count<th_array[index].num_mem_sets ; l++,set_count++ ) {
				for (k=0 ; k<contiguous_pages; k++) {
					th_array[index].contig_mem[pnum++] = (char *)current_rule->cont_memory_pool.cont_mem_set[l][k];
				}
			}

			DEBUG_LOG("[%d] Pages to write for thread %d = %d and contiguous_pages = %d\n",__LINE__,index,pnum,contiguous_pages);
			th_array[index].pages_to_write = pnum;
			ptr = (char *)&th_array[index].pattern;

			for (i=0; i<sizeof(th_array[index].pattern); i++) {
				*ptr++ = index + 1;
			}

			pthread_attr_init(&th_array[index].thread_attrs_detached);
			pthread_attr_setdetachstate(&th_array[index].thread_attrs_detached, PTHREAD_CREATE_JOINABLE);

			j++;
			index += system_information.core_smt_array[core];
			/*index += get_smt_status(system_information.cores_in_instance[core]);*/
		}
	}
		else if (testcase == PREFETCH_ONLY) {
		current_rule->num_cache_threads_to_create	= 0;
		rc = E_WRONG_TEST_CASE;
		sprintf(msg,"[%d] Error: Current testcase (%d) does not support Cache. Exitting!\n",__LINE__,current_rule->testcase_type);
		hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
		return (rc);
	}

	return (rc);
}

/*
 * This function setup the array of thread_context's so that thread specific information
 * can be initialised, prior to creation of the threads themselves.
 */

int setup_thread_context_array(int thread_type) {
	int rc = SUCCESS;

	switch(thread_type) {
		case CACHE:
			if ( (rc = setup_cache_thread_context()) != SUCCESS) {
				sprintf(msg,"[%d] Error in creating cache thread context\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
			}
			break;
		case PREFETCH:
			if ( (rc = setup_prefetch_thread_context()) != SUCCESS) {
				sprintf(msg,"[%d] Error in creating prefetch thread context\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
			}
			break;
		case ALL:
			if ( (rc = setup_thread_context_array(CACHE)) != SUCCESS) {
				sprintf(msg,"[%d] Error in creating cache thread context\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
			}
			if ( (rc = setup_thread_context_array(PREFETCH)) != SUCCESS) {
				sprintf(msg,"[%d] Error in creating Prefetch thread context\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
			}
			break;
		default:
			sprintf(msg,"[%d] Invalid thread type = %d\n",__LINE__,thread_type);
			hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
			rc = FAILURE;
			break;
	}

	return (rc);
}

int cleanup_thread_context_array(int thread_type) {
	int j,thd,index=0, core, smt,rc = SUCCESS;
	int num_cores				= system_information.num_cores;
	int testcase				= current_rule->testcase_type;
	int num_cache_threads		= current_rule->num_cache_threads_created;
	int num_prefetch_threads	= current_rule->num_prefetch_threads_created;

	DEBUG_LOG("\n[%d] called with thread_type = %d\n",__LINE__,thread_type);

	rc = pthread_mutex_lock( &mutex );
	if(rc != 0) {
		sprintf(msg,"[%d] %s: Could not acquire lock\n",__LINE__,__FUNCTION__);
		hxfmsg(&h_d,errno,HTX_HE_SOFT_ERROR,msg);
		return (FAILURE);
	}

	/* Cleanup Prefetch threads here ,if required */
	switch ( thread_type ) {
		case PREFETCH:
			if ( current_rule->pf_conf == 0 ) {
				sprintf(msg,"[%d] Prefetch is not enabled, so no need of cleanup\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(SUCCESS);
			}

			if(testcase == PREFETCH_ONLY) {
				for(index = 0; index < num_prefetch_threads; index++) {
					pthread_attr_destroy(&th_array[index].thread_attrs_detached);
					memset(&th_array[index], 0, sizeof(th_array[index]));
				} /* end of for loop */
			} else {
				int i=0;
				for(core=0; core < num_cores ; core++) {
					int core_num = system_information.cores_in_instance[core];
					if ( core_to_exclude(&(current_rule->exclude_cores[0]), core_num) == TRUE ) {
						continue;
					}
					/*smt = get_smt_status(system_information.cores_in_instance[core]);*/
					smt = system_information.core_smt_array[core];
					for(j = 1; j<smt ; j++) {
						index = ( i * smt ) + j;
                        if(th_array[index].thread_type == CACHE)continue;
						pthread_attr_destroy(&th_array[index].thread_attrs_detached);
						memset(&th_array[index], 0, sizeof(th_array[index]));
					}
					i += smt; 
				 }
			}
			break;

		case CACHE:
			if( (testcase == CACHE_BOUNCE_ONLY) || ( testcase == CACHE_BOUNCE_WITH_PREF) || ( testcase == CACHE_ROLL_WITH_PREF)) {
			/* If testcase is CACHE_BOUNCE or testcase is CACHE_ROLLOVER AND prefetch is running, then */
			/* one instance of cache thread runs per core											  */ 
	
				index = 0;
                for(core=0,thd=0;thd<num_cache_threads;thd++,core++){
                    if(system_information.pvr == POWER9_CUMULUS){
                        if(core == system_information.num_cores){
                            index = (system_information.core_smt_array[core]/2);;
                            core = 0;
                        }
                    }
					int core_num = system_information.cores_in_instance[core];
					if ( core_to_exclude(&(current_rule->exclude_cores[0]), core_num) == TRUE ) {
						continue;
					}

					pthread_attr_destroy(&th_array[index].thread_attrs_detached);
					memset(&th_array[index], 0, sizeof(th_array[index]));
					/*index += get_smt_status(system_information.cores_in_instance[core]);*/
					index += system_information.core_smt_array[core];
				}
			} else if( testcase == CACHE_ROLL_ONLY) {
				/* If Testcase is CACHE_ROLLOVER and Prefetch is OFF, then all threads run as cache threads per core */
				for(index = 0; index < num_cache_threads; index += 1) { 
					pthread_attr_destroy(&th_array[index].thread_attrs_detached);
					memset(&th_array[index], 0, sizeof(th_array[index]));
				}
			}	   /* End of else-if statement */
			break;

		case ALL:
			if ( (rc = cleanup_thread_context_array(CACHE)) != SUCCESS) {
				sprintf(msg,"[%d] Error in cleaning up cache thread context\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(FAILURE);
			}
			if((rc = cleanup_thread_context_array(PREFETCH)) != SUCCESS) {
				sprintf(msg,"[%d] Error in cleaning up prefetch thread context\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return(FAILURE);
			}
			break;

		default:
			sprintf(msg,"[%d] Error: Invalid thread type = %d\n",__LINE__,thread_type);
			hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
			rc = E_UNKNWON_THR_TYPE;
			break;
	}

	/* Release the mutex lock */

	rc = pthread_mutex_unlock( &mutex );
	if(rc != 0) {
		sprintf(msg,"[%d] %s: Could not release lock\n",__LINE__,__FUNCTION__);
		hxfmsg(&h_d,errno,HTX_HE_SOFT_ERROR,msg);
		return (FAILURE);
	}

	return rc;
}

int create_prefetch_threads(void){
	int i, j,index, core, smt,rc = SUCCESS;
	int testcase_type = current_rule->testcase_type;
	int num_prefetch_threads	= current_rule->num_prefetch_threads_to_create;

	if ( current_rule->pf_conf == 0 ) {
		current_rule->num_prefetch_threads_created = 0;
	  	sprintf(msg,"[%d] Prefetch is disabled, hence no need to create prefetch threads \n",__LINE__);
		hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
		return (SUCCESS);
	}

	switch(testcase_type) {
		case PREFETCH_ONLY:
			if( !exit_flag ) {
				int core_num;
				core = 0;
				for( index=0 ; index < num_prefetch_threads ; ) {
					core_num = system_information.cores_in_instance[core];
					if ( core_to_exclude(&(current_rule->exclude_cores[0]), core_num) == TRUE ) {
						continue;
					}
					
					if(pthread_create(&th_array[index].tid, &th_array[index].thread_attrs_detached,(void *(*)(void *))prefetch_irritator, (void *)&th_array[index]) ) {
						/* If create threads fails, perform cleanup and return fail(-1) to calling function */
						sprintf(msg, "Prefetch pthread_create call failed with %d \n", errno);
						hxfmsg(&h_d, errno, HTX_HE_HARD_ERROR, msg);
						sprintf(msg,"[%d] calling cleanup prefetch \n",__LINE__);
						hxfmsg(&h_d, 0, HTX_HE_INFO, msg); 
						if ( (rc = cleanup_threads(PREFETCH)) != SUCCESS) {
							sprintf(msg, "[%d] Cleaning up of Prefetch pthreads failed \n", __LINE__);
							hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
							return (FAILURE);
						}
						return (FAILURE);
					} /* end of if */
					else {
						mem.prefetch_threads++;
						current_rule->num_prefetch_threads_created++;
					} /* end of else */

					/*smt = get_smt_status(system_information.cores_in_instance[core]);*/
					smt = system_information.core_smt_array[core];
					if( (index+1)%smt == 0 ) {
						core++;
					}

					index++;
				}
				DEBUG_LOG("\n[%d] Info: Prefetch threads created = %d",__LINE__,current_rule->num_prefetch_threads_created);
			}	/* End of cheking exit_flag */
			break;
		case CACHE_BOUNCE_WITH_PREF:
		case CACHE_ROLL_WITH_PREF:
			i = 0;
			if( !exit_flag ) {
				for(core=0; core < system_information.num_cores ; core++ ) {
					int core_num = system_information.cores_in_instance[core];
					if ( core_to_exclude(&(current_rule->exclude_cores[0]), core_num) == TRUE ) {
						continue;
					}
					/*smt = get_smt_status(system_information.cores_in_instance[core]);*/
					smt = system_information.core_smt_array[core];
					for(j = 1; j<smt ; j++) {
						index = i + j;
                        if(th_array[index].thread_type == CACHE)continue;
						if(pthread_create(&th_array[index].tid, &th_array[index].thread_attrs_detached,(void *(*)(void *))prefetch_irritator, (void *)&th_array[index]) ) {
							/* If create threads fails, perform cleanup and return fail(-1) to calling function */
							sprintf(msg, "Prefetch pthread_create call failed with %d \n", errno);
							hxfmsg(&h_d, errno, HTX_HE_HARD_ERROR, msg);
							sprintf(msg,"[%d] calling cleanup prefetch \n",__LINE__);
							hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
							if ( (rc = cleanup_threads(PREFETCH))!= SUCCESS) {
								sprintf(msg, "[%d] Cleaning up of Prefetch pthreads failed \n", __LINE__);
								hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
								return (FAILURE);
							}
							return (FAILURE);
						}										/* end of if			 */
						else {
							mem.prefetch_threads++;
							current_rule->num_prefetch_threads_created++;
						}										/* end of else		   */
					}											/* end of inner for loop */
					i += smt;
				}												/* End of outer for loop */ 
				DEBUG_LOG("\n[%d] Info: Prefetch threads created = %d",__LINE__,current_rule->num_prefetch_threads_created);
			}
			break;

		case CACHE_BOUNCE_ONLY:
		case CACHE_ROLL_ONLY:
			rc = E_WRONG_TEST_CASE;
			current_rule->num_prefetch_threads_created = 0;
			sprintf(msg,"[%d] Error: Current testcase (%d) does not support prefetch. Exitting! \n",__LINE__,testcase_type);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return (rc);

		default:
			rc = E_INVALID_TEST_CASE;
			sprintf(msg,"[%d] Error: Invalid testcase_type = %d\n",__LINE__,testcase_type);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			break;
	}

	return (rc);
}

int create_cache_threads(void) {
	int rc 				= SUCCESS;
	int testcase_type 	= current_rule->testcase_type;
	int	index			= 0;
	int	core 			= 0;
	int num_cores		= system_information.num_cores;
    int num_cache_threads = current_rule->num_cache_threads_created;
    int thd;
	switch(testcase_type) {
		case CACHE_BOUNCE_ONLY :
		case CACHE_BOUNCE_WITH_PREF:
		case CACHE_ROLL_WITH_PREF:
		case CACHE_ROLL_ONLY:
			/* If Testcase type is CACHE_BOUNCE / ROLLOVER with prefetch on, 	*/
			/* then one instance of cache thread runs per core.					*/
		  
			if(!exit_flag ) {
                for(core=0,thd=0;thd<num_cache_threads;thd++,core++){
                    if(system_information.pvr == POWER9_CUMULUS){
                       if(core == num_cores){
                            core = 0;
                            index = (system_information.core_smt_array[core]/2);
                        }
                    }
					if ( core_to_exclude(&(current_rule->exclude_cores[0]), system_information.cores_in_instance[core]) == TRUE ) {
						continue;
					}
					rc = pthread_create(&th_array[index].tid, &th_array[index].thread_attrs_detached,(void *(*)(void *))write_read_and_compare, (void *)&th_array[index]);
					if ( rc != 0 ) {
						/* If create threads fails, perform cleanup and return fail(-1) to calling function */
						sprintf(msg, "pthread_create failed with %d \n", rc);
						hxfmsg(&h_d, errno, HTX_HE_HARD_ERROR, msg);
						sprintf(msg,"[%d] Calling cleanup ALL\n",__LINE__);
						hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
						if ( (rc = cleanup_threads(ALL)) != SUCCESS) {
							sprintf(msg, "[%d] Cleaning up of cache and prefetch threads failed \n", __LINE__);
							hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
							return (FAILURE);
						}
						return (FAILURE);
					}
					else {
						mem.cache_threads++;
						current_rule->num_cache_threads_created++;
					}

					/*index += get_smt_status(system_information.cores_in_instance[core]);*/
					index += system_information.core_smt_array[core];
					if(gang_size == 1){/*In case of equiliser only one thread need to be created*/
						break;
					}
				}
				DEBUG_LOG("\n[%d] Info:  Total cache threads created = %d\n",__LINE__,current_rule->num_cache_threads_created); 
			}
			break;
#if 0
		case CACHE_ROLL_ONLY:
				/* If Prefetch is off AND If Testcase type is CACHE_ROLLOVER,
			 	* then increment index by 1
			 	*/
				if( !exit_flag ) {
					for ( core=0 ; core < num_cores; core++ ) {
						if ( core_to_exclude(&(current_rule->exclude_cores[0]), system_information.cores_in_instance[core]) == TRUE ) {
							continue;
						}

						for(j=0; j < system_information.core_smt_array[core]/*get_smt_status(system_information.cores_in_instance[core])*/ ; j++) { 
							if(pthread_create(&th_array[index].tid, &th_array[index].thread_attrs_detached,(void *(*)(void *))write_read_and_compare, (void *)&th_array[index]) ) {
					   		
								/* If create threads fails, perform cleanup and return fail(-1) to calling function */
					   			sprintf(msg, "pthread_create failed with %d \n", errno);
					   			hxfmsg(&h_d, errno, HTX_HE_HARD_ERROR, msg);
					   			sprintf(msg,"[%d] Calling cleanup ALL\n",__LINE__);
					   			hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
								if ( (rc = cleanup_threads(ALL)) != SUCCESS) {
									sprintf(msg, "[%d] Cleaning up of cache and prefetch threads failed \n", __LINE__);
									hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
									return (FAILURE);
								}
								return (FAILURE);
				   			} else {
					   			mem.cache_threads++;
								current_rule->num_cache_threads_created++;
				   			}
							index++;
			   			}

					}
					DEBUG_LOG("\n[%d] Info:  Total cache threads created = %d\n",__LINE__,current_rule->num_cache_threads_created); 
		   		}
				break;
#endif
		case PREFETCH_ONLY:
			rc = E_WRONG_TEST_CASE;
			current_rule->num_cache_threads_created = 0;
			sprintf(msg,"[%d] Error: Current testcase (%d) does not support cache threads. Exitting! \n",__LINE__,testcase_type);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			return rc;

		default:
			rc = E_INVALID_TEST_CASE;
			sprintf(msg,"[%d] Error: Invalid testcase_type = %d\n",__LINE__,testcase_type);
			hxfmsg(&h_d, rc, HTX_HE_HARD_ERROR, msg);
			break;
	}

	return (rc);
}

int create_threads(int thread_type) {
	int rc = SUCCESS;

	switch(thread_type) {
		case PREFETCH:
			if ( (rc = create_prefetch_threads()) != SUCCESS ) {
				sprintf(msg,"[%d] Error in creating prefetch thread \n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
			}
			break;
		case CACHE:
			if ( (rc = create_cache_threads()) != SUCCESS ) {
				sprintf(msg,"[%d] Error in creating cache thread \n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
			}
			break;
		case ALL:
			if ( (rc = create_threads(PREFETCH)) != SUCCESS ) {
				sprintf(msg,"[%d] Error in creating prefetch thread \n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
			}
			if ( (rc = create_threads(CACHE)) != SUCCESS ) {
				sprintf(msg,"[%d] Error in creating cache thread \n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
			}
			break;
		default:
			sprintf(msg,"[%d] Invalid thread type = %d\n",__LINE__,thread_type);
			hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
			rc = FAILURE;
			break;
	}

	mem.total_threads = mem.cache_threads + mem.prefetch_threads;

	return (rc);
}

int cleanup_threads(int thread_type) {
	int thread_no , rc = SUCCESS, th_rc;
	void *tresult = (void *)&th_rc ;
	int num_cache_threads 	= current_rule->num_cache_threads_created;
	int num_prefetch_threads = current_rule->num_prefetch_threads_created;

	switch(thread_type) {

	case PREFETCH:
		if( num_prefetch_threads == 0) {
			PRINT_LOG("\n[%d] No prefetch threads to be cleaned up\n",__LINE__);
			return (rc);
		}
		else {
			for(thread_no = 0; thread_no< total_cpus; thread_no++) {
				if(th_array[thread_no].thread_type != PREFETCH) {
					continue;
				}

#if defined(__HTX_LINUX__) && defined(AWAN)
				sprintf(msg,"[%d] Joining prefetch thread_no %d, pthread id : 0x%x \n",__LINE__,thread_no,th_array[thread_no].tid);
	            hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
#endif
				rc = pthread_join(th_array[thread_no].tid, &tresult);
				if (rc != 0) {
				sprintf(msg,"[%d]cleanup_threads:pthread_join failed ! errno %d :(%d): tnum=%d\n",
							__LINE__,errno, rc, th_array[thread_no].tid);
					hxfmsg(&h_d,rc, HTX_HE_HARD_ERROR , msg);
					return (FAILURE);
				} else {
					th_array[thread_no].tid = -1;
					th_array[thread_no].thread_type = ALL;

					PRINT_LOG("\n[%d] Prefetch Thread %d Just Joined\n",__LINE__,thread_no);
				}
			}
		}
		break;

	case CACHE:
		if ( num_cache_threads == 0 ) {
			PRINT_LOG("\n[%d] No cache threads to cleanup!!",__LINE__);
		}
		else {
			for(thread_no = 0; thread_no < total_cpus; thread_no++){
				if(th_array[thread_no].thread_type != CACHE){
					continue;
				}
				else {
#if defined(__HTX_LINUX__) && defined(AWAN)
					sprintf(msg,"[%d] Joining cache thread_no %d, pthread id : 0x%x \n",__LINE__,thread_no,th_array[thread_no].tid);
	            	hxfmsg(&h_d, 0, HTX_HE_INFO, msg);
#endif
					rc = pthread_join(th_array[thread_no].tid, &tresult);
					if ((rc != 0) && (rc != ESRCH) ) {
						sprintf(msg,"[%d] cleanup_threads:pthread_join failed ! errno %d :(%d): tnum=%x\n",
									__LINE__,errno, rc, th_array[thread_no].tid);
						hxfmsg(&h_d, HTX_HE_HARD_ERROR , rc , msg);
						return (FAILURE);
					}
					else {
						th_array[thread_no].tid 		= -1;
						th_array[thread_no].thread_type = ALL;
						if ( rc != ESRCH ) {
							PRINT_LOG("\n[%d] Cache Thread %d Just Joined\n",__LINE__,thread_no);
						}
					}
				}
		 	}
		}
		break;

	case ALL:
		PRINT_LOG("\n[%d] calling cleanup prefetch \n",__LINE__);
		if (( rc = cleanup_threads(PREFETCH)) != SUCCESS) {
			sprintf(msg,"[%d] Error in cleaning up prefetch threads\n",__LINE__);
			hxfmsg(&h_d, rc, HTX_HE_INFO , msg);
			return (FAILURE);
		}
		PRINT_LOG("\n[%d] calling cleanup cache \n",__LINE__);
		if (( rc = cleanup_threads(CACHE)) != SUCCESS) {
			sprintf(msg,"[%d] Error in cleaning up prefetch threads\n",__LINE__);
			hxfmsg(&h_d, rc, HTX_HE_INFO , msg);
			return (FAILURE);
		}
		break;
	}

	return (rc);
}

int derive_prefetch_algorithm_to_use(int index) {

	int rc 		= FAILURE;
	int pvr 	= system_information.pvr;
	int pf_conf	= current_rule->pf_conf;
	int prefetch_algorithms[MAX_PREFETCH_ALGOS] = { PREFETCH_IRRITATOR , PREFETCH_NSTRIDE ,
													PREFETCH_PARTIAL, PREFETCH_TRANSIENT , PREFETCH_NA };
														
	static int	prefetch_algo_counter = 0;
	int		pf_algo_to_use;

	#ifndef __HTX_LINUX__
		if((pvr == POWER6) || ((pvr == POWER7) && (_system_configuration.implementation == POWER_6))) {
			return PREFETCH_IRRITATOR;
		}
	#else
		if(pvr == POWER6) {
			return PREFETCH_IRRITATOR;
		}
	#endif
	else if(pvr >= POWER7 && pvr <= POWER8_VENICE) {

		if(system_information.smt_threads == 2) {
			return RR_ALL_ENABLED_PREFETCH_ALGORITHMS;
		}
		else /*if(system_information.smt_threads == 4)*/ {

			if ( pf_conf == PREFETCH_OFF ) {
				return PREFETCH_OFF;
			} else {
				while ( 1 ) {
					pf_algo_to_use = pf_conf & prefetch_algorithms[ prefetch_algo_counter++ % MAX_PREFETCH_ALGOS ];
					if ( pf_algo_to_use ) {
						return pf_algo_to_use;
					} 
				}
			} /* End of switch case */
		} /* End of else if */
	} /* End of else if */

	return rc;
}

int get_next_cpu(int thread_type, int test_case, int thread_no, int core_no) {
	int cpu = FAILURE;
	int smt,i, index_in_core,core_num,p9_cumulus_flag=0;
	static int core_indexi_count[MAX_CORES_PER_CHIP] = {[0 ... (MAX_CORES_PER_CHIP -1)] = 1};
    static int cache_thread_count = 0;

    if(system_information.pvr == POWER9_CUMULUS){
        p9_cumulus_flag = 1;
    }
	core_num									= system_information.cores_in_instance[core_no];
	if ( core_to_exclude(&(current_rule->exclude_cores[0]), core_num) == TRUE ) {
		DEBUG_LOG("[%d] Core %d is in exclude list, hence exitting \n",__LINE__, core_no);
		return (E_CORE_EXCLUDED);
	} else {
		/*smt = get_smt_status(core_no);*/
		smt = system_information.core_smt_array[core_no];
		if(thread_type == CACHE) {
            if(cache_thread_count >= system_information.num_cores){
                index_in_core = (smt/2);
            }else{
			    index_in_core = 0;
            }
            if(p9_cumulus_flag){
                cache_thread_count++;
                if(cache_thread_count == (2*system_information.num_cores)){
                    cache_thread_count = 0;
                }
            }
		}
		else {
			index_in_core = (core_indexi_count[core_no]++);/* Hotplug changes */
            if((p9_cumulus_flag) && (index_in_core == (smt/2))){
                index_in_core = (core_indexi_count[core_no]++);
            }
			if(core_indexi_count[core_no]  == smt) {
				core_indexi_count[core_no] = 1;
			}
		}
		cpu = system_information.cpus_in_core_array[core_no][index_in_core];
		DEBUG_LOG("[%d]system_information.cpus_in_core_array[%d][%d]=%d \n",__LINE__,core_no,index_in_core,cpu);
		if( gang_size == 1){
			cpu = system_information.instance_number;
		}
		DEBUG_LOG("thread_no=%d(th type = %d), core_no = %d, cpu = %d\n,index_in_core=%d\n",thread_no,thread_type,core_no,cpu,index_in_core);
	}

	return cpu;
}

int is_empty_list(int list[], int list_size) {

	int rc 		= FALSE;
	int count 	= 0;

	for ( count=0 ; count<list_size ; count++ ) {
		if ( list[count] != -1 ) {
			rc = TRUE;
			break;
		}
	}

	return rc;
}

int get_num_threads(int instance_number, int test_case, int type, struct ruleinfo *rule) {
	int core,p9_cumulus_flag=0;
	int num_threads = FAILURE;
	int smt 		= 0;

    if(system_information.pvr == POWER9_CUMULUS){
        p9_cumulus_flag = 1;
    }
	if ( (rule != NULL) || (is_empty_list(&(rule->exclude_cores[0]), MAX_CORES_PER_CHIP) != TRUE) ) {
		num_threads = get_num_threads_for_rule(instance_number, test_case, type, rule);
	} else {
		if( type == PREFETCH ) {
			switch ( test_case ) {
				case PREFETCH_ONLY :														  /* If cache test is off, then all the threads run as Prefetch threads.	*/
					num_threads = system_information.number_of_logical_cpus;
					break;

				case CACHE_BOUNCE_ONLY:
				case CACHE_ROLL_ONLY:
					num_threads = 0;
					break;

				case CACHE_BOUNCE_WITH_PREF:
				case CACHE_ROLL_WITH_PREF:
					for( core=0 ; core<system_information.num_cores ; core++ ) {							 /* If cache test is on with Prefetch feature enabled, then  */
						/*smt = get_smt_status(system_information.cores_in_instance[core]);*/
						smt = system_information.core_smt_array[core];
						if (smt == -1) {
							sprintf(msg,"[%d] syscfg library returned -1. Exitting !!",__LINE__);
							hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
							return (FAILURE);
						}
						num_threads += ( smt - 1 - p9_cumulus_flag);														/* we reserve one thread from each core for cache and other */
					}																					/* remaining threads are reserved for running prefetch.	 */
					break;

				default:
					sprintf(msg,"[%d] Wrong Testcase type (%d) \n!!",__LINE__,test_case);
					hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
					return (FAILURE);
			}
		} else if( type == CACHE ) {
			switch ( test_case ) {
				case CACHE_BOUNCE_ONLY:
				case CACHE_BOUNCE_WITH_PREF:
				case CACHE_ROLL_ONLY:																					/* If running ROLLOVER with no prefetch, then all threads run as CACHE threads */
					for( core=0 ; core<system_information.num_cores ; core++ ) {								 /* In case number of cache threads is required, we count one */
						/*smt = get_smt_status(system_information.cores_in_instance[core] );*/
						smt = system_information.core_smt_array[core];
						if (smt == -1) {
							sprintf(msg,"[%d] syscfg library returned -1. Exitting !!",__LINE__);
							hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
							return (FAILURE);
						}
						if( smt >= 1){																			/* thread for every core in the chip, which has atleast one  */
						    num_threads += (p9_cumulus_flag + 1);				
                        }
					}
					break;
				
				case CACHE_ROLL_WITH_PREF:
					for( core=0 ; core<system_information.num_cores ; core++ ) {						/* If running ROLLOVER with prefetch, then we	*/
						if( system_information.core_smt_array[core] >= 1){								/* count one thread for every core in chip	   */
						    num_threads += (p9_cumulus_flag + 1);				
                        }
					}
					break;

				case PREFETCH_ONLY:
					num_threads = 0;
					break;

				default:
					sprintf(msg,"[%d] Wrong Testcase type (%d) \n!!",__LINE__,test_case);
					hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
					return (FAILURE);
			}
		}
	}

	return num_threads;
}

int core_to_exclude(int core_list[], int core_num) {

	int rc = FALSE;
	int count = 0;

	for ( count=0 ; count<MAX_CORES_PER_CHIP ; count++ ) {
		if ( core_list[count] == core_num ) {
			rc = TRUE;
			break;
		}
	}

	return (rc);
}

int get_num_threads_for_rule(int instance_number, int test_case, int type, struct ruleinfo * rule) {
	int core,p9_cumulus_flag=0;
	int num_threads = 0;
	int smt 		= 0;
  
    if(system_information.pvr == POWER9_CUMULUS){
        p9_cumulus_flag=1;
    }
	if( type == PREFETCH ) {
		switch ( test_case ) {
			case PREFETCH_ONLY :
				for( core=0 ; core<system_information.num_cores ; core++ ) {							 /* If cache test is on with Prefetch feature enabled, then  */
					/*smt = get_smt_status(system_information.cores_in_instance[core]);*/
					smt = system_information.core_smt_array[core];
					if (smt == -1) {
						sprintf(msg,"[%d] syscfg library returned -1. Exitting !!",__LINE__);
						hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
						return (FAILURE);
					}
					if ( core_to_exclude(&(rule->exclude_cores[0]), system_information.cores_in_instance[core]) == FALSE )
						num_threads += smt; 
				}
				break;

			case CACHE_BOUNCE_ONLY:																		/* If Prefetch is not enabled, return 0	*/
			case CACHE_ROLL_ONLY:
				num_threads = 0;
				break;

			case CACHE_BOUNCE_WITH_PREF:																/* In case Prefetch is enabled, then we count smt-1 threads	*/
			case CACHE_ROLL_WITH_PREF:																	/* per core as prefetch threads.							*/
				for( core=0 ; core<system_information.num_cores ; core++ ) {							
					smt = system_information.core_smt_array[core];
					/*smt = get_smt_status(system_information.cores_in_instance[core]);*/
					if (smt == -1) {
						sprintf(msg,"[%d] syscfg library returned -1. Exitting !!",__LINE__);
						hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
						return (FAILURE);
					}
					if ( core_to_exclude(&(rule->exclude_cores[0]), system_information.cores_in_instance[core]) == FALSE ) {
						DEBUG_LOG("\n[%d]Number of cores = %d",__LINE__,system_information.num_cores);
						num_threads += (smt - 1 - p9_cumulus_flag); 
					}
				}
				break;

			default:
				sprintf(msg,"[%d] Wrong Testcase type (%d) \n!!",__LINE__,test_case);
				hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
		}
	} else if ( type == CACHE ) {
		switch ( test_case ) {
			case PREFETCH_ONLY :																		/* If Cache test is off, then return 0.	*/
				num_threads = 0;
				break;

			case CACHE_BOUNCE_ONLY:																		/* In case of Cache Bounce test case, or rollover with prefetch	*/
			case CACHE_BOUNCE_WITH_PREF:																/* we count 1 thread per core for running cache test case.		*/
			case CACHE_ROLL_WITH_PREF:
			case CACHE_ROLL_ONLY:																		/* In case of Prefetch testcase with Prefetch OFF,		*/
				for( core=0 ; core<system_information.num_cores ; core++ ) {							
					smt = system_information.core_smt_array[core];
					/*smt = get_smt_status(system_information.cores_in_instance[core]);*/
					if (smt == -1) {
						sprintf(msg,"[%d] syscfg library returned -1. Exitting !!",__LINE__);
						hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
						return (FAILURE);
					}
					if ( core_to_exclude(&(rule->exclude_cores[0]), system_information.cores_in_instance[core]) == FALSE )
						num_threads +=(p9_cumulus_flag + 1); 
				}
				break;
#if 0
			case CACHE_ROLL_ONLY:																		/* In case of Prefetch testcase with Prefetch OFF,		*/
				for( core=0 ; core<system_information.num_cores ; core++ ) {							/* we count all threads for running cache test case.	*/
					smt = system_information.core_smt_array[core];
					/*smt = get_smt_status(system_information.cores_in_instance[core]);*/
					if (smt == -1) {
						sprintf(msg,"[%d] syscfg library returned -1. Exitting !!",__LINE__);
						hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
						return (FAILURE);
					}
					if ( core_to_exclude(&(rule->exclude_cores[0]), system_information.cores_in_instance[core]) == FALSE )
						num_threads += smt; 
				}
				if (num_threads > 16 && system_information.pvr == POWER7P) {
					printf("[%d] Num threads = %d\n",__LINE__,num_threads);
					num_threads = get_num_threads(instance_number, CACHE_BOUNCE_ONLY, type, rule);	
					printf("[%d] Num threads = %d\n",__LINE__,num_threads);
				}
				break;										
#endif
			default:
				sprintf(msg,"[%d] Wrong Testcase type (%d) \n!!",__LINE__,test_case);
				hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
				return (FAILURE);
		}

	} else {
		sprintf(msg,"[%d] Wrong thread type (%d) \n!!",__LINE__,type);
		hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
		return (FAILURE);
	}

	return (num_threads);
}

/*
 * Function to parse rule_id which ends with a stanza number 	
 * and returns the stanza number 								
 *
 * Input  = rule id string										
 * Output = stanza number (appended at the end of the string)	
 */

int get_test_id(char *rule_id) {
	int stanza_num;
	char *tmp;

	tmp = strdup(rule_id);
	strsep(&tmp,"_");
	stanza_num = atoi(tmp);

	DEBUG_LOG("stanza_num = %d\n",stanza_num);
	if (tmp) free(tmp);
	return (stanza_num);
}

/********************************************************************************************************/
/* This function is used to synchronize threads while running.											*/
/* Each thread while entering this function, increases the count by 1 mod number of threads				*/
/* This way the count goes from 1 to num_cache_threads and then wraps back to 0.						*/
/* As long as the count is not equal to number of cache threads, they block.							*/
/* If the count at any time equals num_cache_threads, that threads signals all other threads to unblock.*/
/* This way it is ensured that all the threads at synched before they continue.							*/
/*																										*/
/* INPUT = thread index																					*/
/* OUTPUT = none																						*/
/********************************************************************************************************/

int synchronize_threads(int index) {
	int rc = SUCCESS;
	static int count = 0;
	int num_cache_threads = current_rule->num_cache_threads_to_create;

	rc = pthread_mutex_lock(&sync_mutex);
	if (rc != 0) {
		sprintf(msg,"%s :[%d] Could not acquire mutex \n",__FUNCTION__,__LINE__);
		hxfmsg(&h_d, errno, HTX_HE_SOFT_ERROR, msg);
	}

	count++;
	count = count % num_cache_threads;
	if ( count == 0 || exit_flag != 0) {
		rc = pthread_cond_broadcast(&sync_cond);
		if (rc != 0 ) {
			sprintf(msg,"%s :[%d] Error in broadcasting condition, error = %d \n",__FUNCTION__,__LINE__, rc);
			hxfmsg(&h_d, rc, HTX_HE_SOFT_ERROR, msg);
			return (rc);
		}
	}else {
		rc = pthread_cond_wait(&sync_cond, &sync_mutex);
		if (rc != 0 ) {
			sprintf(msg,"%s :[%d] Error in condition waiting, error = %d \n",__FUNCTION__,__LINE__,rc);
			hxfmsg(&h_d, errno, HTX_HE_SOFT_ERROR, msg);
			return (rc);
		}
	}

	rc = pthread_mutex_unlock(&sync_mutex);
	if (rc != 0) {
		sprintf(msg,"%s :[%d] Could not release mutex, error = %d \n",__FUNCTION__,__LINE__,rc);
		hxfmsg(&h_d, errno, HTX_HE_SOFT_ERROR, msg);
	}

	return rc;
}

/****************************************************************************/
/*  This function is used to dump important data in case of any miscompare 	*/
/*  PARAMS :																*/
/*	@int thread_index = this is the thread number spawned					*/
/*  @char *addr		  = this is the character pointer to the address where	*/
/*						the miscompare is seen first.						*/
/*  RETURN VALUE :															*/
/*  -1  if some error happens												*/
/*   0  on success.															*/
/****************************************************************************/

int dump_miscompare_data(int thread_index, void *addr) {

	int rc = 0;
	char dump_file_name[256];
	FILE *dump_fp;
	int page_found = FALSE;
	int i;
	
	strcpy(dump_file_name,h_d.htx_exer_log_dir);
	sprintf(dump_file_name,"%s/hxecache_miscompare.%d.%d.%lx",dump_file_name,instance_number,thread_index,th_array[thread_index].seedval);
	dump_fp = fopen(dump_file_name,"w");
	
	if ( dump_fp == NULL ) {
		sprintf(msg,"%s:[%d] Could not create the dump file. errno = %d\n",__FUNCTION__,__LINE__,errno);
		hxfmsg(&h_d, errno, HTX_HE_SOFT_ERROR, msg);
		return (-1);
	}
		
	fprintf(dump_fp,"#############################################################################################\n");
	fprintf(dump_fp,"####################          Hxecache Miscompare Dump File            ######################\n");
	fprintf(dump_fp,"#############################################################################################\n");
	
	fprintf(dump_fp,"Exerciser instance             = %d\n",instance_number);
	fprintf(dump_fp,"Thread number                  = 0x%x\n",thread_index);
	fprintf(dump_fp,"Bind to CPU                    = %d\n",th_array[thread_index].bind_to_cpu);
	fprintf(dump_fp,"Test case id                   = %d\n",th_array[thread_index].tc_id);
	fprintf(dump_fp,"Pattern                        = 0x%llx\n",th_array[thread_index].pattern);
	fprintf(dump_fp,"Current loop number            = %d\n",th_array[thread_index].current_oper);
	fprintf(dump_fp,"Thread type                    = %d\n",th_array[thread_index].thread_type);
	fprintf(dump_fp,"Prefetch algorithm             = %d\n",th_array[thread_index].prefetch_algorithm);
	if ( th_array[thread_index].thread_type == 2 ) {
		fprintf(dump_fp,"DSCR value written             = %llx\n", th_array[thread_index].written_dscr_val);
		fprintf(dump_fp,"DSCR value read back           = %llx\n", th_array[thread_index].read_dscr_val);
	}
	fprintf(dump_fp,"Memory starting address offset = %d\n",th_array[thread_index].memory_starting_address_offset);
	fprintf(dump_fp,"Memory starting address        = %p\n\n\n",th_array[thread_index].start_of_contiguous_memory);
	fprintf(dump_fp,"Dumping the HUGE page where miscompare occurred\n\n");
	fflush(dump_fp);

	for(i=0; i<mem.num_pages; i++) {
		unsigned char *current_page_addr 	= mem.ea[i];
		unsigned char *next_page_addr		= mem.ea[i+1]; 

		if ((unsigned char *)addr >= current_page_addr && (unsigned char *)addr <= next_page_addr) {
			hexdump(dump_fp,(unsigned char *)current_page_addr,huge_page_size);
			page_found = TRUE;
		}
	}

	if ( system_information.pvr >= POWER8_MURANO ) {
		printf("[%d] Looking in Prefetch area for miscompares \n",__LINE__);
		if ( (unsigned char *) addr >= mem.prefetch_4k_memory && (unsigned char *) addr <= (mem.prefetch_4k_memory + current_rule->num_prefetch_threads_to_create*PREFETCH_MEM_SZ) ) {
			hexdump(dump_fp,(unsigned char *)addr, 512) ;
			page_found = TRUE;
		}
	}
	if (page_found == FALSE) {
		sprintf(msg,"[%d] Error : The miscomparing address %p was not found in allocated memory range. Exitting!! \n",__LINE__,addr);
		hxfmsg(&h_d, errno, HTX_HE_SOFT_ERROR, msg);
		fflush(dump_fp);
		fclose(dump_fp);
		return (-1);
	}	

	fflush(dump_fp);
	fclose(dump_fp);

	/*print_once_thread_data(current_rule);*/

	/*dump_rule_structure(current_rule);*/

	sprintf(msg,"Dump file generated : %s\n",dump_file_name);
	hxfmsg(&h_d, errno, HTX_HE_INFO, msg);
	return rc;		
}

void hexdump(FILE *f,const unsigned char *s,int l)
{
	int n=0;

	for( ; n < l ; ++n) {
		if((n%16) == 0)
			fprintf(f,"\n%p",(s+(n%16)));
		fprintf(f," %02x",*s++);
	}
	fprintf(f,"\n");
}

int find_memory_requirement(void) {
	struct ruleinfo *rule = &h_r[0];
	int 			i, rc = SUCCESS;
	int				tgt_cache;
	int				pvr = system_information.pvr;
	int				mem_needed;

	tgt_cache = rule->tgt_cache;
	for ( i=1; i<=num_testcases; i++ ) {
		switch( pvr ) {
			case POWER7:
				mem_needed = calculate_mem_requirement_for_p7(rule) ;
				if ( mem_needed < 0 ) {
					sprintf(msg,"[%d] Error in memory requirement calculation \n",__LINE__);
					hxfmsg(&h_d, mem_needed, HTX_HE_HARD_ERROR, msg);
					return (mem_needed);
				}
				break;
			case POWER7P:
				mem_needed = calculate_mem_requirement_for_p7p(rule) ;
				if ( mem_needed < 0 ) {
					sprintf(msg,"[%d] Error: Unable to calculate memory requirement \n",__LINE__);
					hxfmsg(&h_d, mem_needed, HTX_HE_HARD_ERROR, msg);
					return (mem_needed);
				}
				break;
			case POWER8_MURANO:
			case POWER8_VENICE:
			case POWER8_PRIME:
				mem_needed = calculate_mem_requirement_for_p8(rule);
				if ( mem_needed < 0 ) {
					sprintf(msg,"[%d] Error: Unable to calculate memory requirement \n",__LINE__);
					hxfmsg(&h_d, mem_needed, HTX_HE_HARD_ERROR, msg);
					return (mem_needed);
				}
				break;
            case POWER9_NIMBUS:
            case POWER9_CUMULUS:
                mem_needed = calculate_mem_requirement_for_p9(rule);
                if ( mem_needed < 0 ) {
                    sprintf(msg,"[%d] Error: Unable to calculate memory requirement \n",__LINE__);
                    hxfmsg(&h_d, mem_needed, HTX_HE_HARD_ERROR, msg);
                    return (mem_needed);
                }
                break;
                
			default:
				sprintf(msg,"[%d] Error: Unknown PVR (0x %x). Exitting \n",__LINE__,pvr);
				hxfmsg(&h_d, E_UNKNOWN_PVR, HTX_HE_HARD_ERROR, msg);
				return (E_UNKNOWN_PVR);
				break;
		}

		rule++;
	}

	return (rc);
}
int calculate_mem_requirement_for_p9(struct ruleinfo *rule_ptr) {

    int tc_type                             = rule_ptr->testcase_type;
    int target_cache                        = rule_ptr->tgt_cache;
    int cache_size;
    int worst_case_cache_memory_required    = -1;
    int worst_case_contiguous_memory_pages_required = -1;
    int worst_case_prefetch_memory_required = -1;
    int testcase                            = rule_ptr->testcase_type;
    int num_prefetch_threads;
    int num_cache_threads;

    rule_ptr->num_prefetch_threads_to_create    = get_num_threads(system_information.instance_number, testcase, PREFETCH, rule_ptr);
    rule_ptr->num_cache_threads_to_create       = get_num_threads(system_information.instance_number, testcase, CACHE, rule_ptr);

    if(gang_size == 1){
        num_cache_threads = 1;
    }
    else{
        num_cache_threads                       = rule_ptr->num_cache_threads_to_create;
    }
    num_prefetch_threads                        = rule_ptr->num_prefetch_threads_to_create;



    switch(tc_type) {
        case CACHE_BOUNCE_ONLY :
            if ( num_cache_threads != 0 ) {
                worst_case_cache_memory_required                    = huge_page_size;
                cache_page_req                                      = (worst_case_cache_memory_required/(huge_page_size));
                rule_ptr->cache_HUGE_pages_required                  = cache_page_req;
                rule_ptr->cont_memory_pool.contiguous_mem_required  = huge_page_size;
                rule_ptr->cont_memory_pool.num_sets                 = 1;
            } else {
                rule_ptr->cache_HUGE_pages_required                  = 0;
                rule_ptr->cont_memory_pool.contiguous_mem_required  = 0;
                rule_ptr->cont_memory_pool.num_sets                 = 0;
            }
            rule_ptr->prefetch_HUGE_pages_required               = 0;
            prefetch_page_req                                   = 0;
            rule_ptr->cont_memory_pool.prefetch_sets            = 0;
            break;

        case CACHE_BOUNCE_WITH_PREF :
            if ( num_cache_threads != 0 ) {
            worst_case_cache_memory_required                    = huge_page_size;
            cache_page_req                                      = (worst_case_cache_memory_required/(huge_page_size));
            rule_ptr->cache_HUGE_pages_required                  = cache_page_req;
            rule_ptr->cont_memory_pool.contiguous_mem_required  = huge_page_size;
            rule_ptr->cont_memory_pool.num_sets                 = 1;
            } else {
                rule_ptr->cache_HUGE_pages_required                  = 0;
                rule_ptr->cont_memory_pool.contiguous_mem_required  = 0;
                rule_ptr->cont_memory_pool.num_sets                 = 0;
            }
            prefetch_page_req                                   = (num_prefetch_threads * (8*M))/(huge_page_size) + ((num_prefetch_threads * (8*M))%(huge_page_size) == 0 ? 0:1 ) ;
            rule_ptr->prefetch_HUGE_pages_required               = prefetch_page_req;
            worst_case_prefetch_memory_required                 = (prefetch_page_req) * (huge_page_size);
            rule_ptr->cont_memory_pool.prefetch_sets            = num_prefetch_threads;
            break;

        case CACHE_ROLL_WITH_PREF:
        case CACHE_ROLL_ONLY :
            if ( num_cache_threads != 0 ) {
                cache_size                                          = system_information.cinfo[target_cache].cache_size;
                rule_ptr->cont_memory_pool.contiguous_mem_required  = cache_size;
                worst_case_contiguous_memory_pages_required         = (rule_ptr->cont_memory_pool.contiguous_mem_required / (huge_page_size)) + ((rule_ptr->cont_memory_pool.contiguous_mem_required % (huge_page_size)) == 0 ? 0:1);
                cache_page_req                                      = worst_case_contiguous_memory_pages_required * num_cache_threads;
                rule_ptr->cache_HUGE_pages_required                  = cache_page_req;
                rule_ptr->cont_memory_pool.num_sets                 = num_cache_threads;
            }else{
                rule_ptr->cont_memory_pool.contiguous_mem_required  = 0;
                rule_ptr->cache_HUGE_pages_required                  = 0;
                rule_ptr->cont_memory_pool.num_sets                 = 0;
            }
            prefetch_page_req                                   = (num_prefetch_threads * (8*M))/(huge_page_size) + ((num_prefetch_threads * (8*M))%(huge_page_size) == 0 ? 0:1 ) ;
            rule_ptr->prefetch_HUGE_pages_required               = prefetch_page_req;
            worst_case_prefetch_memory_required                 = (prefetch_page_req) * (huge_page_size);
            worst_case_cache_memory_required                    = cache_page_req * (huge_page_size);
            rule_ptr->cont_memory_pool.prefetch_sets            = num_prefetch_threads;
            DEBUG_LOG("*******[%d] prefetch pages reqd = %d, cache pages reqd = %d, cache threads = %d, prefetch threads = %d\n",
                    __LINE__,rule_ptr->prefetch_HUGE_pages_required,rule_ptr->cache_HUGE_pages_required,num_cache_threads, num_prefetch_threads);
            break;

        case PREFETCH_ONLY :
            worst_case_cache_memory_required                    = 0;
            cache_page_req                                      = 0;
            prefetch_page_req                                   = (num_prefetch_threads * (8*M))/(huge_page_size) + ((num_prefetch_threads * (8*M))%(huge_page_size) == 0 ? 0:1 ) ;
            rule_ptr->prefetch_HUGE_pages_required               = prefetch_page_req;
            worst_case_prefetch_memory_required                 = prefetch_page_req * (huge_page_size);
            rule_ptr->cont_memory_pool.contiguous_mem_required  = 0;
            rule_ptr->cont_memory_pool.num_sets                 = 0;
            rule_ptr->cont_memory_pool.prefetch_sets            = num_prefetch_threads;
            break;

        default:
            sprintf(msg,"[%d] Error: Invalid testcase type combination (%d) \n",__LINE__, testcase);
            hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
            rule_ptr->num_prefetch_threads_to_create            = 0;
            rule_ptr->num_cache_threads_to_create               = 0;
            break;
    }
    DEBUG_LOG("[%d] cache pages required = %d, contiguous mem sets required = %d\n",__LINE__,rule_ptr->cache_HUGE_pages_required,rule_ptr->cont_memory_pool.num_sets);
    rule_ptr->total_HUGE_pages_required = rule_ptr->cache_HUGE_pages_required + rule_ptr->prefetch_HUGE_pages_required;
    return ( worst_case_cache_memory_required + worst_case_prefetch_memory_required );
}
int calculate_mem_requirement_for_p7(struct ruleinfo *rule_ptr) {
	
	int	tc_type 							= rule_ptr->testcase_type;
	int target_cache						= rule_ptr->tgt_cache;
	int cache_size;
	int	worst_case_cache_memory_required	= -1;
	int	worst_case_contiguous_memory_pages_required	= -1;
	int	worst_case_prefetch_memory_required	= -1;
	int testcase							= rule_ptr->testcase_type;
	int num_prefetch_threads;
	int num_cache_threads;

	rule_ptr->num_prefetch_threads_to_create	= get_num_threads(system_information.instance_number, testcase, PREFETCH, rule_ptr);
	rule_ptr->num_cache_threads_to_create		= get_num_threads(system_information.instance_number, testcase, CACHE, rule_ptr);
	if(gang_size == 1){
		num_cache_threads = 1;
	}
	else{
		num_cache_threads							= /*rule_ptr->num_cache_threads_to_create;*/ system_information.num_cores;
	}
	num_prefetch_threads						= rule_ptr->num_prefetch_threads_to_create;

	switch(tc_type) {
		case CACHE_BOUNCE_ONLY :
			if ( num_cache_threads != 0 ) {
				worst_case_cache_memory_required					= huge_page_size;
				cache_page_req 										= (worst_case_cache_memory_required/(huge_page_size));
				rule_ptr->cache_HUGE_pages_required					= cache_page_req;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= huge_page_size;
				rule_ptr->cont_memory_pool.num_sets					= 1;
			} else {
				rule_ptr->cache_HUGE_pages_required					= 0;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= 0;
				rule_ptr->cont_memory_pool.num_sets					= 0;
			}
			rule_ptr->prefetch_HUGE_pages_required				= 0;
			prefetch_page_req 									= 0;
			rule_ptr->cont_memory_pool.prefetch_sets			= 0;
			break;

		case CACHE_BOUNCE_WITH_PREF :
			if ( num_cache_threads != 0 ) {
			worst_case_cache_memory_required					= huge_page_size;
			cache_page_req 										= (worst_case_cache_memory_required/(huge_page_size));
			rule_ptr->cache_HUGE_pages_required					= cache_page_req;
			rule_ptr->cont_memory_pool.contiguous_mem_required	= huge_page_size;
			rule_ptr->cont_memory_pool.num_sets					= 1;
			} else {
				rule_ptr->cache_HUGE_pages_required					= 0;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= 0;
				rule_ptr->cont_memory_pool.num_sets					= 0;
			}
			prefetch_page_req									= (num_prefetch_threads * (8*M))/(huge_page_size) + ((num_prefetch_threads * (8*M))%(huge_page_size) == 0 ? 0:1 ) ;
			rule_ptr->prefetch_HUGE_pages_required				= prefetch_page_req;
			worst_case_prefetch_memory_required 				= (prefetch_page_req) * (huge_page_size);
			rule_ptr->cont_memory_pool.prefetch_sets			= num_prefetch_threads;
			break;

		case CACHE_ROLL_WITH_PREF:
		case CACHE_ROLL_ONLY :
			if ( num_cache_threads != 0 ) {
				cache_size 											= system_information.cinfo[target_cache].cache_size;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= 2*(cache_size);;
				worst_case_contiguous_memory_pages_required 		= (rule_ptr->cont_memory_pool.contiguous_mem_required / (huge_page_size)) + ((rule_ptr->cont_memory_pool.contiguous_mem_required % (huge_page_size)) == 0 ? 0:1);
				cache_page_req										= worst_case_contiguous_memory_pages_required * num_cache_threads;
				rule_ptr->cache_HUGE_pages_required					= cache_page_req;
				rule_ptr->cont_memory_pool.num_sets					= system_information.num_cores/* - rule_ptr->num_excluded_cores*/;
			} else {
				rule_ptr->cache_HUGE_pages_required					= 0;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= 0;
				rule_ptr->cont_memory_pool.num_sets					= 0;
			}
			prefetch_page_req									= (num_prefetch_threads * (8*M))/(huge_page_size) + ((num_prefetch_threads * (8*M))%(huge_page_size) == 0 ? 0:1 ) ;
			rule_ptr->prefetch_HUGE_pages_required				= prefetch_page_req;
			worst_case_prefetch_memory_required 				= (prefetch_page_req) * (huge_page_size);
			worst_case_cache_memory_required					= cache_page_req * (huge_page_size);
			rule_ptr->cont_memory_pool.prefetch_sets			= num_prefetch_threads;
			printf("*******[%d] prefetch pages reqd = %d, cache pages reqd = %d, cache threads = %d, prefetch threads = %d\n",
					__LINE__,rule_ptr->prefetch_HUGE_pages_required,rule_ptr->cache_HUGE_pages_required,num_cache_threads, num_prefetch_threads);
			break;

		case PREFETCH_ONLY :
			worst_case_cache_memory_required					= 0;
			cache_page_req 										= (worst_case_cache_memory_required/(huge_page_size));
			rule_ptr->cache_HUGE_pages_required					= cache_page_req;
			prefetch_page_req									= (num_prefetch_threads * (8*M))/(huge_page_size) + ((num_prefetch_threads * (8*M))%(huge_page_size) == 0 ? 0:1 ) ;
			rule_ptr->prefetch_HUGE_pages_required				= prefetch_page_req;
			worst_case_prefetch_memory_required					= prefetch_page_req * (huge_page_size);
			rule_ptr->cont_memory_pool.contiguous_mem_required	= 0;
			rule_ptr->cont_memory_pool.num_sets					= 0;
			rule_ptr->cont_memory_pool.prefetch_sets			= num_prefetch_threads;
			break;

		default:
			worst_case_cache_memory_required	= -1;
			cache_page_req 						= -1;
			worst_case_prefetch_memory_required	= -1;
			prefetch_page_req 					= -1;
			sprintf(msg,"[%d] Invalid testcase type combination \n",__LINE__);
			hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
			rule_ptr->num_prefetch_threads_to_create			= 0;
			rule_ptr->num_cache_threads_to_create				= 0;
			break;
	}

	rule_ptr->total_HUGE_pages_required = rule_ptr->cache_HUGE_pages_required + rule_ptr->prefetch_HUGE_pages_required;
	DEBUG_LOG("[%d] cache HUGE pages needed = %d\n",__LINE__,rule_ptr->cache_HUGE_pages_required);
	DEBUG_LOG("[%d] prefetch HUGE pages needed = %d\n",__LINE__,rule_ptr->prefetch_HUGE_pages_required);
	DEBUG_LOG("[%d]worst_case_cache_memory_required=%d,worst_case_prefetch_memory_required=%d\n",__LINE__,worst_case_cache_memory_required,worst_case_prefetch_memory_required);
	return ( worst_case_cache_memory_required + worst_case_prefetch_memory_required );
}

int calculate_mem_requirement_for_p7p(struct ruleinfo *rule_ptr) {

	int	worst_case_cache_memory_required			= -1;
	int	worst_case_contiguous_memory_pages_required	= -1;
	int worst_case_prefetch_memory_required			= -1;
	int testcase									= rule_ptr->testcase_type;
	int target_cache								= rule_ptr->tgt_cache;
	int cache_size;
	int num_prefetch_threads;
	int num_cache_threads;

	rule_ptr->num_prefetch_threads_to_create	= get_num_threads(system_information.instance_number, testcase, PREFETCH, rule_ptr);
	rule_ptr->num_cache_threads_to_create		= get_num_threads(system_information.instance_number, testcase, CACHE, rule_ptr);
	num_cache_threads							= /*rule_ptr->num_cache_threads_to_create;*/ system_information.num_cores;
	num_prefetch_threads						= rule_ptr->num_prefetch_threads_to_create;
	/*num_cache_threads							= rule_ptr->num_cache_threads_to_create;*/

	cache_page_req 						= -1;
	prefetch_page_req 					= -1;

	switch(testcase) {
		case CACHE_BOUNCE_ONLY :
			if ( num_cache_threads != 0 ) {
				worst_case_cache_memory_required					= huge_page_size;
				cache_page_req 										= (worst_case_cache_memory_required/(huge_page_size));
				rule_ptr->cache_HUGE_pages_required					= cache_page_req;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= huge_page_size;
				rule_ptr->cont_memory_pool.num_sets					= 1;
			} else {
				rule_ptr->cache_HUGE_pages_required					= 0;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= 0;
				rule_ptr->cont_memory_pool.num_sets					= 0;
			}
			rule_ptr->prefetch_HUGE_pages_required				= 0;
			prefetch_page_req 									= 0;
			rule_ptr->cont_memory_pool.prefetch_sets			= 0;
			break;

		case CACHE_BOUNCE_WITH_PREF :
			if ( num_cache_threads != 0 ) {
				worst_case_cache_memory_required					= huge_page_size;
				cache_page_req 										= (worst_case_cache_memory_required/(huge_page_size));
				rule_ptr->cache_HUGE_pages_required					= cache_page_req;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= huge_page_size;
				rule_ptr->cont_memory_pool.num_sets					= 1;
			} else {
				rule_ptr->cache_HUGE_pages_required					= 0;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= 0;
				rule_ptr->cont_memory_pool.num_sets					= 0;
			}
			prefetch_page_req									= (num_prefetch_threads * (8*M))/(huge_page_size) + ((num_prefetch_threads * (8*M))%(huge_page_size) == 0 ? 0:1 ) ;
			rule_ptr->prefetch_HUGE_pages_required				= prefetch_page_req;
			worst_case_prefetch_memory_required 				= (prefetch_page_req) * (huge_page_size);
			rule_ptr->cont_memory_pool.prefetch_sets			= num_prefetch_threads;
			break;

		case CACHE_ROLL_ONLY :
		case CACHE_ROLL_WITH_PREF:
			if ( num_cache_threads != 0 ) {
				cache_size 											= system_information.cinfo[target_cache].cache_size;
				rule_ptr->cont_memory_pool.contiguous_mem_required	= 2*(cache_size);;
				worst_case_contiguous_memory_pages_required 		= (rule_ptr->cont_memory_pool.contiguous_mem_required / (huge_page_size)) + ((rule_ptr->cont_memory_pool.contiguous_mem_required % (huge_page_size)) == 0 ? 0:1);
				cache_page_req										= worst_case_contiguous_memory_pages_required * num_cache_threads;
				rule_ptr->cache_HUGE_pages_required					= cache_page_req;
				rule_ptr->cont_memory_pool.num_sets					= system_information.num_cores /*- rule_ptr->num_excluded_cores*/;
			} else {
				rule_ptr->cont_memory_pool.contiguous_mem_required	= 0;
				rule_ptr->cache_HUGE_pages_required					= 0;
				rule_ptr->cont_memory_pool.num_sets					= 0;
			}
			prefetch_page_req									= (num_prefetch_threads * (8*M))/(huge_page_size) + ((num_prefetch_threads * (8*M))%(huge_page_size) == 0 ? 0:1 ) ;
			rule_ptr->prefetch_HUGE_pages_required				= prefetch_page_req;
			worst_case_prefetch_memory_required 				= (prefetch_page_req) * (huge_page_size);
			worst_case_cache_memory_required					= cache_page_req * (huge_page_size);
			rule_ptr->cont_memory_pool.prefetch_sets			= num_prefetch_threads;
			break;

		case PREFETCH_ONLY :
			worst_case_cache_memory_required					= 0;
			cache_page_req 										= 0;
			prefetch_page_req									= (num_prefetch_threads * (8*M))/(huge_page_size) + ((num_prefetch_threads * (8*M))%(huge_page_size) == 0 ? 0:1 ) ;
			rule_ptr->prefetch_HUGE_pages_required				= prefetch_page_req;
			worst_case_prefetch_memory_required					= prefetch_page_req * (huge_page_size);
			rule_ptr->cont_memory_pool.contiguous_mem_required	= 0;
			rule_ptr->cont_memory_pool.num_sets					= 0;
			rule_ptr->cont_memory_pool.prefetch_sets			= num_prefetch_threads;
			break;

		default:
			sprintf(msg,"[%d] Error: Invalid testcase type combination (%d) \n",__LINE__, testcase);
			hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
			rule_ptr->num_prefetch_threads_to_create			= 0;
			rule_ptr->num_cache_threads_to_create				= 0;
			break;
	}

	DEBUG_LOG("[%d] cache pages required = %d, contiguous mem sets required = %d\n",__LINE__,rule_ptr->cache_HUGE_pages_required,rule_ptr->cont_memory_pool.num_sets);
	rule_ptr->total_HUGE_pages_required = rule_ptr->cache_HUGE_pages_required + rule_ptr->prefetch_HUGE_pages_required;
	return ( worst_case_cache_memory_required + worst_case_prefetch_memory_required );
}

int calculate_mem_requirement_for_p8(struct ruleinfo *rule_ptr) { 
	DEBUG_LOG("[%d] calculating memory for P8 \n",__LINE__);
	return (calculate_mem_requirement_for_p7(rule_ptr));
}
int print_log(const char *format, ...) {
	time_t 	current_time;
	va_list ap;

#ifdef DEBUG
	FILE *fp = stdout;
#else
#ifdef RUNLOG_SCREEN
	FILE *fp = stdout;
#else
	FILE *fp = log_fp;
#endif
#endif


	va_start (ap, format);
	time(&current_time);

	vfprintf(fp,format,ap);
	fflush(fp);
	va_end(ap);

	return 0;
}

int get_device_instance(const char *device_name) {
    int ins_num = 0;
    int i = 0;
    if ( device_name ) {
        while (i < strlen(device_name)) {
            if(*(device_name + i) >= '0' && *(device_name + i) <= '9') {
                ins_num = ins_num*10 + *(device_name + i) - '0';
            }
            i++;
        }
    }

    return ins_num;
}

int print_once_thread_data(struct ruleinfo *rule) {

	int rc = SUCCESS;
	int i = 0,j = 0;

	DEBUG_LOG("[%d] total threads = %d, cache threads to create = %d Prefetch threads to create = %d\n",
				__LINE__,(rule->num_cache_threads_to_create + rule->num_prefetch_threads_to_create), rule->num_cache_threads_to_create,rule->num_prefetch_threads_to_create);
	PRINT_LOG("\n[%d]#########################################################################",__LINE__);
	PRINT_LOG("\n[%d] Thread specific info for testcase = %s",__LINE__,&(rule->rule_id[0]));
	PRINT_LOG("\n[%d]#########################################################################",__LINE__);
	for ( i=0 ; i<total_cpus; i++ ) {
		switch ( th_array[i].thread_type ) {
			case CACHE:
			case PREFETCH:
				PRINT_LOG("\n[%d] Thread index                      = %d",__LINE__,th_array[i].thread_no);
				if ( th_array[i].thread_type == CACHE ) {
					PRINT_LOG("\n[%d] Thread type                       = CACHE",__LINE__); 
				} else if ( th_array[i].thread_type == PREFETCH ) {
					PRINT_LOG("\n[%d] Thread type                       = PREFETCH",__LINE__); 
				}
				PRINT_LOG("\n[%d] Bound to CPU                      = %d",__LINE__,th_array[i].bind_to_cpu);
				PRINT_LOG("\n[%d] Pattern                           = 0x%llx",__LINE__,th_array[i].pattern);
		
				switch(th_array[i].prefetch_algorithm) {
					case PREFETCH_OFF:
						PRINT_LOG("\n[%d] Prefetch algorithm                = PREFETCH_OFF",__LINE__);
						break;
					case PREFETCH_IRRITATOR:
						PRINT_LOG("\n[%d] Prefetch algorithm                = PREFETCH_IRRITATOR",__LINE__);
						break;
					case PREFETCH_NSTRIDE:
						PRINT_LOG("\n[%d] Prefetch algorithm                = PREFETCH_NSTRIDE",__LINE__);
						break;
					case PREFETCH_PARTIAL:
						PRINT_LOG("\n[%d] Prefetch algorithm                = PREFETCH_PARTIAL",__LINE__);
						break;
					case PREFETCH_TRANSIENT:
						PRINT_LOG("\n[%d] Prefetch algorithm                = PREFETCH_TRANSIENT",__LINE__);
						break;
					case RR_ALL_ENABLED_PREFETCH_ALGORITHMS:
						PRINT_LOG("\n[%d] Prefetch algorithm                = Random",__LINE__);
						break;
					default:
						PRINT_LOG("\n[%d] Unknown Prefetch algorithm",__LINE__);
						break;
				}

				PRINT_LOG("\n[%d] Offset within cache line          = %d",__LINE__,th_array[i].offset_within_cache_line);
				PRINT_LOG("\n[%d] Memory starting address offset    = %d",__LINE__,th_array[i].memory_starting_address_offset);
				if ( th_array[i].thread_type == CACHE ) {
					for ( j=0 ; j<th_array[i].pages_to_write ; j++ ) {
						PRINT_LOG("\n[%d] E.A. of contig_mem[%d]             = %p",__LINE__,j,th_array[i].contig_mem[j]);
					}
				} else if ( th_array[i].thread_type == PREFETCH ) {
					PRINT_LOG("\n[%d] Start of contiguous memory        = %p",__LINE__,th_array[i].start_of_contiguous_memory);
				}
				PRINT_LOG("\n[%d]*****************************************************",__LINE__);
				break;

			default:
				break;
		}
	}
	return rc;
}

void dump_rule_structure(struct ruleinfo *rule_ptr) {

	struct ruleinfo *rule;
	int 			testcase_num;
	int				i;
	int				loop_count;
	char			*active_prefetch_algos;

	print_log("\n\n[%d]##########################################################",__LINE__);
	print_log("\n[%d]            Dumping Rule structure elements               ",__LINE__);
	print_log("\n[%d]##########################################################",__LINE__);

	if ( rule_ptr == NULL ) {
		rule = &(h_r[0]);
		loop_count = num_testcases;
	} else {
		rule = rule_ptr;
		loop_count = 1;
	}

	for ( testcase_num = 0 ; testcase_num < loop_count ; testcase_num++ ) {
		print_log("\n[%d] Rule id                                = %s",__LINE__,&(rule->rule_id[0]));
		
		if ( rule->tgt_cache == L2 ) {
			print_log("\n[%d] Target cache                           = L2",__LINE__);
		}else if ( rule->tgt_cache == L3 ) {
			print_log("\n[%d] Target cache                           = L3",__LINE__);
		}
	
		switch ( rule->testcase_type ) {
			case CACHE_BOUNCE_ONLY:
				print_log("\n[%d] Test case                              = CACHE_BOUNCE_ONLY",__LINE__);
				break;

			case CACHE_ROLL_ONLY:
				print_log("\n[%d] Test case                              = CACHE_ROLL_ONLY",__LINE__);
				break;

			case CACHE_BOUNCE_WITH_PREF:
				print_log("\n[%d] Test case                              = CACHE_BOUNCE_WITH_PREF",__LINE__);
				break;

			case CACHE_ROLL_WITH_PREF:
				print_log("\n[%d] Test case                              = CACHE_ROLL_WITH_PREF",__LINE__);
				break;

			case PREFETCH_ONLY:
				print_log("\n[%d] Test case                              = PREFETCH_ONLY",__LINE__);
				break;

			default:
				print_log("\n[%d] Test case                              = UNKNOWN",__LINE__);
				break;
		}

		print_log("\n[%d] Core number excluded in this test case       =",__LINE__);
		for (i=0 ; i<rule->num_excluded_cores ; i++) {
			print_log(" %d ",rule->exclude_cores[i]);
		}
		active_prefetch_algos = (char *) malloc(MAX_PREFETCH_NAME_LENGTH);
		memset(active_prefetch_algos,0,MAX_PREFETCH_NAME_LENGTH);

		get_enabled_prefetch_algos ( rule->pf_conf, active_prefetch_algos );	

		if ( rule->testcase_type != CACHE_BOUNCE_ONLY || rule->testcase_type != CACHE_ROLL_ONLY ) {
			print_log("\n[%d] Prefetch algorithm enabled             = %s",__LINE__,active_prefetch_algos);
		} else { 
			print_log("\n[%d] Prefetch algorithm enabled             = ",__LINE__);
		}
		print_log("\n[%d] Prefetch threads to be created         = %d",__LINE__,rule->num_prefetch_threads_to_create);
		print_log("\n[%d] Cache threads to be created            = %d",__LINE__,rule->num_cache_threads_to_create);
		print_log("\n[%d] HUGE pages required for Cache           = %d",__LINE__,rule->cache_HUGE_pages_required);
		print_log("\n[%d] HUGE pages required for Prefetch        = %d",__LINE__,rule->prefetch_HUGE_pages_required);
		if ( rule->use_contiguous_pages == TRUE ) {
		print_log("\n[%d] Using HUGE contiguous pages for cache  = Yes",__LINE__);
		print_log("\n[%d] Contiguous memory required             = %d bytes ( %d MB )",__LINE__,rule->cont_memory_pool.contiguous_mem_required, (rule->cont_memory_pool.contiguous_mem_required/1048576) );
		} else {
		print_log("\n[%d] Using HUGE contiguous pages            = No",__LINE__);
		}
		print_log("\n[%d] Number of sets of contiguous pages     = %d",__LINE__,rule->cont_memory_pool.num_sets);
		
		for(i=0; i<rule->cont_memory_pool.num_sets; i++) {
			print_log("\n[%d] cache_thread_address[%d][0]             = %p",__LINE__,i,rule->cont_memory_pool.cont_mem_set[i][0]);
			print_log("\n[%d] cache_thread_address[%d][1]             = %p",__LINE__,i,rule->cont_memory_pool.cont_mem_set[i][1]);
		}

		for(i=0; i<rule->cont_memory_pool.prefetch_sets; i++) {
			print_log("\n[%d] Prefetch_thread_address[%d]             = %p",__LINE__,i,rule->cont_memory_pool.prefetch_mem_set[i]);
		}
		print_log("\n[%d]********************************************\n",__LINE__);
		rule++;
		free(active_prefetch_algos);
	}
}

int get_core_list_from_string(char *input, int *output) {
	int rc = FAILURE;
	int count = 0;
	char *token;
	const char delims[] =" ,";

	token = strtok(input,delims);

	DEBUG_LOG("[%d] input = %s, token = %s\n",__LINE__,input, token);
	while (token != NULL) {
		DEBUG_LOG("[%d] count = %d, token = %s, core num = %d\n",__LINE__, count, token, output[count]);
		output[count] = atoi(token);
		count++;
		rc = count;
		token = strtok(NULL,delims);
	}

	return rc;
}

void get_enabled_prefetch_algos ( unsigned int prefetch_conf , char enabled_algos[] ) {

	if ( prefetch_conf & PREFETCH_OFF ) {
		strcat(enabled_algos,"No Prefetch ");
	}

	if ( prefetch_conf & PREFETCH_IRRITATOR ) {
		strcat( enabled_algos,"PREFETCH_IRRITATOR ");
	}

	if ( prefetch_conf & PREFETCH_NSTRIDE ) {
		strcat( enabled_algos, "PREFETCH_NSTRIDE ");
	}

	if ( prefetch_conf & PREFETCH_PARTIAL ) {
		strcat( enabled_algos, "PREFETCH_PARTIAL ");
	}

	if ( prefetch_conf & PREFETCH_TRANSIENT ) {
		strcat( enabled_algos, "PREFETCH_TRANSIENT ");
	}

	if ( prefetch_conf & PREFETCH_NA ) {
		strcat( enabled_algos, "PREFETCH_NA ");
	}

	if ( prefetch_conf & RR_ALL_ENABLED_PREFETCH_ALGORITHMS ) {
		strcat( enabled_algos, "PREFETCH_IRRITATOR PREFETCH_NSTRIDE PREFETCH_PARTIAL PREFETCH_TRANSIENT PREFETCH_NA ");
	}

}

/*
 * This function is used to generate the list of actual cores, which will be excluded in this instance.
 * The disable_cores array has the relative core numbers for this instance to be disabled.
 * Using this value in the array as the index into the list of cores available on this instance,
 * the actual core numbers to be excluded is determined.
 * INPUT PARAMS:
 * a pointer to ruleinto structure
 * an integer array containing relative core numbers to be excluded
 * total number of cores that are being disabled
 * OUTPUT
 * None
 */

void populate_cores_to_disable(struct ruleinfo *ruleptr, int *disable_cores, int num_cores_disabled) {
	int i=0;
	int index;
	int core_hash[MAX_CORES_IN_CHIP];
	int cores_to_exclude[MAX_CORES_IN_CHIP];
	long int rand_num;

	for ( i=0 ; i<MAX_CORES_IN_CHIP; i++ ) {
		core_hash[i] = -1;
	}

	DEBUG_LOG("[%d] cores to exclude = %d\n",__LINE__,num_cores_disabled);
	if ( disable_cores == (int *)NULL ) {
		disable_cores = cores_to_exclude;
		for (i=0 ; i< num_cores_disabled; i++ ) {
			do {
				lrand48_r(&sbuf,&rand_num);
				rand_num %= num_cores_disabled;
				DEBUG_LOG("[%d] random number generated = %d \n",__LINE__,rand_num);
			}while ( core_hash[rand_num] == 1 );
			core_hash[rand_num] = 1;
			cores_to_exclude[i] = rand_num;
		}
	}

	for ( i=0 ; i<num_cores_disabled ; i++ ) {
		index = disable_cores[i];
		ruleptr->exclude_cores[i] = system_information.cores_in_instance[index];
	}

}

#ifdef __HTX_LINUX__
int get_update_syschanges_hotplug() 

{
	int retcode = SUCCESS;
	int i,j;

	sprintf(msg,"cpus=%d   and	cores = %d before hotplug\n",system_information.number_of_logical_cpus,system_information.num_cores);
	hxfmsg(&h_d,retcode,HTX_HE_INFO, msg);
	
	retcode = get_system_hardware_details();
	if ( retcode != SUCCESS ) {
		sprintf(msg,"[%d]: sigusr:Error while collecting system information. Exitting !!\n",__LINE__);
		hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
		return ( retcode );
	}
	sprintf(msg,"cpus=%d  and	cores = %d after re populate\n",system_information.number_of_logical_cpus,system_information.num_cores);
	hxfmsg(&h_d,retcode,HTX_HE_INFO, msg);

	for(i=0 ; i<num_testcases ;i++) {
		for ( j=0 ; j<MAX_HUGE_PAGES ; j++ ) {
			h_r[i].mem_page_status[j] = PAGE_FREE;
		}
	}
	cleanup_done = 0;
	retcode = memory_set_cleanup();
	if ( retcode != SUCCESS ) {
		sprintf(msg,"[%d]: Error while releasing memory. Exitting !!\n",__LINE__);
		hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
		return ( retcode );
	}
	retcode = find_memory_requirement();
	if ( retcode != SUCCESS ) {
		sprintf(msg,"[%d] sigusr2:Error: Unable to calculate memory_requirement (retcode = %d)\n",__LINE__,retcode);
		hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
		return ( retcode );
	}
	do {
		retcode = allocate_worst_case_memory();

		if ( retcode == E_NOT_ENOUGH_CONT_PAGES ) {
			cleanup_done = 0;
			if ( (retcode = memory_set_cleanup())!= SUCCESS) {
				sprintf(msg,"[%d] Error: Unable to cleanup memory set\n",__LINE__);
				hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
				return (retcode);
			}
		
			for(i=0 ; i<num_testcases ;i++) {
				for ( j=0 ; j<MAX_HUGE_PAGES ; j++ ) {
					h_r[i].mem_page_status[j] = PAGE_FREE;
				}
			}
		} else if ( retcode != SUCCESS ) {
			sprintf(msg,"[%d]: Error while allocating memory (returned = %d) !!\n",__LINE__,retcode);
			hxfmsg(&h_d, retcode, HTX_HE_HARD_ERROR, msg);
			cleanup_done = 0;
			if ( (retcode = memory_set_cleanup())!= SUCCESS) {
				sprintf(msg,"[%d]: Error while releasing memory. Exitting !!\n",__LINE__);
				hxfmsg(&h_d, 0, HTX_HE_HARD_ERROR, msg);
				return (retcode);
			}
			return (retcode);
		}

	}while ( retcode != SUCCESS );
	/* Now dump the system wide running information */
	dump_system_info();

	/* Now dump the rule structure */
	dump_rule_structure(NULL);
	update_sys_detail_flag = 0;
	return (retcode);
}

int find_EA_to_RA(unsigned long ea,unsigned long long* ra){
    const int __endian_bit = 1;
    int i, c, pid, status;
    uint64_t read_val, file_offset;
    char path_buf [0x100] = {};
    FILE * f;
    char *end;
    sprintf(path_buf, "/proc/self/pagemap");

    /*printf("Big endian? %d\n", is_bigendian());*/
    f = fopen(path_buf, "rb");
    if(!f){
        printf("Error! Cannot open %s\n", path_buf);
        return -1;
    }

    /*Shifting by virt-addr-offset number of bytes*/
    /*and multiplying by the size of an address (the size of an entry in pagemap file)*/
    file_offset = ea / getpagesize() * PAGEMAP_ENTRY;
    /*printf("Vaddr: 0x%lx, Page_size: %d, Entry_size: %d\n", ea , getpagesize(), PAGEMAP_ENTRY);
    printf("Reading %s at 0x%llx\n", path_buf, (unsigned long long) file_offset);*/
    status = fseek(f, file_offset, SEEK_SET);
    if(status){
        perror("Failed to do fseek!");
        return -1;
    }
    errno = 0;
    read_val = 0;
    unsigned char c_buf[PAGEMAP_ENTRY];
    for(i=0; i < PAGEMAP_ENTRY; i++){
        c = getc(f);
        if(c==EOF){
            printf("\nReached end of the file\n");
            return 0;
        }
        if(is_bigendian())
           c_buf[i] = c;
        else
            c_buf[PAGEMAP_ENTRY - i - 1] = c;
        /*printf("[%d]0x%x ", i, c);*/
    }
    for(i=0; i < PAGEMAP_ENTRY; i++){
        read_val = (read_val << 8) + c_buf[i];
    }
    /*printf("\n");
    printf("Result: 0x%llx\n", (unsigned long long) read_val);*/
    if(GET_BIT(read_val, 63)){
        /*printf("PFN: 0x%llx\n",(unsigned long long) GET_PFN(read_val));*/
        *ra = (unsigned long long)GET_PFN(read_val);
    }else
        /*printf("Page not present\n");*/
    if(GET_BIT(read_val, 62))
        /*printf("Page swapped\n");*/
    fclose(f);
    return 0;
}

#endif
