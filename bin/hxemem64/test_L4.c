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

#include "hxemem64.h"
#define PAGE_SIZE 			(16 * MB)
#define MCS_MASK			0
#define MBA_MASK			1
#define CC_MASK					2
#define	MAX_MASKS			5
#define CACHE_LINE_SIZE     	128
#define L3_CACHE_SIZE			(8 * MB)
#define L3_ASSOCIATIVITY 		8
#define L4_CACHE_SIZE			(16 * MB)
#define L4_ASSOCIATIVITY    	16
/*#define debug_l4*/

#ifndef __HTX_LINUX__
#pragma reg_killed_by trap
#endif
/*#define PRINT_BITS_DEBUG*/
struct mem_req {
    long *ea;  /* array to store effective addr of each page */
    unsigned long long *real_addr; /* array to store Real addr of each page */
	long ra;
    int num_pages;
    int num_of_contig_pages;
} static mem;

struct mask_info {
	int bit_mask_length;
	int bm_position;
}static volatile masks[MAX_MASKS];
int num_masks = 0 ;

struct L4Roll_thread_context {
	pthread_t thread_id;
	int bit_mask[2];
	int thread_no;
	int bind_cpu;
	int pattern;
	unsigned int thread_offset;
}th_ctx[MAX_THREADS_PER_CHIP];


static int cpus_in_instance[MAX_CPUS_PER_CHIP];
static int instance_number;
static int num_mcs, num_mba_per_mcs,num_gps, num_threads;
htxsyscfg_smt_t smt_details;
extern struct private_data priv;
extern struct rule_format *stanza_ptr,r;
extern struct memory_info mem_info;
extern struct htx_data stats;
static volatile int threads_per_mcs;
static long mem_required=0;
static int mba_threads=0;
static int num_bits_tot_mem=0;

/* Function Declaration */
void test_L4_Rollover(void);
void cleanup_mem(int,char *);
void do_write_and_read_compare_mem(void *);
int get_cpu_deatils();
int get_device_instance(const char *);
void write_mem(char*,int );
void read_and_compare_mem(char *,int);
int get_mem_config_l4();

/* L4 test case is a multi-threaded process that runs per chip,set of threads target a mcs(inturn mba for mba testcase).
* Each thread rolls over a congruence class and move to next CC,
*Compare operation is disabled at the moment*/

void test_L4_Rollover(void)
{
	int i,num_th,rc,total_cpus;
	int L4_shm_id,memflg,th_to_pick;
	static int flag=1;
	char *start_address, *ptr_pattern;
	long p, *temp_ptr;
	unsigned int bytes_exclude_mask;
	/*below variables are to set mem plicy for local memory*/
#ifndef __HTX_LINUX__
	struct shmid_ds shm_buf;
    int vm_early_lru = 1; /* 1 extends local affinity via paging space.*/
    int vm_num_policies = VM_NUM_POLICIES;
    int vm_policies[VM_NUM_POLICIES] = {/* -1 = don't change policy */
    -1, /* VM_POLICY_TEXT */
    -1, /* VM_POLICY_STACK */
    -1, /* VM_POLICY_DATA */
    P_FIRST_TOUCH, /* VM_POLICY_SHM_NAMED */
    P_FIRST_TOUCH, /* VM_POLICY_SHM_ANON */
    -1, /* VM_POLICY_MAPPED_FILE */
    -1, /* VM_POLICY_UNMAPPED_FILE */
    };
#endif

	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"L4 : Starting the L4 Rollover test case\n");
	STATS_VAR_INC(test_id, 1)
	STATS_HTX_UPDATE(UPDATE)
    total_cpus = get_cpu_deatils();
	if(total_cpus == 0){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT," There are no cpus in this chip,Thus exiting \n");
			exit(1);
	}
    if(flag){
        get_smt_details(&smt_details);
        if(smt_details.smt_threads == 0) {
            displaym(HTX_HE_INFO,DBG_MUST_PRINT, "get_smt_details returned smt_threads = 0 ! Will assume single threaded.");
            smt_details.smt_threads = 1;
        }

        rc = get_mem_config_l4();
        if (rc != 0) {
			displaym(HTX_HE_INFO,DBG_MUST_PRINT,"get_mem_config failed with rc = %d\n",rc);
            cleanup_mem(L4_shm_id, start_address);
            exit(1);
        }
		mem_required = ( num_mcs * 16777216 );
        /* Update mask for MCS */
        for(i=0;; i++) {
            if((1 << i) == num_mcs){
                masks[MCS_MASK].bit_mask_length = i;
                break;
            }
        }
        masks[MCS_MASK].bm_position = 7;
        num_masks++;
        num_threads = total_cpus;
		/* Balancing number of threads,so that threads creating per MCS will access proper addresses*/
		if(num_threads % smt_details.smt_threads != 0){
			num_threads = (num_threads - (num_threads % smt_details.smt_threads));
		}
		threads_per_mcs = total_cpus/num_mcs;/*number SW threads targeting a MCS in a group */
        /* If oper is MBA_TEST, update mask for MBA also. */
        if (strcmp (stanza_ptr->oper, "MBA") == 0 ) {
			mba_threads = total_cpus/num_mcs;/*number of threads equally divided to target both MBAs per MCS*/
			/* make sure number of mba threads per mcs is power of 2*/
			masks[MBA_MASK].bit_mask_length=0;
			while(mba_threads >= 2 ) {
				mba_threads = mba_threads/2;
				masks[MBA_MASK].bit_mask_length++;
			}
			/*masks[MBA_MASK].bit_mask_length++;*//*Next number with power of 2 is considered */
			mba_threads = (1 << masks[MBA_MASK].bit_mask_length);	
            masks[MBA_MASK].bm_position= masks[MCS_MASK].bit_mask_length + 15;
            num_masks++;
			mem_required *= num_mba_per_mcs;
			num_threads   = num_mcs * mba_threads;

			masks[CC_MASK].bit_mask_length = 13;
			masks[CC_MASK].bm_position	   = masks[MCS_MASK].bit_mask_length + 7;
        }
		for(i=0;;i++){
			if((1 << i) == mem_required) {
				num_bits_tot_mem = i;
				break;
			}
		}
        flag = 0;
        /*printf("num_gps= %d   num_mcs=%d  num_mba_per_mcs=%d num_masks=%d\n",num_gps,num_mcs,num_mba_per_mcs,num_masks);*/
        displaym(HTX_HE_INFO, DBG_MUST_PRINT, "memory groups=%d  Number of MCS per group = %d   Number of MBA per MCS = %d Memory required=%d num_bits_tot_mem=%d mba_threads=%d num_threads=%d\n", 
			num_gps,num_mcs,num_mba_per_mcs,mem_required,num_bits_tot_mem,mba_threads,num_threads);
    }

    /* bind the process to a cpu and then alloate memory so that we get loclal memory always */
    displaym(HTX_HE_INFO, DBG_MUST_PRINT, "binding to cpu %d for getting mem\n", cpus_in_instance[0]);
    do_the_bind_proc(BIND_TO_PROCESS, cpus_in_instance[0],-1);


