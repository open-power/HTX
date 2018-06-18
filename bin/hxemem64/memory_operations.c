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
#define TB_FREQUENCY 512000000
#ifdef __HTX_LINUX__
    #define dcbf(x) __asm ("dcbf 0,%0\n\t": : "r" (x))
    #define read_tb(var) __asm volatile ("mfspr %0, 268":"=r"(var));
#else
    #pragma mc_func dcbf    { "7C0018AC" }          /* dcbf r3 */
    unsigned long long read_tb(void);
    #pragma mc_func read_tb {"7c6c42a6"}
    #pragma reg_killed_by read_tb gr3
#endif
extern struct nest_global g_data;
extern struct mem_exer_info mem_g;

int mem_operation_write_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned long *rw_ptr = (unsigned long *)seg_address;
    for (i=0; i < num_operations; i++) {
        *rw_ptr = *(unsigned long *)pattern_ptr;
        rw_ptr = rw_ptr + 1;
    }
    rc = 0;
    return (rc);

}

int mem_operation_write_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned int *rw_ptr = (unsigned int *)seg_address;
    for (i=0; i < num_operations; i++) {
        *rw_ptr = *(unsigned int*)pattern_ptr;
        rw_ptr = rw_ptr + 1;
    }
    rc = 0;
    return (rc);

}
int mem_operation_write_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned char *rw_ptr = (unsigned char *)seg_address;
    for (i=0; i < num_operations; i++) {
        *rw_ptr = *(unsigned char*)pattern_ptr;
        rw_ptr = rw_ptr + 1;
    }
    rc = 0;
    return (rc);

}
int mem_operation_read_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    volatile unsigned long *rw_ptr = (unsigned long *)seg_address;
    unsigned long consume_data=0;
    unsigned long read_data;
    for (i=0; i < num_operations; i++) {
        read_data = *(unsigned long *)rw_ptr;
        consume_data += (read_data+i);/*to avoide compiler optimization*/
        rw_ptr = rw_ptr + 1;
    }
    mem_g.dummy_read_data = consume_data;/*to avoide compiler optimization*/
    rc = 0;
    return (rc);
}

int mem_operation_read_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    volatile unsigned int *rw_ptr = (unsigned int *)seg_address;
    unsigned long consume_data=0;
    unsigned int read_data;
    for (i=0; i < num_operations; i++) {
        read_data = *(unsigned int *)rw_ptr;
        consume_data += (read_data+i);/*to avoide compiler optimization*/
        rw_ptr = rw_ptr + 1; 
    }
    mem_g.dummy_read_data = consume_data;/*to avoide compiler optimization*/
    rc = 0;
    return (rc);
}
int mem_operation_read_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    volatile unsigned char *rw_ptr = (unsigned char *)seg_address;
    unsigned long consume_data=0;
    unsigned char read_data;
    for (i=0; i < num_operations; i++) {
        read_data = *(unsigned char *)rw_ptr;
        consume_data += (read_data+i);/*to avoide compiler optimization*/
        rw_ptr = rw_ptr + 1; 
    }
    mem_g.dummy_read_data = consume_data;/*to avoide compiler optimization*/
    rc = 0;
    return (rc);
}
int mem_operation_comp_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    volatile unsigned long *ptr = (unsigned long *)seg_address; 
    unsigned long temp_val1,temp_val2 = *(unsigned long *)pattern_ptr;
    unsigned long i=0,rc=0;
    for (i=0; i < num_operations; i++) {
        if( (temp_val1 = *ptr) != temp_val2) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx (addr:%p)\n",__LINE__,__FUNCTION__,
                    temp_val2,temp_val1,ptr);
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        rc = 0;
    } 
    return rc;
}
int mem_operation_comp_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    volatile unsigned int *ptr = (unsigned int *)seg_address; 
    unsigned int temp_val1,temp_val2=*(unsigned int *)pattern_ptr;
    unsigned long i=0,rc=0;
    for (i=0; i < num_operations; i++) {
        if( (temp_val1 = *ptr) != temp_val2 ) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x (addr:%p)\n",__LINE__,__FUNCTION__,
                    temp_val2,temp_val1,ptr);
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        rc = 0;
    } 
    return rc;
}
int mem_operation_comp_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    volatile unsigned char *ptr = (unsigned char *)seg_address; 
    unsigned char temp_val1,temp_val2 = *(unsigned char *)pattern_ptr;
    unsigned long i=0,rc=0;
    for (i=0; i < num_operations; i++) {
        if( (temp_val1 = *ptr) != temp_val2 ) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x (addr:%p)\n",__LINE__,__FUNCTION__,
                    temp_val2,temp_val1,ptr);
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        rc = 0;
    } 
    return rc;
}

