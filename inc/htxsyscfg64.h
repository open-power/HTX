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

/* @(#)86       1.10.1.10  src/htx/usr/lpp/htx/inc/htxsyscfg64_new.h, htx_libhtxsyscfg64, htxfedora 5/19/15 06:49:43  */

/* Name : htxsyscfg64.h                                  *
 * Description: Contains declarations for variable and *
 *              functions in htxsyscfg library         *
 */
 
#ifndef __HTXSYSCFG_H64__
#define __HTXSYSCFG_H64__

#if defined(_DR_HTX_) && !defined(__HTX_LINUX__)
#    include <sys/types.h>
#    include <sys/dr.h>
#    include <sys/procfs.h>
#endif

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
#define P8_CORE_MASK 0x00000078

/*
1 1100 0000 0000  8 possible nodes   >> 10
0 0011 1000 0000  8 possible chips   >> 7
0 0000 0111 1000  12 possible cores  >> 3
0 0000 0000 0111  8 possible threads
*/ 
#define P7_NODE_MASK 0x00000380
#define P7_CHIP_MASK 0x00000060
#define P7_CORE_MASK 0x0000001C
#define P6_NODE_MASK 0x000000E0
#define P6_CHIP_MASK 0x00000018
#define P6_CORE_MASK 0x00000002
/* P9 macros for CUMULUS */
#define P9_CUMULUS_GET_NODE(_PIR_)   ((_PIR_ & P9_CUMULUS_NODE_MASK) >> 11)
#define P9_CUMULUS_GET_CHIP(_PIR_)   ((_PIR_ & P9_CUMULUS_CHIP_MASK) >> 8)
#define P9_CUMULUS_GET_CORE(_PIR_)   ((_PIR_ & P9_CUMULUS_CORE_MASK) >> 3)

/* P9 macros for NIMBUS */
#define P9_NIMBUS_GET_NODE(_PIR_)   ((_PIR_ & P9_NIMBUS_NODE_MASK) >> 11)
#define P9_NIMBUS_GET_CHIP(_PIR_)   ((_PIR_ & P9_NIMBUS_CHIP_MASK) >> 8)
#define P9_NIMBUS_GET_CORE(_PIR_)   ((_PIR_ & P9_NIMBUS_CORE_MASK) >> 2)

#define P8_GET_NODE(_PIR_)   ((_PIR_ & P8_NODE_MASK) >> 10)
#define P8_GET_CHIP(_PIR_)   ((_PIR_ & P8_CHIP_MASK) >> 7)
#define P8_GET_CORE(_PIR_)   ((_PIR_ & P8_CORE_MASK) >> 3)
#define P7_GET_NODE(_PIR_)   ((_PIR_ & P7_NODE_MASK) >> 7)
#define P7_GET_CHIP(_PIR_)   ((_PIR_ & P7_CHIP_MASK) >> 5)
#define P7_GET_CORE(_PIR_)   ((_PIR_ & P7_CORE_MASK) >> 2)
#define P6_GET_NODE(_PIR_)   ((_PIR_ & P6_NODE_MASK) >> 5)
#define P6_GET_CHIP(_PIR_)   ((_PIR_ & P6_CHIP_MASK) >> 3)
#define P6_GET_CORE(_PIR_)   ((_PIR_ & P6_CORE_MASK) >> 1)

/*type of virtualization*/
#define PVM_GUEST		-1
#define KVM_GUEST		0 
#define NV			1
#define BML				2
#define PVM_PROC_SHARED_GUEST	3

/* page size indexez */
#define PAGE_INDEX_4K    0
#define PAGE_INDEX_64K   1
#define PAGE_INDEX_2M    2
#define PAGE_INDEX_16M   3
#define PAGE_INDEX_16G   4

#define MAX_PAGE_SIZES 5
#define MAX_POOLS 64

#define MAX_FILTER_STR    128