#ifndef __HTX_LINUX__

	/* To get local memory for L4 test case set flag early_lru=1 to select P_FIRST_TOUCH policy(similar to setting MEMORY_AFFINITY environment variable to MCM)*/
    rc = vm_mem_policy(VM_SET_POLICY,&vm_early_lru, &vm_policies ,vm_num_policies);
    if (rc != 0){
    	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"vm_mem_policy() call failed with return value = %d\n",rc);
    }

    rc = system("vmo -o enhanced_affinity_vmpool_limit=-1");
    if(rc < 0){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"vmo -o enhanced_affinity_vmpool_limit=-1    failed for  L4 test case"
        "failed errno %d rc = %d\n",errno,rc);
        exit(1);
    }
    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"vmo -o enhanced_affinity_vmpool_limit=-1 rc = %d,errno =%d\n",rc,errno);

	L4_shm_id  = shmget(IPC_PRIVATE,mem_required,IPC_CREAT | 0666);
	if (L4_shm_id == -1) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"test_L4_Rollover: shmget 128MB errored out with errno: %d\n",errno);
	}
	shm_buf.shm_pagesize= 16 * MB;
    if((rc = shmctl(L4_shm_id,SHM_PAGESIZE, &shm_buf)) == -1) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"shmctl failed with %d while setting page size.\n",errno);
    }
#else
	memflg = (IPC_CREAT | IPC_EXCL |SHM_R | SHM_W | SHM_HUGETLB);
	L4_shm_id  = shmget(IPC_PRIVATE,mem_required,memflg);
	if (L4_shm_id == -1) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"test_L4_Rollover: shmget %d errored out with errno: %d\n",mem_required,errno);
	}

#endif
    /* unbind the process */
    do_the_bind_proc(BIND_TO_PROCESS, UNBIND_ENTITY,-1);

	start_address = (char  *) shmat (L4_shm_id,(int *)0, 0);
    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"L4 test case:seg start address:%lx\n",start_address);
	if ((int  *)start_address == (int *)-1) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"test_L4_Rollover: shmat errored out with errno : %d \n",errno);
    	}
	else {
		displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"test_L4_Rollover: Attached shared memory segment at virtual address 0x%x\n",start_address );
	}
#if 0
	rc = mlock(start_address, mem_required);
    	if(rc == -1) {
        	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"\ntest_L4_Rollover: mlock failed with errno: %d.\n", errno);
        	cleanup_mem(L4_shm_id, start_address);
    		exit(1);
	}
#endif

	mem.num_pages = ((int)mem_required)/PAGE_SIZE;
	/* Allocate memory to store EA of each page  */
	mem.ea = (long  *)malloc (sizeof(long) * (mem_required/PAGE_SIZE));
	if(mem.ea == NULL){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
                 "test_L4_Rollover:(malloc failed) "
                 "allocation to mem.ea failed! errno = %d(%s)\n", errno, strerror(errno));
			cleanup_mem(L4_shm_id, start_address);
			exit(1);
	}
	temp_ptr= mem.ea;

	/*printf("\n size of ea=%d, loop limit=%d\n", sizeof(mem.ea), start_address+mem_required);*/

	/* Allocate memory to store RA of each page  - disabling it for now as it is not required */
