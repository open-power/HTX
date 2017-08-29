#include "nest_framework.h"
#define BYTES_EXC  16
#ifndef __HTX_LINUX__
    void dcbtna(volatile unsigned long long);
    void dcbtds(volatile unsigned long long);
    void dcbtds_0xA(volatile unsigned long long);

#pragma mc_func dcbtna { "7E201A2C" }
/*
    dcbt 0, r3, 0b10001
    011111  10001  00000    00011   01000101100
    31      th      ra      rb      278
 */
#pragma mc_func dcbtds { "7D001A2C" }
/*
    dcbt 0,r3,01000
    011111  01000   00000   00011   01000101100
    31      th      ra      rb      278
*/
#pragma mc_func dcbtds_0xA {"7D401A2C" }
/*
    dcbt 0,r3,01010
    011111  01010   00000   00011   01000101100
    31      th      ra      rb      278
*/
#else
#define dcbtna(x) __asm ("dcbt 0,%0,0x11\n\t": : "r" (x))
#define dcbtds(x) __asm ("dcbt 0,%0,0x8\n\t": : "r" (x))
#define dcbtds_0xA(x) __asm ("dcbt 0,%0,0xA\n\t": : "r" (x))
#endif
#define mtspr_dscr(x) __asm volatile ("mtspr 3, %0\n\t": "=r"(x))
#define mfspr_dscr(x) __asm volatile ("mfspr %0, 3\n\t": : "r"(x))

struct cache_exer_info cache_g;
extern struct nest_global g_data;

