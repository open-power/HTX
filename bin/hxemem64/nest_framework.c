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
#define DEBUG_ON


/*move below global vars to mem specific file later*/
int AME_enabled = FALSE;
char  page_size_name[MAX_PAGE_SIZES][8]={"4K","64K","2M","16M","16G"};/*RV2:add new supported pages*/
struct nest_global g_data;


/*********************************************
 * Function Prototypes						*
 * ******************************************/
#if 0
void SIGCPUFAIL_handler(int,int, struct sigcontext *);
void reg_sig_handlers(void);
int read_cmd_line(int argc,char *argv[]); 
void get_test_type(void);
void get_cache_details(void);
int get_system_details(void);
int apply_process_settings(void);

#ifdef __HTX_LINUX__
extern void SIGUSR2_hdl(int, int, struct sigcontext *);
#endif
#endif
/***********************************************************************
 *
* SIGTERM signal handler                                               *
************************************************************************/
 
void SIGTERM_hdl (int sig, int code, struct sigcontext *scp)
{
    /* Dont use display function inside signal handler.
     * the mutex lock call will never return and process
     * will be stuck in the display function */
    hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,"hxemem64: Sigterm Received!\n");
    g_data.exit_flag = 1;
}
#if 0
void SIGSEGV_hdl(int sig, int code, struct sigcontext *scp)
{
    char msg[50];
    hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,"hxemem64: sig11 Received!\n");
     
}
#endif


#ifndef __HTX_LINUX__

void SIGCPUFAIL_handler(int sig, int code, struct sigcontext *scp)
{
#if 0
    char hndlmsg[512];
    int no_of_proc;

    hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,"SIGCPUFAIL signal recieved.\n");

    g_data.sigvector.sa_handler = (void (*)(int))SIG_IGN;
    (void) sigaction(SIGCPUFAIL, &g_data.sigvector, (struct sigaction *) NULL);

    no_of_proc = get_num_of_proc();
    if (mem_info.tdata_hp[mem_info.num_of_threads - 1].thread_num == (no_of_proc - 1) && mem_info.tdata_hp[mem_info.num_of_threads - 1].tid != -1) {
        if (bindprocessor(BINDTHREAD, mem_info.tdata_hp[mem_info.num_of_threads - 1].kernel_tid, PROCESSOR_CLASS_ANY)) {
            sprintf(hndlmsg,"Unbind failed. errno %d, for thread num %d, TID %d.\n", errno, mem_info.num_of_threads - 1, mem_info.tdata_hp[mem_info.num_of_threads - 1].tid);
        }
    }
    g_data.sigvector.sa_handler = (void (*)(int))SIGCPUFAIL_handler;
    (void) sigaction(SIGCPUFAIL, &g_data.sigvector, (struct sigaction *) NULL);
#endif
	}
#endif



void reg_sig_handlers()
{
	/* SIGTERM handler registering */
    sigemptyset(&(g_data.sigvector.sa_mask));
    g_data.sigvector.sa_flags = 0 ;
    g_data.sigvector.sa_handler = (void (*)(int)) SIGTERM_hdl;
    sigaction(SIGTERM, &g_data.sigvector, (struct sigaction *) NULL);

   /*debug:Register segv handler*/ 
#if 0
    g_data.sigvector.sa_handler = (void (*)(int))SIGSEGV_hdl;
    sigaction(SIGSEGV,&g_data.sigvector,(struct sigaction *) NULL);
#endif
#ifdef __HTX_LINUX__
	/* Register SIGUSR2 for handling CPU hotplug */
    g_data.sigvector.sa_handler = (void (*)(int)) SIGUSR2_hdl;
    sigaction(SIGUSR2, &g_data.sigvector, (struct sigaction *) NULL);
	
#else

	/* Register SIGCPUFIAL handler  */
    g_data.sigvector.sa_handler = (void (*)(int)) SIGCPUFAIL_handler;
    (void) sigaction(SIGCPUFAIL, &g_data.sigvector, (struct sigaction *) NULL);
#endif

}

