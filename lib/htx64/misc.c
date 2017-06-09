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

/* @(#)68     1.17.3.25  src/htx/usr/lpp/htx/lib/misc64/misc.c, libmisc, htxfedora 7/30/15 06:09:11 */

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/misclib.h>
#include <sys/ioctl.h>
#include "hxihtx64.h"

/* Function declarations. */

int check_cpu_exist(int);
int get_logical_2_physical(int);
int get_cpu_version(void);
int htx_unbind_thread(void);
int htx_unbind_process(void);
int bind_process(pid_t, int, int);
int bind_thread(pthread_t, int, int);
int htx_bind_thread(int, int);
int htx_bind_process(int, int);
int get_true_cpu_version(void);

int get_real_address(void *, void *) ;
int get_online_cpu_mask(cpu_set_t *,size_t);
int do_trap_htx64(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long, unsigned long,unsigned long,unsigned long);
cpu_set_t* allocate_cpu_mask(size_t *);
int  print_sysfs_cpu(int,size_t,cpu_set_t*);

struct htx_data             *misc_htx_data=NULL;
char                        msg[1500];


/* Function definitions. */
int retry_open_calls(FILE** f_ptr,const char* str,const char* mode){
    int retries = 0;
    while(retries < 5){
		sleep(1);
        *f_ptr = fopen(str, mode);
        if(*f_ptr != NULL){
            return 0;
        }
        else {
            retries++;
        }
    }
		sprintf(msg,"fopen failed for %s with retry count=%d,errno=%d\n",str,retries,errno);
   		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
    return -errno;
}

void set_misc_htx_data(struct htx_data *p_htx_data)
{
    misc_htx_data = p_htx_data;
}



/* This function returns cpu version considering compat mode. */
int get_cpu_version(void)
{
	FILE *fp;
	char str[80];
	int pvr, TempPvr;

	__asm __volatile ("mfspr %0, 287" : "=r" (pvr));
	TempPvr = pvr >> 16;

	sprintf(str,"grep : /proc/cpuinfo 2>/dev/null | grep cpu | head -n 1 | awk '{print $3}'");

	fp = popen(str, "r");
	if(fp == NULL) {
		return pvr;
	}

	if(fgets(str, 80, fp) != NULL) {
		pclose(fp);
	} else {
		printf("Could not get the os pvr value \n ");
		pclose(fp);
	}


	if ((TempPvr == 0x3e) && (!strncmp(str,"POWER5",6))) {
		pvr = 0x003b0300; // P6 running in P5 mode
	}

	if ((TempPvr == 0x3f || TempPvr == 0x4a) && (!strncmp(str,"POWER6",6))) {
		pvr = 0x003e0000; // P7/P7+ running in P6 mode
	}

	if ((TempPvr == 0x4b || TempPvr == 0x4d) && (!strncmp(str,"POWER7",6))) {
 		pvr = 0x003f0000; // P8 running in P7 mode
	}

	if ((TempPvr == 0x4e || TempPvr == 0x4f) && (!strncmp(str,"POWER8",6))) {
 		pvr = 0x004b0000; // P9 running in P8 mode
	}
	return pvr;
}

/* This function returns true cpu version irrespective of compat mode. */
int get_true_cpu_version(void)
{
	int pvr;

	__asm __volatile ("mfspr %0, 287" : "=r" (pvr));
	return pvr;
}