void* prefetch_thread_function(void *tn){
    struct thread_data* th = (struct thread_data*)tn;
    struct cache_exer_thread_info *local_ptr = &cache_g.cache_th_data[th->thread_num];
    int rc=SUCCESS,oper,num_oper = g_data.stanza_ptr->num_oper;
    unsigned long long starting_address,random_no,memory_fetch_size,temp_storage = 0x1, temp_pattern = 0x1;
    int cache_type = g_data.stanza_ptr->tgt_cache;
    int cache_line_size = g_data.sys_details.cache_info[cache_type].cache_line_size;
    unsigned int loop_count;
    unsigned char *start_addr;
    long int offset;

    local_ptr->seedval = time(NULL);
    srand48_r(local_ptr->seedval,&local_ptr->buffer);

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
    starting_address = (unsigned long long)local_ptr->prefetch_memory_start_address;
    memory_fetch_size = (PREFETCH_MEMORY_PER_THREAD - BYTES_EXC);
    loop_count  = memory_fetch_size / cache_line_size;
    for (oper = 0; oper < num_oper; oper++) {
        if(g_data.exit_flag){
            pthread_exit((void *)1);
        }
        random_no = get_random_number_pref(th->thread_num);
        random_no = (unsigned long long)(random_no<<32) | (random_no);
        local_ptr->prev_seed = random_no;

        /* Now write DSCR if needed */
        if ( g_data.sys_details.true_pvr >= POWER8_MURANO ) {
            rc = prefetch_randomise_dscr(random_no, g_data.stanza_ptr->pf_dscr ,th->thread_num);
            if(rc != SUCCESS){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"prefetch_randomise_dscr() failed with rc = %d\n",rc);
                g_data.exit_flag = 1;
                pthread_exit((void *)1); 
            }
        }
        if (local_ptr->prefetch_algorithm == RR_ALL_ENABLED_PREFETCH_ALGORITHMS) {
            /* Run all the enabled prefetch variants in round robin method */

            /* If prefetch nstride is set in the current prefetch configuration */
            if ( (PREFETCH_NSTRIDE & g_data.stanza_ptr->pf_conf) == PREFETCH_NSTRIDE ) {
                n_stride(starting_address,memory_fetch_size,random_no,&local_ptr->prefetch_scratch_mem[0]);
            }
            if(g_data.exit_flag){
                pthread_exit((void *)1);
            }
           /* If prefetch partial is set in the current prefetch configuration */
            if ( (PREFETCH_PARTIAL & g_data.stanza_ptr->pf_conf) == PREFETCH_PARTIAL ) {
                partial_dcbt(starting_address,memory_fetch_size,random_no,&local_ptr->prefetch_scratch_mem[0]);
            }

            if(g_data.exit_flag){
                pthread_exit((void *)1);
            }
            if ( (PREFETCH_IRRITATOR &  g_data.stanza_ptr->pf_conf) == PREFETCH_IRRITATOR ) {
                rc = do_prefetch( starting_address , memory_fetch_size , random_no, th->thread_num, loop_count,local_ptr->pattern);
                if ( rc != 0 ) {
                    dump_miscompare_data(th->thread_num,(unsigned char *)starting_address);
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Miscompare in Prefetch!! Expected data = 0x%llx Actual data = 0x%llx "
                        "thread_index :0x%x Start of memory = %llx, memory size = 0x%llx\n",__LINE__,__FUNCTION__,local_ptr->pattern,
                        *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)),th->thread_num,starting_address,memory_fetch_size);
                    pthread_exit((void *)1);
                }
            }
            if(g_data.exit_flag){
                pthread_exit((void *)1);
            }
            if( (PREFETCH_TRANSIENT & g_data.stanza_ptr->pf_conf) == PREFETCH_TRANSIENT ) {
                offset      = random_no % (long)16;
                start_addr = (unsigned char *)starting_address + offset;
                rc = transient_dcbt((unsigned long long)start_addr,loop_count,local_ptr->pattern);
                if ( rc != 0 ) {
                    dump_miscompare_data(th->thread_num,(unsigned char *)starting_address);
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Miscompare in Prefetch!! Expected data = 0x%llx Actual data = 0x%llx "
                        "thread_index :0x%x Start of memory = %llx, memory size = 0x%llx\n",__LINE__,__FUNCTION__,local_ptr->pattern,
                        *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)),th->thread_num,starting_address,memory_fetch_size);
                    pthread_exit((void *)1);
                }
            }
            if(g_data.exit_flag){
                pthread_exit((void *)1);
            }
            if ( (PREFETCH_NA & g_data.stanza_ptr->pf_conf) == PREFETCH_NA ) {
                offset      = random_no % (long)16;
                start_addr = (unsigned char *)starting_address + offset;
                rc = prefetch_dcbtna((unsigned long long)start_addr, loop_count,local_ptr->pattern);
                if ( rc != 0 ) {
                    dump_miscompare_data(th->thread_num,(unsigned char *)starting_address);
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Miscompare in Prefetch!!Expected data = 0x%llx Actual data = 0x%llx copied data = %llx0x,"
                        " copied pattern = %llx0x, thread_index : 0x%x Start of memory = %llx,memory size = 0x%llx\n",__LINE__,__FUNCTION__,local_ptr->pattern,
                        *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)),temp_storage,temp_pattern,th->thread_num,starting_address,memory_fetch_size);
                        pthread_exit((void *)1);
                }
            }
            if(g_data.exit_flag){
                pthread_exit((void *)1);
            }
        }else{/* Else Run only the specified algorithm */
            if(local_ptr->prefetch_algorithm == PREFETCH_NSTRIDE) {
                /*lrand48_r(&th->buffer, &random_no);*/
                n_stride(starting_address, memory_fetch_size, random_no, &local_ptr->prefetch_scratch_mem[0]);
            }
            else if(local_ptr->prefetch_algorithm == PREFETCH_PARTIAL) {
                partial_dcbt(starting_address, memory_fetch_size, random_no, &local_ptr->prefetch_scratch_mem[0]);
            }else if ( local_ptr->prefetch_algorithm == PREFETCH_IRRITATOR ) {
                rc = do_prefetch( starting_address, memory_fetch_size , random_no,th->thread_num,loop_count,local_ptr->pattern);
                if ( rc != 0 ) {
                    dump_miscompare_data(th->thread_num, (unsigned char *)starting_address);
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Miscompare in Prefetch!! Expected data = 0x%llx Actual data = 0x%llx "
                        "thread_index :0x%x Start of memory = %llx, memory size = 0x%llx\n",__LINE__,__FUNCTION__,local_ptr->pattern,
                        *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)),th->thread_num,starting_address,memory_fetch_size);
                    pthread_exit((void *)1);
                }
            }else if( local_ptr->prefetch_algorithm == PREFETCH_TRANSIENT ) {
                offset      = random_no % (long)16;
                start_addr = (unsigned char *)starting_address + offset;
                rc = transient_dcbt((unsigned long long)start_addr,loop_count,local_ptr->pattern);
                if ( rc != 0 ) {
                    dump_miscompare_data(th->thread_num,(unsigned char *)starting_address);
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Miscompare in Prefetch!! Expected data = 0x%llx Actual data = 0x%llx "
                        "thread_index :0x%x Start of memory = %llx, memory size = 0x%llx\n",__LINE__,__FUNCTION__,local_ptr->pattern,
                        *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)),th->thread_num,starting_address,memory_fetch_size);
                    pthread_exit((void *)1);
                }
            }else if ( local_ptr->prefetch_algorithm == PREFETCH_NA ) {
                offset      = random_no % (long)16;
                start_addr = (unsigned char *)starting_address + offset;
                rc = prefetch_dcbtna((unsigned long long)start_addr, loop_count,local_ptr->pattern);
                if ( rc != 0 ) {
                    dump_miscompare_data(th->thread_num,(unsigned char *)starting_address);
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Miscompare in Prefetch!!Expected data = 0x%llx Actual data = 0x%llx copied data = %llx0x,"
                        " copied pattern = %llx0x, thread_index : 0x%x Start of memory = %llx,memory size = 0x%llx\n",__LINE__,__FUNCTION__,local_ptr->pattern,
                        *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)),temp_storage,temp_pattern,th->thread_num,starting_address,memory_fetch_size);
                        pthread_exit((void *)1);
                }
            }
            if(g_data.exit_flag){
                pthread_exit((void *)1);
            }
        }
    }
    htx_unbind_thread();
    pthread_exit((void *)1);
}