#if 0
	i=0;
	mem.real_addr = (unsigned long long *) malloc (sizeof(unsigned long long) * mem_required/PAGE_SIZE);
	if(mem.real_addr == NULL){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
                 "test_L4_Rollover:(malloc failed) "
                 "allocation to mem.real_addr failed! errno = %d(%s)\n", errno, strerror(errno));
	}
#endif
   	for(p=(long)start_address; p < ((long)start_address+mem_required); p += PAGE_SIZE, temp_ptr++) {
       	*(temp_ptr) = (long)p;
		/*printf("\n address=%lx\n", p);*/
		displaym(HTX_HE_INFO,DBG_IMP_PRINT,"page start address:%lx\n",p);
	}
#if 0
	#ifndef __HTX_LINUX__
       	getRealAddress(p, 0, &(mem.real_addr[i]));
	#else
		/*printf("user eff address:%lx     real addr:%llx\n",mem.ea[i], mem.real_addr[i]);*/
		get_real_address(p, &mem.real_addr[i]);
		/*printf("user eff address:%lx     real addr:%llx\n",mem.ea[i], mem.real_addr[i]);*/
	#endif
       	i++;
#endif

	stanza_ptr->num_threads = num_threads;
	mem_info.num_of_threads = stanza_ptr->num_threads;
    /* Allocate 'mem_info.num_of_threads' number of "struct thread_data " this is required as memory for mem_info.tdata_hp ptr was allocated later stage*/
    alocate_mem_for_mem_info_num_of_threads();
    if ((unsigned long*)mem_info.tdata_hp== NULL) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
                 "test_L4_Rollover:(malloc failed) Creation of thread data "
                 "structures failed! errno = %d(%s)\n", errno, strerror(errno));
            cleanup_mem(L4_shm_id, start_address);
			exit(1) ;
    }
	bytes_exclude_mask = (1 << (masks[MCS_MASK].bit_mask_length + masks[MCS_MASK].bm_position)); /* we fix bits that represents cache line plus mask (i.e 7+log(mcs))*/ 
	/*if(mkdir("/tmp/L41", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)){
		printf("mkdir failed with error%d\n",errno);
		if(errno != 17)
			exit(1);
	} */
	
	displaym(HTX_HE_INFO, DBG_IMP_PRINT, "num_threads=%d threads_per_mcs = %d bytes_exclude_mask=%d\n",num_threads,threads_per_mcs,bytes_exclude_mask);
	/*setting up thread context*/
	for(num_th = 0; num_th < num_threads  ; num_th++) {
		th_ctx[num_th].thread_no = num_th;
		th_ctx[num_th].bind_cpu = cpus_in_instance[num_th % total_cpus]; /* making use of  all cpus in this chip*/
		th_ctx[num_th].bit_mask[MCS_MASK] = num_th/threads_per_mcs;
		th_ctx[num_th].thread_offset = (num_th % threads_per_mcs) * bytes_exclude_mask;
		if(stanza_ptr->mcs_mask != -1){
			th_ctx[num_th].bit_mask[MCS_MASK] = stanza_ptr->mcs_mask;
		}
		if (strcmp (stanza_ptr->oper, "MBA") == 0 ) {
			th_ctx[num_th].bit_mask[MCS_MASK] = num_th/mba_threads;
			if(stanza_ptr->mcs_mask != -1){
				th_ctx[num_th].bit_mask[MCS_MASK] = stanza_ptr->mcs_mask;
			}
			th_ctx[num_th].bit_mask[MBA_MASK] = (num_th % mba_threads);
		}
		/*printf(" masks: %x and %x for %d\n",th_ctx[num_th].bit_mask[0],th_ctx[num_th].bit_mask[1],num_th);*/
        ptr_pattern = (char *)&th_ctx[num_th].pattern;
		*ptr_pattern++ = 0xa;
        for (i=0; i<(sizeof(th_ctx[num_th].pattern)-1); i++) {
            *ptr_pattern++ = num_th;
        }

	}
	displaym(HTX_HE_INFO,DBG_IMP_PRINT,"L4 Test case:Number of threads to create=%d\n",num_threads);
	for( num_th = 0 ; num_th < num_threads; num_th++) {
      	if (pthread_create(&th_ctx[num_th].thread_id,NULL,(void *(*)(void *))do_write_and_read_compare_mem, (void *)&th_ctx[num_th])) {
     		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"test_L4_Rollover: Error in creating thread no %d, returned = %d\n",num_th,errno);
       		cleanup_mem(L4_shm_id, start_address);
			exit(1);
		}
		mem_info.tdata_hp[num_th].tid = th_ctx[num_th].thread_id;
		mem_info.tdata_hp[num_th].thread_num = th_ctx[num_th].thread_no;
    }
	for ( num_th = 0 ; num_th < num_threads; num_th++) {
       	int thread_retval;
       	void *th_join_result = (void *)&thread_retval;

       	rc = pthread_join(th_ctx[num_th].thread_id, &th_join_result);

       	if ( rc != 0 ) {
      		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"test_L4_Rollover: Error in Joining thread no %d, returned = %d\n",num_th,rc);
       		cleanup_mem(L4_shm_id, start_address);
			exit(1);
		}
	 	mem_info.tdata_hp[num_th].tid = -1;
		mem_info.tdata_hp[num_th].thread_num = -1;
   	}

	cleanup_mem(L4_shm_id, start_address);
	if (mem.ea != NULL) {
		free(mem.ea);
		mem.ea = NULL;
	}
