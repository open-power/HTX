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
extern struct nest_global g_data;
extern struct mem_exer_info mem_g;

int do_stride_operation(int t){
    int pi=0,seg,rc=SUCCESS ,misc_detected=0;
    struct segment_detail sd;
    void *shr_mem_cur_ptr;
    int num_oper=0,nw=0,nr=0,nc=0;
    unsigned long no_of_strides, oper_per_stride;
    static int main_misc_count = 0;
    int miscompare_count, trap_flag;
    unsigned long seg_seed, seed;
    struct thread_data* th = &g_data.thread_ptr[t];
    struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[t]);
    th->testcase_thread_details = (void *)local_ptr;

    sd.oper = OPER_STRIDE;
    if( (g_data.stanza_ptr->misc_crash_flag) && (g_data.kdb_level) ){
        trap_flag = 1;    /* Trap is set */
    }else{
        trap_flag = 0;    /* Trap is not set */
    }
    if(g_data.stanza_ptr->attn_flag){
        trap_flag = 2;    /* attn is set */
    }

    if (g_data.stanza_ptr->seed == 0 ) {
        seed = (unsigned int)time(NULL);
    } else {
        seed = (unsigned int)g_data.stanza_ptr->seed;
    }
    for (num_oper =0;num_oper < g_data.stanza_ptr->num_oper; num_oper++){
        for (seg=0;seg<local_ptr->num_segs;){
            shr_mem_cur_ptr = (void *)local_ptr->seg_details[seg].shm_mem_ptr;
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
            seg_seed = seed;
            local_ptr->seg_details[seg].in_use = 1;

            if( g_data.stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS ) {
                sd.width = LS_DWORD;
            } else {
                sd.width = g_data.stanza_ptr->width;
            }        
            no_of_strides = (sd.seg_size / g_data.stanza_ptr->stride_sz);
            oper_per_stride = (g_data.stanza_ptr->stride_sz / g_data.stanza_ptr->pattern_size[pi]);

            nw = g_data.stanza_ptr->num_writes;
            nr = g_data.stanza_ptr->num_reads;
            nc = g_data.stanza_ptr->num_compares;

            while(nw>0 || nr>0 || nc>0){
                if(nw>0){
                    seed = seg_seed;
					srand48_r(seed,&local_ptr->buffer);
                    switch (sd.width) {
                        case LS_DWORD:
                            rc = write_dword(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi,local_ptr);
                            break;
                        case LS_WORD:
                            rc = write_word(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi,local_ptr);
                            break;
                        case LS_BYTE:
                            rc = write_byte(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi,local_ptr);
                            break;
                    }
                    nw--;
                }
                if (g_data.exit_flag == 1) {
                    goto exit;                
                }
                if(nr>0){
                    seed = seg_seed;
					srand48_r(seed,&local_ptr->buffer);
                    switch (g_data.stanza_ptr->width) {
                        case LS_DWORD:
                            rc = read_dword(shr_mem_cur_ptr, no_of_strides, oper_per_stride);
                            break;
                        case LS_WORD:
                            rc = read_word(shr_mem_cur_ptr, no_of_strides, oper_per_stride);
                            break;
                        case LS_BYTE:
                            rc = read_byte(shr_mem_cur_ptr, no_of_strides, oper_per_stride);
                            break;
                    }
                    nr--;
                }
                if (g_data.exit_flag == 1) {
                    goto exit;
                }
                if(nc>0){
                    seed = seg_seed;
					srand48_r(seed,&local_ptr->buffer);
                    switch (g_data.stanza_ptr->width) {
                        case LS_DWORD:
                            rc = read_comp_dword(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi,trap_flag,&sd);
                            break;
                        case LS_WORD:
                            rc = read_comp_word(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi,trap_flag,&sd);
                            break;
                        case LS_BYTE:
                            rc = read_comp_byte(shr_mem_cur_ptr, no_of_strides, oper_per_stride, pi,trap_flag,&sd);
                            break;
                    }
					if(rc != SUCCESS){
                    	displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:compare_data returned %d for compare count %d\n",
                    	__LINE__,__FUNCTION__,rc,nc);
						break;
					}
                    nc--;
                }
                if (g_data.exit_flag == 1) {
                    goto exit;
                }
            }
            if(rc != SUCCESS){/* If there is any miscompare, rc will have the offset */
                miscompare_count = dump_miscompared_buffers(t,rc,seg,main_misc_count,&seg_seed,num_oper,trap_flag,pi,&sd);
                STATS_VAR_INC(bad_others, 1);
                main_misc_count++;
                misc_detected++ ;
                STATS_VAR_INC(bad_others, 1)
                STATS_HTX_UPDATE(UPDATE)
            }
            if (g_data.exit_flag == 1) {
                goto exit;
            } /* endif */
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

        exit:
            STATS_VAR_INC(bytes_writ,((g_data.stanza_ptr->num_writes - nw) * sd.seg_size))
            STATS_VAR_INC(bytes_read,((g_data.stanza_ptr->num_reads - nr) * sd.seg_size))
            STATS_VAR_INC(bytes_read,((g_data.stanza_ptr->num_compares - nc - misc_detected) * sd.seg_size))
            /*STATS_HTX_UPDATE(UPDATE)*/
            misc_detected = 0;
            if (g_data.exit_flag == 1) {
                return -1;
            }
        }/*seg loop*/

    }/* num_oper loop*/
    return SUCCESS;
}
/*******************************************************************************
 *  fucntions to store a double word for nstride test case
 *******************************************************************************/
