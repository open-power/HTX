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

unsigned char get_random_no_8(unsigned long *lseed)
{
    unsigned char r_num;
    long int tmp = rand_r((unsigned int*)lseed);
    r_num = (unsigned char)(tmp % 255);
    return r_num;
}

unsigned int get_random_no_32(unsigned long *lseed)
{
    unsigned int r_num = rand_r((unsigned int*)lseed);
    return r_num;
}

unsigned long get_random_no_64(unsigned long *lseed) {

    long int tmp;
    unsigned long r_num;
    tmp = rand_r((unsigned int*)lseed);
    *lseed = tmp;
    r_num = ((unsigned long) tmp << 32);
    r_num |= rand_r((unsigned int*)lseed);
    return r_num;
}

int rand_operation_write_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned long *rw_ptr = (unsigned long *)seg_address;
    unsigned long *lseed      = (unsigned long *)seed;
    unsigned long rand_no;
    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_64(lseed);
        *rw_ptr = rand_no;
        rw_ptr = rw_ptr + 1;
        *lseed = (unsigned long)rand_no;
    }
    *(unsigned long*)seed = (unsigned long) rand_no;
    rc = 0;
    return (rc);
}

int rand_operation_write_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned int *rw_ptr = (unsigned int*)seg_address;
    unsigned long *lseed      = (unsigned long*)seed;
    unsigned int rand_no = *(unsigned long*)seed;
    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_32(lseed);
        *rw_ptr = rand_no;
        rw_ptr = rw_ptr + 1;
        *lseed = (unsigned long)rand_no;
    }
    *(unsigned long*)seed = (unsigned long) rand_no;
    rc = 0;
    return (rc);
}

int rand_operation_write_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned char *rw_ptr = (unsigned char*)seg_address;
    unsigned long *lseed      = (unsigned long*)seed;
    unsigned char rand_no; 
    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_8(lseed);
        *rw_ptr = rand_no;
        rw_ptr = rw_ptr + 1;
        *lseed = (unsigned long)rand_no;
    }
    *(unsigned long*)seed = (unsigned long) rand_no;
    rc = 0;
    return (rc);
}

int rand_operation_comp_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned long *ptr   = (unsigned long *)seg_address;
    unsigned long *lseed = (unsigned long *)seed;
    unsigned long rand_no; 
    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_64(lseed);
        if(*ptr != rand_no) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            rc = i+1;
            break;
        }
        *(unsigned long*)lseed = (unsigned long)rand_no;
        ptr = ptr + 1;
        rc = 0;
    }
    *(unsigned long*)seed = (unsigned long)rand_no;
    return rc;
}

int rand_operation_comp_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned int *ptr   = (unsigned int*)seg_address;
    unsigned long *lseed = (unsigned long *)seed;
    unsigned int rand_no = *(unsigned long*)seed;
    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_32(lseed);
        if(*ptr != rand_no) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long )rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            rc = i+1;
            break;
        }
        *lseed = (unsigned long)rand_no;
        ptr = ptr + 1;
        rc = 0;
    }
    *(unsigned long*)seed = (unsigned long) rand_no;
    return rc;
}

int rand_operation_comp_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned char *ptr   = (unsigned char*)seg_address;
    unsigned long *lseed = (unsigned long *)seed;
    unsigned char rand_no = *(unsigned long*)seed;
    for (i=0;i<num_operations;i++){
        rand_no = get_random_no_8(lseed);
        if(*ptr != rand_no) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)rand_no,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                #endif
            }
            rc = i+1;
            break;
        }
        *lseed = (unsigned long)rand_no;
        ptr = ptr + 1;
        rc = 0;
    }
    *(unsigned long*)seed = (unsigned long) rand_no;
    return rc;
}

int rand_operation_rim_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned long  *w_ptr = (unsigned long *)seg_address;
    unsigned long  *lseed = (unsigned long *)seed;
    unsigned long rand_no = *(unsigned long*)seed,read_dw_data;
    for (i=0;i < num_operations;i++){
        rand_no = get_random_no_64(lseed);
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
            rc = i+1;
            break;
        }
        *lseed = (unsigned long)rand_no;
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    *(unsigned long*)seed = (unsigned long) rand_no;
    return rc;
}

int rand_operation_rim_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned int *w_ptr = (unsigned int*)seg_address;
    unsigned long *lseed = (unsigned long *)seed;
    unsigned int rand_no = *(unsigned long*)seed,read_data;
    for (i=0;i < num_operations;i++){
        rand_no = get_random_no_32(lseed);
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
            rc = i+1;
            break;
        }
        *lseed = (unsigned long)rand_no;
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    *(unsigned long*)seed = (unsigned long) rand_no;
    return rc;
}

int rand_operation_rim_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned char *w_ptr = (unsigned char*)seg_address;
    unsigned long *lseed = (unsigned long *)seed;
    unsigned char rand_no= *(unsigned long*)seed,read_data;
    for (i=0;i < num_operations;i++){
        rand_no = get_random_no_8(lseed);
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
            rc = i+1;
            break;
        }
        *lseed = (unsigned long)rand_no;
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    *(unsigned long*)seed = (unsigned long) rand_no;
    return rc;
}

