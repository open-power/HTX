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
#include <sys/resource.h>
#define DEBUG_ON


/*move below global vars to mem specific file later*/
int AME_enabled = FALSE;
char  page_size_name[MAX_PAGE_SIZES][8]={"4K","64K","2M","16M","16G"};
struct nest_global g_data;
extern struct mem_exer_info mem_g;
extern struct cache_exer_info cache_g;
int cpu_add_remove_test = 0;
struct shm_alloc_th_data {
        int thread_num;
        int chip_num;
        int bind_cpu_num;
        int chip_cpu_num;
        pthread_t tid;
}*shm_alloc_th=NULL;

exer_mem_alloc_fptr  memory_req_exer_fun[MAX_TEST_TYPES]=
    {
        &fill_per_chip_segment_details,
        &fill_per_chip_segment_details,
        (exer_mem_alloc_fptr)&fun_NULL,
        &fill_cache_exer_mem_req
    };

exer_stanza_oper_fptr exer_stanza_operation_fun[MAX_TEST_TYPES]=
    {
        &run_mem_stanza_operation,
        &run_mem_stanza_operation,
        &run_tlb_operaion,
        &cache_exer_operations
    };
/*********************************************
 * Function Prototypes						*
 * ******************************************/
void SIGRECONFIG_handler (int sig, int code, struct sigcontext *scp);
int check_ame_enabled(void);
int check_if_ramfs(void);
int fill_per_chip_segment_details(void);
int fill_system_segment_details(void);
int reset_segment_owners(void);
int get_shared_memory(void);
int allocate_buffers(int,int,struct shm_alloc_th_data* );
int deallocate_buffers(int,int,struct shm_alloc_th_data*);
int fill_affinity_based_thread_structure(struct chip_mem_pool_info*, struct chip_mem_pool_info*,int);
int fill_and_allocate_memory_segments(void);
void* get_shm(void*);
void* remove_shm(void*);
void* mem_thread_function(void *);
void* stats_update_thread_function(void*);
/***********************************************************************
 *
* SIGTERM signal handler                                               *
************************************************************************/
 
void SIGTERM_hdl (int sig, int code, struct sigcontext *scp)
{
    char hndlmsg[200];
    /* Dont use display function inside signal handler.
     * the mutex lock call will never return and process
     * will be stuck in the display function */
    sprintf(hndlmsg,"%s: Sigterm Received!\n",EXER_NAME);
    hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
    g_data.exit_flag = SET;
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
#ifndef __HTX_LINUX__
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

    i = DEFAULT_INSTANCE;
    sscanf(DEVICE_NAME,"%[^0-9]s",tmp);
    strcat(tmp,"%d");

    if ((j=sscanf(DEVICE_NAME,tmp,&i)) < 1) {
        strcpy(msg,"read_cmd_line: No chip instance is specified with device name, assuming it as 0 for cache exerciser\n");
        if(g_data.gstanza.global_debug_level == DBG_DEBUG_PRINT) {
            sprintf(msg,"%sDebug info:%s,i=%d,j=%d\n",msg,tmp,i,j);
        }
        i = DEFAULT_INSTANCE;
    }
    else {
        sprintf(msg,"specified chip instance is = %i\n",i);
    }

    if(g_data.standalone == 1) {
        if ((strncmp ((char*)(DEVICE_NAME+5),"cach",4)) == 0){
            displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%s",msg);
        }
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
	else if (	((strncmp ((char*)(DEVICE_NAME+5),"fabn",4)) == 0) || (strncmp ((char*)(DEVICE_NAME+5),"fabc",4)== 0)	|| 
		(strncmp ((char*)(DEVICE_NAME+5),"fabxo",5)== 0) ){
		g_data.test_type = FABRICB;
	}
	else if ((strncmp ((char*)(DEVICE_NAME+5),"tlb",3)) == 0){
		g_data.test_type = TLB;
	}	
	else {
		displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:unknown device name %s, supported htx nest devices are:"
            "mem,cache<chip_num>,fabn,fabc,fabxo,tlb\n",__LINE__,__FUNCTION__,DEVICE_NAME);
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

	g_data.sys_details.cache_info[L2].cache_size = cpu_cache_information.L2_size;
	g_data.sys_details.cache_info[L2].cache_associativity=cpu_cache_information.L2_asc;;
	g_data.sys_details.cache_info[L2].cache_line_size = cpu_cache_information.L2_line;

	g_data.sys_details.cache_info[L3].cache_line_size = cpu_cache_information.L3_line;
	g_data.sys_details.cache_info[L3].cache_size = cpu_cache_information.L3_size;
	g_data.sys_details.cache_info[L3].cache_associativity=cpu_cache_information.L3_asc;;

	g_data.sys_details.cache_info[L4].cache_line_size = 128;
	g_data.sys_details.cache_info[L4].cache_size = 16 * MB;
	g_data.sys_details.cache_info[L4].cache_associativity=16;
	
}


/*************************************************************
* get_system_details(): Collects all system hardware details *
*returns: 0 on success										 *
*		  -1 on failure										 *
*************************************************************/	

int get_system_details() {
	int rc = SUCCESS,i=0,pi=0,count=0,node=0,chip=0,core=0,lcpu=0;
    char oth_exer_hugepage_users[20];
    FILE *fp=NULL;
	htxsyscfg_smt_t     system_smt_information;
	htxsyscfg_memory_t	sys_mem_info;
	SYS_STAT			hw_stat;
	htxsyscfg_env_details_t htx_env;
	unsigned long		free_mem,tot_mem,free_pages=0;
	struct mem_info* memptr = &g_data.sys_details.memory_details;
	struct sys_info* sysptr = &g_data.sys_details;
	struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];

    fp = popen("ps -ef | grep hxecache | grep -v grep | wc -l", "r");
    if (fp == NULL) {
        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,
                        "[%d]%spopen failed for ps command: errno(%d)\n",__LINE__,__FUNCTION__,errno);
    }
    rc  = fscanf(fp,"%s",oth_exer_hugepage_users);
    if (rc == 0 || rc == EOF) {
        displaym(HTX_HE_SOFT_ERROR, DBG_MUST_PRINT,"[%d]%s: "
                        "Could not read oth_exer_hugepage_users from pipe\n",__LINE__,__FUNCTION__);
    }
    pclose(fp);

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
    if(system_smt_information.max_smt_threads >= 1) {
        sysptr->smt_threads = system_smt_information.max_smt_threads;
    }
    else if(sysptr->shared_proc_mode == 0) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:Incorrect value of smt_threads obtained : %d, Exiting\n",__LINE__,__FUNCTION__,system_smt_information.max_smt_threads);
        return (FAILURE);
    }

    /* Get pvr information */
    sysptr->os_pvr=get_cpu_version();
    sysptr->os_pvr = (sysptr->os_pvr)>>16 ;

    sysptr->true_pvr = get_true_cpu_version();
    sysptr->true_pvr = (sysptr->true_pvr)>>16;

    if(g_data.test_type == MEM){/*collect mem details once all exerciser are started*/
        /*REV:below calls need to be replaced with single lib call*/
        rc=get_memory_size_update();
        if ( rc ) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"get_memory_size__update() failed with rc=%d\n", rc);
                return (rc);
        }
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
#ifndef __HTX_LINUX__
	if((memptr->pdata[PAGE_INDEX_16G].avail > 0) && (g_data.test_type == MEM)){
		displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s: Found 16G page enabled, AIX has limitation in differentiating 16M and 16G pages at SRAD level, "
			"Memory exercser will run at system memory mode\n",__LINE__,__FUNCTION__);
		g_data.sys_details.shared_proc_mode = 1;

	}
