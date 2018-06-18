/* IBM_PROLOG_BEGIN_TAG */
/*
 * Copyright 2003,2016 IBM International Business Machines Corp.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/* IBM_PROLOG_END_TAG */

/* @(#)47       1.3.3.25  src/htx/usr/lpp/htx/bin/show_syscfg/show_syscfg.c, htxconf, htxubuntu 8/20/17 09:51:37 */

#include "htxsyscfg64.h"

int main(int argc, char* argv[])
{
    int              i,dynamic=0;
    int rc,rc1,rc2;
    rc=rc1=rc2=0;
    unsigned int     pvr_value_os,pvr_value_true, proc_version, proc_revision;
    SYS_STAT         Sys_stat; 
    unsigned int     Pvr,j,k;

    htxsyscfg_lpar_t t;
    htxsyscfg_core_t u;
    htxsyscfg_memory_t v;
	htxsyscfg_env_details_t e;
    htxsyscfg_cache_t w;
    if((argc==2) && (strcmp(argv[1],"shm")==0)) {
	    attach_to_syscfg();
    }
	else{
		init_syscfg_with_malloc();
	}

    pvr_value_true = get_true_cpu_version();
	pvr_value_os = get_cpu_version();
    Pvr = pvr_value_true>>16;

    proc_version = pvr_value_os >> 16;
    proc_revision = pvr_value_os & 0xffff;

    if((argc==2) && (strcmp(argv[1],"dynamic")==0)) {
        dynamic = 1;
    }


    if((argc==2) && (strcasecmp(argv[1],"pvr")==0)) {
        printf("OS PVR Value                            : 0x%x\n",pvr_value_os);
		printf("TRUE PVR Value                          : 0x%x\n",pvr_value_true);
        printf("Processor Version                       : 0x%x\n",proc_version);
        printf("Processor Revision                      : 0x%x\n",proc_revision);
        #ifndef __HTX_LINUX__
        printf("Power6Compatmode                        : %s\n",(__power_version_6_Compat())?"True":"False" );
        #endif
        exit(0);
    }

    if((argc==2) && (strcasecmp(argv[1],"help")==0)) {
        printf("type 'pvr' for pvr details \n");
		printf("type 'shm' for system information from shm \n");
        exit(0);
    }


    get_lpar_details(&t);

char cP6_Compat_String[256] = {0};
char cp7_Compat_String[256] = {0};
char cp8_Compat_String[256] = {0};


if ( get_p6_compat_mode(Pvr) )
{
	strcpy (cP6_Compat_String, "True");
}
else
{
	strcpy (cP6_Compat_String, "False");
}

if ( get_p7_compat_mode(Pvr) )
{
	strcpy (cp7_Compat_String, "True");
}
else
{
	strcpy (cp7_Compat_String, "False");
}

if ( get_p8_compat_mode(Pvr) )
{
        strcpy (cp8_Compat_String, "True");
}
else
{
        strcpy (cp8_Compat_String, "False");
}


get_env_details(&e);


	
    if(!dynamic) {
        printf("---------------------------LPAR details---------------------------\n");
        printf("Hostname                                : %s\n",t.host_name);
		printf("Host Endianness                         : %s\n",e.endianess);
		printf("Machine type                            : %s\n",e.virt_typ);
		printf("shared_processor_mode			: %s\n",e.proc_shared_mode);	
        printf("MTM                                     : %s\n",t.mtm);
        printf("OS version                              : %s\n",t.os_version);
        #ifndef __HTX_LINUX__
        if(t.partition.partition_type==PART_DEDICATED)
            printf("Partition type                          : Dedicated partition\n");
        else if(t.partition.partition_type==PART_SHARED)
            printf("Partition type                          : Shared partition\n");
        else if(t.partition.partition_type==PART_FULL)
            printf("Partition type                          : Full system partition\n");

        printf("Partition name                          : %s\n",t.partition.partition_name);
        printf("Firmware version                        : %s\n",t.firmware_version);
        #endif
        printf("Com IP                                  : %s\n\n",t.com_ip);
        printf("---------------------------PVR details---------------------------\n");
        printf("OS PVR Value                            : 0x%x\n",pvr_value_os);
		printf("TRUE PVR Value                          : 0x%x\n",pvr_value_true);
		printf("TRUE PVR ver                            : 0x%x\n",Pvr);
        printf("Processor Version                       : 0x%x\n",proc_version);
	if(proc_version == PV_POWER9_NIMBUS || proc_version == PV_POWER9_CUMULUS) 
		printf("Fused/Normal core type			: %s\n",(get_p9_core_type()? "Normal":"Fused"));
        printf("Processor Revision                      : 0x%x\n",proc_revision);
        printf("Power6Compatmode                        : %s\n",cP6_Compat_String);
        printf("Power7Compatmode                        : %s\n",cp7_Compat_String);
        printf("Power8Compatmode                        : %s\n",cp8_Compat_String);

    }

    if(dynamic) {
        printf("\n");
    }

    printf("Time stamp                              : %s\n",t.time_stamp);

    printf("\n\n---------------------------CPU  details---------------------------\n");

    printf("Physical cpus                           : %d\n",t.cpus.p_cpus);
    printf("Logical cpus                            : %d\n",t.cpus.l_cpus);
    printf("Virtual cpus                            : %d\n",t.cpus.v_cpus);

/*
    printf("Physical-virtual cpu percentage         : %.2f%%\n",t.cpus.phy_to_virt*100);
    printf("Physical-logical cpu percentage         : %.2f%%\n",t.cpus.phy_to_logical*100);
*/

    printf("\n\n---------------------------Core details---------------------------\n");


    get_core_details(&u);

    printf("Smt threads                             : %d\n",u.smtdetails.smt_threads);
	printf("Min smt threads                         : %d\n",u.smtdetails.min_smt_threads);
	printf("Max smt threads                         : %d\n",u.smtdetails.max_smt_threads);
    printf("Processor speed                         : %llu Hz\n", u.cpu_clock_freq);
    if(!dynamic) {
        printf("\n\n---------------------------Nest details---------------------------\n");

        L1cache(&w);

        printf("L1 instruction cache size               : %d KB\n",w.L1_isize/1024);
        printf("L1 data cache size                      : %d KB\n",w.L1_dsize/1024);
        printf("L1 instruction cache associativity      : %d\n",w.L1_iasc);
        printf("L1 data cache associativity             : %d\n",w.L1_dasc);
        printf("L1 instruction cache line size          : %d bytes\n",w.L1_iline);
        printf("L1 data cache line size                 : %d bytes\n",w.L1_dline);

        #ifdef __HTX_LINUX__
        L2L3cache(&w);
        #else
        L2cache(&w);
        L3cache(&w);
        #endif

        printf("L2 cache size                           : %.2f MB\n",w.L2_size/1024/1024);
        printf("L2 cache associativity                  : %d\n",w.L2_asc);
        printf("L3 cache size                           : %d MB\n",w.L3_size/1024/1024);
        printf("\n");

    }


    printf("\n\n--------------------------Memory details--------------------------\n");


	rc = get_memory_details(&v);
    if(rc < 0){
		printf("get_memory_details() returned rc = %d\n",rc);
		exit(1);
	}

    printf("Real memory size                        : %llu MB\n", v.mem_size.real_mem/(1024*1024));
    printf("Free real memory                        : %llu MB\n",v.mem_size.free_real_mem/(1024*1024));
	for(i=0; i<MAX_PAGE_SIZES; i++) {	
		if(v.page_details[i].supported) {
			printf("Total pages(%8d)                   : %lu\n",v.page_details[i].page_size,v.page_details[i].total_pages);
			printf("Free pages(%8d)                    : %lu\n",v.page_details[i].page_size,v.page_details[i].free_pages);
		}
	} 

	printf("\n\n--------------------------Pool wise Memory details--------------------------\n");
	for(j=0;j<v.num_numa_nodes;j++){
		if(v.mem_pools[j].has_cpu_or_mem){
			printf("NUMA Node no:%d\n",v.mem_pools[j].node_num);
			printf("        cpus:%d=",v.mem_pools[j].num_procs);
			for(k=0;k<v.mem_pools[j].num_procs;k++){
				printf(":%d",v.mem_pools[j].procs_per_pool[k]);
			}
			printf("\n");
			printf("        Total mem            :%llu bytes\n",v.mem_pools[j].mem_total);
			printf("        Free mem             :%llu bytes\n",v.mem_pools[j].mem_free);
			for(i=0; i<MAX_PAGE_SIZES; i++) {
				if(v.mem_pools[j].page_info_per_mempool[i].supported) {
					printf("        Total pages(%8d):%lu\n",v.mem_pools[j].page_info_per_mempool[i].page_size,v.mem_pools[j].page_info_per_mempool[i].total_pages);
					printf("        Free pages(%8d) :%lu\n",v.mem_pools[j].page_info_per_mempool[i].page_size,v.mem_pools[j].page_info_per_mempool[i].free_pages);
				}
			}
		}
	}


    printf("\n");
        #ifndef __HTX_LINUX__
    printf("Total paging space                      : %llu MB\n",v.page_details.paging_space_total*4/1024);
    printf("Free paging space                       : %llu MB\n",v.page_details.paging_space_free*4/1024);


    printf("\nNumber of available memory pools        : %d \n",v.mem_pools.mpools);
    printf("Maximum number of memory pools          : %d\n",v.mem_pools.maxmpools);
    printf("Maximum number of processors that may be contained in the rset : %d\n", v.mem_pools.maxprocs);
    printf("Amount of available memory in the rset  : %d MB\n",v.mem_pools.memsize);
        #endif


    printf("\n\n--------------------------Hardware details--------------------------\n");
    Sys_stat.nodes = 0;
    Sys_stat.chips = 0;
    Sys_stat.cores = 0;

    get_hardware_stat(&Sys_stat);
	int cores_exc = 0;
	
	if(!strcmp(e.proc_shared_mode,"yes"))
	{
		printf("Number of nodes \t\t\t: NA\n");
		printf("Number of chips \t\t\t: NA\n");
    	printf("Number of Cores \t\t\t: %u\n",Sys_stat.cores);
    	printf("Number of Cpus  \t\t\t: %u\n",Sys_stat.cpus);
    	printf("\n\n--------------------------CPUS PER NODE-----------------------------\n");
		printf("Can not be determined on shared proc mode  \n");  
		printf("\n\n--------------------------CPUS PER CHIP-----------------------------\n");
		printf("Can not be determined on shared proc mode  \n");
		printf("\n\n--------------------------CPUS PER CORE-----------------------------\n");
		printf("Can not be determined on shared proc mode  \n");
		
	}
	else
	{
		printf("Number of nodes \t\t\t: %u\n",Sys_stat.nodes);
    	printf("Number of chips \t\t\t: %u\n",Sys_stat.chips);
    	printf("Number of Cores \t\t\t: %u\n",Sys_stat.cores);
    	printf("Number of Cpus  \t\t\t: %u\n",Sys_stat.cpus);
		for(i=0;i<16;i++){
			for(j=0;j<3;j++){
				if(global_ptr->syscfg.numberArray[i][j] != -2){
					cores_exc++;
				}
			}
		}
		cores_exc = cores_exc/3;
		if(cores_exc != 0){
			printf("Number of excluded cores \t\t: %d",cores_exc);
			printf(" (Hotplug not supported) \n");

		printf("\n\n--------------------------CORES EXCLUDED------------------------------\n");
        
		printf("Values \t\t\t\t\t: Node \t Chip \t Core\n");
			for(i=0; i<cores_exc; i++){
	        	    printf("Entry %d \t\t\t\t:   %d \t   %d \t   %d \t ",i,global_ptr->syscfg.numberArray[i][0],global_ptr->syscfg.numberArray[i][1],global_ptr->syscfg.numberArray[i][2]);
	        	    printf("\n");
		        }
		}

	printf("\n\n--------------------------CHIPS PER NODE-----------------------------\n");
	signed int chips_in_node[MAX_CHIPS_PER_NODE];
    int node_no;
	for(node_no=0; node_no < Sys_stat.nodes; node_no++) {
		rc = get_chips_in_node(node_no,chips_in_node);
		if (rc == -1) {
			printf("Something wrong. Check node number\n");
			exit(1);
		}
		 printf("Chips in node %d (%d)\t\t\t:",node_no,rc);
            for(i=0; i<rc;i++) {
                printf("%5d",chips_in_node[i]);
            }
            printf("\n");
        }

	
	
	    printf("\n\n--------------------------CORES PER NODE-----------------------------\n");
    	signed int cores_in_node[MAX_CORES_PER_NODE];
    	signed int cpus_in_node[MAX_THREADS_PER_NODE];
    	int rc = -1;
    	for(node_no=0; node_no < Sys_stat.nodes; node_no++) {
        	rc = get_cores_in_node(node_no,cores_in_node);
			if (rc == -1) {
				printf("Something wrong. Check node number\n");
				exit(1);
			}
        	printf("Cores in node %d (%d)\t\t\t:",node_no,rc);
        	for(i=0; i<rc;i++) {
            	printf("%5d",cores_in_node[i]);
        	}
        	printf("\n");
    	}
    rc1 = pthread_rwlock_rdlock(&(global_ptr->syscfg.rw));
    if (rc1 !=0  ) {
        printf("\n lock inside get_hardware_config failed with errno=%d\n",rc1);
    }
	

		printf("\n\n--------------------------CPUS PER NODE-----------------------------\n");
		rc = -1;
		for(node_no=0; node_no < Sys_stat.nodes; node_no++) {
				rc = get_cpus_in_node(node_no,cpus_in_node);
				if (rc == -1) {
					printf("Something wrong. Check node number\n");
				}else{
					printf("CPUs in node %d (%d)\t\t\t:",node_no,rc);
					for(i=0; i<MAX_THREADS_PER_NODE; i++) {
						if(cpus_in_node[i] == -1)
									break;
						else {
							printf("%5d",cpus_in_node[i]);    
							if ((global_ptr->syscfg.duplicate[cpus_in_node[i]][0]) != 0) {   /* This proc is duplicated */
								for(j=1; j<=(global_ptr->syscfg.duplicate[cpus_in_node[i]][0]); j++) {
									printf("/%d",(global_ptr->syscfg.duplicate[cpus_in_node[i]][j]));
								}
							}
						}
					}
					printf("\n");
				}
		}

		printf("\n\n--------------------------CPUS PER CHIP-----------------------------\n");
		signed int cpus_in_chip[MAX_THREADS_PER_CHIP];
		int chip_no;
		rc = -1;
		for(chip_no=0; chip_no < Sys_stat.chips; chip_no++) {
		rc = get_cpus_in_chip(chip_no,cpus_in_chip);
				if (rc == -1) {
					printf("Something wrong. Check chip number\n");
				}else{
					printf("CPUs in chip %d (%d)\t\t\t:",chip_no,rc);
					for(i=0; i<MAX_THREADS_PER_CHIP; i++) {
						if(cpus_in_chip[i] == -1)
							break;
						else {
							printf("%5d",cpus_in_chip[i]); 
							if ((global_ptr->syscfg.duplicate[cpus_in_chip[i]][0]) != 0) {   /* This proc is duplicated */
								for(j=1; j<=(global_ptr->syscfg.duplicate[cpus_in_chip[i]][0]); j++) {
									printf("/%d",(global_ptr->syscfg.duplicate[cpus_in_chip[i]][j]));
								}   
						}

					}
					}
					printf("\n");
				}
		}


		printf("\n\n--------------------------CPUS PER CORE-----------------------------\n");
		signed int cpus_in_core[MAX_THREADS_PER_CORE];
		int core_no;
		rc=-1;
		for(core_no=0; core_no<Sys_stat.cores; core_no++) {
        	rc = get_phy_cpus_in_core(core_no,cpus_in_core);
				if (rc == -1) {
					printf("Something wrong. Check core number\n");
				}else{
					printf("CPUs in core %d (%d)\t\t\t:",core_no,rc);
					for(i=0; i<MAX_THREADS_PER_CORE; i++) {
						if(cpus_in_core[i] == -1)
							break;
						else {
							printf("%5d",cpus_in_core[i]);    
							if ((global_ptr->syscfg.duplicate[cpus_in_core[i]][0]) != 0) {   /* This proc is duplicated */
								for(j=1; j<=(global_ptr->syscfg.duplicate[cpus_in_core[i]][0]); j++) {
									printf("/%d",(global_ptr->syscfg.duplicate[cpus_in_core[i]][j]));
								}
							}
						}
					}
					printf("\n");
				}
		}

	    rc2 = pthread_rwlock_unlock(&(global_ptr->syscfg.rw));
	    if (rc2 !=0  ) {
			printf("\n unlock inside get_hardware_config failed with errno=%d\n",rc2);
	    }

    	}

    if((argc==2) && (strcmp(argv[1],"shm")==0)) {
		printf("no detach in show_syscfg for shm");
    }
	else{
		detach_syscfg_with_malloc();
	}

    return 0;
}

