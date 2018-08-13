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
extern char page_size_name[MAX_PAGE_SIZES][8];
extern struct nest_global g_data;
extern struct mem_exer_info mem_g;
struct fabb_exer_info fab_g;
#if 0
int find_per_cpu_mem_requirement(){
    int rc = SUCCESS;
    int chip=0,cores=0;
    while(!g_data.sys_details.chip_mem_pool_data[chip].num_cpus){
        chip++;
        if(chip == g_data.sys_details.num_chip_mem_pools)break;
    }
    cores = (g_data.sys_details.chip_mem_pool_data[chip].num_cpus/g_data.sys_details.smt_threads);

    switch (g_data.sys_details.os_pvr) {

        case POWER8_MURANO:
        case POWER8_VENICE:
        case POWER8P_GARRISION:
        case POWER9_CUMULUS:
            fab_g.seg_size = ((g_data.sys_details.cinfo[L3].cache_size * 2 * cores)/g_data.sys_details.chip_mem_pool_data[chip].num_cpus);
            break;

        case POWER9_NIMBUS:
            fab_g.seg_size = ((g_data.sys_details.cinfo[L3].cache_size * 2 * (cores/2))/g_data.sys_details.chip_mem_pool_data[chip].num_cpus);
            break;
        
        default:
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Unknown pvr: ox%x  detatected, exiting.. \n",__LINE__,__FUNCTION__,g_data.sys_details.os_pvr);
            return FAILURE;
       
    }

    return rc;
}
#endif
int set_fabricb_exer_page_preferances(){
    int rc = SUCCESS,chip=0;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    int num_chips = g_data.sys_details.num_chip_mem_pools;

	g_data.gstanza.global_alloc_huge_page = 0;/*Overwritting rule parameter, disabling use of huge page*/
    for(chip=0;chip<num_chips;chip++){
        if(mem_details_per_chip[chip].memory_details.pdata[PAGE_INDEX_64K].supported){
            mem_details_per_chip[chip].memory_details.pdata[PAGE_INDEX_4K].supported = FALSE;
            g_data.sys_details.memory_details.pdata[PAGE_INDEX_4K].supported = FALSE;
        }

    }
    return rc;
}
int mark_destination_chips_inter_node_thread_level(int* start_chip_in_node,int div_factor,FILE* fp){
    int rc = SUCCESS,absolute_chip=0,n,c,cpu,core,k,count=0,host_cpu;
    int remote_node =0,remote_chip = 0,count_no_fab_nodes=0;
    struct sys_info* sysptr = &g_data.sys_details;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];

	for(n=0;n<sysptr->nodes;n++){
	/*printf("###node = %d\n",n);*/
		for(c=0,remote_chip=0;c<sysptr->node[n].num_chips;c++,remote_chip++){
			remote_node = (n+1)%sysptr->nodes;
			/*printf("\t**chip = %d\n",c);*/

			/*k indicates remote chip in remote  node*/
			/* For every cpu in a chip identify a memory chunk on remote same offset chip of remote node, fill fab_g.dest_chip structure.
			And eventually count number of segments per chip*/
			for(cpu=0,k=(start_chip_in_node[remote_node] + (remote_chip%sysptr->node[remote_node].num_chips));cpu<mem_details_per_chip[absolute_chip].num_cpus;cpu++){
				count = 0;
				host_cpu = mem_details_per_chip[absolute_chip].cpulist[cpu];
				while(mem_details_per_chip[k].memory_details.total_mem_avail <= 0){
					count++;
					k = (k+1)%(sysptr->node[remote_node].num_chips + start_chip_in_node[remote_node]);
					if(count == sysptr->node[remote_node].num_chips){
						displaym(HTX_HE_INFO,DBG_MUST_PRINT,"for cpu:%d, No memory found behind remote node:%d, num_chips=%d\n",host_cpu,n,count)
;
						remote_node++;
						break;
					}
				}
				if(remote_node == n){
					remote_node = (remote_node + 1)%sysptr->nodes;
					k = (sysptr->node[n].num_chips + start_chip_in_node[n] + (remote_chip % sysptr->node[remote_node].num_chips));
				}
				if ( remote_node >= sysptr->nodes){
					remote_node = 0;
					k = start_chip_in_node[remote_node] + (remote_chip%sysptr->node[remote_node].num_chips);
					if(remote_node == n){
						k = (sysptr->node[n].num_chips + start_chip_in_node[n] + (remote_chip % sysptr->node[remote_node].num_chips));
						remote_node = (remote_node + 1)%sysptr->nodes;
					}
				}
				k = (k%sysptr->num_chip_mem_pools);/*can be removed*/
				if(mem_details_per_chip[k].memory_details.total_mem_avail > 0){
					fab_g.dest_chip[host_cpu].seg_num  = fab_g.segs_per_chip[k]++;
					fab_g.dest_chip[host_cpu].chip_num = k;
					fprintf(fp,"%d(node:%d)\t%d\t\t%d (node:%d)\n-----------------------------------------------\n",absolute_chip,n,host_cpu,k,remote_node);
				}
                k = (k + sysptr->node[remote_node++].num_chips);
           	}
			/*capture total L3 size for every chip*/
			fab_g.fab_chip_L3_sz[absolute_chip] = ((sysptr->node[n].chip[c].num_cores/div_factor) * g_data.sys_details.cache_info[L3].cache_size);
			absolute_chip++;
		}
	}
	return rc;
}
int mark_destination_chips_inter_node_core_level(int* start_chip_in_node,int div_factor,FILE* fp){
    int rc = SUCCESS,absolute_chip=0,n,c,cpu,core,k,count=0,host_cpu;
    int remote_node =0,remote_chip = 0,count_no_fab_nodes=0;
    struct sys_info* sysptr = &g_data.sys_details;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];

	for(n=0;n<sysptr->nodes;n++){
		for(c=0,remote_chip=0;c<sysptr->node[n].num_chips;c++,remote_chip++){
			remote_node = (n+1)%sysptr->nodes;
			for(core=0,k=(start_chip_in_node[remote_node] + (remote_chip%sysptr->node[remote_node].num_chips));core<mem_details_per_chip[absolute_chip].num_cores;core++){
				if(fab_g.fab_cores[absolute_chip][core] == INTER_NODE){
					count = 0;
					while(mem_details_per_chip[k].memory_details.total_mem_avail <= 0){
						count++;
						k = (k+1)%(sysptr->node[remote_node].num_chips + start_chip_in_node[remote_node]);
						if(count == sysptr->node[remote_node].num_chips){
							displaym(HTX_HE_INFO,DBG_MUST_PRINT,"for cpu:%d, No memory found behind remote node:%d, num_chips=%d\n",host_cpu,n,count);
							remote_node++;
							break;
						}
					}
					if(remote_node == n){
						remote_node = (remote_node + 1)%sysptr->nodes;
						k = (sysptr->node[n].num_chips + start_chip_in_node[n] + (remote_chip % sysptr->node[remote_node].num_chips));
					}
					if ( remote_node >= sysptr->nodes){
						remote_node = 0;
						k = start_chip_in_node[remote_node] + (remote_chip%sysptr->node[remote_node].num_chips);
						if(remote_node == n){
							k = (sysptr->node[n].num_chips + start_chip_in_node[n] + (remote_chip % sysptr->node[remote_node].num_chips));
							remote_node = (remote_node + 1)%sysptr->nodes;
						}
					}
					k = (k%sysptr->num_chip_mem_pools);/*can be removed*/
					for(cpu=0;cpu<mem_details_per_chip[absolute_chip].core_array[core].num_procs;cpu++){
						host_cpu = mem_details_per_chip[absolute_chip].core_array[core].lprocs[cpu];
						fab_g.dest_chip[host_cpu].seg_num  = fab_g.segs_per_chip[k]++;
						fab_g.dest_chip[host_cpu].chip_num = k;
						fprintf(fp,"%d(node:%d)\t%d\t\t%d (node:%d)\n-----------------------------------------------\n",absolute_chip,n,host_cpu,k,remote_node);
					}
					k = (k + sysptr->node[remote_node++].num_chips);
				}
			}
			/*capture total L3 size for every chip*/
			fab_g.fab_chip_L3_sz[absolute_chip] = ((sysptr->node[n].chip[c].num_cores/div_factor) * g_data.sys_details.cache_info[L3].cache_size);
			absolute_chip++;
		}
	}
	return rc;
}
int mark_destination_chips_intra_node_thread_level(int* start_chip_in_node,int div_factor,FILE* fp){
    int rc = SUCCESS,absolute_chip=0,n,c,cpu,core,k,count=0,host_cpu;
    int remote_node =0,remote_chip = 0,count_no_fab_nodes=0;
    struct sys_info* sysptr = &g_data.sys_details;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];

	for(n=0;n<sysptr->nodes;n++){
		if(sysptr->node[n].num_chips <= 1){
			displaym(HTX_HE_INFO,DBG_MUST_PRINT,"No Fabricbus detected in node:%d(num chips=%d),Node must have atleast 2 chips in it,"
			  "  continuing to next node\n",n,sysptr->node[n].num_chips);
			count_no_fab_nodes++;
			continue;
		}
		for(c=0;c<sysptr->node[n].num_chips;c++){
			/*k indicates remote chip within a node*/
			/* For every cpu in a chip identify a memory chunk on remote chip of same node, fill fab_g.dest_chip structure.
				And eventually count number of segments per chip*/
			for(cpu=0,k=absolute_chip+1;cpu<mem_details_per_chip[absolute_chip].num_cpus;cpu++,k++){
				host_cpu = mem_details_per_chip[absolute_chip].cpulist[cpu];
				if(k == absolute_chip)k++;
				if(k >= start_chip_in_node[n] + sysptr->node[n].num_chips ){
					k = start_chip_in_node[n];
					if(k == absolute_chip)k++;
				}
				count = 0;
				/* skip memory less chips,loop untill chip with memory behind it is identified*/
				while(mem_details_per_chip[k].memory_details.total_mem_avail <= 0){
					count++;
					k = (k+1)%(sysptr->node[n].num_chips + start_chip_in_node[n]);
					if(k == absolute_chip){
						count++;
					}
					if(count == sysptr->node[n].num_chips) {
						displaym(HTX_HE_INFO,DBG_MUST_PRINT,"for cpu:%d,No memory found behind node:%d, num_chips=%d\n",host_cpu,n,count);
							break;
					}
				}
				if((mem_details_per_chip[k].memory_details.total_mem_avail > 0) && (k != absolute_chip)){
					fab_g.dest_chip[host_cpu].seg_num  = fab_g.segs_per_chip[k]++;
					fab_g.dest_chip[host_cpu].chip_num = k;
					fprintf(fp,"%d (node:%d)\t%d\t\t%d (node:%d)\n-------------------------------------------\n",absolute_chip,n,host_cpu,k,n);
				}
			}
			/*capture total L3 size for every chip*/
			fab_g.fab_chip_L3_sz[absolute_chip] = ((sysptr->node[n].chip[c].num_cores/div_factor) * g_data.sys_details.cache_info[L3].cache_size);
			absolute_chip++;
		}
	}
	if(count_no_fab_nodes == sysptr->nodes){
		print_partition_config(HTX_HE_SOFT_ERROR);
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:No fabricbus detected within any node of total nodes:%d, "
			"There must be atleast 2 chips enabled for a node, exiting..\n",
			__LINE__,__FUNCTION__,sysptr->nodes);
		return FAILURE;
	}
	return rc;
}
int mark_destination_chips_intra_node_core_level(int* start_chip_in_node,int div_factor,FILE* fp){
	int rc = SUCCESS,absolute_chip=0,n,c,cpu,core,k,count=0,host_cpu;
	int remote_node =0,remote_chip = 0,count_no_fab_nodes=0;
	struct sys_info* sysptr = &g_data.sys_details;
	struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];

	for(n=0;n<sysptr->nodes;n++){
		if(sysptr->node[n].num_chips <= 1){
			displaym(HTX_HE_INFO,DBG_MUST_PRINT,"No Fabricbus detected in node:%d(num chips=%d),Node must have atleast 2 chips in it,"
				"  continuing to next node\n",n,sysptr->node[n].num_chips);
			count_no_fab_nodes++;
			continue;
		}
		for(c=0;c<sysptr->node[n].num_chips;c++){
			/*k indicates remote chip within a node*/
			/* For every cpu in a chip identify a memory chunk on remote chip of same node, fill fab_g.dest_chip structure.
				And eventually count number of segments per chip*/
			for(core=0,k=absolute_chip+1;core<mem_details_per_chip[absolute_chip].num_cores;core++){
				if(fab_g.fab_cores[absolute_chip][core] == INTRA_NODE){
					if(k == absolute_chip)k++;
					if(k >= start_chip_in_node[n] + sysptr->node[n].num_chips ){
						k = start_chip_in_node[n];
						if(k == absolute_chip)k++;
					}
					count = 0;
					/* skip memory less chips,loop untill chip with memory behind it is identified*/
					while(mem_details_per_chip[k].memory_details.total_mem_avail <= 0){
						count++;
						k = (k+1)%(sysptr->node[n].num_chips + start_chip_in_node[n]);
						if(k == absolute_chip){
							count++;
						}
						if(count == sysptr->node[n].num_chips) {
							displaym(HTX_HE_INFO,DBG_MUST_PRINT,"for cpu:%d,No memory found behind node:%d, num_chips=%d\n",host_cpu,n,count);
							break;
						}
					}
					if((mem_details_per_chip[k].memory_details.total_mem_avail > 0) && (k != absolute_chip)){
						for(cpu=0;cpu<mem_details_per_chip[absolute_chip].core_array[core].num_procs;cpu++){
								host_cpu = mem_details_per_chip[absolute_chip].core_array[core].lprocs[cpu];
								fab_g.dest_chip[host_cpu].seg_num  = fab_g.segs_per_chip[k]++;
								fab_g.dest_chip[host_cpu].chip_num = k;
								fprintf(fp,"%d (node:%d)\t%d\t\t%d (node:%d)\n-------------------------------------------\n",absolute_chip,n,host_cpu,k,n);
							}
						}
					k++;
				}
			}
		   	/*capture total L3 size for every chip*/
			fab_g.fab_chip_L3_sz[absolute_chip] = ((sysptr->node[n].chip[c].num_cores/div_factor) * g_data.sys_details.cache_info[L3].cache_size);
			absolute_chip++;
		}
	}
	if(count_no_fab_nodes == sysptr->nodes){
		print_partition_config(HTX_HE_SOFT_ERROR);
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:No fabricbus detected within any node of total nodes:%d, "
			"There must be atleast 2 chips enabled for a node, exiting..\n",
			__LINE__,__FUNCTION__,sysptr->nodes);
		return FAILURE;
	}
	return rc;
}
int mark_cores_to_drive_links(int oper){
	int rc = SUCCESS,type,ch,c;
	struct sys_info* sysptr = &g_data.sys_details;
	struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];

    switch(oper) {
        case INTRA_NODE: 
			for(ch=0;ch<g_data.sys_details.num_chip_mem_pools;ch++){
				for(c=0;c<mem_details_per_chip[ch].num_cores;c++){
					fab_g.fab_cores[ch][c]=INTRA_NODE;
				}
			}
			break;

		case INTER_NODE:
			for(ch=0;ch<g_data.sys_details.num_chip_mem_pools;ch++){
				for(c=0;c<mem_details_per_chip[ch].num_cores;c++){
					fab_g.fab_cores[ch][c]=INTER_NODE;
				}
			}
			break;

		case ALL_FABRIC:
			type = INTRA_NODE;
			for(ch=0;ch<g_data.sys_details.num_chip_mem_pools;ch++){
				for(c=0;c<mem_details_per_chip[ch].num_cores;c++){
					type = (c % 2) == 0 ? INTRA_NODE : INTER_NODE;
					fab_g.fab_cores[ch][c] = type;
				}
			}
			break;

		default:
			break;		
	}	

	return rc;
}
int memory_segments_calculation(){
    int rc = SUCCESS,oper = -1,absolute_chip=0,n,c,cpu,core,k,div_factor = 1;
    int start_chip_in_node[MAX_NODES] = {0};
    int prev_node_chips = 0,count=0,host_cpu;
    int remote_node =0,remote_chip = 0,count_no_fab_nodes=0;
    struct sys_info* sysptr = &g_data.sys_details;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
	char dump_file[100];
	FILE *fp;

	for(int i=0;i<MAX_CHIPS;i++){
		for(int j=0;j<MAX_CORES_PER_CHIP;j++){
			fab_g.fab_cores[i][j] = -1;
		}
	}
	/*DD1_NORMAL_CORE is 1 */
    if((DD1_NORMAL_CORE == get_p9_core_type())&& (((g_data.sys_details.true_pvr == POWER9_NIMBUS)) || (g_data.sys_details.true_pvr == POWER9_CUMULUS))){
        div_factor = 2;
    }
    if((strncmp ((char*)(DEVICE_NAME+5),"fabn",4)) == 0){
        oper = INTER_NODE;
    }
    else if((strncmp ((char*)(DEVICE_NAME+5),"fabc",4)== 0)){
        oper = INTRA_NODE;
    }     
    else if((strncmp ((char*)(DEVICE_NAME+5),"fabxo",5)== 0)){
        oper = ALL_FABRIC;
    }     
	rc = mark_cores_to_drive_links(oper);
	if(rc != SUCCESS){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:mark_cores_to_drive_links() failied with rc=%d\n",rc);
		return rc;
	}
	sprintf(dump_file, "%s/%s_cpu_to_mem_map",g_data.htx_d.htx_exer_log_dir,(char*)(DEVICE_NAME+5));
	fp = fopen(dump_file, "w");
	if ( fp == NULL) {
		displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s():Error opening %s file,errno:%d(%s),continuing without dumping cpu->mem maping info\n",
			__LINE__,__FUNCTION__,dump_file,errno,strerror(errno));
	}else{
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"cpu to memory chip mapping information is logged into file:%s/%s_cpu_to_mem_map\n",g_data.htx_d.htx_exer_log_dir,(char*)(DEVICE_NAME+5));
    }

	fprintf(fp, "NOTE: 1)Chip numbers are contiguous at system reference level.\n2)cpu to memory chip mapping may not be valid if user disables cpus/mem from rule file filter option\n");
	fprintf(fp,"***********************************************************************************************************\n");

	fprintf(fp,"CHIP\t\tCPU\tDESTINATION_MEM_CHIP\n===============================================================\n");