#endif
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
        mem_details_per_chip[i].num_cores = 0;
        for(int c =0; c < MAX_CORES_PER_CHIP; c++){
            mem_details_per_chip[i].core_array[c].num_procs = 0;
            mem_details_per_chip[i].inuse_core_array[c].num_procs = 0;
            for(int cpu = 0;cpu < MAX_CPUS_PER_CORE;cpu++){
                mem_details_per_chip[i].core_array[c].lprocs[cpu] = -1;
                mem_details_per_chip[i].inuse_core_array[c].lprocs[cpu] = -1;
            }    
        }
        if(!mem_details_per_chip[i].has_cpu_and_mem){
            sysptr->unbalanced_sys_config = 1;
        }
		/* Fill page level details for SRAD/NUMA considering 50 % as 4k and 64K in AIX(due to AIX limitation of getting 4k and 64k details per srad**/
		for(pi=0;pi<MAX_PAGE_SIZES; pi++){
			if(memptr->pdata[pi].supported == TRUE) {
                /*On larger systems, if cache exer is running,limit 1 huge page per cpu for mem exer to avoid race condition prob in mem allocation of hugepages*/
                if((g_data.test_type == MEM) && (strtoul(oth_exer_hugepage_users,NULL, 10) > 0) && (pi >= PAGE_INDEX_2M) &&
                    (sys_mem_info.mem_pools[i].page_info_per_mempool[pi].free_pages > mem_details_per_chip[i].num_cpus)){
                    free_pages = mem_details_per_chip[i].num_cpus;
                }else{
                    free_pages = sys_mem_info.mem_pools[i].page_info_per_mempool[pi].free_pages;
                }
#ifndef __HTX_LINUX__ 
				if((pi < PAGE_INDEX_2M)){/*use explicit page indexes check*/
					free_mem = (mem_details_per_chip[i].memory_details.total_mem_free * 0.5);
					free_mem = free_mem/memptr->pdata[pi].psize;
					free_mem = free_mem * memptr->pdata[pi].psize;
					
					tot_mem  = (mem_details_per_chip[i].memory_details.total_mem_avail * 0.5);
					tot_mem  = tot_mem / memptr->pdata[pi].psize;
					tot_mem  = tot_mem * memptr->pdata[pi].psize;
				}else{
                    free_mem = free_pages * memptr->pdata[pi].psize;
                    tot_mem  = sys_mem_info.mem_pools[i].page_info_per_mempool[pi].total_pages * memptr->pdata[pi].psize;
				}
#else
				free_mem = free_pages * memptr->pdata[pi].psize;
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
	/* below code is to take care of chips having only memory but no cpus (Cache exerciser)*/
    int srad_counter=0,s=0;
	int srad_map[MAX_CHIPS];
	for ( int srad=0; srad < sysptr->num_chip_mem_pools; srad++){
		srad_map[srad] = -1;
		if (g_data.sys_details.chip_mem_pool_data[srad].num_cpus > 0) {
			srad_map[s++]=srad;
		}
	}
	s = 0;
    for( node = 0; node < MAX_NODES && node < hw_stat.nodes ; node++ ) {
		if(global_ptr->syscfg.node[node].num_chips > 0) {
	    	for(chip = 0; chip < MAX_CHIPS && chip < global_ptr->syscfg.node[node].num_chips; chip++ ) {	        
	        	if(global_ptr->syscfg.node[node].chip[chip].num_cores > 0) { 
		   			for(core = 0; core < MAX_CORES && core < global_ptr->syscfg.node[node].chip[chip].num_cores; core++ ) {
						if(global_ptr->syscfg.node[node].chip[chip].core[core].num_procs > 0) {
			    			for(lcpu=0;lcpu < MAX_CPUS &&  lcpu < global_ptr->syscfg.node[node].chip[chip].core[core].num_procs;lcpu++){
				
								sysptr->node[node].chip[chip].core[core].lprocs[lcpu] = global_ptr->syscfg.node[node].chip[chip].core[core].lprocs[lcpu];
		   	   					sysptr->node[node].chip[chip].core[core].num_procs++;
                                /*update chip_mem_pool structure ass well*/
								srad_counter = srad_map[s];
								if(srad_counter == -1){
									displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: srad mapping to syscfg info is wrong, please check srad/numa o/p with syscfg!..exiting\n");
									return (FAILURE);
								}
                                g_data.sys_details.chip_mem_pool_data[srad_counter].core_array[core].lprocs[lcpu] = global_ptr->syscfg.node[node].chip[chip].core[core].lprocs[lcpu];
                                g_data.sys_details.chip_mem_pool_data[srad_counter].core_array[core].num_procs++;
								debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"sysptr->node[%d].chip[%d].core[%d].lprocs[%d]=%d\n",node,chip,
								core,lcpu,sysptr->node[node].chip[chip].core[core].lprocs[lcpu]);

     			    		}
		     		        sysptr->node[node].chip[chip].num_cores++;
                            g_data.sys_details.chip_mem_pool_data[srad_counter].num_cores++;
		   				}
		   			}	
                    memcpy(&sysptr->node[node].chip[chip].mem_details,&g_data.sys_details.chip_mem_pool_data[srad_counter].memory_details,sizeof(struct mem_info));         	
                    s++;
                    sysptr->node[node].num_chips++;
                    sysptr->node[node].num_cores += sysptr->node[node].chip[chip].num_cores;
				}
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
	rc = fscanf(fp,"%lu",&sysptr->shmmax);
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

    if (g_data.exit_flag == SET) {
        return(-1);
    } 

    g_data.rf_last_mod_time = 0;
	if ((read_rules()) < 0 ) {
		exit(1);/*RV3: check for error possibility*/
	}

    if (g_data.exit_flag == SET) {
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
			displaym(HTX_HE_INFO,DBG_MUST_PRINT,"\nsegment details logged into %s/mem_segment_details\n",g_data.htx_d.htx_exer_log_dir);
			break;
		case CACHE:
			displaym(HTX_HE_INFO,DBG_MUST_PRINT,"\ncache test current stanza details logged into %s/cache%d_log\n",g_data.htx_d.htx_exer_log_dir,g_data.dev_id);
			break;
		
		case TLB:
            mem_g.memory_allocation = DO_NOT_ALLOCATE_MEM;
			break;

		case FABRICB:
            if(g_data.sys_details.shared_proc_mode == 1){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: Fabricbus exerciser does not support shared processor configured systems, exiting..\n",
                    __LINE__,__FUNCTION__);
                exit(1);
            }
			displaym(HTX_HE_INFO,DBG_MUST_PRINT,"\nsegment details logged into %s/mem_segment_details\n",g_data.htx_d.htx_exer_log_dir);
            ret_code = memory_segments_calculation();
			if(ret_code != SUCCESS){
					exit(1);
			}
			break;

		default:
			displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:wrong test type,it must be one of - /mem/cache/tlbie/fabricb \n",
					__LINE__,__FUNCTION__);			
			return(FAILURE);
	}
    ret_code = nest_framework_fun();
    if(ret_code != SUCCESS){
            exit(1);
    }

	return(SUCCESS);
}/*end of main*/

int nest_framework_fun(){

    int rc,prev_debug_level;

    mem_g.shm_cleanup_done  = 0;
    g_data.exit_flag        = 0;
    rc = atexit((void*)remove_shared_memory);
    if(rc != 0) {
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d] Error: Could not register for atexit!\n",__LINE__);
        return ( rc );
    }

    /*Register SIGRECONFIG handler */
    if(g_data.htx_d.htx_dr != 1){
#ifndef __HTX_LINUX__
        g_data.sigvector.sa_handler = (void (*)(int)) SIGRECONFIG_handler;
        sigaction(SIGRECONFIG, &g_data.sigvector, (struct sigaction *) NULL);
#endif
#ifdef __HTX_LINUX__
        /* Register SIGUSR2 for handling CPU hotplug */
        g_data.sigvector.sa_handler = (void (*)(int)) SIGUSR2_hdl;
        sigaction(SIGUSR2, &g_data.sigvector, (struct sigaction *) NULL);

#endif
    }
    /* update global pointer */
    g_data.test_type_ptr = (struct mem_test_info*)&mem_g;

    /* If cpus are in dedicated mode, disable cpu/mem filter usage, instaed use full system mem view*/
    if(g_data.sys_details.shared_proc_mode == 1){
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"shared proc mode detected,disabling cpu/mem filter usage\n");
        g_data.gstanza.global_disable_filters = 1;
    }
    /* Check if AME is enabled on system */
#ifndef __HTX_LINUX__
    rc =  check_ame_enabled();
    if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:check_ame_enabled()failed wit rc = %d\n",
            __LINE__,__FUNCTION__,rc);
        return (FAILURE);
    }
#endif

    /* Checking if filesystem is on RAM if yes dma_flag would be set */
    rc = check_if_ramfs();
    if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:check_if_ramfs() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
        return(FAILURE);
    }
    rc = fill_exer_huge_page_requirement();
    if(rc != SUCCESS){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_exer_huge_page_requirement() failed wit rc = %d\n",
            __LINE__,__FUNCTION__,rc);
        return (FAILURE);
    }
    if (g_data.exit_flag == SET) {
        exit(1);
    }
    rc = fill_and_allocate_memory_segments();
    if(rc != SUCCESS){
        return (FAILURE);
    }
    pthread_t stats_th_tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    rc = pthread_create((pthread_t *)&stats_th_tid,&attr,stats_update_thread_function,NULL);
    if(rc != 0){
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_create failed with rc = %d (errno %d):(%s) while creating stats update thread"
            "\ncontinuing without update of stats\n",__LINE__,__FUNCTION__,rc,errno,strerror(errno));
    }
    do {
        STATS_VAR_INIT(test_id, 0)
        g_data.stanza_ptr = &g_data.stanza[0];

        while( strcmp(g_data.stanza_ptr->rule_id,"NONE") != 0)
        {
            if(cpu_add_remove_test == 1){
                g_data.stanza_ptr->disable_cpu_bind = 1;
            }
            prev_debug_level = g_data.gstanza.global_debug_level;
            /* Set the debug level to the debug level specified in this stanza */
            if(!g_data.standalone){
                g_data.gstanza.global_debug_level = g_data.stanza_ptr->debug_level;
            }
            rc = (*exer_stanza_operation_fun[g_data.test_type])();

            g_data.gstanza.global_debug_level = prev_debug_level;
            if(rc != SUCCESS){
                remove_shared_memory();
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:exiting ...\n",__LINE__,__FUNCTION__);
                exit(1);
            }
            if (g_data.exit_flag == SET) {
                remove_shared_memory();
                exit(0);
            }

            if(g_data.thread_ptr != NULL){
                free(g_data.thread_ptr);
            }
            if(mem_g.mem_th_data != NULL){
                free(mem_g.mem_th_data);
            }
            if(cache_g.cache_th_data != NULL){
                free(cache_g.cache_th_data);
            }
            g_data.stanza_ptr++;
        }
        STATS_HTX_UPDATE(FINISH);
    /*  if ((read_rules()) < 0 ) {
            remove_shared_memory();
            exit(1);
        }*/
        if (g_data.exit_flag == SET) {
            remove_shared_memory();
            return(FAILURE);
        } /* endif */
    }while (g_data.standalone != 1); /* while run type is not standalone (OTH) */

    rc = remove_shared_memory();
    if(rc != SUCCESS) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:remove_shared_memory()failed with rc = %d",__LINE__,__FUNCTION__,rc);
    }
    return(SUCCESS);
}
/*******************************************************************************************
*This function checks if the filesystem is located on RAM, if yes then dma_flag will be set*
*so that DMA case can be excluded in such case                                             *
*******************************************************************************************/
int check_if_ramfs(void)
{
    int filedes=0;
    unsigned int mode_flag;
    char pattern_nm[256];
    char* pat_ptr = getenv("HTXPATTERNS");
    mem_g.dma_flag = 0; /*intialize with zero*/
    if(pat_ptr == NULL){
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s(): env variable HTXPATTERNS is not set,pat_ptr=%p"
        ", thus exiting..\n",__LINE__,__FUNCTION__,pat_ptr);
        exit(1);
    }else{
        strcpy(pattern_nm,pat_ptr);
    }
    strcat(pattern_nm,"/HEXFF");
    mode_flag = S_IWUSR | S_IWGRP | S_IWOTH;
    errno = 0 ;

    debug(HTX_HE_INFO, DBG_IMP_PRINT, "check_if_ramfs : Entered check_if_ramfs \n");
    filedes = open(pattern_nm, O_DIRECT | O_RDONLY, mode_flag);
    if ( errno == 22) {
        displaym(HTX_HE_INFO, DBG_MUST_PRINT,"[%d]%s::open call returned with EINVAL(errno = %d).This can\
            be because if filesystem is located on RAM,then  O_DIRECT flag is considered as invalid, in such case we will skip DMA test\
            case. \n",__LINE__,__FUNCTION__,errno);

         mem_g.dma_flag = 1;
         errno = 0 ;
    }

    if(filedes > 0 ) {
           debug(HTX_HE_INFO, DBG_IMP_PRINT, "check_if_ramfs : closing filedes = %d \n",filedes);
           close (filedes);
    }

    return(SUCCESS);
}

