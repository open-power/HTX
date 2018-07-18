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
*/
#include "nest_framework.h"
#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )
static int cont_mem_chunks_indexes[MAX_HUGE_PAGES][2];
static unsigned char* prefetch_mem_chunks[MAX_CPUS_PER_CHIP] = {[0 ... (MAX_CPUS_PER_CHIP - 1)] = NULL} ;
static int cpus_track[MAX_CORES_PER_CHIP][MAX_CPUS_PER_CORE];
struct cache_exer_info cache_g;
extern char page_size_name[MAX_PAGE_SIZES][8];
extern struct nest_global g_data;
extern struct mem_exer_info mem_g;

void update_hxecache_stats(void);
int sort_huge_pages(void);
int fill_contiguous_chunks_details(void);
int identify_contiguous_mem_chunks(void);
int worstcase_mem_to_allocate_p9(void);
int worstcase_mem_to_allocate_p8(void);
int fill_cache_exer_thread_context(void);
int fill_prefetch_threads_context(void);
int derive_prefetch_algorithm_to_use(void);
int setup_prefetch_memory(void);
int find_cont_mem_cache_thread(int);
int fill_cache_threads_context(void);
void hexdump(FILE*,const unsigned char*,int);
int trap_function(void*,int,unsigned long long*);
void cache_write_dword(void*,int);
void cache_write_word(void*,int);
void cache_write_byte(void*,int );
int cache_read_byte(void*,int );
int cache_read_word(void*,int );
int cache_read_dword(void*,int );
int cache_read_compare_mem(int tn);
int cache_write_mem(int tn);
int create_cache_threads(void);
int create_prefetch_threads(void);
int create_threads(void);
int print_thread_data(void);
int modify_filters_for_undertest_cache_instance(void);

int fill_cache_exer_mem_req(){
    int rc = SUCCESS,chip=g_data.dev_id,chip_map_count,i;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    struct page_wise_seg_info *seg_details;
    unsigned long seg_size=0;
    int seg=0,num_segs=0;/*One seg of hugepage size and one segment with base pagesize */
    printf("coming in fill_cache_exer_mem_req()\n");

    /*Below logic is to map logical instance number to logical chip number of Nest FW structure having cpus in it, i.e
    to avoid mapping to chip having only memory in it(srad to PIR map)*/
	cache_g.cache_instance = g_data.dev_id;
    chip_map_count = -1;
    for(i=0;i<g_data.sys_details.num_chip_mem_pools;i++){
        if(mem_details_per_chip[i].num_cores > 0){
            chip_map_count++;
            if ( chip == chip_map_count){
                chip = i;
				cache_g.cache_instance = chip;
                break;
            }
        } 
    }

    /* If selected chip does not have memory, over write the structure and still go ahead n fill segment details for that chip*/
    if(mem_details_per_chip[chip].memory_details.total_mem_avail ==  0){
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"There is no mem behind chip:%d, cache instance may not get local memory!\n",chip);

        mem_details_per_chip[chip].has_cpu_and_mem = 1;
        for(i=0; i<MAX_PAGE_SIZES; i++) {
            mem_details_per_chip[chip].memory_details.pdata[i].supported = g_data.sys_details.memory_details.pdata[i].supported;
        }
    } 

    /*for prefetch operaton prefer 64K  over 4k pagesize backed memory(use any one og base pagesize)*/
    if(mem_details_per_chip[chip].memory_details.pdata[PAGE_INDEX_64K].supported){
        mem_details_per_chip[chip].memory_details.pdata[PAGE_INDEX_4K].supported = FALSE;
        g_data.sys_details.memory_details.pdata[PAGE_INDEX_4K].supported = FALSE;
    }

    switch(g_data.sys_details.true_pvr) {
        case POWER7:
        case POWER8_MURANO:
        case POWER8_VENICE:
        case POWER8P_GARRISION:
            rc = worstcase_mem_to_allocate_p8();
            if ( rc != SUCCESS){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:calculate_mem_requirement_for_p8 failed with %d\n", 
                    __LINE__,__FUNCTION__,rc);
                return(rc);
            }
            break;
        case POWER9_NIMBUS:
        case POWER9_CUMULUS:
            rc = worstcase_mem_to_allocate_p9();
            if ( rc != SUCCESS){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:calculate_mem_requirement_for_p9 failed with %d\n",
                    __LINE__,__FUNCTION__,rc);
                return(rc);
            }
            break;

        default:
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Error: Unknown PVR (0x%x). Exitting \n",
                __LINE__,__FUNCTION__,g_data.sys_details.true_pvr);
            return(FAILURE);
    }
    for(int pi=0;pi<MAX_PAGE_SIZES;pi++){
        /*if((!mem_details_per_chip[chip].memory_details.pdata[pi].supported) || (mem_details_per_chip[chip].memory_details.pdata[pi].free == 0)){*/
        /*checking system level page mem because even if under test chip does not have mem,cache exer should not fail*/
        if(!g_data.sys_details.memory_details.pdata[pi].supported || (g_data.sys_details.memory_details.pdata[pi].free == 0)){ 
            continue;
        }
        if((pi == PAGE_INDEX_2M) || (pi == PAGE_INDEX_16M)){
            num_segs = 1;
            seg_size = cache_g.worst_case_cache_memory_required;
            cache_g.cache_pages.huge_page_index = pi;
            if(g_data.sys_details.true_pvr < POWER8_MURANO){
                seg_size += cache_g.worst_case_prefetch_memory_required;
                cache_g.cache_pages.prefetch_page_index = pi;
            }
        }else if(pi < PAGE_INDEX_2M && g_data.sys_details.true_pvr >= POWER8_MURANO){
            num_segs = 1;
            seg_size = cache_g.worst_case_prefetch_memory_required;
            /*printf("cache_g.worst_case_prefetch_memory_required=%lu\n",cache_g.worst_case_prefetch_memory_required);*/
            cache_g.cache_pages.prefetch_page_index = pi;
        }else{
            continue;
        }
        mem_details_per_chip[chip].memory_details.pdata[pi].num_of_segments = num_segs;
        if(num_segs > 0){
            mem_details_per_chip[chip].memory_details.pdata[pi].page_wise_seg_data = (struct page_wise_seg_info*)malloc(num_segs * sizeof(struct page_wise_seg_info));
            if(mem_details_per_chip[chip].memory_details.pdata[pi].page_wise_seg_data == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno:%d(%s)for size=%llu\n",
                    __LINE__,__FUNCTION__,errno,strerror(errno),num_segs * sizeof(struct page_wise_seg_info));
                return(FAILURE);
            }
            seg_details = mem_details_per_chip[chip].memory_details.pdata[pi].page_wise_seg_data;
        }
        for(seg=0;seg<num_segs; seg++){
            seg_details[seg].shm_size = seg_size;
            seg_details[seg].original_shm_size = seg_size;
            seg_details[seg].seg_num   =   seg;
            seg_details[seg].shm_mem_ptr = NULL;
            seg_details[seg].shmid      = -1;
            displaym(HTX_HE_INFO,DBG_DEBUG_PRINT," chip:%dseg_num=%d:shm sizes[%u] = %lu (%s pages:%lu)\n",
                chip,seg_details[seg].seg_num,seg,seg_details[seg].shm_size,page_size_name[pi],
                (seg_details[seg].shm_size/mem_details_per_chip[chip].memory_details.pdata[pi].psize));
        }
        mem_g.total_segments += mem_details_per_chip[chip].memory_details.pdata[pi].num_of_segments;
    }
    return rc;
}
int sort_huge_pages(){
    int rc = SUCCESS;
    int i, j, min_index = -1;
    struct cache_mem_info* mem = &cache_g.cache_pages;    
    unsigned char *min_ea = mem->ea[0];
    unsigned long long  min, tmp;
    unsigned char       *tmp_ea;

    for(i = 0; i < (mem->num_pages) ; i++) {
        min = mem->ra[i];
        min_index = -1;
        for(j = i + 1; j < mem->num_pages; j++) {
            if(mem->ra[j] < min){
                min    = mem->ra[j];
                min_ea = mem->ea[j];
                min_index = j;
            }
        }
        if(min_index != -1) {
            /* swap real_addr array */
            tmp         = mem->ra[i];
            mem->ra[i]  = min;
            mem->ra[min_index] = tmp;
    
            /* swap effective addr array */
            tmp_ea      = mem->ea[i];
            mem->ea[i]  = min_ea;
            mem->ea[min_index] = (unsigned char *)tmp_ea;
        }
    }
    displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"After sorting addresses:\n");
    for(i = 0; i < (mem->num_pages) ; i++) {
        displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"page[%d]:EA = %p, RA = %lu\n",i,mem->ea[i],(unsigned long)mem->ra[i]);
    }
    return rc;
}

