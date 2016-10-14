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

#include "hxemem64.h"
extern struct rule_format *stanza_ptr,r;
extern struct memory_info mem_info;
extern struct private_data priv;
extern struct htx_data stats;
extern volatile int logical_cpus[];
extern volatile int sig_flag;
extern char  page_size[MAX_NUM_PAGE_SIZES][4];
#ifdef __HTX_LINUX__
   int do_trap_htx64 (unsigned long arg1,
                      unsigned long arg2,
                      unsigned long arg3,
                      unsigned long arg4,
                      unsigned long arg5,
                      unsigned long arg6,
					  unsigned long arg7,
					  unsigned long arg8);
#else
#pragma mc_func trap1 { "7c810808" }
#pragma reg_killed_by trap1
#endif
int gen_tlbie (void);
void do_TLB_write_read_compare_mem(void* th);
void do_TLB_write_read_compare_mem_new(void* th);
int get_system_details();
int num_cores;
volatile int update_sys_detail_flag = 1;
int core_array[256];
struct system_details {
	int total_cpus;
	int num_cores;
	int cpus_in_core_array[256][100];
	int smt_per_core_array[256];
}sys_details; 	
/*
 * Function Name: gen_tlbie
 *
 * Description:
 *    Created for the Field feature 541203 which requires the hardware generates
 *    tlbies regularly. Only 1 shared memory region is allocated (16M intially) and so we
 *    4k  4k pages are generated and we touch each region and then detach the shared
 *    memory and repeat the process to generate more tlbies on that memory region with
 *    our exerciser process.
 * Input:
 * Output:
 * Notes:
 */