#ifndef __HTX_LINUX__
int check_ame_enabled()
{
    int rc = SUCCESS;
    lpar_info_format2_t info;

    rc = lpar_get_info(LPAR_INFO_FORMAT2, &info, sizeof(info));
    if (rc) {
        displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT, "[%d]%s:lpar_get_info failed with rc=%d,errno: %d\n", __LINE__,__FUNCTION__,rc,errno);
        return (FAILURE);
    }
    if ((info.lpar_flags & LPAR_INFO2_AME_ENABLED) == LPAR_INFO2_AME_ENABLED) {
        mem_g.AME_enabled = 1;
    } else {
        mem_g.AME_enabled = 0;
    }
    return rc;
}
#endif

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
        if((g_data.test_type == CACHE) && (i != g_data.dev_id)){
            continue;
        }
        if(mem_details_per_chip[i].has_cpu_and_mem){
            sprintf(msg_temp,"\nchip no:%d\n\tcores:%d\t\tcpus:%d",i,mem_details_per_chip[i].num_cores,mem_details_per_chip[i].num_cpus);
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
            if(strlen(msg) >= (2*KB)){
                displaym(HTX_HE_INFO,DBG_MUST_PRINT,"%s\n",msg);
                msg[0]=0;/*move terminal char to start of array*/
            }
        }
    }
    displaym(msg_type,DBG_MUST_PRINT,"%s\n",msg);
}

void print_memory_allocation_seg_details(int msg_print_level){
    int pi,n;
    char msg[4096],msg_temp[1024];

    sprintf(msg,"=======rule id:%s, Mem segment details*========\n"
        "In rule file:alloc_mem_percent=%f, alloc_segment_size=%ld KB\n",
        g_data.stanza_ptr->rule_id,g_data.gstanza.global_alloc_mem_percent,(g_data.gstanza.global_alloc_segment_size/ KB));
    if(g_data.gstanza.global_disable_filters == 1){
        for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
            if(!g_data.sys_details.memory_details.pdata[pi].supported)continue;
            unsigned long long mem = ((pi < PAGE_INDEX_2M) ?
                (g_data.gstanza.global_alloc_mem_percent/100.0) * g_data.sys_details.memory_details.pdata[pi].free :
                g_data.sys_details.memory_details.pdata[pi].free);
            sprintf(msg_temp,"\n\tpage size:%s\t mem:%llu MB\t total segments=%d\n",
                page_size_name[pi],(mem/MB),g_data.sys_details.memory_details.pdata[pi].num_of_segments);
            strcat(msg,msg_temp);
            if(g_data.sys_details.memory_details.pdata[pi].page_wise_usage_mem > 0){
                sprintf(msg_temp,"\tIn use mem details:\n\tmem = %llu MB\t"
                    "\tnum_segs:%d\n",(g_data.sys_details.memory_details.pdata[pi].page_wise_usage_mem/MB)
                    ,g_data.sys_details.memory_details.pdata[pi].in_use_num_of_segments);
                strcat(msg,msg_temp);
            }
        }
    }else{
        for(n = 0; n <g_data.sys_details.num_chip_mem_pools;n++){
            if(!g_data.sys_details.chip_mem_pool_data[n].has_cpu_and_mem)continue;
            if((g_data.test_type == CACHE) && (n != g_data.dev_id)){
                continue;
            }
            sprintf(msg_temp,"Chip[%d]:cpus:%d\n",n,g_data.sys_details.chip_mem_pool_data[n].num_cpus);
            strcat(msg,msg_temp);
            if(g_data.test_type != CACHE){
                if(g_data.sys_details.chip_mem_pool_data[n].in_use_num_cpus > 0){
                    sprintf(msg_temp,"\tIn use cpus: %d,\t",g_data.sys_details.chip_mem_pool_data[n].in_use_num_cpus);
                    strcat(msg,msg_temp);
                    if(g_data.sys_details.chip_mem_pool_data[n].in_use_num_cpus != g_data.sys_details.chip_mem_pool_data[n].num_cpus){
                        for(int i=0;i<g_data.sys_details.chip_mem_pool_data[n].in_use_num_cpus;i++){
                            sprintf(msg_temp,":%d",g_data.sys_details.chip_mem_pool_data[n].in_use_cpulist[i]);
                            strcat(msg,msg_temp);
                        }
                    }
                }
            }else {
                struct core_info* core_ptr=NULL;
                if(g_data.sys_details.chip_mem_pool_data[n].inuse_num_cores){
                    core_ptr = g_data.sys_details.chip_mem_pool_data[n].inuse_core_array;
                }else{
                    core_ptr = g_data.sys_details.chip_mem_pool_data[n].core_array;
                }
                for(int c=0;c<g_data.sys_details.chip_mem_pool_data[n].num_cores;c++){
                    sprintf(msg_temp,"\ncore %d :",c);
                    strcat(msg,msg_temp);
                    for(int i=0;i<core_ptr[c].num_procs;i++){
                        sprintf(msg_temp,"  %d",core_ptr[c].lprocs[i]);
                        strcat(msg,msg_temp);
                    }
                }
            }
            for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
                if(!g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].supported)continue;
                unsigned long long mem = ((pi < PAGE_INDEX_2M) ?
                    (g_data.gstanza.global_alloc_mem_percent/100.0) * g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].free :
                    g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].free);
                sprintf(msg_temp,"\n\tpage size:%s\t mem:%llu MB\t total segments=%d\n",
                    page_size_name[pi],(mem/MB),g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].num_of_segments);
                strcat(msg,msg_temp);
                if(g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_usage_mem > 0){
                    sprintf(msg_temp,"\tIn use mem details:\n\tmem = %llu MB\t"
                        "\tnum_segs:%d\n",(g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_usage_mem/MB)
                        ,g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].in_use_num_of_segments);
                    strcat(msg,msg_temp);
                }
            }
            if(strlen(msg) >= (2*KB)){
                displaym(HTX_HE_INFO,msg_print_level,"%s\n",msg);
                msg[0]=0;/*move terminal char to start of array*/
            }
        }
    }
    displaym(HTX_HE_INFO,msg_print_level,"%s\n",msg);
}

int log_mem_seg_details(){

    char dump_file[100];
    FILE *fp;
    struct chip_mem_pool_info *chip = &g_data.sys_details.chip_mem_pool_data[0];
    struct mem_info* sys_mem_details = &g_data.sys_details.memory_details;
    int n,pi,seg,bind_cpu = -1;
    char temp_str[100];
    if(g_data.gstanza.global_disable_filters == 0){
        sprintf(temp_str,"CHIP LEVEL MEMORY EXERCISE");
    }else{
        sprintf(temp_str,"SYSTEM VIEW MEMORY EXERCISE,NO STRICT AFFINITY MAINTAINED");
    }
    sprintf(dump_file, "%s/mem_segment_details",g_data.htx_d.htx_exer_log_dir);

    fp = fopen(dump_file, "w");
    if ( fp == NULL) {
        displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s():Error opening %s file,errno:%d(%s),continuing without debug log file \n"
            ,__LINE__,__FUNCTION__,dump_file,errno,strerror(errno));
        return (FAILURE);
    }
    fprintf(fp, "STANZA:rule_id = %s, %s\n",g_data.stanza_ptr->rule_id,temp_str);
    fprintf(fp,"***************************************************************************"
                "***************************************************************************\n");
    fprintf(fp," In_use_seg_num\t page_size\tOwning_thread\tcpu_num \tmem_chip_num\t segment_id \t\t segment_size \t\t segment_EA\n");
    if(g_data.gstanza.global_disable_filters == 0){
        for(n = 0; n <g_data.sys_details.num_chip_mem_pools;n++){
            if(!chip[n].has_cpu_and_mem)continue;
            fprintf(fp,"========================================================================"
                        "========================================================================\n");
            for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
                for(seg=0;seg<chip[n].memory_details.pdata[pi].num_of_segments;seg++){
                    if(!chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].in_use)continue;
                        if(g_data.stanza_ptr->disable_cpu_bind == 0 && chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].owning_thread != -1){
                            bind_cpu = g_data.thread_ptr[chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].owning_thread].bind_proc;
                        }
                        fprintf(fp," \t%lu \t %s \t\t %d \t\t %d \t\t %d \t\t %lu \t\t %lu \t\t 0x%llx\n",
                            chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].seg_num,page_size_name[pi],
                            chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].owning_thread,
                            bind_cpu,
                            chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].mem_chip_num,
                            chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].shmid,
                            chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].shm_size,
                            (unsigned long long)chip[n].memory_details.pdata[pi].page_wise_seg_data[seg].shm_mem_ptr);
                }
            }
        }
    }else{
        fprintf(fp,"========================================================================"
                    "========================================================================\n");
        for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
            for(seg=0;seg<sys_mem_details->pdata[pi].in_use_num_of_segments;seg++){
                if(!sys_mem_details->pdata[pi].page_wise_seg_data[seg].in_use)continue;
                    if(g_data.stanza_ptr->disable_cpu_bind == 0 && sys_mem_details->pdata[pi].page_wise_seg_data[seg].owning_thread != -1){
                        bind_cpu = g_data.thread_ptr[sys_mem_details->pdata[pi].page_wise_seg_data[seg].owning_thread].bind_proc;
                    }
                    fprintf(fp," \t%lu \t %s \t\t %d \t\t %d \t\t %d \t\t %lu \t\t %lu \t\t 0x%llx\n",
                        sys_mem_details->pdata[pi].page_wise_seg_data[seg].seg_num,page_size_name[pi],
                        sys_mem_details->pdata[pi].page_wise_seg_data[seg].owning_thread,
                        bind_cpu,
                        sys_mem_details->pdata[pi].page_wise_seg_data[seg].mem_chip_num,
                        sys_mem_details->pdata[pi].page_wise_seg_data[seg].shmid,
                        sys_mem_details->pdata[pi].page_wise_seg_data[seg].shm_size,
                        (unsigned long long)sys_mem_details->pdata[pi].page_wise_seg_data[seg].shm_mem_ptr);
            }
        }
    }
    fprintf(fp,"****************************************************************************"
                "****************************************************************************\n");
    fclose(fp);
    return SUCCESS;
}	

