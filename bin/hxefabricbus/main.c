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

#include "fabricbus.h"

/* HTX data */
struct htx_data htx_d;

struct sigaction sigvector;

#ifdef _DR_HTX_
   struct sigaction sigvector_dr;
   sigset_t mask;
#endif
	htxsyscfg_smt_t smt_detail;

/*
 * crash_on_mis_global will be controlled by htxkdblevel shell variable.
 * This will have precedence over crash_on_misc specified in rulefile.
 */
int crash_on_misc_global, errno;
char * htxkdblevel;

/*
 * For P8 we dont need 16M pages, as prefetching is now done on EA. 
 * For older Power releases, keep it enabled. 
 * 0 - disabled, use base page size.  
 * 1 - Enabled - Use large pages.   
 */ 
int SET_PAGESIZE = 0; 

/* Shared memory details created for each exer + 1(for random pattern ) */
MEMORY_SET mem[MAX_CPUS + 1];
struct thread_context th_array[MAX_CPUS];
unsigned int exit_flag = 0;

int Recvd_SIGUSR2 = 0;
int memory_set_cleanup();


int
main(int argc, char *argv[]) {

	char rf_name[100];
    FILE * rf_fp;
    char msg_buf[1024];
    int rc = 0, i = 0, mem_alloc, cpu, threads, node, chip, proc;
    unsigned int num_stanzas = 0, max_page_req = 0, mem_page_req = 0;
    SYS_CONF_TYP * scfg;                  /* system configuration */
    unsigned int pvr, smt_threads, tot_cpus, tc;
    unsigned long long * random_no_buffer = NULL;
    unsigned long long seedval, memory_set_size;
    unsigned int threads_created = 0;
    pthread_mutex_t threads_create_lock ;
    int th_rc;
	void *tresult = (void *)th_rc ; /* thread join status */
	unsigned long long num_pages_available = 0, fabbus_requested= 0;  

    /*
     * memory_mapping is populated in configure_memory_allocation function.
     * Defines the mapping of host_cpu vs dest_cpu.
     * Cpu Host_cpu has remote memory on Cpu dest_cpu.
     * memory_mapping[][HOST_CPU] ---> host cpu.
     * memory_mapping[][DEST_CPU] ---> dest cpu.
     * configurable through rule file parameter memory_configure.
     */
     unsigned int memory_mapping[MAX_CPUS][2];
   	 unsigned int num_cpus_mapped = 0;  /* keeps track of number of cpus mapped */
     unsigned int query_pages = 0, query_memconf = 0, query_sysconf = 0, query_maskconf = 0;

	/* 
	 * AND,OR Masks for each cpu defined by memory_mapping[][HOST_CPU] above.  
	 */ 
	MASK_STRUCT masks[MAX_CPUS];

     /*
      * Populated via rf_read_rules()
      */
     struct rf_stanza rf_info[MAX_TC];
     unsigned int num_stanza;
     struct rf_stanza * current_stanza;
#ifndef __HTX_LINUX__
	int early_lru = 1; /* 1 extends local affinity via paging space.*/
    int num_policies = VM_NUM_POLICIES;
    int policies[VM_NUM_POLICIES] = {/* -1 = don't change policy */
    									-1, /* VM_POLICY_TEXT */
    									-1, /* VM_POLICY_STACK */
    									-1, /* VM_POLICY_DATA */
    									P_FIRST_TOUCH, /* VM_POLICY_SHM_NAMED */
    									P_FIRST_TOUCH, /* VM_POLICY_SHM_ANON */
    									-1, /* VM_POLICY_MAPPED_FILE */
    									-1, /* VM_POLICY_UNMAPPED_FILE */
    								};	

#endif 
     /* copy command line arguments passed */
     if(argv[0]) strcpy(htx_d.HE_name, argv[0]);
     if(argv[1]) strcpy(htx_d.sdev_id, argv[1]);
     if(argv[2]) strcpy(htx_d.run_type, argv[2]);
     strcpy(rf_name, argv[3]);

     if(argv[4]) {
		if((strcasecmp(argv[4], "query_pages")) == 0) {
        	query_pages = 1;
        } else
        if((strcasecmp(argv[4], "query_memconf")) ==0) {
            query_memconf = 1;
        } else
        if ((strcasecmp(argv[4], "query_sysconf")) == 0) {
            query_sysconf = 1;
		} else 
		if ((strcasecmp(argv[4], "query_maskconf")) == 0) { 
			query_maskconf = 1;
        } else {
            sprintf(msg_buf, " hxefabricbus invoked with wrong parameter= %s \n",argv[4]);
            hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg_buf);
            return (-1);
        }
    }
    /*  Register SIGTERM handler */
    sigemptyset((&sigvector.sa_mask));
    sigvector.sa_flags = 0;
    sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
    sigaction(SIGTERM, &sigvector, (struct sigaction *) NULL);

	/*  Register SIGUSR2 handler function */
	sigvector.sa_handler = (void (*)(int)) SIGUSR2_hdl;
	sigaction(SIGUSR2, &sigvector, (struct sigaction *) NULL);

    /* Set the program behaviour in case of miscompare */
    htxkdblevel = getenv("HTXKDBLEVEL");
    if (htxkdblevel) {
    	crash_on_misc_global = atoi(htxkdblevel);
    } else {
        crash_on_misc_global = 0;
    }
    rc = atexit(memory_set_cleanup);
    if(rc != 0) {
		sprintf(msg_buf, "[%d] Error: Could not register for atexit!\n",__LINE__);
		hxfmsg(&htx_d, -1, HTX_HE_SOFT_ERROR, msg_buf);
        return (rc);
    }

    /* Calculate PVR value */
#ifdef __HTX_LINUX__
	__asm __volatile ("mfspr %0, 287" : "=r" (pvr));
#else
    pvr = (unsigned int) getPvr();
#endif
    pvr = pvr >> 16;


    if(pvr < PVR_POWER6 ) {
       	sprintf(msg_buf, " Fabricbus exer runs on Power6 and above boxes. pvr =0x %x\n", pvr);
       	hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg_buf);
    	return (-1);
    }
	/* Configure which page size we are going to use */ 
	if(pvr >= PVR_POWER8_MURANO){/* Use 4 K */
		SET_PAGESIZE= 0;	 /* We dont need to configure 16M pages, as its default */  
	} else { /* Use 16M pages */ 
		SET_PAGESIZE = 1; 
	}

    hxfupdate(START, &htx_d);
    if((!(strlen(htx_d.htx_log_dir))) || (!(strlen(htx_d.htx_exer_log_dir)))){
        sprintf(msg_buf,"htx lib log direcories found to be htx_log_dir= %s and htx_exer_log_dir= %s, thus updating them to '/tmp/'\n",htx_d.htx_log_dir,htx_d.htx_exer_log_dir);
        hxfmsg(&htx_d,0, HTX_HE_INFO, msg_buf);
        strcpy(htx_d.htx_log_dir,"/tmp/");
        strcpy(htx_d.htx_exer_log_dir,"/tmp/");

    }
#ifndef __HTX_LINUX__
    /* Machine should be dedicated LPAR  */
    if(_system_configuration.splpar_status & 0x2) {
        sprintf(msg_buf, "\n This test can not run on shared processors lpar config. Please configure the lpar with dedicated cpus and rerun the test !\n");
        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
        exit(15);
    }

    /* Calculate SMT thread value */
    if(_system_configuration.smt_status & SMT_ENABLED)
        smt_threads = _system_configuration.smt_threads;

	/* Set VM Polocy to get local memory */
	rc = vm_mem_policy(VM_SET_POLICY,&early_lru, &policies ,num_policies);
	if(rc) { 
		sprintf(msg_buf, "\n vm_mem_policy failed with rc = %d, errno = %d \n", rc, errno); 	
		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
        exit(16); 
	}