/*****************************************************************************
*      displaym(int sev,int debug, const char *format) which print the message on
*   the screen for stand alone mode run or in the /tmp/htxmsg file otherwise.
*****************************************************************************/
int displaym(int sev,int  debug, const char *format,...) {/*RV2: remove locking and g_data.msg_n_err_logging_lock flag*/
    va_list varlist;
    char msg[4096];
    int err; /* error number */

   /* Collect the present error number into err local variable*/
    err = errno;

   /*
    * If debug level is 0 (MUST PRINT) we sud always print.
    * If debug level is > 0  and level of this print is less or equal to
    * g_data.gstanza.global_debug_level (cmd line global debug level variable) then display this message else
    * dont display this message. And zero can't be greater than any other debug level
    * so it always prints below.*/
    if( debug > g_data.gstanza.global_debug_level) {
            return(0);
     }

    va_start(varlist,format);
    /*if (g_data.msg_n_err_logging_lock)
        pthread_mutex_lock(&g_data.tmutex);*/

    if (g_data.standalone == 1) {
        vprintf(format,varlist);
        fflush(stdout);
    }
    else {
        vsprintf(msg,format,varlist);
        hxfmsg(&g_data.htx_d,err,sev,msg);
    }

    /*if (g_data.msg_n_err_logging_lock)
        pthread_mutex_unlock(&g_data.tmutex);*/

    va_end(varlist);
    return(0);
}
/*****************************************************************************
*   read_cmd_line function reads the main programs command line
*   params into the g_data  data structure variable
*****************************************************************************/
int read_cmd_line(int argc,char *argv[]) 
{
	int i=-1,j=0;
	char tmp[30],msg[200];
	char* htxkdblevel;
	/* Collecting the command line arguments */
    if (argc > 1){
        strcpy (DEVICE_NAME, argv[1]);
	}
    else {
		displaym(HTX_HE_INFO,DBG_MUST_PRINT,"device name is missing as first argument, assuming it as /dev/mem\n");
        strcpy (DEVICE_NAME, "/dev/mem");
	}
    if (argc > 2) {
        strcpy (RUN_MODE, argv[2]);
        for (i = 0; (i < 4) && (RUN_MODE[i] != '\0'); i++)
            RUN_MODE[i] = toupper (RUN_MODE[i]);

        if ( strcmp (RUN_MODE, "REG") != 0)
        {
            strcpy (RUN_MODE, "OTH");
            g_data.standalone = 1;
        }
    }
    else  {
        strcpy (RUN_MODE, "OTH");
        g_data.standalone = 1;
    }
    strcpy (EXER_NAME, argv[0]);
    if (argc > 3) {
        strcpy (g_data.rules_file_name, argv[3]);
    }
    else { /*Default value */
        char* rules_tmp_ptr = getenv("HTXREGRULES");
        if(rules_tmp_ptr == NULL){
            displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s(): env variable HTXREGRULES is not set,rules_tmp_ptr=%p"
            ", thus exiting..\n",__LINE__,__FUNCTION__,rules_tmp_ptr);
            exit(1);
        }else{
            strcpy(g_data.rules_file_name,rules_tmp_ptr);
        }
        strcat(g_data.rules_file_name,"/hxemem64/default");
    }

    if (argc > 4) {
        sscanf(argv[4],"%d",&g_data.gstanza.global_debug_level);
    }
    else {
		g_data.gstanza.global_debug_level=0;
    }

    /*
     *  Environment Variable stating whether to enter kdb in case if miscompare.
     */

    htxkdblevel = getenv("HTXKDBLEVEL");
    if (htxkdblevel) {
		g_data.kdb_level = atoi(htxkdblevel);
    }
    else {
        g_data.kdb_level = 0;
    }

    /* Get the logical processor number in the device name like /dev/mem2 means
    2nd logical procesor
    */

    /*RV2: override filters with below cpu number in case of EQUILIZER*/
    sscanf(DEVICE_NAME,"%[^0-9]s",tmp);
    strcat(tmp,"%d");

    i = UNSPECIFIED_LCPU;
    if ((j=sscanf(DEVICE_NAME,tmp,&i)) < 1) {
        strcpy(msg,"read_cmd_line: No binding specified with the device name.\n");
        if(g_data.gstanza.global_debug_level == DBG_DEBUG_PRINT) {
            sprintf(msg,"%sDebug info:%s,i=%d,j=%d\n",msg,tmp,i,j);
        }
        i = UNSPECIFIED_LCPU;
    }
    else {
        sprintf(msg,"Binding is specified proc=%i\n",i);
    }

    if(g_data.standalone == 1) {
        displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%s",msg);
    }
    /*g_data.bind_proc=i;*/
	g_data.dev_id = i;
    return(0);

} /* End of read_cmd_line() function */


void get_test_type()
{
	if(strncmp((char*)(DEVICE_NAME+5),"mem",3) == 0){
		g_data.test_type = MEM;
	}	
	else if ((strncmp ((char*)(DEVICE_NAME+5),"cach",4)) == 0){
		g_data.test_type = CACHE;
	}
	else if (	((strncmp ((char*)(DEVICE_NAME+5),"fabn",4)) == 0) || (strncmp ((char*)(DEVICE_NAME+5),"fabc",4)== 0)	){
		g_data.test_type = FABRICB;
	}
	else if ((strncmp ((char*)(DEVICE_NAME+5),"tlb",3)) == 0){
		g_data.test_type = TLB;
	}	
	else {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:unknown device name %s\n",__LINE__,__FUNCTION__,DEVICE_NAME);
		exit(1);
	}
		
}

