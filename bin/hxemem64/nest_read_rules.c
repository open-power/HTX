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

struct rule_info r;
extern struct mem_exer_info mem_g;
extern struct nest_global g_data;

void free_pattern_buffers(void);
/*****************************************************************************
 *  This routine sets the default values for the specified rule stanza. You  *
 *  will need to modify it to suit your needs.                               *
 ****************************************************************************/
void set_defaults(void)
{
    strcpy(r.rule_id,"");
    strcpy(r.pattern_id,"        ");
    r.num_oper = 1;
    r.num_writes = 1;
    r.disable_cpu_bind = 0;
    r.num_reads = 1;
    r.num_compares = 1;
    r.num_threads= -1;
    r.percent_hw_threads = 10;
    r.random_bind = 0;
    r.affinity = LOCAL;
    strcpy(r.oper,"MEM");
    r.operation = OPER_MEM;
    strcpy(r.compare,"YES");
    r.compare_flag = 1;
    strcpy(r.crash_on_mis,"YES");
    r.misc_crash_flag = 1;
    strcpy(r.turn_attn_on,"NO");
    r.attn_flag = 0;
    r.width = LS_DWORD;
	r.mem_percent = DEFAULT_MEM_PERCENT;
	r.cpu_percent = DEFAULT_CPU_PERCENT;
    r.debug_level = 0;
    r.num_patterns = 0;
    strcpy(r.pattern_name[0]," ");
    r.nx_rem_th_flag = 1;
    r.nx_perf_flag = 0;
    r.nx_async_flag = 0;
    r.stride_sz = 128;
    r.mem_l4_roll = 134217728;
    r.bm_position = 7;
    r.bm_length = 2;
    r.mcs_mask = -1;
    r.seed = 0; 
    strcpy(r.tlbie_test_case,"RAND_TLB");
    strcpy(r.cpu_filter_str[0],"N*P*C*T*");
    strcpy(r.mem_filter_str[0],"N*P*[4K_100%,64K_100%,2M_100%,16M_100%]");
    r.num_mem_filters = 1;
    r.num_cpu_filters = 1;
    for(int i=0;i<MAX_PAGE_SIZES;i++){
        r.seg_size[i] = -1;
    }
}

/*****************************************************************************
*  This routine reads a line from "stdin" into the specified string.  It     *
*  returns the length of the string. If the length is 1 the line is blank.   *
*  When it reaches EOF the length is set to 0,                               *
*****************************************************************************/
int get_line( char s[], int lim, FILE *fp)
{
    int c=0,i;

    i=0;
    while (--lim > 0 && ((c = fgetc(fp)) != EOF) && c != '\n') {
        s[i++] = c;
     }
    if (c == '\n')
        s[i++] = c;
    s[i] = '\0';
     return(i);
}

