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

#include "nest_framework.h"
#define MAX_TLB_FUNCTIONS 2
#define RAND_TLB_OPER   0
#define SEQ_TLB_OPER    1
#define CPU_BIND    1 
#define CPU_UNBIND  0

extern char page_size_name[MAX_PAGE_SIZES][8];
extern struct nest_global g_data;
extern struct mem_exer_info mem_g;
typedef void (*op_tlb_fptr)(void*);
static int cpus_in_core_array[MAX_CORES][MAX_CPUS_PER_CORE];
static int smt_core_array[MAX_CORES];
int update_sys_detail_flag = SET;
#ifdef __HTX_LINUX__
void tlb_SIGUSR2_hdl(int, int, struct sigcontext *);
#else
void tlb_SIGRECONFIG__handler (int sig, int code, struct sigcontext *scp);
#endif
void handle_cpu_binding(struct thread_data *t,int bind);
void tlb_gen_random_cpus(void* th);
void tlb_gen_sequential_cores(void* th);
int remove_shared_memory_tlb(struct thread_data *t);
int get_shared_memory_tlb(struct thread_data *t);

op_tlb_fptr tlb_thread_functions[MAX_TLB_FUNCTIONS]= {tlb_gen_random_cpus,tlb_gen_sequential_cores};

int run_tlb_operaion()
{
    int rc = SUCCESS,num_th,page_index,tlb_oper_type=RAND_TLB_OPER;
    unsigned int j1=12,*seed1;
    int num_threads = g_data.sys_details.cores * ((float)g_data.stanza_ptr->percent_hw_threads/100); 
    int seg = 0; /* only one seg*/
printf("cores=%d and threads = %d, val=%d\n",g_data.sys_details.cores,num_threads,(g_data.stanza_ptr->percent_hw_threads/100));
    if (num_threads < 1){
        num_threads = 1;
    }
    g_data.stanza_ptr->num_threads = num_threads;
    if(update_sys_detail_flag){
        for(int i=0;i<g_data.sys_details.cores;i++){
            #ifdef __HTX_LINUX__
            rc = get_phy_cpus_in_core(i,cpus_in_core_array[i]);
            #else
            rc = get_cpus_in_core(i,cpus_in_core_array[i]);
            #endif
            if(rc > 0){
                smt_core_array[i] = rc;
            }else{
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:get_cpus_in_core/get_phy_cpus_in_core  returned rc = %d for core : %d\n",
                __LINE__,__FUNCTION__,rc,i);
                exit(1);
            }
            update_sys_detail_flag = UNSET;
        }
    }
    if (g_data.stanza_ptr->seg_size[PAGE_INDEX_4K] == -1 ) {
        g_data.stanza_ptr->seg_size[PAGE_INDEX_4K] = ((4*KB) * (4*KB));
    }
    if (g_data.stanza_ptr->seg_size[PAGE_INDEX_64K] == -1 ) {
        g_data.stanza_ptr->seg_size[PAGE_INDEX_64K] = ((64*KB) * (4*KB));
    }
    
    rc = allocate_mem_for_threads();
    if(rc != SUCCESS){
        return(FAILURE);
    }
    g_data.htx_d.test_id++;
    hxfupdate(UPDATE,&g_data.htx_d);

    if(strcmp(g_data.stanza_ptr->tlbie_test_case,"SEQ_TLB")==0){
        tlb_oper_type = SEQ_TLB_OPER;
    }else if(strcmp(g_data.stanza_ptr->tlbie_test_case,"RAND_TLB")==0){
        tlb_oper_type = RAND_TLB_OPER;
    }
    seed1 = &j1;
    for (num_th = 0 ; num_th < num_threads ; num_th++){
		j1 += num_th;
        struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[num_th]);
        g_data.thread_ptr[num_th].testcase_thread_details = (void *)local_ptr;
        g_data.thread_ptr[num_th].thread_num = num_th;
        while(1){
            page_index = (rand_r(seed1) % 2);
            if( g_data.sys_details.memory_details.pdata[page_index].supported ){
                break;
            }
            j1++;
        }
        displaym(HTX_HE_INFO,DBG_IMP_PRINT,"TLBIE : thread %d will use %s page(total pages:%d) backed shm memory\n",
            g_data.thread_ptr[num_th].thread_num,page_size_name[page_index],
            (g_data.stanza_ptr->seg_size[page_index]/g_data.sys_details.memory_details.pdata[page_index].psize));

        local_ptr->num_segs = 1;
        if(local_ptr->num_segs != 0){
            local_ptr->seg_details = (struct page_wise_seg_info*) malloc(local_ptr->num_segs * sizeof(struct page_wise_seg_info));
            if(local_ptr->seg_details == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed for memory %d bytes with errno(%d)%s, num_segs for thread %d = %d\n",
                    __LINE__,__FUNCTION__,(local_ptr->num_segs * sizeof(struct page_wise_seg_info)),errno,strerror(errno),
                    g_data.thread_ptr[num_th].thread_num,local_ptr->num_segs);
                return(FAILURE);
            }
        }
        local_ptr->seg_details[seg].owning_thread     = num_th;
        local_ptr->seg_details[seg].shm_size          = g_data.stanza_ptr->seg_size[page_index];
        local_ptr->seg_details[seg].page_size_index   = page_index;
        local_ptr->seg_details[seg].seg_num           = 0;
        local_ptr->seg_details[seg].shm_mem_ptr       = NULL;
        local_ptr->seg_details[seg].shmid             = -1;
        local_ptr->seg_details[seg].in_use            = 1;
        local_ptr->seg_details[seg].cpu_chip_num      = -1;
        local_ptr->seg_details[seg].mem_chip_num      = -1;

        if (pthread_create(&g_data.thread_ptr[num_th].tid,NULL,(void *)(tlb_thread_functions[tlb_oper_type]),(void *)&g_data.thread_ptr[num_th])){
            displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,"[%d]%s:Error in creating thread no %d, errno:%d(%s) \n",
                __LINE__,__FUNCTION__,num_th,errno,strerror(errno));
        }

        j1++;
    } 

    for (num_th = 0 ; num_th < num_threads ; num_th++){
        int thread_retval,rc;
        void *th_join_result = (void *)&thread_retval;
        
        rc = pthread_join(g_data.thread_ptr[num_th].tid,&th_join_result);
        if ( rc != 0 ) {
            displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,"[%d]%s:Error in Joining thread no %d, returned = %d,errno:%d(%s)\n",
                __LINE__,__FUNCTION__,num_th,rc,errno,strerror(errno));
        }
        g_data.thread_ptr[num_th].tid = -1;
        
    }

    return rc;
}