#else
{
	int rc, count=0;
    	if( (argv[4]) || (strncmp(htx_d.run_type, "OTH", 3) == 0) ) {
        	if( init_syscfg_with_malloc() == -1) {
			sprintf(msg_buf, "init_syscfg_with_malloc returned with -1");
			hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg_buf);
			return (-1);
		} 
 	 } else {

		rc = repopulate_syscfg(&htx_d);
		if( rc != 0) {
		while(count < 10){
				sleep(1);
		    rc = repopulate_syscfg(&htx_d);
		    if(!rc){
			break;
		    }
		    else {
			count++;
		    }
		}
		if(count == 10){
					sprintf(msg_buf,"[%d] Repopulation of syscfg unsuccessful. Exiting\n",__LINE__);
					hxfmsg(&htx_d, rc, HTX_HE_SOFT_ERROR, msg_buf);
			}
		}
	}
	get_smt_details(&smt_detail);
	smt_threads = smt_detail.smt_threads;
}
#endif
    if(smt_threads == 0) {
		sprintf(msg_buf, "\n hxefabricbus could not detect SMT status smt_threads = %d Assuming smt_threads = 1\n",smt_threads);
		hxfmsg(&htx_d, rc, HTX_HE_INFO, msg_buf);
        smt_threads == 1 ;
    }


#ifdef _DR_HTX_
    #if 0
        sigprocmask(SIG_UNBLOCK, NULL, &mask);
        mask.hisigs &= 0x02000000;
        sigprocmask (SIG_UNBLOCK, &mask , NULL);
    #endif
        sigemptyset(&(sigvector_dr.sa_mask));
        sigvector_dr.sa_flags = 0;
        sigvector_dr.sa_handler = (void (*)(int))DR_handler;
        rc = sigaction(SIGRECONFIG, &sigvector_dr, (struct sigaction *) NULL);
        if (rc != 0) {
           printf(msg_buf,"sigaction failed(%d) for SIGRECONFIG\n", errno );
           hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
        }
#endif

    /* read rule file */
    rc = rf_read_rules(rf_name, rf_info, &num_stanzas, pvr);
    if (rc) {
        sprintf(msg_buf, " Rule file parsing  failed! rc = %d", rc);
        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
        return (rc);
    }

#ifndef __HTX_LINUX__
   	tot_cpus = _system_configuration.ncpus;
#else
	tot_cpus = get_nprocs();