int do_prefetch( unsigned long long starting_address , unsigned long long memory_fetch_size , unsigned long long random_no,
                unsigned int thread_no, unsigned long long loop_count,unsigned long long pattern) {
    unsigned int        direction_bitmask   = 0x40; /* 0b 0100 0000 - 57th bit */
    unsigned long long  temp_mask = 0UL;;
    int                 rc,i,stream_id;
    unsigned long long  start_addr = starting_address;
    struct cache_exer_thread_info *local_ptr = &cache_g.cache_th_data[thread_no];
    unsigned long prefetch_streams = local_ptr->prefetch_streams;

    /*
     *  phase 1 dcbt ra,rb ,01000
     *  EA interpreted as shown below
     *     +-----------------------------------------+----+-+------+
     * EA  |                EATRUNC                  |D UG|/|  ID  |
     *     +-----------------------------------------+----+-+------+
     *     0                                         57  58  60    63
     */
    if ( starting_address & direction_bitmask ) {
        /*
           If 57th bit is set, the prefetching happens in backwards direction.
           If it is reset ( 0 ), prefetching happens in forward direction.
        */
        starting_address += memory_fetch_size;
    }

    /*
       Now, Create the Prefetch streams. Set the IDs of the streams being generated.
    */

    starting_address = starting_address >> 4;
    starting_address = starting_address << 4;
    for (i=0 ; i< prefetch_streams ; i++) {
        stream_id = (thread_no-1)*prefetch_streams + i;
        starting_address |= stream_id;
        dcbtds(starting_address);
    }
    /*
     *  phase 2 dcbt ra,rb,01010
     * EA is interpreted as follows.
     *
     *     +------------------+----+--+---+---+----------+---+-+----+
     * EA  |        ///       |GO S|/ |DEP|///| UNIT_CNT |T U|/| ID |
     *     +------------------+----+--+---+---+----------+---+-+----+
     *     0                 32  34 35  38    47        57 58 60  63
     * randomise DEPTH 36:38 and set U to 1 [unlimited number of data units ]
     */

    /* First clear out the upper 32 bits */
    starting_address &= 0xffffffff00000000;

    temp_mask >>=25;
    temp_mask <<= 25;
    temp_mask |= ((random_no & 0x7) << 25);
    temp_mask |= 0x0020;

    starting_address |= temp_mask;

    for ( i=0 ; i<prefetch_streams ; i++ ) {
        stream_id = (thread_no-1)*prefetch_streams + i;
        starting_address |= stream_id;
        dcbtds_0xA(starting_address);
    }

    /*
     * phase 3 dcbt ra,rb,01010 with go bits set
     *      +------------------+----+--+---+---+----------+---+-+----+
     *   EA |        ///       |GO S|/ |DEP|///| UNIT_CNT |T U|/| ID |
     *      +------------------+----+--+---+---+----------+---+-+----+
     *      0                 32  34 35  38    47        57 58 60  63
     *
     */

    /* set go field */
    starting_address |= 0x00008000;
    /* Zero out the last 4 bits (ID bits) */
    starting_address >>= 4;
    starting_address <<= 4;

    /*
     * One dcbt instruction with GO bit =1 is sufficient to kick off all the nascent streams .
     * dcbt 0,3,0xA
     */
    dcbtds_0xA(starting_address);

    /* Now that the stream has been described and kicked off, consume the stream. */
    rc = prefetch(start_addr, loop_count, pattern);

    return (rc);
}


