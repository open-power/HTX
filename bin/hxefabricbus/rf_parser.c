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

char msg[100];
extern struct htx_data htx_d;
extern int errno;


int
get_line( char s[], FILE *fp) {

    int c,i;
    i=0;
    while (((c = fgetc(fp)) != EOF) && c != '\n') {
        s[i++] = c;
    }
    if (c == '\n')
        s[i++] = c;

    s[i] = '\0';
    return(i);
}

int
parse_line(char s[]) {

    int len, i = 0, j = 0;

    while(s[i] == ' ' || s[i] == '\t') {
        i++;
    }
    if(s[i] == '*') {
        return(0);
    }
    len = strlen(s);
    for(; i < len && s[i] != '\0'; i++) {
        if (s[i] == '=')
            s[i] = ' ';
        s[j++] = s[i];
    }
    s[j] = '\0';
    return((s[0] == '\n')? 1 : j);
}

static int
str2dec( char str[], unsigned long long * result, unsigned int *len_8) {
    unsigned long long temp;
	unsigned int i; 
    char temp_str[MAX_STANZA_NAME], * actual_string;
	char msg[100]; 
	unsigned int len = strlen(str);
	*len_8 = 0 ; 
	if ((strncmp(str,"0X",2) == 0)||(strncmp(str,"0x",2) == 0)) { 
		actual_string = str + 2; 
		len = len - 2; 
		if(len % 16 != 0) { 
			/* Pattern should be multiple of 8 bytes i.e, 16 characters */
             sprintf(msg,"# %s pattern value should be in multiple of 8 bytes, len=%d \n", actual_string, len);
             hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
             return(-1);
		}
		if( len % 16 > 4 ) { 
			/* Pattern should be max 32 bytes */ 
			sprintf(msg,"# %s pattern value should be max 32bytes, len=%d \n", actual_string, len);
            hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }
		for(i = 0; i < len; i += 16) {
			strncpy(temp_str, actual_string, 16); 
			temp_str[16] = '\0';
			temp = strtoull(temp_str, NULL, 16);
			*result = temp ; 
			DEBUGON("%llx", temp); 
			actual_string +=  16 ;
			result = result + 1;  
			*len_8 = *len_8 + 1; 
		}
	} else { 
		actual_string = str;
		if(len % 16 != 0) {
            /* Pattern should be multiple of 8 bytes i.e, 16 characters */
             sprintf(msg,"# %s pattern value should be in multiple of 8 bytes, len=%d \n", actual_string, len);
             hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
             return(-1);
        }
		if( len % 16 > 4 ) {
            /* Pattern should be max 32 bytes */
            sprintf(msg,"# %s pattern value should be max 32bytes, len=%d \n", actual_string, len);
            hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }

		for(i = 0; i < len; i += 16) {
            strncpy(temp_str, actual_string, 16);
            temp_str[16] = '\0';
            temp = strtoull(temp_str, NULL, 10);
			DEBUGON("%llx", temp); 
			*result = temp ; 
			actual_string += 16 ;
            result = result + 1;
			*len_8 = *len_8 + 1; 
        }
	}	 			
    return 0;
}

/* FUNCTION int fill_pattern_details(int pi , PATTERN * pattern, char * input_pattern)
 *      pi                       : pattern index in the rules stanza
 *     str                       : pattern structure to be filled
 *     input_pattern : pattern being parsed one by one from rules file
 */