void* stats_update_thread_function(void* null_ptr){
    int t=0;
    for(;;){
        while((t++) < STATS_UPDATE_INTERVAL){
            sleep(1);
            if (g_data.exit_flag == SET) {
                break;
            }
        }

        pthread_mutex_lock(&g_data.tmutex);
        g_data.htx_d.bytes_writ = g_data.nest_stats.bytes_writ;
        g_data.htx_d.bytes_read = g_data.nest_stats.bytes_read;
        g_data.htx_d.bad_others = g_data.nest_stats.bad_others;
        /*Reset nest stats variables*/
        g_data.nest_stats.bytes_writ = 0;
        g_data.nest_stats.bytes_read = 0;
        g_data.nest_stats.bad_others = 0;
        pthread_mutex_unlock(&g_data.tmutex);

        if((g_data.htx_d.bytes_writ > 0) || (g_data.htx_d.bytes_read > 0) || (g_data.htx_d.bad_others > 0)){
            STATS_HTX_UPDATE(UPDATE);
        }
        if (g_data.exit_flag == SET) {
            pthread_exit((void *)0);
        }
        t = 0;
    }

}

int fill_and_allocate_memory_segments(){
    int rc = SUCCESS;
    mem_g.total_segments = 0;/*initialize overall segments count to 0*/
    if(mem_g.memory_allocation == ALLOCATE_MEM){
        if(g_data.gstanza.global_disable_filters == 1){
            /* consider % of whole memory and fill segment details for all supported page size pools*/
            if(g_data.test_type != MEM){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: system level memory test is supported only in mem exer, test type=%d,(MEM=0,FAB,TLBIE,CACHE)\n"
                    "g_data.gstanza.global_disable_filters found to be 1, exititng..\n",__LINE__,__FUNCTION__,g_data.test_type,g_data.gstanza.global_disable_filters);
                return (FAILURE);
            }
            rc = fill_system_segment_details();
            if  ( rc != SUCCESS){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:fill_system__segment_details() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
                return (FAILURE);
            }
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Due to system configuration,Memory Exercising will happen at System level memory,\n"
                "Total threads to be created to get shared memory =%d\n",g_data.gstanza.global_num_threads);
        }else{
        /* consider % of per chip memory and fill segment details for all supported page size pools*/
            rc = (*memory_req_exer_fun[g_data.test_type])();
            if  ( rc != SUCCESS){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:mem requirement filling fun  failed with rc = %d for test = %d(mem=0,fabc=1,tlb=2,cache=3)\n",
                    __LINE__,__FUNCTION__,rc,g_data.test_type);
                return (FAILURE);
            }
            /* consider strict local affinity to allocate memory(only for memory exerciser)*/
            #ifndef __HTX_LINUX__
             if ( g_data.test_type == MEM){
                rc = system("vmo -o enhanced_affinity_vmpool_limit=-1");
                if(rc < 0){
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"vmo -o enhanced_affinity_vmpool_limit=-1"
                    "failed errno %d rc = %d\n",errno,rc);
                    return(FAILURE);
                }
                displaym(HTX_HE_INFO,DBG_IMP_PRINT,"vmo -o enhanced_affinity_vmpool_limit=-1 rc = %d,errno =%d\n",rc,errno);
            }
            #endif
            displaym(HTX_HE_INFO,DBG_MUST_PRINT,"Total threads to be created to get shared memory =%d\n",g_data.sys_details.tot_cpus);
        }
        #ifdef __HTX_LINUX__
        rc = modify_shared_mem_limits_linux(mem_g.total_segments);
        if  ( rc != SUCCESS){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:modify_shared_mem_limits_linux() failed with rc = %d\n",__LINE__,__FUNCTION__,rc);
            return (FAILURE);
        }
        #endif

        int current_priority = getpriority(PRIO_PROCESS,g_data.pid);
        if(current_priority == -1){
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:getpriority() failed with %d,while retrieving crrent priority of exer(pid : %llu) errno:%d(%s)\n",
                __LINE__,__FUNCTION__,current_priority,g_data.pid,errno,strerror(errno));
        }
        rc = setpriority(PRIO_PROCESS,g_data.pid,0);/* boost exer priority to zero temporarily so that mem allocation finishes quickly*/
        if(rc == -1){
            displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:setpriority() failed with %d while setting priority to 0, errno:%d(%s)\n",
                __LINE__,__FUNCTION__,rc,errno,strerror(errno));
        }
        print_memory_allocation_seg_details(DBG_MUST_PRINT);
        /*create shared memories by spawning multiple threads*/
        rc = get_shared_memory();
        if(rc != SUCCESS) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:get_shared_memory() failed with rc = %d",__LINE__,__FUNCTION__,rc);
            return (rc);
        }
        if (g_data.exit_flag == SET) {
            remove_shared_memory();
            exit(1);
        }

        if(current_priority != -1){
            rc = setpriority(PRIO_PROCESS,g_data.pid,current_priority);
            if(rc == -1){
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:setpriority() failed with %d,while setting priority back to %d  errno:%d(%s)\n",
                     __LINE__,__FUNCTION__,rc,current_priority,errno,strerror(errno));
            }
        }
    }
    return rc;
}