int gen_tlbie()
{

	unsigned int j1=12,*seed1;
	int num_th,rc,page_index;
#if 0
#ifdef __HTX_LINUX__
	/* Register handler for SIGUSR2 for cpu hotplug add/remove */
	sigemptyset(&(sigvector.sa_mask));      /* do not block signals */
	sigvector.sa_flags = 0; /* do not restart system calls on sigs */
	sigvector.sa_handler = (void (*)(int)) tlbie_SIGUSR2_hdl;
	sigaction(SIGUSR2, &sigvector, (struct sigaction *) NULL);
#endif
#endif


    /* Update system_detail data structure */
    if (update_sys_detail_flag == 1 ) {
		rc = get_system_details();
    	if( rc <  0) {
    	    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d] get_system_details failed. Exiting\n",__LINE__);
    	    exit(1);
    	}
		 displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d] Collected all systems details,get_system_details() successful\n",__LINE__);
		update_sys_detail_flag = 0;
	}
	 seed1 = &j1;

	if(stanza_ptr->percent_hw_threads > 100){
		stanza_ptr->percent_hw_threads = 100;
	}
	int num_threads = sys_details.num_cores * ((float)stanza_ptr->percent_hw_threads/100);
	if (num_threads < 1){
		 num_threads = 1;
	}	
	stanza_ptr->num_threads = num_threads;
    mem_info.num_of_threads = stanza_ptr->num_threads;
    /* Allocate 'mem_info.num_of_threads' number of "struct thread_data " */
    alocate_mem_for_mem_info_num_of_threads();
    if ((unsigned long*)mem_info.tdata_hp== NULL) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,
                 "(malloc failed) Creation of thread data "
                 "structures failed! errno = %d(%s)\n", errno, strerror(errno));
       return(-1) ;
    }
	/*printf("threads = %d\n",num_threads);*/
    STATS_VAR_INC(test_id, 1)
   /* stats.test_id++;*/
    STATS_HTX_UPDATE(UPDATE)

    if (stanza_ptr->seg_size[PAGE_INDEX_4K] == 0 ) {
        stanza_ptr->seg_size[PAGE_INDEX_4K] = DEF_SEG_SZ_4K;
    }
	if(tlbie_test_flag){
	    for (num_th = 0 ; num_th < num_threads ; num_th++) {
        	mem_info.tdata_hp[num_th].thread_num = num_th;
        	mem_info.tdata_hp[num_th].tlbie_shm_id = shmget(IPC_PRIVATE,stanza_ptr->seg_size[PAGE_INDEX_4K] ,IPC_CREAT | 0666);
        	if (mem_info.tdata_hp[num_th].tlbie_shm_id == -1) {
            	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: shmget (4k*4k) errored out with errno : %d for  thread no. : %d\n",errno,mem_info.tdata_hp[num_th].thread_num);
        	}	
        	mem_info.tdata_hp[num_th].tlbie_shm_ptr = (int  *) shmat (mem_info.tdata_hp[num_th].tlbie_shm_id, (int *)0, 0);
        	if ((int *)mem_info.tdata_hp[num_th].tlbie_shm_ptr == (int *)-1) {
            	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: shmat (4k*4k) errored out with errno : %d for  thread no. : %d\n",errno,mem_info.tdata_hp[num_th].thread_num);
        	}
        	else {
          		display(HTX_HE_INFO,DBG_INFO_PRINT,"TLBIE: Attached (4k*4k) shared memory segment at virtual address \
  				0x%x, pid =%d\n",tlbie_shm_ptr, pid);
        	}	
    	}

		for ( num_th = 0 ; num_th < num_threads ; num_th++) {
    		if (pthread_create(&mem_info.tdata_hp[num_th].tid,NULL,(void *(*)(void *))do_TLB_write_read_compare_mem, (void *)&mem_info.tdata_hp[num_th])) {
        		displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,"Error in creating thread no %d, returned = %d\n",num_th,errno);
       		}	
		}
	}
	else {
		for (num_th = 0 ; num_th < num_threads ; num_th++){
			mem_info.tdata_hp[num_th].thread_num = num_th;
			/* Generate a random number:*/ 

			while(1){
				page_index = (rand_r(seed1) % 3);
				if( mem_info.pdata[page_index].supported ){
					break;
				}
				j1++;
			}
			if(page_index == PAGE_INDEX_4K){
				mem_info.tdata_hp[num_th].rand_page_sz = 4 * KB;
			}	
			else if (page_index == PAGE_INDEX_64K) {
				mem_info.tdata_hp[num_th].rand_page_sz = 64 * KB;
			}
			else {
				mem_info.tdata_hp[num_th].rand_page_sz = 16 * MB;
			}
		
			displaym(HTX_HE_INFO,DBG_IMP_PRINT,"TLBIE : thread %d is using %s page backed shm memory\n",mem_info.tdata_hp[num_th].thread_num,page_size[page_index]);	

			j1++;
		}	
		for ( num_th = 0 ; num_th < num_threads ; num_th++) {
    		if (pthread_create(&mem_info.tdata_hp[num_th].tid,NULL,(void *(*)(void *))do_TLB_write_read_compare_mem_new, (void *)&mem_info.tdata_hp[num_th])) {
        		displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,"Error in creating thread no %d, returned = %d\n",num_th,errno);
       		}	
            else {
                displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"TLBIE : thread %d is created sucessfull\n",num_th);
            }
		}
	}
		
	for ( num_th = 0 ; num_th < num_threads ; num_th++) {
        int thread_retval,rc;
        void *th_join_result = (void *)&thread_retval;

        rc = pthread_join(mem_info.tdata_hp[num_th].tid, &th_join_result);
		mem_info.tdata_hp[num_th].tid = -1;
        if ( rc != 0 ) {
            displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,"Error in Joining thread no %d, returned = %d\n",num_th,rc);
        }
	}
	return 0;
}