#if 0
    rc = find_per_cpu_mem_requirement();
    if( rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:find_per_cpu_mem_requirement() faiiled with rc =%d\n",
             __LINE__,__FUNCTION__,rc);
        return rc;
    }
#endif
        
    for(n=0;n<sysptr->nodes;n++){
        start_chip_in_node[n]=prev_node_chips;
        prev_node_chips  += sysptr->node[n].num_chips;
    }
    for(int i=0;i<MAX_CHIPS;i++){
        fab_g.fab_chip_L3_sz[i] = -1;
    }                
    for(int i=0;i<MAX_CPUS;i++){
        fab_g.dest_chip[i].chip_num = -1;
		fab_g.dest_chip[i].seg_num  = -1;
    }
	/*Identify and mark remote memory chunk for every cpu based inter_node and intra_node test case*/
    switch(oper) {
        case INTRA_NODE:
			if(g_data.gstanza.global_fab_links_drive_type == CORES_FAB){
				displaym(HTX_HE_INFO,DBG_MUST_PRINT,"core boundaries will be used to drive  links within nodes\n");
				rc = mark_destination_chips_intra_node_core_level(start_chip_in_node,div_factor,fp);
				if(rc != SUCCESS){
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:mark_destination_chips_intra_node_core_level() failed with rc =%d\n",
						__LINE__,__FUNCTION__,rc);
					return rc;
				}
			}else if(g_data.gstanza.global_fab_links_drive_type == THREADS_FAB){
				rc = mark_destination_chips_intra_node_thread_level(start_chip_in_node,div_factor,fp);
				if(rc != SUCCESS){
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:mark_destination_chips_intra_node_thread_level() failed with rc =%d\n",
						__LINE__,__FUNCTION__,rc);
					return rc;
				}
            }else {
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:wrong value of g_data.global_fab_links_drive_type = %d, please check rule file\n",
					g_data.gstanza.global_fab_links_drive_type);
				return (FAILURE);
            }
            break;

        case INTER_NODE:
			if(sysptr->nodes <= 1){
                print_partition_config(HTX_HE_SOFT_ERROR);
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:No fabricbus detected between nodes, total nodes found to be = %d, "
                    "There must be atleast 2 nodes in a partition, exiting..\n",
					__LINE__,__FUNCTION__,sysptr->nodes);
				return FAILURE;
			}
			if(g_data.gstanza.global_fab_links_drive_type == CORES_FAB){
				displaym(HTX_HE_INFO,DBG_MUST_PRINT,"core boundaries will be used to drive  links between nodes\n");
				rc = mark_destination_chips_inter_node_core_level(start_chip_in_node,div_factor,fp);
				if(rc != SUCCESS){
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:mark_destination_chips_intara_node_core_level() failed with rc =%d\n",
						__LINE__,__FUNCTION__,rc);
					return rc;
				}
			}else if(g_data.gstanza.global_fab_links_drive_type == THREADS_FAB){
				rc = mark_destination_chips_inter_node_thread_level(start_chip_in_node,div_factor,fp);
				if(rc != SUCCESS){
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:mark_destination_chips_intara_node_core_level() failed with rc =%d\n",
						__LINE__,__FUNCTION__,rc);
					return rc;
				}
			}else {
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:wrong value of g_data.global_fab_links_drive_type = %d, please check rule file\n",
					g_data.gstanza.global_fab_links_drive_type);
				return (FAILURE);
			}
			/*for(int i=0;i<sysptr->num_chip_mem_pools;i++){
				printf("\t L3 chip size = %ld, aboslute chip:%d, segs=%d\n",fab_g.fab_chip_L3_sz[i],i,fab_g.segs_per_chip[i]);
			}*/
            break;

		case ALL_FABRIC:
			
			rc = mark_destination_chips_intra_node_core_level(start_chip_in_node,div_factor,fp);
			if(rc != SUCCESS){
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:mark_destination_chips_intra_node_core_level() failed with rc =%d\n",
					__LINE__,__FUNCTION__,rc);
				return rc;
			}
			rc = mark_destination_chips_inter_node_core_level(start_chip_in_node,div_factor,fp);
			if(rc != SUCCESS){
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:mark_destination_chips_intara_node_core_level() failed with rc =%d\n",
					__LINE__,__FUNCTION__,rc);
				return rc;
			}
			break;

        default: displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Unknown operation in fabricbus exer,  %d\n",
            __LINE__,__FUNCTION__,oper);        
	}
	fclose(fp);
    return rc;
		
}