void tlb_gen_random_cpus(void* th){
    int i,rc=SUCCESS,iter;
    struct thread_data* t = (struct thread_data*)th;
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t->thread_num]);
    t->testcase_thread_details = (void *)local_ptr;
    unsigned int buf,*parse_write_shm_ptr,*parse_read_shm_ptr,j1=(t->thread_num + 12), j2=(t->thread_num + 16);
    int bind_proc,seg = 0;/*seg = 0 always */
    int num_4k_pages = (g_data.stanza_ptr->seg_size[PAGE_INDEX_4K]) / (4*KB);
    int ptr_increment = (4*KB)/sizeof(int);
    char msg_text[1024];
    unsigned int *seed1 = &j1;
    unsigned int *seed2 = &j2;


    local_ptr->seg_details[seg].page_size_index = PAGE_INDEX_4K;
    for (iter=0; iter < g_data.stanza_ptr->num_oper; iter++,j1++,j2++) {
        bind_proc = rand_r(seed1);
        t->bind_proc = (bind_proc % g_data.sys_details.tot_cpus);
        debug(HTX_HE_INFO,DBG_MUST_PRINT,"[%s]num_oper:%d,thread:%d,touching 4k pages : %d by binding to cpu %d\n",
            __FUNCTION__,iter,t->thread_num,num_4k_pages,t->bind_proc);
        rc = get_shared_memory_tlb(t);
        if(rc != SUCCESS){
            g_data.exit_flag = 1;
            release_thread_resources(t->thread_num);
            pthread_exit((void *)1);
        }
        local_ptr->seg_details[seg].shm_size = g_data.stanza_ptr->seg_size[PAGE_INDEX_4K];
        parse_write_shm_ptr = (unsigned int *)local_ptr->seg_details[seg].shm_mem_ptr;
        parse_read_shm_ptr = (unsigned int *)local_ptr->seg_details[seg].shm_mem_ptr;

        handle_cpu_binding(t,CPU_BIND);
        for (i = 0; i < num_4k_pages; i++) {
            /*printf("####WRITE:touching page:%d at %p\n",i,parse_write_shm_ptr);*/
            *parse_write_shm_ptr=0xBEEFDEAD;
             parse_write_shm_ptr += ptr_increment ;
        }
        handle_cpu_binding(t,CPU_UNBIND);

        /* Touch pages again with random cpu for read and compare*/
        bind_proc = rand_r(seed2);
        t->bind_proc = (bind_proc % g_data.sys_details.tot_cpus);
        handle_cpu_binding(t,CPU_BIND);
        debug(HTX_HE_INFO,DBG_MUST_PRINT,"[%s]num_oper:%d,thread:%d,touching 4k pages : %d for READ by binding to cpu %d\n",
            __FUNCTION__,iter,t->thread_num,num_4k_pages,t->bind_proc);

        for (i = 0; i < num_4k_pages; i++) {
            /*printf("####READ:touching page:%d at %p\n",i,parse_read_shm_ptr);*/
            buf = *(unsigned int*)parse_read_shm_ptr;
            parse_read_shm_ptr += ptr_increment ;
            if (buf != 0xBEEFDEAD) {
                /* trap to kdb on miscompare
                *R3=0xBEEFDEAD
                *R4=expected value
                *R5=Actual value
                *R6=read buffer address
                *R7=bind proc
                *R8=current num oper
                *R9=thread pointer
                *R10=current stanza pointer*/
                if( (g_data.stanza_ptr->misc_crash_flag) && (g_data.kdb_level) ){
                    displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "TLBIE:Miscompare Detected\n");
                    #ifdef __HTX_LINUX__
                    do_trap_htx64 ((unsigned long)0xBEEFDEAD,\
                                (unsigned long)0xBEEFDEAD,\
                                (unsigned long)buf,\
                                (unsigned long)parse_read_shm_ptr,\
                                (unsigned long)t->bind_proc,\
                                (unsigned long)iter,\
                                (unsigned long)t,\
                                (unsigned long)g_data.stanza_ptr);
                    #else

                    trap(0xBEEFDEAD,0xBEEFDEAD,buf,parse_read_shm_ptr,iter,t,g_data.stanza_ptr);
                    #endif
                }
                sprintf(msg_text,"MISCOMPARE(hxetlbie) in rule %s,Rules file=%s\n"
                        "Expected pattern=%x,Actual value=%x at Effective address = %llx\n"
                        "Segment address=%llx,num oper=%d,Thread number = %d, CPU number bound with = %d\n",\
                        g_data.stanza_ptr->rule_id,g_data.rules_file_name,\
                        0xBEEFDEAD,(unsigned int)buf,(unsigned long long)parse_read_shm_ptr,\
                        (unsigned long long)local_ptr->seg_details[seg].shm_mem_ptr,iter,t->thread_num,t->bind_proc);
                displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "TLBIE:Miscompare Detected\n%s", msg_text);
                exit(1);

             }
        }
        handle_cpu_binding(t,CPU_UNBIND);
        rc = remove_shared_memory_tlb(t);
        if(rc != SUCCESS){
            g_data.exit_flag = 1;
            release_thread_resources(t->thread_num);
            pthread_exit((void *)1);
        }
        if(g_data.exit_flag){
            break;
        }
    }
    release_thread_resources(t->thread_num);
    pthread_exit((void *)0);
}

