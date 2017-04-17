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
#include "nest_framework.h"
#include <sys/resource.h>

extern char page_size_name[MAX_PAGE_SIZES][8];
extern struct nest_global g_data;
int cpu_add_remove_test = 0;
struct shm_alloc_th_data {
		int thread_num;
		int chip_num;
		int bind_cpu_num;
		int chip_cpu_num;
		pthread_t tid;
}*shm_alloc_th=NULL;
void SIGRECONFIG_handler (int sig, int code, struct sigcontext *scp);
int check_ame_enabled(void);
int check_if_ramfs(void);
int fill_per_chip_segment_details(void);
int fill_system_segment_details(void);
int run_mem_stanza_operation(void);
int fill_thread_context(void);
int fill_thread_context_with_filters_disabled(void);
int log_mem_seg_details(void);
int create_and_run_thread_operation(void);
int reset_segment_owners(void);
int get_shared_memory(void);
int allocate_buffers(int,int,struct shm_alloc_th_data* );
int deallocate_buffers(int,int,struct shm_alloc_th_data*);
int remove_shared_memory(void);
int fill_affinity_based_thread_structure(struct chip_mem_pool_info*, struct chip_mem_pool_info*,int);
int do_mem_operation(int);
int do_rim_operation(int);
int do_dma_operation(int);
int get_mem_access_operation(int);
void print_memory_allocation_seg_details(void);
void* get_shm(void*);
void* remove_shm(void*);
void* mem_thread_function(void *);
void* stats_update_thread_function(void*);
int apply_filters(void);
#ifdef __HTX_LINUX__
void SIGUSR2_hdl(int, int, struct sigcontext *);
#endif

struct mem_exer_info mem_g;
/*op_fptr operation_fun_typ[MAX_PATTERN_TYPES][MAX_MEM_ACCESS]= {
    {&mem_operation_write_dword,&mem_operation_write_word,&mem_operation_write_byte,&mem_operation_read_dword,
            &mem_operation_read_word,&mem_operation_read_byte,&mem_operation_comp_dword,\
            &mem_operation_comp_word,&mem_operation_comp_byte,NULL,NULL,NULL,NULL,NULL},
    {&mem_operation_write_dword,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
    {&mem_operation_write_addr,NULL,NULL,NULL,NULL,NULL,&mem_operation_comp_addr,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
    {&mem_operation_write_dword,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}
};*/

op_fptr operation_fun_typ[MAX_PATTERN_TYPES][MAX_OPER_TYPES][MAX_MEM_ACCESS_TYPES]= 
{
    {
        {&mem_operation_write_dword,&mem_operation_write_word,&mem_operation_write_byte},
        {&mem_operation_read_dword,&mem_operation_read_word,&mem_operation_read_byte},
        {&mem_operation_comp_dword,&mem_operation_comp_word,&mem_operation_comp_byte},
        {&mem_operation_rim_dword,&mem_operation_rim_word,&mem_operation_rim_byte}
    },
    {
        {&pat_operation_write_dword,&pat_operation_write_word,&pat_operation_write_byte},
        {&mem_operation_read_dword,&mem_operation_read_word,&mem_operation_read_byte},
        {&pat_operation_comp_dword,&pat_operation_comp_word,&pat_operation_comp_byte},
        {&pat_operation_rim_dword,&pat_operation_rim_word,&pat_operation_rim_byte}
    },
    {
        {&mem_operation_write_addr,NULL,NULL},
        {&mem_operation_read_dword,NULL,NULL},
        {&mem_operation_comp_addr,NULL,NULL},
        {&mem_operation_write_addr_comp,NULL,NULL}
    },
    {
        {&rand_operation_write_dword,&rand_operation_write_word,&rand_operation_write_byte},
        {&mem_operation_read_dword,&mem_operation_read_word,&mem_operation_read_byte},
        {&rand_operation_comp_dword,&rand_operation_comp_word,&rand_operation_comp_byte},
        {&rand_operation_rim_dword,&rand_operation_rim_word,&rand_operation_rim_byte}
    }
};
int mem_exer_opearation(){
	int rc,previous_stanza_threads=0;

    mem_g.shm_cleanup_done = 0;
    rc = atexit((void*)remove_shared_memory);
    if(rc != 0) {
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d] Error: Could not register for atexit!\n",__LINE__);
        return ( rc );
    }

#ifndef __HTX_LINUX__
    /*Register SIGRECONFIG handler */
    g_data.sigvector.sa_handler = (void (*)(int)) SIGRECONFIG_handler;
    sigaction(SIGRECONFIG, &g_data.sigvector, (struct sigaction *) NULL);
#endif
	/* update global pointer */
	g_data.test_type_ptr = (struct mem_test_info*)&mem_g;

	/* If cpus are in dedicated mode, disable cpu/mem filter usage, instaed use full system mem view*/
	if(g_data.sys_details.shared_proc_mode == 1){
		displaym(HTX_HE_INFO,DBG_MUST_PRINT,"shared proc mode detected,disabling cpu/mem filter usage\n");
		g_data.gstanza.global_disable_filters = 1;
	}	
	/* Check if AME is enabled on system */
#ifndef __HTX_LINUX__
    rc =  check_ame_enabled();    
    if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:check_ame_enabled()failed wit rc = %d\n",
            __LINE__,__FUNCTION__,rc);
        return (FAILURE);
    }
#endif

	/* Checking if filesystem is on RAM if yes dma_flag would be set */
    rc = check_if_ramfs();
	if(rc != SUCCESS){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:check_if_ramfs() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
		return(FAILURE);
	}
	rc = fill_exer_huge_page_requirement();
	if(rc != SUCCESS){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_exer_huge_page_requirement() failed wit rc = %d\n",
			__LINE__,__FUNCTION__,rc);	
		return (FAILURE);
	}
    if (g_data.exit_flag == SET) {
        exit(1);
    }

    mem_g.total_segments = 0;/*initialize overall segments count to 0*/
    if(mem_g.memory_allocation == ALLOCATE_MEM){
        if(g_data.gstanza.global_disable_filters == 1){
            /* consider % of whole memory and fill segment details for all supported page size pools*/
            rc = fill_system_segment_details();
            if  ( rc != SUCCESS){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_system__segment_details() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
                return (FAILURE);
            }
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Due to system configuration,Memory Exercising will happen at System level memory,\n"
                "Total threads to be created to get shared memory =%d\n",g_data.gstanza.global_num_threads);
        }else{
        /* consider % of per chip memory and fill segment details for all supported page size pools*/
            rc = fill_per_chip_segment_details();
            if  ( rc != SUCCESS){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_per_chip_segment_details() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
                return (FAILURE);
            }
            /* consider strict local affinity to allocate memory*/
            #ifndef __HTX_LINUX__
             if ( g_data.test_type == MEM){
                rc = system("vmo -o enhanced_affinity_vmpool_limit=-1");    
                if(rc < 0){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"vmo -o enhanced_affinity_vmpool_limit=-1"
                    "failed errno %d rc = %d\n",errno,rc);
                    return(FAILURE);
                }
                displaym(HTX_HE_INFO,DBG_IMP_PRINT,"vmo -o enhanced_affinity_vmpool_limit=-1 rc = %d,errno =%d\n",rc,errno);
            }
            #endif
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Total threads to be created to get shared memory =%d\n",g_data.sys_details.tot_cpus);
        }
        #ifdef __HTX_LINUX__
        rc = modify_shared_mem_limits_linux(mem_g.total_segments);
        if  ( rc != SUCCESS){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:modify_shared_mem_limits_linux() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
            return (FAILURE);
        }
        #endif
        int current_priority = getpriority(PRIO_PROCESS,g_data.pid);
        if(current_priority == -1){
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:getpriority() failed with %d,while retrieving crrent priority of exer(pid : %llu) errno:%d(%s)\n",
                __LINE__,__FUNCTION__,current_priority,g_data.pid,errno,strerror(errno));
        }
        rc = setpriority(PRIO_PROCESS,g_data.pid,0);/* boost exer priority to zero temporarily so that mem allocation finishes quickly*/
        if(rc == -1){
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:setpriority() failed with %d while setting priority to 0, errno:%d(%s)\n",
                __LINE__,__FUNCTION__,rc,errno,strerror(errno));
        }
        print_memory_allocation_seg_details();
        /*create shared memories by spawning multiple threads*/
        rc = get_shared_memory();
        if(rc != SUCCESS) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:get_shared_memory() failed with rc = %d",__LINE__,__FUNCTION__,rc);
            return (rc);
        }
        if (g_data.exit_flag == SET) {
            remove_shared_memory();
            exit(1);
        }

        if(current_priority != -1){
            rc = setpriority(PRIO_PROCESS,g_data.pid,current_priority);
            if(rc == -1){
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:setpriority() failed with %d,while setting priority back to %d  errno:%d(%s)\n",
                     __LINE__,__FUNCTION__,rc,current_priority,errno,strerror(errno));
            }
        }
	    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"segment details logged into %s/mem_segment_details\n",g_data.htx_d.htx_exer_log_dir);
    }
    pthread_t stats_th_tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create((pthread_t *)&stats_th_tid,&attr,stats_update_thread_function,NULL);
    if(rc != 0){
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_create failed with rc = %d (errno %d):(%s) while creating stats update thread"
            "\ncontinuing without update of stats\n",__LINE__,__FUNCTION__,rc,errno,strerror(errno));
    }
	do {
		STATS_VAR_INIT(test_id, 0)
		g_data.stanza_ptr = &g_data.stanza[0];

		while( strcmp(g_data.stanza_ptr->rule_id,"NONE") != 0)
		{
            if(cpu_add_remove_test == 1){
                g_data.stanza_ptr->disable_cpu_bind = 1;
            }
#if 0
            /* If mem_DR_done flag is set then mem device will goes to DT */
            if(g_data.mem_DR_flag){
                STATS_HTX_UPDATE(RECONFIG);
                g_data.mem_DR_flag= 0;
                remove_shared_memory();
                exit(0);
            }
#endif
            if ( g_data.test_type == TLB){
                rc = run_tlb_operaion();
            }else if (g_data.test_type == MEM || g_data.test_type == FABRICB){
                rc = run_mem_stanza_operation();
            }else{
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:test_type :%d dest not support, exiting ...\n",
                    __LINE__,__FUNCTION__,g_data.test_type);
                exit(1);
            }
            
            if(rc != SUCCESS){
                remove_shared_memory();
                exit(1);
            }
            if (g_data.exit_flag == SET) {
                remove_shared_memory();
                exit(0);
            }
            previous_stanza_threads=g_data.stanza_ptr->num_threads;

            if(g_data.thread_ptr != NULL){
                free(g_data.thread_ptr);
            }
            if(mem_g.mem_th_data != NULL){
                free(mem_g.mem_th_data);
            }
            g_data.stanza_ptr++;
        }
		STATS_HTX_UPDATE(FINISH);
	/*	if ((read_rules()) < 0 ) {
            remove_shared_memory();
			exit(1);
		}*/
        if (g_data.exit_flag == SET) {
            remove_shared_memory();
            return(FAILURE);
        } /* endif */
	}while (g_data.standalone != 1); /* while run type is not standalone (OTH) */

	rc = remove_shared_memory();
	if(rc != SUCCESS) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:remove_shared_memory()failed with rc = %d",__LINE__,__FUNCTION__,rc);
	}
	return(SUCCESS);
}

/**************************************************************************************
*run_mem_stanza_operation: calls specific test case per stanza as specified in rule file*
***************************************************************************************/
int run_mem_stanza_operation(){
	int rc=SUCCESS;
    int prev_debug_level = g_data.gstanza.global_debug_level;

	/* Set the debug level to the debug level specified in this stanza */
	if (g_data.stanza_ptr->debug_level) {
		g_data.gstanza.global_debug_level = g_data.stanza_ptr->debug_level;
	}
    rc = apply_filters();
    if(rc != SUCCESS) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:apply_filters() failed with rc = %d",__LINE__,__FUNCTION__,rc);
        remove_shared_memory();
        return (rc);
    }
    print_memory_allocation_seg_details();
    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Total threads to be created = %d\n",g_data.stanza_ptr->num_threads);
    mem_g.affinity = g_data.stanza_ptr->affinity;

    rc = fill_thread_context();
    if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: fill_thread_context() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
        return(FAILURE);
    }

    rc = log_mem_seg_details();
    if(rc != SUCCESS){
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"log_mem_seg_details() returned rc =%d\n",rc);
     }

     g_data.htx_d.test_id++;
     hxfupdate(UPDATE,&g_data.htx_d);

     rc = create_and_run_thread_operation();
     if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: create_and_run_thread_operation() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
        return(FAILURE);
     }


     g_data.gstanza.global_debug_level = prev_debug_level;	
     return(SUCCESS);
}

int fill_system_segment_details(){
    int pi;
    unsigned long used_mem=0,std_seg_size,rem_seg_size=0;
    struct page_wise_seg_info *sys_seg_details;
    struct mem_info* sys_memptr = &g_data.sys_details.memory_details;
    unsigned int num_segs=0,fixed_size_num_segs=0,seg,k;
    #ifdef  __HTX_LINUX__
    long long hard_limt_mem_size = DEF_SEG_SZ_4K * (32 * KB); /* Linux has a hard limit of 32K for total number of shms (SHMNI)*/
    unsigned long new_seg_size       = sys_memptr->total_mem_avail/(32 * KB);
    #endif
    int total_cpus ;
    if( g_data.gstanza.global_num_threads > 0){
        total_cpus = g_data.gstanza.global_num_threads;
    }else{
        total_cpus = g_data.sys_details.tot_cpus;
        g_data.gstanza.global_num_threads = total_cpus;
    }

    for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
        if((sys_memptr->pdata[pi].supported == TRUE) && (sys_memptr->pdata[pi].free != 0)){
            if (pi < PAGE_INDEX_16G) {
                if(g_data.gstanza.global_alloc_segment_size == -1){
                    std_seg_size = DEF_SEG_SZ_4K; /* 256MB for < 16G */
                }else{	
                    if(g_data.gstanza.global_alloc_segment_size >= sys_memptr->pdata[pi].free){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:alloc_segment_size parameter in rule file %d given value %ld (page:%s)more than %llu"
                            " avialable in system, Either remove 'alloc_segment_size' or set to smaller value.\n",__LINE__,__FUNCTION__,g_data.rules_file_name,
                            g_data.gstanza.global_alloc_segment_size,page_size_name[pi],sys_memptr->pdata[pi].free);
                        return(FAILURE);
                    }
                    std_seg_size = g_data.gstanza.global_alloc_segment_size;
                    #ifdef  __HTX_LINUX__
                    if((sys_memptr->total_mem_avail/std_seg_size) >= 32 * KB){
                        std_seg_size = DEF_SEG_SZ_4K;
                        g_data.gstanza.global_alloc_segment_size= DEF_SEG_SZ_4K;
                        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Overwriting rule file defined segment size to %lu,"
                            " due to linux hardlimit(32K) on num segments per process\n",std_seg_size);
                    }
                    #endif
                }
                #ifdef  __HTX_LINUX__
                if ((sys_memptr->total_mem_avail > hard_limt_mem_size) && (std_seg_size < new_seg_size)) {
                     displaym(HTX_HE_INFO,DBG_MUST_PRINT,"setting standard segment size to %lu, due to linux hardlimit(32K) on num segments/process\n",new_seg_size);
                    std_seg_size = new_seg_size;
                 }
                #endif
            }else{
                std_seg_size = DEF_SEG_SZ_16G; /* 1 16GB page */
            }
            if (pi < PAGE_INDEX_2M){
                if(g_data.gstanza.global_alloc_mem_size> 0){/* if alloc_mem_size is configured then we ignore alloc_mem_percent*/
                    used_mem = g_data.gstanza.global_alloc_mem_size;
                    if(used_mem < sys_memptr->pdata[pi].psize){
                        used_mem = sys_memptr->pdata[pi].psize;
                    }
                    g_data.gstanza.global_alloc_mem_percent= (used_mem / sys_memptr->pdata[pi].free) * 100;
                }else{
                    used_mem    = (g_data.gstanza.global_alloc_mem_percent/100.0) * sys_memptr->pdata[pi].free;
                    if(g_data.gstanza.global_alloc_mem_percent == 0){
                        continue;
                    }
                }
            }else{
                used_mem    = sys_memptr->pdata[pi].free;
            }
            used_mem = (used_mem/sys_memptr->pdata[pi].psize);
            used_mem = (used_mem * sys_memptr->pdata[pi].psize);

            if((used_mem/total_cpus) < std_seg_size){
                if(used_mem != 0){
                    num_segs = total_cpus;
                }
                fixed_size_num_segs=num_segs;
                rem_seg_size = 0;
            }else {
                num_segs      =  (used_mem/std_seg_size);
                rem_seg_size  = (used_mem%std_seg_size);
                fixed_size_num_segs=num_segs;
                if(rem_seg_size > 0){
                    num_segs++;
                    rem_seg_size = rem_seg_size/sys_memptr->pdata[pi].psize;
                    rem_seg_size = rem_seg_size * sys_memptr->pdata[pi].psize;
                }
            }
            /* Allocate malloc memory to hold segment management details for every page pool from whole system mem*/
            sys_memptr->pdata[pi].page_wise_seg_data = (struct page_wise_seg_info*)malloc(num_segs * sizeof(struct page_wise_seg_info));
            if(sys_memptr->pdata[pi].page_wise_seg_data == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno:%d(%s)for size=%llu\n",
                     __LINE__,__FUNCTION__,errno,strerror(errno),num_segs * sizeof(struct page_wise_seg_info));
                return(FAILURE);
            }

            sys_memptr->pdata[pi].num_of_segments 		 =  num_segs;
            sys_seg_details = sys_memptr->pdata[pi].page_wise_seg_data;
            for(seg=0; seg<fixed_size_num_segs; seg++){
                if((used_mem/total_cpus) < std_seg_size){/*If memory (huge pages specially) falls to be less than 256M,adjust segment size to num of threads*/
                    sys_seg_details[seg].shm_size = (used_mem/total_cpus);
                    if((sys_seg_details[seg].shm_size%sys_memptr->pdata[pi].psize)){
                        sys_seg_details[seg].shm_size = sys_seg_details[seg].shm_size/sys_memptr->pdata[pi].psize;
                        sys_seg_details[seg].shm_size = sys_seg_details[seg].shm_size * sys_memptr->pdata[pi].psize;
                    }
                }else{
                    sys_seg_details[seg].shm_size=std_seg_size;
                }
                sys_seg_details[seg].seg_num   =   seg;
                sys_seg_details[seg].original_shm_size = sys_seg_details[seg].shm_size;	
            }
                /*
                * Preserve value of seg from above loop
                * in case another segment of smaller size is possible
                */
            if (rem_seg_size > 0){
                sys_seg_details[seg].shm_size = rem_seg_size;
                sys_seg_details[seg].original_shm_size = rem_seg_size;
                sys_seg_details[seg].seg_num   =   seg;
            }			
            for(k=0;k<num_segs;k++){
                sys_seg_details[k].shm_mem_ptr = NULL;
                sys_seg_details[k].shmid       = -1;
                debug(HTX_HE_INFO,DBG_DEBUG_PRINT," seg_num=%d:shm sizes[%u] = %lu (%s pages:%lu)\n",
                    sys_seg_details[k].seg_num,k,sys_seg_details[k].shm_size,page_size_name[pi],
                    (sys_seg_details[k].shm_size/sys_memptr->pdata[pi].psize));
            }
        }
        mem_g.total_segments += sys_memptr->pdata[pi].num_of_segments;
    }
    return (SUCCESS);
}
/************************************************************************************
*Alocates shared memory segments for all supported page size pools.
*considering 90% of system avialable memory(per chip),divide page wise segemnts	among threads to allocate memory.						
************************************************************************************/