/*********************************************************************************************
* get_shared_memory(pi),spawn threads equal to cpus per chip to create shared memory segments
* for all supported page size pols
*********************************************************************************************/
int get_shared_memory(){
    int ti,n,rc,nchips;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    int tot_sys_threads = g_data.sys_details.tot_cpus;
    int cpus_track=0,mem_allocated=0;

    shm_alloc_th = (struct shm_alloc_th_data*) malloc(sizeof( struct shm_alloc_th_data) * tot_sys_threads);
    if(shm_alloc_th == NULL){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s():malloc failed for thread_mem_alloc_data for with errno:%d(%s)size=%llu\n",
            __LINE__,__FUNCTION__,errno,strerror(errno),(sizeof(struct shm_alloc_th_data) * tot_sys_threads));
        return(FAILURE);
    }

    if(g_data.gstanza.global_disable_filters == 0){
        nchips = g_data.sys_details.num_chip_mem_pools;
        for(n=0;n<nchips;n++){
            if(!mem_details_per_chip[n].has_cpu_and_mem){
                displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s:chip%d either does not have memory or cpu behind it" \
                        " free mem = %llu and total cpus = %d\n",__LINE__,__FUNCTION__,n,
                        mem_details_per_chip[n].memory_details.total_mem_free,mem_details_per_chip[n].num_cpus);
                cpus_track  += mem_details_per_chip[n].num_cpus;
                continue;
            }
            for(ti=0;ti<mem_details_per_chip[n].num_cpus;ti++){
                shm_alloc_th[cpus_track+ti].thread_num = (cpus_track+ti);
                shm_alloc_th[cpus_track+ti].chip_num   = n;
                shm_alloc_th[cpus_track+ti].bind_cpu_num   = mem_details_per_chip[n].cpulist[ti];
                shm_alloc_th[cpus_track+ti].chip_cpu_num = ti;
                shm_alloc_th[cpus_track+ti].tid      = -1;
                 debug(HTX_HE_INFO,DBG_IMP_PRINT,"Creating thread %d to allocate memory by cpu = %d from chip %d\n",(cpus_track+ti),shm_alloc_th[cpus_track+ti].bind_cpu_num,n);
                rc = pthread_create((pthread_t *)&(shm_alloc_th[cpus_track+ti].tid),NULL, get_shm,(void *)&(shm_alloc_th[cpus_track+ti]));
                /*printf("##########Pthread_create : cpus_track=%d, ti=%d, tid=%llx\n",  cpus_track, ti, shm_alloc_th[cpus_track+ti].tid);*/
                if ( rc != 0){
                    displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:pthread_create failed rc = %d and errno = %d(%s) for thread %d\n",
                            __LINE__,__FUNCTION__,rc,errno,strerror(errno),cpus_track+ti);
                    return (FAILURE);
                }
                mem_allocated = 1;
            }
            cpus_track  += mem_details_per_chip[n].num_cpus;
            if (g_data.exit_flag == SET) {
                remove_shared_memory();
                exit(1);
            }
        }
        cpus_track=0;
        for(n=0;n<nchips;n++){
            if(!mem_details_per_chip[n].has_cpu_and_mem){
                cpus_track  += mem_details_per_chip[n].num_cpus;
                continue;
            }
            for(ti=0;ti<mem_details_per_chip[n].num_cpus;ti++){
                /*printf("********Joining th. cpus_track=%d, ti=%d, tid=%llx\n", cpus_track, ti, shm_alloc_th[cpus_track+ti].tid);*/
                rc = pthread_join(shm_alloc_th[cpus_track+ti].tid,NULL);
                if(rc != 0) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join failed rc = %d and errno = %d(%s)for thread %d\n",
                        __LINE__,__FUNCTION__,rc,errno,strerror(errno),cpus_track+ti);
                    return (FAILURE);
                }
                else {
                    /* debug(HTX_HE_INFO,DBG_DEBUG_PRINT," Thread %d  joined successfully after shm allocation\n",cpus_track+ti); */
                    shm_alloc_th[cpus_track+ti].tid          = -1;
                }
            }
            cpus_track  += mem_details_per_chip[n].num_cpus;

            if (g_data.exit_flag == SET) {
                remove_shared_memory();
                exit(1);
            }
        }
        displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"\n********Shared Memory allocation Completed for CHIP: %d ***************************\n",n);

        if(g_data.test_type == CACHE){
            if(mem_allocated == 0){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: Memory is not allocated from any of chips,needs investigation...exiting\n",__LINE__,__FUNCTION__);
                return (FAILURE);
            } 
            rc = setup_memory_to_use();
            if(rc != SUCCESS){
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: setup_memory_to_use() failed with rc =%d\n",__LINE__,__FUNCTION__,rc);
                return(FAILURE);
            }
            return rc;
        }
    }else {
        tot_sys_threads = g_data.gstanza.global_num_threads;
        for(ti=0;ti<tot_sys_threads;ti++){
            shm_alloc_th[ti].thread_num = ti;
            shm_alloc_th[ti].chip_num   = -1;
            shm_alloc_th[ti].bind_cpu_num = ti;
            shm_alloc_th[ti].tid = -1;
            debug(HTX_HE_INFO,DBG_IMP_PRINT,"Creating thread %d to allocate memory by cpu = %d\n",ti,shm_alloc_th[ti].bind_cpu_num);
            rc = pthread_create((pthread_t *)&(shm_alloc_th[ti].tid),NULL, get_shm,(void *)&(shm_alloc_th[ti]));
            if(rc != 0){
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:pthread_create failed rc = %d and errno = %d(%s) for thread %d\n",
                        __LINE__,__FUNCTION__,rc,errno,strerror(errno),ti);
                return (FAILURE);
            }
        }
        if (g_data.exit_flag == SET) {
            remove_shared_memory();
            exit(1);
        }

        for(ti=0;ti<tot_sys_threads;ti++){
           rc = pthread_join(shm_alloc_th[ti].tid,NULL);
            if(rc != 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join failed rc = %d and errno = %d(%s)for thread %d\n",
                    __LINE__,__FUNCTION__,rc,errno,strerror(errno),ti);
                return (FAILURE);
            }
            else {
                /* debug(HTX_HE_INFO,DBG_DEBUG_PRINT," Thread %d  joined successfully after shm allocation\n",ti); */
                shm_alloc_th[ti].tid          = -1;
            }
        }

        if (g_data.exit_flag == SET) {
            remove_shared_memory();
            exit(1);
        }
    }
    return(SUCCESS);
}
int remove_shared_memory(){

    int n,ti,rc,pi;
    int nchips = g_data.sys_details.num_chip_mem_pools,cpus_track=0;
    int tot_sys_threads = g_data.sys_details.tot_cpus;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    if(shm_alloc_th == NULL){
        return SUCCESS;
    }
    if(g_data.gstanza.global_disable_filters == 0){
        for(n=0;n<nchips;n++){
            if(!mem_details_per_chip[n].has_cpu_and_mem){
                cpus_track  += mem_details_per_chip[n].num_cpus;
                continue;
            }
            for(ti=0;ti<mem_details_per_chip[n].num_cpus;ti++){
                debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"Creating thread %d to deallocate memory \n",ti);
                rc = pthread_create((pthread_t *)&shm_alloc_th[cpus_track+ti].tid,NULL,remove_shm,(void *)&shm_alloc_th[cpus_track+ti]);
                if(rc !=0 ){
                    displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:pthread_create failed rc = %d and errno = %d(%s)\n",__LINE__,__FUNCTION__,rc,errno,strerror(errno));
                    return (FAILURE);
                }
            }
            cpus_track      += mem_details_per_chip[n].num_cpus;
        }
        cpus_track = 0;
        for(n=0;n<nchips;n++){
            if(!mem_details_per_chip[n].has_cpu_and_mem){
                cpus_track  += mem_details_per_chip[n].num_cpus;
                continue;
            }
            for(ti=0;ti<mem_details_per_chip[n].num_cpus;ti++){
                rc = pthread_join(shm_alloc_th[cpus_track+ti].tid,NULL);
                if(rc != 0) {
                    displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join failed rc = %d and errno = %d(%s)"
                        " for thread %d\n",__LINE__,__FUNCTION__,rc,errno,strerror(errno),cpus_track+ti);
                    return (FAILURE);
                }
            }
            cpus_track      += mem_details_per_chip[n].num_cpus;
            for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
                if(!g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].supported)continue;
                if(g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_seg_data != NULL){
                    free(g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_seg_data);
                }
            }
        }
        #ifndef __HTX_LINUX__
        if ( g_data.test_type == MEM){
            rc = system("vmo -d enhanced_affinity_vmpool_limit");
            displaym(HTX_HE_INFO,DBG_IMP_PRINT,"%s():vmo -d enhanced_affinity_vmpool_limit rc=%d \t errno =%d\n",\
            __FUNCTION__,rc,errno);
        }
        #endif
        /* clear cache exer contiguous mem info malloc memory*/
        if(g_data.test_type == CACHE){
            for(int i=0;i<cache_g.cache_pages.num_pages;i++){
                if(cache_g.cont_mem[i].huge_pg != NULL){
                    free(cache_g.cont_mem[i].huge_pg);
                }
            }
            if(cache_g.cont_mem != NULL){
                free(cache_g.cont_mem);
            }
        }
    }else{
        tot_sys_threads = g_data.gstanza.global_num_threads;
        for(ti=0;ti<tot_sys_threads;ti++){
            debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"Creating thread %d to deallocate memory \n",ti);
            rc = pthread_create((pthread_t *)&shm_alloc_th[ti].tid,NULL,remove_shm,(void *)&shm_alloc_th[ti]);
            if(rc !=0 ){
                displaym(HTX_HE_HARD_ERROR, DBG_MUST_PRINT,"[%d]%s:pthread_create failed rc = %d and errno = %d(%s),thread_num=%d\n",
                    __LINE__,__FUNCTION__,rc,errno,strerror(errno),ti);
                return (FAILURE);
            }
        }
        for(ti=0;ti<tot_sys_threads;ti++){
            rc = pthread_join(shm_alloc_th[ti].tid,NULL);
            if(rc != 0) {
                displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:pthread_join failed rc = %d and errno = %d(%s)"
                    " for thread %d\n",__LINE__,__FUNCTION__,rc,errno,strerror(errno),ti);
                 return (FAILURE);
            }
        }
        for(pi = 0;pi<MAX_PAGE_SIZES;pi++){
            if(!g_data.sys_details.memory_details.pdata[pi].supported)continue;
            if(g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data != NULL){
                free(g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data);
            }
        }
    }
    if(shm_alloc_th != NULL){
        free(shm_alloc_th);
        shm_alloc_th = NULL;
    }
    mem_g.shm_cleanup_done = 1;
    return SUCCESS;
}

void* get_shm(void* t){
    struct shm_alloc_th_data *th = (struct shm_alloc_th_data *)t;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    struct mem_info* sys_mem_details = &g_data.sys_details.memory_details;
    int n = th->chip_num;
    int rc,pi,seg_num,num_segs=0,seg_inc = 0;;
    int tot_cpus = g_data.gstanza.global_num_threads;
    if(!g_data.gstanza.global_disable_cpu_bind){
    /*replace below call with bind_to_cpu,common betweeen AIX/LINUX*/
    #ifndef __HTX_LINUX__
        if (g_data.cpu_DR_flag != 1){
            rc = htx_bind_thread(th->bind_cpu_num,th->bind_cpu_num);
        }
    #else
        rc = htx_bind_thread(th->bind_cpu_num,-1);
    #endif

        if(rc < 0){/*REV: hotplug/dr need to take care*/
                displaym(HTX_HE_INFO,DBG_MUST_PRINT,"bind call failed for cpu %d (th:%d),local memory may not be guaranteed\n",th->bind_cpu_num,th->thread_num);
        }
    }
    for(pi=0;pi<MAX_PAGE_SIZES; pi++) {
        if(!sys_mem_details->pdata[pi].supported){
            continue;
        }
        if(g_data.gstanza.global_disable_filters == 0){
            /*segs per chip per page pool will divided among threads of same chip,such that each seg index will be picked up by a cpu (segment_index%total_cpus_in_that_chip)*/

            /*cache port chnages*/
            /*if(mem_details_per_chip[n].memory_details.pdata[pi].free == 0){
                continue;
            }*/
            num_segs = mem_details_per_chip[n].memory_details.pdata[pi].num_of_segments;
            seg_num = th->chip_cpu_num % mem_details_per_chip[n].num_cpus;
            seg_inc = mem_details_per_chip[n].num_cpus;
        }else{
            if(sys_mem_details->pdata[pi].free == 0){
                continue;
            }
            num_segs = sys_mem_details->pdata[pi].num_of_segments;
            seg_num = th->thread_num % tot_cpus;
            seg_inc = tot_cpus;
        }
        while(seg_num < num_segs ){
            /*displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_num=%d of page pool=%s in chip %d will be aloocated by thread %d,cpu %d\n",seg_num,page_size_name[pi],n,th->thread_num,th->bind_cpu_num);*/
            rc = allocate_buffers(pi,seg_num,th);
            if(rc != SUCCESS){
                g_data.exit_flag = SET;
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:allocate_buffers failed with rc = %d for allocation thread %d, page pool = %s\n", \
                                __LINE__,__FUNCTION__,rc,th->thread_num,page_size_name[pi]);
                pthread_exit((void *)1);
            }
            /*seg_num +=  mem_details_per_chip[n].num_cpus;*/
            seg_num += seg_inc;
        }
    }

    if(!g_data.gstanza.global_disable_cpu_bind){
        rc = htx_unbind_thread();
    }
    pthread_exit((void *)0);
}