void tlb_gen_sequential_cores(void* th){

    int rc=SUCCESS,ptr_increment=0,num_pages=0,iter,cpu_index=0;
    struct thread_data* t = (struct thread_data*)th;
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t->thread_num]);
    t->testcase_thread_details = (void *)local_ptr;
    unsigned int *parse_write_shm_ptr,*parse_read_shm_ptr,*page_ptr;
    int bind_proc,page_offset=0,seg = 0;/*seg = 0 always */
    int pi = local_ptr->seg_details[seg].page_size_index;

    struct pattern_typ {
        short core_num;
        short hexa;
        short thread_num;
        short temp;
    }pattern[g_data.sys_details.cores];
    unsigned long *pattern_ptr;
    struct pattern_typ temp_obj;

    printf("thread:%d, in sequential test\n",t->thread_num);
    if(local_ptr->seg_details[seg].in_use == 1){
        ptr_increment   = (g_data.sys_details.memory_details.pdata[pi].psize/g_data.sys_details.cores)/sizeof(int);
        num_pages       = ((g_data.stanza_ptr->seg_size[pi])/g_data.sys_details.memory_details.pdata[pi].psize);
        page_offset   = (g_data.sys_details.memory_details.pdata[pi].psize/sizeof(int));
    }


    for (int i = 0; i < g_data.sys_details.cores; i++) {
            pattern[i].core_num = i; 
            pattern[i].hexa = 0xAA;
            pattern[i].thread_num = t->thread_num;
            pattern[i].temp = 0;
    }

    for (iter=0; iter < g_data.stanza_ptr->num_oper; iter++) {
        rc = get_shared_memory_tlb(t);
        if(rc != SUCCESS){
            g_data.exit_flag = 1;
            release_thread_resources(t->thread_num);
            pthread_exit((void *)1);
        }
        parse_write_shm_ptr = (unsigned int *)local_ptr->seg_details[seg].shm_mem_ptr;
        parse_read_shm_ptr = (unsigned int *)local_ptr->seg_details[seg].shm_mem_ptr;
        page_ptr = parse_write_shm_ptr;

        for (int c = 0; c < g_data.sys_details.cores; c++) {
            cpu_index = (t->thread_num % smt_core_array[c]);
            bind_proc = cpus_in_core_array[c][cpu_index];
            t->bind_proc = bind_proc;
            debug(HTX_HE_INFO,DBG_MUST_PRINT,"num_oper:%d,thread:%d,touching pages : %d(%s) by binding to cpu %d of core %d\n",
                iter,t->thread_num,num_pages,page_size_name[pi],bind_proc,c);

            handle_cpu_binding(t,CPU_BIND);
            parse_write_shm_ptr = page_ptr;
            parse_write_shm_ptr += (c * ptr_increment);
        
            for(int p=0;p<num_pages;p++) {
                pattern_ptr = (unsigned long*)&pattern[c];
                /*printf("####WRITE:touching page:%d at %p\n",p,parse_write_shm_ptr);*/
                memcpy(parse_write_shm_ptr, pattern_ptr, sizeof(struct pattern_typ));    

                parse_write_shm_ptr += page_offset;
            }
            handle_cpu_binding(t,CPU_UNBIND);
        }
    
        page_ptr = parse_read_shm_ptr;
        for (int c = 0; c < g_data.sys_details.cores; c++) {
            cpu_index = (t->thread_num % smt_core_array[c]);
            bind_proc = cpus_in_core_array[c][cpu_index];
            t->bind_proc =  bind_proc;
            handle_cpu_binding(t,CPU_BIND);
            parse_read_shm_ptr = page_ptr;
            parse_read_shm_ptr += (c * ptr_increment) ;
            for(int p=0;p<num_pages;p++) {
                pattern_ptr = (unsigned long*)&pattern[c];
                /*printf("####READ:touching page;%d at %p\n",p,parse_read_shm_ptr);*/
                memcpy(&temp_obj, parse_read_shm_ptr, sizeof(struct pattern_typ));
                if ( memcmp(&temp_obj, pattern_ptr, sizeof(struct pattern_typ) ) ) {
                    /* trap to kdb on miscompare
                    *R3=0xBEEFDEAD
                    *R4=Expected value address
                    *R5=actual value address  address
                    *R6-page index
                    *R7=CPU number to bound
                    *R8=current num_oper
                    *R9=Thread pointer
                    *R10=current stanza pointer*/
                    if( (g_data.stanza_ptr->misc_crash_flag) && (g_data.kdb_level) ){
                        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "TLBIE:Miscompare Detected\n");
                        #ifdef __HTX_LINUX__
                        do_trap_htx64 ((unsigned long)0xBEEFDEAD,\
                                (unsigned long)&pattern[c],\
                                (unsigned long)parse_read_shm_ptr,\
                                (unsigned long)pi,\
                                (unsigned long)t->bind_proc,\
                                (unsigned long)iter,\
                                (unsigned long)t,\
                                (unsigned long)g_data.stanza_ptr);
                        #else

                        trap(0xBEEFDEAD,pattern[c],parse_read_shm_ptr,t->bind_proc,iter,t,g_data.stanza_ptr);
                        #endif
                    }
                    char msg_text[500];
                    sprintf(msg_text,"MISCOMPARE(hxetlbie) in rule %s,Rules file=%s\n"
                            "Expected pattern=%llx,Actual value=%llx at Effective address = %llx\n"
                            "Segment address=%llx,num oper=%d,Thread number = %d, CPU number bound with = %d\n",\
                            g_data.stanza_ptr->rule_id,g_data.rules_file_name,\
                            *((unsigned long long*)pattern_ptr),*(unsigned long long*)&temp_obj,(unsigned long long)parse_read_shm_ptr,\
                            local_ptr->seg_details[seg].shm_mem_ptr,iter,t->thread_num,bind_proc);
                    displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT, "TLBIE:Miscompare Detected\n%s", msg_text);
                    exit(1);
                }
                parse_read_shm_ptr += page_offset;
            }
            handle_cpu_binding(t,CPU_UNBIND);
        }

        rc = remove_shared_memory_tlb(t);
        if(rc != SUCCESS){
            g_data.exit_flag = 1;
            release_thread_resources(t->thread_num);
            pthread_exit((void *)1);
        }
        if(g_data.exit_flag){        
            break;
        }
    }
    release_thread_resources(t->thread_num);
    pthread_exit((void *)0);
}