/************************************************************************* 
FUNCTION int fill_pattern_details(int pi , char *str, int *line)		 *
 *      pi : pattern index in the rules stanza							 *	
 *     str : pattern name string										 *
 *    line : pointer to line number currently parsed in the rules file	 *
*************************************************************************/
int fill_pattern_details(int pi, char *str, int *line)
{
    char tmp_str[32],buff[128],*pstr=&buff[0],**ptr=&pstr,*strptr;
    int len  = 0, i,res=0;
    unsigned long val = 0;
    debug(HTX_HE_INFO,DBG_INFO_PRINT,"#fill_pattern_details: pi=%d,str=%s,line=%d\n",\
                pi,str,*line);

    /*strncpy(r.pattern_name[pi],str,66);*/
    if( strncmp(str,"0X",2) == 0 ) {
        if (strcmp(r.oper,"DMA")==0){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "[%d]%s:line#%d pattern = %s "
                     "(DMA oper cannot have HEX pattern like 0xBEEFDEAD)\nOnly file"
                    "name is allowed in pattern field.\n",__LINE__,__FUNCTION__,*line, str);
        }
        /* Pattern is specified as a HEX immediate value like 0xFFFFFFFF */
        len=strlen(str);
        len = len - 2;
        if (len == 0) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:HEX pattern cannot be zero sized\n",__LINE__,__FUNCTION__);
            /*re_initialize_global_vars();*/
            exit(1);
        }
        if ( len%16 != 0 ) {
            /* Pattern should be multiple of 8 bytes i.e, 16 characters */
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:#line %d: %s pattern value "
                "should be in multiple of 8 bytes \n",__LINE__,__FUNCTION__,*line,str);
            return(1);
        }
        /* Fill the size of the pattern */
        r.pattern_size[pi]=len/2; /* 0xFF 2 characters is 1 byte */
        if ( (r.pattern_size[pi]) & (r.pattern_size[pi]-1)) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:#line %d: %s pattern's length "
                "should be power of 2 bytes\n",__LINE__,__FUNCTION__,*line,str);
            return(1);
        }
        r.pattern[pi]=(char *) malloc(r.pattern_size[pi]);
        if (r.pattern[pi] == NULL ) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:#line %d: %s pattern value,"
                    " pattern buffer malloc error - %d (%s)\n",__LINE__,__FUNCTION__,*line,str,errno,strerror(errno));
        }

        debug(HTX_HE_INFO,DBG_INFO_PRINT,"#fill_pattern_details: len =%d\n",len);
        str=str+2;
        i = 0;
        while (i < len) {
            strncpy(tmp_str,str,16);
            tmp_str[16]='\0';
            val=strtoul(tmp_str,NULL,16);
            if (val == 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:#line %d: %s pattern is not "
                        "proper hex value\n",__LINE__,__FUNCTION__,*line,str);
            }
            debug(HTX_HE_INFO,DBG_INFO_PRINT,"#fill_pattern_details: val =0x%lx\n",val);
            *(unsigned long *)(&r.pattern[pi][8*(i/16)])=val;
            i=i+16;
            str=str+16;
        }
        if (r.pattern_size[pi] > MIN_PATTERN_SIZE ) {
            r.pattern_type[pi]= PATTERN_SIZE_BIG;
        } else {
            r.pattern_type[pi]= PATTERN_SIZE_NORMAL;
        }

        for (i=0; i<r.pattern_size[pi]; i++) {
            debug(HTX_HE_INFO,DBG_INFO_PRINT,"-%x-",r.pattern[pi][i]);
        }
    }
    else {
        strptr=strtok_r(str,"()",ptr);
        strptr=strtok_r(NULL,"()",ptr);
        if (strptr!=NULL){
            val=atoi(strptr);
               debug(HTX_HE_INFO,DBG_INFO_PRINT,"#fill_pattern_details:In (%s) size of patttern "
                "to be extracted specified as %d\n",strptr,val);
            /* Size of the pattern cannot be specified for RANDOM and ADDRESS patterns*/
            if ((strcmp(r.pattern_name[pi],"RANDOM")==0 )&& (strcmp(r.pattern_name[pi],"ADDRESS")==0)) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:#line %d: %s Size of the pattern cannot"
                        " be specified for RANDOM and ADDRESS patterns \n",__LINE__,__FUNCTION__,*line,str);
                return(1);
                /* Fill the size of the pattern with default value */
            }
            r.pattern_size[pi]=val;
        }
        else {
            if ((strcmp(r.pattern_name[pi],"RANDOM")!=0 )&& (strcmp(r.pattern_name[pi],"ADDRESS")!=0)) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:#line %d: %s pattern's length "
                    "should be specified with the pattern file name \n",__LINE__,__FUNCTION__,*line,str);
                return(1);
                /* Fill the size of the pattern with default value */
            }
            /* If pattern is ADDRESS or RANDOM, size is assigned as 8 */
            r.pattern_size[pi] = MIN_PATTERN_SIZE;
        }
        if ( (r.pattern_size[pi]) & (r.pattern_size[pi]-1)) {
            displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                     "[%d]%s:#line %d: %s pattern's length should be power of "
                     "2 bytes\n",__LINE__,__FUNCTION__, *line, str);
            return(1);
        }
        if (r.pattern_size[pi] > MAX_PATTERN_SIZE ||
            r.pattern_size[pi] < MIN_PATTERN_SIZE) {
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                         "[%d]%s:#line %d: %s pattern has improper pattern size "
                         "(should be 8 <= size <= 4096)\n",__LINE__,__FUNCTION__,*line, str);
                return(1);
        } else {
            if (r.pattern_size[pi] % MIN_PATTERN_SIZE != 0) {
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                         "[%d]%s:#line %d: %s pattern has improper pattern size "
                         "(should multiple of 8 bytes)\n",__LINE__,__FUNCTION__, *line, str);
                return(1);
            }
            r.pattern[pi]=(char *) malloc(r.pattern_size[pi]);
            if (r.pattern[pi] == NULL ) {
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                        "[%d]%s:#line %d: %s pattern value, pattern buffer malloc "
                        "error - %d (%s)\n",__LINE__,__FUNCTION__,*line, str, errno, strerror(errno));
            }
            char* pat_tmp_ptr = getenv("HTXPATTERNS");
            if(pat_tmp_ptr == NULL){
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s(): env variable HTXPATTERNS is not set,pat_tmp_ptr=%p"
                ", thus exiting..\n",__LINE__,__FUNCTION__,pat_tmp_ptr);
                return (-1);

            }else{
                strcpy(buff,pat_tmp_ptr);
            }
            strcat(buff,r.pattern_name[pi]);
            debug(HTX_HE_INFO, DBG_INFO_PRINT,
                     "#fill_pattern_details:In (pattern name = %s) size = %d\n",
                     buff, r.pattern_size[pi]);
            if (strcmp(r.pattern_name[pi],"ADDRESS") != 0 &&
                strcmp(r.pattern_name[pi],"RANDOM") != 0) {
				res = hxfpat(buff, (char *)&r.pattern[pi][0],r.pattern_size[pi]);
                     if(res != 0) {
                    displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                             "[%d]%s:pattern fetching problem -error - %d",__LINE__,__FUNCTION__, res);
                    return(1);
                }
                if (r.pattern_size[pi] > MIN_PATTERN_SIZE ) {
                    r.pattern_type[pi] = PATTERN_SIZE_BIG;
                } else {
                    r.pattern_type[pi] = PATTERN_SIZE_NORMAL;
                }
            } else {
                if (strcmp(r.pattern_name[pi],"ADDRESS") == 0) {
                    *(unsigned long *)r.pattern[pi] = ADDR_PAT_SIGNATURE;
                    r.pattern_type[pi] = PATTERN_ADDRESS;
                } else {
                    *(unsigned long *)r.pattern[pi] = RAND_PAT_SIGNATURE;
                    r.pattern_type[pi] = PATTERN_RANDOM;
                }
            }
        }
    }
    return(0);
}
/*****************************************
*Frees pattern structure allocations	 *
*****************************************/
void free_pattern_buffers()
{
    struct rule_info *stnza_ptr=&g_data.stanza[0];
    int i=0;

    while( strcmp(stnza_ptr->rule_id,"NONE") != 0)
    {
        for (i=0; i< stnza_ptr->num_patterns; i++){
            if ( stnza_ptr->pattern[i]){
                free(stnza_ptr->pattern[i]);
                stnza_ptr->pattern[i]=NULL;
            }
			printf("###########...%d\n",i);
            strcpy(stnza_ptr->pattern_name[i],"\0");
            stnza_ptr->pattern_size[i]=0;
        }
        stnza_ptr++;
    }
}