/* This function is used to trap lpar to kernel debugger. */
int do_trap_htx64(unsigned long arg1, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5, unsigned long arg6,unsigned long arg7,unsigned long arg8)
{
    	int fd,rc;

    	struct trap_struct64{
        	unsigned long r3val;
        	unsigned long r4val;
        	unsigned long r5val;
        	unsigned long r6val;
        	unsigned long r7val;
        	unsigned long r8val;
			unsigned long r9val;
			unsigned long r10val;
    	};
    	struct trap_struct64 trap_struct;

    	trap_struct.r3val = arg1;
    	trap_struct.r4val = arg2;
    	trap_struct.r5val = arg3;
    	trap_struct.r6val = arg4;
    	trap_struct.r7val = arg5;
    	trap_struct.r8val = arg6;
		trap_struct.r9val = arg7;
		trap_struct.r10val = arg8;

    	fd = open("/dev/miscchar", O_RDWR);
    	if (fd < 0)
    	{
        	printf("Error opening file in user fn do_trap_htx64\n");
        	return -1;
    	}

    	rc = ioctl(fd, TRAP_HTX64, &trap_struct);
    	close(fd);

    	return rc;
}

/* This function returns real address for passed effective address. */
int get_real_address(void *ea, void *ra)
{
	int fd, rc;

	struct getAddress{
        	unsigned long long EffAddr;
        	unsigned long long RealAddr;
    	}ga;

    	/* Zero out the structure */
    	memset(&ga, 0, sizeof(ga)) ;
    	ga.EffAddr = (unsigned long)ea;

    	fd = open("/dev/miscchar", O_RDWR);
    	if (fd < 0) {
        	printf("get_real_address: %s\n", strerror(errno)) ;
        	printf("get_real_address: Open of device file /dev/miscchar failed with errno = %d\n", errno);
        	return -1;
    	}

    	rc = ioctl(fd, EFF2PHYSICAL, &ga);

    	if(rc) {
        	printf("For EffAddr=0x%llx, RealAddr=0x%llx\n", ga.EffAddr, ga.RealAddr) ;
        	*(unsigned long long *)ra = ga.RealAddr;
		close(fd);
        	return 0;
    	} else {
        	printf("get_real_address: ioctl to get Real Address failed\n") ;
		close(fd);
        	return -1;
    	}
}

/* Below functions binds thread/process on physical cpu passed.
   If physical cpu passed is -1 then it calculates corresponding
   physical cpu for logical cpu passed. Calling thread/process is binded
   on calculated physical cpu and physical cpu number is returned
   as function return value.
   Return value:
   >=0: Physical Cpu for Logical Cpu passed.
   -1: Logical Cpu passed not available.
   -2: Physical Cpu passed Hot Plug removed.
   Oher -ve values: System call failure.
*/

/* Function to bind thread. */
int htx_bind_thread(int lcpu, int pcpu)
{
	int rc;

	rc = bind_thread(pthread_self(), lcpu, pcpu);
	return rc;
}

int bind_thread(pthread_t tid, int lcpu, int pcpu)
{
	int rc,err_num,count;
	cpu_set_t *cpu_mask;
	size_t size;

	if(pcpu == -1) {
		/* Find physical cpu if pcpu value passed is -1. */
		pcpu = get_logical_2_physical(lcpu);
		if(pcpu < 0){
			 return pcpu;
		}
	} else {
		/* Check if passed physical cpu is existing. */
		rc = check_cpu_exist(pcpu);
		if(rc == 0)
			return -2;
		else if(rc < 0 )
			return rc;
	}
	cpu_mask = (cpu_set_t *)allocate_cpu_mask(&size);
	/* Proceed with binding current thread to physical cpu. */
	CPU_ZERO_S(size,cpu_mask);
	CPU_SET_S(pcpu,size,cpu_mask);

	err_num = pthread_setaffinity_np(tid, size, cpu_mask);
	if(err_num) {
		/*sleep(60);  As per Linux design there is delay in updating  sysfs online file on cpu hotplug */
		if(err_num == EINVAL) {
             print_sysfs_cpu(pcpu,size,cpu_mask);
			 rc = check_cpu_exist(pcpu);
			if(rc == 0){
				CPU_FREE(cpu_mask);
				return -2;	/* Return -2 to indicate that cpu got removed due to hot plug. */
			}
			else if(rc < 0){
				CPU_FREE(cpu_mask);
				return rc;
			}	
			count = 0;
			while(count < 5){
				sleep(60);
				rc = pthread_setaffinity_np(tid, size, cpu_mask);
				if(rc){
		            print_sysfs_cpu(pcpu,size,cpu_mask);
			
						sprintf(msg,"[%d][%s]retrying to bind for pcpu=%d after waiting 60 secs,count = %d\n",__LINE__,__FUNCTION__,pcpu,count);
						hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
					count++;
				}
				else{
					CPU_FREE(cpu_mask);
					return pcpu;
				}
			}
		}
		CPU_FREE(cpu_mask);
		return -err_num;
	}
	CPU_FREE(cpu_mask);
	return pcpu;
}