/* This function is used to randomise the DSCR register.
 * The Inputs are a random number, and an integer parameter.
 * The integer parameters is to be interpreted as follows:
 * DSCR_RANDOM          = In this case the 64 bit random number is simply written to DSCR
 * DSCR_LSDISABLE       = In this case only bit 58 LSD is set. All other bits are randomised
 */

int prefetch_randomise_dscr(unsigned long long random_number, unsigned int what, unsigned int thread_no) {
/*
   Structure of DSCR
   +------------+-------+-------+-------+-------+-------+-------+-----------+-------+-------+-------+-------+-------+
   |    //      |  SWTE | HWTE  |  STE  |  LTE  | SWUE  | HWUE  |  UNIT CNT |  URG  |  LSD  | SNSE  |  SSE  | DPFD  |
   +------------+-------+-------+-------+-------+-------+-------+-----------+-------+-------+-------+-------+-------+
   0          3839      40      41      42      43      44      45        5455    5758      59      60      61      63
 */

    /*unsigned long long  local_random_number;*/
    unsigned long long  read_dscr = 0;
    int rc = SUCCESS;
    struct cache_exer_thread_info *local_ptr = &cache_g.cache_th_data[thread_no];

    mfspr_dscr(&read_dscr);
    local_ptr->read_dscr_val = read_dscr;
    switch ( what ) {
        case DSCR_RANDOM:
            mtspr_dscr(random_number);
            local_ptr->written_dscr_val = random_number;
            break;
        case DSCR_LSDISABLE:
            /*local_random_number = random_number | (0x1<<5);*/
            mtspr_dscr(random_number);
            local_ptr->written_dscr_val = random_number;
            /*printf("[%d] Original random number = 0x%x , After setting 58th bit = 0x%x\n",__LINE__,random_number,local_random_number);*/
            break;
        case DSCR_DEFAULT:
            break;
        default:
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Wrong parameter = %d,for dscr in rule file\n",__LINE__,__FUNCTION__,what);        
            rc = FAILURE;
            break;
    }
    return rc;
}

int get_random_number_pref(int t){
    unsigned long int val_8 ;
    struct cache_exer_thread_info *local_ptr = &cache_g.cache_th_data[t];
    lrand48_r(&local_ptr->buffer, &local_ptr->random_pattern);
    val_8 = (local_ptr->random_pattern <<16)| local_ptr->random_pattern ;

    return val_8 ;
}