/*************************************************************
*get_cache_details:returns: void 							 *
*									 						 *
*************************************************************/
void get_cache_details() {
    htxsyscfg_cache_t cpu_cache_information;
#ifdef __HTX_LINUX__
        L2L3cache( &cpu_cache_information);
#else
        L2cache( &cpu_cache_information);
        L3cache( &cpu_cache_information);
#endif

	g_data.sys_details.cinfo[L2].cache_size = cpu_cache_information.L2_size;
	g_data.sys_details.cinfo[L2].cache_associativity=cpu_cache_information.L2_asc;;
	g_data.sys_details.cinfo[L2].cache_line_size = cpu_cache_information.L2_line;

	g_data.sys_details.cinfo[L3].cache_line_size = cpu_cache_information.L3_line;
	g_data.sys_details.cinfo[L3].cache_size = cpu_cache_information.L3_size;
	g_data.sys_details.cinfo[L3].cache_associativity=cpu_cache_information.L3_asc;;

	g_data.sys_details.cinfo[L4].cache_line_size = 128;
	g_data.sys_details.cinfo[L4].cache_size = 16 * MB;
	g_data.sys_details.cinfo[L4].cache_associativity=16;
	
}


/*************************************************************
* get_system_details(): Collects all system hardware details *
*returns: 0 on success										 *
*		  -1 on failure										 *
*************************************************************/	

int get_system_details() {
	int rc = SUCCESS,i=0,pi=0,count=0,node=0,chip=0,core=0,lcpu=0;
	htxsyscfg_smt_t     system_smt_information;
	htxsyscfg_memory_t	sys_mem_info;
	SYS_STAT			hw_stat;
	htxsyscfg_env_details_t htx_env;
	unsigned long		free_mem,tot_mem;
	struct mem_info* memptr = &g_data.sys_details.memory_details;
	struct sys_info* sysptr = &g_data.sys_details;
	struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
	/* Ask for the library to update its info */
	rc = repopulate_syscfg(&g_data.htx_d);
    if( rc != 0) {
        while(count < 10){
            sleep(1);
            rc = repopulate_syscfg(&g_data.htx_d);
            if(!rc){
                break;
            }
            else {
                count++;
            }
        }
        if(count == 10 && rc != 0){
               displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s: Repopulation of syscfg unsuccessful with rc = %d. Exiting\n",__LINE__,__FUNCTION__,rc);
                return (FAILURE);
        }
    }
	get_env_details(&htx_env);
	if(strcmp(htx_env.proc_shared_mode,"yes") == 0){
		sysptr->shared_proc_mode =1;
	}
	else {
		sysptr->shared_proc_mode = 0;
	}
	get_smt_details(&system_smt_information);
    /* Check if smt value returned is valid. If not then exit. */
    if(system_smt_information.smt_threads >= 1) {
        sysptr->smt_threads = system_smt_information.smt_threads;
    }
    else {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Incorrect value of smt_threads obtained : %d, Exiting\n",__LINE__,__FUNCTION__,system_smt_information.smt_threads);
        return (FAILURE);
    }

    /* Get pvr information */
#if defined(__HTX_LINUX__) && defined(AWAN)
    sysptr->os_pvr = POWER8_MURANO; /* Hardcode p8 value for AWAN */
#elif defined(__HTX_LINUX__) && !defined(AWAN)
    /*system_information.pvr = get_pvr();*/
    sysptr->os_pvr=get_cpu_version();
    sysptr->os_pvr = (sysptr->os_pvr)>>16 ;
	/*####*** add true pvr details*/
#endif
    if(g_data.test_type == MEM){/*collect mem details once all exerciser are started*/
	/*REV:below calls need to be replaced with single lib call*/
	rc=get_memory_size_update();
    if ( rc ) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_memory_size__update() failed with rc=%d\n", rc);
			return (rc);
    }
#ifdef __HTX_LINUX__
    rc=get_page_size_update();
    if ( rc ) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_page_size__update() failed with rc=%d\n", rc);
			return (rc);
    }
#endif

    rc = get_page_details();
    if ( rc ) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_page_details() failed with rc=%d\n", rc);
			return (rc);
    }

    #ifdef __HTX_LINUX__
    rc = get_memory_pools();
    if(rc < 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:syscfg fun get_memory_pools() failed with rc = %d\n",
			__LINE__,__FUNCTION__,rc);
			return (rc);
    }
    #else
	rc = get_memory_pools_update();
	if(rc < 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:syscfg fun get_memory_pools_update() failed with rc = %d\n",
			__LINE__,__FUNCTION__,rc);
			return (rc);
	}
    #endif
    }
	rc = get_memory_details(&sys_mem_info);
	if(rc < 0){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:get_memory_details() returned rc = %d\n",__LINE__,__FUNCTION__,rc);
        return(FAILURE);
    }
	memptr->total_mem_avail = sys_mem_info.mem_size.real_mem;
	memptr->total_mem_free  = sys_mem_info.mem_size.free_real_mem;
#ifndef __HTX_LINUX__
	memptr->pspace_avail	  = sys_mem_info.mem_size.paging_space_total;/*RV2: check, pspase can be removed*/
	memptr->pspace_free 	  = sys_mem_info.mem_size.paging_space_free;