int fill_per_chip_segment_details(){

    int pi,n,count=0,rc = SUCCESS;
    unsigned long used_mem=0,std_seg_size,rem_seg_size=0;
    struct page_wise_seg_info *seg_details;
    struct mem_info* memptr = &g_data.sys_details.memory_details;
    unsigned int num_segs=0,fixed_size_num_segs=0,seg,k;
    int num_of_chips = g_data.sys_details.num_chip_mem_pools;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
#ifdef  __HTX_LINUX__
    long long hard_limt_mem_size = DEF_SEG_SZ_4K * (32 * KB); /* Linux has a hard limit of 32K for total number of shms (SHMNI)*/
    unsigned long new_seg_size       = memptr->total_mem_avail/(32 * KB);
#else 
    float percent_factor = 0.0;
#endif

    /* In Linux use whichever supported base page(currently either 4k or 64k)*/
    for(n = 0; n < num_of_chips; n++){
        if(g_data.gstanza.global_alloc_huge_page == 0){
            mem_details_per_chip[n].memory_details.pdata[PAGE_INDEX_2M].supported = FALSE;
            mem_details_per_chip[n].memory_details.pdata[PAGE_INDEX_16M].supported = FALSE;
            mem_details_per_chip[n].memory_details.pdata[PAGE_INDEX_16G].supported = FALSE;
        }
        if(!mem_details_per_chip[n].has_cpu_and_mem){
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s:chip %d either does not have memory or cpu behind it" \
                    " free mem = %llu and total cpus = %d\n",__LINE__,__FUNCTION__,n,mem_details_per_chip[n].memory_details.total_mem_free,mem_details_per_chip[n].num_cpus);
                count++;
                continue;
        }
        /* Fabricbus exerciser specific path*/
        if ( g_data.test_type == FABRICB){
                rc = fill_fabb_segment_details(n);
                if(rc != SUCCESS){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_fabb_segment_details() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
                    return rc;
                }else{
                    continue;
                }
        } 
        for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
            if((mem_details_per_chip[n].memory_details.pdata[pi].supported == TRUE) && (mem_details_per_chip[n].memory_details.pdata[pi].free != 0)) {
                if (pi < PAGE_INDEX_16G) {
                    if(g_data.gstanza.global_alloc_segment_size== -1){
                        std_seg_size = DEF_SEG_SZ_4K; /* 256MB for < 16G */
                    }else{
                        if(g_data.gstanza.global_alloc_segment_size >=  mem_details_per_chip[n].memory_details.pdata[pi].free){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:global_alloc_segment_size parameter in rule file %s given value %ld (page:%s)more than %llu for chip %d\n"
                                "Either remove 'global_alloc_segment_size' or set to smaller value.\n",__LINE__,__FUNCTION__,g_data.rules_file_name,
                                g_data.gstanza.global_alloc_segment_size,page_size_name[pi], mem_details_per_chip[n].memory_details.pdata[pi].free,n);
                            return (FAILURE);
                        }
                        std_seg_size = g_data.gstanza.global_alloc_segment_size;
                        #ifdef  __HTX_LINUX__
                        if((memptr->total_mem_avail/std_seg_size) >= 32 * KB){
                            std_seg_size = DEF_SEG_SZ_4K;
                            g_data.gstanza.global_alloc_segment_size= DEF_SEG_SZ_4K;
                            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Overwriting rule file defined segment size to %lu,"
                                " due to linux hardlimit(32K) on num segments per process\n",std_seg_size);
                        }
                        #endif
                    }
#ifdef  __HTX_LINUX__
                    if ((memptr->total_mem_avail > hard_limt_mem_size) && (std_seg_size < new_seg_size)) {
                        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"setting standard segment size to %lu, due to linux hardlimit(32K) on num segments/process\n",new_seg_size);
                        std_seg_size = new_seg_size;
                    }
#endif
                }else {
                    std_seg_size = DEF_SEG_SZ_16G; /* 1 16GB page */
                }	
                if (pi < PAGE_INDEX_2M){
                    if(g_data.gstanza.global_alloc_mem_size > 0){/* if alloc_mem_size is configured then we ignore alloc_mem_percent*/
                        used_mem = g_data.gstanza.global_alloc_mem_size;
                        if(used_mem < mem_details_per_chip[n].memory_details.pdata[pi].psize){
                            used_mem = mem_details_per_chip[n].memory_details.pdata[pi].psize;
                        }
                        g_data.gstanza.global_alloc_mem_percent= (used_mem / mem_details_per_chip[n].memory_details.pdata[pi].free) * 100;
                    }else{
                        used_mem	= (g_data.gstanza.global_alloc_mem_percent/100.0) * mem_details_per_chip[n].memory_details.pdata[pi].free;
                        if(g_data.gstanza.global_alloc_mem_percent == 0){
                            continue;
                        }
                    }
                }else{
                    used_mem    = mem_details_per_chip[n].memory_details.pdata[pi].free;
                }
                used_mem = (used_mem/mem_details_per_chip[n].memory_details.pdata[pi].psize);
                used_mem = (used_mem*mem_details_per_chip[n].memory_details.pdata[pi].psize);

                if((used_mem/mem_details_per_chip[n].num_cpus) < std_seg_size){
                    num_segs = mem_details_per_chip[n].num_cpus;
                    /* Below check is added because on certain system per chip 16M pages found to be less than num cpus in that chip*/
                    if((used_mem/mem_details_per_chip[n].num_cpus) < mem_details_per_chip[n].memory_details.pdata[pi].psize){
                        num_segs = (used_mem / mem_details_per_chip[n].memory_details.pdata[pi].psize);
                    }
                    fixed_size_num_segs=num_segs;
                    rem_seg_size = 0;
                }else {
                    num_segs      =  (used_mem/std_seg_size);
                    rem_seg_size  = (used_mem%std_seg_size);
                    fixed_size_num_segs=num_segs;
                    if(rem_seg_size > 0){
                        num_segs++;
                        rem_seg_size = rem_seg_size/mem_details_per_chip[n].memory_details.pdata[pi].psize;
                        rem_seg_size = rem_seg_size * mem_details_per_chip[n].memory_details.pdata[pi].psize;
                    }
                }
                /* Allocate malloc memory to hold segment management details for every page pool per chip*/
                mem_details_per_chip[n].memory_details.pdata[pi].page_wise_seg_data = (struct page_wise_seg_info*)malloc(num_segs * sizeof(struct page_wise_seg_info));
                if(mem_details_per_chip[n].memory_details.pdata[pi].page_wise_seg_data == NULL){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno:%d(%s)for size=%llu\n",
                        __LINE__,__FUNCTION__,errno,strerror(errno),num_segs * sizeof(struct page_wise_seg_info));
                    return(FAILURE);
                }
                mem_details_per_chip[n].memory_details.pdata[pi].num_of_segments =  num_segs;
                seg_details = mem_details_per_chip[n].memory_details.pdata[pi].page_wise_seg_data;
                for(seg=0; seg<fixed_size_num_segs; seg++){
                    if((used_mem/mem_details_per_chip[n].num_cpus) < std_seg_size){/*If memory (huge pages specially) falls to be less than 256M,adjust segment size to num of threads*/
                        seg_details[seg].shm_size = (used_mem/mem_details_per_chip[n].num_cpus);
                        if(seg_details[seg].shm_size < mem_details_per_chip[n].memory_details.pdata[pi].psize){
                            seg_details[seg].shm_size = mem_details_per_chip[n].memory_details.pdata[pi].psize;
                        }
                        if((seg_details[seg].shm_size%mem_details_per_chip[n].memory_details.pdata[pi].psize)){
                            seg_details[seg].shm_size = seg_details[seg].shm_size/mem_details_per_chip[n].memory_details.pdata[pi].psize;
                            seg_details[seg].shm_size = seg_details[seg].shm_size*mem_details_per_chip[n].memory_details.pdata[pi].psize;
                        }
                    }else{
                            seg_details[seg].shm_size=std_seg_size;
                    }
                    seg_details[seg].seg_num   =   seg;
                    seg_details[seg].original_shm_size = seg_details[seg].shm_size;
                }
                /*
                * Preserve value of seg from above loop
                * in case another segment of smaller size is possible
                */

                if (rem_seg_size > 0){
                    seg_details[seg].shm_size=rem_seg_size;
                    seg_details[seg].original_shm_size=rem_seg_size;
                    seg_details[seg].seg_num = seg;
                }
                for(k=0;k<num_segs;k++){
                    seg_details[k].shm_mem_ptr = NULL;
                    seg_details[k].shmid      = -1;
                    debug(HTX_HE_INFO,DBG_DEBUG_PRINT," seg_num=%d:shm sizes[%u] = %lu (%s pages:%lu)\n",
                        seg_details[k].seg_num,k,seg_details[k].shm_size,page_size_name[pi],
                        (seg_details[k].shm_size/mem_details_per_chip[n].memory_details.pdata[pi].psize));
                }
            }
            mem_g.total_segments += mem_details_per_chip[n].memory_details.pdata[pi].num_of_segments;
        }
    }
    if(count == num_of_chips){
        print_partition_config(HTX_HE_SOFT_ERROR);
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:system configuration does't seems to be good, few chips have only cpus and few have only" 
            "memory, count=%d, num_of_chips=%d\n",__LINE__,__FUNCTION__,count,num_of_chips);
        return (FAILURE);
    }
    return (SUCCESS);
}
/*********************************************************************************************
* get_shared_memory(pi),spawn threads equal to cpus per chip to create shared memory segments 
* for all supported page size pols
*********************************************************************************************/
int get_shared_memory(){
    int ti,n,rc,nchips;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    int tot_sys_threads = g_data.sys_details.tot_cpus;
    int cpus_track=0; 
        
    shm_alloc_th = (struct shm_alloc_th_data*) malloc(sizeof( struct shm_alloc_th_data) * tot_sys_threads);
    if(shm_alloc_th == NULL){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s():malloc failed for thread_mem_alloc_data for with errno:%d(%s)size=%llu\n",
            __LINE__,__FUNCTION__,errno,strerror(errno),(sizeof(struct shm_alloc_th_data) * tot_sys_threads));
        return(FAILURE); 
    }	
    
    if(g_data.gstanza.global_disable_filters == 0){
        nchips = g_data.sys_details.num_chip_mem_pools;	
        for(n=0;n<nchips;n++){
            if(!mem_details_per_chip[n].has_cpu_and_mem){
                displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s:chip%d either does not have memory or cpu behind it" \
                        " free mem = %llu and total cpus = %d\n",__LINE__,__FUNCTION__,n,
                        mem_details_per_chip[n].memory_details.total_mem_free,mem_details_per_chip[n].num_cpus);
                cpus_track  += mem_details_per_chip[n].num_cpus;
                continue;
            }
            for(ti=0;ti<mem_details_per_chip[n].num_cpus;ti++){
                shm_alloc_th[cpus_track+ti].thread_num = (cpus_track+ti);
                shm_alloc_th[cpus_track+ti].chip_num   = n;
                shm_alloc_th[cpus_track+ti].bind_cpu_num   = mem_details_per_chip[n].cpulist[ti];
                shm_alloc_th[cpus_track+ti].chip_cpu_num = ti;
                shm_alloc_th[cpus_track+ti].tid 	 = -1;
                 debug(HTX_HE_INFO,DBG_IMP_PRINT,"Creating thread %d to allocate memory by cpu = %d from chip %d\n",(cpus_track+ti),shm_alloc_th[cpus_track+ti].bind_cpu_num,n); 
                rc = pthread_create((pthread_t *)&(shm_alloc_th[cpus_track+ti].tid),NULL, get_shm,(void *)&(shm_alloc_th[cpus_track+ti]));
                /*printf("##########Pthread_create : cpus_track=%d, ti=%d, tid=%llx\n",  cpus_track, ti, shm_alloc_th[cpus_track+ti].tid);*/
                if ( rc != 0){
                    displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:pthread_create failed rc = %d and errno = %d(%s) for thread %d\n",
                            __LINE__,__FUNCTION__,rc,errno,strerror(errno),cpus_track+ti);
                    return (FAILURE);
                }

            }
            cpus_track  += mem_details_per_chip[n].num_cpus;
            if (g_data.exit_flag == SET) {
                remove_shared_memory();
                exit(1);
            }
        }
        cpus_track=0;	
        for(n=0;n<nchips;n++){
            if(!mem_details_per_chip[n].has_cpu_and_mem){
                cpus_track  += mem_details_per_chip[n].num_cpus;
                continue;
            }
            for(ti=0;ti<mem_details_per_chip[n].num_cpus;ti++){
                /*printf("********Joining th. cpus_track=%d, ti=%d, tid=%llx\n", cpus_track, ti, shm_alloc_th[cpus_track+ti].tid);*/
                rc = pthread_join(shm_alloc_th[cpus_track+ti].tid,NULL);
                if(rc != 0) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join failed rc = %d and errno = %d(%s)for thread %d\n",
                        __LINE__,__FUNCTION__,rc,errno,strerror(errno),cpus_track+ti);
                    return (FAILURE);
                }
                else {
                    /* debug(HTX_HE_INFO,DBG_DEBUG_PRINT," Thread %d  joined successfully after shm allocation\n",cpus_track+ti); */
                    shm_alloc_th[cpus_track+ti].tid          = -1;
                }
            }
            cpus_track	+= mem_details_per_chip[n].num_cpus;
                    
            if (g_data.exit_flag == SET) {
                remove_shared_memory();
                exit(1);
            }
        }
        displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"\n********Shared Memory allocation Completed for CHIP: %d ***************************\n",n);
    }else {
        tot_sys_threads = g_data.gstanza.global_num_threads;
        for(ti=0;ti<tot_sys_threads;ti++){
            shm_alloc_th[ti].thread_num = ti;
            shm_alloc_th[ti].chip_num   = -1;
            shm_alloc_th[ti].bind_cpu_num = ti;
            shm_alloc_th[ti].tid = -1;
            debug(HTX_HE_INFO,DBG_IMP_PRINT,"Creating thread %d to allocate memory by cpu = %d\n",ti,shm_alloc_th[ti].bind_cpu_num);
            rc = pthread_create((pthread_t *)&(shm_alloc_th[ti].tid),NULL, get_shm,(void *)&(shm_alloc_th[ti]));
            if(rc != 0){
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:pthread_create failed rc = %d and errno = %d(%s) for thread %d\n",
                        __LINE__,__FUNCTION__,rc,errno,strerror(errno),ti);
                return (FAILURE);
            }
        }
        if (g_data.exit_flag == SET) {
            remove_shared_memory();
            exit(1);
        }
    
        for(ti=0;ti<tot_sys_threads;ti++){
           rc = pthread_join(shm_alloc_th[ti].tid,NULL);
            if(rc != 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join failed rc = %d and errno = %d(%s)for thread %d\n",
                    __LINE__,__FUNCTION__,rc,errno,strerror(errno),ti);
                return (FAILURE);
            }
            else {
                /* debug(HTX_HE_INFO,DBG_DEBUG_PRINT," Thread %d  joined successfully after shm allocation\n",ti); */
                shm_alloc_th[ti].tid          = -1;
            }
        }

        if (g_data.exit_flag == SET) {
            remove_shared_memory();
            exit(1);
        }
    }	
    return(SUCCESS);
}