int write_dword(void *addr, unsigned long no_of_strides, int oper_per_stride, int pi, struct mem_exer_thread_info *th)
{
    unsigned long m,l=0,rand_no,*ptr = (unsigned long *) addr;
    int k,n=0, i;

    for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
        for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < (g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width);i++){
                if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS) {
                     ptr[l + i] = (unsigned long)(ptr + l + i);
                } else if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
                    rand_no  = get_random_no_64(&th->buffer);
                    ptr[l + i] = rand_no;
                } else {
                    ptr[l + i] = *((unsigned long *)(g_data.stanza_ptr->pattern[pi] + i)); /* access pattern */
                }
            } /* End loop i */
            l = l + g_data.stanza_ptr->stride_sz/g_data.stanza_ptr->width;
        } /* End loop m */
        n = n + g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width;
        l = n;
    }  /* End loop k */
    return 0;
} /* end write_dword */

/*******************************************************************************
 *  fucntions to read a double word for nstride test case
 *******************************************************************************/
int read_dword(void *addr, unsigned long no_of_strides, int oper_per_stride)
{
    volatile unsigned long buf;
    unsigned long *ptr = (unsigned long *) addr;
    unsigned long m,l=0,consume_data=0;
    int k,n=0;

    for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
        for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
            buf = ptr[l];
            consume_data += (buf+m+k);
            l = l + g_data.stanza_ptr->stride_sz/g_data.stanza_ptr->width;
        } /* End loop m */
        n = n + 1;
        l = n;
    }  /* End loop k */
    mem_g.dummy_read_data = consume_data;/*to avoide compiler optimization*/
    return 0;
} /* end read_dword */

/*******************************************************************************
 *  fucntions to read/Comp a double word for nstride test case
 *******************************************************************************/
int read_comp_dword(void *addr, unsigned long no_of_strides, int oper_per_stride, int pi,int trap_flag,struct segment_detail *seg)
{
    unsigned long m,l=0,buf, expected_val, *ptr = (unsigned long *) addr;
    int k,n=0, rc = 0, i;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);

    for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
        for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < (g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width);i++){
                buf = ptr[l + i];
                if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_ADDRESS) {
                    expected_val = (unsigned long)(ptr + l + i);
                } else if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
                    expected_val = (unsigned long)get_random_no_64(&th->buffer);
                } else {
                    expected_val = *((unsigned long *)(g_data.stanza_ptr->pattern[pi] + i)); /* access pattern */
                }
                if (buf != expected_val) {
                    if(trap_flag){
                    #ifndef __HTX_LINUX__
                        trap(0xBEEFDEAD,(l+i),(unsigned long)addr,(unsigned long)expected_val,(unsigned long)(ptr + l + i),(unsigned long)seg,(unsigned long)g_data.stanza_ptr);
                    #else
                        do_trap_htx64(0xBEEFDEAD,(l+i),(unsigned long)addr,(unsigned long)expected_val,(unsigned long)(ptr + l + i),(unsigned long)seg,(unsigned long)g_data.stanza_ptr,0);
                    #endif
                    }
                    displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%lx\tactual data = 0x%lx(addr:%p\n",__LINE__,__FUNCTION__,
                        expected_val,buf,&ptr[l + i]);
                    rc = l + i + 1;
                    return rc;
                }
            } /* End loop i */
            l = l + g_data.stanza_ptr->stride_sz/g_data.stanza_ptr->width;
        } /* End loop m */
        n = n + g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width;
        l = n;
    }  /* End loop k */
    return rc;
} /* end read_comp_dword */