#endif
	
	for(i=0; i<MAX_PAGE_SIZES; i++) {
		memptr->pdata[i].supported = FALSE;
		if(sys_mem_info.page_details[i].supported) {
			memptr->pdata[i].supported = TRUE;
			memptr->pdata[i].psize = sys_mem_info.page_details[i].page_size;
			memptr->pdata[i].avail = (memptr->pdata[i].psize * sys_mem_info.page_details[i].total_pages);
			memptr->pdata[i].free  = (memptr->pdata[i].psize * sys_mem_info.page_details[i].free_pages);
		}
	}			

	/* Fill SRAD/NUMA-NODE related data structres */
#ifdef __HTX_LINUX__
	sysptr->num_chip_mem_pools = sys_mem_info.num_numa_nodes;
#else
	sysptr->num_chip_mem_pools = sys_mem_info.num_srads;
#endif
	if(sysptr->num_chip_mem_pools <= 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Incorrect value of srads/numa_nodes obtained : %d, "
			"Some issue in reading system config by syscfg, Exiting\n",__LINE__,__FUNCTION__,sysptr->num_chip_mem_pools);	
		exit(1);
	}
    sysptr->unbalanced_sys_config = 0;
	for(i=0;i<sysptr->num_chip_mem_pools;i++){
		mem_details_per_chip[i].num_cpus						= sys_mem_info.mem_pools[i].num_procs; /* No of cpus in this chip*/
        mem_details_per_chip[i].chip_id                         = i;
		memcpy(mem_details_per_chip[i].cpulist,sys_mem_info.mem_pools[i].procs_per_pool,(sizeof(int)*mem_details_per_chip[i].num_cpus));
		mem_details_per_chip[i].memory_details.total_mem_avail 	= sys_mem_info.mem_pools[i].mem_total;
		mem_details_per_chip[i].memory_details.total_mem_free	= sys_mem_info.mem_pools[i].mem_free;
		mem_details_per_chip[i].has_cpu_and_mem 			= sys_mem_info.mem_pools[i].has_cpu_or_mem;               /*flag,to check node has either cpus or memory */ 	
        if(!mem_details_per_chip[i].has_cpu_and_mem){
            sysptr->unbalanced_sys_config = 1;
        }
		/* Fill page level details for SRAD/NUMA considering 50 % as 4k and 64K in AIX(due to AIX limitation of getting 4k and 64k details per srad**/
		for(pi=0;pi<MAX_PAGE_SIZES; pi++){
			if(memptr->pdata[pi].supported == TRUE) {
#ifndef __HTX_LINUX__ 
				if((pi < PAGE_INDEX_2M)){/*use explicit page indexes check*/
					free_mem = (mem_details_per_chip[i].memory_details.total_mem_free * 0.5);
					free_mem = free_mem/memptr->pdata[pi].psize;
					free_mem = free_mem * memptr->pdata[pi].psize;
					
					tot_mem  = (mem_details_per_chip[i].memory_details.total_mem_avail * 0.5);
					tot_mem  = tot_mem / memptr->pdata[pi].psize;
					tot_mem  = tot_mem * memptr->pdata[pi].psize;
				}else{
					free_mem = sys_mem_info.mem_pools[i].page_info_per_mempool[pi].free_pages * memptr->pdata[pi].psize;
					tot_mem  = sys_mem_info.mem_pools[i].page_info_per_mempool[pi].total_pages * memptr->pdata[pi].psize;
				}
#else
				free_mem = sys_mem_info.mem_pools[i].page_info_per_mempool[pi].free_pages * memptr->pdata[pi].psize;
				tot_mem  = sys_mem_info.mem_pools[i].page_info_per_mempool[pi].total_pages * memptr->pdata[pi].psize;
#endif
				mem_details_per_chip[i].memory_details.pdata[pi].supported = TRUE;
				mem_details_per_chip[i].memory_details.pdata[pi].free = free_mem; 
				mem_details_per_chip[i].memory_details.pdata[pi].avail = tot_mem;
				mem_details_per_chip[i].memory_details.pdata[pi].psize= memptr->pdata[pi].psize;
                /*initialize memory filter specific data structures*/
                mem_details_per_chip[i].memory_details.pdata[pi].page_wise_usage_mpercent = 0;
                mem_details_per_chip[i].memory_details.pdata[pi].page_wise_usage_mem      = 0;
                mem_details_per_chip[i].memory_details.pdata[pi].page_wise_usage_npages   = 0;
                mem_details_per_chip[i].memory_details.pdata[pi].in_use_num_of_segments   = 0; 
   			}
		}
	}
	/*get hardware htx_d info */
	rc = get_hardware_stat( &hw_stat );
	if (rc == -1) {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:get_hardware_stat() returned rc = %d\n",__LINE__,__FUNCTION__,rc);
		return (FAILURE);
	}
	sysptr->nodes = hw_stat.nodes;
	sysptr->chips = hw_stat.chips;
	sysptr->cores = hw_stat.cores;
	sysptr->tot_cpus = hw_stat.cpus;

#ifndef __HTX_LINUX__
    rc = syscfg_lock_sem(SYSCFG_LOCK);
#else
    rc = pthread_rwlock_rdlock(&(global_ptr->syscfg.rw));