int remove_shared_memory(){

    int n,ti,rc,pi;
    int nchips = g_data.sys_details.num_chip_mem_pools,cpus_track=0;
    int tot_sys_threads = g_data.sys_details.tot_cpus;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    if(shm_alloc_th == NULL){
        return SUCCESS;
    }
    if(g_data.gstanza.global_disable_filters == 0){
        for(n=0;n<nchips;n++){
            if(!mem_details_per_chip[n].has_cpu_and_mem){
                cpus_track  += mem_details_per_chip[n].num_cpus;
                continue;
            }
            for(ti=0;ti<mem_details_per_chip[n].num_cpus;ti++){
                debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"Creating thread %d to deallocate memory \n",ti);
                rc = pthread_create((pthread_t *)&shm_alloc_th[cpus_track+ti].tid,NULL,remove_shm,(void *)&shm_alloc_th[cpus_track+ti]);
                if(rc !=0 ){
                    displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:pthread_create failed rc = %d and errno = %d(%s)\n",__LINE__,__FUNCTION__,rc,errno,strerror(errno));
                    return (FAILURE);
                }	
            }
            cpus_track      += mem_details_per_chip[n].num_cpus;
        }
        cpus_track = 0;
        for(n=0;n<nchips;n++){
            if(!mem_details_per_chip[n].has_cpu_and_mem){
                cpus_track  += mem_details_per_chip[n].num_cpus;
                continue;
            }
            for(ti=0;ti<mem_details_per_chip[n].num_cpus;ti++){
                rc = pthread_join(shm_alloc_th[cpus_track+ti].tid,NULL);
                if(rc != 0) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join failed rc = %d and errno = %d(%s)"
                        " for thread %d\n",__LINE__,__FUNCTION__,rc,errno,strerror(errno),cpus_track+ti);
                    return (FAILURE);
                }
            }				
            cpus_track      += mem_details_per_chip[n].num_cpus;
            for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
                if(!g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].supported)continue;
                if(g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_seg_data != NULL){
                    free(g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_seg_data);
                }
            }
        }
        #ifndef __HTX_LINUX__
        if ( g_data.test_type == MEM){
            rc = system("vmo -d enhanced_affinity_vmpool_limit");
            displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%s():vmo -d enhanced_affinity_vmpool_limit rc=%d \t errno =%d\n",\
            __FUNCTION__,rc,errno);
        }
        #endif
    }else{
        tot_sys_threads = g_data.gstanza.global_num_threads;
        for(ti=0;ti<tot_sys_threads;ti++){
            debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"Creating thread %d to deallocate memory \n",ti);
            rc = pthread_create((pthread_t *)&shm_alloc_th[ti].tid,NULL,remove_shm,(void *)&shm_alloc_th[ti]);
            if(rc !=0 ){
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:pthread_create failed rc = %d and errno = %d(%s),thread_num=%d\n",
                    __LINE__,__FUNCTION__,rc,errno,strerror(errno),ti);
                return (FAILURE);
            }
        }
        for(ti=0;ti<tot_sys_threads;ti++){
            rc = pthread_join(shm_alloc_th[ti].tid,NULL);
            if(rc != 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join failed rc = %d and errno = %d(%s)"
                    " for thread %d\n",__LINE__,__FUNCTION__,rc,errno,strerror(errno),ti);
                 return (FAILURE);
            }
        }
        for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
            if(!g_data.sys_details.memory_details.pdata[pi].supported)continue;
            if(g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data != NULL){
                free(g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data);
            }
        }
    }
    if(shm_alloc_th != NULL){
        free(shm_alloc_th);
        shm_alloc_th = NULL;
    }	
    mem_g.shm_cleanup_done = 1;
    return SUCCESS;

}
void* get_shm(void* t){
    struct shm_alloc_th_data *th = (struct shm_alloc_th_data *)t;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    struct mem_info* sys_mem_details = &g_data.sys_details.memory_details;
    int n = th->chip_num;
    int rc,pi,seg_num,num_segs=0,seg_inc = 0;;
    int tot_cpus = g_data.gstanza.global_num_threads;
    if(!g_data.gstanza.global_disable_cpu_bind){
    /*replace below call with bind_to_cpu,common betweeen AIX/LINUX*/
    #ifndef __HTX_LINUX__
        if (g_data.cpu_DR_flag != 1){
            rc = bindprocessor(BINDTHREAD,thread_self(),th->bind_cpu_num);
        }
    #else
        rc = htx_bind_thread(th->bind_cpu_num,-1);
    #endif

        if(rc < 0){/*REV: hotplug/dr need to take care*/
                displaym(HTX_HE_INFO,DBG_MUST_PRINT,"bind call failed for cpu %d (th:%d),local memory may not be guaranteed\n",th->bind_cpu_num,th->thread_num);
        } 
    }
    for(pi=0;pi<MAX_PAGE_SIZES; pi++) {
        if(!sys_mem_details->pdata[pi].supported){
            continue;
        }
        if(g_data.gstanza.global_disable_filters == 0){
            /*segs per chip per page pool will divided among threads of same chip,such that each seg index will be picked up by a cpu (segment_index%total_cpus_in_that_chip)*/
            if(mem_details_per_chip[n].memory_details.pdata[pi].free == 0){
                continue;
            }
            num_segs = mem_details_per_chip[n].memory_details.pdata[pi].num_of_segments;
            seg_num = th->chip_cpu_num % mem_details_per_chip[n].num_cpus;
            seg_inc = mem_details_per_chip[n].num_cpus;
        }else{
            if(sys_mem_details->pdata[pi].free == 0){
                continue;
            }
            num_segs = sys_mem_details->pdata[pi].num_of_segments;
            seg_num = th->thread_num % tot_cpus;
            seg_inc = tot_cpus;
        }
        while(seg_num < num_segs ){
            /*displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_num=%d of page pool=%s in chip %d will be aloocated by thread %d,cpu %d\n",seg_num,page_size_name[pi],n,th->thread_num,th->bind_cpu_num);*/
            rc = allocate_buffers(pi,seg_num,th);
            if(rc != SUCCESS){
                g_data.exit_flag = SET;
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:allocate_buffers failed with rc = %d for allocation thread %d, page pool = %s\n", \
                                __LINE__,__FUNCTION__,rc,th->thread_num,page_size_name[pi]);
                pthread_exit((void *)1);
            }
            /*seg_num +=  mem_details_per_chip[n].num_cpus;*/
            seg_num += seg_inc;
        }
    }

    if(!g_data.gstanza.global_disable_cpu_bind){
    /*replace below call with bind_to_cpu,common betweeen AIX/LINUX*/
    #ifndef __HTX_LINUX__
        rc = bindprocessor(BINDTHREAD, th->tid,-1);
    #else
        rc = htx_unbind_thread();
    #endif
    }
    pthread_exit((void *)0);
}
void* remove_shm(void* t){

    struct shm_alloc_th_data *th = (struct shm_alloc_th_data *)t;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    struct mem_info* sys_mem_details = &g_data.sys_details.memory_details;
    int n = th->chip_num;
    int rc,pi,seg_num=0,num_segs=0,seg_inc = 0;;
    int tot_cpus = g_data.gstanza.global_num_threads;

    for(pi=0;pi<MAX_PAGE_SIZES; pi++) {
        if(!sys_mem_details->pdata[pi].supported){
            continue;
        }
        if(g_data.gstanza.global_disable_filters == 0){
            if(mem_details_per_chip[n].memory_details.pdata[pi].free == 0){
                continue;
            }
            num_segs = mem_details_per_chip[n].memory_details.pdata[pi].num_of_segments;
            seg_num=0;
            seg_num = th->chip_cpu_num % mem_details_per_chip[n].num_cpus;
            seg_inc = mem_details_per_chip[n].num_cpus;
        }else{
            if(sys_mem_details->pdata[pi].free == 0){
                continue;
            }
            num_segs = sys_mem_details->pdata[pi].num_of_segments;
            seg_num=th->thread_num % tot_cpus;
            seg_inc = tot_cpus;
        }
        while(seg_num < num_segs ){
            /*displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_num=%d of page pool=%s in chip %d will be aloocated by thread %d,cpu %d\n",seg_num,page_size_name[pi],n,th->thread_num,th->bind_cpu_num);*/
            rc = deallocate_buffers(pi,seg_num,th);
            if(rc != SUCCESS){
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:deallocate_buffers failed with rc = %d for allocation thread %d, page pool = %s\n", \
                                __LINE__,__FUNCTION__,rc,th->thread_num,page_size_name[pi]);
                pthread_exit((void *)1);
            }
            seg_num += seg_inc;
        }
    }
    pthread_exit((void *)0);
}

int allocate_buffers(int pi, int seg_num, struct shm_alloc_th_data *th){
    unsigned long i, memflg;
    int n = th->chip_num; 
    int bound_cpu = th->bind_cpu_num;
#ifndef __HTX_LINUX__
    struct shmid_ds shm_buf = { 0 };
#endif
    struct page_wise_seg_info *seg_details = NULL;

    if(g_data.gstanza.global_disable_filters == 0){
        seg_details = &g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_seg_data[seg_num];
    }else{
        seg_details = &g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg_num];
    }

    if(g_data.gstanza.global_disable_cpu_bind){
        bound_cpu = -1;
    }
#ifndef __HTX_LINUX__
    memflg = (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#else
    memflg = (IPC_CREAT | IPC_EXCL |SHM_R | SHM_W);
    if(pi >= PAGE_INDEX_2M) {
        memflg |= (SHM_HUGETLB);
    }
#endif
    if (g_data.exit_flag == SET) {
         pthread_exit((void *)1);
    } /* endif */
    seg_details->shmid = shmget (IPC_PRIVATE, seg_details->shm_size, memflg);
    if(seg_details->shmid  == -1){
        char msg[1024];
        sprintf(msg,"[%d]%s:shmget failed with -1 by thread = %d(bound to cpu:%d) for size=%lu,seg_num=%d,page pool=%s,errno:%d(%s)\n", \
                __LINE__,__FUNCTION__,th->thread_num,bound_cpu,seg_details->shm_size,seg_num,page_size_name[pi],errno,strerror(errno));	
        if(errno == ENOMEM){
            strcat(msg,"Looks like previous htx run was not a clean exit,Please check if any stale shared memory chunks are present" 
                ",clear them or reboot the partition, make sure desired free memory is avialable in the system before re starting the test.\n"); 
        }
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s\n",msg);
        return (FAILURE);
    }
    else {
        debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"(shmget success)thread:%d for page:%s, shm id[%d]=%lu and size = %lu\n",
                    th->thread_num,page_size_name[pi],seg_num,seg_details->shmid,seg_details->shm_size);
    }
#ifndef __HTX_LINUX__
    if ( seg_details->shm_size > (256*MB)) {
        if (shmctl(seg_details->shmid,SHM_GETLBA, &shm_buf)) {	
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: shmctl failed with err:(%d)%s, for seg_num = %d,shm_id=%lu while setting SHM_GETLBA"
                "for page = %s and segment_size = %lu \n",__LINE__,__FUNCTION__,errno,strerror(errno),seg_num,seg_details->shmid,page_size_name[pi],seg_details->shm_size);	
            return  (FAILURE);
        }
    }	
    shm_buf.shm_pagesize = g_data.sys_details.memory_details.pdata[pi].psize;

    if (shmctl(seg_details->shmid,SHM_PAGESIZE, &shm_buf)) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:shmctl failed with err:(%d)%s,for seg_num = %d,shm_id=%lu,thread=%d while setting SHM_PAGESIZE"
                    " for page = %s and segment_size = %lu \n",__LINE__,__FUNCTION__,errno,strerror(errno),seg_num,seg_details->shmid,th->thread_num,page_size_name[pi],seg_details->shm_size);
        return  (FAILURE);
    }
#endif
    seg_details->shm_mem_ptr = (void *)shmat(seg_details->shmid,(char *) 0, 0);
    if (seg_details->shm_mem_ptr  == (void *)-1) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:shmat failed with err:(%d)%s,for seg_num = %d,shm_id=%lu while setting SHM_PAGESIZE"
                 "for page = %s and segment_size = %lu \n",__LINE__,__FUNCTION__,errno,strerror(errno),seg_num,seg_details->shmid,page_size_name[pi],seg_details->shm_size);
        return (FAILURE);
    }
    debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"(shmat success)Allocated/Attached  shared memory buffer%d, id = %lu, shm_memp = "
            "0x%lx and page size =%s by thread : %d\n",seg_num,seg_details->shmid,seg_details->shm_mem_ptr,page_size_name[pi],th->thread_num);

    /* write 8 byte value to every page of this segment*/
    long long* temp_ptr = (long long*) seg_details->shm_mem_ptr;
    unsigned long num_pages = 	seg_details->shm_size/g_data.sys_details.memory_details.pdata[pi].psize;
    for(i =0; i < num_pages; i++)
    {
        *temp_ptr = 0xBBBBBBBBBBBBBBBBULL;
        temp_ptr += (g_data.sys_details.memory_details.pdata[pi].psize/8);

    }
    /* initialize other parameters of segment management structure*/
    seg_details->owning_thread   = -1;
    seg_details->page_size_index = pi;
    seg_details->in_use			 = 0;
    seg_details->cpu_chip_num		 = -1; 
    seg_details->mem_chip_num		 = th->chip_num; 

    return (SUCCESS);
}