/****************** Common include files  ******************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <signal.h>
#include <errno.h>
#include <strings.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "hxihtxmp.h"
#include "hxihtx64.h"

#ifndef __HTX_LINUX__                                    /* Only for AIX */
    #include <libperfstat.h>
    #include <memory.h>
    #include <sys/proc.h>
    #include <sys/rset.h>
    #include <sys/lock_def.h>
    #include <sys/thread.h>
    #include <sys/vminfo.h>
    #include <sys/processor.h>
    #include <sys/systemcfg.h>
    #include <sys/vminfo.h>
    #pragma mc_func trap { "7c810808" }
    #include <sys/types.h>

#endif

#ifdef __HTX_LINUX__                                    /* Only for Linux */
    #include <linux/version.h>
    #include <asm/ioctl.h>
    #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        #define IOCTL_NUM_IOWR
    #else
        #define IOCTL_NUM_IOWR_BAD
    #endif
    #include <sys/stat.h>
    #include <sys/mount.h>
    #include <sys/ioctl.h>
    #include <linux/unistd.h>
    #include <fcntl.h>
    #include <stdint.h>
    #include <stdarg.h>
    #include <unistd.h>
    #include <sched.h>
	#include <sys/misclib.h>

    #include <linux/version.h>
    #include <asm/ioctl.h>
    #if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        #define IOCTL_NUM_IOWR
    #else
        #define IOCTL_NUM_IOWR_BAD
    #endif

    /*
     * Since glibc has no wrapper for gettid
     * we need to use the syscall() function call it
     * indirectly. We need an argument of pid_t
     * to be given to sched_setaffinity when binding
     * a thread to a processor, pthread_self() returns
     * a thread id of type pthread_t AND getpid() returns
     * the pid of the parent thread.
     * The following include is req for syscall()
     */

    #include <sys/syscall.h>
    #define BINDPROCESS  1                      /* Bind all threads in process Who        */
    #define BINDTHREAD   2                      /* Only bind thread Who                   */
    #define PROCESSOR_CLASS_ANY ((cpu_t)(-1))   /* Unbound                                */
    typedef short cpu_t;                        /* logical processor ID                   */


#endif

#if defined(_DR_HTX_) && !defined(__HTX_LINUX__)
    #include <sys/types.h>
    #include <sys/dr.h>
    #include <sys/procfs.h>
#endif

#ifdef DEBUG
    #define DEBUGON printf
    #define FPRINTF(a,x,...)
#else
    #define DEBUGON
    #define DEBUGOFF(x, ...)
    #define FPRINTF(a,x,...)
#endif

/********************     Defined constants     ********************/

#define SYSCFG_SHM_KEY          0x99999
#define  B     1
#define  K     1024*B
#define  M     1024*K

#define MAX_STANZA_NAME         100
#define MAX_STANZA_PATTERNS     9
#define MIN_PATTERN_SIZE        8
#define MAX_PATTERN_SIZE        4096
#define PG_SZ_16M               (16*M)
#define ADDR_PAT_SIGNATURE      0x414444525F504154  /* Hex equiv of "ADDR_PAT" */
#define RAND_PAT_SIGNATURE      0x52414E445F504154  /* Hex equiv of "RAND_PAT" */
#define SMT_ENABLED             0x2

#define BIT_PATTERN_WIDTH      8   /* 8 BYTES wide */
#define NO_CPU_DEFINED         -1
#define HOST_CPU               0
#define DEST_CPU               1

#define PVR_POWER6                  0x3e
#define P6_L3CACHE_SIZE             32*M
#define P6_MAX_CORES_PER_CHIP       2
#define P6_MAX_SMT_STATUS           2

#define PVR_POWER7              0x3f
#define P7_L3CACHE_SIZE         4*M
#define P7_MAX_CORES_PER_CHIP   8
#define P7_MAX_SMT_STATUS       4

#define PVR_POWER7PLUS              0x4a
#define P7PLUS_L3CACHE_SIZE         10*M
#define P7PLUS_MAX_CORES_PER_CHIP   8
#define P7PLUS_MAX_SMT_STATUS       4

#define POWER8_L3CACHE_SIZE			8*M
#define POWER8_MAX_CORES_PER_CHIP	12
#define POWER8_MAX_SMT_STATUS		8