#endif
    if (rc != 0 ) {
       displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:lock inside nest_framework.c failed with rc = %d and errno=%d\n", __LINE__,__FUNCTION__,rc,errno);
        
    }
    int srad_counter=0;
    for( node = 0; node < MAX_NODES && node < hw_stat.nodes ; node++ ) {
		if(global_ptr->syscfg.node[node].num_chips > 0) {
	    	for(chip = 0; chip < MAX_CHIPS && chip < global_ptr->syscfg.node[node].num_chips; chip++ ) {	        
	        	if(global_ptr->syscfg.node[node].chip[chip].num_cores > 0) { 
		   			for(core = 0; core < MAX_CORES && core < global_ptr->syscfg.node[node].chip[chip].num_cores; core++ ) {
						if(global_ptr->syscfg.node[node].chip[chip].core[core].num_procs > 0) {
			    			for(lcpu=0;lcpu < MAX_CPUS &&  lcpu < global_ptr->syscfg.node[node].chip[chip].core[core].num_procs;lcpu++){
				
								sysptr->node[node].chip[chip].core[core].lprocs[lcpu] = global_ptr->syscfg.node[node].chip[chip].core[core].lprocs[lcpu];
		   	   					sysptr->node[node].chip[chip].core[core].num_procs++;
								debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"sysptr->node[%d].chip[%d].core[%d].lprocs[%d]=%d\n",node,chip,
								core,lcpu,sysptr->node[node].chip[chip].core[core].lprocs[lcpu]);

     			    		}

		   				}
		     		    sysptr->node[node].chip[chip].num_cores++;
		   			}	
                    memcpy(&sysptr->node[node].chip[chip].mem_details,&g_data.sys_details.chip_mem_pool_data[srad_counter].memory_details,sizeof(struct mem_info));         	
				}
                srad_counter++;
	    		sysptr->node[node].num_chips++;
	    	}
		}
    }
#ifndef __HTX_LINUX__
    rc = syscfg_unlock_sem(SYSCFG_LOCK);
#else
    rc = pthread_rwlock_unlock(&(global_ptr->syscfg.rw));
#endif
    if (rc !=0  ) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:unlock inside framework.c failed with rc = %d and errno=%d\n",__LINE__,__FUNCTION__,rc,errno);
		return(FAILURE);
    }

	get_cache_details();
	return (SUCCESS);

}
/*RV3: look in old code for modification of shmax,shmni*/
int apply_process_settings() {
	int rc;
	struct sys_info* sysptr = &g_data.sys_details;
	FILE *fp=NULL;
#ifndef __HTX_LINUX__
    int early_lru = 1; /* 1 extends local affinity via paging space.*/
    int num_policies = VM_NUM_POLICIES;
    int policies[VM_NUM_POLICIES] = {/* -1 = don't change policy */
    -1, /* VM_POLICY_TEXT */
    P_FIRST_TOUCH, /* VM_POLICY_STACK */
    P_FIRST_TOUCH, /* VM_POLICY_DATA */
    P_FIRST_TOUCH, /* VM_POLICY_SHM_NAMED */
    P_FIRST_TOUCH, /* VM_POLICY_SHM_ANON */
    -1, /* VM_POLICY_MAPPED_FILE */
    -1, /* VM_POLICY_UNMAPPED_FILE */
    };


	/* To get local memory set flag early_lru=1 to select P_FIRST_TOUCH policy(similiar to setting MEMORY_AFFINITY environment variable to MCM)*/
	rc = vm_mem_policy(VM_SET_POLICY,&early_lru, &policies ,num_policies);
	if (rc != 0){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"vm_mem_policy() call failed with return value = %d\n",rc);
	}
#endif

#ifdef __HTX_LINUX__
	/* collect shm related values from file system*/
	fp = popen("cat /proc/sys/kernel/shmmax","r");
	if (fp == NULL ) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:popen failed for /proc/sys/kernel/shmmax,errno=%d\n",
			__LINE__,__FUNCTION__,errno);
		return (FAILURE);
	}
	rc = fscanf(fp,"%d",&sysptr->shmmax);
	if (rc == 0 || rc == EOF) {
		displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:fscanf failed for shmmax with rc = %d,errno=%d\n",
			__LINE__,__FUNCTION__,rc,errno);
		pclose(fp);
		return (FAILURE);
	}
	pclose(fp);


    fp = popen("cat /proc/sys/kernel/shmall","r");
    if (fp == NULL) {
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:popen failed for /proc/sys/kernel/shmall,errno=%d\n",
			 __LINE__,__FUNCTION__,errno);
		return (FAILURE);
    }
    rc  = fscanf(fp,"%d",&sysptr->shmall);
    if (rc == 0 || rc == EOF) {
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:fscanf failed for shmall with rc = %d,errno=%d\n",
			 __LINE__,__FUNCTION__,rc,errno);
		pclose(fp);
		return (FAILURE);
    }
    pclose(fp);

    fp = popen("cat /proc/sys/kernel/shmmni","r");
    if (fp == NULL)  {
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:popen failed for /proc/sys/kernel/shmmni,errno=%d\n",
			__LINE__,__FUNCTION__,errno);
		return (FAILURE);
    }
    rc  = fscanf(fp,"%d",&sysptr->shmmni);
    if (rc == 0 || rc == EOF) {
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:fscanf failed for shmmni with rc = %d,errno=%d\n",
			__LINE__,__FUNCTION__,rc,errno);
		pclose(fp);
		return (FAILURE);
    }
    pclose(fp);