int
fill_pattern_details(int pi, PATTERN * pattern, char * input_pattern) {

    char tmp_str[32],buff[128],*pstr=&buff[0],**ptr=&pstr,*strptr;
    int len  = 0, i,res=0;
    unsigned long val = 0;
    DEBUGON("#fill_pattern_details: pi=%d,input_pattern=%s \n",pi,input_pattern);
    strcpy(pattern->pattern_name, input_pattern);
        DEBUGON("#fill_pattern_details: pi=%d, pattern->pattern_name=%s \n", pi, pattern->pattern_name);
    if( (strncmp(input_pattern,"0X",2) == 0)||(strncmp(input_pattern,"0x",2) == 0) ) {
        /* Pattern is specified as a HEX immediate value like 0xFFFFFFFF */
        len=strlen(input_pattern);
        len = len - 2;
        if (len == 0) {
            sprintf(msg,"#fill_pattern_detail HEX pattern=%s cannot be zero sized\n", input_pattern);
            hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(1);
        }
        if ( len%16 != 0 ) {
            /* Pattern should be multiple of 8 bytes i.e, 16 characters */
             sprintf(msg,"#Pattern %d: %s pattern value should be in multiple of 8 bytes \n", pi, input_pattern);
             hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
             return(1);
        }
        pattern->pattern_size=len/2;    /* converting hex string sz into equiv sz in bytes */
        if ( (pattern->pattern_size) & (pattern->pattern_size - 1)) {
            sprintf(msg,"#%s pattern's length should be power of 2 bytes\n", input_pattern);
            hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(1);
        }
                input_pattern = input_pattern + 2 ;
        i = 0;
        while (i < len) {
            strncpy(tmp_str,input_pattern,(2*BIT_PATTERN_WIDTH));
            tmp_str[16]='\0';
            val=strtoul(tmp_str,NULL,16);
            if (val == 0) {
                sprintf(msg,"#Pattern %d: %s pattern is not proper hex value\n", pi, input_pattern);
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(1);
            }
            *(unsigned long *)(&pattern->actual_pattern[8*(i/16)])=val;
            DEBUGON("#fill_pattern_details: val =0x%lx, pattern=0x%lx\n",val, *(unsigned long *)(&pattern->actual_pattern[8*(i/16)]));
            i=i+16;
            input_pattern = input_pattern + 2 ;
        }
        pattern->pattern_type= PATTERN_FIXED;
                DEBUGON("#fill_pattern_details:Input_pattern=%s, pattern_name=%s,pattern_size=%d,pattern_type=%d \n",
                                                        input_pattern,pattern->pattern_name, pattern->pattern_size,  pattern->pattern_type);
    } else {
        strptr=strtok_r(input_pattern,"()",ptr);
        DEBUGON("strptr = %s \n",strptr);
        strcpy(pattern->pattern_name, strptr);
        strptr=strtok_r(NULL,"()",ptr);
        if (strptr!=NULL){
            val=atoi(strptr);
            DEBUGON("#fill_pattern_details:In (%s) size of patttern to be extracted specified as %d\n",strptr,val);
            /* Size of the pattern cannot be specified for ADDRESS patterns*/
            if ((strcmp(pattern->pattern_name,"RANDOM")==0 )&& (strcmp(pattern->pattern_name,"ADDRESS")==0)) {
                sprintf(msg,"#pattern=%d : %s Size of the pattern cannot be specified for ADDRESS patterns \n",pi, input_pattern);
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(1);
            }
            pattern->pattern_size=val;
        } else {
            if ((strcmp(pattern->pattern_name,"RANDOM")!=0 )&& (strcmp(pattern->pattern_name,"ADDRESS")!=0)) {
                sprintf(msg,"#Pattern %d %s pattern's length should be specified with the pattern file name \n",pi, input_pattern);
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(1);
            }
            /* If pattern is ADDRESS size is assigned as MIN_PATTERN_SIZE */
            pattern->pattern_size = MIN_PATTERN_SIZE;
        }
        if ((pattern->pattern_size) & (pattern->pattern_size - 1)) {
            sprintf(msg,"#Pattern %d: %s pattern's length should be power of 2 bytes\n", pi, input_pattern);
            hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(1);
        }
        if (pattern->pattern_size > MAX_PATTERN_SIZE ||
            pattern->pattern_size < MIN_PATTERN_SIZE) {
                sprintf(msg," #Pattern %d: %s pattern has improper pattern size (should be 8 <= size <= 4096)\n", pi, input_pattern);
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(1);
        } else {
            if (pattern->pattern_size % MIN_PATTERN_SIZE != 0) {
                sprintf(msg,"#Pattern %d: %s pattern has improper pattern size (should multiple of 8 bytes)\n", pi, input_pattern);
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(1);
            }

            strcpy(buff,getenv("HTXPATTERNS"));
            strcat(buff,pattern->pattern_name);
            DEBUGON("#fill_pattern_details:In (pattern name = %s) size = %d\n",buff, pattern->pattern_size);
            if ((strcmp(pattern->pattern_name,"RANDOM")!=0 )&& (strcmp(pattern->pattern_name,"ADDRESS")!=0)) {
                if ((hxfpat(buff, (char *)pattern->actual_pattern,pattern->pattern_size)) != 0) {
                    sprintf(msg,"pattern fetching problem -error - %d", res);
                    hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                    return(1);
                }
               pattern->pattern_type = PATTERN_FIXED;
            } else {
                if (strcmp(pattern->pattern_name,"ADDRESS") == 0) {
                    *(unsigned long *)pattern->actual_pattern = ADDR_PAT_SIGNATURE;
                    pattern->pattern_type = PATTERN_ADDRESS;
                } else {
                    *(unsigned long *)pattern->actual_pattern = RAND_PAT_SIGNATURE;
                    pattern->pattern_type = PATTERN_RANDOM;
                }

            }
        }
    }
    return(0);
}