#define L3CACHE_LINESIZE           128
#define MAX_NODE                   8
#define MAX_NODES                  8
#define MAX_CHIPS_PER_NODE         8
#define MAX_CORES_PER_CHIP         24
#define MAX_CPUS_PER_CORE          8
#define MAX_CPUS_PER_CHIP          (MAX_CORES_PER_CHIP * MAX_CPUS_PER_CORE)
#define MAX_THREADS                (MAX_CPUS_PER_CHIP * MAX_CHIPS_PER_NODE * MAX_NODE)

#define MAX_CPUS                  (MAX_NODE * MAX_CHIPS_PER_NODE * MAX_CPUS_PER_CHIP )
#define MAX_CPUS_PER_NODE         (MAX_CHIPS_PER_NODE * MAX_CPUS_PER_CHIP)
#define MAX_CORES_PER_NODE        (MAX_CORES_PER_CHIP * MAX_CHIPS_PER_NODE)

#define MAX_CHIPS                 (MAX_NODE * MAX_CHIPS_PER_NODE)
#define MAX_SMT_THREAD_PER_CHIP   MAX_CPUS_PER_CHIP
#define MAX_CORES                 (MAX_CORES_PER_CHIP*MAX_CHIPS_PER_NODE*MAX_NODE)
#define MAX_THREADS_PER_NODE      MAX_CPUS_PER_NODE
#define MAX_THREADS_PER_CHIP      MAX_CPUS_PER_CHIP
#define MAX_THREADS_PER_CORE      MAX_CPUS_PER_CORE
#define MAX_PROCS_PER_POOL		  512

#define PART_FULL              1
#define PART_DEDICATED         2
#define PART_SHARED            3

#define SMT_CAPABLE         0x1
#define SMT_ENABLED         0x2

#define PV_POWER4               0x35  /* GP */
#define PV_POWER4_P             0x38  /* GQ */
#define PV_PPC970               0x39  /* GPUL */
#define PV_POWER5               0x3A  /* GR */
#define PV_POWER5_P             0x3B  /* GS */
#define PV_PPC970_P             0x3C  /* GPUL2 */
#define PV_POWER5_PP            0x3D  /* GT */
#define PV_POWER6               0x3E  /* ECLIPZ */
#define PV_POWER7               0x3F  /* P7 */
#define PV_POWER7PLUS           0x4A  /* P7+ */
#define PV_POWER8_MURANO		0x4B  /* P8 */	
#define PV_POWER8_VENICE		0x4D  /* P8 */
#define PV_POWER8_PRIME			0x4C
#define PV_POWER9_NIMBUS		0x4E  /* P9 */
#define PV_POWER9_CUMULUS		0x4F  /* P9 */
#define PV_CellBE               0x70  /* STI */

/* Possible values of the structure_status member of SYS_CONF structure */

#define UPDATE_IN_PROGRESS   0
#define AVAILABLE            1
#define UPDATE_DONE          2

/* Map types */

#define M_NODE     0
#define M_CHIP     1
#define M_CORE     2


/********************     Structures     ********************/


typedef struct {
    unsigned int num_procs;
    unsigned int lprocs[MAX_CPUS_PER_CORE];
	unsigned int pprocs[MAX_CPUS_PER_CORE];	
} CORE;

typedef struct {
    unsigned int num_cores;
    CORE core[MAX_CORES_PER_CHIP];
    CORE lcore[MAX_CORES_PER_CHIP];
    CORE pcore[MAX_CORES_PER_CHIP];
	int num_cores_excluded;
} CHIP;

typedef struct {
    unsigned int num_chips;
    CHIP chip[MAX_CHIPS_PER_NODE];
} NODE ;

typedef struct {
    unsigned int num_nodes;
    NODE     node[MAX_NODE];
    int      structure_status;
	int Logical_cpus[MAX_THREADS];
	int duplicate[MAX_THREADS][11];
    int Physical_cores[MAX_CORES];
	int Physical_cpus[MAX_THREADS];
	int numberArray[MAX_CORES_PER_CHIP][3];
	pthread_rwlock_t rw ;
} SYS_CONF;

