/* IBM_PROLOG_BEGIN_TAG */
/* 
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * 		 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */
/* @(#)28       1.11  src/htx/usr/lpp/htx/bin/hxecache/prefetch_irritator.c, exer_cache, htxubuntu 9/24/15 01:14:26  */

#include "hxecache.h"

#define BYTES_EXC 16    /* Last few bytes of segment where it loads from has to be within segemnt */
                        /* ld => 16 bytes  */
/* Layout of thread specific data segment
 * address:            .long 0,0
 * endaddress:        .long 0,0
 * time_seed:        .long   0x0
 * mask:            .long 0x3ffffff0  # GO/STOP and STREAM IDS are set to 0 [only in prefetch.s]
 * direction:        .long 0x0
 * proc_no:            .long proc_no
 * 64 bytes
 */

extern struct htx_data          h_d;
extern char                     msg[500];
extern struct memory_set        mem;
extern struct thread_context    th_array[MAX_CPUS];
extern volatile int             exit_flag;
extern int 						pcpus_thread_wise[];
extern int 						tot_thread_count;
unsigned int					prefetch_streams;
int 							do_prefetch ( unsigned long long starting_address , unsigned long long memory_fetch_size , unsigned long long random_no, unsigned int thread_no, unsigned long long loop_count, unsigned long long pattern);
extern volatile int				prefetch(unsigned long long , unsigned long long , unsigned long long );
extern void                     n_stride(unsigned long long , unsigned long long , unsigned long long ,unsigned long long*);
extern volatile int				transient_dcbt(unsigned long long , unsigned int, unsigned long long );
extern void                     partial_dcbt(unsigned long long , unsigned long long , unsigned long long ,unsigned long long*);
extern volatile int				prefetch_dcbtna(unsigned long long starting_address, unsigned int loop_count, unsigned long long pattern, unsigned long long *temp_storage, unsigned long long *temp_pattern);
int								prefetch_dcbtna_dbg(unsigned long long starting_address, unsigned long long memory_fetch_size, unsigned long long pattern, unsigned int loop_count, unsigned int cache_line_size);
int								prefetch_randomise_dscr(unsigned long long random_no, unsigned int what, unsigned int thread_no);
extern struct sys_info          system_information;
unsigned long int               get_random_number_perf(int );

#ifdef __HTX_LINUX__
    extern cpu_set_t default_cpu_affinity_mask;
#endif