/*******************************************************************************
 *  fucntions to store a word for nstride test case
 *******************************************************************************/
int write_word(void *addr, unsigned long no_of_strides, int oper_per_stride, int pi, struct mem_exer_thread_info *th)
{
    unsigned int rand_no, *ptr = (unsigned int *) addr;
    unsigned long m,l=0;
    int k,n=0, i;

    for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
        for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
            /*for(i=0; i < g_data.stanza_ptr->pattern_size[pi]; i+=g_data.stanza_ptr->width) {*/ /* Loop for pattern_size */
			for(i=0; i < (g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width);i++){
                if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
                    rand_no = (unsigned int)get_random_no_32(&th->buffer);
                    ptr[l + i] = rand_no;
                } else {
                    ptr[l + i] = *((unsigned int *)(g_data.stanza_ptr->pattern[pi] + i)); /* access pattern */
                }
            } /* End loop i */
            l = l + g_data.stanza_ptr->stride_sz/g_data.stanza_ptr->width;
        } /* End loop m */
        n = n + g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width;
        l = n;
    }  /* End loop k */
    return 0;
} /* end write_word */

/*******************************************************************************
 *  fucntions to read a word for nstride test case
 *******************************************************************************/
int read_word(void *addr, unsigned long no_of_strides, int oper_per_stride)
{
    volatile unsigned int buf;
    unsigned int *ptr = (unsigned int *) addr;
    unsigned long m,consume_data=0,l=0;
    int k,n=0;

    for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
        for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
            buf = ptr[l];
            consume_data += (buf+m+k);/*to avoide compiler optimization*/
            l = l + g_data.stanza_ptr->stride_sz/g_data.stanza_ptr->width;
        } /* End loop m */
        n = n + 1;
        l = n;
    }  /* End loop k */
    mem_g.dummy_read_data = consume_data;/*to avoide compiler optimization*/
    return 0;
} /* end read_word */

/*******************************************************************************
 *  fucntions to read/Comp a word for nstride test case
 *******************************************************************************/
int read_comp_word(void *addr, unsigned long no_of_strides, int oper_per_stride, int pi, int trap_flag,struct segment_detail *seg)
{
    unsigned int buf, expected_val, *ptr = (unsigned int *) addr;
    unsigned long m,l=0;
    int k,n=0, rc = 0, i;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);


    for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
        for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < (g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width);i++){
                buf = ptr[l + i];
                if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
                    expected_val = (unsigned int)get_random_no_32(&th->buffer);
                } else {
                    expected_val = *((unsigned int *)(g_data.stanza_ptr->pattern[pi] + i)); /* access pattern */
                }
                if (buf != expected_val) {
					if(trap_flag){
					#ifndef __HTX_LINUX__
						trap(0xBEEFDEAD,(l+i),(unsigned long)addr,(unsigned long)expected_val,(unsigned long)(ptr + l + i),(unsigned long)seg,(unsigned long)g_data.stanza_ptr);
					#else
						do_trap_htx64(0xBEEFDEAD,(l+i),(unsigned long)addr,(unsigned long)expected_val,(unsigned long)(ptr + l + i),(unsigned long)seg,(unsigned long)g_data.stanza_ptr,0);
					#endif
					}
                    displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x(addr:%p)\n",__LINE__,__FUNCTION__,
                        expected_val,buf,&ptr[l + i]);
                    rc = l + i + 1;
                    return rc;
                }
            } /* End loop i */
            l = l + g_data.stanza_ptr->stride_sz/g_data.stanza_ptr->width;
        } /* End loop m */
        n = n + g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width;
        l = n;
    }  /* End loop k */
    return rc;
} /* end read_comp_word */