#endif
	return (SUCCESS);

}
/*****************************************************************************
*  MAIN: begin program main procedure                                        *
*****************************************************************************/
int main(int argc, char *argv[])
{
	int ret_code=SUCCESS;
    char command[256];
	/*Register signal  handlers*/
	reg_sig_handlers();

	/* Intialize mutex variable */
    if ( pthread_mutex_init(&g_data.tmutex,NULL) != 0) { 
         displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread init error (%d): %s\n",__LINE__,__FUNCTION__,errno,strerror(errno));
    }
    g_data.msg_n_err_logging_lock= 1; /* flag set that means mutex variable exists*/


	/* Read the command line parameters */
	ret_code = read_cmd_line(argc,argv);
	if(ret_code != SUCCESS){
		 displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:read_cmd_line() failed with ret_code = %d\n",__LINE__,__FUNCTION__,ret_code);
	}
	
	/* Store the pid of the process */
	g_data.pid=getpid();

#ifdef __HTX_LINUX__
    /* Set coredump config file to 0x31, so that if any coredump generated should not contain share memory mapings*/
    sprintf(command,"echo 49 > /proc/%d/coredump_filter",g_data.pid);
    ret_code = system(command);
    if(ret_code == -1){
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s: comand: '%s' failed, If any core dump, it will also include shared mem mapings!!\n",
            __LINE__,__FUNCTION__,command);
    }
#endif
	get_test_type(); /*retreives running exer type*/

	/* Notify Supervisor of exerciser start and 
	 * set htx_data flag(hotplug_cpu) to make supervisor aware that we want to recieve SIGUSR2 in case of Linux hotplug add/remove */
#ifdef __HTX_LINUX__
    STATS_VAR_INIT(hotplug_cpu, 1)
#endif
    STATS_HTX_UPDATE(START)
    STATS_HTX_UPDATE(UPDATE)
	displaym(HTX_HE_INFO,DBG_MUST_PRINT,"%s %s %s %s %d\n",EXER_NAME,DEVICE_NAME,RUN_MODE,\
		g_data.rules_file_name,g_data.gstanza.global_debug_level);

    if((!(strlen(g_data.htx_d.htx_log_dir))) || (!(strlen(g_data.htx_d.htx_exer_log_dir)))){
     
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"htx lib log direcories found to be htx_log_dir= %s and htx_exer_log_dir= %s, thus updating them to '/tmp/'\n",
            g_data.htx_d.htx_log_dir,g_data.htx_d.htx_exer_log_dir);
        strcpy(g_data.htx_d.htx_log_dir,"/tmp/");
        strcpy(g_data.htx_d.htx_exer_log_dir,"/tmp/");
    }
	FILE* fp;
	sprintf(command,"grep -i global_startup_delay %s | awk '{print $3}'",g_data.rules_file_name);
    fp = popen(command,"r");
    if (fp == NULL) {
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                        "popen failed for command:%d errno(%d)\n",command,errno);
        return(FAILURE);
    }
    ret_code  = fscanf(fp,"%d",&g_data.gstanza.global_startup_delay);
    if (ret_code == 0 || ret_code == EOF) {
		displaym(HTX_HE_HARD_ERROR,DBG_DEBUG_PRINT,"setting global_startup_delay to %d sec",DEFAULT_DELAY);
		g_data.gstanza.global_startup_delay = DEFAULT_DELAY;
    }
    pclose(fp);
    if((g_data.standalone != 1) && (g_data.test_type == MEM) && (g_data.gstanza.global_startup_delay)){
        displaym(HTX_HE_INFO, DBG_MUST_PRINT, "Waiting %d second(s), "
                 "while other exercisers allocate their memory.\n",
                g_data.gstanza.global_startup_delay);
		sleep(g_data.gstanza.global_startup_delay);
    }	
	/* Collect system details */
	ret_code = get_system_details();
	if(ret_code != SUCCESS){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:get_system_details() failed with ret_code = %d\n",__LINE__,__FUNCTION__,ret_code);
		exit(1);
	}
    print_partition_config(HTX_HE_INFO);

    if (g_data.exit_flag == 1) {
        return(-1);
    } 

    g_data.rf_last_mod_time = 0;
	if ((read_rules()) < 0 ) {
		exit(1);/*RV3: check for error possibility*/
	}

    if (g_data.exit_flag == 1) {
        return(-1);
    } 
	
	/* apply process level settings/policies */
	ret_code = apply_process_settings();
	if(ret_code != SUCCESS){
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:(apply_process_settings) failed with ret_code = %d\n",__LINE__,__FUNCTION__,ret_code);
		exit(1);
	}

	/* Call test specific functions */
	switch (g_data.test_type) {
		case MEM:
			ret_code = mem_exer_opearation();
			if(ret_code != SUCCESS){
					exit(1);
			}
			break;
		
		case TLB:
			break;
	
		case CACHE:
			break;

		case FABRICB:
            if(g_data.sys_details.shared_proc_mode == 1){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: Fabricbus exerciser does not support shared processor configured systems, exiting..\n",
                    __LINE__,__FUNCTION__);
                exit(1);
            }
            ret_code = memory_segments_calculation();
			if(ret_code != SUCCESS){
					exit(1);
			}
			ret_code = mem_exer_opearation();
			if(ret_code != SUCCESS){
					exit(1);
			}
			break;

		default:
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:wrong test type,it must be one of - /mem/cache/tlbie/fabricb \n",
					__LINE__,__FUNCTION__);			
			return(FAILURE);
	}

	return(SUCCESS);
}/*end of main*/