/* Function to bind process. */
int htx_bind_process(int lcpu, int pcpu)
{
	int rc;

	rc = bind_process(getpid(), lcpu, pcpu);
	return rc;
}

/* Function to bind to a given process. */
int bind_process(pid_t pid, int lcpu, int pcpu)
{
	int rc, err_num,count;
	cpu_set_t *cpu_mask;
	size_t size;

	if(pcpu == -1) {
		/* Find physical cpu if pcpu value passed is -1. */
		pcpu = get_logical_2_physical(lcpu);
		if(pcpu < 0){
			return pcpu;
		}
	} else {
		/* Check if passed physical cpu is existing. */
		rc = check_cpu_exist(pcpu);
		if(rc == 0){
			return -2;
		}		
		else if(rc < 0) {
			return rc;
		}
	}

	/* Proceed with binding current thread to physical cpu. */
    cpu_mask = (cpu_set_t *)allocate_cpu_mask(&size);

    /* Proceed with binding current thread to physical cpu. */
    CPU_ZERO_S(size,cpu_mask);
    CPU_SET_S(pcpu,size,cpu_mask);
	rc = sched_setaffinity(pid, size, cpu_mask);
	if(rc) {
		sleep(60); /* As per Linux design there is delay in updating  sysfs online file on cpu hotplug */
		err_num = errno;
		if(err_num == EINVAL) {
			rc = check_cpu_exist(pcpu);
			if(rc == 0) {
				CPU_FREE(cpu_mask);
				return -2;	/* Return -2 to indicate that cpu got removed due to hot plug. */
			}
			else if(rc < 0) {
				CPU_FREE(cpu_mask);
			 	return rc;
			}
            count = 0;
            while(count < 5){
                rc = sched_setaffinity(pid, size, cpu_mask);
                if(rc){
		            print_sysfs_cpu(pcpu,size,cpu_mask);
                    	sprintf(msg,"[%d][%s]retrying to bind for pcpu=%d after waiting 60 secs,count = %d\n",__LINE__,__FUNCTION__,pcpu,count);
						hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
                    count++;
                    sleep(60);
                }
                else{
                    CPU_FREE(cpu_mask);
                    return pcpu;
                }
            }

		}
		CPU_FREE(cpu_mask);
		return -err_num;
	}
	CPU_FREE(cpu_mask);
	return pcpu;
}

/* Below function unbinds calling thread/process and restores online cpu affinity mask. */
/* Function to unbind thread. */
int htx_unbind_thread(void)
{
	int rc;
	cpu_set_t *cpu_mask;
	size_t size;

	/* Generate current online cpu mask. */
    cpu_mask = (cpu_set_t *)allocate_cpu_mask(&size);
    /* Proceed with binding current thread to physical cpu. */
    CPU_ZERO_S(size,cpu_mask);
	rc = get_online_cpu_mask(cpu_mask,size);
	if ( rc < 0) {
		return rc;
	}
	rc = pthread_setaffinity_np(pthread_self(), size, cpu_mask);
	CPU_FREE(cpu_mask);
	return ( (rc>0)?-rc:0);
}