typedef struct {
    unsigned int nodes;
    unsigned int chips;
    unsigned int cores;
    unsigned int cpus;
pthread_rwlock_t rw;
} SYS_STAT;

typedef struct {
    signed   int start;
    signed   int stop;
    unsigned int num_cpus;
} CPU_MAP;

typedef struct {
    SYS_STAT stat;
    CPU_MAP cpu_map, cpus_index_in_node[MAX_NODE],cpus_index_in_chip[MAX_CHIPS],cpus_index_in_core[MAX_CORES];
}SYSTEM_CPU_MAP;

/*
*
LPAR structures
*
*/

typedef struct
{
        int partition_type;          /* System is in Full system partition mode or not */
        char partition_name[50];      /* Name of current partition */
}htxsyscfg_partition_t;

typedef struct
{
        char endianess[10];
        char virt_typ[50];
        char proc_shared_mode[4];
        int virt_flag;
}htxsyscfg_env_details_t;


typedef struct
{
         int l_cpus;                   /* Number of logical cpus */
         int p_cpus;                   /* Number of physical cpus */
         int v_cpus;                   /* Number of virtual cpus */

        float phy_to_virt;            /* Physical to virtual cpus ratio */
        float phy_to_logical;         /* Physical to logical cpus ratio */
}htxsyscfg_cpus_t;

typedef struct
{
    char host_name[100];          /* System host name */
    char mtm[50];                 /* Machine type model */
    char os_version[25];          /* Latest installed maintenance and technology level of the system */

    htxsyscfg_partition_t partition;

    char firmware_version[100];   /* Firmware level of the system */
    char com_ip[100];             /* IP address of current host */
    char time_stamp[50];          /* Time and date */

    htxsyscfg_cpus_t cpus;
    htxsyscfg_env_details_t env_details;
	pthread_rwlock_t rw;
} htxsyscfg_lpar_t;


/*
*
core structures
*
*/

typedef struct
{
      int smt_capable;           /* SMT capable or not */
      int smt_enabled;           /* SMT enabled or not */
      int smt_threads;           /* SMT threads per cpu */
	  int min_smt_threads;
	  int max_smt_threads;
}htxsyscfg_smt_t;

typedef struct
{
        htxsyscfg_smt_t smtdetails;
        long long cpu_clock_freq;  /* CPU clock frequency in Hz */
        long long tb_clock_freq;    /* Timebase clock frequency */
		pthread_rwlock_t rw;
} htxsyscfg_core_t;

/*
*
Memeory structures
*
*/

typedef struct
{
         long long real_mem;             /* Size of real memory in bytes */
         long long virt_mem;             /* Size of virtual memory in bytes */
         long long free_real_mem;        /* Size of free real memory in bytes */
}htxsyscfg_memsize_t;

typedef struct
{
         int num_sizes;                  /*Number of page sizes supported */
		 int supported;
		 int page_size;          		/*supported page size*/
		 unsigned long page_sizes[50];
		 unsigned long free_pages;		 /*number of free pges*/
		 unsigned long total_pages;
         float swap_space;               /* Current swap space size */
}htxsyscfg_pages_t;

typedef struct
{
         int has_cpu_or_mem;               /*flag,to check node has either cpus or memory */
		 int node_num;
         int num_procs;             /* number of processors in the resource set */
	  	 long long mem_total;
		 unsigned long long mem_free;
		 unsigned long long mem_avail;
         int procs_per_pool[MAX_PROCS_PER_POOL];
         htxsyscfg_pages_t page_info_per_mempool[MAX_PAGE_SIZES];
}htxsyscfg_mempools_t;

typedef struct
{
        htxsyscfg_memsize_t mem_size;
        htxsyscfg_pages_t page_details[MAX_PAGE_SIZES];
		htxsyscfg_mempools_t mem_pools[MAX_POOLS];
		int num_numa_nodes;
		pthread_rwlock_t rw;
}htxsyscfg_memory_t;

/*
*
cache structures
*
*/

