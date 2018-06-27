static char sccsid[] = "@(#)71	1.1  src/htx/usr/lpp/htx/bin/hxecapi_afu_dir/get_rule.c, exer_capi, htxrhel7 10/20/16 01:09:59";
/****************************************************************************
*File Name:            get_rule_capi.c
*File Description:     Contains code to parse rule file.
****************************************************************************/

#include "get_rule.h"

/****************************************************************************
*Function Name:        parse_line
*Function Description: This routine takes specified string as input.  
*                      It returns what pattern of string it is.
*Function arguments:   string
*Return Value:         0 indicates comment line in rule file.
*                      1 indicates some white spaces and newline.
*                      Otherwise, indicates there may be valid test case 
*                      parameter.
****************************************************************************/

static int parse_line(char s[])
{
    int		len;		/* Length of a string */
    int 	i = 0;		/* Loop count variable */
    int 	j = 0;		/* Loop count variable */

    while(s[i] == ' ' || s[i] == '\t') {
        i++;
    }
    if(s[i] == '*') {
        return(0);
    }
    len = strlen(s);
    for(; i < len && s[i] != '\0'; i++) {
        s[j++] = s[i];
    }
    s[j] = '\0';
    return((s[0] == '\n')? 1 : j);
}

/****************************************************************************
*Function Name:        get_line
*Function Description: This routine reads a line into the      
*                      specified string.  It returns the length of the      
*                      string.  If the length is 1 the line is blank.  When 
*                      it reaches EOF the length is set to 0,               
*Function arguments:   rule file descriptor, string, length
*Return Value:         0 to indicate EOF. 
*                      1 to indicate blank line.
*                      Otherwise, the length of the line.
****************************************************************************/

static int get_line(FILE *fd, char *s, int lim)
{
    int		c;                      /* input character                  */
    int     i;                      /* array index                      */

    i = 0;                          /* set array index to 0             */
    while ((--lim > 0) && ((c = getc(fd)) != EOF) && (c != '\n')) {
        s[i++] = c;                 /* copy char to array               */
    }                               /* endwhile                         */
    if (c == '\n') {                /* newline character?               */
        s[i++] = c;                 /* copy char to array               */
    }                               /* endif                            */
    s[i] = '\0';                    /* copy string terminator to array  */
    return (i);                     /* return number of chars in line   */
}                                   /* get_line()                       */

/****************************************************************************
*Function Name:        SetDefaults
*Function Description: Sets default rule info structure paramenters
*Function arguments:   Rule info structure
*Return Value:         void
****************************************************************************/

static void SetDefaults(struct rule_info *rule_ptr)
{
    strcpy(rule_ptr->rule_id,DEFAULT_RULE_ID);
    rule_ptr->testcase     =    DEFAULT_TEST_CASE;
    rule_ptr->bufsize       =    DEFAULT_BUFVAL_VALUE;
    rule_ptr->compare      =    DEFAULT_CMP_VALUE;
    rule_ptr->num_threads  =    DEFAULT_NUM_THREADS;
	rule_ptr->num_oper	   =	DEFAULT_NUM_OPER;
}

/****************************************************************************
*Function Name:        get_rule_capi
*Function Description: Rule file parsing function
*Function arguments:   Rule file path
*Return Value:         Success or failure
****************************************************************************/