void do_TLB_write_read_compare_mem(void* th)
{
    int iter;
    struct thread_data* th_loc = (struct thread_data*)th;
    int *parse_write_shm_ptr,*parse_read_shm_ptr;
    unsigned int buf;
    unsigned int *seed1,*seed2;
    unsigned int i,j1=12, j2=13;
    int bind_cpu_rand;
	char msg_text[4096];
    seed1 = &j1;
    seed2 = &j2;
	int	num_4k_pages = (stanza_ptr->seg_size[PAGE_INDEX_4K]) / (4*KB);
    int ptr_increment = (4*KB)/sizeof(int);
    for (iter=0; iter<=stanza_ptr->num_oper; iter++, j1++, j2++) {
    	parse_write_shm_ptr = (int *)th_loc->tlbie_shm_ptr;
    	parse_read_shm_ptr = (int *)th_loc->tlbie_shm_ptr;
    	bind_cpu_rand = rand_r(seed1);
    	bind_cpu_rand = bind_cpu_rand%get_num_of_proc();
    	do_the_bind_proc(BIND_TO_THREAD, bind_cpu_rand,-1);
    	for (i = 0; i < num_4k_pages; i++) {
        	*parse_write_shm_ptr=0xBEEFDEAD;
       		 parse_write_shm_ptr += ptr_increment ;
    	}
    	do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
    	bind_cpu_rand = rand_r(seed2);
    	bind_cpu_rand = bind_cpu_rand % sys_details.total_cpus;
#ifndef __HTX_LINUX__
		while(1) {
			/* Check for signal handler falg,If any other thread is upadting logical_cpus[] list then wait */
			while(sig_flag){
			  /*do nothing*/
			}
			if(logical_cpus[bind_cpu_rand] == 1) {
				break;
			}
			bind_cpu_rand = (bind_cpu_rand++) % sys_details.total_cpus;
		}
		if(logical_cpus[bind_cpu_rand] != 1) {
			iter=0;
			continue;
		}
#endif
    	do_the_bind_proc(BIND_TO_THREAD, bind_cpu_rand,-1);
		displaym(HTX_HE_INFO, DBG_DEBUG_PRINT, "TLBIE:Thread:%d bound to cpu:%d\n",th_loc->thread_num,bind_cpu_rand);
	

    	for (i = 0; i < num_4k_pages; i++) {
       	  	 buf = *(unsigned int*)parse_read_shm_ptr;
       		 parse_read_shm_ptr += ptr_increment ;
       		if (buf != 0xBEEFDEAD) {
        		/* trap to kdb on miscompare
        		*R3=0xBEEFDEAD
        		*R4=expected value
        		*R5=Actual value
				*R6=current page number
				*R7=read buffer address
        		*R8=CPU number to bound 
        		*R9=Thread number
				*R10=current num oper*/
        		if((stanza_ptr->misc_crash_flag) && (priv.htxkdblevel)){
					displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "TLBIE:Miscompare Detected\n");
 					#ifdef __HTX_LINUX__
                	do_trap_htx64 ((unsigned long)0xBEEFDEAD,\
                                (unsigned long)0xBEEFDEAD,\
                                (unsigned long)buf,\
                                (unsigned long)i,\
                                (unsigned long)parse_read_shm_ptr,\
								(unsigned long)bind_cpu_rand,\
								(unsigned long)th_loc->thread_num,\
                                (unsigned long)iter);
            		#else

			   		trap1(0xBEEFDEAD,0xBEEFDEAD,buf,i,parse_read_shm_ptr,bind_cpu_rand,th_loc->thread_num,iter);
					#endif
				}
 		        sprintf(msg_text,"MISCOMPARE(hxetlbie) in rule %s,Rules file=%s\n"
        	            "Expected pattern=%x,Actual value=%x at Effective address = %llx\n"
            	        "Segment address=%llx,num oper=%d,Thread number = %d, CPU number bound with = %d\n",\
                	    stanza_ptr->rule_id,priv.rules_file_name,\
                    	0xBEEFDEAD,(unsigned int)buf,(unsigned long long)parse_read_shm_ptr,\
						(unsigned long long)th_loc->tlbie_shm_ptr,iter,th_loc->thread_num,bind_cpu_rand);
           		displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "TLBIE:Miscompare Detected\n%s", msg_text);
            	exit(1);

       		 }
    	 }
    	do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);

  	/* Create a new  shared memory region */
     	th_loc->sec_shm_id = shmget(IPC_PRIVATE,stanza_ptr->seg_size[PAGE_INDEX_4K],IPC_CREAT | 0666);
     	if (th_loc->sec_shm_id == -1) {
          	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: secondary shmget (4k*4k) errored out with_loc errno:%d\n"\
                    ,errno);
        }


     	/* Attach the new shared memory region */
     	th_loc->sec_shm_ptr = (int  *) shmat (th_loc->sec_shm_id, (int *)0, 0);
     	if ((int *)th_loc->sec_shm_ptr == (int *)-1) {
          displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: shmat (16M) errored out with_loc errno : %d\n",errno);
     	}
     	else {
            displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"TLBIE: Attached (16M) shared memory segment at virtual address\
 0x%x, tid=%d\n",th_loc->sec_shm_ptr,th_loc->tid);
        }
     	/* Dettach the first shm */
     	if(shmdt(th_loc->tlbie_shm_ptr) == -1) {
           displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE : shmdt (16M) errored out with_loc errno : %d\n",errno);
    	 }
    	 else {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"TLBIE: Dettached (16M) shared memory segment at virtual address\
 			0x%x, tid=%llx\n",th_loc->tlbie_shm_ptr, th_loc->tid);    
     }

    	 /* Remove the first shm area */
     	if (shmctl(th_loc->tlbie_shm_id,IPC_RMID, 0) == -1) {
          displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: SHM REMOVE shmctl(16M) errored out with_loc errno:%d\n"\
                    ,errno);
        }

      	/* Copy the new shared memory variable into the old shared memory variable */
      	th_loc->tlbie_shm_id = th_loc->sec_shm_id;
      	th_loc->tlbie_shm_ptr = th_loc->sec_shm_ptr;

        display(HTX_HE_INFO,DBG_DEBUG_PRINT,"TLBIE: iter no = %d , pid =%d\n",iter, pid);


        if (priv.exit_flag == 1) {
            break;
        }
	
    }
	/* Dettach the rem shm */
    if(shmdt(th_loc->tlbie_shm_ptr) == -1) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE : shmdt (16M) errored out with_loc errno : %d\n",errno);
    }else {
        if (strcmp(r.messages,"YES") == 0) {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"TLBIE: Dettached (16M) shared memory segment at virtual address\
			0x%x, tid=%llx\n",th_loc->tlbie_shm_ptr, th_loc->tid);
        }
    }


    /* Remove the remaining shm area */
    if (shmctl(th_loc->tlbie_shm_id,IPC_RMID, 0) == -1) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: SHM REMOVE shmctl(16M) errored out with_loc errno : %d\n",errno);
    }

    display(HTX_HE_INFO,DBG_IMP_PRINT,"TLBIE : End of the TLBIE test case\n");


}