void prefetch_irritator(void *arg)
{
    int i, rc, no_of_pages, tid , thread_no, tc, oper , number_of_operations;
    unsigned long long  saved_seed, random_no , starting_address , memory_fetch_size;
	pthread_t ptid;
	unsigned char *start_addr;
    struct thread_context *th = (struct thread_context *)arg;
 	struct ruleinfo *current_rule 	= th->current_rule;
	int cache_type = current_rule->tgt_cache;
	int cache_line_size = system_information.cinfo[cache_type].line_size;
	unsigned int loop_count 	;
	long int	offset;
	unsigned long long temp_storage = 0x1, temp_pattern = 0x1;

    /*
     * char *contig_mem[NUM_SEGS*SEG_SIZE/(16*M)]; Physically contiguous
     * memory pointer. memory_set_size variable  gives total memory
     * allocated both are variables of global structure.
     */

    thread_no = th->thread_no ;
	int pcpu = pcpus_thread_wise[thread_no];
    tid       = th->bind_to_cpu;    /* Bind to the processor */
	ptid	  = th->tid;			/* PThread Id for this thread	*/
	prefetch_streams 	= th->prefetch_streams;		/* Number of prefetch streams for this thread. 	*/

    if (current_rule->testcase_type != PREFETCH_ONLY) {
        /* Set Thread Cancel Type as ASYNCHRONOUS */
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    }

    #ifdef __HTX_LINUX__
       /*printf(" Prefetch:calling htx_bind with pcpu=%d for thread_no= %d\n",pcpu,thread_no);*/
    if(pcpu == -1){
        pcpu = htx_bind_thread(tid, -1);
        rc = pcpu;
        pcpus_thread_wise[thread_no]=pcpu;
        if(pcpu < 0){
            pcpus_thread_wise[thread_no]= -1;
        }

    }
    else {
        rc = htx_bind_thread(tid,pcpu);
    }
    #else
        rc = bindprocessor(BINDTHREAD, thread_self(), tid);
    #endif
		DEBUG_LOG("[%d] thread %d, binding to cpu %d \n",__LINE__,thread_no,tid);
	    
		if(rc < 0) {
	#ifdef __HTX_LINUX__
            if( rc == -2) {
				tot_thread_count --;
                sprintf(msg,"lcpu:%d(pcpu=%d) prefetch has been hot removed, thread will be terminating now tot_thread_count=%d\n",tid,pcpu,tot_thread_count);
                hxfmsg(&h_d, errno, HTX_HE_INFO, msg);
                pthread_exit(NULL);
            }
            else {
                sprintf(msg, "%d: Bindprocessor for prefetch irritator on lcpu:%d and corresponding pcpu:%d failed with rc=%d\n", __LINE__, tid,pcpu,rc);
                hxfmsg(&h_d, errno, HTX_HE_HARD_ERROR, msg);
            }

		#else
        sprintf(msg, "Binding to cpu:%d  failed with errno: %d \n",tid,errno);
        hxfmsg(&h_d, errno, HTX_HE_SOFT_ERROR, msg);

		#endif

        } /* End of if */
        else {
			/*sprintf(msg,"::physical cpu:%d for log cpu:%d\n",pcpu,tid);
            hxfmsg(&h_d, rc , HTX_HE_INFO, msg);*/
            #ifdef DEBUG
            sprintf(msg,"[%d] Bindprocessor success [prefetch thread_bo %d]! cpu_no : %d , pthread id : 0x%x \n",__LINE__,thread_no,tid,ptid);
            hxfmsg(&h_d, errno, HTX_HE_INFO, msg);
            #endif
        }

        th->seedval = time(NULL);
        srand48_r(th->seedval,&th->buffer);

       number_of_operations = current_rule->num_oper;

		starting_address = (unsigned long long)(th_array[thread_no].start_of_contiguous_memory);
		memory_fetch_size = current_rule->prefetch_memory_size - BYTES_EXC ;
		loop_count	= memory_fetch_size / cache_line_size;
       for (oper = 0; oper < number_of_operations ; oper++) {

        /* if SIGTERM was received, exit */
        if(exit_flag != 0) {
            break;
        }

        random_no = get_random_number_perf(thread_no);
        random_no = (unsigned long long)(random_no<<32) | (random_no);
		/*random_no = 0xaabbccdd;
		random_no = th_array[thread_no].random_pattern;*/
        th_array[thread_no].prev_seed = random_no;

		/* Now write DSCR if needed */
		if ( system_information.pvr >= POWER8_MURANO ) {
			prefetch_randomise_dscr(random_no, th->current_rule->pf_dscr , thread_no);
		}

        if (th_array[thread_no].prefetch_algorithm == RR_ALL_ENABLED_PREFETCH_ALGORITHMS) {
            /* Run all the enabled prefetch variants in round robin method */

            /* If prefetch nstride is set in the current prefetch configuration */
            if ( (PREFETCH_NSTRIDE & current_rule->pf_conf) == PREFETCH_NSTRIDE ) {
                n_stride(starting_address,memory_fetch_size,random_no,&th_array[thread_no].prefetch_scratch_mem[0]);
            }

        	/* if SIGTERM was received, exit */
            if (exit_flag != 0) {
                break;
            }

            /* If prefetch partial is set in the current prefetch configuration */
            if ( (PREFETCH_PARTIAL & current_rule->pf_conf) == PREFETCH_PARTIAL ) {
                partial_dcbt(starting_address,memory_fetch_size,random_no,&th_array[thread_no].prefetch_scratch_mem[0]);
            }

        	/* if SIGTERM was received, exit */
            if (exit_flag != 0) {
                break;
            }

            if ( (PREFETCH_IRRITATOR & current_rule->pf_conf) == PREFETCH_IRRITATOR ) {
				rc = do_prefetch( starting_address , memory_fetch_size , random_no, thread_no, loop_count, th_array[thread_no].pattern);
				if ( rc != 0 ) {
					sprintf(msg,"[%d] Miscompare in Prefetch!! Expected data = 0x%llx Actual data = 0x%llx thread_index :0x%x Start of memory = %llx, memory size = 0x%llx\n"
								,__LINE__,th_array[thread_no].pattern, *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)), thread_no, starting_address, memory_fetch_size);
					hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);
					dump_miscompare_data(thread_no, (unsigned char *)starting_address);
					return;
				}
                /*prefetch(starting_address,memory_fetch_size,random_no,&th_array[thread_no].prefetch_scratch_mem[0]);*/
            }

        	/* if SIGTERM was received, exit */
            if (exit_flag != 0) {
                break;
            }

            if( (PREFETCH_TRANSIENT & current_rule->pf_conf) == PREFETCH_TRANSIENT ) {
				/*lrand48_r(&th->buffer, &offset);*/
				offset		= random_no % (long)16;

				start_addr = (unsigned char *)starting_address + offset;
				rc = transient_dcbt((unsigned long long)start_addr, loop_count, th_array[thread_no].pattern );
				if ( rc != 0 ) {
					sprintf(msg,"[%d] Miscompare in Prefetch!! Expected data = 0x%llx Actual data = 0x%llx thread_index : 0x%x Start of memory = %llx, memory size = 0x%llx\n"
								,__LINE__,th_array[thread_no].pattern, *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)), thread_no, starting_address, memory_fetch_size);
					hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);
					dump_miscompare_data(thread_no, (unsigned char *)starting_address);
					return;
				}
            }

        	/* if SIGTERM was received, exit */
            if (exit_flag != 0) {
                break;
            }

			if ( (PREFETCH_NA & current_rule->pf_conf) == PREFETCH_NA ) {

				/*lrand48_r(&th->buffer, &offset);*/
				offset		= random_no % (long)16;
				start_addr = (unsigned char *)starting_address + offset;
				rc = prefetch_dcbtna((unsigned long long)start_addr, loop_count, th_array[thread_no].pattern,&temp_storage,&temp_pattern);
				if ( rc != 0 ) {
					sprintf(msg,"[%d] Miscompare in Prefetch!! Expected data = 0x%llx Actual data = 0x%llx copied data = %llx0x, copied pattern = %llx0x, thread_index : 0x%x Start of memory = %llx, memory size = 0x%llx\n"
								,__LINE__,th_array[thread_no].pattern, *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)), temp_storage, temp_pattern, thread_no, starting_address, memory_fetch_size);
					hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);
					dump_miscompare_data(thread_no, (unsigned char *)starting_address);
					return;
				}
			}
            if (exit_flag != 0) {
                break;
            }

        }
        else { /* Else Run only the specified algorithm */
            /*starting_address = (unsigned long long)(th_array[thread_no].start_of_contiguous_memory);
            memory_fetch_size = current_rule->prefetch_memory_size - BYTES_EXC ;*/

            if(th_array[thread_no].prefetch_algorithm == PREFETCH_NSTRIDE) {
				/*lrand48_r(&th->buffer, &random_no);*/
                n_stride(starting_address, memory_fetch_size, random_no, &th_array[thread_no].prefetch_scratch_mem[0]);
            }
            else if(th_array[thread_no].prefetch_algorithm == PREFETCH_PARTIAL) {
                partial_dcbt(starting_address, memory_fetch_size, random_no, &th_array[thread_no].prefetch_scratch_mem[0]);
            }
            else if(th_array[thread_no].prefetch_algorithm == PREFETCH_TRANSIENT) {
				/*lrand48_r(&th->buffer, &offset);*/
				offset		= random_no % (long)16;
				start_addr = (unsigned char *)starting_address + offset;

				rc = transient_dcbt((unsigned long long)start_addr, loop_count, th_array[thread_no].pattern );
				if ( rc != 0 ) {
					sprintf(msg,"[%d] Miscompare in Prefetch!! Expected data = 0x%llx Actual data = 0x%llx thread_index : 0x%x Start of memory = %llx, memory size = 0x%llx\n"
								,__LINE__,th_array[thread_no].pattern, *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)), thread_no, starting_address, memory_fetch_size);
					hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);
					dump_miscompare_data(thread_no, (unsigned char *)starting_address);
					return;
				}
            }
            else if(th_array[thread_no].prefetch_algorithm == PREFETCH_IRRITATOR) {
				rc = do_prefetch( starting_address , memory_fetch_size , random_no, thread_no, loop_count, th_array[thread_no].pattern);
				if ( rc != 0 ) {
					sprintf(msg,"[%d] Miscompare in Prefetch!! Expected data = 0x%llx Actual data = 0x%llx thread_index : 0x%x Start of memory = %llx, memory size = 0x%llx\n"
								,__LINE__,th_array[thread_no].pattern, *(unsigned long long *)((unsigned char *)starting_address + 128*(loop_count-rc)), thread_no, starting_address, memory_fetch_size);
					hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);
					dump_miscompare_data(thread_no, (unsigned char *)starting_address);
					return;
				}
            }
			else if ( th_array[thread_no].prefetch_algorithm == PREFETCH_NA ) {
				/*lrand48_r(&th->buffer, &offset);*/
				offset		= random_no % (long)16;
				start_addr = (unsigned char *)starting_address + offset;
				rc = prefetch_dcbtna((unsigned long long)start_addr, loop_count, th_array[thread_no].pattern,&temp_storage, &temp_pattern);
				if ( rc != 0 ) {
					sprintf(msg,"[%d] Miscompare in Prefetch ( returned %d)!! Expected data = 0x%llx Actual data = 0x%llx copied data = 0x%llx, copied pattern = 0x%llx, thread_index : 0x%x Start of memory = %llx, offset = %ld\n"
								,__LINE__, rc, th_array[thread_no].pattern, *(unsigned long long *)((unsigned char *)start_addr + 128*(loop_count-rc)), temp_storage, temp_pattern, thread_no, starting_address, offset);
					hxfmsg(&h_d, 0, HTX_HE_MISCOMPARE, msg);
					dump_miscompare_data(thread_no, (unsigned char *)starting_address);
					return;
				}
			}

        	/* if SIGTERM was received, exit */
            if(exit_flag != 0) {
                break;
            }
        }

    } /* End of for loop */
	#ifdef __HTX_LINUX__
        /* Restore original/default CPU affinity so that it binds to ANY available processor */

        rc = htx_unbind_thread();
	#else
        rc = bindprocessor(BINDTHREAD, thread_self(), PROCESSOR_CLASS_ANY);
	#endif
        if(rc == -1) {
                sprintf(msg, "%d: Unbinding from cpu:%d failed with errno %d \n",__LINE__, tid, errno);
                hxfmsg(&h_d, errno, HTX_HE_SOFT_ERROR, msg);
        }
	
