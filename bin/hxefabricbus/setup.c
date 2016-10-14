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

#define P9_NIMBUS_NODE_MASK 0x0000000000007800
#define P9_NIMBUS_CHIP_MASK 0x0000000000000700
#define P9_NIMBUS_CORE_MASK 0x000000000000007C

/*
0111 1000 0000 0000  8 possible nodes   >> 11
0000 0111 0000 0000  8 possible chips   >> 8
0000 0000 0111 1100  24 possible cores  >> 2
0000 0000 0000 0011  4 possible threads
*/


#define P9_CUMULUS_NODE_MASK 0x0000000000007800
#define P9_CUMULUS_CHIP_MASK 0x0000000000000700
#define P9_CUMULUS_CORE_MASK 0x0000000000000078

/*
0111 1000 0000 0000  8 possible nodes   >> 11
0000 0111 0000 0000  8 possible chips   >> 8
0000 0000 0111 1000  12 possible core chiplets  >> 3
0000 0000 0000 0111  8 possible threads
*/

#define P8_NODE_MASK 0x00001C00
#define P8_CHIP_MASK 0x00000380
#define P7_NODE_MASK 0x00000380
#define P7_CHIP_MASK 0x00000060
#define P6_NODE_MASK 0x000000E0
#define P6_CHIP_MASK 0x00000018

/* P9 macros for CUMULUS */
#define P9_CUMULUS_GET_NODE(_PIR_)   ((_PIR_ & P9_CUMULUS_NODE_MASK) >> 11)
#define P9_CUMULUS_GET_CHIP(_PIR_)   ((_PIR_ & P9_CUMULUS_CHIP_MASK) >> 8)
#define P9_CUMULUS_GET_CORE(_PIR_)   ((_PIR_ & P9_CUMULUS_CORE_MASK) >> 3)

/* P9 macros for NIMBUS */
#define P9_NIMBUS_GET_NODE(_PIR_)   ((_PIR_ & P9_NIMBUS_NODE_MASK) >> 11)
#define P9_NIMBUS_GET_CHIP(_PIR_)   ((_PIR_ & P9_NIMBUS_CHIP_MASK) >> 8)
#define P9_NIMBUS_GET_CORE(_PIR_)   ((_PIR_ & P9_NIMBUS_CORE_MASK) >> 2)

#define P8_GET_NODE(_PIR_) 	 (( _PIR_ & P8_NODE_MASK) >> 10)
#define P8_GET_CHIP(_PIR_)   (( _PIR_ & P8_CHIP_MASK) >> 7)
#define P8_GET_PHYSICAL_CHIP(_PIR_) ((_PIR_ & (P8_NODE_MASK | P8_CHIP_MASK)) >> 7)
#define P7_GET_NODE(_PIR_)   ((_PIR_ & P7_NODE_MASK) >> 7)
#define P7_GET_CHIP(_PIR_)   ((_PIR_ & P7_CHIP_MASK) >> 5)
#define P7_GET_PHYSICAL_CHIP(_PIR_) ((_PIR_ & (P7_NODE_MASK | P7_CHIP_MASK)) >> 5)
#define P6_GET_NODE(_PIR_)	 ((_PIR_ & P6_NODE_MASK) >> 5)
#define P6_GET_CHIP(_PIR_)	 ((_PIR_ & P6_CHIP_MASK) >> 3)
#define P6_GET_PHYSICAL_CHIP(_PIR_) ((_PIR_ & (P6_NODE_MASK | P6_CHIP_MASK)) >> 3)

extern int errno;
extern struct htx_data htx_d;
static int l_p[MAX_CPUS] = {-1};
extern int SET_PAGESIZE; 

unsigned long long
memory_per_thread(unsigned int pvr) ;


