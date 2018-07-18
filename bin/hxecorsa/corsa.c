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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <sys/shm.h>
#include <sys/types.h>

#ifdef __HTX_LINUX__
#include "genwqe_card.h"
#endif

#include "hxemem64.h"
#include "nx_corsa.h"
#include "htxsyscfg64.h"


#define KB					1024
#define MB					((unsigned long long)(1024*KB))
#define SEG_SIZE 				256*MB
#define UNBIND_ENTITY		-1
#define BIT_PATTERN_WIDTH   8       /* 8 BYTES wide */
#define TIME_SEED			-2
#define MAX_PERFTEST_NUM_OPER 500
#define MODE_CRB_KILL       2
#define ASYM_NUM_RETRIES    100000
#define MAX_BUFF_SZ         16777216
#define	EEH_SLEEP			5


#ifdef _LIB_DEBUG_
	#define LDPRINT(fmt, ...)  printf(fmt, ## __VA_ARGS__ );
#else
	#define LDPRINT(fmt, ...)
#endif

#ifdef _LIB_MALLOC_DEBUG
	#define MALLOC_PRINT(fmt, ...)	printf(fmt, ## __VA_ARGS__ );
#else
	#define MALLOC_PRINT(fmt, ...)
#endif




struct chip_details{
	int chip_number;
	int total_cpus_per_chip;
	int cpus_in_chip[MAX_CPUS_PER_CHIP];
	int cpu_index;
	int chip_rule;
	int chip_thread_percent;
};


struct cpu_task_table{
	int nx_operation;
	int num_of_threads;
	unsigned int nx_min_buf_size;
	unsigned int nx_max_buf_size;
	int cpu_index;
	int task_rule;
	unsigned int *cpu_list;
};

struct nx_time_real{
	unsigned long long start;
	unsigned long long finish;
	unsigned long long complete;
	double latency;
	double ker_lat;
	unsigned int read_data;
	unsigned int write_data;
};

struct thr_bandwidth{
	double cumulative_time;
	double ker_cumulative_time;
	unsigned int total_read_data;
	unsigned int total_write_data;
	unsigned int cumulative_data;
	double	bandwidth;
	double	ker_bandwidth;
};

struct th_ctx {
	unsigned int thread_num;
	int thread_flag;
	pthread_t tid;
	pthread_t kernel_tid;
	int operation;
	int current_operation;
	unsigned int bind_proc;
	unsigned int num_oper;
	unsigned int crc;
	unsigned int buffer_size;
	unsigned int seed;
	unsigned int shm_offset;
	unsigned long long *shm_addr_offset;
	struct nx_time_real compress_time[100];
	struct nx_time_real decompress_time[100];
	struct thr_bandwidth compress_bw;
	struct thr_bandwidth decompress_bw;
	th_context* handler;
	th_context* direct_buffer;
};

struct nx_mem_data{
	int  number_of_chips;
	pthread_mutex_t tmutex;
	struct chip_details chip[MAX_CHIPS];
	struct cpu_task_table cpu_task[MAX_NX_TASK];
	int number_of_threads;
	struct th_ctx *thread_ct;
};

struct nx_mem_data  nx;
int shm_id, shm_flg;
unsigned long long *shm_addr, *shm_addr_start, *shm_addr_end;
SYS_STAT  Sys_stat;
struct timespec time_sleep;
unsigned int    eeh_retries = 1;

pthread_mutex_t corsa_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t corsa_cond = PTHREAD_COND_INITIALIZER;
int th_sync = 0;

struct drand48_data     rand_buf;
extern struct htx_data stats;
extern struct private_data priv;
extern struct rule_format r, *stanza_ptr;
char msg[1000];
extern int get_num_of_proc(void);
int mem_compress_decompress(void);
int populate_chip_data(void);
int fill_th_data(void);
int create_th_n_run(void);
void * run_oper(void *tn);
int deallocate_corsa_memory(void);
void init_random_gen(int);
u32 get_rand_32(void);
u64 get_rand_64(void);
int gen_rand_no(void);
int gen_random_range(int min, int max);
int gen_random_range_32(int min, int max);
int create_shm(unsigned long long *);
int fill_random_data(void *, unsigned int );
int corsa_compress_only(int);
int corsa_bypass_compare(int);
int corsa_cmprss_decmprss_compare(int);
int get_cpus(int ,int ,float);
int get_num_th(int ,float );
int comp_bufs(unsigned long long *,unsigned long long *,unsigned long long *, unsigned long long *, \
unsigned int, unsigned int, unsigned int, int);
void  dump_bufs(int ,char *,char *, char*, char*, int);
void hex_dump(FILE *,const unsigned char *,int );
int report_latency(void);
int report_ker_bw_latency(void);
extern int get_num_of_proc(void);
th_context * corsa_get_buffer(int, int, int);
int memcopy_buffer(th_context *, int);
int inflate_buffer(th_context *);
int deflate_buffer(th_context *);
int corsa_execute_algo(ALGOS, char, th_context *, int, int);
int corsa_release_buffer(th_context *);

int corsa_compress_decompress()
{
	unsigned long long *addr=NULL;
	int rc=-1;
	char  *eeh_env = NULL;

	/* Read env variable for eeh */
	eeh_env = getenv("HTXEEHRETRIES");
	if ( eeh_env != NULL ) {
		eeh_retries = atoi(eeh_env);
	} else {
		eeh_retries = 1;
	}

	/* increment test_id for updating the stanza on the screen*/
	stats.test_id++;
	hxfupdate(UPDATE,&stats);

	rc = populate_chip_data();
	if(rc != 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"populate_chip_data failed with errno = %d rc = %d\n",errno,rc);
		deallocate_corsa_memory();
		exit(1);
	}

	rc = fill_th_data();
	if(rc != 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"fill_th_data failed with errno = %d rc = %d\n",errno,rc);
		deallocate_corsa_memory();
		exit(1);
	}


	/* create shared memory for storing random numbers */
	rc = create_shm(addr);
	if(rc != 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"create_shm failed with errno = %d rc = %d\n",errno,rc);
		deallocate_corsa_memory();
		exit(1);
	}

	/* Populate the shared mem */
	rc = fill_random_data(shm_addr, SEG_SIZE);
	if(rc != 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"fill_random_data failed with errno = %d rc = %d\n",errno,rc);
		deallocate_corsa_memory();
		exit(1);
	}


	rc = create_th_n_run();
	if(rc != 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"create_th_n_run failed with errno = %d rc = %d\n",errno,rc);
		deallocate_corsa_memory();
		exit(1);
	}

	/* Stats Update */
	if ( nx.thread_ct[0].operation == BYPASS ) {
		/* Handle memcopy separately as it has only one pass */
		stats.good_others = nx.number_of_threads * stanza_ptr->num_oper; /* ioctls() issued. 1/num_oper */
	} else {
		/* For CDC and C-ONLY operations. this runs 2 operations per num_oper */
		stats.good_others = nx.number_of_threads * stanza_ptr->num_oper * 2; /* ioctl() issued. 2/num_oper */
	}
	stats.bytes_read = nx.number_of_threads * stanza_ptr->num_oper * nx.thread_ct[0].buffer_size;
	stats.bytes_writ = nx.number_of_threads * stanza_ptr->num_oper * nx.thread_ct[0].buffer_size;

	/* deallocating the memory before return */
	displaym(HTX_HE_INFO,DBG_INFO_PRINT,"calling deallocate_corsa_memory() after the execution of nx operations \n");
	deallocate_corsa_memory();
	return(0);
}