void print_partition_config(int msg_type){
    int i=0,pi=0;
    char msg[4096],msg_temp[2048];
    struct mem_info* memptr = &g_data.sys_details.memory_details;
    struct sys_info* sysptr = &g_data.sys_details;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];

    sprintf(msg,"SYSTEM DETAILS:\t\tNodes=%d\tChips=%d\tCores=%d\tLogical cpus=%d\n" 
        "\n\tTotal memory:%llu MB\n\tFree memory:%llu MB\n\tTotal paging space:%lu\n\tFreepaging space:%lu\n",
        sysptr->nodes,sysptr->chips,sysptr->cores,sysptr->tot_cpus,
        (memptr->total_mem_avail/MB),(memptr->total_mem_free/MB),memptr->pspace_avail,memptr->pspace_free);

    for(i=0; i<MAX_PAGE_SIZES; i++) {
    if(memptr->pdata[i].supported) {
            sprintf(msg_temp,"\tpage size:%lu(%s)\t total pages=%lu\tfree pages=%lu\n",memptr->pdata[i].psize,page_size_name[i],
                (memptr->pdata[i].avail/memptr->pdata[i].psize),
                (memptr->pdata[i].free/memptr->pdata[i].psize)); 
            strcat(msg,msg_temp);
        }
    }
    displaym(msg_type,DBG_MUST_PRINT,"%s",msg);
    sprintf(msg,"Chip level mem details:\n\nCHIP\tCPU\tMEM\n-----------------------\n");
    for(i=0;i<sysptr->num_chip_mem_pools;i++){
        if(mem_details_per_chip[i].num_cpus > 0){
            sprintf(msg_temp," %d\t Y\t",i);
        }else{
            sprintf(msg_temp," %d\t N\t",i);
        }
        strcat(msg,msg_temp);
        if(mem_details_per_chip[i].memory_details.total_mem_avail > 0){
            sprintf(msg_temp," Y\t\n");
        }else {
            sprintf(msg_temp," N\t\n");
        }  
        strcat(msg,msg_temp);
    } 
    for(i=0;i<sysptr->num_chip_mem_pools;i++){
        if(mem_details_per_chip[i].has_cpu_and_mem){
            sprintf(msg_temp,"\nchip no:%d\n\tcpus:%d",i,mem_details_per_chip[i].num_cpus);
            strcat(msg,msg_temp);
            /*for(k=0;k<mem_details_per_chip[i].num_cpus;k++){
                sprintf(msg_temp,":%d",mem_details_per_chip[i].cpulist[k]);
                strcat(msg,msg_temp);
            }*/
            sprintf(msg_temp,"\n\tTotal mem :%llu MB\n\tFree mem :%llu MB\n",
                (mem_details_per_chip[i].memory_details.total_mem_avail/MB),(mem_details_per_chip[i].memory_details.total_mem_free/MB));
            strcat(msg,msg_temp);
            for(pi=0;pi<MAX_PAGE_SIZES; pi++){
                if(mem_details_per_chip[i].memory_details.pdata[pi].supported){
                    sprintf(msg_temp,"\tpage size:%lu(%s)\t total pages =%lu\tfree pages =%lu\n",mem_details_per_chip[i].memory_details.pdata[pi].psize, page_size_name[pi],
                        (mem_details_per_chip[i].memory_details.pdata[pi].avail/mem_details_per_chip[i].memory_details.pdata[pi].psize),
                        (mem_details_per_chip[i].memory_details.pdata[pi].free/mem_details_per_chip[i].memory_details.pdata[pi].psize));
                    strcat(msg,msg_temp);
                }
            }
        }
    }
    displaym(msg_type,DBG_MUST_PRINT,"%s\n",msg);
}
	