int deallocate_buffers(int pi, int seg_num, struct shm_alloc_th_data *th){

    int rc;
    int n = th->chip_num; 
    struct page_wise_seg_info *seg_details = NULL;

    if(g_data.gstanza.global_disable_filters == 0){
        seg_details = &g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_seg_data[seg_num];
    }else{
        seg_details = &g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg_num];
    }
    if (seg_details->shm_mem_ptr != NULL) {/* If shared memory head pointer is allocated */
        debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"Removing segment #%d of page size %s by thread: %d "
            "shm_mem id=%lu, shm_memp = 0x%lx\n",seg_num,page_size_name[pi],th->thread_num,seg_details->shmid,seg_details->shm_mem_ptr);
        rc = shmdt(seg_details->shm_mem_ptr);
        if(rc != 0){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:(th:%d)shmdt failed with err:(%d)%s,at adress:0x%lx for seg_num = %d,shm_id=%lu for page = %s"
            "and segment_size = %lu \n",__LINE__,__FUNCTION__,th->thread_num,errno,strerror(errno),seg_details->shm_mem_ptr,seg_num,seg_details->shmid,page_size_name[pi],seg_details->shm_size);
            return (FAILURE);
        }
        rc = shmctl(seg_details->shmid,IPC_RMID,(struct shmid_ds *) NULL);
        if (rc != 0) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:(th:%d)shmctl failed with err:(%d)%s,for seg_num = %d,shm_id=%lu for page = %s"
            "and segment_size = %lu belongs to chip:%d\n",__LINE__,__FUNCTION__,th->thread_num,
            errno,strerror(errno),seg_num,seg_details->shmid,page_size_name[pi],seg_details->shm_size,n);
            return (FAILURE);
        }
        debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"Released shared memory buffer %d id = %d, shm_memp = 0x%lx page size = %s\n",
        seg_num,seg_details->shmid,seg_details->shm_mem_ptr,page_size_name[pi]);
        seg_details->shm_mem_ptr = NULL;
    }
    return SUCCESS;
}

int apply_filters(){

    struct cpu_filter_info* cf = &g_data.stanza_ptr->filter.cf;
    struct mem_filter_info* mf = &g_data.stanza_ptr->filter.mf;
    struct chip_mem_pool_info* chp = &g_data.sys_details.chip_mem_pool_data[0];
    struct page_wise_data *page_wise_details=NULL;
    struct page_wise_data *sys_page_wise_details=NULL;
    int node,chip,core,lcpu,pi,seg,sys_chip_count = 0,rc=SUCCESS;
    long long mem_in_use = 0;
    
    if(g_data.gstanza.global_disable_filters == 0){
        g_data.stanza_ptr->num_threads = 0;
        for(chip =0; chip<MAX_CHIPS;chip++){
            chp[chip].in_use_num_cpus= 0;
            chp[chip].is_chip_mem_in_use= FALSE;
            for(int j=0;j<MAX_CPUS_PER_SRAD;j++){
                chp[chip].in_use_cpulist[j] = -1;
            }
            for(pi=0;pi<MAX_PAGE_SIZES;pi++){
                chp[chip].memory_details.pdata[pi].page_wise_usage_mem = 0;
                chp[chip].memory_details.pdata[pi].in_use_num_of_segments = 0;
            }

        }

        for( node = 0; node < g_data.sys_details.nodes; node++){
            if(cf->node[node].node_num == -1){
                sys_chip_count += g_data.sys_details.node[node].num_chips;
                continue;
            }
            for(chip = 0; chip < g_data.sys_details.node[node].num_chips; chip++){
                if(cf->node[node].chip[chip].chip_num == -1){/*if cpu filter tells not to consider this chip  then do not populate cpus*/
                    sys_chip_count++;
                    continue;
                }
                for(core = 0; core < MAX_CORES_PER_CHIP; core++){
                    if(cf->node[node].chip[chip].core[core].core_num == -1){
                        continue;
                    }
                    for(lcpu=0;lcpu < MAX_CPUS_PER_CORE;lcpu++){
                        if(cf->node[node].chip[chip].core[core].lprocs[lcpu] == -1){
                            continue;
                        }
                        chp[sys_chip_count].in_use_cpulist[chp[sys_chip_count].in_use_num_cpus++] = cf->node[node].chip[chip].core[core].lprocs[lcpu];
                        g_data.stanza_ptr->num_threads++;
                    }        
                }
                sys_chip_count++;
            }

        }

        sys_chip_count = 0;
        for( node = 0; node < get_num_of_nodes_in_sys(); node++){
            if(mf->node[node].node_num == -1){
                sys_chip_count += get_num_of_chips_in_node(node);
                continue;
            }
            for(chip = 0; chip < get_num_of_chips_in_node(node); chip++){
                if(mf->node[node].chip[chip].chip_num == -1){
                    sys_chip_count++;
                    continue;
                }
                chp[sys_chip_count].is_chip_mem_in_use= TRUE;
                page_wise_details = &g_data.sys_details.chip_mem_pool_data[sys_chip_count].memory_details.pdata[0];
                for(pi=0;pi<MAX_PAGE_SIZES;pi++){
                    mem_in_use = 0;
                    if(!(page_wise_details[pi].supported) || (mf->node[node].chip[chip].mem_details.pdata[pi].page_wise_usage_mem == 0)){
                        continue;
                    }
                    /*handling fabricbus case differently,as mem requirement is diffrent*/
                    if ( g_data.test_type == FABRICB){
                        continue;
                    }
                    page_wise_details[pi].page_wise_usage_mem = mf->node[node].chip[chip].mem_details.pdata[pi].page_wise_usage_mem;
                    for(seg=0;seg<page_wise_details[pi].num_of_segments;seg++){
                        /*reset the segment size field with original value,which might have got modified by previous stanza mem filter*/
                        page_wise_details[pi].page_wise_seg_data[seg].shm_size =  page_wise_details[pi].page_wise_seg_data[seg].original_shm_size;
                        /* If rule file has entry of segment size for a given page,if it is less than original alocated seg size then overwirte with new*/
                        if(g_data.stanza_ptr->seg_size[pi] != -1){
                            if(g_data.stanza_ptr->seg_size[pi] < page_wise_details[pi].page_wise_seg_data[seg].shm_size){
                                page_wise_details[pi].page_wise_seg_data[seg].shm_size = g_data.stanza_ptr->seg_size[pi];
                            }
                        }
                        if(page_wise_details[pi].page_wise_usage_mem < page_wise_details[pi].page_wise_seg_data[seg].shm_size){
                            page_wise_details[pi].page_wise_seg_data[seg].shm_size = page_wise_details[pi].page_wise_usage_mem;
                            page_wise_details[pi].page_wise_seg_data[seg].in_use = 0;
                            page_wise_details[pi].in_use_num_of_segments++;
                            break;
                        }
                        mem_in_use += page_wise_details[pi].page_wise_seg_data[seg].shm_size;
                        if(mem_in_use > page_wise_details[pi].page_wise_usage_mem)break;
                        page_wise_details[pi].page_wise_seg_data[seg].in_use = 0;
                        page_wise_details[pi].in_use_num_of_segments++;
                    }
                }            
                sys_chip_count++;
            }
        } 
        if ( g_data.test_type == FABRICB){
            for(chip =0;chip< g_data.sys_details.num_chip_mem_pools;chip++){
                for(pi=0;pi<MAX_PAGE_SIZES;pi++){
                    if(!(page_wise_details[pi].supported))continue;
                    rc = modify_fabb_shmsize_based_on_cpufilter(chip,pi);
                    if(rc != SUCCESS){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:modify_fabb_shmsize_based_on_cpufilter() failed "
                           "with rc = %d\n",__LINE__,__FUNCTION__,rc);
                        return rc;
                    }	
                }
            }
        }
    }else{
        int dma_mem_percent = g_data.stanza_ptr->mem_percent;
        int dma_cpu_percent = g_data.stanza_ptr->cpu_percent;
        if((g_data.stanza_ptr->operation == OPER_DMA) || (g_data.stanza_ptr->operation == OPER_RIM)){
            if(g_data.stanza_ptr->mem_percent == DEFAULT_MEM_PERCENT){
                dma_mem_percent = 5;
            }
            if(g_data.stanza_ptr->cpu_percent == DEFAULT_CPU_PERCENT){
                dma_cpu_percent = 10;
            }
        }
        sys_page_wise_details = &g_data.sys_details.memory_details.pdata[0];
        for(pi=0;pi<MAX_PAGE_SIZES;pi++){
            mem_in_use = 0;
            sys_page_wise_details[pi].page_wise_usage_mem = 0;
            sys_page_wise_details[pi].in_use_num_of_segments = 0;
            if(!(sys_page_wise_details[pi].supported) || (sys_page_wise_details[pi].free == 0)){
                continue;
            }
            if(pi < PAGE_INDEX_2M){
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].free * (g_data.gstanza.global_alloc_mem_percent/100.0);/*allocated memory*/
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].page_wise_usage_mem * (dma_mem_percent/100.0);
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].page_wise_usage_mem / sys_page_wise_details[pi].psize;
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].page_wise_usage_mem * sys_page_wise_details[pi].psize;
            }else{
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].free;
            }


            for(seg=0;seg<sys_page_wise_details[pi].num_of_segments;seg++){
                /*reset the segment size field with original value,which might have got modified by previous stanza mem filter*/
                sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size =  sys_page_wise_details[pi].page_wise_seg_data[seg].original_shm_size;
                /* If rule file has entry of segment size for a given page,if it is less than original alocated seg size then overwirte with new*/
                if(g_data.stanza_ptr->seg_size[pi] != -1){
                    if(g_data.stanza_ptr->seg_size[pi] < sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size){
                        sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size = g_data.stanza_ptr->seg_size[pi];
                    }
                }
                if(sys_page_wise_details[pi].page_wise_usage_mem < sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size){
                    sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size = sys_page_wise_details[pi].page_wise_usage_mem;
                    sys_page_wise_details[pi].in_use_num_of_segments++;
                    break;
                }
                mem_in_use += sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size;
                if(mem_in_use > sys_page_wise_details[pi].page_wise_usage_mem)break;
                sys_page_wise_details[pi].in_use_num_of_segments++;
            }
        }
        /*check for "num_threads" mentioned in rule, else check for "cpu_percent", other wise set it to g_num_threads(global stanza, deafault value - total lcpus)*/
        if(g_data.stanza_ptr->num_threads == -1){
            if(g_data.stanza_ptr->cpu_percent < 100){
                g_data.stanza_ptr->num_threads = (g_data.sys_details.tot_cpus * (dma_cpu_percent/100.0));
                if (g_data.stanza_ptr->num_threads == 0){
                    g_data.stanza_ptr->num_threads = 1;
                }
            }else{
                g_data.stanza_ptr->num_threads = g_data.gstanza.global_num_threads;
            }
        }
    }
    return SUCCESS;
}

int fill_thread_context(){
    int i,chip,rc,start_thread_index=0;
    struct chip_mem_pool_info *chp = &g_data.sys_details.chip_mem_pool_data[0];
    int mem_chip_used[MAX_CHIPS] = {[0 ...(MAX_CHIPS-1)]=-1};

    rc = allocate_mem_for_threads();
    if(rc != SUCCESS){
        return(FAILURE);
    }
    if(g_data.gstanza.global_disable_filters == 1){
        rc = fill_thread_context_with_filters_disabled();
        if(rc != SUCCESS){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_thread_context_with_filters_disabled() failed with rc = %d\n",
                __LINE__,__FUNCTION__,rc);
            return (FAILURE);
        }
        return (SUCCESS);		
    }
    for(chip = 0; chip < MAX_CHIPS; chip++){
        if(chp[chip].in_use_num_cpus <= 0)continue;

        switch(mem_g.affinity){
            case LOCAL:
                if((chp[chip].is_chip_mem_in_use != TRUE) || (chp[chip].in_use_num_cpus <= 0)){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: Specified filter in rule file %s, rule_id:%s  resulted into  _NO_ cpu or mem usage for chip %d ,"
                        "please correct the filters or change affinity=LOCAL setting\n",__LINE__,__FUNCTION__,g_data.rules_file_name,g_data.stanza_ptr->rule_id,chip);
                    return (FAILURE);
                }    
                rc = fill_affinity_based_thread_structure(&chp[chip],&chp[chip],start_thread_index);
                if(rc != SUCCESS){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_affinity_based_thread_structure() returned rc = %d\n",__LINE__,__FUNCTION__,rc);
                    return (FAILURE);
                }
                break;
            case REMOTE_CHIP:
                {
                    long int mem_chip=chip;
                    for(;;){
                        mem_chip = (mem_chip + 1)%g_data.sys_details.num_chip_mem_pools;
                        if(mem_chip == chip){
                            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s: (affinity=REMOTE_CHIP)for cpu chip %d,there is no remote memory "
                                "chip is provided in memory filters for rule=%s,rule file=%s\n,over writing to affinity = local, please "
                                "check system config or modify filters to run remote_chip test.\n"
                                ,__LINE__,__FUNCTION__,chip,g_data.stanza_ptr->rule_id,g_data.rules_file_name);
                            mem_g.affinity = LOCAL;
                            break;
                        }
                        if(mem_chip_used[mem_chip] == 1){
                            continue;
                        }
                        if(chp[mem_chip].is_chip_mem_in_use == TRUE) {
                            break;
                        } 
                    }
                    debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"affity(remote_chip):cpu chip = %d mem chip = %d th=%d\n",chip,mem_chip,start_thread_index);
                    mem_chip_used[mem_chip] = 1;
                    rc = fill_affinity_based_thread_structure(&chp[chip],&chp[mem_chip],start_thread_index);
                    if(rc != SUCCESS){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_affinity_based_thread_structure() returned rc = %d\n",__LINE__,__FUNCTION__,rc);
                        return (FAILURE);
                    }
                }
                break;

            case INTER_NODE:
            case INTRA_NODE:
                if ( g_data.test_type != FABRICB){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: affinity setting to 'INTER_NODE' or 'INTRA_NODE' is supported only for HTX device names fabn,\nfabc "
                    "test cases respectively,modify rule file:%s and rerun\n",__LINE__,__FUNCTION__,g_data.rules_file_name);
                    return FAILURE;
                }
                rc = fill_fabb_thread_structure(&chp[chip],start_thread_index);
                if(rc != SUCCESS){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_fabb_internode_thread_structure() returned rc = %d\n",__LINE__,__FUNCTION__,rc);
                    return rc;
                }
                break;
            case FLOATING:
                g_data.stanza_ptr->disable_cpu_bind = 1;
                rc = fill_affinity_based_thread_structure(&chp[chip],&chp[chip],start_thread_index);
                if(rc != SUCCESS){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_affinity_based_thread_structure() returned rc = %d\n",__LINE__,__FUNCTION__,rc);
                    return (FAILURE);
                }
                break;
            default:
                displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s:Inavlid affinity value provided in rule file %s,rule_id:%s, over writing it to 'LOCAL' and continuing..\n",
                    __LINE__,__FUNCTION__,g_data.rules_file_name,g_data.stanza_ptr->rule_id);            
                mem_g.affinity = LOCAL;
                if((chp[chip].is_chip_mem_in_use != TRUE) || (chp[chip].in_use_num_cpus <= 0)){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: Specified filter in rule file %s, rule_id:%s  resulted into  _NO_ cpu or mem usage for chip %d ,"
                        "please correct the filters or change affinity=LOCAL setting\n",__LINE__,__FUNCTION__,g_data.rules_file_name,g_data.stanza_ptr->rule_id,chip);
                    return (FAILURE);
                }    
                rc = fill_affinity_based_thread_structure(&chp[chip],&chp[chip],start_thread_index);
                if(rc != SUCCESS){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_affinity_based_thread_structure() returned rc = %d\n",__LINE__,__FUNCTION__,rc);
                    return (FAILURE);
                }
        }

        start_thread_index += chp[chip].in_use_num_cpus;
        
    }    


    return(SUCCESS);
}