void handle_cpu_binding(struct thread_data *t,int bind){
    int bind_proc = t->bind_proc,rc;
    if(!g_data.stanza_ptr->disable_cpu_bind){
        if(bind == CPU_BIND){
            /*replace below call with bind_to_cpu,common betweeen AIX/LINUX*/
        #ifndef __HTX_LINUX__
            t->kernel_tid = thread_self();
            /*rc = bindprocessor(BINDTHREAD,t->kernel_tid,bind_proc);*/
            rc = htx_bind_thread(bind_proc,bind_proc);
        #else
            rc = htx_bind_thread(bind_proc,-1);
        #endif
            if(rc < 0){
            #ifdef __HTX_LINUX__
                if (rc == -2 || rc == -1) {
                    displaym(HTX_HE_INFO,DBG_MUST_PRINT,"(th:%d)lcpu : %d has been hot removed, exiting thread "
                           "next stanza operations will run without cpu bind\n",t->thread_num,bind_proc);
                    pthread_mutex_lock(&g_data.tmutex);
                        g_data.stanza_ptr->disable_cpu_bind = 1;
                    pthread_mutex_unlock(&g_data.tmutex);
                }else{
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: bind to cpu %d by thread %d is failed with rc = %d,err:%d(%s)\n",
                        __LINE__,__FUNCTION__,bind_proc,t->thread_num,rc,errno,strerror(errno));
                    g_data.exit_flag = 1;
                }
            #else
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: bind to cpu %d by thread %d is failed with rc = %d,err:%d(%s)\n",
                    __LINE__,__FUNCTION__,bind_proc,t->thread_num,rc,errno,strerror(errno));
                g_data.exit_flag = 1;
            #endif
                release_thread_resources(t->thread_num);
				remove_shared_memory_tlb(t);
                pthread_exit((void *)1);
            }
        }else if(bind == CPU_UNBIND){
            rc = htx_unbind_thread();
            t->bind_proc = -1;
        }
            
    }
}