#endif
	if(tot_cpus == 0 ) {
		sprintf(msg_buf, "\n hxefabricbus could not detect tot cpus on system tot_cpus=%d, Assuming 1 \n",tot_cpus);
		hxfmsg(&htx_d, rc, HTX_HE_INFO, msg_buf);
		tot_cpus = 1 ;
	}

    /* Automatically detect System Configuration */
	scfg = (SYS_CONF_TYP *)malloc(sizeof(SYS_CONF_TYP));
	if(scfg == NULL) {
		sprintf(msg_buf, "\n malloc failed rc = %d \n", scfg);
		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
        exit(1);
    }

	memset(scfg, 0, sizeof(SYS_CONF_TYP));
	rc =read_hardware_config(scfg, tot_cpus, pvr);
    if(rc) {
    	sprintf(msg_buf, "\n %s : Fabricbus failed with rc - %d. Will Exit !", __FUNCTION__, rc);
        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
        exit(1);
    }

    /* Intialize the htxmp library datastructures */
    rc = mp_initialize(tot_cpus, &htx_d);
    if(rc == -1) {
       sprintf(msg_buf, "\n %s : Fabricbus failed to intialize htxmp datastructures, errno = %d \n",errno);
       hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
       exit(1);
    }

    do { /* LOOP infinitely if exer started with REG or EMC cmd line argument */
		if(Recvd_SIGUSR2) {
			/* We exited prev test coz of SIGUSR2, User would have modified rules file.
			 * Read Rules again
			 */
			sprintf(msg_buf, "Reading Rules file again coz of SIGUSR2 \n");
			hxfmsg(&htx_d, rc, HTX_HE_INFO, msg_buf);
			memset(rf_info, 0, sizeof(struct rf_stanza) * MAX_TC);
			num_stanzas = 0;
			rc = rf_read_rules(rf_name, rf_info, &num_stanzas, pvr);
    		if (rc) {
        		sprintf(msg_buf, " Rule file parsing  failed! rc = %d", rc);
        		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
        		return (rc);
    		}
			Recvd_SIGUSR2 = 0;
		}
        for(tc = 0; tc < num_stanzas; tc++) {   /* Loop for each stanza in rules file */
    	    current_stanza = &rf_info[tc] ;


            if((current_stanza->cec_nodes != 0 ) && (current_stanza->chips_per_node !=0) && (current_stanza->cores_per_chip !=0)) {
            	/*
                 * If user provided non zero cec_nodes, chips_per_node and cores_per_chip
                 * override the previously detected system configuration.
                 */
                 int j, k = 0, procs = 0;
                 /* Systems usually dont have symetric distribution. So in these cases use these rules parm to
                  * depict nearest possible system configuration.
                  */
                 if((current_stanza->cec_nodes * current_stanza->chips_per_node * current_stanza->cores_per_chip * smt_threads) > tot_cpus) {
                 	sprintf(msg_buf, "Wrong user defined parameters: cec_nodes: %d, chips_per_node: %d, cores_per_chip: %d. Max allowed = %d . Exiting !",
                                    current_stanza->cec_nodes, current_stanza->chips_per_node, current_stanza->cores_per_chip, tot_cpus);
                    hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg_buf);
                    exit(1);
                 }
                 /* Override the previously detected System Configuration */
                 memset(scfg, 0 , sizeof(SYS_CONF_TYP));
                 scfg->num_nodes = current_stanza->cec_nodes ;
                 for(i = 0; i < current_stanza->cec_nodes; i ++) {
                 	scfg->node[i].num_chips = current_stanza->chips_per_node ;
                    for(j = 0; j < current_stanza->chips_per_node; j++) {
                   	 	scfg->node[i].chip[j].num_procs = current_stanza->cores_per_chip * smt_threads ;
						for(k = 0; k < scfg->node[i].chip[j].num_procs; k ++) {
                        	scfg->node[i].chip[j].lprocs[k] = procs;
                        	procs++;
						}
                    }
                }
            }
			printf("******************  HXEFABRICBUS DETECTED HARDWARE CONFIGURATION  *********************\n");
 			printf( "Num_nodes = %d \n",scfg->num_nodes);
    		for(node=0; node < scfg->num_nodes; node++) {
        		printf("Node = %d ,",node);
        		printf("num_chips = %d \n", scfg->node[node].num_chips);
        		for(chip = 0; chip < scfg->node[node].num_chips; chip++) {
            		printf("Chip = %d, num_procs=%d \n", chip, scfg->node[node].chip[chip].num_procs);
            		printf("Processor : ");
            		for(proc = 0; proc < scfg->node[node].chip[chip].num_procs; proc ++) {
                		printf("%u, ", scfg->node[node].chip[chip].lprocs[proc]);
            		}
            		printf(" \n");
        		}
        		printf(" \n");
    		}
   		 	printf( "****************************************************************************************\n");

			if(query_sysconf)
				continue;  /* If user specify diff sys configuration thru multiple stanzas in rule, then display sys config for each of them */

            /* If rules file parm memory_configure = 0 then use predefined algos to decide memory mapping,
             * else use had specified memory mapping thru file read from there
             */
            num_cpus_mapped = 0;
            for(i = 0; i < MAX_CPUS; i++) {
                memory_mapping[i][HOST_CPU] = NO_CPU_DEFINED;
                memory_mapping[i][DEST_CPU] = NO_CPU_DEFINED;
            }
			mem_alloc = current_stanza->memory_allocation;
            if(current_stanza->memory_configure == 0 || query_memconf ) {
                /* IF this standard exerciser run, then we need to verify requried number of pages 
				 * allocated meets our requirement */
                FILE * fp ;
				unsigned long long psize; 
				long long int pages_available = 0; 
                char fname[256];

                if(query_memconf == 0 && query_pages == 0 ) {
                    if(SET_PAGESIZE) { /* P7P and below uses 16M pages */
                        psize = 16 * M;
                    } else { /* P8 and above */
                        psize = 4 * K;
                    }

               	#ifndef __HTX_LINUX__
                    struct vminfo_psize info ; 
					info.psize = psize; 
                    rc = vmgetinfo(&info, VMINFO_PSIZE, sizeof(struct vminfo_psize));
                    if(rc == -1) {
                        sprintf(msg_buf,"%s: vmgetinfo call for VMINFO_PSIZE cmd failed with errno %d for page size =0x%llx\n", __FUNCTION__, errno, info.psize);
                        hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
                        return(-1);
                    }
                    num_pages_available = (unsigned long long)(info.numfrb * 0.90);
				#else
					if(psize == 4* K) { 
						pages_available = GetMemoryDetails("MemAvailable", &htx_d); 
						if(pages_available == 0 || pages_available == -1) { 
							pages_available = GetMemoryDetails("MemFree", &htx_d);
						}
					} else { 
						pages_available = GetMemoryDetails("HugePages_Free", &htx_d); 
					}
		
					if(pages_available == -1) {
						sprintf(msg_buf,"%s: GetMemoryDetails call from /proc/meminfo  failed for page size =0x%llx\n", __FUNCTION__, psize); 
						hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
                        return(-1);
                    }
					pages_available = (pages_available / psize); 		
	
					num_pages_available = (unsigned long long)(pages_available * 0.90); 	
				#endif 
                    /* Next lets find out what was our requirement */
					strcpy(fname,htx_d.htx_log_dir);
                    sprintf(fname, "%s/fabricbus_mem_req_%d",fname, mem_alloc);
                    if((fp=fopen(fname,"r")) == NULL){
                        sprintf(msg_buf, "fopen of file %s failed with errno=%d",fname,errno);
                        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
                        return(-1);
                    }

                    printf("Reading from file %s\n", fname);
						 
                    rc = fscanf(fp,"%lld\n",&fabbus_requested);
                    if (rc == 0 || rc == EOF) {
                        sprintf(msg_buf, "fscanf of num_nodes on file %s failed with errno =%d",fname,errno);
                        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
                        return(-1);
                    }
					/* For P8 we write 0 to "/tmp/fabricbus_mem_req_<mem_alloc>" */
               		if(pvr >= PVR_POWER8_MURANO){ 
						/* This logic won't work for mem_alloc = 0, 1,2, we only use them for debugging 
						 * As long as mem_alloc = 3,4 we try to map all CPUs,  we are good with below equation 
						 */  
						fabbus_requested = (((unsigned long long)memory_per_thread(pvr) * (unsigned long long)tot_cpus)/psize) ; 
					} 	
					printf("fabbus_requested=%#llx, num_pages_available=%#llx, psize=%#llx, memory_per_thread = %#llx, tot_cpus = %#llx\n",
						fabbus_requested, num_pages_available, psize, memory_per_thread(pvr), tot_cpus); 
                    fclose(fp);
					if(fabbus_requested > num_pages_available) { 
						/* Put this message, so that we know we are running on skewed memory config system */ 
                    	sprintf(msg_buf, " System is falling short of memory,  num_pages_available = %#llx, exerciser demands =%#llx, memory_per_thread=%#llx, pagesize=%#llx \n", 
															num_pages_available, fabbus_requested, memory_per_thread(pvr), psize );
                    	hxfmsg(&htx_d, 0, HTX_HE_INFO, msg_buf);
					}
                }

            	/*
                 * Predefined Algo's used for Memory mapping,
                 * Done on the basis of rules parameter : memory_allocation.
                 */
                 rc = configure_memory_allocation(scfg, mem_alloc, memory_mapping, &num_cpus_mapped, current_stanza->threads_per_node, num_pages_available, 
																fabbus_requested, pvr, query_memconf, query_pages);
                 if(rc) {
                 	sprintf(msg_buf," configure_memory_allocation failed with rc = %d \n", rc);
                    hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
					exit(-1);
                 }
                 if(query_memconf ) {
                 	/*
                     * fabbus started with query_memconf command while creating mdt's, print the memory
                     * mapping to /tmp/fabricbus_mem_config_*(* = mem_alloc).
                     */
                   	FILE * memfptr ;
                    char file_mem[256];
					strcpy(file_mem,htx_d.htx_log_dir);
                    sprintf(file_mem, "%s/fabricbus_mem_config_%d",file_mem, mem_alloc);
                    memfptr = fopen(file_mem,"w");
                    if(memfptr == NULL)  {
                    	printf("Error opening file - %s",file_mem);
                        exit(1);
                    }
                    for(i = 0; i < num_cpus_mapped; i++) {
                        sprintf(msg_buf,"[%d,%d] \n", memory_mapping[i][HOST_CPU], memory_mapping[i][DEST_CPU]);
                        fprintf(memfptr,"%s \n ",msg_buf);
                    }
                   	fclose(memfptr);
                   	return(0);
                }
            } else {
                /* If rules parameter memory_configure = 1,
                 * read user defined memory mapping from file
                 */
                rc = user_def_memory_mapping(mem_alloc, &num_cpus_mapped, memory_mapping);
                if(rc == -1 ) {
                    sprintf(msg_buf,"user_def_memory_mapping() fun failed, Unable to read from /tmp/fabricbus_mem_config  file errno =%d \n",errno);
                    hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
					return(-1);
                }
            }
			/* Intialize the mask structure */ 
			for(i = 0; i < num_cpus_mapped; i++) { 
				masks[i].host_cpu = memory_mapping[i][HOST_CPU]; 
				masks[i].dest_cpu = memory_mapping[i][DEST_CPU];
				masks[i].and_mask_len = current_stanza->mask_strct.and_mask_len; 
				memcpy(&masks[i].and_mask[0], &current_stanza->mask_strct.and_mask[0], (sizeof(unsigned long long) * masks[i].and_mask_len)); 
				masks[i].or_mask_len = current_stanza->mask_strct.or_mask_len; 
				memcpy(&masks[i].or_mask[0], &current_stanza->mask_strct.or_mask[0], (sizeof(unsigned long long) * masks[i].or_mask_len)); 
			}
			 	 
			if(query_maskconf) { 
				/* This command will generate a sample mask file for user */ 
				char file_masks[256];
				int j = 0, host_rad, dest_rad[MAX_NODES],  cnt = 0, found = 0 ;  
				FILE * mfptr ;  
				strcpy(file_masks,htx_d.htx_log_dir);
				sprintf(file_masks, "%s/fabricbus_masks_%d",file_masks, mem_alloc); 
	
				mfptr = fopen(file_masks, "w"); 
				if(mfptr == NULL ) {
                    sprintf(msg_buf,"\n Error opening file - %s", file_masks);
                    hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
                    exit(1);
                }
				if(mem_alloc == 3) { 
					sprintf(msg_buf, " [ HOST_NODE, DEST_NODE, AND_MASK, OR_MASK ] \n");
				} else if(mem_alloc == 4) { 
					sprintf(msg_buf, " [ HOST_CHIP, DEST_CHIP, AND_MASK, OR_MASK ] \n");
				}   
				fprintf(mfptr,"%s",msg_buf);
				
				for(i = 0; i < num_cpus_mapped; i++) { 
					if(masks[i].dest_cpu == NO_CPU_DEFINED || masks[i].host_cpu == NO_CPU_DEFINED) 
						continue; 	
					if(i == 0) { 
						cnt = 0; 
						host_rad = get_physical_number(masks[i].host_cpu, mem_alloc, pvr); 
						dest_rad[cnt] = get_physical_number(masks[i].dest_cpu, mem_alloc, pvr); 
						cnt++;
					} else if(host_rad  == get_physical_number(masks[i].host_cpu, mem_alloc, pvr)) { 
						for(j = 0; j < cnt; j++) { 
							DEBUGON(" i=%d, host_rad=%d, cnt=%d, found=%d, dest_rad=%d \n", 
									i, host_rad, cnt, found, get_physical_number(masks[i].dest_cpu, mem_alloc, pvr)); 
							if(dest_rad[j] == get_physical_number(masks[i].dest_cpu, mem_alloc, pvr)) { 
								/* We have already dumped this entry */ 
								found = 1; 
								break;
							} 
						}	 
						if(found == 0) { 
							dest_rad[cnt] = get_physical_number(masks[i].dest_cpu, mem_alloc, pvr);   
							cnt++ ; 
						} else {  
							found = 0; 
							continue;
						} 	
					} else {  
						cnt = 0; 
						host_rad = get_physical_number(masks[i].host_cpu, mem_alloc, pvr); 
						dest_rad[cnt] = get_physical_number(masks[i].dest_cpu, mem_alloc, pvr); 
						cnt++;
					}
					sprintf(msg_buf, "[ %d, %d, 0x", get_physical_number(masks[i].host_cpu, mem_alloc, pvr), get_physical_number(masks[i].dest_cpu, mem_alloc, pvr));
					fprintf(mfptr,"%s",msg_buf);
					for(j = 0; j < masks[i].and_mask_len; j++) { 
						sprintf(msg_buf,"%llx",masks[i].and_mask[j]); 
						fprintf(mfptr,"%s",msg_buf);
					}
					fprintf(mfptr,", 0x");
					for(j = 0; j < masks[i].or_mask_len; j++) { 
						sprintf(msg_buf,"%llx",masks[i].or_mask[j]); 
						fprintf(mfptr,"%s",msg_buf); 
					} 
					fprintf(mfptr," ]\n ");
				}
				fclose(mfptr);
				return(0);		
			} 					
           	if(current_stanza->mask_configure) {
                /* User has Specified some masks, read it from file
                 */
                rc = user_def_mask_mapping(mem_alloc, num_cpus_mapped, pvr, masks);
                if(rc == -1) {
                    sprintf(msg_buf,"user_def_mask_mapping() fun failed with rc =%d,Unable to read from /tmp/fabricbus_masks , errno = %d \n",rc, errno);
                    hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
                    return(-1);
                }

            }

            for(i = 0; i < num_cpus_mapped; i++) {
				int j ; 
				DEBUGON( "host_cpu = %d, dest_cpu = %d, and_mask = 0x", masks[i].host_cpu, masks[i].dest_cpu); 		 
				for(j = 0; j < masks[i].and_mask_len; j++) {    
					DEBUGON("%llx", masks[i].and_mask[j]); 
				} 
				DEBUGON(", or_mask = 0x");
				for(j = 0; j < masks[i].or_mask_len; j++)  {  
					DEBUGON("%llx", masks[i].or_mask[j]);
				} 
				DEBUGON("\n");
			}

            if(query_pages) {
                unsigned int page_req = 0;
                for(i = 0; i < num_cpus_mapped; i++) {
                    if(query_pages && memory_mapping[i][DEST_CPU] != NO_CPU_DEFINED) {
                        page_req++;
                    }
                }
				if(page_req > max_page_req)
                   	max_page_req = page_req ;

            	FILE * qfptr;
            	char file_pages[256];
				unsigned int pages_configured = 0;
				/* 
				 * First magic number in calculation below is calculated as : 
				 * memory_per_chip = L3CACHE_SIZE * NUM_CORES_PER_CHIP (due to LCO) 
				 * Now to spill-over L3 we need 2 * memory_per_chip. 
				 * Total memory requirement per thread = 2 * ( L3CACHE_SIZE * NUM_CORES_PER_CHIP) MB 
				 * Number of 16M Page required per thread = ((2 * ( L3CACHE_SIZE * NUM_CORES_PER_CHIP) MB) / 16MB) 
				 * Magic Number = Number of 16M Page Requirement per thread :-) 
				 */ 
				if(pvr == PVR_POWER6 || pvr == PVR_POWER7) { 
            		mem_page_req =(int)( 4 * max_page_req + 0.10 * 4 * max_page_req + 1); /* 10% extra for each thread */
				} else if(pvr == PVR_POWER7PLUS) { 
					mem_page_req = (int)( 10 * max_page_req + 0.10 * 10 * max_page_req + 1); /* 10% extra for each thread */
				} else if(pvr >= PVR_POWER8_MURANO){
					mem_page_req = 0;	/* Don't need 16M pages */ 
				} else { 
					mem_page_req = 4 * max_page_req; 
				} 	
				/* Need to account for pages, if user specifies random buffer size through rules file */ 
				if(current_stanza->randbuf_size > 0 ) { 
					if(SET_PAGESIZE) {
						mem_page_req += current_stanza->randbuf_size ; 
					} else {  
						mem_page_req += 0; 
					}
				} 
				strcpy(file_pages,htx_d.htx_log_dir);
            	sprintf(file_pages, "%s/fabricbus_mem_req_%d",file_pages, mem_alloc);
            	qfptr = fopen(file_pages, "w");
            	if(qfptr == NULL ) {
                	sprintf(msg_buf,"\n Error opening file - %s", file_pages);
                	hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
                	exit(1);
            	}
				/* Read from file, how many pages already configured for this memory_allocation */
				fscanf(qfptr, "%d", &pages_configured);
				DEBUGON("pages_configured = %d, required = %d \n", pages_configured, mem_page_req);
				if(pages_configured != 0 && pages_configured > mem_page_req)
					fprintf(qfptr,"%d \n ",pages_configured);
				else
            		fprintf(qfptr,"%d \n ",mem_page_req);
				errno = 0;
            	fclose(qfptr);
            	continue;
        	}
		

	        if ( current_stanza->seed == 0 ) { /* If read from rules file */
                seedval = time(NULL);
            } else {
                seedval = current_stanza->seed;
            }
            srand48(seedval);

			if(num_cpus_mapped == 0) {
				sprintf(msg_buf, "No fabricbus detected on this system. This program would exit !!!! num_cpus_mapped=%d \n", num_cpus_mapped);
				hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
                return(-1);
            }


            /* Fabricbus bus requires 2 * L3CACHE_SIZE */
			memory_set_size = memory_per_thread(pvr); 
            
            for(i = 0; i < MAX_CPUS + 1; i++) {
                mem[i].seg_addr = NULL;
                mem[i].shm_id = -1;
                mem[i].memory_set_size = memory_set_size;
            }
			/* Fabricbus requires 16M * 4 pages for each logical proc, configure the req 16M pages */
            if( (rc = get_cont_phy_mem(num_cpus_mapped, memory_mapping, memory_set_size, mem)) == -1 ) {
            	sprintf(msg_buf,"\n Could not get required memory. This program needs (2*L3 cache size) memory per logical processor to run. Exiting....\n");
               	hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
                exit(1);
            }

            for(i = 0; i < current_stanza->fixed_pattern.num_patterns; i++) {
                if(current_stanza->fixed_pattern.pattern[i].pattern_type == PATTERN_RANDOM) {
           			/* Fill a pool of memory_set_size buffer with random numbers,
                 	 * each thread thereafter would offset into the memory location on the basis of its thread no
                 	 */
					int random_buffer_size; 
					random_buffer_size = memory_set_size; 
					if(current_stanza->randbuf_size > 0)  { 
						/* Number of pages to be allocated for random buffer */  
						random_buffer_size = current_stanza->randbuf_size * 16 * M; 
					}  
                    rc = fill_random_buffer(random_buffer_size, &random_no_buffer, &mem[MAX_CPUS]); 
                    if(rc) {
                        sprintf(msg_buf, "\n Could not populate random buffer rc =%d \n",rc);
                        hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg_buf);
                        exit(1);
                    }
                	break; /* Once per stanza */
                }
            } /* Off PATTERN_RANDOM if condition */



            memset(th_array, -1, MAX_CPUS * sizeof(struct thread_context));
            for(i = 0; i < MAX_CPUS; i ++) {
                th_array[i].tid = -1;
            }

            htx_d.test_id = tc + 1;
            hxfupdate(UPDATE, &htx_d);

            /* create threads to start operations */
            for(cpu = 0; cpu < num_cpus_mapped ; cpu++) {
                if(memory_mapping[cpu][DEST_CPU] != NO_CPU_DEFINED  ) {
                    threads = memory_mapping[cpu][HOST_CPU];
                } else {
                    continue ;
                }
				if(Recvd_SIGUSR2) {
					/* If we recvd a SIGUSR before creating threads, Break of this loop */
					break;
				}

                pthread_attr_init(&th_array[threads].thread_attrs);
                pthread_attr_setdetachstate(&th_array[threads].thread_attrs, PTHREAD_CREATE_JOINABLE);
                pthread_attr_setscope(&th_array[threads].thread_attrs, PTHREAD_SCOPE_PROCESS);
				pthread_mutex_init(&threads_create_lock, NULL);

                th_array[threads].bind_to_cpu = threads;
                th_array[threads].tc_id = tc;
                th_array[threads].rand_buffer = (unsigned long long *)random_no_buffer;
                th_array[threads].num_oper = current_stanza->num_oper;
                th_array[threads].seg_addr = mem[threads].seg_addr;
                th_array[threads].memory_set_size = mem[threads].memory_set_size;
				th_array[threads].randbuf_size = current_stanza->randbuf_size * 16 * M; 
                th_array[threads].compare = current_stanza->compare;
                th_array[threads].crash_on_misc = current_stanza->crash_on_misc;
                th_array[threads].wrc_iterations = current_stanza->wrc_iterations;
                th_array[threads].add_iterations = current_stanza->add_iterations;
                th_array[threads].wrc_data_pattern_iterations = current_stanza->wrc_data_pattern_iterations;
                th_array[threads].wrc_iterations = current_stanza->wrc_iterations;
                th_array[threads].copy_iterations = current_stanza->copy_iterations;
                th_array[threads].add_iterations = current_stanza->add_iterations;
                th_array[threads].daxpy_iterations = current_stanza->daxpy_iterations;
                th_array[threads].daxpy_scalar = current_stanza->daxpy_scalar;
                th_array[threads].fixed_pattern.num_patterns = current_stanza->fixed_pattern.num_patterns;
                memcpy(th_array[threads].fixed_pattern.pattern,current_stanza->fixed_pattern.pattern, (sizeof(PATTERN) * th_array[threads].fixed_pattern.num_patterns));
				memset(&th_array[threads].th_htx_d, 0, sizeof(struct htx_data ));
				strcpy(th_array[threads].th_htx_d.HE_name, htx_d.HE_name);
				strcpy(th_array[threads].th_htx_d.sdev_id, htx_d.sdev_id);
				strcpy(th_array[threads].th_htx_d.run_type, htx_d.run_type);
				th_array[threads].th_htx_d.test_id =  htx_d.test_id;			
				memcpy(&th_array[threads].and_mask[0], &masks[cpu].and_mask[0], (sizeof(unsigned long long) * masks[cpu].and_mask_len));  
				th_array[threads].and_mask_len = masks[cpu].and_mask_len;  
				memcpy(&th_array[threads].or_mask[0], &masks[cpu].or_mask[0], (sizeof(unsigned long long) * masks[cpu].or_mask_len)); 
				th_array[threads].or_mask_len = masks[cpu].or_mask_len; 

                if(pthread_create(&th_array[threads].tid, &th_array[threads].thread_attrs,
                                                (void *(*)(void *))run_test_stages,
                                                (void *)&th_array[threads]) ) {
           			sprintf(msg_buf, "\n pthread_create failed with %d", errno);
                    hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg_buf);
                    exit_flag = 1;
                    break;
               } else {
                    if(!pthread_mutex_lock(&threads_create_lock)) {
                        threads_created++;
                        DEBUGON("\ntotal threads =%d  thread created\n",threads_created);
                    } else {
                        sprintf(msg_buf, "pthread_mutex_lock failed with %d", errno);
                        hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg_buf);
                        exit_flag = 1;
                    }
                    if(pthread_mutex_unlock( &threads_create_lock)) {
                        sprintf(msg_buf, "pthread_mutex_unlock failed with %d",errno);
        				hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg_buf);
                        exit_flag = 1;
                        break;
                    }
                }
            } /* Off num_cpus_mapped loop */


            for(i=0; i < MAX_CPUS ; i++) {

                if ( th_array[i].tid != -1){


                    rc = pthread_join(th_array[i].tid,&tresult);
                    if ( rc != 0) {
                        sprintf(msg_buf,"pthread_join failed ! errno %d :(%d): tnum=%d\n",errno,rc,th_array[i].tid);
                        hxfmsg(&htx_d, HTX_HE_HARD_ERROR , rc , msg_buf);
                        exit_flag =1;
                        break ;
           		    } else {
                        th_array[i].tid = -1 ;
                        DEBUGON("\n Thread %d Just Joined\n" ,i);
                    }
             	}
            }
            /* Free memory */
            memory_set_cleanup();

        } /* Off num_stanza loop */
      	if(query_pages || query_sysconf) {
			/* We are here, we have scanned thru all stanzas and calculated memory_allocation
			 * for eahc memory_allocation defined in rules file
			 */
			exit(0);
		}
       	/*
       	 * hxfupdate call with FINISH arg indicates one more rule file pass done
       	 */
       	hxfupdate(FINISH, &htx_d);
       	if(exit_flag)
           	break;
    } while( (rc = strcmp(htx_d.run_type, "REG") == 0) || (rc = strcmp(htx_d.run_type, "EMC") == 0) || (rc = strcmp(htx_d.run_type, "OTH") == 0));

	free(scfg); 
    memory_set_cleanup();
    pthread_mutex_destroy(&threads_create_lock);
    return 0;
}

