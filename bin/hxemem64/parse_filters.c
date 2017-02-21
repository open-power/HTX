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
/*#define debug displaym*/
extern char page_size_name[MAX_PAGE_SIZES][8];
extern struct rule_info r;
extern struct nest_global g_data;
int parse_mem_filter(char filter_ptr[][MAX_POSSIBLE_ENTRIES]){
    int i,j,filt_idx,num_nodes = 0,num_chips = 0;
    unsigned long node_num,chip_num,chips_in_this_node,nodes_in_this_sys;
    char* chr_ptr[r.num_mem_filters][16],*end_ptr;
    char *ptr[16],*tmp,*tmp_str=NULL,c[8],tmp_filter_str[MAX_POSSIBLE_ENTRIES];
    struct sys_info* sysptr = &g_data.sys_details;
    char msg[4096],msg_temp[2048];


    for(int node = 0; node < MAX_NODES;node++ ){
        for(int chip = 0; chip < MAX_CHIPS_PER_NODE;chip++){
            for(int core = 0; core < MAX_CORES_PER_CHIP;core++){
                for(int lcpu=0;lcpu < MAX_CPUS_PER_CORE;lcpu++){
                   r.filter.mf.node[node].chip[chip].core[core].lprocs[lcpu]=-1;
                   r.filter.mf.node[node].chip[chip].core[core].num_procs = 0;
                }
                r.filter.mf.node[node].chip[chip].core[core].core_num=-1;
            }
            r.filter.mf.node[node].chip[chip].num_cores=0;
            r.filter.mf.node[node].chip[chip].chip_num =-1;
            for(int p=0;p<MAX_PAGE_SIZES;p++){
                r.filter.mf.node[node].chip[chip].mem_details.pdata[p].page_wise_usage_mem=0;
                r.filter.mf.node[node].chip[chip].mem_details.pdata[p].page_wise_usage_mpercent=0;
                r.filter.mf.node[node].chip[chip].mem_details.pdata[p].page_wise_usage_npages=0;
            }
        }
        r.filter.mf.node[node].num_chips=0;
        r.filter.mf.node[node].node_num=-1;
    }r.filter.mf.num_nodes =0;

	if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
    	sprintf(msg,"\n====================================================\n"
        	"Memory filter\n====================================================\n");
	}
    for(filt_idx = 0; filt_idx < r.num_mem_filters; filt_idx++){
	
		if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
        	sprintf(msg_temp,"mem filter[%d]=%s\n",filt_idx,filter_ptr[filt_idx]);
        	strcat(msg,msg_temp);
		}
        i = 0,j=0,node_num=0,num_nodes=0;
        strcpy(tmp_filter_str,filter_ptr[filt_idx]);
        if((strchr(tmp_filter_str,'N') == NULL)||(strchr(tmp_filter_str,'P')== NULL)){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
            "for mem_filer:%s,filter num:%d,cpu filter must conatin both 'N' & 'P'\n",
            __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx);
            return FAILURE;
        }

        if ((chr_ptr[filt_idx][i] = strtok(tmp_filter_str, "NP")) != NULL) {
            i++;
            while ((chr_ptr[filt_idx][i] = strtok(NULL, "NP")) != NULL) {
                i++;
            }

        }else {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s for mem filter :%s,"
                    "filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx);
                return (FAILURE);
        }
        nodes_in_this_sys = get_num_of_nodes_in_sys();
        if (chr_ptr[filt_idx][0][0] == '*'){
            num_nodes = nodes_in_this_sys;
            r.filter.mf.num_nodes = nodes_in_this_sys;
            for(int n=0;n<num_nodes;n++){
                if(r.filter.mf.node[n].node_num == -1){
                     r.filter.mf.node[n].node_num=n;
                }
            }
        }
        else if (strchr(chr_ptr[filt_idx][0],'[') != NULL) { /* indicates a range of nodes is given in filter */
            if((tmp_str = (char*)malloc(strlen(chr_ptr[filt_idx][0])+2)) == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno %d(%s) for size = %d\n"
                        ,__LINE__,__FUNCTION__,errno,strerror(errno),(strlen(chr_ptr[filt_idx][0])+2));
                return FAILURE;
            }
            strcpy(tmp_str,chr_ptr[filt_idx][0]);
            if((ptr[j++] = strtok(tmp_str,"[],")) == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s for mem filter :%s,"
                    "filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx);
                return (FAILURE);
            }

            while ((ptr[j]=strtok(NULL,",]")) != NULL ){
                j++;
            }
            errno=0;
            for(int k = 0;k<j;k++){
                if ((tmp = strchr(ptr[k],'%')) != NULL) { /* indicates a percentage of nodes is given in filter */
                    num_nodes = (nodes_in_this_sys * (float)((atoi(tmp+1))/100.0));
                    if((num_nodes< 0) ||  (num_nodes> nodes_in_this_sys)){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule  %s, rule file:%s for mem_filer:%s,"
                            "filter num:%d, node percentage as %d%(num_nodes=%d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                             filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_nodes);
                        return (FAILURE);
                    }
                    if(num_nodes== 0)num_nodes= 1;
                    for(int n =0;n<num_nodes;n++){
                        if(r.filter.mf.node[n].node_num == -1){
                             r.filter.mf.node[n].node_num = n;
                        }
                    }
                }else if (strchr(ptr[k],'-') == NULL){
                    node_num = strtol(ptr[k],&end_ptr,10);
                    if(node_num >= nodes_in_this_sys ||  errno != 0 || end_ptr == ptr[k]){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s  for mem_filer:%s,"
                            "filter num:%d, node filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            filter_ptr[filt_idx], filt_idx,node_num,nodes_in_this_sys);
                        return (FAILURE);
                    }
                    num_nodes++;
                    if(r.filter.mf.node[node_num].node_num == -1){
                        r.filter.mf.node[node_num].node_num = node_num;
                    }
                }

                else {
                    strcpy(c,ptr[k]);
                    if((tmp=strtok(c,"-")) == NULL){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule  rule %s, rule file:%s " 
                            "for mem_filer:%s,filter num:%d\n",__LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            filter_ptr[filt_idx],filt_idx);
                        return (FAILURE);

                    }
                    if((tmp=strtok(NULL,"-")) == NULL){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                           "for mem_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            filter_ptr[filt_idx],filt_idx);
                        return (FAILURE);

                    }

                    errno = 0;
                    node_num = strtol(tmp,&end_ptr,10);
                    if(r.filter.mf.node[node_num].node_num == -1){
                        r.filter.mf.node[node_num].node_num = node_num;
                    }
                    if(node_num >= nodes_in_this_sys ||  errno != 0 || end_ptr == ptr[k]){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            "for mem_filer:%s,filter num:%d, node filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            filter_ptr[filt_idx],filt_idx,node_num,nodes_in_this_sys);
                         return (FAILURE);
                    }
                    errno = 0;
                    for(int n=strtol(&ptr[k][0],&end_ptr,10);n <= node_num;n++){
                        if(n >= nodes_in_this_sys || errno != 0 || end_ptr == &ptr[k][0]){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for mem_filer:%s,filter num:%d, node filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                                filter_ptr[filt_idx],filt_idx,node_num,nodes_in_this_sys);
                            return (FAILURE);
                        }
                        if(r.filter.mf.node[n].node_num == -1){
                        r.filter.mf.node[n].node_num = n;
                    }
                    num_nodes++;
                }
            }
        }
        free(tmp_str);
        }else if ((tmp = strchr(chr_ptr[filt_idx][0],'%')) != NULL) { /* indicates a percentage of nodes is given in filter */
            num_nodes = (nodes_in_this_sys * (float)((atoi(tmp+1))/100.0));
            if((num_nodes < 0) ||  (num_nodes > nodes_in_this_sys)){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                    "for mem_filer:%s,filter num:%d, node percentage as %d%(num_nodes=%d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                        filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_nodes);
                     return (FAILURE);
            }
            if(num_nodes == 0)num_nodes=1;
            for(int n=0;(n<num_nodes);n++){
                if(r.filter.mf.node[n].node_num == -1)
                r.filter.mf.node[n].node_num=n;
            }
        }else {
            errno = 0;
            node_num = strtol(chr_ptr[filt_idx][0],&end_ptr,10);
            if(node_num >= nodes_in_this_sys ||  errno != 0 || end_ptr == chr_ptr[filt_idx][0]){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                    "for mem_filter:%s,filter num:%d, node filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                    filter_ptr[filt_idx],filt_idx,node_num,nodes_in_this_sys);
                return (FAILURE);
            }
            num_nodes = 1;
            if(r.filter.mf.node[node_num].node_num == -1){
                r.filter.mf.node[node_num].node_num = node_num;
            }
        }

		if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
        	sprintf(msg_temp,"\nnodes=%d:\t",num_nodes);
        	strcat(msg,msg_temp);
        	for(int k=0;k<MAX_NODES;k++){
            	if(r.filter.mf.node[k].node_num == -1) continue;
            	sprintf(msg_temp," %d \t",r.filter.mf.node[k].node_num);
            	strcat(msg,msg_temp);
        	}	
		}
        struct chip_info *chip_ptr = NULL;
        r.filter.mf.num_nodes = num_nodes;

        for (int n = 0; n < MAX_NODES; n++) {
			int temp_chips[MAX_CHIPS_PER_NODE]={[0 ... (MAX_CHIPS_PER_NODE- 1)] = -1};
            num_chips = 0;chip_num=0;
            chips_in_this_node= get_num_of_chips_in_node(n);
            chip_ptr = &r.filter.mf.node[n].chip[0];
            if((r.filter.mf.node[n].node_num == -1) || (r.filter.mf.node[n].num_chips == chips_in_this_node)){
                continue;
            }
            if (strchr(chr_ptr[filt_idx][1],'[') != NULL) {

                if((tmp_str = (char*)malloc(strlen(chr_ptr[filt_idx][1])+2)) == NULL){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno %d(%s) for size = %d\n"
                        ,__LINE__,__FUNCTION__,errno,strerror(errno),(strlen(chr_ptr[filt_idx][1])+2));
                    return FAILURE;
                }
                j=0;
                strcpy(tmp_str,chr_ptr[filt_idx][1]);
                debug(HTX_HE_INFO,DBG_MUST_PRINT,"tmp_str = %s\n",tmp_str);
                if((ptr[j++] = strtok(tmp_str,"[],")) == NULL){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                    "for mem_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                    filter_ptr[filt_idx],filt_idx);
                    return (FAILURE);
                }
                debug(HTX_HE_INFO,DBG_MUST_PRINT,"ptr[0]=%s\n",ptr[0]);
                if (chr_ptr[filt_idx][1][0]  == '*'){
                        num_chips = chips_in_this_node;
                        for(int chp = 0;(chp<chips_in_this_node);chp++){
                            if(chip_ptr[chp].chip_num == -1){
                                chip_ptr[chp].chip_num = chp;
								temp_chips[chp] = chp;
                            }
                        }    
                }else{
                    errno = 0;     
                    chip_num = strtol(ptr[0],&end_ptr,10);
                    if(chip_num >= chips_in_this_node ||  errno != 0 || end_ptr == ptr[0]){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule rule %s, rule file:%s "
                        "for [%d]mem_filer:%s,chip field as %s\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                        filt_idx,filter_ptr[filt_idx],ptr[0]);
                        return (FAILURE);
                        
                    }           
                    if(chip_ptr[chip_num].chip_num == -1){
                        chip_ptr[chip_num].chip_num = chip_num;
						temp_chips[chip_num] = chip_num;
                        num_chips++;
                    }
                }
                while ((ptr[j]=strtok(NULL,",]")) != NULL ){
                    debug(HTX_HE_INFO,DBG_MUST_PRINT,"ptr[%d]=%s\n",j,ptr[j]);
                    j++;
                }
                for(int k = 0,l=1;l<j;k++,l++){/*l =1,because first token immidiately after "P" in mem_filter is  always chip number,jumping to page details*/
                
                    char *page_size_tok[6],*page_value_tok[6],tmp_page_value[8];
                    int page_index;
                    unsigned long num_value;
                    if(((page_size_tok[k] = strtok(ptr[l],"_")) == NULL) ){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule  %s, rule file:%s "
                            "for [%d]mem_filer:%s,page filed as %s\n",__LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            filt_idx,filter_ptr[filt_idx],ptr[k]);
                            return (FAILURE);
                    }        
                    debug(HTX_HE_INFO,DBG_MUST_PRINT,"page_size_tok[%d]=%s\n",k,page_size_tok[k]);
                    if((strcmp(page_size_tok[k],"4K") == 0) || (strcmp(page_size_tok[k],"4k") == 0)){
                       page_index = PAGE_INDEX_4K; 
                    }else if((strcmp(page_size_tok[k],"64K") == 0) || (strcmp(page_size_tok[k],"64k") == 0)){
                        page_index = PAGE_INDEX_64K;
                    }else if((strcmp(page_size_tok[k],"2M") == 0) || (strcmp(page_size_tok[k],"2m") == 0)){
                        page_index = PAGE_INDEX_2M;
                    }else if((strcmp(page_size_tok[k],"16M") == 0) || (strcmp(page_size_tok[k],"16m") == 0)){
                        page_index = PAGE_INDEX_16M;
                    }else if((strcmp(page_size_tok[k],"16G") == 0) || (strcmp(page_size_tok[k],"16g") == 0)){
                        page_index = PAGE_INDEX_16G;
                    }else{
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                        "for [%d]mem_filer:%s,page filed as %s\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                        filt_idx,filter_ptr[filt_idx],page_size_tok[k]);
                        return (FAILURE);
                    }

                    if((page_value_tok[k] = strtok(NULL,"_")) == NULL) {
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                        "for [%d]mem_filer:%s,page filed as %s\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                        filt_idx,filter_ptr[filt_idx],ptr[l]);
                        return (FAILURE);
                    }
                    if(!(sysptr->memory_details.pdata[page_index].supported)){
                        displaym(HTX_HE_INFO,DBG_IMP_PRINT,"\npage size(%s) mentioned in filter [%d]mem_filer:%s  does not support on this OS/Ssytem,Continuing with next page size\n",
                            page_size_name[page_index],filt_idx,filter_ptr[filt_idx]);
                        continue;
                    }
                    debug(HTX_HE_INFO,DBG_MUST_PRINT,"page_value_tok[%d]=%s\n",k,page_value_tok[k]);
                    strcpy(tmp_page_value,page_value_tok[k]);
                    errno = 0;     
                    if(strstr(tmp_page_value,"%")){
                        page_value_tok[k] = strtok(tmp_page_value,"%");
                        num_value = strtol(page_value_tok[k],&end_ptr,10);
						for(int chip_n =0; chip_n < MAX_CHIPS_PER_NODE; chip_n++){
							if((temp_chips[chip_n]  == -1) || (chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mem != 0)){
								continue;	
							}
                        	if((num_value  <= 0) || (num_value  > 100) ||  (errno != 0) || (end_ptr == page_value_tok[k])) {
                            	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                	"for [%d]mem_filer:%s,page percetage filed as(%s):%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                                	filt_idx,filter_ptr[filt_idx],page_size_name[page_index],num_value);
                            	return (FAILURE);
                        	}
                    

                        	chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mpercent = num_value; 
                            int temp_mem_percent = g_data.gstanza.global_alloc_mem_percent;
                            if(page_index < PAGE_INDEX_2M){
                                if(g_data.gstanza.global_alloc_mem_size > 0 && g_data.gstanza.global_alloc_mem_size < sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free){
                                    temp_mem_percent = (g_data.gstanza.global_alloc_mem_size * 100.0) / (sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free);
                                }
                        	    chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mem = (num_value/100.0) * 
                                    (sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free) * (temp_mem_percent/100.0);                         
                            }else{
                                chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mem = (num_value/100.0) *
                                    (sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free);
                            }
						}
                    }else if(strstr(tmp_page_value,"MB")){
                        page_value_tok[k] = strtok(tmp_page_value,"MB");
                        errno = 0;
                        num_value = strtol(page_value_tok[k],&end_ptr,10);
						for(int chip_n =0; chip_n < MAX_CHIPS_PER_NODE; chip_n++){
							if((temp_chips[chip_n]  == -1) || (chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mem != 0 ||
                                sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free == 0 )){
								continue;	
							}
							if(((num_value * MB) <= 0) || ((num_value * MB) >= sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free)
								 || (errno != 0) || (end_ptr == page_value_tok[k])){
								displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule rule %s, rule file:%s "
								"for [%d]mem_filer:%s,page value field as(%s):%d MB,memory abosulte value must not be <=0"
								"  nor should exceed max limit %luMB\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
								filt_idx,filter_ptr[filt_idx],page_size_name[page_index],num_value,
								(sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free/MB));
								return (FAILURE);

							}
							chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mem = (num_value * MB);
						}
                    }else if(strstr(tmp_page_value,"GB")){
                        page_value_tok[k] = strtok(tmp_page_value,"GB");
                        errno = 0;
                        num_value = strtol(page_value_tok[k],&end_ptr,10);
						for(int chip_n =0; chip_n < MAX_CHIPS_PER_NODE; chip_n++){
							if((temp_chips[chip_n]  == -1) || (chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mem != 0 ||
                                sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free == 0 )){
								continue;	
							}
							if(((num_value * GB) <= 0) || ((num_value * GB) >= sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free) ||
								 (errno != 0) || (end_ptr == page_value_tok[k])){
								displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
								"file for [%d]mem_filer:%s,page value field as(%s):%dGB,memory abosulte value must not be <=0"
								"  nor should exceed max limit %luMB\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
								filt_idx,filter_ptr[filt_idx],page_size_name[page_index],num_value,
								(sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free/MB));
								return (FAILURE);

							}
                        	chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mem = (num_value * GB);
						}
                    }else if(strstr(tmp_page_value,"#")){
                        unsigned long mem_size=0;
                        page_value_tok[k] = strtok(tmp_page_value,"#");
                        errno = 0;
                        num_value = strtol(page_value_tok[k],&end_ptr,10);
						for(int chip_n =0; chip_n < MAX_CHIPS_PER_NODE; chip_n++){
							temp_chips[chip_n] = chip_ptr[chip_n].chip_num;
							if((temp_chips[chip_n]  == -1) || (chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mem != 0 ||
                                sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free == 0 )){
								continue;	
							}
                        	mem_size  = (num_value * sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].psize);
                        	if((mem_size <=0) || (mem_size >= sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free) || (errno != 0) || (end_ptr == page_value_tok[k])){
                            	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            	"for [%d]mem_filer:%s,number of pages as (%s):%d pages, num pages  must not be <=0  "
                            	"nor should exceed %d ,errored out for node:%d,chip:%d\n",__LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            	filt_idx,filter_ptr[filt_idx],page_size_name[page_index],num_value,
                            	(sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].free/sysptr->node[n].chip[chip_n].mem_details.pdata[page_index].psize),
                                 n,chip_n);
                            	return (FAILURE);

                       	 	}	 
                        	chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_mem = mem_size;
                        	chip_ptr[chip_n].mem_details.pdata[page_index].page_wise_usage_npages = num_value;
						}
                    }else {

                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            "for [%d]mem_filer:%s,page filed as %s must be in percentage  or MB or GB or #, "
                            "as documneted in rule file\n",__LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            filt_idx,filter_ptr[filt_idx],tmp_page_value);
                             return (FAILURE);
                    }
                    debug(HTX_HE_INFO,DBG_MUST_PRINT,"page_value_tok[%d]=%s,num_value=%d\n",k,page_value_tok[k],num_value);
					if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
						for(int chip_n =0; chip_n < MAX_CHIPS_PER_NODE; chip_n++){
							if(chip_ptr[chip_n].chip_num == -1)continue;	
                    		sprintf(msg_temp,"\nmem filter N[%d]P%d]page[%s] mem_to_use =%lu\n",n,chip_n,page_size_name[page_index],
                        		r.filter.mf.node[n].chip[chip_n].mem_details.pdata[page_index].page_wise_usage_mem);
                    		strcat(msg,msg_temp);
						}
					}
                }                    
                free(tmp_str);
                r.filter.mf.node[n].num_chips = num_chips;
				/*initialize chip_num=-1 so that if any next filter with different chip of same node should not consider current filter page values*/
				for(int chip_n =0; chip_n < MAX_CHIPS_PER_NODE; chip_n++){
					temp_chips[chip_n] = -1;
				}
            }else{
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                    "for [%d]mem_filer:%s,chip field as %c\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                    filt_idx,filter_ptr[filt_idx],chr_ptr[filt_idx][1][0]);
                    return (FAILURE);
            }
            
        }
    }
    displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"%s\n",msg);
    return SUCCESS;
}
int parse_cpu_filter(char filter_ptr[][MAX_POSSIBLE_ENTRIES]){
    int i,j,filt_idx,num_nodes = 0,num_chips = 0,num_cores=0,num_cpus=0;
    unsigned long node_num,chip_num,core_num,cpu_num,cores_in_this_chip,chips_in_this_node,nodes_in_this_sys,cpus_in_this_core;
    char *tmp_str=NULL,msg[8192],msg_temp[2048];
    char* chr_ptr[r.num_cpu_filters][8],*end_ptr;
    char *ptr[16],*tmp,c[8],tmp_filter_str[MAX_POSSIBLE_ENTRIES];


    for(int node = 0; node < MAX_NODES;node++ ){
        for(int chip = 0; chip < MAX_CHIPS_PER_NODE;chip++){
            for(int core = 0; core < MAX_CORES_PER_CHIP;core++){
                for(int lcpu=0;lcpu < MAX_CPUS_PER_CORE;lcpu++){
                   r.filter.cf.node[node].chip[chip].core[core].lprocs[lcpu]=-1;
                   r.filter.cf.node[node].chip[chip].core[core].num_procs = 0;
                }
                r.filter.cf.node[node].chip[chip].core[core].core_num=-1;
            }
            r.filter.cf.node[node].chip[chip].num_cores=0;
            r.filter.cf.node[node].chip[chip].chip_num =-1;
            
        }
        r.filter.cf.node[node].num_chips=0;
        r.filter.cf.node[node].node_num=-1;
    }r.filter.cf.num_nodes =0;

#if 0
    for(int row = 0;row < MAX_FILTERS;row++){
        for(int col=0;col<MAX_NODES;col++){
            nodes[row][col] = -1;
        }
    }
#endif
	if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
    	sprintf(msg,"CPU Filter:");
	}
    for(filt_idx = 0; filt_idx < r.num_cpu_filters; filt_idx++){
		if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
        	sprintf(msg_temp,"\n=====================================================\n"
            	"cpu filter[%d]=%s\n",filt_idx,filter_ptr[filt_idx]);
        	strcat(msg,msg_temp);
		}
        i = 0,j=0,node_num=0,num_nodes=0;
        strcpy(tmp_filter_str,filter_ptr[filt_idx]);
        if((strchr(tmp_filter_str,'N') == NULL)||(strchr(tmp_filter_str,'P')== NULL)||(strchr(tmp_filter_str,'C')== NULL)||(strchr(tmp_filter_str,'T')== NULL)){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
            "for cpu_filer:%s,filter num:%d,cpu filter must conatin all 'N','P','C' and 'T'\n", 
            __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx);
            return FAILURE;
        }
        if ((chr_ptr[filt_idx][i] = strtok(tmp_filter_str, "NPCT")) != NULL) {
            i++;
            while ((chr_ptr[filt_idx][i] = strtok(NULL, "NPCT")) != NULL) {
                i++;    
            }
        }else {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                filter_ptr[filt_idx],filt_idx);
            return (FAILURE);
        }
        nodes_in_this_sys = get_num_of_nodes_in_sys();
        if (chr_ptr[filt_idx][0][0] == '*'){
            num_nodes = nodes_in_this_sys;
            r.filter.cf.num_nodes = nodes_in_this_sys;
            for(int n=0;n<num_nodes;n++){
                if(r.filter.cf.node[n].node_num == -1){
                     r.filter.cf.node[n].node_num=n;
                }
            }
        }
        else if (strchr(chr_ptr[filt_idx][0],'[') != NULL) { /* indicates a range of nodes is given in filter */
            if((tmp_str = (char*)malloc(strlen(chr_ptr[filt_idx][0])+2)) == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno %d(%s) for size = %d\n"
                        ,__LINE__,__FUNCTION__,errno,strerror(errno),(strlen(chr_ptr[filt_idx][0])+2));
                return FAILURE;
            }
            strcpy(tmp_str,chr_ptr[filt_idx][0]);
            if((ptr[j++] = strtok(tmp_str,"[],")) == NULL){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                    "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                    filter_ptr[filt_idx],filt_idx);
                return (FAILURE);
            }
            
            while ((ptr[j]=strtok(NULL,",]")) != NULL ){
                j++;
            }
            errno=0;
            for(int k = 0;k<j;k++){
                if ((tmp = strchr(ptr[k],'%')) != NULL) { /* indicates a percentage of nodes is given in filter */
                    num_nodes = (nodes_in_this_sys * (float)((atoi(tmp+1))/100.0));
                    if((num_nodes< 0) ||  (num_nodes> nodes_in_this_sys)){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            "for cpu_filer:%s,filter num:%d, node percentage as %d%(num_nodes=%d)\n", __LINE__,__FUNCTION__,
                            r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_nodes);
                        return (FAILURE);
                    }
                    if(num_nodes== 0)num_nodes= 1;
                    for(int n =0;n<num_nodes;n++){
                        if(r.filter.cf.node[n].node_num == -1){
                             r.filter.cf.node[n].node_num = n;
                        }
                    }
                }else if (strchr(ptr[k],'-') == NULL){
                    node_num = strtol(ptr[k],&end_ptr,10);
                    if(node_num >= nodes_in_this_sys ||  errno != 0 || end_ptr == ptr[k]){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            "for cpu_filer:%s,filter num:%d, node filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                            r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,node_num,nodes_in_this_sys);
                        return (FAILURE);
                    }
                    num_nodes++;
                    if(r.filter.cf.node[node_num].node_num == -1){
                        r.filter.cf.node[node_num].node_num = node_num;
                    }
                }
                    
                else { 
                    strcpy(c,ptr[k]);
                    if((tmp=strtok(c,"-")) == NULL){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            filter_ptr[filt_idx],filt_idx);
                        return (FAILURE);

                    }
                    if((tmp=strtok(NULL,"-")) == NULL){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            filter_ptr[filt_idx],filt_idx);
                        return (FAILURE);

                    }
                    errno = 0;
                    node_num = strtol(tmp,&end_ptr,10);
                    if(r.filter.cf.node[node_num].node_num == -1){
                        r.filter.cf.node[node_num].node_num = node_num;
                    }
                    if(node_num >= nodes_in_this_sys ||  errno != 0 || end_ptr == ptr[k]){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            "for cpu_filer:%s,filter num:%d, node filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                            filter_ptr[filt_idx],filt_idx,node_num,nodes_in_this_sys);
                         return (FAILURE);
                    }
                    errno = 0;
                    for(int n=strtol(&ptr[k][0],&end_ptr,10);n <= node_num;n++){
                        if(n >= nodes_in_this_sys || errno != 0 || end_ptr == &ptr[k][0]){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d, node filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                                filter_ptr[filt_idx],filt_idx,node_num,nodes_in_this_sys);
                            return (FAILURE);
                        }
                        if(r.filter.cf.node[n].node_num == -1){
                            r.filter.cf.node[n].node_num = n;
                        }
                        num_nodes++;
                    }
                }
            }
            free(tmp_str);
        }else if ((tmp = strchr(chr_ptr[filt_idx][0],'%')) != NULL) { /* indicates a percentage of nodes is given in filter */
            num_nodes = (nodes_in_this_sys * (float)((atoi(tmp+1))/100.0));
            if((num_nodes < 0) ||  (num_nodes > nodes_in_this_sys)){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                    "for cpu_filer:%s,filter num:%d, node percentage as %d%(num_nodes=%d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                    filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_nodes);
                return (FAILURE);
            }
            if(num_nodes == 0)num_nodes=1;
            for(int n=0;(n<num_nodes);n++){
                if(r.filter.cf.node[n].node_num == -1)
                r.filter.cf.node[n].node_num=n;
            }
        }else {
            errno = 0;
            node_num = strtol(chr_ptr[filt_idx][0],&end_ptr,10);
            if(node_num >= nodes_in_this_sys ||  errno != 0 || end_ptr == chr_ptr[filt_idx][0]){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                    "for cpu_filer:%s,filter num:%d, node filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                     filter_ptr[filt_idx],filt_idx,node_num,nodes_in_this_sys);
                return (FAILURE);
            }
            num_nodes = 1;
            if(r.filter.cf.node[node_num].node_num == -1){
                r.filter.cf.node[node_num].node_num = node_num;
            }
        }
		if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
				sprintf(msg_temp,"nodes=%d:\t",num_nodes);
				strcat(msg,msg_temp);
				for(int k=0;k<MAX_NODES;k++){
					if(r.filter.cf.node[k].node_num == -1) continue;
					sprintf(msg_temp," %d \t",r.filter.cf.node[k].node_num);
					strcat(msg,msg_temp);
        		} 
		}
        struct chip_info *chip_ptr = NULL;
        r.filter.cf.num_nodes = num_nodes;
        for (int n = 0; n < MAX_NODES; n++) {
            num_chips = 0;chip_num=0;
            chips_in_this_node= get_num_of_chips_in_node(n);
            chip_ptr = &r.filter.cf.node[n].chip[0];
            if((r.filter.cf.node[n].node_num == -1) || (r.filter.cf.node[n].num_chips == chips_in_this_node)){
                continue;
            } 
            if (chr_ptr[filt_idx][1][0] == '*'){
                    num_chips = chips_in_this_node;
                    for(int chp = 0;(chp<chips_in_this_node);chp++){
                        if(chip_ptr[chp].chip_num == -1)
                             chip_ptr[chp].chip_num = chp;
                    }
            } else if (strchr(chr_ptr[filt_idx][1],'[') != NULL) { /*indicates a range of chips is given in filter */
        
                if((tmp_str = (char*)malloc(strlen(chr_ptr[filt_idx][1])+2)) == NULL){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno %d(%s) for size = %d\n"
                            ,__LINE__,__FUNCTION__,errno,strerror(errno),(strlen(chr_ptr[filt_idx][1])+2));
                    return FAILURE;
                }
                j=0;
                strcpy(tmp_str,chr_ptr[filt_idx][1]);
                if((ptr[j++] = strtok(tmp_str,"[],")) == NULL){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                        "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,
                        r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx);
                    return (FAILURE);
                }

                while ((ptr[j]=strtok(NULL,",]")) != NULL ){
                    j++;
                }
                errno=0;
                for(int k = 0;k<j;k++){
                    if ((tmp = strchr(ptr[k],'%')) != NULL) { /* indicates a percentage of chips is given in filter */
                        num_chips = (chips_in_this_node * (float)((atoi(tmp+1))/100.0));
                        if((num_chips < 0) ||  (num_chips > chips_in_this_node)){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d, chip percentage as %d%(num_chips=%d)\n", __LINE__,__FUNCTION__,
                                r.rule_id,g_data.rules_file_name, filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_chips);
                            return (FAILURE);
                        }
                        if(num_chips== 0)num_chips= 1;
                        for(int chp =0;(chp<num_chips);chp++){
                            if(chip_ptr[chp].chip_num == -1)
                                chip_ptr[chp].chip_num = chp;
                        }
                    }else if (strchr(ptr[k],'-') == NULL){
                        chip_num = strtol(ptr[k],&end_ptr,10);
                        if(chip_num >= chips_in_this_node ||  errno != 0 || end_ptr == ptr[k]){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d, chip filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                                r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,chip_num,chips_in_this_node);
                            return (FAILURE);
                        }
                        if(chip_ptr[chip_num].chip_num == -1){
                            chip_ptr[chip_num].chip_num = chip_num;
                            num_chips++;
                        }
                    }
                    else {
                        strcpy(c,ptr[k]);
                        if((tmp=strtok(c,"-")) == NULL){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                                filter_ptr[filt_idx],filt_idx);
                            return (FAILURE);

                        }
                        if((tmp=strtok(NULL,"-")) == NULL){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                                filter_ptr[filt_idx],filt_idx);
                            return (FAILURE);

                        }
                        errno = 0;
                        chip_num = strtol(tmp,&end_ptr,10);
                        if(chip_ptr[chip_num].chip_num == -1){
                            chip_ptr[chip_num].chip_num = chip_num;
                        }
                        if(chip_num >= chips_in_this_node ||  errno != 0 || end_ptr == ptr[k]){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule  %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d, chip field as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                                r.rule_id,g_data.rules_file_name, filter_ptr[filt_idx],filt_idx,chip_num,chips_in_this_node);
                             return (FAILURE);
                        }
                        errno = 0;
                        for(int chp=strtol(&ptr[k][0],&end_ptr,10);(chp <= chip_num);chp++){
                            if(chp >=chips_in_this_node || errno != 0 || end_ptr == &ptr[k][0]){
                                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d, chip filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                                r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,chip_num,chips_in_this_node);
                                return (FAILURE);
                            }
                            if(chip_ptr[chp].chip_num==-1){
                                chip_ptr[chp].chip_num=chp;
                            }
                            num_chips++;
                       
                        }
                    }
                }
                free(tmp_str);

            }else if ((tmp = strchr(chr_ptr[filt_idx][1],'%')) != NULL) { /* indicates a percentage of chips is given in filter */  
                num_chips = (chips_in_this_node * (float)((atoi(tmp+1))/100.0));
                if((num_chips < 0) ||  (num_chips > chips_in_this_node)){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                        "for cpu_filer:%s,filter num:%d, chip percentage as %d%(num_chips=%d)\n", __LINE__,__FUNCTION__,
                        r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_chips);
                    return (FAILURE);
                }
                if(num_chips == 0)num_chips = 1;
                for(int chp =0;(chp<num_chips);chp++){
                    if(chip_ptr[chp].chip_num==-1)
                        chip_ptr[chp].chip_num = chp;
                }
            }else {
                errno = 0;
                chip_num = strtol(chr_ptr[filt_idx][1],&end_ptr,10);
                if(chip_num >= chips_in_this_node ||  errno != 0 || end_ptr == chr_ptr[filt_idx][1]){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                    "for cpu_filer:%s,filter num:%d, chip filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                    r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,chip_num,chips_in_this_node);
                    return (FAILURE);
                }
                num_chips = 1;
                if(chip_ptr[chip_num].chip_num == -1){
                    chip_ptr[chip_num].chip_num = chip_num;
                }
                
            }
            r.filter.cf.node[n].num_chips = num_chips;
			if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
				sprintf(msg_temp,"\nN:%d,chips=%d:\t",n,num_chips);
				strcat(msg,msg_temp);
				for(int k=0;k<MAX_CHIPS_PER_NODE;k++){
					if(chip_ptr[k].chip_num == -1)continue;
					sprintf(msg_temp," %d \t",chip_ptr[k].chip_num);
					strcat(msg,msg_temp);
				}
			}	
            for (int chp = 0; chp < MAX_CHIPS_PER_NODE; chp++) {
                num_cores = 0;core_num=0;
                cores_in_this_chip=get_num_of_cores_in_chip(n,chp);
                if((chip_ptr[chp].chip_num  == -1) || (r.filter.cf.node[n].chip[chp].num_cores == cores_in_this_chip)){
                    continue;
                }
                struct core_info *core_ptr = &r.filter.cf.node[n].chip[chp].core[0];
                if (chr_ptr[filt_idx][2][0] == '*'){
                        num_cores = cores_in_this_chip;
                        for(int c = 0;(c<cores_in_this_chip); c++){
                            if (core_ptr[c].core_num == -1)
                                 core_ptr[c].core_num = c;
                        }

                } else if (strchr(chr_ptr[filt_idx][2],'[') != NULL) { /*indicates a range of cores is given in filter */

                    if((tmp_str = (char*)malloc(strlen(chr_ptr[filt_idx][2])+2)) == NULL){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno %d(%s) for size = %d\n"
                                ,__LINE__,__FUNCTION__,errno,strerror(errno),(strlen(chr_ptr[filt_idx][2])+2));
                        return FAILURE;
                    }
                    j=0;
                    strcpy(tmp_str,chr_ptr[filt_idx][2]);
                    if((ptr[j++] = strtok(tmp_str,"[],")) == NULL){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s" 
                        "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,
                        g_data.rules_file_name,filter_ptr[filt_idx],filt_idx);
                        return (FAILURE);
                    }
                    while ((ptr[j]=strtok(NULL,",]")) != NULL ){
                        j++;
                    }
                    errno=0;
                    for(int k = 0;k<j;k++){

                        if ((tmp = strchr(ptr[k],'%')) != NULL) { /* indicates a percentage of cores is given in filter */
                            num_cores = (cores_in_this_chip * (float)((atoi(tmp+1))/100.0));
                            if((num_cores < 0) ||  (num_cores > cores_in_this_chip)){
                                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                    "for cpu_filer:%s,filter num:%d, core percentage as %d%(num_cores=%d)\n", __LINE__,__FUNCTION__,
                                    r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_cores);
                                return (FAILURE);
                            }
                            if(num_cores == 0)num_cores = 1;
                            for(int core =0;(core<num_cores);core++){
                                if (core_ptr[core].core_num == -1)
                                core_ptr[core].core_num = core;
                            }
                        }else if (strchr(ptr[k],'-') == NULL){
                            errno = 0;
                            core_num = strtol(ptr[k],&end_ptr,10);
                            if(core_num >= cores_in_this_chip ||  errno != 0 || end_ptr == ptr[k]){
                                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                    "for cpu_filer:%s,filter num:%d, core filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                                    r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,core_num,cores_in_this_chip);
                                return (FAILURE);
                            }
                            if(core_ptr[core_num].core_num == -1){
                                core_ptr[core_num].core_num = core_num;
                            }
                            num_cores++;
                        }
                        else {
                            strcpy(c,ptr[k]);
                            if((tmp=strtok(c,"-")) == NULL){
                                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                    "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                                    filter_ptr[filt_idx],filt_idx);
                                return (FAILURE);

                            }
                            if((tmp=strtok(NULL,"-")) == NULL){
                                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                    "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                                    filter_ptr[filt_idx],filt_idx);
                                return (FAILURE);

                            }
                            errno = 0;
                            core_num = strtol(tmp,&end_ptr,10);
                            if(core_ptr[core_num].core_num == -1){
                                core_ptr[core_num].core_num = core_num;
                            }
                            if(core_num >= cores_in_this_chip ||  errno != 0 || end_ptr == ptr[k]){
                                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule file %s, rule file:%s "
                                    "for cpu_filer:%s,filter num:%d, core field as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                                    r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,core_num,cores_in_this_chip);
                                 return (FAILURE);
                            }
                            errno = 0;
                            for(int c=strtol(&ptr[k][0],&end_ptr,10);c <= core_num;c++){
                                if(c >=cores_in_this_chip || errno != 0 || end_ptr == &ptr[k][0]){
                                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                        "for cpu_filer:%s,filter num:%d, core filed as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                                        r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,core_num,cores_in_this_chip);
                                    return (FAILURE);
                                }
                                if(core_ptr[c].core_num==-1){
                                    core_ptr[c].core_num=c;
                                }
                                num_cores++;

                            }
                        }
                    }
                    free(tmp_str);
                }else if ((tmp = strchr(chr_ptr[filt_idx][2],'%')) != NULL) { /* indicates a percentage of cores is given in filter */

                    num_cores = (cores_in_this_chip * (float)((strtol(tmp+1,&end_ptr,10))/100.0));
                    if((num_cores < 0) ||  (num_cores > cores_in_this_chip)){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            "for cpu_filer:%s,filter num:%d, core percentage as %d%(num_cores=%d)\n", __LINE__,__FUNCTION__,
                            r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_cores);
                        return (FAILURE);
                    }
                    if(num_cores == 0)num_cores = 1;
                    for(int c =0;(c<num_cores);c++){
                        if(core_ptr[c].core_num == -1)
                            core_ptr[c].core_num = c;
                    }
                }else {
                    errno = 0;
                    core_num = strtol(chr_ptr[filt_idx][2],&end_ptr,10);
                    if(core_num >= cores_in_this_chip ||  errno != 0 || end_ptr == chr_ptr[filt_idx][2]){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                            "for cpu_filer:%s,filter num:%d, core field as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                            r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,core_num,cores_in_this_chip);
                        return (FAILURE);
                    }
                    num_cores = 1;
                    if(core_ptr[core_num].core_num == -1){
                        core_ptr[core_num].core_num = core_num;
                    }
                }
                r.filter.cf.node[n].chip[chp].num_cores = num_cores;
				if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
                	sprintf(msg_temp,"\nN:%d P:%d,cores=%d:\t",n,chp,num_cores);
                	strcat(msg,msg_temp);
                	for(int k=0;k<MAX_CORES_PER_CHIP;k++){
                    	if(core_ptr[k].core_num == -1)continue;
                    	sprintf(msg_temp," %d \t",core_ptr[k].core_num);
                    	strcat(msg,msg_temp);
                	}
				}
                for (int cor = 0; cor < MAX_CORES_PER_CHIP; cor++) {
                    num_cpus = 0;cpu_num=0;
                    cpus_in_this_core= get_num_of_cpus_in_core(n,chp,cor);
                    if((core_ptr[cor].core_num  == -1) || (r.filter.cf.node[n].chip[chp].core[cor].num_procs == cpus_in_this_core)){
                        continue;
                    }
                    unsigned int *thread_ptr = &r.filter.cf.node[n].chip[chp].core[cor].lprocs[0];
                    if (chr_ptr[filt_idx][3][0] == '*'){
                            num_cpus = cpus_in_this_core;
                            for(int t = 0;t<cpus_in_this_core;t++)
                                thread_ptr[t] = get_logical_cpu_num(n,chp,cor,t);

                    } else if (strchr(chr_ptr[filt_idx][3],'[') != NULL) { /*indicates a range of cores is given in filter */

                        if((tmp_str = (char*)malloc(strlen(chr_ptr[filt_idx][3])+2)) == NULL){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:malloc failed with errno %d(%s) for size = %d\n"
                                    ,__LINE__,__FUNCTION__,errno,strerror(errno),(strlen(chr_ptr[filt_idx][3])+2));
                            return FAILURE;
                        }
                        j=0;
                        strcpy(tmp_str,chr_ptr[filt_idx][3]);
                        if((ptr[j++] = strtok(tmp_str,"[],")) == NULL){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,
                            g_data.rules_file_name,filter_ptr[filt_idx],filt_idx);
                            return (FAILURE);
                        }

                        while ((ptr[j]=strtok(NULL,",]")) != NULL ){
                            j++;
                        }
                        errno=0;
                        for(int k = 0;k<j;k++){
                            if ((tmp = strchr(ptr[k],'%')) != NULL) { /* indicates a percentage of cores is given in filter */
                                num_cpus = (cpus_in_this_core * (float)((atoi(tmp+1))/100.0));
                                if((num_cpus < 0) ||  (num_cpus > cpus_in_this_core)){
                                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                        "for cpu_filer:%s,filter num:%d, cpu percentage as %d%(num_cpus=%d)\n", __LINE__,__FUNCTION__,
                                        r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_cpus); 
                                    return (FAILURE);
                                }
                                if(num_cpus == 0)num_cpus = 1;
                                for(int t =0;(t<num_cpus);t++){
                                    if (thread_ptr[t] == -1)
                                        thread_ptr[t] = get_logical_cpu_num(n,chp,cor,t);
                                }

                            }else if (strchr(ptr[k],'-') == NULL){
                                errno = 0;
                                cpu_num = strtol(ptr[k],&end_ptr,10);
                                if(cpu_num >= cpus_in_this_core ||  errno != 0 || end_ptr == ptr[k]){
                                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                        "for cpu_filer:%s,filter num:%d, thread field as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                                        r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,cpu_num,cpus_in_this_core);
                                    return (FAILURE);
                                }
                                if(thread_ptr[cpu_num]== -1){
                                    thread_ptr[cpu_num]= get_logical_cpu_num(n,chp,cor,cpu_num);
                                }
                                num_cpus++;
                            }
                            else {
                                strcpy(c,ptr[k]);
                                if((tmp=strtok(c,"-")) == NULL){
                                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                        "cpu_filer:%s,filter num:%d\n", __LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                                        filter_ptr[filt_idx],filt_idx);
                                    return (FAILURE);

                                }
                                if((tmp=strtok(NULL,"-")) == NULL){
                                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                        "for cpu_filer:%s,filter num:%d\n",__LINE__,__FUNCTION__,r.rule_id,g_data.rules_file_name,
                                        filter_ptr[filt_idx],filt_idx);
                                    return (FAILURE);

                                }
                                errno = 0;
                                cpu_num = strtol(tmp,&end_ptr,10);
                                if(thread_ptr[cpu_num]== -1){
                                    thread_ptr[cpu_num] = get_logical_cpu_num(n,chp,cor,cpu_num);
                                }
                                if(cpu_num >= cpus_in_this_core ||  errno != 0 || end_ptr == ptr[k]){
                                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                        "for cpu_filer:%s,filter num:%d, thread field as %d ( must be < %d)\n", __LINE__,__FUNCTION__,r.rule_id,
                                        g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,cpu_num,cpus_in_this_core);
                                     return (FAILURE);
                                }
                                errno = 0;
                                for(int t=strtol(&ptr[k][0],&end_ptr,10);t<= cpu_num;t++){
                                    if(t >=cpus_in_this_core || errno != 0 || end_ptr == &ptr[k][0]){
                                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                            "for cpu_filer:%s,filter num:%d, thread field as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                                            r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,cpu_num,cpus_in_this_core);
                                        return (FAILURE);
                                    }
                                    if(thread_ptr[t] == -1){
                                         thread_ptr[t]=get_logical_cpu_num(n,chp,cor,t);
                                    }
                                    num_cpus++;

                                }
                            }
                        }
                        free(tmp_str);
                    }else if ((tmp = strchr(chr_ptr[filt_idx][3],'%')) != NULL) { /* indicates a percentage of cores is given in filter */
                        num_cpus = (cpus_in_this_core * (float)((atoi(tmp+1))/100.0));
                        if((num_cpus < 0) ||  (num_cpus > cpus_in_this_core)){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d, thread percentage as %d%(num_cpus=%d)\n", __LINE__,__FUNCTION__,
                                r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,atoi(tmp+1),num_cpus);
                            return (FAILURE);
                        }
                        if(num_cpus == 0)num_cpus = 1;
                        for(int t =0;(t<num_cpus);t++){
                            if(thread_ptr[t]==-1)
                                thread_ptr[t] = get_logical_cpu_num(n,chp,cor,t);
                        }
                    }else {
                        errno = 0;
                        cpu_num = strtol(chr_ptr[filt_idx][3],&end_ptr,10);
                        if(cpu_num >= cpus_in_this_core ||  errno != 0 || end_ptr == chr_ptr[filt_idx][3]){
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Improper value is provided in rule %s, rule file:%s "
                                "for cpu_filer:%s,filter num:%d, cpu field as %d ( must be < %d)\n", __LINE__,__FUNCTION__,
                                r.rule_id,g_data.rules_file_name,filter_ptr[filt_idx],filt_idx,cpu_num,cpus_in_this_core);
                            return (FAILURE);
                        }
                        num_cpus = 1;
                        if(thread_ptr[cpu_num] == -1){
                            thread_ptr[cpu_num] = get_logical_cpu_num(n,chp,cor,cpu_num);
                        }
                    }
                    r.filter.cf.node[n].chip[chp].core[cor].num_procs = num_cpus;
					if(g_data.gstanza.global_debug_level >= DBG_DEBUG_PRINT){
                    	sprintf(msg_temp,"\nN:%d P:%d C:%d,cpus=%d:\t",n,chp,cor,num_cpus);
                    	strcat(msg,msg_temp);
						if(num_cpus == get_num_of_cpus_in_core(n,chp,cor)){
							sprintf(msg_temp," [%d - %d]\n",thread_ptr[0],thread_ptr[num_cpus-1]);
							strcat(msg,msg_temp);
						}else{
                    		for(int k=0;k<MAX_CPUS_PER_CORE;k++){
								if(thread_ptr[k] == -1)continue;
                        		sprintf(msg_temp," %d \t",thread_ptr[k]);
                        		strcat(msg,msg_temp);
                    		}
						}
					}
                }

            }
        }

        
    }
    displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"%s\n",msg);
    return SUCCESS;
}