int get_shared_memory_tlb(struct thread_data *t){
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t->thread_num]);
    t->testcase_thread_details = (void *)local_ptr;
    int tlbie_shm_id,*tlbie_shm_ptr,memflg,seg = 0;/*Always only one segment*/
    int pi = local_ptr->seg_details[seg].page_size_index;
    #ifndef __HTX_LINUX__
    struct shmid_ds shm_buf;
    tlbie_shm_id = shmget(IPC_PRIVATE,g_data.stanza_ptr->seg_size[pi],IPC_CREAT | 0666);
    if(tlbie_shm_id == -1) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: shmget errored out with errno:%d(%s) for thread no.: %d\n",
            __LINE__,__FUNCTION__,errno,strerror(errno),t->thread_num);
        return FAILURE;
    }
    shm_buf.shm_pagesize=g_data.sys_details.memory_details.pdata[pi].psize;
    if((shmctl(tlbie_shm_id,SHM_PAGESIZE, &shm_buf)) == -1){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:shmctl failed while setting page sizei %s,errno:%d(%s).\n",
            __LINE__,__FUNCTION__,page_size_name[pi],errno,strerror(errno));
        return FAILURE;
    }
    #else
    memflg = (IPC_CREAT | IPC_EXCL |SHM_R | SHM_W );
    if((g_data.sys_details.memory_details.pdata[pi].psize == 16 * MB) || (g_data.sys_details.memory_details.pdata[pi].psize == (2 * MB))){
        memflg |= SHM_HUGETLB;
    }
    tlbie_shm_id = shmget(IPC_PRIVATE,g_data.stanza_ptr->seg_size[pi],memflg);
    #endif
    tlbie_shm_ptr = (int  *) shmat (tlbie_shm_id, (int *)0, 0);
    if ((int *)tlbie_shm_ptr == (int *)-1) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:shmat errored out with errno : %d(%s)for  thread no. : %d,pagesize=%s\n",
            __LINE__,__FUNCTION__,errno,strerror(errno),page_size_name[pi]);
        return FAILURE;
    }
    local_ptr->seg_details[seg].shm_mem_ptr       = (char *)tlbie_shm_ptr;
    local_ptr->seg_details[seg].shmid             = tlbie_shm_id;

    return SUCCESS;
}

