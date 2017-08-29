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
#include <pthread.h>
#include "htxsyscfg64.h"

#include <sys/shm.h>
#include <sys/mman.h>

#ifdef __HTX_LINUX__
#	include <sys/stat.h>
#	include <sys/ioctl.h>
#	include <linux/unistd.h>
#	include <fcntl.h>
#endif
#include <sys/types.h>
#include <sys/ipc.h>

#ifndef __HTX_LINUX__
#	include <sys/systemcfg.h>
#	include <sys/vminfo.h>
#	include <sys/processor.h>
#endif

#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strings.h>
#include <hxihtx64.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef __HTX_LINUX__
#	include <sys/thread.h>
#	include <memory.h>
#endif

#ifdef __HTX_LINUX__
#	include <stdint.h>
#	include <strings.h>
#	include <string.h>
#	include <stdarg.h>
#	include <unistd.h>
#	include <sched.h>

/*
 * Since glibc has no wrapper for gettid
 * we need to use the syscall() function call it
 * indirectly. We need an argument of pid_t
 * to be given to sched_setaffinity when binding
 * a thread to a processor, pthread_self() returns
 * a thread id of type pthread_t AND getpid() returns
 * the pid of the parent thread.
 * The following include is req for syscall()
 */
#include <sys/syscall.h>
#endif

#include <errno.h>

#if defined(_DR_HTX_) && !defined(__HTX_LINUX__)
#	include <sys/types.h>
#	include <sys/dr.h>
#	include <sys/procfs.h>
#endif

#if 0
#ifndef __HTX_LINUX__
#	pragma mc_func trap { "7c810808" }
#endif
#endif

#ifndef __HTX_LINUX__
	void dcbtna(volatile unsigned long long);
	void dcbtds(volatile unsigned long long);
	void dcbtds_0xA(volatile unsigned long long);

#pragma mc_func dcbtna { "7E201A2C" }
/*
   	dcbt 0, r3, 0b10001
	011111  10001  00000	00011	01000101100
	31		th		ra		rb		278
 */
#pragma mc_func dcbtds { "7D001A2C" }
/*
   	dcbt 0,r3,01000
	011111	01000	00000	00011	01000101100
	31		th		ra		rb		278
*/
#pragma mc_func dcbtds_0xA {"7D401A2C" }
/*
   	dcbt 0,r3,01010
	011111	01010	00000	00011	01000101100
	31		th		ra		rb		278
*/
#else
#define dcbtna(x) __asm ("dcbt 0,%0,0x11\n\t": : "r" (x))
#define dcbtds(x) __asm ("dcbt 0,%0,0x8\n\t": : "r" (x))
#define dcbtds_0xA(x) __asm ("dcbt 0,%0,0xA\n\t": : "r" (x))
#endif

#define mtspr_dscr(x) __asm volatile ("mtspr 3, %0\n\t": "=r"(x))
#define mfspr_dscr(x) __asm volatile ("mfspr %0, 3\n\t": : "r"(x))

/* Log levels */
#define MINIMUM 0
#define LOGFILE 1
#define SCREEN  2

#define RUN_LOG_FILE "/tmp/hxecache.runlog"
#ifdef DEBUG
#define DEBUG_LOG print_log
#else
#define DEBUG_LOG(fmt,...)
#endif

/*#define PRINT_LOG(format, ...) ((log_level == MINIMUM ) ? 0 : print_log(format, __VA_ARGS__)) */
#if defined(RUNLOG) || defined(RUNLOG_SCREEN)
#define PRINT_LOG print_log
#else
#define PRINT_LOG(fmt, ...)
#endif

#define  B	 				1
#define  K	 				1024*B
#define  M	 				1024*K

/* Memory page size */
#define PREF_PG_SIZE		4*K

/* Target cache types	*/
#define L2 					0
#define L3 					1

/* Thread types */
#define ALL		 			0	/* For an uninitialised thread */
#define CACHE				1
#define PREFETCH			2

/* DSCR types */
#define DSCR_DEFAULT		0
#define DSCR_RANDOM			1
#define DSCR_LSDISABLE		2
/* Testcase types */
#define	CACHE_BOUNCE_ONLY		1
#define	CACHE_BOUNCE_WITH_PREF	2
#define	CACHE_ROLL_ONLY			3
#define	CACHE_ROLL_WITH_PREF	4
#define	PREFETCH_ONLY			5