int fill_affinity_based_thread_structure(struct chip_mem_pool_info *cpu_chip, struct chip_mem_pool_info *mem_chip,int start_thread_index){

    int thread_num,pi;
    int rem_segs[MAX_PAGE_SIZES],avg_segs_per_thread[MAX_PAGE_SIZES],seg_count_track[MAX_PAGE_SIZES];
    int num_threads_in_this_chip = cpu_chip->in_use_num_cpus;
    int num_segs_per_thread[num_threads_in_this_chip][MAX_PAGE_SIZES];
    struct thread_data* th = &g_data.thread_ptr[start_thread_index];

    for(pi=0;pi<MAX_PAGE_SIZES;pi++){
        seg_count_track[pi] = 0;
        if(mem_chip->memory_details.pdata[pi].in_use_num_of_segments > 0){
            if(mem_chip->memory_details.pdata[pi].in_use_num_of_segments >= num_threads_in_this_chip){
                avg_segs_per_thread[pi] = (mem_chip->memory_details.pdata[pi].in_use_num_of_segments/num_threads_in_this_chip);
            }else {
                avg_segs_per_thread[pi] = 0;
            }
            rem_segs[pi] = (mem_chip->memory_details.pdata[pi].in_use_num_of_segments % num_threads_in_this_chip);
            
    
        }
        
    }
    for(thread_num=0;thread_num < num_threads_in_this_chip; thread_num++){
        struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[thread_num+start_thread_index]);
        th[thread_num].testcase_thread_details = (void *)local_ptr;
        th[thread_num].thread_num               = thread_num+start_thread_index;
        th[thread_num].bind_proc                = cpu_chip->in_use_cpulist[thread_num];
        th[thread_num].thread_type              = MEM;
        local_ptr->num_segs = 0;
        for(pi=0;pi<MAX_PAGE_SIZES;pi++){
            if(mem_chip->memory_details.pdata[pi].in_use_num_of_segments > 0){
                num_segs_per_thread[thread_num][pi]=avg_segs_per_thread[pi];
                local_ptr->num_segs += avg_segs_per_thread[pi];
                if((rem_segs[pi]--) > 0){
                    local_ptr->num_segs++;
                    num_segs_per_thread[thread_num][pi]++;               
                }
            }

        }
        if(local_ptr->num_segs != 0){
            local_ptr->seg_details = (struct page_wise_seg_info*) malloc(local_ptr->num_segs * sizeof(struct page_wise_seg_info));
            if(local_ptr->seg_details == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed for memory %d bytes with errno(%d)%s, num_segs for thread %d = %d\n",
                    __LINE__,__FUNCTION__,(local_ptr->num_segs * sizeof(struct page_wise_seg_info)),errno,strerror(errno),th[thread_num].thread_num,local_ptr->num_segs);
                return(FAILURE);
            }
        }

    }
    for(thread_num=0;thread_num < num_threads_in_this_chip; thread_num++){
        /*th[thread_num].testcase_thread_details  = (struct mem_exer_thread_info*) &mem_g.mem_th_data[thread_num+start_thread_index];
        struct mem_exer_thread_info *local_ptr = (struct mem_exer_thread_info*)th[thread_num].testcase_thread_details;*/
        struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[thread_num+start_thread_index]);
        th[thread_num].testcase_thread_details = (void *)local_ptr;
        debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"thread:%d cpu to bind=%d total segments: %d \n",th[thread_num].thread_num,th[thread_num].bind_proc,local_ptr->num_segs);
        int j = 0;
        if(local_ptr->num_segs == 0)continue;
        for(pi=0;pi<MAX_PAGE_SIZES;pi++){
            if((mem_chip->memory_details.pdata[pi].in_use_num_of_segments > 0) && (seg_count_track[pi] < mem_chip->memory_details.pdata[pi].in_use_num_of_segments)){
                debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"   Page:%s  num_segs=%d\n",page_size_name[pi],num_segs_per_thread[thread_num][pi]);
                for(int seg=0;(seg < num_segs_per_thread[thread_num][pi]) && (j < local_ptr->num_segs);seg++,j++){
                    if(mem_chip->memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].owning_thread != -1)continue;
                    local_ptr->seg_details[j].owning_thread = th[thread_num].thread_num;
                    local_ptr->seg_details[j].page_size_index   = pi;
                    local_ptr->seg_details[j].shmid             = mem_chip->memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track[pi]].shmid;
                    local_ptr->seg_details[j].shm_size          = mem_chip->memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track[pi]].shm_size;
                    local_ptr->seg_details[j].shm_mem_ptr       = mem_chip->memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track[pi]].shm_mem_ptr;
                    local_ptr->seg_details[j].seg_num           = mem_chip->memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track[pi]].seg_num;
                    local_ptr->seg_details[j].mem_chip_num      = mem_chip->memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track[pi]].mem_chip_num;
                    local_ptr->seg_details[j].cpu_chip_num      = cpu_chip->chip_id;

                    /*update thread number in global segment details data structure*/
                    mem_chip->memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].owning_thread =th[thread_num].thread_num;
                    mem_chip->memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track[pi]].cpu_chip_num = cpu_chip->chip_id;
                    mem_chip->memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track[pi]].in_use=1;

                    debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"      seg_details[%d]=%d(count/th =%d),mem_chip=%d, owning_thread=%d,shmid=%lu,shm_size=%lu,EA=%llx\n",
                        (seg+seg_count_track[pi]),local_ptr->seg_details[j].seg_num,j,local_ptr->seg_details[j].mem_chip_num,local_ptr->seg_details[j].owning_thread,
                        local_ptr->seg_details[j].shmid,local_ptr->seg_details[j].shm_size,local_ptr->seg_details[j].shm_mem_ptr);
                }
                seg_count_track[pi] += num_segs_per_thread[thread_num][pi];
            }
        }
    }
    return (SUCCESS);
}

int fill_thread_context_with_filters_disabled(){

    int thread_num,pi;
    int rem_segs[MAX_PAGE_SIZES],avg_segs_per_thread[MAX_PAGE_SIZES],seg_count_track[MAX_PAGE_SIZES];
    int total_threads = g_data.stanza_ptr->num_threads;
    int num_segs_per_thread[total_threads][MAX_PAGE_SIZES];
    struct thread_data* th = &g_data.thread_ptr[0];

    for(pi=0;pi<MAX_PAGE_SIZES;pi++){
        seg_count_track[pi] = 0;
        if(g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments > 0){
            if(g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments >= total_threads){
                avg_segs_per_thread[pi] = (g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments / total_threads);
            }else{
                avg_segs_per_thread[pi] = 0;
            }
            rem_segs[pi] = (g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments % total_threads);
        }/*else{
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:in_use_num_of_segments=%lu, something went wrong in segment calculation, \n",
                __LINE__,__FUNCTION__,g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments);
            return(FAILURE);
        }*/		
    }

    for(thread_num=0;thread_num < total_threads;thread_num++){
        struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[thread_num]);
        th[thread_num].testcase_thread_details = (void *)local_ptr;
        th[thread_num].thread_num               = thread_num;
        th[thread_num].bind_proc                = thread_num;
        th[thread_num].thread_type              = MEM;
        local_ptr->num_segs = 0;
        for(pi=0;pi<MAX_PAGE_SIZES;pi++){
            if(g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments > 0){
                num_segs_per_thread[thread_num][pi]=avg_segs_per_thread[pi];
                local_ptr->num_segs += avg_segs_per_thread[pi];
                if((rem_segs[pi]--) > 0){
                    local_ptr->num_segs++;
                    num_segs_per_thread[thread_num][pi]++;
                }
            }
        }
        if(local_ptr->num_segs != 0){
            local_ptr->seg_details = (struct page_wise_seg_info*) malloc(local_ptr->num_segs * sizeof(struct page_wise_seg_info));
            if(local_ptr->seg_details == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed for memory %d bytes with errno(%d)%s, num_segs for thread %d = %d\n",
                    __LINE__,__FUNCTION__,(local_ptr->num_segs * sizeof(struct page_wise_seg_info)),errno,strerror(errno),th[thread_num].thread_num,local_ptr->num_segs);
                return(FAILURE);
            }
        }
    }

    for(thread_num=0;thread_num < total_threads;thread_num++){
        struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[thread_num]);
        th[thread_num].testcase_thread_details = (void *)local_ptr;
        debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"thread:%d cpu to bind=%d total segments: %d \n",th[thread_num].thread_num,th[thread_num].bind_proc,local_ptr->num_segs);
        int j = 0;
        if(local_ptr->num_segs == 0)continue;
        for(pi=0;pi<MAX_PAGE_SIZES;pi++){
            if((g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments > 0) && (seg_count_track[pi] < g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments)){
                debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"   Page:%s  num_segs=%d\n",page_size_name[pi],num_segs_per_thread[thread_num][pi]);
                for(int seg=0;(seg < num_segs_per_thread[thread_num][pi]) && (j < local_ptr->num_segs);seg++,j++){
                    if(g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].owning_thread != -1)continue;
                    local_ptr->seg_details[j].owning_thread = th[thread_num].thread_num;
                    local_ptr->seg_details[j].page_size_index   = pi;
                    local_ptr->seg_details[j].shmid             = g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].shmid;
                    local_ptr->seg_details[j].shm_size			= g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].shm_size;
                    local_ptr->seg_details[j].shm_mem_ptr       = g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].shm_mem_ptr;
                    local_ptr->seg_details[j].seg_num			= g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].seg_num;
                    local_ptr->seg_details[j].mem_chip_num      = -1;/* In case of affinity=no/disable_filter mode/shared proc, its not possible to update chip nums*/
                    local_ptr->seg_details[j].cpu_chip_num      = -1;				
                
                    /*update thread number in global segment details data structure*/
                    g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].owning_thread =th[thread_num].thread_num;
                    g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].cpu_chip_num = -1;
                    g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg +seg_count_track [pi]].in_use = 1;

                    debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"      seg_details[%d]=%d(count/th =%d),mem_chip=%d, owning_thread=%d,shmid=%lu,shm_size=%lu,EA=%llx\n",
                        (seg+seg_count_track[pi]),local_ptr->seg_details[j].seg_num,j,local_ptr->seg_details[j].mem_chip_num,local_ptr->seg_details[j].owning_thread,
                        local_ptr->seg_details[j].shmid,local_ptr->seg_details[j].shm_size,local_ptr->seg_details[j].shm_mem_ptr);
                }
                seg_count_track[pi] += num_segs_per_thread[thread_num][pi];
            }
        }
    }
    return (SUCCESS);
}

int create_and_run_thread_operation(){
    int rc,thread_num,tnum,thread_track=0;
    struct thread_data* th = &g_data.thread_ptr[0];
    void *tresult;
    for(thread_num=0;thread_num < g_data.stanza_ptr->num_threads;thread_num++){
        struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[thread_num]);
        th[thread_num].testcase_thread_details = (void *)local_ptr;
        tnum = thread_num;
        th[thread_num].tid= -1;
        th[thread_num].kernel_tid = -1;
        if(local_ptr->num_segs == 0) continue;
        /*displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Creating thread %d for cpu num : %d for exercising total segs = %d\n",thread_num,th[tnum].bind_proc,local_ptr->num_segs);*/

        rc = pthread_create((pthread_t *)&th[tnum].tid,NULL,mem_thread_function,&th[tnum]);
        if(rc != 0){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"pthread_create failed with rc = %d (errno %d):(%s) for thread_num = %d\n",rc,errno,strerror(errno),tnum);
            return(FAILURE);
        }
        thread_track++;
    }
    if(thread_track != g_data.stanza_ptr->num_threads){
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Actual threads created %d (mem filter resulted into less number of segments),"
            "where as total threads to be created as per cpu filters=%d \n", thread_track,g_data.stanza_ptr->num_threads);
    }
    thread_track =0;
    for(thread_num=0;thread_num < g_data.stanza_ptr->num_threads;thread_num++){
        struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[thread_num]);
        th[thread_num].testcase_thread_details = (void *)local_ptr;
        tnum = thread_num;
        if(local_ptr->num_segs == 0) {
            th[thread_num].thread_num   = -1;
            th[thread_num].bind_proc    = -1;
            th[thread_num].tid          = -1;
            th[thread_num].kernel_tid   = -1;
            continue;
        }
        rc = pthread_join(th[tnum].tid,&tresult);
        if(rc != 0){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"pthread_join with rc = %d (errno %d):(%s) for thread_num = %d \n",rc,errno,strerror(errno),thread_num);
            return(FAILURE);
        }   
        thread_track++;
        th[thread_num].thread_num   = -1;
        th[thread_num].tid          = -1; 
        th[thread_num].kernel_tid   = -1;
        th[thread_num].bind_proc    = -1;
    }
    if(thread_track != g_data.stanza_ptr->num_threads){
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Total threads joined %d \n",thread_track);
    }

    rc = reset_segment_owners();
    return (SUCCESS);
}



void* mem_thread_function(void *tn){
    int rc;
    struct thread_data* th = (struct thread_data*)tn;
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[th->thread_num]);
    th->testcase_thread_details = (void *)local_ptr;
    debug(HTX_HE_INFO,DBG_IMP_PRINT,"Thread(%d):Inside mem_thread_function binding to cpu %d,segs=%d\n",th->thread_num,th->bind_proc,local_ptr->num_segs);

    if(!g_data.stanza_ptr->disable_cpu_bind){
        /*replace below call with bind_to_cpu,common betweeen AIX/LINUX*/
    #ifndef __HTX_LINUX__
        th->kernel_tid = thread_self();
        rc = bindprocessor(BINDTHREAD,th->kernel_tid,th->bind_proc);
    #else
        rc = htx_bind_thread(th->bind_proc,-1);
    #endif
        if(rc < 0){
        #ifdef __HTX_LINUX__
            if (rc == -2 || rc == -1) {
                displaym(HTX_HE_INFO,DBG_IMP_PRINT,"(th:%d)lcpu : %d has been hot removed\n",th->thread_num,th->bind_proc);
            }else{
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT," bind to cpu %d by thread %d is failed with rc = %d,err:%d(%s)\n",
                    th->bind_proc,th->thread_num,rc,errno,strerror(errno));
                release_thread_resources(th->thread_num);
                pthread_exit((void *)1);
            }
        #else
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT," bind to cpu %d by thread %d is failed with rc = %d,err:%d(%s)\n",
                th->bind_proc,th->thread_num,rc,errno,strerror(errno));
            release_thread_resources(th->thread_num);
            pthread_exit((void *)1);
        #endif
        }
        
    }

    switch(g_data.stanza_ptr->operation){
        case OPER_MEM:
            rc = do_mem_operation(th->thread_num);
            break;

        case OPER_RIM:
            rc = do_rim_operation(th->thread_num);
            break;

        case OPER_DMA:
            if(!mem_g.dma_flag){
                rc = do_dma_operation(th->thread_num);
            }
            break;

        case OPER_STRIDE:
            rc = do_stride_operation(th->thread_num);
            break;

        default:
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:The stanza's (%s) oper(%s) field"
                "is invalid.Please check the oper field \n",__LINE__,__FUNCTION__,g_data.stanza_ptr->rule_id,g_data.stanza_ptr->oper);
    }

    release_thread_resources(th->thread_num);
    pthread_exit((void *)0);
}