void
SIGTERM_hdl (int sig) {
    int i,rc;
    hxfmsg(&htx_d, 0, HTX_HE_INFO, "sigterm received");
    exit_flag = 1 ;
    char msg_buf[100];

    for(i=0; i < MAX_CPUS ; i++) {
        if ( th_array[i].tid != -1){
            DEBUGON("Cancellin thread tid - %d \n",th_array[i].tid );
            rc = pthread_cancel(th_array[i].tid);
            if ( rc != 0 && rc != ESRCH ) {
                 sprintf(msg_buf,"pthread_cancel failed ! errno : %d, tnum: %d rc: %d", errno, th_array[i].tid, rc);
                 hxfmsg(&htx_d, HTX_HE_HARD_ERROR , rc , msg_buf);
            }
        }
    }
    memory_set_cleanup();
    exit(0);
}

void
SIGUSR2_hdl(int sig) {
	int i, rc = 0;
	char msg_buf[100];

	if(Recvd_SIGUSR2 == 0) {
		sprintf(msg_buf,"Recvd SIGUSR2 ..Will Restart test with new set of rules parm \n");
		hxfmsg(&htx_d, 0,HTX_HE_INFO, msg_buf);
		/* Set this flag, so that main process takes appropriates steps */
		Recvd_SIGUSR2 = 1;
		for(i=0; i < MAX_CPUS ; i++) {
    	    if ( th_array[i].tid != -1){
            DEBUGON("Cancellin thread tid - %d \n",th_array[i].tid );
        	    rc = pthread_cancel(th_array[i].tid);
            	if ( rc != 0 && rc != ESRCH ) {
                 	sprintf(msg_buf,"pthread_cancel failed ! errno : %d, tnum: %d rc: %d", errno, th_array[i].tid, rc);
                 	hxfmsg(&htx_d, HTX_HE_HARD_ERROR , rc , msg_buf);
            	}
        	}
    	}
    	memory_set_cleanup();

	} else {
		sprintf(msg_buf,"Prev Instance of SIGUSR2 not yet processed, This Signal would be ignored. \n");
		hxfmsg(&htx_d, HTX_HE_INFO, 0, msg_buf);
	}
}