int fill_contiguous_chunks_details(){
    int rc = SUCCESS,p,i,j;
    struct cache_mem_info* mem = &cache_g.cache_pages;
    int next_page_distance,contig_mem_size = mem->huge_page_size;

    i=0;
    j=0;
    cache_g.num_cont_mem_chunks =   0;
    for(p=0; p<mem->num_pages; p++){
        next_page_distance = (mem->ra[p+1] - mem->ra[p]) * getpagesize() ;

        cache_g.cont_mem[i].huge_pg[j].page_num = p;
        cache_g.cont_mem[i].huge_pg[j++].ea     = mem->ea[p];
        cache_g.cont_mem[i].num_pages++;
        
        if ((next_page_distance == mem->huge_page_size) && (contig_mem_size < cache_g.contiguous_mem_required)){
            contig_mem_size += mem->huge_page_size;
            if(contig_mem_size >= cache_g.contiguous_mem_required){
                contig_mem_size = mem->huge_page_size;
                cache_g.cont_mem[i].huge_pg[j].page_num = (++p);
                cache_g.cont_mem[i].huge_pg[j].ea     = mem->ea[p];
                cache_g.cont_mem[i].num_pages++;
                i++;
                j=0;
            }
        }else{
            contig_mem_size = mem->huge_page_size;
            i++;
            j=0;
        }
    }
    cache_g.num_cont_mem_chunks = i;
    for(int ch=0;ch<cache_g.num_cont_mem_chunks;ch++){
            cont_mem_chunks_indexes[ch][0] = cache_g.cont_mem[ch].num_pages;
            cont_mem_chunks_indexes[ch][1] = ch;
    }
    return rc;
}
int identify_contiguous_mem_chunks(){
    int rc = SUCCESS;
    int num_conti_pages_rqd = (cache_g.contiguous_mem_required/cache_g.cache_pages.huge_page_size);
	if(cache_g.contiguous_mem_required % cache_g.cache_pages.huge_page_size){
		num_conti_pages_rqd++;
	}
    for(int ch=0;ch<MAX_HUGE_PAGES;ch++){
        for(int idx=0;idx<2;idx++){
           cont_mem_chunks_indexes[ch][idx] = -1;
        } 
    }
    cache_g.cont_mem    =  (struct contiguous_mem_info*)malloc(sizeof(struct contiguous_mem_info) * cache_g.cache_pages.num_pages * MEM_MULT_FACTOR);
    if(cache_g.cont_mem == NULL){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno:%d(%s)for size=%llu\n",
                __LINE__,__FUNCTION__,errno,strerror(errno),(sizeof(struct contiguous_mem_info) * cache_g.cache_pages.num_pages));
            return(FAILURE);
    }
    for(int i=0;i<cache_g.cache_pages.num_pages;i++){
        cache_g.cont_mem[i].in_use_mem_chunk = 0;
        cache_g.cont_mem[i].num_pages = 0;
        cache_g.cont_mem[i].huge_pg = (struct huge_page_info*)malloc(sizeof(struct huge_page_info) * num_conti_pages_rqd);
        if(cache_g.cont_mem[i].huge_pg == NULL){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno:%d(%s)for size=%llu, i=%d\n",
                __LINE__,__FUNCTION__,errno,strerror(errno),(sizeof(struct huge_page_info) * num_conti_pages_rqd),i);
            return(FAILURE);
        }
        for(int p=0;p<num_conti_pages_rqd;p++){
            cache_g.cont_mem[i].huge_pg[p].page_num = -1;
            cache_g.cont_mem[i].huge_pg[p].in_use   = 0;
            cache_g.cont_mem[i].huge_pg[p].ea = NULL;
        }
    }

    rc = fill_contiguous_chunks_details();
    if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_contiguous_chunks_details() failed with rc = %d\n",
            __LINE__,__FUNCTION__,rc);
        return(FAILURE);
    }
    
    /* sort indexes of contiguous mem chunks wrt to num of pages in each chunk into a 2D array*/
    int val,idx;
    for(int m=0;m<cache_g.num_cont_mem_chunks;m++){
        for(int n=m+1;n<cache_g.num_cont_mem_chunks;n++){
                 
            if(cont_mem_chunks_indexes[n][0] > cont_mem_chunks_indexes[m][0]){
                val = cont_mem_chunks_indexes[m][0];
                idx = cont_mem_chunks_indexes[m][1];
                cont_mem_chunks_indexes[m][0] = cont_mem_chunks_indexes[n][0];
                cont_mem_chunks_indexes[m][1] = cont_mem_chunks_indexes[n][1];
                cont_mem_chunks_indexes[n][0] = val; 
                cont_mem_chunks_indexes[n][1] = idx;
            }
        }
    }    

    for(int i=0;i<cache_g.num_cont_mem_chunks;i++){
        int k = cont_mem_chunks_indexes[i][1];
        if(cache_g.cont_mem[k].num_pages){
            displaym(HTX_HE_INFO,DBG_IMP_PRINT,"\nconti chunk:%2d(num_pages=%d)::",k,cache_g.cont_mem[k].num_pages);
            for(int p=0;p<num_conti_pages_rqd;p++){
                displaym(HTX_HE_INFO,DBG_IMP_PRINT,"[%2d]\t",cache_g.cont_mem[k].huge_pg[p].page_num);
            }
            if(cache_g.cont_mem[k].num_pages == num_conti_pages_rqd)
                displaym(HTX_HE_INFO,DBG_IMP_PRINT,"<========");
        }
    } 
    return rc;
}

int setup_memory_to_use(){
    int rc = SUCCESS,i=0;
    int cache_pi    = cache_g.cache_pages.huge_page_index;
    int prefetch_pi = cache_g.cache_pages.prefetch_page_index;
    unsigned char       *p, *addr;
    struct chip_mem_pool_info* chip_under_test  = &g_data.sys_details.chip_mem_pool_data[cache_g.cache_instance];
    struct cache_mem_info* mem = &cache_g.cache_pages;

    unsigned long   shm_size    = chip_under_test->memory_details.pdata[cache_pi].page_wise_seg_data[0].shm_size;
    int             page_size   = chip_under_test->memory_details.pdata[cache_pi].psize; 
    addr        = (unsigned char*)chip_under_test->memory_details.pdata[cache_pi].page_wise_seg_data[0].shm_mem_ptr;  
    cache_g.cache_pages.huge_page_size = page_size;

/*RVC put check if 2M/16M are > 0*/
    for(p=addr; p < addr+shm_size; p+=page_size) {
         mem->ea[i] = p;
        #ifdef __HTX_LINUX__
        find_EA_to_RA((unsigned long)mem->ea[i],&mem->ra[i]);
        #else
        getRealAddress(p, 0, &mem->ra[i]);
        #endif
        mem->page_status[i] = PAGE_FREE;
        displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"huge page(%s)[%d]:EA = %p, RA = %lu\n",page_size_name[cache_pi],i,mem->ea[i],(unsigned long)mem->ra[i]);
        mem->num_pages = ++i;
    }

    rc = sort_huge_pages();
    if ( rc != SUCCESS){
        displaym(DBG_MUST_PRINT,HTX_HE_HARD_ERROR,"[%d]%s:sort_huge_pages() failed with %d\n",
            __LINE__,__FUNCTION__,rc);
        return(rc);
    }
    rc = identify_contiguous_mem_chunks();
    if ( rc != SUCCESS){
        displaym(DBG_MUST_PRINT,HTX_HE_HARD_ERROR,"[%d]%s:dentify_contiguous_mem_chunks() failed with %d\n",
            __LINE__,__FUNCTION__,rc);
        return(rc);
    }
    cache_g.cache_pages.prefetch_ea = (unsigned char*)chip_under_test->memory_details.pdata[prefetch_pi].page_wise_seg_data[0].shm_mem_ptr;
    
    return rc;
}
int worstcase_mem_to_allocate_p9() {
    int rc = SUCCESS;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[cache_g.cache_instance];
    int fused_core_factor=1;

    if(DD1_FUSED_CORE == get_p9_core_type()){
        fused_core_factor = 2;
    }
    cache_g.max_possible_cache_threads      = (mem_details_per_chip->num_cores * fused_core_factor);
    cache_g.max_possible_prefetch_threads   = mem_details_per_chip->num_cpus;
	if(g_data.sys_details.memory_details.pdata[PAGE_INDEX_2M].supported){
    	cache_g.contiguous_mem_required             = g_data.sys_details.cache_info[L3].cache_size;
    	cache_g.worst_case_cache_memory_required    = (cache_g.contiguous_mem_required * cache_g.max_possible_cache_threads * MEM_MULT_FACTOR);
	}else if(g_data.sys_details.memory_details.pdata[PAGE_INDEX_16M].supported){
		if(cache_g.contiguous_mem_required < g_data.sys_details.memory_details.pdata[PAGE_INDEX_16M].psize){
			cache_g.contiguous_mem_required = g_data.sys_details.memory_details.pdata[PAGE_INDEX_16M].psize;
		}
		cache_g.worst_case_cache_memory_required    = (cache_g.contiguous_mem_required * cache_g.max_possible_cache_threads);
	}
    cache_g.worst_case_prefetch_memory_required = (cache_g.max_possible_prefetch_threads * (PREFETCH_MEMORY_PER_THREAD));
    
    
    return rc;
}