/* 	free(mem.real_addr); */

}

void do_write_and_read_compare_mem(void *th_ctx)
{
	int num_oper,i,j,nr,cc_offset,mask_iter, test;
	char buff[50], *local_ptr=NULL,*next_cc_address=NULL;
	unsigned int *address=NULL;
	struct L4Roll_thread_context *th = (struct L4Roll_thread_context*)th_ctx;
	unsigned char bit_mask=0;
	unsigned long long temp=0, temp_mask=0,cc_temp,k;
	long *temp_ptr=NULL;
	long thread_start_addr;
    int buf;
    char msg_text[4096];
	unsigned int num_of_sets, inner_loop_limit,outer_loop_limit;
	FILE* fp;
	int rc;
	char fname[1024];
	sprintf(fname,"/tmp/L41/addresses_%d",th->thread_no);
	#ifdef debug_l4
	fp = fopen(fname,"w");
	if(fp == NULL){
		printf("fopen failed with errno:%d\n",errno);
		exit(1);
	}
	#endif
	num_of_sets      = (L3_CACHE_SIZE) / (L3_ASSOCIATIVITY * CACHE_LINE_SIZE);
	inner_loop_limit = L3_ASSOCIATIVITY * 2;
	outer_loop_limit = (num_of_sets / num_threads);
	do_the_bind_proc(BIND_TO_THREAD,th->bind_cpu,-1);
	if (strcmp (stanza_ptr->oper, "MBA") == 0 ) {
		cc_offset = (1 << (masks[CC_MASK].bit_mask_length + masks[CC_MASK].bm_position));
		outer_loop_limit = 1 << ( masks[CC_MASK].bit_mask_length - masks[MBA_MASK].bit_mask_length);
		inner_loop_limit = 1 << (num_bits_tot_mem - (masks[CC_MASK].bit_mask_length + masks[CC_MASK].bm_position));
		 for(num_oper=0; num_oper < stanza_ptr->num_oper; num_oper++) {
			test = 0;
			temp_ptr = mem.ea;
				address = (unsigned int*)(*temp_ptr);
				local_ptr = (char *)address;
					for(k=0;k<outer_loop_limit;k++){ /*outer loop to generate addresses for CC(13 bits) for a fixed mcs and mba masked cacheline*/
						temp =(unsigned long)local_ptr;
						cc_temp = k;
						cc_temp = (cc_temp >> 8);
						cc_temp = (cc_temp << (masks[MBA_MASK].bit_mask_length + 8));
						cc_temp = (k & 0xff) | cc_temp;
						temp_mask = (1 << (masks[CC_MASK].bit_mask_length)) - 1;
						temp_mask = temp_mask << masks[CC_MASK].bm_position;
						temp  = (temp & (~temp_mask)) | (cc_temp << masks[CC_MASK].bm_position);
						for(mask_iter=0;mask_iter < num_masks;mask_iter++) {
							bit_mask = th->bit_mask[mask_iter];
							/*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"write:bit_mask=%x:mask_iter=%d\n",bit_mask,mask_iter);*/
							temp_mask = (1 << (masks[mask_iter].bit_mask_length)) - 1;
							temp_mask = temp_mask << masks[mask_iter].bm_position;
							/*printf("temp_mask=%llx   bit_mask=%lx  temp=%llx th->thread_no=%d\n",temp_mask,bit_mask,temp,th->thread_no);*/
							temp  = (temp & (~temp_mask)) | (bit_mask<< masks[mask_iter].bm_position);
						}
						address = (unsigned int*)temp;
						for(j=0;j<inner_loop_limit;j++){/*to reach all CCs belongs to this thread*/
							#ifdef debug_l4
							fprintf(fp,"th:%d,mask=%x,offst=%x writing at adrress:%lx\n",th->thread_no,cc_temp,th->thread_offset,address);
							fflush(fp);
							#endif
							*address = (unsigned int ) th->pattern;
							address += (cc_offset/4);/* Jump of 4MB to hit same CC*/
						}
						#ifdef debug_l4
						fprintf(fp,"--------------------------------------------------------------old cc addr=%llx,new cc_addr=%llx cc=%d\n",next_cc_address,(next_cc_address+(CACHE_LINE_SIZE * num_mcs * threads_per_mcs)),j);
						fflush(fp);
						#endif
				}	
				if (priv.exit_flag == 1) {
					pthread_exit((void *)0);
				} /* endif */
				#if 1
				for(nr=0;nr<stanza_ptr->num_reads;nr++) {
					temp_ptr = mem.ea;
						address = (unsigned int*)(*temp_ptr);
						local_ptr = (char *)address;
							for(k=0;k<outer_loop_limit;k++){
								temp =(unsigned long)local_ptr;
								cc_temp = k;
								cc_temp = (cc_temp >> 8);
								cc_temp = (cc_temp << (masks[MBA_MASK].bit_mask_length + 8));
								cc_temp = (k & 0xff) | cc_temp;
								temp_mask = (1 << (masks[CC_MASK].bit_mask_length)) - 1;
								temp_mask = temp_mask << masks[CC_MASK].bm_position;
								temp  = (temp & (~temp_mask)) | (cc_temp << masks[CC_MASK].bm_position);
								for(mask_iter=0;mask_iter < num_masks;mask_iter++) {
									bit_mask = th->bit_mask[mask_iter];
									/*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"write:bit_mask=%x:mask_iter=%d\n",bit_mask,mask_iter);*/
									temp_mask = (1 << (masks[mask_iter].bit_mask_length)) - 1;
									temp_mask = temp_mask << masks[mask_iter].bm_position;
									/*printf("temp_mask=%llx   bit_mask=%lx  temp=%llx th->thread_no=%d\n",temp_mask,bit_mask,temp,th->thread_no);*/
									temp  = (temp & (~temp_mask)) | (bit_mask<< masks[mask_iter].bm_position);
								}
								address = (unsigned int*)temp;
								for(j=0;j<inner_loop_limit;j++){/*to reach all CCs belongs to this thread*/
									/* printf("in read.......temp_mask=%llx addr=%llx, bit_mask=%lx  temp=%llx th->thread_no=%d, num_oper=%d\n",temp_mask,address, bit_mask,temp,th->thread_no, i); */
									buf = *(int*)address;
									address += (cc_offset/4);
								}
								/*Compare operation is disable as it may affect BW throughput*/
								#if 0
								if (buf != th->pattern) {
									/* trap to kdb on miscompare
									*R3=0xBEEFDEADFFFFFFFF
									*R4=Starting Address of the Shared Memory Segment
									*R5=Address of Miscomparing Location
									*R6=Expected pattern
									*R7=Size in bytes of Shared Memory Segment
									*R8=thread context address*/
									if((stanza_ptr->misc_crash_flag) && (priv.htxkdblevel)){
										displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "test_L4_Rollover:Miscompare Detected\n");
										 #ifdef __HTX_LINUX__
											do_trap_htx64 ((unsigned long)0xBEEFDEADFFFFFFFF,\
															(unsigned long)mem.ea[0],\
															(unsigned long)address,\
															(unsigned long)th->pattern,\
															(unsigned long)134217728,\
															(unsigned long)th);
										#else
											trap(0xBEEFDEADFFFFFFFF,mem.ea[0],address,th->pattern,134217728,th);
										#endif
									}
									else {
										sprintf(msg_text,"MEMORY MISCOMPARE(hxemem64) in rule %s,Rules file=%s\n"
												"Expected pattern=%x,Actual value=%x at Effective address = %llx\n"
												"Thread number = %d with mask = %x, CPU number bound with = %d\n"
												"temp_mask=%llx bit_mask=%lx temp=%llx num_oper=%d  num_masks=%d page index=%d \n"
												"masks[MCS_MASK].bit_mask_length=%d,masks[MCS_MASK].bm_position=%d \n",\
												stanza_ptr->rule_id,priv.rules_file_name,\
												th->pattern,buf,address,\
												th->thread_no,th->bit_mask[MCS_MASK],th->bind_cpu,\
												temp_mask,bit_mask,temp,num_oper,num_masks,i,masks[MCS_MASK].bit_mask_length,masks[MCS_MASK].bm_position);
										displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "test_L4_Rollover:Miscompare Detected\n%s", msg_text);
										pthread_exit((void *)0);
									}
								}
								#endif
								#ifdef PRINT_BITS_DEBUG
								printf("Thread[%d]", th->thread_no);
								sprintf(buff,"%x",address);
								hexa(buff);
								printf("\n");
								#endif
							}
			}/*loop for num reades ends */
			if (priv.exit_flag == 1) {
				pthread_exit((void *)0);
			} /* endif */
		#endif
		}/*for loop for num opers ends here*/
	}
	else {
	
		 for(num_oper=0; num_oper < stanza_ptr->num_oper; num_oper++) {
			test = 0;
			temp_ptr = mem.ea;
			 for(i=0;i<mem.num_pages;i++) {
				address = (unsigned int*)(*temp_ptr);
				local_ptr = (char *)address;
				local_ptr += th->thread_offset;
				next_cc_address = local_ptr;
					for(j=0;j<outer_loop_limit;j++){/*to reach all CCs belongs to this thread*/
							local_ptr = next_cc_address;
							for(k=0;k<inner_loop_limit;k++){ /*inner loop to roll out L3 CC*/
								temp =(unsigned long)local_ptr;
								for(mask_iter=0;mask_iter < num_masks;mask_iter++) {
									bit_mask = th->bit_mask[mask_iter];
									/*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"write:bit_mask=%x:mask_iter=%d\n",bit_mask,mask_iter);*/
									temp_mask = (1 << (masks[mask_iter].bit_mask_length)) - 1;
									temp_mask = temp_mask << masks[mask_iter].bm_position;
									/*printf("temp_mask=%llx   bit_mask=%lx  temp=%llx th->thread_no=%d\n",temp_mask,bit_mask,temp,th->thread_no);*/
									temp  = (temp & (~temp_mask)) | (bit_mask<< masks[mask_iter].bm_position);
								}
								address = (unsigned int*)temp;
								#ifdef debug_l4
								fprintf(fp,"th:%d,mask=%d,offst=%x writing at adrress:%lx\n",th->thread_no,bit_mask,th->thread_offset,address);
								fflush(fp);
								#endif
								*address = (unsigned int ) th->pattern;
								#ifdef PRINT_BITS_DEBUG
								printf("Thread[%d]", th->thread_no);
								sprintf(buff,"%x",address);
								hexa(buff);
								printf("\n");
								#endif
								local_ptr += 1*MB;
							}
							#ifdef debug_l4
							fprintf(fp,"-------------------------------------------------------------------------------------cc=%d\n",j);
							fflush(fp);
							#endif
							next_cc_address += (CACHE_LINE_SIZE * num_mcs * threads_per_mcs);
					}
				temp_ptr++;
			}
			if (priv.exit_flag == 1) {
				pthread_exit((void *)0);
			} /* endif */
		#if 1
			for(nr=0;nr<stanza_ptr->num_reads;nr++) {
					temp_ptr = mem.ea;
					 for(i=0;i<mem.num_pages;i++) {
						address = (unsigned int*)(*temp_ptr);
						local_ptr = (char *)address;
						local_ptr += th->thread_offset;
						next_cc_address = local_ptr;
						for(j=0;j<outer_loop_limit;j++){
							local_ptr = next_cc_address;
							for(k=0;k<inner_loop_limit;k++){
								temp =(unsigned long)local_ptr;
								for(mask_iter=0;mask_iter < num_masks;mask_iter++) {
									bit_mask = th->bit_mask[mask_iter];
									/*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"read:bit_mask=%x:mask_iter=%d\n",bit_mask,mask_iter);*/
									temp_mask = (1 << (masks[mask_iter].bit_mask_length)) - 1;
									temp_mask = temp_mask << masks[mask_iter].bm_position;
									temp  = (temp & (~temp_mask)) | (bit_mask<< masks[mask_iter].bm_position);
								}
								address = (unsigned int*)temp;
								/* printf("in read.......temp_mask=%llx addr=%llx, bit_mask=%lx  temp=%llx th->thread_no=%d, num_oper=%d\n",temp_mask,address, bit_mask,temp,th->thread_no, i); */
								buf = *(int*)address;
								#if 0
								if(!strcmp(stanza_ptr->compare,"YES")) {
										if (buf != th->pattern) {
											/* trap to kdb on miscompare
											*R3=0xBEEFDEADFFFFFFFF
											*R4=Starting Address of the Shared Memory Segment
											*R5=Address of Miscomparing Location
											*R6=Expected pattern
											*R7=Size in bytes of Shared Memory Segment
											*R8=thread context address*/
											if((stanza_ptr->misc_crash_flag) && (priv.htxkdblevel)){
												displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "test_L4_Rollover:Miscompare Detected\n");
												 #ifdef __HTX_LINUX__
													do_trap_htx64 ((unsigned long)0xBEEFDEADFFFFFFFF,\
																	(unsigned long)mem.ea[0],\
																	(unsigned long)address,\
																	(unsigned long)th->pattern,\
																	(unsigned long)134217728,\
																	(unsigned long)th);
												#else
													trap(0xBEEFDEADFFFFFFFF,mem.ea[0],address,th->pattern,134217728,th);
												#endif
											}
											else {
												sprintf(msg_text,"MEMORY MISCOMPARE(hxemem64) in rule %s,Rules file=%s\n"
														"Expected pattern=%x,Actual value=%x at Effective address = %llx\n"
														"Thread number = %d with mask = %x, CPU number bound with = %d\n"
														"temp_mask=%llx bit_mask=%lx temp=%llx num_oper=%d  num_masks=%d page index=%d \n"
														"masks[MCS_MASK].bit_mask_length=%d,masks[MCS_MASK].bm_position=%d \n",\
														stanza_ptr->rule_id,priv.rules_file_name,\
														th->pattern,buf,address,\
														th->thread_no,th->bit_mask[MCS_MASK],th->bind_cpu,\
														temp_mask,bit_mask,temp,num_oper,num_masks,i,masks[MCS_MASK].bit_mask_length,masks[MCS_MASK].bm_position);
												displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "test_L4_Rollover:Miscompare Detected\n%s", msg_text);
												pthread_exit((void *)0);
											}
										}
								}
								#endif
								#ifdef PRINT_BITS_DEBUG
								printf("Thread[%d]", th->thread_no);
								sprintf(buff,"%x",address);
								hexa(buff);
								printf("\n");
								#endif
								local_ptr += 1*MB;
							}
							next_cc_address += (CACHE_LINE_SIZE * num_mcs * threads_per_mcs);
						}
						temp_ptr++;
					}
			}
			if (priv.exit_flag == 1) {
				pthread_exit((void *)0);
			} /* endif */
		#endif
		}/*for loop for num of reads ends here*/


	}
	do_the_bind_proc(BIND_TO_THREAD,UNBIND_ENTITY,-1);
	#ifdef debug_l4
	fclose(fp);
	#endif
}