void
run_test_stages(void *arg ) {

    unsigned long long  *end_addr, *patt, split_size, patt_8 ;
    volatile unsigned long long read_value;
    struct thread_context *t = (struct thread_context *)arg;
    int i, j, k, th_index;
    unsigned int num_page_ops, cache_line_size, pat_index = 0,patt_size;
    unsigned long long index, start_index, max_index, next_oper_index = 0;
    unsigned long long *addr_8;
    int tc = t->tc_id;
    int cpu = t->bind_to_cpu;
    int rc, oper, len_and_mask = 0, len_or_mask = 0;
    char msg[200];

    rc = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    if(rc != 0) {
        sprintf(msg, "\n Unable to set canceltype errno - %d \n",errno);
        hxfmsg(&htx_d, errno, HTX_HE_SOFT_ERROR, msg);
    }

#ifdef __HTX_LINUX__
    rc = htx_bind_thread(BIND_THE_THREAD, cpu);
#else
    rc = bindprocessor(BINDTHREAD, thread_self(), cpu);
#endif
    if(rc == -1) {
        sprintf(msg, "\n %d:bindprocessor failed with %d", __LINE__, errno);
        hxfmsg(&htx_d, errno, HTX_HE_SOFT_ERROR, msg);
    }

    th_index = mp_start(&t->th_htx_d);
    if(th_index == -1) {
        sprintf(msg, "\n Unable to get unique thread index, th_index = %d \n",th_index);
        hxfmsg(&htx_d, errno, HTX_HE_SOFT_ERROR, msg);
        /* This would make all  failing instance to use same locks */
        th_index = 0;
    }

    /*
     * If num_oper is 0 then run infinitely else num_oper times.
     */

    for(oper = 0; oper < t->num_oper || t->num_oper == 0; oper++) {
        if(exit_flag || Recvd_SIGUSR2) {
            break;
        }
		DEBUGON("thread_no = %d, num_oper = %d, patt_index = %d, next_oper_index = %d \n", cpu, oper, pat_index, next_oper_index);
        /*
         * WRC algorithm: Write to every byte in the 64MB region using double
         * word load/store instructions, read back the entire region and compare
         * each byte. Repeat the process for the 9 data patterns.
         */
        for(i = 0; i < t->wrc_iterations; i++) {
            /* Write into the whole 64M area */
	        addr_8 = (unsigned long long *)t->seg_addr;
            end_addr = (unsigned long long *) ((char *)addr_8 + t->memory_set_size);

			if(t->fixed_pattern.pattern[pat_index].pattern_type == PATTERN_ADDRESS) {
				for(; addr_8 < end_addr; )  {
					*addr_8 = (unsigned long long)addr_8 ;
					addr_8++;
				}
			} else
			if(t->fixed_pattern.pattern[pat_index].pattern_type == PATTERN_RANDOM ) {
				max_index = (t->randbuf_size / BIT_PATTERN_WIDTH);
				start_index = (cpu + oper + i +  next_oper_index) % max_index  ;
				len_and_mask = 0; 
				len_or_mask = 0; 
				DEBUGON("\nWRC Rand_Write, oper = %d, cpu = %d, next_oper_index = %d, start_index = %d, seg_addr=0x%llx, rbuf_index=0x%llx \n", 
							oper, cpu, next_oper_index, start_index, addr_8,(unsigned long long*)&t->rand_buffer[start_index]); 
				for(; addr_8 < end_addr; )  {
					*addr_8 = ((t->rand_buffer[start_index] & t->and_mask[len_and_mask]) | t->or_mask[len_or_mask]);
                    addr_8++;
                    start_index = ((start_index + 1) % max_index) ;
					len_and_mask = ((len_and_mask + 1) % t->and_mask_len); 
					len_or_mask = ((len_or_mask + 1) % t->or_mask_len); 
                }
			} else
			if(t->fixed_pattern.pattern[pat_index].pattern_type == PATTERN_FIXED) {
				start_index = 0;
				patt = (unsigned long long *)t->fixed_pattern.pattern[pat_index].actual_pattern;
            	patt_size = t->fixed_pattern.pattern[pat_index].pattern_size;
				max_index = (patt_size / BIT_PATTERN_WIDTH) ;
           		DEBUGON("\nWRC Fix_Write: oper = %d cpu=%d, addr=0x%llx, pattern_type=%d pattern_name=%s",
                           	oper, cpu, addr_8, t->fixed_pattern.pattern[pat_index].pattern_type, t->fixed_pattern.pattern[pat_index].pattern_name);
				for(; addr_8 < end_addr; )  {
					*addr_8 = *(unsigned long long *)(patt + start_index) ;
                    addr_8++;
					start_index = ((start_index + 1) % max_index);
				}
			}
            if (t->compare != TRUE) {
                /* Read back the pattern in scope. */
                addr_8 = (unsigned long long *)t->seg_addr;
                end_addr = (unsigned long long *) ((char *)addr_8 + t->memory_set_size);
                DEBUGON("\nWRC Read only: oper = %d, cpu=%d, start addr=%lx,"
                                " end addr=%lx", oper, cpu, addr_8, end_addr);
                for(; addr_8 < end_addr; addr_8++) {
                    read_value = *addr_8;
                }
            }
            else {  /* if compare == TRUE */
                /* Write into the whole 64M area */
                addr_8 = (unsigned long long *)t->seg_addr;
                end_addr = (unsigned long long *) ((char *)addr_8 + t->memory_set_size);

				if(t->fixed_pattern.pattern[pat_index].pattern_type == PATTERN_ADDRESS) {
					 for(index = 0; addr_8 < end_addr; index++)  {
						patt_8 = (unsigned long long )addr_8 ;
						read_value = *addr_8;
						if ((read_value != patt_8) && (crash_on_misc_global == 1) &&
                                (t->crash_on_misc == 1)) {
							#ifndef __HTX_LINUX__
								trap(0xBEEFDEAD, t->seg_addr, t->seg_addr, (index * BIT_PATTERN_WIDTH), addr_8, &patt_8 );
							#else
								do_trap_htx64(0xBEEFDEAD, t->seg_addr, t->seg_addr, (index * BIT_PATTERN_WIDTH), addr_8, &patt_8 );
							#endif
                                sprintf(msg, "Found Miscompare !! Expected value = %lx Actual value = %lx", patt_8, read_value);
                                hxfmsg(&htx_d, -1, HTX_HE_MISCOMPARE, msg);
                                return;
 		                }
						addr_8 ++;
					}
				} else
				if(t->fixed_pattern.pattern[pat_index].pattern_type == PATTERN_RANDOM ) {
					max_index = (t->randbuf_size / BIT_PATTERN_WIDTH);
					start_index = (cpu + oper + i +  next_oper_index) % max_index ;
					len_and_mask = 0; 
					len_or_mask = 0; 
        	        for(index = 0; addr_8 < end_addr; index++ )  {
                        patt_8 = ((t->rand_buffer[start_index ] & t->and_mask[len_and_mask]) | t->or_mask[len_or_mask]);
                        read_value = *addr_8; ;
						if ((read_value != patt_8) && (crash_on_misc_global == 1) &&
                                (t->crash_on_misc == 1)) {
							#ifndef __HTX_LINUX__
								trap(0xBEEFDEAD, &t->rand_buffer[0], t->seg_addr,
                                                                (index * BIT_PATTERN_WIDTH), addr_8, &patt_8);
                            #else
                                do_trap_htx64(0xBEEFDEAD, &t->rand_buffer[0], t->seg_addr,
																(index * BIT_PATTERN_WIDTH), addr_8, &patt_8);
                            #endif

                                sprintf(msg, "Found Miscompare !! Expected value = %lx Actual value = %lx", patt_8, read_value);
                                hxfmsg(&htx_d, -1, HTX_HE_MISCOMPARE, msg);
                                return;
    	                }
						addr_8++;
						len_and_mask = ((len_and_mask + 1) % t->and_mask_len);
                    	len_or_mask = ((len_or_mask + 1) % t->or_mask_len);
                    	start_index = ((start_index + 1) % max_index);
					}
				} else
				if(t->fixed_pattern.pattern[pat_index].pattern_type == PATTERN_FIXED) {
					start_index = 0 ;
                    patt = (unsigned long long *)t->fixed_pattern.pattern[pat_index].actual_pattern;
                	patt_size = t->fixed_pattern.pattern[pat_index].pattern_size;
					max_index = (patt_size / BIT_PATTERN_WIDTH) ;
           			DEBUGON("\nWRC Fix_Read: oper = %d cpu=%d, addr=0x%llx, pattern_type=%d pattern_name=%s",
                           	oper, cpu, addr_8, t->fixed_pattern.pattern[pat_index].pattern_type, t->fixed_pattern.pattern[pat_index].pattern_name);
					for(index = 0; addr_8 < end_addr; index++ )  {
                        patt_8 = *(unsigned long long *)(patt + start_index );
						read_value = *addr_8;
						if ((read_value != patt_8) && (crash_on_misc_global == 1) &&
                                (t->crash_on_misc == 1)) {
							#ifndef __HTX_LINUX__
								trap(0xBEEFDEAD, patt, t->seg_addr, (index * BIT_PATTERN_WIDTH), addr_8, &patt_8 );
							#else
								do_trap_htx64(0xBEEFDEAD, patt, t->seg_addr, (index * BIT_PATTERN_WIDTH), addr_8, &patt_8 );
							#endif
                                sprintf(msg, "Found Miscompare !! Expected value = %lx Actual value = %lx", patt_8, read_value);
                                hxfmsg(&htx_d, -1, HTX_HE_MISCOMPARE, msg);
                                return;
                        }
						addr_8++;
                        start_index = ((start_index + 1) % max_index);
                    }
				}
            } /* end: if compare != TRUE */
			if(t->fixed_pattern.pattern[pat_index].pattern_type == PATTERN_FIXED) { 
        		/* Change pattern every wrc_data_pattern_iterations */
        		if ((i % t->wrc_data_pattern_iterations) == 0) {
            		pat_index++;
            		if (pat_index == t->fixed_pattern.num_patterns) {
                		pat_index=0;
            		}
        		}
			} 
			if(t->fixed_pattern.pattern[pat_index].pattern_type == PATTERN_RANDOM) { 
				/* If random buffer size is greater than max bytes we are writing
				 * in one oper, then we should read the whole random buffer
				 * next_oper_index should point to next memory_set_size we are 
			 	 * writing. 
				 */
				 next_oper_index = (( next_oper_index + (t->memory_set_size / BIT_PATTERN_WIDTH)) % (t->randbuf_size / BIT_PATTERN_WIDTH));    
				 
			}	
    	} /* Off wrc iteration loop */
        /* Keep track of bytes written and read */
        t->th_htx_d.bytes_writ = t->wrc_iterations * t->memory_set_size;
        t->th_htx_d.bytes_read = t->wrc_iterations * t->memory_set_size;


        /* Check exit signal */
        if (exit_flag || Recvd_SIGUSR2) {
             break;
        }
        /* For algos that need to divide the 64M region and operate */
        split_size = t->memory_set_size/2;

        /* cinfo[1] points to the L3 cache info; line size = 128 */
        cache_line_size = L3CACHE_LINESIZE;

        /*
         * Copy algorithm: Split the 64MB region in half.
         * Access 1st 8 bytes of a cache line using double word load/store
         * instructions with 128B offset. Preload the 64MB buffer with data.
         * Re-use the current data set through the above WRC exercise.
         */
         DEBUGON("\nCOPY algo: oper = %d, cpu=%d, Copy to addr=%lx", oper, cpu, t->seg_addr);
         for(i = 0; i < t->copy_iterations; i++) {
            /* Set the copy-to:addr_8 pointer */
            addr_8 = (unsigned long long *)t->seg_addr;
            /* Not really the end addr. Instead, the address to copy from */
            end_addr = (unsigned long long *) ((char *)t->seg_addr +
                                                   split_size);
            for(k = 0; k < split_size; k += cache_line_size) {
                *addr_8  = *end_addr;    /* Copy, finally! */
                addr_8   = (unsigned long long *)((char *)addr_8+cache_line_size);
                end_addr = (unsigned long long *)((char *)end_addr+cache_line_size);
            }
        }

        /* Keep track of bytes written and read */
        t->th_htx_d.bytes_read += t->copy_iterations * split_size;
        t->th_htx_d.bytes_writ += t->copy_iterations * split_size;


        /* Check exit signal */
        if (exit_flag || Recvd_SIGUSR2) {
           break;
        }

        /*
         * Add algorithm: Split the 64MB region in half.
         * Run the add algorithm. Access all of bytes of the cache line.
         */
        DEBUGON("\n ADD algo: oper = %d, cpu=%d, Operand 1 addr = %lx", oper, cpu, t->seg_addr);
        for(i = 0; i < t->add_iterations; i++) {
           addr_8 = (unsigned long long *)t->seg_addr;
           /* Not really the end addr. Instead, address of 2nd operand */
           end_addr = (unsigned long long *) ((char *)t->seg_addr +
                                               split_size);
           for(k = 0; k < split_size ; k += (sizeof(unsigned long long))) {
                *addr_8 = *addr_8+*end_addr;
                addr_8++;
                end_addr++;
           }
        }
        /* Keep track of bytes written and read */
        t->th_htx_d.bytes_read += t->add_iterations * t->memory_set_size;
        t->th_htx_d.bytes_writ += (t->add_iterations * t->memory_set_size)/2;


        /* Check exit signal */
        if (exit_flag || Recvd_SIGUSR2) {
            break;
        }
		/*
         * Daxpy algorithm: Split the 64MB region in half.
         * Run a daxpy algorithm: a[k] = a[k]+scalar*b[k]; for k=0 to N
         */
        DEBUGON("\nDAXPY algo: oper =%d, cpu=%d, Operand 1 addr = %lx", oper, cpu, t->seg_addr);
        for(i=0; i < t->daxpy_iterations; i++) {
            addr_8 = (unsigned long long *)t->seg_addr;
            end_addr = (unsigned long long *) ((char *)t->seg_addr +
                                                  split_size);
            int k_max;
            k_max = split_size/(sizeof(unsigned long long));

            for(k = 0; k < k_max ; k += (sizeof(unsigned long long))) {
                addr_8[k] = addr_8[k]+(t->daxpy_scalar*end_addr[k]);
            }
        }

        /* Keep track of bytes written and read */
        t->th_htx_d.bytes_read += t->daxpy_iterations * t->memory_set_size;
        t->th_htx_d.bytes_writ += (t->daxpy_iterations * t->memory_set_size)/2;

        /* Check exit signal */
        if (exit_flag || Recvd_SIGUSR2) {
            break;
        }
        /* Update pass count at each iteration */

        hxfupdate(UPDATE, &t->th_htx_d);
    } /* end of num_oper loop */

#ifdef __HTX_LINUX__
    rc = htx_unbind_thread();
#else
    rc = bindprocessor(BINDTHREAD, thread_self(), PROCESSOR_CLASS_ANY);
#endif
    if(rc == -1) {
        sprintf(msg, "\n%d: bindprocessor failed with %d while unbinding",__LINE__,  errno);
        hxfmsg(&htx_d, errno, HTX_HE_SOFT_ERROR, msg);
    }
    return;
}