int populate_chip_data(void)
{
	int num_proc, i, j, k, reminder_cpus;
	signed int cpus_in_chip[MAX_CPUS_PER_CHIP];
	int chip_no, task_no, ret, chip_save, task_save, default_thread_percent = 0;
	int rc = -1;
	int oper_threads, total_task_thr_count = 0, remainder_threads = 0, total_chip_thr_count = 0;
	float thread_percent;
	unsigned int *cpu_list;
	htxsyscfg_env_details_t env_details;
	int total_cpus;


	/* Here we are populating chip data structure from the system */
	Sys_stat.nodes = 0;
	Sys_stat.chips = 0;
	Sys_stat.cores = 0;

#ifdef __HTX_LINUX__
	total_cpus = get_nprocs();
#else
	total_cpus = _system_configuration.ncpus;
#endif
    rc = repopulate_syscfg(&stats);
    if (rc) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"repopulate_syscfg failed with rc = %d\n",rc);
        return (-1);
    }

	rc = get_hardware_stat(&Sys_stat);

	(void)get_env_details(&env_details);

	if ( strcasecmp(env_details.proc_shared_mode, "yes") == 0 ) {
		Sys_stat.chips = Sys_stat.cores / MAX_CORES_PER_CHIP;
		if ( Sys_stat.cores % MAX_CORES_PER_CHIP )
			Sys_stat.chips++;

		Sys_stat.nodes = Sys_stat.chips / MAX_CHIPS_PER_NODE;
		if ( Sys_stat.chips % MAX_CHIPS_PER_NODE )
			Sys_stat.nodes++;
	}

	i = 0;
    for(chip_no=0; chip_no < Sys_stat.chips; chip_no++) {

		if ( strcasecmp(env_details.proc_shared_mode, "yes") == 0 ) {
			j = 0;
			while ( i < total_cpus && i < MAX_CPUS_PER_CHIP ) {
				cpus_in_chip[j]	= i;
				i++; j++;
			}
			rc = j;
		} else {
			rc = get_cpus_in_chip(chip_no,cpus_in_chip);
			if (rc == -1) {
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_cpus_in_chip failed with rc = %d\n",rc);
				return (-1);
			}
		}

		nx.chip[chip_no].chip_number = chip_no;
		nx.chip[chip_no].total_cpus_per_chip = rc;
		nx.chip[chip_no].cpu_index = 0;
		nx.chip[chip_no].chip_rule = DEFAULT_RULE;
		nx.chip[chip_no].chip_thread_percent = 0;
		total_chip_thr_count += nx.chip[chip_no].total_cpus_per_chip;
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"CPUs in chip no-%d = %d \n",chip_no,nx.chip[chip_no].total_cpus_per_chip);
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"MAX_THREADS_PER_CHIP = %d \n",MAX_THREADS_PER_CHIP);

		for(k=0; k<MAX_THREADS_PER_CHIP; k++) {
			if(cpus_in_chip[k] != -1) {
				nx.chip[chip_no].cpus_in_chip[k] = cpus_in_chip[k];
				displaym(HTX_HE_INFO,DBG_DEBUG_PRINT," %5d", nx.chip[chip_no].cpus_in_chip[k]);
			}

		} /* END of for MAX_THREADS_PER_CHIP */

	} /* END of for  chip_no loop */


	/* Marking chip to be specific or DEFAULT  also storing the thread percent for each chip and
		default chip */

	for(task_no = 0; task_no < stanza_ptr->number_of_nx_task; task_no++) {

		if((stanza_ptr->nx_task[task_no].chip != -1) && (stanza_ptr->nx_task[task_no].chip >=0 && stanza_ptr->nx_task[task_no].chip < MAX_P7_CHIP)){

			/* indicating that this chip has specific rule file entries */
			chip_no = stanza_ptr->nx_task[task_no].chip;
			nx.chip[chip_no].chip_rule = CHIP_SPECIFIC;
			nx.chip[chip_no].chip_thread_percent += stanza_ptr->nx_task[task_no].thread_percent;
		}
		else if(stanza_ptr->nx_task[task_no].chip == -1){
			default_thread_percent += stanza_ptr->nx_task[task_no].thread_percent;
		}
		nx.cpu_task[task_no].cpu_index = 0;

	} /* END of for loop task_no */




	/* Checking thread percent for each chip and default chip if % is more than 100 then throw error */

	if(default_thread_percent > 100){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"\n \ncheck the thread percent for default chip* in task_struct\n");
		return(1);
	}

	for(chip_no=0; chip_no < Sys_stat.chips; chip_no++) {
		if(nx.chip[chip_no].chip_thread_percent > 100){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"\ncheck the thread percent for chip_no = %d in task_struct\n",chip_no);
			return(1);
		}
	}




	/* making the entries in cpu_task structure from task_struct 	*
	 * coversion of percentage of threads to number of threads      */

	for(task_no = 0; task_no < stanza_ptr->number_of_nx_task; task_no++) {

		chip_no = stanza_ptr->nx_task[task_no].chip;
		thread_percent = stanza_ptr->nx_task[task_no].thread_percent;

		nx.cpu_task[task_no].nx_operation = stanza_ptr->nx_task[task_no].nx_operation;
		nx.cpu_task[task_no].nx_min_buf_size = stanza_ptr->nx_task[task_no].nx_min_buf_size;
		nx.cpu_task[task_no].nx_max_buf_size = stanza_ptr->nx_task[task_no].nx_max_buf_size;

		oper_threads = get_num_th(chip_no, thread_percent);
		if(oper_threads == -1){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_num_th failed \n");
			return(1);
		}

		nx.cpu_task[task_no].num_of_threads = oper_threads;

		displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"\n 1st: task_no = %d \t num_of_threads = %d \n", task_no,nx.cpu_task[task_no].num_of_threads);

		total_task_thr_count += nx.cpu_task[task_no].num_of_threads;
	}





	/* During percent to number of threads conbversion some threads remains, just allocate it to *
		all the tasks sequentially */

	remainder_threads = total_chip_thr_count - total_task_thr_count;
	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"\nremainder_threads  = %d \n", remainder_threads);
	if (stanza_ptr->nx_rem_th_flag){

		task_no = 0;
		while(remainder_threads > 0){
			displaym(HTX_HE_INFO,DBG_INFO_PRINT,"\n task_no = %d \t remainder_threads = %d \n",task_no,remainder_threads);
			nx.cpu_task[task_no].num_of_threads ++;
			if(task_no < stanza_ptr->number_of_nx_task - 1){
				task_no++;
			}
			else{
				task_no = 0;
			}

			remainder_threads --;
		}
	}


	/* Creating cpu list for each of the tasks with the mentioned percentage only  *
	 * get_cpus call takes care of allocting percentage of cpus from each chip *
	 * will allocated remaining threads in next section  						   */

	for(task_no = 0; task_no < stanza_ptr->number_of_nx_task; task_no++) {

	chip_no = stanza_ptr->nx_task[task_no].chip;
	thread_percent = stanza_ptr->nx_task[task_no].thread_percent;


		if(nx.cpu_task[task_no].num_of_threads <= 0){
			continue;
		}

		cpu_list = NULL;
		cpu_list = (unsigned int *) valloc(sizeof(int) * nx.cpu_task[task_no].num_of_threads );
		if(cpu_list == NULL){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"populate_chip_data:(malloc failed) Creation of cpu_list"
			"failed! errno = %d(%s) \t num_of_threads = %d\n", errno, strerror(errno),nx.cpu_task[task_no].num_of_threads);
			return(-1);
	}

		nx.cpu_task[task_no].cpu_list = cpu_list;
		memset(nx.cpu_task[task_no].cpu_list, 0x00, sizeof(int) * nx.cpu_task[task_no].num_of_threads);

		ret = get_cpus(chip_no, task_no, thread_percent);
		if(ret == -1){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_cpus failed \n");
			return(1);
		}

	} /* END of task_no loop*/


	/* This piece of code allocais the remining cpus from each chip to the tasks one by one */
	if (stanza_ptr->nx_rem_th_flag){

	task_no = 0;
		for(chip_no = 0;chip_no < Sys_stat.chips; chip_no++) {

			reminder_cpus = nx.chip[chip_no].total_cpus_per_chip - nx.chip[chip_no].cpu_index;
			if (reminder_cpus > 0) {

				for(i = 0; i < 	reminder_cpus; i++){

					chip_save = nx.chip[chip_no].cpu_index;
					task_save = nx.cpu_task[task_no].cpu_index;

					nx.cpu_task[task_no].cpu_list[task_save] = nx.chip[chip_no].cpus_in_chip[chip_save];

					nx.cpu_task[task_no].cpu_index = task_save + 1;
					nx.chip[chip_no].cpu_index = chip_save + 1;

					if(task_no < stanza_ptr->number_of_nx_task - 1){
						task_no++;
					}
					else{
						task_no = 0;
					}
				}
			}
		}
	}

	/*If nx performance flag nx_performance_data is enabled then use nx.cpu_task[0].num_of_threads */
	/*else get total number of threads (lcpu's) on the system as number_of_threads */

	num_proc = get_num_of_proc();
	nx.number_of_threads = num_proc;

#if 0
	displaym(HTX_HE_INFO,DBG_IMP_PRINT," \n number of logical processor on the system = %d \n number of threads"
	" using here = %d \n",num_proc, nx.number_of_threads);


	displaym(HTX_HE_INFO,DBG_INFO_PRINT,"\n DEBUG ROUTINE \n");
	displaym(HTX_HE_INFO,DBG_INFO_PRINT,"stanza_ptr->number_of_nx_task = %d \n",stanza_ptr->number_of_nx_task);

	for(chip_no=0; chip_no < Sys_stat.chips; chip_no++) {
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"\n********chip_number = %d *************\n",nx.chip[chip_no].chip_number);

		displaym(HTX_HE_INFO,DBG_INFO_PRINT," total_cpus_per_chip = %d \n cpu_index = %d \n    chip_rule = %d \n",
		nx.chip[chip_no].total_cpus_per_chip, nx.chip[chip_no].cpu_index, nx.chip[chip_no].chip_rule);
	}


	for(task_no = 0; task_no < stanza_ptr->number_of_nx_task; task_no++) {
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"\n******** cpu_task number = %d *****\n",task_no);

		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"nx_operation = %d \t nx_min_buf_size = %d \t nx__max_buf_size = %d \ntask_no.num_of_threads = %d \n",
		nx.cpu_task[task_no].nx_operation, nx.cpu_task[task_no].nx_min_buf_size, nx.cpu_task[task_no].nx_max_buf_size, nx.cpu_task[task_no].num_of_threads);

		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"cpulist for task_no = %d \n",task_no);
		for(i =0 ; i < nx.cpu_task[task_no].num_of_threads; i++){
			displaym(HTX_HE_INFO,DBG_INFO_PRINT,"nx.cpu_task[%d].cpu_list[%d] = %d\t",
			task_no,i,nx.cpu_task[task_no].cpu_list[i]);
		}

	}
#endif

	return(0);
}/*END of populate_chip_data */


int fill_th_data()
{
	int i, tn, task_no;
	unsigned int tmp_no;
	int seed = TIME_SEED;
	/* unsigned int seed = (unsigned int)stanza_ptr->seed; */
	struct th_ctx *thread_ct;

	nx.thread_ct = NULL;
	thread_ct = (struct th_ctx *) malloc((sizeof(struct th_ctx)) * nx.number_of_threads);
	if(thread_ct == NULL){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"fill_th_data:(malloc failed) Creation of thread data structures failed for th#%d! errno = %d(%s)\n",
		nx.number_of_threads, errno, strerror(errno));
		return(-1);
	}

	nx.thread_ct = thread_ct;

	/* memset the whole of the thread_ct  pointer area..i.e,  elements of  struct th_ctx zero*/
	memset(nx.thread_ct, 0x00, nx.number_of_threads * (sizeof(struct th_ctx)));
	displaym(HTX_HE_INFO,DBG_INFO_PRINT,"nx.thread_ct = %#lx, number_of_nx_task = %d \n",nx.thread_ct, stanza_ptr->number_of_nx_task);

	init_random_gen(seed);


	for(task_no = 0; task_no < stanza_ptr->number_of_nx_task; task_no++) {
		for(i =0 ; i < nx.cpu_task[task_no].num_of_threads; i++){
			tn = nx.cpu_task[task_no].cpu_list[i];
			thread_ct[tn].thread_num = tn;
			thread_ct[tn].thread_flag = THREAD_ACTIVE;
			thread_ct[tn].operation =  nx.cpu_task[task_no].nx_operation;

			tmp_no = gen_random_range_32(nx.cpu_task[task_no].nx_min_buf_size, nx.cpu_task[task_no].nx_max_buf_size);
			thread_ct[tn].buffer_size = tmp_no;
			tmp_no = gen_random_range(0, 255);
			thread_ct[tn].shm_offset = tmp_no;
			thread_ct[tn].seed = seed;
			thread_ct[tn].num_oper = stanza_ptr->num_oper;
		} /* END of num_of_threads */

	} /* END of task_no loop */


#if 0
	/* debug print routine */
	for(i = 0; i < nx.number_of_threads; i++){
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"thread_ct[%d].thread_num = %d \t thread_ct[%d].operation = %d \t "
		"thread_ct[%d].buffer_size = %d \t thread_ct[%d].num_oper = %d , thread_ct[%d].shm_offset = %d\n"
		,i,thread_ct[i].thread_num,i,thread_ct[i].operation,i, thread_ct[i].buffer_size,i,\
		thread_ct[i].num_oper,i,thread_ct[i].shm_offset );
	}
#endif
	return(0);
}/* END of fill_th_data */


int create_th_n_run(void)
{
	int i=0,res;
	void *tresult;
	int tnum;

	/* When non-0 NUM_THREADS value is provided in rulefile and it is less than total no of CPUs in system */
	if ( (nx.number_of_threads > stanza_ptr->num_threads) && (stanza_ptr->num_threads > 0) ) {
		nx.number_of_threads = stanza_ptr->num_threads;
	}

	for ( i=0; i<nx.number_of_threads; i++ ) {
		nx.thread_ct[i].direct_buffer = corsa_get_buffer(3, MAX_BUFF_SZ, 65536);
		if ( nx.thread_ct[i].direct_buffer == NULL ) {
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_buffer(th#%d) failed for direct_buffer. returned %d\n",i, nx.thread_ct[i].direct_buffer);
			return 1;
		}
	}

	for(i=0; i< nx.number_of_threads; i++) {
	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"create_th_n_run :number_of_threads = %d, flag=%d[%d, THREAD_ACTIVE]\n",nx.number_of_threads, nx.thread_ct[i].thread_flag, THREAD_ACTIVE);

		if( nx.thread_ct[i].thread_flag != THREAD_ACTIVE){
			continue;
		}

		tnum = nx.thread_ct[i].thread_num;
		tresult=(void *)&tnum;

		res=pthread_create((pthread_t *)&nx.thread_ct[i].tid,NULL, run_oper, &nx.thread_ct[i].thread_num);

		if ( res != 0) {
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"pthread_create "
			"failed(errno %d):(%s): tnum=%d\n",errno,strerror(errno),i);
			return(res);
		}
	}

	for(i=0; i< nx.number_of_threads; i++) {

		if( nx.thread_ct[i].thread_flag != THREAD_ACTIVE){
			continue;
		}

		res=pthread_join(nx.thread_ct[i].tid,&tresult);
		if ( res != 0) {
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"pthread_join failed(errno %d):(%s): tnum=%d\n",errno,strerror(errno),i);
			return(res);
		}
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Thread %d Just Joined\n" ,i);
		nx.thread_ct[i].thread_num= -1;
		nx.thread_ct[i].tid = -1;

	}
	return(0);

} /* END of create_th_n_run(void) */

