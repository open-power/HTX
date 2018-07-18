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
extern char page_size_name[MAX_PAGE_SIZES][8];
extern struct nest_global g_data;
struct mem_exer_info mem_g;
int fill_mem_thread_context(void);
int fill_mem_thread_context_with_filters_disabled(void);
int create_and_run_mem_thread_operation(void);
int reset_segment_owners(void);
int fill_affinity_based_thread_structure(struct chip_mem_pool_info*, struct chip_mem_pool_info*,int);
int do_mem_operation(int);
int do_rim_operation(int);
int do_dma_operation(int);
int do_rand_mem_operation(int);
int get_mem_access_operation(int);
void* mem_thread_function(void *);
void miscomp_rand_temp(unsigned long num_operations,void *seg_address,void *seg);
#ifdef __HTX_LINUX__
void SIGUSR2_hdl(int, int, struct sigcontext *);
#endif

mem_op_fptr  mem_operation_fun[MAX_MEM_OPER_TYPES]=
    {
        &do_mem_operation,
        &do_stride_operation,
        &do_rim_operation,
        &do_dma_operation,
        &do_rand_mem_operation,
        &do_rand_mem_operation,
        (mem_op_fptr)&fun_NULL,
        (mem_op_fptr)&fun_NULL,
        (mem_op_fptr)&fun_NULL,
    };