#if defined(__HTX_MAMBO__) || defined(AWAN)
	printf("[%d] Thread no: %d, completed passes : %d\n",__LINE__, thread_no, oper);
#endif
}

/* This function is used to randomise the DSCR register.
 * The Inputs are a random number, and an integer parameter.
 * The integer parameters is to be interpreted as follows:
 * DSCR_RANDOM			= In this case the 64 bit random number is simply written to DSCR
 * DSCR_LSDISABLE		= In this case only bit 58 LSD is set. All other bits are randomised
 */

int prefetch_randomise_dscr(unsigned long long random_number, unsigned int what, unsigned int thread_no) {

/*
   Structure of DSCR
   +------------+-------+-------+-------+-------+-------+-------+-----------+-------+-------+-------+-------+-------+
   |    //      |  SWTE | HWTE  |  STE  |  LTE  | SWUE  | HWUE  |  UNIT CNT |  URG  |  LSD  | SNSE  |  SSE  | DPFD  |
   +------------+-------+-------+-------+-------+-------+-------+-----------+-------+-------+-------+-------+-------+
   0          3839      40      41      42      43      44      45        5455	  5758      59      60      61      63
 */

	unsigned long long 		local_random_number;
	unsigned long long	read_dscr;
	int rc = 0;

	mfspr_dscr(&read_dscr);
	th_array[thread_no].read_dscr_val = read_dscr;
	switch ( what ) {
		case DSCR_RANDOM:
			mtspr_dscr(random_number);
			th_array[thread_no].written_dscr_val = random_number;
			rc = 0;
			break;
		case DSCR_LSDISABLE:
			local_random_number = random_number | (0x1<<5);
			mtspr_dscr(random_number);
			th_array[thread_no].written_dscr_val = random_number;
#ifdef DEBUG
			printf("[%d] Original random number = 0x%x , After setting 58th bit = 0x%x\n",__LINE__,random_number,local_random_number);
#endif
			rc = 0;
			break;
		case DSCR_DEFAULT:
			break;
		default:
			sprintf(msg,"[%d] Wrong parameter = %d\n", __LINE__, what);
			hxfmsg(&h_d, -1, HTX_HE_HARD_ERROR, msg);
			rc = -1;
			break;
	}
	/*sleep (1);*/
	return rc;
}
unsigned long int get_random_number_perf(int index)
{
    unsigned long int val_8 ;

    lrand48_r(&th_array[index].buffer, &th_array[index].random_pattern);
    val_8 = (th_array[index].random_pattern <<16)| th_array[index].random_pattern ;

    return val_8 ;
}
int do_prefetch ( unsigned long long starting_address , unsigned long long memory_fetch_size , unsigned long long random_no, unsigned int thread_no, unsigned long long loop_count, unsigned long long pattern) {

	unsigned int 		direction_bitmask	= 0x40; /* 0b 0100 0000 - 57th bit */
	unsigned long long 	temp_mask = 0UL;;
	int					rc,i,stream_id;
	unsigned long long	start_addr = starting_address;

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