void do_TLB_write_read_compare_mem_new(void* th)
{
    int rc=0,iter,tlbie_shm_id,*tlbie_shm_ptr,memflg;
    struct thread_data* th_loc = (struct thread_data*)th;
    unsigned int *parse_write_shm_ptr,*parse_read_shm_ptr,*page_ptr;
    unsigned int i,j,cpu_index;
    int bind_cpu,write_flag =0 ;
	char msg_text[4096];/*pattern[sys_details.num_cores][8];*/
	struct pattern_typ {
		short core_num;
		short hexa;
		short thread_num;
		short temp;
	}pattern[sys_details.num_cores];
	unsigned long *pattern_ptr;
	struct pattern_typ temp_obj;
	
	struct shmid_ds shm_buf;

	int ptr_increment = (th_loc->rand_page_sz/sys_details.num_cores)/sizeof(int);
	int num_pages	  = ((stanza_ptr->seg_size[PAGE_INDEX_4K]) / th_loc->rand_page_sz);
	int page_offset   = (th_loc->rand_page_sz/sizeof(int));
	
	for (i = 0; i < sys_details.num_cores; i++) {
            pattern[i].core_num = i;
            pattern[i].hexa = 0xAA;
            pattern[i].thread_num = th_loc->thread_num;
			pattern[i].temp = 0;
	}			
    for (iter=0; iter<=stanza_ptr->num_oper; iter++) {
	#ifndef __HTX_LINUX__
		tlbie_shm_id = shmget(IPC_PRIVATE,stanza_ptr->seg_size[PAGE_INDEX_4K] ,IPC_CREAT | 0666);
		if(tlbie_shm_id == -1) {
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: shmget errored out with errno : %d for  thread no. : %d\n",errno,th_loc->thread_num);
		}
		shm_buf.shm_pagesize=th_loc->rand_page_sz;
		if((shmctl(tlbie_shm_id,SHM_PAGESIZE, &shm_buf)) == -1){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"shmctl failed with %d while setting page size.\n",errno);
		}
	#else
		memflg = (IPC_CREAT | IPC_EXCL |SHM_R | SHM_W );
		if(th_loc->rand_page_sz == 16 *MB){
			memflg |= SHM_HUGETLB;
		}
		tlbie_shm_id = shmget(IPC_PRIVATE,stanza_ptr->seg_size[PAGE_INDEX_4K],memflg);
		if(tlbie_shm_id == -1) {
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: shmget errored out with errno : %d for  thread no. : %d\n",errno,th_loc->thread_num);
		}
	#endif
		tlbie_shm_ptr = (int  *) shmat (tlbie_shm_id, (int *)0, 0);
		if ((int *)tlbie_shm_ptr == (int *)-1) {
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: shmat errored out with errno : %d for  thread no. : %d\n",errno,th_loc->thread_num);
		}
		else {
    		display(HTX_HE_INFO,DBG_DEBUG_PRINT,"TLBIE: Attached shared memory segment at virtual address \
       		0x%x\n",tlbie_shm_ptr);
    	}
		
	
		parse_write_shm_ptr = (unsigned int *)tlbie_shm_ptr;
    	parse_read_shm_ptr = (unsigned int *)tlbie_shm_ptr;
		page_ptr = parse_write_shm_ptr;
    	for (i = 0; i < sys_details.num_cores; i++) {
		bind_cpu = sys_details.cpus_in_core_array[i][0];
#ifndef __HTX_LINUX__
			for(cpu_index=1;cpu_index < sys_details.smt_per_core_array[i]; cpu_index++) {
				/* Check for signal handler falg,If any other thread is upadting logical_cpus[] list then wait */
				while(sig_flag){
		  			/*do nothing*/
				}
				if(logical_cpus[bind_cpu] == 1) {
					displaym(HTX_HE_INFO, DBG_DEBUG_PRINT, "TLBIE:Thread:%d binding  to cpu:%d\n",th_loc->thread_num,bind_cpu);
                        	break;
                }
				bind_cpu = sys_details.cpus_in_core_array[i][cpu_index];
			}
			if ( cpu_index == sys_details.smt_per_core_array[i]) { /* If all cpus are ofline from that core then continue */
				#if 0
				parse_write_shm_ptr = page_ptr;				/*incrememt pointers as current core is skipped*/
				parse_write_shm_ptr += (i * ptr_increment);	
				continue;
				#endif
				write_flag = 1;
			}
#endif
				
			rc = do_the_bind_proc(BIND_TO_THREAD,bind_cpu,bind_cpu);
#ifdef __HTX_LINUX__
			if(rc < 0) {
				if(rc == -2 || rc == -1) {
					/* If this cpu has been hotpluged then bind to next available cpu of same core */
					for(cpu_index=1;cpu_index < sys_details.smt_per_core_array[i]; cpu_index++) {
						bind_cpu = sys_details.cpus_in_core_array[i][cpu_index];
						rc = do_the_bind_proc(BIND_TO_THREAD,bind_cpu,bind_cpu);
						if(rc > 0){
							displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"Thread %d bound to cpu:%d\n",th_loc->thread_num,bind_cpu);
							break;
						}
					
					}
					if ( cpu_index == sys_details.smt_per_core_array[i]) { /* If all cpus are ofline from that core then et a flag to write known pattern which will not be compared*/
						write_flag = 1;
					}
				}
				else
						displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread %d bind failed with rc = %d\n for pcpu=%d",th_loc->thread_num,rc,bind_cpu);	
			}
			else
						displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"Thread %d bound to cpu:%d\n",th_loc->thread_num,bind_cpu);