mem_data_op_fptr mem_wrc_fun[MAX_PATTERN_TYPES][MAX_DATA_OPER_TYPES][MAX_MEM_ACCESS_TYPES]= 
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
        {&mem_operation_write_addr,(mem_data_op_fptr)&fun_NULL,(mem_data_op_fptr)&fun_NULL},
        {&mem_operation_read_dword,(mem_data_op_fptr)&fun_NULL,(mem_data_op_fptr)&fun_NULL},
        {&mem_operation_comp_addr,(mem_data_op_fptr)&fun_NULL,(mem_data_op_fptr)&fun_NULL},
        {&mem_operation_write_addr_comp,(mem_data_op_fptr)&fun_NULL,(mem_data_op_fptr)&fun_NULL}
    },
    {
        {&rand_operation_write_dword,&rand_operation_write_word,&rand_operation_write_byte},
        {&mem_operation_read_dword,&mem_operation_read_word,&mem_operation_read_byte},
        {&rand_operation_comp_dword,&rand_operation_comp_word,&rand_operation_comp_byte},
        {&rand_operation_rim_dword,&rand_operation_rim_word,&rand_operation_rim_byte}
    },
    {
        {&rand_mem_access_wr_dword,&rand_mem_access_wr_word,&rand_mem_access_wr_byte},
        {&rand_mem_access_rd_dword,&rand_mem_access_rd_word,&rand_mem_access_rd_byte},
        {&rand_mem_access_cmp_dword,&rand_mem_access_cmp_word,&rand_mem_access_cmp_byte},
        {(mem_data_op_fptr)&fun_NULL,(mem_data_op_fptr)&fun_NULL,(mem_data_op_fptr)&fun_NULL}
    }
};

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
					if(std_seg_size < sys_memptr->pdata[pi].psize){
						std_seg_size = sys_memptr->pdata[pi].psize;
					}
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
                    g_data.gstanza.global_alloc_mem_percent= ((double)((double)used_mem / (double)sys_memptr->pdata[pi].free)) * 100.0;
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

            if((((used_mem/total_cpus) < std_seg_size)) || (std_seg_size == DEF_SEG_SZ_16G)){
                if(used_mem != 0){
                    num_segs = total_cpus;
                }
				/* Below check is added because on certain system per chip 16G pages found to be less than num cpus*/
				if((used_mem/total_cpus) < sys_memptr->pdata[pi].psize){
					num_segs = (used_mem / sys_memptr->pdata[pi].psize);
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
                if(((used_mem/total_cpus) < std_seg_size) || (std_seg_size == DEF_SEG_SZ_16G)){/*If memory (huge pages specially) falls to be less than 256M,adjust segment size to num of threads*/
                    sys_seg_details[seg].shm_size = (used_mem/total_cpus);
					if(sys_seg_details[seg].shm_size < sys_memptr->pdata[pi].psize){
						sys_seg_details[seg].shm_size = sys_memptr->pdata[pi].psize;
					}
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
    int num_of_chips = g_data.sys_details.num_chip_mem_pools,min_segments=0;
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
        if(g_data.stanza[0].operation == LATENCY_TEST){
            min_segments = LATENCY_TEST_SEGMENTS;/*in LATENCY_TEST, objective is to measure on memory latency, we will take 3 readings(one for each segment) */
        }else{
            min_segments = mem_details_per_chip[n].num_cpus;
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
						if(std_seg_size < mem_details_per_chip[n].memory_details.pdata[pi].psize){
							std_seg_size = mem_details_per_chip[n].memory_details.pdata[pi].psize;
						}
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
                        g_data.gstanza.global_alloc_mem_percent= ((double)(used_mem / mem_details_per_chip[n].memory_details.pdata[pi].free)) * 100;
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
                if(((used_mem/min_segments) < std_seg_size) || (g_data.stanza[0].operation == LATENCY_TEST)){
                    num_segs = min_segments;
                    /* Below check is added because on certain system per chip 16M pages found to be less than num cpus(i.e min_segments) in that chip*/
                    if((used_mem/min_segments) < mem_details_per_chip[n].memory_details.pdata[pi].psize){
                        num_segs = (used_mem / mem_details_per_chip[n].memory_details.pdata[pi].psize);
                    }
                    fixed_size_num_segs=num_segs;
                    rem_seg_size = 0;
                }else {
                    num_segs      = (used_mem/std_seg_size);
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
                    if((used_mem/min_segments) < std_seg_size){/*If memory (huge pages specially) falls to be less than 256M,adjust segment size to num of threads*/
                        seg_details[seg].shm_size = (used_mem/min_segments);
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
/**************************************************************************************
*run_mem_stanza_operation: calls specific test case per stanza as specified in rule file*
***************************************************************************************/
int run_mem_stanza_operation(){
	int rc=SUCCESS;

    rc = apply_filters();
    if(rc != SUCCESS) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:apply_filters() failed with rc = %d",__LINE__,__FUNCTION__,rc);
        remove_shared_memory();
        return (rc);
    }
    print_memory_allocation_seg_details(DBG_MUST_PRINT);
    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Total threads to be created = %d\n",g_data.stanza_ptr->num_threads);
    mem_g.affinity = g_data.stanza_ptr->affinity;

    rc = fill_mem_thread_context();
    if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: fill_mem_thread_context() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
        return(FAILURE);
    }

    rc = log_mem_seg_details();
    if(rc != SUCCESS){
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"log_mem_seg_details() returned rc =%d\n",rc);
     }

     g_data.htx_d.test_id++;
     hxfupdate(UPDATE,&g_data.htx_d);

     rc = create_and_run_mem_thread_operation();
     if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: create_and_run_mem_thread_operation() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
        return(FAILURE);
     }


     return(SUCCESS);
}
int fill_mem_thread_context(){
    int chip,rc,start_thread_index=0;
    struct chip_mem_pool_info *chp = &g_data.sys_details.chip_mem_pool_data[0];
    int mem_chip_used[MAX_CHIPS] = {[0 ...(MAX_CHIPS-1)]=-1};

    rc = allocate_mem_for_threads();
    if(rc != SUCCESS){
        return(FAILURE);
    }
    if(g_data.gstanza.global_disable_filters == 1){
        rc = fill_mem_thread_context_with_filters_disabled();
        if(rc != SUCCESS){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_mem_thread_context_with_filters_disabled() failed with rc = %d\n",
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
        if(g_data.stanza_ptr->seed == 0){
            local_ptr->th_seed = th[thread_num].thread_num;
        }
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
        if(local_ptr->num_segs > 0){
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

int fill_mem_thread_context_with_filters_disabled(){

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
        local_ptr->write_latency = 0.0;
        local_ptr->read_latency = 0.0;
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

int create_and_run_mem_thread_operation(){
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
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_create failed with rc = %d (errno %d):(%s) for thread_num = %d\n",
                __LINE__,__FUNCTION__,rc,errno,strerror(errno),tnum);
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
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join with rc = %d (errno %d):(%s) for thread_num = %d \n",
                __LINE__,__FUNCTION__,rc,errno,strerror(errno),thread_num);
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
        rc = htx_bind_thread(th->bind_proc,th->bind_proc);
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
    if(mem_operation_fun[g_data.stanza_ptr->operation] == NULL){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:The stanza's (%s) oper(%s) field"
            "is invalid.Please check the oper field \n",__LINE__,__FUNCTION__,g_data.stanza_ptr->rule_id,g_data.stanza_ptr->oper);
        release_thread_resources(th->thread_num);
        pthread_exit((void *)0);
    }
    if((g_data.stanza_ptr->operation == OPER_DMA) && (mem_g.dma_flag == 1)){
        release_thread_resources(th->thread_num);
        pthread_exit((void *)0);
    }
    rc = (*mem_operation_fun[g_data.stanza_ptr->operation])(th->thread_num);

    release_thread_resources(th->thread_num);
    pthread_exit((void *)0);
}

int do_mem_operation(int t){
    int  num_oper,seg,pi=0,rc=0,misc_detected = 0; /* pi is pattern index */
    static int main_misc_count=0;
    struct segment_detail sd;
    struct thread_data* th = &g_data.thread_ptr[t];
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t]);
    th->testcase_thread_details = (void *)local_ptr;
    int nw=0,nr=0,nc=0,trap_flag=0,mem_access_type_index;
    unsigned long seed = local_ptr->th_seed;
    unsigned long backup_seed = seed;
    
    debug(HTX_HE_INFO,DBG_IMP_PRINT,"Starting do_mem_operation with num_oper = %d for thread[%d]= %d to exercise tot segs=%d cpu = %d \n"
                    ,g_data.stanza_ptr->num_oper,t,th->thread_num,local_ptr->num_segs,th->bind_proc);
    if( (g_data.stanza_ptr->misc_crash_flag) && (g_data.kdb_level) )
        trap_flag = 1;    /* Trap is set */
    else
        trap_flag = 0;    /* Trap is not set */

    if(g_data.stanza_ptr->attn_flag)
        trap_flag = 2;    /* attn is set */


    for (num_oper =0;num_oper < g_data.stanza_ptr->num_oper; num_oper++){
        /*if(g_data.mem_DR_flag){
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d is exiting for current stanza on mem DR operation \n",t);
            return(-1);
        }*/
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
            backup_seed = local_ptr->th_seed;
            local_ptr->seg_details[seg].in_use = 1;
            if( g_data.stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                sd.width = LS_DWORD;
            } else {
                sd.width = g_data.stanza_ptr->width;
            }
            /*rand_debug*/
            local_ptr->rc_offset = 0;
            local_ptr->rc_pass_count = 0;

            nw = g_data.stanza_ptr->num_writes;
            nr = g_data.stanza_ptr->num_reads;
            nc = g_data.stanza_ptr->num_compares;
            mem_access_type_index = get_mem_access_operation(pi);
            while(nw>0 || nr>0 || nc>0){
                if(nw>0){
                    seed = backup_seed;
					srand48_r(seed,&local_ptr->buffer);
                    rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][WRITE][mem_access_type_index])(
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
                    seed = backup_seed;
					srand48_r(seed,&local_ptr->buffer);
                    rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][READ][mem_access_type_index])(
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
                    seed = backup_seed;
					srand48_r(seed,&local_ptr->buffer);
                    rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][COMPARE][mem_access_type_index])(
                                                            (sd.seg_size/sd.width),\
                                                            local_ptr->seg_details[seg].shm_mem_ptr,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            trap_flag,\
                                                            &sd,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            &seed);
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
                dump_miscompared_buffers(t,rc,seg,main_misc_count,&backup_seed,num_oper,trap_flag,pi,&sd);
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
    int  k,num_oper,seg,pi=0,rc=0,misc_detected = 0; /* pi is pattern index */
    static int main_misc_count=0;
    struct segment_detail sd;
    struct thread_data* th = &g_data.thread_ptr[t];
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t]);
    th->testcase_thread_details = (void *)local_ptr;
    int nw=0,trap_flag=0,mem_access_type_index;
    unsigned long seed = local_ptr->th_seed, seg_size;
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
        /*if(g_data.mem_DR_flag){
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d is exiting for current stanza on mem DR operation \n",t);
            return(-1);
        }*/
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
            backup_seed = local_ptr->th_seed;
            local_ptr->seg_details[seg].in_use = 1;
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
				srand48_r(seed,&local_ptr->buffer);
                for(k=0;(k*seg_size < sd.seg_size);k++){/*k operates on next sub segments */
                    sd.sub_seg_num=k;
                    rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][RIM][mem_access_type_index])
                                                            ((seg_size/sd.width),\
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
                dump_miscompared_buffers(t,rc,seg,main_misc_count,&seed,num_oper,trap_flag,pi,&sd);
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
    int  num_oper,seg,pi=0,rc=0,misc_detected = 0; /* pi is pattern index */
    int  chars_read,fildes,file_size=0,mode_flag;
    static int main_misc_count=0;
    struct segment_detail sd;
    struct thread_data* th = &g_data.thread_ptr[t];
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t]);
    th->testcase_thread_details = (void *)local_ptr;
    int nw=0,nr=0,nc=0,trap_flag=0,mem_access_type_index;
    unsigned long seed = local_ptr->th_seed;
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
            local_ptr->seg_details[seg].in_use = 1;
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
                    rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][READ][mem_access_type_index])(
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
                    rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][COMPARE][mem_access_type_index])(
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
                    dump_miscompared_buffers(t,rc,seg,main_misc_count,&seed,num_oper,trap_flag,pi,&sd);
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
int do_rand_mem_operation(int t){
    int  num_oper,seg,misc_detected=0,pi=0,rc=0;
    static int main_misc_count=0;
    struct segment_detail sd;
    struct thread_data* th = &g_data.thread_ptr[t];
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t]);
    int nw=0,nr=0,nc=0,trap_flag=0,mem_access_type_index,operations;
    int cache_line_size = g_data.sys_details.cache_info[L3].cache_line_size;
    unsigned long seed = local_ptr->th_seed;
    unsigned long backup_seed = seed;

        
    /*displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Starting do_rand_mem_operation with num_oper = %d for thread[%d]= %d to exercise tot segs=%d cpu = %d \n"
        ,g_data.stanza_ptr->num_oper,t,th->thread_num,local_ptr->num_segs,th->bind_proc);*/

    for (num_oper =0;num_oper < g_data.stanza_ptr->num_oper; num_oper++){
    
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
            backup_seed = local_ptr->th_seed;
            local_ptr->seg_details[seg].in_use = 1;
            if( g_data.stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                sd.width = LS_DWORD;
            } else {
                sd.width = g_data.stanza_ptr->width;
            }
            nw = g_data.stanza_ptr->num_writes;
            nr = g_data.stanza_ptr->num_reads;
            nc = g_data.stanza_ptr->num_compares;

            operations = sd.seg_size/cache_line_size;
            if(g_data.stanza_ptr->operation == LATENCY_TEST){
                nc = 0;
                operations = LATENCY_TEST_OPERS;
            }
            g_data.stanza_ptr->pattern_type[pi] = RANDOM_MEM_ACCESS;
            mem_access_type_index = get_mem_access_operation(pi);
            while(nw>0 || nr>0 || nc>0) {
                if(nw>0){
                    seed = backup_seed;
					srand48_r(seed,&local_ptr->buffer);
                    rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][WRITE][mem_access_type_index])(
                                                            operations,\
                                                            local_ptr->seg_details[seg].shm_mem_ptr,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            trap_flag,\
                                                            &sd,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            &seed);
                    nw--;
                    /*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d  complted %d Write  operations, for seg = %d(page_wise seg #%d),page=%d,pat=%d,\n",
                        t,(nw+1),seg,sd.seg_num,page_size_name[sd.page_index],pi);*/
                }
                if (g_data.exit_flag == SET) {
                    goto update_exit;
                }
                if(nr>0){
                    seed = backup_seed;
					srand48_r(seed,&local_ptr->buffer);
                    rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][READ][mem_access_type_index])(
                                                            operations,\
                                                            local_ptr->seg_details[seg].shm_mem_ptr,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            trap_flag,\
                                                            &sd,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            &seed);

                    nr--;
                }
                    /*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d  complted %d READ operations, for seg = %d(page_wise seg #%d),page=%d,pat=%d\n",
                        t,(nr+1),seg,sd.seg_num,page_size_name[sd.page_index],pi);*/

                if (g_data.exit_flag == SET) {
                    goto update_exit;
                }
                if(nc>0){
                    seed = backup_seed;
					srand48_r(seed,&local_ptr->buffer);
                    rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][COMPARE][mem_access_type_index])(
                                                            operations,\
                                                            local_ptr->seg_details[seg].shm_mem_ptr,\
                                                            g_data.stanza_ptr->pattern[pi],\
                                                            trap_flag,\
                                                            &sd,\
                                                            g_data.stanza_ptr,\
                                                            &g_data.stanza_ptr->pattern_size[pi],\
                                                            &seed);
                    if(rc != SUCCESS){
                        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:compare_data returned %d for compare count %d\n",
                            __LINE__,__FUNCTION__,rc,nc);
                        break;
                    }
                    nc--;
                    /*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Thread: %d  complted %d READ-COMPARE operations, for seg = %d(page_wise seg #%d),page=%d,pat=%d\n",
                        t,(nr+1),seg,sd.seg_num,page_size_name[sd.page_index],pi);*/
                }
                if (g_data.exit_flag == SET && rc == SUCCESS) {
                    goto update_exit;
                } /* endif */
            }                
            if(rc != SUCCESS){/* If there is any miscompare, rc will have the offset */
                dump_miscompared_buffers(t,rc,seg,main_misc_count,&seed,num_oper,trap_flag,pi,&sd);
                STATS_VAR_INC(bad_others, 1);
                main_misc_count++;
                misc_detected++ ;
                STATS_VAR_INC(bad_others, 1)
                /*STATS_HTX_UPDATE(UPDATE)*/
            }
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
            misc_detected = 0;
            if (g_data.exit_flag == SET) {
                return -1;
            }
        }/*seg loop*/
    }/*num_oper loop*/            

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
            expected_pat = local_ptr->rand_expected_value;
        }else {
            if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                expected_pat =  (unsigned long)(shm_mem_ea+((rc-1)*(width)+new_offset));
            }
            else {
                expected_pat = *(unsigned long *)(&g_data.stanza_ptr->pattern[pi][pat_offset]);
            }

        }
        memcpy(bit_pattern,(char*)&expected_pat,sizeof(unsigned long));

        sprintf(msg_text,"************************************************************************\n"
				"MEMORY MISCOMPARE(hxemem64) in\tstanza %s,\tnum_oper = %d,\tRules file=%s\n\n"
                "Memory Segment deatils:\n\tseg#:\t\t%lu\n\tshm_id:\t\t%lu\n\tStarting EA:\t%p\n\t"
                "segment size:\t%lu \n\tpage size:\t%s\n\tSub-Seg Number:\t%lu\n\t"
                "read/write width:\t%d\nTrap_Flag:\t%d\nThread:\t%d\nnum_of_threads:\t%d\n\n"
                "CHIP Details:(Ignore if affinity=FLOATING)\n\tcpu: %d\tbelongs to chip: %d\n\tMemory belongs to chip: %d\n\n"
                "Miscompare Offset in the Shared memory segment is (%ld x %d) %ld bytes from the "
                "starting location,\n\tData expected:\t0x%lx\n\tData retrieved:\t0x%lx\n\t\t",
                g_data.stanza_ptr->rule_id,pass,g_data.rules_file_name,\
                local_ptr->seg_details[seg].seg_num,local_ptr->seg_details[seg].shmid,shm_mem_ea,\
                local_ptr->seg_details[seg].shm_size,page_size_name[ps],sd->sub_seg_num, \
                width,trap_flag,ti,g_data.stanza_ptr->num_threads,th->bind_proc, \
                local_ptr->seg_details[seg].cpu_chip_num, local_ptr->seg_details[seg].mem_chip_num,\
                (rc-1+(new_offset/width)),width, \
                (rc-1+(new_offset/width))*(width),\
                expected_pat,*(unsigned long*)(shm_mem_ea +(rc-1)*(width)+new_offset));


        switch(width){

            /*Need to take care of prints in case of LE mode*/
            case LS_DWORD: {
                        sprintf(msg_temp,"miscomparing dword(0x%lx)"
                                 " at dword Address(%p)\n\t\tActual misc Addr=%p, misc dword data=0x%lx\n",\
                                *(unsigned long *)(shm_mem_ea +((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),
                                (shm_mem_ea + ((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),\
                                (shm_mem_ea + (rc-1)*(width)+new_offset),\
                                *(unsigned long*)(shm_mem_ea +(rc-1)*(width)+new_offset));
                        break;
                    }
            case LS_WORD: {
                        sprintf(msg_temp,"miscomparing dword(0x%lx)"
                                 " at dword Address(%p)\n\t\tActual misc Addr=%p, misc data word=0x%x\n",\
                                *(unsigned long*)(shm_mem_ea + ((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),
                                (shm_mem_ea + ((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),\
                                (shm_mem_ea + (rc-1)*(width)+new_offset),\
                                *(unsigned int*)(shm_mem_ea + (rc-1)*(width)+new_offset));
                        break;
                    }
            case LS_BYTE: {
                        sprintf(msg_temp,"miscomparing dword(0x%lx)"
                                 " at dword Address(%p)\n\t\tActual misc Addr=%p, misc data byte=0x%x\n",\
                                *(unsigned long*)(shm_mem_ea + ((((rc-1)*(width))/MIN_PATTERN_SIZE)*MIN_PATTERN_SIZE)+new_offset),
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
            /*rand_debug*/
            srand48_r(local_ptr->th_seed,&local_ptr->buffer); 
            miscomp_rand_temp((local_ptr->seg_details[seg].shm_size/sd->width),local_ptr->seg_details[seg].shm_mem_ptr,sd);
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"dumped all necessary data,exiting\n");
            g_data.exit_flag = SET;
            pthread_exit((void *)0);
        }
        tmp_shm_size = local_ptr->seg_details[seg].shm_size - new_offset; /* Size of the remaining segment to be compared now */
        if (ps == PAGE_INDEX_16G ) {
            tmp_shm_size = tmp_shm_size % sd->sub_seg_size;
        }

        if (g_data.stanza_ptr->operation == OPER_STRIDE) {
            return miscompare_count;
        } else if ( g_data.stanza_ptr->operation != OPER_RIM ) {
            rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][COMPARE][mem_access_type_index])(
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
            rc = (*mem_wrc_fun[g_data.stanza_ptr->pattern_type[pi]][RIM][mem_access_type_index])(
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

/*rand_debug*/
void miscomp_rand_temp(unsigned long num_operations,void *seg_address,void *seg){
    unsigned int rand_no,temp_val;
    unsigned int *ptr   = (unsigned int*)seg_address;
    unsigned long i;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);
    char fname1[128],fname2[128],fname3[128];
    FILE* f2;
    unsigned int *rand_buf_ptr=NULL,*tptr;
    sprintf(fname2,"%s/hxemem.%s.2nd_RC__th_%lu__randbuf",g_data.htx_d.htx_exer_log_dir,g_data.stanza_ptr->rule_id,sd->owning_thread);
    sprintf(fname3,"%s/hxemem.%s.write_buf__th_%lu",g_data.htx_d.htx_exer_log_dir,g_data.stanza_ptr->rule_id,sd->owning_thread);
    displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s():write buffer dumped at %s and\n 2nd RC buffer containing generated randum numbers at %s\n",
            __LINE__,__FUNCTION__,fname2,fname3);
    
    rand_buf_ptr = (unsigned int*)malloc(sd->seg_size);
    if(rand_buf_ptr == NULL){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s(): malloc error (errno:%d) \n",__LINE__,__FUNCTION__,errno);
        exit(1);
    }
    tptr = rand_buf_ptr;
    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_32(&th->buffer);
        *tptr= rand_no;
        tptr++;
        if((i == th->rc_offset) || (i == (th->rc_offset - 1))){
            sprintf(fname1,"%s/hxemem.%s.2nd_RC__drand48_buf__word_%d__th_%lu__pass_%lu",g_data.htx_d.htx_exer_log_dir,g_data.stanza_ptr->rule_id,i,sd->owning_thread,th->rc_pass_count);
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s():2nd RC,drand48 structure content dumpbed in %s for word=%d\n",
                __LINE__,__FUNCTION__,fname1,i);
            unsigned char* p = (unsigned char*)&th->buffer;
            FILE *f;
            f = fopen(fname1,"w");
            if ( f == NULL) {
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s():Error opening %s file,errno:%d(%s),continuing without debug log file \n"
                ,__LINE__,__FUNCTION__,fname1,errno,strerror(errno));
            }
            for(int n=0; n < sizeof(th->buffer) ; ++n) {
                fprintf(f," %02x",*p++);
            }
            fprintf(f,"\n");
            fclose(f);
        }
    }
    hxfsbuf((char*)seg_address,sd->seg_size,fname3,&g_data.htx_d);
    hxfsbuf((char*)rand_buf_ptr,sd->seg_size,fname2,&g_data.htx_d);
    fflush(stdout);
    free(rand_buf_ptr);
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