void * run_oper(void *tn)
{
	int ti= *(int *)tn;
	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Thread(%d):Inside run_oper\n",ti);

	if (nx.thread_ct[ti].operation == CDC ){
			if (corsa_cmprss_decmprss_compare(ti) != 0) {
				pthread_exit((void *)1);
			}
	}
	else if (nx.thread_ct[ti].operation == CDC_CRC){
		nx.thread_ct[ti].crc = TRUE;
			displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Calling cmprss_decmprss_compare with crc\n");
			if (corsa_cmprss_decmprss_compare(ti) != 0) {
				pthread_exit((void *)1);
			}
	}
	else if(nx.thread_ct[ti].operation == BYPASS){
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Calling corsa_bypass_compare\n");
		if (corsa_bypass_compare(ti) != 0) {
			pthread_exit((void *)1);
		}
	}
	else  if(nx.thread_ct[ti].operation == C_ONLY){
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Calling corsa_compress_only\n");
		if (corsa_compress_only(ti) != 0) {
			pthread_exit((void *)1);
		}
	}
	else  if(nx.thread_ct[ti].operation == C_CRC){
		nx.thread_ct[ti].crc = TRUE;
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Calling corsa_compress_only with crc set\n");
		if (corsa_compress_only(ti) != 0) {
			pthread_exit((void *)1);
		}
	}
	else {
	   displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"The  nx.thread_ct[%d].operation field is invalid.Please check the oper field \n",ti,nx.thread_ct[ti].operation );
			 pthread_exit((void *)1);
	}

} /* END of run_oper */



int corsa_cmprss_decmprss_compare(int ti)
{
	unsigned long long  *end_addr, *patt, *addr_8, *output_addr_8, patt_8, start_index, *rand_buff_addr, *intermediate_buff;
	struct th_ctx *t = nx.thread_ct;
	unsigned int pi = 0, patt_size; /* pi is patter_index initialised to zero */
	int num_buffers, buf_size, i, j, ret, additional_buff, num_i;
	int mode,bi;

	buf_size = nx.thread_ct[ti].buffer_size;
	num_buffers = 3;

	additional_buff = (buf_size * 0.15);     /* In case of RANDOM date we need more buff size for pass1 buffer */
	additional_buff = additional_buff - ((int)(buf_size * 0.15) % 8); /* making multiple of 8  byte */
	nx.thread_ct[ti].handler = nx.thread_ct[ti].direct_buffer;

	/* Reset all buffers to clear data from prev runs. */
	for ( j=0; j<3; j++ ) {
		bzero((char *)nx.thread_ct[ti].handler->all_buf[j].ea, MAX_BUFF_SZ);
	}


	for(i = 0; i < num_buffers; i++){
		displaym(HTX_HE_INFO,DBG_DEBUG_PRINT," cmprss_decmprss_compare:tn = %d  buf_addr[%d].buf_size = %d \tbuf_addr[%d].ea = %ul\n",
		ti,i,nx.thread_ct[ti].handler->all_buf[i].buf_size,i, (unsigned long)nx.thread_ct[ti].handler->all_buf[i].ea);
	}

	t[ti].shm_addr_offset = (unsigned long long*)((char *) shm_addr_start + (t[ti].shm_offset * MB));
	rand_buff_addr = t[ti].shm_addr_offset;
	displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"t[%d].shm_addr_offset = 0x%llx \n",ti,t[ti].shm_addr_offset);
	init_random_gen((unsigned int)stanza_ptr->seed);


	if(stanza_ptr->nx_async_flag){
		mode = 1;
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"mode selected is async \n");
	}
	else {
		mode = 0;
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"mode selected is sync \n");
	}


	for (num_i = 0; num_i < stanza_ptr->num_oper; num_i++) {

		addr_8 = (unsigned long long *)t[ti].handler->all_buf[0].ea;
		end_addr = (unsigned long long *)((char *)addr_8 +t[ti].buffer_size);
		patt_size = stanza_ptr->pattern_size[pi];

		if (priv.exit_flag == 1) {
			displaym(HTX_HE_INFO,DBG_MUST_PRINT," cmprss_decmprss_compare(%d): received sigterm \n",ti);
			pthread_exit(0);
		}

		/* INITIALISING the BUFFERS with 0's before each opertion */
		for(bi = 0; bi <  num_buffers; bi++){
			memset((unsigned long long *)t[ti].handler->all_buf[bi].ea, 0x00,  buf_size + additional_buff);
		}


		if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
			for(; addr_8 < end_addr; )  {

				patt = addr_8;
				patt_8 = (unsigned long long)patt;
				*addr_8 = patt_8 ;
				addr_8++;

			}
		}else if( stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
			if ((rand_buff_addr >= (unsigned long long*) shm_addr_end) || ((rand_buff_addr+(t[ti].buffer_size)) > (unsigned long long*) shm_addr_end)){

				rand_buff_addr = (unsigned long long*)shm_addr_start;

			}

			memcpy((char*)addr_8, (char*)rand_buff_addr, t[ti].buffer_size);
			rand_buff_addr+= (t[ti].buffer_size);


			/* Buffer Data Management of the shared segment*/
			if ((rand_buff_addr >= (unsigned long long*) shm_addr_end) || ((rand_buff_addr+(t[ti].buffer_size)) > (unsigned long long*) shm_addr_end)){

				rand_buff_addr = (unsigned long long*)shm_addr_start;

			}
		}else if((stanza_ptr->pattern_type[pi] == PATTERN_SIZE_NORMAL) ||(stanza_ptr->pattern_type[pi] == PATTERN_SIZE_BIG)){
			start_index = 0;
			patt = (unsigned long long *)stanza_ptr->pattern[pi];

			for(; addr_8 < end_addr; )  {
				patt_8 = *(unsigned long long *)(patt + (start_index % (patt_size / BIT_PATTERN_WIDTH)));
				*addr_8 = patt_8 ;
				addr_8++;
				start_index ++;
			}
		}


		t[ti].handler->i_buf = &(t[ti].handler->all_buf[0]);
		t[ti].handler->o_buf = &(t[ti].handler->all_buf[1]);
		t[ti].handler->inlen = buf_size;
		t[ti].handler->outlen_pass1 = buf_size + additional_buff;
		t[ti].handler->outlen_pass2 = buf_size + additional_buff;
		t[ti].handler->inlen_ptr = &(t[ti].handler->inlen);
		t[ti].handler->outlen_ptr = &(t[ti].handler->outlen_pass1);

		ret =  corsa_execute_algo( corsa_deflate, NULL, t[ti].handler, 0, mode);
		if(ret != 0){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s:corsa_execute_algo compression failed with errno %d \t ret value = %d\n",
				__FUNCTION__, errno,ret);
			return 1;
		}

		t[ti].handler->i_buf = &(t[ti].handler->all_buf[1]);
		t[ti].handler->o_buf = &(t[ti].handler->all_buf[2]);
		t[ti].handler->outlen_pass2 = buf_size;
		t[ti].handler->inlen_ptr = &(t[ti].handler->outlen_pass1);
		t[ti].handler->outlen_ptr = &(t[ti].handler->outlen_pass2);

		ret =  corsa_execute_algo( corsa_inflate, NULL, t[ti].handler, 0, mode);
		if(ret != 0){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"corsa_execute_algo decompression_with_crc failed with errno %d \t ret value = %d\n",errno,ret);
			return 1;
		}

		if(stanza_ptr->compare_flag){
			addr_8 = (unsigned long long *)t[ti].handler->all_buf[0].ea;
			end_addr = (unsigned long long *)((char *)addr_8 +t[ti].buffer_size);
			output_addr_8 = (unsigned long long *)t[ti].handler->all_buf[2].ea;
			intermediate_buff = (unsigned long long *)t[ti].handler->all_buf[1].ea;
			patt_size = stanza_ptr->pattern_size[pi];

			if((stanza_ptr->pattern_type[pi] >= PATTERN_SIZE_NORMAL) &&(stanza_ptr->pattern_type[pi] <= PATTERN_RANDOM)){

				ret = comp_bufs(addr_8, end_addr, output_addr_8, intermediate_buff, patt_size, pi, t[ti].buffer_size, ti);
				if(ret) {
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT," CDC: comp_bufs failed with ret value = %d\n",ret);
					return 1;
				}
			} /* END of if */
		}/* END of stanza_ptr->compare */
	}/* END of for stanza_ptr->num_oper */

	return (0);
}

int corsa_bypass_compare(ti)
{
	unsigned long long  *end_addr, *patt, *addr_8, *output_addr_8, patt_8, start_index, *rand_buff_addr, *intermediate_buff;
	struct th_ctx *t = nx.thread_ct;
	unsigned int pi = 0, patt_size; /* pi is patter_index initialised to zero */
	int num_buffers, buf_size, i, j, ret;
	int mode, bi;


	if(stanza_ptr->nx_async_flag){
		mode = 1;
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%d:mode selected is async \n", __LINE__);
	}
	else {
		mode = 0;
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%d:mode selected is sync \n", __LINE__);
	}

	buf_size = nx.thread_ct[ti].buffer_size;
	num_buffers = 3;

	nx.thread_ct[ti].handler = nx.thread_ct[ti].direct_buffer;
	/* Reset all buffers to clear data from prev runs. */
	for ( j=0; j<3; j++ ) {
		bzero((char *)nx.thread_ct[ti].handler->all_buf[j].ea, MAX_BUFF_SZ);
	}

	t[ti].shm_addr_offset = (unsigned long long*)((char *) shm_addr_start + (t[ti].shm_offset * MB));
	rand_buff_addr = t[ti].shm_addr_offset;
	displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"t[%d].shm_addr_offset = 0x%llx \n",ti,t[ti].shm_addr_offset);
	init_random_gen((unsigned int)stanza_ptr->seed);


	for (i = 0; i < stanza_ptr->num_oper; i++ ) {

		addr_8 = (unsigned long long *)t[ti].handler->all_buf[0].ea;
		end_addr = (unsigned long long *)((char *)addr_8 +t[ti].buffer_size);
		patt_size = stanza_ptr->pattern_size[pi];

		if (priv.exit_flag == 1) {
			displaym(HTX_HE_INFO,DBG_MUST_PRINT," corsa_bypass_compare(%d): received sigterm \n",ti);
			pthread_exit(0);
		}

		/* INITIALISING the BUFFERS with 0xDD before each opertion */
		for(bi = 0; bi <  num_buffers; bi++){
			memset((unsigned long long *)t[ti].handler->all_buf[bi].ea, 0xDD,  buf_size);
		}


		if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
			for(; addr_8 < end_addr; )  {

				patt = addr_8;
				patt_8 = (unsigned long long)patt;
				*addr_8 = patt_8 ;
				addr_8++;

			}
		}else if( stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
			if ((rand_buff_addr >= (unsigned long long*) shm_addr_end) || ((rand_buff_addr+(t[ti].buffer_size)) > (unsigned long long*) shm_addr_end)){

				rand_buff_addr = (unsigned long long*)shm_addr_start;

			}

			memcpy((char*)addr_8, (char*)rand_buff_addr, t[ti].buffer_size);
			rand_buff_addr+= (t[ti].buffer_size);


			/* Buffer Data Management of the shared segment*/
			if ((rand_buff_addr >= (unsigned long long*) shm_addr_end) || ((rand_buff_addr+(t[ti].buffer_size)) > (unsigned long long*) shm_addr_end)){

				rand_buff_addr = (unsigned long long*)shm_addr_start;

			}

		}else if((stanza_ptr->pattern_type[pi] == PATTERN_SIZE_NORMAL) ||(stanza_ptr->pattern_type[pi] == PATTERN_SIZE_BIG)){
			start_index = 0;
			patt = (unsigned long long *)stanza_ptr->pattern[pi];

			for(; addr_8 < end_addr; )  {
				patt_8 = *(unsigned long long *)(patt + (start_index % (patt_size / BIT_PATTERN_WIDTH)));
				*addr_8 = patt_8 ;
				addr_8++;
				start_index ++;
			}
		}

		t[ti].handler->i_buf = &(t[ti].handler->all_buf[0]);
		t[ti].handler->o_buf = &(t[ti].handler->all_buf[1]);
		t[ti].handler->inlen = buf_size;
		t[ti].handler->outlen_pass1 = buf_size;
		t[ti].handler->outlen_pass2 = buf_size;
		t[ti].handler->inlen_ptr = &(t[ti].handler->inlen);
		t[ti].handler->outlen_ptr = &(t[ti].handler->outlen_pass1);

		ret =  corsa_execute_algo( corsa_memcopy, NULL, t[ti].handler, 0, mode);
		if(ret != 0){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"corsa_execute_algo failed with errno %d \t ret value = %d\n",
				errno,ret);
			return 1;
		}

		if(stanza_ptr->compare_flag){

			addr_8 = (unsigned long long *)t[ti].handler->all_buf[0].ea;
			end_addr = (unsigned long long *)((char *)addr_8 +t[ti].buffer_size);
			output_addr_8 = (unsigned long long *)t[ti].handler->all_buf[1].ea;
			intermediate_buff = (unsigned long long *)t[ti].handler->all_buf[1].ea;
			patt_size = stanza_ptr->pattern_size[pi];

			if((stanza_ptr->pattern_type[pi] >= PATTERN_SIZE_NORMAL) &&(stanza_ptr->pattern_type[pi] <= PATTERN_RANDOM)){
				ret = comp_bufs(addr_8, end_addr, output_addr_8, intermediate_buff, patt_size, pi, t[ti].buffer_size, ti);
				if(ret){
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT," BYPASS: comp_bufs failed with ret value = %d\n",ret);
					return 1;
				}
			} /* END of if */

		}/* END of stanza_ptr->compare */

	}/* END of for stanza_ptr->num_oper */

	return(0);

} /* END of bypass compare */