typedef struct
{
         int L1_isize;            /* Size of L1 instruction cache in bytes */
         int L1_dsize;            /* Size of L1 data cache in bytes */
         int L1_iasc;             /* L1 instruction cache associativity */
         int L1_dasc;             /* L1 data cache associativity */
         int L1_iline;            /* L1 instruction cache line size */
         int L1_dline;            /* L1 data cache line size */
         #ifdef __HTX_LINUX__
         float L2_size;             /* L2 cache size in MB */
         #else
         float L2_size;           /* L2 cache size in in bytes */
         #endif
         int L2_asc;              /* L2 cache associativity */
         int L2_line;             /* L2 cache line size */

         int L3_size;             /* L3 cache size in bytes */
         int L3_asc;              /* L3 cache associativity */
         int L3_line;             /* L3 cache line size */
		 pthread_rwlock_t rw;
} htxsyscfg_cache_t;

typedef struct {
	unsigned int global_pvr; 
	SYS_CONF syscfg;
	SYSTEM_CPU_MAP system_cpu_map;
    htxsyscfg_lpar_t global_lpar;
    htxsyscfg_core_t global_core;
    htxsyscfg_memory_t global_memory;
    htxsyscfg_cache_t global_cache;
	int get_node_value;
	int get_chip_value;
	int get_core_value;
}GLOBAL_SYSCFG;
	
extern GLOBAL_SYSCFG *global_ptr;

/* Below structures are used for filtering and storing info about enabled
   system resources for a given resource string  of format N*P*C*T* */

struct core_filter_info{
    unsigned int core_num;
    unsigned int num_procs;
    unsigned int lprocs[MAX_CPUS_PER_CORE];
};

struct chip_filter_info{
    unsigned int chip_num;
    unsigned int num_cores;
    unsigned int lprocs[MAX_CPUS_PER_CHIP];
    struct core_filter_info core[MAX_CORES_PER_CHIP];
    /* struct mem_info mem_details; */
};

struct node_filter_info{
    unsigned int node_num;
    unsigned int num_chips;
    unsigned int lprocs[MAX_CPUS_PER_NODE];
    struct chip_filter_info chip[MAX_CHIPS_PER_NODE];
};

struct resource_filter_info {
    struct node_filter_info node[MAX_NODES];
    int num_nodes;
};

/********************     Function Declarations     ********************/


pthread_rwlockattr_t              rwlattr;

int  pthread_rwlock_rdlock(pthread_rwlock_t *rwlock );

int  pthread_rwlock_wrlock(pthread_rwlock_t *rwlock );

int pthread_rwlock_unlock(pthread_rwlock_t *rwlock); 

/*for initialising shared memory*/
int init_syscfg(void);

/*for command line run and show_sysfcg*/
int init_syscfg_with_malloc(void);

/*for passing the excluded cores array */
int pass_core_exclusion(int *number);

/* to attach to the existing shared memory*/
int attach_to_syscfg(void);

/*to delete the shm created*/
int detach_syscfg(void);

/*to free the malloc'd memory*/
int detach_syscfg_with_malloc(void);

/*to update the shm with latest values*/
int update_syscfg(void);

/*Returns output of a shell command (cmd) in a string*/
int get_cmd_op(char*,const char[]);

/*Retrieves hostname*/
void get_hostname(char*);

/*Retrieves MTM details */
int get_mtm(void);

/*Retrieves OS version*/
int get_osversion(void);

/*Retrieves system type Full partition mode, Shared or Dedicated*/
int get_partition_type(void);

/*Partition name*/
int get_partition_name(void);

/*Retrieves details of physical, virtual and logical processors*/
int phy_logical_virt_cpus(htxsyscfg_cpus_t*);

/*Retrieves lpar level details*/
extern int get_lpar_details(htxsyscfg_lpar_t*);

/*Retrieves smt details*/
int get_smt_details(htxsyscfg_smt_t*);

/*Retrieves processor frequency*/
long long get_proc_freq(void);

/*Retrieves core details*/
int get_core_details(htxsyscfg_core_t*);

/*Retrieves memory size*/
int get_memory_size(htxsyscfg_memsize_t*);