/*****************************************************************************
*  for each keyword parameter of the rule stanza:                            *
*    1. assign default value to corresponding program variable.              *
*    2. assign specified value to corresponding program variable.            *
*    3. validity check each variable.                                        *
*                                                                            *
*  return code =  0 valid stanza                                             *
*              =  1 invalid stanza                                           *
*              = -1 EOF                                                      *
*****************************************************************************/
int get_rule(int * line, FILE *fp)
{
    char  s[MAX_RULE_LINE_SIZE],buff[500],*pstr=&buff[0],**ptr=&pstr;
    char  keywd[80];
    int   i,j;
    int   keywd_count;         /* keyword count                               */
    int   rc;                  /* return code                                 */
    int   first_line;          /* line number of first keyword in stanza      */


    /* Set the default values in the stanza pointer variable */
    set_defaults();
    keywd_count = 0;
    first_line = *line + 1;
    while ((get_line(s,MAX_RULE_LINE_SIZE,fp)) > 1)
    {
        *line = *line + 1;
        if (s[0] == '*')
            continue;

        for (i=0; s[i]!='\n' && s[i] != '\0'; i++) {
            s[i] = toupper(s[i]);
            if (s[i] == '=')
                s[i] = ' ';
        } /* endfor */

        keywd_count++;
        sscanf(s,"%s",keywd);

        if((strcmp(keywd,"GLOBAL_ALLOC_MEM_PERCENT"))== 0) {
            sscanf(s,"%*s %u",&g_data.gstanza.global_alloc_mem_percent); 
            if((g_data.gstanza.global_alloc_mem_percent<=0) && (g_data.gstanza.global_alloc_mem_percent>= 100)){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s"
                        " must be greater than 0 and less than 100",*line,keywd,r.rule_id);
                exit(1);
            }
        }else if((strcmp(keywd,"GLOBAL_MEM_4K_USE_PERCENT"))== 0) {
            sscanf(s,"%*s %u",&g_data.gstanza.global_mem_4k_use_percent); 
            if((g_data.gstanza.global_mem_4k_use_percent <=0) && (g_data.gstanza.global_mem_4k_use_percent > 100)){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s"
                        " must be greater than 0 and less than equal to 100",*line,keywd,r.rule_id);
                exit(1);
            }
        }else if((strcmp(keywd,"GLOBAL_MEM_64K_USE_PERCENT"))== 0) {
            sscanf(s,"%*s %u",&g_data.gstanza.global_mem_64k_use_percent); 
            if((g_data.gstanza.global_mem_64k_use_percent <=0) && (g_data.gstanza.global_mem_64k_use_percent > 100)){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s"
                        " must be greater than 0 and less than equal to 100",*line,keywd,r.rule_id);
                exit(1);
            }
        }else if((strcmp(keywd,"GLOBAL_ALLOC_MEM_SIZE"))==0){
            sscanf(s,"%*s %ld",&g_data.gstanza.global_alloc_mem_size);
            if(g_data.gstanza.global_alloc_mem_size<=0) {
               displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s"
                    " must be greater than 0",*line,keywd,r.rule_id);
                exit(1); 
            }
        }else if((strcmp(keywd,"GLOBAL_ALLOC_SEGMENT_SIZE"))==0){
            sscanf(s,"%*s %ld",&g_data.gstanza.global_alloc_segment_size);
            if(g_data.gstanza.global_alloc_segment_size <=0) {
               displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s"
                    " must be greater than 0",*line,keywd,r.rule_id);
                exit(1); 
            }
        }else if((strcmp(keywd,"GLOBAL_DISABLE_FILTERS"))==0){
			char bp[20];
			(void) sscanf(s,"%*s %s",bp);
			if (strcmp(bp,"NO") == 0 ) {
				g_data.gstanza.global_disable_filters = 0;
			}else if (strcmp(bp,"YES") == 0) {
				g_data.gstanza.global_disable_filters = 1;
			}else{
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
					"(must be NO or YES)\n",*line, keywd, bp);
                exit(1); 
            }
        }else if((strcmp(keywd,"GLOBAL_ALLOC_HUGE_PAGE"))==0){
			char bp[20];
			(void) sscanf(s,"%*s %s",bp);
			if (strcmp(bp,"NO") == 0 ) {
				g_data.gstanza.global_alloc_huge_page = 0;
			}else if (strcmp(bp,"YES") == 0) {
				g_data.gstanza.global_alloc_huge_page = 1;
			}else{
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
					"(must be NO or YES)\n",*line, keywd, bp);
                exit(1); 
            }
        }else if((strcmp(keywd,"GLOBAL_NUM_THREADS"))== 0) {
			sscanf(s,"%*s %d",&g_data.gstanza.global_num_threads); 
			if((g_data.gstanza.global_num_threads<0) && (g_data.gstanza.global_num_threads>g_data.sys_details.tot_cpus)){
				displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s"
						" must be greater than 0 and less than total cppus=%d",*line,keywd,r.rule_id,g_data.sys_details.tot_cpus);
				exit(1);
			}
        }
        else if((strcmp(keywd,"GLOBAL_DEBUG_LEVEL"))== 0) {
            if(!g_data.standalone){
                sscanf(s,"%*s %d",&g_data.gstanza.global_debug_level); 
                if((g_data.gstanza.global_debug_level<0) && (g_data.gstanza.global_debug_level>4)){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s"
                            " must be greater than 0 and less than 4",*line,keywd,r.rule_id);
                    exit(1);
                }
            }
        }
        else if((strcmp(keywd,"GLOBAL_DISABLE_CPU_BIND"))== 0) {
            char bp[20];
            (void) sscanf(s,"%*s %s",bp);
            if (strcmp(bp,"NO") == 0 ) {
                g_data.gstanza.global_disable_cpu_bind= 0;
            }else if (strcmp(bp,"YES") == 0) {
                g_data.gstanza.global_disable_cpu_bind= 1;
            }
            else {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                         "(must be NO or YES)\n",*line, keywd, bp);

                exit(1);
            }
        }
        else if ((strcmp(keywd,"GLOBAL_STARTUP_DELAY")) == 0)
        {
            sscanf(s, "%*s %d", &g_data.gstanza.global_startup_delay);
            if ((g_data.gstanza.global_startup_delay < 0) || (g_data.gstanza.global_startup_delay > 120)) {
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "line# %d %s = %d "
                        " (startup_delay must be >= 0 and <= 120 seconds)\n",
                         *line, keywd, g_data.gstanza.global_startup_delay);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"RULE_ID")) == 0) {
                sscanf(s,"%*s %s",r.rule_id);
                if ((strlen(r.rule_id)) > 24) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %s"
                            " (must be 8 characters or less)\n",*line,keywd,\
                            r.rule_id);
                    exit(1);
                } /* endif */
        }
        else if ((strcmp(keywd,"PATTERN_ID")) == 0)
        {
            char *tmp_str;
            int i = 0;

            tmp_str=strtok_r(s," \n",ptr);
            while (tmp_str != NULL && i <= 9 ){
                tmp_str=strtok_r(NULL," \n",ptr);
                debug(HTX_HE_INFO,DBG_INFO_PRINT,"#tmp_str (%d) = %s\n",\
                        i,tmp_str);
                if (tmp_str == NULL ) {
                    break;
                }
                /* Ignore RANDOM pattern if AME is enabled as random pattern reduces performance of AME enabled system */
                if ((mem_g.AME_enabled == 1)&& (strcmp(tmp_str, "RANDOM") == 0)) {
                    displaym(HTX_HE_INFO, DBG_MUST_PRINT, "AME is enabled on system, hence ignoring RANDOM pattern as it degrades the performance.\n");
                    continue;
                }
                if ((strlen(tmp_str)) > 66) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %s (must "
                            "be 34 characters or less)\n",s);
                }
                strcpy(r.pattern_name[i],tmp_str);
                if ( fill_pattern_details(i,r.pattern_name[i],line) != 0 ) {
                    exit(1);
                }
                i++;
            }
            if (tmp_str) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %s (more than "
                        "10 patterns cannot be specified in a single rule\n",s);
            }
            if ( i ) {
                r.num_patterns = i;
            }
            else {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line#%d %s (pattern not"
                    " specified properly )\n",*line,s);
                exit(1);
            }

            debug(HTX_HE_INFO,DBG_INFO_PRINT,"num of patterns = %d\n",\
                    r.num_patterns );

            if ((strcmp(r.oper,"DMA")==0)&&(r.num_patterns > 1)){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line#%d %s "
                     "(DMA oper cannot have more than one pattern)\n",*line, s);
                exit(1);
            }

            for (i=0; i<r.num_patterns; i++) {
                debug(HTX_HE_INFO,DBG_INFO_PRINT,"pattern name = %s\n "
                        "pattern_size =%d\npattern =0x",r.pattern_name[i],\
                        r.pattern_size[i]);
                if ((strcmp(r.oper,"DMA")==0) && ((strcmp(r.pattern_name[i],"ADDRESS")==0)\
                        || (strcmp(r.pattern_name[i],"RANDOM")==0))) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line#%d %s = %s "
                     "(DMA oper cannot have ADDRESS or RANDOM pattern test case in "
                     "the rules file %s)\n",*line,keywd,r.pattern_name[i],\
                     g_data.rules_file_name);
                    exit(1);
                }
                for (j=0; j<r.pattern_size[i]; j++) {
                    debug(HTX_HE_INFO,DBG_INFO_PRINT,"%0x",r.pattern[i][j]);
                }
                debug(HTX_HE_INFO,DBG_INFO_PRINT,"\n");
            }

        }/*if (strcmp(keywd,pattern_id... ends */
        else if (strcmp(keywd,"SEED")==0) {
            char seed_str[128];
            (void) sscanf(s,"%*s %s",seed_str);
            if ((strncmp(seed_str,"0X",2) != 0)||( strlen(seed_str) > 18 )) {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s " "(must enter hexadecimal (like 0x456) which is less "
                        "or equal to 8 bytes\n",*line,keywd,seed_str);
            }else { 
                (void) sscanf(seed_str,"0X%lu",&r.seed);
            }        
        }
        else if ((strcmp(keywd,"NUM_OPER")) == 0)
        {
            sscanf(s,"%*s %d", &r.num_oper);
            if (r.num_oper < 1) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        "(must be 1 or greater) \n",*line,keywd,r.num_oper);
                exit(1);
            } /* endif */
        }
        else if (strcmp(keywd,"NUM_THREADS")==0) {
            int num_proc=0;
            sscanf(s,"%*s %d",&r.num_threads);
            num_proc = g_data.sys_details.tot_cpus;/*############### need change, include syscfg call*/
            if ((r.num_threads < 1) || (r.num_threads > num_proc)) {
                     displaym(HTX_HE_INFO,DBG_MUST_PRINT,"line# %d %s = %ld "
                         "(num of threads not in valid range(<%d))\nAdjusting"
                         "it to %d",*line,keywd, r.num_threads,\
                         num_proc,num_proc);
            }
        }
        else if ((strcmp(keywd,"OPER"))== 0)
        {
            int i = 0;
            (void) sscanf(s,"%*s %s",r.oper);
            if (strcmp(r.oper,"MEM") == 0 ) {
                r.operation=OPER_MEM;
            } else if (strcmp(r.oper,"DMA") == 0 ) {
                r.operation=OPER_DMA;
            } else if ( strcmp(r.oper,"RIM") == 0 ) {
                r.operation=OPER_RIM;
            } else if ( strcmp(r.oper,"TLB") == 0 ) {
                r.operation=OPER_TLB;
            } else if ( strcmp(r.oper,"MPSS") == 0) {
                r.operation=OPER_MPSS;
            } else if (strcmp(r.oper, "STRIDE") == 0) {
                r.operation=OPER_STRIDE;
            } else if (strcmp(r.oper, "L4_ROLL") == 0) {
                r.operation=OPER_L4_ROLL;
            } else if (strcmp(r.oper, "MBA") == 0) {
                r.operation=OPER_MBA;
            }
            else {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line# %d %s =%s \
                         (must be MEM/DMA/RIM/TLB/MPSS/STRIDE)\n",*line,\
                         keywd, r.oper);
                  exit(1);
            } /* end else */
            if ((strcmp(r.oper,"DMA")==0) && ((r.num_patterns > 1) || \
                    (strncmp(r.pattern_name[0],"0X",2)==0))){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line#%d %s "
                     "(DMA oper cannot have more than one pattern)\n",*line, s);
                exit(1);
            }
            for (i=0; i<r.num_patterns; i++) {
                if ((strcmp(r.oper,"DMA")==0) && ((strcmp(r.pattern_name[i],"ADDRESS")==0)\
                            || (strcmp(r.pattern_name[i],"RANDOM")==0))) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "line#%d %s = %s "
                             "(DMA oper cannot have ADDRESS or RANDOM pattern test case in "
                             "the rules file %s)\n",*line,keywd,r.oper,\
                             g_data.rules_file_name);
                    exit(1);
                }
            }
        }
        else if ((strcmp(keywd,"DISABLE_CPU_BIND"))== 0)
        {
            char bp[20];
            (void) sscanf(s,"%*s %s",bp);
            if (strcmp(bp,"NO") == 0 ) {
                r.disable_cpu_bind= 0;
            }else if (strcmp(bp,"YES") == 0) {
                r.disable_cpu_bind= 1;
            }
            else {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                         "(must be NO or YES)\n",*line, keywd, bp);
                  exit(1);
            } /* end else */
        }
    else if ((strcmp(keywd,"RANDOM_BIND"))== 0)
       {
           char bm[20];
           (void) sscanf(s,"%*s %s",bm);
             if (strcmp(bm,"NO") == 0 ) {
                 r.random_bind = 0;
             }else if (strcmp(bm,"YES") == 0) {
                 r.random_bind= 1;
             }
             else {
                  displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                          "(must be BIND_RANDOM  or BIND_FIXED)\n",*line, keywd, bm);
                   exit(1);
             } /* end else */
        }
    else if ((strcmp(keywd,"AFFINITY"))== 0)
       {
           char ay[20];
           (void) sscanf(s,"%*s %s",ay);
            if (strcmp(ay,"LOCAL") == 0 ) {
                 r.affinity = LOCAL;
            }else if((strcmp(ay,"REMOTE_CHIP") == 0 )) {
                r.affinity = REMOTE_CHIP;
             }else if((strcmp(ay,"FLOATING") == 0 )) {
                 r.affinity = FLOATING;
             }else if((strcmp(ay,"INTRA_NODE") == 0 )) {
                 r.affinity = INTRA_NODE;
             }else if((strcmp(ay,"INTER_NODE") == 0 )) {
                 r.affinity = INTER_NODE;
             }else {
                  displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                          "(must be any of : LOCAL/REMOTE_CHIP/FLOATING/INTRA_NODE/INTER_NODE)\n",*line, keywd, ay);
                  exit(1);
             } /* end else */
            if((r.affinity==REMOTE_CHIP || r.affinity==INTRA_NODE || r.affinity==INTER_NODE) && (g_data.sys_details.num_chip_mem_pools <= 1)){
                displaym(HTX_HE_INFO,DBG_MUST_PRINT,
                    "partition has %d num of chips, (line# %d %s =%s) need atleast 2 chips,change "
                    "affinity setting  will be overwritten to  LOCAL in %s file.\n",
                    g_data.sys_details.num_chip_mem_pools,*line,keywd,ay,g_data.rules_file_name);
                    r.affinity = LOCAL;
            }
        }

        else if ((strcmp(keywd,"SWITCH_PAT_PER_SEG"))== 0)
        {
            char sp[20];
            (void) sscanf(s,"%*s %s",sp);
            if (strcmp(sp,"NO") == 0 ) {
                r.switch_pat = SW_PAT_OFF;
            }else if (strcmp(sp,"YES") == 0) {
                r.switch_pat = SW_PAT_ON;
            }else if (strcmp(sp,"ALL") == 0) {
                r.switch_pat = SW_PAT_ALL;
            }
            else {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                         "(must be NO or YES)\n",*line, keywd, sp);
                  exit(1);
            } /* end else */
        }
        else if ((strcmp(keywd,"COMPARE"))== 0)
        {
            (void) sscanf(s,"%*s %s",r.compare);
            if (strcmp(r.compare,"NO") == 0 ) {
                r.compare_flag = 0;
            } else if ( strcmp(r.compare,"YES") == 0 ) {
                r.compare_flag = 1;
            }
            else {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s =%s "
                         "(must be NO or YES)\n",*line, keywd, r.compare);
                exit(1);
            } /* end else  */
        }
        else if ((strcmp(keywd,"NUM_WRITES")) == 0)
        {
            sscanf(s,"%*s %d", &r.num_writes);
            if ((r.num_writes < 0) )
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(num_writes should be > 0)\n",*line, keywd, \
                        r.num_writes);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"NUM_READ_ONLY")) == 0)
        {
            sscanf(s,"%*s %d", &r.num_reads);
            if ((r.num_reads < 0) )
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.num_reads);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"NUM_READ_COMP")) == 0)
        {
            sscanf(s,"%*s %d", &r.num_compares);
            if ((r.num_compares < 0) )
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.num_compares);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"MEM_PERCENT")) == 0)
        {
            sscanf(s,"%*s %d",&r.mem_percent);
            if (r.mem_percent < MIN_MEM_PERCENT) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                         "(mem_percent must be >= %d%)\n",*line, keywd,\
                         r.mem_percent, MIN_MEM_PERCENT);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"CPU_PERCENT")) == 0)
        {
            sscanf(s,"%*s %d",&r.cpu_percent);
            if (r.cpu_percent < MIN_CPU_PERCENT) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                         "(mem_percent must be >= %d%)\n",*line, keywd,\
                         r.mem_percent, MIN_CPU_PERCENT);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"WIDTH")) == 0)
        {
            sscanf(s,"%*s %d",&r.width);
            if ((r.width != LS_BYTE) &&
                (r.width != LS_WORD) &&
                (r.width != LS_DWORD)) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d"
                        " (width must be 1, 4, or 8)\n",*line, keywd, r.width);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"DEBUG_LEVEL")) == 0)
        {
            sscanf(s,"%*s %d",&r.debug_level);
            if ((r.debug_level < 0) || (r.debug_level > 3 ) ) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %d "
                        " (debug_level must be >= 0 and <= 3)\n",*line, keywd,\
                        r.debug_level);
                exit(1);
            }              /* endif */
        }
        else if ((strcmp(keywd,"SEG_SIZE_4K")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_4K]);
            if (r.seg_size[PAGE_INDEX_4K] < MIN_SEG_SIZE)
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.seg_size[PAGE_INDEX_4K]);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"SEG_SIZE_64K")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_64K]);
            if (r.seg_size[PAGE_INDEX_64K] < MIN_SEG_SIZE)
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.seg_size[PAGE_INDEX_64K]);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"SEG_SIZE_2M")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_2M]);
            if (r.seg_size[PAGE_INDEX_2M] < MIN_SEG_SIZE)
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.seg_size[PAGE_INDEX_2M]);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"SEG_SIZE_16M")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_16M]);
            if (r.seg_size[PAGE_INDEX_16M] < MIN_SEG_SIZE)
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n",*line, keywd, \
                        r.seg_size[PAGE_INDEX_16M]);
                exit(1);
            }              /* end else */
        }
        else if ((strcmp(keywd,"SEG_SIZE_16G")) == 0)
        {
            sscanf(s,"%*s %ld", &r.seg_size[PAGE_INDEX_16G]);
            if ((unsigned long) r.seg_size[PAGE_INDEX_16G] <
                (unsigned long)(16*GB))
            {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"line# %d %s = %ld "
                        "(segment size not in valid range)\n", *line, keywd,\
                        r.seg_size[PAGE_INDEX_16G]);
                exit(1);
            }              /* end else */
        }