int corsa_compress_only(ti)
{
	unsigned long long  *end_addr, *patt, *addr_8, patt_8, start_index, *rand_buff_addr;
	struct th_ctx *t = nx.thread_ct;
	unsigned int pi = 0, patt_size; /* pi is patter_index initialised to zero */
	int num_buffers, buf_size, num_i, j, ret, additional_buff;
	int mode, bi;

	if(stanza_ptr->nx_async_flag){
		mode = 1;
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"mode selected is async \n");
	}
	else {
		mode = 0;
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"mode selected is sync \n");
	}

	buf_size = nx.thread_ct[ti].buffer_size;
	num_buffers = 3;

	additional_buff = (buf_size * 0.15);     /* In case of RANDOM date we need more buff size for pass1 buffer */
	additional_buff = additional_buff - ((int)(buf_size * 0.15) % 8); /* making multiple of 8  byte */

	nx.thread_ct[ti].handler = nx.thread_ct[ti].direct_buffer;
	/* Reset all buffers to clear data from prev runs. */
	for ( j=0; j<3; j++ ) {
		bzero((char *)nx.thread_ct[ti].handler->all_buf[j].ea, 4096);
	}

	t[ti].shm_addr_offset = (unsigned long long*)((char *) shm_addr_start + (t[ti].shm_offset * MB));
	rand_buff_addr = t[ti].shm_addr_offset;
	displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"t[%d].shm_addr_offset = 0x%llx \n",ti,t[ti].shm_addr_offset);
	init_random_gen((unsigned int)stanza_ptr->seed);


	for (num_i = 0; num_i < stanza_ptr->num_oper; num_i++ ) {
		addr_8 = (unsigned long long *)t[ti].handler->all_buf[0].ea;
		end_addr = (unsigned long long *)((char *)addr_8 +t[ti].buffer_size);
		patt_size = stanza_ptr->pattern_size[pi];

		if (priv.exit_flag == 1) {
			displaym(HTX_HE_INFO,DBG_MUST_PRINT," corsa_compress_only(%d): received sigterm \n",ti);
			pthread_exit(0);
		}

		/* INITIALISING the BUFFERS with 0's before each opertion */
		for(bi = 0; bi <  num_buffers; bi++){
			memset((unsigned long long *)t[ti].handler->all_buf[bi].ea, 0xCC,  buf_size);
		}

		if( stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
			for(; addr_8 < end_addr; )  {

				patt = addr_8;
				patt_8 = (unsigned long long)patt;
				*addr_8 = patt_8 ;
				addr_8++;

			}
		}else  	if( stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
			if ((rand_buff_addr >= (unsigned long long*) shm_addr_end) || ((rand_buff_addr+(t[ti].buffer_size)) > (unsigned long long*) shm_addr_end)){

				rand_buff_addr = (unsigned long long*)shm_addr_start;

			}

			memcpy((char*)addr_8, (char*)rand_buff_addr, t[ti].buffer_size);
			rand_buff_addr+= (t[ti].buffer_size);


			/* Buffer Data Management of the shared segment*/
			if ((rand_buff_addr >= (unsigned long long*) shm_addr_end) || ((rand_buff_addr+(t[ti].buffer_size)) > (unsigned long long*) shm_addr_end)){

				rand_buff_addr = (unsigned long long*)shm_addr_start;

			}

		}else if((stanza_ptr->pattern_type[pi] == PATTERN_SIZE_NORMAL) ||(stanza_ptr->pattern_type[pi] == PATTERN_SIZE_BIG)){
			start_index = 0;
			patt = (unsigned long long *)stanza_ptr->pattern[pi];

			for(; addr_8 < end_addr; )  {
				patt_8 = *(unsigned long long *)(patt + (start_index % (patt_size / BIT_PATTERN_WIDTH)));
				*addr_8 = patt_8 ;
				addr_8++;
				start_index ++;
			}
		}

		t[ti].handler->i_buf = &(t[ti].handler->all_buf[0]);
		t[ti].handler->o_buf = &(t[ti].handler->all_buf[1]);
		t[ti].handler->inlen = buf_size;
		t[ti].handler->outlen_pass1 = buf_size + additional_buff;
		t[ti].handler->outlen_pass2 = buf_size + additional_buff;
		t[ti].handler->inlen_ptr = &(t[ti].handler->inlen);
		t[ti].handler->outlen_ptr = &(t[ti].handler->outlen_pass1);

		ret =  corsa_execute_algo( corsa_deflate, NULL, t[ti].handler, 0, mode);
		if(ret != 0){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s:corsa_execute_algo compression failed with errno %d \t ret value = %d\n",
				__FUNCTION__, errno,ret);
			return 1;
		}
	}/* END of for stanza_ptr->num_oper */


	return(0);

}/* END of corsa_compress_only */

int create_shm(unsigned long long *addr)
{

	/* Set up shared memory for filling in random numbers */
	shm_flg = (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

	/* creation of shared segment */
	shm_id = shmget(IPC_PRIVATE, SEG_SIZE, shm_flg);
	if ( shm_id == -1 ) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"\n Error in creating shared segment errno:%d", errno);
		return(-1);
	} else  {
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"\n Shared segment created success shmid = %d\n",shm_id);
	}

	/* Attaching to shared memory */
	shm_addr = (unsigned long long *) shmat(shm_id, 0, 0);
	addr = shm_addr;
	if ( addr == (unsigned long long*)-1 ) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT," Error while attaching to the shared segment errno:%d", errno);
		return(-1);
	}
	shm_addr_start = shm_addr;
	shm_addr_end = (unsigned long long*) ((char *) shm_addr + SEG_SIZE);

	return(0);
}

int fill_random_data(void *addr, unsigned int shm_size)
{
	unsigned long long *addr_8, *end_addr;
	addr_8 = (unsigned long long *)addr;
	end_addr = (unsigned long long *) ((char *)addr + shm_size);
	displaym(HTX_HE_INFO,DBG_DEBUG_PRINT," fill_random_data: addr_8 = 0x%llx , end_addr = 0x%llx \n", addr_8, end_addr);

	/* Initialising random number */
	init_random_gen(TIME_SEED);

	for(; addr_8 < end_addr; ) {
		*addr_8 = get_rand_64();
		addr_8++;
	}

	/* debug loop */
	addr_8 = (unsigned long long *)addr;
	for(; addr_8 < end_addr; ) {
		addr_8++;
	}

	return(0);
}


int deallocate_corsa_memory(){
	int i, rc, task_no;
	displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"In side deallocate_corsa_memory \n");

	/* release all the buffers */
	for ( i=0; i<nx.number_of_threads; i++ ) {
		if ( nx.thread_ct[i].direct_buffer != NULL ) {
			rc = corsa_release_buffer(nx.thread_ct[i].direct_buffer);
			if ( rc ) {
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"release_buffer of direct_buffer failed for thread#%d with rc=%d\n", i, rc);
			}
		}
	}

	if(nx.thread_ct){
		free(nx.thread_ct);
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"deallocate_corsa_memory: free of nx.thread_ct DONE \n");
	}

	for(task_no = 0; task_no < stanza_ptr->number_of_nx_task; task_no++) {
		if(nx.cpu_task[task_no].cpu_list ){

			free(nx.cpu_task[task_no].cpu_list);
			displaym(HTX_HE_INFO,DBG_IMP_PRINT,"deallocate_corsa_memory: free of nx.cpu_task[task_no].cpu_list DONE \n");
		}

	}
	if(shm_addr){
		rc =  shmdt(shm_addr);
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"deallocate_corsa_memory:shmdt DONE \n");
		if(rc != 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"deallocate_corsa_memory: errno (%d) detaching shared memory segment rc=%d\n",errno,rc);
		}

		rc = shmctl(shm_id, IPC_RMID, (int) NULL);
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"deallocate_corsa_memory:RMID DONE \n");
		if(rc != 0){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"deallocate_corsa_memory: errno (%d) deleting IPC_RMID shared memory segment rc=%d\n",errno,rc);
		}
	}
	return(0);
}


#if 0
/***********************************************************************
* This function does the binding of the process to the logical processor
* specified by priv.bind_proc variable which is filled while reading
* command line parameters.
* id - 0 (BIND_TO_PROCESS), 1 (thread bind)
* bind_proc = logical processor number or -1 (to unbind)
***********************************************************************/
int bind_to_proc(int bind_proc)
{
	int rc=0;
	pthread_t          tid;
	int                ti;

	ti = bind_proc;

	if (bind_proc != UNBIND_ENTITY) {
		bind_proc = bind_proc%get_num_of_proc();
	}

	if (bind_proc == UNBIND_ENTITY) {
		bind_proc = PROCESSOR_CLASS_ANY;
	} else {
		tid = thread_self();
		rc = bindprocessor(BINDTHREAD, tid, bind_proc);
		nx.thread_ct[ti].kernel_tid = tid;
	}
	nx.thread_ct[ti].bind_proc = bind_proc;

	if (rc != 0) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Warning! error binding "
				 "to processor %d.", bind_proc);
	} else {
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"Successfully binded thread "
				"(tid =%d) to the logical proc %d\n", ti, bind_proc);
	}

	return(rc);
}
#endif

/*
 * This function will initialize the random number generator with the seed from the test case.
 * The buffer used here needs to be declared in the test case datastructure along with seed n
 * the used(&buffer) needs to be replaced with proper call (&sdata->...->tc_ptr->buffer).
 * Also, this function needs to be called ONCE ONLY and not each time random number has to be
 * generated. Thats y its kept as a separate function.
 *
 * srand48_r is a thread safe libc call.
 */
void init_random_gen(int original_seed)
{
	time_t cur_time;
	unsigned long seed;
	time(&cur_time);

	if(original_seed == TIME_SEED){
		seed = (unsigned  long) cur_time;
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"original_seed == -2 so using lrand48 seed = %ul \n",seed);
	}
	else{
		seed = (unsigned  long) original_seed;
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"using supplied seed ==  %u\n",seed);
	}

	srand48_r(seed, &rand_buf);
}


/*
 * This function generates the actual random number. This is in-turn used in 64-bit random no
 * generator function, which is eventually used in 128-bit random number generator sunroutine.
 *
 * The mrand48_r is a thread safe version of mrand48 and is a libc subroutine. It has a range
 * eventually distributed over [-2^31 to 2^31].
 */