int mem_operation_write_addr(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned long address = *(unsigned long *)seg_address;
    unsigned long *rw_ptr = (unsigned long *)seg_address;
    for (i=0; i < num_operations; i++) {
        *rw_ptr = address;
        rw_ptr = rw_ptr + 1;
        address = (unsigned long)rw_ptr;
    }
    rc = 0;
    return (rc);
}

int mem_operation_comp_addr(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned long *ptr = (unsigned long *)seg_address ;
    unsigned long address = *(unsigned long *)seg_address;
    unsigned long temp_val;
    for (i=0; i < num_operations; i++) {
        if( (temp_val = *ptr) != address) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx (addr:%p)\n",__LINE__,__FUNCTION__,
                    address,temp_val,ptr);
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        address = (unsigned long)ptr;
        rc = 0;
    }
    return rc;
}
int  mem_operation_rim_dword( unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned long *w_ptr = (unsigned long *)seg_address;
    unsigned long read_data;
    unsigned long temp_val;

    for (i=0; i < num_operations; i++) {
        *w_ptr = *(unsigned long *)pattern_ptr;
        read_data = *(unsigned long *)w_ptr;
        dcbf((volatile unsigned long*)w_ptr);
        read_data = *(unsigned long *)w_ptr;
        if(read_data != (temp_val = *(unsigned long *)pattern_ptr)) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx (addr:%p)\n",__LINE__,__FUNCTION__,
                temp_val,read_data,w_ptr);
            rc = i+1;
            break;
        }
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    return rc;
}
int  mem_operation_rim_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned int *w_ptr = (unsigned int*)seg_address;
    unsigned int read_data;
    unsigned int temp_val;
    for (i=0; i < num_operations; i++) {
        *w_ptr = *(unsigned int*)pattern_ptr;
        read_data = *(unsigned int*)w_ptr;
        dcbf((volatile unsigned int*)w_ptr);
        read_data = *(unsigned int*)w_ptr;
        if(read_data != (temp_val = *(unsigned int*)pattern_ptr)) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x\n (addr:%p)",__LINE__,__FUNCTION__,
                temp_val,read_data,w_ptr);

            rc = i+1;
            break;
        }
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    return rc;
}

int  mem_operation_rim_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    char *w_ptr = (char*)seg_address;
    char read_data;
    char temp_val;
    for (i=0; i < num_operations; i++) {
        *w_ptr = *(char*)pattern_ptr;
        read_data = *(char*)w_ptr;
        dcbf((volatile char*)w_ptr);
        read_data = *(char*)w_ptr;
        if(read_data != (temp_val = *(char*)pattern_ptr)) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x (addr:%p)\n",__LINE__,__FUNCTION__,
                temp_val,read_data,w_ptr);
            rc = i+1;
            break;
        }
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    return rc;
}

int mem_operation_write_addr_comp(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned long *rw_ptr = (unsigned long *)seg_address;
    unsigned long address = *(unsigned long *)seg_address;
    unsigned long read_dw_data;
    for (i=0;i<num_operations;i++){
        *rw_ptr = address;
        dcbf((volatile unsigned long*)rw_ptr);
        read_dw_data = *(unsigned long *)rw_ptr;
        if(read_dw_data != address) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)rw_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)rw_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx (addr:%p)\n",__LINE__,__FUNCTION__,
                address,read_dw_data,rw_ptr);
            rc = i+1;
            break;
        }
        rw_ptr  = rw_ptr + 1;
        address = (unsigned long)rw_ptr;
        rc = 0;
    }
    return rc;
}

double find_rand_num_gen_fun_latency(unsigned long rand){

    unsigned long *lseed = &rand;
    unsigned long t1,t2,rand_no;
	struct drand48_data buffer;
    int i;
	srand48_r((long int)*lseed,&buffer);
    #ifndef __HTX_LINUX__
    t1 = read_tb();
    #else
    read_tb(t1);
    #endif

    for(i=0;i<1000;i++){

        rand_no = get_random_no_32(&buffer);

    }
    #ifndef __HTX_LINUX__
    t2 = read_tb();
    #else
    read_tb(t2);
    #endif
    return ( ((t2 - t1)/(double)TB_FREQUENCY)/1000.0 );
}