int fill_fabb_segment_details(int chip){
    int rc = SUCCESS,pi,num_segs=0,seg=0;        
    unsigned long seg_size;
    struct page_wise_seg_info *seg_details;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    
    seg_size = (g_data.sys_details.cache_info[L3].cache_size * 2);
    num_segs = fab_g.segs_per_chip[chip];
    for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
        if((mem_details_per_chip[chip].memory_details.pdata[pi].supported == TRUE) && (mem_details_per_chip[chip].memory_details.pdata[pi].free != 0)) {
            seg_size = (seg_size/mem_details_per_chip[chip].memory_details.pdata[pi].psize);
            seg_size = (seg_size*mem_details_per_chip[chip].memory_details.pdata[pi].psize);
            /* Allocate malloc memory to hold segment management details for every page pool per chip*/
            mem_details_per_chip[chip].memory_details.pdata[pi].page_wise_seg_data = (struct page_wise_seg_info*)malloc(num_segs * sizeof(struct page_wise_seg_info));
            if(mem_details_per_chip[chip].memory_details.pdata[pi].page_wise_seg_data == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno:%d(%s)for size=%llu\n",
                    __LINE__,__FUNCTION__,errno,strerror(errno),num_segs * sizeof(struct page_wise_seg_info));
                return(FAILURE);
            }
            mem_details_per_chip[chip].memory_details.pdata[pi].num_of_segments =  num_segs;
            seg_details = mem_details_per_chip[chip].memory_details.pdata[pi].page_wise_seg_data;
            for(seg=0;seg<num_segs; seg++){
                seg_details[seg].shm_size = seg_size;
                seg_details[seg].original_shm_size = seg_size;
                seg_details[seg].seg_num   =   seg;
                seg_details[seg].shm_mem_ptr = NULL;
                seg_details[seg].shmid      = -1;
                displaym(HTX_HE_INFO,DBG_DEBUG_PRINT," chip:%dseg_num=%d:shm sizes[%u] = %lu (%s pages:%lu)\n",
                    chip,seg_details[seg].seg_num,seg,seg_details[seg].shm_size,page_size_name[pi],
                    (seg_details[seg].shm_size/mem_details_per_chip[chip].memory_details.pdata[pi].psize));
            }
        }
        mem_g.total_segments += mem_details_per_chip[chip].memory_details.pdata[pi].num_of_segments;
    }
    return rc;
}
int modify_fabb_shmsize_based_on_cpufilter(int chip,int pi){
    struct chip_mem_pool_info* chp = &g_data.sys_details.chip_mem_pool_data[0];
    int rc = SUCCESS,seg = 0;
	displaym(HTX_HE_INFO,DBG_MUST_PRINT," tot L3 size of chip : %d = %lu\n",chip,fab_g.fab_chip_L3_sz[chip]);
    for(int cpu =0;cpu < chp[chip].in_use_num_cpus; cpu++){
		int found_dest_chip  = 0;
		if((fab_g.dest_chip[chp[chip].in_use_cpulist[cpu]].chip_num  == -1) || (fab_g.dest_chip[chp[chip].in_use_cpulist[cpu]].seg_num == -1)){
			continue;
		}
        for(int dest_chip =0;dest_chip < g_data.sys_details.num_chip_mem_pools;dest_chip++){

            if (fab_g.dest_chip[chp[chip].in_use_cpulist[cpu]].chip_num  == dest_chip){

				if(!chp[dest_chip].is_chip_mem_in_use){
                    continue;
                }
				seg = fab_g.dest_chip[chp[chip].in_use_cpulist[cpu]].seg_num;
				found_dest_chip = 1;
				
                chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size = chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].original_shm_size;

                chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size = 
                ((( fab_g.fab_chip_L3_sz[chip] * 2) / chp[chip].in_use_num_cpus) <= chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].original_shm_size) ?
                ((fab_g.fab_chip_L3_sz[chip] * 2) / chp[chip].in_use_num_cpus) : chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].original_shm_size;
				debug(HTX_HE_INFO,DBG_MUST_PRINT,"cpu:%d dest_chip=%d, segid=%lu seg_size(%d)=%lu\n",chp[chip].in_use_cpulist[cpu],
				dest_chip,chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].shmid,seg,chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size);
                

                /*page align*/
                chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size =
                (chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size * chp[dest_chip].memory_details.pdata[pi].psize);

                chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size =
                (chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size / chp[dest_chip].memory_details.pdata[pi].psize);

                chp[dest_chip].memory_details.pdata[pi].in_use_num_of_segments++;
                chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].in_use = 0;
				/*page_wise_usage_mem contains memory specfied in filter, the value will be used in fabricbus test case only for logging purpose*/
				chp[dest_chip].memory_details.pdata[pi].page_wise_usage_mem += chp[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size;
			}
        }
		if(!found_dest_chip){
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: for cpu:%d(chip:%d) there is no destination memory chip found,\nPlease run with default mem_filer value, "
				"memory mapping is dumped in %s/%s_cpu_to_mem_map\n",__LINE__,__FUNCTION__,chp[chip].in_use_cpulist[cpu],
				chip,g_data.htx_d.htx_exer_log_dir,(char*)(DEVICE_NAME+5));
				return(FAILURE);
		}
    }
    return rc;
}