u32 get_rand_32(void)
{
	long int temp;

	lrand48_r(&rand_buf, &temp);
	return ((u32)temp);
}


/*
 * This function generates 64-bit random number. The logic here is to generate 2 32-bit random
 * number using get_rand_32() and append one after the other.
 */
u64 get_rand_64(void)
{
	long int tmp;
	u64 rand_no;

	tmp = get_rand_32();

	rand_no = ((u64)tmp << 32);
	rand_no |= get_rand_32();

	return (rand_no);
}

/* random number generator */
int gen_rand_no(void)
{
	int randy;
	unsigned long long Gen_seed = get_rand_64();
	randy = (Gen_seed*25173+13849)%65536;
	return (randy>0)? randy : (randy*(-1)); /* to be possitive*/
	if (Gen_seed%10 < 3){
		Gen_seed--;
	}
}

int gen_random_range(int min, int max)/* generate a random number in a range */
{
	int randy;
	if (min>max) {
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"gen_random_range is called with min %d > max %d returning min\n", min,max);
		return min;
	}else if(min == max){
		return min;
	}
	randy = gen_rand_no()%(max-min+1) + min;
	return randy;
} /* Random DFP number */


int gen_random_range_32(int min, int max)/* generate a random number of multiple of 32 in a range */
{
	int randy;
	if (min>max) {
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"gen_random_range_32 is called with min %d > max %d returning min\n", min,max);
		if ( min % 8 ) { 
			min = min + ( 8 - (min % 8));
		}
		return min;
	}else if(min == max){
		if ( min % 8 ) {
			min = min + ( 8 - (min % 8));
		}
		return min;
	}
	randy = (gen_rand_no()%(max-min+1)) + min;
	randy = randy + (8 - (randy % 8));
	return randy;
}

/* This routine will return the number of threads when  specified the thread percent *
 * for a particular chip or DEFAULT chips. 					 						 *
 * returns number of chips on success or returns -1 on failure 						 */

int get_num_th(int chip_num,float percentage)
{
	int threads = 0;
	int chip_no = chip_num;

	/* Converting thread percent per operation in to number of threads per opation  *
	 * and populating it in chip struct.											*/

	if(percentage < 0 || percentage > 100){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_num_th: wrong percentage passed %d \n",percentage);
		return -1;
	}

	if(chip_no >=0 && chip_no < MAX_P7_CHIP){
	/* Here we have to convert percent to number of threads for a SPECIFIC chip */

		threads = (int) (percentage * nx.chip[chip_no].total_cpus_per_chip) / 100;
	}
	else if(chip_no == -1){
	/* Here we have to convert percent to number of threads for all DEFAULT CHIPS */

		for(chip_no = 0;chip_no < Sys_stat.chips; chip_no++) {

			if(nx.chip[chip_no].chip_rule == DEFAULT_RULE){
				threads += (int) (percentage * nx.chip[chip_no].total_cpus_per_chip) / 100;
			}

		}

	}
	else{
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_num_th: wrong chip number %d \n",chip_num);
		return -1;
	}

	return(threads);

} /* END of get_num_th */



/* This routine update the cpu_task's cpu_list given the chip_no task_no and number of threads as input *
 * returns 0 on success or returns -1 on failure 														*/

int get_cpus(int chip_num, int task_num, float percentage)
{

	int i, chip_save, task_save;
	int chip_no = chip_num;
	int task_no = task_num;
	int num_of_threads;

	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"chip_num = %d task_num = %d",chip_num, task_num);

	if(chip_no >=0 && chip_no < MAX_P7_CHIP){
	/* Here we have to populate cpu_list for SPECIFIC chip in task*/

		num_of_threads = get_num_th(chip_no, percentage);
		chip_save = nx.chip[chip_no].cpu_index;
		task_save = nx.cpu_task[task_no].cpu_index;

		for(i = 0; i < num_of_threads; i++){
			nx.cpu_task[task_no].cpu_list[task_save + i] = nx.chip[chip_no].cpus_in_chip[chip_save + i];
		}
		nx.chip[chip_no].cpu_index = chip_save + i;
		nx.cpu_task[task_no].cpu_index = task_save + i;

	}
	else if(chip_no == -1){
	/* Here handling DEFAULT_RULE CASE */

		for(chip_no = 0;chip_no < Sys_stat.chips; chip_no++) {

			if(nx.chip[chip_no].chip_rule == DEFAULT_RULE){

				num_of_threads = get_num_th(chip_no, percentage);
				chip_save = nx.chip[chip_no].cpu_index;
				task_save = nx.cpu_task[task_no].cpu_index;

				for(i = 0; i < num_of_threads; i++){

					nx.cpu_task[task_no].cpu_list[task_save + i] = nx.chip[chip_no].cpus_in_chip[(chip_save + i)];

				}
				nx.chip[chip_no].cpu_index = chip_save + i;
				nx.cpu_task[task_no].cpu_index = task_save + i;

			}

		}

	}
	else{
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_cpus: wrong chip number %d \n",chip_num);
		return -1;
	}

	return(0);


}/* END of get_cpus */


int comp_bufs(unsigned long long *input_addr_8,unsigned long long *end_addr, unsigned long long *output_addr_8,
     	unsigned long long *intermediate_buffer, unsigned int patt_size, unsigned int pi, unsigned int buffer_size, int ti)
{

	int miscompare_cnt = 0;
	unsigned int offset = 0;
	unsigned long long original_value, output_value;
	unsigned long long *input_addr, *output_addr;

	for(input_addr=input_addr_8, output_addr=output_addr_8; input_addr < end_addr; input_addr++, output_addr++ )  {
		original_value = *input_addr;
		output_value = *output_addr;

		if ((original_value != output_value ) && (stanza_ptr->misc_crash_flag == 1)){
			offset = output_addr - output_addr_8;

			/*	trap(0xBEEFDEAD, offset, output_start_addr, input_start_addr, output_addr_8,&buff_size, &(stanza_ptr->pattern_name[pi]),&(stanza_ptr->rule_id));

			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"Found Miscompare Expected value = 0x%llx \t"
			" Actual value = 0x%llx \t  at offset = %u \t output_start_addr= 0x%llx \t input_start_addr= 0x%llx\t"
			" output_addr_8 = 0x%llx \n"
			,original_value, output_value, offset, output_start_addr, input_start_addr, output_addr_8 );
			*/

			displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Found Miscompare Expected value = 0x%llx \t"
			" Actual value = 0x%llx \t  at offset = %u \t output_start_addr= 0x%llx \t input_start_addr= 0x%llx\t"
			" output_addr = 0x%llx \n"
			,original_value, output_value, offset, input_addr_8, output_addr_8, output_addr);

			miscompare_cnt ++;
			/* break; */
		}

	}/* END of for addr_8 */

	if( miscompare_cnt != 0){
		save_bufs((char*)input_addr_8, (char *)output_addr_8, (char *)intermediate_buffer, offset, miscompare_cnt, buffer_size,ti,pi);
	}

	return(miscompare_cnt);

}


int save_bufs(char *input_start_addr, char *output_start_addr, char *intermediate_buffer,unsigned int offset,\
	int miscompare_cnt, unsigned int buff_size, int tn, unsigned int pi)
{

	int operation;
	char fname[128], host_name[50], msg[1000];
	sprintf(fname,"/tmp/hxecorsa_miscompare.%d.%d",tn,getpid());
	FILE *fp;
#if 0
	char *buff1 = nx.thread_ct[tn].handler->all_buf[0].ea, *buff2 = nx.thread_ct[tn].handler->all_buf[1].ea;
	char *buff3 = nx.thread_ct[tn].handler->all_buf[2].ea;
#endif

	char *buff1=input_start_addr;
	char *buff2=intermediate_buffer;
	char *buff3=output_start_addr;

	sprintf(msg,"Dump file generated @%s",fname);
	hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR,msg);

	fp = fopen(fname, "w");
	if ( fp == NULL) {
		sprintf(msg, "Error opening nx_mem_log file \n");
		hxfmsg(&stats, -1, HTX_HE_HARD_ERROR, msg);
		exit(-1);
	}

	get_hostname(host_name);
	fprintf(fp, "################################################################################\n");
	fprintf(fp, "#                                                                              #\n");
	fprintf(fp, "#                         Hxecorsa Exerciser Dump File                         #\n");
	fprintf(fp, "#                                                                              #\n");
	fprintf(fp, "################################################################################\n\n");
	fprintf(fp, "===========================Common Configuration Data============================\n");

	fprintf(fp, "CPU PVR value      : 0x%08lx\n", (unsigned int) get_true_cpu_version());
	fprintf(fp, "Failing PID        : %ull\n", getpid());
	fprintf(fp, "Failing Thread no  : %d\n", tn);

	fprintf(fp, "\n Thread specific data");
	fprintf(fp, "\n thread_num = %u \n",nx.thread_ct[tn].thread_num);
	fprintf(fp, "\n tid = %d",nx.thread_ct[tn].tid);
	fprintf(fp, "\n number of operation = %u",nx.thread_ct[tn].num_oper);
	fprintf(fp, "\n crc = %u",nx.thread_ct[tn].crc);
	fprintf(fp, "\n buffer_size = %u",nx.thread_ct[tn].buffer_size);
	fprintf(fp, "\n seed = %d",nx.thread_ct[tn].seed);
	fprintf(fp, "\npattern_name = %s \t rule_id = %s \n",stanza_ptr->pattern_name[pi],stanza_ptr->rule_id);

	operation = nx.thread_ct[tn].operation;

	if(operation == 0)
		fprintf(fp, "\n operation = CDC-CRC ");
	else if(operation ==1)
		fprintf(fp, "\n operation = CDC");
	else if(operation ==2)
		fprintf(fp, "\n operation = BYPASS");
	else if(operation ==3)
		fprintf(fp, "\n operation = COMPRESS-CRC-ONLY ");
	else if(operation ==4)
		fprintf(fp, "\n operation = COMPRESS-ONLY ");

	fprintf(fp,"miscompare offset = %u \n",offset);
	fprintf(fp,"Input buffer start addr = 0x%llx \n",buff1);
	fprintf(fp,"output buffer start addr =  0x%llx \n",buff3);

	fprintf(fp, "\n \n Plain text dump: Input");
	fprintf(fp, "\n ***********************");
	hex_dump(fp, buff1, buff_size);
	fflush(fp);

	fprintf(fp, "\n \n Intermediate buffer: PASS1 OUTPUT");
	fprintf(fp, "\n **********************************");
	hex_dump(fp, buff2, buff_size);
	fflush(fp);

	if (operation ==0 || operation == 1){
		fprintf(fp, "\n\n Final buffer : PASS2 OUTPUT DECOMPRESSION");
		fprintf(fp, "\n ****************************");
		hex_dump(fp, buff3, buff_size);
		fflush(fp);
	}

	fprintf(fp, "\n\n Control Buffer : ");
	fprintf(fp, "\n *****************");
	hex_dump(fp, &(nx.thread_ct[tn].direct_buffer->corsa_buf), sizeof(nx.thread_ct[tn].direct_buffer->corsa_buf));

	if (fclose(fp)) {
		sprintf(msg, "[%s:%d] fclose failed\n", __FUNCTION__, __LINE__);
		hxfmsg(&stats, -1, HTX_HE_SOFT_ERROR, msg);
	}

	return(0);

}

#if 0
/* This function calculates bandwidth and latency and repots the same in the file nx_mem_performance.out */