void cleanup_mem(int shm_id,char *addr)
{
	int rc;
    if(addr != NULL) {
        if (shmdt(addr) == -1) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"shmdt errored out with errno:%d\n", errno);
            addr = NULL;
        }
        else if(shm_id != -1) {
            if (shmctl(shm_id, IPC_RMID, 0) == -1) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"shmctl failed for IPC_RMID with errno: %d\n", errno);
            }
            shm_id = -1;
        }
    }
    #ifndef __HTX_LINUX__
    /* set the enhanced_affinity_vmpool_limit value to default  */
    rc = system("vmo -d enhanced_affinity_vmpool_limit");
    if(rc < 0){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"vmo -d enhanced_affinity_vmpool_limit   failed"
        "failed errno %d rc = %d\n",errno,rc);
        exit(1);
    }
    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"cleanup_mem in test L4:vmo -d enhanced_affinity_vmpool_limit rc=%d \t errno =%d\n",\
    rc,errno);
    #endif
}




#ifdef PRINT_BITS_DEBUG
void hexa(char * hexval)
{
        int i=0;
        while(hexval[i] != '\0'){
                printf(" ");
        switch(hexval[i])
         {
                case '0':
                         printf("0000");
                        break;
                 case '1':
                        printf("0001");
                        break;
                 case '2':
                        printf("0010");
                         break;
                case '3':
                        printf("0011");
                         break;
                 case '4':
                        printf("0100");
                         break;
                case '5':
                         printf("0101");
                        break;
                 case '6':
                        printf("0110");
                        break;
                case '7':
                        printf("0111");
                        break;
                case '8':
                        printf("1000");
                         break;
                case '9':
                        printf("1001");
                        break;
		case 'a':
                case 'A':
                         printf("1010");
                        break;
                case 'b':
                case 'B':
                         printf("1011");
                        break;
                case 'c':
                case 'C':
                        printf("1100");
                        break;
                 case 'd':
                 case 'D':
                        printf("1101");
                         break;
                case 'e':
                case 'E':
                         printf("1110");
                         break;
                 case 'f':
                case 'F':
                        printf("1111");
                         break;
                default:
                        printf("\n");
                         break;
        }
        i++;
}}
#endif