int rand_mem_access_wr_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,cache_line_sz =128;
    unsigned long *rw_ptr = (unsigned long *)seg_address;
    unsigned long offset,addr,time_ticks1,time_ticks2;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);
    double rand_fun_latency;
    if( g_data.stanza_ptr->operation == LATENCY_TEST){
        rand_fun_latency = find_rand_num_gen_fun_latency(rand_no);
        rand_fun_latency = (rand_fun_latency * num_operations);
        #ifndef __HTX_LINUX__
        time_ticks1 = read_tb();
        #else
        read_tb(time_ticks1);
        #endif
    }
    for (i=0;i<num_operations;i++){
           
		rand_no = get_random_no_32(&th->buffer);
        offset = (rand_no % sd->seg_size);
        addr   = ((unsigned long)seg_address + offset);
        addr   = (addr/cache_line_sz);
        addr   = (addr * cache_line_sz);
        rw_ptr = (unsigned long*)addr;
        *rw_ptr = *(unsigned long *)pattern_ptr;
        /*printf("written at %p with 0x%llx, rand_no = %llu\n",rw_ptr,*(unsigned long *)pattern_ptr,rand_no);*/
    }
    if( g_data.stanza_ptr->operation == LATENCY_TEST){
        #ifndef __HTX_LINUX__
        time_ticks2 = read_tb();
        #else
        read_tb(time_ticks2);
        #endif
        th->write_latency = (double)((time_ticks2 - time_ticks1)/(double)TB_FREQUENCY);
        th->write_latency = ((th->write_latency - rand_fun_latency)/(double)num_operations);
    	printf("\nwrite: rand_fun_latency = %.10llf\n ",rand_fun_latency);
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"*********************************************************************************\n"
                    "[thread:%d,segment:%d,seg_size:%llu B] Ticks = 0x%lx  WRITE LATENCY = %.5llf ns\n",sd->owning_thread,
                    sd->seg_num,sd->seg_size,((time_ticks2 - time_ticks1)/num_operations),(th->write_latency * 1000000000.0));
    }
    return 0;
}

int rand_mem_access_wr_word( unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,cache_line_sz =128;
    unsigned int *rw_ptr = (unsigned int *)seg_address;
    unsigned long offset,addr;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i<num_operations;i++){

        rand_no = get_random_no_32(&th->buffer);
        offset = (rand_no % sd->seg_size);
        addr   = ((unsigned long)seg_address + offset);
        addr   = (addr/cache_line_sz);
        addr   = (addr * cache_line_sz);
        rw_ptr = (unsigned int*)addr;
        *rw_ptr = *(unsigned int*)pattern_ptr;
        /*printf("written at %p with 0x%llx, rand_no = %llu\n",rw_ptr,*(unsigned long *)pattern_ptr,rand_no);*/
    }
    return 0;
}

int rand_mem_access_wr_byte( unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,cache_line_sz =128;
    unsigned char *rw_ptr = (unsigned char*)seg_address;
    unsigned long offset,addr;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i<num_operations;i++){

        rand_no = get_random_no_32(&th->buffer);
        offset = (rand_no % sd->seg_size);
        addr   = ((unsigned long)seg_address + offset);
        addr   = (addr/cache_line_sz);
        addr   = (addr * cache_line_sz);
        rw_ptr = (unsigned char*)addr;
        *rw_ptr = *(unsigned char*)pattern_ptr;
        /*printf("written at %p with 0x%llx, rand_no = %llu\n",rw_ptr,*(unsigned long *)pattern_ptr,rand_no);*/
    }
    return 0;
}


int rand_mem_access_rd_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,cache_line_sz =128;
    unsigned long *rw_ptr = (unsigned long *)seg_address;
    unsigned long offset,addr,time_ticks1,time_ticks2,consume_data=0,read_data;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);
    double rand_fun_latency;
    if( g_data.stanza_ptr->operation == LATENCY_TEST){
        rand_fun_latency = find_rand_num_gen_fun_latency(rand_no);
        rand_fun_latency = (rand_fun_latency * num_operations);
        #ifndef __HTX_LINUX__
        time_ticks1 = read_tb();
        #else
        read_tb(time_ticks1);
        #endif
    }
    for (i=0;i<num_operations;i++){

		rand_no = get_random_no_32(&th->buffer);
        offset = (rand_no % sd->seg_size);
        addr   = ((unsigned long)seg_address + offset);
        addr   = (addr/cache_line_sz);
        addr   = (addr * cache_line_sz);
        rw_ptr = (unsigned long*)addr;
        read_data = *(unsigned long *)rw_ptr;
        consume_data += (read_data+i);/*to avoide compiler optimization*/

    }

    if( g_data.stanza_ptr->operation == LATENCY_TEST){
        #ifndef __HTX_LINUX__
        time_ticks2 = read_tb();
        #else
        read_tb(time_ticks2);
        #endif
        th->read_latency = (double)((time_ticks2 - time_ticks1)/(double)TB_FREQUENCY);
        th->read_latency = ((th->read_latency - rand_fun_latency)/(double)num_operations);
    	printf("\nread: rand_fun_latency = %.10llf\n",rand_fun_latency);
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"*********************************************************************************\n"
                    "[thread:%d,segment:%d,seg_size:%llu B] Ticks = 0x%lx  READ LATENCY = %.5llf ns\n",sd->owning_thread,
                    sd->seg_num,sd->seg_size,((time_ticks2 - time_ticks1)/num_operations),(th->read_latency * 1000000000.0));
    }
    mem_g.dummy_read_data = consume_data;/*to avoide compiler optimization*/
    return 0;
}