#endif
			parse_write_shm_ptr = page_ptr;
			parse_write_shm_ptr += (i * ptr_increment);	
			
			for(j=0;j<num_pages;j++) {
				/*sprintf(pattern[i],"%X%X%X%X\n",i,0xA,0xA,th_loc->thread_num);*/
				if(write_flag){
					*parse_write_shm_ptr = 0xFFFFFFFF;
				}
				else {
					pattern_ptr = (unsigned long*)&pattern[i];
        			memcpy(parse_write_shm_ptr, pattern_ptr, sizeof(struct pattern_typ));
				}
				/*printf("at adrress %llx ===>%llx  and pattern is %llx\n",parse_write_shm_ptr,*(unsigned long*)parse_write_shm_ptr,*pattern_ptr);*/
       			parse_write_shm_ptr += page_offset;
    		}
			write_flag = 0;
			do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
		}

	
		page_ptr = parse_read_shm_ptr;
    	for (i = 0; i < sys_details.num_cores; i++) {
			bind_cpu = sys_details.cpus_in_core_array[i][0];
#ifndef __HTX_LINUX__
            for(cpu_index=1;cpu_index < sys_details.smt_per_core_array[i]; cpu_index++) {
                /* Check for signal handler falg,If any other thread is upadting logical_cpus[] list then wait */
                while(sig_flag){
                    /*do nothing*/
                }
                if(logical_cpus[bind_cpu] == 1) {
                    displaym(HTX_HE_INFO, DBG_DEBUG_PRINT, "TLBIE:Thread:%d binding  to cpu:%d\n",th_loc->thread_num,bind_cpu);
                            break;
                }
                bind_cpu = sys_details.cpus_in_core_array[i][cpu_index];
            }
			#if 0
            if ( cpu_index == sys_details.smt_per_core_array[i]) { /* If all cpus are ofline from that core then continue */
				parse_read_shm_ptr = page_ptr;  /* Increment pointers as core is skipped*/
				parse_read_shm_ptr += (i * ptr_increment) ;
                continue;
            }
			#endif 
#endif

			rc = do_the_bind_proc(BIND_TO_THREAD,bind_cpu,bind_cpu);
#ifdef __HTX_LINUX__
			if(rc < 0) {
				if(rc == -2 || rc == -1) {
					for(cpu_index=1;cpu_index < sys_details.smt_per_core_array[i]; cpu_index++) {
						bind_cpu = sys_details.cpus_in_core_array[i][cpu_index];
						rc = do_the_bind_proc(BIND_TO_THREAD,bind_cpu,bind_cpu);
						if(rc > 0){
							displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"Thread %d bound to cpu:%d\n",th_loc->thread_num,bind_cpu);
							break;
						}
					
					}
				}
				else
						displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread %d bind failed with rc = %d\n for pcpu=%d",th_loc->thread_num,rc,bind_cpu);	
			}
			else
						displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"Thread %d bound to cpu:%d\n",th_loc->thread_num,bind_cpu);