int worstcase_mem_to_allocate_p8(){
    int rc = SUCCESS;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[cache_g.cache_instance];

    cache_g.max_possible_cache_threads      = mem_details_per_chip->num_cores;
    cache_g.max_possible_prefetch_threads   = mem_details_per_chip->num_cpus;
    cache_g.contiguous_mem_required             = (2 * g_data.sys_details.cache_info[L3].cache_size);
    cache_g.worst_case_cache_memory_required    = (cache_g.contiguous_mem_required * cache_g.max_possible_cache_threads);
    cache_g.worst_case_prefetch_memory_required = (cache_g.max_possible_prefetch_threads * (PREFETCH_MEMORY_PER_THREAD));

    return rc;
}

int fill_cache_exer_thread_context(){
    int rc = SUCCESS,excluded_cores=0,pref_cpu = 0;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[cache_g.cache_instance];

    int fused_core_cache_thread = 0;
    if((DD1_FUSED_CORE == get_p9_core_type())&& ((g_data.sys_details.true_pvr == POWER9_NIMBUS) || (g_data.sys_details.true_pvr == POWER9_CUMULUS))){
        fused_core_cache_thread = 1;
    }
    if(g_data.stanza_ptr->cache_test_case == PREFETCH_ONLY){
        pref_cpu = 0;
    }else {
        pref_cpu = (1+fused_core_cache_thread);
    }
    g_data.stanza_ptr->num_cache_threads_to_create = 0;
    g_data.stanza_ptr->num_prefetch_threads_to_create = 0;

    for(int m=0;m<MAX_CORES_PER_CHIP;m++){
        for(int n=0;n<MAX_CPUS_PER_CORE;n++){
            cpus_track[m][n] = UNSET;
        }
    }
    for(int c=0; c<mem_details_per_chip->num_cores; c++){    
        if(mem_details_per_chip->inuse_core_array[c].num_procs > 0){
            if(g_data.stanza_ptr->cache_test_case != PREFETCH_ONLY){
				if((fused_core_cache_thread == 1) && (mem_details_per_chip->inuse_core_array[c].num_procs >= 2)){
                	g_data.stanza_ptr->num_cache_threads_to_create = (g_data.stanza_ptr->num_cache_threads_to_create + 1 + fused_core_cache_thread);
				}else {
					if(mem_details_per_chip->inuse_core_array[c].num_procs == 1){
						displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"[%d]%s:There is only 1 cpu found in core :%d ,cache "
							"rollover may not possible for  this core.\n",__LINE__,__FUNCTION__,c);
					}
					g_data.stanza_ptr->num_cache_threads_to_create = (g_data.stanza_ptr->num_cache_threads_to_create + 1);
				}
            }
        }else{
            excluded_cores++;
        }
        if((g_data.stanza_ptr->cache_test_case != CACHE_BOUNCE_ONLY) && (g_data.stanza_ptr->cache_test_case != CACHE_ROLL_ONLY &&
                (g_data.stanza_ptr->pf_conf != PREFETCH_OFF))){
            for(int cpu=pref_cpu; cpu<mem_details_per_chip->inuse_core_array[c].num_procs;cpu++){
                g_data.stanza_ptr->num_prefetch_threads_to_create++;
            }
        }
    }
    if(excluded_cores){
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s:Looks like %d number of cores are excluded in cpu_filter, "
            "cache operation will not perfomrmed for these cores\n",__LINE__,__FUNCTION__,excluded_cores);
    } 

    /*g_data.stanza_ptr->num_threads = (g_data.stanza_ptr->num_prefetch_threads_to_create + g_data.stanza_ptr->num_cache_threads_to_create);*/
    rc = allocate_mem_for_threads();
    if(rc != SUCCESS){
        return(FAILURE);
    }
    struct thread_data* th = &g_data.thread_ptr[0];
    struct cache_exer_thread_info *local_ptr = NULL;
    for(int i=0;i<g_data.stanza_ptr->num_threads;i++){
        local_ptr = &cache_g.cache_th_data[i];
        th[i].testcase_thread_details  = NULL;
        th[i].thread_num               = -1;
        th[i].tid                      = -1;
        th[i].kernel_tid               = -1;
        th[i].bind_proc                = -1;
        local_ptr->oper_type           = -1;
    }
    if(g_data.stanza_ptr->num_cache_threads_to_create > 0){
        rc = fill_cache_threads_context();
    }
    if((g_data.stanza_ptr->num_prefetch_threads_to_create > 0)){
        rc = fill_prefetch_threads_context();
    }
    if((g_data.stanza_ptr->num_prefetch_threads_to_create <= 0) && (g_data.stanza_ptr->num_cache_threads_to_create <= 0)){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: num of threads resulted into zero,cache threads=%d, prefect threads=%d,"
            " total_threads = %d\n",__LINE__,__FUNCTION__,g_data.stanza_ptr->num_cache_threads_to_create,
            g_data.stanza_ptr->num_prefetch_threads_to_create,g_data.stanza_ptr->num_threads);
        return FAILURE;
    }
    return rc;
}
int fill_prefetch_threads_context(){
    int thread_num=0,rc=SUCCESS,cpu=0,core=0;
    struct chip_mem_pool_info* chip_under_test  = &g_data.sys_details.chip_mem_pool_data[cache_g.cache_instance];
    struct thread_data* th = &g_data.thread_ptr[0];
    struct cache_exer_thread_info *local_ptr = NULL;
    int cache_threads_per_core = 1;
    int tot_threads = g_data.stanza_ptr->num_prefetch_threads_to_create + g_data.stanza_ptr->num_cache_threads_to_create;

    rc = setup_prefetch_memory();

    if((g_data.sys_details.true_pvr == POWER9_NIMBUS) || (g_data.sys_details.true_pvr == POWER9_CUMULUS)){
        if(DD1_FUSED_CORE == get_p9_core_type()){
            cache_threads_per_core = 2;
        }
       
    }
    if(g_data.stanza_ptr->cache_test_case == PREFETCH_ONLY){
        cache_threads_per_core = 0;
    }
    for(int t=0;t<g_data.stanza_ptr->num_prefetch_threads_to_create;t++){
        local_ptr = &cache_g.cache_th_data[thread_num];

        if(thread_num >= tot_threads){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: thread_num :%d exeeds max threads to be created count : %d\n",
             __LINE__,__FUNCTION__,thread_num,tot_threads);
            return FAILURE;
        }
        if(local_ptr->oper_type == OPER_CACHE){
/*printf("##coming here for thread %d\n",thread_num);*/
            thread_num++;
            t--;
            continue;
        }
        /*loop until it finds a in_use core with more than  "cache_threads_per_core" cpus,*/
/*printf("thread %d(%d),core_array[%d].num_procs=%d,cache_threads_per_core=%d,cores=%d\n",thread_num,t,core,chip_under_test->inuse_core_array[core].num_procs,cache_threads_per_core,chip_under_test->inuse_num_cores);*/
        while((chip_under_test->inuse_core_array[core].num_procs <= cache_threads_per_core) && (g_data.stanza_ptr->cache_test_case != PREFETCH_ONLY)){
            core++;
            if(core >= chip_under_test->inuse_num_cores){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: thread:%d could not find sufficient cpus in any cores:%d,pref threads to create=%d\n", 
                    __LINE__,__FUNCTION__,thread_num,core,g_data.stanza_ptr->num_prefetch_threads_to_create);
                return FAILURE;
            }
        } 
        while(cpus_track[core][cpu] == SET){
            cpu = (cpu+1);
            if(cpu >= chip_under_test->inuse_core_array[core].num_procs){
                /*displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s:Could not find any cpu for thread :%d for prefetch opertaion,cpus in core(%d)=%d\n",
                    __LINE__,__FUNCTION__,thread_num,core,chip_under_test->inuse_core_array[core].num_procs);*/
                cpu=0;
                core++;
            }
            if(core >= chip_under_test->inuse_num_cores){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: thread:%d could not find free cpus in any cores:%d,pref threads to create=%d\n", 
                    __LINE__,__FUNCTION__,thread_num,core,g_data.stanza_ptr->num_prefetch_threads_to_create);
                return FAILURE;
            }
        }
        th[thread_num].testcase_thread_details  = (void *)local_ptr;
        cpus_track[core][cpu] = SET;
        th[thread_num].thread_num               = thread_num;
        th[thread_num].bind_proc                = chip_under_test->inuse_core_array[core].lprocs[cpu];
        local_ptr->oper_type                    = OPER_PREFETCH;
        local_ptr->prefetch_algorithm           = derive_prefetch_algorithm_to_use();

        char* pat_ptr   = (char *)&local_ptr->pattern;
        for (int k=0; k<sizeof(local_ptr->pattern); k++) {
            *pat_ptr++ = (thread_num+1);
        }
        local_ptr->prefetch_memory_start_address = prefetch_mem_chunks[t];
        local_ptr->prefetch_streams            = MAX_PREFETCH_STREAMS / (chip_under_test->inuse_core_array[core].num_procs - cache_threads_per_core);

        thread_num++;
    }
    return rc;
}
int derive_prefetch_algorithm_to_use(){

    int rc      = FAILURE;
    int pf_conf = g_data.stanza_ptr->pf_conf;
    static int  prefetch_algo_counter = 0;

    int prefetch_algorithms[MAX_PREFETCH_ALGOS] = { PREFETCH_IRRITATOR , PREFETCH_NSTRIDE ,
                                                    PREFETCH_PARTIAL, PREFETCH_TRANSIENT , PREFETCH_NA };
    struct chip_mem_pool_info* chip_under_test  = &g_data.sys_details.chip_mem_pool_data[cache_g.cache_instance];

    int     pf_algo_to_use;

    if ( pf_conf == PREFETCH_OFF ) {
        return PREFETCH_OFF;
    }
    for(int c=0;c<chip_under_test->inuse_num_cores;c++){
        if(chip_under_test->inuse_core_array[c].num_procs < 4){
            return RR_ALL_ENABLED_PREFETCH_ALGORITHMS;
        }
    }
    while ( 1 ) {
        pf_algo_to_use = pf_conf & prefetch_algorithms[ prefetch_algo_counter++ % MAX_PREFETCH_ALGOS ];
        if ( pf_algo_to_use ) {
            return pf_algo_to_use;
        }
    }
    return rc;
}
int setup_prefetch_memory(){
    int rc=SUCCESS,t=0;
    int pref_threads = g_data.stanza_ptr->num_prefetch_threads_to_create;

    for(int i=0;i<MAX_CPUS_PER_CHIP;i++){
        prefetch_mem_chunks[i]=NULL;
    }
    if(g_data.sys_details.true_pvr >= POWER8_MURANO){
        for(int i=0;i<pref_threads;i++){
            prefetch_mem_chunks[i]= (unsigned char*)(cache_g.cache_pages.prefetch_ea + (PREFETCH_MEMORY_PER_THREAD * i));
        }
    }else{
        for(int c=0;c<cache_g.num_cont_mem_chunks;c++){
            if(cache_g.cont_mem[c].in_use_mem_chunk  == 0){
                for(int p=0;p<cache_g.cont_mem[c].num_pages;p++){
                    if((cache_g.cont_mem[c].huge_pg[p].in_use == 0) && (t < pref_threads)){
                        prefetch_mem_chunks[t++]  = (unsigned char*)cache_g.cont_mem[c].huge_pg[p].ea;
                        if(t >= pref_threads)break;
                        prefetch_mem_chunks[t++]= (unsigned char*)(cache_g.cont_mem[c].huge_pg[p].ea + PREFETCH_MEMORY_PER_THREAD);
                        cache_g.cont_mem[c].huge_pg[p].in_use = 1;
                    }             
                }
                cache_g.cont_mem[c].in_use_mem_chunk  = 1;
                if(t >= pref_threads)break;
            }
        }
    }
    
    return rc;
}
int find_cont_mem_cache_thread(int t){
    static unsigned long print_count = 0;
    int print_flag,pt=0;
    struct thread_data* th = &g_data.thread_ptr[t];
    struct cache_exer_thread_info *local_ptr = &cache_g.cache_th_data[t];

    print_count++;
    if(print_count <= (g_data.stanza_ptr->num_cache_threads_to_create+1)){/*RVC*/
        print_flag =1;
    }
    for(int c=0;c<cache_g.num_cont_mem_chunks;c++){
        int k = cont_mem_chunks_indexes[c][1];
        if(cache_g.cont_mem[k].in_use_mem_chunk  == 0){
            if(cache_g.cont_mem[k].num_pages >= local_ptr->num_mem_sets){
                cache_g.cont_mem[k].in_use_mem_chunk = 1;
                for(int p=0;p<local_ptr->num_mem_sets;p++){
                    local_ptr->contig_mem[p] = cache_g.cont_mem[k].huge_pg[p].ea;
                    cache_g.cont_mem[k].huge_pg[p].in_use = 1;
                }
                local_ptr->found_cont_pages = 1;
                return SUCCESS;
            }else{
                for(int p=0;p<cache_g.cont_mem[k].num_pages;p++){
                    if(cache_g.cont_mem[k].huge_pg[p].in_use == 0){
                        local_ptr->contig_mem[pt++] = cache_g.cont_mem[k].huge_pg[p].ea;
                        cache_g.cont_mem[k].huge_pg[p].in_use = 1;
                        if(pt == local_ptr->num_mem_sets){
                            if(!print_flag){
                                displaym(HTX_HE_INFO,DBG_MUST_PRINT,"cache thread %d,could not find %d num of contiguous pages, it will "
                                "run with non contiguous pages\n",th[t].thread_num,local_ptr->num_mem_sets);
                            }
                            return SUCCESS;
                        }
                    }
                }
                cache_g.cont_mem[k].in_use_mem_chunk = 1;
            }
        }
    }

    /*will reach here only if thread could not find desired pages */
    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: Unable to update thread %d with desired huge pages(%d),something went wrong, "
        "total chunks:%d,\n",__LINE__,__FUNCTION__,t,local_ptr->num_mem_sets,cache_g.num_cont_mem_chunks);
    return (FAILURE);
}
int fill_cache_threads_context(){
    int thread_num=0,rc=SUCCESS,cpu=0,core=0,single_thread=0;
    struct chip_mem_pool_info* chip_under_test  = &g_data.sys_details.chip_mem_pool_data[cache_g.cache_instance];
    struct thread_data* th = &g_data.thread_ptr[0];
    struct cache_exer_thread_info *local_ptr = NULL;
    int fused_core_flag = 0,p9_mult_factor = 2,mem_per_thread;
    int core_end_index = -1,walk_2_into_cores=0;/*to handle imbalanced core cpu_filter config for fused core mode*/

    if((g_data.sys_details.true_pvr == POWER9_NIMBUS) || (g_data.sys_details.true_pvr == POWER9_CUMULUS)){
        if(DD1_FUSED_CORE == get_p9_core_type()){
            fused_core_flag = 1;
        }
        p9_mult_factor = 1;
    }
    for(int t=0,j=0;t<g_data.stanza_ptr->num_cache_threads_to_create;t++,j++){
        local_ptr = &cache_g.cache_th_data[thread_num];
        if(local_ptr->oper_type == OPER_PREFETCH){
            continue;
        }
        if((fused_core_flag == 1) && (core == chip_under_test->inuse_num_cores)){
            core = 0;      
            cpu = (chip_under_test->inuse_core_array[core].num_procs/2);
			if(chip_under_test->inuse_core_array[0].num_procs == 1 ){/* Take care of thread_num if only cpu is enabled in the core*/
				single_thread = 1;
			}else{
				thread_num = chip_under_test->inuse_core_array[0].num_procs/2;
			}
        	local_ptr = &cache_g.cache_th_data[thread_num];
            walk_2_into_cores = 1;
            core_end_index = -1;
        }
		if(single_thread != 1) {
				th[thread_num].testcase_thread_details  = (void *)local_ptr; 
				th[thread_num].thread_num               = thread_num;
				th[thread_num].bind_proc                = chip_under_test->inuse_core_array[core].lprocs[cpu];
				if(cpus_track[core][cpu] == UNSET){
					cpus_track[core][cpu] = SET;
				}
				local_ptr->oper_type                    = OPER_CACHE;
				local_ptr->start_class                  = 0;
				local_ptr->found_cont_pages             = 0;

				char* pat_ptr   = (char *)&local_ptr->pattern;
				for (int k=0; k<sizeof(local_ptr->pattern); k++) {
					*pat_ptr++ = (thread_num+1);
				}
				for(int p=0;p<MAX_HUGE_PAGES_PER_CACHE_THREAD;p++){
					 local_ptr->contig_mem[p] = NULL;
				}
				switch (g_data.stanza_ptr->cache_test_case){
					case CACHE_BOUNCE_ONLY:
					case CACHE_BOUNCE_WITH_PREF:
						mem_per_thread = (g_data.sys_details.cache_info[L2].cache_line_size/g_data.stanza_ptr->num_cache_threads_to_create);
						local_ptr->walk_class_jump = 1;
						local_ptr->offset_within_cache_line = (mem_per_thread * j);
						local_ptr->num_mem_sets = 1;

						break;
					case CACHE_ROLL_ONLY:
					case CACHE_ROLL_WITH_PREF:
						local_ptr->walk_class_jump  =  1;
						local_ptr->end_class        =  (p9_mult_factor * g_data.sys_details.cache_info[g_data.stanza_ptr->tgt_cache].cache_associativity) - 1;    
						local_ptr->num_mem_sets     =  (cache_g.contiguous_mem_required/cache_g.cache_pages.huge_page_size);
						if(cache_g.contiguous_mem_required % cache_g.cache_pages.huge_page_size){
							local_ptr->num_mem_sets++;
						}
						local_ptr->offset_within_cache_line = 0;
						break;
				}
				rc = find_cont_mem_cache_thread(thread_num);
				if(rc != SUCCESS){
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: fill_thread_mem_details() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
					return(FAILURE);
				}
			}
        /*printf("cache threads: %d  cpu = %d\n",thread_num,th[thread_num].bind_proc);*/
        if(walk_2_into_cores){/*wlaking 2nd time into cores for identifying 2nd cache thread index  for p9 fused core config*/
			single_thread = 0;
        	core_end_index += (chip_under_test->inuse_core_array[core].num_procs);
        	core++;
			if(chip_under_test->inuse_core_array[core].num_procs == 2){/* if cpus = 2, generate thread_num differntly to avoid overwriting first cache thread*/
				thread_num = (core_end_index + chip_under_test->inuse_core_array[core].num_procs);
			}else if(chip_under_test->inuse_core_array[core].num_procs > 2){
            	thread_num = (core_end_index + (chip_under_test->inuse_core_array[core].num_procs/2));
			}else{/*There is only one cpu in the core*/
				single_thread = 1;
			}
        }else{
            thread_num += (chip_under_test->inuse_core_array[core].num_procs); 
        	core++;
        }
    }    

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
int dump_miscompare_data(int tn,void *addr) {
    int rc = SUCCESS,page_found=FALSE;
    struct thread_data* th = &g_data.thread_ptr[tn];
    struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[tn]);
    char dump_file_name[256];
    FILE *dump_fp;
    int huge_page_size = cache_g.cache_pages.huge_page_size;

    strcpy(dump_file_name,g_data.htx_d.htx_exer_log_dir);
    sprintf(dump_file_name,"%s/hxecache_miscompare.%d.%d.%lx",dump_file_name,g_data.dev_id,tn,th->tid);
    dump_fp = fopen(dump_file_name,"w");

    if ( dump_fp == NULL ) {
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:Could not create the dump file. errno = %d(%s)\n",__LINE__,__FUNCTION__,errno,strerror(errno));
        return (FAILURE);
    }else{
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT," Miscompare details are dumped into dump file %s\n",dump_file_name);
    }

    fprintf(dump_fp,"#############################################################################################\n");
    fprintf(dump_fp,"####################          Hxecache Miscompare Dump File            ######################\n");
    fprintf(dump_fp,"#############################################################################################\n");

    fprintf(dump_fp,"Exerciser instance             = %d\n",cache_g.cache_instance);
    fprintf(dump_fp,"Thread number                  = 0x%x\n",tn);
    fprintf(dump_fp,"Bind to CPU                    = %d\n",th->bind_proc);
    fprintf(dump_fp,"Pattern                        =0x%llx\n",local_ptr->pattern);
    fprintf(dump_fp,"Current loop number            = %d\n",th->current_num_oper);
    fprintf(dump_fp,"Thread type                    = %d\n",local_ptr->oper_type);
    fprintf(dump_fp,"Prefetch algorithm             = %d\n",local_ptr->prefetch_algorithm);
    if (local_ptr->oper_type == 2){
        fprintf(dump_fp,"DSCR value written             = %llx\n",local_ptr->written_dscr_val);
        fprintf(dump_fp,"DSCR value read back           = %llx\n",local_ptr->read_dscr_val);
    }
    fprintf(dump_fp,"Dumping the HUGE page where miscompare occurred\n\n");
    fprintf(dump_fp,"contiguous memory pages address for thread %d::\n",tn);
    for(int p=0;p<local_ptr->num_mem_sets;p++){
        fprintf(dump_fp,"[%d]%p\n",p,local_ptr->contig_mem[p]); 
        unsigned char *current_page_start_addr    = (unsigned char *)local_ptr->contig_mem[p];
        unsigned char *current_page_end_addr      = (unsigned char *)(local_ptr->contig_mem[p] + huge_page_size);  
        if (((unsigned char *)addr >=current_page_start_addr) && ((unsigned char *)addr <= current_page_end_addr)){
            hexdump(dump_fp,(unsigned char *)current_page_start_addr,huge_page_size);
            page_found = TRUE;
        }
        
    }

    if(g_data.sys_details.true_pvr >= POWER8_MURANO){
        if ( (unsigned char *) addr >= (unsigned char*)cache_g.cache_pages.prefetch_ea && (unsigned char *) addr <= 
            (unsigned char*)(cache_g.cache_pages.prefetch_ea + g_data.stanza_ptr->num_prefetch_threads_to_create*PREFETCH_MEMORY_PER_THREAD)){
            hexdump(dump_fp,(unsigned char *)addr, 512) ;
            page_found = TRUE;
        }
    }
    
    if (page_found == FALSE) {
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:Error : The miscomparing address %p was not found in allocated memory range.\n",__LINE__,__FUNCTION__,addr);
        fflush(dump_fp);
        fclose(dump_fp);
        return (FAILURE);
    }
    fflush(dump_fp);
    fclose(dump_fp);
    return rc;
}
int trap_function(void *addr,int tn,unsigned long long *pattern_ptr){
    struct thread_data* th = &g_data.thread_ptr[tn];
    int rc = SUCCESS;
    int trap_flag;
    if( (g_data.stanza_ptr->misc_crash_flag) && (g_data.kdb_level) )
        trap_flag = 1;    /* Trap is set */
    else
        trap_flag = 0;    /* Trap is not set */

    if(g_data.stanza_ptr->attn_flag)
        trap_flag = 2;    /* attn is set */
    if(trap_flag){
#ifdef __HTX_LINUX__
        do_trap_htx64((unsigned long)0xBEEFDEAD,(unsigned long) addr,(unsigned long) pattern_ptr,
            (unsigned long)tn,(unsigned long)&g_data.thread_ptr[tn],(unsigned long)&cache_g,
            (unsigned long)&g_data, (unsigned long)&g_data.stanza_ptr);
#else
        trap(0xBEEFDEAD,addr,pattern_ptr,tn,&g_data.thread_ptr[tn],&cache_g,&g_data,&g_data.stanza_ptr);
#endif
    }else{
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]thread:%d bound to cpu %d miscompared!!,EA = %p,data= %llx,pattern = %llx,rule id = %s\n",
            __LINE__,tn,th->bind_proc,addr,*((unsigned long long*)addr),*((unsigned long long *)pattern_ptr),g_data.stanza_ptr->rule_id);
        dump_miscompare_data(tn,addr);
        return FAILURE;
    }
    return rc;
}
void cache_write_dword(void *addr,int tn )
{
    struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[tn]);
    char               *pat_ptr = (char *)&local_ptr->pattern;
    unsigned long long *ptr  = (unsigned long long *)addr;

    *ptr = *(unsigned long long *)pat_ptr;