int
read_hardware_config(SYS_CONF_TYP * scfg, unsigned int tot_cpus, unsigned int pvr)  {

    int i, j = 0;
    int bind_to_cpu , rc, pir;
	int node, chip , proc;
    char msg[4096];
	SYS_CONF_TYP physical_scfg ;
	int num_nodes = 0, chips_per_node = 0;

    for(i = 0; i < MAX_CPUS ; i++) {
        l_p[i] = NO_CPU_DEFINED;
    }
#ifndef __HTX_LINUX__
    for(i = 0 ; i < tot_cpus ; i++ ) {
        bind_to_cpu = i ;
       	rc = bindprocessor(BINDPROCESS, getpid(), bind_to_cpu);
        if(rc == -1) {
            sprintf(msg, "\n bindprocessor failed with rc = %d, i = %d, errno = %d", rc, i, errno);
			hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }

        pir = getPir();
        l_p[i] = pir ;

       	rc = bindprocessor(BINDPROCESS, getpid(), PROCESSOR_CLASS_ANY);
        if(rc == -1) {
            sprintf( msg, "\n%d: bindprocessor failed with %d while unbinding, rc = %d, i = %d ",__LINE__,  errno, rc, i);
			hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }
    }
#else
	FILE *fp;
	char command[200],fname[256];
	int rad, lcpu, nrads, num_procs;

	strcpy(fname,htx_d.htx_exer_log_dir);
	sprintf(fname,"%s/node_details.%d",fname,getpid());
    /*sprintf(command,"/usr/lpp/htx/etc/scripts/get_node_details.sh "
            "> %s\n",fname);*/
    strcpy(command,getenv("HTXSCRIPTS"));
    sprintf(command, "%s/get_node_details.sh > %s\n", command, fname);

    if ( (rc = system(command)) == -1 ) {
            sprintf(msg, "sytem command to get_node_details failed with %d",rc);
            hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        	return(-1);
    }
    if ((fp=fopen(fname,"r"))==NULL){
            sprintf(msg, "fopen of file %s failed with errno=%d",fname,errno);
            hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
        	return(-1);
    }

    /* Get total number of chips */
    rc = fscanf(fp,"num_nodes=%d\n",&nrads);
    if (rc == 0 || rc == EOF) {
            sprintf(msg, "fscanf of num_nodes on file %s failed with errno =%d",fname,errno);
            hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
            return(-1);
    }
    for (rad = 0; rad < nrads; rad++) {
            int cpu_index = 0;
            int cur_chip = -1; /* Current chip */
            /* Fetch the current chip and the num procs in the chip */
            rc = fscanf(fp,"node=%d,cpus_in_node=%d,cpus",&cur_chip,&num_procs);
            if (rc == 0 || rc == EOF) {
                 sprintf(msg, "fscanf: chip =%d,Fetching of cpus in chip error from"
                        " file %s failed with errno =%d",rad,fname,errno);
                 hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
                 return(-1);
            }

            if ( num_procs > tot_cpus ) {
                sprintf(msg, "num_procs %d in this chip cannot be more than max procs %d "
                        ,num_procs,tot_cpus);
                hxfmsg(&htx_d,0,HTX_HE_HARD_ERROR, msg);
                return(-1);
            }

           /*
            * To find the actual processors (logical cpu Ids that can be
            * specified in an rset):
            */
            cpu_index = 0 ;

            while ( cpu_index < num_procs) {
                rc = fscanf(fp,":%d",&lcpu);
                if ( rc == 0 || rc == EOF) {
                    sprintf(msg, "fscanf: cpu fetch for chip=%d caused error from "
                                 " file %s failed with errno =%d",rad,fname,errno);
                    hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
                    return(-1);
                }
				/* Bind to lcpu and get the PIR value for it */
				rc = htx_bind_process(BIND_THE_PROCESS, lcpu);
				if(rc == -1) {
		            sprintf(msg, "\n bindprocessor failed with %d, binding cpu=%d", errno, lcpu);
            		hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg);
             		return(-1);
        		}

                l_p[lcpu] = get_cpu_id(lcpu);
				rc = htx_unbind_process();
				if(rc == -1) {
					sprintf(msg, "\n bindprocessor failed with %d, unbinding th=%d, cpu=%d", errno, lcpu);
					hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg);
					return(-1);
				}

				DEBUGON("l_p[%d]=%d\n",lcpu, l_p[lcpu]);
                cpu_index++;
            }
            rc = fscanf(fp,"\n");
            if (rc == EOF) {
                   sprintf(msg, "fscanf: expecting new line character at the end of"
                                " node %d data reading from file %s failed with errno =%d",\
                                rad,fname,errno);
                   hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
                   return(-1);
            }
     }