#endif
			parse_read_shm_ptr = page_ptr;
			parse_read_shm_ptr += (i * ptr_increment) ;
			for(j=0;j<num_pages;j++) {
				pattern_ptr = (unsigned long*)&pattern[i];
				memcpy(&temp_obj, parse_read_shm_ptr, sizeof(struct pattern_typ));
				if ( memcmp(&temp_obj, pattern_ptr, sizeof(struct pattern_typ) ) ) {
					 if( *parse_read_shm_ptr != 0xFFFFFFFF) {
					
							/* trap to kdb on miscompare
							*R3=0xBEEFDEAD
							*R4=Expected value address
							*R5=actual value address  address
							*R6=page number
							*R7=CPU number to bound 
							*R8=Thread number
							*R9=current num_oper
							*R10=thread context */
							if((stanza_ptr->misc_crash_flag) && (priv.htxkdblevel)){
								displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "TLBIE:Miscompare Detected\n");
								#ifdef __HTX_LINUX__
								do_trap_htx64 ((unsigned long)0xBEEFDEAD,\
										(unsigned long)&pattern[i],\
										(unsigned long)parse_read_shm_ptr,\
										(unsigned long)j,\
										(unsigned long)bind_cpu,\
										(unsigned long)th_loc->thread_num,\
										(unsigned long)iter,\
										(unsigned long)th_loc);
								#else

								trap1(0xBEEFDEAD,pattern[i],parse_read_shm_ptr,j,bind_cpu,th_loc->thread_num,iter,th_loc);
								#endif
							}
							sprintf(msg_text,"MISCOMPARE(hxetlbie) in rule %s,Rules file=%s\n"
									"Expected pattern=%llx,Actual value=%llx at Effective address = %llx\n"
									"Segment address=%llx,num oper=%d,Thread number = %d, CPU number bound with = %d\n",\
									stanza_ptr->rule_id,priv.rules_file_name,\
									*((unsigned long long*)pattern_ptr),*(unsigned long long*)&temp_obj,(unsigned long long)parse_read_shm_ptr,\
									(unsigned long long)tlbie_shm_ptr,iter,th_loc->thread_num,bind_cpu);
							displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "TLBIE:Miscompare Detected\n%s", msg_text);
							exit(1);

					}
       		 	}	
		
    	   		parse_read_shm_ptr += page_offset;
		 }
		do_the_bind_proc(BIND_TO_THREAD, UNBIND_ENTITY,-1);
		}
		
    	 
     	/* Dettach the first shm */
     	 if(shmdt(tlbie_shm_ptr) == -1) {
           displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE : shmdt (16M) errored out with_loc errno : %d\n",errno);
    	 }
    	 else {
            display(HTX_HE_INFO,DBG_DEBUG_PRINT,"TLBIE: Dettached (16M) shared memory segment at virtual address\
 			0x%x, tid=%llx\n",tlbie_shm_ptr, th_loc->tid);    
     	 }
			
    	 /* Remove the first shm area*/ 
     	if (shmctl(tlbie_shm_id,IPC_RMID, 0) == -1) {
          displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE: SHM REMOVE shmctl(16M) errored out with_loc errno:%d\n"\
                    ,errno);
        }
		
        if (priv.exit_flag == 1) {
            break;
        }
	 /*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"num_oper = %d completed for thread = %d\n",iter,th_loc->thread_num);*/
	}	
}
int get_system_details() 
{

	int rc=0, i, j,count=0;
	htxsyscfg_cpus_t system_cpu_information;
	htxsyscfg_core_t t;
	SYS_STAT         Sys_stat;

	/* Ask for the library to update its info */
	rc = repopulate_syscfg(&stats);
	if( rc != 0) {
		while(count < 60){
			sleep(5);
			rc = repopulate_syscfg(&stats);
			if(!rc){
				break;
			}
			else {
				count++;
			}
		}
		if(count == 60){
			displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "Repopulation of syscfg unsuccessful. Exiting\n");
			exit(1);
		}
	}
	
	phy_logical_virt_cpus(&system_cpu_information);
	sys_details.total_cpus = system_cpu_information.l_cpus;
	displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"**Total logical cpus:%d\n",sys_details.total_cpus);
	for(i=0; i<sys_details.total_cpus; i++) {
		logical_cpus[i] = 1;
	}
	rc = get_smt_details(&(t.smtdetails));
	if( rc != 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE:get_smt_details() failed\n");
		exit(1);
	}
	Sys_stat.cores = 0;
	get_hardware_stat(&Sys_stat);
	sys_details.num_cores = Sys_stat.cores;
	
	for(i=0;i<sys_details.num_cores;i++){	
		int smt;
		smt = get_smt_status(i);
		if(smt == -1) {
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"TLBIE:get_smt_status() for core %d failed\n",i);
			exit(1);
		}
		sys_details.smt_per_core_array[i] = smt;
		#ifdef __HTX_LINUX__
		rc = get_phy_cpus_in_core(i,sys_details.cpus_in_core_array[i]);
		#else
		rc = get_cpus_in_core(i,sys_details.cpus_in_core_array[i]);
		#endif
		for(j=0 ; j<smt; j++) {
			displaym(HTX_HE_INFO, DBG_DEBUG_PRINT, "sys_details.cpus_in_core_array[%d][%d]=%d\n",i,j,sys_details.cpus_in_core_array[i][j]);
		}
	}

	if(sys_details.num_cores > 512) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"cores are more than 512,all cores will not touch page\n");
		exit(1);
	}
	return rc;
}