void
set_defaults(struct rf_stanza h_r[], unsigned int pvr) { 
	/*
 	 * This function sets default values for every test case in rule file.
 	 * Following are the defaults :
 	 *
 	 * compare - 1      i.e. compare the results
 	 * crash_on_misc - 1    i.e. drop to KDB incase of miscompare
 	 * num_oper - 1000  i.e. run the WRC threads for 2000 passes
 	 * memory_configure - 0 i.e. use algo for memory mapping
 	 * seed - 0     i.e generated on the run
 	 */
	int i, j;
	memset(h_r, 0, (sizeof(struct rf_stanza) * MAX_TC)); 
	for(i = 0; i < MAX_TC ; i++) { 
		h_r[i].compare = 1;
    	h_r[i].wrc_data_pattern_iterations = 32;
    	h_r[i].wrc_iterations    = 32;
    	h_r[i].copy_iterations   = 32;
    	h_r[i].add_iterations    = 32;
    	h_r[i].daxpy_iterations  = 32;
    	h_r[i].daxpy_scalar      = 2;
    	h_r[i].crash_on_misc     = 1;
    	h_r[i].num_oper          = 8;
    	h_r[i].memory_configure  = 0 ;
    	h_r[i].seed = 0;
		h_r[i].mask_strct.and_mask[0] = 0xFFFFffffFFFFffffull;
		h_r[i].mask_strct.and_mask_len = 1; 	
    	h_r[i].mask_strct.or_mask[0]  = 0x0000000000000000ull;
		h_r[i].mask_strct.or_mask_len = 1 ; 
		h_r[i].randbuf_size = 4 ; 
		h_r[i].threads_per_node = MAX_CPUS;

			
		h_r[i].fixed_pattern.num_patterns = MAX_STANZA_PATTERNS; 
		for(j = 0 ; j < MAX_STANZA_PATTERNS ; j++) {
			/* Set the default patterns to be used */      
			h_r[i].fixed_pattern.pattern[j].pattern_size = BIT_PATTERN_WIDTH ; 
			h_r[i].fixed_pattern.pattern[j].pattern_type = PATTERN_FIXED ; 
		} 
		if(pvr == PVR_POWER6) { 
			strncpy(h_r[i].fixed_pattern.pattern[0].pattern_name,"0xFFFFFFFFFFFFFFFFULL",10);
			*(unsigned long long *)h_r[i].fixed_pattern.pattern[0].actual_pattern = 0xFFFFFFFFFFFFFFFFULL;
			strncpy(h_r[i].fixed_pattern.pattern[1].pattern_name,"0x0000000000000000ULL",10);
			*(unsigned long long *)h_r[i].fixed_pattern.pattern[1].actual_pattern = 0x0000000000000000ULL;
			strncpy(h_r[i].fixed_pattern.pattern[2].pattern_name,"0xAAAA5555AAAA5555ULL",10);
			*(unsigned long long *)h_r[i].fixed_pattern.pattern[2].actual_pattern = 0xAAAA5555AAAA5555ULL;
			strncpy(h_r[i].fixed_pattern.pattern[3].pattern_name,"0x5555AAAA5555AAAAULL",10);
			*(unsigned long long *)h_r[i].fixed_pattern.pattern[3].actual_pattern = 0x5555AAAA5555AAAAULL; 
		} else { 
			strncpy(h_r[i].fixed_pattern.pattern[0].pattern_name,"0x0000000000000000ULL",10);
			*(unsigned long long *)h_r[i].fixed_pattern.pattern[0].actual_pattern = 0x0000000000000000ULL; 
			strncpy(h_r[i].fixed_pattern.pattern[1].pattern_name,"0xFFFFFFFFFFFFFFFFULL",10);
            *(unsigned long long *)h_r[i].fixed_pattern.pattern[1].actual_pattern = 0xFFFFFFFFFFFFFFFFULL;
			strncpy(h_r[i].fixed_pattern.pattern[2].pattern_name,"0x5555555555555555ULL",10);
			*(unsigned long long *)h_r[i].fixed_pattern.pattern[2].actual_pattern = 0x5555555555555555ULL; 
			strncpy(h_r[i].fixed_pattern.pattern[3].pattern_name,"0xAAAAAAAAAAAAAAAAULL",10); 
			*(unsigned long long *)h_r[i].fixed_pattern.pattern[3].actual_pattern = 0xAAAAAAAAAAAAAAAAULL; 
		} 
		strncpy(h_r[i].fixed_pattern.pattern[4].pattern_name,"0xCCCCCCCCCCCCCCCCULL",10);
    	*(unsigned long long *)h_r[i].fixed_pattern.pattern[4].actual_pattern = 0xCCCCCCCCCCCCCCCCULL;
    	strncpy(h_r[i].fixed_pattern.pattern[5].pattern_name,"0x3333333333333333ULL",10);
    	*(unsigned long long *)h_r[i].fixed_pattern.pattern[5].actual_pattern = 0x3333333333333333ULL;
    	strncpy(h_r[i].fixed_pattern.pattern[6].pattern_name,"0x0F0F0F0F0F0F0F0FULL",10);
    	*(unsigned long long *)h_r[i].fixed_pattern.pattern[6].actual_pattern = 0x0F0F0F0F0F0F0F0FULL;
    	strncpy(h_r[i].fixed_pattern.pattern[7].pattern_name,"0x3C3C3C3C3C3C3C3CULL",10);
    	*(unsigned long long *)h_r[i].fixed_pattern.pattern[7].actual_pattern = 0x3C3C3C3C3C3C3C3CULL;
    	strncpy(h_r[i].fixed_pattern.pattern[8].pattern_name,"0x5A5A5A5A5A5A5A5AULL",10);
    	*(unsigned long long *)h_r[i].fixed_pattern.pattern[8].actual_pattern  = 0x5A5A5A5A5A5A5A5AULL;
			
	} 
}	
	 
	
 