int get_rule_capi(struct htx_data * htx_d, char rules_file_name[], uint32_t * num_stanza)
{
    char   		s[MAX_STRING];         			/* String received */
    char   		keywd[MAX_STRING];  			/* Rule file keywords*/
    char   		temp[MAX_STRING];       		/* Temporary Variable */
    char   		error = 'n';    				/* Error track */
	char		msg[MSG_TEXT_SIZE];				/* htx message */
    int    		keywd_count;    				/* Number of keywords read*/
    int    		i = 0;          				/* Loop count */
    int    		j = 0;          				/* Loop count */
	int    		rc;             				/* Return value */
	int    		flag = -1;      				/* Check for new stanza */
    static int 	line = 0;						/* Rule file line count */
    FILE   		*fptr;          				/* file pointer */

    /* Opening capi rule file */ 
	if ( (fptr = fopen(rules_file_name, "r")) == NULL ) {
        sprintf(msg, "Error opening %s file\n", rules_file_name);
        hxfmsg(htx_d, errno, HTX_HE_SOFT_ERROR, msg);
        return -1;
    }
    
    keywd_count = 0;

    /* Reads rule file data repeatedly until EOF */
    while ( get_line(fptr, s, 200) >= 1) {
        line += 1;
        /*
         * rc = 0 indicates End of File.
         * rc = 1 indicates only '\n' (newline char) on the line.
         * rc > 1 more characters.
         */

        rc = parse_line(s);
        /*
         * rc = 0 indicates comment line in rule file.
         * rc = 1 indicates some white spaces and newline.
         * rc > 1 indicates there may be valid test case parameter.
         */

        if(rc == 0) {
            continue;
        } else if(rc == 1) {
            if(flag == 0) {         /* Checks for new stanza */
				keywd_count = 0;    /* New Stanza */
                i++;                /* Increment cnt for new stanza */
            }
            flag = 1;
            continue;
        } else {
            if ( s[0] != '*' ) { /* '*' represents comments in rule file */
                for ( j = 0; s[j] != '\n'; j++ ) { 
    				/* Formate string
	                 * Eg: Input String:  testcase = COPY 
    	             *     Output String: TESTCASE   COPY
        	         */
                    s[j] = toupper(s[j]);
                    if (s[j] == '=' ) {
                        s[j] = ' ';
                    }
                }
				/* 
				 * Start of a new stanza, Assigns default values to 
				 * rule_info structure 
				 */
                if ( keywd_count == 0 ) {  
            		flag = 0;			
                    SetDefaults(&rule_data[i]); 
    				(* num_stanza)++; 	/* Total Number of Stanzas */
                }
    
                keywd_count++;

                sscanf(s, "%s", keywd);
                /* 
				 * If tag match is found update rule_info structure 
				 */
                if ( (strcmp(keywd, RULE_TAG)) == 0 ) {
                    sscanf(s, "%*s %s", rule_data[i].rule_id) ; 
					if(strlen(rule_data[i].rule_id) < 0 || strlen(rule_data[i].rule_id) > MAX_STRING) { 
                    	sprintf(msg, "line# %d %s - Rule_id should be less than %d. ", line, keywd, MAX_STRING); 
                        hxfmsg(htx_d, 0, HTX_HE_SOFT_ERROR, msg);
						error = 'y';
					}
		    	} else if ( (strcmp(keywd, TESTCASE_TAG)) == 0 ) {
                    sscanf(s, "%*s %s", temp); 
	                if ((strcmp(temp, "COPY")) == 0 ) {
    	            	rule_data[i].testcase = MEMCOPY_COMMAND_COPY;
        	        } else if ((strcmp(temp, "INTERRUPT")) == 0 ) {
            	    	rule_data[i].testcase = MEMCOPY_COMMAND_INTERRUPT;
                    } else {
                    	sprintf(msg, "line# %d %s = %s (must be COPY or INTERRUPT or COPY_ON_CACHELINE) \n", line, keywd, temp);
	                    hxfmsg(htx_d, 0, HTX_HE_SOFT_ERROR, msg);
    	                error = 'y';
                    }
         		} else if ( (strcmp(keywd, BUFVAL_TAG)) == 0 ) {
					sscanf(s, "%*s %d", &rule_data[i].bufsize);
					if((rule_data[i].bufsize < 0) || (rule_data[i].bufsize > MAX_COPY_SIZE) ) {
                        sprintf(msg,  "line# %d  %s bufsize = %d (must be >= 0 and <= %d) \n", line, keywd, rule_data[i].bufsize, MAX_COPY_SIZE);
                        hxfmsg(htx_d, 0, HTX_HE_SOFT_ERROR, msg);
                        error = 'y';
                    }		
	    	    } else if ( (strcmp(keywd, COMPARE_TAG)) == 0 ) {
					sscanf(s, "%*s %s", temp); 
	                if ( (strcmp(temp, "TRUE")) == 0 ) {
    	            	rule_data[i].compare = TRUE;
        	        } else if ( (strcmp(temp, "FALSE")) == 0 ) {
            	    	rule_data[i].compare = FALSE;
                	} else {
                    	sprintf(msg, "line# %d %s = %s (must be TRUE or FALSE) \n", line, keywd, temp);
	                    hxfmsg(htx_d, 0, HTX_HE_SOFT_ERROR, msg);
    	                error = 'y';
                    }
		        } else if ( (strcmp(keywd, NUM_THREADS_TAG)) == 0 ) {
                    sscanf(s, "%*s %d", &rule_data[i].num_threads);
                    if(rule_data[i].num_threads < 1 || rule_data[i].num_threads > MAX_THREADS) {   
                    	sprintf(msg, "line# %d %s = %d (must be >= 1 and <= 32)", line, keywd, rule_data[i].num_threads);
                        hxfmsg(htx_d, 0, HTX_HE_SOFT_ERROR, msg);
                        error = 'y'; 
                    }
	            } else if ( (strcmp(keywd, NUM_OPER_TAG)) == 0 ) {
                    sscanf(s, "%*s %d", &rule_data[i].num_oper); 
                    if(rule_data[i].num_oper < 0 ) { 
                    	sprintf(msg, "line# %d %s = %d (must be >= 1 ) \n", line, keywd, rule_data[i].num_oper); 
                        hxfmsg(htx_d, 0, HTX_HE_SOFT_ERROR, msg);
                        error = 'y';
                    }
                }
            }
		}
		if(*num_stanza == MAX_STANZA) {
			sprintf(msg, "Maximum stanza count reached. \n");
			hxfmsg(htx_d, 0, HTX_HE_SOFT_ERROR, msg);
			error = 'y';
			break;
		}
	}
    if(error == 'n')
        return(0);
    else
        return(-1);
}

/****************************************************************************
*Function Name:        print_rule_file_data
*Function Description: prints parsed rule file 
*Function arguments:   stanza count
*Return Value:         void
****************************************************************************/

void print_rule_file_data(int num_stanz)
{
    int i = 0;      /* Loop count */


	printf("\n\n*********************\n");
	printf("Parsed Rule file data\n");
	printf("*********************\n\n");

    /* Printing parsed data */
    for(i = 0; i < num_stanz; i++) {
        printf("RULE ID is %s\n",rule_data[i].rule_id);
        printf("TEST CASE is %d\n",rule_data[i].testcase);
        printf("BUF VAL is %d\n",rule_data[i].bufsize);
        printf("COMPARE is %d\n",rule_data[i].compare);
        printf("NUM THREADS is %d\n",rule_data[i].num_threads);
        printf("NUM OPER is %d\n",rule_data[i].num_oper);
    }
}