#ifdef _DR_HTX_
void
DR_handler(int sig, int code, struct sigcontext *scp) {

    int  rc = 0, dr_mem = 0;
    pid64_t pid;
    char tmp[128], workstr[80];
    dr_info_t DRinfo;
        char msg[100];

    if(dr_mem > 0) { /* To handle memory related DR Operation */
        --dr_mem;
        return;
    }
    pid = getpid();
    do {
        rc = dr_reconfig(DR_QUERY, &DRinfo);
    } while ((rc < 0) && (errno == 4));

    if (rc < 0) {
        sprintf(msg, "%s for %s: Error in DR handler.  \n\
        dr_reconfig system call returned error(%d).\n",
        htx_d.HE_name, htx_d.sdev_id, errno);
        hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
        return;
    }
    sprintf(msg, "dr_reconfig output in %s for %s: Check Phase: %d, Pre phase: %d, Doit Phase: %d, Post Phase: %d, Posterr or Phase: %d,Mem: %d, Lcpu: %d, Bcpu: %d\n",
                                htx_d.HE_name, htx_d.sdev_id, DRinfo.check, DRinfo.pre, DRinfo.doit, DRinfo.post, DRinfo.posterror, DRinfo.mem, DRinfo.lcpu, DRinfo.bcpu);
    hxfmsg(&htx_d,0,HTX_HE_INFO,msg);

    if ((DRinfo.check == 1) && (DRinfo.mem == 1)){
        dr_mem = 2;
        return;
    }
    else if ((DRinfo.check == 1) && (DRinfo.mem != 1)){
        dr_mem = 0;
    }
    #ifdef __ENT_CAP__
    /*
     * ent_cap indicates that the DR is only for a change in
     * processing power so no need to exit. Just send ACK to OS for DR.
     */
    if (DRinfo.ent_cap == 1) {
        if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)){
            sprintf(msg,"dr_reconfig(DR_RECONFIG_DONE) failed with errno %d \n", errno);
            hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
            return;
        }
    }
    #endif
    /* cpu remove or add operation */
    if (DRinfo.cpu == 1 && DRinfo.pre == 1) {
        sprintf(msg, "hxecache: DR in pre phase. It will exit soon !");
        hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
        hxfupdate(RECONFIG, &htx_d);
        exit_flag = 1;
        return;
    } else {
        if(dr_reconfig(DR_RECONFIG_DONE,&DRinfo)) {
            sprintf(msg,"dr_reconfig(DR_RECONFIG_DONE) failed.  error no %d \n", errno);
            hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
        }
    } /* rem cpu */
}
#endif