/*unsigned long long temp = (volatile unsigned long long *)*ptr;
   temp = temp * tn;*/
}

void cache_write_word(void *addr,int tn )
{
    struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[tn]);
    char               *pat_ptr = (char *)&local_ptr->pattern;
    unsigned int *ptr  = (unsigned int*)addr;

    *ptr = *(unsigned int *)pat_ptr;
}
void cache_write_byte(void *addr,int tn )
{
    struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[tn]);
    char               *pat_ptr = (char *)&local_ptr->pattern;
    unsigned char *ptr  = (unsigned char*)addr;

    *ptr = *(unsigned char *)pat_ptr;
}

int cache_read_byte(void *addr,int tn )
{
    int rc = SUCCESS;
    struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[tn]);
    char                *pattern_ptr    = (char *) &local_ptr->pattern;
    unsigned char *mem_addr_ptr   = (unsigned char*)addr;

    if( *(unsigned char*)pattern_ptr != *mem_addr_ptr) {
            rc = trap_function(addr,tn,(void*)pattern_ptr);
            return rc;
    }

    return rc;
}
int cache_read_word(void *addr,int tn )
{
    int rc = SUCCESS;
    struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[tn]);
    char                *pattern_ptr    = (char *) &local_ptr->pattern;
    unsigned int *mem_addr_ptr   = (unsigned int*)addr;

    if( *(unsigned int*)pattern_ptr != *mem_addr_ptr) {
            rc = trap_function(addr,tn,(void*)pattern_ptr);
            return rc;
    }

    return rc;
}
int cache_read_dword(void *addr,int tn )
{
    int rc = SUCCESS;
    struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[tn]);
    char                *pattern_ptr    = (char *) &local_ptr->pattern;
    unsigned long long  *mem_addr_ptr   = (unsigned long long*)addr;

    if( *(unsigned long long *)pattern_ptr != *mem_addr_ptr) {
            rc = trap_function(addr,tn,(void*)pattern_ptr);
            return rc;
    }
    return rc;
}