/* Generic ON/OFF states used for flags*/
#define OFF					0
#define ON					1

#define FOUR_BITS_ON 		0xf

/* Prefetch Algorithms */
#define PREFETCH_OFF		0
#define PREFETCH_IRRITATOR	1
#define PREFETCH_NSTRIDE	2
#define PREFETCH_PARTIAL	4
#define PREFETCH_TRANSIENT	8
#define PREFETCH_NA							16
#define RR_ALL_ENABLED_PREFETCH_ALGORITHMS 	32

#define GANG_SIZE 			8
#define MAX_CONTIG_PAGES 	5
#define MAX_RULEID_LEN		40		/* Maximum characters in Rule id string */
#define MAX_PREFETCH_ALGOS	5
#define MAX_PREFETCH_STREAMS		16
#define	MAX_PREFETCH_NAME_LENGTH	200
/*
 * For now create one 512 MB seg and see if you can get required physical
 * contiguous pages.
 */

#define NUM_SEGS 			2
#define SEG_SIZE 			256*M
#define MAX_TC 				12
#define MAX_DEVS 			1024
#if 0
#define MAX_CPUS			1024
#endif
#define LOGFILE_W	   		"wlog"
#define LOGFILE_R	   		"rlog"

#define PAGE_FREE			0
#define CACHE_PAGE			1
#define PAGE_HALF_FREE 		2
#define PREFETCH_PAGE  		3
#define PREFETCH_MEM_SZ 	8*M

/* Exit flag codes */
#define ERR_EXIT 			1
#define NORMAL_EXIT			2
#define DR_EXIT 			3

#define FALSE 				0
#define TRUE 				1
#define MAX_SYNC_PHASES 	4

#define BYTE_WIDTH	 		0
#define SHORT_WIDTH			1
#define INT_WIDTH			2
#define LONG_WIDTH			3
#define RANDOM_WIDTH 		4

/* Power processor types */
#define POWER6				0x3e
#define POWER7				0x3f
#define POWER7P				0x4a
#define POWER8_MURANO		0x4b
#define POWER8_VENICE		0x4d
#define POWER8_PRIME		0x4c
#define POWER9_NIMBUS       0x4e
#define POWER9_CUMULUS      0x4f

/* Operation return values */
#define SUCCESS					0
#define FAILURE					-1
#define E_NOT_ENOUGH_CONT_PAGES -2
#define E_NOT_ENOUGH_FREE_PAGES	-3
#define E_UNKNWON_THR_TYPE		-4
#define E_NOT_ENOUGH_MEM		-5
#define E_SHMCTL_FAIL			-6
#define E_SHMAT_FAIL			-7
#define E_MLOCK_FAIL			-8
#define E_NO_FILE				-9
#define E_NO_SET_SHMMAX			-10
#define E_UNKNOWN_DATA_WIDTH	-11
#define E_SHM_DETACH_FAIL		-12
#define E_SHM_UNLOCK_FAIL		-13
#define E_IPC_RMID_FAIL			-14
#define E_MISCOMPARE			-15
#define E_WRONG_TEST_CASE		-16
#define E_INVALID_TEST_CASE		-17
#define E_RF_PARSE_FAIL			-18
#define E_NO_LOG_FILE			-19
#define E_UNKNOWN_PVR			-20
#define E_UNKNOWN_PAGE_STATUS	-21
#define E_CORE_EXCLUDED			-22

/* Maximum SMT value */
#define MAX_SMT				8


/* Maximum number of Prefetch threads */
#define	MAX_CORES_IN_CHIP		24
#define MAX_PREFETCH_THREADS	( (MAX_SMT - 1)*MAX_CORES_IN_CHIP
/* Maximum 4K pages to be allocated for prefetch threads */
#define MAX_4K_PAGES		( (PREFETCH_MEM_SZ / PREF_PG_SIZE) * MAX_PREFETCH_THREADS )

/* Maximum HUGE pages to be allocated */
#define MAX_HUGE_PAGES_PER_CORE	10
#define MAX_HUGE_PAGES      (MAX_HUGE_PAGES_PER_CORE * MAX_CORES_IN_CHIP) 
struct cache_info {
	int line_size;	/* Cache line size */
	int asc;		/* cache associativity */
	int num_sets;	/* Number of sets */
	int cache_size;	/* Total cache size */
};