int report_latency()
{

	int ti, ni, tn, i, j;
	struct th_ctx *t = nx.thread_ct;
	FILE *fp;
	char file_name[50];
	unsigned int temp_long;
	double average_compress_cumulative_time = 0, average_decompress_cumulative_time = 0;
	double average_compress_bandwidth = 0, average_decompress_bandwidth = 0, temp_double = 0;
	double 	max_compress_time = 0, min_compress_time = 0, max_decompress_time = 0, min_decompress_time = 0;
	unsigned int average_compress_cumulative_data = 0, average_decompress_cumulative_data = 0;
	/* Here we are calculating bandwidth for each thread for num_of_operations (say 10) */

	for (tn = 0; tn < nx.cpu_task[0].num_of_threads ; tn ++){

		if( nx.thread_ct[tn].thread_flag != THREAD_ACTIVE){
			continue;
		}

		ti = tn;

		for (ni = 0; ni < t[ti].num_oper; ni++){

			t[ti].compress_bw.total_read_data += t[ti].compress_time[ni].read_data;
			t[ti].compress_bw.total_write_data += t[ti].compress_time[ni].write_data;
			t[ti].compress_bw.cumulative_time += t[ti].compress_time[ni].latency;

			t[ti].decompress_bw.total_read_data += t[ti].decompress_time[ni].read_data;
			t[ti].decompress_bw.total_write_data += t[ti].decompress_time[ni].write_data;
			t[ti].decompress_bw.cumulative_time += t[ti].decompress_time[ni].latency;
		}

		t[ti].compress_bw.cumulative_data = t[ti].compress_bw.total_read_data + t[ti].compress_bw.total_write_data;
		t[ti].compress_bw.bandwidth = (double)t[ti].compress_bw.cumulative_data / t[ti].compress_bw.cumulative_time;
		average_compress_cumulative_time += t[ti].compress_bw.cumulative_time;
		average_compress_cumulative_data += t[ti].compress_bw.cumulative_data;

		t[ti].decompress_bw.cumulative_data = t[ti].decompress_bw.total_read_data + t[ti].decompress_bw.total_write_data;
		t[ti].decompress_bw.bandwidth = (double)t[ti].decompress_bw.cumulative_data / t[ti].decompress_bw.cumulative_time;
		average_decompress_cumulative_time += t[ti].decompress_bw.cumulative_time;
		average_decompress_cumulative_data += t[ti].decompress_bw.cumulative_data;
	}

	average_compress_cumulative_time = average_compress_cumulative_time /(double) nx.cpu_task[0].num_of_threads;
	average_compress_cumulative_data = average_compress_cumulative_data / nx.cpu_task[0].num_of_threads;

	average_decompress_cumulative_time = average_decompress_cumulative_time /(double) nx.cpu_task[0].num_of_threads;
	average_decompress_cumulative_data = average_decompress_cumulative_data / nx.cpu_task[0].num_of_threads;


	max_compress_time = t[0].compress_bw.cumulative_time;
	min_compress_time = t[0].compress_bw.cumulative_time;
	for(i = 0; i <  nx.cpu_task[0].num_of_threads; i++){

		/* displaym(HTX_HE_INFO,DBG_INFO_PRINT,"@@@ i = %d, j = %d t[i].compress_bw.cumulative_time = %.10f \
		\n",i,j, t[i].compress_bw.cumulative_time); */
		if(max_compress_time < t[i].compress_bw.cumulative_time){
			max_compress_time = t[i].compress_bw.cumulative_time;
		}

		if(min_compress_time > t[i].compress_bw.cumulative_time)	 {
			min_compress_time = t[i].compress_bw.cumulative_time;
		}
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"@@@ max_compress_time = %.10f \t min_compress_time = %.10f \n",
		max_compress_time, min_compress_time);

	}/* END of for loop i */

	max_decompress_time = t[0].decompress_bw.cumulative_time;
	min_decompress_time = t[0].decompress_bw.cumulative_time;
	for(i = 0; i <  nx.cpu_task[0].num_of_threads; i++){

		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"@@@ i = %d, j = %d t[i].decompress_bw.cumulative_time = %.10f \
		\n",i,j, t[i].decompress_bw.cumulative_time);
		if(max_decompress_time < t[i].decompress_bw.cumulative_time){
			max_decompress_time = t[i].decompress_bw.cumulative_time;
		}

		if(min_decompress_time > t[i].decompress_bw.cumulative_time)     {
			min_decompress_time = t[i].decompress_bw.cumulative_time;
		}
		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"@@@ max_decompress_time = %.10f \t min_decompress_time = %.10f \n",
		max_decompress_time, min_decompress_time);

	}/* END of for loop i */

	/* Now we have all the data populated in the data structures need to put all the infor mation in a file */

	sprintf(file_name, "/tmp/nx_mem_performance_app.out");
	fp = fopen(file_name, "a");
	if(fp == NULL){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"fopen failed \n");
		return -1;
	}
	displaym(HTX_HE_INFO,DBG_MUST_PRINT," %s file is creted for nx mem compress/decompress performance data \n",file_name);
	fprintf(fp, "################################################################################\n");
	fprintf(fp, "#                                                                              #\n");
	fprintf(fp, "#        nx_mem compress/decompress performance data                           #\n");
	fprintf(fp, "#                                                                              #\n");
	fprintf(fp, "################################################################################\n\n");
	fprintf(fp, "===========================stanza = %s \t pattern = %s===================================\n",
	stanza_ptr->rule_id, stanza_ptr->pattern_name[0]);

	ni = 0;
	for (tn = 0; tn < nx.cpu_task[0].num_of_threads ; tn ++){

		if( nx.thread_ct[tn].thread_flag != THREAD_ACTIVE){
			 displaym(HTX_HE_INFO,DBG_INFO_PRINT,"\n tn = %d is not THREAD_ACTIVE \n",tn);
			continue;
		}

		ti = tn;

		fprintf(fp,"\n \n \n======================Performance data for the thread number %d=======================\n",ti);
		fprintf(fp,"Bandwidth is calculated over %d number of operations of buffer size %d \n",t[ti].num_oper, \
			t[ti].compress_time[ni].read_data);
		fprintf(fp," pattern = %s \t rule_id = %s \n",stanza_ptr->pattern_name[0],stanza_ptr->rule_id);

		fprintf(fp,"\n-----------------data for compression nx operation-------------------\n");
		fprintf(fp,"bandwidth:-\n");
		fprintf(fp,"cumulative_time = %.10f secs \n",t[ti].compress_bw.cumulative_time);
		fprintf(fp,"read_data = %u bytes\t write_data = %u bytes\t cumulative_data = %u bytes\n",\
		t[ti].compress_bw.total_read_data, t[ti].compress_bw.total_write_data, t[ti].compress_bw.cumulative_data);
		fprintf(fp,"bandwidth = %.10f \n",t[ti].compress_bw.bandwidth);

		fprintf(fp,"\nlatency:-\n");
		temp_long = t[ti].compress_bw.cumulative_data / t[ti].num_oper;
		temp_double = t[ti].compress_bw.cumulative_time / (double)  t[ti].num_oper;
		fprintf(fp,"latency calculated for data %u bytes\n",temp_long);
		fprintf(fp,"latency =  %.10f secs \n",temp_double);


		fprintf(fp,"\n-----------------data for decompression nx operation------------------\n");
		fprintf(fp,"bandwidth:-\n");
		fprintf(fp,"cumulative_time = %.10f secs \n",t[ti].decompress_bw.cumulative_time);
		fprintf(fp,"read_data = %u bytes\t write_data = %u bytes\t cumulative_data = %u bytes\n",\
		t[ti].decompress_bw.total_read_data, t[ti].decompress_bw.total_write_data, t[ti].decompress_bw.cumulative_data);
		fprintf(fp,"bandwidth = %.10f \n",t[ti].decompress_bw.bandwidth);

		fprintf(fp,"\nlatency:-\n");
		temp_long = t[ti].decompress_bw.cumulative_data / t[ti].num_oper;
		temp_double = t[ti].decompress_bw.cumulative_time / (double)  t[ti].num_oper;
		fprintf(fp,"latency calculated for data %u bytes\n",temp_long);
		fprintf(fp,"latency =  %.10f secs \n",temp_double);

	}
	fprintf(fp,"\n===========================Average performance data for %d threads====================================\n",
	nx.cpu_task[0].num_of_threads);

	fprintf(fp,"\n-----------------------Average data for compression operation---------------------\n");
	fprintf(fp," max_compress_cumulative_time = %.10f secs \n",max_compress_time);
	fprintf(fp," min_compress_cumulative_time = %.10f secs \n",min_compress_time);
	average_compress_bandwidth = (double)average_compress_cumulative_data / average_compress_cumulative_time;
	fprintf(fp,"Average compress_cumulative_data = %u bytes\n",average_compress_cumulative_data);
	fprintf(fp,"Average compress_cumulative_time =  %.10f secs\n",average_compress_cumulative_time);
	fprintf(fp,"\tAverage bandwitdh = %.10f \n",average_compress_bandwidth);

	fprintf(fp,"\n-----------------------Average data for decompression operation---------------------\n");
	fprintf(fp," max_decompress_cumulative_time = %.10f secs \n",max_decompress_time);
	fprintf(fp," min_decompress_cumulative_time = %.10f secs \n",min_decompress_time);
	average_decompress_bandwidth = (double)average_decompress_cumulative_data / average_decompress_cumulative_time;
	fprintf(fp,"Average decompress_cumulative_data = %u bytes \n",average_decompress_cumulative_data);
		fprintf(fp,"Average decompress_cumulative_time =  %.10f secs \n",average_decompress_cumulative_time);
	fprintf(fp,"\tAverage bandwitdh = %.10f \n",average_decompress_bandwidth);
	fprintf(fp,"\n ==================================================================================================\n");

	fclose(fp);

	return(0);
}/* END of report_latency */
#endif


void  dump_bufs(int ti, char *oper, char *buff1, char *buff2, char* buff3, int buff_size)
{

	char dump_file[100], msg[1000];
	FILE *fp;

	sprintf(dump_file, "/tmp/nx_mem_dump");
	sprintf(msg,"Dump file generated @%s",dump_file);
	hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR,msg);

	fp = fopen(dump_file, "a");
	if ( fp == NULL) {
		sprintf(msg, "Error opening nx_mem_log file \n");
		hxfmsg(&stats, -1, HTX_HE_HARD_ERROR, msg);
		exit(-1);
	}

	fprintf(fp, "\n");
	fprintf(fp, "\n operation : %s\n",oper);


	if(buff1){
		fprintf(fp, "\n \n Plain text dump: Input");
		fprintf(fp, "\n ***********************");
		hex_dump(fp, buff1, buff_size);
		fflush(fp);
	}

	if(buff2){
		fprintf(fp, "\n Intermediate buffer: PASS 1 OUTPUT COMPRESSION");
		fprintf(fp, "\n **********************************");
		hex_dump(fp, buff2, buff_size);
		fflush(fp);
	}

	if(buff3){
		fprintf(fp, "\n\n Final buffer : PASS2 OUTPUT DECOMPRESSION");
		fprintf(fp, "\n ****************************");
		hex_dump(fp, buff3,buff_size);
		fflush(fp);
	}

	fclose(fp);

}


