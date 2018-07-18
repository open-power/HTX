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

#ifdef __HTX_LINUX__
#define dcbf(x) __asm ("dcbf 0,%0\n\t": : "r" (x))
#else
#pragma mc_func dcbf    { "7C0018AC" }          /* dcbf r3 */
#endif
extern struct nest_global g_data;
extern struct mem_exer_info mem_g;
unsigned char get_random_no_8(struct drand48_data *buf)
{
    unsigned char r_num;
	long tmp;
	lrand48_r(buf,&tmp);
    r_num = (unsigned char)(tmp % 255);
    return r_num;
}

/*unsigned int get_random_no_32(unsigned int *lseed)
{
    unsigned int r_num = rand_r((unsigned int*)lseed);
    return r_num;
}*/

unsigned int get_random_no_32(struct drand48_data *buf){
    long int rand_num;
    lrand48_r(buf,&rand_num);
    return (unsigned int)rand_num;
}

/*unsigned long get_random_no_64(unsigned int *lseed) {

    long int tmp;
    unsigned long r_num;
    tmp = rand_r((unsigned int*)lseed);
    *lseed = tmp;
    r_num = ((unsigned long) tmp << 32);
    r_num |= rand_r((unsigned int*)lseed);
    return r_num;*/
unsigned long get_random_no_64(struct drand48_data *buf){
	unsigned long val_8 ;
	long rand_num;
	lrand48_r(buf,&rand_num);
	val_8 = (unsigned long)rand_num << 32;
	lrand48_r(buf,&rand_num);
	val_8 = val_8 | rand_num;
	return val_8 ;
}

int rand_operation_write_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned long *rw_ptr = (unsigned long *)seg_address;
    unsigned long rand_no=*(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_64(&th->buffer);
        *rw_ptr = rand_no;
        rw_ptr = rw_ptr + 1;
    }
    rc = 0;
    return (rc);
}

int rand_operation_write_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned int *rw_ptr = (unsigned int*)seg_address;
    unsigned int rand_no = *(unsigned long*)seed;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_32(&th->buffer);
        *rw_ptr = rand_no;
        rw_ptr = rw_ptr + 1;
    }
    rc = 0;
    return (rc);
}

int rand_operation_write_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned char *rw_ptr = (unsigned char*)seg_address;
    unsigned char rand_no=*(unsigned long*)seed;; 
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_8(&th->buffer);
        *rw_ptr = rand_no;
        rw_ptr = rw_ptr + 1;
    }
    rc = 0;
    return (rc);
}

int rand_operation_comp_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned long *ptr   = (unsigned long *)seg_address;
    unsigned long rand_no = *(unsigned long*)seed; 
    unsigned long temp_val;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_64(&th->buffer);
        if( (temp_val = *ptr) != rand_no) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx (addr:%p), thread:%d\n",__LINE__,__FUNCTION__,
                rand_no,temp_val,ptr,sd->owning_thread);
			th->rand_expected_value = rand_no;
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        rc = 0;
    }
    return rc;
}

int rand_operation_comp_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned int *ptr   = (unsigned int*)seg_address;
    unsigned int rand_no = *(unsigned long*)seed;
    unsigned int temp_val;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);
    /*rand_debug*/
    unsigned long long prev_drand48_first_8B=0,*drand48_ptr=NULL;
    char fname1[128];
    for (i=0;i<num_operations;i++){
        drand48_ptr = (unsigned long long*)&th->buffer;
        prev_drand48_first_8B = (unsigned long long)*drand48_ptr;
        rand_no = get_random_no_32(&th->buffer);
        if( (temp_val = *ptr) != rand_no) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long )rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x (addr:%p), thread:%d\n",__LINE__,__FUNCTION__,
                rand_no,temp_val,ptr,sd->owning_thread);
			th->rand_expected_value = rand_no;
            /*rand_debug*/
            th->rc_offset = i;
            sprintf(fname1,"%s/hxemem.%s.RC__drand48_buf__word_%lu__th_%lu__pass_%lu",g_data.htx_d.htx_exer_log_dir,g_data.stanza_ptr->rule_id,i,sd->owning_thread,th->rc_pass_count);
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
            fprintf(f,"\nprev_value=%llx\n",prev_drand48_first_8B);
            fclose(f);

            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        rc = 0;
    }
    th->rc_pass_count++;
    return rc;
}

int rand_operation_comp_byte( unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned char *ptr   = (unsigned char*)seg_address;
    unsigned char rand_no = *(unsigned long*)seed;
    unsigned char temp_val;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_8(&th->buffer);
        if( (temp_val = *ptr) != rand_no) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x (addr:%p),thread:%d\n",__LINE__,__FUNCTION__,
                rand_no,temp_val,ptr,sd->owning_thread);
			th->rand_expected_value = rand_no;
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        rc = 0;
    }
    return rc;
}

int rand_operation_rim_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned long  *w_ptr = (unsigned long *)seg_address;
    unsigned long rand_no = *(unsigned long*)seed,read_dw_data;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i < num_operations;i++){
        rand_no = get_random_no_64(&th->buffer);
        *w_ptr = rand_no;
        read_dw_data = *(unsigned long *)w_ptr;
        dcbf((volatile unsigned long*)w_ptr);
        if(read_dw_data != rand_no) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif

            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x (addr:%p)\n",__LINE__,__FUNCTION__,
                rand_no,read_dw_data,w_ptr);
			th->rand_expected_value = rand_no;
            rc = i+1;
            break;
        }
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    return rc;
}

int rand_operation_rim_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned int *w_ptr = (unsigned int*)seg_address;
    unsigned int rand_no = *(unsigned long*)seed,read_data;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i < num_operations;i++){
        rand_no = get_random_no_32(&th->buffer);
        *w_ptr = rand_no;
        read_data = *(unsigned int*)w_ptr;
        dcbf((volatile unsigned int*)w_ptr);
        if(read_data != rand_no) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long *)seg_address,(unsigned long)rand_no,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x (addr:%p)\n",__LINE__,__FUNCTION__,
                rand_no,read_data,w_ptr);
            rc = i+1;
			th->rand_expected_value = rand_no;
            break;
        }
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    return rc;
}

int rand_operation_rim_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0;
    unsigned char *w_ptr = (unsigned char*)seg_address;
    unsigned char rand_no= *(unsigned long*)seed,read_data;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (i=0;i < num_operations;i++){
        rand_no = get_random_no_8(&th->buffer);
        *w_ptr = rand_no;
        read_data = *(unsigned char*)w_ptr;
        dcbf((volatile unsigned char*)w_ptr);
        if(read_data != rand_no) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x (addr:%p)\n",__LINE__,__FUNCTION__,
                rand_no,read_data,w_ptr);
            rc = i+1;
			th->rand_expected_value = rand_no;
            break;
        }
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    return rc;
}