#endif
	/* Intialize the sysconf data structure */
	for(node = 0; node < MAX_NODES; node++) {
		for(chip = 0; chip < MAX_CHIPS_PER_NODE; chip ++) {
			for(proc = 0; proc < MAX_CPUS_PER_CHIP; proc ++) {
				physical_scfg.node[node].chip[chip].lprocs[proc] = -1;
			}
			physical_scfg.node[node].chip[chip].num_procs = 0;
		}
		physical_scfg.node[node].num_chips = 0;
	}
	physical_scfg.num_nodes = 0 ;

    i = 0 ;
   	while(i < MAX_CPUS) {
		if(l_p[i] == NO_CPU_DEFINED)  {
			i++;
			continue;
		}
        if(pvr == PVR_POWER9_NIMBUS){
            node = P9_NIMBUS_GET_NODE(l_p[i]);
            chip = P9_NIMBUS_GET_CHIP(l_p[i]);
        } else if(pvr == PVR_POWER9_CUMULUS){
            node = P9_CUMULUS_GET_NODE(l_p[i]);
            chip = P9_CUMULUS_GET_CHIP(l_p[i]);
        } else if(pvr == PVR_POWER8_MURANO || pvr == PVR_POWER8_VENICE || pvr == PVR_POWERP8P_GARRISION) {
			node = P8_GET_NODE(l_p[i]);
			chip = P8_GET_CHIP(l_p[i]);
    	} else if(pvr == PVR_POWER7 || pvr == PVR_POWER7PLUS) {
        	node = P7_GET_NODE(l_p[i]);
        	chip = P7_GET_CHIP(l_p[i]);
    	} else if(pvr == PVR_POWER6) {
			node = P6_GET_NODE(l_p[i]);
			chip = P6_GET_CHIP(l_p[i]);
		} else {
			sprintf(msg, "\n We shldnt have been here pvr = %x,  PVR mismatch. Exiting !!\n", pvr);
            hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(-1);
		}
		DEBUGON("Node=%d,Chip%d,proc=%d\n",node,chip,i);
       	physical_scfg.node[node].chip[chip].lprocs[physical_scfg.node[node].chip[chip].num_procs++] = i ;
       	i++;
    }

	for(i = 0; i < MAX_NODES; i ++) {
		for( j = 0; j < MAX_CHIPS_PER_NODE; j++) {
			if(physical_scfg.node[i].chip[j].num_procs > 0) {
				physical_scfg.node[i].num_chips ++;
			}
		}
		if(physical_scfg.node[i].num_chips > 0) {
			physical_scfg.num_nodes ++;
		}
	}

	/* Copy the system configuration to scfg */
	for(node = 0; node < MAX_NODES; node++) {
		if(physical_scfg.node[node].num_chips == 0)
			continue;
		chips_per_node = 0;
		for(chip = 0; chip < MAX_CHIPS_PER_NODE; chip ++) {

			if(physical_scfg.node[node].chip[chip].num_procs == 0)
				continue;
			memcpy(&scfg->node[num_nodes].chip[chips_per_node], &physical_scfg.node[node].chip[chip], sizeof(CHIP));
			chips_per_node++;
		}
		if(chips_per_node != physical_scfg.node[node].num_chips) {
			sprintf(msg, "%s: Something Wrong !!! Node = %d, chips_per_node = %d, physical_scfg_chip = %d \n",
													 __FUNCTION__,node, chips_per_node, physical_scfg.node[node].num_chips) ;
			hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }
		scfg->node[num_nodes].num_chips = physical_scfg.node[node].num_chips;
		num_nodes++;
	}
	if(num_nodes != physical_scfg.num_nodes) {
		sprintf(msg, "%s: Something Wrong !!! Num_nodes = %d, physical_num_nodes = %d \n",
										__FUNCTION__, num_nodes, physical_scfg.num_nodes);
		hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
        return(-1);
    }

	scfg->num_nodes = physical_scfg.num_nodes;

    return 0 ;
}

unsigned int
get_physical_number(unsigned int cpu, unsigned int mem_alloc, unsigned int pvr) {


   	if(l_p[cpu] == NO_CPU_DEFINED)
		return(-1);
	if(mem_alloc == 3) {
        if(pvr == PVR_POWER9_NIMBUS)
            return(P9_NIMBUS_GET_NODE(l_p[cpu]));
        if(pvr == PVR_POWER9_CUMULUS)
             return(P9_CUMULUS_GET_NODE(l_p[cpu]));
        
		if(pvr == PVR_POWER8_MURANO || pvr == PVR_POWER8_VENICE || pvr == PVR_POWERP8P_GARRISION) {
			return(P8_GET_NODE(l_p[cpu]));
    	} else if(pvr == PVR_POWER7 || pvr == PVR_POWER7PLUS) {
			return(P7_GET_NODE(l_p[cpu]));
    	} else if(pvr == PVR_POWER6) {
        	return(P6_GET_NODE(l_p[cpu]));
    	} else {
        	return(-1);
    	}
   	} else if(mem_alloc == 4) {
        if(pvr == PVR_POWER9_NIMBUS)
            return(P9_NIMBUS_GET_CHIP(l_p[cpu]));
        if(pvr == PVR_POWER9_CUMULUS)
             return(P9_CUMULUS_GET_CHIP(l_p[cpu]));
		if(pvr == PVR_POWER8_MURANO || pvr == PVR_POWER8_VENICE || pvr == PVR_POWERP8P_GARRISION) {
			return(P8_GET_PHYSICAL_CHIP(l_p[cpu]));
        } else if(pvr == PVR_POWER7 || pvr == PVR_POWER7PLUS) {
            return(P7_GET_PHYSICAL_CHIP(l_p[cpu]));
        } else if(pvr == PVR_POWER6) {
            return(P6_GET_PHYSICAL_CHIP(l_p[cpu]));
        } else {
            return(-1);
        }
	}
	return(-1);
}