int do_mem_operation(int t){
    int  num_oper,miscompare_count,seg,pi=0,rc=0,misc_detected = 0; /* pi is pattern index */
    static int main_misc_count=0;
    struct segment_detail sd;
    struct thread_data* th = &g_data.thread_ptr[t];
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t]);
    th->testcase_thread_details = (void *)local_ptr;
    int nw=0,nr=0,nc=0,trap_flag=0,mem_access_type_index;
    unsigned long seed = g_data.stanza_ptr->seed;
    unsigned long backup_seed = seed;
    
    debug(HTX_HE_INFO,DBG_IMP_PRINT,"Starting load_store_buffers with num_oper = %d for thread[%d]= %d to exercise tot segs=%d cpu = %d \n"
                    ,g_data.stanza_ptr->num_oper,t,th->thread_num,local_ptr->num_segs,th->bind_proc);
    if( (g_data.stanza_ptr->misc_crash_flag) && (g_data.kdb_level) )
        trap_flag = 1;    /* Trap is set */
    else
        trap_flag = 0;    /* Trap is not set */

    if(g_data.stanza_ptr->attn_flag)
        trap_flag = 2;    /* attn is set */


    for (num_oper =0;num_oper < g_data.stanza_ptr->num_oper; num_oper++){
        if(g_data.mem_DR_flag){
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d is exiting for current stanza on mem DR operation \n",t);
            return(-1);
        }
        for (seg=0;seg<local_ptr->num_segs;){
            sd.page_index=local_ptr->seg_details[seg].page_size_index;
            sd.seg_num=local_ptr->seg_details[seg].seg_num;
            sd.seg_size=local_ptr->seg_details[seg].shm_size;
            sd.owning_thread = local_ptr->seg_details[seg].owning_thread;
            sd.cpu_owning_chip = local_ptr->seg_details[seg].cpu_chip_num;
            sd.mem_owning_chip = local_ptr->seg_details[seg].mem_chip_num;;
            sd.affinity_index  = g_data.stanza_ptr->affinity;
            sd.thread_ptr_addr = (unsigned long)th;
            sd.global_ptr_addr = (unsigned long)&g_data;
            sd.sub_seg_num=0;
            sd.sub_seg_size=0;
            backup_seed = seed;
            if( g_data.stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                sd.width = LS_DWORD;
            } else {
                sd.width = g_data.stanza_ptr->width;
            }
            nw = g_data.stanza_ptr->num_writes;
            nr = g_data.stanza_ptr->num_reads;
            nc = g_data.stanza_ptr->num_compares;
            mem_access_type_index = get_mem_access_operation(pi);
            while(nw>0 || nr>0 || nc>0){
                if(nw>0){
                    seed = backup_seed;
                    rc = (*operation_fun_typ[g_data.stanza_ptr->pattern_type[pi]][WRITE][mem_access_type_index])(
                                                            (sd.seg_size/sd.width),\
                                                            local_ptr->seg_details[seg].shm_mem_ptr,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            trap_flag,\
                                                            &sd,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            &seed);
                    nw--;
                }
                    debug(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d  complted %d Write  operations, for seg = %d(page_wise seg #%d),page=%d,pat=%d\n",
                        t,nw,seg,sd.seg_num,page_size_name[sd.page_index],pi);
                if (g_data.exit_flag == SET) {
                    goto update_exit;
                } /* endif */
            
                if(nr>0){ 
                    rc = (*operation_fun_typ[g_data.stanza_ptr->pattern_type[pi]][READ][mem_access_type_index])(
                                                            (sd.seg_size/sd.width),\
                                                            local_ptr->seg_details[seg].shm_mem_ptr,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            trap_flag,\
                                                            &sd,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            &seed);

                    /*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d  complted %d Read operations\n",t,nr);*/
                    nr--;
                }
                if (g_data.exit_flag == SET) {
                    goto update_exit;
                } /* endif */

                if(nc>0){
                    rc = (*operation_fun_typ[g_data.stanza_ptr->pattern_type[pi]][COMPARE][mem_access_type_index])(
                                                            (sd.seg_size/sd.width),\
                                                            local_ptr->seg_details[seg].shm_mem_ptr,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            trap_flag,\
                                                            &sd,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            &backup_seed);
                    if(rc != SUCCESS){
                        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:compare_data returned %d for compare count %d\n",
                            __LINE__,__FUNCTION__,rc,nc);
                        break;
            
                    }
                    debug(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d  complted %d Read_Cmpare operations\n",t,nc);
                    nc--; 
                }
                if (g_data.exit_flag == SET && rc == SUCCESS) {
                    goto update_exit;
                } /* endif */

            }
            if(rc != SUCCESS){/* If there is any miscompare, rc will have the offset */
                miscompare_count = dump_miscompared_buffers(t,rc,seg,main_misc_count,&backup_seed,num_oper,trap_flag,pi,&sd);
                STATS_VAR_INC(bad_others, 1);
                main_misc_count++;
                misc_detected++ ;
                STATS_VAR_INC(bad_others, 1)
                /*STATS_HTX_UPDATE(UPDATE)*/
            }
            if (g_data.exit_flag == SET) {
                goto update_exit;
            } /* endif */
            /* hxfupdate(UPDATE,&stats);*/

            switch(g_data.stanza_ptr->switch_pat) {
                 case SW_PAT_ALL:               /* SWITCH_PAT_PER_SEG = ALL */
                    /* Stay on this segment until all patterns are tested.
                    Advance segment index once for every num_patterns */
                    pi++;
                    if (pi >= g_data.stanza_ptr->num_patterns) {
                        pi = 0; /* Go back to the 1st pattern */
                        seg++;        /* Move to the new Seg */
                    }
                     break;
                case SW_PAT_ON:                /* SWITCH_PAT_PER_SEG = YES */
                    /* Go back to the 1st pattern */
                    pi++;
                    if (pi >= g_data.stanza_ptr->num_patterns) {
                        pi = 0;
                    }
                    /* Fall through */
                case SW_PAT_OFF:                /* SWITCH_PAT_PER_SEG = NO */
                    /* Fall through */
                    default:
                    seg++;        /* Increment Seg idx: case 1,0 and default */
            } /* end of switch */

        update_exit:
            STATS_VAR_INC(bytes_writ,((g_data.stanza_ptr->num_writes - nw) * sd.seg_size))
            STATS_VAR_INC(bytes_read,((g_data.stanza_ptr->num_reads - nr) * sd.seg_size))
            STATS_VAR_INC(bytes_read,((g_data.stanza_ptr->num_compares - nc - misc_detected) * sd.seg_size))
            /*STATS_HTX_UPDATE(UPDATE)*/
            misc_detected = 0;
            if (g_data.exit_flag == SET) {
                return -1;
            }
        }/*seg loop*/

    }/* num_oper loop*/
    return (SUCCESS);
}

int do_rim_operation(int t){
    int  k,num_oper,miscompare_count,seg,pi=0,rc=0,misc_detected = 0; /* pi is pattern index */
    static int main_misc_count=0;
    struct segment_detail sd;
    struct thread_data* th = &g_data.thread_ptr[t];
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t]);
    th->testcase_thread_details = (void *)local_ptr;
    int nw=0,trap_flag=0,mem_access_type_index;
    unsigned long seed = g_data.stanza_ptr->seed,seg_size;
    unsigned long backup_seed = seed;

    debug(HTX_HE_INFO,DBG_IMP_PRINT,"Starting load_store_buffers with num_oper = %d for thread[%d]= %d to exercise tot segs=%d cpu = %d \n"
                    ,g_data.stanza_ptr->num_oper,t,th->thread_num,local_ptr->num_segs,th->bind_proc);
    if( (g_data.stanza_ptr->misc_crash_flag) && (g_data.kdb_level) )
        trap_flag = 1;    /* Trap is set */
    else
        trap_flag = 0;    /* Trap is not set */

    if(g_data.stanza_ptr->attn_flag)
        trap_flag = 2;    /* attn is set */


    for (num_oper =0;num_oper < g_data.stanza_ptr->num_oper; num_oper++){
        if(g_data.mem_DR_flag){
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d is exiting for current stanza on mem DR operation \n",t);
            return(-1);
        }
        for (seg=0;seg<local_ptr->num_segs;){
            sd.page_index=local_ptr->seg_details[seg].page_size_index;
            sd.seg_num=local_ptr->seg_details[seg].seg_num;
            sd.seg_size=local_ptr->seg_details[seg].shm_size;
            sd.owning_thread = local_ptr->seg_details[seg].owning_thread;
            sd.cpu_owning_chip = local_ptr->seg_details[seg].cpu_chip_num;
            sd.mem_owning_chip = local_ptr->seg_details[seg].mem_chip_num;;
            sd.affinity_index  = g_data.stanza_ptr->affinity;
            sd.thread_ptr_addr = (unsigned long)th;
            sd.global_ptr_addr = (unsigned long)&g_data;
            sd.sub_seg_num=-1;
            sd.sub_seg_size=DEF_SEG_SZ_4K;
            backup_seed = seed;
            if( g_data.stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                sd.width = LS_DWORD;
            } else {
                sd.width = g_data.stanza_ptr->width;
            }
            /* 16G page segment is logically operated in 256MB chunks
             *  to test for RIM operation only*/
            if ( sd.page_index  == PAGE_INDEX_16G ) {
                seg_size = sd.sub_seg_size;
            } else {
                seg_size = local_ptr->seg_details[seg].shm_size;
            }
            nw = g_data.stanza_ptr->num_writes;
            mem_access_type_index = get_mem_access_operation(pi);
            while(nw>0){
            seed = backup_seed;
                for(k=0;(k*seg_size < sd.seg_size);k++){/*k operates on next sub segments */
                    sd.sub_seg_num=k;
                    rc = (*operation_fun_typ[g_data.stanza_ptr->pattern_type[pi]][RIM][mem_access_type_index])
                                                            ((sd.seg_size/sd.width),\
                                                            local_ptr->seg_details[seg].shm_mem_ptr+k*seg_size,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            trap_flag,\
                                                            &sd,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            &seed);
    
                    debug(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d  complted %d RIM operations(write,read,flush,readcompare), for seg = %d(page_wise seg #%d),page=%d,pat=%d\n",
                        t,nw,seg,sd.seg_num,page_size_name[sd.page_index],pi);
                    if (g_data.exit_flag == SET) {
                        goto update_exit;
                    }    /* endif */
                    nw--;
                }
            }
            if(rc != SUCCESS){/* If there is any miscompare, rc will have the offset */
                miscompare_count = dump_miscompared_buffers(t,rc,seg,main_misc_count,&seed,num_oper,trap_flag,pi,&sd);
                STATS_VAR_INC(bad_others, 1);
                main_misc_count++;
                misc_detected++ ;
                STATS_VAR_INC(bad_others, 1)
                /*STATS_HTX_UPDATE(UPDATE)*/
            }
            if (g_data.exit_flag == SET) {
                goto update_exit;
            } /* endif */
            /* hxfupdate(UPDATE,&stats);*/

            switch(g_data.stanza_ptr->switch_pat) {
                 case SW_PAT_ALL:               /* SWITCH_PAT_PER_SEG = ALL */
                    /* Stay on this segment until all patterns are tested.
                    Advance segment index once for every num_patterns */
                    pi++;
                    if (pi >= g_data.stanza_ptr->num_patterns) {
                        pi = 0; /* Go back to the 1st pattern */
                        seg++;        /* Move to the new Seg */
                    }
                     break;
                case SW_PAT_ON:                /* SWITCH_PAT_PER_SEG = YES */
                    /* Go back to the 1st pattern */
                    pi++;
                    if (pi >= g_data.stanza_ptr->num_patterns) {
                        pi = 0;
                    }
                    /* Fall through */
                case SW_PAT_OFF:                /* SWITCH_PAT_PER_SEG = NO */
                    /* Fall through */
                    default:
                    seg++;        /* Increment Seg idx: case 1,0 and default */
            } /* end of switch */

        update_exit:
            STATS_VAR_INC(bytes_writ,((g_data.stanza_ptr->num_writes - nw - misc_detected) * WRC * sd.seg_size))
            /*STATS_HTX_UPDATE(UPDATE)*/
            misc_detected = 0;
            if (g_data.exit_flag == SET) {
                return -1;
            }
        }/*seg loop*/

    }/* num_oper loop*/
    return SUCCESS;
}

int do_dma_operation(int t){
    int  num_oper,miscompare_count,seg,pi=0,rc=0,misc_detected = 0; /* pi is pattern index */
    int  chars_read,fildes,file_size=0,mode_flag;
    static int main_misc_count=0;
    struct segment_detail sd;
    struct thread_data* th = &g_data.thread_ptr[t];
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t]);
    th->testcase_thread_details = (void *)local_ptr;
    int nw=0,nr=0,nc=0,trap_flag=0,mem_access_type_index;
    unsigned long seed = g_data.stanza_ptr->seed;
    struct stat fstat;
    char *ptr;
    char pattern_nm[256];
    unsigned long pat_size;

    char* pat_tmp_ptr = getenv("HTXPATTERNS");
    if(pat_tmp_ptr == NULL){
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s(): env variable HTXPATTERNS is not set,pat_tmp_ptr=%p"
            ", thus exiting..\n",__LINE__,__FUNCTION__,pat_tmp_ptr);
        return(FAILURE);
    }else{
        strcpy(pattern_nm,pat_tmp_ptr);
    }
    strcat(pattern_nm,g_data.stanza_ptr->pattern_name[0]);
    pat_size = g_data.stanza_ptr->pattern_size[0];

    debug(HTX_HE_INFO,DBG_IMP_PRINT,"\n(T=%d)Entered do_dma_operation() pattern name=%s,nam=%s , size=%u \n",\
        t,pattern_nm,g_data.stanza_ptr->pattern_name[0],pat_size);

    if( (g_data.stanza_ptr->misc_crash_flag) && (g_data.kdb_level) )
        trap_flag = 1;    /* Trap is set */
    else
        trap_flag = 0;    /* Trap is not set */

    if(g_data.stanza_ptr->attn_flag)
        trap_flag = 2;    /* attn is set */

    mode_flag = S_IWUSR | S_IWGRP | S_IWOTH;
    /* Opening the file in Direct I/O mode ( or what can be called RAW mode ) */
    #ifndef __HTX_BML__
    if ((fildes = open(pattern_nm, O_DIRECT | O_RDONLY , mode_flag)) == -1) {
    #else
    if ((fildes = open(pattern_nm, O_RDONLY , mode_flag)) == -1) {
    #endif
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s():thread %d, open() failed for file %s,errno:%d(%s)\n",\
            __LINE__,__FUNCTION__,t,pattern_nm,errno,strerror(errno));
        return FAILURE;
    }
    for (num_oper =0;num_oper < g_data.stanza_ptr->num_oper; num_oper++){
        if(stat (pattern_nm, &fstat) == -1){
             displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s():thread %d,Error(%s(%d)) while getting the pattern %s"
                " file stats",__LINE__,__FUNCTION__,t,strerror(errno),errno,pattern_nm);
        }else{
            file_size=fstat.st_size;
        }
        if (pat_size > file_size ) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s():thread %d,pat_size(%ld) cannot be greater than file's (%s) size %d",
                __LINE__,__FUNCTION__,t,pat_size,pattern_nm,file_size);
            return FAILURE;      
        }
        for (seg=0;seg<local_ptr->num_segs;seg++){
            nw = g_data.stanza_ptr->num_writes;
            nr = g_data.stanza_ptr->num_reads;
            nc = g_data.stanza_ptr->num_compares;
            mem_access_type_index = get_mem_access_operation(pi);
            long seg_size = local_ptr->seg_details[seg].shm_size;
            unsigned long total_bytes_read = 0; /* Temporary variable */
            unsigned int read_size = 0;
            sd.page_index=local_ptr->seg_details[seg].page_size_index;
            sd.seg_num=local_ptr->seg_details[seg].seg_num;
            sd.seg_size=local_ptr->seg_details[seg].shm_size;
            sd.owning_thread = local_ptr->seg_details[seg].owning_thread;
            sd.cpu_owning_chip = local_ptr->seg_details[seg].cpu_chip_num;
            sd.mem_owning_chip = local_ptr->seg_details[seg].mem_chip_num;;
            sd.affinity_index  = g_data.stanza_ptr->affinity;
            sd.thread_ptr_addr = (unsigned long)th;
            sd.global_ptr_addr = (unsigned long)&g_data;
            sd.sub_seg_num=0;
            sd.sub_seg_size=0;            
            sd.width = g_data.stanza_ptr->width;
            while(nw>0 || nr>0 || nc>0){
                if(nw>0){
                    seg_size = sd.seg_size;
                    total_bytes_read = 0; /* Temporary variable */
                    ptr = local_ptr->seg_details[seg].shm_mem_ptr;
                    while ( seg_size > 0 ) {
                        /* Rewind file before each read(write into memory) */
                        if(total_bytes_read% pat_size == 0 ) {
                            rc = lseek(fildes,0,0);
                            if (rc == -1) {
                                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s():thread %d,lseek error(%s(%d))on file %d\n",
                                    __LINE__,__FUNCTION__,t,strerror(errno),errno,pattern_nm);
                                return(FAILURE);
                            }
                        }    
                        if ( (seg_size/pat_size) == 0) {
                            read_size = seg_size%pat_size;
                        } else {
                            read_size = pat_size;
                        }

                        if ((chars_read = read(fildes, ptr, read_size)) == -1) {
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s():thread %d,error reading from disk(%s(%d)) - %s, "
                            "file_size =%d, chars_read =%d,page size=%s, seg_num=%ld, seg_addr=0x%llx\n", __LINE__,__FUNCTION__,
                            t,strerror(errno),errno,pattern_nm,file_size,chars_read,page_size_name[sd.page_index],seg,ptr);
                            return(FAILURE);
                        }
                        if (g_data.exit_flag == SET) {
                            return(-1);
                        }
                        if (chars_read < read_size) {
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s():thread %d,ot able to read from disk - %s, "
                            "properly patt_size=%d,file_size =%d, chars_read =%d, err=%d, page size =%s,seg_num=%ld\n", __LINE__,
                            __FUNCTION__,t,g_data.stanza_ptr->pattern_name[0],pat_size,file_size,chars_read,errno,page_size_name[sd.page_index],seg);
                            return(FAILURE);
                        }else{/* If we dont update time stamp of htx, mem may result in hung(with mdt.bu),due to heavy sys call read() operation*/
                            STATS_VAR_INC(bytes_writ,chars_read)
                        }	
                        /* Now we have successfully read chars_read bytes */
                        total_bytes_read += chars_read;
                        ptr+=chars_read;
                       seg_size-=chars_read;
                    }/*ends while (seg_size> 0 )*/                
                    nw--;
                }/*ends if(nw>0*/
                if(nr > 0){
                    rc = (*operation_fun_typ[g_data.stanza_ptr->pattern_type[pi]][READ][mem_access_type_index])(
                                                            (sd.seg_size/g_data.stanza_ptr->width),\
                                                            local_ptr->seg_details[seg].shm_mem_ptr,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            0,\
                                                            0,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            NULL);                    
                    nr--;
                }         
                if (g_data.exit_flag == SET) {
                    goto update_exit;
                }
                if(nc > 0){
                    rc = (*operation_fun_typ[g_data.stanza_ptr->pattern_type[pi]][COMPARE][mem_access_type_index])(
                                                            (sd.seg_size/g_data.stanza_ptr->width),\
                                                            local_ptr->seg_details[seg].shm_mem_ptr,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            trap_flag,\
                                                            &sd,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            NULL);
                    if(rc != SUCCESS){
                        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:compare_data returned %d for compare count %d\n",
                            __LINE__,__FUNCTION__,rc,nc);
                        break;
                    }
                    nc--;
                }
                if (g_data.exit_flag == SET && rc == SUCCESS) {
                    goto update_exit;
                } /* endif */                
                if(rc != SUCCESS){/* If there is any miscompare, rc will have the offset */
                    miscompare_count = dump_miscompared_buffers(t,rc,seg,main_misc_count,&seed,num_oper,trap_flag,pi,&sd);
                    STATS_VAR_INC(bad_others, 1);
                    main_misc_count++;
                    misc_detected++ ;
                    STATS_VAR_INC(bad_others, 1)
                    /*STATS_HTX_UPDATE(UPDATE)*/
                }
            }/* end of while(nw>0 || nr>0 || nc>0)*/
            update_exit:
            STATS_VAR_INC(bytes_writ,((g_data.stanza_ptr->num_writes - nw) * sd.seg_size))
            STATS_VAR_INC(bytes_read,((g_data.stanza_ptr->num_reads - nr) * sd.seg_size))
            STATS_VAR_INC(bytes_read,((g_data.stanza_ptr->num_compares - nc - misc_detected) * sd.seg_size))
            /*STATS_HTX_UPDATE(UPDATE)*/
            misc_detected = 0;
            if (g_data.exit_flag == SET) {
                return -1;
            }
        }/*ends seg loop*/
    }/*ends num_oper loop*/
    close(fildes);
    return SUCCESS;
}