#if 0
             else if ((strcmp(keywd,"NX_PERFORMANCE_DATA")) == 0)
            {
                    sscanf(s,"%*s %s",r.nx_performance_data);
                    if ((strcmp(r.nx_performance_data,"NO")) == 0) {
                        r.nx_perf_flag = 0;
                    }
                    else {
                        r.nx_perf_flag = 1;
                    }
            }
             else if ((strcmp(keywd,"NX_ASYNC")) == 0)
            {
                    sscanf(s,"%*s %s",r.nx_async);
                    if ((strcmp(r.nx_async,"NO")) == 0) {
                        r.nx_async_flag= 0;
                    }
                    else {
                        r.nx_async_flag= 1;
                    }
            }
             else if ((strcmp(keywd,"NX_REMINDER_THREADS_FLAG")) == 0)
            {
                    sscanf(s,"%*s %s",r.nx_reminder_threads_flag);
                    if ((strcmp(r.nx_reminder_threads_flag,"NO")) == 0) {
                    r.nx_rem_th_flag = 0;
                    }
                    else {
                r.nx_rem_th_flag = 1;
                    }
            }
        else if ((strcmp(keywd,"NX_MEM_OPERATIONS"))== 0)
                {
            displaym(HTX_HE_INFO,DBG_INFO_PRINT,"INSIDE NX_MEM_OPERATIONS\n");
            char *chr_ptr, *str1, tmp_buf[100];
            char  *s1, *s2;
            int task = 0, field = 0, val;
            float valf;
            str1 = s;

            while((task < MAX_NX_TASK) && (str1 != NULL)){
                chr_ptr = strsep(&str1, "[");
                if(str1 != NULL) {

                    s1 = strsep(&str1, "]");
                    if(str1 != NULL) {
                        strcpy(tmp_buf, s1);
                        displaym(HTX_HE_INFO,DBG_INFO_PRINT,"s1 = %s \n",s1);
                    }
                    else {
                        displaym(HTX_HE_INFO,DBG_INFO_PRINT,"# task = %d \t tmp_buf  = %s\n",
                        task,tmp_buf);
                        return(-1);
                    }

                }
                else {
                    displaym(HTX_HE_INFO,DBG_INFO_PRINT," before TASK break \n");
                    break;
                }
                s2 = tmp_buf;
                field = 0;
                while(field < MAX_NX_FIELD) {
                    chr_ptr = strsep(&s2, ":");

                    if((s2 == NULL) && (field != 4)) {
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"\n All the fields of the task %d"
                        "should be specified ",task);
                        exit(1);
                }


                switch(field) {
                    case 0: /*In this field we expect only nx_mem_oper name */
                        if( strcmp(chr_ptr,"CDC-CRC") == 0){
                            r.nx_task[task].nx_operation = CDC_CRC;
                        }else
                        if( strcmp(chr_ptr,"CDC") == 0){
                            r.nx_task[task].nx_operation = CDC;
                        }else
                        if( strcmp(chr_ptr,"BYPASS") == 0){
                            r.nx_task[task].nx_operation = BYPASS;
                        }else
                        if( strcmp(chr_ptr,"C-CRC") == 0){
                            r.nx_task[task].nx_operation = C_CRC;
                        }else
                        if( strcmp(chr_ptr,"C-ONLY") == 0){
                            r.nx_task[task].nx_operation = C_ONLY;
                        }else {
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper nx_mem ope"
                            "ration chr_ptr = %s\n",chr_ptr);
                            exit(1);
                        }
                    break;

                    case 1: /* In this field we look for percentage of threads/chip */
                        valf = atof(chr_ptr);
                        if( valf >= 0 && valf <= 100){
                            r.nx_task[task].thread_percent = valf;
                        }
                        else {
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper thread"
                            "percent  thread_percent = %d \t task = %d \t field = %d \t"
                            " line = %s \n",valf,task,field,s);
                            exit(1);
                        }
                    break;

                    case 2: /* This field is for min_buf */
                        val = atoi(chr_ptr);
                        if( val > 0 ){
                            r.nx_task[task].nx_min_buf_size = (unsigned int) val;
                        }
                        else {
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper min_buf_size = %s \t  task = %d \t field = %d \t line = %s \n",
                            chr_ptr,task,field,s);
                            exit(1);
                        }
                    break;
                    case 3:  /* This field is for max_buf_size */
                        val = atoi(chr_ptr);
                        if( val >= r.nx_task[task].nx_min_buf_size){
                            r.nx_task[task].nx_max_buf_size = (unsigned int) val;
                        }
                        else {
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper "
                            "max_buf_size = %s \t task = %d \t field = %d \t line = %s \n"
                            "max_buf_size should be >= min_buf_size",
                            chr_ptr,task,field,s);
                            exit(1);
                        }
                    break;

                    case 4:
                        /* We are here means reached the last field which is chip number */
                        if(strcmp(chr_ptr,"*") == 0){
                            r.nx_task[task].chip = -1;
                            break;
                        }

                        val = atoi(chr_ptr);
                        if( val >= 0 && val < MAX_P7_CHIP ){
                            r.nx_task[task].chip = val;
                        }
                        else {
                            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper "
                            "chip = %s \t task = %d \t field = %d \t line = %s \n",
                            chr_ptr,task,field,s);
                            exit(1);
                        }
                    break;

                    default:
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"specify proper "
                        "values for line  = %s \t task = %d \t field = %d \t line = %s \n",
                        chr_ptr,task,field,s);
                         exit(1);

                    } /* END of switch */

                field++;
                } /* END of field */

            task++;
            r.number_of_nx_task = task; /*saving number of tasks */
            } /* END of while i < 20 */
        /* This is a DEBUG TEST LOOP */
        int d;
        i = task;
        for(task = 0; task < i; task ++){
            d = task;
            displaym(HTX_HE_INFO,DBG_INFO_PRINT,"r.nx_task[%d].nx_operation = %d \t"
            "  r.nx_task[%d].thread_percent = %f \t r.nx_task[%d].nx_min_buf_size = %d \n"
            "r.nx_task[%d].nx_max_buf_size = %d \t \t"
            " r.nx_task[%d].chip = %d \n\n",task,r.nx_task[d].nx_operation,task,r.nx_task[d].thread_percent, \
            task,r.nx_task[d].nx_min_buf_size,task,r.nx_task[d].nx_max_buf_size,task,
            r.nx_task[d].chip );

        }

        }
        else if ((strcmp(keywd,"CORSA_PERFORMANCE")) == 0)
        {
            sscanf(s,"%*s %s",r.corsa_performance);
            if ((strcmp(r.corsa_performance,"NO")) == 0) {
                r.corsa_perf_flag = 0;
            }
            else {
                r.corsa_perf_flag = 1;
            }
        }