int get_cpu_deatils(){
	int rc,total_cpus = 0;
    rc = repopulate_syscfg(&stats);
    if( rc != 0) {
		displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,"test_L4_Rollover:Repopulation if syscfg unsuccessful. Exiting\n");
	}
    instance_number = priv.bind_proc;
    total_cpus  = get_cpus_in_chip(instance_number,cpus_in_instance);
	return total_cpus;
    /*printf("total cpus:%d\n",total_cpus);*/
}

/* collects the centaur grouping info i.e mcs,mba. 
*	currently for BML and AIX values are hardcoded*/
int get_mem_config_l4()
{
	FILE *fp, *fp1;
	char str[128], *strptr, mem_node_str[64], mapped_node[64];
	char buf[64], *ptsr=buf, **ptr=&ptsr;
	int rc=0, node_found = 0, chip_mem, mapped_node_num ;
	unsigned long long mem_node;
	int node_num, node_num_1, mcs_interleave, mcs_interleave_1;
	int mba_interleave, mba_interleave_1;
    unsigned int pvr;
 #ifdef __HTX_LINUX__
    pvr = (unsigned int) get_true_cpu_version();
 #else
    pvr = (unsigned int) getTruePvr();
 #endif
    pvr = pvr >> 16;
	/* instance_no = priv.bind_proc; */
#if 0
	/* map memory node for this instance */
	sprintf(str, "ls /sys/devices/system/node | grep node | head -%d | tail -1", (instance_number + 1));
	fp = popen(str, "r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				"popen failed with errno: %d while mapping memory node to device", errno);
		return -1;
	}
	rc = fscanf(fp, "%s", mapped_node);
	if (rc == 0 || rc == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				"fscanf failed with errno. %d while mapping memory node to device", errno);
		return -1;
	}
	displaym(HTX_HE_INFO, DBG_MUST_PRINT,
			"Device mem%d is mapped to memory %s\n", instance_number, mapped_node);

	/* get the mapped node no. */
	mapped_node_num = atoi(mapped_node + 4);
	displaym(HTX_HE_INFO,DBG_DEBUG_PRINT, "mapped node num: %d\n", mapped_node_num);

	/* Check if memory is available on current chip. If no available memory, give WARNING and assign default values for mem config
	parameters and return. */
	sprintf (str, "cat /sys/devices/system/node/%s/meminfo | grep MemTotal | awk '{print $4}'", mapped_node);
	fp = popen(str, "r");
	if (fp == NULL) {
 		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				"popen failed with errno: %d while getting memory for %s.\n", errno, mapped_node);
				/*printf("popen failed with errno: %d while getting memory for %s.\n", errno, mapped_node);*/
		return -1;
	}
	rc = fscanf(fp, "%d", &chip_mem);
	if (rc == 0 || rc == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				"fscanf failed with errno. %d while getting memory on %s.\n", errno, mapped_node);