int
memory_set_cleanup() {

    int rc, i;
	char msg[100];
    for(i = 0; i < MAX_CPUS + 1; i++) {

        if( NULL != mem[i].seg_addr) {
			rc = munlock(mem[i].seg_addr, mem[i].memory_set_size);
        	if(rc == -1) {
            	sprintf(msg,"\n munlock failed with %d, for seg_addr=%#llx, size=%#x \n", errno, mem[i].seg_addr, mem[i].memory_set_size);
            	hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
            	return(-1);
        	}

            rc = shmdt(mem[i].seg_addr);
            if(rc != 0) {
                sprintf(msg,"\n%s: shmdt failed with %d for seg_addr =%#llx \n",__FUNCTION__, errno, mem[i].seg_addr);
                hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
                return(-1);
            }
            mem[i].seg_addr=NULL;
            if(mem[i].shm_id != -1) {
                rc = shmctl(mem[i].shm_id, IPC_RMID, (struct shmid_ds *) NULL);
                if(rc != 0) {
                    sprintf(msg,"\n%s: shmctl for IPC_RMID failed with %d for id=%#x \n",__FUNCTION__, errno, mem[i].shm_id);
                    hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
                    return(-1) ;
                }
                mem[i].shm_id= -1 ;
            }
        }
    }
    return(0);
}