int fill_exer_huge_page_requirement(){
	int rc = SUCCESS;
	unsigned long avail_16m,avail_16g,free_16m,free_16g;
	struct mem_info* memptr = &g_data.sys_details.memory_details;
	FILE *fp;
    char log_dir[256];
	int huge_page_size,huge_page_index;
	if(memptr->pdata[PAGE_INDEX_16M].supported){
		huge_page_index = PAGE_INDEX_16M;
		huge_page_size = (16 * MB);
	}else if(memptr->pdata[PAGE_INDEX_2M].supported){
		huge_page_index = PAGE_INDEX_2M;
		huge_page_size = (2 * MB);
	}
    strcpy(log_dir,g_data.htx_d.htx_log_dir);
    strcat(log_dir,"/freepages");
	if ((fp=fopen(log_dir,"r"))==NULL) {
         displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:fopen of memory free pages (%s) failed with errno:%d(%s)\n"
            "exerciser may continue to run without huge pages\n",__LINE__,__FUNCTION__,log_dir,errno,strerror(errno));
	}
	else {
		rc = fscanf(fp,"avail_16M=%lu,avail_16G=%lu,free_16M=%lu,free_16G=%lu",&avail_16m,\
                    &avail_16g,&free_16m,&free_16g);
		if(rc == 0 || rc == EOF) {
			displaym(HTX_HE_INFO,DBG_MUST_PRINT, "[%d]%s:Error while reading file (%s),errno:%d(%s)" 
				"making huge  page support as false\n",__LINE__,__FUNCTION__,log_dir,errno,strerror(errno));
			memptr->pdata[PAGE_INDEX_2M].supported = FALSE;
			memptr->pdata[PAGE_INDEX_16M].supported = FALSE;
			memptr->pdata[PAGE_INDEX_16G].supported = FALSE;
		}else{

            if(g_data.gstanza.global_alloc_huge_page == 0){
                memptr->pdata[PAGE_INDEX_16M].supported = FALSE;
                memptr->pdata[PAGE_INDEX_2M].supported = FALSE;
                memptr->pdata[PAGE_INDEX_16G].supported = FALSE;
            }
			
			switch(g_data.test_type) {
			
				case MEM:
						if(memptr->pdata[PAGE_INDEX_16M].supported || memptr->pdata[PAGE_INDEX_2M].supported){
							 memptr->pdata[huge_page_index].avail = (avail_16m * huge_page_size);
							 memptr->pdata[huge_page_index].free	= (free_16m * huge_page_size);
						}
						 memptr->pdata[PAGE_INDEX_16G].avail = (avail_16g * 16 * GB);
						 memptr->pdata[PAGE_INDEX_16G].free	= (free_16g * 16 * GB);
						 break;

				/* Add cache,fabric,tlbie,L4 */
                case FABRICB:
                        rc = set_fabricb_exer_page_preferances(); 
                    break;
                        
				default:
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]wrong test type arg is passed to fun %s for filling huge page deatils from %s\n",
							__LINE__,__FUNCTION__,log_dir);			
					return(FAILURE);
			}		
		fclose(fp);
		displaym(HTX_HE_INFO,DBG_MUST_PRINT,"huge page details from %s avail_16M/2M=%lu,avail_16G=%lu,free_16M/2M=%lu,free_16G=%lu\n",log_dir,avail_16m,avail_16g,free_16m,free_16g);
		}
		
	}
	return (SUCCESS);
}
#ifdef __HTX_LINUX__
int modify_shared_mem_limits_linux(unsigned long total_segments){
    int rc;
    FILE *fp;
    char oth_exer_segments[20],cmd[100];
    unsigned long new_shmmni;
    /* Set shmmax value to 256MB only if the value on current system is less than that */
    /* taken care in mem.setup by setting up shmmax to 1GB*/
    if (g_data.sys_details.shmmax < 268435456) {
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Changing the /proc/sys/kernel/shmmax variable to 256MB\n");
        rc = system ("echo 268435456 > /proc/sys/kernel/shmmax");
        if(rc < 0){
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"unable to change the /proc/sys/kernel/shmmax variable to 256MB\n");
        }
    }
    
    /* /proc/sys/kernel/shmall value have been taken care in htx.setup by setting up it to 90% of total memory*/
    
    fp = popen("ipcs -m | grep root | wc -l", "r");
    if (fp == NULL) {
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,
                        "popen failed for ipcs command: errno(%d)\n", errno);
        return(-1);
    }
    rc  = fscanf(fp,"%s",oth_exer_segments);
    if (rc == 0 || rc == EOF) {
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s: "
                        "Could not read oth_exer_segments from pipe\n",__LINE__,__FUNCTION__);
        return(FAILURE);
    }
    pclose(fp);

    if(g_data.sys_details.shmmni < strtoul(oth_exer_segments, NULL, 10) + total_segments) {
        new_shmmni = g_data.sys_details.shmmni + strtoul(oth_exer_segments, NULL, 10) + total_segments;
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Changing the /proc/sys/kernel/shmmni variable to %lu\n",new_shmmni);
        sprintf(cmd,"echo %lu > /proc/sys/kernel/shmmni", new_shmmni);
        rc = system (cmd);
        if(rc < 0){
            displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:unable to change the /proc/sys/kernel/shmmni variable to %lu\n",
                __LINE__,__FUNCTION__,new_shmmni);
            return (FAILURE);
        }
    }
    return SUCCESS;
}
#endif