int fill_fabb_thread_structure(struct chip_mem_pool_info *cpu_chip,int start_thread_index){
    int thread_num,pi,seg=0,seg_count_track[MAX_PAGE_SIZES];
    int num_threads_in_this_chip = cpu_chip->in_use_num_cpus;
    struct thread_data* th = &g_data.thread_ptr[start_thread_index];
    struct chip_mem_pool_info *mem_chip = &g_data.sys_details.chip_mem_pool_data[0];

    for(pi=0;pi<MAX_PAGE_SIZES;pi++){
        seg_count_track[pi] = 0;
    }

    for(thread_num=0;thread_num < num_threads_in_this_chip; thread_num++){
        struct mem_exer_thread_info *local_ptr = &(mem_g.mem_th_data[thread_num+start_thread_index]);
        th[thread_num].testcase_thread_details = (void *)local_ptr;
        th[thread_num].thread_num               = thread_num+start_thread_index;
        th[thread_num].bind_proc                = cpu_chip->in_use_cpulist[thread_num];
        th[thread_num].thread_type              = FABRICB;
        local_ptr->num_segs = 0;
		if((fab_g.dest_chip[th[thread_num].bind_proc].chip_num  == -1) || (fab_g.dest_chip[th[thread_num].bind_proc].seg_num == -1)){
			continue;
		}

        local_ptr->num_segs = 1; /* Only one segment is accessed by a thread in fabricbus exerciser*/
        if(local_ptr->num_segs != 0){
            local_ptr->seg_details = (struct page_wise_seg_info*) malloc(local_ptr->num_segs * sizeof(struct page_wise_seg_info));
            if(local_ptr->seg_details == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed for memory %d bytes with errno(%d)%s, num_segs for thread %d = %d\n",
                    __LINE__,__FUNCTION__,(local_ptr->num_segs * sizeof(struct page_wise_seg_info)),errno,strerror(errno),th[thread_num].thread_num,local_ptr->num_segs);
                return(FAILURE);
            }
        }

    
        for(int dest_chip =0;dest_chip< g_data.sys_details.num_chip_mem_pools;dest_chip++){
            if (fab_g.dest_chip[th[thread_num].bind_proc].chip_num == dest_chip){
                for(pi=0;pi<MAX_PAGE_SIZES;pi++){
                    if(mem_chip[dest_chip].memory_details.pdata[pi].in_use_num_of_segments > 0) {
						seg_count_track[pi] = fab_g.dest_chip[th[thread_num].bind_proc].seg_num;
                        for(seg=0;seg < local_ptr->num_segs; seg++){
                            /*only 1 segment per thread*/
                            if(mem_chip[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg_count_track[pi]].owning_thread != -1){
                                continue;
                            }
                            local_ptr->seg_details[seg].owning_thread     = th[thread_num].thread_num;
                            local_ptr->seg_details[seg].shm_size          = mem_chip[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg_count_track[pi]].shm_size;  
                            local_ptr->seg_details[seg].page_size_index   = pi;
                            local_ptr->seg_details[seg].shmid             = mem_chip[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg_count_track[pi]].shmid;
                            local_ptr->seg_details[seg].shm_mem_ptr       = mem_chip[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg_count_track[pi]].shm_mem_ptr;
                            local_ptr->seg_details[seg].seg_num           = mem_chip[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg_count_track[pi]].seg_num;
                            local_ptr->seg_details[seg].mem_chip_num      = mem_chip[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg_count_track[pi]].mem_chip_num;
                            local_ptr->seg_details[seg].cpu_chip_num      = cpu_chip->chip_id;

                            /*update thread number in global segment details data structure*/
                            mem_chip[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg_count_track[pi]].owning_thread =th[thread_num].thread_num;
                            mem_chip[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg_count_track[pi]].cpu_chip_num = cpu_chip->chip_id;
                            mem_chip[dest_chip].memory_details.pdata[pi].page_wise_seg_data[seg_count_track[pi]].in_use=1;
                            displaym(HTX_HE_INFO,DBG_IMP_PRINT,"Thread:%d, seg:%d cpu_chip:%d mem_chip:%d shmid:%lu shm_size:%lu\n",th[thread_num].thread_num,
								local_ptr->seg_details[seg].seg_num,local_ptr->seg_details[seg].cpu_chip_num,local_ptr->seg_details[seg].mem_chip_num,
								local_ptr->seg_details[seg].shmid,local_ptr->seg_details[seg].shm_size);
                        }
                        /*seg_count_track[pi]++;*/
					}
                }  
            }
        }
    }
    
    return (SUCCESS);
}