struct thread_context {
	int 					thread_no;
	pthread_t 				tid;
	pthread_attr_t  		thread_attrs_detached;
	int 					bind_to_cpu;
	int 					tc_id;
	struct drand48_data 	buffer;
	long int 				random_pattern;
	long int 				seedval;
	unsigned int  			prev_seed;
	unsigned int 			saved_seed;
	char 					dev_name[50];
	unsigned long long 		pattern;
	unsigned int 			current_oper;
	struct ruleinfo *		current_rule;
	unsigned long long 		prefetch_scratch_mem[64/sizeof(unsigned long long)];
	int 					thread_type;						/* Possible values PREFETCH, CACHE, ALL. 									*/
	int 					prefetch_algorithm;					/* Used by prefetch threads to determine which prefetch algorithm to run.	*/
	int 					start_class;
	int 					end_class;
	int 					walk_class_jump;
	int 					offset_within_cache_line;
	int 					cache_instance_under_test;			/* The physical instance of cpu cache being tested by this thread.			*/

	int 					memory_starting_address_offset;		/* Offset of the starting address in memory segment that maps to. 			*/
												 				/* the cache instance being tested by this thread. 							*/
	char* 					start_of_contiguous_memory;			/* This is a pointer to the start address of contiguous memory that maps 	*/
												 				/* the cache. This will be used as a starting point upon which offsets will	*/
												 				/* be added and memory address to read/write will be calculated.			*/
	unsigned long long int 	sync_mask;
	char					*contig_mem[MAX_HUGE_PAGES_PER_CORE*MAX_CORES_PER_CHIP];
	int						prefetch_streams;					/* The maximum number of prefetch streams for the thread. Valid only for prefetch threads. 	*/
	unsigned long long		read_dscr_val;						/* The DSCR value that is read from SPR 3.													*/
	unsigned long long		written_dscr_val;					/* The DSCR value that is written to SPR 3.													*/
	int						pages_to_write;						/* Number of HUGEB pages written by ( cache ) thread.										*/	
	int						num_mem_sets;						/* Number of sets of contiguous memory to be written by ( cache ) thread.					*/
};

struct memory_set {
	unsigned char 		*seg_addr[NUM_SEGS];			   					/* shared memory EA. 										*/
	int 				shm_id[NUM_SEGS];				   					/* shared memory ids arrary. 								*/
	int 				key[NUM_SEGS];										/* shared memory keys array. 								*/
	unsigned long long 	real_addr[MAX_HUGE_PAGES];							/* shared memory RA arrary 									*/
	unsigned char 		*ea[MAX_HUGE_PAGES];									/* shared memory EA array  Maximum mem required in case		*/
																			/* of p7+ rollover, 128MB/core in worst case.				*/
	int 				num_pages;											/* Num of HUGE pages allocated 								*/
	unsigned long long 	page_size;
	unsigned int 		cache_threads;										/* Number of cache threads created. 						*/
	unsigned int 		prefetch_threads;									/* Prefetch threads created.								*/
	unsigned int 		total_threads;										/* Total threads created.									*/
	unsigned int 		contiguous_memory_size;	 							/* Size of conitguous memory. 								*/
	unsigned int 		memory_set_size[NUM_SEGS];							/* Size of entire memory set. 								*/
	unsigned int 		page_status[MAX_HUGE_PAGES];							/* Status of the page (FREE / CACHE / PREFETCH).			*/
	unsigned char		*prefetch_4k_memory;								/* Memory allocated (of 4K page size) for prefetch threads	*/
};

struct sys_info {
	unsigned int		pvr;
	int			   		smt_threads;
	int			   		number_of_logical_cpus;
	int			   		start_cpu_number;					  						/* Start cpu number for this instance	   			*/
	int			   		end_cpu_number;												/* End cpu number for this instance		 			*/
	int			   		instance_number;					   						/* Instance number						  			*/
	int			   		cores_in_instance[MAX_CORES_PER_CHIP];						/* List of core number available in chip			*/
	int					cpus_in_instance[MAX_CPUS_PER_CHIP];						/* List of CPUs available in chip.					*/
	int			   		num_cores;							 						/* Number of cores in the instance		  			*/
	SYS_STAT			sys_stat;							  						/* Hardware statistics					  			*/
	CPU_MAP		   		cpu_in_chip_map;					   						/* Map containing range of CPUS in instance 		*/
	int					core_smt_array[MAX_CORES_PER_CHIP];							/* Array containing SMT of each core in instance.	*/
	int					cpus_in_core_array[MAX_CORES_PER_CHIP][MAX_CPUS_PER_CORE];	/* Array containing CPU list in each core. */
	int 				p9_dd1_bit;
	struct cache_info 	cinfo[2];							  						/* Contains cache info for L2 and L3				*/
};