int cache_read_compare_mem(int tn){
    struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[tn]);
    int             walk_class, walk_line, memory_per_set, lines_per_set;
    int             data_width, rc = SUCCESS;
    unsigned int    offset1, offset2 , offset_sum;
    unsigned char   *addr = NULL;
    int             ct,current_line_size,current_asc,p9_mult_factor=2;
    int             huge_page_size = cache_g.cache_pages.huge_page_size;
    if((g_data.sys_details.true_pvr == POWER9_NIMBUS) || (g_data.sys_details.true_pvr == POWER9_CUMULUS)){
        p9_mult_factor = 1;
    }

    ct                  = g_data.stanza_ptr->tgt_cache;
    current_line_size   = g_data.sys_details.cache_info[ct].cache_line_size;
    current_asc         = g_data.sys_details.cache_info[ct].cache_associativity;
    data_width          = g_data.stanza_ptr->width;
    memory_per_set      = g_data.sys_details.cache_info[ct].cache_size/current_asc;
    lines_per_set       = memory_per_set/current_line_size;

    if (g_data.stanza_ptr->target_set != -1) {
        if(g_data.stanza_ptr->target_set > ((p9_mult_factor * current_asc ) - 1)) {
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"Specified target class-%d is out of range-%d \n",
                g_data.stanza_ptr->target_set,((2 * current_asc ) - 1));
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"limitting class to %d \n", (( p9_mult_factor * current_asc ) - 1));
        }else{
            local_ptr->start_class = local_ptr->end_class = g_data.stanza_ptr->target_set;
        }
    }
    /*
     * The external loop targets line in set.
     * Offset1 decides the line in set.
     */
    for(walk_line = 0; walk_line < lines_per_set; walk_line++) {
        offset1 = walk_line*current_line_size + local_ptr->offset_within_cache_line;
        for(walk_class = local_ptr->start_class; walk_class <= local_ptr->end_class; walk_class +=local_ptr->walk_class_jump){
            offset2 = walk_class*memory_per_set;
            offset_sum      = offset1 + offset2;

            addr = local_ptr->contig_mem[offset_sum/huge_page_size] + (offset_sum%(huge_page_size));

            switch(data_width) {
                case LS_DWORD:
                    rc = cache_read_dword(addr,tn);
                    break;

                case LS_WORD:
                    rc = cache_read_word(addr,tn);
                    break;

                case LS_BYTE:
                    rc = cache_read_byte(addr,tn);
                    break;

                default:
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d] Unknown Data width ( %d ). Exitting\n",__LINE__,data_width);
                    return (FAILURE);
            }
            if(rc != SUCCESS){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:miscompare found!\n",__LINE__,__FUNCTION__);
                return rc;
            } 
        }
    }
    return rc;
}
int cache_write_mem(int tn){
    struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[tn]);
    int             walk_class, walk_line, memory_per_set, lines_per_set, ct, current_line_size;
    int             data_width;
    unsigned char   *addr = NULL;
    unsigned int    offset1, offset2;
    int             rc = SUCCESS;
    int             current_asc;
    int             offset_sum;
    int             p9_mult_factor=2;
    int             huge_page_size = cache_g.cache_pages.huge_page_size;
    if((g_data.sys_details.true_pvr == POWER9_NIMBUS) || (g_data.sys_details.true_pvr == POWER9_CUMULUS)){
        p9_mult_factor = 1;
    }

    ct                  = g_data.stanza_ptr->tgt_cache;
    current_line_size   = g_data.sys_details.cache_info[ct].cache_line_size;
    current_asc         = g_data.sys_details.cache_info[ct].cache_associativity;
    data_width          = g_data.stanza_ptr->width;
    memory_per_set      = g_data.sys_details.cache_info[ct].cache_size/current_asc;
    lines_per_set       = memory_per_set/current_line_size;

    if (g_data.stanza_ptr->target_set != -1) {
        if(g_data.stanza_ptr->target_set > ((p9_mult_factor * current_asc ) - 1)) {
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"Specified target class-%d is out of range-%d \n",
                g_data.stanza_ptr->target_set,((2 * current_asc ) - 1)); 
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"limitting class to %d \n", (( p9_mult_factor * current_asc ) - 1));
        }else{
            local_ptr->start_class = local_ptr->end_class = g_data.stanza_ptr->target_set;
        }
    }
    /*
     * The external loop targets line in set.
     * Offset1 decides the line in set.
     */
    for(walk_line = 0; walk_line < lines_per_set; walk_line++) {
        offset1 = walk_line*current_line_size + local_ptr->offset_within_cache_line;
        for(walk_class = local_ptr->start_class; walk_class <= local_ptr->end_class; walk_class +=local_ptr->walk_class_jump){
            offset2 = walk_class*memory_per_set;
            offset_sum      = offset1 + offset2;

            addr = local_ptr->contig_mem[offset_sum/huge_page_size] + (offset_sum%(huge_page_size));

            switch(data_width) {
                case LS_DWORD:
                    cache_write_dword(addr,tn);
                    break;

                case LS_WORD:
                    cache_write_word(addr,tn);
                    break; 

                case LS_BYTE:
                    cache_write_byte(addr,tn);
                    break;

                default:
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d] Unknown Data width ( %d ). Exitting\n",__LINE__,data_width);
                    return (FAILURE);
            }
        }
    }
    return rc;
}
void* cache_thread_function(void *tn){
    int rc=SUCCESS,nw=0,nrc=0;
    struct thread_data* th = (struct thread_data*)tn;   
    int oper,num_oper = g_data.stanza_ptr->num_oper;

    if(!g_data.stanza_ptr->disable_cpu_bind){
        rc = htx_bind_thread(th->bind_proc,-1);

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
            pthread_exit((void *)1);
        #endif
        }
    }

    rc = SUCCESS;
    for(oper = 0;((g_data.exit_flag != SET) && (oper < num_oper));oper++){
        nw = g_data.stanza_ptr->num_writes;
        nrc = g_data.stanza_ptr->num_compares;
        th->current_num_oper = oper;
        while((nw>0 || nrc>0) && (rc == SUCCESS)){
            if(nw>0){
                cache_write_mem(th->thread_num);
                nw--;
            }
            if(nrc > 0){
                rc = cache_read_compare_mem(th->thread_num);
                nrc--;
            }
        }

        if(rc != SUCCESS){
			break;
        }
    }
    htx_unbind_thread();
    pthread_exit((void *)1);
}
int create_cache_threads(){
    int thread_num=0,rc=SUCCESS,core=0,single_thread=0;
    struct chip_mem_pool_info* chip_under_test  = &g_data.sys_details.chip_mem_pool_data[cache_g.cache_instance];
    struct thread_data* th = &g_data.thread_ptr[0];
    int fused_core_flag = 0;
    int core_end_index = -1,walk_2_into_cores=0;/*to handle imbalanced core cpu_filter config for fused core mode*/
    if((g_data.sys_details.true_pvr == POWER9_NIMBUS) || (g_data.sys_details.true_pvr == POWER9_CUMULUS)){
        if(DD1_FUSED_CORE == get_p9_core_type()){
            fused_core_flag = 1;
        }
    }
    void *tresult;
    for(int t=0;t<g_data.stanza_ptr->num_cache_threads_to_create;t++){
        if((fused_core_flag == 1) && (core == chip_under_test->inuse_num_cores)){
            core = 0;
            if(chip_under_test->inuse_core_array[0].num_procs == 1 ){/* Take care of thread_num if only cpu is enabled in the core*/
                single_thread = 1;
            }else{
            	thread_num = chip_under_test->inuse_core_array[0].num_procs/2;
			}
            walk_2_into_cores = 1;
            core_end_index = -1;
        }
		if(single_thread != 1){
			rc = pthread_create((pthread_t *)&th[thread_num].tid,NULL,cache_thread_function,&th[thread_num]);
			if(rc != 0){
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_create failed with rc = %d (errno %d):(%s) for thread_num = %d\n",
					__LINE__,__FUNCTION__,rc,errno,strerror(errno),thread_num);
				return(FAILURE);
			}
			/*else{
				printf("th %d (%d)created..\n",th[thread_num].thread_num,thread_num);
			}*/
		}
        if(walk_2_into_cores){/*wlaking 2nd time into cores for identifying 2nd cache thread index  for p9 fused core config*/
			single_thread = 0;
        	core_end_index += (chip_under_test->inuse_core_array[core].num_procs);
        	core++;
			if(chip_under_test->inuse_core_array[core].num_procs == 2){/* if cpus <= 2, generate thread_num differntly to avoid overwriting first cache thread*/
				thread_num = (core_end_index + chip_under_test->inuse_core_array[core].num_procs);
			}else if(chip_under_test->inuse_core_array[core].num_procs > 2){
            	thread_num = (core_end_index + (chip_under_test->inuse_core_array[core].num_procs/2));
            }else{/*There is only one cpu in the core*/
                single_thread = 1;
            }			
        }else{
            thread_num += (chip_under_test->inuse_core_array[core].num_procs); 
			core++;
        }
    }

    thread_num=0;
    core = 0;
	single_thread = 0;
    walk_2_into_cores = 0;
    core_end_index = -1;
    for(int t=0;t<g_data.stanza_ptr->num_cache_threads_to_create;t++){
        if((fused_core_flag == 1) && (core == chip_under_test->inuse_num_cores)){
            core = 0;
            if(chip_under_test->inuse_core_array[0].num_procs == 1 ){/* Take care of thread_num if only cpu is enabled in the core*/
                single_thread = 1;
            }else{
                thread_num = chip_under_test->inuse_core_array[0].num_procs/2;
            }
            walk_2_into_cores = 1;
            core_end_index = -1;
        }
        if(single_thread != 1){
			rc = pthread_join(th[thread_num].tid,&tresult);
			if(rc != 0){
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join with rc = %d (errno %d):(%s) for thread_num = %d \n",
					__LINE__,__FUNCTION__,rc,errno,strerror(errno),thread_num);
				return(FAILURE);
			}
			/*else{
				printf("## th %d (%d)Joined..\n",th[thread_num].thread_num,thread_num);
			}*/
			th[thread_num].thread_num   = -1;
			th[thread_num].tid          = -1;
			th[thread_num].kernel_tid   = -1;
			th[thread_num].bind_proc    = -1;
		}
        if(walk_2_into_cores){/*wlaking 2nd time into cores for identifying 2nd cache thread index  for p9 fused core config*/
			single_thread = 0;
        	core_end_index += (chip_under_test->inuse_core_array[core].num_procs);
        	core++;
			if(chip_under_test->inuse_core_array[core].num_procs == 2){/* if cpus <= 2, generate thread_num differntly to avoid overwriting first cache thread*/
				thread_num = (core_end_index + chip_under_test->inuse_core_array[core].num_procs);
			}else if (chip_under_test->inuse_core_array[core].num_procs > 2){
                thread_num = (core_end_index + (chip_under_test->inuse_core_array[core].num_procs/2));
            }else{/*There is only one cpu in the core*/
				single_thread = 1;
			}
        }else{
            thread_num += (chip_under_test->inuse_core_array[core].num_procs); 
        	core++;
        }
    }

    return rc;
}
int create_prefetch_threads(){
    int thread_num=0,rc=SUCCESS;
    struct thread_data* th = &g_data.thread_ptr[0];
    struct cache_exer_thread_info *local_ptr = NULL;
    int tot_threads = g_data.stanza_ptr->num_prefetch_threads_to_create + g_data.stanza_ptr->num_cache_threads_to_create;

    void *tresult;
    for(int t=0;t<g_data.stanza_ptr->num_prefetch_threads_to_create;t++){
        local_ptr = &cache_g.cache_th_data[thread_num];
        if(thread_num >= tot_threads){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: thread_num :%d exeeds max threads to be created count : %d\n",
             __LINE__,__FUNCTION__,thread_num,tot_threads);
            return FAILURE;
        }
        if(local_ptr->oper_type == OPER_CACHE){
            thread_num++;
            t--;
            continue;
        }
        rc = pthread_create((pthread_t *)&th[thread_num].tid,NULL,prefetch_thread_function,&th[thread_num]);
        if(rc != 0){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_create failed with rc = %d (errno %d):(%s) for thread_num = %d\n",
                __LINE__,__FUNCTION__,rc,errno,strerror(errno),thread_num);
            return(FAILURE);
        }
        /*else{
            printf(" prefetch, th %d (%d)created..\n",th[thread_num].thread_num,thread_num);
        }*/
        thread_num++;   

    }

    thread_num=0;
    for(int t=0;t<g_data.stanza_ptr->num_prefetch_threads_to_create;t++){
        local_ptr = &cache_g.cache_th_data[thread_num];
        if(thread_num >= tot_threads){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: thread_num :%d exeeds max threads to be created count : %d\n",
             __LINE__,__FUNCTION__,thread_num,tot_threads);
            return FAILURE;
        }
        if(local_ptr->oper_type == OPER_CACHE){
            thread_num++;
            t--;
            continue;
        }
        rc = pthread_join(th[thread_num].tid,&tresult);
        if(rc != 0){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join with rc = %d (errno %d):(%s) for thread_num = %d \n",
                __LINE__,__FUNCTION__,rc,errno,strerror(errno),thread_num);
            return(FAILURE);
        }
        /*else{
            printf("##th %d (%d)Joined..\n",th[thread_num].thread_num,thread_num);
        }*/
        th[thread_num].thread_num   = -1;
        th[thread_num].tid          = -1;
        th[thread_num].kernel_tid   = -1;
        th[thread_num].bind_proc    = -1;
        thread_num++;   
    }
    return rc;
}
int create_threads(){
    int rc = SUCCESS;

    if(g_data.stanza_ptr->num_cache_threads_to_create > 0){
        rc = create_cache_threads();
        if(rc != SUCCESS){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: create_cache_threads() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
            return(FAILURE);
        }
           
    }
    if(g_data.stanza_ptr->num_prefetch_threads_to_create > 0){
        rc = create_prefetch_threads();
        if(rc != SUCCESS){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: create_prefetch_threads() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
            return(FAILURE);
        }
    
    }

    return rc;
}
int cache_exer_operations(){
    int rc=SUCCESS;
    printf("coming in cache_exer_operations () \n");

    modify_filters_for_undertest_cache_instance();

    rc = apply_filters();
    if(rc != SUCCESS) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:apply_filters() failed with rc = %d",__LINE__,__FUNCTION__,rc);
        remove_shared_memory();
        return (rc);
    }
    
    print_memory_allocation_seg_details(DBG_IMP_PRINT);

    rc = fill_cache_exer_thread_context();
    if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: fill_cache_exer_thread_context() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
        return(FAILURE);
    }

    print_thread_data();


    displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Threads to be created (Cache threads=%d\nPrefetch threads=%d)\n"
        ,g_data.stanza_ptr->num_cache_threads_to_create,g_data.stanza_ptr->num_prefetch_threads_to_create);

     g_data.htx_d.test_id++;
     hxfupdate(UPDATE,&g_data.htx_d);

    rc = create_threads();
    if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:create_threads() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
        return(FAILURE);
    }

    update_hxecache_stats();
    /*reset contiguous mem inuse details*/
    for(int c=0;c<cache_g.num_cont_mem_chunks;c++){
        cache_g.cont_mem[c].in_use_mem_chunk = 0;
        for(int p=0;p<cache_g.cont_mem[c].num_pages;p++){
            cache_g.cont_mem[c].huge_pg[p].in_use =0;
        }
    }


    return rc;
}