/* Function to unbind process. */
int htx_unbind_process(void)
{
	int rc;
	cpu_set_t *cpu_mask;
	size_t size;

	/* Generate current online cpu mask. */
    cpu_mask = (cpu_set_t *)allocate_cpu_mask(&size);
    /* Proceed with binding current thread to physical cpu. */
    CPU_ZERO_S(size,cpu_mask);
	rc = get_online_cpu_mask(cpu_mask,size);
	if ( rc < 0) {
		return rc;
	}

	rc = sched_setaffinity(getpid(), size, cpu_mask);
	if(rc){
		CPU_FREE(cpu_mask);
		return -errno;
	}

	CPU_FREE(cpu_mask);
	return 0;
}


/* This function returns mask of currently online cpus. */
int get_online_cpu_mask(cpu_set_t *ptr_mask,size_t size)
{
	FILE *fp;
	int i,rc, c1, c2;
	char *tok_str, *save_ptr, cpu_str[1024],fname[1024];
	
	sprintf(fname,"/sys/devices/system/cpu/online");
    CPU_ZERO_S(size,ptr_mask);
	fp = fopen(fname,"r");
	if(fp == NULL) {
		rc = retry_open_calls(&fp,fname,"r");
		if(rc < 0){
			return rc;
		}
	}
		
	rc = fscanf(fp, "%s", cpu_str);
	if(rc==0){
		sprintf(msg,"[%d][%s]Failed to open %s with errno=%d\n",__LINE__,__FUNCTION__,fname,errno);
		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
	}

	tok_str = strtok_r(cpu_str, ",", &save_ptr);
	while(tok_str != NULL)
	{
		if(strchr(tok_str, '-') != NULL)
		{
			sscanf(tok_str, "%d-%d", &c1, &c2);
		} else {
			sscanf(tok_str, "%d", &c1);
			c2 = c1;
		}

		for(i=c1; i<=c2; i++)
			CPU_SET_S(i,size, ptr_mask);

		tok_str = strtok_r(NULL, ",", &save_ptr);
	}

	fclose(fp);

	return 0;
}


/* This function checks if a particular physical cpu is existing.
   Returns 1 if cpu is available(online) and 0 if its offline. */
int check_cpu_exist(int pcpu)
{
	FILE *fp;
	int rc,status = -1;
	char fname[128];

	sprintf(fname, "/sys/devices/system/cpu/cpu%d/online", pcpu);

	fp = fopen(fname, "r");
	if ( fp == NULL)
	{
		rc = retry_open_calls(&fp,fname,"r");
		if (rc < 0){
        	return(rc);
		}
	}
	rc = fscanf(fp,"%d", &status);
	if(rc==0){
		sprintf(msg,"[%d][%s]Failed to open %s with errno=%d\n",__LINE__,__FUNCTION__,fname,errno);
		hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
	}
    fclose(fp);
	return status;
}


/* This function returns physical cpu for a logical cpu. */
int get_logical_2_physical(int lcpu)
{
	int i,rc, c = -1;
	cpu_set_t *cpu_mask;
	size_t size;

	cpu_mask = (cpu_set_t *)allocate_cpu_mask(&size);
    /* Proceed with binding current thread to physical cpu. */
    CPU_ZERO_S(size,cpu_mask);

	rc = get_online_cpu_mask(cpu_mask,size);
	if ( rc < 0) {
		return rc;
	}

	for(i=0; i<size*8; i++)
	{
		if(CPU_ISSET_S(i,size, cpu_mask))
			c++;

		if(c == lcpu){
			CPU_FREE(cpu_mask);
			return i;
		}
	}
	print_sysfs_cpu(-1,size,cpu_mask);
	CPU_FREE(cpu_mask);
	return -1;
}

cpu_set_t* allocate_cpu_mask(size_t *size_ptr) {
	cpu_set_t *mask;
	int nrcpus;

	nrcpus  = sysconf(_SC_NPROCESSORS_CONF);
	if (nrcpus == -1) {
		perror("sysconf");
		return (cpu_set_t*)(-1);
	}

	mask = CPU_ALLOC(nrcpus);
	*size_ptr = CPU_ALLOC_SIZE(nrcpus);
	return mask;
}