/*Retrieves page size details*/
int get_page_size(htxsyscfg_pages_t*);

/*Retrieves memory pools details*/
int get_memory_pools(void);

/*Retrieves memory details*/
int get_memory_details(htxsyscfg_memory_t*);

/*Retrieves L1 cache details*/
int L1cache(htxsyscfg_cache_t*);

/*Retrieves L2 cache details*/
int L2cache(htxsyscfg_cache_t*);

/*Reads the pvr register */
void read_pvr(int*);

unsigned int get_pvr(void);

/*Retrieves L3 cache details*/
int L3cache(htxsyscfg_cache_t*);

/* AIX function to get value of PVR */
unsigned long getPvr(void);

#ifdef __HTX_LINUX__
/* Using bindprocessor function in misc64 library for Linux */
int bindprocessor (int What, int Who, int Where);

/*gets the pir value*/
int get_cpu_id(int);

/*gets the compatible mode pvr value*/
int get_cpu_version(void);

/*gets the true pvr value*/
int get_true_cpu_version(void);

/*gets thei last two bytes of compatible mode pvr value*/
int get_cpu_revision(void);

/*gets thei last two bytes of true pvr value*/
int get_true_cpu_revision(void);

int htx_bind(int htx_logical_cpu_num);

/*Checks if /sys is mounted or not. If not then mounts it*/
int sys_mount_check(void);

/*Removes new line at end, if present.*/
void remove_newline_at_end(char*);

/*Retrieves location codes of all devices */
void get_location_codes(void);

/*Retrieves timebase details*/
long long get_timebase(void);

/*Retrieves L2 & L3 details*/
int L2L3cache(htxsyscfg_cache_t *t);
#endif

/*Retrieves hardware details*/

int get_hardware_config(SYS_CONF *sys_conf,unsigned int tot_cpus, unsigned int pvr,int type);

int get_p6_compat_mode (unsigned int Pvr);
int get_p7_compat_mode(unsigned int Pvr);
int get_virtualization_type(void);
int get_endianess (void);


/* Threadsafe wrapper to get_hardware_config */
int get_hardware_config_threadsafe(SYS_CONF *sys_conf,unsigned int tot_cpus, unsigned int pvr,int need);

/* Fetches CPUs in a node */
int get_cpus_in_node(int node_no,signed int *cpus_in_node);

/* Fetches CPUs in a chip */
int get_cpus_in_chip(int chip_no,signed int *cpus_in_chip);

/* Fetches CPUs in a core */
int get_cpus_in_core(int core_no,signed int *cpus_in_core);

int get_num_of_nodes_in_sys(void);

int get_num_of_chips_in_node(int node_num);

int get_num_of_cores_in_chip(int node_num, int chip_num);

int get_num_of_cpus_in_core(int node_num, int chip_num, int core_num);

int get_logical_cpu_num(int node_num, int chip_num, int core_num, int thread_num);

int get_chips_in_node(int node_num, int *chip_number_list);

int get_cores_in_chip(int node_num, int chip_num, int *core_number_list);

/* Function to parse the resource string to provide resources enabled in the given string */
int parse_filter(char filter_str[][MAX_FILTER_STR], struct resource_filter_info *filter, int num_cpu_filters);

/*Retrieves No of cpus, cores, chips and nodes in hardware*/
int get_hardware_stat(SYS_STAT *sys_stat);

/* Repopulates the SYS_CONF structure */
int repopulate_syscfg(struct htx_data *p_htx_data);

/* Retrieves the SMT status of a core */
int get_smt_status(int core_no);

int get_core_info(int *core_number_list, int instance);

/* Fetches cores in node and returns number of cores in that node*/
int get_cores_in_node(int node_num, int *core_number_list);

int get_cpu_map(int type, CPU_MAP *cpu_map, int instance_number);

/* Returns number of CPUs using sysfs on BML-AWAN */
/* procfs is not available on BML-AWAN runs */
int get_nprocs_awan(void);

/* Returns number of SMT threads using sysfs on BML-AWAN */
int get_smt_awan(void);

#endif /* __HTXSYSCFG_H64__ */