int print_thread_data(){
    int rc = SUCCESS;
    struct thread_data* th = &g_data.thread_ptr[0];
    char log_file_name[256];
    FILE *log_fp=NULL;
    
    strcpy(log_file_name,g_data.htx_d.htx_exer_log_dir);

    sprintf(log_file_name,"%s/cache%d_log",log_file_name,g_data.dev_id);
    log_fp = fopen(log_file_name,"w");

    if(log_fp == NULL ) {
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:Could not create the log file. errno = %d(%s)\n",__LINE__,__FUNCTION__,errno,strerror(errno));
        return (FAILURE);
    }
    fprintf(log_fp,"#############################################################################################\n");
    fprintf(log_fp,"####################          Hxecache Log File            ######################\n");
    fprintf(log_fp,"#############################################################################################\n");

    fprintf(log_fp,"Exerciser instance          = %d\n",cache_g.cache_instance);
    fprintf(log_fp,"PVR                         = %x\n",g_data.sys_details.true_pvr);
    fprintf(log_fp,"L3 size                     = %llx MB \n\n",(g_data.sys_details.cache_info[L3].cache_size/MB));
    fprintf(log_fp,"Current Rule id             = %s\n",g_data.stanza_ptr->rule_id);
    fprintf(log_fp,"target cache                = %d (L2=1,L3=2)\n",g_data.stanza_ptr->tgt_cache);
    fprintf(log_fp,"test_case                   = %d\n",g_data.stanza_ptr->cache_test_case);
    fprintf(log_fp,"pf_irritator                = %d\n",g_data.stanza_ptr->pf_irritator);
    fprintf(log_fp,"pf_nstride                  = %d\n",g_data.stanza_ptr->pf_nstride);
    fprintf(log_fp,"pf_partial                  = %d\n",g_data.stanza_ptr->pf_partial);
    fprintf(log_fp,"pf_transient                = %d\n",g_data.stanza_ptr->pf_transient);
    fprintf(log_fp,"pf_dcbtna                   = %d\n",g_data.stanza_ptr->pf_dcbtna);
    fprintf(log_fp,"pf_dscr                     = %d\n",g_data.stanza_ptr->pf_dscr);
    fprintf(log_fp,"pf_conf                     = %d\n",g_data.stanza_ptr->pf_conf);
    fprintf(log_fp,"number of cache threads     = %d\n",g_data.stanza_ptr->num_cache_threads_to_create);
    fprintf(log_fp,"number of prefetch threads  = %d\n",g_data.stanza_ptr->num_prefetch_threads_to_create);   

    fprintf(log_fp,"\n****************************************************************************************\n");
    fprintf(log_fp,"**********      Thread details          ********************************\n");

    for(int thread_num =0;thread_num < g_data.stanza_ptr->num_threads;thread_num++){
        fprintf(log_fp,"****************************************************************************************\n");
        
        if(th[thread_num].thread_num  == -1){
            continue;
        }
        struct cache_exer_thread_info *local_ptr = &(cache_g.cache_th_data[thread_num]);

        fprintf(log_fp,"thread_num              = %d\n",th[thread_num].thread_num);
        fprintf(log_fp,"cpu to bind             = %d\n",th[thread_num].bind_proc);
        fprintf(log_fp,"Thread type             = %d(1-CACHE,2-PREFETCH)\n",local_ptr->oper_type);
        fprintf(log_fp,"pattern                 = %llx\n",local_ptr->pattern);
        if(local_ptr->oper_type == OPER_CACHE){
            fprintf(log_fp,"contiguous pages rqd    = %d\n",local_ptr->num_mem_sets);
            fprintf(log_fp,"using contiguous pages  = %d (1-yes,0-no)\n",local_ptr->found_cont_pages);
            for(int p=0;p<local_ptr->num_mem_sets;p++){
                fprintf(log_fp,"page[%d] = %p\n",p,local_ptr->contig_mem[p]);
            }
            fprintf(log_fp,"start class             = %d\n",local_ptr->start_class);
            fprintf(log_fp,"end class               = %d\n",local_ptr->end_class);   
            fprintf(log_fp,"class jump              = %d\n",local_ptr->walk_class_jump);
            fprintf(log_fp,"offset within cacheline = %d\n",local_ptr->offset_within_cache_line);
        }else{
            fprintf(log_fp,"prefetch alogorithm     = 0X%X\n",local_ptr->prefetch_algorithm);
            fprintf(log_fp,"prefetch mem size       = %llx MB\n",PREFETCH_MEM_SZ/MB);
            fprintf(log_fp,"prefetch start addr     = %p\n",local_ptr->prefetch_memory_start_address);
            fprintf(log_fp,"prefetch streams        = %d\n",local_ptr->prefetch_streams);
        }
    }
    fflush(log_fp);
    fclose(log_fp);
    return rc;
}
int modify_filters_for_undertest_cache_instance(){
    struct cpu_filter_info* cf = &g_data.stanza_ptr->filter.cf;
    /*struct mem_filter_info* mf = &g_data.stanza_ptr->filter.mf;*/
    int node,chip,sys_chip_count = 0,rc=SUCCESS;

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
            if(g_data.dev_id != sys_chip_count++){
                cf->node[node].chip[chip].chip_num = -1;
                /*mf->node[node].chip[chip].chip_num = -1;*/
            }
        }
    }
    return rc;
}
void update_hxecache_stats(){

    int num_oper        = g_data.stanza_ptr->num_oper;
    int nw              = g_data.stanza_ptr->num_writes;
    int nc              = g_data.stanza_ptr->num_compares;
    int cache_threads   = g_data.stanza_ptr->num_cache_threads_to_create;
    /*int pref_threads    = g_data.stanza_ptr->num_prefetch_threads_to_create;*/
    int num_lines       = (cache_g.contiguous_mem_required/g_data.sys_details.cache_info[L3].cache_line_size);
    int num_wr_per_line = (g_data.sys_details.cache_info[L3].cache_line_size/g_data.stanza_ptr->width);
    /*int pref_thread_mem = PREFETCH_MEMORY_PER_THREAD;*/
    unsigned long data_written=0;
    unsigned long data_read=0;

    data_written = (num_oper * nw * cache_threads * num_wr_per_line * num_lines);
    data_read    = (num_oper * nc * cache_threads * num_wr_per_line * num_lines);

    STATS_VAR_INC(bytes_writ,data_written);
    STATS_VAR_INC(bytes_read,data_read);
}
#ifdef __HTX_LINUX__
int find_EA_to_RA(unsigned long ea,unsigned long long* ra){
    const int __endian_bit = 1;
    int i, c,status;
    uint64_t read_val, file_offset;
    char path_buf [0x100] = {};
    FILE * f;
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