#endif
        else if ((strcmp(keywd,"STRIDE_SZ"))== 0)
        {
            sscanf(s,"%*s %d",&r.stride_sz);
            if (r.stride_sz > MAX_STRIDE_SZ) {
                displaym(HTX_HE_INFO, DBG_MUST_PRINT, "Stride size (%d) is greater than MAX_STRIDE_SZ(%d).\n"
                                                      "Seeting it to MAX_STRIDE_SZ.\n",
                                                      r.stride_sz, MAX_STRIDE_SZ);
                r.stride_sz = MAX_STRIDE_SZ;
            }
        }

        else if ((strcmp(keywd,"PERCENT_HW_THREADS"))== 0)
        {
             sscanf(s,"%*s %d",&r.percent_hw_threads);
        }
        else if ((strcmp(keywd,"TLBIE_TEST_CASE"))==0){
            sscanf(s,"%*s %s",r.tlbie_test_case);
        }
        else if ((strcmp(keywd,"CPU_FILTER"))==0){

            char *tmp_str;
            int i = 0;
            tmp_str=strtok_r(s," \n",ptr);
            while (tmp_str != NULL && i <= MAX_FILTERS) {
                tmp_str=strtok_r(NULL," \n",ptr);
                if (tmp_str == NULL ) {
                    break;
                }
                strcpy(r.cpu_filter_str[i],tmp_str);
                debug(HTX_HE_INFO,DBG_INFO_PRINT,"[%d]cpu_filter = %s\n",i,r.cpu_filter_str[i]);
                i++;
            }   
            if (tmp_str) {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:line# %s (more than "
                    "%d filters cannot be specified in a single rule\n",__LINE__,__FUNCTION__,s,MAX_FILTERS);
            }
            if ( i ) {
                 r.num_cpu_filters = i;
            }
            else {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:line#%d %s (pattern not specified properly )\n",__LINE__,__FUNCTION__,*line,s);
            }         
        }
        else if ((strcmp(keywd,"MEM_FILTER"))==0){

            char *tmp_str;
            int i = 0;
            tmp_str=strtok_r(s," \n",ptr);
            while (tmp_str != NULL && i <= MAX_FILTERS) {
                tmp_str=strtok_r(NULL," \n",ptr);
                if (tmp_str == NULL ) {
                    break;
                }
                strcpy(r.mem_filter_str[i],tmp_str);
                debug(HTX_HE_INFO,DBG_INFO_PRINT,"[%d]mem_filter = %s\n",i,r.mem_filter_str[i]);
                i++;
            }   
            if (tmp_str) {
                 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:line# %s (more than "
                    "%d filters cannot be specified in a single rule\n",__LINE__,__FUNCTION__,s,MAX_FILTERS);
            }
            if ( i ) {
                 r.num_mem_filters = i;
            }
            else {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:line#%d %s (pattern not specified properly )\n",__LINE__,__FUNCTION__,*line,s);
            }         
        }
        else
        {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:line# %d keywd = %s "
                    "(invalid)\n",__LINE__,__FUNCTION__,*line,keywd);
            exit(1);
        }
    } /* end while */

    *line = *line + 1;
    if (keywd_count > 0)
    {
        if ((strcmp(r.rule_id,"")) == 0)
        {
             displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT, "[%d]%s:line# %d rule_id not"
                     " specified \n",__LINE__,__FUNCTION__,first_line);
             exit(1);
        }
         else rc=0;
    }
    else rc=EOF;

    if (rc != EOF ) {

		if(((g_data.gstanza.global_disable_filters == 0) || (g_data.sys_details.unbalanced_sys_config == 0)) && (g_data.sys_details.shared_proc_mode != 1)){
            g_data.gstanza.global_disable_filters = 0;
			/* calling cpu/mem  filter parsing modules*/
			rc = parse_cpu_filter(r.cpu_filter_str);
			if(rc != SUCCESS){
				exit(1);
			}
			rc = parse_mem_filter(r.mem_filter_str);
			if(rc != SUCCESS){
				exit(1);
			}
		}
		/* Doing a MEMCOPY .. instead of individual copy*/
		memcpy(g_data.stanza_ptr,&r,sizeof(r));

		displaym(HTX_HE_INFO,DBG_INFO_PRINT,"+All the keywords are read "
				"sucessfully\n");
    }

    return(rc);
}