void* remove_shm(void* t){

    struct shm_alloc_th_data *th = (struct shm_alloc_th_data *)t;
    struct chip_mem_pool_info* mem_details_per_chip  = &g_data.sys_details.chip_mem_pool_data[0];
    struct mem_info* sys_mem_details = &g_data.sys_details.memory_details;
    int n = th->chip_num;
    int rc,pi,seg_num=0,num_segs=0,seg_inc = 0;;
    int tot_cpus = g_data.gstanza.global_num_threads;

    for(pi=0;pi<MAX_PAGE_SIZES; pi++) {
        if(!sys_mem_details->pdata[pi].supported){
            continue;
        }
        if(g_data.gstanza.global_disable_filters == 0){
            /*if(mem_details_per_chip[n].memory_details.pdata[pi].free == 0){
                continue;
            }*/
            num_segs = mem_details_per_chip[n].memory_details.pdata[pi].num_of_segments;
            seg_num=0;
            seg_num = th->chip_cpu_num % mem_details_per_chip[n].num_cpus;
            seg_inc = mem_details_per_chip[n].num_cpus;
        }else{
            if(sys_mem_details->pdata[pi].free == 0){
                continue;
            }
            num_segs = sys_mem_details->pdata[pi].num_of_segments;
            seg_num=th->thread_num % tot_cpus;
            seg_inc = tot_cpus;
        }
        while(seg_num < num_segs ){
            /*displaym(HTX_HE_INFO,DBG_DEBUG_PRINT,"seg_num=%d of page pool=%s in chip %d will be aloocated by thread %d,cpu %d\n",seg_num,page_size_name[pi],n,th->thread_num,th->bind_cpu_num);*/
            rc = deallocate_buffers(pi,seg_num,th);
            if(rc != SUCCESS){
                displaym(HTX_HE_SOFT_ERROR,DBG_MUST_PRINT,"[%d]%s:deallocate_buffers failed with rc = %d for allocation thread %d, page pool = %s\n", \
                                __LINE__,__FUNCTION__,rc,th->thread_num,page_size_name[pi]);
                pthread_exit((void *)1);
            }
            seg_num += seg_inc;
        }
    }
    pthread_exit((void *)0);
}

int allocate_buffers(int pi, int seg_num, struct shm_alloc_th_data *th){
    unsigned long i, memflg;
    int n = th->chip_num;
    int rc=SUCCESS,bound_cpu = th->bind_cpu_num;
    struct shmid_ds shm_buf = { 0 };
    struct page_wise_seg_info *seg_details = NULL;

    if(g_data.gstanza.global_disable_filters == 0){
        seg_details = &g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_seg_data[seg_num];
    }else{
        seg_details = &g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg_num];
    }

    if(g_data.gstanza.global_disable_cpu_bind){
        bound_cpu = -1;
    }
#ifndef __HTX_LINUX__
    memflg = (IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
#else
    memflg = (IPC_CREAT | IPC_EXCL |SHM_R | SHM_W);
    if(pi >= PAGE_INDEX_2M) {
        memflg |= (SHM_HUGETLB);
    }
#endif
    if (g_data.exit_flag == SET) {
         pthread_exit((void *)1);
    } /* endif */
    seg_details->shmid = shmget (IPC_PRIVATE, seg_details->shm_size, memflg);
    if(seg_details->shmid  == -1){
        char msg[1024];
        sprintf(msg,"[%d]%s:shmget failed with -1 by thread = %d(bound to cpu:%d) for size=%lu,seg_num=%d,page pool=%s,errno:%d(%s)\n", \
                __LINE__,__FUNCTION__,th->thread_num,bound_cpu,seg_details->shm_size,seg_num,page_size_name[pi],errno,strerror(errno));
        if(errno == ENOMEM){
            strcat(msg,"Looks like previous htx run was not a clean exit,Please check if any stale shared memory chunks are present"
                ",clear them or reboot the partition, make sure desired free memory is avialable in the system before re starting the test.\n");
        }else if(errno == EINVAL){
            strcat(msg,"Unable to create new segment as the size is less than SHMMIN or greater than SHMMAX."
                " Modify shm parameters and run again\n");
        }
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"%s\n",msg);
        return (FAILURE);
    }
    else {
        debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"(shmget success)thread:%d for page:%s, shm id[%d]=%lu and size = %lu\n",
                    th->thread_num,page_size_name[pi],seg_num,seg_details->shmid,seg_details->shm_size);
    }
#ifndef __HTX_LINUX__
    if ( seg_details->shm_size > (256*MB)) {
        if (shmctl(seg_details->shmid,SHM_GETLBA, &shm_buf)) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: shmctl failed with err:(%d)%s, for seg_num = %d,shm_id=%lu while setting SHM_GETLBA"
                "for page = %s and segment_size = %lu \n",__LINE__,__FUNCTION__,errno,strerror(errno),seg_num,seg_details->shmid,page_size_name[pi],seg_details->shm_size);
            return  (FAILURE);
        }
    }
    shm_buf.shm_pagesize = g_data.sys_details.memory_details.pdata[pi].psize;

    if (shmctl(seg_details->shmid,SHM_PAGESIZE, &shm_buf)) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:shmctl failed with err:(%d)%s,for seg_num = %d,shm_id=%lu,thread=%d while setting SHM_PAGESIZE"
                    " for page = %s and segment_size = %lu \n",__LINE__,__FUNCTION__,errno,strerror(errno),seg_num,seg_details->shmid,th->thread_num,page_size_name[pi],seg_details->shm_size);
        return  (FAILURE);
    }
#endif
    seg_details->shm_mem_ptr = (void *)shmat(seg_details->shmid,(char *) 0, 0);
    if (seg_details->shm_mem_ptr  == (void *)-1) {
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:shmat failed with err:(%d)%s,for seg_num = %d,shm_id=%lu while setting SHM_PAGESIZE"
                 "for page = %s and segment_size = %lu \n",__LINE__,__FUNCTION__,errno,strerror(errno),seg_num,seg_details->shmid,page_size_name[pi],seg_details->shm_size);
        return (FAILURE);
    }
    debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"(shmat success)Allocated/Attached  shared memory buffer%d, id = %lu, shm_memp = "
            "0x%lx and page size =%s by thread : %d\n",seg_num,seg_details->shmid,seg_details->shm_mem_ptr,page_size_name[pi],th->thread_num);

    /* pin  hugepages backed memory chunk  shm area into memory(only in case cache exer) */
    if((g_data.test_type == CACHE) && (pi >= PAGE_INDEX_2M)) {
#ifndef __HTX_LINUX__
        rc = mlock(seg_details->shm_mem_ptr,seg_details->shm_size);
        if(rc == -1){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:mlock failed with err:(%d)%s,for seg_num = %d,adress=%p for page = %s and segment_size = %lu \n",
                __LINE__,__FUNCTION__,errno,strerror(errno),seg_num,seg_details->shm_mem_ptr,page_size_name[pi],seg_details->shm_size);
            return (FAILURE);
        }
#else
        if ((rc = shmctl(seg_details->shmid,SHM_LOCK,&shm_buf)) == -1) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:shmctl failed with err:(%d)%s,for seg_num = %d,shm_id=%lu,thread=%d while setting SHM_LOCK"
                " for page = %s and segment_size = %lu \n",__LINE__,__FUNCTION__,errno,strerror(errno),seg_num,seg_details->shmid,th->thread_num,page_size_name[pi],seg_details->shm_size);
            return  (FAILURE);
        }
#endif
    }
    /* write 8 byte value to every page of this segment*/
    long long* temp_ptr = (long long*) seg_details->shm_mem_ptr;
    unsigned long num_pages =   seg_details->shm_size/g_data.sys_details.memory_details.pdata[pi].psize;
    for(i =0; i < num_pages; i++)
    {
        *temp_ptr = 0xBBBBBBBBBBBBBBBBULL;
        temp_ptr += (g_data.sys_details.memory_details.pdata[pi].psize/8);

    }
    /* initialize other parameters of segment management structure*/
    seg_details->owning_thread   = -1;
    seg_details->page_size_index = pi;
    seg_details->in_use          = 0;
    seg_details->cpu_chip_num        = -1;
    seg_details->mem_chip_num        = th->chip_num;

    return (SUCCESS);
}
int deallocate_buffers(int pi, int seg_num, struct shm_alloc_th_data *th){

    int rc;
    int n = th->chip_num;
    struct page_wise_seg_info *seg_details = NULL;

    if(g_data.gstanza.global_disable_filters == 0){
        seg_details = &g_data.sys_details.chip_mem_pool_data[n].memory_details.pdata[pi].page_wise_seg_data[seg_num];
    }else{
        seg_details = &g_data.sys_details.memory_details.pdata[pi].page_wise_seg_data[seg_num];
    }
    if (seg_details->shm_mem_ptr != NULL) {/* If shared memory head pointer is allocated */
        debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"Removing segment #%d of page size %s by thread: %d "
            "shm_mem id=%lu, shm_memp = 0x%lx\n",seg_num,page_size_name[pi],th->thread_num,seg_details->shmid,seg_details->shm_mem_ptr);
        rc = shmdt(seg_details->shm_mem_ptr);
        if(rc != 0){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:(th:%d)shmdt failed with err:(%d)%s,at adress:0x%lx for seg_num = %d,shm_id=%lu for page = %s"
            "and segment_size = %lu \n",__LINE__,__FUNCTION__,th->thread_num,errno,strerror(errno),seg_details->shm_mem_ptr,seg_num,seg_details->shmid,page_size_name[pi],seg_details->shm_size);
            return (FAILURE);
        }
        rc = shmctl(seg_details->shmid,IPC_RMID,(struct shmid_ds *) NULL);
        if (rc != 0) {
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:(th:%d)shmctl failed with err:(%d)%s,for seg_num = %d,shm_id=%lu for page = %s"
            "and segment_size = %lu belongs to chip:%d\n",__LINE__,__FUNCTION__,th->thread_num,
            errno,strerror(errno),seg_num,seg_details->shmid,page_size_name[pi],seg_details->shm_size,n);
            return (FAILURE);
        }
        debug(HTX_HE_INFO,DBG_DEBUG_PRINT,"Released shared memory buffer %d id = %d, shm_memp = 0x%lx page size = %s\n",
        seg_num,seg_details->shmid,seg_details->shm_mem_ptr,page_size_name[pi]);
        seg_details->shm_mem_ptr = NULL;
    }
    return SUCCESS;
}