int print_sysfs_cpu(int pcpu,size_t size,cpu_set_t* mask){
    FILE *fp1,*fp2;
    int i,online = -1,rc=0, global_only=0;
	char g_online[128];
    char fname1[128],fname2[128];
	long long* ptr;
	int iter;
	iter = size/sizeof(long long);


	if ( pcpu == -1 ) {
		global_only = 1;
	}

	if ( global_only == 0 ) {
		sprintf(fname1, "/sys/devices/system/cpu/cpu%d/online", pcpu);
		sprintf(fname2, "/sys/devices/system/cpu/online");
		fp1 = fopen(fname1, "r");
		fp2 = fopen(fname2, "r");
		if ( fp1 != NULL  &&  fp2 != NULL )
		{
			rc = fscanf(fp1,"%d",  &online);
			if(rc==0){
				sprintf(msg,"[%d][%s]Failed to open %s with errno=%d\n",__LINE__,__FUNCTION__,fname1,errno);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			}
			fclose(fp1);
			rc = fscanf(fp2, "%s", g_online);
			if(rc==0){
				sprintf(msg,"[%d][%s]Failed to open %s with errno=%d\n",__LINE__,__FUNCTION__,fname2,errno);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			}
			fclose(fp2);
			if((misc_htx_data != NULL)  && ((strcmp (misc_htx_data->run_type, "OTH") == 0))) {	
				sprintf(msg,"[%d][%s]sysfs contents: /sys/devices/system/cpu/cpu%d/online value is %d\n /sys/devices/system/cpu/online value is %s \n And content of cpu_set_t mask is:", 
			__LINE__,__FUNCTION__,pcpu,online,g_online); 
			}
			else if (misc_htx_data != NULL){
				sprintf(msg,"[%d][%s]sysfs contents: /sys/devices/system/cpu/cpu%d/online value is %d\n /sys/devices/system/cpu/online value is %s \n And content of cpu_set_t mask is:",
            __LINE__,__FUNCTION__,pcpu,online,g_online);
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			}
		} else {
			if(misc_htx_data == NULL){
				printf("[%d][%s]Failed to open sysfs fp1=%p,fp2=%p with errno=%d\n",__LINE__,__FUNCTION__,fp1,fp2,errno);
			}
			else{
				sprintf(msg,"[%d][%s]Failed to open sysfs fp1=%p,fp2=%p with errno=%d\n",__LINE__,__FUNCTION__,fp1,fp2,errno);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			}
			return -errno;
		}
		
		ptr = (long long*)mask;
		for(i=0;i<iter;i++){
                        char loc_msg[9];
                        snprintf(loc_msg,9,"%lld\t",*ptr);
                        strncat(msg,loc_msg,9);
                        ptr++;

		}
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
	} else {
		sprintf(fname1, "/sys/devices/system/cpu/online");
		fp1 = fopen(fname1, "r");
		if ( fp1 != NULL )
		{
			rc = fscanf(fp1, "%s", g_online);
			if(rc==0){
				sprintf(msg,"[%d][%s]Failed to open %s with errno=%d\n",__LINE__,__FUNCTION__,fname1,errno);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			}
			fclose(fp1);
				sprintf(msg,"sysfs contents: /sys/devices/system/cpu/online value is %s \n And content of cpu_set_t mask is:", g_online); 
		} else {
				sprintf(msg,"[%d][%s]Failed to open sysfs fp1=%p with errno=%d\n",__LINE__,__FUNCTION__,fp1,errno);
				hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);
			return  -errno;
		}
		
		ptr = (long long*)mask;
		for(i=0;i<iter;i++){
			char loc_msg[9];
			snprintf(loc_msg,9,"%lld\t",*ptr);
			strncat(msg,loc_msg,9);
			ptr++;
		}
			hxfmsg(misc_htx_data, 0, HTX_HE_INFO, msg);

	}
    return rc;
}