struct mempool_t {
	unsigned int	contiguous_mem_required;					/* Contiguous memory required.												*/
	unsigned int	num_sets;									/* Number of sets of contiguous memory areas needed.						*/
	unsigned int	prefetch_sets;								/* Number of sets of prefetch memory.										*/
	unsigned char	*cont_mem_set[MAX_CPUS_PER_CHIP][MAX_HUGE_PAGES_PER_CORE];	/* Array of pointers to store contiguous memory locations.					*/
	unsigned char	*prefetch_mem_set[MAX_CPUS_PER_CHIP];		/* Array of pointers to store memory address allocated to Prefetch threads.	*/
};

struct ruleinfo {
	char 					rule_id[MAX_RULEID_LEN+1];			/* Test case name											 	*/
	int						tgt_cache;							/* L2 - 0, L3 - 1											 	*/
	int 					bound;							  	/* Yes - 1, No - 0												*/
	int 					compare;							/* Yes - 1, No - 0												*/
	int 					data_width;						 	/* 0-byte, 1-short, 2-word, 3-double, 4-randomize			 	*/
	int 					target_set;						 	/* Which set you want to target. -1 means all				 	*/
	int 					crash_on_misc;					  	/* 0 - No, 1 - Yes, i.e. Crash to KDB on Miscompare		   		*/
	int 					num_oper;						   	/* 0 - infinite, +ve integer								  	*/
	int 					seed;							   	/* seed													   		*/
	int 					testcase_type;					  	/* 0 - Cache Rollover, 1 - Cache Bounce					   		*/
	int 					gang_size;						  	/* default = 8, for Equaliser = 1							 	*/
	int 					thread_sync;						/* Default = TRUE, used to enable/disable sync between threads	*/
	unsigned int			pf_irritator;						/* Prefetch irritator on/off								  	*/
	unsigned int 			pf_nstride;							/* Prefetch n-stride  on/off								  	*/
	unsigned int 			pf_partial;							/* Prefetch partial  on/off								   		*/
	unsigned int 			pf_transient;						/* Prefetch transient  on/off								 	*/
	unsigned int			pf_dcbtna;							/* Prefetch DCBTNA on/off										*/
	unsigned int			pf_dscr;							/* Prefetch Randomise DSCR on/off								*/
	unsigned int 			pf_conf;							/* Prefetch configuration, a unique state of a combination of 	*/
																/* the above prefetch options on/off						  	*/
	unsigned int			prefetch_memory_size;				/* The amount of memory which will be prefetched.				*/
	unsigned int			cache_memory_size;					/* The amount of memory that will be written by cache threads	*/
	unsigned int			testcase_conf;						/* 32-bit variable to store testcase configuration.				*/
	unsigned int			cache_HUGE_pages_required;			/* Cache memory required for this rule stanza					*/
	unsigned int			prefetch_HUGE_pages_required;		/* Prefetch memory required for this rule stanza				*/
	unsigned int 			total_HUGE_pages_required;			/* Actual amount of HUGE pages needed for this rule.				*/
	int						mem_page_status[MAX_HUGE_PAGES];		/* Indicates the allocated page belongs to CACHE/PREFETCH.		*/
	struct mempool_t		cont_memory_pool;					/* Structure to store contiguous memory requirements.			*/
	int						exclude_cores[MAX_CORES_PER_CHIP];	/* List of cores that should be excluded from execution.		*/
	int						num_excluded_cores;
	int						skip;								/* TRUE or FALSE. Indicates whether this rule is being skipped	*/
	int						num_cache_threads_created;			/* Number of cache threads actually created in this rule.		*/
	int						num_prefetch_threads_created;		/* Number of prefetch threads actually created in this rule.	*/
	int						num_cache_threads_to_create;		/* Number of cache threads needed to be created in this rule.	*/
	int						num_prefetch_threads_to_create;		/* Number of prefetch threads needed to be created in this rule.*/
	int						use_contiguous_pages;				/* TRUE or FALSE, indicates we are looking for contiguous pages	*/
};