int apply_filters(){

    struct cpu_filter_info* cf = &g_data.stanza_ptr->filter.cf;
    struct mem_filter_info* mf = &g_data.stanza_ptr->filter.mf;
    struct chip_mem_pool_info* chp = &g_data.sys_details.chip_mem_pool_data[0];
    struct page_wise_data *page_wise_details=NULL;
    struct page_wise_data *sys_page_wise_details=NULL;
    int node,chip,core,lcpu,pi,seg,sys_chip_count = 0,rc=SUCCESS;
    long long mem_in_use = 0;

    if(g_data.gstanza.global_disable_filters == 0){
        g_data.stanza_ptr->num_threads = 0;
        for(chip =0; chip<MAX_CHIPS;chip++){
            chp[chip].in_use_num_cpus= 0;
            chp[chip].inuse_num_cores=0;
            chp[chip].is_chip_mem_in_use= FALSE;
            for(int c =0; c < MAX_CORES_PER_CHIP; c++){
                chp[chip].inuse_core_array[c].num_procs = 0;
                for(int cpu = 0;cpu < MAX_CPUS_PER_CORE;cpu++){
                    chp[chip].inuse_core_array[c].lprocs[cpu] = -1;
                }
            }
            for(int j=0;j<MAX_CPUS_PER_SRAD;j++){
                chp[chip].in_use_cpulist[j] = -1;
            }
            for(pi=0;pi<MAX_PAGE_SIZES;pi++){
                chp[chip].memory_details.pdata[pi].page_wise_usage_mem = 0;
                chp[chip].memory_details.pdata[pi].in_use_num_of_segments = 0;
            }

        }

        for( node = 0; node < g_data.sys_details.nodes; node++){
            if(cf->node[node].node_num == -1){
                sys_chip_count += g_data.sys_details.node[node].num_chips;
                continue;
            }
            for(chip = 0; chip < g_data.sys_details.node[node].num_chips; chip++){
                if(cf->node[node].chip[chip].chip_num == -1){/*if cpu filter tells not to consider this chip  then do not populate cpus*/
                    sys_chip_count++;
                    continue;
                }
                for(core = 0; core < MAX_CORES_PER_CHIP; core++){
                    if(cf->node[node].chip[chip].core[core].core_num == -1){
                        continue;
                    }
                    for(lcpu=0;lcpu < MAX_CPUS_PER_CORE;lcpu++){
                        if(cf->node[node].chip[chip].core[core].lprocs[lcpu] == -1){
                            continue;
                        }
						if(g_data.test_type == CACHE){
							sys_chip_count = cache_g.cache_instance;
						}
                        chp[sys_chip_count].in_use_cpulist[chp[sys_chip_count].in_use_num_cpus++] = cf->node[node].chip[chip].core[core].lprocs[lcpu];
                        chp[sys_chip_count].inuse_core_array[chp[sys_chip_count].inuse_num_cores].lprocs[chp[sys_chip_count].inuse_core_array[core].num_procs++] =
                            cf->node[node].chip[chip].core[core].lprocs[lcpu];
                        g_data.stanza_ptr->num_threads++;/* this will be overwritten in cache exer, since it has prefetch test case enballing control from rule*/
                    }
                    chp[sys_chip_count].inuse_num_cores++;
                }
                sys_chip_count++;
            }

        }

        sys_chip_count = 0;
        for( node = 0; node < get_num_of_nodes_in_sys(); node++){
            if(mf->node[node].node_num == -1){
                sys_chip_count += get_num_of_chips_in_node(node);
                continue;
            }
            for(chip = 0; chip < get_num_of_chips_in_node(node); chip++){
                if(mf->node[node].chip[chip].chip_num == -1){
                    sys_chip_count++;
                    continue;
                }
                chp[sys_chip_count].is_chip_mem_in_use= TRUE;
                page_wise_details = &g_data.sys_details.chip_mem_pool_data[sys_chip_count].memory_details.pdata[0];
                for(pi=0;pi<MAX_PAGE_SIZES;pi++){
                    mem_in_use = 0;
                    if(!(page_wise_details[pi].supported) || (mf->node[node].chip[chip].mem_details.pdata[pi].page_wise_usage_mem == 0)){
                        continue;
                    }
                    /*handling fabricbus and cache exercisers differently,as mem requirement is diffrent*/
                    if ( (g_data.test_type == FABRICB) || (g_data.test_type == CACHE)){
                        continue;
                    }
                    page_wise_details[pi].page_wise_usage_mem = mf->node[node].chip[chip].mem_details.pdata[pi].page_wise_usage_mem;
                    for(seg=0;seg<page_wise_details[pi].num_of_segments;seg++){
                        /*reset the segment size field with original value,which might have got modified by previous stanza mem filter*/
                        page_wise_details[pi].page_wise_seg_data[seg].shm_size =  page_wise_details[pi].page_wise_seg_data[seg].original_shm_size;
                        /* If rule file has entry of segment size for a given page,if it is less than original alocated seg size then overwirte with new*/
                        if(g_data.stanza_ptr->seg_size[pi] != -1){
                            if(g_data.stanza_ptr->seg_size[pi] < page_wise_details[pi].page_wise_seg_data[seg].shm_size){
                                page_wise_details[pi].page_wise_seg_data[seg].shm_size = g_data.stanza_ptr->seg_size[pi];
                            }
                        }
                        if(page_wise_details[pi].page_wise_usage_mem < page_wise_details[pi].page_wise_seg_data[seg].shm_size){
                            page_wise_details[pi].page_wise_seg_data[seg].shm_size = page_wise_details[pi].page_wise_usage_mem;
                            page_wise_details[pi].page_wise_seg_data[seg].in_use = 0;
                            page_wise_details[pi].in_use_num_of_segments++;
                            break;
                        }
                        mem_in_use += page_wise_details[pi].page_wise_seg_data[seg].shm_size;
                        if(mem_in_use > page_wise_details[pi].page_wise_usage_mem)break;
                        page_wise_details[pi].page_wise_seg_data[seg].in_use = 0;
                        page_wise_details[pi].in_use_num_of_segments++;
                    }
                }
                sys_chip_count++;
            }
        }
        if ( g_data.test_type == FABRICB){
            for(chip =0;chip< g_data.sys_details.num_chip_mem_pools;chip++){
                for(pi=0;pi<MAX_PAGE_SIZES;pi++){
                    if(!(page_wise_details[pi].supported))continue;
                    rc = modify_fabb_shmsize_based_on_cpufilter(chip,pi);
                    if(rc != SUCCESS){
                        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s:modify_fabb_shmsize_based_on_cpufilter() failed "
                           "with rc = %d\n",__LINE__,__FUNCTION__,rc);
                        return rc;
                    }
                }
            }
        }
    }else{
        int dma_mem_percent = g_data.stanza_ptr->mem_percent;
        int dma_cpu_percent = g_data.stanza_ptr->cpu_percent;
        if((g_data.stanza_ptr->operation == OPER_DMA) || (g_data.stanza_ptr->operation == OPER_RIM)){
            if(g_data.stanza_ptr->mem_percent == DEFAULT_MEM_PERCENT){
                dma_mem_percent = 5;
            }
            if(g_data.stanza_ptr->cpu_percent == DEFAULT_CPU_PERCENT){
                dma_cpu_percent = 10;
            }
        }
        sys_page_wise_details = &g_data.sys_details.memory_details.pdata[0];
        for(pi=0;pi<MAX_PAGE_SIZES;pi++){
            mem_in_use = 0;
            sys_page_wise_details[pi].page_wise_usage_mem = 0;
            sys_page_wise_details[pi].in_use_num_of_segments = 0;
            if(!(sys_page_wise_details[pi].supported) || (sys_page_wise_details[pi].free == 0)){
                continue;
            }
            if(pi < PAGE_INDEX_2M){
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].free * (g_data.gstanza.global_alloc_mem_percent/100.0);/*allocated memory*/
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].page_wise_usage_mem * (dma_mem_percent/100.0);
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].page_wise_usage_mem / sys_page_wise_details[pi].psize;
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].page_wise_usage_mem * sys_page_wise_details[pi].psize;
            }else{
                sys_page_wise_details[pi].page_wise_usage_mem = sys_page_wise_details[pi].free;
            }


            for(seg=0;seg<sys_page_wise_details[pi].num_of_segments;seg++){
                /*reset the segment size field with original value,which might have got modified by previous stanza mem filter*/
                sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size =  sys_page_wise_details[pi].page_wise_seg_data[seg].original_shm_size;
                /* If rule file has entry of segment size for a given page,if it is less than original alocated seg size then overwirte with new*/
                if(g_data.stanza_ptr->seg_size[pi] != -1){
                    if(g_data.stanza_ptr->seg_size[pi] < sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size){
                        sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size = g_data.stanza_ptr->seg_size[pi];
                    }
                }
                if(sys_page_wise_details[pi].page_wise_usage_mem < sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size){
                    sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size = sys_page_wise_details[pi].page_wise_usage_mem;
                    sys_page_wise_details[pi].in_use_num_of_segments++;
                    break;
                }
                mem_in_use += sys_page_wise_details[pi].page_wise_seg_data[seg].shm_size;
                if(mem_in_use > sys_page_wise_details[pi].page_wise_usage_mem)break;
                sys_page_wise_details[pi].in_use_num_of_segments++;
            }
        }
        /*check for "num_threads" mentioned in rule, else check for "cpu_percent", other wise set it to g_num_threads(global stanza, deafault value - total lcpus)*/
        if(g_data.stanza_ptr->num_threads == -1){
            if(g_data.stanza_ptr->cpu_percent < 100){
                g_data.stanza_ptr->num_threads = (g_data.sys_details.tot_cpus * (dma_cpu_percent/100.0));
                if (g_data.stanza_ptr->num_threads == 0){
                    g_data.stanza_ptr->num_threads = 1;
                }
            }else{
                g_data.stanza_ptr->num_threads = g_data.gstanza.global_num_threads;
            }
        }
    }
    return SUCCESS;
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
	}else{
        displaym(HTX_HE_INFO,DBG_MUST_PRINT,"[%d]%s:huge page size is neither 2M nor 16M\n",__LINE__,__FUNCTION__);
        return 0;
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

                case TLB:

                    break;

                case CACHE:
                    
                     break;
                        
				default:
					displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]wrong test type arg is passed to fun %s for filling huge page(page id:%d,size=%d) deatils from %s\n",
							__LINE__,__FUNCTION__,huge_page_index,huge_page_size,log_dir);			
					return(FAILURE);
			}		
		fclose(fp);
		displaym(HTX_HE_INFO,DBG_MUST_PRINT,"huge page details from %s avail_16M/2M=%lu,avail_16G=%lu,free_16M/2M=%lu,free_16G=%lu\n",log_dir,avail_16m,avail_16g,free_16m,free_16g);
		}
		
	}
	return (SUCCESS);
}