int
configure_memory_allocation(SYS_CONF_TYP * scfg, unsigned int mem_alloc, unsigned int memory_mapping[][2], unsigned int * num_cpus_mapped, unsigned int threads_per_node, 
							unsigned long long num_pages_available, unsigned long long fabbus_requested, unsigned int pvr, unsigned int query_memconf, unsigned int query_pages) {

    char msg[4096];
    /*
     * cpu_node keeps nodes vs logical procs configurations.
     * cpu_chip keeps chip vs logical procs configurations.
     * This is filled by reading PIR register of each physical processor.
     * -1 indicates no logical cpu on that place.
     */
	 int cpu_sys[MAX_NODES][MAX_CHIPS][MAX_CPUS_PER_CHIP];
     int cpu_node[MAX_NODES][MAX_CPUS_PER_NODE];
     int cpu_chip[MAX_CHIPS][MAX_CPUS_PER_CHIP];

    /*
     * scfg_node maintains no of logical cpus on every node.
     * scfg_chip maintains no of logical cpus on every chip.
     */
     int scfg_sys[MAX_NODES][MAX_CHIPS],scfg_node[MAX_NODES], scfg_chip[MAX_CHIPS];

    /*
     * num_nodes : total number of nodes in system.
     * num_chips : total number of chips in system.
     * chips_per_node[] : number of chips available in each node
     */
     int num_nodes, num_chips;
     int chips_per_node[MAX_NODES] = {0};
     unsigned int i, j,k;
     int node, cpu, pjmp, chip, cpus_mapped = 0;

    /* Intialize local variables */
    num_nodes = num_chips = 0;
    memset(scfg_node, 0, MAX_NODES * sizeof(unsigned int));
    memset(chips_per_node, 0, MAX_NODES * sizeof(unsigned int));
    memset(scfg_chip, 0, MAX_CHIPS * sizeof(unsigned int));
	memset(scfg_sys,0,(MAX_NODES * MAX_CHIPS * sizeof(unsigned int)));

	for(i = 0; i < MAX_NODES; i++) {
		for(j = 0; j < MAX_CHIPS; j++) {
			for(k = 0; k<MAX_CPUS_PER_CHIP;k++) {
				cpu_sys[i][j][k] = NO_CPU_DEFINED;
			}
		}
	}
    for(i = 0; i < MAX_NODES; i++) {
        for(j = 0; j < MAX_CPUS_PER_NODE; j++) {
            cpu_node[i][j] = NO_CPU_DEFINED;
            cpu_node[i][j] = NO_CPU_DEFINED;
        }
    }
    for(i = 0; i < MAX_CHIPS; i++) {
        for(j = 0; j<MAX_CPUS_PER_CHIP;j++)  {
            cpu_chip[i][j] = NO_CPU_DEFINED ;
            cpu_chip[i][j] = NO_CPU_DEFINED ;
        }
    }

    /* extract Node/Chip level info from SYS_CONF_TYP structure for InterNode/IntraNode memory mapping */
    num_nodes = scfg->num_nodes ;
    for(node = 0; node < num_nodes; node++) {
        chips_per_node[node] = scfg->node[node].num_chips;
        for(chip = 0; chip < chips_per_node[node]; chip++) {
            scfg_chip[num_chips] = scfg->node[node].chip[chip].num_procs;
			scfg_sys[node][chip] = scfg->node[node].chip[chip].num_procs;
            for(cpu = 0; cpu < scfg_chip[num_chips]; cpu ++) {
                cpu_chip[num_chips][cpu] = scfg->node[node].chip[chip].lprocs[cpu];
                cpu_node[node][scfg_node[node]] = scfg->node[node].chip[chip].lprocs[cpu];
				cpu_sys[node][chip][cpu]	= scfg->node[node].chip[chip].lprocs[cpu];
                scfg_node[node]++;
            }
            num_chips++;
        }
    }
	if(query_pages ==0 && query_memconf == 0) { 
    	if(num_pages_available < fabbus_requested) {
        	/* System is falling short of 16M pages, We need to equally distribute these pages amongst nodes/chips */
        	unsigned long long memory_set_size = 0, pages_per_rad;
        	if(mem_alloc == 3 ) {
            	pages_per_rad = num_pages_available / num_nodes;
        	} else if(mem_alloc == 4) {
              	pages_per_rad = num_pages_available / num_chips;
        	} else {
              	pages_per_rad = MAX_CPUS;
        	}
        	memory_set_size = memory_per_thread(pvr);
			if(SET_PAGESIZE) { 
        		threads_per_node = ( pages_per_rad * 16 * M) / memory_set_size ;
			} else { 
				threads_per_node = ( pages_per_rad * 4 * K ) / memory_set_size ; 
			}
        	if(threads_per_node < 1) {
            	sprintf(msg," System is too short to create one thread per node/chip, num_pages_available=%#llx, pages_per_rad = %#x, memory_set_size = %#x, threads_per_node = %#x, SET_PAGESIZE=%#x this instance will exit...\n",
                                 num_pages_available, pages_per_rad, memory_set_size, threads_per_node, SET_PAGESIZE);
            	hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            	return(-1);
        	}
    	}
	}

	if((mem_alloc == 3) && (num_nodes < 2)) {
		sprintf(msg,"Only one CEC node detected num_nodes= %d, insufficient hardware to run inter node fabric bus test. Atleast two CEC nodes are needed on system \n",
						num_nodes);
        hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
		return(-1);
	}
    switch(mem_alloc) {

    	case 0 : { /* Local */
            for(node = 0; node < num_nodes; node++) {
                pjmp = scfg_node[node]/threads_per_node;
                if(pjmp == 0) pjmp = 1;
                for(cpu = 0; cpu < scfg_node[node]; cpu += pjmp) {
                    memory_mapping[cpus_mapped][HOST_CPU] = cpu_node[node][cpu];
                    memory_mapping[cpus_mapped][DEST_CPU] = cpu_node[node][cpu];
                    cpus_mapped++;
                }
            }
        }
        break;
        case 1 : { /* Remote */
            for(node = 0; node < num_nodes; node++) {
                int x, y;
                pjmp = scfg_node[node]/threads_per_node;
                if(pjmp == 0) pjmp = 1;
                x = (node + 1) % num_nodes;
                for(cpu = 0, y = 0; cpu < scfg_node[node]; cpu += pjmp, y += pjmp) {
                    if(cpu_node[x][y] == NO_CPU_DEFINED ) {
                    	if(pjmp > 1) {
                        	y = 1;
                        	cpu -= pjmp;
                        	continue;
                    	} else {
                        	break;
                    	}
                	}
                	memory_mapping[cpus_mapped][HOST_CPU] = cpu_node[node][cpu];
                	memory_mapping[cpus_mapped][DEST_CPU] = cpu_node[x][y];
                	cpus_mapped++;
            	}
        	}
        }
        break;
        case 2 : { /* Remote Spreaded */
            for(node = 0; node < num_nodes; node++) {
                int x, y = 0, exhausted = -1;
                int cnt_thrds = 0 ;
                pjmp = scfg_node[node]/threads_per_node;
                if(pjmp == 0) pjmp = 1;
                x = node;
                for(cpu = 0; cpu < scfg_node[node]; cpu += pjmp) {
                    cnt_thrds ++ ;
                    if ( cnt_thrds > threads_per_node )
                        break ;
                     x = (x + 1) % num_nodes;
                     if(x == node) {
                     	x = (x + 1) % num_nodes;
                     	y++;
                     }
                     if(y >= scfg_node[x]) {
                     	if(exhausted == -1) exhausted = x;
                     	else if (x == exhausted) break;
                       	cpu -= pjmp;
                       	continue;
                     } else {
                     	exhausted = -1;
                     }
                    memory_mapping[cpus_mapped][HOST_CPU] = cpu_node[node][cpu];
                    memory_mapping[cpus_mapped][DEST_CPU] = cpu_node[x][y];
                    cpus_mapped++;
              	}
            }
        }
        break;
        case 3: { /* Remote Spreaded  - all */
			for(node = 0; node < num_nodes; node++) {
				int remote_node, remote_cpu = 0,remote_chip=0;
				int thread_per_chip = threads_per_node/chips_per_node[node];
				for(chip = 0; chip<chips_per_node[node];chip++,remote_chip++){
					remote_cpu  = 0;
					remote_node = node;
					for(cpu=0;cpu < ((scfg_sys[node][chip] < thread_per_chip)? scfg_sys[node][chip]:thread_per_chip);cpu++){
						remote_node = (remote_node + 1) % num_nodes;
						if(remote_node == node) {
							remote_node = (remote_node + 1) % num_nodes;
							remote_cpu++;
						}			
						
						if(chip > chips_per_node[remote_node]){
							remote_chip = (chips_per_node[remote_node] - 1);	
						}
							
						if(remote_cpu >= thread_per_chip){
							continue;
						}
						if(remote_cpu >= scfg_sys[remote_node][remote_chip]){
							memory_mapping[cpus_mapped][HOST_CPU] = cpu_sys[node][chip][cpu];
							memory_mapping[cpus_mapped][DEST_CPU] = cpu_sys[remote_node][remote_chip][scfg_sys[remote_node][remote_chip]-1];
						}else {
							
							memory_mapping[cpus_mapped][HOST_CPU] = cpu_sys[node][chip][cpu];
							memory_mapping[cpus_mapped][DEST_CPU] = cpu_sys[remote_node][remote_chip][remote_cpu];
						}
						cpus_mapped++;
					}
				}
			}
        }
        break;
        case 4 : { /* Intra Node memory mapping */
            int chip_cnt = 0;
            for(node = 0; node < num_nodes; node++) {
            	int remote_chip, remote_cpu = 0, exhausted = -1;

            	if( chips_per_node[node] < 2 ) {
					chip_cnt += chips_per_node[node] ;
                	continue;
            	}
            	for(chip = 0; chip < chips_per_node[node]; chip++) {
                	remote_chip = chip;
					remote_cpu = 0;
                	for(cpu = 0 ; cpu < ((scfg_chip[chip_cnt + chip] < threads_per_node) ? scfg_chip[chip_cnt + chip] : threads_per_node) ; cpu++ ) {
        	        	remote_chip = (remote_chip + 1) % chips_per_node[node];
                   		if(remote_chip == chip) {
                        	remote_chip = (remote_chip + 1) % chips_per_node[node];
                        	remote_cpu++;
                    	}
                        if(remote_cpu >= threads_per_node) {
                            continue;
						}
                    	if( remote_cpu >= scfg_chip[remote_chip]) {
							memory_mapping[cpus_mapped][HOST_CPU] = cpu_chip[chip_cnt + chip][cpu];
							memory_mapping[cpus_mapped][DEST_CPU] = cpu_chip[chip_cnt + remote_chip][scfg_chip[remote_chip] - 1];
                    	} else {
                    		memory_mapping[cpus_mapped][HOST_CPU] = cpu_chip[chip_cnt + chip][cpu];
                    		memory_mapping[cpus_mapped][DEST_CPU] = cpu_chip[chip_cnt + remote_chip][remote_cpu];
						}
                    	cpus_mapped++;
                	}
            	}
            	chip_cnt += chips_per_node[node] ;
            }
        }
        break;
        default : {
        	sprintf(msg," Wrong rules file parm memory_allocation type =%d \n",mem_alloc);
            hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }
    } /* Off Switch */
    *num_cpus_mapped = cpus_mapped;
#ifdef DEBUGON
    sprintf(msg," num_cpus_mapped = %d \n",cpus_mapped);
    hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
    for(j = 0; j < cpus_mapped; j++) {
        sprintf(msg,"cpu - %d memory on - %d \n",memory_mapping[j][HOST_CPU], memory_mapping[j][DEST_CPU]);
        hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
    }
#endif
	return 0;
}