int
rf_read_rules(const char rf_name[], struct rf_stanza rf_info[], unsigned int * num_stanzas, unsigned int pvr) {

    char line[200], keywd[200];
    struct rf_stanza * current_ruleptr = NULL;
    int eof_flag = 0, num_tc = 0, keyword_match, rc, change_tc = 1, i;
	FILE *rf_ptr;

	if ( (rf_ptr = fopen(rf_name, "r")) == NULL ) {
    	sprintf(msg,"error open %s ",rf_name);
    	hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg);
    	return(-1);
   	}
	set_defaults(rf_info, pvr);

    current_ruleptr = &rf_info[0];
    do {
        rc = get_line(line, rf_ptr);
        /*
         * rc = 0 indicates End of File.
         * rc = 1 indicates only '\n' (newline char) on the line.
         * rc > 1 more characters.
         */
        if(rc == 0) {
            eof_flag = 1;
            break;
        }
        DEBUGON("\n%s: line - %s rc - %d",__FUNCTION__, line, rc);
        /*
         * rc = 1 indicates a newline which means end of current testcase.
         */
        if(rc == 1) {
            change_tc = 1;
            continue;
        }
        /*
         * rc = 0 indicates comment line in rule file.
         * rc = 1 indicates some white spaces and newline.
         * rc > 1 indicates there may be valid test case parameter.
         */
        rc = parse_line(line);
        DEBUGON("\n%s: line - %s rc - %d",__FUNCTION__, line, rc);
        if(rc == 0 || rc == 1) {
            if(rc == 1)
                change_tc = 1;
                continue;
        } else {
	        if(rc > 1 && change_tc == 1) {
                current_ruleptr = &rf_info[num_tc];
   		        if(num_tc >= MAX_TC) {
        	        sprintf(msg,"\n Max num of test cases allowed are %d", MAX_TC);
            	    hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                	return(-1);
            	}
            	num_tc++;
            	DEBUGON("\n Next rule - %d", num_tc);
            	change_tc = 0;
        	}
        }
        sscanf(line,"%s",keywd);
        DEBUGON("\n%s: keywd - %s",__FUNCTION__, keywd);
        if ( (strcmp(keywd, "rule_id")) == 0 ) {
            char tmp[30];
            sscanf(line, "%*s %s", tmp);
            if (((strlen(tmp)) < 1) || ((strlen(tmp)) > 19) ) {
                sprintf(msg, "\n rule_id string (%s) length should be in the range"
                         " 1 < length < 19", current_ruleptr->rule_id);
                hxfmsg(&htx_d, 0, HTX_HE_SOFT_ERROR, msg);
                strncpy(current_ruleptr->rule_id, tmp, 19);
            } else {
                strcpy(current_ruleptr->rule_id, tmp);
                DEBUGON("\n rule_id - %s", current_ruleptr->rule_id);
            }
        } else if ((strcmp(keywd, "compare")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->compare);
            DEBUGON("\n compare - %d", current_ruleptr->compare);
            if (current_ruleptr->compare != 0 && current_ruleptr->compare != 1) {
                sprintf(msg, "\n for 'compare' possible values are 0 or 1");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                DEBUGON("\n compare - %d", current_ruleptr->compare);
            }
        } else if ((strcmp(keywd, "crash_on_misc")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->crash_on_misc);
            DEBUGON("\n crash_on_misc - %d", current_ruleptr->crash_on_misc);
            if (current_ruleptr->crash_on_misc < -1) {
                sprintf(msg, "\n crash_on_misc must be either 0 or 1");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                DEBUGON("\n crash_on_misc - %d", current_ruleptr->crash_on_misc);
            }
        } else if ((strcmp(keywd, "num_oper")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->num_oper);
        	DEBUGON("\n num_oper - %d", current_ruleptr->num_oper);
            if (current_ruleptr->num_oper < -1) {
                sprintf(msg, "\n num_oper must be either 0 or +ve integer");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                DEBUGON("\n num_oper - %d", current_ruleptr->num_oper);
            }
        } else if ((strcmp(keywd, "threads_per_node")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->threads_per_node);
            DEBUGON("\n threads_per_node - %d", current_ruleptr->threads_per_node);
            if (current_ruleptr->threads_per_node < 1 || current_ruleptr->threads_per_node > MAX_SMT_THREAD_PER_CHIP) {
                sprintf(msg, "\n threads_per_node should be b/w 1, 2, 4, 8, 16 & 32 ");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                DEBUGON("\n threads_per_node - %d ", current_ruleptr->threads_per_node);
            }
        } else if ((strcmp(keywd, "memory_allocation")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->memory_allocation);
            DEBUGON("\n memory_allocation - %d", current_ruleptr->memory_allocation);
            if (current_ruleptr->memory_allocation < 0 || current_ruleptr->memory_allocation > 4) {
                sprintf(msg, "\n memory_allocation should be 0,1,2 3 or 4");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                DEBUGON("\n memory_allocation - %d", current_ruleptr->memory_allocation);
            }
        } else if((strcmp(keywd, "memory_configure")) == 0) {
            sscanf(line, "%*s %d", &current_ruleptr->memory_configure);
            DEBUGON("\n Memory Configure = %d \n",current_ruleptr->memory_configure);
            if( current_ruleptr->memory_configure != 0 && current_ruleptr->memory_configure != 1 ) {
                sprintf(msg,"\n memory_configure values are either 0 or 1");
                hxfmsg(&htx_d,0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
               DEBUGON(" \n Memory Configure - %d ",current_ruleptr->memory_configure);
            }
		} else if((strcmp(keywd, "mask_configure")) == 0) { 
			sscanf(line, "%*s %d", &current_ruleptr->mask_configure); 
			DEBUGON("\n Mask Configure = %d \n", current_ruleptr->mask_configure);
			if( current_ruleptr->mask_configure != 0 && current_ruleptr->mask_configure != 1 ) {
				sprintf(msg,"\n mask configure values are either 0 or 1"); 
				hxfmsg(&htx_d,0, HTX_HE_HARD_ERROR, msg);
				return(-1);
			} else {	
				DEBUGON("\n Mask Configure = %d \n", current_ruleptr->mask_configure);
			}	
        } else if ((strcmp(keywd, "cec_nodes")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->cec_nodes);
            if(current_ruleptr->cec_nodes < 1) {
                sprintf(msg,"\n cec_nodes = %d illegal value !!!!", current_ruleptr->cec_nodes);
                hxfmsg(&htx_d,0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                DEBUGON("\n cec_nodes - %d",current_ruleptr->cec_nodes);
            }
        } else if ((strcmp(keywd, "chips_per_node")) == 0) {
       		sscanf(line, "%*s %d",&current_ruleptr->chips_per_node);
            if(current_ruleptr->chips_per_node < 1 ) {
                sprintf(msg,"\n chips_per_node = %d illegal value !!!!", current_ruleptr->chips_per_node);
                hxfmsg(&htx_d,0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                DEBUGON("\nchips_per_node - %d", current_ruleptr->chips_per_node);
            }
        } else if ((strcmp(keywd, "cores_per_chip")) == 0) {
            sscanf(line, "%*s %d",&current_ruleptr->cores_per_chip);
            if(current_ruleptr->cores_per_chip < 1) {
                sprintf(msg,"\n cores_per_chip= %d illegal value !!!!", current_ruleptr->cores_per_chip);
                hxfmsg(&htx_d,0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            } else {
                DEBUGON("\nchips_per_node - %d", current_ruleptr->cores_per_chip);
            }
        } else if ((strcmp(keywd, "pattern_id")) == 0) {
           char * tmp_str;
           int i = 0;
           char buff[500],*pstr=&buff[0],**ptr=&pstr;
           tmp_str=strtok_r(line," \n",ptr);
           while (tmp_str != NULL && i <= 9 ){
                tmp_str=strtok_r(NULL," \n",ptr);
                int j=0,len,k=0;
                DEBUGON("\n PATTERN #tmp_str (%d) = %s\n",i,tmp_str);
                if (tmp_str == NULL ) {
                    break;
                }
                /* If we have leading white spaces in the token remove then *
                 * Cases like : pattern_id =       HXEFF(128)               *
                 */
                while(tmp_str[j] == ' ' || tmp_str[j] == '\t') {
                    j++;
                }
                len = strlen(tmp_str);
                for(; j < len && tmp_str[j] != '\0'; j++) {
                    tmp_str[k++] = tmp_str[j];
                }
                tmp_str[k] = '\0';
                DEBUGON("PATTERN #tmp_str (%d) = %s\n",i,tmp_str);
                if ((strlen(tmp_str)) > 66) {  /*  32 *2 + 1 byte for '0x' + 1 byte for null char */
                    sprintf(msg,"line# %s (must be 34 characters or less)\n",line);
                    hxfmsg(&htx_d,0, HTX_HE_HARD_ERROR, msg);
                    return(-1);
                }
                if ( fill_pattern_details(i, &current_ruleptr->fixed_pattern.pattern[i], tmp_str) != 0 ) {
                    return(-1);
                }
                i++ ;
            }
            if (tmp_str) {
                sprintf(msg," line# %s (more than 9 patterns cannot be specified in a single rule\n",line);
                hxfmsg(&htx_d,0, HTX_HE_HARD_ERROR, msg);
            }
            if ( i ) {
                current_ruleptr->fixed_pattern.num_patterns = i;
            } else {
                sprintf(msg," line# %s(pattern not specified properly )\n",line);
                hxfmsg(&htx_d,0, HTX_HE_HARD_ERROR, msg);
            }
            DEBUGON("\n Num of patterns = %d \n",current_ruleptr->fixed_pattern.num_patterns);
        } else if ((strcmp(keywd, "wrc_iterations")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->wrc_iterations);
            DEBUGON("\n wrc_iterations - %d", current_ruleptr->wrc_iterations);
            if (current_ruleptr->wrc_iterations < 0) {
                sprintf(msg, "\n wrc_iterations must be either 0 or +ve integer");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            }
        } else if((strcmp(keywd,"wrc_data_pattern_iterations")) == 0) {
            sscanf(line, "%*s %d", &current_ruleptr->wrc_data_pattern_iterations);
            DEBUGON("\n wrc_data_pattern_iterations - %d",current_ruleptr->wrc_data_pattern_iterations);
            if (current_ruleptr->wrc_data_pattern_iterations < 0) {
                sprintf(msg, "\n wrc_data_pattern_iterations must be either 0 or +ve integer");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            }
        } else if ((strcmp(keywd, "copy_iterations")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->copy_iterations);
            DEBUGON("\n copy_iterations - %d", current_ruleptr->copy_iterations);
            if (current_ruleptr->copy_iterations < 0) {
                sprintf(msg, "\n copy_iterations must be either 0 or +ve integer");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            }
        } else if ((strcmp(keywd, "add_iterations")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->add_iterations);
            DEBUGON("\n add_iterations - %d", current_ruleptr->add_iterations);
            if (current_ruleptr->add_iterations < 0) {
                sprintf(msg, "\n add_iterations must be either 0 or +ve integer");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            }
        } else if ((strcmp(keywd, "daxpy_iterations")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->daxpy_iterations);
            DEBUGON("\n daxpy_iterations - %d", current_ruleptr->daxpy_iterations);
            if (current_ruleptr->daxpy_iterations < 0) {
            	sprintf(msg, "\n daxpy_iterations must be either 0 or +ve integer");
            	hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            	return(-1);
            }
        } else if ((strcmp(keywd, "daxpy_scalar")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->daxpy_scalar);
            DEBUGON("\n daxpy_scalar - %d", current_ruleptr->daxpy_scalar);
            if (current_ruleptr->daxpy_scalar < 0) {
                sprintf(msg, "\n daxpy_scalar must be either 0 or +ve integer");
                hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                return(-1);
            }
        } else if ((strcmp(keywd, "seed")) == 0 ) {
            sscanf(line, "%*s %d", &current_ruleptr->seed);
            DEBUGON("\n Seed - %x", current_ruleptr->seed);
		} else if((strcmp(keywd, "random_buffer_size")) == 0) { 
			sscanf(line, "%*s %d", &current_ruleptr->randbuf_size); 
			DEBUGON("\n Random Buffer Size - %d \n", current_ruleptr->randbuf_size); 
			if(current_ruleptr->randbuf_size < 0) { 
				sprintf(msg, "\n Random Buffer Size must be greater thn 0, %d \n", current_ruleptr->randbuf_size);
				hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
				return(-1);
			}
		} else if(( strcmp(keywd, "and_mask")) == 0) {
        	char temp[64];
        	sscanf(line, "%*s %s",temp);
        	DEBUGON("\n and_mask String = %s \n",temp);
        	rc = str2dec(temp, current_ruleptr->mask_strct.and_mask, &current_ruleptr->mask_strct.and_mask_len);
        	if(rc) {
            	sprintf(msg, "keywd# %s Improper format specified !!!!",keywd);
            	hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            	return(-1);
        	}
			DEBUGON("\n Len = %d,and_mask =",current_ruleptr->mask_strct.and_mask_len);
			for(i = 0; i < current_ruleptr->mask_strct.and_mask_len; i ++) { 
				DEBUGON("%llx",current_ruleptr->mask_strct.and_mask[i]); 
			} 
			current_ruleptr->mask_strct.host_cpu = -1; /* Apply to All */ 
    	} else if(( strcmp(keywd, "or_mask")) == 0 ) { 
        	char temp[64];
        	sscanf(line, "%*s %s",temp);
        	rc = str2dec(temp, current_ruleptr->mask_strct.or_mask, &current_ruleptr->mask_strct.or_mask_len);
        	if(rc) {
            	sprintf(msg, "keywd# %s Improper format specified !!!!",keywd);
            	hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            	return(-1);
        	}
			DEBUGON("\n Len = %d,or_mask = %llx", current_ruleptr->mask_strct.or_mask_len);
			for(i = 0; i < current_ruleptr->mask_strct.or_mask_len; i ++) { 
				DEBUGON("%llx",current_ruleptr->mask_strct.or_mask[i]); 
			}
			current_ruleptr->mask_strct.host_cpu = -1; /* Apply to All */
        } else {
            sprintf(msg, "\n Wrong keyword - %s specified in rule file. Exiting !!\n", keywd);
            hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
            return(-1);
        }

    } while(eof_flag == 0);
    *num_stanzas = num_tc;
	fclose(rf_ptr); 
    return(0);
}



int rf_close(FILE * rule_file_ptr) {

    int rc = 0;
    rc = fclose(rule_file_ptr);
    if (rc == EOF) {
        sprintf(msg, "file close failed \n");
        hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
        return(-1);
    }
    return rc;
}

int
user_def_memory_mapping(int mem_alloc, unsigned int * num_cpus_mapped, unsigned int memory_mapping[][2]) {

    FILE *fptr;
    int rc,eof_flag = 0 ;
    char line[200], file_mem[256];
    int node, host_cpu= -1, dest_cpu = -1, cpus_mapped = 0;
    int cpu_index[MAX_CHIPS] ={0};

	strcpy(file_mem,htx_d.htx_log_dir);
    sprintf(file_mem,"%s/fabricbus_mem_config_%d",file_mem,mem_alloc);
    if ((fptr = fopen(file_mem,"r" )) == NULL) {
        sprintf(msg,"error open %s ",file_mem );
        hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg);
        return(-1);
    }
    printf("READING FROM FILE : %s \n",file_mem ) ;
    do {
        char * val;
        char * chr_ptr ;
        rc = get_line(line,fptr);
        /*
         * rc = 0 indicates End of File.
         * rc = 1 indicates only '\n' (newline char) on the line.
         * rc > 1 more characters.
         */
        if(rc == 1) continue;
        if(rc == 0) {
            eof_flag == 1 ;
            break ;
		}	 
      	printf("\n line = %s rc = %d ", line, rc);

        rc = parse_line(line);
        printf("Line - %s rc - %d ",line,rc);
       if(rc == 0 || rc == 1) {
            continue;
        }
        val = line ;
        while (val != NULL) {
            char *node_chr_ptr,*temp_chr_ptr,*host_cpu_ptr ;
            chr_ptr = strsep(&val,"]");
            printf("LEFT OVER VALUE = %s \n",val);

            if(chr_ptr != NULL) {
                temp_chr_ptr = strsep(&chr_ptr,"[");
                printf("BLOCK TO PROCESSED : chr_ptr - %s \n",chr_ptr);
                errno = 0;
                if(chr_ptr != NULL) {
                	host_cpu_ptr=strsep(&chr_ptr,",");
                	host_cpu=atoi(host_cpu_ptr);
                	if(errno) {
                    	sprintf(msg, "On line#: %s, atoi failed with %d", line, errno);
                    	hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                		return(-1);
                	}
                	dest_cpu = atoi(chr_ptr);
                	if(errno) {
               	 		sprintf(msg, "On line#: %s, atoi failed with %d", line, errno);
                    	hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                    	return(-1);
                	}
                	printf(" host_cpu=%d, dest_cpu=%d \n",host_cpu,dest_cpu);
                	memory_mapping[cpus_mapped][HOST_CPU] = host_cpu;
                	memory_mapping[cpus_mapped][DEST_CPU] = dest_cpu;
                	printf("%d : Host : %d, dest : %d \n", cpus_mapped, memory_mapping[cpus_mapped][HOST_CPU], memory_mapping[cpus_mapped][DEST_CPU]);
                	cpus_mapped++;
            	} else {
                	sprintf(msg,"\n Improper format in line# %s. Couldnt find ',' char.", line);
                	hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
               	 	return(-1);
            	}
            } else {
                sprintf(msg,"\n Improper format in line# %s. Couldnt find ']' char.", line);
                hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
                return(-1);
            }
            char * temp_ptr=strsep(&val,";");
        }
        printf("\n **********************************************************\n");
    }while(eof_flag == 0);


    *num_cpus_mapped = cpus_mapped;
    fclose(fptr);
    return 0;
}

void 
chomp( char tmp_str[]) { 

	int k = 0, j = 0; 
	int len; 

	while(tmp_str[j] == ' ' || tmp_str[j] == '\t') {
		j++;
	}
	len = strlen(tmp_str);
	for(; j < len && tmp_str[j] != '\0'; j++) {
		if(tmp_str[j] == ' ' || tmp_str[j] == '\t') 
			continue;
		tmp_str[k++] = tmp_str[j];
	}
	tmp_str[k] = '\0';

}

int
user_def_mask_mapping(unsigned int mem_alloc, unsigned int num_cpus_mapped, unsigned int pvr, MASK_STRUCT masks[]) { 

	FILE *fptr; 
	int rc, eof_flag = 0, i ;
	char line[200], file_mask[256];
	char * host_ptr, * dest_ptr, * and_mask_ptr, *or_mask_ptr; 
	unsigned int host, dest; 
	unsigned long long and_mask[4], or_mask[4];
	char * temp = NULL; 

	strcpy(file_mask,htx_d.htx_log_dir);
	sprintf(file_mask,"%s/fabricbus_masks_%d",file_mask,mem_alloc);
	
	if ((fptr = fopen(file_mask, "r" )) == NULL) {
        sprintf(msg,"error open %s ",file_mask );
        hxfmsg(&htx_d, errno, HTX_HE_HARD_ERROR, msg);
        return(-1);
    }

	do {
		char * val;
        char * chr_ptr ;
		rc = get_line(line,fptr);
		/*
         * rc = 0 indicates End of File.
         * rc = 1 indicates only '\n' (newline char) on the line.
         * rc > 1 more characters.
         */
		if(rc == 1) continue;
        if(rc == 0) {
            eof_flag == 1 ;
            break ;
        }
		
		rc = parse_line(line);
       	if(rc == 0 || rc == 1) {
            continue;
        }
		DEBUGON("Line# %s \n", line);
		val = line; 
		temp = strsep(&val, "]"); 
		if(temp != NULL) { 
			char * temp_ptr= NULL; 
			temp_ptr = strsep(&temp, "["); 
			if(temp != NULL) { 
				host_ptr=strsep(&temp,",");
				chomp(host_ptr); 
				if((strncmp(host_ptr, "HOST", 4) == 0))  { 
					DEBUGON("host_ptr = %s \n", host_ptr); 
					continue; 
				} else { 
					host = atoi(host_ptr);
				} 	 
				if(temp != NULL)  { 
					dest_ptr = strsep(&temp,",");
					chomp(dest_ptr);
					if(strncmp(dest_ptr, "DEST", 4) == 0) { 
						DEBUGON("dest_ptr = %s \n", dest_ptr); 
						continue; 
					} else { 
						dest = atoi(dest_ptr);
					}
					if(temp != NULL) { 
						and_mask_ptr = strsep(&temp, ","); 
						chomp(and_mask_ptr);
						if(temp != NULL) { 
							or_mask_ptr =  temp; 
							chomp(or_mask_ptr);
						} else { 
							sprintf(msg, "Improper Format = %s, OR MAsk not specified. \n", line); 
							hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg); 
							return(-1);  	
						}
					} else { 
						sprintf(msg, "Improper Format = %s, And Mask not specified \n", line); 
						hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg); 
						return(-1);	
					}
				} else { 
					sprintf(msg, "Improper Format = %s, Dest Cpu not specified \n", line); 
					hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg);
					return(-1); 
				} 
			} else { 
				sprintf(msg, "Improper Format = %s, Host Cpu Not specified \n", line); 
				hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg); 
				return(-1); 
			} 
		} else { 
			sprintf(msg, "Improper Format = %s, [ not found \n", line); 
			hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg); 
			return(-1); 
		} 
		/* No error in parsing, populate my data mask data structure */ 
		if(host == NO_CPU_DEFINED || dest == NO_CPU_DEFINED) { 
			sprintf(msg, "Illegal Value for host = %d, dest = %d specified \n", host,dest); 
			hxfmsg(&htx_d, -1, HTX_HE_HARD_ERROR, msg); 
			return(-1);
        }
		for(i = 0; i < num_cpus_mapped; i++) { 
			if((get_physical_number(masks[i].host_cpu, mem_alloc, pvr) == host) && 
				(get_physical_number(masks[i].dest_cpu, mem_alloc, pvr) == dest)) { 
				DEBUGON("i = %d, host = %d, dest = %d, masks[i].host_cpu = %d, masks[i].dest_cpu = %d \n", i, host, dest, masks[i].host_cpu, masks[i].dest_cpu); 
				DEBUGON("and_mask = %s, len = %d, or_mask = %s, len = %d \n", and_mask_ptr, strlen(and_mask_ptr), or_mask_ptr, strlen(or_mask_ptr)); 
				rc = str2dec(and_mask_ptr, &masks[i].and_mask[0], &masks[i].and_mask_len); 	
				if(rc) {
	                sprintf(msg, "keywd# %s Improper format specified !!!!", and_mask_ptr);
    	            hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
        	        return(-1);
            	}
				rc = str2dec(or_mask_ptr, &masks[i].or_mask[0], &masks[i].or_mask_len); 
				if(rc) {
                    sprintf(msg, "keywd# %s Improper format specified !!!!", or_mask_ptr);
                    hxfmsg(&htx_d, 0, HTX_HE_HARD_ERROR, msg);
                    return(-1);
                }
			} 
		} 
	} while(eof_flag == 0); 		

	fclose(fptr);	
} 