/****************************************************************
*Reads rule file and updates rule_info structre for evry stanza *
*function will return without parcing rule file,If there is no  *
*change in rule file											*
****************************************************************/
int read_rules(void)
{
    struct stat fstat;
    int i =0;
    int line_number=0;

    if ( -1 ==  stat (g_data.rules_file_name, &fstat)) {
        displaym(HTX_HE_INFO,DBG_MUST_PRINT," [%d]%s: Error occoured while "
                "getting the rules file stats for file %s: errno %d: %s \n",__LINE__,__FUNCTION__,g_data.rules_file_name,errno,strerror(errno));
    }
    else {
        if (g_data.rf_last_mod_time != 0) {
            if (g_data.rf_last_mod_time == fstat.st_mtime) {
                displaym(HTX_HE_INFO,DBG_INFO_PRINT," No changes in the rules "
                        "file stanza so skipping the read again \n");
                return(0);
            }
        }
        if (g_data.rf_last_mod_time) {
            free_pattern_buffers();
        }
        g_data.rf_last_mod_time = fstat.st_mtime;
    } /* else ends */
    if ( (g_data.rf_ptr= fopen(g_data.rules_file_name,"r")) == NULL) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:The rules file name parameter %s to the hxemem64 binary is incorrect,fopen(),(%d)%s\n",
			__LINE__,__FUNCTION__,g_data.rules_file_name,errno,strerror(errno));
        return(-1);
    }

    i = 0;
    /* Initialize global rule parameters here with default values*/
    g_data.gstanza.global_alloc_mem_percent= DEFAULT_MEM_ALOC_PERCENT;
	g_data.gstanza.global_mem_4k_use_percent = 50;
	g_data.gstanza.global_mem_64k_use_percent = 50;
	g_data.gstanza.global_alloc_mem_size= -1;
	g_data.gstanza.global_startup_delay   = DEFAULT_DELAY; 	
	g_data.gstanza.global_disable_filters = 1;
    g_data.gstanza.global_alloc_huge_page = 1;
	g_data.gstanza.global_num_threads = -1;
    if(!g_data.standalone) {
        g_data.gstanza.global_debug_level = 0;
    }
    g_data.gstanza.global_disable_cpu_bind = 0;
    g_data.gstanza.global_alloc_segment_size = -1;

    g_data.stanza_ptr=&g_data.stanza[0];
    while ( (get_rule(&line_number,g_data.rf_ptr)) != EOF ) {
        g_data.stanza_ptr++;
        i++;
        if ( i == MAX_STANZAS-2 ) {
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s:The number of stanzas %d in rule file %s is more than expected (%d)) \n",
				__LINE__,__FUNCTION__,i,g_data.rules_file_name,MAX_STANZAS);
        }
    }

    strcpy(g_data.stanza_ptr->rule_id,"NONE");

    g_data.stanza_ptr=&g_data.stanza[0];
    i = 0;
    while( strcmp(g_data.stanza_ptr->rule_id,"NONE") != 0)
    {
        int pati,patj;
        i++;
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"stanza number #%d,\nrule_id=%s\n",i,g_data.stanza_ptr->rule_id);
        for (pati=0; pati<g_data.stanza_ptr->num_patterns; pati++) {
            debug(HTX_HE_INFO,DBG_INFO_PRINT,"pattern name = %s\n "
                    "pattern_size =%d\npattern =0x",g_data.stanza_ptr->pattern_name[pati],\
                    g_data.stanza_ptr->pattern_size[pati]);
            for (patj=0; patj<r.pattern_size[pati]; patj++) {
                debug(HTX_HE_INFO,DBG_INFO_PRINT,"%0x",r.pattern[pati][patj]);
            }
            debug(HTX_HE_INFO,DBG_INFO_PRINT,"\n");
        }
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->pattern_id=%s\n",g_data.stanza_ptr->pattern_id);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->oper=%s\n",g_data.stanza_ptr->oper);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->compare=%s\n",g_data.stanza_ptr->compare);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->crash_on_mis=%s\n",g_data.stanza_ptr->crash_on_mis);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->turn_attn_on=%s\n",g_data.stanza_ptr->turn_attn_on);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_oper=%d\n",g_data.stanza_ptr->num_oper);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_writes=%d\n",g_data.stanza_ptr->num_writes);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_reads=%d\n",g_data.stanza_ptr->num_reads);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->num_compares=%d\n",g_data.stanza_ptr->num_compares);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->width=%d\n",g_data.stanza_ptr->width);
        debug(HTX_HE_INFO,DBG_INFO_PRINT,"pres_stanza->debug_level=%d\n",g_data.stanza_ptr->debug_level);
        g_data.stanza_ptr++;
    } /* while (strcmp(.... ENDS */

	fclose(g_data.rf_ptr);
     return(0);
} /* read_rules function ends */