th_context * corsa_get_buffer(int no_buf, int buf_size, int alignment)
{
	int i, temp_retries;
	int max_buf_size;
	char info_msg[1024];
	th_context *node;
	buf_addr *temp_buf;
	char *local_ptr;
	int rc=-1;

	/* LDPRINT("[%d]:Coming to %s for dev=%s, no_buf=%d, buf_size=%d, alignment=%d\n", __LINE__, __FUNCTION__, stats.HE_name, no_buf, buf_size, alignment); */

	/* Argument checking */
	if  ( no_buf > MAX_BUFF ) {
		sprintf(info_msg, "%s[%d]:Can not allocate more than %d (MAX_BUFF) buffers. Usage error. Returning.\n", __FUNCTION__, __LINE__, MAX_BUFF);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
		return(NULL);
	}

	if ( buf_size <= 0 ) {
		sprintf(info_msg, "%s[%d]:Buffer size can not be <= 0. Usage error. Returning.\n", __FUNCTION__, __LINE__);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
		return(NULL);
	}

	if ( alignment < 0 ) {
		sprintf(info_msg, "%s[%d]:Alignment specified is negative. Usage error. Returning.\n", __FUNCTION__, __LINE__);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
		return(NULL);
	}


	/* Working code */
	node=malloc(sizeof(th_context));
	if ( node == NULL ) {
		sprintf(info_msg, "%s[%d]:Allocation for node th_context failed with errno %d(%s)\n", __FUNCTION__, __LINE__, errno, strerror(errno));
		hxfmsg(&stats, 1, HTX_HE_INFO, info_msg);
		LDPRINT("node allocation failed with errno:%d\n", errno);
		return(NULL);
	}

	MALLOC_PRINT("Memory allocated for node. node addr = %llx, size = %d\n", node, sizeof(th_context));
	LDPRINT("Memory allocated for node. node addr = %llx\n", node);
	memset(node, 0, sizeof(th_context));

	if ( alignment ) {
		max_buf_size = ( buf_size + (alignment-1) );
	}
	else {
		max_buf_size = buf_size;
	}

	/* Data buffers */
	for ( i=0; i<no_buf; i++ ) {
		temp_buf = &(node->all_buf[i]);

		/* Allocate memory */
		temp_buf->buf_size = buf_size;
		temp_buf->alloc_size = max_buf_size;
		temp_buf->alignment = alignment;

		local_ptr = malloc(max_buf_size);
		rc = mlock(local_ptr, max_buf_size);
		if ( rc ) {
			sprintf(info_msg, "%s[%d]: mlock failed with errno=%d(%s). rc=%d", errno, strerror(errno), rc);
			hxfmsg(&stats, 1, HTX_HE_HARD_ERROR, info_msg);
			return (-1);
		}

		MALLOC_PRINT("Memory allocated for buffer[%d]. buff addr = %llx, size = %d\n", i, local_ptr, max_buf_size);
		temp_buf->ea_org = local_ptr;

		if ( alignment ) {
			local_ptr = (char *)((u64)((u64)local_ptr + (u64)(alignment-1)) & (u64)(~((u64)alignment - 1)));
		}
		temp_buf->ea = local_ptr;

		/*LDPRINT("node[%llx]->all_buf[%d]->ea_org=%llx, ea=%llx, size=%d, align=%d\n", node, i,
			node->all_buf[i].ea_org, node->all_buf[i].ea, node->all_buf[i].buf_size, node->all_buf[i].alignment); */

	}


	/* Open Corsa device (as I/O) and save fd, to be used in ioctl() for execution.
	 * Also handle EEH.
	 */
	temp_retries = eeh_retries;
	while ( temp_retries > 0 ) {
	#ifndef __HTX_LINUX__
		node->corsa_fd = open(stats.sdev_id, O_RDONLY, S_IRUSR /*ignored by corsa*/);
	#else
		/* Same exerciser is used for both PCIe Gzip corsa(A5)  and CAPI Gzip corsa(A7).
		 * CAPI corsa only supports compression/decompression and hence FD is not required as
		 * there are no IOCTLs to eb performed. 
		 * So, if the device name has cxl (CAPI corsa) then dont open() the device. 
		 */

		if ( strncmp(&(stats.sdev_id[5]), "genwqe", 6) == 0 ) { /* PCIe Corsa */
			node->corsa_fd = open(stats.sdev_id, O_RDWR);
		}
	#endif
		if ( node->corsa_fd != -1 )
			break;

		temp_retries--;
		sleep(EEH_SLEEP);
	}
	if ( node->corsa_fd == -1 ) {
		sprintf(info_msg,"%s[%d]:open() failed for %s with %d(%s).\nCan not run on hardware. Testing can be done using standard zlib in software.",
		__FUNCTION__, __LINE__, stats.sdev_id, errno, strerror(errno));
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
	}


	return(node);
}

#ifndef __HTX_LINUX__
int memcopy_buffer(th_context *th_ct, int mode_of_oper)
{
	int rc=0, temp_retries;
	char info_msg[1024];

	/* Copy EEH retries to local var. */
	temp_retries = eeh_retries;

	/* Zero out entire accel_enq_cmd_t structure before using it */
	memset((void *)&(th_ct->corsa_buf), 0, sizeof(accel_enq_cmd_t));
	th_ct->corsa_buf.ac_func = 0x01;

	th_ct->corsa_buf.cmd = 0x03;
	th_ct->corsa_buf.cmdopts = 0;

	if ( mode_of_oper ) {
		/* Async mode */
		th_ct->corsa_buf.in_flags = ACCEL_FLG_ASYNC;
	} else {
		/* Sync mode */
		th_ct->corsa_buf.in_flags = 0; /* sync  job */
	}

	th_ct->corsa_buf.timeout = 30; /* No timeout, wait forever */

	/* Input buffer */
	ATS_SET_TYPE(&(th_ct->corsa_buf.app.ats), 0, ATS_FLAT_R);
	th_ct->corsa_buf.app.asiv.as_uint64s[0] = (unsigned long long)th_ct->i_buf->ea; /* start address of input data */
	th_ct->corsa_buf.app.asiv.as_uint32s[2] = *(th_ct->inlen_ptr); /* Size of input data. */
	th_ct->corsa_buf.app.asiv.as_uint32s[3] = 0;

	/* Output buffer */
	ATS_SET_TYPE(&(th_ct->corsa_buf.app.ats), 2, ATS_FLAT_RW);
	th_ct->corsa_buf.app.asiv.as_uint64s[2] = (unsigned long long)th_ct->o_buf->ea; /* start address of output data */
	th_ct->corsa_buf.app.asiv.as_uint32s[6] = *(th_ct->outlen_ptr);
	th_ct->corsa_buf.app.asiv.as_uint32s[7] = 0;


	/* Issue ioctl() to corsa */
	while ( temp_retries > 0 ) {
		rc = ioctl(th_ct->corsa_fd, ACCEL_ENQ_CMD, (void *)&(th_ct->corsa_buf));
		if ( rc == 0 ) {
			break;
		}
		if ( rc && temp_retries ) {
			temp_retries--;
			sleep(EEH_SLEEP);
			continue;
		}
	}
#if 1

	if (rc) {
	sprintf(info_msg, "/tmp/hxecorsa_miscompare.%d.%d", getpid(), th_ct->corsa_fd);
	FILE *fp;
	fp = fopen(info_msg, "w");
	sprintf(info_msg, "Output buffers dumped in : %s\n", info_msg);
	hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);

	fprintf(fp, "\n\nAfter ioctl.\n=============\n\naccel_enq_command_t\n-------------------\n");
	hex_dump(fp, (const unsigned char *)&(th_ct->corsa_buf), sizeof(th_ct->corsa_buf));

	fprintf(fp, "\napp_info_t\n----------\n");
	hex_dump(fp, (const unsigned char *)&(th_ct->corsa_buf.app), sizeof(th_ct->corsa_buf.app));

	fprintf(fp, "\n Input bytes processed : %d\n", th_ct->corsa_buf.app.ibdc);
	fprintf(fp, "\n Output bytes generated : %d\n", th_ct->corsa_buf.app.obdc);

	fprintf(fp, "\nInput buffer\n------------\n");
	hex_dump(fp, (const unsigned char *)(th_ct->i_buf->ea), *(th_ct->inlen_ptr));
	/* hex_dump(fp, (const unsigned char *)ip, 2*KB); */
	fprintf(fp, "\nOutput buffer\n-------------\n");
	hex_dump(fp, (const unsigned char *)(th_ct->o_buf->ea), *(th_ct->inlen_ptr));
	/* hex_dump(fp, (const unsigned char *)op, 64*KB); */
	fprintf(fp, "\nOutput buffer\n-------------\n");
	hex_dump(fp, (const unsigned char *)(th_ct->o_buf->ea), *(th_ct->inlen_ptr));
	/* hex_dump(fp, (const unsigned char *)op, 64*KB); */
	fclose(fp);
	}
#endif

	if ( rc ) {
		sprintf(info_msg, "%s[%d]: ioctl failed for %s with %d(%s).out_flag = %#x, retc = %#x\n",
			__FUNCTION__, __LINE__, stats.sdev_id, errno, strerror(errno), th_ct->corsa_buf.out_flags, th_ct->corsa_buf.app.retc);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
		return(-1);
	}


	if ( mode_of_oper ) {
		/* Async Job - Keep polling for out_flag. */
		while ( th_ct->corsa_buf.out_flags != ACCEL_CMD_COMPLETED &&
				th_ct->corsa_buf.out_flags != ACCEL_CMD_TIMEOUT &&
				th_ct->corsa_buf.out_flags != ACCEL_CMD_ABORTED ) {

			/* Grab the mutex and wait on corsa_cond condition variable.
			 * You will be woken up when job completes and process
			 * receives SIGIO. Waking logic in in SIGIO handler.
			 */

			rc = pthread_mutex_lock(&corsa_mutex);
			if ( rc ) {
				sprintf(info_msg, "%s[%d]: pthread_mutex_lock failed with %d(%s).\n", __FUNCTION__, __LINE__, errno, strerror(errno));
				hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
				return(-1);
			}

			rc = pthread_cond_wait(&corsa_cond, &corsa_mutex);
			if ( rc ) {
				sprintf(info_msg, "%s[%d]: pthread_cond_wait failed with %d(%s).\n", __FUNCTION__, __LINE__, errno, strerror(errno));
				hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
				return(-1);
			}

			/* You have been woken up (maybe spurious wake).
			 * You have the lock, release it.
			 */
			pthread_mutex_unlock(&corsa_mutex);

			usleep(5);
		}
	}

	/* printf("%s[%d]: Happy faces. ioctl returned for %s(open=corsa_memcopy) with rc=%d. out_flag = %#x, retc = %#x\n",
		__FUNCTION__, __LINE__, stats.sdev_id, rc, th_ct->corsa_buf.out_flags, th_ct->corsa_buf.app.retc); */
	return(rc);
}
#else /* __HTX_LINUX__ */
int memcopy_buffer(th_context *th_ct, int mode_of_oper)
{
	int rc=0, temp_retries;
	struct asiv_memcpy *asiv;
	struct asv_memcpy *asv;
	char info_msg[1024];

	/* Copy EEH retries to local var. */
	temp_retries = eeh_retries;

	asiv = (struct asiv_memcpy *)&(th_ct->corsa_buf.asiv);
	asv = (struct asv_memcpy *)&(th_ct->corsa_buf.asv);

	/* Zero out entire accel_enq_cmd_t structure before using it */
	memset((void *)&(th_ct->corsa_buf), 0, sizeof(struct genwqe_ddcb_cmd));
	th_ct->corsa_buf.acfunc = 0x01;

	th_ct->corsa_buf.cmd = 0x03;
	th_ct->corsa_buf.cmdopts = 0x0000;
	th_ct->corsa_buf.asiv_length= 0x40 - 0x20;
	th_ct->corsa_buf.asv_length = 0xC0 - 0x80;

	asiv->inp_buff = (uint64_t)th_ct->i_buf->ea; /* start address of input data */
	asiv->inp_buff_len =  (uint32_t)*(th_ct->inlen_ptr); /* Size of input data. */
	asiv->outp_buff = (uint64_t)th_ct->o_buf->ea; /* start address of output data */
	asiv->outp_buff_len = (uint32_t)*(th_ct->outlen_ptr);
	asiv->in_adler32 = (uint32_t)1;
	asiv->in_crc32 = (uint32_t)0;


	th_ct->corsa_buf.ats = (ATS_SET_FLAGS(struct asiv_memcpy, inp_buff, ATS_TYPE_SGL_RD) | ATS_SET_FLAGS(struct asiv_memcpy, outp_buff, ATS_TYPE_SGL_RDWR));

	/* Issue ioctl() to corsa */
	while ( temp_retries > 0 ) {
		rc = ioctl(th_ct->corsa_fd, GENWQE_EXECUTE_DDCB, &(th_ct->corsa_buf));
		if ( rc >= 0 ) {
			break;
		}
		if ( rc && temp_retries ) {
			temp_retries--;
			sleep(EEH_SLEEP);
			continue;
		}
	}

#if 1
	if ( rc < 0 ) {
	sprintf(info_msg, "/tmp/hxecorsa_miscompare.%d.%d", getpid(), th_ct->corsa_fd);
	FILE *fp;
	fp = fopen(info_msg, "w");
	if ( fp == NULL ) {
		sprintf(info_msg, "corsa dump file opening failed with errno=%d\n", errno);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
	}

	sprintf(info_msg, "Output buffers dumped in : %s\n", info_msg);
	hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);

	fprintf(fp, "\n\nAfter ioctl.\n=============\n\ngenwqe_ddcb_cmd\n---------------\n");
	hex_dump(fp, (const unsigned char *)&(th_ct->corsa_buf), sizeof(th_ct->corsa_buf));

	fprintf(fp, "\n Input bytes processed : %d\n", asv->inp_processed);
	fprintf(fp, "\n Output bytes generated : %d\n", asv->outp_returned);

	fprintf(fp, "\nInput buffer\n------------\n");
	hex_dump(fp, (const unsigned char *)(th_ct->i_buf->ea), *(th_ct->inlen_ptr));
	/* hex_dump(fp, (const unsigned char *)ip, 2*KB); */
	fprintf(fp, "\nOutput buffer\n-------------\n");
	hex_dump(fp, (const unsigned char *)(th_ct->o_buf->ea), *(th_ct->inlen_ptr));
	/* hex_dump(fp, (const unsigned char *)op, 64*KB); */
	fprintf(fp, "\nOutput buffer\n-------------\n");
	hex_dump(fp, (const unsigned char *)(th_ct->o_buf->ea), *(th_ct->inlen_ptr));
	/* hex_dump(fp, (const unsigned char *)op, 64*KB); */
	fclose(fp);
	}
