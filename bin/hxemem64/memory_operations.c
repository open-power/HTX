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
extern struct mem_exer_info mem_g;

int mem_operation_write_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned long *rw_ptr = (unsigned long *)seg_address;
    for (i=0; i < num_operations; i++) {
        *rw_ptr = *(unsigned long *)pattern_ptr;
        rw_ptr = rw_ptr + 1;
    }
    rc = 0;
    return (rc);

}

int mem_operation_write_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned int *rw_ptr = (unsigned int *)seg_address;
    for (i=0; i < num_operations; i++) {
        *rw_ptr = *(unsigned int*)pattern_ptr;
        rw_ptr = rw_ptr + 1;
    }
    rc = 0;
    return (rc);

}
int mem_operation_write_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned char *rw_ptr = (unsigned char *)seg_address;
    for (i=0; i < num_operations; i++) {
        *rw_ptr = *(unsigned char*)pattern_ptr;
        rw_ptr = rw_ptr + 1;
    }
    rc = 0;
    return (rc);

}
int mem_operation_read_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
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

int mem_operation_read_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
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
int mem_operation_read_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
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
int mem_operation_comp_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    volatile unsigned long *ptr = (unsigned long *)seg_address, *loc_pattern_ptr = (unsigned long *)pattern_ptr;
    int i=0,rc=0;
    for (i=0; i < num_operations; i++) {
        if(*ptr != *loc_pattern_ptr) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #endif
            }
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        rc = 0;
    } 
    return rc;
}
int mem_operation_comp_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    volatile unsigned int *ptr = (unsigned int *)seg_address, *loc_pattern_ptr = (unsigned int *)pattern_ptr;
    int i=0,rc=0;
    for (i=0; i < num_operations; i++) {
        if(*ptr != *loc_pattern_ptr) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #endif
            }
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        rc = 0;
    } 
    return rc;
}
int mem_operation_comp_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    volatile unsigned char *ptr = (unsigned char *)seg_address, *loc_pattern_ptr = (unsigned char *)pattern_ptr;
    int i=0,rc=0;
    for (i=0; i < num_operations; i++) {
        if(*ptr != *loc_pattern_ptr) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #endif
            }
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        rc = 0;
    } 
    return rc;
}

int mem_operation_write_addr(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
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

int mem_operation_comp_addr(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned long *ptr = (unsigned long *)seg_address ;
    unsigned long address = *(unsigned long *)seg_address;
    for (i=0; i < num_operations; i++) {
        if(*ptr != address) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                #endif
            }
            rc = i+1;
            break;
        }
        ptr = ptr + 1;
        address = ptr;
        rc = 0;
    }
    return rc;
}
int  mem_operation_rim_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned long *w_ptr = (unsigned long *)seg_address;
    unsigned long read_data;
    for (i=0; i < num_operations; i++) {
        *w_ptr = *(unsigned long *)pattern_ptr;
        read_data = *(unsigned long *)w_ptr;
        dcbf((volatile unsigned long*)w_ptr);
        read_data = *(unsigned long *)w_ptr;
        if(read_data != *(unsigned long *)pattern_ptr) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #endif
            }
            rc = i+1;
            break;
        }
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    return rc;
}
int  mem_operation_rim_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    unsigned int *w_ptr = (unsigned int*)seg_address;
    unsigned int read_data;
    for (i=0; i < num_operations; i++) {
        *w_ptr = *(unsigned int*)pattern_ptr;
        read_data = *(unsigned int*)w_ptr;
        dcbf((volatile unsigned int*)w_ptr);
        read_data = *(unsigned int*)w_ptr;
        if(read_data != *(unsigned int*)pattern_ptr) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #endif
            }

            rc = i+1;
            break;
        }
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    return rc;
}

int  mem_operation_rim_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
    char *w_ptr = (unsigned char*)seg_address;
    char read_data;
    for (i=0; i < num_operations; i++) {
        *w_ptr = *(char*)pattern_ptr;
        read_data = *(char*)w_ptr;
        dcbf((volatile char*)w_ptr);
        read_data = *(char*)w_ptr;
        if(read_data != *(char*)pattern_ptr) {
            if(trap_flag){
                #ifndef __HTX_LINUX__
                trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #else
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                #endif
            }

            rc = i+1;
            break;
        }
        w_ptr = w_ptr + 1;
        rc = 0;
    }
    return rc;
}

int mem_operation_write_addr_comp(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0;
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
                do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)rw_ptr,(unsigned long)seg,(unsigned long)stanza);
                #endif
            }
            rc = i+1;
            break;
        }
        rw_ptr  = rw_ptr + 1;
        address = rw_ptr;
        rc = 0;
    }
    return rc;
}