int dump_miscompared_buffers(int ti, unsigned long rc, int seg, int main_misc_count,
        unsigned long *seed_ptr,int pass,int trap_flag,int pi,struct segment_detail* sd){
    char fname[128], msg_text[4096],msg_temp[4096];
    struct thread_data* th = &g_data.thread_ptr[ti];
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[ti]);
    th->testcase_thread_details = (void *)local_ptr;
    int miscompare_count=0, page_offset;
    int  tmp_shm_size=0,buffer_size;
    int ps = local_ptr->seg_details[seg].page_size_index;
    unsigned long new_offset=0;
    unsigned long pat_offset=0; /* pattern offset when pat len > 8 we need this */
    unsigned long expected_pat = 0; /* pattern value which is expected */
    char bit_pattern[8];
    char *shm_mem_ea = local_ptr->seg_details[seg].shm_mem_ptr;
 
    int mem_access_type_index = get_mem_access_operation(pi);
    int width = sd->width;
    displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,"MISCOMPARE DETECTED, "
             "Details:segment number=%d,page size=%s,"
             "width=%d ,shared memory size = 0x%lx and seed = 0x%lx,ptr=%p,rc=%ld\n",
             seg,page_size_name[ps],width,local_ptr->seg_details[seg].shm_size,*seed_ptr,local_ptr->seg_details[seg].shm_mem_ptr,rc);

    if (ps == PAGE_INDEX_16G) {
        new_offset = sd->sub_seg_num*sd->sub_seg_size;
    }

    do
    {
        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "#1.0.dump_miscompared_buffers\n");
        pat_offset = ((rc-1+(new_offset/width))*(width))\
                                    %(g_data.stanza_ptr->pattern_size[pi]);
        if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_RANDOM ) {
            expected_pat = *seed_ptr;
        }else {
            if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                expected_pat =  (unsigned long)(shm_mem_ea+((rc-1)*(width)+new_offset));
            }
            else {
                expected_pat = *(unsigned long *)(&g_data.stanza_ptr->pattern[pi][pat_offset]);
            }

        }
        memcpy(bit_pattern,(char*)&expected_pat,sizeof(unsigned long));

        sprintf(msg_text,"MEMORY MISCOMPARE(hxemem64) in stanza %s,num_oper = %d,Rules file=%s\n"
                "Segment deatils:   seg#:%lu shm  id:%lu\nShared memory Segment Starting EA=0x%p,\n"
                "size = %lu \nShared memory segment consists of pages of size =%s\n"
                "read/write width = %d, Trap_Flag = %d Sub-Segment Number=%lu, Thread=%d,num_of_threads=%d\n"
                "(Ignore if affinity=FLOATING)chip  details: cpu:%d belongs to chip:%d, Memory  belongs to chip= %d\n"
                "Miscompare Offset in the Shared memory segment is (%ld x %d) %ld bytes from the "
                "starting location,\nData expected = 0x%lx\nData retrieved: ",
                g_data.stanza_ptr->rule_id,pass,g_data.rules_file_name,\
                local_ptr->seg_details[seg].seg_num,local_ptr->seg_details[seg].shmid,shm_mem_ea,\
                local_ptr->seg_details[seg].shm_size,page_size_name[ps],\
                width,trap_flag,sd->sub_seg_num,ti,g_data.stanza_ptr->num_threads,th->bind_proc, \
                local_ptr->seg_details[seg].cpu_chip_num, local_ptr->seg_details[seg].mem_chip_num,\
                (rc-1+(new_offset/width)),width, \
                (rc-1+(new_offset/width))*(width),\
                expected_pat);

        switch(width){

            /*Need to take care of prints in case of LE mode*/
            case LS_DWORD: {
                        sprintf(msg_temp,"miscomparing dword(0x%lx)"
                                 " at dword Address(%p)\nActual misc Addr=%p,misc dword data=0x%lx\n",\
                                *(unsigned long *)(shm_mem_ea +((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),
                                (shm_mem_ea + ((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),\
                                (shm_mem_ea + (rc-1)*(width)+new_offset),\
                                *(unsigned long*)(shm_mem_ea +(rc-1)*(width)+new_offset));
                        break;
                    }
            case LS_WORD: {
                        sprintf(msg_temp,"miscomparing word(0x%x)"
                                 " at dword Address(0x%p)\nActual misc Addr=0x%p,misc data word=0x%x\n",\
                                *(unsigned int*)(shm_mem_ea + ((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),
                                (shm_mem_ea + ((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),\
                                (shm_mem_ea + (rc-1)*(width)+new_offset),\
                                *(unsigned int*)(shm_mem_ea + (rc-1)*(width)+new_offset));
                        break;
                    }
            case LS_BYTE: {
                        sprintf(msg_temp,"miscomparing byte(0x%x)"
                                 " at dword Address(0x%p)\nActual misc Addr=0x%p,misc data byte=0x%x\n",\
                                *(unsigned char *)(shm_mem_ea + ((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),
                                (shm_mem_ea + ((((rc-1)*(width))/LS_DWORD)*MIN_PATTERN_SIZE)+new_offset),\
                                (shm_mem_ea + (rc-1)*(width)+new_offset),\
                                *(unsigned char*)(shm_mem_ea + ( rc-1)*(width)+new_offset));
                        break;
                    }
        }
        
         strcat(msg_text,msg_temp); 
        /* If pattern is RANDOM cannot generate dumps and
         * If operation is RIM it does write and read compare immediately so cannot dump buffer files
         */
        if ((g_data.stanza_ptr->pattern_type[pi] != PATTERN_RANDOM ) && (g_data.stanza_ptr->operation != OPER_RIM )) {
            page_offset = (rc-1)*width;/* Offset of miscompare in bytes */
            new_offset = new_offset + (page_offset/(4*KB))*(4*KB); /* The 4k page aligned offset in bytes */
            page_offset &= 0xfff;
 
            tmp_shm_size = local_ptr->seg_details[seg].shm_size - new_offset; /* Size of the remaining segment to be compared now */
            if (ps == PAGE_INDEX_16G ) {
                tmp_shm_size = tmp_shm_size % sd->sub_seg_size;
            }
            /* Size of the page=4096. If it is last page, it can be less */
            buffer_size = (tmp_shm_size>=(4*KB)) ? (4*KB):tmp_shm_size;
            sprintf(fname,"%s/hxemem.%s.shmem.%d.%d_addr_0x%016lx_offset_%d",g_data.htx_d.htx_exer_log_dir,g_data.stanza_ptr->rule_id,\
                main_misc_count,miscompare_count,(unsigned long)(shm_mem_ea+new_offset), page_offset);

            displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "miscompare file name = %s\n",fname);

             if( (main_misc_count<11) && (miscompare_count < 10)) {   /* Number of miscompares to be saved is 10 */
                sprintf(fname,"%s/hxemem.%s.shmem.%d.%d_addr_0x%016lx_offset_%d",g_data.htx_d.htx_exer_log_dir,g_data.stanza_ptr->rule_id,\
                    main_misc_count,miscompare_count, (unsigned long )(shm_mem_ea+new_offset), page_offset);
                hxfsbuf((shm_mem_ea+new_offset),buffer_size, fname,&g_data.htx_d);       
                sprintf(msg_temp,"The miscomparing data in data buffer segment is in the file %s.\n",fname);
                strcat(msg_text,msg_temp); 
                sprintf(fname,"%s/hxemem.%s.pat.%d.%d_addr_0x%016lx_offset_%d",g_data.htx_d.htx_exer_log_dir,g_data.stanza_ptr->rule_id,\
                    main_misc_count,miscompare_count, (unsigned long )(shm_mem_ea+new_offset), page_offset);
                sprintf(msg_temp,"The pattern data which is expected is in the file %s.\n",fname);
                strcat(msg_text,msg_temp);
                hxfsbuf(bit_pattern, MIN_PATTERN_SIZE, fname,&g_data.htx_d);
                miscompare_count++;
            }
            if(main_misc_count > 10) {
                sprintf(msg_temp,"Maximum number of miscompare buffers(10) have already been saved for this segment\n"
                        " No Dump file correspoding to this miscompare will be found in %s  directory.\n",g_data.htx_d.htx_exer_log_dir);
                strcat(msg_text,msg_temp);
            }
            displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "%s",msg_text);

            new_offset = new_offset +(4*KB);    /* Bytes already compared till now (4k page aligned) */
            if (ps != PAGE_INDEX_16G) {
                if(new_offset >= local_ptr->seg_details[seg].shm_size)
                    break;
            } else {
                if(new_offset >= (sd->sub_seg_num+1)*sd->sub_seg_size)
                    break;
            }

        } else { /* pattern_type == PATTERN_RANDOM */
            page_offset = (rc-1)*width;/* Offset of miscompare in bytes */
            new_offset = new_offset+page_offset+width;
            sprintf(msg_temp,"PATTERN is RANDOM or it is a RIM Operation so hxemem64 is not generating dump.\n"
                        "hxemem64 starts compare from the next dword/word/byte\n");
            strcat(msg_text,msg_temp);
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"%s\n",msg_text);
             miscompare_count++;
        }
        tmp_shm_size = local_ptr->seg_details[seg].shm_size - new_offset; /* Size of the remaining segment to be compared now */
        if (ps == PAGE_INDEX_16G ) {
            tmp_shm_size = tmp_shm_size % sd->sub_seg_size;
        }

        if (g_data.stanza_ptr->operation == OPER_STRIDE) {
            return miscompare_count;
        } else if ( g_data.stanza_ptr->operation != OPER_RIM ) {
            rc = (*operation_fun_typ[g_data.stanza_ptr->pattern_type[pi]][COMPARE][mem_access_type_index])(
                                                    (tmp_shm_size/sd->width),\
                                                    (local_ptr->seg_details[seg].shm_mem_ptr + new_offset),\
                                                    g_data.stanza_ptr->pattern[pi],\
                                                    trap_flag,\
                                                    sd,\
                                                    g_data.stanza_ptr,\
                                                    &g_data.stanza_ptr->pattern_size[pi],
                                                    seed_ptr);
        }
        else {/* RIM TEST CASE */
            rc = (*operation_fun_typ[g_data.stanza_ptr->pattern_type[pi]][RIM][mem_access_type_index])(
                                                    (tmp_shm_size/sd->width),\
                                                    (local_ptr->seg_details[seg].shm_mem_ptr + new_offset),\
                                                    g_data.stanza_ptr->pattern[pi],\
                                                    trap_flag,\
                                                    sd,\
                                                    g_data.stanza_ptr,\
                                                    &g_data.stanza_ptr->pattern_size[pi],
                                                    seed_ptr);
        }
        if (g_data.exit_flag == SET) {
            return -1;
        }

        if(rc==0 || (miscompare_count == 11)) {
            break;
        }
    } while(1);
    return miscompare_count;
}
int get_mem_access_operation(int pat){
   if(g_data.stanza_ptr->pattern_type[pat] == PATTERN_ADDRESS ) {
       return DWORD;
    }else {
        if (g_data.stanza_ptr->width == LS_DWORD) return DWORD;
        else if (g_data.stanza_ptr->width == LS_WORD)  return WORD;
        else if (g_data.stanza_ptr->width == LS_BYTE)  return BYTE;
        else{
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: not supported load store width is passed, %d\n",
                __LINE__,__FUNCTION__,g_data.stanza_ptr->width);
            exit(1);
        }
    }

}

int reset_segment_owners(){
    int pi,n,seg;
    struct chip_mem_pool_info *chip = &g_data.sys_details.chip_mem_pool_data[0];
    struct mem_info *sys_mem_details = &g_data.sys_details.memory_details;
    if(g_data.gstanza.global_disable_filters == 0){
        for(n = 0; n <g_data.sys_details.num_chip_mem_pools;n++){
            if(!chip[n].has_cpu_and_mem)continue;
            for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
                for(seg=0;seg<chip[n].memory_details.pdata[pi].num_of_segments;seg++){
                    if(chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].in_use){
                        chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].owning_thread  = -1;
                        chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].in_use         = 0;
                        chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].cpu_chip_num   = -1;
                    }	
                }
            }
        }
    }else{
        for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
            for(seg=0;seg< sys_mem_details->pdata[pi].num_of_segments;seg++){
                if(sys_mem_details->pdata[pi].page_wise_seg_data[seg].in_use){
                    sys_mem_details->pdata[pi].page_wise_seg_data[seg].owning_thread    = -1;
                    sys_mem_details->pdata[pi].page_wise_seg_data[seg].in_use           = 0;
                    sys_mem_details->pdata[pi].page_wise_seg_data[seg].cpu_chip_num     = -1;
                }
            }
        }
    }
    return SUCCESS;
}
void release_thread_resources(int thread_num){

    struct mem_exer_thread_info *local_ptr = (struct mem_exer_thread_info*)g_data.thread_ptr[thread_num].testcase_thread_details;
    free(local_ptr->seg_details);

}
/*******************************************************************************************
*This function checks if the filesystem is located on RAM, if yes then dma_flag will be set*
*so that DMA case can be excluded in such case											   *
*******************************************************************************************/

int check_if_ramfs(void)
{
    int filedes=0;
    unsigned int mode_flag;
    char pattern_nm[256];
    char* pat_ptr = getenv("HTXPATTERNS");
    mem_g.dma_flag = 0; /*intialize with zero*/
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

    debug(HTX_HE_INFO, DBG_IMP_PRINT, "check_if_ramfs : Entered check_if_ramfs \n");
    filedes = open(pattern_nm, O_DIRECT | O_RDONLY, mode_flag);
    if ( errno == 22) {
        displaym(HTX_HE_INFO, DBG_MUST_PRINT,"[%d]%s::open call returned with EINVAL(errno = %d).This can\
            be because if filesystem is located on RAM,then  O_DIRECT flag is considered as invalid, in such case we will skip DMA test\
            case. \n",__LINE__,__FUNCTION__,errno);

         mem_g.dma_flag = 1;
         errno = 0 ;
    }

    if(filedes > 0 ) {
           debug(HTX_HE_INFO, DBG_IMP_PRINT, "check_if_ramfs : closing filedes = %d \n",filedes);
           close (filedes);
    }

    return(SUCCESS);
}

