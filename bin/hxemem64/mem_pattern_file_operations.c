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

int pat_operation_write_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {
    int i=0,rc=0,pat_itr=0;
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

int pat_operation_write_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    int i=0,rc=0,pat_itr=0;
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

int pat_operation_write_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    int i=0,rc=0,pat_itr=0;
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
int pat_operation_comp_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    int i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned long *ptr = (unsigned long *)seg_address, *loc_pattern_ptr = (unsigned long *)pattern_ptr;
    unsigned int pat_limit    = pat_size/8;
    
    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            if(*ptr != *loc_pattern_ptr) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
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
        
int pat_operation_comp_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    int i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned int *ptr = (unsigned int*)seg_address, *loc_pattern_ptr = (unsigned int*)pattern_ptr;
    unsigned int pat_limit    = pat_size/8;

    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            if(*ptr != *loc_pattern_ptr) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
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

int pat_operation_comp_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed) {

    int i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    char *ptr = (char *)seg_address, *loc_pattern_ptr = (char*)pattern_ptr;
    unsigned int pat_limit    = pat_size/8;

    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            if(*ptr != *loc_pattern_ptr) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
                rc = i+1;
                return (rc);
            }
            loc_pattern_ptr = loc_pattern_ptr + 1;
            ptr             = ptr + 1;
        }
        loc_pattern_ptr = (char*)pattern_ptr;
    }
    return (rc);
}

int  pat_operation_rim_dword(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned long *w_ptr = (unsigned long *)seg_address, *loc_pattern_ptr = (unsigned long *)pattern_ptr;
    unsigned long read_dw_data;
    unsigned int pat_limit    = pat_size/8;
    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            *w_ptr = *(unsigned long *)loc_pattern_ptr;
            read_dw_data = *(unsigned long *)w_ptr;
            dcbf((volatile unsigned long*)w_ptr);
            read_dw_data = *(unsigned long *)w_ptr;
            if(read_dw_data != *(unsigned long *)loc_pattern_ptr) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
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

int pat_operation_rim_word(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    unsigned int *w_ptr = (unsigned int*)seg_address, *loc_pattern_ptr = (unsigned int*)pattern_ptr;
    unsigned int read_dw_data;
    unsigned int pat_limit    = pat_size/8;
    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            *w_ptr = *(unsigned int*)loc_pattern_ptr;
            read_dw_data = *(unsigned int*)w_ptr;
            dcbf((volatile unsigned int*)w_ptr);
            read_dw_data = *(unsigned int*)w_ptr;
            if(read_dw_data != *(unsigned int*)loc_pattern_ptr) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif

                }
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

int pat_operation_rim_byte(int num_operations,void *seg_address,void *pattern_ptr,int trap_flag,void *seg,void *stanza,void *pattern_sz_ptr,void* seed){
    int i=0,rc=0,pat_itr=0;
    unsigned int  pat_size = *(unsigned int*)pattern_sz_ptr;
    char *w_ptr = (char*)seg_address, *loc_pattern_ptr = (char*)pattern_ptr;
    char read_dw_data;
    unsigned int pat_limit    = pat_size/8;
    for (i=0; i < num_operations/pat_limit; i++) {
        for(pat_itr=0;pat_itr < pat_limit;pat_itr++){
            *w_ptr = *(char*)loc_pattern_ptr;
            read_dw_data = *(char*)w_ptr;
            dcbf((volatile char*)w_ptr);
            read_dw_data = *(char*)w_ptr;
            if(read_dw_data != *(char*)loc_pattern_ptr) {
                if(trap_flag){
                    #ifndef __HTX_LINUX__
                    trap(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza);
                    #else
                    do_trap_htx64(0xBEEFDEAD,i,(unsigned long)seg_address,(unsigned long)pattern_ptr,(unsigned long)w_ptr,(unsigned long)seg,(unsigned long)stanza,0);
                    #endif
                }
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