int rand_mem_access_rd_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,cache_line_sz =128;
    unsigned int *rw_ptr = (unsigned int *)seg_address,read_data=0;
    unsigned long offset,addr,consume_data=0;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i<num_operations;i++){

        rand_no = get_random_no_32(&th->buffer);
        offset = (rand_no % sd->seg_size);
        addr   = ((unsigned long)seg_address + offset);
        addr   = (addr/cache_line_sz);
        addr   = (addr * cache_line_sz);
        rw_ptr = (unsigned int*)addr;
        read_data = *(unsigned int*)rw_ptr;
        consume_data += (read_data+i);/*to avoide compiler optimization*/
    }
    mem_g.dummy_read_data = consume_data;/*to avoide compiler optimization*/
    return 0;
}

int rand_mem_access_rd_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,cache_line_sz =128;
    unsigned char *rw_ptr = (unsigned char*)seg_address,read_data=0;
    unsigned long offset,addr,consume_data=0;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i<num_operations;i++){

        rand_no = get_random_no_32(&th->buffer);
        offset = (rand_no % sd->seg_size);
        addr   = ((unsigned long)seg_address + offset);
        addr   = (addr/cache_line_sz);
        addr   = (addr * cache_line_sz);
        rw_ptr = (unsigned char*)addr;
        read_data = *(unsigned char*)rw_ptr;
        consume_data += (read_data+i);/*to avoide compiler optimization*/
    }
    mem_g.dummy_read_data = consume_data;/*to avoide compiler optimization*/
    return 0;
}

int rand_mem_access_cmp_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0,cache_line_sz =128;
    unsigned long *rw_ptr = (unsigned long *)seg_address;
    unsigned long offset,addr,read_data=0,temp_val1 = *(unsigned long *)pattern_ptr;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);
    for (i=0;i<num_operations;i++){

        rand_no = get_random_no_32(&th->buffer);
        offset = (rand_no % sd->seg_size);
        addr   = ((unsigned long)seg_address + offset);
        addr   = (addr/cache_line_sz);
        addr   = (addr * cache_line_sz);
        rw_ptr = (unsigned long*)addr;
        read_data = *(unsigned long *)rw_ptr;
        if(read_data != temp_val1) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)rw_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)rw_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }                
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx (addr:%p)\n",__LINE__,__FUNCTION__,
                    temp_val1,read_data,rw_ptr);
			rc = i+1;
			break;
        }
		rc = 0;
    }
    return rc;
}

int rand_mem_access_cmp_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0,cache_line_sz =128;
    unsigned int *rw_ptr = (unsigned int*)seg_address;
    unsigned long offset,addr;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);
	unsigned int read_data=0,temp_val1 = *(unsigned int*)pattern_ptr;
    for (i=0;i<num_operations;i++){

        rand_no = get_random_no_32(&th->buffer);
        offset = (rand_no % sd->seg_size);
        addr   = ((unsigned long)seg_address + offset);
        addr   = (addr/cache_line_sz);
        addr   = (addr * cache_line_sz);
        rw_ptr = (unsigned int*)addr;
        read_data = *(unsigned int*)rw_ptr;
        if(read_data != temp_val1) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)rw_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)rw_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx (addr:%p)\n",__LINE__,__FUNCTION__,
                    temp_val1,read_data,rw_ptr);
			rc = i+1;
			break;
        }
		rc = 0;
    }
    return rc;
}

int rand_mem_access_cmp_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0,cache_line_sz =128;
    unsigned char *rw_ptr = (unsigned char*)seg_address;
    unsigned long offset,addr;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);
    unsigned char read_data=0,temp_val1 = *(unsigned char*)pattern_ptr;
    for (i=0;i<num_operations;i++){

        rand_no = get_random_no_32(&th->buffer);
        offset = (rand_no % sd->seg_size);
        addr   = ((unsigned long)seg_address + offset);
        addr   = (addr/cache_line_sz);
        addr   = (addr * cache_line_sz);
        rw_ptr = (unsigned char*)addr;
        read_data = *(unsigned char*)rw_ptr;
        if(read_data != temp_val1) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)rw_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)rw_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx (addr:%p)\n",__LINE__,__FUNCTION__,
                    temp_val1,read_data,rw_ptr);
			rc = i+1;
			break;
        }
		rc = 0;
    }
    return rc;
}