long long int 
GetMemoryDetails(char string[], struct htx_data * htx_d) { 

    FILE * meminfo = NULL; 
    char line[256], command[1024];
	int rc = 0; 
	unsigned long long mem_size = 0; 

	sprintf(command, "cat /proc/meminfo | awk ' /%s/ { print $2}'", string); 
	meminfo = popen(command, "r"); 
    if(meminfo == NULL || meminfo == -1) { 
		sprintf(htx_d->msg_text, "popen failed for /proc/meminfo, rc = %#x, string=%s\n", meminfo, string); 
		hxfmsg(htx_d, NULL, HTX_HE_INFO, htx_d->msg_text); 
		return(-1); 
	}	
	rc = fscanf(meminfo,"%llu\n", &mem_size); 
	if(rc == 0 || rc == EOF) { 
		sprintf(htx_d->msg_text, "Cannot find %s keyword in /proc/meminfo, rc = %#x \n",string, rc); 
		hxfmsg(htx_d, NULL, HTX_HE_INFO, htx_d->msg_text); 
	}
    fclose(meminfo);
	/* Input parms is in KB */ 
	if(strstr(string, "HugePages") != NULL) { 
		mem_size = mem_size * 16 * M; 
	} else { 
		mem_size = mem_size * 1024; 
	}
	printf("GetMemoryDetails found keywd = %s, mem_size=%#llx\n",string, mem_size); 
    return(mem_size);
}