extern int Logical_cpus[MAX_THREADS];	   				/* Array of available logical processors					  	*/


#ifdef __HTX_LINUX__
#	include <linux/version.h>
#	include <asm/ioctl.h>
#	if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#		define IOCTL_NUM_IOWR
#	else
#		define IOCTL_NUM_IOWR_BAD
#	endif
#endif

#define MAXSYNCWORDS 20		/* This defines the size of the sync word array				 */
#define DISTANCE	 10		/* This defines the distance between sync words to be updated   */
#ifndef GANGSIZE
#define GANGSIZE	  8
#endif

/* Function declarations */
int				memory_set_cleanup(void);
int				get_cont_phy_mem(long long size);
int				write_mem(int index , int tid);
int				read_and_compare_mem(int index , int tid);
int				read_rule_file(void);
int				get_line(char *, FILE *);
int				parse_line(char *);
int				find_cache_details(void);
int				get_logical_to_physical_cpu(int);
int				create_threads(int thread_type);
int				derive_prefetch_algorithm_to_use(int index);
int				find_reqd_cache_mem(unsigned int mem_req , struct ruleinfo *rule, int memory_set);
int				find_reqd_contig_pages(unsigned int mem_req , struct ruleinfo *rule, int memory_set);
int				find_reqd_pages(unsigned int mem_req , struct ruleinfo *rule, int memory_set);
void			setup_memory_set(void);
void			SIGTERM_hdl(int );
void			set_defaults(void);
void			write_read_and_compare(void *);
int				setup_thread_context_array(int thread_type);
int				cleanup_thread_context_array(int thread_type);
int				cleanup_threads(int thread_type);
long long		find_worst_case_memory_required(void);
long long		find_worst_case_prefetch_memory_required(void);
extern void		prefetch_irritator(void * arg);
int				get_next_cpu(int thread_type, int test_case, int thread_no, int core_no);
int				update_hxecache_stats(int current_test_case);
int 			calculate_mem_requirement_for_p7p(struct ruleinfo *rule_ptr);
int 			calculate_mem_requirement_for_p7(struct ruleinfo *rule_ptr);
int 			calculate_mem_requirement_for_p8(struct ruleinfo *rule_ptr);
int 			calculate_mem_requirement_for_p9(struct ruleinfo *rule_ptr);
unsigned int 	get_max_total_mem_req(void);
unsigned int 	get_max_cache_mem_req(void);
int 			wait_for_threads_termination(int current_test_case);
int 			do_thread_cleanup(int thread_type);
int 			create_and_dispatch_threads(int current_test_case);
int 			allocate_worst_case_memory(void);
int 			find_memory_requirement(void);
int 			get_system_hardware_details(void);
int 			do_initial_setup(char **arg_list);
int 			register_signal_handlers(void);

/* Assembly routines for the thread sync  */
extern void	   	set_sync_word(volatile unsigned long int *,int,int , int);				 /*	 For setting the sync word to all 1s	*/
extern void	   	wait_for(volatile unsigned long int *,int,int);							 /*	  Wait till the sync word becomes 0	  */
extern void	   	change(volatile unsigned long int *,unsigned long long int, int,int);		/*   Turn off thread's bit in the sync word */

unsigned char* 		find_prefetch_mem(struct ruleinfo *);
unsigned char*		find_prefetch_mem_p8(int thread_num);
unsigned long int 	get_random_number(int seg);
int					synchronize_threads(int);
int 				dump_miscompare_data(int thread_index, void *addr);

/* Helper functions */
int 	is_empty_list(int list[], int list_size);
int 	core_to_exclude(int core_list[], int core_num);
int 	get_test_id(char *rule_id);
void 	hexdump(FILE *f,const unsigned char *s,int l);
int 	get_num_threads(int instance_number, int test_case, int type, struct ruleinfo *rule);
int 	get_num_threads_for_rule(int instance_number, int test_case, int type, struct ruleinfo * rule);
int 	set_shmmax_val(unsigned long long shm_size);
int 	dump_system_info(void);
void 	dump_rule_structure(struct ruleinfo *);
int		print_once_thread_data(struct ruleinfo *rule);
int 	get_device_instance(const char *device_name);
int 	print_log(const char *format, ...);
int 	get_core_list_from_string(char *input, int output[]);

#if defined(_DR_HTX_) && !defined(__HTX_LINUX__)
	void DR_handler(int sig, int code, struct sigcontext *scp);
#endif

