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

int pat_operation_write_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {
    unsigned long i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned long *rw_ptr          = (unsigned long *)seg_address;
    unsigned long *loc_pattern_ptr = (unsigned long *)pattern_ptr;
    unsigned int pat_limit    = pat_size/8;

    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            *rw_ptr         = *(unsigned long *)loc_pattern_ptr;
            rw_ptr         = rw_ptr + 1;
            loc_pattern_ptr = loc_pattern_ptr +1;
        }
        loc_pattern_ptr = (unsigned long *)pattern_ptr;
    }
    return (rc);
}

int pat_operation_write_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    unsigned long i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned int *rw_ptr          = (unsigned int*)seg_address;
    unsigned int *loc_pattern_ptr = (unsigned int*)pattern_ptr;
    unsigned int pat_limit    = pat_size/8;

    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            *rw_ptr         = *(unsigned int*)loc_pattern_ptr;
            rw_ptr         = rw_ptr + 1;
            loc_pattern_ptr = loc_pattern_ptr +1;
        }
        loc_pattern_ptr = (unsigned int*)pattern_ptr;
    }
    return (rc);
}

int pat_operation_write_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    unsigned long i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    char *rw_ptr          = (char*)seg_address;
    char *loc_pattern_ptr = (char*)pattern_ptr;
    unsigned int pat_limit    = pat_size/8;

    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            *rw_ptr         = *(char*)loc_pattern_ptr;
            rw_ptr         = rw_ptr + 1;
            loc_pattern_ptr = loc_pattern_ptr +1;
        }
        loc_pattern_ptr = (char*)pattern_ptr;
    }
    return (rc);
}
int pat_operation_comp_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    unsigned long i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned long *ptr = (unsigned long *)seg_address, *loc_pattern_ptr = (unsigned long *)pattern_ptr;
    unsigned long temp_val1,temp_val2;
    unsigned int pat_limit    = pat_size/8;
    
    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            if( (temp_val1 = *ptr) != (temp_val2 = *loc_pattern_ptr)) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx\n",__LINE__,__FUNCTION__,
                    temp_val2,temp_val1);
                rc = i+1;
                return (rc);
            }
            loc_pattern_ptr = loc_pattern_ptr + 1;
            ptr             = ptr + 1;
        }
        loc_pattern_ptr = (unsigned long *)pattern_ptr;
    }
    return (rc);
}
        
int pat_operation_comp_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    unsigned long i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned int *ptr = (unsigned int*)seg_address, *loc_pattern_ptr = (unsigned int*)pattern_ptr;
    unsigned int temp_val1,temp_val2;
    unsigned int pat_limit    = pat_size/8;

    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            if( (temp_val1 = *ptr) != (temp_val2 = *loc_pattern_ptr) ) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x\n",__LINE__,__FUNCTION__,
                    temp_val2,temp_val1);
                rc = i+1;
                return (rc);
            }
            loc_pattern_ptr = loc_pattern_ptr + 1;
            ptr             = ptr + 1;
        }
        loc_pattern_ptr = (unsigned int*)pattern_ptr;
    }
    return (rc);
}

int pat_operation_comp_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    unsigned long i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned char *ptr = (unsigned char *)seg_address, *loc_pattern_ptr = (char*)pattern_ptr;
    unsigned char temp_val1,temp_val2;
    unsigned int pat_limit    = pat_size/8;

    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            if( (temp_val1 = *ptr) != (temp_val2 = *loc_pattern_ptr) ) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x\n",__LINE__,__FUNCTION__,
                    temp_val2,temp_val1);
                rc = i+1;
                return (rc);
            }
            loc_pattern_ptr = loc_pattern_ptr + 1;
            ptr             = ptr + 1;
        }
        loc_pattern_ptr = (unsigned char*)pattern_ptr;
    }
    return (rc);
}

int  pat_operation_rim_dword(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned long *w_ptr = (unsigned long *)seg_address, *loc_pattern_ptr = (unsigned long *)pattern_ptr;
    unsigned long read_dw_data,temp_val;
    unsigned int pat_limit    = pat_size/8;
    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            *w_ptr = *(unsigned long *)loc_pattern_ptr;
            read_dw_data = *(unsigned long *)w_ptr;
            dcbf((volatile unsigned long*)w_ptr);
            read_dw_data = *(unsigned long *)w_ptr;
            if(read_dw_data != (temp_val = *(unsigned long *)loc_pattern_ptr)) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx\n",__LINE__,__FUNCTION__,
                    temp_val,read_dw_data);
                rc = i+1;
                return (rc);
            }
            w_ptr = w_ptr + 1;
            loc_pattern_ptr = loc_pattern_ptr + 1;
        }
        loc_pattern_ptr = (unsigned long *)pattern_ptr;
    }
    return (rc);
}

int pat_operation_rim_word(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned int *w_ptr = (unsigned int*)seg_address, *loc_pattern_ptr = (unsigned int*)pattern_ptr;
    unsigned int read_data,temp_val;
    unsigned int pat_limit    = pat_size/8;
    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            *w_ptr = *(unsigned int*)loc_pattern_ptr;
            read_data = *(unsigned int*)w_ptr;
            dcbf((volatile unsigned int*)w_ptr);
            read_data = *(unsigned int*)w_ptr;
            if(read_data != (temp_val = *(unsigned int*)loc_pattern_ptr) ) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif

                }
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x\n",__LINE__,__FUNCTION__,
                    temp_val,read_data);
                rc = i+1;
                return (rc);
            }
            w_ptr = w_ptr + 1;
            loc_pattern_ptr = loc_pattern_ptr + 1;
        }
        loc_pattern_ptr = (unsigned int*)pattern_ptr;
    }
    return (rc);
}

int pat_operation_rim_byte(unsigned long num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    unsigned long i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    char *w_ptr = (char*)seg_address, *loc_pattern_ptr = (char*)pattern_ptr;
    char read_data,temp_val;
    unsigned int pat_limit    = pat_size/8;
    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            *w_ptr = *(char*)loc_pattern_ptr;
            read_data = *(char*)w_ptr;
            dcbf((volatile char*)w_ptr);
            read_data = *(char*)w_ptr;
            if(read_data != (temp_val = *(char*)loc_pattern_ptr)) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x\n",__LINE__,__FUNCTION__,
                    temp_val,read_data);
                rc = i+1;
                return (rc);
            }
            w_ptr = w_ptr + 1;
            loc_pattern_ptr = loc_pattern_ptr + 1;
        }
        loc_pattern_ptr = (char*)pattern_ptr;
    }
    return (rc);
}