#endif

	if ( rc < 0 ) {
		sprintf(info_msg, "%s[%d]: ioctl failed for %s with %d(%s), retc = %#x\n",
			__FUNCTION__, __LINE__, stats.sdev_id, errno, strerror(errno), th_ct->corsa_buf.retc);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
		return(-1);
	}


	/* printf("%s[%d]: Happy faces. ioctl returned for %s(open=corsa_memcopy) with rc=%d. out_flag = %#x, retc = %#x\n",
		__FUNCTION__, __LINE__, stats.sdev_id, rc, th_ct->corsa_buf.out_flags, th_ct->corsa_buf.app.retc); */
	if ( rc >= 0 ) {
		/* sprintf(info_msg, "%s[%d]: ioctl completed successfully for memcopy with rc=%d, errno=%d, retc=%d, attn=%d, input buf=%d, input pb=%d, output pb=%d\n",
			__FUNCTION__, __LINE__, rc, errno, th_ct->corsa_buf.retc, th_ct->corsa_buf.attn, asiv->inp_buff_len, asv->inp_processed, asv->outp_returned);
		hxfmsg(&stats, 0, HTX_HE_INFO, info_msg); */
		return(0);
	}
	else {
		return(rc);
	}
}
#endif /* __HTX_LINUX__ */


int inflate_buffer(th_context *th_ct)
{
	z_stream input_stream;
	int in_buf_len, out_buf_len, rc, temp_retries;
	char info_msg[1024];

	/* EEN retries limit */
	temp_retries = eeh_retries;
	in_buf_len = *(th_ct->inlen_ptr);
	out_buf_len = *(th_ct->outlen_ptr);

	input_stream.zalloc = Z_NULL;
	input_stream.zfree = Z_NULL;
	input_stream.next_in = (unsigned char *)th_ct->i_buf->ea;
	input_stream.avail_in = in_buf_len;
	input_stream.next_out = (unsigned char *)th_ct->o_buf->ea;
	input_stream.avail_out = th_ct->o_buf->buf_size;

	rc = inflateInit(&input_stream);
	if ( rc != Z_OK ) {
		sprintf(info_msg, "inflateInit() for inflate_buffer failed with rc=%d\n", rc);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
	}

	rc=-1;
	while ( input_stream.avail_in != 0  && temp_retries > 0 ) {
		rc = inflate(&input_stream, Z_NO_FLUSH);
		if ( rc == Z_STREAM_END )
			break;

		if ( rc != Z_STREAM_END && rc != Z_OK && temp_retries ) {
			temp_retries--;
			sleep(EEH_SLEEP);
			continue;
		}

	}

	if ( rc != Z_STREAM_END && rc != Z_OK ) {
		sprintf(info_msg, "%s[%d]: inflate didn not finish properly(even after %d retries in case of EEH). rc=%d", __FUNCTION__, __LINE__, eeh_retries, rc);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
		return(-1);
	}

	inflateEnd(&input_stream);
	return(0);
}

int deflate_buffer(th_context *th_ct)
{
	z_stream input_stream;
	int in_buf_len, out_buf_len, rc, temp_retries, prev_errno;
	char info_msg[1024];

	/* Local variable for EEH retries count */
	temp_retries = eeh_retries;

	in_buf_len = *(th_ct->inlen_ptr);
	out_buf_len = *(th_ct->outlen_ptr);

	input_stream.zalloc = Z_NULL;
	input_stream.zfree = Z_NULL;
	input_stream.next_in = (unsigned char *)th_ct->i_buf->ea;
	input_stream.avail_in = in_buf_len;
	input_stream.next_out = (unsigned char *)th_ct->o_buf->ea;
	input_stream.avail_out = th_ct->o_buf->buf_size;

	prev_errno = errno;
	errno = 0;
	rc = deflateInit(&input_stream, Z_BEST_COMPRESSION);
	if ( rc != Z_OK ) {
		sprintf(info_msg, "deflateInit() failed for deflate_buffer with rc = %d\n", rc);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
	}
	if ( errno ) {
		sprintf(info_msg, "deflateInit() sets errno to %d. prev_val = %d\n", errno, prev_errno);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
	}

	prev_errno = errno;
	errno = 0;
	while ( input_stream.avail_in != 0 && temp_retries > 0 ) {
		rc = deflate(&input_stream, Z_NO_FLUSH);
		if ( rc != Z_OK  && temp_retries ) {
			temp_retries--;
			sleep(EEH_SLEEP);
			continue;
		}
	}
	if ( errno ) {
		sprintf(info_msg, "deflate(Z_NO_FLUSH) sets errno to %d. prev_val = %d\n", errno, prev_errno);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
	}

	prev_errno = errno;
	errno = 0;
	temp_retries = eeh_retries;
	while ( temp_retries > 0 ) {
		rc = deflate(&input_stream, Z_FINISH);
		if ( rc == Z_STREAM_END || rc == Z_OK ) {
			break;
		}
		if ( rc != Z_STREAM_END && rc != Z_OK && temp_retries ) {
			temp_retries--;
			sleep(EEH_SLEEP);
			continue;
		}
	}
	if ( errno ) {
		sprintf(info_msg, "deflate(Z_FINISH) sets errno to %d. prev_val = %d\n", errno, prev_errno);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
	}

	if ( rc != Z_STREAM_END && rc != Z_OK ) {
		sprintf(info_msg, "%s[%d]:deflate(Z_FINISH) did not complete properly(even after %d retries for EEH). rc=%d\n", __FUNCTION__, __LINE__, eeh_retries, rc);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
		return(-2);
	}

	*(th_ct->outlen_ptr) = input_stream.total_out;

	deflateEnd(&input_stream);
	return(0);
}

int corsa_execute_algo(ALGOS algo_name, char oper, th_context *th_ct, int partial, int mode_of_oper)
{
	int rc, prev_errno;
	char info_msg[1024];

	/* Clear errno after preserving prev value is any */
	prev_errno = errno;
	errno = 0;

	if ( algo_name == corsa_deflate || algo_name == corsa_deflate_crc ) {
		rc = deflate_buffer(th_ct);
	}
	else if ( algo_name == corsa_inflate || algo_name == corsa_inflate_crc ) {
		rc = inflate_buffer(th_ct);
	}
	else if ( algo_name == corsa_memcopy) {
		rc = memcopy_buffer(th_ct, mode_of_oper);
	}
	else {
		sprintf(info_msg, "Unknown algo_name=%d specified. Please check rulefile settings.\n", algo_name);
		hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
		rc = -1;
	}

	if ( prev_errno || errno ) {
		sprintf(info_msg, "Got either non-0 errno. Prev_errno = %d, errno = %d.\n", prev_errno, errno);
		hxfmsg(&stats, 0, HTX_HE_INFO, info_msg);
	}

	return(rc);
}




int corsa_release_buffer(th_context *th)
{
	int i, temp_retries, rc;
	buf_addr *temp_buf;
	char info_msg[1024];

	for ( i=0; i<MAX_BUFF; i++ ) {
		temp_buf = &(th->all_buf[i]);

		if ( temp_buf->ea_org != NULL ) {
			MALLOC_PRINT("Freeing buffer[%d], addr=%llx\n", i, temp_buf->ea_org);
#if 0
			rc = shmdt(temp_buf->ea_org);
			if ( rc ) {
				sprintf(info_msg,"%s[%d]:shmdt() failed for buffer(%d) with errno=%d(%s)\n", __FUNCTION__, __LINE__, i, errno, strerror(errno));
				hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
				return(errno);
			}

			rc = shmctl(temp_buf->shm_id, IPC_RMID, (struct shmid_ds *) NULL);
			if ( rc ) {
				sprintf(info_msg,"%s[%d]:shmctl(IPC_RMID) failed for buffer(%d) with errno=%d(%s)\n", __FUNCTION__, __LINE__, i, errno, strerror(errno));
				hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
				return(errno);
			}
#else
			free(temp_buf->ea_org);
#endif
		}

#if 0
		if ( temp_buf->buf_size > 4096 ) { /* Indirect DDE */
			MALLOC_PRINT("Freeing iDDE [%d]. addr=%llx\n", i, temp_buf->indirect_dde_buf);
			free(temp_buf->indirect_dde_buf);
		}

		/* CSBCPB/Dictionary buffers */
		temp_buf = &(th->csbcpb_info_buf[i]);
		if ( temp_buf->ea_org != NULL ) {
			MALLOC_PRINT("Freeing CSBCPB/Dictionary buffer[%d]. addr=%llx\n", i, temp_buf->ea_org);
			free(temp_buf->ea_org);
		}
#endif
	}

	if ( strncmp(&(stats.sdev_id[5]), "genwqe", 6) == 0 ) { /* PCIe Corsa */
		temp_retries = eeh_retries;
		while ( temp_retries > 0 ) {
			rc = close (th->corsa_fd);
			if ( rc == 0 )
				break;
	
			temp_retries--;
			sleep(EEH_SLEEP);
		}

		if ( rc ) {
			sprintf(info_msg, "close() of corsa adapter failed with errno=%d(%s).\n", errno, strerror(errno));
			hxfmsg(&stats, 1, HTX_HE_SOFT_ERROR, info_msg);
			return(-1);
		}
	}

	MALLOC_PRINT("Freeing node. addr=%llx\n", th);
	free(th);

	return(0);
}

void hex_dump(FILE *f,const unsigned char *s,int l)
{
	int n=0;

	for( ; n < l ; ++n) {
		if((n%16) == 0)
			fprintf(f,"\n%llx:%04x :",s+n, n);
		fprintf(f," %02x",s[n]);
	}
	fprintf(f,"\n");
}