/*				printf("fscanf failed with errno. %d while getting memory on %s.\n", errno, mapped_node);*/
		return -1;
	}
	pclose(fp);
	if (chip_mem == 0) {
		displaym(HTX_HE_INFO, DBG_MUST_PRINT,
				"WARNING!! There is no memory on %s, Testcase results will not be as expected.\n", mapped_node);
			/*	printf("WARNING!! There is no memory on chip %d, Testcase results will not be as expected.\n", instance_number);*/
	/* Assign default values for the below variables */
		num_gps = 1;
        if(pvr == 0x4b || pvr == 0x4c)
            num_mcs = 4;
        else if (pvr == 0x4d)
            num_mcs = 8;

		num_mba_per_mcs = 2;
		return 0;
	}
/* look for the corresponding memory node entry in device-tree */
	sprintf(str, "ls /proc/device-tree | grep memory");
	fp = popen(str, "r");
	if (fp == NULL) {
		 displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
					"popen failed while getting memory mapping for instance %d with errno %d", instance_number, errno);
					/*printf("popen failed while getting memory mapping for instance %d with errno %d", instance_number, errno);*/
		return -1;
	}

	while(1) {
		rc = fgets(mem_node_str, 64, fp);
		if (rc == NULL && feof(fp)) {
			if (node_found == 0) {
				 displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
						"could not find memory node for instance %d in device-tree", instance_number);
				/*printf("could not find t_L4_mem_details.cemory node for instance %d in device-tree", instance_number);*/
				pclose(fp);
				return -1;
			} else {
				break;
			}
		}

		strptr = strtok_r(mem_node_str, "\n", ptr);
		strcpy(mem_node_str, strptr);
		sprintf(str, "od -x /proc/device-tree/%s/ibm,associativity | head -1 | cut -d \" \" -f 6,7", mem_node_str);
		displaym(HTX_HE_INFO, DBG_DEBUG_PRINT, "str: %s\n", str);
		fp1 = popen(str, "r");
		if (fp1 == NULL) {
			 displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
						"popen failed while getting memory mapping for instance %d with errno %d", instance_number, errno);
			/*printf("popen failed while getting memory mapping for instance %d with errno %d", instance_number, errno);*/
			return -1;
		}
		fscanf(fp1, "%x %x", &node_num, & node_num_1);
		node_num = (node_num << 16) | node_num_1;
		if (node_num == mapped_node_num) {
			 displaym(HTX_HE_INFO, DBG_IMP_PRINT,
					"corresponding node in device-tree is: %s\n", str);
			node_found = 1;
			num_gps++;
		}
		pclose(fp1);
	}

	if (num_gps > 1) {
      displaym(HTX_HE_INFO, DBG_MUST_PRINT,
				"No. of memory groups are more than on chip %d. Testcase results will not be as expected.", instance_number);
		/*printf("No. of memory groups are more than on chip %d. Testcase results will not be as expected.", instance_number);*/
	}