int allocate_mem_for_threads(){

    g_data.thread_ptr = (struct thread_data*)malloc(sizeof(struct thread_data) * g_data.stanza_ptr->num_threads);
    if(g_data.thread_ptr == NULL){
        displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: malloc failed to allocate %d bytes of memory with error(%d):%s, total threads =%d\n",
            __LINE__,__FUNCTION__,(sizeof(struct thread_data) * g_data.stanza_ptr->num_threads),errno,strerror(errno),g_data.stanza_ptr->num_threads);
        return(FAILURE);

    }
    for(int i=0;i<g_data.stanza_ptr->num_threads;i++){
        g_data.thread_ptr[i].thread_num = -1;
        g_data.thread_ptr[i].bind_proc = -1;

    }
    if((g_data.test_type == MEM) || (g_data.test_type == FABRICB) || (g_data.test_type == TLB)){
        mem_g.mem_th_data = (struct mem_exer_thread_info*)malloc(sizeof(struct mem_exer_thread_info) * g_data.stanza_ptr->num_threads);
        if(mem_g.mem_th_data == NULL){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: malloc failed to allocate %d bytes of memory with error(%d):%s, total threads =%d\n",
                __LINE__,__FUNCTION__,(sizeof(struct mem_exer_thread_info) * g_data.stanza_ptr->num_threads),errno,strerror(errno),g_data.stanza_ptr->num_threads);
            return(FAILURE);
        }
    }else if(g_data.test_type == CACHE){
        cache_g.cache_th_data = (struct cache_exer_thread_info*)malloc(sizeof(struct cache_exer_thread_info) * g_data.stanza_ptr->num_threads);
        if(cache_g.cache_th_data == NULL){
            displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"[%d]%s: malloc failed to allocate %d bytes of memory with error(%d):%s, total threads =%d\n",
                __LINE__,__FUNCTION__,(sizeof(struct cache_exer_thread_info) * g_data.stanza_ptr->num_threads),errno,strerror(errno),g_data.stanza_ptr->num_threads);
                        return(FAILURE);
        }

    }
    return SUCCESS;
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
        return(FAILURE);
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

/************************************************************************
*       SIGUSR2 signal handler for cpu hotplug add/remove               *
*************************************************************************/

#ifdef __HTX_LINUX__
void SIGUSR2_hdl(int sig, int code, struct sigcontext *scp)
{

    g_data.stanza_ptr->disable_cpu_bind = 1;
    cpu_add_remove_test = 1;
    hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,"Recieved SIGUSR2 signal\n");
#if 0
    int i, rc = 0;

    if (mem_g.affinity == LOCAL ) {
        /*for (i=0; i < mem_info.num_of_threads; i++) {
            if (mem_info.tdata_hp[i].tid != -1) {
                rc = check_cpu_status_sysfs(mem_info.tdata_hp[i].physical_cpu);
                if (rc = 0) {
                    displaym(HTX_HE_INFO,DBG_MUST_PRINT," cpu %d has been removed, thread:%d exiting in affinity yes case \n",mem_info.tdata_hp[i].physical_cpu,mem_info.tdata_hp[i].thread_num);
                    pthread_cancel(mem_info.tdata_hp[i].tid);
                }
            }
        }*/
        g_data.hotplug_flag= 1;
    }
    g_data.hotplug_flag = 1;
#endif
}
#endif

#ifdef    _DR_HTX_
void SIGRECONFIG_handler(int sig, int code, struct sigcontext *scp)
{
    int rc;
    char hndlmsg[512];
    dr_info_t dr_info;          /* Info about DR operation, if any */
    int i, bound_cpu;

    hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,"DR: SIGRECONFIG signal received \n");

    do {
        rc = dr_reconfig(DR_QUERY,&dr_info);
    } while ( rc < 0 && errno == EINTR);
    if ( rc == -1) {
        if ( errno != ENXIO){
            hxfmsg(&g_data.htx_d,errno,HTX_HE_HARD_ERROR, "dr_reconfig(DR_QUERY) call failed. \n");
        }
        return;
    }

    if (dr_info.mem == 1){
        sprintf(hndlmsg,"DR: DLPAR details"
            "Phase - Check:  %d, Pre: %d, Post: %d, Post Error: %d\n"\
            "Type - Mem add: %d remove: %d, ent_cap = %d, hibernate = %d \n",\
             dr_info.check, dr_info.pre, dr_info.post, dr_info.posterror, dr_info.add, dr_info.rem, dr_info.ent_cap, dr_info.hibernate);
        hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
    }

    if (dr_info.cpu == 1 && (dr_info.rem || dr_info.add) ) {
        sprintf(hndlmsg,"DR: DLPAR details"
            "Phase - Check:  %d, Pre: %d, Post: %d, Post Error: %d\n"\
            "Type - Cpu add: %d remove: %d, bcpu = %d \n",\
             dr_info.check, dr_info.pre, dr_info.post, dr_info.posterror, dr_info.add, dr_info.rem, dr_info.bcpu);
        hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
    }
    if ( g_data.stanza_ptr == NULL ) {
        g_data.stanza_ptr = &g_data.stanza[0];
    }
    /*
     * Check-phase for CPU DR
     * Handle only CPU removals, new CPUs will be used from the next stanza.
     */
    if (dr_info.check && dr_info.cpu && dr_info.rem) {

        g_data.cpu_DR_flag = 1;
        cpu_add_remove_test = 1;
        /* Disable next binds and look for current bound cpu to thread,unbind it*/
        g_data.stanza_ptr->disable_cpu_bind = 1;
        if(g_data.thread_ptr != NULL){
            for(i=0; i<g_data.stanza_ptr->num_threads; i++) {
                /* Also check if the running thread has already completed. */
                if ((g_data.thread_ptr[i].bind_proc == dr_info.bcpu) &&
                    (g_data.thread_ptr[i].tid != -1)) {
                    /* Unbind thread from the CPU under DR */
                    if (bindprocessor(BINDTHREAD,g_data.thread_ptr[i].kernel_tid,
                        PROCESSOR_CLASS_ANY)) {
                        if (errno == ESRCH) {
                            continue;
                        }
                        sprintf(hndlmsg,"Unbind failed. errno %d(%s), for thread num %d,cpu=%d"
                                ", TID %d.\n", errno,strerror(errno),i,g_data.thread_ptr[i].bind_proc,g_data.thread_ptr[i].tid);
                        hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
                    }
                    /*
                     * More than one thread could be bound to the same CPU.
                     * as bind_proc = bind_proc%get_num_of_proc()
                     * Run through all the threads, don't break the loop.
                     */
                }
            }
        }
        sprintf(hndlmsg,"DR: In check phase and cpu:%d is removed, we unbind threads which are affectd\n",dr_info.bcpu);
        hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);

        if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)){
            sprintf(hndlmsg,"dr_reconfig(DR_RECONFIG_DONE) failed."
                    " error %d \n", errno);
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
        }
        else{
            sprintf(hndlmsg,"DR:DR_RECONFIG_DONE Success!!,in check phase for cpu=%d \n",dr_info.bcpu);
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);

        }
        return;
    }
    /* Post/error phase; reset tracker count  */
    if ((dr_info.post || dr_info.posterror) && dr_info.cpu && dr_info.rem) {
        g_data.cpu_DR_flag = 0;  /*falg is set to recollect cpu details for tlbie test case*/
    }

    /* For any other signal check/Pre/Post-phase, respond with DR_RECONFIG_DONE */
    if (dr_info.check || dr_info.pre || dr_info.post ) {
        if (dr_info.mem && dr_info.check) {
            sprintf(hndlmsg,"DR:Mem DR operation performed,setting  g_data.mem_DR_flag = 1");
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
            g_data.mem_DR_flag = 1;
        }
        if (dr_reconfig(DR_RECONFIG_DONE,&dr_info)){
            sprintf(hndlmsg,"dr_reconfig(DR_RECONFIG_DONE) failed."
                    " error %d ,check=%d,pre=%d,post=%d\n",dr_info.check,dr_info.pre,dr_info.post,errno);
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
        }
        else{
            sprintf(hndlmsg,"DR:DR_RECONFIG_DONE Successfully after check=%d, pre=%d, post=%d   phhase !! \n",dr_info.check,dr_info.pre,dr_info.post);
            hxfmsg(&g_data.htx_d,0,HTX_HE_INFO,hndlmsg);
        }
    }
    return;
}
#endif

int fun_NULL(){
	displaym(HTX_HE_HARD_ERROR,DBG_MUST_PRINT,"hxemem64:Function pointer trying to dereference NULL, please check the decalrations");
	exit(1);
}