int remove_shared_memory_tlb(struct thread_data *t){
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t->thread_num]);
    t->testcase_thread_details = (void *)local_ptr;
    int seg = 0;
    if(local_ptr->seg_details[seg].shm_mem_ptr != NULL){
        /* Dettach the shm */
        if(shmdt(local_ptr->seg_details[seg].shm_mem_ptr) == -1) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:shmdt failed with %d(%s) for size=%lu,EA=%lu\n",
            __LINE__,__FUNCTION__,errno,strerror(errno),local_ptr->seg_details[seg].shm_size,local_ptr->seg_details[seg].shm_mem_ptr);
            return FAILURE;
        }
        local_ptr->seg_details[seg].shm_mem_ptr = NULL;
    }
    if(local_ptr->seg_details[seg].shmid != -1){
        /* Remove shm area*/
        if (shmctl(local_ptr->seg_details[seg].shmid,IPC_RMID, 0) == -1) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:shmctl failed with %d(%s) for size=%lu, shm id=%lu\n",
            __LINE__,__FUNCTION__,errno,strerror(errno),local_ptr->seg_details[seg].shm_size,local_ptr->seg_details[seg].shmid);
            return FAILURE;
        }
        local_ptr->seg_details[seg].shmid = -1;
    }

    return SUCCESS;
}