/* Get the mem config parameters from the device-tree */
	sprintf(str, "od -x /proc/device-tree/%s/ibm,interleave-mask | head -1 | cut -d \" \" -f 2-5", mem_node_str);
	fp = popen(str, "r");
	if (fp == NULL) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				"popen failed while getting ibm,mc value from device-tree with errno: %d", errno);
				/*printf("popen failed while getting ibm,mc value from device-tree with errno: %d", errno);*/
		return -1;
	}
	rc = fscanf(fp, "%x %x %x %x", &mcs_interleave, &mcs_interleave_1, &mba_interleave, &mba_interleave_1);
	if (rc == 0 || rc == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
				"Did not get correct values from device-tree for memory config");
				/*printf("Did not get correct values from device-tree for memory config.\n");*/
		return -1;
	}
	pclose(fp);

	num_mcs = (((mcs_interleave << 16) | mcs_interleave_1) / CACHE_LINE_SIZE ) + 1;
	if (((mba_interleave << 16) | mba_interleave_1) > 0 ) {
		num_mba_per_mcs = 2;
	} else {
		num_mba_per_mcs = 1;
	}

    /* The below part of code is temporary as we are seeing some problems with interleave-mask, once that is
    fixed, remove below code */
        if(pvr == 0x4b || pvr == 0x4c)
            num_mcs = 4;
        else if (pvr == 0x4d)
            num_mcs = 8;
		else {
			displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Unknown PVR=%X, setting default num_mcs = 4\n",pvr);
			num_mcs = 4;
		}
        num_mba_per_mcs = 2;
        num_gps= 1;

#endif
   	if(pvr == 0x4b || pvr == 0x4c)
    	  num_mcs = 4;
   	else if (pvr == 0x4d)
     	 num_mcs = 8;
	else {
		displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Unknown PVR=%X, setting default num_mcs = 4\n",pvr);
		num_mcs = 4;
	}
    num_mba_per_mcs = 2;
    num_gps= 1;
	return 0;
}