#ifndef __HTX_LINUX__
int check_ame_enabled()
{
    int rc = SUCCESS;
    lpar_info_format2_t info;

    rc = lpar_get_info(LPAR_INFO_FORMAT2, &info, sizeof(info));
    if (rc) {
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "[%d]%s:lpar_get_info failed with rc=%d,errno: %d\n", __LINE__,__FUNCTION__,rc,errno);
        return (FAILURE);
    }
    if ((info.lpar_flags & LPAR_INFO2_AME_ENABLED) == LPAR_INFO2_AME_ENABLED) {
        mem_g.AME_enabled = 1;
    } else {
        mem_g.AME_enabled = 0;
    }
    return rc;
}
#endif

/************************************************************************
*       SIGUSR2 signal handler for cpu hotplug add/remove               *
*************************************************************************/

#ifdef __HTX_LINUX__
void SIGUSR2_hdl(int sig, int code, struct sigcontext *scp)
{

    g_data.stanza_ptr->disable_cpu_bind = 1;
    cpu_add_remove_test = 1;
    hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,"Recieved SIGUSR2 signal\n");
#if 0
    int i, rc = 0;

    if (mem_g.affinity == LOCAL ) {
        /*for (i=0; i < mem_info.num_of_threads; i++) {
            if (mem_info.tdata_hp[i].tid != -1) {
                rc = check_cpu_status_sysfs(mem_info.tdata_hp[i].physical_cpu);
                if (rc = 0) {
                    displaym(HTX_HE_INFO,DBG_MUST_PRINT," cpu %d has been removed, thread:%d exiting in affinity yes case \n",mem_info.tdata_hp[i].physical_cpu,mem_info.tdata_hp[i].thread_num);
                    pthread_cancel(mem_info.tdata_hp[i].tid);
                }
            }
        }*/
        g_data.hotplug_flag= 1;
    }
    g_data.hotplug_flag = 1;
#endif
}
#endif

#ifdef    _DR_HTX_
void SIGRECONFIG_handler(int sig, int code, struct sigcontext *scp)
{
    int rc;
    char hndlmsg[512];
    dr_info_t dr_info;          /* Info about DR operation, if any */
    int i, bound_cpu;

    hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,"DR: SIGRECONFIG signal received \n");

    do {
        rc = dr_reconfig(DR_QUERY,&dr_info);
    } while ( rc < 0 && errno == EINTR);
    if ( rc == -1) {
        if ( errno != ENXIO){
            hxfmsg(&g_data.htx_d,errno,HTX_HE_HARD_ERROR, "dr_reconfig(DR_QUERY) call failed. \n");
        }
        return;
    }

    if (dr_info.mem == 1){
        sprintf(hndlmsg,"DR: DLPAR details"
            "Phase - Check:  %d, Pre: %d, Post: %d, Post Error: %d\n"\
            "Type - Mem add: %d remove: %d, ent_cap = %d, hibernate = %d \n",\
             dr_info.check, dr_info.pre, dr_info.post, dr_info.posterror, dr_info.add, dr_info.rem, dr_info.ent_cap, dr_info.hibernate);
        hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
    }

    if (dr_info.cpu == 1 && (dr_info.rem || dr_info.add) ) {
        sprintf(hndlmsg,"DR: DLPAR details"
            "Phase - Check:  %d, Pre: %d, Post: %d, Post Error: %d\n"\
            "Type - Cpu add: %d remove: %d, bcpu = %d \n",\
             dr_info.check, dr_info.pre, dr_info.post, dr_info.posterror, dr_info.add, dr_info.rem, dr_info.bcpu);
        hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
    }
    if ( g_data.stanza_ptr == NULL ) {
        g_data.stanza_ptr = &g_data.stanza[0];
    }
    /*
     * Check-phase for CPU DR
     * Handle only CPU removals, new CPUs will be used from the next stanza.
     */
    if (dr_info.check && dr_info.cpu && dr_info.rem) {

        g_data.cpu_DR_flag = 1;
        cpu_add_remove_test = 1;
        /* Disable next binds and look for current bound cpu to thread,unbind it*/
        g_data.stanza_ptr->disable_cpu_bind = 1;
        if(g_data.thread_ptr != NULL){
            for(i=0; i<g_data.stanza_ptr->num_threads; i++) {
                /* Also check if the running thread has already completed. */
                if ((g_data.thread_ptr[i].bind_proc == dr_info.bcpu) &&
                    (g_data.thread_ptr[i].tid != -1)) {
                    /* Unbind thread from the CPU under DR */
                    if (bindprocessor(BINDTHREAD,g_data.thread_ptr[i].kernel_tid,
                        PROCESSOR_CLASS_ANY)) {
                        if (errno == ESRCH) {
                            continue;
                        }
                        sprintf(hndlmsg,"Unbind failed. errno %d(%s), for thread num %d,cpu=%d"
                                ", TID %d.\n", errno,strerror(errno),i,g_data.thread_ptr[i].bind_proc,g_data.thread_ptr[i].tid);
                        hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
                    }
                    /*
                     * More than one thread could be bound to the same CPU.
                     * as bind_proc = bind_proc%get_num_of_proc()
                     * Run through all the threads, don't break the loop.
                     */
                }
            }
        }
        sprintf(hndlmsg,"DR: In check phase and cpu:%d is removed, we unbind threads which are affectd\n",dr_info.bcpu);
        hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);

        if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)){
            sprintf(hndlmsg,"dr_reconfig(DR_RECONFIG_DONE) failed."
                    " error %d \n", errno);
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
        }
        else{
            sprintf(hndlmsg,"DR:DR_RECONFIG_DONE Success!!,in check phase for cpu=%d \n",dr_info.bcpu);
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);

        }
        return;
    }
    /* Post/error phase; reset tracker count  */
    if ((dr_info.post || dr_info.posterror) && dr_info.cpu && dr_info.rem) {
        g_data.cpu_DR_flag = 0;  /*falg is set to recollect cpu details for tlbie test case*/
    }

    /* For any other signal check/Pre/Post-phase, respond with DR_RECONFIG_DONE */
    if (dr_info.check || dr_info.pre || dr_info.post ) {
        if (dr_info.mem && dr_info.check) {
            sprintf(hndlmsg,"DR:Mem DR operation performed,setting  g_data.mem_DR_flag = 1");
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
            g_data.mem_DR_flag = 1;
        }
        if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)){
            sprintf(hndlmsg,"dr_reconfig(DR_RECONFIG_DONE) failed."
                    " error %d ,check=%d,pre=%d,post=%d\n",dr_info.check,dr_info.pre,dr_info.post,errno);
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
        }
        else{
            sprintf(hndlmsg,"DR:DR_RECONFIG_DONE Successfully after check=%d, pre=%d, post=%d   phhase !! \n",dr_info.check,dr_info.pre,dr_info.post);
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
        }
    }
    return;
}
#endif

void print_memory_allocation_seg_details(){
    int pi,n;
    char msg[4096],msg_temp[1024];

    sprintf(msg,"=======rule id:%s, Mem segment details*========\n"
        "In rule file:alloc_mem_percent=%u, alloc_segment_size=%ld KB\n",
        g_data.stanza_ptr->rule_id,g_data.gstanza.global_alloc_mem_percent,(g_data.gstanza.global_alloc_segment_size/ KB));
    if(g_data.gstanza.global_disable_filters == 1){
        for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
            if(!g_data.sys_details.memory_details.pdata[pi].supported)continue;
            unsigned long long mem = ((pi < PAGE_INDEX_2M) ?
                (g_data.gstanza.global_alloc_mem_percent/100.0) * g_data.sys_details.memory_details.pdata[pi].free :
                g_data.sys_details.memory_details.pdata[pi].free);
            sprintf(msg_temp,"\n\tpage size:%s\t mem:%llu MB\t total segments=%d\n",
                page_size_name[pi],(mem/MB),g_data.sys_details.memory_details.pdata[pi].num_of_segments);
            strcat(msg,msg_temp);
            if(g_data.sys_details.memory_details.pdata[pi].page_wise_usage_mem > 0){
                sprintf(msg_temp,"\tIn use mem details:\n\tmem = %llu MB\t"
                    "\tnum_segs:%d\n",(g_data.sys_details.memory_details.pdata[pi].page_wise_usage_mem/MB)
                    ,g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments);	
                strcat(msg,msg_temp);
            }
        }
	}else{	
		for(n = 0; n <g_data.sys_details.num_chip_mem_pools;n++){
			if(!g_data.sys_details.chip_mem_pool_data[n].has_cpu_and_mem)continue;
			sprintf(msg_temp,"Chip[%d]:cpus:%d\n",n,g_data.sys_details.chip_mem_pool_data[n].num_cpus);
			strcat(msg,msg_temp);
			if(g_data.sys_details.chip_mem_pool_data[n].in_use_num_cpus > 0){
				sprintf(msg_temp,"\tIn use cpus: %d,\t",g_data.sys_details.chip_mem_pool_data[n].in_use_num_cpus);
				strcat(msg,msg_temp);
				if(g_data.sys_details.chip_mem_pool_data[n].in_use_num_cpus != g_data.sys_details.chip_mem_pool_data[n].num_cpus){
					for(int i=0;i<g_data.sys_details.chip_mem_pool_data[n].in_use_num_cpus;i++){
						sprintf(msg_temp,":%d",g_data.sys_details.chip_mem_pool_data[n].in_use_cpulist[i]);
						strcat(msg,msg_temp);
					}
				}
			}
			for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
				if(!g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].supported)continue;
				unsigned long long mem = ((pi < PAGE_INDEX_2M) ? 
					(g_data.gstanza.global_alloc_mem_percent/100.0) * g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].free :
					g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].free);
				sprintf(msg_temp,"\n\tpage size:%s\t mem:%llu MB\t total segments=%d\n",
					page_size_name[pi],(mem/MB),g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].num_of_segments);
				strcat(msg,msg_temp); 
				if(g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_usage_mem > 0){
					sprintf(msg_temp,"\tIn use mem details:\n\tmem = %llu MB\t"
						"\tnum_segs:%d\n",(g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_usage_mem/MB)
						,g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].in_use_num_of_segments);
					strcat(msg,msg_temp);
				}
			}
			if(strlen(msg) >= (2*KB)){
    			displaym(HTX_HE_INFO,DBG_MUST_PRINT,"%s\n",msg);
				msg[0]=0;/*move terminal char to start of array*/
			}
		}   
	}
    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"%s\n",msg);
}

int log_mem_seg_details(){
    
    char dump_file[100];
    FILE *fp;
    struct chip_mem_pool_info *chip = &g_data.sys_details.chip_mem_pool_data[0];
	struct mem_info* sys_mem_details = &g_data.sys_details.memory_details;
    int n,pi,seg,bind_cpu = -1;
    char temp_str[100];
    if(g_data.gstanza.global_disable_filters == 0){
        sprintf(temp_str,"CHIP LEVEL MEMORY EXERCISE");
    }else{
        sprintf(temp_str,"SYSTEM VIEW MEMORY EXERCISE,NO STRICT AFFINITY MAINTAINED");
    }
    sprintf(dump_file, "%s/mem_segment_details",g_data.htx_d.htx_exer_log_dir);

    fp = fopen(dump_file, "w");
    if ( fp == NULL) {
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s():Error opening %s file,errno:%d(%s),continuing without debug log file \n"
            ,__LINE__,__FUNCTION__,dump_file,errno,strerror(errno));
        return (FAILURE);
    }
    fprintf(fp, "STANZA:rule_id = %s, %s\n",g_data.stanza_ptr->rule_id,temp_str);
    fprintf(fp,"***************************************************************************"
                "***************************************************************************\n");
    fprintf(fp," In_use_seg_num\t page_size\tOwning_thread\tcpu_num \tmem_chip_num\t segment_id \t\t segment_size \t\t segment_EA\n");
	if(g_data.gstanza.global_disable_filters == 0){
		for(n = 0; n <g_data.sys_details.num_chip_mem_pools;n++){
			if(!chip[n].has_cpu_and_mem)continue;
			fprintf(fp,"========================================================================"
						"========================================================================\n");
			for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
				for(seg=0;seg<chip[n].memory_details.pdata[pi].num_of_segments;seg++){
					if(!chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].in_use)continue;
						if(g_data.stanza_ptr->disable_cpu_bind == 0 && chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].owning_thread != -1){
							bind_cpu = g_data.thread_ptr[chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].owning_thread].bind_proc;
						}
						fprintf(fp," \t%lu \t %s \t\t %d \t\t %d \t\t %d \t\t %lu \t\t %lu \t\t 0x%llx\n",
							chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].seg_num,page_size_name[pi],
							chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].owning_thread,
							bind_cpu,
							chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].mem_chip_num,
							chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].shmid,
							chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size,
							(unsigned long long)chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].shm_mem_ptr);
				}
			}
		}
	}else{
		fprintf(fp,"========================================================================"
					"========================================================================\n");
		for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
			for(seg=0;seg<sys_mem_details->pdata[pi].in_use_num_of_segments;seg++){
				if(!sys_mem_details->pdata[pi].page_wise_seg_data[seg].in_use)continue;
					if(g_data.stanza_ptr->disable_cpu_bind == 0 && sys_mem_details->pdata[pi].page_wise_seg_data[seg].owning_thread != -1){
						bind_cpu = g_data.thread_ptr[sys_mem_details->pdata[pi].page_wise_seg_data[seg].owning_thread].bind_proc;
					}
					fprintf(fp," \t%lu \t %s \t\t %d \t\t %d \t\t %d \t\t %lu \t\t %lu \t\t 0x%llx\n",
						sys_mem_details->pdata[pi].page_wise_seg_data[seg].seg_num,page_size_name[pi],
						sys_mem_details->pdata[pi].page_wise_seg_data[seg].owning_thread,
						bind_cpu,
						sys_mem_details->pdata[pi].page_wise_seg_data[seg].mem_chip_num,
						sys_mem_details->pdata[pi].page_wise_seg_data[seg].shmid,
						sys_mem_details->pdata[pi].page_wise_seg_data[seg].shm_size,
						(unsigned long long)sys_mem_details->pdata[pi].page_wise_seg_data[seg].shm_mem_ptr);
			}
		}
	}
	fprintf(fp,"****************************************************************************"
                "****************************************************************************\n");
    fclose(fp);
    return SUCCESS;
}

void* stats_update_thread_function(void* null_ptr){
    int t=0;
    for(;;){
        while((t++) < STATS_UPDATE_INTERVAL){
            sleep(1);
            if (g_data.exit_flag == SET) {
                break;
            }
        }

        pthread_mutex_lock(&g_data.tmutex);
        g_data.htx_d.bytes_writ = g_data.nest_stats.bytes_writ;
        g_data.htx_d.bytes_read = g_data.nest_stats.bytes_read;
        g_data.htx_d.bad_others = g_data.nest_stats.bad_others;
        /*Reset nest stats variables*/
        g_data.nest_stats.bytes_writ = 0;
        g_data.nest_stats.bytes_read = 0;
        g_data.nest_stats.bad_others = 0;
        pthread_mutex_unlock(&g_data.tmutex);

        if((g_data.htx_d.bytes_writ > 0) || (g_data.htx_d.bytes_read > 0) || (g_data.htx_d.bad_others > 0)){
            STATS_HTX_UPDATE(UPDATE);
        }
        if (g_data.exit_flag == SET) {
            pthread_exit((void *)0);
        }
        t = 0;
    }

}

int allocate_mem_for_threads(){

    g_data.thread_ptr = (struct thread_data*)malloc(sizeof(struct thread_data) * g_data.stanza_ptr->num_threads);
    if(g_data.thread_ptr == NULL){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: malloc failed to allocate %d bytes of memory with error(%d):%s, total threads =%d\n",
            __LINE__,__FUNCTION__,(sizeof(struct thread_data) * g_data.stanza_ptr->num_threads),errno,strerror(errno),g_data.stanza_ptr->num_threads);
        return(FAILURE);

    }
    for(int i=0;i<g_data.stanza_ptr->num_threads;i++){
        g_data.thread_ptr[i].thread_num = -1;
        g_data.thread_ptr[i].bind_proc = -1;

    }
    mem_g.mem_th_data = (struct mem_exer_thread_info*)malloc(sizeof(struct mem_exer_thread_info) * g_data.stanza_ptr->num_threads);
    if(mem_g.mem_th_data == NULL){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: malloc failed to allocate %d bytes of memory with error(%d):%s, total threads =%d\n",
            __LINE__,__FUNCTION__,(sizeof(struct mem_exer_thread_info) * g_data.stanza_ptr->num_threads),errno,strerror(errno),g_data.stanza_ptr->num_threads);
        return(FAILURE);
    }
    return SUCCESS;
}