int
get_cont_phy_mem(unsigned int num_cpus_mapped, unsigned int memory_mapping[][2], long long size, MEMORY_SET mem[]) {

    char *p, *addr, msg[4096];
    int i=0, ii, rc, host_cpu , j;
    size_t shm_size;
    unsigned int cpu, bind_to_cpu;
    unsigned int memflg;

#ifndef __HTX_LINUX__ /* AIX Specific */

    psize_t psize = 16*M;
    struct shmid_ds shm_buf = { 0 };
	struct vminfo_psize vminfo_64k = { 0 }; 

    memflg = (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if(SET_PAGESIZE) {
        psize = 16*M;
    } else {
        psize = 4 * K;
    }
#else   /* Linux Specific */
    unsigned long long psize;

    memflg = (IPC_CREAT | IPC_EXCL |SHM_R | SHM_W);
    if(SET_PAGESIZE) {
        memflg |= (SHM_HUGETLB);
        psize = 16*M;
    } else {
        psize = 4 * K;
    }

    /* shmmax controls the maximum amount of memory to be allocated for shared memory */
    rc = system ("echo 268435456 > /proc/sys/kernel/shmmax");
    if (rc < 0){
        sprintf(msg,"unable to change the /proc/sys/kernel/shmmax variable\n");
        hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
    }

    /* The maximum amount of shared memory that can be allocated. */
    rc = system ("echo 268435456 > /proc/sys/kernel/shmall");
    if (rc < 0) {
        sprintf(msg,"unable to change the /proc/sys/kernel/shmall");
        hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);
    }
#endif


    DEBUGON("\n Need %llx physically contiguous memory \n", size);

    shm_size = (size < PG_SZ_16M ? PG_SZ_16M : size);

    for(cpu = 0; cpu < num_cpus_mapped; cpu ++) {

    	if(memory_mapping[cpu][DEST_CPU] != NO_CPU_DEFINED) {
            host_cpu = memory_mapping[cpu][HOST_CPU];
            bind_to_cpu = memory_mapping[cpu][DEST_CPU];
        } else {
            continue;
        }
#ifdef __HTX_LINUX__
	    rc = htx_bind_process(BIND_THE_PROCESS, bind_to_cpu);
#else
   		rc = bindprocessor(BINDPROCESS, getpid(), bind_to_cpu);
#endif
        if(rc == -1) {
            sprintf(msg, "\n bindprocessor failed with %d, binding th=%d, cpu=%d", errno, cpu,bind_to_cpu);
            hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }

        mem[host_cpu].shm_id = shmget(IPC_PRIVATE, shm_size, memflg);
        if(mem[host_cpu].shm_id == -1) {
            memory_set_cleanup();
            sprintf(msg,"shmget failed with %d !\n", errno);
            hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }

#ifndef __HTX_LINUX__
	if(SET_PAGESIZE) {  
        if ( shm_size > 256*M) {
            if ((rc = shmctl(mem[host_cpu].shm_id, SHM_GETLBA, &shm_buf)) == -1)   {
                memory_set_cleanup();
                sprintf(msg,"\n shmctl failed to get minimum alignment requirement - %d", errno);
                hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
                return(-1);
            }
        }
        shm_buf.shm_pagesize = psize;
        if( (rc = shmctl(mem[host_cpu].shm_id, SHM_PAGESIZE, &shm_buf)) == -1) {
            memory_set_cleanup();
            sprintf(msg,"\n shmctl failed with %d while setting page size.",errno);
            hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }
	}
#endif
        addr = (char *) shmat(mem[host_cpu].shm_id, 0, 0);
        if(-1 == (int)addr) {
            sprintf(msg,"\n shmat failed with %d",errno);
            hxfmsg(&htx_d, -1,HTX_HE_HARD_ERROR, msg);
            memory_set_cleanup();
            return(-1);
        }

        /* to be on safer side write touch the remote memory obtained */
        char * mem_addr = addr;
        for(i=0; i < (shm_size/psize); i++) {
            *(unsigned long long *)mem_addr = 0xFFFFffffFFFFffff ; 
            mem_addr += psize;
        }

        sprintf(msg, "%s: cpu %d, bound to cpu %d, seg_addr = 0x%llx of size = 0x%llx\n",
									__FUNCTION__, host_cpu, bind_to_cpu, (unsigned long long *)addr, shm_size);
        hxfmsg(&htx_d, 0, HTX_HE_INFO, msg);

		/* Lock the new shared memory, so that its always memory resident */ 
        rc = mlock(addr, shm_size);
        if(rc == -1) {
            sprintf(msg,"\n mlock failed with %d", errno);
            hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
            memory_set_cleanup();
            return(-1);
        }

        mem[host_cpu].seg_addr = addr;
		mem[host_cpu].memory_set_size = shm_size; 

#ifdef __HTX_LINUX__
	   	rc = htx_unbind_process();
#else
    	rc = bindprocessor(BINDPROCESS, getpid(), PROCESSOR_CLASS_ANY);
#endif
        if(rc == -1) {
            sprintf(msg, "\n%d: bindprocessor failed with %d while unbinding",__LINE__,  errno);
            hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }
    } /* Off num_cpus_mapped loop */
    return(0);
}