/*******************************************************************************
 *  fucntions to store a byte for nstride test case
 *******************************************************************************/
int write_byte(void *addr, unsigned long no_of_strides, int oper_per_stride, int pi, struct mem_exer_thread_info *th)
{
    char rand_no, *ptr = (char *) addr;
    unsigned long m,l=0;
    int k,n=0, i;

    for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
        for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < (g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width);i++){
                if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
                    rand_no = (char)get_random_no_8(&th->buffer);
                    ptr[l + i] = rand_no;
                } else {
                    ptr[l + i] = *((char *)(g_data.stanza_ptr->pattern[pi] + i)); /* access pattern */
                }
            } /* End loop i */
            l = l + g_data.stanza_ptr->stride_sz/g_data.stanza_ptr->width;
        } /* End loop m */
        n = (n + g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width);
        l = n;
    }  /* End loop k */
    return 0;
} /* end write_byte */

/*******************************************************************************
 *  fucntions to read a byte for nstride test case
 *******************************************************************************/
int read_byte(void *addr, unsigned long no_of_strides, int oper_per_stride)
{
    volatile char buf;
    char  *ptr = (char *) addr;
    int k,n=0;
    unsigned long m,consume_data=0,l=0;

    for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
        for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
            buf = ptr[l];
            consume_data += (buf+m+k);/*to avoide compiler optimization*/
            l = l + g_data.stanza_ptr->stride_sz/g_data.stanza_ptr->width;
        } /* End loop m */
        n = n + 1;
        l = n;
    }  /* End loop k */
    mem_g.dummy_read_data = consume_data;/*to avoide compiler optimization*/
    return 0;
} /* end read_byte */

/*******************************************************************************
 *  fucntions to read/Comp a byte for nstride test case
 *******************************************************************************/
int read_comp_byte(void *addr, unsigned long no_of_strides, int oper_per_stride, int pi,int trap_flag,struct segment_detail *seg)
{
    char buf, expected_val, *ptr = (char *) addr;
    int k,n=0, rc = 0, i;
    struct segment_detail* sd = (struct segment_detail*) seg;
    struct mem_exer_thread_info *th = &(mem_g.mem_th_data[sd->owning_thread]);
    unsigned long m,l=0;


    for (k=0; k < oper_per_stride; k++) { /* Outer loop for the no. of load/store oper. within a given stride */
        for (m=0; m < no_of_strides; m++) {  /* Inner loop for the no. of stride in a given segment */
			for(i=0; i < (g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width);i++){
                buf = ptr[l + i];
                if (g_data.stanza_ptr->pattern_type[pi] == PATTERN_RANDOM) {
                    expected_val = (char)get_random_no_8(&th->buffer);
                } else {
                    expected_val = *((char *)(g_data.stanza_ptr->pattern[pi] + i)); /* access pattern */
                }
                if (buf != expected_val) {
                   	if(trap_flag){
                    #ifndef __HTX_LINUX__
                        trap(0xBEEFDEAD,(l+i),(unsigned long)addr,(unsigned long)expected_val,(unsigned long)(ptr + l + i),(unsigned long)seg,(unsigned long)g_data.stanza_ptr);
                    #else
                        do_trap_htx64(0xBEEFDEAD,(l+i),(unsigned long)addr,(unsigned long)expected_val,(unsigned long)(ptr + l + i),(unsigned long)seg,(unsigned long)g_data.stanza_ptr,0);
                    #endif
                    }
                    displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:1st read_comp failure,expected data = 0x%x\tactual data = 0x%x (addr:%p)\n",__LINE__,__FUNCTION__,
                        expected_val,buf,&ptr[l + i]);
                    rc = l + i + 1;
                    return rc;
                }
            } /* End loop i */
            l = l + g_data.stanza_ptr->stride_sz/g_data.stanza_ptr->width;
        } /* End loop m */
        n = n + g_data.stanza_ptr->pattern_size[pi]/g_data.stanza_ptr->width;
        l = n;
    }  /* End loop k */
    return rc;
} /* end read_comp_byte */