long int
get_random_number() {

    long int val_8, temp;

    temp = lrand48();
    val_8= temp << 32;
    temp = lrand48();
    val_8 = val_8 | temp;
    return val_8 ;
}

int
fill_random_buffer(unsigned long long size, unsigned long long ** seg_addr, MEMORY_SET * mem)  {


    size_t shm_size;
    char msg[4096];
    int shm_id, rc = 0;
    unsigned int memflg;
    unsigned long long *addr_8, *end_addr, *addr;
	unsigned long long i , j ;

#ifndef __HTX_LINUX__ /* AIX Specific */ 

   	psize_t psize = 16*M;
    struct shmid_ds shm_buf = { 0 };
	
	memflg = (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
	if(SET_PAGESIZE) { 
		memflg |= (SHM_LGPAGE | SHM_PIN);
    	psize = 16*M;
	} else { 
		psize = 4 * K; 
	} 
#else	/* Linux Specific */ 
	unsigned long long psize;

    memflg = (IPC_CREAT | IPC_EXCL |SHM_R | SHM_W);
	if(SET_PAGESIZE) { 
		memflg |= (SHM_HUGETLB);
		psize = 16*M;
	} else { 
		psize = 4 * K; 
	} 
#endif 

    shm_size = size;


    shm_id = shmget(IPC_PRIVATE, shm_size , memflg);
    if(shm_id == -1) {
          sprintf(msg,"shmget failed with %d !\n", errno);
          hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
          return(-1);
    }

#ifndef __HTX_LINUX__
	if( SET_PAGESIZE) {  
	    if ( shm_size > 256*M) {
    	  if ((rc = shmctl(shm_id, SHM_GETLBA, &shm_buf)) == -1)   {
        	    sprintf(msg,"\n shmctl failed to get minimum alignment requirement - %d", errno);
            	hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
            	return(-1);
       		}
    	}
    	shm_buf.shm_pagesize = psize;
    	if( (rc = shmctl(shm_id, SHM_PAGESIZE, &shm_buf)) == -1) {
         	sprintf(msg,"\n shmctl failed with %d while setting page size.",errno);
         	hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
         	return(-1);
    	}
	} 	
#endif
    addr = (unsigned long long *) shmat(shm_id, 0, 0);
    if(-1 == (int)addr) {
        sprintf(msg,"\n shmat failed with %d",errno);
        hxfmsg(&htx_d, -1,HTX_HE_HARD_ERROR, msg);
        memory_set_cleanup();
        return(-1);
    }
    addr_8 = (unsigned long long *)addr;
    end_addr = (unsigned long long *) ((char *)addr + shm_size);
	DEBUGON("%s: addr_8 = %llx , end_addr = %llx \n",__FUNCTION__, addr_8, end_addr);
    for(; addr_8 < end_addr; ) {
        *addr_8 = get_random_number();
        addr_8++;
    }

	/* Lock the new shared memory, so that its always memory resident */
	rc = mlock(addr, shm_size);
	if(rc == -1) {
		sprintf(msg,"\n mlock failed with %d", errno);
		hxfmsg(&htx_d, rc, HTX_HE_HARD_ERROR, msg);
		memory_set_cleanup();
		return(-1);
	}

	mem->seg_addr = (char *)addr ;
	mem->shm_id = shm_id ;
	mem->memory_set_size = size;
	*seg_addr = (unsigned long long *)addr ;
    return 0;
}

unsigned long long
memory_per_thread(unsigned int pvr) {

    char msg_buf[1024];

    /* General concept here is Fabricbus exerciser 
	 * needs to write to  2 * L3CACHE_SIZE to push the 
	 * data on fabric busses 
	 */
    if(pvr == PVR_POWER6) {
        return(2 * P6_L3CACHE_SIZE);
    } else if(pvr == PVR_POWER7) {
    	/* Due to P7 and above systems LCO affinity, L3 private cache for each core cache are shared,
     	 * to get a memory out of NODE,
     	 * effective L3 memory size = 2 * NUM_CORES_PER_CHIP * L3CACHE_SIZE
     	 */
        return(2 * P7_MAX_CORES_PER_CHIP * P7_L3CACHE_SIZE);
    } else if(pvr == PVR_POWER7PLUS) {
    	/* Its just a plus added to Power7, everything else remains same */
        return(2 * P7PLUS_MAX_CORES_PER_CHIP * P7PLUS_L3CACHE_SIZE) ;
    } else if(pvr == PVR_POWER8_MURANO) {
    	/* MURANO has 6 cores per chip */
        return(2 * P8_MURANO_MAX_CORES_PER_CHIP * P8_L3CACHE_SIZE);
    } else if(pvr == PVR_POWER8_VENICE || pvr == PVR_POWERP8P_GARRISION) {
        return(2 * P8_VENICE_MAX_CORES_PER_CHIP * P8_L3CACHE_SIZE);
    }else if(pvr == PVR_POWER9_NIMBUS){
        return(2 * P9_NIMBUS_MAX_CORES_PER_CHIP * P9_L3CACHE_SIZE);
    }else if(pvr == PVR_POWER9_CUMULUS){
        return(2 * P9_CUMULUS_MAX_CORES_PER_CHIP * P9_L3CACHE_SIZE);
    } else {
        sprintf(msg_buf, " %s, Illegal PVR = %#x \n", __FUNCTION__, pvr);
        hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg_buf);
        exit(1);
    }
}


